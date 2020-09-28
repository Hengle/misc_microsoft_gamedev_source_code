//==============================================================================
// unitactionherodeath.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionHeroDeath : public BAction
{
   public:
      BUnitActionHeroDeath() {}
      virtual ~BUnitActionHeroDeath() {}

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               isAllowedWhenGameOver() { return (true); }

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

      //XXXHalwes - 5/2/2008 - No fatalities against heroes?
      //void                       setFlagDoingFatality(bool v) { mFlagDoingFatality = v; }
      //bool                       getFlagDoingFatality() { return (mFlagDoingFatality); }

      //Add block pool
      DECLARE_FREELIST(BUnitActionHeroDeath, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      // SPC VoG hero stuff
      void                       playSPCHeroDownedSound();
      void                       playSPCHeroRevivedSound();

      //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
      //void                       checkForImpact();

      //XXXHalwes - 5/2/2008 - Any flying Heroes?  Minister?
      //float                      mCrashRollAccel;
      //float                      mRollRate;
      //float                      mCrashPitchAccel;
      //float                      mPitchRate;

      BEntityID                  mKillingEntity;
      long                       mKillingWeaponType;
      BUnitOppID                 mOppID;      
      float                      mRegenRate;
      int                        mDeathAnimType;

      //XXXHalwes - 5/2/2008 - No fatalities against heroes?
      //bool                       mFlagDoingFatality:1;
};
