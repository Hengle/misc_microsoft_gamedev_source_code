//============================================================================
//
// File: visibleLightManager.cpp
//  
// Copyright (c) 2006-2008, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "visibleLightManager.h"
#include "renderDraw.h"
#include "volumeIntersection.h"
#include "effectIntrinsicManager.h"
#include "grannyInstanceRenderer.h"
#include "math\VMXIntersection.h"
#include "render.h"
#include "gpuHeap.h"

#include "debugPrimitives.h"
#include "primDraw2D.h"
#include "math\plane.h"

#define LIGHT_BUFFER_SHADER_FILENAME "lightBuffer\\lightBuffer.bin"

//============================================================================
// Globals
//============================================================================
BVisibleLightManager gVisibleLightManager;

//============================================================================
// BVisibleLightManager::BVisibleLightManager
//============================================================================
BVisibleLightManager::BVisibleLightManager() :
   mInitialized(false),
   mpLightTexture(NULL),
   mpVisibleLightTexels(NULL),
   mWorldMinY(0.0f),
   mWorldMaxY(0.0f),
   mLightObjectLinkedArrayManager(mLightObjectArrayAllocator),
   mNumShadowedLights(0),
   mLocalLightingEnabled(true),
   mLightBufferingEnabled(true),
   mpLightBufferEffect(NULL),
   mpAccumLightVertexDecl(NULL),
   mpRenormVertexDecl(NULL)
{
   Utils::ClearObj(mpLightBuffers);
   mLightBufferBounds.clear();
   Utils::ClearObj(mWorldToLightBuffer);
   Utils::ClearObj(mLightBufferToWorld);
   Utils::ClearObj(mWorldFocusMin);
   Utils::ClearObj(mWorldFocusMax);
}

//============================================================================
// BVisibleLightManager::~BVisibleLightManager
//============================================================================
BVisibleLightManager::~BVisibleLightManager()
{
}

//============================================================================
// BVisibleLightManager::init
//============================================================================
void BVisibleLightManager::init(void)
{
   if (mInitialized)
      return;

   mpLightTexture = NULL;

   mpLightTexture = NULL;
   
   mWorldMinY = 0.0f;
   mWorldMaxY = 0.0f;

   clear();

   commandListenerInit();

   mInitialized = true;
}

//============================================================================
// BVisibleLightManager::deinit
//============================================================================
void BVisibleLightManager::deinit(void)
{
   if (!mInitialized)
      return;
   
   gRenderThread.blockUntilGPUIdle();
   
   commandListenerDeinit();

   mVisibleLights.clear();
   mVisibleLightState.clear();
   mActiveToVisibleLights.clear();
   mLightBufferedLights.clear();

   mInitialized = false;
}

//============================================================================
// BVisibleLightManager::clear
//============================================================================
void BVisibleLightManager::clear(void)
{
   mVisibleLights.reserve(cMaxExpectedLocalLights);
   mVisibleLights.resize(0);

   mVisibleLightState.resize(cMaxExpectedLocalLights);

   mVisibleLightShadows.resize(cMaxExpectedLocalLights, 1);
   mVisibleLightShadows.setAll(0);

   mActiveToVisibleLights.resize(cMaxExpectedLocalLights);
   mActiveToVisibleLights.setAll(USHRT_MAX);
   
   mShadowedLightObjectsArray.resize(cMaxExpectedLocalLights);
   mShadowedLightObjectsArray.setAll(NULL);
   
   mLightBufferedLights.clear();
}

//============================================================================
// BVisibleLightManager::findLights
//============================================================================
void BVisibleLightManager::findLights(BSceneLightManager::BActiveLightIndexArray& activeLightIndices, BSceneLightManager::BGridRect gridRect, XMVECTOR min, XMVECTOR max, bool refine, bool visibleLightIndices)
{
   //ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(mInitialized);

   gRenderSceneLightManager.getLightGrid().getAllObjects(&activeLightIndices, gridRect);

   if (refine)
   {
      XMVECTOR boundingSphere = XMVectorMultiply(XMVectorAdd(min, max), XMVectorReplicate(.5f));
      boundingSphere = __vrlimi(boundingSphere, XMVector3LengthEst(XMVectorSubtract(max, boundingSphere)), VRLIMI_CONST(0, 0, 0, 1), 0);

      uint dstIndex = 0;

      for (uint i = 0; i < activeLightIndices.size(); i++)
      {
         const uint activeLightIndex = activeLightIndices[i];
         const BBaseLocalLightParams& baseParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(activeLightIndex);

         if (!BVMXIntersection::sphereVsAABB(baseParams.getPosRadius(), min, max))
            continue;

         if (baseParams.isSpotLight())
         {
            XMVECTOR atSpotOuter = baseParams.getAtSpotOuter();
            const bool touching = BVMXIntersection::coneSphere(baseParams.getPosRadius(), atSpotOuter, .5f * atSpotOuter.w, boundingSphere);
            if (!touching)
               continue;
         }

         if (visibleLightIndices)
         {
            const uint visibleLightIndex = mActiveToVisibleLights[activeLightIndex];
            if (visibleLightIndex != USHRT_MAX)
            {
               activeLightIndices[dstIndex] = (WORD)visibleLightIndex;
               dstIndex++;
            }
         }
         else
         {
            activeLightIndices[dstIndex] = (WORD)activeLightIndex;
            dstIndex++;
         }
      }   

      activeLightIndices.resize(dstIndex);
   }
   else if (visibleLightIndices)
   {
      uint dstIndex = 0;

      for (uint i = 0; i < activeLightIndices.size(); i++)
      {
         const uint activeLightIndex = activeLightIndices[i];

         const uint visibleLightIndex = mActiveToVisibleLights[activeLightIndex];
         if (visibleLightIndex != USHRT_MAX)
         {
            activeLightIndices[dstIndex] = (WORD)visibleLightIndex;
            dstIndex++;
         }
      }   

      activeLightIndices.resize(dstIndex);
   }
}

