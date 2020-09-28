//==============================================================================
// ai.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aimission.h"
#include "aiattackmission.h"
#include "aidefendmission.h"
#include "aipowermission.h"
#include "aimissiongroup.h"
#include "aimissiontarget.h"
#include "aitopic.h"
#include "alert.h"
#include "piecewise.h"
#include "SimTarget.h"
#include "gamefilemacros.h"


// forward declarations
class BBidManager;
class BDesignSphere;
class BKB;
class BPlayer;
class BTeam;


//==============================================================================
// Helpful container class.
//==============================================================================
class BAIDifficultySettingDefault
{
public:
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BSimString mName;
   float mValue;
};

typedef BSmallDynamicSimArray<BAIDifficultySettingDefault> BAIDifficultySettingDefaultArray;


//==============================================================================
//==============================================================================
class BAIDifficultySetting
{
public:
   BAIDifficultySetting();
   ~BAIDifficultySetting();

   const BSimString& getName() const { return (mName); }
   const BSimString& getInputLabel() const { return (mInputLabel); }
   const BSimString& getOutputLabel() const { return (mOutputLabel); }
   const BSimString& getComment() const { return (mComment); }

   void setName(const BSimString& v) { mName = v; }
   void setInputLabel(const BSimString& v) { mInputLabel = v; }
   void setOutputLabel(const BSimString& v) { mOutputLabel = v; }
   void setComment(const BSimString& v) { mComment = v; }

   const BPiecewiseFunc& getFunc() const { return (mFunc); }
   BPiecewiseFunc& getFunc() { return (mFunc); }

   void reset();
   bool loadFromXML(const BXMLNode &aiDifficultySettingNode);
   bool canGetDefaultValueByName(const BSimString& defaultName, float& defaultValue) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BSimString mName;
   BSimString mInputLabel;
   BSimString mOutputLabel;
   BSimString mComment;

   BPiecewiseFunc mFunc;
   BAIDifficultySettingDefaultArray mDefaults;
};


//==============================================================================
//==============================================================================
class BAIGlobals
{
public:

   BAIGlobals();

   BVector getScoringStartLoc() const { BASSERT(mbScoringStartLoc); return (mScoringStartLoc); }
   const BEntityIDArray& getScoringSquadList() const { BASSERT(mbScoringSquadList); return (mScoringSquadList); }
   bool getOKToAttack() const { return (mbOKToAttack); }
   bool getOKToDefend() const { return (mbOKToDefend); }
   float getBiasOffense() const { return (mBiasOffense); }
   float getBiasNodes() const { return (mBiasNodes); }
   float getBiasMainBases() const { return (mBiasMainBases); }
   BPlayerID getHatedPlayer() const { return (mHatedPlayer); }
   float getScoringMinDistance() const { return (mScoringMinDistance); }
   float getScoringMaxDistance() const { return (mScoringMaxDistance); }
   DWORD getTargetRecalculatePeriod() const { return (mTargetRecalculatePeriod); }
   DWORD getMaxStalenessToTargetUnit() const { return (mMaxStalenessToTargetUnit); }
   float getSquadAcquireRadiusSqr() const { return (mSquadAcquireRadiusSqr); }
   uint getMaxTargetsToShiftQueue() const { return (mMaxTargetsToShiftQueue); }
   float getRangePenaltyRate() const { return (mRangePenaltyRate); }
   float getAttackMissionScoreMultiplier() const { return (mAttackMissionScoreMultiplier); }
   float getDefendMissionScoreMultiplier() const { return (mDefendMissionScoreMultiplier); }
   const BAIMissionIDArray& getOpportunityRequests() const { return (mOpportunityRequests); }
   BAIMissionIDArray& getOpportunityRequests() { return (mOpportunityRequests); }
   const BAIMissionIDArray& getTerminalMissions() const { return (mTerminalMissions); }
   BAIMissionIDArray& getTerminalMissions() { return (mTerminalMissions); }
   float getPlayerModifier(BPlayerID playerID);
   void setPlayerModifier(BPlayerID playerID, float modifier);
   void setWinRange(bool useMinWin, float minWin, bool useMaxWin, float maxWin){ mbUseMinWin = useMinWin; mMinWin = minWin; mbUseMaxWin = useMaxWin; mMaxWin = maxWin;}
   bool getUseMinWin(){return mbUseMinWin;}
   bool getUseMaxWin(){return mbUseMaxWin;}
   float getMinWin(){return mMinWin;}
   float getMaxWin(){return mMaxWin;}

