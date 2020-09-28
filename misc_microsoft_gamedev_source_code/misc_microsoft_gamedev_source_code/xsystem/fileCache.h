//============================================================================
//
// File: fileCache.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "containers\filenameTree.h"
#include "resource\asyncECFArchiveLoader.h"
#include "threading\monitor.h"
#include "resource\ecfArchiveTypes.h"

typedef uint64 BFileCacheHandle;
const uint64 cInvalidFileCacheHandle = UINT64_MAX;

class BFileCache : public BAsyncECFArchiveLoader::BCallbackInterface
{
public:
   typedef uint BFileHandle;
   enum { cInvalidFileHandle = 0 };
      
                           BFileCache();
                           ~BFileCache();
   
   bool                    init(const char* pArchiveFilename, BAsyncECFArchiveLoader* pLoader, bool dispatchEventsWhileWaiting = false);
   bool                    deinit(bool ignoreLockedFiles);
   
   bool                    getBlocking(void) const { return mBlockingEnabled; }
   void                    setBlocking(bool blockingEnabled) { mBlockingEnabled = blockingEnabled; }
   
   uint                    getNumLockedFiles(void);
   
   // Do not lock a file cache for very long! This could cause the I/O helper thread to slow down.
   void                    lock(void) { mMonitor.enter(); }
   void                    unlock(void) { mMonitor.leave(); }
      
   enum eCacheStatus
   {
      // Negative status values must indicate errors
      cCacheStatusInvalid       = -100,
      cCacheStatusLoadCanceled,
      cCacheStatusLoadFailed,
      
      // Positive status values must indicate success, or busy, and must increase as the archive is loaded!
      cCacheStatusBusy           = 0,
      cCacheStatusHeadersReady,
      cCacheStatusDirectoryReady,
      cCacheStatusReceivedFile,
      
      cCacheStatusLoadSucceeded,
   };
                  
   eCacheStatus            getStatus(int* pStatusData = NULL, uint* pNumFilesLocked = NULL);
   void                    getArchiveFilename(BString& filename);
         
   bool                    wait(eCacheStatus desiredCacheStatus);
   bool                    wait(BFileHandle fileHandle);
   
   bool                    isFileLoaded(BFileHandle fileHandle);
   
   int                     getFileIndex(BFileHandle fileHandle) const { return (int)fileHandle - 1; }
      
   // Waits until the cache status changes, or the cache status is Invalid, failed, or succeeded.
   void                    waitUntilStatusChangesOrFinished(eCacheStatus& newStatus, int& newStatusData);
   
   // These get and find methods will block until the archive's headers/filename chunk is loaded! (Unless blocking is disabled.)
   int                     getNumFiles(void);
   BFileHandle             getFileByIndex(uint fileIndex);
   BFileHandle             findFileByName(const char* pFilename);
   
   struct BFindFilesInfo
   {
      uint64      mDate;
      BString     mFullName;            
      BString     mRelPath;
      BString     mName;
      BFileHandle mFileHandle;
      bool        mIsDir : 1;
   };
   typedef BDynamicArray<BFindFilesInfo, ALIGN_OF(BFileHandle), BDynamicArrayRenderHeapAllocator> BFindFilesInfoArray;
   bool                    findFiles(const char* pPath, const char* pSpec, bool wantFiles, bool wantDirs, bool recurse, BFindFilesInfoArray& results);
   
   enum eFileStatus
   {
      cFileStatusNotLoaded,
      cFileStatusLoaded,
      cFileStatusDiscarded
   };
   
   struct BFileDesc
   {
      uint64                  mDecompTiger64;
      uint64                  mDate;
      BFileHandle             mHandle;
      uint                    mCompSize;
      uint                    mDecompSize;
      
      eECFArchiveCompMethod   mCompMethod;
      eFileStatus             mStatus;
      uchar                   mLockCount;
      
      BFixedStringMaxPath     mFullName;
   };
   
   bool                    getFileDesc(BFileHandle fileHandle, BFileDesc& desc);
            
   bool                    lockFileData(BFileHandle fileHandle, BConstDataBuffer& dataBuffer);
   bool                    unlockFileData(BFileHandle fileHandle);
   bool                    getFileStatus(BFileHandle fileHandle, BFileCache::eFileStatus& status, uint& lockCount);
   bool                    discardFileData(BFileHandle fileHandle);
   bool                    claimFileData(BFileHandle fileHandle, BMemoryHeapAllocation& allocation);
      
