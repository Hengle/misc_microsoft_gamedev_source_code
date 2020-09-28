//==============================================================================
// actionmove.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionMove.h"
#include "ConfigsGame.h"
#include "Formation2.h"
#include "grannyinstance.h"
#include "pather.h"
#include "Platoon.h"
#include "protoobject.h"
#include "squad.h"
#include "syncmacros.h"
#include "Tactic.h"
#include "unit.h"
#include "UnitOpportunity.h"
#include "visual.h"
#include "World.h"
#include "squadactionmove.h"
#include "grannymanager.h"
#include "prepost.h"
#include "protosquad.h"
#include "simhelper.h"

//#define DEBUGACTION
//#define ENABLE_UNIT_COLLISIONS

#define cMaxFindPathAttempts              3

#ifndef _MOVE4
#define _MOVE4
#endif


#ifdef _MOVE4
const float cActionCompleteEpsilon = 0.1f;
const long cMaxRetryAttempts = 3;
#endif

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BPathMoveData, 4, &gSimHeap)

//==============================================================================
//==============================================================================
BPathMoveData::BPathMoveData()
{
   clear();
}

//==============================================================================
//==============================================================================
BPathMoveData::~BPathMoveData()
{
}

//==============================================================================
//==============================================================================
void BPathMoveData::onAcquire()
{
   clear();
}

//==============================================================================
//==============================================================================
void BPathMoveData::onRelease()
{
}

//==============================================================================
//==============================================================================
void BPathMoveData::clear()
{
   mPath.reset();
   mCurrentWaypoint = -1;
   mPathTime = 0;
   mLinkedPath = NULL;
   mPathLevel = cPathLevelUser;
}

//==============================================================================
//==============================================================================
bool BPathMoveData::isJumpPath() const
{
   if (mPath.getFlag(BPath::cJump))
      return (true);
   if (mLinkedPath)
      return(mLinkedPath->isJumpPath());
   return (false);
}




//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionMove, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionMove::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }
     
   
   //If we're in a gaggle formation, go persistent.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   pUnit->setFlagTurning(false);
   if (pUnit && pUnit->getParentSquad() && (pUnit->getParentSquad()->getFormationType() == BFormation2::eTypeGaggle))
      setFlagPersistent(true);

   mHasTurnAnimation = pUnit->hasAnimation(cAnimTypeTurnAround);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   pUnit->setFlagTurning(false);

   //Release our controllers.
   releaseControllers();

   mpOwner->stopMove();

   clearPathData();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::init()
{
   if (!BAction::init())
      return (false);

   mFlagConflictsWithIdle=true;
   mTarget.reset();
   mOppID=BUnitOpp::cInvalidID;
   mPathMoveData=NULL;
   mVelocity=0.0f;
   mVelocityRandomFactor=1.0f;
   mVelocityRandomFactorTimer=0.0f;
   mWaitTimer=0.0f;
   mFlagSquadMove=false;
   mRefreshPathData = false;
   mHasTurnAnimation=false;
#ifdef _MOVE4
   mRetryAttempts_4 = 0;
#endif
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::clearPathData()
{
   BPathMoveData* pTemp1 = mPathMoveData;
   while (pTemp1 != NULL)
   {
      BPathMoveData* pTemp2 = pTemp1->mLinkedPath;
      BPathMoveData::releaseInstance(pTemp1);
      pTemp1 = pTemp2;
   }
   mPathMoveData = NULL;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   syncUnitActionData("BUnitActionMove::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionMove::setState state", state);
   #ifdef DEBUGACTION
   mpOwner->debug("UnitActionMove::setState:: OldState=%s, newState=%d.", getStateName(), state);
   #endif

   switch (state)
   {
      case cStateWorking:
      case cStateTurning:
      case cStateStartMove:
      case cStateStopMove:
         if (!pUnit->isMoving())
            mpOwner->startMove();
         break;
         
      case cStateWait:
         mpOwner->endMove();
         mWaitTimer=getRandRangeFloat(cSimRand, 0.4f, 1.0f);
         break;

      case cStateDone:
      case cStateFailed:
         if (state == cStateDone)
            pUnit->completeOpp(mOppID, true);
         else if (state == cStateFailed)
            pUnit->completeOpp(mOppID, false);

         mpOwner->stopMove();
         break;
   }
 
   return(BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::update(float elapsedTime)
{
   SCOPEDSAMPLE(BUnitActionMove_update);
   BASSERT(mpOwner);
   BDEBUG_ASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit));
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//-- FIXING PREFIX BUG ID 3358
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERTM(pSquad, "Trying to move a unit with invalid parent squad!");
   //mpOwner->debug("MoveUpdate:: Pos=(%6.2f, %6.2f), For=(%6.2f, %6.2f), State=%s, Vel=(%6.2f, %6.2f).",
   //   mpOwner->getPosition().x, mpOwner->getPosition().z, mpOwner->getForward().x,
   //   mpOwner->getForward().z, getStateName(), mpOwner->getVelocity().x, mpOwner->getVelocity().z);

   //If we no longer have the controllers we need, fail.
   if (!validateControllers())
   {
      //If we're persistent, try to re-grab the controllers.
      if (!getFlagPersistent() || !grabControllers())
      {
         syncUnitActionData("BUnitActionMove::update validateControllers failed", mpOwner->getID().asLong());
         setState(cStateFailed);
         return (true);
      }
   }

   // If we're doing a fatality, we're not moving
   if (pUnit->getFlagDoingFatality())
      return true;

   //SPECIAL PROCESSING: If we're persistent and gaggling, then we want to set our
   //target to the target of our attack action (if we have one).
   if (mFlagPersistent && pSquad && (pSquad->getFormationType() == BFormation2::eTypeGaggle))
   {
//-- FIXING PREFIX BUG ID 3357
      const BAction* pAttackAction=pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack);
//--
      if (pAttackAction)
      {
         const BSimTarget* pSimTarget=pAttackAction->getTarget();
         if (pSimTarget && pSimTarget->getID().isValid())
            setTarget(BSimTarget(pSimTarget->getID()));
         else
            setTarget(BSimTarget());
      }
      else
         setTarget(BSimTarget());
   }

   bool moving = pUnit->isMoving();

   //Main logic is here.  Don't put too much linear execution stuff in the actual
   //case statements themselves; write straightforward methods for that.
   switch (mState)
   {
      /////////////////////////////////////////////////////////////////////////////////////
      // SLB: This is to fix the problem of actions going away on their own
      case cStateNone:
      {
         long moveAnimType = pUnit->getAnimationType(cMovementAnimationTrack);
         switch (moveAnimType)
         {
            case cAnimTypeTurnAround:
            case cAnimTypeTurnLeft:
            case cAnimTypeTurnRight:
            case cAnimTypeTurnRight45Forward:
            case cAnimTypeTurnRight45Back:
            case cAnimTypeTurnLeft45Back:
            case cAnimTypeTurnLeft45Forward:
               setState(cStateTurning);
               return (true);

            case cAnimTypeTurnWalk:
            case cAnimTypeTurnJog:
            case cAnimTypeTurnRun:
            case cAnimTypeIdleWalk:
            case cAnimTypeIdleJog:
            case cAnimTypeIdleRun:
               setState(cStateStartMove);
               working(elapsedTime, moving);
               return (true);

            case cAnimTypeWalkIdle:
            case cAnimTypeJogIdle:
            case cAnimTypeRunIdle:
               setState(cStateStopMove);
               working(elapsedTime, moving);
               return (true);

            default:
               moving = pUnit->isMoveAnimType(moveAnimType);
               break;
         }
      }
      /////////////////////////////////////////////////////////////////////////////////////

      case cStatePathing:
      {
         #ifdef _MOVE4
         if (++mRetryAttempts_4 > cMaxRetryAttempts)
            setState(cStateFailed);
         else
         {
            setState(cStateWorking);
            // DLM - Do a single update of working as soon as we set the state to working -- this should
            // remove the one update latency between when a unit is commanded and when it actually starts moving
            working(elapsedTime, true);
         }
         break;
         #endif
         // Create initial high level path to target
         BActionState newState = createInitialPathData();
         mRefreshPathData = false;

         setState(newState);
         if (mState == cStateWorking)
            working(elapsedTime, moving);

         break;
      }

      case cStateStartMove:
      {
         long activeAnimation = pUnit->getAnimationType(cActionAnimationTrack);
         if ((activeAnimation != cAnimTypeTurnWalk) && (activeAnimation != cAnimTypeTurnJog) && (activeAnimation != cAnimTypeTurnRun) &&
             (activeAnimation != cAnimTypeIdleWalk) && (activeAnimation != cAnimTypeIdleJog) && (activeAnimation != cAnimTypeIdleRun))
            setState(cStateWorking);
         working(elapsedTime, true);
         break;
      }

      case cStateStopMove:
      {
         long activeAnimation = pUnit->getAnimationType(cActionAnimationTrack);
         if ((activeAnimation != cAnimTypeWalkIdle) && (activeAnimation != cAnimTypeJogIdle) && (activeAnimation != cAnimTypeRunIdle))
            setState(cStateDone);

         break;
      }

      case cStateWorking:
         working(elapsedTime, moving);
         break;
      
      case cStateTurning:
      {
         long activeAnimation = pUnit->getAnimationType(cActionAnimationTrack);
         if ((activeAnimation == cAnimTypeTurnWalk) || (activeAnimation == cAnimTypeTurnJog) || (activeAnimation == cAnimTypeTurnRun))
         {
            setState(cStateStartMove);
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         }
         break;
      }

      case cStateWait:
         if (elapsedTime > mWaitTimer)
         {
            mWaitTimer=0.0f;
            setState(cStateWorking);
         }
         else
            mWaitTimer-=elapsedTime;
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::working(float elapsedTime, bool moving)
{
   BASSERT(mpOwner);
   BDEBUG_ASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit));
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//-- FIXING PREFIX BUG ID 3359
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return;

   // Temporary immobility
   if (pSquad->getFlagNonMobile())
   {
      pUnit->setAnimationRate(0.0f);
      return;
   }

   //If we cannot move, then fail.
   if (!pUnit->canMove())
   {
      setState(cStateFailed);
      return;
   }

   BActionState newState = cStateWorking;

   // A new move was probably commanded while this one was still moving so clear out current move and try again
   if (mRefreshPathData)
   {
      clearPathData();
      newState = createInitialPathData();
      mRefreshPathData = false;
   }

   //Follow movement.
   if (((mState == cStateWorking) || (mState == cStateStartMove) || (mState == cStateStopMove)) && (newState == cStateWorking))
   {
      if (pSquad->getFormationType() == BFormation2::eTypeGaggle)
         newState=followPathGaggle(elapsedTime);
      else
         newState=followPath(elapsedTime, moving);
   }

   //Handle state changes.
   if ((mState != cStateStartMove) && (mState != cStateStopMove) && (mState != cStateTurning) && (newState != cStateWorking))
      setState(newState);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::playTurnAnimation(long animType)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   // Get turn animation data
   BVisualAnimationData turnAnimationData = pUnit->getVisual()->getAnimationData(cActionAnimationTrack, animType);
   long turnAnimID = turnAnimationData.mAnimAsset.mIndex;
//-- FIXING PREFIX BUG ID 3360
   const BGrannyAnimation* pTurnGrannyAnim = gGrannyManager.getAnimation(turnAnimID);
//--

   // Get start animation data
   long unitMoveAnimType = pUnit->getMoveAnimType();
   long startAnimType;
   if (unitMoveAnimType == pUnit->getWalkAnim())
      startAnimType = cAnimTypeTurnWalk;
   else if (unitMoveAnimType == pUnit->getRunAnim())
      startAnimType = cAnimTypeTurnRun;
   else
      startAnimType = cAnimTypeTurnJog;
   BVisualAnimationData startAnimationData = pUnit->getVisual()->getAnimationData(cActionAnimationTrack, startAnimType);
   long startAnimID = startAnimationData.mAnimAsset.mIndex;
//-- FIXING PREFIX BUG ID 3361
   const BGrannyAnimation* pStartGrannyAnim = gGrannyManager.getAnimation(startAnimID);
//--

   // Will it collide with anything?
   BVector intersection;
   BVector estimatedEndPosition;
   BVector estimatedEndPosition2;
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);
   worldMatrix.transformVectorAsPoint(pTurnGrannyAnim->getTotalMotionExtraction(), estimatedEndPosition);
   worldMatrix.transformVector(pStartGrannyAnim->getTotalMotionExtraction(), estimatedEndPosition2);
   estimatedEndPosition += estimatedEndPosition2;
   if (pUnit->getNearestCollision(pUnit->getPosition(), estimatedEndPosition, true, true, false, intersection))
      return false;

   // If not, set the animation and return true
   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, animType, true, false, turnAnimID, true);
   pUnit->computeAnimation();
   BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateIdle, animType));
   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, startAnimType, true, false, startAnimID, true);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::playStartMoveAnimation()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   long moveAnimType = pUnit->getMoveAnimType();
   long animType;
   if (moveAnimType == pUnit->getWalkAnim())
      animType = cAnimTypeIdleWalk;
   else if (moveAnimType == pUnit->getRunAnim())
      animType = cAnimTypeIdleRun;
   else
      animType = cAnimTypeIdleJog;

   if (!pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, animType, true, false, -1, true, true))
      return false;
   pUnit->computeAnimation();
   BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateIdle, animType));
   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::playStopMoveAnimation()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   long moveAnimType = pUnit->getMoveAnimType();
   long animType;
   if (moveAnimType == pUnit->getWalkAnim())
      animType = cAnimTypeWalkIdle;
   else if (moveAnimType == pUnit->getRunAnim())
      animType = cAnimTypeRunIdle;
   else
      animType = cAnimTypeJogIdle;

   if (!pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, animType, true, false, -1, true, true))
      return false;
   pUnit->computeAnimation();
   BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateIdle, animType));
   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionMove::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));
   mTarget=target;
}

