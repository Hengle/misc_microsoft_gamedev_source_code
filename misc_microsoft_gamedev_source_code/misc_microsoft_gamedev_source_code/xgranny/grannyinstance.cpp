//============================================================================
// grannyinstance.cpp
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "grannyinstance.h"

// xgranny
#include "grannymanager.h"
#include "grannymodel.h"
#include "grannyanimation.h"
#include "grannyInstanceRenderer.h"

// xgamerender
#include "boundingBox.h"

// xsystem
#include "mathutil.h"
#include "pointin.h"
#include "position.h"
#include "perf.h"

// xrender
#include "ugxGeomManager.h"

//xgame
#include "..\xgame\syncmacros.h"

#include "xvisual.h"

// Defines
//#define DEBUG_RENDER_BOUNDING_BOX

BGrannyInstance_updateSyncCallback BGrannyInstance::sUpdateSyncCallback = BGrannyInstance::defaultUpdateSyncCallback;

GFIMPLEMENTVERSION(BGrannyInstance, 3);

//============================================================================
// BGrannyInstance::BGrannyInstance
//============================================================================
BGrannyInstance::BGrannyInstance() :
   mRefCount(0),
   mModelIndex(-1),
   mGrannyManagerIndex(-1),
   mStateChange(0),
   mModelInstance(NULL),
   mpRenderMaskResetCallback(NULL),
   mAnimClock(0.0f),
   mOldClock(0.0f),
   mNewClock(0.0f),
   mMeshRenderMaskAllSet(false),
   mHasMotionExtraction(false),
   mActiveAnimTime(0.0f),
   //mActiveAnimDuration(0.0f),
   mpSampleAnimCache(NULL),
   mInterpolation(1.0),
   mpClockOverride(NULL),
   mIsInterpolating(false),   
   mClockSet(false),
   mRenderPrepareLocalPose(NULL)

   //mpAttachmentTransformTrack(NULL)
{
}

//============================================================================
// BGrannyInstance::~BGrannyInstance
//============================================================================
BGrannyInstance::~BGrannyInstance()
{
	deinit();
}

//============================================================================
// BGrannyInstance::deinit
//============================================================================
void BGrannyInstance::deinit()
{
   stopAnimations(0, 0.0f);
   stopAnimations(1, 0.0f);

   mIKNodes.clear();

	// Free instance.
	if(mModelInstance)
	{      
      GrannyFreeModelInstance(mModelInstance);
		mModelInstance=NULL;
	}

   mpRenderMaskResetCallback = NULL;
	
   // ajl 7/9/08 - mRefCount not being reset? is that okay?
   
   mAnimClock = 0.0f;
   mOldClock = 0.0f;
   mNewClock = 0.0f;
   
   mModelIndex=-1;
   
   mInterpolation = 1.0f;
   mIsInterpolating = false;

   if (mpClockOverride)
   {
      HEAP_DELETE(mpClockOverride, gSimHeap);
      mpClockOverride = NULL;
   }

   mHasMotionExtraction = false;
   mMeshRenderMaskAllSet = false;   
   mClockSet = false;
   
   mUVOffsets.clear();

   if (mpSampleAnimCache)
   {
      gGrannyManager.releaseSampleAnimCache(mpSampleAnimCache);
      mpSampleAnimCache=NULL;
   }

   mStateChange = 0;

   mMeshRenderMask.clear();

   mActiveAnimTime = 0.0f;
   
   // Shouldn't get nuked with this hanging around.
   BASSERT(mRenderPrepareLocalPose == NULL);

   //mGrannyManagerIndex=-1;
}

//============================================================================
// BGrannyInstance::init
//============================================================================
bool BGrannyInstance::init(long modelIndex, const BVisualModelUVOffsets* pUVOffsets)
{
	// First nuke anything existing.
	deinit();

	// Look up model.
	BGrannyModel* pModel = gGrannyManager.getModel(modelIndex);
	if ((pModel == NULL) || (pModel->getGrannyFileInfo() == NULL))
	{
		return(false);
	}
		
	// Set model index.
	BDEBUG_ASSERT(modelIndex >= SHRT_MIN && modelIndex <= SHRT_MAX);
	mModelIndex = (short)modelIndex;

	if ((pModel->getGrannyFileInfo()->ModelCount < 1) || (pModel->getGrannyFileInfo()->Models[0] == NULL))
		return false;

	// Create a new instance.
	mModelInstance = GrannyInstantiateModel(pModel->getGrannyFileInfo()->Models[0]);
	if(mModelInstance == NULL)
      return false;

   //-- set up the render mask
   //-- turn all the bits on execpt for the "damage" meshes
   setMeshRenderMaskToUndamageState();
   
   if (pUVOffsets)
      mUVOffsets = *pUVOffsets;
   else
      mUVOffsets.clear();
      
	// Success.
	return(true);
}

//============================================================================
// BGrannyInstance::setClock
//============================================================================
void BGrannyInstance::setClock(float seconds)
{
	// Check if we have an instance.
	if(!mModelInstance)
		return;

	// Cache time
	mAnimClock = seconds;

	// Set time.
	GrannySetModelClock(mModelInstance, seconds);   

	// Kill any unneeded controls.
	GrannyFreeCompletedModelControls(mModelInstance);

   mStateChange++;
}

//============================================================================
void BGrannyInstance::updateInterpolatedClockValues()
{
   if (mClockSet)
   {
      mOldClock = mNewClock;
      mNewClock = mAnimClock;
   }
   else
   {
      mOldClock = mNewClock = mAnimClock;
      mClockSet = true;
   }

   // Update IK interpolation history.
   int numIKNodes = getNumIKNodes();
   for (int i = 0; i < numIKNodes; i++)
   {
      BIKNode *pIKNode = &mIKNodes[i];
      if (pIKNode->mIsActive)
      {
         if(!pIKNode->mAnchorPosUpdated)
            pIKNode->setOldAnchorPos(pIKNode->getAnchorPos());
         else
            pIKNode->mAnchorPosUpdated = false;
         if(!pIKNode->mTargetPosUpdated)
            pIKNode->setOldTargetPos(pIKNode->getTargetPos());
         else
            pIKNode->mTargetPosUpdated = false;
      }
   }
}

//============================================================================
//============================================================================
void BGrannyInstance::resetInterpolatedClockValues()
{
   mOldClock = mNewClock = mAnimClock;
}

//============================================================================
// BGrannyInstance::setClockOverride
//============================================================================
void BGrannyInstance::setClockOverride(float animPos, float animDuration, long actionIndex, long movementIndex, float elapsed)
{
   if (!mpClockOverride)
   {
      mpClockOverride = HEAP_NEW(BClockOverride, gSimHeap);
      if (!mpClockOverride)
         return;
      mpClockOverride->mOldAnimPos = animPos;
   }
   else
      mpClockOverride->mOldAnimPos = mpClockOverride->mNewAnimPos;
   
   mpClockOverride->mNewAnimPos = animPos;
   mpClockOverride->mAnimDuration = animDuration;
   mpClockOverride->mActionIndex = actionIndex;
   mpClockOverride->mMovementIndex = movementIndex;
   mpClockOverride->mElapsed = elapsed;
}

//============================================================================
// BGrannyInstance::clearClockOverride
//============================================================================
void BGrannyInstance::clearClockOverride()
{
   if (mpClockOverride)
   {
      HEAP_DELETE(mpClockOverride, gSimHeap);
      mpClockOverride = NULL;
   }
}

//============================================================================
// BGrannyInstance::setInterpolatedClocks
//============================================================================
void BGrannyInstance::setInterpolatedClocks(granny_model_instance *modelInstance, BGrannySaveControlState* pSaveStates, uint maxSaveStates, uint& numSaveStates)
{
   numSaveStates = 0;

   if (!pSaveStates || maxSaveStates <= 0)
      return;

   if (!isInterpolating())
      return;   

   float interpolatedClock = getInterpolatedClock();

   // Save off the old control clock values and updating the clocks to the interpolated value.
   if (mpClockOverride)
   {  
      float animPos = Math::Lerp(mpClockOverride->mOldAnimPos, mpClockOverride->mNewAnimPos, mInterpolation);
      float elapsed = Math::Lerp(0.0f, mpClockOverride->mElapsed, mInterpolation);

      for(granny_model_control_binding* pBinding = GrannyModelControlsBegin(modelInstance);
         pBinding != GrannyModelControlsEnd(modelInstance);
         pBinding = GrannyModelControlsNext(pBinding))
      {
         granny_control* pControl = GrannyGetControlFromBinding(pBinding);
         if (pControl)
         {
            if (numSaveStates < maxSaveStates)
            {
               BGrannySaveControlState& cs = pSaveStates[numSaveStates];
               cs.Control = pControl;
               GrannyGetControlClockValues(pControl, &cs.LocalClock, &cs.CurrentClock, &cs.dTLocalClockPending, &cs.LoopIndex);
               numSaveStates++;
            }
            else
            {
               BASSERT(0);
               return;
            }

            bool clockSet = false;

            void** pUserData = GrannyGetControlUserDataArray(pControl);
            if (pUserData != NULL)
            {
               long userDataAnimIndex = (long)pUserData[GrannyControlUserData_AnimIndex];
               long userDataTrackIndex = (long)pUserData[GrannyControlUserData_AnimationTrack];
               if (((userDataTrackIndex == cActionAnimationTrack) && (userDataAnimIndex == mpClockOverride->mActionIndex)) ||
                  ((userDataTrackIndex == cMovementAnimationTrack) && (userDataAnimIndex == mpClockOverride->mMovementIndex)))
               {
                  float localClock = GrannyGetControlRawLocalClock(pControl);
                  mpClockOverride->mOldClock = localClock;
                  float newLocalClock = localClock + Math::Clamp((animPos - localClock), elapsed * -4.0f, elapsed * 4.0f); // clamp change to 4x regular anim speed
                  newLocalClock = Math::Clamp(newLocalClock, 0.0f, mpClockOverride->mAnimDuration);
                  GrannySetControlRawLocalClock(pControl, newLocalClock);
                  clockSet = true;
               }                      
            }

            if (!clockSet)
               GrannySetControlClock(pControl, interpolatedClock);
         }
      }

      return;
   }

   for(granny_model_control_binding* pBinding = GrannyModelControlsBegin(modelInstance);
      pBinding != GrannyModelControlsEnd(modelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (pControl)
      {
         if (numSaveStates < maxSaveStates)
         {
            BGrannySaveControlState& cs = pSaveStates[numSaveStates];
            cs.Control = pControl;
            GrannyGetControlClockValues(pControl, &cs.LocalClock, &cs.CurrentClock, &cs.dTLocalClockPending, &cs.LoopIndex);
            GrannySetControlClock(pControl, interpolatedClock);
            numSaveStates++;
         }
         else
         {
            BASSERT(0);
            return;
         }
      }
   }
}

//============================================================================
// BGrannyInstance::unsetInterpolatedClocks
//============================================================================
void BGrannyInstance::unsetInterpolatedClocks(granny_model_instance *modelInstance, const BGrannySaveControlState* pSaveStates, uint numSaveStates)
{
   if (numSaveStates <= 0 || !pSaveStates)
      return;

   if (!isInterpolating())
      return;
   
   // Restore the control clock values.
   uint index = 0;
   for(granny_model_control_binding* pBinding = GrannyModelControlsBegin(modelInstance);
      pBinding != GrannyModelControlsEnd(modelInstance);
      pBinding = GrannyModelControlsNext(pBinding), ++index)
   {
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (pControl)
      {
         if (index < numSaveStates)
         {
            const BGrannySaveControlState& cs = pSaveStates[index];
            if (cs.Control == pControl)
               GrannySetControlClockValues(pControl, cs.LocalClock, cs.CurrentClock, cs.dTLocalClockPending, cs.LoopIndex);
            else
            {
               BASSERT(0);
               return;
            }
         }
         else
         {
            BASSERT(0);
            return;
         }
      }
   }
}

//============================================================================
// BGrannyInstance::getGrannyControlClock
//============================================================================
float BGrannyInstance::getGrannyControlClock()
{
   if(mModelInstance)
   {
      granny_model_control_binding* pBinding=GrannyModelControlsBegin(mModelInstance);
      if (pBinding && pBinding != GrannyModelControlsEnd(mModelInstance))
      {
//-- FIXING PREFIX BUG ID 7713
         const granny_control* pControl=GrannyGetControlFromBinding(pBinding);
//--
         if(pControl)
         {
            float clock=GrannyGetControlClock(pControl);
            GrannyModelControlsEnd(mModelInstance);
            return clock;
         }
      }
   }      
   return 0.0f;
}

//============================================================================
// BGrannyInstance::updateGrannySync
//
// ajl 7/26/07 - see comment about this in BVisualItem::updateGrannySync
//============================================================================
void BGrannyInstance::updateGrannySync(bool doSyncChecks)
{
   if (mModelInstance)
      sUpdateSyncCallback(mModelInstance, doSyncChecks);
}

//============================================================================
//============================================================================
void BGrannyInstance::defaultUpdateSyncCallback(granny_model_instance* modelInstance, bool doSyncChecks)
{
   if (!modelInstance)
      return;
   for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(modelInstance);
        pBinding != GrannyModelControlsEnd(modelInstance);
        pBinding = GrannyModelControlsNext(pBinding))
   {
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (pControl)
         GrannyGetControlClampedLocalClock(pControl);
   }
}

