//============================================================================
//
// File: eventDispatcher.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "xcore.h"
#include "eventDispatcher.h"
#include "threading\setThreadName.h"
#include "containers\staticArray.h"
#include "..\xsystem\timelineprofilersample.h"

#ifndef BUILD_FINAL
// jce [11/18/2008] -- this #ifdef XBOX isn't strictly necessary but it creates a dependency on xsystem
// which apparently isn't compiling for win32 at the moment.
#ifdef XBOX
#define EVENT_DISPATCHER_LOGGING
#include "..\xsystem\bfileStream.h"
#include "stream\bufferStream.h"
#endif
#endif

//============================================================================
// Globals
//============================================================================
__declspec(thread) BThreadIndex BEventDispatcher::mThreadIndex = cThreadIndexInvalid;

//============================================================================
// BEventDispatcher::BEventDispatcher
//============================================================================
BEventDispatcher::BEventDispatcher()
{
   clear();

#ifdef XBOX   
   // I'm assuming gEventDispatcher is in the lib segment, so it's constructed before everybody else!
   init();
   
   registerThread(cThreadIndexSim, GetCurrentThreadId(), 8192, 512);
#endif   
}

//============================================================================
// BEventDispatcher::BThreadData::clear
//============================================================================
void BEventDispatcher::BThreadData::clear(void)
{
   mDispatcherClientHandle = cInvalidEventReceiverHandle;

   mLowestFreeHandleSlot = 0;
   mHandleSlots.clear();

   mWakeupEvent.reset();
   mEvents.clear();      

   mAsyncDispatchCount = 0;
   mSyncDispatchCount = 0;
   mMaxSyncDispatchCount = 0;
   mTotalAsyncEvents = 0;
   mTotalSyncEvents = 0; 

   //mNumAsyncEvents = 0;
   //mNextAsyncEventIndex = 0;
   
   mDeferredEvents.clear();
   
   Utils::ClearObj(mHandleEvents);

   mNumHandleEvents = 0;
   mHandleEventRover = 0;
   
   const uint cSpinCount = 2000;
   mHandleMutex.setSpinCount(cSpinCount);
   mEvents.setSpinCount(cSpinCount);
}

//============================================================================
// BEventDispatcher::clear
//============================================================================
void BEventDispatcher::clear(void)
{
   mInitialized = false;
   
   mMaxThreadClients = 0;
   
   for (uint i = 0; i < cMaxThreads; i++)
      mThreadData[i].clear();
         
   Utils::ClearObj(mThreadID);
}

//============================================================================
// BEventDispatcher::~BEventDispatcher
//============================================================================
BEventDispatcher::~BEventDispatcher()
{
   // I'm assuming gEventDispatcher is in the lib segment, so it's destructed before everybody else!
   deinit();
}

//============================================================================
// BEventDispatcher::init
//============================================================================
void BEventDispatcher::init(uint maxClients)
{
   BDEBUG_ASSERT((maxClients > 0) && (maxClients <= BEventHandleStruct::cMaxSlots));
   BCOMPILETIMEASSERT(cMaxThreads <= BEventHandleStruct::cMaxThreads);
   
   if (mInitialized)
      return;
         
   mMaxThreadClients = maxClients;
   
   for (uint threadIndex = 0; threadIndex < cMaxThreads; threadIndex++)
   {
      BThreadData& threadData = mThreadData[threadIndex];
      
      threadData.clear();
                                    
      threadData.mHandleSlots.resize(maxClients);
      for (uint slotIndex = 0; slotIndex < maxClients; slotIndex++)
         threadData.mHandleSlots[slotIndex].mCurUseCount = 1;
        
      threadData.mWakeupEvent.reset();      
      
      threadData.mEvents.resize(cDefaultEventQueueSize);
      
      threadData.mDeferredEvents.resize(64);
   }
            
   mInitialized = true;
}

//============================================================================
// BEventDispatcher::deinit
//============================================================================
void BEventDispatcher::deinit(void)
{
   if (!mInitialized)
      return;
      
   // FIXME
   // Could send down cEventClassSetEventAndWait events to each thread, then wait for reception. At this point, it should be safe to pull the rug
   // out from each thread.
      
#if 0
   for (uint threadIndex = 0; threadIndex < cMaxThreads; threadIndex++)
   {
      // The outer lock on the thread prevents someone from removing a client while we are dispatching to them.
      BScopedCriticalSection lock(mThreadData[threadIndex].mMutex);
                  
      for ( ; ; )
      {
         BEvent event;
         if (!mThreadData[threadIndex].mEvents.popFront(event))
            break;

         if (event.mpPayload)
            event.mpPayload->deleteThis(false);
      }
   }
#endif   
      
   mInitialized = false;
}

//============================================================================
// BEventDispatcher::registerThread
//============================================================================
void BEventDispatcher::registerThread(BThreadIndex threadIndex, DWORD threadId, uint eventQueueSize, uint syncEventQueueSize)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[threadIndex];
   
   if (threadId == 0)
   {
      // Notify all clients that the thread is exiting.
      // New clients could be added in parallel, what to do about that?
      uint numClientsFound = 0;
      for (uint i = 0; i < threadData.mHandleSlots.getSize(); i++)
      {
         threadData.mHandleMutex.lock();
         
         const bool valid = threadData.mHandleSlots[i].mpClient != NULL;
         
         BEventHandleStruct handle(i, threadData.mHandleSlots[i].mCurUseCount, threadIndex, (threadData.mHandleSlots[i].mFlags & BHandleSlot::cHFPermitAsyncImmediateRemoval) != 0);
         
         threadData.mHandleMutex.unlock();
         
         if (valid)
         {
            // Handle could have become invalid here -- dispatcher should toss invalid handles.
            send(cInvalidEventReceiverHandle, *reinterpret_cast<BEventReceiverHandle*>(&handle), cEventClassThreadIsTerminating);
            numClientsFound++;
         }
      }
      
      waitUntilThreadQueueEmpty(threadIndex);
      
//      trace("BEventDispatcher::registerThread: Alerted %u client(s) of thread %u's pending demise", numClientsFound, threadIndex);
   }
   
   if (threadData.mDispatcherClientHandle != cInvalidEventReceiverHandle)
   {
      removeClientDeferred(mThreadData[threadIndex].mDispatcherClientHandle, false);
      waitUntilThreadQueueEmpty(threadIndex);
   }
      
   // mThreadIndex is in TLS.
   mThreadIndex = threadId ? threadIndex : cInvalidIndex;
   
   mThreadID[threadIndex] = threadId;
         
   if (threadId)   
   {
      threadData.mEvents.resize(eventQueueSize);
      threadData.mSynchronousEvents.resize(syncEventQueueSize);
            
      threadData.mDispatcherClientHandle = addClient(this, threadIndex, false);
   }
}

//============================================================================
// BEventDispatcher::getThreadId
//============================================================================
DWORD BEventDispatcher::getThreadId(BThreadIndex threadIndex) const
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   return mThreadID[threadIndex];
}

//============================================================================
// BEventDispatcher::getThreadName
//============================================================================
const char* BEventDispatcher::getThreadName(eThreadIndex threadIndex)
{  
   BCOMPILETIMEASSERT(cThreadIndexMax == 6);
   
   switch (threadIndex)
   {
      case cThreadIndexSim:            return "Sim";
      case cThreadIndexSimHelper:      return "SimHelper";
      case cThreadIndexRender:         return "Render";
      case cThreadIndexRenderHelper:   return "RenderHelper";
      case cThreadIndexIO:             return "IO";
      case cThreadIndexMisc:           return "Misc";
   }
   return "?";
}

