//==============================================================================
// player.h
//
// Copyright (c) 2005-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "civ.h"
#include "cost.h"
#include "leaders.h"
#include "maximumsupportedplayers.h"
#include "pop.h"
#include "powerentry.h"
#include "simtypes.h"
#include "rallypoint.h"

// Forward declarations
class BAI;
class BAlertManager;
class BCiv;
class BHintEngine;
class BKB;
class BProtoObject;
class BProtoSquad;
class BProtoTech;
class BSquad;
class BTeam;
class BTechTree;
class BUnit;
class BUser;
class BObject;
class BStatsPlayer;
class BWeaponType;
class BHumanPlayerAITrackingData;
class BAIFactoidManager;

//==============================================================================
//==============================================================================
class BUniqueProtoObjectEntry
{
   public:
      BProtoObject*  mProtoObject;
      //BProtoObjectID mBasePOID;   // not using these fields for now
      //BEntityID      mEntityID;
};

//==============================================================================
//==============================================================================
class BUniqueProtoSquadEntry
{
   public:
      BProtoSquad*   mProtoSquad;
      //BProtoSquadID  mBasePSID;   // not using these fields for now
      //BEntityID      mSquadID;
};

#define UNIQUEPROTOPLAYERMASK 0x7F000000                                      // 7 bits for player ID (don't use sign bit as some things check sign for validity)
#define UNIQUEPROTOIDMASK 0x80FFFFFF                                          // preserve the sign bit in the proto ID
#define GETUNIQUEPROTOPLAYERID(n) (((n & UNIQUEPROTOPLAYERMASK) >> 24) - 1)   // player ID offset by 1 so player 0 can have uniques
#define GETUNIQUEPROTOID(n) (n & UNIQUEPROTOIDMASK)
#define MAKEUNIQUEPID(playerID, id) ( ((playerID + 1) << 24) | (id & UNIQUEPROTOIDMASK) )

//==============================================================================
// BPlayerPop
//==============================================================================
class BPlayerPop
{
   public:
                     // 16 bytes total
      float mCount;  // 4 bytes
      float mCap;    // 4 bytes
      float mMax;    // 4 bytes
      float mFuture; // 4 bytes
};

typedef BDynamicSimArray<BProtoObject*,4,BDynamicArrayNoConstructNoGrowClearNewOptions> BPlayerProtoObjectArray;
typedef BDynamicSimArray<BProtoSquad*,4,BDynamicArrayNoConstructNoGrowClearNewOptions> BPlayerProtoSquadArray;
typedef BDynamicSimArray<BProtoTech*,4,BDynamicArrayNoConstructNoGrowClearNewOptions> BPlayerProtoTechArray;
typedef BDynamicSimArray<BUniqueProtoObjectEntry,4,BDynamicArrayNoConstructNoGrowClearNewOptions> BPlayerUniqueProtoObjectArray;
typedef BDynamicSimArray<BUniqueProtoSquadEntry,4,BDynamicArrayNoConstructNoGrowClearNewOptions> BPlayerUniqueProtoSquadArray;

//==============================================================================
// BPlayer
//==============================================================================
class BPlayer
{
   public:
      // Halwes - 4/2/2008 - Player state enumerations need to be reflected in gamedata.xml with PlayerState tags for the editor and trigger system
      //                     to work correctly.
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      enum
      {
         cPlayerStatePlaying      = 0,
         cPlayerStateResigned     = 1,
         cPlayerStateDefeated     = 2,
         cPlayerStateDisconnected = 3,
         cPlayerStateWon          = 4,
         cPlayerStateCount,
      };
      // Halwes - 4/2/2008 - Player state enumerations need to be reflected in gamedata.xml with PlayerState tags for the editor and trigger system
      //                     to work correctly.
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! READ THIS COMMENT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      enum BPlayerNetState
      {
         cPlayerNetStateConnected = 0,
         cPlayerNetStateDisconnected,
      };

      enum 
      { 
         cSupportPowerStatusUnavailable, 
         cSupportPowerStatusAvailable, 
         cSupportPowerStatusSelected, 
      };

      enum
      {
         cPlayerTypeNPC,
         cPlayerTypeComputerAI,
         cPlayerTypeHuman,
      };

                                          BPlayer();
                                          ~BPlayer();

