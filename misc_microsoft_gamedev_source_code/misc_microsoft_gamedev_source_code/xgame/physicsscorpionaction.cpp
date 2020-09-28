//============================================================================
// File: physicsscorpionaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicsscorpionaction.h"
#include "object.h"
#include "visual.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "terrainphysicsheightfield.h"
#include "configsgame.h"
#include "physicsInfo.h"

//============================================================================
// Constants
const float cHardpointY = -0.05f;
const hkVector4 cTreadPoints[2] = 
{
   hkVector4(-6.0f, cHardpointY, 0.0f, 0.0f),
   hkVector4( 6.0f, cHardpointY, 0.0f, 0.0f),
};

const float cTreadHingeAngleMax = cPiOver4 / 3.0f;
const float cTreadHingeAngleMin = -cPiOver4 / 3.0f;
const float cTreadHingeAngleAdjustmentRate = cPiOver4 * 1.0f;
const float cTreadSuspensionHeightMax = 0.7f;
const float cTreadSuspensionHeightMin = -0.7f;
const float cTreadSuspensionHeightAdjustmentRate = 8.0f;

float WrapUV(float value)
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
BPhysicsScorpionAction::BPhysicsScorpionAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo) :
   BPhysicsGroundVehicleAction(body, pEntity, pInfo)
{
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
}

//============================================================================
//============================================================================
BPhysicsScorpionAction::~BPhysicsScorpionAction()
{

}

//============================================================================
//============================================================================
void BPhysicsScorpionAction::updateVisuals(const char* treadNames[4])
{
   BObject* pObj = mEntity->getObject();
   if (!pObj)
      return;
   BVisual* pVis = pObj->getVisual();
   if (!pVis)
      return;

   float flTreadHeight, frTreadHeight, blTreadHeight, brTreadHeight;
   float flTreadAngle, frTreadAngle, blTreadAngle, brTreadAngle;
   float leftTreadScroll, rightTreadScroll;

   getTreadData(flTreadHeight, frTreadHeight, blTreadHeight, brTreadHeight,
      flTreadAngle, frTreadAngle, blTreadAngle, brTreadAngle,
      leftTreadScroll, rightTreadScroll);

   pObj->setUV1Offset(0.0f, rightTreadScroll);
   pObj->setUV2Offset(0.0f, leftTreadScroll);

   // 1 - Back right
   // 2 - Back left
   // 3 - Front right
   // 4 - Front left
   //static const char* gTreadNames[4]={"bone tread br", "bone tread bl", "bone tread fr", "bone tread fl" };
   for (uint i = 0; i < 4; i++)
   {
      BVisualItem* pTreadAttachment = pVis->getAttachmentByToBoneName(treadNames[i]);
      if (pTreadAttachment)
      {
         BMatrix mtx;// = pTreadAttachment->mTransform;
         mtx.makeIdentity();

         // Suspension bouncing
         if (i == 0)
         {
            mtx.multRotateX(brTreadAngle);
            mtx.setTranslation(0.0f, brTreadHeight, 0.0f);
         }
         else if (i == 1)
         {
            mtx.multRotateX(blTreadAngle);
            mtx.setTranslation(0.0f, blTreadHeight, 0.0f);
         }
         else if (i == 2)
         {
            mtx.multRotateX(frTreadAngle);
            mtx.setTranslation(0.0f, frTreadHeight, 0.0f);
         }
         else if (i == 3)
         {
            mtx.multRotateX(flTreadAngle);
            mtx.setTranslation(0.0f, flTreadHeight, 0.0f);
         }

         // Set transform
         pTreadAttachment->setTransform(mtx);

         // Update wheel spin for wheels attached to tread
         if (mpVehicleData->mTreadWheelAttachments)
         {
            for (int j = 0; j < pTreadAttachment->getNumberAttachments(); j++)
            {
               BVisualItem* pWheelAttachment = pTreadAttachment->getAttachmentByIndex(j);
               if (pWheelAttachment)
               {
                  BMatrix mtx;// = pTreadAttachment->mTransform;

                  mtx.makeRotateX(rightTreadScroll);

                  // Set transform
                  pWheelAttachment->setTransform(mtx);
               }
            }
         }
      }
   }
}