//============================================================================
// BEventDispatcher::getThreadWakeupEvent
//============================================================================
BThreadIndex BEventDispatcher::getThreadIndexByID(DWORD threadID)
{
   BDEBUG_ASSERT(mInitialized);
      
   for (uint i = 0; i < cMaxThreads; i++)
      if (mThreadID[i] == threadID)
         return static_cast<BThreadIndex>(i);
   
   return cThreadIndexInvalid;
}

//============================================================================
// BEventDispatcher::getThreadWakeupEvent
//============================================================================
BWin32Event& BEventDispatcher::getThreadWakeupEvent(BThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   return mThreadData[threadIndex].mWakeupEvent;
}

//============================================================================
// BEventDispatcher::dispatchEvent
//============================================================================
void BEventDispatcher::dispatchEvent(const BEvent& event)
{
   const BThreadIndex curThreadIndex = getThreadIndex();
   BThreadData& threadData = mThreadData[curThreadIndex];
   const BEventHandleStruct& to = *(const BEventHandleStruct*)&event.mToHandle;
   
   bool delivered = false;
   bool forwarded = false;   
   
   if (to.mPermitAsyncRemoval)
      threadData.mHandleSlots[to.mSlotIndex].mMutex.lock();
   
   if (isValidHandle(to))
   {
      delivered = true;

      BDEBUG_ASSERT(threadData.mHandleSlots[to.mSlotIndex].mpClient);

      // mFromHandle can't be validated here, because it may have come from a different thread and I don't want to lock its data too.
      // If the client sends a reply, we'll just validate the handle later.
      forwarded = threadData.mHandleSlots[to.mSlotIndex].mpClient->receiveEvent(event, curThreadIndex);
   }

   if (to.mPermitAsyncRemoval)
      threadData.mHandleSlots[to.mSlotIndex].mMutex.unlock();

   if (cEventClassClientRemove == event.mEventClass)
   {
      if (!delivered)
      {
//         trace("BEventDispatcher::dispatchEvents: A client has requested a deferred remove, but this event could not be delivered because the client no longer exists.");
      }

      freeHandle(*(const BEventHandleStruct*)&event.mToHandle);
   }
   
   if ((event.mEventFlags & BEvent::cEventFlagRequestDeliveryReceipt) && (event.mFromHandle != cInvalidEventReceiverHandle))
   {
      BEvent replyEvent(event);
      if (forwarded)
         replyEvent.mpPayload = NULL;
      std::swap(replyEvent.mToHandle, replyEvent.mFromHandle);
      replyEvent.mEventFlags &= ~BEvent::cEventFlagRequestDeliveryReceipt;
      replyEvent.mEventFlags &= ~BEvent::cEventFlagSetDeliveredFlag;
      replyEvent.mEventFlags |= (delivered ? BEvent::cEventFlagDeliverySucceeded : BEvent::cEventFlagDeliveryFailed);
      replyEvent.mEventFlags |= BEvent::cEventFlagDeliveryReceiptReply;
      send(replyEvent);
   }
   else
   {
      if ((!forwarded) && (event.mpPayload))
      {
         event.mpPayload->deleteThis(delivered);
      }
   }         
}            

//============================================================================
// BEventDispatcher::dispatchSynchronousEvents
//============================================================================
void BEventDispatcher::dispatchSynchronousEvents(BEventReceiverHandle receiverHandle)
{
   //SCOPEDSAMPLE(DispatchSyncEvents);
   
   BDEBUG_ASSERT(mInitialized);
               
   const BThreadIndex curThreadIndex = getThreadIndex();
   if (curThreadIndex < 0)
      return;
   
   BThreadData& threadData = mThreadData[curThreadIndex];
      
   // Disallow reentrant dispatch unless they are targeting a particular client.
   BDEBUG_ASSERT((threadData.mSyncDispatchCount == 0) || (receiverHandle != 0) );   
   
   threadData.mSyncDispatchCount++;
   threadData.mMaxSyncDispatchCount = Math::Max(threadData.mSyncDispatchCount, threadData.mMaxSyncDispatchCount);
      
   if (receiverHandle != cInvalidEventReceiverHandle)
   {
      BStaticArray<BEvent, 64> eventsToDispatch;
      
      do
      {
         eventsToDispatch.resize(0);
         
         uint index = 0;
         while (index < threadData.mSynchronousEvents.getSize())
         {
            BEvent& event = threadData.mSynchronousEvents.peekFront(index);
            
            if ( (event.mToHandle == receiverHandle)  && ((event.mEventFlags & BEvent::cEventFlagAlreadyDispatched) == 0) )
            {
               event.mEventFlags |= BEvent::cEventFlagAlreadyDispatched;
               
               eventsToDispatch.pushBack(event);
            }
            
            index++;
         }
         
         for (uint i = 0; i < eventsToDispatch.size(); i++)
            dispatchEvent(eventsToDispatch[i]);
            
      } while (eventsToDispatch.size());        
   }
   else
   {
      for ( ; ; )
      {
         BEvent event;
         if (!threadData.mSynchronousEvents.popFront(event))
            break;
            
         threadData.mTotalSyncEvents++;
         
         if ((event.mEventFlags & BEvent::cEventFlagAlreadyDispatched) == 0)                              
            dispatchEvent(event);
      }      
   }      
   
   threadData.mSyncDispatchCount--;
}

//============================================================================
// BEventDispatcher::forceSynchronousDispatch
//============================================================================
void BEventDispatcher::forceSynchronousDispatch(BThreadIndex threadIndex)
{
   const BEventReceiverHandle eventHandle = getDispatcherClientHandle(threadIndex);
   send(eventHandle, eventHandle, cEventClassDispatchSynchronousEvents);
}

//============================================================================
// BEventDispatcher::dispatchEvents
//============================================================================
void BEventDispatcher::dispatchEvents(void)
{
   BDEBUG_ASSERT(mInitialized);
         
   const BThreadIndex curThreadIndex = getThreadIndex();
   if (curThreadIndex < 0)
      return;

   BThreadData& threadData = mThreadData[curThreadIndex];
                  
   BEventQueue& eventQueue = threadData.mEvents;
            
   threadData.mAsyncDispatchCount++;
   // rg [7/22/07] - Reentrant dispatching is not supported!
   BASSERT(1 == threadData.mAsyncDispatchCount);
         
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "DispatchEvents");
   
   enum { cMaxEvents = 64 };
   BEvent curAsyncEvents[cMaxEvents];              
   
   for ( ; ; )
   {
      const uint numAsyncEvents = eventQueue.popFront(cMaxEvents, curAsyncEvents);
      if (!numAsyncEvents)
         break;
                     
      threadData.mTotalAsyncEvents += numAsyncEvents;
                     
      for (uint asyncEventIndex = 0; asyncEventIndex < numAsyncEvents; asyncEventIndex++)
      {
         const BEvent& event = curAsyncEvents[asyncEventIndex];
                        
         if (cEventClassSetEventAndWait == event.mEventClass)
         {
            PIXSetMarkerNoColor("DispatchEventsSetEventAndWait");
            
            HANDLE setHandle = reinterpret_cast<HANDLE>(event.mPrivateData);
            HANDLE waitHandle = reinterpret_cast<HANDLE>(event.mPrivateData2);
                        
            if (setHandle != INVALID_HANDLE_VALUE)
            {
               SetEvent(setHandle);
            }

            if (waitHandle != INVALID_HANDLE_VALUE)
            {
               WaitForSingleObject(waitHandle, INFINITE);
               
               if (setHandle != INVALID_HANDLE_VALUE)
               {
                  SetEvent(setHandle);
               }
            }
         }               
         else
         {   
            bool delivered = false;

            const BEventHandleStruct& to = *(const BEventHandleStruct*)&event.mToHandle;            
            to;
            BDEBUG_ASSERT((to.mUseCount > 0) && (to.mSlotIndex < mMaxThreadClients) && (to.mThreadIndex < cMaxThreads));
                        
            if ((event.mEventFlags & BEvent::cEventFlagSynchronous) && (threadData.mSynchronousEvents.getMaxSize()))
            {
               if (!threadData.mSynchronousEvents.pushBack(event))
               {
                  threadData.mSynchronousEvents.resize(Math::Min<uint>(2048, threadData.mSynchronousEvents.getMaxSize() * 2));
                  
                  if (!threadData.mSynchronousEvents.pushBack(event))
                  {
                     BFAIL("BEventDispatcher::dispatchEvents: Sync event queue overflowed!");
                  }
               }
            }
            else
            {
               dispatchEvent(event);
            }

            if (event.mEventFlags & BEvent::cEventFlagSetDeliveredFlag)
            {
               BDEBUG_ASSERT(event.mPrivateData2);
               Sync::InterlockedExchangeExport(reinterpret_cast<LONG*>(event.mPrivateData2), delivered);
            }
            else if (event.mEventFlags & BEvent::cEventFlagSetWin32Event)
            {
               SetEvent(reinterpret_cast<HANDLE>(event.mPrivateData2));
            }               
         }
      } // while
   } // for
   
   threadData.mAsyncDispatchCount--;
   
   PIXEndNamedEvent();
}

