//==============================================================================
// unitactiondodge.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionDodge : public BAction
{
   public:
      BUnitActionDodge();
      virtual ~BUnitActionDodge() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder) { return BAction::connect(pOwner, pOrder); }
      virtual void               disconnect() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      bool                       canDodge() const;
      bool                       tryDodge(BEntityID dodgeeID, BVector trajectory);

      void                       setDodgeAnimOppID(BUnitOppID v) { mDodgeAnimOppID = v; }
      void                       setDodgeeID(BEntityID id) { mDodgeeID = id; }
      BEntityID                  getDodgeeID() const { return mDodgeeID; }

      DWORD                      getDodgeInitTime() const { return mDodgeInitTime; }
      DWORD                      getDodgeCooldownDoneTime() { return mDodgeCooldownDoneTime; }

      DECLARE_FREELIST(BUnitActionDodge, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       obstructedByTerrainBoundaries(BVector pos, float radius);

      BUnitOppID                 mDodgeAnimOppID;
      BEntityID                  mDodgeeID; // entity the unit is dodging, NULL if not currently dodging
      DWORD                      mDodgeCooldownDoneTime;
      DWORD                      mDodgeInitTime;
//      DWORD                      mNextRollTime;
};