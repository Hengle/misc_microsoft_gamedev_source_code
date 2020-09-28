//==============================================================================
// actionmoveair.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionMoveAir.h"
#include "UnitActionRangedAttack.h"
#include "UnitActionDetonate.h"
#include "unitactionairtrafficcontrol.h"
#include "SquadActionMove.h"
#include "SquadActionCarpetBomb.h"
#include "ConfigsGame.h"
#include "unit.h"
#include "syncmacros.h"
#include "Platoon.h"
#include "protoobject.h"
#include "squad.h"
#include "grannyinstance.h"
#include "UnitOpportunity.h"
#include "visual.h"
#include "World.h"
#include "pather.h"
#include "worldsoundmanager.h"
#include "tactic.h"

#define cMaxFindPathAttempts              3
#define cTransitionTimeNavBackToSquad     2.0f
#define cTransitionTimeNavToTarget        1.5f
#define cTransitionTimeHover              2.0f
#define cMaxStrayDistance                35.0f
#define cTransitionTimeExceedSquadDist    1.0f


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionMoveAir, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BUnitActionMove::connect(pOwner, pOrder))
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMoveAir::disconnect()
{
   if (mAirBase != cInvalidObjectID)
   {
      BUnit* pAirBase = gWorld->getUnit(mAirBase);
      if (pAirBase)
      {
         BUnitActionAirTrafficControl* pControllerAction = reinterpret_cast<BUnitActionAirTrafficControl*>(pAirBase->getActionByType(BAction::cActionTypeUnitAirTrafficControl));
         BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
         if (pControllerAction)
            pControllerAction->releaseSpot(pUnit);
      }
      mAirBase = cInvalidObjectID;
   }
  
   BUnitActionMove::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::init(void)
{
   if (!BAction::init())
      return false;

   mFlagConflictsWithIdle=true;
   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;
   mPathMoveData=NULL;
   mFlagSquadMove=false;
   mState=cStateNone;

   mGoalPosition = cInvalidVector;
   mTurnRate = 0.0f;
   mRollRate = 0.0f;
   mRollAngle = 0.0f;

   mGoalAltitudeInc = 0.0f;
   mCurrAltitudeInc = 0.0f;
   mAltitudeSelectTimer = 0.0f;
   mSpeedSelectTimer = 0.0f;
   mGoalSpeed = 0.0f;
   mRollDelayTimer = 0.0f;
   mTargetDist = 0.0f;
   mPrevAltChange = 0.0f;
   mAttackRunDelayTimer = 0.0f;
   mHoverTimer = 0.0f;

   setFlagPersistent(true);
   mTacticState = cTacticNav;
   mManeuverState = cManeuverDefault;
   mRollDir = cDirLeft;
   mbInverted = false;
   mbUpright = false;
   mbDiveAttack = false;
   mbFirstAttackPass = true;
   mbCanLand = false;
   mbLaunch = false;
   mbReturnToBase = false;
   mAirBase = cInvalidObjectID;
   mSpotForward.zero();
   mSpotForward.x = 1.0;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::setState(BActionState state)
{
   syncUnitActionData("BUnitActionMove::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionMove::setState state", state);

//-- FIXING PREFIX BUG ID 2048
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   switch (state)
   {
      case cStateWorking:
         mpOwner->startMove();
         break;

      case cStateFailed:
         mpOwner->stopMove();

//         pUnit->completeOpp(mOppID, false);

         // Release our controllers
//         releaseControllers();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::update(float elapsedTime)
{
   BDEBUG_ASSERT(mpOwner);
   BDEBUG_ASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit));
/*
   //If we no longer have the controllers we need, fail.
   if (!validateControllers())
   {
      syncUnitActionData("BUnitActionMoveAir::update validateControllers failed", mpOwner->getID().asLong());
      setState(cStateFailed);
      return (true);
   }
*/
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   // update attack delay timer
   mAttackRunDelayTimer -= elapsedTime;
   if (mAttackRunDelayTimer < 0.0f)
      mAttackRunDelayTimer = 0.0f;

   // update hover timer
   mHoverTimer -= elapsedTime;
   if (mHoverTimer < 0.0f)
      mHoverTimer = 0.0f;

   // If we're doing a fatality, we're not moving
   if (pUnit->getFlagDoingFatality())
      return true;

   BSquad *pSquad = pUnit->getParentSquad();
   if (!pSquad)
      return false;

    // If squad move is not in working state and there is an attack action, let squad leader drag the squad position
/*
//-- FIXING PREFIX BUG ID 2051
   const BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
//-- FIXING PREFIX BUG ID 2052
   const BSquadActionMove* pSquadMove = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
//--
   if (pAttackAction && (!pSquadMove || pSquadMove->getState() != cStateWorking) && (pUnit->getID() == pSquad->getChild(0)))
   {
      float squadDist = pSquad->getPosition().distanceEstimate(mGoalPosition);
      float ownDist = pUnit->getPosition().distanceEstimate(mGoalPosition);
      if (ownDist < squadDist)
         pSquad->setPosition(pUnit->getPosition());
   }
*/
   BSquadActionCarpetBomb* pCarpetBomb = reinterpret_cast<BSquadActionCarpetBomb*>(pSquad->getActionByType(BAction::cActionTypeSquadCarpetBomb));

   if (mGoalPosition == cInvalidVector)
   {
      // Get a pointer to our airbase and then get a landing spot assignment
      BEntityRef *pEntityRef = pSquad->getFirstEntityRefByType(BEntityRef::cTypeTrainLimitBuilding);
      if (pEntityRef)
         mAirBase = pEntityRef->mID;
      BUnit* pAirBase = gWorld->getUnit(mAirBase);
      if (pAirBase)
      {
         BUnitActionAirTrafficControl* pControllerAction = reinterpret_cast<BUnitActionAirTrafficControl*>(pAirBase->getActionByType(BAction::cActionTypeUnitAirTrafficControl));
         if (pControllerAction)
            mPadSpot = pControllerAction->requestLandingSpot(pUnit, mSpotForward);

         mGoalPosition = mBasePosition = pAirBase->getPosition();
      }
      else
      {
         mGoalPosition = mBasePosition = mPadSpot = pUnit->getPosition(); // No airbase - someone just conjured my owner out of the air
         mbLaunch = true;
      }

      mpOwner->setFlagTiesToGround(false);

      // Make sure the squad's carpet bomb action has the correct base position (trigger spawned aircraft may have a problem otherwise)
      if (pCarpetBomb)
         pCarpetBomb->setStartingLocation(mBasePosition);
   }
//   else if (pAttackAction && (mTarget.getPosition() != cInvalidVector) && (mTacticState != cTacticExtend))
//      mGoalPosition = mTarget.getPosition();
//   else
//      mGoalPosition = pSquad->getPosition();

   //Main logic is here.  Don't put too much linear execution stuff in the actual
   //case statements themselves; write straightforward methods for that.
   BActionState newState = mState;
   switch (mState)
   {
      case cStateNone:

         pUnit->setAmmunition(pUnit->getAmmoMax());
         mbReturnToBase = false;
         advanceToNewPosition2(elapsedTime, mPadSpot, mSpotForward);
         if (mbLaunch || mAirBase==cInvalidObjectID)
            setState(cStatePathing);
         break;

      case cStatePathing:

         // Create initial high level path to target
         newState = createInitialPathData();
         setState(newState);
         break;

      case cStateWorking:

         //Follow movement.
         newState = followPath(elapsedTime);
/*
         BVector squadPos = pSquad->getPosition();
         BVector unitPos = pUnit->getPosition();
         squadPos.y = mBasePosition.y;

         if (mbReturnToBase && (unitPos.distance(mBasePosition) < 30.0f)) // Home - land me
         {
            launch(false);
            setState(cStateNone);
         }
*/
         //Handle state changes.
         if (newState != cStateWorking)
            setState(newState);
         break;
   }


   // If bomber or air striker - Set the squad position toward the strike location from the lead unit, but not farther than a specific range
   if (pUnit->getID() == pSquad->getChild(0) && pCarpetBomb) // Only member or flight lead
   {
      BVector strikePos = mBasePosition;
      float squadLeadRange = 100.0f;
      if (mGoalPosition != mBasePosition)
      {
         if (pCarpetBomb)
         {
            strikePos = pCarpetBomb->getCarpetBombLocation();
            squadLeadRange = 40.0f;
         }

         // If the squad action has trashed the vector - just go home
         if (strikePos == cInvalidVector)
            strikePos = mBasePosition;
      }

      BVector unitToStrikePosVec = strikePos - pUnit->getPosition();
      float unitToStrikePosDist = unitToStrikePosVec.length();
      unitToStrikePosVec.normalize();

      BVector newSquadPos = pSquad->getPosition();
      if (unitToStrikePosDist > squadLeadRange)
      {
         newSquadPos = pUnit->getPosition() + unitToStrikePosVec * squadLeadRange;
      }
      else
         newSquadPos = strikePos;

      pSquad->setPosition(newSquadPos);
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMoveAir::followPath(float elapsedTime)
{   
   // Calculate new position based on current pathing method
   BVector newPosition;
   BVector newForward;
   calcFlightMovement(elapsedTime, newPosition, newForward);
   // Set the new position
   advanceToNewPosition2(elapsedTime, newPosition, newForward);

   return (cStateWorking);
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMoveAir::calcFlightMovement(float elapsedTime, BVector& newPosition, BVector& newForward)
{
   // NOTE: If this ever returns false, the movement data is trashed/malformed.
   BUnit* pUnit = mpOwner->getUnit();
   newPosition = pUnit->getPosition();
   newForward = pUnit->getForward();
   newForward.y = 0.0f;

   // This already checked by caller for validity
   BActionState newState = cStateWorking;
   float velocityToFutureTarget = -1.0f;
   
   updateTactics();

   // We want "level" speed - ignore vertical component here
   BVector currVel = pUnit->getVelocity();
   currVel.y = 0.0f;
   float currentSpeed = currVel.length();
   float maxSpeed = pUnit->getMaxVelocity();

   // Calculate our movement direction.  First get goal heading, then apply turn rate over time to get new heading.
   BVector heading = pUnit->getForward();
   heading.y = 0.0f;
   heading.safeNormalize();

   BVector goalHeading = mGoalPosition - pUnit->getPosition();
   goalHeading.y = 0;
   if (!goalHeading.safeNormalize()) // If we are on the goal position, keep current heading
      goalHeading = heading;

   BVector newHeading = heading;
   float maxTurnRate = pUnit->getProtoObject()->getTurnRate() * cPi / 180.0f; // rad/sec
   float turnAccelerationRate = maxTurnRate; // rad/sec/sec
   BIntrinsicVector  headingCross = heading.cross(goalHeading);
   float             headingDot   = heading.dot(goalHeading);
   float goalTurnRate = headingCross.length(); // Decreases to zero as vectors align
   goalTurnRate = sqrtf(goalTurnRate); // This smoothly increases the aggressiveness of the turn

   if (headingDot < 0.0f) // If more than 90 degree to go...
      goalTurnRate = maxTurnRate; // Use max turn rate

   goalTurnRate *= maxTurnRate; // Scale to vehicle performance


   if (headingCross.y < 0.0f) // Correct for direction of turn
      goalTurnRate*= -1.0f;

   // Now that we have a goal turn rate, apply turn acceleration
   float goalTurnRateDelta = goalTurnRate - mTurnRate;
   float maxTurnRateDelta = turnAccelerationRate * elapsedTime;

   if (fabs(goalTurnRateDelta) < maxTurnRateDelta) // We can match goal turn rate now
      mTurnRate = goalTurnRate;
   else // Otherwise, apply max turn rate acceleration (in the proper direction) to mTurnRate
   {
      if (goalTurnRateDelta < 0.0f)
         maxTurnRateDelta *= -1.0f;
      mTurnRate += maxTurnRateDelta;
   }

   if (mTurnRate > maxTurnRate)
      mTurnRate = maxTurnRate;
   if (mTurnRate < -maxTurnRate)
      mTurnRate = -maxTurnRate;
/*
   // Degraded Flight Control: If damaged, introduce a random error into turn rate
   float damageFraction = 1.0f - pUnit->getHPPercentage();
   if (damageFraction > 0.0f)
   {
      float errorScale = 4.0f * elapsedTime * damageFraction;
      mTurnRate += getRandRangeFloat(cSimRand, -errorScale, errorScale);
   }
*/
   // Calculate new heading
   float turnAngle = mTurnRate * elapsedTime;
   newHeading.rotateXZ(turnAngle);

   // Calculate a plausible roll angle
   mRollAngle = mTurnRate * mTurnRate * -0.8f / (maxTurnRate * maxTurnRate);
   if (mTurnRate < 0.0f)
      mRollAngle *= -1.0f;

   // Calculate velocity.  Check desired velocity based on projected position.  Use some fraction of max or 100% max speed
   // HACK: R&D - waiting for design decisions
   const float yDisplacement = mpOwner->getYDisplacement();
   float minAlt = yDisplacement - 5.0f;
   float maxAlt = yDisplacement + 15.0f;
   float changeAltTime = 4.0f;
   float changeSpeedTime = 1.0f;

   float goalSpeed = currentSpeed;
   if (mTacticState == cTacticExtend || velocityToFutureTarget > 0.6f * maxSpeed)
      goalSpeed = maxSpeed;
   else
      goalSpeed = 0.6f * maxSpeed;

   // HACK: R&D - waiting for design decisions
   if (pUnit->getProtoObject()->isType(gDatabase.getObjectType("Flood")))
   {
      changeAltTime = 2.0f;
      if (mSpeedSelectTimer <= 0.0f)
      {
         mGoalSpeed = getRandRangeFloat(cSimRand, 0.7f * maxSpeed, 1.0f * maxSpeed);
         if (mGoalSpeed < 0.4f * maxSpeed)
            mGoalSpeed = 0.4f * maxSpeed;

         mSpeedSelectTimer = changeSpeedTime;
      }
      mSpeedSelectTimer -= elapsedTime;
      goalSpeed = mGoalSpeed;
   }

   float deltaSpeed = goalSpeed - currentSpeed;
   currentSpeed += 1.0f * deltaSpeed * elapsedTime;

   // Calculate altitude
   BVector position = pUnit->getPosition();
   const float lookAheadScale = 3.0f;
   float newHeight;

   if (gTerrainSimRep.flightHeightsLoaded())
      gTerrainSimRep.getFlightHeightRaycast(position, position.y, true, true);
   else
      gTerrainSimRep.getCameraHeight(position, true);

   BVector forwardPosition = position + lookAheadScale * newForward;
   if (gTerrainSimRep.flightHeightsLoaded())
      gTerrainSimRep.getFlightHeightRaycast(position, position.y, true, true);
   else
      gTerrainSimRep.getCameraHeight(forwardPosition, true);

   newHeight = Math::Max(position.y, forwardPosition.y) + yDisplacement;

   // Add a randomized component for natural variation
   if (mAltitudeSelectTimer <= 0.0f)
   {
      mAltitudeSelectTimer = changeAltTime;
      mGoalAltitudeInc = getRandRangeFloat(cSimRand, minAlt, maxAlt);
   }

   // Manage climb/dive behavior for ground targeting
   float diveAltitudeInc = 0.0f;
   if (mbDiveAttack)
      diveAltitudeInc = 5.0f;
   else
      diveAltitudeInc = 0.0f;

   mCurrAltitudeInc += 2.0f * (mGoalAltitudeInc + diveAltitudeInc - mCurrAltitudeInc) * elapsedTime;
   float diff = 2.0f * newHeight + mCurrAltitudeInc - 2.0f * (pUnit->getPosition()).y;

   float altitudeChange = diff * elapsedTime;

   // Update Behavior Timers
   mAltitudeSelectTimer -= elapsedTime;
   mRollDelayTimer -= elapsedTime;

   // Calculate our position change.
   BVector currentVelocity = newHeading * currentSpeed;
   BVector positionChange = currentVelocity * elapsedTime;
   positionChange.y = altitudeChange;

   // Add in any offset due to evasive roll
   pUnit->setVelocity(currentVelocity);
   if( mManeuverState == cManeuverRoll )
   {
      BVector fwd = newForward;
      fwd.y = 0.0f;
      fwd.safeNormalize();
      BVector rollOffset = cYAxisVector.cross(fwd);
      rollOffset *= -mRollRate;
      positionChange += 3.0f * rollOffset * elapsedTime;
   }

//-- FIXING PREFIX BUG ID 2055
   const BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
   if (mTacticState == cTacticKamikazeDive && pAttackAction) // Move precisely toward target
   {
      const BSimTarget* simTarget = pAttackAction->getTarget();
      BVector targetPos;
      targetPos = mGoalPosition;
//-- FIXING PREFIX BUG ID 2054
      const BEntity* pTarget = NULL;
//--
      if (simTarget->getID().isValid())
      {
         pTarget = gWorld->getEntity(simTarget->getID());
         if (pTarget)
            targetPos = pTarget->getPosition();
         else
            BASSERT(0);
      }

      float diveFraction = altitudeChange / (targetPos.y - pUnit->getPosition().y);
      BVector positionOffset = targetPos - pUnit->getPosition();
      positionChange = diveFraction * positionOffset;

      newHeading = positionChange;
   }

   // Calculate the new position
   newPosition = pUnit->getPosition();
   newPosition += positionChange;
   newForward = newHeading;

   // Include altitude change into forward vector
   mPrevAltChange = altitudeChange;
   newForward.y = mPrevAltChange + elapsedTime * (altitudeChange - mPrevAltChange);
   newForward.safeNormalize();

   return (newState);
}

//==============================================================================
//==============================================================================
void BUnitActionMoveAir::advanceToNewPosition2(float elapsedTime, BVector newPosition, BVector newForward)
{
   BUnit* pUnit = mpOwner->getUnit();
   pUnit->setForward(newForward);
   pUnit->setUp(cYAxisVector);
   pUnit->calcRight();
   pUnit->calcUp();
   
   // No motion extraction. Just set position.
   #ifdef SYNC_Unit
      syncUnitData("BUnitActionMoveAir::advanceToNewPosition2", newPosition);
   #endif
   pUnit->setPosition(newPosition);
}

//==============================================================================
//==============================================================================
void BUnitActionMoveAir::updateManeuvers(float elapsedTime, BVector& newPosition, BVector& newForward)
{
   switch (mManeuverState)
   {
      case cManeuverDefault:
      {
         // Check whether to start a roll or Immelman turn
         BUnit* pUnit = mpOwner->getUnit();
         pUnit->setForward(newForward);
         pUnit->setUp(cYAxisVector);
         pUnit->calcRight();
         pUnit->calcUp();

         // Roll this puppy
         pUnit->roll(mRollAngle);

         break;
      }
      case cManeuverRoll:
      {
         mTurnRate = 0.0f; // No Turning during the roll!

         BVector upVec = mpOwner->getUp();

         float rollCos = upVec.y;
         float rollSin = 1.0f - (rollCos * rollCos);
         if (rollCos < 0.0f)
           mbInverted = true;

         if (mbInverted && rollCos > 0.0f)
            mbUpright = true;

         // BSR-FIXME: Initial test values
         float maxRollRate = 5.0f; // rad/sec
         float rollAccelerationRate = 10.0f; // rad/sec/sec
         float goalRollRate = rollSin; // Decreases to zero as we roll upright
         goalRollRate = sqrtf(goalRollRate); // This smoothly increases the aggressiveness of the roll

         if (!mbInverted || rollCos < 0.0f) // If more than 90 degree to go...
            goalRollRate = 1.0f; // Use max roll rate

         goalRollRate *= maxRollRate; // Scale to vehicle performance

         if (mRollDir == cDirRight) // Correct for direction of roll
            goalRollRate*= -1.0f;

         // Now that we have a goal roll rate, apply roll acceleration
         float goalRollRateDelta = goalRollRate - mRollRate;
         float maxRollRateDelta = rollAccelerationRate * elapsedTime;

         if (fabs(goalRollRateDelta) < maxRollRateDelta) // We can match goal roll rate now
            mRollRate = goalRollRate;
         else // Otherwise, apply max roll rate acceleration (in the proper direction) to mRollRate
         {
            if (goalRollRateDelta < 0.0f)
               maxRollRateDelta *= -1.0f;
            mRollRate += maxRollRateDelta;
         }

         // Degraded Flight Control: If damaged, introduce a random error into roll rate
         BUnit* pUnit = mpOwner->getUnit();

         float damageFraction = 1.0f - pUnit->getHPPercentage();
         if (damageFraction > 0.0f)
         {
            float errorScale = 4.0f * elapsedTime * damageFraction;
            mRollRate += getRandRangeFloat(cSimRand, -errorScale, errorScale);
         }

         // Calculate new roll angle
         float rollAngle = mRollRate * elapsedTime;
         mpOwner->roll(rollAngle);

         // Are we done yet?
         BVector rightVec = mpOwner->getRight();
         if (mbInverted && mbUpright && fabs(rightVec.y) <= 0.1f)
            mManeuverState = cManeuverDefault;

         break;
      }
      case cManeuverImmelman:
         break;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionMoveAir::updateTactics()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return;

   //-- FIXING PREFIX BUG ID 2064
   const BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
   //--

   // If unit has moved too far away from squad, force it to move back.  This uses squad leash distance as a factor
   // but this might be a little farther than we want.  Unit leash distance is way too short so use a fraction of squad leash distance.
   float distFromSquad = pUnit->calculateXZDistance(pSquad);
   float maxStrayDistance = Math::Max(cMaxStrayDistance, pSquad->getLeashDistance() * 0.5f);
   if (distFromSquad > maxStrayDistance)
   {
      mTacticState = cTacticNavBackToSquad;
      mHoverTimer = cTransitionTimeExceedSquadDist;
   }


   switch (mTacticState)
   {
   case cTacticNavBackToSquad:
      pUnit->setFlagAttackBlocked(true);
      mGoalPosition = pSquad->getPosition();

      if (mHoverTimer <= 0.0f)
      {
         mTacticState = cTacticNav;
         mHoverTimer = cTransitionTimeNavToTarget;
      }

      break;

   case cTacticNav:

      pUnit->setFlagAttackBlocked(true);

      if (pAttackAction)
      {
         const BSimTarget* simTarget = pAttackAction->getTarget();
         BVector targetPos;
//-- FIXING PREFIX BUG ID 2061
         const BEntity* pTarget = NULL;
//--
         if (simTarget->getID().isValid())
         {
            pTarget = gWorld->getEntity(simTarget->getID());
            if (!pTarget)
               return;

            targetPos = pTarget->getPosition();
         }
         else if(simTarget->isPositionValid())
         {
            targetPos = simTarget->getPosition();
         }

         mTacticState = cTacticStrafe;

         // Choose a position half way to target so the units don't stray too far from the squad
         mGoalPosition.assignSum(targetPos, pSquad->getPosition());
         mGoalPosition.scale(0.5f);
      }
      else
         mGoalPosition = pSquad->getPosition();

      break;

   case cTacticStrafe:

      mbDiveAttack = false;

      if (!pAttackAction)
         mTacticState = cTacticNav;
      else if (mHoverTimer <= 0.0f)
      {
         mTacticState = cTacticLaunchHover;
         mHoverTimer = cTransitionTimeHover;
      }

      break;
   case cTacticLaunchHover:

      if (mHoverTimer > 0.0f)
      {
         mGoalSpeed = 0.0f;
         pUnit->setFlagAttackBlocked(false);
      }
      else
      {
         mHoverTimer = cTransitionTimeNavBackToSquad;
         mTacticState = cTacticNavBackToSquad;
         pUnit->setFlagAttackBlocked(true);
      }

      break;
   }
}


//=============================================================================
//=============================================================================
BEntityID BUnitActionMoveAir::initEffectsFromProtoObject(long protoObjectID, BVector pos)
{
   BObjectCreateParms parms;
   parms.mPlayerID = mpOwner->getPlayerID();
   parms.mPosition = pos;
   parms.mRight = cXAxisVector;
   parms.mForward = cZAxisVector;
   parms.mProtoObjectID = protoObjectID;   
   parms.mIgnorePop = true;
   parms.mNoTieToGround = true;
   parms.mPhysicsReplacement = false;
   parms.mType = BEntity::cClassTypeObject;   
   parms.mStartBuilt=true;

   BEntityID id = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 2069
   const BObject* pObject = gWorld->createObject(parms);
//--
   if (pObject)
      id = pObject->getID();

   return id;
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::validateRange() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

//-- FIXING PREFIX BUG ID 2072
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   //Get our range as defined by our PA.
   float range = 0.0f;
   if (mTarget.isRangeValid())
   {
      range = mTarget.getRange();
   }
   else if (mpProtoAction)
   {
      range = mpProtoAction->getMaxRange(pUnit);
   }

   //See if our squad is in range of our target.
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 2071
      const BEntity* pEntity = gWorld->getEntity(mTarget.getID());
//--
      if (!pEntity)
      {
         return (false);
      }

      return (pSquad->calculateXZDistance(pEntity) <= range);      
   }

   if (!mTarget.isPositionValid())
   {
      return (false);
   }

   return (pSquad->calculateXZDistance(mTarget.getPosition()) <= range);
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVECTOR(pStream, mGoalPosition);
   GFWRITEVECTOR(pStream, mBasePosition);
   GFWRITEVECTOR(pStream, mPadSpot);
   GFWRITEVECTOR(pStream, mSpotForward);
   GFWRITEVAL(pStream, BEntityID, mAirBase);
   GFWRITEVAR(pStream, float, mTargetDist);
   GFWRITEVAR(pStream, float, mTurnRate);
   GFWRITEVAR(pStream, float, mRollRate);
   GFWRITEVAR(pStream, float, mRollAngle);
   GFWRITEVAR(pStream, float, mGoalAltitudeInc);
   GFWRITEVAR(pStream, float, mCurrAltitudeInc);
   GFWRITEVAR(pStream, float, mPrevAltChange);
   GFWRITEVAR(pStream, float, mAltitudeSelectTimer);
   GFWRITEVAR(pStream, float, mSpeedSelectTimer);
   GFWRITEVAR(pStream, float, mGoalSpeed);
   GFWRITEVAR(pStream, float, mRollDelayTimer);
   GFWRITEVAR(pStream, float, mAttackRunDelayTimer);
   GFWRITEVAR(pStream, float, mHoverTimer);
   GFWRITEVAR(pStream, BTacticState, mTacticState);
   GFWRITEVAR(pStream, BManeuverState, mManeuverState);
   GFWRITEVAR(pStream, BDirection, mRollDir);
   GFWRITEBITBOOL(pStream, mbInverted);
   GFWRITEBITBOOL(pStream, mbUpright);
   GFWRITEBITBOOL(pStream, mbDiveAttack);
   GFWRITEBITBOOL(pStream, mbFirstAttackPass);
   GFWRITEBITBOOL(pStream, mbCanLand);
   GFWRITEBITBOOL(pStream, mbLaunch);
   GFWRITEBITBOOL(pStream, mbReturnToBase);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveAir::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVECTOR(pStream, mGoalPosition);
   GFREADVECTOR(pStream, mBasePosition);
   GFREADVECTOR(pStream, mPadSpot);
   GFREADVECTOR(pStream, mSpotForward);
   GFREADVAR(pStream, BEntityID, mAirBase);
   GFREADVAR(pStream, float, mTargetDist);
   GFREADVAR(pStream, float, mTurnRate);
   GFREADVAR(pStream, float, mRollRate);
   GFREADVAR(pStream, float, mRollAngle);
   GFREADVAR(pStream, float, mGoalAltitudeInc);
   GFREADVAR(pStream, float, mCurrAltitudeInc);
   GFREADVAR(pStream, float, mPrevAltChange);
   GFREADVAR(pStream, float, mAltitudeSelectTimer);
   GFREADVAR(pStream, float, mSpeedSelectTimer);
   GFREADVAR(pStream, float, mGoalSpeed);
   GFREADVAR(pStream, float, mRollDelayTimer);
   GFREADVAR(pStream, float, mAttackRunDelayTimer);
   GFREADVAR(pStream, float, mHoverTimer);
   GFREADVAR(pStream, BTacticState, mTacticState);
   GFREADVAR(pStream, BManeuverState, mManeuverState);
   GFREADVAR(pStream, BDirection, mRollDir);
   GFREADBITBOOL(pStream, mbInverted);
   GFREADBITBOOL(pStream, mbUpright);
   GFREADBITBOOL(pStream, mbDiveAttack);
   GFREADBITBOOL(pStream, mbFirstAttackPass);
   GFREADBITBOOL(pStream, mbCanLand);
   GFREADBITBOOL(pStream, mbLaunch);
   GFREADBITBOOL(pStream, mbReturnToBase);

   return true;
}
