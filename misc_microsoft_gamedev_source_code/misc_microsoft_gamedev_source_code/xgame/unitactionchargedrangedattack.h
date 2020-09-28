//==============================================================================
// UnitActionBomb.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionChargedRangedAttack : public BAction
{
   public:
      BUnitActionChargedRangedAttack() { }
      virtual ~BUnitActionChargedRangedAttack() { }

      virtual bool         init();
      virtual bool         update(float elapsed);

      void                 clearCharge()        { mFlagReset = true; }
      bool                 isCharged() const;

      float                getDamageCharge() const;

      bool                 isValidPullTarget(const BObject* pObject, const BProtoAction* pProtoAction = NULL) const;
      bool                 isValidPullTarget(const BUnit* pUnit, const BProtoAction* pProtoAction = NULL) const;
      bool                 isValidPullTarget(const BSquad* pSquad, const BProtoAction* pProtoAction = NULL) const;

      bool                 canPull(const BProtoAction* pProtoAction, const BUnit* pTarget) const;

      DECLARE_FREELIST(BUnitActionChargedRangedAttack, 4);
  
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BEntityID            mChargedEffect;
      float                mCharge;
      bool                 mFlagReset:1;
};