   void setScoringStartLoc(BVector v) { mScoringStartLoc = v; mbScoringStartLoc = true; }
   void setScoringSquadList(const BEntityIDArray& v) { mScoringSquadList = v; mbScoringSquadList = true; }
   void setOKToAttack(bool v) { mbOKToAttack = v; }
   void setOKToDefend(bool v) { mbOKToDefend = v; }
   void setBiasOffense(float v) { mBiasOffense = v; }
   void setBiasNodes(float v) { mBiasNodes = v; }
   void setBiasMainBases(float v) { mBiasMainBases = v; }
   void setHatedPlayer(BPlayerID v) { mHatedPlayer = v; }
   void setScoringMinDistance(float v) { mScoringMinDistance = v; }
   void setScoringMaxDistance(float v) { mScoringMaxDistance = v; }
   void setTargetRecalculatePeriod(DWORD v) { mTargetRecalculatePeriod = v; }
   void setMaxStalenessToTargetUnit(DWORD v) { mMaxStalenessToTargetUnit = v; }

   void setSquadAcquireRadiusSqr(float v) { mSquadAcquireRadiusSqr = v; }
   void setMaxTargetsToShiftQueue(uint v) { mMaxTargetsToShiftQueue = v; }
   void setRangePenaltyRate(float v) { mRangePenaltyRate = v; }
   void setAttackMissionScoreMultiplier(float v) { mAttackMissionScoreMultiplier = v; }
   void setDefendMissionScoreMultiplier(float v) { mDefendMissionScoreMultiplier = v; }
   void addOpportunityRequest(BAIMissionID v) { mOpportunityRequests.uniqueAdd(v); }
   void removeOpportunityRequest(BAIMissionID v) { mOpportunityRequests.remove(v); }
   void clearOpportunityRequests() { mOpportunityRequests.resize(0); }
   void addTerminalMission(BAIMissionID v) { mTerminalMissions.uniqueAdd(v); }
   void removeTerminalMission(BAIMissionID v) { mTerminalMissions.remove(v); }
   void clearTerminalMissions() { mTerminalMissions.resize(0); }

   bool useScoringStartLoc() const { return (mbScoringStartLoc); }

   void clearScoringParms() { mbScoringSquadList = false; mbScoringStartLoc = false; }
   void initBiases() { mBiasOffense = 0.0f; mBiasNodes = 0.0f; mBiasMainBases = 0.0f; mHatedPlayer = cInvalidPlayerID; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector mScoringStartLoc;
   BEntityIDArray mScoringSquadList;

   BAIMissionIDArray mOpportunityRequests;  // Missions requesting the script to check for possible opportunities.
   BAIMissionIDArray mTerminalMissions;     // Missions that are succeeded, failed, invalid, etc...

   float mBiasOffense;     // Range -1.0f to 1.0f  Positive increases score of attack targets, depresses defend targets
   float mBiasNodes;       // Range -1.0f to 1.0f  Positive value increases score of node (attack or defend) targets
   float mBiasMainBases;   // Range -1.0f to 1.0f  Positive value increases score of primary base (attack or defend) targets.
   BPlayerID mHatedPlayer; // All targets owned by this player are more attractive.

   float mScoringMinDistance;             // In calculating mission target "inst" scores, use
   float mScoringMaxDistance;             // these as the min and max distances.
   DWORD mTargetRecalculatePeriod;        // Only recalculate potential targets every 5 seconds...
   DWORD mMaxStalenessToTargetUnit;       // How stale can a unit be to be considered a target.
   float mSquadAcquireRadiusSqr;          // The radius (squared) at which we will acquire a squad.
   uint mMaxTargetsToShiftQueue;          // How many targets can we queue at a time for a group.
   float mRangePenaltyRate;               // How much do we penalize for being too far out of range.
   float mAttackMissionScoreMultiplier;   
   float mDefendMissionScoreMultiplier;
   float mPlayerModifiers[cMaximumSupportedPlayers];  // 1.0 is default, multiplies value of all target assets.
   float mMinWin;
   float mMaxWin;

   bool mbScoringSquadList : 1;
   bool mbScoringStartLoc  : 1;
   bool mbOKToAttack       : 1;
   bool mbOKToDefend       : 1;
   bool mbUseMinWin        : 1;
   bool mbUseMaxWin        : 1;
};


//==============================================================================
// class BAI
// Contains all the update routines / data for the engine-side AI
//==============================================================================
class BAI
{
public:
   BAI(BPlayerID playerID, BTeamID teamID);
   BAI() {};
   ~BAI();
   void update();

