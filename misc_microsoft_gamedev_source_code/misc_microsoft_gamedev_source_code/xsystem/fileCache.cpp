//============================================================================
//
// File: fileCache.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xsystem.h"
#include "fileCache.h"
#include "compressedStream.h"
#include "stream\byteStream.h"

typedef BDynamicArray<uchar, 4, BDynamicArrayRenderHeapAllocator> BDynamicRenderByteArray;

//============================================================================
// BFileCache::BFileCache
//============================================================================
BFileCache::BFileCache() :
   mpArchiveLoader(NULL),
   mLoadHandle(BAsyncECFArchiveLoader::cInvalidLoadHandle),
   mCacheStatus(cCacheStatusInvalid),
   mCacheStatusData(0),
   mCacheStatusChangeCounter(0),
   mNumFilesLocked(0),
   mLastCallbackMessage(BAsyncECFArchiveLoader::cCMTInvalid),
   mDispatchEventsWhileWaiting(true),
   mBlockingEnabled(true)
{
}

//============================================================================
// BFileCache::~BFileCache
//============================================================================
BFileCache::~BFileCache()
{
   deinit(true);
}

//============================================================================
// BFileCache::init
//============================================================================
bool BFileCache::init(const char* pArchiveFilename, BAsyncECFArchiveLoader* pLoader, bool dispatchEventsWhileWaiting)
{
   SCOPEDSAMPLE(BFileCache_init)

   
   BDEBUG_ASSERT(pArchiveFilename && strlen(pArchiveFilename) && pLoader);
   
   if (!deinit(false))
      return false;
      
   mMonitor.enter();
   
   mDispatchEventsWhileWaiting = dispatchEventsWhileWaiting;
   
   mArchiveFilename.set(pArchiveFilename);
   
   mpArchiveLoader = pLoader;
               
   bool success = pLoader->begin(pArchiveFilename, this, 0, mLoadHandle);
      
   eCacheStatus newStatus = cCacheStatusBusy;
   
   if (!success)
   {
      mpArchiveLoader = NULL;
      newStatus = cCacheStatusLoadFailed;
   }
   
   changeStatus(newStatus);
      
   mMonitor.leave();
   
   sendNotifications(newStatus);
   
   return success;
}

//============================================================================
// BFileCache::waitUntilAllFilesUnlocked
//============================================================================
BOOL BFileCache::waitUntilAllFilesUnlocked(void* pCallbackDataPtr, uint64 callbackData)
{
//-- FIXING PREFIX BUG ID 421
   const BFileCache* pFileCache = static_cast<BFileCache*>(pCallbackDataPtr);
//--
   BDEBUG_ASSERT(pFileCache);
   
   return 0 == pFileCache->mNumFilesLocked;
}

//============================================================================
// BFileCache::waitUntilArchiveLoaderIsRemoved
//============================================================================
BOOL BFileCache::waitUntilArchiveLoaderIsRemoved(void* pCallbackDataPtr, uint64 callbackData)
{
//-- FIXING PREFIX BUG ID 422
   const BFileCache* pFileCache = static_cast<BFileCache*>(pCallbackDataPtr);
//--
   BDEBUG_ASSERT(pFileCache);

   return NULL == pFileCache->mpArchiveLoader;
}

