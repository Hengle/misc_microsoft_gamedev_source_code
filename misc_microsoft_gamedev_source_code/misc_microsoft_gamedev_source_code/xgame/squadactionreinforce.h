//==============================================================================
// squadactionreinforce.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionReinforce : public BAction
{
   public:
      BSquadActionReinforce(){}
      virtual ~BSquadActionReinforce(){}

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      // Member interfaces
      void                       setReinforcementsSquad(BEntityID reinforcementsSquad){ mReinforcementsSquad = reinforcementsSquad; }
      void                       setReinforceSquad(BEntityID squadID) { mReinforceSquad = squadID; }
      void                       setFlyInLocation(BVector location){ mFlyInLocation = location; }
      void                       setFlyOffLocation(BVector location){ mFlyOffLocation = location; }
      void                       setCompletionSoundCue(BCueIndex cueIndex){ mCompletionSoundCue = cueIndex; }

      // Flag interfaces
      void                       setFlyInFrom(bool v){ mFlagFlyInFrom = v; }
      void                       setUnloadSquad(bool v){ mFlagUnloadSquad = v; }
      void                       setFlyOffTo(bool v){ mFlagFlyOffTo = v; }
      void                       setUseMaxHeight(bool v){ mFlagUseMaxHeight = v; }

      static bool                reinforceSquad(BSimOrder* pOrder, BEntityID reinforceSquad, BEntityID reinforcementsSquad, const BVector flyInLocation = cInvalidVector, const BVector flyOffLocation = cInvalidVector, BCueIndex soundCue = cInvalidCueIndex, bool useMaxHeight = true);

      DECLARE_FREELIST(BSquadActionReinforce, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       flyInTransport();
      bool                       flyOffTransport();
      void                       preLoadSquad();
      bool                       isSquadLoaded();
      bool                       flyInFrom();
      bool                       flyOffTo();
      bool                       unloadSquad();
      void                       selfDestruct();
      void                       playCompletionSoundCue();
      bool                       prepReinforcements();

      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();
      void                       setupChildOrder(BSimOrder* pOrder);

      BSimTarget                 mTarget;      
      BSimOrder*                 mpChildOrder;
      BVector                    mFlyInLocation;
      BVector                    mFlyOffLocation;
      BEntityID                  mReinforcementsSquad;
      BEntityID                  mReinforceSquad;
      BCueIndex                  mCompletionSoundCue;      
      BActionID                  mChildActionID;
      BUnitOppID                 mUnitOppID;  
      BActionState               mFutureState;      
      uint8                      mUnitOppIDCount;

      bool                       mFlagFlyInFrom:1;
      bool                       mFlagUnloadSquad:1;
      bool                       mFlagFlyOffTo:1;
      bool                       mFlagUseMaxHeight:1;
      bool                       mFlagActionFailed:1;
      bool                       mFlagAnyFailed:1;      
      bool                       mFlagNonMobileCache:1;
};
