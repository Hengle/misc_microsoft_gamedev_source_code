//============================================================================
// File: physicsgrizzlyaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicsgrizzlyaction.h"
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
//============================================================================
// Constants

const float cTeleportImpulseFactor = 0.16666666f;
const float cExtraGravityFactor = (4.0f - GAMEUNITS_PER_METER_FACTOR) / GAMEUNITS_PER_METER_FACTOR;

// Chassis inertia constants
const float cChassisUnitInertiaYaw = 3.0f; 
const float cChassisUnitInertiaRoll = 2.4f; 
const float cChassisUnitInertiaPitch = 3.0f; 

// Suspension constants
const float cSpringLength = 1.3f;
const float cWheelRadius = 1.0f;
const float cSpringStrength = 50.0f;
const float cDampingCompression = 2.0f;
const float cDampingRelaxation = 2.0f;
const float cNormalClippingAngle = 0.1f;

/*
const float cHardpointFrontZ = 1.3f;
const float cHardpointBackZ = -1.1f;
const float cHardpointY = -0.05f;
const float cHardpointX = 1.1f;
*/
const float cHardpointFrontZ = 5.0f;
const float cHardpointBackZ = -6.0f;
const float cHardpointY = -0.05f;
const float cHardpointX = 3.0f;

const hkVector4 cHardpoints[4] = 
{
   hkVector4(-cHardpointX, cHardpointY, cHardpointFrontZ, 0.0f),
   hkVector4( cHardpointX, cHardpointY, cHardpointFrontZ, 0.0f),
   hkVector4(-cHardpointX, cHardpointY, cHardpointBackZ, 0.0f),
   hkVector4( cHardpointX, cHardpointY, cHardpointBackZ, 0.0f)
};

const hkVector4 cTreadPoints[2] = 
{
   hkVector4(-6.0f, cHardpointY, 0.0f, 0.0f),
   hkVector4( 6.0f, cHardpointY, 0.0f, 0.0f),
};


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
const float cMinSideFriction = -40.0f;
const float cPathSideFriction = -40.0f;

// Reorientation constants
const hkVector4 cReorientRotationAxis(0.0f, 0.0f, 1.0f, 0.0f);
const hkVector4 cReorientUpAxis(0.0f, 1.0f, 0.0f, 0.0f);
const float cReorientStrength = 0.75f;
const float cReorientDamping = 0.1f;


const float cTreadHingeAngleMax = cPiOver4 / 3.0f;
const float cTreadHingeAngleMin = -cPiOver4 / 3.0f;
const float cTreadHingeAngleAdjustmentRate = cPiOver4 * 1.0f;
const float cTreadSuspensionHeightMax = 0.7f;
const float cTreadSuspensionHeightMin = -0.7f;
const float cTreadSuspensionHeightAdjustmentRate = 8.0f;

const float cTreadRaySeparationDistanceFront = 4.7f;
const float cTreadRaySeparationDistanceBack = 3.5f;
const float cTreadRayLength = 6.0f;

const float cTreadHardPointHeight = 2.15f;



const float cTreadHardpointFrontZ = 3.0f;
const float cTreadHardpointFrontX = 4.0f;
const float cTreadHardpointBackZ = -4.3;
const float cTreadHardpointBackX = 4.5f;
const float cTreadHardpointY = -0.05f;

const float cTreadRaySeparationDistance[4] = 
{
   cTreadRaySeparationDistanceFront,
   cTreadRaySeparationDistanceFront,
   cTreadRaySeparationDistanceBack,
   cTreadRaySeparationDistanceBack
};

const hkVector4 cTreadHardpoints1[4] = 
{
   hkVector4(-cTreadHardpointFrontX, cTreadHardpointY, cTreadHardpointFrontZ, 0.0f),
   hkVector4( cTreadHardpointFrontX, cTreadHardpointY, cTreadHardpointFrontZ, 0.0f),
   hkVector4(-cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ, 0.0f),
   hkVector4( cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ, 0.0f)
};

const hkVector4 cTreadHardpoints2[4] = 
{
   hkVector4(-cTreadHardpointFrontX, cTreadHardpointY, cTreadHardpointFrontZ + cTreadRaySeparationDistanceFront, 0.0f),
   hkVector4( cTreadHardpointFrontX, cTreadHardpointY, cTreadHardpointFrontZ + cTreadRaySeparationDistanceFront, 0.0f),
   hkVector4(-cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ - cTreadRaySeparationDistanceBack, 0.0f),
   hkVector4( cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ - cTreadRaySeparationDistanceBack, 0.0f)
};




