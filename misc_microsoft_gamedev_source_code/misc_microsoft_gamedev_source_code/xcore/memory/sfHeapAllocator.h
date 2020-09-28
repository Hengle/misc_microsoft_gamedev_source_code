//============================================================================
//
//  sfHeapAllocator.h
//
//  Copyright (c) 2008, Ensemble Studios
//
//  rg [1/20/08] - This is a straightforward segregated fits heap allocator.
//  It's per-allocated block overhead seems too high for practical use.
//  It's been throughly tested. The number of bins is too high, and the bin
//  sizes need some tweaking. Also, the bin search should be tweaked to first
//  check the first available block, then only resorting to a binary search.
//
//============================================================================
#pragma once
#include "containers\hashMap.h"
#include "containers\segmentedArray.h"

//----------------------------------------------------------------------------
//  Class BSFHeapAllocator
//----------------------------------------------------------------------------
class BSFHeapAllocator
{
   BSFHeapAllocator(const BSFHeapAllocator&);
   BSFHeapAllocator& operator=(const BSFHeapAllocator&);
   
public:
   BSFHeapAllocator();
   ~BSFHeapAllocator();
   
   void clear();
   
   void addRegion(void* pRegion, uint regionSize);
         
   void getAllocInfo(uint& totalNumAllocations, uint& totalBytesAllocated, uint& totalBytesFree) const;
   void getMemoryInfo(uint& totalFreeChunks, uint& largestFreeChunk, uint& totalBins, uint& totalOverhead) const;

   enum 
   { 
      cAllocAlignmentShift = 4U, 
      cAllocAlignment = 1U << cAllocAlignmentShift 
   };
   
   void* alloc(uint size, uint* pActualSize = NULL);
   void free(void* p);
   uint getSize(void* p) const;
   
   bool check(void) const;
   
private:
   struct BChunk
   {
      BChunk() { }
      BChunk(void* p, uint size) : mPtr(p), mSize(size) { }
      
      void* mPtr;
      uint mSize;
   };
   
   typedef BSegmentedArray<BChunk, 4> BChunkArray;
   
   struct BBin 
   {
      BChunkArray mChunks;
   };

   enum { cNumBins = 256 };   
   
   typedef uint BBinIndex;
   typedef BHashMap<BBinIndex, BBin> BBinHashMap;
   BBinHashMap mBins;
   uint mLowestUsedBinIndex;
      
   typedef BHashMap<uint, uint> BPtrSizeHashMap;
   
   BPtrSizeHashMap mAllocatedChunks;
   BPtrSizeHashMap mFreeChunks;
      
   uint mTotalBytesAllocated;
   uint mTotalBytesFree;
   uint mTotalRegionBytes;
   uint mOperationIndex;

   void eraseFreeChunk(void* pChunk, uint chunkSize);
   void addFreeChunk(void* pChunk, uint chunkSize);
   void coaleseFreeChunk(void*& pChunk, uint& chunkSize);
   BBinIndex getBinIndex(uint size) const;
};
