//==============================================================================
// achievementmanager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "achievementmanager.h"
#include "xbox.h"
#include "gamedirectories.h"
#include "user.h"
#include "userachievements.h"
#include "XLastGenerated.h"
#include "database.h"
#include "protoobject.h"
#include "config.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "userprofilemanager.h"
#include "campaignmanager.h"
#include "gamesettings.h"
#include "world.h"
#include "scoremanager.h"
#include "skullmanager.h"
#include "uiticker.h"

// xgamerender
#include "FontSystem2.h"

// Globals
BAchievementManager gAchievementManager;

#ifndef BUILD_FINAL
BAchievementDisplayManager gAchievementDispayManager;
#endif

#ifndef BUILD_FINAL
//==============================================================================
// BAchievementDisplay::BAchievementDisplay
//==============================================================================
BAchievementDisplay::BAchievementDisplay(BUString &text, float xPos, float yPos, long justify, float point, float alpha, BColor color, float time ) : 
//   mTextIndex(-1),
   mXPos(xPos),
   mYPos(yPos),
   mJustify(justify),
   mPoint(point),
   mAlpha(alpha),
   mColor(color),
   mTime(time)
{
   // look up the string index
//   mTextIndex = gDatabase.getLocStringIndex(textID);

   mText = text;
}

//==============================================================================
// BAchievementDisplayManager::BAchievementDisplayManager
//==============================================================================
BAchievementDisplayManager::BAchievementDisplayManager()
{
   mEntries.clear();
}

//==============================================================================
// BAchievementDisplayManager::~BAchievementDisplayManager
//==============================================================================
BAchievementDisplayManager::~BAchievementDisplayManager()
{
   for(int i=0; i<mEntries.getNumber(); i++)
      delete mEntries[i];
}

//==============================================================================
// BAchievementDisplayManager::render
//==============================================================================
void BAchievementDisplayManager::render()
{
   if(mEntries.getNumber() < 1)
      return;

   BHandle fontHandle = gFontManager.getFontDenmark24();
   gFontManager.setFont( fontHandle );
   float relativeX1 = gUI.mfSafeX1 + mEntries[0]->mXPos;
   float relativeY1 = gUI.mfSafeY1 + mEntries[0]->mYPos;
//   const BUString &text = gDatabase.getLocStringFromIndex(mEntries[0]->mTextIndex);
   gFontManager.drawText(fontHandle, relativeX1, relativeY1, mEntries[0]->mText.getPtr(), mEntries[0]->mColor.asDWORD(), mEntries[0]->mJustify);

   mEntries[0]->mTime -= (float)gWorld->getLastUpdateRealtimeDelta();
   if(mEntries[0]->mTime < 0.0f)
   {
      delete mEntries[0];
      mEntries.removeIndex(0, true);
   }
}

//==============================================================================
// BAchievementDisplayManager::add
//==============================================================================
void BAchievementDisplayManager::add(BUString &text, float xPos, float yPos, long justify, float point, float alpha, BColor color, float time)
{
   BAchievementDisplay *newentry = new BAchievementDisplay(text, xPos, yPos, justify, point, alpha, color, time);
   mEntries.add(newentry);
}
#endif



//==============================================================================
// BAchievementProto::BAchievementProto
//==============================================================================
BAchievementProto::BAchievementProto() : 
   mType(cAT_None),
   mGamerPoints(0),
   mLiveIndex(0),
   mID(0)
{
}

//==============================================================================
// BAchievementProto::~BAchievementProto
//==============================================================================
BAchievementProto::~BAchievementProto()
{
}

//==============================================================================
// BAchievementProto::loadXML
//==============================================================================
bool BAchievementProto::loadXML(BXMLNode root)
{

   // Sanity.
   if(!root)
      return(false);

   // Get the name of this achievement
   root.getAttribValueAsString("name", mName);

   // The gamer picture (if any) to grant with this achievement
   root.getAttribValueAsString("gamerPicture", mGamerPicture);

   //Get the Live index
   root.getAttribValueAsDWORD("LiveIndex", mLiveIndex);
   //BASSERT(mLiveIndex!=0);

   // Success.
   return true;
}

//==============================================================================
// BAchievementProto
//==============================================================================
bool BAchievementProto::isMatch(int id) const
{
   return (id == mID);
}


//==============================================================================
// BAchievementProto
//==============================================================================
bool BAchievementProto::isMatch(const BString& achievementName) const
{
   return (achievementName.compare(mName) == 0);
}


//==============================================================================
// BProtoAccumulatorAchievement
//==============================================================================
BProtoAccumulatorAchievement::BProtoAccumulatorAchievement() :
   mAction(eActionNone),
   mQuantity(0),
   mDBID(-1)
{
   mType = cAT_Accumulator;
   mGameTypes.clear();
   mGameTypes.setNumber(BGameSettings::cNumberNetworkTypes);
}

//==============================================================================
//==============================================================================
BProtoAccumulatorAchievement::~BProtoAccumulatorAchievement()
{
}


//==============================================================================
//==============================================================================
bool BProtoAccumulatorAchievement::isMatch(long dbid, AccumulatorActionType actionType, const BGameSettings* pSettings)
{
   if( pSettings==NULL )
      return false;
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);

   BASSERT(result);
   // are the conditions met?
   return ( (mDBID == dbid) && 
            (actionType == mAction) && 
            (mGameTypes.isBitSet(gameType)) );
}


//==============================================================================
//==============================================================================
bool BProtoAccumulatorAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString tempStr;
      if (name=="Unit")
      {
         childNode.getText(tempStr);

         long id = gDatabase.getProtoObject(tempStr);
         BProtoObject * pObj = gDatabase.getGenericProtoObject(id);
         if (pObj == NULL)
            return false;

         mDBID = pObj->getDBID();
         if (mDBID <= 0)
            return false;           // couldn't find the unit
      }
      else if (name=="Action")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Kill")==0)
            mAction = eActionKill;
         else if (temp.compare("Build")==0)
            mAction = eActionBuild;
      }
      else if (name=="QuantityRequired")
      {
         childNode.getTextAsInt(mQuantity);
      }
      else if (name=="GameType")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Skirmish")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeSkirmish);
         else if (temp.compare("Campaign")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeCampaign);
      }
   }

   return true;
}




//==============================================================================
// BProtoCampaignAchievement
//==============================================================================
BProtoCampaignAchievement::BProtoCampaignAchievement() 
{
   mType = cAT_Campaign;
   mScoreImproved = false;
   mAllSkullDebuffs = false;
   mLifeTimeScore = -1;
   mGameTypes.clear();
   mGameTypes.setNumber(BGameSettings::cNumberNetworkTypes);
   mRestrictions.clear();
   mRestrictions.setNumber(eNumRestrictions);
   mDifficulties.clear();
   mDifficulties.setNumber(DifficultyType::cNumberDifficultyTypes);
   mCompletedScenarios.clear();
}

//==============================================================================
//==============================================================================
BProtoCampaignAchievement::~BProtoCampaignAchievement()
{
}


