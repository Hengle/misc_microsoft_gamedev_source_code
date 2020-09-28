//============================================================================
//
//  xboxTextureHeap.h
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "containers\sparseBitArray.h"
#include "containers\hashMap.h"
#include "memory\bfHeapAllocator.h"
#include <xgraphics.h>

class BXboxTextureHeap
{
public:
   BXboxTextureHeap();
   ~BXboxTextureHeap();
   
   void init();
   void deinit();

   bool reserveValleys(uint num, uint width, uint height, uint depth, uint levels, D3DFORMAT fmt, D3DRESOURCETYPE textureType);
   
   void* getValley(const IDirect3DTexture9* pTex, uint* pAllocSize);
   bool unlockValley(const IDirect3DTexture9* pTex);
   bool releaseValley(const IDirect3DTexture9* pTex);
   
   void* alloc(uint size, uint* pActualSize = NULL);
   // Returns true if the block belonged to the texture manager. 
   bool free(void* p);
   uint getSize(void* p);
   
   bool check(void);
   
private:
   struct BValleyClass
   {
      uint                       mAllocSize;
      uint                       mUnusedSize;
      
      BDynamicCHeapArray<uint>   mRegions;
      
      BDynamicCHeapArray<void*>  mFreePtrs;
      BDynamicCHeapArray<void*>  mUsedPtrs;
            
   };
   
   enum eValleyState
   {
      cVSInvalid  = 0,
      cVSAvailable,
      cVSLocked,
      cVSOccupied
   };
   
   struct BValley
   {
      uint16         mClassIndex;
      
      eValleyState   mState;
      
      void*          mpData;
      uint           mSize;
      
      bool           mUnusedRegionsInUse;
   };
   
   // Protected by mValleyMutex
   BCriticalSection                    mValleyMutex;
   
   BDynamicCHeapArray<BValleyClass>    mValleyClasses;
   
   typedef BHashMap<void*, BValley, BHasher<void*>, BEqualTo<void*>, true, BCRunTimeFixedHeapAllocator> BValleyHashMap;
   BValleyHashMap                      mValleyHashMap;
   
   struct BUnusedRegion
   {
      BUnusedRegion() { }
      BUnusedRegion(void* pStart, uint size) : mpStart(pStart), mSize(size) { }
      
      void* mpStart;
      uint mSize;
   };
   
   BDynamicCHeapArray<BUnusedRegion>   mUnusedRegionBuf;
   
   uint                                mUnusedRegionBufLargestUnused;
      
   // Unprotected - I'm assuming the caller will be locking before calling alloc or free
   BSparseBitArray                     mRegionPages;
   
   BBFHeapAllocator                    mRegionHeap;
   
   uint                                mRegionHeapMaxUnusedSize;
      
   uint                                mTotalPhysAllocations;
   uint                                mTotalPhysBytes;
   
   void*                               allocatePhysicalMemory(uint size);
   void                                deletePhysicalMemory(void* p, uint size);
   
   void                                getTextureRegions(const IDirect3DTexture9* pHeader, BDynamicCHeapArray<uint>& regions, uint& allocSize, uint& unusedSize);
   uint                                getOrCreateValleyClass(BDynamicCHeapArray<uint>& regions, uint allocSize, uint unusedSize);
   void*                               createNewValley(uint valleyClassIndex, eValleyState initialState);
   
   void                                queueUnusedRegions(const BValley& valley);
   void                                flushUnusedRegions();
};

