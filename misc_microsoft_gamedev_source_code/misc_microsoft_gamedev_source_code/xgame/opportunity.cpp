//==============================================================================
// opportunity.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "opportunity.h"
#include "entity.h"
#include "SimOrder.h"
#include "squad.h"
#include "squadactionattack.h"
#include "squadactionmove.h"
#include "world.h"
#include "tactic.h"
#include "protosquad.h"
#include "unitquery.h"
#include "database.h"
#include "unitactiongather.h"
#include "unitactionrangedattack.h"
#include "unitactionrevive.h"
#include "config.h"
#include "configsgame.h"
#include "squadlosvalidator.h"
#include "team.h"


//-- Instantiate Entity Behaviors
BDefaultSquadBehavior* gDefaultSquadBehavior = new BDefaultSquadBehavior();

BDynamicSimArray<BOpportunityResult>   BDefaultSquadBehavior::mTempOpportunityResults;
BDynamicSimUIntArray                   BDefaultSquadBehavior::mTempOpportunityOrder;


//==============================================================================
//==============================================================================
XMFINLINE bool BOpportunityResult::operator<(const BOpportunityResult &oppResultToCompare) const
{
   // Checks whether the opp result passed in has greater priority than this one
   // Compare criteria based on this order
   //    1 - priority
   //    2 - current attack target
   //    3 - distance

   // Priority
   if (mPriority < oppResultToCompare.mPriority)
      return true;
   else if (mPriority > oppResultToCompare.mPriority)
      return false;

   // Current attack target
   if (!mCurrentTarget && oppResultToCompare.mCurrentTarget)
      return true;
   else if (mCurrentTarget && !oppResultToCompare.mCurrentTarget)
      return false;

   // Distance
   if (mDistance > oppResultToCompare.mDistance)
      return true;

    return false;
}


//==============================================================================
//==============================================================================
BOpportunityList::BOpportunityList() :
   mEntityBehavior(NULL)
{
}

//==============================================================================
//==============================================================================
BOpportunityList::~BOpportunityList()
{
}

//==============================================================================
//==============================================================================
void BOpportunityList::setBehavior(BEntity* pOwner)
{
   //-- Setup the proper behavior. We can reevaluate this later. 
   //-- The general idea is we can dynamically change our behavior by plugging in a different behavior class.
   //-- Right now everything will use it's default types behavior.
   if(pOwner->getClassType() == BEntity::cClassTypeSquad)
   {
      //-- For now assume default squad behavior
      mEntityBehavior = (gDefaultSquadBehavior);
   }
}

//==============================================================================
//==============================================================================
void BOpportunityList::addOpportunity(BEntityID targetID, BVector targetPosition,
   uint8 opportunityType, BProtoAction* pAction, float movementDistance)
{
   //This only accepts squad targets.
   BDEBUG_ASSERT(targetID.getType() == BEntity::cClassTypeSquad);

   BOpportunity& opportunity = mOpportunities.grow();
   opportunity.mTargetPosition = targetPosition;
   opportunity.mTargetID = targetID;
   opportunity.mType = opportunityType;
   opportunity.mpProtoAction = pAction;
   opportunity.mMovementDistance = movementDistance < 0 ? 0 : movementDistance;
}

//==============================================================================
//==============================================================================
void BOpportunityList::clearOpportunities()
{
   // Don't free the memory.  Just set the array size to 0.
   mOpportunities.resize(0);
   mTopOpportunityIndex = -1;
}

//==============================================================================
//==============================================================================
void BOpportunityList::setPriority(uint index, float priority)
{
   if(index >= mOpportunities.getSize())
   {
      BDEBUG_ASSERT(0);
      return;
   }

   mOpportunities.get(index).mPriority = priority;
}

//==============================================================================
//==============================================================================
bool BOpportunityList::getTopOpportunity(BOpportunity& opportunity)
{
   if(mTopOpportunityIndex >= 0 && mTopOpportunityIndex < mOpportunities.getNumber())
   {
      opportunity = mOpportunities.get(mTopOpportunityIndex);
      return true;
   }
   else
      return false;
}

//==============================================================================
//==============================================================================
void BOpportunityList::findOpportunities(BEntity* pEntity, bool isStrafing)
{
   if(mEntityBehavior)
      mEntityBehavior->findOpportunities(this, pEntity, isStrafing);
}

