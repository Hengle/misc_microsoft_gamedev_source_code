//==============================================================================
// unitactionrangedattack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "actionmanager.h"
#include "unitactionunderattack.h"
#include "unitactionrangedattack.h"
#include "unitactionmove.h"
#include "unitactionmoveair.h"
#include "DebugPrimitives.h"
#include "Formation2.h"
#include "unit.h"
#include "world.h"
#include "objectmanager.h"
#include "database.h"
#include "tactic.h"
#include "protoobject.h"
#include "visual.h"
#include "damagehelper.h"
#include "syncmacros.h"
#include "UnitOpportunity.h"
#include "battle.h"
#include "configsgame.h"
#include "simhelper.h"
#include "squadactioncarpetbomb.h"
#include "squadactionattack.h"
#include "unitactionsecondaryturretattack.h"
#include "unitactionavoidcollisionair.h"
#include "unitactionstasis.h"
#include "Physics.h"
#include "physicsInfoManager.h"
#include "physicsInfo.h"
#include "unitquery.h"
#include "worldSoundManager.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "squadlosvalidator.h"
#include "grannyinstance.h"
#include "unitactionchargedrangedattack.h"

//#define DEBUGOPPS

const float cTopYVelOfKnockbackUnit = 6.0f;
const float cMedYVelOfKnockbackUnit = 3.0f;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionRangedAttack, 5, &gSimHeap);

BDynamicSimArray<BUnitActionRangeAttackScanResult> BUnitActionRangedAttack::mTempScanResults;
BDynamicSimUIntArray                               BUnitActionRangedAttack::mTempScanOrder;


//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   syncUnitActionData("BUnitActionRangedAttack::connect owner ID", pOwner->getID().asLong());

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   // If shield draining attack and we have no shields, don't connect
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

//-- FIXING PREFIX BUG ID 3312
   const BTactic* pTactic = pUnit->getTactic();
//--
   BDEBUG_ASSERT(pTactic);
   const BWeapon *pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());

   if (pUnit->getFlagHasShield() && pWeapon && pWeapon->getFlagShieldDrain())
   {
      BSquad* pSquad = pUnit->getParentSquad();

      if (pUnit->getShieldpoints() <= 0.0f)
      {
         if (pSquad && pSquad->getActionByType(BAction::cActionTypeSquadAttack))
            pSquad->getActionByType(BAction::cActionTypeSquadAttack)->setTarget(::cInvalidObjectID);
         BAction::disconnect();
         return (false);
      }

      // If our shields are recharging, stop them
      if (pSquad)
      {
         pSquad->setFlagStopShieldRegen(true);
      }
   }

   // If crashing, don't connect
   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
   {
      BAction::disconnect();
      return (false);
   }

   syncUnitActionCode("BUnitActionRangedAttack::connect");

   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      pOwner->debug("RangedAttack: ID=%d, Connect.", mID);
   #endif

   // If a secondary turret is using our hardpoint, tell it to give it up
   BUnitActionSecondaryTurretAttack* pSecondaryTurretAttack = pUnit->getSecondaryTurretAttack(mpProtoAction->getHardpointID());
   if (pSecondaryTurretAttack)
   {
      pSecondaryTurretAttack->setState(cStateWait);
      mSecondaryTurretAttackID = pSecondaryTurretAttack->getID();
   }

   //DJB - Remove this hack once I get rid of secondary turret attack, or if we get rid of the notion of 2 guns that can't shoot at the same time
   pSecondaryTurretAttack = pUnit->getSecondaryTurretAttack(mpProtoAction->getAlternateHardpointID());
   if (pSecondaryTurretAttack)
   {
      pSecondaryTurretAttack->setState(cStateWait);
      mAltSecondaryTurretAttackID = pSecondaryTurretAttack->getID();
   }

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }

   syncUnitActionCode("BUnitActionRangedAttack::connect");

   //Get the launch point.
   if (pUnit->getVisual())
      mLaunchPointHandle=pUnit->getVisual()->getNextPointHandle(cActionAnimationTrack, -1, cVisualPointLaunch);

   //DCPTODO: Determine what type of attack we have.
   if (mpProtoAction->getProjectileID() >= 0)
   {
      mFlagMelee=false;
      mFlagBeam=mpProtoAction->isBeam();
      mFlagProjectile=!mFlagBeam;
      mFlagKnockback=false;
   }
   else
   {
      mFlagMelee=true;
      mFlagBeam=false;
      mFlagProjectile=false;
      mFlagKnockback=mpProtoAction->getFlagKnockback();
   }

   //-- Reset the preattack cooldown when attacks connect
   long numWeapons = 0;
   if(pUnit->getTactic())
      numWeapons = pUnit->getTactic()->getNumberWeapons();
   for(long i = 0; i < numWeapons; i++)
      pUnit->setPreAttackWaitTimerSet(i, false);

   // Add slave attack action
   if (mpProtoAction->getSlaveAttackActionID() != -1)
   {
      BTactic* pTactic = pUnit->getTactic();
      BASSERT(pTactic); // Need a tactic

      //Create the action.
      BAction* pSlaveAttackAction = gActionManager.createAction(BAction::cActionTypeUnitSlaveTurretAttack);
      pSlaveAttackAction->setProtoAction(pTactic->getProtoAction(mpProtoAction->getSlaveAttackActionID()));
      pSlaveAttackAction->setParentAction(this);
      bool success = pUnit->addAction(pSlaveAttackAction);
      BASSERT(success); // Shouldn't fail
   }

   mTargetDPS = mpProtoAction->getDamagePerSecond();
   mAddedBaseDPS = mpProtoAction->getAddedBaseDPS();

   mMissedTentacleAttack = false;

   // If we're carrying over ramped damage from another attack action, initialize us
   if (mpProtoAction->keepDPSRamp())
   {
      float rampTime = fabs(pWeapon->getDPSRampTime());
      float dpsRamp = pUnit->getDPSRampValue();

      mBeamDPSRampTimer = (dpsRamp / mpProtoAction->getDamagePerSecond()) * 100.0f;
      mBeamDPSRampTimer = min(mBeamDPSRampTimer, pWeapon->getFinalDPSPercent());
      mBeamDPSRampTimer = mBeamDPSRampTimer * 0.01f * rampTime;
   }

   // Setup beams
   mTargetBeamProtoProjectile = pUnit->getProtoObject()->getTargetBeam();
   mKillBeamProtoProjectile = pUnit->getProtoObject()->getKillBeam();

   // if we have a low detail anim, increment the global ref count
   if(mpProtoAction->getLowDetailAnimType() != -1)
      gWorld->incrementLowDetailAnimRefCount();

   syncUnitActionCode("BUnitActionRangedAttack::connect");

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::disconnect(void)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   syncUnitActionData("BUnitActionRangedAttack::disconnect owner ID", mpOwner->getID().asLong());

   //Release our controllers.
   releaseControllers();

   // If we were stopping the shields from recharging, stop
   const BTactic* pTactic = pUnit->getTactic();
   BDEBUG_ASSERT(pTactic);

   const BWeapon *pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());

   if (pUnit->getFlagHasShield() && pWeapon && pWeapon->getFlagShieldDrain())
   {
      BSquad* pSquad = pUnit->getParentSquad();

      if (pSquad)
      {
         pSquad->setFlagStopShieldRegen(false);
      }
   }

   // change the squad mode if we need to
   int newSquadMode = mpProtoAction->getNewSquadMode();
   if (mFlagHasAttacked && newSquadMode != -1)
   {
      BSquad* pSquad = pUnit->getParentSquad();
      if(pSquad)
      {
         BSquadAI* pSquadAI = pSquad->getSquadAI();
         if (pSquadAI)
            pSquadAI->setMode(newSquadMode);
      }
   }

   // change the tactic state if we need to
   int newTacticState = mpProtoAction->getNewTacticState();
   if (mFlagHasAttacked && newTacticState != -1)
      pUnit->setTacticState(newTacticState);
   if (mFlagHasAttacked && mpProtoAction->getClearTacticState())
      pUnit->clearTacticState();

   if (!gWorldReset && pUnit->isAlive())
   {
      if (mSecondaryTurretAttackID != -1)
      {
         BUnitActionSecondaryTurretAttack* pAction = reinterpret_cast<BUnitActionSecondaryTurretAttack*>(pUnit->findActionByID(mSecondaryTurretAttackID));
         if (pAction && pAction->getType() == BAction::cActionTypeUnitSecondaryTurretAttack && pAction->getFlagDestroy() == false)
         {
            pAction->setState(cStateNone);
            if(mTarget.isValid())
               pAction->setPreferredTarget(mTarget);               
         }
         mSecondaryTurretAttackID = -1;
      }

      //DJB - Remove this hack once I get rid of secondary turret attack, or if we get rid of the notion of 2 guns that can't shoot at the same time
      if (mAltSecondaryTurretAttackID != -1)
      {
         BUnitActionSecondaryTurretAttack* pAction = reinterpret_cast<BUnitActionSecondaryTurretAttack*>(pUnit->findActionByID(mAltSecondaryTurretAttackID));
         if (pAction && pAction->getType() == BAction::cActionTypeUnitSecondaryTurretAttack && pAction->getFlagDestroy() == false)
         {
            pAction->setState(cStateNone);
            if(mTarget.isValid())
               pAction->setPreferredTarget(mTarget);               
         }
         mAltSecondaryTurretAttackID = -1;
      }

      // Release slave attack action
      if (mpProtoAction && mpProtoAction->getSlaveAttackActionID() != -1)
      {
         BAction* pSlaveAttackAction = pUnit->getActionByType(BAction::cActionTypeUnitSlaveTurretAttack);
         if (pSlaveAttackAction && (pSlaveAttackAction->getParentAction() == this) && (pSlaveAttackAction->getFlagDestroy() == false))
         {
            pSlaveAttackAction->setParentAction(NULL);
            pSlaveAttackAction->setState(cStateDone);
         }
      }
   }

   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("RangedAttack: ID=%d, Disconnect.", mID);
   #endif

   //-- Blow away the reload
   if(mReloadOppID != BUnitOpp::cInvalidID)
   {
      pUnit->removeOpp(mReloadOppID, true);
      mReloadOppID=BUnitOpp::cInvalidID;
   }

   //Idle the anim.
   playIdleAnimation(true);

   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());

   // If holding target in stasis, release the target
   if (pTargetUnit && mFlagStasisApplied)
   {
      BSquad* pTargetSquad = pTargetUnit->getParentSquad();
      if (pTargetSquad)
         pTargetSquad->releaseStasisEffect();
   }

   // Notify the target that we are no longer attacking it.
   if (pTargetUnit)
      pTargetUnit->removeAttackingUnit(pUnit->getID(), getID());
   
   //Remove us from any battles we're in.
   if (mFlagHasAttacked)
   {
      pUnit->attackActionEnded();
      mFlagHasAttacked = false;
   }

   //Remove any move opp we may have given the unit.  DO NOT remove the action
   //since we're in the middle of stuff with this one.
   pUnit->removeOpp(mMoveOppID, false);
   mMoveOppID=BUnitOpp::cInvalidID;

   pUnit->setFlagTurning(false);

#if DPS_TRACKER
   mTargetDPS = -1.0f;
