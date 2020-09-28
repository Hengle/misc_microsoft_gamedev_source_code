//==============================================================================
// aimission.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"
#include "aimissiongroup.h"
#include "aimissionscore.h"
#include "aimissionconditions.h"
#include "aimissiontarget.h"
#include "aimissiontargetwrapper.h"
#include "bid.h"
#include "powertypes.h"
#include "gamefilemacros.h"

// forward declarations
class BAI;
class BKB;
class BPlayer;
class BSquad;
class BTeam;
class BAITopic;
class BAIScoreModifier;
class BAIPlayerModifier;
class BProtoPower;


// Typedefs
typedef std::pair<BObjectTypeID, float> BScoreMultPair;
typedef BSmallDynamicSimArray<BScoreMultPair> BScoreMultPairArray;
typedef std::pair<BPlayerID, float> BPlayerMultPair;
typedef BSmallDynamicSimArray<BPlayerMultPair> BPlayerMultPairArray;

//==============================================================================
//==============================================================================
class BAIScoreModifier
{
public:
   BAIScoreModifier(){};
   float GetScoreModifier(const BSquad *pSquad, float defaultResult = 1.0f) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BScoreMultPairArray mMultipliers;
};


//==============================================================================
//==============================================================================
class BAIPlayerModifier
{
public:
   BAIPlayerModifier(){};
   float GetPlayerModifier(BPlayerID playerID, float defaultResult = 1.0f) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BPlayerMultPairArray mMultipliers;
};


//==============================================================================
// Container class.
//==============================================================================
class BSquadGroupPair
{
public:
   BSquadGroupPair();
   BSquadGroupPair(BEntityID squadID);
   BSquadGroupPair(BEntityID squadID, BAIGroupID groupID);
   BEntityID getSquadID() const { return (mSquadID); }
   BAIGroupID getGroupID() const { return (mGroupID); }
   void set(BEntityID squadID, BAIGroupID groupID) { mSquadID = squadID; mGroupID = groupID; }
   void setSquadID(BEntityID v) { mSquadID = v; }
   void setGroupID(BAIGroupID v) { mGroupID = v; }

protected:
   BEntityID mSquadID;
   BAIGroupID mGroupID;
};

typedef BSmallDynamicSimArray<BSquadGroupPair> BSquadGroupPairArray;


//==============================================================================
//==============================================================================
class BAIMissionCache
{
public:
   BAIMissionCache(){}
   ~BAIMissionCache(){}

   void clear();

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   // Mission cache.
   BKBSquadIDArray mWorkDistGatherables;        // Gatherable within the work dist.
   BKBSquadIDArray mWorkDistEnemies;            // All within work dist.
   BKBSquadIDArray mWorkDistGarrisonedEnemies;  // All garrisoned within work dist
   BKBSquadIDArray mWorkDistVisibleEnemies;     // Visible within work dist. 
   BKBSquadIDArray mWorkDistDoppledEnemies;     // Last known pos within work dist.
   BKBSquadIDArray mWorkDistAllies;             // All within work dist.
   BKBSquadIDArray mSecureDistEnemies;          // All within secure dist.
   BKBSquadIDArray mSecureDistAllies;           // All within secure dist.
   BKBSquadIDArray mHotZoneDistEnemies;         // All within hot zone dist.
   BKBSquadIDArray mHotZoneDistAllies;          // All within hot zone dist.
   BKBSquadIDArray mHotZoneDistSelf;            // All within hot zone dist.
   BAIGroupIDArray mGathererGroups;             // My groups who can gather.
   BAIGroupIDArray mGroupsInsideLeash;          // My groups who are inside leash dist.
   BAIGroupIDArray mGroupsOutsideLeash;         // My groups who are outside leash dist.
   BAIGroupIDArray mGroupsInsideRally;          // My groups who are inside rally dist.
   BAIGroupIDArray mGroupsOutsideRally;         // My groups who are outside rally dist.
   BAIGroupIDArray mGroupsMeleeSquads;          // My groups who are melee squads.
   BAITeleporterZoneIDArray mTeleportZones;     // Teleport zones that will get us to the target faster.