      bool                                setup(long id, long mpid, BSimString& name, BUString& localisedName, long civID, long leaderID, float difficulty, BSmallDynamicSimArray<long>* pForbidObjects, BSmallDynamicSimArray<long>* pForbidSquads, BSmallDynamicSimArray<long>* pForbidTechs, BSmallDynamicSimArray<BLeaderPop>* pLeaderPops, uint8 playerType, int scenarioID, bool fromSave);
      bool                                initializeFactoidManager();
      void                                reset();
      void                                update(float elapsedTime);
      void                                render();     

      long                                getID() const { return mID; }
      long                                getMPID() const { return mMPID; }

      XUID                                getXUID() const { return mXUID; }
      void                                setXUID(XUID newXuid) { mXUID = newXuid; }

      void                                setLookAtPos(BVector pos) { mLookAtPos = pos; }
      BVector                             getLookAtPos() const { return (mLookAtPos); }

      long                                getColorIndex() const { return mColorIndex; }
      void                                setColorIndex(long color) { mColorIndex = color; }

      BPlayerID                           getCoopID() const { return mCoopID; }
      void                                setCoopID(BPlayerID id);

      BPlayerID                           getScenarioID() const { return mScenarioID; }

      long                                getCivID() const { return mCivID; }
      BCiv*                               getCiv() const;

      BPlayerState                        getPlayerState() const { return mPlayerState; }
      void                                setPlayerState(BPlayerState v, bool fromSave=false);
      bool                                isPlaying() const { return (mPlayerState == cPlayerStatePlaying); }
      uint32                              getGamePlayedTime() const { return mGamePlayedTime; } 

      BPlayerNetState                     getNetState() const { return mNetState; }
      void                                setNetState(BPlayerNetState netState) { mNetState = netState; }

      const BSimString&                   getName() const { return mName; }
      const BUString&                     getLocalisedDisplayName() const { return mLocalisedDisplayName; }

      long                                getLeaderID() const { return mLeaderID; }
      BLeader*                            getLeader() const;

      long                                getNumberProtoObjects() const { return mProtoObjects.getNumber(); }
      BProtoObject*                       getProtoObject(long id) const;
      long                                getNumberUniqueProtoObjects() const { return mUniqueProtoObjects.getNumber(); }
      long                                getUniqueProtoObjectIndex(long id) const;
      long                                getUniqueProtoObjectIDFromIndex(long index) const;

      long                                getNumberProtoSquads() const { return mProtoSquads.getNumber(); }
      BProtoSquad*                        getProtoSquad(long id) const;
      long                                getNumberUniqueProtoSquads() const { return mUniqueProtoSquads.getNumber(); }
      long                                getUniqueProtoSquadIndex(long id) const;
      long                                getUniqueProtoSquadIDFromIndex(long index) const;

      long                                getNumberProtoTechs() const { return mProtoTechs.getNumber(); }
      BProtoTech*                         getProtoTech(long id) const;

      BTeam*                              getTeam();
      const BTeam*                        getTeam() const;
      BTeamID                             getTeamID() const { return mTeamID; }
      void                                setTeamID(BTeamID id);

      BUser*                              getUser()             { return mpUser;  }
      void                                setUser(BUser* pUser);

      void                                sendResign();
      void                                doResign();

      void                                sendDisconnect();
      void                                doDisconnect();

      void                                addStrength(int32 strength);
      void                                subtractStrength(int32 strength);
      int32                               getStrength() const;
      int32                               getActualStrength() const { return mStrength; }
      void                                setActualStrength(int32 val) { mStrength=val; }

      void                                processGameEnd(bool force=false);

      // AI
      float                                getAIStrength() const;
     
      BAlertManager*        getAlertManager() const { return (mpAlertManager); }