//============================================================================
// BGrannyInstance::playAnimation
//============================================================================
bool BGrannyInstance::playAnimation(long animationTrack, long animIndex, float playWeight, float currentTime, long loopCount, float transitionTime, float timeIntoAnimation)
{
   //mpAttachmentTransformTrack = NULL;
   if (mModelInstance == NULL)
   {
      // The only way it should be possible for this to happen is if an anim file or Granny file
      // is reloaded through drag and drop and this instance's Granny model file is messed up and
      // won't reload.  This situation shouldn't occur through normal running of the game because
      // this Granny instance will never be created if the model isn't loaded.
      BFAIL("No instance handle.  (BGrannyInstance::playAnimation)");
      return false;
   }

   // Look for animation.
   BGrannyAnimation *pAnim = gGrannyManager.getAnimation(animIndex);
   if(!pAnim)
      return false;

   mStateChange++;

	// Ease out current animations.
	long stoppedCount = stopAnimations(animationTrack, transitionTime);
	
	// Create a control to play the animation.
	granny_control* pControl = NULL;
	granny_int32x trackGroupIndex;
	if (GrannyFindTrackGroupForModel(pAnim->getGrannyFileInfo()->Animations[0],
		GrannyGetSourceModel(mModelInstance)->Name,
		&trackGroupIndex))
	{
		granny_controlled_animation_builder* pBuilder = GrannyBeginControlledAnimation(mAnimClock+currentTime, pAnim->getGrannyFileInfo()->Animations[0]);
		if(pBuilder)
		{
			GrannySetTrackGroupTarget(pBuilder, trackGroupIndex, mModelInstance);

         BGrannyModel* pGrannyModel = gGrannyManager.getModel(mModelIndex);
         if (pGrannyModel)
            GrannySetTrackGroupModelMask(pBuilder, trackGroupIndex, pGrannyModel->getTrackMask(animationTrack));

			// Change the extraction mode to the one set in the anim.  This defaults to none.
         BGrannyAnimation::BMotionExtractionMode motionExtractionMode = pAnim->getMotionExtractionMode();
         mHasMotionExtraction = (motionExtractionMode == BGrannyAnimation::cNoExtraction) ? false : true;
			GrannySetTrackGroupAccumulation(pBuilder, trackGroupIndex, (granny_accumulation_mode) motionExtractionMode);

			// If the animation has model data, tell Granny that this is the model the anim was originally modeled for.
			// Granny will attempt to map the anim from the original model to the model being animated in case their
			// skeletons are different.
			if (pAnim->hasModelData())
			{
				GrannySetTrackGroupBasisTransform(pBuilder, trackGroupIndex, pAnim->getGrannyFileInfo()->Models[0], GrannyGetSourceModel(mModelInstance));
			}

			pControl = GrannyEndControlledAnimation(pBuilder);
		}
	}

	if(pControl)
   {
	   // Set it to auto-free itself.
	   GrannyFreeControlOnceUnused(pControl);

	   // Set loop count.  0 means indefinitely.
      GrannySetControlLoopCount(pControl, loopCount);
      GrannySetControlForceClampedLooping(pControl, !mHasMotionExtraction);

	   //-- set the play weight for this control
	   GrannySetControlWeight(pControl, playWeight);

	   // Start easing this animation in.
	   if(stoppedCount > 0)
		   GrannyEaseControlIn(pControl, transitionTime, false);

      // Prevent the control from being checked for completion
      GrannySetControlCompletionCheckFlag(pControl, false);

	   // Offset into animation itself by the requested amount of time.
	   GrannySetControlRawLocalClock(pControl, timeIntoAnimation);

	   void** ppUserData = GrannyGetControlUserDataArray(pControl);
	   if(ppUserData != NULL)
	   {
		   ppUserData[GrannyControlUserData_AnimIndex] = (void*)animIndex;
		   ppUserData[GrannyControlUserData_IsBlendControl] = (void*)0;
		   ppUserData[GrannyControlUserData_EasingOut] = (void*)FALSE;
		   ppUserData[GrannyControlUserData_AnimationTrack] = (void*)animationTrack;
	   }
   }

   // Success.
	return(true);
}

//============================================================================
// BGrannyInstance::blendAnimation
//============================================================================
bool BGrannyInstance::blendAnimation(long animationTrack, long animIndex, float playWeight, float currentTime, long loopCount, float transitionTimeIn, float transitionTimeOut, float timeIntoAnimation)
{
	BGrannyAnimation* pAnim = gGrannyManager.getAnimation(animIndex);
	if(!pAnim)
		return(false);

   mStateChange++;

	// Create a control to play the animation.
	granny_control* pControl = NULL;
	granny_int32x trackGroupIndex;
	if (GrannyFindTrackGroupForModel(pAnim->getGrannyFileInfo()->Animations[0], GrannyGetSourceModel(mModelInstance)->Name, &trackGroupIndex))
	{
		granny_controlled_animation_builder* pBuilder = GrannyBeginControlledAnimation(mAnimClock+currentTime, pAnim->getGrannyFileInfo()->Animations[0]);
		if(pBuilder)
		{
			GrannySetTrackGroupTarget(pBuilder, trackGroupIndex, mModelInstance);

         BGrannyModel* pGrannyModel = gGrannyManager.getModel(mModelIndex);
         if (pGrannyModel)
            GrannySetTrackGroupModelMask(pBuilder, trackGroupIndex, pGrannyModel->getTrackMask(animationTrack));

         // Change the extraction mode to the one set in the anim.  This defaults to none.
         BGrannyAnimation::BMotionExtractionMode motionExtractionMode = pAnim->getMotionExtractionMode();
         mHasMotionExtraction = (motionExtractionMode == BGrannyAnimation::cNoExtraction) ? false : true;
         GrannySetTrackGroupAccumulation(pBuilder, trackGroupIndex, (granny_accumulation_mode) motionExtractionMode);

			// If the animation has model data, tell Granny that this is the model the anim was originally modeled for.
			// Granny will attempt to map the anim from the original model to the model being animated in case their
			// skeletons are different.
			if ((pAnim->getGrannyFileInfo()->ModelCount > 0) && (pAnim->getGrannyFileInfo()->Models) && (pAnim->getGrannyFileInfo()->Models[0]))
			{
				GrannySetTrackGroupBasisTransform(pBuilder, trackGroupIndex, pAnim->getGrannyFileInfo()->Models[0], GrannyGetSourceModel(mModelInstance));
			}

			pControl = GrannyEndControlledAnimation(pBuilder);
		}
	}

	if(pControl)
   {
	   // Set it to auto-free itself.
	   GrannyFreeControlOnceUnused(pControl);

	   //-- set the play weight for this control
	   GrannySetControlWeight(pControl, playWeight);

	   // Start easing this animation in.
	   GrannyEaseControlIn(pControl, transitionTimeIn, true);

	   // Set loop count.  0 means indefinitely.
      // Force it to 1 for now since blend anims are only set to run once anyway.  Granny
      // may assert if it is set to 0.
      loopCount = 1;
	   GrannySetControlLoopCount(pControl, loopCount);
      GrannySetControlForceClampedLooping(pControl, !mHasMotionExtraction);
	   //if(loopCount == 0)
		   //GrannySetControlLoopCount(pControl, loopCount);
         //GrannySetControlForceClampedLooping(pControl, true);
	   //else
	   {
		   GrannySetControlEaseOut(pControl, true);
		   float endTime = mAnimClock + currentTime + pAnim->getDuration() - timeIntoAnimation;
		   float startTime = endTime-transitionTimeOut;
		   if(startTime<currentTime)
			   startTime=currentTime;

		   GrannySetControlEaseOutCurve(pControl, startTime, endTime, 1.0f, 1.0f, 0.0f, 0.0f);
		   GrannyCompleteControlAt(pControl, endTime);
	   }

	   // Offset into animation itself by the requested amount of time.
	   GrannySetControlRawLocalClock(pControl, timeIntoAnimation);

	   void** ppUserData = GrannyGetControlUserDataArray(pControl);
	   if(ppUserData != NULL)
	   {
		   ppUserData[GrannyControlUserData_AnimIndex] = (void*)animIndex;
		   ppUserData[GrannyControlUserData_IsBlendControl] = (void*)1;
		   ppUserData[GrannyControlUserData_EasingOut] = (void*)FALSE;
		   ppUserData[GrannyControlUserData_AnimationTrack] = (void*)animationTrack;
	   }
   }

	// Success.
	return(true);
}

