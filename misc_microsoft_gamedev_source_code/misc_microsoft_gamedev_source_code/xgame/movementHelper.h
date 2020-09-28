//==============================================================================
// movmenethelper.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once 

//==============================================================================
//Forward Declarations
//==============================================================================
class BEntity;
class BProjectile;
class BSquad;
class BUnit;

//==============================================================================
// BMovementHelper
//==============================================================================
class BMovementHelper
{
   public:

      BMovementHelper();
      ~BMovementHelper();

      //bool updateTargetLocation(BEntity* pEntity, bool bAllowEndMove);
      //void doUnitMovement(BUnit* pUnit, float elapsedTime, bool orienControl, float speedScale);
      //void doSquadMovement(BSquad* pSquad, float elapsedTime, float speedScale);

      void doProjectileMovement(BProjectile* pEntity, BVector perturbance, float elapsedTime, bool isAffectedByGravity, bool isTracking, float gravity, float turnRate, BVector& facingVector, float speedScale);

      //BVector calculateDirection(BEntity* pEntity);
      //BVector calculateVelocity(BEntity* pEntity, float elapsedTime, float speedScale, BVector dir, bool orienControl);
      //bool findCircleCenter(BVector& center, BVector& p1, BVector& p2, BVector& p3);
      //float distanceToWaypoint(BEntity* pEntity);

   private:
      void updatePosition(BEntity* pEntity, BVector stepVelocity);
};