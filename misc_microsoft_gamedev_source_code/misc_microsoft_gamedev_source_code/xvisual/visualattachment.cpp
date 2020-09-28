//==============================================================================
// visualattachment.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "visualattachment.h"

//==============================================================================
// BVisualAttachment::BVisualAttachment
//==============================================================================
BVisualAttachment::BVisualAttachment() : 
   BVisualItem(), 
   mAnimType(-1),
   mFromBoneHandle(-1), 
   mToBoneHandle(-1), 
   mIndex(-1), 
   mpName(NULL),
   mFlags()
{ 
   setFlag(BVisualAttachment::cFlagActive, true);
   mMatrix.makeIdentity();
   mTransform.makeIdentity(); 
}

//==============================================================================
// BVisualAttachment::BVisualAttachment
//==============================================================================
BVisualAttachment::BVisualAttachment(const BVisualAttachment& source)
{
   *this=source;
}

//==============================================================================
// BVisualAttachment::~BVisualAttachment
//==============================================================================
BVisualAttachment::~BVisualAttachment()
{
};

//==============================================================================
// BVisualAttachment::operator=
//==============================================================================
BVisualAttachment& BVisualAttachment::operator=(const BVisualAttachment& source)
{
   if(this==&source)
      return *this;

   BVisualItem::operator=(source);

   mAnimType=source.mAnimType;
   mFromBoneHandle=source.mFromBoneHandle;
   mToBoneHandle=source.mToBoneHandle;
   mMatrix=source.mMatrix;
   mTransform=source.mTransform;
   mIndex=source.mIndex;
   mpName=source.mpName;
   mFlags=source.mFlags;

   return *this;
}

//==============================================================================
// BVisualAttachment::updateState
//==============================================================================
void BVisualAttachment::updateState(long animationTrack, long animType, bool synced, long modelAssetType, long modelAssetIndex, const BVisualModelUVOffsets& uvOffsets, long animAssetType, long animAssetIndex, BProtoVisualTagArray* pTags, BProtoVisualPointArray* pPoints, BFloatProgression* pOpacityProgression, BVector minCorner, BVector maxCorner, bool applyInstantly, float timeIntoAnimation, float tweenTime, long damageIndex, bool reset, BVisualItem* startOnThisAttachment, DWORD tintColor, const BMatrix& worldMatrix)
{
   BVisualItem::updateState(animationTrack, animType, synced, modelAssetType, modelAssetIndex, uvOffsets, animAssetType, animAssetIndex, pTags, pPoints, pOpacityProgression, minCorner, maxCorner, applyInstantly, timeIntoAnimation, tweenTime, damageIndex, reset, startOnThisAttachment, tintColor, worldMatrix);
   mAnimType=animType;
}