//==============================================================================
// BGrannyInstance::getAverageExtractedMotion
//==============================================================================
BVector BGrannyInstance::getAverageExtractedMotion() const
{
   BVector averageExtractedMotion;
   averageExtractedMotion.zero();

   if (mModelInstance == NULL)
      return averageExtractedMotion;

   float totalWeight = 0.0f;

   // Iterate over controls
   for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mModelInstance);
        pBinding != GrannyModelControlsEnd(mModelInstance);
        pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
//-- FIXING PREFIX BUG ID 7716
      const granny_control* pControl = GrannyGetControlFromBinding(pBinding);
//--
      if (!pControl)
         continue;

      void** ppUserData = GrannyGetControlUserDataArray(pControl);

      if((long)ppUserData[GrannyControlUserData_IsBlendControl] == 1)
         continue;

      if ((long)ppUserData[GrannyControlUserData_AnimationTrack] != 1/*cMovementAnimationTrack*/)
         continue;

      BGrannyAnimation *pAnim = gGrannyManager.getAnimation((long)ppUserData[GrannyControlUserData_AnimIndex]);
      if (!pAnim)
         continue;

      if (pAnim->getMotionExtractionMode() != BGrannyAnimation::cNoExtraction)
      {
         float weight = GrannyGetControlWeight(pControl);
         averageExtractedMotion += pAnim->getTotalMotionExtraction() * weight;
         totalWeight += weight;
      }
   }

   if (totalWeight > cFloatCompareEpsilon)
      averageExtractedMotion /= totalWeight;

   return averageExtractedMotion;
}

//============================================================================
// BGrannyInstance::stopAnimations
//============================================================================
long BGrannyInstance::stopAnimations(long animationTrack, float stopOverSeconds)
{
   // Keep count of number of controls we are stopping.
   long count = 0;
   if(!mModelInstance)
      return(0);

   bool immediate = (stopOverSeconds <= cFloatCompareEpsilon) ? true : false;

   // Iterate over controls
   for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mModelInstance);
        pBinding != GrannyModelControlsEnd(mModelInstance);
        pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (!pControl)
         continue;

      void** ppUserData = GrannyGetControlUserDataArray(pControl);

      if((long)ppUserData[GrannyControlUserData_IsBlendControl] == 1)
         continue;

      if ((long)ppUserData[GrannyControlUserData_EasingOut] && !immediate)
         continue;

      if ((long)ppUserData[GrannyControlUserData_AnimationTrack] != animationTrack)
         continue;

      mStateChange++;

      const uint cMaxActiveControls = 32;
      if (count > cMaxActiveControls)
      {
         GrannyFreeControl(pControl);
      }
      else
      {
         // Ease it out.
         float easeTime=GrannyEaseControlOut(pControl, stopOverSeconds);

         // Tell it to go away at it's stopping time.
         GrannySetControlCompletionCheckFlag(pControl, true);
         GrannyCompleteControlAt(pControl, easeTime);

         ppUserData[GrannyControlUserData_EasingOut] = (void*)TRUE;
      }

      // Increment count.
      count++;
   }

   // Give back count.
   return(count);
}

//============================================================================
// BGrannyInstance::setAnimationRate
//============================================================================
void BGrannyInstance::setAnimationRate(long animationTrack, float animationRate)
{
   if (!mModelInstance)
      return;

   mStateChange++;

   // Iterate over controls
   for (granny_model_control_binding* pBinding = GrannyModelControlsBegin(mModelInstance);
      pBinding != GrannyModelControlsEnd(mModelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (!pControl)
         continue;

      void** ppUserData = GrannyGetControlUserDataArray(pControl);

      if((long)ppUserData[GrannyControlUserData_IsBlendControl] == 1)
         continue;

      // Skip it if it's not the right animation track
      if ((long)ppUserData[GrannyControlUserData_AnimationTrack] != animationTrack)
         continue;

      // Set animation rate
      GrannySetControlSpeed(pControl, animationRate);
   }
}

//============================================================================
// BGrannyInstance::freeGrannyControls
//============================================================================
void BGrannyInstance::freeGrannyControls()
{
   if(mModelInstance == NULL)
      return;
   
   mStateChange++;

   // Iterate over controls
   granny_model_control_binding* pBinding = GrannyModelControlsBegin(mModelInstance);
   while (pBinding != GrannyModelControlsEnd(mModelInstance))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (!pControl)
         break;
      
      // Free it
      GrannyFreeControl(pControl);

      pBinding = GrannyModelControlsBegin(mModelInstance);
   }
}

//============================================================================
// BGrannyInstance::recenterClock
//============================================================================
void BGrannyInstance::recenterClock(float delta)
{
   mStateChange++;

   // Iterate over controls
   for(granny_model_control_binding* pBinding = GrannyModelControlsBegin(mModelInstance);
      pBinding != GrannyModelControlsEnd(mModelInstance);
      pBinding = GrannyModelControlsNext(pBinding))
   {
      // Get the control.
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (!pControl)
         continue;
      float time = GrannyGetControlClock(pControl);
      time -= delta;
      GrannySetControlClockOnly(pControl, time);
   }
}

//==============================================================================
// BGrannyInstance::update
//==============================================================================
void BGrannyInstance::update(float elapsedTime, bool synced)
{
   setClock(mAnimClock+elapsedTime);
   mActiveAnimTime += elapsedTime;
   mStateChange++;

   #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BGrannyInstance::update mAnimClock", mAnimClock);
      }
   #endif
}

//==============================================================================
// BGrannyInstance::getExtractedMotion
//==============================================================================
void BGrannyInstance::getExtractedMotion(float elapsedTime, BMatrix &matrix) const
{
   if (mModelInstance == NULL)
      return;

   // Initialize the input matrix from the passed in matrix so any extracted motion
   // is added to the orientation and position data already in the matrix.
   granny_matrix_4x4 mat1;
   matrix.getD3DXMatrix((D3DMATRIX&) mat1);

   GrannyUpdateModelMatrix(mModelInstance, elapsedTime, *mat1, *mat1, false);

   matrix.setD3DXMatrix((const D3DMATRIX&) mat1);
}

//==============================================================================
// BGrannyInstance::setMeshRenderMask
//==============================================================================
void BGrannyInstance::setMeshRenderMask(const BBitArray& mask)
{
   // This gets called when reading files.  It's crucial the number of bits matches
   // the number of meshes in the current model.  Don't do a straight copy because
   // the number of meshes might have changed from when the file was saved.
   // If the number of meshes did change then there might be problems here because
   // the mesh order might have changed.
   long maskIndex;
   long count = min(mMeshRenderMask.getNumber(), mask.getNumber());
   for (maskIndex = 0; maskIndex < count; maskIndex++)
   {
      if (mask.isBitSet(maskIndex))
         mMeshRenderMask.setBit(maskIndex);
      else
         mMeshRenderMask.clearBit(maskIndex);
   }
   
   mMeshRenderMaskAllSet = mMeshRenderMask.areAllBitsSet();
}

//==============================================================================
// BGrannyInstance::setMeshVisible
//==============================================================================
void BGrannyInstance::setMeshVisible(uint meshIndex, bool flag)
{
   //-- make sure the meshIndex is valid
   uint count = GrannyGetSourceModel(mModelInstance)->MeshBindingCount;
   if (meshIndex >= count)
   {
#ifndef BUILD_FINAL
      char buf[1024 * 5];
      const char* filename = getFilename();
      sprintf_s(buf, sizeof(buf), "BGrannyInstance::setMeshVisible (Mesh index out of bounds): meshIndex = %i count = %i name = %s", meshIndex, count, filename ? filename : "");
      BFAIL(buf);
#endif
      return;
   }
  
   if (flag)
   {
      mMeshRenderMask.setBit(meshIndex);
      
      if (!mMeshRenderMaskAllSet)
         mMeshRenderMaskAllSet = mMeshRenderMask.areAllBitsSet();
   }
   else
   {
      mMeshRenderMask.clearBit(meshIndex);
      mMeshRenderMaskAllSet = false;
   }
}

//==============================================================================
// BGrannyInstance::setMeshRenderMaskToUndamageState
//==============================================================================
void BGrannyInstance::setMeshRenderMaskToUndamageState()
{
//-- FIXING PREFIX BUG ID 7718
   const BGrannyModel* pGrannyModel=gGrannyManager.getModel(mModelIndex);
//--
   if(pGrannyModel)
   {
      BBitArray previousMask = mMeshRenderMask;
      mMeshRenderMask = pGrannyModel->getUndamageRenderMask();
      mMeshRenderMaskAllSet = mMeshRenderMask.areAllBitsSet();

      if (mpRenderMaskResetCallback)
         mpRenderMaskResetCallback(this, previousMask);
   }
}

//==============================================================================
// BGrannyInstance::setMeshRenderMaskToDamageNoHitsState
//==============================================================================
void BGrannyInstance::setMeshRenderMaskToDamageNoHitsState()
{
//-- FIXING PREFIX BUG ID 7719
   const BGrannyModel* pGrannyModel=gGrannyManager.getModel(mModelIndex);
//--
   if(pGrannyModel)
   {
      BBitArray previousMask = mMeshRenderMask;
      mMeshRenderMask = pGrannyModel->getDamageNoHitsRenderMask();
      mMeshRenderMaskAllSet = mMeshRenderMask.areAllBitsSet();

      if (mpRenderMaskResetCallback)
         mpRenderMaskResetCallback(this, previousMask);
   }
}

//==============================================================================
// BGrannyInstance::getMeshCount
//==============================================================================
long BGrannyInstance::getMeshCount( void ) const
{
   if (!mModelInstance)
      return (0);
 
   return GrannyGetSourceModel(mModelInstance)->MeshBindingCount;
}