   DWORD mTimestampWorkDistGatherables;
   DWORD mTimestampWorkDistEnemies;
   DWORD mTimestampWorkDistVisibleEnemies;
   DWORD mTimestampWorkDistDoppledEnemies;
   DWORD mTimestampWorkDistAllies;
   DWORD mTimestampSecureDistEnemies;
   DWORD mTimestampSecureDistAllies;
   DWORD mTimestampHotZoneDistEnemies;
   DWORD mTimestampHotZoneDistAllies;
   DWORD mTimestampHotZoneDistSelf;
   DWORD mTimestampGathererGroups;
   DWORD mTimestampGroupsInsideLeash;
   DWORD mTimestampGroupsOutsideLeash;
   DWORD mTimestampGroupsInsideRally;
   DWORD mTimestampGroupsOutsideRally;
   DWORD mTimestampTeleportZones;
};


//==============================================================================
//==============================================================================
class BAIMission
{
public:
   BAIMission();
   ~BAIMission();

   void toggleActive(DWORD currentGameTime, bool active);
   virtual void resetNonIDData();
   bool isStateTerminal();

   // Get accessors
   const BAIMissionScore& getLaunchScores() const { return (mLaunchScores); }
   BAIMissionID& getID() { return (mID); }
   BAIMissionID getID() const { return (mID); }
   BAITopicID getAITopicID() const { return (mAITopicID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BAIMissionType getType() const { return (mType); }
   void setType(BAIMissionType v) { mType = v; }
   BAIMissionState getState() const { return (mState); }
   DWORD getTimestampActiveToggled() const { return (mTimestampActiveToggled); }
   DWORD getTimestampStateChanged() const { return (mTimestampStateChanged); }
   DWORD getTimestampNextOppDetect() const { return (mTimestampNextOppDetect); }
   DWORD getTimestampNextAct() const { return (mTimestampNextAct); }
   DWORD getTimestampNextThink() const { return (mTimestampNextThink); }

   void payTimeCostAct(DWORD currentGameTime);
   void payTimeCostThink(DWORD currentGameTime);
   void payTimeCostPowerEval(DWORD currentGameTime);
   void payTimeCostODSTEval(DWORD currentGameTime);
   bool canAct(DWORD currentGameTime) const;
   bool canThink(DWORD currentGameTime) const;
   bool canPowerEval(DWORD currentGameTime) const;
   bool canODSTEval(DWORD currentGameTIme) const;
   bool AIQExceededMaxFocusTime(DWORD currentGameTime) const;

   bool getFlagActive() const { return (mFlagActive); }
   bool getFlagMoveAttack() const { return (mFlagMoveAttack); }
   void setFlagMoveAttack(bool v) { mFlagMoveAttack = v; }

   void destroyAllTargetWrappers();
   void reorderTargetWrapper(BAIMissionTargetWrapperID id, uint index);

   void addWrapperID(BAIMissionTargetWrapperID wrapperID) { mTargetWrapperIDs.add(wrapperID); }
   void removeWrapperID(BAIMissionTargetWrapperID wrapperID) { mTargetWrapperIDs.remove(wrapperID); }
   bool containsWrapperID(BAIMissionTargetWrapperID wrapperID) { return (mTargetWrapperIDs.contains(wrapperID)); }

   void addGroupID(BAIGroupID groupID) { mGroups.add(groupID); mWorkingGroups.add(groupID); }
   void removeGroupID(BAIGroupID groupID) { mGroups.remove(groupID); mWorkingGroups.remove(groupID); }
   bool containsGroupID(BAIGroupID groupID) { return (mGroups.contains(groupID)); }
   void deleteAllGroups();

   BAIMissionTargetID getFinalTargetWrapperTargetID() const;

   void setLaunchScores(BAIMissionScore& launchScores) { mLaunchScores = launchScores; }
   void setID(BAIMissionID id) { mID = id; }
   void setID(uint refCount, BAIMissionType type, uint index) { mID.set(refCount, type, index); }
   void setAITopicID(BAITopicID aiTopicID) { mAITopicID = aiTopicID; }
   void setPlayerID(BPlayerID v) { mPlayerID = v; }
   void setState(BAIMissionState state);
   void setFlagEnableOdstDrop(bool v) { mFlagEnableOdstDrop = v; }


   // More helper stuff
   bool isType(BAIMissionType type) const { return (mType == type); }
   bool isState(BAIMissionState state) const { return (mState == state); }

   void addSquads(BEntityIDArray squadsToAdd);
   void addSquad(BEntityID squadToAdd);
   void removeSquads(BEntityIDArray squadsToRemove);
   void removeSquad(BEntityID squadToRemove);
   BSquad* validateSquadForMission(BEntityID squadID);
   void removeInvalidSquads();
   uint getSquadsFromGroups(const BAIGroupIDArray& groupIDs, BEntityIDArray& squadIDs);
   void moveGroupsToPosition(const BAIGroupIDArray& groupIDs, const BVector location);
   void setGroupState(const BAIGroupIDArray& groupIDs, BAIGroupState gs);
   void setGroupRole(const BAIGroupIDArray& groupIDs, BAIGroupRole gr);
   uint getGroupsByState(BAIGroupState gs, BAIGroupIDArray& groupIDs);
   uint getGroupsByRole(BAIGroupRole gr, BAIGroupIDArray& groupIDs);
   BAIScoreModifier* getScoreModifier() { return (&mScoreModifier); }
   BAIPlayerModifier* getPlayerModifier() { return (&mPlayerModifier); }

   uint getSquads(BEntityIDArray& squads) const;
   const BEntityIDArray& getSquads() const { return (mSquads); }
   uint getNumSquads() const;
   const BAIGroupIDArray& getGroups() const { return (mGroups); }
   uint getGroups(BAIGroupIDArray& groupIDs) const;
   uint getNumGroups() const { return (mGroups.getSize()); }
   uint getUngroupedSquads(BEntityIDArray& ungroupedSquads) const;
   uint getNumUngroupedSquads() const;
   
   // Remove squads from a groups in this mission.
   void ungroupSquads(const BEntityIDArray& squadIDs);
   void ungroupSquad(BEntityID squadID);
   
   // Add squads to a group in this mission.
   void groupSquads(const BEntityIDArray& squadIDs, BAIGroupID groupID);
   void groupSquad(BEntityID squadID, BAIGroupID groupID);

   // Group maintenance (processing ungrouped squads into existing or new groups, folding too-small groups together...)
   void processUngroupedSquads();
   void processUngroupedSquadsForExistingGroups();
   void processUngroupedSquadsForNewGroups();
   BAIGroupID findExistingGroupForSquad(BEntityID squadID) const;

   BAIGroup* getMostRecentlyTaskedGroup(const BAIGroupIDArray& groupIDs);
   void getUntaskedThisStateGroups(BAIGroupIDArray& untaskedThisStateGroupIDs) const;

   BAIGroup* getAvailableGroupForProtoSquad(BProtoSquadID protoSquadID);
   BAIGroup* getNextGroupToTask();
   void deleteAllTasks(BAIGroupID groupID);
   void deleteAllTasks(const BAIGroupIDArray& groupIDs);

   void recalculateGroupPositions();
   void recalculateSquadAnalysis();

   void updateGroups(DWORD currentGameTime);

   uint recalcStandardAttackTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcDamageAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcSuicideAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcJumppackAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcOverburnAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcFlashBangAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcStasisAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcRamAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcHijackAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcScoutStaleTargetTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcGatherTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks);
   uint recalcRepairOtherTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks);

