//==============================================================================
// unitactionmovewarthog.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionMoveWarthog.h"
#include "ConfigsGame.h"
#include "DesignObjectsManager.h"
#include "Entity.h"
#include "Physics.h"
#include "SyncMacros.h"
#include "Unit.h"
#include "World.h"
#include "protoobject.h"
#include "worldsoundmanager.h"


//==============================================================================
//==============================================================================
bool BImpulseEntry::apply(BUnit* pUnit, float elapsedTime)
{
   BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
   if (!pPhysicsObject || (mDuration < cFloatCompareEpsilon))
      return (false);

   //Figure out the impulse point.
   BVector impulsePoint;
   BPhysicsMatrix physicsObjectRotation;
   pPhysicsObject->getRotation(physicsObjectRotation);
   physicsObjectRotation.transformVectorAsPoint(mImpulseOffset, impulsePoint);
   BVector physicsObjectPosition;
   pPhysicsObject->getPosition(physicsObjectPosition);
   impulsePoint+=physicsObjectPosition;

   //Figure out how much of the impulse to apply.
   float impulsePercentage;
   bool done=false;
   if (mTimer+elapsedTime >= mDuration)
   {
      impulsePercentage=(mDuration-mTimer)/mDuration;
      done=true;
   }
   else
   {
      impulsePercentage=elapsedTime/mDuration;
      mTimer+=elapsedTime;
   }
   BVector impulse=mImpulse;
   impulse*=impulsePercentage;

   //Apply it.
   if (mImpulseOffset.length() < cFloatCompareEpsilon)
      pPhysicsObject->applyImpulse(impulse);
   else
      pPhysicsObject->applyPointImpulse(impulse, impulsePoint);
      
   return (!done);
}