//============================================================================
// BVisibleLightManager::calcSceneFocusRegion
//============================================================================
bool BVisibleLightManager::calcSceneFocusRegion(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   const BFrustum& worldFrustum = gRenderDraw.getWorkerSceneMatrixTracker().getWorldFrustum();

   Plane planes[8];
   Utils::FastMemCpy(planes, &worldFrustum.plane(0), 6 * sizeof(Plane));
   planes[6].setFromNormalOrigin(BVec3(0.0f, 1.0f, 0.0f), BVec3(0.0f, mWorldMinY, 0.0f));
   planes[7].setFromNormalOrigin(BVec3(0.0f, -1.0f, 0.0f), BVec3(0.0f, mWorldMaxY, 0.0f));
   typedef BStaticArray<BVec3, 16> BVec3Array;
   BVec3Array frustumVerts;
   calcVolumeIntersection<BVec3Array, 256>(frustumVerts, planes, 8);
   if (frustumVerts.isEmpty())
      return false;

   XMVECTOR min = XMVectorReplicate(1e+10f);
   XMVECTOR max = XMVectorNegate(min);

   for (uint i = 0; i < frustumVerts.size(); i++)
   {
      XMVECTOR v = XMLoadVector3(&frustumVerts[i]);
      min = XMVectorMin(min, v);
      max = XMVectorMax(max, v);
   }

   mWorldFocusMin = min;
   mWorldFocusMax = max;

   return true;
}

//============================================================================
// BVisibleLightManager::findVisibleLights
//============================================================================
void BVisibleLightManager::findVisibleLights(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   mVisibleLights.resize(0);
   mLightBufferedLights.resize(0);

   Utils::FastMemSet(mActiveToVisibleLights.getPtr(), 0xFF, mActiveToVisibleLights.getSizeInBytes());   
   
   if (!calcSceneFocusRegion())
      return;
      
   if (!mLocalLightingEnabled)
      return;
      
   findLights(mVisibleLights, gRenderSceneLightManager.getGridRect(mWorldFocusMin, mWorldFocusMax), mWorldFocusMin, mWorldFocusMax, false);

   if (mVisibleLights.isEmpty())
      return;

   std::sort(mVisibleLights.begin(), mVisibleLights.end());

   Utils::BPrefetchState prefetchState = Utils::BeginPrefetchLargeStruct(&gRenderSceneLightManager.getActiveLocalLightBaseParams(mVisibleLights[0]), gRenderSceneLightManager.getBaseLocalLightParams().end(), 6);

   const BVolumeCuller& volumeCuller = gRenderDraw.getWorkerSceneVolumeCuller();
   BDEBUG_ASSERT(volumeCuller.getUsingBasePlanes());

   XMVECTOR frustumVertices[BMatrixTracker::cNumFrustumVertices];
   gRenderDraw.getWorkerSceneMatrixTracker().getFrustumVertices(frustumVertices);

   const XMMATRIX worldToView = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToView);
   const XMMATRIX viewToScreen = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTViewToProj) * gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTProjToScreen);
   const XMMATRIX worldToScreen = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj) * gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTProjToScreen);
      
   uint dstIndex = 0;      
   for (uint i = 0; i < mVisibleLights.size(); i++)
   {
      const uint activeLightIndex = mVisibleLights[i];
      BDEBUG_ASSERT(activeLightIndex < gRenderSceneLightManager.getNumActiveLocalLights());

      const BBaseLocalLightParams& baseParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(activeLightIndex);
      const BLocalLightParams& params = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);
      
      prefetchState = Utils::UpdatePrefetchLargeStruct(&baseParams, gRenderSceneLightManager.getBaseLocalLightParams().end(), 6);

      int result = BVMXIntersection::IntersectSphereFrustum(baseParams.getPosRadius(), volumeCuller.getBasePlanes(), frustumVertices);
      if (result == 0)
         continue;

      const bool spotLight = baseParams.isSpotLight();

      if (spotLight)
      {
         if (BVMXIntersection::cappedConeVsFrustum(volumeCuller.getBasePlanes(), baseParams.getPosRadius(), baseParams.getAtSpotOuter(), baseParams.getSpotOuter() * .5f, baseParams.getRadius()))
            continue;
      }

      XMVECTOR centerRadius = BVMXIntersection::transformSphere(baseParams.getPosRadius(), worldToView);
      XMVECTOR screenCenter;
      XMVECTOR screenRadius;
      BVMXIntersection::eProjectSphereResult projResult = BVMXIntersection::projectSphere(screenCenter, screenRadius, centerRadius, viewToScreen);
      float area = 9999999.0f;
      float radius = USHRT_MAX;
      if (projResult == BVMXIntersection::cPSPartial)
      {
         radius = screenRadius.x;
         area = screenRadius.x * screenRadius.x * Math::fPi;
      }

      if (spotLight)
      {
         XMVECTOR boxMin, boxMax;
         XMMATRIX boxToWorld;
         BVMXIntersection::calculateCappedConeOBB(boxMin, boxMax, boxToWorld, baseParams.getPosRadius(), baseParams.getAtSpotOuter(), baseParams.getSpotOuter() * .5f, baseParams.getRadius());

         XMMATRIX worldToBox;
         XMMatrixInverseByTranspose(worldToBox, boxToWorld);

         float spotBoxArea;
         bool success = BVMXIntersection::calculateBoxArea(spotBoxArea, 
            XMVector3Transform(gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPos(), worldToBox),
            boxMin,
            boxMax,
            boxToWorld,
            worldToScreen);

         if ((success) && (spotBoxArea < area))
            radius = sqrt(spotBoxArea * (1.0f / Math::fPi));
      }            

      radius = Math::Min<float>(radius, USHRT_MAX);

      uint lightFadeFract = 255;            
      uint shadowFadeFract = 255;

      static float lightStartFadeRadius = 15.0f;//50.0f;
      static float lightEndFadeRadius = 30.0f;//150.0f;

      static float shadowStartFadeRadius = 100.0f;
      static float shadowEndFadeRadius = 250.0f;

      lightFadeFract = Math::Clamp(Math::FloatToIntTrunc(255.0f * (radius - lightStartFadeRadius) / (lightEndFadeRadius - lightStartFadeRadius)), 0, 255);
      shadowFadeFract = Math::Clamp(Math::FloatToIntTrunc(255.0f * (radius - shadowStartFadeRadius) / (shadowEndFadeRadius - shadowStartFadeRadius)), 0, 255);

      if (lightFadeFract < 1)
         continue;
      
      if ((mLightBufferingEnabled) && (!gRenderSceneLightManager.getActiveLightShadows(activeLightIndex)) && (params.getLightBuffered()))
      {
         BLightBufferedLight* p = mLightBufferedLights.enlarge(1);
         
         p->mActiveLightIndex = static_cast<WORD>(activeLightIndex);
         p->mState.mLightFadeFract = (uchar)lightFadeFract;
         p->mState.mShadowFadeFract = (uchar)shadowFadeFract;
         p->mState.mCircleRadius = (ushort)Math::FloatToIntTrunc(radius);
      }
      else
      {
         mVisibleLightState[dstIndex].mLightFadeFract = (uchar)lightFadeFract;
         mVisibleLightState[dstIndex].mShadowFadeFract = (uchar)shadowFadeFract;
         mVisibleLightState[dstIndex].mCircleRadius = (ushort)Math::FloatToIntTrunc(radius);

         mActiveToVisibleLights[activeLightIndex] = (WORD)dstIndex;

         mVisibleLights[dstIndex] = (WORD)activeLightIndex;
         dstIndex++;
      }         

      //    gpDebugPrimitives->addDebugSphere(baseParams.mPosRadius, baseParams.mPosRadius.w, 0xFFFFFFFF);
   }   

   mVisibleLights.resize(dstIndex);
}

