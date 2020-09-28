//============================================================================
//
// File: simpleThreadManager.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
// This is only used in tools.
//
//============================================================================
#include "xcore.h"
#include "simpleThreadManager.h"

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
BSimpleThreadManager gThreadManager;
      
//-----------------------------------------------------------------------------
// BSimpleThreadManager::BSimpleThreadManager
//-----------------------------------------------------------------------------
BSimpleThreadManager::BSimpleThreadManager(void) : 
   mNumThreads(0),
   mInCriticalSection(false),
   mWakeupSemaphore(INVALID_HANDLE_VALUE),
   mHelperThreadExitFlag(false),
   mNumWorkEntriesRemaining(0),
   mWorkFIFO(cMaxWorkEntries)
{
   std::fill(mThreadHandles, mThreadHandles + MaxThreads, INVALID_HANDLE_VALUE);
   
   InitializeCriticalSection(&mCriticalSection);
   
   mWakeupSemaphore = CreateSemaphore(NULL, 0, cMaxWorkEntries * 2, NULL);         
   BVERIFY(mWakeupSemaphore);
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::init
//-----------------------------------------------------------------------------                  
bool BSimpleThreadManager::init(uint numThreads)
{
   if (numThreads >= MaxThreads)
      return false;
   
   deinit();
   
   mNumThreads = numThreads;
         
   for (uint i = 0; i < mNumThreads; i++)
   {
      uint threadID = 0;
      mThreadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, &helperThreadProc, this, 0, &threadID);
      if (NULL == mThreadHandles[i])
      {
         mNumThreads = i;
         
         deinit();
         
         mNumThreads = 0;
            
         return false;
      }
   }
   
   return true;
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::setThreadProcessing
//-----------------------------------------------------------------------------
void BSimpleThreadManager::setThreadProcessor(uint threadIndex, uint processorIndex)
{
   processorIndex;
   
   if (threadIndex >= mNumThreads)
      return;

#ifdef XBOX
   BDEBUG_ASSERT(processorIndex < 6);
   XSetThreadProcessor(mThreadHandles[threadIndex], processorIndex);
#endif     
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::deinit
//-----------------------------------------------------------------------------
void BSimpleThreadManager::deinit(void)
{
   mWorkFIFO.clear();
   
   mHelperThreadExitFlag = true;

   uint numThreadsStopped = 0;
   while (numThreadsStopped != mNumThreads)
   {
      ReleaseSemaphore(mWakeupSemaphore, mNumThreads, NULL);
      Sleep(10);

      for (uint i = 0; i < mNumThreads; i++)
      {
         if ((mThreadHandles[i] != INVALID_HANDLE_VALUE) && 
               (WAIT_OBJECT_0 == WaitForSingleObject(mThreadHandles[i], 0)))
         {
            CloseHandle(mThreadHandles[i]);
            mThreadHandles[i] = INVALID_HANDLE_VALUE;
            numThreadsStopped++;
         }                  
      }
   }

   mHelperThreadExitFlag = false;
   
   mNumThreads = 0;
   mNumWorkEntriesRemaining = 0;
}
                  
//-----------------------------------------------------------------------------
// BSimpleThreadManager::~BSimpleThreadManager
//-----------------------------------------------------------------------------                  
BSimpleThreadManager::~BSimpleThreadManager()
{
   deinit();
               
   DeleteCriticalSection(&mCriticalSection);

   if (INVALID_HANDLE_VALUE != mWakeupSemaphore)
   {
      CloseHandle(mWakeupSemaphore);
      mWakeupSemaphore = NULL;
   }
}
         
//-----------------------------------------------------------------------------
// BSimpleThreadManager::run
//-----------------------------------------------------------------------------
bool BSimpleThreadManager::run(BWorkFuncPtr pWorkFunc, void* data0, void* data1, bool sleepIfFull)
{
   if (0 == mNumThreads)
   {
      pWorkFunc(data0, data1);
      return true;
   }
   
   BWorkEntry workEntry(pWorkFunc, data0, data1);

   // rg [12/31/05] - This should probably be done using a semaphore to track the # of slots left in the FIFO.
   for ( ; ; )
   {
      const bool success = mWorkFIFO.pushBack(workEntry);
      if (success)
      {
         InterlockedIncrement(&mNumWorkEntriesRemaining);
         
         ReleaseSemaphore(mWakeupSemaphore, 1, NULL);
         
         break;
      }
      else
      {
         if (!sleepIfFull)
            return false;
            
         // An event and waitforsingleobject could be used here to make the wait more efficient.
         Sleep(1);
      }
   }      
      
   return true;
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::run
//-----------------------------------------------------------------------------
bool BSimpleThreadManager::run(uint num, BWorkFuncPtr pWorkFunc, void* data0, bool sleepIfFull)
{
   if (static_cast<int>(num) > mWorkFIFO.getMaxSize())
      return false;
      
   for (uint i = 0; i < num; i++)
   {
      bool success = run(pWorkFunc, data0, (void*)i, sleepIfFull);
      if (!success)
         return false;
   }

   return true;
}
      
//-----------------------------------------------------------------------------
// BSimpleThreadManager::sync
//-----------------------------------------------------------------------------
void BSimpleThreadManager::sync(bool workOnThisThread, bool sleepUntilFinished)
{
   if (workOnThisThread)
   {
      for ( ; ; )
      {
         BWorkEntry workEntry;
         const bool success = mWorkFIFO.popFront(workEntry);
         if (!success)
            break;
         
         workEntry.mpWorkFunc(workEntry.mWorkFuncData0, workEntry.mWorkFuncData1);
         
         InterlockedDecrement(&mNumWorkEntriesRemaining);
      }
   }
         
   if (sleepUntilFinished)
   {
      while (mNumWorkEntriesRemaining > 0)
      {
         Sleep(2);
      }
   }
}
   
//-----------------------------------------------------------------------------
// BSimpleThreadManager::globalLock
//-----------------------------------------------------------------------------
void BSimpleThreadManager::globalLock(void)
{
   if (!mNumThreads)
      return;
      
   EnterCriticalSection(&mCriticalSection);
   
   BVERIFY(!mInCriticalSection);
   
   mInCriticalSection = true;
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::globalUnlock
//-----------------------------------------------------------------------------      
void BSimpleThreadManager::globalUnlock(void)
{
   if (!mNumThreads)
      return;
   
   BVERIFY(mInCriticalSection);
   
   mInCriticalSection = false;
   
   LeaveCriticalSection(&mCriticalSection);
}

//-----------------------------------------------------------------------------
// BSimpleThreadManager::helperThreadProc
//-----------------------------------------------------------------------------                                 
uint __stdcall BSimpleThreadManager::helperThreadProc(void* pArguments)
{
   BSimpleThreadManager* pThreadManager = reinterpret_cast<BSimpleThreadManager*>(pArguments);
   
   for ( ; ; )
   {
      WaitForSingleObject(pThreadManager->mWakeupSemaphore, INFINITE);
      
      if (pThreadManager->mHelperThreadExitFlag)
         break;
      
      BWorkEntry workEntry;
      
      const bool success = pThreadManager->mWorkFIFO.popFront(workEntry);
      if (success)
      {
         workEntry.mpWorkFunc(workEntry.mWorkFuncData0, workEntry.mWorkFuncData1);
         
         InterlockedDecrement(&pThreadManager->mNumWorkEntriesRemaining);
      }
   }
   
   return 0;
}

void BSimpleThreadManager::testWorkFunc(void* pData0, void* pData1)
{
   DWORD x = 0;   
   for (DWORD i = 0; i < 100000; i++)
      //for (DWORD i = 0; i < 10000000; i++)
      x += i;

   gThreadManager.globalLock();
   printf("workFunc: %i %i %i %X\n", pData0, pData1, GetTickCount(), x);      
   gThreadManager.globalUnlock();

   //Sleep(10);

   bool* pDoneFlags = reinterpret_cast<bool*>(pData0);
   pDoneFlags[(DWORD)pData1] = true;
}

void BSimpleThreadManager::test(void)
{
   for (int t = 0; t < 10000; t++)
   {
      int numThreads = rand() % 8;
      int numWorkUnits = (rand() % 200) + 1;

      printf("Threads: %i, Work Units: %i\n", numThreads, numWorkUnits);
      
      gThreadManager.init(numThreads);

      DWORD s = GetTickCount();
      
      bool doneFlags[1024];
      Utils::ClearObj(doneFlags);

      bool success = gThreadManager.run(numWorkUnits, testWorkFunc, doneFlags);
      success;
      
      gThreadManager.sync(true);

      DWORD e = GetTickCount();

      printf("Total time: %i\n", e - s);

      for (int i = 0; i < numWorkUnits; i++)
      {
         BVERIFY(doneFlags[i]);
      }

      gThreadManager.deinit();
   }      
}      
