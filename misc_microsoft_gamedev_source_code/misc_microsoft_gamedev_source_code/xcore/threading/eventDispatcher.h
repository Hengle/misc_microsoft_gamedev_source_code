//============================================================================
//
// File: eventDispatcher.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

// xcore
#include "threading\synchronizedFIFO.h"
#include "threading\win32Event.h"
#include "threading\win32WaitableTimer.h"
#include "containers\queue.h"
#include "functor\functor.h"

//============================================================================
// enum eThreadIndex
// Note: The following two enums are game specific and should be 
// moved into a separate file out of xcore, or moved to the thread 
// manager's header. BEventDispatcher itself doesn't care which 
// constants are assigned threads (except for BEventDispatcher::cMaxThreads).
// If you change this method, be sure to also change BEventDispatcher::getThreadName!
//============================================================================
enum eThreadIndex
{
   cThreadIndexInvalid = -1,
   
   cThreadIndexSim,           //0
   cThreadIndexSimHelper,     //1

   cThreadIndexRender,        //2
   cThreadIndexRenderHelper,  //3
      
   cThreadIndexIO,            //4
   cThreadIndexMisc,          //5
   
   //cThreadIndexRenderIdle,    //6

   cThreadIndexMax,

   cRTIForceDWORD = 0xFFFFFFFF
};

#define ASSERT_THREAD(index) BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == (index))

//============================================================================
// enum eEventClass
//============================================================================
enum eEventClass
{
   cEventClassUndefined = 0,
   
   // An event with this class will be immediately sent to an added client. It will always be the first event a client receives.
   cEventClassClientAdded,
   
   // The client was removed from the system by calling removeClientDeferred. The client will no longer receive events from the event system.
   cEventClassClientRemove,
   
   // The client's thread is exiting, but the client is still registered to receive events on that thread. Do cleanup here.
   cEventClassThreadIsTerminating,
         
   // Sets the supplied Win-32 event in mPrivateData, then calls WaitForSingleObject on mPrivateData2.
   cEventClassSetEventAndWait,
   
   cEventClassCallback,
   cEventClassFunctorCallback,
   cEventClassDataFunctorCallback,
         
   cEventClassDispatchSynchronousEvents,
   
   cEventClassAsyncFile,
   cEventClassReloadNotify,
   
   cEventClassRegisterHandleWithEvent,
   cEventClassRegisterHandleWithCallback,
   cEventClassDeregisterHandleWithEvent,
   
   cEventClassHalt,
   
   cEventClassFirstUser,
               
   cEventClassMax
};   

typedef int BThreadIndex;

typedef uint64 BEventReceiverHandle;
const BEventReceiverHandle cInvalidEventReceiverHandle = UINT64_MAX;

//============================================================================
// class BEventPayLoad
//============================================================================
class __declspec(novtable) BEventPayload
{
public:
   // delivered will be true if the payload was passed to the event receiver object.
   // Otherwise, the receiver removed itself before the event could be delivered.
   virtual void deleteThis(bool delivered) = 0;
};

//============================================================================
// struct BEventHandleStruct
//============================================================================
struct BEventHandleStruct
{
   BEventHandleStruct() { }

   BEventHandleStruct(uint slotIndex, uint useCount, BThreadIndex threadIndex, bool permitAsyncRemoval) : 
      mSlotIndex(slotIndex), 
      mUseCount(useCount), 
      mThreadIndex(threadIndex),
      mUserData(0),
      mPermitAsyncRemoval(permitAsyncRemoval)
   { 
      BCOMPILETIMEASSERT(sizeof(BEventHandleStruct) == 8);
   }

   enum { cSlotBits           = 12, cMaxSlots = 1 << cSlotBits };
   enum { cThreadBits         = 3, cMaxThreads = 1 << cThreadBits };
   enum { cAsyncRemovalBits   = 1 };
   enum { cUserBits           = 16 };
   enum { cUseCountBits       = 32 };
      
