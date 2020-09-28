//============================================================================
// File: physicsghostaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicsghostaction.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "debugprimitives.h"
#include "configsgame.h"
#include "world.h"
#include "physics.h"
#include "physicsworld.h"
#include "usermanager.h"
#include "user.h"
#include "unitactionmoveghost.h"
#include "unitactionrangedattack.h"
#include "protoobject.h"
#include "terrainphysicsheightfield.h"
#include "squadactionmove.h"

#define _NEWWARTHOG_MOVEMENT

//============================================================================
// Constants

const float cExtraGravityFactor = (4.0f - GAMEUNITS_PER_METER_FACTOR) / GAMEUNITS_PER_METER_FACTOR;

// Chassis inertia constants
const float cChassisUnitInertiaYaw = 1.0f; 
const float cChassisUnitInertiaRoll = 0.4f; 
const float cChassisUnitInertiaPitch = 1.0f; 

// Suspension constants
const float cSpringLength = 1.3f;
const float cWheelRadius = 1.0f;
const float cSpringStrength = 30.0f;
const float cDampingCompression = 1.0f;
const float cDampingRelaxation = 0.5f;
const float cNormalClippingAngle = 0.1f;

const float cHardpointFrontZ = 1.3f;
const float cHardpointBackZ = -1.1f;
const float cHardpointY = -0.05f;
const float cHardpointX = 1.1f;
const hkVector4 cHardpoints[4] = 
{
   hkVector4(-cHardpointX, cHardpointY, cHardpointFrontZ, 0.0f),
   hkVector4( cHardpointX, cHardpointY, cHardpointFrontZ, 0.0f),
   hkVector4(-cHardpointX, cHardpointY, cHardpointBackZ, 0.0f),
   hkVector4( cHardpointX, cHardpointY, cHardpointBackZ, 0.0f)
};

// Acceleration and damping constants
const float cAccelerationFactor = 1.0f;
const float cTurningFactor = 1.0f;
const float cLinearVelDampingBase = 0.1f;
const float cLinearVelDampingPerTire = 1.0f;
const float cAngularVelDamping = 2.0f;
const float cDirectInputForwardAccelRate = 30.0f;
const float cDirectInputRightAccelRate = 30.0f;
const float cDirectInputTurnRate = 0.1f;

const float cAccelRate = 1.0f;
const float cDecelRate = -1.0f;
const float cTurnK = 10.0f;
const float cTurnDamp = 0.0f;
const float cAttackTurnK = 10.0f;
const float cAttackTurnDamp = 0.0f;
const float cFwdK = 2.5f;
const float cFwdDamp = 0.0f;
const float cStopFwdK = 5.0f;
const float cStopFwdDamp = 0.0f;
const float cRightK = 2.5f;
const float cRightDamp = 0.0f;

// Friction constants
const float cMaxSideFriction = -20.0f;
const float cMaxForwardFriction = -100.0f;
const float cMinSideFriction = -3.0f;
const float cMinForwardFriction = 0.0f;
const float cBackTireSideFrictionFactor = 0.1f;

const float cOverturnTorque = 20.0f;

// Wheel visual constants
const float cSpinFactor = 2.0f;
const float cMaxWheelTurn = cPiOver2;
const float cSpringStableLength = 1.0f;
const float cWheelHeightMin = -0.4f;
const float cWheelHeightMax = 0.3f;

// Reorientation constants
const hkVector4 cReorientRotationAxis(0.0f, 0.0f, 1.0f, 0.0f);
const hkVector4 cReorientUpAxis(0.0f, 1.0f, 0.0f, 0.0f);
const float cReorientStrength = 0.5f;
const float cReorientDamping = 0.1f;

const float cNoTurnVel = 20.0f;
const float cNoTurnDist = 5.0f;


//============================================================================
//============================================================================
BPhysicsGhostAction::BPhysicsGhostAction(hkpRigidBody* body, BEntity* pEntity) :
   hkpUnaryAction( body ),
   mPrevTurning(0.0f),
   mReorient(true)
{
   mCurrentSuspensionLengths[0] = cSpringLength;
   mCurrentSuspensionLengths[1] = cSpringLength;
   mCurrentSuspensionLengths[2] = cSpringLength;
   mCurrentSuspensionLengths[3] = cSpringLength;

   mEntity = pEntity;
   mDesiredPos = mEntity->getPosition();

   // Init inertia tensor
   float mass = body->getMass();

   hkMatrix3 matrix;
   matrix.setIdentity();
   matrix.setDiagonal(cChassisUnitInertiaPitch * mass, cChassisUnitInertiaYaw * mass, cChassisUnitInertiaRoll * mass);

   body->setInertiaLocal(matrix);
}

