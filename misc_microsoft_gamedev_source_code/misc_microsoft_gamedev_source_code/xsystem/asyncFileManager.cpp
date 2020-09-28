//==============================================================================
// asyncFileManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xsystem.h"
#include "asyncFileManager.h"
#include "file\win32File.h"
#include "threading\workDistributor.h"
#include "xfs.h"

//==============================================================================
// Globals
//==============================================================================
BAsyncFileManager gAsyncFileManager;

BHelperThread gIOHelperThread;

volatile LONG BAsyncFileManager::BRequestPacket::mNumOutstandingRequestPackets;

//==============================================================================
// BAsyncFileManager::BRequestPacket::BRequestPacket
//==============================================================================
BAsyncFileManager::BRequestPacket::BRequestPacket() :
   BEventPayload(),
   mReceiverHandle(cInvalidEventReceiverHandle),
   mPrivateData0(0),
   mPrivateData1(0),
   mRequestType(cFileRead),
   mDirID(0),
   mpData(NULL),
   mDataLen(0),
   mFreeData(false),
   mSucceeded(false),
   mPending(false),
   mUsedAltFilename(false),
   mRequestIndex(0),
   mPriority(-1),
   mCallbackThreadIndex(cThreadIndexInvalid),
   mSynchronousReply(false),
   mDiscardOnClose(false)
{
   InterlockedIncrement(&mNumOutstandingRequestPackets);
}         

//==============================================================================
// BAsyncFileManager::BRequestPacket::BRequestPacket
//==============================================================================
BAsyncFileManager::BRequestPacket::BRequestPacket(const BRequestPacket& other) :
   BEventPayload(),
   mReceiverHandle(cInvalidEventReceiverHandle),
   mPrivateData0(0),
   mPrivateData1(0),
   mRequestType(cFileRead),
   mDirID(0),
   mpData(NULL),
   mDataLen(0),
   mFreeData(false),
   mSucceeded(false),
   mPending(false),
   mUsedAltFilename(false),
   mRequestIndex(0),
   mPriority(-1),
   mCallbackThreadIndex(cThreadIndexInvalid),
   mDiscardOnClose(false)
{
   InterlockedIncrement(&mNumOutstandingRequestPackets);
   *this = other;
}

//==============================================================================
// BAsyncFileManager::BRequestPacket::BRequestPacket
//==============================================================================
 BAsyncFileManager::BRequestPacket::BRequestPacket(BEventReceiverHandle receiverHandle, DWORD privateData0, DWORD privateData1, eRequestType requestType, int dirID, const char* pFilename, void* pData, uint dataLen) :
   BEventPayload(),
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
   mUsedAltFilename(false),
   mRequestIndex(0),
   mCallbackThreadIndex(cThreadIndexInvalid),
   mDiscardOnClose(false)
{
   InterlockedIncrement(&mNumOutstandingRequestPackets);
}

//==============================================================================
// BAsyncFileManager::BRequestPacket::~BRequestPacket
//==============================================================================
BAsyncFileManager::BRequestPacket::~BRequestPacket() 
{ 
   InterlockedDecrement(&mNumOutstandingRequestPackets); 
   deinit(); 
}   

//==============================================================================
// BAsyncFileManager::BRequestPacket::operator=
//==============================================================================
BAsyncFileManager::BRequestPacket& BAsyncFileManager::BRequestPacket::operator= (const BRequestPacket& rhs)
{
   if (this == &rhs)
      return *this;
   
   deinit();
   
   *static_cast<BEventPayload*>(this) = rhs;
   
   mReceiverHandle = rhs.mReceiverHandle;
   mPrivateData0 = rhs.mPrivateData0;
   mPrivateData1 = rhs.mPrivateData1;
   mRequestType = rhs.mRequestType;
   mDirID = rhs.mDirID;
   mFilename = rhs.mFilename;
   
   if (rhs.mpData)
   {
      mpData = BAlignedAlloc::Malloc(rhs.mDataLen);
      Utils::FastMemCpy(mpData, rhs.mpData, rhs.mDataLen);
      mDataLen = rhs.mDataLen;
      
      mFreeData = true;
   }
      
   mSucceeded = rhs.mSucceeded;
   mPending = rhs.mPending;
   mUsedAltFilename = rhs.mUsedAltFilename;
   mRequestIndex = rhs.mRequestIndex;
   mPriority = rhs.mPriority;
   mCallback = rhs.mCallback;
   mCallbackThreadIndex = rhs.mCallbackThreadIndex;
   mSynchronousReply = rhs.mSynchronousReply;
   mDiscardOnClose = rhs.mDiscardOnClose;
      
   return *this;
}