   uint mSlotIndex            : cSlotBits; 
   uint mThreadIndex          : cThreadBits;
   uint mPermitAsyncRemoval   : cAsyncRemovalBits;
   uint mUserData             : cUserBits;
   uint mUseCount             : cUseCountBits;
};

//============================================================================
// struct BEvent
// BEvent must be bitwise copyable. Should be 32 bytes.
//============================================================================
struct BEvent
{
   enum 
   {
      // Request a delivery receipt
      cEventFlagRequestDeliveryReceipt = 1,
      
      // Delivery receipt reply event will have one of these flags set.
      cEventFlagDeliverySucceeded      = 2,
      cEventFlagDeliveryFailed         = 4,
      
      // If set, the dispatcher will write a 0 or 1 into the volatile LONG addressed by mPrivateData2 
      // to indicate the event was either dropped or delivered. This is dangerous; it's used internally by
      // the synchronous delivery mechanism.
      cEventFlagSetDeliveredFlag       = 8,
      
      // If set, the dispatcher will set the specified Win32 event after the event is dispatched (or not).
      // The handle must be in mPrivateData2.
      cEventFlagSetWin32Event         = 16,
      
      cEventFlagSynchronous           = 32,
      
      cEventFlagAlreadyDispatched     = 64,
      
      cEventFlagDeliveryReceiptReply  = 128
   };
   
   BEvent() { }

   BEvent(BEventReceiverHandle fromHandle, BEventReceiverHandle toHandle, uint eventClass, uint privateData, uint privateData2, BEventPayload* pPayload, bool deliveryReceipt = false, bool synchronous = false) : 
      mFromHandle(fromHandle), 
      mToHandle(toHandle), 
      mEventClass(static_cast<uint16>(eventClass)), 
      mPrivateData(privateData), 
      mPrivateData2(privateData2), 
      mpPayload(pPayload) 
   { 
      BCOMPILETIMEASSERT(sizeof(BEvent) == 32);
      mEventFlags = static_cast<uint16>(deliveryReceipt ? cEventFlagRequestDeliveryReceipt : 0);
      if (synchronous)
         mEventFlags |= cEventFlagSynchronous;
   }
   
   void clear(void)
   {
      mFromHandle = cInvalidEventReceiverHandle;
      mToHandle = cInvalidEventReceiverHandle;
      mPrivateData = 0;
      mPrivateData2 = 0;
      mEventClass = 0;
      mEventFlags = 0;
      mpPayload = NULL;
   }
      
   BEventReceiverHandle mFromHandle;
   BEventReceiverHandle mToHandle;
   uint mPrivateData;
   uint mPrivateData2;
   BEventPayload* mpPayload;
   uint16 mEventClass;
   uint16 mEventFlags;
};

// This is the interface that must implemented in order to receive events on a thread.
class __declspec(novtable) BEventReceiverInterface
{
public:
   // Notes about receiveEvent():
   // IN MOST USAGE SITUATIONS receiveEvent() RETURNS FALSE.
   // Return false to have the manager call deleteThis() on the payload. 
   // Return true if the payload was resent/forwarded to another client. It won't be automatically deleted in this case.
   // DO NOT return true from this virtual method unless you have taken ownership of the event's payload!!! Otherwise you will cause a leak.
   
   // toHandle is guaranteed to remain valid while you are in this method, but the fromHandle may be or become stale.
   // It's fine to send a reponse back to fromHandle, even if it's stale, because the dispatcher drops stale handles on the floor.
   // fromHandle could be cInvalidEventReceiverHandle.
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex) = 0;
};

//============================================================================
// class BEventDispatcher
//============================================================================
class BEventDispatcher : public BEventReceiverInterface
{
public:
   enum { cMaxThreads = 8 };
   
   BEventDispatcher();
   ~BEventDispatcher();
      
   void init(uint maxClients = 3072);
      