float GrizzlyWrapUV(float value);


//============================================================================
//============================================================================
BPhysicsGrizzlyAction::BPhysicsGrizzlyAction(hkpRigidBody* body, BEntity* pEntity) :
   hkpUnaryAction( body ),
   mPrevTurning(0.0f),
   mReorient(true),
   mStayOnPath(false),
   mTrailDecal(NULL)
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


   mTreadTangentHingeAngleMax = tan(cTreadHingeAngleMax);
   mTreadTangentHingeAngleMin = tan(-cTreadHingeAngleMin);


   // Init tread positions
   for(int i = 0; i < 4; i++)
   {
      mCurrentTreadSuspensionHeight[i] = 0.0f;
      mCurrentTreadHingeAngle[i] = 0.0f;
   }

   mCurrentLeftTreadScroll = 0.0f;
   mCurrentRightTreadScroll = 0.0f;

   mTrailDecal = new BTerrainTrailDecalGenerator();
}
//============================================================================
//============================================================================
BPhysicsGrizzlyAction::~BPhysicsGrizzlyAction()
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
void BPhysicsGrizzlyAction::getTreadData(float& flTreadHeight, float& frTreadHeight, float& blTreadHeight, float& brTreadHeight,
                                          float& flTreadAngle, float& frTreadAngle, float& blTreadAngle, float& brTreadAngle,
                                          float& leftTreadScroll, float& rightTreadScroll) const
{
   static bool current = true;
   if(current)
   {
      flTreadHeight = mCurrentTreadSuspensionHeight[0];
      frTreadHeight = mCurrentTreadSuspensionHeight[1];
      blTreadHeight = mCurrentTreadSuspensionHeight[2];
      brTreadHeight = mCurrentTreadSuspensionHeight[3];

      flTreadAngle = mCurrentTreadHingeAngle[0];
      frTreadAngle = mCurrentTreadHingeAngle[1];
      blTreadAngle = mCurrentTreadHingeAngle[2];
      brTreadAngle = mCurrentTreadHingeAngle[3];
   }
   else
   {
      flTreadHeight = mDesiredTreadSuspensionHeight[0];
      frTreadHeight = mDesiredTreadSuspensionHeight[1];
      blTreadHeight = mDesiredTreadSuspensionHeight[2];
      brTreadHeight = mDesiredTreadSuspensionHeight[3];

      flTreadAngle = mDesiredTreadHingeAngle[0];
      frTreadAngle = mDesiredTreadHingeAngle[1];
      blTreadAngle = mDesiredTreadHingeAngle[2];
      brTreadAngle = mDesiredTreadHingeAngle[3];
   }

   leftTreadScroll = mCurrentLeftTreadScroll;
   rightTreadScroll = mCurrentRightTreadScroll;
}