//============================================================================
// BFileCache::deinit
//============================================================================
bool BFileCache::deinit(bool ignoreLockedFiles)
{
   mMonitor.enter();
   
   if (mpArchiveLoader)
   {
      bool success = mpArchiveLoader->cancel(mLoadHandle, false);

      if (!success)
      {
         trace("BFileCache::deinit: Archive load cancel failed!");
      }

      mMonitor.waitForCondition(waitUntilArchiveLoaderIsRemoved, this, 0, INVALID_HANDLE_VALUE, INFINITE, mDispatchEventsWhileWaiting);
   }
   
   if (!ignoreLockedFiles)
   {
      if (mNumFilesLocked)
      {
         trace("BFileCache::deinit: Waiting for all files to be unlocked!");

         if (mMonitor.waitForCondition(waitUntilAllFilesUnlocked, this, 0, INVALID_HANDLE_VALUE, 3000, mDispatchEventsWhileWaiting) < 0)
         {
            trace("BFileCache::deinit: Wait timed out!");
         }

         if (mNumFilesLocked)
            return false;
      }
   }
      
   mArchiveFilename.empty();
   mpArchiveLoader = NULL;
   mLoadHandle = BAsyncECFArchiveLoader::cInvalidLoadHandle;
   
   mFilenameTree.clear();
   for (uint i = 0; i < mFileInfoArray.getSize(); i++)
      mFileInfoArray[i].mAllocation.free();
   mFileInfoArray.clear();
   
   mCacheStatus = cCacheStatusInvalid;
   mCacheStatusData = 0;
   mCacheStatusChangeCounter++;
   
   mNumFilesLocked = 0;
   mLastCallbackMessage = BAsyncECFArchiveLoader::cCMTInvalid;
   
   mMonitor.leave();
      
   sendNotifications(cCacheStatusInvalid, 0);
   
   // Give any waiting threads some time to clean up.
   gEventDispatcher.sleep(10);
   
   return true;
}

//============================================================================
// BFileCache::getNumLockedFiles
//============================================================================
uint BFileCache::getNumLockedFiles(void)
{
   mMonitor.enter();
   
   const uint numLockedFiles = mNumFilesLocked;
   
   mMonitor.leave();
   
   return numLockedFiles;
}

//============================================================================
// BFileCache::message
//============================================================================
void BFileCache::message(uint64 callbackData, BAsyncECFArchiveLoader::eCallbackMessageType messageType)
{
   if (messageType == BAsyncECFArchiveLoader::cCMTLoadBegun)
      return;
      
   mMonitor.enter();
   
   mLastCallbackMessage = messageType;
   
   eCacheStatus newStatus = mCacheStatus;
    
   switch (messageType)
   {  
      case BAsyncECFArchiveLoader::cCMTDeletingRequest:
      {
         mpArchiveLoader = NULL;
         mLoadHandle = BAsyncECFArchiveLoader::cInvalidLoadHandle;
         
         if ((mCacheStatus != cCacheStatusLoadFailed) && (mCacheStatus != cCacheStatusLoadSucceeded) && (mCacheStatus != cCacheStatusLoadCanceled))
            newStatus = cCacheStatusLoadFailed;
         
         break;
      }
      case BAsyncECFArchiveLoader::cCMTLoadFinished:
      {
         newStatus = cCacheStatusLoadSucceeded;
         break;
      }
      case BAsyncECFArchiveLoader::cCMTErrorLoadCanceled:
      {
         newStatus = cCacheStatusLoadCanceled;
         break;
      }

      // JAR - trying to track down a FATAL
      case BAsyncECFArchiveLoader::cCMTErrorOpenFailed:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorOpenFailed");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorReadFailed:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorReadFailed");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorBadSignature:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorBadSignature");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorBadArchive:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorBadArchive");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorBadChunk:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorBadChunk");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorChunkAllocFailed:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorChunkAllocFailed");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorChunkReadyCallbackFailed:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorChunkReadyCallbackFailed");
#endif
            break;
         }
      case BAsyncECFArchiveLoader::cCMTErrorHeaderDataCallbackFailed:
         {
            newStatus = cCacheStatusLoadFailed;
#ifndef BUILD_FINAL
            BFATAL_FAIL("BFileCache::message : cCMTErrorHeaderDataCallbackFailed");
#endif
            break;
         }
      // JAR - trying to track down a FATAL ^^^

      default:
      {
         // load failed, invalid message type or a new case was added. Pass this
         // along for a generic failure later.
         newStatus = cCacheStatusLoadFailed;
         break;
      }
   }
   
   changeStatus(newStatus);
   
   mMonitor.leave();
   
   sendNotifications(newStatus);
}