      // Resources
      enum // flags for addResource to distinguish where we are getting the amount from
      {
         cFlagNormal = 0,
         cFlagFromGather = 1,
         cFlagFromTribute = 2,
         cFlagFromBounty = 4,
      };
      bool                                isResourceActive(long r) const { return (mActiveResources.isSet(r)!=0); }
      float                               getResource(long resourceID) const;
      void                                setResource(long resourceID, float amount);
      void                                setResources(const BCost *pResources);
      const BCost&                        getResources() const;
      void                                addResource(long resourceID, float amount, uint flags=cFlagNormal, bool noHandicap=false);
      bool                                checkCost(const BCost* pCost) const;
      bool                                payCost(const BCost* pCost);
      bool                                refundCost(const BCost* pCost);
      bool                                checkAvail(const BCost* pCost) const;
      void                                setBountyResource(long resourceID) { mBountyResource=resourceID; }
      long                                getBountyResource() const { return mBountyResource; }
      float                               getTotalResource(long resourceID) const;
      void                                setTotalResource(long resourceID, float amount);
      void                                setTotalResources(const BCost* pResources);
      const BCost&                        getTotalResources() const;
      void                                addTotalResource(long resourceID, float amount);
      bool                                checkTotal(const BCost* pCost) const;
      // Handicap
      void                                setResourceHandicap(float value){mHandicapMultiplier=value;}
      float                               getResourceHandicap(){return mHandicapMultiplier;}

      // Rates
      float                               getRate(int rateID) const { return mpRateAmounts[rateID] * mpRateMultipliers[rateID]; }
      float                               getRateAmount(int rateID) const { return mpRateAmounts[rateID]; }
      void                                setRateAmount(int rateID, float val) { mpRateAmounts[rateID]=val; }
      void                                adjustRateAmount(int rateID, float val) { mpRateAmounts[rateID]+=val; }
      float                               getRateMultiplier(int rateID) const { return mpRateMultipliers[rateID]; }
      void                                setRateMultiplier(int rateID, float val) { mpRateMultipliers[rateID]=val; }

      // Pop
      float                               getPopCount(long id) const;
      void                                setPopCount(long id, float count);
      void                                adjustPopCount(long id, float count);

      float                               getPopCap(long id) const;
      void                                setPopCap(long id, float count);
      void                                adjustPopCap(long id, float count);

      float                               getPopMax(long id) const;
      void                                setPopMax(long id, float count);
      void                                adjustPopMax(long id, float count);

      float                               getPopFuture(long id) const;
      void                                setPopFuture(long id, float count);
      void                                adjustPopFuture(long id, float count);
      void                                payPopFuture(const BPopArray* pPops);
      void                                refundPopFuture(const BPopArray* pPops);

      bool                                checkPop(long id, float count) const;
      bool                                checkPops(const BPopArray* pPops) const;

      // Rally point
      bool                                haveRallyPoint() const { return getFlagRallyPoint(); }
      BVector                             getRallyPoint() const;
      void                                getRallyPoint(BVector& rallyPoint, BEntityID& rallyPointEntityID);
      void                                setRallyPoint(BVector point, BEntityID entityID);
      void                                clearRallyPoint();
      void                                setRallyPointVisible(bool val);
      void                                checkRallyPoint();

      // Is gaia.
      bool                                isGaia() const { return (mID == cGaiaPlayer); }
      
      // Human or AI
      bool                                isHuman(void) const { return (mPlayerType == cPlayerTypeHuman); }
      bool                                isComputerAI(void) const {return (mPlayerType == cPlayerTypeComputerAI);}
      bool                                isNPC() const { return mPlayerType == cPlayerTypeNPC; }
      int8                                getPlayerType() const { return mPlayerType; }
      void                                setPlayerType(int8 v) { mPlayerType = v; }

      // Enemy/Ally data
      bool                                isEnemy(long playerID) const;
      bool                                isAlly(long playerID, bool selfAsAlly=true) const;
      bool                                isSelf(long playerID) const;
      bool                                isNeutral(long playerID) const;

      bool                                isEnemy(const BPlayer* pPlayer) const;
      bool                                isAlly(const BPlayer* pPlayer, bool selfAsAlly=true) const;
      bool                                isSelf(const BPlayer* pPlayer) const;
      bool                                isNeutral(const BPlayer* pPlayer) const;

