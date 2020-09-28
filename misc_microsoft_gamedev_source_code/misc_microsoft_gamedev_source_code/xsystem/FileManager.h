//============================================================================
//
//  FileManager.h
//
//  Copyright 2002-2007 Ensemble Studios
//
//============================================================================
#pragma once
#include "fileSystem.h"
#include "fileCache.h"
#include "threading\monitor.h"
#include "containers\queue.h"

//==============================================================================
// eFileManagerError
//==============================================================================
enum eFileManagerError
{
   cFME_SUCCESS            = 0,
   
   // File manager errors are always negative.
   cFME_INVALID_FILENAME   = -16384,
   cFME_INVALID_DIRID,
   cFME_INVALID_HANDLE,
   cFME_NOT_INITIALIZED,
   cFME_WOULD_HAVE_BLOCKED,
   cFME_FILES_STILL_OPEN,
   cFME_CACHE_INIT_FAILED,
   cFME_CACHE_DEINIT_FAILED,
   cFME_CACHE_LOAD_FAILED,
   cFME_FIND_FILES_FAILED,
   cFME_INVALID_PARAMETER,
   cFME_DOES_NOT_EXIST,
   cFME_OPEN_FAILED,
   cFME_WRITE_FAILED,
   cFME_CLOSE_FAILED,
   cFME_LOCK_FAILED,
   cFME_UNLOCK_FAILED,
   cFME_DISCARD_FAILED,
   cFME_FILE_IS_DISCARDED,
   cFME_INTERNAL_ERROR,
   cFME_NOT_RECORDING_OPEN_FILES,
   cFME_FAILED,
};

//==============================================================================
// BFileFindFlags
//==============================================================================
enum BFileFindFlags
{
   BFFILE_WANT_DIRS            = 0x00000001,
   BFFILE_WANT_FILES           = 0x00000002,
   BFFILE_RECURSE_SUBDIRS      = 0x00000004,
   BFFILE_TRY_XMB              = 0x00000008
};

//==============================================================================
// BFileManager
//==============================================================================
class BFileManager
{
public:
   BFileManager();
   ~BFileManager();
         
   enum eFileManagerConfigFlags
   {
      cEnableLooseFiles                   = 1,
      cBypassXFSForArchives               = 2,
      cDisableFailedArchives              = 4,
      cPanicOnFailedArchives              = 8,
      cRecordOpenedFiles                  = 16,
      cWillBeUsingArchives                = 32,
      cEnableLogging                      = 64,
      
      cFMCFForceDWORD                     = 0xFFFFFFFF
   };
   
   void                    init(uint configFlags, uint numPublicKeys, const BSHA1* pPublicKeys, const uint64 decryptKey[3]);
   eFileManagerError       deinit(void);
   
   eFileManagerError       saveFileOpenLog(long dirID, const char* pFilename);
   void                    clearFileOpenLog(void);
   void                    enableFileOpenLogging(bool enabled);
   void                    flushLogMessages(void);
      
   // flushOpenFiles() is not truely thread safe!! Do not use this unless you really need it!
   void                    flushOpenFiles(void);
         
   bool                    getInitialized(void) const { return mInitialized; }
   uint                    getConfigFlags(void) const { return mConfigFlags; }
   void                    setConfigFlags(uint flags) { mConfigFlags = flags; }
         
   struct BStatus
   {
      void clear(void)
      {
         Utils::ClearObj(*this);
      }
      
      uint mNumWorkDirs;
      
      uint mNumActiveFiles;
      uint mNumActiveLooseFiles;
                  
      uint mNumActiveCaches;
      
      uint mNumCachesEnabled;
      uint mNumCachesLocked;
      
      uint mNumCachesFailed;
      uint mNumCachesBusy;
      uint mNumCachesHeadersReady;
      uint mNumCachesDirectoryReady;
      uint mNumCachesReadingFiles;
      uint mNumCachesSucceeded;
   };
   eFileManagerError       getStatus(BStatus& status);
                     
   // -- Directory list manipulation
   enum { cBaseDirListID = 0, cProductionDirListID = 1 };
   
   long                    getBaseDirListID(void) const { return cBaseDirListID; }
   long                    getProductionDirListID(void) const { return cProductionDirListID; }
   
   eFileManagerError       createDirList(const char* pDir, long& dirID, long priority = 0);
   eFileManagerError       setDirList(long dirID, const char* pDir, long priority = 0);
   eFileManagerError       addDirList(long dirID, const char* pDir, long priority);
   eFileManagerError       getDirListSize(long dirID, uint& size) const;
   
   template<class StringType>
   inline eFileManagerError getDirListEntry(StringType& dir, long dirID, uint listIndex = 0) const;
      