//==============================================================================
//==============================================================================
bool BProtoCampaignAchievement::isMatch(BUser *user, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty, int grade)
{
   if( pSettings==NULL || user == NULL || user->getProfile() == NULL)
      return false;

   // check that the user meets the restrictions
   if ( mRestrictions.isBitSet(eWin) && (!isWin))
      return false;

   if ( mRestrictions.isBitSet(eRanked) && (!isRanked))
      return false;

   //Check the difficulty levels
   if ( !mDifficulties.areAllBitsClear() )
   {
      // [10/23/2008 xemu] ok, our difficulty also "counts" as any easier difficulty, so iterate through them
      int d;
      bool diffMatch = false;
      for (d = difficulty; d >=0; d--)
      {
         if (mDifficulties.isBitSet(d))
         {
            diffMatch = true;
            break;
         }
      }
      if (!diffMatch)
         return false;
   }

   if ( mScoreImproved && !gScoreManager.wasCampaignScoreImproved(user->getPlayerID()) )
      return false;

   if( mAllSkullDebuffs && !gCollectiblesManager.areAllDebuffSkullsActive() )
      return false;

   if( (mLifeTimeScore > 0) && !(user->getProfile()->mCampaignProgress.getLifetimeCampaignScore() >= (uint)mLifeTimeScore) )
      return false;

   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);

   // is this the right type of game?
   if (!mGameTypes.isBitSet(gameType))
      return false;

   if (mCompletedScenarios.getNumber() > 0)
   {
      if(user==NULL || user->getProfile()==NULL)
         return false;

      for(int i=0; i<mCompletedScenarios.getNumber(); i++)
      {
         BCompletedScenarioRequirement &req = mCompletedScenarios.get(i);

         int difficulty2 = req.difficulty;
         int mode = req.mode;

         bool coop = false;
         int currmode = 0;
         pSettings->getBool(BGameSettings::cCoop, coop);
         if(coop)
            currmode = 1;

         long nodeID = -1;
         // [10/14/2008 xemu] otherwise, no name means only for the scenario just completed (-1 default)
         if (req.name.length() > 0)  //Use the settings for the current game
            nodeID = gCampaignManager.getCampaign(0)->getNodeIDByName( req.name );


         if (nodeID == -1)
         {
            // [10/14/2008 xemu] check the "current scenario" requirements 
            // [10/14/2008 xemu] first for Momma's Boy, any gold medal
            // [10/14/2008 xemu] note this also handles "Own Worst Enemy" since the all debuffs check is earlier in this function 
            if (req.medalRequired > 0)
            {
               if (grade != req.medalRequired)
                  return(false);
            }

            // [10/14/2008 xemu] also "Backscratcher", which is a win in co-op
            if( (mode >=0) && (currmode != mode) )
               return false;

         }
         else
         {
            // [10/23/2008 xemu] this is awful, this code flow needs to be shot into the sun 
            if (difficulty2 == -1)
            {
               //Check to make sure the specific scenario has been completed
               if( !user->getProfile()->mCampaignProgress.isScenarioCompleted( nodeID, mode, difficulty2 ) )
                  return false;
               //If beating it under par time is a requirement, check that now.
               if( req.partime && !user->getProfile()->mCampaignProgress.isScenarioParBeaten( nodeID, mode, difficulty2 ) )
                  return false;
               //If beating it with a certain medal is a requirement, check that now.
               if( (req.medalRequired > 0) && !user->getProfile()->mCampaignProgress.isScenarioMedalEarned( req.medalRequired, nodeID, mode, difficulty2 ) )
                  return false;
            }
            else
            {
               //Check to make sure the specific scenario has been completed
               if( !user->getProfile()->mCampaignProgress.isScenarioCompleted( nodeID, mode, difficulty2, CAMPAIGN_DIFFICULTY_LEVELS - 1 ) )
                  return false;
               //If beating it under par time is a requirement, check that now.
               if( req.partime && !user->getProfile()->mCampaignProgress.isScenarioParBeaten( nodeID, mode, difficulty2, CAMPAIGN_DIFFICULTY_LEVELS - 1 ) )
                  return false;
               //If beating it with a certain medal is a requirement, check that now.
               if( (req.medalRequired > 0) && !user->getProfile()->mCampaignProgress.isScenarioMedalEarned( req.medalRequired, nodeID, mode, difficulty2, CAMPAIGN_DIFFICULTY_LEVELS - 1 ) )
                  return false;
            }
            //If solo vs coop is required, check that now.
            if( (mode >=0) && (currmode != mode) )
               return false;
         }
      }
   }
   return true;
}


//==============================================================================
//==============================================================================
bool BProtoCampaignAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString tempStr;
      
      if (name=="GameType")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Skirmish")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeSkirmish);
         else if (temp.compare("Campaign")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeCampaign);
      }
      else if (name=="Restriction")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Win")==0)
            mRestrictions.setBit(eWin);
         else if (temp.compare("Rank")==0)
            mRestrictions.setBit(eRanked);
      }
      else if (name=="Difficulty")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Legendary")==0)
            mDifficulties.setBit(DifficultyType::cLegendary);
         else if (temp.compare("Heroic")==0)
            mDifficulties.setBit(DifficultyType::cHard);
         else if (temp.compare("Normal")==0)
            mDifficulties.setBit(DifficultyType::cNormal);
         else if (temp.compare("Easy")==0)
            mDifficulties.setBit(DifficultyType::cEasy);
      }
      else if (name=="CompletedScenario")
      {
         BString temp;
         BCompletedScenarioRequirement entry;
         entry.name.empty();
         entry.difficulty=-1;
         entry.partime=false;
         entry.medalRequired=-1;
         entry.mode=-1;
         childNode.getAttribValueAsString("name", entry.name);
         childNode.getAttribValueAsBool("parBeat", entry.partime);
         childNode.getAttribValueAsInt("medal",entry.medalRequired);
         childNode.getAttribValueAsString("Difficulty", temp);
         if (temp.compare("Legendary")==0)
            entry.difficulty = DifficultyType::cLegendary;
         else if (temp.compare("Heroic")==0)
            entry.difficulty = DifficultyType::cHard;
         else if (temp.compare("Normal")==0)
            entry.difficulty = DifficultyType::cNormal;
         else if (temp.compare("Easy")==0)
            entry.difficulty = DifficultyType::cEasy;
         childNode.getAttribValueAsString("Mode", temp);
         if (temp.compare("solo")==0)
            entry.mode = 0;
         else if (temp.compare("coop")==0)
            entry.mode = 1;
         mCompletedScenarios.add(entry);
      }
      else if (name=="ScoreImprovement")
      {
         mScoreImproved = true;
      }
      else if (name=="AllSkullDebuffs")
      {
         mAllSkullDebuffs = true;
      }
      else if (name=="LifetimeScore")
      {
         childNode.getTextAsInt(mLifeTimeScore);
      }
   }

   return true;
}



//==============================================================================
// BProtoSkirmishAchievement
//==============================================================================
BProtoSkirmishAchievement::BProtoSkirmishAchievement() 
{
   mType = cAT_Skirmish;
   mSkirmishRestrictions.clear();
   mSkirmishRestrictions.setNumber(eSkrmNumRestrictions);
   mDifficulties.clear();
   mDifficulties.setNumber(DifficultyType::cNumberDifficultyTypes);
   mSquadLimit = -1;
   mMinScore = -1;
   mMaxTime = -1;
   mRequiredRank = -1;
   mNumPlayersRequirements.clear();
   mMaps.clear();
   mLeaders.clear();
   mModes.clear();
}

//==============================================================================
//==============================================================================
BProtoSkirmishAchievement::~BProtoSkirmishAchievement()
{
}


