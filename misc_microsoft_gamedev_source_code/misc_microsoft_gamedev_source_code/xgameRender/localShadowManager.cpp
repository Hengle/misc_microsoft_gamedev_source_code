//============================================================================
// File: localShadowManager.cpp
//
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "localShadowManager.h"

#include "primDraw2D.h"
#include "renderDraw.h"
#include "effectIntrinsicManager.h"
#include "render.h"
#include "inputsystem.h"
#include "math\VMXIntersection.h"

#include "..\shaders\shared\localLightingRegs.inc"

#pragma warning ( disable : 4238 ) // nonstandard extension used : class rvalue used as lvalue

//============================================================================
// Globals
//============================================================================
BLocalShadowManager gLocalShadowManager;

const uint cLocalShadowBufferPageDim = 512U;
const double cLocalShadowBufferInvPageDim  = 1.0f / cLocalShadowBufferPageDim;

//============================================================================
// BLocalShadowManager::BLocalShadowManager
//============================================================================
BLocalShadowManager::BLocalShadowManager() :
   mpRenderTarget(NULL),
   mpDepthStencil(NULL),
   mpShadowBufTexArray(NULL),
   mInitialized(false),
   mInShadowGenPass(false),
   mCurPass(0),
   mNumPasses(0)
{
   BCOMPILETIMEASSERT(MAX_LOCAL_SHADOW_BUFFER_SLICES == cMaxSlices);
   
   for (uint i = 0; i < cMaxPasses; i++)
      mPassParams[i].clear();
      
   Utils::ClearObj(mSliceUsedFlags);
      
   Utils::ClearObj(mVisibleLightIndices);
   
   for (uint i = 0; i < cMaxCachedLights; i++)
      mCachedLights[i].clear();

#ifndef BUILD_FINAL      
   mDebugDisplay = false;
   mDebugSlice = 0;
#endif
}

//============================================================================
// BLocalShadowManager::~BLocalShadowManager
//============================================================================
BLocalShadowManager::~BLocalShadowManager()
{
}

//============================================================================
// BLocalShadowManager::getSliceUsedPageFlags
//============================================================================
uint BLocalShadowManager::getSliceUsedPageFlags(uint slice) const
{
   BDEBUG_ASSERT(slice < cMaxSlices);
   return mSliceUsedFlags[slice];
}

//============================================================================
// BLocalShadowManager::setSliceUsedPageFlags
//============================================================================
void BLocalShadowManager::setSliceUsedPageFlags(uint slice, uint val, uint bitMask)
{
   BDEBUG_ASSERT(slice < cMaxSlices);
   if (val)
      mSliceUsedFlags[slice] |= bitMask;
   else
      mSliceUsedFlags[slice] &= ~bitMask;
}

//============================================================================
// BLocalShadowManager::findAvailableSlicePair
//============================================================================
bool BLocalShadowManager::findAvailableSlicePair(uint& slice)
{
   for (uint i = 0; i < cMaxSlices - 1; i++)
   {
      const uint curSliceUsedFlags = getSliceUsedPageFlags(i);
      const uint nextSliceUsedFlags = getSliceUsedPageFlags(i + 1);
      
      if ((curSliceUsedFlags == 0) && (nextSliceUsedFlags == 0))
      {
         slice = i;
         return true;
      }
   }
   
   return false;
} 

//============================================================================
// BLocalShadowManager::findAvailableSinglePage
//============================================================================
bool BLocalShadowManager::findAvailableSinglePage(uint& slice, uint& pageIndex, uint& pageBitMask)
{
   for (int i = cMaxSlices - 1; i >= 0; i--)
   {
      const uint sliceUsedFlags = getSliceUsedPageFlags(i);

      if (sliceUsedFlags != 15)
      {
         uint bitMask = 1;
         uint j;
         for (j = 0; j < 4; j++, bitMask <<= 1)
            if ((sliceUsedFlags & bitMask) == 0)
               break;
         
         slice = i;
         pageIndex = j;
         pageBitMask = bitMask;
            
         return true;
      }
   }

   return false;
} 


//============================================================================
// BLocalShadowManager::findAvailableCachedLightSlot
//============================================================================
int BLocalShadowManager::findAvailableCachedLightSlot(void)
{
   for (uint i = 0; i < cMaxCachedLights; i++)
      if (!mCachedLights[i].isUsed())
         return i;
   
   return -1;
}

