//==============================================================================
// File: freelist.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#include "xcore.h"
#include "freelist.h"

uint gFreeListDummy;

#include "math\random.h"

#if FREELIST_TRACKING
void BFreeListTracker::getStatistics(BStats& stats, BFreeListDescArray* pFreeLists)
{
   getTrackerListMutex().lock();

   const BFreeListTrackerList& objectList = getTrackerList();

   stats.clear();
   if (pFreeLists)
      pFreeLists->clear();
   
   for (BFreeListTrackerList::constIterator it = objectList.begin(); it != objectList.end(); ++it)
   {
      const BFreeListTracker* ppTracker = *it;

      uint typeSize, groupSize, numAllocated, numAcquired, totalOperations;
      
      if (!ppTracker->getState(typeSize, groupSize, numAllocated, numAcquired, totalOperations))
         continue;

      stats.mTotalFreeLists++;
      stats.mTotalElementsAllocated += numAllocated;
      stats.mTotalElementsAcquired += numAcquired;
      stats.mTotalBytesAcquired += typeSize * numAcquired;
      stats.mTotalBytesAllocated += typeSize * numAllocated;
      
      if (pFreeLists)
      {
         BFreeListDesc* pDesc = pFreeLists->enlarge(1);
         pDesc->mpFreeList = ppTracker->getFreeList();
         pDesc->mTypeSize = typeSize;
         pDesc->mGroupSize = groupSize;
         pDesc->mNumAllocated = numAllocated;
         pDesc->mNumAcquired = numAcquired;
         pDesc->mTotalOperations = totalOperations;
      }         
   }

   getTrackerListMutex().unlock();
}
#endif // #if FREELIST_TRACKING

void freelistTest(void)
{
   Random rand;

   for ( ; ; )
   {
      BFreeList<uint64, 1, true> freelist;

      const uint numAllocations = rand.iRand(1, 1000000);
      BDynamicArray<uint> activeAllocs(numAllocations);
      BDynamicArray<uint> freeSlots;
      uint totalAllocs = 0;

      int freeRatio = 2;
      int allocRatio = 2;
      uint testIndex = 0;
      if (rand.iRand(0, 2) == 0)
      {
         for (uint i = 0; i < numAllocations; i++)
         {
            uint64* p = freelist.acquire(activeAllocs[i]);
            *p = i;         
            totalAllocs++;
         }
      }
      else
      {
         testIndex = 1;
         for (uint i = 0; i < numAllocations; i++)
         {
            freeSlots.pushBack(i);
            activeAllocs[i] = UINT_MAX;
         }
         freeRatio = 1000;
      }         

      for (uint q = 0; q < 100000; q++)
      {
         if ((q & 1023) == 1023)
         {
            printf("\nAllocated: %u, Free: %u\n", freelist.getNumberAllocated(), freelist.getNumberFree());
            freeRatio = rand.iRand(2, 5);
            allocRatio = rand.iRand(10, 30);
         }

         if (totalAllocs && (rand.iRand(0, freeRatio) == 0))
         {
            uint index = rand.iRand(0, activeAllocs.getSize());
            uint allocIndex = activeAllocs[index];
            if (allocIndex != UINT_MAX)
            {
               BVERIFY(freelist[allocIndex] == index);
               freelist[allocIndex] = 0xFEFEFEFE;
               freelist.release(allocIndex);

               activeAllocs[index] = UINT_MAX;
               totalAllocs--;

               freeSlots.pushBack(index);
            }
         }

         if ((freeSlots.getSize()) && (rand.iRand(0, allocRatio) == 0))
         {
            uint freeSlotIndex = rand.iRand(0, freeSlots.getSize());
            uint index = freeSlots[freeSlotIndex];
            freeSlots.removeIndex(freeSlotIndex, false);

            uint allocIndex;
            uint64* p = freelist.acquire(allocIndex);
            
            uint i;
            BVERIFY(freelist.getIndex(p, i));
            BVERIFY(i == allocIndex);

            if (testIndex == 0)
            {
               BVERIFY((*p == 0xFEFEFEFE) || (*p == 0));
            }

            *p = index;
            BVERIFY(activeAllocs[index] == UINT_MAX);
            activeAllocs[index] = allocIndex;
            totalAllocs++;
         }

         if (rand.fRand() < .01f)
         {
            BVERIFY(freelist.check());
            printf(".");
         }
      }

      BVERIFY(freelist.check());

      for (uint i = 0; i < freelist.getHighWaterMark(); i++)
         if (freelist.isValidIndex(i))
            freelist.release(i);

      BVERIFY(freelist.check());  
      printf("\n!!!!!!!!!!!!!!!!!\n");

#if FREELIST_TRACKING
      BFreeListTracker::BStats stats;
      BFreeListTracker::BFreeListDescArray freeLists;
      BFreeListTracker::getStatistics(stats, &freeLists);
      printf("Total FreeLists: %u\nTotal FreeList Bytes Allocated: %u\nTotal FreeList Bytes Acquired: %u\n", 
         stats.mTotalFreeLists,
         stats.mTotalBytesAllocated,
         stats.mTotalBytesAcquired);
#endif         
   }      
}   
