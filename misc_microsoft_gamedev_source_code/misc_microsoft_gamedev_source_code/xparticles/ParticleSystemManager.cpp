//============================================================================
//
//  ParticleSystemManager.cpp
//
//  Copyright (c) 2000-2006 Ensemble Studios
//
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xparticlescommon.h"

#include "mathutil.h"
#include "radixsorter.h"
#include "xrender.h"
#include "xmlreader.h"
#include "config.h"
#include "econfigenum.h"
#include "configsparticles.h"

#include "workdirsetup.h"
#include "boundingBox.h"
#include "BoundingSphere.h"
#include "vertexTypes.h"
#include "image.h"
#include "reloadManager.h"
#include "timer.h"

#include "render.h"
#include "tiledAA.h"

#include "ParticleSystemManager.h"
#include "particlerenderer.h"
#include "particletexturemanager.h"
#include "particleeffectdata.h"
#include "particlevertextypes.h"
#include "rangeCheck.h"

#include "dynamicGPUBuffer.h"
#include "math\VMXIntersection.h"

#include "threading\workDistributor.h"

#include "memory\allocationLogger.h"

#define ENABLE_MULTITHREADING
//#define ENABLE_DEBUG_PRIMITIVES
//#define ENABLE_BBOX_RENDERING
//#define ENABLE_DEBUG_DISTANCEFADERAGE_RENDERING

#define USE_PRIORITY_SYSTEM

#include "debugprimitives.h"

// rg - FIXME, once we move the fill depth stencil surface to the tiled AA manager
#include "tonemapManager.h"
#include "gpuHeap.h"

//#define DEBUG_TRACE_RECORD
#ifdef  DEBUG_TRACE_RECORD
#include "tracerecording.h"
#include "xbdm.h"
#pragma comment( lib, "tracerecording.lib" )
#endif

const uint cMaxPermittedEmitters = 8192;
const uint cMaxParticleVerts = 45000;
const uint cMaxAllocatedFrameStorage = cMaxParticleVerts * Math::Max(Math::Max(sizeof(BPTrailVertexPT), sizeof(BPVertexPT)), sizeof(BPBeamVertexPT));
const uint cDynamicGPUBufferSize = (uint)(cMaxAllocatedFrameStorage * 2.0f);
const uint cMaxParticleBlockHeapSize = 6U * 1024U * 1024U;

BParticleSystemManager gPSManager;

__declspec(thread) Random* BParticleSystemManager::mpRandom;
BCountDownEvent BParticleSystemManager::mRemainingWorkBuckets;
bool BParticleSystemManager::mPermitEmission;

const float cEmitterFadeStartDistance  = 90.0f;
const float cEmitterFadeEndDistance    = 190.0f;
const float cEmitterFadeRange          = cEmitterFadeEndDistance - cEmitterFadeStartDistance;
const float cEmitterOneOverFadeRange   = 1.0f / cEmitterFadeRange;
const XMVECTOR cEmitterFadeStartDistanceV = XMVectorSplatX(XMLoadScalar(&cEmitterFadeStartDistance));
const XMVECTOR cEmitterFadeEndDistanceV = XMVectorSplatX(XMLoadScalar(&cEmitterFadeEndDistance));
const XMVECTOR cEmitterFadeRangeV = XMVectorSplatX(XMLoadScalar(&cEmitterFadeRange));
const XMVECTOR cEmitterOneOverFadeRangeV = XMVectorSplatX(XMLoadScalar(&cEmitterOneOverFadeRange));

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
static const float MIN_UPDATE_RADIUS = 2.0f;

