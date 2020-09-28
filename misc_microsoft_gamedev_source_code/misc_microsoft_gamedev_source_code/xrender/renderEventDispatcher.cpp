7// renderEventDispatcher.h
#pragma once
#include "xrender.h"
#include "renderEventDispatcher.h"

BRenderEventDispatcher gRenderEventDispatcher;
__declspec(thread) BRenderThreadIndex BRenderEventDispatcher::mRenderThreadIndex;

BRenderEventDispatcher::BRenderEventDispatcher() :
   mInitialized(false),
   mMaxThreadClients(0)
{
   clear();
   
   mDelayedEvents.reserve(512);
   mHeartbeatReceivers.reserve(16);      
}

BRenderEventDispatcher::~BRenderEventDispatcher()
{
}

void BRenderEventDispatcher::clear(void)
{
   for (uint i = 0; i < cMaxThreads; i++)
   {  
      mThreadData[i].mLowestFreeHandleSlot = 0;
      mThreadData[i].mWakeupEvent.reset();
      mThreadData[i].mHandleSlots.clear();
      mThreadData[i].mEvents.clear();      
   }
   
   mMaxThreadClients = 0;
}

void BRenderEventDispatcher::init(uint maxClients)
{
   BDEBUG_ASSERT(maxClients > 0);
   
   if (mInitialized)
      return;
   
   mMaxThreadClients = maxClients;
   
   for (uint threadIndex = 0; threadIndex < cMaxThreads; threadIndex++)
   {
      BThreadData& threadData = mThreadData[threadIndex];
      
      threadData.mLowestFreeHandleSlot = 0;
      threadData.mWakeupEvent.reset();
                  
      threadData.mHandleSlots.resize(maxClients);
      for (uint slotIndex = 0; slotIndex < maxClients; slotIndex++)
         threadData.mHandleSlots[slotIndex].mCurUseCount = 1;
         
      threadData.mEvents.clear();   
   }
      
   clearHeartbeatReceivers();
   
   mHeartbeatTimer.set(33);
         
   mInitialized = true;
}

void BRenderEventDispatcher::deinit(void)
{
   if (!mInitialized)
      return;

   mHeartbeatTimer.cancel();
   
   clearHeartbeatReceivers();
   
   for (uint threadIndex = 0; threadIndex < cMaxThreads; threadIndex++)
   {
      // The outer lock on the thread prevents someone from removing a client while we are dispatching to them.
      BScopedCriticalSection lock(mThreadData[threadIndex].mMutex);
                  
      for ( ; ; )
      {
         BRenderEvent event;
         if (!mThreadData[threadIndex].mEvents.popFront(event))
            break;

         if (event.mpPayload)
            event.mpPayload->deleteThis(false);
      }
   }

   mInitialized = false;
}

void BRenderEventDispatcher::setThreadId(BRenderThreadIndex threadIndex, DWORD threadId)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));
         
   mRenderThreadIndex = threadIndex;
}

BWin32Event& BRenderEventDispatcher::getThreadWakeupEvent(BRenderThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   return mThreadData[threadIndex].mWakeupEvent;
}

BWin32WaitableTimer& BRenderEventDispatcher::getHeartbeatTimer(void) 
{
   BDEBUG_ASSERT(mInitialized);

   return mHeartbeatTimer;
}

void BRenderEventDispatcher::dispatchEvents(void)
{
   BDEBUG_ASSERT(mInitialized);
         
   const BRenderThreadIndex curThreadIndex = getThreadIndex();
   if (curThreadIndex < 0)
      return;

   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatch %i", curThreadIndex);

   BThreadData& threadData = mThreadData[curThreadIndex];
   
   // This lock prevents a different thread from removing a client during dispatching.
   // If client adds/removes where guaranteed to only happen on the same thread as the dispatch,
   // this lock wouldn't be necessary.
   BScopedCriticalSection lock(threadData.mMutex);
      
   for ( ; ; )
   {
      BRenderEvent event;
      if (!threadData.mEvents.popFront(event))
         break;

      const BRenderEventHandleStruct* pTo = (const BRenderEventHandleStruct*)&event.mToHandle;

      if (isValidHandle(*pTo))
      {
         // mFromHandle can't be validated here, because it may have come from a different thread and I don't want to lock its data too.
         // If the client sends a reply, we'll just validate the handle later.
         const bool resent = threadData.mHandleSlots[pTo->mSlotIndex].mpClient->receiveEvent(event, curThreadIndex);
            
         if ((!resent) && (event.mpPayload))
            event.mpPayload->deleteThis(true);
      }
      else
      {
         if (event.mpPayload)
            event.mpPayload->deleteThis(false);
      }
   }
   
   PIXEndNamedEvent();
}

