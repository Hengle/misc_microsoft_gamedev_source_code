//==============================================================================
//
// File: ugxGeomRenderTypes.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once

#include "threading\eventDispatcher.h"
#include "..\xvisual\visualRenderAttributes.h"

class BVolumeCuller;
class BGrannyInstance;

extern IDirect3DDevice9* gpUGXD3DDev;

enum eUGXGeomPass
{
   // Visible passes
   cUGXGeomPassMain,
   cUGXGeomPassOverallAlpha,
   
   // Reflection passes
   cUGXGeomPassMainReflect,
   cUGXGeomPassOverallAlphaReflect,
   
   cUGXGeomPassFirstUnlightable,
   
   // All passes below here will not be local lightable!
   
   // Distortion
   cUGXGeomPassDistortion = cUGXGeomPassFirstUnlightable,
   
   // Shadow generation passes -- MUST be last!
   cUGXGeomPassShadowGen,        // orthographic/perspective projection
   cUGXGeomPassDPShadowGen,      // dual paraboloid projection

   cUGXGeomPassMax
};

enum 
{
   cUGXGeomLayerOpaque           = 1,
   cUGXGeomLayerAdditive         = 2,
   cUGXGeomLayerOver             = 4,

   cUGXGeomLayerBlended          = (cUGXGeomLayerAdditive | cUGXGeomLayerOver),
   
   cUGXGeomLayerDistortion       = 8,
   
   cUGXGeomLayerAllVisible       = (cUGXGeomLayerOpaque | cUGXGeomLayerAdditive | cUGXGeomLayerOver),
   
   cUGXGeomLayerAll              = (cUGXGeomLayerOpaque | cUGXGeomLayerAdditive | cUGXGeomLayerOver | cUGXGeomLayerDistortion)
};

enum eUGXGeomStatus
{
   cUGXGeomStatusInvalid,
   cUGXGeomStatusPending,
   cUGXGeomStatusReady,
   cUGXGeomStatusFailed,

   cUGXGeomStatusMax,
};

// Also update gUGXGeomVisModeDesc!
enum eUGXGeomVisMode
{
   cVMDisabled,
         
   cVMAlbedo,
   cVMAO,
   cVMXForm,
   cVMEmXForm,
   cVMSelf,
   
   cVMEnvMask,
   cVMEnv,
   cVMSpec,
   cVMOpacity,
   
   cVMAmbientSum,
   cVMDiffuseSum,
   cVMSpecSum,
   cVMWorldNormal,
   
   cVMTangentNormal,
   cVMSpecColor,
   cVMSpecPower,
   
   cVMHighlight,
   cVMModulate,
         
   cVMNum
};

enum eUGXGeomTextureMode
{
   cTMNormal,
   cTMAllWhite,
   cTMMipVis,
   cTMAllFlat,
   cTMNum
};

inline const char* getUGXGeomTextureModeDesc(eUGXGeomTextureMode mode)
{
   switch (mode)
   {
      case cTMNormal:   return "disabled";
      case cTMAllWhite: return "all-white";
      case cTMMipVis:   return "mipmap vis";
      case cTMAllFlat:  return "all-flat";
   }
   return "?";
}

enum eUGXGeomRenderFlags
{
   cRFNoFlags = 0,
   
   cRFShadowReceiver    = 1,
   cRFShadowCaster      = 2,
   cRFGlobalLighting    = 4,
   cRFLocalLighting     = 8,
   cRFLocalReflection   = 16,
   cRFLargeModel        = 32,
   cRFDestructable      = 64,

   cRFDefaultModelFlags = cRFLocalLighting | cRFGlobalLighting | cRFShadowCaster | cRFShadowReceiver,
   
   cRFForceDWORD        = 0xFFFFFFFF
};

extern const char* gUGXGeomVisModeDesc[cVMNum];

//==============================================================================
// class BUGXGeomRenderInfo
//==============================================================================
class BUGXGeomRenderInfo : public BEventPayload
{
public:
   BUGXGeomRenderInfo()
   {
   }
   
   void clear(void)
   {
      mNumBones = 0;
      mNumMeshes = 0;
      mNumSections = 0;
      mLayerFlags = 0;
      mRenderFlags = cRFDefaultModelFlags;
   }
   
   WORD mNumBones;
   
   WORD mNumMeshes;

   WORD mNumSections;
         
   uchar mLayerFlags;

   eUGXGeomRenderFlags mRenderFlags;
               
   static BUGXGeomRenderInfo* newInstance(void)
   {
      return new BUGXGeomRenderInfo;  
   }
        
private:         
   virtual void deleteThis(bool delivered)
   {
      delete this;
   }
};

