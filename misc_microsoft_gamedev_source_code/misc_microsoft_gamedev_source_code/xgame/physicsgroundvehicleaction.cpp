//============================================================================
// File: physicsgroundvehicleaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicsgroundvehicleaction.h"
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
#include "protoobject.h"
#include "terrainphysicsheightfield.h"
#include "squadactionmove.h"
#include "terraintraildecal.h"
#include "terraineffectmanager.h"
#include "terraineffect.h"
#include "physicsinfo.h"
#include "Common\Base\Container\LocalArray\hkLocalArray.h"

#ifndef _MOVE4
#define _MOVE4
#endif


//============================================================================
// Constants

const float cTeleportImpulseFactor = 0.16666666f;
const float cExtraGravityFactor = 2.0f;

const float cPathTurnK = 150.0f;
const float cPathTurnDamp = 1.0f;

// Reorientation constants
const hkVector4 cReorientRotationAxis(0.0f, 0.0f, 1.0f, 0.0f);
const hkVector4 cReorientUpAxis(0.0f, 1.0f, 0.0f, 0.0f);
const float cReorientStrength = 0.75f;
const float cReorientDamping = 0.1f;
const float cInertiaMultiplier = 1.0f / (150.0f * 0.4f); // This is ratio based on the warthog mass and roll inertia factor

//============================================================================
//============================================================================
BPhysicsGroundVehicleAction::BPhysicsGroundVehicleAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo) :
   hkpUnaryAction( body ),
   mPrevLinearVelocity(cOriginVector),
   mDistBelowTerrain(0.0f),
   mTerrainCollision(false)
{
   mEntity = pEntity;

   BASSERT(pInfo && pInfo->getVehicleInfo());
   setVehicleInfo(pInfo->getVehicleInfo());
   setUserData(BPhysicsInfo::cGround); // this is a hack for reloading

   mTrailDecal= new BTerrainTrailDecalGenerator();
}

//============================================================================
//============================================================================
BPhysicsGroundVehicleAction::~BPhysicsGroundVehicleAction()
{
   if(mTrailDecal)
   {
      mTrailDecal->endTrail();
      delete mTrailDecal;
      mTrailDecal = NULL;
   }
}

//============================================================================
//============================================================================
void BPhysicsGroundVehicleAction::setVehicleInfo(BPhysicsVehicleInfo* newData)
{
   BASSERT(newData);
   mpVehicleData = newData;

   // Resize arrays based on hardpoints
   int numHardpoints = mpVehicleData->mHardpoints.getNumber();
   mCurrentSuspensionLengths.resize(numHardpoints);
   mWheelContactPts.resize(numHardpoints);
   #ifndef BUILD_FINAL
      mSuspensionDist.resize(numHardpoints);
   #endif

   for (int i = 0; i < numHardpoints; i++)
   {
      mCurrentSuspensionLengths[i] = mpVehicleData->mSpringLength;
      mWheelContactPts[i].set(0,0,0);
   }

   // Reset inertia
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);

   float mass = pRB->getMass();
   hkMatrix3 matrix;
   matrix.setIdentity();
   matrix.setDiagonal(mpVehicleData->mChassisUnitInertiaPitch * mass, mpVehicleData->mChassisUnitInertiaYaw * mass, mpVehicleData->mChassisUnitInertiaRoll * mass);

   pRB->setInertiaLocal(matrix);
}

//============================================================================
//============================================================================
void BPhysicsGroundVehicleAction::applyAction( const hkStepInfo& stepInfo )
{
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);

   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   // Suspension constants
   const float cSpringLength = mpVehicleData->mSpringLength;
   const float cWheelRadius = mpVehicleData->mWheelRadius;
   const float cSpringStrength = mpVehicleData->mSpringStrength;
   const float cDampingCompression = mpVehicleData->mDampingCompression;
   const float cDampingRelaxation = mpVehicleData->mDampingRelaxation;
   const float cNormalClippingAngle = mpVehicleData->mNormalClippingAngle;

   // Get hkpSampledHeightFieldShape for raycasting against
   BASSERT(gTerrainSimRep.getPhysicsHeightfield());
   const hkpEntity* pHeightfieldEntity = gTerrainSimRep.getPhysicsHeightfield()->getHKEntity();
   BASSERT(pHeightfieldEntity->getCollidable());