IMPLEMENT_FREELIST(BUnitActionMoveWarthog, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionMoveWarthog::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(0); // MPB 11/29/2007 This shouldn't be used currently
   if (!BUnitActionMove::connect(pOwner, pOrder))
      return (false);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Reactivate physics object (and disallow deactivation) during a move
   BPhysicsObject* pPO = pUnit->getPhysicsObject();
   BASSERT(pPO);
   pPO->enableDeactivation(false);
   pPO->forceActivate();
   
   // Put into cAnimationStateWarthogMove so this action can have explicit
   // control over anim type
   // SLB: This system interferes with walk cycles and makes it impossible for the warthog to attack while moving
   //pUnit->setAnimationState(BObjectAnimationState::cAnimationStateWarthogMove, cAnimTypeWalk, true);

   updateHitAndRun();
  
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::disconnect()
{
   BUnit* pUnit = mpOwner->getUnit();

   //-- DJBFIXME: Data drive this after E3
   if(mPlayingInAirSound)
   {    
      BCueIndex index = gSoundManager.getCueIndex("stop_warthog_fly");
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false);
      mPlayingInAirSound = false;
   }

   // Allow physics object to deactivate once a move is complete
   BPhysicsObject* pPO = pUnit->getPhysicsObject();
   BASSERT(pPO);
   pPO->enableDeactivation(true);

   BUnitActionMove::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveWarthog::init()
{
   if (!BUnitActionMove::init())
      return (false);

   mTargetPos=cOriginVector;
   mLastPosition=cInvalidVector;
   mLastPositionValid=false;
   mTargetPosIsGoal=true;
   mHitAndRun=false;
   mSkidding = false;
   mInAir = false;
   mIgnoreTerrainCollisions=false;
   mPlayingInAirSound = false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveWarthog::update(float elapsedTime)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //If we no longer have the controllers we need, fail.
   if (!validateControllers())
   {
      syncUnitActionData("BUnitActionMove::update validateControllers failed", pUnit->getID().asLong());
      setState(cStateFailed);
      return (true);
   }

   //Update our Hit and Run status.
   updateHitAndRun();

   //Main logic is here.  Don't put too much linear execution stuff in the actual
   //case statements themselves; write straightforward methods for that.
   switch (mState)
   {
      case cStateNone:
      case cStatePathing:
      {
         mTargetPos = pUnit->getPosition();
         mTargetPosIsGoal = true;

         // Create initial high level path to target
         BActionState newState = createInitialPathData();

         setState(newState);
         //NOTE: This intentionally falls through to the next case (as long as it's cStateWorking:).
         if (mState != cStateWorking)
            break;
      }

      case cStateWorking:
      {
         const BSquad* pSquad = pUnit->getParentSquad();
         BASSERT(pSquad);
         if (!pSquad)
            return false;

         // Temporary immobility
         if (pSquad->getFlagNonMobile())
            break;

         //Update our jump pad check.
         updateJumpPad();
         
         //Update our impulses.
         updateImpulses(elapsedTime);

         // Update our animation based on skidding
         updateSkidAnimState();
      
         //Follow movement.
         BActionState newState = followPath(elapsedTime);

         //Handle state changes.
         if (newState != cStateWorking)
            setState(newState);
         break;
      }
   }

   // Keep velocity up-to-date
//-- FIXING PREFIX BUG ID 5063
   const BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
//--
   BVector velocity;
   pPhysicsObject->getLinearVelocity(velocity);
   pUnit->setVelocity(velocity);

   //Done.
   return (true);
}


//==============================================================================
//==============================================================================
BActionState BUnitActionMoveWarthog::followPath(float elapsedTime)
{
   // Make sure there is valid path data
   if (mPathMoveData == NULL)
      return cStateFailed;

   static BEntityIDArray ignoreUnits;
   BUnit* pUnit = mpOwner->getUnit();

   // Calculate new position based on current pathing method
   BVector newPosition;
   BActionState actionState = calcMovement2(elapsedTime, newPosition);

   // Done or couldn't calculate movement
   if ((actionState != cStateWorking) && (actionState != cStatePathing))
   {
      // Make sure final position got set
      if (actionState == cStateDone)
         mTargetPos = newPosition;
         //advanceToNewPosition(elapsedTime, newPosition);

      return actionState;
   }

   // Check for obstructions
   BSimCollisionResult collisionResult = cSimCollisionNone;
   static BEntityIDArray collisions;
   collisions.clear();
   if (actionState != cStatePathing)
   {
      buildIgnoreList(ignoreUnits, false);
      collisionResult = pUnit->checkForCollisions(ignoreUnits, pUnit->getPosition(), newPosition, mPathMoveData->mPathTime, !mHitAndRun, collisions);
   }

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
      actionState = calcMovement2(elapsedTime, newPosition);
      if (actionState == cStateFailed)
         return actionState;
   }

   // Set the new position
   mTargetPos = newPosition;
   //advanceToNewPosition(elapsedTime, newPosition);

   return cStateWorking;
}


