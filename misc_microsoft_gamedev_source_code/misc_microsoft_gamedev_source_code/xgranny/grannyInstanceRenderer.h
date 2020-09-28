//============================================================================
// grannyInstanceRenderer.h
//  
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once

// Includes
#include "visualRenderAttributes.h"
#include "ugxGeomRenderTypes.h"
#include "volumeCuller.h"
#include "broadPhase2D.h"
#include "sceneLightManager.h"

#include <granny.h>

// Forward declarations
class BGrannyInstance;

//============================================================================
// BGrannyInstanceRenderer
//============================================================================
class BGrannyInstanceRenderer
{
   BGrannyInstanceRenderer(const BGrannyInstanceRenderer&);
   BGrannyInstanceRenderer& operator= (const BGrannyInstanceRenderer&);
   
public:
   BGrannyInstanceRenderer();
   ~BGrannyInstanceRenderer();
   
   void init(void);
   void deinit(void);
   
   void begin(void);
   
   void queue(BGrannyInstance* pInstance, const BVisualRenderAttributes* RESTRICT pRenderAttributes, const XMMATRIX* RESTRICT pWorldMatrix);
      
   void flushBegin(void);
   void flushEnd(void);
   
   void finish(void);
               
#ifndef BUILD_FINAL
   void debugRenderBoundingBoxes(void);
#endif   

#ifndef BUILD_FINAL
   struct BStats
   {
      uint mTotalInstances;
      uint mTotalBones;
      uint mTotalGPUFrameStorage;
      
      uint mMaxEverInstances;
      uint mMaxEverBones;
      uint mMaxEverGPUFrameStorage;
            
      uint mMaxEverModelBones;
   };
   
   void clearStats(void) { Utils::ClearObj(mStats); }
   BStats& getStats(void) { return mStats; }
#endif   
   
private:
   
   Utils::BPrefetchState mQueuedInstancesPrefetch;
   BDynamicArray<BUGXGeomRenderInstanceAttributes, 16> mQueuedInstances;

   BDynamicArray<ushort> mModelHist;
   BDynamicArray<ushort> mSortedIndices;
   
   uchar* mpGPUFrameStorage;
   uint mTotalGPUFrameStorageNeeded;
   
   struct BSampleAnimWorkEntry
   {
      float                   mModelToWorld[16];
      //float                   mInterpolation;
      BGrannyInstance*        mpGrannyInstance;
      granny_model_instance*  mpModelInstance;
      bool                    mHasIKNodes : 1;
      bool                    mSubupdating : 1;
   };
   
   BDynamicArray<BSampleAnimWorkEntry, 4, BDynamicArraySimHeapAllocator, BDynamicArrayNoConstructOptions> mSampleAnimWorkEntries;

#ifndef BUILD_FINAL   
   BStats mStats;
#endif   
                     
   bool mInitialized : 1;
   bool mWaitForSampling : 1;
      
   struct BGrannyPoses
   {
      granny_local_pose* mpLocalPose;
      granny_world_pose* mpWorldPose;
   };
   
   static BGrannyPoses mGrannyPoses[cThreadIndexMax];
   
   static BCountDownEvent mRemainingBuckets;
   
   static __declspec(thread) BGrannyPoses mThreadGrannyPose;
            
   void reset(void);   
      
   void sortInstances(void);
   uchar* allocFrameStorage(uint& totalGPUFrameStorageNeeded, uint& numGrannyAnimSamples);
   bool sampleAnimations(uchar* pGPUFrameStorage, uint numGrannyAnimSamples);
   
   static void sampleAnimAndPack(
      const BGrannyInstance* pGrannyInstance, granny_model_instance* pModelInstance, bool hasActiveIKNodes, uint grannyBoneCount, 
      const void* pModelToWorld, granny_local_pose* pLocalPose, granny_world_pose* pWorldPose, uchar* pGPUFrameStorageNext, bool dontPack=false);
      
   static void sampleAnimCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);

   static bool initGrannyPosesCallback(uint privateData, BEventPayload* pPayload);
};

extern BGrannyInstanceRenderer gGrannyInstanceRenderer;
