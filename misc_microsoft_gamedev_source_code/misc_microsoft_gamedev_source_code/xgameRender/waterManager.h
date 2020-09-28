//============================================================================
// File: waterManager.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

#include "atgFont.h"

class BCamera;

//============================================================================
// BWaterManager
// Manages the reflection pass buffers and setup
//============================================================================

class BWaterManager
{
   public:
      BWaterManager();
      ~BWaterManager();

      // Allocate / release reflection buffer texture
      void                 allocateReflectionBuffer();
      void                 releaseReflectionBuffer();

      // Reflection render begin/end - allocs render target / depth buffer and sets up camera / viewport to reflect
      // about the reflectionPlane
      void                 reflectionRenderBegin();
      void                 reflectionRenderEnd();

      void                 setReflectionPlane(XMVECTOR reflectionPlane) { mReflectionPlane = reflectionPlane; }
      void                 setReflectionObjMinMax(XMVECTOR objmin, XMVECTOR objmax) { mReflectionObjMin = objmin; mReflectionObjMax = objmax;}
      void                 enableReflectionBufferUpdate(bool enable) { mUpdateReflectionBuffer = enable; }
      bool                 isReflectionBufferUpdateEnabled() const { return mUpdateReflectionBuffer; }

      // Debug functions
      #ifndef BUILD_FINAL
         enum eRenderMode
         {
            cRenderNone    = 0,
            cRenderTerrain = 1,
            cRenderUnits   = 2,
            cRenderAll     = (cRenderTerrain | cRenderUnits)
         };

         eRenderMode       getRenderMode() const { return mRenderMode; }
         bool              isDebugDrawEnabled() const { return mDebugDraw; }
         void              debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed);
         void              debugDraw(ATG::Font& font);
      #endif

   protected:

      void                 setReflectionMatricesAndViewport(XMVECTOR reflectionPlane);

      XMVECTOR             mReflectionPlane;
      XMVECTOR             mReflectionObjMin;
      XMVECTOR             mReflectionObjMax;

      uint                 mWidth;
      uint                 mHeight;
      D3DFORMAT            mRenderFormat;
      D3DFORMAT            mDepthFormat;
      D3DFORMAT            mTextureFormat;
      D3DMULTISAMPLE_TYPE  mMultisampleType;
      IDirect3DSurface9*   mpRenderTarget;
      IDirect3DSurface9*   mpDepthStencil;
      IDirect3DTexture9*   mpReflectionBuffer;
      bool                 mUpdateReflectionBuffer;

      #ifndef BUILD_FINAL
         eRenderMode       mRenderMode;
         bool              mDebugDraw : 1;
         bool              mFullScreenDebugDraw : 1;
      #endif
};

extern BWaterManager gWaterManager;