   eFileManagerError       setDirListModifier(long dirID, const char* pModifier);
   
   template<class StringType>
   inline eFileManagerError getDirListModifier(StringType& dir, long dirID) const;
                    
   //-- File caches
   eFileManagerError       addFileCache(const char* pAbsoluteFilename, BHandle& handle, bool discardOpenedFiles = false, bool checkForExistance = false);
   eFileManagerError       removeFileCache(const BHandle handle);
   eFileManagerError       enableFileCache(const BHandle handle);
   eFileManagerError       disableFileCache(const BHandle handle);
   eFileManagerError       isFileCacheEnabled(const BHandle handle, bool& isEnabled);
   eFileManagerError       getFileCacheStatus(const BHandle handle, BFileCache::eCacheStatus& status, uint& statusData);
   eFileManagerError       getFileCacheLockCount(const BHandle handle, uint& lockCount);
   eFileManagerError       dumpFileCacheInfo(const BHandle handle);
   eFileManagerError       dumpAllFileCacheInfo();
   eFileManagerError       waitUntilAllCachesAreLoadedOrFailed();
   eFileManagerError       pauseIO();
   eFileManagerError       resumeIO();
   bool                    isIOPaused();
                                    	
   //-- File Operations
   enum eFileSearchFlags
   { 
      cNoSearch            = 0,
      cFindBestFile        = 1,
      cFailIfNotFound      = 2
   };
   
   enum eFileSystemFlags
   {
      cUseAll              = 0,
      cIgnoreFileCaches    = 1,
      cIgnoreLooseFiles    = 2,
      cPreferFilesInCaches = 4,
            
      cDoNotWait           = 8,
      
      cForceLooseFile      = 16,
   };
   
   template<class StringType>
   inline eFileManagerError constructQualifiedPath(long dirID, const char* pFilename, StringType& qualifiedPath, uint fileSearchFlags = cFindBestFile, uint fileSystemFlags = cUseAll);
         
   struct BFileDesc
   {
      void clear(void)
      {
         mSize = 0;
         mCompSize = 0;
         mTime = 0;
         mDirID = 0;
         mDirIDEntryIndex = 0;
         mFullName.empty();
         mRelativeName.empty();
         mFileCache = NULL;
         mFileCacheFileHandle = BFileCache::cInvalidFileHandle;
         mLockCount = 0;
         mStatus = cImmediatelyOpenable;
         mLooseFile = false;
         mReadOnly = false;
         mCompressed = false;
         mCompMethod = cECFCompMethodStored;
         mLooseFileExists = false;
         mCacheFileExists = false;
      }
      
      uint64                  mSize;
      uint64                  mCompSize;
      uint64                  mTime;
      
      long                    mDirID;
      uint                    mDirIDEntryIndex;
                  
      BHandle                 mFileCache;
      BFileCache::BFileHandle mFileCacheFileHandle;
      
      BFixedStringMaxPath     mFullName;
      BFixedStringMaxPath     mRelativeName;
      
      uchar                   mLockCount;
      
      eECFArchiveCompMethod   mCompMethod;
      
      enum eFileStatus
      {
         cLoadPending         = -1,
         cImmediatelyOpenable = 0,
         cDiscarded           = 1,
      };
      eFileStatus             mStatus;
                        
      bool                    mLooseFile  : 1;
      bool                    mReadOnly   : 1;
      bool                    mCompressed : 1;
      
      bool                    mLooseFileExists : 1;
      bool                    mCacheFileExists : 1;
   };
   
   eFileManagerError       getFileDesc(const char* pFullFilename, BFileDesc& desc, uint fileSystemFlags = cUseAll);
   eFileManagerError       getFileDesc(long dirID, const char* pRelFilename, BFileDesc& desc, uint fileSystemFlags = cUseAll);
   
   eFileManagerError       doesFileExist(const char* pFullFilename, uint fileSystemFlags = cUseAll);
   
   template<class StringType>
   eFileManagerError       doesFileExist(long dirID, const char* pRelFilename, StringType* pFullPath = NULL, uint fileSystemFlags = cUseAll);
   
   eFileManagerError       doesFileExist(long dirID, const char* pRelFilename) { return doesFileExist(dirID, pRelFilename, (BString*)NULL, cUseAll); }
      
   eFileManagerError       findFiles(const char* pPath, const char* pMask, BDynamicArray<BString>& files, bool fullPath = false, long flags = BFFILE_WANT_FILES, uint fileSystemFlags = cUseAll);
   eFileManagerError       findFiles(long dirID, const char* pMask, BDynamicArray<BString>& files, bool fullPath = false, long flags = BFFILE_WANT_FILES, uint fileSystemFlags = cUseAll);
   
