//============================================================================
// File: physicshoverflightaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicshoverflightaction.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "debugprimitives.h"
#include "configsgame.h"
#include "world.h"
#include "physics.h"
#include "physicsworld.h"
#include "usermanager.h"
#include "user.h"
#include "unitactionrangedattack.h"
#include "unitactionavoidcollisionair.h"
#include "protoobject.h"
#include "squadactionmove.h"
#include "squadactiontransport.h"
#include "tactic.h"

//============================================================================
// Constants

// Chassis inertia constants
const float cChassisUnitInertiaYaw = 1.0f; 
const float cChassisUnitInertiaRoll = 0.4f; 
const float cChassisUnitInertiaPitch = 1.0f; 

const float cClimbRateMult = 1.0f;
const float cDescentRateMult = 0.5f;
const float cRadZ = 4.0f;
const float cRadX = 4.0f;

// Acceleration and damping constants
const float cAccelerationFactor = 1.0f;

const float cAccelRate = 1.0f;
const float cDecelRate = -1.0f;
const float cTurnK = 5.0f;
const float cTurnDamp = 0.0f;

// Reorientation constants
const hkVector4 cReorientRotationAxis(0.0f, 0.0f, 1.0f, 0.0f);
const hkVector4 cReorientUpAxis(0.0f, 1.0f, 0.0f, 0.0f);
const float cReorientStrength = 0.5f;
const float cReorientDamping = 0.1f;

//const float cNoTurnVel = 20.0f;
const float cNoTurnDist = 5.0f;

const float cMinAGLAlt = 5.0f;

//============================================================================
//============================================================================
BPhysicsHoverFlightAction::BPhysicsHoverFlightAction(hkpRigidBody* body, BEntity* pEntity) :
   hkpUnaryAction( body ),
   mPrevTurning(0.0f),
   mReorient(true)
{
   mEntity = pEntity;
   mDesiredPos = mEntity->getPosition();
   mVertAvoidOffset = 0.0f;
   mTimeSinceStopped = 0.0f;

   // Init inertia tensor
   float mass = body->getMass();

   hkMatrix3 matrix;
   matrix.setIdentity();
   matrix.setDiagonal(cChassisUnitInertiaPitch * mass, cChassisUnitInertiaYaw * mass, cChassisUnitInertiaRoll * mass);

   body->setInertiaLocal(matrix);

   for (int i=0; i<5; i++)
      mLookAheadHeight[i] = -10000.0f;
   mLookAheadIndex = 0;
}

