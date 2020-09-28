//============================================================================
// File: physicsghostaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Physics/Dynamics/Entity/hkpRigidBody.h"

class BEntity;
class BUnitActionMoveGhost;
class BSquadActionMove;

//============================================================================
// BPhysicsGhostction
// Simulates physics for ghosts
//============================================================================
class BPhysicsGhostAction: public hkpUnaryAction
{
   public:

      BPhysicsGhostAction(hkpRigidBody* body, BEntity* pEntity);

      /// no need to clone as we don't use clone func in this demo
      virtual hkpAction* clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

      void              enableUpStability(bool enable) { mReorient = enable; }
      bool              isUpStabilityEnabled() const { return mReorient; }

      #ifndef BUILD_FINAL
         void           debugRender();
      #endif

   private:

      virtual void      applyAction(const hkStepInfo& stepInfo);
      void              calcMovement(const hkStepInfo& stepInfo, float& fwdAccel, float& rightAccel, float& turning, bool& atGoal, BSquadActionMove* pSMA);
      void              reorient(const hkStepInfo& stepInfo);

      BVector           mDesiredPos;
      BEntity*          mEntity;
      float             mCurrentSuspensionLengths[4];
      float             mPrevTurning;

      #ifndef BUILD_FINAL
         float             mSuspensionDist[4];
      #endif

      bool              mReorient : 1;
};
