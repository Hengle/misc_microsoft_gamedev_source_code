//==============================================================================
// userprofilemanager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "asynctaskmanager.h"
#include "xbox.h"
#include "cost.h"
#include "statsManager.h"
#include "campaignprogress.h"

// Forward Declarations
class BUser;
class BUserProfileManager;
class BUserAchievementList;

#define SR_MAX_NUM_LEADERS 10
#define SR_MAX_NUM_MAPS 20
#define SR_MAX_NUM_MODES 10
#define SR_MAX_NUM_RANKMODES 2

// Global variable for the one BProfileManager object
extern BUserProfileManager gUserProfileManager;

enum SETTINGS
{
   Setting_Gamer_Achievements = 0,
   Setting_Gamer_Creds,
   Setting_Gamer_Motto,
   Setting_Gamer_Region,
   Setting_Gamer_Reputation,
   Setting_Gamer_Titles_Played,
   Setting_Gamer_Zone,
   Setting_Option_Voice_Muted,
   Setting_Option_Voice_Speakers,
   Setting_Option_Voice_Volume,
   Setting_Game_Difficulty,
   Setting_Auto_Aim,
   Setting_Auto_Center,
   Setting_Thumbstick_Control,
   Setting_YAxis_Inversion,
   Setting_Race_Transmission_Type,
   Setting_Race_Camera_Location,
   Setting_Race_Brake_Control,
   Setting_Race_Accelerator_Control,
   Setting_Title_Achievements,
   Setting_Title_Creds,
   Setting_Controller_Vibration,
   Setting_Gamer_Picture,
   Setting_Title1,
   Setting_Title2,       
//   Setting_Title3, // we are not going to use these right now.
   NUM_SETTINGS
};

// Must match the Setting enumeration above
const DWORD SettingIDs[ NUM_SETTINGS ] =
{
   XPROFILE_GAMERCARD_ACHIEVEMENTS_EARNED,
   XPROFILE_GAMERCARD_CRED,
   XPROFILE_GAMERCARD_MOTTO,
   XPROFILE_GAMERCARD_REGION,
   XPROFILE_GAMERCARD_REP,
   XPROFILE_GAMERCARD_TITLES_PLAYED,
   XPROFILE_GAMERCARD_ZONE,
   XPROFILE_OPTION_VOICE_MUTED,
   XPROFILE_OPTION_VOICE_THRU_SPEAKERS,
   XPROFILE_OPTION_VOICE_VOLUME,
   // XPROFILE_CONTROL_SENSITIVITY_OPTIONS
   XPROFILE_GAMER_DIFFICULTY,
   // XPROFILE_PREFERRED_COLOR_OPTIONS
   // XPROFILE_PREFERRED_COLOR_OPTIONS_SECOND
   XPROFILE_GAMER_ACTION_AUTO_AIM,
   XPROFILE_GAMER_ACTION_AUTO_CENTER,
   XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL,
   XPROFILE_GAMER_YAXIS_INVERSION,
   XPROFILE_GAMER_RACE_TRANSMISSION,
   XPROFILE_GAMER_RACE_CAMERA_LOCATION,
   XPROFILE_GAMER_RACE_BRAKE_CONTROL,
   XPROFILE_GAMER_RACE_ACCELERATOR_CONTROL,
   XPROFILE_GAMERCARD_TITLE_ACHIEVEMENTS_EARNED,
   XPROFILE_GAMERCARD_TITLE_CRED_EARNED,
   XPROFILE_OPTION_CONTROLLER_VIBRATION,
   XPROFILE_GAMERCARD_PICTURE_KEY,
   XPROFILE_TITLE_SPECIFIC1,
   XPROFILE_TITLE_SPECIFIC2,
//   XPROFILE_TITLE_SPECIFIC3
};

struct ProfileTitle1 
{
   int mStreamSize;
   // [5/6/2008 JRuediger] We save a compressed stream after these 4 bytes.
};

//-- Title2 is mainly for AI: SPCAI or Skirmish
struct ProfileTitle2
{
   int mStreamSize;
   // [5/6/2008 JRuediger] We save a compressed stream after these 4 bytes.
};

#define DEFINE_USERPROFILE_DATA(varType, varName, titleNumber)\
   protected:\
   varType m##varName;\
   public:\
   const varType&   get##varName() const {return(m##varName);}\
   void             set##varName(const varType& foo) {m##varName=foo; mTitle##titleNumber##Dirty = true;}

struct BProfileRank
{
   BProfileRank() :
      mWon(0),
      mValue(0)
   {
   }
   BProfileRank(uint16 rank) :
      mWon(0),
      mValue(rank)
   {
   }

   void reset()
   {
      mWon = 0;
      mValue = 0;
   }