   // deinit kills all in-flight events without delivering them.
   void deinit(void);
   
   bool getInitialized(void) const { return mInitialized; }
   
   enum { cDefaultEventQueueSize = 2048, cDefaultSyncEventQueueSize = 0 };
      
   // This method is a replacement for CreateThread(). It waits until the thread has started and registered 
   // itself with the system before it returns. It automatically deregisters the thread when the user provided
   // thread function returns. Remember to close the handle when the thread exits!
   HANDLE createThread(
      BThreadIndex threadIndex,
      const char* pName,
      uint asyncEventQueueSize,
      uint syncEventQueueSize,
      LPSECURITY_ATTRIBUTES lpThreadAttributes, 
      DWORD dwStackSize, 
      LPTHREAD_START_ROUTINE lpStartAddress, 
      LPVOID lpParameter, 
      DWORD dwCreationFlags, 
      LPDWORD lpThreadId,
      int hardwareThread = -1);

   // Low-level API to set/get a thread's ID by index. Use setThreadId() if you create your own threads, instead of calling createThread().
   // setThreadId() must be called from the thread being changed, because this uses TLS.
   // Set threadId to 0 if you are removing a thread (this does not clear the thread's queue).
   // The event queue sizes must be a power of 2.
   void registerThread(BThreadIndex threadIndex, DWORD threadId, uint asyncEventQueueSize = cDefaultEventQueueSize, uint syncEventQueueSize = cDefaultSyncEventQueueSize);
   void deregisterThread(BThreadIndex threadIndex) { registerThread(threadIndex, 0, 0, 0); }
   
   DWORD getThreadId(BThreadIndex threadIndex) const;
   BThreadIndex getThreadIndex(void) const { return mThreadIndex; }
   
   static const char* getThreadName(eThreadIndex threadIndex);
      
   BThreadIndex getThreadIndexByID(DWORD threadID);
   
   // This event will be set if a message is waiting.
   BWin32Event& getThreadWakeupEvent(BThreadIndex threadIndex);
      
   // This asserts if you pass in an obviously bad handle, however it does not do a full validity check because that would require a lock.
   BThreadIndex getHandleThreadIndex(BEventReceiverHandle handle);
   
   void dispatchSynchronousEvents(BEventReceiverHandle receiverHandler = cInvalidEventReceiverHandle);
   
   void forceSynchronousDispatch(BThreadIndex threadIndex);
            
   // dispatchEvents() takes the current thread's critical section. This will prevent other threads from
   // adding/removing clients during dispatching. 
   void dispatchEvents(void);
   
   // -1 if timed out, otherwise the returned index indicates which handle succeeded the wait.
   // If maxTimeToWait is INFINITE and numHandles == 0, this routine will never return!!
   // If maxTimeToWait is 0, the handles are checked but no waiting is done.
         
   int wait(uint numHandles = 0, const HANDLE* pHandles = NULL, DWORD maxTimeToWait = INFINITE, bool dispatchEvents = true);
   int sleep(DWORD maxTimeToWait, bool dispatchEvents = true) { return wait(0, NULL, maxTimeToWait, dispatchEvents); }
   int waitSingle(HANDLE handle, DWORD timeout = INFINITE, bool dispatchEvents = true) { if (handle != INVALID_HANDLE_VALUE) return wait(1, &handle, timeout, dispatchEvents); else return wait(0, NULL, timeout, dispatchEvents); }
                                 
   // Adds a new client to the system.
   // clientThreadIndex is the thread the client will receive events on. If clientThreadIndex is cInvalidIndex, the current thread's index is used.
   // If permitAsyncImmediateRemoval is true, a lightweight lock will be taken every time an event is dispatched to this client, or when the client is immediately removed. 
   // If you know for sure that you'll be removing the client from the same thread that it will be receiving events on, or if you're going to remove it by 
   // calling removeClientDeferred, you should set permitAsyncImmediateRemoval to false, which disables the per-dispatch lock.
   BEventReceiverHandle addClient(BEventReceiverInterface* pClient, BThreadIndex clientThreadIndex = cInvalidIndex, bool permitAsyncImmediateRemoval = false);
   