//==============================================================================
//==============================================================================
bool BProtoSkirmishAchievement::isMatch(BUser *user, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty, BUserAchievement *pA )
{
   if( pSettings==NULL || user == NULL || user->getProfile() == NULL)
      return false;

   // check that the user meets the restrictions
   if ( mSkirmishRestrictions.isBitSet(eSkrmWin) && (!isWin))
      return false;

   if ( mSkirmishRestrictions.isBitSet(eSkrmHighestScore) && ( gScoreManager.getFinalScore(user->getPlayerID()) <  gScoreManager.getHighestFinalScore()))
      return false;

   if ( mSkirmishRestrictions.isBitSet(eSkrmRanked) && (!isRanked))
      return false;


   // [10/17/2008 xemu] ensure there are AI opponents
   if (mSkirmishRestrictions.isBitSet(eSkrmHasAIOpponents))
   {
      int p;
      bool hasAI = false;
      BTeamID ourTeam = user->getTeamID();
      // [10/17/2008 xemu] note we start with player 1 here not 0, since GAIA technically meets the requirements 
      for (p=1; p < gWorld->getNumberPlayers(); p++)
      {
         BPlayer *pPlayer = gWorld->getPlayer(p);
         if ((pPlayer != NULL) && pPlayer->isComputerAI() && (pPlayer->getTeamID() != ourTeam))
         {
            hasAI = true;
            break;
         }
      }
      if (!hasAI)
         return(false);
   }

   long network=0;
   pSettings->getLong(BGameSettings::cNetworkType, network);
   if ( mSkirmishRestrictions.isBitSet(eSkrmLive) && (network != BGameSettings::cNetworkTypeLive))
      return false;

   if ( mSkirmishRestrictions.isBitSet(eSkrmDualScarabs) )
   {
      BEntityIDArray squadIDs;
      user->getPlayer()->getSquadsOfType(gDatabase.getPSIDSkirmishScarab(), squadIDs);
      if (squadIDs.getNumber() < 2)
         return false;
   }

   if ( mLeaders.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
      for (int i=0; i<mLeaders.getNumber(); i++)
      {
         if( !pAA->getLeaderCompleted(i) )
            return false;  
      }
   }

   if ( mMaps.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
      for (int i=0; i<mMaps.getNumber(); i++)
      {
         if( !pAA->getMapCompleted(i) )
            return false;  
      }
   }

   if ( mModes.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
      for (int i=0; i<mModes.getNumber(); i++)
      {
         if( !pAA->getModeCompleted(i) )
            return false;  
      }
   }

   //Check the difficulty levels
   if ( !mDifficulties.areAllBitsClear() )
   {
      // [10/23/2008 xemu] ok, our difficulty also "counts" as any easier difficulty, so iterate through them
      int d;
      bool diffMatch = false;
      for (d = difficulty; d >=0; d--)
      {
         if (mDifficulties.isBitSet(d))
         {
            diffMatch = true;
            break;
         }
      }
      if (!diffMatch)
         return false;
   }

   if ( (mSquadLimit >= 0) )
   {
      BStatsPlayer* pPlayerStats = gStatsManager.getStatsPlayer(user->getPlayerID());
      if (!pPlayerStats)
         return false;

      BStatRecorderTotal* pStatsMilitaryTotal = reinterpret_cast<BStatRecorderTotal*>(pPlayerStats->getStatsRecorder("MilitarySquadTotals2"));
      // [10/7/2008 DPM] the starting squads are no longer counted
      int squadsBuilt = pStatsMilitaryTotal->getStatTotal().mBuilt;

#ifndef BUILD_FINAL
      int playerSquadCount = user->getPlayer()->getTotalSquadCount();
      gConsoleOutput.debug("playerSquadCount %d, squadsBuilt %d, squadLimit %d", playerSquadCount, squadsBuilt, mSquadLimit);
#endif

      if ( squadsBuilt > mSquadLimit )
         return false;
   }

   if ( (mMinScore >= 0) && (gScoreManager.getFinalScore(user->getPlayerID()) < mMinScore) )
      return false;

   if ( (mMaxTime >= 0) && (gScoreManager.getCompletionTime(user->getPlayerID()) > (DWORD)(mMaxTime*1000)) )
      return false;

   if (mNumPlayersRequirements.getNumber() > 0)
   {
      long userTeam = user->getTeamID();
      long numPlayers = 0;
      pSettings->getLong(BGameSettings::cPlayerCount, numPlayers);

      for(int i=0; i<mNumPlayersRequirements.getNumber(); i++)
      {
         BNumPlayersRequirement &req = mNumPlayersRequirements.get(i);
         int matchingPlayers = 0;
         for (int i=0; i<numPlayers; i++)
         {
            //Check team
            long team=0;
            pSettings->getLong(PSINDEX(i+1, BGameSettings::cPlayerTeam), team);
            if( (req.team == cEnemy) && (team == userTeam) )
               continue;
            if( (req.team == cAlly) && (team != userTeam) )
               continue;

            //Check player type
            long type=0;
            pSettings->getLong(PSINDEX(i+1, BGameSettings::cPlayerType), type);
            if( (req.playertype == cHuman) && (type != BPlayer::cPlayerTypeHuman) )
               continue;
            if( (req.playertype == cAI) && (type != BPlayer::cPlayerTypeComputerAI) )
               continue;

            matchingPlayers++;
         }

         if( (req.range == cMinimum) && (matchingPlayers < req.number) )
            return false;

         if( (req.range == cMaximum) && (matchingPlayers > req.number) )
            return false;

         if( (req.range == cExact) && (matchingPlayers != req.number) )
            return false;
      }
   }

   // [9/30/2008 xemu] compare against target Live rank 
   if (mRequiredRank != -1)
   {
      const BUserProfile *pProfile = user->getProfile();
      if (pProfile == NULL)
         return(false);
      if (pProfile->getRank().mRank < mRequiredRank)
         return(false);
   }

   return true;
}

//==============================================================================
//==============================================================================
void BProtoSkirmishAchievement::updateLeaderMapAndMode(BUser *user, BGameSettings* pSettings, bool isWin, BUserAchievement *pA) 
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (user == NULL)
      return;

   if ( mLeaders.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
 
      BSimString &currentLeader = user->getPlayer()->getLeader()->mName;

      for (int i=0; i<mLeaders.getNumber(); i++)
      {
         if( !pAA->getLeaderCompleted(i) && mLeaders[i].name.compare( currentLeader.getPtr() ) == 0 && (!mLeaders[i].win || isWin) )
         {
            pAA->setLeaderCompleted(i);
         }
      }
   }
   if ( mMaps.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
 
      //Get the current map name
      BFixedStringMaxPath currentMap;
      bool result = pSettings->getString(BGameSettings::cMapName, currentMap, MAX_PATH);
      BASSERT(result);
      int pos = currentMap.findRight('\\');
      if ((pos >= 0) && (pos < ((int)currentMap.getLen()-1)))
         currentMap.crop((uint)pos+1, currentMap.getLen());

      for (int i=0; i<mMaps.getNumber(); i++)
      {
         if( !pAA->getMapCompleted(i) && mMaps[i].name.compare( currentMap ) == 0 && (!mMaps[i].win || isWin) )
         {
            pAA->setMapCompleted(i);
         }
      }
   }
   if ( mModes.getNumber() > 0 )
   {
      BUserSkirmishAchievement* pAA = reinterpret_cast<BUserSkirmishAchievement*>(pA);
      BASSERT(pAA);
 
      long currentModeID = -1;
      bool result = pSettings->getLong(BGameSettings::cGameMode, currentModeID);
      BASSERT(result);

      for (int i=0; i<mModes.getNumber(); i++)
      {
         if( !pAA->getModeCompleted(i) )
         {
            long gmID = gDatabase.getGameModeIDByName( mModes[i].name );
            if( (gmID == currentModeID) && (!mModes[i].win || isWin) )
            {
               pAA->setModeCompleted(i);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BProtoSkirmishAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString tempStr;
      
      if (name=="Restriction")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Win")==0)
            mSkirmishRestrictions.setBit(eSkrmWin);
         else if (temp.compare("HighestScore")==0)
            mSkirmishRestrictions.setBit(eSkrmHighestScore);
         else if (temp.compare("Live")==0)
            mSkirmishRestrictions.setBit(eSkrmLive);
         else if (temp.compare("DualScarabs")==0)
            mSkirmishRestrictions.setBit(eSkrmDualScarabs);
         else if (temp.compare("WinEveryLeader")==0)
            mSkirmishRestrictions.setBit(eSkrmWinEveryLeader);
         else if (temp.compare("PlayEveryMap")==0)
            mSkirmishRestrictions.setBit(eSkrmPlayEveryMap);
         else if (temp.compare("WinEveryMode")==0)
            mSkirmishRestrictions.setBit(eSkrmWinEveryMode);
         else if (temp.compare("HasAIOpponents")==0)
            mSkirmishRestrictions.setBit(eSkrmHasAIOpponents);
         else if (temp.compare("Ranked")==0)
            mSkirmishRestrictions.setBit(eSkrmRanked);
         else
            BASSERTM(0, "Unknown Achivement Restriction");
      }
      else if (name=="Difficulty")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Legendary")==0)
            mDifficulties.setBit(DifficultyType::cLegendary);
         else if (temp.compare("Heroic")==0)
            mDifficulties.setBit(DifficultyType::cHard);
         else if (temp.compare("Normal")==0)
            mDifficulties.setBit(DifficultyType::cNormal);
         else if (temp.compare("Easy")==0)
            mDifficulties.setBit(DifficultyType::cEasy);
         else
            BASSERTM(0, "Unknown Achivement Difficulty");
      }
      else if (name=="SquadLimit")
      {
         childNode.getTextAsInt(mSquadLimit);
      }
      else if (name=="MinScore")
      {
         childNode.getTextAsInt(mMinScore);
      }
      else if (name=="MaxTime")
      {
         childNode.getTextAsInt(mMaxTime);
      }
      else if (name=="NumPlayers")
      {
         BNumPlayersRequirement entry;
         entry.number = 0;
         entry.playertype = cEither;
         entry.team = cEither;
         entry.range = cExact;

         childNode.getTextAsInt(entry.number);

         BString temp;
         childNode.getAttribValueAsString("PlayerType", temp);
         if (temp.compare("Human")==0)
            entry.playertype = cHuman;
         else if (temp.compare("AI")==0)
            entry.playertype = cAI;
         else if (temp.compare("Either")==0)
            entry.playertype = cEither;
         else
            BASSERTM(0, "Unknown Achivement PlayerType");

         childNode.getAttribValueAsString("Team", temp);
         if (temp.compare("Ally")==0)
            entry.team = cAlly;
         else if (temp.compare("Enemy")==0)
            entry.team = cEnemy;
         else if (temp.compare("Either")==0)
            entry.team = cEither;
         else
            BASSERTM(0, "Unknown Achivement Team");

         childNode.getAttribValueAsString("Range", temp);
         if (temp.compare("Minimum")==0)
            entry.range = cMinimum;
         else if (temp.compare("Maximum")==0)
            entry.range = cMaximum;
         else if (temp.compare("Exact")==0)
            entry.range = cExact;
         else
            BASSERTM(0, "Unknown Achivement Range");

         mNumPlayersRequirements.add(entry);
      }
      else if (name=="Leader")
      {
         BNameAndWin entry;
         entry.win = false;

         childNode.getText(entry.name);
         childNode.getAttribValueAsBool("Win", entry.win);
         mLeaders.add(entry);
      }
      else if (name=="Map")
      {
         BNameAndWin entry;
         entry.win = false;

         childNode.getText(entry.name);
         childNode.getAttribValueAsBool("Win", entry.win);

         mMaps.add(entry);
      }
      else if (name=="Mode")
      {
         BNameAndWin entry;
         entry.win = false;

         childNode.getText(entry.name);
         childNode.getAttribValueAsBool("Win", entry.win);

         mModes.add(entry);
      }
      else if (name=="Rank")
      {
         childNode.getTextAsInt(mRequiredRank);
      }

   }

   return true;
}