#endif
   
   if(mpProtoAction->getLowDetailAnimType() != -1)
      gWorld->decrementLowDetailAnimRefCount();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::init()
{
   if (!BAction::init())
      return false;

   mFlagConflictsWithIdle=true;

   mCachedYawHardpointMatrix.makeIdentity();
   mFlagYawHardpointMatrixCached = false;

   mTarget.reset();
   mTargetingLead.zero();
   mOppID=BUnitOpp::cInvalidID;

   mMoveOppID=BUnitOpp::cInvalidID;
   mReloadOppID=BUnitOpp::cInvalidID;
   mFutureState=cStateNone;

   mLaunchPointHandle=-1;
   mAttachmentHandle=-1;
   mBoneHandle=-1;
   mHitZoneIndex=-1;
   
   mFlagAttacking=false;
   mFlagHasAttacked=false;
   mFlagMelee=false;
   mFlagProjectile=false;
   mFlagBeam=false;
   mFlagStasisApplied=false;
   mFlagStasisDrainApplied=false;
   mFlagStasisBombApplied=false;
   mFlagKnockback=false;
   mFlagForceUnitToFace=false;
   mFlagCompleteWhenDoneAttacking=false;

   mStrafingSpot.zero();
   mStrafingDirection.zero();
   mStrafingDirectionTimer = 0.0f;
   mStrafingSpotStuckTimer = 0.0f;
   mFlagStrafing = false;
   mFlagStrafingDirection = false;
   mBeamProjectileID = cInvalidObjectID;

   mBeamDPSRampTimer = 0.0f;
   mSelfHealAmount = 0.0f;

   mSecondaryTurretAttackID = -1;
   mAltSecondaryTurretAttackID = -1;

   mTargetDPS = 0.0f;
   mAddedBaseDPS = 0.0f;

   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   mTargetBeamProjectileID = cInvalidObjectID;
   mKillBeamProjectileID = cInvalidObjectID;
   mTargetBeamProtoProjectile = cInvalidObjectID;
   mKillBeamProtoProjectile = cInvalidObjectID;
   mKillBeamDamage = 0.0f;
   
   mLastLOSValidationTime=(DWORD)0;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::setState(BActionState state)
{
   syncUnitActionData("BUnitActionRangedAttack::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionRangedAttack::setState state", state);
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;
   BDEBUG_ASSERT(mpProtoAction);
//-- FIXING PREFIX BUG ID 3314
   const BUnit* pTarget=gWorld->getUnit(mTarget.getID());
//--
   
   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("RangedAttack::setState: OldState=%s, NewState=%d, WaitTimer=%f.", getStateName(), state, pUnit->getAttackWaitTimer());
   #endif

   switch (state)
   {
      //Moving.
      case cStateMoving:
      {
         //If try to go to moving and we're an attack from a secondary opp, fail.
         const BUnitOpp* pCurrentOpp=pUnit->getOppByID(getOppID());
         if (pCurrentOpp && (pCurrentOpp->getType() == BUnitOpp::cTypeSecondaryAttack) || !pCurrentOpp)
         {
            setState(cStateFailed);
            return (false);
         }
         
         //See if we can get our desired location, fail if not.  If we have a MeleeRange
         //action and we have a valid target, then move to the target's location.
         BVector desiredLocation;
         if (mpProtoAction->getMeleeRange() && pTarget)
            desiredLocation=pTarget->getPosition();
         else
         {
            if (!pSquad->getDesiredChildLocation(pUnit->getID(), desiredLocation))
            {
               setState(cStateFailed);
               return (false);
            }
         }

         // Early out and don't create unit move opportunities if this unit is under
         // physics control.
         // MPB TODO - We may eventually want physics objects that can unit move
         if (pSquad->isSquadAPhysicsVehicle())
         {
            setState(cStateFailed);
            return (false);
         }
         
         //Create the move opp.
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

         playIdleAnimation();
         break;
      }

      case cStateNone:
      {
         //BASSERT(!mFlagStrafing);
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         playIdleAnimation();
         break;
      }
         
      case cStateWorking:
      {
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;
         break;
      }

      case cStateDone:
      case cStateFailed:
      case cStateBlocked:
      {
         //Remove any move opp we may have given the unit.  DO NOT remove the action
         //since we're in the middle of stuff with this one.
         pUnit->removeOpp(mMoveOppID, false);
         mMoveOppID=BUnitOpp::cInvalidID;

         if (state != cStateBlocked)
            releaseControllers();

         //Idle the anim.
         playIdleAnimation();

         // Notify the target that we are no longer attacking it.
         BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
         if (pTargetUnit)
            pTargetUnit->removeAttackingUnit(pUnit->getID(), getID());
         
         //E3: If we're going done, roll for a cheer opp.
         if ((state == cStateDone) && (pUnit->hasAnimation(cAnimTypeCheer)))
         {
            if (getRand(cSimRand)%4 == 0)
            {
               BUnitOpp* pNewOpp=BUnitOpp::getInstance();
               pNewOpp->init();
               pNewOpp->setSource(pUnit->getID());
               pNewOpp->setUserData(cAnimTypeCheer);
               pNewOpp->setType(BUnitOpp::cTypeCheer);
               pNewOpp->setPriority(BUnitOpp::cPriorityLow);
               pNewOpp->setExistForOneUpdate(true);
               if (!pUnit->addOpp(pNewOpp))
                  BUnitOpp::releaseInstance(pNewOpp);
            }
         }

         if (state == cStateDone)
         {
            pUnit->completeOpp(mOppID, true);
         }
         else if (state == cStateFailed)
         {
            pUnit->completeOpp(mOppID, false);
         }
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::update(float elapsed)
{
   SCOPEDSAMPLE(BUnitActionRangedAttack_update)
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   // TRB 11/21/08 - For following while attacking, the attack action normally goes away once the attack
   // anim completes.  Check for the case where the unit isn't actively attacking and the opp has already
   // gone away.
   if (mFlagCompleteWhenDoneAttacking && (getState() != cStateWorking))
   {
      setState(cStateFailed);
   }

   // Invalidate the cached hardpoint matrix
   mFlagYawHardpointMatrixCached = false;

//-- FIXING PREFIX BUG ID 3317
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   if (pUnit->getFlagDoingFatality())
      return true;

   const BProtoObject* pProtoObject = pUnit->getProtoObject();
   BDEBUG_ASSERT(mpProtoAction);

   syncUnitActionData("BUnitActionRangedAttack::update mState", mState);

   #ifdef DEBUGOPPS
   if (mpOwner->getPlayerID() == 1)
      mpOwner->debug("RangedAttack::update: State=%d, WaitTimer=%f.", getStateName(), pUnit->getAttackWaitTimer());
   #endif

   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());

   // If holding target in stasis, check for stasis bomb detonation
   if (pTargetUnit && mFlagStasisApplied)
   {
      if (mpProtoAction->getFlagStasisBomb() && (pTargetUnit->getHitpoints() <= 1.0f) && mpProtoAction->getLinkedActionID()) // The "<= 1.0f" catches aircraft whose death has been delayed for crash control.
      {
         // Trigger Stasis bomb
         const BProtoAction* pStasisBombProtoAction = pUnit->getTactic()->getProtoAction(mpProtoAction->getLinkedActionID());
         BVector aoeGroundZero = pTargetUnit->getPosition();

         // show our impact effect / explosion
         gWorld->createTerrainEffectInstance(pStasisBombProtoAction, aoeGroundZero, cZAxisVector, pUnit->getPlayerID(), pUnit->getFlagVisibleToAll());
         pStasisBombProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeImpact, aoeGroundZero, false, cInvalidObjectID);

         BDamageHelper::doAreaEffectDamage(pUnit->getPlayerID(), pUnit->getTeamID(), pUnit->getID(), const_cast<BProtoAction*>(pStasisBombProtoAction), aoeGroundZero);
      }
   }

   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   BVector pos;
   switch (mState)
   {
      case cStateNone:
      {
         // We shouldn't be strafing in this state
         //BASSERT(!mFlagStrafing);

         //NOTE: IF YOU PUT LOGIC IN HERE, MAKE SURE YOU SEE IF IT'S NEEDED IN
         //THE WORKING STATE, TOO.  THE NEEDS ARE SLIGHTLY DIFFERENT ENOUGH THAT
         //A COMPLETE COMMONIZATION IS NOT POSSIBLE.

         //Don't attack while moving if that flag is set
         if (pUnit->isMoving() && pProtoObject->getFlagDontAttackWhileMoving())
            break;

         //If we're blocked, go to that state.
         if (pUnit->getFlagAttackBlocked() || pSquad->getFlagAttackBlocked())
         {
            setState(cStateBlocked);
            break;
         }

         //Check ammo.
         if (!hasEnoughAmmoForFullAttack())
         {
            syncUnitActionCode("BUnitActionRangedAttack::update stateFailed");
            setState(cStateFailed);
            break;
         }

         //Validate that our target is valid.
         if (!mTarget.getID().isValid() && !mTarget.isPositionValid())
         {
            syncUnitActionCode("BUnitActionRangedAttack::update stateFailed");
            setState(cStateFailed);
            break;
         }

         if (mInitialTargetPos == cInvalidVector)
         {
            bool validHitZone;
            BVector targetOffset;
            getTargetPosition(mTarget, mInitialTargetPos, validHitZone, targetOffset);
            mFacingPos = mInitialTargetPos;
         }

         //Check target state.
         BUnit* pTarget = gWorld->getUnit(mTarget.getID());
         if ((pTarget != NULL) && (!pTarget->isAttackable(mpOwner)))
         {
            syncUnitActionCode("BUnitActionRangedAttack::update stateDone");
            setState(cStateDone);
            break;
         }

         //Check range.

         // TRB 9/2/08 - Following moving target change.
         // Allow unit attack action to at least start up, if the squad has initiated it.  When following, the squad
         // can be in range for a very brief time and delays of even a frame or two in the unit processing the squad's
         // commands can cause the attack to fail to start up.
         bool isTargetMoving = false;
         if (pTarget && pTarget->isMoving())
            isTargetMoving = true;

         if (!isTargetMoving && !validateUnitAttackRange(mTarget, mLastLOSValidationTime))
         {
            if (!pSquad->canMove())
            {
               setState(cStateFailed);
               break;
            }

//-- FIXING PREFIX BUG ID 3315
            const BUnitActionMoveAir* pFlightAction = (BUnitActionMoveAir*)pUnit->getActionByType(BAction::cActionTypeUnitMoveAir);
//--
            if (pFlightAction || pUnit->getFlagPhysicsControl())
            {
               if (pUnit->getFlagPhysicsControl() && pUnit->getPhysicsObject())
                  pUnit->getPhysicsObject()->forceActivate();

               if(!pFlightAction) // DJB - Adding back range checking for ground units.
               {
                  setState(cStateFailed);
                  break;
               }
            }
            else
            {
               // TRB 9/2/08 - Don't allow unit attack action to enter moving state.  The squad will do this.
               // Should this break or fall through to the code below?
               setState(cStateFailed);
               break;
            }
         }

         //At this point, we need to have the controllers to do anything.
         if (!grabControllers())
            break;

         // Can't go on if the animation is locked
         if (pUnit->isAnimationLocked())
            break;

         //At this point, we want to start turning towards the target.
         updateTargetingLead();

         cacheYawHardpointMatrix();

         bool turning = false;
         if (!updateOrientation(elapsed, turning))
         {
            pUnit->setFlagTurning(turning);
            break;
         }
         else
            pUnit->setFlagTurning(turning);

          //At this point, we can still wait for a few things, but we want the controllers
         //in order to get to here.  If we're still waiting to attack, then keep waiting.
         if (pUnit->attackWaitTimerOn(mpProtoAction->getWeaponID()))
         {            
            // AJL FIXME 1/17/08 - Added this so that if the unit starts with wait timers on all weapons,
            // this will cause attacking to occur as soon as any of the weapons timers are up.
            // Possibly have wait timer send a notification when a weapon is finished so this doesn't
            // have to be done every update.
            checkForBetterProtoAction();
            break;
         }

         // Wait for recovery to end if recovering from a sprint.
         if (pSquad->getRecoverType()==cRecoverAttack)
            break;

         //Make sure we can hit the target.  We need to pass this before we can go to
         //the Working state.
         if (!canHitTarget())
         {
            if (mFlagStrafing)
               stopStrafing();

            checkForBetterProtoAction();
            break;
         }

         //Finally.  Go.
         syncUnitActionCode("BUnitActionRangedAttack::update stateWorking");
         setState(cStateWorking);
         break;
      }

      case cStateMoving:
      {
         // We shouldn't be strafing in this state
         //BASSERT(!mFlagStrafing);

         //Check range.
         if (validateUnitAttackRange(mTarget, mLastLOSValidationTime))
            setState(cStateNone);
         break;
      }

      case cStateWorking:
      {
         //NOTE: IF YOU PUT LOGIC IN HERE, MAKE SURE YOU SEE IF IT'S NEEDED IN
         //THE WORKING STATE, TOO.  THE NEEDS ARE SLIGHTLY DIFFERENT ENOUGH THAT
         //A COMPLETE COMMONIZATION IS NOT POSSIBLE.

         //If we have the instant attack set and we haven't attacked yet, then do that.
         //Break if we do that.
         if (!mFlagHasAttacked && mpProtoAction->getInstantAttack())
         {
            doAttack(-1, -1);
         }

         //If we're moving, then just wait.
         if (mMoveOppID != BUnitOpp::cInvalidID)
            break;

         // Don't attack while moving if that flag is set
         if (pUnit->isMoving() && pProtoObject->getFlagDontAttackWhileMoving())
         {
            setState(cStateDone);
            break;
         }

         //Check target state.
         if (mpProtoAction->targetsAir())
         {
            // Keep the physics object awake to orient toward the target
            if (pUnit->getFlagPhysicsControl() && pUnit->getPhysicsObject())
               pUnit->getPhysicsObject()->forceActivate();
         }

         BUnit* pTarget = gWorld->getUnit(mTarget.getID());
         if (mTarget.getID().isValid() && (!pTarget || !pTarget->isAttackable(mpOwner) || pTarget->isHibernating()) && (!mFlagStrafing || !scanForTarget()))
         {
            syncUnitActionCode("BUnitActionRangedAttack::update stateDone");
            setState(cStateDone);
            break;
         }

         //If we've gotten blocked, go to that state.
         if (pUnit->getFlagAttackBlocked() || pSquad->getFlagAttackBlocked())
         {
            setState(cStateBlocked);
            break;
         }

         //If we've lost the controllers, go back to None.
         if (!(validateControllers() || pUnit->getFlagDoingFatality()))
         {
            setState(cStateNone);
            break;
         }

         if (pTarget && pTarget->getFlagNotAttackable())
         {
            setState(cStateDone);
            break;
         }

         // TRB 5/27/08:  Complete attacks once started even if target goes out of range.
         bool completeAttack = true;
         //bool completeAttack = false;
         //if (mpProtoAction)
         //   completeAttack = mpProtoAction->isMeleeAttack();

         //Check range.
         if (!completeAttack && !validateUnitAttackRange(mTarget, mLastLOSValidationTime))
         {
            const BUnitOpp* pCurrentOpp=pUnit->getOppByID(getOppID());
            //-- If we don't have an opp, and our target is now out of range, we're done.
            if(!pCurrentOpp)
            {
               setState(cStateFailed);
               break;
            }

            // mrh 3/18/08
            //bool foundOtherTarget = false;

            if (!pSquad->canMove())
            {
               setState(cStateFailed);
               break;
            }

            // mrh 3/18/08 - Remove this for alpha, this causes guys to not chase their target as soon as it goes out of range (which is not how any normal unit works.)
            //if (mFlagStrafing)
            //{
            //   const BUnitOpp* pCurrentOpp=pUnit->getOppByID(getOppID());
            //   if (!(pCurrentOpp && (pCurrentOpp->getType() == BUnitOpp::cTypeAttack) && (pCurrentOpp->getTarget().getID() == mTarget.getID())))
            //   {
            //      if (scanForTarget())
            //         foundOtherTarget = true;
            //   }
            //}

            // mrh 3/18/08
//-- FIXING PREFIX BUG ID 3316
            const BUnitActionMoveAir* pFlightAction = (BUnitActionMoveAir*)pUnit->getActionByType(BAction::cActionTypeUnitMoveAir);
//--
            if (pFlightAction || pUnit->getFlagPhysicsControl())
            {
               if (pUnit->getFlagPhysicsControl() && pUnit->getPhysicsObject())
                  pUnit->getPhysicsObject()->forceActivate();
            }

            // mrh 3/18/08
            if (!pFlightAction)
            {
               // mrh 3/18/08
               //if (foundOtherTarget)
               //   setState(cStateMoving);
               //else
               setState(cStateFailed);
               break;
            }

            // mrh 3/18/08 - This is confusing.  Replaced with above code to keep infantry from never failing when targets go out of range.
            //BUnitActionMoveAir* pFlightAction = (BUnitActionMoveAir*)pUnit->getActionByType(BAction::cActionTypeUnitMoveAir);
            //if (pFlightAction || pUnit->getFlagPhysicsControl())
            //{
            //   if (pUnit->getFlagPhysicsControl() && pUnit->getPhysicsObject())
            //      pUnit->getPhysicsObject()->forceActivate();
            //
            //   if(!pFlightAction) // DJB - Adding back range checking for ground units.
            //   {
            //      setState(cStateFailed);
            //      break;
            //   }
            //}
            //else if (foundOtherTarget)
            //{
            //   setState(cStateMoving);
            //   break;
            //}
         }

         //Update helpers.
         updateTargetingLead();

         cacheYawHardpointMatrix();

         bool turning = false;
         if (!updateOrientation(elapsed, turning))
         {
            pUnit->setFlagTurning(turning);
            setState(cStateNone);
            break;
         }
         else
            pUnit->setFlagTurning(turning);

         if (mFlagStrafing && mFlagBeam && !canStrafeToTarget())
         {
            setState(cStateNone);
            break;
         }

         //Make sure we can hit the target.  If that fails, bounce back to None.
         //if ((!mFlagStrafing || mFlagBeam) && !canHitTarget())
         //DCP 11/18/08:  This is what's breaking the beam strafing (now that the data is correct).  Coker will re-fix the butt-shooting.
         if (!mFlagStrafing && !canHitTarget())
         {
            setState(cStateNone);            
            break;
         }

         BASSERT(mState == cStateWorking);         

         // Can't go on if the animation is locked or disabled
         if (pUnit->isAnimationLocked())
            break;

         // Try fatality anim - if started, don't attempt to play the attack anim
         if (tryFatality())
            break;

         //At this point, we can still wait for a few things, but we want the controllers
         //in order to get to here.  If we're still waiting to attack, then keep waiting.
         if (pUnit->attackWaitTimerOn(mpProtoAction->getWeaponID()))
         {
            break;
         }

         //Play our attack if we're working.
         playAttackAnimation();
         break;
      }

      case cStateBlocked:
      {
         // We shouldn't be strafing in this state
         //BASSERT(!mFlagStrafing);

         if (!pUnit->getFlagAttackBlocked() && !pSquad->getFlagAttackBlocked())
            setState(cStateNone);
         break;         
      }
   }

   //-- Check to see if we should add to our damage bank   
   updateDamageBank(elapsed);

   // SLB: For debugging
   //if (mFlagStrafing)
   //{
   //   gpDebugPrimitives->addDebugSphere(mStrafingSpot, 1.0f, cDWORDRed);
   //   if (mFlagStrafingDirection)
   //      gpDebugPrimitives->addDebugAxis(mStrafingSpot, cOriginVector, cOriginVector, mStrafingDirection, 3.0f * mStrafingDirection.length());
   //}

   updateBeam(elapsed);

   /*
   // Debug stuff
#if !defined (BUILD_FINAL)
   BMatrix matrix;
   matrix.makeTranslate(mInitialTargetPos);
   gpDebugPrimitives->addDebugCircle(matrix, 5.0f, cDWORDBlue);
#endif
   */

   mFlagYawHardpointMatrixCached = false;

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   switch (eventType)
   {
      // Trigger sweet spotting IK
      case BEntity::cEventAnimSweetSpotTag:
      {
         BVisual* pVisual = const_cast<BVisual *>(pUnit->getVisual());
         if (pVisual)
         {
//-- FIXING PREFIX BUG ID 3318
            const BProtoVisualTag* pTag = (BProtoVisualTag *)data;
//--
            long boneHandle = pTag->mToBoneHandle;
            float start = pTag->mPosition;
            float sweetSpot = pTag->mValue1;
            float end = pTag->mValue2;
            BVector targetPosition;
            BVector targetOffset;
            bool validHitZone;
            getTargetPosition(mTarget, targetPosition, validHitZone, targetOffset);

            if (mpProtoAction->getFlagTentacle())
            {
               pVisual->setIKNodeSweetSpot(boneHandle, mInitialTargetPos, start, sweetSpot, end);
            }
            else
            {
               pVisual->setIKNodeSweetSpot(boneHandle, targetPosition, start, sweetSpot, end);
            }
         }
         break;
      }

      case BEntity::cEventAnimAttackTag:
      {
         syncUnitActionCode("BUnitActionRangedAttack::notify stateAttacking");

         if (mpProtoAction->getFlagTentacle())
         {
            BVector targetPosition;
            BVector targetOffset;
            bool validHitZone;
            getTargetPosition(mTarget, targetPosition, validHitZone, targetOffset);

            float dist = mInitialTargetPos.xyDistance(targetPosition);

            if (dist < 5.0f) // Todo: Data drive this?
            {
               mMissedTentacleAttack = false;
               doAttack(data, data2);
            }
            else
               mMissedTentacleAttack = true;
         }
         else
            doAttack(data, data2);

         break;
      }

      case BEntity::cEventAnimChain:
         {
            BVector targetPosition;
            BVector targetOffset;
            bool validHitZone;
            getTargetPosition(mTarget, targetPosition, validHitZone, targetOffset);

            // Now face the target position (we can't do this at the end of the attac, as it causes popping)
            mInitialTargetPos = targetPosition;
            mFacingPos = mInitialTargetPos;
            // Lack of break intentional -CJS

            // TRB 5/27/08:  Complete attacks once started even if target goes out of range.
            // But once anim completes stop attacking if target moved out of range.
            if (!validateUnitAttackRange(mTarget, mLastLOSValidationTime))
            {
               syncUnitActionCode("BUnitActionRangedAttack::notify outOfRange");

               // TRB 9/4/08 - Set future state since anim events were changing the anim state after this notify call and
               // overwriting the work done by calling setState.
               mFutureState = cStateFailed;
               break;
            }
         }

      case BEntity::cEventAnimLoop:
      case BEntity::cEventAnimEnd:
      {
         if ((data2 == cActionAnimationTrack) && ((mState == cStateWorking) || (mState == cStateMoving)))
         {
            updateVisualAmmo();

            if (mState == cStateWorking)
            {
               syncUnitActionData("BUnitActionRangedAttack::notify senderID", senderID.asLong());
               syncUnitActionData("BUnitActionRangedAttack::notify pUnit", pUnit->getID().asLong());

               // TRB 5/27/08:  Complete attacks once started even if target goes out of range.
               // But once anim completes stop attacking if target moved out of range.
               if (!validateUnitAttackRange(mTarget, mLastLOSValidationTime))
               {
                  syncUnitActionCode("BUnitActionRangedAttack::notify outOfRange");
                  setState(cStateFailed);
                  break;
               }

               if (checkForBetterProtoAction())
               {
                  // New proto action found so keep working
                  syncUnitActionCode("BUnitActionRangedAttack::notify foundBetterProtoAction");
                  break;
               }

               syncUnitActionData("BUnitActionRangedAttack::notify dontLoopAttackAnim", mpProtoAction->getDontLoopAttackAnim());
               if (mpProtoAction->getDontLoopAttackAnim() && senderID == pUnit->getID())
               {
                  syncUnitActionCode("BUnitActionRangedAttack::notify stateDone");
                  setState(cStateDone);
                  break;
               }

               //Do we have enough ammo to do another anim of attacks?
               if (!hasEnoughAmmoForFullAttack())
               {
                  if (mpProtoAction->getStopAttackingWhenAmmoDepleted())
                  {
                     syncUnitActionCode("BUnitActionRangedAttack::notify stateDone");
                     setState(cStateDone);
                     break;
                  }
                  else
                  {
                     syncUnitActionCode("BUnitActionRangedAttack::notify stateFailed");
                     setState(cStateFailed);
                     break;
                  }
               }
               else if (eventType == BEntity::cEventAnimEnd)
               {
                  syncUnitActionCode("BUnitActionRangedAttack::notify stateNone");
                  setState(cStateNone);
                  break;
               }
            }
         }
         break;
      }

      case BEntity::cEventOppComplete:
      {
         //If we have a completed opp that's our move opp, deal with that.
         if (data == mMoveOppID)
         {
            if (validateUnitAttackRange(mTarget, mLastLOSValidationTime))
               mFutureState = cStateNone;
            else
               mFutureState = cStateFailed;
         }
         else if(data == mReloadOppID)
         {
            mReloadOppID = BUnitOpp::cInvalidID;
         }
         break;
      }
      
      case BEntity::cEventSquadModeChanging:         
      {    
         if(data == BSquadAI::cModeAbility)
         {
            //-- Make sure the unit can still use the current protoaction. If it can't then we need to go away now.         
//-- FIXING PREFIX BUG ID 3321
            const BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
//--
            BTactic* pTactic = pUnit->getTactic();
            if (pTargetUnit)            
            {
               BVector sourcePos;
//-- FIXING PREFIX BUG ID 3319
               const BSquad* pParentSquad = pUnit->getParentSquad();
//--
               if (pParentSquad)
                  sourcePos = pParentSquad->getPosition();
               else
                  sourcePos = pUnit->getPosition();

               BProtoAction *pNewProtoAction = pTactic->getProtoAction(pUnit->getTacticState(), pTargetUnit,
               pTargetUnit->getPosition(), pUnit->getPlayer(), sourcePos, pUnit->getID(), -1, true,
               BAction::cActionTypeUnitRangedAttack, false, true);
               if(!pNewProtoAction)
               {
                  //DJBFIXMEALPHA: This is a copy paste hack do minimize changes for a last second alpha fix. 
                  // The proper fix is to move all the secondary and slave disconnect code into a seperate method
                  // that will get called when cStateDone is set on the ranged attack action. That way, the secondary
                  // and slave actions will properly stop attacking BEFORE the model logic changes the attachments out from under
                  // them. 

                  //-- Remove slave attack now before the model logic changes.
                  if (mpProtoAction && mpProtoAction->getSlaveAttackActionID() != -1)
                  {
                     BAction* pSlaveAttackAction = pUnit->getActionByType(BAction::cActionTypeUnitSlaveTurretAttack);
                     if (pSlaveAttackAction && (pSlaveAttackAction->getParentAction() == this))
                     {
                        pSlaveAttackAction->setParentAction(NULL);
                        pSlaveAttackAction->setState(cStateDone);
                     }
                  }

                  //-- We gotta go away now.                
                  setState(cStateDone);
               }
            }                    
         }
         break;
      }
      case BEntity::cEventRecomputeVisualCompleted:
      {
         // If currently attacking, reinitialize the attack to update the visual info
         if (mState == cStateWorking)
            setState(cStateWorking);
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::setTarget(BSimTarget target)
{
   BDEBUG_ASSERT((target.isIDValid() && (target.getID().getType() == BEntity::cClassTypeUnit)) || target.isPositionValid());
   
   mTarget = target;

   if (target.isPositionValid())
      mInitialTargetPos = mTarget.getPosition();
   else
      mInitialTargetPos = cInvalidVector;

   mFacingPos = mInitialTargetPos;
   mFlagForceUnitToFace = false; // new target - don't need to force facing until updateOrientation evaluated
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::setOppID(BUnitOppID oppID)
{
   if (mpOwner && (mOppID != BUnitOpp::cInvalidID))
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionRangedAttack::getPriority() const
{
//-- FIXING PREFIX BUG ID 3322
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
// Get the ranged attack launch position
//==============================================================================
const BVector BUnitActionRangedAttack::getLaunchLocation() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BVector position = cInvalidVector;
   getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &position);

   return (position);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.

   //If we have to have a hardpoint, check that.
//-- FIXING PREFIX BUG ID 3323
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   if (mpProtoAction->getHardpointID() >= 0)
   {
      long hardpointController=pUnit->getHardpointController(mpProtoAction->getHardpointID());
      if (hardpointController != (long)mID)
         return (false);
   }
   //If we're the main attack, check that.
   if (mpProtoAction->getMainAttack())
   {
      if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
         return (false);
   }
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   syncUnitActionData("BUnitActionRangedAttack::grabControllers hardpointID", mpProtoAction->getHardpointID());

   //If we have to have a hardpoint, grab that.
   if (mpProtoAction->getHardpointID() != -1)
   {
      if (!pUnit->grabHardpoint(mID, mpProtoAction->getHardpointID(), getOppID()))
      {
         syncUnitActionCode("BUnitActionRangedAttack::grabControllers return false 1");
         return (false);
      }
   }

   syncUnitActionData("BUnitActionRangedAttack::grabControllers mainAttack", mpProtoAction->getMainAttack());

   //If we're the main attack, grab the attack controller.
   if (mpProtoAction->getMainAttack())
   {
      if (!pUnit->grabController(BActionController::cControllerAnimation, this, getOppID()))
      {
         if (mpProtoAction->getHardpointID() != -1)
            pUnit->releaseHardpoint(mID, mpProtoAction->getHardpointID());
         syncUnitActionCode("BUnitActionRangedAttack::grabControllers return false 2");
         return (false);
      }
   }

   syncUnitActionData("BUnitActionRangedAttack::grabControllers hasPersistentMoveAction", pUnit->hasPersistentMoveAction());

   if (!pUnit->hasPersistentMoveAction()) // Leave the orient controller alone for units with persistent move actions
   {
      //Try to grab the orient controller.  It's okay if we don't get that.
      pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   }
   
   //-- No more reloading happening
   mReloadOppID = BUnitOpp::cInvalidID;
   
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //If we have a hardpoint, release it.
   if (mpProtoAction->getHardpointID() != -1)
      pUnit->releaseHardpoint(mID, mpProtoAction->getHardpointID());
   //If we're the main attack, release the attack controller.
   if (mpProtoAction->getMainAttack())
      pUnit->releaseController(BActionController::cControllerAnimation, this);
   //Release the orientation controller in case we have it.
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::playIdleAnimation(bool force)
{
   if (mFlagAttacking || force)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);

      pUnit->setPostAnim(mID, mpProtoAction, false);
      stopStrafing();
      mFlagAttacking = false;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::playAttackAnimation()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //-- See if this unit wants to use cooldown at start   
   bool timerSet = addPreAttackCooldown(mpProtoAction, pUnit);         
   if(timerSet)
   {
      checkForBetterProtoAction(); //-- Check and see if there is another action we can do
      return;
   }

   //Set new animation.
   long animType = -1;
   if(mpProtoAction->getLowDetailAnimType() != -1 && gWorld->getLowDetailAnimRefCount() >= mpProtoAction->getLowDetailAnimThreshold())
      animType=mpProtoAction->getLowDetailAnimType(); // passed our threshold, so play low detail anim
   else
      animType=mpProtoAction->getAnimType();

   if (animType != pUnit->getAnimationType(cActionAnimationTrack))
   {
      long startAnimType = mpProtoAction->getStartAnimType();
      bool playStartAnim = (!mFlagAttacking && (startAnimType != -1)) ? true : false;
      bool lock = playStartAnim ? mpProtoAction->isStartAnimNoInterrupt() : mpProtoAction->isAnimNoInterrupt();
      long playThisAnimType = playStartAnim ? startAnimType : animType;

      if (mpProtoAction->getPullUnits())
      {
         BUnitActionChargedRangedAttack *pAttackAction = (BUnitActionChargedRangedAttack*)pUnit->getActionByType(BAction::cActionTypeUnitChargedRangedAttack);
         BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());

         // [10-20-08 CJS] Override attack anim when we're doing a pull
         if (pTargetUnit && pAttackAction && pAttackAction->canPull(mpProtoAction, pTargetUnit))
            playThisAnimType = pAttackAction->getProtoAction()->getAnimType();
      }

      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateRangedAttack, playThisAnimType, false, false, -1, lock);
      pUnit->computeAnimation();
      BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateRangedAttack, playThisAnimType));
      mFlagAttacking = true;
   }

   //Poke in our attack anim length as the units attack wait timer   
   pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), 0.0f);
   float animLen=pUnit->getAnimationDuration(cActionAnimationTrack);
   if (animLen == 0.0f) // No attack anim? we're done here.
   {
      setState(cStateDone);
      return;
   }
   else
   {
      if (mpProtoAction->isAttackWaitTimerEnabled())
         pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), animLen);
   }

   //-- If the weapon has an aditional cooldown. Then apply it now.   
   addAttackCooldown(mpProtoAction, pUnit);

   if (mpProtoAction->isStrafing() && !mFlagStrafing)
   {
      mStrafingSpot = getTargetStrafeSpot();
      startStrafing();
      mStrafingDirection.zero();
      mStrafingDirectionTimer = 0.0f;
      mStrafingSpotStuckTimer = 0.0f;
      mFlagStrafingDirection = false;
   }
}