//-- FIXING PREFIX BUG ID 3621
   const hkpSampledHeightFieldShape* pHeightfield = (hkpSampledHeightFieldShape*) pHeightfieldEntity->getCollidable()->getShape();
//--
	const hkTransform& heightfieldTrans = pHeightfieldEntity->getCollidable()->getTransform();

   // Get current chassis rotation
   hkRotation chassisRotation;
   chassisRotation.set(pRB->getRotation());

   //============================================================================
   // Update wheel spin for visual (based on linear velocity along forward vector)
   hkVector4 linVel = pRB->getLinearVelocity();
   mPrevLinearVelocity.x = linVel(0);
   mPrevLinearVelocity.y = linVel(1);
   mPrevLinearVelocity.z = linVel(2);

   //============================================================================
   // Suspension - store off several pieces of data for use below
   bool hasRayHit = false;

   uint numHardpoints = static_cast<uint>(mpVehicleData->mHardpoints.getNumber());
//-- FIXING PREFIX BUG ID 3622
   const hkVector4* cHardpoints = reinterpret_cast<hkVector4*>(mpVehicleData->mHardpoints.getData());
//--

   // Set up temp data
   hkLocalArray<bool> rayHit(numHardpoints);
   hkLocalArray<float> suspensionForces(numHardpoints);
   hkLocalArray<hkVector4> hardPts(numHardpoints);
   hkLocalArray<hkVector4> contactPts(numHardpoints);
   hkLocalArray<hkVector4> contactNormals(numHardpoints);
   hkLocalArray<hkVector4> rayEndPts(numHardpoints);
   hkLocalArray<float> clippedInvContactDotSuspension(numHardpoints);
   hkLocalArray<float> suspensionRelativeVelocity(numHardpoints);
   rayHit.setSize(numHardpoints);
   suspensionForces.setSize(numHardpoints);
   hardPts.setSize(numHardpoints);
   contactPts.setSize(numHardpoints);
   contactNormals.setSize(numHardpoints);
   rayEndPts.setSize(numHardpoints);
   clippedInvContactDotSuspension.setSize(numHardpoints);
   suspensionRelativeVelocity.setSize(numHardpoints);

   {
      // Get orientation and suspension direction (down)
      hkTransform trans;
      trans.set(pRB->getRotation(), pRB->getPosition());

      // Using suspension relative to the chassis, not just world up.  World up stuff here for legacy
      const bool suspUp = false;

      hkRotation rot;
      rot.set(pRB->getRotation());
      #if defined(CODE_ANALYSIS_ENABLED)
         hkVector4 suspDir = rot.getColumn(1);
         suspDir.setNeg4(suspDir);
      #else
         hkVector4 suspDir;
         if (suspUp)
            suspDir = (hkVector4) -cYAxisVector;
         else
            suspDir = -rot.getColumn(1);
      #endif

      // Collide all wheels
      for (uint i = 0; i < numHardpoints; i++)
      {
         // Init rayHit, suspension force
         rayHit[i] = false;
         suspensionForces[i] = 0.0f;

         // Init hardpoint (where suspension is connected to chassis)
         hardPts[i].setTransformedPos(trans, cHardpoints[i]);

         // Init endpoint
         rayEndPts[i] = hardPts[i] + suspDir * (cSpringLength + cWheelRadius);

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
            hasRayHit = true;
            rayHit[i] = true;
            if (suspUp)
               contactNormals[i] = cYAxisVector;
            else
               contactNormals[i] = output.m_normal;

            // Calc length of suspension from hard point to collision point
            hkReal hitDistance = output.m_hitFraction * (cSpringLength + cWheelRadius);
            mCurrentSuspensionLengths[i] = hitDistance - cWheelRadius;

            // Calc contact position
            hkVector4 contactPointWsPosition;
            contactPointWsPosition.setAddMul4(hardPts[i], suspDir, hitDistance);
            contactPts[i] = mWheelContactPts[i] = contactPointWsPosition;

            // Calc the suspension velocity relative to the ground and projected along
            // the collision normal
            hkVector4 chassis_velocity_at_contactPoint;
            pRB->getPointVelocity(contactPts[i], chassis_velocity_at_contactPoint);

            hkReal projVel = contactNormals[i].dot3( chassis_velocity_at_contactPoint );

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
      for (uint i = 0; i < numHardpoints; i++)
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

   BSquad* pParentSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();

   //============================================================================
   // Input acceleration and turning
   {
      pRB->setLinearDamping(0.0f);
      pRB->setAngularDamping(0.0f);

      // If contacting the ground, apply driving and friction forces

      if (1)//(hasRayHit)
      {
         //============================================================================
         // Linear impulse

         // Calculate forcable teleport impulse
         hkVector4 teleportImpulse;
         {
            hkVector4 currentVel = pRB->getLinearVelocity();
            hkVector4 currentPos = pRB->getPosition();
            hkVector4 desiredPos = currentPos;
            if (pParentSquad)
            {
               #ifdef NEW_TURNRADIUS
                  desiredPos = pParentSquad->getTurnRadiusPos();
               #else
                 desiredPos = pParentSquad->getPosition();
               #endif
            }
 
            hkVector4 oppImpulse = -currentVel;
            oppImpulse(1) = 0.0f;
            oppImpulse.mul4(pRB->getMass());
            
            hkVector4 impulse = desiredPos - currentPos;
            impulse(1) = 0.0f;
            impulse.mul4(pRB->getMass() * stepInfo.m_invDeltaTime * cTeleportImpulseFactor);
 
            teleportImpulse = oppImpulse + impulse;

            // MPB [10/2/2008] - Disable this as it could result in some high upward impulses
            // only in plane of orientation
            /*
            hkRotation rot;
            rot.set(pRB->getRotation());
            hkVector4 chassisUp = rot.getColumn(1);
            hkReal impulseScalarY = teleportImpulse.dot3( chassisUp );
            chassisUp.mul4(impulseScalarY);
            teleportImpulse = teleportImpulse - chassisUp;
            */
         }
 
         // Apply teleport impulse
         pRB->applyLinearImpulse(teleportImpulse);

         //============================================================================
         // Angular impulse

         // This is for calculating/applying an angular impulse that forces the rigid
         // body to match the sim orientation (similar to the teleport linear impulse above for forcing sim positioning in physics).
         // It does this in yaw only so that suspension / gravity forces can still pitch/roll the body.
         if (pParentSquad)
         {
            // Get desired orientation
            BMatrix rot;
            #ifdef NEW_TURNRADIUS
               BVector desiredFwd = pParentSquad->getTurnRadiusFwd();
               rot.makeOrient(desiredFwd, cYAxisVector, cYAxisVector.cross(desiredFwd));
            #else
               pParentSquad->getRotation(rot);
            #endif

            // Convert orientation to hkQuaternion
            BPhysicsMatrix physMtx;
            rot.getD3DXMatrix(physMtx);
            hkRotation hkRot;
            BPhysics::convertRotation(physMtx, hkRot);
            hkQuaternion desiredOrientation(hkRot);

            // current orientation
            hkQuaternion currentOrientation = pRB->getRotation();

            // Calculate angular velocity to get to desired orientation
            hkQuaternion quatDif;
            quatDif.setMulInverse(desiredOrientation, currentOrientation);
            quatDif.normalize();
            hkReal angle = quatDif.getAngle();
            hkVector4 angularVelocity;
            if(angle < 1e-3f)
            {
               angularVelocity.setZero4();
            }
            else
            {
               quatDif.getAxis(angularVelocity);
               angularVelocity.setMul4(angle * stepInfo.m_invDeltaTime, angularVelocity);
            }

            // Calculate impulse from current/desired angular velocity.  We have to incorporate
            // the inertia here as the inverse inertia is multiplied into the impulse in applyAngularImpulse.
            // This is similar to how mass is multipled in for linear impulses
            hkMatrix3 worldInertia;
            pRB->getInertiaWorld( worldInertia );

            hkVector4 impulse;
            hkVector4 velocityDifference; velocityDifference.setSub4( angularVelocity, pRB->getAngularVelocity() );        
            impulse.setMul3( worldInertia, velocityDifference);

            // Only use the portion of the impulse about the chassisUp (yaw only)
            hkRotation currentRot;
            currentRot.set(pRB->getRotation());
            hkVector4 chassisUp = currentRot.getColumn(1);
            float impulseScalarYaw = chassisUp.dot3(impulse);
            impulse.setMul4(impulseScalarYaw, chassisUp);

            // Apply impulse
            pRB->applyAngularImpulse(impulse);
         }

         // MPB - This is the "obsolete" spring based turning method.  Instead we use the forcible
         // version above.
         /*
         // Calculate turning to get to desired forward
         if (hasRayHit)
         {
            float turning = 0.0f;
            calcTurning(turning, pParentSquad);

            // Calculate turning impulse
            hkVector4 turnImpulse;
            turnImpulse.set(0.0f, stepInfo.m_deltaTime * pRB->getMass() * turning, 0.0f, 0.0f);

            // Apply impulse
            pRB->applyAngularImpulse(turnImpulse);
         }
         */
      }
   }

   //============================================================================
   // Reorient towards up axis to prevent or limit flipping over

   // Calculate axis to rotate about (cross product of world up and chassis up)
   // Use roll axis if cross product invalid
   hkRotation currentRot;
   currentRot.set(pRB->getRotation());
   hkVector4 chassisUp = currentRot.getColumn(1);
   hkVector4 rotationAxis;
   rotationAxis.setCross(chassisUp, cYAxisVector);
   if (rotationAxis.lengthSquared3() > cFloatCompareEpsilon)
      rotationAxis.normalize3();
   else
      rotationAxis = cReorientRotationAxis;

   // Put axis in body space
   hkVector4 bodyRotationAxis;
   bodyRotationAxis.setRotatedInverseDir(pRB->getRotation(), rotationAxis);

   // Calculate the reorient strength and damping
   // These are scaled versions of the warthog reorient parameters.  The scaling is based on a ratio
   // of inertia tensors, which are in turn based on the mass and inertia constants
   float inertiaFactor = pRB->getMass() * mpVehicleData->mChassisUnitInertiaRoll * cInertiaMultiplier;
   float strength = cReorientStrength * inertiaFactor;
   float damping = cReorientDamping * inertiaFactor;

   // Reorient
   reorient(stepInfo, bodyRotationAxis, strength, damping);

   //============================================================================
   // Manually resolve body collisions with the terrain as these are turned off
   // as far as Havok is concerned (the collision filter disables collision between
   // this vehicle and the terrain).

   // TODO - We may want to ensure that either the entire rigid body is above the terrain.
   // Currently it just ensures that the suspension hardpoints are above

   // Get the maximum future distance below the terrain for all the hardpoints
   bool belowTerrain = false;
   float distBelowTerrain = 0.0f;
   hkTransform trans;
   trans.set(pRB->getRotation(), pRB->getPosition());
   for (uint i = 0; i < numHardpoints; i++)
   {
      hkVector4 currentHardpointPos;
      currentHardpointPos.setTransformedPos(trans, cHardpoints[i]);
      hkVector4 futureHardpointPos = currentHardpointPos + pRB->getLinearVelocity() * stepInfo.m_deltaTime;

      // Transform future position to physics heightfield space and convert to tile coordinates
      hkVector4 futurePosRelHeightfield;
      futurePosRelHeightfield.setTransformedInversePos(heightfieldTrans, futureHardpointPos);
      hkVector4 futureTilePos = futurePosRelHeightfield * pHeightfield->m_floatToIntScale;
      int xPos = floor(futureTilePos(0));
      int zPos = floor(futureTilePos(2));
      float subX = futureTilePos(0) - xPos;
      float subZ = futureTilePos(2) - zPos;

      // Get the heightfield height at future rigid body xz
      hkVector4 normalOut;
      float heightOut;
      int triIndexOut;
      pHeightfield->getHeightAndNormalAt(xPos, zPos, subX, subZ, normalOut, heightOut, triIndexOut);

      if (futureHardpointPos(1) < heightOut)
      {
         distBelowTerrain = Math::Max(distBelowTerrain, heightOut - futureHardpointPos(1));
         belowTerrain = true;
      }
   }

   // If any of the hardpoints are under the terrain, set up this action to resolve the collision
   // single threaded.  The resolution involves setting the position of the rigid body directly and
   // it is dangerous to do this multithreaded.  So, here we just update the dist below terrain variable and
   // add this action to the global list of actions to resolve collisions for.
   if (belowTerrain)
   {
      mDistBelowTerrain = distBelowTerrain;

      // Add this action to thread-safe list of actions to resolve collisions for.  And only add it once
      if (!mTerrainCollision)
      {
         gWorld->addPhysicsVehicleTerrainCollisionAsync(this);
         mTerrainCollision = true;
      }
   }
}

//============================================================================
// If any of the hardpoints are under the terrain, move the rigid body enough so they
// are all above, zero out any downward velocity, and apply an upward impulse proportional
// to prevVel * restitution to simulate some bounciness.
// This is set up to execute in applyAction above, with the proper distBelowTerrain
// variable having bee
//============================================================================
void BPhysicsGroundVehicleAction::mainThreadTerrainCollisionResolution()
{
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);
   if (!pRB)
      return;

   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   BASSERT(mTerrainCollision);

   // Zero out current downward velocity
   hkVector4 currentVel = pRB->getLinearVelocity();
   float downWardVel = currentVel(1);
   currentVel(1) = 0.0f;
   pRB->setLinearVelocity(currentVel);

   // Move rigid body up so that the lowest hardpoint is above the terrain (by wheel radius)
   const float cWheelRadius = mpVehicleData->mWheelRadius;
   hkVector4 currentPos = pRB->getPosition();
   currentPos(1) += mDistBelowTerrain + cWheelRadius;
   pRB->setPosition(currentPos);

   // Apply an impulse for bounciness
   float oppVelScalar = -downWardVel * pRB->getMaterial().getRestitution() * pRB->getMass();
   hkVector4 impulse;
   impulse.setMul4(oppVelScalar, cYAxisVector);
   pRB->applyLinearImpulse(impulse);

   // Handled
   mTerrainCollision = false;
}



