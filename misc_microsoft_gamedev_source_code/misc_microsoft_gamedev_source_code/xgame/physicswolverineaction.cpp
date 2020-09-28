//============================================================================
// File: physicswolverineaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicswolverineaction.h"
#include "object.h"
#include "visual.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "terrainphysicsheightfield.h"
#include "configsgame.h"

//============================================================================
// Constants
const float cSpringLength = 1.3f;
const float cWheelRadius = 1.0f;

const float cTreadHingeAngleMax = cPiOver4 / 3.0f;
const float cTreadHingeAngleMin = -cPiOver4 / 3.0f;
const float cTreadHingeAngleAdjustmentRate = cPiOver4 * 1.0f;
const float cTreadSuspensionHeightMax = 0.7f;
const float cTreadSuspensionHeightMin = -0.7f;
const float cTreadSuspensionHeightAdjustmentRate = 8.0f;

const float cTreadRaySeparationDistanceBack = 2.5f;
const float cTreadRayLength = 6.0f;

const float cTreadHardPointHeight = 2.15f;

const float cTreadHardpointBackZ = -3.0f;
const float cTreadHardpointBackX = 3.5f;
const float cTreadHardpointY = -0.4f;

const hkVector4 cTreadPoints[2] = 
{
   hkVector4(-2.5f, cTreadHardpointY, -3.4f, 0.0f),
   hkVector4( 2.5f, cTreadHardpointY, -3.4f, 0.0f),
};

const float cTreadRaySeparationDistance[2] = 
{
   cTreadRaySeparationDistanceBack,
   cTreadRaySeparationDistanceBack
};

const hkVector4 cTreadHardpoints1[2] = 
{
   hkVector4(-cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ, 0.0f),
   hkVector4( cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ, 0.0f)
};

const hkVector4 cTreadHardpoints2[2] = 
{
   hkVector4(-cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ - cTreadRaySeparationDistanceBack, 0.0f),
   hkVector4( cTreadHardpointBackX, cTreadHardpointY, cTreadHardpointBackZ - cTreadRaySeparationDistanceBack, 0.0f)
};

// Wheel visual constants
const float cSpinFactor = 2.0f;
const float cMaxWheelTurn = cPiOver2 * 0.5f;
const float cSpringStableLength = 1.0f;
const float cWheelHeightMin = -1.0f;
const float cWheelHeightMax = 0.3f;

float WolverineWrapUV(float value)
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

//============================================================================
//============================================================================
BPhysicsWolverineAction::BPhysicsWolverineAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo) :
   BPhysicsGroundVehicleAction(body, pEntity, pInfo),
   mWheelSpinAngle(0.0f),
   mPrevYaw(0.0f)
{
   mTreadTangentHingeAngleMax = tan(cTreadHingeAngleMax);
   mTreadTangentHingeAngleMin = tan(-cTreadHingeAngleMin);

   // Init tread positions
   for(int i = 0; i < 2; i++)
   {
      mCurrentTreadSuspensionHeight[i] = 0.0f;
      mCurrentTreadHingeAngle[i] = 0.0f;
   }

   mCurrentLeftTreadScroll = 0.0f;
   mCurrentRightTreadScroll = 0.0f;
}

//============================================================================
//============================================================================
BPhysicsWolverineAction::~BPhysicsWolverineAction()
{

}

//============================================================================
//============================================================================
void BPhysicsWolverineAction::updateVisuals(float elapsedTime)
{
   BObject* pObj = mEntity->getObject();
   if (!pObj)
      return;
   BVisual* pVis = pObj->getVisual();
   if (!pVis)
      return;

   // calculate the angular velocity
   float currentYaw = pObj->getForward().getAngleAroundY();
   float angularVelocity = currentYaw - mPrevYaw;
   mPrevYaw = currentYaw;
   if (angularVelocity > cPi)
      angularVelocity = -(cPi + cPi - angularVelocity);
   else if (angularVelocity < -cPi)
      angularVelocity = cPi + cPi + angularVelocity;
   angularVelocity /= elapsedTime;

   float spinAngle, turnAngle, wheelHeight[2];
   getWheelData(angularVelocity, spinAngle, turnAngle, wheelHeight[0], wheelHeight[1]);
   mPrevYaw = pObj->getForward().getAngleAroundY();

   float treadHeight[2];
   float treadAngle[2];
   float leftTreadScroll, rightTreadScroll;

   getTreadData(treadHeight[0], treadHeight[1], treadAngle[0], treadAngle[1], leftTreadScroll, rightTreadScroll);
   pObj->setUV1Offset(0.0f, rightTreadScroll);
   pObj->setUV2Offset(0.0f, leftTreadScroll);

   // 0 - left
   // 1 - right
   static const char* gWheelNames[2]={"BoneRWheel", "BoneLWheel"};
   static const char* gTreadNames[2]={"BoneLTread", "BoneRTread"};
   for (uint i = 0; i < 2; i++)
   {
      BVisualItem* pWheelAttachment = pVis->getAttachmentByToBoneName(gWheelNames[i]);
      if (pWheelAttachment)
      {
         BMatrix mtx;// = pWheelAttachment->mTransform;

         if (i == 0)
         {
            mtx.makeRotateX(-spinAngle);
            mtx.multRotateY(turnAngle);
            mtx.setTranslation(0.0f, wheelHeight[i], 0.0f);
         }
         else
         {
            mtx.makeRotateX(spinAngle);
            mtx.multRotateY(turnAngle);
            mtx.setTranslation(0.0f, wheelHeight[i], 0.0f);
         }

         // Set transform
         pWheelAttachment->setTransform(mtx);
      }

      BVisualItem* pTreadAttachment = pVis->getAttachmentByToBoneName(gTreadNames[i]);
      if (pTreadAttachment)
      {
         BMatrix mtx;// = pTreadAttachment->mTransform;
         mtx.makeIdentity();

         // Suspension bouncing
         mtx.makeRotateZ(treadAngle[0]);
         mtx.setTranslation(0.0f, treadHeight[0], 0.0f);

         // Set transform
         pTreadAttachment->setTransform(mtx);
      }
   }
}