//==============================================================================
//==============================================================================
//DCP 06/08/07: Tim doesn't want this, but I'm leaving the code here just in case.
//E3: Re-enabling to control Scarab Spear.
bool BUnitActionRangedAttack::hasEnoughAmmoForFullAttack()
{
//-- FIXING PREFIX BUG ID 3325
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (!mpProtoAction->usesAmmo())
      return (true);

   float minAmmoAmount=0.0f;
   if (mpProtoAction->getMaxNumAttacksPerAnim() > 0)
      minAmmoAmount=mpProtoAction->getMaxNumAttacksPerAnim()*mpProtoAction->getDamagePerAttack();

   syncUnitActionData("BUnitActionRangedAttack::enoughAmmoForFullAttack minAmmoAmount", minAmmoAmount);
   syncUnitActionData("BUnitActionRangedAttack::enoughAmmoForFullAttack getAmmunition", pUnit->getAmmunition());

   if (pUnit->getAmmunition() < minAmmoAmount)
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::doAttack(long attachmentHandle, long boneHandle)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BDEBUG_ASSERT(mpProtoAction);

   if (mState == cStateBlocked)
      return (false);

   // Notify the target that we have begun to attack it.
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (pTargetUnit)
      pTargetUnit->addAttackingUnit(pUnit->getID(), getID());         

   //If our PA has visual ammo, decrement it.
   if (mpProtoAction->getVisualAmmo() > 0)
      pUnit->decrementVisualAmmo(static_cast<uint>(mpProtoAction->getWeaponID()), 1);

   if (mFlagProjectile)
      doProjectileAttack(attachmentHandle, boneHandle);
   else if (mFlagMelee)
      doMeleeAttack(attachmentHandle, boneHandle);
   else if (mFlagBeam)
      doBeamAttack(attachmentHandle, boneHandle);
   
   if (mFlagKnockback)
      doKnockbackAttack(attachmentHandle, boneHandle);

   if ((mpProtoAction->getFlagStasis() && !mFlagStasisApplied) ||
      (mpProtoAction->getFlagStasisDrain() && !mFlagStasisDrainApplied) ||
      (mpProtoAction->getFlagStasisBomb() && !mFlagStasisBombApplied))
      applyStasisEffect();

   if (mpProtoAction->getFlagDoShakeOnAttackTag())
      mpProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeImpact, pUnit->getPosition(), false, pUnit->getID());

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::doMeleeAttack(long attachmentHandle, long boneHandle)
{
   attachmentHandle;
   boneHandle;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!pTarget)
      return;
   BDEBUG_ASSERT(mpProtoAction);

   //Calculate the damage amount.         
   float damage=BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), mTarget.getID(), mpProtoAction->usesHeightBonusDamage());
   //Notify that we are entering combat.
   if (!mFlagHasAttacked)
   {
      mFlagHasAttacked=true;
      pUnit->attackActionStarted(mTarget.getID());
   }

   //Use some ammo.
   //E3: Re-enabling to control Scarab Spear.
   if (mpProtoAction->usesAmmo())
   {
      float ammoAmount=mpProtoAction->getDamagePerAttack();
      pUnit->adjustAmmunition(-ammoAmount);
   }

   // Throw the unit if we're suppose to.
   if(mpProtoAction->getThrowAliveUnits())
   {
      //-- Throw the whole sqaud. Put this on a flag if we don't want this behavior by default
//-- FIXING PREFIX BUG ID 3326
      const BSquad* pParentSquad = pTarget->getParentSquad();
//--
      if(pParentSquad)
      {
         uint numChildren = pParentSquad->getNumberChildren();
         for(uint i=0; i < numChildren; i++)
         {
            BUnit *pTargetToBeThrown = gWorld->getUnit(pParentSquad->getChild(i));
            if(pTargetToBeThrown)
               pTargetToBeThrown->throwUnit(pUnit->getID(), mpProtoAction->getID());
         }
      }
      else
         pTarget->throwUnit(pUnit->getID(), mpProtoAction->getID());
   }

   // AOE
   float damageDealt = 0.0f;
   if (mpProtoAction->getAOERadius() > 0.0f)
   {
      BEntityIDArray unitsKilled;
      BVector targetPosition = pTarget->getPosition();
      BVector direction = targetPosition - pUnit->getPosition();

      damageDealt = BDamageHelper::doAreaEffectDamage(pUnit->getPlayerID(), pUnit->getTeamID(), pUnit->getID(), const_cast<BProtoAction*>(mpProtoAction), damage, 
         targetPosition, cInvalidObjectID, direction, &unitsKilled, mTarget.getID(), mHitZoneIndex);
   }
   else
   {
      //Deal some damage.
      BVector direction=pTarget->getPosition()-pUnit->getPosition();
     
      damageDealt = BDamageHelper::doDamageWithWeaponType(pUnit->getPlayerID(), pUnit->getTeamID(), pTarget->getID(), const_cast<BProtoAction*>(mpProtoAction), damage,
         mpProtoAction->getWeaponType(), true, direction, 1.0f, pUnit->getPosition(), cInvalidObjectID, pUnit->getID());               

   }

   // See if we need to try to do a pickup
   if (!pUnit->getFlagDoingFatality() && (mpProtoAction->getFlagPickupObject() || (mpProtoAction->getFlagPickupObjectOnKill() && pTarget->getHitpoints() < cFloatCompareEpsilon)))
      pickupObject();

   //-- Play the melee sound
   pUnit->playMeleeAttackSound(pTarget);

   //See if we kill ourselves on attack.
   if (mpProtoAction->getKillSelfOnAttack() && (damageDealt > cFloatCompareEpsilon))
      pUnit->setHitpoints(0.0f);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::doProjectileAttack(long attachmentHandle, long boneHandle)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BDEBUG_ASSERT(mpProtoAction);

   //Projectile launch parms.
   BObjectCreateParms parms;
   parms.mCreatedByPlayerID=pUnit->getPlayerID();
   parms.mPlayerID=pUnit->getPlayerID();
   parms.mProtoObjectID=mpProtoAction->getProjectileID();
   if (parms.mProtoObjectID == -1)
      return;

   syncProjectileData("BUnitActionRangedAttack::doAttack Unit ID", pUnit->getID().asLong());
   syncProjectileData("BUnitActionRangedAttack::doAttack boneHandle", boneHandle);

   //Calculate the point to launch the projectile from.
   getLaunchPosition(pUnit, attachmentHandle, boneHandle, &parms.mPosition, &parms.mForward, &parms.mRight);

   if (mpProtoAction->getCarriedObjectAsProjectileVisual())
   {
//-- FIXING PREFIX BUG ID 3327
      const BObject* carriedObject = gWorld->getObject(pUnit->getCarriedObjectID());
//--
      if (carriedObject)
      {
         parms.mSourceVisual = carriedObject->getID();
         parms.mVisualVariationIndex = carriedObject->getVisualVariationIndex();

         // set the color player id of the object to the carried object - this will set the color x form correctly
         parms.mPlayerID = carriedObject->getColorPlayerID();
      }
   }
   
   //Save the attachment and bone handles.
   mAttachmentHandle=attachmentHandle;
   mBoneHandle=boneHandle;

   doProjectileAttack(parms, pUnit->getID(), mTarget.getID());

   // Bounces
   BEntityID newTarget = mTarget.getID();
   BEntityID curTarget = mTarget.getID();

   BTactic* pTactic = pUnit->getTactic();
   if (!pTactic)
      return;

   const BWeapon* pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
   if (!pWeapon)
      return;

   uint8 bounces = pWeapon->getNumBounces();

   BEntityIDArray hitUnits(0, bounces + 1);
   hitUnits.add(mTarget.getID());

   for (int i = 0; i < bounces; ++i)
   {
      BUnit* pCurTarget = gWorld->getUnit(curTarget);
      if (!pCurTarget)
         continue;

      // Calculate new target
      BEntityIDArray results(0, 100);

      BUnitQuery query(pCurTarget->getPosition(), pWeapon->getBounceRange(), false);
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);   // Only look for enemies
      query.setFlagIgnoreDead(true);
      gWorld->getUnitsInArea(&query, &results);

      if (results.getNumber() <= 0)
         break;

      // Reset sorted list
      mTempScanResults.setNumber(0);
      mTempScanOrder.setNumber(0);

      long numUnits = results.getNumber();
      for (long i = 0; i < numUnits; i++)
      {
         BEntityID targetID = results[i];
         BUnit* pTarget = gWorld->getUnit(targetID);
         if (!pTarget)
            continue;

         // Skip current target
         if (targetID == curTarget)
            continue;

         // Make sure target is valid
         if (!pTarget || !pTarget->isAttackable(mpOwner))
            continue;

         //Don't Auto Attack Me
         if (pTarget->getFlagDontAutoAttackMe())
            continue;

         // Get  priority
         float priority = mpProtoAction->getTargetPriority(pTarget->getProtoObject());
         if (priority < 0.0f)
            continue;

         // Compute distance
         float dist = pTarget->getPosition().distanceSqr(pCurTarget->getPosition());

         BVector targetPosition = applyGravityOffset(pTarget, pTarget->getPosition());
         BUnitActionRangeAttackScanResult curResult = BUnitActionRangeAttackScanResult(pTarget, targetPosition, priority, dist);
         mTempScanResults.add(curResult);
         uint curResultIndex = static_cast<uint>(mTempScanResults.getNumber()) - 1;

         // Sort based on our sorting order
         //    1 - priority
         //    2 - distance
         uint insertIndex = 0;
         uint numOrderList = static_cast<uint>(mTempScanOrder.getNumber());
         while ((insertIndex < numOrderList) &&
                ((curResult.getPriority() < mTempScanResults[mTempScanOrder[insertIndex]].getPriority()) ||
                ((curResult.getPriority() == mTempScanResults[mTempScanOrder[insertIndex]].getPriority()) && (curResult.getDotProduct() > mTempScanResults[mTempScanOrder[insertIndex]].getDotProduct()))))
         {
            insertIndex++;
         }
         mTempScanOrder.insertAtIndex(curResultIndex, insertIndex);
      }

      // Now that the results are sorted from best to last, iterate through them to find the first one that passes the 
      // expensive tests.  The expensive tests is the LOS test which checks for collisions.
      //
      uint numOrderList = static_cast<uint>(mTempScanOrder.getNumber());
      for (uint i = 0; i < numOrderList; i++)
      {
         const BUnitActionRangeAttackScanResult *pCurResult = &mTempScanResults[mTempScanOrder[i]];

         // Check LOS to target
         if (gConfig.isDefined(cConfigTrueLOS))
         {
            if(!gSquadLOSValidator.validateLOS(parms.mPosition, pCurResult->getTarget()->getPosition(), pUnit->getParentSquad(), mpProtoAction, pCurResult->getTarget()->getParentSquad()))
               continue;
         }

         // Make sure we haven't already hit this unit
         if (hitUnits.contains(pCurResult->getTarget()->getID()))
            continue;

         // Use this result.  We are done.
         newTarget = pCurResult->getTarget()->getID();
         break;
      }

      // Can't find new target, done
      if (curTarget == newTarget)
         break;

      BUnit* pNewTarget = gWorld->getUnit(newTarget);
      if (!pNewTarget)
         continue;

      // Calculate new parms
      parms.mPosition = pCurTarget->getPosition();
      parms.mForward = pNewTarget->getPosition() - pCurTarget->getPosition();
      parms.mForward.normalize();
      
      parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
      parms.mRight.normalize();

      doProjectileAttack(parms, curTarget, newTarget);

      curTarget = newTarget;

      hitUnits.add(curTarget);
   }

   // If any of the hit units are outside LOS, light them up
   uint count = hitUnits.size();
   for (uint i = 0; i < count; ++i)
   {
      BUnit* pCurUnit = gWorld->getUnit(hitUnits[i]);
      if (!pCurUnit)
         continue;

      if (!pCurUnit->isVisible(mpOwner->getTeamID()))
      {
         // Not visible, add revealer
         gWorld->createRevealer(mpOwner->getTeamID(), pCurUnit->getPosition(), 10.0f, 5000);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::doProjectileAttack(BObjectCreateParms& parms, const BEntityID startID, const BEntityID targetID)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BUnit* pTarget = (mFlagStrafing && !canHitTarget()) ? NULL : gWorld->getUnit(targetID);
   BDEBUG_ASSERT(mpProtoAction);

   //Get the range.
   float range=mpProtoAction->getMaxRange(pUnit);

   syncProjectileData("BUnitActionRangedAttack::doAttack Unit ID", pUnit->getID().asLong());
   if (pTarget)
      syncProjectileData("BUnitActionRangedAttack::doAttack Target ID", pTarget->getID().asLong());

   syncProjectileData("BUnitActionRangedAttack::doAttack launchPosition", parms.mPosition);

   //Does this protoaction and this target mean we need to shoot at the foot of the target?   
   bool targetGround = false;
   bool collideWithUnits = true;
   if(pTarget && mpProtoAction->getTargetsFootOfUnit())
   {
      targetGround = true;
      collideWithUnits = false;
   }

   //Figure out the target position and offset.
   BVector targetPos;
   BVector targetOffset;
   bool validHitZone;
   getTargetPosition(mTarget, targetPos, validHitZone, targetOffset, targetGround);
//    gpDebugPrimitives->addDebugSphere(targetPos, 1.0f, cDWORDBlue, BDebugPrimitives::cCategoryNone, 2.0f);
//    gpDebugPrimitives->addDebugSphere(targetPos + targetOffset, 1.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone, 2.0f);
//    gpDebugPrimitives->addDebugSphere(targetPos + targetOffset + mTargetingLead, 3.0f, cDWORDYellow);

   //Calculate the damage amount.
   float damage=BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), targetID, mpProtoAction->usesHeightBonusDamage());

   //Notify that we are entering combat.
   if (!mFlagHasAttacked)
   {
      mFlagHasAttacked=true;
      pUnit->attackActionStarted(targetID);
   }

   //Use some ammo.
   //E3: Re-enabling to control Scarab Spear.
   if (mpProtoAction->usesAmmo())
   {
      float ammoAmount=mpProtoAction->getDamagePerAttack();
      pUnit->adjustAmmunition(-ammoAmount);
   }

   //Projectile orientation.
   BVector projectileOrientation=(mpProtoAction->getHardpointID() != -1) ? getOrientation() : XMVectorZero();

   //Take unit accuracy scalar into account.
   bool useMovingAccuracy = (pUnit->isMoving() && (pUnit->getVelocity().length() >= (pUnit->getDesiredVelocity() * 0.9f))) ? true : false;
   float accuracy=mpProtoAction->getAccuracy()*pUnit->getAccuracyScalar() * pUnit->getDodgeScalar();
   float movingAccuracy=mpProtoAction->getMovingAccuracy()*pUnit->getAccuracyScalar() * pUnit->getDodgeScalar();
   float maxDeviation=mpProtoAction->getMaxDeviation();
   float movingMaxDeviation=mpProtoAction->getMovingMaxDeviation();
   if (pUnit->getAccuracyScalar())
   {
      maxDeviation/=pUnit->getAccuracyScalar();
      movingMaxDeviation/=pUnit->getAccuracyScalar();

      // DMG NOTE: Does Tim want this?
      maxDeviation/=pUnit->getDodgeScalar();
      movingMaxDeviation/=pUnit->getDodgeScalar();
   }
   
   //Actually launch the projectile.
   BEntityID projectileId = cInvalidObjectID;
   if (validHitZone)
   {
      targetOffset += gWorld->getProjectileDeviation(parms.mPosition, targetPos + targetOffset, mTargetingLead,
         useMovingAccuracy ? movingAccuracy : accuracy, range,
         useMovingAccuracy ? movingMaxDeviation : maxDeviation,
         mpProtoAction->getAccuracyDistanceFactor(), mpProtoAction->getAccuracyDeviationFactor());
      projectileId = gWorld->launchProjectile( parms, targetOffset, mTargetingLead, projectileOrientation, damage,
         mpProtoAction, (IDamageInfo*)mpProtoAction, mpOwner->getID(), pTarget, mHitZoneIndex, false, collideWithUnits );
   }
   else
   {
      BVector lead = pTarget ? mTargetingLead : XMVectorZero();
      targetOffset += gWorld->getProjectileDeviation(parms.mPosition, pTarget ? ( pTarget->getPosition() + targetOffset ) : targetOffset,
         lead, useMovingAccuracy ? movingAccuracy : accuracy, range,
         useMovingAccuracy ? movingMaxDeviation : maxDeviation,
         mpProtoAction->getAccuracyDistanceFactor(), mpProtoAction->getAccuracyDeviationFactor());
      projectileId = gWorld->launchProjectile( parms, targetOffset, lead, projectileOrientation, damage,
         mpProtoAction, (IDamageInfo*)mpProtoAction, mpOwner->getID(), pTarget, -1, false, collideWithUnits );
   }

   // if we're throwing our carried object, then update any offset, then destroy the carried object
   if (mpProtoAction->getCarriedObjectAsProjectileVisual())
   {
      BObject* projectile = gWorld->getObject(projectileId);
//-- FIXING PREFIX BUG ID 3329
      const BObject* carriedObject = gWorld->getObject(pUnit->getCarriedObjectID());
//--
      if (projectile && carriedObject)
      {
         projectile->setForward(carriedObject->getForward());
         projectile->setRight(carriedObject->getRight());
         projectile->setUp(carriedObject->getUp());

         projectile->setCenterOffset(carriedObject->getCenterOffset());
         projectile->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true, true);
         projectile->computeAnimation();
      }

      pUnit->destroyCarriedObject();
   }

   //See if we kill ourselves on attack.
   if (mpProtoAction->getKillSelfOnAttack())
      pUnit->setHitpoints(0.0f);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::validateHardpoint() const
{
//-- FIXING PREFIX BUG ID 3330
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (mpProtoAction->getHardpointID() < 0)
      return (false);

  return (pUnit->validateHardpoint(mpProtoAction->getHardpointID()));  
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::canStrafeToTarget()
{
   bool useHardpoint=validateHardpoint();
   if (!useHardpoint)
      return true;

   BVector launchPosition, targetPosition;
   bool isMoving;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   const BHardpoint *pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   if (!pHP)
      return false;

   if (!getHardpointPosition(launchPosition))
      return false;

   if (!getSimTargetPosition(targetPosition, isMoving))
      return false;

   BVector targetForward = (targetPosition - launchPosition);
   targetForward.normalize();

   float yawRight = pHP->getYawRightMaxAngle();

   // if this unit has no restrictions on yaw (it can rotate 360 degrees), then use the forward orientation of the hard point
   if (yawRight >= cPi)
   {
      // restrict it to 3/4*pi so that it doesn't strafe through itself.
      return getOrientation().dot(targetForward) > XMScalarCos(cThreePiOver4);
   }

   // This is the "right thing to do" if we ever have beams that don't shoot out of the front of the unit. This is also the much more expensive thing to do.
   //return pUnit->canYawHardpointToWorldPos(mpProtoAction->getHardpointID(), targetPosition, NULL);
   return pUnit->getForward().dot(targetForward) > XMScalarCos(yawRight);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::canHitTarget()
{
   if (mpProtoAction->getDontCheckOrientTolerance())
   {
      return (true);
   }

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3331
   const BUnit* pTarget=gWorld->getUnit(mTarget.getID());
//--

   //Don't check orientation for non-mobile units without turrets.
   bool useHardpoint=validateHardpoint();
   if (!useHardpoint && (pUnit->getProtoObject()->getFlagNonMobile() || pUnit->isLockedDown()))
      return (true);

   // SLB: HACK
   //if (useHardpoint && pUnit->getHardpoint(0)->getFlagSingleBoneIK() && !mFlagStrafing) //-- Removed this because we need to check orientation for infantry
     //return true;

   //Get desired orientation.
   BVector targetPosition;
   bool targetIsMoving;
   if(!getSimTargetPosition(targetPosition, targetIsMoving))
      return false;

   //Launch position. 
   targetPosition+=mTargetingLead;
   
   BVector launchPosition;
   if (useHardpoint)
      getHardpointPosition(launchPosition);
   else
      launchPosition=pUnit->getPosition();

   const float stationaryToleranceAngle=gDatabase.getStationaryTargetAttackToleranceAngle();
   const float movingToleranceAngle=gDatabase.getMovingTargetAttackToleranceAngle();
   const float trackingMovingToleranceAngle=gDatabase.getMovingTargetTrackingAttackToleranceAngle();
   const float abilityToleranceAngle = cPi * cOneThird;

   BPlayer* pPlayer = pUnit->getPlayer();
//-- FIXING PREFIX BUG ID 3332
   const BProtoObject* pProto = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : NULL;
//--
   bool isTracking=false;
   if (pProto)
      isTracking=true;

   float tolerance=1.0f;

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   // TRB 11/15/08:  Even hackier tolerance check than the one for melee.  If ability, allow larger tolerance so it doesn't get stuck in none state and block future orders.
   if (mTarget.getAbilityID() != -1)
      tolerance=XMScalarCos(abilityToleranceAngle);
   else if (pHP && (pHP->getYawLeftMaxAngle() < cFloatCompareEpsilon && pHP->getYawRightMaxAngle() < cFloatCompareEpsilon))
      tolerance=XMScalarCos(XMConvertToRadians((targetIsMoving && isTracking ? trackingMovingToleranceAngle : movingToleranceAngle * 0.5f)));
   //DCP 11/014/08:  The most uber-est melee hack ever.
   else if (mFlagMelee)
      tolerance=XMScalarCos(cPiOver2);
   else
      tolerance=XMScalarCos(XMConvertToRadians((targetIsMoving ? (isTracking ? trackingMovingToleranceAngle : movingToleranceAngle) : stationaryToleranceAngle) * 0.5f));

   // Loosen up the attack tolerance for bombers and strike aircraft
   BSquad* pSquad=pUnit->getParentSquad();
   if (!pSquad)
      return false;
//-- FIXING PREFIX BUG ID 3333
   const BSquadActionCarpetBomb* pBombAction=reinterpret_cast <BSquadActionCarpetBomb*> (pSquad->getActionByType(BAction::cActionTypeSquadCarpetBomb));
//--
   bool bOutOfRange = false;
   if (pBombAction)
      tolerance = 5.0f;

   const BMatrix *cachedYawHardpointMatrix = NULL;
   if (mFlagYawHardpointMatrixCached)
      cachedYawHardpointMatrix = &mCachedYawHardpointMatrix;

   //-- Just see if we could pitch to it if we wanted to, if we're using pitch and yaw as tolerance.
   if(useHardpoint && pUnit->getHardpoint(mpProtoAction->getHardpointID())->getFlagUseYawAndPitchAsTolerance())
   {
      if(pUnit->canPitchHardpointToWorldPos(mpProtoAction->getHardpointID(), targetPosition) == false)
         return false;
      if(pUnit->canYawHardpointToWorldPos(mpProtoAction->getHardpointID(), targetPosition, cachedYawHardpointMatrix) == false)
         return false;
      
      return true;
   }

   targetPosition = applyGravityOffset(pTarget, targetPosition);
   bool result = (bOutOfRange) ? false : pUnit->isHardpointOrientedToWorldPos(mpProtoAction->getHardpointID(), targetPosition, tolerance, cachedYawHardpointMatrix);
   return result;
}

//==============================================================================
//==============================================================================
BVector BUnitActionRangedAttack::getOrientation() const
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BVector forward=pUnit->getForward();
   long hardpointID=mpProtoAction->getHardpointID();
   if (hardpointID < 0)
      return (forward);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (mFlagYawHardpointMatrixCached)
   {
      msBoneMatrix = mCachedYawHardpointMatrix;
      mCachedYawHardpointMatrix.getTranslation(msBonePos);
      if (!pUnit->getHardpointYawTransform(hardpointID, transformedBoneMatrix))
         return (forward);
   }
   else
   {
      if (!pUnit->getHardpointYawLocation(hardpointID, msBonePos, msBoneMatrix, &transformedBoneMatrix))
         return (forward);
   }

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   pUnit->getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);
   worldToHardpointMatrix.invert();

   //-- get transformed bone direction
   BVector transformedBoneDir;
   transformedBoneMatrix.getForward(transformedBoneDir);
   transformedBoneDir.normalize();

   forward = XMVector3TransformNormal(transformedBoneDir, worldToHardpointMatrix);
 
   return (forward);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::updateTargetingLead()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   // If we've got a ground target w/o a valid entity, or we're a melee attack, or we're the super scarab from scenario 7, zero and skip.
   if ((mTarget.isPositionValid() && !mTarget.isIDValid()) || mFlagMelee || (pUnit && (pUnit->getProtoID() == gDatabase.getPOIDScn07Scarab())))
   {
      mTargetingLead.zero();
      return;
   }

   BPlayer* pPlayer = pUnit->getPlayer();
//-- FIXING PREFIX BUG ID 3335
   const BProtoObject* pProto = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : NULL;
//--
   float projectileSpeed=0.0f;
   if (pProto)
      projectileSpeed=pProto->getDesiredVelocity();

   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   BVector targetVelocity;
   targetVelocity.zero();
   float targetSpeed = 0.0f;
   if (pTarget)
   {
      targetVelocity=pTarget->getVelocity();
      targetSpeed=targetVelocity.length();
   }
   if (!pTarget || !pTarget->isMoving() || (projectileSpeed < cFloatCompareEpsilon) || (targetSpeed < cFloatCompareEpsilon))
   {
      mTargetingLead.zero();
      return;
   }

   BVector attackerPos=pUnit->getPosition();
   BVector targetPos=pTarget->getPosition();
   float d=attackerPos.distance(targetPos);
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
void BUnitActionRangedAttack::cacheYawHardpointMatrix()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   mFlagYawHardpointMatrixCached = false;
   if (mpProtoAction)
   {
      long hardpointID = mpProtoAction->getHardpointID();
      if (hardpointID >= 0)
      {
         BVector tempPos;
         if (pUnit->getHardpointYawLocation(hardpointID, tempPos, mCachedYawHardpointMatrix))
            mFlagYawHardpointMatrixCached = true;
      }
   }
}

