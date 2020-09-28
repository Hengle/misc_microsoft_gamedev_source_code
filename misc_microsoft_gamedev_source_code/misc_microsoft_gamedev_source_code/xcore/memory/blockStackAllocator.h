//============================================================================
//
//  File: blockStackAllocator.h
//
//  Copyright (c) 2008, Ensemble Studios
// 
//============================================================================
#include "containers\segmentedArray.h"

//============================================================================
// class BBlockStackAllocator
//============================================================================
class BBlockStackAllocator
{
   BBlockStackAllocator(const BBlockStackAllocator&);
   BBlockStackAllocator& operator= (const BBlockStackAllocator&);
public:
   BBlockStackAllocator();
   ~BBlockStackAllocator();
   
   bool init(BMemoryHeap* pHeap, uint initialSize, uint growSize, uint maxUnusedBlockAge);
   void kill();
   
   BMemoryHeap* getHeap() const { return mpHeap; }

   enum { cMaxAligment = 4096U };         
   void* alloc(uint size, uint alignment, bool growIfNeeded = true);
         
   void freeAll(bool deleteAllBlocks = false);
   void setAllBlocks(uint c);
   
   uint getNumBlocks() const { return mBlocks.getSize(); }
   uint getBlockSize(uint index) const { return mBlocks[index].mSize; }
   void* getBlockPtr(uint index) const { return mBlocks[index].mPtr; }
      
   uint getTotalBlockBytesAllocated() const { return mTotalBlockBytesAllocated; }
   uint getNumAllocations() const { return mNumAllocations; }
   uint getTotalBytesAllocated() const { return mTotalBytesAllocated; }
   uint getLargestAllocation() const { return mLargestAllocation; }
   uint getMaxTotalBytesAllocated() const { return mMaxTotalBytesAllocated; }
   uint getMaxLargestAllocation() const { return mMaxLargestAllocation; }

private:
   struct BBlock
   {
      void* mPtr;
      uint  mSize;
      uint  mNext;
      uint  mLastFrameUsed;
      
      uint getBytesAvailable(uint alignment = sizeof(uint)) const
      {
         BDEBUG_ASSERT(mNext <= mSize);
         const uint bytesAvailable = mSize - mNext;
         const uint bytesToAlignUp = Utils::BytesToAlignUp(static_cast<uchar*>(mPtr) + mNext, alignment);
         if (bytesToAlignUp > bytesAvailable)
            return 0;
         return bytesAvailable - bytesToAlignUp;
      }
   };
   
   typedef BSegmentedArray<BBlock, 3> BBlockArray;
   
   BMemoryHeap*   mpHeap;
   BBlockArray    mBlocks;
   uint           mInitialSize;
   uint           mGrowSize;
   uint           mCurFrame;
   uint           mMaxUnusedBlockAge;
   uint           mTotalBlockBytesAllocated;
      
   uint           mNumAllocations;
   uint           mTotalBytesAllocated;
   uint           mLargestAllocation;
   uint           mMaxTotalBytesAllocated;
   uint           mMaxLargestAllocation;
   BBlock*        mpDV;
   
   BBlock* newBlock(uint size, uint alignment);
};