//==============================================================================
// BProtoTriggerAchievement
//==============================================================================
BProtoTriggerAchievement::BProtoTriggerAchievement()
{
   mType = cAT_Trigger;
}

//==============================================================================
//==============================================================================
BProtoTriggerAchievement::~BProtoTriggerAchievement()
{
}

//==============================================================================
//==============================================================================
bool BProtoTriggerAchievement::isMatch(const BString& achievementName) const
{
   return BAchievementProto::isMatch(achievementName);
}

//==============================================================================
//==============================================================================
bool BProtoTriggerAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   return true;
}

//==============================================================================
// BProtoMetaAchievement
//==============================================================================
BProtoMetaAchievement::BProtoMetaAchievement()
{
   mType = cAT_Meta;
}

//==============================================================================
BProtoMetaAchievement::~BProtoMetaAchievement()
{
}

//==============================================================================
bool BProtoMetaAchievement::isMatch(BUser *pUser) const
{
   // [10/6/2008 xemu] go through all the required achievements and see if the user has unlocked them
   int i;
   for (i=0; i < mRequiredAchievements.getNumber(); i++)
   {
      int achIndex = mRequiredAchievements[i];
      BUserAchievementList * pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {
         BUserAchievement *pA = pAchievements->getAchievement(achIndex);
         if (!pA->isGranted())
            return(false);
      }
   }
   // [10/6/2008 xemu] if we get here, then we haven't failed on any of the achievement filters, so we are all good 
   return true;
}

//==============================================================================
bool BProtoMetaAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      //BSimString tempStr;
      if (name == "Achievement")
      {
         BString achievementName;
         childNode.getText(achievementName);
         int index = gAchievementManager.getAchievementIndex(achievementName);
         if (index != -1)
         {
            mRequiredAchievements.add(index);
         }
      }
   }
   return true;
}

//==============================================================================
// BProtoMetaAchievement::getDebugString
//==============================================================================
void BProtoMetaAchievement::getDebugString(BUser *pUser, BString &debugText) const
{
   int i;
   debugText.set("");
   BUserAchievementList * pAchievements = pUser->getAchievementList();
   if (pAchievements == NULL)
      return;

   for (i=0; i < mRequiredAchievements.getNumber(); i++)
   {
      int achIndex = mRequiredAchievements[i];
      BUserAchievement *pA = pAchievements->getAchievement(achIndex);
      if (pA->isGranted())
         debugText.append("X");
      else
         debugText.append("-");
      if ((i+1) % 10 == 0)
         debugText.append(" ");
   }
}


//==============================================================================
// BProtoMiscAchievement
//==============================================================================
BProtoMiscAchievement::BProtoMiscAchievement()
{
   mType = cAT_Misc;
   mMiscType = cMiscATUnknown;
   mQuantity = 0;
}

//==============================================================================
BProtoMiscAchievement::~BProtoMiscAchievement()
{
}

//==============================================================================
bool BProtoMiscAchievement::isMatch(BUser *pUser) const
{
   switch (mMiscType)
   {
      case cMiscATSkulls:
         {
            // MS 12/8/2008: PHX-18996, we don't want to count "owner only" skulls, so I added a
            // third parameter to getNumSkullsCollected() that prevents those from being counted.
            uint32 numSkulls = gCollectiblesManager.getNumSkullsCollected(pUser, false, false);
            if (numSkulls >= mQuantity)
               return(true);
         }
         break;
      case cMiscATTimeline:
         {
            uint32 numTimelineEvents = gCollectiblesManager.getNumTimelineEventsUnlocked(pUser);
            if (numTimelineEvents >= mQuantity)
               return(true);
         }
         break;
      case cMiscATPlayedTime:
         {
            if (pUser->getProfile() != NULL)
            {
               uint32 totalTime = pUser->getProfile()->getTotalGameTime();
               if (totalTime > mQuantity)
                  return(true);
            }
         }
         break;
      case cMiscATFinalKillByPower:
         {
            // [10/7/2008 xemu] bascially if we ever even get here, then it must have happened 
            return(true);
         }
         break;
      default:
         BASSERT(0);
         break;
   }
   // [10/3/2008 xemu] if we fall through, no dice 
   return(false);
}

//==============================================================================
bool BProtoMiscAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      //BSimString tempStr;
      if (name == "Skulls")
      {
         mMiscType = cMiscATSkulls;
         // [10/3/2008 xemu] note this function sets mQuantity 
         childNode.getTextAsUInt32(mQuantity);
      }
      if (name == "TimelineEvents")
      {
         mMiscType = cMiscATTimeline;
         // [10/3/2008 xemu] note this function sets mQuantity 
         childNode.getTextAsUInt32(mQuantity);
      }
      if (name == "PlayedTime")
      {
         mMiscType = cMiscATPlayedTime;
         // [10/3/2008 xemu] note this function sets mQuantity 
         childNode.getTextAsUInt32(mQuantity);
      }
      if (name == "FinalKillByPower")
      {
         mMiscType = cMiscATFinalKillByPower;
      }
   }

   return true;
}