float BParticleSystemManager::mRandomFloats[PS_NUM_RAND_FLOATS];

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BParticleSystemManager::BParticleSystemManager():      
   mInstanceBlockSize(0),
   mParticleBlockSize(0),
   mNumInstancesRendered(0),
   mNumParticlesRendered(0),
   mNumParticlesAllocated(0),
   mUsedInstanceList(&gParticleHeap),
   mFreeInstanceList(&gParticleHeap),
   mInstanceBlocks(&gParticleHeap),
   mpDistortionTexture(NULL),
   mDistortionBufferWidth(0),
   mDistortionBufferHeight(0),
   mpDynamicGPUBuffer(NULL),
   mpColorSurface(NULL),
   mpDepthSurface(NULL),
   mpColorSurfaceAliasedAA(NULL),
   mpDepthSurfaceAliasedAA(NULL),
   mLightBuffering(true),
   mLightBufferIntensityScale(1.0f),
   mTimeSpeed(1.0f),
   mUpdateSorter(&gParticleHeap),
   mRenderSorter(&gParticleHeap),
   mLocalTotalNumAllowedInstancesThisUpdate(0),
   mSunColor(XMVectorSplatOne()),
   mbFlagMemoryPoolsInitalized(false)
{
   setFlagPauseUpdate(false);
   setFlagDistanceFade(true);
   setFlagUseAliasedFillSurface(true);
   setFlagEnableCulling(true);

   for (int i = 0; i < cThreadIndexMax; i++)
      mThreadEventHandle[i]=cInvalidEventReceiverHandle;

#ifdef ALLOCATION_LOGGER   
   #ifdef USE_SEPERATE_PARTICLE_HEAP
      getAllocationLogger().registerHeap("ParticleHeap", &gParticleHeap, false);
   #endif      
   getAllocationLogger().registerHeap("ParticleBlockHeap", &gParticleBlockHeap, false);   
#endif   

#ifndef BUILD_FINAL
   mLastReloadPath.empty();
   mLastReloadTick = 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleSystemManager::~BParticleSystemManager()
{   
}

//============================================================================
//  workerInit
//============================================================================
void BParticleSystemManager::workerInit(void* pData)
{
   BDEBUG_ASSERT(!mpDynamicGPUBuffer);
   mpDynamicGPUBuffer = ALIGNED_NEW(BDynamicGPUBuffer, gParticleHeap);
   mpDynamicGPUBuffer->init(cDynamicGPUBufferSize, 64);
}

//============================================================================
//  workerDeinit
//============================================================================
void BParticleSystemManager::workerDeinit(void* pData)
{
   if (mpDynamicGPUBuffer)
   {
      ALIGNED_DELETE(mpDynamicGPUBuffer, gParticleHeap);
      mpDynamicGPUBuffer = NULL;
   }
}

//============================================================================
//  INTERFACE
//============================================================================
void BParticleSystemManager::init(long dataBlockSize, long instanceBlockSize, long particleBlockSize)
{
   ASSERT_MAIN_THREAD
      
   mRemainingWorkBuckets.clear();

   //-- Validate Parameters.
   if (dataBlockSize < 1)
      return;
   if (instanceBlockSize < 1)
      return;
   if (particleBlockSize < 1)
      return;
   
   //-- Init the members.
   mInstanceBlockSize        = instanceBlockSize;
   mParticleBlockSize        = particleBlockSize;
   mData.reserve(dataBlockSize);   
   mData.clear();
   mDataHashmap.clear();

   initMemoryPools();
   
   mPSysManTime              = 0;
   mPSysManTimeF             = 0.0f;
   
   mLightBuffering = true;
   mLightBufferIntensityScale = 1.0f;
   mSunColor = XMVectorSplatOne();

   for (int threadID = 0; threadID < cThreadIndexMax; ++threadID)
   {
      if (mThreadEventHandle[threadID] == cInvalidEventReceiverHandle)
         mThreadEventHandle[threadID] = gEventDispatcher.addClient(this, threadID);
   }

   for (int curThread = 0; curThread < cThreadIndexMax; ++curThread)
   {
      gEventDispatcher.send(cInvalidEventReceiverHandle,mThreadEventHandle[curThread], cParticleSystemEventClassCreateThreadLocalRandom, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
   }

   BASSERT(mpRandom);
   for(int i=0; i<PS_NUM_RAND_FLOATS; i++)
      mRandomFloats[i] = mpRandom->fRand(-1.0f, 1.0f);
   
   gParticleRenderer.init();
   gPSTextureManager.init();
      
   //-- register with the reload manager
   BReloadManager::BPathArray Paths;
   Paths.push_back("*.pfx");
   gReloadManager.registerClient(Paths, BReloadManager::cFlagSynchronous|BReloadManager::cFlagSubDirs, mThreadEventHandle[cThreadIndexRender]);

   mRenderInstances.resize(gTiledAAManager.getNumTiles());   
   mRenderNearLayerInstances.resize(gTiledAAManager.getNumTiles());

   initDistortionBuffers(gTiledAAManager.getTilingWidth(), gTiledAAManager.getTilingHeight());

   // distance fade
   mEmitterFadeStartDistance = cEmitterFadeStartDistance;
   mEmitterFadeEndDistance   = cEmitterFadeEndDistance;   
   mEmitterOneOverFadeRange  = cEmitterOneOverFadeRange;

   mEmitterFadeStartDistanceV = cEmitterFadeStartDistanceV;
   mEmitterFadeEndDistanceV   = cEmitterFadeEndDistanceV;   
   mEmitterOneOverFadeRangeV  = cEmitterOneOverFadeRangeV;

   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BParticleSystemManager::workerInit));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::initDistortionBuffers(uint width, uint height)
{   
   // rg [1/31/08] - FIXME : Change the distortion buffer to use the GPU frame heap
   
   const uint halfWidth = Math::Max(1U, width / 2U);
   const uint halfHeight = Math::Max(1U, height / 2U);
   
   mDistortionBufferWidth = halfWidth;
   mDistortionBufferHeight = halfHeight;
   
   mDistortionRenderTarget.set(halfWidth, halfHeight, D3DFMT_G16R16_EDRAM, D3DFMT_D24S8, D3DMULTISAMPLE_NONE);
   if(!mDistortionRenderTarget.createDeviceObjects(0))
      return false;

   return  true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::kill()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.blockUntilGPUIdle();
      
   deinitMemoryPools();
   
   mInstanceBlockSize        = 0;
   mParticleBlockSize        = 0;
   mNumInstancesRendered     = 0;
   mNumParticlesRendered     = 0;
   mNumParticlesAllocated    = 0;
   mData.clear();
   mDataHashmap.clear();
      
   killAllData();

   setFlagPauseUpdate(false);
   setFlagDistanceFade(true);
   setFlagUseAliasedFillSurface(true);
   setFlagRenderMagnets(false);   

   gParticleRenderer.deInit();
   gPSTextureManager.deInit();

   for (int curThread = 0; curThread < cThreadIndexMax; ++curThread)
   {
      gEventDispatcher.send(cInvalidEventReceiverHandle,mThreadEventHandle[curThread], cParticleSystemEventClassDeleteThreadLocalRandom, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
   }

   for (int threadID = 0; threadID < cThreadIndexMax; ++threadID)
   {
      if (mThreadEventHandle[threadID])
         gEventDispatcher.removeClientDeferred(mThreadEventHandle[threadID], true);
   }         

   //-- Distortion
   mDistortionRenderTarget.destroyDeviceObjects();
   
   mUpdateSorter.freeIndices();
   mRenderSorter.freeIndices();
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BParticleSystemManager::workerDeinit));
   
#ifdef USE_SEPERATE_PARTICLE_HEAP
   gParticleHeap.verify();
#endif   
   gParticleBlockHeap.verify();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::killAllInstances()
{
   ASSERT_RENDER_THREAD

   BHandle hInstance;
   BParticleEmitter* pInstance = mUsedInstanceList.getHead(hInstance);
   //-- To be safe kill any PFX parent particles first
   while (pInstance && pInstance->getData() && (pInstance->getData()->mProperties.mType == BEmitterBaseData::ePFX))
   {
      pInstance->kill();

#ifdef BUILD_DEBUG
      BHandle hFreeListNode = NULL;
      bool bInFreeList = findEmitterInFreelist(pInstance, hFreeListNode);
      BASSERT(!bInFreeList);
      if (!bInFreeList)
#endif
      {
         mFreeInstanceList.addToTail(pInstance);
      }         

      pInstance = mUsedInstanceList.removeAndGetNext(hInstance);      
   }

   //-- kill any remaining particles
   pInstance = mUsedInstanceList.getHead(hInstance);
   while (pInstance)
   {
      pInstance->kill();
   
#ifdef BUILD_DEBUG
      BHandle hFreeListNode = NULL;
      bool bInFreeList = findEmitterInFreelist(pInstance, hFreeListNode);
      BASSERT(!bInFreeList);
      if (!bInFreeList)
#endif
      {
         mFreeInstanceList.addToTail(pInstance);
      }         
      
      pInstance = mUsedInstanceList.removeAndGetNext(hInstance);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::killAllData()
{
   //-- kill the data and hash map
   for (int i = 0; i < mData.getSize(); ++i)
   {
      BParticleEffectDefinition* pData = mData[i];
      if (pData)
      {
         pData->deInit();
         ALIGNED_DELETE(pData, gParticleHeap);
         mData[i] = NULL;
      }
   }
   mData.clear();
   mDataHashmap.clear();
   mDataHashmap.setMaxEntries(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::initMemoryPools(void)
{
   if (mbFlagMemoryPoolsInitalized)
      return;

   mUsedInstanceList.reset(mInstanceBlockSize, mInstanceBlockSize);
   mFreeInstanceList.reset(mInstanceBlockSize, mInstanceBlockSize);

   BParticleEmitter* pInstance = mFreeInstanceList.removeHead();
   BASSERT(pInstance == NULL);
   if (pInstance == NULL)
   {
      //-- Add a new block.
      BParticleEmitter* pBlock = ALIGNED_NEW_ARRAY(BParticleEmitter, mInstanceBlockSize, gParticleHeap);
      mInstanceBlocks.addToTail(pBlock);
      for (long index = 0; index < mInstanceBlockSize; ++index)
         mFreeInstanceList.addToTail(&pBlock[index]);      
   }   

   mbFlagMemoryPoolsInitalized = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::deinitMemoryPools(void)
{
   //-- Delete all the instance blocks.
   BParticleEmitter* pInstanceBlock = mInstanceBlocks.removeHead();
   while (pInstanceBlock)
   {
      ALIGNED_DELETE_ARRAY(pInstanceBlock, gParticleHeap);
      pInstanceBlock = mInstanceBlocks.removeHead();
   }

   mUsedInstanceList.reset();
   mFreeInstanceList.reset();
   mInstanceBlocks.reset();   
         
   mEffectList.clear();         

   // clear all local lists
   for (int x = 0; x < mRenderInstances.getSize(); ++x)
      mRenderInstances[x].clear();
   mRenderInstances.clear();

   mRenderDistortionInstances.clear();
   for (int y = 0; y < mRenderNearLayerInstances.getSize(); ++y)
      mRenderNearLayerInstances[y].clear();
   mRenderNearLayerInstances.clear();

   mRenderNoTilingInstances.clear();

   mLocalTotalNumAllowedInstancesThisUpdate = 0;
   mLocalInstances.clear();
   mLocalOrder.clear();
   mLocalList.clear();

   mPFXInstances.clear();
   mZLocalInstances.clear();
   mZLocalOrder.clear();
   mZLocalList.clear();

   mbFlagMemoryPoolsInitalized = false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::initTextureSystem()
{
   bool bSuccess = gPSTextureManager.initTextureManagement();
   bSuccess;
   BASSERT(bSuccess);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::deinitTextureSystem()
{
   gPSTextureManager.deinitTextureManagement();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BParticleSystemManager::getNumDatasLoaded()
{
   ASSERT_RENDER_THREAD
   
   //return mDataList1.getSize();
   return mData.getSize();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BParticleSystemManager::getNumInstancesAllocated()
{
   ASSERT_RENDER_THREAD
   
   return (mInstanceBlockSize * mInstanceBlocks.getSize());   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BParticleSystemManager::getNumInstancesInUse()
{
   ASSERT_RENDER_THREAD
   
   return (getNumInstancesAllocated() - mFreeInstanceList.getSize());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::enableBBoxRendering(bool enable)
{
   gParticleRenderer.setFlag(BParticleRenderer::eFlagRenderBBoxes, enable);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::setDistanceFade(float startDistance, float endDistance)
{
   mEmitterFadeStartDistance = startDistance;   
   mEmitterFadeEndDistance = endDistance;   
   mEmitterFadeRange =  (mEmitterFadeEndDistance - mEmitterFadeStartDistance);
   mEmitterOneOverFadeRange = 1.0f / mEmitterFadeRange;

   mEmitterFadeStartDistanceV = XMVectorSplatX(XMLoadScalar(&mEmitterFadeStartDistance));;
   mEmitterFadeEndDistanceV   = XMVectorSplatX(XMLoadScalar(&mEmitterFadeEndDistance));   
   mEmitterOneOverFadeRangeV  = XMVectorSplatX(XMLoadScalar(&mEmitterOneOverFadeRange));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BParticleSystemManager::getNumParticlesAllocated()
{
   ASSERT_RENDER_THREAD
   
   return mNumParticlesAllocated;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::clearUpdateLists()
{        
   mPFXInstances.resize(0);                     
   mLocalInstances.resize(0);
   mLocalOrder.resize(0);
   mLocalList.resize(0);
   mLocalTotalNumAllowedInstancesThisUpdate = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::clearRenderLists()
{
   const uint tileCount = gTiledAAManager.getNumTiles();
   mRenderInstances.resize(tileCount);
   
   for (uint i = 0; i < mRenderInstances.getSize(); i++)
      mRenderInstances[i].resize(0);

   mRenderNearLayerInstances.resize(tileCount);
   for (uint j = 0; j < mRenderNearLayerInstances.getSize(); ++j)
      mRenderNearLayerInstances[j].resize(0);
  
   //-- clear our list
   mRenderDistortionInstances.resize(0);
   mRenderNoTilingInstances.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateTickAndCull()
{
#ifndef BUILD_FINAL   
   uint totalParticlesAllocated = 0;   
#endif

   //trace("   ps time: %d", getTime());

   uint totalParticleBytesAllocated = 0;         
      
   BVector viewPosition;
   BVector position;
   BVector position2;
   BMatrix emitterMatrix;
   BMatrix emitterMatrix2;

   XMVECTOR cameraPos = XMVectorZero();
   XMVECTOR distanceToCameraV = XMVectorZero();      
   XMVECTOR halfV = XMVectorSplatX(XMLoadScalar(&cOneHalf));   

   BHandle hInstance;
   BParticleEmitter* pInstance = mUsedInstanceList.getHead(hInstance);
      
   //-- compute the z value of each emitter in view space so we can sort it
   XMVECTOR a;
   XMVECTOR b;
   const float cDefaultCullRadius = 30.0f;
   XMVECTOR cullRadiusDefault = XMVectorSplatX(XMLoadScalar(&cDefaultCullRadius));
   XMVECTOR cullRadiusV;
   XMVECTOR opacityV = gVectorOne;
   XMVECTOR bboxRadius = XMVectorZero();
   XMVECTOR bboxCentroid = XMVectorZero();
   XMVECTOR bboxDelta = XMVectorZero();
   BParticleEmitterDefinition* pData = NULL;   
   while (pInstance)
   {
      // get and validate our emitter proto data
      pData = pInstance->getData();
      BDEBUG_ASSERT(pData!=NULL);
      if (!pData)
      {
         pInstance = mUsedInstanceList.getNext(hInstance);
         continue;
      }
            
#ifndef BUILD_FINAL   
      totalParticlesAllocated += pInstance->getTotalParticlesAllocated();
#endif            
      emitterMatrix = pInstance->getTransform();
      emitterMatrix.getTranslation(position);
      
      
      //trace("       %p render -- %0.2f %0.2f %0.2f", pInstance, position.x, position.y, position.z);

#if 0 // Sim Visibility Test Code -- verifies that the sim is reporting valid visibility flags to us
      static bool bDisplayVisibility = true;
      if (bDisplayVisibility)
      {
         gpDebugPrimitives->addDebugSphere(position, pInstance->getFlag(BParticleEmitter::eFlagVisible) ? 0.5f : 1.0f, pInstance->getFlag(BParticleEmitter::eFlagVisible) ? cDWORDGreen : cDWORDRed, BDebugPrimitives::cCategoryParticles, 0.25f);
      }
#endif

#if 0 // DEBUG CODE - DON'T TURN ON UNLESS YOU WANT TO DEBUG a specific effect     
      static bool bDebugScarabBeam = false;
      if (bDebugScarabBeam)
      {
         BParticleEmitterDefinition* pEmitterData = pInstance->getData();
         if (pEmitterData && (pEmitterData->mParentPfxName.findRight("chimera_beam_01") != -1))
         {
            long temp = 0;
            temp++;
         }
      }
#endif     
      
      // compute bounding box info that we need for culling.
      a = pInstance->getBoundingMins();
      b = pInstance->getBoundingMaxs();
      bboxDelta = XMVectorSubtract(b,a);
      bboxCentroid = XMVectorMultiplyAdd(bboxDelta, halfV, a);
      bboxRadius = XMVectorMultiply(XMVector3Length(bboxDelta), halfV);
      // check if any of the axis of the box is larger than our cull radius if so then use that for the rough cull
      cullRadiusV = XMVectorMax(bboxRadius, cullRadiusDefault);

      // by default everything is not faded out or culled
      pInstance->setVisibleInAllViewports(true);
      pInstance->setBoundSphereVisibleForAllViewports(true);      
      pInstance->setCompletelyFadedOutForAllViewports(false);

      //-- perform cull checks if necessary      
      if (!pInstance->getFlag(BParticleEmitter::eFlagNearLayer) && 
          !pData->mProperties.mAlwaysRender && 
          !pData->mProperties.mAlwaysActive &&
           pData->mProperties.mType!=BEmitterBaseData::ePFX &&
           mbFlagEnableCulling)
      {               
         int viewportCount = gRenderDraw.getNumViewports();
         
         for (int v=0; v<viewportCount; ++v)
         {                        
            const BVolumeCuller& culler = gRenderDraw.getViewportDesc(v).mVolumeCuller;

      #ifdef ENABLE_BBOX_RENDERING
            gpDebugPrimitives->addDebugSphere(bboxCentroid, cullRadiusV.x, 0xFF00FFFF, BDebugPrimitives::cCategoryParticles);
            gpDebugPrimitives->addDebugBox(a, b, 0xFF00FFFF, BDebugPrimitives::cCategoryParticles);

            static bool bShowSphere = false;
            static BParticleEmitter* pTargetInstance = NULL;
            if (bShowSphere && (pInstance == pTargetInstance))
               gpDebugPrimitives->addDebugSphere(bboxCentroid, cullRadiusV.x, 0xFF00FFFF, BDebugPrimitives::cCategoryParticles);
      #endif
                        
            bool boundingSphereVisible = culler.isSphereVisible(bboxCentroid, cullRadiusV);         
            pInstance->setBoundSphereVisible(v, boundingSphereVisible);
         }
                           
#ifndef BUILD_FINAL      
         if (getFlagDistanceFade())
#endif
         {         
            int viewportCount = gRenderDraw.getNumViewports();
            for (int v=0; v<viewportCount; ++v)
            {
               const BRenderDraw::BViewportDesc& viewportDesc = gRenderDraw.getViewportDesc(v);
               cameraPos = viewportDesc.mMatrixTracker.getWorldCamPos();
               if (pInstance->getBoundSphereVisible(v))
               {
                  distanceToCameraV = XMVector3Length(XMVectorSubtract((XMVECTOR)bboxCentroid, cameraPos));                 
                  #ifdef ENABLE_DEBUG_DISTANCEFADERAGE_RENDERING
                  gpDebugPrimitives->addDebugSphere(bboxCentroid, mEmitterFadeStartDistance, 0xFF00FF00, BDebugPrimitives::cCategoryParticles);
                  gpDebugPrimitives->addDebugSphere(bboxCentroid, mEmitterFadeEndDistance, 0xFFFF0000, BDebugPrimitives::cCategoryParticles);
                  #endif

                  // am i too far away from the camera?  
                  if(XMVector4Greater(distanceToCameraV, mEmitterFadeEndDistanceV))
                  {                     
                     // how many pixels are visible even though I am too far.  If too many are visible 
                     // then we we can't be culled.
                     float projArea;
                     bool projResult = BVMXIntersection::calculateBoxArea(
                        projArea, 
                        viewportDesc.mMatrixTracker.getWorldCamPos(), 
                        a, b,
                        XMMatrixIdentity(),
                        viewportDesc.mMatrixTracker.getMatrix(cMTWorldToScreen));

                     float cVisiblePixelRadius = 10.0f;
                     float projRadius = sqrt(projArea * Math::fOOPi);
                     if (projResult && (projRadius < cVisiblePixelRadius))
                        pInstance->setCompletelyFadedOut(v, true);                  
                  }                                    
               }               
            }
         }
      }      
            
      //-- Everyone performs a tick to see if we can reclaim it.
      if (pInstance->tick(getTime()))
      {
#ifndef BUILD_FINAL   
         mWorkerStats.mTotalEmittersForceKilled++;
#endif                  
         BHandle nextHInstance = hInstance;
         BParticleEmitter* pNextInstance = mUsedInstanceList.getNext(nextHInstance);

         addEmitterToFreelist(pInstance);
         detachEmitterFromEffect(pInstance);
         pInstance->kill();
         
         #ifdef BUILD_DEBUG
         BHandle usedListHandle = NULL;
         bool bInUsedList = findEmitterInUsedList(pInstance, usedListHandle);
         BASSERT(bInUsedList);
         #endif

         mUsedInstanceList.remove(hInstance);

         pInstance = pNextInstance;
         hInstance = nextHInstance;
         continue;
      }

      //-- process the result of the tick and see if we can reclaim it.
      if (pInstance->isDead())
      {
#ifndef BUILD_FINAL   
         mWorkerStats.mTotalEmittersKilled++;
#endif         
         
         BHandle nextHInstance = hInstance;
         BParticleEmitter* pNextInstance = mUsedInstanceList.getNext(nextHInstance);

         addEmitterToFreelist(pInstance);
         detachEmitterFromEffect(pInstance);
         pInstance->kill();

         #ifdef BUILD_DEBUG
         BHandle usedListHandle = NULL;
         bool bInUsedList = findEmitterInUsedList(pInstance, usedListHandle);
         BASSERT(bInUsedList);
         #endif

         mUsedInstanceList.remove(hInstance);

         pInstance = pNextInstance;
         hInstance = nextHInstance;         
         continue;
      }
      
      // Update totalParticleBytesAllocated here so we properly account for killed instances.
      totalParticleBytesAllocated += pInstance->getTotalParticlesAllocatedInBytes();
      
      //-- skip any emitters that are not visible or not set to be always active
      if ( !pInstance->getFlag(BParticleEmitter::eFlagVisible) && 
           !pData->mProperties.mAlwaysRender &&
           !pData->mProperties.mAlwaysActive)
      {         
         // Emitter isn't visible, release all of its memory but don't kill it because it is still active.
         pInstance->setFlag(BParticleEmitter::eFlagHibernate, true);
         pInstance->returnAllParticles(false);
         pInstance->resetBoundingBox();
         pInstance = mUsedInstanceList.getNext(hInstance);
         continue;
      }
       
      //-- all the checks have been performed as necessary handle them for non Parent PFX emitters
      if (pData->mProperties.mType!=BEmitterBaseData::ePFX)
      {
         if ((!pInstance->getBoundSphereVisibleInAnyViewport()) || (pInstance->getCompletelyFadedOutForAllViewports()))
         {
            // Emitter isn't visible, release all of its memory but don't kill it because it is still active.
            pInstance->setFlag(BParticleEmitter::eFlagHibernate, true);
            pInstance->returnAllParticles(false);
            pInstance->resetBoundingBox();
            pInstance = mUsedInstanceList.getNext(hInstance);
            continue;
         }
      }


      
      //-- add instance to the appropriate pools for further processing
      if (pData->mProperties.mType == BEmitterBaseData::ePFX)
      {
         mPFXInstances.add(pInstance);
      }
      else
      {         
         mLocalInstances.add(pInstance);
         mLocalList.add(hInstance);

#ifdef USE_PRIORITY_SYSTEM
         mLocalOrder.add(pInstance->computeScore());
#else
         viewMatrix.transformVectorAsPoint(position, viewPosition);
         oneOverZ = 1.0f / viewPosition.z;
         mLocalOrder.add(oneOverZ);
#endif
      }
            
#ifndef BUILD_FINAL
      //-- disable distance fading and force to full opacity
      if (!getFlagDistanceFade())
         pInstance->setOpacity(1.0f);
#endif

      //-- if this instance is a near layer emitter then its opacity is always 1.0f
      if (pInstance->getFlag(BParticleEmitter::eFlagNearLayer) || pData->mProperties.mAlwaysRender)
         pInstance->setOpacity(1.0f);

      pInstance = mUsedInstanceList.getNext(hInstance);
   }
   
   mPermitEmission = (totalParticleBytesAllocated <= cMaxParticleBlockHeapSize);

#ifndef BUILD_FINAL
   mWorkerStats.mTotalParticlesAllocated = totalParticlesAllocated;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateParentEmitters()
{   
   for (int p=0; p < mPFXInstances.getNumber(); ++p)
   {
      mPFXInstances[p]->updatePhase1(mPermitEmission);
      mPFXInstances[p]->updatePhase2(NULL, NULL, 0, NULL, NULL, 0);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateRenderList(int viewportIndex)
{
   clearRenderLists();

   //-- PHX-18326
   //-- Bullet Proofed this in case someone calls us with a bogus viewport index   
   //-- If these asserts pop that means that the rendercontrol is calling us with an 
   //-- invalid viewport index and we need to figure out why that is happening.
   BASSERT(viewportIndex >= 0);
   BASSERT(viewportIndex < BParticleEmitter::cMaxViewports);
   if (viewportIndex < 0 || viewportIndex >= BParticleEmitter::cMaxViewports)
      return;

   DWORD* pIndices = mUpdateSorter.getIndices();
   if (!pIndices)
      return;

   uint numTiles = gTiledAAManager.getNumTiles();

#ifndef BUILD_FINAL
   mWorkerStats.setNumTiles(numTiles);
#endif  

#ifdef USE_PRIORITY_SYSTEM
   mZLocalInstances.resize(0);
   mZLocalOrder.resize(0);
   mZLocalList.resize(0);
   
   float* pNewEntry = NULL;
   BParticleEmitter* pInstance = NULL;
   BMatrix emitterMatrix;
   BVector position;
   BVector viewPosition;
   BMatrix viewMatrix = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToView);
   
   for (uint q = 0; (q < mLocalTotalNumAllowedInstancesThisUpdate) && (q < mLocalInstances.getSize()); q++)
   {
      pInstance = mLocalInstances[pIndices[q]];    

      if (!pInstance->getVisibleInViewport(viewportIndex))
         continue;

      mZLocalOrder.add(1.0f);
      pNewEntry = &(mZLocalOrder[mZLocalOrder.getSize()-1]);

      emitterMatrix = pInstance->getTransform();
      emitterMatrix.getTranslation(position);
      viewMatrix.transformVectorAsPoint(position, viewPosition);

      mZLocalList.add(mLocalList[pIndices[q]]);
      mZLocalInstances.add(pInstance);

      XMStoreScalar(pNewEntry, XMVectorSplatZ(XMVectorReciprocal(viewPosition)));
   }

   mRenderSorter.sort(mZLocalOrder.getData(), mZLocalOrder.getNumber());
   DWORD* pZIndices = mRenderSorter.getIndices();      
#endif

   static bool bDebugRender = false;
   XMVECTOR a;
   XMVECTOR b;
   BParticleEmitterDefinition* pData=NULL;

   for(int i=0; i<mZLocalInstances.getNumber(); i++)
   {
#ifdef USE_PRIORITY_SYSTEM
      pInstance = mZLocalInstances[pZIndices[i]];
#else
      pInstance = mLocalInstances[pIndices[i]];
#endif

      // get and validate our emitter proto data
      pData = pInstance->getData();
      BDEBUG_ASSERT(pData!=NULL);
      if (!pData)
         continue;

      if (bDebugRender)
         pInstance->debugRenderParticles();     

#ifndef BUILD_FINAL
      if (mbFlagRenderMagnets)
         pInstance->debugRenderMagnets();
#endif

      if (!pInstance->getFlag(BParticleEmitter::eFlagVisible) && !pData->mProperties.mAlwaysRender)
         continue;

#ifndef ENABLE_MULTITHREADING
      pInstance->update();
#endif
#ifndef BUILD_FINAL         
      mWorkerStats.mNumUpdatedParticles+=pInstance->getNumParticles();
#endif



#ifndef ENABLE_MULTITHREADING
      //-- cap the total number of vertices that we can render
      const uint numVerts = pInstance->getNumVerts();
      if ((totalVerts += numVerts) >= cMaxParticleVerts)
         break;
#endif
      //-- If this is a distortion effect just add it to our distortion list and continue
      //-- these don't need tile culling because they are being rendered to a seperate 
      //-- buffer that is not antialiased
      if (pInstance->getData()->mProperties.mBlendMode == BEmitterBaseData::eDistortion)
      {         
         mRenderDistortionInstances.add(pInstance);
         continue;
      }
      
      //-- add near layer emitters to all tiles for now
      if (pInstance->getFlag(BParticleEmitter::eFlagNearLayer))
      {         
         for (uint tile = 0; tile < numTiles; ++tile)
         {
            mRenderNearLayerInstances[tile].add(pInstance);
         }
         continue;
      }

      if (pInstance->getData()->mProperties.mFillOptimized)
      {
         mRenderNoTilingInstances.add(pInstance);
         continue;
      }
              
      a = pInstance->getBoundingMins();
      b = pInstance->getBoundingMaxs();      
      
      for (uint tile = 0; tile < numTiles; ++tile)
      {
         const BVolumeCuller& tileCuller = gTiledAAManager.getTileVolumeCuller(tile);

         if (!tileCuller.isAABBVisibleBounds(a, b))
            continue;

         mRenderInstances[tile].add( pInstance );

         #ifndef BUILD_FINAL
         mWorkerStats.mTileData[tile].mNumEmitters++;
         uint numParticles = pInstance->getNumParticles();
         mWorkerStats.mTileData[tile].mNumParticles+= numParticles;

         mWorkerStats.mTotalNumRenderedEmittersAcrossAllTiles++;
         mWorkerStats.mTotalNumRenderedParticlesAcrossAllTiles+=numParticles;
         #endif
      }
   }

   mRenderSorter.freeIndices();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateEmitters(BTimer& timer)
{
   mUpdateSorter.sort(mLocalOrder.getData(), mLocalOrder.getNumber());
   DWORD* pIndices = mUpdateSorter.getIndices();   
   const long totNumInstances = mLocalInstances.getNumber();
   if (totNumInstances)
   {
      uint totalVerts = 0;
      totalVerts;

      mLocalTotalNumAllowedInstancesThisUpdate = totNumInstances;
#ifdef ENABLE_MULTITHREADING
      {
         //-- compose the work data before we submit to the worker threads
         BDynamicParticleArray<BThreadUpdateData> threadData;
         threadData.resize(totNumInstances);         
         for (int j = 0; j < totNumInstances; ++j)
         {
            //pCurEmitter = mLocalInstances[pIndices[j]];
            threadData[j].mpEmitter      = mLocalInstances[pIndices[j]];
                        
            #ifndef BUILD_FINAL            
            mWorkerStats.mNumUpdatedParticlesPhase1 += mLocalInstances[pIndices[j]]->getNumParticles();
#if 0
            mWorkerStats.mNumUpdatedEmittersTypes[mLocalInstances[pIndices[j]]->getData()->mProperties.mType]+=1;
#endif
            #endif            
         }

         #ifndef BUILD_FINAL
         mWorkerStats.mNumUpdatedEmitters = totNumInstances;
         mWorkerStats.mNumUpdatedEmittersPhase1 = totNumInstances;
         #endif

         //-- initialize our work bucket countdown
         const uint cNumInstancesPerWorkBucketLog2 = 2;
         const uint cNumInstancesPerWorkBucket = 1 << cNumInstancesPerWorkBucketLog2;
         uint totalWorkBuckets = (totNumInstances + cNumInstancesPerWorkBucket - 1) >> cNumInstancesPerWorkBucketLog2;
         mRemainingWorkBuckets.set(totalWorkBuckets);

         //-- flush any existing buckets out
         gWorkDistributor.flush();
        
         //-- submit work to worker threads Update Phase 1
         BThreadUpdateData* pNextWorkEntry = threadData.getPtr();
         for (int k = 0; k < totNumInstances; ++k)
         {
            //emitterUpdatePhase1Callback((void*) pNextWorkEntry, 0, 0, false);
            gWorkDistributor.queue(emitterUpdatePhase1Callback, pNextWorkEntry, 0, cNumInstancesPerWorkBucket);
            pNextWorkEntry++;
         }

         //-- work for our work to be done
         gWorkDistributor.flushAndWaitSingle(mRemainingWorkBuckets);


         //-- compute the full VB size and fill out the rest of the worker data
         mTotalGPUFrameStorageNeededVB = 0;
         mTotalGPUFrameStorageNeededIB = 0;
         uint frameStorageNeededVB = 0;
         uint frameStorageNeededIB = 0;
         mLocalTotalNumAllowedInstancesThisUpdate = 0;
         for (int j = 0; j < totNumInstances; ++j)
         {
            //pCurEmitter = threadData[j].mpEmitter;
            threadData[j].mpEmitter->computeGPUFrameStorage(frameStorageNeededVB, frameStorageNeededIB);

            //-- if we have reached our max amount of frame storage then 
            //-- bail out
            if ((mTotalGPUFrameStorageNeededVB + mTotalGPUFrameStorageNeededIB + frameStorageNeededVB + frameStorageNeededIB) >= cMaxAllocatedFrameStorage)
               break;

            #ifndef BUILD_FINAL
            mWorkerStats.mNumUpdatedParticlesPhase2 += threadData[j].mpEmitter->getNumParticles();
            #endif

            //-- increment our count of instances that we are allowed to render
            mLocalTotalNumAllowedInstancesThisUpdate++;
            //-- the current storage need value is our start offset for this instance
            threadData[j].mStartOffsetVB = mTotalGPUFrameStorageNeededVB;
            threadData[j].mStartOffsetIB = mTotalGPUFrameStorageNeededIB;
            mTotalGPUFrameStorageNeededVB += frameStorageNeededVB;
            mTotalGPUFrameStorageNeededIB += frameStorageNeededIB;            
         }

         #ifndef BUILD_FINAL
         mWorkerStats.mNumUpdatedEmittersPhase2 = mLocalTotalNumAllowedInstancesThisUpdate;        
         #endif

         //-- if we got nothing to do then bail out.
         if (mTotalGPUFrameStorageNeededVB == 0)
         {
            #ifndef BUILD_FINAL
            mWorkerStats.mTotalUpdateTime = timer.getElapsedSeconds();
            updateRunTimeStats();
            #endif
            return;
         }
         
         IDirect3DVertexBuffer9* pVB = mpDynamicGPUBuffer->createVB(&mVB, mTotalGPUFrameStorageNeededVB);
         BVERIFY(pVB);
         void* pVBBuffer = NULL;
         pVB->Lock(0, 0, &pVBBuffer, 0);

         void* pIBBuffer = NULL;
         IDirect3DIndexBuffer9* pIB = NULL;
         if (mTotalGPUFrameStorageNeededIB > 0)
         {
            pIB = mpDynamicGPUBuffer->createIB(&mIB, mTotalGPUFrameStorageNeededIB);
            BVERIFY(pIB);
            pIB->Lock(0,0, &pIBBuffer, 0);
         }

         //-- initialize our work bucket countdown for phase 2 which updates all
         //-- existing particles and fills out the VBs
         gWorkDistributor.flush();
         totalWorkBuckets = (mLocalTotalNumAllowedInstancesThisUpdate + cNumInstancesPerWorkBucket - 1) >> cNumInstancesPerWorkBucketLog2;
         mRemainingWorkBuckets.set(totalWorkBuckets);

         pNextWorkEntry = threadData.getPtr();
         for (uint k = 0; k < mLocalTotalNumAllowedInstancesThisUpdate; ++k)
         {
            pNextWorkEntry->mpVB = pVB;
            pNextWorkEntry->mpIB = pIB;
            pNextWorkEntry->mpBufferVB = (void*) (((uchar*) pVBBuffer) + pNextWorkEntry->mStartOffsetVB);
            pNextWorkEntry->mpBufferIB = (void*) (((uchar*) pIBBuffer) + pNextWorkEntry->mStartOffsetIB);

            //emitterUpdatePhase2Callback((void*)pNextWorkEntry, 0, 0, false);
            gWorkDistributor.queue(emitterUpdatePhase2Callback, pNextWorkEntry, 0, cNumInstancesPerWorkBucket);
            pNextWorkEntry++;
         }
         
         //-- work for our work to be done
         gWorkDistributor.flushAndWaitSingle(mRemainingWorkBuckets);

         //-- unlock the VB and invalidate the GPU cache
         pVB->Unlock();

         if (pIB)
            pIB->Unlock();
      }      
#endif            
   } // if (totNumInstances)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::clearDebugStats()
{   
#ifndef BUILD_FINAL
   mWorkerStats.mNumUpdatedEmitters = 0;
   mWorkerStats.mNumUpdatedEmittersPhase1 = 0;
   mWorkerStats.mNumUpdatedEmittersPhase2 = 0;
   mWorkerStats.mNumUpdatedParticles = 0;
   mWorkerStats.mNumUpdatedParticlesPhase1 = 0;
   mWorkerStats.mNumUpdatedParticlesPhase2 = 0;
   mWorkerStats.mTotalNumRenderedParticlesAcrossAllTiles = 0;
   mWorkerStats.mTotalNumRenderedEmittersAcrossAllTiles = 0;   
#endif   
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateDebugStats(BTimer& timer)
{
#ifndef BUILD_FINAL
   timer.stop();
   mWorkerStats.mTotalUpdateTime = timer.getElapsedSeconds();
   mWorkerStats.mNumActiveEffects = mEffectList.getNumber();
   mWorkerStats.mDataCount = mData.getNumber();
   mWorkerStats.mUniqueEmitters = 0;
   for (int i = 0; i < mData.getNumber(); ++i)
   {
      mWorkerStats.mUniqueEmitters += mData[i]->mEmitters.getNumber();
   }

   mWorkerStats.mSceneMaxRenderedParticles = Math::Max(mWorkerStats.mSceneMaxRenderedParticles, mWorkerStats.mTotalNumRenderedParticlesAcrossAllTiles);
   mWorkerStats.mSceneMaxRenderedEmitters  = Math::Max(mWorkerStats.mSceneMaxRenderedEmitters, mWorkerStats.mTotalNumRenderedEmittersAcrossAllTiles);

   mWorkerStats.mTotalNumActiveEffectEmitters  = 0;
   mWorkerStats.mNumActiveLoopingEffectEmitters = 0;
   mWorkerStats.mNumActiveNonLoopEffectEmitters = 0;
   mWorkerStats.mNumActiveInvalidDataEffectEmitters = 0;
   for (uint j = 0; j < mWorkerStats.mNumActiveEffects; ++j)
   {
      uint numEmitters = mEffectList[j]->mEmitters.getNumber();
      mWorkerStats.mTotalNumActiveEffectEmitters += numEmitters;
      for (uint k = 0; k < numEmitters; ++k)
      {
         BParticleEmitterDefinition* pData = mEffectList[j]->mEmitters[k]->getData();
         if (pData)
         {
            if (pData->mProperties.mLoop)
               mWorkerStats.mNumActiveLoopingEffectEmitters++;
            else
               mWorkerStats.mNumActiveNonLoopEffectEmitters++;
         }
         else
            mWorkerStats.mNumActiveInvalidDataEffectEmitters++;
      }
   }
   updateRunTimeStats();
#endif   
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateFrame()
{
   ASSERT_RENDER_THREAD

   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryParticles);

#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDump = false;
   if (bTraceDump)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\psUpdate.bin" );
   }
#endif
   
   BTimer timer;   
   gPSTextureManager.tick();

   #ifdef ENABLE_DEBUG_PRIMITIVES
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryParticles);   
   //gpDebugPrimitives->addDebugSphere(p1, 3.0f, 0xFFFFFFFF, BDebugPrimitives::cCategoryParticles);
   #endif


   #ifndef BUILD_FINAL      
   timer.start();
   #endif

   SCOPEDSAMPLE(BParticleSystemUpdate);

   clearUpdateLists();

   updateTickAndCull();
      
   #ifndef BUILD_FINAL   
   clearDebugStats();   
   #endif

   updateParentEmitters();   

   if (gpRenderDebugPrimitives)
      gpRenderDebugPrimitives->setThreadSafeFlag(true);

   updateEmitters(timer);

   if (gpRenderDebugPrimitives)
      gpRenderDebugPrimitives->setThreadSafeFlag(false);
            
#ifdef DEBUG_TRACE_RECORD
   if (bTraceDump)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }
#endif
     
#ifndef BUILD_FINAL   
   updateDebugStats(timer);   
#endif   

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderBegin()
{
   ASSERT_RENDER_THREAD
   
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif

   SCOPEDSAMPLEID(ParticleRenderBegin, 0xFFFF0000);
   int tileCount = mRenderInstances.getNumber();
   int instanceCount = 0;

   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderInstances[i].getNumber();
      for (int j = 0; j < instanceCount; j++)
         mRenderInstances[i][j]->renderBegin();
   }

#ifndef BUILD_FINAL
   timer.stop();
   mWorkerStats.mRenderTime = timer.getElapsedSeconds();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::render(uint tileIndex)
{
   ASSERT_RENDER_THREAD      
   SCOPEDSAMPLEID(ParticleRender, 0xFFFF0000);   
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);   
   
   debugRangeCheck(tileIndex, mRenderInstances.getNumber());
   int numInstancesToRender = mRenderInstances[tileIndex].getNumber();
   for (int i = 0; i < numInstancesToRender; i++)
   {
      mRenderInstances[tileIndex][i]->render(tileIndex);
   }
   
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderEnd()
{
   ASSERT_RENDER_THREAD
      
   int tileCount = mRenderInstances.getNumber();
   int instanceCount = 0;
   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderInstances[i].getNumber();
      for (int j = 0; j < instanceCount; ++j)
         mRenderInstances[i][j]->renderEnd();
   }            
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderEndOfFrame()
{
   ASSERT_RENDER_THREAD
      
   int tileCount = mRenderInstances.getNumber();
   int instanceCount = 0;
   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderInstances[i].getNumber();
      for (int j = 0; j < instanceCount; ++j)
         mRenderInstances[i][j]->renderEndOfFrame();

      mRenderInstances[i].clear();
   }            
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNoTilingBegin()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNoTilingBegin, 0xFFFF0000);
   

   uint width  = gTiledAAManager.getWidth();
   uint height = gTiledAAManager.getHeight();
   uint halfWidth = width / 2;
   uint halfHeight = height /2;
   D3DFORMAT colorFormat = gTiledAAManager.getEDRAMColorFormat();
   D3DFORMAT depthFormat = gTiledAAManager.getDepthFormat();

   D3DSURFACE_PARAMETERS surfParamsColor;
   Utils::ClearObj(surfParamsColor);   
   HRESULT status = gGPUFrameHeap.createRenderTarget(width, height, colorFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &mpColorSurface, &surfParamsColor);
   BVERIFY(SUCCEEDED(status));

   D3DSURFACE_PARAMETERS surfParamsDepth;   
   Utils::ClearObj(surfParamsDepth);
   surfParamsDepth.Base += XGSurfaceSize(width, height, colorFormat, D3DMULTISAMPLE_NONE);      
   status = gGPUFrameHeap.createRenderTarget(width, height, depthFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &mpDepthSurface, &surfParamsDepth);
   BVERIFY(SUCCEEDED(status));

   gToneMapManager.fillDepthStencilSurface(mpColorSurface, mpDepthSurface, width, height, getFlagUseAliasedFillSurface());
   gToneMapManager.fillColorSurface(mpColorSurface, mpDepthSurface, width, height, getFlagUseAliasedFillSurface());

   D3DSURFACE_PARAMETERS surfParamsColorAliasedAA;
   Utils::ClearObj(surfParamsColorAliasedAA);   
   status = gGPUFrameHeap.createRenderTarget(halfWidth, halfHeight, colorFormat, D3DMULTISAMPLE_4_SAMPLES, 0, FALSE, &mpColorSurfaceAliasedAA, &surfParamsColorAliasedAA);
   BVERIFY(SUCCEEDED(status));

   D3DSURFACE_PARAMETERS surfParamsDepthAliasedAA;   
   Utils::ClearObj(surfParamsDepthAliasedAA);
   surfParamsDepthAliasedAA.Base += XGSurfaceSize(halfWidth, halfHeight, colorFormat, D3DMULTISAMPLE_4_SAMPLES);      
   status = gGPUFrameHeap.createRenderTarget(halfWidth, halfHeight, depthFormat, D3DMULTISAMPLE_4_SAMPLES, 0, FALSE, &mpDepthSurfaceAliasedAA, &surfParamsDepthAliasedAA);
   BVERIFY(SUCCEEDED(status));
   BASSERT(surfParamsDepth.Base == surfParamsDepthAliasedAA.Base);

   IDirect3DSurface9* pRenderTarget = mpColorSurfaceAliasedAA;
   IDirect3DSurface9* pDepthStencil = mpDepthSurfaceAliasedAA;
   if (!getFlagUseAliasedFillSurface())
   {
      pRenderTarget = mpColorSurface;
      pDepthStencil = mpDepthSurface;
   }

   D3DSURFACE_DESC desc;
   pRenderTarget->GetDesc(&desc);
      
   BMatrixTracker&  matrixTracker   = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();
   
   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = desc.Width;
   viewport.Height = desc.Height;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;

   matrixTracker.setViewport(viewport);
   
   renderViewport.setSurf(0, pRenderTarget);
   renderViewport.setDepthStencilSurf(pDepthStencil);
   renderViewport.setViewport(viewport);

   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);
      
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif
   
   int instanceCount = mRenderNoTilingInstances.getNumber();
   for (int i = 0; i < instanceCount; i++)
      mRenderNoTilingInstances[i]->renderBegin();

#ifndef BUILD_FINAL
   timer.stop();
   mWorkerStats.mRenderTime = timer.getElapsedSeconds();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNoTiling()
{
   ASSERT_RENDER_THREAD      
   SCOPEDSAMPLEID(ParticleRenderNoTiling, 0xFFFF0000);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);

   static DWORD mask = 0xffffffff;
   BD3D::mpDev->SetRenderState(D3DRS_MULTISAMPLEMASK, mask);
   
   int numInstancesToRender = mRenderNoTilingInstances.getNumber();
   for (int i = 0; i < numInstancesToRender; i++)
   {
      mRenderNoTilingInstances[i]->render(-1);
   }

   BD3D::mpDev->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xffffffff);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNoTilingEnd()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNoTilingEnd, 0xFFFF0000);
      
   int instanceCount = mRenderNoTilingInstances.getNumber();
   for (int i = 0; i < instanceCount; ++i)
      mRenderNoTilingInstances[i]->renderEnd();
      
   BD3D::mpDev->SetRenderTarget(0, mpColorSurface);   
   BD3D::mpDev->SetDepthStencilSurface(mpDepthSurface);

   D3DRECT srcRect;
   srcRect.x1 = 0;
   srcRect.y1 = 0;
   srcRect.x2 = gTiledAAManager.getWidth();
   srcRect.y2 = gTiledAAManager.getHeight();
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, &srcRect, gTiledAAManager.getColorTexture(), NULL, 0, 0, NULL, 0.0f, 0, NULL);      
   
   gRenderDraw.resetWorkerActiveMatricesAndViewport();
   
   gGPUFrameHeap.releaseD3DResource(mpColorSurface);
   mpColorSurface = NULL;
   gGPUFrameHeap.releaseD3DResource(mpDepthSurface);
   mpDepthSurface = NULL;

   gGPUFrameHeap.releaseD3DResource(mpColorSurfaceAliasedAA);
   mpColorSurfaceAliasedAA = NULL;
   gGPUFrameHeap.releaseD3DResource(mpDepthSurfaceAliasedAA);
   mpDepthSurfaceAliasedAA = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNoTilingEndOfFrame()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNoTilingEndOfFrame, 0xFFFF0000);
      
   int instanceCount = mRenderNoTilingInstances.getNumber();
   for (int i = 0; i < instanceCount; ++i)
      mRenderNoTilingInstances[i]->renderEndOfFrame();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderDistortionBegin()
{
   // rg [10/25/07] - This method may be called even though no distortion particles are visible, due to distortion meshes.
   // This stuff should be moved to a "distortion manager".
   ASSERT_RENDER_THREAD    

   SCOPEDSAMPLEID(ParticleRenderDistortionBegin, 0xFFFF0000);
   
   const uint width = Math::Clamp<uint>(gTiledAAManager.getWidth() / 2U, 1U, mDistortionRenderTarget.getWidth());
   const uint height = Math::Clamp<uint>(gTiledAAManager.getHeight() / 2U, 1U, mDistortionRenderTarget.getHeight());
   
   // rg - FIXME, once we move the fill depth stencil surface to the tiled AA manager
   gToneMapManager.fillDepthStencilSurface(mDistortionRenderTarget.getColorSurf(), mDistortionRenderTarget.getDepthSurf(), width, height, false);
   
   mDistortionRenderTarget.begin(NULL);
   
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0, 0 );
      
   BMatrixTracker&  matrixTracker   = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();
      
   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = width;
   viewport.Height = height;
   
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;

   matrixTracker.setViewport(viewport);

   renderViewport.setSurf(0, mDistortionRenderTarget.getColorSurf());
   renderViewport.setDepthStencilSurf(mDistortionRenderTarget.getDepthSurf());
   renderViewport.setViewport(viewport);
   
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);
   
   int instanceCount = mRenderDistortionInstances.getNumber();
   for (int j = 0; j < instanceCount; j++)
      mRenderDistortionInstances[j]->renderBegin();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderDistortion()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderDistortion, 0xFFFF0000);
   
   int numInstancesToRender = mRenderDistortionInstances.getNumber();
   for (int i = 0; i < numInstancesToRender; i++)
      mRenderDistortionInstances[i]->render(-1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderDistortionEnd()
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLEID(ParticleRenderDistortionEnd, 0xFFFF0000);
   int instanceCount = mRenderDistortionInstances.getNumber();
   for (int j = 0; j < instanceCount; ++j)
      mRenderDistortionInstances[j]->renderEnd();   

   HRESULT hres = gGPUFrameHeap.createTexture(mDistortionBufferWidth, mDistortionBufferHeight, 1, 0, D3DFMT_G16R16F_EXPAND, D3DPOOL_DEFAULT, &mpDistortionTexture, NULL);
   BVERIFY(SUCCEEDED(hres));
   
   if (mpDistortionTexture)
      mDistortionRenderTarget.resolve(mpDistortionTexture);   
      
   mDistortionRenderTarget.end();   
   
   gRenderDraw.resetWorkerActiveMatricesAndViewport();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderDistortionEndOfFrame()
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLEID(ParticleRenderDistortionEndOfFrame, 0xFFFF0000);
   int instanceCount = mRenderDistortionInstances.getNumber();
   for (int j = 0; j < instanceCount; ++j)
      mRenderDistortionInstances[j]->renderEndOfFrame();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNearLayerBegin()
{
   ASSERT_RENDER_THREAD    
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif

   SCOPEDSAMPLEID(ParticleRenderNearLayerBegin, 0xFFFF0000);
   
   int tileCount = mRenderNearLayerInstances.getNumber();
   int instanceCount = 0; 
   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderNearLayerInstances[i].getNumber();
      for (int j = 0; j < instanceCount; j++)
         mRenderNearLayerInstances[i][j]->renderBegin();
   }

#ifndef BUILD_FINAL
   timer.stop();
   mWorkerStats.mRenderTime = timer.getElapsedSeconds();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNearLayer(uint tileIndex)
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNearLayer, 0xFFFF0000);

   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);

   debugRangeCheck(tileIndex, mRenderNearLayerInstances.getNumber());
   int numInstancesToRender = mRenderNearLayerInstances[tileIndex].getNumber();
   for (int i = 0; i < numInstancesToRender; i++)
   {
      mRenderNearLayerInstances[tileIndex][i]->render(tileIndex);
   }

   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNearLayerEnd()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNearLayerEnd, 0xFFFF0000);

   int tileCount = mRenderNearLayerInstances.getNumber();
   int instanceCount = 0;
   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderNearLayerInstances[i].getNumber();
      for (int j = 0; j < instanceCount; ++j)
         mRenderNearLayerInstances[i][j]->renderEnd();      
   }            
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::renderNearLayerEndOfFrame()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(ParticleRenderNearLayerEndOfFrame, 0xFFFF0000);

   int tileCount = mRenderNearLayerInstances.getNumber();
   int instanceCount = 0;
   for (int i = 0; i < tileCount; i++)
   {
      instanceCount = mRenderNearLayerInstances[i].getNumber();
      for (int j = 0; j < instanceCount; ++j)
         mRenderNearLayerInstances[i][j]->renderEndOfFrame();

      mRenderNearLayerInstances[i].clear();
   }            
}

//============================================================================
//============================================================================
void BParticleSystemManager::releaseGPUFrameHeapResources()
{
   if (mpDistortionTexture)
   {
      gGPUFrameHeap.releaseD3DResource(mpDistortionTexture);
      mpDistortionTexture = NULL;
   }
}

//============================================================================
//============================================================================
void BParticleSystemManager::releaseDynamicGPUResources()
{
   // Important: DO NOT do any more particle rendering after unlock() is called!
   mUpdateSorter.freeIndices();
   mpDynamicGPUBuffer->unlock();

#ifndef BUILD_FINAL

#if 0
   static bool bDoExtendedStats = true;
   if (bDoExtendedStats)
   {
      // Code to find emitters that have gone stale
      BHandle hInstance;
      BParticleEmitter* pInstance = mUsedInstanceList.getHead(hInstance);
      while( pInstance )
      {
         BParticleEmitter::eEmitterState state = pInstance->getState();
         if (state == BParticleEmitter::eStateAboutToDie)
         {
            mWorkerStats.mTotalEmittersAboutToDie++;
            if (pInstance->getData()) 
            {           
               if (pInstance->getData()->mProperties.mLoop)
                  mWorkerStats.mTotalEmittersAboutToDieLooping++;
               else 
                  mWorkerStats.mTotalEmittersAboutToDieNonLooping++;
            }
            else 
               mWorkerStats.mTotalEmittersAboutDieUnaccountedFor++;
         }
         else if (state == BParticleEmitter::eStateActive)
         {
            mWorkerStats.mTotalEmittersActive++;
            if (pInstance->getData()) 
            {           
               if (pInstance->getData()->mProperties.mLoop)
                  mWorkerStats.mTotalEmittersActiveLooping++;
               else 
                  mWorkerStats.mTotalEmittersActiveNonLooping++;
            }
            else 
               mWorkerStats.mTotalEmittersActiveUnaccountedFor++;
         }

         pInstance = mUsedInstanceList.getNext(hInstance);
      }
   }
#endif

   // Okay for this to use the primary heap.
   BParticleSystemManager::BStats* pPayload = new BParticleSystemManager::BStats(mWorkerStats);

   //-- can clear our worker stats now
   mWorkerStats.clear();

   gEventDispatcher.send(cInvalidEventReceiverHandle, mThreadEventHandle[cThreadIndexSim], cParticleSystemEventClassUpdateStats, 0, 0, pPayload);
#endif   
}

//============================================================================
//============================================================================
int BParticleSystemManager::findDataIndex(const BString& filename)
{
   ASSERT_RENDER_THREAD

   if (filename.length() == 0)
      return -1;

   BString temp = filename;
   temp.removeExtension();
   temp.toLower();

   BParticleEffectDataHashMap::const_iterator it = mDataHashmap.find(temp);
   if (it == mDataHashmap.end())
      return -1;

   return it->second;         
}

//============================================================================
// BParticleEffectDefinition* BParticleSystemManager::getData()
//============================================================================
BParticleEffectDefinition* BParticleSystemManager::getData(const BString& filename, int* pIndex)
{
   ASSERT_RENDER_THREAD

#if 0
   static bool bDumpHashMap = false;
   if (bDumpHashMap)
   {
      dumpHashmap();      
      bDumpHashMap = false;
   }
#endif
   
   int dataIndex = findDataIndex(filename);
   if (dataIndex != -1)
   {
      if (pIndex)
         *pIndex = dataIndex;

      return getData(dataIndex);
   }

   //- if we got here then we need to load it.
   int index = -1;
   if (!loadEffectDefinition(filename, &index))
   {
      if (pIndex)
         *pIndex = -1;

      return NULL;
   }

   if (pIndex)
      *pIndex = index;

   return getData(index);
}

//=================================================================`===========
// const BParticleEffectDefinition& BParticleSystemManager::getData();
//============================================================================
BParticleEffectDefinition* BParticleSystemManager::getData(int index)
{
   ASSERT_RENDER_THREAD
   
   debugRangeCheck(index, mData.getSize());
   //return mData[index];
   return mData[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::loadEffectDefinition(const BString& fileName, int* pIndex)
{
   ASSERT_RENDER_THREAD
   
   if (fileName.length() == 0)
      return false;
   
   BString filepath = fileName;
   
   //-- Add a blank to the list.  We will load the file after its
   //-- already in the list, so we don't have to deal with a deep copy.
   BParticleEffectDefinition* pData = ALIGNED_NEW(BParticleEffectDefinition, gParticleHeap);
   bool ok = pData->load(filepath);
   if (!ok)
   {
      ALIGNED_DELETE(pData, gParticleHeap);
      if (pIndex)
         *pIndex = -1;

      return false;
   }
   
   // add to data array
   int newIndex = mData.getSize();   
   mData.pushBack(&pData, 1);
   *pIndex = newIndex;

   // add to hash for fast searches
   filepath.removeExtension();
   filepath.toLower();
   mDataHashmap.insert(filepath, newIndex);  

   return true;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEmitter* BParticleSystemManager::createEmitter()
{
   ASSERT_RENDER_THREAD
   
   BParticleEmitter* pInstance = mFreeInstanceList.removeHead();
   if (pInstance == NULL)
   {
      //-- Add a new block.
      BParticleEmitter* pBlock = ALIGNED_NEW_ARRAY(BParticleEmitter, mInstanceBlockSize, gParticleHeap);
      mInstanceBlocks.addToTail(pBlock);
      for (long index = 0; index < mInstanceBlockSize; ++index)
         mFreeInstanceList.addToTail(&pBlock[index]);

      //-- Try again.
      pInstance = mFreeInstanceList.removeHead();
   }

   //-- Add it to the used list.
   if (pInstance)
   {
#ifdef BUILD_DEBUG
      BHandle hNode;
      bool bInUsedList = findEmitterInUsedList(pInstance, hNode);
      BASSERT(bInUsedList == false);
#endif
      mUsedInstanceList.addToTail(pInstance);
   }

   return pInstance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::releaseEffect(BParticleEffect* pEffect, bool bKillImmediately, bool releaseEmitters)
{
   ASSERT_RENDER_THREAD
      
   if (!pEffect)
      return;

   int activeEffectCount = mEffectList.getSize();
   BParticleEffect* pBack = NULL;
   for (int i = 0; i < activeEffectCount; ++i)
   {
      if (mEffectList[i] == pEffect)
      {
         uint count = pEffect->mEmitters.getSize();
         for (uint k = 0; k < count; ++k)
         {
            if (releaseEmitters)
               releaseEmitter(pEffect->mEmitters[k], bKillImmediately); 

            //-- mark the emitter up to let it know it doesn't need to
            //-- notify the parent emitter.
            BDEBUG_ASSERT(pEffect->mEmitters[k] != NULL);
            pEffect->mEmitters[k]->setParentEffectIndex(-2);
         }

         pBack = mEffectList.back();

         if (pBack != mEffectList[i])
         {
            mEffectList[i] = pBack;

            //if I am getting shifted tell my emitters my new index
            if (pBack)
            {
               for (uint j = 0; j < pBack->mEmitters.getSize(); ++j)
               {
                  pBack->mEmitters[j]->setParentEffectIndex(i);
               }
            }            
         }

         mEffectList.popBack();
       
         

         //-- decrement our ref count of the data
         BParticleEffectDefinition* pData = getData(pEffect->mDataIndex);
         BDEBUG_ASSERT(pData!=NULL);
         pData->decRef();

         pEffect->mDataIndex = -1;

         //-- delete the actual effect instance
         ALIGNED_DELETE(pEffect, gParticleHeap);
         return;
      }
   }    
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEffect* BParticleSystemManager::createEffect(const BParticleCreateParams& params) //int index, BMatrix matrix)
{
   ASSERT_RENDER_THREAD
   
   BParticleEffectDefinition* pEffectData = getData(params.mDataHandle);
   if (!pEffectData)
      return NULL;

   uint refCount = pEffectData->mRefCount;
   refCount;
   
#if 0
   if (refCount >= cMaxInstancesPerDataLimit)
   {
#ifndef BUILD_FINAL 
      mWorkerStats.mNumDroppedEffects++;
#endif
      return NULL;
   }
#endif
   //-- how many emitters does this effect need?
   int emitterCount = pEffectData->mEmitters.size();

   int emittersLeft = cMaxPermittedEmitters - mUsedInstanceList.getSize();

   // if we don't have any emitters left don't create the effect;
   if (emittersLeft < 1)
   {
//      trace("Too many Emitters: %d", mUsedInstanceList.getSize());      
      return NULL;
   }
      
   // if we don't have enough open emitter slots left to create the ENTIRE
   // effect then don't create the effect at all
   if (emitterCount > emittersLeft)
   {
//      trace("ParticleSystemManager (WARNING) : Effect could not be created!  Not enough open emitters available");
      return NULL;
   }
   
   BParticleEffect* pNewEffect = ALIGNED_NEW(BParticleEffect, gParticleHeap);
   pNewEffect->mDataIndex = params.mDataHandle; //index;
   pNewEffect->mPriority = params.mPriority;
   BDEBUG_ASSERT(pNewEffect!=NULL);

   int newEffectIndex = mEffectList.getSize();
   for(int i=0; i<emitterCount; i++)
   {
      // rg [8/26/06] - HACK
      if (mUsedInstanceList.getSize() >= cMaxPermittedEmitters)
      {
//         trace("Too many Emitters: %d", mUsedInstanceList.getSize());
         BASSERT(0);
         break;
      }
      
      BParticleEmitterDefinition* pEmitterData = pEffectData->mEmitters[i];
      BDEBUG_ASSERT(pEmitterData!=NULL);

      BParticleEmitter* pEmitter = createEmitter();
      BDEBUG_ASSERT(pEmitter!=NULL);

      if (pEmitter)
      {
#ifndef BUILD_FINAL   
         mWorkerStats.mTotalEmittersCreated++;
#endif                  
         pEmitter->init(this, pEmitterData, params.mMatrix, params.mTintColor, 0.0f);
         pEmitter->setTransform(params.mMatrix);
         pEmitter->setFlag(BParticleEmitter::eFlagNearLayer, params.mNearLayerEffect);
         pEmitter->setPriority(static_cast<float>(params.mPriority));
         pEmitter->setParentEffectIndex(newEffectIndex);
        
         //-- add it to our effect
         pNewEffect->mEmitters.pushBack(pEmitter);
      }
   }

   BASSERT(pNewEffect->mEmitters.getSize() > 0);

   //-- increment our ref count
   pEffectData->addRef();

   mEffectList.add(pNewEffect);
   return pNewEffect;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::destroyAllInstances()
{
   ASSERT_RENDER_THREAD
   
   mRenderInstances.clear();
   mRenderDistortionInstances.clear();
   mRenderNearLayerInstances.clear();
   mRenderNoTilingInstances.clear();
  
   killAllInstances();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::destroyAllData()
{
   ASSERT_RENDER_THREAD        
   killAllData();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::releaseEmitter(BParticleEmitter* pEmitter, bool killNow /*false*/)
{
   ASSERT_RENDER_THREAD
   
   if (!pEmitter)
      return;

   BParticleEmitterDefinition* pData = pEmitter->getData();
   if (pData && pData->mProperties.mKillImmediatelyOnRelease)
      killNow = true;

   //During load there are instances that are getting created and immediately destroyed.
   //It would be better if this never happened, but when it does, we want to kill the particle
   //instantly instead of letting it die slowly out.
   bool wasCreatedThisFrame = ( pEmitter->getStartTime() == getTime() );

   if (!killNow && !wasCreatedThisFrame) 
   {
      pEmitter->setToDie();      
      return;
   }

   pEmitter->kill();

   addEmitterToFreelistAndRemoveFromUsedList(pEmitter);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::detachEmitterFromEffect(BParticleEmitter* pEmitter)
{
   ASSERT_RENDER_THREAD
   if (!pEmitter)
      return;

   int index = pEmitter->getParentEffectIndex();

   //-- the effect released the emitter on his own we don't need to notify it.
   if (index == -2)
      return;

   debugRangeCheck(index, 0, (int) mEffectList.getNumber());
   if (index < 0 || index >= mEffectList.getNumber())
      return;

   BParticleEffect* pEffect = mEffectList[index];
   if (!pEffect)
   {
      #ifndef BUILD_DEBUG
      trace("WARNING: Emitter had bogus parent index!!!!");
      #endif

      return;
   }

   BParticleEmitter* pBack = NULL;
   for (int i = 0; i < (int) pEffect->mEmitters.getSize(); i++)
   {
      if (pEmitter == pEffect->mEmitters[i])
      {
         pBack = pEffect->mEmitters.back();
         if (pBack != pEffect->mEmitters[i])
            pEffect->mEmitters[i] = pBack;

         pEffect->mEmitters.popBack();
      }
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::addEmitterToFreelistAndRemoveFromUsedList(BParticleEmitter* pEmitter)
{
   ASSERT_RENDER_THREAD

   BHandle hUsedListNode = NULL;
   bool bInUsedList = findEmitterInUsedList(pEmitter, hUsedListNode);
   BDEBUG_ASSERT(bInUsedList);      
   mUsedInstanceList.remove(hUsedListNode);
   addEmitterToFreelist(pEmitter);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::addEmitterToFreelist(BParticleEmitter* pEmitter)
{   
   ASSERT_RENDER_THREAD

#ifdef BUILD_DEBUG
   BHandle hFreeListNode = NULL;
   bool bInFreeList = findEmitterInFreelist(pEmitter, hFreeListNode);
   BASSERT(!bInFreeList);
   if (!bInFreeList)
#endif
   {
      mFreeInstanceList.addToTail(pEmitter);
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::findEmitterInFreelist(BParticleEmitter* pEmitter, BHandle& hNode)
{
   ASSERT_RENDER_THREAD
   hNode = mFreeInstanceList.findPointerForward(pEmitter);
   if (hNode == NULL)
      return false;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::findEmitterInUsedList(BParticleEmitter* pEmitter, BHandle& hNode)
{
   ASSERT_RENDER_THREAD
   hNode = mUsedInstanceList.findPointerForward(pEmitter);
   if (hNode == NULL)
      return false;
   return true;
}

//============================================================================
//  HELPER FUNCTIONS
//============================================================================
long BParticleSystemManager::getRandom(long range)
{
   BDEBUG_ASSERT(range > 0);

   BASSERT(mpRandom);   
   return mpRandom->iRand(0, range);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BParticleSystemManager::getRandomRange(float min, float max)
{
   BASSERT(mpRandom);   
   return mpRandom->fRand(min, max);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BParticleSystemManager::applyVariance(float value, float variance)
{
   BASSERT(variance >= 0.0f);

   //-- If there's no variance, just give the value back. 
   if (variance < cFloatCompareEpsilon)
      return value;

   //-- Apply variance.
   BASSERT(mpRandom);   
   float percent = mpRandom->fRand(-1.0f, 1.0f);
   float scale   = 1.0f + (percent * variance);
   return (value * scale);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD BParticleSystemManager::lerpColor(DWORD color1, DWORD color2, float alpha)
{
   ASSERT_RENDER_THREAD
   XMCOLOR c1(color1);
   XMCOLOR c2(color2);

   XMVECTOR vC1 = XMLoadColor((const XMCOLOR*) &c1);;
   XMVECTOR vC2 = XMLoadColor((const XMCOLOR*) &c2);

   XMVECTOR vOut = XMVectorLerp(vC1, vC2, alpha);
   XMCOLOR outColor;
   XMStoreColor(&outColor, vOut);

   return outColor.c;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BParticleSystemManager::lerpScalar(float a, float b, float alpha)
{
   ASSERT_RENDER_THREAD
   XMVECTOR v;
   v.x = a;
   XMVECTOR v2;
   v2.x = b;
   XMVECTOR vOut = XMVectorLerp(v, v2, alpha);
   return vOut.x;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updatePSysManTime(double gameTime)
{
   ASSERT_RENDER_THREAD
   if (getFlagPauseUpdate())
      return;
      
   // rg [3/1/07] - This may be dangerous? Is this absolute time?
   mPSysManTime = Math::FloatToIntTrunc((float)(gameTime * 1000.0f));

   mPSysManTimeF = gameTime;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::reinitAllInstanceTimeData()
{
   ASSERT_RENDER_THREAD
   BHandle                   hInstance;
   BParticleEmitter* pInstance = mUsedInstanceList.getHead(hInstance);
   while( pInstance )
   {
      pInstance->reinitTimeData();
      pInstance = mUsedInstanceList.getNext(hInstance);
   }
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
void BParticleSystemManager::calculateOrthoProjMatrix(float fov, long windowWidth, long windowHeight, float nearZ, float farZ, D3DXMATRIX &orthoProjMatrix)
{
   ASSERT_RENDER_THREAD
   // Use near plane to calculate width and height of the viewport
   float width = nearZ * tan(fov / 2.0f) * 2;
   float height = width * windowHeight / windowWidth;

   D3DXMatrixOrthoLH(&orthoProjMatrix, width, height, nearZ, farZ);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::updateRunTimeStats()
{   
   ASSERT_RENDER_THREAD
#ifndef BUILD_FINAL
   BHandle handle;
   BParticleEmitter* pEmitter = mUsedInstanceList.getHead(handle);
   BParticleEmitterDefinition* pData = NULL;
   mWorkerStats.mNumActiveLoopEmitters = 0;
   mWorkerStats.mNumActiveNonLoopEmitters = 0;
   mWorkerStats.mNumActiveEmitters = 0;
   mWorkerStats.mNumActiveParticles = 0;
   while (pEmitter)
   {     
      mWorkerStats.mNumActiveEmitters++;
      mWorkerStats.mNumActiveParticles += pEmitter->getNumParticles();

      pData = pEmitter->getData();
      if (pData)
      {
         if (pData->mProperties.mLoop)
            mWorkerStats.mNumActiveLoopEmitters++;
         else
            mWorkerStats.mNumActiveNonLoopEmitters++;
      }
      pEmitter = mUsedInstanceList.getNext(handle);
   }

   mWorkerStats.mSceneMaxActiveEffects = Math::Max(mWorkerStats.mSceneMaxActiveEffects, mWorkerStats.mNumActiveEffects);
   mWorkerStats.mSceneMaxActiveEmitters = Math::Max(mWorkerStats.mSceneMaxActiveEmitters, mWorkerStats.mNumActiveEmitters);
   mWorkerStats.mSceneMaxActiveParticles= Math::Max(mWorkerStats.mSceneMaxActiveParticles, mWorkerStats.mNumActiveParticles);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{   
   switch(threadIndex)
   {
      case cThreadIndexRender:
         {
            if (event.mEventClass == cEventClassReloadNotify)
            {
               BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
               #ifndef BUILD_FINAL
                  trace("%s\n", pPayload->mPath.getPtr());
               #endif
               
               gConsoleOutput.status("Reloading particle file: %s", pPayload->mPath.getPtr());

               BString filepath(pPayload->mPath);
               filepath.findAndReplace("art\\", "");
               return reloadEffect(filepath);
            }
            break;
         }

      case cThreadIndexSim:
         {
#ifndef BUILD_FINAL         
            if (event.mEventClass == cParticleSystemEventClassUpdateStats)
            {
               BParticleSystemManager::BStats* pPayload = static_cast<BParticleSystemManager::BStats*> (event.mpPayload);
               ASSERT_MAIN_THREAD
               mSimStats = *pPayload;
            }
#endif            
            break;
         }
   }

   if (event.mEventClass == cParticleSystemEventClassCreateThreadLocalRandom)
   {
      BASSERT(mpRandom == NULL);

      mpRandom = new Random;
      mpRandom->setSeed(GetTickCount());
   }
   else if (event.mEventClass == cParticleSystemEventClassDeleteThreadLocalRandom)
   {
      BASSERT(mpRandom != NULL);
      if (mpRandom)
      {
         delete mpRandom;
         mpRandom = NULL;
      }
   }
   
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEffectDefinition* BParticleSystemManager::reloadData(int index)
{
   ASSERT_RENDER_THREAD
   if (mData[index] == NULL)
      return NULL;

   BString filename = mData[index]->mName;
   ALIGNED_DELETE(mData[index], gParticleHeap);
   
   //-- Add a blank to the list.  We will load the file after its
   //-- already in the list, so we don't have to deal with a deep copy.
   BParticleEffectDefinition* pData = ALIGNED_NEW(BParticleEffectDefinition, gParticleHeap);
   pData->load(filename);
   mData[index] = pData;
   return pData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleSystemManager::reloadEffect(const BString path)
{
#ifndef BUILD_FINAL
   ASSERT_RENDER_THREAD

#if 0
   //test code
   static bool bDumpHashmap = false;
   if (bDumpHashmap)
   {
      dumpHashmap();
      bDumpHashmap = false;
   }
#endif

   //-- throttle reload events for the same file
   int tick = GetTickCount();
   int tickThreshold = 500;
   if (tick - mLastReloadTick < tickThreshold)
      if (path.compare(mLastReloadPath) == 0)
         return true;

   mLastReloadPath = path;
   mLastReloadTick = tick;

   int dataIndex = findDataIndex(path);
   if (dataIndex == -1)
      return false;
   
   BParticleEffectDefinition* pData = reloadData(dataIndex);
   if (!pData)
      return false;

   int effectCount = mEffectList.getSize();
   int newEmitterCount = pData->mEmitters.size();
   BMatrix matrix;   
   BMatrix lastMatrix;
   BMatrix matrix2;
   BMatrix lastMatrix2;
   DWORD   tintColor;
   BParticleEffect* pPFX = NULL;

   for (int i = 0; i < effectCount;  i++)
   {
      pPFX = mEffectList[i];
      if (!pPFX)
         continue;

      if (pPFX->mDataIndex == dataIndex)
      {
         uint count = pPFX->mEmitters.getSize();         
         if (count > 0)
         {
            matrix = pPFX->mEmitters[0]->getTransform();
            lastMatrix = pPFX->mEmitters[0]->getLastUpdateTransform();
            matrix2 = pPFX->mEmitters[0]->getTransform2();
            lastMatrix2 = pPFX->mEmitters[0]->getLastUpdateTransform2();
            tintColor = pPFX->mEmitters[0]->getTintColor();
         }
         else
         {
            matrix = pPFX->mTransform;
            lastMatrix = pPFX->mTransform;            
            matrix2 = pPFX->mSecondaryTransform;
            lastMatrix2 = pPFX->mSecondaryTransform;
            tintColor = pPFX->mTintColor;
         }

         for (uint j = 0; j < count; ++j)
         {
            releaseEmitter(pPFX->mEmitters[j], true);      
         }

         pPFX->mEmitters.resize(newEmitterCount);
         for(int k=0; k<newEmitterCount; k++)
         {
            // rg [8/26/06] - HACK
            if (mUsedInstanceList.getSize() >= cMaxPermittedEmitters)
            {
               BASSERT(0);
               break;
            }
               
            BParticleEmitterDefinition* pEmitterData = pData->mEmitters[k];
            BDEBUG_ASSERT(pEmitterData!=NULL);

            BParticleEmitter* pEmitter = createEmitter();
            BDEBUG_ASSERT(pEmitter!=NULL);

            pEmitter->init(this, pEmitterData, matrix, tintColor, 0.0f);
            pEmitter->setTransform(matrix);
            pEmitter->setSecondaryTransform(matrix2);
            pEmitter->setParentEffectIndex(i);

            //-- add it to our effect
            pPFX->mEmitters[k]= pEmitter;
         }      
      }
   }
#endif

   return true;
}

#ifndef BUILD_FINAL
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::getSimStats(BStats& stats)
{
   ASSERT_MAIN_THREAD;
   stats = mSimStats;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::emitterUpdatePhase1Callback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(BParticleSystemUpdatePhase1);

   BParticleSystemManager::BThreadUpdateData* pWorkEntry = static_cast<BParticleSystemManager::BThreadUpdateData*>(privateData0);
   BDEBUG_ASSERT(pWorkEntry!= NULL);

   pWorkEntry->mpEmitter->updatePhase1(mPermitEmission);

   if (lastWorkEntryInBucket)
      mRemainingWorkBuckets.decrement();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::emitterUpdatePhase2Callback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(BParticleSystemUpdatePhase2);

   BParticleSystemManager::BThreadUpdateData* pWorkEntry = static_cast<BParticleSystemManager::BThreadUpdateData*>(privateData0);
   BDEBUG_ASSERT(pWorkEntry!= NULL);

   pWorkEntry->mpEmitter->updatePhase2(pWorkEntry->mpVB, pWorkEntry->mpBufferVB, pWorkEntry->mStartOffsetVB, pWorkEntry->mpIB, pWorkEntry->mpBufferIB, pWorkEntry->mStartOffsetIB);

   if (lastWorkEntryInBucket)
      mRemainingWorkBuckets.decrement();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleSystemManager::dumpHashmap() const
{
   trace("PFX Hashmap");
   int index = -1;
   BString temp;
   for (BParticleEffectDataHashMap::const_iterator i = mDataHashmap.begin(); i != mDataHashmap.end(); ++i)
   {
      index = i->second;
      temp = i->first;      
      if (!temp.isEmpty())
         trace(" [%d] = %s", index, temp.getPtr());
   }      
}