   typedef void (*BNotifyCallbackPtr)(eCacheStatus newStatus, uint statusData, void* pCallbackDataPtr, uint64 callbackData);
   typedef uint BNotifyCallbackHandle;
   enum { cInvalidNotifyCallbackHandle = 0 };
   void                    addNotifyCallback(BNotifyCallbackPtr pCallback, void* pCallbackDataPtr, uint64 callbackData, BNotifyCallbackHandle& handle);
   bool                    removeNotifyCallback(BNotifyCallbackHandle handle);
   
   bool                    getDispatchEventsWhileWaiting(void);
   void                    setDispatchEventsWhileWaiting(bool dispatchEventsWhileWaiting);
               
private:
   virtual void            message(uint64 callbackData, BAsyncECFArchiveLoader::eCallbackMessageType messageType);
   virtual bool            headerData(uint64 callbackData, const void* p, uint size);
   virtual bool            chunkAlloc(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation, uint chunkSize);
   virtual bool            chunkReady(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation);
   
   // Do not call sendNotifications() while the monitor is locked!
   void                    sendNotifications(eCacheStatus newStatus, int statusData = 0);
   void                    changeStatus(eCacheStatus newStatus, int statusData = 0);
   bool                    processFilenameChunk(BMemoryHeapAllocation& allocation);
   bool                    processDataChunk(uint chunkIndex, BMemoryHeapAllocation& allocation);
   
   static BOOL             waitForStatusOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData);
   bool                    waitForStatusOrFailure(eCacheStatus desiredCacheStatus, bool blocking);
   
   static BOOL             waitForFileOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData);
   bool                    waitForFileOrFailure(uint fileIndex, bool blocking);
   
   static BOOL             waitUntilAllFilesUnlocked(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL             waitUntilArchiveLoaderIsRemoved(void* pCallbackDataPtr, uint64 callbackData);
   
   static BOOL             waitUntilStatusChangesOrFinishedCallback(void* pCallbackDataPtr, uint64 callbackData);
            
   struct BFileInfo
   {
      BFileInfo() { clear(); }
      
      uint64                     mDecompTiger64;
      uint64                     mDate;
      BFilenameTree::BNodeIndex  mTreeNodeIndex;
      
      uint                       mCompSize;
      uint                       mDecompSize;
      
      uint                       mNameOfs;
      
      BMemoryHeapAllocation      mAllocation;
      
      uchar                      mLoadCount;
      
      uchar                      mLockCount;
      
      eECFArchiveCompMethod      mCompMethod;
                  
      eFileStatus                mStatus;
      
      void clear(void)
      {
         mDecompTiger64 = 0;
         mDate = 0;
         mTreeNodeIndex = BFilenameTree::cInvalidNodeIndex;
         mCompSize = 0;
         mDecompSize = 0;
         mNameOfs = 0;
         mAllocation.clear();
         mLoadCount = 0;
         mLockCount = 0;
         mCompMethod = cECFCompMethodStored;
         mStatus = cFileStatusNotLoaded;
      }
   };
   
   typedef BDynamicArray<BFileInfo, ALIGN_OF(BFileInfo), BDynamicArrayRenderHeapAllocator> BFileInfoArray;
   
   BMonitor                mMonitor;
   
   BString                 mArchiveFilename;
   BAsyncECFArchiveLoader* mpArchiveLoader;
   
   BFilenameTree           mFilenameTree;
   BFileInfoArray          mFileInfoArray;
                        
   BAsyncECFArchiveLoader::BLoadHandle mLoadHandle;
      
   eCacheStatus            mCacheStatus;
   int                     mCacheStatusData;
   uint                    mCacheStatusChangeCounter;
   
   uint                    mNumFilesLocked;
   
   BAsyncECFArchiveLoader::eCallbackMessageType mLastCallbackMessage;
   
   // Notify callback array 
   struct BRegisteredCallback
   {
      BRegisteredCallback() { }
      
      BRegisteredCallback(BNotifyCallbackPtr pCallback, void* pCallbackDataPtr, uint64 callbackData) :
         mpCallback(pCallback),
         mpCallbackDataPtr(pCallbackDataPtr),
         mCallbackData(callbackData)
      {
      }
      
      uint64               mCallbackData;
      void*                mpCallbackDataPtr;
      BNotifyCallbackPtr   mpCallback;
   };
   
   typedef BDynamicArray<BRegisteredCallback, ALIGN_OF(BRegisteredCallback), BDynamicArrayRenderHeapAllocator> BRegisteredCallbackArray;
   BRegisteredCallbackArray mCallbacks;
   BLightWeightMutex        mCallbackMutex;
   
   bool                     mDispatchEventsWhileWaiting : 1;
   bool                     mBlockingEnabled : 1;
};



















