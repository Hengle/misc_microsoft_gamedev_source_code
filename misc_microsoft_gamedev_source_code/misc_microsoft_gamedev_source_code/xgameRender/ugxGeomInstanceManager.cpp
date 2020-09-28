//==============================================================================
//
// File: ugxGeomInstanceManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "ugxGeomInstanceManager.h"
#include "ugxGeomRender.h"
#include "sceneLightManager.h"
#include "visibleLightManager.h"
#include "localShadowManager.h"
#include "ugxGeomManager.h"
#include "XColorUtils.h"
#include "math\VMXUtils.h"
#include "waterManager.h"
#include "ugxGeomRenderPackedBone.h"
#include "tiledAA.h"
#include "DCBManager.h"
#include "ugxGeomUberEffectManager.h"
#include "packedTextureManager.h"

static const uint cRenderAllInstancesLimit = 128;

BUGXGeomInstanceManager gUGXGeomInstanceManager;

//==============================================================================
// BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::operator<
// This method should compare by all attributes.
//==============================================================================
bool BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::operator< (const BUGXGeomInstanceManager::BInstanceSortableRenderAttributes& rhs) const
{
   const BInstanceSortableRenderAttributes& lhs = *this;
#define COMPARE(v) if (lhs.##v < rhs.##v) return true; else if (lhs.##v > rhs.v##) return false; 
   COMPARE(mFarLayer);
   COMPARE(mNearLayer);
   COMPARE(mModelIndex)
   COMPARE(mBatchIndex);
   COMPARE(mTileFlags);
   COMPARE(mTintColor);   
   COMPARE(mPlayerColorIndex);
   COMPARE(mMultiframeTextureIndex);
   COMPARE(mNumPixelLights);
   COMPARE(mGlobalLighting);
   COMPARE(mLocalLighting);
   COMPARE(mDirShadows);
   COMPARE(mLocalShadows);
   COMPARE(mLocalReflection);
   COMPARE(mSampleBlackmap);
   COMPARE(mEmissiveIntensity);
   COMPARE(mHighlightIntensity);
#undef COMPARE            

   const BOOL leftHasExtended  = (lhs.mpExtendedAttributes != NULL);
   const BOOL rightHasExtended = (rhs.mpExtendedAttributes != NULL);
   
   if (leftHasExtended | rightHasExtended)
   {
      if (leftHasExtended < rightHasExtended)
         return true;
      else if (leftHasExtended > rightHasExtended)
         return false;
      else 
         return (*lhs.mpExtendedAttributes) < (*rhs.mpExtendedAttributes);
   }         
   
   return false;
}

//==============================================================================
// BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::compareVisual
// This method should compare by only those attributes that are relevant during the visual pass.
//==============================================================================
bool BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::compareVisual(const BUGXGeomInstanceManager::BInstanceSortableRenderAttributes& lhs, const BUGXGeomInstanceManager::BInstanceSortableRenderAttributes& rhs) 
{
#define COMPARE(v) if (lhs.##v != rhs.##v) return false; 
   COMPARE(mModelIndex)
   COMPARE(mBatchIndex);
   COMPARE(mTileFlags);
   COMPARE(mTintColor);   
   COMPARE(mPlayerColorIndex);
   COMPARE(mMultiframeTextureIndex);
   COMPARE(mNumPixelLights);
   COMPARE(mGlobalLighting);
   COMPARE(mLocalLighting);
   COMPARE(mDirShadows);
   COMPARE(mLocalShadows);
   COMPARE(mLocalReflection);
   COMPARE(mSampleBlackmap);
   COMPARE(mEmissiveIntensity);
   COMPARE(mHighlightIntensity);
#undef COMPARE        

   const BOOL leftHasExtended  = (lhs.mpExtendedAttributes != NULL);
   const BOOL rightHasExtended = (rhs.mpExtendedAttributes != NULL);

   if (leftHasExtended != rightHasExtended)
      return false;
   else if (leftHasExtended)
      return lhs.mpExtendedAttributes->compareVisual(*rhs.mpExtendedAttributes);
       
   return true;
}

//==============================================================================
// BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::compareShadowGen
// This method should compare by only those attributes that are relevant during the shadow pass.
//==============================================================================
bool BUGXGeomInstanceManager::BInstanceSortableRenderAttributes::compareShadowGen(const BUGXGeomInstanceManager::BInstanceSortableRenderAttributes& lhs, const BUGXGeomInstanceManager::BInstanceSortableRenderAttributes& rhs) 
{
#define COMPARE(v) if (lhs.##v != rhs.##v) return false; 
   COMPARE(mModelIndex)
   COMPARE(mTintColor);   
   COMPARE(mMultiframeTextureIndex);
   COMPARE(mBatchIndex);
#undef COMPARE            

   const BOOL leftHasExtended  = (lhs.mpExtendedAttributes != NULL);
   const BOOL rightHasExtended = (rhs.mpExtendedAttributes != NULL);
   
   if (leftHasExtended != rightHasExtended)
      return false;
   else if (leftHasExtended)
      return lhs.mpExtendedAttributes->compareShadowGen(*rhs.mpExtendedAttributes);
      
   return true;
}

//==============================================================================
// BUGXGeomInstanceManager::BUGXGeomInstanceManager
//==============================================================================
BUGXGeomInstanceManager::BUGXGeomInstanceManager() :
   mNextBatchIndex(0),
   mpGroupVolumeCuller(NULL),
   mSceneLayerFlags(0),
   mInitialized(false),
   mDCBRenderingActive(false),
   mpCommandBuffer(NULL),
   mSavedFillMode(0),
   mSavedColorWriteEnable(0),
   mSavedHalfPixelOffset(false),
   mTotalDCBDrawCalls(0)
{
   Utils::ClearObj(mInstanceData);
   Utils::ClearObj(mSavedRenderTargetSurf);
   Utils::ClearObj(mSavedDepthStencilSurf);
      
   BCOMPILETIMEASSERT((sizeof(BUGXGeomRenderInstanceAttributes) % 16) == 0);
}

//==============================================================================
// BUGXGeomInstanceManager::~BUGXGeomInstanceManager
//==============================================================================
BUGXGeomInstanceManager::~BUGXGeomInstanceManager()
{

}

//==============================================================================
// BUGXGeomInstanceManager::init
//==============================================================================
void BUGXGeomInstanceManager::init(void)
{
   BDEBUG_ASSERT(!mInitialized);
   
   mInitialized = true;
   
   mNextBatchIndex = 0;
   
   mSortedInstanceIndices.reserve(cMaxExpectedInstances);
   mOriginalToSortedInstanceIndices.reserve(cMaxExpectedInstances);
   mCulledOpaqueInstanceIndices.reserve(cMaxExpectedInstances);
   mCulledAdditiveInstanceIndices.reserve(cMaxExpectedInstances);
   mCulledOverInstanceIndices.reserve(cMaxExpectedInstances);
   mCulledOverallAlphaInstanceIndices.reserve(cMaxExpectedInstances);
   mLocalLightInstanceIndices.reserve(cMaxExpectedInstances);
   mCulledFarLayerInstanceIndices.reserve(4);
   mCulledNearLayerInstanceIndices.reserve(1);
   mPerInstanceDataArray.resize(cRenderAllInstancesLimit);
   mTempIndices.reserve(cMaxExpectedInstances);
   
   commandListenerInit();
   
   mGameTime = 0.0f;
   mpGroupVolumeCuller = NULL;
   
   mMinPlayerColorIndex = 0;
   mMaxPlayerColorIndex = 0;
   Utils::ClearObj(mPlayerColorIndexUsed);
   mNumObscurableInstances = 0;
}