//==============================================================================
//==============================================================================
void BUnitActionMove::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }

   if (mOppID != oppID)
      mRefreshPathData = true;

   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionMove::getPriority() const
{
//-- FIXING PREFIX BUG ID 3362
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
void BUnitActionMove::debugRender()
{
   #ifdef RENDERGAGGLE
   //Draw the gaggle directions.
   if (mGaggleDirections[0].length())
      gTerrainSimRep.addDebugThickLineOverTerrain(mpOwner->getPosition(), mpOwner->getPosition()+mGaggleDirections[0]*3.0f, cDWORDWhite, cDWORDWhite, 0.5f, BDebugPrimitives::cCategoryPathing);
   if (mGaggleDirections[1].length())
      gTerrainSimRep.addDebugThickLineOverTerrain(mpOwner->getPosition(), mpOwner->getPosition()+mGaggleDirections[1]*3.0f, cDWORDYellow, cDWORDYellow, 0.5f, BDebugPrimitives::cCategoryPathing);
   if (mGaggleDirections[2].length())
      gTerrainSimRep.addDebugThickLineOverTerrain(mpOwner->getPosition(), mpOwner->getPosition()+mGaggleDirections[2]*3.0f, cDWORDOrange, cDWORDOrange, 0.5f, BDebugPrimitives::cCategoryPathing);
   if (mGaggleDirections[3].length())
      gTerrainSimRep.addDebugThickLineOverTerrain(mpOwner->getPosition(), mpOwner->getPosition()+mGaggleDirections[3]*3.0f, cDWORDGreen, cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryPathing);
   for (uint i=0; i < mGaggleObstructions.getSize(); i++)
      gTerrainSimRep.addDebugLineOverTerrain(mpOwner->getPosition(), mGaggleObstructions[i], cDWORDGreen, cDWORDGreen, 0.2f, BDebugPrimitives::cCategoryPathing);
   #endif
   #ifdef _MOVE4
   gTerrainSimRep.addDebugThickCircleOverTerrain(mTarget.getPosition(), 0.5f, 0.1f, cDWORDWhite, 0.75f, BDebugPrimitives::cCategoryPathing);
   #else
   BPathMoveData* pPathData = mPathMoveData;
   while (pPathData != NULL)
   {
      DWORD color = cDWORDBlack;
      switch (pPathData->mPathLevel)
      {
         case BPathMoveData::cPathLevelLow:
            color = cDWORDBlue;
            break;
         case BPathMoveData::cPathLevelMid:
            color = cDWORDYellow;
            break;
         default:
            color = cDWORDWhite;
            break;
      }
      pPathData->mPath.render(color, color, true);
      pPathData = pPathData->mLinkedPath;
   }
   #endif
}

//==============================================================================
//==============================================================================
void BUnitActionMove::drawDebugArrow(BVector arrowDirection, float height, DWORD color)
{
   BVector right;
   right.assignCrossProduct(BVector(0.0f, 1.0f, 0.0f), arrowDirection);
   BMatrix matrix;
   matrix.makeOrient(arrowDirection, BVector(0.0f, 1.0f, 0.0f), right);
   BVector position = mpOwner->getPosition();
   position += (arrowDirection * (mpOwner->getObstructionRadius() * 2.0f));
   position.y += height;
   matrix.multTranslate(position.x, position.y, position.z);
   gpDebugPrimitives->addDebugArrow(matrix, BVector(1.5f, 1.0f, 3.0f), color);
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BUnitActionMove::getDebugLine(uint index, BSimString &string) const
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3364
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);

   if (index < 2)
      return (BAction::getDebugLine(index, string));
   switch (index)
   {
      case 2:
      {
         string.format("MovePri=%d, Vel=%5.2f, DesVel=%5.2f, VelRandFactor=%5.2f, VelRandTime=%5.2f.", pSquad ? pSquad->getMovementPriority(pUnit->getID()) : 0,
            mVelocity, pUnit->getDesiredVelocity(), mVelocityRandomFactor, mVelocityRandomFactorTimer);
         break;
      }
   }
}
#endif

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::createInitialPathData()
{
   // Create initial high level path to target
   clearPathData();
   BPathMoveData* pPathData=BPathMoveData::getInstance();
   BDEBUG_ASSERT(pPathData);
   
   syncUnitActionCode("createInitialPathData");

   //Find out where we want to go.
   BVector targetPosition;
   float timeNeeded;
   if (!determineTargetPosition(targetPosition, timeNeeded))
   {
      syncUnitActionCode("determineTargetPosition failed");
      return (cStateFailed);
   }

   //Finish filling the pathdata in.
   pPathData->mPath.addWaypointAtStart(targetPosition);
   pPathData->mPathLevel = BPathMoveData::cPathLevelUser;
   pPathData->mCurrentWaypoint = 0;
   pPathData->mLinkedPath = NULL;
   pPathData->mPathTime = 0;                 // Set user level path time to 0 since all collisions should trigger repathing
   mPathMoveData = pPathData;

   return cStateWorking;
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::advancePathData()
{
   syncUnitActionCode("advancePathData");

   if (mPathMoveData == NULL)
   {
      syncUnitActionCode("mPathMoveData is null");
      return cStateDone;
   }

   BUnit* pUnit = mpOwner->getUnit();
   BDEBUG_ASSERT(pUnit);
   BVector currentPosition(pUnit->getPosition());

   // Go through paths and their waypoints and see if it needs to be advanced
   while (mPathMoveData != NULL)
   {
      bool advancedWaypoint = false;
//-- FIXING PREFIX BUG ID 3365
      const BEntity* pTarget = NULL;
//--
      bool reachedSquadTarget = true;

      syncUnitActionData("mPathMoveData->mPathLevel", mPathMoveData->mPathLevel);

      // If only user path remaining then update the waypoint to match where squad's says to go
      if (mPathMoveData->mPathLevel == BPathMoveData::cPathLevelUser)
      {
         BVector tempTargetPosition;
         float timeNeeded = -1.0f;
         if (!determineTargetPosition(tempTargetPosition, timeNeeded))
         {
            syncUnitActionCode("advancePathData -- determineTargetPosition failed");
            return (cStateFailed);
         }

         mPathMoveData->mPath.setWaypoint(mPathMoveData->mCurrentWaypoint, tempTargetPosition);

         // If more time is needed then aren't close enough to squad's final target to stop move action
         bool squadMove = mFlagSquadMove || (!mTarget.isIDValid() && !mTarget.isPositionValid());
         if (squadMove && (timeNeeded >= cFloatCompareEpsilon))
            reachedSquadTarget = false;
      }

      // See whether current position is close enough to current waypoint to move to the next
      if (mPathMoveData->mCurrentWaypoint < mPathMoveData->mPath.getNumberWaypoints())
      {
         // Calculate the remaining distance
         float distanceRemaining;
         if (pTarget != NULL)
            distanceRemaining=pUnit->calculateXZDistance(pTarget);
         else
            distanceRemaining=pUnit->getPosition().xzDistance(mPathMoveData->mPath.getWaypoint(mPathMoveData->mCurrentWaypoint));

         // Within range of waypoint.  Don't use target range for intermediate waypoints.  Only user level ones.
         float range=cFloatCompareEpsilon;
         if ((mPathMoveData->mPathLevel == BPathMoveData::cPathLevelUser) &&
            (mTarget.getID().isValid() || mTarget.isPositionValid()) &&
            mTarget.isRangeValid() &&
            !mFlagSquadMove)
            range=mTarget.getRange();
         if (distanceRemaining <= range)
         {
            mPathMoveData->mCurrentWaypoint++;
            advancedWaypoint = true;
         }
      }

      // Path advanced past last waypoint so remove this path
      if (mPathMoveData->mCurrentWaypoint >= mPathMoveData->mPath.getNumberWaypoints())
      {
         // If the end of the squad path hasn't been reached then don't delete user level path
         // because we're not done yet.  (This can happen if unit gets close to intermediate future
         // position like a path from one waypoint to another then back to the original waypoint.)
         if (!reachedSquadTarget && (mPathMoveData->mPathLevel == BPathMoveData::cPathLevelUser) && (mPathMoveData->mPath.getNumberWaypoints() > 0))
         {
            mPathMoveData->mCurrentWaypoint = mPathMoveData->mPath.getNumberWaypoints() - 1;
            break;
         }

         BPathMoveData* pPathData = mPathMoveData;
         mPathMoveData = pPathData->mLinkedPath;
         BPathMoveData::releaseInstance(pPathData);
      }
      // If neither waypoint or path advanced then nothing left to do so break out
      else if (!advancedWaypoint)
      {
         break;
      }
   }

   // Reached the end of the user path so done moving
   if (mPathMoveData == NULL)
      return cStateDone;

   // If low level path finished and there's anything but just a user path left, make sure another path is found
   if (mPathMoveData->mPathLevel != BPathMoveData::cPathLevelUser)
      return cStatePathing;

   return cStateWorking;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::attemptVectorPathToNextUserWaypoint(BEntityIDArray& ignoreUnits)
{
   // Find the user path
   BPathMoveData* pUserPathData = mPathMoveData;
   while ((pUserPathData != NULL) && (pUserPathData->mPathLevel != BPathMoveData::cPathLevelUser))
   {
      pUserPathData = pUserPathData->mLinkedPath;
   }

   // Check for collisions between current position and next user waypoint
   if ((pUserPathData != NULL) && (pUserPathData->mCurrentWaypoint < pUserPathData->mPath.getNumberWaypoints()))
   {
      BUnit* pUnit = mpOwner->getUnit();

      BVector targetPosition = pUserPathData->mPath.getWaypoint(pUserPathData->mCurrentWaypoint);
      static BEntityIDArray collisions;
      BSimCollisionResult collisionResult = pUnit->checkForCollisions(ignoreUnits, pUnit->getPosition(), targetPosition, pUserPathData->mPathTime, true, collisions);

      // If no collision found then just use user path
      if (collisionResult == cSimCollisionNone)
      {
         // Delete other paths
         while (mPathMoveData != pUserPathData)
         {
            BPathMoveData* pTempPathData = mPathMoveData;
            mPathMoveData = mPathMoveData->mLinkedPath;
            BPathMoveData::releaseInstance(pTempPathData);
         }

         // Can use user path so no other paths necessary
         return true;
      }
   }

   // Need mid or low level path to path around obstructions
   return false;
}

//==============================================================================
//==============================================================================
BUnitActionMove::BFindPathResult BUnitActionMove::findPath(BVector targetPosition, BPathLevel pathLevel, const BEntityIDArray& ignoreUnits)
{
//-- FIXING PREFIX BUG ID 3367
   const BUnit* pUnit = mpOwner->getUnit();
//--
   BDEBUG_ASSERT(pUnit);

   // Map path level to the type of path the pather can find
   int patherType = BPather::cUndefined;
   switch (pathLevel)
   {
      case BPathMoveData::cPathLevelMid:
         patherType = BPather::cLongRange;
         break;

      case BPathMoveData::cPathLevelLow:
      default:
         patherType = BPather::cShortRange;
         break;
   }

   BPathMoveData* pPathData = BPathMoveData::getInstance();

   // Get the path
   //Tommy: This feels like it's not right in terms of passing in a 0 pathing range.  Shouldn't
   //that be tied to the type of target and path we want?
   int result = gPather.findPath(pUnit->getID().asLong(), pUnit->getClassType(), &gObsManager,
      pUnit->getPosition(), targetPosition, pUnit->getObstructionRadius(), 0.0f,
      ignoreUnits, &(pPathData->mPath), false, false, pUnit->canJump(), -1L, patherType);

   // If we're in range then it might be necessary to advance the higher level path and try again.
   if (result == BPath::cInRangeAtStart)
   {
      BPathMoveData::releaseInstance(pPathData);
      return cFindPathInRangeAtStart;
   }

   // If that failed, bail now.
   if ((result == BPath::cFailed) || (pPathData->mPath.getNumberWaypoints() <= 0))
   {
      BPathMoveData::releaseInstance(pPathData);
      return cFindPathFailure;
   }

   // Insert path in list
   pPathData->mPathLevel = pathLevel;
   pPathData->mCurrentWaypoint = 0;
   pPathData->mPathTime = gWorld->getGametime();
   pPathData->mLinkedPath = mPathMoveData;
   mPathMoveData = pPathData;

   // Advance to next waypoint if already at first one
   if (mPathMoveData->mPath.getNumberWaypoints() >= 2)
   {
      BVector tempPosition = mPathMoveData->mPath.getWaypoint(0);
      if (tempPosition.almostEqualXZ(pUnit->getPosition()))
      {
         mPathMoveData->mCurrentWaypoint = 1;
      }
   }

   return cFindPathSuccess;
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::findPath()
{
   syncUnitActionData("BUnitActionMove::findPath owner ID", mpOwner->getID().asLong());

   static BEntityIDArray ignoreUnits;
   buildIgnoreList(ignoreUnits, false);

   BActionState actionState = cStateWorking;

   // Loop to find a path in case an error was encountered during a search and it needs to attempt the search again
   bool continuePathFinding = true;
   uint pathingAttempts = 0;
   while (continuePathFinding && (pathingAttempts < cMaxFindPathAttempts))
   {
      // Don't loop unless necessary
      continuePathFinding = false;
      pathingAttempts++;

      // Get target position.  Pointer and range checking should have been done by calling function.
      BVector targetPosition = mPathMoveData->mPath.getWaypoint(mPathMoveData->mCurrentWaypoint);

      BFindPathResult findPathResult = cFindPathSuccess;

      // Generate paths that are of lower levels than the current path.  All old paths have already been
      // removed from the path linked list at this point.  The current path should have a valid current waypoint too.
      switch (mPathMoveData->mPathLevel)
      {
         case BPathMoveData::cPathLevelUser:
         {
            // Need a mid and low level path or just a low level path based on how close unit is to target

            // Attempt just a short range path if close enough
//-- FIXING PREFIX BUG ID 3368
            const BUnit* pUnit = mpOwner->getUnit();
//--
            float distanceToTargetSqr = pUnit->calculateXZDistanceSqr(targetPosition);
            if (distanceToTargetSqr < cMaximumLowLevelDistSqr)
            {
               findPathResult = findPath(targetPosition, BPathMoveData::cPathLevelLow, ignoreUnits);
               if (findPathResult == cFindPathSuccess)
                  break;
            }

            // Find a long range path since short range failed or unit too far from target
            findPathResult = findPath(targetPosition, BPathMoveData::cPathLevelMid, ignoreUnits);

            // Generate short range path if mid level was found above
            if ((findPathResult == cFindPathSuccess) && (mPathMoveData->mPathLevel == BPathMoveData::cPathLevelMid))
            {
               targetPosition = mPathMoveData->mPath.getWaypoint(mPathMoveData->mCurrentWaypoint);
               findPathResult = findPath(targetPosition, BPathMoveData::cPathLevelLow, ignoreUnits);
            }

            break;
         }


         case BPathMoveData::cPathLevelMid:
         {
            // Need a low level path

            findPathResult = findPath(targetPosition, BPathMoveData::cPathLevelLow, ignoreUnits);
            break;
         }


         case BPathMoveData::cPathLevelLow:
         {
            // Bad low level path?

            // Remove old low level path
            BPathMoveData* pOldLowPathData = mPathMoveData;
            mPathMoveData = mPathMoveData->mLinkedPath;
            BPathMoveData::releaseInstance(pOldLowPathData);

            // Should always be a user path but check anyway
            if (mPathMoveData == NULL)
               return cStateFailed;

            // Get new target position
            targetPosition = mPathMoveData->mPath.getWaypoint(mPathMoveData->mCurrentWaypoint);

            findPathResult = findPath(targetPosition, BPathMoveData::cPathLevelLow, ignoreUnits);

            break;
         }
      }

      // Failed to find path because current path near a waypoint.  Advance the current path and try to
      // find a lower level path again.
      if (findPathResult == cFindPathInRangeAtStart)
      {
         BActionState tempActionState = advancePathData();
         if (tempActionState == cStateFailed)
            actionState = cStateFailed;
         else
            continuePathFinding = true;
      }

      // Failed to find path
      else if (findPathResult == cFindPathFailure)
      {
         actionState = cStateFailed;
      }
   }

   // If pathing attemps exceeded return failure
   if (continuePathFinding && (pathingAttempts >= cMaxFindPathAttempts))
      actionState = cStateFailed;

   syncUnitActionData("BUnitActionMove::findPath new actionState", actionState);

   return actionState;
}

//==============================================================================
//==============================================================================
float BUnitActionMove::calcDistanceRemaining(BUnit* pUnit, BEntity* pTarget, BVector targetPosition)
{
   float distanceRemaining = 0.0f;

   // If on a unit path then use obstruction radius to move to next waypoint sooner.
   if ((mPathMoveData != NULL) && (mPathMoveData->mPathLevel != BPathMoveData::cPathLevelUser))
   {
      if (pTarget != NULL)
         distanceRemaining = pUnit->calculateXZDistance(pTarget);
      else
         distanceRemaining = pUnit->calculateXZDistance(targetPosition);
   }
   // Else want to hit the exact target so do a point to point distance check
   else
   {
      // Check against unit radius
      if (pTarget != NULL)
         distanceRemaining = pUnit->calculateXZDistance(pTarget);
      else
         distanceRemaining = pUnit->getPosition().xzDistance(targetPosition);
   }
   return distanceRemaining;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::calcTurning(float elapsedTime, float distanceChange, float distanceRemainingToTarget,
   BVector& newPosition, BVector& newForward)
{
   // newPosition and newForward are already set to desired values.
   // This function adjusts them if the turn is too great.

   BUnit* pUnit = mpOwner->getUnit();
   BDEBUG_ASSERT(pUnit && pUnit->getProtoObject());

   // Smoothly rotate towards the requested angle
   float unitAngle = pUnit->getForward().getAngleAroundY();
   float reqAngle = newForward.getAngleAroundY();
   float angleDiff = reqAngle - unitAngle;
   pUnit->setFlagTurning((angleDiff >= XMConvertToRadians(5.0f)));
   if (angleDiff >= -cFloatCompareEpsilon && angleDiff <= cFloatCompareEpsilon)
   {
      // If the difference between the angles is almost zero, don't need to adjust forward vector.
      return false;
   }

   // Get turn rate
   float turnSpeed = pUnit->getProtoObject()->getTurnRate() * cRadiansPerDegree;
   if (turnSpeed < cFloatCompareEpsilon)
      return false;

   if (angleDiff > cPi)
      angleDiff = -(cPi + cPi - angleDiff);
   else if (angleDiff < -cPi)
      angleDiff = cPi + cPi + angleDiff;

   // ajl 1/9/02 - Yaw at a constant speed. The old way would yaw much 
   // slower as the angle difference got smaller. I tried to keep some
   // of the nice smoothness that the old way had by slowing the turn
   // speed down a little when the angle difference is less than 45 degrees.
   float yawAmount;
   float absDiff = (float) fabs(angleDiff);
   if (absDiff < cPi * 0.03125)
      turnSpeed *= 0.125f;
   else if (absDiff < cPi * 0.0625)
      turnSpeed *= 0.25f;
   else if (absDiff < cPiOver8)
      turnSpeed *= 0.50f;
   else if (absDiff < cPiOver4)
      turnSpeed *= 0.75f;

   float maxTurn = turnSpeed * elapsedTime;
   if (absDiff > maxTurn)
   {
      if (angleDiff < 0.0f)
         yawAmount = -maxTurn;
      else
         yawAmount = maxTurn;
   }
   else
   {
      //yawAmount = angleDiff;
      // Turn isn't too large so don't need to adjust forward vector.
      return false;
   }

   // New forward
   BMatrix tempMatrix;
   tempMatrix.makeRotateArbitrary(yawAmount, pUnit->getUp());
   tempMatrix.transformVector(pUnit->getForward(), newForward);
   newForward.normalize();

   // Limit distanceChange based on turn radius
//-- FIXING PREFIX BUG ID 3371
   const BSquad* pParentSquad = pUnit->getParentSquad();
//--
   if (pParentSquad)
   {
      const BProtoSquad* pPS = pParentSquad->getProtoSquad();
      if (pPS && pPS->hasTurnRadius())
      {
         float minTurnRadius = pPS->getTurnRadiusMin();
         //float maxTurnRadius = pPS->getTurnRadiusMax();
         float minVel = minTurnRadius * turnSpeed;
         float maxVel = (elapsedTime > cFloatCompareEpsilon) ? (distanceChange / elapsedTime) : 100.0f;
         const float cutoffAngle = cPiOver2;
         float interpFactor = Math::Min(absDiff / cutoffAngle, 1.0f);
         float newVel = Math::Lerp(maxVel, minVel, interpFactor);
         distanceChange = newVel * elapsedTime;
      }
   }

   // Scale distance to move by how long it will take to complete the turn
   if ((distanceRemainingToTarget > cFloatCompareEpsilon) && (absDiff > maxTurn))
   {
      // Decrease clamp distance by some factor since moving forward while turning may increase
      // total turn than if rotating in place
      float clampDist = distanceRemainingToTarget * maxTurn / absDiff * 0.5f;
      if (clampDist < distanceChange)
         distanceChange = clampDist;
   }

   // Determine position in new direction
   newPosition = pUnit->getPosition();
   newPosition += (newForward * distanceChange);

   // If pathing around obstruction then see if tighter turning needed
   if ((mPathMoveData != NULL) && (mPathMoveData->mPathLevel != BPathMoveData::cPathLevelUser))
   {
      static BEntityIDArray ignoreUnits;
      static BEntityIDArray collisions;
      BVector intersectionPoint;
      buildIgnoreList(ignoreUnits, false);

      BSimCollisionResult collisionResult = pUnit->checkForCollisions(ignoreUnits, pUnit->getPosition(), newPosition, mPathMoveData->mPathTime, true, collisions, &intersectionPoint);
      if (collisionResult != cSimCollisionNone)
      {
         newPosition = intersectionPoint;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::calcMovement(float elapsedTime, BVector& newPosition,
   BVector& newForward, BVector& unlimitedNewForward)
{
   syncUnitActionData("BUnitActionMove::calcMovement -- elapsedTime", elapsedTime);
   
   // NOTE: If this ever returns false, the movement data is trashed/malformed.
   BUnit* pUnit = mpOwner->getUnit();
   BDEBUG_ASSERT(pUnit);
   syncUnitActionData("BUnitActionMove::calcMovement -- unitID", pUnit->getID());

//-- FIXING PREFIX BUG ID 3372
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
   {
      syncUnitActionCode("BUnitActionMove::calcMovement -- no squad");
      return cStateFailed;
   }
   syncUnitActionData("BUnitActionMove::calcMovement -- squadID", pSquad->getID());
   
   newPosition = pUnit->getPosition();
   newForward = pUnit->getForward();
   unlimitedNewForward=newForward;

   // In Move4, just move towards our target.  FTLOG that's all unitActionMove needs
   // to do anyway.
   #ifdef _MOVE4
   syncUnitActionData("BUnitActionMove::calcMovement -- mFlagSquadMove", mFlagSquadMove);
   if (mFlagSquadMove)
   {
      BVector destination = mTarget.getPosition();
      syncUnitActionData("BUnitActionMove::calcMovement -- destination/target position", destination);
      syncUnitActionData("BUnitActionMove::calcMovement -- target range valid", mTarget.isRangeValid());
      
      // So now we're worrying about targets.  Really.  
      if (mTarget.isRangeValid())
      {
         // Range checking is done squad to squad.
         // For this check to be completely accurate we would need to have the proto action for the melee and min range
         // values.  However the move action doesn't have the proper attack proto action set so we'll use default
         // values instead.  This should be ok though.
         syncUnitActionData("BUnitActionMove::calcMovement -- target range", mTarget.getRange());
         if (pSquad->isChildInRange(false, pUnit->getID(), mTarget, 0.0f, mTarget.getRange(), 0.0f))
         {
            syncUnitActionCode("BUnitActionMove::calcMovement -- child in range");
            return cStateDone;
         }
      }
      if (destination != cInvalidVector)
      {
         BVector direction = destination - pUnit->getPosition();
         direction.y = 0;
         float distanceRemaining = direction.length();

         syncUnitActionData("BUnitActionMove::calcMovement -- direction", direction);
         syncUnitActionData("BUnitActionMove::calcMovement -- distance remaining", distanceRemaining);

         if (distanceRemaining < cActionCompleteEpsilon)
         {
            // If we caught up to the squad, see if it's done.  If so, then we can be done.
            // if not, go into pathing state (which is, in this case, really just a "retry" state)
            if (pSquad->hasCompletedMovement_4())
            {
               syncUnitActionCode("BUnitActionMove::calcMovement -- squad has completed movement");
               return cStateDone;
            }
            else
            {
               syncUnitActionCode("BUnitActionMove::calcMovement -- squad has not completed movement");
               return cStatePathing;
            }
         }
         // Reset retry attempts anytime we successfully move.
         mRetryAttempts_4 = 0;
         direction.safeNormalize();

         // Get the squad's velocity as our velocity
//-- FIXING PREFIX BUG ID 3374
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (!pSquad)
            return cStateFailed;
         const BSquadActionMove* pAction = pSquad->getNonPausedMoveAction_4();
         float velocity = pUnit->getDesiredVelocity();
         if (pSquad->isMoving() && pAction)
            velocity = pAction->getVelocity();
         
         float positionChangeScalar = elapsedTime * velocity;
         // Any time the positionChangeScalar is greater than the distance remaining, snap to the target position.
         if (positionChangeScalar > distanceRemaining)
         {
            positionChangeScalar = distanceRemaining;
         }
         newPosition = pUnit->getPosition() + (direction * positionChangeScalar);
         newForward = direction;
         //  What the hell is this?
         unlimitedNewForward = direction;

         // Adjust the position and direction based on the turn rate
         calcTurning(elapsedTime, positionChangeScalar, distanceRemaining, newPosition, newForward);

         return cStateWorking;
      }
   }
   #endif

   //Check our target distance if we have a valid range.
   if (!mFlagSquadMove && mTarget.isRangeValid())
   {
      float distanceToTarget;
      if (mTarget.getID().isValid())
      {
//-- FIXING PREFIX BUG ID 3373
         const BEntity* pTarget=gWorld->getEntity(mTarget.getID());
//--
         if (!pTarget)
            return (cStateFailed);
         distanceToTarget=pUnit->calculateXZDistance(pTarget);
         //See if we're done.      
         if (distanceToTarget <= mTarget.getRange())
            return (cStateDone);
      }
      else if (mTarget.isPositionValid())
      {
         distanceToTarget=pUnit->getPosition().xzDistance(mTarget.getPosition());
         //See if we're done.      
         if (distanceToTarget <= mTarget.getRange())
            return (cStateDone);
      }
   }

   // This already checked by caller for validity
   BPathMoveData *pPathData = mPathMoveData;
   BDEBUG_ASSERT(pPathData);

   BVector targetPosition(0.0f);
   float distanceRemaining = 0.0f;
   BActionState newState = cStateWorking;
   BEntity* pTarget = NULL;

   syncUnitActionData("BUnitActionMove::calcMovement -- pPathData->mPathLevel", pPathData->mPathLevel);
   syncUnitActionData("BUnitActionMove::calcMovement -- pPathData->mCurrentWaypoint", pPathData->mCurrentWaypoint);

   // Validate path data
   if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
   {
      syncUnitActionCode("BUnitActionMove::calcMovement -- pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints()");
      return cStateFailed;
   }

   // If doing vector movement then update the waypoint to match where squad's says to go
   float velocityToFutureTarget =pUnit->getDesiredVelocity();
   if (pPathData->mPathLevel == BPathMoveData::cPathLevelUser)
   {
      syncUnitActionCode("BUnitActionMove::calcMovement -- user level path");
   
      // NOTE:
      // User level path can only have one waypoint now.  If this needs to change then need
      // to fix updating of user level path with updated target position.
      BDEBUG_ASSERT(pPathData->mPath.getNumberWaypoints() <= 1);

      //Get our target position to move to.
      BVector tempTargetPosition;
      float timeNeeded;
      if (!determineTargetPosition(tempTargetPosition, timeNeeded))
         return (cStateFailed);

      pPathData->mPath.setWaypoint(pPathData->mCurrentWaypoint, tempTargetPosition);

      // If squad returned a valid time to future position, use it to calculate the velocity needed
      // to reach the future position in time.
      if (timeNeeded > cFloatCompareEpsilon)
      {
         float distToFutureTarget = tempTargetPosition.xzDistance(pUnit->getPosition());
         velocityToFutureTarget = distToFutureTarget / timeNeeded;
      }
   }
   //Else, if we are moving for our squad, speed up if it's going fast.  This is a "guesstimate"
   //calculation.  We may have to make it better.
   else if (mFlagSquadMove)
   {
      syncUnitActionData("BUnitActionMove::calcMovement -- squad move, using action velocity", pSquad->getActionVelocity());
      velocityToFutureTarget=pSquad->getActionVelocity();
   }

   targetPosition = pPathData->mPath.getWaypoint(pPathData->mCurrentWaypoint);
   syncUnitActionData("BUnitActionMove::calcMovement -- targetPosition", targetPosition);

   // Calculate the remaining distance
   distanceRemaining = calcDistanceRemaining(pUnit, pTarget, targetPosition);
   syncUnitActionData("BUnitActionMove::calcMovement -- distanceRemaining", distanceRemaining);
   
   // Handle the case where we're at the next waypoint.
   if (distanceRemaining <= cFloatCompareEpsilon)
   {
      syncUnitActionCode("BUnitActionMove::calcMovement -- at waypoint");

      pPathData->mCurrentWaypoint++;
      if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
      {
         // Reached end of current path.  If user level path then no position change needed because close
         // enough to target.  If not user level path then set to target position to get unit past obstructions.
         if (pPathData->mPathLevel != BPathMoveData::cPathLevelUser)
         {
            BVector direction = targetPosition - pUnit->getPosition();
            direction.y = 0;
            syncUnitActionData("BUnitActionMove::calcMovement -- direction", direction);
            if (direction.safeNormalize())
            {
               newForward = direction;
               unlimitedNewForward=newForward;
               syncUnitActionData("BUnitActionMove::calcMovement -- newForward", newForward);
               syncUnitActionData("BUnitActionMove::calcMovement -- unlimitedNewForward", unlimitedNewForward);
            }
            newPosition = targetPosition;
            syncUnitActionData("BUnitActionMove::calcMovement -- newPosition", newPosition);
         }
         return (advancePathData());
      }

      targetPosition = pPathData->mPath.getWaypoint(pPathData->mCurrentWaypoint);
      distanceRemaining = calcDistanceRemaining(pUnit, pTarget, targetPosition);

      syncUnitActionData("BUnitActionMove::calcMovement -- targetPosition", targetPosition);
      syncUnitActionData("BUnitActionMove::calcMovement -- distanceRemaining", distanceRemaining);
   }

   //Bound our desired velocity to what we can actually do.
   mVelocity=Math::Min(velocityToFutureTarget, pUnit->getMaxVelocity());
   syncUnitActionData("BUnitActionMove::calcMovement -- mVelocity", mVelocity);

   // Calculate our movement direction.  If the resulting vector is invalid (which
   // shouldn't happen unless the path is malformed), just bail.
   BVector direction = targetPosition - pUnit->getPosition();
   direction.y = 0;
   if (!direction.safeNormalize())
      return cStateFailed;

   syncUnitActionData("BUnitActionMove::calcMovement -- direction", direction);

   // Calculate our position change.  If we wrap past the current waypoint, go to the next one
   // if we have one (with the funky position stuff that entails).  If we don't have one, still
   // do the move, but we're done after that.
   float positionChangeScalar = mVelocity * elapsedTime;
   BVector positionChange = direction * positionChangeScalar;
   syncUnitActionData("BUnitActionMove::calcMovement -- positionChange", positionChange);
   float distanceChange = positionChange.length();
   if (distanceRemaining < 0.0f)
      distanceRemaining = 0.0f;
   if (distanceChange > distanceRemaining)
   {
      pPathData->mCurrentWaypoint++;
      if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
      {
         syncUnitActionData("BUnitActionMove::calcMovement -- reached end of current path, dist remaining", distanceRemaining);
         
         // Reached end of current path so advance the path data and see if move is done
         if (distanceRemaining > cFloatCompareEpsilon)
            positionChangeScalar = distanceRemaining;
         else
            positionChangeScalar = 0.0f;

         newState = advancePathData();
      }
      else
      {
         // Track the distance we need to wrap.
         float wrapDistance = distanceChange - distanceRemaining;
         syncUnitActionData("BUnitActionMove::calcMovement -- wrapDistance", wrapDistance);

         // Redo the direction calc using the waypoint we're wrapping past as the current position.
         newPosition = targetPosition;
         targetPosition = pPathData->mPath.getWaypoint(pPathData->mCurrentWaypoint);
         direction = targetPosition - newPosition;
         direction.y = 0;
         if (!direction.safeNormalize())
            return cStateFailed;
         positionChangeScalar = wrapDistance;
         syncUnitActionData("BUnitActionMove::calcMovement -- positionChangeScalar", positionChangeScalar);
      }
   }

   // Calculate the new position
   newPosition = pUnit->getPosition();
   positionChange = direction * positionChangeScalar;
   newPosition += positionChange;
   newForward = direction;
   unlimitedNewForward=newForward;

   syncUnitActionData("BUnitActionMove::calcMovement -- newPosition", newPosition);
   syncUnitActionData("BUnitActionMove::calcMovement -- newForward", newForward);
   syncUnitActionData("BUnitActionMove::calcMovement -- unlimitedNewForward", unlimitedNewForward);

   // Adjust the position and direction based on the turn rate
   calcTurning(elapsedTime, positionChangeScalar, distanceRemaining, newPosition, newForward);

   syncUnitActionData("BUnitActionMove::calcMovement -- returning newState", newState);

   return newState;
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::calcMovementGaggle(float elapsedTime, BVector& newPosition,
   BVector& newForward, BVector& unlimitedNewForward)
{
   //Prep.
//-- FIXING PREFIX BUG ID 3376
   const BUnit* pUnit = mpOwner->getUnit();
//--
   BDEBUG_ASSERT(pUnit);
   BPathMoveData *pPathData = mPathMoveData;
   BDEBUG_ASSERT(pPathData);

   //We only go to a point; this doesn't follow paths.
   if ((pPathData->mPathLevel != BPathMoveData::cPathLevelUser) ||
      (pPathData->mPath.getNumberWaypoints() < 1))
      return (cStateFailed);
   //Validate path data.
   if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
      return (cStateFailed);
   
   //Default everything.
   newPosition=pUnit->getPosition();
   newForward=pUnit->getForward();
   unlimitedNewForward=newForward;

   //Get our target position to move to.  We explicitly do not stop if we've reached
   //the target because that's just not what Gaggle does.
   BVector targetPosition;
   float timeNeeded;
   float distanceToTarget;
   if (!determineTargetPosition2(0.1f, targetPosition, distanceToTarget, timeNeeded))
      return (cStateFailed);

   //Now that we know where we want to go, let's do the gaggle stuff.  This will almost
   //certainly change targetPosition and desiredVelocity, so any values calculated above
   //relative to those may be wrong after this call.
   float desiredVelocity=pUnit->getDesiredVelocity();
   calcGaggle(elapsedTime, targetPosition, desiredVelocity);

   //TargetPosition has now been "gaggled", so just execute it as best as possible.
   //Set the waypoint for grins.
   pPathData->mPath.setWaypoint(pPathData->mCurrentWaypoint, targetPosition);

   //Calculate our movement direction.  If the resulting vector is invalid, just bail.
   BVector direction=targetPosition-pUnit->getPosition();
   direction.y=0.0f;
   float distToTargetPos = direction.length();

   // jce [10/14/2008] -- it didn't seem right that you would fail if you caught up with your flock position so I'm just leaving the guy untouched
   // and returning cStateWorking
   if(distToTargetPos < cFloatCompareEpsilon)
      return(cStateWorking);
   //if (!direction.safeNormalize())
     // return (cStateFailed);
   
   // Normalize.
   direction /= distToTargetPos;

   //Save the unlimited direction.
   unlimitedNewForward=direction;

   // jce [10/14/2008] -- compute a correction factor based on how far out of position we are.  The distances are picked by experiment.  
   // At max distance, there will be no correction.  At min distance, there will be normal correction.
   const float cDistForNoCorrection = 25.0f;
   const float cDistForMinCorrection = 5.0f;
   float correctionFactor;
   if(distToTargetPos <= cDistForMinCorrection)
      correctionFactor = 1.0f;
   else if(distToTargetPos >= cDistForNoCorrection)
      correctionFactor = 0.0f;
   else
      correctionFactor = 1.0f - (distToTargetPos-cDistForMinCorrection)/(cDistForNoCorrection-cDistForMinCorrection);
      
   //Limit the turn.
   limitTurn(elapsedTime, pUnit->getForward(), pUnit->getUp(), direction, correctionFactor);
   
   //Limit the velocity change.
   limitVelocity(elapsedTime, desiredVelocity, mVelocity);
   
   //Calculate our position change.
   float distanceToMove=mVelocity*elapsedTime;
   BVector positionChange=direction*distanceToMove;
   
   //Set the variables.
   newPosition=pUnit->getPosition()+positionChange;
   newForward=direction;

   //Return Working since we're still going.
   return (cStateWorking);
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::followPath(float elapsedTime, bool moving)
{
   // Make sure there is valid path data
   if (mPathMoveData == NULL)
      return cStateFailed;

   static BEntityIDArray ignoreUnits;
   BUnit* pUnit = mpOwner->getUnit();
   BDEBUG_ASSERT(pUnit);

   //TEMP.
   bool turning=gConfig.isDefined(cConfigTurning);

   // Calculate new position based on current pathing method
   BVector newPosition;
   BVector newForward;
   BVector unlimitedNewForward;
   BActionState actionState=calcMovement(elapsedTime, newPosition, newForward, unlimitedNewForward);

   // Done or couldn't calculate movement
   if ((actionState != cStateWorking) && (actionState != cStatePathing))
   {
      // Make sure final position got set
      if (actionState == cStateDone)
         advanceToNewPosition(elapsedTime, newPosition, newForward);

      return actionState;
   }

   //Turn/Stop/Start detection
   syncUnitActionData("followPath: turning", turning);
   syncUnitActionData("followPath: mHasTurnAnimation", mHasTurnAnimation);
   syncUnitActionData("followPath: animationState", pUnit->getAnimationState());
   syncUnitActionData("followPath: animationLocked", pUnit->isAnimationLocked());
   syncUnitActionData("followPath: isControllerFree", pUnit->isControllerFree(BActionController::cControllerAnimation));
   if (turning && (mState == cStateWorking) && mHasTurnAnimation && (pUnit->getAnimationState() == BObjectAnimationState::cAnimationStateIdle) && !pUnit->isAnimationLocked() && pUnit->isControllerFree(BActionController::cControllerAnimation))
   {
//-- FIXING PREFIX BUG ID 3377
      const BSquad* pSquad = pUnit->getParentSquad();
//--
      BDEBUG_ASSERT(pSquad);
      if (!pSquad)
      {
         syncUnitActionCode("No squad!");
         return cStateFailed;
      }
      const float projectionTime = pSquad->getDefaultMovementProjectionTime();
      BVector position = pUnit->getPosition();
      BVector targetPosition;
      float timeToTarget;

      syncUnitActionCode("followPath: determineTargetPosition");
      if (determineTargetPosition(targetPosition, timeToTarget))
      {
         if (timeToTarget >= projectionTime)
         {
            // Turning
            BVector forward = pUnit->getForward();
            BVector desiredForward;
            desiredForward.assignDifference(targetPosition, position);
            forward.y = 0.0f;
            desiredForward.y = 0.0f;
            bool failedTurn = false;
            if (forward.safeNormalize() && desiredForward.safeNormalize())
            {
               float turnAngle = (float)fabs(desiredForward.angleBetweenVector(forward));
               if (turnAngle > (cRadiansPerDegree * 22.5f))
               {
                  if (turnAngle > (cRadiansPerDegree * 157.5f))
                  {
                     if (playTurnAnimation(cAnimTypeTurnAround))
                        return (cStateTurning);
                     failedTurn = true;
                  }
                  else
                  {
                     BVector right = pUnit->getRight();
                     right.y = 0.0f;
                     if (right.safeNormalize())
                     {
                        long animType = -1;
                        if (desiredForward.dot(right) >= 0.0f)
                        {
                           if (turnAngle < (cRadiansPerDegree * 67.5f))
                              animType = cAnimTypeTurnRight45Forward; // 45 right
                           else if (turnAngle > (cRadiansPerDegree * 112.5))
                              animType = cAnimTypeTurnRight45Back; // 135 right
                           else
                              animType = cAnimTypeTurnRight; // 90 right
                        }
                        else
                        {
                           if (turnAngle < (cRadiansPerDegree * 67.5f))
                              animType = cAnimTypeTurnLeft45Forward; // 45 left
                           else if (turnAngle > (cRadiansPerDegree * 112.5))
                              animType = cAnimTypeTurnLeft45Back; // 135 left
                           else
                              animType = cAnimTypeTurnLeft; // 90 left
                        }

                        if (playTurnAnimation(animType))
                           return (cStateTurning);
                        failedTurn = true;
                     }
                  }
               }
            }

            // Start
            if (!moving && !failedTurn)
            {
               if (playStartMoveAnimation())
                  return (cStateStartMove);
            }
         }
         else if (moving && (timeToTarget < (projectionTime - 0.7f)))
         {
            // Stopping
            if (playStopMoveAnimation())
               return (cStateStopMove);
         }
      }
   }

   // Check for obstructions
   #if (!defined(_MOVE4) || defined(ENABLE_UNIT_COLLISIONS))
   BSimCollisionResult collisionResult = cSimCollisionNone;
   #endif

   if (actionState != cStatePathing)
   {
      // Unit collisions disabled for new movement.  May need to change this to only do collision checks for certain unit types.
      #ifdef ENABLE_UNIT_COLLISIONS
         buildIgnoreList(ignoreUnits, false);
         static BEntityIDArray collisions;
         collisionResult = pUnit->checkForCollisions(ignoreUnits, pUnit->getPosition(), newPosition, mPathMoveData->mPathTime, true, collisions);

         // Ignore terrain obstructions if on low level path since terrain is static and path get around it.  Collision was
         // probably just a floating point precision issue.
         if ((collisionResult == cSimCollisionTerrain) && (mPathMoveData != NULL) && (mPathMoveData->mPathLevel == BPathMoveData::cPathLevelLow))
            collisionResult = cSimCollisionNone;
      #endif
   }

   // jce [5/5/2008] -- Not in _MOVE4, else badness
   #ifndef _MOVE4
   // Repath around obstructions or because path ended
   if ((actionState == cStatePathing) || (collisionResult != cSimCollisionNone))
   {
      // Quick check to see if no obstructions between current position and next user waypoint.
      // If there aren't any then just vector path to it and remove other paths.
      if (actionState == cStatePathing)
      {
         buildIgnoreList(ignoreUnits, false);
         if (attemptVectorPathToNextUserWaypoint(ignoreUnits))
            return cStateWorking;
      }

      // Find new path
      actionState = findPath();
      if (actionState != cStateWorking)
         return actionState;

      // Calculate new position based on new pathing method
      actionState = calcMovement(elapsedTime, newPosition, newForward, unlimitedNewForward);
      if (actionState == cStateFailed)
         return actionState;
   }
   #endif
   
   // Set the new position
   advanceToNewPosition(elapsedTime, newPosition, newForward);

   return cStateWorking;
}

//==============================================================================
//==============================================================================
BActionState BUnitActionMove::followPathGaggle(float elapsedTime)
{
   //Make sure there is valid path data.
   if (!mPathMoveData)
      return (cStateFailed);
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3379
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return cStateFailed;

   //See about our wait.
   mWaitTimer+=elapsedTime;
   if (!pSquad->isMoving() && (mWaitTimer > 2.0f))
   {
      if (getRand(cSimRand)%500 == 0)
         return (cStateWait);
   }

   //Get the gaggle move info.
   BVector newPosition;
   BVector newForward;
   BVector unlimitedNewForward;
   BActionState actionState=calcMovementGaggle(elapsedTime, newPosition, newForward, unlimitedNewForward);

   //Gaggle movement should either return Failed or Working.  Failed is a catastrophic error.
   BDEBUG_ASSERT((actionState == cStateWorking) || (actionState == cStateFailed));
   if (actionState != cStateWorking)
      return (cStateFailed);

   // TRB 6/24/08 - Determne the type of target.  Kind of a hack to keep flood outside of large target obstructions
   // like buildings while allowing them to get close to infantry to attach to them.
   // jce [10/14/2008] -- nuking this since it does nothing but waste cycles given the fact that the code below it was already commented out.
   /*
   bool addTargetToIgnoreList = true;
   if (mTarget.isIDValid())
   {
//-- FIXING PREFIX BUG ID 3378
      const BEntity* pEntity = gWorld->getEntity(mTarget.getID());
//--
      if (pEntity && pEntity->getProtoObject() && pEntity->getProtoObject()->isType(gDatabase.getOTIDBuilding()))
         addTargetToIgnoreList = false;
   }
   */

   // 16 JUL 08 - BSR: Disabling the following clause since according to Dusty all pathing obstruction checks should be happening
   // in SquadActionMove. This fixes a problem with infection forms that get stuck while their squad is free to move.
/*
   //Check for any obstructions.
   static BObstructionNodePtrArray collisionObs;
   collisionObs.clear();
   static BEntityIDArray ignoreUnits;
   buildIgnoreList(ignoreUnits, addTargetToIgnoreList);
   BVector tempIntersectionPoint(0.0f);
   //Actually do the collision check.  Check any terrain or non-moving units.
   long lLandOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockLandUnits;

   long lFloodOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockFloodUnits;

   long lScarabOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockScarabUnits;

   long lObOptions = lLandOptions;
   if (pUnit->getProtoObject()->getMovementType() == cMovementTypeFlood)
      lObOptions = lFloodOptions;
   else if (pUnit->getProtoObject()->getMovementType() == cMovementTypeScarab)
      lObOptions = lScarabOptions;

   long lObNodeType=BObstructionManager::cObsNodeTypeAll;
   gObsManager.begin(BObstructionManager::cBeginEntity, pUnit->getID().asLong(),
      pUnit->getClassType(), lObOptions, lObNodeType, 0, cDefaultRadiusSofteningFactor,
      &ignoreUnits, pUnit->canJump());
   bool collision=gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections,
      pUnit->getPosition(), newPosition, true, tempIntersectionPoint, collisionObs);
   gObsManager.end();

   //If we're going to run into anything, see if we can attack someone from the same squad
   //as our target.  If not, wait for a bit.
   if (collision || (collisionObs.getSize() > 0))
   {
      //See if we have a melee range attack.
      BAction* pAttackAction=pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack);
      if (!pAttackAction || !pAttackAction->getTarget())
         return (cStateWait);
      if (!pAttackAction->getProtoAction() || !pAttackAction->getProtoAction()->getMeleeRange())
         return (cStateWait);
   
      //If we do, get the squad of the target.
      BUnit* pTargetUnit=gWorld->getUnit(pAttackAction->getTarget()->getID());
      if (!pTargetUnit)
         return (cStateWait);
      
      //Now, go through the obstructions we're about to hit to see if any of them are in the same squad.
      //If so, reset our attack target to the thing we just ran into.
      for (uint i=0; i < collisionObs.getSize(); i++)
      {
         BOPObstructionNode* pObstructionNode=collisionObs[i];
         if (!pObstructionNode)
            continue;
         BEntity* pObstructionEntity=pObstructionNode->mObject;
         if (pObstructionEntity && (pObstructionEntity->getClassType() == BEntity::cClassTypeUnit))
         {
            if (pObstructionEntity->getParentID() != pTargetUnit->getParentID())
               continue;
            BSimTarget newTarget(pObstructionEntity->getID(), mTarget.getRange());
            setTarget(newTarget);
            pAttackAction->setTarget(newTarget);
            break;
         }
      }
      
      //Now wait.
      return (cStateWait);
   }
*/

   //Set the new position.
   advanceToNewPosition(elapsedTime, newPosition, newForward);

   return cStateWorking;
}

//==============================================================================
//==============================================================================
void BUnitActionMove::advanceToNewPosition(float elapsedTime, BVector newPosition, BVector newForward)
{
   BUnit* pUnit = mpOwner->getUnit();
   //BVisual* pVisual = pUnit->getVisual();

   //-- Store the old movement data
   BPrePost prePostInfo(*pUnit);
   prePostInfo.prePositionChanged();


   // Set direction if we have the orient controller.
   //if (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID)
   //{
   //pUnit->debug("MoveANP:: setting newFor=(%6.2f, %6.2f).", newForward.x, newForward.z);
      pUnit->setForward(newForward);
      pUnit->calcRight();
      pUnit->calcUp();
   //}
   syncUnitActionData("BUnitActionMove::advanceToNewPosition forward", pUnit->getForward());
   syncUnitActionData("BUnitActionMove::advanceToNewPosition up", pUnit->getUp());
   syncUnitActionData("BUnitActionMove::advanceToNewPosition right", pUnit->getRight());

   if ((mState == cStateStartMove) || (mState == cStateStopMove))
      return;

   BSimHelper::advanceToNewPosition(pUnit, elapsedTime, newPosition, newForward);

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionMove::advanceToNewPosition", newPosition);
   #endif

   //-- notify that the movement is complete
   prePostInfo.postPositionChanged();

   syncUnitActionData("BUnitActionMove::advanceToNewPosition position", pUnit->getPosition());
}

//==============================================================================
//==============================================================================
void BUnitActionMove::buildIgnoreList(BEntityIDArray& ignoreUnits, bool addTarget)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3382
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return;

   ignoreUnits.setNumber(pSquad->getNumberChildren());
   uint index;
   for (index = 0; index < pSquad->getNumberChildren(); index++)
      ignoreUnits[index] = pSquad->getChild(index);
   
   //Add my target. 
   if (addTarget && mTarget.getID().isValid())
      ignoreUnits.add(mTarget.getID());
   
   //Add my target's squad.  
   /*if (addTarget && mTarget.getID().isValid())
   {
      BUnit* pTarget=gWorld->getUnit(mTarget.getID());
      if (pTarget)
      {
         BSquad* pSquad=pTarget->getParentSquad();
         if (pSquad)
         {
            const BEntityIDArray& targets=pSquad->getChildList();
            uint oldSize=ignoreUnits.getSize();
            ignoreUnits.setNumber(oldSize+targets.getSize());
            for (uint i=0; i < targets.getSize(); i++)
               ignoreUnits[oldSize+i]=targets[i];
         }
         else
            ignoreUnits.add(pTarget->getID());
      }
   }*/
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 3383
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   //We must have the orient controller (at least for now)
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID)
      return (false);
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //We must have the orient controller (at least for now).
   if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
      return (false);
   
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //Release the orientation controller.
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::determineTargetPosition(BVector& position, float& timeNeeded)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERTM(pSquad, "Trying to move a unit with an invalid parent squad!");
   if (!pSquad)
      return false;

   //Get our projected time.   
   float projectedTime = pSquad ? pSquad->getDefaultMovementProjectionTime() : 0.0f;

   //If we're due to a squad move, then get our desired position from our squad.
   if (mFlagSquadMove)
   {
      syncUnitActionData("BUnitActionMove::determineTargetPosition projectedTime", projectedTime);
      syncUnitActionData("BUnitActionMove::determineTargetPosition unit ID", pUnit->getID());
      if (pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectedTime, position, timeNeeded))
         return (true);
   }
   //Else, try the target.
   else if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3385
      const BEntity* pTarget=gWorld->getEntity(mTarget.getID());
//--
      if (pTarget)
      {
         position=pTarget->getPosition();
         if (mVelocity > 0.0f)
            timeNeeded=position.xzDistance(pUnit->getPosition())/mVelocity;
         else
            timeNeeded=0.0f;
         return (true);
      }
   }
   //Else, try the position.
   else if (mTarget.isPositionValid())
   {
      position=mTarget.getPosition();
      if (mVelocity > 0.0f)
         timeNeeded=position.xzDistance(pUnit->getPosition())/mVelocity;
      else
         timeNeeded=0.0f;
      return (true);
   }
   //Else, punt back to our squad.
   else
   {
      syncUnitActionCode("BUnitActionMove::determineTargetPosition last else");
      if (pSquad && pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectedTime, position, timeNeeded))
         return (true);
   }

   //Failure.
   return (false);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::determineTargetPath(BDynamicVectorArray& positions, float& timeNeeded, float& distanceNeeded, int& projectedTargetWaypoint)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   positions.clear();
   projectedTargetWaypoint = 0;

   //Get our projected time.
   // TODO:  totalProjectedTime is an approximation that should change based on how far ahead anim determination or unit to unit collision projection needs it.
   float projectedTargetTime = pSquad->getDefaultMovementProjectionTime();
   float totalProjectedTime = projectedTargetTime * 5.0f;

   bool gotSquadPath = false;

   //If we're due to a squad move, then get our desired position from our squad.
   if (mFlagSquadMove)
   {
      if (pSquad->getFutureDesiredChildPath(pUnit->getID(), totalProjectedTime, positions, timeNeeded, distanceNeeded))
         gotSquadPath = true;
   }
   //Else, try the target.
   else if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3386
      const BEntity* pTarget=gWorld->getEntity(mTarget.getID());
//--
      if (pTarget)
      {
         positions.add(pTarget->getPosition());
         if (mVelocity > 0.0f)
         {
            distanceNeeded = positions[0].xzDistance(pUnit->getPosition());
            timeNeeded = distanceNeeded / mVelocity;
         }
         else
         {
            timeNeeded=0.0f;
            distanceNeeded = 0.0f;
         }
         return (true);
      }
   }
   //Else, try the position.
   else if (mTarget.isPositionValid())
   {
      positions.add(mTarget.getPosition());
      if (mVelocity > 0.0f)
      {
         distanceNeeded = positions[0].xzDistance(pUnit->getPosition());
         timeNeeded = distanceNeeded / mVelocity;
      }
      else
      {
         timeNeeded=0.0f;
         distanceNeeded = 0.0f;
      }
      return (true);
   }
   //Else, punt back to our squad.
   else if (pSquad->getFutureDesiredChildPath(pUnit->getID(), totalProjectedTime, positions, timeNeeded, distanceNeeded))
      gotSquadPath = true;

   // If got a projected path from the squad, determine the intermediate waypoint along the path that unit should head towards.
   // This is similar to the projected target point that this used to get when it got a point from the squad rather than a path.
   // TODO: Find a better way to do this since the squad move action velocity calculation and getFutureDesiredChildPath are a lot more complex than this.
   if (gotSquadPath)
   {
      if (positions.getSize() > 1)
      {
         // Get squad velocity
//-- FIXING PREFIX BUG ID 3387
         const BSquadActionMove* pAction = reinterpret_cast<BSquadActionMove*> (pSquad->getActionByType(BAction::cActionTypeSquadMove));
//--
         if (pAction != NULL)
         {
            BPath path;
            path.setWaypoints(positions.getData(), positions.getSize());

            BVector targetPosition;
            float tempRealDistance;
            long newWaypoint = 0;
            if (path.calculatePointAlongPath(pAction->getVelocity() * projectedTargetTime, positions[0], 0, targetPosition, tempRealDistance, newWaypoint) &&
                (newWaypoint < positions.getNumber()))
            {
               // Add a new waypoint for the found point if there isn't one already near it.  This 
               if (!targetPosition.almostEqualXZ(positions[newWaypoint]))
                  newWaypoint = positions.insertAtIndex(targetPosition, newWaypoint);

               projectedTargetWaypoint = newWaypoint;
            }
         }
      }

      return true;
   }

   //Failure.
   return (false);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::determineTargetPosition2(float projectionTime, BVector& position, float& distance, float& timeNeeded)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BSquad* pSquad=pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   // jce [10/14/2008] -- See if we're in range of our target.
   float range = 0.0f;
   if(mTarget.isRangeValid())
      range = mTarget.getRange();

   //If we're due to a squad move, then get our desired position from our squad.
   if (mFlagSquadMove)
   {
      syncUnitActionData("BUnitActionMove::determineTargetPosition2 projectedTime", projectionTime);
      syncUnitActionData("BUnitActionMove::determineTargetPosition2 unit ID", pUnit->getID());
      if (pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectionTime, position, timeNeeded))
      {
         distance=pUnit->calculateXZDistance(position);
         return (true);
      }
   }
   //Else, try the target.
   else if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3388
      const BEntity* pTarget=gWorld->getEntity(mTarget.getID());
//--
      if (pTarget)
      {
         distance=pUnit->calculateXZDistance(pTarget);
         if(distance > range)
         {
            position=pTarget->getPosition();
            distance=pUnit->calculateXZDistance(pTarget);
        
            if (mVelocity > 0.0f)
               timeNeeded=distance/mVelocity;
            else
               timeNeeded=0.0f;
            return (true);
         }
      }
   }
   //Else, try the position.
   else if (mTarget.isPositionValid())
   {
      distance=pUnit->calculateXZDistance(mTarget);
      if(distance > range)
      {
         position=mTarget.getPosition();
         if (mVelocity > 0.0f)
            timeNeeded=distance/mVelocity;
         else
            timeNeeded=0.0f;
         return (true);
      }
   }

   //Else, punt back to our squad.
   // jce [10/14/2008] -- made this happen any time you fall through, which might be because you have a target but are in range.
   syncUnitActionCode("BUnitActionMove::determineTargetPosition2 last else");
   if (pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectionTime, position, timeNeeded))
   {
      distance=pUnit->calculateXZDistance(position);
      return (true);
   }

   //Failure.
   distance=0.0f;
   position=pUnit->getPosition();
   timeNeeded=0.0f;
   return (false);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::calcGaggle(float elapsedTime, BVector& targetPosition, float& velocity)
{
   //NOTE: This is intentionally written to be very simple and straightforward.  It's not
   //optimized yet and does extra calculations that the current weighting may or may not
   //use.  PLEASE DO NOT change that yet:)

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3391
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;

   //Handle degenerate case.
   if (pSquad->getNumberChildren() <= 1)
      return (true);

   //Update our gaggle velocity random factor.
   mVelocityRandomFactorTimer+=elapsedTime;
   if (mVelocityRandomFactorTimer > 1.0f)
   {
      mVelocityRandomFactor=getRandRangeFloat(cSimRand, 0.75f, 1.1f);
      mVelocityRandomFactorTimer=0.0f;
   }

   //Calc target stuff.
   BVector directionToTarget=targetPosition-pUnit->getPosition();
   directionToTarget.y=0.0f;
   const float cFlockInvertTargetDistance=0.25f;
   float distanceToTarget=directionToTarget.length();
   directionToTarget.normalize();
   //Decide if we can avoid our target or not.  This is something of a hack given how
   //the persistent move action plays with the targeting and the whole goofy squad vs.
   //unit thing.
   bool canAvoidTarget=true;
   BEntityID targetSquadID=cInvalidObjectID;
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 3389
      const BUnit* pTarget=gWorld->getUnit(mTarget.getID());
//--
      if (pTarget && pUnit->getPlayer()->isEnemy(pTarget->getPlayerID()))
      {
         if (pTarget->getParentSquad())
            targetSquadID=pTarget->getParentSquad()->getID();
         canAvoidTarget=false;
      }
   }

   //Get list of units to consider.
   BEntityIDArray flockMates;
   for (uint i=0; i < pSquad->getNumberChildren(); i++)
      flockMates.add(pSquad->getChild(i));

   //Rip through and get all of our flock data.  We don't count ourselves in any of this.
   const float cFlockSeparationDistance=2.0f;
   BVector flockForward=cOriginVector;
   BVector flockCenter=cOriginVector;
   BVector flockSeparationDirection=cOriginVector;
   float flockSeparationWeightMax=0.0f;
   uint flockSeparationCount=0;
   uint flockMateCount=0;
   for (uint i=0; i < flockMates.getSize(); i++)
   {
      BUnit* pFlockMate=gWorld->getUnit(flockMates[i]);
      if (!pFlockMate || (pFlockMate == pUnit))
         continue;

      //If our squad is moving, only consider units in front of this one.
      if (pSquad->isMoving())
      {
         BVector flockMateDirection=pFlockMate->getPosition()-pUnit->getPosition();
         flockMateDirection.y=0.0f;
         flockMateDirection.safeNormalize();
         float flockMateAngle=pUnit->getForward().angleBetweenVector(flockMateDirection);
         if (abs(flockMateAngle) > cPiOver2)
            continue;
      }
      
      flockMateCount++;
      BVector flockMateForward=pFlockMate->getForward();
      flockMateForward.y=0.0f;
      flockMateForward.safeNormalize();
      flockForward+=flockMateForward;
      flockCenter+=pFlockMate->getPosition();

      //See if this unit is within our separation distance.
      float flockMateDistance=pUnit->calculateXZDistance(pFlockMate);
      if (flockMateDistance < cFlockSeparationDistance)
      {
         //Get the direction.
         BVector flockMateDirection=pFlockMate->getPosition()-pUnit->getPosition();
         flockMateDirection.y=0.0f;
         flockMateDirection.safeNormalize();
         //Weight it by how far the flock mate is impinging us.
         float weight=1.0f-(flockMateDistance/cFlockSeparationDistance);
         flockMateDirection*=weight;
         flockSeparationDirection+=flockMateDirection;
         flockSeparationCount++;
         //Track the max weight.
         if (weight > flockSeparationWeightMax)
            flockSeparationWeightMax=weight;
      }
   }
   
   //Calculate the flock center.   
   BVector directionToFlockCenter(0.0f, 0.0f, 0.0f);
   if (flockMateCount > 0)
   {
      flockCenter/=(float)flockMateCount;
      directionToFlockCenter=flockCenter-pUnit->getPosition();
      directionToFlockCenter.y=0.0f;
      directionToFlockCenter.safeNormalize();
   }
   
   //Calculate the flock forward.
   if (flockMateCount > 0)
   {
      flockForward/=(float)flockMateCount;
      flockForward.y=0.0f;
      flockForward.safeNormalize();
   }

   //Calculate the flock separation direction.
   if (flockSeparationCount > 0)
   {
      flockSeparationDirection/=(float)flockSeparationCount;
      flockSeparationDirection.y=0.0f;
      flockSeparationDirection.safeNormalize();
   }

   //Get obstructions.
   BVector obstructionProjectedPosition=pUnit->getForward();
   obstructionProjectedPosition*=20.0f;
   obstructionProjectedPosition+=pUnit->getPosition();
   static BObstructionNodePtrArray obstructions;
   getGaggleObstructions(pUnit->getPosition(), obstructionProjectedPosition, pUnit->getObstructionRadius()*4.0f, obstructions);
   //Run through to find any that we need to care about.
   const float cObstructionDistanceLimit=8.0f;
   uint obstructionCount=0;
   BVector obstructionDirection=cOriginVector;
   float obstructionWeightMax=0.0f;
   #ifdef RENDERGAGGLE
   mGaggleObstructions.clear();
   #endif
   for (uint i=0; i < obstructions.getSize(); i++)
   {
      //Check the obstruction entity.
      BOPObstructionNode* pObstructionNode=obstructions[i];
      if (!pObstructionNode)
         continue;
//-- FIXING PREFIX BUG ID 3390
      const BEntity* pObstructionEntity=pObstructionNode->mObject;
//--
      if (pObstructionEntity)
      {
         //Ignore anyone in our squad.
         if (pObstructionEntity->getParentID() == mpOwner->getParentID())
            continue;
         //If we cannot avoid our target, then skip our target's obstruction or
         //the obstruction of anyone else in the target's squad.
         if (!canAvoidTarget)
         {
            if ((pObstructionEntity->getID() == mTarget.getID()) ||
               (pObstructionEntity->getParentID() == targetSquadID))
               continue;
         }
      }
         
      //Get the hull data.
      const BOPQuadHull* pHull=pObstructionNode->getHull();
      if (!pHull)
         continue;
      BVector hullCenter;
      pHull->computeCenter(hullCenter);
      float distanceToHull=pHull->distance(pUnit->getPosition());
      distanceToHull-=pUnit->getObstructionRadius();
      if (distanceToHull < 0.0f)
         distanceToHull=0.0f;

      //Skip obstructions that are too far away to care about.
      if (distanceToHull > cObstructionDistanceLimit)
         continue;

      //Figure the direction to the collision.
      BVector tempObstructionDirection=hullCenter-pUnit->getPosition();
      tempObstructionDirection.y=0.0f;
      tempObstructionDirection.safeNormalize();
      #ifdef RENDERGAGGLE
      mGaggleObstructions.add(hullCenter);
      #endif

      //Factor the weight based on distance.
      float weight=1.0f-(distanceToHull/cObstructionDistanceLimit);
      tempObstructionDirection*=weight;
      
      //Add it now.
      obstructionDirection+=tempObstructionDirection;
      obstructionCount++;
      //Track the max weight.
      if (weight > obstructionWeightMax)
         obstructionWeightMax=weight;
   }

   //Finish the obstruction calculations.
   if (obstructionCount > 0)
   {
      obstructionDirection/=(float)obstructionCount;
      obstructionDirection.y=0.0f;
      obstructionDirection.safeNormalize();
   }
   
   //Direction Factors:
   //  D0:  Go toward/away from the target position.
   //  D1:  Separate from close flockmates.
   //  D2:  Go in the same direction as flockmates.
   //  D3:  Avoid obstructions.
   BVector d[4];
   float w[4];

   //D0:  Go toward/away from the target position.
   if (canAvoidTarget && (distanceToTarget < cFlockInvertTargetDistance))
   {
      d[0]=-directionToTarget;
      w[0]=3.0f;
   }
   else
   {
      d[0]=directionToTarget;
      w[0]=2.0f;
   }
   //D1:  Separate from close flockmates.
   if (flockSeparationCount > 0)
   {
      d[1]=-flockSeparationDirection;
      w[1]=3.0f;
   }
   else
   {
      d[1]=cOriginVector;
      w[1]=0.0f;
   }
   //D2:  Go in the same direction as flockmates.
   if (flockMateCount > 0)
   {
      d[2]=flockForward;
      w[2]=0.3f;
   }
   else
   {
      d[2]=cOriginVector;
      w[2]=0.0f;
   }
   //D3:  Avoid obstructions.
   if (obstructionCount > 0)
   {
      d[3]=-obstructionDirection;
      w[3]=5.0f+10.0f*obstructionWeightMax;
   }
   else
   {
      d[3]=cOriginVector;
      w[3]=0.0f;
   }
   
   //Get our desired forward.
   BVector desiredForward=cOriginVector;
   for (uint i=0; i < 4; i++)
   {
      desiredForward+=d[i]*w[i];
      #ifdef RENDERGAGGLE
      if (w[i] > 0.0f)
         mGaggleDirections[i]=d[i];
      else
         mGaggleDirections[i]=cOriginVector;
      #endif
   }
   desiredForward.y=0.0f;
   desiredForward.safeNormalize();

   //Velocity Factors:
   //   0:  Match squad.  If the squad isn't going anywhere, just take our desired velocity.
   //   1:  Factor in our personal "random" factor.
   //   2:  Slow down if avoiding obstructions.
   //   3:  Slow down if separating from flockmates (and not avoiding obstructions).
   //   4:  Speed up if far away from target (and not slowing down).

   //0:  Match squad.  If the squad isn't going anywhere, just take our desired velocity.
   velocity=pSquad->getActionVelocity();
   if (velocity < cFloatCompareEpsilon)
      velocity=pUnit->getDesiredVelocity();
   //1:  Factor in our personal "random" factor.
   velocity*=mVelocityRandomFactor;
   //2:  Slow down if avoiding obstructions.
   const float cObstructionSlowdownSpread=0.6f;
   if (obstructionCount > 0)
   {
      float obstructionSlowdown=1.0f-cObstructionSlowdownSpread*obstructionWeightMax;
      velocity*=obstructionSlowdown;
   }
   //3:  Slow down if separating from flockmates (and not avoiding obstructions).
   const float cSeparationSlowdownSpread=0.3f;
   if ((flockSeparationCount > 0) && (obstructionCount == 0))
   {
      float separationSlowdown=1.0f-cSeparationSlowdownSpread*flockSeparationWeightMax;
      velocity*=separationSlowdown;
   }
   //4:  Speed up if far away from target (and not slowing down).
   const float cDistanceToTargetLimit=5.0f;
   if ((flockSeparationCount == 0) && (obstructionCount == 0) && (distanceToTarget > cDistanceToTargetLimit))
      velocity*=2.0f;
      
   //Calculate the new position by projecting 1 second into the future.  The
   //amount of time is less important as long as it's consistent for all the
   //units in the squad and reasonably short.
   const float cGaggleProjectionTime=1.0f;
   float distanceToMove=velocity*cGaggleProjectionTime;
   BVector positionChange=desiredForward*distanceToMove;
   targetPosition=pUnit->getPosition();
   targetPosition+=positionChange;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::limitTurn(float elapsedTime, BVector forward, BVector up, BVector& newForward, float correctionFactor)
{
   //newForward is set to what we want, forward is set to what we have now.
   //This function adjusts newForward if it turns "too much".

//-- FIXING PREFIX BUG ID 3393
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit && pUnit->getProtoObject());

   //Get turn rate.
   float turnSpeed=pUnit->getProtoObject()->getTurnRate()*cRadiansPerDegree;
   if (turnSpeed < cFloatCompareEpsilon)
      return (false);

   //Smoothly rotate towards the requested angle
   float unitAngle=forward.getAngleAroundY();
   float reqAngle=newForward.getAngleAroundY();
   float angleDiff=reqAngle-unitAngle;
   if ((angleDiff >= -cFloatCompareEpsilon) && (angleDiff <= cFloatCompareEpsilon))
   {
      // If the difference between the angles is almost zero, don't need to adjust forward vector.
      return false;
   }

   if (angleDiff > cPi)
      angleDiff = -(cPi + cPi - angleDiff);
   else if (angleDiff < -cPi)
      angleDiff = cPi + cPi + angleDiff;

   // ajl 1/9/02 - Yaw at a constant speed. The old way would yaw much 
   // slower as the angle difference got smaller. I tried to keep some
   // of the nice smoothness that the old way had by slowing the turn
   // speed down a little when the angle difference is less than 45 degrees.
   float yawAmount;
   float absDiff = (float) fabs(angleDiff);
   if (absDiff < cPi * 0.03125)
      turnSpeed *= 0.125f;
   else if (absDiff < cPi * 0.0625)
      turnSpeed *= 0.25f;
   else if (absDiff < cPiOver8)
      turnSpeed *= 0.50f;
   else if (absDiff < cPiOver4)
      turnSpeed *= 0.75f;

   float maxTurn = turnSpeed * elapsedTime;
   
   // jce [10/14/2008] -- adjust based on correctionFactor.  0.0 correctionFactor = allow full turn, 1.0 correction factor = normal cap on maximum
   maxTurn = Math::Lerp(absDiff, maxTurn, correctionFactor);
   
   if (absDiff > maxTurn)
   {
      if (angleDiff < 0.0f)
         yawAmount = -maxTurn;
      else
         yawAmount = maxTurn;
   }
   else
   {
      //yawAmount = angleDiff;
      // Turn isn't too large so don't need to adjust forward vector.
      return false;
   }

   // New forward
   BMatrix tempMatrix;
   tempMatrix.makeRotateArbitrary(yawAmount, up);
   tempMatrix.transformVector(forward, newForward);
   newForward.normalize();

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::getGaggleObstructions(BVector pos1, BVector pos2, float radius, BObstructionNodePtrArray& obstructions)
{
   BVector tempIntersectionPoint(0.0f);
   obstructions.clear();

   //Check any terrain or non-moving units.
//-- FIXING PREFIX BUG ID 3395
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit && pUnit->getProtoObject());
   long lLandOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockLandUnits;

   long lFloodOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockFloodUnits;

   long lScarabOptions = BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |
      BObstructionManager::cIsNewTypeBlockScarabUnits;

   long lObOptions = lLandOptions;
   if (pUnit->getProtoObject()->getMovementType() == cMovementTypeFlood)
      lObOptions = lFloodOptions;
   else if (pUnit->getProtoObject()->getMovementType() == cMovementTypeScarab)
      lObOptions = lScarabOptions;
   long lObNodeType=BObstructionManager::cObsNodeTypeAll;
   if (mpOwner->getFlagFlying())
      lObOptions=0;

   //Do the obstruction check
   gObsManager.begin(BObstructionManager::cBeginEntity, radius, lObOptions, lObNodeType, 0, cDefaultRadiusSofteningFactor, NULL, pUnit->canJump());
   gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, pos1, pos2, 
      true, tempIntersectionPoint, obstructions);
   gObsManager.end();
}