//==============================================================================
// BProtoMapAchievement
//==============================================================================
BProtoMapAchievement::BProtoMapAchievement()
{
   mType = cAT_Map;

   mMaps.setNumber(0);
   mGameTypes.clear();
   mGameTypes.setNumber(BGameSettings::cNumberNetworkTypes);
   mRestrictions.clear();
   mRestrictions.setNumber(eNumRestrictions);
}

//==============================================================================
//==============================================================================
BProtoMapAchievement::~BProtoMapAchievement()
{
   for(int i=0; i<mMaps.getNumber(); i++)
      delete mMaps[i];
   mMaps.setNumber(0);
}


//==============================================================================
//==============================================================================
bool BProtoMapAchievement::isMatch(const BString& mapName, const BGameSettings* pSettings, bool isWin, bool isRanked) const
{
   if( pSettings==NULL )
      return false;
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);

   bool matchedMap=false;
   for (int i=0; i<mMaps.getNumber(); i++)
   {
      if (mMaps[i]->getMapName().compare(mapName.getPtr())==0)
      {
         matchedMap=true;
         break;
      }
   }
   if (!matchedMap)
      return false;

   // check that the user meets the restrictions
   if ( mRestrictions.isBitSet(eWin) && (!isWin))
      return false;

   if ( mRestrictions.isBitSet(eRanked) && (!isRanked))
      return false;

   // is this the right type of game?
   if (!mGameTypes.isBitSet(gameType))
      return false;

   return true;
}


//==============================================================================
//==============================================================================
bool BProtoMapAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString tempStr;

      if (name=="GameType")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Skirmish")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeSkirmish);
         else if (temp.compare("Campaign")==0)
            mGameTypes.setBit(BGameSettings::cGameTypeCampaign);
      }
      else if (name=="MapDescription")
      {
         long quantity=0;
         BString mapName;
         childNode.getAttribValueAsLong("quantity", quantity);
         childNode.getText(mapName);

         BMapDescription* pMap=new BMapDescription(mapName, quantity);
         mMaps.add(pMap);

      }
      else if (name=="Restriction")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Win")==0)
            mGameTypes.setBit(eWin);
         else if (temp.compare("Rank")==0)
            mGameTypes.setBit(eRanked);
      }
   }

   return true;
}




//==============================================================================
// BProtoScenarioAchievement
//==============================================================================
BProtoScenarioAchievement::BProtoScenarioAchievement() :
   mDifficulty(DifficultyType::cNormal)
{
   mType = cAT_Scenario;
}

//==============================================================================
//==============================================================================
BProtoScenarioAchievement::~BProtoScenarioAchievement()
{
}


//==============================================================================
//==============================================================================
bool BProtoScenarioAchievement::isMatch(const BString& scenario, int difficulty) const
{
   if (mDifficulty != difficulty)
      return false;

   if (scenario.compare(mScenarioName.getPtr())!=0)
      return false;

   return true;
}


//==============================================================================
//==============================================================================
bool BProtoScenarioAchievement::loadXML(BXMLNode node)
{
   // let the parent do its thing.
   BAchievementProto::loadXML(node);
#if 0  //this can be done in BProtoCampaignAchievement
   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString tempStr;

      if (name=="ScenarioName")
      {
         childNode.getText(mScenarioName);
      }
      else if (name=="Difficulty")
      {
         BString temp;
         childNode.getText(temp);
         if (temp.compare("Moderate")==0)
            mDifficulty=eModerate;
         else if (temp.compare("Difficulty")==0)
            mDifficulty=eModerate;
      }
   }
#endif
   return true;
}



//==============================================================================
// BGamerPicAsyncTask::BGamerPicAsyncTask
//==============================================================================
BGamerPicAsyncTask::BGamerPicAsyncTask() :
   BAsyncTask()
{
}

//==============================================================================
// BGamerPicAsyncTask::BGamerPicAsyncTask
//==============================================================================
BGamerPicAsyncTask::~BGamerPicAsyncTask()
{
}

//==============================================================================
// BGamerPicAsyncTask::grantGamerPic
//==============================================================================
bool BGamerPicAsyncTask::grantGamerPic(const BUser * user, DWORD imageID)
{
   if (user == NULL)
      return false;

   if (!user->isSignedIn())
      return false;

   // fixme - validate the image id here.

   // send the achievement off
   DWORD dwStatus = XUserAwardGamerPicture(user->getPort(), imageID, 0, &mOverlapped);
   if (dwStatus != ERROR_IO_PENDING)
      return false;

   return true;
}






//==============================================================================
// BWriteAchievementAsyncTask::BWriteAchievementAsyncTask
//==============================================================================
BWriteAchievementAsyncTask::BWriteAchievementAsyncTask() : 
   BAsyncTask(),
   mXuid(0),
   mPort(0),
   mdwAchievementCount(0),
   mpAchievements(NULL)
{
}

//==============================================================================
// BWriteAchievementAsyncTask::BWriteAchievementAsyncTask
//==============================================================================
BWriteAchievementAsyncTask::~BWriteAchievementAsyncTask()
{
   if (mpAchievements)
      delete mpAchievements;
}

//==============================================================================
// BWriteAchievementAsyncTask::startWrite
//==============================================================================
bool BWriteAchievementAsyncTask::startWrite(const BUser * user, DWORD achievementID)
{
   if (user == NULL)
      return false;

   if (!user->isSignedIn())
      return false;

   // fixme - hack until we can get some real achievements in here
   // validate the achievement ID
   if ( (achievementID < ACHIEVEMENT_ID_MIN) || (achievementID > ACHIEVEMENT_ID_MAX) )
      return false;

   mpAchievements = new XUSER_ACHIEVEMENT;
   mdwAchievementCount = 1;

   mpAchievements[0].dwUserIndex = user->getPort();
   mpAchievements[0].dwAchievementId = achievementID;


   // send the achievement off
   DWORD dwStatus = XUserWriteAchievements( mdwAchievementCount, mpAchievements, &mOverlapped);
   if (dwStatus != ERROR_IO_PENDING)
   {
      delete mpAchievements;
      return false;
   }

   return true;
}



//-------------------------------------------------------------------------------------

//==============================================================================
// BAchievementManager::BAchievementManager
//==============================================================================
BReadAchievementsAsyncTask::BReadAchievementsAsyncTask() : 
   BAsyncTask(),
   mhEnum(NULL),
   mXuid(0),
   mPort(0),
   mdwAchievementCount(0),
   mpAchievements(NULL)
{

}

//==============================================================================
// BAchievementManager::BAchievementManager
//==============================================================================
BReadAchievementsAsyncTask::~BReadAchievementsAsyncTask()
{
   if (mpAchievements)
      delete mpAchievements;
}

//==============================================================================
// BReadAchievementsAsyncTask::startRead
//==============================================================================
bool BReadAchievementsAsyncTask::startRead(const BUser * user)
{
   if (user == NULL)
      return false;

   // if the user is not signed in, we cannot get the achievements
   if (!user->isSignedIn())
      return false;

   // get the XUID of the user
   mXuid = user->getXuid();

   DWORD cbBuffer;
   // Create enumerator for the default device
   DWORD dwStatus = XUserCreateAchievementEnumerator(
      0,                            // retrieve achievements for the current title
      user->getPort(),              // local signed-in user 0 is making the request
      mXuid,                         // achievements for the current user are to be found 
      XACHIEVEMENT_DETAILS_ALL,     // information on all achievements is to be retrieved
      0,                            // starting achievement index
      BAchievementManager::ACHIEVEMENT_COUNT,            // number of achievements to return
      &cbBuffer,                    // bytes needed
      &mhEnum );

   if( (dwStatus != ERROR_SUCCESS ) )
   {
      blogerrortrace("BAchievementManager::retrieveAchievements: Couldn't create achievement enumerator." );
      return false;
   }

   // How many are we retrieving?
   mdwAchievementCount = cbBuffer / XACHIEVEMENT_SIZE_FULL;

   // get our memory ready for the data coming in
   mpAchievements = new BYTE[cbBuffer];

   // kick off the enumeration
   DWORD result = XEnumerate(mhEnum, mpAchievements, cbBuffer, NULL, &mOverlapped);
   if( (result != ERROR_IO_PENDING) && (result != ERROR_SUCCESS))
   {
      CloseHandle(mhEnum);
      return false;
   }

   return true;
}

