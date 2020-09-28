//============================================================================
// grannyInstanceRenderer.cpp
//  
// Copyright (c) 2006 Ensemble Studios
//
// rg [2/14/06] - Should improve the sort, it doesn't need to be a full sort,
// a hacky sort by hash keys would be fine.
//============================================================================

// Includes
#include "common.h"
#include "grannyInstanceRenderer.h"
#include "grannyInstance.h"
#include "grannyManager.h"
#include "grannyModel.h"
#include "render.h"
#include "renderHelperThread.h"
#include "containers\staticArray.h"
#include "threading\workDistributor.h"
#include "ugxGeomRenderPackedBone.h"
#include "ugxGeomInstanceManager.h"
#include "xcolorUtils.h"
#include "debugprimitives.h"
#include "math\VMXUtils.h"
#include "config.h"
#include "econfigenum.h"

// Constants
const uint cNumExtraBonesPerInstance = 2;
const bool cMultithreadedAnimSampling = true;
   
// Globals
BGrannyInstanceRenderer gGrannyInstanceRenderer;

BCountDownEvent BGrannyInstanceRenderer::mRemainingBuckets;

__declspec(thread) BGrannyInstanceRenderer::BGrannyPoses BGrannyInstanceRenderer::mThreadGrannyPose;
BGrannyInstanceRenderer::BGrannyPoses BGrannyInstanceRenderer::mGrannyPoses[cThreadIndexMax];

extern bool gEnableSubUpdating;

//============================================================================
// BGrannyInstanceRenderer::BGrannyInstanceRenderer
//============================================================================
BGrannyInstanceRenderer::BGrannyInstanceRenderer() :
   mInitialized(false),
   mpGPUFrameStorage(NULL),
   mTotalGPUFrameStorageNeeded(0),
   mWaitForSampling(false)
{
#ifndef BUILD_FINAL
   clearStats();
#endif
}

//============================================================================
// BGrannyInstanceRenderer::~BGrannyInstanceRenderer
//============================================================================
BGrannyInstanceRenderer::~BGrannyInstanceRenderer()
{
}

//============================================================================
// BGrannyInstanceRenderer::initGrannyPosesCallback
//============================================================================
bool BGrannyInstanceRenderer::initGrannyPosesCallback(uint privateData, BEventPayload* pPayload)
{
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   
   mThreadGrannyPose.mpLocalPose = mGrannyPoses[threadIndex].mpLocalPose;
   mThreadGrannyPose.mpWorldPose = mGrannyPoses[threadIndex].mpWorldPose;
   
   return false;
}

//============================================================================
// BGrannyInstanceRenderer::init
//============================================================================
void BGrannyInstanceRenderer::init(void)
{
   if (mInitialized)
      return;
   
   mInitialized = true;
   mWaitForSampling = false;
   
   mpGPUFrameStorage = NULL;
   mTotalGPUFrameStorageNeeded = 0;
   mRemainingBuckets.clear();
      
   mQueuedInstances.reserve(BUGXGeomInstanceManager::cMaxExpectedInstances);
   
   mSampleAnimWorkEntries.reserve(512);
   
   // Granny's new pose functions are not thread safe, so allocate them on the sim thread and distibute them to all registered worker threads so they can initialize their TLS entries.
   for (uint i = 0; i < cThreadIndexMax; i++)
   {
      if (!gEventDispatcher.getThreadId(i))
         continue;
         
      mGrannyPoses[i].mpLocalPose = GrannyNewLocalPose(BGrannyManager::cMaxBones);
      mGrannyPoses[i].mpWorldPose = GrannyNewWorldPose(BGrannyManager::cMaxBones);
      
      gEventDispatcher.submitCallback(static_cast<BThreadIndex>(i), initGrannyPosesCallback, 0, NULL, true);
   }
   
#ifndef BUILD_FINAL
   clearStats();
#endif   
}

