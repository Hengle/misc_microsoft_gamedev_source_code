//==============================================================================
// unitactionchangemode.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "cost.h"

class BUnit;


//==============================================================================
//==============================================================================
class BUnitActionChangeMode : public BAction
{
   public:      
      BUnitActionChangeMode() { }
      virtual ~BUnitActionChangeMode() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;

      void                       setSquadMode(long val)  { mSquadMode = val; }
      long                       getSquadMode()          { return mSquadMode; }

      DECLARE_FREELIST(BUnitActionChangeMode, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      long                       mSquadMode;
      long                       mAnimType;
      BUnitOppID                 mOppID;
      BActionState               mFutureState;
};