//==============================================================================
// BReadAchievementsAsyncTask::processResult
//==============================================================================
void BReadAchievementsAsyncTask::processResult()
{
   BAsyncTask::processResult();

   CloseHandle(mhEnum);       // don't leak the handle
}



//==============================================================================
// BAchievementManager::BAchievementManager
//==============================================================================
BAchievementManager::BAchievementManager()
{
   mFinalKillUnitType = -1;
   mVersion = 0;
}

//==============================================================================
// BAchievementManager::~BAchievementManager
//==============================================================================
BAchievementManager::~BAchievementManager()
{
   removeAllRules();
}

//==============================================================================
// BAchievementManager::~BAchievementManager
//==============================================================================
void BAchievementManager::retrieveAchievements(const BUser *user)
{
   if (user == NULL)
      return;

   // if the user is not signed in, we cannot get the achievements
   if (!user->isSignedIn())
      return;

   BReadAchievementsAsyncTask * task = new BReadAchievementsAsyncTask();

   if (!task->startRead(user))
      delete task;

   task->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(task);

}

//==============================================================================
// BAchievementManager::startRead
//==============================================================================
void BAchievementManager::notify(DWORD eventID, void * task)
{
   // fixme - notify somebody that the read of the achievements is done
}






//==============================================================================
// BAchievementManager::update
//==============================================================================
void BAchievementManager::update(float elapsedTime)
{
}

//==============================================================================
// BAchievementManager::grantGamerPicture
//==============================================================================
void BAchievementManager::grantGamerPicture(const BUser *user, DWORD imageID)
{
   if (user == NULL)
      return;

   if (!user->isSignedIn())
      return;

   BGamerPicAsyncTask* pTask = new BGamerPicAsyncTask();
   if (!pTask->grantGamerPic(user, imageID))
   return;

   pTask->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(pTask);
}


//==============================================================================
// BAchievementManager::grantAchievement
//
// Before writing an achievement, XContentGetCreator must be called on the
// loaded save game to verify that the current user is in fact the same user
// who created the save game. Only call XUserWriteAchievements if this is
// true, otherwise the title is violating TCR 069 [GP No Sharing of Achievements]
//==============================================================================
void BAchievementManager::grantAchievement(const BUser *user, DWORD dwAchievementID)
{
   if (user == NULL)
      return;

   if (!user->isSignedIn())
      return;

   BWriteAchievementAsyncTask * task  = new BWriteAchievementAsyncTask();

   if (!task->startWrite(user, dwAchievementID))
      return;

   task->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(task);
}

//==============================================================================
// BAchievementManager::loadAchievements
//==============================================================================
bool BAchievementManager::loadAchievements()
{
   mFinalKillUnitType = gDatabase.getObjectType("Unit");

   BXMLReader reader;
   if (!reader.load(cDirData, "achievements.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return(false);   

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "Achievements");

   //Create an achievement rule for each entry.
   for (long i=0; i < rootNode.getNumberChildren(); i++)
   {
      //Get child node.
      BXMLNode node(rootNode.getChild(i));

      BAchievementProto* pRule = NULL;
      // Create the class based on the tag
      // Check for Achievement node.
      if (node.getName().compare("AchievementXMLVersion") == 0)
      {
         mVersion = 0;
         node.getTextAsUInt32(mVersion);
      }
      if (node.getName().compare("Achievement") == 0)
         pRule = new BAchievementProto();
      else if (node.getName().compare("Accumulator") == 0)
         pRule = new BProtoAccumulatorAchievement();
      else if (node.getName().compare("Campaign") == 0)
         pRule = new BProtoCampaignAchievement();
      else if (node.getName().compare("Skirmish") == 0)
         pRule = new BProtoSkirmishAchievement();
      else if (node.getName().compare("Trigger") == 0)
         pRule = new BProtoTriggerAchievement();
      else if (node.getName().compare("Map") == 0)
         pRule = new BProtoMapAchievement();
      else if (node.getName().compare("Scenario") == 0)
         pRule = new BProtoScenarioAchievement();
      else if (node.getName().compare("Meta") == 0)
         pRule = new BProtoMetaAchievement();
      else if (node.getName().compare("Misc") == 0)
         pRule = new BProtoMiscAchievement();

      if (pRule == NULL)
         continue;

      // Load the achievement
      pRule->loadXML(node);

      // set the ID
      pRule->setID( getAchievementID(pRule->getName()) );

      // Add an achievement to the list.
      mRules.add(pRule);
   }

   return(true);
}

//=============================================================================
// BAchievementManager::removeAllRules
//=============================================================================
void BAchievementManager::removeAllRules(void)
{
   for(int i=0; i<mRules.getNumber(); i++)
      delete mRules[i];
   mRules.setNumber(0);
}


//=============================================================================
// BAchievementManager::processAchievement
//=============================================================================
void BAchievementManager::processAchievement(BUser *pUser, BAchievementProto *pRule, BUserAchievement *pA)
{
   // grant the achievement
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigFakeAchievements))
   {
      //const BUString &text = gDatabase.getLocStringFromIndex(1007);
      BUString text;
      BUString temp;
      text.format(L"%s Fake Achievement Granted", pRule->getName().asUnicode(temp));
      gAchievementDispayManager.add(text, 600.0f, 500.0f, BFontManager2::cJustifyCenter, 38.0f, 1.0f, BColor(255,255,255), 15.0f);
   }
   else
#endif
   {
      grantAchievement(pUser, pRule->getLiveIndex());
      checkGrantPicture(pUser, pRule);
   }

   uint32 gameTime = 0;
   if( pUser->getProfile() )
      gameTime = pUser->getProfile()->getTotalGameTime();

   pA->setGranted(gameTime);         // fixme - we should really wait until we hear that the achievement was granted successfully

   updateMeta(pUser);
}

//=============================================================================
// BAchievementManager::updateAccumulator
//=============================================================================
void BAchievementManager::updateAccumulator(BUser * pUser, long dbid, AccumulatorActionType actionType, BGameSettings* pSettings, int quantity)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   uint count = mRules.getNumber();
   for (uint i=0; i<count; i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Accumulator))
         continue;

      BProtoAccumulatorAchievement* pRule = reinterpret_cast<BProtoAccumulatorAchievement*>(mRules[i]);

      // our achievement counter?
      if (!pRule->isMatch(dbid, actionType, pSettings))
         continue;

      // now update the quantity and check if the achievement was earned.
      BUserAchievementList * pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {
         BUserAchievement * pA = pAchievements->getAchievement(i);
         if (pA && !pA->isGranted())
         {
            BUserAccumulatorAchievement* pAA = reinterpret_cast<BUserAccumulatorAchievement*>(pA);
            pAA->addQuantity(quantity);         // add the data into the quantity

            if (pAA->getQuantity() >= pRule->getQuantity())
            {
               processAchievement(pUser, pRule, pA);
            }
         }
      }
   }
}


//=============================================================================
// BAchievementManager::updateCampaign
//=============================================================================
void BAchievementManager::updateCampaign(BUser * pUser, BGameSettings* pSettings, bool isWin, bool isRanked, int quantity, int difficulty, int grade)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   BUserAchievementList * pAchievements = pUser->getAchievementList();
   BASSERT(pAchievements);
   if (!pAchievements)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Campaign))
         continue;

      //Get the corresponding userachievement to check it's status
      //If we've already granted this achievement, don't bother checking it
      BUserAchievement * pA = pAchievements->getAchievement(i);
      BASSERT(pA);
      if (!pA || pA->isGranted())  
         continue;

      //Check to see if we have a match
      BProtoCampaignAchievement* pRule = reinterpret_cast<BProtoCampaignAchievement*>(mRules[i]);
      if (!pRule->isMatch(pUser, pSettings, isWin, isRanked, difficulty, grade))
         continue;

      //If we've gotten here, we have a match!  Congrats, dude.
      processAchievement(pUser, pRule, pA);

   }
}


