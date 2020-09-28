//============================================================================
//
//  ParticleSystemManager.h
//
//  Copyright (c) 2000 Ensemble Studios
//

//============================================================================
#pragma once

#include "particleHeap.h"
#include "particleSystemData.h"
#include "particleemitter.h"
#include "particleSystemParticle.h"
#include "particledatatracker.h"
#include "particleemitterset.h"
#include "particleeffectdata.h"
#include "renderToTextureXbox.h"
#include "radixsorter.h"

#include "effect.h"
#include "threading\eventDispatcher.h"

#include "math\random.h"
#include "timer.h"

//#include "propertyDefs.h"
//----------------------------------------------------------------------------
//  Callback Prototypes
//----------------------------------------------------------------------------
typedef DWORD (PS_GET_TIME_FUNC)       (void);
typedef bool  (PS_COLLISION_FUNC)      (const BVector& p1, const BVector& p2, BVector& intersection, BVector& normal, bool terrain, bool water, bool units, void* pParam);
typedef bool  (PS_GET_WIND_FUNC)       (const BVector& position, BVector& result);

const long PS_NUM_RAND_FLOATS = 4096; //1024

static const XMVECTOR gVectorOne = { 1.0f, 1.0f, 1.0f, 1.0f };
static const XMVECTOR gVectorFloatCompEps = { cFloatCompareEpsilon, cFloatCompareEpsilon, cFloatCompareEpsilon, cFloatCompareEpsilon };

const cMaxInstancesPerDataLimit = 1024;

class BDynamicGPUBuffer;

typedef int BParticleEffectDataHandle;

//============================================================================
// class BParticleCreateParams
//============================================================================
class BParticleCreateParams
{
public: 
   BParticleCreateParams() : mDataHandle(-1), mPriority(1), mNearLayerEffect(false), mTintColor(0xFFFFFFFF)  {};
   ~BParticleCreateParams() {};

   BMatrix                   mMatrix;
   BParticleEffectDataHandle mDataHandle;
   int                       mPriority;
   DWORD                     mTintColor;
   bool                      mNearLayerEffect:1;
};

//----------------------------------------------------------------------------
//  Class BParticleSystemManager
//  All methods except init/deinit can only be called from the render thread!
//----------------------------------------------------------------------------
class BParticleSystemManager : public BEventReceiverInterface
{
public:   

   //Enum for particle type
   enum 
   {
      cParticleSystem,
      cParticleSet
   };

   //Pre configured Render effects (no need to do this each time an instance is created.)
   enum 
   {
      cNormal = 0,
      cAdditive,
      cSubtractive,
      cNumParticleRenderEffects
   };

   enum 
   {
      cParticleSystemEventClassUpdateStats = cEventClassFirstUser,
      cParticleSystemEventClassCreateThreadLocalRandom,
      cParticleSystemEventClassDeleteThreadLocalRandom,
   };
   
   //-- Construction/Destruction
                              BParticleSystemManager();
   virtual                   ~BParticleSystemManager();            
   
   //-- Interface
   void                       init                       (long dataBlockSize, long instanceBlockSize, long particleBlockSize);   
   void                       kill                       (void);
   void                       initMemoryPools            (void);
   void                       deinitMemoryPools          (void);
   void                       initTextureSystem          (void);
   void                       deinitTextureSystem        (void);
   void                       killAllInstances           (void);
   void                       killAllData                (void);

   void                       setMaxParticles            (long max);
   long                       getMaxParticles            (void);
   long                       getNumDatasLoaded          (void);
   long                       getNumInstancesAllocated   (void);
   long                       getNumInstancesInUse       (void);
   long                       getNumParticlesAllocated   (void);
   long                       getNumParticlesInUse       (void);
   void                       setPaused(bool pause)      { setFlagPauseUpdate(pause); };
   bool                       getPaused() const          { return getFlagPauseUpdate(); };
   void                       setTimeSpeed(float speed)  { mTimeSpeed = speed; }   
   float                      getTimeSpeed() const       { return mTimeSpeed ; }
   void                       enableDistanceFade(bool enable) { setFlagDistanceFade(enable); };
   void                       setDistanceFade(float startDistance, float endDistance);
   void                       enableBBoxRendering(bool enable);   
   bool                       getRenderNoTilingParticles() const { return (mRenderNoTilingInstances.getSize() > 0); };

   BOOL                       isDistortionEnabled        (void) const { return mRenderDistortionInstances.getNumber() > 0;}
   IDirect3DTexture9*         getDistortionTexture       (void) const { return mpDistortionTexture; };
   