//==============================================================================
// BGrannyInstance::computeBoundingBox
//==============================================================================
void BGrannyInstance::computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners)
{
   bool cornersChanged = false;
   if (initCorners)
   {
      pMinCorner->set(1e+10f, 1e+10f, 1e+10f);
      pMaxCorner->set(-1e+10f, -1e+10f, -1e+10f);
   }
         
   BVector min, max;
   BBoundingBox boneBox;

   BGrannyModel* pGrannyModel=gGrannyManager.getModel(mModelIndex);
   if(!pGrannyModel)
   {
      pMinCorner->set(0.0f, 0.0f, 0.0f);
      pMaxCorner->set(0.0f, 0.0f, 0.0f);   
      return;
   }
   
   granny_mesh_binding** pMeshBindings=pGrannyModel->getMeshBindings();
   if(!pMeshBindings)
   {
      pMinCorner->set(0.0f, 0.0f, 0.0f);
      pMaxCorner->set(0.0f, 0.0f, 0.0f);
      return;
   }

//-- FIXING PREFIX BUG ID 7722
   const granny_world_pose* pWorldPose=computeWorldPose();
//--
   granny_model* pModel=GrannyGetSourceModel(mModelInstance);

   for(long i=0; i<pModel->MeshBindingCount; i++)
   {
      granny_mesh* pMesh=pModel->MeshBindings[i].Mesh;

//-- FIXING PREFIX BUG ID 7721
      const granny_mesh_binding* pMeshBinding=pMeshBindings[i];
//--
      if(!pMeshBinding)
         continue;

      for(long j=0; j<pMesh->BoneBindingCount; j++)
      {
         if(!mMeshRenderMask.isBitSet(i))
            continue;

         granny_int32x const* pIndices = GrannyGetMeshBindingToBoneIndices(pMeshBinding);
         if (!pIndices)
         {
            BFAIL("BGrannyInstance::computeBoundingBox GrannyGetMeshBindingToBoneIndices failed!");
            continue;
         }
         int boneIndex = pIndices[j];
         //int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];

//-- FIXING PREFIX BUG ID 7720
         const BMatrix* pMatrix = (BMatrix*)GrannyGetWorldPose4x4(pWorldPose, boneIndex);
//--

         granny_bone_binding& boneBinding=pMesh->BoneBindings[j];

         const XMVECTOR OBBMin(XMLoadFloat3((const XMFLOAT3*)&boneBinding.OBBMin));
         const XMVECTOR OBBMax(XMLoadFloat3((const XMFLOAT3*)&boneBinding.OBBMax));

         boneBox.initializeTransformed(OBBMin, OBBMax, *pMatrix);

         boneBox.computeWorldCorners(min, max);

         if(XMVector3IsNaN(min))
            continue;

         pMinCorner->x = Math::Min(pMinCorner->x, min.x);
         pMinCorner->y = Math::Min(pMinCorner->y, min.y);
         pMinCorner->z = Math::Min(pMinCorner->z, min.z);

         pMaxCorner->x = Math::Max(pMaxCorner->x, max.x);
         pMaxCorner->y = Math::Max(pMaxCorner->y, max.y);
         pMaxCorner->z = Math::Max(pMaxCorner->z, max.z);
         
         cornersChanged = true;

#ifdef DEBUG_RENDER_BOUNDING_BOX
         boneBox.draw(cDWORDYellow, true);
#endif
      }
   }
   
   if ((!cornersChanged) && (initCorners))
   {
      pMinCorner->set(0.0f, 0.0f, 0.0f);
      pMaxCorner->set(0.0f, 0.0f, 0.0f);
   }
}

//============================================================================
// BGrannyInstance::getBoneHandle
//============================================================================
long BGrannyInstance::getBoneHandle(const char* pBoneName)
{
   BGrannyModel* pGrannyModel=gGrannyManager.getModel(mModelIndex);
   if(pGrannyModel)
      return pGrannyModel->getBoneHandle(pBoneName);
   else
      return -1;
}

