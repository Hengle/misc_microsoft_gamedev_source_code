//============================================================================
// File: physicswarthogaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicswarthogaction.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "debugprimitives.h"
#include "configsgame.h"
#include "world.h"
#include "physics.h"
#include "physicsworld.h"
#include "usermanager.h"
#include "user.h"
#include "unitactionmovewarthog.h"
#include "protoobject.h"
#include "terrainphysicsheightfield.h"
#include "squadactionmove.h"
#include "terraintraildecal.h"
#include "terraineffectmanager.h"
#include "terraineffect.h"
#include "object.h"
#include "visual.h"
#include "unitactionrangedattack.h"
#include "unitactionphysics.h"

#ifndef _MOVE4
#define _MOVE4
#endif

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
const float cSpringStrength = 50.0f;
const float cDampingCompression = 2.5f;
const float cDampingRelaxation = 2.5f;
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

// Direct input driving constants
const float cLinearVelDampingBase = 0.1f;
const float cLinearVelDampingPerTire = 1.0f;
const float cAngularVelDamping = 2.0f;
const float cDirectInputAccelRate = 100.0f;
const float cDirectInputTurnRate = 30.0f;

// Acceleration and turning constants
const float cDecelRate = -1.0f;
const float cFwdK = 2.5f;
const float cFwdDamp = 0.0f;
const float cStopFwdK = 5.0f;
const float cStopFwdDamp = 0.0f;
const float cMaxTurnK = 50.0f;
const float cMinTurnK = 30.0f;
const float cTurnDamp = 0.3f;
const float cPathTurnK = 150.0f;
const float cPathTurnDamp = 1.0f;

// Friction constants
const float cMaxSideFriction = -40.0f;
const float cMinSideFriction = -4.0f;
const float cPathSideFriction = -40.0f;

// Wheel visual constants
const float cSpinFactor = 2.0f;
const float cMaxWheelTurn = cPiOver2 * 0.5f;
const float cSpringStableLength = 1.0f;
const float cWheelHeightMin = -0.4f;
const float cWheelHeightMax = 0.3f;

// Reorientation constants
const hkVector4 cReorientRotationAxis(0.0f, 0.0f, 1.0f, 0.0f);
const hkVector4 cReorientUpAxis(0.0f, 1.0f, 0.0f, 0.0f);
const float cReorientStrength = 0.75f;
const float cReorientDamping = 0.1f;

//============================================================================
//============================================================================
BPhysicsWarthogAction::BPhysicsWarthogAction(hkpRigidBody* body, BEntity* pEntity, bool chopper) :
   hkpUnaryAction( body ),
   mLastDistance(0.0f),
   mTimeOverturned(0.0f),
   mWheelSpinAngle(0.0f),
   mPrevTurning(0.0f),
   mReorient(true),
   mStayOnPath(false),
   mFlagInAir(false),
   mFlagSkidding(false),
   mFlagChopper(chopper),
   mCanPivotToTarget(false)
{
   mCurrentSuspensionLengths[0] = cSpringLength;
   mCurrentSuspensionLengths[1] = cSpringLength;
   mCurrentSuspensionLengths[2] = cSpringLength;
   mCurrentSuspensionLengths[3] = cSpringLength;

   mWheelContactPts[0].set(0,0,0);
   mWheelContactPts[1].set(0,0,0);
   mWheelContactPts[2].set(0,0,0);
   mWheelContactPts[3].set(0,0,0);

   mEntity = pEntity;
   mDesiredPos = mEntity->getPosition();

   // Init inertia tensor
   float mass = body->getMass();

   hkMatrix3 matrix;
   matrix.setIdentity();
   matrix.setDiagonal(cChassisUnitInertiaPitch * mass, cChassisUnitInertiaYaw * mass, cChassisUnitInertiaRoll * mass);

   body->setInertiaLocal(matrix);

   mTrailDecal[0] = new BTerrainTrailDecalGenerator();
   if (mFlagChopper)
      mTrailDecal[1] = NULL;
   else
      mTrailDecal[1] = new BTerrainTrailDecalGenerator();
}