   void                       resetRenderFlags           (void);
   
   void                       renderBegin                ();
   void                       render                     (uint tileIndex);
   void                       renderEnd                  ();
   void                       renderEndOfFrame           ();

   void                       renderNoTilingBegin        ();
   void                       renderNoTiling             ();
   void                       renderNoTilingEnd          ();
   void                       renderNoTilingEndOfFrame   ();
   
   void                       renderDistortionBegin      ();
   void                       renderDistortion           ();
   void                       renderDistortionEnd        ();
   void                       renderDistortionEndOfFrame ();


   void                       renderNearLayerBegin       ();
   void                       renderNearLayer            (uint tileIndex);
   void                       renderNearLayerEnd         ();
   void                       renderNearLayerEndOfFrame  ();

   void                       releaseDynamicGPUResources ();
   void                       releaseGPUFrameHeapResources();
         
   void                       updateFrame                ();
   void                       updateRenderList           (int viewportIndex);
   long                       getNumInstancesRendered    (void) { return mNumInstancesRendered;     }
   long                       getNumParticlesRendered    (void) { return mNumParticlesRendered;     }   

   //-- Resource Management
   int                              findDataIndex        (const BString& filename);
   BParticleEffectDefinition*       getData              (const BString& filename, int* pIndex);   
   BParticleEffectDefinition*       getData              (int index);
   
   bool                       loadEffectDefinition       (const BString& fileName, int* pIndex);
   
   BParticleEmitter*          createEmitter              (void);      
   void                       destroyAllInstances        (void);
   void                       destroyAllData             (void);
   void                       releaseEmitter             (BParticleEmitter* pEmitter, bool killNow = false);   

  
   void                       releaseEffect              (BParticleEffect* pEffect, bool bKillImmediately, bool releaseEmitters = true);   
   BParticleEffect*           createEffect               (const BParticleCreateParams& params);   

   // base directory ID
   void                       setBaseDirectoryID(long ID) { mBaseDirectoryID = ID; }
   long                       getBaseDirectoryID(void) const { return mBaseDirectoryID; }   
   
   //-- Helper Functions
   static long                getRandom                  (long range);
   static float               getRandomRange             (float min, float max);
   static float               applyVariance              (float value, float variance);
   static DWORD               lerpColor                  (DWORD color1, DWORD color2, float alpha);
   static float               lerpScalar                 (float a, float b, float alpha);
   static inline float        getIndexedVarience         (const long index, const float basevalue, const float varience) {return basevalue * (1.0f + mRandomFloats[index&(PS_NUM_RAND_FLOATS-1)]*varience); }
   
   // Please try to use getTimeFloat() for new code, it's much more accurate!
   long                       getTime                    (void){ return(mPSysManTime); }
   double                     getTimeFloat               (void){ return(mPSysManTimeF); }
   
   void                       updatePSysManTime          (double gameTime);
   void                       reinitAllInstanceTimeData  ();
   void                       setParticlePerformanceValues( long emissionRate, long emissionCap, bool tracers, bool envEffects );    

   void                       updateRunTimeStats();
   
   // Flags
   void                       setFlagPauseUpdate(bool value)           { mbFlagPauseUpdate = value; };
   bool                       getFlagPauseUpdate() const               { return mbFlagPauseUpdate;  };
   void                       setFlagDistanceFade(bool value)          { mbFlagDistanceFade = value; };
   bool                       getFlagDistanceFade() const              { return mbFlagDistanceFade;  };
   void                       setFlagUseAliasedFillSurface(bool value) { mbFlagUseAliasedFillSurface = value; };
   bool                       getFlagUseAliasedFillSurface() const     { return mbFlagUseAliasedFillSurface; };
   void                       setFlagRenderMagnets(bool value)         { mbFlagRenderMagnets = value; };
   bool                       getFlagRenderMagnets() const             { return mbFlagRenderMagnets; };
   void                       setFlagEnableCulling(bool value)         { mbFlagEnableCulling = value; };
   bool                       getFlagEnableCulling() const             { return mbFlagEnableCulling; };
         
   // For statistics purposes only.
   BDynamicGPUBuffer*         getDynamicGPUBuffer(void) { return mpDynamicGPUBuffer; }
   
   bool                       getLightBufferingEnabled() const { return mLightBuffering; }
   void                       setLightBufferingEnabled(bool enabled) { mLightBuffering = true; }
   