/*
      // Only consider obstructions in front of this one
      BVector hullDirection = hullCenter - unitPosition;
      hullDirection.y = 0.0f;
      hullDirection.normalize();
      float hullAngle = directionToTarget.angleBetweenVector(hullDirection);
      if (abs(hullAngle) > (120.0f * cRadiansPerDegree))
         continue;

      // See if unit will intersect hull
      // TOMMYTODO:  Is this check necessary?
      BVector intersectPoint;
      if (!pHull->rayIntersects(unitPosition, directionToTarget, intersectPoint, NULL, NULL))
         continue;

      // If unit moving then go in opposite direction
      BVector offsetDirection(0.0f);
      bool isMoving = false;
      if ((pObstructionNode->mObject != NULL) && (pObstructionNode->mObject->isMoving()))
      {
         if (distToHull < cCloseFlockMateLimit)
         {
            // TOMMYTODO:  This has problems if heading directly towards a unit but it has the nice characteristic of pushing
            // away from the obstruction, which can be used to better slow this unit or move it in the opposite direction.
            float distanceFactor = cCloseFlockMateLimit - distToHull;
            offsetDirection = hullDirection;
            offsetDirection *= distanceFactor;

            numObstructions++;
            isMoving = true;
         }
      }
      // Else find vector around the edge of the obstruction
      else
      {
         // TOMMYTODO:  This method seems to have a problem going full around obstructions, especially once a unit
         // has mostly moved past the obstruction.

         // Get perpendicular to direction to the obstruction center
         BVector perpHullDirection(hullCenter.z - unitPosition.z, 0.0f, unitPosition.x - hullCenter.x);
         if (hullAngle < 0.0f)
            perpHullDirection *= -1.0f;
         perpHullDirection.normalize();

         offsetDirection = hullDirection * distToHull;
         offsetDirection += (perpHullDirection * hullRadius);
         offsetDirection.normalize();

         numObstructions++;
      }


      //Get the intersection point.
      //BVector intersectionPoint;
      //if (!rayIntersects(pUnit->getPosition(), pUnit->getForward(), intersectionPoint, NULL, NULL))
      //   continue;

      //Get the closest point on the hull.
      //BVector hullClosestPoint=pHull->findClosestPointOnHull(pUnit->getPosition(), NULL, NULL);

*/