   // Immediately removes a client. 
   // If this is called from the thread that this handle receive's events on, it will always be immediately removed the system (independent of this client's permitAsyncImmediateRemoval flag).
   // getHandleThreadIndex(handle) must equal getThreadIndex() in this instance.
   // If you must remove a client from a different thread, it must have been added to the system by setting permitAsyncImmediateRemoval to true.
   // Note that immediate removal from a thread different than the client's dispatch thread invites deadlocks if you're not careful!
   void removeClientImmediate(BEventReceiverHandle handle);
   
   // Sends a cEventClassClientRemove event to the client. When the client returns from processing this message, it will be removed from the system
   // and will no longer receive events.
   // The object associated with this handle CANNOT go out of scope until it processes this event!
   // If waitForDelivery is true, this method won't return until the send completes on the destination thread.
   // IF YOU DON'T UNDERSTAND THIS, set waitForDelivery to true!
   void removeClientDeferred(BEventReceiverHandle handle, bool waitForDelivery);
            
   // send() takes ownership of pPayload. The payload will always be deleted (via the deleteThis() member) by the manager after delivery.
   // Queued events may not necessarily be delivered. If the receiving client removes themselves while the event
   // is in flight, for example. In this case, the payload is dropped on the floor and the payload is deleted.
   // fromHandle can be invalid, in case no reply is needed.
   // send() can only fail if the destination thread's event queue is full, even after waiting for a while for the queue to empty.
   // If waitForDelivery is true, send() will wait until the message is either delivered or dropped. (Messages can still be dispatched 
   // on the caller's thread during the wait). This is SLOW, only intended for initialization and shutdown code. The mPrivateData2 failed of the 
   // BEvent struct must be unused for sync delivery. The return value is set only if waitForDelivery is true. Otherwise it's always true.
   bool send(const BEvent& event, bool deferIfFull = true, bool waitForDelivery = false);
      
   // Deprecated!
   bool sendParams(
      BEventReceiverHandle fromHandle, BEventReceiverHandle toHandle, 
      uint16 eventClass = 0, 
      uint privateData = 0, 
      uint privateData2 = 0, 
      BEventPayload* pPayload = NULL, 
      bool deliveryReceipt = false, 
      bool deferIfFull = true, 
      bool waitForDelivery = false,
      bool synchronousDispatch = false);
      
   enum
   {
      cSendDeliveryReceipt       = 1,
      cSendNoWaitIfFull          = 2,
      cSendWaitForDelivery       = 4,
      cSendSynchronousDispatch   = 8
   };
      
   bool send(
      BEventReceiverHandle fromHandle, BEventReceiverHandle toHandle, 
      uint16 eventClass = 0, 
      uint privateData = 0, 
      uint privateData2 = 0, 
      BEventPayload* pPayload = NULL, 
      DWORD flags = 0) 
   {
      return send(
         BEvent(fromHandle, toHandle, eventClass, privateData, privateData2, pPayload, (flags & cSendDeliveryReceipt) != 0, (flags & cSendSynchronousDispatch) != 0), 
         (flags & cSendNoWaitIfFull) == 0, 
         (flags & cSendWaitForDelivery) != 0 );
   }       
      
   // Sends an event to the destination thread's dispatcher.
   // If setHandle is not INVALID_HANDLE_VALUE, SetEvent() is called on the specified handle.
   // If waitHandle is not INVALID_HANDLE_VALUE, WaitForSingleObject() is then called on the specified handle with an infinite timeout.
   // At least one handle must be valid.
   // This can be used to implement destination thread event fences, or to safely pause the destination thread from dispatching events.
   void sendSetAndWaitEvent(BThreadIndex destThread, HANDLE setHandle = INVALID_HANDLE_VALUE, HANDLE waitHandle = INVALID_HANDLE_VALUE);
   