//==============================================================================
//==============================================================================
void BOpportunityList::calculatePriorities(BEntity* pEntity)
{
   if(mEntityBehavior)
      mEntityBehavior->calculatePriorities(this, pEntity);
}


//==============================================================================
//==============================================================================
void BDefaultSquadBehavior::findOpportunities(BOpportunityList* pList, BEntity* pEntity, bool isStrafing)
{
   if(!pList)
      return;

   //-- Blow away our old opportunities
   pList->clearOpportunities();

   //-- The entity must be a squad
   BDEBUG_ASSERT(pEntity->getClassType() == BEntity::cClassTypeSquad);
   BSquad *pSquad = pEntity->getSquad();

   //-- get my player
   long playerID=pSquad->getPlayerID();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   // Get a unit from our squad to use for tactics stuff
   BUnit* pSampleUnit = pSquad->getLeaderUnit();
   if (!pSampleUnit)
      return;

   //Do the can attack check as early as possible.
   BTactic* pTactic = pSampleUnit->getTactic();
   if (!pTactic)
      return;
   if (!pTactic->canAttack() && !pTactic->canGather() && !pTactic->canAutoRepair())
      return;

    // Unit visibility isn't setup until after the first update
   if (pSquad->getFlagFirstUpdate())
      return;
   // Can't do anything if being transported
   if (pSquad->getFlagPassiveGarrisoned())
      return;
   // Can't do anything if garrisoning, etc.
   if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || (pSquad->isGarrisoned() && !pSquad->isInCover()) || pSquad->isAttached())
      return;   
   // Can't do anything if hitched or in the process of hitching or unhitching
   if (pSquad->isHitched() || pSquad->getFlagIsHitching() || pSquad->getFlagIsHitching())
      return;
   // Squads that are changing modes can't do anything else
   if (pSquad->getFlagChangingMode())
      return;

   //Can't do anything while being built.
   if (!pSampleUnit->getFlagBuilt())
      return;

   //Can't do anything if outside of playable bounds
   if (pSquad->isOutsidePlayableBounds())
      return;

   // Attack blocked, we can't do anything
   if (pSquad->getFlagAttackBlocked())
      return;

   //Look for attack opportunities if not recovering
   if (pTactic->canAttack() &&
      (pSquad->getRecoverType() != cRecoverAttack) &&
      (pSquad->getSquadMode() != BSquadAI::cModePassive))
      findAttackOpportunities(pList, pSquad, pSampleUnit, pTactic, pPlayer, isStrafing);

   //Look for gather opportunities if last command received was "Gather"
   if ((pList->getNumOpportunities() == 0) && pTactic->canGather() && pSquad->getLastCommandType() == BSimOrder::cTypeGather)
      findGatherOpportunities(pList, pSquad, pSampleUnit, pTactic, pPlayer);

   //Auto repair.
   if ((pList->getNumOpportunities() == 0) && pTactic->canAutoRepair())
      findAutoRepairOpportunities(pList, pSquad, pSampleUnit, pTactic, pPlayer);
}