//============================================================================
// BGrannyInstanceRenderer::deinit
//============================================================================
void BGrannyInstanceRenderer::deinit(void)
{
   if (mInitialized)
      return;

   mInitialized = false;
   
   for (uint i = 0; i < cThreadIndexMax; i++)
   {
      GrannyFreeLocalPose(mGrannyPoses[i].mpLocalPose);
      mGrannyPoses[i].mpLocalPose = NULL;
      
      GrannyFreeWorldPose(mGrannyPoses[i].mpWorldPose);
      mGrannyPoses[i].mpWorldPose = NULL;
   }
   
   mQueuedInstances.clear();
   mSampleAnimWorkEntries.clear();
}

//============================================================================
// BGrannyInstanceRenderer::begin
//============================================================================
void BGrannyInstanceRenderer::begin(void)
{
   gGrannyManager.lockInstances();
   
   reset();
   
   if (!mQueuedInstances.empty())
      mQueuedInstancesPrefetch = Utils::BeginPrefetch(mQueuedInstances.getPtr(), 3);
}

//============================================================================
// BGrannyInstanceRenderer::finish
//============================================================================
void BGrannyInstanceRenderer::finish(void)
{
   gGrannyManager.unlockInstances();
}

//============================================================================
// BGrannyInstanceRenderer::queue
//============================================================================
void BGrannyInstanceRenderer::queue(
   BGrannyInstance* pInstance, 
   const BVisualRenderAttributes* RESTRICT pRenderAttributes, 
   const XMMATRIX* RESTRICT pWorldMatrix)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mQueuedInstances.size() == BUGXGeomInstanceManager::cMaxExpectedInstances)
      return;
   
   const uint modelIndex = pInstance->mModelIndex;
            
   BUGXGeomRenderInstanceAttributes* RESTRICT pDstAttributes = mQueuedInstances.pushBackNoConstruction(1);
   
   mQueuedInstancesPrefetch = Utils::UpdatePrefetch(mQueuedInstancesPrefetch, pDstAttributes, 3);      
   
   XMStoreFloat4x4A16( (XMFLOAT4X4A16*)&pDstAttributes->mWorldMatrix, XMLoadFloat4x4A16( (const XMFLOAT4X4A16*)pWorldMatrix ) );
   
   pDstAttributes->mVisualAttributes = *pRenderAttributes;
   
   if (pInstance->getMeshRenderMaskAllSet())
      pDstAttributes->mMeshMask.setAll();
   else
   {
      const BBitArray& bitArray = pInstance->getMeshRenderMask();
      if (!bitArray.getBits())
         pDstAttributes->mMeshMask.setAll();
      else
         pDstAttributes->mMeshMask.set(bitArray.getBits(), bitArray.getNumberBytes());
   }

#if 0
   // HACK HACK - For testing
   static uint64 bitMask = UINT64_MAX;
   pDstAttributes->mMeshMask = bitMask;
#endif
   
   pDstAttributes->mMainThread.mpModelInstance = pInstance->getModelInstance();
   pDstAttributes->mMainThread.mpGrannyInstance = pInstance;
   pDstAttributes->mMainThread.mHasIKNodes = (pInstance->getNumIKNodes() > 0);
   pDstAttributes->mMainThread.mModelIndex = (ushort)modelIndex;
   //pDstAttributes->mMainThread.mInterpolation = interpolation;
   
   if (modelIndex >= mModelHist.getSize())
      mModelHist.resize(modelIndex + 1);
   mModelHist[modelIndex]++;
}

//============================================================================
// BGrannyInstanceRenderer::reset
//============================================================================
void BGrannyInstanceRenderer::reset(void)
{
   mQueuedInstances.resize(0);
   
   mModelHist.setAll((ushort)0);
}

#ifndef BUILD_FINAL
//============================================================================
// BGrannyInstanceRenderer::debugRenderBoundingBoxes
//============================================================================
void BGrannyInstanceRenderer::debugRenderBoundingBoxes(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   for (uint i = 0; i < mQueuedInstances.size(); i++)
   {
      const BUGXGeomRenderInstanceAttributes& attr = mQueuedInstances[i];

      XMVECTOR min = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&attr.mVisualAttributes.mBounds[0]));
      XMVECTOR max = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&attr.mVisualAttributes.mBounds[3]));

      gpDebugPrimitives->addDebugBox(min, max, D3DCOLOR_ARGB(255,255,255,255), BDebugPrimitives::cCategoryNone, -1);
   }
}
#endif