//==============================================================================
// BAsyncFileManager::BRequestPacket::deinit
//==============================================================================
void BAsyncFileManager::BRequestPacket::deinit(void)
{
   if (mFreeData)
   {
      BAlignedAlloc::Free(mpData);
      mpData = NULL;
      mFreeData = false;
   }
}            

//==============================================================================
// BAsyncFileManager::BRequestPacket::deleteThis
//==============================================================================
void BAsyncFileManager::BRequestPacket::deleteThis(bool delivered)
{
   delivered;
   delete this;
}

//==============================================================================
// BAsyncFileManager::BAsyncFileManager
//==============================================================================
BAsyncFileManager::BAsyncFileManager() :
   mEventReceiverHandle(cInvalidEventReceiverHandle),
   mNumOutstandingRequests(0),
   mNumOutstandingIdleRequests(0),
   mNextRequestIndex(0),
   mQueuedRequestsSemaphore(0, cMaxQueuedRequests),
   mWaitTime(33)
{
   mQueuedRequests.reserve(64);
   mQueuedIdleRequests.reserve(64);
}

//==============================================================================
// BAsyncFileManager::~BAsyncFileManager
//==============================================================================
BAsyncFileManager::~BAsyncFileManager()
{
}

//==============================================================================
// BAsyncFileManager::threadFunc
//==============================================================================
void BAsyncFileManager::threadFunc(uint data)
{
   BAsyncFileManager* pAsyncFileManager = reinterpret_cast<BAsyncFileManager*>(data);
   
   DWORD lastIdleCheckTime = GetTickCount();
   
   for ( ; ; )
   {
      HANDLE handles[3];
      handles[0] = gIOHelperThread.getTerminateEvent().getHandle();
      handles[1] = pAsyncFileManager->mQueuedRequestsSemaphore.getHandle();
      
      uint numHandles = 2;
      if (gWorkDistributor.getInitialized())
      {
         handles[2] = gWorkDistributor.getWorkAvailableSemaphore().getHandle();
         numHandles = 3;
      }
                  
      int waitResult = gEventDispatcher.wait(numHandles, handles, 5);
      
      if (0 == waitResult)
         break;
      else if (waitResult == 1) 
      {
         BRequestPacket* pPacket = NULL;
         
         pAsyncFileManager->mRequestQueueMutex.lock();
            
         if (!pAsyncFileManager->mQueuedRequests.empty())
         {
            pPacket = pAsyncFileManager->mQueuedRequests.back();
            pAsyncFileManager->mQueuedRequests.popBack();
         }
         
         pAsyncFileManager->mRequestQueueMutex.unlock();
         
         if (pPacket)
         {
            bool completed = pAsyncFileManager->processRequest(pPacket);
            if (completed)
            {
               pAsyncFileManager->mWaitTime = Math::Min(60, pAsyncFileManager->mWaitTime + 20);
               
               InterlockedDecrement(&pAsyncFileManager->mNumOutstandingRequests);
            }
            else
            {
               pPacket->setPriority(pPacket->getPriority() - 256);
               
               pAsyncFileManager->mRequestQueueMutex.lock();

               pAsyncFileManager->mQueuedIdleRequests.insert(0, pPacket);

               pAsyncFileManager->mRequestQueueMutex.unlock();
               
               InterlockedIncrement(&pAsyncFileManager->mNumOutstandingIdleRequests);
            }
         }
      }
      else if (waitResult == 2)
      {
         BDEBUG_ASSERT(numHandles >= 3);
         gWorkDistributor.popWorkAndProcess();
      }
      
      DWORD timeSinceLastIdleCheck = GetTickCount() - lastIdleCheckTime;
      
      if (timeSinceLastIdleCheck >= (DWORD)pAsyncFileManager->mWaitTime)
      {
         BRequestPacket* pPacket = NULL;
         
         pAsyncFileManager->mRequestQueueMutex.lock();
         
         if (!pAsyncFileManager->mQueuedIdleRequests.empty())
         {
            pPacket = pAsyncFileManager->mQueuedIdleRequests.back();
            pAsyncFileManager->mQueuedIdleRequests.popBack();
         }
         
         pAsyncFileManager->mRequestQueueMutex.unlock();
         
         if (pPacket)
         {
            pAsyncFileManager->mWaitTime = Math::Max(4, pAsyncFileManager->mWaitTime - 10);
            
            bool completed = pAsyncFileManager->processRequest(pPacket);
            if (completed)
               InterlockedDecrement(&pAsyncFileManager->mNumOutstandingIdleRequests);
            else
            {
               pAsyncFileManager->mRequestQueueMutex.lock();

               pAsyncFileManager->mQueuedIdleRequests.insert(0, pPacket);

               pAsyncFileManager->mRequestQueueMutex.unlock();
            }
         }
         
         lastIdleCheckTime = GetTickCount();
      }
   }
}

