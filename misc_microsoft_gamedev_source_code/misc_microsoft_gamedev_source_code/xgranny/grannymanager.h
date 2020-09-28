//============================================================================
// grannymanager.h
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

#ifndef _GRANNYMANAGER_H_
#define _GRANNYMANAGER_H_

#include <granny.h>
#include "string\stringtable.h"
#include "fileManager.h"
#include "poolable.h"

//============================================================================
// Constants
//============================================================================

// This is the name used by the Granny post-export tool so files processed by
// the post-export tool can be differentiated from file not processed.
#define POST_GRANNY_EXPORT_TOOL_NAME "ESPostExport"

const long cAttachmentDepthMax = 6;

//============================================================================
// forward declarations
//============================================================================
class BGrannyModel;
class BGrannyAnimation;
class BGrannyInstance;

//============================================================================
// BGrannyReader
//
// Wrapper for doing granny's I/O through our BFile system instead of default 
// win32 file handling
//============================================================================
#pragma warning(disable: 4511)
class BGrannyReader
{
   public:
      granny_file_reader      mGrannyReader;
      BFile                   mFile;
};
#pragma warning(default: 4511)

void __cdecl grannyCloseReader(char const* sourceFileName, granny_int32x sourceLineNumber, granny_file_reader* readerInit);
granny_file_reader * __cdecl grannyOpenReader(char const* sourceFileName, granny_int32x sourceLineNumber, char const* fileNameToOpen);

//============================================================================
//============================================================================
class BGrannySampleAnimCache : public IPoolable
{
   public:
      BGrannySampleAnimCache();
      ~BGrannySampleAnimCache();

      DECLARE_FREELIST(BGrannySampleAnimCache, 10);

      //IPoolable Methods
      virtual void            onAcquire();
      virtual void            onRelease();

      granny_local_pose*      mLocalPose;
      uint16                  mStateChange;
      uint8                   mMaxBoneIndex;
      bool                    mLocalPoseSet:1;
      bool                    mUseIK:1;
};


//============================================================================
//============================================================================
class BRenderPrepareCacheBucket
{
   public:
      BRenderPrepareCacheBucket() : mUsedCount(0) {};
      
      BSmallDynamicSimArray<granny_local_pose*> mPoses;
      long                                      mUsedCount;    // jce [11/4/2008] -- this is really just for sanity checking that everyone is freeing their poses
};
enum
{
   cNumRenderPrepareCacheBuckets = 7
};


class BSparsePoseCache
{
   public:
      granny_local_pose*               mLocalPose;
      granny_int32x*                   mSparseBoneArray;
      granny_int32x*                   mSparseBoneArrayReverse;
};

//============================================================================
// BGrannyManager
//============================================================================
class BGrannyManager
{
   public:
                              BGrannyManager();
                              ~BGrannyManager();

      bool                    init(const BCHAR_T* pDirName);
      void                    deinit();
      void                    gameInit();
      
      // Reloading support.
      void                    reInitInstances(long modelIndex);
      void                    validateInstances();
      void                    rebindAnimation(int animIndex);     // used when animation is reloaded.
      
      // Unloads all assets associated with each granny model/animation. 
      // Returns false if there are any active instances!
      bool                    unloadAll();

      // Models.
      long                    getNumModels() const { return (mModels.getNumber()); }
      long                    getNumLoadedModels() const;
      long                    findModel(const BCHAR_T* pFileName);
      long                    createModel(const BCHAR_T* pFileName, bool loadFile=true);
      long                    getOrCreateModel(const BCHAR_T* pFileName, bool loadFile=true);
      BGrannyModel*           getModel(long index, bool ensureLoaded = true);
            
      // Animations.
      long                    getNumAnimations() const { return (mAnimations.getNumber()); }
      long                    getNumLoadedAnimations() const;
      long                    findAnimation(const BCHAR_T* pFileName);
      long                    createAnimation(const BCHAR_T* pFileName, bool loadFile=true);
      long                    getOrCreateAnimation(const BCHAR_T* pFileName, bool loadFile=true, bool synced=false);
      BGrannyAnimation*       getAnimation(long index, bool ensureLoaded = true, bool synced=false);
      

      // Instances
      long                    getNumInstances() const { return (mInstances.getNumber()); }
      BGrannyInstance*        createInstance();
      void                    incInstanceRef(BGrannyInstance* pInst);
      void                    releaseInstance(BGrannyInstance* pInst);

      // Temp poses for all to use.
      granny_local_pose*      getLocalPose() {const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex(); return(mLocalPose[threadIndex]);}
		granny_local_pose*      getBlendLocalPose() {const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex(); return(mBlendLocalPose[threadIndex]);}
      granny_world_pose*      getWorldPose() {const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex(); return(mWorldPose[threadIndex]);}