//============================================================================
// BLocalShadowManager::init
//============================================================================
void BLocalShadowManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mInitialized)
      return;

   mInitialized = true;
   mInShadowGenPass = false;
   mCurPass = 0;
   mNumPasses = 0;
   
   for (uint i = 0; i < cMaxPasses; i++)
      mPassParams[i].clear();
      
   Utils::ClearObj(mSliceUsedFlags);      

   Utils::ClearObj(mVisibleLightIndices);
   
   for (uint i = 0; i < cMaxCachedLights; i++)
      mCachedLights[i].clear();
      
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);

   surfaceParams.Base = 0;

   HRESULT hres = gRenderDraw.createRenderTarget(cLocalShadowBufferPageDim, cLocalShadowBufferPageDim, D3DFMT_G16R16_EDRAM, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRenderTarget, &surfaceParams);
   if (FAILED(hres))
   {
      BFATAL_FAIL("CreateRenderTarget failed");
   }
   
   surfaceParams.Base = XGSurfaceSize(cLocalShadowBufferPageDim, cLocalShadowBufferPageDim, D3DFMT_G16R16_EDRAM, D3DMULTISAMPLE_NONE);

   hres = gRenderDraw.createDepthStencilSurface(cLocalShadowBufferPageDim, cLocalShadowBufferPageDim, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &mpDepthStencil, &surfaceParams);
   if (FAILED(hres))
   {
      BFATAL_FAIL("CreateDepthStencilSurface failed");
   }

   hres = gRenderDraw.createArrayTexture(cLocalShadowBufferPageDim, cLocalShadowBufferPageDim, cMaxSlices, 1, 0, D3DFMT_D24S8, 0, &mpShadowBufTexArray, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("CreateArrayTexture failed");
   }
   
   for (uint i = 0; i < cMaxSlices; i++)
   {
      D3DLOCKED_RECT lockedRect;
      mpShadowBufTexArray->LockRect(i, 0, &lockedRect, NULL, 0);
      
      const uint size = lockedRect.Pitch * cLocalShadowBufferPageDim;
      
      memset(lockedRect.pBits, 0, size);
      
      gRenderDraw.invalidateGPUCache(lockedRect.pBits, size);
      
      mpShadowBufTexArray->UnlockRect(i, 0);
   }
   
   gEffectIntrinsicManager.set(cIntrinsicLocalShadowMapTexture, &mpShadowBufTexArray, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BLocalShadowManager::deinit
//============================================================================
void BLocalShadowManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (!mInitialized)
      return;
     
   gRenderDraw.releaseD3DResource(mpRenderTarget); 
   mpRenderTarget = NULL;
   
   gRenderDraw.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;
   
   gRenderDraw.releaseD3DResource(mpShadowBufTexArray);      
   mpShadowBufTexArray = NULL;
   
   gEffectIntrinsicManager.set(cIntrinsicLocalShadowMapTexture, &mpShadowBufTexArray, cIntrinsicTypeTexturePtr);      
           
   mInitialized = false;
}