   uint16 mWon;
   // 0 <Blank>
   // 1 Recruit     - 0 first game gets you to Recruit
   // 2 Lieutenant  - 10 wins
   // 3 Captain     - 25
   // 4 Major       - 50
   // 5 Commander   - 100
   // 6 Colonel     - 200
   // 7 Brigadier   - 500
   // 8 General     - 1000
   union
   {
      struct
      {
         uint16 mRank : 4;
         uint16 mLevel : 11;
         uint16 mServerUpdated : 1;
      };
      uint16 mValue;
   };
};

namespace BRank
{
   enum
   {
      eScoreMultiplier = 8000,
   };

   const float cStickWithItMultiplier = 0.2f;
   const float cNaturalBornKiller = 0.4f;

   enum BRankByScore
   {
      eUnranked   = 0 * eScoreMultiplier,
      eRecruit    = 0 * eScoreMultiplier,
      eLieutenant = 10 * eScoreMultiplier,
      eCaptain    = 25 * eScoreMultiplier,
      eMajor      = 50 * eScoreMultiplier,
      eCommander  = 100 * eScoreMultiplier,
      eColonel    = 200 * eScoreMultiplier,
      eBrigadier  = 300 * eScoreMultiplier,
      eGeneral    = 400 * eScoreMultiplier,
   };
}

struct BWinCount
{
   uint16 index;
   uint16 wins;
   uint16 games;
};

//-------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------
class BUserProfile
{
public:
   BUserProfile(XUSER_READ_PROFILE_SETTING_RESULT* pResults = NULL);
   ~BUserProfile();

   enum
   {
      eLastCampaignModeLocal=0,
      eLastCampaignModeLan,
      eLastCampaignModeLive,
   };

   enum
   {
      eControlScheme1=0,
      eControlScheme2,
      eControlScheme3,
      eControlScheme4,
      eControlScheme5,
   };

   XUSER_PROFILE_SETTING*  getProfileSetting(uint profileSettingID);
   bool                    compressInteralData(BUser* pUser);
   BUserAchievementList*   getAchievementList() { return mpAchievementList; }
   const BUserAchievementList*   getAchievementList() const { return mpAchievementList; }
   void                    initializeAchievements();
   void                    addElapsedGameTime(uint32 elapsed) { mTotalGameTime += elapsed; }
   uint32                  getTotalGameTime(void) const { return mTotalGameTime; }
   void                    setSkullBits(uint32 bits) { mSkullBits = bits; }   
   uint32                  getSkullBits(void) const { return mSkullBits; }

   void                    setTimeLineBits(uint64 bits) { mTimeLineBits = bits; }   
   uint64                  getTimeLineBits(void) const { return mTimeLineBits; }
   void                    setTimeLineVisitedBits(uint64 bits) { mTimeLineVisitedBits = bits; }   
   uint64                  getTimeLineVisitedBits(void) const { return mTimeLineVisitedBits; }

   void                    updateMatchmakingRank(bool won, long score);
   void                    updateMatchmakingLevel(uint level);
   uint                    getNextRankAt() const;

   const BCampaignProgress& getCampaignProgress() const { return mCampaignProgress; }
   const BProfileRank&     getRank() const { return mMatchmakingRank; }
   BWinCount*              getLeaderWinsByIndex(int rankMode, int index);
   BWinCount*              getMapWinsByIndex(int rankMode, int index);
   BWinCount*              getModeWinsByIndex(int rankMode, int index);

   void                    reportSkirmishGameCompleted(bool win, long leader, BFixedStringMaxPath &map, long gamemode, bool ranked);
   void                    setupTickerInfo();

   // [5/1/2008 JRuediger] Macro is VariableType, VariableName, Number of the Title it goes into. 1 - 3
   
   //-- ProfileTitle1
   DEFINE_USERPROFILE_DATA(uint8, TitleOneVersion, 1);
   //DEFINE_USERPROFILE_DATA(int, CurrentCampaignNode, 1);
   DEFINE_USERPROFILE_DATA(uint8, LastModePlayed, 1);

   // Game Options
   DEFINE_USERPROFILE_DATA(bool, Option_AIAdvisorEnabled, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_DefaultAISettings, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_DefaultCampaignDifficulty, 1);
   // Control Options
   DEFINE_USERPROFILE_DATA(uint8, Option_ControlScheme, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_RumbleEnabled, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_CameraRotationEnabled, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_XInverted, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_YInverted, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_DefaultZoomLevel, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_StickyCrosshairSensitivity, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_ScrollSpeed, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_CameraFollowEnabled, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_HintsEnabled, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_SelectionSpeed, 1);
   // Audio Options
   DEFINE_USERPROFILE_DATA(uint8, Option_MusicVolume, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_SFXVolume, 1);
   DEFINE_USERPROFILE_DATA(uint8, Option_VoiceVolume, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_SubtitlesEnabled, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_ChatTextEnabled, 1);
   // Graphics Options
   DEFINE_USERPROFILE_DATA(uint8, Option_Brightness, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_FriendOrFoeColorsEnabled, 1);
   DEFINE_USERPROFILE_DATA(bool, Option_MiniMapZoomEnabled, 1);

   

