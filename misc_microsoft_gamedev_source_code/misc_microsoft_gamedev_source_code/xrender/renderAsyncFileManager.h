// File: renderAsyncFile.h
#pragma once

// xcore
#include "threading\win32Event.h"
#include "containers\queue.h"

// local
#include "renderEventDispatcher.h"
#include "renderCommand.h"

class BRenderAsyncFileManager : public BRenderEventReceiverInterface
{
public:
   // This interface is called from the main thread to do file I/O on behalf of the async file manager.
   class BMainThreadFileInterface
   {
   public:
      // Memory for the data buffer must be allocated through gRenderThread.newAligned().
      virtual bool readFile(int dirID, const char* pFilename, void** pData, uint* pDataLen) = 0;
      virtual bool writeFile(int dirID, const char* pFilename, const void* pData, uint dataLen) = 0;
   };
   
   BRenderAsyncFileManager();
               
   virtual ~BRenderAsyncFileManager();
      
   void init(BMainThreadFileInterface* pFileIOInterface);
   
   void deinit(void);
      
   enum eRequestType
   {
      cFileRead,
      cFileWrite,
            
      cRequestMax
   };
         
   class BRequestPacket : public BRenderEventPayload
   {
   public:
      BRequestPacket();
               
      BRequestPacket(BRenderEventReceiverHandle receiverHandle, DWORD privateData0, DWORD privateData1, eRequestType requestType, int dirID, const char* pFilename, void* pData = NULL, uint dataLen = 0);
                        
      virtual ~BRequestPacket() { deinit(); }
      
      virtual void deleteThis(bool delivered);
      
      void deinit(void);
            
      DWORD getPrivateData0(void) const { return mPrivateData0; }
      void setPrivateData0(DWORD privateData0) { mPrivateData0 = privateData0; }
      
      DWORD getPrivateData1(void) const { return mPrivateData1; }
      void setPrivateData1(DWORD privateData1) { mPrivateData1 = privateData1; }
      
      bool getDeleteData(void) const { return mFreeData; }
      void setDeleteData(bool freeData) { mFreeData = freeData; }
      
      void* getData(void) const { return mpData; }
      uint getDataLen(void) const { return mDataLen; }
      
      void setData(void* pData) { mpData = pData; }
      void setDataLen(uint dataLen) { mDataLen = dataLen; }
      
      eRequestType getRequestType(void) const { return mRequestType; }
      
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
                  
      void setPending(bool pending) { mPending = pending; }
      bool getPending(void) { return mPending; }
      
      BRenderEventReceiverHandle getReceiverHandle(void) const { return mReceiverHandle; }
      void setReceiverHandle(BRenderEventReceiverHandle handle) { mReceiverHandle = handle; }
                              
   private:      
      BRenderEventReceiverHandle mReceiverHandle;
      
      DWORD mPrivateData0;
      DWORD mPrivateData1;
            
      void* mpData;
      uint mDataLen;
      
      eRequestType mRequestType;
      
      int mDirID;
      BFixedString128 mFilename;
      BFixedString128 mAltFilename;
                  
      bool mPending : 1;
      bool mFreeData : 1;
      bool mSucceeded : 1;
      bool mUsedAltFilename : 1;
      
      BRequestPacket(const BRequestPacket& other);
      BRequestPacket& operator= (const BRequestPacket& rhs);
   };
   
   BRequestPacket* newRequestPacket(void);
               
   // Submits a request for file I/O.
   // The memory pointed to by pPacket MUST remain stable and unmodified by the caller during the request.
   // It will be passed back to you by the event dispatcher.
   // You can then free/reuse the packet's memory.
   // This memory will be modified by the main thread during processing.
   // This method may be called from any thread. 
   // You must handle failures gracefully. The packet may never come back.
   bool submitRequest(BRequestPacket* pPacket);
   
   BRenderEventReceiverHandle getEventHandle(void) const { return mMainEventReceiverHandle; }
   
   uint getNumOutstandingRequests(void) const;
               
private:
   BRenderEventReceiverHandle mMainEventReceiverHandle;
   BMainThreadFileInterface* mpMainThreadFileInterface;
   BHeartbeatHandle mHeartbeatHandle;
   
   enum { cMaxQueuedRequests = 2048 };
   typedef BQueue<BRequestPacket*, cMaxQueuedRequests> BRequestQueue;
   BRequestQueue mQueuedRequests;
         
   volatile LONG mNumOutstandingRequests;
      
   bool processRequest(BRequestPacket* pPacket);
   void processQueuedRequests(void);
   
   virtual bool receiveEvent(const BRenderEvent& event, int threadIndex);
}; 

extern BRenderAsyncFileManager gRenderAsyncFileManager;
