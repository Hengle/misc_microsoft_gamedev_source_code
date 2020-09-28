//============================================================================
// File: physicscobraaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "physicsgroundvehicleaction.h"

//============================================================================
// BPhysicsCobraction
// Simulates physics for the cobra
//============================================================================
class BPhysicsCobraAction: public BPhysicsGroundVehicleAction
{
   public:

      BPhysicsCobraAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo);
      virtual ~BPhysicsCobraAction();

      virtual void      updateVisuals(float elapsedTime);
      void              setFlagGremlin(bool v) { mFlagGremlin = v; }

   protected:

      virtual void      applyAction(const hkStepInfo& stepInfo);

      float             mWheelSpinAngle;
      float             mPrevYaw;
      bool              mFlagGremlin:1;
};