   // The callback version of findFiles() is emulated using the above two functions, so they are just slower.
   eFileManagerError       findFiles(const char* pPath, const char* pMask, long flags, FindFileFunc* pFunc, void* pParam, bool fullPath = true, uint fileSystemFlags = cUseAll);
   eFileManagerError       findFiles(long dirID, const char* pMask, long flags, FindFileFunc* pFunc, void* pParam, bool fullPath = true, uint fileSystemFlags = cUseAll);
         
   eFileManagerError       discardFiles(const char* pPath, const char* pMask, bool recurseSubdirs = false, const BHandle fileCacheHandle = NULL, uint* pNumFilesDiscarded = NULL); 
   eFileManagerError       discardFiles(long dirID, const char* pMask, bool recurseSubdirs = false, const BHandle fileCacheHandle = NULL, uint* pNumFilesDiscarded = NULL);
            

private:
   friend BFile;
      
   //==============================================================================
   // BWorkDirList
   //==============================================================================    
   class BWorkDirList
   {
   public:
      BWorkDirList() { }
      
      void                    clear(void) { mDirs.resize(0); }

      void                    setModifier(const char* pModifier) { mModifier.set(pModifier); }
      const BString&          getModifier(void) const { return mModifier; }
      
      void                    addDir(const char* pDirName, long priority);
      
      template<class StringType>
      bool                    getHighestPriorityDir(StringType& dir) const { return getDirNameByIndex(dir, 0); }

      uint                    getNumberDirs(void) const { return mDirs.getNumber(); }
      
      template<class StringType>
      inline bool             getDirNameByIndex(StringType& dir, uint dirIndex);
            
   protected:
      struct BWorkDir
      {
         BWorkDir() : mPriority(-1) { }

         BString mName;
         long    mPriority;
      };

      // list of directories, kept sorted by descending priority (higher number means lower priority)
      typedef BDynamicArray<BWorkDir, ALIGN_OF(BWorkDir), BDynamicArrayRenderHeapAllocator> BWorkDirArray;
      BWorkDirArray           mDirs;

      // modifier used for run-time modification of a sub-path for this directory space.
      // For example, if the directory ID would resolve to "c:\data", and the modifier
      // is "user1", file paths using this ID will resolve to "c:\data\user1".
      BString                 mModifier;
   };
   typedef BDynamicArray<BWorkDirList*, ALIGN_OF(BWorkDirList*), BDynamicArrayRenderHeapAllocator> BWorkDirListPtrArray;
   
   //==============================================================================
   // BActiveFile
   //==============================================================================   
   struct BActiveFile
   {
      BActiveFile() : 
         mpFile(NULL), 
         mpDataStream(NULL), 
         mpTransStream(NULL),
         mFileCacheIndex(0),
         mFileCacheFileHandle(BFileCache::cInvalidFileHandle),
         mOpenFlags(0)
      { 
      }
                  
      BFile*                  mpFile;
      BStream*                mpDataStream;
      BStream*                mpTransStream;
      
      int                     mFileCacheIndex;
      BFileCache::BFileHandle mFileCacheFileHandle;
      
      uint                    mOpenFlags;
   };
   
   typedef BDynamicArray<BActiveFile, ALIGN_OF(BActiveFile), BDynamicArrayRenderHeapAllocator> BActiveFileArray;
   
   //==============================================================================
   // BActiveFileCache
   //==============================================================================
   struct BActiveFileCache
   {
      BActiveFileCache() : 
         mpFileCache(NULL), 
         mLastStatusData(0),
         mLastStatus(BFileCache::cCacheStatusInvalid),
         mLockCount(0),
         mEnabled(false),
         mDiscardOpenedFiles(false),
         mNotifyHandle(BFileCache::cInvalidNotifyCallbackHandle)
      { 
      }

      BFileCache*                         mpFileCache;
      uint                                mLastStatusData;
      
      uint                                mLockCount;
      
      BHandle                             mHandle;
                  
      BFileCache::BNotifyCallbackHandle   mNotifyHandle;
      BFileCache::eCacheStatus            mLastStatus;
      
      bool                                mEnabled : 1;
      bool                                mDiscardOpenedFiles : 1;
   };
   typedef BDynamicArray<BActiveFileCache, ALIGN_OF(BActiveFileCache), BDynamicArrayRenderHeapAllocator> BActiveFileCacheArray;
   
   //==============================================================================
   // BPotentialFile
   //==============================================================================
   struct BPotentialFile
   {
      BString     mFullname;            
      BString     mRelname;
            
      bool operator== (const BPotentialFile& other) const
      {
         return mRelname == other.mRelname;
      }
      
