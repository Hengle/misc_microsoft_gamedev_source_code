//============================================================================
//
// File: helperThread.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "helperThread.h"
#include "workDistributor.h"

//============================================================================
// BHelperThread::BHelperThread
//============================================================================
BHelperThread::BHelperThread() :
   mInitialized(false),
   mDistributeWork(false),
   mThreadHandle(INVALID_HANDLE_VALUE),
   mEventHandle(cInvalidEventReceiverHandle),
   mThreadIndex(cThreadIndexInvalid),
   mpThreadFunc(NULL),
   mThreadFuncData(0),
   mpCallbackFunc(NULL),
   mCallbackData(0)
{
}

//============================================================================
// BHelperThread::~BHelperThread
//============================================================================
BHelperThread::~BHelperThread()
{
   if (INVALID_HANDLE_VALUE != mThreadHandle)
   {
      trace("BHelperThread::~BHelperThread: Helper thread was not properly deinitialized!");

      mTerminate.set();
            
      WaitForSingleObject(mThreadHandle, 30000);
   }
}

//============================================================================
// BHelperThread::init
//============================================================================
void BHelperThread::init(BThreadIndex threadIndex, uint processorIndex, const char* pName, BThreadFuncPtr pThreadFunc, uint threadFuncData, uint eventQueueSize, uint syncEventQueueSize, bool distributeWork) 
{
   BASSERT(!mInitialized);
   
   if (mInitialized)
      return;
      
   processorIndex;
   threadIndex;
         
   mInitialized = true;
   mDistributeWork = distributeWork;
   mThreadIndex = threadIndex;
   mpThreadFunc = pThreadFunc;
   mThreadFuncData = threadFuncData;
   
   mTerminate.reset();
   
   mEventHandle = gEventDispatcher.addClient(this, threadIndex); 
            
   mThreadHandle = gEventDispatcher.createThread(threadIndex, pName, eventQueueSize, syncEventQueueSize, NULL, 65536, workerThreadFunc, this, 0, NULL, processorIndex);
      
   if (NULL == mThreadHandle)
   {
      BFATAL_FAIL("_beginthread failed");
   }
}

//============================================================================
// BHelperThread::deinit
//============================================================================
void BHelperThread::deinit(void)
{
   if (!mInitialized)
      return;
      
   gEventDispatcher.removeClientDeferred(mEventHandle, true);
               
   int status = gEventDispatcher.pumpAndWaitSingle(16000, 4, mThreadHandle);
   status;
   
   CloseHandle(mThreadHandle);
   mThreadHandle = INVALID_HANDLE_VALUE;
         
   mInitialized = false;
}

//============================================================================
// BHelperThread::sync
//============================================================================
void BHelperThread::sync(void)
{
   BDEBUG_ASSERT(mEventHandle != cInvalidEventReceiverHandle);
   BDEBUG_ASSERT(mThreadIndex != cThreadIndexInvalid);
      
   if (mThreadIndex != cThreadIndexInvalid)
   {
      gEventDispatcher.sendSetAndWaitEvent(mThreadIndex, mSyncEvent.getHandle());
      gEventDispatcher.waitSingle(mSyncEvent.getHandle());
   }
}   

//============================================================================
// BHelperThread::invokeCallback
//============================================================================
void BHelperThread::invokeCallback(BThreadCallbackFuncPtr pCallbackFunc, void* pPrivateData)
{
   mCallbackMutex.lock();
   
   mpCallbackFunc = pCallbackFunc;
   mCallbackData = pPrivateData;
   
   mCallbackMutex.unlock();   
   
   mCallbackEvent.set();
}

//============================================================================
// BHelperThread::workerThreadFunc
//============================================================================
DWORD BHelperThread::workerThreadFunc(void* pData)
{
#ifdef XBOX
   __try
#endif   
   {
      BHelperThread* p = reinterpret_cast<BHelperThread*>(pData);
      
      if (p->mpThreadFunc)
         p->mpThreadFunc(p->mThreadFuncData);
      else
      {
         for ( ; ; )
         {
            HANDLE handles[2] = { p->mCallbackEvent.getHandle(), p->mTerminate.getHandle() };
                     
            int handleIndex;
            if (p->mDistributeWork)
               handleIndex = gWorkDistributor.wait(2, handles, INFINITE, 16, true, true);
            else
               handleIndex = gEventDispatcher.pumpAndWait(INFINITE, 16, 2, handles);
               
            if (handleIndex == 0)
            {
               BThreadCallbackFuncPtr  pCallbackFunc;
               void*                   callbackData;
               
               p->mCallbackMutex.lock();
               
               pCallbackFunc = p->mpCallbackFunc;
               callbackData = p->mCallbackData;
               
               p->mpCallbackFunc = NULL;
               p->mCallbackData = 0;
               
               p->mCallbackMutex.unlock();
               
               pCallbackFunc(callbackData);
            }
            else if (handleIndex == 1)
               break;
         }               
      }
   }
#ifdef XBOX   
   // Catch any assertion exceptions.
   __except(gAssertionSystem.xboxHandleAssertException(GetExceptionInformation()))
   {
   }
#endif   
         
   return 0;
}

//============================================================================
// BHelperThread::receiveEvent
//============================================================================
bool BHelperThread::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   threadIndex;
   
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         break;
      }
      case cEventClassClientRemove:
      {
         mTerminate.set();
         break;
      }
   }
            
   return false;
}