//============================================================================
// BFileCache::headerData
//============================================================================
bool BFileCache::headerData(uint64 callbackData, const void* p, uint size)
{
   callbackData;
   size;
   
   const BECFArchiveHeader& header = *static_cast<const BECFArchiveHeader*>(p);
   const uint numChunks = header.getNumChunks();
   const BECFArchiveChunkHeader* pChunkHeaders = reinterpret_cast<const BECFArchiveChunkHeader*>(static_cast<const BYTE*>(p) + header.getSize());
   
   if (numChunks < 2)
      return false;
      
   const uint totalFiles = numChunks - 1;      

   BFileInfoArray fileInfoArray;
   fileInfoArray.resize(totalFiles);
   
   for (uint fileIndex = 0; fileIndex < totalFiles; fileIndex++)
   {
      const BECFArchiveChunkHeader& chunkHeader = pChunkHeaders[1 + fileIndex];
      
      BFileInfo& fileInfo = fileInfoArray[fileIndex];
      fileInfo.mDate          = chunkHeader.mDate;
      fileInfo.mDecompSize    = chunkHeader.mDecompSize;
      fileInfo.mCompSize      = chunkHeader.getSize();
      fileInfo.mDecompTiger64 = chunkHeader.getID();
      fileInfo.mCompMethod    = static_cast<eECFArchiveCompMethod>(chunkHeader.getFlags() & cECFCompMethodMask);
      fileInfo.mNameOfs       = (chunkHeader.mNameOfs[0] << 16U) | (chunkHeader.mNameOfs[1] << 8U) | chunkHeader.mNameOfs[2];
   }
           
   mMonitor.enter();
         
   mFileInfoArray.swap(fileInfoArray);
   
   changeStatus(cCacheStatusHeadersReady);
   
   mMonitor.leave();
      
   for (uint i = 0; i < fileInfoArray.getSize(); i++)
      fileInfoArray[i].mAllocation.free();
   
   sendNotifications(cCacheStatusHeadersReady);
   
   fileInfoArray.clear();

   return true;
}

//============================================================================
// BFileCache::chunkAlloc
//============================================================================
bool BFileCache::chunkAlloc(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation, uint chunkSize)
{
   return allocation.alloc(chunkSize, &gFileBlockHeap);
}

//============================================================================
// BFileCache::processFilenameChunk
//============================================================================
bool BFileCache::processFilenameChunk(BMemoryHeapAllocation& allocation)
{
#ifdef XBOX
   if ((!allocation.getPtr()) || (!allocation.getSize()))
   {
      allocation.free();
      return false;
   }
      
   BByteStream byteStream(allocation.getPtr(), allocation.getSize());
   
   BInflateStream inflStream;
   if (!inflStream.open(byteStream))
   {
      allocation.free();
      return false;
   }
      
   if (!inflStream.sizeKnown())
   {
      allocation.free();
      return false;
   }
      
   const uint cMaxFilenameDataChunkSize = 16 * 1024 * 1024;
   if (inflStream.size() > UINT_MAX)
   {
      allocation.free();
      return false;
   }
      
   const uint filenameDataSize = static_cast<uint>(inflStream.size());
   if (filenameDataSize > cMaxFilenameDataChunkSize)
   {
      allocation.free();
      return false;      
   }
 
   // + 1 so the final byte in the array is always 0 (null terminator)
   BDynamicRenderByteArray filenameData(filenameDataSize + 1);
   
   if (!inflStream.readBytes(filenameData.getPtr(), filenameDataSize))
   {
      allocation.free();
      return false;
   }
      
   if (!inflStream.close())
   {
      allocation.free();
      return false;
   }
   
   allocation.free();
   
   {
      BScopedMonitor lock(mMonitor);
      
      if (mFileInfoArray.isEmpty())
         return false;
      
      mFilenameTree.clear();
      
      for (uint fileIndex = 0; fileIndex < mFileInfoArray.size(); fileIndex++)
      {
         BFileInfo& fileInfo = mFileInfoArray[fileIndex];
         
         if (fileInfo.mNameOfs >= filenameDataSize)
            return false;
         
         const char* pFilename = (const char*)&filenameData[fileInfo.mNameOfs];
         const uint filenameLen = strlen(pFilename);
         if (filenameLen >= MAX_PATH)
            return false;
         
         if (BFilenameTree::cSucceeded != mFilenameTree.add(pFilename, 0, fileIndex, true, &fileInfo.mTreeNodeIndex))  
            return false;
      }
      
      changeStatus(cCacheStatusDirectoryReady);
   }      
   
   sendNotifications(cCacheStatusDirectoryReady);
#endif // XBOX
   return true;
}

