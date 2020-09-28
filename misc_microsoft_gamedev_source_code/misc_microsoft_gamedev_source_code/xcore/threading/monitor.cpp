//============================================================================
//
// File: monitor.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "monitor.h"
#include "threading\eventDispatcher.h"

//============================================================================
// BMonitor::BMonitor
//============================================================================
BMonitor::BMonitor(uint spinCount, bool useEventDispatcher) :
   mRoverIndex(0),
   mSpinCount(spinCount),
   mTLS(TlsAlloc()),
   mUseEventDispatcher(useEventDispatcher)
{
   BVERIFY(TLS_OUT_OF_INDEXES != mTLS);
      
   mWaiterSpinlock.lock();
   
   for (uint i = 0; i < cMaxWaitingThreads; i++)
   {
      mWaitList[i].mpCallback          = NULL;
      mWaitList[i].mpCallbackDataPtr   = NULL;
      mWaitList[i].mCallbackData       = 0;
      mWaitList[i].mSatisfied          = FALSE;
   }
   
   mWaiterSpinlock.unlock();
}

//============================================================================
// BMonitor::~BMonitor
//============================================================================
BMonitor::~BMonitor()
{
   if (getCurLockCount())
   {
      trace("BMonitor::~BMonitor: Deleting an entered monitor!");
      
      setCurLockCount(0);
      
      mMonitorSpinlock.unlock();
   }
   
   if (mMonitorSpinlock.isLocked())
   {
      trace("BMonitor::~BMonitor: Deleting a monitor currently entered by another thread!");
      
      // Really, this shouldn't happen (right)
      do
      {  
         Sleep(1);
      } while (mMonitorSpinlock.isLocked());
   }
   
   const uint cMaxT = 8;
   for (uint t = 0; t < cMaxT; t++)
   {
      mWaiterSpinlock.lock();
      
      uint numWaiters = 0;
      for (uint i = 0; i < cMaxWaitingThreads; i++)
         if (mWaitList[i].mpCallback)
            numWaiters++;      
      
      mWaiterSpinlock.unlock();
      
      if (!numWaiters)
         break;
      
      trace("BMonitor::~BMonitor: Sleeping on a monitor with active waiters!");    
      
      Sleep(1000);
   }      
   
   mWaiterSpinlock.lock();
   
   uint numWaiters = 0;
   for (uint i = 0; i < cMaxWaitingThreads; i++)
      if (mWaitList[i].mpCallback)
         numWaiters++;    
   
   mWaiterSpinlock.unlock();
              
   if (numWaiters)         
      trace("BMonitor::~BMonitor: Deleting a monitor with dead waiters!");    
   
   TlsFree(mTLS);
}

//============================================================================
// BMonitor::enter
//============================================================================
void BMonitor::enter(void) 
{ 
   DWORD curCount = getCurLockCount();
   
   BDEBUG_ASSERT(curCount != 0xFFFFFFFF);
   
   curCount++;
   setCurLockCount(curCount);
   
   if (1 == curCount)
   {
      mMonitorSpinlock.lock(mSpinCount); 
   }
}

//============================================================================
// BMonitor::tryEnter
//============================================================================
BOOL BMonitor::tryEnter(void) 
{ 
   DWORD curCount = getCurLockCount();
   
   if (!curCount)
   {
      if (!mMonitorSpinlock.tryLock())
         return FALSE;
   }
   
   BDEBUG_ASSERT(curCount != 0xFFFFFFFF);
   
   curCount++;
   setCurLockCount(curCount);
   
   return TRUE;
}

//============================================================================
// BMonitor::leave
//============================================================================
void BMonitor::leave(void) 
{ 
   DWORD curCount = getCurLockCount();
   BDEBUG_ASSERT(curCount);
   curCount--;
   setCurLockCount(curCount);
   
   if (!curCount)
      leaveAndScanConditions(); 
}