//============================================================================
// BEventDispatcher::flushDeferredEvents
//============================================================================
void BEventDispatcher::flushDeferredEvents(void)
{
   const BThreadIndex curThreadIndex = getThreadIndex();
   BDEBUG_ASSERT(curThreadIndex >= 0);

   BThreadData& threadData = mThreadData[curThreadIndex];

   while (!threadData.mDeferredEvents.getEmpty())
   {
      const BEvent& event = threadData.mDeferredEvents.peekFront(0);
      
      const BEventHandleStruct& toHandle = *reinterpret_cast<const BEventHandleStruct*>(&event.mToHandle);
      const uint toThreadIndex = toHandle.mThreadIndex;      
      
      BDEBUG_ASSERT((toThreadIndex >= 0) && (toThreadIndex < cMaxThreads));
      
      BThreadData& destThreadData = mThreadData[toThreadIndex];
      
      //trace("flushDeferredEvents: From: 0x%I64X To: 0x%I64X Payload: %X, First Payload DWORD: 0x%X", event.mFromHandle, event.mToHandle, event.mpPayload, event.mpPayload ? *(DWORD*)event.mpPayload : 0);
      
      const bool sent = destThreadData.mEvents.pushBack(event);
      
      if (!sent)
         break;
         
      destThreadData.mWakeupEvent.set();
      
      threadData.mDeferredEvents.popFront();
   }
}

//============================================================================
// BEventDispatcher::processHandleEvent
//============================================================================
void BEventDispatcher::processHandleEvent(uint index)
{
   const BThreadIndex curThreadIndex = getThreadIndex();
   BDEBUG_ASSERT(curThreadIndex >= 0);
   
//-- FIXING PREFIX BUG ID 749
   const BThreadData& threadData = mThreadData[curThreadIndex];
//--
   
   if (index >= threadData.mNumHandleEvents)
   {
      BDEBUG_ASSERT(0);
      return;
   }
   
   const BHandleEvent& handleEvent = threadData.mHandleEvents[index];
   
   if (handleEvent.mpCallback)
      (*handleEvent.mpCallback)(handleEvent.mEvent.mPrivateData);
   else
      pushEvent(getHandleThreadIndex(handleEvent.mEvent.mToHandle), handleEvent.mEvent, handleEvent.mDeferIfQueueFull);
}

//============================================================================
// BEventDispatcher::wait
//============================================================================
int BEventDispatcher::wait(uint numHandles, const HANDLE* pHandles, DWORD maxTimeToWait, bool dispatchDuringWait)
{
   if (!mInitialized)
   {
      BDEBUG_ASSERT(false);
      return -1;
   }
            
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "EventDispatchWait");

   BDEBUG_ASSERT((mInitialized) && ((numHandles > 0) || (maxTimeToWait != INFINITE)) );
      
   const BThreadIndex curThreadIndex = getThreadIndex();
   BDEBUG_ASSERT(curThreadIndex >= 0);
               
   BThreadData& threadData = mThreadData[curThreadIndex];
                           
   int returnStatus = -1;
   
   flushDeferredEvents();
   
   enum { cMaxHandles = 64 };
   HANDLE handles[cMaxHandles];
   uchar handleEventIndex[BThreadData::cMaxHandleEvents];

   if (numHandles)
      Utils::FastMemCpy(handles, pHandles, numHandles * sizeof(HANDLE));

   uint totalHandles = numHandles;   
   
   int asyncHandleIndex = -1;
   if (dispatchDuringWait)
   {
      asyncHandleIndex = totalHandles;
      handles[totalHandles] = threadData.mWakeupEvent.getHandle();
      totalHandles++;
   }
   
   const uint numBaseHandles = totalHandles;
                 
   if (!dispatchDuringWait)
   {
      if (!totalHandles)
      {
         Sleep(maxTimeToWait);
         
         flushDeferredEvents();
      }
      else
      {
         const DWORD startTime = GetTickCount();
         
         for ( ; ; )
         {
            totalHandles = numBaseHandles;  
            
            int firstHandleEventIndex = -1;
            if (threadData.mNumHandleEvents)
            {
               firstHandleEventIndex = numBaseHandles;

               uint srcIndex = threadData.mHandleEventRover;

               BDEBUG_ASSERT(threadData.mNumHandleEvents <= BThreadData::cMaxHandleEvents);
               for (uint i = 0; i < threadData.mNumHandleEvents; i++)
               {
                  while (srcIndex >= threadData.mNumHandleEvents)
                     srcIndex -= threadData.mNumHandleEvents;
                  
                  BDEBUG_ASSERT(srcIndex < threadData.mNumHandleEvents);
                  handles[totalHandles] = threadData.mHandleEvents[srcIndex].mHandle;
                  handleEventIndex[i] = static_cast<uchar>(srcIndex);
                  
                  totalHandles++;
                  srcIndex++;
               }

               threadData.mHandleEventRover++;
               if (threadData.mHandleEventRover >= threadData.mNumHandleEvents)
                  threadData.mHandleEventRover = 0;
            }
            
            BDEBUG_ASSERT(totalHandles <= cMaxHandles);
            
            DWORD timeToWait = INFINITE;            

            if (maxTimeToWait == 0)
               timeToWait = 0;
            else if (maxTimeToWait != INFINITE)
            {
               const DWORD timeSoFar = GetTickCount() - startTime;
               if (timeSoFar >= maxTimeToWait)
                  break;
               timeToWait = maxTimeToWait - timeSoFar;
            }

            timeToWait = Math::Min<DWORD>(timeToWait, 60);      
            
            const DWORD status = WaitForMultipleObjects(totalHandles, handles, FALSE, timeToWait);
            
            flushDeferredEvents();
                        
            if (status != WAIT_TIMEOUT)
            {
               if ( (status == WAIT_FAILED) || ((status >= WAIT_ABANDONED_0) && (status <= (WAIT_ABANDONED_0 + totalHandles - 1))) )
               {
                  BFATAL_FAIL("BEventDispatcher::wait: Wait failed");
               }
               
               const int handleIndex = status - WAIT_OBJECT_0;
                              
               if (handleIndex < (int)numHandles)
               {
                  returnStatus = handleIndex;
                  break;
               }
               else if ((firstHandleEventIndex != -1) && (handleIndex >= firstHandleEventIndex))
                  processHandleEvent(handleEventIndex[handleIndex - firstHandleEventIndex]);
               else
               {
                  BASSERT(0);
               }
            }
            
            if (timeToWait == 0)
               break;
         }            
      }         
   }
   else
   {
      const DWORD startTime = GetTickCount();
      
      for ( ; ; )
      {
         totalHandles = numBaseHandles;  
         
         int firstHandleEventIndex = -1;
         if (threadData.mNumHandleEvents)
         {
            firstHandleEventIndex = numBaseHandles;

            uint srcIndex = threadData.mHandleEventRover;

            BDEBUG_ASSERT(threadData.mNumHandleEvents <= BThreadData::cMaxHandleEvents);
            for (uint i = 0; i < threadData.mNumHandleEvents; i++)
            {
               while (srcIndex >= threadData.mNumHandleEvents)
                  srcIndex -= threadData.mNumHandleEvents;

               BDEBUG_ASSERT(srcIndex < threadData.mNumHandleEvents);
               handles[totalHandles] = threadData.mHandleEvents[srcIndex].mHandle;
               handleEventIndex[i] = static_cast<uchar>(srcIndex);

               totalHandles++;
               srcIndex++;
            }

            threadData.mHandleEventRover++;
            if (threadData.mHandleEventRover >= threadData.mNumHandleEvents)
               threadData.mHandleEventRover = 0;
         }
         
         BDEBUG_ASSERT(totalHandles <= cMaxHandles);
         
         DWORD timeToWait = INFINITE;            
         
         if (maxTimeToWait == 0)
            timeToWait = 0;
         else if (maxTimeToWait != INFINITE)
         {
            const DWORD timeSoFar = GetTickCount() - startTime;
            if (timeSoFar >= maxTimeToWait)
               break;
            timeToWait = maxTimeToWait - timeSoFar;
         }
         
         timeToWait = Math::Min<DWORD>(timeToWait, 60);
                           
         const DWORD status = WaitForMultipleObjects(totalHandles, handles, FALSE, timeToWait);
         
         flushDeferredEvents();
                                 
         if (status == WAIT_TIMEOUT)
         {
            
         }
         else if ((status >= WAIT_OBJECT_0) && (status <= WAIT_OBJECT_0 + totalHandles - 1))   
         {
            const int handleIndex = status - WAIT_OBJECT_0;
            
            if (handleIndex < (int)numHandles)
            {
               returnStatus = handleIndex;
               break;
            }
            else if (handleIndex == asyncHandleIndex)
               dispatchEvents();
            else if ((firstHandleEventIndex != -1) && (handleIndex >= firstHandleEventIndex))
               processHandleEvent(handleEventIndex[handleIndex - firstHandleEventIndex]);
            else
            {
               BASSERT(0);
            }
         }
         else
         {
            BFATAL_FAIL("BEventDispatcher::wait: Wait failed");
         }
         
         if (timeToWait == 0)
            break;
      }
   }      
   
   PIXEndNamedEvent();
   
   return returnStatus;
}