   float                      getLightBufferIntensityScale() const { return mLightBufferIntensityScale; }
   void                       setLightBufferIntensityScale(float scale) { mLightBufferIntensityScale = scale; }
   XMVECTOR                   getSunColor() const { return mSunColor; }
   void                       setSunColor(XMVECTOR value) { mSunColor = XMVectorSelect(value, XMVectorSplatOne(), XMVectorSelectControl(0,0,0,1)); }

#ifndef BUILD_FINAL
   BParticleDataTracker    mRuntimeStats;
   float  mDebugStatUpdateTime;
   float  mDebugStatRenderTime;   
#endif

#ifndef BUILD_FINAL   
   struct BStats :  public BEventPayload
   {
      public: 
         BStats(){clear(); clearMaxStats();};

         void clear(void)
         {
            mNumActiveEmitters = 0;
            mNumActiveParticles = 0;
            mTotalUpdateTime = 0;
            mRenderTime = 0;
            mEmissionTime = 0;
            mVMXUpdateTime = 0;
            mEmissionTimeBeam = 0;
            mEmissionTimeNormal = 0;
            mEmissionTimeTrail = 0;
                        
            mNumUpdatedEmitters = 0;
            mNumUpdatedParticles = 0;
            mNumUpdatedParticlesPhase1 = 0;
            mNumUpdatedEmittersPhase1 = 0;
            mNumUpdatedParticlesPhase2 = 0;
            mNumUpdatedEmittersPhase2 = 0;
                        
            mTotalNumRenderedEmittersAcrossAllTiles = 0;
            mTotalNumRenderedParticlesAcrossAllTiles = 0;

            mTotalEmittersCreated = 0;
            mTotalParticlesAllocated = 0;
            mTotalEmittersForceKilled = 0;
            mTotalEmittersKilled = 0;

            mNumActiveEffects = 0;
            mNumActiveLoopEmitters = 0;
            mNumActiveNonLoopEmitters = 0;
            mTotalNumActiveEffectEmitters = 0;
            mNumActiveNonLoopEffectEmitters = 0;
            mNumActiveLoopingEffectEmitters = 0;

            mTotalEmittersAboutToDie = 0;
            mTotalEmittersAboutToDieLooping = 0;
            mTotalEmittersAboutToDieNonLooping = 0;
            mTotalEmittersAboutDieUnaccountedFor = 0;

            mTotalEmittersActive = 0;
            mTotalEmittersActiveLooping = 0;
            mTotalEmittersActiveNonLooping = 0;
            mTotalEmittersActiveUnaccountedFor = 0;
            
            for (int i = 0; i < BEmitterBaseData::eTypeTotal; ++i)
               mNumUpdatedEmittersTypes[i] = 0;

            mNumDroppedEffects = 0;
            mDataCount = 0;
            mUniqueEmitters = 0;
         }

         void clearMaxStats()
         {
            mSceneMaxActiveEffects  = 0;
            mSceneMaxActiveEmitters = 0;
            mSceneMaxActiveParticles = 0;
            mSceneMaxRenderedParticles = 0;
            mSceneMaxRenderedEmitters = 0;
         }

         // Okay for this to use the primary heap.
         virtual void deleteThis(bool delivered) { delete this; }
         void setNumTiles(uint numTiles) { mTileData.clear(); mTileData.resize(numTiles); }

      struct TileData
      {
         uint mNumEmitters;
         uint mNumParticles;
      };

      double  mTotalUpdateTime;
      double  mVMXUpdateTime;
      double  mEmissionTime;
      double  mEmissionTimeBeam;
      double  mEmissionTimeNormal;
      double  mEmissionTimeTrail;
      double  mRenderTime;
      uint    mNumActiveEffects;
      uint    mTotalNumActiveEffectEmitters;
      uint    mNumActiveNonLoopEffectEmitters;
      uint    mNumActiveLoopingEffectEmitters;
      uint    mNumActiveInvalidDataEffectEmitters;
      uint    mNumActiveParticles;
      uint    mNumActiveEmitters;
      uint    mNumActiveLoopEmitters;
      uint    mNumActiveNonLoopEmitters;

      uint    mNumUpdatedEmitters;
      uint    mNumUpdatedParticles;      
      uint    mNumUpdatedParticlesPhase1;
      uint    mNumUpdatedEmittersPhase1;
      uint    mNumUpdatedParticlesPhase2;
      uint    mNumUpdatedEmittersPhase2;

      uint    mNumUpdatedEmittersTypes[BEmitterBaseData::eTypeTotal];

      uint    mTotalNumRenderedEmittersAcrossAllTiles;
      uint    mTotalNumRenderedParticlesAcrossAllTiles;
      
