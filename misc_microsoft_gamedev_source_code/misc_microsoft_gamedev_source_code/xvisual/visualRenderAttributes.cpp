//==============================================================================
// visualRenderAttributes.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "visualRenderAttributes.h"

//==============================================================================
// BUberVisualRenderAttributes::BUberVisualRenderAttributes
//==============================================================================
BUberVisualRenderAttributes::BUberVisualRenderAttributes() :
   IExtendedVisualRenderAttributes(cEVRAUber)
{
   clear();
}

//==============================================================================
// BUberVisualRenderAttributes::clear
//==============================================================================
void BUberVisualRenderAttributes::clear(void)
{
   mAddTexture = cInvalidManagedTextureHandle;
   mAddTexR = 1.0f;
   mAddTexG = 1.0f;
   mAddTexB = 1.0f;
   mAddTexInten = 1.0f;
   mAddTexUVXFormU.clear();
   mAddTexUVXFormV.clear();
   mAddTexClamp = false;

   mLerpTexture = cInvalidManagedTextureHandle;
   mLerpTexR = 1.0f;
   mLerpTexG = 1.0f;
   mLerpTexB = 1.0f;
   mLerpTexOpacity = 1.0f;
   mLerpTexUVXFormU.clear();
   mLerpTexUVXFormV.clear();
   mLerpTexClamp = false;
}

//==============================================================================
// BUberVisualRenderAttributes::operator==
//==============================================================================
bool BUberVisualRenderAttributes::operator== (const IExtendedVisualRenderAttributes& rhs) const
{
   if (mType != rhs.mType)
      return false;
      
   BUberVisualRenderAttributes& uberLHS = (BUberVisualRenderAttributes&)*this;
   BUberVisualRenderAttributes& uberRHS = (BUberVisualRenderAttributes&)rhs;

#define COMPARE(v) if (uberLHS.##v != uberRHS.##v) return false; 
   COMPARE(mAddTexture);
   COMPARE(mAddTexR);
   COMPARE(mAddTexG);
   COMPARE(mAddTexB);
   COMPARE(mAddTexInten);
   COMPARE(mAddTexUVXFormU);
   COMPARE(mAddTexUVXFormV);
   COMPARE(mLerpTexture);
   COMPARE(mLerpTexR);
   COMPARE(mLerpTexG);
   COMPARE(mLerpTexB);
   COMPARE(mLerpTexOpacity);
   COMPARE(mLerpTexUVXFormU);
   COMPARE(mLerpTexUVXFormV); 
#undef COMPARE                 

   return true;
}

//==============================================================================
// BUberVisualRenderAttributes::operator<
//==============================================================================
bool BUberVisualRenderAttributes::operator< (const IExtendedVisualRenderAttributes& rhs) const
{
   if (mType != rhs.mType)
      return mType < rhs.mType;
   
   BUberVisualRenderAttributes& uberLHS = (BUberVisualRenderAttributes&)*this;
   BUberVisualRenderAttributes& uberRHS = (BUberVisualRenderAttributes&)rhs;
   
#define COMPARE(v) if (uberLHS.##v < uberRHS.##v) return true; else if (uberLHS.##v > uberRHS.v##) return false; 
   COMPARE(mAddTexture);
   COMPARE(mAddTexR);
   COMPARE(mAddTexG);
   COMPARE(mAddTexB);
   COMPARE(mAddTexInten);
   COMPARE(mAddTexUVXFormU);
   COMPARE(mAddTexUVXFormV);
   COMPARE(mLerpTexture);
   COMPARE(mLerpTexR);
   COMPARE(mLerpTexG);
   COMPARE(mLerpTexB);
   COMPARE(mLerpTexOpacity);
   COMPARE(mLerpTexUVXFormU);
   COMPARE(mLerpTexUVXFormV); 
#undef COMPARE             
    
   return false;
}

//==============================================================================
// BUberVisualRenderAttributes::compareVisual
//==============================================================================
bool BUberVisualRenderAttributes::compareVisual(const IExtendedVisualRenderAttributes& rhs) const
{
   if (mType != rhs.mType)
      return false;
      
   return *this == (*(BUberVisualRenderAttributes*)&rhs);
}

//==============================================================================
// BUberVisualRenderAttributes::compareShadowGen
//==============================================================================
bool BUberVisualRenderAttributes::compareShadowGen(const IExtendedVisualRenderAttributes& rhs) const
{
   if (mType != rhs.mType)
      return false;
      
   return true;
}

//==============================================================================
// BVisualRenderAttributes::BVisualRenderAttributes
//==============================================================================
BVisualRenderAttributes::BVisualRenderAttributes() : 
   mPixelXFormColor(0xFFFFFFFF), 
   mTintColor(0xFF000000),
   mShadowReceiver(true),
   mShadowCaster(true),
   mGlobalLighting(true),
   mLocalLighting(true),
   mFarLayer(false),
   mNearLayer(false),
   mObscurable(false),
   mAppearsBelowDecals(false),
   mSampleBlackmap(false),
   mPlayerColorIndex(0),
   mpExtendedAttributes(NULL),
   mReflectBoneIndex(0),
   mAOTintValue(255),
   mMultiframeTextureIndex(0),
   mEmissiveIntensity(1.0f),
   mHighlightIntensity(0.0f),
   mProjectedArea(-1.0f)
{
   Utils::ClearObj(mBounds);
   Utils::ClearObj(mUVOffsets);
}

//==============================================================================
// BVisualRenderAttributes::BVisualRenderAttributes
//==============================================================================
BVisualRenderAttributes::BVisualRenderAttributes(DWORD pixelXFormColor, DWORD tintColor, XMVECTOR min, XMVECTOR max) : 
   mPixelXFormColor(pixelXFormColor), 
   mTintColor(tintColor),
   mShadowReceiver(true),
   mShadowCaster(true),
   mGlobalLighting(true),
   mLocalLighting(true),
   mFarLayer(false),
   mNearLayer(false),
   mObscurable(false),
   mAppearsBelowDecals(false),
   mSampleBlackmap(false),
   mPlayerColorIndex(0),
   mpExtendedAttributes(NULL),
   mReflectBoneIndex(0),
   mAOTintValue(255),
   mMultiframeTextureIndex(0),
   mEmissiveIntensity(1.0f),
   mHighlightIntensity(0.0f),
   mProjectedArea(-1.0f)
{
   Utils::ClearObj(mUVOffsets);
   setBounds(min, max);
}

//==============================================================================
// BVisualRenderAttributes::clear
//==============================================================================
void BVisualRenderAttributes::clear(void)
{
   mPixelXFormColor = 0xFFFFFFFF;
   mTintColor = 0xFF000000;
   Utils::ClearObj(mUVOffsets);
   Utils::ClearObj(mBounds);
   mShadowReceiver = true;
   mShadowCaster = true;
   mGlobalLighting = true;
   mLocalLighting = true;
   mFarLayer = false;
   mNearLayer = false;
   mObscurable = false;
   mAppearsBelowDecals = false;
   mSampleBlackmap = false;
   mPlayerColorIndex = 0;
   mpExtendedAttributes = NULL;
   mReflectBoneIndex = 0;
   mAOTintValue = 255;
   mMultiframeTextureIndex = 0;
   mEmissiveIntensity = 1.0f;
   mHighlightIntensity = 0.0f;
   mProjectedArea = -1.0f;
}