//============================================================================
// BEventDispatcher::addClient
//============================================================================
BEventReceiverHandle BEventDispatcher::addClient(BEventReceiverInterface* pClient, BThreadIndex clientThreadIndex, bool permitAsyncImmediateRemoval)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "EventDispatcherAddClient");
   
   if (cInvalidIndex == clientThreadIndex)
      clientThreadIndex = getThreadIndex();
   
   BDEBUG_ASSERT(mInitialized && (clientThreadIndex >= 0) && (clientThreadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[clientThreadIndex];
         
   BEventHandleStruct handle;
   
   handle = allocHandle(clientThreadIndex, permitAsyncImmediateRemoval);
   
   // Locking this handle slot in case this slot was previously asynchronously removed.
   threadData.mHandleSlots[handle.mSlotIndex].mMutex.lock();
   
   threadData.mHandleSlots[handle.mSlotIndex].mpClient = pClient;
   
   threadData.mHandleSlots[handle.mSlotIndex].mFlags = 0;
   if (permitAsyncImmediateRemoval)
      threadData.mHandleSlots[handle.mSlotIndex].mFlags |= BHandleSlot::cHFPermitAsyncImmediateRemoval;
   
   threadData.mHandleSlots[handle.mSlotIndex].mMutex.unlock();
   
   BEventReceiverHandle eventHandle = *reinterpret_cast<BEventReceiverHandle*>(&handle);
            
   PIXEndNamedEvent();
   
   send(cInvalidEventReceiverHandle, eventHandle, cEventClassClientAdded); 
   
   return eventHandle;
}

//============================================================================
// BEventDispatcher::removeClientImmediate
//============================================================================
void BEventDispatcher::removeClientImmediate(BEventReceiverHandle h)
{
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "EventDispatcherRemoveClientImmed");
   
   BDEBUG_ASSERT(mInitialized);
      
   const BEventHandleStruct& handle = *reinterpret_cast<const BEventHandleStruct*>(&h);
   BDEBUG_ASSERT((handle.mSlotIndex < mMaxThreadClients) && (handle.mUseCount > 0) && (handle.mThreadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[handle.mThreadIndex];
   
   if (threadData.mHandleSlots[handle.mSlotIndex].mFlags & BHandleSlot::cHFPermitAsyncImmediateRemoval)
   {
      threadData.mHandleSlots[handle.mSlotIndex].mMutex.lock();

      BDEBUG_ASSERT(isValidHandle(handle));

      freeHandle(handle);      

      threadData.mHandleSlots[handle.mSlotIndex].mMutex.unlock();
   }
   else if (getThreadIndex() == static_cast<BThreadIndex>(handle.mThreadIndex))
   {
      // Caller is on same thread as the handle, safe to immediately remove because dispatching can't be occurring to this handle's owner concurrently.
      
      BDEBUG_ASSERT(isValidHandle(handle));
      
      freeHandle(handle);      
   }
   else if (getThreadId(static_cast<BThreadIndex>(handle.mThreadIndex)) == 0)
   {
      trace("BEventDispatcher::removeClientImmediate: Removing a client from a thread that has already exited!");
      
      BDEBUG_ASSERT(isValidHandle(handle));

      freeHandle(handle);      
   }
   else 
   {
      BFATAL_FAIL("BEventDispatcher::removeClientImmediate: Cannot immediately remove a client from a different thread than it receives events on. Use removeClientDeferred instead, or set permitAsyncImmediateRemoval to true when calling addClient.");
   }
        
   PIXEndNamedEvent();
}

//============================================================================
// BEventDispatcher::removeClientDeferred
//============================================================================
void BEventDispatcher::removeClientDeferred(BEventReceiverHandle handle, bool waitForDelivery)
{
   send(cInvalidEventReceiverHandle, handle, cEventClassClientRemove, 0, 0, NULL, waitForDelivery ? cSendWaitForDelivery : 0);
}

//============================================================================
// BEventDispatcher::pushEvent
//============================================================================
bool BEventDispatcher::pushEvent(BThreadIndex toThreadIndex, const BEvent& event, bool deferIfFull)
{
   BThreadData& destThreadData = mThreadData[toThreadIndex];
   BDEBUG_ASSERT(destThreadData.mEvents.getMaxSize() > 0);
   
   const BThreadIndex curThreadIndex = getThreadIndex();
   if (curThreadIndex < 0)
   {
      // User is sending from a thread that is not registered into the system. We can't defer these events if the dest FIFO is full.
      bool sent = destThreadData.mEvents.pushBack(event);
      if (sent)
         destThreadData.mWakeupEvent.set();
      return sent;
   }
   
   flushDeferredEvents();
      
   BDEBUG_ASSERT(curThreadIndex < cMaxThreads);
   BThreadData& curThreadData = mThreadData[curThreadIndex];
      
   if (curThreadData.mDeferredEvents.getSize())
   {
      if (!deferIfFull)
         return false;
         
      if (!curThreadData.mDeferredEvents.pushBack(event))
      {
         if (curThreadData.mDeferredEvents.getMaxSize() >= 8192)
         {
#ifdef EVENT_DISPATCHER_LOGGING
            //CLM [11.11.08]
            //this is a fatal fail anyway, so in order to gather more information
            //we're not concerned with molesting the events structure
            BFileSystemStream csvFileStream;
            if (!csvFileStream.open(cDirProduction, "eventDispatcherFIFO.csv", cSFWritable | cSFSeekable))
            {
               BFATAL_FAIL("Unable to open file eventDispatcherFIFO.csv");
               
            }

            BBufferStream outputStream(csvFileStream);

            
            outputStream.printf("Thread : %i\n", (curThreadIndex));
            outputStream.printf("FromHandle, ToHandle,\n");

            while (!curThreadData.mDeferredEvents.getEmpty())
            {
               const BEvent& event = curThreadData.mDeferredEvents.peekFront(0);

               outputStream.printf("%I64d,%I64d\n", event.mToHandle, event.mFromHandle);

               curThreadData.mDeferredEvents.popFront();//CLM this needs to crash anyway, so we can corrupt the events here.
            }

            outputStream.close();

            csvFileStream.close();

            BFATAL_FAIL("BEventDispatcher::pushEvent: Deferred event FIFO grew too big! eventDispatcherFIFO.csv written to disk.");
#else

            

            //CLM [11.13.08] We propigate the failure here back up to the calling method, simply because we're not sure the operational
            //parameters that occur when this bug happens in final builds.
            return false;
#endif

         }

         curThreadData.mDeferredEvents.resize(Math::Max<DWORD>(8, curThreadData.mDeferredEvents.getMaxSize() * 2));
         if (!curThreadData.mDeferredEvents.pushBack(event))
         {
            BDEBUG_ASSERT(0);
         }
      }
         
      const bool waitingOnSelf = (curThreadIndex == toThreadIndex);

      if (!waitingOnSelf)
      {
         const BEvent& firstDeferredEvent = curThreadData.mDeferredEvents.peekFront(0);
         const BEventHandleStruct& firstDeferredToHandle = *reinterpret_cast<const BEventHandleStruct*>(&firstDeferredEvent.mToHandle);
         
         if (firstDeferredToHandle.mThreadIndex == (uint)curThreadIndex)
         {
            flushDeferredEvents();
         }
         else
         {
            uint i;
            for (i = 0; i < 4; i++)
            {
               flushDeferredEvents();
                              
               if (curThreadData.mDeferredEvents.getEmpty())
                  break;
                  
               const BEvent& firstDeferredEvent = curThreadData.mDeferredEvents.peekFront(0);
               const BEventHandleStruct& firstDeferredToHandle = *reinterpret_cast<const BEventHandleStruct*>(&firstDeferredEvent.mToHandle);
               if (firstDeferredToHandle.mThreadIndex == (uint)curThreadIndex)
                  break;
               
               Sleep(5);
            }
         }            
      }
   }
   else
   {
      bool sent = destThreadData.mEvents.pushBack(event);
         
      if (!sent)
      {
         if (!deferIfFull)
            return false;

         trace("BEventDispatcher::send: WARNING: Destination thread %u's FIFO is full, throttling on thread %u!", toThreadIndex, curThreadIndex);

         const bool waitingOnSelf = (curThreadIndex == toThreadIndex);

         if (!waitingOnSelf)
         {
            for (uint i = 0; i < 8; i++)
            {
               Sleep(5);
               sent = destThreadData.mEvents.pushBack(event);
               if (sent)   
                  break;
            }
         }

         if (!sent)
         {
            if (!curThreadData.mDeferredEvents.pushBack(event))
            {
               if (curThreadData.mDeferredEvents.getMaxSize() >= 8192)
               {
#ifdef EVENT_DISPATCHER_LOGGING
                  //CLM [11.11.08]
                  //this is a fatal fail anyway, so in order to gather more information
                  //we're not concerned with molesting the events structure
                  BFileSystemStream csvFileStream;
                  if (!csvFileStream.open(cDirProduction, "eventDispatcherFIFO.csv", cSFWritable | cSFSeekable))
                  {
                     BFATAL_FAIL("Unable to open file eventDispatcherFIFO.csv");

                  }

                  BBufferStream outputStream(csvFileStream);


                  outputStream.printf("Thread : %i\n", (curThreadIndex));
                  outputStream.printf("FromHandle, ToHandle,\n");

                  while (!curThreadData.mDeferredEvents.getEmpty())
                  {
                     const BEvent& event = curThreadData.mDeferredEvents.peekFront(0);

                     outputStream.printf("%I64d,%I64d\n", event.mToHandle, event.mFromHandle);

                     curThreadData.mDeferredEvents.popFront();//CLM this needs to crash anyway, so we can corrupt the events here.
                  }

                  outputStream.close();

                  csvFileStream.close();

                  BFATAL_FAIL("BEventDispatcher::pushEvent: Deferred event FIFO grew too big! eventDispatcherFIFO.csv written to disk.");
#else
                  //CLM [11.13.08] We propigate the failure here back up to the calling method, simply because we're not sure the operational
                  //parameters that occur when this bug happens in final builds.
                  return false;
#endif
               }
               
               curThreadData.mDeferredEvents.resize(Math::Max<DWORD>(8, curThreadData.mDeferredEvents.getMaxSize() * 2));
               if (!curThreadData.mDeferredEvents.pushBack(event))
               {
                  BDEBUG_ASSERT(0);
               }
            }
         }
      }
      
      if (sent)
         destThreadData.mWakeupEvent.set();
   }      
         
   return true;      
}

//============================================================================
// BEventDispatcher::send
//============================================================================
bool BEventDispatcher::send(const BEvent& event, bool deferIfFull, bool waitForDelivery)
{
   BDEBUG_ASSERT(mInitialized);
   
   PIXSetMarker(D3DCOLOR_ARGB(255,255,255,255), "EventDispatcherSend");
      
   const BEventHandleStruct& fromHandle = *reinterpret_cast<const BEventHandleStruct*>(&event.mFromHandle);
   const BEventHandleStruct& toHandle = *reinterpret_cast<const BEventHandleStruct*>(&event.mToHandle);
   const uint toThreadIndex = toHandle.mThreadIndex;
         
   fromHandle;
      
   BDEBUG_ASSERT((event.mFromHandle == cInvalidEventReceiverHandle) || ((fromHandle.mSlotIndex < mMaxThreadClients) && (fromHandle.mUseCount > 0) && (fromHandle.mThreadIndex < cMaxThreads)));
   BDEBUG_ASSERT((toHandle.mSlotIndex < mMaxThreadClients) && (toHandle.mUseCount > 0) && (toThreadIndex >= 0) && (toThreadIndex < cMaxThreads));
   
   if (waitForDelivery)
   {
      BDEBUG_ASSERT(0 != mThreadID[toThreadIndex]);
   }
   
   // We can't immediately send the event if the receiver is on the same thread, because another thread
   // could remove the client while we are trying to send the event. Perhaps we can forbid this as an optimization.
            
   const BEvent* pEvent = &event;
      
   volatile LONG deliveredFlag = -1;      
         
   BEvent eventCopy;
   if (waitForDelivery)
   {
      // If you're waiting, mPrivaeData2 cannot be used!
      BDEBUG_ASSERT(event.mPrivateData2 == 0);
      
      eventCopy = event;
      pEvent = &eventCopy;

      eventCopy.mEventFlags |= BEvent::cEventFlagSetDeliveredFlag;      
      
      // No need for a memory barrier here, the synchronized FIFO will do that when it locks its critical section.      
      eventCopy.mPrivateData2 = reinterpret_cast<uint>(&deliveredFlag);
   }
   
   if (!pushEvent(toThreadIndex, *pEvent, deferIfFull))
      return false;
      
   if (waitForDelivery)
   {
      // If this turns into an infinite loop, it means the dispatcher never got around to processing this send, which
      // could mean the target thread is stuck in an infinite loop somewhere and not dispatching events.
      do
      {
         wait(0, NULL, 1);
         
         // No need for a memory barrier here, the wait() method calls WaitForMultipleObjects.
      } while (-1 == deliveredFlag);
   }
   
   return true;
}

//============================================================================
// BEventDispatcher::sendParams
//============================================================================
bool BEventDispatcher::sendParams(
   BEventReceiverHandle fromHandle, 
   BEventReceiverHandle toHandle, 
   uint16 eventClass, 
   uint privateData, 
   uint privateData2, 
   BEventPayload* pPayload, 
   bool deliveryReceipt, 
   bool deferIfFull, 
   bool waitForDelivery, 
   bool synchronousDispatch)
{
   return send(BEvent(fromHandle, toHandle, eventClass, privateData, privateData2, pPayload, deliveryReceipt, synchronousDispatch), deferIfFull, waitForDelivery);
}

//============================================================================
// BEventDispatcher::sendSetAndWaitEvent
//============================================================================
void BEventDispatcher::sendSetAndWaitEvent(BThreadIndex threadIndex, HANDLE setHandle, HANDLE waitHandle)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));  
   BDEBUG_ASSERT((setHandle != INVALID_HANDLE_VALUE) || (waitHandle != INVALID_HANDLE_VALUE));
      
   PIXSetMarkerNoColor("EventDispatcherSendSetAndWaitEvent");
      