void BRenderEventDispatcher::dispatchDelayedEvents(void)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatchDelayedEvents");

   BScopedCriticalSection lock(mDelayedEventsMutex);

   // This is a dumb algorithm. If there are too many delayed events this will take too long due to all the copy overhead.
   uint dstIndex = 0;
   for (uint index = 0; index < mDelayedEvents.size(); index++)
   {
      BDelayedEvent& delayedEvent = mDelayedEvents[index];

      if (delayedEvent.mFramesRemaining == 0)
      {
         const BRenderEventHandleStruct* pTo = (const BRenderEventHandleStruct*)&delayedEvent.mToHandle;
         BThreadData& destThreadData = mThreadData[pTo->mThreadIndex];

         destThreadData.mEvents.pushBack(delayedEvent);

         destThreadData.mWakeupEvent.set();
      }
      else
      {
         delayedEvent.mFramesRemaining--;

         if (index != dstIndex)
            mDelayedEvents[dstIndex] = delayedEvent;

         dstIndex++;
      }
   }

   mDelayedEvents.resize(dstIndex);

   PIXEndNamedEvent();
}

BHeartbeatHandle BRenderEventDispatcher::addHeartbeatReceiver(BRenderEventReceiverHandle handle, uint frameInterval, uint privateData)
{
   BScopedCriticalSection lock(mHeartbeatMutex);

   mHeartbeatReceivers.pushBack(BHeartbeatReceiver(handle, frameInterval, privateData));

   return static_cast<BHeartbeatHandle>(mHeartbeatReceivers.size() - 1);
}

bool BRenderEventDispatcher::removeHeartbeatReceiver(BHeartbeatHandle handle)
{
   BScopedCriticalSection lock(mHeartbeatMutex);

   if ((handle < 0) || (handle >= (int)mHeartbeatReceivers.size()))
   {
      trace("BRenderEventDispatcher::removeHeartbeatReceiver: Invalid handle");
      return false;
   }

   mHeartbeatReceivers.erase(handle);

   return true;   
}

void BRenderEventDispatcher::clearHeartbeatReceivers(void)
{
   BScopedCriticalSection lock(mHeartbeatMutex);

   mHeartbeatReceivers.clear();
}

void BRenderEventDispatcher::tickHeartbeatReceivers(void)
{
   BScopedCriticalSection lock(mHeartbeatMutex);

   for (uint index = 0; index < mHeartbeatReceivers.size(); index++)
   {
      BHeartbeatReceiver& receiver = mHeartbeatReceivers[index];

      if (receiver.mFramesRemaining <= 1)
      {
         gRenderEventDispatcher.send(cInvalidRenderEventReceiverHandle, receiver.mHandle, cRenderEventClassHeartbeat, receiver.mPrivateData, 0, NULL);
         receiver.mFramesRemaining  = receiver.mInterval;
      }
      else
      {
         receiver.mFramesRemaining--;
      }
   }
}

void BRenderEventDispatcher::dispatchHeartbeat(void)
{
   dispatchDelayedEvents();

   tickHeartbeatReceivers();
}