//============================================================================
//============================================================================
void BPhysicsGrizzlyAction::applyAction( const hkStepInfo& stepInfo )
{
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
   BASSERT(pRB);

   // Get hkpSampledHeightFieldShape for raycasting against
   BASSERT(gTerrainSimRep.getPhysicsHeightfield());
   const hkpEntity* pHeightfieldEntity = gTerrainSimRep.getPhysicsHeightfield()->getHKEntity();
   BASSERT(pHeightfieldEntity->getCollidable());
   hkpSampledHeightFieldShape* pHeightfield = (hkpSampledHeightFieldShape*) pHeightfieldEntity->getCollidable()->getShape();

   // Get current chassis rotation
   hkRotation chassisRotation;
   chassisRotation.set(pRB->getRotation());

   hkVector4 linVel = pRB->getLinearVelocity();
   mPrevLinearVelocity.x = linVel(0);
   mPrevLinearVelocity.y = linVel(1);
   mPrevLinearVelocity.z = linVel(2);
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
      //hkVector4 suspDir = (hkVector4) -cYAxisVector;
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
         rayEndPts[i] = hardPts[i] + suspDir * (cSpringLength + cWheelRadius);

         // Raycast - don't raycast against the whole world, just the terrain heightfield, this
         // makes for a 2x perf increase
         hkpShapeRayCastInput input;
         input.m_from = hardPts[i];
         input.m_to = rayEndPts[i];
         input.m_rayShapeCollectionFilter = NULL;
         hkpShapeRayCastOutput output;
         pHeightfield->castRay(input, output);

         //============================================================================
         // Calculate collision info and suspension forces if we got a collision for
         // with this wheel
         if (output.hasHit())
         {
            rayHit[i] = true;
            //contactNormals[i] = output.m_normal;
            contactNormals[i] = cYAxisVector;

            // Calc length of suspension from hard point to collision point
            hkReal hitDistance = output.m_hitFraction * (cSpringLength + cWheelRadius);
            mCurrentSuspensionLengths[i] = hitDistance - cWheelRadius;

            // Calc contact position
            hkVector4 contactPointWsPosition;
            contactPointWsPosition.setAddMul4(hardPts[i], suspDir, hitDistance);
            contactPts[i] =  mWheelContactPts[i] = contactPointWsPosition;

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


      
      //============================================================================
      // Calc left/right tread scrolling
      hkVector4 treadPoint;
      hkVector4 chassis_velocity_at_treadPoint;
      hkReal treadForwardVel;

      static float sTreadVelocityScalar = -0.001f;


      // Left
      treadPoint.setTransformedPos(trans, cTreadPoints[0]);

      pRB->getPointVelocity(treadPoint, chassis_velocity_at_treadPoint);
      treadForwardVel = trans.getColumn(2).dot3( chassis_velocity_at_treadPoint );

      mCurrentLeftTreadScroll += treadForwardVel * sTreadVelocityScalar;
      mCurrentLeftTreadScroll = GrizzlyWrapUV(mCurrentLeftTreadScroll);



      // Right
      treadPoint.setTransformedPos(trans, cTreadPoints[1]);

      pRB->getPointVelocity(treadPoint, chassis_velocity_at_treadPoint);
      treadForwardVel = trans.getColumn(2).dot3( chassis_velocity_at_treadPoint );

      mCurrentRightTreadScroll += treadForwardVel * sTreadVelocityScalar;
      mCurrentRightTreadScroll = GrizzlyWrapUV(mCurrentRightTreadScroll);
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


   BSquadActionMove* pSMA = NULL;
   BSquad* pParentSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
   if (pParentSquad)
      pSMA = reinterpret_cast<BSquadActionMove*>(pParentSquad->getActionByType(BAction::cActionTypeSquadMove));

   //============================================================================
   // Input acceleration and turning
   {
      // Input acceleration and turning
      float acceleration = 0.0f;
      float turning = 0.0f;
      bool atGoal = false;

      // Calculate acceleration / turning to get to desired position
      if (rayHit[0] || rayHit[1] || rayHit[2] || rayHit[3])
      {
         calcMovement(acceleration, turning, atGoal, pSMA);
      }

      pRB->setLinearDamping(0.0f);
      pRB->setAngularDamping(0.0f);

      mPrevTurning = pRB->getAngularVelocity()(1);    // save off for wheel movement


      // If contacting the ground, apply driving and friction forces

      if (1)//(rayHit[0] || rayHit[1] || rayHit[2] || rayHit[3])
      {
         // Calculate forcable teleport impulse
         hkVector4 teleportImpulse;
         {
            hkVector4 currentVel = pRB->getLinearVelocity();
            hkVector4 currentPos = pRB->getPosition();
            hkVector4 desiredPos = currentPos;
            if (pSMA)
            {
               #ifdef NEW_TURNRADIUS
                  desiredPos = pSMA->getTurnRadiusPos();
               #else
                  desiredPos = pSMA->getOwner()->getPosition();
               #endif
            }
            else if (pParentSquad)
               desiredPos = pParentSquad->getPosition();
 
            hkVector4 oppImpulse = -currentVel;
            oppImpulse(1) = 0.0f;
            oppImpulse.mul4(pRB->getMass());
            
            hkVector4 impulse = desiredPos - currentPos;
            impulse(1) = 0.0f;
            impulse.mul4(pRB->getMass() * stepInfo.m_invDeltaTime * cTeleportImpulseFactor);
 
            teleportImpulse = oppImpulse + impulse;

            // only in plane of orientation
            hkRotation rot;
            rot.set(pRB->getRotation());
            hkVector4 chassisUp = rot.getColumn(1);
            hkReal impulseScalarY = teleportImpulse.dot3( chassisUp );
            chassisUp.mul4(impulseScalarY);
            teleportImpulse = teleportImpulse - chassisUp;

         }
         // Calculate natural physics impulse
         hkVector4 physicsImpulse;
         {
            // Calculate forward acceleration impulse
            hkVector4 forwardImpulse;
            forwardImpulse = chassisRotation.getColumn(2);
            forwardImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * acceleration);
            //pRB->applyLinearImpulse(forwardImpulse);
 
            // Side friction impulse
            float sideFriction;
            if (mStayOnPath)
               sideFriction = cPathSideFriction;
            else if (atGoal)
               sideFriction = cMaxSideFriction;
            else
               sideFriction = cMinSideFriction;
 
            hkVector4 rightImpulse;
            rightImpulse = chassisRotation.getColumn(0);
            float currentVelAlongRight = linVel.dot3(rightImpulse);
            rightImpulse.mul4(stepInfo.m_deltaTime * pRB->getMass() * currentVelAlongRight * sideFriction);
            //pRB->applyLinearImpulse(rightImpulse);
            physicsImpulse = forwardImpulse + rightImpulse;
         }
 
         // Apply blend of teleport ant physics impulses
         static float linearAmtFactor = 1.0f;
         pRB->applyLinearImpulse(teleportImpulse * linearAmtFactor + physicsImpulse * (1.0f - linearAmtFactor));

         // Calculate turning impulse
         hkVector4 turnImpulse;
         turnImpulse.set(0.0f, stepInfo.m_deltaTime * pRB->getMass() * turning, 0.0f, 0.0f);
         pRB->applyAngularImpulse(turnImpulse);
      }
   }

   /*
   // Reorient towards up axis to prevent or limit flipping over
   if (mReorient)
      reorient(stepInfo);
      */



   // Compute new desired tread params (angle and suspension)
   //
   int i;

   hkTransform trans;
   trans.set(pRB->getRotation(), pRB->getPosition());

   hkRotation rot;
   rot.set(pRB->getRotation());
   hkVector4 suspDir = -rot.getColumn(1);

   for(i = 0; i < 4; i++)
   {
      hkVector4 startPt1, startPt2;
      hkVector4 endPt1, endPt2;
      float contactDist1, contactDist2;

      // Compute start and end points of the casting rays
      startPt1.setTransformedPos(trans, cTreadHardpoints1[i]);
      startPt1 = startPt1 + suspDir * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
      endPt1 = startPt1 + suspDir * cTreadRayLength;

      startPt2.setTransformedPos(trans, cTreadHardpoints2[i]);
      startPt2 = startPt2 + suspDir * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
      endPt2 = startPt2 + suspDir * cTreadRayLength;



      // Raycast both rays
      // - don't raycast against the whole world, just the terrain heightfield, this
      //   makes for a 2x perf increase
      hkpShapeRayCastInput input1;
      input1.m_from = startPt1;
      input1.m_to = endPt1;
      input1.m_rayShapeCollectionFilter = NULL;
      hkpShapeRayCastOutput output1;
      pHeightfield->castRay(input1, output1);

      hkpShapeRayCastInput input2;
      input2.m_from = startPt2;
      input2.m_to = endPt2;
      input2.m_rayShapeCollectionFilter = NULL;
      hkpShapeRayCastOutput output2;
      pHeightfield->castRay(input2, output2);

      if (output1.hasHit())
      {
         contactDist1 = output1.m_hitFraction * cTreadRayLength;
      }
      else
      {
         contactDist1 = cMaximumFloat;
      }

      if (output2.hasHit())
      {
         contactDist2 = output2.m_hitFraction * cTreadRayLength;
      }
      else
      {
         contactDist2 = cMaximumFloat;
      }


      // Change contact distance space to be from the center of the ray, and clamp maximum extends
      //
      contactDist1 -= (cTreadRayLength * 0.5f);
      contactDist2 -= (cTreadRayLength * 0.5f);

      // Invert so positive contact distance is below the ground and negative is above
      contactDist1 *= -1.0f;
      contactDist2 *= -1.0f;

      contactDist1 = Math::Clamp(contactDist1, cTreadSuspensionHeightMin, cTreadSuspensionHeightMax);
      contactDist2 = Math::Clamp(contactDist2, cTreadSuspensionHeightMin - (cTreadRaySeparationDistance[i] * mTreadTangentHingeAngleMin), cTreadSuspensionHeightMax + (cTreadRaySeparationDistance[i] * mTreadTangentHingeAngleMax));


      #ifndef BUILD_FINAL
         mTreadCollisionDist1[i] = contactDist1;
         mTreadCollisionDist2[i] = contactDist2;
      #endif

      // Compute angle (theta) for line that crosses through both contact points
      float diff = contactDist2 - contactDist1;
      float theta = atanf(diff / cTreadRaySeparationDistance[i]);
      
      float desiredHingeAngle = 0.0f;
      float desiredSuspensionHeight = 0.0f;

      
      // Clamp theta
      if((theta < cTreadHingeAngleMin) || (theta > cTreadHingeAngleMax))
      {
         desiredHingeAngle = Math::Clamp(theta, cTreadHingeAngleMin, cTreadHingeAngleMax);

         if(theta > 0)
         {
            desiredSuspensionHeight = contactDist2 - (cTreadRaySeparationDistance[i] * mTreadTangentHingeAngleMax);
         }
         else
         {
            desiredSuspensionHeight = contactDist1;
         }
      }
      else
      {
         desiredHingeAngle = theta;
         desiredSuspensionHeight = contactDist1;
      }


      mDesiredTreadHingeAngle[i] = desiredHingeAngle;
      mDesiredTreadSuspensionHeight[i] = desiredSuspensionHeight;
   }


   // Interpolate current tread params into desired params over time
   //
   for(i = 0; i < 4; i++)
   {
      if(mCurrentTreadHingeAngle[i] != mDesiredTreadHingeAngle[i])
      {
         if(mCurrentTreadHingeAngle[i] < mDesiredTreadHingeAngle[i])
         {
            mCurrentTreadHingeAngle[i] += cTreadHingeAngleAdjustmentRate * stepInfo.m_deltaTime;
            if(mCurrentTreadHingeAngle[i] > mDesiredTreadHingeAngle[i])
               mCurrentTreadHingeAngle[i] = mDesiredTreadHingeAngle[i];
         }
         else 
         {
            mCurrentTreadHingeAngle[i] -= cTreadHingeAngleAdjustmentRate * stepInfo.m_deltaTime;
            if(mCurrentTreadHingeAngle[i] < mDesiredTreadHingeAngle[i])
               mCurrentTreadHingeAngle[i] = mDesiredTreadHingeAngle[i];
         }      
      }

      if(mCurrentTreadSuspensionHeight[i] != mDesiredTreadSuspensionHeight[i])
      {
         if(mCurrentTreadSuspensionHeight[i] < mDesiredTreadSuspensionHeight[i])
         {
            mCurrentTreadSuspensionHeight[i] += cTreadSuspensionHeightAdjustmentRate * stepInfo.m_deltaTime;
            if(mCurrentTreadSuspensionHeight[i] > mDesiredTreadSuspensionHeight[i])
               mCurrentTreadSuspensionHeight[i] = mDesiredTreadSuspensionHeight[i];
         }
         else 
         {
            mCurrentTreadSuspensionHeight[i] -= cTreadSuspensionHeightAdjustmentRate * stepInfo.m_deltaTime;
            if(mCurrentTreadSuspensionHeight[i] < mDesiredTreadSuspensionHeight[i])
               mCurrentTreadSuspensionHeight[i] = mDesiredTreadSuspensionHeight[i];
         }  
      }
   }

}


