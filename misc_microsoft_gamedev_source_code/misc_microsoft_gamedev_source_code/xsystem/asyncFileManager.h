//==============================================================================
// asyncFileManager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xcore
#include "threading\win32Event.h"
#include "threading\win32Semaphore.h"
#include "threading\helperThread.h"
#include "containers\queue.h"

// local
#include "threading\eventDispatcher.h"

//==============================================================================
// class BAsyncFileManager
//==============================================================================
class BAsyncFileManager : public BEventReceiverInterface
{
public:
   BAsyncFileManager();
               
   virtual ~BAsyncFileManager();
      
   void init(void);
   void deinit(void);
                  
   enum eRequestType
   {
      cFileRead,
      cFileWrite,
            
      cRequestMax
   };
                     
   class BRequestPacket : public BEventPayload
   {
      friend BAsyncFileManager;
            
   public:
      BRequestPacket();
      
      BRequestPacket(const BRequestPacket& other);
      BRequestPacket& operator= (const BRequestPacket& rhs);
      
      BRequestPacket(BEventReceiverHandle receiverHandle, DWORD privateData0, DWORD privateData1, eRequestType requestType, int dirID, const char* pFilename, void* pData = NULL, uint dataLen = 0);

      virtual ~BRequestPacket();
      
      void deinit(void);
                        
      DWORD getPrivateData0(void) const { return mPrivateData0; }
      void setPrivateData0(DWORD privateData0) { mPrivateData0 = privateData0; }
      
      DWORD getPrivateData1(void) const { return mPrivateData1; }
      void setPrivateData1(DWORD privateData1) { mPrivateData1 = privateData1; }
      
      // By default, the packet's data will NOT be deleted when writing. 
      // Call setDeleteData() to true to have the data automatically deleted after the packet is processed.
      bool getDeleteData(void) const { return mFreeData; }
      void setDeleteData(bool freeData) { mFreeData = freeData; }
      
      void* getData(void) const { return mpData; }
      uint getDataLen(void) const { return mDataLen; }
      
      // Data should be allocated with BAlignedAlloc::Malloc().
      void setData(void* pData) { mpData = pData; }
      void setDataLen(uint dataLen) { mDataLen = dataLen; }
      
      eRequestType getRequestType(void) const { return mRequestType; }
      void setRequestType(eRequestType type) { mRequestType = type; }
      
      void setDirID(int dirID) { mDirID = dirID; }
      int getDirID(void) const { return mDirID; }
            
      void setFilename(const char* pFilename) { mFilename = pFilename; }
      const BFixedString128& getFilename(void) const { return mFilename; }
      
      void setAltFilename(const char* pFilename) { mAltFilename = pFilename; }
      const BFixedString128& getAltFilename(void) const { return mAltFilename; }
      
      void setUsedAltFilename(bool usedAltFilename) { mUsedAltFilename = usedAltFilename; }
      bool getUsedAltFilename(void) { return mUsedAltFilename; }
                  
      bool getSucceeded(void) const { return mSucceeded; }
      void setSucceeded(bool succeeded) { mSucceeded = succeeded; }            
                  
      // DO NOT poll this value, because it will be read/written by a worker thread.
      void setPending(bool pending) { mPending = pending; }
      bool getPending(void) { return mPending; }
      
      BEventReceiverHandle getReceiverHandle(void) const { return mReceiverHandle; }
      void setReceiverHandle(BEventReceiverHandle handle) { mReceiverHandle = handle; }
      
      uint getRequestIndex(void) const { return mRequestIndex; }
      void setRequestIndex(uint requestIndex) { mRequestIndex = requestIndex; }
      
      int getPriority(void) const { return mPriority; }
      void setPriority(int priority) { mPriority = static_cast<char>(priority); }
      
      BThreadIndex getCallbackThreadIndex(void) const { return mCallbackThreadIndex; }
      void setCallbackThreadIndex(BThreadIndex threadIndex) { mCallbackThreadIndex = threadIndex; }
      
      const BEventDispatcher::BDataFunctor& getCallback(void) const { return mCallback; }
      void setCallback(const BEventDispatcher::BDataFunctor& callback) { mCallback = callback; }
      
      void setSynchronousReply(bool synchronousReply) { mSynchronousReply = synchronousReply; }                              
      bool getSynchronousReply(void) const { return mSynchronousReply; }      
      
      void setDiscardOnClose(bool discardOnClose) { mDiscardOnClose = discardOnClose; }
      bool getDiscardOnClose(void) const { return mDiscardOnClose; }
      
   private:      
      BEventReceiverHandle mReceiverHandle;
      
      BThreadIndex mCallbackThreadIndex;
      BEventDispatcher::BDataFunctor mCallback;
            
      DWORD mPrivateData0;
      DWORD mPrivateData1;
            
      void* mpData;
      uint mDataLen;
      
      eRequestType mRequestType;
      
      int mDirID;
      BFixedString128 mFilename;
      BFixedString128 mAltFilename;
      
      uint mRequestIndex;
      
      char mPriority;
                  
      bool mPending : 1;
      bool mFreeData : 1;
      bool mSucceeded : 1;
      bool mUsedAltFilename : 1;
      bool mSynchronousReply : 1;
      bool mDiscardOnClose : 1;
      
      static volatile LONG mNumOutstandingRequestPackets;
          
      virtual void deleteThis(bool delivered);
   };
   
   BRequestPacket* newRequestPacket(void);
   void deleteRequestPacket(BRequestPacket* pPacket);
               
   // Submits a request for file I/O.
   // Once a request is submitted, DO NOT access the BRequestPacket object until you receive a reply event!
   // The memory pointed to by pPacket MUST remain stable and unmodified by the caller during the request.
   // It will be passed back to you by the event dispatcher.
   // You can then free/reuse the packet's memory.
   // This memory will be modified by a worker thread during processing.
   // This method may be called from any thread. 
   // The response event will have an event class of cEventClassAsyncFile.
   // Returns the submission's request index, which will never be 0.
   uint submitRequest(BRequestPacket* pPacket);
         
   BEventReceiverHandle getSimEventHandle(void) const { return mEventReceiverHandle; }
      
   uint getNumOutstandingRequests(void) const;
   uint getNumOutstandingIdleRequests(void) const;   
   
   void syncNormal(void);
   void syncIdle(void);
   void syncAll(void) { syncNormal(); syncIdle(); }
                        
private:
   BEventReceiverHandle mEventReceiverHandle;
      
   enum { cMaxQueuedRequests = 500 };
   typedef BDynamicArray<BRequestPacket*> BRequestQueue;
   BRequestQueue mQueuedRequests;
   BRequestQueue mQueuedIdleRequests;
   BCriticalSection mRequestQueueMutex;
   
   BWin32Semaphore mQueuedRequestsSemaphore;
   
   uint mNumFrameRequestsRemaining;
   
   uint mNextRequestIndex;
   
   int mWaitTime;
         
   volatile LONG mNumOutstandingRequests;
   volatile LONG mNumOutstandingIdleRequests;
         
   bool processRequest(BRequestPacket* pPacket);
      
   static void threadFunc(uint data);
   
   static int readFile(int dirID, const char* pFilename, void** pData, uint* pDataLen, bool discardOnClose);
   static bool writeFile(int dirID, const char* pFilename, const void* pData, uint dataLen);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
}; 

//==============================================================================
// externs
//==============================================================================
extern BAsyncFileManager gAsyncFileManager;

extern BHelperThread gIOHelperThread;