      // Power Related
      bool                                canUsePower(BProtoPowerID protoPowerID, BEntityID squadID = cInvalidObjectID) const;
      bool                                hasAvailablePowerEntryUses(BProtoPowerID protoPowerID) const;
      bool                                canAffordToCastPower(BProtoPowerID protoPowerID) const;
      bool                                passesPop(BProtoPowerID protoPowerID) const;
      bool                                usePower(BProtoPowerID protoPowerID, BEntityID squadID, BEntityID targetID, float costMultiplier = 1.0f, bool ignoreRequirements = false);
      bool                                restartPowerRecharge(BProtoPowerID protoPowerID, BEntityID squadID);
      bool                                addPowerEntry(BProtoPowerID protoPowerID, BEntityID squadID, long uses, int iconLocation, bool ignoreCost = false, bool ignoreTechPrereqs = false, bool ignorePop = false);
      void                                removePowerEntry(BProtoPowerID protoPowerID, BEntityID squadID);
      void                                removePowerEntryAtIconLocation(int iconLocation);
      void                                clearPowerEntries() { mPowerEntries.clear(); }
      BPowerEntry*                        findPowerEntry(BProtoPowerID protoPowerID);
      const BPowerEntry*                  findPowerEntry(BProtoPowerID protoPowerID) const;
      long                                getNumPowerEntries(void) const { return (mPowerEntries.getNumber()); }
      BPowerEntry*                        getPowerEntryAtIndex(long index) { BASSERT(index >= 0 && index < mPowerEntries.getNumber()); return (&mPowerEntries[index]); }
      uint                                getNumberSupportPowers() const { return mSupportPowerStatus.getSize(); }
      const BLeaderSupportPower*          getSupportPower(uint index) const;
      BYTE                                getSupportPowerStatus(uint index) const { return mSupportPowerStatus[index]; }
      void                                setSupportPowerStatus(uint index, BYTE status);
      BSmallDynamicSimArray<BPowerEntry>& getPowerEntries() { return mPowerEntries; }

      // Tech tree
      BTechTree*                          getTechTree() { return mpTechTree; }
      const BTechTree*                    getTechTree() const { return mpTechTree; }

      BProtoObjectID                      getBuildingProtoIDToTrainSquad(BProtoSquadID protoSquadID, bool requireInstance) const;
      BProtoObjectIDArray                 getBuildingProtoIDArrayToTrainSquad(BProtoSquadID protoSquadID, bool requireInstance) const;
      BProtoObjectID                      getBuildingProtoIDToResearchTech(BProtoTechID protoTechID, bool requireInstance) const;
      BProtoObjectID                      getBuildingProtoIDToBuildStructure(BProtoObjectID protoObjectID, bool requireInstance) const;

      uint                                getCurrentlyLegalSquadsToTrain(BProtoSquadIDArray& legalSquads) const;
      uint                                getCurrentlyLegalTechsToResearch(BTechIDArray& legalTechs) const;
      uint                                getCurrentlyLegalBuildingsToConstruct(BProtoObjectIDArray& legalBuildings) const;

      // Unit count & future & dead unit count
      void                                addUnitToProtoObject(const BUnit* pUnit, BProtoObjectID protoObjectID);
      void                                removeUnitFromProtoObject(const BUnit* pUnit, BProtoObjectID protoObjectID);
      BProtoObject*                       allocateUniqueProtoObject(const BProtoObject* pBasePO, const BPlayer* pBasePlayer, BEntityID entityID);
      uint                                getUnitsOfType(BObjectTypeID objectTypeID, BEntityIDArray& unitIDs) const;
      uint                                getUnits(BEntityIDArray& unitIDs) const;
      uint                                getNumUnitsOfType(BObjectTypeID objectTypeID) const;
      uint                                getTotalUnitCount() const { return (mTotalUnitCounts); }
      void                                adjustFutureUnitCount(BProtoObjectID protoObjectID, int amount);
      uint                                getFutureUnitCount(BObjectTypeID objectTypeID) const;
      uint                                getTotalFutureUnitCount() const { return (mTotalFutureUnitCounts); }
      void                                adjustDeadUnitCount(BProtoObjectID protoObjectID, int amount);
      uint                                getDeadUnitCount(BObjectTypeID objectTypeID) const;
      uint                                getTotalDeadUnitCount() const { return (mTotalDeadUnitCounts); }
      float                               getTotalCombatValue() const { return (mTotalCombatValue); }

