//==============================================================================
// unitactioninfectdeath.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionInfectDeath : public BAction
{
   public:
      BUnitActionInfectDeath() { }
      virtual ~BUnitActionInfectDeath() { }

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

      void                       setInfectionPlayerID(uint8 id) { mInfectionPlayerID = id; }
      void                       setFormerSquad(BEntityID id) { mFormerSquad = id; }
      void                       setSkipDeathAnim(bool val) { mbSkipDeathAnim = val; }

      //Add block pool
      DECLARE_FREELIST(BUnitActionInfectDeath, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      bool                       findSpotToDie();

      BSimTarget                 mTarget;
      //BDynamicSimArray<uint>     mUnitOppIDs;
      uint8                      mInfectionPlayerID;
      BActionID                  mChildActionID;
      BEntityID                  mFormerSquad;
      bool                       mbSkipDeathAnim;
      BUnitOppID                 mOppID;

};