namespace
{
   void storeHalf4(XMHALF4* pDst, XMHALF4 half)
   {
      *pDst = half;
   }

   void storeHalf(HALF* pDst, HALF half)
   {
      *pDst = half;
   }

   void storeFloat4(XMFLOAT4* pDst, XMVECTOR f)
   {
      XMStoreFloat4NC(pDst, f);
   }

   void storeFloat(float* pDst, float f)
   {
      *pDst = f;
   }
}

//============================================================================
// BVisibleLightManager::updateLightTextures
//============================================================================
void BVisibleLightManager::updateLightTextures(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   updateVisibleLightTexture();
   updateLightBufferTexture();
}

const uint cLightBufferWidth = 128;
const uint cLightBufferHeight = 128;
const uint cLightBufferDepth = 16;
const D3DFORMAT cLightBufferColorTexFormat = D3DFMT_A2R10G10B10;
const D3DFORMAT cLightBufferColorSurfFormat = D3DFMT_A2R10G10B10;

const D3DFORMAT cLightBufferIntermediateVectorTexFormat = D3DFMT_A16B16G16R16F;
const D3DFORMAT cLightBufferIntermediateVectorSurfFormat =  D3DFMT_A16B16G16R16_EDRAM;

const D3DFORMAT cLightBufferVectorTexFormat = D3DFMT_A2R10G10B10;
const D3DFORMAT cLightBufferVectorSurfFormat = D3DFMT_A2R10G10B10;

// Light values are normalized by multiplying by cLightBufferIntensityScale 
const float cLightBufferIntensityScale = 1.0f / 12.0f;

struct BLightBufferVertex
{
   XMVECTOR mPosRadius;
   XMVECTOR mColor;
   XMVECTOR mLightParams; // omniMul, omniAdd, spotMul, spotAdd
   XMVECTOR mSpotAtDecayDist; 
};

//============================================================================
// BVisibleLightManager::getLightBuffer
//============================================================================
IDirect3DVolumeTexture9* BVisibleLightManager::getLightBuffer(uint index) const
{
   if (mLightBufferedLights.isEmpty())
      return NULL;
   
   return mpLightBuffers[index];
}

