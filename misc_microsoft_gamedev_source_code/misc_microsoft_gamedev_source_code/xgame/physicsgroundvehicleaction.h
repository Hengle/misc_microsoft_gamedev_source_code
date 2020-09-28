//============================================================================
// File: physicsgroundVehicleaction.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#pragma once

//============================================================================
// Includes
#include "Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Physics/Dynamics/Entity/hkpRigidBody.h"
#include "containers\staticArray.h"
#include "entity.h"

class BEntity;
class BSquad;
class BTerrainTrailDecalGenerator;
class BPhysicsInfo;
class BPhysicsVehicleInfo;

//============================================================================
// BPhysicsGroundVehicleAction
// Simulates physics for the groundVehicle
//============================================================================
class BPhysicsGroundVehicleAction : public hkpUnaryAction
{
   public:

      BPhysicsGroundVehicleAction(hkpRigidBody* body, BEntity* pEntity, const BPhysicsInfo* pInfo);
      virtual ~BPhysicsGroundVehicleAction();

      /// no need to clone as we don't use clone func in this demo
      virtual hkpAction*            clone( const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms ) const { return HK_NULL; }

      virtual void                  updateVisuals(float elapsedTime) {};
      virtual void                  spawnPhysicsEventVisuals(long terrainEffectsHandle);

      BPhysicsVehicleInfo*          getVehicleInfo() const { return mpVehicleData; }
      void                          setVehicleInfo(BPhysicsVehicleInfo* newData);

      BEntityID                     getSimEntityID() const { if (mEntity) return mEntity->getID(); else return cInvalidObjectID; }
      void                          mainThreadTerrainCollisionResolution();

      // Debug funcs
      #ifndef BUILD_FINAL
         virtual void               debugRender();
      #endif

   protected:

      virtual void                  applyAction(const hkStepInfo& stepInfo);
      void                          calcTurning(float& turning, BSquad* pParentSquad);
      void                          reorient(const hkStepInfo& stepInfo, const hkVector4 rotationAxis, float strength, float damping);


      BStaticArray<BVector>         mWheelContactPts;
      BStaticArray<float>           mCurrentSuspensionLengths;
      BVector                       mPrevLinearVelocity;
      BTerrainTrailDecalGenerator*  mTrailDecal;
      BEntity*                      mEntity;
      BPhysicsVehicleInfo*          mpVehicleData;
      float                         mDistBelowTerrain;
      bool                          mTerrainCollision:1;

      #ifndef BUILD_FINAL
         BStaticArray<float>        mSuspensionDist;
      #endif
};