   BCampaignProgress          mCampaignProgress;
   BUserAchievementList*      mpAchievementList; 
   uint32                     mTotalGameTime;
   uint32                     mSkullBits;
   uint64                     mTimeLineBits;
   uint64                     mTimeLineVisitedBits;

   uint8                      mNumLeaders;
   uint8                      mNumMaps;
   uint8                      mNumModes;
   BWinCount                  mSkirmishLeaders[SR_MAX_NUM_RANKMODES][SR_MAX_NUM_LEADERS];  
   BWinCount                  mSkirmishMaps[SR_MAX_NUM_RANKMODES][SR_MAX_NUM_MAPS];
   BWinCount                  mSkirmishModes[SR_MAX_NUM_RANKMODES][SR_MAX_NUM_MODES];

   BProfileRank               mMatchmakingRank;
   uint32                     mMatchmakingScore;


   //-- ProfileTitle2
   //Moved to the player record - eric
   /*
   DEFINE_USERPROFILE_DATA(uint8, AIMemoryVersion, 2);
   DEFINE_USERPROFILE_DATA(BCost, AIAvgResourcesPerGame, 2);
   DEFINE_USERPROFILE_DATA(uint, NumGamesPlayedVSAI, 2);
   DEFINE_USERPROFILE_DATA(uint, NumGamesTheAIWonVSMe, 2);
   DEFINE_USERPROFILE_DATA(bool, DidIWinLastGameVSAI, 2);
   DEFINE_USERPROFILE_DATA(uint, FirstTimeAIAttackedMe, 2);
   DEFINE_USERPROFILE_DATA(uint, FirstTimeIAttackedAI, 2);
   DEFINE_USERPROFILE_DATA(uint, PreviousStratsAIUsedOnMe, 2);
   DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupSquadTotals, 2);
   DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupTechTotals, 2);
   DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupObjectTotals, 2);
   DEFINE_USERPROFILE_DATA(uint, AutoDifficultyLevel, 2);
   DEFINE_USERPROFILE_DATA(uint, AutoDifficultyNumGamesPlayed, 2);
   DEFINE_USERPROFILE_DATA(BStatAbilityIDHashMap, AbilitiesTotal, 2);
   DEFINE_USERPROFILE_DATA(BStatProtoPowerIDHash, PowersTotal, 2);
   */
   byte* getAITrackingDataMemoryPointer();

   bool getTitle1VersionChanged( void ) const { return mTitle1VersionChanged; }

protected:

   // [5/1/2008 JRuediger] Title specific serialization.
   void  initTitle1(void);
   bool  loadTitle1(void);
   bool  saveTitle1(void);
   void  initTitle2(void);
   bool  loadTitle2(void);
   bool  saveTitle2(void);
   void  initTitle3(void);
   bool  loadTitle3(void);
   bool  saveTitle3(void);
   bool  saveCompressedData(BDynamicStream* pCurrentStream, SETTINGS settings);


   // XUSER_READ_PROFILE_SETTING_RESULT*  mpSettingResults;
   XUSER_PROFILE_SETTING               mSettings[ NUM_SETTINGS ];

   bool              mInitialized;
   bool              mTitle1Dirty;
   bool              mTitle2Dirty;
   bool              mTitle3Dirty;
   bool              mTitle1VersionChanged;
};

//==============================================================================
// BProfileReadAsyncTask - read a player's profile
//==============================================================================
class BProfileReadAsyncTask: public BAsyncTask
{

public:
   BProfileReadAsyncTask();
   ~BProfileReadAsyncTask();

   bool startRead(BUser* pUser, BUser *pRemoteUser=NULL, XUID xuid=0);
   void processResult();

protected:
   DWORD                               mdwSettingSizeMax;
   XUSER_READ_PROFILE_SETTING_RESULT*  mpSettingResults;
   XUID                                mXuid;


   BUser* mpUser;
};


//==============================================================================
// BProfileWriteAsyncTask - write a player's profile
//==============================================================================
class BProfileWriteAsyncTask : public BAsyncTask
{

public:
   BProfileWriteAsyncTask();
   ~BProfileWriteAsyncTask();

   bool startWrite(BUser* pUser);
   void processResult();

protected:
};

//==============================================================================
// BUserProfileManager
//==============================================================================
class BUserProfileManager : BAsyncNotify
{

public:

   BUserProfileManager();
   ~BUserProfileManager();

   bool        readProfile(BUser* pUser, BUser *pRemoteUser=NULL, XUID xuid=0);
   bool        writeProfile(BUser* pUser);

   // Async Task Notifications
   void notify(DWORD eventID, void* pTask);

protected:
};
