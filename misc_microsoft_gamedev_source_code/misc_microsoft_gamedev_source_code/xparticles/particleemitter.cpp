//============================================================================
//
//  particleemitter.cpp
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xparticlescommon.h"
#include "particleemitter.h"
#include "particlevertextypes.h"
#include "ParticleSystemManager.h"
#include "boundingBox.h"
#include "radixsorter.h"
#include "D3dTexture.h"
#include "vertexTypes.h"
#include "particlerenderer.h"
#include "particletexturemanager.h"
#include "math\VMXUtils.h"
#include "math\VMXIntersection.h"
#include "..\xgameRender\tiledAA.h"

#include "timer.h"
#include "TerrainSimRep.h"

//#define DEBUG_TOTAL_UPDATE_TIMER
//#define DEBUG_UPDATE_TIMER
//#define DEBUG_EMISSION_TIMER
//#define DEBUG_CLEANUP_TIMER
//#define DEBUG_TRACE_RECORD
//#define DEBUG_VMX_UPDATE

#ifdef  DEBUG_TRACE_RECORD
#include "tracerecording.h"
#include "xbdm.h"
#pragma comment( lib, "tracerecording.lib" )
#endif

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
const int   MIN_VELOCITY_TIME_SLICE = 1;
const float MIN_UPDATE_RADIUS = 2.0f;
const float PBIAS = -0.01f;


const int cBeamTesselation = 100;

//============================================================================
//  MACROS
//============================================================================
#define RANDOM(range)          (BParticleSystemManager::getRandom(range))
#define RANDOM_RANGE(min, max) (BParticleSystemManager::getRandomRange(min, max))
#define VARIANCE(val, var)     (BParticleSystemManager::applyVariance(val, var))
#define RTVARIANCE(index, val, var)     BParticleSystemManager::getIndexedVarience(index, val, var)
#define TIMEVARIANCE(val, var) ((long)(BParticleSystemManager::applyVariance(val, var)*1000.0f))


//============================================================================
//  PRIVATE ENUMS
//============================================================================



