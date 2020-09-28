//============================================================================
//
// File: workDistributor.h
//
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "synchronizedFIFO.h"
#include "synchronizedBlockAllocator.h"
#include "win32Semaphore.h"

//============================================================================
// class BWorkDistributor
// This class assumes one or more worker threads are actively calling wait() 
// or getWorkAvailableSemaphore()/popWorkAndProcess().
//============================================================================
class BWorkDistributor
{
   BWorkDistributor(const BWorkDistributor&);
   BWorkDistributor& operator= (const BWorkDistributor&);
   
public:
   BWorkDistributor();
   ~BWorkDistributor();

   void init(void);
   void deinit(void);
   
   bool getInitialized(void) const { return mInitialized; }
   bool getPermittingNewWork(void) const { return mPermitNewWork; }

   // Any thread may call queue(). The work will be appended to a work bucket of size cMaxWorkEntriesPerBucket work entries.
   // The work function will not be called until after the current work bucket is flushed. queue() will automatically call flush  
   // when the current work buffer becomes full.
   typedef void (*BWorkFuncPtr)(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   uint queue(BWorkFuncPtr pWorkFuncPtr, void* privateData0 = NULL, uint64 privateData1 = 0, int maxWorkEntriesInBucket = INT_MAX);

   // Closes the current work bucket, if any.
   void flush(void);
               
   // Returns a semaphore which is released each time a work bucket is pushed into the work FIFO.
   BWin32Semaphore& getWorkAvailableSemaphore(void) { return mWorkBucketsAvailable; }
   
   // Pops and processes a work bucket. A wait must be satisfied on the work available semaphore before calling this method.
   void popWorkAndProcess(void);
   
   // wait() is like gEventDispatcher.wait(), except this method processes work buckets as they become available.
   int wait(
      uint numHandles = 0, 
      const HANDLE* pHandles = NULL, 
      DWORD maxTimeToWait = INFINITE, 
      DWORD timeBetweenPumps = 8, 
      bool dispatchEvents = true, 
      bool pumpSynchronousEvents = false);
      
   int flushAndWait(
      uint numHandles = 0, 
      const HANDLE* pHandles = NULL, 
      DWORD maxTimeToWait = INFINITE, 
      DWORD timeBetweenPumps = 8, 
      bool dispatchEvents = true, 
      bool pumpSynchronousEvents = false)
   {
      flush();
      return wait(numHandles, pHandles, maxTimeToWait, timeBetweenPumps, dispatchEvents, pumpSynchronousEvents);
   }      
      
   int waitSingle(
      HANDLE handle, 
      DWORD maxTimeToWait = INFINITE, DWORD timeBetweenPumps = 8, 
      bool dispatchEvents = true, bool pumpSynchronousEvents = false)
   {
      if (INVALID_HANDLE_VALUE == handle)
         return wait(0, NULL, maxTimeToWait, timeBetweenPumps, dispatchEvents, pumpSynchronousEvents);
      else
         return wait(1, &handle, maxTimeToWait, timeBetweenPumps, dispatchEvents, pumpSynchronousEvents);
   }
   
   int flushAndWaitSingle(
      HANDLE handle, 
      DWORD maxTimeToWait = INFINITE, DWORD timeBetweenPumps = 8, 
      bool dispatchEvents = true, bool pumpSynchronousEvents = false)
   {
      flush();
      return waitSingle(handle, maxTimeToWait, timeBetweenPumps, dispatchEvents, pumpSynchronousEvents);
   }    
      
   static uint getWorkEntryBucketSize(void) { return cMaxWorkEntriesPerBucket; }
   static uint getWorkEntryBucketSizeLog2(void) { return cMaxWorkEntriesPerBucketLog2; }

private:
   enum 
   { 
      cMaxWorkEntriesPerBucketLog2 = 4, 
      cMaxWorkEntriesPerBucket = 1 << cMaxWorkEntriesPerBucketLog2 
   };

   struct BWorkEntry
   {
      BWorkFuncPtr mpWorkFuncPtr;
      void* mpPrivateData0;
      uint64 mPrivateData1;
   };

   struct BWorkBucket
   {
      BWorkEntry mEntries[cMaxWorkEntriesPerBucket];
   };

   enum { cMaxWorkBuckets = 2048 };

   typedef BSynchronizedBlockAllocator<BWorkBucket, cMaxWorkBuckets, false> BWorkBucketAllocator;
   BWorkBucketAllocator mWorkBucketAllocator;

   typedef BSynchronizedFIFO<BWorkBucket*> BWorkBucketFIFO;
   BWorkBucketFIFO mWorkBucketFIFO;

   BWin32Semaphore mWorkBucketsAvailable;
         
   bool mInitialized : 1;
   bool mPermitNewWork : 1;
   
   static __declspec(thread) BWorkBucket* mpActiveBucket;
   static __declspec(thread) uint mActiveBucketSize;
   
   void openBucket(void);
   void closeBucket(void);
};

//============================================================================
// Externs
//============================================================================
extern BWorkDistributor gWorkDistributor;
