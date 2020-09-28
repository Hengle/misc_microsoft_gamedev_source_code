//==============================================================================
// unitactionbuff.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionBuff : public BAction
{
   public:
      BUnitActionBuff() { }
      virtual ~BUnitActionBuff() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void disconnect();
      virtual bool init();
      virtual bool setState(BActionState state);
      virtual bool update(float elapsed);

      DECLARE_FREELIST(BUnitActionBuff, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool needsToHeal() const;

      BUnitOppID mHealerAnimOppID;
};