//============================================================================
// BLocalShadowManager::addPass
//============================================================================
void BLocalShadowManager::addPass(
   uint visibleLightIndex, uint activeLightIndex, 
   uint slice, uint numSlices, uint u, uint v, uint size, uint uvBoundsIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   //BLightObjectLinkedArray* pObjects = gRenderSceneLightManager.getShadowedLightObjectList(visibleLightIndex);
   
   const BBaseLocalLightParams& baseLightParams = gRenderSceneLightManager.getActiveLocalLightBaseParams(activeLightIndex);
   
   BLocalLightParams& lightParams = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);
   
   lightParams.setShadowUVBoundsIndex(uvBoundsIndex);
      
   if (lightParams.getType() == cLTOmni)
   {
      XMMATRIX worldToViewX;
      //worldToViewX.r[0] = baseLightParams.getRightSpotInner();
      //worldToViewX.r[2] = baseLightParams.getAtSpotOuter();
      worldToViewX.r[0] = XMVectorSet(1.0f,  0.0f, 0.0f, 0.0f);
      worldToViewX.r[2] = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
      worldToViewX.r[1] = XMVector3Cross(worldToViewX.r[2], worldToViewX.r[0]);
      worldToViewX.r[0].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[0].x + baseLightParams.getPosRadius().y*worldToViewX.r[0].y + baseLightParams.getPosRadius().z*worldToViewX.r[0].z);
      worldToViewX.r[1].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[1].x + baseLightParams.getPosRadius().y*worldToViewX.r[1].y + baseLightParams.getPosRadius().z*worldToViewX.r[1].z);
      worldToViewX.r[2].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[2].x + baseLightParams.getPosRadius().y*worldToViewX.r[2].y + baseLightParams.getPosRadius().z*worldToViewX.r[2].z);
      worldToViewX.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

      XMMATRIX worldToView(XMMatrixTranspose(worldToViewX));
   
      const float shadowIndexOffset = 1.0f / (2.0f * cMaxSlices);   
      lightParams.setShadowIndex( -((slice * 1.0f / cMaxSlices) + shadowIndexOffset) - 1.0f );
   
      float radius = baseLightParams.getRadius();

      Plane viewPlanes[6];
      viewPlanes[0].setFromEquation(-1.0f, 0.0f, 0.0f, radius);
      viewPlanes[1].setFromEquation( 1.0f, 0.0f, 0.0f, radius);
      viewPlanes[2].setFromEquation( 0.0f, -1.0f, 0.0f, radius);
      viewPlanes[3].setFromEquation( 0.0f,  1.0f, 0.0f, radius);
      
      for (uint passIndex = 0; passIndex < 2; passIndex++)
      {
         mVisibleLightIndices[mNumPasses] = (ushort)visibleLightIndex;
         
         BPassParams& passParams = mPassParams[mNumPasses];      
         passParams.mU = (WORD)u;
         passParams.mV = (WORD)v;
         passParams.mSize = (WORD)size;
         passParams.mSlice = (uchar)(slice + passIndex);
         passParams.mLightType = lightParams.getType();
         passParams.mLightID = lightParams.getLightID();
         passParams.mPassIndex = (uchar)passIndex;
         
         if (passIndex)
            passParams.mWorldToView = worldToView * XMMatrixScaling(1.0f, 1.0f, -1.0f);
         else
            passParams.mWorldToView = worldToView;         
         
         passParams.mViewToProj = XMMatrixIdentity();
                  
         if (!passIndex)
         {
            viewPlanes[4].setFromEquation( 0.0f,  0.0f, -1.0f, radius);
            viewPlanes[5].setFromEquation( 0.0f,  0.0f,  1.0f, 0.0f);
         }
         else
         {
            viewPlanes[4].setFromEquation( 0.0f,  0.0f, -1.0f, 0.0f);
            viewPlanes[5].setFromEquation( 0.0f,  0.0f,  1.0f, radius);
         }

         XMMATRIX viewToWorld;
         XMMatrixInverseByTranspose(viewToWorld, passParams.mWorldToView);
         XMVECTOR worldPlanes[6];
         for (uint i = 0; i < 6; i++)
         {
            Plane tempWorldPlane(Plane::transformOrthonormal(viewPlanes[i], (const BMatrix44&)viewToWorld));
            worldPlanes[i] = XMLoadFloat4((XMFLOAT4*)&tempWorldPlane.equation());
         }
         passParams.mVolumeCuller.setBasePlanes(worldPlanes);
         
         //XMMATRIX scale = XMMatrixScaling((size * .5f) / cLocalShadowBufferPageDim, -(size * .5f) / cLocalShadowBufferPageDim, 1.0f);
         //XMMATRIX tran = XMMatrixTranslation((size * .5f + u + .5f) / cLocalShadowBufferPageDim, (size * .5f + v + .5f) / cLocalShadowBufferPageDim, 0.0f);
         if (!passIndex)
         {
            XMMATRIX worldToTex(XMMatrixTranspose(passParams.mWorldToView));
            lightParams.setShadowMatrixCol(0, worldToTex.r[0]); 
            lightParams.setShadowMatrixCol(1, worldToTex.r[1]);
            lightParams.setShadowMatrixCol(2, worldToTex.r[2]);
         }
         
         mNumPasses++;
      }
   }
   else
   {
      XMMATRIX worldToViewX;
      worldToViewX.r[0] = baseLightParams.getRightSpotInner();
      worldToViewX.r[2] = baseLightParams.getAtSpotOuter();
      worldToViewX.r[1] = XMVector3Cross(worldToViewX.r[2], worldToViewX.r[0]);
      worldToViewX.r[0].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[0].x + baseLightParams.getPosRadius().y*worldToViewX.r[0].y + baseLightParams.getPosRadius().z*worldToViewX.r[0].z);
      worldToViewX.r[1].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[1].x + baseLightParams.getPosRadius().y*worldToViewX.r[1].y + baseLightParams.getPosRadius().z*worldToViewX.r[1].z);
      worldToViewX.r[2].w = -(baseLightParams.getPosRadius().x*worldToViewX.r[2].x + baseLightParams.getPosRadius().y*worldToViewX.r[2].y + baseLightParams.getPosRadius().z*worldToViewX.r[2].z);
      worldToViewX.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

      XMMATRIX worldToView(XMMatrixTranspose(worldToViewX));
   
      mVisibleLightIndices[mNumPasses] = (ushort)visibleLightIndex;
      
      const float shadowIndexOffset = 1.0f / (2.0f * cMaxSlices);   
      lightParams.setShadowIndex((slice * 1.0f / cMaxSlices) + shadowIndexOffset);
      
      BPassParams& passParams = mPassParams[mNumPasses];      
      passParams.mU = (WORD)u;
      passParams.mV = (WORD)v;
      passParams.mSize = (WORD)size;
      passParams.mSlice = (uchar)slice;
      passParams.mLightType = lightParams.getType();
      passParams.mLightID = lightParams.getLightID();
      passParams.mWorldToView = worldToView;
               
      BDEBUG_ASSERT((baseLightParams.getSpotOuter()) > 0.0f && (baseLightParams.getSpotInner() > 0.0f));
      
      const float nearZ = 1.0f;
      const float farZ = 128.0f;
      passParams.mViewToProj = XMMatrixPerspectiveFovLH(baseLightParams.getSpotOuter(), 1.0f, nearZ, farZ);

      XMMATRIX worldToProj = XMMatrixMultiply(passParams.mWorldToView, passParams.mViewToProj);

      {
         const float nearZ = .5f;
         const float farZ = baseLightParams.getRadius() + .5f;
         XMMATRIX viewToProj = XMMatrixPerspectiveFovLH(baseLightParams.getSpotOuter(), 1.0f, nearZ, farZ);

         XMMATRIX worldToProj = XMMatrixMultiply(passParams.mWorldToView, viewToProj);

         BFrustum cullingFrustum;
         cullingFrustum.set(*reinterpret_cast<const BMatrix44*>(&worldToProj));
      
         XMVECTOR radVec = XMVectorReplicate(baseLightParams.getRadius());
         XMVECTOR omniWorldMin = baseLightParams.getPosRadius() - radVec;
         XMVECTOR omniWorldMax = baseLightParams.getPosRadius() + radVec;
         
         XMVECTOR cullingFrustumVerts[8];
         BVMXIntersection::computeFrustumVertices(cullingFrustumVerts, worldToProj);
         XMVECTOR spotWorldMin;
         XMVECTOR spotWorldMax;
         BVMXIntersection::computeBoundsOfPoints(cullingFrustumVerts, 8, &spotWorldMin, &spotWorldMax);
         
         spotWorldMin = XMVectorMax(spotWorldMin, omniWorldMin);
         spotWorldMax = XMVectorMin(spotWorldMax, omniWorldMax);
                     
         passParams.mVolumeCuller.setBasePlanes(cullingFrustum, false);
         passParams.mVolumeCuller.setBaseBounds(spotWorldMin, spotWorldMax);
      }

      XMMATRIX scale = XMMatrixScaling((size * .5f) / cLocalShadowBufferPageDim, -(size * .5f) / cLocalShadowBufferPageDim, 1.0f);
      XMMATRIX tran = XMMatrixTranslation((size * .5f + u + .5f) / cLocalShadowBufferPageDim, (size * .5f + v + .5f) / cLocalShadowBufferPageDim, 0.0f);
      XMMATRIX worldToTex = worldToProj * scale * tran;
      worldToTex = XMMatrixTranspose(worldToTex);
      lightParams.setShadowMatrixCol(0, worldToTex.r[0]); 
      lightParams.setShadowMatrixCol(1, worldToTex.r[1]);
      lightParams.setShadowMatrixCol(2, worldToTex.r[3]);
      
      mNumPasses++;
   }      
}

