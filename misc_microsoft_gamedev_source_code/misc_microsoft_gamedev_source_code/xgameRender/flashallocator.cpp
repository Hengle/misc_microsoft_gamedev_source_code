//============================================================================
//
// File: flashallocator.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
// rg [1/26/08] - Important: ALL Scaleform allocations require 16 byte 
// alignment on 360, even when it doesn't actually call the aligned allocation
// functions!
//
//============================================================================
#include "xgamerender.h"
#include "scaleformIncludes.h"
#include "flashallocator.h"





//=============================================================================
// BMemoryHeapStats::sum
//=============================================================================
void BRenderGAllocatorHeapStats::sum(const BRenderGAllocatorHeapStats& d)
{
   mCurrentAllocations     += d.mCurrentAllocations;
   mCurrentAllocationSize  += d.mCurrentAllocationSize;
   mMostAllocations        += d.mMostAllocations;
   mMostAllocationSize     += d.mMostAllocationSize;
   mTotalAllocations       += d.mTotalAllocations;
   mTotalAllocationSize    += d.mTotalAllocationSize;
   mTotalNews              += d.mTotalNews;
   mTotalDeletes           += d.mTotalDeletes;
   mTotalReallocations     += d.mTotalReallocations; 
   mTotalLockTime          += d.mTotalLockTime;
   mTotalLocks             += d.mTotalLocks;

   for (uint i = 0; i < cMaxThreads; i++)
      mThreadHist[i] += d.mThreadHist[i];
}

//=============================================================================
// BRenderGAllocatorHeapStats::createDelta
//=============================================================================
void BRenderGAllocatorHeapStats::createDelta(const BRenderGAllocatorHeapStats& d)
{
   mCurrentAllocations     -= d.mCurrentAllocations;
   mCurrentAllocationSize  -= d.mCurrentAllocationSize;
   mMostAllocations        -= d.mMostAllocations;
   mMostAllocationSize     -= d.mMostAllocationSize;
   mTotalAllocations       -= d.mTotalAllocations;
   mTotalAllocationSize    -= d.mTotalAllocationSize;
   mTotalNews              -= d.mTotalNews;
   mTotalDeletes           -= d.mTotalDeletes;
   mTotalReallocations     -= d.mTotalReallocations; 
   mTotalLockTime          -= d.mTotalLockTime;
   mTotalLocks             -= d.mTotalLocks;

   for (uint i = 0; i < cMaxThreads; i++)
      mThreadHist[i] -= d.mThreadHist[i];
}     

//=============================================================================
// BRenderGAllocatorHeapStats::trackNew
//=============================================================================
void BRenderGAllocatorHeapStats::trackNew(int actualSize)
{
   mCurrentAllocations++;
   mCurrentAllocationSize += actualSize;

   mMostAllocations = max(mMostAllocations, mCurrentAllocations);
   mMostAllocationSize = max(mMostAllocationSize, mCurrentAllocationSize);

   mTotalAllocations++;
   mTotalAllocationSize += actualSize;

   mTotalNews++;
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}

//=============================================================================
// BRenderGAllocatorHeapStats::trackDelete
//=============================================================================
void BRenderGAllocatorHeapStats::trackDelete(int actualSize)
{
   mCurrentAllocations--;
   mCurrentAllocationSize -= actualSize;

   mTotalDeletes++;
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}

//=============================================================================
// BRenderGAllocatorHeapStats::trackRealloc
//=============================================================================
void BRenderGAllocatorHeapStats::trackRealloc(int origSize, int newSize)
{
   int delta = newSize - origSize;

   mCurrentAllocationSize += delta;
   mMostAllocationSize = max(mMostAllocationSize, mCurrentAllocationSize);

   mTotalReallocations++;  
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}



//============================================================================
// Globals
//============================================================================
BRenderGAllocator gRenderGAllocator;
BRenderGDebugAllocator gRenderGDebugAllocator;

//============================================================================
// InitGAllocator
//============================================================================
void InitGFXAllocator(void)
{
   GMemory::SetAllocator(&gRenderGAllocator);
   GMemory::SetDebugAllocator(&gRenderGDebugAllocator);
}

//============================================================================
// GetFlashAllocationStats
//============================================================================
void GetFlashAllocationStats(uint& numAllocs)
{
   numAllocs = gNumActiveFlashAllocs;
}