//==============================================================================
// BAsyncFileManager::init
//==============================================================================
void BAsyncFileManager::init(void)
{
   if (cInvalidEventReceiverHandle != mEventReceiverHandle)
      return;
      
   mEventReceiverHandle = gEventDispatcher.addClient(this, cThreadIndexIO);
   
   mNumOutstandingRequests = 0;
   mNumOutstandingIdleRequests = 0;
   
   gIOHelperThread.init(cThreadIndexIO, 4, "IOHelper", threadFunc, reinterpret_cast<DWORD>(this));
}

//==============================================================================
// BAsyncFileManager::deinit
//==============================================================================
void BAsyncFileManager::deinit(void)
{
   if (cInvalidEventReceiverHandle == mEventReceiverHandle)
      return;
      
   //syncAll();
   
   gEventDispatcher.removeClientDeferred(mEventReceiverHandle, true);
   mEventReceiverHandle = cInvalidEventReceiverHandle;
   
   gIOHelperThread.deinit();
}

//==============================================================================
// BAsyncFileManager::syncNormal
//==============================================================================
void BAsyncFileManager::syncNormal(void)
{
   while (getNumOutstandingRequests())
      gEventDispatcher.wait(0, NULL, 33);
}

//==============================================================================
// 
//==============================================================================
void BAsyncFileManager::syncIdle(void)
{
   while (getNumOutstandingIdleRequests())
      gEventDispatcher.wait(0, NULL, 33);
}

//==============================================================================
// BAsyncFileManager::newRequestPacket
//==============================================================================
BAsyncFileManager::BRequestPacket* BAsyncFileManager::newRequestPacket(void)
{
   return new BAsyncFileManager::BRequestPacket;
}

//==============================================================================
// BAsyncFileManager::deleteRequestPacket
//==============================================================================
void BAsyncFileManager::deleteRequestPacket(BAsyncFileManager::BRequestPacket* pPacket)
{
   delete pPacket;
}

//==============================================================================
// BAsyncFileManager::submitRequest
//==============================================================================
uint BAsyncFileManager::submitRequest(BRequestPacket* pPacket)
{
   BDEBUG_ASSERT(cInvalidEventReceiverHandle != mEventReceiverHandle && pPacket);
   BDEBUG_ASSERT( (pPacket->getReceiverHandle() != cInvalidEventReceiverHandle) || (pPacket->getCallbackThreadIndex() != cThreadIndexInvalid) );
         
   pPacket->setPending(true);
   if (0 == mNextRequestIndex)
      mNextRequestIndex++;
   pPacket->setRequestIndex(mNextRequestIndex);
   mNextRequestIndex++;

   const int priority = pPacket->getPriority();

   mRequestQueueMutex.lock();
   
   BRequestQueue* pQueue = (priority >= 0) ? &mQueuedRequests : &mQueuedIdleRequests;

   int index;
   for (index = pQueue->size() - 1; index >= 0; index--)
      if (priority >= (*pQueue)[index]->getPriority())
         break;      
   pQueue->insert(index + 1, pPacket);
   
   mRequestQueueMutex.unlock();
   
   if (priority >= 0)
   {
      mQueuedRequestsSemaphore.release();
      InterlockedIncrement(&mNumOutstandingRequests);
   }
   else
   {
      InterlockedIncrement(&mNumOutstandingIdleRequests);
   }
         
   return pPacket->getRequestIndex();
}

//==============================================================================
// BAsyncFileManager::receiveEvent
//==============================================================================
bool BAsyncFileManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   threadIndex;
   
   return true;
}