//============================================================================
// struct BLightToUpdate
//============================================================================
struct BLightToUpdate
{
   uint mVisibleLightIndex;
   uint mCachedLightIndex;
   
   BLightToUpdate() { }
   BLightToUpdate(uint visibleLightIndex, uint cachedLightIndex) : mVisibleLightIndex(visibleLightIndex), mCachedLightIndex(cachedLightIndex) { }
};

//============================================================================
// BLocalShadowManager::shadowGenPrep
//============================================================================
void BLocalShadowManager::shadowGenPrep(const BVec3& worldMin, const BVec3& worldMax, float focusHeight)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(mInitialized);
      
   mNumPasses = 0;
   mCurPass = 0;
   
   BStaticArray<WORD, 256> newLights;
   BStaticArray<BLightToUpdate, 256> lightsToUpdate;
   BStaticArray<uchar, (cMaxCachedLights + 7) / 8, false> cachedLightFoundFlags((cMaxCachedLights + 7) / 8);
      
   const uint curFrame = gRenderThread.getCurMainFrameNoBarrier();
         
   for (uint visibleLightIndex = 0; visibleLightIndex < gVisibleLightManager.getNumVisibleLights(); visibleLightIndex++)
   {
      const uint activeLightIndex = gVisibleLightManager.getVisibleLight(visibleLightIndex);
      
      if (!gRenderSceneLightManager.getActiveLightShadows(activeLightIndex))
         continue;
        
      BLocalLightParams& lightParams = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);
      
      if ( ((lightParams.getType() != cLTSpot) && (lightParams.getType() != cLTOmni)) ||
           (gVisibleLightManager.getVisibleLightInfo(visibleLightIndex).mShadowFadeFract < 1) )
      {
         lightParams.setInvalidShadowIndex();
         continue;
      }
      
      uint cachedLightIndex;
      for (cachedLightIndex = 0; cachedLightIndex < cMaxCachedLights; cachedLightIndex++)
      {
         if (!mCachedLights[cachedLightIndex].isUsed()) 
            continue;
         
         if (mCachedLights[cachedLightIndex].mLightID == lightParams.getLightID())
            break;
      }
      
      if (cachedLightIndex < cMaxCachedLights)
      {
         lightsToUpdate.pushBack(BLightToUpdate(visibleLightIndex, cachedLightIndex));
      
         cachedLightFoundFlags[cachedLightIndex>>3] |= (1 << (cachedLightIndex & 7));
      }
      else
      {
         newLights.pushBack((WORD)visibleLightIndex);
      }
   }
   
   for (uint i = 0; i < cMaxCachedLights; i++)
   {
      if (!mCachedLights[i].isUsed())
         continue;
      
      if (0 == (cachedLightFoundFlags[i >> 3] & (1 << (i & 7))))
      {
         if (mCachedLights[i].mSlice >= 0)
         {
            for (uint j = 0; j < mCachedLights[i].mNumSlices; j++)
               setSliceUsedPageFlags(mCachedLights[i].mSlice + j, false, mCachedLights[i].mPageFlags);
         }
         mCachedLights[i].clear();
      }
   }
   
   for (uint i = 0; i < newLights.size(); i++)
   {
      const uint visibleLightIndex = newLights[i];
      const uint activeLightIndex = gVisibleLightManager.getVisibleLight(visibleLightIndex);
      
      BLocalLightParams& lightParams = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);
      
      lightParams.setInvalidShadowIndex();
      
      int cachedLightIndex = findAvailableCachedLightSlot();
      if (cachedLightIndex == -1)
         continue;
                  
      BCachedLight& cachedLight = mCachedLights[cachedLightIndex];
      cachedLight.mLightID = lightParams.getLightID();
      
      lightsToUpdate.pushBack(BLightToUpdate(visibleLightIndex, cachedLightIndex));
   }

   static bool partialUpdating = false;
   uint updateStart = 0;
   uint updateEnd = lightsToUpdate.size();
   if (partialUpdating)
   {
      updateStart = 0;
      updateEnd = lightsToUpdate.size() / 2;
   
      if (curFrame & 1)
      {
         updateStart = updateEnd;
         updateEnd = lightsToUpdate.size();
      }
   }      
         
   for (uint i = updateStart; i < updateEnd; i++)
   {
      const uint visibleLightIndex = lightsToUpdate[i].mVisibleLightIndex;
      const uint cachedLightIndex = lightsToUpdate[i].mCachedLightIndex;
      
      const uint activeLightIndex = gVisibleLightManager.getVisibleLight(visibleLightIndex);
      
      BLocalLightParams& lightParams = gRenderSceneLightManager.getActiveLocalLightParams(activeLightIndex);
                  
      BCachedLight& cachedLight = mCachedLights[cachedLightIndex];
      
      BDEBUG_ASSERT(cachedLight.mLightID == lightParams.getLightID());
      
      if (cachedLight.mSlice < 0)
      {
         const bool omniLight = (lightParams.getType() == cLTOmni);
         
         uint u, v, size, slice, numSlices, pageIndex, pageBitMask, uvBoundsIndex;
         
         bool foundPage;
         if (omniLight)
         {
            size = cLocalShadowBufferPageDim;
            pageIndex = 0;
            pageBitMask = 15;
            numSlices = 2;
            foundPage = findAvailableSlicePair(slice);
            uvBoundsIndex = 0;
            u = 0;
            v = 0;
         }
         else
         {
            size = cLocalShadowBufferPageDim / 2;
            numSlices = 1;
            foundPage = findAvailableSinglePage(slice, pageIndex, pageBitMask);
            uvBoundsIndex = pageIndex + 1;
            u = (pageIndex & 1) ? (cLocalShadowBufferPageDim / 2) : 0;
            v = (pageIndex & 2) ? (cLocalShadowBufferPageDim / 2) : 0;
         }
            
         if (!foundPage)
            continue;
         
         for (uint j = 0; j < numSlices; j++)   
            setSliceUsedPageFlags(slice + j, true, pageBitMask);
         
         cachedLight.mU = (WORD)u;
         cachedLight.mV = (WORD)v;
         cachedLight.mSize = (WORD)size;
         
         cachedLight.mSlice = (char)slice;
         cachedLight.mNumSlices = (uchar)numSlices;
         cachedLight.mPageFlags = (uchar)pageBitMask; 
         cachedLight.mUVBoundsIndex = (uchar)uvBoundsIndex;
      }
      
      if ((mCurPass + cachedLight.mNumSlices) <= cMaxPasses)
      {
         cachedLight.mLastFrameUpdated = curFrame;
         addPass(visibleLightIndex, activeLightIndex, cachedLight.mSlice, cachedLight.mNumSlices, cachedLight.mU, cachedLight.mV, cachedLight.mSize, cachedLight.mUVBoundsIndex);
      }
   }
}

