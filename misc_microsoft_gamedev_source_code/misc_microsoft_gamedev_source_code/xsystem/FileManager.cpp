//============================================================================
//
//  FileManager.cpp
//
//  Copyright 2002-2006 Ensemble Studios
// 
//============================================================================
#include "xsystem.h"
//#include "fileManager.h"
#include "workdirsetup.h"
#include "file\lowLevelFileIO.h"
#include "file\win32FindFiles.h"
#include "file\win32FileStream.h"
#include "stream\byteStream.h"
#include "stream\bufferStream.h"
#include "rawInflateStream.h"
#include "fileUtils.h"

#ifdef XBOX


// Call this when there's a read error loading archives
// this is actually defined in "..\xgamerender\xgamerender.h"
void DiskReadFailAlert(void);
#endif

#pragma warning(disable: 4995)

//#ifndef XBOX
//   #error This module is currently Xbox only.
//#endif

namespace
{
   template<class StringType>
   void joinPathWithFilename(StringType& path, const char* pFilename)
   {
      if (!path.length())
      {
         path.set(pFilename);
         return;
      }

      if ((pFilename[0] == '\\') || (pFilename[0] == '/'))
         pFilename++;

      path.append(pFilename);
   }
   
   bool isValidFilename(const char* pFilename)
   {  
      if ((strchr(pFilename, '*') != NULL) || (strchr(pFilename, '?') != NULL) || (strchr(pFilename, ':') != NULL))
         return false;
      return true;
   }
   
   void createLooseFilename(BFixedStringMaxPath& looseFilename, const char* pFullFilename, bool writeable)
   {
      BDEBUG_ASSERT(looseFilename.getPtr() != pFullFilename);
      
#ifdef XBOX
      looseFilename.set(writeable ? BFileUtils::getXboxTempPath() : BFileUtils::getXboxGamePath());
#endif // XBOX
      joinPathWithFilename(looseFilename, pFullFilename);
   }
}

//============================================================================
// BFileManager::BFileManager
//============================================================================
BFileManager::BFileManager() :
   mConfigFlags(0),
   mInitialized(false),
   mTotalFileCaches(0),
   mNotifyQueue(512),
   mNotifyQueueLock(512),
   mMonitor(512, false),
   mIOPauseCounter(0)
{
   // do not do anything here!
}

//============================================================================
// BFileManager::~BFileManager
//============================================================================
BFileManager::~BFileManager()
{
   // do not do anything here!
}

//============================================================================
// BFileManager::init
//============================================================================
void BFileManager::init(uint configFlags, uint numPublicKeys, const BSHA1* pPublicKeys, const uint64 decryptKey[3])
{
   BScopedMonitor lock(mMonitor);
         
   if (mInitialized)
      return;
   
   ILowLevelFileIO* pLowLevelFileIO = ILowLevelFileIO::getDefault();
   if (configFlags & cBypassXFSForArchives)
      pLowLevelFileIO = &gWin32LowLevelFileIO;

   const uint cArchiveLoaderBufSize = (uint)(4.0f * 1024U * 1024U);
   
#ifdef XBOX
   mAsyncArchiveLoader.init(cThreadIndexIO, numPublicKeys, pPublicKeys, decryptKey, pLowLevelFileIO, cArchiveLoaderBufSize);
#endif // XBOX

   mActiveFiles.reserve(8);
   mActiveCaches.reserve(8);
   mWorkDirs.reserve(16);
   
   mConfigFlags = configFlags;
   
   mIOPauseCounter = 0;
   mInitialized = true;
   
   // The base dir ID can never be modified by the application, but the 
   // production dir ID can.
   long baseDirID;
   createDirList("", baseDirID);
   BVERIFY(cBaseDirListID == baseDirID);

   long prodDirID;
   createDirList("", prodDirID);
   BVERIFY(cProductionDirListID == prodDirID);
}