   bool getPlayerAdaptiveDifficultyValue(float& rPlayerDifficultyValue);

   // Mission target stuff.
   const BAIMissionTargetIDArray& getMissionTargetIDs() const { return (mMissionTargetIDs); }
   void scoreMissionTarget(BAIMissionTargetID missionTargetID);
   float scoreMissionTarget(BAIMissionTargetID missionTargetID, BAIMissionScore& score, bool updateTargets);
   const BAIMissionTarget* scoreAllMissionTargets();

   uint getMissionIDsByType(BAIMissionType missionType, BAIMissionIDArray& missionIDs) const;
   const BAIMissionIDArray& getMissionIDs() const { return (mMissionIDs); }
   const BAITopicIDArray& getTopicIDs() const { return (mTopicIDs); }
   uint getNumActiveTopics() const;
   uint getNumActiveMissions() const;



   // Get accessors
   BAIScoreModifier* getScoreModifier() { return (&mScoreModifier); }
   BAIPlayerModifier* getPlayerModifier() { return (&mPlayerModifier); }

   // Mission target stuff.
   void generateBaseMissionTarget(BKBBase* pKBBase);
   void generateDesignSphereMissionTarget(BDesignSphere* pDesignSphere);
      
   // Bid Manager
   BBidManager* getBidManager() { return (mpBidManager); }
   const BBidManager* getBidManager() const { return (mpBidManager); }

   // AI Globals
   BAIGlobals* getAIGlobals() { return (mpAIGlobals); }
   const BAIGlobals* getAIGlobals() const { return (mpAIGlobals); }

   void addMissionID(BAIMissionID missionID) { mMissionIDs.add(missionID); }
   void removeMissionID(BAIMissionID missionID) { mMissionIDs.remove(missionID); }
   bool containsMissionID(BAIMissionID missionID) { return (mMissionIDs.contains(missionID)); }
   void addTargetID(BAIMissionTargetID targetID) { mMissionTargetIDs.add(targetID); }
   void removeTargetID(BAIMissionTargetID targetID) { mMissionTargetIDs.remove(targetID); }
   bool containsTargetID(BAIMissionTargetID targetID) { return (mMissionTargetIDs.contains(targetID)); }
   void addTopicID(BAITopicID topicID) { mTopicIDs.add(topicID); }
   void removeTopicID(BAITopicID topicID) { mTopicIDs.remove(topicID); }
   bool containsTopicID(BAITopicID topicID) { return (mTopicIDs.contains(topicID)); }

   float getAllyAssetValueInArea(BVector position, float radius, bool requireVis, bool selfAsAlly) const;
   float getEnemyAssetValueInArea(BVector position, float radius, bool requireVis) const;
   void getAllyKBSquadsInArea(BVector position, float radius, bool requireVis, bool selfAsAlly, BKBSquadIDArray& results) const;
   void getEnemyKBSquadsInArea(BVector position, float radius, bool requireVis, BKBSquadIDArray& results) const;

