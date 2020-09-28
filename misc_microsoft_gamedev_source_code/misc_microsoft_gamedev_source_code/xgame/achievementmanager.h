//==============================================================================
// achievementmanager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "asynctaskmanager.h"
#include "XLastGenerated.h"

// Forward Declarations
class BUser;
class BAchievementManager;
class BGameSettings;
class BUserAchievement;

// Global variable for the one BAchievementManager object
extern BAchievementManager gAchievementManager;

#ifndef BUILD_FINAL
class BAchievementDisplayManager;
extern BAchievementDisplayManager gAchievementDispayManager;
#endif

// fixme - this is a hack until we get some real achievements
#define ACHIEVEMENT_ID_MIN                     ACHIEVEMENT_ACH_MEET_SERGEANT_FORGE
#define ACHIEVEMENT_ID_MAX                     ACHIEVEMENT_ACH_24_HOURS_OF_QUALITY


enum AchievementType
{
   cAT_None = 0,
   cAT_Accumulator,
   cAT_Campaign,
   cAT_Skirmish,
   cAT_Map,
   cAT_Scenario,
   cAT_Leaders,
   cAT_Abilities,
   cAT_Trigger,
   cAT_Meta,
   cAT_Misc,
};

enum AchievementRestrictions
{
   eRanked,
   eWin,
   eNumRestrictions
};

enum SkirmishRestrictions
{
   eSkrmWin,
   eSkrmHighestScore,
   eSkrmLive,
   eSkrmDualScarabs,
   eSkrmWinEveryLeader,
   eSkrmPlayEveryMap,
   eSkrmWinEveryMode,
   eSkrmHasAIOpponents,
   eSkrmRanked,

   eSkrmNumRestrictions
};

enum NumPlayers
{
   cHuman,
   cAI,
   cAlly,
   cEnemy,
   cMinimum,
   cMaximum,
   cEither,
   cExact
};

// [10/3/2008 xemu] various sub-categories of miscellaneous achievement 
enum MiscAchievementType
{
   cMiscATUnknown,
   cMiscATSkulls,
   cMiscATTimeline,
   cMiscATPlayedTime,
   cMiscATFinalKillByPower,
};

struct BCompletedScenarioRequirement
{
   BString name;
   int difficulty;
   int medalRequired;
   int mode;
   bool partime;
};

struct BNumPlayersRequirement
{
   int number;
   int playertype;
   int team;
   int range;
};

struct BNameAndWin
{
   BString name;
   bool win;
};


#ifndef BUILD_FINAL
//==============================================================================
// BAchievementDisplay
//==============================================================================
class BAchievementDisplay
{
public:
   BAchievementDisplay(BUString &text, float xPos, float yPos, long justify, float point, float alpha, BColor color, float time );
   ~BAchievementDisplay(){}

   BUString mText;
   float mXPos;
   float mYPos;
   long  mJustify;
   float mPoint;
   float mAlpha;
   BColor mColor;
   float mTime;

};

//==============================================================================
// BAchievementDisplayManager
//==============================================================================
class BAchievementDisplayManager
{
public:
   BAchievementDisplayManager();
   ~BAchievementDisplayManager();

   void render();
   void add(BUString &text, float xPos, float yPos, long justify, float point, float alpha, BColor color, float time);

   BDynamicSimArray<BAchievementDisplay *> mEntries;
};
#endif

//==============================================================================
// BAchievementProto - Base class to hold the achievement rule
//==============================================================================
class BAchievementProto
{
   public:
      BAchievementProto();
      virtual ~BAchievementProto();

      // methods
      bool isType(int type) const { return (type==mType); }
      const BString& getName() const { return mName; }
      int getType() const { return mType; }

      void setID(int id) { mID = id; }
      int getID() const { return mID; }

      const BString& getGamerPicture() const { return mGamerPicture; }
      DWORD getLiveIndex() const { return mLiveIndex; }

      // methods
      virtual bool loadXML(BXMLNode root);

      virtual bool isMatch(int id) const;
      bool isMatch(const BString& achievementName) const;

   protected:
      // common base class fields
      int mID;                      //position in list
      int mGamerPoints;             //points awarded when this achievement is completed
      DWORD    mLiveIndex;          //Index in the header file that matches this achievement
      BString  mName;               // name of achievement
      BString  mGamerPicture;
      AchievementType mType;
      
};

enum AccumulatorActionType
{
   eActionNone,
   eActionKill,
   eActionBuild,
   eNumActions
};

