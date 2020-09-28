//==============================================================================
// visualattachment.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "visualitem.h"

//==============================================================================
// BVisualAttachment
//==============================================================================
class BVisualAttachment : public BVisualItem
{
   public:
                                 BVisualAttachment();
                                 BVisualAttachment(const BVisualAttachment& source);
                                 ~BVisualAttachment();
      enum
      {
         cFlagUsed,
         cFlagUser,
         cFlagActive,
         cFlagUseTransform,
         cFlagUseLifespan,
         cFlagSyncAnims,
         cFlagDisregardOrient
      };

      BVisualAttachment&         operator=(const BVisualAttachment& source);

      virtual void               updateState(long animationTrack, long animType, bool synced, long modelAssetType, long modelAssetIndex, const BVisualModelUVOffsets& uvOffsets, long animAssetType, long animAssetIndex, BProtoVisualTagArray* pTags, BProtoVisualPointArray* pPoints, BFloatProgression* pOpacityProgression, BVector minCorner, BVector maxCorner, bool applyInstantly, float timeIntoAnimation, float tweenTime, long damageIndex, bool reset, BVisualItem* startOnThisAttachment, DWORD tintColor, const BMatrix& worldMatrix);

      bool                       getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                       setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      BMatrix                    mMatrix;      
      BMatrix                    mTransform;
      
      long                       mAnimType;

      long                       mFromBoneHandle;
      long                       mToBoneHandle;      
      long                       mIndex;
      const BSimString*          mpName;
      float                      mLifespan;      
      
      UTBitVector<8>             mFlags;
};

typedef BSmallDynamicSimArray<BVisualAttachment, 16>  BVisualAttachmentArray;