//============================================================================
// BLocalShadowManager::shadowGenInit
//============================================================================
void BLocalShadowManager::shadowGenInit(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mInitialized && !mInShadowGenPass);
   mInShadowGenPass = true;
   
   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
      
   static float depthBias = .0001f;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));
   
   static float slopeScaleBias = 1.5f;
   BD3D::mpDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS , CAST(DWORD, slopeScaleBias));
}

//============================================================================
// BLocalShadowManager::shadowGenBegin
//============================================================================
void BLocalShadowManager::shadowGenBegin(uint pass, bool& dualParaboloid)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mInitialized && pass < mNumPasses);
      
   BMatrixTracker&  matrixTracker   = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();
      
   const BPassParams& passParams = mPassParams[pass];

   matrixTracker.setMatrix(cMTWorldToView, passParams.mWorldToView);
   matrixTracker.setMatrix(cMTViewToProj, passParams.mViewToProj);

   D3DVIEWPORT9 viewport;
   viewport.X = passParams.mU;
   viewport.Y = passParams.mV;
   viewport.Width = passParams.mSize;
   viewport.Height = passParams.mSize;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;

   matrixTracker.setViewport(viewport);

   renderViewport.setSurf(0, NULL);
   renderViewport.setDepthStencilSurf(mpDepthStencil);
   renderViewport.setViewport(viewport);

   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);
   
   D3DRECT rect;
   rect.x1 = viewport.X;
   rect.y1 = viewport.Y;
   rect.x2 = rect.x1 + viewport.Width;
   rect.y2 = rect.y1 + viewport.Height;
   gRenderDraw.clear(1, &rect, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
   
   dualParaboloid = (cLTOmni == passParams.mLightType);
}