//============================================================================
// BFileCache::processDataChunk
//============================================================================
bool BFileCache::processDataChunk(uint chunkIndex, BMemoryHeapAllocation& allocation)
{
   BDEBUG_ASSERT(chunkIndex);
   
   const uint fileIndex = chunkIndex - 1;
   
   {
      BScopedMonitor lock(mMonitor);
            
      if (fileIndex >= mFileInfoArray.getSize())
      {
         allocation.free();
         return false;
      }
         
      BFileInfo& fileInfo = mFileInfoArray[fileIndex];
      
      if (allocation.getSize() != fileInfo.mCompSize)
      {
         allocation.free();
         return false;
      }
      
      fileInfo.mAllocation.free();
      fileInfo.mAllocation = allocation;
      
      fileInfo.mStatus = cFileStatusLoaded;
      
      changeStatus(cCacheStatusReceivedFile, fileIndex);
   }
   
   sendNotifications(cCacheStatusReceivedFile, fileIndex);      
         
   return true;
}

//============================================================================
// BFileCache::chunkReady
//============================================================================
bool BFileCache::chunkReady(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation)
{
   if (!chunkIndex)
      return processFilenameChunk(allocation);
   else
      return processDataChunk(chunkIndex, allocation);
}

//============================================================================
// BFileCache::changeStatus
//============================================================================
void BFileCache::changeStatus(eCacheStatus newStatus, int statusData)
{
   mCacheStatus = newStatus;
   mCacheStatusData = statusData;
   mCacheStatusChangeCounter++;
}

//============================================================================
// BFileCache::addNotifyCallback
//============================================================================
void BFileCache::addNotifyCallback(BNotifyCallbackPtr pCallback, void* pCallbackDataPtr, uint64 callbackData, BNotifyCallbackHandle& handle)
{
   BScopedLightWeightMutex lock(mCallbackMutex);

   for (uint i = 0; i < mCallbacks.getSize(); i++)
   {
      if (!mCallbacks[i].mpCallback)
      {
         handle = i + 1;
         mCallbacks[i].mpCallback = pCallback;
         mCallbacks[i].mpCallbackDataPtr = pCallbackDataPtr;
         mCallbacks[i].mCallbackData = callbackData;
         return;
      }
   }
   
   handle = mCallbacks.getSize() + 1; 

   mCallbacks.pushBack(BRegisteredCallback(pCallback, pCallbackDataPtr, callbackData));
}

//============================================================================
// BFileCache::removeNotifyCallback
//============================================================================
bool BFileCache::removeNotifyCallback(BNotifyCallbackHandle handle)
{
   BScopedLightWeightMutex lock(mCallbackMutex);
   
   if ((handle < 1) || (handle > mCallbacks.getSize()))
      return false;
      
   handle--;
   
   if (!mCallbacks[handle].mpCallback)   
      return false;
      
   if (handle == (mCallbacks.getSize() - 1)) 
      mCallbacks.popBack();
   else
   {
      mCallbacks[handle].mpCallback = NULL;
      mCallbacks[handle].mpCallbackDataPtr = NULL;
      mCallbacks[handle].mCallbackData = 0;
   }
   
   return true;
}

//============================================================================
// BFileCache::sendNotifications
//============================================================================
void BFileCache::sendNotifications(eCacheStatus newStatus, int statusData)
{
   BScopedLightWeightMutex lock(mCallbackMutex);
   
   for (uint i = 0; i < mCallbacks.getSize(); i++)
   {
      if (mCallbacks[i].mpCallback)
      {
         (*mCallbacks[i].mpCallback)(newStatus, statusData, mCallbacks[i].mpCallbackDataPtr, mCallbacks[i].mCallbackData);
      }
   }
}