//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BParticleEmitter::BParticleEmitter():
   mpData(NULL),
   mpManager(NULL),
   mpVB(NULL),
   mVertCount(0),
   mUpdateDeltaTime(0),
   mOpacity(1.0f),
   mpIB(NULL),
   mIndexCount(0),
   mPriority(1000.0f),
   mParticles(&gParticleBlockHeap),
   mParentEffectIndex(-1),
   mTintColor(cDWORDWhite)
{
   kill();
   mFlags.setAll(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleEmitter::~BParticleEmitter()
{
   kill();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::init(BParticleSystemManager* pManager, BParticleEmitterDefinition* pData, XMMATRIX matrix, DWORD tintColor, float additionalDormantTime)
{
   //-- Validate.
   BASSERT(pManager);
   BASSERT(pData);

   //-- Reset everything.
   kill();
      
   //-- See what time it is.
   long curTime = pManager->getTime();
   mClockTime            = curTime;
   mPrevClockTime        = curTime;
   mTotalTimeInvisible   = 0;

   //-- Init basic stuff.
   mpData                = pData;
   mpManager             = pManager;
   mStartTime            = curTime;
   mStartTimeF           = pManager->getTimeFloat();
   mTintColor            = tintColor;

   // rg [8/26/06] - HACK, change this so it accurately calculates the overall lifetime of the emitter if it's non-looping!
   // This should be as low as possible.
   mMaxLifeTime          = (long)pData->mMaxEmitterLife;
   mDeathTime            = curTime + mMaxLifeTime;
   
   mLastUpdateTime       = curTime - TIMEVARIANCE(mpData->mProperties.mInitialUpdate, mpData->mProperties.mInitialUpdateVar);
   
   //trace("Init: LastUpdateTime = %d", mLastUpdateTime);
   
   float rate = VARIANCE(mpData->mProperties.mEmissionRate, mpData->mProperties.mEmissionRateVar);
   //trace("Emission Rate = %4.3f", rate);
   mParticleEmmisionRate = (rate > cFloatCompareEpsilon) ? (1000.0f / rate) : 0.0f;
   //mParticleEmmisionRate = (mpData->mProperties.mEmissionRate > cFloatCompareEpsilon) ? (1.0f / mpData->mProperties.mEmissionRate) : 0.0f;
   
   XMVECTOR emitterPos = matrix.r[3];
   static XMVECTOR bboxOffset = XMVectorSet(1024.0f, 1024.0f, 1024.0f, 0.0f);
   mBoundingMins = XMVectorSubtract(emitterPos, bboxOffset);
   mBoundingMaxs = XMVectorAdd(emitterPos, bboxOffset);
     
   //-- Init initial activity.
   mAdditionalDelay = additionalDormantTime;

   if ((pData->mProperties.mStartDelay + mAdditionalDelay) > 0.0f)
   {
      mState        = eStateDormant;
      mActivityTime = (long)(mAdditionalDelay*1000.0f) + TIMEVARIANCE(pData->mProperties.mStartDelay, mpData->mProperties.mStartDelayVar);
   }
   else
   {
      mState        = eStateActive;
      mActivityTime = TIMEVARIANCE(mpData->mProperties.mEmissionTime, mpData->mProperties.mEmissionTimeVar);
   }

   //-- Init extents data.
   mParticleMaxRadius = pData->mProperties.mUpdateRadius;
   
   mScaleOverride = 1.0f;
   //-- Init other data.
      
   //-- If its never going to emit anything, just stop it now.
   if (!emitsNormally())
      stop();   

   //-- when a trail gets first created add a particle for the start position of the trail.
   if (mpData->mProperties.mType == BEmitterBaseData::eTrail || mpData->mProperties.mType == BEmitterBaseData::eTrailCross)
   {            
      emitParticle(mLastUpdateTime, matrix);
   } 

   //-- set the transform
   setTransform(matrix);
   mLastUpdateTransform = matrix;
   setFlag(eFlagUsedLastTransform, false);
   setFlag(eFlagUsedLastTransform2, false);

   //-- we are visible by default
   setFlag(eFlagVisible, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleEmitter::computeBoundingBox(BVector &minCorner, BVector &maxCorner)
{
   BASSERT(0);
   maxCorner = getBoundingMaxs();
   minCorner = getBoundingMins();
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::renderBegin(void)
{
   ASSERT_RENDER_THREAD
   //-- deprecated 
   //-- but lets leave this here in case we need to do something
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::render(int tileIndex)
{
   ASSERT_RENDER_THREAD

#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDump = false;
   if (bTraceDump)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\particleRender.bin" );
   }
#endif
   
   if (mpVB == NULL || mVertCount == 0)
      return;   
   
   if (getData() == NULL)
      return;

   int texHandle = getData()->mTextures.mDiffuse.mTextureArrayIndex;
   int texHandle2 = getData()->mTextures.mDiffuse2.mTextureArrayIndex;
   int texHandle3 = getData()->mTextures.mDiffuse3.mTextureArrayIndex;
   int intensityTexHandle = getData()->mTextures.mIntensity.mTextureArrayIndex;
   BDEBUG_ASSERT(texHandle != -1);   

   switch (getData()->mProperties.mType)
   {
      case BEmitterBaseData::eBillBoard:
         submitRenderCommand(tileIndex, BParticleRenderer::eRenderBillboard, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         break;
      case BEmitterBaseData::eUpfacing:
         submitRenderCommand(tileIndex, BParticleRenderer::eRenderUpfacing, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         break;
      case BEmitterBaseData::eVelocityAligned:
         submitRenderCommand(tileIndex, BParticleRenderer::eRenderVelocityAligned, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         break;
      case BEmitterBaseData::eOrientedAxialBillboard:
         submitRenderCommand(tileIndex, BParticleRenderer::eRenderAxial, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         break;
      case BEmitterBaseData::eTerrainPatch:
         submitRenderCommand(tileIndex, BParticleRenderer::eRenderTerrainPatch, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         break;
      case BEmitterBaseData::eTrail:
         {
            //if (mpData->mProperties.mTrailUVType == BEmitterBaseData::EmitterTrailUVType::eStretch)
            if (mpData->mProperties.mTrailUVType == BEmitterBaseData::eStretch)
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrailStretch, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
            else
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrail, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         }
         break;         
      case BEmitterBaseData::eTrailCross:
         {
            if (mpData->mProperties.mTrailUVType == BEmitterBaseData::eStretch)
            {
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrailCrossStretchPass1, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrailCrossStretchPass2, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
            }
            else
            {
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrailCrossPass1, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
               submitRenderCommand(tileIndex, BParticleRenderer::eRenderTrailCrossPass2, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
            }
         }
         break;
      case BEmitterBaseData::eBeam:
         {
            BParticleRenderer::eRenderCommand commandType = BParticleRenderer::eRenderBeam;

            if (mpData->mProperties.mBeamAlignmentType == BEmitterBaseData::eBeamAlignToCamera)
               commandType = BParticleRenderer::eRenderBeam;
            else if (mpData->mProperties.mBeamAlignmentType == BEmitterBaseData::eBeamAlignVertical)
               commandType = BParticleRenderer::eRenderBeamVertical;
            else if (mpData->mProperties.mBeamAlignmentType == BEmitterBaseData::eBeamAlignHorizontal)
               commandType = BParticleRenderer::eRenderBeamHorizontal;                        

            submitRenderCommand(tileIndex, commandType, mpVB, mVBStartOffset, mpIB, mIBStartOffset, mVertCount, mIndexCount, texHandle, texHandle2, texHandle3, intensityTexHandle);
         }
         break;

      case BEmitterBaseData::ePFX:
         break;

      default: 
         return; 
         break;
   };
   
  
#ifdef DEBUG_TRACE_RECORD
   if (bTraceDump)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::renderEnd()
{
   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::renderEndOfFrame()
{
   //-- invalidate the VB pointer because it is only good for this frame
   mpVB       = NULL;
   mVertCount = 0;
   mpIB       = NULL;
   mIndexCount = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::returnAllParticles(bool bKillImmediately)
{
   int count = mParticles.getSize();
   if (count > 0 && mpData && mpData->mProperties.mType == BEmitterBaseData::ePFX)
   {
      for (int i = 0; i < count; ++i)
         mParticles[i].releaseEffect(bKillImmediately);
   }
      
   mParticles.clear();
   
   mVertCount = 0;
   mpVB = NULL;
   mpIB = NULL;
   mIndexCount = 0;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::resetBoundingBox()
{   
   const float boxSize = 1024.0f;
   XMVECTOR bboxOffset = XMVectorSet(boxSize, boxSize, boxSize, 0.0f);   
   
   XMVECTOR emitterPos = mTransform.r[3];
   if (mpData && mpData->mProperties.mType == BEmitterBaseData::eBeam)
      emitterPos = XMVectorLerpV(mTransform.r[3], mTransform2.r[3], XMVectorSet(0.5, 0.5f, 0.5f, 0.0f));      
   
   mBoundingMins = XMVectorSubtract(emitterPos, bboxOffset);
   mBoundingMaxs = XMVectorAdd(emitterPos, bboxOffset);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::kill()
{
   returnAllParticles(true);
   
   //-- Reset private data.
   mState                     = eStateKilled;
   mStartTime                 = 0;
   mStartTimeF                = 0.0f;
   mClockTime                 = 0;
   mTotalTimeInvisible        = 0;
   mPrevClockTime             = 0;
   mStopTime                  = 0;
   mLastUpdateTime            = 0;
   mUpdateDeltaTime           = 0;
   mLastPositionTime          = 0;
   mActivityTime              = 0;
   mNextParticleWait          = 0;
   mParticleEmmisionRate      = 0.0f;
   mEmissionDistanceRemaining = 0.0f;
   mOpacity                   = 1.0f;
   mpData                     = NULL;
   mpManager                  = NULL;
   mPriority                  = 1000.0f;
   mTintColor                 = cDWORDWhite;
   mTransform.makeIdentity();
   mTransform2.makeIdentity();
   mLastUpdateTransform.makeIdentity();
   mLastUpdateTransform2.makeIdentity();
   mParticles.clear();   
   mParentEffectIndex = -1;

   //-- Reset extents data.
   mParticleMaxRadius = 0.0f;
   mBoundingMins = XMVectorZero();
   mBoundingMaxs = XMVectorZero();

   mpVB = NULL;
   mpIB = NULL;
   mVBStartOffset = 0;
   mIBStartOffset = 0;
   mVertCount = 0;
   mIndexCount = 0;
   mFlags.zero();
   mDeathTime = 0;
   mMaxLifeTime = 0;
   mTotalTimeInvisible = 0;
   mScaleOverride = 1.0f;
   mParticleMaxRadius = 0.0f;
   mAdditionalDelay = 0.0f;
   mCommandListenerHandle = cInvalidCommandListenerHandle;
   Utils::ClearObj(mBoundSphereVisible, 0);
   Utils::ClearObj(mCompletelyFadedOut, 0);
   Utils::ClearObj(mIsVisibleInViewport, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::stop()
{
   if (mState == eStateStopped)
      return;

   mState    = eStateStopped;
   mStopTime = mLastUpdateTime;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool  BParticleEmitter::tick(long currentTime)
{
   mPrevClockTime = mClockTime;
   mClockTime = currentTime;
   const long deltaTime = Math::Max<long>(0, mClockTime - mPrevClockTime);
   
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   for (int v=0; v<viewportCount; ++v)
   {
      if (getBoundSphereVisible(v) && !getCompletelyFadedOut(v))
         setVisibleInViewport(v, true);
      else
         setVisibleInViewport(v, false);
   }

   bool isBoundingSphereVisibleInAnyViewport = getBoundSphereVisibleInAnyViewport();
   bool isCompletelyFadedOutForAllViewports = getCompletelyFadedOutForAllViewports();
            
   if ((!isBoundingSphereVisibleInAnyViewport) || (isCompletelyFadedOutForAllViewports))
   {
      const bool looping = (mpData) && (mpData->mProperties.mLoop);
      
      if ((!looping) && (mClockTime >= mDeathTime))
      {
         // Tell the caller that this emitter is no longer useful to keep around.
         return true;
      }
          
      mTotalTimeInvisible += deltaTime;
      const uint cInvisibleTimeThreshold = 2000;
      if (mTotalTimeInvisible >= cInvisibleTimeThreshold)
      {
         mTotalTimeInvisible = cInvisibleTimeThreshold;

         //-- if I was marked to die and I have been invisible for too long then
         //-- just kill me completely.
         if ((mState == eStateKilled) || (mState == eStateAboutToDie))
            return true;
                  
         // Emitter hasn't been visible for a while, release all of its memory but don't kill it
         // because it is still active.
         setFlag(eFlagHibernate, true);
         returnAllParticles(false);
         resetBoundingBox();
      }
   }
   else
   {
      mTotalTimeInvisible = 0;
   }
   
   // BTK
   // if the emitter is dead then return true so it is removed from the
   // used list
   if (isDead())
      return true;
   
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::updatePhase1(bool canEmitNewParticles)
{
   BDEBUG_ASSERT( (mState == eStateDormant) || 
      (mState == eStateActive)  || 
      (mState == eStateStopped) ||
      (mState == eStateAboutToDie));

   BASSERT(mpManager && mpData);
   if (!mpManager || !mpData)
      return;

   //-- reset the rendering data 
   mpVB = NULL;
   mVertCount = 0;
   mVBStartOffset = 0;

   mpIB = NULL;
   mIndexCount = 0;
   mIBStartOffset = 0;

   // jce [11/4/2008] -- Mark that we've updated using the currently stored last transforms.
   setFlag(eFlagUsedLastTransform, true);
   setFlag(eFlagUsedLastTransform2, true);
   
   //-- if our time gets out of whack for some reason then fix it
   if (mLastUpdateTime > mClockTime)
      mLastUpdateTime = mClockTime;

   //-- Compute the time delta.
   mUpdateDeltaTime = static_cast<long>(Math::Max<long>(0, mClockTime - mLastUpdateTime) * mpManager->getTimeSpeed());
   if (mUpdateDeltaTime <= 0)
      return;

   mLastUpdateTime  = mClockTime;
   //trace("LastUpdateTime = %d", mClockTime);
        
   //-- if we got this far and our state is hibernate.  That means we just became visible after being invisible
   if (getFlag(eFlagHibernate))
   {
      //-- return all the particles and start fresh next frame
      setFlag(eFlagHibernate, false);
      returnAllParticles(false);
      resetBoundingBox();
      mUpdateDeltaTime =0;      
      return;
   }

   if (mpData->mProperties.mType == BEmitterBaseData::eBillBoard || 
       mpData->mProperties.mType == BEmitterBaseData::eUpfacing  ||
       mpData->mProperties.mType == BEmitterBaseData::eVelocityAligned || 
       mpData->mProperties.mType == BEmitterBaseData::eOrientedAxialBillboard ||
       mpData->mProperties.mType == BEmitterBaseData::eTerrainPatch)
   {
      cleanupDeadParticlesUnsorted();
   }
   
   //-- Emit new particles.
   if (canEmitNewParticles)
      processEmission(mUpdateDeltaTime);
#ifdef DEBUG_UPDATE_TIMER
   float emissionTime = timer.getElapsedSeconds() * 1000.0f;
   if (!mParticles.empty())
      trace("Update: %f Emission %f\n", updateTime, emissionTime);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::updatePhase2(IDirect3DVertexBuffer9* pVB, void* pVBBuffer, uint vbStartOffset, IDirect3DIndexBuffer9* pIB, void* pIBBuffer, uint ibStartOffset)
{
   BDEBUG_ASSERT(mpData!=NULL);
   BDEBUG_ASSERT(mpManager!=NULL);

#ifdef DEBUG_UPDATE_TIMER   
   timer.stop();
   timer.start();
#endif

   mpVB = pVB;
   mVBStartOffset = vbStartOffset;
   mpIB = pIB;
   mIBStartOffset = ibStartOffset;

   //-- Update existing particles.
   updateParticles(mUpdateDeltaTime, pVBBuffer, pIBBuffer);

#ifdef DEBUG_UPDATE_TIMER
   float updateTime = timer.getElapsedSeconds() * 1000.0f;
   timer.stop();
   timer.start();
#endif
}

//----------------------------------------------------------------------------
//  releaseParticles()
//  This function simply returns particles that aren't being used anymore to
//  the particle manager.  It only needs to be called when the normal
//  update() isn't called.
//----------------------------------------------------------------------------
void BParticleEmitter::releaseParticles(long currentTime)
{
   BASSERT(mpData);
   if (!mpData)
      return;

   //-- Compute the time delta.
   long deltaTime = (currentTime - mLastUpdateTime);
   if (deltaTime <= 0)
      return;

   //-- If the update interval is longer than the max particle life, we know
   //-- we can release all our particles.
   float maxLife = mpData->mProperties.mParticleLife * (1.0f + mpData->mProperties.mParticleLifeVar) * 1000.0f;
   if (deltaTime > (long)maxLife)
   {
      returnAllParticles(false);
      resetBoundingBox();
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::setTransform(BMatrix mtx)
{   
   //-- Update the tranform.
   // jce [11/4/2008] -- Only do this if we have already used the previous transform in an update.  This
   // protects multiple calls to setTransform before an update from wiping out the history.
   if(getFlag(eFlagUsedLastTransform))
   {
      mLastUpdateTransform = mTransform; //-- cache the old transform
      
      // Remember that we've got a new previous transform that we haven't used yet.
      setFlag(eFlagUsedLastTransform, false);
   }
   
   mTransform = mtx;

   /*
   BVector pos;
   mtx.getTranslation(pos);
   trace("   %p setTransform %0.2f %0.2f %0.2f", this, pos.x, pos.y, pos.z);
   */

   if( mpData && mpData->mProperties.mIgnoreRotation )
   {
      BVector tempvec;
      mtx.getTranslation(tempvec);
      mTransform.makeIdentity();
      mTransform.setTranslation( tempvec );
   }

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::setSecondaryTransform(BMatrix mtx)
{
   //-- Update the tranform.
   // jce [11/4/2008] -- Only do this if we have already used the previous transform in an update.  This
   // protects multiple calls to setTransform before an update from wiping out the history.
   if(getFlag(eFlagUsedLastTransform2))
   {
      mLastUpdateTransform2 = mTransform; //-- cache the old transform

      // Remember that we've got a new previous transform that we haven't used yet.
      setFlag(eFlagUsedLastTransform2, false);
   }
   
   mTransform2 = mtx;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::setTintColor(DWORD color)
{
   mTintColor = color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleEmitter::isDead()
{
   if (mState == eStateKilled)
      return true;

   if (mState == eStateStopped || mState == eStateAboutToDie)
   {
      if (mParticles.getSize() == 0)
      {
         mState = eStateKilled;
         return true;
      }
   }
      
   return false;
}

//============================================================================
//  DATA INTERFACE
//============================================================================
//============================================================================
//  CALLBACK INTERFACE
//============================================================================

//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleEmitter::emitsNormally()
{
   // jce 8/31/2001 -- fixed so this doesn't crash.
   if(!mpData)
   {
      BASSERT(0);
      return(false);
   }

   if (mpData->mProperties.mEmissionTime > cFloatCompareEpsilon)
      return true;

   if ((mpData->mProperties.mLoop) && (mpData->mProperties.mStartDelay < cFloatCompareEpsilon))
      return true;

   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float BParticleEmitter::computeParticleTextureArrayLookup(int textureArrayIndex)
{  
   BDEBUG_ASSERT(mpData!=NULL);
   BDEBUG_ASSERT(mpData->mTextures.mDiffuse.mStages.getSize() > 0);   

   if (mpData->mTextures.mDiffuse.mStages.getSize() == 1)
      return 0.0f;

   float n = (float) mpData->mTextures.mDiffuse.mStages.getSize();
   float zLookup = (1.0f / (2.0f * n)) + (((float) textureArrayIndex) / n);
   return zLookup;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD BParticleEmitter::choosePaletteColor(long index)
{
   //-- Validate.
   if (mpData->mColor.mPallette.getSize() <= 0)
      return 0;

   index %= mpData->mColor.mPallette.getSize();;
   return mpData->mColor.mPallette[index].mColor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::processEmission(long updateTime)
{
   if (mState==eStateAboutToDie || mState == eStateKilled)
      return;

   BASSERT(mpData);
   if (!mpData)
      return;
   
   //-- Make sure we emit normally.
   if (!emitsNormally())
      return;

   //-- No more emission if we are stopped.
   if (mState == eStateStopped)
      return;

   //-- If this particle system ALWAYS emits, life is easy.
   if ((mState == eStateActive) && mpData->mProperties.mLoop && (mpData->mProperties.mLoopDelay < cFloatCompareEpsilon))
   {
      emitParticles(updateTime);
      return;
   }

   //-- If we are dormant, lets catch up to the next active time.
   long trueTime    = mLastUpdateTime;
   long currentTime = mLastUpdateTime - updateTime;
   if (mState == eStateDormant)
   {
      if (updateTime > mActivityTime)
      {
         //-- Update times.
         updateTime  -= mActivityTime;
         currentTime += mActivityTime;

         //-- Change state.
         mState        = eStateActive;
         mActivityTime = TIMEVARIANCE(mpData->mProperties.mEmissionTime, mpData->mProperties.mEmissionTimeVar);

         //-- We could be an "always active" emitter that is just leaving its
         //-- initial dormancy.  If so, we don't want to enter the code below.
         //-- Lets short circuit here.
         if (mpData->mProperties.mLoop && (mpData->mProperties.mLoopDelay < cFloatCompareEpsilon))
         {
            emitParticles(updateTime);
            return;
         }
      }
      else
      {
         //-- Update times.
         mActivityTime -= updateTime;
         return;
      }
   }

   //-- Process an active/dormant cycle.
   while (updateTime > 0)
   {
      //-- Process the active portion.
      if (updateTime > mActivityTime)
      {
         //-- Update times.
         updateTime  -= mActivityTime;
         currentTime += mActivityTime;

         //-- Emit.
         mLastUpdateTime = currentTime;
         emitParticles(mActivityTime);
         mLastUpdateTime = trueTime;

         //-- Change state.
         mState        = eStateDormant;
         mActivityTime = TIMEVARIANCE(mpData->mProperties.mLoopDelay, mpData->mProperties.mLoopDelayVar);

         //-- If we don't loop, we stop the emitter.
         if (!mpData->mProperties.mLoop)
         {
            stop();
            return;
         }
      }
      else
      {
         //-- Update times.
         mActivityTime  -= updateTime;
         mLastUpdateTime = trueTime;

         //-- Emit.
         emitParticles(updateTime);
         return;
      }

      //-- Process the dormant time
      if (updateTime > mActivityTime)
      {
         //-- Update times.
         updateTime  -= mActivityTime;
         currentTime += mActivityTime;

         //-- Change state.
         mState        = eStateActive;
         mActivityTime = TIMEVARIANCE(mpData->mProperties.mEmissionTime, mpData->mProperties.mEmissionTimeVar);
      }
      else
      {
         //-- Update times.
         mActivityTime -= updateTime;
         return;
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::emitParticles(long updateTime)
{
   BDEBUG_ASSERT(mpManager && mpData);


   /*
   if (gpRenderDebugPrimitives)
   {
      BVector pos, fwd, up, right;
      mTransform.getTranslation(pos);
      mTransform.getForward(fwd);
      mTransform.getUp(up);
      mTransform.getRight(right);
      
      gpRenderDebugPrimitives->addDebugAxis(pos, right, up, fwd, 5.0f);
   }
   */


#ifndef BUILD_FINAL
   BTimer timer;
#endif
   if (mpData->mProperties.mType == BEmitterBaseData::eBeam)
   {      
#ifndef BUILD_FINAL
   timer.start();
#endif
      emitBeamParticles(updateTime);
#ifndef BUILD_FINAL
   timer.stop();
   mpManager->mWorkerStats.mEmissionTime += timer.getElapsedSeconds();
   mpManager->mWorkerStats.mEmissionTimeBeam += timer.getElapsedSeconds();
#endif
      return;
   }

   if (mpData->mProperties.mType == BEmitterBaseData::eTrail || mpData->mProperties.mType == BEmitterBaseData::eTrailCross)
   {   
#ifndef BUILD_FINAL
   timer.start();
#endif
      emitTrailParticles(updateTime);
#ifndef BUILD_FINAL
   timer.stop();
   mpManager->mWorkerStats.mEmissionTime += timer.getElapsedSeconds();
   mpManager->mWorkerStats.mEmissionTimeTrail += timer.getElapsedSeconds();
#endif   
      return;
   }

   //-- If we don't emit, we don't need to be here.
   if ((mParticleEmmisionRate < cFloatCompareEpsilon))
      return;

#ifndef BUILD_FINAL
   timer.start();
#endif

#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDump = false;
   if (bTraceDump)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\particleEmit.bin" );
   }
#endif

   float oneOverUpdateTime = 1.0f / updateTime;
   float alpha = 1.0f;
   XMMATRIX curMatrix = mTransform;
      
   //-- compute the when the last update occurred because 
   //-- mLastUpdateTime has already been set to the current clock time;
   //-- we assume that we started emitting right after the last update occurred so
   //-- in the view of the emitter the current time starts from when the last update 
   //-- occurred
   float currentTime     = (float) (mLastUpdateTime - updateTime);
   float updateTimeLeft  = (float) updateTime;
   float accumulatedTime = 0.0f;   
   
   //-- do we have enough time during this update to spit out a particle?
   BParticleSystemParticle* pParticle = NULL;   
   while (updateTimeLeft > mNextParticleWait)
   {
      alpha = accumulatedTime * oneOverUpdateTime;
            
      //-- subtract the waiting time we had left from the update time we had coming in
      updateTimeLeft  -= mNextParticleWait;
      currentTime     += mNextParticleWait;
      accumulatedTime += mNextParticleWait;

      curMatrix.r[3] = XMVectorLerp(mLastUpdateTransform.r[3], mTransform.r[3], alpha);

      pParticle = emitParticle((long) currentTime, curMatrix);
      if (pParticle)
      {
         //-- reinit our particle wait to how emission rate that was determined upon start up
         mNextParticleWait = mParticleEmmisionRate; 
      }
      else
      {
         //-- we couldn't emit because we reached out max possibly -- set the time to zero so we emit a particle as soon
         //-- as we are allowed to and bail out
         mNextParticleWait = updateTimeLeft;
         break;
      }
      //trace("Emission Wait = %u", mNextParticleWait);
   }

   mNextParticleWait = Math::ClampLow((mNextParticleWait - updateTimeLeft), 0.0f);
   //trace("Rate = %4.3f, # Emitted= %u, Total =%u", mParticleEmmisionRate/1000.0f,  particlesEmittedThisFrame, mParticles.getSize());

#ifndef BUILD_FINAL
   timer.stop();
   mpManager->mWorkerStats.mEmissionTime += timer.getElapsedSeconds();
   mpManager->mWorkerStats.mEmissionTimeNormal += timer.getElapsedSeconds();
#endif

#ifdef DEBUG_TRACE_RECORD
   if (bTraceDump)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }
#endif


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::emitBeamParticles(long updateTime)
{
   //-- for beams we only emit one particle
   if (mParticles.isEmpty())
   {
      int age = mLastUpdateTime + updateTime;
      emitParticle(age, mTransform);
      return;
   }
   
   //-- if the particle is supposed to die and it is a looping beam particle then just reinit the current particle
   //-- so we don't have a one frame lag
   int currentTime = mpManager->getTime();   
   if (mpData->mProperties.mLoop && (currentTime >= mParticles[0].mDeathTime))
   {
      //-- clear the current one
      mParticles.clear();
      //-- and re-create it and init it
      int age = mLastUpdateTime + updateTime;
      emitParticle(age, mTransform);
      return;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::emitTrailParticles(long updateTime)
{   
   if (mpData->mProperties.mTrailEmissionType == BEmitterBaseData::eEmitByLength)
   {
      emitTrailParticlesByLength(updateTime);   
   }
   else if (mpData->mProperties.mTrailEmissionType == BEmitterBaseData::eEmitByTime)
   {
      emitTrailParticlesByTime(updateTime);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::emitTrailParticlesByLength(long updateTime)
{
   //-- The emitter is active, so emit particles.
   long currentTime    = mLastUpdateTime - updateTime;

   //-- if this trail has no segments yet then add one.
   if (mParticles.getSize() <= 0)
   {
      BParticleSystemParticle* pParticle = emitParticle(currentTime, mTransform);
      pParticle;
      //BDEBUG_ASSERT(pParticle!=NULL);      
      return;
   }

   //BVector curEmitterPosition;
   //mTransform.getTranslation(curEmitterPosition);
   
   float segmentLength = mpData->mProperties.mTrailSegmentLength;
   if (segmentLength <= cFloatCompareEpsilon)
      segmentLength = 1.0f;

   BDEBUG_ASSERT(mParticles.getSize() > 0);

   //-- compute how many segments we are going to add
   //-- get the last trail segment   
   BParticleSystemParticle& tailParticle = mParticles.back();
   XMVECTOR tailPos = tailParticle.mPosition;

    //curEmitterPosition.distance(tailParticle.mPosition);
   XMVECTOR distanceV = XMVector3Length(XMVectorSubtract(mTransform.r[3], tailPos));
   float distanceTraveledThisUpdate = distanceV.x;
   
   int numSegments = Math::FloatToIntRound(distanceTraveledThisUpdate/segmentLength);
   //-- add new segments to the trail
   BMatrix matrix = mTransform;
   BVector newPos;
   float alpha;
   float oneOverNumSegments = 1.0f / (float) numSegments;
   int age;
   for (int j=0; j<numSegments; ++j)
   {      
      alpha = (float)(j+1.0f) * oneOverNumSegments;
      age = (int) (mLastUpdateTime + (updateTime*alpha));
            
      matrix.r[3] = XMVectorLerp(tailPos, mTransform.r[3], alpha);

      emitParticle(age, matrix);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::emitTrailParticlesByTime(long updateTime)
{
   //-- The emitter is active, so emit particles.
   long currentTime    = mLastUpdateTime - updateTime;
   
   //-- if this trail has no segments yet then add one.
   if (mParticles.getSize() <= 0)
   {
      BParticleSystemParticle* pParticle = emitParticle(currentTime, mTransform);
      pParticle;
      BDEBUG_ASSERT(pParticle!=NULL);      
      return;
   }

   BDEBUG_ASSERT(mParticles.getSize() > 0);

   //-- compute how many segments we are going to add
   //-- get the last trail segment   
   BParticleSystemParticle& tailParticle = mParticles.back();
   BVector tailPosition = tailParticle.mPosition;
   int     tailStartTime = tailParticle.mBirthTime;

   int numSegments = 0;
   BDEBUG_ASSERT(mpData->mProperties.mEmissionRate > 0);
   float updateTimeInSeconds = (mLastUpdateTime-tailStartTime) * 0.001f;
   float newRate = 1.0f / VARIANCE(mpData->mProperties.mEmissionRate, mpData->mProperties.mEmissionRateVar);
   numSegments = Math::FloatToIntRound(updateTimeInSeconds / newRate);

   if (numSegments <= 0)
      return;

   //-- add new segments to the trail
   BMatrix matrix = mTransform;
   BVector curEmitterPosition;
   mTransform.getTranslation(curEmitterPosition);
   BVector newPos;
   int age;
   float alpha;
   float oneOverNumSegments = 1.0f / ((float) numSegments);
   for (int j=0; j<numSegments; ++j)
   {
      //float alpha = (float) (j+1.0f) / ((float) (numSegments));
      alpha = (float) (j+1.0f) * oneOverNumSegments;

      newPos.lerpPosition(alpha, tailPosition, curEmitterPosition);
      matrix.setTranslation(newPos);
      age = (int) (mLastUpdateTime + (updateTime*alpha));
      //trace("tail: (%4.3f,%4.3f,%4.3f) emitterPos:(%4.3f,%4.3f,%4.3f)", tailPosition.x, tailPosition.y, tailPosition.z, curEmitterPosition.x, curEmitterPosition.y, curEmitterPosition.z);
      //trace("newPos (%4.3f,%4.3f,%4.3f) alpha (%4.3f)", newPos.x, newPos.y, newPos.z, alpha);
      emitParticle(age, matrix);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleSystemParticle* BParticleEmitter::emitParticle(long curTime, BMatrix curEmitterMatrix)
{
   BDEBUG_ASSERT(mpManager && mpData);
   
   //-- Make sure we are allowed to emit another particle.
   if ((long)mParticles.getSize() >= mpData->mProperties.mMaxParticles)
      return NULL;
         
   BParticleSystemParticle* pParticle = mParticles.pushBackNoConstruction(1);   
   BDEBUG_ASSERT(pParticle);

   //-- Init it.
   pParticle->init(this, RANDOM(PS_NUM_RAND_FLOATS), curTime, curEmitterMatrix);

   //-- Return it.
   return pParticle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::updateParticles(long updateTime, void* __restrict pVBBuffer, void* pIBBuffer)
{
   if(!mpData || !mpManager)
   {
      BDEBUG_ASSERT(0);
      return;
   }

   //-- Handle 0 particle case.
   if (mParticles.getSize() == 0)
   {
      return;
   }

#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDump = false;
   if (bTraceDump)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\particleUp.bin" );
   }
#endif
   
   XMVECTOR fUp = __vpermwi(XMConvertVectorUIntToFloat(XMLoadScalar(&updateTime), 0), 0) * .001f;
   //trace("UpdateTime: %u, (%4.3f)", updateTime, fUp.x);

#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDumpCollision = false;
   if (bTraceDumpCollision)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\particleCollision.bin" );
   }
#endif   

   if (mpData->mProperties.mCollisionDetectionTerrain)
   {
      updateCollisionsVMX(fUp);
   }

   updateMagnetsVMX(fUp);

#ifdef DEBUG_TRACE_RECORD
   if (bTraceDumpCollision)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDumpCollision = false;
   }
#endif


   if (mpData->mProperties.mType==BEmitterBaseData::ePFX)
   {
#ifndef BUILD_FINAL
      BTimer timer;
      timer.start();
#endif 

      updatePFXParticlesVMX(fUp);

#ifndef BUILD_FINAL
      timer.stop();
      mpManager->mWorkerStats.mVMXUpdateTime = timer.getElapsedSeconds();
#endif   
   }
   else if (mpData->mProperties.mType==BEmitterBaseData::eBeam)
   {

#ifndef BUILD_FINAL
      BTimer timer;
      timer.start();
#endif 
                  
      updateBeamParticlesVMX(fUp, pVBBuffer, mVertCount);

#ifndef BUILD_FINAL
      timer.stop();
      mpManager->mWorkerStats.mVMXUpdateTime = timer.getElapsedSeconds();
#endif

   }
   //-- trails don't do anything like this
   else if (mpData->mProperties.mType==BEmitterBaseData::eTrail || mpData->mProperties.mType==BEmitterBaseData::eTrailCross)
   {
#ifndef BUILD_FINAL
      BTimer timer;
      timer.start();
#endif 
      
      updateTrailParticlesLVMX(fUp, pVBBuffer, mVertCount);

#ifndef BUILD_FINAL
      timer.stop();
      mpManager->mWorkerStats.mVMXUpdateTime = timer.getElapsedSeconds();
#endif
      cleanupDeadParticlesSorted();
   }
   else 
   {
#ifndef BUILD_FINAL
      BTimer timer;
      timer.start();
#endif 
      
      updateParticlesVMX(fUp, pVBBuffer, mVertCount);
      
      if (mpData->mProperties.mSortParticles && (mpData->mProperties.mBlendMode == BEmitterBaseData::eAlphaBlend))
      {
         sortParticlesVMX(pVBBuffer, mVertCount, pIBBuffer, mIndexCount);         
      }

#ifndef BUILD_FINAL
      timer.stop();
      mpManager->mWorkerStats.mVMXUpdateTime = timer.getElapsedSeconds();
#endif
   }
   
#ifdef DEBUG_TRACE_RECORD
   if (bTraceDump)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::sortParticlesVMX(const void* __restrict pBuffer, int vertCount, void* pIB, int& indexCount)
{      
   BMatrix viewMatrix = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToView);

   BPVertexPT* pVB = (BPVertexPT*)pBuffer;
   BDynamicParticleArray<float> zValues;
   zValues.resize(vertCount);
   XMVECTOR viewPos;

   for (uint i = 0; i < mParticles.getSize(); ++i)
   {            
      viewPos = XMVector3Transform(mParticles[i].mPosition, viewMatrix);
      zValues[i] = viewPos.z;
      pVB++;
   }

   BRadixSorter sorter;
   sorter.sort(zValues.getData(), zValues.getNumber());


   DWORD* pIndices = sorter.getIndices();

   WORD* pIndexBuffer = (WORD*) pIB;
   uint curIndex = 0;   
   for (int j = zValues.getNumber()-1; j >= 0; --j)
   {      
      pIndexBuffer[curIndex]   = (WORD) (pIndices[j] * 4);
      _WriteBarrier();
      pIndexBuffer[curIndex+1] = (WORD) (pIndices[j] * 4)+1;
      _WriteBarrier();
      pIndexBuffer[curIndex+2] = (WORD) (pIndices[j] * 4)+2;
      _WriteBarrier();
      pIndexBuffer[curIndex+3] = (WORD) (pIndices[j] * 4)+3;
      _WriteBarrier();

      curIndex+= 4;
   }

   indexCount = vertCount * 4;
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updateMagnetsVMX(XMVECTOR fUpdateTime)
{
#ifdef DEBUG_VMX_UPDATE
   BTimer timer;
   timer.start();
#endif DEBUG_VMX_UPDATE
   int magnetCount = mpData->mMagnets.getSize();
   if (magnetCount == 0)
      return;
   
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 5)
      prefetchState = Utils::BeginPrefetch(&mParticles[4], 0);
   
   uint numParticles = mParticles.getSize();
   BParticleSystemParticle* pParticle = NULL;
   
   
   XMVECTOR magnetPos;
   XMVECTOR forceV;
   XMVECTOR rotationalForceV;
   XMVECTOR radiusV;
   XMVECTOR heightV;

   XMVECTOR distanceV;
   XMVECTOR directionV;   
   XMVECTOR dampeningV;
   XMVECTOR attractorForceV;
   XMVECTOR attractorRotationForceV;
   XMVECTOR attractorDampeningV;
   XMVECTOR turbulencV;
   XMVECTOR turbulenceInfluenceV;

   XMVECTOR alphaV;
   XMVECTOR axisV;
   XMVECTOR pointA;
   XMVECTOR pointB;
   XMVECTOR yAxisVector = XMVectorSet(0.0f,1.0f,0.0f, 0.0f);   
   XMVECTOR pointOnLine;

   XMMATRIX rotationMatrix;
   
   XMVECTOR oneV  = XMVectorSet(1,1,1,1);
   XMVECTOR zeroV = XMVectorZero();
   XMVECTOR selectControlXYZ = XMVectorSelectControl(0,0,0,1);
   XMVECTOR tV;
   XMVECTOR localAxis;
   XMVECTOR emitterPos = XMVectorSelect(mTransform.r[3], zeroV, selectControlXYZ);
      
   int randomIndex;
   BMagnetData* pMagnetData = NULL;
   for (int m = 0; m < magnetCount; ++m)
   {
      pMagnetData = mpData->mMagnets[m];
      BDEBUG_ASSERT(pMagnetData);

      radiusV              = XMVectorSplatX(XMLoadScalar(&pMagnetData->mRadius));
      heightV              = XMVectorSplatX(XMLoadScalar(&pMagnetData->mHeight));
      forceV               = XMVectorMultiply(XMVectorSplatX(XMLoadScalar(&pMagnetData->mForce)), fUpdateTime);
      rotationalForceV     = XMVectorMultiply(XMVectorSplatX(XMLoadScalar(&pMagnetData->mRotationalForce)), fUpdateTime);
      dampeningV           = XMVectorMultiply(XMVectorSplatX(XMLoadScalar(&pMagnetData->mDampening)), fUpdateTime);
      turbulenceInfluenceV = XMVectorMultiply(XMVectorSplatX(XMLoadScalar(&pMagnetData->mTurbulence)), fUpdateTime);

      if (mpData->mProperties.mTiedToEmitter)
         magnetPos = pMagnetData->mPosOffset;
      else 
         magnetPos = XMVectorAdd(emitterPos, pMagnetData->mPosOffset);

      if (pMagnetData->mType == BMagnetData::eSphere)
      {
         for (uint i = 0; i < numParticles; i++)
         {         
            pParticle = &mParticles[i];

            directionV = XMVectorSubtract(magnetPos, pParticle->mPosition);         
            distanceV = XMVector3Length(directionV);            

            // -- is it outside the sphere?
            if (XMVector3Greater(distanceV, radiusV))
               continue;

            // linear attractor force
            alphaV = XMVectorMultiply(distanceV, XMVectorReciprocal(radiusV));
            alphaV = XMVectorClamp(alphaV, zeroV, oneV);
            attractorForceV = XMVectorMultiply(forceV, alphaV);
            attractorForceV = XMVectorMultiply(XMVector3Normalize(directionV), attractorForceV);
            pParticle->mVelocity = XMVectorAdd(pParticle->mVelocity, XMVectorSelect(attractorForceV, zeroV, selectControlXYZ));
                                                                       
            //-- dampening
            attractorDampeningV  = XMVectorNegate(XMVectorMultiply(pParticle->mVelocity, dampeningV));     
            pParticle->mVelocity = XMVectorAdd( pParticle->mVelocity, XMVectorSelect(attractorDampeningV, zeroV, selectControlXYZ));                  

            //-- turbulence
            randomIndex = RANDOM(PS_NUM_RAND_FLOATS-3);
            turbulencV = XMVectorSet(gPSManager.mRandomFloats[randomIndex], gPSManager.mRandomFloats[randomIndex+1], gPSManager.mRandomFloats[randomIndex+2], 0.0f);               
            pParticle->mVelocity = XMVectorMultiplyAdd(turbulenceInfluenceV, turbulencV, pParticle->mVelocity);
         }
      }
      else if (pMagnetData->mType == BMagnetData::eCylinder)
      {         
         axisV = XMVectorMultiply(yAxisVector, heightV);
         axisV = XMVector3TransformNormal(axisV, pMagnetData->mRotation);

         if (!mpData->mProperties.mTiedToEmitter)
            axisV = XMVector3TransformNormal(axisV, mTransform);

         pointA = magnetPos;
         pointB = XMVectorAdd(magnetPos, axisV);    

#if 0
         if (gpRenderDebugPrimitives)
         {
            if (mpData->mProperties.mTiedToEmitter)
               gpRenderDebugPrimitives->addDebugLine(XMVectorAdd(pointA, XMVectorSelect(mTransform.r[3], zeroV, selectControlXYZ)), XMVectorAdd(pointB, XMVectorSelect(mTransform.r[3], zeroV, selectControlXYZ)), cDWORDPurple, cDWORDPurple, BDebugPrimitives::cCategoryParticles, -1.0f);            
            else 
               gpRenderDebugPrimitives->addDebugLine(pointA, pointB, cDWORDPurple, cDWORDPurple, BDebugPrimitives::cCategoryParticles, -1.0f);
         }
#endif

         for (uint i = 0; i < numParticles; i++)
         {         
            pParticle = &mParticles[i];                        
            
            BVMXIntersection::closestPointOnLineSegmentTheta(pParticle->mPosition, pointA, pointB, localAxis, tV);

            if (XMVector3Greater(tV, oneV))
               continue;
            else if (XMVector3Less(tV, zeroV))
               continue;

            // passed the height test
            tV = XMVectorClamp(tV, XMVectorZero(), oneV);

            // compute the point on the line
            pointOnLine = XMVectorMultiplyAdd(localAxis, tV, pointA);

            directionV = XMVectorSubtract(pointOnLine, pParticle->mPosition);

#if 0
            if (gpRenderDebugPrimitives)
               gpRenderDebugPrimitives->addDebugLine(pParticle->mPosition, pointOnLine, cDWORDPurple, cDWORDPurple, BDebugPrimitives::cCategoryParticles, -1.0f);
#endif

            distanceV = XMVector3Length(directionV);
            
            // test if its inside the radius
            if (XMVector3Greater(distanceV, radiusV))
               continue;          

            // the particle is inside the cylinder -- apply all forces
                        
            // linear attractor force
            alphaV = XMVectorMultiply(distanceV, XMVectorReciprocal(radiusV));
            alphaV = XMVectorClamp(alphaV, zeroV, oneV);
            attractorForceV = XMVectorMultiply(forceV, alphaV);
            attractorForceV = XMVectorMultiply(XMVector3Normalize(directionV), attractorForceV);
            pParticle->mVelocity = XMVectorAdd(pParticle->mVelocity, XMVectorSelect(attractorForceV, zeroV, selectControlXYZ));

            // rotation
            attractorRotationForceV = XMVector3Normalize(XMVector3Cross(axisV, directionV));
            attractorRotationForceV = XMVectorMultiply(rotationalForceV, attractorRotationForceV);                  
            pParticle->mVelocity = XMVectorAdd( pParticle->mVelocity, XMVectorSelect(attractorRotationForceV, zeroV, selectControlXYZ));
                                                
            // dampening
            attractorDampeningV  = XMVectorNegate(XMVectorMultiply(pParticle->mVelocity, dampeningV));               
            pParticle->mVelocity = XMVectorAdd( pParticle->mVelocity, XMVectorSelect(attractorDampeningV, zeroV, selectControlXYZ));

            // turbulence
            randomIndex = RANDOM(PS_NUM_RAND_FLOATS-3);
            turbulencV = XMVectorSet(gPSManager.mRandomFloats[randomIndex], gPSManager.mRandomFloats[randomIndex+1], gPSManager.mRandomFloats[randomIndex+2], 0.0f);               
            pParticle->mVelocity = XMVectorMultiplyAdd(turbulenceInfluenceV, turbulencV, pParticle->mVelocity);
         }
      }
   }

#ifdef DEBUG_VMX_UPDATE   
   double totalTime = timer.getElapsedSeconds();
   double timePerParticle = totalTime / numParticles;
   double cycles = timePerParticle * 3200000000;
   trace("Collision VMX  Particles Processed: %d, total time: %2.6f, cycles per particle: %4.3f", numParticles, totalTime, cycles);
#endif
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updateCollisionsVMX(XMVECTOR fUpdateTime)
{
#ifdef DEBUG_VMX_UPDATE
   BTimer timer;
   timer.start();
#endif DEBUG_VMX_UPDATE

   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 5)
      prefetchState = Utils::BeginPrefetch(&mParticles[4], 0);
   
   //-- Dampen the velocity.   
   XMVECTOR vEnergyLoss;
   XMVECTOR vEnergyLoss2;
   XMVECTOR vEnergyLoss3;
   XMVECTOR vEnergyLoss4;

   XMVECTOR vEnergyLossValue         = XMLoadScalar(&mpData->mProperties.mCollisionEnergyLoss);
   XMVECTOR vEnergyLossValueVariance = XMLoadScalar(&mpData->mProperties.mCollisionEnergyLossVar);
   XMVECTOR randomNumberV;
   XMVECTOR randomNumberV2;
   XMVECTOR randomNumberV3;
   XMVECTOR randomNumberV4;
               
   float worldY = 0.0f;
   float worldY2 = 0.0f;
   float worldY3 = 0.0f;
   float worldY4 = 0.0f;
   float collisionOffset = mpData->mProperties.mCollisionOffset;
   uint numParticles = mParticles.getSize();
   BParticleSystemParticle* pParticle = NULL;
   BParticleSystemParticle* pParticle2= NULL;
   BParticleSystemParticle* pParticle3= NULL;
   BParticleSystemParticle* pParticle4= NULL;

   uint index = 0;
   for (uint i = 0; i < numParticles/4; i++)
   {          
      index = i * 4;

      if ((index + 4) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[index + 4], 0);

      pParticle  = &mParticles[index];
      pParticle2 = &mParticles[index+1];
      pParticle3 = &mParticles[index+2];
      pParticle4 = &mParticles[index+3];

      worldY  = gTerrainSimRep.getHeightFast(pParticle->mPosition) + collisionOffset;
      worldY2 = gTerrainSimRep.getHeightFast(pParticle2->mPosition)+ collisionOffset;
      worldY3 = gTerrainSimRep.getHeightFast(pParticle3->mPosition)+ collisionOffset;
      worldY4 = gTerrainSimRep.getHeightFast(pParticle4->mPosition)+ collisionOffset;

      randomNumberV  = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV2 = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle2->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV3 = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle3->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV4 = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle4->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);

      vEnergyLoss  = XMVectorSubtract(gVectorOne, XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV,  vEnergyLossValue, vEnergyLossValueVariance)));
      vEnergyLoss2 = XMVectorSubtract(gVectorOne, XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV2, vEnergyLossValue, vEnergyLossValueVariance)));
      vEnergyLoss3 = XMVectorSubtract(gVectorOne, XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV3, vEnergyLossValue, vEnergyLossValueVariance)));
      vEnergyLoss4 = XMVectorSubtract(gVectorOne, XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV4, vEnergyLossValue, vEnergyLossValueVariance)));

      if (pParticle->mPosition.y < worldY)
      {
         pParticle->mPosition.y = worldY;
         pParticle->mVelocity = XMVectorMultiply(XMVector3Reflect(pParticle->mVelocity, cYAxisVector), vEnergyLoss);         
      }
      if (pParticle2->mPosition.y < worldY2)
      {
         pParticle2->mPosition.y = worldY2;
         pParticle2->mVelocity = XMVectorMultiply(XMVector3Reflect(pParticle2->mVelocity, cYAxisVector), vEnergyLoss2);         
      }
      if (pParticle3->mPosition.y < worldY3)
      {
         pParticle3->mPosition.y = worldY3;
         pParticle3->mVelocity = XMVectorMultiply(XMVector3Reflect(pParticle3->mVelocity, cYAxisVector), vEnergyLoss3);         
      }
      if (pParticle4->mPosition.y < worldY4)
      {
         pParticle4->mPosition.y = worldY4;
         pParticle4->mVelocity = XMVectorMultiply(XMVector3Reflect(pParticle4->mVelocity, cYAxisVector), vEnergyLoss4);         
      }
   }

   int particlesLeft = numParticles & 3;
   // rg [3/21/08] - This looked wrong - this code was accessing the very beginning of the array. I'm adding firstParticle here.
   const uint firstParticle = numParticles & ~3;
   for (int j = 0; j < particlesLeft; j++)
   {          
      pParticle = &mParticles[firstParticle + j];

      worldY = gTerrainSimRep.getHeightFast(pParticle->mPosition) + collisionOffset;

      randomNumberV = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      vEnergyLoss = XMVectorSubtract(gVectorOne,XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV, vEnergyLossValue, vEnergyLossValueVariance)));
                                                
      if (pParticle->mPosition.y < worldY)
      {
         pParticle->mPosition.y = worldY;
         pParticle->mVelocity = XMVectorMultiply(XMVector3Reflect(pParticle->mVelocity, cYAxisVector), vEnergyLoss);         
      }
   }

#ifdef DEBUG_VMX_UPDATE   
   double totalTime = timer.getElapsedSeconds();
   double timePerParticle = totalTime / numParticles;
   double cycles = timePerParticle * 3200000000;
   trace("Collision VMX  Particles Processed: %d, total time: %2.6f, cycles per particle: %4.3f", numParticles, totalTime, cycles);
#endif
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updatePFXParticlesVMX(XMVECTOR fUpdateTime)
{      
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 3)
      prefetchState = Utils::BeginPrefetch(&mParticles[2], 0);
            
   XMVECTOR speedAdjustedVelocity;
   BParticleSystemParticle* pParticle;    
   XMVECTOR intensity;      
   XMVECTOR position;
   XMVECTOR birthTimeV;
   XMVECTOR oneOverfLifeTimeV;
   XMVECTOR lifeAlphaV;
   XMVECTOR lifeInSecondsV;   
   XMVECTOR randomNumberV;
   XMVECTOR gravityV;   
   
   intensity = gVectorOne;         
   
   XMVECTOR vBBoxOffset = mpData->mBBoxOffset;

   float speedLookupTableSize      = ((float) mpData->mSpeed.getLookupTableSize()) - 0.00001f;
   XMVECTOR speedLookupTableSizeV  = XMLoadScalar(&speedLookupTableSize);
   int   speedProgressionLookupIndex = 0;
   XMVECTOR speedProgressionValueV = gVectorOne;
   XMVECTOR speedValue = gVectorOne;

   XMVECTOR vSpeedValueX    = XMLoadScalar(&mpData->mSpeed.mValue.x);
   XMVECTOR vSpeedValueY    = XMLoadScalar(&mpData->mSpeed.mValue.y);
   XMVECTOR vSpeedValueZ    = XMLoadScalar(&mpData->mSpeed.mValue.z);
   XMVECTOR vSpeedValueVarX = XMLoadScalar(&mpData->mSpeed.mValueVar.x);
   XMVECTOR vSpeedValueVarY = XMLoadScalar(&mpData->mSpeed.mValueVar.y);
   XMVECTOR vSpeedValueVarZ = XMLoadScalar(&mpData->mSpeed.mValueVar.y);

   XMVECTOR vAccelerationValue    = XMLoadScalar(&mpData->mProperties.mAcceleration);
   XMVECTOR vAccelerationValueVar = XMLoadScalar(&mpData->mProperties.mAccelerationVar);   

   XMVECTOR accelerationV;
   XMVECTOR velocityV;

   XMVECTOR vGravityValue    = XMVectorZero();
   XMVECTOR vGravityValueVar = XMVectorZero();
   if (mpData->mForce.mUseInternalGravity)
   {
      vGravityValue    = XMLoadScalar(&mpData->mForce.mInternalGravity);
      vGravityValueVar = XMLoadScalar(&mpData->mForce.mInternalGravityVar);      
   }
      
   int   currentTime  = mpManager->getTime();
   float currentTimeF = (float)mpManager->getTimeFloat();
   XMVECTOR currentTimeV = XMVectorSplatX(XMLoadScalar(&currentTimeF));

   XMMATRIX emitterMatrix = XMMatrixIdentity();

   if (mpData->mProperties.mTiedToEmitter)
      emitterMatrix = mTransform;

#ifdef DEBUG_VMX_UPDATE
   BTimer timer;
   timer.start();
#endif DEBUG_VMX_UPDATE

   XMVECTOR tempIndex;

   uint numParticles = mParticles.getSize();
#ifdef DEBUG_VMX_UPDATE
   const uint origNumParticles = numParticles;
#endif
   XMCOLOR temp;

   static const XMVECTOR gBounds = {9999,9999,9999,0};
   mBoundingMaxs = -gBounds;
   mBoundingMins =  gBounds;

   XMMATRIX pfxMatrix = XMMatrixIdentity();
   //XMVECTOR pfxRight   = cXAxisVector;
   //XMVECTOR pfxUp      = cYAxisVector;
   //XMVECTOR pfxForward = cZAxisVector;
   //XMVECTOR posTemp;

   bool bParentIsVisible = getFlag(BParticleEmitter::eFlagVisible);

   for (uint i = 0; i < numParticles; i++)
   {          
      if ((i + 2) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[i + 2], 0);
         
      pParticle = &mParticles[i];

      //-- See if we died of old age.
      if (currentTime >= mParticles[i].mDeathTime)
      {         
         mParticles[i].releaseEffect(false);
         mParticles[i] = mParticles[numParticles - 1];
         numParticles--;
         i--;         
         continue;
      }

      if (pParticle->mpEffect==NULL)
         continue;

      pParticle->mpEffect->setFlag(BParticleEmitter::eFlagVisible, bParentIsVisible);
      
      BDEBUG_ASSERT(mParticles[i].mState==BParticleSystemParticle::eStateActive);

      randomNumberV     = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      birthTimeV        = XMVectorSplatX(XMLoadScalar(&pParticle->mFBirthTime));
      oneOverfLifeTimeV = XMVectorSplatX(XMLoadScalar(&pParticle->mFOOLifeTime));           

      speedValue = PS_COMPUTE_VARIANCE_X(randomNumberV, vSpeedValueX, vSpeedValueVarX);

      lifeInSecondsV = XMVectorSubtract(currentTimeV, birthTimeV);
      
      lifeAlphaV = XMVectorMax(XMVectorMin(XMVectorMultiply(lifeInSecondsV, oneOverfLifeTimeV), gVectorOne), XMVectorZero());
      
      tempIndex = XMConvertVectorFloatToInt(XMVectorTruncate(XMVectorMultiply(lifeAlphaV, speedLookupTableSizeV)),0);
      XMStoreScalar(&speedProgressionLookupIndex, tempIndex);
      
      speedValue =__vrlimi(speedValue, PS_COMPUTE_VARIANCE_X(randomNumberV, vSpeedValueY, vSpeedValueVarY), VRLIMI_CONST(0,1,0,0), 3); // get Y

      //-- get gravity
      gravityV = __vrlimi(XMVectorZero(), PS_COMPUTE_VARIANCE_X(randomNumberV, vGravityValue, vGravityValueVar), VRLIMI_CONST(0,1,0,0), 3); 

      speedProgressionValueV = __vrlimi(XMLoadDHenN3(&mpData->mSpeed.mLookupTable[speedProgressionLookupIndex]), gVectorOne, VRLIMI_CONST(0,0,0,1), 0);

      //-- compute acceleration and add in gravity
      //-- add in gravity with acceleration and
      accelerationV = XMVectorMultiplyAdd(XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV, vAccelerationValue, vAccelerationValueVar)), pParticle->mVelocity, gravityV);      

      //-- now apply the acceleration (plus gravity force to the velocity);
      velocityV     = XMVectorMultiplyAdd(accelerationV, fUpdateTime, pParticle->mVelocity);

      speedValue =__vrlimi(speedValue, PS_COMPUTE_VARIANCE_X(randomNumberV, vSpeedValueZ, vSpeedValueVarZ), VRLIMI_CONST(0,0,1,0), 2); // get Z      

      speedValue = XMVectorMultiply(speedProgressionValueV, speedValue);
            
      speedAdjustedVelocity = XMVectorMultiply(velocityV, speedValue);      
            
      position = XMVectorMultiplyAdd(speedAdjustedVelocity, fUpdateTime, pParticle->mPosition);      
      
      //-- Fill Position and tumble     
      pParticle->mPosition = position;            

      //-- Update bounding box
      mBoundingMaxs = XMVectorMax(XMVectorAdd(position, vBBoxOffset) , mBoundingMaxs);
      mBoundingMins = XMVectorMin(XMVectorSubtract(position, vBBoxOffset), mBoundingMins);            
      
      pParticle->mVelocity = velocityV;
     
      position = XMVector3Transform(position, emitterMatrix);

#if 0
      if (numParticles > 1)
      {
         if (i > 0)
            pfxForward = XMVector3Normalize(position - mParticles[i-1].mPosition);            

         XMVECTOR dot = XMVectorAbs(XMVector3Dot(pfxUp, pfxForward));
         if (dot.x <= cFloatCompareEpsilon)
         {
            pfxUp = XMVector3Normalize(XMVector3Cross(pfxForward, pfxRight));
            pfxRight = XMVector3Normalize(XMVector3Cross(pfxUp, pfxForward));
         }
         else
         {
            pfxRight = XMVector3Normalize(XMVector3Cross(pfxUp, pfxForward));
            pfxUp    = XMVector3Normalize(XMVector3Cross(pfxForward, pfxRight));         
         }
      }

      pfxMatrix.r[0] = pfxRight;
      pfxMatrix.r[1] = pfxUp;
      pfxMatrix.r[2] = pfxForward;
#endif
      pfxMatrix.r[3] = position;
            
      pParticle->mpEffect->updateWorldMatrix(pfxMatrix, NULL);
   }

   //-- if the particle effect is tied to the emitter we need to transform the BBox
   if (mpData->mProperties.mTiedToEmitter)
      transformAABB();

   mParticles.resize(numParticles);
   mParticles.reserve(0);

#ifdef DEBUG_VMX_UPDATE
      double totalTime = timer.getElapsedSeconds();
      double timePerParticle = totalTime / origNumParticles;
      double cycles = timePerParticle * 3200000000;
      trace("Processed: %d, total time: %2.6f, cycles per particle: %4.3f", origNumParticles, totalTime, cycles);
#endif
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updateTrailParticlesLVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount)
{
   BParticleSystemParticle* pParticle = NULL;   
   BPTrailVertexPT* pVB = (BPTrailVertexPT*) pBuffer;

   //-- fill out colors and scale;
   XMVECTOR scale;

   XMVECTOR vOpacity     = XMLoadScalar(&mpData->mOpacity.mValue);
   XMVECTOR vOpacityVar  = XMLoadScalar(&mpData->mOpacity.mValueVar);

   //-- uniform scale
   XMVECTOR vScaleValueW    = XMLoadScalar(&mpData->mScale.mValue.w);
   XMVECTOR vScaleValueVarW = XMLoadScalar(&mpData->mScale.mValueVar.w);
   XMVECTOR uniformScaleV;

   XMVECTOR vScaleValueX    = XMLoadScalar(&mpData->mScale.mValue.x);
   XMVECTOR vScaleValueY    = XMLoadScalar(&mpData->mScale.mValue.y);
   XMVECTOR vScaleValueVarX = XMLoadScalar(&mpData->mScale.mValueVar.x);
   XMVECTOR vScaleValueVarY = XMLoadScalar(&mpData->mScale.mValueVar.y);

   XMVECTOR vBBoxOffset = mpData->mBBoxOffset;

   XMVECTOR vIntensityValue    = XMLoadScalar(&mpData->mIntensity.mValue);
   XMVECTOR vIntensityValueVar = XMLoadScalar(&mpData->mIntensity.mValueVar);

   XMVECTOR colorV       = gVectorOne;
   XMVECTOR randomNumberV;
   XMVECTOR birthTimeV;
   XMVECTOR oneOverfLifeTimeV;
   XMVECTOR lifeAlphaV;
   XMVECTOR lifeInSecondsV;
   float currentTimeF = (float)mpManager->getTimeFloat();
   XMVECTOR currentTimeV = XMLoadScalar(&currentTimeF);
   XMVECTOR velocityV;
   
   uint numParticles = mParticles.getSize();
   
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 5)
      prefetchState = Utils::BeginPrefetch(&mParticles[4], 0);
      
   XMVECTOR intensity; 

   static const XMVECTOR gBounds = {9999,9999,9999,0};
   mBoundingMaxs = -gBounds;
   mBoundingMins =  gBounds;
   vertCount = 0;

   for (uint i = 0; i < numParticles; ++i)
   {
      if ((i + 4) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[i + 4], 0);
         
      pParticle = &mParticles[i];
      BDEBUG_ASSERT(pParticle != NULL);
      
      vertCount++;

      randomNumberV      = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      birthTimeV         = XMLoadScalar(&pParticle->mFBirthTime);
      oneOverfLifeTimeV  = XMLoadScalar(&pParticle->mFOOLifeTime);      

      //-- compute the uniform scale
      uniformScaleV = XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueW, vScaleValueVarW));

      //-- compute the opacity value and variance and store it in the w of the color vector
      colorV =__vrlimi(colorV, PS_COMPUTE_VARIANCE_X(randomNumberV, vOpacity, vOpacityVar), VRLIMI_CONST(0,0,0,1), 1); // get Z

      lifeInSecondsV = XMVectorSubtract(currentTimeV, birthTimeV);

      lifeAlphaV = XMVectorMax(XMVectorMin(XMVectorMultiply(lifeInSecondsV, oneOverfLifeTimeV), gVectorOne), XMVectorZero());

      velocityV = __vrlimi(pParticle->mVelocity, lifeInSecondsV, VRLIMI_CONST(0,0,0,1), 1);

      //-- get x for scale      
      scale   = PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueX, vScaleValueVarX);
      scale   =__vrlimi(scale, PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueY, vScaleValueVarY), VRLIMI_CONST(0,1,0,0), 3); // get Y

      //-- scale xy uniformely
      scale = XMVectorMultiply(scale, uniformScaleV);

      //-- needed for texture animation
      scale   =__vrlimi(scale, lifeInSecondsV, VRLIMI_CONST(0,0,1,0), 2); // get Z

      //-- store our random value in our scale w component
      scale = __vrlimi(scale, randomNumberV, VRLIMI_CONST(0,0,0,1), 1);

      intensity = PS_COMPUTE_VARIANCE_X(randomNumberV, vIntensityValue, vIntensityValueVar);
      //-- store life and intensity      
      lifeAlphaV = __vrlimi(lifeAlphaV, intensity, VRLIMI_CONST(0,1,0,0), 3);
      
      //-- Update bounding box
      mBoundingMaxs = XMVectorMax(XMVectorAdd((XMVECTOR)pParticle->mPosition,vBBoxOffset) , mBoundingMaxs);
      mBoundingMins = XMVectorMin(XMVectorSubtract((XMVECTOR)pParticle->mPosition,vBBoxOffset), mBoundingMins);

      

      //-- store the color
      XMStoreFloat4NC(&pVB->mPos, pParticle->mPosition);
      _WriteBarrier();
      XMStoreHalf4(&pVB->mScale, scale);
      _WriteBarrier();
      XMStoreHalf4(&pVB->mUp, velocityV);
      _WriteBarrier();
      XMStoreHalf2(&pVB->mLife,  lifeAlphaV);
      _WriteBarrier();
      XMStoreColor(&pVB->mColor, colorV);
      _WriteBarrier();
      pVB->mTextureZ = pParticle->mTextureArrayZ;
      _WriteBarrier();

      pVB++;
   }
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updateBeamParticlesVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount)
{      
   BPBeamVertexPT* pVB = (BPBeamVertexPT*) pBuffer;
   
   uint numParticles = mpData->mProperties.mBeamTesselation; //mpData->mProperties.mEmissionRate < 1.0f ? 1 : (uint) mpData->mProperties.mEmissionRate;
   
   XMVECTOR vBBoxOffset = mpData->mBBoxOffset;
   
   //-- uniform scale
   XMVECTOR vScaleValueW    = XMLoadScalar(&mpData->mScale.mValue.w);
   XMVECTOR vScaleValueVarW = XMLoadScalar(&mpData->mScale.mValueVar.w);
   XMVECTOR uniformScaleV;

   XMVECTOR vScaleValueX    = XMLoadScalar(&mpData->mScale.mValue.x);
   XMVECTOR vScaleValueY    = XMLoadScalar(&mpData->mScale.mValue.y);
   XMVECTOR vScaleValueVarX = XMLoadScalar(&mpData->mScale.mValueVar.x);
   XMVECTOR vScaleValueVarY = XMLoadScalar(&mpData->mScale.mValueVar.y);

   //-- opacity
   XMVECTOR vOpacity     = XMLoadScalar(&mpData->mOpacity.mValue);
   XMVECTOR vOpacityVar  = XMLoadScalar(&mpData->mOpacity.mValueVar);

   //-- intensity
   XMVECTOR vIntensityValue    = XMLoadScalar(&mpData->mIntensity.mValue);
   XMVECTOR vIntensityValueVar = XMLoadScalar(&mpData->mIntensity.mValueVar);
   
   XMVECTOR scale;
   XMVECTOR colorV = gVectorOne;
   XMVECTOR randomNumberV;
   XMVECTOR birthTimeV;
   XMVECTOR oneOverfLifeTimeV;
   XMVECTOR lifeAlphaV;
   XMVECTOR lifeInSecondsV;
   float currentTimeF = (float)mpManager->getTimeFloat();
   XMVECTOR currentTimeV = XMLoadScalar(&currentTimeF);
   XMVECTOR velocityV;
   XMVECTOR intensity;      

   static const XMVECTOR gBounds = {9999,9999,9999,0};
   mBoundingMaxs = -gBounds;
   mBoundingMins =  gBounds;
   vertCount = 0;

   BParticleSystemParticle* pParticle = &mParticles[0];

   //-- if our life time is over then start the life cycle again.
   int currentTime = mpManager->getTime();   
   if (currentTime >= pParticle->mDeathTime)
   {
      mParticles.clear();
      return;
   }         

   XMVECTOR startPosition;
   XMVECTOR endPosition;
   XMVECTOR distanceAlpha;

   startPosition = XMVectorSet(mpData->mShape.mXPosOffset, mpData->mShape.mYPosOffset, mpData->mShape.mZPosOffset, 1.0f);
   endPosition   = XMVectorSet(mpData->mShape.mXPosOffset, mpData->mShape.mYPosOffset, mpData->mShape.mZPosOffset, 1.0f);
    
   // mTransform has rotation but mTransform2 does not.  So just use the translation part of the matrix to get
   // consistent start and end positions.  So the offset will act like a world space offset, which should be ok
   // since only the y offset is currently used for beams.
//   startPosition = XMVector3Transform(startPosition, mTransform);
//   endPosition   = XMVector3Transform(endPosition, mTransform2);
   startPosition = XMVectorAdd(startPosition, mTransform.r[3]);
   endPosition   = XMVectorAdd(endPosition, mTransform2.r[3]);

   bool static bUseHack = false;
   if (bUseHack)
      endPosition = XMVectorAdd(startPosition, XMVectorSet(10.0f,10.0f,10.f, 0.0f));   

   XMVECTOR directionV = endPosition - startPosition;
   XMVECTOR rightV = XMVectorSet(1,0,0,0);

   XMVECTOR upV = XMVector3Normalize(XMVector3Cross(directionV, rightV));
   rightV       = XMVector3Normalize(XMVector3Cross(upV, directionV));   

   static bool bUseLifeTime = true;
   static bool bUseOffset = false;
   
   XMVECTOR pos;
   float oneOverNumParticlesMinusOne =  1.0f / (numParticles - 1);

   for (uint i = 0; i < numParticles; ++i)
   {  
      float alpha = (float) i * oneOverNumParticlesMinusOne;

      pos = XMVectorLerp(startPosition, endPosition, alpha);

      //pos = XMVectorHermiteV(startPosition, tangent0, endPosition, tangent1, XMVectorReplicate(alpha));

      vertCount++;

      distanceAlpha      = XMLoadScalar(&alpha);
      randomNumberV      = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) & (PS_NUM_RAND_FLOATS - 1)]);
      birthTimeV         = XMLoadScalar(&pParticle->mFBirthTime);
      oneOverfLifeTimeV  = XMLoadScalar(&pParticle->mFOOLifeTime);      
    
      //-- compute the uniform scale
      uniformScaleV = XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueW, vScaleValueVarW));

      //-- compute the opacity value and variance and store it in the w of the color vector
      colorV =__vrlimi(colorV, PS_COMPUTE_VARIANCE_X(randomNumberV, vOpacity, vOpacityVar), VRLIMI_CONST(0,0,0,1), 1); // get Z            

      lifeInSecondsV = XMVectorSubtract(currentTimeV, birthTimeV);

      lifeAlphaV = XMVectorMax(XMVectorMin(XMVectorMultiply(lifeInSecondsV, oneOverfLifeTimeV), gVectorOne), XMVectorZero());      
            
      velocityV = __vrlimi(pParticle->mVelocity, lifeInSecondsV, VRLIMI_CONST(0,0,0,1), 1);

      //-- get x for scale      
      scale   = PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueX, vScaleValueVarX);
      scale   =__vrlimi(scale, PS_COMPUTE_VARIANCE_X(randomNumberV, vScaleValueY, vScaleValueVarY), VRLIMI_CONST(0,1,0,0), 3); // get Y

      //-- scale xy uniformely
      scale = XMVectorMultiply(scale, uniformScaleV);

      //-- needed for texture animation
      scale   =__vrlimi(scale, lifeInSecondsV, VRLIMI_CONST(0,0,1,0), 2); // get Z
      
      //-- store our random value in our scale w component
      scale = __vrlimi(scale, randomNumberV, VRLIMI_CONST(0,0,0,1), 1);

      intensity = PS_COMPUTE_VARIANCE_X(randomNumberV, vIntensityValue, vIntensityValueVar);
      //-- store life in x component and intensity in y component
      lifeAlphaV = __vrlimi(lifeAlphaV, intensity, VRLIMI_CONST(0,1,0,0), 3);
      //-- store distance alpha in the Z component
      lifeAlphaV = __vrlimi(lifeAlphaV, distanceAlpha, VRLIMI_CONST(0,0,1,0), 2);
            
      //-- Update bounding box
      mBoundingMaxs = XMVectorMax(XMVectorAdd(pos, vBBoxOffset), mBoundingMaxs);
      mBoundingMins = XMVectorMin(XMVectorSubtract(pos, vBBoxOffset), mBoundingMins);

      //-- store the color
      XMStoreFloat4NC(&pVB->mPos, pos);
      _WriteBarrier();
      XMStoreHalf4(&pVB->mScale, scale);
      _WriteBarrier();
      XMStoreHalf4(&pVB->mUp, velocityV);
      _WriteBarrier();
      XMStoreHalf4(&pVB->mLife,  lifeAlphaV);
      _WriteBarrier();
      XMStoreColor(&pVB->mColor, colorV);
      _WriteBarrier();
      pVB->mTextureZ = pParticle->mTextureArrayZ;
      _WriteBarrier();

      pVB++;
   }
}

//----------------------------------------------------------------------------
// This is an optimized function with all LHS faults removed.
// Do not modify this unless you know what you are doing.
//----------------------------------------------------------------------------
void BParticleEmitter::updateParticlesVMX(XMVECTOR fUpdateTime, void* __restrict pBuffer, int& vertCount)
{
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 3)
      prefetchState = Utils::BeginPrefetch(&mParticles[2], 0);

   vertCount = 0;
   BPVertexPT* pVB = (BPVertexPT*)pBuffer;
   
   XMVECTOR speedAdjustedVelocity;
   BParticleSystemParticle* pParticle; 
   XMVECTOR scale;
   XMVECTOR intensity;   
   XMVECTOR tumbleParamsV;
   XMVECTOR tumbleParamsAngleV;
   XMVECTOR tumbleParamsVelocityV;
   XMVECTOR position;
   XMVECTOR birthTimeV;
   XMVECTOR oneOverfLifeTimeV;
   XMVECTOR lifeAlphaV;
   XMVECTOR lifeInSecondsV;
   XMVECTOR oneOverTextureStageCountV;
   XMVECTOR oneOverTwoTextureStageCountV;   
   XMVECTOR randomNumberV;
   XMVECTOR randomNumberV2;
   XMVECTOR randomNumberV3;
   XMVECTOR randomNumberV4;

   XMVECTOR gravityV;
   XMVECTOR colorV = gVectorOne;   

   XMVECTOR vOpacity     = XMLoadScalar(&mpData->mOpacity.mValue);
   XMVECTOR vOpacityVar  = XMLoadScalar(&mpData->mOpacity.mValueVar);

   intensity = gVectorOne;

   float textureStageCountF = (float) mpData->mTextures.mDiffuse.mStages.getSize();
   float oneOverTextureStageCountF = 1.0f / textureStageCountF;
   float oneOverTwoTextureStageCountF = 1.0f / (2.0f * textureStageCountF);

   oneOverTextureStageCountV = XMLoadScalar(&oneOverTextureStageCountF);
   oneOverTwoTextureStageCountV = XMLoadScalar(&oneOverTwoTextureStageCountF);   

   //-- uniform scale
   XMVECTOR vScaleValueW    = XMLoadScalar(&mpData->mScale.mValue.w);
   XMVECTOR vScaleValueVarW = XMLoadScalar(&mpData->mScale.mValueVar.w);
   XMVECTOR uniformScaleV;

   //-- non uniform scale
   XMVECTOR vScaleValueX    = XMLoadScalar(&mpData->mScale.mValue.x);
   XMVECTOR vScaleValueY    = XMLoadScalar(&mpData->mScale.mValue.y);
   XMVECTOR vScaleValueVarX = XMLoadScalar(&mpData->mScale.mValueVar.x);
   XMVECTOR vScaleValueVarY = XMLoadScalar(&mpData->mScale.mValueVar.y);
   XMVECTOR vBBoxOffset = mpData->mBBoxOffset;

   float speedLookupTableSize      = ((float) mpData->mSpeed.getLookupTableSize()) - 0.00001f;
   XMVECTOR speedLookupTableSizeV  = XMLoadScalar(&speedLookupTableSize);
   int   speedProgressionLookupIndex = 0;
   XMVECTOR speedProgressionValueV = gVectorOne;
   XMVECTOR speedValue = gVectorOne;

   XMVECTOR vSpeedValueX    = XMLoadScalar(&mpData->mSpeed.mValue.x);
   XMVECTOR vSpeedValueY    = XMLoadScalar(&mpData->mSpeed.mValue.y);
   XMVECTOR vSpeedValueZ    = XMLoadScalar(&mpData->mSpeed.mValue.z);
   XMVECTOR vSpeedValueVarX = XMLoadScalar(&mpData->mSpeed.mValueVar.x);
   XMVECTOR vSpeedValueVarY = XMLoadScalar(&mpData->mSpeed.mValueVar.y);
   XMVECTOR vSpeedValueVarZ = XMLoadScalar(&mpData->mSpeed.mValueVar.y);

   XMVECTOR vAccelerationValue    = XMLoadScalar(&mpData->mProperties.mAcceleration);
   XMVECTOR vAccelerationValueVar = XMLoadScalar(&mpData->mProperties.mAccelerationVar);   

   XMVECTOR accelerationV;
   XMVECTOR velocityV;

   XMVECTOR vGravityValue    = XMVectorZero();
   XMVECTOR vGravityValueVar = XMVectorZero();
   if (mpData->mForce.mUseInternalGravity)
   {
      vGravityValue    = XMLoadScalar(&mpData->mForce.mInternalGravity);
      vGravityValueVar = XMLoadScalar(&mpData->mForce.mInternalGravityVar);      
   }

   XMVECTOR vIntensityValue    = XMLoadScalar(&mpData->mIntensity.mValue);
   XMVECTOR vIntensityValueVar = XMLoadScalar(&mpData->mIntensity.mValueVar);
   
   float currentTimeF = (float)mpManager->getTimeFloat();
   XMVECTOR currentTimeV = XMVectorSplatX(XMLoadScalar(&currentTimeF));
      
#ifdef DEBUG_VMX_UPDATE
   BTimer timer;
   timer.start();
#endif DEBUG_VMX_UPDATE

   XMVECTOR tempIndex;

   uint numParticles = mParticles.getSize();
#ifdef DEBUG_VMX_UPDATE
   const uint origNumParticles = numParticles;
#endif
   XMCOLOR temp;

   static const XMVECTOR gBounds = {9999,9999,9999,0};
   mBoundingMaxs = -gBounds;
   mBoundingMins =  gBounds;

   for (uint i = 0; i < numParticles; i++)
   {          
      if ((i + 2) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[i + 2], 0);
      pParticle = &mParticles[i];
            
      //BDEBUG_ASSERT(mParticles[i].mState==BParticleSystemParticle::eStateActive);

      randomNumberV     = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience) % (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV2    = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience+1) % (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV3    = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience+2) % (PS_NUM_RAND_FLOATS - 1)]);
      randomNumberV4    = XMLoadScalar(&gPSManager.mRandomFloats[(pParticle->mVarience+3) % (PS_NUM_RAND_FLOATS - 1)]);
      birthTimeV        = XMVectorSplatX(XMLoadScalar(&pParticle->mFBirthTime));
      oneOverfLifeTimeV = XMVectorSplatX(XMLoadScalar(&pParticle->mFOOLifeTime));     
      tumbleParamsV     = XMLoadHalf2(&pParticle->mTumbleParams);     
      
      speedValue = PS_COMPUTE_VARIANCE_X(randomNumberV, vSpeedValueX, vSpeedValueVarX);
      
      lifeInSecondsV = XMVectorSubtract(currentTimeV, birthTimeV);

      /* DEBUG CODE -- DO NOT ENABLE
      float curFrame = floor(lifeInSecondsV.x * (float) mpData->mTextures.mDiffuse.mUVAnimation.mFramesPerSecond);
      float frame1   = fmodf(curFrame,mpData->mTextures.mDiffuse.mUVAnimation.mNumFrames);
      trace("LifeInSeconds = %4.3f Frame=%4.3f", lifeInSecondsV.x, frame1);
      */

      lifeAlphaV = XMVectorMax(XMVectorMin(XMVectorMultiply(lifeInSecondsV, oneOverfLifeTimeV), gVectorOne), XMVectorZero());
      
      tumbleParamsAngleV    = __vrlimi(XMVectorZero(), tumbleParamsV, VRLIMI_CONST(1,0,0,0), 0);
      tumbleParamsVelocityV = __vrlimi(XMVectorZero(), tumbleParamsV, VRLIMI_CONST(1,0,0,0), 1);

      tempIndex = XMConvertVectorFloatToInt(XMVectorTruncate(XMVectorMultiply(lifeAlphaV, speedLookupTableSizeV)),0);
      XMStoreScalar(&speedProgressionLookupIndex, tempIndex);
            
      //-- compute the uniform scale
      uniformScaleV = XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV4, vScaleValueW, vScaleValueVarW));
                        
      speedValue =__vrlimi(speedValue, PS_COMPUTE_VARIANCE_X(randomNumberV3, vSpeedValueY, vSpeedValueVarY), VRLIMI_CONST(0,1,0,0), 3); // get Y

      //-- get gravity
      gravityV = __vrlimi(XMVectorZero(), PS_COMPUTE_VARIANCE_X(randomNumberV2, vGravityValue, vGravityValueVar), VRLIMI_CONST(0,1,0,0), 3); 

      speedProgressionValueV = __vrlimi(XMLoadDHenN3(&mpData->mSpeed.mLookupTable[speedProgressionLookupIndex]), gVectorOne, VRLIMI_CONST(0,0,0,1), 0);
      
      //-- compute acceleration and add in gravity
      //-- add in gravity with acceleration and
      accelerationV = XMVectorMultiplyAdd(XMVectorSplatX(PS_COMPUTE_VARIANCE_X(randomNumberV3, vAccelerationValue, vAccelerationValueVar)), pParticle->mVelocity, gravityV);
                                
      //-- now apply the acceleration (plus gravity force to the velocity);
      velocityV     = XMVectorMultiplyAdd(accelerationV, fUpdateTime, pParticle->mVelocity);
                        
      speedValue =__vrlimi(speedValue, PS_COMPUTE_VARIANCE_X(randomNumberV, vSpeedValueZ, vSpeedValueVarZ), VRLIMI_CONST(0,0,1,0), 2); // get Z

      //-- compute the opacity value and variance and store it in the w of the color vector
      colorV =__vrlimi(colorV, PS_COMPUTE_VARIANCE_X(randomNumberV4, vOpacity, vOpacityVar), VRLIMI_CONST(0,0,0,1), 1); // get Z

      speedValue = XMVectorMultiply(speedProgressionValueV, speedValue);

      //-- get x for scale      
      scale   = PS_COMPUTE_VARIANCE_X(randomNumberV2, vScaleValueX, vScaleValueVarX);

      //-- get Intensity
      intensity = PS_COMPUTE_VARIANCE_X(randomNumberV, vIntensityValue, vIntensityValueVar);

      speedAdjustedVelocity = XMVectorMultiply(velocityV, speedValue);      

      //-- store life and intensity      
      lifeAlphaV = __vrlimi(lifeAlphaV, intensity, VRLIMI_CONST(0,1,0,0), 3);

      //-- get y for scale      
      scale   =__vrlimi(scale, PS_COMPUTE_VARIANCE_X(randomNumberV3, vScaleValueY, vScaleValueVarY), VRLIMI_CONST(0,1,0,0), 3); // get Y
            
      position = XMVectorMultiplyAdd(speedAdjustedVelocity, fUpdateTime, pParticle->mPosition);
                 
      //-- Update tumble.
      tumbleParamsV = __vrlimi(tumbleParamsV, XMVectorMultiplyAdd(tumbleParamsVelocityV, fUpdateTime, tumbleParamsAngleV), VRLIMI_CONST(1,0,0,0), 0);

      //-- scale xy uniformely
      scale = XMVectorMultiply(scale, uniformScaleV);

      //-- Fill Position and tumble     
      pParticle->mPosition = position;
      
      //-- needed for texture animation
      scale   =__vrlimi(scale, lifeInSecondsV, VRLIMI_CONST(0,0,1,0), 2); // get Z

      //-- store our random value in our scale w component
      scale   =__vrlimi(scale, randomNumberV, VRLIMI_CONST(0,0,0,1), 1);
      
      //-- Update bounding box
      mBoundingMaxs = XMVectorMax(XMVectorAdd(position, vBBoxOffset) , mBoundingMaxs);
      mBoundingMins = XMVectorMin(XMVectorSubtract(position, vBBoxOffset), mBoundingMins);

      XMStoreHalf2(&pParticle->mTumbleParams, tumbleParamsV);      
      pParticle->mVelocity = velocityV;
      velocityV = __vrlimi(velocityV, lifeInSecondsV, VRLIMI_CONST(0,0,0,1), 1);

      XMStoreFloat4NC(&pVB->mPos, __vrlimi(position,  tumbleParamsV, VRLIMI_CONST(0,0,0,1), 1));
      _WriteBarrier();
      XMStoreHalf4(&pVB->mScale, scale);
      _WriteBarrier();            
      XMStoreHalf4(&pVB->mVelocity, velocityV);
      _WriteBarrier();
      XMStoreHalf2(&pVB->mLife, lifeAlphaV);
      _WriteBarrier();
      XMStoreColor(&(pVB->mColor), colorV);      
      _WriteBarrier();
      pVB->mTextureZ = pParticle->mTextureArrayZ;
      _WriteBarrier();
      
      ++pVB;
      ++vertCount;
   }

   //-- if the particle effect is tied to the emitter we need to transform the BBox
   if (mpData->mProperties.mTiedToEmitter)
      transformAABB();
   
#ifdef DEBUG_VMX_UPDATE
   double totalTime = timer.getElapsedSeconds();
   double timePerParticle = totalTime / origNumParticles;
   double cycles = timePerParticle * 3200000000;
   trace("Processed: %d, total time: %2.6f, cycles per particle: %4.3f", origNumParticles, totalTime, cycles);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::transformAABB()
{
   XMVECTOR c0 = XMVector3Transform(mBoundingMins, (XMMATRIX) mTransform);
   XMVECTOR c1 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(0,0,1,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c2 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(0,1,0,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c3 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(0,1,1,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c4 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(1,0,0,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c5 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(1,0,1,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c6 = XMVector3Transform(__vrlimi(mBoundingMins, mBoundingMaxs, VRLIMI_CONST(1,1,0,0), 0), (XMMATRIX) mTransform);
   XMVECTOR c7 = XMVector3Transform(mBoundingMaxs, (XMMATRIX) mTransform);

   mBoundingMins = XMVectorMin(c0, c1);
   mBoundingMins = XMVectorMin(mBoundingMins, c2);
   mBoundingMins = XMVectorMin(mBoundingMins, c3);
   mBoundingMins = XMVectorMin(mBoundingMins, c4);
   mBoundingMins = XMVectorMin(mBoundingMins, c5);
   mBoundingMins = XMVectorMin(mBoundingMins, c6);
   mBoundingMins = XMVectorMin(mBoundingMins, c7);

   mBoundingMaxs = XMVectorMax(c0, c1);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c2);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c3);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c4);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c5);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c6);
   mBoundingMaxs = XMVectorMax(mBoundingMaxs, c7);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::cleanupDeadParticlesSorted()
{
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 5)
      prefetchState = Utils::BeginPrefetch(&mParticles[4], 0);

   int  currentManagerTime = mpManager->getTime();
   currentManagerTime;
   int  currentTime  = mClockTime;
   uint particleCount = mParticles.getSize();
   uint dstIndex = 0;
   for (uint i = 0; i < particleCount; i++)
   {
      if ((i + 4) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[i + 4], 0);

      if (mParticles[i].isDead() || currentTime >= mParticles[i].mDeathTime)
      {
         mParticles[i].releaseEffect(false);
         continue;
      }
         
      if (dstIndex != i)
         mParticles[dstIndex] = mParticles[i];
      
      dstIndex++;
   }
   
   mParticles.resize(dstIndex);
   mParticles.reserve(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleEmitter::cleanupDeadParticlesUnsorted()
{
   Utils::BPrefetchState prefetchState = NULL;
   if (mParticles.getSize() >= 5)
      prefetchState = Utils::BeginPrefetch(&mParticles[4], 0);

   int  currentTime  = mpManager->getTime();
   uint numParticles = mParticles.getSize();
   for (uint i = 0; i < numParticles; i++)
   {          
      if ((i + 4) < mParticles.getSize())
         prefetchState = Utils::UpdatePrefetch(prefetchState, &mParticles[i + 4], 0);
         
      //-- See if we died of old age.
      if (currentTime >= mParticles[i].mDeathTime)
      {
         mParticles[i].releaseEffect(false);
         mParticles[i] = mParticles[numParticles - 1];
         numParticles--;
         i--;         
      }      
   }
   mParticles.resize(numParticles);
   mParticles.reserve(0);
}

#define INT_VAL(x) (*(long *) &(x))
#define SIGN_BIT(x) ((INT_VAL(x)&(1<<31)))
#define SAME_SIGNS(a,b) (SIGN_BIT(a)==SIGN_BIT(b))

//==============================================================================
// BParticleEmitter::reinitTimeData
//==============================================================================
void BParticleEmitter::reinitTimeData()
{
   mStartTime            = mpManager->getTime();
   mStartTimeF           = mpManager->getTimeFloat();
   mDeathTime            = mStartTime + mMaxLifeTime;
   mLastUpdateTime       = mStartTime - TIMEVARIANCE(mpData->mProperties.mInitialUpdate, mpData->mProperties.mInitialUpdateVar);
}

//==============================================================================
// MAIN THREAD FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
// pData has to be a buffer from frame Storage if its not null
//------------------------------------------------------------------------------
void BParticleEmitter::submitRenderCommand(int tileIndex, BParticleRenderer::eRenderCommand type, void* pBufferVB, int startOffsetVB, void* pBufferIB, int startOffsetIB, int vertexCount, int indexCount, int texHandle, int texHandle2, int texHandle3, int intensityTexHandle)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pBufferVB != NULL);
   BDEBUG_ASSERT(type < BParticleRenderer::eRenderCommandTotal);

   BParticleEmitterRenderCommand command;
   BParticleEmitterRenderCommand* pCommand = &command;
   
   pCommand->mVertexCount   = vertexCount;
   pCommand->mpVB           = pBufferVB;
   pCommand->mStartOffsetVB = startOffsetVB;
   pCommand->mIndexCount    = indexCount;
   pCommand->mpIB           = pBufferIB;
   pCommand->mStartOffsetIB = startOffsetIB;

   pCommand->mTextureHandle = texHandle;
   pCommand->mTextureHandle2 = texHandle2;
   pCommand->mTextureHandle3 = texHandle3;
   pCommand->mIntensityTextureHandle = intensityTexHandle;
   pCommand->mAABBMins      = mBoundingMins;
   pCommand->mAABBMaxs      = mBoundingMaxs;
   

   pCommand->mTextureStageCounts.x = mpData->mTextures.mDiffuse.mInvTextureArraySize;
   pCommand->mTextureStageCounts.y = mpData->mTextures.mDiffuse2.mInvTextureArraySize;
   pCommand->mTextureStageCounts.z = mpData->mTextures.mDiffuse3.mInvTextureArraySize;
   pCommand->mTextureStageCounts.w = mpData->mTextures.mIntensity.mInvTextureArraySize;

   pCommand->mUVVelocity0.x = mpData->mTextures.mDiffuse.mUVAnimation.mScrollU;
   pCommand->mUVVelocity0.y = mpData->mTextures.mDiffuse.mUVAnimation.mScrollV;
   pCommand->mUVVelocity0.z = mpData->mTextures.mDiffuse2.mUVAnimation.mScrollU;
   pCommand->mUVVelocity0.w = mpData->mTextures.mDiffuse2.mUVAnimation.mScrollV;

   pCommand->mUVVelocity1.x = mpData->mTextures.mDiffuse3.mUVAnimation.mScrollU;
   pCommand->mUVVelocity1.y = mpData->mTextures.mDiffuse3.mUVAnimation.mScrollV;
   pCommand->mUVVelocity1.z = mpData->mTextures.mIntensity.mUVAnimation.mScrollU;
   pCommand->mUVVelocity1.w = mpData->mTextures.mIntensity.mUVAnimation.mScrollV;

   pCommand->mUVRandomOffsetSelector0.x = mpData->mTextures.mDiffuse.mUVAnimation.mUseRandomScrollOffsetU ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector0.y = mpData->mTextures.mDiffuse.mUVAnimation.mUseRandomScrollOffsetV ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector0.z = mpData->mTextures.mDiffuse2.mUVAnimation.mUseRandomScrollOffsetU ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector0.w = mpData->mTextures.mDiffuse2.mUVAnimation.mUseRandomScrollOffsetV ? 1.0f : 0.0f;

   pCommand->mUVRandomOffsetSelector1.x = mpData->mTextures.mDiffuse3.mUVAnimation.mUseRandomScrollOffsetU ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector1.y = mpData->mTextures.mDiffuse3.mUVAnimation.mUseRandomScrollOffsetV ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector1.z = mpData->mTextures.mIntensity.mUVAnimation.mUseRandomScrollOffsetU ? 1.0f : 0.0f;
   pCommand->mUVRandomOffsetSelector1.w = mpData->mTextures.mIntensity.mUVAnimation.mUseRandomScrollOffsetV ? 1.0f : 0.0f;

   pCommand->mVertex1Color = mpData->mColor.mVertex1Color;
   pCommand->mVertex2Color = mpData->mColor.mVertex2Color;
   pCommand->mVertex3Color = mpData->mColor.mVertex3Color;
   pCommand->mVertex4Color = mpData->mColor.mVertex4Color;
   
   pCommand->mTintColor = XMVectorSplatOne();
   if (mpData->mColor.mPlayerColor)
   {                  
      pCommand->mTintColor = XMVectorMultiply(XMLoadColor(&mTintColor), mpData->mColor.mPlayerColorIntensity);
   }

   //-- does this emmiter want to be affected by the light set sun color
   if (mpData->mColor.mSunColor)
   {      
      XMVECTOR sunColor = gPSManager.getSunColor();
      sunColor = XMVectorMultiply(sunColor, mpData->mColor.mSunColorIntensity);
      pCommand->mTintColor = XMVectorMultiply(pCommand->mTintColor, sunColor);
   }

   XMVECTOR beamStart = mTransform.r[3];
   XMVECTOR beamEnd = mTransform2.r[3];
   pCommand->mBeamForward = XMVector3Normalize(XMVectorSubtract(beamEnd, beamStart));   

   pCommand->mBeamProgressionAlphaSelector.x = mpData->mProperties.mBeamColorByLength ? 1.0f : 0.0f;  
   pCommand->mBeamProgressionAlphaSelector.y = mpData->mProperties.mBeamOpacityByLength ? 1.0f : 0.0f; 
   pCommand->mBeamProgressionAlphaSelector.z = mpData->mProperties.mBeamIntensityByLength ? 1.0f : 0.0f;
   pCommand->mBeamProgressionAlphaSelector.w = 0.0f;
   
   // If the .XML file contained a light buffer flag, then use that, otherwise try to choose a suitable default.
   if (!gPSManager.getLightBufferingEnabled())
      pCommand->mLightBuffering = false;
   else
   {
      switch (mpData->mProperties.mBlendMode)
      {
         case BEmitterBaseData::eAlphaBlend:
         case BEmitterBaseData::ePremultipliedAlpha:
         {
            if (mpData->mProperties.mLightBufferValueLoaded)   
               pCommand->mLightBuffering = mpData->mProperties.mLightBuffer;
            else
               pCommand->mLightBuffering = true;
            break;
         }
         default:
            pCommand->mLightBuffering = false;
            break;
      }
   }

   pCommand->mLightBufferIntensityScale = mpData->mProperties.mLightBufferIntensityScale;
      
   float uvWidth      = (float) mpData->mTextures.mDiffuse.mUVAnimation.mFrameWidth  / (float) mpData->mTextures.mDiffuse.mWidth;
   float uvHeight     = (float) mpData->mTextures.mDiffuse.mUVAnimation.mFrameHeight / (float) mpData->mTextures.mDiffuse.mHeight;
   float framesPerRow = (float)mpData->mTextures.mDiffuse.mWidth / (float) mpData->mTextures.mDiffuse.mUVAnimation.mFrameWidth;
   int   numRows      = (int)(mpData->mTextures.mDiffuse.mHeight / (float) mpData->mTextures.mDiffuse.mUVAnimation.mFrameHeight);
   float numFrames    = framesPerRow * numRows;

   //-- get emitter attraction and clamp it
   float emitterAtttraction = VARIANCE(mpData->mProperties.mEmitterAttraction, mpData->mProperties.mEmitterAttractionVar);
   emitterAtttraction = Math::Clamp(emitterAtttraction, 0.0f, 1.0f);

   BMatrix worldMatrix;
   BMatrix lastUpdateWorldMatrix;
   if (mpData->mProperties.mTiedToEmitter)
   {
      worldMatrix = getTransform();
      lastUpdateWorldMatrix = mLastUpdateTransform;
   }
   else
   {
      worldMatrix.makeIdentity();
      lastUpdateWorldMatrix.makeIdentity();
   }
   
   pCommand->mEmitterMatrix          = worldMatrix;
   pCommand->mEmitterAttraction      = emitterAtttraction;
   pCommand->mLastUpdateEmitterMatrix= lastUpdateWorldMatrix;

   pCommand->mTextureAnimationData.x = uvWidth;
   pCommand->mTextureAnimationData.y = uvHeight;
   pCommand->mTextureAnimationData.z = framesPerRow;
   pCommand->mTextureAnimationData.w = numFrames; 

   pCommand->mTimeData.x = (float)getManager()->getTimeFloat();
   pCommand->mTimeData.y = mpData->mTextures.mDiffuse.mUVAnimation.mFramesPerSecond;
   pCommand->mTimeData.z = (vertexCount > 1) ? (1.0f / (float) (vertexCount-1)) : 0.0f;
   pCommand->mTimeData.w = 0.0f;

   pCommand->mOpacity = mOpacity;

   pCommand->mProgressionTextureV = mpData->mProgressionV;

   pCommand->mTerrainDecalYOffset = mpData->mProperties.mTerrainDecalYOffset;
   pCommand->mTerrainDecalTesselation = mpData->mProperties.mTerrainDecalTesselation;


   // soft particles
   const BMatrix44& screenToView = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix44(cMTScreenToView);

   int tileOffsetX = 0;
   int tileOffsetY = 0;
   if (tileIndex != -1)
   {
      const RECT& tileRect = gTiledAAManager.getTileRect(tileIndex);
      tileOffsetX = tileRect.left;
      tileOffsetY = tileRect.top;
   }

   pCommand->mSoftParticleToggle = mpData->mProperties.mSoftParticles;
   pCommand->mSoftParticleParams = BVec4(screenToView[2][3], screenToView[3][3], (float) tileOffsetX, (float) tileOffsetY);   
   pCommand->mSoftParticleParams2 = BVec4(mpData->mProperties.mFillOptimized ? 2.0f : 1.0f, mpData->mProperties.mSoftParticleFadeRange, 0, 0);

   switch (mpData->mProperties.mBlendMode)
   {
      case BEmitterBaseData::eAlphaBlend:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_Alphablend;
         break;
      case BEmitterBaseData::eAdditive:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_Additive;
         break;
      case BEmitterBaseData::eSubtractive:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_Subtractive;
         break;
      case BEmitterBaseData::eDistortion:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_Distortion;
         break;
      case BEmitterBaseData::ePremultipliedAlpha:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_PremultiplyAlpha;
         break;
      default:
         pCommand->mBlendMode = BParticleRenderer::eRenderBlendMode_Alphablend;
   };
         
   //-- store whether the diffuse texture is animated?
   pCommand->mTextureAnimationToggle = mpData->mTextures.mDiffuse.mUVAnimation.mUseUVAnimation;
   pCommand->mIntensityMaskToggle    = mpData->mTextures.mIntensity.mStages.getNumber() > 0;

   //-- store whether we need to use multitexturing
   pCommand->mMultiTextureToggle = (mpData->mTextures.mDiffuse.mStages.getNumber() > 0) && (mpData->mTextures.mDiffuse2.mStages.getNumber() > 0);
   pCommand->mMultiTexture3LayersToggle = mpData->mTextures.mDiffuse3.mStages.getNumber() > 0;

   pCommand->mMultiTextureBlendMultiplier  = XMVectorZero();   
   if (mpData->mTextures.mDiffuseLayer1To2BlendMode == BEmitterTextureData::eBlendMultiply)
   {
      pCommand->mMultiTextureBlendMultiplier.x = 1.0f;
      pCommand->mMultiTextureBlendMultiplier.y = 0.0f;
   }
   else if (mpData->mTextures.mDiffuseLayer1To2BlendMode == BEmitterTextureData::eBlendAlpha)
   {
      pCommand->mMultiTextureBlendMultiplier.x = 0.0f;
      pCommand->mMultiTextureBlendMultiplier.y = 1.0f;
   }
   
   if (mpData->mTextures.mDiffuseLayer2To3BlendMode == BEmitterTextureData::eBlendMultiply)
   {
      pCommand->mMultiTextureBlendMultiplier.z = 1.0f;
      pCommand->mMultiTextureBlendMultiplier.w = 0.0f;
   }
   else if (mpData->mTextures.mDiffuseLayer2To3BlendMode == BEmitterTextureData::eBlendAlpha)
   {
      pCommand->mMultiTextureBlendMultiplier.z = 0.0f;
      pCommand->mMultiTextureBlendMultiplier.w = 1.0f;
   }   
   
   //-- fill out pallette colors
   pCommand->mPalletteToggle         = mpData->mColor.mType == BEmitterColorData::ePalletteColor ? true : false;
   if (pCommand->mPalletteToggle)
   {
      uint palletteCount = mpData->mColor.mPallette.getNumber();
      if (palletteCount > cMaxPalletteColors)
         palletteCount = cMaxPalletteColors;

      //-- subtract one so we can pick an index between 0 - 7 in the shader
      pCommand->mNumPalletteColors = (BYTE)(palletteCount - 1);
      for (uint i = 0; i < palletteCount; ++i)
      {
         pCommand->mPalletteColor[i] = XMLoadColor((XMCOLOR*)&mpData->mColor.mPallette[i].mColor);
      }
   }

   gParticleRenderer.render(type, (const uchar*)pCommand);
}

//============================================================================
//============================================================================
void BParticleEmitter::computeGPUFrameStorage(uint& frameStorageVB, uint& frameStorageIB)
{
   BDEBUG_ASSERT(mpData!=NULL);
   uint gpuFrameStorageSizeVB = 0;
   uint gpuFrameStorageSizeIB = 0;
   switch (mpData->mProperties.mType)
   {
      case BEmitterBaseData::eTrail:
      case BEmitterBaseData::eTrailCross:
         gpuFrameStorageSizeVB = mParticles.getSize() * sizeof(BPTrailVertexPT);
         break;
      case BEmitterBaseData::eBeam:
         {
            uint tesselation = mpData->mProperties.mBeamTesselation < 1 ? 1 : (uint) mpData->mProperties.mBeamTesselation;
            gpuFrameStorageSizeVB = tesselation * sizeof(BPBeamVertexPT);
         }
         break;
      case BEmitterBaseData::eBillBoard:
      case BEmitterBaseData::eOrientedAxialBillboard:
      case BEmitterBaseData::eUpfacing:
      case BEmitterBaseData::eVelocityAligned:
      case BEmitterBaseData::eTerrainPatch:
         {
            gpuFrameStorageSizeVB = mParticles.getSize() * sizeof(BPVertexPT);
            gpuFrameStorageSizeIB = ((mpData->mProperties.mSortParticles) && (mpData->mProperties.mBlendMode == BEmitterBaseData::eAlphaBlend))? mParticles.getSize() * 4 * sizeof(WORD) : 0;
            break;
         }
   }

   frameStorageVB = gpuFrameStorageSizeVB;
   frameStorageIB = gpuFrameStorageSizeIB;   
}

//============================================================================
//============================================================================
float BParticleEmitter::computeScore() const
{
   // we negate the score because the radix sorter sorts only ascendingly.

   float score;
   double age = Math::ClampLow((double) (gPSManager.getTimeFloat() - mStartTimeF), (double) 0.0f);
   score = (float)(- ((mPriority * 1000000.0f) - (age * 10000.0f)));
   return score;
}

//============================================================================
//============================================================================
void BParticleEmitter::debugRenderParticles()
{
   if (mpData && mpData->mProperties.mType != BEmitterBaseData::eTrail)
      return;

   float radius = 0.25f;
   DWORD color;
   float life;
   for (uint i = 0; i < mParticles.getSize(); ++i)
   {      
      if (i == 0)
      {
         radius = 0.25f;
         color  = cDWORDGreen;
         life   = 2.0f; 
      }
      else
      {
         radius = 0.15f;
         color  = cDWORDYellow;
         life   = 1.0f; 
      }
      
      gpDebugPrimitives->addDebugSphere(mParticles[i].mPosition, radius, color, BDebugPrimitives::cCategoryNone,  life );
   }
}

//============================================================================
//============================================================================
void BParticleEmitter::debugRenderMagnets()
{
   uint magnetCount = mpData->mMagnets.getSize();
   if (magnetCount == 0)
      return;
   
   XMVECTOR oneV = XMVectorSet(1,1,1,1);
   XMVECTOR zeroV = XMVectorZero();

   XMVECTOR selectControl =  XMVectorSelectControl(0,0,0,1);   
   XMVECTOR emitterPos = XMVectorSelect(mTransform.r[3], zeroV, selectControl);
   XMVECTOR magnetPos;
   DWORD color; 
   XMMATRIX matrix;
   
   for (uint m = 0; m < magnetCount; ++m)
   {  
      BMagnetData* pMagnet = mpData->mMagnets[m];

      matrix = pMagnet->mRotation;
      matrix.r[3] = XMVectorSelect(pMagnet->mPosOffset, oneV, selectControl);      
      matrix = XMMatrixMultiply(matrix, mTransform);

      magnetPos = XMVectorAdd(emitterPos, pMagnet->mPosOffset);

      gpDebugPrimitives->addDebugAxis(matrix, 5.0f, BDebugPrimitives::cCategoryParticles, -1.0f);
               
      color = cDWORDGreen;
      if (pMagnet->mForce < 0.0f)
         color = cDWORDRed;

      if (pMagnet->mType == BMagnetData::eCylinder)
      {
         gpDebugPrimitives->addDebugCylinder(matrix, pMagnet->mRadius, pMagnet->mHeight, color, BDebugPrimitives::cCategoryParticles);         
      }
      else
      {
         gpDebugPrimitives->addDebugSphere(magnetPos, pMagnet->mRadius, color, BDebugPrimitives::cCategoryParticles);
      }
   }   
}

//============================================================================
//============================================================================
bool BParticleEmitter::getBoundSphereVisibleInAnyViewport() const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   for (int v = 0; v < viewportCount; ++v)
   {
      if (mBoundSphereVisible[v])
         return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getBoundSphereVisibleForAllViewports() const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   for (int v = 0; v < viewportCount; ++v)
   {
      if (!mBoundSphereVisible[v])
         return false;
   }
   return true;
}

//============================================================================
//============================================================================
void BParticleEmitter::setBoundSphereVisibleForAllViewports(bool bValue)
{
   for (int v = 0; v < cMaxViewports; ++v)
      mBoundSphereVisible[v] = bValue;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getCompletelyFadedOutForAllViewports() const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   for (int v = 0; v < viewportCount; ++v)
   {
      if (!mCompletelyFadedOut[v])
         return false;
   }
   return true;
}

//============================================================================
//============================================================================
void BParticleEmitter::setCompletelyFadedOutForAllViewports(bool bValue)
{
   for (int v = 0; v < cMaxViewports; ++v)
      mCompletelyFadedOut[v] = bValue;
}

//============================================================================
//============================================================================
void BParticleEmitter::setBoundSphereVisible(int viewportIndex, bool bValue)
{
   debugRangeCheck(viewportIndex, cMaxViewports);
   mBoundSphereVisible[viewportIndex] = bValue;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getBoundSphereVisible(int viewportIndex) const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   debugRangeCheck(viewportIndex, viewportCount);
   return mBoundSphereVisible[viewportIndex];
}

//============================================================================
//============================================================================
void BParticleEmitter::setCompletelyFadedOut(int viewportIndex, bool bValue)
{
   debugRangeCheck(viewportIndex, cMaxViewports);
   mCompletelyFadedOut[viewportIndex] = bValue;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getCompletelyFadedOut(int viewportIndex) const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   debugRangeCheck(viewportIndex, viewportCount);
   return mCompletelyFadedOut[viewportIndex];
}

//============================================================================
//============================================================================
void BParticleEmitter::setVisibleInAllViewports(bool bValue)
{
   for (int v = 0; v < cMaxViewports; ++v)
      mIsVisibleInViewport[v] = bValue;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getVisibleInAllViewports() const
{
   for (int v = 0; v < cMaxViewports; ++v)
   {
      if (!mIsVisibleInViewport[v])
         return false;
   }
   return true;
}

//============================================================================
//============================================================================
void BParticleEmitter::setVisibleInViewport(int viewportIndex, bool bValue)
{
   debugRangeCheck(viewportIndex, cMaxViewports);
   mIsVisibleInViewport[viewportIndex] = bValue;
}

//============================================================================
//============================================================================
bool BParticleEmitter::getVisibleInViewport(int viewportIndex) const
{
   int viewportCount = Math::Min(static_cast<int>(cMaxViewports), static_cast<int>(gRenderDraw.getNumViewports()));
   debugRangeCheck(viewportIndex, viewportCount);
   return mIsVisibleInViewport[viewportIndex];
}
