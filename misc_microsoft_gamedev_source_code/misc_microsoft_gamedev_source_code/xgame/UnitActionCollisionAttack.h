//==============================================================================
// UnitActionCollisionAttack.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionCollisionAttack : public BAction
{
   public:
      BUnitActionCollisionAttack() { }
      virtual ~BUnitActionCollisionAttack() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      void                       setMoveOrder(BSimOrder* pOrder) { mpMoveOrder = pOrder; if (mpMoveOrder) mpMoveOrder->incrementRefCount(); }
      void                       setAbilityID(int abilityID) { mAbilityID = abilityID; }

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target) { mTarget=target; }

      void                       setFlagIgnoreTarget(bool v) { mFlagIgnoreTarget = v; }
      void                       setFlagCollideWithFriendlies(bool v) { mFlagCollideWithFriendlies = v; }
      void                       setFlagIgnoreAmmo(bool v) { mFlagIgnoreAmmo = v; }
      void                       setFlagUseObstructionForCollisions(bool v) { mFlagUseObstructionForCollisions = v; }

      bool                       isBowlableOrRammable(BEntity* pEntity) const;
      bool                       isBowlable(BEntity* pEntity) const;

      DECLARE_FREELIST(BUnitActionCollisionAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       handleEvades(const BEntityIDArray& collisions, BVector unitForward, BVector unitPosition);
      void                       handleCollisions(const BEntityIDArray& collisions);
      bool                       isUnitBowlableOrRammable(BUnit* pUnit, bool& bowlable, bool& rammable, bool checkRange) const;
      void                       makeLeashOpps(BSquad* pSquad);
      void                       checkForCollisionsWithObstruction(BUnit* pUnit, BEntityIDArray& collisions, const BEntityIDArray& ignoreUnits);

      BSimTarget                 mTarget;
      BSimOrder*                 mpMoveOrder;
      BEntityIDArray             mEvadedUnits;  
      int                        mAbilityID;
      bool                       mFlagIgnoreTarget:1;
      bool                       mFlagCollideWithFriendlies:1;
      bool                       mFlagIgnoreAmmo:1;
      bool                       mFlagUseObstructionForCollisions:1;
};