   // Blocks until the destination thread's FIFO is empty. 
   // If allowEventsToDispatch is true, events can be dispatched on the caller's thread during the wait. 
   // Otherwise, a low-level Win32 Wait() call is used. If destThread is the current thread, allowEventsToDispatch must be true.
   void waitUntilThreadQueueEmpty(BThreadIndex destThread, bool allowEventsToDispatch = true);
   
   void pumpUntilThreadQueueEmpty(BThreadIndex destThread, bool pumpDestThread = true);
      
   // Callback returns true if it takes ownership of the payload.
   typedef bool (*BCallbackFunc)(uint privateData, BEventPayload* pPayload);

   // Submits a callback to this helper thread.
   // Callable from any thread.
   void submitCallback(BThreadIndex threadIndex, BCallbackFunc pCallback, uint privateData = 0, BEventPayload* pPayload = NULL, bool waitForDelivery = false, bool synchronousDispatch = false);
   
   // This version of submitCallback() is very slow (thousands of cycles) due to operator new overhead!
   // pFunctor MUST persist until delivery!
   // If deleteFunctor is true, the functor will be deleted by calling BAlignedAlloc::Delete().
   //
   // Example:
   // void callback(const BString& c, uint i) { printf("Callback here! %u %s\n", i, c.getPtr()); }
   // typedef GF::Functor<void, TYPELIST_2(BString, uint)> MyFunctor;
   // MyFunctor myFunctor(&callback);
   // BEventDispatcher::BFunctor functor = GF::Bind<0>(myFunctor, BString("blah"));
   // gEventDispatcher.submitFunctor(cThreadIndexRender, functor, 100);
   typedef GF::Functor<void, TYPELIST_0()> BFunctor;
   void submitFunctor(BThreadIndex threadIndex, const BFunctor& functor, bool waitForDelivery = false, bool synchronousDispatch = false, bool deferIfFull = true);
   
   typedef GF::Functor<void, TYPELIST_1(void*)> BDataFunctor;
   void submitDataFunctor(BThreadIndex threadIndex, const BDataFunctor& functor, void* pPrivateData = 0, bool waitForDelivery = false, bool synchronousDispatch = false, bool deferIfFull = true);
      
   int pumpAndWait(DWORD timeToWait, DWORD timeBetweenPumps = 8, uint numHandles = 0, const HANDLE* pHandles = NULL, BEventReceiverHandle receiverHandle = cInvalidEventReceiverHandle);
   
   int pumpAndWaitSingle(DWORD timeToWait, DWORD timeBetweenPumps = 8, HANDLE handle = INVALID_HANDLE_VALUE, BEventReceiverHandle receiverHandle = cInvalidEventReceiverHandle)
   {  
      if (INVALID_HANDLE_VALUE != handle)
         return pumpAndWait(timeToWait, timeBetweenPumps, 1, &handle, receiverHandle);
      else
         return pumpAndWait(timeToWait, timeBetweenPumps, 0, NULL, receiverHandle);
   }
   
   int pumpThreadAndWait(BThreadIndex threadIndex, DWORD timeToWait, DWORD timeBetweenPumps = 8, HANDLE handle = INVALID_HANDLE_VALUE, bool pumpCurThread = true);
   
   int pumpAllThreads(DWORD timeToWait, DWORD timeBetweenPumps = 8, HANDLE handle = INVALID_HANDLE_VALUE);
   
   // Associates a Win-32 handle with an event. When the handle becomes signaled, the event will be sent.
   // The event can't have a payload!
   void registerHandleWithEvent(HANDLE handle, const BEvent& event, bool deferIfQueueFull = true);
   
   typedef void (*BHandleEventCallbackFunc)(uint privateData);
   void registerHandleWithCallback(HANDLE handle, BThreadIndex threadIndex, BHandleEventCallbackFunc pCallback, uint privateData = 0);
   
