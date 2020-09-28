//============================================================================
//
// File: workDistributor.cpp
//
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "workDistributor.h"
#include "eventDispatcher.h"
#include "..\xsystem\timelineprofilersample.h"

//============================================================================
// Globals
//============================================================================
BWorkDistributor gWorkDistributor;

__declspec(thread) BWorkDistributor::BWorkBucket* BWorkDistributor::mpActiveBucket;
__declspec(thread) uint BWorkDistributor::mActiveBucketSize;

//============================================================================
// BWorkDistributor::BWorkDistributor
//============================================================================
BWorkDistributor::BWorkDistributor() :
   mInitialized(false),
   mPermitNewWork(false),
   mWorkBucketFIFO(cMaxWorkBuckets),
   mWorkBucketsAvailable(0, cMaxWorkBuckets)
{
}

//============================================================================
// BWorkDistributor::~BWorkDistributor
//============================================================================
BWorkDistributor::~BWorkDistributor()
{
}

//============================================================================
// BWorkDistributor::init
//============================================================================
void BWorkDistributor::init(void)
{
   if (mInitialized)
      return;

   MemoryBarrier();                 
   
   mWorkBucketAllocator.clear();
   
   mWorkBucketFIFO.clear();
   
   mInitialized = true;
   mPermitNewWork = true;
   
   MemoryBarrier();
}

//============================================================================
// BWorkDistributor::deinit
//============================================================================
void BWorkDistributor::deinit(void)
{
   if ((!mInitialized) || (!gEventDispatcher.getInitialized()))
      return;
   
   MemoryBarrier();     
         
   flush();
         
   // Prevent worker threads from creating new work.
   mPermitNewWork = false;      
   
   // Wait for any worker threads to finish up.
   gEventDispatcher.sleep(250);
   
   for ( ; ; )
   {
      if (mWorkBucketFIFO.getSize() == 0)
         break;
         
      wait(0, NULL, 4);
   }
      
   mInitialized = false; 
   
   MemoryBarrier();
}

//============================================================================
// BWorkDistributor::openBucket
//============================================================================
void BWorkDistributor::openBucket(void)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(!mpActiveBucket);
         
   for ( ; ; )
   {
      mpActiveBucket = mWorkBucketAllocator.alloc();
      if (mpActiveBucket)
         break;
      //gEventDispatcher.sleep(1);
      wait(0, NULL, 1);
   }
         
   mActiveBucketSize = 0;
}

//============================================================================
// BWorkDistributor::closeBucket
//============================================================================
void BWorkDistributor::closeBucket(void)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(mpActiveBucket);
         
   if ((!mActiveBucketSize) || (!mPermitNewWork))
   {
      mWorkBucketAllocator.free(mpActiveBucket);
   }
   else
   {
      if (mActiveBucketSize < cMaxWorkEntriesPerBucket)
         mpActiveBucket->mEntries[mActiveBucketSize].mpWorkFuncPtr = NULL;
         
      for ( ; ; )
      {
         if (mWorkBucketFIFO.pushFront(mpActiveBucket))
            break;
         //gEventDispatcher.sleep(1);
         wait(0, NULL, 1);
      }

      mWorkBucketsAvailable.release();
   }
         
   mpActiveBucket = NULL;
   mActiveBucketSize = 0;
}

//============================================================================
// BWorkDistributor::queue
//============================================================================
uint BWorkDistributor::queue(BWorkFuncPtr pWorkFuncPtr, void* privateData0, uint64 privateData1, int maxWorkEntriesInBucket)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(pWorkFuncPtr);
   if (!mPermitNewWork)
      return 0;
      
   if (!mpActiveBucket)
      openBucket();
   
   BDEBUG_ASSERT(mActiveBucketSize < cMaxWorkEntriesPerBucket);   
   
   const uint entryIndex = mActiveBucketSize;
   BWorkEntry& entry = mpActiveBucket->mEntries[mActiveBucketSize];
   entry.mpWorkFuncPtr = pWorkFuncPtr;
   entry.mpPrivateData0 = privateData0;
   entry.mPrivateData1 = privateData1;
   
   mActiveBucketSize++;  
   
   if ((int)mActiveBucketSize >= Math::Min<int>(maxWorkEntriesInBucket, cMaxWorkEntriesPerBucket))
      closeBucket();

   return entryIndex;      
}

//============================================================================
// BWorkDistributor::flush
//============================================================================
void BWorkDistributor::flush(void)
{
   if (!mInitialized)   
      return;
      
   if (mpActiveBucket)
      closeBucket();
}

//============================================================================
// BWorkDistributor::popWorkAndProcess
//============================================================================
void BWorkDistributor::popWorkAndProcess(void)
{
   if (!mInitialized)
      return;

   SCOPEDSAMPLE(BWorkDistributor_popWorkAndProcess); 
   
   BWorkBucket* pBucket;
   bool success = mWorkBucketFIFO.popBack(pBucket);   
   if (!success)
   {
      BFATAL_FAIL("BWorkDistributor::popWorkAndProcess: popBack() failed!");
   }
   
   for (uint i = 0; i < cMaxWorkEntriesPerBucket; i++)
   {
      if (pBucket->mEntries[i].mpWorkFuncPtr)
      {
         const bool lastWorkEntryInBucket = (i == (cMaxWorkEntriesPerBucket - 1)) || (NULL == pBucket->mEntries[i + 1].mpWorkFuncPtr);
         (*pBucket->mEntries[i].mpWorkFuncPtr)(pBucket->mEntries[i].mpPrivateData0, pBucket->mEntries[i].mPrivateData1, i, lastWorkEntryInBucket);
      }
      else
         break;
   }   
   
   mWorkBucketAllocator.free(pBucket);
}

//============================================================================
// BWorkDistributor::wait
//============================================================================
int BWorkDistributor::wait(uint numHandles, const HANDLE* pHandles, DWORD maxTimeToWait, DWORD timeBetweenPumps, bool dispatchEvents, bool pumpSynchronousEvents)
{
   if (!mInitialized)
      return gEventDispatcher.wait(numHandles, pHandles, maxTimeToWait, dispatchEvents);
            
   const uint cMaxHandles = 64;
   BDEBUG_ASSERT(numHandles < cMaxHandles);
   
   HANDLE handles[cMaxHandles];
   uint totalHandles = numHandles;
   
   if (pHandles)
      Utils::FastMemCpy(handles, pHandles, numHandles * sizeof(HANDLE));
   else
   {
      BDEBUG_ASSERT(numHandles == 0);
   }
   
   const uint workAvailableHandleIndex = totalHandles;
   handles[totalHandles] = mWorkBucketsAvailable.getHandle();
   totalHandles++;
   
   const DWORD startTime = GetTickCount();

   int returnStatus = -1;
   for ( ; ; )
   {
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
      
      int result;
      if (pumpSynchronousEvents)
      {
         BDEBUG_ASSERT(dispatchEvents);
         result = gEventDispatcher.pumpAndWait(timeToWait, timeBetweenPumps, totalHandles, handles);
      }
      else
      {
         result = gEventDispatcher.wait(totalHandles, handles, timeToWait, dispatchEvents);
      }
      
      if (result == static_cast<int>(workAvailableHandleIndex))
         popWorkAndProcess();
      else if ((result >= 0) && (result < static_cast<int>(numHandles)))
      {
         returnStatus = result;
         break;
      }
         
      if (maxTimeToWait == 0)
         break;
   }   
   
   return returnStatus;
}
