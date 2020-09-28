//==============================================================================
// tonemapManager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "threading\eventDispatcher.h"

#include "effect.h"
#include "renderThread.h"

//==============================================================================
// class BToneMapParams
//==============================================================================
class BToneMapParams
{
public:
   BToneMapParams() 
   {
      clear();
   }
   
   void clear(void)
   {
      mRTransform = BVector(1.0f, 0.0f, 0.0f);
      mGTransform = BVector(0.0f, 1.0f, 0.0f);
      mBTransform = BVector(0.0f, 0.0f, 1.0f);

      mMiddleGrey = .4f;
      mBrightMaskThresh = .9f;
      
      mBloomIntensity = .65f;
      mBloomSigma = 2.0f;
      mAdaptationRate = .06f;
      
      mLogAveMin = .5f;
      mLogAveMax = 250.0f;
      mWhitePointMin = 30.0f;
      mWhitePointMax = 250.0f;
      
      mQuarterResBlooms = true;
      
      mDOFFarBlurPlaneDist = 250.0f;
      mDOFFocalPlaneDist = 100.0f;
      mDOFNearBlurPlaneDist = -50.0f;
      mDOFMaxBlurriness = 1.0f;

      mColorTransformFactor = 0.0f;
      mBlurFactor = 0.0f;
      mBlurX = 0.0f;
      mBlurY = 0.0f;
      mRadialBlurScale = 0.0f;
      mRadialBlurMax = 0.0f;
      
      mDOFEnabled = false;
      mRadialBlur = false;
   }

   bool directionalOrRadialBlurring() const;
   bool radialBlurring() const;
   void enforceLimits(void);

   BVector mRTransform;
   BVector mGTransform;
   BVector mBTransform;

   float mMiddleGrey;
   float mBrightMaskThresh;
   
   float mBloomIntensity;
   float mBloomSigma;
   
   float mAdaptationRate;
   float mLogAveMin;
   float mLogAveMax;
   float mWhitePointMin;
   float mWhitePointMax;
   
   float mDOFFarBlurPlaneDist;
   float mDOFFocalPlaneDist;
   float mDOFNearBlurPlaneDist;      
   float mDOFMaxBlurriness;

   float mColorTransformFactor;
   float mBlurFactor;
   float mBlurX;
   float mBlurY;
   float mRadialBlurScale;
   float mRadialBlurMax;
   
   bool mDOFEnabled : 1;
   bool mQuarterResBlooms : 1;
   bool mRadialBlur : 1;
};

//==============================================================================
// class BToneMapManager
//==============================================================================
class BToneMapManager : public BEventReceiver, BRenderCommandListener
{
public:
   BToneMapManager();
   ~BToneMapManager();

   // Sim thread only.
   void init(void);
   void deinit(void);

   // Sim/render threads.
   bool isInitialized(void) const { return mInitialized; }

   // Render threads only.
   void beginFrame(void);
   
   // pDistortionTexture may be NULL
   void tonemap(IDirect3DTexture9* pDistortionTexture, bool resetAdapation, bool disableAdaptation, uint xOfs, uint yOfs, bool unswizzleRadianceBuf, uint viewportIndex);
   
   // Sim/render threads.
   const BToneMapParams& getParams(uint viewportIndex) const;
   void setParams(const BToneMapParams& params, uint viewportIndex);
   
   // fillDepthStencilSurface() restores the device's depth buffer in EDRAM.
   // rg FIXME: This should be moved somewhere else (tiled AA manager?)
   void fillDepthStencilSurface(IDirect3DSurface9* pDstColor, IDirect3DSurface9* pDstDepth, uint width, uint height, bool unswizzleRadianceBuf);
   void fillColorSurface(IDirect3DSurface9* pDstColor, IDirect3DSurface9* pDstDepth, uint width, uint height, bool unswizzleRadianceBuf);
   
   bool getDOFEnabled(void) const { return mDOFEnabled; }
   void setDOFEnabled(bool enabled) { mDOFEnabled = enabled; }
      
   bool getPostEffectsEnabled(void) const { return mPostEffectsEnabled; }
   void setPostEffectsEnabled(bool enabled) { mPostEffectsEnabled = enabled; }

private:
   BFXLEffect mToneMapEffect;
   
   uint mWidthDiv2, mHeightDiv2;
   uint mWidthDiv4, mHeightDiv4;
   uint mWidthDiv16, mHeightDiv16;
   uint mWidthDiv64, mHeightDiv64;
   
   IDirect3DSurface9* mpRadSurf2;
   IDirect3DTexture9* mpRadTex2;
   
   IDirect3DSurface9* mpRadSurf4;
   IDirect3DTexture9* mpRadTex4;
   
   IDirect3DSurface9* mpAveLumSurf16;
   IDirect3DTexture9* mpAveLumTex16;
   
   IDirect3DSurface9* mpAveLumSurf64;
   IDirect3DTexture9* mpAveLumTex64;
   
   IDirect3DSurface9* mpAveLumSurf1;
   IDirect3DTexture9* mpAveLumTex1[2];
   uint mCurLumTexIndex;
   
   IDirect3DSurface9* mpBrightMaskSurf;
   enum { cNumTempBufs = 3 };
   IDirect3DTexture9* mpBloomBuf[cNumTempBufs];
   
   IDirect3DTexture9* mpDOFBlurTex;
   IDirect3DSurface9* mpDOFBlurSurf;

   enum { cNumViewports = 2 };
   BToneMapParams mParams[cNumViewports];
   BToneMapParams mSimParams[cNumViewports];
   uint mCurrentViewportIndex;
                  
   bool mInitialized : 1;
   bool mDOFEnabled : 1;
   bool mPostEffectsEnabled : 1;
      
   void loadEffect(void);
   void initEffectParams(void);
   void clearTexture(IDirect3DTexture9* pTex);
   
   void releaseWorkTextures(void);
   void allocateWorkTextures(void);
   
   void renderQuad(int x, int y, int width, int height, float ofsX = -.5f, float ofsY = -.5f, bool grid = false, float uLo0 = 0.0f, float uHi0 = 1.0f, float vLo0 = 0.0f, float vHi0 = 1.0f, float uLo1 = 0.0f, float uHi1 = 1.0f, float vLo1 = 0.0f, float vHi1 = 1.0f);
   void computeAdaptation(bool resetAdapation, bool disableAdaptation);
   void finalPass(IDirect3DTexture9* pDistortionTexture, uint xOfs, uint yOfs, bool unswizzleRadianceBuf);
   void reduct4(IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, uint dstWidth, uint dstHeight, bool grid, uint techniqueIndex);
   void reduct2(IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, uint dstWidth, uint dstHeight, bool grid, uint techniqueIndex);
   void reduct1(IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, uint techniqueIndex);
   void brightMask(void);
   void bloomFilter(void);
   void filterAdaptation(bool resetAdapation, bool disableAdaptation);
   void DOFBlur(void);
      
   enum
   {
      cRenderCommandSetParams = 0,
   };
      
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   virtual void initDeviceData(void);
   virtual void frameBegin(void);

   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

extern BToneMapManager gToneMapManager;