//==============================================================================
// BUGXGeomInstanceManager::reset
//==============================================================================
void BUGXGeomInstanceManager::reset()
{
   BDEBUG_ASSERT(mInitialized);

   gRenderThread.blockUntilGPUIdle();

   mSortedInstanceIndices.resize(0);
   mOriginalToSortedInstanceIndices.resize(0);
   mCulledOpaqueInstanceIndices.resize(0);
   mCulledAdditiveInstanceIndices.resize(0);
   mCulledOverInstanceIndices.resize(0);
   mCulledOverallAlphaInstanceIndices.resize(0);
   mLocalLightInstanceIndices.resize(0);
   mCulledFarLayerInstanceIndices.resize(0);
   mCulledNearLayerInstanceIndices.resize(0);
   mPerInstanceDataArray.resize(cRenderAllInstancesLimit);
   mTempIndices.resize(0);
   mUniqueModels.setMaxEntries(0);

   mInstanceSortableAttributes.resize(0);
   mInstanceLightState.resize(0);
   Utils::ClearObj(mInstanceData);

   mNextBatchIndex = 0;

   commandListenerDeinit();

   mGameTime = 0.0f;
   mpGroupVolumeCuller = NULL;
   mMinPlayerColorIndex = 0;
   mMaxPlayerColorIndex = 0;
   Utils::ClearObj(mPlayerColorIndexUsed);
   mNumObscurableInstances = 0;

   commandListenerInit();
}

//==============================================================================
// BUGXGeomInstanceManager::deinit
//==============================================================================
void BUGXGeomInstanceManager::deinit(void)
{
   BDEBUG_ASSERT(mInitialized);
   
   gRenderThread.blockUntilGPUIdle();
   
   mpGroupVolumeCuller = NULL;
   
   mSortedInstanceIndices.clear();
   mOriginalToSortedInstanceIndices.clear();
   mCulledOpaqueInstanceIndices.clear();
   mCulledAdditiveInstanceIndices.clear();
   mCulledOverInstanceIndices.clear();
   mCulledOverallAlphaInstanceIndices.clear();
   mLocalLightInstanceIndices.clear();
   mCulledFarLayerInstanceIndices.clear();
   mCulledNearLayerInstanceIndices.clear();
   mPerInstanceDataArray.clear();
   mTempIndices.clear();
   mUniqueModels.setMaxEntries(0);

   mInstanceSortableAttributes.clear();
   mInstanceLightState.clear();
   Utils::ClearObj(mInstanceData);

   commandListenerDeinit();
   
   mInitialized = false;
}

//==============================================================================
// BUGXGeomInstanceManager::simSetInstances
//==============================================================================
void BUGXGeomInstanceManager::simSetInstances(
   uint numInstances, const BUGXGeomRenderInstanceAttributes* pCPUFrameStorageInstanceAttributes, 
   void* pGPUFrameStorageBones, uint GPUFrameStorageSize)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BInstanceData* pData = (BInstanceData*)gRenderThread.submitCommandBegin(*this, cRenderCommandSetInstances, sizeof(BInstanceData));
   
   pData->mNumInstances                         = numInstances;
   pData->mpCPUFrameStorageInstanceAttributes   = pCPUFrameStorageInstanceAttributes;
   pData->mpGPUFrameStorageBones                = pGPUFrameStorageBones;
   pData->mGPUFrameStorageSize                  = GPUFrameStorageSize;
   
   gRenderThread.submitCommandEnd(sizeof(BInstanceData));
}           

//==============================================================================
// BUGXGeomInstanceManager::initDeviceData
//==============================================================================
void BUGXGeomInstanceManager::initDeviceData(void)
{
   mInstanceSortableAttributes.reserve(cMaxExpectedInstances);
   mInstanceLightState.reserve(cMaxExpectedInstances);
}

//==============================================================================
// BUGXGeomInstanceManager::frameBegin
//==============================================================================
void BUGXGeomInstanceManager::frameBegin(void)
{
   mpGroupVolumeCuller = NULL;
}

//==============================================================================
// BUGXGeomInstanceManager::renderSetInstances
//==============================================================================
void BUGXGeomInstanceManager::renderSetInstances(const BRenderCommandHeader& header, const uchar* pData)
{
   BDEBUG_ASSERT(header.mLen == sizeof(BInstanceData));
   mInstanceData = *reinterpret_cast<const BInstanceData*>(pData);
}

//==============================================================================
// BUGXGeomInstanceManager::processCommand
//==============================================================================
void BUGXGeomInstanceManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRenderCommandSetInstances:
      {
         renderSetInstances(header, pData);
         break;
      }
   }
}

//==============================================================================
// BUGXGeomInstanceManager::frameEnd
//==============================================================================
void BUGXGeomInstanceManager::frameEnd(void)
{
   mpGroupVolumeCuller = NULL;
}

