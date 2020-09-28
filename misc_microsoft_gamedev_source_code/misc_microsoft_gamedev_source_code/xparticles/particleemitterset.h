//============================================================================
// particleemitterset.h
// Copyright (c) 2000-2006 Ensemble Studios
//============================================================================
#pragma once

#include "visualitem.h"
#include "particleemitter.h"
#include "visualinstance.h"

//----------------------------------------------------------------------------
// Callable from the render thread only!
//----------------------------------------------------------------------------
class BParticleEffect
{
   public:
      BParticleEffect();
     ~BParticleEffect();

      void updateWorldMatrix(BMatrix worldMatrix, const BMatrix* RESTRICT pLocalMatrix);
      void setSecondaryTransform(BMatrix transform);
      void setFlag          (int flag, bool bState);
      void setTintColor(DWORD color);
      
      BDynamicRenderArray<BParticleEmitter*> mEmitters;
      
#ifndef BUILD_FINAL
      // These are only used for debug tools and reloading.  Do not enable in final builds
      XMMATRIX mTransform;
      XMMATRIX mSecondaryTransform;
      DWORD    mTintColor;
#endif

      int mDataIndex;
      int mPriority;
};