//=============================================================================
// BAchievementManager::updateSkirmish
//=============================================================================
void BAchievementManager::updateSkirmish(BUser * pUser, BGameSettings* pSettings, bool isWin, bool isRanked, int difficulty)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   BUserAchievementList * pAchievements = pUser->getAchievementList();
   BASSERT(pAchievements);
   if (!pAchievements)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Skirmish))
         continue;

      //Get the corresponding userachievement to check it's status
      //If we've already granted this achievement, don't bother checking it
      BUserAchievement * pA = pAchievements->getAchievement(i);
      BASSERT(pA);
      if (!pA || pA->isGranted())  
         continue;

      //Update the counts before checking if it is a match.
      BProtoSkirmishAchievement* pRule = reinterpret_cast<BProtoSkirmishAchievement*>(mRules[i]);
      pRule->updateLeaderMapAndMode(pUser, pSettings, isWin, pA);

      //Check to see if we have a match
      if (!pRule->isMatch(pUser, pSettings, isWin, isRanked, difficulty, pA))
         continue;

      //If we've gotten here, we have a match!  Congrats, dude.
      processAchievement(pUser, pRule, pA);
   }
}


//=============================================================================
// BAchievementManager::updateMap
//=============================================================================
void BAchievementManager::updateMap(const BUser * pUser, const BSimString& map, const BGameSettings* pSettings, bool isWin, bool isRanked, int quantity)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Map))
         continue;

      BProtoMapAchievement* pRule = reinterpret_cast<BProtoMapAchievement*>(mRules[i]);

      // our achievement counter?
      if (!pRule->isMatch(map, pSettings, isWin, isRanked))
         continue;

      // fixme - now update the quantity and check if the achievement was earned.
   }
}

//=============================================================================
// BAchievementManager::updateScenario
//=============================================================================
void BAchievementManager::updateScenario(const BString& scenarioName, int difficulty)
{
   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Scenario))
         continue;

      BProtoScenarioAchievement* pRule = reinterpret_cast<BProtoScenarioAchievement*>(mRules[i]);

      // our achievement counter?
      if (!pRule->isMatch(scenarioName, difficulty))
         continue;

      // fixme - now update the quantity and check if the achievement was earned.
   }
}

//=============================================================================
// BAchievementManager::updateMisc
//=============================================================================
void BAchievementManager::updateMisc(BUser *pUser, MiscAchievementType miscType)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   // [10/3/2008 xemu] go through and check the "misc" achievements
   // [10/3/2008 xemu] amount of shared code between here and the other update funcs is awful 
   BUserAchievementList * pAchievements = pUser->getAchievementList();
   BASSERT(pAchievements);
   if (!pAchievements)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Misc))
         continue;

      //Get the corresponding userachievement to check it's status
      //If we've already granted this achievement, don't bother checking it
      BUserAchievement * pA = pAchievements->getAchievement(i);
      BASSERT(pA);
      if (!pA || pA->isGranted())  
         continue;

      //Check to see if we have a match
      BProtoMiscAchievement* pRule = reinterpret_cast<BProtoMiscAchievement*>(mRules[i]);
      if (pRule->mMiscType != miscType)
         continue;

      if (!pRule->isMatch(pUser))
         continue;

      processAchievement(pUser, pRule, pA);
   }
}

//=============================================================================
// BAchievementManager::updateMeta
//=============================================================================
void BAchievementManager::updateMeta(BUser *pUser)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   // [10/3/2008 xemu] go through and check the "meta" achievements
   // [10/3/2008 xemu] amount of shared code between here and the other update funcs is awful 
   BUserAchievementList * pAchievements = pUser->getAchievementList();
   BASSERT(pAchievements);
   if (!pAchievements)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Meta))
         continue;

      //Get the corresponding userachievement to check it's status
      //If we've already granted this achievement, don't bother checking it
      BUserAchievement * pA = pAchievements->getAchievement(i);
      BASSERT(pA);
      if (!pA || pA->isGranted())  
         continue;

      //Check to see if we have a match
      BProtoMetaAchievement* pRule = reinterpret_cast<BProtoMetaAchievement*>(mRules[i]);

      if (!pRule->isMatch(pUser))
         continue;

      processAchievement(pUser, pRule, pA);
   }
}

//=============================================================================
// BAchievementManager::reportTriggerAchievement
//=============================================================================
void BAchievementManager::reportTriggerAchievement(BUser *pUser, BString &name)
{
   BASSERT(pUser);
   if(!pUser->isSignedIn() || !pUser->getPlayer() || !pUser->getPlayer()->isPlaying())
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_Trigger))
         continue;

      BProtoTriggerAchievement* pRule = reinterpret_cast<BProtoTriggerAchievement*>(mRules[i]);

      // our achievement counter?
      if (!pRule->isMatch(name))
         continue;

      // now update the achievement
      BUserAchievementList * pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {
         BUserAchievement * pA = pAchievements->getAchievement(i);
         if (pA && !pA->isGranted())
         {
            processAchievement(pUser, pRule, pA);
         }
      }

   }
}



//=============================================================================
// BAchievementManager::updateAchievement
//=============================================================================
void BAchievementManager::updateAchievement(BUser* pUser, int id)
{
   // [10/21/2008 xemu] in some edge cases these update functions get called from AI players, so adding some bulletproofing 
   if (pUser == NULL)
      return;

   for (int i=0; i<mRules.getNumber(); i++)
   {
      // our type?
      if (!mRules[i]->isType(cAT_None))
         continue;

      BAchievementProto* pRule = mRules[i];

      // our achievement counter?
      if (!pRule->isMatch(id))
         continue;

      // now update the achievement
      BUserAchievementList * pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {
         BUserAchievement * pA = pAchievements->getAchievement(i);
         if (pA && !pA->isGranted())
         {
            processAchievement(pUser, pRule, pA);
         }
      }
   }
}


//==============================================================================
// BAchievementManager::checkGrantPicture
//==============================================================================
bool BAchievementManager::checkGrantPicture(const BUser* pUser, const BAchievementProto* pAchievement)
{
   if (pUser == NULL)
      return false;

   if (!pUser->isSignedIn())
      return false;

   int imageID = getGamerPictureID(pAchievement->getGamerPicture());
   if (imageID < 0)
      return false;

   grantGamerPicture(pUser, imageID);
   return true;
}

//==============================================================================
// BDatabase::getObjectClass
//==============================================================================
int BAchievementManager::getGamerPictureID(const char* pName) const
{
   if(stricmp(pName, "Forge")==0)
      return GAMER_PICTURE_GPIC_2_FORGE;
   else if(stricmp(pName, "Anders")==0) 
      return GAMER_PICTURE_GPIC_1_ANDERS;

   return -1;
}


//==============================================================================
// BDatabase::getAchievementID
//==============================================================================
int BAchievementManager::getAchievementID(const char* pName) const
{
   /*
#ifndef TITLEID_HALO_WARS_ALPHA
   if (!gConfig.isDefined(cConfigAlpha))
   {
      if(stricmp(pName, "Intermingler")==0)
         return ACHIEVEMENT_ACH_TEST_01;
      else if(stricmp(pName, "GruntsAbound")==0)
         return ACHIEVEMENT_ACH_TEST_02;
      else if(stricmp(pName, "MasterChief")==0)
         return ACHIEVEMENT_ACH_TEST_03;
      else if(stricmp(pName, "UniversalTraveler")==0)
         return ACHIEVEMENT_ACH_TEST_04;
      else if(stricmp(pName, "Cadet")==0)
         return ACHIEVEMENT_ACH_TEST_05;
   }
#endif
   */
   return -1;
}

