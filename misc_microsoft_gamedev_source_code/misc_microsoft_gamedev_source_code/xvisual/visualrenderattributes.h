//==============================================================================
// visualRenderAttributes.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "D3DTextureManager.h"
#include "math\generalVector.h"
#include "D3DTextureManager.h"


//==============================================================================
// enum eExtendedVisualRenderAttributesType
//==============================================================================
enum eExtendedVisualRenderAttributesType
{  
   cEVRAUber,
   
   cEVRANum
};

//==============================================================================
// IExtendedVisualRenderAttributes
//==============================================================================
class IExtendedVisualRenderAttributes
{
public:
   IExtendedVisualRenderAttributes(eExtendedVisualRenderAttributesType type) : mType(type) { }
   
   virtual eExtendedVisualRenderAttributesType getType(void) const { return mType; }
      
   virtual void clear(void) = 0;
   
   virtual bool operator== (const IExtendedVisualRenderAttributes& rhs) const = 0;
   virtual bool operator< (const IExtendedVisualRenderAttributes& rhs) const = 0;
   
   virtual bool compareVisual(const IExtendedVisualRenderAttributes& rhs) const = 0;
   virtual bool compareShadowGen(const IExtendedVisualRenderAttributes& rhs) const = 0;
   
   eExtendedVisualRenderAttributesType mType;
};

//==============================================================================
// BUberVisualRenderAttributes
//==============================================================================
class BUberVisualRenderAttributes : public IExtendedVisualRenderAttributes
{
public:
   BUberVisualRenderAttributes();
   
   virtual void clear(void);

   virtual bool operator== (const IExtendedVisualRenderAttributes& rhs) const;
   virtual bool operator< (const IExtendedVisualRenderAttributes& rhs) const;

   virtual bool compareVisual(const IExtendedVisualRenderAttributes& rhs) const;
   virtual bool compareShadowGen(const IExtendedVisualRenderAttributes& rhs) const;
      
   BManagedTextureHandle   mAddTexture;
   BManagedTextureHandle   mLerpTexture;
   
   // Do not change the order or size of these members!
   float                   mAddTexR;
   float                   mAddTexG;
   float                   mAddTexB;
   float                   mAddTexInten;
   BVec4                   mAddTexUVXFormU; // texcoord U column
   BVec4                   mAddTexUVXFormV; // texcoord V column
       
   // Do not change the order or size of these members!
   float                   mLerpTexR;
   float                   mLerpTexG;
   float                   mLerpTexB;
   float                   mLerpTexOpacity;
   // mLerpTexUVXFormU and V form a 4x2 matrix. Worldspace (X,Y,Z,1) is multiplied by this matrix to compute 
   // the UV used to sample the lerp matrix. The mLerpTexUVXFormU vector is dotted against (X,Y,Z,1) to compute U, etc.
   BVec4                   mLerpTexUVXFormU; // texcoord U column
   BVec4                   mLerpTexUVXFormV; // texcoord V column

   bool                    mAddTexClamp:1;
   bool                    mLerpTexClamp:1;
   
   // Example to set the add texture's uv offset and scale. Other transforms are possible.
   void setAddTexUVOfsAndScale(float uOfs, float vOfs, float scale)
   {
      mAddTexUVXFormU.set(scale, 0.0f, 0.0f, uOfs);
      mAddTexUVXFormV.set(0.0f, 0.0f, scale, vOfs);
   }
   
   // Example to set the lerp texture's uv offset and scale. Other transforms are possible.
   void setLerpTexUVOfsAndScale(float uOfs, float vOfs, float scale)
   {
      mLerpTexUVXFormU.set(scale, 0.0f, 0.0f, uOfs);
      mLerpTexUVXFormV.set(0.0f, 0.0f, scale, vOfs);
   }
};

//==============================================================================
// BVisualRenderAttributes
//==============================================================================
class BVisualRenderAttributes
{
public:
   BVisualRenderAttributes();
   
   BVisualRenderAttributes(
      DWORD pixelXFormColor, DWORD tintColor, 
      XMVECTOR min = XMVectorZero(), XMVECTOR max = XMVectorZero());
   
   void clear(void);
   
   void setBounds(XMVECTOR min, XMVECTOR max) { XMStoreFloat3((XMFLOAT3*)&mBounds[0], min); XMStoreFloat3((XMFLOAT3*)&mBounds[3], max); }
   
   XMVECTOR getBoundsMin(void) const { return XMLoadFloat3((const XMFLOAT3*)&mBounds[0]); }
   XMVECTOR getBoundsMax(void) const { return XMLoadFloat3((const XMFLOAT3*)&mBounds[3]); }
   
   bool getAlphaBlend(void) const { return (mTintColor & 0xFF000000) < 0xFF000000; }

   DWORD mPixelXFormColor;       
   
   // A of tint color controls overall alpha.
   DWORD mTintColor;             
   
   // Per-channel UV offset.
   enum { cMaxUVOffsets = 3 };
   float mUVOffsets[cMaxUVOffsets][2];
   
   // HDR texture map intensity multipliers.
   float mEmissiveIntensity;
   float mHighlightIntensity;
   
   // Conservative bounding volume.
   float mBounds[6];
   
   // Projected area of bounding volume.
   float mProjectedArea;
   
   // mpExtendedAttributes should point to CPU frame storage! May be NULL.
   IExtendedVisualRenderAttributes* mpExtendedAttributes;
         
   uint16 mReflectBoneIndex;
   
   uchar mPlayerColorIndex;
   
   uchar mAOTintValue;
   
   uchar mMultiframeTextureIndex;
         
   bool mShadowReceiver : 1;
   bool mShadowCaster   : 1;
   
   // mGlobalLighting is currently ignored. Global lighting is controlled by the gDirLightEnabled intrinsic, managed by BVisualLightManager.
   bool mGlobalLighting : 1;
   
   bool mLocalLighting  : 1;
   
   bool mFarLayer       : 1;
   bool mNearLayer      : 1;
   
   bool mObscurable     : 1;
   
   bool mAppearsBelowDecals : 1;
   
   bool mSampleBlackmap : 1;
};

//==============================================================================
// BVisualModelUVOffsets
//==============================================================================
struct BVisualModelUVOffsets
{
   BVisualModelUVOffsets() { clear(); }

   void clear() { Utils::ClearObj(*this); }  

   enum { cMaxUVOffsets = BVisualRenderAttributes::cMaxUVOffsets };
   BVec2 mUVOffsets[cMaxUVOffsets];

   bool operator== (const BVisualModelUVOffsets& rhs) const { for (uint i = 0; i < cMaxUVOffsets; i++) if (mUVOffsets[i] != rhs.mUVOffsets[i]) return false; return true; }
   bool operator!= (const BVisualModelUVOffsets& rhs) const { return !(*this == rhs); }
};