   // Removes an association between a Win-32 handle and an event. 
   void deregisterHandle(HANDLE handle, BThreadIndex threadIndex);
   
   // Requests the destination thread to stop and wait. This call may take a long time if the destination thread's FIFO is large.
   // After calling this method, you must first call haltThreadWait(), then resumeThread(), otherwise you'll leak memory and handles.
   BHandle haltThreadRequest(BThreadIndex threadIndex);
   // Waits for the destination thread to acknowledge that it's halted. You MUST call this before calling resumeThread.
   void haltThreadWait(BHandle handle);
   // Resumes the destination thread.
   void resumeThread(BHandle handle);
                  
   static BEventReceiverHandle setHandlePrivateData(BEventReceiverHandle handle, uint privateData);
   static uint getHandlePrivateData(BEventReceiverHandle handle);
   static BEventReceiverHandle removeHandlePrivateData(BEventReceiverHandle handle);
   static bool compareHandles(BEventReceiverHandle handle0, BEventReceiverHandle handle1);
                                          
private:
   BEventDispatcher(const BEventDispatcher&);
   BEventDispatcher& operator= (const BEventDispatcher&);
         
   DWORD mThreadID[cThreadIndexMax];
   
   uint mMaxThreadClients;
            
   struct BHandleSlot
   {
      BHandleSlot() : mCurUseCount(0), mpClient(NULL) { }

      BLightWeightMutex mMutex;
      
      DWORD mFlags;
      
      enum EHandleFlags
      {
         cHFPermitAsyncImmediateRemoval = 1,
      };
      
      uint mCurUseCount;
      
      BEventReceiverInterface* mpClient;
   };
   
   typedef BDynamicArray<BHandleSlot, ALIGN_OF(BHandleSlot), BDynamicArrayDefaultAllocator, BDynamicArrayNoGrowOptions> BHandleSlotArray;
        
   typedef BSynchronizedFIFO<BEvent> BEventQueue;
   
   class BHandleEvent
   {
   public:
      BHandleEvent() { }
      BHandleEvent(HANDLE handle, const BEvent& event, bool deferIfQueueFull) : 
         mHandle(handle), 
         mEvent(event), 
         mDeferIfQueueFull(deferIfQueueFull), 
         mpCallback(NULL) 
      { 
      }
      
      BHandleEvent(HANDLE handle, BHandleEventCallbackFunc pCallback, uint privateData) : 
         mHandle(handle), 
         mDeferIfQueueFull(false), 
         mpCallback(pCallback)
      { 
         mEvent.mPrivateData = privateData;
      }
      
      HANDLE mHandle;
      BEvent mEvent;
      BHandleEventCallbackFunc mpCallback;
      bool mDeferIfQueueFull;
   };
   
   class BRegisterEventWithHandlePayload : public BEventPayload, public BHandleEvent
   {
   public:
      BRegisterEventWithHandlePayload(HANDLE handle, const BEvent& event, bool deferIfQueueFull = true) : BHandleEvent(handle, event, deferIfQueueFull) { }
      BRegisterEventWithHandlePayload(HANDLE handle, BHandleEventCallbackFunc pCallback, uint privateData) : BHandleEvent(handle, pCallback, privateData) { }
      
      virtual void deleteThis(bool delivered) { delivered; delete this; }
   };
      
   struct BThreadData
   { 
      BEventReceiverHandle mDispatcherClientHandle;
      
      BCriticalSection     mHandleMutex;
      BHandleSlotArray     mHandleSlots;
      uint                 mLowestFreeHandleSlot;
      
      BWin32Event          mWakeupEvent;
      BEventQueue          mEvents;
      
      BQueue<BEvent>       mDeferredEvents;
                  
      uint                 mAsyncDispatchCount;
      uint                 mSyncDispatchCount;
      
      uint                 mMaxSyncDispatchCount;
      uint                 mTotalAsyncEvents;
      uint                 mTotalSyncEvents;
      