//============================================================================
// BVisibleLightManager::getLightBufferIntensityScale
//============================================================================
float BVisibleLightManager::getLightBufferIntensityScale(void) const
{
   return 1.0f / cLightBufferIntensityScale;
}

//============================================================================
// BVisibleLightManager::getWorldToLightBuffer
//============================================================================
const XMMATRIX& BVisibleLightManager::getWorldToLightBuffer(void) const
{
   return mWorldToLightBuffer;
}

//============================================================================
// BVisibleLightManager::updateLightBufferParams
//============================================================================
void BVisibleLightManager::updateLightBufferParams(void)
{
   AABB bounds;
   
   bounds.initExpand();

   for (uint i = 0; i < mLightBufferedLights.getSize(); i++)
   {
      const BLightBufferedLight& light = mLightBufferedLights[i];
      const BBaseLocalLightParams& baseParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(light.mActiveLightIndex);
      //const BLocalLightParams& params = gRenderSceneLightManager.getActiveLocalLightParams(light.mActiveLightIndex);      

      XMVECTOR posRadius(baseParams.getPosRadius());

      BVec3 low(posRadius.x - posRadius.w, posRadius.y - posRadius.w, posRadius.z - posRadius.w);
      BVec3 high(posRadius.x + posRadius.w, posRadius.y + posRadius.w, posRadius.z + posRadius.w);

      bounds.expand(low);      
      bounds.expand(high);
   }

   bounds.low() -= BVec3(1.0f, 1.0f, 1.0f);
   bounds.high() += BVec3(1.0f, 1.0f, 1.0f);
   
   mLightBufferBounds = bounds;
   //mLightBufferBounds.low().set(mWorldFocusMin.x, mWorldFocusMin.y, mWorldFocusMin.z);
   //mLightBufferBounds.high().set(mWorldFocusMax.x, mWorldFocusMax.y, mWorldFocusMax.z);
   
   for (uint i = 0; i < 3; i++)
   {
      if (mLightBufferBounds.dimension(i) < .5f)
      {
         mLightBufferBounds.low()[i] -= .25f;
         mLightBufferBounds.high()[i] += .25f;
      }
   }

   mWorldToLightBuffer = XMMatrixTranslation(-mLightBufferBounds.low()[0], -mLightBufferBounds.low()[1], -mLightBufferBounds.low()[2]);
   mWorldToLightBuffer = mWorldToLightBuffer * XMMatrixScaling(1.0f / mLightBufferBounds.dimension(0), 1.0f / mLightBufferBounds.dimension(1), 1.0f / mLightBufferBounds.dimension(2));

   XMMATRIX yzFlip;
   Utils::ClearObj(yzFlip);

   yzFlip._11 = 1.0f;
   yzFlip._32 = 1.0f;
   yzFlip._23 = 1.0f;
   yzFlip._44 = 1.0f;

   mWorldToLightBuffer = mWorldToLightBuffer * yzFlip;

   XMVECTOR det;
   mLightBufferToWorld = XMMatrixInverse(&det, mWorldToLightBuffer);  
}