//============================================================================
//============================================================================
BPhysicsWarthogAction::~BPhysicsWarthogAction()
{
   if(mTrailDecal[0])
   {
      mTrailDecal[0]->endTrail();
      delete mTrailDecal[0];
      mTrailDecal[0] = NULL;
   }

   if(mTrailDecal[1])
   {
      mTrailDecal[1]->endTrail();
      delete mTrailDecal[1];
      mTrailDecal[1] = NULL;
   }
}
//============================================================================
//============================================================================
void BPhysicsWarthogAction::getWheelData(float& spinAngle, float& turnAngle, float& flWheelHeight, float& frWheelHeight, float& blWheelHeight, float& brWheelHeight) const
{
//-- FIXING PREFIX BUG ID 3217
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BASSERT(pRB);
   pRB;

   // Rotation angle (0 to 2Pi)
   spinAngle = mWheelSpinAngle;

   // Percent (-1.0f to 1.0f) turning left to right
   turnAngle = mPrevTurning / cPi;
   turnAngle = Math::Clamp(turnAngle, -1.0f, 1.0f) * cMaxWheelTurn;

   // Percent (-1.0f to 1.0f) of min / max wheel height
   if (mCurrentSuspensionLengths[0] > cSpringStableLength)
      flWheelHeight = Math::Clamp(((mCurrentSuspensionLengths[0] - cSpringStableLength) / (cSpringLength - cSpringStableLength)), 0.0f, 1.0f) * cWheelHeightMin;
   else
      flWheelHeight = Math::Clamp(((cSpringStableLength - mCurrentSuspensionLengths[0]) / cSpringStableLength), 0.0f, 1.0f) * cWheelHeightMax;

   if (mCurrentSuspensionLengths[1] > cSpringStableLength)
      frWheelHeight = Math::Clamp(((mCurrentSuspensionLengths[1] - cSpringStableLength) / (cSpringLength - cSpringStableLength)), 0.0f, 1.0f) * cWheelHeightMin;
   else
      frWheelHeight = Math::Clamp(((cSpringStableLength - mCurrentSuspensionLengths[1]) / cSpringStableLength), 0.0f, 1.0f) * cWheelHeightMax;

   if (mCurrentSuspensionLengths[2] > cSpringStableLength)
      blWheelHeight = Math::Clamp(((mCurrentSuspensionLengths[2] - cSpringStableLength) / (cSpringLength - cSpringStableLength)), 0.0f, 1.0f) * cWheelHeightMin;
   else
      blWheelHeight = Math::Clamp(((cSpringStableLength - mCurrentSuspensionLengths[2]) / cSpringStableLength), 0.0f, 1.0f) * cWheelHeightMax;

   if (mCurrentSuspensionLengths[3] > cSpringStableLength)
      brWheelHeight = Math::Clamp(((mCurrentSuspensionLengths[3] - cSpringStableLength) / (cSpringLength - cSpringStableLength)), 0.0f, 1.0f) * cWheelHeightMin;
   else
      brWheelHeight = Math::Clamp(((cSpringStableLength - mCurrentSuspensionLengths[3]) / cSpringStableLength), 0.0f, 1.0f) * cWheelHeightMax;
}

//============================================================================
//============================================================================
void BPhysicsWarthogAction::updateWarthogVisuals()
{
   if(!mEntity || !mEntity->getObject())
      return;

   BVisual* pVis = mEntity->getObject()->getVisual();
   if (!pVis)
      return;

   float spinAngle, turnAngle, flWheelHeight, frWheelHeight, blWheelHeight, brWheelHeight;
   getWheelData(spinAngle, turnAngle, flWheelHeight, frWheelHeight, blWheelHeight, brWheelHeight);

   // 1 - Back right
   // 2 - Front right
   // 3 - Back left
   // 4 - Front left
   static const char* gWheelNames[4]={"bone_wheel_01", "bone_wheel_02", "bone_wheel_03", "bone_wheel_04" };
   for (uint i = 0; i < 4; i++)
   {
      BVisualItem* pWheelAttachment = pVis->getAttachmentByToBoneName(gWheelNames[i]);
      if (pWheelAttachment)
      {
         BMatrix mtx;// = pWheelAttachment->mTransform;

         // Spin
         if (i == 0 || i == 1)
            mtx.makeRotateX(spinAngle);
         else  // reverse rotation direction of left tires since they're rotated around 180 degrees
            mtx.makeRotateX(-spinAngle);

         // Turning
         if (i == 1 || i == 3)
            mtx.multRotateY(turnAngle);
         else // reverse rotation of back tires
            mtx.multRotateY(-turnAngle * 0.5f);

         // Suspension bouncing
         if (i == 0)
            mtx.setTranslation(0.0f, brWheelHeight, 0.0f);
         else if (i == 1)
            mtx.setTranslation(0.0f, frWheelHeight, 0.0f);
         else if (i == 2)
            mtx.setTranslation(0.0f, blWheelHeight, 0.0f);
         else if (i == 3)
            mtx.setTranslation(0.0f, flWheelHeight, 0.0f);

         // Set transform
         pWheelAttachment->setTransform(mtx);
      }
   }
}