//==============================================================================
// BGrannyInstance::getBone
//==============================================================================
bool BGrannyInstance::getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix, BBoundingBox* pBox, const BMatrix* pWorldMatrix, bool applyIK)
{
   SCOPEDSAMPLE(BGrannyInstance_getBone); 

   if(!mModelInstance || boneHandle==-1)
      return false;

   long boneIndex=BONEFROMGRANNYBONEHANDLE(boneHandle);
   if(boneIndex < 0)
      return false;

//-- FIXING PREFIX BUG ID 7723
   const granny_skeleton* pSkeleton=GrannyGetSourceSkeleton(mModelInstance);
//--
   if(boneIndex >= pSkeleton->BoneCount)
      return false;

   BMatrix workMatrix;
   BMatrix* pWorkMatrix;
   if (pMatrix)
      pWorkMatrix=pMatrix;
   else
   {
      workMatrix.makeIdentity();
      pWorkMatrix=&workMatrix;
   }

   gGrannyManager.incStatGetBoneCallCount();

   // Do we need to IK?
   long numIKNodes = getNumIKNodes();
   bool useIK = false;
   if (applyIK && numIKNodes)
   {
      for (long i = numIKNodes - 1; i >= 0; i--)
      {
         const BIKNode &IKNode = getIKNode(i);
         if (IKNode.mIsActive && (BONEFROMGRANNYBONEHANDLE(IKNode.mBoneHandle) <= boneIndex))
         {
            if ((IKNode.mNodeType == BIKNode::cIKNodeTypeSingleBone) || pWorldMatrix)
               useIK = true;
            break;
         }
      }
   }

   granny_local_pose* pLocalPose = NULL;

   // Handle sample anim caching
   bool cacheEnabled = false;
   bool usedCache = false;
   if (gGrannyManager.getEnableSampleAnimCache())
   {
      if (!mpSampleAnimCache)
         mpSampleAnimCache = gGrannyManager.createSampleAnimCache();
      if (mpSampleAnimCache)
      {
         pLocalPose = mpSampleAnimCache->mLocalPose;
         cacheEnabled = true;
         if (mpSampleAnimCache->mLocalPoseSet)
         {
            if (mpSampleAnimCache->mStateChange != mStateChange || mpSampleAnimCache->mUseIK != useIK || boneIndex > mpSampleAnimCache->mMaxBoneIndex)
            {
               mpSampleAnimCache->mLocalPoseSet = false;
               mpSampleAnimCache->mStateChange = mStateChange;
               if (boneIndex > mpSampleAnimCache->mMaxBoneIndex)
                  mpSampleAnimCache->mMaxBoneIndex = (uint8)boneIndex;
            }
            else
            {
               if (useIK)
               {
                  granny_world_pose* pWorldPose = gGrannyManager.getWorldPose();
                  GrannyBuildWorldPose(pSkeleton, 0, boneIndex + 1, pLocalPose, (const granny_real32*)pWorldMatrix, pWorldPose);
                  *pWorkMatrix = *((BMatrix*) GrannyGetWorldPose4x4(pWorldPose, boneIndex));
               }
               else
                  GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, pLocalPose, (granny_real32*)pWorldMatrix, (granny_real32*)pWorkMatrix, NULL, NULL);
               gGrannyManager.incStatGetBoneCachedCount();
               usedCache = true;
            }
         }
      }
   }

   if (!pLocalPose)
      pLocalPose = gGrannyManager.getLocalPose();

   if (!usedCache)
   {
      // Apply IK
      if (useIK)
      {
         granny_world_pose* pWorldPose=gGrannyManager.getWorldPose();
         bool worldPoseDirty = true;

         long sampleToBone;
         if (cacheEnabled && mpSampleAnimCache)
         {
            GrannySampleModelAnimations(mModelInstance, 0, mpSampleAnimCache->mMaxBoneIndex + 1, pLocalPose);
            mpSampleAnimCache->mLocalPoseSet = true;
            sampleToBone = mpSampleAnimCache->mMaxBoneIndex;
         }
         else
         {
            GrannySampleModelAnimations(mModelInstance, 0, boneIndex + 1, pLocalPose);
            sampleToBone = boneIndex;
         }

         for (long i = 0; i < numIKNodes; i++)
         {
            const BIKNode &IKNode = getIKNode(i);
            long IKBoneIndex = BONEFROMGRANNYBONEHANDLE(IKNode.mBoneHandle);
            if (IKNode.mIsActive && (IKBoneIndex <= sampleToBone))
            {
               switch (IKNode.mNodeType)
               {
               case BIKNode::cIKNodeTypeSingleBone:
                  {
                     granny_transform* boneTransform = GrannyGetLocalPoseTransform(pLocalPose, BONEFROMGRANNYBONEHANDLE(IKNode.mBoneHandle));
                     granny_transform  IKTransform;
                     GrannyMakeIdentity(&IKTransform);
                     IKTransform.Flags = GrannyHasOrientation;
                     IKTransform.Orientation[0] = IKNode.getAnchorPos().x;
                     IKTransform.Orientation[1] = IKNode.getAnchorPos().y;
                     IKTransform.Orientation[2] = IKNode.getAnchorPos().z;
                     IKTransform.Orientation[3] = IKNode.getAnchorPos().w;
                     granny_triple savedPosition;
                     savedPosition[0] = boneTransform->Position[0];
                     savedPosition[1] = boneTransform->Position[1];
                     savedPosition[2] = boneTransform->Position[2];
                     GrannyPreMultiplyBy(boneTransform, &IKTransform);
                     boneTransform->Position[0] = savedPosition[0];
                     boneTransform->Position[1] = savedPosition[1];
                     boneTransform->Position[2] = savedPosition[2];
                     GrannyBuildWorldPose(pSkeleton, 0, boneIndex + 1, pLocalPose, (const granny_real32*)pWorldMatrix, pWorldPose);
                     worldPoseDirty = false;
                  }
                  break;

               default:
                  // jce [11/14/2008] -- the world pose is actually unneeded here because we have our target in local space and don't pass any world matrix in since we're
                  // just trying to cache the local pose.  
                  GrannyBuildWorldPose(pSkeleton, 0, pSkeleton->BoneCount, pLocalPose, (const granny_real32*)NULL, gGrannyManager.getWorldPose());
                  BVector targetPos = IKNode.getTargetPos();
                  GrannyIKUpdate(IKNode.mLinkCount, IKBoneIndex, (const granny_real32*)&targetPos, 20, pSkeleton, (const granny_real32*)NULL, pLocalPose, pWorldPose);
                  
                  // jce [11/14/2008] -- just force a new world pose to get it updated with IK
                  worldPoseDirty = true;
                  
                  break;
               }
            }
         }

         if (worldPoseDirty)
         {
            GrannyBuildWorldPose(pSkeleton, 0, boneIndex + 1, pLocalPose, (const granny_real32*)pWorldMatrix, pWorldPose);
            worldPoseDirty = false;
         }

         *pWorkMatrix = *((BMatrix*) GrannyGetWorldPose4x4(pWorldPose, boneIndex));
      }
      else
      {
         /*
         // SLB: This is a test
         if (mpAttachmentTransformTrack)
         {
            float wrappedT = fmodf(mActiveAnimTime, mActiveAnimDuration);
            granny_transform result;
            GrannyMakeIdentity(&result);
            GrannyEvaluateCurveAtT(3, false, true, &mpAttachmentTransformTrack->PositionCurve, true, mActiveAnimDuration, wrappedT, result.Position, GrannyCurveIdentityPosition);
            GrannyEvaluateCurveAtT(4, true, true, &mpAttachmentTransformTrack->OrientationCurve, true, mActiveAnimDuration, wrappedT, result.Orientation, GrannyCurveIdentityOrientation);
            GrannyBuildCompositeTransform4x4(&result, (granny_real32*)pWorkMatrix);
         }
         else
         */
         {
            //bool debuging = false;
            //if (debuging)
            //{
            //   int ControlCount = 0;
            //   for(granny_model_control_binding *Binding = GrannyModelControlsBegin(mModelInstance); Binding != GrannyModelControlsEnd(mModelInstance); Binding = GrannyModelControlsNext(Binding))
            //   {
            //      granny_control *Control = GrannyGetControlFromBinding(Binding);
            //      printf("%d %s: %f %f %f %f %d %d\n", ControlCount++,
            //         GrannyGetAnimationBindingFromControlBinding(Binding)->ID.Animation->Name,
            //         GrannyGetControlRawLocalClock(Control),
            //         GrannyGetControlWeight(Control),
            //         GrannyGetControlEffectiveWeight(Control),
            //         GrannyGetControlLocalDuration(Control),
            //         GrannyGetControlLoopCount(Control),
            //         GrannyGetControlLoopIndex(Control));
            //   }
            //}

            if (cacheEnabled && mpSampleAnimCache)
            {
               GrannySampleModelAnimations(mModelInstance, 0, mpSampleAnimCache->mMaxBoneIndex + 1, pLocalPose);
               mpSampleAnimCache->mLocalPoseSet = true;
               
               GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, pLocalPose, (granny_real32*)pWorldMatrix, (granny_real32*)pWorkMatrix, NULL, NULL);
            }
            else
            {
               // jce [11/6/2008] -- sample only for the bone we need.
               BMatrix test1;
               granny_int32x sparseBoneArray[BGrannyManager::cMaxBones];
               granny_int32x sparseBoneArrayReverse[BGrannyManager::cMaxBones];
               granny_int32x sparseBoneCount = GrannySparseBoneArrayCreateSingleBone(pSkeleton, boneIndex, sparseBoneArray, sparseBoneArrayReverse);
               GrannySampleModelAnimationsLODSparse(mModelInstance, 0, sparseBoneCount, pLocalPose, 0.0f, sparseBoneArray);
               GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, pLocalPose, (granny_real32*)pWorldMatrix, (granny_real32*)pWorkMatrix, sparseBoneArray, sparseBoneArrayReverse);
            }

            //if (debuging)
            //{
            //   for (long bone = 0; bone <= boneIndex; bone++)
            //   {
            //      granny_transform* XForm = GrannyGetLocalPoseTransform(pLocalPose, bone);
            //      printf("%d: %d [%f %f %f] [%f %f %f %f] [[%f %f %f] [%f %f %f] [%f %f %f]]\n",
            //         bone, XForm->Flags,
            //         XForm->Position[0],
            //         XForm->Position[1],
            //         XForm->Position[2],
            //         XForm->Orientation[0],
            //         XForm->Orientation[1],
            //         XForm->Orientation[2],
            //         XForm->Orientation[3],
            //         XForm->ScaleShear[0][0],
            //         XForm->ScaleShear[0][1],
            //         XForm->ScaleShear[0][2],
            //         XForm->ScaleShear[1][0],
            //         XForm->ScaleShear[1][1],
            //         XForm->ScaleShear[1][2],
            //         XForm->ScaleShear[2][0],
            //         XForm->ScaleShear[2][1],
            //         XForm->ScaleShear[2][2]);

            //      //granny_real32 const* Offset = (granny_real32*)pWorldMatrix;
            //      //granny_real32 const* Work = (granny_real32*)pWorkMatrix;
            //      //pWorkMatrix->makeIdentity();
            //      //if (Offset)
            //      //   printf("o: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
            //      //   Offset[0], Offset[1], Offset[2], Offset[3],
            //      //   Offset[4], Offset[5], Offset[6], Offset[7],
            //      //   Offset[8], Offset[9], Offset[10], Offset[11],
            //      //   Offset[12], Offset[13], Offset[14], Offset[15]);
            //      //printf("w: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
            //      //   Work[0], Work[1], Work[2], Work[3],
            //      //   Work[4], Work[5], Work[6], Work[7],
            //      //   Work[8], Work[9], Work[10], Work[11],
            //      //   Work[12], Work[13], Work[14], Work[15]);

            //      //*GrannyGetLocalPoseTransform(pLocalPose, bone) = pSkeleton->Bones[bone].LocalTransform;
            //      //GrannyGetWorldMatrixFromLocalPose(pSkeleton, bone, pLocalPose, (granny_real32*)pWorldMatrix, (granny_real32*)pWorkMatrix, NULL, NULL);

            //      //if (Offset)
            //      //   printf("o: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
            //      //   Offset[0], Offset[1], Offset[2], Offset[3],
            //      //   Offset[4], Offset[5], Offset[6], Offset[7],
            //      //   Offset[8], Offset[9], Offset[10], Offset[11],
            //      //   Offset[12], Offset[13], Offset[14], Offset[15]);
            //      //printf("w: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
            //      //   Work[0], Work[1], Work[2], Work[3],
            //      //   Work[4], Work[5], Work[6], Work[7],
            //      //   Work[8], Work[9], Work[10], Work[11],
            //      //   Work[12], Work[13], Work[14], Work[15]);
            //   }

            //}

            // jce [11/6/2008] -- moved above inside if statement
            //GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, pLocalPose, (granny_real32*)pWorldMatrix, (granny_real32*)pWorkMatrix, NULL, NULL);
         }
      }
   }

   /*
   //-- BTK -- We need to find a better way to guarantee normalized matrices
   //-- sometimes granny passes back none orthonormal matrices so we
   //-- need to make sure that the orientation vectors are normalized
   BVector forward, up, right, translation;
   pWorkMatrix->getUp(up);
   pWorkMatrix->getForward(forward);
   pWorkMatrix->getRight(right);
   pWorkMatrix->getTranslation(translation);

   right.normalize();
   up.normalize();
   forward.normalize();

   pWorkMatrix->makeOrient(forward, up, right);
   pWorkMatrix->setTranslation(translation);
   */

   if(pPos)
      pWorkMatrix->getTranslation(*pPos);

   if(pBox)
   {
      long meshIndex=MESHFROMGRANNYBONEHANDLE(boneHandle);
      long bindingIndex=BINDINGFROMGRANNYBONEHANDLE(boneHandle);
      if(meshIndex!=-1 && bindingIndex!=-1)
      {
         granny_model* pModel=GrannyGetSourceModel(mModelInstance);
         granny_mesh* pMesh=pModel->MeshBindings[meshIndex].Mesh;
         granny_bone_binding& boneBinding=pMesh->BoneBindings[bindingIndex];
         pBox->initialize(*((BVector*)boneBinding.OBBMin), *((BVector*)boneBinding.OBBMax));
         pBox->transform(*pWorkMatrix);
      }
      else
      {
         BVector pos;
         pWorkMatrix->getTranslation(pos);
         pBox->initialize(pos, pos);
      }
   }

   return true;
}



//==============================================================================
//==============================================================================
bool BGrannyInstance::getBoneForRender(long boneHandle, BMatrix &matrix)
{
   SCOPEDSAMPLE(BGrannyInstance_getBoneInterpolated);

   // jce [11/3/2008] -- The local pose is supposed to be set up if you are calling this function.  If it's not, you're not calling this during the processing
   // of the render queue and at least for now that's not supported.
   if(!mRenderPrepareLocalPose)
   {
      BFAIL("Trying to get interpolated bone pos without a prepared local pose -- this is not supported right now");
      return(false);
   }

   if(!mModelInstance || boneHandle==-1)
      return false;

   long boneIndex=BONEFROMGRANNYBONEHANDLE(boneHandle);
   if(boneIndex < 0)
      return false;

   const granny_skeleton* pSkeleton=GrannyGetSourceSkeleton(mModelInstance);
   if(boneIndex >= pSkeleton->BoneCount)
      return false;

   BMatrix worldMatrix;
   worldMatrix.makeIdentity();
   GrannyGetWorldMatrixFromLocalPose(pSkeleton, boneIndex, mRenderPrepareLocalPose, (const granny_real32*)&worldMatrix, (granny_real32*)&matrix, NULL, NULL);

   return true;
}


//==============================================================================
// BGrannyInstance::verifyBoneCount
//==============================================================================
inline void BGrannyInstance::verifyBoneCount(long boneCount)
{
   if (boneCount > BGrannyManager::cMaxBones)
   {
//-- FIXING PREFIX BUG ID 7724
      const BGrannyModel* pModel = gGrannyManager.getModel(mModelIndex);
//--
      if (pModel != NULL)
      {
         BSimString errorMsg;
         errorMsg.format(B("Num bones in %s (%d) exceeds max number of bones allowed (%d)."), pModel->getFilename().getPtr(), boneCount, BGrannyManager::cMaxBones);
         BASSERTM(0, errorMsg.getPtr());
      }
      else
      {
         BSimString errorMsg;
         errorMsg.format(B("Num bones in <file unknown> (%d) exceeds max number of bones allowed (%d)."), boneCount, BGrannyManager::cMaxBones);
         BASSERTM(0, errorMsg.getPtr());
      }
   }
}

