//==============================================================================
// squadactionrepair.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BSquadActionRepairUnit
{
   public:
      BEntityID mUnitID;
      float     mHitpoints;
};

//==============================================================================
//==============================================================================
class BSquadActionRepair : public BAction
{
   public:
      BSquadActionRepair() { }
      virtual ~BSquadActionRepair() { }

      //IPoolable Methods
      //virtual void               onAcquire() { }
      //virtual void               onRelease() { }
      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      //virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      float                      getRepairPercent() const;

      DECLARE_FREELIST(BSquadActionRepair, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       addUnitOpp();
      void                       removeUnitOpp();
      void                       InitFromSquad(BSquad* pSquad);


      BSmallDynamicSimArray<BSquadActionRepairUnit>   mUnits;
      BSmallDynamicSimArray<int> mReinforcements;
      float                      mTimer;
      BUnitOppID                 mUnitOppID;
      DWORD                      mDamagedTime;
      float                      mRepairPercent;
      int                        mReinforceIndex;
      bool                       mSaveFlagNonMobile;
      BEntityID                  mTargetSquad;
};