      uint    mTotalParticlesAllocated;
      uint    mTotalEmittersCreated;
      uint    mTotalEmittersForceKilled;
      uint    mTotalEmittersKilled;

      uint    mTotalEmittersInUsedList;
      
      uint    mTotalEmittersActive;
      uint    mTotalEmittersActiveLooping;
      uint    mTotalEmittersActiveNonLooping;
      uint    mTotalEmittersActiveUnaccountedFor;

      uint    mTotalEmittersAboutToDie;
      uint    mTotalEmittersAboutToDieLooping;
      uint    mTotalEmittersAboutToDieNonLooping;
      uint    mTotalEmittersAboutDieUnaccountedFor;

      uint    mSceneMaxActiveEffects;
      uint    mSceneMaxActiveEmitters;
      uint    mSceneMaxActiveParticles;
      uint    mSceneMaxRenderedParticles;
      uint    mSceneMaxRenderedEmitters;

      uint    mNumDroppedEffects;
      uint    mDataCount;
      uint    mUniqueEmitters;

      BDynamicParticleArray<TileData> mTileData;
      
   }; 
   void getSimStats(BStats& stats);       
   BStats mWorkerStats;
   BStats mSimStats;

   BString mLastReloadPath;
   int     mLastReloadTick;
#endif   

   struct BThreadUpdateData
   {
      BParticleEmitter*       mpEmitter;
      BHandle                 mEmitterHandle;
      IDirect3DVertexBuffer9* mpVB;
      IDirect3DIndexBuffer9*  mpIB;
      void*                   mpBufferVB;
      void*                   mpBufferIB;
      uint                    mStartOffsetVB;
      uint                    mStartOffsetIB;
   };

   static float               mRandomFloats[PS_NUM_RAND_FLOATS];   
  
private:   
   BEventReceiverHandle       mThreadEventHandle[cThreadIndexMax];

   bool                       initDistortionBuffers(uint width, uint height);

   //-- Private Functions   
   void                       calculateOrthoProjMatrix(float fov, long windowWidth, long windowHeight, float nearZ, float farZ, D3DXMATRIX &orthoProjMatrix);   

   //-- Effect Management
   void                       addEmitterToFreelistAndRemoveFromUsedList(BParticleEmitter* pEmitter);
   void                       addEmitterToFreelist(BParticleEmitter* pEmitter);
   bool                       findEmitterInFreelist(BParticleEmitter* pEmitter, BHandle& hNode);
   bool                       findEmitterInUsedList(BParticleEmitter* pEmitter, BHandle& hNode);
   void                       detachEmitterFromEffect(BParticleEmitter* pEmitter);


   //-- Event Receiver Interface
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   BParticleEffectDefinition* reloadData(int index);
   bool reloadEffect(const BString path);
   
   //-- Update Helper Functions
   void clearUpdateLists();
   void clearRenderLists();
   void updateTickAndCull();
   void updateParentEmitters();
   void updateEmitters(BTimer& timer);
   
   void clearDebugStats();
   void updateDebugStats(BTimer& timer);

   void dumpHashmap() const;

   //-- Multi Threading functions
   static void emitterUpdatePhase1Callback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   static void emitterUpdatePhase2Callback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   
   void workerInit(void* pData);
   void workerDeinit(void* pData);

   BVector                                mCameraPos;
   XMVECTOR                               mEmitterFadeStartDistanceV;
   XMVECTOR                               mEmitterFadeEndDistanceV;   
   XMVECTOR                               mEmitterOneOverFadeRangeV;

   XMVECTOR                               mSunColor;
   //-- Private Data
   long                                   mBaseDirectoryID;

   float                                  mTimeSpeed;   
   long                                   mInstanceBlockSize;
   long                                   mParticleBlockSize;
   long                                   mNumInstancesRendered;
   long                                   mNumParticlesRendered;
   long                                   mNumParticlesAllocated;   
 
   float                                  mEmitterFadeStartDistance;
   float                                  mEmitterFadeEndDistance;   
   float                                  mEmitterFadeRange;
   float                                  mEmitterOneOverFadeRange;

   // resource header structs
   IDirect3DVertexBuffer9                 mVB;
   IDirect3DIndexBuffer9                  mIB;

   BRenderToTextureHelperXbox             mDistortionRenderTarget;
   IDirect3DTexture9*                     mpDistortionTexture;
   uint                                   mDistortionBufferWidth;
   uint                                   mDistortionBufferHeight;