//==============================================================================
// BGrannyInstance::computeWorldPose
//==============================================================================
granny_world_pose* BGrannyInstance::computeWorldPose(D3DXMATRIX* pWorldMatrix, bool useIK)
{
   long boneCount = GrannyGetSourceSkeleton(mModelInstance)->BoneCount;
   #ifdef ENABLE_BASSERT_NORMAL
      verifyBoneCount(boneCount);
   #endif

   GrannySampleModelAnimations(mModelInstance, 0, boneCount, gGrannyManager.getLocalPose());

   if(pWorldMatrix)
	   GrannyBuildWorldPose(GrannyGetSourceSkeleton(mModelInstance), 0, boneCount, gGrannyManager.getLocalPose(), *pWorldMatrix, gGrannyManager.getWorldPose());
   else
	   GrannyBuildWorldPose(GrannyGetSourceSkeleton(mModelInstance), 0, boneCount, gGrannyManager.getLocalPose(), NULL, gGrannyManager.getWorldPose());


   // rg [1/16/08] - Determine if any IK nodes are actually active. If not, we can use the accelerated sampling API.
   if (useIK && (pWorldMatrix != NULL))
   {
      // The useIK functionality only works in world space, so if the world matrix isn't passed in, don't 
      // bother computing the IK since it will be wrong.
      //

      bool hasIKNodes = false;

      int numIKNodes = getNumIKNodes();
      for (int i = 0; i < numIKNodes; i++)
      {
         if (getIKNode(i).mIsActive)
         {
            hasIKNodes = true;
            break;
         }
      }         


      if (hasIKNodes)
      {
         //SCOPEDSAMPLE(GrannyProcessIKNodes);

//-- FIXING PREFIX BUG ID 7727
         const granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(mModelInstance);
//--
         int numIKNodes = getNumIKNodes();
         for (int i = 0; i < numIKNodes; i++)
         {
            const BIKNode *pIKNode = &getIKNode(i);
            if (pIKNode->mIsActive)
            {
               int boneIndex = BONEFROMGRANNYBONEHANDLE(pIKNode->mBoneHandle);
               BDEBUG_ASSERT((boneIndex >= 0) && (boneIndex < (long)boneCount));

               switch (pIKNode->mNodeType)
               {
                  case BIKNode::cIKNodeTypeSingleBone:
                  {
                     granny_transform* boneTransform = GrannyGetLocalPoseTransform(gGrannyManager.getLocalPose(), boneIndex);
                     granny_transform  IKTransform;
                     GrannyMakeIdentity(&IKTransform);
                     IKTransform.Flags = GrannyHasOrientation;
                     IKTransform.Orientation[0] = pIKNode->getAnchorPos().x;
                     IKTransform.Orientation[1] = pIKNode->getAnchorPos().y;
                     IKTransform.Orientation[2] = pIKNode->getAnchorPos().z;
                     IKTransform.Orientation[3] = pIKNode->getAnchorPos().w;
                     granny_triple savedPosition;
                     savedPosition[0] = boneTransform->Position[0];
                     savedPosition[1] = boneTransform->Position[1];
                     savedPosition[2] = boneTransform->Position[2];
                     GrannyPreMultiplyBy(boneTransform, &IKTransform);
                     boneTransform->Position[0] = savedPosition[0];
                     boneTransform->Position[1] = savedPosition[1];
                     boneTransform->Position[2] = savedPosition[2];
                     
                     if(pWorldMatrix)
                        GrannyBuildWorldPose(pSkeleton, 0, boneCount, gGrannyManager.getLocalPose(), (const granny_real32*)pWorldMatrix, gGrannyManager.getWorldPose());
                     else
                        GrannyBuildWorldPose(pSkeleton, 0, boneCount, gGrannyManager.getLocalPose(), NULL, gGrannyManager.getWorldPose());
                     break;
                  }
                  default:
                  {
                     // jce [11/14/2008] -- the world pose is actually unneeded here because we have our target in local space and don't pass any world matrix in since we're
                     // just trying to cache the local pose.  
                     GrannyBuildWorldPose(pSkeleton, 0, pSkeleton->BoneCount, gGrannyManager.getLocalPose(), (const granny_real32*)NULL, gGrannyManager.getWorldPose());
                     BVector targetPos = pIKNode->getTargetPos();
                     GrannyIKUpdate(pIKNode->mLinkCount, boneIndex, (const granny_real32*)&targetPos, 20, pSkeleton, (const granny_real32*)NULL, gGrannyManager.getLocalPose(), gGrannyManager.getWorldPose());
                     
                     // Make the real world pose, updated with IK.
                     GrannyBuildWorldPose(pSkeleton, 0, boneCount, gGrannyManager.getLocalPose(), (const granny_real32*)pWorldMatrix, gGrannyManager.getWorldPose());
                     break;
                  }
               }
            }
         }
      }
   }

   return (gGrannyManager.getWorldPose());
}

//==============================================================================
// BGrannyInstance::getCompositeBuffer
//==============================================================================
granny_matrix_4x4* BGrannyInstance::getCompositeBuffer(D3DXMATRIX* pWorldMatrix/*=NULL*/)
{
   return (GrannyGetWorldPoseComposite4x4Array(computeWorldPose(pWorldMatrix)));
}

//============================================================================
// BGrannyInstance::computeVertexCount
//============================================================================
long BGrannyInstance::computeVertexCount() const
{
   long totalVertexCount=0;
   for(long meshIndex = 0; meshIndex < GrannyGetSourceModel(mModelInstance)->MeshBindingCount; meshIndex++)
   {
//-- FIXING PREFIX BUG ID 7729
      const granny_mesh* pMesh = GrannyGetSourceModel(mModelInstance)->MeshBindings[meshIndex].Mesh;
//--
      long numVertices=GrannyGetMeshVertexCount(pMesh);
      totalVertexCount += numVertices;
   }
   return(totalVertexCount);
}

//============================================================================
// BGrannyInstance::computeTriangleCount
//============================================================================
long BGrannyInstance::computeTriangleCount() const
{
   long totalIndexCount=0;
   for(long meshIndex = 0; meshIndex < GrannyGetSourceModel(mModelInstance)->MeshBindingCount; meshIndex++)
   {
//-- FIXING PREFIX BUG ID 7730
      const granny_mesh* pMesh = GrannyGetSourceModel(mModelInstance)->MeshBindings[meshIndex].Mesh;
//--
      long numIndices=GrannyGetMeshIndexCount(pMesh);
      totalIndexCount += numIndices;
   }
   return(totalIndexCount/3);
}

//============================================================================
// BGrannyInstance::getFilename
//============================================================================
const BCHAR_T* BGrannyInstance::getFilename() const
{
	// Look up model.
//-- FIXING PREFIX BUG ID 7731
	const BGrannyModel* pModel = gGrannyManager.getModel(mModelIndex);
//--
	if(!pModel)
	   return(sEmptyString);

   // Get filename.	
	return(pModel->getFilename());
}

#include "visualManager.h"

//============================================================================
void BGrannyInstance::preRender(float interpolation)
{
   mIsInterpolating = true;
   mInterpolation = interpolation;
}

//============================================================================
float BGrannyInstance::getInterpolatedClock()
{   
   return Math::Lerp(mOldClock, mNewClock, mInterpolation);
}

//============================================================================
// BGrannyInstance::render
//============================================================================
void BGrannyInstance::render(const BVisualRenderAttributes* pRenderAttributes)
{
//-- FIXING PREFIX BUG ID 7732
   const BGrannyModel* pGrannyModel = gGrannyManager.getModel(mModelIndex);
//--
   if ((!pGrannyModel) || (pGrannyModel->getUGXGeomHandle() == cInvalidEventReceiverHandle) || (pGrannyModel->getUGXGeomStatus() != cUGXGeomStatusReady))
      return;
                  
   gGrannyInstanceRenderer.queue(this, pRenderAttributes, &gRender.getWorldXMMatrix());

#ifdef DEBUG_RENDER_BOUNDING_BOX
   BVector minCorner, maxCorner;
   computeBoundingBox(&minCorner, &maxCorner, true);

   BBoundingBox box;
   box.initialize(minCorner, maxCorner);
   box.draw(cDWORDCyan, true);
#endif
}

//==============================================================================
// BGrannyInstance::setIKNode
//==============================================================================
void BGrannyInstance::setIKNode(long node, BVector targetPos)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return;
   BIKNode &IKNode = mIKNodes.get(node);
   IKNode.setTargetPos(targetPos);
   mStateChange++;
}

//==============================================================================
// BGrannyInstance::setIKNode
//==============================================================================
void BGrannyInstance::setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return;
   BIKNode &IKNode = mIKNodes.get(node);
   IKNode.setAnchorPos(cOriginVector);
   IKNode.setOldAnchorPos(cOriginVector);
   IKNode.setTargetPos(targetPos);
   IKNode.setOldTargetPos(targetPos);
   IKNode.mStart = 0.0f;
   IKNode.mSweetSpot = 0.0f;
   IKNode.mEnd = 0.0f;
   IKNode.mBoneHandle = boneHandle;
   IKNode.mLinkCount = linkCount;
   IKNode.mNodeType = nodeType;
   IKNode.mHasAnchor = false;
   IKNode.mIsActive = false;
   IKNode.mLockComplete = false;
   IKNode.mHeightOnlyLock = true;
   IKNode.mIdleTransitioning = false;
   IKNode.mIdleTransitionLockStarted = false;
   IKNode.mAnchorPosUpdated = false;
   IKNode.mTargetPosUpdated = false;
   mStateChange++;
}

//==============================================================================
//==============================================================================
BIKNode* BGrannyInstance::getIKNodeFromTypeBoneHandle(long type, long boneHandle)
{
   long numIKNodes = mIKNodes.getNumber();

   for (long i = 0; i < numIKNodes; i++)
   {
      BIKNode &IKNode = mIKNodes.get(i);
      if ((IKNode.mNodeType == type) && (boneHandle == IKNode.mBoneHandle))
         return &IKNode;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
BIKNode* BGrannyInstance::getIKNodeFromIndex(long node)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return NULL;

   BIKNode &IKNode = mIKNodes.get(node);
   return &IKNode;
}

//==============================================================================
// BGrannyInstance::lockIKNodeToGround
//==============================================================================
void BGrannyInstance::lockIKNodeToGround(long boneHandle, bool lock, float start, float end)
{
   long numIKNodes = mIKNodes.getNumber();

   for (long i = 0; i < numIKNodes; i++)
   {
      BIKNode &IKNode = mIKNodes.get(i);
      if ((IKNode.mNodeType == BIKNode::cIKNodeTypeGround) && (boneHandle == IKNode.mBoneHandle))
      {
         if (IKNode.mHasAnchor != lock)
         {
            IKNode.mHasAnchor = lock;

            // Reset lock complete if lock state changed
            if ((end - start) <= 0.0f)
               IKNode.mLockComplete = true;
            else
               IKNode.mLockComplete = false;
         }

         // Set anchor pos, lock start/end
//#define CLAMP_TO_ANCHOR
#ifdef CLAMP_TO_ANCHOR
         if (lock)
#endif
            IKNode.setAnchorPos(IKNode.getTargetPos());
         IKNode.mStart = start;
         IKNode.mEnd = end;

         return;
      }
   }
   mStateChange++;
}

//==============================================================================
//==============================================================================
void BGrannyInstance::setIKNodeLockComplete(long node, bool lockComplete)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return;
   BIKNode &IKNode = mIKNodes.get(node);
   IKNode.mLockComplete = lockComplete;
   mStateChange++;
}

