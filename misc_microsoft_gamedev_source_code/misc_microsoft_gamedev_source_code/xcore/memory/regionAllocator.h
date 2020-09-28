//============================================================================
//
//  File: regionAllocator.h
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "containers\segmentedArray.h"

//============================================================================
// class BRegionAllocator
//============================================================================
class BRegionAllocator
{
public:
   BRegionAllocator(const BRegionAllocator&);
   BRegionAllocator& operator=(const BRegionAllocator&);

public:
   BRegionAllocator();
   
   void clear();
   
   struct BReservedRegion
   {
      BReservedRegion() { }
      BReservedRegion(uint startOfs, uint endOfs) : mStartOfs(startOfs), mEndOfs(endOfs) { }
      
      void set(uint startOfs, uint endOfs) { mStartOfs = startOfs; mEndOfs = endOfs; }
      
      uint mStartOfs;   // inclusive
      uint mEndOfs;     // exclusive
   };

   bool init(void* pStart, uint size, const BReservedRegion* pReservedRegions, uint numReservedRegions);

   void getAllocStats(uint& totalAllocatedBlocks, uint& totalAllocatedBytes, uint& totalFreeBytes) const;
   bool getLargestFreeRegion(uint& largestFreeRegion) const;
      
   void* alloc(uint size, uint* pActualSize = NULL);
   bool free(void* p);
   uint getSize(void* p) const;

   bool check(void) const;
   
   uint getNumRegions() const { return mRegions.getSize(); }
   uint getRegionSize(uint index) const { return mRegions[index] & cSizeMask; }
   bool isRegionUsed(uint index) const { return (mRegions[index] & (cFreeFlag | cReservedFlag)) == 0; }
   bool isRegionFree(uint index) const { return (mRegions[index] & cFreeFlag) != 0; }
   bool isRegionReserved(uint index) const { return (mRegions[index] & cReservedFlag) != 0; }
   
   enum 
   { 
      cFreeFlag      = 0x80000000,
      cReservedFlag  = 0x40000000, 
      cSizeMask      = 0x3FFFFFFF 
   };
   
   const BDynamicArray<uint>& getRegions(void) const { return mRegions; }

private:
   void* mpStart;
   uint mSize;   
   uint mAvail;
   //BSegmentedArray<uint, 4> mRegions;
   BDynamicArray<uint> mRegions;
   
   uint mTotalAllocatedBlocks;
   uint mTotalAllocatedBytes;
   uint mTotalFreeBytes;
   
   void* getEnd(void) const { return static_cast<uchar*>(mpStart) + mSize; }
   bool isFreeRegion(uint index) const { return (mRegions[index] & cFreeFlag) != 0; }
   bool isReservedRegion(uint index) const { return (mRegions[index] & cReservedFlag) != 0; }
   bool isUsedRegion(uint index) const { return (mRegions[index] & (cFreeFlag | cReservedFlag)) == 0; }
   void setRegionUsed(uint index) { mRegions[index] &= ~cFreeFlag; }
   void setRegionFree(uint index) { mRegions[index] |= cFreeFlag; }   
         
   int locateRegion(uint ofs, uint* pStartOfs = NULL) const;
   bool carveReservedRegion(uint startOfs, uint size);
};
