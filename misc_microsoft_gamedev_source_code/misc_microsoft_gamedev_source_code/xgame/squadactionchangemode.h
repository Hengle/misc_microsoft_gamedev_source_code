//==============================================================================
// squadactionchangemode.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "UnitOpportunity.h"

//==============================================================================
//==============================================================================
class BSquadActionChangeMode : public BAction
{
   public:
      BSquadActionChangeMode() { }
      virtual ~BSquadActionChangeMode() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      void                       setSquadMode(int8 val)    { mSquadMode = val; }
      int8                       getSquadMode() const       { return mSquadMode; }

      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction=v; }

      virtual bool               isInterruptible() const    { return false; }

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target) { mTarget=target; }

      DECLARE_FREELIST(BSquadActionChangeMode, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();

      BAction*                   mpParentAction;
      BUnitOppID                 mUnitOppID;
      BActionState               mFutureState;
      int8                       mSquadMode;
      BSimTarget                 mTarget;
};