//==============================================================================
//==============================================================================
void BDefaultSquadBehavior::findAttackOpportunities(BOpportunityList* pList, BSquad* pSquad,
   BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer, bool isStrafing)
{
   //Are we passive?
   if (pSquad->getProtoSquad() && (pSquad->getProtoSquad()->getStance() == BProtoSquad::cStancePassive))
      return;
   // Cloaked units should not attack either
   // [10-2-08 CJS] Cloaked units should auto-attack (PHX-13014)
   /*else if (!pStrikeAction && pSquad->getFlagCloaked() && !pSquad->getFlagCloakDetected())
      return;*/

   //Make sure that our attacking won't disrupt the current command and figure out
   //what type of attack we can do.
   uint8 attackType=BOpportunity::eOpportunityTypeAttack;
   const BSimOrderEntry* pOrderEntry=pSquad->getOrderEntry(true, true);
   if (pOrderEntry)
   {
      // MSC: I talked to Stark and he said this check was supposed to stop squads for looking for attacks if they have a join order, so I'm moving the check out.
      if (pOrderEntry->getType() == BSimOrder::cTypeJoin)
         return;
      
      // Don't target stuff if we're executing a non-move/non-attack command and we reached our destination
      if ((pOrderEntry->getType() != BSimOrder::cTypeAttack) && (pOrderEntry->getType() != BSimOrder::cTypeMove) && !pSquad->isMoving())
         return;
      //If we have a sim order, that can be overridden.  Else, if we don't have move attack enabled
      //on this order, we can only do a secondary attack.
      if (pOrderEntry->getOrder()->getPriority() != BSimOrder::cPrioritySim)
      {
         if (pOrderEntry->getOrder()->getAttackMove())
            attackType=BOpportunity::eOpportunityTypeAttackMove;
         else
            attackType=BOpportunity::eOpportunityTypeAttackSecondary;
      }
   }
   
   //If we're strafing, we can't do a primary attack.
   //DCP: We could maybe pinch off more attacks, but I am trying to preserve functionality here.
   //DCP/MC 11/18/08: Turning this off to fix retargeting.
   //if (isStrafing && (attackType == BOpportunity::eOpportunityTypeAttack))
   //   return;

   // Don't generate secondary opps if can't attack and move at the same time
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if ((attackType == BOpportunity::eOpportunityTypeAttackSecondary) && pProtoSquad && !pProtoSquad->getFlagCanAttackWhileMoving())
      return;

   //Get the ranges.
   // mrh 3/13/08 - Leash distance no longer vary with weapons.
   float leashDistance=pSquad->getLeashDistance();
   float aggroDistance=pSquad->getAggroDistance();

   //Get the enemy list.
   const BEntityIDArray& results=pSquad->getVisibleEnemySquads();

   //Add opportunities to attack other squads.
   for (uint i=0; i < results.getSize(); i++)
   {
      BSquad* pTargetSquad=gWorld->getSquad(results[i]);
      if (!pTargetSquad)
         continue;
      
      if (pTargetSquad->getFlagCloaked() && !pTargetSquad->getFlagCloakDetected())
         continue;

      BUnit* pTargetUnit=pTargetSquad->getLeaderUnit();
      if (!pTargetUnit)
         continue;

      //Don't Auto Attack Me.
      if (pTargetUnit->getFlagDontAutoAttackMe())
         continue;
      // don't target downed heroes.
      if (pTargetUnit->getFlagDown())
         continue;

      //Get an attack action.
      BProtoAction *pProtoAction=pTactic->getProtoAction(pSampleUnit->getTacticState(), pTargetUnit,
         pTargetUnit->getPosition(), pPlayer, pSquad->getPosition(), pSampleUnit->getID(), -1, true,
         BAction::cActionTypeUnitRangedAttack);      
      if (!pProtoAction || (pProtoAction->getActionType() == BAction::cActionTypeInvalid))
         continue;      
      BDEBUG_ASSERTM(pProtoAction->getActionType() == BAction::cActionTypeUnitRangedAttack, "A squad is defined as 'can attack', but has no attack action!");

      // Melee attacks can't be used against units in cover
      if (pProtoAction->isMeleeAttack() && (pTargetSquad->isInCover() || pTargetUnit->isInCover()))
         continue;

      //Get the distance to the target.
      float targetDist=pSquad->calculateXZDistance(pTargetSquad);
      // mrh 3/18/08 - Min range is now defined as edge of attacker to center point of target.  Ummm... ok.
      float targetMinRangeDist=pSquad->calculateXZDistance(pTargetSquad->getPosition());

      //Validate min range.
      if ((pProtoAction->getMinRange() > cFloatCompareEpsilon) && (targetMinRangeDist < pProtoAction->getMinRange()))
         continue;

      //See if this potential target is attacking an ally.
      bool targetIsAttackingAlly=false;
      //-- FIXING PREFIX BUG ID 2444
      const BAction* pTargetAttackAction=pTargetSquad->getActionByType(BAction::cActionTypeSquadAttack);
      //--
      if (pTargetAttackAction)
      {
         //-- FIXING PREFIX BUG ID 2442
         const BEntity* pTargetTarget=gWorld->getEntity(pTargetAttackAction->getTarget()->getID());
         //--
         if (pTargetTarget && pSquad->getPlayer()->isAlly(pTargetTarget->getPlayerID()))
            targetIsAttackingAlly=true;
      }

      //Check max range.
      float weaponRange=pProtoAction->getMaxRange(pSampleUnit);

      float requiredMovementDistance=0.0f;
      // If this is a weapon that uses group range, the initial range check is overly broad since the attack space is not circular
      // Check to see that it is in a legally targetable location before proceeding.
      BWeapon* pWeapon = NULL;
      BUnit* pUnit = pSquad->getLeaderUnit();
      if (pUnit)
      {
         BTactic* pTactic = pUnit->getTactic();
         if (pTactic)
            pWeapon = (BWeapon*)pTactic->getWeapon(pProtoAction->getWeaponID());
      }

      if (pWeapon && pUnit && pWeapon->getFlagUseGroupRange() && (pUnit->getGroupDiagSpan() > 0.0f))
      {
         BVector tgtPos = pTargetUnit->getPosition();
         BOPQuadHull* hull = pUnit->getGroupHull();
         if (hull && pTargetUnit)
         {
            float fDist = hull->distance(tgtPos);

            fDist -= pTargetUnit->getObstructionRadius();
            if (fDist > pProtoAction->getMaxRange(pUnit, false))
               continue;
         }
      }
      else if (targetDist > weaponRange)
      {
         requiredMovementDistance=targetDist-weaponRange;
         //If we can't move at all for this, skip.
         bool bCanAutoUnlock = pSampleUnit->canAutoUnlock();
         if (!pSquad->getSquadAI()->canMoveToAttack(bCanAutoUnlock) ||
            (attackType == BOpportunity::eOpportunityTypeAttackSecondary))
            continue;
         //See if we have to move too much for this target.  The distance
         //we check against is the aggro distance if the target isn't attacking
         //an ally or the leash distance if it is.
         float targetDistanceFromLeash=pSquad->calculateXZDistance(pSquad->getLeashPosition(), pTargetSquad);
         float requiredMovementDistanceFromAnchor=targetDistanceFromLeash-weaponRange;
         if (requiredMovementDistanceFromAnchor > leashDistance)
            continue;
         if (!targetIsAttackingAlly)
         {
            if ((requiredMovementDistanceFromAnchor > aggroDistance) && (targetDist > aggroDistance))
               continue;
         }
      }

      // Are we ignoring this target squad for a period of time (due to the ping-pong leash issue.)
      BEntityID targetSquadID = pTargetSquad->getID();
      if (pSquad->canMove() && pSquad->getSquadAI()->isSquadIgnored(targetSquadID))
         continue;

      //Add the opp to the list.
      pList->addOpportunity(targetSquadID, cInvalidVector, attackType, pProtoAction, requiredMovementDistance);
   }
}