//============================================================================
// BVisibleLightManager::updateLightBufferTexture
//============================================================================
void BVisibleLightManager::updateLightBufferTexture(void)
{
   if ((mLightBufferedLights.isEmpty()) || (!mpLightBufferEffect))
      return;
   
   updateLightBufferParams();

   const bool effectIsDirty = mpLightBufferEffect->tick();
   
   if (!mpLightBufferEffect->isEffectValid())
      return;
   
   if (effectIsDirty)
   {
      mAccumLightTechnique = mpLightBufferEffect->getFXLEffect().getTechnique("AccumLightTechnique");
      mRenormTechnique = mpLightBufferEffect->getFXLEffect().getTechnique("RenormAndPackTechnique");
      mLightToWorldParam = mpLightBufferEffect->getFXLEffect()("gLightToWorld");
      mWorldToLightParam = mpLightBufferEffect->getFXLEffect()("gWorldToLight");
      mSlicePlaneParam = mpLightBufferEffect->getFXLEffect()("gSlicePlane");
      mLightBufValues0Param = mpLightBufferEffect->getFXLEffect()("gLightBufValues0");
      mLightBufValues1Param = mpLightBufferEffect->getFXLEffect()("gLightBufValues1");
      mLightBufferVectorSampler = mpLightBufferEffect->getFXLEffect()("LightBufferVectorSampler");
   }   
   
   mpLightBufferEffect->getFXLEffect().updateIntrinsicParams();
            
   IDirect3DVertexBuffer9* pVB;
   {
      BScopedPIXNamedEvent scopedPIXEvent("LightBufferFillVB");
   
      pVB = gRenderDraw.createDynamicVB(mLightBufferedLights.getSize() * sizeof(BLightBufferVertex));
      
      BLightBufferVertex* pDstVerts;
      pVB->Lock(0, 0, (void**)&pDstVerts, 0);
      for (uint lightIndex = 0; lightIndex < mLightBufferedLights.getSize(); lightIndex++)
      {
         const BLightBufferedLight& light = mLightBufferedLights[lightIndex];
         const BBaseLocalLightParams& baseParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(light.mActiveLightIndex);
         const BLocalLightParams& params = gRenderSceneLightManager.getActiveLocalLightParams(light.mActiveLightIndex);      
         
         const float radius = baseParams.getRadius();
         const float farAttenStart = Math::Min(.999f, params.getFarAttenStart());

         const float omniOOFalloffRange = 1.0f / (1.0f - farAttenStart);

         const float omniMul = -((1.0f / radius) * omniOOFalloffRange);
         const float omniAdd = 1.0f + farAttenStart * omniOOFalloffRange;
         
         float spotMul = 0.0f;
         float spotAdd = 100.0f;

         if (params.getType() == cLTSpot)
         {
            static float fovFudge = 0.0f;
            const float spotInner = Math::Max(Math::fDegToRad(.0125f), baseParams.getSpotInner() - Math::fDegToRad(fovFudge));
            const float spotOuter = Math::Max(Math::fDegToRad(.025f), baseParams.getSpotOuter() - Math::fDegToRad(fovFudge));
            spotMul = 1.0f / ((cos(spotInner * .5f) - cos(spotOuter * .5f)));
            spotAdd = 1.0f - cos(spotInner * .5f) * spotMul;
         }         
              
         XMVECTOR posRadius = baseParams.getPosRadius();
         XMVECTOR lightParams = XMVectorSet(omniMul, omniAdd, spotMul, spotAdd);
         XMVECTOR spotAtDecayDist = __vrlimi(baseParams.getAtSpotOuter(), XMVectorReplicate(params.getDecayDist()), VRLIMI_CONST(0, 0, 0, 1), 0);
         XMVECTOR color = XMVectorMultiply(params.getColor(), XMVectorReplicate(light.mState.mLightFadeFract * 1.0f/255.0f));
         
         color = __vrlimi(color, XMVectorReplicate(params.getSpecular()), VRLIMI_CONST(0, 0, 0, 1), 0);
         
         pDstVerts[0].mPosRadius = posRadius; _WriteBarrier();
         pDstVerts[0].mColor = color;
         pDstVerts[0].mLightParams = lightParams; _WriteBarrier();
         pDstVerts[0].mSpotAtDecayDist = spotAtDecayDist; _WriteBarrier();
                     
         pDstVerts++;
      }         
      
      pVB->Unlock();
   }      
   
   {
      BScopedPIXNamedEvent scopedPIXEvent("LightBufferRender");
                  
      IDirect3DSurface9 lightBufferColorSurface;
      
      D3DSURFACE_PARAMETERS surfaceParams;
      Utils::ClearObj(surfaceParams);
      surfaceParams.Base = 0;
      XGSetSurfaceHeader(cLightBufferWidth, cLightBufferHeight, cLightBufferColorSurfFormat, D3DMULTISAMPLE_NONE, &surfaceParams, &lightBufferColorSurface, NULL);
      
      IDirect3DSurface9 lightBufferIntermediateVectorSurface;      
      surfaceParams.Base = XGSurfaceSize(cLightBufferWidth, cLightBufferHeight, cLightBufferColorTexFormat, D3DMULTISAMPLE_NONE);
      XGSetSurfaceHeader(cLightBufferWidth, cLightBufferHeight, cLightBufferIntermediateVectorSurfFormat, D3DMULTISAMPLE_NONE, &surfaceParams, &lightBufferIntermediateVectorSurface, NULL);
      
      IDirect3DSurface9 lightBufferVectorSurface;      
      surfaceParams.Base += XGSurfaceSize(cLightBufferWidth, cLightBufferHeight, cLightBufferIntermediateVectorSurfFormat, D3DMULTISAMPLE_NONE);
      XGSetSurfaceHeader(cLightBufferWidth, cLightBufferHeight, cLightBufferVectorSurfFormat, D3DMULTISAMPLE_NONE, &surfaceParams, &lightBufferVectorSurface, NULL);
      
      BD3D::mpDev->SetDepthStencilSurface(NULL);
               
      D3DXVECTOR4 clearColor(0.0f, 0.0f, 0.0f, 0.0f);
      
      mWorldToLightParam = mWorldToLightBuffer;
      mLightToWorldParam = mLightBufferToWorld;
      mLightBufValues0Param = BVec4(static_cast<float>(cLightBufferWidth), static_cast<float>(cLightBufferHeight), cLightBufferIntensityScale, 1.0f / cLightBufferIntensityScale);
      
      IDirect3DTexture9* pIntermediateVectorTexture;
      HRESULT hres = gGPUFrameHeap.createTexture(cLightBufferWidth, cLightBufferHeight, 1, 0, cLightBufferIntermediateVectorTexFormat, 0, &pIntermediateVectorTexture, NULL);
      
      mLightBufferVectorSampler = pIntermediateVectorTexture;
      
      if (!SUCCEEDED(hres))
      {
         BASSERT(0);
      }
      else
      {
         BVec4 quadVertices[4];
         quadVertices[0].set(0.0f, 0.0f, 0.0f, 0.0f);
         quadVertices[1].set((float)cLightBufferWidth, 0.0f, 1.0f, 0.0f);
         quadVertices[2].set((float)cLightBufferWidth, (float)cLightBufferHeight, 1.0f, 1.0f);
         quadVertices[3].set(0.0f, (float)cLightBufferHeight, 0.0f, 1.0f);
         
         for (uint sliceIndex = 0; sliceIndex < cLightBufferDepth; sliceIndex++)
         {
            BD3D::mpDev->SetRenderTarget(0, &lightBufferColorSurface);
            BD3D::mpDev->SetRenderTarget(1, &lightBufferIntermediateVectorSurface);
            
            float normSlice = (sliceIndex + .5f) / cLightBufferDepth;
            float y = Math::Lerp(mLightBufferBounds.low()[1], mLightBufferBounds.high()[1], normSlice);
            
            BVec4 planeEquation(0.0f, 1.0f, 0.0f, y);
                           
            mSlicePlaneParam = planeEquation;
            
            if (sliceIndex == 0)
               BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 0.0f, 0);
               
            mLightBufValues1Param = BVec4(1.0f / cLightBufferWidth, 1.0f / cLightBufferHeight, normSlice, y);
            
            BD3D::mpDev->SetStreamSource(0, pVB, 0, sizeof(BLightBufferVertex));
            BD3D::mpDev->SetVertexDeclaration(mpAccumLightVertexDecl);
            
            mAccumLightTechnique.begin(FXL_RESTORE_DEFAULT_STATE);
            mAccumLightTechnique.beginPass(0);
            mAccumLightTechnique.commitU();
                  
            BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, mLightBufferedLights.getSize() * 4);
            
            mAccumLightTechnique.endPass();
            mAccumLightTechnique.end();
                        
            BD3D::mpDev->Resolve( 
               D3DRESOLVE_RENDERTARGET0 | ((sliceIndex < (cLightBufferDepth - 1)) ? D3DRESOLVE_CLEARRENDERTARGET : 0), 
               NULL, mpLightBuffers[0], NULL, 0, sliceIndex, (D3DVECTOR4*)&clearColor, 0.0f, 0, NULL);

            BD3D::mpDev->Resolve( 
               D3DRESOLVE_RENDERTARGET1 | ((sliceIndex < (cLightBufferDepth - 1)) ? D3DRESOLVE_CLEARRENDERTARGET : 0),  
               NULL, pIntermediateVectorTexture, NULL, 0, 0, (D3DVECTOR4*)&clearColor, 0.0f, 0, NULL);
            
            // Renormalize and pack the light vectors   
            BD3D::mpDev->SetRenderTarget(0, &lightBufferVectorSurface);               
            BD3D::mpDev->SetRenderTarget(1, NULL);
            
            BD3D::mpDev->SetVertexDeclaration(mpRenormVertexDecl);
            
            mRenormTechnique.begin(FXL_RESTORE_DEFAULT_STATE);
            mRenormTechnique.beginPass(0);
            mRenormTechnique.commitU();
                                    
            BD3D::mpDev->DrawVerticesUP(D3DPT_QUADLIST, 4, quadVertices, sizeof(quadVertices[0]));

            mRenormTechnique.endPass();
            mRenormTechnique.end();
            
            BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpLightBuffers[1], NULL, 0, sliceIndex, (D3DVECTOR4*)&clearColor, 0.0f, 0, NULL);
         }
         
         gRenderDraw.unsetTextures();          
         
         mLightBufferVectorSampler = NULL;
                           
         gGPUFrameHeap.releaseD3DResource(pIntermediateVectorTexture);
      }  
                  
      gRenderDraw.getWorkerActiveRenderViewport().setToDevice();
      gRenderDraw.clearStreamSource(0);
   }      
}