      // Squad count & future & dead squad count.
      void                                addSquadToProtoSquad(const BSquad* pSquad, BProtoSquadID protoSquadID);
      void                                removeSquadFromProtoSquad(const BSquad* pSquad, BProtoSquadID protoSquadID);
      BProtoSquad*                        allocateUniqueProtoSquad(const BProtoSquad* pBasePS, const BPlayer* pBasePlayer, BEntityID squadID );
      uint                                getSquadsOfType(BProtoSquadID protoSquadID, BEntityIDArray& squadIDs) const;
      uint                                getSquads(BEntityIDArray& squadIDs) const;
      uint                                getNumSquadsOfType(BProtoSquadID protoSquadID) const;
      uint                                getTotalSquadCount() const { return (mTotalSquadCounts); }
      void                                adjustFutureSquadCount(BProtoSquadID protoSquadID, int amount);
      uint                                getFutureSquadCount(BProtoSquadID protoSquadID) const;
      uint                                getTotalFutureSquadCount() const { return (mTotalFutureSquadCounts); }
      void                                adjustDeadSquadCount(BProtoSquadID protoSquadID, int amount);
      uint                                getDeadSquadCount(BProtoSquadID protoSquadID) const;
      uint                                getTotalDeadSquadCount() const { return (mTotalDeadSquadCounts); }

      //Squad AI tokens.
      void                                updateSquadAITokens();
      void                                addSquadAISquad(const BSquad *pSquad);
      void                                removeSquadAISquad(const BSquad *pSquad);

      // Goto base tracking. Bases are kept in order based on the time they were built.
      uint                                getNumberGotoBases() const { return mGotoBases.getSize(); }
      BEntityID                           getGotoBase(uint index) const { return mGotoBases[index]; }
      void                                addGotoBase(BEntityID id); //now it uniquely adds
      void                                removeGotoBase(BEntityID id) { mGotoBases.remove(id, true); }
      void                                clearGotoBases() { mGotoBases.clear(); }

      // Weapon types per player
      BWeaponType*                        getWeaponTypeByID(int id) const;
      float                               getDamageModifier(int weaponTypeID, BEntityID targetID, BVector damageDirection, float &shieldedDamageModifier);

      float                               getAbilityRecoverTime(int id) const { return (id<0 || id>=mAbilityRecoverTimes.getNumber() ? 0.0f : mAbilityRecoverTimes[id]); }
      void                                setAbilityRecoverTime(int id, float val) { if (id>=0 && id<mAbilityRecoverTimes.getNumber()) mAbilityRecoverTimes[id]=val; }

      float                               getWeaponPhysicsMultiplier() const { return mWeaponPhysicsMultiplier; }
      void                                setWeaponPhysicsMultiplier(float value) { mWeaponPhysicsMultiplier = value; }

      float                               getAIDamageMultiplier() const { return mAIDamageMultiplier; }
      void                                setAIDamageMultiplier(float value) { mAIDamageMultiplier = value; }
      float                               getAIDamageTakenMultiplier() const { return mAIDamageTakenMultiplier; }
      void                                setAIDamageTakenMultiplier(float value) { mAIDamageTakenMultiplier = value; }
      float                               getAIBuildSpeedMultiplier() const { return mAIBuildSpeedMultiplier; }
      void                                setAIBuildSpeedMultiplier(float value) { mAIBuildSpeedMultiplier = value; }

      // Helper functions for AI
      BUnit* getClosestUnitOfType(BObjectTypeID objectTypeID, BVector location) const;
      BUnit* getAnyUnitOfType(BObjectTypeID objectTypeID) const;
      BUnit* getBuildingToTrainSquad(BProtoSquadID protoSquadID, BVector *pLocation) const;
      BUnit* getBuildingToResearchTech(BProtoTechID protoTechID, BVector *pLocation) const;
      BUnit* getBuildingToBuildStructure(BProtoObjectID protoObjectID, BVector *pLocation, const BEntityIDArray& blockedBuilders) const;
      BUnit* getLeastQueuedBuildingOfType(BProtoObjectIDArray objectTypeIDsD) const;
      void getTrainQueueTimes(float *pTotalTime) const;
      uint getAvailableUniqueTechBuildings(BProtoTechID protoTechID) const;
      BAIFactoidManager* getFactoidManager() { return mpFactoidManager; }
      void setFactoidManager(BAIFactoidManager* pManager) { mpFactoidManager = pManager; }



      // Resource trickle
      float                               getResourceTrickleRate(long resourceID) const { return mResourceTrickleRate.get(resourceID); }
      void                                setResourceTrickleRate(long resourceID, float rate);
      BCost                               getResourceTrickleRate() const { return (mResourceTrickleRate); }
      void                                setResourceTrickleRate(BCost cost);

      // Difficulty
      float                               getDifficulty() const { return (mDifficulty); }
      void                                setDifficulty(float newDiff) { mDifficulty = newDiff; }
      BDifficultyType                     getDifficultyType() const;

