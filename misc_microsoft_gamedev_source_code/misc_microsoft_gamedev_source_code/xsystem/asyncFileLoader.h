// File: asyncFileLoader.h
#pragma once

#include "asyncFileManager.h"

//==============================================================================
// class BAsyncFileLoader
// Simple wrapper to help load files asynchronously.
// Note: This is a single threaded API!
//==============================================================================
class BAsyncFileLoader : public BEventReceiver
{
public:
   BAsyncFileLoader(BThreadIndex clientThreadIndex = cInvalidIndex);

   ~BAsyncFileLoader();

   // This method frees up any memory allocated for the request packet.
   // This includes the file data, unless you call assumeDataOwnership()!
   // May be called at any time, even while a request is in progress.
   //
   // if you set deinit to true, we'll also unregister with the event dispatcher
   void clear(bool deinit=false);

   // Submits a request to load a file asynchronously.
   // You may call this while a request is in progress. Only the last
   // submitted request will be recognized and all previous requests will be ignored.
   bool load(long dirID, const char* pFilename, bool highPriorityRequest = false, bool discardOnClose = false);

   bool getPending(void) const;

   // Returns true if the request has completed. The request may not have been successful!
   bool getReady(void) const;

   // Blocks until the request has completed.
   // Returns false if the wait times out, or it no request is pending.
   bool waitUntilReady(DWORD timeout = INFINITE);

   // Returns true if the request is finished and was successful.
   bool getSucceeded(void) const;

   // Returns true if the request is finished and was a failure.
   bool getFailed(void) const;

   // Returns a pointer to the file data.
   // Call clear() once you are done reading the data to free it.
   void* getData(void) const;

   // Returns the length of the file data, or -1 if the request hasn't completed or succeeded.
   int getDataLen(void) const;

   // Prevents the data buffer returned by getData() from being deleted when clear() is called.
   // You must delete the buffer by calling BAlignedAlloc::Free()!
   bool assumeDataOwnership(void);

   BThreadIndex getOwnerThreadIndex(void) const { return mOwnerThread; }
   void setOwnerThread(BThreadIndex ownerThread);

   typedef void (*BEventCallback)(BAsyncFileLoader* pLoader, void* pData);
   void setEventCallback(BEventCallback pCallback, void* pCallbackData) { mpEventCallback = pCallback; mpEventCallbackData = pCallbackData; }   

private:
   uint mRequestID;
   BAsyncFileManager::BRequestPacket* mpRequestPacket;
   BThreadIndex mOwnerThread;

   BEventCallback mpEventCallback;
   void* mpEventCallbackData;

   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};