//==============================================================================
bool BUnitActionRangedAttack::getSimTargetPosition(BVector &targetPosition, bool &isMoving)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   if (!pUnit)
      return false;

   isMoving = false;
   // if we have a target id, resolve it to a unit to use for the position.
   if (mTarget.isIDValid())
   {
      const BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
      if (!pTargetUnit)
      {
         const BSquad* pSquad = gWorld->getSquad(mTarget.getID());
         if (pSquad && pSquad->getLeaderUnit())
            pTargetUnit = pSquad->getLeaderUnit();
      }
      if (pTargetUnit)
      {
         float intersectDistSqr=0.0f;
         targetPosition = pTargetUnit->getSimCenter();
         BVector trajectory, onObsPosition;
         trajectory = pTargetUnit->getSimCenter() - pUnit->getSimCenter();
         bool collided = false;

/*
         // try to use the physics object first
         BPhysicsObject* pPhysicsObject = pTargetUnit->getPhysicsObject();
         if(pPhysicsObject)
         {
            BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pPhysicsObject->getInfoID(), true);

            if (pInfo && (!pInfo->isVehicle() || pInfo->isAircraft()))
            {
               if (pPhysicsObject->raySegmentIntersects(pUnit->getSimCenter(), trajectory, true, intersectDistSqr))
               {
                  trajectory.normalize();
                  onObsPosition = trajectory * sqrt(intersectDistSqr);
                  targetPosition = onObsPosition + pUnit->getSimCenter();
                  collided = true;
               }
            }
         }
*/

         // start with the sim center and then add in the obstruction
         if (!collided && pTargetUnit->getSimBoundingBox()->raySegmentIntersectsFirst(pUnit->getSimCenter(), trajectory, true, NULL, intersectDistSqr))
         {
            trajectory.normalize();
            onObsPosition = trajectory * sqrt(intersectDistSqr);
            targetPosition = onObsPosition + pUnit->getSimCenter();
         }
         isMoving = pTargetUnit->isMoving();
      }
      else
         return false;
   }
   // else, no unit so try the position from the target
   else if (mTarget.isPositionValid())
      targetPosition = mTarget.getPosition();
   else
      return (false);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::updateHardpointOrientation(float elapsed) 
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   long hardpointID = mpProtoAction->getHardpointID();
   syncUnitActionData("BUnitActionRangedAttack::updateHardpointOrientation id", hardpointID);
   if (hardpointID < 0)
      return (false);

   //-- Don't pitch or yaw if we're just checking tolerance
   if (pUnit->getHardpoint(mpProtoAction->getHardpointID())->getFlagUseYawAndPitchAsTolerance())
      return false;

   //Figure the target position from the sim target.
   bool targetMoving = false;
   BVector targetPosition;
   BVector actualTargetPosition;
   if (!getSimTargetPosition(targetPosition, targetMoving))
      return false;

   actualTargetPosition = targetPosition;

   // Calculate target location
   BVector targetLocation = targetPosition + mTargetingLead;
   actualTargetPosition += mTargetingLead;

   // Calculate strafing spot
   if (mFlagStrafing)
   {
      bool equal = targetLocation.almostEqual(mStrafingSpot, 0.01f);
      bool stuck = (mStrafingSpotStuckTimer >= 0.5f);

      // Update damage ramp control
      mBeamDPSRampTimer += elapsed;

      // Apply jitter
      //DCP 11/18/08:  We're turning off jitter.  It looks bad.
      /*if (!(stuck || equal))
      {
         const float jitter = mpProtoAction->getStrafingJitter();
         if (jitter != -1.0f)
         {
            BVector jitterVector(getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f));
            jitterVector *= (elapsed * jitter) / jitterVector.length();

            targetLocation += jitterVector;
         }
      }*/

      // Clamp to strafing tracking speed if we're strafing
      const float strafingTrackingSpeed = mpProtoAction->getStrafingTrackingSpeed();
      if (strafingTrackingSpeed != -1.0f)
      {
         const float elapsedStrafingTrackingSpeed = strafingTrackingSpeed * elapsed;

         BVector strafingOffset;
         strafingOffset.assignDifference(targetLocation, mStrafingSpot);
         if (strafingOffset.length() > elapsedStrafingTrackingSpeed)
         {
            BVector targetStrafingSpot = mStrafingSpot;
            BVector direction = strafingOffset;
            direction.normalize();
            targetStrafingSpot += direction * elapsedStrafingTrackingSpeed;

            // Get launch position
            BVector launchPosition;
            getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition);
            BVector launchForward;
            launchForward.assignDifference(targetStrafingSpot, launchPosition);
            launchForward.normalize();

            // add in the obstruction radius, since MOST of our code calculates range from obs to obs, not from the launch position
            launchPosition = (launchForward * pUnit->getObstructionRadius()) + launchPosition;

            //SLB: Debugging
            //gpDebugPrimitives->addDebugLine(launchPosition, targetStrafingSpot, cDWORDYellow, cDWORDYellow);

            // Get target strafing spot
            //gTerrainSimRep.rayIntersects(targetStrafingSpot, launchForward, targetStrafingSpot);

            // Clamp to (weapon range + the amount of strafing we're allowing) which COULD be longer than the max range.. slightly
            float maxRange = mpProtoAction->getMaxRange(pUnit) + elapsedStrafingTrackingSpeed;
            if (maxRange < targetStrafingSpot.distance(launchPosition))
            {
               targetStrafingSpot = launchForward * maxRange;
               targetStrafingSpot += launchPosition;
            }

            targetLocation = targetStrafingSpot;
         }
      }

      // Clamp to strafing turn rate to smooth out strafe point turns
      if (equal) // Strafe spot isn't moving
      {
         mStrafingSpotStuckTimer = 0.0f;

         // Idle strafing direction
         if (mFlagStrafingDirection)
         {
            const float strafingTurnRate = mpProtoAction->getStrafingTurnRate();

            // Inc timer
            mStrafingDirectionTimer += elapsed;
            // Halwes - 5/30/2008 - Verify strafing turn rate is positive before bothering with division
            if ((strafingTurnRate < 0.0f) || (mStrafingDirectionTimer >= (strafingTurnRate / cTwoPi)))
            {
               // Reset strafing direction
               mStrafingDirection.zero();
               mStrafingDirectionTimer = 0.0f;
               mFlagStrafingDirection = false;
            }
         }
      }
      else // Strafe spot is moving
      {
         // Make sure we use a turn rate
         const float strafingTurnRate = mpProtoAction->getStrafingTurnRate();
         // Halwes - 5/30/2008 - The strafing turn rate is converted to radians when it is read in so if it is set to -1.0 in the data it will NOT be -1.0 here, so
         //                      I changed this to test for a positive turn rate.
         if (strafingTurnRate >= 0.0f)
         {
            mStrafingDirectionTimer = 0.0f;

            // Are we currently strafing direction limited?
            if (mFlagStrafingDirection) // Yes
            {
               const float maxAngle = strafingTurnRate * elapsed;

               // Calculate target direction
               BVector targetDirection;
               targetDirection.assignDifference(targetLocation, mStrafingSpot);
               const float targetDistance = targetDirection.length();
               targetDirection *= (1.0f / targetDistance);

               // Compare with strafing direction.
               const float angle = mStrafingDirection.angleBetweenVector(targetDirection);

               // Clamp
               if ((angle > maxAngle) && !stuck)
               {
                  BVector axis;
                  axis.assignCrossProduct(mStrafingDirection, targetDirection);
                  axis.normalize();

                  BMatrix rotation;
                  rotation.makeRotateArbitrary(maxAngle, axis);
                  rotation.transformVector(mStrafingDirection, mStrafingDirection);

                  const float scale = (mStrafingDirection.dot(targetDirection) * 0.5f) + 0.5f;
                  BDEBUG_ASSERT(scale >= 0.0f);
                  BDEBUG_ASSERT(scale <= 1.0f);
                  targetLocation = mStrafingSpot;
                  targetLocation += mStrafingDirection * targetDistance * scale * scale;

                  if (targetMoving)
                     mStrafingSpotStuckTimer = 0.0f;
                  else
                     mStrafingSpotStuckTimer += elapsed;
               }
               else
               {
                  mStrafingDirection = targetDirection;
                  mStrafingSpotStuckTimer = 0.0f;
               }
            }
            else // No
            {
               // Set initial strafing direction
               mStrafingDirection.assignDifference(targetLocation, mStrafingSpot);
               mStrafingDirection.normalize();
               mFlagStrafingDirection = true;
               mStrafingSpotStuckTimer = 0.0f;
            }
         }
      }

      // Set strafing spot
      mStrafingSpot = targetLocation;
   }

   // Apply gravity offset
   targetLocation = applyGravityOffset(gWorld->getUnit(mTarget.getID()), targetLocation);
   actualTargetPosition = applyGravityOffset(gWorld->getUnit(mTarget.getID()), actualTargetPosition);

   const BMatrix *cachedYawHardpointMatrix = NULL;
   if (mFlagYawHardpointMatrixCached)
      cachedYawHardpointMatrix = &mCachedYawHardpointMatrix;

   if (pUnit->getHardpoint(hardpointID)->getFlagCombined())
      return pUnit->orientHardpointToWorldPos(hardpointID, targetLocation, elapsed, &actualTargetPosition, cachedYawHardpointMatrix);

   bool bYawResult = pUnit->yawHardpointToWorldPos(hardpointID, targetLocation, elapsed, &actualTargetPosition, cachedYawHardpointMatrix);
   bool bPitchResult = pUnit->pitchHardpointToWorldPos(hardpointID, targetLocation, elapsed, &actualTargetPosition);

   return bYawResult && bPitchResult;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::updateOrientation(float elapsed, bool &turning)
{

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BProtoObject* pProtoObject = pUnit->getProtoObject();
   BDEBUG_ASSERT(mpProtoAction);

   // By default, we're not turning
   turning = false;

   //We're not supposed to be turning so skip this
   if (mpProtoAction->getDontCheckOrientTolerance() || ((mState == cStateWorking) && mpProtoAction->getStationary()))
   {
      syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
      return (true);
   }

   //If our hardpoint is valid, then use that.   
   if (validateHardpoint())
   {      
      //bool result = updateHardpointOrientation(elapsed);
      //if (result && !scarab)
      //   return true;
      //updateHardpointOrientation(elapsed);
      bool hardpointCanFace = updateHardpointOrientation(elapsed); 
      if (hardpointCanFace && !mFlagForceUnitToFace)
         return true;
   }
   
   // Halwes - 1/29/2008 - Commented this check out to allow non-mobile units without turrets to orient to target.  This is needed
   //                      for Sniper units set to non-mobile via triggers, and possibly for units under cover.
   // TRB 7/6/07 - Don't check orientation for non-mobile units without turrets.  This used to return
   // false, but that would prevent dug in infantry from attacking.  If the art for dug in guys
   // changes so it can orient then this check should be changed.  canHitTarget() already does this check.
   //if (!mpOwner->canMove())
   //{
   //   syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
   //   return (true);
   //}

   //-- DJB - Added to keep shade turrets from rotating once their target is beyond their turrets arc of fire.
   if (!pProtoObject->getFlagCanRotate() || !mpProtoAction->canOrientOwner())
   {
      syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
      return (true);
   }


   //Figure the target position.
   BVector targetPosition;
   if (mTarget.isIDValid())
   {
//-- FIXING PREFIX BUG ID 3337
      const BEntity* pEntity=gWorld->getEntity(mTarget.getID());
//--
      if (!pEntity)
      {
         syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
         return (false);
      }
      //If our target is attached to us, return true.
      if (pEntity->getFlagAttached())
      {
         if (pUnit->findAttachment(mTarget.getID()))
         {
            syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
            return (true);
         }
      }

      targetPosition=pEntity->getPosition();
   }
   else if (mTarget.isPositionValid())
      targetPosition=mTarget.getPosition();
   else
      return (false);

   if (mpProtoAction->getFlagTentacle())
   {
      targetPosition = mFacingPos;
   }
   
   //Figure the orientation change.  This will have to change when we have turning
   //in the game.
   BVector forward=targetPosition+mTargetingLead-mpOwner->getPosition();
   forward.y=0.0f;
   if (!forward.safeNormalize())
   {
      syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
      return (false); 
   }

   //Figure out what the change angle is.  If it's more than nothing, we need
   //to grab the orient controller to do this.
   float changeAngle=mpOwner->getForward().angleBetweenVector(forward);
   if (changeAngle < cRadiansPerDegree)
   {
      syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
      mFlagForceUnitToFace = false; // we're facing the target now, don't force facing again until hardpoint range exceeded
      return (true);
   }

   //Else, we require the orient controller to do this.
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != getID())
   {
      //If we don't have it, see if we can grab it.
      if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
      {
         syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
         return (false);
      }
   }

   // Get current forward (either from the squad turn radius forward or the unit)
   BSquad* pParentSquad = pUnit->getParentSquad();
   BVector currentFwd;
   if (pParentSquad && pParentSquad->getFlagUpdateTurnRadius())
   {
      currentFwd = pParentSquad->getTurnRadiusFwd();
      changeAngle = currentFwd.angleBetweenVector(forward);

      if (changeAngle < cRadiansPerDegree)
      {
         syncUnitActionCode("BUnitActionRangedAttack::updateOrientation");
         mFlagForceUnitToFace = false; // we're facing the target now, don't force facing again until hardpoint range exceeded
         return (true);
      }
   }
   else
      currentFwd = mpOwner->getForward();

   // Calc new forward based on turn rate
   BVector desiredFwd = forward;
   float maxTurn = pProtoObject->getTurnRate() * cRadiansPerDegree * elapsed;
   bool turnRight = currentFwd.cross(desiredFwd).y > 0.0f;
   if (changeAngle > maxTurn)
   {
      float rotateAmt;
      if (turnRight)
         rotateAmt = maxTurn;
      else
         rotateAmt = -maxTurn;

      forward = currentFwd;
      forward.rotateXZ(rotateAmt);
   }
   
   // Set the turning flag for units that turn in place (and animate)
   if (pProtoObject->getFlagTurnInPlace() && !pUnit->isMoving())
   {
      turning = (changeAngle >= cRadiansPerDegree);
      pUnit->setFlagTurningRight(turnRight);
   }
   
   // For physics units, activate the physics, set some data, and return.  No need to set the unit rotation
   // since this is reset by the physics anyway.
