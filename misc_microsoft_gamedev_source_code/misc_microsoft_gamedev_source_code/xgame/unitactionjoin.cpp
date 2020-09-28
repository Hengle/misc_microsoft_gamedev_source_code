//==============================================================================
// unitactionjoin.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "aimission.h"
#include "UnitActionJoin.h"
#include "Unit.h"
#include "User.h"
#include "UserManager.h"
#include "selectionmanager.h"
#include "World.h"                            
#include "game.h"
#include "tactic.h"
#include "protosquad.h"
#include "particlesystemmanager.h"
#include "particlegateway.h"
#include "unitactionheal.h"
#include "unitactionbubbleshield.h"
#include "simhelper.h"
#include "grannymanager.h"
#include "grannymodel.h"
#include "grannyinstance.h"
#include "ability.h"
#include "generaleventmanager.h"
#include "triggermanager.h"
#include "configsgame.h"
#include "squadactionattack.h"
#include "unitactionavoidcollisionair.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionJoin, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionJoin::connect(BEntity* pOwner, BSimOrder* pOrder)
{

   BUnit* pUnit=reinterpret_cast<BUnit*>(pOwner);
   if (!pUnit)
      return false;
   if (!mpProtoAction)
      return false;
   if (!BAction::connect(pOwner, pOrder))
      return (false);
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
   if (!pTargetSquad)
      return false;
   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
   if (!pTargetUnit)
      return false;

   // Don't join buildings or enemy squads
   const BProtoObject* pTargetProto = pTargetSquad->getProtoObject();
   if (!pTargetProto || pTargetProto->isType(gDatabase.getOTIDBuilding()) || pTargetProto->isType(gDatabase.getOTIDBuildingSocket()))
      return false;

   

   // Ensure that no more than one unit of our type can join a squad
   MERGE_TYPE mergeType = mpProtoAction->getMergeType();
   if (!mAllowMultiple)
   {
      if (pTargetSquad->hasMergeType(mergeType))
         return false;
   }

   // If I have another join action, don't do another one
   long numActions = pUnit->getNumberActions();
   for (long i = 0; i < numActions; ++i)
   {
      const BAction* pAction = pUnit->getActionByIndexConst(i);
      if (pAction == this)
         continue;

      if (pAction->getType() == BAction::cActionTypeUnitJoin)
         return false;
   }

   // If the target squad has a join action, don't join it
   // Make sure the target doens't have another join action of the same type as me
   int ofs = 0;
   BTactic* pTactic = pTargetUnit->getTactic();
   if (pTactic)
   {
      BProtoAction *pProto = pTactic->getProtoAction(ofs);
      while (pProto)
      {
         if (pProto->getActionType() == BAction::cActionTypeUnitJoin)
         {
            if (pProto->getMergeType() == mergeType)
               return false;
         }

         ++ofs;
         pProto = pTactic->getProtoAction(ofs);
      }
   }

   //==============================================================================
   // Do join type specific connection stuff
   JOIN_TYPE joinType = mpProtoAction->getJoinType();
   if (joinType == cJoinTypeFollow || joinType == cJoinTypeFollowAttack)
   {
      // Set action persistent
      mFlagPersistent = true;

      // Make invulnerable and un-selectable
      pUnit->setFlagInvulnerable(true);
      pUnit->setFlagSelectable(false);
      pUnit->setFlagCollidable(false);
      pUnit->updateObstruction();

      BSquad* pParent = pUnit->getParentSquad();
      if (pParent)
      {
         pParent->setFlagSelectable(false);
         pParent->setFlagCollidable(false);
         pParent->updateObstruction();

         // Make sure I don't attack anything unless my parent does (assuming it can attack)
         if (pTactic && pTactic->hasEnabledAttackAction(false) && joinType == cJoinTypeFollowAttack)
            pParent->setPassive();
      }
   }
   else if (joinType == cJoinTypeMerge)
   {
      // Merge squads by adding unit to target squad and updating proto squad type
      BSquad* pParentSquad = pUnit->getParentSquad();
      if (pParentSquad)
      {
         BProtoSquadID mergedID;
         bool canMerge = pParentSquad->getProtoSquad()->canMerge(pTargetSquad->getProtoSquadID(), mergedID);
         BASSERT(canMerge);
         if (canMerge)
         {
            // Save off original proto squad id for later restoration
            mOriginalProtoSquadID = pParentSquad->getProtoSquadID();

            // Remove this unit from its squad and add to target squad
            pParentSquad->removeChild(pUnit->getID(), false);
            pParentSquad->setFlagIgnorePop(true);  // set ignorePop so that the pop for this squad isn't subtracted upon death
            pTargetSquad->addChild(pUnit->getID());

            // Reinitialize formation for target squad
            //pTargetSquad->initPosition(true);

            // Transform proto squad type
            BPlayer* pPlayer = const_cast<BPlayer*>(pTargetSquad->getPlayer());
            if (pPlayer)
            {
               // Change proto squad
               pPlayer->removeSquadFromProtoSquad(pTargetSquad, pTargetSquad->getProtoSquadID());
               pTargetSquad->setProtoID(mergedID);
               pPlayer->addSquadToProtoSquad(pTargetSquad, mergedID);
            }

            // Notify target squad it has gotten a new unit
            pTargetSquad->notify(BEntity::cEventSquadUnitAdded, pTargetSquad->getID(), pUnit->getID(), 0);

            // TODO - Die last
         }
      }
   }
   else if (joinType == cJoinTypeBoard)
   {
      BSquad* pParentSquad = pUnit->getParentSquad();

      // Get the controllers
      if (!grabControllers())
         return false;

      // If it's an aircraft and it's crashing, it's not jackable
      BUnitActionAvoidCollisionAir* pAvoid = (BUnitActionAvoidCollisionAir*)pTargetUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir);
      if (pAvoid && pAvoid->Crashing())
         return false;

      // Cache board target's former owner
      mBoardTargetFormerOwnerID = pTargetUnit->getPlayerID();

      // Set action persistent so it can't be removed until the boarding is complete
      mFlagPersistent = true;

      // Look for anim to board
      uint assetIndex;

      // Mark the target as "NotAttackable" because I am trying to board this thing and I want it alive when I get on!
      if (pTargetUnit)
         pTargetUnit->setFlagNotAttackable(true);

      // MS 9/23/2008: set "being boarded" flag too
      if (pTargetUnit)
         pTargetUnit->setFlagBeingBoarded(true);

      // Add entity ref to target unit for boarding unit
      pTargetUnit->addEntityRef(BEntityRef::cTypeBoardingUnit, pUnit->getID(), 0, 0);

      // E3 2008--hide the HP bar so it doesn't get in the way
      if (pParentSquad)
         pParentSquad->setFlagHasHPBar(false);

      BFatality* pFatality = mpProtoAction->getFatality(pUnit, pTargetUnit, assetIndex);
      if (pFatality)
      {
         // Start fatality.  If started successfully, return
         DWORD boardTime = static_cast<DWORD>(mpProtoAction->getJoinBoardTime() * 1000.0f);
         bool started = gFatalityManager.startFatality(mID, pUnit, pTargetUnit, pFatality, assetIndex, boardTime);
         if (started)
         {
            // Remove and disable idles during boarding
            // TODO - Do we still need this?
            setFlagConflictsWithIdle(true);
            pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);

            // Set fatality end anim type to look for to trigger boarding
            mBoardStartAnimType = pFatality->mAttackerAnimType;
            mBoardEndAnimType = pFatality->mAttackerEndAnimType;

            pUnit->setFlagTiesToGround(false);

            // MPB [8/26/2008] - Taking this out for now so that hijacking targets don't lose their current movement/attacks.
            // If the fatality offset stuff is working, it shouldn't be a problem to move the target while the spartan is hopping on.

            // If the fatality system isn't doing this, set fatality flag and clear out hardpoints/actions on target
            // so it won't move while the spartan is engaging
            /*
            pTargetUnit->setFlagDoingFatality(true);

            if (!pFatality->mAnimateTarget)
            {
               long numTargetHardpoints = pTargetUnit->getNumberHardpoints();
               for (long i = 0; i < numTargetHardpoints; i++)
                  pTargetUnit->clearHardpoint(i);
               pTargetUnit->removeOpps();
               for (long i = pTargetUnit->getNumberActions() - 1; i >= 0; i--)
               {
                  const BAction* pAction = pTargetUnit->getActionByIndexConst(i);
                  if (!pAction->getFlagPersistent() && (pAction->getType() != BAction::cActionTypeUnitUnderAttack) && (pAction->getType() != BAction::cActionTypeUnitRevealToTeam))
                     pTargetUnit->removeActionByID(i);
               }
               pTargetUnit->clearController(BActionController::cControllerOrient);
               pTargetUnit->clearController(BActionController::cControllerAnimation);
            }
            */

         }
         else
         {
            // If no fatality started, but there is a board time/anim -> attach, play anim, and start timer
            if (mpProtoAction->getJoinBoardTime() > 0.0f)
            {
               pTargetUnit->attachObject(pUnit->getID(), -1, -1, false);
               pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateHandAttack, mpProtoAction->getJoinBoardAnimType());//, false, false, -1, true);
               pUnit->computeAnimation();
               mBoardTimerEnd = gWorld->getGametime() + static_cast<DWORD>(mpProtoAction->getJoinBoardTime() * 1000.0f);

               // Remove and disable idles during boarding
               // TODO - Do we still need this?
               setFlagConflictsWithIdle(true);
               pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
            }
            // Otherwise, directly board
            else
               board();
         }

         // Trigger jacking reaction
         if (pUnit->getTeamID() != pTargetUnit->getTeamID())
         {
            gWorld->createPowerAudioReactions(pUnit->getPlayerID(), cSquadSoundChatterReactJacking, pUnit->getPosition(), pUnit->getLOS(), pUnit->getParentID());
         }
      }
      else
      {
         // If no fatality started, but there is a board time/anim -> attach, play anim, and start timer
         if (mpProtoAction->getJoinBoardTime() > 0.0f)
         {
            pTargetUnit->attachObject(pUnit->getID(), -1, -1, false);
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateHandAttack, mpProtoAction->getJoinBoardAnimType());//, false, false, -1, true);
            pUnit->computeAnimation();
            mBoardTimerEnd = gWorld->getGametime() + static_cast<DWORD>(mpProtoAction->getJoinBoardTime() * 1000.0f);

            // Remove and disable idles during boarding
            // TODO - Do we still need this?
            setFlagConflictsWithIdle(true);
            pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
         }
         // Otherwise, directly board
         else
            board();
      }

      if (pParentSquad && pTargetUnit)
      {
         // Set squad and leash position to target position
         pParentSquad->setPosition(pTargetUnit->getPosition());
         pParentSquad->setLeashPosition(pTargetUnit->getPosition());
      }
   }
   else
   {
      BASSERTM(0, "Bad proto action join type");
      return false;
   }

   //==============================================================================
   // Joined successfully, so apply all affects
   //==============================================================================

   // Set join flag on squad we joined so no one else can join it
   pTargetSquad->setMergeType(mergeType);
   pTargetSquad->incrementMergeCount();

   //==============================================================================
   // Fixup selection
   BSelectionManager* pSelMgr = gUserManager.getPrimaryUser()->getSelectionManager();
   if (pSelMgr->isUnitSelected(pUnit->getID()))
   {
      pSelMgr->unselectUnit(pUnit->getID());
      pSelMgr->selectSquad(mTarget.getID());
   }
   if (gGame.isSplitScreen())
   {
      pSelMgr = gUserManager.getSecondaryUser()->getSelectionManager();
      if (pSelMgr->isUnitSelected(pUnit->getID()))
      {
         pSelMgr->unselectUnit(pUnit->getID());
         pSelMgr->selectSquad(mTarget.getID());
      }
   }

   // Apply post join effects.  For boarding join type, wait until the board is complete
   if (joinType != cJoinTypeBoard)
      applyPostJoinEffects();

   // If we're following keep our speed up so we can keep up with the unit we're following
   if (joinType == cJoinTypeFollow || joinType == cJoinTypeFollowAttack)
   {
      float ratio = pTargetSquad->getMaxVelocity() / pUnit->getMaxVelocity();
      pUnit->setVelocityScalar(ratio * 2.0f);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());

   //==============================================================================
   // Apply disconnect effects if successful join was made
   JOIN_TYPE joinType = mpProtoAction->getJoinType();
   if (pUnit && mpProtoAction && mJoinComplete)
   {
      //==============================================================================
      // Take damage on reversion
      float damage = mpProtoAction->getJoinRevertDamagePct() * pUnit->getHPMax();
      if (pUnit->getDamageTakenScalar() > cFloatCompareEpsilon)
         damage /= pUnit->getDamageTakenScalar(); // scale this so unit actually takes 'damage' amount of hitpoints
      if (damage > 0.0f)
      {
         BDamageHelper::doDamageWithWeaponType(-1, -1, pUnit->getID(), NULL, damage, -1, false, cInvalidVector, 1.0f, cInvalidVector, cInvalidObjectID);
      }

      BSquad* pParentSquad = pUnit->getParentSquad();

      //==============================================================================
      // Do join type specific disconnect stuff
      if ((joinType == cJoinTypeFollow || joinType == cJoinTypeFollowAttack) && pUnit && pUnit->isAlive())
      {
         // SLB: Only kill the squad if the target is dead. Otherwise it means we're disconnecting because we're dead and killing ourselves a second time causes problems.
         // If we lost our target, then kill this squad
         if (!pTargetSquad || !pTargetSquad->isAlive())
         {
            // Follow joins kill the unit on disconnect
            pUnit->setFlagInvulnerable(false);
            pUnit->setFlagDiesAtZeroHP(true);
            //pUnit->kill(true);

            if (pParentSquad && pParentSquad->isAlive())
               pParentSquad->kill(true);
         }
      }

      if (joinType == cJoinTypeBoard && pParentSquad)
      {
         if (gConfig.isDefined(cConfigDisplayUnjoinMaxTeleDist))
         {
            gTerrainSimRep.addDebugCircleOverTerrain(pParentSquad->getPosition(), mpProtoAction->getUnjoinMaxTeleportDist(), cDWORDBlue, 1.0f, BDebugPrimitives::cCategoryNone, 10.0f);
         }
         BSimHelper::findPosForAirborneSquad(pParentSquad, mpProtoAction->getUnjoinMaxTeleportDist(), false);
      }

      if (pParentSquad && mpProtoAction->doesJoinOverrideVeterancy())
      {
         pParentSquad->setFlagSpartanVeterancy(false);
         pParentSquad->setFlagContainedSpartan(false);
         pParentSquad->hideSelectionDecal();
      }
   }

   // E3 2008--unhide the HP bar
   if (pUnit && pUnit->getParentSquad())
      pUnit->getParentSquad()->setFlagHasHPBar(true);

   // Unset merge flags and other stuff that was set upon merging/boarding complete
   MERGE_TYPE mergeType = (mpProtoAction ? mpProtoAction->getMergeType() : cMergeTypeGround);
   if (pTargetSquad) // need to clear these flags whether we completed or not. (Spartans can die while jacking)
   {
      // [11-24-08 CJS] UNLESS it's for protectors, as if a protector failed to join, there was already a protector on there
      // and we don't want to clear the flags.
      bool valid = ((joinType != cJoinTypeFollow && joinType != cJoinTypeFollowAttack) || mJoinComplete);
      if (valid)
      {
         pTargetSquad->clearMergeType(mergeType);
         pTargetSquad->decrementMergeCount();
      }

      if (mJoinComplete && mpProtoAction->doesJoinOverrideVeterancy())
      {
         pTargetSquad->setFlagSpartanVeterancy(false);
         pTargetSquad->setFlagSpartanContainer(false);
      }
   }

   //==============================================================================
   // Boarding specific disconnect stuff - unset anything that started on attempting
   // to board
   if (pUnit && joinType == cJoinTypeBoard)
   {
      // Mark the target no longer "NotAttackable"
      if (pTargetSquad)
      {
         BUnit* pTargetUnit = gWorld->getUnit(pTargetSquad->getChild(0));
         if (pTargetUnit)
            pTargetUnit->setFlagNotAttackable(false);

         // MS 9/23/2008: unset "being boarded" flag
         if (pTargetUnit)
            pTargetUnit->setFlagBeingBoarded(false);

         // Remove the boarding entity ref
         if (pTargetUnit)
            pTargetUnit->removeEntityRef(BEntityRef::cTypeBoardingUnit, pUnit->getID());
      }

      // [11-5-08] CJS Reset unit that boarded to being colldiable
      pUnit->setFlagUnhittable(false);

      // Forcibly stop fatality
      gFatalityManager.stopFatality(mpOwner->getID(), false);

      // Set anim back to idle
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, pUnit->getIdleAnim(), true, true);
   }

   // Detach unit if attached
   if (pUnit && pUnit->isAttached())
   {
      BObject* pAttachTo = pUnit->getAttachedToObject();
      if (pAttachTo)
      {
         BUnit* pAttachToUnit = pAttachTo->getUnit();
         if (pAttachToUnit)
            pAttachToUnit->unattachObject(pUnit->getID());
      }
   }

   // Restore tiesToGround flag
   if (pUnit)
   {
      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (pProtoObject)
         pUnit->setFlagTiesToGround(!pProtoObject->getFlagNoTieToGround());
   }

   //Release our controllers.
   releaseControllers();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::init(void)
{
   if (!BAction::init())
      return (false);

   mOppID = BUnitOpp::cInvalidID;
   mTarget.reset();
   mOriginalTarget.reset();
   mOriginalProtoSquadID = cInvalidProtoSquadID;
   mChildOppID = BUnitOpp::cInvalidID;
   mBoardTargetFormerOwnerID = cInvalidPlayerID;
   mAttackTarget.reset();
   mAttackTargetUpdate = 0.0f;
   mBoardStartAnimType = -1;
   mBoardEndAnimType = -1;
   mJoinComplete = false;
   mAllowMultiple = false;
   mTargetJoined = false;
   mBoardTimerEnd = 0;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::setState(BActionState state)
{
   // Set recovery timer for spartan hijack.  It has to be set here rather than in the squad
   // action since the squad action goes away immediately while this action hangs around until
   // the spartan is kicked out.
   if ((state == cStateDone) || (state == cStateFailed))
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      BSquad* pSquad = pUnit->getParentSquad();
      if (pSquad)
      {
         int abilityID = gDatabase.getSquadAbilityID(pSquad, mTarget.getAbilityID());
         const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
         if (pAbility)
            pSquad->setRecover(pAbility->getRecoverType(), pSquad->getPlayer()->getAbilityRecoverTime(abilityID), pAbility->getID());
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::update(float elapsed)
{
   //Validate our target.
   BSquad* pTarget=gWorld->getSquad(mTarget.getID());

   if (!pTarget || !pTarget->isAlive())
   {
   	  // If the current target died and we had switched to it, switch back
      if (mTargetJoined)
      {
         changeTarget(mOriginalTarget);

         mTargetJoined = false;
         return true;
      }
      else
      {
         setState(cStateDone);
         return (true);
      }
   }
   BUnit* pTargetUnit = pTarget->getLeaderUnit();

   JOIN_TYPE joinType = mpProtoAction->getJoinType();

   // Do boarding specific updates
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (joinType == cJoinTypeBoard)
   {
      // Check boarding timer to do the board
      if ((mBoardTimerEnd > 0) && (gWorld->getGametime() > mBoardTimerEnd))
      {
         board();
         mBoardTimerEnd = 0;
      }

      if (pUnit)
      {
         // Set flag so that the motion extracted boarding anims won't check for collisions
         pUnit->setFlagMotionCollisionChecked(true);

         // MPB [9/30/2008] - This sucks to check every update, but there aren't many of these and
         // doing this for every upgrade would be worse.
         // Check to see if the protoAction to use has been changed (via a tech upgrade)
         // If it has changed, reset new buffs
         long abilityID = mTarget.getAbilityID();
         long actionType = mpProtoAction->getActionType();
         BTactic* pTactic = pUnit->getTactic();
         if (pTactic && pTargetUnit)
         {
            BProtoAction *pPA = pTactic->getProtoAction(pUnit->getTacticState(), pTargetUnit, pTargetUnit->getPosition(),
               pUnit->getPlayer(), pUnit->getPosition(), pUnit->getID(), abilityID, false, 
               actionType, false, false, NULL, false, NULL, NULL, true);
            if (pPA && (pPA != getProtoAction()))
            {
               // Unapply old buffs, set protoaction, and apply new buffs
               applyBuffs(getProtoAction(), true);
               setProtoAction(pPA);
               applyBuffs(pPA, false);
            }
         }
      }
   }

   // For merge joins, validate squad mates.  If killed, revert to original squad type
   if ((mOriginalProtoSquadID != cInvalidProtoSquadID) && (pTarget->getNumberChildren() == 1) && (pTarget->getChild(0) == pUnit->getID()))
   {
      pTarget->transform(mOriginalProtoSquadID, false);
      setState(cStateDone);
      return true;
   }

   if (joinType == cJoinTypeFollowAttack || joinType == cJoinTypeFollow)
   {
      // If our target has switched teams, switch teams with them
      if (pTarget->getPlayerID() != mpOwner->getPlayerID())
      {
         // If target already has another unit joined, stop so we can be used again
         if (mpProtoAction && pTarget->hasMergeType(mpProtoAction->getMergeType()))
         {
            setState(cStateDone);
            mpOwner->kill(false);
            return true;
         }

         // Enemy units change owner
         BPlayer* pTargetPlayer = const_cast<BPlayer*>(pTarget->getPlayer());
         BPlayer* pUnitPlayer = const_cast<BPlayer*>(mpOwner->getPlayer());
         if (pUnitPlayer->isEnemy(pTargetPlayer))
         {
            BUnit* pOwner = mpOwner->getUnit();
            if (pOwner)
            {
               BSquad* pSquad = pOwner->getParentSquad();
               if (pSquad)
               {
                  pSquad->changeOwner(pTarget->getPlayerID(), true, pSquad->getID());
               }
            }
         }
      }

      // If our target boarded a vehicle, switch targets
      if (pTarget->getFlagContainedSpartan() && !mTargetJoined)
      {
         if (pTargetUnit)
         {
            uint count = pTargetUnit->getNumberEntityRefs();

            for (uint i = 0; i < count; ++i)
            {
//-- FIXING PREFIX BUG ID 1555
               const BEntityRef* pRef = pTargetUnit->getEntityRefByIndex(i);
//--

               if (!pRef)
                  continue;

               if (pRef->mType == BEntityRef::cTypeContainingUnit)
               {
                  BUnit* pTargetOfTargetUnit = gWorld->getUnit(pRef->mID);

                  if (!pTargetOfTargetUnit)
                     continue;

//-- FIXING PREFIX BUG ID 1554
                  const BSquad* pTargetOfTargetSquad = pTargetOfTargetUnit->getParentSquad();
//--
                  if (pTargetOfTargetSquad)
                  {
                     BSimTarget target;

                     // Save off original target
                     mOriginalTarget = mTarget;

                     target.reset();
                     target.setID(pTargetOfTargetSquad->getID());

                     changeTarget(target);

                     mTargetJoined = true;
                     break;
                  }
               }
            }
         }
      }
   }

   // For follow and follow attack modes, if our target has gone through a teleporter, teleport to their location
   if ((joinType == cJoinTypeFollowAttack || joinType == cJoinTypeFollow) && pUnit && pTarget->getFlagTeleported())
   {
      BSquad* pParentSquad = pUnit->getParentSquad();

      if (pParentSquad)
      {
         BVector position = pTarget->getAveragePosition();

         if (pUnit->getFlagFlying())
         {
            position.y += pUnit->getProtoObject()->getFlightLevel();

            // Also, offset us a bit so we're not right on top of them
            float dist = pParentSquad->getObstructionRadius() + pUnit->getObstructionRadius();
            position.x += dist * 2.0f;
         }

         pParentSquad->stop();
         pParentSquad->stopAllChildren(0);
         BPlatoon* pPlatoon = pParentSquad->getParentPlatoon();
         if (pPlatoon)
            pPlatoon->setPosition(position);
         pParentSquad->setPosition(position);
         pParentSquad->setLeashPosition(position);
         pParentSquad->noStopSettle();
         pParentSquad->updateObstruction();
         pParentSquad->dirtyLOS();
         pParentSquad->setFlagTeleported(true);
         pUnit->setPosition(position);

         //pParentSquad->doTeleport(position, 1, true, false);
         #ifdef SYNC_Unit
            syncUnitData("BUnitActionJoin::update", position);
         #endif
      }
   }

   // For follow attack joins, set the attack target so ranged attacks will work
   mAttackTargetUpdate += elapsed;

   if (mAttackTargetUpdate > 0.5f)
   {
      mAttackTargetUpdate = 0.0f;

      if (joinType == cJoinTypeFollowAttack)
      {
         BUnit* pUnit = static_cast<BUnit*>(mpOwner);
         BASSERT(pUnit);
         BSquad* pSquad = pUnit->getParentSquad();

         if (pTarget && pTarget->getLeaderUnit())
         {
            // No valid attack opp, check for an attack action on the parent
//-- FIXING PREFIX BUG ID 1556
            const BSquadActionAttack* pAttack = reinterpret_cast<const BSquadActionAttack*>(pTarget->getActionByTypeConst(BAction::cActionTypeSquadAttack));
//--
            DWORD dummy = 0;
            if (pAttack && pAttack->getTarget()->isValid() && pAttack->validateRange(dummy))
            {
               if (mChildOppID != BUnitOpp::cInvalidID && mAttackTarget != *pAttack->getTarget())
               {
                  pSquad->removeOppFromChildren(mChildOppID);
                  mChildOppID = BUnitOpp::cInvalidID;
               }

               if (mChildOppID == BUnitOpp::cInvalidID)
               {
                  mAttackTarget = *pAttack->getTarget();

                  // Add an attack opportunity for ourselves
                  BUnitOpp opp;
                  opp.init();
                  opp.setTarget(mAttackTarget);
                  opp.setType(BUnitOpp::cTypeAttack);
                  opp.setSource(pSquad->getID());
                  opp.setPriority(BUnitOpp::cPrioritySquad);
                  opp.generateID();
                  mChildOppID = opp.getID();
                  pSquad->addOppToChildren(opp);

                  //pSquad->setNormal();

                  return (true);
               }
            }
            else if (!pAttack && mChildOppID != BUnitOpp::cInvalidID)
            {
               pSquad->removeOppFromChildren(mChildOppID);
               mChildOppID = BUnitOpp::cInvalidID;

               BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
               if (pTargetSquad)
               {
                  BUnit *pTargetLeaderUnit = pTargetSquad->getLeaderUnit();
                  if (pTargetLeaderUnit)
                  {
                     BTactic* pTactic = pTargetLeaderUnit->getTactic();
                     if (pTactic && pTactic->hasEnabledAttackAction(false))
                        pSquad->setPassive();
                  }
               }
            }
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (!validateTag(eventType, senderID, data1, data2))
      return;

   switch (eventType)
   {
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimChain:
      {
         // Trigger boarding at the end of the boardEnd anim
         if ((senderID == mpOwner->getID()) &&
            (data2 == cActionAnimationTrack))
         {
            long animType = (long) data1;

            // MPB [7/10/2008] - Wait until boarding is done to unset the fatality flag
            // There are still problems with movement offseting the spartan anims
            // If board start is done, unset the fatality flag so the target can move again
            /*
            if (animType == mBoardStartAnimType)
            {
               BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
               if (!pTargetSquad)
                  return;
               BUnit* pTargetUnit = gWorld->getUnit(pTargetSquad->getChild(0));
               if (pTargetUnit)
                  pTargetUnit->setFlagDoingFatality(false);
            }
            else
            */
            // If the board end is done, time to board
            if (animType == mBoardEndAnimType)
               board();
         }
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::setOppID(BUnitOppID oppID)
{
   // TRB 11/20/08 - Don't update the controller's opp ID if the current opp ID is invalid
   // since this will hijack an ungrabbed controller, keeping others from grabbing it.
   if (mpOwner && (mOppID != BUnitOpp::cInvalidID))
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeSquad));
   if (target.isValid())
   {
      mTarget = target;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::applyPostJoinEffects()
{
   // Default buff apply (using current protoaction)
   applyBuffs(mpProtoAction, false);

   changeTarget(mTarget);

   // Set join complete
   mJoinComplete = true;

   // Invalidated jacked entity for trigger system
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (pUnit && (mBoardTargetFormerOwnerID != cInvalidPlayerID) && (pUnit->getPlayerID() != mBoardTargetFormerOwnerID))
   {
      gTriggerManager.invalidateEntityID(mTarget.getID());
   }
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::applyAIMissionBookkeeping(BSquad& rTargetSquad, BSquad& rHijackerSquad)
{
   // Remove the hijacked unit from the mission he is in.
   rTargetSquad.removeFromAIMission();

   // If the hijacker (spartan) was in a mission, add the hijacked unit to that mission, and ungroup the spartan.
   // The spartan will be unable to re-group until he is no longer hijacking the unit.
   BAIMission* pMission = gWorld->getAIMission(rHijackerSquad.getAIMissionID());
   if (pMission)
   {
      pMission->addSquad(rTargetSquad.getID());
      pMission->ungroupSquad(rHijackerSquad.getID());
   }
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::applyBuffs(const BProtoAction* pPA, bool unapply)
{
//-- FIXING PREFIX BUG ID 1559
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
//-- FIXING PREFIX BUG ID 1560
   const BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
//--
   if (!pTargetSquad)
      return;
   if (!pPA)
      return;

   //==============================================================================
   // Apply buffs to target squad

   // Determine modifiers
   float damageModifier = 1.0f;
   float damageTakenModifier = 1.0f;
   if (pPA->isDamageBuffByCombatValue() && pUnit->getProtoObject())
   {
      // Calculate modifiers based on relative combat value
      float joinCV = pUnit->getProtoObject()->getCombatValue();
      float targetCV = pTargetSquad->getCombatValue();

      if (joinCV > 0.0f && targetCV > 0.0f)
      {
         float cvRatio = joinCV / targetCV;
         damageModifier = 1.0f + cvRatio * mpProtoAction->getDamageBuffFactor();
         if (mpProtoAction->getDamageTakenBuffFactor() > 0.0f)
            damageTakenModifier = 1.0f / (1.0f + (cvRatio / mpProtoAction->getDamageTakenBuffFactor()));
      }
   }
   else
   {
      // Standard modifiers
      damageModifier = mpProtoAction->getDamageBuffFactor();
      damageTakenModifier = mpProtoAction->getDamageTakenBuffFactor();
   }

   // If we're unapplying the buffs, use the reciprocal
   if (unapply)
   {
      if (damageModifier != 0.0f)
         damageModifier = 1.0f / damageModifier;
      else
         damageModifier = 0.0f;
      if (damageTakenModifier != 0.0f)
         damageTakenModifier = 1.0f / damageTakenModifier;
      else
         damageTakenModifier = 0.0f;
   }

   // Apply modifiers to squadmates
   for (uint i = 0; i < pTargetSquad->getNumberChildren(); i++)
   {
      BUnit* pSquadmate = gWorld->getUnit(pTargetSquad->getChild(i));
      if (pSquadmate && (pSquadmate != pUnit))
      {
         pSquadmate->adjustDamageModifier(damageModifier);
         pSquadmate->adjustDamageTakenScalar(damageTakenModifier);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::board()
{
   if (mJoinComplete)
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BSquad* pSquad = pUnit->getParentSquad();
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
   if (!pTargetSquad)
      return;
   BUnit* pTargetUnit = gWorld->getUnit(pTargetSquad->getChild(0));
   if (!pTargetUnit)
      return;

   // Enemy units change owner
   BPlayer* pTargetPlayer = const_cast<BPlayer*>(pTargetSquad->getPlayer());
   BPlayer* pUnitPlayer = const_cast<BPlayer*>(pUnit->getPlayer());
   if (pUnitPlayer->isEnemy(pTargetPlayer))
      pTargetSquad->changeOwner(pUnitPlayer->getID(), true, pUnit->getID());

   // Garrison unit inside target
   if (pTargetUnit)
   {
      // [10/2/2008 xemu] send event to trigger system
      gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityJacked, pTargetUnit->getPlayerID(), pTargetUnit->getID(), pUnit->getID()); 

      // Garrison
      pTargetUnit->unattachObject(pUnit->getID());
      pTargetUnit->containUnit(pUnit->getID());
      pTargetUnit->setFlagIsTypeGarrison(true); // set garrison type flag to true so that contained units get updated
      pUnit->setFlagTiesToGround(true);
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, pUnit->getIdleAnim(), true, true); // Set anim back to idle

      // Apply veterancy
      if (mpProtoAction->doesJoinOverrideVeterancy())
      {
         // Apply current spartan vet upgrades to target
         pTargetUnit->upgradeLevel(0, pSquad->getLevel(), true, pUnit->getProtoObject());

         // Set flags so future XP is split between boarded vehicle and spartan and spartan
         // upgrades are applied to container units
         pTargetSquad->setFlagSpartanVeterancy(true);
         pTargetSquad->setFlagSpartanContainer(true);
         pSquad->setFlagSpartanVeterancy(true);
         pSquad->setFlagContainedSpartan(true);
      }

      // Add hijacked effect - the lifespan on the protoObject will take care
      // of deleting it.
      if (mpProtoAction)
      {
         BProtoObjectID hijackedProtoID = mpProtoAction->getProtoObject();
         if (hijackedProtoID != cInvalidProtoObjectID)
         {
            pTargetUnit->addAttachment(hijackedProtoID);
         }
      }

      // Mark the target no longer "NotAttackable"
      pTargetUnit->setFlagNotAttackable(false);

      // MS 9/23/2008: unset "being boarded" flag
      pTargetUnit->setFlagBeingBoarded(false);

      // MPB [7/10/2008] - Unset the fatality once boarding is complete so the unit can move again
      pTargetUnit->setFlagDoingFatality(false);

      // CJS [11-4-08] Setting unit to be not hittable so it's not hit while inside
      pUnit->setFlagUnhittable(true);

      // Remove the boarding entity ref
      pTargetUnit->removeEntityRef(BEntityRef::cTypeBoardingUnit, pUnit->getID());

      // For open-cockpit vehicles, swap the default driver with the spartan driver mesh
      BVisual* pTargetVisual = pTargetUnit->getVisual();
      if (pTargetVisual)
      {
         BGrannyInstance* pGI = pTargetVisual->getGrannyInstance();
         if (pGI)
         {
            BBitArray tempMask;
            bool swapped = swapDriverMeshRendering(pGI, tempMask);
            if (swapped)
               pGI->setMeshRenderMaskResetCallback(swapDriverMeshRendering);
         }
      }
   }

   // Handle bookkeeping for AI's
   applyAIMissionBookkeeping(*pTargetSquad, *pSquad);

   // Apply buffs
   applyPostJoinEffects();

   // Can idle now
   setFlagConflictsWithIdle(false);

   // Done animating, so release controllers
   releaseControllers();

   // Trigger commandeer reaction
   if (pUnit->getTeamID() == pTargetUnit->getTeamID())
   {
      gWorld->createPowerAudioReactions(pUnit->getPlayerID(), cSquadSoundChatterReactCommandeer, pUnit->getPosition(), pUnit->getLOS(), pUnit->getParentID());
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 1563
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   bool valid = (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID);
   if (!valid)
      return (false);

   valid = (pUnit->getController(BActionController::cControllerAnimation)->getActionID() == mID);

   return (valid);
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the animation controller
   grabbed = pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionJoin::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return;

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);   

   // Release the animation controller
   pUnit->releaseController(BActionController::cControllerAnimation, this);
}


//==============================================================================
//==============================================================================
void BUnitActionJoin::changeTarget(const BSimTarget& newTarget)
{
   BASSERT(mpOwner);

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   setTarget(newTarget);

   //==============================================================================
   // If we have a heal action, set its target to this squad
   BUnitActionHeal* pHeal = static_cast<BUnitActionHeal*>(pUnit->getActionByType(BAction::cActionTypeUnitHeal));
   if (pHeal)
      pHeal->setTarget(newTarget);

   //==============================================================================
   // If we have a shield action, set its target to this squad
   BUnitActionBubbleShield* pShield = static_cast<BUnitActionBubbleShield*>(pUnit->getActionByType(BAction::cActionTypeUnitBubbleShield));
   if (pShield)
      pShield->setTarget(newTarget);
}


//==============================================================================
//==============================================================================
bool BUnitActionJoin::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITECLASS(pStream, saveType, mAttackTarget);
   GFWRITECLASS(pStream, saveType, mOriginalTarget);
   GFWRITEVAR(pStream, BProtoSquadID, mOriginalProtoSquadID);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BUnitOppID, mChildOppID);
   GFWRITEVAR(pStream, float, mAttackTargetUpdate);
   GFWRITEVAR(pStream, long, mBoardStartAnimType);
   GFWRITEVAR(pStream, long, mBoardEndAnimType);
   GFWRITEVAR(pStream, DWORD, mBoardTimerEnd);
   GFWRITEBITBOOL(pStream, mJoinComplete);
   GFWRITEBITBOOL(pStream, mAllowMultiple);
   GFWRITEBITBOOL(pStream, mTargetJoined);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADCLASS(pStream, saveType, mAttackTarget);
   GFREADCLASS(pStream, saveType, mOriginalTarget);
   GFREADVAR(pStream, BProtoSquadID, mOriginalProtoSquadID);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BUnitOppID, mChildOppID);
   GFREADVAR(pStream, float, mAttackTargetUpdate);
   GFREADVAR(pStream, long, mBoardStartAnimType);
   GFREADVAR(pStream, long, mBoardEndAnimType);
   GFREADVAR(pStream, DWORD, mBoardTimerEnd);
   GFREADBITBOOL(pStream, mJoinComplete);
   GFREADBITBOOL(pStream, mAllowMultiple);
   GFREADBITBOOL(pStream, mTargetJoined);

   gSaveGame.remapAnimType(mBoardStartAnimType);
   gSaveGame.remapAnimType(mBoardEndAnimType);

   // If boarding is complete, add the mesh render reset callback that
   // ensures the spartan is swapped for the default driver
   if (mpProtoAction)
   {
      JOIN_TYPE joinType = mpProtoAction->getJoinType();
      if (joinType == cJoinTypeBoard && mJoinComplete)
      {
         BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
         if (pTargetSquad)
         {
            BUnit* pTargetUnit = gWorld->getUnit(pTargetSquad->getChild(0));
            if (pTargetUnit)
            {
               BVisual* pVis = pTargetUnit->getVisual();
               if (pVis)
               {
                  BGrannyInstance* pGI = pVis->getGrannyInstance();
                  if (pGI)
                     pGI->setMeshRenderMaskResetCallback(swapDriverMeshRendering);
               }
            }
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionJoin::swapDriverMeshRendering(BGrannyInstance* pGrannyInstance, const BBitArray& previousMask)
{
   if (!pGrannyInstance)
      return false;

   BGrannyModel* pGrannyModel = gGrannyManager.getModel(pGrannyInstance->getModelIndex(), true);
   if (pGrannyModel)
   {
      long spartanDriverIndex = pGrannyModel->getMeshIndex(BSimString("SpartanDriver"));
      long defaultDriverIndex = pGrannyModel->getMeshIndex(BSimString("Driver"));
      if (spartanDriverIndex >= 0 && defaultDriverIndex >= 0)
      {
         pGrannyInstance->setMeshVisible(spartanDriverIndex, true);
         pGrannyInstance->setMeshVisible(defaultDriverIndex, false);
         return true;
      }
   }

   return false;
}