      bool operator< (const BPotentialFile& other) const
      {
         return mRelname.compare(other.mRelname) < 0;
      }
   };
   typedef BDynamicArray<BPotentialFile, ALIGN_OF(BPotentialFile), BDynamicArrayRenderHeapAllocator> BPotentialFileArray;
   
   mutable BMonitor        mMonitor;
   
   BWorkDirListPtrArray    mWorkDirs;
   
   BActiveFileArray        mActiveFiles;
   
   BActiveFileCacheArray   mActiveCaches;
   
   BAsyncECFArchiveLoader  mAsyncArchiveLoader;
   
   BDynamicArray<BRenderString, ALIGN_OF(BRenderString), BDynamicArrayRenderHeapAllocator> mOpenedFiles;

#ifndef BUILD_FINAL   
   BLightWeightMutex       mLogMessagesMutex;
   BDynamicArray<BRenderString, ALIGN_OF(BRenderString), BDynamicArrayRenderHeapAllocator> mLogMessages;
#endif   
               
   uint                    mConfigFlags;
   
   LONG                    mIOPauseCounter;
   
   ushort                  mTotalFileCaches;
   
   bool                    mInitialized;
         
   struct BFileCacheNotifyEvent
   {
      BFileCacheNotifyEvent() { }
      
      BFileCacheNotifyEvent(BFileCache::eCacheStatus newStatus, uint statusData, uint64 callbackData) :
         mNewStatus(newStatus),
         mStatusData(statusData),
         mCallbackData(callbackData)
      {
      }
      
      uint64                     mCallbackData;
      BFileCache::eCacheStatus   mNewStatus;
      uint                       mStatusData;
   };
   
   BQueue<BFileCacheNotifyEvent> mNotifyQueue;
   BWin32Event                   mNotifyQueueEvent;
   BCriticalSection              mNotifyQueueLock;
       
   eFileManagerError openFile(long dirID, const char* pFilename, uint fileFlags, BFile* pFile, uint fileSystemFlags, BStream** ppStream, long& fileIndex);
   eFileManagerError openFileInternal(long dirID, const char* pFilename, uint fileFlags, BFile* pFile, uint fileSystemFlags, BStream** ppStream, long& fileIndex);
   eFileManagerError removeFile(int fileIndex);
      
   eFileManagerError constructQualifiedPathInternal(long dirID, const char* pFilename, BFixedStringMaxPath& qualifiedPath, uint fileSearchFlags, uint fileSystemFlags);
   
   BWorkDirList*     getDirList(long dirID) const;

   eFileManagerError doesFileExistInternal(const char* pFullFilename, uint fileSystemFlags);
   eFileManagerError doesFileExistInternal(long dirID, const char* pRelFilename, BFixedStringMaxPath& fullPath, uint fileSystemFlags);
   eFileManagerError waitForAllArchiveDirsOrFailure(uint fileSystemFlags);

   eFileManagerError findLooseFile(const char* pFullFilename, WIN32_FILE_ATTRIBUTE_DATA& findFileData);

   eFileManagerError  findFilesInternal(const char* pPath, const char* pMask, BPotentialFileArray& potentialFiles, long flags, uint fileSystemFlags);
      
   eFileManagerError waitForFileOrFailure(const BHandle handle, BFileCache::BFileHandle fileHandle, uint fileSystemFlags);
   
   void              monitorWait(BMonitor::BTestConditionCallback* pCallback, void* pCallbackDataPtr = NULL, uint64 callbackData = 0);
   
   void              createFileList(BPotentialFileArray& potentialFiles, BDynamicArray<BString>& files, bool removeXMBExtension, bool fullPath);
   
   void              emptyNotifyQueue(void);
   
   eFileManagerError discardFilesInternal(long dirID, const char* pPath, const char* pMask, bool recurseSubdirs, const BHandle fileCacheHandle, uint* pNumFilesDiscarded);
   
   void              log(const char* pMsg, ...);
      
   static void       fileCacheNotifyCallback(BFileCache::eCacheStatus newStatus, uint statusData, void* pCallbackDataPtr, uint64 callbackData);

   static BOOL       waitUntilAllCachesAreLoadedOrFailedCallback(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL       waitForAllDirsOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData);
   static BOOL       waitForFileOrFailureCallback(void* pCallbackDataPtr, uint64 callbackData);
};

extern BFileManager gFileManager;

// The "base" directory list ID is always an empty string, and is not modifiable by the caller in any way.
const long cDirBase        = BFileManager::cBaseDirListID;

const long cDirProduction  = BFileManager::cProductionDirListID;

#include "fileManager.inl"
