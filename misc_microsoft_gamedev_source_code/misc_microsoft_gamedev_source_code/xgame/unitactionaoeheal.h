//==============================================================================
// unitactionaoeheal.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionAoeHeal : public BAction
{
   public:
      BUnitActionAoeHeal() { }
      virtual ~BUnitActionAoeHeal() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual bool init();
      virtual bool update(float elapsed);

      DECLARE_FREELIST(BUnitActionAoeHeal, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BUnitOppID  mHealerAnimOppID;
      BPowerID    mPowerID;
};