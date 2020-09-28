//============================================================================
// File: occlusion.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================
#pragma once

#include "atgFont.h"

//-------------------------------------------
class BOcclusionManager
{
public:
   enum eOcclusionResult
   {
      eOR_FullyHidden =0,
      eOR_FullyVisible,
      eOR_Indeterminate,
   };


   BOcclusionManager();
   ~BOcclusionManager();

   void     destroy();

   // Allocate / release reflection buffer texture
   void     allocateOcclusionBuffer();
   void     releaseOcclusionBuffer();

   void     generateOcclusionBuffer();
   eOcclusionResult     testOcclusion(BVector min, BVector max,XMMATRIX transform);


   void     clearOcclusionBuffer(){mDoClearOcclusionBuffer = true;}

   #ifndef BUILD_FINAL
   void              toggleDebugDraw() {mDebugDraw = !mDebugDraw;}
   bool              isDebugDrawEnabled() const { return mDebugDraw; }
   void              debugDraw(ATG::Font& font);

   #endif

   void              toggleOcclusionTestEnabled() { mOcclusionTestEnabled = !mOcclusionTestEnabled;}
   bool              isOcclusionTestEnabled()const { return mOcclusionTestEnabled; }
private:

   void     preGenerateOcclusionBuffer();
   void     postGenerateOcclusionBuffer();

   void     renderTerrainToBuffer();
   void     renderOccluderObjects();

   void     resolvePackDepthBuffer();
   void     calcScreenSpaceBBox(BVector min, BVector max,XMMATRIX transform, XMVECTOR &frontDist, XMVECTOR &minBounds, XMVECTOR &maxBounds);

   void     clearOcclusionBufferInternal();

   int                  mBufferWidth;
   int                  mBufferHeight;
   int                  mNumXMVectorsX;
   int                  mNumXMVectorsY;
   D3DFORMAT            mRenderFormat;
   D3DFORMAT            mDepthFormat;
   D3DFORMAT            mTextureFormat;
   D3DMULTISAMPLE_TYPE  mMultisampleType;
   IDirect3DSurface9*   mpRenderTarget;
   IDirect3DSurface9*   mpDepthStencil;
   IDirect3DTexture9*   mpOcclusionBufferTexture;

   XMVECTOR*            mpOcclusionBuffer;
   XMMATRIX             mWorldToScreen;
   XMVECTOR             mCameraPos;
   float*               mpRenderTargetPointer;

   bool                 mOcclusionTestEnabled : 1;
   bool                 mDoClearOcclusionBuffer : 1;
#ifndef BUILD_FINAL
   bool              mDebugDraw : 1;
#endif

};

extern BOcclusionManager gOcclusionManager;