      // Player abilities.
      void                                addAbility(long v);
      void                                removeAbility(long v);
      void                                removeAllAbilities();
      bool                                canUseAbility(long v);

      void                                recomputeUnitVisuals(int protoObjectID);

      bool                                getFlagRallyPoint() const { return (mFlagRallyPoint); }
      void                                setFlagRallyPoint(bool v) { mFlagRallyPoint=v; }
      bool                                getFlagBountyResource() const { return (mFlagBountyResource); }
      void                                setFlagBountyResource(bool v) { mFlagBountyResource=v; }
      bool                                getFlagMinimapBlocked() const { return (mFlagMinimapBlocked); }
      void                                setFlagMinimapBlocked(bool v) { mFlagMinimapBlocked=v; }
      bool                                getFlagLeaderPowersBlocked() const { return (mFlagLeaderPowersBlocked); }
      void                                setFlagLeaderPowersBlocked(bool v) { mFlagLeaderPowersBlocked=v; }
//      bool                                getFlagHuman() const { return (mFlagHuman); }
//      void                                setFlagHuman(bool v) { mFlagHuman=v; }      
      bool                                getFlagDefeatedDestroy() const { return mFlagDefeatedDestroy; }
      void                                setFlagDefeatedDestroy(bool v) { mFlagDefeatedDestroy=v; }

      // Skulls
      void                                setFloodPoofPlayer(BPlayerID id) { mFloodPoofPlayer = id; }
      BPlayerID                           getFloodPoofPlayer() const { return mFloodPoofPlayer; }

      BStatsPlayer*                       getStats() const { return mpStats; }

      float                               getTributeCost() const { return mTributeCost; }
      void                                setTributeCost(float val) { mTributeCost=val; }

      const BCost*                        getRepairCost() const { return &mRepairCost; }
      float                               getRepairCost(long r) const { return mRepairCost.get(r); }
      void                                setRepairCost(long r, float v) { mRepairCost.set(r, v); }
      void                                setRepairCost(BCost& cost) { mRepairCost=cost; }
      float                               getRepairTime() const { return mRepairTime; }
      void                                setRepairTime(float v) { mRepairTime=v; }

      float                               getShieldRegenRate() const { return mShieldRegenRate; }
      void                                setShieldRegenRate(float val) { mShieldRegenRate=val; }

      DWORD                               getShieldRegenDelay() const { return mShieldRegenDelay; }
      void                                setShieldRegenDelay(DWORD val) { mShieldRegenDelay=val; }

      bool                                getHaveLeaderPower() const;
      float                               getLeaderPowerChargeCost(int protoObjectID=-1) const;
      uint                                getLeaderPowerChargeCostCount(int protoObjectID) const;
      uint                                getLeaderPowerChargeCount() const;
      float                               getLeaderPowerChargePercent() const;

      void                                recalculateMaxForProtoSquad(BProtoSquad* pProtoSquad);
      void                                recalculateMaxHPForProtoSquad(BProtoSquad* pProtoSquad);
      void                                recalculateMaxSPForProtoSquad(BProtoSquad* pProtoSquad);
      void                                recalculateMaxAmmoForProtoSquad(BProtoSquad* pProtoSquad);
      void                                recalculateMaxForProtoSquad(BProtoSquadID protoSquadID)     { recalculateMaxForProtoSquad(getProtoSquad(protoSquadID)); }
      void                                recalculateMaxHPForProtoSquad(BProtoSquadID protoSquadID)   { recalculateMaxHPForProtoSquad(getProtoSquad(protoSquadID)); }
      void                                recalculateMaxSPForProtoSquad(BProtoSquadID protoSquadID)   { recalculateMaxSPForProtoSquad(getProtoSquad(protoSquadID)); }
      void                                recalculateMaxAmmoForProtoSquad(BProtoSquadID protoSquadID) { recalculateMaxAmmoForProtoSquad(getProtoSquad(protoSquadID)); }

      void                                recalculateMaxForProtoSquads();
      void                                recalculateMaxHPForProtoSquads();
      void                                recalculateMaxSPForProtoSquads();
      void                                recalculateMaxAmmoForProtoSquads();

