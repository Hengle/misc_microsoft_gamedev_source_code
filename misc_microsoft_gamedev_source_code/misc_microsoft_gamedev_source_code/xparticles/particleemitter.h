//============================================================================
//
//  particleemitter.h
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#pragma once

#define ENABLE_DEPTHPARTICLES

#include "particlesystem.h"
#include "particlerenderer.h"
#include "containers\segmentedArray.h"

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
class BParticleSystemParticle;
class BParticleSystemManager;

//----------------------------------------------------------------------------
//  Class BParticleEmitter
//----------------------------------------------------------------------------
class BParticleEmitter
{
   BParticleEmitter(const BParticleEmitter&);
   BParticleEmitter& operator= (const BParticleEmitter&);
   
public:

   //-- Construction/Destruction
   BParticleEmitter();
   ~BParticleEmitter();

   enum eEmitterFlag
   {
      eFlagVisible = 0,
      eFlagHibernate,
      eFlagNearLayer, //-- emitter should be rendered in the near layer       
      eFlagUsedLastTransform,
      eFlagUsedLastTransform2,
      eFlagTotal,
   };

   enum eEmitterState
   {
      eStateKilled = 0, // the emitter is dead
      eStateDormant,    // the emitter is sitting around waiting a specific amount of time to emit new particles
      eStateActive,     // the emitter is active and emitting particles
      eStateStopped,    // the emitter was stopped, its active but not emitting any particles
      eStateAboutToDie, // the emitter is about to die waiting for all of its particles to die off before it releases itself      
      eStateTotal
   };

   enum
   {
      cMaxViewports = 2,
   };
   
   //-- XBOX Rendering
   void  submitRenderCommand(int tileIndex, BParticleRenderer::eRenderCommand type, void* pBufferVB, int startOffsetVB, void* pBufferIB, int startOffsetIB, int vertexCount, int indexCount, int texHandle, int texHandle2, int texHandle3, int intensityTexHandle);
   
   bool   computeBoundingBox  (BVector &minCorner, BVector &maxCorner);                                          