   IDirect3DSurface9*                     mpColorSurface;
   IDirect3DSurface9*                     mpDepthSurface;

   IDirect3DSurface9*                     mpColorSurfaceAliasedAA;
   IDirect3DSurface9*                     mpDepthSurfaceAliasedAA;

   
   typedef BHashMap<BString, int> BParticleEffectDataHashMap;
   BParticleEffectDataHashMap                mDataHashmap;
   BDynamicParticleArray<BParticleEffectDefinition*> mData;
   BPointerList<BParticleEmitter>            mUsedInstanceList;
   BPointerList<BParticleEmitter>            mFreeInstanceList;
   BPointerList<BParticleEmitter>            mInstanceBlocks;
   BDynamicParticleArray<BParticleEffect*>   mEffectList;

   BDynamicParticleArray< BDynamicParticleArray<BParticleEmitter*> > mRenderInstances;
   BDynamicParticleArray<BParticleEmitter*>  mRenderDistortionInstances;
   BDynamicParticleArray< BDynamicParticleArray <BParticleEmitter*> > mRenderNearLayerInstances;
   BDynamicParticleArray<BParticleEmitter*>  mRenderNoTilingInstances;
      
   long                                     mPSysManTime;   
   double                                   mPSysManTimeF;   

   //-- Mulithreading Data
   static BCountDownEvent                   mRemainingWorkBuckets;
   static bool                              mPermitEmission;
   uint                                     mTotalGPUFrameStorageNeededVB;
   uint                                     mTotalGPUFrameStorageNeededIB;
   
   BDynamicGPUBuffer*                       mpDynamicGPUBuffer;
   
   float                                    mLightBufferIntensityScale;
   

   bool                                     mLightBuffering;

   //-- Work Arrays   
   uint                                     mLocalTotalNumAllowedInstancesThisUpdate;
   BDynamicParticleArray<BParticleEmitter*> mLocalInstances;
   BDynamicParticleArray<float>             mLocalOrder;
   BDynamicParticleArray<BHandle>           mLocalList;

   BDynamicParticleArray<BParticleEmitter*> mPFXInstances;   

   BRadixSorter                             mUpdateSorter;
   BRadixSorter                             mRenderSorter;


   BDynamicParticleArray<BParticleEmitter*> mZLocalInstances;
   BDynamicParticleArray<float>             mZLocalOrder;
   BDynamicParticleArray<BHandle>           mZLocalList;

   static __declspec(thread) Random*        mpRandom;   

   // flags
   bool mbFlagPauseUpdate           : 1;
   bool mbFlagDistanceFade          : 1;
   bool mbFlagUseAliasedFillSurface : 1;
   bool mbFlagRenderMagnets         : 1;
   bool mbFlagEnableCulling         : 1;
   bool mbFlagMemoryPoolsInitalized : 1;
};

extern BParticleSystemManager gPSManager;

#define PS_COMPUTE_VARIANCE_X(randomV, baseValue, varience) \
   ( XMVectorMultiply(XMVectorMultiplyAdd(randomV, varience, gVectorOne), baseValue) )

#define PS_GET_VARIANCE_X(randomV, baseValue, varience) \
   ( ((randomV * XMLoadScalar((&varience)) + gVectorOne) * XMLoadScalar(&(baseValue))) )

#define PS_GET_INDEXED_VARIANCE_X(index, baseValue, varience) \
   ( (XMLoadScalar(&gPSManager.mRandomFloats[(index) & (PS_NUM_RAND_FLOATS - 1)]) * XMLoadScalar((&varience)) + gVectorOne) * XMLoadScalar(&(baseValue)) )

#define PS_GET_INDEXED_VARIANCE_4(index, baseValue, varience) __vpermwi(PS_GET_INDEXED_VARIANCE_X(index, baseValue, varience), 0)

#define PS_GET_INDEXED_VARIANCE_Y(index, baseValue, varience) __vrlimi(XMVectorZero(), PS_GET_INDEXED_VARIANCE_X(index, baseValue, varience), VRLIMI_CONST(0,1,0,0), 3)
#define PS_GET_INDEXED_VARIANCE_Z(index, baseValue, varience) __vrlimi(XMVectorZero(), PS_GET_INDEXED_VARIANCE_X(index, baseValue, varience), VRLIMI_CONST(0,0,1,0), 2)
#define PS_GET_INDEXED_VARIANCE_W(index, baseValue, varience) __vrlimi(XMVectorZero(), PS_GET_INDEXED_VARIANCE_X(index, baseValue, varience), VRLIMI_CONST(0,0,0,1), 1)




