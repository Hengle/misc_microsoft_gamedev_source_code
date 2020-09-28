//============================================================================
// File: physicshoverflightaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Physics/Dynamics/Entity/hkpRigidBody.h"

class BEntity;
class BSquadActionMove;

//============================================================================
// BPhysicsHoverFlightAction
// Simulates physics for units that fly and hover
//============================================================================
class BPhysicsHoverFlightAction: public hkpUnaryAction
{
   public:

      BPhysicsHoverFlightAction(hkpRigidBody* body, BEntity* pEntity);

      /// no need to clone as we don't use clone func in this demo
      virtual hkpAction* clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

      void              enableUpStability(bool enable) { mReorient = enable; }
      bool              isUpStabilityEnabled() const { return mReorient; }

   private:

      virtual void      applyAction(const hkStepInfo& stepInfo);
      void              calcMovement(const hkStepInfo& stepInfo, float& fwdAccel, float& rightAccel, float& yaw, float& pitch, float& roll, bool& atGoal, BSquadActionMove* pSMA);
      void              reorient(const hkStepInfo& stepInfo);

      BVector           mDesiredPos;
      float             mLookAheadHeight[5];
      float             mCurrentSuspensionLengths[4];
      float             mPrevTurning;
      float             mVertAvoidOffset;
      float             mHoverAltitudeOffset;
      float             mTimeSinceStopped;
      BEntity*          mEntity;
      uint8             mLookAheadIndex;
      bool              mReorient : 1;
};