//============================================================================
// BGrannyInstanceRenderer::sortInstances
//============================================================================
void BGrannyInstanceRenderer::sortInstances(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mQueuedInstances.empty())
      return;
      
   SCOPEDSAMPLE(BGrannyInstanceRendererSort);
   
   Utils::BPrefetchState prefetchState = Utils::BeginPrefetch(mQueuedInstances.getPtr(), &mQueuedInstances.back(), 3);
      
   BStaticArray<uint, 512> modelOfs;
   modelOfs.resize(mModelHist.getSize());
   
   uint totalInstances = 0;
   for (uint i = 0; i < mModelHist.getSize(); i++)
   {
      modelOfs[i] = totalInstances;
      totalInstances += mModelHist[i];
   }
   
   mSortedIndices.resize(mQueuedInstances.getSize());
      
   for (uint i = 0; i < mQueuedInstances.getSize(); i++)
   {
      prefetchState = Utils::UpdatePrefetch(prefetchState, &mQueuedInstances[i], 3);
      
      const uint modelIndex = mQueuedInstances[i].mMainThread.mModelIndex;
      
      uint ofs = modelOfs[modelIndex];
      modelOfs[modelIndex] = ofs + 1;
      
      mSortedIndices[ofs] = static_cast<ushort>(i);
   }
}

//============================================================================
// BGrannyInstanceRenderer::allocFrameStorage
//============================================================================
uchar* BGrannyInstanceRenderer::allocFrameStorage(uint& totalGPUFrameStorageNeeded, uint& numGrannyAnimSamples)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   SCOPEDSAMPLE(BGrannyInstanceRendererAllocFrameStorage);
   
   numGrannyAnimSamples = 0;
         
   uint totalBones = 0;
   for (uint i = 0; i < mModelHist.getSize(); i++)
   {
      if (mModelHist[i])
      {
//-- FIXING PREFIX BUG ID 7688
         const BGrannyModel* pModel = gGrannyManager.getModel(i);
//--

         const uint grannyBoneCount = pModel->getNumBones();
         const uint actualBoneCount = grannyBoneCount + cNumExtraBonesPerInstance;
         
#ifndef BUILD_FINAL
         mStats.mMaxEverModelBones = Math::Max(mStats.mMaxEverModelBones, actualBoneCount);
#endif         
         
         if (grannyBoneCount)
            numGrannyAnimSamples += mModelHist[i];
         
         totalBones += actualBoneCount * mModelHist[i];
      }
   }
        
   totalGPUFrameStorageNeeded = cUGXGeomRenderBytesPerPackedBone * totalBones;

#ifndef BUILD_FINAL
   mStats.mTotalInstances = mQueuedInstances.getSize();
   mStats.mMaxEverInstances = Math::Max(mStats.mMaxEverInstances, mStats.mTotalInstances);
   mStats.mTotalGPUFrameStorage = totalGPUFrameStorageNeeded;
   mStats.mMaxEverGPUFrameStorage = Math::Max(mStats.mMaxEverGPUFrameStorage, mStats.mTotalGPUFrameStorage);
   mStats.mTotalBones = totalBones;
   mStats.mMaxEverBones = Math::Max(mStats.mMaxEverBones, mStats.mTotalBones);
#endif   

   uchar* pGPUFrameStorage = NULL;

   if (totalGPUFrameStorageNeeded)
      pGPUFrameStorage = reinterpret_cast<uchar*>(gRenderThread.allocateGPUFrameStorage(totalGPUFrameStorageNeeded, cUGXGeomRenderBytesPerPackedBone, false));
   
   return pGPUFrameStorage;
}

