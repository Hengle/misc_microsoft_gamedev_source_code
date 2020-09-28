//============================================================================
// File: physicsgrizzlyaction.h
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
class BTerrainTrailDecalGenerator;

//============================================================================
// BPhysicsGrizzlyction
// Simulates physics for grizzly tanks
//============================================================================
class BPhysicsGrizzlyAction: public hkpUnaryAction
{
   public:

      BPhysicsGrizzlyAction(hkpRigidBody* body, BEntity* pEntity);
      ~BPhysicsGrizzlyAction();

      /// no need to clone as we don't use clone func in this demo
      virtual hkpAction* clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

      void              getTreadData(float& flTreadHeight, float& frTreadHeight, float& blTreadHeight, float& brTreadHeight,
                                       float& flTreadAngle, float& frTreadAngle, float& blTreadAngle, float& brTreadAngle,
                                       float& leftTreadScroll, float& rightTreadScroll) const;
      void              enableUpStability(bool enable) { mReorient = enable; }
      bool              isUpStabilityEnabled() const { return mReorient; }

      #ifndef BUILD_FINAL
         void           debugRender();
      #endif

      void              spawnPhysicsEventVisuals(long terrainEffectsHandle);

   private:

      virtual void      applyAction(const hkStepInfo& stepInfo);
      void              calcMovement(float& acceleration, float& turning, bool& atGoal, BSquadActionMove* pSMA);
      void              reorient(const hkStepInfo& stepInfo);

      BTerrainTrailDecalGenerator*   mTrailDecal;
      BVector           mWheelContactPts[4];

      BVector           mDesiredPos;
      BEntity*          mEntity;
      float             mCurrentSuspensionLengths[4];
      
      float             mCurrentTreadHingeAngle[4];
      float             mDesiredTreadHingeAngle[4];
      float             mCurrentTreadSuspensionHeight[4];
      float             mDesiredTreadSuspensionHeight[4];

      float             mCurrentLeftTreadScroll;
      float             mCurrentRightTreadScroll;

      float             mTreadTangentHingeAngleMax;
      float             mTreadTangentHingeAngleMin;

      float             mPrevTurning;
      BVector           mPrevLinearVelocity;

      #ifndef BUILD_FINAL
         float             mSuspensionDist[4];
         float             mTreadCollisionDist1[4];
         float             mTreadCollisionDist2[4];
      #endif
         
      bool              mReorient : 1;
      bool              mStayOnPath : 1;
};