//==============================================================================
// BAchievementManager::getAchievementIndex
//==============================================================================
int BAchievementManager::getAchievementIndex(const BString &achievementName) const
{
   int i;
   for (i=0; i < mRules.getNumber(); i++)
   {
      BAchievementProto *pRule = mRules[i];
      if (pRule == NULL)
         continue;
      if (pRule->getName() == achievementName)
      {
         return(i);
      }
   }
   return(-1);
}

//============================================================================
//============================================================================
void BAchievementManager::setupTickerInfo() 
{
   BUString tempString = "";
   BUser* pUser = gUserManager.getPrimaryUser();
   if(!pUser)
      return;

   int lastAchievement = -1;
   int lastAchievementTime = -1;
   int numAchievements = 0;
   for (int i=0; i<mRules.getNumber(); i++)
   {
      //if (!mRules[i]->isType(cAT_None))
 
      //BAchievementProto* pRule = mRules[i];

      BUserAchievementList * pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {
         BUserAchievement * pA = pAchievements->getAchievement(i);
         if (pA)
         {
            if(pA->isGranted())
            {
               if((int)pA->getGrantGameTime() > lastAchievementTime)
               {
                  lastAchievementTime = pA->getGrantGameTime();
                  lastAchievement = i;
               }
               numAchievements++;
            }
         }
      }
   }

   //number of achievements //Achievements awarded: %1!d! out of %2!d!
   if( numAchievements > 0 )
   {
      tempString.locFormat(gDatabase.getLocStringFromID(25710), numAchievements, mRules.getNumber());
      BUITicker::addString(tempString, 3, -1, BUITicker::eNumAchievements);
   }

   //XUserCreateAchievementEnumerator(0, gUserManager.getPrimaryUser()->getPort(), INVALID_XUID, XACHIEVEMENT_DETAILS_LABEL, lastAchievement, 1, 

   //Last achievement awarded: %1!s!
   if( lastAchievement >= 0 )
   {
      BUString titleText;
      if (getLocalizedAchievementTitle(lastAchievement, titleText))
      {
         //BAchievementProto *pAchProto = mRules[lastAchievement];
         tempString.locFormat(gDatabase.getLocStringFromID(25711), titleText.getPtr());
         BUITicker::addString(tempString, 3, -1, BUITicker::eLastAchievement);
      }
   }

}

//============================================================================
bool BAchievementManager::getLocalizedAchievementTitle(long index, BUString &titleText)
{
   // [10/22/2008 xemu] get the live ID
   if ((index < 0) || (index >= mRules.getNumber()))
      return(false);

   // [10/27/2008 xemu] if we have no user, nothing to display here
   if (gUserManager.getPrimaryUser() == NULL)
      return(false);
   if (!gUserManager.getPrimaryUser()->isSignedIn())
      return(false);

   DWORD liveIndex = mRules[index]->getLiveIndex();

   // [10/22/2008 xemu] adapted from XDK sample 
   HANDLE hEnum;
   DWORD cbBuffer;

   // Create enumerator for the default device
   DWORD dwStatus;
   dwStatus = XUserCreateAchievementEnumerator(
      0,                              // Enumerate for the current title
      gUserManager.getPrimaryUser()->getPort(),
      INVALID_XUID,                       // If INVALID_XUID, the current user's achievements
      // are enumerated
      XACHIEVEMENT_DETAILS_ALL,
      0,                              // starting achievement index
      mRules.getNumber(),              // number of achievements to return
      &cbBuffer,                      // bytes needed
      &hEnum );

   // [11/19/2008] Bug 18394 - If we fail to create an enumerator, return
   if (dwStatus != ERROR_SUCCESS)
      return(false);

   // Enumerate achievements
   DWORD dwAchievementCount = 0;
   DWORD dwItems;
   BYTE *achivementData = new BYTE[XACHIEVEMENT_SIZE_FULL * mRules.getNumber()];

   if( XEnumerate( hEnum, achivementData, XACHIEVEMENT_SIZE_FULL * mRules.getNumber(),
      &dwItems, NULL ) == ERROR_SUCCESS )
   {
      dwAchievementCount = dwItems;
   }

   // Retrieve achievement info
   XACHIEVEMENT_DETAILS* rgAchievements = ( XACHIEVEMENT_DETAILS* )achivementData;

   for( DWORD i = 0; i < dwAchievementCount; ++i )
   {
      if (rgAchievements[i].dwId == liveIndex)
      {
         titleText.set(rgAchievements[i].pwszLabel);
         CloseHandle( hEnum );
         delete achivementData;
         return(true);
      }
   }

   CloseHandle( hEnum );
   delete achivementData;
   return(false);
}

//============================================================================
void BAchievementManager::fixupEarnedAchievements(BUser *pUser)
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigFakeAchievements))
      return;
#endif

   // [11/21/2008 xemu] look for all the achievements we think we have, and make sure they have all actually been granted properly
   const BUserAchievementList *pUserAchievementList = pUser->getAchievementList();
   if (pUserAchievementList == NULL)
      return;

   // [11/21/2008 xemu] first get the set of actual achievements out of the XDK

   // Create enumerator for the default device
   HANDLE hEnum;
   DWORD dwStatus;
   DWORD cbBuffer;
   dwStatus = XUserCreateAchievementEnumerator(
      0,                              // Enumerate for the current title
      pUser->getPort(),
      INVALID_XUID,                       // If INVALID_XUID, the current user's achievements
      // are enumerated
      XACHIEVEMENT_DETAILS_ALL,
      0,                              // starting achievement index
      mRules.getNumber(),              // number of achievements to return
      &cbBuffer,                      // bytes needed
      &hEnum );

   // [11/19/2008] Bug 18394 - If we fail to create an enumerator, return
   if (dwStatus != ERROR_SUCCESS)
      return;

   // Enumerate achievements
   DWORD dwAchievementCount = 0;
   DWORD dwItems;
   BYTE *achivementData = new BYTE[XACHIEVEMENT_SIZE_FULL * mRules.getNumber()];

   if( XEnumerate( hEnum, achivementData, XACHIEVEMENT_SIZE_FULL * mRules.getNumber(),
      &dwItems, NULL ) == ERROR_SUCCESS )
   {
      dwAchievementCount = dwItems;
   }

   // Retrieve achievement info
   XACHIEVEMENT_DETAILS* rgAchievements = ( XACHIEVEMENT_DETAILS* )achivementData;

   // [11/21/2008 xemu] now go through each achievement, and make sure we match up 
   for( DWORD i = 0; i < dwAchievementCount; ++i )
   {
      // [11/21/2008 xemu] only check for Achievements we haven't earned, since we only really care about that case, not the converse
      if (!AchievementEarned(rgAchievements[i].dwFlags))
      {
         // [11/21/2008 xemu] find the achievement index that matches this liveIndex
         // [11/21/2008 xemu] this whole algorithm is kind of awful and N-squared, but trying to stick with a safe & simple fix right before ZBR 
         int achIndex;
         for (achIndex=0; achIndex<mRules.getNumber(); achIndex++)
         {
            BAchievementProto *pProtoAch = mRules[achIndex];
            if (pProtoAch == NULL)
               continue;

            if (pProtoAch->getLiveIndex() == rgAchievements[i].dwId)
            {
               const BUserAchievement *pUserAch = pUserAchievementList->getAchievement(achIndex);
               if (pUserAch == NULL)
                  continue;

               if (pUserAch->isGranted())
               {
                  // [11/21/2008 xemu] ok, this is the case we really care about -- we think achievement is granted, but the real system does not 
                  // [11/21/2008 xemu] so fix the situation by re-granting the Achievement
                  grantAchievement(pUser, rgAchievements[i].dwId);
               }
            }
         }
      }
   }


   // [11/21/2008 xemu] clean up the XDK stuff 
   CloseHandle( hEnum );
   delete achivementData;
}