//==============================================================================
// BGrannyInstance::isIKNodeActive
//==============================================================================
bool BGrannyInstance::isIKNodeActive(long node) const
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return false;
   const BIKNode &IKNode = mIKNodes.get(node);
   return IKNode.mIsActive;
}

//==============================================================================
// BGrannyInstance::setIKNodeActive
//==============================================================================
void BGrannyInstance::setIKNodeActive(long node, bool active)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return;
   BIKNode &IKNode = mIKNodes.get(node);
   IKNode.mIsActive = active;
   mStateChange++;
}

//==============================================================================
// BGrannyInstance::setIKNodeSingleBone
//==============================================================================
void BGrannyInstance::setIKNodeSingleBone(long node, BVector position, BQuaternion orientation)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return;
   BIKNode &IKNode = mIKNodes.get(node);
   IKNode.setTargetPos(position);
   IKNode.setAnchorPos(orientation);
   mStateChange++;
}

//==============================================================================
// BGrannyInstance::getIKNodeSingleBone
//==============================================================================
bool BGrannyInstance::getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return false;

   const BIKNode &IKNode = mIKNodes.get(node);
   if (IKNode.mIsActive)
   {
      position = IKNode.getTargetPos();
      orientation.x = IKNode.getAnchorPos().x;
      orientation.y = IKNode.getAnchorPos().y;
      orientation.z = IKNode.getAnchorPos().z;
      orientation.w = IKNode.getAnchorPos().w;
   }

   return IKNode.mIsActive;
}

//==============================================================================
// BGrannyInstance::setIKNodeSweetSpot
//==============================================================================
void BGrannyInstance::setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end)
{
   long numIKNodes = mIKNodes.getNumber();

   for (long i = 0; i < numIKNodes; i++)
   {
      BIKNode &IKNode = mIKNodes.get(i);
      if ((IKNode.mNodeType == BIKNode::cIKNodeTypeSweetSpot) && (boneHandle == IKNode.mBoneHandle))
      {
         IKNode.setAnchorPos(sweetSpotPos);
         IKNode.mStart = start;
         IKNode.mSweetSpot = sweetSpot;
         IKNode.mEnd = end;
         IKNode.mHasAnchor = true;
         IKNode.mIsActive = true;
         mStateChange++;
         return;
      }
   }
}

//==============================================================================
// BGrannyInstance::setIKNode
//==============================================================================
long BGrannyInstance::getIKNodeBoneHandle(long node) const
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return -1;
   const BIKNode &IKNode = mIKNodes.get(node);
   return IKNode.mBoneHandle;
}

//==============================================================================
// BGrannyInstance::getIKNodeAnchor
//==============================================================================
bool BGrannyInstance::getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return false;

   const BIKNode &IKNode = mIKNodes.get(node);
   if (IKNode.mHasAnchor)
   {
      anchorPos = IKNode.getAnchorPos();
      lockStartTime = IKNode.mStart;
      lockEndTime = IKNode.mEnd;
      lockComplete = IKNode.mLockComplete;
   }

   return IKNode.mHasAnchor;
}

//==============================================================================
// BGrannyInstance::getIKNodeSweetSpot
//==============================================================================
bool BGrannyInstance::getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end)
{
   if (node < 0 || node >= mIKNodes.getNumber())
      return false;

   const BIKNode &IKNode = mIKNodes.get(node);
   if (IKNode.mIsActive)
   {
      sweetSpotPos = IKNode.getAnchorPos();
      start = IKNode.mStart;
      sweetSpot = IKNode.mSweetSpot;
      end = IKNode.mEnd;
   }

   return IKNode.mIsActive;
}


//==============================================================================
// BGrannyInstance::raySegmentIntersects
//==============================================================================
bool BGrannyInstance::raySegmentIntersects(const BVector &originWorldSpace, const BVector &vectorWorldSpace, bool segment, const BMatrix &worldMatrix, float *distanceSqr, float &intersectDistanceSqr, long &intersectBoneHandle, BVector &intersectBoneSpacePos, BVector &intersectBoneSpaceDir)
{
   BBoundingBox boneBox;

   BGrannyModel* pGrannyModel=gGrannyManager.getModel(mModelIndex);
   if(!pGrannyModel)
   {
      return false;
   }
   
   granny_mesh_binding** pMeshBindings=pGrannyModel->getMeshBindings();
   if(!pMeshBindings)
   {
      return false;
   }


   bool foundHit = false;
   float closestInstersectionDistanceSqr = cMaximumFloat;
   long closestBoneHandle = -1;
//-- FIXING PREFIX BUG ID 7711
   const BMatrix* closetsBoneMatrix = NULL;
//--



//-- FIXING PREFIX BUG ID 7712
   const granny_world_pose* pWorldPose=computeWorldPose((D3DXMATRIX*)&worldMatrix, true);
//--
   granny_model* pModel=GrannyGetSourceModel(mModelInstance);

   for(long i=0; i<pModel->MeshBindingCount; i++)
   {
      if(!mMeshRenderMask.isBitSet(i))
         continue;

      granny_mesh* pMesh=pModel->MeshBindings[i].Mesh;

//-- FIXING PREFIX BUG ID 7710
      const granny_mesh_binding* pMeshBinding=pMeshBindings[i];
//--
      if(!pMeshBinding)
         continue;

      for(long j=0; j<pMesh->BoneBindingCount; j++)
      {
         granny_bone_binding& boneBinding=pMesh->BoneBindings[j];

         const BVector OBBMin(XMLoadFloat3((const XMFLOAT3*)&boneBinding.OBBMin));
         const BVector OBBMax(XMLoadFloat3((const XMFLOAT3*)&boneBinding.OBBMax));

         // Disregard boxes with no volume
         if(OBBMin.almostEqual(OBBMax))
            continue;

         granny_int32x const* pIndices = GrannyGetMeshBindingToBoneIndices(pMeshBinding);
         if (!pIndices)
         {
            BFAIL("BGrannyInstance::raySegmentIntersects GrannyGetMeshBindingToBoneIndices failed!");
            continue;
         }
         int boneIndex = pIndices[j];
         //int boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];
         BMatrix* pBoneWorldSpaceMat = (BMatrix*)GrannyGetWorldPose4x4(pWorldPose, boneIndex);

         boneBox.initializeTransformed(OBBMin, OBBMax, *pBoneWorldSpaceMat);

         float curIntersectDistanceSqr;
         if(boneBox.raySegmentIntersects(originWorldSpace, vectorWorldSpace, segment, NULL, curIntersectDistanceSqr))
         {
            if(curIntersectDistanceSqr < closestInstersectionDistanceSqr)
            {
               foundHit = true;
               closestInstersectionDistanceSqr = curIntersectDistanceSqr;
               closestBoneHandle = GRANNYBONEHANDLE(boneIndex, i, j);;
               closetsBoneMatrix = pBoneWorldSpaceMat;
            }
         }
      }
   }


   if(foundHit)
   {
      intersectDistanceSqr = closestInstersectionDistanceSqr;
      intersectBoneHandle = closestBoneHandle;

      // Compute bone space position and direction
      BVector originBoneSpace, vectorBoneSpace;

      BMatrix invBoneWorldSpaceMat = *closetsBoneMatrix;
      invBoneWorldSpaceMat.invert();

      invBoneWorldSpaceMat.transformVectorAsPoint(originWorldSpace, originBoneSpace);
      invBoneWorldSpaceMat.transformVector(vectorWorldSpace, vectorBoneSpace);

      intersectBoneSpaceDir = vectorBoneSpace;
      intersectBoneSpaceDir.normalize();

      vectorBoneSpace.normalize();
      vectorBoneSpace.scale(sqrt(closestInstersectionDistanceSqr));
      intersectBoneSpacePos = originBoneSpace + vectorBoneSpace;

      return true;
   }
   else
   {
      return false;
   }
}

//==============================================================================
//==============================================================================
void BGrannyInstance::clearSampleAnimCache()
{
   if (mpSampleAnimCache)
   {
      gGrannyManager.releaseSampleAnimCache(mpSampleAnimCache);
      mpSampleAnimCache = NULL;
   }
}

//==============================================================================
//==============================================================================
void BGrannyInstance::renderPrepare()
{
   // Sanity check -- should have no pose when we come in here... at least in the current system
   // where we can't keep these frame to frame.
   BASSERT(!mRenderPrepareLocalPose);
   
   // Get a pose to work with.
   mRenderPrepareLocalPose = gGrannyManager.getRenderPreparePose(this);
   
   // We're supposed to get a valid pose back!
   BASSERT(mRenderPrepareLocalPose);
   
   //trace("   prepare %p", this);
}


