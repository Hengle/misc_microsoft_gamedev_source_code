//==============================================================================
// movementhelper.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "movementHelper.h"
#include "entity.h"
#include "unit.h"
#include "squad.h"
#include "squadActionMove.h"
#include "protoobject.h"
#include "database.h"
#include "world.h"
#include "game.h"

//==============================================================================
// BMovementHelper::BMovementHelper
//==============================================================================
BMovementHelper::BMovementHelper( void )
{
}

//==============================================================================
// BMovementHelper::~BMovementHelper
//==============================================================================
BMovementHelper::~BMovementHelper( void )
{
}

//==============================================================================
//==============================================================================
/*bool BMovementHelper::updateTargetLocation(BEntity *pEntity, bool bAllowEndMove)
{ 
   if(!pEntity)
      return false;

   if(pEntity->getFlagMoving() == false)
      return false;

   BPath *path = pEntity->getPath();

   //-- make sure movement state is accurate
   if (path->getNumberWaypoints() == 0)
   {
      if (pEntity->getMovementData() != BEntity::cMovementDataNeedsPath)
         pEntity->setMovementData(BEntity::cMovementDataUnderParentControl);
   }
   else
      pEntity->setMovementData(BEntity::cMovementDataHasPath);

   //-- do we need a path?
   if (pEntity->getMovementData() == BEntity::cMovementDataNeedsPath)
   {
      pEntity->sendEvent(pEntity->getParentID(), pEntity->getID(), BEntity::cEventNeedPath, 0 );
      pEntity->setMovementData(BEntity::cMovementDataWaitingForPath);
      return false;
   }

   if (pEntity->getMovementData() == BEntity::cMovementDataWaitingForPath)
      return false;

   if (pEntity->getCurrentWaypoint() != -1)
   {
      pEntity->setTargetLocation(path->getWaypoint(pEntity->getCurrentWaypoint()));
   }

   BUnit* pUnit = pEntity->getUnit();

   float tolerance = cFloatCompareEpsilon;
   if(pUnit)
      tolerance += pUnit->getObstructionRadius();

   float distanceRemaining = pEntity->getTargetLocation().xzDistance(pEntity->getPosition());

   if (distanceRemaining <= tolerance)
   {
      if (pEntity->getCurrentWaypoint() != -1)
      {     
         long currentWaypoint = pEntity->getCurrentWaypoint() + 1;

         if (currentWaypoint >= path->getNumberWaypoints())
         {
            pEntity->setCurrentWaypoint(-1);
            path->zeroWaypoints();
         }
         else
         {            
            pEntity->setCurrentWaypoint(currentWaypoint);
            pEntity->setTargetLocation(path->getWaypoint(currentWaypoint));
         }
      }

      if (pEntity->getCurrentWaypoint() == -1)
      {
         if (bAllowEndMove)
         {
            bool doEndMove = true;
            if (pEntity->getMovementData() == BEntity::cMovementDataUnderParentControl)
            {
               doEndMove = false;
               BSquad* pSquad = pUnit->getParentSquad();
               if (pSquad && pSquad->getCurrentWaypoint() == -1)
                  doEndMove = true;
            }
            if (doEndMove)
               pEntity->endMove();
         }

         return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BMovementHelper::doSquadMovement(BSquad* pSquad, float elapsedTime, float speedScale)
{
   if(!pSquad)
      return;

   if(pSquad->getFlagMoving() == false)
      return;

   //-- update direction
   BVector dir = calculateDirection(pSquad);
   pSquad->setForward(dir);
   pSquad->calcRight();
   pSquad->calcUp();

   //-- update velocity
   BVector stepVelocity = calculateVelocity(pSquad, elapsedTime, speedScale, dir, false);
   //blogtrace("elapsed (%0.2f) step velocity(%0.2f, %0.2f, %0.2f) remainder(%0.2f)", elapsedTime, stepVelocity.x, stepVelocity.y, stepVelocity.z, excessDist);

   //-- update position
   updatePosition(pSquad, stepVelocity);
}

//==============================================================================
//==============================================================================
void BMovementHelper::doUnitMovement(BUnit* pUnit, float elapsedTime, bool orienControl, float speedScale)
{
   if(!pUnit)
      return;

   if(pUnit->getFlagMoving() == false)
      return;

   //-- update direction
   BVector dir = calculateDirection(pUnit);

   if(!pUnit->getHasFacingCommand())
      pUnit->setGoalVector(dir);

   //-- update velocity
   BVector stepVelocity = calculateVelocity(pUnit, elapsedTime, speedScale, dir, orienControl);

   //-- update position
   updatePosition(pUnit, stepVelocity);
}*/

