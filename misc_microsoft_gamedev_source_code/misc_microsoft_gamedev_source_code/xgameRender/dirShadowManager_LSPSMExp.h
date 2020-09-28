//============================================================================
//
// File: dirShadowManager.h
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "containers\staticArray.h"
#include "math\generalVector.h"

typedef BStaticArray<BVec3, 64> BPointArray;

class BDirShadowManager
{
public:
   BDirShadowManager();
   ~BDirShadowManager();
   
   void init(void);
   
   void deinit(void);
         
   void shadowGenBegin(float worldMinY, float worldMaxY, int pass);

   void shadowGenEnd(int pass);
      
   bool getInShadowGenPass(void) const { return mInShadowPass; }

#ifndef BUILD_FINAL   
   void drawDebugInfo(void);
#endif   

   uint getNumPasses(void) const { return cMaxPasses; }
   
private:
   XMMATRIX mTextureScale;
   
   enum { cMaxPasses = 3 };
         
   XMMATRIX mWorldToView[cMaxPasses];
   XMMATRIX mViewToProj[cMaxPasses];
   XMMATRIX mWorldToProj[cMaxPasses];
   
   XMMATRIX mPrevWorldToView[cMaxPasses];
   XMMATRIX mPrevViewToProj[cMaxPasses];
   
   XMMATRIX mWorldToTex[cMaxPasses];
   
   BVec3 mViewMin[cMaxPasses];
   BVec3 mViewMax[cMaxPasses];
      
   BPointArray mWorldPoints[cMaxPasses];
   BPointArray mViewPoints[cMaxPasses];
         
   IDirect3DSurface9* mpRenderTarget;
   IDirect3DArrayTexture9* mpShadowBuffer;
   
   bool mInShadowPass;
   
   static void calcClippedPolyhedron(BPointArray& points, float worldMinY, float worldMaxY, float extraNearClipDist);
   static void calcWorldPolyhedron(BPointArray& points, float dist);
   
   void shadowGenPrep(float worldMinY, float worldMaxY, int pass);
   void clear(void);
   void updateTextureScaleMatrix(void);
};

extern BDirShadowManager gDirShadowManager;