//==============================================================================
//==============================================================================
BActionState BUnitActionMoveWarthog::calcMovement2(float elapsedTime, BVector& newPosition)
{
   mTargetPosIsGoal = true;

   // NOTE: If this ever returns false, the movement data is trashed/malformed.
   BUnit* pUnit = mpOwner->getUnit();
   newPosition=pUnit->getPosition();

   //Check our target distance if we have a valid range.
   if (mTarget.isRangeValid())
   {
      float distanceToTarget;
      if (mTarget.getID().isValid())
      {
//-- FIXING PREFIX BUG ID 5067
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
//-- FIXING PREFIX BUG ID 5068
   const BEntity* pTarget = NULL;
//--

   // Validate path data
   if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
      return cStateFailed;

   // If doing vector movement then update the waypoint to match where squad's says to go
   float velocityToFutureTarget = -1.0f;
   if (pPathData->mPathLevel == BPathMoveData::cPathLevelUser)
   {
      // NOTE:
      // User level path can only have one waypoint now.  If this needs to change then need
      // to fix updating of user level path with updated target position.
      BASSERT(pPathData->mPath.getNumberWaypoints() <= 1);

      BVector tempTargetPosition;
      float timeNeeded = 0.0f;
      BSquad *pSquad = pUnit->getParentSquad();
      if (!pSquad)
         return cStateFailed;

      // Get suggested time to project into future.  This should correspond to the time it takes for the platoon
      // to move the distance between two long range path waypoints.
      // This can change in the future if units need to do something more intelligent.
      float projectedTime = pSquad->getDefaultMovementProjectionTime();
      if (projectedTime < 0.0f)
         projectedTime = 3.0f;//cDefaultMovementProjectionTime;

      // Get target position and range depending on how target was specified
      if (mFlagSquadMove)
      {
         syncUnitActionData("BUnitActionMoveWarthog::calcMovement2 projectedTime", projectedTime);
         syncUnitActionData("BUnitActionMoveWarthog::calcMovement2 unit ID", pUnit->getID());
         if (!pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectedTime, tempTargetPosition, timeNeeded))
            return cStateFailed;
      }
      else if (mTarget.getID().isValid())
      {
         pTarget = gWorld->getEntity(mTarget.getID());
         if (!pTarget)
            return (cStateFailed);
         tempTargetPosition = pTarget->getPosition();
      }
      else if (mTarget.isPositionValid())
         tempTargetPosition = mTarget.getPosition();
      else
      {
         syncUnitActionCode("BUnitActionMove::calcMovement2 last else");
         if (!pSquad->getFutureDesiredChildLocation(pUnit->getID(), projectedTime, tempTargetPosition, timeNeeded))
            return cStateFailed;
      }
      pPathData->mPath.setWaypoint(pPathData->mCurrentWaypoint, tempTargetPosition);

      // If squad returned a valid time to future position, use it to calculate the velocity needed
      // to reach the future position in time.
      if (timeNeeded > cFloatCompareEpsilon)
      {
         float distToFutureTarget = tempTargetPosition.xzDistance(pUnit->getPosition());
         velocityToFutureTarget = distToFutureTarget / timeNeeded;

      }
      if (timeNeeded >= projectedTime)
         mTargetPosIsGoal = false;
   }
   else
      mTargetPosIsGoal = false;

   targetPosition = pPathData->mPath.getWaypoint(pPathData->mCurrentWaypoint);

   // Calculate the remaining distance
   //Tommy: At this point, don't we really just want to check distance to a position (and let the
   //overall range check take care of the target)?
   if (pTarget != NULL)
      distanceRemaining=pUnit->calculateXZDistance(pTarget);
   else
      distanceRemaining=pUnit->calculateXZDistance(targetPosition);
   
   // Handle the case where we're at the next waypoint.
   if (distanceRemaining <= cFloatCompareEpsilon)
   {
      pPathData->mCurrentWaypoint++;
      if (pPathData->mCurrentWaypoint >= pPathData->mPath.getNumberWaypoints())
      {
         // Reached end of current path.  No position change needed because close enough.  This
         // used to set position to target position but that caused jumping at end of movement.
         return (advancePathData());
      }

      targetPosition = pPathData->mPath.getWaypoint(pPathData->mCurrentWaypoint);
      newPosition = targetPosition;
      if (pTarget != NULL)
         distanceRemaining=pUnit->calculateXZDistance(pTarget);
      else
         distanceRemaining=pUnit->calculateXZDistance(targetPosition);
   }

   newPosition = targetPosition;
   return newState;
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::updateHitAndRun()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5070
   const BSquad* pSquad=pUnit->getParentSquad();
//--
   BASSERT(pSquad);
   if (pSquad && pSquad->getSquadMode() == BSquadAI::cModeHitAndRun)
      mHitAndRun=true;
   else
      mHitAndRun=false;
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::updateJumpPad()
{
   //Get the unit data.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5071
   const BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
//--
   if (!pPhysicsObject)
      return;

   //Get the DOM.
   BDesignObjectManager* pDOM=gWorld->getDesignObjectManager();
   if (!pDOM || (pDOM->getDesignLineCount() == 0))
      return;

   //If we have a valid last position, do the check.
   BVector currentPosition=pUnit->getPosition();
   if (mLastPositionValid && (currentPosition != mLastPosition))
   {
      for (uint i=0; i < pDOM->getDesignLineCount(); i++)
      {
         //Get the line.  Ignore non-physics lines.
         BDesignLine& line=pDOM->getDesignLine(i);
         if (line.mDesignData.isPhysicsLine())
         {
            //See if we intersect.
            BVector intersectionPoint;
            if (line.incidenceIntersects(pUnit->getForward(), mLastPosition, currentPosition, intersectionPoint))
            {
               //Impulse is Vector1.
               BVector impulse;
               line.mDesignData.getValuesAsVector(1, impulse);
               //Impulse randomness is Value11.
               float impulseRandomness=line.mDesignData.mValues[10];
               if (impulseRandomness > 0.0f)
               {
                  if (fabs(impulse.x) > 0.0f)
                     impulse.x+=getRandRangeFloat(cSimRand, -impulseRandomness, impulseRandomness);
                  if (fabs(impulse.y) > 0.0f)
                     impulse.y+=getRandRangeFloat(cSimRand, -impulseRandomness, impulseRandomness);
                  if (fabs(impulse.z) > 0.0f)
                     impulse.z+=getRandRangeFloat(cSimRand, -impulseRandomness, impulseRandomness);
               }

               //ImpulseOffset (in ModelSpace) is Vector2.
               BVector impulseOffset;
               line.mDesignData.getValuesAsVector(2, impulseOffset);
               
               //Duration is Value12.
               float duration=line.mDesignData.mValues[11];
               
               //Add this impulse to our list.
               BImpulseEntry impulseEntry;
               impulseEntry.setImpulse(impulse, impulseOffset);
               impulseEntry.setDuration(duration);
               mImpulses.add(impulseEntry);
            }
         }
         /*else if (line.mDesignData.isTerrainCollisionToggleLine())
         {
            //See if we intersect.
            BVector intersectionPoint;
            if (line.incidenceIntersects(pUnit->getForward(), mLastPosition, currentPosition, intersectionPoint))
               mIgnoreTerrainCollisions=!mIgnoreTerrainCollisions;
         }*/
      }
   }

   //Save our position.
   mLastPosition=currentPosition;
   mLastPositionValid=true;
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::updateImpulses(float elapsedTime)
{
   //Get the unit data.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   
   for (uint i=0; i < mImpulses.getSize(); i++)
   {
      if (!mImpulses[i].apply(pUnit, elapsedTime))
      {
         mImpulses.removeIndex(i);
         i--;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::updateSkidAnimState()
{
   //Get the unit data.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5072
   const BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
//--
   if (!pPhysicsObject)
      return;

   // Get velocity and right vectors
   BVector linVel;
   BPhysicsMatrix rot;
   pPhysicsObject->getLinearVelocity(linVel);
   pPhysicsObject->getRotation(rot);
   BVector right = rot.getRight();
   linVel.y = 0.0f;
   right.y = 0.0f;

   bool wasSkidding = mSkidding;

   static float cIdleMagnitude = 1.0f;
   static float cSkidMagnitude = 5.0f;   
   // If we're not moving, or moving slowly, put in idle
   if (linVel.length() < cIdleMagnitude)
   {
      // SLB: This system interferes with walk cycles and makes it impossible for the warthog to attack while moving
      //pUnit->setAnimationState(BObjectAnimationState::cAnimationStateWarthogMove, cAnimTypeIdle, true);
      mSkidding = false;
   }
   // Turn on skid anim if our velocity is sufficiently towards the unit's right vector (using Run anim for now)
   else if (fabs(linVel.dot(right)) > cSkidMagnitude)
   {
      // SLB: This system interferes with walk cycles and makes it impossible for the warthog to attack while moving
      //pUnit->setAnimationState(BObjectAnimationState::cAnimationStateWarthogMove, cAnimTypeRun, true);
      mSkidding = true;      
   }
   // Otherwise go back to regular move state
   else
   {
      // Go back to jog instead of walk if we were already moving
      // SLB: This system interferes with walk cycles and makes it impossible for the warthog to attack while moving
      //if (mSkidding)
         //pUnit->setAnimationState(BObjectAnimationState::cAnimationStateWarthogMove, cAnimTypeJog, true);
      mSkidding = false;      
   }

   //-- Play sound
   if(wasSkidding != mSkidding)
   {
      if(mSkidding)
      {
         BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundSkidOn);
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, true);
      }      
      else
      {
         BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundSkidOff);
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionMoveWarthog::setInAir(bool inAir)
{ 
   bool changedState = (mInAir != inAir);
   mInAir = inAir; 

   if(changedState)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      if(!mInAir && mPlayingInAirSound)
      {
         //-- DJBFIXME: Data drive this after E3
         BCueIndex index = gSoundManager.getCueIndex("stop_warthog_fly");
            gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false );
         mPlayingInAirSound = false;
      }
      else if(mInAir && !mPlayingInAirSound)
      {
         //-- Is moving fast enough to play this sound?
         BVector velocity = cOriginVector;
//-- FIXING PREFIX BUG ID 5073
         const BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
//--
         if (pPhysicsObject)
            pPhysicsObject->getLinearVelocity(velocity);

         if(velocity.length() > 20.0f)
         {
            //-- DJBFIXME: Data drive this after E3
            BCueIndex index = gSoundManager.getCueIndex("play_warthog_fly");
            gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, true, cInvalidCueIndex, true, true);
            mPlayingInAirSound = true;
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveWarthog::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASSARRAY(pStream, saveType, mImpulses, uint8, 20);
   GFWRITEVECTOR(pStream, mTargetPos);
   GFWRITEVECTOR(pStream, mLastPosition);
   GFWRITEBITBOOL(pStream, mLastPositionValid);
   GFWRITEBITBOOL(pStream, mTargetPosIsGoal);
   GFWRITEBITBOOL(pStream, mHitAndRun);
   GFWRITEBITBOOL(pStream, mSkidding);
   GFWRITEBITBOOL(pStream, mInAir);
   GFWRITEBITBOOL(pStream, mIgnoreTerrainCollisions);
   GFWRITEBITBOOL(pStream, mPlayingInAirSound);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionMoveWarthog::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASSARRAY(pStream, saveType, mImpulses, uint8, 20);
   GFREADVECTOR(pStream, mTargetPos);
   GFREADVECTOR(pStream, mLastPosition);
   GFREADBITBOOL(pStream, mLastPositionValid);
   GFREADBITBOOL(pStream, mTargetPosIsGoal);
   GFREADBITBOOL(pStream, mHitAndRun);
   GFREADBITBOOL(pStream, mSkidding);
   GFREADBITBOOL(pStream, mInAir);
   GFREADBITBOOL(pStream, mIgnoreTerrainCollisions);
   GFREADBITBOOL(pStream, mPlayingInAirSound);
   return true;
}

//==============================================================================
//==============================================================================
bool BImpulseEntry::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mImpulse);
   GFWRITEVECTOR(pStream, mImpulseOffset);
   GFWRITEVAR(pStream, float, mDuration);
   GFWRITEVAR(pStream, float, mTimer);
   return true;
}

//==============================================================================
//==============================================================================
bool BImpulseEntry::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mImpulse);
   GFREADVECTOR(pStream, mImpulseOffset);
   GFREADVAR(pStream, float, mDuration);
   GFREADVAR(pStream, float, mTimer);
   return true;
}