//============================================================================
// BVisibleLightManager::updateVisibleLightTexture
//============================================================================
void BVisibleLightManager::updateVisibleLightTexture(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   const uint lightTextureWidth = cMaxExpectedLocalLights * cTexelsPerLight;

   // rg [6/19/06] - This data can be read by the GPU as a texture, as a vertex buffer, or as vertex/pixel shader constants!

   D3DLOCKED_RECT lockedRect;
   mpLightTexture = gRenderDraw.lockDynamicTexture(lightTextureWidth, 1, D3DFMT_LIN_A32B32G32R32F, &lockedRect, NULL, false);

#if 0
   // 0 OmniPos, OmniMul
   // 1 Color, OmniAdd
   // 2 DecayDist, SpotMul, SpotAdd, ShadowIndex
   // 3 spotAt specContrib
   // 4 shadowCol0
   // 5 shadowCol1
   // 6 shadowCol2
   // 7 shadowDarkness
#endif

   XMFLOAT4* pTexels = reinterpret_cast<XMFLOAT4*>(lockedRect.pBits);

   mpVisibleLightTexels = pTexels;

   const XMVECTOR zero = XMVectorZero();

   mVisibleLightShadows.setAll(FALSE);

   mNumShadowedLights = 0;

   for (uint visibleLightIndex = 0; visibleLightIndex < mVisibleLights.size(); visibleLightIndex++)
   {
      XMFLOAT4* const pStartTexels = pTexels;
      pStartTexels;

      const uint activeLightIndex = mVisibleLights[visibleLightIndex];

      const BBaseLocalLightParams& baseParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(activeLightIndex);
      const BLocalLightParams& params = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);

      const uint lightFadeFract = mVisibleLightState[visibleLightIndex].mLightFadeFract;
      const uint shadowFadeFract = mVisibleLightState[visibleLightIndex].mShadowFadeFract;

      const bool shadows = (gRenderSceneLightManager.getActiveLightShadows(activeLightIndex) != FALSE) && (params.isValidShadowIndex());

      mNumShadowedLights += shadows;

      mVisibleLightShadows.set(visibleLightIndex, shadows);

      const float radius = baseParams.getRadius();
      const float farAttenStart = Math::Min(.999f, params.getFarAttenStart());

      const float omniOOFalloffRange = 1.0f / (1.0f - farAttenStart);

      const float omniMul = -((1.0f / radius) * omniOOFalloffRange);
      const float omniAdd = 1.0f + farAttenStart * omniOOFalloffRange;

      XMVECTOR pos = __vrlimi(baseParams.getPosRadius(), XMVectorReplicate(omniMul), VRLIMI_CONST(0, 0, 0, 1), 0);

      // pos, omniMul
      storeFloat4(pTexels, pos);
      pTexels++;

      // color, omniAdd
      XMVECTOR color = params.getColor();
      // rg - color should already be in linear light
      color = XMVectorMultiply(color, XMVectorReplicate(lightFadeFract * 1.0f/255.0f));
      XMStoreFloat3((XMFLOAT3*)pTexels, color);
      storeFloat((float*)pTexels+3, omniAdd);
      pTexels++;

      // 2 DecayDist, SpotMul, SpotAdd, ShadowIndex
      float spotMul = 0.0f;
      float spotAdd = 100.0f;

      if (params.getType() == cLTSpot)
      {
         static float fovFudge = 0.0f;
         const float spotInner = Math::Max(Math::fDegToRad(.0125f), baseParams.getSpotInner() - Math::fDegToRad(fovFudge));
         const float spotOuter = Math::Max(Math::fDegToRad(.025f), baseParams.getSpotOuter() - Math::fDegToRad(fovFudge));
         spotMul = 1.0f / ((cos(spotInner * .5f) - cos(spotOuter * .5f)));
         spotAdd = 1.0f - cos(spotInner * .5f) * spotMul;
      }         

      storeFloat((float*)pTexels, params.getDecayDist());
      storeFloat((float*)pTexels+1, spotMul);
      storeFloat((float*)pTexels+2, spotAdd);

      storeFloat((float*)pTexels+3, shadows ? params.getShadowIndex() : -1.0f);
      pTexels++;

      // 3 spotAt 

      // 3.0 because spec to diffuse ratio is 3:1
      const float specInten = params.getSpecular() ? 3.14f : 0.0f;
      if (params.getType() == cLTSpot)
         storeFloat4(pTexels, __vrlimi(baseParams.getAtSpotOuter(), XMVectorReplicate(specInten), VRLIMI_CONST(0,0,0,1), 0));
      else
         storeFloat4(pTexels, XMVectorReplicate(specInten));
      pTexels++;

      // 4 shadowCol0
      // 5 shadowCol1
      // 6 shadowCol2
      // 7 ShadowDarkness
      if (shadows)
      {         
         storeFloat4(pTexels, params.getShadowMatrixCol(0));
         storeFloat4(pTexels+1, params.getShadowMatrixCol(1));
         storeFloat4(pTexels+2, params.getShadowMatrixCol(2));
         //storeFloat4(pTexels+3, XMVectorSet(Math::Lerp(1.0f, params.getShadowDarkness(), shadowFadeFract * 1.0f/255.0f), 0.0f, 0.0f, 0.0f));
         storeFloat4(pTexels+3, XMVectorSet((float)params.getShadowUVBoundsIndex(), Math::Lerp(1.0f, params.getShadowDarkness(), shadowFadeFract * 1.0f/255.0f), 0.0f, 0.0f));
      }
      else
      {
         storeFloat4(pTexels, zero);
         storeFloat4(pTexels+1, zero);
         storeFloat4(pTexels+2, zero);         
         storeFloat4(pTexels+3, zero);         
      }
      pTexels += 4;

      BDEBUG_ASSERT((pTexels - pStartTexels) == cTexelsPerLight);
   }

   gRenderDraw.unlockDynamicTexture();

   gEffectIntrinsicManager.set(cIntrinsicLightTexture, &mpLightTexture, cIntrinsicTypeTexturePtr);   
}