int BRenderEventDispatcher::wait(uint numHandles, const HANDLE* pHandles, DWORD maxTimeToWait)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatchWait");
   
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((numHandles > 0) || (maxTimeToWait != INFINITE));

   const BRenderThreadIndex curThreadIndex = getThreadIndex();
   BDEBUG_ASSERT(curThreadIndex >= 0);
      
   enum { cMaxHandles = 62 };
   BDEBUG_ASSERT(numHandles < cMaxHandles);
   
   const uint totalHandles = 2 + numHandles;
   HANDLE handles[2 + cMaxHandles];
   
   if (numHandles)
      Utils::FastMemCpy(handles, pHandles, numHandles * sizeof(HANDLE));
      
   handles[numHandles] = mThreadData[curThreadIndex].mWakeupEvent.getHandle();
   handles[numHandles + 1] = mHeartbeatTimer.getHandle();
   
   const DWORD startTime = GetTickCount();
   
   for ( ; ; )
   {
      DWORD timeLeft = INFINITE;            
      
      if (maxTimeToWait != INFINITE)
      {
         const DWORD timeSoFar = GetTickCount() - startTime;
         if (timeSoFar >= maxTimeToWait)
            break;
         timeLeft = maxTimeToWait - timeSoFar;
      }
      
      DWORD status = WaitForMultipleObjects(totalHandles, handles, FALSE, timeLeft);
            
      if (status == WAIT_TIMEOUT)
         break;
      else if ((status >= WAIT_OBJECT_0) && (status <= WAIT_OBJECT_0 + totalHandles - 1))   
      {
         if (status == WAIT_OBJECT_0 + numHandles)
            dispatchEvents();
         else if (status == WAIT_OBJECT_0 + numHandles + 1)
            dispatchHeartbeat();
         else
         {
            PIXEndNamedEvent();
            return status - WAIT_OBJECT_0;
         }
      }
      else
      {
         BFAIL("BRenderEventDispatcher::wait: Wait failed");
      }
   }
   
   PIXEndNamedEvent();
   
   return -1;
}

BRenderEventReceiverHandle BRenderEventDispatcher::addClient(BRenderEventReceiverInterface* pClient, BRenderThreadIndex clientThreadIndex)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatcherAddClient");
   
   if (cInvalidIndex == clientThreadIndex)
      clientThreadIndex = getThreadIndex();
   
   BDEBUG_ASSERT(mInitialized && (clientThreadIndex >= 0) && (clientThreadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[clientThreadIndex];
         
   BScopedCriticalSection lock(threadData.mMutex);

   BRenderEventHandleStruct handle = allocHandle(clientThreadIndex);
   
   threadData.mHandleSlots[handle.mSlotIndex].mpClient = pClient;
   
   PIXEndNamedEvent();
   
   return *reinterpret_cast<BRenderEventReceiverHandle*>(&handle);
}

void BRenderEventDispatcher::removeClient(BRenderEventReceiverHandle h)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatcherRemoveClient");
   
   BDEBUG_ASSERT(mInitialized);

   const BRenderEventHandleStruct& handle = *reinterpret_cast<const BRenderEventHandleStruct*>(&h);
   BDEBUG_ASSERT((handle.mSlotIndex < mMaxThreadClients) && (handle.mUseCount > 0) && (handle.mThreadIndex < cMaxThreads));
      
   BThreadData& threadData = mThreadData[handle.mThreadIndex];
   
   BScopedCriticalSection lock(threadData.mMutex);   
   
   BDEBUG_ASSERT(isValidHandle(handle));
   
   freeHandle(handle);      
   
   PIXEndNamedEvent();
}

bool BRenderEventDispatcher::send(const BRenderEvent& event)
{
   PIXSetMarker(D3DCOLOR_ARGB(255,255,255,255), "RenderEventDispatcherSend");
   
   BDEBUG_ASSERT(mInitialized);
   
   const BRenderEventHandleStruct& fromHandle = *reinterpret_cast<const BRenderEventHandleStruct*>(&event.mFromHandle);
   const BRenderEventHandleStruct& toHandle = *reinterpret_cast<const BRenderEventHandleStruct*>(&event.mToHandle);
      
   BDEBUG_ASSERT((event.mFromHandle == cInvalidRenderEventReceiverHandle) || ((fromHandle.mSlotIndex < mMaxThreadClients) && (fromHandle.mUseCount > 0) && (fromHandle.mThreadIndex < cMaxThreads)));
   BDEBUG_ASSERT((toHandle.mSlotIndex < mMaxThreadClients) && (toHandle.mUseCount > 0) && (toHandle.mThreadIndex < cMaxThreads));
   
   // We can't immediately send the event if the receiver is on the same thread, because another thread
   // could remove the client while we are trying to send the event. Perhaps we can forbid this as an optimization.
   
   BThreadData& destThreadData = mThreadData[toHandle.mThreadIndex];
      
   // There's no need to lock all of the thread's data. We're merely inserting into it's event queue, 
   // which is an atomic operation.
   // The handle will be checked for validity during the dispatch.
   if (!destThreadData.mEvents.pushBack(event))
      return false;
   
   destThreadData.mWakeupEvent.set();
      
   return true;
}