   bool recalcBestOrbitalLoc(float orbitalRadius, const BEntityIDArray& targetEnemyUnits, BVector& resultVec);

   const BTaskInfo* getHighestScoringTask(const BAIGroup& group, const BTaskInfoArray& potentialTasks);

   virtual void update(DWORD currentGameTime);
   virtual void updateStateSuccess(DWORD currentGameTime);
   virtual void updateStateFailure(DWORD currentGameTime);
   virtual void updateStateCreate(DWORD currentGameTime);
   virtual void updateStateWorking(DWORD currentGameTime);
   virtual void updateStateWithdraw(DWORD currentGameTime);
   virtual void updateStateRetreat(DWORD currentGameTime);

   void processPossibleOpportunityTargets(DWORD currentGameTime);

   bool processLeashMove(BAIGroup& rGroup, const BAIMissionTarget& rTarget);
   bool processRallyMove(BAIGroup& rGroup, const BAIMissionTarget& rTarget);


   void addBid(BBidID bidID);
   void removeBid(BBidID bidID);

   void bulkGroupSetState(const BAIGroupIDArray& groupIDs, BAIGroupState groupState);
   void taskGroups(const BAIGroupIDArray& groupIDs, const BTaskInfo& taskInfo, bool queueTask = false, bool moveAttack = false);

#ifndef BUILD_FINAL
   BSimString mName;    // Debugging and logging/tracing tool
   uint mDebugID;       // UI Friendly ID for debugging purposes
   void debugRender();
#endif