//==============================================================================
//==============================================================================
void BMovementHelper::doProjectileMovement(BProjectile *pEntity, BVector perturbance, float elapsedTime, bool isAffectedByGravity, bool isTracking, float gravity, float turnRate, BVector &facingVector, float speedScale)
{
   if(!pEntity)
      return;

   if(pEntity->getFlagMoving() == false)
      return;

   BVector stepVelocity;
   if (isTracking)
   {
      // Do we need to turn?
      BVector velocity = pEntity->getVelocity();
      BVector perturbedVelocity = velocity + perturbance;
      BVector targetDir = pEntity->getTargetLocation() - pEntity->getPosition();
      targetDir.normalize();
      float perturbedSpeed = perturbedVelocity.length();
      BVector velocityDir = (perturbedSpeed <= cFloatCompareEpsilon) ? velocity : XMVectorScale(perturbedVelocity, 1.0f / perturbedSpeed);
      float targetTurnAngle = (1.0f - velocityDir.dot(targetDir)) * 90.0f;

      if (Math::fAbs(targetTurnAngle) > cFloatCompareEpsilon)
      {
         // Looks like we need to turn. Cap turn angle with turn rate.
         float maxTurnAngle = turnRate * elapsedTime;
         float minTurnAngle = -maxTurnAngle;
         targetTurnAngle = Math::Clamp(targetTurnAngle, minTurnAngle, maxTurnAngle);

         // Turn
         float speed = velocity.length();
         BVector turnAxis = velocityDir.cross(targetDir);
         BMatrix rotationMatrix = XMMatrixRotationAxis(turnAxis, XMConvertToRadians(targetTurnAngle));
         velocityDir = XMVector3TransformNormal(velocityDir, rotationMatrix);
         pEntity->setVelocity(velocityDir * speed);
      }

      stepVelocity = XMVectorScale(velocityDir, perturbedSpeed * elapsedTime);
      updatePosition(pEntity, stepVelocity);
      facingVector = velocity;
   }
   else if (isAffectedByGravity)
   {
      BVector oldVelocity = pEntity->getVelocity();
      BVector newVelocity = oldVelocity;
      newVelocity.y -= gravity * elapsedTime;
      stepVelocity = (oldVelocity + newVelocity) * (0.5f * elapsedTime);
      updatePosition(pEntity, stepVelocity);
      pEntity->setVelocity(newVelocity);
      facingVector = newVelocity;
   }
   else
   {
      stepVelocity = pEntity->getVelocity() * elapsedTime;
      updatePosition(pEntity, stepVelocity);
      facingVector = stepVelocity;
   }
}

