//============================================================================
// File: physicsscorpionaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "physicsgroundvehicleaction.h"

//============================================================================
// BPhysicsScorpionction
// Simulates physics for tanks
//============================================================================
class BPhysicsScorpionAction : public BPhysicsGroundVehicleAction
{
   public:

      BPhysicsScorpionAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo);
      virtual ~BPhysicsScorpionAction();

      void              updateVisuals(const char* treadNames[4]);

      #ifndef BUILD_FINAL
         virtual void   debugRender();
      #endif

   protected:

      virtual void      applyAction(const hkStepInfo& stepInfo);
      void              getTreadData(float& flTreadHeight, float& frTreadHeight, float& blTreadHeight, float& brTreadHeight,
                                                float& flTreadAngle, float& frTreadAngle, float& blTreadAngle, float& brTreadAngle,
                                                float& leftTreadScroll, float& rightTreadScroll) const;

      float             mCurrentTreadHingeAngle[4];
      float             mDesiredTreadHingeAngle[4];
      float             mCurrentTreadSuspensionHeight[4];
      float             mDesiredTreadSuspensionHeight[4];

      float             mCurrentLeftTreadScroll;
      float             mCurrentRightTreadScroll;

      float             mTreadTangentHingeAngleMax;
      float             mTreadTangentHingeAngleMin;

      #ifndef BUILD_FINAL
         float          mTreadCollisionDist1[4];
         float          mTreadCollisionDist2[4];
      #endif
};