//============================================================================
// BMonitor::waitForCondition
//============================================================================
int BMonitor::waitForCondition(BTestConditionCallback* pCallback, void* pCallbackDataPtr, uint64 callbackData, uint numWaitHandles, const HANDLE* pWaitHandles, DWORD maxTimeToWait, bool dispatchEvents)
{
   BDEBUG_ASSERT(pCallback);
   BDEBUG_ASSERT(getCurLockCount());
   
   if (pCallback(pCallbackDataPtr, callbackData))
      return 0;
   
   mWaiterSpinlock.lock();
      
   uint i;
   for (i = 0; i < cMaxWaitingThreads; i++)
      if (!mWaitList[i].mpCallback)
         break;
   BVERIFY(i < cMaxWaitingThreads);
   
   BWaiter& waiter = mWaitList[i];
   
   waiter.mpCallback          = pCallback;
   waiter.mpCallbackDataPtr   = pCallbackDataPtr;
   waiter.mCallbackData       = callbackData;
   waiter.mSatisfied          = FALSE;
   waiter.mEvent.reset();
   
   mWaiterSpinlock.unlock();
            
   leaveAndScanConditions();

   int returnIndex = 0;
   
   const uint cMaxWaitHandles = 64;
   BVERIFY(numWaitHandles < cMaxWaitHandles);
   
   HANDLE handles[cMaxWaitHandles];
   uint totalHandles = 1;
   handles[0] = waiter.mEvent.getHandle();
   
   if (numWaitHandles)
   {
      BDEBUG_ASSERT(pWaitHandles);
      memcpy(handles + totalHandles, pWaitHandles, sizeof(HANDLE) * numWaitHandles);
      totalHandles += numWaitHandles;
   }
               
   if (mUseEventDispatcher)
   {
      returnIndex = gEventDispatcher.wait(totalHandles, handles, maxTimeToWait, dispatchEvents);

#if 0      
      if (pWaitHandles)
      {
         HANDLE handles[2] = { waitHandle, waiter.mEvent.getHandle() };
         const int handleIndex = gEventDispatcher.wait(2, handles, maxTimeToWait, dispatchEvents);
            
         if (-1 == handleIndex)
            returnIndex = -1;
         else if (0 == handleIndex)
            returnIndex = 1;
      }
      else
      {
         if (gEventDispatcher.waitSingle(waiter.mEvent, maxTimeToWait, dispatchEvents) == -1)
            returnIndex = -1;
      }  
#endif      
   }
   else
   {
      DWORD result = WaitForMultipleObjects(totalHandles, handles, FALSE, maxTimeToWait);
      if ((result == WAIT_ABANDONED) || (result == WAIT_TIMEOUT) || (result == WAIT_FAILED))
         returnIndex = -1;
      else 
         returnIndex = result - WAIT_OBJECT_0;
   }      
   
   mWaiterSpinlock.lock();

   const BOOL wasSatisfied = waiter.mSatisfied;                          

   waiter.mpCallback = NULL;

   mWaiterSpinlock.unlock();

   if (0 == returnIndex)
   {
      BDEBUG_ASSERT(wasSatisfied);  
   }
   else
   {
      if (!wasSatisfied)
         mMonitorSpinlock.lock();
   }
            
   return returnIndex;                
}

//============================================================================
// BMonitor::leaveAndScanConditions
//============================================================================
void BMonitor::leaveAndScanConditions(void)
{
   mWaiterSpinlock.lock();
   
   mRoverIndex++;

   for (uint i = 0; i < cMaxWaitingThreads; i++)
   {
      BWaiter& waiter = mWaitList[(mRoverIndex + i) & cMaxWaitingThreadsMask];
      
      if ((waiter.mpCallback) && (!waiter.mSatisfied))
      {
         if (waiter.mpCallback(waiter.mpCallbackDataPtr, waiter.mCallbackData))
         {
            waiter.mSatisfied = TRUE;

            waiter.mEvent.set();
            
            mWaiterSpinlock.unlock();
            
            return;      
         }
      }            
   }         
      
   mWaiterSpinlock.unlock();
   
   mMonitorSpinlock.unlock();
}
