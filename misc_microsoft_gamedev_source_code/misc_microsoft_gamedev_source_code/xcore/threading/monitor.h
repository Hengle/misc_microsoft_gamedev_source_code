//============================================================================
//
// File: monitor.h
// Copyright (c) 2007, Ensemble Studios
//
// Basic implementation of C. A. R. Hoare's Monitor
// http://www.cs.mtu.edu/~shene/NSF-3/e-Book/MONITOR/basics.html
//============================================================================
#pragma once
#include "win32Event.h"

//============================================================================
// class BMonitorLock
// This is modeled after:
// http://msdn2.microsoft.com/en-us/library/ms810428.aspx
//============================================================================
class BMonitorLock
{
   BMonitorLock(const BMonitorLock&);
   BMonitorLock& operator= (const BMonitorLock&);
   
public:
   BMonitorLock() :
      mSpinlock(FALSE),
      mAvailableCount(1),
      mNumWaitingThreads(0)
   {
   }
   
   void lock(DWORD maxSpinCount = 256) 
   {
      uint spinCount = 0;
            
      for ( ; ; )
      {
         acquireSpinlock();
         
         if (InterlockedCompareExchange(&mAvailableCount, 0, 1) == 1)
         {
            releaseSpinlock();
            break;
         }   
                                    
         spinCount++;
         
         if (spinCount >= maxSpinCount)
         {
            InterlockedIncrement(&mNumWaitingThreads);
                                    
            releaseSpinlock();
                                    
            mEvent.wait();
            
            break;
         }
         
         releaseSpinlock();

#ifdef XBOX         
         for (DWORD spin = 8; spin > 0; spin--)
         {
            YieldProcessor(); 
            __asm { or r0,r0,r0 } 
            YieldProcessor(); 
            __asm { or r1,r1,r1 }
         }               
#else
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();

         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
#endif         
         
      }
      
      MemoryBarrier();
   }
   
   void unlock(void)
   {
      acquireSpinlock();

      MemoryBarrier();
            
      if (mNumWaitingThreads)
      {
         mNumWaitingThreads--;

         mEvent.set();
      }
      else
      {
         InterlockedExchange(&mAvailableCount, 1);   
      }

      releaseSpinlock();
   }
   
   BOOL tryLock(void)
   {
      acquireSpinlock();

      if (InterlockedCompareExchange(&mAvailableCount, 0, 1) == 1)
      {
         releaseSpinlock();
         
         MemoryBarrier();
         
         return TRUE;
      }   
      
      releaseSpinlock();
      
      return FALSE;
   }
      
   BOOL isLocked(void) 
   { 
      acquireSpinlock();

      MemoryBarrier();
      
      const BOOL isLocked = (mAvailableCount == 0);
      
      releaseSpinlock();
      
      return isLocked;
   }

private:
   volatile long           mSpinlock;
   volatile long           mAvailableCount;
   volatile long           mNumWaitingThreads;
   BWin32Event             mEvent;
   
   void acquireSpinlock(void)
   {
      while (InterlockedExchange(&mSpinlock, TRUE))
      {
         YieldProcessor();
      }
   }
   
   void releaseSpinlock(void)
   {
      InterlockedExchange(&mSpinlock, FALSE);
   }
};

// Monitors are fast to construct, but slow to delete!
class BMonitor
{
   BMonitor(const BMonitor&);
   BMonitor& operator= (const BMonitor&);
   
public:
   BMonitor(uint spinCount = 256, bool useEventDispatcher = true);
   ~BMonitor();
   
   bool getUseEventDispatcher(void) const { return mUseEventDispatcher; }
   void setUseEventDispatcher(bool useEventDispatcher) { mUseEventDispatcher = useEventDispatcher; }
   
   // Enters (i.e. locks) the monitor. 
   // This is implemented as a spinlock that begins to sleep after a few thousand spins.
   // Recursive enters are supported.
   void enter(void);
   
   // Tries to enter the monitor. Returns TRUE if successful.
   // Never spinlocks or sleeps.
   // Recursive enters are supported.
   BOOL tryEnter(void);

   typedef BOOL (BTestConditionCallback)(void* pCallbackDataPtr, uint64 callbackData);
   
   // The callback will only be called when the monitor is entered.
   // This class only supports a maximum of cMaxWaitingThreads waiting threads!
   // Be very careful setting dispatchEvents to true!
   // Returns 0 if the condition was satisfied, or 1 or higher if one of the supplied handle(s) became signaled. In all cases, the monitor will be locked when this method returns.
   // Returns -1 on timeout.
   int waitForCondition(BTestConditionCallback* pCallback, void* pCallbackDataPtr, uint64 callbackData, uint numWaitHandles, const HANDLE* pWaitHandles, DWORD maxTimeToWait = INFINITE, bool dispatchEvents = false);
   
   int waitForCondition(BTestConditionCallback* pCallback, void* pCallbackDataPtr = NULL, uint64 callbackData = 0, HANDLE waitHandle = INVALID_HANDLE_VALUE, DWORD maxTimeToWait = INFINITE, bool dispatchEvents = false)
   {
      if (waitHandle != INVALID_HANDLE_VALUE)
         return waitForCondition(pCallback, pCallbackDataPtr, callbackData, 1, &waitHandle, maxTimeToWait, dispatchEvents);
      else
         return waitForCondition(pCallback, pCallbackDataPtr, callbackData, 0, NULL, maxTimeToWait, dispatchEvents);
   }
   
   void leave(void);
   
   // Returns the current thread's lock count. Only intended as a debug aid.
   DWORD getCurLockCount(void) const { return (DWORD)TlsGetValue(mTLS); }

private:
   BMonitorLock  mMonitorSpinlock;
   BMonitorLock  mWaiterSpinlock;
   
   DWORD mTLS;
   
   uint mRoverIndex;
   uint mSpinCount;
   bool mUseEventDispatcher;
      
   // Must be a power of two
   enum 
   { 
#ifdef XBOX   
      cMaxWaitingThreads = 8, 
#else
      cMaxWaitingThreads = 64, 
#endif      
      cMaxWaitingThreadsMask = cMaxWaitingThreads - 1 
   };
         
   struct BWaiter
   {
      uint64                     mCallbackData;
      void*                      mpCallbackDataPtr;
      BTestConditionCallback*    mpCallback;
      BOOL                       mSatisfied;
            
      BWin32Event                mEvent;
   };
   
   BWaiter mWaitList[cMaxWaitingThreads];
                  
   void leaveAndScanConditions(void);
      
   void setCurLockCount(DWORD newCount) { TlsSetValue(mTLS, (LPVOID)newCount); }
};

class BScopedMonitor
{
   BScopedMonitor(const BScopedMonitor&);
   BScopedMonitor& operator= (const BScopedMonitor&);
   
public:
   BScopedMonitor(BMonitor& monitor) : 
      mMonitor(monitor) 
   { 
      mMonitor.enter(); 
   }
   
   ~BScopedMonitor() 
   { 
      mMonitor.leave(); 
   }
   
private:
   BMonitor& mMonitor;   
};