//============================================================================
// BGrannyInstanceRenderer::sampleAnimAndPack
//============================================================================
void BGrannyInstanceRenderer::sampleAnimAndPack(
   const BGrannyInstance* pGrannyInstance, granny_model_instance* pModelInstance, bool hasIKNodes, uint grannyBoneCount, 
   const void* pModelToWorld, granny_local_pose* pLocalPose, granny_world_pose* pWorldPose, uchar* pGPUFrameStorageNext, bool dontPack)
{
#if 0
   uint numControls = 0;
   for(granny_model_control_binding* pBinding = GrannyModelControlsBegin(pModelInstance);
      pBinding != GrannyModelControlsEnd(pModelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      numControls++;
   }      
   static uint gMaxControls;
   if (numControls > gMaxControls)
   {
      gMaxControls = numControls;
      
      trace("Max controls: %u (%s)", gMaxControls, GrannyGetSourceModel(pModelInstance)->Name);
   }
#endif   
   
   // rg [1/16/08] - Determine if any IK nodes are actually active. If not, we can use the accelerated sampling API.
   if (hasIKNodes)
   {
      hasIKNodes = false;
      
      int numIKNodes = pGrannyInstance->getNumIKNodes();
      for (int i = 0; i < numIKNodes; i++)
      {
         if (pGrannyInstance->getIKNode(i).mIsActive)
         {
            hasIKNodes = true;
            break;
         }
      }         
   }
   
   // jce [11/3/2008] -- New caching system should be placing an already posed guy into a local pose on the instance.  To be
   // safe, I'm going to check if there is one and if not, use the old method.
   // And yes, I am evilly casting away const here :(
   granny_local_pose *cachedLocalPose = (granny_local_pose*)pGrannyInstance->getRenderPrepareLocalPose();
   
   if(!cachedLocalPose)
   {
      //SCOPEDSAMPLE(GrannySampleModelAnimations);
      if (hasIKNodes)
      {
//-- FIXING PREFIX BUG ID 7685
         const granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(pModelInstance);
//--
         GrannySampleModelAnimations(pModelInstance, 0, grannyBoneCount, pLocalPose);
         GrannyBuildWorldPose(pSkeleton, 0, grannyBoneCount, pLocalPose, (const granny_real32*)pModelToWorld, pWorldPose);
      }
      else
      {
         GrannySampleModelAnimationsAccelerated(pModelInstance, grannyBoneCount, (const granny_real32*)pModelToWorld, pLocalPose, pWorldPose);
      }
   }
   else
   {
      // jce [11/3/2008] -- use the cachedLocalPose if we have one instead of the generic granny manager one.
      pLocalPose = cachedLocalPose;

      // Build the world pose.
      const granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(pModelInstance);
      GrannyBuildWorldPose(pSkeleton, 0, grannyBoneCount, cachedLocalPose, (const granny_real32*)pModelToWorld, pWorldPose);
   }

   if (hasIKNodes && !cachedLocalPose)
   {
      SCOPEDSAMPLE(GrannyProcessIKNodes);

      const granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(pModelInstance);
      int numIKNodes = pGrannyInstance->getNumIKNodes();
      for (int i = 0; i < numIKNodes; i++)
      {
         const BIKNode *pIKNode = &pGrannyInstance->getIKNode(i);
         if (pIKNode->mIsActive)
         {
            int boneIndex = BONEFROMGRANNYBONEHANDLE(pIKNode->mBoneHandle);
            BDEBUG_ASSERT((boneIndex >= 0) && (boneIndex < (long)grannyBoneCount));

            switch (pIKNode->mNodeType)
            {
               case BIKNode::cIKNodeTypeSingleBone:
               {
                  granny_transform* boneTransform = GrannyGetLocalPoseTransform(pLocalPose, boneIndex);
                  granny_transform  IKTransform;
                  GrannyMakeIdentity(&IKTransform);
                  IKTransform.Flags = GrannyHasOrientation;
                  BVector interpolatedDir = pIKNode->getInterpolatedAnchorPosAsOrientation(0.0f);  // jce [11/14/2008] -- TODO: get real time into this
                  IKTransform.Orientation[0] = interpolatedDir.x;
                  IKTransform.Orientation[1] = interpolatedDir.y;
                  IKTransform.Orientation[2] = interpolatedDir.z;
                  IKTransform.Orientation[3] = interpolatedDir.w;
                  granny_triple savedPosition;
                  savedPosition[0] = boneTransform->Position[0];
                  savedPosition[1] = boneTransform->Position[1];
                  savedPosition[2] = boneTransform->Position[2];
                  GrannyPreMultiplyBy(boneTransform, &IKTransform);
                  boneTransform->Position[0] = savedPosition[0];
                  boneTransform->Position[1] = savedPosition[1];
                  boneTransform->Position[2] = savedPosition[2];
                  break;
               }
               default:
               {
                  // jce [11/14/2008] -- Fake local space world pose since granny requires it but we're working in local space
                  GrannyBuildWorldPose(pSkeleton, 0, pSkeleton->BoneCount, pLocalPose, (const granny_real32*)NULL, pWorldPose);
                  BVector targetPos = pIKNode->getInterpolatedTargetPos(0.0f);   // jce [11/14/2008] -- TODO: get real time into this
                  GrannyIKUpdate(pIKNode->mLinkCount, boneIndex, (const granny_real32*)&targetPos, 20, pSkeleton, NULL, pLocalPose, pWorldPose);
                  break;
               }
            }
         }
      }
      
      // Build the world pose.
      GrannyBuildWorldPose(pSkeleton, 0, grannyBoneCount, pLocalPose, (const granny_real32*)pModelToWorld, pWorldPose);
   }

   if (!dontPack)
   {
      //SCOPEDSAMPLE(PackMatrices);
      
      const granny_matrix_4x4* RESTRICT pCompositeBuffer = GrannyGetWorldPoseComposite4x4Array(pWorldPose);

      uint srcBoneIndex = 0;
      for (uint boneIter4 = 0; boneIter4 < (grannyBoneCount >> 2); boneIter4++)
      {
         // If this assert fires something bad happened inside of granny related to animation processing!
         BDEBUG_ASSERT(Math::IsValidFloat(*(float*)(pCompositeBuffer + srcBoneIndex)));
         
         BUGXGeomRenderPackedBone::packMatrix4(pGPUFrameStorageNext, (float*)(pCompositeBuffer + srcBoneIndex));

         srcBoneIndex += 4;
         pGPUFrameStorageNext += 4 * cUGXGeomRenderBytesPerPackedBone;
      }

      for (uint boneIter = 0; boneIter < (grannyBoneCount & 3); boneIter++)
      {
         // If this assert fires something bad happened inside of granny related to animation processing!
         BDEBUG_ASSERT(Math::IsValidFloat(*(float*)(pCompositeBuffer + srcBoneIndex)));
         
         BUGXGeomRenderPackedBone::packMatrix(pGPUFrameStorageNext, XMLoadFloat4x4A16( (const XMFLOAT4X4A16*) &pCompositeBuffer[srcBoneIndex] ) );

         srcBoneIndex++;
         pGPUFrameStorageNext += cUGXGeomRenderBytesPerPackedBone;
      }
   }
}

//============================================================================
// BGrannyInstanceRenderer::sampleAnimCallback
//============================================================================
void BGrannyInstanceRenderer::sampleAnimCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(sampleAnimCallback);
   
   BSampleAnimWorkEntry* pWorkEntry = static_cast<BSampleAnimWorkEntry*>(privateData0);
   
   BDEBUG_ASSERT(mThreadGrannyPose.mpLocalPose && mThreadGrannyPose.mpWorldPose);
   
   uint grannyBoneCount = static_cast<uint>(privateData1 >> 32);
   uchar* pGPUFrameStorage = reinterpret_cast<uchar*>(privateData1);
   XMMATRIX modelToWorld = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&pWorkEntry->mModelToWorld));

   // rg [7/8/08] - This is not thread safe.
   //bool clockSet = (gConfig.isDefined(cConfigEnableSubUpdating)) ? pWorkEntry->mpGrannyInstance->isClockSet() : false;
   /*for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(pWorkEntry->mpModelInstance);
      pBinding != GrannyModelControlsEnd(pWorkEntry->mpModelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      GrannySetControlClockOnly(pControl, pWorkEntry->mpGrannyInstance->getInterpolatedClock());
   }*/

   const uint cMaxSaveStates = 100;
   BGrannySaveControlState saveStates[cMaxSaveStates];
   uint numSaveStates = 0;

   pWorkEntry->mpGrannyInstance->setInterpolatedClocks(pWorkEntry->mpModelInstance, saveStates, cMaxSaveStates, numSaveStates);   

   sampleAnimAndPack(pWorkEntry->mpGrannyInstance, pWorkEntry->mpModelInstance, pWorkEntry->mHasIKNodes, grannyBoneCount, &modelToWorld, mThreadGrannyPose.mpLocalPose, mThreadGrannyPose.mpWorldPose, pGPUFrameStorage);   

   pWorkEntry->mpGrannyInstance->unsetInterpolatedClocks(pWorkEntry->mpModelInstance, saveStates, numSaveStates);
   
   /*for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(pWorkEntry->mpModelInstance);
      pBinding != GrannyModelControlsEnd(pWorkEntry->mpModelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      GrannySetControlClockOnly(pControl, pWorkEntry->mpGrannyInstance->getAnimClock());
   }

   pWorkEntry->mpGrannyInstance->updateGrannySync();      */
   
   if (lastWorkEntryInBucket)
      mRemainingBuckets.decrement();
}

