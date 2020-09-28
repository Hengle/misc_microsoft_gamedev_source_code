//==============================================================================
// unitactioncapture.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "UnitActionCapture.h"
#include "ProtoObject.h"
#include "SyncMacros.h"
#include "Tactic.h"
#include "UIGame.h"
#include "Unit.h"
#include "User.h"
#include "UserManager.h"
#include "World.h"                            
#include "game.h"

//#define DEBUGACTION


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionCapture, 5, &gSimHeap);



//==============================================================================
//==============================================================================
bool BUnitActionCapture::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);

   if (!BAction::connect(pOwner, pOrder))
      return (false);

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }

//-- FIXING PREFIX BUG ID 5024
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   BASSERT(pUnit->getParentSquad());
   BASSERT(!pUnit->getParentSquad()->isSquadAPhysicsVehicle());

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionCapture::disconnect()
{
   #ifdef DEBUGACTION
   mpOwner->debug("UnitActionCapture::disconnect: AID=%d.", mID);
   #endif
   
   //Release our controllers.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   releaseControllers();
   
   //Creepy, but necessary, call to event-drive the target's captured flag.
   BUnit *pTarget=gWorld->getUnit(mTarget.getID());
   if (pTarget)
   {
      pTarget->removeEntityRef(BEntityRef::cTypeCapturingUnit, pUnit->getID());
      if (!pTarget->getFirstEntityRefByType(BEntityRef::cTypeCapturingUnit))
         pTarget->setFlagBeingCaptured(false);
   }

   //Remove any move opp we may have given the unit.  DO NOT remove the action
   //since we're in the middle of stuff with this one.
   pUnit->removeOpp(mMoveOppID, false);
   mMoveOppID=BUnitOpp::cInvalidID;

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::init(void)
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle=true;
      
   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;
   mMoveOppID=BUnitOpp::cInvalidID;
   mFutureState=cStateNone;
   mFlagSecondChanceMove=false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::setState(BActionState state)
{
   #ifdef DEBUGACTION
   mpOwner->debug("UnitActionCapture::setState:: AID=%d, OldState=%s, newState=%d.", mID, getStateName(), state);
   #endif

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5018
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   BASSERT(!pSquad->isSquadAPhysicsVehicle());

   //Validate our target.
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!pTarget && (state != cStateFailed))
      return (setState(cStateFailed));

   switch (state)
   {
      //Moving.
      case cStateMoving:
      {
         BVector desiredLocation;
         if (!pSquad->getDesiredChildLocation(pUnit->getID(), desiredLocation))
         {
            setState(cStateFailed);
            return (false);
         }

         BUnitOpp* pOpp=BUnitOpp::getInstance();
         BASSERT(pOpp);
         pOpp->init();
         BSimTarget moveTarget(desiredLocation, 0.0f);
         pOpp->setTarget(moveTarget);
         pOpp->setType(BUnitOpp::cTypeMove);
         pOpp->setSource(pUnit->getID());
         pOpp->setPriority(BUnitOpp::cPriorityCritical);
         pOpp->generateID();

         //Add it.
         if (!pUnit->addOpp(pOpp))
         {
            BUnitOpp::releaseInstance(pOpp);
            setState(cStateFailed);
            return (false);
         }
         mMoveOppID=pOpp->getID();
         break;
      }

      case cStateWorking:
      {
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType());
         pUnit->computeAnimation();
         BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateWork, mpProtoAction->getAnimType()));
         pTarget->addEntityRef(BEntityRef::cTypeCapturingUnit, pUnit->getID(), (short)pUnit->getPlayerID(), 0);
         pTarget->setFlagBeingCaptured(true);
         if (pTarget->getCapturePlayerID() == cInvalidPlayerID)
            pTarget->setCapturePlayerID(pUnit->getPlayerID());
            
         //Reset our second chance move flag here.
         mFlagSecondChanceMove=false;
         break;
      }

      case cStateDone:
      case cStateFailed:
      case cStateNone:
      {
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         if (state == cStateDone)
            pUnit->completeOpp(mOppID, true);
         else if (state == cStateFailed)
            pUnit->completeOpp(mOppID, false);

         releaseControllers();
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         
         //Creepy, but necessary, call to event-drive the target's captured flag.
         if (pTarget)
         {
            pTarget->removeEntityRef(BEntityRef::cTypeCapturingUnit, pUnit->getID());
            if (!pTarget->getFirstEntityRefByType(BEntityRef::cTypeCapturingUnit))
               pTarget->setFlagBeingCaptured(false);
         }
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   
   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   //Validate our target.
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!pTarget)
   {
      setState(cStateFailed);
      return (true);
   }

   switch (mState)
   {
      case cStateNone:
      {
         //Check range.
         if (!validateRange())
         {
            setState(cStateMoving);
            break;
         }
         //At this point, we need to have the controllers to do anything.
         if (!grabControllers())
            break;

         // Can't go on if the animation is locked
         if (pUnit->isAnimationLocked())
            break;

         //We're good, figure out what we're doing.
         long ownerPlayerID=pUnit->getPlayerID();
         long targetPlayerID=pTarget->getPlayerID();
         long capturePlayerID=pTarget->getCapturePlayerID();
         #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionCapture::update ownerID", pUnit->getID().asLong());
         syncUnitActionData("BUnitActionCapture::update mTargetID", mTarget.getID().asLong());
         syncUnitActionData("BUnitActionCapture::update ownerPlayerID", ownerPlayerID);
         syncUnitActionData("BUnitActionCapture::update targetPlayerID", targetPlayerID);
         syncUnitActionData("BUnitActionCapture::update capturePlayerID", capturePlayerID);
         #endif

         //Make sure we can capture the target.
         if (!pTarget->isCapturable(pUnit->getPlayerID(), pUnit->getParentID()))
         {
            setState(cStateFailed);
            break;
         }

         setState(cStateWorking);
         break;
      }

      case cStateMoving:
      {
         //Check range.
         if (validateRange())
            setState(cStateNone);
         break;
      }

      case cStateWorking:
      {
         //Make sure we can still capture the target.
         if (!pTarget->isCapturable(pUnit->getPlayerID(), pUnit->getParentID()))
         {
            setState(cStateFailed);
            break;
         }
         //If we're moving, then just wait.
         if (mMoveOppID != BUnitOpp::cInvalidID)
            break;
      
         //If we've lost the controllers, go back to None.
         if (!validateControllers())
         {
            setState(cStateNone);
            break;
         }
         //Check range.  If we fail, we want to try to move again, but only once.
         if (!validateRange())
         {
            if (!mFlagSecondChanceMove)
            {
               mFlagSecondChanceMove=true;
               setState(cStateMoving);
            }
            else
               setState(cStateFailed);
            break;
         }
         //See if we're done.
         if (pTarget->getPlayerID() == pUnit->getPlayerID())
         {
            setState(cStateDone);
            if (mpProtoAction->getFlagDieOnBuilt())
            {
               mpOwner->kill(true);
               return (false);
            }
            break;
         }

         //Stop/settle the target (DCP: not sure why?)
         //pTarget->settle(); // SLB: This was messing me up so I commented it out
         
         //Figure out what our build points are going to be.
         float points=mpProtoAction->getWorkRate(pUnit->getID()) * elapsed;
         if (gWorld->getFlagQuickBuild())
            points*=30.0f;
         float curPoints=pTarget->getCapturePoints();
         float maxPoints=pTarget->getProtoObject()->getBuildPoints();

         //If nobody is capturing it then take it for our own.
         if (pTarget->getCapturePlayerID() == cInvalidPlayerID)
            pTarget->setCapturePlayerID(pUnit->getPlayerID());

         //If we're not the capturing player, we're undoing capture.
         bool done=false;
         if (pTarget->getCapturePlayerID() != pUnit->getPlayerID())
         {
            pTarget->adjustCapturePoints(-points, elapsed);
            //If that's caused the capture playerID to go back to invalid,
            //then take it for our own (which will start building it for us next time).
            if (pTarget->getCapturePlayerID() == cInvalidPlayerID)
               pTarget->setCapturePlayerID(pUnit->getPlayerID());
         }
         //Else if we are the capturing player, then add the points.
         else
         {
            if (curPoints+points >= maxPoints)
               done=true;
            pTarget->adjustCapturePoints(points, elapsed);
         }
         
         syncUnitActionData("BUnitActionCapture::update curPoints", curPoints);
         syncUnitActionData("BUnitActionCapture::update points", points);
         syncUnitActionData("BUnitActionCapture::update maxPoints", maxPoints);
         gWorld->notify(BEntity::cEventCapturePercent, pTarget->getID(), 0, 0);
         if (done)
         {
            #ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionCapture::update ownerID", pUnit->getID().asLong());
            #endif

            BPlayerID newPlayerID=pUnit->getPlayerID();
            if (gWorld->getFlagCoop())
            {
//-- FIXING PREFIX BUG ID 5019
               const BPlayer* pNewPlayer=pUnit->getPlayer();
//--
               if (pNewPlayer->getCoopID()!=cInvalidPlayerID)
                  newPlayerID=pNewPlayer->getCoopID();
            }
            BSquad* pTargetParentSquad = pTarget->getParentSquad();
            if (pTargetParentSquad)
            {
               pTargetParentSquad->changeOwner(newPlayerID);
               pTargetParentSquad->getSquadAI()->setMode(BSquadAI::cModeLockdown);

               // Restore hitpoints on captured unit(s)
               for (uint i=0; i<pTargetParentSquad->getNumberChildren(); i++)
               {
                  BUnit* pUnit=gWorld->getUnit(pTargetParentSquad->getChild(i));
                  if (pUnit)
                     pUnit->setHitpoints(pUnit->getProtoObject()->getHitpoints());
               }
            }


            //-- Play capture ui sound
            if (mpOwner->getPlayerID() == gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() || (gGame.isSplitScreen() && mpOwner->getPlayerID() == gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID()))
            {
               //-- Play the capture complete ui sound
               BCueIndex cue = pTarget->getProtoObject()->getSound(cObjectSoundCaptureComplete);
               gSoundManager.playCue(cue);
            }

            setState(cStateDone);

            gWorld->notify(BEntity::cEventCaptured, pTarget->getID(), mpOwner->getPlayerID(), 0);

            if (mpProtoAction->getFlagDieOnBuilt())
            {
               mpOwner->kill(true);
               return (false);
            }
            break;
         }
         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionCapture::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   #ifdef DEBUGACTION
   mpOwner->debug("UnitActionCapture::notify:: AID=%d, eventType=%d, senderID=%d, data1=%d, data2=%d.", mID, eventType, senderID, data1, data2);
   #endif
   //If we have a completed opp that's our move opp, deal with that.
   if ((eventType == BEntity::cEventOppComplete) &&
      (senderID == mpOwner->getID()) &&
      (data1 == mMoveOppID))
   {
      if (validateRange())
         mFutureState=cStateNone;
      else
         mFutureState=cStateFailed;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionCapture::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));
   mTarget=target;
}

//==============================================================================
//==============================================================================
void BUnitActionCapture::setOppID(BUnitOppID oppID)
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
uint BUnitActionCapture::getPriority() const
{
//-- FIXING PREFIX BUG ID 5021
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
bool BUnitActionCapture::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5022
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //We need them all.
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID)
      return (false);
   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Take them all.
   if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
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
void BUnitActionCapture::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::validateRange() const
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5023
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   BDEBUG_ASSERT(mpProtoAction);
   if (!pSquad)
      return false;

   //Determine our range.
   float range;
   if (mTarget.isRangeValid())
      range=mTarget.getRange();
   else
      range=mpProtoAction->getMaxRange(pUnit);
   //See if we're GTG.
   return (pSquad->isChildInRange(false, pUnit->getID(), mTarget, 0.0f, range, 1.0f));
}


//==============================================================================
//==============================================================================
bool BUnitActionCapture::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BUnitOppID, mMoveOppID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionCapture::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BUnitOppID, mMoveOppID);
   GFREADVAR(pStream, BActionState, mFutureState);
   return true;
}
