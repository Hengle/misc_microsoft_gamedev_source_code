// File: asyncFileLoader.cpp
#include "xsystem.h"
#include "asyncFileLoader.h"

//==============================================================================
// BAsyncFileLoader::BAsyncFileLoader
//==============================================================================
BAsyncFileLoader::BAsyncFileLoader(BThreadIndex clientThreadIndex) :
   BEventReceiver(),
   mRequestID(0),
   mpRequestPacket(NULL),
   mOwnerThread(clientThreadIndex),
   mpEventCallback(NULL),
   mpEventCallbackData(NULL)
{
   if (cThreadIndexInvalid == mOwnerThread)
      mOwnerThread = gEventDispatcher.getThreadIndex();
      
   eventReceiverInit(clientThreadIndex, true);
}

//==============================================================================
// BAsyncFileLoader::~BAsyncFileLoader
//==============================================================================
BAsyncFileLoader::~BAsyncFileLoader()
{
   if (mOwnerThread != gEventDispatcher.getThreadIndex())
   {
      // You shouldn't do this!
      eventReceiverDeinit();
      return;
   }
   
   // This is much faster than calling eventReceiverDeinit(false), because it won't 
   // involve a dispatch.
   clear();
   
   eventReceiverDeinit(true);
}

//==============================================================================
// BAsyncFileLoader::clear
//==============================================================================
void BAsyncFileLoader::clear(bool deinit)
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (mpRequestPacket)
   {
      BDEBUG_ASSERT(0 == mRequestID);
      gAsyncFileManager.deleteRequestPacket(mpRequestPacket);      
      mpRequestPacket = NULL;
   }

   mRequestID = 0;

   if (deinit)
      eventReceiverDeinit(true);
}

//==============================================================================
// BAsyncFileLoader::load
//==============================================================================
bool BAsyncFileLoader::load(long dirID, const char* pFilename, bool highPriorityRequest, bool discardOnClose)
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   // The user shouldn't submit more than one active requests to this helper, but if they do let's do something reasonable. 
   clear();
   
   BAsyncFileManager::BRequestPacket* pRequestPacket = gAsyncFileManager.newRequestPacket();
   pRequestPacket->setReceiverHandle(mEventHandle);
   pRequestPacket->setDirID(dirID);
   pRequestPacket->setFilename(pFilename);
   pRequestPacket->setPriority(highPriorityRequest ? 1 : -1);
   // DO NOT change this to synchronous, some wait methods below don't pump messages.
   pRequestPacket->setSynchronousReply(false);
   pRequestPacket->setDiscardOnClose(discardOnClose);

   mRequestID = gAsyncFileManager.submitRequest(pRequestPacket);
   BDEBUG_ASSERT(0 != mRequestID);

   return true;
}

//==============================================================================
// BAsyncFileLoader::getPending
//==============================================================================
bool BAsyncFileLoader::getPending(void) const
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   return 0 != mRequestID;
}

//==============================================================================
// BAsyncFileLoader::getReady
//==============================================================================
bool BAsyncFileLoader::getReady(void) const 
{ 
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   return NULL != mpRequestPacket; 
}

//==============================================================================
// BAsyncFileLoader::waitUntilReady
//==============================================================================
bool BAsyncFileLoader::waitUntilReady(DWORD timeout)
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (getReady())
      return true;
      
   if (mRequestID == 0)
      return false;
   
   const DWORD startTime = GetTickCount();
   for ( ; ; )
   {
      if (getReady())
         break;
      
      if (timeout != INFINITE)
      {
         if ((GetTickCount() - startTime) >= timeout)
            return false;
      }
      
      gEventDispatcher.sleep(4);
   }
   
   return true;
}

//==============================================================================
// BAsyncFileLoader::getSucceeded
//==============================================================================
bool BAsyncFileLoader::getSucceeded(void) const
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (NULL == mpRequestPacket)
      return false;
      
   return mpRequestPacket->getSucceeded();
}

//==============================================================================
// BAsyncFileLoader::getFailed
//==============================================================================
bool BAsyncFileLoader::getFailed(void) const
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());
   
   if (NULL == mpRequestPacket)
      return false;
      
   return !mpRequestPacket->getSucceeded();
}

//==============================================================================
// BAsyncFileLoader::getData
//==============================================================================
void* BAsyncFileLoader::getData(void) const 
{ 
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (!getSucceeded())
      return NULL;
      
   return mpRequestPacket->getData();
}

//==============================================================================
// BAsyncFileLoader::getDataLen
//==============================================================================
int BAsyncFileLoader::getDataLen(void) const 
{ 
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (!getSucceeded())
      return -1;
      
   return mpRequestPacket->getDataLen();
}

//==============================================================================
// BAsyncFileLoader::assumeDataOwnership
//==============================================================================
bool BAsyncFileLoader::assumeDataOwnership(void)
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   if (!getSucceeded())
      return false;

   mpRequestPacket->setDeleteData(false);
   return true;
}

//==============================================================================
// BAsyncFileLoader::setOwnerThread
//==============================================================================
void BAsyncFileLoader::setOwnerThread(BThreadIndex ownerThread) 
{ 
   BDEBUG_ASSERT(cThreadIndexInvalid != mOwnerThread);
   
   if (mOwnerThread == ownerThread)
      return;

   eventReceiverDeinit();
   
   clear();
   
   mOwnerThread = ownerThread;
            
   eventReceiverInit(ownerThread, true);
}

//==============================================================================
// BAsyncFileLoader::receiveEvent
//==============================================================================
bool BAsyncFileLoader::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mOwnerThread == gEventDispatcher.getThreadIndex());

   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
      {
         BAsyncFileManager::BRequestPacket* pPacket = (BAsyncFileManager::BRequestPacket*)event.mpPayload;
         
         // Only pay attention to the latest requests -- drop all others.
         if (pPacket->getRequestIndex() == mRequestID)
         {
            BDEBUG_ASSERT(!mpRequestPacket);
            mpRequestPacket = pPacket;
            mRequestID = 0;
            
            if (mpEventCallback)
               mpEventCallback(this, mpEventCallbackData);
                        
            return true;
         }

         break;
      }
      case cEventClassThreadIsTerminating:
      case cEventClassClientRemove:
      {
         clear();
         
         break;         
      }
   }

   return false;
}


