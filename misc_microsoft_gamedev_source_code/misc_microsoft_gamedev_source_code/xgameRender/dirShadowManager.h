//============================================================================
//
// File: dirShadowManager.h
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "volumeCuller.h"
#include "containers\staticArray.h"
#include "math\generalVector.h"
#include "math\vectorInterval.h"
#include "renderThread.h"
#include "effect.h"
#include "atgFont.h"

#include "ugxGeomInstanceManager.h"
#include "terrain.h"

class BFXLEffectFileLoader;

#define DIR_SHADOW_MANAGER_DEBUG 0

#ifdef BUILD_FINAL
   #undef DIR_SHADOW_MANAGER_DEBUG
   #define DIR_SHADOW_MANAGER_DEBUG 0
#endif

//============================================================================
// class BDirShadowManager
//============================================================================
class BDirShadowManager : public BRenderCommandListener
{
public:
   BDirShadowManager();
   ~BDirShadowManager();
   
   void init(void);
   void deinit(void);

   // All methods callable from the render thread.
   
   void allocateShadowBuffers(void);
   void releaseShadowBuffers(void);
   
   enum BShadowMode
   {
      cSMFPS,
      cSMRTS,
      cSMSingleMap,
      cSMHighestQuality,
      cSMMegaScreenshot,
   };
   
   void shadowGenPrep(const BVec3& worldMin, const BVec3& worldMax, BShadowMode shadowMode, bool enableZExtentOptimization);
   
   const BVolumeCuller& getPassVolumeCuller(uint pass) const { BDEBUG_ASSERT(pass < cMaxPasses); return mVolumeCullers[pass]; }
   const XMMATRIX& getPassWorldToProj(uint pass) const { BDEBUG_ASSERT(pass < cMaxPasses); return mWorldToProj[pass]; }
   const BFrustum& getPassFrustum(uint pass) const { BDEBUG_ASSERT(pass < cMaxPasses); return mFrustums[pass]; }

   uint getNumPasses(void) const;         
   
   IDirect3DArrayTexture9* getShadowBuffer(void) const { return mpShadowBuffer; }
      
   void shadowGenInit(void);
   
   void shadowGenBeginPass(uint pass);

   bool getInShadowGenPass(void) const { return mInShadowPass; }
   
   void shadowGenEndPass(uint pass);
   
   void shadowGenDeinit(void);

#ifndef BUILD_FINAL
   void debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed);
   void debugDraw(ATG::Font& font);
#endif   
      
private:
   XMMATRIX mTextureScale;
   
   enum { cMaxPasses = 8 };
   
   uint mNumPasses;
   uint mFinalCSMPassIndex;
   
   uint mWidth;
   uint mHeight;
   
   AABB mWorldBounds;
         
   XMMATRIX mWorldToView[cMaxPasses];
   XMMATRIX mViewToProj[cMaxPasses];
   XMMATRIX mWorldToProj[cMaxPasses];
   
   XMMATRIX mPrevWorldToView[cMaxPasses];
   XMMATRIX mPrevViewToProj[cMaxPasses];
               
   BVolumeCuller mVolumeCullers[cMaxPasses];
      
   BFrustum mFrustums[cMaxPasses];
   
   BVec3 mViewMin[cMaxPasses];
   BVec3 mViewMax[cMaxPasses];
   float mMinZ[cMaxPasses];
   float mMaxZ[cMaxPasses];
   
   enum { cMaxInclusionPlanes = 16 };
   XMVECTOR mInclusionPlanes[cMaxInclusionPlanes];
   uint mNumInclusionPlanes;
   
   IDirect3DSurface9* mpRenderTarget;
   IDirect3DSurface9* mpDepthStencil;
   IDirect3DArrayTexture9* mpShadowBuffer; 
         
   BFXLEffectFileLoader* mpEffectLoader;
   BFXLEffectTechnique mHFilterTechnique;
   BFXLEffectTechnique mVFilterTechnique;
   BFXLEffectTechnique m2DFilterTechnique;
   
   BShadowMode mPrevShadowMode;
               
   bool mInShadowPass;
      
   typedef BStaticArray<BVec3, 128> BVec3StaticArray;
   BVec3StaticArray mWorldPoints[cMaxPasses];
   BVec3StaticArray mViewPoints[cMaxPasses];
      
   static bool calcClippedPolyhedronVerts(
      BVec3StaticArray& points, 
      float worldMinY = 0.0f, float worldMaxY = 0.0f, 
      float extraNearClipDist = 0.0f, 
      const Plane* pExtraPlanes = NULL, uint numExtraPlanes = 0);
   static void calcWorldPolyhedronVerts(BVec3StaticArray& points, float dist, float worldMinY, float worldMaxY, float lowY, float highY, float midY);

   void clear(void);
   
   void computeFocus(float& lowY, float& highY, float& midY);
   
   void releaseTempBuffers(void);
   void resolveShadowBufferSurface(uint pass);
   
   void shadowGenPrepForPass(const BVec3& worldMin, const BVec3& worldMax, float lowY, float highY, float midY, float dist, uint pass, bool cascaded);
   void updateTextureScaleMatrix(uint width, uint height);
   void updateInclusionPlanes(void);
   
   bool tickEffect(void);
   
   static XMMATRIX computeZScaleMatrix(float minZ, float maxZ);

   void sceneIteratorCallback(XMVECTOR worldMin, XMVECTOR worldMax, bool terrainChunk);
   static bool terrainIteratorCallback(const BTerrain::BSceneIterateParams& params, uint chunkX, uint chunkY, XMVECTOR worldMin, XMVECTOR worldMax, void* pData);
   static bool ugxIteratorCallback(const BUGXGeomInstanceManager::BSceneIterateParams& params, uint instanceIndex, XMVECTOR worldMin, XMVECTOR worldMax, void* pData);            

   void updateMatricesAndVolumerCullers(bool snapTranslation);
   void optimizeZExtents(const BVec3& worldMin, const BVec3& worldMax, bool enableZExtentOptimization);

#ifndef BUILD_FINAL
   int mDebugCurPage;   
   bool mDebugDisplay;
   void drawDebugFrustum(const BMatrix44& frustProjToWorld, const BMatrix44& worldToProj, DWORD nearColor, DWORD farColor, DWORD connectColor, uint xOfs, uint yOfs, uint rectWidth, uint rectHeight);
#endif   

   // BRenderCommandListener interface
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader &header, const uchar *pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

extern BDirShadowManager gDirShadowManager;