//============================================================================
//============================================================================
void BPhysicsGhostAction::applyAction( const hkStepInfo& stepInfo )
{
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);

   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   // Get hkpSampledHeightFieldShape for raycasting against
   BASSERT(gTerrainSimRep.getPhysicsHeightfield());
   const hkpEntity* pHeightfieldEntity = gTerrainSimRep.getPhysicsHeightfield()->getHKEntity();
   BASSERT(pHeightfieldEntity->getCollidable());
//-- FIXING PREFIX BUG ID 4743
   const hkpSampledHeightFieldShape* pHeightfield = (hkpSampledHeightFieldShape*) pHeightfieldEntity->getCollidable()->getShape();
//--
	const hkTransform& heightfieldTrans = pHeightfieldEntity->getCollidable()->getTransform();

   // Get current chassis rotation
   hkRotation chassisRotation;
   chassisRotation.set(pRB->getRotation());

   //============================================================================
   // Suspension - store off several pieces of data for use below
   bool rayHit[4] =  { false, false, false, false };
   float suspensionForces[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
   hkVector4 hardPts[4];
   hkVector4 contactPts[4];
   hkVector4 contactNormals[4];
   {
      // Get orientation and suspension direction (down)
      hkTransform trans;
      trans.set(pRB->getRotation(), pRB->getPosition());

      hkRotation rot;
      rot.set(pRB->getRotation());
#if defined(CODE_ANALYSIS_ENABLED)
	  hkVector4 suspDir = rot.getColumn(1);
	  suspDir.setNeg4(suspDir);
#else
      hkVector4 suspDir = -rot.getColumn(1);
#endif
      // Collide all wheels
      hkVector4 rayEndPts[4];
      float clippedInvContactDotSuspension[4];
      float suspensionRelativeVelocity[4];

      for (uint i = 0; i < 4; i++)
      {
         // Init hardpoint (where suspension is connected to chassis)
         hardPts[i].setTransformedPos(trans, cHardpoints[i]);

         // Init endpoint
#if defined(CODE_ANALYSIS_ENABLED)
		 suspDir.mul4(cSpringLength + cWheelRadius);
		 hardPts[i].add4(suspDir);
		 rayEndPts[i] = hardPts[i];
#else
         rayEndPts[i] = hardPts[i] + suspDir * (cSpringLength + cWheelRadius);
#endif
         // Raycast - don't raycast against the whole world, just the terrain heightfield, this
         // makes for a 2x perf increase
         hkpShapeRayCastInput input;
         input.m_from.setTransformedInversePos(heightfieldTrans, hardPts[i]);
         input.m_to.setTransformedInversePos(heightfieldTrans, rayEndPts[i]);
         input.m_rayShapeCollectionFilter = NULL;
         hkpShapeRayCastOutput output;
         pHeightfield->castRay(input, output);

         //============================================================================
         // Calculate collision info and suspension forces if we got a collision for
         // with this wheel
         if (output.hasHit())
         {
            rayHit[i] = true;
            contactNormals[i] = output.m_normal;

            // Calc length of suspension from hard point to collision point
            hkReal hitDistance = output.m_hitFraction * (cSpringLength + cWheelRadius);
            mCurrentSuspensionLengths[i] = hitDistance - cWheelRadius;

            // Calc contact position
            hkVector4 contactPointWsPosition;
            contactPointWsPosition.setAddMul4(hardPts[i], suspDir, hitDistance);
            contactPts[i] = contactPointWsPosition;

            // Calc the suspension velocity relative to the ground and projected along
            // the collision normal
            hkVector4 chassis_velocity_at_contactPoint;
            pRB->getPointVelocity(contactPts[i], chassis_velocity_at_contactPoint);

            // Assume zero ground velocity for now (we may need this if warthog is driving on a moving surface)
            hkVector4 groundVelocityAtContactPoint;
            //groundRigidBody->getPointVelocity( contactPts[i], groundVelocityAtContactPoint);
            groundVelocityAtContactPoint.set(0.0f, 0.0f, 0.0f, 0.0f);

            hkVector4 chassisRelativeVelocity;
            chassisRelativeVelocity.setSub4( chassis_velocity_at_contactPoint, groundVelocityAtContactPoint);

            hkReal projVel = contactNormals[i].dot3( chassisRelativeVelocity );

            hkReal denominator = contactNormals[i].dot3( suspDir );
            if ( denominator >= -cNormalClippingAngle)
            {
               suspensionRelativeVelocity[i] = 0.0f;
               clippedInvContactDotSuspension[i] = 1.0f / cNormalClippingAngle;
            }
            else
            {
               hkReal inv = -1.f / denominator;
               suspensionRelativeVelocity[i] = projVel * inv;
               clippedInvContactDotSuspension[i] = inv;
            }


            //============================================================================
            // Calc suspension force
            {
               //	Spring constant component
               const hkReal lengthDiff = cSpringLength - mCurrentSuspensionLengths[i];
               suspensionForces[i] = cSpringStrength * lengthDiff * clippedInvContactDotSuspension[i];

               // damping 
               if (suspensionRelativeVelocity[i] < 0.0f)
                  suspensionForces[i] -= cDampingCompression * suspensionRelativeVelocity[i];
               else
                  suspensionForces[i] -= cDampingRelaxation * suspensionRelativeVelocity[i];
            }
/*
#if !defined(CODE_ANALYSIS_ENABLED)
            // Debug rendering
            if (gConfig.isDefined(cConfigDebugRenderShape))
            {
               gpDebugPrimitives->addDebugLine((BVector)hardPts[i], (BVector)contactPts[i], 0xff00ff00, 0xff00ff00);
               gpDebugPrimitives->addDebugLine((BVector)contactPts[i], (BVector)rayEndPts[i], 0xffff0000, 0xffff0000);
            }
#endif
*/
         }
         //============================================================================
         // Not in contact : position wheel in a nice (rest length) position
         else
         {
            rayHit[i] = false;
            contactNormals[i].setNeg4(suspDir);
            contactPts[i] = rayEndPts[i];
            mCurrentSuspensionLengths[i] = cSpringLength;
            clippedInvContactDotSuspension[i] = 1.0f;
            suspensionRelativeVelocity[i] = 0.0f;
            suspensionForces[i] = 0.0f;

/*
#if !defined(CODE_ANALYSIS_ENABLED)
            // Debug rendering
            if (gConfig.isDefined(cConfigDebugRenderShape))
               gpDebugPrimitives->addDebugLine((BVector)hardPts[i], (BVector)rayEndPts[i], 0xffff0000, 0xffff0000);
#endif
*/
         }

         #ifndef BUILD_FINAL
            if (output.hasHit())
            {
               mSuspensionDist[i] = output.m_hitFraction;
            }
            else
            {
               mSuspensionDist[i] = 1.0f;
            }
         #endif
      }
   }

   //============================================================================
   // Apply extra gravity
   {
      hkVector4 gravForce;
      gravForce.setMul4(pRB->getMass() * cExtraGravityFactor, gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());
      pRB->applyForce( stepInfo.m_deltaTime, gravForce );
   }

   //============================================================================
   // Apply suspension forces
   {
      for (uint i = 0; i < 4; i++)
      {
         // Apply suspension force in direction of contact normal at the hard point
         if (suspensionForces[i] > 0.0f)
         {
            hkVector4 suspImpulse;
            suspImpulse.setMul4(stepInfo.m_deltaTime * pRB->getMass() * suspensionForces[i], contactNormals[i]);
            pRB->applyPointImpulse(suspImpulse, hardPts[i]);
         }
      }
   }

   // Grab warthog move action and set in air state
   /*
   BUnitActionMoveGhost* pGMA = reinterpret_cast<BUnitActionMoveGhost*>(mEntity->getActionByType(BAction::cActionTypeUnitMoveGhost));
   if (pGMA)
   {
      bool inAir = (!rayHit[0] && !rayHit[1] && !rayHit[2] && !rayHit[3]);
      pGMA->setInAir(inAir);
   }
   */
   BSquadActionMove* pSMA = NULL;
   BSquad* pParentSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
   if (pParentSquad)
      pSMA = reinterpret_cast<BSquadActionMove*>(pParentSquad->getActionByType(BAction::cActionTypeSquadMove));

   //============================================================================
   // Input acceleration and turning
   {
      // Apply acceleration / turning based on controller 2 input
      float forwardAccel = 0.0f;
      float rightAccel = 0.0f;
      float turning = 0.0f;
      bool atGoal = false;

      // Direct driving hack
      static bool driveWarthog = false;
      if (driveWarthog)
      {
         forwardAccel = -gInputSystem.getGamepad(1).getStickLY() * cDirectInputForwardAccelRate;
         rightAccel = gInputSystem.getGamepad(1).getStickLX() * cDirectInputRightAccelRate;
         turning = gInputSystem.getGamepad(1).getStickRX() * cDirectInputTurnRate;
      }
      // Calculate acceleration / turning to get to desired position
      else
      {
         calcMovement(stepInfo, forwardAccel, rightAccel, turning, atGoal, pSMA);
         /*
         if (pSMA)
            calcMovement(stepInfo, acceleration, turning, atGoal, pSMA);
         else if (pGMA)
            calcMovement(stepInfo, acceleration, turning, atGoal, pGMA);
         */
      }

      pRB->setLinearDamping(0.0f);
      pRB->setAngularDamping(0.0f);

      // Calculate forward acceleration impulse
      hkVector4 forwardImpulse;
      forwardImpulse = chassisRotation.getColumn(2);
      forwardImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * cAccelerationFactor * forwardAccel);

      // Calculate right acceleration impulse
      hkVector4 rightImpulse;
      rightImpulse = chassisRotation.getColumn(0);
      rightImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * cAccelerationFactor * rightAccel);

      // MPB [7/10/2008] Put this in - but after E3!!!!
      /*
      static bool compensateForGrav = true;
      if (compensateForGrav)
      {
         // Calculate total gravitational force for this frame
         hkVector4 totalGravForce;
         totalGravForce.setMul4(stepInfo.m_deltaTime * (1.0f + pRB->getMass() * cExtraGravityFactor), gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());

         // Scale up the forward impulse to compensate for gravity messing with desired forward
         hkVector4 forwardDir;
         forwardDir = chassisRotation.getColumn(2);
         float antiGravScalar = -totalGravForce.dot3(forwardDir);
         forwardImpulse.add4(antiGravScalar * forwardDir);

         // Scale up the right impulse to compensate for gravity messing with desired forward
         hkVector4 rightDir;
         rightDir = chassisRotation.getColumn(0);
         antiGravScalar = -totalGravForce.dot3(rightDir);
         rightImpulse.add4(antiGravScalar * rightDir);
      }
      */

      // Apply linear impulse
      pRB->applyLinearImpulse(forwardImpulse + rightImpulse);

      // Calculate turning impulse
      hkVector4 turnImpulse;
      turnImpulse.set(0.0f, stepInfo.m_deltaTime * pRB->getMass() * turning, 0.0f, 0.0f);
      pRB->applyAngularImpulse(turnImpulse);

   }

   // Reorient towards up axis to prevent or limit flipping over
   if (mReorient)
      reorient(stepInfo);
}