//-- FIXING PREFIX BUG ID 3338
   const BUnitActionMoveAir* pFlightAction = (BUnitActionMoveAir*)pUnit->getActionByType(BAction::cActionTypeUnitMoveAir);
//--
   if (pFlightAction || pUnit->getFlagPhysicsControl())
   {
      if (pUnit->getFlagPhysicsControl() && pUnit->getPhysicsObject())
      {
         if (pParentSquad && pParentSquad->getFlagUpdateTurnRadius() && !pUnit->isMoving())
         {
            pParentSquad->setTurnRadiusFwd(forward);
            mFlagForceUnitToFace = true; // started rotating - don't stop until facing the target head on. This could be set above, but I don't know how it will affect aircraft that early out below
         }

         pUnit->getPhysicsObject()->forceActivate();
      }
      return (true);
   }

   mFlagForceUnitToFace = true; // started rotating - don't stop until facing the target head on. This could be set above, but I don't know how it will affect aircraft that early out above

   //We're good, change it.
   // MPB 7/2/2007 - Use setRotation api so that we can update the physics
   // rotation at the same time
   BVector right, up;
   right.assignCrossProduct(mpOwner->getUp(), forward);
   right.normalize();
   up.assignCrossProduct(forward, right);
   up.normalize();

   BMatrix rot;
   rot.makeOrient(forward, up, right);
   mpOwner->setRotation(rot, true);

   syncUnitActionData("BUnitActionRangedAttack::updateOrientation forward", mpOwner->getForward());
   syncUnitActionData("BUnitActionRangedAttack::updateOrientation up", mpOwner->getUp());
   syncUnitActionData("BUnitActionRangedAttack::updateOrientation right", mpOwner->getRight());
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::getHardpointPosition(BVector& pos) const
{
//-- FIXING PREFIX BUG ID 3339
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   BVector hardpointPos;
   BMatrix hardpointMatrix;

   if (mFlagYawHardpointMatrixCached)
   {
      hardpointMatrix = mCachedYawHardpointMatrix;
      mCachedYawHardpointMatrix.getTranslation(hardpointPos);
   }
   else
   {
      long id=mpProtoAction->getHardpointID();
      if (id == -1)
         return (false);

      if (!pUnit->getHardpointYawLocation(id, hardpointPos, hardpointMatrix))
         return (false);
   }

   BMatrix mat;
   pUnit->getWorldMatrix(mat);
   mat.transformVectorAsPoint(hardpointPos, pos);
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::getLaunchPosition(BUnit* pUnit, long attachmentHandle,
   long boneHandle, BVector* position, BVector* forward, BVector* right) const
{
   if (boneHandle!=-1)
   {
      if (attachmentHandle!=-1)
      {
         BMatrix attachmentMat;
//-- FIXING PREFIX BUG ID 3340
         const BVisualItem* pVisualItem=pUnit->getVisual()->getAttachment(attachmentHandle, &attachmentMat);
//--
         if (pVisualItem)
         {
            BMatrix unitMat;
            pUnit->getWorldMatrix(unitMat);

            BMatrix mat;
            mat.mult(attachmentMat, unitMat);

            BMatrix boneMat;
            if (pVisualItem->getBone(boneHandle, position, &boneMat, NULL, &mat))
            {
               syncProjectileData("BUnitActionRangedAttack::doAttack bone pos1", *position);
               //pUnit->syncCheckVisual();

               if (forward)
               {
                  boneMat.getForward(*forward);
                  forward->normalize();
               }
               if (right)
               {
                  boneMat.getRight(*right);
                  right->normalize();
               }

               return;
            }
         }
      }
      else
      {
//-- FIXING PREFIX BUG ID 3341
         const BVisualItem* pVisualItem=pUnit->getVisual();
//--
         if (pVisualItem)
         {
            BMatrix mat;
            pUnit->getWorldMatrix(mat);
            BMatrix boneMat;
            if (pVisualItem->getBone(boneHandle, position, &boneMat, NULL, &mat))
            {
               syncProjectileData("BUnitActionRangedAttack::doAttack bone pos2", *position);
               //pUnit->syncCheckVisual();

               if (forward)
               {
                  boneMat.getForward(*forward);
                  forward->normalize();
               }
               if (right)
               {
                  boneMat.getRight(*right);
                  right->normalize();
               }

               return;
            }
         }
      }
   }

   if (mLaunchPointHandle != -1)
   {  
//-- FIXING PREFIX BUG ID 3342
      const BVisual* pVisual=pUnit->getVisual();
//--
      if (pVisual)
      {
         BVector launchPoint;
         BMatrix launchPointMatrix;
         if (pVisual->getPointPosition(cActionAnimationTrack, mLaunchPointHandle, launchPoint, &launchPointMatrix))
         {
            syncProjectileData("BUnitActionRangedAttack::doAttack animType", pUnit->getVisual()->getAnimationType(cActionAnimationTrack));
            syncProjectileData("BUnitActionRangedAttack::doAttack launchPoint", launchPoint);
            //pUnit->syncCheckVisual();
            BMatrix mat;
            pUnit->getWorldMatrix(mat);
            BMatrix wsLaunchPointMatrix;
            wsLaunchPointMatrix.mult(launchPointMatrix, mat);

            //BVector foo;
            //mat.transformVectorAsPoint(launchPoint, foo);
            //*position=foo;

            BDEBUG_ASSERT(position);
            wsLaunchPointMatrix.getTranslation(*position);
            if (forward)
            {
               wsLaunchPointMatrix.getForward(*forward);
               forward->normalize();
            }
            if (right)
            {
               wsLaunchPointMatrix.getRight(*right);
               right->normalize();
            }
            return;
         }
      }
   }

   // SLB: Trying it out to see if it's fixed
   //// FIXME AJL 9/18/06 - Visual center is going out of sync too, so disabling until we fix it.
   //*position = pUnit->getVisualCenter();
   //*position= pUnit->getPosition();
   //position->y+=pUnit->getProtoObject()->getObstructionRadiusY();

   // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
   *position = pUnit->getSimCenter();

   if (forward)
      *forward = pUnit->getForward();
   if (right)
      *right = pUnit->getRight();
   syncProjectileData("BUnitActionRangedAttack::doAttack visualCenter", *position);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::getTargetPosition(BSimTarget target, BVector& targetPosition, bool& validHitZone,
   BVector& targetOffset, bool targetGround)
{
   if (mFlagStrafing && !canHitTarget())
   {
      targetPosition.zero();
      validHitZone = false;
      targetOffset = mStrafingSpot;
      return;
   }

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BUnit* pTarget = gWorld->getUnit(target.getID());   

   if (targetGround)
   {
      BVector tempPos = (pTarget) ? pTarget->getPosition() : target.getPosition();
      float retHeight;
      gTerrainSimRep.getHeightRaycast(tempPos, retHeight, true);
      targetPosition = BVector(tempPos.x, retHeight, tempPos.z);
      validHitZone = false;
      targetOffset = cOriginVector;
      return;
   }
     
   //Hit Zone/Target Position.
   if (pTarget && pTarget->getHitZonePosition(mHitZoneIndex, targetPosition))
      validHitZone=true;
   else
   {
      validHitZone=false;
      //If we're shooting at the ground, spread our projectile a bit.
      if (!pTarget && (mpProtoAction->getProjectileSpread() > 0.0f))
         targetPosition = BSimHelper::randomCircularDistribution(target.getPosition(), mpProtoAction->getProjectileSpread(), 0.0f);
      else
      {
         if (pTarget)
            targetPosition=pTarget->getPosition();
         else
            targetPosition=target.getPosition();
      }
   }

   syncUnitActionData("BUnitActionRangedAttack::getTargetPosition targetPosition", targetPosition);
   syncUnitActionData("BUnitActionRangedAttack::getTargetPosition validHitZone", validHitZone);

   /*
   // FIXME AJL 9/7/06 - Bone positions are causing MP games to go out of sync.
   // Disabling for now until we fix the problem.
//-- FIXING PREFIX BUG ID 3343
   const BVisual* pTargetVisual = ((gWorld->isMultiplayerSim() || !pTarget) ? NULL : pTarget->getVisual());
//--
   */
   if (pTarget)
   {
      if (validHitZone)
      {
         // Halwes - 1/30/2007 - Is this the correct calculation and use of the offset vector?
         targetOffset=BVector( 0.0f, pTarget->getHitZoneOffset( mHitZoneIndex ), 0.0f );
         syncProjectileData("BUnitActionRangedAttack::doAttack targetOffset1", targetOffset);
         return;
      }
      // SLB: Use impact points for tracking projectiles
      // [4/30/08 CJS] Fix for NULL dereference
      else if (pUnit->getPlayer()->getProtoObject(mpProtoAction->getProjectileID()) &&
               pUnit->getPlayer()->getProtoObject(mpProtoAction->getProjectileID())->getFlagTracking())
      {
         //// AJL 11/17/06 - Turn off impact points for Tim for now so he can get his accuracy system working.
         //// We can add back impact points once we know what design and art want and have a plan for how they
         //// are going to work with the accuracy system.
         BVisual *pTargetVisual = pTarget->getVisual();
         if (pTargetVisual)
         {
            long impactPointHandle = pTargetVisual->getRandomPoint(cActionAnimationTrack, cVisualPointImpact);
            syncProjectileData("BUnitActionRangedAttack::doAttack impactPointHandle", impactPointHandle);
            if ((impactPointHandle != -1) && pTargetVisual->getPointPosition(cActionAnimationTrack, impactPointHandle, targetOffset))
            {
               BMatrix unitToWorldMatrix;
               pTarget->getWorldMatrix(unitToWorldMatrix);
               unitToWorldMatrix.transformVector(targetOffset, targetOffset);
               syncProjectileData("BUnitActionRangedAttack::doAttack targetOffset1", targetOffset);
               return;
            }
         }
      }

      // SLB: Trying it out to see if it's fixed
      //// FIXME AJL 9/18/06 - Visual center is going out of sync too, so disabling until we fix it.
      //BVector diff = pTarget->getVisualCenter() - pTarget->getPosition();
      //targetOffset = BVector(0.0f, Math::Min(pTarget->getProtoObject()->getObstructionRadiusY(), diff.y), 0.0f);
      //targetOffset=BVector(0.0f, pTarget->getProtoObject()->getObstructionRadiusY(), 0.0f);

      // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
      targetOffset = pTarget->getSimCenter() - pTarget->getPosition();

      syncProjectileData("BUnitActionRangedAttack::doAttack targetOffset1", targetOffset);
      return;
   }

   targetOffset=targetPosition;

   syncProjectileData("BUnitActionRangedAttack::doAttack targetOffset1", targetOffset);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::updateVisualAmmo()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BDEBUG_ASSERT(pUnit->getTactic());

   //If this doesn't take visual ammo, skip.
   if (mpProtoAction->getVisualAmmo() == 0)
      return;

   //Else, see if we have any for this weapon.
   uint weaponIndex=static_cast<uint>(mpProtoAction->getWeaponID());
   if (pUnit->hasVisualAmmo(weaponIndex))
      return;

   //Else, create an opp to reload.
   BUnitOpp* pNewOpp=BUnitOpp::getInstance();
   pNewOpp->init();
   pNewOpp->setType(BUnitOpp::cTypeReload);
   pNewOpp->setPriority(BUnitOpp::cPriorityCritical);
   pNewOpp->setUserData(static_cast<uint16>(mpProtoAction->getReloadAnimType()));
   pNewOpp->setUserData2(static_cast<uint8>(weaponIndex));
   pNewOpp->setSource(mpOwner->getID());
   pNewOpp->setNotifySource(true);
   pNewOpp->generateID();
   if (!pUnit->addOpp(pNewOpp))
      BUnitOpp::releaseInstance(pNewOpp);
   else
      mReloadOppID = pNewOpp->getID();
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::checkForBetterProtoAction()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

//-- FIXING PREFIX BUG ID 3344
   const BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
//--
   if (!pTargetUnit)
      return false;

   BTactic* pTactic = pUnit->getTactic();
   if(!pTactic)
      return false;

   //-- Need to have flag set in order for timers/ammo to work. Otherwise it will just stick with the action its using.
   if(mpProtoAction->canFindBetterAction() == false)
      return false;

   //Get an attack action.
   BVector sourcePos;
//-- FIXING PREFIX BUG ID 3345
   const BSquad* pParentSquad = pUnit->getParentSquad();
//--
   if (pParentSquad)
      sourcePos = pParentSquad->getPosition();
   else
      sourcePos = pUnit->getPosition();

   BProtoAction *pNewProtoAction = pTactic->getProtoAction(pUnit->getTacticState(), pTargetUnit,
      pTargetUnit->getPosition(), pUnit->getPlayer(), sourcePos, pUnit->getID(), -1, true,
      BAction::cActionTypeUnitRangedAttack, true, true);
   if (!pNewProtoAction)
      return false;      
   
   BDEBUG_ASSERT(pNewProtoAction->getActionType() == BAction::cActionTypeUnitRangedAttack);

   // SLB: We have a better action to perform
   if (pNewProtoAction != mpProtoAction)
   {
      setState(cStateNone);
      playIdleAnimation();

      //Release our controllers.
      releaseControllers();

      // Assign new protoaction
      mpProtoAction = pNewProtoAction;

      // Grab controllers now
      grabControllers();

      mFlagHasAttacked=false;
      mFlagMelee=false;
      mFlagProjectile=false;

      //DCPTODO: Determine what type of attack we have.
      if (mpProtoAction->getProjectileID() >= 0)
      {
         mFlagMelee=false;
         mFlagBeam=mpProtoAction->isBeam();
         mFlagProjectile=!mFlagBeam;
      }
      else
      {
         mFlagMelee=true;
         mFlagBeam=false;
         mFlagProjectile=false;
      }

      mTargetDPS = mpProtoAction->getDamagePerSecond();
      mAddedBaseDPS = mpProtoAction->getAddedBaseDPS();      

      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const
{
//-- FIXING PREFIX BUG ID 3347
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (senderID != pUnit->getID())
      return false;

   switch (eventType)
   {
   case BEntity::cEventAnimAttackTag:
      {
         if (mpProtoAction)
         {
            const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
            if ((data != -1) && pHP && !pHP->getFlagSingleBoneIK() && (pHP->getYawAttachmentHandle() != -1))
            {
               if (pHP->getYawAttachmentHandle() == (long)data)
                  return true;
               if ((pHP->getPitchAttachmentHandle() != -1) && (pHP->getPitchAttachmentHandle() == (long)data))
                  return true;

               return false;
            }

            return (data == -1);
         }
         return false;
      }

   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         const BVisual* pVisual = pUnit->getVisual();
         return (pVisual && (pVisual->getAnimationType(data2) == (long)data));
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::updateDamageBank(float elapsed)
{
   //-- Preserve DPS based on Target DPS if we're blocked and we've got our preserve DPS flag on
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if(pUnit && pUnit->getFlagPreserveDPS() && mTargetDPS != 0.0f)
   {
      if(mState == cStateBlocked || mState == cStateNone)
      {
         BSquad* pSquad = pUnit->getParentSquad();
         if(pSquad)
            pSquad->adjustDamageBank(mTargetDPS * elapsed);
      }
   }   

   //-- Add additional DPS into bank if we've got addedBaseDPS specified
   if(mAddedBaseDPS != 0.0f)
   {
      BSquad* pSquad = pUnit->getParentSquad();
      if(pSquad)
         pSquad->adjustDamageBank(mAddedBaseDPS * elapsed);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::addAttackCooldown(const BProtoAction* pProtoAction, BUnit* pUnit)
{
   if(!pProtoAction || !pUnit)
      return false;

   float cooldownMax = pProtoAction->getPostAttackCooldownMax();
   float rolledCooldown = pProtoAction->rollPostAttackCooldown();

   if(cooldownMax != 0.0f)
   {      
      float waitTimer = pUnit->getAttackWaitTimer(pProtoAction->getWeaponID());
      waitTimer += rolledCooldown;
      pUnit->setAttackWaitTimer(pProtoAction->getWeaponID(), waitTimer);         
      return pUnit->attackWaitTimerOn(pProtoAction->getWeaponID());      
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::addPreAttackCooldown(const BProtoAction* pProtoAction, BUnit* pUnit)
{
   if(!pProtoAction || !pUnit)
      return false;

   float cooldownMax = pProtoAction->getPreAttackCooldownMax();
   if(cooldownMax == 0.0f)
      return false;

   float rolledCooldown = pProtoAction->rollPreAttackCooldown();

   //-- Only set timer if it hasn't already been set, or if we're being forced (on connecting, we don't care about previous timer value)
   if(pUnit->getPreAttackWaitTimerSet(pProtoAction->getWeaponID()) == false)
   {
      pUnit->setPreAttackWaitTimer(pProtoAction->getWeaponID(), rolledCooldown);         
      pUnit->setPreAttackWaitTimerSet(pProtoAction->getWeaponID(), true);
      return true;
   }

   pUnit->setPreAttackWaitTimerSet(pProtoAction->getWeaponID(), false);
   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::scanForTarget()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;
   BDEBUG_ASSERT(mpProtoAction);

   // Notify the target that we are no longer attacking it.
   BEntityID oldTargetUnitID = mTarget.getID();
   BUnit* pOldTargetUnit = gWorld->getUnit(oldTargetUnitID);
   if (pOldTargetUnit)
      pOldTargetUnit->removeAttackingUnit(pUnit->getID(), getID());

   // paranoia
   if (!validateHardpoint())
      return false;

   if (mpProtoAction->targetsAir())
      return false;

   //-- use overall max range from current strafing spot
   float maxRange = mpProtoAction->getStrafingMaxDistance();

   //-- Query for nearby units
   BVector ownerPos = mStrafingSpot;
   BEntityIDArray results(0, 100);

   // TRB 3/18/08:  If target squad still has children then only consider them.
   // Getting the parent squad's attack action target is the most reliable way to get the target squad.
   // The unit opp could be used, but it isn't being kept updated by the code below and it seemed better
   // for the squad and unit actions to always stay in sync.  Ideally the opp needs to be updated too.
   // This poking between squad and unit actions is getting out of hand.
//-- FIXING PREFIX BUG ID 3354
   const BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
//--
   if (pAttackAction && pAttackAction->getTarget() && pAttackAction->getTarget()->isIDValid())
   {
//-- FIXING PREFIX BUG ID 3351
      const BSquad* pTargetSquad = gWorld->getSquad(pAttackAction->getTarget()->getID());
//--
      if (pTargetSquad)
      {
         for (uint i = 0; i < pTargetSquad->getNumberChildren(); i++)
         {
            BEntityID newTargetID = pTargetSquad->getChild(i);
            if (newTargetID != oldTargetUnitID)
               results.add(pTargetSquad->getChild(i));
         }
      }
   }

   // No other units in current target squad so get units in area
   if (results.getNumber() == 0)
   {
      BUnitQuery query(ownerPos, maxRange, false);
      query.setUnitVisibility(pUnit->getPlayerID());
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);   // Only look for enemies
      query.setFlagIgnoreDead(true);
      gWorld->getUnitsInArea(&query, &results);
   }
   if (results.getNumber() <= 0)
      return false;

   BSimTarget bestTarget;

   // Get launch position
   BVector launchPosition;
   BVector launchForward;
   getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition, &launchForward);

   // Get max attack range
   float maxAttackRange;
   if (mTarget.isRangeValid())
      maxAttackRange = mTarget.getRange();
   else
      maxAttackRange = mpProtoAction->getMaxRange(pUnit);


   // Reset sorted list
   mTempScanResults.setNumber(0);
   mTempScanOrder.setNumber(0);

   // Iterate through results create a sorted list of valid results.  Sorted from best to worst
   //
   long numUnits = results.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BEntityID targetID = results[i];
      BUnit* pTarget = gWorld->getUnit(targetID);
      if (!pTarget)
         continue;

      // Skip current target
      if (targetID == mTarget.getID())
         continue;

      // Make sure target is valid
      if (!pTarget || !pTarget->isAttackable(mpOwner))
         continue;

      //Don't Auto Attack Me
      if (pTarget->getFlagDontAutoAttackMe())
         continue;

      // Get  priority
      float priority = mpProtoAction->getTargetPriority(pTarget->getProtoObject());
      if (priority < 0.0f)
         continue;

      // Compute DotProduct
      BVector targetPosition = applyGravityOffset(pTarget, pTarget->getPosition());
      BVector targetVector;
      targetVector.assignDifference(targetPosition, launchPosition);
      targetVector.normalize();
      float dot = launchForward.dot(targetVector);


      // Add to the result array
      BUnitActionRangeAttackScanResult curResult = BUnitActionRangeAttackScanResult(pTarget, targetPosition, priority, dot);
      mTempScanResults.add(curResult);
      uint curResultIndex = static_cast<uint>(mTempScanResults.getNumber()) - 1;

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
      mTempScanOrder.insertAtIndex(curResultIndex, insertIndex);
   }

   // Now that the results are sorted from best to last, iterate through them to find the first one that passes the 
   // expensive tests.  The expensive tests are the canOrientToTargetPosition and the LOS test which checks for
   // collisions.
   //
   uint numOrderList = static_cast<uint>(mTempScanOrder.getNumber());
   for (uint i = 0; i < numOrderList; i++)
   {
      const BUnitActionRangeAttackScanResult *pCurResult = &mTempScanResults[mTempScanOrder[i]];

      // Check we can orient to target
      if(!mpProtoAction->getDontCheckOrientTolerance() && !canOrientToTargetPosition(pCurResult->getTargetPosition()))
         continue;

      // Check LOS to target
      if (gConfig.isDefined(cConfigTrueLOS))
      {
         if(!gSquadLOSValidator.validateLOS(launchPosition, pCurResult->getTarget()->getPosition(), pUnit->getParentSquad(), mpProtoAction, pCurResult->getTarget()->getParentSquad()))
            continue;
      }

      // Use this result.  We are done.
      bestTarget.set(pCurResult->getTarget()->getID(), pCurResult->getTargetPosition());
      break;
   }


   // Target found!
   if (bestTarget.isValid())
   {
      if (grabControllers())
      {
         // Preserve old range
         if (mTarget.isRangeValid())
            bestTarget.setRange(mTarget.getRange());
         setTarget(bestTarget);

         // TODO: (TRB 3/18/08)  Need to update the unit opp associated with this?

         // Update squad's target
         BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
         if (pAttackAction)
         {
//-- FIXING PREFIX BUG ID 3353
            const BEntity* pNewTargetUnit = gWorld->getEntity(bestTarget.getID());
//--
            BEntityID newTargetSquadID = cInvalidObjectID;
            if (pNewTargetUnit)
               newTargetSquadID = pNewTargetUnit->getParentID();

            // Make sure target is different
            const BSimTarget* pOldSquadTarget = pAttackAction->getTarget();
            if ((pOldSquadTarget == NULL) || !pOldSquadTarget->isIDValid() ||
                ((pOldSquadTarget->getID() != bestTarget.getID()) && (pOldSquadTarget->getID() != newTargetSquadID)))
            {
               // Make sure old target squad doesn't still have children
               bool squadStillHasChildren = false;
               if (pOldSquadTarget && pOldSquadTarget->isIDValid())
               {
//-- FIXING PREFIX BUG ID 3352
                  const BSquad* pOldTargetSquad = gWorld->getSquad(pAttackAction->getTarget()->getID());
//--
                  if (pOldTargetSquad)
                  {
                     for (uint i = 0; i < pOldTargetSquad->getNumberChildren(); i++)
                     {
                        if (pOldTargetSquad->getChild(i) != oldTargetUnitID)
                        {
                           squadStillHasChildren = true;
                           break;
                        }
                     }
                  }
               }

               if (!squadStillHasChildren)
               {
                  pSquad->calculateRange(&bestTarget, NULL);
                  pAttackAction->setTarget(bestTarget);
               }
            }
         }

         return true;
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
BVector BUnitActionRangedAttack::applyGravityOffset(const BUnit* pTarget, BVector targetPosition) const
{
   // SLB: Pitch up to match the ballistic arc
   if (mpProtoAction->isWeaponAffectedByGravity(mpOwner->getPlayer()))
   {
      const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);

      BVector hardpointPosition;
      BMatrix hardpointMatrix;
      bool gotHardpoint = false;

      if (mFlagYawHardpointMatrixCached)
      {
         hardpointMatrix = mCachedYawHardpointMatrix;
         mCachedYawHardpointMatrix.getTranslation(hardpointPosition);
         gotHardpoint = true;
      }
      else
      {
         if (pUnit->getHardpointYawLocation(mpProtoAction->getHardpointID(), hardpointPosition, hardpointMatrix))
            gotHardpoint = true;
      }

      // Get hardpoint matrix in local space
      if (gotHardpoint)
      {
         // Transform hardpoint matrix to world space
         BMatrix unitWorldMatrix;
         pUnit->getWorldMatrix(unitWorldMatrix);
         hardpointMatrix.mult(hardpointMatrix, unitWorldMatrix);

         // Get world space hardpoint position
         hardpointMatrix.getTranslation(hardpointPosition);

         // Get projectile proto object
         const BPlayer *pPlayer = pUnit->getPlayer();
         const BProtoObject *pProtoObject = pPlayer->getProtoObject(mpProtoAction->getProjectileID());

         // Calculate projectile world space launch direction
         BVector launchDirection;
         bool useGravity;
         float gravity;
         const float maxRange = mpProtoAction->getMaxRange(pUnit);
         gWorld->calculateProjectileLaunchDirection(maxRange, pUnit, pTarget, pProtoObject, hardpointPosition, targetPosition, launchDirection, useGravity, gravity);

         // Project target location onto launchDirection vector
         if (useGravity)
         {
            launchDirection.normalize();
            targetPosition = hardpointPosition + (launchDirection * 100.0f);
         }
      }
   }

   return targetPosition;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::canOrientToTargetPosition(BVector targetPosition) const
{
//-- FIXING PREFIX BUG ID 3295
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   long hardpointID = mpProtoAction->getHardpointID();
   const BMatrix *cachedYawHardpointMatrix = NULL;
   if (mFlagYawHardpointMatrixCached)
      cachedYawHardpointMatrix = &mCachedYawHardpointMatrix;
   return (pUnit->canYawHardpointToWorldPos(hardpointID, targetPosition, cachedYawHardpointMatrix) && pUnit->canPitchHardpointToWorldPos(hardpointID, targetPosition));
}

//==============================================================================
//==============================================================================
BVector BUnitActionRangedAttack::getTargetStrafeSpot()
{
   BVector targetPosition;

   bool targetMoving;
   if (!getSimTargetPosition(targetPosition, targetMoving))
   {
      BASSERT(0);
      return BVector(0,0,0);
   }

   targetPosition = targetPosition + mTargetingLead;

   return (targetPosition);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::stopStrafing()
{
   if (!gWorldReset && mFlagStrafing)
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);
      BSquad* pSquad = pUnit->getParentSquad();
      BDEBUG_ASSERT(pSquad);

      mFlagStrafing = false;
      mFlagStrafingDirection = false;
      mStrafingDirection.zero();
      mStrafingDirectionTimer = 0.0f;

      if (pSquad)
      {
         BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
         if (pAttackAction)
            pAttackAction->decStrafing();
      }            

      BProjectile* pProjectile = gWorld->getProjectile(mBeamProjectileID);
      if (pProjectile)
      {
         pProjectile->kill(true);
         mBeamProjectileID = cInvalidObjectID;         
      }

      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      BProjectile* pTargetBeamProjectile = gWorld->getProjectile(mTargetBeamProjectileID);
      if (pTargetBeamProjectile)
      {
         pTargetBeamProjectile->kill(true);
         mTargetBeamProjectileID = cInvalidObjectID;
      }

      BProjectile* pKillBeamProjectile = gWorld->getProjectile(mKillBeamProjectileID);
      if (pKillBeamProjectile)
      {
         pKillBeamProjectile->kill(true);
         mKillBeamProjectileID = cInvalidObjectID;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::startStrafing()
{
   if (!mFlagStrafing)
   {
      BDEBUG_ASSERT(mBeamProjectileID == cInvalidObjectID);
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);
      BSquad* pSquad = pUnit->getParentSquad();
      BDEBUG_ASSERT(pSquad);

      mFlagStrafing = true;

      if (pSquad)
      {
         BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
         if (pAttackAction)
            pAttackAction->incStrafing();
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::updateBeam(float elapsed)
{
   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   if (mFlagStrafing && ((mBeamProjectileID != cInvalidObjectID) || (mTargetBeamProjectileID != cInvalidObjectID) || (mKillBeamProjectileID != cInvalidObjectID)))
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);
      if (!pUnit)
         return;
      BSquad* pSquad = pUnit->getParentSquad();
      BDEBUG_ASSERT(pSquad);
      if (!pSquad)
         return;
      BSquadAI* pSquadAI = pSquad->getSquadAI();
      BDEBUG_ASSERT(pSquadAI);
      if (!pSquadAI)
         return;
      
      BProjectile* pProjectile = gWorld->getProjectile(mBeamProjectileID);
      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      BProjectile* pTargetProjectile = gWorld->getProjectile(mTargetBeamProjectileID);
      BProjectile* pKillProjectile = gWorld->getProjectile(mKillBeamProjectileID);
      // Modify beam visibility based on squad mode      
      if (pUnit->getProtoID() == gDatabase.getPOIDScn07Scarab())
      {
         long squadMode = pSquadAI->getMode();
         switch (squadMode)
         {
            case BSquadAI::cModeNormal:
            case BSquadAI::cModeScarabScan:
               changeBeamState( pProjectile, true );
               changeBeamState( pTargetProjectile, false );
               changeBeamState( pKillProjectile, false, true, 0.0f );
               break;

            case BSquadAI::cModeScarabTarget:
               changeBeamState( pProjectile, false );
               changeBeamState( pTargetProjectile, true );
               changeBeamState( pKillProjectile, false, true, 0.0f );
               break;

            case BSquadAI::cModeScarabKill:
               changeBeamState( pProjectile, false );
               changeBeamState( pTargetProjectile, false );
               changeBeamState( pKillProjectile, true, true, mKillBeamDamage );
               break;
         }
      }
            
      if (pProjectile)
      {
         // Get launch position
         BVector launchPosition;
         BVector launchRight;
         getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition, NULL, &launchRight);

         // Get beam direction
         BVector beamForward;
         beamForward.assignDifference(mStrafingSpot, launchPosition);
         beamForward.normalize();
         BVector beamUp;
         beamUp.assignCrossProduct(beamForward, launchRight);
         beamUp.normalize();
         BVector beamRight;
         beamRight.assignCrossProduct(beamUp, beamForward);;
         beamRight.normalize();

         // Update beam projectile
         float damageAmount = 0.0f;
         // Check for a damage ramp set-up
//-- FIXING PREFIX BUG ID 3297
         const BTactic* pTactic = pUnit->getTactic();
//--
         if(pTactic)
         {
            const BWeapon *pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
            if (pWeapon && ((pWeapon->getStartDPSPercent() != 100.0f) || (pWeapon->getFinalDPSPercent() != 100.0f)) && (fabs(pWeapon->getDPSRampTime()) > cFloatCompareEpsilon))
            {
               float deltaDPS = pWeapon->getFinalDPSPercent() - pWeapon->getStartDPSPercent();
               float rampRate = 1.0f / fabs(pWeapon->getDPSRampTime());
               float currRampMult = rampRate * mBeamDPSRampTimer;
               if (currRampMult > 1.0f)
                  currRampMult = 1.0f;
               float currDPS = mpProtoAction->getDamagePerSecond() * 0.01f * (pWeapon->getStartDPSPercent() + deltaDPS * currRampMult);
               damageAmount = currDPS * elapsed * pUnit->getDamageModifier();

               // If we're draining shields, then do so
               if (pUnit->getFlagHasShield() && pWeapon && pWeapon->getFlagShieldDrain())
               {
                  // Cause our shields to not regen until we're done
                  BSquad* pSquad = pUnit->getParentSquad();
                  if (pSquad)
                  {
                     pSquad->setFlagShieldDamaged(true);
                     pSquad->updateLastDamagedTime();
                  }

                  float shieldDrain = (currDPS - mpProtoAction->getDamagePerSecond()) * elapsed;
                  pUnit->setShieldpoints(pUnit->getShieldpoints() - shieldDrain);

                  // Out of shields, stop special attack
                  if (pUnit->getShieldpoints() <= 0.0f)
                  {
                     pUnit->setShieldpoints(0.0f);               
                     setState(cStateDone);
                     return;
                  }
               }

               if (mpProtoAction->keepDPSRamp())
                  pUnit->setDPSRampValue(currDPS);
            }
            else
               damageAmount = mpProtoAction->getDamagePerSecond() * elapsed * pUnit->getDamageModifier();

            // Check for a shield ramp set-up
            if (pUnit->getFlagHasShield() && pWeapon && pWeapon->getFlagShieldDrain())
            {
               /*float deltaSPS = pWeapon->getFinalSPSPercent() - pWeapon->getStartSPSPercent();
               float shieldRampRate = 1.0f / fabs(pWeapon->getSPSRampTime());
               float currRampMult = shieldRampRate * mBeamDPSRampTimer;
               if (currRampMult > 1.0f)
               currRampMult = 1.0f;
               float currSPS = (pWeapon->getStartSPSPercent() + deltaSPS * currRampMult);
               pUnit->setShieldpoints(pUnit->getShieldpoints() - currSPS * elapsed);

               // Cause our shields to not regen until we're done
               BSquad* pSquad = pUnit->getParentSquad();
               if (pSquad)
               {
               pSquad->setFlagShieldDamaged(true);
               pSquad->updateLastDamagedTime();
               }

               // Out of shields, stop special attack
               if (pUnit->getShieldpoints() <= 0.0f)
               {
               pUnit->setShieldpoints(0.0f);               
               setState(cStateDone);
               return;
               }

               float currDPS;
               if (pWeapon->getFinalDPSPercent() > 0.0f) 
               {
               float deltaDPS = pWeapon->getFinalDPSPercent() - pWeapon->getStartSPSPercent();
               deltaDPS = min(deltaSPS, deltaDPS);
               currDPS = (pWeapon->getStartSPSPercent() + deltaDPS * currRampMult);
               }
               else
               currDPS = currSPS;

               damageAmount += currDPS * elapsed;

               if (mpProtoAction->keepDPSRamp())
               pUnit->setDPSRampValue(currDPS);*/
            }
         }

         pProjectile->setPosition(launchPosition);
         pProjectile->setForward(beamForward);
         pProjectile->setRight(beamRight);
         pProjectile->setTargetLocation(mStrafingSpot);
         pProjectile->setDamage(damageAmount);
         pProjectile->setDamageInfo((IDamageInfo*)mpProtoAction);

         // Halwes - 10/2/2008 - Update beams here if sub-updating to alleviate draw delay
         pProjectile->update(elapsed);
         BObject* pHead = gWorld->getObject(pProjectile->getBeamHeadID());
         if (pHead)
            pHead->update(elapsed);
         BObject* pTail = gWorld->getObject(pProjectile->getBeamTailID());
         if (pTail)
            pTail->update(elapsed);

         // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
         if (pTargetProjectile)
         {
            pTargetProjectile->setPosition(launchPosition);
            pTargetProjectile->setForward(beamForward);
            pTargetProjectile->setRight(beamRight);
            pTargetProjectile->setTargetLocation(mStrafingSpot);
            pTargetProjectile->setDamage(damageAmount);
            pTargetProjectile->setDamageInfo((IDamageInfo*)mpProtoAction);
            // Halwes - 10/2/2008 - Update beams here if sub-updating to alleviate draw delay
            pTargetProjectile->update(elapsed);
            BObject* pHead = gWorld->getObject(pTargetProjectile->getBeamHeadID());
            if (pHead)
               pHead->update(elapsed);
            BObject* pTail = gWorld->getObject(pTargetProjectile->getBeamTailID());
            if (pTail)
               pTail->update(elapsed);
         }

         if (pKillProjectile)
         {
            pKillProjectile->setPosition(launchPosition);
            pKillProjectile->setForward(beamForward);
            pKillProjectile->setRight(beamRight);
            pKillProjectile->setTargetLocation(mStrafingSpot);
            pKillProjectile->setDamage(damageAmount);
            pKillProjectile->setDamageInfo((IDamageInfo*)mpProtoAction);
            // Halwes - 10/2/2008 - Update beams here if sub-updating to alleviate draw delay
            pKillProjectile->update(elapsed);
            BObject* pHead = gWorld->getObject(pKillProjectile->getBeamHeadID());
            if (pHead)
               pHead->update(elapsed);
            BObject* pTail = gWorld->getObject(pKillProjectile->getBeamTailID());
            if (pTail)
               pTail->update(elapsed);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::changeBeamState( BProjectile *projectile, bool visible, bool setDamage, float damage)
{
   if (projectile == NULL)
      return;

   projectile->setFlagVisibleForOwnerOnly(!visible);

   if(setDamage)
      projectile->setDamage(damage);

   BObject* pHead = gWorld->getObject(projectile->getBeamHeadID());
   if (pHead)
      pHead->setFlagVisibleForOwnerOnly(!visible);

   BObject* pTail = gWorld->getObject(projectile->getBeamTailID());
   if (pTail)
      pTail->setFlagVisibleForOwnerOnly(!visible);
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::doBeamAttack(long attachmentHandle, long boneHandle)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if (mBeamProjectileID == cInvalidObjectID)
   {
      //Save the attachment and bone handles.
      mAttachmentHandle = attachmentHandle;
      mBoneHandle = boneHandle;

      // Get launch position
      BVector launchPosition;
      BVector launchRight;
      getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition, NULL, &launchRight);

      // Get beam direction
      BVector beamForward;
      beamForward.assignDifference(mStrafingSpot, launchPosition);
      beamForward.normalize();
      BVector beamUp;
      beamUp.assignCrossProduct(beamForward, launchRight);
      beamUp.normalize();
      BVector beamRight;
      beamRight.assignCrossProduct(beamUp, beamForward);;
      beamRight.normalize();

      // Get target position
      BVector targetPosition;
      bool moving;
      if (!getSimTargetPosition(targetPosition, moving))
         targetPosition = launchPosition;

      // Create beam projectile
      BObjectCreateParms parms;
      parms.mPlayerID = pUnit->getPlayerID();
      parms.mProtoObjectID = mpProtoAction->getProjectileID();
      parms.mPosition = launchPosition;
      parms.mForward = beamForward;
      parms.mRight = beamRight;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;
      parms.mForward = targetPosition; // Used to set the target location when creating the beam

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         mBeamProjectileID = pProjectile->getID();
         pProjectile->setFlagBeam(true);
         pProjectile->setFlagNoWorldUpdate(true);
         if (mpProtoAction)
         {
            pProjectile->setFlagBeamCollideWithUnits(mpProtoAction->doesBeamCollideWithUnits());
            pProjectile->setFlagBeamCollideWithTerrain(mpProtoAction->doesBeamCollideWithTerrain());
         }
         pProjectile->setParentID(pUnit->getID());
         pProjectile->setAttachmentHandle(attachmentHandle);
         pProjectile->setBoneHandle(boneHandle);
         pProjectile->setLaunchPointHandle(mLaunchPointHandle);
         float damage = BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), mTarget.getID(), mpProtoAction->usesHeightBonusDamage());
         pProjectile->setDamage(damage);
         pProjectile->setDamageInfo((IDamageInfo*) mpProtoAction);
         pProjectile->setTargetLocation(targetPosition);

         // Set up stasis drain effect
//-- FIXING PREFIX BUG ID 3298
         const BTactic* pTactic = pUnit->getTactic();
//--
         BDEBUG_ASSERT(pTactic);
         const BWeapon* pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
         if (mpProtoAction->getFlagStasis())
            mSelfHealAmount = mpProtoAction->getDamagePerSecond() * pWeapon->getStasisHealToDrainRatio();
      }
   }

   if (mSelfHealAmount != 0.0f)
   {
      BSquad* pSquad = pUnit->getParentSquad();
      float excessHP = 0.0f;
      pSquad->repairHitpoints(mSelfHealAmount, false, excessHP);
   }

   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   if ((mTargetBeamProtoProjectile != cInvalidObjectID) && (mTargetBeamProjectileID == cInvalidObjectID))
   {
      //Save the attachment and bone handles.
      //mAttachmentHandle = attachmentHandle;
      //mBoneHandle = boneHandle;

      // Get launch position
      BVector launchPosition;
      BVector launchRight;
      getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition, NULL, &launchRight);

      // Get beam direction
      BVector beamForward;
      beamForward.assignDifference(mStrafingSpot, launchPosition);
      beamForward.normalize();
      BVector beamUp;
      beamUp.assignCrossProduct(beamForward, launchRight);
      beamUp.normalize();
      BVector beamRight;
      beamRight.assignCrossProduct(beamUp, beamForward);;
      beamRight.normalize();

      // Get target position
      BVector targetPosition;
      if (mTarget.isPositionValid())
         targetPosition = mTarget.getPosition();
      else if (mTarget.isIDValid())
      {
         BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
         if (pTargetEntity)
            targetPosition = pTargetEntity->getPosition();
         else
            targetPosition = launchPosition;
      }
      else
         targetPosition = launchPosition;

      // Create beam projectile
      BObjectCreateParms parms;
      parms.mPlayerID = pUnit->getPlayerID();
      parms.mProtoObjectID = mTargetBeamProtoProjectile;
      parms.mPosition = launchPosition;
      parms.mForward = beamForward;
      parms.mRight = beamRight;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;
      parms.mForward = targetPosition; // Used to set the target location when creating the beam

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         mTargetBeamProjectileID = pProjectile->getID();
         pProjectile->setFlagBeam(true);
         pProjectile->setFlagNoWorldUpdate(true);
         if (mpProtoAction)
         {
            pProjectile->setFlagBeamCollideWithUnits(mpProtoAction->doesBeamCollideWithUnits());
            pProjectile->setFlagBeamCollideWithTerrain(mpProtoAction->doesBeamCollideWithTerrain());
         }
         pProjectile->setParentID(pUnit->getID());
         pProjectile->setAttachmentHandle(attachmentHandle);
         pProjectile->setBoneHandle(boneHandle);
         pProjectile->setLaunchPointHandle(mLaunchPointHandle);
         float damage = BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), mTarget.getID(), mpProtoAction->usesHeightBonusDamage());
         pProjectile->setDamage(damage);
         pProjectile->setDamageInfo((IDamageInfo*) mpProtoAction);
         pProjectile->setTargetLocation(targetPosition);

         // Set up stasis drain effect
         BTactic* pTactic = pUnit->getTactic();
         BDEBUG_ASSERT(pTactic);
         const BWeapon* pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
         if (mpProtoAction->getFlagStasis())
            mSelfHealAmount = mpProtoAction->getDamagePerSecond() * pWeapon->getStasisHealToDrainRatio();
      }
   }

   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   if ((mKillBeamProtoProjectile != cInvalidObjectID) && (mKillBeamProjectileID == cInvalidObjectID))
   {
      //Save the attachment and bone handles.
      //mAttachmentHandle = attachmentHandle;
      //mBoneHandle = boneHandle;

      // Get launch position
      BVector launchPosition;
      BVector launchRight;
      getLaunchPosition(pUnit, mAttachmentHandle, mBoneHandle, &launchPosition, NULL, &launchRight);

      // Get beam direction
      BVector beamForward;
      beamForward.assignDifference(mStrafingSpot, launchPosition);
      beamForward.normalize();
      BVector beamUp;
      beamUp.assignCrossProduct(beamForward, launchRight);
      beamUp.normalize();
      BVector beamRight;
      beamRight.assignCrossProduct(beamUp, beamForward);;
      beamRight.normalize();

      // Get target position
      BVector targetPosition;
      if (mTarget.isPositionValid())
         targetPosition = mTarget.getPosition();
      else if (mTarget.isIDValid())
      {
         BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
         if (pTargetEntity)
            targetPosition = pTargetEntity->getPosition();
         else
            targetPosition = launchPosition;
      }
      else
         targetPosition = launchPosition;

      // Create beam projectile
      BObjectCreateParms parms;
      parms.mPlayerID = pUnit->getPlayerID();
      parms.mProtoObjectID = mKillBeamProtoProjectile;
      parms.mPosition = launchPosition;
      parms.mForward = beamForward;
      parms.mRight = beamRight;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;
      parms.mForward = targetPosition; // Used to set the target location when creating the beam

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         mKillBeamProjectileID = pProjectile->getID();
         pProjectile->setFlagBeam(true);
         pProjectile->setFlagNoWorldUpdate(true);
         if (mpProtoAction)
         {
            pProjectile->setFlagBeamCollideWithUnits(mpProtoAction->doesBeamCollideWithUnits());
            pProjectile->setFlagBeamCollideWithTerrain(mpProtoAction->doesBeamCollideWithTerrain());
         }
         pProjectile->setParentID(pUnit->getID());
         pProjectile->setAttachmentHandle(attachmentHandle);
         pProjectile->setBoneHandle(boneHandle);
         pProjectile->setLaunchPointHandle(mLaunchPointHandle);
         float damage = BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), mTarget.getID(), mpProtoAction->usesHeightBonusDamage());
         pProjectile->setDamage(damage);
         mKillBeamDamage = damage;
         pProjectile->setDamageInfo((IDamageInfo*) mpProtoAction);
         pProjectile->setTargetLocation(targetPosition);

         // Set up stasis drain effect
         BTactic* pTactic = pUnit->getTactic();
         BDEBUG_ASSERT(pTactic);
         const BWeapon* pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
         if (mpProtoAction->getFlagStasis())
            mSelfHealAmount = mpProtoAction->getDamagePerSecond() * pWeapon->getStasisHealToDrainRatio();
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::doKnockbackAttack(long attachmentHandle, long boneHandle)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!pTarget)
      return;
   BDEBUG_ASSERT(mpProtoAction);

   // Don't do anything if we're attached to something
   if (pTarget->isAttached())
      return;
   if (!pTarget->getPhysicsObject())
      return;

   BVector impulse;
   impulse = pTarget->getPosition() - pUnit->getPosition();
   impulse.normalize();
   impulse.y = fabs(impulse.y);

   // Multiply in physics force
   BTactic* pTactic = pUnit->getTactic();
   if(!pTactic)
      return;

//-- FIXING PREFIX BUG ID 3299
   const BWeapon* pWeapon = (BWeapon*)pTactic->getWeapon(mpProtoAction->getWeaponID());
//--
   if(!pWeapon)
      return;

   // Apply launch angle
   float launchAngle = getRandRangeFloat(cSimRand, pWeapon->mpStaticData->mPhysicsLaunchAngleMin, pWeapon->mpStaticData->mPhysicsLaunchAngleMax);
   launchAngle = Math::fDegToRad(launchAngle);
   impulse *= cosf(launchAngle); 

   float force = getRandRangeFloat(cSimRand, pWeapon->mpStaticData->mPhysicsForceMin, pWeapon->mpStaticData->mPhysicsForceMax);
   force *= pTarget->getPhysicsObject()->getMass();
   impulse *= force;

   // If the vehicle already has a certain amount of upward velocity, don't add any more
   BVector vel;
   pTarget->getPhysicsObject()->getLinearVelocity(vel);
   if(vel.y > cTopYVelOfKnockbackUnit)
      impulse.y = 0.0f;
   else if(vel.y > cMedYVelOfKnockbackUnit)  //scale the impulse down if the vel is between cMedYVelOfKnockbackUnit and cTopYVelOfKnockbackUnit;
      impulse.y = impulse.y * (1.0 - (vel.y - cMedYVelOfKnockbackUnit) / (cTopYVelOfKnockbackUnit - cMedYVelOfKnockbackUnit));

   pTarget->getPhysicsObject()->applyImpulse(impulse);
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::attemptToRemove()
{
   //-- The point of this code is to keep the ranged attack around if it is trying to be removed, and we want to allow the unit
   //-- to continue shooting between the removal and when a new opp comes in.
   if(getProtoAction() && getProtoAction()->persistBetweenOpps())
   {
//-- FIXING PREFIX BUG ID 3300
      const BUnit* pUnit = getOwner()->getUnit();
//--
      if(pUnit)
      {
         setOppID(BUnitOpp::cInvalidID);
         setOrder(NULL);
         return false; //-- It's not ok to remove the action.
      }
   }
   return true; //-- Go ahead and remove us.
}

//==============================================================================
//==============================================================================
void BUnitActionRangedAttack::changeTarget(const BSimTarget* pNewTarget, BUnitOppID oppID, BSimOrder* pOrder)
{
//-- FIXING PREFIX BUG ID 3301
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   // Notify the target that we are no longer attacking it.
   BUnit* pOldTargetUnit = gWorld->getUnit(mTarget.getID());
   if (pOldTargetUnit)
      pOldTargetUnit->removeAttackingUnit(pUnit->getID(), getID());

   // If holding old target in stasis, release the target
   if (pOldTargetUnit && mFlagStasisApplied)
   {
      BSquad* pTargetSquad = pOldTargetUnit->getParentSquad();
      if (pTargetSquad)
      {
         pTargetSquad->releaseStasisEffect();
         mFlagStasisApplied = false;
         mFlagStasisDrainApplied = false;
         mFlagStasisBombApplied = false;
      }
   }

   // Change target
   if (pNewTarget)
      setTarget(*pNewTarget);

   // Change Opp   
   setOppID(oppID);

   // Change Order
   setOrder(pOrder);
}

//=============================================================================
// BUnitActionRangedAttack::applyStasisEffect
//=============================================================================
void  BUnitActionRangedAttack::applyStasisEffect()
{
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   if (!pTarget)
      return;
   BSquad* pTargetSquad = pTarget->getParentSquad();
   if (!pTargetSquad)
      return;

   // transporters can't be stasis - would have loved to put this in the target rule
   // but the special case target ground code for that won't allow it
   if (pTarget->isType(gDatabase.getOTIDTransporter()))
      return;

   pTargetSquad->addStasisEffect();

//-- FIXING PREFIX BUG ID 3302
   const BUnitActionStasis* pExistingStasisAction = reinterpret_cast<BUnitActionStasis*>(pTarget->getActionByType(BAction::cActionTypeUnitStasis));
//--
   if (!pExistingStasisAction)
   {
      //Create the action.
      BAction* pNewAction = gActionManager.createAction(BAction::cActionTypeUnitStasis);
      bool success = pTarget->addAction(pNewAction);
      BASSERT(success); // Shouldn't fail
   }

   mFlagStasisApplied = true;

   if (mpProtoAction->getFlagStasisDrain())
      mFlagStasisDrainApplied = true;

   if (mpProtoAction->getFlagStasisBomb())
      mFlagStasisBombApplied = true;

}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::canPickup(const BUnit* pUnit)
{
   if (!pUnit)
      return false;

   // can't pickup a unit if they're attached to something else, being destroyed, or doing a fatality
   if (pUnit->isAttached() || pUnit->getFlagDestroy() || pUnit->getFlagDoingFatality())
      return false;

   // always allow picking up of thrown physics objects - may need to setup flags for this soon
   if (pUnit->getProtoID() == gDatabase.getPOIDPhysicsThrownObject())
      return true;

   // the only corpses we're allowed to pickup are infantry right now
   if ((!pUnit->isAlive() || pUnit->getHitpoints() <= cFloatCompareEpsilon) && !pUnit->getFlagDoingFatality() && pUnit->getProtoObject() && pUnit->getProtoObject()->isType(gDatabase.getOTIDInfantry()))
      return true;

   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::pickupObject()
{
   syncUnitActionCode("BUnitActionRangedAttack::pickupObject");

   // if one of these functions succeeds, we actually succeeded at picking up an object
   // see if we can tear a part off our target
   if (tearAttachmentOffTarget())
      return true;

   // if not, see if we can tear a part off our target
   if (tearPartOffTarget())
      return true;

   // if not, see if we can find a carry object
   return findCarryObject();   
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::tearAttachmentOffTarget()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearAttachmentOffTarget invalid owner");
      return false;
   }

   // do the tear off attachment if we have a valid target (if hitpoints are <= 0, then the until will die on the next frame)
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (!pTargetUnit)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearAttachmentOffTarget invalid target");
      return false;
   }
   BRelationType relation = gWorld->getTeamRelationType(pUnit->getTeamID(), pTargetUnit->getTeamID());
   syncUnitActionData("BUnitActionRangedAttack::tearAttachmentOffTarget relation", relation);
   syncUnitActionData("BUnitActionRangedAttack::tearAttachmentOffTarget pTargetUnit->getHitpoints", pTargetUnit->getHitpoints());
   syncUnitActionData("BUnitActionRangedAttack::tearAttachmentOffTarget pTargetUnit->getFlagDoingFatality", pTargetUnit->getFlagDoingFatality());
   if (relation != cRelationTypeEnemy || pTargetUnit->getHitpoints() > cFloatCompareEpsilon || pTargetUnit->getFlagDoingFatality())
      return false;

   // grab our target's visual
   BVisual * pTargetVisual = pTargetUnit->getVisual();
   if (!pTargetVisual)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearAttachmentOffTarget no visual");
      return false;
   }

   // see if there's an attachment we can pull off
   BVisualItem* origAttachment = findNearestThrowableAttachment(pTargetUnit, pUnit->getPosition());
   if (!origAttachment || !origAttachment->mpName)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearAttachmentOffTarget invalid attachment");
      return false;
   }

   // there is an attachment we can rip off, so create the carry object
   BVector centerOfMassOffset;
   centerOfMassOffset.zero();
   long fromBoneHandle = -1;
//-- FIXING PREFIX BUG ID 3305
   const BObject* carryObject = createCarryObject(pTargetUnit, origAttachment, fromBoneHandle, centerOfMassOffset);
//--
   if (!carryObject)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearAttachmentOffTarget no carry object");
      return false;
   }

   // hide the attachment on the original object 
   origAttachment->setGrannyMeshMask(false);

   // do the carry!
   pUnit->carryObject(carryObject->getID(), fromBoneHandle, NULL);

   return true;
}

//==============================================================================
//==============================================================================
BVisualItem* BUnitActionRangedAttack::findNearestThrowableAttachment(BObject* pObject, const BVector& positionWorldSpace)
{
   if (!pObject)
      return NULL;

   BVisual* pVisual = pObject->getVisual();
   if (!pVisual)
      return NULL;

   // Compute model space position
   BVector positionModelSpace;
   BMatrix matrix;
   pObject->getInvWorldMatrix(matrix);
   matrix.transformVectorAsPoint(positionWorldSpace, positionModelSpace);

   float closestDistSqr = cMaximumFloat;
   BVisualItem* closestAttachment = NULL;
   long count = pVisual->mAttachments.getNumber();
   for (long i = 0; i < count; ++i)
   {
      BVisualItem* pAttachment = pVisual->mAttachments[i];
      if(pAttachment->getFlag(BVisualItem::cFlagUsed))
      {
         // we ignore ones that are fully hidden or not granny instances
//-- FIXING PREFIX BUG ID 3306
         const BGrannyInstance* pGrannyInstance = pAttachment->getGrannyInstance();
//--
         if (!pGrannyInstance || pGrannyInstance->getMeshRenderMask().areAllBitsClear())
            continue;

         // the attachment must have a pickup handle as well
         if (pAttachment->getPointHandle(cActionAnimationTrack, cVisualPointPickup) == -1)
            continue;

         // else, go ahead and check the distance
         // Find the closest to the given position
         BVector bonePosition;
         pVisual->getBone(pAttachment->mToBoneHandle, &bonePosition);

         float newDistanceSqr = positionModelSpace.distanceSqr(bonePosition);
         if(newDistanceSqr < closestDistSqr)
         {
            closestDistSqr = newDistanceSqr;
            closestAttachment = pAttachment;
         }
      }
   }

   return closestAttachment;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::tearPartOffTarget()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // check for a valid target
   BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
   if (!pTargetUnit)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearPartOffTarget invalid target");
      return false;
   }
   BRelationType relation = gWorld->getTeamRelationType(pUnit->getTeamID(), pTargetUnit->getTeamID());
   syncUnitActionData("BUnitActionRangedAttack::tearPartOffTarget relation", relation);
   if (relation != cRelationTypeEnemy)
      return false;

   // do the tear off part 
   BEntityID partId = pTargetUnit->forceThrowPartImpactPoint(pUnit->getPosition());
   BObject* pPartObject = gWorld->getObject(partId);
   if (!pPartObject)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearPartOffTarget no part");
      return false;
   }

   // create the carry object
   BVector centerOfMassOffset;
   centerOfMassOffset.zero();
   long fromBoneHandle = -1;
//-- FIXING PREFIX BUG ID 3307
   const BObject* carryObject = createCarryObject(pPartObject, NULL, fromBoneHandle, centerOfMassOffset);
//--
   if (!carryObject)
   {
      syncUnitActionCode("BUnitActionRangedAttack::tearPartOffTarget no carry object");
      return false;
   }

   // actually carry the object
   pUnit->carryObject(carryObject->getID(), fromBoneHandle, &centerOfMassOffset);

   // destroy the old unit
   pPartObject->kill(true);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::findCarryObject()
{
   syncUnitActionCode("BUnitActionRangedAttack::findCarryObject");

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3308
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BASSERT(pSquad);

   // our target is the only thing we can pickup now
   BUnit* pTarget=gWorld->getUnit(mTarget.getID());
   if (!pTarget)
   {
      syncUnitActionCode("BUnitActionRangedAttack::findCarryObject invalid target");
      return false;
   }
   if (!canPickup(pTarget))
   {
      syncUnitActionCode("BUnitActionRangedAttack::findCarryObject can't pickup target");
      return false;
   }

   // create the carry object
   BVector centerOfMassOffset;
   centerOfMassOffset.zero();
   long fromBoneHandle = -1;
//-- FIXING PREFIX BUG ID 3309
   const BObject* carryObject = createCarryObject(pTarget, NULL, fromBoneHandle, centerOfMassOffset);
//--
   if (!carryObject)
   {
      syncUnitActionCode("BUnitActionRangedAttack::findCarryObject createCarryObject failed");
      return false;
   }

   // actually carry the object
   if (!pUnit->carryObject(carryObject->getID(), fromBoneHandle, &centerOfMassOffset))
   {
      syncUnitActionCode("BUnitActionRangedAttack::findCarryObject carryObject failed");
      return false;
   }

   // kill off the old object
   pTarget->kill(true);

   return true;
}

//==============================================================================
//==============================================================================
BObject* BUnitActionRangedAttack::createCarryObject(const BObject* pSourceObject, BVisualItem* pSourceVisual, long& meshPickupBone, BVector& centerOfMassOffset)
{
   syncUnitActionCode("BUnitActionRangedAttack::createCarryObject");

//-- FIXING PREFIX BUG ID 3310
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   if (!pSourceObject)
      return NULL;

   // create a copy of the object
   BObjectCreateParms params;
   params.mPosition = pSourceObject->getPosition();
   params.mForward = pSourceObject->getForward();
   params.mRight = pSourceObject->getRight();
   params.mNoTieToGround = true;
   params.mPlayerID = pUnit->getPlayerID();
   params.mPhysicsReplacement = true;
   params.mProtoObjectID = gDatabase.getPOIDPhysicsThrownObject();
   params.mProtoSquadID = -1;
   params.mStartBuilt = false;
   params.mType = BEntity::cClassTypeObject;
   BObject* returnObject = NULL;

   // if we're building off a visual item, use createvis, else build on our own
   if (pSourceVisual)
      returnObject = gWorld->createVisItemPhysicsObject(params, pSourceVisual, NULL, NULL);
   else
   {
      // if this is an object not based off of the visual item, we need to do a few special things here
      params.mSourceVisual = pSourceObject->getID();
      returnObject = gWorld->createObject(params);
   }
   if (!returnObject)
      return NULL;

   // set the player id and the color id correctly
   returnObject->setPlayerID(pUnit->getPlayerID());
   returnObject->setColorPlayerID(pSourceObject->getColorPlayerID());

   // if we have a physics object, get the physics center of mass
   if (pSourceObject->getPhysicsObject())
   {
      BVector centerOfMassLocation;
      pSourceObject->getPhysicsObject()->getCenterOfMassLocation(centerOfMassLocation);
      centerOfMassOffset = centerOfMassLocation - pSourceObject->getPosition();
   }

   // go ahead and lookup the pickup bone as well
   // see if the object we're picking up has a pickup visual point
//-- FIXING PREFIX BUG ID 3311
   const BVisual* attachVisual = returnObject->getVisual();
//--
   if (attachVisual)
   {
      long pickupPointHandle = attachVisual->getPointHandle(cActionAnimationTrack, cVisualPointPickup);
      const BProtoVisualPoint* pickupPointData = attachVisual->getPointProto(cActionAnimationTrack, pickupPointHandle);
      if (pickupPointData)
         meshPickupBone = pickupPointData->mBoneHandle;
   }

   return returnObject;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::tryFatality()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   // MS 11/13/2008: PHX-17972, don't do this if we're garrisoned or attached
   if(pUnit->isGarrisoned() || pUnit->isAttached())
      return false;

   //////////////////////////////////////////////////////////////////////////
   // SLB: Fatality stuff
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   if (pTarget && !pTarget->getFlagInfected())
   {
      // Are we about to kill the target?
      BVector direction = pTarget->getPosition() - pUnit->getPosition();
      float damageAmount = BDamageHelper::getDamageAmount(pUnit->getID(), mpProtoAction->getDamagePerAttack(), mTarget.getID(), mpProtoAction->usesHeightBonusDamage(), false);
      if (BDamageHelper::doesDamageWithWeaponTypeKillUnit(pUnit->getPlayerID(), pUnit->getTeamID(), pTarget->getID(), const_cast<BProtoAction*>(mpProtoAction), damageAmount,
         mpProtoAction->getWeaponType(), true, direction, 1.0f, pUnit->getPosition(), pUnit->getID()))
      {
         // Yes we are. Lets see if we can do a fatality move.
         uint assetIndex;
         BFatality* pFatality = mpProtoAction->getFatality(pUnit, pTarget, assetIndex);
         if (pFatality)
         {
            // Start fatality.  If started successfully, return
            bool started = gFatalityManager.startFatality(mID, pUnit, pTarget, pFatality, assetIndex);
            if (started)
               return true;
         }
      }
   }
   return false;
   //////////////////////////////////////////////////////////////////////////
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVECTOR(pStream, mTargetingLead);
   GFWRITEVECTOR(pStream, mStrafingSpot);
   GFWRITEVECTOR(pStream, mStrafingDirection);
   GFWRITEVAR(pStream, float, mStrafingDirectionTimer);
   GFWRITEVAR(pStream, float, mStrafingSpotStuckTimer);
   GFWRITEVAR(pStream, float, mBeamDPSRampTimer);
   GFWRITEVAR(pStream, BEntityID, mBeamProjectileID);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BUnitOppID, mMoveOppID);
   GFWRITEVAR(pStream, BUnitOppID, mReloadOppID);
   GFWRITEVAR(pStream, long, mLaunchPointHandle);
   GFWRITEVAR(pStream, long, mAttachmentHandle);
   GFWRITEVAR(pStream, long, mBoneHandle);
   GFWRITEVAR(pStream, long, mHitZoneIndex);      
   GFWRITEVAR(pStream, float, mTargetDPS);
   GFWRITEVAR(pStream, float, mAddedBaseDPS);
   GFWRITEVAR(pStream, float, mSelfHealAmount);
   GFWRITEVAR(pStream, int, mSecondaryTurretAttackID);
   GFWRITEVAR(pStream, int, mAltSecondaryTurretAttackID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   GFWRITEVECTOR(pStream, mInitialTargetPos);
   GFWRITEVECTOR(pStream, mFacingPos);
   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   GFWRITEVAR(pStream, BEntityID, mTargetBeamProjectileID);
   GFWRITEVAR(pStream, BEntityID, mKillBeamProjectileID);
   GFWRITEVAR(pStream, BProtoObjectID, mTargetBeamProtoProjectile);
   GFWRITEVAR(pStream, BProtoObjectID, mKillBeamProtoProjectile);
   GFWRITEVAR(pStream, float, mKillBeamDamage);
   GFWRITEVAR(pStream, DWORD, mLastLOSValidationTime);
   GFWRITEBITBOOL(pStream, mFlagAttacking);
   GFWRITEBITBOOL(pStream, mFlagHasAttacked);
   GFWRITEBITBOOL(pStream, mFlagMelee);
   GFWRITEBITBOOL(pStream, mFlagProjectile);
   GFWRITEBITBOOL(pStream, mFlagBeam);
   GFWRITEBITBOOL(pStream, mFlagStrafing);
   GFWRITEBITBOOL(pStream, mFlagStrafingDirection);
   GFWRITEBITBOOL(pStream, mFlagStasisApplied);
   GFWRITEBITBOOL(pStream, mFlagStasisDrainApplied);
   GFWRITEBITBOOL(pStream, mFlagStasisBombApplied);
   GFWRITEBITBOOL(pStream, mFlagKnockback);
   GFWRITEBITBOOL(pStream, mMissedTentacleAttack);
   GFWRITEBITBOOL(pStream, mFlagForceUnitToFace);
   GFWRITEBITBOOL(pStream, mFlagCompleteWhenDoneAttacking);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRangedAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVECTOR(pStream, mTargetingLead);
   GFREADVECTOR(pStream, mStrafingSpot);
   GFREADVECTOR(pStream, mStrafingDirection);
   GFREADVAR(pStream, float, mStrafingDirectionTimer);
   GFREADVAR(pStream, float, mStrafingSpotStuckTimer);
   GFREADVAR(pStream, float, mBeamDPSRampTimer);
   GFREADVAR(pStream, BEntityID, mBeamProjectileID);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BUnitOppID, mMoveOppID);
   if (BAction::mGameFileVersion >= 10)
      GFREADVAR(pStream, BUnitOppID, mReloadOppID);
   GFREADVAR(pStream, long, mLaunchPointHandle);
   GFREADVAR(pStream, long, mAttachmentHandle);
   GFREADVAR(pStream, long, mBoneHandle);
   GFREADVAR(pStream, long, mHitZoneIndex);      
   GFREADVAR(pStream, float, mTargetDPS);
   GFREADVAR(pStream, float, mAddedBaseDPS);
   GFREADVAR(pStream, float, mSelfHealAmount);
   GFREADVAR(pStream, int, mSecondaryTurretAttackID);
   GFREADVAR(pStream, int, mAltSecondaryTurretAttackID);
   GFREADVAR(pStream, BActionState, mFutureState);
   GFREADVECTOR(pStream, mInitialTargetPos);
   GFREADVECTOR(pStream, mFacingPos);
   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   if (BAction::mGameFileVersion >= 17)
   {
      GFREADVAR(pStream, BEntityID, mTargetBeamProjectileID);
      GFREADVAR(pStream, BEntityID, mKillBeamProjectileID);
      GFREADVAR(pStream, BProtoObjectID, mTargetBeamProtoProjectile);
      GFREADVAR(pStream, BProtoObjectID, mKillBeamProtoProjectile);
      GFREADVAR(pStream, float, mKillBeamDamage);
   }
   if (BAction::mGameFileVersion >= 42)
      GFREADVAR(pStream, DWORD, mLastLOSValidationTime);

   GFREADBITBOOL(pStream, mFlagAttacking);
   GFREADBITBOOL(pStream, mFlagHasAttacked);
   GFREADBITBOOL(pStream, mFlagMelee);
   GFREADBITBOOL(pStream, mFlagProjectile);
   GFREADBITBOOL(pStream, mFlagBeam);
   GFREADBITBOOL(pStream, mFlagStrafing);
   GFREADBITBOOL(pStream, mFlagStrafingDirection);
   GFREADBITBOOL(pStream, mFlagStasisApplied);
   GFREADBITBOOL(pStream, mFlagStasisDrainApplied);
   GFREADBITBOOL(pStream, mFlagStasisBombApplied);
   GFREADBITBOOL(pStream, mFlagKnockback);
   GFREADBITBOOL(pStream, mMissedTentacleAttack);
   GFREADBITBOOL(pStream, mFlagForceUnitToFace);

   if (BAction::mGameFileVersion >= 50)
      GFREADBITBOOL(pStream, mFlagCompleteWhenDoneAttacking);

   // The hardpoint matrix shouldn't have been cached while saving since that would mean it was in the middle
   // of calling update.  Just clear this flag to ensure it gets calculated next update, and the matrix doesn't need to be saved.
   mFlagYawHardpointMatrixCached = false;

   return true;
}