//==============================================================================
//==============================================================================
void BDefaultSquadBehavior::findGatherOpportunities(BOpportunityList* pList, BSquad* pSquad, BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer)
{
//-- FIXING PREFIX BUG ID 2447
   const BSquadActionMove* pSquadMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
//--
//-- FIXING PREFIX BUG ID 2448
   const BUnitActionGather* pUnitGatherAction = reinterpret_cast<BUnitActionGather*>(pSampleUnit->getActionByType(BAction::cActionTypeUnitGather));
//--

   if (!pSquadMoveAction && !pUnitGatherAction)
   {
      // Restoring hard-coded search range so results are consistent compared to before search optimizations
      const float searchRange = 55.0f;

      //-- Query for nearby units
      const BEntityIDArray& results = pSquad->getVisibleSquads();

      if (results.getNumber() <= 0)
         return;

      for (uint i=0; i < results.getSize(); i++)
      {
         BSquad* pTargetSquad=gWorld->getSquad(results[i]);
         if (!pTargetSquad)
            continue;

         const BUnit* pTargetUnit=pTargetSquad->getLeaderUnit();
         if (!pTargetUnit || !pTargetUnit->isType(gDatabase.getOTIDGatherable()))
            continue;

         float distToTarget = pTargetSquad->calculateXZDistance(pSquad);
         if (distToTarget > searchRange)
            continue;

         BProtoAction *pProtoAction=pTactic->getProtoAction(pSampleUnit->getTacticState(), pTargetUnit,
            pTargetUnit->getPosition(), pPlayer, pSquad->getPosition(), pSampleUnit->getID(), -1, true,
            BAction::cActionTypeUnitGather);
         if (!pProtoAction)
            continue;
         BDEBUG_ASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitGather);

         //Add the opp to the list.
         BEntityID targetSquadID = pTargetSquad->getID();
         float requiredMovementDistance = pSquad->calculateXZDistance(pTargetSquad);
         pList->addOpportunity(targetSquadID, pTargetSquad->getPosition(), BOpportunity::eOpportunityTypeGather, pProtoAction, requiredMovementDistance);
      }
   }
}