//-- FIXING PREFIX BUG ID 752
   const BThreadData& destThreadData = mThreadData[threadIndex];   
//--

   BEvent event(cInvalidEventReceiverHandle, destThreadData.mDispatcherClientHandle, cEventClassSetEventAndWait, reinterpret_cast<uint>(setHandle), reinterpret_cast<uint>(waitHandle), NULL, false);
               
   BDEBUG_ASSERT(destThreadData.mEvents.getMaxSize() > 0);
   
   pushEvent(threadIndex, event);
}

//============================================================================
// BEventDispatcher::waitUntilThreadQueueEmpty
//============================================================================
void BEventDispatcher::waitUntilThreadQueueEmpty(BThreadIndex destThread, bool allowEventsToDispatch)
{
   BDEBUG_ASSERT(mInitialized && (destThread >= 0) && (destThread < cMaxThreads));  

   BWin32Event event;
   sendSetAndWaitEvent(destThread, event.getHandle());
   
   const bool waitingOnSelf = (getThreadIndex() == destThread);
   waitingOnSelf;
      
   if (allowEventsToDispatch)
   {
      waitSingle(event.getHandle(), INFINITE);
   }
   else
   {
      // Can't do a low-level wait if the dest thread is the same as the thread that called us, because that would lockup the app.
      BASSERT(!waitingOnSelf);
      
      event.wait();
   }
}