//============================================================================
//============================================================================
void BPhysicsGroundVehicleAction::calcTurning(float& turning, BSquad* pParentSquad)
{
//-- FIXING PREFIX BUG ID 3623
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--

   //============================================================================
   // Get current and desired xz directions
   BVector currentFwd = mEntity->getForward();
   currentFwd.y = 0.0f;
   currentFwd.normalize();

   // Get desired fwd
   BVector desiredFwd;

   if (pParentSquad)
      desiredFwd = pParentSquad->getTurnRadiusFwd();
   else
      desiredFwd = currentFwd;
   desiredFwd.y = 0.0f;
   if (!desiredFwd.safeNormalize())
      desiredFwd = currentFwd;

   //============================================================================
   // Turning
   float angleDiff = desiredFwd.angleBetweenVector(currentFwd);
   if (desiredFwd.cross(currentFwd).y > 0.0f)
      angleDiff = -angleDiff;

   // If at the goal, orient front or back or warthog towards desired (whichever is closer)
   /*
   if (atGoal)
   {
      if (angleDiff > cPiOver2)
         angleDiff = angleDiff - cPi;
      else if (angleDiff < -cPiOver2)
         angleDiff = angleDiff + cPi;
   }
   */

   // Calculate turning to move towards desired using an underdamped spring
   float turnK = cPathTurnK;
   float turnDampConstant = cPathTurnDamp * 2.0f * sqrt(turnK);
   hkVector4 currentAngVel = pRB->getAngularVelocity();
   turning = turnK * angleDiff - turnDampConstant * currentAngVel(1);
}