//============================================================================
//============================================================================
void BPhysicsHoverFlightAction::applyAction( const hkStepInfo& stepInfo )
{
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);

   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   BSquadActionMove* pSMA = NULL;
   BSquad* pParentSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
   if (pParentSquad)
   {
      pSMA = reinterpret_cast<BSquadActionMove*>(pParentSquad->getActionByType(BAction::cActionTypeSquadMove));
      BVector squadPos = pParentSquad->getPosition();
      squadPos.y = (reinterpret_cast<BUnit*>(mEntity))->getPosition().y;
      pParentSquad->setPosition(squadPos);
   }

   //============================================================================
   // Input altitude control
   BUnit*  pUnit = reinterpret_cast<BUnit*>(mEntity);
   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->getProtoAction())
      mHoverAltitudeOffset = pUnitAvoidAction->getProtoAction()->getHoverAltitudeOffset();
   else
      mHoverAltitudeOffset = 0.0f;

   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
   {
      BVector crashPos = pUnitAvoidAction->getCrashPos();
      bool bKamikaze = false;
      if (pUnitAvoidAction->getKamikazeTarget() && pUnitAvoidAction->getKamikazeTarget()->isAlive())
      {
         crashPos = pUnitAvoidAction->getKamikazeTarget()->getPosition();
         bKamikaze = true;
      }

      // Calculate a throttle setting to change altitude
      // Init baseline hover altitude
      float goalAltMSL = crashPos.y;
      float currAltMSL = mEntity->getPosition().y;
      float goalAltChange = goalAltMSL - currAltMSL;

      BVector vecThroughTgt = crashPos - pUnit->getPosition();
      vecThroughTgt.normalize();
      vecThroughTgt *= 50.0f;
      vecThroughTgt.y -= 100.0f;
      mDesiredPos = crashPos + vecThroughTgt;  // Aim through the target to keep speed up

      const hkVector4 velWorld = pRB->getLinearVelocity();
      float currClimbRate = velWorld(1);

      BVector horizVel;
      horizVel.x = velWorld(0);
      horizVel.y = 0.0f;
      horizVel.z = velWorld(2);
      float horizSpeed = horizVel.length();
      if (horizSpeed < cFloatCompareEpsilon)
         horizSpeed = cFloatCompareEpsilon;
      float horizDist = pUnit->getPosition().xzDistance(crashPos);
      float timeToImpact = horizDist / horizSpeed;
      if (timeToImpact < cFloatCompareEpsilon)
         timeToImpact = cFloatCompareEpsilon;
      float goalClimbRate = goalAltChange / timeToImpact;

      if (!bKamikaze) // Don't exceed max speed if just crashing. If in kamikaze mode, do what it takes to get to the target
      {
         float maxSpeed = pUnit->getProtoObject()->getDesiredVelocity();
         goalClimbRate = Math::Clamp<float>(goalClimbRate, -maxSpeed, maxSpeed);
      }
      float throttle = 1.2f * (goalClimbRate - currClimbRate);

      // Apply vertical force
      hkVector4 counterGravImpulse;
      counterGravImpulse.setMul4(pRB->getMass() * -1.0f * stepInfo.m_deltaTime, gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());

      hkVector4 hoverImpulse;
      hoverImpulse.setMul4(stepInfo.m_deltaTime * pRB->getMass() * throttle, cReorientUpAxis);

      pRB->applyLinearImpulse(counterGravImpulse + hoverImpulse);
   }
   else
   {
      float terrainOffset = 0.0f;
      bool useTerrainOffset = false;
      //XXXHalwes - 8/1/2008 - May use this again, but polished animations seem to be doing the trick.
      if (pUnit->isType(gDatabase.getOTIDTransporter()))
      {
//-- FIXING PREFIX BUG ID 1256
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {
            const BSquadActionTransport* pAction = (const BSquadActionTransport*)(pSquad->getActionByTypeConst(BAction::cActionTypeSquadTransport));
            if (pAction)
               useTerrainOffset = pAction->getHoverOffset(terrainOffset);
         }
      }

      float height[4];
      float flightHeight[4];
      BVector pos[4];
      BMatrix matrix;
      mEntity->getWorldMatrix(matrix);

      matrix.transformVectorAsPoint(BVector(0.0f, 0.0f, cRadZ), pos[0]);
      matrix.transformVectorAsPoint(BVector(-cRadX, 0.0f, -cRadZ), pos[1]);
      matrix.transformVectorAsPoint(BVector(cRadX, 0.0f, -cRadZ), pos[2]);

      int numPoints = 3;
      BVector futurePosition;

      if (pSMA)
      {
         float desiredTimeIntoFuture = 0.2f * (1.0f + mLookAheadIndex);
         float realTimeNeeded;
         pSMA->getFuturePosition(desiredTimeIntoFuture, true, futurePosition, realTimeNeeded);
         pos[3] = futurePosition;
         numPoints = 4;
      }

      // Check flight height if the data exists, otherwise use camera height or terrain height and bump the altitude up to nominal flight height (+16)
      for (int i=0; i<numPoints; i++)
      {
         gTerrainSimRep.getHeight(pos[i], true);
         height[i] = pos[i].y + 8.0f;

         BASSERTM(gTerrainSimRep.flightHeightsLoaded(), "No flight mesh found for this level - Load in PhoenixEditor and Re-export.");

         if (useTerrainOffset)
            gTerrainSimRep.getHeight(pos[i], true);
         else if (gTerrainSimRep.flightHeightsLoaded())
         {
            BVector startPos = pos[i];
            startPos.y += 100.0f;
            gTerrainSimRep.getFlightHeightRaycast(startPos, pos[i].y, true, true);
         }
         else if (gTerrainSimRep.cameraHeightsLoaded())
         {
            gTerrainSimRep.getCameraHeight(pos[i], true);
            pos[i].y += 16.0f;
         }
         else
         {
            gTerrainSimRep.getHeight(pos[i], true);
            pos[i].y += 16.0f;
         }
         flightHeight[i] = pos[i].y;

         if (!useTerrainOffset)
         {
            if ((height[i] + cMinAGLAlt) > (flightHeight[i] + mHoverAltitudeOffset))
               flightHeight[i] = height[i] + cMinAGLAlt - mHoverAltitudeOffset;
         }

      }

      // Find the highest look ahead point
      float maxGroundHeight = -10000.0f;
      if (numPoints == 4)
      {
         mLookAheadHeight[mLookAheadIndex] = flightHeight[3];
         for (uint i = 0; i < 5; i++)
         {
            if (mLookAheadHeight[i] > maxGroundHeight)
               maxGroundHeight = flightHeight[3] = mLookAheadHeight[i];
         }
      }

      // Find the highest of all current test points
      for (int i = 0; i < numPoints; i++)
      {
         if (flightHeight[i] > maxGroundHeight)
            maxGroundHeight = flightHeight[i];
      }

      // Calculate a throttle setting to change altitude
      // Init baseline hover altitude
      float goalAltMSL;
      if (useTerrainOffset)
         goalAltMSL = maxGroundHeight + terrainOffset;
      else
         goalAltMSL = maxGroundHeight + mHoverAltitudeOffset + mVertAvoidOffset;
      float currAltMSL = mEntity->getPosition().y;
      float goalAltChange = goalAltMSL - currAltMSL;
      float goalClimbRate = cClimbRateMult * goalAltChange;
      // Limit descent rate to prevent diving too fast
      if (goalClimbRate < 0.0f)
         goalClimbRate *= cDescentRateMult;

      const hkVector4 velWorld = pRB->getLinearVelocity();
      float currClimbRate = velWorld(1);
      float throttle = cClimbRateMult * (goalClimbRate - currClimbRate);

      // Apply vertical force
      hkVector4 counterGravImpulse;
      counterGravImpulse.setMul4(pRB->getMass() * -1.0f * stepInfo.m_deltaTime, gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());

      hkVector4 hoverImpulse;
      hoverImpulse.setMul4(stepInfo.m_deltaTime * pRB->getMass() * throttle, cReorientUpAxis);

      pRB->applyLinearImpulse(counterGravImpulse + hoverImpulse);

      mLookAheadIndex++;
      if (mLookAheadIndex >= 5)
         mLookAheadIndex = 0;
   }

   //============================================================================
   // Input acceleration and turning
   {
      // Get current chassis rotation
      hkRotation chassisRotation;
      chassisRotation.set(pRB->getRotation());

      // Apply acceleration / turning based on controller 2 input
      float forwardAccel = 0.0f;
      float rightAccel = 0.0f;
      float yaw = 0.0f;
      float pitch = 0.0f;
      float roll = 0.0f;
      bool atGoal = false;

      // Calculate acceleration / turning to get to desired position
      calcMovement(stepInfo, forwardAccel, rightAccel, yaw, pitch, roll, atGoal, pSMA);

      pRB->setLinearDamping(0.0f);
      pRB->setAngularDamping(0.0f);

      // Calculate forward acceleration impulse
      hkVector4 forwardImpulse;
      forwardImpulse = chassisRotation.getColumn(2);
      forwardImpulse(1) = 0.0f;
      forwardImpulse.normalize3();
      forwardImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * cAccelerationFactor * forwardAccel);

      // Calculate right acceleration impulse
      hkVector4 rightImpulse;
      rightImpulse = chassisRotation.getColumn(0);
      rightImpulse(1) = 0.0f;
      rightImpulse.normalize3();
      rightImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * cAccelerationFactor * rightAccel);

      // Apply linear impulse
      pRB->applyLinearImpulse(forwardImpulse + rightImpulse);

      // Calculate yaw impulse
      hkVector4 yawImpulse;
      yawImpulse.set(0.0f, stepInfo.m_deltaTime * pRB->getMass() * yaw, 0.0f, 0.0f);
      pRB->applyAngularImpulse(yawImpulse);

      // Calculate pitch impulse
      hkVector4 pitchImpulse = chassisRotation.getColumn(0);
      pitchImpulse(0) *= stepInfo.m_deltaTime * pRB->getMass() * pitch;
      pitchImpulse(1) *= stepInfo.m_deltaTime * pRB->getMass() * pitch;
      pitchImpulse(2) *= stepInfo.m_deltaTime * pRB->getMass() * pitch;
      pRB->applyAngularImpulse(pitchImpulse);

      // Calculate roll impulse
      hkVector4 rollImpulse = chassisRotation.getColumn(2);
      rollImpulse(0) *= stepInfo.m_deltaTime * pRB->getMass() * roll;
      rollImpulse(1) *= stepInfo.m_deltaTime * pRB->getMass() * roll;
      rollImpulse(2) *= stepInfo.m_deltaTime * pRB->getMass() * roll;
      pRB->applyAngularImpulse(rollImpulse);
   }

   // Reorient towards up axis to prevent or limit flipping over
   if (mReorient)
      reorient(stepInfo);
}


