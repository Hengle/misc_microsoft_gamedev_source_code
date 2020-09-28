//==============================================================================
// skullManager.h
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once

#if 1

#include "gamefilemacros.h"

class BUser;
class BCollectiblesManager;
class BGameSettings;
class BProtoObject;

// Global variable for the one BCollectiblesManager object
extern BCollectiblesManager gCollectiblesManager;

enum SkullModifiers
{
   cSK_None = 0,
   cSK_Score,
   cSK_GruntTank,
   cSK_GruntConfetti,
   cSK_Physics,
   cSK_ScarabBeam, //5
   cSK_MinimapDisable,
   cSK_Weakness,
   cSK_HitpointMod,
   cSK_DamageMod,
   cSK_Veterancy, //10
   cSK_AbilityRecharge,
   cSK_DeathExplode,
   cSK_TrainMod,
   cSK_SupplyMod,
   cSK_PowerRecharge, //15

   // [10/7/2008 xemu] "LCE" custom unit skulls
   cSK_UnitModWarthog,
   cSK_UnitModWraith,
};

enum SkullModifierTargets
{
   cSKT_NoTarget = 0,
   cSKT_PlayerUnits,
   cSKT_NonPlayerUnits,
   cSKT_OwnerOnly,
};

enum TimeLineEventTypes
{
   cTLET_Invalid = 0,
   cTLET_BlackBox,
   cTLET_Leader,
   cTLET_Map,
};

//============================================================================
//BSkullObject
//============================================================================
class BSkullModifier
{
   public:
      BSkullModifier();
      ~BSkullModifier();

      int   mModifierType;
      float mModifierValue;
      int   mModifierTarget;
};

//============================================================================
//BProtoSkull
//============================================================================
class BProtoSkull
{
   public:
      BProtoSkull();
     ~BProtoSkull();
      
      bool loadXML(BXMLNode node);
      bool isOwnerOnly() const;
      bool checkActive(long playerID) const;


      BDynamicSimArray<BSkullModifier *> mModifiers;
      BString mName;
      BString mDisplayImageOn;
      BString mDisplayImageOff;
      BString mDisplayImageLocked;
            
      long mObjectDBID;      
      int  mDisplayNameID;
      int  mDescriptionID;
      int  mIndex;
      bool mActive : 1; // [10/10/2008 xemu] have we received a real activation command for this skull? 
      bool mUIActive : 1; // [10/10/2008 xemu] have we been flagged active locally in our UI?
      bool mSelfActive : 1; // [10/10/2008 xemu] have we been activated by this player in specific, as an actual command?
      bool mSelfUIActive : 1; // [10/10/2008 xemu] have we been activated by this player in specific, as a UI input?
      bool mHidden : 1;
      bool mCommandSent : 1;
      bool mOnFromBeginning : 1; // [10/20/2008 xemu] was this skull turned on in the first 60 seconds? 
};

//============================================================================
//BProtoTimeLineEvent
//============================================================================
class BProtoTimeLineEvent
{
   public:
      BProtoTimeLineEvent();
     ~BProtoTimeLineEvent();
      
      bool loadXML(BXMLNode node);

      BString mEventName;      //For matching to the timeline system.
      long mEventType;
      long mObjectDBID;        //If it is a blackbox
      BString mLeaderName;  //If it is triggered by playing with a leader
      BString mMapName;     //If it is triggered by playing with a particular map
};

//============================================================================
//BCollectiblesManager
//============================================================================
class BCollectiblesManager 
{
   public:
      BCollectiblesManager();
      ~BCollectiblesManager();

      void reset();

      bool loadCollectiblesDefinitions();

      void reportSkullCollected(BUser* pUser, long objectdbid);     
      const BProtoSkull* getSkullDefinition(int index) const;      
      int  getNumberSkulls(bool countHidden) const;
      int  getNumSkullsCollected(BUser *pUser, bool countHidden, bool countOwnerOnly) const;
      bool isSkullHidden(int index) const;

      void sendSkullActivationCommands(long playerID);
      void setSkullActivationUI(int index, bool active, long playerID);
      bool getSkullActivationUI(int index) const;
      void setSkullActivationInternal(int index, bool active, long playerID);
      bool areAllDebuffSkullsActive() const;
      bool canToggleSkull(int index) const;

      float getScoreModifierUI();

      bool hasSkullBeenCollected(BUser* pUser, int index);
      bool hasBlackBoxBeenCollected(BUser* pUser, int index);
      void clearSkullActivations();

      void reportBlackBoxCollected(BUser* pUser, const BProtoObject* pTarget);
      void updateGame(BUser *pUser, BGameSettings* pSettings);
      bool hasTimeLineEventBeenUnlocked(BUser *pUser, const BString &name);
      bool hasTimeLineEventBeenSeen(BUser *pUser, const BString &name);
      bool haveNewTimeLineEventsBeenUnlocked(BUser *pUser);
      bool markAllTimeLineEventsSeen(BUser *pUser);
      int  getNumTimelineEventsUnlocked(BUser *pUser);

      bool getRocketAllGrunts() const {return mbRocketAllGrunts;}
      bool isMinimapHidden() const {return mbMinimapHidden;}
      int  getBonusSquadLevels() const { return mBonusSquadLevels; }

      float getDeathExplodeChance() const { return mDeathExplodeChance; }
      int   getDeathExplodeObjectType() const { return mDeathExplodeObjectType; }
      long  getDeathExplodeProtoObject() const { return mDeathExplodeProtoObject; }

      void unlockHiddenSkulls();
      void   setupTickerInfo();

#ifndef BUILD_FINAL
      void overrideSkullsCollected(BUser *user, uint32 bits);
      void overrideTimeLineEventsCollected(BUser *user, long type, bool collected);
#endif

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      bool getUnlockData(bool &unlock, int &skullUnlockIndex);
      void unhideSkull(BUser *pUser, int skullUnlockIndex);

      BDynamicSimArray<BProtoSkull *> mSkullDefinitions;
      BDynamicSimArray<BProtoTimeLineEvent *> mTimeLineEventDefinitions;
      uint32 mVersion;
      bool mbRocketAllGrunts;
      bool mbMinimapHidden;
      int mBonusSquadLevels;

      float          mDeathExplodeChance;
      int            mDeathExplodeObjectType;
      long           mDeathExplodeProtoObject;
};

#endif
