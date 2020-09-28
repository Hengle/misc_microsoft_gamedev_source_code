//============================================================================
// File: physicscobraaction.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "common.h"
#include "physicscobraaction.h"
#include "object.h"
#include "visual.h"

// Wheel visual constants
const float cSpringLength = 1.3f;
const float cMaxWheelTurn = cPiOver2 * 0.5f;
const float cSpringStableLength = 1.0f;
const float cWheelHeightMin = -1.0f;
const float cWheelHeightMax = 0.3f;
const float cSpinFactor = 2.0f;
const float cWheelRadius = 1.0f;

//============================================================================
//============================================================================
BPhysicsCobraAction::BPhysicsCobraAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo) :
   BPhysicsGroundVehicleAction(body, pEntity, pInfo),
   mWheelSpinAngle(0.0f),
   mPrevYaw(0.0f),
   mFlagGremlin(false)
{

}

//============================================================================
//============================================================================
BPhysicsCobraAction::~BPhysicsCobraAction()
{

}

//============================================================================
//============================================================================
void BPhysicsCobraAction::updateVisuals(float elapsedTime)
{
   BObject* pObj = mEntity->getObject();
   if (!pObj)
      return;
   BVisual* pVis = pObj->getVisual();
   if (!pVis)
      return;

   // Rotation angle (0 to 2Pi)
   float spinAngle = mWheelSpinAngle;

   // calculate the angular velocity
   float currentYaw = pObj->getForward().getAngleAroundY();
   float angularVelocity = currentYaw - mPrevYaw;
   mPrevYaw = currentYaw;
   if (angularVelocity > cPi)
      angularVelocity = -(cPi + cPi - angularVelocity);
   else if (angularVelocity < -cPi)
      angularVelocity = cPi + cPi + angularVelocity;
   angularVelocity /= elapsedTime;

   // Percent (-1.0f to 1.0f) turning left to right
   float turnAngle = angularVelocity / cPi;
   turnAngle = Math::Clamp(turnAngle, -1.0f, 1.0f) * cMaxWheelTurn;

   //float spinAngle, turnAngle, wheelHeight[6];
   //pPCA->getWheelData(spinAngle, turnAngle, wheelHeight[5], wheelHeight[2], wheelHeight[4], wheelHeight[1], wheelHeight[3], wheelHeight[0]);

   static const char* gWheelNames[6]={"BoneWheel01", "BoneWheel02", "BoneWheel03", "BoneWheel04", "BoneWheel05", "BoneWheel06"};
   static const char* gGremlinWheelNames[6]={"BoneWheel04", "BoneWheel06", "BoneWheel02", "BoneWheel03", "BoneWheel05", "BoneWheel01"};

   // Get proper ordered set of wheel names
   const char** wheelNames;
   if (mFlagGremlin)
      wheelNames = gGremlinWheelNames;
   else
      wheelNames = gWheelNames;

   for (uint i = 0; i < 6; i++)
   {
      BVisualItem* pWheelAttachment = pVis->getAttachmentByToBoneName(wheelNames[i]);
      if (pWheelAttachment)
      {
         // Calc wheel height
         // Percent (-1.0f to 1.0f) of min / max wheel height
         float wheelHeight;
         if (mCurrentSuspensionLengths[i] > cSpringStableLength)
            wheelHeight = Math::Clamp(((mCurrentSuspensionLengths[i] - cSpringStableLength) / (cSpringLength - cSpringStableLength)), 0.0f, 1.0f) * cWheelHeightMin;
         else
            wheelHeight = Math::Clamp(((cSpringStableLength - mCurrentSuspensionLengths[i]) / cSpringStableLength), 0.0f, 1.0f) * cWheelHeightMax;

         BMatrix mtx;// = pWheelAttachment->mTransform;

         // Spin
         if (i <= 2)
            mtx.makeRotateX(spinAngle);
         else  // reverse rotation direction of left tires since they're rotated around 180 degrees
            mtx.makeRotateX(-spinAngle);

         // Turning
         if (i == 2)
            mtx.multRotateY(turnAngle);
         else if (i == 5)
         {
            if (mFlagGremlin)
               mtx.multRotateY(turnAngle);
            else
               mtx.multRotateY(-turnAngle);
         }

         // Suspension bouncing
         if (mFlagGremlin || (i <= 2))
            mtx.setTranslation(0.0f, wheelHeight, 0.0f);
         else
            mtx.setTranslation(0.0f, -wheelHeight, 0.0f);

         // Set transform
         pWheelAttachment->setTransform(mtx);
      }
   }
}

//============================================================================
//============================================================================
void BPhysicsCobraAction::applyAction(const hkStepInfo& stepInfo)
{
   // Get current chassis rotation / vel
//-- FIXING PREFIX BUG ID 3628
   const hkpRigidBody* pRB = static_cast<hkpRigidBody*>(m_entity);
//--
   BASSERT(pRB);
   hkRotation chassisRotation;
   chassisRotation.set(pRB->getRotation());
   hkVector4 linVel = pRB->getLinearVelocity();

   // Calculate wheel spin angle
   float spinSpeed = static_cast<float>(linVel.dot3(chassisRotation.getColumn(2))) / (cTwoPi * cWheelRadius); // rotations / second
   mWheelSpinAngle += (spinSpeed * stepInfo.m_deltaTime * cSpinFactor);
   mWheelSpinAngle = fmod(mWheelSpinAngle, cTwoPi);

   // Base class apply
   BPhysicsGroundVehicleAction::applyAction(stepInfo);
}
