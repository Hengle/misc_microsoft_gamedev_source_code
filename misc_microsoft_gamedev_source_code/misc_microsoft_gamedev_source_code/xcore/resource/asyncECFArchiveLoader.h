//============================================================================
//
// File: asyncECFArchiveLoader.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "threading\eventDispatcher.h"
#include "hash\bsha1.h"
#include "hash\tiger.h"
#include "file\lowLevelFileIO.h"

class BAsyncECFArchiveLoader : public BEventReceiver
{
public:
   BAsyncECFArchiveLoader();
   ~BAsyncECFArchiveLoader();

   void init(eThreadIndex threadIndex, uint numPublicKeys, const BSHA1* pPublicKeys, const uint64 decryptKey[3], ILowLevelFileIO* pLowLevelFileIO = NULL, uint bufSize = 0);
   void deinit(void);
   
   bool getInitialized(void) const { return cInvalidEventReceiverHandle != mEventHandle; }

   enum eCallbackMessageType
   {
      cCMTInvalid = 0,
      cCMTLoadBegun,
      cCMTLoadFinished,
      cCMTDeletingRequest,
      
      cCMTErrorLoadCanceled = -100,
      cCMTErrorOpenFailed,
      cCMTErrorReadFailed,
      cCMTErrorBadSignature,
      cCMTErrorBadArchive,
      cCMTErrorBadChunk,
      cCMTErrorChunkAllocFailed,
      cCMTErrorChunkReadyCallbackFailed,
      cCMTErrorHeaderDataCallbackFailed,
   };

   class BCallbackInterface
   {
   public:
      virtual ~BCallbackInterface() { }

      virtual void message(uint64 callbackData, eCallbackMessageType messageType)
      {
         callbackData;
         messageType;
      }
      
      virtual bool headerData(uint64 callbackData, const void* p, uint size)
      {
         callbackData;
         p;
         size;
         return true;
      }
      
      virtual bool chunkAlloc(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation, uint chunkSize)
      {
         callbackData;
         chunkIndex;
         return allocation.alloc(chunkSize, &gRenderHeap);
      }
      
      // The allocation MUST either be saved for later or free, even if you return false!
      virtual bool chunkReady(uint64 callbackData, uint chunkIndex, BMemoryHeapAllocation& allocation)
      {
         callbackData;
         chunkIndex;
         #pragma push_macro("free")
         #undef free
         bool success = allocation.free();
         #pragma pop_macro("free")
         success;
         BDEBUG_ASSERT(success);
         return true;
      }
   };
   
   typedef uint64 BLoadHandle;
   enum { cInvalidLoadHandle = 0 };

   bool begin(const char* pFilename, BCallbackInterface* pCallback, uint64 callbackData, BLoadHandle& handle);

   bool cancel(BLoadHandle handle, bool wait = false);
   
   bool cancelAll(bool wait = false);
   
   // Waiting is unlikely to be a good idea.
   bool pause(bool wait = false);
   bool resume(void);
   
private:
   struct BRequest
   {
      uint64               mCallbackData;
      uint                 mNonce;         
      BCallbackInterface*  mpCallback;
            
      // FATX limits filenames to 42 characters, so 64 should be more than enough.
      BFixedString64      mFilename;
   };
   
   enum
   {
      cEventClassLoadBegin = cEventClassFirstUser,
      cEventClassLoadCancel,
      cEventClassLoadCancelAll,
      cEventReadFileEvent,
      cEventClassPause,
      cEventClassResume
   }; 
   
   ILowLevelFileIO*           mpLowLevelFileIO;
         
   typedef BDynamicArray<BSHA1, ALIGN_OF(BSHA1), BDynamicArrayRenderHeapAllocator> BSHA1Array;  
   BSHA1Array                 mPublicKeys;
   uint64                     mDecryptKey[3];
      
   typedef BDynamicArray<BRequest*, ALIGN_OF(BRequest*), BDynamicArrayRenderHeapAllocator> BRequestPtrArray;     
   BRequestPtrArray           mRequestQueue;
   BRequest*                  mpCurRequest;
   
   HANDLE                     mArchiveFileHandle;
   uint64                     mArchiveFileSize;
   uint64                     mArchiveFileOfs;
            
   // This class assumes double buffering!
   enum { cNumBuffers = 2, cDefaultBufSize = 5U * 1024U * 1024U };
   void*                      mpBuffers[cNumBuffers];
   uint                       mBufSize;
   
   uint                       mCurBufIndex;
   
   BWin32Event                mOverlappedEvent;
   OVERLAPPED                 mOverlapped;
   uint                       mCurReadSize;
            
   BMemoryHeapAllocation      mCurChunkAllocation;
   BTigerHashGen              mCurChunkTigerHashGen;
   int                        mCurChunkIndex;
   uint                       mCurChunkOfs;
         
   struct BChunkInfo
   {
      uint64   mOfs;
      uint     mSize;
      uchar    mCompTiger128[16];
   };
   typedef BDynamicArray<BChunkInfo, ALIGN_OF(BChunkInfo), BDynamicArrayRenderHeapAllocator> BChunkInfoArray; 
   BChunkInfoArray            mChunkInfo;
   
   enum
   {
      cStateOpenArchive,
      cStateInitiateRead,
      cStateWaitingForRead,
      cStateCompleteRead,
      cStateFinishedArchive,
      cStateCloseArchive
   };
   
   uint                       mCurState;
   bool                       mReadIsPending;
   bool                       mIsPaused;
   
   eThreadIndex               mThreadIndex;
         
   void clear(void);
   void initBuffers(void);
   void deinitBuffers(void);
   void deleteRequest(BRequest* pRequest);
   void deleteActiveRequest(void);
   void reportMessage(BRequest* pRequest, eCallbackMessageType messageType);
   bool openArchive(DWORD& nextState);
   bool initiateRead(DWORD& nextState);
   bool waitingForRead(DWORD& nextState);
   void decryptData(const BYTE* pSrc, BYTE* pDst, uint size, uint64 bufFileOfs);
   bool processHeaderData(uint64 bufFileOfs, uint bufIndex, uint bufSize, uint& totalHeaderBytes);
   bool processChunkData(uint64 bufFileOfs, uint bufIndex, uint bufSize, uint initialBufOfs);
   bool processBuffer(uint64 bufFileOfs, uint bufIndex, uint bufSize);
   bool completeRead(DWORD& nextState);
   bool finishedArchive(DWORD& nextState);
   bool closeArchive(DWORD& nextState);
   void tickState(void);
   void processLoadBegin(const BEvent& event);
   void processReadFileEvent(const BEvent& event);
   void cancelRequest(uint index);
   void cancelCurrentRequest(void);
   void processLoadCancel(const BEvent& event);
   void processClientAdded(void);
   void cancelAllRequests(void);
   void processCancelAll(void);
   void processPause(void);
   void processResume(void);
   void processClientRemove(bool terminating);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};