//============================================================================
//============================================================================
void BPhysicsWarthogAction::updateChopperVisuals()
{
   if(!mEntity || !mEntity->getObject())
      return;

   BVisual* pVis = mEntity->getObject()->getVisual();
   if (!pVis)
      return;

   float spinAngle, turnAngle, flWheelHeight, frWheelHeight, blWheelHeight, brWheelHeight;
   getWheelData(spinAngle, turnAngle, flWheelHeight, frWheelHeight, blWheelHeight, brWheelHeight);

   // Bone Wheel01 - right wheel
   // Bone Wheel02 - left wheel
   // Bone Gyro - Gyro, Ram upgrade, Stabilizer upgrade

   // Roll gyro and spin attached wheels
   long gyroHandle = pVis->getAttachmentHandle("Gyro");
   BVisualItem* pGyroAttachment = const_cast<BVisualItem*>(pVis->getAttachment(gyroHandle));
   if (pGyroAttachment)
   {
      BMatrix gyroMtx;
      gyroMtx.makeRotateZ(-turnAngle);

      pGyroAttachment->setTransform(gyroMtx);

      // Spin wheels
      static const char* gWheelNames[2]={"Bone Wheel01", "Bone Wheel02"};
      for (uint i = 0; i < 2; i++)
      {
         BVisualItem* pWheelAttachment = pGyroAttachment->getAttachmentByToBoneName(gWheelNames[i]);
         if (pWheelAttachment)
         {
            BMatrix mtx;// = pWheelAttachment->mTransform;

            // Spin
            if (i == 0)
               mtx.makeRotateX(spinAngle);
            else  // reverse rotation direction of left tires since they're rotated around 180 degrees
               mtx.makeRotateX(-spinAngle);

            // Set transform
            pWheelAttachment->setTransform(mtx);
         }
      }
   }

   // Also roll the ram upgrade
   long ramHandle = pVis->getAttachmentHandle("RamUpgrade");
   BVisualItem* pRamAttachment = const_cast<BVisualItem*>(pVis->getAttachment(ramHandle));
   if (pRamAttachment)
   {
      BMatrix mtx;
      mtx.makeRotateZ(-turnAngle);

      pRamAttachment->setTransform(mtx);
   }
}

