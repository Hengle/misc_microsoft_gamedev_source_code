//============================================================================
//
// File: helperThread.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "threading\eventDispatcher.h"

//============================================================================
// class BHelperThread
//============================================================================
class BHelperThread : public BEventReceiverInterface
{
   BHelperThread(const BHelperThread&);
   BHelperThread& operator= (const BHelperThread&);

public:
   BHelperThread();
   ~BHelperThread();

   typedef void (*BThreadFuncPtr)(uint);

   // Main thread
   void init(
      BThreadIndex threadIndex, 
      uint processorIndex, 
      const char* pName = NULL, 
      BThreadFuncPtr pThreadFunc = NULL, 
      uint threadFuncData = 0, 
      uint eventQueueSize = BEventDispatcher::cDefaultEventQueueSize, 
      uint syncEventQueueSize = BEventDispatcher::cDefaultSyncEventQueueSize,
      bool distributeWork = false);
      
   void deinit(void);
         
   // Inserts a fence into this helper thread's event queue, then waits for it to be passed.
   // Note: It's faster if you insert the event into the buffer yourself, right after sending off some work. Then wait on this event later.
   void sync(void);
   
   // Important: invokeCallback() does not use a queue! 
   // It causes the worker thread to wake up and call your callback as quickly as possible.
   typedef void (*BThreadCallbackFuncPtr)(void* pPrivateData);
   void invokeCallback(BThreadCallbackFuncPtr pCallbackFunc, void* pPrivateData);
   
   HANDLE getThreadHandle(void) const { return mThreadHandle; }
   BEventReceiverHandle getEventHandle(void) const { return mEventHandle; }
   BThreadIndex getThreadIndex(void) const { return mThreadIndex; }
   
   BWin32Event& getTerminateEvent(void) { return mTerminate; }
   BWin32Event& getSyncEvent(void) { return mSyncEvent; }
   
   bool getInitialized(void) const { return mInitialized; }

private:
   HANDLE                  mThreadHandle;
   BEventReceiverHandle    mEventHandle;
   BWin32Event             mTerminate;
   BThreadIndex            mThreadIndex;
   
   BThreadFuncPtr          mpThreadFunc;
   uint                    mThreadFuncData;
   
   BWin32Event             mSyncEvent;
   
   BCriticalSection        mCallbackMutex;
   BWin32Event             mCallbackEvent;
   BThreadCallbackFuncPtr  mpCallbackFunc;
   void*                   mCallbackData;
              
   bool                    mInitialized : 1;
   bool                    mDistributeWork : 1;
      
   static DWORD __stdcall workerThreadFunc(void* pData);   
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