//============================================================================
// BEventDispatcher::pumpUntilThreadQueueEmpty
//============================================================================
void BEventDispatcher::pumpUntilThreadQueueEmpty(BThreadIndex destThread, bool pumpDestThread)
{
   BDEBUG_ASSERT(mInitialized && (destThread >= 0) && (destThread < cMaxThreads));  
         
   if (pumpDestThread)
      forceSynchronousDispatch(destThread);
   
   BWin32Event event;   
   sendSetAndWaitEvent(destThread, event.getHandle());
   
   pumpAndWaitSingle(INFINITE, 8, event.getHandle());
}

//============================================================================
// BEventDispatcher::isValidHandle
//============================================================================
bool BEventDispatcher::isValidHandle(BEventHandleStruct handle) const
{
   BDEBUG_ASSERT((handle.mUseCount > 0) && (handle.mSlotIndex < mMaxThreadClients) && (handle.mThreadIndex < cMaxThreads));
   return mThreadData[handle.mThreadIndex].mHandleSlots[handle.mSlotIndex].mCurUseCount == handle.mUseCount;
}

//============================================================================
// BEventDispatcher::allocHandle
//============================================================================
BEventHandleStruct BEventDispatcher::allocHandle(BThreadIndex threadIndex, bool permitAsyncRemoval) 
{
   BThreadData& threadData = mThreadData[threadIndex];
   
   BScopedCriticalSection lock(threadData.mHandleMutex);

   uint index = 0;   
   for ( ; ; )
   {
      if (threadData.mLowestFreeHandleSlot == mMaxThreadClients)
      {
         BFATAL_FAIL("BEventDispatcher::allocHandle: Out of handles");
      }

      index = threadData.mLowestFreeHandleSlot;
      threadData.mLowestFreeHandleSlot++;

      if (!threadData.mHandleSlots[index].mpClient)
         break;
   }

   return BEventHandleStruct(index, threadData.mHandleSlots[index].mCurUseCount, threadIndex, permitAsyncRemoval);
}

//============================================================================
// BEventDispatcher::freeHandle
//============================================================================
void BEventDispatcher::freeHandle(BEventHandleStruct handle)
{
   const uint slotIndex = handle.mSlotIndex;
   const BThreadIndex threadIndex = handle.mThreadIndex;
   const uint useCount = handle.mUseCount;
   useCount;
         
   BDEBUG_ASSERT((useCount > 0) && (slotIndex < mMaxThreadClients) && (threadIndex >= 0) && (threadIndex < cMaxThreads));
   
   BThreadData& threadData = mThreadData[threadIndex];
   
   BScopedCriticalSection lock(threadData.mHandleMutex);
   
   threadData.mHandleSlots[slotIndex].mCurUseCount++;      
   threadData.mHandleSlots[slotIndex].mpClient = NULL;
   threadData.mLowestFreeHandleSlot = Math::Min<uint>(threadData.mLowestFreeHandleSlot, slotIndex);
}

//============================================================================
// BEventDispatcher::getHandleThreadIndex
//============================================================================
BThreadIndex BEventDispatcher::getHandleThreadIndex(BEventReceiverHandle h)
{
   const BEventHandleStruct& handle = *reinterpret_cast<const BEventHandleStruct*>(&h);   

   BDEBUG_ASSERT((handle.mSlotIndex < mMaxThreadClients) && (handle.mUseCount > 0) && (handle.mThreadIndex >= 0) && (handle.mThreadIndex < cMaxThreads));   
   
   return static_cast<BThreadIndex>(handle.mThreadIndex);
}

//============================================================================
// BEventDispatcher::setHandlePrivateData
//============================================================================
BEventReceiverHandle BEventDispatcher::setHandlePrivateData(BEventReceiverHandle handle, uint privateData)
{
   BDEBUG_ASSERT(privateData <= (1 << BEventHandleStruct::cUserBits));
   
   reinterpret_cast<BEventHandleStruct*>(&handle)->mUserData = privateData;
   return handle;
}

//============================================================================
// BEventDispatcher::getHandlePrivateData
//============================================================================
uint BEventDispatcher::getHandlePrivateData(BEventReceiverHandle handle)
{
   return reinterpret_cast<const BEventHandleStruct*>(&handle)->mUserData;
}

//============================================================================
// BEventDispatcher::removeHandlePrivateData
//============================================================================
BEventReceiverHandle BEventDispatcher::removeHandlePrivateData(BEventReceiverHandle handle)
{
   reinterpret_cast<BEventHandleStruct*>(&handle)->mUserData = 0;
   return handle;
}

//============================================================================
// BEventDispatcher::compareHandles
//============================================================================
bool BEventDispatcher::compareHandles(BEventReceiverHandle handle0, BEventReceiverHandle handle1)
{
   const BEventHandleStruct* pHandleStruct0 = reinterpret_cast<const BEventHandleStruct*>(&handle0);
   const BEventHandleStruct* pHandleStruct1 = reinterpret_cast<const BEventHandleStruct*>(&handle1);
   
   return 
      (pHandleStruct0->mSlotIndex   == pHandleStruct1->mSlotIndex) &&
      (pHandleStruct0->mThreadIndex == pHandleStruct1->mThreadIndex) &&
      (pHandleStruct0->mUseCount    == pHandleStruct1->mUseCount);
}

