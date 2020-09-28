//==============================================================================
// unitactionslaveturretattack.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;


//==============================================================================
//==============================================================================
class BUnitActionSlaveTurretAttack : public BAction
{
   public:
      BUnitActionSlaveTurretAttack() { }
      virtual ~BUnitActionSlaveTurretAttack() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      //ParentAction.
      virtual BAction*           getParentAction() const { return(mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction = v; }

      //Add block pool
      DECLARE_FREELIST(BUnitActionSlaveTurretAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       canHitTarget();
      bool                       isBlocked() const;
      bool                       grabControllers();
      void                       releaseControllers();
      bool                       validateControllers() const;
      bool                       validateTarget() const;
      //bool                       validateRange() const;
      bool                       validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const;
      void                       doProjectileAttack(long attachmentHandle, long boneHandle);
      void                       getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector & position, BVector & forward, BVector & right) const;
      void                       getTargetPosition(const BUnit* pTarget, BVector& targetPosition, BVector& targetOffset, bool targetGround) const;
      float                      getDamageAmount(const BUnit* pUnit, const BUnit* pTarget) const;
      void                       getTurretPositionForward(BVector &position, BVector &forward) const;
      void                       startAttacking();
      void                       stopAttacking();
      void                       tryAttackingTarget();
      void                       trackTarget(float elapsed);
      BVector                    applyGravityOffset(const BUnit* pTarget, BVector targetPosition) const;
      const BSimTarget*          getTarget() const { return (getParentAction()) ?  getParentAction()->getTarget() : NULL; }
      void                       updateTargetingLead();
      bool                       updateTurretRotation(float elapsed);

      BVector                    mTargetingLead;
      BAction*                   mpParentAction;
      DWORD                      mLastLOSValidationTime;

      bool                       mIsAttacking:1;
};
