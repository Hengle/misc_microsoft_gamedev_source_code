//==============================================================================
// unitactionunderattack.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;


//==============================================================================
//==============================================================================
class BUnitActionUnderAttack : public BAction
{
public:
   BUnitActionUnderAttack() {}
   virtual ~BUnitActionUnderAttack() {}

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();
   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   // Stuff for finding out about what is attacking us.
   void                       addAttackingUnit(BEntityID unitID, BActionID actionID);
   void                       removeAttackingUnit(BEntityID unitID, BActionID actionID);
   bool                       isBeingAttackedByUnit(BEntityID unitID, BActionID actionID, uint &index) const;
   bool                       isBeingAttackedByUnit(BEntityID unitID) const;
   bool                       isBeingAttacked() const;
   uint                       getNumberAttackingUnits() const;
   BEntityID                  getAttackingUnitByIndex(uint index) const;
   uint                       getAttackingUnits(BEntityIDArray& attackingUnits) const;

   // Stuff for finding out about what damaged us.
   void                       addDamage(BEntityID attackerUnitID, BTeamID attackingTeamID, DWORD expires);
   bool                       wasDamagedBy(BEntityID attackerUnitID, BTeamID attackingTeamID, uint &index) const;

   DECLARE_FREELIST(BUnitActionUnderAttack, 5);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   class BUnitDamagedBy
   {
   public:
      BEntityID   mAttackerUnitID;
      BTeamID     mAttackingTeamID;
      DWORD       mExpires;
   };

   class BUnitAttackedBy
   {
   public:
      BEntityID   mAttackingUnitID;
      BActionID   mAttackingActionID;
   };

   BSmallDynamicSimArray<BUnitAttackedBy> mAttackingUnits;
   BSmallDynamicSimArray<BUnitDamagedBy>  mUnitDamagedBy;
};