//============================================================================
// BEventDispatcher::getDispatcherClientHandle
//============================================================================
BEventReceiverHandle BEventDispatcher::getDispatcherClientHandle(BThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mInitialized && threadIndex >= 0 && threadIndex < cThreadIndexMax);
   BEventReceiverHandle eventHandle = mThreadData[threadIndex].mDispatcherClientHandle;
   BDEBUG_ASSERT(eventHandle != cInvalidEventReceiverHandle);
   return eventHandle;
}

//============================================================================
// BEventDispatcher::submitCallback
//============================================================================
void BEventDispatcher::submitCallback(BThreadIndex threadIndex, BCallbackFunc pCallback, uint privateData, BEventPayload* pPayload, bool waitForDelivery, bool synchronousDispatch)
{
   const BEventReceiverHandle eventHandle = getDispatcherClientHandle(threadIndex);
   sendParams(eventHandle, eventHandle, cEventClassCallback, (uint)pCallback, privateData, pPayload, false, true, waitForDelivery, synchronousDispatch);
}

//============================================================================
// BEventDispatcher::submitFunctor
//============================================================================
void BEventDispatcher::submitFunctor(BThreadIndex threadIndex, const BFunctor& functor, bool waitForDelivery, bool synchronousDispatch, bool deferIfFull)
{
   const BEventReceiverHandle eventHandle = getDispatcherClientHandle(threadIndex);
   BFunctorCallback* pPayload = BAlignedAlloc::New<BFunctorCallback>(ALIGN_OF(BFunctorCallback), gPrimaryHeap, functor);
   sendParams(eventHandle, eventHandle, cEventClassFunctorCallback, 0, 0, pPayload, false, deferIfFull, waitForDelivery, synchronousDispatch);   
}

//============================================================================
// BEventDispatcher::submitDataFunctor
//============================================================================
void BEventDispatcher::submitDataFunctor(BThreadIndex threadIndex, const BDataFunctor& functor, void* pPrivateData, bool waitForDelivery, bool synchronousDispatch, bool deferIfFull)
{
   const BEventReceiverHandle eventHandle = getDispatcherClientHandle(threadIndex);
   BDataFunctorCallback* pPayload = BAlignedAlloc::New<BDataFunctorCallback>(ALIGN_OF(BFunctorCallback), gPrimaryHeap, functor);
   sendParams(eventHandle, eventHandle, cEventClassDataFunctorCallback, reinterpret_cast<uint>(pPrivateData), 0, pPayload, false, deferIfFull, waitForDelivery, synchronousDispatch);   
}

//============================================================================
// BEventDispatcher::receiveEvent
//============================================================================
bool BEventDispatcher::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   threadIndex;
   
   BThreadData& threadData = mThreadData[threadIndex];
   
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         break;
      }
      case cEventClassClientRemove:
      {
         break;
      }
      case cEventClassCallback:
      {
         BCallbackFunc pCallbackFunc = (BCallbackFunc)event.mPrivateData;

         return pCallbackFunc(event.mPrivateData2, event.mpPayload);
      }   
      case cEventClassFunctorCallback:
      {
         BFunctorCallback* pCallbackFunctor = static_cast<BFunctorCallback*>(event.mpPayload);
         pCallbackFunctor->execute();
         break;
      }
      case cEventClassDataFunctorCallback:
      {
         BDataFunctorCallback* pCallbackFunctor = static_cast<BDataFunctorCallback*>(event.mpPayload);
         pCallbackFunctor->execute(reinterpret_cast<void*>(event.mPrivateData));
         break;
      }
      case cEventClassDispatchSynchronousEvents:
      {
         dispatchSynchronousEvents();
         break;
      }
      case cEventClassRegisterHandleWithEvent:
      {
//-- FIXING PREFIX BUG ID 754
         const BRegisterEventWithHandlePayload* pPayload = (BRegisterEventWithHandlePayload*)event.mpPayload;
//--
         
         uint i;
         for (i = 0; i < threadData.mNumHandleEvents; i++)
         {
            if (threadData.mHandleEvents[i].mHandle == pPayload->mHandle)
            {
               threadData.mHandleEvents[i] = *pPayload;
               break;
            }
         }
         
         if (i == threadData.mNumHandleEvents)
         {
            BVERIFY(threadData.mNumHandleEvents < BThreadData::cMaxHandleEvents);
            
            threadData.mHandleEvents[threadData.mNumHandleEvents] = *pPayload;
            threadData.mNumHandleEvents++;
         }
         
         break;
      }
      case cEventClassRegisterHandleWithCallback:
      {
         BRegisterEventWithHandlePayload* pPayload = (BRegisterEventWithHandlePayload*)event.mpPayload;

         uint i;
         for (i = 0; i < threadData.mNumHandleEvents; i++)
         {
            if (threadData.mHandleEvents[i].mHandle == pPayload->mHandle)
            {
               threadData.mHandleEvents[i].mEvent.mPrivateData = pPayload->mEvent.mPrivateData;
               threadData.mHandleEvents[i].mpCallback = pPayload->mpCallback;
               break;
            }
         }

         if (i == threadData.mNumHandleEvents)
         {
            BVERIFY(threadData.mNumHandleEvents < BThreadData::cMaxHandleEvents);

            threadData.mHandleEvents[threadData.mNumHandleEvents] = *pPayload;
            threadData.mNumHandleEvents++;
         }

         break;
      }
      case cEventClassDeregisterHandleWithEvent:
      {
         HANDLE handle = (HANDLE)event.mPrivateData;
         
         bool found = false;
         
         uint i;
         for (i = 0; i < threadData.mNumHandleEvents; i++)
         {
            if (threadData.mHandleEvents[i].mHandle == handle)
            {
               threadData.mHandleEvents[i] = threadData.mHandleEvents[threadData.mNumHandleEvents - 1];
               threadData.mNumHandleEvents--;
               found = true;
               break;
            }
         }
         
         if (!found)
         {
//            trace("BEventDispatcher::receiveEvent: Unable to deregister handle");
         }
         
         break;
      }
   }
   
   return false;
}

//============================================================================
// class BCreateThreadHelper
//============================================================================
class BCreateThreadHelper
{
public:
   BCreateThreadHelper(BThreadIndex threadIndex, const char* pThreadName, BWin32Event& event, LPTHREAD_START_ROUTINE pThreadStart, LPVOID threadParam, uint eventQueueSize, uint syncEventQueueSize) :
      mThreadIndex(threadIndex),
      mpThreadName(pThreadName),
      mEvent(event),
      mpThreadStart(pThreadStart),
      mThreadParam(threadParam),
      mEventQueueSize(eventQueueSize),
      mSyncEventQueueSize(syncEventQueueSize)
   {
   }
            
   static DWORD WINAPI ThreadStartFunc(LPVOID p)
   {
      BCreateThreadHelper* pData = static_cast<BCreateThreadHelper*>(p);
      
      char buf[64];
      sprintf_s(buf, "HelperThread%u", pData->mThreadIndex);
      SetThreadName(GetCurrentThreadId(), pData->mpThreadName ? pData->mpThreadName : buf);
      
      gEventDispatcher.registerThread(pData->mThreadIndex, GetCurrentThreadId(), pData->mEventQueueSize, pData->mSyncEventQueueSize);
      
      // Must make copies, because as soon as we set the event pData will point to dead data!
      const LPTHREAD_START_ROUTINE pThreadStart = pData->mpThreadStart;
      const BThreadIndex threadIndex = pData->mThreadIndex;
      const LPVOID threadParam = pData->mThreadParam;
      
      pData->mEvent.set();
      pData = NULL;
      
      const DWORD status = pThreadStart(threadParam);
      
      gEventDispatcher.registerThread(threadIndex, 0, 0);
      
      return status;
   }

private:   
   BThreadIndex mThreadIndex;
   BWin32Event& mEvent;
   LPTHREAD_START_ROUTINE mpThreadStart;
   LPVOID mThreadParam;
   const char* mpThreadName;
   uint mEventQueueSize;
   uint mSyncEventQueueSize;
};