//==============================================================================
// BProtoAccumulatorAchievement
//==============================================================================
class BProtoAccumulatorAchievement : public BAchievementProto
{

public:
   BProtoAccumulatorAchievement();
   virtual ~BProtoAccumulatorAchievement();

   virtual bool loadXML(BXMLNode root);

   bool isMatch(long dbid, AccumulatorActionType actionType, const BGameSettings* pSettings);

   int getQuantity() const { return mQuantity; }

protected:

   long        mDBID;
   AccumulatorActionType mAction;
   int         mQuantity;
   BBitArray   mGameTypes;

};


//==============================================================================
// BProtoCampaignAchievement
//==============================================================================
class BProtoCampaignAchievement : public BAchievementProto
{

public:
   BProtoCampaignAchievement();
   virtual ~BProtoCampaignAchievement();

   virtual bool loadXML(BXMLNode root);

   bool isMatch(BUser *user, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty, int grade);

   int getNumScenariosToBeCompleted() const { return mCompletedScenarios.getNumber(); }

protected:

   bool        mScoreImproved;
   bool        mAllSkullDebuffs;
   int         mLifeTimeScore;
   BBitArray   mRestrictions;
   BBitArray   mGameTypes;
   BBitArray   mDifficulties;
   BDynamicSimArray<BCompletedScenarioRequirement> mCompletedScenarios;

};

//==============================================================================
// BProtoSkirmishAchievement
//==============================================================================
class BProtoSkirmishAchievement : public BAchievementProto
{

public:
   BProtoSkirmishAchievement();
   virtual ~BProtoSkirmishAchievement();

   virtual bool loadXML(BXMLNode root);

   bool isMatch(BUser *user, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty, BUserAchievement *pA);
   void updateLeaderMapAndMode(BUser *user, BGameSettings* pSettings, bool isWin, BUserAchievement *pA);

   uint getNumMaps() const {return mMaps.getNumber();}
   uint getNumLeaders() const {return mLeaders.getNumber();}
   uint getNumModes() const {return mModes.getNumber();}
   int getRequiredRank() const { return mRequiredRank; }

protected:

   BBitArray   mSkirmishRestrictions;
   BBitArray   mDifficulties;
   int mSquadLimit;
   int mMinScore;
   int mMaxTime;
   int mRequiredRank;
   BDynamicSimArray<BNumPlayersRequirement> mNumPlayersRequirements;

   BDynamicSimArray<BNameAndWin> mMaps;
   BDynamicSimArray<BNameAndWin> mLeaders;
   BDynamicSimArray<BNameAndWin> mModes;

};

//==============================================================================
// BProtoTriggerAchievement
//==============================================================================
class BProtoTriggerAchievement : public BAchievementProto
{

public:
   BProtoTriggerAchievement();
   virtual ~BProtoTriggerAchievement();

   virtual bool loadXML(BXMLNode root);
   bool isMatch(const BString& achievementName) const;

protected:

};

//==============================================================================
// BProtoMetaAchievement
//==============================================================================
class BProtoMetaAchievement : public BAchievementProto
{
public:
   BProtoMetaAchievement();
   virtual ~BProtoMetaAchievement();

   virtual bool loadXML(BXMLNode root);
   bool isMatch(BUser *pUser) const;

   void getDebugString(BUser *pUser, BString &debugText) const;

protected:
   BDynamicSimArray<int>    mRequiredAchievements;
};

//==============================================================================
// BProtoMiscAchievement
//==============================================================================
class BProtoMiscAchievement : public BAchievementProto
{
public:
   MiscAchievementType     mMiscType;
   uint32                  mQuantity;

   BProtoMiscAchievement();
   virtual ~BProtoMiscAchievement();

   virtual bool loadXML(BXMLNode root);
   bool isMatch(BUser *pUser) const;
};

//==============================================================================
// MapDescription
//==============================================================================
class BMapDescription
{
public:
   BMapDescription(const BString& mapName, int quantity) :
      mQuantity(quantity)
   {
      mMapName=mapName;
   };


   const BString& getMapName() const { return mMapName; }
   int getQuantity() const { return mQuantity; }

protected:
   BString mMapName;
   int     mQuantity;
};

//==============================================================================
// BProtoMapAchievement
//==============================================================================
class BProtoMapAchievement : public BAchievementProto
{

public:
   BProtoMapAchievement();
   virtual ~BProtoMapAchievement();

   virtual bool loadXML(BXMLNode root);

