// renderEventDispatcher.h
#pragma once

// xcore
#include "threading\synchronizedFIFO.h"
#include "threading\win32Event.h"
#include "threading\win32WaitableTimer.h"

typedef uint64 BRenderEventReceiverHandle;
const BRenderEventReceiverHandle cInvalidRenderEventReceiverHandle = UINT64_MAX;

class BRenderEventPayload
{
public:
   // delivered will be true if the payload was passed to the event receiver object.
   // Otherwise, the receiver removed itself before the event could be delivered.
   virtual void deleteThis(bool delivered) = 0;
};

enum eRenderThreadIndex
{
   cRenderThreadIndexMain = 0,
   cRenderThreadIndexWorker = 1,
   cRenderThreadIndexHelper = 2,
   
   cRenderThreadIndexMax,
   
   cRTIForceDWORD = 0xFFFFFFFF
};

enum eRenderEventClass
{
   cRenderEventClassUndefined = 0,
   
   cRenderEventClassHeartbeat,
   cRenderEventClassAsyncFile,
   cRenderEventClassTextureStatusChanged,
   cRenderEventClassTextureManager,
      
   cRenderEventClassMax,
   
   cRECForceDWORD = 0xFFFFFFFF
};

typedef int BRenderThreadIndex;

struct BRenderEventHandleStruct
{
   BRenderEventHandleStruct() { }

   BRenderEventHandleStruct(uint slotIndex, uint useCount, BRenderThreadIndex threadIndex) : mSlotIndex(slotIndex), mUseCount(useCount), mThreadIndex(threadIndex) { }

   uint mSlotIndex : 32;
   uint mUseCount : 24;
   int mThreadIndex : 8;
};

// BRenderEvent must be bitwise copyable
struct BRenderEvent         
{
   BRenderEvent() { }

   BRenderEvent(BRenderEventReceiverHandle fromHandle, BRenderEventReceiverHandle toHandle, uint eventClass, uint64 privateData, uint32 privateData2, BRenderEventPayload* pPayload) : 
      mFromHandle(fromHandle), 
      mToHandle(toHandle), 
      mEventClass(eventClass), 
      mPrivateData(privateData), 
      mPrivateData2(privateData2), 
      mpPayload(pPayload) 
   { 
   }

   BRenderEventReceiverHandle mFromHandle;
   BRenderEventReceiverHandle mToHandle;
   uint64 mPrivateData;
   uint mPrivateData2;
   uint mEventClass;
   BRenderEventPayload* mpPayload;
};

class BRenderEventReceiverInterface
{
public:
   // Return true if the payload was resent/forwarded to another client. It won't be automatically deleted in this case.
   // Return false to have the manager call deleteThis() on the payload.
   // toHandle is guaranteed to remain valid while you are in this method, but the fromHandle may be or become stale.
   // It's fine to send a reponse back to fromHandle, even if it's stale, because the dispatcher drops stale handles on the floor.
   // fromHandle could be cInvalidRenderEventReceiverHandle.
   virtual bool receiveEvent(const BRenderEvent& event, int threadIndex) = 0;
};

typedef int BHeartbeatHandle;
enum { cInvalidHeartbeatHandle = cInvalidIndex };

class BRenderEventDispatcher
{
public:
   enum { cMaxThreads = cRenderThreadIndexMax };
   
   BRenderEventDispatcher();
   
   ~BRenderEventDispatcher();
      
   void init(uint maxClients = 1024);
      
   // deinit kills all in-flight events without delivering them.
   void deinit(void);
            
   // setThreadId() should be called from the thread being changed.
   void setThreadId(BRenderThreadIndex threadIndex, DWORD threadId);
   DWORD getThreadId(BRenderThreadIndex threadIndex);
   
   BWin32Event& getThreadWakeupEvent(BRenderThreadIndex threadIndex);
   BWin32WaitableTimer& getHeartbeatTimer(void);
   
   // Returns 0 if the current thread isn't registered.
   BRenderThreadIndex getThreadIndex(void) const { return mRenderThreadIndex; }
      
   // dispatchEvents() takes the current thread's critical section. This will prevent other threads from
   // adding/removing clients during dispatching. 
   void dispatchEvents(void);
   
   void dispatchHeartbeat(void);
                 
   // -1 if timed out, otherwise the returned index indicates which handle succeeded the wait.
   // If maxTimeToWait is INFINITE and numHandles == 0, this routine will never return!!
   int wait(uint numHandles = 0, const HANDLE* pHandles = NULL, DWORD maxTimeToWait = INFINITE);
   int wait(HANDLE handle, DWORD timeout = INFINITE) { return wait(1, &handle, timeout); }
                           