//==============================================================================
//==============================================================================
void BGrannyInstance::renderPrepareSampleAnims()
{
   // Sanity check -- should have a local pose to work with at this point.
   BASSERT(mRenderPrepareLocalPose);

   const uint cMaxSaveStates = 100;
   BGrannySaveControlState saveStates[cMaxSaveStates];
   uint numSaveStates = 0;

   // Set the interpolated clock, saving off the old clock values.
   setInterpolatedClocks(mModelInstance, saveStates, cMaxSaveStates, numSaveStates);   

   // Sample animations into the cache for later use.
   const granny_skeleton* skeleton = GrannyGetSourceSkeleton(mModelInstance);
   GrannySampleModelAnimations(mModelInstance, 0, skeleton->BoneCount, mRenderPrepareLocalPose);

   // Update any IK nodes.  Essentially cut&paste from elsewhere.
   int numIKNodes = getNumIKNodes();
   for (int i = 0; i < numIKNodes; i++)
   {
      const BIKNode *pIKNode = &getIKNode(i);
      if (pIKNode->mIsActive)
      {
         int boneIndex = BONEFROMGRANNYBONEHANDLE(pIKNode->mBoneHandle);
         BDEBUG_ASSERT((boneIndex >= 0) && (boneIndex < (long)skeleton->BoneCount));

         switch (pIKNode->mNodeType)
         {
            case BIKNode::cIKNodeTypeSingleBone:
            {
               granny_transform* boneTransform = GrannyGetLocalPoseTransform(mRenderPrepareLocalPose, boneIndex);
               granny_transform  IKTransform;
               GrannyMakeIdentity(&IKTransform);
               IKTransform.Flags = GrannyHasOrientation;
               
               BVector interpolatedDir = pIKNode->getInterpolatedAnchorPosAsOrientation(mInterpolation);
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
               // jce [11/14/2008] -- the world pose is actually unneeded here because we have our target in local space and don't pass any world matrix in since we're
               // just trying to cache the local pose.  
               GrannyBuildWorldPose(skeleton, 0, skeleton->BoneCount, mRenderPrepareLocalPose, (const granny_real32*)NULL, gGrannyManager.getWorldPose());
               BVector targetPos = pIKNode->getInterpolatedTargetPos(mInterpolation);
               GrannyIKUpdate(pIKNode->mLinkCount, boneIndex, (const granny_real32*)&targetPos, 20, skeleton, (const granny_real32*)NULL, mRenderPrepareLocalPose, gGrannyManager.getWorldPose());
               break;
            }
         }
      }
   }

   // Restore the old clock values.
   unsetInterpolatedClocks(mModelInstance, saveStates, numSaveStates);

}


//==============================================================================
//==============================================================================
void BGrannyInstance::cleanupRenderPrepare()
{
   // We don't need the pose any more.
   const granny_skeleton* skeleton = GrannyGetSourceSkeleton(mModelInstance);
   if(skeleton)
      gGrannyManager.releaseRenderPreparePose(mRenderPrepareLocalPose, skeleton->BoneCount);
   else
   {
      BFAIL("null skeleton");
   }
   mRenderPrepareLocalPose = NULL;
   
   
   //trace("   cleanup %p", this);
}


//==============================================================================
//==============================================================================
bool BGrannyInstance::save(BStream* pStream, int saveType) const
{
   // Mesh render mask
   bool saveMask = (!getMeshRenderMaskAllSet());
   GFWRITEVAR(pStream, bool, saveMask);
   if (saveMask)
   {
      const BBitArray& meshRenderMask = getMeshRenderMask();
      GFWRITEVAL(pStream, uint8, meshRenderMask.getNumber());
      GFWRITEBITARRAY(pStream, meshRenderMask, uint8, 255);
   }

   // IK nodes
   GFWRITECLASSARRAY(pStream, saveType, mIKNodes, uint8, 200);

   // Clock override
   bool clockOverride = (mpClockOverride != NULL);
   GFWRITEVAR(pStream, bool, clockOverride);
   if (clockOverride)
      GFWRITECLASSPTR(pStream, saveType, mpClockOverride);

   return true;
}

//==============================================================================
//==============================================================================
bool BGrannyInstance::load(BStream* pStream, int saveType)
{
   // Mesh render mask
   bool loadMask;
   GFREADVAR(pStream, bool, loadMask);
   if (loadMask)
   {
      long maskCount;
      GFREADVAL(pStream, uint8, long, maskCount);
      BBitArray mask;
      mask.setNumber(maskCount, false);
      mask.clear();
      GFREADBITARRAY(pStream, mask, uint8, 255);
      setMeshRenderMask(mask);
   }

   if (mGameFileVersion >= 2)
   {
      // IK nodes
      GFREADCLASSARRAY(pStream, saveType, mIKNodes, uint8, 200);

      // Clock override
      bool clockOverride;
      GFREADVAR(pStream, bool, clockOverride);
      if (clockOverride)
      {
         mpClockOverride = HEAP_NEW(BClockOverride, gSimHeap);
         if (mpClockOverride)
            GFREADCLASSPTR(pStream, saveType, mpClockOverride)
         else
            GFREADTEMPCLASS(pStream, saveType, BClockOverride)
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BIKNode::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mAnchorPos);
   GFWRITEVECTOR(pStream, mTargetPos);
   GFWRITEVECTOR(pStream, mOldAnchorPos);
   GFWRITEVECTOR(pStream, mOldTargetPos);
   GFWRITEVAR(pStream, float, mStart);
   GFWRITEVAR(pStream, float, mSweetSpot);
   GFWRITEVAR(pStream, float, mEnd);
   GFWRITEVAR(pStream, long,  mBoneHandle);
   GFWRITEVAR(pStream, uint8, mLinkCount);
   GFWRITEVAR(pStream, uint8, mNodeType);
   GFWRITEBITBOOL(pStream, mIsActive);
   GFWRITEBITBOOL(pStream, mHasAnchor);
   GFWRITEBITBOOL(pStream, mLockComplete);
   GFWRITEBITBOOL(pStream, mHeightOnlyLock);
   GFWRITEBITBOOL(pStream, mIdleTransitioning);
   GFWRITEBITBOOL(pStream, mIdleTransitionLockStarted);
   GFWRITEBITBOOL(pStream, mAnchorPosUpdated);
   GFWRITEBITBOOL(pStream, mTargetPosUpdated);
   return true;
}

//==============================================================================
//==============================================================================
bool BIKNode::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mAnchorPos);
   GFREADVECTOR(pStream, mTargetPos);
   if(BGrannyInstance::mGameFileVersion > 2)
   {
      GFREADVECTOR(pStream, mOldAnchorPos);
      GFREADVECTOR(pStream, mOldTargetPos);
   }
   else
   {
      mOldAnchorPos = mAnchorPos;
      mOldTargetPos = mTargetPos;
   }
   GFREADVAR(pStream, float, mStart);
   GFREADVAR(pStream, float, mSweetSpot);
   GFREADVAR(pStream, float, mEnd);
   GFREADVAR(pStream, long,  mBoneHandle);
   GFREADVAR(pStream, uint8, mLinkCount);
   GFREADVAR(pStream, uint8, mNodeType);
   GFREADBITBOOL(pStream, mIsActive);
   GFREADBITBOOL(pStream, mHasAnchor);
   GFREADBITBOOL(pStream, mLockComplete);
   GFREADBITBOOL(pStream, mHeightOnlyLock);
   GFREADBITBOOL(pStream, mIdleTransitioning);
   GFREADBITBOOL(pStream, mIdleTransitionLockStarted);
   if(BGrannyInstance::mGameFileVersion > 2)
   {
      GFREADBITBOOL(pStream, mAnchorPosUpdated);
      GFREADBITBOOL(pStream, mTargetPosUpdated);
   }
   else
   {
      mAnchorPosUpdated = false;
      mTargetPosUpdated = false;
   }
   return true;
}


//==============================================================================
//==============================================================================
BIKNode::BIKNode() :
   mStart(0.0f),
   mSweetSpot(0.0f),
   mEnd(0.0f),
   mBoneHandle(0),
   mLinkCount(0),
   mNodeType(cIKNodeTypeNotSet),
   mIsActive(false),
   mHasAnchor(false),
   mLockComplete(false),
   mHeightOnlyLock(false),
   mIdleTransitioning(false),
   mIdleTransitionLockStarted(false),
   mAnchorPosUpdated(false),
   mTargetPosUpdated(false),
   mAnchorPos(cOriginVector),
   mTargetPos(cOriginVector),
   mOldAnchorPos(cOriginVector),
   mOldTargetPos(cOriginVector)
{
}


//==============================================================================
//==============================================================================
void BIKNode::setAnchorPos(BVector pos)
{
   if(!mAnchorPosUpdated)
   {
      mOldAnchorPos = mAnchorPos;
      mAnchorPosUpdated = true;
   }
   mAnchorPos = pos;
}


//==============================================================================
//==============================================================================
void BIKNode::setTargetPos(BVector pos)
{
   if(!mTargetPosUpdated)
   {
      mOldTargetPos = mTargetPos;
      mTargetPosUpdated = true;
   }
   mTargetPos = pos;
}


//==============================================================================
//==============================================================================
void BIKNode::setOldAnchorPos(BVector pos)
{
   mOldAnchorPos = pos;
}


//==============================================================================
//==============================================================================
void BIKNode::setOldTargetPos(BVector pos)
{
   mOldTargetPos = pos;
}


//==============================================================================
//==============================================================================
BVector BIKNode::getInterpolatedAnchorPosAsOrientation(float interpolation) const
{
   // jce [11/14/2008] -- the anchor pos when used for single bone "IK" is used as an orientation, so we
   // interpolate it as such.
   
   // TRB 11/18/08 - BQuaternion has evil constructors.  If you pass a BVector this will call the constructor
   // that treats the vector as euler angles and converts it to a quaternion.  Force it to call the regular
   // quaternion copy constructor so no crazy conversion is done.
   BQuaternion old((XMVECTOR) mOldAnchorPos);
   BQuaternion current((XMVECTOR) mAnchorPos);
   
   BQuaternion interpolated;
   old.slerp(current, interpolation, interpolated);

   return(interpolated);
}


//==============================================================================
//==============================================================================
BVector BIKNode::getInterpolatedTargetPos(float interpolation) const
{
   // Lerp based on current interpolation value
   BVector result;
   result.lerpPosition(interpolation, mOldTargetPos, mTargetPos);
   return(result);
}


//==============================================================================
//==============================================================================
bool BClockOverride::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mOldAnimPos);      
   GFWRITEVAR(pStream, float, mNewAnimPos);      
   GFWRITEVAR(pStream, long, mActionIndex);
   GFWRITEVAR(pStream, long, mMovementIndex);
   GFWRITEVAR(pStream, float, mElapsed);
   GFWRITEVAR(pStream, float, mAnimDuration);
   GFWRITEVAR(pStream, float, mOldClock);
   return true;
}

//==============================================================================
//==============================================================================
bool BClockOverride::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, float, mOldAnimPos);      
   GFREADVAR(pStream, float, mNewAnimPos);      
   GFREADVAR(pStream, long, mActionIndex);
   GFREADVAR(pStream, long, mMovementIndex);
   GFREADVAR(pStream, float, mElapsed);
   GFREADVAR(pStream, float, mAnimDuration);
   GFREADVAR(pStream, float, mOldClock);
   return true;
}