      BQueue<BEvent>       mSynchronousEvents;
           
      enum { cMaxHandleEvents = 8 };
      BHandleEvent         mHandleEvents[cMaxHandleEvents];
      uint                 mNumHandleEvents;
      uint                 mHandleEventRover;
      
      void clear(void);
   };
   
   BThreadData mThreadData[cMaxThreads];
         
   // TLS index of current thread
   static __declspec(thread) BThreadIndex mThreadIndex;
      
   bool mInitialized : 1;
   
   void clear(void);      
   void flushDeferredEvents(void);
   void dispatchEvent(const BEvent& event);
   BEventHandleStruct allocHandle(BThreadIndex threadIndex, bool permitAsyncRemoval);
   bool isValidHandle(BEventHandleStruct handle) const;
   void freeHandle(BEventHandleStruct handle);
   BEventReceiverHandle getDispatcherClientHandle(BThreadIndex threadIndex);
   bool pushEvent(BThreadIndex threadIndex, const BEvent& event, bool deferIfFull = true);
   void processHandleEvent(uint index);
   
   template<class FunctorType>
   class BFunctorCallbackTemplate : public BEventPayload
   {
      FunctorType mFunctor;
      
   public:
      BFunctorCallbackTemplate(const FunctorType& functor) :
         mFunctor(functor)
      {
      }      
      
      void execute(void)
      {
         mFunctor();
      }

      void execute(void* pPrivateData)
      {
         mFunctor(pPrivateData);
      }

      virtual void deleteThis(bool delivered)
      {
         delivered;
         BAlignedAlloc::Delete(this);            
      }
   };

   typedef BFunctorCallbackTemplate<BFunctor> BFunctorCallback;
   typedef BFunctorCallbackTemplate<BDataFunctor> BDataFunctorCallback;
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

extern BEventDispatcher gEventDispatcher;

//============================================================================
// class BEventReceiver
// Simple helper class to make creating new classes that receive events a bit easier.
//============================================================================
class BEventReceiver : public BEventReceiverInterface
{
public:
   inline BEventReceiver() : 
      mEventHandle(cInvalidEventReceiverHandle)
   {
   }
   
   inline void eventReceiverInit(BThreadIndex clientThreadIndex = cThreadIndexInvalid, bool permitAsyncImmediateRemoval = false)
   {
      if (mEventHandle == cInvalidEventReceiverHandle)
      {
         mEventHandle = gEventDispatcher.addClient(this, clientThreadIndex, permitAsyncImmediateRemoval);
      }
   }

   // If forceImmediateRemoval is true, removing the client will much faster if you are calling this method from the client's thread.
   // However, it won't receive cEventClassClientRemove event when immediately removed.
   inline void eventReceiverDeinit(bool forceImmediateRemoval = false)
   {
      if (mEventHandle == cInvalidEventReceiverHandle)
         return;
         
      // Check if client's thread is still valid - if not try to immediately remove the client, instead of sending an event and waiting for it to be received.
      const BThreadIndex handleThreadIndex = gEventDispatcher.getHandleThreadIndex(mEventHandle);
      if (!gEventDispatcher.getThreadId(handleThreadIndex))
      {
         trace("BEventReceiver::eventReceiverDeinit: Trying to remove a client from an exited thread! This will probably fail");
         
         forceImmediateRemoval = true;
      }
         
      if (forceImmediateRemoval)
         gEventDispatcher.removeClientImmediate(mEventHandle);
      else 
      {
         // Must wait because this method guarantees that the object will no longer receive messages once this method returns.
         gEventDispatcher.removeClientDeferred(mEventHandle, true);
      }
            
      mEventHandle = cInvalidEventReceiverHandle;
   }
   
   BEventReceiverHandle getEventHandle(void) const { return mEventHandle; }

protected:
   BEventReceiverHandle mEventHandle;
};