//============================================================================
//============================================================================
void BPhysicsWarthogAction::applyAction( const hkStepInfo& stepInfo )
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
//-- FIXING PREFIX BUG ID 3220
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
   float spinSpeed = static_cast<float>(linVel.dot3(chassisRotation.getColumn(2))) / (cTwoPi * cWheelRadius); // rotations / second
   mWheelSpinAngle += (spinSpeed * stepInfo.m_deltaTime * cSpinFactor);
   mWheelSpinAngle = fmod(mWheelSpinAngle, cTwoPi);

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
            contactPts[i] = mWheelContactPts[i] = contactPointWsPosition;

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
   mFlagInAir = (!rayHit[0] && !rayHit[1] && !rayHit[2] && !rayHit[3]);

   BSquadActionMove* pSMA = NULL;
   BSquad* pParentSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
   if (pParentSquad)
   {
      pSMA = pParentSquad->getNonPausedMoveAction_4();
      //pSMA = reinterpret_cast<BSquadActionMove*>(pParentSquad->getActionByType(BAction::cActionTypeSquadMove));
   }

   //============================================================================
   // Input acceleration and turning
   {
      // Input acceleration and turning
      float acceleration = 0.0f;
      float turning = 0.0f;
      bool atGoal = false;

      // Apply acceleration / turning based on controller 2 input

      // Direct driving hack
      static bool driveWarthog = false;
      if (driveWarthog)
      {
         acceleration = -gInputSystem.getGamepad(1).getStickLY() * cDirectInputAccelRate;
         turning = gInputSystem.getGamepad(1).getStickRX() * cDirectInputTurnRate;

         // Velocity damping (the poor man's friction)
         float linDamping = cLinearVelDampingBase;
         if (rayHit[0])
            linDamping += cLinearVelDampingPerTire;
         if (rayHit[1])
            linDamping += cLinearVelDampingPerTire;
         pRB->setLinearDamping(linDamping);
         pRB->setAngularDamping(cAngularVelDamping);
      }
      // Calculate acceleration / turning to get to desired position
      else
      {
         if (rayHit[0] || rayHit[1] || rayHit[2] || rayHit[3])
         {
            calcMovement(stepInfo, acceleration, turning, atGoal, pSMA);
            /*
            if (pSMA)
               calcMovement(stepInfo, acceleration, turning, atGoal, pSMA);
            else if (pWMA)
               calcMovement(stepInfo, acceleration, turning, atGoal, pWMA);
            */
         }
         pRB->setLinearDamping(0.0f);
         pRB->setAngularDamping(0.0f);
      }
      mPrevTurning = pRB->getAngularVelocity()(1);    // save off for wheel movement

      //============================================================================
      // If contacting the ground, apply driving and friction forces
      if (rayHit[0] || rayHit[1] || rayHit[2] || rayHit[3])
      {
         //============================================================================
         // Calculate forward acceleration impulse
         hkVector4 forwardDir;
         forwardDir = chassisRotation.getColumn(2);
         hkVector4 forwardImpulse;
         forwardImpulse.setMul4(stepInfo.m_deltaTime * pRB->getMass() * acceleration, forwardDir);

         // Calculate total gravitational force for this frame
         hkVector4 totalGravForce;
         totalGravForce.setMul4(stepInfo.m_deltaTime * (1.0f + pRB->getMass() * cExtraGravityFactor), gWorld->getPhysicsWorld()->getHavokWorld()->getGravity());

         // Scale up the forward impulse to compensate for gravity messing with desired forward
         float antiGravScalar = -totalGravForce.dot3(forwardDir);
         forwardImpulse.add4(antiGravScalar * forwardDir);

         //============================================================================
         // Side friction impulse
         float sideFriction;
         if (mStayOnPath)
            sideFriction = cPathSideFriction;
         else if (atGoal)
            sideFriction = cMaxSideFriction;
         else
            sideFriction = cMinSideFriction;

         //============================================================================
         // Calculate right frictional, anti-grav impulse
         hkVector4 rightDir;
         rightDir = chassisRotation.getColumn(0);
         float currentVelAlongRight = linVel.dot3(rightDir);

         // Frictional part
         hkVector4 rightImpulse;
         rightImpulse.setMul4(stepInfo.m_deltaTime * pRB->getMass() * currentVelAlongRight * sideFriction, rightDir);

         // Anti-grav part
         float antiGravRightScalar = -totalGravForce.dot3(rightDir);
         rightImpulse.add4(antiGravRightScalar * rightDir);

         //============================================================================
         // Apply linear impulse
         pRB->applyLinearImpulse(forwardImpulse + rightImpulse);

         //============================================================================
         // Calculate turning impulse
         hkVector4 turnImpulse;
         turnImpulse.set(0.0f, stepInfo.m_deltaTime * pRB->getMass() * turning, 0.0f, 0.0f);
         pRB->applyAngularImpulse(turnImpulse);
      }
   }

   // Reorient towards up axis to prevent or limit flipping over
   if (mReorient)
      reorient(stepInfo);
}