   bool processStateChangeConditions(DWORD currentGameTime);
   bool processMissionFailedConditions();
   bool processTargetRetreatConditions(const BAIMissionTargetWrapper& wrapper, const BAIMissionTarget& target);
   bool processTargetSuccessConditions(DWORD currentGameTime, const BAIMissionTargetWrapper& wrapper, const BAIMissionTarget& target);

   bool processPowerLaunch(DWORD currentGameTime);
   bool processPowerLaunchCleansing(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchOrbital(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchCarpetBombing(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchCryo(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchRage(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchWave(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchDisruption(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchRepair(BBid& rBid, BProtoPowerID protoPowerID);
   bool processPowerLaunchODST(DWORD currentGameTime);
   BPowerID launchPower(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BEntityID squadID, BVector launchPos, bool bCreateMission);
   virtual bool processMissionTimeout(DWORD currentGameTime);

   const BAIMissionTargetWrapper* getCurrentTargetWrapper() const;
   BAIMissionTargetWrapper* getCurrentTargetWrapper();

   void updateCache(DWORD currentGameTime);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   float getRangeModifier(const BAIGroup& group, const BSquad& targetSquad) const;
   float getMostlyDeadModifier(const BKBSquad& targetSquad) const;

   BAIMissionScore mLaunchScores;
   BEntityIDArray mSquads;
   BEntityIDArray mUngroupedSquads;
   BSquadGroupPairArray mSquadGroupMap;
   BAIGroupIDArray mGroups;
   BAIGroupIDArray mWorkingGroups;
   BAIScoreModifier mScoreModifier;
   BAIPlayerModifier mPlayerModifier;
   BBidIDArray mBidIDs;

   BAIMissionTargetWrapperIDArray mTargetWrapperIDs;

   // Keep track of these in case they change.
   BAIMissionTargetWrapperID mPrevUpdateWrapperID;
   BAIMissionTargetID mPrevUpdateTargetID;

   uint mFailConditionMinSquads; // If the mission as a whole has < than this many squads, it goes to the failure state
   float mManualRetreatPosX;  
   float mManualRetreatPosZ;

   // Mission cache.
   BAIMissionCache mCache;
   
   BAIMissionID mID;
   BAITopicID mAITopicID;
   BPlayerID mPlayerID;
   BAIMissionType mType;
   BAIMissionState mState;

   DWORD mTimestampActiveToggled;
   DWORD mTimestampStateChanged;
   DWORD mTimestampNextOppDetect;
   DWORD mTimestampNextAct;                  // When is the next time we can possibly act.
   DWORD mTimestampNextThink;                // Just throttle the AI thinking a bit...
   DWORD mTimestampNextPowerEval;            // When can we consider launching a power
   DWORD mTimestampNextODSTEval;
   DWORD mTimestampEnemiesCleared;           // Timestamp when we started detecting no enemies in the current target area

   // Flags
   bool mFlagActive                    : 1;
   bool mFlagAllowOpportunityTargets   : 1;
   bool mFlagMoveAttack                : 1;  // If true, issue move attack commands instead of moves.
   bool mFlagEnemiesCleared            : 1; // If true, enemies have been cleared from the current target.
   bool mFlagManualRetreatPos          : 1; // If we have manually specified the retreat pos.
   bool mFlagRepeatTargets             : 1;
   bool mFlagEnableOdstDrop            : 1;  // Does the mission have blanket permission to launch 
};