//==============================================================================
// BUGXGeomInstanceManager::deinitDeviceData
//==============================================================================
void BUGXGeomInstanceManager::deinitDeviceData(void)
{
   if (mpCommandBuffer)
   {
      gDCBManager.release(mpCommandBuffer);
      mpCommandBuffer = NULL;
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderUpdate
//==============================================================================
void BUGXGeomInstanceManager::renderUpdate(double gameTime)
{
   mGameTime = gameTime;
}

//==============================================================================
// BUGXGeomInstanceManager::renderCreateSortableAttributes
//==============================================================================
void BUGXGeomInstanceManager::renderCreateSortableAttributes(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderCreateSortableAttributes);
   
   mInstanceLightState.resize(mInstanceData.mNumInstances);
   mInstanceSortableAttributes.resize(mInstanceData.mNumInstances);

   if (!mInstanceData.mNumInstances)
      return;
      
   uint curModelIndex = UINT_MAX;
   eUGXGeomRenderFlags curModelRenderFlags = cRFNoFlags;
//-- FIXING PREFIX BUG ID 6650
   const BUGXGeomRender* pUGXGeomRender = NULL;
//--

   const bool dirShadowsEnabled = gRenderSceneLightManager.getDirLight(gRenderSceneLightManager.getCurLightCategory()).mShadows;

   Utils::BPrefetchState instancePrefetchState                    = Utils::BeginPrefetch(mInstanceData.mpCPUFrameStorageInstanceAttributes, mInstanceData.mpCPUFrameStorageInstanceAttributes + mInstanceData.mNumInstances, 3);
   Utils::BPrefetchState instanceSortableAttributesPrefetchState  = Utils::BeginPrefetch(mInstanceSortableAttributes.getPtr(), mInstanceSortableAttributes.end(), 3);
   Utils::BPrefetchState instanceLightStatePrefetchState          = Utils::BeginPrefetch(mInstanceLightState.getPtr(), mInstanceLightState.end(), 3);

   uint curModelLayerFlags = 0;
   uint sceneLayerFlags = 0;
   bool largeModel = false;
   
   mMinPlayerColorIndex = UINT_MAX;
   mMaxPlayerColorIndex = 0;
   Utils::ClearObj(mPlayerColorIndexUsed);
   mNumObscurableInstances = 0;
   
   mUniqueModels.clear();
         
   for (uint instanceIndex = 0; instanceIndex < mInstanceData.mNumInstances; instanceIndex++)
   {
      const BUGXGeomRenderInstanceAttributes& instanceAttributes = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex];
      const BVisualRenderAttributes& visualAttributes = instanceAttributes.mVisualAttributes;

      const XMVECTOR boundsMin = visualAttributes.getBoundsMin();
      const XMVECTOR boundsMax = visualAttributes.getBoundsMax();
      const BSceneLightManager::BGridRect gridRect = gRenderSceneLightManager.getGridRect(boundsMin, boundsMax);

      BInstanceSortableRenderAttributes* pInstanceSortableAttributes = &mInstanceSortableAttributes[instanceIndex];            
      BInstanceLightState* pInstanceLightState = &mInstanceLightState[instanceIndex];

      instancePrefetchState                     = Utils::UpdatePrefetch(instancePrefetchState, &instanceAttributes, mInstanceData.mpCPUFrameStorageInstanceAttributes + mInstanceData.mNumInstances, 3);
      instanceSortableAttributesPrefetchState   = Utils::UpdatePrefetch(instanceSortableAttributesPrefetchState, pInstanceSortableAttributes, mInstanceSortableAttributes.end(), 3);
      instanceLightStatePrefetchState           = Utils::UpdatePrefetch(instanceLightStatePrefetchState, pInstanceLightState, mInstanceLightState.end(), 3);

      const uint modelIndex = gEventDispatcher.getHandlePrivateData(instanceAttributes.mUGXGeomHandle);
      
      BDEBUG_ASSERT(modelIndex <= 0xFFFF);
      
      BModelHashMap::InsertResult insertResult(mUniqueModels.insert(modelIndex | (visualAttributes.mMultiframeTextureIndex << 16U), visualAttributes.mProjectedArea));
      if (!insertResult.second)
      {
         BModelHashMap::iterator it = insertResult.first;
         it->second = Math::Max(it->second, visualAttributes.mProjectedArea);
      }
      
      if (modelIndex != curModelIndex)
      {
         curModelIndex           = modelIndex;
         pUGXGeomRender          = gUGXGeomManager.getGeomRenderByHandle(instanceAttributes.mUGXGeomHandle);
         if (pUGXGeomRender)
         {
            curModelRenderFlags  = pUGXGeomRender->getRenderFlags();  
            curModelLayerFlags   = pUGXGeomRender->getLayerFlags();
            
            largeModel           = (curModelRenderFlags & cRFLargeModel) != 0;
            
            sceneLayerFlags |= curModelRenderFlags;
         }
      }
                        
      const bool shadowCaster    = visualAttributes.mShadowCaster     && ((curModelRenderFlags & cRFShadowCaster) != 0);
      const bool shadowReceiver  = visualAttributes.mShadowReceiver   && ((curModelRenderFlags & cRFShadowReceiver) != 0);
      const bool globalLighting  = visualAttributes.mGlobalLighting   && ((curModelRenderFlags & cRFGlobalLighting) != 0);
      const bool localLighting   = visualAttributes.mLocalLighting    && ((curModelRenderFlags & cRFLocalLighting) != 0);
      
      uint tileFlags = 0;
      uint tileBitMask = 1;
      for (uint tileIndex = 0; tileIndex < gTiledAAManager.getNumTiles(); tileIndex++, tileBitMask <<= 1)
      {  
         const BVolumeCuller& volumeCuller = gTiledAAManager.getTileVolumeCuller(tileIndex);
         
         if (volumeCuller.isAABBVisibleBounds(boundsMin, boundsMax))
            tileFlags |= tileBitMask;
      }
           
      pInstanceSortableAttributes->mTintColor            = visualAttributes.mTintColor;
      pInstanceSortableAttributes->mHighlightIntensity   = visualAttributes.mHighlightIntensity;
      pInstanceSortableAttributes->mEmissiveIntensity    = visualAttributes.mEmissiveIntensity;
      pInstanceSortableAttributes->mGlobalLighting       = globalLighting;
      pInstanceSortableAttributes->mLocalLighting        = localLighting;
      pInstanceSortableAttributes->mDirShadows           = dirShadowsEnabled && shadowReceiver;
      pInstanceSortableAttributes->mLocalShadows         = false;
      pInstanceSortableAttributes->mFarLayer             = visualAttributes.mFarLayer;
      pInstanceSortableAttributes->mNearLayer            = visualAttributes.mNearLayer;
      pInstanceSortableAttributes->mpExtendedAttributes  = visualAttributes.mpExtendedAttributes;
      pInstanceSortableAttributes->mLayerFlags           = static_cast<uchar>(curModelLayerFlags);
      pInstanceSortableAttributes->mShadowCaster         = shadowCaster;
      pInstanceSortableAttributes->mShadowReceiver       = shadowReceiver;
      pInstanceSortableAttributes->mLocalReflection      = ((curModelRenderFlags & cRFLocalReflection) != 0);
      pInstanceSortableAttributes->mSampleBlackmap       = visualAttributes.mSampleBlackmap;
      pInstanceSortableAttributes->mTileFlags            = static_cast<uchar>(tileFlags);      
      pInstanceSortableAttributes->mMultiframeTextureIndex = visualAttributes.mMultiframeTextureIndex;
      
      uint playerColorIndex = 0;
      if (visualAttributes.mObscurable)
      {
         mNumObscurableInstances++;
         
         playerColorIndex = Math::Min<uint>(visualAttributes.mPlayerColorIndex, cMaxPlayerColorIndex);
               
         mMinPlayerColorIndex = Math::Min(mMinPlayerColorIndex, playerColorIndex);
         mMaxPlayerColorIndex = Math::Max(mMaxPlayerColorIndex, playerColorIndex);
         mPlayerColorIndexUsed[playerColorIndex] = true;
      }
      
      pInstanceSortableAttributes->mPlayerColorIndex     = static_cast<uchar>(playerColorIndex);
                  
      uint batchIndex = 0xFFFFFFFF;

      // Disable instancing if the model is in the far or near layers, of if it has a non-default mesh mask.
      if ((visualAttributes.mFarLayer) || (visualAttributes.mNearLayer) || (instanceAttributes.mMeshMask.areAnyNotSet()))
      {
         batchIndex = mNextBatchIndex;
         mNextBatchIndex++;
         if (mNextBatchIndex == 0xFFFF)
            mNextBatchIndex = 0;
      }

      pInstanceSortableAttributes->mModelIndex = static_cast<ushort>(curModelIndex);
      pInstanceSortableAttributes->mBatchIndex = static_cast<ushort>(batchIndex);

      pInstanceSortableAttributes->mLocalShadows = false;
      pInstanceSortableAttributes->mDirShadows = shadowReceiver && dirShadowsEnabled;

      pInstanceSortableAttributes->mNumPixelLights = 0;
            
      if ((localLighting) || (shadowReceiver) || (shadowCaster))
      {
         BSceneLightManager::BActiveLightIndexArray lights;

         gVisibleLightManager.findLights(lights, gridRect, boundsMin, boundsMax, true, true);

         if ((shadowCaster) || (shadowReceiver))
         {
            for (uint i = 0; i < lights.size(); i++)
            {
               const uint visibleLightIndex = lights[i];
               const uint activeLightIndex = gVisibleLightManager.getVisibleLight(visibleLightIndex);

               const bool lightCastsShadows = gRenderSceneLightManager.getActiveLightShadows(activeLightIndex);
               
               if (lightCastsShadows)
               {
                  if (shadowReceiver) 
                     pInstanceSortableAttributes->mLocalShadows = true;

                  if (shadowCaster) 
                     gVisibleLightManager.addToShadowedLightObjectList(visibleLightIndex, instanceIndex);
               }            
            }
         }         

         if ((localLighting) && (!largeModel))
         {
            const uint numPixelLights = Math::Min<uint>(BInstanceLightState::cMaxPixelLights, lights.getSize());
                        
            memcpy(pInstanceLightState->mPixelLights, lights.getPtr(), numPixelLights * sizeof(short));
            
            pInstanceSortableAttributes->mNumPixelLights = static_cast<uchar>(numPixelLights);
         }         
      }         
   }
   
   mSceneLayerFlags = sceneLayerFlags;
}

//==============================================================================
// BUGXGeomInstanceManager::instanceCompareFunc
//==============================================================================
bool BUGXGeomInstanceManager::instanceCompareFunc(uint i, uint j)
{
   const BInstanceSortableRenderAttributes& lhs = gUGXGeomInstanceManager.mInstanceSortableAttributes[i];
   const BInstanceSortableRenderAttributes& rhs = gUGXGeomInstanceManager.mInstanceSortableAttributes[j];
   return lhs < rhs;
}

//==============================================================================
// BUGXGeomInstanceManager::renderSort
//==============================================================================
void BUGXGeomInstanceManager::renderSort(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderSort);
   
   const uint totalInstances = mInstanceSortableAttributes.size();

   mSortedInstanceIndices.resize(totalInstances);
   mOriginalToSortedInstanceIndices.resize(totalInstances);

   if (!totalInstances)
      return;

   for (uint i = 0; i < totalInstances; i++)
      mSortedInstanceIndices[i] = static_cast<ushort>(i);

   std::sort(mSortedInstanceIndices.begin(), mSortedInstanceIndices.end(), instanceCompareFunc);

   for (uint i = 0; i < totalInstances; i++)
      mOriginalToSortedInstanceIndices[mSortedInstanceIndices[i]] = (ushort)i;
}

