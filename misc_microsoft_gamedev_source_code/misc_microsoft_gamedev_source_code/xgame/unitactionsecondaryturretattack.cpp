//==============================================================================
// unitactionsecondaryturretattack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionsecondaryturretattack.h"
#include "world.h"
#include "tactic.h"
#include "unitquery.h"
#include "protoobject.h"
#include "visual.h"
#include "config.h"
#include "configsgame.h"
#include "squadlosvalidator.h"
#include "unitactionrevive.h"
#include "squadai.h"
#include "protosquad.h"
#include "unitactionavoidcollisionair.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionSecondaryTurretAttack, 5, &gSimHeap);

BDynamicSimArray<BUnitActionSecondaryTurretScanResult>   BUnitActionSecondaryTurretAttack::mTempScanResults;
BDynamicSimUIntArray                                     BUnitActionSecondaryTurretAttack::mTempScanOrder;


//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   // If crashing, don't connect
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
   {
      BAction::disconnect();
      return (false);
   }

   // Need a hardpoint
   if (validateHardpoint())      
   {
      // Take the hardpoint, and hold it until the primary attack borrows it from us.
      grabControllers();
   }

   mIsAttacking = false;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::disconnect(void)
{
//-- FIXING PREFIX BUG ID 3116
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (mIsAttacking)
      stopAttacking();

   // Notify the target that we are no longer attacking it.
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (pTargetUnit)
      pTargetUnit->removeAttackingUnit(pUnit->getID(), getID());

   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::init()
{
   if (!BAction::init())
      return false;

   mTarget.reset();
   mPreferredTarget.reset();
   mTargetingLead.zero();
   mIsAttacking = false;
   mOtherSecondaryToldToWait = false;
   mWaitingForLockdown = false;
   mOtherSecondaryID = -1;
   mFirstUpdate = true;
   mLastLOSValidationTime=(DWORD)0;
   
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::setState(BActionState state)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::setState owner ID", mpOwner->getID().asLong());
      syncUnitActionData("BUnitActionSecondaryTurretAttack::setState state", state);
   #endif

//-- FIXING PREFIX BUG ID 3117
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   switch (state)
   {
   case cStateBlocked:
   case cStateNone:
   case cStateWait: //-- Called by unitactionrangedattack, since he's stealing our hardpoint
   case cStatePaused: //-- Called by other secondary turret who is telling us we can't shoot until it's done
      {
         if (mIsAttacking)
            stopAttacking();

         pauseAltSecondary(false);

         mIsAttacking = false;

         // Notify the target that we are no longer attacking it.
         BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
         if (pTargetUnit)
            pTargetUnit->removeAttackingUnit(pUnit->getID(), getID());

         // Release controllers, if we're told to wait, otherwise we should own them
         if (state == cStateWait)
            releaseControllers();         
        
         if(state == cStateNone)
            grabControllers();

         break;
      }

   case cStateWorking:
      {
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::update(float elapsed)
{
   syncUnitActionData("BUnitActionSecondaryTurretAttack::update mState", mState);

   if (!getProtoAction())
      return (false);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //-- If the action isnt enabled, just bail. (This is for the hornet wingmen)
   if (getProtoAction() && getProtoAction()->getFlagDisabled())
   {
      // Go to none state if disabled so secondary turret scan token is released and another
      // secondary turret attack action can take it.
      // Do the same for the controllers.  Note the action tries to grab the controllers in none
      // state so need to check whether to release the controllers regardless of the current state.
      if (getState() != cStateNone)
         setState(cStateNone);
      long hardpointID = getProtoAction()->getHardpointID();
      if ((hardpointID != -1) && (pUnit->getHardpointController(hardpointID) == mID))
         releaseControllers();
      return (true);
   }

   // No update if still emerging from command center
   const BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (pSquad && pSquad->getFlagPlayingBlockingAnimation())
      return (true);

   if(mFirstUpdate)
   {
      //-- Check and see if we need to wait for lockdown
      if(mpProtoAction->getFlagRequiresLockdown())
      {
         if(pSquad && (pSquad->getSquadMode() != BSquadAI::cModeLockdown))
         {
            mWaitingForLockdown = true;
            setState(cStatePaused);            
         }
      }  
   }

   switch (mState)
   {
      case cStateNone:
      {
         //gConsole.output(cChannelSim, "  checking unit %d (squad %d, action %d, hardpoint %d) for scan token", pUnit->getID(), pSquad->getID(), getID(), mpProtoAction->getHardpointID());
         if(pUnit->getFlagSecondaryTurretScanToken(mpProtoAction->getHardpointID()))
         {
            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionSecondaryTurretAttack::update scanning target for hardpoint ", mpProtoAction->getHardpointID());
            #endif
            //gConsole.output(cChannelSim, "    valid token, scanning for target", pUnit->getID(), pSquad->getID());
            scanForTarget();
            pUnit->setFlagSecondaryTurretScanToken(mpProtoAction->getHardpointID(), false);
         }
         break;
      }      

      case cStateWorking:     
         //-- Track
         trackTarget(elapsed);

         //-- Are we still in the working state, after trying to track?
         if(mState == cStateWorking)
         {
            //-- Wait for attack timer if necessary
            if(pUnit->attackWaitTimerOn(mpProtoAction->getWeaponID()))
               break;

            //-- See if we can attack the primary attack's target
            // MS 11/10/2008: PHX-17246, shouldSwitchToPrimaryTarget (used to be checkPrimaryTarget) returns true if we want to switch targets. Checking it for false was the opposite of intended behavior.
            //if(checkPrimaryTarget() == false)
            if(shouldSwitchToPrimaryTarget())
            {
               setState(cStateNone);
               break;
            }         

            // Start attacking if we aren't currently attacking and we can hit the target
            if (canHitTarget() == false)
               setState(cStateNone);
            else
            {
               pauseAltSecondary(true);
               startAttacking();                 
            }
         }

         break;

      case cStateBlocked:
         if (!isBlocked())
            setState(cStateNone);
         break;

      case cStateWait:
      case cStatePaused:
         break;
   }

   mFirstUpdate = false;

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::isBlocked() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3119
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   return (pUnit->getFlagAttackBlocked() || pSquad->getFlagAttackBlocked() || pUnit->getFlagPassiveGarrisoned());
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

//-- FIXING PREFIX BUG ID 3123
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   switch (eventType)
   {
   case BEntity::cEventAnimAttackTag:
      {
         // Validate target
         if (!validateTarget(mTarget.getID()))
         {
            setState(cStateNone);
            break;
         }

         doProjectileAttack(data, data2);
         break;
      }

   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         if ((data2 == cActionAnimationTrack) && (mState == cStateWorking))
         {
            if (mpProtoAction->getDontLoopAttackAnim() && (senderID == pUnit->getID()))
            {
               setState(cStateNone);
               break;
            }

            if (eventType == BEntity::cEventAnimEnd)
            {
               setState(cStateNone);
               break;
            }
         }
         break;
      }

      case BEntity::cEventSquadModeChanaged:
      {
         //-- This action is only enabled during lockdown
         if(mpProtoAction->getFlagRequiresLockdown())
         {
            //-- Is the squad in lockdown?
            bool squadLockedDown = false;
            if(data == BSquadAI::cModeLockdown)
            {           
               squadLockedDown = true;
            }

            if(squadLockedDown && mWaitingForLockdown)
            {            
               mWaitingForLockdown = false;
               setState(cStateNone);
            }         
         }
         break;
      }
      case BEntity::cEventSquadModeChanging:         
      {    
         //-- This action is only enabled during lockdown, so if we're leaving lockdown, we need to pause
         if(mpProtoAction->getFlagRequiresLockdown())
         {         
            if(data != BSquadAI::cModeLockdown)
            {
               mWaitingForLockdown = true;
               setState(cStatePaused);
            }
         }         
         break;
      }
      case BEntity::cEventRecomputeVisualStarting:
      {
         // unlock the attachment, in case the recompute needs to replace it
         setAttachmentAnimationLock(false, true);
         break;
      }
      case BEntity::cEventRecomputeVisualCompleted:
      {
         // if we were attacking, we need to make sure we're in a valid state here for the visuals
         if (mpProtoAction && mIsAttacking)
         {
            // check to see if the attachments have the animations set
            bool attachmentAnimsSet = checkAttachmentAnimationSet(mpProtoAction->getAnimType());
            if (!attachmentAnimsSet)
            {
               // not set, restart the attack to make sure it's all valid
               mIsAttacking = false; 
               startAttacking();
            }
         }
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::setTarget(BSimTarget target)
{
   BDEBUG_ASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));

   // make sure we tell our old unit that we're done with it
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (pUnit && pTargetUnit)
      pTargetUnit->removeAttackingUnit(pUnit->getID(), getID());

   mTarget = target;
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::trackTarget(float elapsed)
{
   // If we're blocked, stop tracking
   if (isBlocked())
   {
      setState(cStateBlocked);
      return;
   }

   // Make sure squad isn't busy with a task where it isn't allowed to attack at the same time
   if (!doesParentOrderAllowAttacking())
   {
      setState(cStateNone);
      return;
   }

   // paranoia
   if (!validateHardpoint())
   {
      setState(cStateNone);
      return;
   }

   // Make sure we still have our controllers
   if (!validateControllers())
   {
      setState(cStateNone);
      return;
   }

   // Validate target
   if (!validateTarget(mTarget.getID()))
   {
      setState(cStateNone);
      return;
   }


   // Validate range
   if (!validateUnitAttackRange(mTarget, mLastLOSValidationTime))
   {
      setState(cStateNone);
      return;
   }

   // Update targeting lead
   updateTargetingLead();

   // Rotate turrets
   if (!updateTurretRotation(elapsed))
   {
      setState(cStateNone);
      return;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::scanForTarget()
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget owner ID", mpOwner->getID().asLong());
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget state", mState);
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget blocked", isBlocked());
   #endif

   // If we're blocked, stop scanning.
   if (isBlocked())
   {
      setState(cStateBlocked);
      return;
   }

   // Make sure squad isn't busy with a task where it isn't allowed to attack at the same time
   if (!doesParentOrderAllowAttacking())
      return;

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget validateHardpoint", validateHardpoint());
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget getHardpointID", mpProtoAction->getHardpointID());
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget mPreferredTarget", mPreferredTarget.getID());
   #endif

   // paranoia
   if (!validateHardpoint())
      return;

   // early out if we wouldn't be able to grab our hardpoint at the bottom in grabControllers()
   if (mpProtoAction->getHardpointID() != -1)
   {
      if (!pUnit->hardpointHasAction(mID, mpProtoAction->getHardpointID()) && !pUnit->canGrabHardpoint(mID, mpProtoAction->getHardpointID(), BUnitOpp::cInvalidID))
      {
         mPreferredTarget.reset();
         return;
      }
   }

   //-- Query for nearby units
   BVector ownerPos = pUnit->getPosition();

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget ownerPos", ownerPos);
   #endif

   // Include leashDistance and obsDiag in searchRange to create the same search criteria that the primary weapon gets through BOpportunity
   // This enables the secondary weapon to engage at the same time the primary does
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   //Get the enemy list.
   const BEntityIDArray& results=pSquad->getVisibleEnemySquads();
   if (results.getNumber() <= 0)
      return;

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget results.getNumber()", results.getNumber());
   #endif

   BVector turretPosition;
   BVector turretForward;
   BSimTarget bestTarget;

   getTurretPositionForward(turretPosition, turretForward);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget turretPosition", turretPosition);
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget turretForward", turretForward);
   #endif

   //-- If we don't have a specified preferred targetID, see if we can use our primary attacks targetID
   if(mPreferredTarget.getID() == cInvalidObjectID)
   {      
      BAction *pAction = pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack);
      if(pAction)
      {
         mPreferredTarget = *(pAction->getTarget());
      }
   }

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget mPreferredTarget", mPreferredTarget.getID());
   #endif

   // Reset sorted list
   mTempScanResults.setNumber(0);
   mTempScanOrder.setNumber(0);

   // precalc hardpoint info
   BVector hardpointPosition;
   BMatrix hardpointMatrix;
   bool getHardpointYawLocationSuccess = pUnit->getHardpointYawLocation(mpProtoAction->getHardpointID(), hardpointPosition, hardpointMatrix);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget getHardpointYawLocationSuccess", getHardpointYawLocationSuccess);
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget hardpointPosition", hardpointPosition);
   #endif

   // Iterate through results create a sorted list of valid results.  Sorted from best to worst
   //
   long numUnits = results.getNumber();
   BVector targetPosition;
   for (long i = 0; i < numUnits; i++)
   {
      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget result", i);
      #endif

      // MS 11/13/2008: PHX-17763 and others, invalidate our last LOS validation time since we're iterating over multiple
      // units and need to do a full check for all of them (since we don't cache this validation time per target).
      mLastLOSValidationTime = 0;

      float priority = 0.0f;
      BEntityID targetID = cInvalidObjectID;

      BSquad *pTargetSquad = gWorld->getSquad(results[i]);
      if (!pTargetSquad)
         continue;
      //If we have a preferred target that's in this squad, then use that target.  Else, just grab the leader unit.
      BUnit* pTarget=NULL;
      if (pTargetSquad->containsChild(mPreferredTarget.getID()))
         pTarget=gWorld->getUnit(mPreferredTarget.getID());
      else
         pTarget=pTargetSquad->getLeaderUnit();
      if (!pTarget)
         continue;
      targetID=pTarget->getID();

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget targetID", targetID);
      #endif

      // Make sure target is valid
      if (!validateTarget(targetID))
         continue;

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pTarget->getFlagDontAutoAttackMe()", pTarget->getFlagDontAutoAttackMe());
      #endif

      //Don't Auto Attack Me
      if (pTarget->getFlagDontAutoAttackMe())
         continue;

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pTargetSquad", pTargetSquad->getID());
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget getFlagCloaked", pTargetSquad->getFlagCloaked());
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget getFlagCloakDetected", pTargetSquad->getFlagCloakDetected());
      #endif

      // Dont attack cloaked units
      if (pTargetSquad && pTargetSquad->getFlagCloaked() && !pTargetSquad->getFlagCloakDetected())
         continue;

      BSimTarget tempSimTarget;
      tempSimTarget.setID(pTarget->getID());

      bool range = validateUnitAttackRange(tempSimTarget, mLastLOSValidationTime);

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget validateUnitAttackRange", range);
      #endif

      if (!range)
         continue;

      // Make sure it's a target we can damage and that it at least matches our best priority
      // DJB: This is nutty, but it's what Tim wants in the tactic file for secondary attacks to determnine what they can target
      // Make sure there is a target rule for this weapon.
      bool canAttack = pUnit->getTactic()->canAttackTarget(targetID, mpProtoAction);

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget canAttack", canAttack);
      #endif

      if(canAttack == false)
         continue;

      // Get  priority
      priority = mpProtoAction->getTargetPriority(pTarget->getProtoObject());

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget priority1", priority);
      #endif

      if(targetID == mPreferredTarget.getID()) // Give preferred target the highest priority
         priority = INT_MAX;

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget priority2", priority);
      #endif

      if (priority < 0.0f)
         continue;


      // Compute DotProduct
      targetPosition = pTarget->getPosition();

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget targetPosition1", targetPosition);
      #endif

      if(getHardpointYawLocationSuccess)
         targetPosition = applyGravityOffset(pTarget, targetPosition, hardpointPosition, hardpointMatrix);

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget targetPosition2", targetPosition);
      #endif

      BVector targetVector;
      targetVector.assignDifference(targetPosition, turretPosition);
      targetVector.normalize();

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget targetVector", targetVector);
      #endif

      float dot = turretForward.dot(targetVector);



      // Add to the result array
      BUnitActionSecondaryTurretScanResult curResult = BUnitActionSecondaryTurretScanResult(pTarget, targetPosition, priority, dot);
      mTempScanResults.add(curResult);
      uint curResultIndex = static_cast<uint>(mTempScanResults.getNumber()) - 1;

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget curResultIndex", (int)curResultIndex);
      #endif

      // Sort based on our sorting order
      //    1 - priority
      //    2 - dot product
      uint insertIndex = 0;
      uint numOrderList = static_cast<uint>(mTempScanOrder.getNumber());
      while ((insertIndex < numOrderList) &&
             ((curResult.getPriority() < mTempScanResults[mTempScanOrder[insertIndex]].getPriority()) ||
             ((curResult.getPriority() == mTempScanResults[mTempScanOrder[insertIndex]].getPriority()) && (curResult.getDotProduct() < mTempScanResults[mTempScanOrder[insertIndex]].getDotProduct()))))
      {
         insertIndex++;
      }
      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget insertIndex", (int)insertIndex);
      #endif
      mTempScanOrder.insertAtIndex(curResultIndex, insertIndex);
   }


   // Now that the results are sorted from best to last, iterate through them to find the first one that passes the 
   // expensive tests.  The expensive tests are the canOrientToTargetPosition and the LOS test which checks for
   // collisions.
   //
   uint numOrderList = static_cast<uint>(mTempScanOrder.getNumber());

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget numOrderList", (int)numOrderList);
   #endif

   for (uint i = 0; i < numOrderList; i++)
   {
      const BUnitActionSecondaryTurretScanResult *pCurResult = &mTempScanResults[mTempScanOrder[i]];

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pCurResult", (int)i);
         if (pCurResult->getTarget())
         {
            syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pCurResult2", pCurResult->getTarget()->getID());
         }
      #endif

      // Check we can orient to target
      if(!mpProtoAction->getDontCheckOrientTolerance() && !canOrientToTargetPosition(pCurResult->getTargetPosition()))
         continue;

      #ifdef SYNC_UnitAction
         if (pCurResult->getTarget())
         {
            syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pCurResult2", pCurResult->getTarget()->getID());
         }
      #endif

      // Check LOS to target
      if (gConfig.isDefined(cConfigTrueLOS))
      {
         if(!gSquadLOSValidator.validateLOS(pUnit->getParentSquad(), mpProtoAction, pCurResult->getTarget()->getParentSquad()))
            continue;
      }

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget pCurResult3", (int)i);
      #endif

      // Use this result.  We are done.
      bestTarget.set(pCurResult->getTarget()->getID(), pCurResult->getTargetPosition());
      break;
   }

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget bestTarget", bestTarget.isValid());
      syncUnitActionData("BUnitActionSecondaryTurretAttack::scanForTarget bestTarget", bestTarget.getID());
   #endif

   // Target found!
   if (bestTarget.isValid())
   {
      if(bestTarget == mPreferredTarget)
         pauseAltSecondary(true);

      if (grabControllers())
      {
         mIsAttacking = false;
         setTarget(bestTarget);
         setState(cStateWorking);
      }
   }

   mPreferredTarget.reset();
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::canOrientToTargetPosition(BVector targetPosition) const
{
//-- FIXING PREFIX BUG ID 3099
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   long hardpointID = mpProtoAction->getHardpointID();
   return (pUnit->canYawHardpointToWorldPos(hardpointID, targetPosition) && pUnit->canPitchHardpointToWorldPos(hardpointID, targetPosition));
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::validateHardpoint() const
{
//-- FIXING PREFIX BUG ID 3100
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (mpProtoAction->getHardpointID() < 0)
      return (false);

   return (pUnit->validateHardpoint(mpProtoAction->getHardpointID()));  
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //If we have to have a hardpoint, grab that.
   if (mpProtoAction->getHardpointID() != -1)
   {
      if (!pUnit->grabHardpoint(mID, mpProtoAction->getHardpointID(), BUnitOpp::cInvalidID))
         return (false);
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //If we have a hardpoint, release it.
   if (mpProtoAction && mpProtoAction->getHardpointID() != -1)
      pUnit->releaseHardpoint(mID, mpProtoAction->getHardpointID());
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::getTurretPositionForward(BVector &position, BVector &forward) const
{
//-- FIXING PREFIX BUG ID 3101
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   long hardpointID = mpProtoAction->getHardpointID();

   // Get hardpoint orientation
   BMatrix matrix;
   if (!pUnit->getHardpointYawTransform(hardpointID, matrix))
   {
      //position = pUnit->getVisualCenter();
      // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
      position = pUnit->getSimCenter();
      forward = pUnit->getForward();
      return;
   }

   forward = matrix.getRow(2);
   forward.normalize();
   matrix.getTranslation(position);

   // Transform to world space
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);
   forward = XMVector3TransformNormal(forward, worldMatrix);
   position = XMVector3Transform(position, worldMatrix);
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 3102
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   if (mpProtoAction->getHardpointID() >= 0)
   {
      long hardpointController = pUnit->getHardpointController(mpProtoAction->getHardpointID());
      if (hardpointController != (long)mID)
         return (false);
   }

   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::updateTargetingLead()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BPlayer* pPlayer = pUnit->getPlayer();
//-- FIXING PREFIX BUG ID 3103
   const BProtoObject* pProto = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : NULL;
//--
   float projectileSpeed = 0.0f;
   if (pProto)
      projectileSpeed = pProto->getDesiredVelocity();

   BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   if (!pTarget || !pTarget->isMoving())
   {
      mTargetingLead.zero();
      return;
   }
   BVector targetVelocity = pTarget->getVelocity();
   float targetSpeed = targetVelocity.length();
   if ((projectileSpeed < cFloatCompareEpsilon) || (targetSpeed < cFloatCompareEpsilon))
   {
      mTargetingLead.zero();
      return;
   }

   BVector attackerPos = pUnit->getPosition();
   BVector targetPos = pTarget->getPosition();
   float d = attackerPos.distance(targetPos);
   if (d < cFloatCompareEpsilon)
   {
      mTargetingLead.zero();
      return;
   }

   float t = d / projectileSpeed;
   targetVelocity *= Math::Min(targetSpeed, mpProtoAction->getMaxVelocityLead())  / targetSpeed;
   BVector newTargetPos = targetPos + targetVelocity * t;
   mTargetingLead = newTargetPos - targetPos;
   float d2 = attackerPos.distance(newTargetPos);
   mTargetingLead *= d2 / d;
}

//==============================================================================
//==============================================================================
/*bool BUnitActionSecondaryTurretAttack::validateRange() const
{
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   const BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   if (!pTarget)
      return false;

   float maxRange = mpProtoAction->getMaxRange(pUnit);
   float distance = pUnit->calculateXZDistance(pTarget);

   return (distance <= maxRange);
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::validateTargetRange(BUnit* target) const
{
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if (!target)
      return (false);

   float maxRange = mpProtoAction->getMaxRange(pUnit);
   float distance = pUnit->calculateXZDistance(target);

   return (distance <= maxRange);
}*/

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::validateTarget(BEntityID targetID) const
{
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if (!targetID.isValid())
      return false;

   BUnit* pTarget = gWorld->getUnit(targetID);
   if (!pTarget)
      return false;

//-- FIXING PREFIX BUG ID 3104
   const BPlayer* pPlayer = mpOwner ? mpOwner->getPlayer() : NULL;
//--
   bool isEnemy = pPlayer ? pPlayer->isEnemy(pTarget->getPlayer()) : false;

   return (pTarget->isAlive() && !pTarget->getFlagDown() && !pTarget->getFlagDestroy() && pTarget->getParentSquad() && (!pTarget->isGarrisoned() || pTarget->isInCover()) &&
      !pTarget->getFlagNotAttackable() && !pTarget->getFlagInvulnerable() && isEnemy && !pUnit->isObjectAttached(pTarget));
}

//==============================================================================
//==============================================================================
BVector BUnitActionSecondaryTurretAttack::applyGravityOffset(const BUnit *pTarget, BVector targetPosition, BVector hardpointPosition, const BMatrix& hardpointMatrix) const
{
   // SLB: Pitch up to match the ballistic arc
   if (mpProtoAction->isWeaponAffectedByGravity(mpOwner->getPlayer()))
   {
      const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);

      // Transform hardpoint matrix to world space
      BMatrix unitWorldMatrix;
      pUnit->getWorldMatrix(unitWorldMatrix);

      BMatrix wsHardpointMatrix;
      wsHardpointMatrix.mult(hardpointMatrix, unitWorldMatrix);

      // Get world space hardpoint position
      BVector wsHardpointPosition;
      wsHardpointMatrix.getTranslation(wsHardpointPosition);

      // Get projectile proto object
      const BPlayer *pPlayer = pUnit->getPlayer();
      const BProtoObject *pProtoObject = pPlayer->getProtoObject(mpProtoAction->getProjectileID());

      // Calculate projectile world space launch direction
      BVector launchDirection;
      bool useGravity;
      float gravity;
      const float maxRange = mpProtoAction->getMaxRange(pUnit);
      gWorld->calculateProjectileLaunchDirection(maxRange, pUnit, pTarget, pProtoObject, wsHardpointPosition, targetPosition, launchDirection, useGravity, gravity);

      // Project target location onto launchDirection vector
      if (useGravity)
      {
         launchDirection.normalize();
         targetPosition = wsHardpointPosition + (launchDirection * 100.0f);
      }
   }

   return targetPosition;
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::updateTurretRotation(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3106
   const BUnit* pTarget = gWorld->getUnit(mTarget.getID());
//--
   if (!pTarget)
      return false;

   // calc hardpoint info
   BVector hardpointPosition;
   BMatrix hardpointMatrix;
   bool getHardpointYawLocationSuccess = pUnit->getHardpointYawLocation(mpProtoAction->getHardpointID(), hardpointPosition, hardpointMatrix);

   BMatrix *pCachedHardpointMatrix = NULL;
   if (getHardpointYawLocationSuccess)
      pCachedHardpointMatrix = &hardpointMatrix;

   BVector targetPosition = pTarget->getPosition() + mTargetingLead;
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::updateTurretRotation targetPosition1", targetPosition);
   #endif

   if(getHardpointYawLocationSuccess)
      targetPosition = applyGravityOffset(pTarget, targetPosition, hardpointPosition, hardpointMatrix);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionSecondaryTurretAttack::updateTurretRotation targetPosition2", targetPosition);
   #endif

   long hardpointID = mpProtoAction->getHardpointID();
   const BHardpoint* pHP = pUnit->getHardpoint(hardpointID);
   if(!pHP)
      return false;

   //-- Just see if we could pitch to it if we wanted to, if we're using pitch and yaw as tolerance.
   if(pHP->getFlagUseYawAndPitchAsTolerance())
   {
      if(pUnit->canPitchHardpointToWorldPos(hardpointID, targetPosition) == false)
         return false;
      if(pUnit->canYawHardpointToWorldPos(hardpointID, targetPosition, pCachedHardpointMatrix) == false)
         return false;
      
      return true;
   }

   if (pUnit->getHardpoint(hardpointID)->getFlagCombined())
      return pUnit->orientHardpointToWorldPos(hardpointID, targetPosition, elapsed, NULL, pCachedHardpointMatrix);

   bool bYawResult = pUnit->yawHardpointToWorldPos(hardpointID, targetPosition, elapsed, NULL, pCachedHardpointMatrix);
   bool bPitchResult = pUnit->pitchHardpointToWorldPos(hardpointID, targetPosition, elapsed);

   return bYawResult && bPitchResult;
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::canHitTarget() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//-- FIXING PREFIX BUG ID 3108
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   if (pSquad && pSquad->getFlagChangingMode())
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::startAttacking()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   BVisual* pVisual = pUnit->getVisual();

   if (pHP && pVisual)
   {
      BVisualItem* pAttachment = pVisual->getAttachment(pHP->getYawAttachmentHandle());
      if (pAttachment)
      {        
         if(mIsAttacking == false)
         {
            pAttachment->setAnimationLock(cActionAnimationTrack, false);
            pAttachment->setAnimationLock(cMovementAnimationTrack, false);

            BMatrix worldMatrix; pUnit->getWorldMatrix(worldMatrix);
            DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);                  
            pVisual->setAnimation(cActionAnimationTrack, mpProtoAction->getAnimType(), false, playerColor, worldMatrix, 0.0f, -1, false, pAttachment);
            pVisual->copyAnimationTrack(cActionAnimationTrack, cMovementAnimationTrack, false, playerColor, worldMatrix, pAttachment);

            pAttachment->setAnimationLock(cActionAnimationTrack, true);
            pAttachment->setAnimationLock(cMovementAnimationTrack, true);

            pAttachment->validateAnimationTracks();
            pAttachment->updateVisibility(pUnit->getVisualIsVisible());
         }
         mIsAttacking = true;

         //Poke in our attack anim length as the attack wait timer for this weapon
         pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), 0.0f);         
         float animLen=pAttachment->getAnimationDuration(cActionAnimationTrack);
         if (animLen != 0.0f)
         {
            if (mpProtoAction->isAttackWaitTimerEnabled())
               pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), animLen);
         }   
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::stopAttacking()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());

   if (pHP)
   {
      pUnit->resetAttachmentAnim(pHP->getYawAttachmentHandle());
   }

   mIsAttacking = false;
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::doProjectileAttack(long attachmentHandle, long boneHandle)
{
   BDEBUG_ASSERT(boneHandle != -1);
   BDEBUG_ASSERT(attachmentHandle != -1);
//-- FIXING PREFIX BUG ID 3111
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());   
   if (!pTarget)
      return;

   //Get the range.
   float maxRange = mpProtoAction->getMaxRange(pUnit);

   //Projectile launch parms.
   BObjectCreateParms parms;
   parms.mPlayerID = pUnit->getPlayerID();
   parms.mProtoObjectID = mpProtoAction->getProjectileID();
   if (parms.mProtoObjectID == -1)
      return;

   // Notify the target that we have begun to attack it.
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (pTargetUnit)
      pTargetUnit->addAttackingUnit(pUnit->getID(), getID());

   syncProjectileData("BUnitActionSecondaryTurretAttack::doProjectileAttack Unit ID", pUnit->getID().asLong());
   if (pTargetUnit)
      syncProjectileData("BUnitActionSecondaryTurretAttack::doProjectileAttack Target ID", pTargetUnit->getID().asLong());

   syncProjectileData("BUnitActionSecondaryTurretAttack::doProjectileAttack launchPosition", parms.mPosition);

   //Calculate the point to launch the projectile from.
   getLaunchPosition(pUnit, attachmentHandle, boneHandle, &parms.mPosition, &parms.mForward, &parms.mRight);

   //Does this protoaction and this target mean we need to shoot at the foot of the target?   
   bool targetGround = false;
   bool collideWithUnits = true;
   if (mpProtoAction->getTargetsFootOfUnit() && pTarget->getProtoObject()->getFlagTargetsFootOfUnit())
   {
      targetGround = true;
      collideWithUnits = false;
   }

   //Figure out the target position and offset.
   BVector targetPos;
   BVector targetOffset;
   getTargetPosition(pTarget, targetPos, targetOffset, targetGround);

   //Calculate the damage amount.         
   float damage = getDamageAmount(pUnit, pTarget);

   //Projectile orientation.
   BVector turretPosition;
   BVector projectileOrientation;
   getTurretPositionForward(turretPosition, projectileOrientation);

   //Take unit accuracy scalar into account.
   bool useMovingAccuracy = (pUnit->isMoving() && (pUnit->getVelocity().length() >= (pUnit->getDesiredVelocity() * 0.9f))) ? true : false;
   float accuracy = mpProtoAction->getAccuracy() * pUnit->getAccuracyScalar();
   float movingAccuracy = mpProtoAction->getMovingAccuracy() * pUnit->getAccuracyScalar();
   float maxDeviation = mpProtoAction->getMaxDeviation();
   float movingMaxDeviation = mpProtoAction->getMovingMaxDeviation();
   if (pUnit->getAccuracyScalar())
   {
      float recAccuracyScalar = 1.0f / pUnit->getAccuracyScalar();
      maxDeviation *= recAccuracyScalar;
      movingMaxDeviation *= recAccuracyScalar;
   }

   //Actually launch the projectile.
   targetOffset += gWorld->getProjectileDeviation(parms.mPosition, pTarget ? ( pTarget->getPosition() + targetOffset ) : targetOffset,
      mTargetingLead, useMovingAccuracy ? movingAccuracy : accuracy, maxRange,
      useMovingAccuracy ? movingMaxDeviation : maxDeviation,
      mpProtoAction->getAccuracyDistanceFactor(), mpProtoAction->getAccuracyDeviationFactor());
   gWorld->launchProjectile(parms, targetOffset, mTargetingLead, projectileOrientation, damage,
      mpProtoAction, (IDamageInfo*)mpProtoAction, mpOwner->getID(), pTarget, -1, false, collideWithUnits );
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector* position, BVector* forward, BVector* right) const
{
   BVisual* pVisual = pUnit->getVisual();
   if (pVisual)
   {
      BMatrix attachmentMat;
//-- FIXING PREFIX BUG ID 3112
      const BVisualItem* pVisualItem = pVisual->getAttachment(attachmentHandle, &attachmentMat);
//--
      if (pVisualItem)
      {
         BMatrix unitMat;
         pUnit->getWorldMatrix(unitMat);

         BMatrix mat;
         mat.mult(attachmentMat, unitMat);

         if (pVisualItem->getBone(boneHandle, position, NULL, NULL, &mat))
         {
            if (forward)
               mat.getForward(*forward);
            if (right)
               mat.getRight(*right);
            return;
         }
      }
   }

   if (position)
      //*position = pUnit->getVisualCenter();
      // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
      *position = pUnit->getSimCenter();
   if (forward)
      *forward = pUnit->getForward();
   if (right)
      *right = pUnit->getRight();
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::getTargetPosition(const BUnit* pTarget, BVector& targetPosition, BVector& targetOffset, bool targetGround) const
{
   if (targetGround)
   {
      BVector tempPos = pTarget->getPosition();
      float retHeight;
      gTerrainSimRep.getHeightRaycast(tempPos, retHeight, true);
      targetPosition = BVector(tempPos.x, retHeight, tempPos.z);
      targetOffset = cOriginVector;

      return;
   }

   //Target Position.
   targetPosition = pTarget->getPosition();
   // MS 10/17/2008: this was, uhm... yeah
   //targetOffset = BVector(0.0f, pTarget->getProtoObject()->getObstructionRadiusY(), 0.0f);
   targetOffset = BVector(0.0f, 0.0f, 0.0f);
}

//==============================================================================
//==============================================================================
float BUnitActionSecondaryTurretAttack::getDamageAmount(const BUnit* pUnit, const BUnit* pTarget) const
{
   //Get the damage modifier.
   float damageModifier = pUnit->getDamageModifier();
   if (damageModifier <= 0.0f)
      return 0.0f;

   //Get the base damage amount.
   float damageAmount = mpProtoAction->getDamagePerAttack();

   //Add in our modifier.
   damageAmount *= damageModifier;

   //Add Height Bonus Damage.
   if (mpProtoAction->usesHeightBonusDamage())
   {
      float heightDiff = pUnit->getPosition().y - pTarget->getPosition().y;
      if (heightDiff > 0)
         damageAmount += damageAmount * heightDiff * gDatabase.getHeightBonusDamage();
   }

   return damageAmount;
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if (senderID != pUnit->getID())
      return false;

   if(!mIsAttacking)
   {
      // VAT: also allow the recompute visual events through - why the heck do we do this? 
      if(eventType != BEntity::cEventSquadModeChanaged && eventType != BEntity::cEventSquadModeChanging && 
         eventType != BEntity::cEventRecomputeVisualStarting && eventType != BEntity::cEventRecomputeVisualCompleted)
         return false;
   }
      

   switch (eventType)
   {
   case BEntity::cEventAnimAttackTag:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
         if (pHP && (pHP->getYawAttachmentHandle() != -1))
         {
            if (pHP->getYawAttachmentHandle() == (long)data)
               return true;
            if ((pHP->getPitchAttachmentHandle() != -1) && (pHP->getPitchAttachmentHandle() == (long)data))
               return true;

            return false;
         }

         return (data == -1);
      }

   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());

         if (pHP && (pHP->getYawAttachmentHandle() != -1))
         {
//-- FIXING PREFIX BUG ID 3113
            const BVisualItem* pVisualItem = pUnit->getVisual()->getAttachment(pHP->getYawAttachmentHandle());
//--
            return (pVisualItem && (pVisualItem->getAnimationType(data2) == (long)data));
         }

         return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionSecondaryTurretAttack::pauseAltSecondary(bool pause)
{
  BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
  BDEBUG_ASSERT(pUnit);

  if(mOtherSecondaryToldToWait == pause)
     return;

   if(mpProtoAction->getAlternateHardpointID() != -1)
   {  
      //-- Find the secondary turret attack action which uses the alternate hardpoint specified.
      if(mOtherSecondaryID == -1)
      {         
         uint numActions = pUnit->getNumberActions();
         for(uint i=0; i < numActions; i++)
         {
            const BAction* pAction = pUnit->getActionByIndexConst(i);
            if(!pAction)
               continue;

            if(pAction->getType() == BAction::cActionTypeUnitSecondaryTurretAttack)
            {            
               if(pAction->getProtoAction()->getHardpointID() == mpProtoAction->getAlternateHardpointID())
               {
                  mOtherSecondaryID = pAction->getID();
                  break;
               }               
            } 
         }
      }

      BAction* pOtherSecondaryAttack = pUnit->findActionByID(mOtherSecondaryID);
      if(pOtherSecondaryAttack)
      {
         if(pause)
            pOtherSecondaryAttack->setState(cStatePaused);
         else
            pOtherSecondaryAttack->setState(cStateNone);

         mOtherSecondaryToldToWait = pause;
      }
   }
}

//==============================================================================
// MS 11/10/2008: this function should be interpreted as "SHOULD I SWITCH TO THE PRIMARY TARGET?".
//==============================================================================
bool BUnitActionSecondaryTurretAttack::shouldSwitchToPrimaryTarget()
{
   //-- If we're not currently attacking the primary target, but we COULD be, then we need to switch.
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if(mTarget.isValid() == false)
      return false;

   BAction *pAction = pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack);
   if(pAction)
   {
      if(mTarget.getID() != (pAction->getTarget()->getID()))
      {
         if(pUnit->getTactic()->canAttackTarget(pAction->getTarget()->getID(), mpProtoAction) == true)
         {
            //-- Can we orient to the primary?
//-- FIXING PREFIX BUG ID 3114
            const BUnit* pTarget = gWorld->getUnit(pAction->getTarget()->getID());
//--
            if(pTarget && canOrientToTargetPosition(pTarget->getPosition()))
            {
               // Switch to Primary
               BSimTarget tempSimTarget;
               tempSimTarget.setID(pTarget->getID());
               setTarget(tempSimTarget);
               return true;
            }
         }
      }
      // MS 11/10/2008: this should fall through and return false at the bottom of the function
   }
   // MS 11/10/2008: this should fall through and return false at the bottom of the function

   return false;
}


//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::doesParentOrderAllowAttacking()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   const BSquad* pSquad = pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);

   // This is a similar check to the one BSquadAI does when it makes
   // sure it isn't gathering when searching for attack opportunities
   const BSimOrderEntry* pOrderEntry = pSquad->getOrderEntry(true, true);
   if (pOrderEntry)
   {
      // The commented out check is the one BSquadAI does.  I'm just going to check for gather orders to lessen the impact of this change.
      //if ((pOrderEntry->getType() != BSimOrder::cTypeAttack) && (pOrderEntry->getType() != BSimOrder::cTypeMove) && (pOrderEntry->getType() != BSimOrder::cTypeJoin) && !pSquad->isMoving())
      if (pOrderEntry->getType() == BSimOrder::cTypeGather)
         return false;
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITECLASS(pStream, saveType, mPreferredTarget);
   GFWRITEVECTOR(pStream, mTargetingLead);
   GFWRITEVAR(pStream, long, mOtherSecondaryID);
   GFWRITEBITBOOL(pStream, mFirstUpdate);
   GFWRITEBITBOOL(pStream, mIsAttacking);
   GFWRITEBITBOOL(pStream, mOtherSecondaryToldToWait);
   GFWRITEBITBOOL(pStream, mWaitingForLockdown);
   GFWRITEVAR(pStream, DWORD, mLastLOSValidationTime);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionSecondaryTurretAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADCLASS(pStream, saveType, mPreferredTarget);
   GFREADVECTOR(pStream, mTargetingLead);
   GFREADVAR(pStream, long, mOtherSecondaryID);

   // deprecated
   if(BAction::mGameFileVersion < 40)
   {
      long discard;
      GFREADVAR(pStream, long, discard);
   }

   GFREADBITBOOL(pStream, mFirstUpdate);
   GFREADBITBOOL(pStream, mIsAttacking);
   GFREADBITBOOL(pStream, mOtherSecondaryToldToWait);
   GFREADBITBOOL(pStream, mWaitingForLockdown);
   if (BAction::mGameFileVersion >= 42)
      GFREADVAR(pStream, DWORD, mLastLOSValidationTime);
   return true;
}