//==============================================================================
// BMovementHelper::calculateDirection
//==============================================================================
/*BVector BMovementHelper::calculateDirection(BEntity *pEntity)
{
   //-- user dir as a force
   BVector dir = pEntity->getTargetLocation() - pEntity->getPosition();

   dir.y = 0;

   // BTK/AJL 10/12/06 - if we have a invalid direction because the entity pos and 
   // the target positions are the same then return the last known forward direction
   bool bSuccess = dir.safeNormalize();
   if (!bSuccess)
      dir = pEntity->getForward();

   return dir;
}

//==============================================================================
// BMovementHelper::calculateVelocity
//==============================================================================
BVector BMovementHelper::calculateVelocity(BEntity *pEntity, float elapsedTime, float speedScale, BVector dir, bool orienControl)
{

//   BVector prevVelocity = pEntity->getVelocity();
   float curVelocity = Math::Min(pEntity->getDesiredVelocity() * speedScale, pEntity->getMaxVelocity());
   float distanceRemaining = pEntity->getTargetLocation().xzDistance(pEntity->getPosition());

   //DCP 04/09/07: Test hack.
   //BSquad *squad=pEntity->getSquad();
   //if (squad)
   //   curVelocity=squad->getCurrentDesiredVelocity();

#if 0
   BUnit* pUnit = pEntity->getUnit();
   if (orienControl && pUnit && pUnit->getProtoObjectFlag(BProtoObject::))
   {
      //-- handle vehicle acceleration and deceleration
      bool forceDecel = false;

      if(0)
      {
      // If we have at least two frames of movement history, find out if the current effective turn capability
      // will allow us to get to the next destination point - If not, decelerate.
      BVector posHistory[2];
      posHistory[0] = pUnit->getPosHistory(0);
      posHistory[1] = pUnit->getPosHistory(1);
      BVector currPos = pEntity->getPosition();
      currPos.y = 0.0f; // Consider X-Z plane only
      if (posHistory[0].lengthSquared() != 0.0f && posHistory[0] != posHistory[1] && posHistory[1] != currPos)
      {
         // Find center of current turning circle
         BVector center;
         center.y = 0.0f; // Consider X-Z plane only
         if (findCircleCenter( center, posHistory[0], posHistory[1], currPos ))
         {
            float turnRadiusSqr = center.distanceSqr(currPos);
            float destRadiusSqr = center.distanceSqr(pEntity->getTargetLocation());
            if (turnRadiusSqr > destRadiusSqr) // Can't get there at this speed - slow down
               forceDecel = true;
         }
      }
      }

      float goalSpeed;
      if (forceDecel)
         goalSpeed = 0.0f;
      else
      {
         // BSR [10/4/06]: Calculate alignment to decrease velocity if we are not aligned with goal direction
         BVector goalVec = ((BUnit*) pEntity)->getGoalVector();
         goalVec.y = 0.0f;
         goalVec.normalize();
         BVector forward = pEntity->getForward();
         forward.y = 0.0f;
         forward.normalize();
         float alignment = goalVec.dot(forward);

         goalSpeed = curVelocity * alignment;

         // Decrease goal speed as we approach our final destination (in the current path)
         if (pEntity->getCurrentWaypoint()== -1)
         //if (pEntity->isOnFinalSegment())
         {
            float decelFactor = 0.25f * distanceRemaining;
            //float decelFactor = 0.25f * pEntity->distanceToCurrentWaypoint();
            if (decelFactor < 1.0f)
               goalSpeed *= decelFactor;
         }
      }

      BVector newVelocity = goalSpeed * pEntity->getForward();
      BVector deltaV = newVelocity - prevVelocity;
      float speedChange = deltaV.length();
      if (speedChange > (50.0f * elapsedTime)) // FIXME - rates should be data driven from the proto
         speedChange = 50.0f * elapsedTime;

      deltaV.normalize();
      deltaV = speedChange * deltaV;
      newVelocity = prevVelocity + deltaV;

      pEntity->setVelocity(newVelocity);

      //pUnit->setPosHistory(0, posHistory[1]);
      //pUnit->setPosHistory(1, currPos);
   }
   else
   {
      //-- handle unit/squad (non-vehicle) acceleration and deceleration
      BVector velocity = pEntity->getVelocity();
      BVector temp = dir;
      float acceleration = pEntity->getAcceleration();
      if (acceleration == 0.0f)
         acceleration = 6.0f; // arbitrary default
      velocity += (dir * (acceleration * elapsedTime));
      float len = velocity.length();
      temp.scale(Math::Min(curVelocity, len));
      pEntity->setVelocity(temp);
   }
#endif

   //-- calculate the step velocity
   // BSR [3/29/07]: BACK TO BASICS - Removing units' ability to deviate from path
   BVector stepVelocity = dir * curVelocity * elapsedTime;
   float moveDist = stepVelocity.length();
   if (moveDist > distanceRemaining)
   {
      stepVelocity = dir;
      stepVelocity.normalize();
      stepVelocity.scale(distanceRemaining);      
   }

   return stepVelocity;
}*/