//============================================================================
//============================================================================
void BPhysicsGhostAction::calcMovement(const hkStepInfo& stepInfo, float& fwdAccel, float& rightAccel, float& turning, bool& atGoal, BSquadActionMove* pSMA)
{
   atGoal = false;
   

   //============================================================================
   // Update new desired position (allow acceleration if desired position changed)
   float maxVelocity = 0.0f;
   float squadVelocity = 0.0f;
   BVector currentPos = mEntity->getPosition();
   float desiredPosIsGoal = true;

   // MPB [7/10/2008]
   // If being hijacked (fatality), don't move.  This is somewhat hacky and needs to be better
   // post E3
//-- FIXING PREFIX BUG ID 4746
   const BUnit* pUnit = mEntity->getUnit();
//--
   if (pUnit && pUnit->getFlagDoingFatality())
   {
      // Keep us where we are, don't update - prevents sliding down hill
      //mDesiredPos = pUnit->getPosition();
      desiredPosIsGoal = true;
      maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();
   }
   else
   {
      // Get desired position and velocity from squad move action
      if (pSMA)
      {
         const BSimTarget *pTarget = pSMA->getTarget();      
         // Units need to move towards their squad's location, not their formation location.
         // mDesiredPos = pSMA->getInterimTarget_4();
         //-- FIXING PREFIX BUG ID 3221
         const BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
         //--
         if (pSquad)
         {
            // Allow us to change teh flag for settling down teh movement.
            //pPhysicsAction->setFlagAllowEarlyTerminate(true);

            // Note -- this is okay because we assume vehicles are always in a squad of one.  If
            // vehicles have to move to squad formation positions, then this code would have to be
            // adjusted to accomodate that. 
            mDesiredPos = pSquad->getPosition();
            BVector direction = pTarget->getPosition() - mDesiredPos;
            // DLM 6/19/08 - Check the squad's state.  If at any time we're in wait state, then set mDesirePosIsGoal to true.
            BActionState state = pSMA->getState();
            if (direction.length() < 0.05f || state == BAction::cStateWait)
               desiredPosIsGoal = true;
            else
               desiredPosIsGoal = false;
            // Trying something where each squad travels at its own velocity
            // Always used platoon velocity -- it'll do the right thing.
#ifdef _NEWWARTHOG_MOVEMENT
            maxVelocity = pSquad->getDesiredVelocity();
            squadVelocity = pSMA->getVelocity();
#else
            maxVelocity = pSMA->getVelocity();
#endif
         }
      }
      // If there is no squad move action, use proto data and the squad position
      else
      {
         // Get max velocity
         BASSERT(mEntity->getUnit());
         BASSERT(mEntity->getUnit()->getProtoObject());
         maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();

//-- FIXING PREFIX BUG ID 4744
         const BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
//--
         if(pSquad)
            mDesiredPos = pSquad->getPosition();
      }
   }

   float decelRate = maxVelocity * cDecelRate;


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

   // Current xz linear velocity
//-- FIXING PREFIX BUG ID 4747
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
#if defined(CODE_ANALYSIS_ENABLED)
   BVector linVel(rand(), 0, rand());
#else
   BVector linVel = pRB->getLinearVelocity();
   linVel.y = 0;
#endif

   // Check stopping distance given current velocity
   // If we're within stopping distance, slow us down.
   // Otherwise accelerate to max velocity
   //float currentVel = linVel.length();
   float currentVelAlongFwd = linVel.dot(currentFwd);
   float currentVelAlongRight = linVel.dot(currentRight);
   float dist = mDesiredPos.xzDistance(currentPos);
   float maxStoppingDist = 0.5f * maxVelocity * maxVelocity / -decelRate;

   // Decel
   if (desiredPosIsGoal && (dist < maxStoppingDist))
   {
      atGoal = true;      

      float desiredVelocity = maxVelocity * dist / maxStoppingDist;

      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * desiredVelocity;
      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      static float cStopFwdK = 5.0f;
      static float cStopFwdDamp = 0.0f;
      float fwdDampConstant = cStopFwdDamp * 2.0f * sqrt(cStopFwdK);
      fwdAccel = cStopFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;

      float desiredVelAlongRight = desiredFwd.dot(currentRight) * desiredVelocity;
      float rightVelDiff = desiredVelAlongRight - currentVelAlongRight;
      static float cStopRightK = 5.0f;
      static float cStopRightDamp = 0.0f;
      float rightDampConstant = cStopRightDamp * 2.0f * sqrt(cStopRightK);
      rightAccel = cStopRightK * rightVelDiff - rightDampConstant * currentVelAlongRight;
   }
   else
   {
#ifdef _NEWWARTHOG_MOVEMENT
      // Scale the velocity based on our distance from our desired position.  If we're further than twice our obstruction radius,
      // go full speed.  As we get closer, adjust down to squad velocity.. 
      float fObsSize = max(0.01f, mEntity->getUnit()->getObstructionRadius());
      if (dist > fObsSize * 2.0f)
         dist = fObsSize * 2.0f;
      float desiredVel = squadVelocity;
      if (dist > fObsSize)
      {
         float fDistFactor = (dist - fObsSize) / fObsSize;
         desiredVel = Math::Lerp(squadVelocity, maxVelocity, fDistFactor);
      }
      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * desiredVel;
      float velDiff = desiredVelAlongFwd - currentVelAlongFwd; // use currentVelAlongFwd because currentVel doesn't indicate direction (this can cause increased reverse velocity)
      float dampConst = 0.0f;
      if (velDiff < 0.0f)
         dampConst = cStopFwdK;
      else
         dampConst = cFwdK;

      float fwdDampConstant = cFwdDamp * 2.0f * sqrt(dampConst);
      fwdAccel = dampConst * velDiff - fwdDampConstant * currentVelAlongFwd;
#else
      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * maxVelocity;
      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      float fwdDampConstant = cFwdDamp * 2.0f * sqrt(cFwdK);
      fwdAccel = cFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;
#endif
      float desiredVelAlongRight = desiredFwd.dot(currentRight) * maxVelocity;
      float rightVelDiff = desiredVelAlongRight - currentVelAlongRight;
      float rightDampConstant = cRightDamp * 2.0f * sqrt(cRightK);
      rightAccel = cRightK * rightVelDiff - rightDampConstant * currentVelAlongRight;
   }

   //============================================================================
   // Turning

   // Check to see if we have an attack target to turn towards
   bool attackAction = false;
//-- FIXING PREFIX BUG ID 4748
   const BUnitActionRangedAttack* pAction = reinterpret_cast<BUnitActionRangedAttack*>(mEntity->getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
   if (pAction && pAction->getTarget())
   {
      attackAction = true;

      BVector targetPos;
      const BSimTarget* pTarget = pAction->getTarget();
      if(pTarget)
      {         
//-- FIXING PREFIX BUG ID 4745
         const BEntity* pEntity=gWorld->getEntity(pTarget->getID());
//--
         if (pEntity)
            targetPos = pEntity->getPosition();
         else
            attackAction = false;
      }
      
      // MPB [7/10/2008]
      // If being hijacked (fatality), don't turn to face target.  This is somewhat hacky and needs to be better
      // post E3
      if (pUnit && pUnit->getFlagDoingFatality())
      {
         desiredFwd = pUnit->getForward();
         desiredFwd.y = 0.0f;
         desiredFwd.safeNormalize();
      }
      else if (attackAction)
      {
         desiredFwd = targetPos - currentPos;
         desiredFwd.y = 0.0f;
         desiredFwd.safeNormalize();
      }
   }

   // No turning if we're at the goal or not accelerating
   if (!attackAction && atGoal)//(atGoal || (dist < cNoTurnDist) || (currentVel < cNoTurnVel)))
   {
      // Damp out current angular velocity
      hkVector4 currentAngVel = pRB->getAngularVelocity();
      const float stopTurnFactor = -0.5f;
      turning = stopTurnFactor * currentAngVel(1);
   }
   // Turn towards desired
   else
   {
      float angleDiff = desiredFwd.angleBetweenVector(currentFwd);
      if (desiredFwd.cross(currentFwd).y > 0.0f)
         angleDiff = -angleDiff;

      // Calculate turning to move towards desired using an overdamped spring
      hkVector4 currentAngVel = pRB->getAngularVelocity();

      if (attackAction) // attack orientation if stiff spring
      {
         float attackTurnDampConstant = cAttackTurnDamp * 2.0f * sqrt(cAttackTurnK);
         turning = cAttackTurnK * angleDiff - attackTurnDampConstant * currentAngVel(1);
      }
      else
      {
         float turnDampConstant = cTurnDamp * 2.0f * sqrt(cTurnK);
         turning = cTurnK * angleDiff - turnDampConstant * currentAngVel(1);
      }
   }
}


//============================================================================
// This code is from hkpReorientAction::applyAction in Havok's sdk
//============================================================================
void BPhysicsGhostAction::reorient(const hkStepInfo& stepInfo)
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

//============================================================================
// Debug funcs
//============================================================================
#ifndef BUILD_FINAL
   void BPhysicsGhostAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
//-- FIXING PREFIX BUG ID 4749
         const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--

         // Get orientation and suspension direction (down)
         hkTransform trans;
         trans.set(pRB->getRotation(), pRB->getPosition());

         hkRotation rot;
         rot.set(pRB->getRotation());
         hkVector4 rigidBodyUp = -rot.getColumn(1);

         hkVector4 startPt;
         hkVector4 endPt;
         hkVector4 collisionPt;

         for(int i = 0; i < 4; i++)
         {
            // Compute start and end points of the suspension rays
            startPt.setTransformedPos(trans, cHardpoints[i]);
            collisionPt = startPt + mSuspensionDist[i] * rigidBodyUp * (cSpringLength + cWheelRadius);
            endPt = startPt + rigidBodyUp * (cSpringLength + cWheelRadius);

            gpDebugPrimitives->addDebugLine((BVector)startPt, (BVector)collisionPt, 0xff00ff00, 0xff00ff00);
            gpDebugPrimitives->addDebugLine((BVector)collisionPt, (BVector)endPt, 0xffff0000, 0xffff0000);
         }
      }
   }
#endif