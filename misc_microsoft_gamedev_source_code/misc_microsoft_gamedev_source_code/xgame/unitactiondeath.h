//==============================================================================
// unitactiondeath.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionDeath : public BAction
{
   public:
      BUnitActionDeath() { }
      virtual ~BUnitActionDeath() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               isAllowedWhenGameOver() { return true; }

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);    

      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;

      // Killing Unit
      void                       setKillingEntity(BEntityID killingEntity) { mKillingEntity = killingEntity; }
      BEntityID                  getKillingEntity() { return (mKillingEntity); }
      void                       setKillingWeaponType(long killingWeaponType) { mKillingWeaponType = killingWeaponType; }
      long                       getKillingWeaponType() { return (mKillingWeaponType); }

      void                       setFlagDoingFatality(bool v) { mFlagDoingFatality = v; }
      bool                       getFlagDoingFatality() { return (mFlagDoingFatality); }

      //Add block pool
      DECLARE_FREELIST(BUnitActionDeath, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      void                       checkForImpact();

      float                      mCrashRollAccel;
      float                      mRollRate;
      float                      mCrashPitchAccel;
      float                      mPitchRate;
      BEntityID                  mKillingEntity;
      long                       mKillingWeaponType;
      long                       mDeathAnimType;
      BUnitOppID                 mOppID;
      bool                       mbSetToDetonate:1;
      bool                       mFlagDoingFatality:1;
};
