//============================================================================
//
//  File: ecfFileData.h
//
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "ecfUtils.h"

class BECFFileData
{
   BECFFileData(const BECFFileData&);
   BECFFileData& operator= (const BECFFileData&);
   
public:
   BECFFileData();
   ~BECFFileData();
   
   void                 clear(void);
      
   bool                 load(BStream* pStream, bool ignoreAlignmentErrors = false, bool storePhysicalCacheLines = true, bool okayToAllocWriteCombinedAsCached = false);
   
   DWORD                getHeaderID(void) const;
   
   bool                 getValid(void) const;
   
   uint                 getNumChunks(void) const;
   
   // Returns cInvalidIndex if not found.
   int                  findChunkByID(uint64 ID, uint startIndex = 0) const;
     
   uint                 getChunkDataLen(uint chunkIndex) const;
      
   // Returns -1 on failure
   int                  getChunkDataLenByID(uint64 id) const;
   
   uint                 getChunkDataAlignment(uint chunkIndex) const;
   void*                getChunkDataPtr(uint chunkIndex) const;
   
   // Returns NULL if not found.
   void*                getChunkDataPtrByID(uint64 ID) const;
   
   void                 discardChunkData(uint chunkIndex);
   void                 acquireChunkDataOwnership(uint chunkIndex);
   void                 discardAllChunks(void);
   
   enum eChunkDataMemProtect
   {
      cCached,
      cPhysicalCached,
      cPhysicalWriteCombined,
      
      cNumMemProtect
   };
   
   eChunkDataMemProtect getChunkDataMemProtect(uint chunkIndex) const;
   BMemoryHeap*         getChunkDataHeap(uint chunkIndex) const;
   
   uint                 getTotalAllocatedMemory(eChunkDataMemProtect p) const { BDEBUG_ASSERT(p < cNumMemProtect); return mTotalAllocated[p]; }
   static bool          isPhysicalMemProtect(eChunkDataMemProtect p) { return p != cCached; }
   
   const BECFFileStream& getECFFileStream(void) const { return mFileStream; }
         BECFFileStream& getECFFileStream(void)       { return mFileStream; }
         
private:
   struct BChunkAllocation
   {
      BChunkAllocation() : mpData(NULL), mProtect(cCached), mpHeap(NULL) { }
      BChunkAllocation(void* pData, eChunkDataMemProtect protect, BMemoryHeap* pHeap) : mpData(pData), mProtect(protect), mpHeap(pHeap) { }
      
      void*                mpData;
      eChunkDataMemProtect mProtect;
      BMemoryHeap*         mpHeap;
   };
   
   BDynamicArray<BChunkAllocation>  mChunkAllocations;
   
   BECFFileStream                   mFileStream;
   
   uint                             mTotalAllocated[cNumMemProtect];
};
