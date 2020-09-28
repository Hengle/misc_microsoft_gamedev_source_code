//==============================================================================
// squadactionairstrike.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BSquadActionAirStrike : public BAction
{
   public:
      BSquadActionAirStrike() { }
      virtual ~BSquadActionAirStrike() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      void                       setAirStrikeLocation(BVector location) { mAirStrikeLocation = location; }
      BVector                    getAirStrikeLocation() { return(mAirStrikeLocation); }
      void                       setStartingLocation(BVector location) { mStartingLocation = location; }
      void                       setAutoRTB(bool flag) { mbAutoReturnToBase = flag; }

      static bool                airStrike(BSimOrder* pSimOrder, BEntityID strikerSquadID, BSimTarget* target, BVector launchLocation);

      DECLARE_FREELIST(BSquadActionAirStrike, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       setupAttacks();
      bool                       strike();
      bool                       returnToBase();

      void                       removeChildActions();
      bool                       actionsComplete(BActionID id);
      void                       checkAmmoAndFuel(float elapsed);
      void                       rearm();
      void                       checkOnStation();

      BVector                    mStartingLocation;
      BVector                    mAirStrikeLocation;
      BEntityID                  mStrikeSquad;
      BActionID                  mChildActionID;
      BActionState               mFutureState;
      float                      mLoiterTime;
      bool                       mbOnStation;
      bool                       mbAutoReturnToBase;
};