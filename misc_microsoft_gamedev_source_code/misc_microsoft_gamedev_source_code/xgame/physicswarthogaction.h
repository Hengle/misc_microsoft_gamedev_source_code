//============================================================================
// File: physicswarthogaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Physics/Dynamics/Entity/hkpRigidBody.h"

class BEntity;
class BUnitActionMoveWarthog;
class BSquadActionMove;
class BTerrainTrailDecalGenerator;

//============================================================================
// BPhysicsWarthogAction
// Simulates physics for warthogs - including a four spring suspension,
// extra gravity, overturn correction, friction, etc.
// Some of the code in applyAction comes from
// hkpVehicleInstance::applyAction and hkpVehicleDefaultSuspension::calcSuspension
//============================================================================
class BPhysicsWarthogAction: public hkpUnaryAction
{
   public:

      BPhysicsWarthogAction(hkpRigidBody* body, BEntity* pEntity, bool chopper);
      ~BPhysicsWarthogAction();

      /// no need to clone as we don't use clone func in this demo
      virtual hkpAction* clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

      void              getWheelData(float& spinAngle, float& turnAngle, float& flWheelHeight, float& frWheelHeight, float& blWheelHeight, float& brWheelHeight) const;
      void              enableUpStability(bool enable) { mReorient = enable; }
      bool              isUpStabilityEnabled() const { return mReorient; }
      #ifndef BUILD_FINAL
         void           debugRender();
      #endif

      void              updateWarthogVisuals();
      void              updateChopperVisuals();
      void              spawnPhysicsEventVisuals(long terrainEffectsHandle);

      bool              getFlagInAir() const { return mFlagInAir; }
      void              setFlagInAir(bool v) { mFlagInAir = v; }
      bool              getFlagSkidding() const { return mFlagSkidding; }
      void              setFlagSkidding(bool v) { mFlagSkidding = v; }
      void              getPrevLinearVelocity(BVector &vec) { vec = mPrevLinearVelocity; }
      bool              getFlagChopper() const { return mFlagChopper; }
      void              setFlagChopper(bool v) { mFlagChopper = v; }


   private:

      virtual void      applyAction(const hkStepInfo& stepInfo);
      void              calcMovement(const hkStepInfo& stepInfo, float& acceleration, float& turning, bool& atGoal, BSquadActionMove* pSMA);
      void              calcMovement(const hkStepInfo& stepInfo, float& acceleration, float& turning, bool& atGoal, BUnitActionMoveWarthog* pWMA);
      void              reorient(const hkStepInfo& stepInfo);

      BTerrainTrailDecalGenerator*   mTrailDecal[2];
      BVector           mWheelContactPts[4];
      float             mCurrLinVelocityLen;

      BVector           mDesiredPos;
      BEntity*          mEntity;
      float             mCurrentSuspensionLengths[4];
      
      float             mLastDistance;
      float             mTimeOverturned;
      float             mWheelSpinAngle;
      float             mPrevTurning;
      BVector           mPrevLinearVelocity;


      #ifndef BUILD_FINAL
         float             mSuspensionDist[4];
      #endif

      bool              mReorient : 1;
      bool              mStayOnPath : 1;
      bool              mFlagInAir : 1;
      bool              mFlagSkidding : 1;
      bool              mFlagChopper : 1;
      bool              mCanPivotToTarget : 1;

};