//============================================================================
// BGrannyInstanceRenderer::sampleAnimations
//============================================================================
bool BGrannyInstanceRenderer::sampleAnimations(uchar* pGPUFrameStorage, uint numGrannyAnimSamples)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   SCOPEDSAMPLE(BGrannyInstanceRendererSampleAnims);
   
   bool useThreadPool = false;
   
   uint cNumWorkEntriesPerBucketLog2 = 4;
   
   if (numGrannyAnimSamples <= 16)
      cNumWorkEntriesPerBucketLog2 = 1;
   else if (numGrannyAnimSamples <= 32)
      cNumWorkEntriesPerBucketLog2 = 2;
   else if (numGrannyAnimSamples <= 64)
      cNumWorkEntriesPerBucketLog2 = 3;
      
   uint cNumWorkEntriesPerBucket = 1U << cNumWorkEntriesPerBucketLog2;
   
   if ((cMultithreadedAnimSampling) && (numGrannyAnimSamples >= 8))
   {
      gWorkDistributor.flush();
      
      mSampleAnimWorkEntries.resize(numGrannyAnimSamples);

      BDEBUG_ASSERT(cNumWorkEntriesPerBucket <= gWorkDistributor.getWorkEntryBucketSize());      
      
      mRemainingBuckets.set((numGrannyAnimSamples + cNumWorkEntriesPerBucket - 1) >> cNumWorkEntriesPerBucketLog2);
                  
      useThreadPool = true;
   }
      
   uchar* RESTRICT pGPUFrameStorageNext = pGPUFrameStorage;
   uint GPUFrameStorageNextVertexIndex = 0;//(pGPUFrameStorage - (uchar*)gRenderThread.getGPUFrameStorageBase()) >> cUGXGeomRenderBytesPerPackedBoneLog2;

   granny_local_pose* RESTRICT pLocalPose = gGrannyManager.getLocalPose();
   granny_world_pose* RESTRICT pWorldPose = gGrannyManager.getWorldPose();