//============================================================================
// BLocalShadowManager::shadowGenEnd
//============================================================================
void BLocalShadowManager::shadowGenEnd(uint pass)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mInitialized && pass < mNumPasses);

   const BPassParams& passParams = mPassParams[pass];
   
   D3DRECT srcRect;
   srcRect.x1 = passParams.mU;
   srcRect.y1 = passParams.mV;
   srcRect.x2 = passParams.mU + passParams.mSize;
   srcRect.y2 = passParams.mV + passParams.mSize;
   
   D3DPOINT dstPoint;
   dstPoint.x = passParams.mU;
   dstPoint.y = passParams.mV;
   
   BD3D::mpDev->Resolve(
      D3DRESOLVE_DEPTHSTENCIL|D3DRESOLVE_ALLFRAGMENTS,
      &srcRect,
      mpShadowBufTexArray,
      &dstPoint,
      0,
      passParams.mSlice,
      NULL,
      1.0f,
      0,
      NULL);
}

//============================================================================
// BLocalShadowManager::shadowGenDeinit
//============================================================================
void BLocalShadowManager::shadowGenDeinit(void)
{
   BDEBUG_ASSERT(mInitialized && mInShadowGenPass);
   mInShadowGenPass = false;
   
   gRenderDraw.resetWorkerActiveMatricesAndViewport();

   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
}