   bool isMatch(const BString& mapName, const BGameSettings* pSettings, bool isWin, bool isRanked) const;

protected:
   BDynamicSimArray<BMapDescription*> mMaps;
   BBitArray   mRestrictions;
   BBitArray   mGameTypes;

};

//==============================================================================
// BProtoScenarioAchievement
//==============================================================================
class BProtoScenarioAchievement : public BAchievementProto
{

public:
   BProtoScenarioAchievement();
   virtual ~BProtoScenarioAchievement();

   virtual bool loadXML(BXMLNode root);

   bool isMatch(const BString& scenarioName, int difficulty) const;

protected:
   BString mScenarioName;
   int mDifficulty;
};


//-------------------------------------------------------------------------------------------------------------------------------------

//==============================================================================
// BGamerPicAsyncTask - give the user a gamer picture
//==============================================================================
class BGamerPicAsyncTask : public BAsyncTask
{

public:
   BGamerPicAsyncTask();
   virtual ~BGamerPicAsyncTask();

   bool grantGamerPic(const BUser * user, DWORD imageID);

protected:
};


//==============================================================================
// BWriteAchievementAsyncTask - give the user an achievement
//==============================================================================
class BWriteAchievementAsyncTask : public BAsyncTask
{

public:
   BWriteAchievementAsyncTask();
   virtual ~BWriteAchievementAsyncTask();

   bool startWrite(const BUser * user, DWORD achievementID);                // kick off the achievement read

protected:
   XUID  mXuid;
   long  mPort;
   DWORD mdwAchievementCount;
   XUSER_ACHIEVEMENT *mpAchievements;

};

//==============================================================================
// BReadAchievementsAsyncTask - reads the user's achievements
//==============================================================================
class BReadAchievementsAsyncTask : public BAsyncTask
{

   public:
      BReadAchievementsAsyncTask();
      virtual ~BReadAchievementsAsyncTask();

      bool startRead(const BUser * user);                // kick off the achievement read

      // overrides
      void processResult();


   private:
      HANDLE   mhEnum;
      XUID     mXuid;
      long     mPort;
      DWORD    mdwAchievementCount;
      BYTE     *mpAchievements;
};

//==============================================================================
// BAchievementManager
//==============================================================================
class BAchievementManager : BAsyncNotify
{
public:

   enum
   {
      ACHIEVEMENT_COUNT = 5,
   };

   BAchievementManager();
   ~BAchievementManager();

   void        retrieveAchievements(const BUser *user);
   void        update(float elapsedTime);

   bool        loadAchievements();

   const BDynamicSimArray<BAchievementProto*>& getRules() { return mRules; }

   void        updateAccumulator(BUser *pUser, long dbid, AccumulatorActionType actionType, BGameSettings* pSettings, int quantity);
   void        updateCampaign(BUser *pUser, BGameSettings* pSettings, bool isWin, bool isRanked, int quantity, int difficulty, int grade);
   void        updateSkirmish(BUser *pUser, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty);
   void        updateMap(const BUser *pUser, const BSimString& map, const BGameSettings* pSettings, bool isWin, bool isRanked, int quantity);
   void        updateScenario(const BString& scenarioName, int difficulty);
   void        updateAchievement(BUser* pUser, int id);
   void        updateMisc(BUser *pUser, MiscAchievementType miscType);
   void        updateMeta(BUser *pUser);

   void        reportTriggerAchievement(BUser *pUser, BString &name);


   // Async Task Notifications
   void notify(DWORD eventID, void * task);

   uint32      getVersion() {return mVersion;}

   int         getAchievementIndex(const BString &achievementName) const;

   void        setupTickerInfo();

   bool        getLocalizedAchievementTitle(long index, BUString &titleText);

   long        getFinalKillUnitType() const { return mFinalKillUnitType; }


   // [11/21/2008 xemu] go through and grant an achievement for anything we earned, but the grant failed.
   void        fixupEarnedAchievements(BUser *pUser);


protected:
   void removeAllRules(void);

   void        processAchievement(BUser *pUser, BAchievementProto *pRule, BUserAchievement *pA);
   void        grantAchievement(const BUser *user, DWORD dwAchievementID);
   void        grantGamerPicture(const BUser *user, DWORD imageID);

   bool checkGrantPicture(const BUser* pUser, const BAchievementProto* pAchievement);

   int getAchievementID(const char* pName) const;
   int getGamerPictureID(const char* pName) const;

   BDynamicSimArray<BAchievementProto*>   mRules;

   uint32 mVersion;

   long        mFinalKillUnitType;
};