   // The add and remove client methods lock the client thread's critical section. 
   // So if the client thread is blocking inside dispatchEvents(), so will this call.
   // clientThreadIndex is the thread the client will receive events on. If clientThreadIndex is cInvalidIndex, the current thread's index is used.
   BRenderEventReceiverHandle addClient(BRenderEventReceiverInterface* pClient, BRenderThreadIndex clientThreadIndex = cInvalidIndex);
   void removeClient(BRenderEventReceiverHandle handle);

   BHeartbeatHandle addHeartbeatReceiver(BRenderEventReceiverHandle handle, uint frameInterval = 1, uint privateData = 0);
   bool removeHeartbeatReceiver(BHeartbeatHandle handle);
         
   // Takes ownership of pPayload. The payload will always be deleted (via the deleteThis() member) by the manager after delivery.
   // Queued events may not necessarily be delivered. If the receiving client removes themselves while the event
   // is in flight, for example. In this case, the payload is dropped on the floor and the payload is deleted.
   // fromHandle can be invalid, in case no reply is needed.
   // send() can only fail if the destination thread's event queue is full.
   bool send(const BRenderEvent& event);
   
   bool send(BRenderEventReceiverHandle fromHandle, BRenderEventReceiverHandle toHandle, uint eventClass = cRenderEventClassUndefined, uint64 privateData = 0, uint32 privateData2 = 0, BRenderEventPayload* pPayload = NULL)
   {
      return send(BRenderEvent(fromHandle, toHandle, eventClass, privateData, privateData2, pPayload));
   }
         
   bool delayedSend(uint framesToDelay, const BRenderEvent& event);
            
   // This asserts if you pass in an obviously bad handle, however it does not do a full validity check because that would require a lock.
   BRenderThreadIndex getThreadIndexFromHandle(BRenderEventReceiverHandle handle);
                                       
private:
   BRenderEventDispatcher(const BRenderEventDispatcher&);
   BRenderEventDispatcher& operator= (const BRenderEventDispatcher&);

   bool mInitialized;   
   uint mMaxThreadClients;
         
   struct BHandleSlot
   {
      BHandleSlot() : mCurUseCount(0), mpClient(NULL) { }

      uint mCurUseCount;
      BRenderEventReceiverInterface* mpClient;
   };
   
   typedef BDynamicArray<BHandleSlot> BHandleSlotArray;
         
   enum { cEventQueueSize = 1024 };
   typedef BSynchronizedFIFO<BRenderEvent, cEventQueueSize> BRenderEventQueue;
   
   struct BThreadData
   {
      BCriticalSection mMutex;
      BWin32Event mWakeupEvent;
      BHandleSlotArray mHandleSlots;
      uint mLowestFreeHandleSlot;
      BRenderEventQueue mEvents;
   };
   
   BThreadData mThreadData[cMaxThreads];
         
   struct BDelayedEvent : BRenderEvent
   {
      BDelayedEvent() { }
      
      BDelayedEvent(uint framesToDelay, const BRenderEvent& event) : 
         BRenderEvent(event),
         mFramesRemaining(framesToDelay)
      {
      }
      
      uint mFramesRemaining;
   };
   
   BCriticalSection mDelayedEventsMutex;
   BDynamicArray<BDelayedEvent, sizeof(uint64), false, true, false> mDelayedEvents;
   
   static __declspec(thread) BRenderThreadIndex mRenderThreadIndex;
   
   BCriticalSection mHeartbeatMutex;
   
   BWin32WaitableTimer mHeartbeatTimer;
   
   class BHeartbeatReceiver
   {
   public:
      BHeartbeatReceiver() { }
      BHeartbeatReceiver(BRenderEventReceiverHandle handle, uint interval, uint privateData) : mHandle(handle), mInterval((WORD)interval), mFramesRemaining((WORD)interval), mPrivateData(privateData) { }
      BRenderEventReceiverHandle mHandle;
      uint mPrivateData;
      WORD mInterval;
      WORD mFramesRemaining;

      bool operator== (const BHeartbeatReceiver& rhs) const { return (mHandle == rhs.mHandle) && (mPrivateData == rhs.mPrivateData); } 
   };

   BDynamicArray<BHeartbeatReceiver> mHeartbeatReceivers;
   
   void clearHeartbeatReceivers(void);
   void tickHeartbeatReceivers(void);
   
   BRenderEventHandleStruct allocHandle(BRenderThreadIndex threadIndex);
   bool isValidHandle(BRenderEventHandleStruct handle) const;
   void freeHandle(BRenderEventHandleStruct handle);
   void dispatchDelayedEvents(void);
   void clear(void);
   
};

extern BRenderEventDispatcher gRenderEventDispatcher;



