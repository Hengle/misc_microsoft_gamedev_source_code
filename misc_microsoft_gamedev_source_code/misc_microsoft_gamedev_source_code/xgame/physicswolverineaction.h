//============================================================================
// File: physicswolverineaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "physicsgroundvehicleaction.h"

//============================================================================
// BPhysicsWolverinection
// Simulates physics for the wolverine
//============================================================================
class BPhysicsWolverineAction: public BPhysicsGroundVehicleAction
{
   public:

      BPhysicsWolverineAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo);
      virtual ~BPhysicsWolverineAction();

      virtual void      updateVisuals(float elapsedTime);

      #ifndef BUILD_FINAL
         virtual void   debugRender();
      #endif

   protected:

      virtual void      applyAction(const hkStepInfo& stepInfo);

      void              getTreadData(float& blTreadHeight, float& brTreadHeight, float& blTreadAngle, float& brTreadAngle, float& leftTreadScroll, float& rightTreadScroll) const;
      void              getWheelData(float angularVelocity, float& spinAngle, float& turnAngle, float& flWheelHeight, float& frWheelHeight) const;

      float             mWheelSpinAngle;

      float             mCurrentTreadHingeAngle[2];
      float             mDesiredTreadHingeAngle[2];
      float             mCurrentTreadSuspensionHeight[2];
      float             mDesiredTreadSuspensionHeight[2];

      float             mCurrentLeftTreadScroll;
      float             mCurrentRightTreadScroll;

      float             mTreadTangentHingeAngleMax;
      float             mTreadTangentHingeAngleMin;

      float             mPrevYaw;

      #ifndef BUILD_FINAL
         float          mTreadCollisionDist1[2];
         float          mTreadCollisionDist2[2];
      #endif
};