//============================================================================
//============================================================================
void BPhysicsWolverineAction::applyAction(const hkStepInfo& stepInfo)
{
//-- FIXING PREFIX BUG ID 3644
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BASSERT(pRB);
   
   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   //============================================================================
   // Update wheel spin for visual (based on linear velocity along forward vector)
   hkRotation chassisRotation;
   chassisRotation.set(pRB->getRotation());
   hkVector4 linVel = pRB->getLinearVelocity();
   float spinSpeed = static_cast<float>(linVel.dot3(chassisRotation.getColumn(2))) / (cTwoPi * cWheelRadius); // rotations / second
   mWheelSpinAngle += (spinSpeed * stepInfo.m_deltaTime * cSpinFactor);
   mWheelSpinAngle = fmod(mWheelSpinAngle, cTwoPi);

   // Get orientation and suspension direction (down)
   hkTransform trans;
   trans.set(pRB->getRotation(), pRB->getPosition());

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
   mCurrentLeftTreadScroll = WolverineWrapUV(mCurrentLeftTreadScroll);



   // Right
   treadPoint.setTransformedPos(trans, cTreadPoints[1]);

   pRB->getPointVelocity(treadPoint, chassis_velocity_at_treadPoint);
   treadForwardVel = trans.getColumn(2).dot3( chassis_velocity_at_treadPoint );

   mCurrentRightTreadScroll += treadForwardVel * sTreadVelocityScalar;
   mCurrentRightTreadScroll = WolverineWrapUV(mCurrentRightTreadScroll);

   //============================================================================
   // Base class apply
   BPhysicsGroundVehicleAction::applyAction(stepInfo);

   // Compute new desired tread params (angle and suspension)
   //
   int i;

   hkRotation rot;
   rot.set(pRB->getRotation());
   hkVector4 suspDir = -rot.getColumn(1);

   // Get hkpSampledHeightFieldShape for raycasting against
   BASSERT(gTerrainSimRep.getPhysicsHeightfield());
   const hkpEntity* pHeightfieldEntity = gTerrainSimRep.getPhysicsHeightfield()->getHKEntity();
   BASSERT(pHeightfieldEntity->getCollidable());
//-- FIXING PREFIX BUG ID 3645
   const hkpSampledHeightFieldShape* pHeightfield = (hkpSampledHeightFieldShape*) pHeightfieldEntity->getCollidable()->getShape();
//--
	const hkTransform& heightfieldTrans = pHeightfieldEntity->getCollidable()->getTransform();

   for(i = 0; i < 2; i++)
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
      input1.m_from.setTransformedInversePos(heightfieldTrans, startPt1);
      input1.m_to.setTransformedInversePos(heightfieldTrans, endPt1);
      input1.m_rayShapeCollectionFilter = NULL;
      hkpShapeRayCastOutput output1;
      pHeightfield->castRay(input1, output1);

      hkpShapeRayCastInput input2;
      input2.m_from.setTransformedInversePos(heightfieldTrans, startPt2);
      input2.m_to.setTransformedInversePos(heightfieldTrans, endPt2);
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
void BPhysicsWolverineAction::getTreadData(float& blTreadHeight, float& brTreadHeight, float& blTreadAngle, float& brTreadAngle, float& leftTreadScroll, float& rightTreadScroll) const
{
   static bool current = true;
   if(current)
   {
      blTreadHeight = mCurrentTreadSuspensionHeight[0];
      brTreadHeight = mCurrentTreadSuspensionHeight[1];

      blTreadAngle = mCurrentTreadHingeAngle[0];
      brTreadAngle = mCurrentTreadHingeAngle[1];
   }
   else
   {
      blTreadHeight = mDesiredTreadSuspensionHeight[0];
      brTreadHeight = mDesiredTreadSuspensionHeight[1];

      blTreadAngle = mDesiredTreadHingeAngle[0];
      brTreadAngle = mDesiredTreadHingeAngle[1];
   }

   leftTreadScroll = mCurrentLeftTreadScroll;
   rightTreadScroll = mCurrentRightTreadScroll;
}

//============================================================================
//============================================================================
void BPhysicsWolverineAction::getWheelData(float angularVelocity, float& spinAngle, float& turnAngle, float& flWheelHeight, float& frWheelHeight) const
{
//-- FIXING PREFIX BUG ID 3646
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BASSERT(pRB);
   pRB;

   // Rotation angle (0 to 2Pi)
   spinAngle = mWheelSpinAngle;

   // Percent (-1.0f to 1.0f) turning left to right
   turnAngle = angularVelocity / cPi;
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
}

//============================================================================
//============================================================================
#ifndef BUILD_FINAL
   void BPhysicsWolverineAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      BPhysicsGroundVehicleAction::debugRender();

      // Re-enable this when needed - will need to cache the hard points, contact points, and end points
      // in the apply action - I don't want to take up the memory right now
      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
//-- FIXING PREFIX BUG ID 3647
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

         hkVector4 collisionPt;

         for(int i = 0; i < 2; i++)
         {
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
   }
#endif
