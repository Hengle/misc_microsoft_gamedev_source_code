//==============================================================================
// unitactionheal.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionHeal : public BAction
{
   public:
      BUnitActionHeal() { }
      virtual ~BUnitActionHeal() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void disconnect();
      virtual bool init();
      virtual bool setState(BActionState state);
      virtual bool update(float elapsed);

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void setTarget(BSimTarget target);

      DECLARE_FREELIST(BUnitActionHeal, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool needsToHeal() const;
      bool timeToHeal(const BSquad* pSquad) const;

      BUnitOppID mHealerAnimOppID;
      BSimTarget mTarget;
};