      BSparsePoseCache*       getLocalSparsePoseCache(long attachmentLevel) 
                                 {
                                    BASSERT(attachmentLevel < cAttachmentDepthMax);
                                    if(attachmentLevel >= cAttachmentDepthMax)
                                       return NULL;
                                    const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex(); 
                                    return(&(mLocalSparsePoseCache[threadIndex][attachmentLevel]));
                                 }


      // Max bones you can shove into these temporary pose variables.
      enum { cMaxBones = 128 };
                  
      void                    lockInstances(void) { mInstancesLocked = true; }
      void                    unlockInstances(void) { mInstancesLocked = false; }
            
      // Granny file fixup
      static void             convertCoordinateSystem(granny_file_info* pGrannyFileInfo, bool model, bool flipWinding = false);
      
      // Returns true if any granny models have been reloaded (one shot).
      bool                    getBoundingBoxesDirty(void) { const bool dirty = mBoundingBoxesDirty; mBoundingBoxesDirty = false; return dirty; }

      int                     getSampleAnimCacheRefCount() const { return mSampleAnimCacheRefCount; }

      int                     getStatFrameGetBoneCallCount() const { return mStatFrameGetBoneCallCount; }
      int                     getStatFrameGetBoneCachedCount() const { return mStatFrameGetBoneCachedCount; }

      int                     getStatTotalGetBoneCallCount() const { return mStatTotalGetBoneCallCount; }
      int                     getStatTotalGetBoneCachedCount() const { return mStatTotalGetBoneCachedCount; }

      void                    incStatGetBoneCallCount() { mStatFrameGetBoneCallCount++; mStatTotalGetBoneCallCount++; }
      void                    incStatGetBoneCachedCount() { mStatFrameGetBoneCachedCount++; mStatTotalGetBoneCachedCount++; }

      void                    resetFrameStats() { mStatFrameGetBoneCallCount=0; mStatFrameGetBoneCachedCount=0; }
      void                    resetTotalStats() { mStatTotalGetBoneCallCount=0; mStatTotalGetBoneCachedCount=0; }

      BGrannySampleAnimCache* createSampleAnimCache();
      void                    releaseSampleAnimCache(BGrannySampleAnimCache* pCache);
      bool                    getEnableSampleAnimCache() const { return mEnableSampleAnimCache; }
      void                    setEnableSampleAnimCache(bool val) { mEnableSampleAnimCache=val; }
      uint                    getSampleAnimCacheCounter() const { return mSampleAnimCacheCounter; }

#ifndef BUILD_FINAL
      struct BAssetAllocStats
      {
         BString  mFilename;
         uint     mAllocationSize;
         
         bool operator< (const BAssetAllocStats& rhs) const { return mAllocationSize > rhs.mAllocationSize; }
      };
      typedef BDynamicArray<BAssetAllocStats> BAssetAllocStatsArray;
      
      void                    getModelAllocStats(BAssetAllocStatsArray& stats);
      void                    getAnimAllocStats(BAssetAllocStatsArray& stats);
#endif

      // jce [10/31/2008] -- Functions related to cache for storing interpolated poses as we go into render to get attachments right.
      void                    beginRenderPrepare();
      granny_local_pose*      getRenderPreparePose(BGrannyInstance *instance);
      void                    releaseRenderPreparePose(granny_local_pose *pose, long numBones);
      void                    endRenderPrepare();   // should be called after all instances are flushed to the renderer
      void                    clearRenderPreparePoseCache();
      

   protected:
      
      BSimString                       mBaseDirectory;
      
      granny_local_pose*               mLocalPose[cThreadIndexMax];
		granny_local_pose*               mBlendLocalPose[cThreadIndexMax];
      granny_world_pose*               mWorldPose[cThreadIndexMax];

      BSparsePoseCache                 mLocalSparsePoseCache[cThreadIndexMax][cAttachmentDepthMax];

      BSmallDynamicSimArray<BGrannyInstance*>   mInstances;
      BSmallDynamicSimArray<BGrannyModel*>      mModels;
      BSmallDynamicSimArray<BGrannyAnimation*>  mAnimations;
      
      BSmallDynamicSimArray<BGrannyInstance*> mRenderPrepareInstances;

      // For caching local poses during rendering.
      BRenderPrepareCacheBucket        mRenderPreparePoseCache[cNumRenderPrepareCacheBuckets];
      
      int                              mStatFrameGetBoneCallCount;
      int                              mStatFrameGetBoneCachedCount;

      int                              mStatTotalGetBoneCallCount;
      int                              mStatTotalGetBoneCachedCount;

      uint                             mSampleAnimCacheRefCount;
      uint                             mSampleAnimCacheIndex;
      uint                             mSampleAnimCacheCounter;

      BStringTable<short, false>       mModelNameTable;
      BStringTable<short, false>       mAnimationNameTable;

      bool                             mInstancesLocked : 1;
      bool                             mBoundingBoxesDirty : 1;
      bool                             mEnableSampleAnimCache : 1;
};

// gGrannyManager
extern BGrannyManager gGrannyManager;

#endif