//==============================================================================
// BUGXGeomInstanceManager::renderUpdateLocalShadowStatus
//==============================================================================
void BUGXGeomInstanceManager::renderUpdateLocalShadowStatus(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderUpdateShadowStatus);

   for (uint instanceIndex = 0; instanceIndex < mInstanceSortableAttributes.size(); instanceIndex++)
   {
      BInstanceSortableRenderAttributes& attributes = mInstanceSortableAttributes[instanceIndex];
      if (!attributes.mLocalShadows)
         continue;

//-- FIXING PREFIX BUG ID 6651
      const BInstanceLightState& lightState = mInstanceLightState[instanceIndex];
//--
      
      bool localShadows = false;
      for (uint lightIndex = 0; lightIndex < attributes.mNumPixelLights; lightIndex++)
      {
         const uint visibleLightIndex = lightState.mPixelLights[lightIndex];
         
         if (gVisibleLightManager.getVisibleLightShadows(visibleLightIndex))
         {
            localShadows = true;
            break;
         }
      }                  

      if (!localShadows)
         attributes.mLocalShadows = localShadows;
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderUpdateLocalReflectionStatus
//==============================================================================
void BUGXGeomInstanceManager::renderUpdateLocalReflectionStatus(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderUpdateLocalReflectionStatus);

   gWaterManager.enableReflectionBufferUpdate(false);
   if (mInstanceSortableAttributes.size() <= 0)
      return;

   XMVECTOR minBounds = XMVectorSet(10000,100000,10000,1);
   XMVECTOR maxBounds = XMVectorSet(-10000,-100000,-10000,1);

   uint bestReflectorIndex = INT_MAX;
   float bestDistSqr = FLT_MAX;
   for (uint instanceIndex = 0; instanceIndex < mInstanceSortableAttributes.size(); instanceIndex++)
   {
      const BUGXGeomRenderInstanceAttributes& ugxRenderAttrib =   mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex];
      const BVisualRenderAttributes& instanceAttr =               mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;
      BInstanceSortableRenderAttributes& attributes =             mInstanceSortableAttributes[instanceIndex];
      if (!attributes.mLocalReflection)
         continue;

      // If instance will be culled out by the scene volume culler, then we don't locally reflect
      if (!gRenderDraw.getWorkerSceneVolumeCuller().isAABBVisibleBounds(instanceAttr.getBoundsMin(), instanceAttr.getBoundsMax()))
      {
         attributes.mLocalReflection = false;
         continue;
      }

      //if we're a large object, walk all our local subsections to determine if any
      // of them are visible.
      {
         SCOPEDSAMPLE(_largeModelCheck);
         BUGXGeomRender* pModel = gUGXGeomManager.getGeomRenderByIndex(attributes.mModelIndex);
         if (!pModel)
            continue;
       
         if(pModel->getRenderFlags() & cRFLargeModel)
         {  
            //CLM [11.10.08] pass in the instance position matrix here.
            const bool isSectionVisible = pModel->isLargeObjectSectionVisible(ugxRenderAttrib.mWorldMatrix,gRenderDraw.getWorkerSceneVolumeCuller());

            if(!isSectionVisible)
            {
               attributes.mLocalReflection = false;
               continue;
            }
         }
      }


      // If instance may reflect, determine if it's the "best" reflector
      XMVECTOR center = (instanceAttr.getBoundsMax() + instanceAttr.getBoundsMin()) * 0.5f;
      float distSqr = XMVector3LengthSq(center - gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPos()).x;
      if (distSqr < bestDistSqr)
      {
         bestDistSqr = distSqr;
         bestReflectorIndex = instanceIndex;
         minBounds = instanceAttr.getBoundsMin();
         maxBounds = instanceAttr.getBoundsMax();
      }
   }

   // No reflectors? - return
   if (bestReflectorIndex == INT_MAX)
      return;

   // Enable water reflection
   gWaterManager.enableReflectionBufferUpdate(true);

   // Set reflection plane
   XMVECTOR reflectionPlane = getLocalReflectionPlane(bestReflectorIndex);
   gWaterManager.setReflectionPlane(reflectionPlane);


   
   // Do another pass turn off reflection for all reflectors that are not coplanar with the
   // best reflector
   for (uint instanceIndex = 0; instanceIndex < mInstanceSortableAttributes.size(); instanceIndex++)
   {
      // Don't check non-reflecting
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;
      BInstanceSortableRenderAttributes& tempAttributes = mInstanceSortableAttributes[instanceIndex];
      if (!tempAttributes.mLocalReflection)
         continue;

      // Don't modify best reflector
      if (instanceIndex == bestReflectorIndex)
      {
         continue;
      }

      // Get reflection plane for this instance
      XMVECTOR tempReflectionPlane = getLocalReflectionPlane(instanceIndex);

      // Determine coplanarity of best reflection plane and this one
      BOOL coplanar = FALSE;
      XMVECTOR epsilon = XMVectorSet(0.01f, 0.01f, 0.01f, 0.01f);
      coplanar = XMPlaneNearEqual(reflectionPlane, tempReflectionPlane, epsilon);

      if (!coplanar)
      {
         tempAttributes.mLocalReflection = false;
         continue;
      }
      
      //build up a grouped bounding box of all the coplanar reflectors.
      //CLM - ideally, this would actually create multiple, separate frustums to cull against
      //but for now, we'll just use them all.
      XMVECTOR minB = instanceAttr.getBoundsMin();
      XMVECTOR maxB = instanceAttr.getBoundsMax();

      //this keeps everything in the XMV registers.
      if(XMVector4Less(XMVectorSplatX(minB),XMVectorSplatX(minBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 4, 1, 2, 3 );
         minBounds = XMVectorPermute(  minBounds, minB,control );
      }
      else if(XMVector4Greater(XMVectorSplatX(maxB),XMVectorSplatX(maxBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 4, 1, 2, 3 );
         maxBounds = XMVectorPermute(  maxBounds, maxB,control );
      }

      if(XMVector4Less(XMVectorSplatY(minB),XMVectorSplatY(minBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 5, 2, 3 );
         minBounds = XMVectorPermute(  minBounds, minB,control );
      }
      else if(XMVector4Greater(XMVectorSplatY(maxB),XMVectorSplatY(maxBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 5, 2, 3 );
         maxBounds = XMVectorPermute(  maxBounds, maxB,control );
      }

      if(XMVector4Less(XMVectorSplatZ(minB),XMVectorSplatZ(minBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 1, 6, 3 );
         minBounds = XMVectorPermute(  minBounds, minB,control );
      }
      else if(XMVector4Greater(XMVectorSplatZ(maxB),XMVectorSplatZ(maxBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 1, 6, 3 );
         maxBounds = XMVectorPermute(  maxBounds, maxB,control );
      }

   }

   gWaterManager.setReflectionObjMinMax(minBounds, maxBounds);
}

//==============================================================================
// BUGXGeomInstanceManager::renderUpdatePackedTextures
//==============================================================================
void BUGXGeomInstanceManager::renderUpdatePackedTextures(void)
{
   if (!gpPackedTextureManager)
      return;
      
   typedef BHashMap<BD3DTextureManager::BManagedTexture*, float, BBitHasher<BD3DTextureManager::BManagedTexture*>, BEqualTo<BD3DTextureManager::BManagedTexture*>, true, BRenderFixedHeapAllocator> BTextureHashMap;
      
   BTextureHashMap uniqueTextures;
   for (BModelHashMap::const_iterator it = mUniqueModels.begin(); it != mUniqueModels.end(); ++it)
   {
      const uint modelIndex = it->first & 0xFFFF;
      const uint multiframeTextureIndex = (it->first >> 16U);
      
//-- FIXING PREFIX BUG ID 6652
      const BUGXGeomRender* pModel = gUGXGeomManager.getGeomRenderByIndex(modelIndex);
//--
      if (!pModel)
         continue;
                     
      const float maxProjectedArea = it->second;
      
      const IUGXGeomSectionRendererArray* pSectionRendererArray = pModel->getSectionRendererArray();
      if (!pSectionRendererArray)
         continue;
         
      for (uint sectionIndex = 0; sectionIndex < pSectionRendererArray->getSize(); sectionIndex++)
      {
         const IUGXGeomSectionRenderer& section = pSectionRendererArray->getSection(sectionIndex);
         
         BD3DTextureManager::BManagedTexture* const* pTextureList;
         uint numTextures;
         section.getTextures(pTextureList, numTextures, multiframeTextureIndex);
         
         for (uint textureIndex = 0; textureIndex < numTextures; textureIndex++)
         {
            if (pTextureList[textureIndex])
            {
               BTextureHashMap::InsertResult insertResult(uniqueTextures.insert(pTextureList[textureIndex], maxProjectedArea));
               if (!insertResult.second)
               {
                  BTextureHashMap::iterator uniqueTexturesIt = insertResult.first;
                  uniqueTexturesIt->second = Math::Max(uniqueTexturesIt->second, maxProjectedArea);
               }
            }
         }
      }
   }
   
   BDynamicRenderArray<BPackedTextureManager::BTextureTouchInfo> texturesToTouch;
   texturesToTouch.reserve(uniqueTextures.getSize());
   for (BTextureHashMap::const_iterator it = uniqueTextures.begin(); it != uniqueTextures.end(); ++it)
   {
//-- FIXING PREFIX BUG ID 6653
      const BD3DTextureManager::BManagedTexture* pTex = it->first;
//--
      float maxProjectedArea = it->second;
      if (maxProjectedArea < 0.0f)
         maxProjectedArea = 99999999.0f;
      
      const BD3DTexture& d3dTex = pTex->getD3DTexture();
      IDirect3DBaseTexture9* pBaseTex = d3dTex.getBaseTexture();
      if (!pBaseTex)
         continue;
      
      if (!gpPackedTextureManager->isPackedTexture(pBaseTex))
         continue;
      
      uint textureDim = Math::Max(pTex->getWidth(), pTex->getHeight());
      uint textureArea = textureDim * textureDim;
      
      static float cProjAreaMultiplier = .7f;
      bool prefersBaseMap = (maxProjectedArea * cProjAreaMultiplier) >= textureArea;

      texturesToTouch.pushBack(BPackedTextureManager::BTextureTouchInfo(pBaseTex, (uint)maxProjectedArea, prefersBaseMap));
   }
         
   if (!texturesToTouch.isEmpty())
   {
      texturesToTouch.sort();
      gpPackedTextureManager->touch(texturesToTouch.getPtr(), texturesToTouch.getSize());
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushUpdateIntrinsics
//==============================================================================
void BUGXGeomInstanceManager::renderFlushUpdateIntrinsics(void)
{
   BDEBUG_ASSERT(!mDCBRenderingActive);
   gUGXGeomManager.getIntrinsicPool() = gEffectIntrinsicManager.getRenderEffectIntrinsicPool();
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushAll
//==============================================================================
void BUGXGeomInstanceManager::renderFlushAll(eUGXGeomPass pass, uint layerFlags, DWORD renderFlushFlags)
{
   ASSERT_THREAD(cThreadIndexRender);
            
   renderFlushInstances(pass, layerFlags, &mSortedInstanceIndices, NULL, renderFlushFlags);
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushLocalLightInstances
//==============================================================================
void BUGXGeomInstanceManager::renderFlushLocalLightInstances(eUGXGeomPass pass, uint layerFlags, uint localLightShadowPassIndex, DWORD renderFlushFlags)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderFlushLocalLightInstances);
      
   const BVisibleLightManager::BVisibleLightIndex visibleLightIndex = gLocalShadowManager.getPassVisibleLightIndex(localLightShadowPassIndex);

   const BLightObjectLinkedArray* pObjectsList = gVisibleLightManager.getShadowedLightObjectList(visibleLightIndex);
   if (!pObjectsList)
      return;

   const BLightObjectLinkedArrayManager& linkedArrayManager = gVisibleLightManager.getLightObjectLinkedArrayManager();

   mLocalLightInstanceIndices.resize(0);

   BLightObjectLinkedArrayManager::BItemIter iter = linkedArrayManager.firstItem(pObjectsList);
   while (iter != BLightObjectLinkedArrayManager::cInvalidItemIter)
   {
      const uint originalObjectIndex = linkedArrayManager.getItem(iter);
      const uint sortedObjectIndex = mOriginalToSortedInstanceIndices[originalObjectIndex];

      mLocalLightInstanceIndices.pushBack((ushort)sortedObjectIndex);

      iter = linkedArrayManager.nextItem(pObjectsList, iter);
   }

   std::sort(mLocalLightInstanceIndices.begin(), mLocalLightInstanceIndices.end());

   for (uint i = 0; i < mLocalLightInstanceIndices.size(); i++)
   {
      const uint sortedObjectIndex = mLocalLightInstanceIndices[i];

      mLocalLightInstanceIndices[i] = mSortedInstanceIndices[sortedObjectIndex];
   }  

   renderFlushInstances(pass, layerFlags, &mLocalLightInstanceIndices, &gLocalShadowManager.getPassVolumeCuller(localLightShadowPassIndex), renderFlushFlags);
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlush
//==============================================================================
void BUGXGeomInstanceManager::renderFlush(eUGXGeomPass pass, uint layerFlags, const BVolumeCuller* pVolumeCuller, bool farLayer, bool nearLayer, DWORD renderFlushFlags)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderFlush);
   
   mCulledOpaqueInstanceIndices.resize(0);

   const uint totalInstances = mSortedInstanceIndices.getSize();
   if (!totalInstances)
      return;

   for (uint i = 0; i < totalInstances; i++)
   {
      const uint instanceIndex = mSortedInstanceIndices[i];
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;
      const BInstanceSortableRenderAttributes& sortableAttr = mInstanceSortableAttributes[instanceIndex];
      
      if (0 == (sortableAttr.mLayerFlags & layerFlags))
         continue;
         
      if ((instanceAttr.mFarLayer != farLayer) || (instanceAttr.mNearLayer != nearLayer))
         continue;
                     
      if (pass >= cUGXGeomPassShadowGen)
      {
         if (!sortableAttr.mShadowCaster)
            continue;
      }
            
      if ((!pVolumeCuller) || (pVolumeCuller->isAABBVisibleBounds(instanceAttr.getBoundsMin(), instanceAttr.getBoundsMax())))
      {
         mCulledOpaqueInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
      }
   }

   renderFlushInstances(pass, layerFlags, &mCulledOpaqueInstanceIndices, pVolumeCuller, renderFlushFlags);
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushVisiblePrep
//==============================================================================
void BUGXGeomInstanceManager::renderFlushVisiblePrep(const BVolumeCuller* pVolumeCuller)
{
   //ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderFlushVisiblePrep);
   
   mpGroupVolumeCuller = pVolumeCuller;
   
   mCulledOpaqueInstanceIndices.resize(0);
   mCulledAdditiveInstanceIndices.resize(0);
   mCulledOverInstanceIndices.resize(0);
   mCulledOverallAlphaInstanceIndices.resize(0);
   mCulledFarLayerInstanceIndices.resize(0);
   mCulledNearLayerInstanceIndices.resize(0);

   const uint totalInstances = mSortedInstanceIndices.getSize();
   if (!totalInstances)
      return;

   for (uint i = 0; i < totalInstances; i++)
   {
      const uint instanceIndex = mSortedInstanceIndices[i];
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;
      const BInstanceSortableRenderAttributes& sortableAttr = mInstanceSortableAttributes[instanceIndex];

      if (instanceAttr.mFarLayer)
         mCulledFarLayerInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
      else if (instanceAttr.mNearLayer)
         mCulledNearLayerInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
      else if ((!pVolumeCuller) || (pVolumeCuller->isAABBVisibleBounds(instanceAttr.getBoundsMin(), instanceAttr.getBoundsMax())))
      {
         if (instanceAttr.getAlphaBlend())
            mCulledOverallAlphaInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
         else 
         {
            if (sortableAttr.mLayerFlags & cUGXGeomLayerOpaque)
               mCulledOpaqueInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
               
            if (sortableAttr.mLayerFlags & cUGXGeomLayerAdditive)
               mCulledAdditiveInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
               
            if (sortableAttr.mLayerFlags & cUGXGeomLayerOver)
               mCulledOverInstanceIndices.pushBack(static_cast<ushort>(instanceIndex));
         }
      }
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushVisibleGroup
//==============================================================================
void BUGXGeomInstanceManager::renderFlushVisibleGroup(
   eGroupIndex group, 
   eUGXGeomPass pass, 
   uint layerFlags, 
   eObscurableFilter obscurableFilter, 
   eBelowDecalsFilter belowDecalsFilter,
   DWORD renderFlushFlags)
{
   //ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderFlushVisibleGroup);
   
   const BDynamicRenderArray<ushort>* pGroupSortedInstances[] = 
   { 
      &mCulledOpaqueInstanceIndices, 
      &mCulledOverallAlphaInstanceIndices, 
      &mCulledAdditiveInstanceIndices, 
      &mCulledOverInstanceIndices, 
      &mCulledFarLayerInstanceIndices,
      &mCulledNearLayerInstanceIndices
   };
   
   const uint cNumGroupSortedInstances = sizeof(pGroupSortedInstances) / sizeof(pGroupSortedInstances[0]);
   cNumGroupSortedInstances;
   
   BDEBUG_ASSERT((group >= 0) && (group < cNumGroupSortedInstances));
   if ((cOFAll == obscurableFilter) && (cBDAll == belowDecalsFilter))
      renderFlushInstances(pass, layerFlags, pGroupSortedInstances[group], mpGroupVolumeCuller, renderFlushFlags);
   else
   {
      mTempIndices.resize(0);
      
      const BDynamicRenderArray<ushort>* pSortedInstances = pGroupSortedInstances[group];
      
      for (uint i = 0; i < pSortedInstances->getSize(); i++)
      {
         const uint instanceIndex = (*pSortedInstances)[i];
         const BUGXGeomRenderInstanceAttributes& instanceAttributes = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex];
         const BVisualRenderAttributes& instanceVisualAttributes = instanceAttributes.mVisualAttributes;  
         
         if (obscurableFilter != cOFAll)
         {
            if (instanceVisualAttributes.mObscurable != (cOFObscurable == obscurableFilter))
               continue;
         }               
          
         if (belowDecalsFilter != cBDAll)
         {
            if (instanceVisualAttributes.mAppearsBelowDecals != (cBDBelowDecals == belowDecalsFilter))
               continue;
         }               
         
         mTempIndices.pushBack(static_cast<ushort>(instanceIndex));
      }
      
      renderFlushInstances(pass, layerFlags, &mTempIndices, mpGroupVolumeCuller, renderFlushFlags); //cRFFSetStencilRefToTeamColor);
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBBeginFork
//==============================================================================
void BUGXGeomInstanceManager::renderFlushDCBBeginFork(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDepthStencilSurf, bool halfPixelOffset)
{
   BDEBUG_ASSERT(!mDCBRenderingActive);
   mDCBRenderingActive = true;
   
   gUGXGeomManager.getIntrinsicPool().getTable() = gEffectIntrinsicManager.getRenderIntrinsicTable();
      
   gpUGXD3DDev = gUGXGeomManager.getCommandBufferDevice();
   gpUGXD3DDev->ReleaseThreadOwnership();
   
   gUGXGeomUberEffectManager.changeDevice(gpUGXD3DDev);
   gUGXGeomManager.getIntrinsicPool().setDevice(gpUGXD3DDev);
      
   BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &mSavedFillMode);
   BD3D::mpDev->GetRenderState(D3DRS_COLORWRITEENABLE, &mSavedColorWriteEnable);
   mSavedHalfPixelOffset = halfPixelOffset;
   
   memcpy(&mSavedRenderTargetSurf, pRenderTarget, sizeof(IDirect3DSurface9));
   memcpy(&mSavedDepthStencilSurf, pDepthStencilSurf, sizeof(IDirect3DSurface9));

   BDEBUG_ASSERT(!mpCommandBuffer);
   mpCommandBuffer = gDCBManager.acquire();
   BVERIFY(mpCommandBuffer);
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBForkCallbackBegin
//==============================================================================
IDirect3DDevice9* BUGXGeomInstanceManager::renderFlushDCBForkCallbackBegin(void)
{
   BDEBUG_ASSERT(mDCBRenderingActive);
   
   D3DTAGCOLLECTION inheritTags = { 0 };

#if 0   
   uint firstReg, numRegs;
   gEffectIntrinsicManager.getSharedFloatIntrinsicRegisterRange(firstReg, numRegs);
   numRegs = (numRegs + 3) & ~3;
   D3DTagCollection_SetVertexShaderConstantFTag(&inheritTags, firstReg, numRegs);
   D3DTagCollection_SetPixelShaderConstantFTag(&inheritTags, firstReg, numRegs);
#endif   
   D3DTagCollection_SetAll(&inheritTags);
      
   gpUGXD3DDev->AcquireThreadOwnership();
   
   gpUGXD3DDev->SetRenderTarget(0, &mSavedRenderTargetSurf);
   gpUGXD3DDev->SetDepthStencilSurface(&mSavedDepthStencilSurf);

   D3DVIEWPORT9 tileViewport;
   gTiledAAManager.getTileViewport(0, tileViewport);
   gpUGXD3DDev->SetViewport(&tileViewport);
   
   // MPB [12/5/2008] - Added the RECORD_ALL_STATE flag because there can be state set after
   // the final draw call and this needs to be recorded and played back to avoid leaving
   // things in a bad way.
   DWORD cbFlags = D3DBEGINCB_OVERWRITE_INHERITED_STATE | D3DBEGINCB_RECORD_ALL_SET_STATE;
   gpUGXD3DDev->BeginCommandBuffer(mpCommandBuffer, cbFlags, &inheritTags, NULL, 0, 0);
   
   gRenderDraw.workerSetDefaultRenderStates(gpUGXD3DDev);
   gRenderDraw.workerSetDefaultSamplerStates(gpUGXD3DDev);

   gUGXGeomManager.getIntrinsicPool().updateAllBool();
         
   gpUGXD3DDev->SetRenderState(D3DRS_FILLMODE, mSavedFillMode);
   gpUGXD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, mSavedColorWriteEnable);
   if (mSavedHalfPixelOffset)
      gpUGXD3DDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
   
   BUGXGeomRender::clearTotalDraws();
   
   return gpUGXD3DDev;
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBForkCallbackEnd
//==============================================================================
void BUGXGeomInstanceManager::renderFlushDCBForkCallbackEnd(void)
{
   BDEBUG_ASSERT(mDCBRenderingActive);
   
   gpUGXD3DDev->SetVertexShader(NULL);
   gpUGXD3DDev->SetPixelShader(NULL);
   gpUGXD3DDev->SetIndices(NULL);
   
   for (uint i = 0; i < D3DMAXSTREAMS; i++)
      gpUGXD3DDev->SetStreamSource(i, NULL, 0, 0);

   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      gpUGXD3DDev->SetTexture(i, NULL);
      
   gpUGXD3DDev->SetRenderTarget(0, NULL);
   gpUGXD3DDev->SetDepthStencilSurface(NULL);
      
   gpUGXD3DDev->EndCommandBuffer();
         
   gpUGXD3DDev->ReleaseThreadOwnership();
   
   mTotalDCBDrawCalls = BUGXGeomRender::getTotalDraws();
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBEndFork
//==============================================================================
void BUGXGeomInstanceManager::renderFlushDCBEndFork(void)
{
   BDEBUG_ASSERT(mDCBRenderingActive);
   mDCBRenderingActive = false;
   
   gUGXGeomManager.getCommandBufferDevice()->AcquireThreadOwnership();
            
   gpUGXD3DDev = BD3D::mpDev;
   
   gUGXGeomUberEffectManager.changeDevice(BD3D::mpDev);
   gUGXGeomManager.getIntrinsicPool().setDevice(BD3D::mpDev);
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBRun
//==============================================================================
void BUGXGeomInstanceManager::renderFlushDCBRun(uint tileIndex)
{
   if (!renderFlushGetDCB())
      return;
   
   BD3D::mpDev->SetShaderGPRAllocation(0, 26, 102);

   BD3D::mpDev->SetIndices(NULL);

   for (uint i = 0; i < D3DMAXSTREAMS; i++)
      BD3D::mpDev->SetStreamSource(i, NULL, 0, 0);

   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      BD3D::mpDev->SetTexture(i, NULL);

   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetPixelShader(NULL);

   gDCBManager.run(renderFlushGetDCB(), 1U << tileIndex);

   BD3D::mpDev->SetShaderGPRAllocation(0, 64, 64);

   BD3D::mpDev->InsertFence();
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlushDCBEnd
//==============================================================================
void BUGXGeomInstanceManager::renderFlushDCBEnd(void)
{
   if (mpCommandBuffer)
   {
      gDCBManager.release(mpCommandBuffer);
      mpCommandBuffer = NULL;
   }
}

//==============================================================================
// BUGXGeomInstanceManager::renderFlush
//==============================================================================
void BUGXGeomInstanceManager::renderFlushInstances(
   eUGXGeomPass pass, 
   uint layerFlags, 
   const BDynamicRenderArray<ushort>* pSortedInstances, 
   const BVolumeCuller* pVolumeCuller,
   DWORD renderFlushFlags)
{
   SCOPEDSAMPLE(BUGXGeomInstanceManager_flushInstances);
         
#ifndef BUILD_FINAL
   mStats.mTotalFlushes++;
#endif

   uint totalInstances = pSortedInstances->getSize();
   if (!totalInstances)
      return;
      
#ifndef BUILD_FINAL
   mStats.mTotalNonEmptyFlushes++;
   mStats.mTotalInstances += pSortedInstances->getSize();
#endif
         
   BUGXGeomRender::globalRenderBegin(mGameTime, mInstanceData.mpGPUFrameStorageBones, mInstanceData.mGPUFrameStorageSize, pass, gRenderSceneLightManager.getGlobalEnvMap());

   uint startIndex = 0;
      
   while (startIndex < totalInstances)
   {
      uint endIndex = startIndex + 1;
      
      const BInstanceSortableRenderAttributes& s = mInstanceSortableAttributes[(*pSortedInstances)[startIndex]];
   
      while (endIndex < totalInstances)
      {
         const BInstanceSortableRenderAttributes& e = mInstanceSortableAttributes[(*pSortedInstances)[endIndex]];

         // Shadow gen comparison func. is also good for the distortion pass.
         if ((pass == cUGXGeomPassDistortion) || (pass >= cUGXGeomPassShadowGen))
         {
            if (!BInstanceSortableRenderAttributes::compareShadowGen(s, e))
               break;
         }
         else
         {
            if (!BInstanceSortableRenderAttributes::compareVisual(s, e))
               break;
         }

         endIndex++;
         if ((endIndex - startIndex) >= cRenderAllInstancesLimit)
            break;
      }

#ifndef BUILD_FINAL
      mStats.mTotalModelBatches++;
#endif      

      renderAllInstances(pass, layerFlags, *pSortedInstances, startIndex, endIndex, pVolumeCuller, renderFlushFlags);

      startIndex = endIndex;
   }

   if (renderFlushFlags & cRFFSetStencilRefToTeamColor)
      gpUGXGeomRSFilter->setRenderState(D3DRS_STENCILREF, 0);

   if (renderFlushFlags & cRFFSetCommandBufferRunPredication)
      gpUGXGeomRSFilter->setCommandBufferPredication(0, 0);
      
   BUGXGeomRender::globalRenderEnd(pass);
}

//==============================================================================
// BUGXGeomInstanceManager::renderAllInstances
// This method renders a set of instances of the same model type, and with 
// equal "breakable" instance attributes.
//==============================================================================
void BUGXGeomInstanceManager::renderAllInstances(
   eUGXGeomPass pass, 
   uint layerFlags, 
   const BDynamicRenderArray<ushort>& indices, 
   uint startIndex, 
   uint endIndex, 
   const BVolumeCuller* pVolumeCuller,
   DWORD renderFlushFlags)
{
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderAllInstances);
         
   const uint firstInstanceIndex = indices[startIndex];
   const BUGXGeomRenderInstanceAttributes& firstInstanceAttributes = mInstanceData.mpCPUFrameStorageInstanceAttributes[firstInstanceIndex];
   //const BVisualRenderAttributes& firstInstanceVisualAttributes = firstInstanceAttributes.mVisualAttributes;
   const BInstanceSortableRenderAttributes& sortableAttributes = mInstanceSortableAttributes[firstInstanceIndex];
               
   BUGXGeomRender* pModel = gUGXGeomManager.getGeomRenderByIndex(sortableAttributes.mModelIndex);
   if (!pModel)
      return;
   
   // This shouldn't happen
   if (pModel->getStatus() != cUGXGeomStatusReady)
      return;
      
   if (renderFlushFlags & cRFFSetStencilRefToTeamColor)
   {
      // Stencil is used to implement obscured units.
      gpUGXGeomRSFilter->setRenderState(D3DRS_STENCILREF, sortableAttributes.mPlayerColorIndex + 1);
   }
            
   BUGXGeomRenderCommonInstanceData instanceCommonData;
   instanceCommonData.mpVolumeCuller         = pVolumeCuller;
   instanceCommonData.mPass                  = pass;
   instanceCommonData.mLayerFlags            = static_cast<uchar>(layerFlags);
   instanceCommonData.mTileFlags             = sortableAttributes.mTileFlags;
   instanceCommonData.mpExtendedAttributes   = sortableAttributes.mpExtendedAttributes;
   instanceCommonData.mTintColor             = sortableAttributes.mTintColor;
   instanceCommonData.mEmissiveIntensity     = sortableAttributes.mEmissiveIntensity;
   instanceCommonData.mHighlightIntensity    = sortableAttributes.mHighlightIntensity;
   instanceCommonData.mSampleBlackmap        = sortableAttributes.mSampleBlackmap;
   instanceCommonData.mMultiframeTextureIndex = sortableAttributes.mMultiframeTextureIndex;

   
   if (renderFlushFlags & cRFFSetCommandBufferRunPredication)
   {
      gpUGXGeomRSFilter->setCommandBufferPredication(0, sortableAttributes.mTileFlags);  
      instanceCommonData.mSetCommandBufferRunPredication = true;
   }
      
   uint numPixelLights = 0;
         
   if (sortableAttributes.mNearLayer)
   {
      instanceCommonData.mGlobalLighting     = sortableAttributes.mGlobalLighting;
   }
   else if (pass < cUGXGeomPassFirstUnlightable)
   {
      numPixelLights = sortableAttributes.mNumPixelLights;
                  
      instanceCommonData.mNumPixelLights       = static_cast<uchar>(numPixelLights);
      instanceCommonData.mGlobalLighting       = sortableAttributes.mGlobalLighting;
      instanceCommonData.mLocalLighting        = sortableAttributes.mLocalLighting;
      instanceCommonData.mLocalLightShadows    = sortableAttributes.mLocalShadows;
      instanceCommonData.mDirLightShadows      = sortableAttributes.mDirShadows;
      instanceCommonData.mLocalReflection      = sortableAttributes.mLocalReflection;
   }


   //CLM [07.23.08]
   if(pass == cUGXGeomPassMainReflect)
   {
      instanceCommonData.mNumPixelLights        = 0;
      instanceCommonData.mLocalLighting         = false;
      instanceCommonData.mSampleBlackmap        = false;
      instanceCommonData.mLocalLightShadows     = false;
      instanceCommonData.mLocalReflection       = false;
   }
   
   pModel->renderBegin(&instanceCommonData);
   
   const uint totalInstances = endIndex - startIndex;
      
   BDEBUG_ASSERT(totalInstances <= mPerInstanceDataArray.getSize());
   BUGXGeomRenderPerInstanceData* pPerInstanceData = mPerInstanceDataArray.getPtr();
      
   // Fill in the per-instance data.
   for (uint instanceIter = 0; instanceIter < totalInstances; instanceIter++)
   {
      const instanceIndex = indices[startIndex + instanceIter];
      const BUGXGeomRenderInstanceAttributes& attributes = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex];
      const BVisualRenderAttributes& visualAttributes = attributes.mVisualAttributes;
      const BInstanceLightState& lightState = mInstanceLightState[instanceIndex];
                  
      XMVECTOR srgbColor = BXColorUtils::D3DColorToFractional(visualAttributes.mPixelXFormColor);

      // Cheap sRGB approximation.
      srgbColor = srgbColor * srgbColor;

      XMVECTOR boneVertexIndex = XMConvertVectorUIntToFloat(XMVectorSplatX(XMLoadScalar(&attributes.mBoneVertexIndex)), 0);
      XMVECTOR vec = __vrlimi(srgbColor, boneVertexIndex, VRLIMI_CONST(0, 0, 0, 1), 0);

      XMStoreFloat4A(reinterpret_cast<XMFLOAT4A*>(&pPerInstanceData[instanceIter].mColorBoneMatrices), vec);

      if (numPixelLights)
         memcpy(&pPerInstanceData[instanceIter].mVisiblePixelLightIndices, lightState.mPixelLights, numPixelLights * sizeof(short));
   }

#ifdef BUILD_DEBUG
   if (firstInstanceAttributes.mMeshMask.areAnyNotSet())
   {
      BDEBUG_ASSERT(1 == totalInstances);
   }
#endif   

   pModel->render(firstInstanceAttributes.mMeshMask, mPerInstanceDataArray.getPtr(), totalInstances);

   pModel->renderEnd();
}

//==============================================================================
// BUGXGeomInstanceManager::calculateViewExtents
//==============================================================================
uint BUGXGeomInstanceManager::renderCalculateViewExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, const BVec3& worldMin, const BVec3& worldMax, AABB& viewBounds)
{
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderCalculateViewExtents);
   
   uint totalNodes = 0;
   
   const AABB worldExtents(worldMin, worldMax);
   
   for (uint instanceIndex = 0; instanceIndex < mInstanceData.mNumInstances; instanceIndex++)
   {
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;
      
      if (!instanceAttr.mShadowCaster)
         continue;
               
      XMVECTOR boundsMin = instanceAttr.getBoundsMin();
      XMVECTOR boundsMax = instanceAttr.getBoundsMax();
      
      if (!volumeCuller.isAABBVisibleBounds(boundsMin, boundsMax))
         continue;
                                      
      AABB worldBounds;
      worldBounds[0].set(boundsMin.x, boundsMin.y, boundsMin.z);
      worldBounds[1].set(boundsMax.x, boundsMax.y, boundsMax.z);
      
      // FIXME - Try to filter out bogus bounding boxes/units.
      if ((boundsMax.y < -250.0f) || (boundsMin.y > 250.0f) || 
         (worldBounds.dimension(0) > 2000.0f) || (worldBounds.dimension(1) > 2000.0f) || (worldBounds.dimension(2) > 2000.0f))
         continue;
         
      // Ignore bounds outside of what's supposed to be the valid world.
      if (!worldExtents.overlaps(worldBounds))
         continue;

      AABB nodeViewBounds(worldBounds.transform3(*reinterpret_cast<const BMatrix44*>(&worldToView)));

      viewBounds.expand(nodeViewBounds);
         
      totalNodes++;   
   }
   
   return totalNodes;
}

//==============================================================================
// BUGXGeomInstanceManager::calculateProjZExtents
//==============================================================================
uint BUGXGeomInstanceManager::renderCalculateProjZExtents(XMMATRIX worldToView, XMMATRIX viewToProj, const BVolumeCuller& volumeCuller, const BVec3& worldMin, const BVec3& worldMax, BInterval& zExtent)
{
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderCalculateProjZExtents);
   
   uint totalNodes = 0;
   
   const XMMATRIX worldToProj = worldToView * viewToProj;
   
   const AABB worldExtents(worldMin, worldMax);

   for (uint instanceIndex = 0; instanceIndex < mInstanceData.mNumInstances; instanceIndex++)
   {
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;

      if (!instanceAttr.mShadowCaster)
         continue;

      XMVECTOR boundsMin = instanceAttr.getBoundsMin();
      XMVECTOR boundsMax = instanceAttr.getBoundsMax();

      if (!volumeCuller.isAABBVisibleBounds(boundsMin, boundsMax))
         continue;

      AABB worldBounds;
      worldBounds[0].set(boundsMin.x, boundsMin.y, boundsMin.z);
      worldBounds[1].set(boundsMax.x, boundsMax.y, boundsMax.z);

      // FIXME - Try to filter out bogus bounding boxes/units.
      if ((boundsMax.y < -250.0f) || (boundsMin.y > 250.0f) || 
          (worldBounds.dimension(0) > 2000.0f) || (worldBounds.dimension(1) > 2000.0f) || (worldBounds.dimension(2) > 2000.0f))
         continue;
         
      // Ignore bounds outside of what's supposed to be the valid world.
      if (!worldExtents.overlaps(worldBounds))
         continue;
      
      for (uint i = 0; i < 8; i++)
      {
         BVec4 c(worldBounds.corner(i));
         c[3] = 1.0f;
         BVec4 p(c * ((BMatrix44&)worldToProj));
         if (p[3] != 0.0f)
         {
            if (p[3] < 0.0f)
               zExtent.expand(0.0f);
            else
               zExtent.expand(p[2] / p[3]);
         }
      }

      totalNodes++;   
   }

   return totalNodes;
}

//==============================================================================
// BUGXGeomInstanceManager::renderSceneIterate
//==============================================================================
uint BUGXGeomInstanceManager::renderSceneIterate(const BSceneIterateParams& params, BSceneIteratorCallbackFunc pIteratorFunc, void* pData)
{
   SCOPEDSAMPLE(BUGXGeomInstanceManager_renderSceneIterate);
   
   uint totalNodes = 0;
   
   for (uint instanceIndex = 0; instanceIndex < mInstanceData.mNumInstances; instanceIndex++)
   {
      const BVisualRenderAttributes& instanceAttr = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex].mVisualAttributes;

      if (params.mShadowCastersOnly)
      {
         if (!instanceAttr.mShadowCaster)
            continue;
      }            

      XMVECTOR boundsMin = instanceAttr.getBoundsMin();
      XMVECTOR boundsMax = instanceAttr.getBoundsMax();

      if (params.mpVolumeCuller)
      {
         if (!params.mpVolumeCuller->isAABBVisibleBounds(boundsMin, boundsMax))
            continue;
      }            

      if (!pIteratorFunc(params, instanceIndex, boundsMin, boundsMax, pData))
         break;

      totalNodes++;   
   }

   return totalNodes;
}

//==============================================================================
// BUGXGeomInstanceManager::getLocalReflectionPlane
//==============================================================================
XMVECTOR BUGXGeomInstanceManager::getLocalReflectionPlane(uint32 instanceIndex) const
{
   // Get plane of reflection for best reflector
   const BUGXGeomRenderInstanceAttributes& instanceAttributes = mInstanceData.mpCPUFrameStorageInstanceAttributes[instanceIndex];
   BUGXGeomRender* pUGXGeomRender = gUGXGeomManager.getGeomRenderByHandle(instanceAttributes.mUGXGeomHandle);
   uint16 reflectBoneIndex = 0;
   if (pUGXGeomRender->getStatus() == cUGXGeomStatusReady)
   {
      reflectBoneIndex = instanceAttributes.mVisualAttributes.mReflectBoneIndex;
      BASSERT(reflectBoneIndex < pUGXGeomRender->getNumBones());
   }

   // If bone other than the root bone is specified, get bone data from GPUFrameStorage
   XMVECTOR planePoint, planeNormal;
   if (reflectBoneIndex > 0)
   {
      // Calculate boneToModel matrix (invert modelToBone)
      const BUGXGeom::BNativeCachedDataType::BoneType& bone = pUGXGeomRender->getGeomData().getCachedData()->bone(reflectBoneIndex);
      XMVECTOR det;
      XMMATRIX boneToModelMtx = &(bone.modelToBone().getMatrix().mRow[0].element[0]);
      boneToModelMtx = XMMatrixInverse(&det, boneToModelMtx);

      // Get gpu frame storage pointer to skeleton data, then offset to reflection bone
//-- FIXING PREFIX BUG ID 6656
      const uchar* pGPUFrameStorageBones = 
//--
         ((uchar*)GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(mInstanceData.mpGPUFrameStorageBones)) + 
         (instanceAttributes.mBoneVertexIndex << cUGXGeomRenderBytesPerPackedBoneLog2);
      pGPUFrameStorageBones += (cUGXGeomRenderBytesPerPackedBone * (reflectBoneIndex + 2));  // skip misc data + modelToWorld at beginning

      // Unpack reflection bone matrix and multiply be boneToModel (inverse bind pose) to get world space bone matrix
      XMMATRIX reflectBoneMtx = BUGXGeomRenderPackedBone::unpackMatrix(pGPUFrameStorageBones);
      reflectBoneMtx = XMMatrixMultiply(boneToModelMtx, reflectBoneMtx);

      planePoint = reflectBoneMtx.r[3];
      planeNormal = reflectBoneMtx.r[2];                    // Use the forward vector for reflect bones
   }
   // Use world matrix if root bone is specified
   else
   {
      planePoint = instanceAttributes.mWorldMatrix.r[3];
      planeNormal = instanceAttributes.mWorldMatrix.r[1];   // Use the up vector for root bone
   }

   XMVECTOR reflectionPlane = XMPlaneFromPointNormal(planePoint, planeNormal);
   return reflectionPlane;
}
