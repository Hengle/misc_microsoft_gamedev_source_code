//============================================================================
// File: localShadowManager.h
//
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math\generalVector.h"
#include "volumeCuller.h"
#include "visibleLightManager.h"
#include "atgFont.h"

//============================================================================
// class BLocalShadowManager
//============================================================================
class BLocalShadowManager
{
public:
   BLocalShadowManager();
   ~BLocalShadowManager();
   
   void init(void);
   void deinit(void);
   
   // All methods callable from the render thread.
   void shadowGenPrep(const BVec3& worldMin, const BVec3& worldMax, float focusHeight);

   uint getNumPasses(void) const { return mNumPasses; }
   uint getCurPass(void) const { return mCurPass; }
   bool getInShadowGenPass(void) const { return mCurPass >= 0; }      

   int getPassVisibleLightIndex(uint pass) const { BDEBUG_ASSERT(pass < cMaxPasses); return mVisibleLightIndices[pass]; }
   const BVolumeCuller& getPassVolumeCuller(uint pass) const { BDEBUG_ASSERT(pass < mNumPasses); return mPassParams[pass].mVolumeCuller; }
               
   void shadowGenInit(void);
   
   void shadowGenBegin(uint pass, bool& dualParaboloid);
   
   void shadowGenEnd(uint pass);
   
   void shadowGenDeinit(void);
      
#ifndef BUILD_FINAL
   void debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed);
   void debugDraw(ATG::Font& font);
#endif   
   
private:
   IDirect3DSurface9* mpRenderTarget;
   IDirect3DSurface9* mpDepthStencil;
   IDirect3DArrayTexture9* mpShadowBufTexArray;  
   
   uint mNumPasses;
   int mCurPass;
   
   bool mInitialized : 1;
   bool mInShadowGenPass : 1;
   
   // Must match MAX_LOCAL_SHADOW_BUFFER_SLICES in localLightingRegs.inc!
   enum { cMaxSlices = 8 };
   DWORD mSliceUsedFlags[cMaxSlices];
               
   enum { cMaxPasses = 32 };
   ushort mVisibleLightIndices[cMaxPasses];
   
   struct BPassParams 
   {
      XMMATRIX          mWorldToView;
      XMMATRIX          mViewToProj;
            
      BVolumeCuller     mVolumeCuller;
      
      DWORD             mLightID;
            
      WORD              mU;
      WORD              mV;
      WORD              mSize;
      uchar             mSlice;
      uchar             mPassIndex;
            
      eLocalLightType   mLightType;
      
      void clear(void)
      {
         Utils::ClearObj(mWorldToView);
         Utils::ClearObj(mViewToProj);
         mVolumeCuller.clear();
         mLightID = cInvalidLocalLightID;
         mU = mV = mSize = 0;
         mSlice = 0;
         mPassIndex = 0;
         mLightType = cInvalidLightType;
      }
   };
         
   BPassParams mPassParams[cMaxPasses];
            
   struct BCachedLight
   {
      DWORD    mLightID;
      uint     mLastFrameUpdated;
      
      WORD     mU;
      WORD     mV;
      WORD     mSize;
      
      char     mSlice;
      uchar    mNumSlices;
      uchar    mPageFlags;
      uchar    mUVBoundsIndex;
      
      bool isUsed(void) const
      {  
         return mLightID != cInvalidLocalLightID;
      }
            
      void clear(void)
      {
         mLightID = cInvalidLocalLightID;
         mLastFrameUpdated = UINT_MAX;
         
         mU = 0;
         mV = 0;
         mSize = 0;
         
         mSlice = -1;
         mNumSlices = 0;
         mPageFlags = 0;
         mUVBoundsIndex = 0;
      }
   };
   
   enum { cMaxCachedLights = 64 };   
   BCachedLight mCachedLights[cMaxCachedLights];
   
#ifndef BUILD_FINAL
   bool mDebugDisplay;
   LONG mDebugSlice;
#endif   
   
   uint getSliceUsedPageFlags(uint slice) const;
   void setSliceUsedPageFlags(uint slice, uint val, uint bitMask);
   bool findAvailableSlicePair(uint& slice);
   bool findAvailableSinglePage(uint& slice, uint& pageIndex, uint& pageBitMask);
   int findAvailableCachedLightSlot(void);

   void addPass(
      uint visibleLightIndex, uint activeLightIndex, 
      uint slice, uint numSlices, uint u, uint v, uint size, uint uvPageIndex);
};

extern BLocalShadowManager gLocalShadowManager;