//============================================================================
// BVisibleLightManager::drawDebugInfo
//============================================================================
void BVisibleLightManager::drawDebugInfo(void)
{
   ASSERT_THREAD(cThreadIndexRender);
}

//============================================================================
// BVisibleLightManager::update
//============================================================================
void BVisibleLightManager::update(float worldMinY, float worldMaxY)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BVisibleLightManagerUpdate);

   mWorldMinY = worldMinY;
   mWorldMaxY = worldMaxY;
   
   findVisibleLights(); 

   mLightObjectLinkedArrayManager.freeAllLists();
   Utils::FastMemSet(mShadowedLightObjectsArray.getPtr(), 0, mVisibleLights.getSize() * sizeof(mShadowedLightObjectsArray[0]));
}

//============================================================================
// BVisibleLightManager::addToShadowedLightObjectList
//============================================================================
bool BVisibleLightManager::addToShadowedLightObjectList(BVisibleLightIndex visibleLightIndex, int objectIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT((objectIndex >= SHRT_MIN) && (objectIndex <= SHRT_MAX));

   const uint activeLightIndex = mVisibleLights[visibleLightIndex];
   activeLightIndex;

   BDEBUG_ASSERT(gRenderSceneLightManager.getActiveLightShadows(activeLightIndex));

   uint linkedArrayListIndex = mShadowedLightObjectsArray[visibleLightIndex];

   BLightObjectLinkedArray* pList;
   if (!linkedArrayListIndex)
   {
      pList = mLightObjectLinkedArrayManager.createList();
      if (NULL == pList)
         return false;

      const uint index = mLightObjectLinkedArrayManager.getAllocatorItemIndex(pList) + 1;
      BDEBUG_ASSERT(index <= UCHAR_MAX);
      
      mShadowedLightObjectsArray[visibleLightIndex] = static_cast<uchar>(index);
   }
   else
   {
      pList = mLightObjectLinkedArrayManager.getBlockPtrFromAllocatorItemIndex(linkedArrayListIndex - 1);
   }

   BLightObjectLinkedArrayManager::BItemIter iter = mLightObjectLinkedArrayManager.insert(pList, (short)objectIndex);
   if (iter == BLightObjectLinkedArrayManager::cInvalidItemIter)
      return false;
   
   return true;
}