//==============================================================================
// BAsyncFileManager::readFile
//==============================================================================
int BAsyncFileManager::readFile(int dirID, const char* pFilename, void** pData, uint* pDataLen, bool discardOnClose)
{
   BFile file;
   bool success = file.openReadOnly(dirID, pFilename, BFILE_OPEN_DO_NOT_WAIT_FOR_FILE | (discardOnClose ? BFILE_OPEN_DISCARD_ON_CLOSE : 0));
   if (!success)
   {
      if (BFILE_WOULD_HAVE_WAITED == file.getLastError())
         return -1;
      else
         return 0;
   }
   
   uint64 fileSize64;
   if (!file.getSize(fileSize64))
      return 0;
   
   const uint cMaxFileSize = 512U*1024U*1024U;
   if (fileSize64 > cMaxFileSize)
      return 0;
      
   uint fileSize = (uint)fileSize64;      
   
   void* pBuf = BAlignedAlloc::Malloc(fileSize);
   if (!pBuf)
      return 0;
      
   if (!file.read(pBuf, fileSize))
   {
      BAlignedAlloc::Free(pBuf);
      return 0;
   }
   
   *pData = pBuf;
   *pDataLen = fileSize;
   
   return 1;
}

//==============================================================================
// BAsyncFileManager::writeFile
//==============================================================================
bool BAsyncFileManager::writeFile(int dirID, const char* pFilename, const void* pData, uint dataLen)
{
   BFile file;
   if (!file.openWriteable(dirID, pFilename))
      return false;

   if (dataLen)
   {
      if (!file.write(pData, dataLen))
      {
         file.close();
         return false;
      }
   }      
   
   if (!file.close())
      return false;

   return true;
}

//==============================================================================
// BAsyncFileManager::processRequest
//==============================================================================
bool BAsyncFileManager::processRequest(BRequestPacket* pPacket)
{
   if (pPacket->getPending())
   {
      switch (pPacket->getRequestType())
      {
         case cFileRead:
         {
            void* pData = NULL;
            uint dataLen = 0;
            
            int status = readFile(pPacket->getDirID(), pPacket->getFilename(), &pData, &dataLen, pPacket->getDiscardOnClose());
            if (-1 == status)
               return false;
            else if (0 == status) 
            {
               if (pPacket->getAltFilename().length())
               {
                  BFileManager::BFileDesc fileDesc;
                  
                  eFileManagerError fileManagerStatus = gFileManager.getFileDesc(pPacket->getDirID(), pPacket->getAltFilename(), fileDesc, BFileManager::cUseAll | BFileManager::cDoNotWait);
                  if (cFME_WOULD_HAVE_BLOCKED == fileManagerStatus)
                     return false;
                  else if (cFME_SUCCESS == fileManagerStatus)
                  {
                     status = readFile(pPacket->getDirID(), pPacket->getAltFilename(), &pData, &dataLen, pPacket->getDiscardOnClose());
                     if (-1 == status)
                        return false;
                     else if (0 == status)
                        break;
                     
                     pPacket->setUsedAltFilename(true);
                  }
                  else
                     break;
               }
               else
                  break;
            }               

            pPacket->setData(pData);
            pPacket->setDataLen(dataLen);
            pPacket->setDeleteData(true);
            pPacket->setSucceeded(true);
                        
            break;
         }
         case cFileWrite:
         {
            if (!writeFile(pPacket->getDirID(), pPacket->getFilename(), pPacket->getData(), pPacket->getDataLen()))
               break;

            pPacket->setSucceeded(true);

            break;
         }
         default:
         {
            BFAIL("Invalid file request type in  async file manager");
         }
      }

      pPacket->setPending(false);
   }   
   
   // Send the results off to the destination.
   if (pPacket->getCallbackThreadIndex() != cThreadIndexInvalid)
   {
      gEventDispatcher.submitDataFunctor(pPacket->getCallbackThreadIndex(), pPacket->getCallback(), pPacket, false, pPacket->getSynchronousReply());
   }
   else
   {
      BDEBUG_ASSERT(pPacket->getReceiverHandle());
      gEventDispatcher.send(mEventReceiverHandle, pPacket->getReceiverHandle(), cEventClassAsyncFile, 0, 0, pPacket, pPacket->getSynchronousReply() ? BEventDispatcher::cSendSynchronousDispatch : 0);
   }
   
   return true;
}

//==============================================================================
// BAsyncFileManager::getNumOutstandingRequests
//==============================================================================
uint BAsyncFileManager::getNumOutstandingRequests(void) const
{
   MemoryBarrier();
   return mNumOutstandingRequests;
}

//==============================================================================
// BAsyncFileManager::getNumOutstandingIdleRequests
//==============================================================================
uint BAsyncFileManager::getNumOutstandingIdleRequests(void) const
{
   MemoryBarrier();
   return mNumOutstandingIdleRequests;
}