//============================================================================
//============================================================================
void BPhysicsGrizzlyAction::calcMovement(float& acceleration, float& turning, bool& atGoal, BSquadActionMove* pSMA)
{
   atGoal = false;
   mStayOnPath = true;
   

   // Initialize desiredFwd to current fwd in case there's no squad below (I don't know
   // why this is happening right now).
   BVector desiredFwd = mEntity->getForward();

   //============================================================================
   // Update new desired position (allow acceleration if desired position changed)
   float maxVelocity = 0.0f;
   BVector currentPos = mEntity->getPosition();
   bool desiredPosIsGoal = true;
   // Get desired position and velocity from squad move action
   if (pSMA)
   {
      // Look some time ahead on the turn path
      // MPB TODO - This is just a hacked 0.5 seconds ahead right now.  Do something better.
      #ifdef _MOVE4
      const BSimTarget *pTarget = pSMA->getTarget();      
      // mDesiredPos = pSMA->getInterimTarget_4();
      BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
      if (pSquad)
      {
         #ifdef NEW_TURNRADIUS
            mDesiredPos = pSMA->getTurnRadiusPos();
         #else
            mDesiredPos = pSquad->getPosition();
         #endif
         BVector direction = pTarget->getPosition() - mDesiredPos;
         // DLM 6/19/08 - Check the squad's state.  If at any time we're in wait state, then set mDesirePosIsGoal to true.
         BActionState state = pSMA->getState();
         if (direction.length() < 0.05f || state == BAction::cStateWait)
            desiredPosIsGoal = true;
         else
            desiredPosIsGoal = false;
         maxVelocity = pSMA->getPlatoonVelocity_4();
         //desiredFwd = mDesiredPos - currentPos;
         #ifdef NEW_TURNRADIUS
            desiredFwd = pSMA->getTurnRadiusFwd();
         #else
            desiredFwd = pSquad->getForward();
         #endif
      }
      #else
      float realTimeNeeded;
      const float lookAheadTime = 0.5f;
      pSMA->getFuturePosition(lookAheadTime, true, mDesiredPos, realTimeNeeded);
      desiredPosIsGoal = (pSMA->getState() != BAction::cStateWorking) || (lookAheadTime > realTimeNeeded);
      maxVelocity = pSMA->getVelocity();

      // If we have a low or mid range path, there was a collision and we need to stay on the path more closely
      /*
      const BPathMoveData* pPathData = pWMA->getPathData();
      if (pPathData && ((pPathData->mPathLevel == BPathMoveData::cPathLevelLow) || (pPathData->mPathLevel == BPathMoveData::cPathLevelMid)))
      {
         mStayOnPath = true;
      }
      */
      //mStayOnPath = true;

      BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
      if(pSquad)
      {
         desiredFwd = pSquad->getForward();
      }
      #endif
   }
   // If there is no squad move action, use proto data and the squad position
   else
   {
      // Get max velocity
      BASSERT(mEntity->getUnit()->getProtoObject());
      maxVelocity = mEntity->getUnit()->getProtoObject()->getDesiredVelocity();

      BSquad* pSquad = (reinterpret_cast<BUnit*>(mEntity))->getParentSquad();
      if(pSquad)
      {
         mDesiredPos = pSquad->getPosition();
         #ifndef NEW_TURNRADIUS
            desiredFwd = pSquad->getForward();
         #endif
      }
      else
         mDesiredPos = currentPos;
   }

   float decelRate = maxVelocity * cDecelRate;


   //============================================================================
   // Get current and desired xz directions
   BVector currentFwd = mEntity->getForward();
   currentFwd.y = 0.0f;
   currentFwd.normalize();

   //BVector desiredFwd = (mDesiredPos - currentPos);
   desiredFwd.y = 0.0f;
   if (!desiredFwd.safeNormalize())
      desiredFwd = currentFwd;

   // Current xz linear velocity
   hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
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
      atGoal = true;
   }
   // Otherwise, accelerate to maximum velocity
   else
   {
      float desiredVel = maxVelocity;
      float velDiff = desiredVel - currentVelAlongFwd; // use currentVelAlongFwd because currentVel doesn't indicate direction (this can cause increased reverse velocity)
      float fwdDampConstant = cFwdDamp * 2.0f * sqrt(cFwdK);
      acceleration = cFwdK * velDiff - fwdDampConstant * currentVel;
   }

   //============================================================================
   // Turning
   float angleDiff = 0.0f;