//==============================================================================
//==============================================================================
bool BUnitActionMove::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEFREELISTITEMPTR(pStream, BPathMoveData, mPathMoveData);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, float, mVelocity);
   GFWRITEVAR(pStream, float, mVelocityRandomFactor);
   GFWRITEVAR(pStream, float, mVelocityRandomFactorTimer);
   GFWRITEVAR(pStream, float, mWaitTimer);
                            
   #ifdef _MOVE4
   GFWRITEVAR(pStream, long, mRetryAttempts_4);
   #else
   GFWRITEVAL(pStream, long, 0);
   #endif   

   GFWRITEBITBOOL(pStream, mFlagSquadMove);
   GFWRITEBITBOOL(pStream, mRefreshPathData);
   GFWRITEBITBOOL(pStream, mHasTurnAnimation);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADCLASS(pStream, saveType, mTarget);
   GFREADFREELISTITEMPTR(pStream, BPathMoveData, mPathMoveData);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, float, mVelocity);
   GFREADVAR(pStream, float, mVelocityRandomFactor);
   GFREADVAR(pStream, float, mVelocityRandomFactorTimer);
   GFREADVAR(pStream, float, mWaitTimer);
                            
   #ifdef _MOVE4
   GFREADVAR(pStream, long, mRetryAttempts_4);
   #else
   GFREADTEMPVAR(pStream, long);
   #endif   

   GFREADBITBOOL(pStream, mFlagSquadMove);
   GFREADBITBOOL(pStream, mRefreshPathData);
   GFREADBITBOOL(pStream, mHasTurnAnimation);

   return true;
}

//==============================================================================
//==============================================================================
bool BPathMoveData::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mPath);
   GFWRITEVAR(pStream, int, mCurrentWaypoint);
   GFWRITEVAR(pStream, DWORD, mPathTime);
   GFWRITEFREELISTITEMPTR(pStream, BPathMoveData, mLinkedPath);
   GFWRITEVAR(pStream, BPathLevel, mPathLevel);
   return true;
}

//==============================================================================
//==============================================================================
bool BPathMoveData::load(BStream* pStream, int saveType)
{
   GFREADCLASS(pStream, saveType, mPath);
   GFREADVAR(pStream, int, mCurrentWaypoint);
   GFREADVAR(pStream, DWORD, mPathTime);
   GFREADFREELISTITEMPTR(pStream, BPathMoveData, mLinkedPath);
   GFREADVAR(pStream, BPathLevel, mPathLevel);
   return true;
}
