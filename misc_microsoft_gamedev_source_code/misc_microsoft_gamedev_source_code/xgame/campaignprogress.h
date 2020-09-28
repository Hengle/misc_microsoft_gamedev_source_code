//==============================================================================
// campaignProgress.h
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once

//If this is changed, the bits required must be changed as well
#define CAMPAIGN_MAX_SCORE 1000000
//#define CAMPAIGN_SCORE_BITS_REQUIRED 20
//#define CAMPAIGN_GRADE_BITS_REQUIRED 4

//Note that this includes all DLCs.  May need to make this dynamic somehow.
#define CAMPAIGN_MAX_SCENARIOS 20

//easy normal heroic legendary
#define CAMPAIGN_DIFFICULTY_LEVELS 4 

//modes - single player, coop
#define CAMPAIGN_MAX_MODES 2

//Limiting the time saved to 15 bits and rounding down to 9 hours (32400 sec)
#define SCENARIO_MAX_TIME 32400

//Forward declarations
class BDynamicStream;
class BInflateStream;
#ifndef BUILD_FINAL
class BDebugTextDisplay;
#endif

//============================================================================
struct BCinematicProgressMapping
{
   long mCinematicNodeID;
   long mProgressIndex;
};

//============================================================================
//BScoreManager
//============================================================================
class BCampaignProgress 
{
   public:
      BCampaignProgress();
      ~BCampaignProgress();

      void enterScore( uint scenarioID, uint mode, uint difficulty, long playerID, uint score, uint grade, uint time, uint partime );
      bool writeToStream( BDynamicStream &stream );
      bool readFromStream( BInflateStream* pInfStream, uint8 version );
      void clear();

      long getCurrentCampaignNode(){ return(mCurrentCampaignNode); }
      void setCurrentCampaignNode(long nodeID);

      bool isScenarioUnlocked(long nodeID);
      bool isCinematicUnlocked(long nodeID);

      long getUnlockedMissionCount() const;
      long getUnlockedMovieCount() const;

      static BCampaignProgress *getCampaignProgress(bool *pShowAll = NULL);

      uint32 getScenarioScore(uint scenarioID, uint mode, uint difficulty);
      uint32 getScenarioMedal(uint scenarioID, uint mode, uint difficulty);
      long   getScenarioMedalStringID(uint scenarioID, uint mode, uint difficulty);
      uint32 getScenarioTime(uint scenarioID, uint mode, uint difficulty);
      bool   getScenarioParMet(uint scenarioID, int mode, uint difficulty);
      uint32 getLifetimeCampaignScore() const { return mLifetimeCampaignScore; } 
      uint32 getLifetimeCampaignScore(uint mode) const;
      uint32 getLifetimeScore(uint mode, uint difficulty) const { return mLifetimeScore[mode][difficulty]; } 

      bool   isScenarioCompleted(uint scenarioID, int mode, int difficulty);
      bool   isScenarioCompleted(uint scenarioID, int mode, int diffStart, int diffStop);
      bool   isScenarioParBeaten(uint scenarioID, int mode, int difficulty);
      bool   isScenarioParBeaten(uint scenarioID, int mode, int diffStart, int diffStop);
      bool   isScenarioMedalEarned(uint medalRequired, uint scenarioID, int mode, int difficulty);
      bool   isScenarioMedalEarned(uint medalRequired, uint scenarioID, int mode, int diffStart, int diffStop);

      bool   isScenarioCompletedByProgressIndex(uint index, int mode, int difficulty);
      long   getNumberOfScenarioNodes() { return mNumberScenarios; }

      void   setupTickerInfo();

      // [7/17/2008 xemu] so the idea here is that both scenarios and cinematics are mapped to a common concept of progress, and these are the ID / index converters 
      long scenarioNodeIDToProgressIndex(long nodeID);
      long progressIndexToScenarioNodeID(long index);
      long cinematicNodeIDToProgressIndex(long nodeID);

      // Mission session ID which is incremented each time a mission is started. Needed to verify save is for last mission played.
      void   incrementSessionID() { mSessionID++; if (mSessionID==0 || mSessionID==UINT16_MAX) mSessionID=1; }
      void   setSessionID(uint16 val) { mSessionID=val; }
      uint16 getSessionID() const { return mSessionID; }

#ifndef BUILD_FINAL
      void dumpInfo() const;
      void displayInfoInStats(BDebugTextDisplay& textDisplay, int page) const;
#endif

   protected:
      void computeVariables();
      void constructNodeIDTable();

      long mCurrentCampaignNode;  //Note that this differs from the index into these arrays as it includes cinematics and scenarios
      uint32 mScenarioScores[CAMPAIGN_MAX_SCENARIOS][CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS];
      uint8  mScenarioGrades[CAMPAIGN_MAX_SCENARIOS][CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS];
      uint16 mScenarioTimes[CAMPAIGN_MAX_SCENARIOS][CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS];
      bool   mScenarioParMet[CAMPAIGN_MAX_SCENARIOS][CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS];
      bool   mScenarioUnlocked[CAMPAIGN_MAX_SCENARIOS];
      long   mScenarioNodeID[CAMPAIGN_MAX_SCENARIOS];
      uint32 mLifetimeScore[CAMPAIGN_MAX_MODES][CAMPAIGN_DIFFICULTY_LEVELS];

      // [7/17/2008 xemu] mapping of cinematics to related lock-control scenarios
      BSmallDynamicSimArray<BCinematicProgressMapping> mCinematicProgressMappings;

      uint32 mLifetimeCampaignScore;
      uint32 mNumberScenarios;

      uint32 mHighestUnlockedScenario;
      uint32 mLowestUnbeatenScenario;
      uint16 mSessionID;
      bool mbNodeIDTableConstructed;
};

