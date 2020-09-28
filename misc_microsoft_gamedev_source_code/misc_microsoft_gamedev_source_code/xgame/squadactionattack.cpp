//==============================================================================
// squadactionattack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionmove.h"
#include "squadactionattack.h"
#include "squadactionairstrike.h"
#include "squad.h"
#include "unit.h"
#include "UnitOpportunity.h"
#include "world.h"
#include "tactic.h"
#include "protoobject.h"
#include "worldsoundmanager.h"
#include "config.h"
#include "configsgame.h"
#include "protosquad.h"
#include "unitactioncollisionattack.h"
#include "ability.h"
#include "SimOrderManager.h"
#include "squadlosvalidator.h"
#include "unitactionrangedattack.h"
#include "generaleventmanager.h"
#include "actionmanager.h"
#include "unitactionavoidcollisionair.h"
#include "squadactionmove.h"

//#define DEBUGOPPS

#define cFailedMoveAttemptLimit 10

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionAttack, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionAttack::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   #ifdef DEBUGOPPS
   if (pEntity->getPlayerID() == 1)
      pEntity->debug("SquadAttack::connect: ID=%d.", mID);
   #endif
   
//-- FIXING PREFIX BUG ID 3675
   const BSquad* pSquad=reinterpret_cast<BSquad*>(pEntity);