      DWORD                               getPowerRechargeTime(BProtoPowerID protoPowerID) const { return (protoPowerID>=0 && protoPowerID<mPowerRechargeTime.getNumber() ? mPowerRechargeTime[protoPowerID] : 0); }
      void                                setPowerRechargeTime(BProtoPowerID protoPowerID, DWORD time) { if (protoPowerID>=0 && protoPowerID<mPowerRechargeTime.getNumber()) mPowerRechargeTime[protoPowerID]=time; }
      int                                 getPowerUseLimit(BProtoPowerID protoPowerID) const { return (protoPowerID>=0 && protoPowerID<mPowerUseLimit.getNumber() ? mPowerUseLimit[protoPowerID] : 0); }
      void                                setPowerUseLimit(BProtoPowerID protoPowerID, int limit) { if (protoPowerID>=0 && protoPowerID<mPowerUseLimit.getNumber()) mPowerUseLimit[protoPowerID]=limit; }
      BPowerLevel                         getPowerLevel(BProtoPowerID protoPowerID) const { return (protoPowerID>=0 && protoPowerID<mPowerLevel.getNumber() ? mPowerLevel[protoPowerID] : 0); }
      void                                setPowerLevel(BProtoPowerID protoPowerID, BPowerLevel level) { if (protoPowerID>=0 && protoPowerID<mPowerLevel.getNumber()) mPowerLevel[protoPowerID]=level; }
      
      void                                setPowerAvailableTime(BProtoPowerID protoPowerID, DWORD timeToWait);
      DWORD                               getPowerAvailableTime(BProtoPowerID protoPowerID) const { return (protoPowerID>=0 && protoPowerID<mPowerAvailableTime.getNumber()) ? (mPowerAvailableTime[protoPowerID]) : 0; }

      BHumanPlayerAITrackingData*         getTrackingData() const { return mpTrackingData; }
      void                                setTrackingData(BHumanPlayerAITrackingData* pTrackingData);

      BHintEngine*                        getHintEngine() const { return mpHintEngine; }

      BEntityID                           getLeaderUnit() const { return mLeaderUnit; }