//============================================================================
//============================================================================
void BPhysicsScorpionAction::applyAction(const hkStepInfo& stepInfo)
{
//-- FIXING PREFIX BUG ID 3576
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BASSERT(pRB);

   // Early out if keyframed
   if (pRB->getMotionType() == hkpMotion::MOTION_KEYFRAMED)
      return;

   // Get orientation and suspension direction (down)
   hkTransform trans;
   trans.set(pRB->getRotation(), pRB->getPosition());

   //============================================================================
   // Calc left/right tread scrolling
   hkVector4 treadPoint;
   hkVector4 chassis_velocity_at_treadPoint;
   hkReal treadForwardVel;

   float sTreadVelocityScalar = mpVehicleData->mTreadVelocityScalar;

   // Left
   treadPoint.setTransformedPos(trans, cTreadPoints[0]);

   pRB->getPointVelocity(treadPoint, chassis_velocity_at_treadPoint);
   treadForwardVel = trans.getColumn(2).dot3( chassis_velocity_at_treadPoint );

   mCurrentLeftTreadScroll += treadForwardVel * sTreadVelocityScalar;
   mCurrentLeftTreadScroll = WrapUV(mCurrentLeftTreadScroll);



   // Right
   treadPoint.setTransformedPos(trans, cTreadPoints[1]);

   pRB->getPointVelocity(treadPoint, chassis_velocity_at_treadPoint);
   treadForwardVel = trans.getColumn(2).dot3( chassis_velocity_at_treadPoint );

   mCurrentRightTreadScroll += treadForwardVel * sTreadVelocityScalar;
   mCurrentRightTreadScroll = WrapUV(mCurrentRightTreadScroll);


   //============================================================================
   // Base class apply
   BPhysicsGroundVehicleAction::applyAction(stepInfo);


   //============================================================================
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
//-- FIXING PREFIX BUG ID 3577
   const hkpSampledHeightFieldShape* pHeightfield = (hkpSampledHeightFieldShape*) pHeightfieldEntity->getCollidable()->getShape();
//--
	const hkTransform& heightfieldTrans = pHeightfieldEntity->getCollidable()->getTransform();

   // Get vehicle tread data
   uint numTreadHardpoints = static_cast<uint>(mpVehicleData->mTreadHardpoints.getNumber());
   BASSERT(numTreadHardpoints == 4);
//-- FIXING PREFIX BUG ID 3578
   const hkVector4* cTreadHardpoints1 = reinterpret_cast<hkVector4*>(mpVehicleData->mTreadHardpoints.getData());
//--
   float cTreadHardPointHeight = mpVehicleData->mTreadHardPointHeight;
   float cTreadRayLength = mpVehicleData->mTreadRayLength;
   uint numZOffsets = mpVehicleData->mTreadHardpointZOffsets.getNumber();
   BASSERT(numZOffsets == numTreadHardpoints);
   float* cTreadHardpointZOffsets = mpVehicleData->mTreadHardpointZOffsets.getData();

   for(i = 0; i < 4; i++)
   {
      hkVector4 startPt1, startPt2;
      hkVector4 endPt1, endPt2;
      float contactDist1, contactDist2;

      // Compute start and end points of the casting rays
      startPt1.setTransformedPos(trans, cTreadHardpoints1[i]);
      startPt1 = startPt1 + suspDir * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
      endPt1 = startPt1 + suspDir * cTreadRayLength;

      hkVector4 zOffset(0.0f, 0.0f, cTreadHardpointZOffsets[i]);
      startPt2.setTransformedPos(trans, cTreadHardpoints1[i] + zOffset);
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
      float treadRaySeparationDistance = fabs(cTreadHardpointZOffsets[i]);
      contactDist2 = Math::Clamp(contactDist2, cTreadSuspensionHeightMin - (treadRaySeparationDistance * mTreadTangentHingeAngleMin), cTreadSuspensionHeightMax + (treadRaySeparationDistance * mTreadTangentHingeAngleMax));


      #ifndef BUILD_FINAL
         mTreadCollisionDist1[i] = contactDist1;
         mTreadCollisionDist2[i] = contactDist2;
      #endif

      // Compute angle (theta) for line that crosses through both contact points
      float diff = contactDist2 - contactDist1;
      float theta = atanf(diff / treadRaySeparationDistance);
      
      float desiredHingeAngle = 0.0f;
      float desiredSuspensionHeight = 0.0f;

      
      // Clamp theta
      if((theta < cTreadHingeAngleMin) || (theta > cTreadHingeAngleMax))
      {
         desiredHingeAngle = Math::Clamp(theta, cTreadHingeAngleMin, cTreadHingeAngleMax);

         if(theta > 0)
         {
            desiredSuspensionHeight = contactDist2 - (treadRaySeparationDistance * mTreadTangentHingeAngleMax);
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
void BPhysicsScorpionAction::getTreadData(float& flTreadHeight, float& frTreadHeight, float& blTreadHeight, float& brTreadHeight,
                                          float& flTreadAngle, float& frTreadAngle, float& blTreadAngle, float& brTreadAngle,
                                          float& leftTreadScroll, float& rightTreadScroll) const
{
   float* cTreadHardpointZOffsets = mpVehicleData->mTreadHardpointZOffsets.getData();
   float angleDir[4];
   angleDir[0] = cTreadHardpointZOffsets[0] > 0.0f ? -1.0f : 1.0f;
   angleDir[1] = cTreadHardpointZOffsets[1] > 0.0f ? -1.0f : 1.0f;
   angleDir[2] = cTreadHardpointZOffsets[2] > 0.0f ? -1.0f : 1.0f;
   angleDir[3] = cTreadHardpointZOffsets[3] > 0.0f ? -1.0f : 1.0f;

   static bool current = true;
   if(current)
   {
      flTreadHeight = mCurrentTreadSuspensionHeight[0];
      frTreadHeight = mCurrentTreadSuspensionHeight[1];
      blTreadHeight = mCurrentTreadSuspensionHeight[2];
      brTreadHeight = mCurrentTreadSuspensionHeight[3];

      flTreadAngle = mCurrentTreadHingeAngle[0] * angleDir[0];
      frTreadAngle = mCurrentTreadHingeAngle[1] * angleDir[1];
      blTreadAngle = mCurrentTreadHingeAngle[2] * angleDir[2];
      brTreadAngle = mCurrentTreadHingeAngle[3] * angleDir[3];
   }
   else
   {
      flTreadHeight = mDesiredTreadSuspensionHeight[0];
      frTreadHeight = mDesiredTreadSuspensionHeight[1];
      blTreadHeight = mDesiredTreadSuspensionHeight[2];
      brTreadHeight = mDesiredTreadSuspensionHeight[3];

      flTreadAngle = mDesiredTreadHingeAngle[0] * angleDir[0];
      frTreadAngle = mDesiredTreadHingeAngle[1] * angleDir[1];
      blTreadAngle = mDesiredTreadHingeAngle[2] * angleDir[2];
      brTreadAngle = mDesiredTreadHingeAngle[3] * angleDir[3];
   }

   leftTreadScroll = mCurrentLeftTreadScroll;
   rightTreadScroll = mCurrentRightTreadScroll;
}

//============================================================================
// Debug funcs
//============================================================================
#ifndef BUILD_FINAL
   void BPhysicsScorpionAction::debugRender()
   {
      ASSERT_THREAD(cThreadIndexSim);

      BPhysicsGroundVehicleAction::debugRender();

      // Re-enable this when needed - will need to cache the hard points, contact points, and end points
      // in the apply action - I don't want to take up the memory right now
      if (gConfig.isDefined(cConfigDebugRenderShape))
      {
//-- FIXING PREFIX BUG ID 3581
         const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--

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

         // Get vehicle tread data
         uint numTreadHardpoints = static_cast<uint>(mpVehicleData->mTreadHardpoints.getNumber());
         BASSERT(numTreadHardpoints == 4);
//-- FIXING PREFIX BUG ID 3582
         const hkVector4* cTreadHardpoints1 = reinterpret_cast<hkVector4*>(mpVehicleData->mTreadHardpoints.getData());
//--
         float cTreadHardPointHeight = mpVehicleData->mTreadHardPointHeight;
         float cTreadRayLength = mpVehicleData->mTreadRayLength;
         uint numZOffsets = mpVehicleData->mTreadHardpointZOffsets.getNumber();
         BASSERT(numZOffsets == numTreadHardpoints);
         float* cTreadHardpointZOffsets = mpVehicleData->mTreadHardpointZOffsets.getData();

         for(int i = 0; i < 4; i++)
         {
            // Compute start and end points of the casting rays
            startPt1.setTransformedPos(trans, cTreadHardpoints1[i]);
            startPt1 = startPt1 + rigidBodyUp * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
            endPt1 = startPt1 + rigidBodyUp * cTreadRayLength;

            hkVector4 zOffset(0.0f, 0.0f, cTreadHardpointZOffsets[i]);
            startPt2.setTransformedPos(trans, cTreadHardpoints1[i] + zOffset);
            startPt2 = startPt2 + rigidBodyUp * (cTreadHardPointHeight - (cTreadRayLength / 2.0f));
            endPt2 = startPt2 + rigidBodyUp * cTreadRayLength;

            gpDebugPrimitives->addDebugLine((BVector)startPt1, (BVector)endPt1, 0xffffff00, 0xffffff00);
            gpDebugPrimitives->addDebugLine((BVector)startPt2, (BVector)endPt2, 0xffffff00, 0xffffff00);


            
            collisionPt.setTransformedPos(trans, cTreadHardpoints1[i]);
            collisionPt = collisionPt + rigidBodyUp * cTreadHardPointHeight;
            collisionPt = collisionPt - rigidBodyUp * mTreadCollisionDist1[i];
            boxMat.setTranslation((BVector)collisionPt);
            gpDebugPrimitives->addDebugBox(boxMat, 0.25f, 0xffff0000);
            
            collisionPt.setTransformedPos(trans, cTreadHardpoints1[i] + zOffset);
            collisionPt = collisionPt + rigidBodyUp * cTreadHardPointHeight;
            collisionPt = collisionPt - rigidBodyUp * mTreadCollisionDist2[i];
            boxMat.setTranslation((BVector)collisionPt);
            gpDebugPrimitives->addDebugBox(boxMat, 0.25f, 0xffff0000);
         }
      }
   }
#endif