//-- FIXING PREFIX BUG ID 7689
   const BGrannyModel* pCurGrannyModel = NULL;
//--
   uint prevModelIndex = UINT_MAX;
   uint grannyBoneCount = 0;
   BEventReceiverHandle ugxGeomHandle = cInvalidEventReceiverHandle;
   BSampleAnimWorkEntry* pNextWorkEntry = mSampleAnimWorkEntries.getPtr();
   
   uint numBucketWorkEntries = 0;
   
   const uint cMaxSaveStates = 100;
   BGrannySaveControlState saveStates[cMaxSaveStates];
   uint numSaveStates = 0;

   for (uint updateIndexIter = 0; updateIndexIter < mQueuedInstances.getSize(); updateIndexIter++)
   {
      const instanceIndex = mSortedIndices[updateIndexIter];
      BUGXGeomRenderInstanceAttributes& instanceAttributes = mQueuedInstances[instanceIndex];

      const uint modelIndex = instanceAttributes.mMainThread.mModelIndex;
      if (modelIndex != prevModelIndex)
      {
         pCurGrannyModel =  gGrannyManager.getModel((long)modelIndex);
         grannyBoneCount = pCurGrannyModel->getNumBones();
         BDEBUG_ASSERT(grannyBoneCount);
         ugxGeomHandle = pCurGrannyModel->getUGXGeomHandle();
         prevModelIndex = modelIndex;
      }

      XMMATRIX modelToWorld = instanceAttributes.mWorldMatrix;
      
      const XMFLOAT4* pInstanceUVOffsets = reinterpret_cast<const XMFLOAT4*>(&instanceAttributes.mMainThread.mpGrannyInstance->getUVOffsets());
            
      // First matrix holds per-instance attributes
      BCOMPILETIMEASSERT(3 == BVisualRenderAttributes::cMaxUVOffsets);
      XMVECTOR uvOffsets1 = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(instanceAttributes.mVisualAttributes.mUVOffsets));
      XMVECTOR uvOffsets2 = XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(&instanceAttributes.mVisualAttributes.mUVOffsets[2]));
   
      // Add in the per-instance UV offsets
      uvOffsets1 += XMLoadFloat4(pInstanceUVOffsets);
      uvOffsets2 += XMLoadFloat2(reinterpret_cast<const XMFLOAT2*>(pInstanceUVOffsets + 1));
            
      XMStoreHalf4((XMHALF4*)(pGPUFrameStorageNext), uvOffsets1);
      XMStoreHalf4((XMHALF4*)(pGPUFrameStorageNext + 8), uvOffsets2);
      
      XMVECTOR zeroVec = XMVectorZero();
            
      // Also called the "AO tint value" in shader code
      XMVECTOR aoTint = gXMOne;
      if (instanceAttributes.mVisualAttributes.mAOTintValue < 255)
         aoTint = XMVectorReplicate(instanceAttributes.mVisualAttributes.mAOTintValue * 1.0f/255.0f);
      
      XMStoreHalf2((XMHALF2*)(pGPUFrameStorageNext+16), aoTint);
      XMStoreFloat3((XMFLOAT3*)(pGPUFrameStorageNext+20), zeroVec);

      pGPUFrameStorageNext += cUGXGeomRenderBytesPerPackedBone;

      // Second matrix is the model to world matrix
      BUGXGeomRenderPackedBone::packMatrix(pGPUFrameStorageNext, modelToWorld);
      pGPUFrameStorageNext += cUGXGeomRenderBytesPerPackedBone;

      if (grannyBoneCount)
      {
         if (useThreadPool)
         {
            XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&pNextWorkEntry->mModelToWorld), modelToWorld);
            pNextWorkEntry->mpGrannyInstance = instanceAttributes.mMainThread.mpGrannyInstance;
            pNextWorkEntry->mpModelInstance = instanceAttributes.mMainThread.mpModelInstance;
            pNextWorkEntry->mHasIKNodes = instanceAttributes.mMainThread.mHasIKNodes;
            pNextWorkEntry->mSubupdating = gEnableSubUpdating;
 //           pNextWorkEntry->mInterpolation = instanceAttributes.mMainThread.mInterpolating;
                                    
            gWorkDistributor.queue(sampleAnimCallback, pNextWorkEntry, (((uint64)grannyBoneCount) << 32U) | (uint64)((uint32)pGPUFrameStorageNext));
            
            numBucketWorkEntries++;
            if (numBucketWorkEntries == cNumWorkEntriesPerBucket)
            {
               numBucketWorkEntries = 0;
               
               gWorkDistributor.flush();
            }
            
            pNextWorkEntry++;
         }
         else
         {

            instanceAttributes.mMainThread.mpGrannyInstance->setInterpolatedClocks(instanceAttributes.mMainThread.mpModelInstance, saveStates, cMaxSaveStates, numSaveStates);
            sampleAnimAndPack(instanceAttributes.mMainThread.mpGrannyInstance, instanceAttributes.mMainThread.mpModelInstance, instanceAttributes.mMainThread.mHasIKNodes, grannyBoneCount, (const granny_real32*)&modelToWorld, pLocalPose, pWorldPose, pGPUFrameStorageNext);
            instanceAttributes.mMainThread.mpGrannyInstance->unsetInterpolatedClocks(instanceAttributes.mMainThread.mpModelInstance, saveStates, numSaveStates);
         }  
         
         pGPUFrameStorageNext += cUGXGeomRenderBytesPerPackedBone * grannyBoneCount;          
      }         

      instanceAttributes.mBoneVertexIndex = GPUFrameStorageNextVertexIndex;
      instanceAttributes.mUGXGeomHandle = ugxGeomHandle;
      
      GPUFrameStorageNextVertexIndex += grannyBoneCount + cNumExtraBonesPerInstance;
   }
         
   if (useThreadPool)
   {
      BDEBUG_ASSERT((pNextWorkEntry - mSampleAnimWorkEntries.getPtr()) <= static_cast<int>(mSampleAnimWorkEntries.getSize()));
      
      gWorkDistributor.flush();
   }
   
   return useThreadPool;
}