//============================================================================
// BEventDispatcher::createThread
//============================================================================
HANDLE BEventDispatcher::createThread(
   BThreadIndex threadIndex,
   const char* pThreadName,
   uint eventQueueSize,
   uint syncEventQueueSize,
   LPSECURITY_ATTRIBUTES lpThreadAttributes, 
   DWORD dwStackSize, 
   LPTHREAD_START_ROUTINE lpStartAddress, 
   LPVOID lpParameter, 
   DWORD dwCreationFlags, 
   LPDWORD lpThreadId,
   int hardwareThread)
{
   hardwareThread;
   BDEBUG_ASSERT(threadIndex >= 0 && threadIndex < cThreadIndexMax);

   BWin32Event event;
   
   BCreateThreadHelper createThreadHelper(threadIndex, pThreadName, event, lpStartAddress, lpParameter, eventQueueSize, syncEventQueueSize);
   
   HANDLE threadHandle = CreateThread(lpThreadAttributes, dwStackSize, BCreateThreadHelper::ThreadStartFunc, &createThreadHelper, dwCreationFlags | CREATE_SUSPENDED, lpThreadId);
   if (NULL == threadHandle)
      return NULL;
   
#ifdef XBOX 
   if (hardwareThread != -1)
      XSetThreadProcessor(threadHandle, hardwareThread);
#endif   

   ResumeThread(threadHandle);
   
   waitSingle(event.getHandle());
      
   return threadHandle;      
}   

//============================================================================
// BEventDispatcher::pumpThreadAndWait
//============================================================================
int BEventDispatcher::pumpThreadAndWait(BThreadIndex threadIndex, DWORD timeToWait, DWORD timeBetweenPumps, HANDLE handle, bool pumpCurThread)
{
   const DWORD startTime = GetTickCount();

   for ( ; ; )
   {
      forceSynchronousDispatch(threadIndex);
      
      if (pumpCurThread)
         dispatchSynchronousEvents();      
         
      int status = waitSingle(handle, timeBetweenPumps);

      if (pumpCurThread)
         dispatchSynchronousEvents();      

      if (status == 0)
         return 0;

      DWORD curTime = GetTickCount() - startTime;
      if ((curTime > timeToWait) || (timeToWait == 0))
         break;
   }

   return -1;
}

//============================================================================
// BEventDispatcher::pumpAndWait
//============================================================================
int BEventDispatcher::pumpAndWait(DWORD timeToWait, DWORD timeBetweenPumps, uint numHandles, const HANDLE* pHandles, BEventReceiverHandle receiverHandle)
{
   const DWORD startTime = GetTickCount();

   for ( ; ; )
   {
      dispatchSynchronousEvents(receiverHandle);      
      
      int status = wait(numHandles, pHandles, timeBetweenPumps);
      
      dispatchSynchronousEvents(receiverHandle);      
      
      if ((status >= 0) && (status < static_cast<int>(numHandles)))
         return status;

      DWORD curTime = GetTickCount() - startTime;
      if ((curTime > timeToWait) || (timeToWait == 0))
         break;
   }
   
   return -1;
}

//============================================================================
// BEventDispatcher::pumpAllThreads
//============================================================================
int BEventDispatcher::pumpAllThreads(DWORD timeToWait, DWORD timeBetweenPumps, HANDLE handle)
{
   SCOPEDSAMPLE(BEventDispatcher_pumpAllThreads)
   const DWORD startTime = GetTickCount();
   
   for ( ; ; )
   {
      for (uint i = 0; i < cThreadIndexMax; i++)
         if (mThreadID[i] != 0)
            forceSynchronousDispatch(static_cast<BThreadIndex>(i));

      int status = waitSingle(handle, timeBetweenPumps);   
      if (status == 0)
         return 0;
      
      DWORD curTime = GetTickCount() - startTime;
      if ((curTime > timeToWait) || (timeToWait == 0))
         break;
   }
   
   return -1;
}

//============================================================================
// BEventDispatcher::registerHandleWithEvent
//============================================================================
void BEventDispatcher::registerHandleWithEvent(HANDLE handle, const BEvent& event, bool deferIfQueueFull)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(handle != INVALID_HANDLE_VALUE);
   BDEBUG_ASSERT(event.mpPayload == NULL);

   BThreadIndex dstThreadIndex = getHandleThreadIndex(event.mToHandle);
   
   BRegisterEventWithHandlePayload* pPayload = new BRegisterEventWithHandlePayload(handle, event, deferIfQueueFull);
   
   send(cInvalidEventReceiverHandle, mThreadData[dstThreadIndex].mDispatcherClientHandle, cEventClassRegisterHandleWithEvent, 0, 0, pPayload);
}

//============================================================================
// BEventDispatcher::registerHandleWithCallback
//============================================================================
void BEventDispatcher::registerHandleWithCallback(HANDLE handle, BThreadIndex threadIndex, BHandleEventCallbackFunc pCallback, uint privateData)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(handle != INVALID_HANDLE_VALUE);
   BDEBUG_ASSERT(threadIndex < cMaxThreads);
   
   BRegisterEventWithHandlePayload* pPayload = new BRegisterEventWithHandlePayload(handle, pCallback, privateData);

   send(cInvalidEventReceiverHandle, mThreadData[threadIndex].mDispatcherClientHandle, cEventClassRegisterHandleWithCallback, 0, 0, pPayload);
}

//============================================================================
// BEventDispatcher::deregisterHandle
//============================================================================
void BEventDispatcher::deregisterHandle(HANDLE handle, BThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mInitialized && (threadIndex >= 0) && (threadIndex < cMaxThreads));  
   BDEBUG_ASSERT(handle != INVALID_HANDLE_VALUE);

   BCOMPILETIMEASSERT(sizeof(HANDLE) <= sizeof(uint));
   
   send(cInvalidEventReceiverHandle, mThreadData[threadIndex].mDispatcherClientHandle, cEventClassDeregisterHandleWithEvent, (uint)handle);
}

//============================================================================
// BHaltThreadData
//============================================================================
struct BHaltThreadData
{
   BThreadIndex   mThreadIndex;
   BWin32Event    mAckEvent;
   BWin32Event    mResumeEvent;   
};

//============================================================================
// BEventDispatcher::haltThreadRequest
//============================================================================
BHandle BEventDispatcher::haltThreadRequest(BThreadIndex threadIndex)
{
   BHaltThreadData* p = new BHaltThreadData;
   p->mThreadIndex = threadIndex;
   
   sendSetAndWaitEvent(threadIndex, p->mAckEvent, p->mResumeEvent);
         
   return p;
}

//============================================================================
// BEventDispatcher::haltThreadWait
//============================================================================
void BEventDispatcher::haltThreadWait(BHandle handle)
{
   BHaltThreadData* p = static_cast<BHaltThreadData*>(handle);
   if (p)
      waitSingle(p->mAckEvent);
}

//============================================================================
// BEventDispatcher::resumeThread
//============================================================================
void BEventDispatcher::resumeThread(BHandle handle)
{
   BHaltThreadData* p = static_cast<BHaltThreadData*>(handle);
   if (p)
   {
      SetEvent(p->mResumeEvent);
      
      waitSingle(p->mAckEvent);
      
      delete p;
   }
}