   //-- System Interface
   void                     init              (BParticleSystemManager* pManager, BParticleEmitterDefinition* pData, XMMATRIX matrix, DWORD tintColor, float additionalTimeDelay);
   void                     kill              ();
   void                     stop              ();
   void                     renderBegin       ();
   void                     render            (int tileIndex);
   void                     renderEnd         ();
   void                     renderEndOfFrame  ();
   bool                     getFlag           (long n) const { return(mFlags.isSet(n)!=0); }
   void                     setFlag           (long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

   void                     setParentEffectIndex(int value) {mParentEffectIndex = value;}
   int                      getParentEffectIndex() const    {return mParentEffectIndex;}
   
   // Returns true if the particle emitter should be killed because it's too old.
   bool                     tick              (long currentTime);
      
   void                     updatePhase1      (bool canEmitNewParticles);
   void                     updatePhase2      (IDirect3DVertexBuffer9* pVB, void* pVBBuffer, uint vbStartOffset, IDirect3DIndexBuffer9* pIB, void* pIBBuffer, uint ibStartOffset);
   void                     releaseParticles  (long currentTime);
   void                     setTransform      (BMatrix mtx);   
   void                     setSecondaryTransform(BMatrix mtx);
   bool                     isDead            ();
   void                     setTintColor      (DWORD color);
   DWORD                    getTintColor      () const { return mTintColor.c; };         

   //-- Data Interface
   void  setMaxRange            (float range);   

   void  setScaleOverride       (const float scale) { mScaleOverride = scale; }
   float getScaleOverride       (void) const        { return mScaleOverride; }         
   
   //-- Inline Interface
   long                    getNumParticles        () const { return mParticles.getSize(); }   
            
   uint                    getTotalParticlesAllocated(void) const { return mParticles.getCapacity(); }
   uint                    getTotalParticlesAllocatedInBytes(void) const { return mParticles.getCapacity() * sizeof(BParticleSystemParticle); }

   const BMatrix&          getTransform           () const { return mTransform;           }
   const BMatrix&          getLastUpdateTransform () const { return mLastUpdateTransform; }
   const BMatrix&          getTransform2          () const { return mTransform2;          }
   const BMatrix&          getLastUpdateTransform2() const { return mLastUpdateTransform2;}


   XMVECTOR                getBoundingMins        () const { return mBoundingMins;        }
   XMVECTOR                getBoundingMaxs        () const { return mBoundingMaxs;        }
   BParticleEmitterDefinition* getData            () const { return mpData;               }
   BParticleSystemManager* getManager             () const { return mpManager;            }

   void                    setPriority            (float value) { mPriority = value ;}
   void                    setToDie               () { mState = eStateAboutToDie; };
   eEmitterState           getState               () const { return mState; };
   void                    setOpacity             (float opacity) { mOpacity = opacity; };
   float                   getOpacity             () const { return mOpacity; };

   BParticleSystemParticle* emitParticle        (long curTime, BMatrix emitterMatrix);
   long                    getLastUpdateTime    () const             { return mLastUpdateTime;           }   
   
   void                    setAdditionalDelay   ( float delay ) { mAdditionalDelay = delay; }            //Set by the particle set

   int                     sizeOf               () {return sizeof(BParticleEmitter);};
   void                    reinitTimeData       ();  //Needed for when time scale changes (for particles initialized during dynamic load screens) // MWC [3/28/2005]

   long                    getStartTime()       { return mStartTime; }

   //Had to make this public in order to prime the instant projectile tracers
   void                    emitParticles       (long updateTime);
   void                    emitTrailParticles  (long updateTime);
   
   uint                    getNumVerts(void) const { return mParticles.getSize(); }

   void                    returnAllParticles(bool bKillImmediately);   
   void                    resetBoundingBox();

   //-- MT Support
   void                    computeGPUFrameStorage(uint& frameStorageVB, uint& frameStorageIB);

   // Priority System
   float                   computeScore() const;

   void                    debugRenderParticles();
   void                    debugRenderMagnets();


   // Viewport helpers 
   bool                    getBoundSphereVisibleInAnyViewport() const;
   void                    setBoundSphereVisibleForAllViewports(bool bValue);
   bool                    getBoundSphereVisibleForAllViewports() const;
   void                    setCompletelyFadedOutForAllViewports(bool bValue);
   bool                    getCompletelyFadedOutForAllViewports() const;
   void                    setBoundSphereVisible(int viewportIndex, bool bValue);
   bool                    getBoundSphereVisible(int viewportIndex) const;
   void                    setCompletelyFadedOut(int viewportIndex, bool bValue); 
   bool                    getCompletelyFadedOut(int viewportIndex) const;

   void                    setVisibleInAllViewports(bool bValue);
   bool                    getVisibleInAllViewports() const;
   void                    setVisibleInViewport(int viewportIndex, bool bValue);
   bool                    getVisibleInViewport(int viewportIndex) const;


private:
   //-- Private Data
   void*                                 mpVB;
   void*                                 mpIB;

   uint                                  mVBStartOffset;
   uint                                  mIBStartOffset;
   
   int                                   mVertCount;
   int                                   mIndexCount;
   
   UTBitVector<64>                       mFlags;
   BMatrix                               mTransform;
   BMatrix                               mTransform2;
   BMatrix                               mLastUpdateTransform;
   BMatrix                               mLastUpdateTransform2;
   XMVECTOR                              mBoundingMins;
   XMVECTOR                              mBoundingMaxs;   
   BVector                               mAdditionalOffset;//Set by the particle set
   
   int                                   mParentEffectIndex;

   eEmitterState                         mState;
   long                                  mClockTime;
   long                                  mPrevClockTime;
   
   long                                  mStartTime;
   double                                mStartTimeF;
   long                                  mDeathTime;
   long                                  mMaxLifeTime;  
   long                                  mTotalTimeInvisible;
   float                                 mPriority;
   
   long                                  mStopTime;
   long                                  mLastUpdateTime;
   long                                  mUpdateDeltaTime;
   long                                  mLastPositionTime;
   long                                  mActivityTime;   
   float                                 mNextParticleWait;
   float                                 mParticleEmmisionRate;
   float                                 mEmissionDistanceRemaining;
   float                                 mScaleOverride;
   float                                 mOpacity;
   XMCOLOR                               mTintColor;
   BParticleEmitterDefinition*           mpData;   
   BParticleSystemManager*               mpManager;

   typedef BSegmentedArray<BParticleSystemParticle, 3> BParticleArray;
      
   BParticleArray                        mParticles;
   
   //-- Extents Data
   float                                 mParticleMaxRadius;
            
   float                                 mAdditionalDelay;                   //Set by the particle set      
      
   //-- Rendering Data
   BCommandListenerHandle                mCommandListenerHandle;   

   //-- state that may change every frame
   bool                                  mBoundSphereVisible[cMaxViewports];
   bool                                  mCompletelyFadedOut[cMaxViewports];
   bool                                  mIsVisibleInViewport[cMaxViewports];
   
   //-- Private Functions   
   bool                     emitsNormally       (void);   
   
   void                     processEmission     (long updateTime);
   void                     emitInitialParticles(void);   
   void                     updateParticles     (long updateTime, void* __restrict pVB, void* pIB);
   void                     updateParticlesLifeOnly();   
   void                     updateParticlesVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount);
   void                     updateTrailParticlesLVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount);
   void                     updateBeamParticlesVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount);
   void                     updatePFXParticlesVMX(XMVECTOR fUpdateTime);
   void                     updateCollisionsVMX(XMVECTOR fUpdateTime);
   void                     sortParticlesVMX(const void* __restrict pBuffer, int vertCount, void* pIB, int& indexCount);
   void                     updateMagnetsVMX(XMVECTOR fUpdateTime);

   
   
      
   DWORD                    choosePaletteColor(long index);
   void                     cleanupDeadParticlesSorted(void);
   void                     cleanupDeadParticlesUnsorted(void);
      
   float                    computeParticleTextureArrayLookup(int textureArrayIndex);

   void                     emitBeamParticles(long updateTime);
   void                     emitTrailParticlesByLength(long updateTime);
   void                     emitTrailParticlesByTime  (long updateTime);
   void                     transformAABB();
};



