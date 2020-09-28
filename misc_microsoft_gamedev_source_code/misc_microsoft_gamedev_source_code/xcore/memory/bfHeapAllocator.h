//============================================================================
//
//  bfHeapAllocator.h
//
//  Copyright (c) 2008, Ensemble Studios
//
//
//============================================================================
#pragma once
#include "containers\hashMap.h"

#ifndef BUILD_FINAL  
   // VERY slow, only intended for debugging.
   //#define BFHEAP_TRACK_ALLOCATED_BLOCKS
#endif   

//----------------------------------------------------------------------------
//  Class BBFHeapAllocator
//----------------------------------------------------------------------------
class BBFHeapAllocator
{
   BBFHeapAllocator(const BBFHeapAllocator&);
   BBFHeapAllocator& operator=(const BBFHeapAllocator&);

public:
   BBFHeapAllocator();
   
   void clear();

   enum { cMinRegionSize = 16 };
   bool addRegion(void* pRegionStart, uint regionSize);
   bool isRegionFree(void* pRegionStart, uint regionSize);
   
   struct BRegion 
   {
      BRegion() { }
      BRegion(void* pStart, uint size) : mpStart(pStart), mSize(size) { }
      
      void* mpStart;
      uint mSize;
   };
   // removeFreeRegions() is a potentially unsafe! 
   // You MUST be sure that no free or allocated chunks could appear immediately before any free region. 
   // This method is unable to verify that this rule is always followed.
   bool removeFreeRegions(const BRegion* pRegions, uint numRegions);

   void getAllocInfo(uint& totalRegionBytes, uint& totalNumAllocations, uint& totalBytesAllocated, uint& totalBytesFree, uint& totalOperations) const;
   void getMemoryInfo(uint& totalFreeChunks, uint& largestFreeChunk, uint& totalBins, uint& totalOverhead, uint& dvSize) const;
   
   void* alloc(uint requestedSize, uint* pActualSize = NULL);
   bool free(void* const p);
   uint getSize(void* p) const;
   
   // Supports 4, 8, and 16 byte alignment
   // Allocations requiring 4 byte alignment have no overhead, 8 byte alignment has 4 bytes overhead, and 16 byte alignment has 12 bytes of overhead.
   enum { cMaxAlignedAllocAlignment = 16 };
   void* alignedAlloc(uint requestedSize, uint* pActualSize = NULL);
   bool alignedFree(void* p);
   uint alignedGetSize(void* p) const;
   
   bool check(void) const;
      
   enum 
   { 
      cPrevInUseFlag = 1, 
      cInUseFlag     = 2,
      cLastFlag      = 4,
      cFlagsMask     = 7,
      cSizeMask      = 0xFFFFFFF8
   };
   
private:
   enum 
   {
      cMinChunkSize = 16,
      
      cMaxSmallChunkSize = 264,
      cSmallChunkShift = 3,
      cNumExactChunkBins = 32,
      
      cAllocAlignmentShift = 3,
      cAllocAlignment = 1 << cAllocAlignmentShift
   };
   
   struct BFreeChunkHeader;
   
   struct BFreeChunkLink
   {
      BFreeChunkLink* mpPrevFree;
      BFreeChunkLink* mpNextFree;
      
      void clear()
      {
         mpPrevFree = NULL;
         mpNextFree = NULL;
      }      
      
      void set(BFreeChunkLink* pPrev, BFreeChunkLink* pNext) { mpPrevFree = pPrev; mpNextFree = pNext; }
      
      const BFreeChunkHeader* getHeaderPtr() const { return reinterpret_cast<const BFreeChunkHeader*>(reinterpret_cast<const uint*>(this) - 1); }
            BFreeChunkHeader* getHeaderPtr()       { return reinterpret_cast<BFreeChunkHeader*>(reinterpret_cast<uint*>(this) - 1); }
            
      void remove()
      {
         mpPrevFree->mpNextFree = mpNextFree;
         mpNextFree->mpPrevFree = mpPrevFree;
      }            
      
      void insertBefore(BFreeChunkLink* pLink)
      {
         BFreeChunkLink* pPrev = mpPrevFree;
         
         mpPrevFree = pLink;
         pLink->mpNextFree = this;
         
         pLink->mpPrevFree = pPrev;
         pPrev->mpNextFree = pLink;
      }
      
      void insertAfter(BFreeChunkLink* pLink)
      {
         BFreeChunkLink* pNext = mpNextFree;
         
         mpNextFree = pLink;
         pLink->mpPrevFree = this;
         
         pLink->mpNextFree = pNext;
         pNext->mpPrevFree = pLink;
      }
   };
   
   struct BFreeChunkHeader
   {
      uint              mSize;
      BFreeChunkLink    mLink;
            
      BOOL isPrevInUse() const { return (mSize & cPrevInUseFlag); }
      BOOL isInUse() const { return (mSize & cInUseFlag); }
      BOOL isFree() const { return (mSize & cInUseFlag) == 0; }
      BOOL isLast() const { return (mSize & cLastFlag); }
      uint getSize() const { return mSize & cSizeMask; }
      void setSize(uint size) { mSize = (mSize & ~cSizeMask) | size; }
   };
   
   enum { cNumBins = 96 };         
   BFreeChunkLink    mBins[cNumBins];
   
   BFreeChunkHeader* mpDV;
   
   uint              mTotalAllocations;   
   uint              mTotalBytesAllocated;
   uint              mTotalBytesFree;
   uint              mTotalRegionBytes;
   
   uint              mLowestRegionPtr;
   uint              mHighestRegionPtr;
   uint              mLowestAllocatedPtr;
   uint              mHighestAllocatedPtr;
   
   uint              mOperationIndex;
   
#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
   typedef BHashMap<void*, uint, BBitHasher<void*>, BEqualTo<void*>, true, BCRunTimeFixedHeapAllocator> BAllocatedBlackHashMap;
   BAllocatedBlackHashMap mAllocatedBlocks;
#endif

   void heapCorruption(const char* pMsg) const;
   void eraseFreeChunk(BFreeChunkHeader* pHeader);
   bool checkRegion(BFreeChunkHeader* pHeader, void*& pRegionStart, uint& regionSize, BFreeChunkHeader*& pBefore, BFreeChunkHeader*& pAfter);
   BFreeChunkLink* findFreeChunk(uint size) const;
   void addToBin(BFreeChunkHeader* pChunkHeader, uint chunkSize);
   uint getBinIndex(uint size) const;
   bool checkFreeChunk(BFreeChunkLink* pCurLink) const;
   bool isValidLinkPtr(const BFreeChunkLink* pLink) const;
   
   void* allocInternal(uint requestedSize, uint* pActualSize);
   bool freeInternal(void* const p);
   uint getSizeInternal(void* p) const;
   
   static bool doRegionsIntersect(void* pRegion1Start, uint region1Size, void* pRegion2Start, uint region2Size);
   static bool isRegionContained(void* pRegion1Start, uint region1Size, void* pRegion2Start, uint region2Size);
   static void* getRawHeapPtr(void* p);
};
