//============================================================================
//
// File: physicalAllocator.h
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "memory\AllocFixed.h"

//============================================================================
// class BPhysicalAllocator
// Not thread safe!
//============================================================================
class BPhysicalAllocator
{
   BPhysicalAllocator(const BPhysicalAllocator&);
   BPhysicalAllocator& operator= (const BPhysicalAllocator&);
   
public:
   BPhysicalAllocator();
   ~BPhysicalAllocator();
         
   bool init(uint initialBlockSize = 0, uint minBlockGrowSize = 32768, DWORD protect = PAGE_READWRITE | PAGE_WRITECOMBINE, BMemoryHeap* pHeap = cXPhysicalAllocDummyHeapPtr);
   void kill(void);
   
   bool releaseUnusedBlocks(void);
   
   bool getInitialized(void) const { return mInitialized; }
   uint getInitialBlockSize(void) const { return mInitialBlockSize; }
   uint getMinBlockSize(void) const { return mMinBlockGrowSize; }
   DWORD getProtect(void) const { return mProtect; }

   enum 
   {
      // Use first fit vs. best fit
      cFirstFit         = 1,
      
      // Search for free memory starting at the "top" of the heap, vs. the "bottom".
      cTopToBottom       = 2
   };
   
   // Returns NULL on failure.
   enum { cMinAlignment = 32, cMaxAlignment = 4096 };
   void* alloc(uint size, uint alignment = cMinAlignment, uint flags = 0);
   
   bool free(void* p);
   
   bool getSize(void* p, uint& size);
   
   struct BHeapStats
   {
      uint totalBlocks;
      uint totalNodes;
      uint totalFreeNodes;
      uint totalBytesAllocated;
      uint totalBytesFree;

      uint largestBlock;
      uint largestAllocatedNode;
      uint largestFreeNode;
      
      void clear(void) { Utils::ClearObj(*this); }
   };
   
   bool check(BHeapStats& stats);
   
   void resetStats(void);
   
   int getTotalAllocationCount(void) const { return mTotalAllocationCount; }
   int getTotalAllocationBytes(void) const { return mTotalAllocationBytes; }
   int getMaxAllocationCount(void) const { return mMaxAllocationCount; }
   int getMaxAllocationBytes(void) const { return mMaxAllocationBytes; }
      
#ifndef BUILD_FINAL
   static void test(void);
#endif   

private:
            
   struct BLink
   {
      BLink* mpPrev;
      BLink* mpNext;
      
      void clear(void) { mpPrev = NULL; mpNext = NULL; }
   };
   
   struct BBlock;
   
   struct BNode : BLink
   {
      uint mPtr;
      uint mSize;
                  
      void clear(void) { Utils::ClearObj(*this); }
      
      BNode* getPrev(void) const { return (BNode*)mpPrev; }
      BNode* getNext(void) const { return (BNode*)mpNext; }
      
      void setPrev(BNode* p) { mpPrev = p; }
      void setNext(BNode* p) { mpNext = p; }
            
      void setPtr(void* p) { BDEBUG_ASSERT(((uint)p & 1) == 0); mPtr = ((uint)p & ~1) | (mPtr & 1); }
      void* getPtr(void) const { return (void*)(mPtr & ~1); }
      
      uint getSize(void) const { return mSize; }
      void setSize(uint size) { mSize = size; }
      
      void setFree(BOOL free) { mPtr &= ~1; mPtr |= (free ? 1 : 0); }
      BOOL getFree(void) const { return (mPtr & 1); }
   };
   
   struct BBlock : BLink
   {
      void* mPtr;
      uint mSize;
      
      uint mTotalFree;
      
      BNode mNodeHead;
                  
      void clear(void) { Utils::ClearObj(*this); }
      
      BBlock* getPrev(void) const { return (BBlock*)mpPrev; }
      BBlock* getNext(void) const { return (BBlock*)mpNext; }

      void setPrev(BBlock* p) { mpPrev = p; }
      void setNext(BBlock* p) { mpNext = p; }
      
      void setPtr(void* p) { mPtr = p; }
      void* getPtr(void) const { return mPtr; }
      
      uint getSize(void) const { return mSize; }
      void setSize(uint size) { mSize = size; }
      
      uint getTotalFree(void) const { return mTotalFree; }
      void setTotalFree(uint totalFree) { mTotalFree = totalFree; }
   };
   
   BMemoryHeap*   mpHeap;
   
   uint           mInitialBlockSize;
   uint           mMinBlockGrowSize;
   DWORD          mProtect;

   int            mTotalAllocationCount;
   int            mTotalAllocationBytes;
   int            mMaxAllocationCount;
   int            mMaxAllocationBytes;
   
   BBlock         mBlockHead;
   
   BAllocFixed    mNodeAllocator;
   
   bool           mInitialized;
   
   BBlock* createBlock(uint blockSize, BBlock* pPrevBlock);
   void freeBlock(BBlock* pBlock);
   void initBlockHead(void);
   void updateStats(int allocationCountDelta, int allocationSizeDelta);
   uint computeTotalBytesNeeded(BNode* pNode, uint size, uint alignment, DWORD flags);
   
   BBlock* findBlockFromPtr(void* p);
   BNode* findNodeFromPtr(BBlock* pBlock, void* p);
   void deleteNode(BNode* pNode);
   void coalesceFreeNode(BBlock* pBlock, BNode* pNode);
};