bool BRenderEventDispatcher::delayedSend(uint framesToDelay, const BRenderEvent& event)
{
   BDEBUG_ASSERT(mInitialized);

   const BRenderEventHandleStruct& fromHandle = *reinterpret_cast<const BRenderEventHandleStruct*>(&event.mFromHandle);
   const BRenderEventHandleStruct& toHandle = *reinterpret_cast<const BRenderEventHandleStruct*>(&event.mToHandle);

   BDEBUG_ASSERT((event.mFromHandle == cInvalidRenderEventReceiverHandle) || ((fromHandle.mSlotIndex < mMaxThreadClients) && (fromHandle.mUseCount > 0) && (fromHandle.mThreadIndex < cMaxThreads)));
   BDEBUG_ASSERT((toHandle.mSlotIndex < mMaxThreadClients) && (toHandle.mUseCount > 0) && (toHandle.mThreadIndex < cMaxThreads));
   
   BScopedCriticalSection lock(mDelayedEventsMutex);
   
   if (mDelayedEvents.size() >= 4096)
   {
      trace("BRenderEventDispatcher::delayedSend: Delayed event buffer is too big!");
      return false;
   }
   
   mDelayedEvents.pushBack(BDelayedEvent(framesToDelay, event));
      
   return true;
}

bool BRenderEventDispatcher::isValidHandle(BRenderEventHandleStruct handle) const
{
   BDEBUG_ASSERT((handle.mUseCount > 0) && (handle.mSlotIndex < mMaxThreadClients) && (handle.mThreadIndex < cMaxThreads));
   return mThreadData[handle.mThreadIndex].mHandleSlots[handle.mSlotIndex].mCurUseCount == handle.mUseCount;
}

BRenderEventHandleStruct BRenderEventDispatcher::allocHandle(BRenderThreadIndex threadIndex) 
{
   BThreadData& threadData = mThreadData[threadIndex];

   uint index = 0;   
   for ( ; ; )
   {
      if (threadData.mLowestFreeHandleSlot == mMaxThreadClients)
      {
         BFAIL("Out of handles");
      }

      index = threadData.mLowestFreeHandleSlot;
      threadData.mLowestFreeHandleSlot++;

      if (!threadData.mHandleSlots[index].mpClient)
         break;
   }

   return BRenderEventHandleStruct(index, threadData.mHandleSlots[index].mCurUseCount, threadIndex);
}

void BRenderEventDispatcher::freeHandle(BRenderEventHandleStruct handle)
{
   const uint slotIndex = handle.mSlotIndex;
   const BRenderThreadIndex threadIndex = handle.mThreadIndex;
   const uint useCount = handle.mUseCount;
   useCount;
         
   BDEBUG_ASSERT((useCount > 0) && (slotIndex < mMaxThreadClients) && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[threadIndex];
   
   threadData.mHandleSlots[slotIndex].mCurUseCount++;      
   threadData.mHandleSlots[slotIndex].mpClient = NULL;
   threadData.mLowestFreeHandleSlot = Math::Min<uint>(threadData.mLowestFreeHandleSlot, slotIndex);
}

int BRenderEventDispatcher::getThreadIndexFromHandle(BRenderEventReceiverHandle h)
{
   BDEBUG_ASSERT(mInitialized);

   const BRenderEventHandleStruct& handle = *reinterpret_cast<const BRenderEventHandleStruct*>(&h);   

   BDEBUG_ASSERT((handle.mSlotIndex < mMaxThreadClients) && (handle.mUseCount > 0) && (handle.mThreadIndex >= 0) && (handle.mThreadIndex < cMaxThreads));   
   
   return handle.mThreadIndex;
}