//==============================================================================
// BMovementHelper::updatePosition
//==============================================================================
void BMovementHelper::updatePosition(BEntity *pEntity, BVector stepVelocity)
{
   // TRB 07/27/07:  Removed old entity movement data.  This wasn't being set anymore so checking the flag was useless.
   //if (pEntity->getMovementData() == BEntity::cMovementDataNeedsPath || pEntity->getMovementData()==BEntity::cMovementDataWaitingForPath)
   //   return;

   //-- move forward
   BVector newPosition = pEntity->getPosition();
   newPosition += stepVelocity;


   //-- check obstruction
   //DCP 07/17/07:  Well, Tommy and I are turning this off because it doesn't work for projectiles anyway.
   //if (pEntity->checkCollisions(pEntity->getPosition(), newPosition) != BEntity::cCollisionNone)
   //{
   //   pEntity->setMovementData(BEntity::cMovementDataNeedsPath);
   //   return;
   //}

   //-- do the update
   #ifdef SYNC_Unit
      if (pEntity->isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BMovementHelper::updatePosition", newPosition);
   #endif
   pEntity->setPosition(newPosition);
}

//==============================================================================
// BMovementHelper::findCircleCenter
//==============================================================================
/*bool BMovementHelper::findCircleCenter(BVector &center, BVector &p1, BVector &p2, BVector &p3)
{
   // Note: This is simplified to find the center of a circle based on the 2-D (X-Z) projection of the points
   // (Cheaper, faster, adequate for purpose)

   // Check to see if they are collinear in the X-Z plane - If not, bail.
   BVector vec12 = p2 - p1;
   BVector vec23 = p3 - p2;
   vec12.y = 0.0f;
   vec23.y = 0.0f;
   vec12.normalize();
   vec23.normalize();
   if ( vec12 == vec23 || ( -1.0f * vec12 ) == vec23 )
      return( false );

   // Not collinear - proceed
   center.x = (((p1.x * p1.x) + (p1.z * p1.z)) * (p3.z - p2.z) + ((p2.x * p2.x) + (p2.z * p2.z)) * (p1.z - p3.z) + ((p3.x * p3.x) + (p3.z * p3.z)) * (p2.z - p1.z)) /
      (2.0f * (p1.x * (p3.z - p2.z) +  p2.x * (p1.z - p3.z) +  p3.x * (p2.z - p1.z)));

   center.z = (((p1.x * p1.x) + (p1.z * p1.z)) * (p3.x - p2.x) + ((p2.x * p2.x) + (p2.z * p2.z)) * (p1.x - p3.x) + ((p3.x * p3.x) + (p3.z * p3.z)) * (p2.x - p1.x)) /
      (2.0f * (p1.z * (p3.x - p2.x) +  p2.z * (p1.x - p3.x) +  p3.z * (p2.x - p1.x)));

   return( true );
}*/





//OLD Move Action
/*
//==============================================================================
//==============================================================================
void BUnitActionMove::disconnect()
{
   mpOwner->removeOrientationController(this);
   BAction::disconnect();   
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::init()
{
   if (!BAction::init())
      return(false);
      
   mFlagConflictsWithIdle=true;
   mTarget.reset();
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::setState(BActionState state)
{
   syncUnitActionData("BUnitActionMove::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionMove::setState state", state);

   BUnit* pUnit=mpOwner->getUnit();

   switch (state)
   {
      case cStateNone:
         break;

      case cStateDone:
      case cStateFailed:
         pUnit->setMovementData(BEntity::cMovementDataUnderParentControl);
         pUnit->getPath()->reset();
         pUnit->setCurrentWaypoint(-1);
      case cStateWait:
         break;
   }
 
   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BUnitActionMove::update(float elapsed)
{
   syncUnitActionData("BUnitActionMove::update owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionMove::update mState", mState);

   BUnit* pUnit=mpOwner->getUnit();

   switch (mState)
   {
      case cStateNone:
      {
         if (!mTarget.isPositionValid())
         {
            setState(cStateFailed);
            break;
         }

         syncUnitActionData("BUnitActionMove::update::cStateNone pos", mTarget.getPosition());

         pUnit->moveToLocation(mTarget.getPosition());
         
         setState(cStateWorking);
         break;
      }
      
      case cStateWorking:
      {
         if (!pUnit->isMoving())
         {
            setState(cStateWait);
            break;
         }
      }
      break;
   
      case cStateWait:
      {
         if (pUnit->isMoving())
         {
            setState(cStateWorking);
            break;
         }
      }

      break;
   }

   //-- update our current waypoint
   setTargetPosition(pUnit->getTargetLocation());

   //-- Update unit's target location, and position/velocity
   if(pUnit->getFlag(BEntity::cFlagPhysicsControl) == false)
   {
      if(pUnit->getOrientationController() != mID)
      {
         //-- Only take control if no one else is connected
         if(pUnit->getOrientationController() == -1)
         {
            pUnit->setOrientationController(this);
            pUnit->setHasFacingCommand(false);
         }
      }

      BVector currentPosition = pUnit->getPosition();

      if(mMovementHelper.updateTargetLocation(pUnit, true))
      {
         bool orienControl = (pUnit->getOrientationController() == mID);
         mMovementHelper.doUnitMovement(pUnit, elapsed, orienControl, pUnit->getSpeedScale());
      }

      BVector newPosition = pUnit->getPosition();

      if (pUnit->isMoving())
         updateAnimationRate(elapsed, currentPosition, newPosition);
      else
         pUnit->setAnimationRate(1.0f);
   }

   //-- now do some housekeeping
   if (!BAction::update(elapsed))
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionMove::updateAnimationRate(float elapsed, BVector currentPosition, BVector newPosition)
{
   BUnit* pUnit=mpOwner->getUnit();

   float extractedDistance = 0.0f;
   BGrannyInstance *pInstance = (BGrannyInstance *) pUnit->getVisual()->mpInstance;
   bool hasMotionExtraction = false;
   if( pInstance )
   {
      hasMotionExtraction = pInstance->hasMotionExtraction();
   }
   if (hasMotionExtraction)
   {
      BMatrix extractedMotion;
      extractedMotion.makeIdentity();
      pInstance->getExtractedMotion(elapsed, extractedMotion);
      BVector extractedPosition = extractedMotion.getRow(3);
      extractedDistance = extractedPosition.length();
   }

   if (hasMotionExtraction && (extractedDistance > cFloatCompareEpsilon))
   {
      BVector trajectory = newPosition - currentPosition;
      float distance = trajectory.length();

      if (distance > cFloatCompareEpsilon)
      {
         trajectory /= distance; // normalize
         float speed = Math::Max(trajectory.dot(pUnit->getForward()), 0.0f);
         float rate = speed * distance / extractedDistance;
         pUnit->setAnimationRate(Math::Min(rate, 2.0f));
      }
      else
         pUnit->setAnimationRate(0.0f);

      pUnit->setPosition(currentPosition);
   }
   else
   {
      pUnit->setAnimationRate(1.0f);
      pUnit->setPosition(newPosition);
   }
};

//==============================================================================
//==============================================================================
void BUnitActionMove::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   BUnit* pUnit=mpOwner->getUnit();

   switch (eventType)
   {
      case BEntity::cEventStopped:
      {
         if(senderID == mpOwner->getID())
         {
            // Set the units goal vector (the direction the unit should face) to the target facing if we have one
            long orienController = pUnit->getOrientationController();
            if(orienController == mID || orienController == -1)
            {
               BSquad* pSquad = pUnit->getParentSquad();
               if(pSquad)
               {
                  BVector facing = pSquad->getTargetFacing();
                  if (facing != cInvalidVector)
                     pUnit->setGoalVector(facing);
                  else
                     pUnit->setGoalVector(pUnit->getForward());
                  pUnit->setHasFacingCommand(true);
               }
            }
            setState(cStateDone);
         }         
         break;
      }
   }
}

*/