//============================================================================
// BFileCache::getStatus
//============================================================================
BFileCache::eCacheStatus BFileCache::getStatus(int* pStatusData, uint* pNumFilesLocked)
{
   BScopedMonitor lock(mMonitor);
   
   if (pStatusData)
      *pStatusData = mCacheStatusData;
   
   if (pNumFilesLocked)
      *pNumFilesLocked = mNumFilesLocked;
      
   return mCacheStatus;
}

//============================================================================
// BFileCache::getArchiveFilename
//============================================================================
void BFileCache::getArchiveFilename(BString& filename)
{
   BScopedMonitor lock(mMonitor);
   filename.set(mArchiveFilename);
}

//============================================================================
// BFileCache::waitUntilStatusChangesOrFinishedCallback
//============================================================================
BOOL BFileCache::waitUntilStatusChangesOrFinishedCallback(void* pCallbackDataPtr, uint64 callbackData)
{
//-- FIXING PREFIX BUG ID 423
   const BFileCache* pFileCache = static_cast<BFileCache*>(pCallbackDataPtr);
//--
   BDEBUG_ASSERT(pFileCache);

   BFileCache::eCacheStatus curStatus = pFileCache->mCacheStatus;
   if ((curStatus < 0) || (curStatus == cCacheStatusLoadSucceeded))
      return TRUE;

   const uint oldCount = static_cast<uint>(callbackData);

   const uint curCount = pFileCache->mCacheStatusChangeCounter;

   return oldCount != curCount;
}

//============================================================================
// BFileCache::waitUntilStatusChangesOrFinished
//============================================================================
void BFileCache::waitUntilStatusChangesOrFinished(eCacheStatus& newStatus, int& newStatusData)
{
   mMonitor.enter();

   mMonitor.waitForCondition(waitUntilStatusChangesOrFinishedCallback, this, mCacheStatusChangeCounter, INVALID_HANDLE_VALUE, INFINITE, false);
   
   newStatus = mCacheStatus;
   newStatusData = mCacheStatusData;

   mMonitor.leave();
}

//============================================================================
// BFileCache::wait
//============================================================================
bool BFileCache::wait(eCacheStatus desiredCacheStatus)
{
   mMonitor.enter();

   const bool success = waitForStatusOrFailure(desiredCacheStatus, true);

   mMonitor.leave();

   return success;
}

//============================================================================
// BFileCache::wait
//============================================================================
bool BFileCache::wait(BFileHandle fileHandle)
{
   if (fileHandle < 1)
      return false;
   
   const uint fileIndex = fileHandle - 1;
   
   mMonitor.enter();

   const bool success = waitForFileOrFailure(fileIndex, true);

   mMonitor.leave();

   return success;
}

//============================================================================
// BFileCache::isFileLoaded
//============================================================================
bool BFileCache::isFileLoaded(BFileHandle fileHandle)
{
   if (fileHandle < 1)
      return false;

   const uint fileIndex = fileHandle - 1;
   
   bool success = false;

   mMonitor.enter();

   if (mCacheStatus >= cCacheStatusDirectoryReady)
   {
      if (cCacheStatusLoadSucceeded == mCacheStatus)
         success = true;
      else if (fileIndex < mFileInfoArray.getSize())
         success = mFileInfoArray[fileIndex].mStatus >= cFileStatusLoaded;
   }

   mMonitor.leave();

   return success;
}

//============================================================================
// BFileCache::waitForStatusOrFailure
//============================================================================
BOOL BFileCache::waitForStatusOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData)
{
//-- FIXING PREFIX BUG ID 424
   const BFileCache* pFileCache = static_cast<BFileCache*>(pCallbackDataPtr);
//--
   BDEBUG_ASSERT(pFileCache);
   
   eCacheStatus desiredStatus = static_cast<eCacheStatus>(callbackData);
   
   eCacheStatus curStatus = pFileCache->mCacheStatus;
   
   return ((curStatus < 0) || (curStatus >= desiredStatus));
}

