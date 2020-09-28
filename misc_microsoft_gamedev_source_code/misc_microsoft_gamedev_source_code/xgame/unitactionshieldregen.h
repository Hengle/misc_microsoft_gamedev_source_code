//==============================================================================
// unitactionshieldregen.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"


//==============================================================================
//==============================================================================
class BUnitActionShieldRegen : public BAction
{
   public:
      BUnitActionShieldRegen() { }
      virtual ~BUnitActionShieldRegen() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BUnitActionShieldRegen, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      DWORD                      mActionEndTime;
};