#ifndef BUILD_FINAL   
//============================================================================
// BLocalShadowManager::debugDraw
//============================================================================
void BLocalShadowManager::debugDraw(ATG::Font& font)
{
   ASSERT_RENDER_THREAD
   if (!mDebugDisplay)
      return;
                  
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);      
   
   WCHAR textBuf[256];
   StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"Slice %i", mDebugSlice);
   font.DrawText(50.0f, 20.0f, 0xFFFFFFFF, textBuf);   

   BD3D::mpDev->SetTexture(0, mpShadowBufTexArray);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, TRUE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTERZ, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTERZ, D3DTEXF_POINT);

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);

   static uint firstPass = 0;
   static uint lastPass = 0;
   static uint rectWidth = 512;
   static uint rectHeight = 512;
   uint slice = mDebugSlice;
   
   BVec4 mul(-1.0f);
   BVec4 add(1.0f);
   BD3D::mpDev->SetPixelShaderConstantF(0, mul.getPtr(), 1);
   BD3D::mpDev->SetPixelShaderConstantF(1, add.getPtr(), 1);

   BVec4 sliceVec(1.0f/(2.0f*cMaxSlices) + (float)slice / cMaxSlices);
   BD3D::mpDev->SetPixelShaderConstantF(2, sliceVec.getPtr(), 1);

   uint xOfs = 50;
   uint yOfs = 50;
   BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+rectWidth, yOfs+rectHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cDepthVisPS);

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
}
#endif      

#ifndef BUILD_FINAL
//============================================================================
// BLocalShadowManager::debugHandleInput
//============================================================================
void BLocalShadowManager::debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed)
{
   if (event == cInputEventControlStart)
   {
      switch (controlType)
      {
         case cKeyS:
         {
            if (altPressed)
               mDebugSlice++;
            else if (shiftPressed)
               mDebugSlice--;
            else if (controlPressed)
            {
               mDebugDisplay = !mDebugDisplay;
               gConsoleOutput.status("Local shadow manager debug display %s", mDebugDisplay ? "Enabled" : "Disabled");
            }

            if (mDebugSlice < 0) 
               mDebugSlice = cMaxSlices - 1;
            else if (mDebugSlice >= cMaxSlices)
               mDebugSlice = 0;

            break;
         }
      }
   }
}
#endif      
