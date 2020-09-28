// File: renderAsyncFileManager.cpp
#include "xrender.h"
#include "renderAsyncFileManager.h"
#include "renderThread.h"

BRenderAsyncFileManager gRenderAsyncFileManager;

BRenderAsyncFileManager::BRequestPacket::BRequestPacket() :
   BRenderEventPayload(),
   mReceiverHandle(cInvalidRenderEventReceiverHandle),
   mPrivateData0(0),
   mPrivateData1(0),
   mRequestType(cFileRead),
   mDirID(0),
   mpData(NULL),
   mDataLen(0),
   mFreeData(false),
   mSucceeded(false),
   mPending(false),
   mUsedAltFilename(false)
{
}         

BRenderAsyncFileManager::BRequestPacket::BRequestPacket(BRenderEventReceiverHandle receiverHandle, DWORD privateData0, DWORD privateData1, eRequestType requestType, int dirID, const char* pFilename, void* pData, uint dataLen) :
   BRenderEventPayload(),
   mReceiverHandle(receiverHandle),
   mPrivateData0(privateData0),
   mPrivateData1(privateData1),
   mRequestType(requestType),
   mDirID(dirID),
   mFilename(pFilename),
   mpData(pData),
   mDataLen(dataLen),
   mFreeData(false),
   mSucceeded(false),
   mPending(false),
   mUsedAltFilename(false)
{
}

void BRenderAsyncFileManager::BRequestPacket::deinit(void)
{
   if (mFreeData)
   {
      gRenderThread.freeAligned(mpData);
      mpData = NULL;
      mFreeData = false;
   }
}            

void BRenderAsyncFileManager::BRequestPacket::deleteThis(bool delivered)
{
   gRenderThread.deleteAligned<BRequestPacket>(this);
}

BRenderAsyncFileManager::BRenderAsyncFileManager() :
   mpMainThreadFileInterface(NULL),
   mMainEventReceiverHandle(cInvalidRenderEventReceiverHandle),
   mHeartbeatHandle(cInvalidHeartbeatHandle),
   mNumOutstandingRequests(0)
{
}

BRenderAsyncFileManager::~BRenderAsyncFileManager()
{
}

void BRenderAsyncFileManager::init(BMainThreadFileInterface* pFileIOInterface)
{
   BDEBUG_ASSERT(pFileIOInterface);
   mpMainThreadFileInterface = pFileIOInterface;
   
   if (cInvalidRenderEventReceiverHandle == mMainEventReceiverHandle)
      mMainEventReceiverHandle = gRenderEventDispatcher.addClient(this, cRenderThreadIndexMain);
   
   if (mHeartbeatHandle == cInvalidHeartbeatHandle)
      mHeartbeatHandle = gRenderEventDispatcher.addHeartbeatReceiver(mMainEventReceiverHandle, 1);

   mNumOutstandingRequests = 0;
}

void BRenderAsyncFileManager::deinit(void)
{
   if (cInvalidRenderEventReceiverHandle != mMainEventReceiverHandle)
   {
      gRenderEventDispatcher.removeClient(mMainEventReceiverHandle);
            
      mMainEventReceiverHandle = cInvalidRenderEventReceiverHandle;
   }
   
   if (mHeartbeatHandle != cInvalidHeartbeatHandle)
   {
      gRenderEventDispatcher.removeHeartbeatReceiver(mHeartbeatHandle);
      mHeartbeatHandle = cInvalidHeartbeatHandle;
   }
}

BRenderAsyncFileManager::BRequestPacket* BRenderAsyncFileManager::newRequestPacket(void)
{
   return gRenderThread.newAligned<BRenderAsyncFileManager::BRequestPacket>();
}

bool BRenderAsyncFileManager::submitRequest(BRequestPacket* pPacket)
{
   BDEBUG_ASSERT(cInvalidRenderEventReceiverHandle != mMainEventReceiverHandle && pPacket);
   BDEBUG_ASSERT(pPacket->getReceiverHandle() != cInvalidRenderEventReceiverHandle);
      
   pPacket->setPending(true);
         
   if (!gRenderEventDispatcher.send(cInvalidRenderEventReceiverHandle, mMainEventReceiverHandle, cRenderEventClassAsyncFile, 0, 0, pPacket))
   {
      pPacket->deleteThis(false);
      return false;
   }
   
   InterlockedIncrementWithSync(&mNumOutstandingRequests);
   
   return true;
}

bool BRenderAsyncFileManager::receiveEvent(const BRenderEvent& event, int threadIndex)
{
   ASSERT_MAIN_THREAD
   
   if (event.mEventClass == cRenderEventClassAsyncFile)
   {
      BRequestPacket* pPacket = reinterpret_cast<BRequestPacket*>(event.mpPayload);
   
      if (!mQueuedRequests.pushFront(pPacket))
      {
         const bool succeeded = processRequest(pPacket);
         
         if (!succeeded)
         {
            trace("BRenderAsyncFileManager::receiveEvent: All queues full, file request must be dropped");
         }
         
         InterlockedDecrementWithSync(&mNumOutstandingRequests);
            
         return succeeded;
      }
   }
   else if (event.mEventClass == cRenderEventClassHeartbeat)
   {
      processQueuedRequests();
   }
   else
   {
      BFAIL("BRenderAsyncFileManager::receiveEvent: Unexpected event class");
   }
   
   return true;
}

bool BRenderAsyncFileManager::processRequest(BRequestPacket* pPacket)
{
   ASSERT_MAIN_THREAD
   
   if (pPacket->getPending())
   {
      switch (pPacket->getRequestType())
      {
         case cFileRead:
         {
            void* pData = NULL;
            uint dataLen = 0;
            if (!mpMainThreadFileInterface->readFile(pPacket->getDirID(), pPacket->getFilename(), &pData, &dataLen))
            {
               if (!mpMainThreadFileInterface->readFile(pPacket->getDirID(), pPacket->getAltFilename(), &pData, &dataLen))
                  break;
               pPacket->setUsedAltFilename(true);
            }

            pPacket->setData(pData);
            pPacket->setDataLen(dataLen);
            pPacket->setDeleteData(true);
            pPacket->setSucceeded(true);

            break;
         }
         case cFileWrite:
         {
            if (!mpMainThreadFileInterface->writeFile(pPacket->getDirID(), pPacket->getFilename(), pPacket->getData(), pPacket->getDataLen()))
               break;

            pPacket->setSucceeded(true);

            break;
         }
         default:
         {
            BFAIL("Invalid file request type in render async file manager");
         }
      }

      pPacket->setPending(false);
   }   

   // Send the results off to the destination.
   if (!gRenderEventDispatcher.send(mMainEventReceiverHandle, pPacket->getReceiverHandle(), cRenderEventClassAsyncFile, 0, 0, pPacket))
   {
      trace("BRenderAsyncFileManager::receiveEvent: send failed");
      return false;
   }
      
   return true;
}

void BRenderAsyncFileManager::processQueuedRequests(void)
{
   ASSERT_MAIN_THREAD   

   const uint cMaxRequestsPerFrame = 1;
            
   for (uint i = 0; i < cMaxRequestsPerFrame; i++)
   {
      BRequestPacket* pPacket;
      if (!mQueuedRequests.popBack(pPacket))
         break;

      if (!processRequest(pPacket))
      {
         // Send failed, try again later.
         mQueuedRequests.pushBack(pPacket);
         break;
      }
      
      InterlockedDecrementWithSync(&mNumOutstandingRequests);
   }
}

uint BRenderAsyncFileManager::getNumOutstandingRequests(void) const
{
   MemoryBarrier();
   
   return mNumOutstandingRequests;
}