//   if (dist > 1.0f)        // If we're really close, don't turn
   {
      angleDiff = desiredFwd.angleBetweenVector(currentFwd);
      if (desiredFwd.cross(currentFwd).y > 0.0f)
         angleDiff = -angleDiff;
   }

   // If at the goal, orient front or back or warthog towards desired (whichever is closer)
   /*
//   if (atGoal)
   {
      if (angleDiff > cPiOver2)
         angleDiff = angleDiff - cPi;
      else if (angleDiff < -cPiOver2)
         angleDiff = angleDiff + cPi;
   }
   */

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
void BPhysicsGrizzlyAction::reorient(const hkStepInfo& stepInfo)
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
// BPhysicsGrizzlyAction::spawnPhysicsEventVisuals
//============================================================================
void BPhysicsGrizzlyAction::spawnPhysicsEventVisuals(long terrainEffectsHandle)
{

   float linVeloLen = mPrevLinearVelocity.length();
   if(linVeloLen < 0.01f)
      return;

   BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(terrainEffectsHandle, true);
   if(pTerrainEffect == NULL)
      return;

   const BVector cEmpty(0,0,0);
   //we need a better 'update hasn't been run yet' condition here...
   if(mWheelContactPts[0].almostEqual(cEmpty) || mWheelContactPts[1].almostEqual(cEmpty))
   {
      if(mTrailDecal != NULL)
         mTrailDecal->endTrail();
      return;
   }

   BVector half = (mWheelContactPts[0] + mWheelContactPts[1]) * 0.5f;
   BYTE surfaceType = gTerrainSimRep.getTileType(half);


   //front tires
   if(mTrailDecal != NULL)
   {
      BRibbonCreateParams* parms = pTerrainEffect->getTrailDecalHandleForType(surfaceType, -1);
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
   void BPhysicsGrizzlyAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      // Re-enable this when needed - will need to cache the hard points, contact points, and end points
      // in the apply action - I don't want to take up the memory right now
      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
         hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);

         // Get orientation and suspension direction (down)
         hkTransform trans;
         trans.set(pRB->getRotation(), pRB->getPosition());

         hkRotation rot;
         rot.set(pRB->getRotation());
         hkVector4 rigidBodyUp = -rot.getColumn(1);
         
         hkVector4 worldUp = (hkVector4) -cYAxisVector;

         hkVector4 startPt1, startPt2;
         hkVector4 endPt1, endPt2;
         hkVector4 collisionPt1;

         
         hkVector4 collisionPt;
         BMatrix boxMat;
         boxMat.makeIdentity();

         for(int i = 0; i < 4; i++)
         {
            // Compute start and end points of the suspension rays
            startPt1.setTransformedPos(trans, cHardpoints[i]);
            collisionPt1 = startPt1 + mSuspensionDist[i] * worldUp * (cSpringLength + cWheelRadius);
            endPt1 = startPt1 + worldUp * (cSpringLength + cWheelRadius);

            gpDebugPrimitives->addDebugLine((BVector)startPt1, (BVector)collisionPt1, 0xff00ff00, 0xff00ff00);
            gpDebugPrimitives->addDebugLine((BVector)collisionPt1, (BVector)endPt1, 0xffff0000, 0xffff0000);

            // Compute start and end points of the casting rays
            startPt1.setTransformedPos(trans, cTreadHardpoints1[i]);
            startPt1 = startPt1 + rigidBodyUp * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
            endPt1 = startPt1 + rigidBodyUp * cTreadRayLength;

            startPt2.setTransformedPos(trans, cTreadHardpoints2[i]);
            startPt2 = startPt2 + rigidBodyUp * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
            endPt2 = startPt2 + rigidBodyUp * cTreadRayLength;

            gpDebugPrimitives->addDebugLine((BVector)startPt1, (BVector)endPt1, 0xffffff00, 0xffffff00);
            gpDebugPrimitives->addDebugLine((BVector)startPt2, (BVector)endPt2, 0xffffff00, 0xffffff00);


            
            collisionPt.setTransformedPos(trans, cTreadHardpoints1[i]);
            collisionPt = collisionPt + rigidBodyUp * cTreadHardPointHeight;
            collisionPt = collisionPt - rigidBodyUp * mTreadCollisionDist1[i];
            boxMat.setTranslation((BVector)collisionPt);
            gpDebugPrimitives->addDebugBox(boxMat, 0.25f, 0xffff0000);
            
            collisionPt.setTransformedPos(trans, cTreadHardpoints2[i]);
            collisionPt = collisionPt + rigidBodyUp * cTreadHardPointHeight;
            collisionPt = collisionPt - rigidBodyUp * mTreadCollisionDist2[i];
            boxMat.setTranslation((BVector)collisionPt);
            gpDebugPrimitives->addDebugBox(boxMat, 0.25f, 0xffff0000);
         }
      }

      if (gConfig.isDefined(cConfigDisplayWarthogVelocity))
      {
         hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
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


float GrizzlyWrapUV(float value)
{
   // wrap scroll values
   if((value > 1.0f) || (value < 0.0f))
   {
      float intergerPortion;
      float fractionalPortion;
      fractionalPortion = modf(value, &intergerPortion);

      if(fractionalPortion < 0.0f)
         return(1 + fractionalPortion);
      else
         return(fractionalPortion);
   }

   return(value);
}