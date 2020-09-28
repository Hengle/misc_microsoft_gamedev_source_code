//============================================================================
// File: nearLayerManager.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

#include "atgFont.h"

class BCamera;

//============================================================================
// BNearLayerManager
// Manages the reflection pass buffers and setup
//============================================================================

class BNearLayerManager
{
   public:
      BNearLayerManager();
      ~BNearLayerManager();

      // Allocate / release texture
      void                 allocateBuffer();
      void                 releaseBuffer();

      // Render begin/end - allocs render target / depth buffer and sets up camera / viewport to reflect
      // about the reflectionPlane
      void                 renderBegin();
      void                 renderEnd();

      bool                 isEnabled() const { return mEnabled; }
      void                 composite();

   protected:

      void                 setMatricesAndViewport();

      IDirect3DSurface9*   mpRenderTarget;
      IDirect3DSurface9*   mpDepthStencil;
      IDirect3DTexture9*   mpTexture;
      uint                 mWidth;
      uint                 mHeight;
      D3DFORMAT            mRenderFormat;
      D3DFORMAT            mDepthFormat;
      D3DFORMAT            mTextureFormat;
      D3DMULTISAMPLE_TYPE  mMultisampleType;
      bool                 mEnabled : 1;
      bool                 mFullScreenDraw : 1;
};

extern BNearLayerManager gNearLayerManager;