//==============================================================================
//==============================================================================
void BDefaultSquadBehavior::findAutoRepairOpportunities(BOpportunityList* pList, BSquad* pSquad, BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer)
{
//-- FIXING PREFIX BUG ID 2450
   const BSquadActionMove* pSquadMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
//--

   if (!pSquadMoveAction)
   {
      BProtoAction *pHealProtoAction = pTactic->getProtoAction(pTactic->getProtoActionID("RepairOther"));
      if (pHealProtoAction == NULL)
      {
         BASSERT(false);
         return;
      }

      // Make sure action enabled
      if (pHealProtoAction->getFlagDisabled())
         return;

      DWORD idleDur = pSquad->getIdleDuration();
      if (idleDur < pHealProtoAction->getAutoRepairIdleTime())
         return;

      float searchRange = pHealProtoAction->getAutoRepairSearchDistance();

      //-- Query for nearby allied units
      const BTeam* pTeam = pPlayer->getTeam();
      BASSERT(pTeam);
      const BEntityIDArray& results = pSquad->getVisibleSquads();

      if (results.getNumber() <= 0)
         return;

      for (uint i=0; i < results.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 2449
         const BSquad* pTargetSquad=gWorld->getSquad(results[i]);
//--
         if (!pTargetSquad)
            continue;

         // [4/18/2008 xemu] don't auto-heal yourself
         if (pTargetSquad->getID() == pSquad->getID())
            continue;

         if (!pTeam->isOnTeam(pTargetSquad->getPlayerID()))
            continue;

         if (pTargetSquad->getHPPercentage() > pHealProtoAction->getAutoRepairThreshold())
            continue;

         float distToTarget = pTargetSquad->calculateXZDistance(pSquad);
         if (distToTarget > searchRange)
            continue;

         //BUnit* pTargetUnit=pTargetSquad->getLeaderUnit();
         //BDEBUG_ASSERT(pTargetUnit);

         // Confirm it follows the target rules
         BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
         if (!pTargetUnit)
            continue;
         BProtoAction *pAction = pTactic->getProtoAction(pSampleUnit->getTacticState(), pTargetUnit,
            pTargetUnit->getPosition(), pPlayer, pSquad->getPosition(), pSampleUnit->getID(), -1, true,
            BAction::cActionTypeSquadRepairOther);
         if (!pAction)
            continue;

         //Add the opp to the list.
         BEntityID targetSquadID = pTargetSquad->getID();
         float requiredMovementDistance = pSquad->calculateXZDistance(pTargetSquad);
         pList->addOpportunity(targetSquadID, pTargetSquad->getPosition(), BOpportunity::eOpportunityTypeAutoRepair, pHealProtoAction, requiredMovementDistance);
      }
   }
}

//==============================================================================
BProtoAction* BDefaultSquadBehavior::getBeamProtoAction(BEntity* pEntity)
{
   BDEBUG_ASSERT(pEntity->getClassType() == BEntity::cClassTypeSquad);
   BSquad *pSquad = pEntity->getSquad();
   if (!pSquad)
      return false;

   BUnit *pMyUnit = pSquad->getLeaderUnit();
   if (!pMyUnit)
      return false;

   BTactic *pTactic = pMyUnit->getTactic();
   if (!pTactic)
      return false;

   int count = pTactic->getNumberProtoActions();
   for (int idx=0; idx<count; idx++)
   {
      if (pTactic->getProtoAction(idx) && pTactic->getProtoAction(idx)->isBeam())
         return pTactic->getProtoAction(idx);
   }
   return NULL;
}

