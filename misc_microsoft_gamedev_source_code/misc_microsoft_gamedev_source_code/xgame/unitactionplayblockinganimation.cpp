//==============================================================================
// unitactionplayblockinganimation.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionplayblockinganimation.h"
#include "Physics.h"
#include "TerrainSimRep.h"
#include "unit.h"
#include "visualitem.h"
#include "world.h"
#include "unitactionrangedattack.h"
#include "xgranny.h"
#include "grannyanimation.h"
#include "visual.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionPlayBlockingAnimation, 4, &gSimHeap);



//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   //NOTE: THIS DOES NOT NEED A PROTOACTION.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //If we have an opp, take our controllers.  If we can't get them, fail.
   if ((mOppID != BUnitOpp::cInvalidID) && !grabControllers())
   {
      BAction::disconnect();
      return (false);
   }

   // TRB 12/1/08 - Set the unit's flag after the connect might fail so that it isn't left permanently modified.
   mFlagSaveIKDisabled=pUnit->getFlagIKDisabled();
   if (!mFlagSaveIKDisabled)
      pUnit->setFlagIKDisabled(true);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (!mFlagSaveIKDisabled)
      pUnit->setFlagIKDisabled(false);

   // Recompute anims--this ensures that we've stopped playing this anim
   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
   pUnit->computeAnimation();

   //Release our controllers.
   releaseControllers();

   //If we're not doing the opp, do the other hookup behavior.
   if (mOppID == BUnitOpp::cInvalidID)
   {
      if (pUnit->getFlagTiesToGround())
         pUnit->tieToGround();

      //DCPTODO: This feels all kinds of creepy in the new paradigm...
      // Tell the unit's squad's cActionTypeSquadPlayBlockingAnimation that we're done
      BSquad *pSquad = gWorld->getSquad(pUnit->getParentID());
      if (pSquad)
         pSquad->notify(BEntity::cEventActionDone, pUnit->getID(), BAction::cActionTypeSquadPlayBlockingAnimation, 0);
   }

   // Re-enable physics
   pUnit->setPhysicsKeyFramed(false);

   //-- No longer need to preserve DPS
   if(mFlagPreserveDPS)
      pUnit->setFlagPreserveDPS(false);

   // Unlock the animation if still locked and this action has locked it.
   if (pUnit->isAnimationLocked() && mFlagHasPlayedAnimation)
      pUnit->unlockAnimation();
   
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::init()
{
   if (!BAction::init())
      return(false);

   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;
   mAnimationEndTime = 0;
   mAnimationState = -1;
   mAnimationType = -1;
   mForceAnimationID = -1;
   mOverrideExitAction.mExitAction = cAnimExitActionLoop;
   mOverrideExitAction.mTweenToAnimation = -1;
   mOverrideExitAction.mTweenTime = 0.0f;
   mFlagForceReset=false;
   mFlagConflictsWithIdle=true;
   mFlagApplyAnimationInstantly=false;
   mTriggerModule.clearNotificationData();
   mFlagTargetInvert=false;
   mFlagSaveIKDisabled=false;
   mFlagLoop=false;
   mFlagAllowMove=false;
   mFlagPreserveDPS=false;
   mFlagOverrideExitAction=false;
   mFlagHasPlayedAnimation=false;
   mFlagDisableMotionExtractionCollisions=false;
   mFlagClearUnitNoRenderFlag=false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (state)
   {
      case cStateDone:
      case cStateFailed:
      case cStateNone:
         if (mOppID != BUnitOpp::cInvalidID)
         {
            if (state == cStateDone)
               pUnit->completeOpp(mOppID, true);
            else if (state == cStateFailed)
               pUnit->completeOpp(mOppID, false);

            releaseControllers();
         }
         else
         {
            if (state == cStateDone)
               mTriggerModule.notifyActionStatus(BTriggerVarActionStatus::cDoneSuccess);
            else if (state == cStateFailed)
               mTriggerModule.notifyActionStatus(BTriggerVarActionStatus::cDoneFailure);
         }

         // Re-enable physics
         pUnit->setPhysicsKeyFramed(false);

         //E3: Move this unit forward some to get it out of the way of other trained things.
         /*float moveDistance=10.0f+getRandRangeFloat(cSimRand, 0.0f, 20.0f);
         BVector movePosition=pUnit->getPhysicsForward()*moveDistance;
         float randAngle=getRandRangeFloat(cSimRand, -cRadiansPerDegree*60.0f, cRadiansPerDegree*20.0f);
         movePosition.rotateXZ(randAngle);
         movePosition+=pUnit->getPhysicsPosition();
         BUnitOpp opp;
         opp.init();
         opp.setTarget(BSimTarget(movePosition, 0.1f));
         opp.setType(BUnitOpp::cTypeMove);
         opp.setSource(pUnit->getID());
         opp.setPriority(BUnitOpp::cPriorityImmediate);
         opp.setOneTime(true);
         opp.generateID();
         pUnit->addOpp(opp);*/
         break;
   }
   
   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5112
   const BUnit* pTarget=gWorld->getUnit(mTarget.getID());
//--

   // Clear the no render flag
   if (mFlagClearUnitNoRenderFlag && pUnit->getFlagNoRender())
      pUnit->setFlagNoRender(false);

   switch (mState)
   {
      case cStateNone:
      {
         if (mOppID != BUnitOpp::cInvalidID)
         {
            //We need to have the controllers to do anything.
            if (!grabControllers())
               break;

            // Can't go on if the animation is locked
            if (pUnit->isAnimationLocked())
               break;

            //Face our target if we have one.
            if (pTarget)
            {
               BVector targetDirection=pTarget->getPosition()-pUnit->getPosition();
               targetDirection.y=0.0f;
               targetDirection.safeNormalize();
               if (mFlagTargetInvert)
                  targetDirection=-targetDirection;
               pUnit->setForward(targetDirection);
               pUnit->calcRight();
               pUnit->calcUp();
            }
         }

         playAnimation();
         setState(cStateWorking);
         break;
      }

      case cStateWorking:
      {
         //If we've lost the controllers, just fail.
         if ((mOppID != BUnitOpp::cInvalidID) && !validateControllers())
         {
            setState(cStateFailed);
            break;
         }

         if (!mFlagLoop && mAnimationEndTime <= gWorld->getGametime())
         {
            if (mOppID != BUnitOpp::cInvalidID)
            {
               const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
               if (pOpp && (pOpp->getType() == BUnitOpp::cTypeEvade))
               {
                  BVector unitPosition=pUnit->getPosition();
                  BVector refUnitPosition=pUnit->getPosition();
                  gTerrainSimRep.getHeight(refUnitPosition, true);
                  if (refUnitPosition.y > (unitPosition.y+2.0f))
                  {
                     pUnit->kill(false);
                     return false; // return false here so the action is removed and the owner unit is referenced in BActionList::update
                  }
               }
            }

            setState(cStateDone);
         }

         //-- Tell the unit it needs to preserve DPS while we're working
         if(mFlagPreserveDPS)
            pUnit->setFlagPreserveDPS(true);

         // TRB 10/27/08 - Set flag so that the motion extracted birth anims won't check for collisions
         if (mFlagDisableMotionExtractionCollisions)
            pUnit->setFlagMotionCollisionChecked(true);

         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::playAnimation()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   BActionID controllerActionID = pUnit->getController(BActionController::cControllerAnimation)->getActionID();
   long forceAnimID = -1;
   if(mForceAnimationID != -1 )
   {
      forceAnimID = mForceAnimationID;
   }
   else if ((controllerActionID == mID) || (controllerActionID == cInvalidActionID))
   {
      // Get animation data
      BVisualAnimationData animationData = pUnit->getVisual()->getAnimationData(cActionAnimationTrack, mAnimationType);
      forceAnimID = animationData.mAnimAsset.mIndex;
   }

//-- FIXING PREFIX BUG ID 5113
   const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(forceAnimID);
//--

   // Disable physics while playing anim if it uses motion extraction
   if (pGrannyAnim && (pGrannyAnim->getMotionExtractionMode() != GrannyNoAccumulation))
   {
      pUnit->setPhysicsKeyFramed(true, true);
   }

   pUnit->setAnimation(mID, mAnimationState, mAnimationType, mFlagApplyAnimationInstantly, mFlagForceReset, mForceAnimationID, true, false, mFlagOverrideExitAction ? &mOverrideExitAction : NULL);
   pUnit->computeAnimation();
   BASSERT((mOppID == BUnitOpp::cInvalidID) || pUnit->isAnimationSet(mAnimationState, mAnimationType));
   mAnimationEndTime=pUnit->getAnimationDurationDWORD(cActionAnimationTrack) + gWorld->getGametime();

   mFlagHasPlayedAnimation = true;
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::setAnimationState(long state, long type, bool applyInstantly, bool forceReset, long forceAnimID)
{
   mAnimationState = state;
   mAnimationType = type;
   mFlagApplyAnimationInstantly=applyInstantly;
   mFlagForceReset = forceReset;
   mForceAnimationID = forceAnimID;
   mFlagOverrideExitAction = false;
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));
   mTarget=target;
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionPlayBlockingAnimation::getPriority() const
{
//-- FIXING PREFIX BUG ID 5114
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5115
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //We need them all.
   if (!mFlagAllowMove && (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID))
      return (false);
   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Take them all.
   if (!(mFlagAllowMove || pUnit->grabController(BActionController::cControllerOrient, this, getOppID())))
      return (false);
   if (!pUnit->grabController(BActionController::cControllerAnimation, this, getOppID()))
   {
      pUnit->releaseController(BActionController::cControllerOrient, this);
      return (false);
   }
   
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
void BUnitActionPlayBlockingAnimation::overrideExitAction(BProtoVisualAnimExitAction* pOverrideExitAction)
{
   if (pOverrideExitAction)
      mOverrideExitAction = *pOverrideExitAction;

   mFlagOverrideExitAction = (pOverrideExitAction) ? true : false;
}


//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BProtoVisualAnimExitAction, mOverrideExitAction);
   GFWRITEVAR(pStream, BActionTriggerModule, mTriggerModule);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, DWORD, mAnimationEndTime);
   GFWRITEVAR(pStream, long, mAnimationState);
   GFWRITEVAR(pStream, long, mAnimationType);
   GFWRITEVAR(pStream, long, mForceAnimationID);
   GFWRITEBITBOOL(pStream, mFlagForceReset);
   GFWRITEBITBOOL(pStream, mFlagApplyAnimationInstantly);
   GFWRITEBITBOOL(pStream, mFlagTargetInvert);
   GFWRITEBITBOOL(pStream, mFlagSaveIKDisabled);
   GFWRITEBITBOOL(pStream, mFlagLoop);
   GFWRITEBITBOOL(pStream, mFlagAllowMove);
   GFWRITEBITBOOL(pStream, mFlagPreserveDPS);
   GFWRITEBITBOOL(pStream, mFlagOverrideExitAction);
   GFWRITEBITBOOL(pStream, mFlagHasPlayedAnimation);
   GFWRITEBITBOOL(pStream, mFlagDisableMotionExtractionCollisions);
   GFWRITEBITBOOL(pStream, mFlagClearUnitNoRenderFlag);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayBlockingAnimation::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BProtoVisualAnimExitAction, mOverrideExitAction);
   GFREADVAR(pStream, BActionTriggerModule, mTriggerModule);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, DWORD, mAnimationEndTime);
   GFREADVAR(pStream, long, mAnimationState);
   GFREADVAR(pStream, long, mAnimationType);
   GFREADVAR(pStream, long, mForceAnimationID);
   GFREADBITBOOL(pStream, mFlagForceReset);
   GFREADBITBOOL(pStream, mFlagApplyAnimationInstantly);
   GFREADBITBOOL(pStream, mFlagTargetInvert);
   GFREADBITBOOL(pStream, mFlagSaveIKDisabled);
   GFREADBITBOOL(pStream, mFlagLoop);
   GFREADBITBOOL(pStream, mFlagAllowMove);
   GFREADBITBOOL(pStream, mFlagPreserveDPS);
   GFREADBITBOOL(pStream, mFlagOverrideExitAction);
   GFREADBITBOOL(pStream, mFlagHasPlayedAnimation);
   if (BAction::mGameFileVersion >= 33)
      GFREADBITBOOL(pStream, mFlagDisableMotionExtractionCollisions);
   if (BAction::mGameFileVersion >= 38)
      GFREADBITBOOL(pStream, mFlagClearUnitNoRenderFlag);
   return true;
}