//============================================================================
// This code is from hkpReorientAction::applyAction in Havok's sdk
//============================================================================
void BPhysicsGroundVehicleAction::reorient(const hkStepInfo& stepInfo, const hkVector4 rotationAxis, float strength, float damping)
{
   // Move the world axis into body space
   hkpRigidBody* rb = static_cast<hkpRigidBody*>( m_entity );

   hkVector4 bodyUpAxis;		bodyUpAxis.setRotatedDir( rb->getRotation(), cReorientUpAxis );
   hkVector4 bodyRotationAxis;	bodyRotationAxis.setRotatedDir( rb->getRotation(), rotationAxis );

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
   av.setMul4( damping * stepInfo.m_invDeltaTime, rb->getAngularVelocity() );

   hkVector4 impulse; 
   impulse.setMul4( strength * angle * stepInfo.m_invDeltaTime, bodyRotationAxis );
   impulse.sub4( av );

   rb->applyAngularImpulse( impulse );
}

//============================================================================
// BPhysicsGroundVehicleAction::spawnPhysicsEventVisuals
//============================================================================
void BPhysicsGroundVehicleAction::spawnPhysicsEventVisuals(long terrainEffectsHandle)
{
   float linVeloLen = mPrevLinearVelocity.length();
   if(linVeloLen < 0.01f)
      return;

   BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(terrainEffectsHandle, true);
   if(pTerrainEffect == NULL)
      return;

   if (mWheelContactPts.getNumber() < 2)
      return;

   const BVector cEmpty(0,0,0);
   //we need a better 'update hasn't been run yet' condition here...
   if(mWheelContactPts[0].almostEqual(cEmpty) || mWheelContactPts[1].almostEqual(cEmpty))
      return;

   BVector half = (mWheelContactPts[0] + mWheelContactPts[1]) * 0.5f;
   BYTE surfaceType = gTerrainSimRep.getTileType(half);


   //front tires
   if(mTrailDecal != NULL)
   {
//-- FIXING PREFIX BUG ID 3624
      const BRibbonCreateParams* parms = pTerrainEffect->getTrailDecalHandleForType(surfaceType, -1);
//--
      if(parms != NULL)
      {   
         if(!mTrailDecal->isSameParams(*parms))
            mTrailDecal->startTrail(*parms);

         mTrailDecal->addPointToTrail(half.x,half.z);
      }
      else
      {
         mTrailDecal->endTrail();
      }
   }
}