//==============================================================================
// class BUGXGeomRenderCommonInstanceData
//==============================================================================
class BUGXGeomRenderCommonInstanceData
{
public:
   BUGXGeomRenderCommonInstanceData();
   
   void clear(void);
   
   const BVolumeCuller*             mpVolumeCuller;
   DWORD                            mTintColor;
   float                            mEmissiveIntensity;
   float                            mHighlightIntensity;
   
   // mpExtendedAttributes must point to CPU frame storage, and may be NULL.
   IExtendedVisualRenderAttributes* mpExtendedAttributes;
   
   uchar                            mNumPixelLights;
   eUGXGeomPass                     mPass;
   uchar                            mLayerFlags;
   uchar                            mTileFlags;
   
   uchar                            mMultiframeTextureIndex;
         
   // mGlobalLighting is currently ignored.
   bool                             mGlobalLighting : 1;
   bool                             mLocalLighting : 1;
   bool                             mDirLightShadows : 1;
   bool                             mLocalLightShadows : 1;
   bool                             mLocalReflection : 1;
   bool                             mSampleBlackmap : 1;
   
   bool                             mSetCommandBufferRunPredication : 1;
};

//==============================================================================
// struct BUGXGeomRenderPerInstanceData
// rg [3/5/06] - X,Y,Z = pixelxform color
// W = vertex index of start of bone matrix array in GPU frame storage
//==============================================================================
struct BUGXGeomRenderPerInstanceData
{
   XMVECTOR mColorBoneMatrices;

   enum { cMaxPixelLights = 16 };
   WORD mVisiblePixelLightIndices[cMaxPixelLights];
};

struct granny_model_instance;

//==============================================================================
// struct BUGXGeomRenderMeshMask
//==============================================================================
struct BUGXGeomRenderMeshMask
{
   enum 
   { 
      // This class is hardcoded to two QWORD's.
      cMaxQWORDs = 2,
      cMaxMeshes = cMaxQWORDs * 64
   };
   
   uint64 mBits[cMaxQWORDs];
   
   void setAll() 
   {
      BCOMPILETIMEASSERT(cMaxQWORDs == 2);
      mBits[0] = UINT64_MAX;
      mBits[1] = UINT64_MAX;
   }
   
   void clearAll()
   {
      mBits[0] = 0;
      mBits[1] = 0;
   }
   
   bool getBit(uint index) const
   {
      BDEBUG_ASSERT(index < cMaxMeshes);
      return (mBits[index >> 6U] & Utils::BBitMasks::get64(index & 63U)) != 0;
   }
   
   void setBit(uint index, bool val)
   {
      BDEBUG_ASSERT(index < cMaxMeshes);
      const uint64 mask = Utils::BBitMasks::get64(index & 63U);
      const uint ofs = index >> 6U;
      
      if (val)
         mBits[ofs] |= mask;
      else
         mBits[ofs] &= ~mask;
   }
   
   bool areAllSet() const
   {
      return (mBits[0] == UINT64_MAX) && (mBits[1] == UINT64_MAX);
   }
   
   bool areAnyNotSet() const
   {
      return (mBits[0] != UINT64_MAX) || (mBits[1] != UINT64_MAX);
   }
   
   void set(const uchar* pBytes, uint numBytes)
   {
      numBytes = Math::Min<uint>(numBytes, sizeof(mBits));
      
      uint bitOfs = 0;
      uint qwordOfs = 0;

      uint64 q0 = 0, q1 = 0;
      for (uint i = 0; i < numBytes; i++)
      {
         const uint64 bits = ((uint64)(pBytes[i])) << ((uint64)bitOfs);
         if (qwordOfs)
            q1 |= bits;
         else
            q0 |= bits;

         bitOfs += 8;
         if (bitOfs == 64)
         {
            bitOfs = 0;
            qwordOfs++;
         }
      }

      mBits[0] = q0;
      mBits[1] = q1;
   }
};

//==============================================================================
// struct BUGXGeomRenderInstanceAttributes
// 16 byte aligned
//==============================================================================
__declspec(align(16))
struct BUGXGeomRenderInstanceAttributes
{
   XMMATRIX                      mWorldMatrix;
   
   BUGXGeomRenderMeshMask        mMeshMask;
   
   BVisualRenderAttributes       mVisualAttributes;

   // "Vertex" offset into GPU frame storage of packed bones, relative to the start of the frame's GPU frame storage.
   // First bones is for extra attributes, second bone is model to world, rest are (optional) skeleton matrices.
   DWORD                         mBoneVertexIndex;
      
   union
   {
      BEventReceiverHandle       mUGXGeomHandle;

      struct 
      {
         granny_model_instance*  mpModelInstance;
         BGrannyInstance*        mpGrannyInstance;
         ushort                  mModelIndex;
         bool                    mHasIKNodes;
      } mMainThread;
   };
};