//==============================================================================
bool BDefaultSquadBehavior::getLaunchInfo(BEntity* pEntity, BProtoAction* pProtoAction, BVector& position, BVector& forward)
{
   BDEBUG_ASSERT(pEntity->getClassType() == BEntity::cClassTypeUnit);
   BUnit *pMyUnit = pEntity->getUnit();
   if (!pMyUnit || pProtoAction->getHardpointID() < 0)
      return false;

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (pMyUnit->getHardpointYawLocation(pProtoAction->getHardpointID(), msBonePos, msBoneMatrix, &transformedBoneMatrix))
   {
      //-- get world to model transform
      BMatrix worldToModelMatrix;
      pMyUnit->getWorldMatrix(worldToModelMatrix);
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
      position = pMyUnit->getPosition() + msBonePos;
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BDefaultSquadBehavior::calculatePriorities(BOpportunityList* pList, BEntity* pEntity)
{
   BDEBUG_ASSERT(pEntity->getClassType() == BEntity::cClassTypeSquad);
   BSquad *pSquad = pEntity->getSquad();

   // Reset sorted list
   mTempOpportunityResults.setNumber(0);
   mTempOpportunityOrder.setNumber(0);

   BEntityID currentAttackTargetID=cInvalidObjectID;
//-- FIXING PREFIX BUG ID 2454
   const BSquadActionAttack* pAttackAction=reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
//--
   if (pAttackAction)
      currentAttackTargetID=pAttackAction->getTarget()->getID();

   // MSC: Hack for beams to prioritize targets in front of us over those behind us because it's a pita to turn around
   BVector launchForward, launchPosition;
   bool launchInfoValid = false;
   BProtoAction *pBeamProtoAction = getBeamProtoAction(pEntity);
   if (pBeamProtoAction && pSquad->getLeaderUnit())
      launchInfoValid = getLaunchInfo(pSquad->getLeaderUnit(), pBeamProtoAction, launchPosition, launchForward);

   // Iterate through opportunities a create a sorted list.  Sorted from best to worst
   //
   BEntityID lastAbilityAttackTargetID = pSquad->getLastAbilityAttackTargetID();
   BEntityID lastAttackTargetID = pSquad->getLastAttackTargetID();
   int lastAttackPriority = pSquad->getLastAttackPriority();
   uint numOpps = pList->getNumOpportunities();
   for(uint i=0; i < numOpps; i++)
   {
      const BOpportunity& opportunity = pList->getOpportunity(i);
//-- FIXING PREFIX BUG ID 2453
      const BSquad* pTargetSquad=gWorld->getSquad(opportunity.mTargetID);
//--
      if (!pTargetSquad)
         continue;
      
      float priority = opportunity.mPriority;    
      uint8 type = opportunity.mType;
      switch(type)
      {
         case BOpportunity::eOpportunityTypeAttack:         
         case BOpportunity::eOpportunityTypeAttackMove: 
         case BOpportunity::eOpportunityTypeAttackSecondary:
         {
            priority += 30000.0f;
            // Prioritize the last target a unit ability was used on.
            if (opportunity.mTargetID == lastAbilityAttackTargetID)
               priority += 20000.0f;
            // Prioritize the last target a unit commanded to attack by the user.
            else if (opportunity.mTargetID == lastAttackTargetID && lastAttackPriority == BSimOrder::cPriorityUser)
               priority += 1000.0f;
            break;
         }
         case BOpportunity::eOpportunityTypeMove:
         {
            priority += 10000.0f;
            break;
         }
      }

      //If we can move, adjust the priority based on the time it will take us to cover the
      //distance we need to move.
      float maxV=pSquad->getMaxVelocity();
      if (pSquad->canMove() && (maxV > 0.0f))
         priority-=gDatabase.getOpportunityDistPriFactor()*(opportunity.mMovementDistance/maxV);

      //-- Adjust the priority by the designer specified value for our target type
//-- FIXING PREFIX BUG ID 2452
      const BProtoAction* pProtoAction = opportunity.mpProtoAction;      
//--
      if (pProtoAction && pTargetSquad->getLeaderUnit())
         priority+=pProtoAction->getTargetPriority(pTargetSquad->getLeaderUnit()->getProtoObject());

      //If this squad is attacking us, then we add in a factor for that.
      if ((type == BOpportunity::eOpportunityTypeAttack) ||
         (type == BOpportunity::eOpportunityTypeAttackMove) ||
         (type == BOpportunity::eOpportunityTypeAttackSecondary))
      {
//-- FIXING PREFIX BUG ID 2451
         const BAction* pAttackAction=pTargetSquad->getActionByTypeConst(BAction::cActionTypeSquadAttack);
//--
         if (pAttackAction && (pAttackAction->getTarget()->getID() == pSquad->getID()))
            priority += gDatabase.getOpportunityBeingAttackedPriBonus();
		   // Otherwise, if squad is our current attack target, apply same bonus (this way we want to keep attacking the current squads)
         else if (opportunity.mTargetID == lastAttackTargetID)
            priority += gDatabase.getOpportunityBeingAttackedPriBonus();         
         // If the squad is stunned, apply same bonus (this way we want to keep attacking stunned squads)
         else if (pTargetSquad->isDazed())
            priority += gDatabase.getOpportunityBeingAttackedPriBonus();

         // MSC: Hack for beams to prioritize targets in front of us over those behind us because it's a pita to turn around
         if (launchInfoValid)
         {
            BVector targetVector;
            targetVector.assignDifference(pTargetSquad->getPosition(), launchPosition);
            targetVector.normalize();

            priority += (launchForward.dot(targetVector));
         }
      }

      //-- We've finished calculating the priority, set it in the opportunity
      pList->setPriority(i, priority);

      //-- Calculate distance to target (would be faster to calculate the distanceSqr here, but there's no API for this)
      float distance = pSquad->calculateXZDistance(pTargetSquad);

      // Add to the result array
      BOpportunityResult curResult = BOpportunityResult(i, priority, distance, (opportunity.mTargetID == currentAttackTargetID));
      mTempOpportunityResults.add(curResult);
      uint curResultIndex = static_cast<uint>(mTempOpportunityResults.getNumber()) - 1;

      // Sort based on our sorting order
      uint insertIndex = 0;
      uint numOrderList = static_cast<uint>(mTempOpportunityOrder.getNumber());
      while ((insertIndex < numOrderList) && (curResult < mTempOpportunityResults[mTempOpportunityOrder[insertIndex]]))
      {
         insertIndex++;
      }
      mTempOpportunityOrder.insertAtIndex(curResultIndex, insertIndex);
   }

   int32 highestPriorityIndex  = -1;

   // Now that the results are sorted from best to last, iterate through them to find the first one that passes the 
   // line of sight check.
   //
   uint numOrderList = static_cast<uint>(mTempOpportunityOrder.getNumber());
   for (uint i = 0; i < numOrderList; i++)
   {
      const BOpportunityResult *pCurResult = &mTempOpportunityResults[mTempOpportunityOrder[i]];

      // Check LOS to target
      if (gConfig.isDefined(cConfigTrueLOS))
      {
         BOpportunity opp = pList->getOpportunity(pCurResult->getIndex());

         // Only check LOS for attack related opportunities
         if((opp.mType == BOpportunity::eOpportunityTypeAttack) ||
            (opp.mType == BOpportunity::eOpportunityTypeAttackSecondary) ||
            (opp.mType == BOpportunity::eOpportunityTypeAttackMove))
         {
            BSquad* pTargetSquad=gWorld->getSquad(opp.mTargetID);
            BASSERT(pTargetSquad);

            if(!gSquadLOSValidator.validateLOS(pSquad, opp.mpProtoAction, pTargetSquad))
               continue;
         }
      }
      
      // Use this result.  We are done.
      highestPriorityIndex=pCurResult->getIndex();
      break;
   }
   
   pList->setTopOpportunityIndex(highestPriorityIndex);
}

//==============================================================================
//==============================================================================
void BOpportunityList::deinit()
{
   mOpportunities.clear();
}

//==============================================================================
//==============================================================================
bool BOpportunityList::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSARRAY(pStream, saveType, mOpportunities, uint8, 200);
   //IEntityBehavior* mEntityBehavior;
   GFWRITEVAR(pStream, int32, mTopOpportunityIndex);
   return true;
}

//==============================================================================
//==============================================================================
bool BOpportunityList::load(BStream* pStream, int saveType)
{
   GFREADCLASSARRAY(pStream, saveType, mOpportunities, uint8, 200);
   //IEntityBehavior* mEntityBehavior;
   GFREADVAR(pStream, int32, mTopOpportunityIndex);
   return true;
}
//==============================================================================
//==============================================================================
bool BOpportunity::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mTargetPosition);
   GFWRITEVAR(pStream, BEntityID, mTargetID);
   GFWRITEPROTOACTIONPTR(pStream, mpProtoAction);
   GFWRITEVAR(pStream, float, mMovementDistance);
   GFWRITEVAR(pStream, float, mPriority);
   GFWRITEVAR(pStream, uint8, mType);
   return true;
}

//==============================================================================
//==============================================================================
bool BOpportunity::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mTargetPosition);
   GFREADVAR(pStream, BEntityID, mTargetID);
   GFREADPROTOACTIONPTR(pStream, mpProtoAction);
   GFREADVAR(pStream, float, mMovementDistance);
   GFREADVAR(pStream, float, mPriority);
   GFREADVAR(pStream, uint8, mType);
   return true;
}