//============================================================================
// BVisibleLightManager::getShadowedLightObjectList
//============================================================================
BLightObjectLinkedArray* BVisibleLightManager::getShadowedLightObjectList(BVisibleLightIndex visibleLightIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   const uint activeLightIndex = mVisibleLights[visibleLightIndex];
   activeLightIndex;

   BDEBUG_ASSERT(gRenderSceneLightManager.getActiveLightShadows(activeLightIndex));

   uint linkedArrayListIndex = mShadowedLightObjectsArray[visibleLightIndex];
   if (!linkedArrayListIndex)
      return NULL;

   return mLightObjectLinkedArrayManager.getBlockPtrFromAllocatorItemIndex(linkedArrayListIndex - 1);
}

//============================================================================
// BVisibleLightManager::initDeviceData
//============================================================================
void BVisibleLightManager::initDeviceData(void)
{
   BDEBUG_ASSERT(!mpLightBufferEffect);
   mpLightBufferEffect = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
   const bool status = mpLightBufferEffect->init(gRender.getEffectCompilerDefaultDirID(), LIGHT_BUFFER_SHADER_FILENAME, true, false, true);
   BVERIFY(status);
         
   createLightBufferTextures();
   
   {         
      const D3DVERTEXELEMENT9 vertexElements[] =
      {
         { 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
         { 0, 16, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
         { 0, 32, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
         { 0, 48, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
         D3DDECL_END()
      };
      
      BD3D::mpDev->CreateVertexDeclaration(vertexElements, &mpAccumLightVertexDecl);
      BVERIFY(mpAccumLightVertexDecl);
   }
   
   {         
      const D3DVERTEXELEMENT9 vertexElements[] =
      {
         { 0, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         D3DDECL_END()
      };

      BD3D::mpDev->CreateVertexDeclaration(vertexElements, &mpRenormVertexDecl);
      BVERIFY(mpRenormVertexDecl);
   }
}

//============================================================================
// BVisibleLightManager::frameBegin
//============================================================================
void BVisibleLightManager::frameBegin(void)
{
}

//============================================================================
// BVisibleLightManager::processCommand
//============================================================================
void BVisibleLightManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
}

//============================================================================
// BVisibleLightManager::frameEnd
//============================================================================
void BVisibleLightManager::frameEnd(void)
{
}

//============================================================================
// BVisibleLightManager::deinitDeviceData
//============================================================================
void BVisibleLightManager::deinitDeviceData(void)
{
   ALIGNED_DELETE(mpLightBufferEffect, gRenderHeap);
   mpLightBufferEffect = NULL;
   
   freeLightBufferTextures();
         
   if (mpAccumLightVertexDecl)
   {
      mpAccumLightVertexDecl->Release();
      mpAccumLightVertexDecl = NULL;
   }
   
   if (mpRenormVertexDecl)
   {
      mpRenormVertexDecl->Release();
      mpRenormVertexDecl= NULL;
   }
}

//============================================================================
// BVisibleLightManager::createLightBufferTextures
//============================================================================
void BVisibleLightManager::createLightBufferTextures(void)
{
   BCOMPILETIMEASSERT(cNumLightBuffers == 2);
   if (!mpLightBuffers[0])
      BD3D::mpDev->CreateVolumeTexture(cLightBufferWidth, cLightBufferHeight, cLightBufferDepth, 1, 0, cLightBufferColorTexFormat, 0, &mpLightBuffers[0], NULL);

   if (!mpLightBuffers[1])
      BD3D::mpDev->CreateVolumeTexture(cLightBufferWidth, cLightBufferHeight, cLightBufferDepth, 1, 0, cLightBufferVectorTexFormat, 0, &mpLightBuffers[1], NULL);

   if ((!mpLightBuffers[0]) || (!mpLightBuffers[1]))
   {
      BFATAL_FAIL("Out of memory");
   }
}

//============================================================================
// BVisibleLightManager::freeLightBufferTextures
//============================================================================
void BVisibleLightManager::freeLightBufferTextures(void)
{
   for (uint i = 0; i < cNumLightBuffers; i++)
   {
      if (mpLightBuffers[i])
      {
         mpLightBuffers[i]->Release();
         mpLightBuffers[i] = NULL;
      }
   }
}

//============================================================================
// BVisibleLightManager::beginLevelLoad
//============================================================================
void BVisibleLightManager::beginLevelLoad(void)
{
   freeLightBufferTextures();
}

//============================================================================
// BVisibleLightManager::endLevelLoad
//============================================================================
void BVisibleLightManager::endLevelLoad(void)
{
   createLightBufferTextures();
}