//============================================================================
//============================================================================
void BPhysicsWarthogAction::calcMovement(const hkStepInfo& stepInfo, float& acceleration, float& turning, bool& atGoal, BSquadActionMove* pSMA)
{
   atGoal = false;
   mStayOnPath = false;

   //============================================================================
   // Update new desired position (allow acceleration if desired position changed)
   float maxVelocity = 0.0f;
   float squadVelocity = 0.0f;
   BVector currentPos = mEntity->getPosition();
   bool desiredPosIsGoal = true;
   // Look for the physics action here, and if we ain't got teh one,
   // we're so done.  MBean says so. 
   BUnitActionPhysics *pPhysicsAction = reinterpret_cast<BUnitActionPhysics *>(mEntity->getActionByType(BAction::cActionTypeUnitPhysics));
   if (!pPhysicsAction)
   {
      acceleration = 0.0f;
      turning = 0.0f;
      atGoal = true;
      return;
   }

   // Get desired position and velocity from squad move action
   if (pSMA)
   {
      #ifdef _MOVE4
      const BSimTarget *pTarget = pSMA->getTarget();      
      // Units need to move towards their squad's location, not their formation location.
      // mDesiredPos = pSMA->getInterimTarget_4();
//-- FIXING PREFIX BUG ID 3221
      const BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
//--
      if (pSquad)
      {
         // Allow us to change teh flag for settling down teh movement.
         pPhysicsAction->setFlagAllowEarlyTerminate(true);

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
      #else
      // Look some time ahead on the turn path
      // MPB TODO - This is just a hacked 0.5 seconds ahead right now.  Do something better.
      float realTimeNeeded;
      const float lookAheadTime = 0.5f;
      pSMA->getFuturePosition(lookAheadTime, true, mDesiredPos, realTimeNeeded);
      desiredPosIsGoal = (pSMA->getState() != BAction::cStateWorking) || (lookAheadTime > realTimeNeeded);
      maxVelocity = pSMA->getVelocity();
      #endif
      //maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();

      // If we have a low or mid range path, there was a collision and we need to stay on the path more closely
      /*
      const BPathMoveData* pPathData = pWMA->getPathData();
      if (pPathData && ((pPathData->mPathLevel == BPathMoveData::cPathLevelLow) || (pPathData->mPathLevel == BPathMoveData::cPathLevelMid)))
      {
         mStayOnPath = true;
      }
      */
      //mStayOnPath = true;
   }
   // If there is no squad move action, use proto data and the squad position
   else
   {
      // Get max velocity
      BASSERT(mEntity->getUnit()->getProtoObject());
      maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();

//-- FIXING PREFIX BUG ID 3222
      const BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
//--
      if(pSquad)
         mDesiredPos = pSquad->getPosition();
      else
         mDesiredPos = currentPos;
   }

   maxVelocity = max(maxVelocity, 0.001f);
   float decelRate = maxVelocity * cDecelRate;

   //============================================================================
   // Get current and desired xz directions
   BVector currentFwd = mEntity->getForward();
   BVector desiredFwd = (mDesiredPos - currentPos);
   currentFwd.y = 0.0f;
   desiredFwd.y = 0.0f;
   currentFwd.normalize();
   if (!desiredFwd.safeNormalize())
      desiredFwd = currentFwd;

   // Current xz linear velocity
//-- FIXING PREFIX BUG ID 3223
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
   float currentVelAlongFwd = linVel.dot(currentFwd);
   float dist = mDesiredPos.xzDistance(currentPos);
   float maxStoppingDist = 0.5f * maxVelocity * maxVelocity / -decelRate;

   // Decel if within max stopping distance and position is the goal
   if (desiredPosIsGoal && (dist < maxStoppingDist))
   {
      float desiredVelocity = maxVelocity * dist / maxStoppingDist;

      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * desiredVelocity;
      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      float fwdDampConstant = cStopFwdDamp * 2.0f * sqrt(cStopFwdK);
      acceleration = cStopFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;
      atGoal = true;
   }
   // Otherwise, accelerate to maximum velocity
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
         acceleration = dampConst * velDiff - fwdDampConstant * currentVelAlongFwd;
      #else
         float desiredVel = maxVelocity;
         float velDiff = desiredVel - currentVelAlongFwd; // use currentVelAlongFwd because currentVel doesn't indicate direction (this can cause increased reverse velocity)
         float fwdDampConstant = cFwdDamp * 2.0f * sqrt(cFwdK);
         acceleration = cFwdK * velDiff - fwdDampConstant * currentVel;
      #endif
   }

   //============================================================================
   // Turning

   // Update canPivot bool - this is true once the vehicle has "reached" the goal
   // position (not just within stopping dist, but with 1.0).
   if (mCanPivotToTarget)
   {
      if (!atGoal)
         mCanPivotToTarget = false;
   }
   else
   {
      if (atGoal && dist <= 1.0f)
         mCanPivotToTarget = true;
   }

   // Override desiredFwd for the chopper if there is an attack target.  It has fixed
   // guns that must face the target.
   bool attackAction = false;
   if (mFlagChopper && mCanPivotToTarget)
   {
      BUnitActionRangedAttack* pAction = reinterpret_cast<BUnitActionRangedAttack*>(mEntity->getActionByType(BAction::cActionTypeUnitRangedAttack));
      if (pAction && pAction->getTarget())
      {
         attackAction = true;

         BVector targetPos;
         const BSimTarget* pTarget = pAction->getTarget();
         if(pTarget)
         {         
            BEntity* pEntity=gWorld->getEntity(pTarget->getID());
            if (pEntity)
               targetPos = pEntity->getPosition();
            else
               attackAction = false;
         }
         
         // MPB [7/10/2008]
         // If being hijacked (fatality), don't turn to face target.  This is somewhat hacky and needs to be better
         // post E3
         BUnit* pUnit = mEntity->getUnit();
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
   }

   float angleDiff = 0.0f;
   if (dist > 1.0f || attackAction)        // If we're really close, don't turn
   {
      angleDiff = desiredFwd.angleBetweenVector(currentFwd);
      if (desiredFwd.cross(currentFwd).y > 0.0f)
         angleDiff = -angleDiff;
   }

   // If at the goal, orient front or back or warthog towards desired (whichever is closer)
   if (atGoal && !attackAction)
   {
      if (angleDiff > cPiOver2)
         angleDiff = angleDiff - cPi;
      else if (angleDiff < -cPiOver2)
         angleDiff = angleDiff + cPi;
   }

   // Calculate turning to move towards desired using an underdamped spring
   float turnK;
   float turnDampConstant;

   // If we need to stay on the path, turning strength and damping is much higher
   if (mStayOnPath)
   {
      turnK = cPathTurnK;
      turnDampConstant = cPathTurnDamp * 2.0f * sqrt(turnK);
   }
   // Turn strength (K) is lerped based on current forward velocity, this is done so you
   // can't turn on a dime at a standstill
   else
   {
      turnK = cMinTurnK + (cMaxTurnK - cMinTurnK) * Math::Clamp(currentVelAlongFwd / maxVelocity, 0.0f, 1.0f);
      turnDampConstant = cTurnDamp * 2.0f * sqrt(turnK);
   }

   hkVector4 currentAngVel = pRB->getAngularVelocity();
   turning = turnK * angleDiff - turnDampConstant * currentAngVel(1);

    //-- Are we skidding?
   static float cIdleMagnitude = 1.0f;
   static float cSkidMagnitude = 5.0f;   
   if (linVel.length() < cIdleMagnitude)
      mFlagSkidding = false;
   else if (fabs(linVel.dot(mEntity->getRight())) > cSkidMagnitude)
      mFlagSkidding = true;      
   else
      mFlagSkidding = false;      

   // DLM - 10/29/08 - For warthogs, if w'ere at the goal and going very slow, go ahead and turn off the key. 
   // DLM - 11/11/08 - make sure wer'e at the goal, and we had a squad move action this update
   //if (atGoal && linVel.length() < cIdleMagnitude && pSMA)
   if (atGoal && linVel.length() < cIdleMagnitude && pRB->getDeactivatorType() != hkpRigidBodyDeactivator::DEACTIVATOR_NEVER &&
      pPhysicsAction->getFlagAllowEarlyTerminate())
   {
      pPhysicsAction->setFlagEarlyTerminate(true);
   }
}

//============================================================================
//============================================================================
void BPhysicsWarthogAction::calcMovement(const hkStepInfo& stepInfo, float& acceleration, float& turning, bool& atGoal, BUnitActionMoveWarthog* pWMA)
{
   atGoal = false;
   mStayOnPath = false;
   
   //============================================================================
   // Update new desired position (allow acceleration if desired position changed)
   BVector currentPos = mEntity->getPosition();
   float desiredPosIsGoal = true;
   if (pWMA)
   {
      mDesiredPos = pWMA->getTargetPos();;
      desiredPosIsGoal = pWMA->isTargetPosTheGoal();

      // If we have a low or mid range path, there was a collision and we need to stay on the path more closely
      const BPathMoveData* pPathData = pWMA->getPathData();
      if (pPathData && ((pPathData->mPathLevel == BPathMoveData::cPathLevelLow) || (pPathData->mPathLevel == BPathMoveData::cPathLevelMid)))
      {
         mStayOnPath = true;
      }
   }
   else
   {
      atGoal = true;
   }

   // Get velocity and acceleration values
   // MPB TODO - we could store these since they don't change
   BASSERT(mEntity->getUnit()->getProtoObject());
   float maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();
   float decelRate = maxVelocity * cDecelRate;


   //============================================================================
   // Get current and desired xz directions
   BVector currentFwd = mEntity->getForward();
   BVector desiredFwd = (mDesiredPos - currentPos);
   currentFwd.y = 0.0f;
   desiredFwd.y = 0.0f;
   currentFwd.normalize();
   desiredFwd.normalize();

   // Current xz linear velocity
//-- FIXING PREFIX BUG ID 3224
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
   float currentVel = linVel.length();
   float currentVelAlongFwd = linVel.dot(currentFwd);
   float dist = mDesiredPos.xzDistance(currentPos);
   float maxStoppingDist = 0.5f * maxVelocity * maxVelocity / -decelRate;


   // Decel if within max stopping distance and position is the goal
   if (desiredPosIsGoal && (dist < maxStoppingDist))
   {
      float desiredVelocity = maxVelocity * dist / maxStoppingDist;

      float desiredVelAlongFwd = desiredFwd.dot(currentFwd) * desiredVelocity;
      float fwdVelDiff = desiredVelAlongFwd - currentVelAlongFwd;
      float fwdDampConstant = cStopFwdDamp * 2.0f * sqrt(cStopFwdK);
      acceleration = cStopFwdK * fwdVelDiff - fwdDampConstant * currentVelAlongFwd;
   }
   // Otherwise, accelerate to maximum velocity
   else
   {
      float desiredVel = maxVelocity;
      float velDiff = desiredVel - currentVel;
      float fwdDampConstant = cFwdDamp * 2.0f * sqrt(cFwdK);
      acceleration = cFwdK * velDiff - fwdDampConstant * currentVel;
   }

   //============================================================================
   // Turning
   float angleDiff = 0.0f;
   if (dist > 1.0f)        // If we're really close, don't turn
   {
      angleDiff = desiredFwd.angleBetweenVector(currentFwd);
      if (desiredFwd.cross(currentFwd).y > 0.0f)
         angleDiff = -angleDiff;
   }

   // If at the goal, orient front or back or warthog towards desired (whichever is closer)
   if (atGoal)
   {
      if (angleDiff > cPiOver2)
         angleDiff = angleDiff - cPi;
      else if (angleDiff < -cPiOver2)
         angleDiff = angleDiff + cPi;
   }

   // Calculate turning to move towards desired using an underdamped spring
   float turnK;
   float turnDampConstant;

   // If we need to stay on the path, turning strength and damping is much higher
   if (mStayOnPath)
   {
      turnK = cPathTurnK;
      turnDampConstant = cPathTurnDamp * 2.0f * sqrt(turnK);
   }
   // Turn strength (K) is lerped based on current forward velocity, this is done so you
   // can't turn on a dime at a standstill
   else
   {
      turnK = cMinTurnK + (cMaxTurnK - cMinTurnK) * Math::Clamp(currentVelAlongFwd / maxVelocity, 0.0f, 1.0f);
      turnDampConstant = cTurnDamp * 2.0f * sqrt(turnK);
   }

   hkVector4 currentAngVel = pRB->getAngularVelocity();
   turning = turnK * angleDiff - turnDampConstant * currentAngVel(1);
}

//============================================================================
// This code is from hkpReorientAction::applyAction in Havok's sdk
//============================================================================
void BPhysicsWarthogAction::reorient(const hkStepInfo& stepInfo)
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
// BPhysicsWarthogAction::spawnPhysicsEventVisuals
//============================================================================
void BPhysicsWarthogAction::spawnPhysicsEventVisuals(long terrainEffectsHandle)
{
   float linVeloLen = mPrevLinearVelocity.length();
   if(linVeloLen < 0.04f)
   {
      if(mTrailDecal[0]) mTrailDecal[0]->addPausePointToTrail();
      if(mTrailDecal[1]) mTrailDecal[1]->addPausePointToTrail();
      return;
   }

   if(getFlagInAir())
   {
      
      if(mTrailDecal[0]) mTrailDecal[0]->addPausePointToTrail();
      if(mTrailDecal[1]) mTrailDecal[1]->addPausePointToTrail();
      mWheelContactPts[0].set(0,0,0);
      mWheelContactPts[1].set(0,0,0);
      mWheelContactPts[2].set(0,0,0);
      mWheelContactPts[3].set(0,0,0);
      return;
   }

   
   
   BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(terrainEffectsHandle, true);
   if(pTerrainEffect == NULL)
      return;

   const BVector cEmpty(0,0,0);

   if(!mWheelContactPts[0].almostEqual(cEmpty) && !mWheelContactPts[1].almostEqual(cEmpty))
   {
      BVector half = (mWheelContactPts[0] + mWheelContactPts[1]) * 0.5f;
      BYTE surfaceType = gTerrainSimRep.getTileType(half);

      //trail
      if(mTrailDecal[0] != NULL)
      {
//-- FIXING PREFIX BUG ID 3225
         const BRibbonCreateParams* parms = pTerrainEffect->getTrailDecalHandleForType(surfaceType, -1);
//--
         if(parms != NULL)
         {   
            if(!mTrailDecal[0]->isSameParams(*parms))
            {
               mTrailDecal[0]->endTrail();
               mTrailDecal[0]->startTrail(*parms);
            }

            mTrailDecal[0]->addPointToTrail(half.x,half.z);
         }
         else
         {
            mTrailDecal[0]->endTrail();
         }
      }
   }
   else
   {
      if(mTrailDecal[0] != NULL)
         mTrailDecal[0]->endTrail();
   }

   
   if(!mFlagChopper && !mWheelContactPts[2].almostEqual(cEmpty) && !mWheelContactPts[3].almostEqual(cEmpty))
   {
      BVector half = (mWheelContactPts[2] + mWheelContactPts[3]) * 0.5f;
      BYTE surfaceType = gTerrainSimRep.getTileType(half);

      //trail
      if(mTrailDecal[1] != NULL)
      {
//-- FIXING PREFIX BUG ID 3226
         const BRibbonCreateParams* parms = pTerrainEffect->getTrailDecalHandleForType(surfaceType, -1);
//--
         if(parms != NULL)
         {   
            if(!mTrailDecal[1]->isSameParams(*parms))
            {
               mTrailDecal[1]->endTrail();
               mTrailDecal[1]->startTrail(*parms);
            }

            mTrailDecal[1]->addPointToTrail(half.x,half.z);
         }
         else
         {
            mTrailDecal[1]->endTrail();
         }
      }
   }
   else
   {
      if(mTrailDecal[1] != NULL)
         mTrailDecal[1]->endTrail();
   }
   

}

//============================================================================
// Debug funcs
//============================================================================
#ifndef BUILD_FINAL
   void BPhysicsWarthogAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
//-- FIXING PREFIX BUG ID 3228
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
      
      if (gConfig.isDefined(cConfigDisplayWarthogVelocity))
      {
//-- FIXING PREFIX BUG ID 3229
         const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
#if defined(CODE_ANALYSIS_ENABLED)
		BVector linVel(rand(), 0, rand());
#else
         BVector linVel = pRB->getLinearVelocity();
         linVel.y = 0;
#endif

         BVector currentFwd = mEntity->getForward();
         currentFwd.y = 0.0f;
         currentFwd.normalize();

         float currentVelAlongFwd = linVel.dot(currentFwd);
         gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "Velocity = %f", currentVelAlongFwd);
      }
   }
#endif