//============================================================================
// BFileCache::waitForStatusOrFailure
//============================================================================
bool BFileCache::waitForStatusOrFailure(eCacheStatus desiredCacheStatus, bool blocking)
{
   if (mCacheStatus < 0)
      return false;
   else if (mCacheStatus >= desiredCacheStatus)
      return true;
      
   if (!blocking)
      return false;
   
   mMonitor.waitForCondition(waitForStatusOrFailureCallback, this, desiredCacheStatus, INVALID_HANDLE_VALUE, INFINITE, mDispatchEventsWhileWaiting);
   
   return mCacheStatus >= desiredCacheStatus;
}

//============================================================================
// BFileCache::waitForChunkOrFailureCallback
//============================================================================
BOOL BFileCache::waitForFileOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData)
{
   BFileCache* pFileCache = static_cast<BFileCache*>(pCallbackDataPtr);
   BDEBUG_ASSERT(pFileCache);

   const uint desiredFileIndex = static_cast<uint>(callbackData);

   eCacheStatus curStatus = pFileCache->mCacheStatus;

   if (curStatus < 0)
      return TRUE;
   else if (curStatus == cCacheStatusLoadSucceeded)
      return TRUE;
   else if (curStatus >= cCacheStatusDirectoryReady)
   {
      if (desiredFileIndex >= pFileCache->mFileInfoArray.getSize())
         return TRUE;
      else if (pFileCache->mFileInfoArray[desiredFileIndex].mStatus >= cFileStatusLoaded)
         return TRUE;
   }
   
   return FALSE;
}

//============================================================================
// BFileCache::getNumFiles
//============================================================================
bool BFileCache::waitForFileOrFailure(uint fileIndex, bool blocking)
{
   if (mCacheStatus < 0)
      return false;
   else if (cCacheStatusLoadSucceeded == mCacheStatus)
      return true;
   else if (mCacheStatus >= cCacheStatusDirectoryReady)
   {
      if (fileIndex >= mFileInfoArray.getSize())
         return false;
      else if (mFileInfoArray[fileIndex].mStatus >= cFileStatusLoaded)
         return true;
   }         
   
   if (!blocking)
      return false;

   mMonitor.waitForCondition(waitForFileOrFailureCallback, this, fileIndex, INVALID_HANDLE_VALUE, INFINITE, mDispatchEventsWhileWaiting);
   
   if (mCacheStatus >= cCacheStatusLoadSucceeded)
      return true;
   else if (mCacheStatus >= cCacheStatusDirectoryReady)
   {
      if (fileIndex < mFileInfoArray.getSize())
      {
         if (mFileInfoArray[fileIndex].mStatus >= cFileStatusLoaded)
            return true;
      }            
   } 
   
   return false;
}

//============================================================================
// BFileCache::getNumFiles
//============================================================================
int BFileCache::getNumFiles(void)
{
   BScopedMonitor lock(mMonitor);
      
   if (!waitForStatusOrFailure(cCacheStatusHeadersReady, mBlockingEnabled))
      return -1;
   
   return mFileInfoArray.getSize();
}

//============================================================================
// BFileCache::getFileByIndex
//============================================================================
BFileCache::BFileHandle BFileCache::getFileByIndex(uint fileIndex)
{
   BScopedMonitor lock(mMonitor);
   
   if (!waitForStatusOrFailure(cCacheStatusHeadersReady, mBlockingEnabled))
      return 0;
      
   if (fileIndex >= mFileInfoArray.getSize())
      return cInvalidFileHandle;
   
   return fileIndex + 1;
}

//============================================================================
// BFileCache::getFileDesc
//============================================================================
bool BFileCache::getFileDesc(BFileHandle fileHandle, BFileDesc& desc)
{
   if (fileHandle < 1)
      return false;

   const uint fileIndex = fileHandle - 1;      

   BScopedMonitor lock(mMonitor);

   if (!waitForStatusOrFailure(cCacheStatusDirectoryReady, mBlockingEnabled))
      return false;

//-- FIXING PREFIX BUG ID 425
   const BFileInfo& fileInfo = mFileInfoArray[fileIndex];
//--

   desc.mStatus         = fileInfo.mStatus;
   desc.mLockCount      = fileInfo.mLockCount;
   desc.mCompMethod     = fileInfo.mCompMethod;
   desc.mCompSize       = fileInfo.mCompSize;
   desc.mDecompSize     = fileInfo.mDecompSize;
   desc.mDate           = fileInfo.mDate;
   desc.mDecompTiger64  = fileInfo.mDecompTiger64;
   desc.mHandle         = fileHandle;
   
   if (!mFilenameTree.getNodeFullName(fileInfo.mTreeNodeIndex, desc.mFullName))
      return false;
   
   return true;
}

