//==============================================================================
// unitactiontentacledormant.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"


//==============================================================================
//==============================================================================
class BUnitActionTentacleDormant : public BAction
{
   public:
      BUnitActionTentacleDormant() { }
      virtual ~BUnitActionTentacleDormant() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v) { mOppID = v; }

      // Active force
      void                       forceActive(float timeActive);

      DECLARE_FREELIST(BUnitActionTentacleDormant, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       isEnemyWithinProximity();

      BUnitOppID                 mOppID;
      float                      mNextProximityCheck;
      bool                       mEnemies;
      bool                       mFlagForcedActive;
      float                      mActiveTimeLeft;
};
