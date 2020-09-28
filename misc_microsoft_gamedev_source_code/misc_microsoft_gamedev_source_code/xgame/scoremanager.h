//==============================================================================
// scoreManager.h
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once

#include "gamefilemacros.h"

#define SCORE_NUM_GRADES 3

//============================================================================
//BScorePlayer
//============================================================================
class BScorePlayer
{
   public:
      BScorePlayer();
      ~BScorePlayer();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

     long mPlayerID;
      DWORD mLastUpdateTime;
      bool mFinalScoreCalculated;
      bool mCampaignScoreImproved;

      long mSquadsKilled;
      long mSquadsLost;
      float mCombatValueKilled;
      float mCombatValueLost;
      float mSkullBonusCombatValue;
      float mSkullBonusObjectives;

      DWORD mMissionCompletionTime;
      int mNumPrimaryObjectives;
      int mNumPrimaryObjectivesCompleted;
      int mNumOptionalObjectives;
      int mNumOptionalObjectivesCompleted;

      float mTimeBonus;
      float mCombatBonus;
      float mMPBonus; // does not need to be saved, only for active MP games

      long mObjectiveScore;
      long mBaseScore;

      long mFinalObjectiveScore;
      long mFinalCombatScore;
      long mFinalBaseScore;
      long mFinalCombatBonus;
      long mFinalTimeBonus;
      long mFinalSkullBonus;

      long mFinalScore;
      long mFinalGrade;
};

//============================================================================
//BScoreScenario
//============================================================================
class BScoreScenario
{
   public:
      BScoreScenario();
      ~BScoreScenario();

      void reset();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      //Note: Objectives are stored in the Objective Manager
      long mScenarioID;

      float mCombatBonusMin;
      float mCombatBonusMax;

      DWORD mMissionMinParTime;
      DWORD mMissionMaxParTime;

      long mScoreGrade[SCORE_NUM_GRADES];
};


//============================================================================
//BScoreManager
//============================================================================
class BScoreManager 
{
   public:
      BScoreManager();
      ~BScoreManager();


      enum
      {
         cGradeNone=0,
         cGradeGold=1,
         cGradeSilver=2,
         cGradeBronze=3,
         cGradeTin=4,
      };

      void reset();

      void initializePlayers();
      bool arePlayersInitialized() {return (mPlayers.getSize()>0);};

      void reportSquadKilled( long playerID, BEntityID targetID, long number=1);
      void reportObjectiveCompleted( bool completed, long value );  //no player needed - this affects all human players
      void reportCampaignScoreImproved(long playerID);
#ifndef BUILD_FINAL
      void reportBogusKill( long playerID );
#endif

      void  resetSkullModifier() { mSkullModifier = 1.0f; }
      void  addSkullModifier(float value) { mSkullModifier *= value; }
      float getSkullModifier() { return mSkullModifier; }

      long  getBaseScore(long playerID);
      float getCombatBonusPercent(long playerID);
      //float getTimeBonusPercent(long playerID);

      //long getFinalObjectiveScore(long playerID);
      //long getFinalCombatScore(long playerID);
      //long getFinalBaseScore(long playerID);
      //long getFinalCombatBonus(long playerID);
      //long getFinalTimeBonus(long playerID);
      //long getFinalSkullBonus(long playerID);
      long getFinalScore(long playerID);
      long getFinalGrade(long playerID);
      bool getFinalScoreValues(long playerID, long &objScore, long &combatScore, long &baseScore, long &combatBonus, float &combatBonusPercent, 
                               long &timeBonus, float &timeBonusPercent, long &skullBonus, long &finalScore, long &finalGrade);

      DWORD getCompletionTime(long playerID);
      long  getHighestFinalScore();
      int   getOptionalObjectivesTotal(long playerID);
      int   getOptionalObjectivesCompleted(long playerID);
      bool  wasCampaignScoreImproved(long playerID);

      long  getScoreRequiredForGrade(long grade);
      const BUString &getStringForGrade(long grade);
      long  getStringIDForGrade(long grade);

      void  setScenarioData( long scenarioID, float combatBonusMin, float combatBonusMax, DWORD missionMinParTime, DWORD missionMaxParTime, long gradeA, long gradeB, long gradeC, long gradeD );
      const BScoreScenario& getScenarioData() const { return mCurrentScenarioData; }

      const void queryPlayers(BSmallDynamicSimArray<BScorePlayer>& players);
      BScorePlayer * getPlayerByID(long id);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      void computeFinalScores(bool force=false);

   protected:


      void update(long playerID, bool force=false);

      BSmallDynamicSimArray<BScorePlayer> mPlayers;
      BScoreScenario                      mCurrentScenarioData;
      long                                mGradeToStringMap[SCORE_NUM_GRADES+2];  // [10/20/2008 xemu] ugh.  3 "real" medals, + none + tin. 
      float                               mSkullModifier;
      bool                                mScenarioDataSet;
      bool                                mFinalScoresCalculated;

};

extern BScoreManager gScoreManager;