//============================================================================
// BFileCache::findFileByName
//============================================================================
BFileCache::BFileHandle BFileCache::findFileByName(const char* pFilename)
{
   BScopedMonitor lock(mMonitor);
   
   if (!waitForStatusOrFailure(cCacheStatusDirectoryReady, mBlockingEnabled))
      return cInvalidFileHandle;
      
   const BFilenameTree::BNodeIndex nodeIndex = mFilenameTree.findNode(pFilename);
   if (nodeIndex == BFilenameTree::cInvalidNodeIndex)
      return cInvalidFileHandle;
      
   if (mFilenameTree.getFileFlags(nodeIndex) & BFilenameTree::cDirFlag)
      return cInvalidFileHandle;
   
   const uint fileIndex = (uint)mFilenameTree.getFileData(nodeIndex);
   BDEBUG_ASSERT(fileIndex < mFileInfoArray.getSize());
   
   return fileIndex + 1;
}

//============================================================================
// BFileCache::findFiles
//============================================================================
bool BFileCache::findFiles(const char* pPath, const char* pSpec, bool wantFiles, bool wantDirs, bool recurse, BFindFilesInfoArray& results)
{
   BScopedMonitor lock(mMonitor);

   if (!waitForStatusOrFailure(cCacheStatusDirectoryReady, mBlockingEnabled))
      return false;
   
   uint flags = wantFiles ? BFilenameTree::cFindFilesWantFiles : 0;
   if (wantDirs) 
      flags |= BFilenameTree::cFindFilesWantDirs;
   if (recurse)
      flags |= BFilenameTree::cFindFilesRecurse;
      
   if (!flags)
      return false;      
   
   BFilenameTree::BFindFilesResults treeResults;
   if (mFilenameTree.findFiles(pPath, pSpec, flags, treeResults) < 0)
      return false;
      
   const uint firstResultIndex = results.getSize();
   results.resize(firstResultIndex + treeResults.getSize());      
   
   for (uint i = 0; i < treeResults.getSize(); i++)
   {
      const uint fileIndex = (uint)mFilenameTree.getFileData(treeResults[i].mNodeIndex);
      const bool isDir = (treeResults[i].mFileFlags & BFilenameTree::cDirFlag) != 0;
      
      BDEBUG_ASSERT(fileIndex < mFileInfoArray.getSize());
//-- FIXING PREFIX BUG ID 426
      const BFileInfo& fileInfo = mFileInfoArray[fileIndex];
//--

      BFindFilesInfo& findFilesInfo = results[firstResultIndex + i];

      findFilesInfo.mFullName.swap(treeResults[i].mFullname);
      findFilesInfo.mRelPath.swap(treeResults[i].mRelPath);
      findFilesInfo.mName.swap(treeResults[i].mName);      
      findFilesInfo.mFileHandle = 1 + fileIndex;
      findFilesInfo.mIsDir = isDir;
      findFilesInfo.mDate = isDir ? 0 : fileInfo.mDate;
   }

   return true;
}

