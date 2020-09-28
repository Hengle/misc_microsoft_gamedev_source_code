//==============================================================================
// UnitActionAreaAttack.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionAreaAttack : public BAction
{
   public:
      BUnitActionAreaAttack() { }
      virtual ~BUnitActionAreaAttack() { }

      virtual bool               init();
      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BUnitActionAreaAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       doAttack();

      float                      mAttackTimer;
      bool                       mFlagStickyProjectile;

};