//--
   BASSERT(pSquad);

   // Garrisoning/Ungarrisoning squads cannot attack
   if (pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsGarrisoning())
   {
      return (false);
   }

   // BSR 5/8/08 - If this action specifically targets air units, reject any ground targets
   // This crappy test allows us to interpret illegal targets as aerial "attack ground" commands
   if (mpProtoAction && mpProtoAction->targetsAir() && mTarget.getID().isValid())
   {
      if (mTarget.getID().getType() == BEntity::cClassTypeSquad)
      {
//-- FIXING PREFIX BUG ID 3673
         const BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
//--
         if (pTargetSquad)
         {
            const BUnit* pTarget = pTargetSquad->getLeaderUnit();
            if (!pTarget || !pTarget->isType(gDatabase.getObjectType("Flying")))
            {
               mTarget.invalidateID();
            }
         }
      }
   }

   if (!BAction::connect(pEntity, pOrder))
      return (false);

   //Figure our range.  This will end up setting the range into mTarget.
   calculateRange(*pSquad, mTarget, mMinRange);

   // Handle ability command
   if (mTarget.isAbilityIDValid() && mTarget.getAbilityID() != -1)
   {
      int abilityID = gDatabase.getSquadAbilityID(pSquad, mTarget.getAbilityID());
//-- FIXING PREFIX BUG ID 3674
      const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--

      // Check the proto object's ability disabled flag
      bool abilityEnabled = true;
      BUnit *pLeaderUnit = pSquad->getLeaderUnit();
      if (pLeaderUnit && pLeaderUnit->getProtoObject() && pLeaderUnit->getProtoObject()->getFlagAbilityDisabled())
         abilityEnabled = false;

      if (pAbility && abilityEnabled && (!pSquad->getFlagRecovering() || (pSquad->getRecoverType() != pAbility->getRecoverType())))
      {
         mFlagAbilityCommand = true;
         mAbilityID = abilityID;
         int squadMode = pAbility->getSquadMode();
         if (squadMode == BSquadAI::cModeLockdown)
         {
            mFlagAutoLock = true;
            mFlagMoveAutoSquadMode = true;
         }
         else if (squadMode == BSquadAI::cModeHitAndRun)
         {
            mFlagHitAndRun = true;
         }
         else if (squadMode != -1)
         {
            mAutoMode = squadMode;
            mFlagAutoMode = true;
            mFlagAutoAttackMode = pAbility->getAttackSquadMode();
            if (squadMode != BSquadAI::cModeNormal)
               mFlagAutoExitMode = !pAbility->getKeepSquadMode();
         }
         if (pAbility->getDontInterruptAttack())
            mFlagDontInterruptAttack=true;
      }
   }
   // ajl 4/20/08 - Jira 5650 X/Y button change
   // TRB 7/30/08 - Changing this so it only unlocks if pressing the Y button.  X button does nothing.
   /*
   else if (pOrder && pOrder->getPriority() == BSimOrder::cPriorityUser)
   {
      if (pSquad->getSquadMode() == BSquadAI::cModeLockdown || pSquad->getSquadMode() == BSquadAI::cModeAbility)
      {
         // Auto go back to normal mode.
         mFlagAutoMode = true;
         mAutoMode = BSquadAI::cModeNormal;
      }
   }
   */

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionAttack::disconnect()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("SquadAttack::disconnect: ID=%d.", mID);
   #endif

   //Remove the child action.
   pSquad->removeActionByID(mChildActionID);
   //Stop our units.
   removeOpps(true);

   if (!mFlagAbilityCommand)
   {
      // Need to reset the last ability attack target ID if this is a non-ability ,attack that occurred after the ability attack.
      if (mTarget.isIDValid() && mTarget.getID() == pSquad->getLastAbilityAttackTargetID())
         pSquad->setLastAbilityAttackTargetID(cInvalidObjectID);
   }

   if (mpProtoAction && mpProtoAction->getTactic())
   {
      const BWeapon* pWeapon = mpProtoAction->getTactic()->getWeapon(mpProtoAction->getWeaponID());
      if (pWeapon && pWeapon->getSmartTargetType() != -1)
      {
         BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
         if (pTargetSquad)
            pTargetSquad->decrementSmartTargetReference(pWeapon->getSmartTargetType());
      }
   }

   if (mpChildOrder)
   {
      mpChildOrder->decrementRefCount();
      if (mpChildOrder->getRefCount() == 0)
         gSimOrderManager.markForDelete(mpChildOrder);
      mpChildOrder = NULL;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::init()
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mMinRange = 0.0f;
   mUnitOppIDs.clear();
   mUnitOppIDCount = 0;
   mHitZoneIndex = -1;
   mChildActionID = cInvalidActionID;
   mAutoMode = -1;
   mAbilityID = -1;
   mFutureState = cStateNone;
   mpParentAction = NULL;
   mpChildOrder = NULL;
   mUnpauseState=cStateNone;
   mNumChildrenStrafing = 0;
   mPreMovePositionX = -1.0f;
   mPreMovePositionZ = -1.0f;
   mNumMoveAttempts = 0;
   mFlagAnyFailed = false;
   mFlagAnySucceeded = false;
   mFlagAbilityCommand = false;
   mFlagAutoLock = false;
   mFlagAutoMode = false;
   mFlagAutoAttackMode = false;
   mFlagAutoExitMode = false;
   mFlagMoveAutoSquadMode = false;
   mFlagHitAndRun = false;
   mFlagDontInterruptAttack = false;
   mFlagAbilityExecuted = false;
   mLastRangeValidationTime = (DWORD)0;
#if DPS_TRACKER
   mRealDPS = -1.0f;
#endif
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::setState(BActionState state)
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   syncSquadData("BSquadActionAttack::setState -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::setState -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);
   syncSquadData("BSquadActionAttack::setState -- mState", mState);
   syncSquadData("BSquadActionAttack::setState -- new State", state);

   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("SquadAttack::setState: OldState=%s, NewState=%d.", getStateName(), state);
   #endif

   switch(state)
   {
      //Moving.
      case cStateMoving:
      {
         //BASSERT(!isStrafing());
         //Remove any opp we might have given our units.
         // TRB 8/27/08 - Don't remove the action along with the opp.  The attack action is allowed to complete the current
         // anim cycle while the move is starting up.
         // TRB 10/1/08 - Remove the opp and action for ability attacks.  The squad won't be monitoring the ability attack
         // which is required if the recovery timer is to be set correctly.
         // This means ability attacks will be aborted if the target moves and the attacker follows.  A solution to this would
         // be to change this action to support simultaneous move and attack opps, but this is a riskier change.
         removeOpps(mFlagAbilityCommand);
         
         //Have our squad move.  If it can't, fail.
         bool platoonMove = false;
         BEntityID parentID = pSquad->getParentID();
         if (mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
            platoonMove = true;

         if (pSquad->getPlayer()->isGaia())
            mChildActionID = pSquad->doMove(mpOrder, this, &mTarget, platoonMove, false, mFlagMoveAutoSquadMode, true);
         else
            mChildActionID = pSquad->doMove(mpOrder, this, &mTarget, platoonMove, false, mFlagMoveAutoSquadMode);

         // If the number of attempted moves is above the limit then fail because the move action isn't getting the squad any close to in range.
         // This is to prevent endless toggling between the move and attack states.
         if ((abs(mPreMovePositionX - pSquad->getPosition().x) < cFloatCompareEpsilon) && (abs(mPreMovePositionZ - pSquad->getPosition().z) < cFloatCompareEpsilon))
         {
            if (mNumMoveAttempts > cFailedMoveAttemptLimit)
               setState(cStateFailed);

            mNumMoveAttempts++;
         }
         else
            mNumMoveAttempts = 1;
         mPreMovePositionX = pSquad->getPosition().x;
         mPreMovePositionZ = pSquad->getPosition().z;

         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateFailed);
            return (true);
         }
         break;
      }

      case cStatePaused:
         //BASSERT(!isStrafing());
         //Save the state.
         mUnpauseState=mState;
         //Pull the opps from our units.
         removeOpps(true);
         break;

      case cStateUnpaused:
         //BASSERT(!isStrafing());
         //Even though we saved the unpause state, we can/should just be able to go
         //back to None and have that work.
         setState(cStateNone);
         //Reset the unpause state.
         mUnpauseState=cStateNone;
         return (true);

      //Attacking.  Give our units an Attack Opp.
      case cStateAttacking:
      {
         // If need to lockdown and configured to do so - lockdown
//-- FIXING PREFIX BUG ID 3677
         const BUnit* pLeader = pSquad->getLeaderUnit();
//--
         if (pLeader && !pLeader->isLockedDown() && (mFlagAutoLock || pLeader->canAutoLock()))
            return setState(cStateLockDown);

         if (mFlagAutoMode && pSquad->getSquadMode() != mAutoMode && mFlagAutoAttackMode)
            return setState(cStateLockDown);

         // TRB 6/26/08 - Check the proto object's ability disabled flag.  This done as late as possible in
         // case player finishes researching the tech just before attack started.
         bool abilityEnabled = true;
         BUnit *pLeaderUnit = pSquad->getLeaderUnit();
         if (pLeaderUnit && pLeaderUnit->getProtoObject() && pLeaderUnit->getProtoObject()->getFlagAbilityDisabled())
            abilityEnabled = false;

         // Cancel attack if recover type is ability which means the ability can't be used until done recovering.
         if (mFlagAbilityCommand && (!abilityEnabled || pSquad->getRecoverType()==cRecoverAbility))
            return setState(cStateDone);

         // If this is a hit-and-run attack, go to the ramming mode
         if (mFlagHitAndRun)
         {
            gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandBowl, pSquad->getPlayerID());
            return setState(cStateRamming);
         }

         if (pSquad->getFlagAttackBlocked())
            return (setState(cStateDone));

         BUnitOpp opp;
         opp.init();

         //Figure out the actual target.  We want to pass down a squad target if
         //we have one.  We explicitly don't copy the target we have in case we
         //have positions or ranges set (as those wouldn't mean the right thing
         //to units).
         BSimTarget actualTarget;
         //Pass down a squad target.
         if (mTarget.getID().isValid() && (mTarget.getID().getType() == BEntity::cClassTypeUnit))
         {
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
            if (pTargetUnit)
            {
//-- FIXING PREFIX BUG ID 3676
               const BSquad* pTargetSquad = pTargetUnit->getParentSquad();
//--
               if (!pTargetSquad || (pTargetSquad->getFlagCloaked() && !pTargetSquad->getFlagCloakDetected()))
               {
                  setState(cStateFailed);
                  return (true);
               }
               actualTarget.setID(pTargetSquad->getID());
            }
            else
            {
               setState(cStateFailed);
               return (true);
            }
         }
         else
            actualTarget.setID(mTarget.getID());

         // TRB 6/18/08 - Don't use ability if still recovering.  This is done here so
         // the squad and units are on the same page.  It was possible for the squad to be
         // recovering here but for the timer to expire before the units started the attack.
         // In that case the squad didn't expect the ability attack but the units did one, which was bad.
         if (mFlagAbilityCommand && abilityEnabled && (pSquad->getRecoverType() != cRecoverAbility))
            actualTarget.setAbilityID(mTarget.getAbilityID());

         // Some squads attack locations so pass through the position and range if it is valid
         if (mTarget.isPositionValid())
            actualTarget.setPosition(mTarget.getPosition());
         if (mTarget.isRangeValid())
            actualTarget.setRange(mTarget.getRange());
         opp.setTarget(actualTarget);
         opp.setType(BUnitOpp::cTypeAttack);
         opp.setSource(pSquad->getID());
         if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
            opp.setTrigger(true);
         else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
            opp.setPriority(BUnitOpp::cPriorityCommand);
         else
            opp.setPriority(BUnitOpp::cPrioritySquad);

         // TRB 6/18/08 - Enable the allow complete flag if this attack doesn't loop (most likely an ability attack).
         // The squad attack action needs to monitor these so it knows when it can go away.
         // VAT 10/22/08 - Also track abilities that drain shield, as those loop an anim, but need to send notification here
         // for retargetting when complete
         long ruleAbilityID = -1;
//-- FIXING PREFIX BUG ID 3679
         const BProtoAction* pProtoAction = pSquad->getProtoActionForTarget(actualTarget.getID(), actualTarget.getPosition(), actualTarget.getAbilityID(), false, NULL, false, &ruleAbilityID);
//--
         bool allowComplete = false;
         if (pProtoAction)
         {
            if (pProtoAction->getDontLoopAttackAnim())
               allowComplete = true;
            else if (pProtoAction->getTactic())
            {
               const BWeapon* pWeapon = pProtoAction->getTactic()->getWeapon(pProtoAction->getWeaponID());
               if (pWeapon && pWeapon->getFlagShieldDrain())
                  allowComplete = true;
            }
         }

         opp.setAllowComplete(allowComplete);

         // TRB 8/14/08 - Clear out the uninterruptible flag if not doing the ability.  Even if the ability is enabled, it still might not be possible to
         // use it based on the target.  Now that we can check whether the proto action is for an ability, clear the uninterruptible flag based on that or the allow complete
         // flag since that used to tell whether the squad action should wait on the unit action.
         if (!allowComplete || (ruleAbilityID == -1))
         {
            mFlagDontInterruptAttack = false;
         }

         //-- Determine which units will attack, and which units will play supporting animations (most likely an ability attack)
         bool specificChildren = false;
         BEntityIDArray childrenToFight;
         BEntityIDArray childrenToSupport;
         if(pProtoAction && pProtoAction->getMaxNumUnitsPerformAction() != -1 && (static_cast<int>(pSquad->getNumberChildren()) > pProtoAction->getMaxNumUnitsPerformAction()))
         {            
            specificChildren = true;
            for(uint i=0; i < pSquad->getNumberChildren(); i++)
               childrenToFight.add(pSquad->getChild(i));

            //-- remove units until we have the correct number remaining, and add them into the supporting units list
            while(childrenToFight.getNumber() > pProtoAction->getMaxNumUnitsPerformAction())
            {
               uint randIndex = getRand(cSimRand)%childrenToFight.getSize();;
               childrenToSupport.add(childrenToFight[randIndex]);
               childrenToFight.removeIndex(randIndex, false);
            }
         }

         //-- Add the attack opp
         bool result = false;
         if(!specificChildren)
            result = addOpps(opp);
         else
            result = addOpps(opp, true, &childrenToFight);
   
         if(!result)
         {
            setState(cStateMoving);
            return (true);
         }

         //-- Add the supporting opp
         if(pProtoAction && !childrenToSupport.isEmpty() && pProtoAction->getSupportAnimType() != -1)
         {
            opp.setType(BUnitOpp::cTypeAnimation);
            BASSERT(pProtoAction->getSupportAnimType() < UINT16_MAX);
            opp.setUserData(static_cast<uint16>(pProtoAction->getSupportAnimType()));
            opp.setAllowComplete(true);
            addOpps(opp, false, &childrenToSupport);
         }

         break;
      }

      // Have to lock down first - and allowed to do so autonomously
      case cStateLockDown:
      {
         setupChildOrder(mpOrder);
         if (!mpChildOrder)
         {
            setState(cStateFailed);
            return (true);
         }
         if (mFlagAutoMode)
            mpChildOrder->setMode((int8)mAutoMode);
         else
            mpChildOrder->setMode((int8)BSquadAI::cModeLockdown);
         mChildActionID = pSquad->doChangeMode(mpChildOrder, this);
         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateFailed);
            return (true);
         }
         break;
      }

      case cStateUnlock:
      {
         mFlagAutoExitMode=false;
         setupChildOrder(mpOrder);
         if (!mpChildOrder)
         {
            setState(cStateFailed);
            return (true);
         }
         mpChildOrder->setMode((int8)BSquadAI::cModeNormal);
         mChildActionID = pSquad->doChangeMode(mpChildOrder, this);
         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateFailed);
            return (true);
         }
         break;
      }

      // Ramming attack does a move straight into the target
      case cStateRamming:
      {
         // Invalidate target range so the move doesn't stop early (when "in range")
         //#ifndef _MOVE4
         if (mTarget.isIDValid())
         {
            // This code projects a point beyond the target.  However, it's not going
            // to work without modifications (hacks) in the squad action system to allow 
            // us to have an entity ID target, but *move* to a different specified 
            // location.  Just moving into the target, with the target set on
            // ignore, seems to be satisfactory for now. 
            /*
            BEntity *pEntity = gWorld->getEntity(mTarget.getID());
            if (!pEntity)
               break;            
            BVector vDir = pEntity->getPosition() - pSquad->getPosition();
            float fMag = vDir.length();
            fMag += pSquad->getObstructionRadius();
            vDir.y = 0;
            vDir.safeNormalize();
            BVector vProjPosition = pSquad->getPosition() + (vDir * fMag);
            BSimTarget tempTarget(vProjPosition, 0.0f);
            tempTarget.setID(mTarget.getID());
            */
            BSimTarget tempTarget(mTarget);
            tempTarget.invalidateRange();

            // Do move.  Don't make it a platoon move.  Really.  It just needs to be a squad move thorugh the target.
            // pass it the target id so it will ignore it and drive through it.  
            bool platoonMove = false;
            mChildActionID = pSquad->doMove(mpOrder, this, &tempTarget, platoonMove, false, mFlagMoveAutoSquadMode, true, mTarget.getID());
            if (mChildActionID == cInvalidActionID)
            {
               setState(cStateFailed);
               return (true);
            }
         }
         /*
         #else
         //Have our squad move.  If it can't, fail.
         bool platoonMove = false;
         BEntityID parentID = pSquad->getParentID();
         if (mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
            platoonMove = true;
         mChildActionID = pSquad->doMove(mpOrder, this, &mTarget, platoonMove, false, mFlagMoveAutoSquadMode, true);
         if (mChildActionID == cInvalidActionID)
         {
            setState(cStateFailed);
            return (true);
         }
         #endif
         */
         break;
      }
   
      //Done/Failed.
      case cStateDone:
      case cStateFailed:
      {
         if (mFlagAutoExitMode && pSquad->getSquadMode()==mAutoMode)
         {
            if (state == cStateFailed)
               mFlagAnyFailed = true;
            else
               mFlagAnyFailed = false;
            setState(cStateUnlock);
            return true;
         }

         //BASSERT(!isStrafing());
         //Remove the child action.
         pSquad->removeActionByID(mChildActionID);
         //Remove the opp we gave the units.
         removeOpps(true);

         // The target may still be alive even if all its children are dead.  This causes the action to exit because of LOS checks or Attack blocks and
         // results in zero XP for the attacker.  Here we're going ahead and exiting if all the children are dead to make sure the attacker
         // gets XP.

         BEntityID targetID = mTarget.getID();
         if (targetID.isValid())
         {
            BEntity* pEnt = gWorld->getEntity(targetID); 
            if (!checkTargetAlive(pEnt) || isTargetCrashingAirUnit(pEnt))
            {
               pSquad->applyBankXP();
            }
         }

         if (mpParentAction)
         {
            if (state == cStateDone)
               mpParentAction->notify(BEntity::cEventActionDone, mpOwner->getID(), getID(), 0);
            else
               mpParentAction->notify(BEntity::cEventActionFailed, mpOwner->getID(), getID(), 0);
         }
         break;
      }     
   }
   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::pause()
{
   return (setState(cStatePaused));
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::unpause()
{
   return (setState(cStateUnpaused));
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::update(float elapsed)
{ 
   SCOPEDSAMPLE(BSquadActionAttack_update);
   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("SquadAttack::update: State=%d.", mState);
   #endif

   syncSquadData("BSquadActionAttack::update -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::update -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);
   syncSquadData("BSquadActionAttack::update -- mState", mState);
   syncSquadData("BSquadActionAttack::update -- mFutureState", mFutureState);

   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   //No update if paused. 
   if (mState == cStatePaused)
      return (true);

   // No update if still emerging from command center
   if (pSquad->getFlagPlayingBlockingAnimation())
      return (true);

#if DPS_TRACKER
   //-- Figure out how to only do this if we have to
   updateTargetDPS();
#endif
 
   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   //Just waiting?
   if (mState == cStateWait)
      return (true);

   //If our target is gone, we're done.
   bool targetIsMobile=false;
   if (!validateTarget(targetIsMobile))
   {
      if (mFlagDontInterruptAttack && mState==cStateAttacking && mUnitOppIDs.getSize()>0)
      {
         //For non-interruptable attacks, wait for the units to finish their attack.
         setState(cStateWait);
         return (true);
      }
      setState(cStateDone);
      return (true);
   }

   // If garrisoning or ungarrisoning stop attacking
   if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning())
   {
      setState(cStateDone);
      return (true);
   }

   //Get range-ness.
   bool inRange = validateRange(mLastRangeValidationTime);

   switch (mState)
   {
      case cStateNone:
      {
         if (mFlagAutoMode && pSquad->getSquadMode() != mAutoMode && !mFlagAutoAttackMode)
            setState(cStateLockDown);
         else if (inRange)
            setState(cStateAttacking);
         else
            setState(cStateMoving);
         mFlagMoveAutoSquadMode = false;
         break;
      }

      case cStateMoving:
      {
         //If we're in range of a mobile target, early out.  Else, wait for movement to truly finish.
         if (targetIsMobile && inRange)
         {
            // Don't move to attacking state if the squad needs to lock down
            const BUnit* pLeader = pSquad->getLeaderUnit();
            if (pLeader && !pLeader->isLockedDown() && (mFlagAutoLock || pLeader->canAutoLock()))
               break;

            setState(cStateAttacking);
            break;
         }

         // If we're not in range, make sure we still have a move action, and it's our move action.
         BSquadActionMove *pMoveAction = pSquad->getNonPausedMoveAction_4();
         BAction *pParentAction = NULL;
         BActionID parentID = cInvalidActionID;
         if (pMoveAction)
         {
            pParentAction = pMoveAction->getParentAction();
            if (pParentAction)
               parentID = pParentAction->getID();
         }
         if (!pMoveAction || parentID != getID())
         {
           setState(cStateNone);
         }
         break;
      }

      case cStateAttacking:
      {
         //If we're not in range, go back to moving.
         if (!inRange)
         {
            // TRB 8/27/08:  Start up a move action.  The unit attack action will be allowed to complete its current anim cycle while moving.
            setState(cStateMoving);
            break;
         }
         else if (pSquad->getFlagAttackBlocked())
         {
            setState(cStateDone);
            break;
         }

         // If a squad is attacking keep marking the squad as detected, so the undetect timer keeps getting reset.
         pSquad->notify(BEntity::cEventDetected, mpOwner->getID(), NULL, NULL);

         //Update our last attack time.
         pSquad->updateLastAttackedTime();
         break;
      }
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionAttack::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);

   syncSquadData("BSquadActionAttack::notify -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::notify -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);
   syncSquadData("BSquadActionAttack::notify -- mState", mState);
   syncSquadData("BSquadActionAttack::notify -- mFutureState", mFutureState);
   syncSquadData("BSquadActionAttack::notify -- eventType", eventType);
   syncSquadData("BSquadActionAttack::notify -- senderID", senderID);

   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         if (data1 == mChildActionID)
         {
            if (mState == cStateLockDown || mState == cStateUnlock)
            {
               mFutureState=cStateFailed;
            }
            else
            {
               // If the move action has failed and the update has not already placed us in the attacking or failed state
               BActionState newFutureState = mFutureState;
               //If the move failed, then work if we're in range, else fail.
               if (validateRange(mLastRangeValidationTime))
                  newFutureState=cStateAttacking;
               else
               {
                  newFutureState=cStateFailed;

                  // Move failed so clear last attack data so squad doesn't keep trying to auto attack
                  // a target it can't reach
                  if (pSquad)
                     pSquad->clearLastAttackTargetData();
               }

               // Don't go into attacking or failed state if already in it
               if (mState != newFutureState)
                  mFutureState = newFutureState;
            }
         }
         break;

      case BEntity::cEventActionDone:
         if (mState == cStateLockDown)
         {
            if (mFlagAutoMode)
            {
               // Once the auto mode is entered, apply the movement modifier if specified in the ability
               if (mFlagAbilityCommand)
               {
//-- FIXING PREFIX BUG ID 3681
                  const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
//--
                  if (pAbility && pAbility->getMovementModifierType()==BAbility::cMovementModifierMode)
                     pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, pAbility->getMovementSpeedModifier(), false);
               }

               // Recompute the range after changing modes.
               calculateRange(*pSquad, mTarget, mMinRange);
               if (validateRange(mLastRangeValidationTime))
                  mFutureState=cStateAttacking;
               else
                  mFutureState=cStateMoving;
               break;
            }

            mFutureState=cStateAttacking;
         }
         else if (mState == cStateUnlock)
         {
            if (mFlagAnyFailed)
               mFutureState = cStateFailed;
            else
            {
               mFutureState = cStateDone;

               // If this was an ability attack and target is still alive, then continue attacking normally
               if (mFlagAbilityCommand)
                  restartAttackingTarget();
            }
         }
         else
         {
            //If the move succeeded, then go to work.

            // Ramming move done, so attack is done
            if ((data1 == mChildActionID) && (mState == cStateRamming))
               mFutureState = cStateDone;
            // If the move action has completed and the update has not already placed us in the attacking state
            else if ((data1 == mChildActionID) && (mState != cStateAttacking))
            {
               // TRB 9/2/08 - Following moving target change.
               // Go to attacking state immediately.  Waiting until the next frame could allow moving targets to move back out of range, preventing
               // the attack from starting up properly.
               setState(cStateAttacking);
            }
         }
         break;

      case BEntity::cEventOppComplete:
         //Data1:  OppID.
         //Data2:  Success.
         //Validate this is one ours and remove it at the same time.
         if (mUnitOppIDs.remove(data1))
         {
            //Track whether or not any failed.
            if (data2)
               mFlagAnySucceeded=true;
            else
               mFlagAnyFailed=true;

            //If we're out of opps, then we're either done or failed.
            if (mUnitOppIDs.getSize() == 0)
            {
               if (mFlagAnyFailed)
                  mFutureState=cStateFailed;
               else
                  mFutureState=cStateDone;

               if (mFlagAbilityCommand)
               {
                  const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
                  if (pAbility)
                  {
                     // Handle recovery mode.
                     if (pAbility->getRecoverStart() == cRecoverAttack && mFlagAnySucceeded)
                        pSquad->setRecover(pAbility->getRecoverType(), pSquad->getPlayer()->getAbilityRecoverTime(mAbilityID), pAbility->getID());

                     // Exit ability attack squad mode if needed.
                     if (mFlagAutoExitMode && pSquad->getSquadMode() != BSquadAI::cModeNormal)
                     {
                        mFutureState=cStateUnlock;
                        break;
                     }
                  }
               }

               // attempt to restart attacking the current target
               // if our protoaction is doesn't loop, restart attack
               if (mpProtoAction  && mpProtoAction->getDontLoopAttackAnim())
                  restartAttackingTarget();
            }
         }
         break;

      // A unit was added to the squad (probably through repair / reinforce).  Add any unit opps associated with this action to
      // the new unit.
      case BEntity::cEventSquadUnitAdded:
         {
            if (pSquad->containsChild(static_cast<BEntityID>(data1)))
            {
               BUnit* pUnit = gWorld->getUnit(data1);
               if (pUnit)
               {
                  if (mUnitOppIDs.getSize() > 0)
                  {
                     // Find a unit opp amongst the squad mate units that is in the
                     // list of unit opps associated with this squad action
//-- FIXING PREFIX BUG ID 3684
                     const BUnitOpp* pSquadmateOpp = NULL;
//--
                     for (uint i = 0; i < pSquad->getNumberChildren(); i++)
                     {
                        // Don't look at the opps for the unit just added
                        if (pSquad->getChild(i) == pUnit->getID())
                           continue;

//-- FIXING PREFIX BUG ID 3683
                        const BUnit* pSquadmate = gWorld->getUnit(pSquad->getChild(i));
//--
                        if (pSquadmate)
                        {
                           // Iterate through list of opps to find one in this actions
                           // opp id list
                           for (uint j = 0; j < pSquadmate->getNumberOpps(); j++)
                           {
                              const BUnitOpp* pOpp = pSquadmate->getOppByIndex(j);
                              if (mUnitOppIDs.contains(pOpp->getID()))
                              {
                                 pSquadmateOpp = const_cast<BUnitOpp*>(pOpp);
                                 break;
                              }
                           }
                        }
                        // If one was found, skip down to the section below
                        if (pSquadmateOpp)
                           break;
                     }

                     // If found the correct unit opp associated with this squad action,
                     // copy and add it to the new unit.
                     if (pSquadmateOpp &&
                         pSquadmateOpp->getTarget().getAbilityID() != -1 &&
                         pSquadmateOpp->getType() != BUnitOpp::cTypeAttack &&
                         pSquadmateOpp->getType() != BUnitOpp::cTypeSecondaryAttack)
                     {
                        // Copy new opp
                        BUnitOpp* pNewOpp = BUnitOpp::getInstance();
                        *pNewOpp = *pSquadmateOpp;
                        pNewOpp->generateID();

                        //Add it.
                        if (!pUnit->addOpp(pNewOpp))
                           BUnitOpp::releaseInstance(pNewOpp);
                        else
                           mUnitOppIDs.add(pNewOpp->getID());
                     }
                  }
               }
            }
         }
         break;
   }    

   syncSquadData("BSquadActionAttack::notify (end) -- mState", mState);
   syncSquadData("BSquadActionAttack::notify (end) -- mFutureState", mFutureState);

   //BASSERT(!isStrafing() || (mFutureState == cStateAttacking) || (mFutureState == cStateNone));
}

//==============================================================================
//==============================================================================
void BSquadActionAttack::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() ||
      (target.getID().getType() == BEntity::cClassTypeUnit) ||
      (target.getID().getType() == BEntity::cClassTypeSquad));
   mTarget=target;
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::validateTarget(bool& targetIsMobile)
{
//   if (isStrafing())
//      return (true);

//-- FIXING PREFIX BUG ID 3686
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   syncSquadData("BSquadActionAttack::validateTarget -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::validateTarget -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);

   BEntityID targetID = mTarget.getID();
   if (targetID.isValid())
   {
      BEntity* pEnt = gWorld->getEntity(targetID);      

      // See if the target is down
      bool down = false;
      if (pEnt)
      {
//-- FIXING PREFIX BUG ID 3685
         const BUnit* pUnit = pEnt->getUnit();         
//--
         if (pUnit)
         {
            down = pUnit->getFlagDown();
         }         
         BSquad* pTargetSquad = pEnt->getSquad();
         if (pTargetSquad)
         {
            down = pTargetSquad->isDown();
         }
      }

      if (!pEnt || !pEnt->isAlive() || down)
      {
         if (mState == cStateMoving)
            return (mTarget.isPositionValid());

         return false;
      }

      //If the target is not something we can attack, return false.
//-- FIXING PREFIX BUG ID 3689
      const BPlayer* pSquadPlayer = pSquad->getPlayer();
//--
//-- FIXING PREFIX BUG ID 3690
      const BPlayer* pEntityPlayer = pEnt->getPlayer();
//--
      // Check Gaia targets that can garrison enemies
      if (pSquadPlayer && !pSquadPlayer->isEnemy(pEntityPlayer) && (!pEnt->getSquad() || !pEnt->getSquad()->hasGarrisonedEnemies(pSquadPlayer->getID())) && !pEntityPlayer->isGaia())
         return (false);

      //If we don't have LOS to the target, return false.
      //FIXME BSR 9/19/07:     ...unless on AirStrike mission 
//-- FIXING PREFIX BUG ID 3691
      const BSquadActionAirStrike* pAirStrike = reinterpret_cast<const BSquadActionAirStrike*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadAirStrike));
//--
      if (!pEnt->isVisibleOrDoppled(pSquadPlayer->getTeamID()) && !pAirStrike && !pSquadPlayer->isGaia())
      {
         // If moving, allow the move to complete even if the target entity isn't visible or is killed.
         if (mState == cStateMoving)
            return (mTarget.isPositionValid());
         return (false);
      }

      //See if our target is mobile.
      targetIsMobile=pEnt->isEverMobile();


      // See if our target is attached to us
      if (pEnt->isAttached())
      {
         BUnit* pTargetUnit = pEnt->getUnit();
         if (!pTargetUnit)
         {
            BSquad* pTargetSquad = pEnt->getSquad();
            if (pTargetSquad )
               pTargetUnit = pTargetSquad->getLeaderUnit();
         }
//-- FIXING PREFIX BUG ID 3678
         const BUnit* pLeaderUnit = pSquad->getLeaderUnit();
//--
         if (pTargetUnit && pLeaderUnit)
         {
            if (pLeaderUnit->isObjectAttached(pTargetUnit))
               return false;
         }
      }

      //Else, return true.
      return (true);
   }
   return (mTarget.isPositionValid());
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::checkTargetAlive(BEntity* pTarget)
{
   if (!pTarget || !pTarget->isAlive())
      return false;

//-- FIXING PREFIX BUG ID 3694
   const BSquad* pTargetSquad = pTarget->getSquad();
//--

   // DMG NOTE: Is it possible for a squad to target anything other than another squad?
   if (!pTargetSquad)
      return false;

   for (uint i = 0; i < pTargetSquad->getNumberChildren(); i++)
   {
//-- FIXING PREFIX BUG ID 3693
      const BUnit* pUnit = gWorld->getUnit(pTargetSquad->getChild(i));
//--

      if (pUnit && pUnit->isAlive())
      {
         return true;
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::isTargetCrashingAirUnit(BEntity* pTarget)
{
   if (!pTarget)
      return false;

   const BSquad* pTargetSquad = pTarget->getSquad();
   if (!pTargetSquad)
      return false;

   if (!pTargetSquad->getLeaderUnit() || !pTargetSquad->getLeaderUnit()->isPhysicsAircraft())
      return false;

   BUnitActionAvoidCollisionAir* pUnitAvoidAction = NULL;
   for (uint i = 0; i < pTargetSquad->getNumberChildren(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(pTargetSquad->getChild(i));
      if (pUnit)
      {
         pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
         if(pUnitAvoidAction && pUnitAvoidAction->Crashing())
            return true;
      }
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BSquadActionAttack::calculateRange(const BSquad& squad, BSimTarget& target, float& minRange)
{
   // VAT: 11/12/08: Calculate a new range
   float previousRange = target.getRange();
   float previousMinRange = minRange;

   // if we didn't refresh the range, return
   if (!squad.calculateRange(&target, &minRange))
      return false;

   // VAT: 11/12/08: see if we need to force a range validation 
   // if our range has become more restrictive
   if (target.getRange() < previousRange || minRange > previousMinRange)
      mLastRangeValidationTime = 0;

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::validateRange(DWORD& lastValidationTime) const
{
   //if (isStrafing())
   //   return true;

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   syncSquadData("BSquadActionAttack::validateRange -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::validateRange -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);

   BEntity* pTarget = NULL;
   if (mTarget.getID().isValid())
      pTarget = gWorld->getEntity(mTarget.getID());

   if (!pTarget && !mTarget.isPositionValid())
      return false;

   //If we have a non-zero validation time, see if we're okay to skip this whole thing.
   if(pTarget)
   {
      BSquad* pTargetSquad = NULL;
      BUnit* pTargetUnit = NULL;
      if(pTarget->getUnit())
      {
         pTargetUnit = pTarget->getUnit();
         pTargetSquad = pTarget->getUnit()->getParentSquad();
      }
      else if(pTarget->getSquad())
      {
         pTargetSquad = pTarget->getSquad();
         pTargetUnit = pTargetSquad->getLeaderUnit();
      }
      if (pTargetUnit && pTargetSquad && pSquad->getLeaderUnit())
      {
         if ((lastValidationTime > (DWORD)0) &&
            (lastValidationTime >= pSquad->getLeaderUnit()->getLastMoveTime()) &&
            (lastValidationTime >= pSquad->getLastMoveTime()) &&
            (lastValidationTime >= pTargetUnit->getLastMoveTime()) &&
            (lastValidationTime >= pTargetSquad->getLastMoveTime()) )
            return (true);
      }
   }

   syncSquadData("BSquadActionAttack::validateRange passed early outs -- Action mID", (int)getID());

   // TRB 9/2/08 - Following moving target change.
   // If the target is moving, move further inside of range before ending the move so the attack action
   // has some extra frames to fire up.
   float rangeMultiplier = 1.0f;
   if ((mState == cStateMoving) && pTarget && pTarget->isMoving())
      rangeMultiplier = gDatabase.getMovingTargetRangeMultiplier();

   // BSR 9/23/08 - Hack for a square peg in a round hole.
   // If this squad uses a group range weapon, a simple range check won't work since the targeting space is not circular
   // Check to see that it is in a legally targetable location.
   BUnit* pLeader = pSquad->getLeaderUnit();
   BTactic* pTactic = pLeader ? pLeader->getTactic() : NULL;
   bool bUseGroupRange = false;

   if (pTactic && pTactic->getNumberWeapons() > 0)
   {
      for (int i=0; i<pTactic->getNumberWeapons(); i++)
      {
         if (pTactic->getWeapon(i)->getFlagUseGroupRange())
         {
            bUseGroupRange = true;
            break;
         }
      }
   }

   if (bUseGroupRange && (pLeader->getGroupDiagSpan() > 0.0f))
   {
      BVector tgtPos = pTarget->getPosition();
      BOPQuadHull* hull = pLeader->getGroupHull();
      if (hull && pTarget)
      {
         float fDist = hull->distance(tgtPos);

         fDist -= pTarget->getObstructionRadius();
         const BAction* pLeaderAttackAction = pLeader->getActionByType(BAction::cActionTypeUnitRangedAttack);
         if (pLeaderAttackAction)
         {
            const BProtoAction* pProto = pLeaderAttackAction->getProtoAction();
            if(pProto)
            {
               if (fDist > pProto->getMaxRange(pLeader, false))
                  return false;
            }
         }
      }
   }
   else if(mTarget.isRangeValid())
   {
      float distance = 0.0f;
      if (pTarget != NULL)
         distance = pSquad->calculateXZDistance(pTarget);
      else
         distance = pSquad->calculateXZDistance(mTarget.getPosition());

      if (distance > (mTarget.getRange() * rangeMultiplier))
         return(false);
   }

   // Min range
   if (mMinRange > cFloatCompareEpsilon)
   {
      // Min range is defined as edge of attacker to center point of target.  This matches the calculation in searchForWork.
      float targetMinRangeDist = 0.0f;
      if (pTarget != NULL)
         targetMinRangeDist = pSquad->calculateXZDistance(pTarget->getPosition());
      else
         targetMinRangeDist = pSquad->calculateXZDistance(mTarget.getPosition());

      if (targetMinRangeDist < (mMinRange - cFloatCompareEpsilon))
         return false;
   }

   if (gConfig.isDefined(cConfigTrueLOS))
   {
      if(pTarget != NULL)
      {
//-- FIXING PREFIX BUG ID 3695
         const BSquad* pTargetSquad = NULL;
//--
         if(pTarget->getUnit())
         {
            pTargetSquad = pTarget->getUnit()->getParentSquad();
         }
         else if(pTarget->getSquad())
         {
            pTargetSquad = pTarget->getSquad();
         }

         if(pTargetSquad)
         {
            // Since we are inrange, now also check for line of sight
            if(!pSquad->validateLOS(mpProtoAction, pTargetSquad))
               return(false);
         }
      }
   }

   //If we made it all the way here, timestamp our validation time to save us some
   //iterations in the future.
   lastValidationTime=gWorld->getGametime();

   syncSquadData("BSquadActionAttack::validateRange -- lastValidationTime", lastValidationTime);

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::addOpps(BUnitOpp opp, bool trackOpps/*=true*/, BEntityIDArray* children/*=NULL*/)
{
   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("SquadAttack::addOpp: ID=%d, Target=%d.", opp.getID(), opp.getTarget().getID());
   #endif

   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 3697
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);
   //BASSERT(mUnitOppIDs.getSize() == 0);

   syncSquadData("BSquadActionAttack::addOpps -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::addOpps -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);

   if(trackOpps)
   {
      if (pSquad->addOppToChildren(opp, &mUnitOppIDs, children))
         return (true);
   }
   else
   {
      if (pSquad->addOppToChildren(opp, NULL, children))
         return (true);
   }
   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionAttack::removeOpps(bool removeActions)
{
   if (mUnitOppIDs.getSize() == 0)
      return;

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 3698
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   syncSquadData("BSquadActionAttack::removeOpps -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::removeOpps -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);

   // TRB 11/21/08 - The opp is going to be removed but the unit attack action can hang around.
   // Set a flag on it to make sure it goes away in the case it isn't running the attack anim anymore.
   if (!removeActions)
   {
      for (uint unitIndex = 0; unitIndex < pSquad->getNumberChildren(); unitIndex++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(unitIndex));
         if (pUnit)
         {
            BUnitActionRangedAttack* pUnitAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
            if (pUnitAttackAction && !pUnitAttackAction->getFlagPersistent())
            {
               // See if the action's opp ID is in the list of opp IDs being removed
               BUnitOppID oppID = pUnitAttackAction->getOppID();
               if (mUnitOppIDs.contains(oppID))
               {
                  pUnitAttackAction->setFlagCompleteWhenDoneAttacking(true);
               }
            }
         }
      }
   }

   pSquad->removeOppFromChildren(mUnitOppIDs, removeActions);
   mUnitOppIDs.clear();
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionAttack::setupChildOrder(BSimOrder* pOrder)
{
   if (!mpChildOrder)
   {
      mpChildOrder = gSimOrderManager.createOrder();
      BASSERT(mpChildOrder);
      mpChildOrder->incrementRefCount();
      mpChildOrder->setOwnerID(pOrder->getOwnerID());
      mpChildOrder->setPriority(pOrder->getPriority());
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::isInterruptible() const
{
   if (mState == cStateLockDown || mState == cStateUnlock)
      return false;

   if (mState == cStateAttacking && mFlagDontInterruptAttack)
      return false;

   return true;
}

#if DPS_TRACKER
//==============================================================================
//==============================================================================
void BSquadActionAttack::updateTargetDPS()
{
   uint numChildren = 0;
   float totalActualDPS = 0.0f;

   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BUnit* pUnit = NULL;
   for(uint i = 0; i < pSquad->getNumberChildren(); i++)
   {
      pUnit = gWorld->getUnit(pSquad->getChild(i));
      if(!pUnit)
         continue;

      float actualDPS = pUnit->getTrackedDPS();
      if(actualDPS == -1.0f) //-- If the unit doesn't have an actual DPS yet, don't count it.
         continue;

      totalActualDPS += actualDPS;      
      numChildren++;
   }

   if(numChildren != 0)
      mRealDPS = totalActualDPS / numChildren;
}
#endif

//==============================================================================
//==============================================================================
bool BSquadActionAttack::haveActiveChildAttacks()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   bool haveActiveAttacks = false;
   for (uint j = 0; j < pSquad->getNumberChildren(); j++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
      if (pUnit)
      {
         // Get unit attack action and make sure it is currently attacking.
         // Make sure action matches a unit opp given by the squad action (this done just as an extra sanity check and probably isn't needed).
         BUnitActionRangedAttack* pUnitAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
         if (pUnitAttackAction && (pUnitAttackAction->getState() == cStateWorking) &&
             (mUnitOppIDs.find(pUnitAttackAction->getOppID()) != -1))
         {
            haveActiveAttacks = true;
            break;
         }
      }
   }
   return haveActiveAttacks;
}

//==============================================================================
//==============================================================================
void BSquadActionAttack::restartAttackingTarget()
{
   if (mpProtoAction->getDontAutoRestart())
      return;

   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;

   syncSquadData("BSquadActionAttack::removeOpps -- Action mID", (int)getID());
   syncSquadData("BSquadActionAttack::removeOpps -- mpOwner mID", (mpOwner) ? mpOwner->getID() : -1);

   // If this was an ability attack and target is still alive, then continue attacking normally
   if (pSquad->isAlive() && pSquad->getNumberChildren()>0)
   {
      bool targetIsMobile=false;
      if (validateTarget(targetIsMobile))
      {
         if (mFlagAbilityCommand)
            mFlagAbilityExecuted = true;

         mFlagAbilityCommand = false;
         mFlagDontInterruptAttack = false;

         mTarget.invalidateAbilityID();

         // Update range and attack.
         calculateRange(*pSquad, mTarget, mMinRange);
         if (validateRange(mLastRangeValidationTime))
            mFutureState=cStateAttacking;
         else
            mFutureState=cStateMoving;
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEARRAY(pStream, uint, mUnitOppIDs, uint8, 50);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, long, mHitZoneIndex);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, int, mAutoMode);
   GFWRITEVAR(pStream, int, mAbilityID);
   GFWRITEVAR(pStream, BActionState, mUnpauseState);
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, BActionState, mFutureState); 
   GFWRITEVAR(pStream, uint8, mNumChildrenStrafing);
   GFWRITEVAR(pStream, float, mMinRange);

   #if DPS_TRACKER
   GFWRITEVAR(pStream, float, mRealDPS);
   #else
   GFWRITEVAL(pStream, float, 0.0f)
   #endif

   GFWRITEBITBOOL(pStream, mFlagAnyFailed);
   GFWRITEBITBOOL(pStream, mFlagAbilityCommand);
   GFWRITEBITBOOL(pStream, mFlagAutoLock);
   GFWRITEBITBOOL(pStream, mFlagAutoMode);
   GFWRITEBITBOOL(pStream, mFlagAutoAttackMode);
   GFWRITEBITBOOL(pStream, mFlagAutoExitMode);
   GFWRITEBITBOOL(pStream, mFlagMoveAutoSquadMode);
   GFWRITEBITBOOL(pStream, mFlagHitAndRun);
   GFWRITEBITBOOL(pStream, mFlagDontInterruptAttack);
   GFWRITEBITBOOL(pStream, mFlagAbilityExecuted);
   GFWRITEBITBOOL(pStream, mFlagAnySucceeded);

   GFWRITEVAR(pStream, float, mPreMovePositionX);
   GFWRITEVAR(pStream, float, mPreMovePositionZ);
   GFWRITEVAR(pStream, uint8, mNumMoveAttempts);

   GFWRITEVAR(pStream, DWORD, mLastRangeValidationTime);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADARRAY(pStream, uint, mUnitOppIDs, uint8, 50);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, long, mHitZoneIndex);
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, int, mAutoMode);
   GFREADVAR(pStream, int, mAbilityID);
   GFREADVAR(pStream, BActionState, mUnpauseState);
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, BActionState, mFutureState); 
   GFREADVAR(pStream, uint8, mNumChildrenStrafing);
   if (BAction::mGameFileVersion >= 12)
      GFREADVAR(pStream, float, mMinRange);

   #if DPS_TRACKER
   GFREADVAR(pStream, float, mRealDPS);
   #else
   GFREADTEMPVAL(pStream, float)
   #endif

   GFREADBITBOOL(pStream, mFlagAnyFailed);
   GFREADBITBOOL(pStream, mFlagAbilityCommand);
   GFREADBITBOOL(pStream, mFlagAutoLock);
   GFREADBITBOOL(pStream, mFlagAutoMode);
   GFREADBITBOOL(pStream, mFlagAutoAttackMode);
   GFREADBITBOOL(pStream, mFlagAutoExitMode);
   GFREADBITBOOL(pStream, mFlagMoveAutoSquadMode);
   GFREADBITBOOL(pStream, mFlagHitAndRun);
   GFREADBITBOOL(pStream, mFlagDontInterruptAttack);

   if (BAction::mGameFileVersion >= 3)
      GFREADBITBOOL(pStream, mFlagAbilityExecuted);

   if (BAction::mGameFileVersion >= 27)
   {
      GFREADBITBOOL(pStream, mFlagAnySucceeded);
   }

   if (BAction::mGameFileVersion >= 19)
   {
      GFREADVAR(pStream, float, mPreMovePositionX);
      GFREADVAR(pStream, float, mPreMovePositionZ);
      GFREADVAR(pStream, uint8, mNumMoveAttempts);
   }

   if (BAction::mGameFileVersion >= 44)
   {
      GFREADVAR(pStream, DWORD, mLastRangeValidationTime);
   }


   return true;
}