//============================================================================
// BFileCache::lockFileData
//============================================================================
bool BFileCache::lockFileData(BFileHandle fileHandle, BConstDataBuffer& dataBuffer)
{
   if (fileHandle < 1)
      return false;
      
   const uint fileIndex = fileHandle - 1;      
      
   BScopedMonitor lock(mMonitor);

   if (!waitForFileOrFailure(fileIndex, mBlockingEnabled))
      return false;
      
   BFileInfo& fileInfo = mFileInfoArray[fileIndex];
      
   if (fileInfo.mStatus != cFileStatusLoaded)
      return false;
   
   if (fileInfo.mLockCount == 255)
      return false;
   
   if (!fileInfo.mLockCount)
      mNumFilesLocked++;
      
   fileInfo.mLockCount++;
         
   dataBuffer.setPtr(fileInfo.mAllocation.getPtr());
   dataBuffer.setLen(fileInfo.mAllocation.getSize());
         
   return true;
}

//============================================================================
// BFileCache::unlockFileData
//============================================================================
bool BFileCache::unlockFileData(BFileHandle fileHandle)
{
   if (fileHandle < 1)
      return false;
      
   const uint fileIndex = fileHandle - 1;      

   BScopedMonitor lock(mMonitor);
   
   BFileInfo& fileInfo = mFileInfoArray[fileIndex];
   
   BDEBUG_ASSERT(fileInfo.mStatus == cFileStatusLoaded);

   if (!fileInfo.mLockCount)
      return false;

   fileInfo.mLockCount--;

   if (!fileInfo.mLockCount)
      mNumFilesLocked--;
                 
   return true;      
}

//============================================================================
// BFileCache::getFileStatus
//============================================================================
bool BFileCache::getFileStatus(BFileHandle fileHandle, BFileCache::eFileStatus& status, uint& lockCount)
{
   if (fileHandle < 1)
      return false;

   const uint fileIndex = fileHandle - 1;      

   BScopedMonitor lock(mMonitor);

   if (!waitForStatusOrFailure(cCacheStatusHeadersReady, mBlockingEnabled))
      return false;

   if (fileIndex >= mFileInfoArray.getSize())
      return false;
      
//-- FIXING PREFIX BUG ID 427
   const BFileInfo& fileInfo = mFileInfoArray[fileIndex];
//--
   
   status = fileInfo.mStatus;
   lockCount = fileInfo.mLockCount;
      
   return true;
}

//============================================================================
// BFileCache::discardFileData
//============================================================================
bool BFileCache::discardFileData(BFileHandle fileHandle)
{
   if (fileHandle < 1)
      return false;

   const uint fileIndex = fileHandle - 1;      

   BScopedMonitor lock(mMonitor);

   if (!waitForFileOrFailure(fileIndex, mBlockingEnabled))
      return false;

   BFileInfo& fileInfo = mFileInfoArray[fileIndex];

   if (fileInfo.mStatus != cFileStatusLoaded)
      return false;

   if (fileInfo.mLockCount)
      return false;
   
   fileInfo.mAllocation.free();
   fileInfo.mStatus = cFileStatusDiscarded;

   return true;      
}

//============================================================================
// BFileCache::claimFileData
//============================================================================
bool BFileCache::claimFileData(BFileHandle fileHandle, BMemoryHeapAllocation& allocation)
{
   if (fileHandle < 1)
      return false;

   const uint fileIndex = fileHandle - 1;      

   BScopedMonitor lock(mMonitor);

   if (!waitForFileOrFailure(fileIndex, mBlockingEnabled))
      return false;

   BFileInfo& fileInfo = mFileInfoArray[fileIndex];

   if (fileInfo.mStatus != cFileStatusLoaded)
      return false;

   if (fileInfo.mLockCount)
      return false;

   allocation = fileInfo.mAllocation;
   fileInfo.mAllocation.clear();
   fileInfo.mStatus = cFileStatusDiscarded;

   return true;
}

//============================================================================
// BFileCache::getDispatchEventsWhileWaiting
//============================================================================
bool BFileCache::getDispatchEventsWhileWaiting(void)
{
   BScopedMonitor lock(mMonitor);
   
   return mDispatchEventsWhileWaiting;
}

//============================================================================
// BFileCache::setDispatchEventsWhileWaiting
//============================================================================
void BFileCache::setDispatchEventsWhileWaiting(bool dispatchEventsWhileWaiting)
{
   BScopedMonitor lock(mMonitor);
   
   mDispatchEventsWhileWaiting = dispatchEventsWhileWaiting;
}