   static void calculateSquadAnalysis(const BEntityIDArray& squadIDs, BAISquadAnalysis& squadAnalysis);
   static void calculateSquadAnalysis(const BKBSquadIDArray& kbSquadIDs, BAISquadAnalysis& squadAnalysis);
   static void calculateSquadAnalysis(const BProtoSquadIDArray& protoSquadIDArray, BPlayerID playerID, BAISquadAnalysis& squadAnalysis);
   static float calculateOffenseAToB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB);
   static float calculateOffenseRatioAToB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB);
   static float calculateUnopposedTimeToKill(const BAISquadAnalysis& attackers, const BAISquadAnalysis& targets, float maxTimeLimit);
   static bool canAHurtB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB);

   bool getFlagAIQ() const { return (mFlagAIQ); }
   void setFlagAIQ(bool v) { mFlagAIQ = v; }
   bool getFlagPaused() const { return (mFlagPaused); }
   void setFlagPaused(bool v) { mFlagPaused = v; }
   void toggleFlagPaused() { mFlagPaused = !mFlagPaused; }

   // Difficulty settings stuff
   bool loadAIDifficultySettings();
   void recalculateDifficultySettings(float difficulty);
   const BAIDifficultySetting* getDifficultySettingByName(const BSimString& name) const;
   BAIDifficultySetting* getDifficultySettingByName(const BSimString& name);

   DWORD getMaxMissionFocusTime() const { return (mMaxMissionFocusTime); }
   DWORD getThinkCostMin() const;
   DWORD getThinkCostMax() const;
   DWORD getActCostMin() const;
   DWORD getActCostMax() const;
   DWORD getPowerEvalIntervalMin() const;
   DWORD getPowerEvalIntervalMax() const;

   void setMaxMissionFocusTime(DWORD v) { mMaxMissionFocusTime = v; }
   void setThinkCostMin(DWORD v) { mThinkCostMin = v; }
   void setThinkCostMax(DWORD v) { mThinkCostMax = v; }
   void setActCostMin(DWORD v) { mActCostMin = v; }
   void setActCostMax(DWORD v) { mActCostMax = v; }
   void setPowerEvalIntervalMin(DWORD v) { mPowerEvalIntervalMin = v; }
   void setPowerEvalIntervalMax(DWORD v) { mPowerEvalIntervalMax = v; }


   #ifndef BUILD_FINAL
   void debugRenderMissionTargets();
   void debugRender();
   #endif

   
   #ifdef BUILD_FINAL   
   inline void aiLog(const char *lpszFormat, ...){lpszFormat;}
   #else
   void aiLog(const char *lpszFormat, ...);
   #endif

   // AIQ Specific stuff.
   void AIQSetTopic(BAITopicID newTopic);
   BAITopicID AIQGetTopic() const { return (mAIQTopic); }
   BTriggerScriptID getAITriggerScriptID() const { return (mAITriggerScriptID); }
   void AIQUpdateLottoTickets(DWORD currentGameTime);
   void AIQTopicLotto(DWORD currentGameTime);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   // Scratch variables for mission target scoring
   float mScoringFriendlyAssets;
   float mScoringEnemyAssets;
   float mScoringAllyForce;
   float mScoringEnemyForce;
   float mScoringMyForce;
   float mScoringDistance;

protected:

   void initializeAIScript();

   // Lotto stuff
   BAIGlobals* mpAIGlobals;                                 // AI Globals
   BBidManager* mpBidManager;                               // already in a manager!

   //BAbilityID mAbilityIDs[AIAbility::cNumAIAbilities];

   BAIMissionTargetIDArray mMissionTargetIDs;
   BAIMissionIDArray mMissionIDs;
   BAITopicIDArray mTopicIDs;
   BAIScoreModifier mScoreModifier;    // Used to store squad modifiers in calculating asset value
   BAIPlayerModifier mPlayerModifier;


   BAITopicID mAIQTopic;
   BTriggerScriptID mAITriggerScriptID;


   // AI Difficulty Settings
   BSmallDynamicSimArray<BAIDifficultySetting> mDifficultySettings;
   DWORD mMaxMissionFocusTime;
   DWORD mThinkCostMin;
   DWORD mThinkCostMax;
   DWORD mActCostMin;
   DWORD mActCostMax;
   DWORD mPowerEvalIntervalMin;
   DWORD mPowerEvalIntervalMax;

   BPlayerID mPlayerID;
   BTeamID mTeamID;

   bool mFlagAIQ : 1;   // Does the AI operate with AIQ or is this just here so we can update scenario missions?
   bool mFlagPaused : 1; // are we paused?

   #ifndef BUILD_FINAL
      long mLog;        // XFS log file for this AI
   #endif
};


//============================================================================
// class BMissionTargetSortFunctor
//============================================================================
class BMissionTargetScoreSortFunctor
{
   public:
      bool operator() (const BAIMissionTargetID leftTargetID, const BAIMissionTargetID rightTargetID) const;
};