//============================================================================
//
// File: simpleThreaderManager.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "synchronizedFIFO.h"

//-----------------------------------------------------------------------------
// class BSimpleThreadManager
//-----------------------------------------------------------------------------
class BSimpleThreadManager
{
public:
   BSimpleThreadManager();
   ~BSimpleThreadManager();
   
   bool init(uint numThreads);

   void setThreadProcessor(uint threadIndex, uint processorIndex);
   
   void deinit(void);
   
   typedef void (*BWorkFuncPtr)(void*, void*);

   // Queue some work.
   // false on failure (buffer full)
   // main thread only
   bool run(BWorkFuncPtr pWorkFunc, void* data0 = NULL, void* data1 = NULL, bool sleepIfFull = true);
   
   // Calls run num times, data1 is the count.
   // main thread only
   bool run(uint num, BWorkFuncPtr pWorkFunc, void* data0 = NULL, bool sleepIfFull = true);
   
   long getWorkRemaining(void) { return mNumWorkEntriesRemaining; }
   
   // Sleep or do work until all work is finished.
   // main thread only
   void sync(bool workOnThisThread = false, bool sleepUntilFinished = true);
   
   // Enters the global critical section. Safe to call from worker thread.
   void globalLock(void);

   // Leaves the block critical section. Safe to call from worker thread.
   void globalUnlock(void);
   
   int getRunMaxSize(void) const { return mWorkFIFO.getMaxSize(); }
   
   static void test(void);
   
private:
   uint mNumThreads;

   CRITICAL_SECTION mCriticalSection;
   bool mInCriticalSection;

   enum { MaxThreads = 32 };
   HANDLE mThreadHandles[MaxThreads];

   HANDLE mWakeupSemaphore;
   bool mHelperThreadExitFlag;

   volatile long mNumWorkEntriesRemaining;      
   
   struct BWorkEntry
   {
      BWorkFuncPtr mpWorkFunc;
      void* mWorkFuncData0;
      void* mWorkFuncData1;

      BWorkEntry() { }

      BWorkEntry(BWorkFuncPtr pWorkFunc, void* data0 = NULL, void* data1 = NULL) : 
         mpWorkFunc(pWorkFunc), 
         mWorkFuncData0(data0), 
         mWorkFuncData1(data1)
      {
      }
   };

   enum { cMaxWorkEntries = 512 };
   BSynchronizedFIFO<BWorkEntry> mWorkFIFO;

   static uint __stdcall helperThreadProc(void* pArguments);
   static void testWorkFunc(void* pData0, void* pData1);
};

extern BSimpleThreadManager gThreadManager;

namespace BSimpleThreadManagerTest
{
   void test(void);
}
