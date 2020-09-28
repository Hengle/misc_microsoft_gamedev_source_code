//============================================================================
// File: effectIntrinsicMapper.h
// rg [2/21/06] - The effect intrinsic mapper class records the linkage between non-shared effect parameters and intrinsic indices.
//============================================================================
#pragma once

#include "containers\staticArray.h"

class BFXLEffect;
class BFXLEffectIntrinsicPool;

//============================================================================
// class BEffectIntrinsicMapper
//============================================================================
class BEffectIntrinsicMapper
{
public:
   BEffectIntrinsicMapper();

   void clear(void);

   void init(BFXLEffect* pEffect, BFXLEffectIntrinsicPool* pPool, bool validateIntrinsics = true);
   void apply(BFXLEffect* pEffect, bool force = false);
      
   BFXLEffectIntrinsicPool* getIntrinsicPool(void) const { return mpIntrinsicPool; }
   
   uint getNumParamToIntrinsicLinks(void) const { return mLinks.getSize(); }
   uint getParamIndex(uint linkIndex) const { return mLinks[linkIndex].mParamIndex; }
   uint getIntrinsicIndex(uint linkIndex) const { return mLinks[linkIndex].mIntrinsicIndex; }
   
private:
#pragma pack(push, 1)   
   struct BParamToIntrinsicLink
   {
      uchar mParamIndex;
      uchar mIntrinsicIndex;

      BParamToIntrinsicLink() { }
      BParamToIntrinsicLink(uchar paramIndex, uchar intrinsicIndex) : mParamIndex(paramIndex), mIntrinsicIndex(intrinsicIndex) { }
   };
#pragma pack(pop)

   uint mPrevGenerationIndex;
   BDynamicArray<BParamToIntrinsicLink, ALIGN_OF(BParamToIntrinsicLink), BDynamicArrayRenderHeapAllocator, BDynamicArraySmallOptions, BDynamicArraySinglePointer> mLinks;
   BFXLEffectIntrinsicPool* mpIntrinsicPool;
};