      // rank is only used in Xbox LIVE games
      uint16                              getRank() const { return mRank; }
      void                                setRank(uint16 value) { mRank = value; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      void                                updateResourceTrickle(float elapsedTime);

      // Power related
      void                                updatePowerRecharge();
      bool                                passesAllPowerTechPrereqs(long protoPowerID) const;
      void                                updateSupportPowerStatus();
      
      BVector                             mLookAtPos;

      BSimString                          mName;                  // 8 bytes
      BUString                            mLocalisedDisplayName;  // 8 bytes

      BRallyPoint                         mRallyPoint;            // 16 bytes

      BStatsPlayer*                       mpStats;

      BPlayerProtoObjectArray             mProtoObjects;          // 12 bytes
      BPlayerProtoSquadArray              mProtoSquads;           // 12 bytes
      BPlayerProtoTechArray               mProtoTechs;            // 12 bytes
      BPlayerUniqueProtoObjectArray       mUniqueProtoObjects;    // 12 bytes
      BPlayerUniqueProtoSquadArray        mUniqueProtoSquads;     // 12 bytes

      BSmallDynamicSimArray<BPowerEntry>  mPowerEntries;          // 8 bytes
      BSmallDynamicSimArray<long>         mAbilities;             // 8 bytes
      BSmallDynamicSimArray<BYTE>         mSupportPowerStatus;    // 8 bytes
      BSmallDynamicSimArray<DWORD>        mPowerRechargeTime;     // 8 bytes
      BSmallDynamicSimArray<int>          mPowerUseLimit;         // 8 bytes
      BSmallDynamicSimArray<BPowerLevel>  mPowerLevel;            // 8 bytes
      BSmallDynamicSimArray<DWORD>        mPowerAvailableTime;    // 8 bytes
      BCost                               mResources;             // 8 bytes
      float*                              mpRateAmounts;          // 4 bytes
      float*                              mpRateMultipliers;      // 4 bytes
      BCost                               mTotalResources;        // 8 bytes
      BCost                               mResourceTrickleRate;   // 8 bytes

      XUID                                mXUID;                  // 64 bytes

      BUser*                              mpUser;                 // 4 bytes
      BPlayerPop*                         mpPops;                 // 4 bytes
      BAIFactoidManager*                  mpFactoidManager;       // 4 bytes  For human players only, manages incoming AI 'factoid' chats.
      BAlertManager*                      mpAlertManager;         // 4 bytes
      BHumanPlayerAITrackingData*         mpTrackingData;         // 4 bytes
      BHintEngine*                        mpHintEngine;           // 4 bytes

      BEntityID                           mLeaderUnit;            // 4 bytes  Keeps track of the leader unit for coveneant, invalid ID if no leader currently exists

      BEntityIDArrayArray                 mUnitsByProtoObject;       // 8 bytes
      BEntityIDArray                      mUnits;                    // 8 bytes
      BEntityIDArrayArray                 mSquadsByProtoSquad;       // 8 bytes
      BEntityIDArray                      mSquads;                   // 8 bytes
      BSmallDynamicSimArray<uint32>       mFutureUnitCounts;         // 8 bytes
      BSmallDynamicSimArray<uint32>       mFutureSquadCounts;        // 8 bytes
      BSmallDynamicSimArray<uint32>       mDeadUnitCounts;           // 8 bytes
      BSmallDynamicSimArray<uint32>       mDeadSquadCounts;          // 8 bytes
      BEntityIDArray                      mSquadAISquads;            // 8 bytes
      uint32                              mTotalUnitCounts;          // 4 bytes
      uint32                              mTotalSquadCounts;         // 4 bytes
      uint32                              mTotalFutureUnitCounts;    // 4 bytes
      uint32                              mTotalFutureSquadCounts;   // 4 bytes
      uint32                              mTotalDeadUnitCounts;      // 4 bytes
      uint32                              mTotalDeadSquadCounts;     // 4 bytes

      BSmallDynamicSimArray<BEntityID>    mGotoBases;             // 8 bytes
      BSmallDynamicSimArray<BWeaponType*> mWeaponTypes;           // 8 bytes
      BSmallDynamicSimArray<float>        mAbilityRecoverTimes;

      BTechTree*                          mpTechTree;             // 4 bytes

      long                                mSquadAISearchIndex;    // 4 bytes
      long                                mSquadAIWorkIndex;      // 4 bytes
      long                                mSquadAISecondaryTurretScanIndex; // 4 bytes

      long                                mMPID;                  // 4 bytes
      long                                mColorIndex;            // 4 bytes
      BPlayerID                           mID;                    // 4 bytes
      BPlayerID                           mCoopID;                // 4 bytes
      BPlayerID                           mScenarioID;            // 4 bytes
      long                                mCivID;                 // 4 bytes
      BTeamID                             mTeamID;                // 4 bytes
      BPlayerState                        mPlayerState;           // 4 bytes
      BPlayerNetState                     mNetState;              // 4 bytes
      long                                mLeaderID;              // 4 bytes
      long                                mPopCount;              // 4 bytes
      long                                mBountyResource;        // 4 bytes
      BEntityID                           mRallyObject;           // 4 bytes
      int32                               mStrength;              // 4 bytes
      float                               mTributeCost;           // 4 bytes
      BCost                               mRepairCost;            // 8 bytes
      float                               mRepairTime;            // 4 bytes
      float                               mHandicapMultiplier;    // 4 bytes
      float                               mShieldRegenRate;       // 4 bytes
      DWORD                               mShieldRegenDelay;      // 4 bytes
      float                               mTotalCombatValue;      // 4 bytes
      float                               mDifficulty;            // 4 bytes
      uint32                              mGamePlayedTime;        // 4 bytes - this is set when the player's game is over, rounded to nearest second
      BPlayerID                           mFloodPoofPlayer;       // 4 bytes - this is used by the "sickness" skull to get player units to convert to flood units upon death
      float                               mWeaponPhysicsMultiplier; // 4 bytes
      float                               mAIDamageMultiplier;    // 4 bytes
      float                               mAIDamageTakenMultiplier; // 4 bytes
      float                               mAIBuildSpeedMultiplier; // 4 bytes
      uint16                              mRank;                  // 2 bytes
      int8                                mPlayerType;            // 1 byte
      int8                                mSquadSearchAttempts;   // 1 byte
      UTBitVector<8>                      mActiveResources;       // 1 byte
      bool                                mFlagRallyPoint:1;      // 1 byte
      bool                                mFlagBountyResource:1;  //
      bool                                mFlagMinimapBlocked:1;  //
      bool                                mFlagLeaderPowersBlocked:1; //
//      bool                                mFlagHuman:1;           // 
      bool                                mFlagDefeatedDestroy:1; //
      bool                                mFlagProcessedGameEnd:1;
};