//============================================================================
// Debug funcs
//============================================================================
#ifndef BUILD_FINAL
   void BPhysicsGroundVehicleAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      // Re-enable this when needed - will need to cache the hard points, contact points, and end points
      // in the apply action - I don't want to take up the memory right now
      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
//-- FIXING PREFIX BUG ID 3626
         const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--

         // Get orientation and suspension direction (down)
         hkTransform trans;
         trans.set(pRB->getRotation(), pRB->getPosition());

         hkRotation rot;
         rot.set(pRB->getRotation());
         hkVector4 rigidBodyUp = -rot.getColumn(1);
         
         hkVector4 worldUp = (hkVector4) -cYAxisVector;

         BMatrix boxMat;
         boxMat.makeIdentity();

         hkVector4 startPt;
         hkVector4 endPt;
         hkVector4 collisionPt;

         float springPlusWheel = mpVehicleData->mSpringLength + mpVehicleData->mWheelRadius;

         uint numHardpoints = static_cast<uint>(mpVehicleData->mHardpoints.getNumber());
//-- FIXING PREFIX BUG ID 3627
         const hkVector4* cHardpoints = reinterpret_cast<hkVector4*>(mpVehicleData->mHardpoints.getData());
//--

         for(uint i = 0; i < numHardpoints; i++)
         {
            // Compute start and end points of the suspension rays
            startPt.setTransformedPos(trans, cHardpoints[i]);
            collisionPt = startPt + mSuspensionDist[i] * rigidBodyUp * springPlusWheel;
            endPt = startPt + rigidBodyUp * springPlusWheel;

            gpDebugPrimitives->addDebugLine((BVector)startPt, (BVector)collisionPt, 0xff00ff00, 0xff00ff00);
            gpDebugPrimitives->addDebugLine((BVector)collisionPt, (BVector)endPt, 0xffff0000, 0xffff0000);
         }
      }
   }
#endif