//============================================================================
//============================================================================
void BPhysicsHoverFlightAction::calcMovement(const hkStepInfo& stepInfo, float& fwdAccel, float& rightAccel, float& yaw, float& pitch, float& roll, bool& atGoal, BSquadActionMove* pSMA)
{
   atGoal = false;
   

   //============================================================================
   // Update new desired position (allow acceleration if desired position changed)
   float maxVelocity = 0.0f;
   BVector currentPos = mEntity->getPosition();
   float desiredPosIsGoal = true;

   // Get desired position and velocity (from squad move action or other source)
   BUnit*  pUnit = reinterpret_cast<BUnit*>(mEntity);
   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   BSquad* pSquad = pUnit->getParentSquad();
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
   {
      maxVelocity = pUnit->getProtoObject()->getDesiredVelocity();
      if (pUnitAvoidAction->getKamikazeTarget() && pUnitAvoidAction->getKamikazeTarget()->isAlive())
         maxVelocity *= 3.0f;
   }
   else if (pSMA)
   {
      mDesiredPos = pSMA->getOwner()->getPosition();
      desiredPosIsGoal = (pSMA->getState() != BAction::cStateWorking);
      maxVelocity = pSMA->getVelocity();
   }
   // If there is no squad move action, use proto data and the squad position
   else
   {
      // Get max velocity
      BASSERT(mEntity->getUnit()->getProtoObject());
      maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();
      if(pSquad)
         mDesiredPos = pSquad->getPosition();
   }

   float decelRate = maxVelocity * cDecelRate;

   // Limit speed for the birth duration of aircraft to help them clear the command center on the way out
   if (pUnitAvoidAction && pUnitAvoidAction->speedLimited())
      maxVelocity = 8.0f;

   //============================================================================
   // Get current and desired xz directions
   BVector currentFwd = mEntity->getForward();
   BVector desiredFwd = (mDesiredPos - currentPos);
   BVector currentRight = mEntity->getRight();
   currentFwd.y = 0.0f;
   desiredFwd.y = 0.0f;
   currentRight.y = 0.0f;
   currentFwd.normalize();
   if (!desiredFwd.safeNormalize())
      desiredFwd = currentFwd;
   currentRight.normalize();

   // Current xyz linear velocity
//-- FIXING PREFIX BUG ID 1258
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BVector linVel = pRB->getLinearVelocity();

   // Check stopping distance given current velocity
   // If we're within stopping distance, slow us down.
   // Otherwise accelerate to max velocity
   float currentVelAlongFwd = linVel.dot(currentFwd);
   float currentVelAlongRight = linVel.dot(currentRight);
   float dist = mDesiredPos.xzDistance(currentPos);
   float maxStoppingDist = 0.0f;
   if (maxVelocity != 0.0f && decelRate != 0.0f)
      maxStoppingDist = 0.5f * maxVelocity * maxVelocity / -decelRate;

   static float cFwdK = 1.0f;
   static float cFwdDamp = 0.0f;
   static float cStopFwdK = 1.0f;
   static float cStopFwdDamp = 1.0f;
   static float cStopRightK = 1.0f;
   static float cStopRightDamp = 0.0f;
   static float cRightK = 1.0f;
   static float cRightDamp = 0.0f;
   float maxFwdAccel;
   float maxRightAccel;

   // Check for collision avoidance reaction
   if (pUnitAvoidAction && pUnitAvoidAction->Avoiding())
   {
      float alt = pRB->getPosition().getQuad().y;
      if ((alt >= pUnitAvoidAction->getNearestObstacleAlt()) &&
         (alt <= pUnitAvoidAction->getNearestObstacleAlt() + 30.0f))
         mVertAvoidOffset = 20.0f - (alt - pUnitAvoidAction->getNearestObstacleAlt());
   }
   else if (mVertAvoidOffset > 0.1f)
      mVertAvoidOffset -= 0.05f;
   else if (mVertAvoidOffset < -0.1f)
      mVertAvoidOffset += 0.05f;
   else
      mVertAvoidOffset = 0.0f;

   // If in stasis, hold position or slow down
   float stasisSpeedMult = 1.0f;
   if(pSquad)
      stasisSpeedMult = pSquad->getStasisSpeedMult();

   // Decel
   float reverseSpeed = pUnit->getReverseSpeed();
   if (desiredPosIsGoal && (dist < maxStoppingDist))
   {
      atGoal = true;      

      float desiredVelocity = stasisSpeedMult * maxVelocity * dist / maxStoppingDist;

      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * desiredVelocity;

      // Don't allow vehicles (like Banshees) that cannot back up to do so.
      // Also limit reverse speed if the object has a maximum reverse speed specified
      if (reverseSpeed == -1.0f) // Not Set - so assume same as maxVel
         reverseSpeed = pUnit->getMaxVelocity();
      if (desiredVelAlongFwd < -reverseSpeed)
         desiredVelAlongFwd = -reverseSpeed;

      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      float fwdDampConstant = cStopFwdDamp * 2.0f * sqrt(cStopFwdK);
      fwdAccel = cStopFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;

      float desiredVelAlongRight = desiredFwd.dot(currentRight) * desiredVelocity;
      float rightVelDiff = desiredVelAlongRight - currentVelAlongRight;
      float rightDampConstant = cStopRightDamp * 2.0f * sqrt(cStopRightK);
      rightAccel = cStopRightK * rightVelDiff - rightDampConstant * currentVelAlongRight;

      maxFwdAccel = cStopFwdK * maxVelocity;
      maxRightAccel = cStopRightK * maxVelocity;
   }
   else
   {
      float desiredVelAlongFwd = stasisSpeedMult * desiredFwd.dot(currentFwd) * maxVelocity;

      // Don't allow vehicles (like Banshees) that cannot back up to do so.
      // Also limit reverse speed if the object has a maximum reverse speed specified
      if (reverseSpeed == -1.0f) // Not Set - so assume same as maxVel
         reverseSpeed = pUnit->getMaxVelocity();
      if (desiredVelAlongFwd < -reverseSpeed)
         desiredVelAlongFwd = -reverseSpeed;

      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      float fwdDampConstant = cFwdDamp * 2.0f * sqrt(cFwdK);
      fwdAccel = cFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;

      float desiredVelAlongRight = stasisSpeedMult * desiredFwd.dot(currentRight) * maxVelocity;
      float rightVelDiff = desiredVelAlongRight - currentVelAlongRight;
      float rightDampConstant = cRightDamp * 2.0f * sqrt(cRightK);
      rightAccel = cRightK * rightVelDiff - rightDampConstant * currentVelAlongRight;

      maxFwdAccel = cFwdK * maxVelocity;
      maxRightAccel = cRightK * maxVelocity;
   }

   //============================================================================
   // Yaw, pitch and roll

   // If in stasis
   if (stasisSpeedMult == 0.0f)
   {
      yaw = pitch = roll = 0.0f;
      return;
   }

   // Check to see if we have an attack target to turn towards
   bool attackAction = false;
//-- FIXING PREFIX BUG ID 1259
   const BUnitActionRangedAttack* pAction = reinterpret_cast<BUnitActionRangedAttack*>(mEntity->getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
   float yawFactor = mEntity->getUnit()->getProtoObject()->getTurnRate() / 120.0f;
   float pitchFactor = 30.0f / 40.0f;
   float rollFactor = 60.0f / 40.0f;
   if (pUnitAvoidAction && pUnitAvoidAction->getProtoAction())
   {
      pitchFactor = pUnitAvoidAction->getProtoAction()->getMaxPitch() / 40.0f;
      rollFactor = pUnitAvoidAction->getProtoAction()->getMaxRoll() / 40.0f;
   }

   bool bIgnoreTarget = false;
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
      bIgnoreTarget = true;

   if (pAction && pAction->getTarget() && !bIgnoreTarget)
   {
      attackAction = true;

      const BSimTarget* pSimTarget = pAction->getTarget();
//-- FIXING PREFIX BUG ID 1257
      const BEntity* pTarget = gWorld->getEntity(pSimTarget->getID());
//--
      BVector targetPos = (pTarget ? pTarget->getPosition() : pSimTarget->getPosition());
      
      BVector targetDir = targetPos - currentPos;
      targetDir.y = 0.0f;
      targetDir.safeNormalize();

      if (attackAction && ((reverseSpeed > 0.0f) || (atGoal || (desiredFwd.dot(targetDir) > 0.7f))))
         desiredFwd = targetDir;
   }

   // No turning if we're at the goal or not accelerating
   if ((!attackAction || bIgnoreTarget) && (atGoal || (desiredPosIsGoal && (dist < cNoTurnDist))))
   {
      // Damp out current angular velocity
      hkVector4 currentAngVel = pRB->getAngularVelocity();
      const float stopTurnFactor = -0.5f;
      yaw = stopTurnFactor * currentAngVel(1);
   }
   // Turn towards desired
   else
   {
      float angleDiff = desiredFwd.angleBetweenVector(currentFwd);
      if (desiredFwd.cross(currentFwd).y > 0.0f)
         angleDiff = -angleDiff;

      // Calculate yaw to move towards desired using an overdamped spring
      hkVector4 currentAngVel = pRB->getAngularVelocity();

      if (attackAction) // attack orientation if stiff spring
      {
         const float cAttackTurnK = 5.0f;
         const float cAttackTurnDamp = 0.0f;
         float attackTurnDampConstant = cAttackTurnDamp * 2.0f * sqrt(cAttackTurnK);
         yaw = yawFactor * cAttackTurnK * angleDiff - attackTurnDampConstant * currentAngVel(1);
      }
      else
      {
         float turnDampConstant = cTurnDamp * 2.0f * sqrt(cTurnK);
         yaw = yawFactor * cTurnK * angleDiff - turnDampConstant * currentAngVel(1);
      }
   }

   // Attenuate pitch and roll in the beginning of a move - and ramp it up over time
   float timeDampenFactor = mTimeSinceStopped;
   if (timeDampenFactor > 1.0f)
      timeDampenFactor = 1.0f;

   // Pitch
   // Calculate pitch to move towards desired using an overdamped spring
   float maxPitchAngle = Math::fDegToRad(45.0f * pitchFactor);
   float currPitchAngle;
   float goalPitchAngle;
   BVector currentFwdVec = mEntity->getForward();
   currPitchAngle = asin(currentFwdVec.y);

   // Pitch due to climb rate
   BVector horizVel = linVel;
   horizVel.y = 0.0f;
   float horizSpeed = horizVel.length();
   if (horizSpeed < 0.01f)
      horizSpeed = 0.01f;
   float climbAngle = atanf(linVel.y / horizSpeed);
   if (maxVelocity == 0.0f)
      goalPitchAngle = 0.0f;
   else
   {
      if (maxVelocity < horizSpeed)
         maxVelocity = horizSpeed;

      goalPitchAngle = 0.25f * climbAngle * horizSpeed / maxVelocity;

      if (maxFwdAccel > cFloatCompareEpsilon)
      {
         // Additional pitch due to acceleration
         goalPitchAngle -= (fwdAccel / maxFwdAccel) * maxPitchAngle * 2.0f;
      }
   }

/*
   // Additional pitch due to velocity
   if ((maxVelocity > cFloatCompareEpsilon && fwdAccel > 0.0f) || (fabs(maxVelocity) > cFloatCompareEpsilon && fwdAccel < 0.0f))
      goalPitchAngle -= Math::fDegToRad(15.0f * pitchFactor) * currentVelAlongFwd / maxVelocity;
//   else
//      goalPitchAngle = 0.0f;
*/

//   if (goalPitchAngle >  maxPitchAngle)  goalPitchAngle =  maxPitchAngle;
//   if (goalPitchAngle < -maxPitchAngle)  goalPitchAngle = -maxPitchAngle;

   float angleDiff = goalPitchAngle - currPitchAngle;

   pitch = -10.0f * angleDiff * timeDampenFactor;

   // Pivot Engines (if required)
   const BProtoObject* protoObj = mEntity->getUnit()->getProtoObject();
   if (protoObj->getFlagHasPivotingEngines())
   {

      // Front Left Engine
      long frontLeftTurbineID = protoObj->findHardpoint("TurbineFL");
      const BHardpoint *pFrontLeftTurbineHP = mEntity->getUnit()->getHardpoint(frontLeftTurbineID);
      if (pFrontLeftTurbineHP)
      {
         float goalAngle = 0.1f * yaw;
         mEntity->getUnit()->pitchHardpointToGoalAngle(frontLeftTurbineID, goalAngle, stepInfo.m_deltaTime);
      }

      // Front Right Engine
      long frontRightTurbineID = protoObj->findHardpoint("TurbineFR");
      const BHardpoint *pFrontRightTurbineHP = mEntity->getUnit()->getHardpoint(frontRightTurbineID);
      if (pFrontRightTurbineHP)
      {
         float goalAngle = 0.1f * -yaw;
         mEntity->getUnit()->pitchHardpointToGoalAngle(frontRightTurbineID, goalAngle, stepInfo.m_deltaTime);
      }

      // Rear Left Engine
      long rearLeftTurbineID = protoObj->findHardpoint("TurbineRL");
      const BHardpoint *pRearLeftTurbineHP = mEntity->getUnit()->getHardpoint(rearLeftTurbineID);
      if (pRearLeftTurbineHP)
      {
         float goalAngle = 1.0f * yaw + 30.0f * currentVelAlongFwd / maxVelocity;
         mEntity->getUnit()->pitchHardpointToGoalAngle(rearLeftTurbineID, goalAngle, stepInfo.m_deltaTime);
      }

      // Rear Right Engine
      long rearRightTurbineID = protoObj->findHardpoint("TurbineRR");
      const BHardpoint *pRearRightTurbineHP = mEntity->getUnit()->getHardpoint(rearRightTurbineID);
      if (pRearRightTurbineHP)
      {
         float goalAngle = 1.0f * -yaw + 30.0f * currentVelAlongFwd / maxVelocity;
         mEntity->getUnit()->pitchHardpointToGoalAngle(rearRightTurbineID, goalAngle, stepInfo.m_deltaTime);
      }
   }

   // Roll
   // Calculate roll to move towards desired using an overdamped spring
   float maxRollAngle = Math::fDegToRad(60.0f * rollFactor);
   float currRollAngle;
   float goalRollAngle;
   BVector currentRightVec = mEntity->getRight();
   currRollAngle = asin(currentRightVec.y);

   if (maxRightAccel > cFloatCompareEpsilon)
      goalRollAngle = (rightAccel / maxRightAccel) * maxRollAngle;
   else
      goalRollAngle = 0.0f;

   if (maxVelocity > cFloatCompareEpsilon)
      goalRollAngle += Math::fDegToRad(60.0f * rollFactor) * currentVelAlongRight / maxVelocity;
   else
      goalRollAngle = 0.0f;

   angleDiff = goalRollAngle - currRollAngle;

   roll = -10.0f * angleDiff * timeDampenFactor;

   mTimeSinceStopped += stepInfo.m_deltaTime;
   if (linVel.length() < 0.3f)
      mTimeSinceStopped = 0.0f;
}


//============================================================================
// This code is from hkpReorientAction::applyAction in Havok's sdk
//============================================================================
void BPhysicsHoverFlightAction::reorient(const hkStepInfo& stepInfo)
{
   // Move the world axis into body space
   hkpRigidBody* rb = static_cast<hkpRigidBody*>( m_entity );

   hkVector4 bodyUpAxis;		bodyUpAxis.setRotatedDir( rb->getRotation(), cReorientUpAxis );
   hkVector4 bodyRotationAxis;	bodyRotationAxis.setRotatedDir( rb->getRotation(), cReorientRotationAxis );

   // Project the world up axis onto the plane that has
   // a normal the same as the body rotation axis.
   hkVector4 projectedUpAxis = cReorientUpAxis;
   const hkReal distance = projectedUpAxis.dot3( bodyRotationAxis );
   projectedUpAxis.addMul4( -distance, bodyRotationAxis );
   projectedUpAxis.normalize3();

   // Get angle between the up axis of the rigid body
   // and the project world up axis.
   hkReal angle = hkMath::acos( bodyUpAxis.dot3( projectedUpAxis ) );

   // If we cross the rigid body up axis with the projected 
   // world up axis we should get a vector thats runs parallel 
   // to the bodies rotation axis. The sign of the value
   // representing the major axis of each vector should match.
   // When the signs don't match then the calculated rotation
   // axis runs in the opposite direction and we need to flip
   // the sign of the angle.
   hkVector4 calculatedRotationAxis; calculatedRotationAxis.setCross( bodyUpAxis, projectedUpAxis );
   hkReal cra	= calculatedRotationAxis( calculatedRotationAxis.getMajorAxis() );
   hkReal ra	= bodyRotationAxis( bodyRotationAxis.getMajorAxis() );
   if( !(hkMath::isNegative( cra ) == hkMath::isNegative( ra )) )
   {
      angle = -angle;
   }

   // Apply the orientating angular impulse including an angular
   // velocity damping factor.
   hkVector4 av; 
   av.setMul4( cReorientDamping * stepInfo.m_invDeltaTime, rb->getAngularVelocity() );

   hkVector4 impulse; 
   impulse.setMul4( cReorientStrength * angle * stepInfo.m_invDeltaTime, bodyRotationAxis );
   impulse.sub4( av );

   rb->applyAngularImpulse( impulse );
}