//============================================================================
// BGrannyInstanceRenderer::flushBegin
//============================================================================
void BGrannyInstanceRenderer::flushBegin(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   SCOPEDSAMPLE(BGrannyInstanceRendererFlushBegin);
               
   if (!mQueuedInstances.empty())
   {
      sortInstances();

      uint numGrannyAnimSamples;
      mpGPUFrameStorage = allocFrameStorage(mTotalGPUFrameStorageNeeded, numGrannyAnimSamples);
      
      // This is a hack - no models will render if GPU FS is full!
      if (!mpGPUFrameStorage)
      {
         mQueuedInstances.resize(0);
         mSortedIndices.resize(0);
         mModelHist.setAll((ushort)0);
         return;
      }
      
      mWaitForSampling = sampleAnimations(mpGPUFrameStorage, numGrannyAnimSamples);
   }
}

//============================================================================
// BGrannyInstanceRenderer::flushEnd
//============================================================================
void BGrannyInstanceRenderer::flushEnd(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   SCOPEDSAMPLE(BGrannyInstanceRendererFlushEnd);
   
   if (mQueuedInstances.empty())
   {
      gUGXGeomInstanceManager.simSetInstances(0, NULL, NULL, 0);
   }
   else
   {
      BUGXGeomRenderInstanceAttributes* pInstanceAttributes = reinterpret_cast<BUGXGeomRenderInstanceAttributes*>(
         gRenderThread.allocateFrameStorage(mQueuedInstances.getSizeInBytes(), 16) );

      Utils::FastMemCpy(pInstanceAttributes, mQueuedInstances.begin(), mQueuedInstances.getSizeInBytes());         
      
      if (mWaitForSampling)
         gWorkDistributor.waitSingle(mRemainingBuckets);
        
      if (mTotalGPUFrameStorageNeeded)
         gRenderDraw.invalidateGPUCache(mpGPUFrameStorage, mTotalGPUFrameStorageNeeded);  
         
      gUGXGeomInstanceManager.simSetInstances(
         mQueuedInstances.getSize(), 
         pInstanceAttributes, 
         mpGPUFrameStorage, 
         mTotalGPUFrameStorageNeeded);
   }
}