//============================================================================
// BFileManager::deinit
//============================================================================
eFileManagerError BFileManager::deinit(void)
{
   BScopedMonitor lock(mMonitor);
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();
   
   for (uint i = 0; i < mActiveFiles.getSize(); i++)
      if (mActiveFiles[i].mpFile)
         return cFME_FILES_STILL_OPEN;
   
   for (uint i = 0; i < mActiveCaches.getSize(); i++)
   {
      if (mActiveCaches[i].mpFileCache) 
      {
         eFileManagerError error = removeFileCache(mActiveCaches[i].mHandle);
         if (error != cFME_SUCCESS)
            return error;
      }
   }
   
   mAsyncArchiveLoader.deinit();
   
   for (uint i = 0; i < mWorkDirs.getSize(); i++)
      delete mWorkDirs[i];

   mWorkDirs.clear();
   
   mInitialized = false;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::saveFileOpenLog
//==============================================================================
eFileManagerError BFileManager::saveFileOpenLog(long dirID, const char* pFilename)
{
#ifdef XBOX  
   BScopedMonitor lock(mMonitor);
 
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
   
   if ((mConfigFlags & cRecordOpenedFiles) == 0)
      return cFME_NOT_RECORDING_OPEN_FILES;
      
   BFixedStringMaxPath fullPath;
   eFileManagerError error = gFileManager.constructQualifiedPath(dirID, pFilename, fullPath, BFileManager::cNoSearch, BFileManager::cIgnoreFileCaches);
   if (error != cFME_SUCCESS)
      return error;

   BFixedStringMaxPath looseFilename;
   createLooseFilename(looseFilename, fullPath, true);

   ILowLevelFileIO::getDefault()->setFileAttributes(looseFilename, FILE_ATTRIBUTE_NORMAL);

   BWin32FileStream* pWin32Stream = new BWin32FileStream;
   if (!pWin32Stream->open(looseFilename.getPtr(), cSFWritable | cSFOptimizeForSequentialAccess))
   {
      delete pWin32Stream;
      return cFME_OPEN_FAILED;
   }
      
   BBufferStream bufferStream(*pWin32Stream);
   
   for (uint i = 0; i < mOpenedFiles.getSize(); i++)
      bufferStream.printf("%s\n", mOpenedFiles[i].getPtr());
   
   if (!bufferStream.close())
   {
      delete pWin32Stream;
      return cFME_WRITE_FAILED;
   }
   
   if (!pWin32Stream->close())
   {
      delete pWin32Stream;
      return cFME_WRITE_FAILED;
   }
   
   delete pWin32Stream;
      
   return cFME_SUCCESS;
#else
   return cFME_WRITE_FAILED;
#endif // XBOX
}

//==============================================================================
// BFileManager::clearFileOpenLog
//==============================================================================
void BFileManager::clearFileOpenLog(void)
{
   BScopedMonitor lock(mMonitor);
   
   mOpenedFiles.clear();
}

//==============================================================================
// BFileManager::enableFileOpenLogging
//==============================================================================
void BFileManager::enableFileOpenLogging(bool enabled)
{
   BScopedMonitor lock(mMonitor);              
   
   bool currentlyEnabled = (mConfigFlags & cRecordOpenedFiles) != 0;
   if (enabled == currentlyEnabled)
      return;
   
   if (!enabled)
   {
      mConfigFlags &= ~cRecordOpenedFiles;   
      mOpenedFiles.clear();
   }
   else
   {
      mConfigFlags |= cRecordOpenedFiles;
   }
}

//==============================================================================
// BFileManager::flushLogMessages
//==============================================================================
void BFileManager::flushLogMessages(void)
{
#ifndef BUILD_FINAL   
   BDynamicArray<BRenderString, ALIGN_OF(BRenderString), BDynamicArrayRenderHeapAllocator> logMessages;
   
   {
      BScopedLightWeightMutex lock(mLogMessagesMutex);
      mLogMessages.swap(logMessages);
   }

   for (uint i = 0; i < logMessages.getSize(); i++)
      gConsoleOutput.fileManager("%s", logMessages[i].getPtr());
#endif   
}

//==============================================================================
// BFileManager::flushOpenFiles
//==============================================================================
void BFileManager::flushOpenFiles(void)
{      
   mMonitor.enter();
         
   for (uint i = 0; i < mActiveFiles.getSize(); i++)
   {
      if (!mActiveFiles[i].mpFile)
         continue;
      
      __try
      {
         // This call is not thread safe!!
         mActiveFiles[i].mpFile->flush();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
      }
   }  
   
   mMonitor.leave();
}

//==============================================================================
// BFileManager::getStatus
//==============================================================================
eFileManagerError BFileManager::getStatus(BFileManager::BStatus& status)
{
   status.clear();
   
   BScopedMonitor lock(mMonitor);
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();
   
   status.mNumWorkDirs = mWorkDirs.getSize();
   
   for (uint i = 0; i < mActiveFiles.getSize(); i++)
   {
      if (mActiveFiles[i].mpFile)
      {
         status.mNumActiveFiles++;
         
         if (mActiveFiles[i].mFileCacheIndex < 0)
            status.mNumActiveLooseFiles++;
      }
   }
   
   for (uint i = 0; i < mActiveCaches.getSize(); i++)
   {
      const BActiveFileCache& activeFileCache = mActiveCaches[i];
      
      if (!activeFileCache.mpFileCache)
         continue;
      
      status.mNumActiveCaches++;
      
      if (activeFileCache.mEnabled)
         status.mNumCachesEnabled++;
      
      if (activeFileCache.mLockCount)
         status.mNumCachesLocked++;
         
      switch (activeFileCache.mLastStatus)
      {
         case BFileCache::cCacheStatusBusy:
         {
            status.mNumCachesBusy++;
            break;
         }
         case BFileCache::cCacheStatusHeadersReady:
         {
            status.mNumCachesHeadersReady++;
            break;
         }
         case BFileCache::cCacheStatusDirectoryReady:
         {
            status.mNumCachesDirectoryReady++;
            break;
         }
         case BFileCache::cCacheStatusReceivedFile:
         {
            status.mNumCachesReadingFiles++;
            break;
         }
         case BFileCache::cCacheStatusLoadSucceeded:
         {
            status.mNumCachesSucceeded++;
            break;
         }
         default:
         {  
            status.mNumCachesFailed++;
            break;
         }
      }
   }

   return cFME_SUCCESS;
}

//==============================================================================
// BWorkDirList::addDir
//==============================================================================
void BFileManager::BWorkDirList::addDir(const char* pDirName, long priority)
{
   BWorkDir dir;

   dir.mName.set(pDirName);
   strPathAddBackSlash(dir.mName);
   dir.mPriority = priority;

   for (long i = 0; i < mDirs.getNumber(); i++)
   {
      if (mDirs[i].mPriority <= dir.mPriority)
      {
         mDirs.insertAtIndex(dir, i);
         return;
      }
   }

   mDirs.add(dir);
   return;
}

//==============================================================================
// BFile::createDirList
//==============================================================================
eFileManagerError BFileManager::createDirList(const char* pDir, long& dirID, long priority)
{  
   dirID = -1;
   
   BScopedMonitor lock(mMonitor);
         
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
         
   dirID = mWorkDirs.getSize();

   BWorkDirList* pDirList = new BWorkDirList;

   pDirList->addDir(pDir, priority);

   mWorkDirs.pushBack(pDirList);
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFile::setDirList
//==============================================================================
eFileManagerError BFileManager::setDirList(long dirID, const char* pDir, long priority)
{
   BScopedMonitor lock(mMonitor);
         
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
         
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;
      
   if (dirID == getBaseDirListID())
      return cFME_INVALID_DIRID;
   
   BWorkDirList* pDirList = mWorkDirs[dirID];
   
   pDirList->clear();
   pDirList->addDir(pDir, priority);
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFile::addDirList
//==============================================================================
eFileManagerError BFileManager::addDirList(long dirID, const char* pDir, long priority)
{
   BScopedMonitor lock(mMonitor);
         
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
         
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;
      
   if (dirID == getBaseDirListID())
      return cFME_INVALID_DIRID;

   BWorkDirList* pDirList = mWorkDirs[dirID];

   pDirList->clear();
   pDirList->addDir(pDir, priority);
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::getDirListSize
//==============================================================================
eFileManagerError BFileManager::getDirListSize(long dirID, uint& size) const
{
   size = 0;
   
   BScopedMonitor lock(mMonitor);
         
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
         
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;

//-- FIXING PREFIX BUG ID 381
   const BWorkDirList* pDirList = mWorkDirs[dirID];
//--

   size = pDirList->getNumberDirs();
   return cFME_SUCCESS;
}

//==============================================================================
// BFile::getDirList
//==============================================================================
BFileManager::BWorkDirList* BFileManager::getDirList(long dirID) const
{
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return NULL;

   return mWorkDirs[dirID];
}

//==============================================================================
// BFile::setDirectoryModifier
//==============================================================================
eFileManagerError BFileManager::setDirListModifier(long dirID, const char* pModifier)
{
   BScopedMonitor lock(mMonitor);
        
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
         
   if (dirID == getBaseDirListID())
      return cFME_INVALID_DIRID;
   
   BWorkDirList* pDirList = getDirList(dirID);
   if (!pDirList)
      return cFME_INVALID_DIRID;

   pDirList->setModifier(pModifier);
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::addFileCache
//==============================================================================
eFileManagerError BFileManager::addFileCache(const char* pAbsoluteFilename, BHandle& handle, bool discardOpenedFiles, bool checkForExistance)
{
   handle = NULL;
         
   BFileCache* pFileCache = new BFileCache;
   pFileCache->setBlocking(false);
      
   BScopedMonitor lock(mMonitor);
   
   if (!mInitialized)
   {
      delete pFileCache;
      return cFME_NOT_INITIALIZED;
   }
   
   if (checkForExistance)
   {
      ILowLevelFileIO* pLowLevelFileIO = ILowLevelFileIO::getDefault();
      if (mConfigFlags & cBypassXFSForArchives)
         pLowLevelFileIO = &gWin32LowLevelFileIO;
      
      WIN32_FIND_DATA findFileData;
      Utils::ClearObj(findFileData);
      
      bool exists = false;

      WIN32_FILE_ATTRIBUTE_DATA fileAttribData;
      if (pLowLevelFileIO->getFileAttributesEx(pAbsoluteFilename, GetFileExInfoStandard, &fileAttribData))
      {
         if ((fileAttribData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            exists = true;
      }      
      
      if (!exists)
         return cFME_DOES_NOT_EXIST;
   }         
   
   emptyNotifyQueue();
         
   uint slot;
   for (slot = 0; slot < mActiveCaches.getSize(); slot++)
      if (!mActiveCaches[slot].mpFileCache)
         break;

   if (slot == mActiveCaches.getSize())
      mActiveCaches.enlarge(1);
      
   BActiveFileCache& activeFileCache = mActiveCaches[slot];
   activeFileCache.mpFileCache = pFileCache;
   activeFileCache.mLastStatusData = 0;
   activeFileCache.mLastStatus = BFileCache::cCacheStatusInvalid;
   activeFileCache.mNotifyHandle = BFileCache::cInvalidNotifyCallbackHandle;
   activeFileCache.mEnabled = false;
   activeFileCache.mDiscardOpenedFiles = discardOpenedFiles;
   activeFileCache.mLockCount = 0;
   
   if (!mTotalFileCaches)
      mTotalFileCaches++;
   handle = (BHandle)(slot | (((uint)mTotalFileCaches) << 16U));
   BDEBUG_ASSERT(NULL != handle);
   mTotalFileCaches++;
   
   activeFileCache.mHandle = handle;
      
   pFileCache->addNotifyCallback(fileCacheNotifyCallback, this, (uint)handle, activeFileCache.mNotifyHandle);
   
   bool success = pFileCache->init(pAbsoluteFilename, &mAsyncArchiveLoader, false);
   if (!success)
   {
      pFileCache->removeNotifyCallback(activeFileCache.mNotifyHandle);
      
      activeFileCache.mpFileCache = NULL;
      delete pFileCache;
                  
      handle = NULL;
      
      return cFME_CACHE_INIT_FAILED;
   }
   
   emptyNotifyQueue();

   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::removeFileCache
//==============================================================================
eFileManagerError BFileManager::removeFileCache(const BHandle handle)
{
   if (!handle)
      return cFME_INVALID_HANDLE;
   
   BFileCache* pFileCache;
   
   {   
      BScopedMonitor lock(mMonitor);      
      
      if (!mInitialized)
         return cFME_NOT_INITIALIZED;
         
      emptyNotifyQueue();
      
      uint slot = ((uint)handle) & 0xFFFF;
      if (slot >= mActiveCaches.getSize())
         return cFME_INVALID_HANDLE;
         
      BActiveFileCache& activeFileCache = mActiveCaches[slot];
      if (handle != activeFileCache.mHandle)
         return cFME_INVALID_HANDLE;
      
      pFileCache = activeFileCache.mpFileCache;
      if (!pFileCache)
         return cFME_INVALID_HANDLE;
         
      if (activeFileCache.mLockCount > 0)
         return cFME_FILES_STILL_OPEN;
      
      pFileCache->removeNotifyCallback(activeFileCache.mNotifyHandle);
      
      emptyNotifyQueue();
            
      activeFileCache.mNotifyHandle = BFileCache::cInvalidNotifyCallbackHandle;
         
      activeFileCache.mpFileCache = NULL;
      activeFileCache.mEnabled = false;
      activeFileCache.mLastStatusData = 0;
      activeFileCache.mLastStatus = BFileCache::cCacheStatusInvalid;
      activeFileCache.mLockCount = 0;
      activeFileCache.mHandle = 0;
   }
   
   if (!pFileCache->deinit(false))
   {
      delete pFileCache;
      return cFME_CACHE_DEINIT_FAILED;  
   }
      
   delete pFileCache;
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::enableFileCache
//==============================================================================
eFileManagerError BFileManager::enableFileCache(const BHandle handle)
{
   if (!handle)
      return cFME_INVALID_HANDLE;

   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();
   
   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;
      
   BActiveFileCache& activeFileCache = mActiveCaches[slot];
   
   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;
      
   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;
      
   if (activeFileCache.mLastStatus == BFileCache::cCacheStatusLoadFailed)
      return cFME_CACHE_LOAD_FAILED;
      
   activeFileCache.mEnabled = true;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::disableFileCache
//==============================================================================
eFileManagerError BFileManager::disableFileCache(const BHandle handle)
{
   if (!handle)
      return cFME_INVALID_HANDLE;

   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();

   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;

   BActiveFileCache& activeFileCache = mActiveCaches[slot];
   
   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;
      
   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;
      
   activeFileCache.mEnabled = false;

   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::isFileCacheEnabled
//==============================================================================
eFileManagerError BFileManager::isFileCacheEnabled(const BHandle handle, bool& isEnabled)
{
   isEnabled = false;
   
   if (!handle)
      return cFME_INVALID_HANDLE;
            
   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();      

   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;

//-- FIXING PREFIX BUG ID 382
   const BActiveFileCache& activeFileCache = mActiveCaches[slot];
//--
   
   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;

   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;
      
   isEnabled = activeFileCache.mEnabled;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::getFileCacheStatus
//==============================================================================
eFileManagerError BFileManager::getFileCacheStatus(const BHandle handle, BFileCache::eCacheStatus& status, uint& statusData)
{
   if (!handle)
      return cFME_INVALID_HANDLE;

   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();

   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;

//-- FIXING PREFIX BUG ID 383
   const BActiveFileCache& activeFileCache = mActiveCaches[slot];
//--

   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;

   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;

   status = activeFileCache.mLastStatus;
   statusData = activeFileCache.mLastStatusData;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::getFileCacheLockCount
//==============================================================================
eFileManagerError BFileManager::getFileCacheLockCount(const BHandle handle, uint& lockCount)
{
   lockCount = 0;
   
   if (!handle)
      return cFME_INVALID_HANDLE;

   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;

   emptyNotifyQueue();      

   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;

//-- FIXING PREFIX BUG ID 384
   const BActiveFileCache& activeFileCache = mActiveCaches[slot];
//--
   
   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;

   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;
            
   lockCount = activeFileCache.mLockCount;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::dumpFileCacheInfo
//==============================================================================   
eFileManagerError BFileManager::dumpFileCacheInfo(const BHandle handle)
{
   BDEBUG_ASSERT(handle);
   
   BScopedMonitor lock(mMonitor);    
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();               
   
   uint slot = ((uint)handle) & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;

   BActiveFileCache& activeFileCache = mActiveCaches[slot];

   if (handle != activeFileCache.mHandle)
      return cFME_INVALID_HANDLE;

   if (!activeFileCache.mpFileCache)
      return cFME_INVALID_HANDLE;

   BFileCache& fileCache = *activeFileCache.mpFileCache;
      
   uint numFiles = fileCache.getNumFiles();
      
   int statusData;
   uint numFilesLocked;
   BFileCache::eCacheStatus cacheStatus = fileCache.getStatus(&statusData, &numFilesLocked);

   tracenocrlf("File Cache Status: ");   
   switch (cacheStatus)
   {
      case BFileCache::cCacheStatusInvalid:        trace("Invalid"); break;
      case BFileCache::cCacheStatusLoadCanceled:   trace("LoadCanceled"); break;
      case BFileCache::cCacheStatusLoadFailed:     trace("LoadFailed"); break;
      case BFileCache::cCacheStatusBusy:           trace("Busy"); break;
      case BFileCache::cCacheStatusHeadersReady:   trace("HeadersReady"); break;
      case BFileCache::cCacheStatusDirectoryReady: trace("DirectoryReady"); break;
      case BFileCache::cCacheStatusReceivedFile:   trace("ReceivedFile"); break;
      case BFileCache::cCacheStatusLoadSucceeded:  trace("LoadSucceeded"); break;
   };
   
   trace("StatusData: %i, NumFilesLocked: %u", statusData, numFilesLocked);
   
   trace("Total Files: %u", numFiles);
   
   uint totalFilesLoaded = 0;
   uint totalFilesNotLoaded = 0;
   uint totalFilesDiscarded = 0;
   uint64 totalBytesLoaded = 0;
   
   for (uint i = 0; i < numFiles; i++)
   {
      BFileCache::BFileHandle fileHandle = fileCache.getFileByIndex(i);
      
      BFileCache::BFileDesc fileDesc;
      
      bool success = fileCache.getFileDesc(fileHandle, fileDesc);
      if (!success)
         break;
           
      tracenocrlf("File: %u, Date: %I64u, Name: %s, CompSize: %u, DecompSize: %u, CompMethod: %u, LockCount: %u, Status: ",
         i, 
         fileDesc.mDate,
         fileDesc.mFullName.getPtr(),
         fileDesc.mCompSize,
         fileDesc.mDecompSize,
         fileDesc.mCompMethod,
         fileDesc.mLockCount);
         
      switch (fileDesc.mStatus)
      {  
         case BFileCache::cFileStatusNotLoaded: tracenocrlf("NotLoaded\n"); totalFilesNotLoaded++; break;
         case BFileCache::cFileStatusLoaded:    tracenocrlf("Loaded\n"); totalFilesLoaded++; break;
         case BFileCache::cFileStatusDiscarded: tracenocrlf("Discarded\n"); totalFilesDiscarded++; break;
      }       
      
      if (fileDesc.mStatus == BFileCache::cFileStatusLoaded)
      {
         totalBytesLoaded += fileDesc.mCompSize;
      }
   }
   
   trace("Total files loaded: %u, not loaded: %u, discarded: %u", totalFilesLoaded, totalFilesNotLoaded, totalFilesDiscarded);
   trace("Total comp. bytes loaded: %I64u", totalBytesLoaded);
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::dumpAllFileCacheInfo
//==============================================================================   
eFileManagerError BFileManager::dumpAllFileCacheInfo()
{
   BScopedMonitor lock(mMonitor);    
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;

   emptyNotifyQueue();            
   
   for (uint i = 0; i < mActiveCaches.getSize(); i++)
   {
      if (!mActiveCaches[i].mpFileCache)
         continue;
         
      eFileManagerError status = dumpFileCacheInfo(mActiveCaches[i].mHandle);
      if (status != cFME_SUCCESS)
         return status;
   }
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::waitUntilAllCachesAreLoadedOrFailedCallback
//==============================================================================   
BOOL BFileManager::waitUntilAllCachesAreLoadedOrFailedCallback(void* pCallbackDataPtr, uint64 callbackData)
{
   BFileManager& fileManager = *static_cast<BFileManager*>(pCallbackDataPtr);

   if (!fileManager.mInitialized)
      return TRUE;

   fileManager.emptyNotifyQueue();

   for (uint i = 0; i < fileManager.mActiveCaches.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 397
      const BActiveFileCache& activeFileCache = fileManager.mActiveCaches[i];
//--
      if (!activeFileCache.mpFileCache)
         continue;
         
      if ((activeFileCache.mLastStatus >= BFileCache::cCacheStatusBusy) && (activeFileCache.mLastStatus <= BFileCache::cCacheStatusReceivedFile))
         return FALSE;
   }
   
   return TRUE;
}

//==============================================================================
// BFileManager::pauseIO
//==============================================================================   
eFileManagerError BFileManager::pauseIO()
{
   if (!mInitialized)
      return cFME_FAILED;
   
   if (InterlockedIncrement(&mIOPauseCounter) == 1)
   {
      if (!mAsyncArchiveLoader.pause())
         return cFME_FAILED;
   }
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::resumeIO
//==============================================================================   
eFileManagerError BFileManager::resumeIO()
{
   if (!mInitialized)
      return cFME_FAILED;

   if (InterlockedDecrement(&mIOPauseCounter) == 0)
   {      
      if (!mAsyncArchiveLoader.resume())
         return cFME_FAILED;
   }         
      
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::isIOPaused
//==============================================================================   
bool BFileManager::isIOPaused()
{
   if (!mInitialized)
      return false;

   MemoryBarrier();
   
   return mIOPauseCounter > 0;
}

//==============================================================================
// BFileManager::waitForAllFileCachesToLoadOrFail
// FIXME: Optimize this call to be lock-free.
//==============================================================================   
eFileManagerError BFileManager::waitUntilAllCachesAreLoadedOrFailed()
{
   BScopedMonitor lock(mMonitor);    
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();  
   
   monitorWait(waitUntilAllCachesAreLoadedOrFailedCallback, this);

   for (uint i = 0; i < mActiveCaches.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 385
      const BActiveFileCache& activeFileCache = mActiveCaches[i];
//--
      if (!activeFileCache.mpFileCache)
         continue;
      
      if ((activeFileCache.mLastStatus >= BFileCache::cCacheStatusBusy) && (activeFileCache.mLastStatus <= BFileCache::cCacheStatusReceivedFile))
         return cFME_INTERNAL_ERROR;
   }

   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::waitForAllDirsOrFailureCallback
//==============================================================================   
BOOL BFileManager::waitForAllDirsOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData)
{
   BFileManager& fileManager = *static_cast<BFileManager*>(pCallbackDataPtr);
   
   if (!fileManager.mInitialized)
      return TRUE;
   
   fileManager.emptyNotifyQueue();
   
   for (uint i = 0; i < fileManager.mActiveCaches.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 398
      const BActiveFileCache& activeFileCache = fileManager.mActiveCaches[i];
//--
      
      if (!activeFileCache.mEnabled)
         continue;
         
      if ((activeFileCache.mLastStatus >= BFileCache::cCacheStatusBusy) && (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
         return FALSE;
   }
   
   return TRUE;
}

//==============================================================================
// BFileManager::waitForAllArchiveDirsOrFailure
//==============================================================================   
eFileManagerError BFileManager::waitForAllArchiveDirsOrFailure(uint fileSystemFlags)
{
   SCOPEDSAMPLE(waitForAllArchiveDirsOrFailure)

   if (fileSystemFlags & cIgnoreFileCaches)
      return cFME_SUCCESS;
      
   if (fileSystemFlags & cDoNotWait)
   {
      emptyNotifyQueue();
      
      for (uint i = 0; i < mActiveCaches.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 386
         const BActiveFileCache& activeFileCache = mActiveCaches[i];
//--
         if (activeFileCache.mEnabled)
         {
            if ((activeFileCache.mLastStatus >= BFileCache::cCacheStatusBusy) && (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
               return cFME_WOULD_HAVE_BLOCKED;
         }               
      }
   }
   
   monitorWait(waitForAllDirsOrFailureCallback, this);
   
   for (uint i = 0; i < mActiveCaches.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 387
      const BActiveFileCache& activeFileCache = mActiveCaches[i];
//--

      if (!activeFileCache.mEnabled)
         continue;

      if (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady)
         return cFME_FAILED;
   }
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::findLooseFile
//==============================================================================
eFileManagerError BFileManager::findLooseFile(const char* pFullFilename, WIN32_FILE_ATTRIBUTE_DATA& fileAttribData)
{
//#ifdef XBOX  
   BFixedStringMaxPath looseFilename;
   createLooseFilename(looseFilename, pFullFilename, false);
         
   BOOL success = ILowLevelFileIO::getDefault()->getFileAttributesEx(looseFilename, GetFileExInfoStandard, &fileAttribData);
   if (success)
   {
      if ((fileAttribData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
         return cFME_SUCCESS;
   }
//#endif // XBOX
   
   return cFME_FAILED;
}
//==============================================================================
// BFileManager::doesFileExistInternal
//==============================================================================
eFileManagerError BFileManager::doesFileExistInternal(const char* pFullFilename, uint fileSystemFlags)
{
   if (!isValidFilename(pFullFilename))
      return cFME_INVALID_FILENAME;
      
   if ((fileSystemFlags & cIgnoreFileCaches) == 0)
   {
      for (uint i = 0; i < mActiveCaches.getSize(); i++)
      {
         BActiveFileCache& activeFileCache = mActiveCaches[i];

         if ((!activeFileCache.mEnabled) || (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
            continue;

         BDEBUG_ASSERT(activeFileCache.mpFileCache);         

         BFileCache& fileCache = *activeFileCache.mpFileCache;
         
         BFileCache::BFileHandle fileHandle = fileCache.findFileByName(pFullFilename);
         if (BFileCache::cInvalidFileHandle != fileHandle)
            return cFME_SUCCESS;
      }
   }
   
   if (((mConfigFlags & cEnableLooseFiles) && ((fileSystemFlags & cIgnoreLooseFiles) == 0)) || ((fileSystemFlags & cForceLooseFile) != 0))
   {
      WIN32_FILE_ATTRIBUTE_DATA findFileData;
      if (findLooseFile(pFullFilename, findFileData) == cFME_SUCCESS)
      {
         return cFME_SUCCESS;
      }
   }
      
   return cFME_FAILED;
}

//==============================================================================
// BFileManager::doesFileExistInternal
//==============================================================================
eFileManagerError BFileManager::doesFileExistInternal(long dirID, const char* pRelFilename, BFixedStringMaxPath& fullPath, uint fileSystemFlags)
{
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;

//-- FIXING PREFIX BUG ID 388
   const BWorkDirList* pDirList = mWorkDirs[dirID];
//--

   for (uint dirIndex = 0; dirIndex < pDirList->getNumberDirs(); dirIndex++)
   {
      if (cFME_SUCCESS != getDirListEntry(fullPath, dirID, dirIndex))
         return cFME_FAILED;

      joinPathWithFilename(fullPath, pRelFilename);
      
      eFileManagerError error = doesFileExistInternal(fullPath, fileSystemFlags);
      if (error != cFME_FAILED)
         return error;
   }
   
   return cFME_DOES_NOT_EXIST;
}

//==============================================================================
// BFileManager::constructQualifiedPathInternal
//==============================================================================
eFileManagerError BFileManager::constructQualifiedPathInternal(long dirID, const char* pFilename, BFixedStringMaxPath& qualifiedPath, uint fileSearchFlags, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pFilename);
   
   qualifiedPath.empty();

   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
            
   if (fileSearchFlags != cNoSearch)
   {
      eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
      if (error != cFME_SUCCESS)
         return error;

      error = doesFileExistInternal(dirID, pFilename, qualifiedPath, fileSystemFlags);
      if (error == cFME_SUCCESS)
         return error;
      
      if (fileSearchFlags & cFailIfNotFound)   
         return cFME_DOES_NOT_EXIST;
   }      
   
   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;

//-- FIXING PREFIX BUG ID 389
   const BWorkDirList* pDirList = mWorkDirs[dirID];
//--

   if (!pDirList->getNumberDirs())
      return cFME_INVALID_DIRID;

   if (cFME_SUCCESS != getDirListEntry(qualifiedPath, dirID, 0))
      return cFME_INVALID_DIRID;

   joinPathWithFilename(qualifiedPath, pFilename);
      
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::getFileDesc
//==============================================================================
eFileManagerError BFileManager::getFileDesc(long dirID, const char* pRelFilename, BFileDesc& desc, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pRelFilename);
   desc.clear();
   
   if (!isValidFilename(pRelFilename))
      return cFME_INVALID_FILENAME;
   
   BScopedMonitor lock(mMonitor);    
   
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;

   eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;
           
//-- FIXING PREFIX BUG ID 390
   const BWorkDirList* pDirList = mWorkDirs[dirID];
//--

   if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
      return cFME_INVALID_DIRID;
      
   if (!pDirList->getNumberDirs())
      return cFME_INVALID_DIRID;
            
   for (uint dirIndex = 0; dirIndex < pDirList->getNumberDirs(); dirIndex++)
   {
      BFixedStringMaxPath fullPath;
      if (cFME_SUCCESS != getDirListEntry(fullPath, dirID, dirIndex))
         return cFME_INVALID_DIRID;

      joinPathWithFilename(fullPath, pRelFilename);
      
      if ((fileSystemFlags & cIgnoreFileCaches) == 0)
      {
         for (uint fileCacheIndex = 0; fileCacheIndex < mActiveCaches.getSize(); fileCacheIndex++)
         {
            BActiveFileCache& activeFileCache = mActiveCaches[fileCacheIndex];

            if ((!activeFileCache.mEnabled) || (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
               continue;

            BDEBUG_ASSERT(activeFileCache.mpFileCache);         

            BFileCache& fileCache = *activeFileCache.mpFileCache;

            BFileCache::BFileHandle fileHandle = fileCache.findFileByName(fullPath);
            if (BFileCache::cInvalidFileHandle != fileHandle)
            {
               BFileCache::BFileDesc fileDesc;
               if (fileCache.getFileDesc(fileHandle, fileDesc))
               {
                  desc.mCacheFileExists = true;
                  
                  if (fileDesc.mDate > desc.mTime)  
                  {
                     desc.mFullName.set(fullPath);
                     desc.mRelativeName.set(pRelFilename);
                     desc.mSize = fileDesc.mDecompSize;
                     desc.mCompSize = fileDesc.mCompSize;
                     desc.mTime = fileDesc.mDate;
                     desc.mDirID = dirID;
                     desc.mDirIDEntryIndex = dirIndex;
                     desc.mFileCache = activeFileCache.mHandle;
                     desc.mFileCacheFileHandle = fileHandle;
                     desc.mLockCount = fileDesc.mLockCount;
                     switch (fileDesc.mStatus)
                     {
                        case BFileCache::cFileStatusNotLoaded: desc.mStatus = BFileDesc::cLoadPending; break;
                        case BFileCache::cFileStatusLoaded:    desc.mStatus = BFileDesc::cImmediatelyOpenable; break;
                        case BFileCache::cFileStatusDiscarded: desc.mStatus = BFileDesc::cDiscarded; break;
                        default:
                        {
                           BDEBUG_ASSERT(0);
                        }
                     }
                     desc.mLooseFile = false;
                     desc.mReadOnly = true;
                     desc.mCompMethod = fileDesc.mCompMethod;
                     desc.mCompressed = (fileDesc.mCompMethod != cECFCompMethodStored);
                  }
               }
            }  
         }
      }         
            
      if (((mConfigFlags & cEnableLooseFiles) && ((fileSystemFlags & cIgnoreLooseFiles) == 0)) || ((fileSystemFlags & cForceLooseFile) != 0))
      {
         if ( ((fileSystemFlags & cPreferFilesInCaches) == 0) || (!desc.mTime) )
         {
            WIN32_FILE_ATTRIBUTE_DATA findFileData;
            if (cFME_SUCCESS == findLooseFile(fullPath, findFileData))
            {
               desc.mLooseFileExists = true;
               
               const uint64 timeStamp = Math::Max(Utils::FileTimeToUInt64(findFileData.ftCreationTime), Utils::FileTimeToUInt64(findFileData.ftLastWriteTime));
               if (timeStamp > desc.mTime)
               {
                  desc.mFullName.set(fullPath);
                  desc.mRelativeName.set(pRelFilename);
                  desc.mSize = ((uint64)findFileData.nFileSizeLow) | (((uint64)findFileData.nFileSizeHigh) << 32U);
                  desc.mCompSize = desc.mSize;
                  desc.mTime = timeStamp;
                  desc.mDirID = dirID;
                  desc.mDirIDEntryIndex = dirIndex;
                  desc.mFileCache = NULL;
                  desc.mFileCacheFileHandle = BFileCache::cInvalidFileHandle;
                  desc.mLockCount = 0;
                  desc.mStatus = BFileDesc::cImmediatelyOpenable;
                  desc.mLooseFile = true;
                  desc.mReadOnly = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0;
                  desc.mCompMethod = cECFCompMethodStored;
                  desc.mCompressed = false;
               }               
            }
         }            
      }

      if (desc.mFullName.length())
         return cFME_SUCCESS;
   }
   
   return cFME_FAILED;
}

//==============================================================================
// BFileManager::getFileDesc
//==============================================================================
eFileManagerError BFileManager::getFileDesc(const char* pFullFilename, BFileDesc& desc, uint fileSystemFlags)
{
   return getFileDesc(getBaseDirListID(), pFullFilename, desc, fileSystemFlags);
}

//==============================================================================
// BFileManager::findFilesInternal
//==============================================================================
eFileManagerError BFileManager::findFilesInternal(const char* pPath, const char* pMask, BPotentialFileArray& potentialFiles, long flags, uint fileSystemFlags)
{
   BFileCache::BFindFilesInfoArray findFilesArray;
   findFilesArray.reserve(256);
   
   if ((fileSystemFlags & cIgnoreFileCaches) == 0)
   {
      for (uint fileCacheIndex = 0; fileCacheIndex < mActiveCaches.getSize(); fileCacheIndex++)
      {
         BActiveFileCache& activeFileCache = mActiveCaches[fileCacheIndex];

         if ((!activeFileCache.mEnabled) || (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
            continue;

         BDEBUG_ASSERT(activeFileCache.mpFileCache);         

         BFileCache& fileCache = *activeFileCache.mpFileCache;

         findFilesArray.resize(0);
                                   
         bool success = fileCache.findFiles(pPath, pMask, (flags & BFFILE_WANT_FILES) != 0, (flags & BFFILE_WANT_DIRS) != 0, (flags & BFFILE_RECURSE_SUBDIRS) != 0, findFilesArray);
         success;
                     
         for (uint i = 0; i < findFilesArray.getSize(); i++)
         {
            const BFileCache::BFindFilesInfo& file = findFilesArray[i];

            BPotentialFile potentialFile;
            potentialFile.mFullname = file.mFullName;

            potentialFile.mRelname = file.mRelPath;
            strPathAddBackSlash(potentialFile.mRelname);
            potentialFile.mRelname += file.mName;
            
            potentialFiles.pushBack(potentialFile);
         }
      }
   }      
   
#ifdef XBOX  
   if (((mConfigFlags & cEnableLooseFiles) && ((fileSystemFlags & cIgnoreLooseFiles) == 0)) || ((fileSystemFlags & cForceLooseFile) != 0))
   {
      BFindFiles findFilesObj;
      
      uint scanFlags = 0;
      if (flags & BFFILE_WANT_FILES     ) scanFlags |= BFindFiles::FIND_FILES_WANT_FILES;
      if (flags & BFFILE_WANT_DIRS      ) scanFlags |= BFindFiles::FIND_FILES_WANT_DIRS;
      if (flags & BFFILE_RECURSE_SUBDIRS) scanFlags |= BFindFiles::FIND_FILES_RECURSE_SUBDIRS;
      
      BFixedStringMaxPath loosePath;
      createLooseFilename(loosePath, pPath, false);
      
      bool success = findFilesObj.scan(loosePath.getPtr(), pMask, scanFlags);
      success;
               
      const char* pGamePath = BFileUtils::getXboxGamePath();
      const uint gamePathLen = strlen(pGamePath);
            
      for (uint i = 0; i < findFilesObj.numFiles(); i++)
      {
         const ::BFileDesc& fileDesc = findFilesObj.getFile(i);
         
         BPotentialFile potentialFile;
         potentialFile.mFullname = fileDesc.fullFilename();
                           
         int l = potentialFile.mFullname.findLeft(pGamePath);
         if (l != -1)
            potentialFile.mFullname.crop(l + gamePathLen, potentialFile.mFullname.length() - 1);
                  
         potentialFile.mRelname = fileDesc.relPathname();
         strPathAddBackSlash(potentialFile.mRelname, false);
         potentialFile.mRelname += fileDesc.filename();
                  
         potentialFiles.pushBack(potentialFile);
      }
   }
#endif // XBOX
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::createFileList
//==============================================================================
void BFileManager::createFileList(BPotentialFileArray& potentialFiles, BDynamicArray<BString>& files, bool removeXMBExtension, bool fullPath)
{
   if (potentialFiles.getSize())
   {
      potentialFiles.sort();

      for (uint i = 0; i < potentialFiles.getSize(); i++)
      {
         const BPotentialFile& potentialFile = potentialFiles[i];

         if (i)
         {
            if (potentialFiles[i - 1].mRelname == potentialFile.mRelname)
               continue;
         }

         BString name(fullPath ? potentialFile.mFullname : potentialFile.mRelname);
         
         if (removeXMBExtension)
         {
            if (name.findRight(".xmb") != -1)
               name.removeExtension();
         }
         
         files.pushBack(name);
      }
   }      
}

//==============================================================================
// BFileManager::findFiles
//==============================================================================
eFileManagerError BFileManager::findFiles(const char* pPath, const char* pMask, BDynamicArray<BString>& files, bool fullPath, long flags, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pPath && strlen(pPath));
   BDEBUG_ASSERT(pMask && strlen(pMask));
      
   BPotentialFileArray potentialFiles;
   potentialFiles.reserve(256);
   
   bool removeXMBExtension = false;
   
   {
      BScopedMonitor lock(mMonitor);    
      
      if (!mInitialized)
         return cFME_NOT_INITIALIZED;

      eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
      if (error != cFME_SUCCESS)
         return error;
         
      error = findFilesInternal(pPath, pMask, potentialFiles, flags, fileSystemFlags);
      
      if ((flags & BFFILE_TRY_XMB) && (potentialFiles.isEmpty()))
      {
         BFixedStringMaxPath tempMask(pMask);
         tempMask += ".xmb";

         error = findFilesInternal(pPath, tempMask, potentialFiles, flags, fileSystemFlags);
         
         if (cFME_SUCCESS == error)
            removeXMBExtension = true;
      }
      
      if ((error != cFME_SUCCESS) && (potentialFiles.isEmpty()))
         return error;
   }
   
   createFileList(potentialFiles, files, removeXMBExtension, fullPath);
               
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::findFiles
//==============================================================================
eFileManagerError BFileManager::findFiles(long dirID, const char* pMask, BDynamicArray<BString>& files, bool fullPath, long flags, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pMask && strlen(pMask));
   
   BPotentialFileArray potentialFiles;
   potentialFiles.reserve(256);

   bool removeXMBExtension = false;
   
   {
      BScopedMonitor lock(mMonitor); 
      
      if (!mInitialized)
         return cFME_NOT_INITIALIZED;   

      eFileManagerError error = waitForAllArchiveDirsOrFailure(fileSystemFlags);
      if (error != cFME_SUCCESS)
         return error;
      
      if ((dirID < 0) || (dirID >= (long)mWorkDirs.getSize()))
         return cFME_INVALID_DIRID;

//-- FIXING PREFIX BUG ID 391
      const BWorkDirList* pDirList = mWorkDirs[dirID];
//--

      for (uint dirIndex = 0; dirIndex < pDirList->getNumberDirs(); dirIndex++)
      {
         BFixedStringMaxPath fullPath;
         if (cFME_SUCCESS != getDirListEntry(fullPath, dirID, dirIndex))
            return cFME_INVALID_DIRID;
      
         uint numFilesFound = potentialFiles.getSize();
         
         error = findFilesInternal(fullPath, pMask, potentialFiles, flags, fileSystemFlags);
         if (error != cFME_SUCCESS)
            return error;
         
         numFilesFound = potentialFiles.getSize() - numFilesFound;
            
         if ((flags & BFFILE_TRY_XMB) && (!numFilesFound))
         {
            BFixedStringMaxPath tempMask(pMask);
            tempMask += ".xmb";

            numFilesFound = potentialFiles.getSize();
            
            error = findFilesInternal(fullPath, tempMask, potentialFiles, flags, fileSystemFlags);
            if (error != cFME_SUCCESS)
               return error;
            
            numFilesFound = potentialFiles.getSize() - numFilesFound;

            if (numFilesFound)
               removeXMBExtension = true;
         }
      }
   }         
   
   createFileList(potentialFiles, files, removeXMBExtension, fullPath);
      
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::findFiles
//==============================================================================
eFileManagerError BFileManager::findFiles(const char* pPath, const char* pMask, long flags, FindFileFunc* pFunc, void* pParam, bool fullPath, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pPath && strlen(pPath));
   BDEBUG_ASSERT(pMask && strlen(pMask));
   BDEBUG_ASSERT(pFunc);
      
   BDynamicArray<BString> files;
   files.reserve(256);
   
   eFileManagerError error = findFiles(pPath, pMask, files, fullPath, flags, fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;
   
   for (uint i = 0; i < files.getSize(); i++)
      if (!pFunc(files[i], pParam))
         break;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::findFiles
//==============================================================================
eFileManagerError BFileManager::findFiles(long dirID, const char* pMask, long flags, FindFileFunc* pFunc, void* pParam, bool fullPath, uint fileSystemFlags)
{
   BDEBUG_ASSERT(pMask && strlen(pMask));
   BDEBUG_ASSERT(pFunc);

   BDynamicArray<BString> files;
   files.reserve(256);

   eFileManagerError error = findFiles(dirID, pMask, files, fullPath, flags, fileSystemFlags);
   if (error != cFME_SUCCESS)
      return error;

   for (uint i = 0; i < files.getSize(); i++)
      if (!pFunc(files[i], pParam))
         break;

   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::discardFilesInternal
//==============================================================================
eFileManagerError BFileManager::discardFilesInternal(long dirID, const char* pPath, const char* pMask, bool recurseSubDirs, const BHandle fileCacheHandle, uint* pNumFilesDiscarded)
{
   BDEBUG_ASSERT(pMask);
   
   //if (pNumFilesDiscarded)
   //   *pNumFilesDiscarded = 0;
   
   BScopedMonitor lock(mMonitor); 

   if (!mInitialized)
      return cFME_NOT_INITIALIZED;   

   BDynamicArray<BString> files;
   eFileManagerError status;
   if (pPath)
      status = findFiles(pPath, pMask, files, true, BFFILE_WANT_FILES | (recurseSubDirs ? BFFILE_RECURSE_SUBDIRS : 0), cIgnoreLooseFiles);
   else
      status = findFiles(dirID, pMask, files, true, BFFILE_WANT_FILES | (recurseSubDirs ? BFFILE_RECURSE_SUBDIRS : 0), cIgnoreLooseFiles);
      
   if (status < 0)
      return status;
      
   for (uint fileIndex = 0; fileIndex < files.getSize(); fileIndex++)
   {
      const BString& filename = files[fileIndex];

      for (uint fileCacheIndex = 0; fileCacheIndex < mActiveCaches.getSize(); fileCacheIndex++)
      {
         BActiveFileCache& activeFileCache = mActiveCaches[fileCacheIndex];

         if (fileCacheHandle)
         {
            if (activeFileCache.mHandle != fileCacheHandle)
               continue;
         }
         else
         {
            if ((!activeFileCache.mEnabled) || (activeFileCache.mLastStatus < BFileCache::cCacheStatusDirectoryReady))
               continue;
         }               

         BDEBUG_ASSERT(activeFileCache.mpFileCache);         

         BFileCache& fileCache = *activeFileCache.mpFileCache;

         BFileCache::BFileHandle fileHandle = fileCache.findFileByName(filename);
         if (BFileCache::cInvalidFileHandle != fileHandle)
         {
            bool success = fileCache.discardFileData(fileHandle);
            if (success)
            {
               log("BFileManager::discardFiles: Discarded file: \"%s\"", filename.getPtr());
               
               if (pNumFilesDiscarded)
               {
                  *pNumFilesDiscarded = *pNumFilesDiscarded + 1;
               }
            }
            else
            {
               log("BFileManager::discardFiles: FAILED discarding file: \"%s\"", filename.getPtr());
            }
         }  
      }
   }         
         
   return cFME_SUCCESS;         
}

//==============================================================================
// BFileManager::discardFiles
//==============================================================================
eFileManagerError BFileManager::discardFiles(const char* pPath, const char* pMask, bool recurseSubdirs, const BHandle fileCacheHandle, uint* pNumFilesDiscarded)
{
   BDEBUG_ASSERT(pPath);
   return discardFilesInternal(-1, pPath, pMask, recurseSubdirs, fileCacheHandle, pNumFilesDiscarded);
}

//==============================================================================
// BFileManager::discardFiles
//==============================================================================
eFileManagerError BFileManager::discardFiles(long dirID, const char* pMask, bool recurseSubdirs, const BHandle fileCacheHandle, uint* pNumFilesDiscarded)
{
   return discardFilesInternal(dirID, NULL, pMask, recurseSubdirs, fileCacheHandle, pNumFilesDiscarded);
}

//==============================================================================
// struct BWaitForFileOrFailureData
//==============================================================================
struct BWaitForFileOrFailureData
{
   BFileManager*           mpFileManager;
   BHandle                 mFileCache;
   BFileCache::BFileHandle mFileHandle;
};

//==============================================================================
// BFileManager::waitForFileOrFailureCallback
//==============================================================================
BOOL BFileManager::waitForFileOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData)
{
   BWaitForFileOrFailureData& data = *static_cast<BWaitForFileOrFailureData*>(pCallbackDataPtr);
   
   BFileManager& fileManager = *data.mpFileManager;
   if (!fileManager.mInitialized)
      return TRUE;
      
   fileManager.emptyNotifyQueue();
   
   uint slot = (uint)data.mFileCache & 0xFFFF;
   if (slot >= fileManager.mActiveCaches.getSize())
      return TRUE;

//-- FIXING PREFIX BUG ID 399
   const BActiveFileCache& activeFileCache = fileManager.mActiveCaches[slot];
//--
   if ((activeFileCache.mHandle != data.mFileCache) || (!activeFileCache.mpFileCache))
      return TRUE;

   if ((activeFileCache.mLastStatus < 0) || (activeFileCache.mLastStatus == BFileCache::cCacheStatusLoadSucceeded))
      return TRUE;
   
   if (activeFileCache.mLastStatus < BFileCache::cCacheStatusReceivedFile) 
      return FALSE;

   //int fileIndex = activeFileCache.mpFileCache->getFileByIndex(data.mFileHandle);
   int fileIndex = activeFileCache.mpFileCache->getFileIndex(data.mFileHandle);
   if (fileIndex < 0)
      return TRUE;

   if ((int)activeFileCache.mLastStatusData >= fileIndex)
      return TRUE;
   
   return FALSE;
}

//==============================================================================
// BFileManager::waitForFileOrFailure
//==============================================================================
eFileManagerError BFileManager::waitForFileOrFailure(const BHandle handle, BFileCache::BFileHandle fileHandle, uint fileSystemFlags)
{
   SCOPEDSAMPLE(BFileManager_waitForFileOrFailure)

   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
            
   BWaitForFileOrFailureData data;
   data.mpFileManager = this;
   data.mFileCache = handle;
   data.mFileHandle = fileHandle;
   
   if (fileSystemFlags & cDoNotWait)
   {
      if (!waitForFileOrFailureCallback(&data, 0))
         return cFME_WOULD_HAVE_BLOCKED;
   }
              
   monitorWait(waitForFileOrFailureCallback, &data);

   uint slot = (uint)handle & 0xFFFF;
   if (slot >= mActiveCaches.getSize())
      return cFME_INVALID_HANDLE;
      
//-- FIXING PREFIX BUG ID 393
   const BActiveFileCache& activeFileCache = mActiveCaches[slot];
//--
   if ((activeFileCache.mHandle != handle) || (!activeFileCache.mpFileCache))
      return cFME_INVALID_HANDLE;
   
   if (activeFileCache.mLastStatus < BFileCache::cCacheStatusReceivedFile) 
      return cFME_FAILED;
            
   if (activeFileCache.mLastStatus == BFileCache::cCacheStatusLoadSucceeded)
      return cFME_SUCCESS;
            
   //int fileIndex = activeFileCache.mpFileCache->getFileByIndex(fileHandle);
   int fileIndex = activeFileCache.mpFileCache->getFileIndex(fileHandle);
   if (fileIndex < 0)
      return cFME_FAILED;
      
   if ((int)activeFileCache.mLastStatusData < fileIndex)
      return cFME_FAILED;
   
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::openFile
//==============================================================================
eFileManagerError BFileManager::openFile(long dirID, const char* pFilename, uint fileFlags, BFile* pFile, uint fileSystemFlags, BStream** ppStream, long& fileIndex)
{
   eFileManagerError error = openFileInternal(dirID, pFilename, fileFlags, pFile, fileSystemFlags, ppStream, fileIndex);

#ifndef BUILD_FINAL      
   if (error != cFME_SUCCESS)
   {
      log("BFileManager::openFile: FAILED opening file, Error: %i, dirID: %i, Name: \"%s\", fileFlags: 0x%X, fileSystemFlags: 0x%X",
         error,
         dirID,
         pFilename,
         fileFlags,
         fileSystemFlags);
   }
   else
   {
      log("BFileManager::openFile: Opened file: dirID: %i, Name: \"%s\", fileFlags: 0x%X, fileSystemFlags: 0x%X",
         dirID,
         pFilename,
         fileFlags,
         fileSystemFlags);
   }
#endif            

   return error;
}

//==============================================================================
// BFileManager::openFileInternal
//==============================================================================
eFileManagerError BFileManager::openFileInternal(long dirID, const char* pFilename, uint fileFlags, BFile* pFile, uint fileSystemFlags, BStream** ppStream, long& fileIndex)
{
   if (fileFlags & BFILE_OPEN_IGNORE_LOOSE)
      fileSystemFlags |= BFileManager::cIgnoreLooseFiles;
   else if (fileFlags & BFILE_OPEN_IGNORE_ARCHIVES)
      fileSystemFlags |= BFileManager::cIgnoreFileCaches;
   
   if (fileFlags & BFILE_OPEN_FORCE_LOOSE)
   {
      fileSystemFlags |= BFileManager::cForceLooseFile;
      
      // Forcing loose files implies ignoring files in file caches.
      fileSystemFlags |= BFileManager::cIgnoreFileCaches;
   }
      
   if (fileFlags & BFILE_OPEN_DO_NOT_WAIT_FOR_FILE)
      fileSystemFlags |= BFileManager::cDoNotWait;
      
   *ppStream = NULL;
   fileIndex = -1;
   
   if ((!pFilename) || (!strlen(pFilename)))
      return cFME_INVALID_FILENAME;
   
   if ((!pFile) || (!ppStream))
      return cFME_INVALID_PARAMETER;
      
   if ((fileFlags & BFILE_OPEN_BYPASS_FILE_MANAGER) == 0)
   {
      if ((strPathIsAbsolute(pFilename)) || (dirID < 0))
         fileFlags |= BFILE_OPEN_BYPASS_FILE_MANAGER;
   }
   
   BScopedMonitor lock(mMonitor);              
            
   BStream* pDataStream = NULL;
   BStream* pTransStream = NULL;
   int fileCacheIndex = -1;
   BFileCache::BFileHandle fileCacheFileHandle = BFileCache::cInvalidFileHandle;
   
   if ((fileFlags & BFILE_OPEN_BYPASS_FILE_MANAGER) || (fileFlags & BFILE_OPEN_READWRITE) || (fileFlags & BFILE_OPEN_WRITEABLE))
   {
      BFixedStringMaxPath fullPath;
      
      if (fileFlags & BFILE_OPEN_BYPASS_FILE_MANAGER)
         fullPath.set(pFilename);
      else
      {
         eFileManagerError error = gFileManager.constructQualifiedPath(dirID, pFilename, fullPath, BFileManager::cNoSearch, BFileManager::cIgnoreFileCaches);
         if (error != cFME_SUCCESS)
            return error;
                  
         BFixedStringMaxPath looseFilename;
         createLooseFilename(looseFilename, fullPath, ((fileFlags & BFILE_OPEN_READWRITE) || (fileFlags & BFILE_OPEN_WRITEABLE)));
         
         fullPath = looseFilename;
      }
      
      uint streamFlags = cSFSeekable;
      if (fileFlags & BFILE_OPEN_READWRITE)
      {
         streamFlags |= cSFReadable | cSFWritable;
         if (fileFlags & BFILE_OPEN_APPEND)
            streamFlags |= cSFOpenExisting;
      }
      else if (fileFlags & BFILE_OPEN_WRITEABLE)
      {
         streamFlags |= cSFWritable;
         if (fileFlags & BFILE_OPEN_APPEND)
            streamFlags |= cSFOpenExisting;
      }
      else
         streamFlags |= cSFReadable;
      
      if (fileFlags & BFILE_OPEN_OPTIMIZE_FOR_RANDOM_ACCESS)
         streamFlags |= cSFOptimizeForRandomAccess;
      else if (fileFlags & BFILE_OPEN_OPTIMIZE_FOR_SEQUENTIAL_ACCESS)
         streamFlags |= cSFOptimizeForSequentialAccess;
         
      if (fileFlags & BFILE_OPEN_OVERWRITE_READONLY)
      {
         ILowLevelFileIO::getDefault()->setFileAttributes(fullPath, FILE_ATTRIBUTE_NORMAL);
      }
      
      if (mConfigFlags & cRecordOpenedFiles)
         mOpenedFiles.pushBack(BString(fullPath));
      
      BWin32FileStream* pWin32Stream = new BWin32FileStream;
      if (!pWin32Stream->open(fullPath.getPtr(), streamFlags))
      {
         delete pWin32Stream;
         return cFME_OPEN_FAILED;
      }
      pDataStream = pWin32Stream;
      
      if (fileFlags & BFILE_OPEN_ENABLE_BUFFERING)
         pTransStream = new BBufferStream(*pDataStream);
   }
   else
   {
      BFileDesc fileDesc;
      eFileManagerError error = getFileDesc(dirID, pFilename, fileDesc, fileSystemFlags);
      if (error != cFME_SUCCESS)
         return error;
         
      if (mConfigFlags & cRecordOpenedFiles)
         mOpenedFiles.pushBack(BString(fileDesc.mFullName));

      if (mConfigFlags & cEnableLogging)
         log("BFileManager::openFileInternal: Opening %s file: %i %s FileFlags: 0x%X FileSystemFlags: 0x%X (Found Loose: %u Cached: %u)", fileDesc.mLooseFile ? "LOOSE" : "CACHED", dirID, pFilename, fileFlags, fileSystemFlags, fileDesc.mLooseFileExists, fileDesc.mCacheFileExists);
      
      if (fileDesc.mLooseFile)      
      {
         BFixedStringMaxPath looseFilename;
         createLooseFilename(looseFilename, fileDesc.mFullName, false);
         
         uint streamFlags = cSFReadable | cSFSeekable;
         if (fileFlags & BFILE_OPEN_OPTIMIZE_FOR_RANDOM_ACCESS)
            streamFlags |= cSFOptimizeForRandomAccess;
         else if (fileFlags & BFILE_OPEN_OPTIMIZE_FOR_SEQUENTIAL_ACCESS)
            streamFlags |= cSFOptimizeForSequentialAccess;
         
         BWin32FileStream* pWin32Stream = new BWin32FileStream;
         if (!pWin32Stream->open(looseFilename, streamFlags))
         {
            delete pWin32Stream;
            return cFME_OPEN_FAILED;
         }
         
         pDataStream = pWin32Stream;
         
         if (fileFlags & BFILE_OPEN_ENABLE_BUFFERING)
            pTransStream = new BBufferStream(*pDataStream);
      }
      else
      {
         if ((fileDesc.mCompMethod != cECFCompMethodStored) && (fileDesc.mCompMethod != cECFCompMethodDeflateRaw))
            return cFME_INTERNAL_ERROR;
         
         if (fileDesc.mStatus == BFileDesc::cDiscarded)
            return cFME_FILE_IS_DISCARDED;
                        
         eFileManagerError error = waitForFileOrFailure(fileDesc.mFileCache, fileDesc.mFileCacheFileHandle, fileSystemFlags);
         if (error != cFME_SUCCESS)
            return error;

         fileCacheIndex = (uint)fileDesc.mFileCache & 0xFFFF;
         BDEBUG_ASSERT(mActiveCaches[fileCacheIndex].mHandle == fileDesc.mFileCache);
         
         BActiveFileCache& activeFileCache = mActiveCaches[fileCacheIndex]; 
         BFileCache* pFileCache = activeFileCache.mpFileCache;
         
         BConstDataBuffer dataBuffer;
         if (!pFileCache->lockFileData(fileDesc.mFileCacheFileHandle, dataBuffer))
            return cFME_LOCK_FAILED;
         
         activeFileCache.mLockCount++;   
                           
         BDEBUG_ASSERT(dataBuffer.getLen() == fileDesc.mCompSize);
            
         pDataStream = new BByteStream((void*)dataBuffer.getPtr(), dataBuffer.getLen(), cSFReadable | cSFSeekable, BString(fileDesc.mFullName));
         
         if (fileDesc.mCompMethod == cECFCompMethodDeflateRaw)
         {
            uint streamFlags = cSFReadable;
            if (fileFlags & BFILE_OPEN_BACKWARD_SEEKS)
               streamFlags |= cSFSeekable;
#ifdef XBOX
            pTransStream = new BRawInflateStream(*pDataStream, fileDesc.mCompSize, fileDesc.mSize, false, streamFlags, BString(fileDesc.mFullName));
#endif // XBOX
         }
      }
      
      fileCacheFileHandle = fileDesc.mFileCacheFileHandle;
   }      
   
   uint activeFileIndex;
   for (activeFileIndex = 0; activeFileIndex < mActiveFiles.getSize(); activeFileIndex++)
      if (!mActiveFiles[activeFileIndex].mpFile)
         break;

   if (activeFileIndex == mActiveFiles.getSize())
      mActiveFiles.enlarge(1);
   
   BActiveFile& activeFile = mActiveFiles[activeFileIndex];
   activeFile.mpFile = pFile;
   activeFile.mpDataStream = pDataStream;
   activeFile.mpTransStream = pTransStream;
   activeFile.mFileCacheIndex = fileCacheIndex;
   activeFile.mFileCacheFileHandle = fileCacheFileHandle;
   activeFile.mOpenFlags = fileFlags;
   
#ifdef XBOX
   if (pTransStream)
      *ppStream = pTransStream;
   else
      *ppStream = pDataStream;
#endif // XBOX

   fileIndex = activeFileIndex;
   return cFME_SUCCESS;
}

//==============================================================================
// BFileManager::removeFile
//==============================================================================
eFileManagerError BFileManager::removeFile(int fileIndex)
{
   BScopedMonitor lock(mMonitor);
         
   if (!mInitialized)
      return cFME_NOT_INITIALIZED;
      
   emptyNotifyQueue();
      
   if ((fileIndex < 0) || (fileIndex >= (int)mActiveFiles.getSize()))
      return cFME_INVALID_HANDLE;
            
   BActiveFile& activeFile = mActiveFiles[fileIndex];
   
   if (activeFile.mFileCacheIndex >= 0)
   {
      if (activeFile.mFileCacheIndex >= (int)mActiveCaches.getSize())
         return cFME_INVALID_HANDLE;
         
//-- FIXING PREFIX BUG ID 395
      const BActiveFileCache& activeFileCache = mActiveCaches[activeFile.mFileCacheIndex];
//--
      if ((!activeFileCache.mpFileCache) || (!activeFileCache.mLockCount))
         return cFME_INVALID_HANDLE;
   }         
   
   eFileManagerError error = cFME_SUCCESS;
         
   if (activeFile.mpTransStream)
   {
      if (!activeFile.mpTransStream->close())
         error = cFME_CLOSE_FAILED;
      
      delete activeFile.mpTransStream;
      activeFile.mpTransStream = NULL;
   }
   
   BString filename;
   
   if (activeFile.mpDataStream)
   {
      filename = activeFile.mpDataStream->getName();
      
      if (!activeFile.mpDataStream->close())
         error = cFME_CLOSE_FAILED;
         
      delete activeFile.mpDataStream;
      activeFile.mpDataStream = NULL;
   }
   
   activeFile.mpFile = NULL;
   
   if (activeFile.mFileCacheIndex >= 0)
   {
      BActiveFileCache& activeFileCache = mActiveCaches[activeFile.mFileCacheIndex];
      
      if (!activeFileCache.mpFileCache->unlockFileData(activeFile.mFileCacheFileHandle))
         error = cFME_UNLOCK_FAILED;
      
      if ((activeFileCache.mDiscardOpenedFiles) || (activeFile.mOpenFlags & BFILE_OPEN_DISCARD_ON_CLOSE))
      {
         if (!activeFileCache.mpFileCache->discardFileData(activeFile.mFileCacheFileHandle))
         {
            error = cFME_DISCARD_FAILED;
            
            log("BFileManager::removeFile: Failed discarding file: \"%s\"", filename.getPtr());
         }
         else
         {
            log("BFileManager::removeFile: Discarded file: \"%s\"", filename.getPtr());
         }
      }
      
      activeFile.mFileCacheIndex = -1;
      activeFile.mFileCacheFileHandle = BFileCache::cInvalidFileHandle;
      
      activeFileCache.mLockCount--;
   }   
   
   activeFile.mOpenFlags = 0;

   return error;
}

//==============================================================================
// BFileManager::monitorWait
//==============================================================================
void BFileManager::monitorWait(BMonitor::BTestConditionCallback* pCallback, void* pCallbackDataPtr, uint64 callbackData)
{
//   SCOPEDSAMPLE(BFileManager_monitorWait)
   for ( ; ; )
   {
      //int index = mMonitor.waitForCondition(pCallback, pCallbackDataPtr, callbackData, mNotifyQueueEvent.getHandle(), INFINITE, true);
      // [10/7/2008 DPM] changing from an INFINITE timeout to 5000 milliseconds.  Hit a condition where the BMonitor::leaveAndScanConditions
      //                 was not satisfied by anyone and then all threads hit the wait never to return.
      int index = mMonitor.waitForCondition(pCallback, pCallbackDataPtr, callbackData, mNotifyQueueEvent.getHandle(), 5000, false);
      
      emptyNotifyQueue();
                  
      if (!index)
         break;
   }
}



//==============================================================================
// BFileManager::emptyNotifyQueue
//==============================================================================
void BFileManager::emptyNotifyQueue(void)
{
   BFileCacheNotifyEvent eventObj;
   
   mNotifyQueueLock.lock();
      
   while (mNotifyQueue.popBack(eventObj))
   {
      const BHandle handle = (BHandle)eventObj.mCallbackData;
      const uint slotIndex = ((uint)handle) & 0xFFFF;

      if (slotIndex >= mActiveCaches.getSize())
      {
         trace("BFileManager::emptyNotifyQueue: Invalid handle");
         continue;
      }

      BActiveFileCache& activeFileCache = mActiveCaches[slotIndex];
      if ((!activeFileCache.mpFileCache) || (activeFileCache.mHandle != handle))
      {
         trace("BFileManager::emptyNotifyQueue: Invalid handle");
         continue;
      }

      activeFileCache.mLastStatus = eventObj.mNewStatus;
      activeFileCache.mLastStatusData = eventObj.mStatusData;
      
      if (activeFileCache.mLastStatus == BFileCache::cCacheStatusLoadFailed)
      {
         if (mConfigFlags & cDisableFailedArchives)
         {
            activeFileCache.mEnabled = false;
         }
         else if (mConfigFlags & cPanicOnFailedArchives)
         {
            //We only want to call the disk failure if we're on the 360
            //otherwise... panic.
#ifdef XBOX
            DiskReadFailAlert();
#else
            BFATAL_FAIL("BFileManager::emptyNotifyQueue: Archive failed to load");
#endif
         }
      }
   }      
   
   mNotifyQueueLock.unlock();
}

//==============================================================================
// BFileManager::log
//==============================================================================
void BFileManager::log(const char* pMsg, ...)
{
   pMsg;

#ifndef BUILD_FINAL   
   if (0 == (mConfigFlags & cEnableLogging))
      return;
      
   va_list args;
   va_start(args, pMsg);

   BRenderString msg;
   msg.formatArgs(pMsg, args);
   
   {
      BScopedLightWeightMutex lock(mLogMessagesMutex);
      BRenderString& str = mLogMessages.grow();
      str.swap(msg);
   }

   va_end(args);
#endif
}

//==============================================================================
// BFileManager::fileCacheNotifyCallback
//==============================================================================
void BFileManager::fileCacheNotifyCallback(BFileCache::eCacheStatus newStatus, uint statusData, void* pCallbackDataPtr, uint64 callbackData)
{
   BFileManager& fileManager = *static_cast<BFileManager*>(pCallbackDataPtr);
   
   fileManager.mNotifyQueueLock.lock();
   
   if (!fileManager.mNotifyQueue.pushFront(BFileCacheNotifyEvent(newStatus, statusData, callbackData)))
   {
      fileManager.mNotifyQueue.resize(fileManager.mNotifyQueue.getSize() * 2);
      const bool success = fileManager.mNotifyQueue.pushFront(BFileCacheNotifyEvent(newStatus, statusData, callbackData));
      BVERIFY(success);
   }
               
   fileManager.mNotifyQueueLock.unlock();
   
   fileManager.mNotifyQueueEvent.set();
}
