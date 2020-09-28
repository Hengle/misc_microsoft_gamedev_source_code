//==============================================================================
// scoreManager.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "scoreManager.h"
#include "world.h"
//#include "unit.h"
#include "gamesettings.h"
#include "protosquad.h"

#include "userprofilemanager.h"
#include "LiveSystem.h"

// Constants

// Globals
BScoreManager gScoreManager;

static float cScoreTimeBonusMin = 0.0f;
static float cScoreTimeBonusMax = 5.0f;

GFIMPLEMENTVERSION(BScoreManager, 4);

//==============================================================================
// Note: This sorts from highest (1st) to lowest (last place)
//==============================================================================
static int __cdecl BScorePlayerCompareFunc(const void* pElem1, const void* pElem2)
{
   BScorePlayer* pPlayer1 = (BScorePlayer*)pElem1;
   BScorePlayer* pPlayer2 = (BScorePlayer*)pElem2;

   if (pPlayer1->mFinalScore > pPlayer2->mFinalScore)
      return -1;
   if (pPlayer1->mFinalScore < pPlayer2->mFinalScore)
      return 1;
   return 0;
}

//============================================================================
//============================================================================
//BScorePlayer
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BScorePlayer::BScorePlayer()
{
   mFinalScoreCalculated = false;
   mCampaignScoreImproved = false;
   mLastUpdateTime = 0;
   mPlayerID = -1;
   mSquadsKilled = 0;
   mSquadsLost = 0;
   mCombatValueKilled = 0.0f;
   mCombatValueLost = 0.0f;
   mSkullBonusCombatValue = 0.0f;
   mSkullBonusObjectives = 0.0f;

   mMissionCompletionTime = 0;
   mNumPrimaryObjectives = 0;
   mNumPrimaryObjectivesCompleted = 0;
   mNumOptionalObjectives = 0;
   mNumOptionalObjectivesCompleted = 0;

   mTimeBonus = 0.0f;
   mCombatBonus = 0.0f;
   mMPBonus = 0.0f;

   mObjectiveScore = 0;
   mBaseScore = 0;

   mFinalObjectiveScore = 0;
   mFinalCombatScore = 0;
   mFinalBaseScore = 0;
   mFinalCombatBonus = 0;
   mFinalTimeBonus = 0;
   mFinalSkullBonus = 0;
   mFinalScore = 0;
   mFinalGrade = 0;
}

//============================================================================
//============================================================================
BScorePlayer::~BScorePlayer()
{
}


//============================================================================
//============================================================================
//BScoreScenario
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BScoreScenario::BScoreScenario()
{
   reset();
}

//============================================================================
//============================================================================
BScoreScenario::~BScoreScenario()
{
}

//============================================================================
//============================================================================
void BScoreScenario::reset()
{
   mScenarioID = -1;

   mCombatBonusMin = 0.0f;
   mCombatBonusMax = 5.0f;

   mMissionMinParTime = 0;
   mMissionMaxParTime = 0;

   mScoreGrade[0] = 0;
}


//============================================================================
//============================================================================
//BScoreManager
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BScoreManager::BScoreManager()
{
   reset();
}

//============================================================================
//============================================================================
BScoreManager::~BScoreManager()
{
   reset();
}

//============================================================================
//============================================================================
void BScoreManager::reset()
{
      mPlayers.clear();
      mScenarioDataSet = false;
      mSkullModifier = 1.0f;
      mFinalScoresCalculated = false;

      mGradeToStringMap[0] = 24777;
      mGradeToStringMap[1] = 24778;
      mGradeToStringMap[2] = 24779;
      mGradeToStringMap[3] = 24780;
      mGradeToStringMap[4] = 24781;

      BASSERT(SCORE_NUM_GRADES == 3);

      mCurrentScenarioData.reset();
}

//============================================================================
//============================================================================
void BScoreManager::setScenarioData( long scenarioID, float combatBonusMin, float combatBonusMax, DWORD missionMinParTime, DWORD missionMaxParTime, long gradeA, long gradeB, long gradeC, long gradeD )
{
   mCurrentScenarioData.mScenarioID = scenarioID;

   mCurrentScenarioData.mCombatBonusMin = combatBonusMin;
   mCurrentScenarioData.mCombatBonusMax = combatBonusMax;

   mCurrentScenarioData.mMissionMinParTime = missionMinParTime;  //time in milliseconds
   mCurrentScenarioData.mMissionMaxParTime = missionMaxParTime;

   mCurrentScenarioData.mScoreGrade[0] = gradeA;
   mCurrentScenarioData.mScoreGrade[1] = gradeB;
   mCurrentScenarioData.mScoreGrade[2] = gradeC;
   // [10/20/2008 xemu] this array is only size 3! 
   //mCurrentScenarioData.mScoreGrade[3] = gradeD;

   mScenarioDataSet = true;
}

//============================================================================
//============================================================================
const void BScoreManager::queryPlayers(BSmallDynamicSimArray<BScorePlayer>& players)
{
   uint count = mPlayers.getSize();
   for (uint i=0; i < count; ++i)
      update(mPlayers[i].mPlayerID);

   players = mPlayers;

   // highest score to lowest score
   players.sort(BScorePlayerCompareFunc);
}

//============================================================================
//============================================================================
void BScoreManager::initializePlayers()
{
   long numPlayers = gWorld->getNumberPlayers();

   int count = 0;
   for (long i=1; i<numPlayers; i++)            // start at 1 to filter out gaia.
   {
      BPlayer* player = gWorld->getPlayer(i);
      if (!player)
         continue;

      // filter out npcs
      if (player->isNPC())
         continue;

      // ok we have somebody we like (AI or Human)
      count++;
   }

   // fail check
   if (count==0)
      return;

   // initialize our player cache
   mPlayers.setNumber(count);

   // Init the values
   for (long i=0; i<count; i++)
   {
      BScorePlayer *playerScore = &mPlayers.get(i);

      playerScore->mFinalScoreCalculated = false;
      playerScore->mCampaignScoreImproved = false;
      playerScore->mLastUpdateTime = 0;
      playerScore->mPlayerID = -1;
      playerScore->mSquadsKilled = 0;
      playerScore->mSquadsLost = 0;
      playerScore->mCombatValueKilled = 0.0f;
      playerScore->mCombatValueLost = 0.0f;
      playerScore->mSkullBonusCombatValue = 0.0f;
      playerScore->mSkullBonusObjectives = 0.0f;

      playerScore->mMissionCompletionTime = 0;
      playerScore->mNumPrimaryObjectives = 0;
      playerScore->mNumPrimaryObjectivesCompleted = 0;
      playerScore->mNumOptionalObjectives = 0;
      playerScore->mNumOptionalObjectivesCompleted = 0;

      playerScore->mTimeBonus = 0.0f;
      playerScore->mCombatBonus = 0.0f;
      playerScore->mMPBonus = 0.0f;

      playerScore->mObjectiveScore = 0;
      playerScore->mBaseScore = 0;

      playerScore->mFinalObjectiveScore = 0;
      playerScore->mFinalCombatScore = 0;
      playerScore->mFinalBaseScore = 0;
      playerScore->mFinalCombatBonus = 0;
      playerScore->mFinalTimeBonus = 0;
      playerScore->mFinalSkullBonus = 0;
      playerScore->mFinalScore = 0;
      playerScore->mFinalGrade = 0;
   }

   // now put the player ID into the player cache on our end for easier look up later.
   int index=0;
   for (long i=1; i<numPlayers; i++)            // start at 1 to filter out gaia.
   {
      BPlayer* player = gWorld->getPlayer(i);
      if (!player)
         continue;

      // filter out npcs
      if (player->isNPC())
         continue;

      // save the ID in our cache.
      BScorePlayer *playerScore = &mPlayers.get(index);
      playerScore->mPlayerID = player->getID();

      // next
      index++;
   }
}

//============================================================================
//============================================================================
BScorePlayer * BScoreManager::getPlayerByID(long id)
{
   for (int i=0; i<mPlayers.getNumber(); i++)
   {
      if (mPlayers[i].mPlayerID == id)
         return &mPlayers.get(i);
   }

   return NULL;
}

//============================================================================
//============================================================================
void BScoreManager::update(long playerID, bool force)
{
//   BScorePlayer *playerScore = &mPlayers.get(playerID);
   BScorePlayer *playerScore = getPlayerByID(playerID);
   if (!playerScore)
      return;

   DWORD gameTime = gWorld->getGametime();
   if( !force && (gameTime == playerScore->mLastUpdateTime) )
      return;

   bool playerIsCoopPlayer = false;
   if (gWorld->getFlagCoop())
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if( pPlayer && pPlayer->isHuman() )
         playerIsCoopPlayer = true;
   }


   //Get the some ancillary objective data
   int numCompleted = 0;
   int numPrimaryCompleted = 0;
   int numIncomplete = 0;
   int numTotal = 0;
   int maxScore = 0;
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();
   if (pObjectiveManager)
   {
      numTotal = pObjectiveManager->getNumberObjectives();
      for (int i = 0; i < numTotal; i++)
      {
         uint objectiveScore = pObjectiveManager->getObjectiveScore(i);
         if( playerIsCoopPlayer )
            objectiveScore /= 2;
         if (pObjectiveManager->getObjectiveCompleted(i))
         {
            numCompleted++;
            if(pObjectiveManager->getObjectiveRequired(i))
               numPrimaryCompleted++;
         }          
         maxScore += objectiveScore;
      }
      numIncomplete = numTotal - numCompleted;

      playerScore->mNumPrimaryObjectives = pObjectiveManager->getNumberRequiredObjectives();
      playerScore->mNumPrimaryObjectivesCompleted = numPrimaryCompleted;
      playerScore->mNumOptionalObjectives = numTotal - playerScore->mNumPrimaryObjectives;
      playerScore->mNumOptionalObjectivesCompleted = numCompleted - numPrimaryCompleted;
   }


   //Compute the base score
   // [10/22/2008 xemu] in campaign games, we only go off of objective score.
   BGameSettings *pSettings = gDatabase.getGameSettings();
   BASSERT(pSettings);
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);
   if (gameType == BGameSettings::cGameTypeSkirmish)
      playerScore->mBaseScore = playerScore->mObjectiveScore + (long)(playerScore->mCombatValueKilled);
   else
      playerScore->mBaseScore = playerScore->mObjectiveScore;

   playerScore->mLastUpdateTime = gameTime;
}


//============================================================================
//============================================================================
void BScoreManager::computeFinalScores(bool force)
{
   if( (!gWorld->getFlagGameOver() && !force) || mFinalScoresCalculated )
      return;

   BGameSettings *pSettings = gDatabase.getGameSettings();
   BASSERT(pSettings);
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);

   DWORD gameTime = gWorld->getGametime();

   //first update all of the base scores
   for (int playerID=0; playerID<gWorld->getNumberPlayers(); playerID++)
   {
      update(playerID, true);
   }

   //Now compute the final scores.
   for (int playerID=1; playerID<gWorld->getNumberPlayers(); playerID++)      // start after gaia
   {
      // BScorePlayer *playerScore = &mPlayers.get(playerID);
      BScorePlayer *playerScore = getPlayerByID(playerID);
      if (!playerScore)
         continue;

      //If mission complete, get the completion time.
      playerScore->mMissionCompletionTime = gameTime;

      //Compute the combat bonus
      playerScore->mCombatBonus = mCurrentScenarioData.mCombatBonusMax;
      if( playerScore->mCombatValueLost > 0 )
         playerScore->mCombatBonus = (playerScore->mCombatValueKilled / playerScore->mCombatValueLost);

      // [10/28/2008 xemu] special case -- if your killed value is zero, so is your bonus!
      if (playerScore->mCombatValueKilled == 0)
         playerScore->mCombatBonus = 0;

      if( playerScore->mCombatBonus < mCurrentScenarioData.mCombatBonusMin )
         playerScore->mCombatBonus = mCurrentScenarioData.mCombatBonusMin;
      else if( playerScore->mCombatBonus > mCurrentScenarioData.mCombatBonusMax )
         playerScore->mCombatBonus = mCurrentScenarioData.mCombatBonusMax;
      //truncate bonus to 2 decimal places so that it matches in calculation with what the user sees
      // [10/28/2008 xemu] adjusted to round up those 2 decimal places 
      float tempCombatBonus = (playerScore->mCombatBonus * 100.0f);
      tempCombatBonus = ceilf(tempCombatBonus);
      playerScore->mCombatBonus = tempCombatBonus / 100.0f;

      //Compute the time Bonus
      playerScore->mTimeBonus = cScoreTimeBonusMin;
      if( playerScore->mMissionCompletionTime <= mCurrentScenarioData.mMissionMinParTime )
         playerScore->mTimeBonus = cScoreTimeBonusMax;
      else if( playerScore->mMissionCompletionTime <= mCurrentScenarioData.mMissionMaxParTime )
         playerScore->mTimeBonus = cScoreTimeBonusMin + (cScoreTimeBonusMax - cScoreTimeBonusMin)*((float)(mCurrentScenarioData.mMissionMaxParTime - playerScore->mMissionCompletionTime)/(float)(mCurrentScenarioData.mMissionMaxParTime - mCurrentScenarioData.mMissionMinParTime));
      //truncate bonus to 2 decimal places so that it matches in calculation with what the user sees
      // [10/28/2008 xemu] adjusted to round up those 2 decimal places 
      float tempTimeBonus = (playerScore->mTimeBonus * 100.0f);
      tempTimeBonus = ceilf(tempTimeBonus);
      playerScore->mTimeBonus = tempTimeBonus / 100.0f;

      playerScore->mMPBonus = 0.0f;

      //Remove the combat score if it is a scenario
      playerScore->mFinalObjectiveScore = playerScore->mObjectiveScore;
      if (gameType == BGameSettings::cGameTypeSkirmish)
      {
         playerScore->mFinalCombatScore = (long)playerScore->mCombatValueKilled;
         playerScore->mFinalSkullBonus = (long)(playerScore->mSkullBonusCombatValue + playerScore->mSkullBonusObjectives);
      }
      else
      {
         playerScore->mFinalCombatScore = 0;
         playerScore->mFinalSkullBonus = (long)(playerScore->mSkullBonusObjectives);
      }

      if (gameType == BGameSettings::cGameTypeSkirmish)
      {
         playerScore->mFinalBaseScore =   playerScore->mFinalObjectiveScore + playerScore->mFinalCombatScore;

         // determine if we played a matchmade MP game
         // determine if the player disconnected or stayed till the end
         // add the bonuses
         if( gLiveSystem && gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->isMatchmadeGame() )
         {
            const BPlayer* pPlayer = gWorld->getPlayer(playerID);
            if (pPlayer->getNetState() != BPlayer::cPlayerNetStateDisconnected)
            {
               if (pPlayer->getPlayerState() == BPlayer::cPlayerStateWon)
                  playerScore->mMPBonus = BRank::cNaturalBornKiller;
               else
                  playerScore->mMPBonus = BRank::cStickWithItMultiplier;
            }
            blog("BScoreManager::computeFinalScores - player[%s] combatBonus[%0.2f] mpBonus[%0.2f] baseScore[%d]", pPlayer->getName().getPtr(), playerScore->mCombatBonus, playerScore->mMPBonus, playerScore->mFinalBaseScore);
         }
      }
      else
         playerScore->mFinalBaseScore =   playerScore->mFinalObjectiveScore;

      playerScore->mFinalCombatBonus = (long)((float)playerScore->mFinalBaseScore * playerScore->mCombatBonus);
      playerScore->mFinalTimeBonus =   (long)((float)playerScore->mFinalBaseScore * playerScore->mTimeBonus);

      playerScore->mFinalScore = (long)((1.0f + playerScore->mMPBonus) * (playerScore->mFinalBaseScore + playerScore->mFinalCombatBonus + playerScore->mFinalTimeBonus + playerScore->mFinalSkullBonus));
   }

   //Now compute the grades.  we have to do this afterwards for coop games so that the final scores can be added together
   for (int playerID=1; playerID<gWorld->getNumberPlayers(); playerID++)      // start after gaia
   {
      // BScorePlayer *playerScore = &mPlayers.get(playerID);
      BScorePlayer *playerScore = getPlayerByID(playerID);
      if (!playerScore)
         continue;

      long finalScore = playerScore->mFinalScore;

      //Add the coop player's score into ours if we are playing coop
      if (gWorld->getFlagCoop())
      {
         BPlayer* pPlayer = gWorld->getPlayer(playerID);
         if (pPlayer->isHuman())
         {
            //Find my team mate
            BScorePlayer *playerScore2 = NULL;
            for(int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer *pTestPlayer = gWorld->getPlayer(i);
               if( (i!=pPlayer->getID()) && pTestPlayer->isHuman() && (pTestPlayer->getTeamID() == pPlayer->getTeamID()) )
               {
                  playerScore2 = getPlayerByID(i);
                  break;
               }
            }

            if (playerScore2)
            {
               finalScore += playerScore2->mFinalScore;
            }
         }
      }

      //Compute the grade (0 is no grade, 1 best, SCORE_NUM_GRADES worst)  
      int grade = 4;
      for(int i=0; i<SCORE_NUM_GRADES; i++)
      {
         if( finalScore >= mCurrentScenarioData.mScoreGrade[i] )
         {
            grade = i+1;
            break;
         }
      }
      playerScore->mFinalGrade = grade;
      

      gConsoleOutput.debug("Player %d", playerScore->mPlayerID);
      gConsoleOutput.debug("  mCombatValueKilled %f", playerScore->mCombatValueKilled);
      gConsoleOutput.debug("  mCombatValueLost %f", playerScore->mCombatValueLost);
      gConsoleOutput.debug("  mCombatBonus %f", playerScore->mCombatBonus);
      gConsoleOutput.debug("  mMissionCompletionTime %d", playerScore->mMissionCompletionTime);
      gConsoleOutput.debug("  mTimeBonus %f", playerScore->mTimeBonus);
      gConsoleOutput.debug("  mObjectiveScore %d", playerScore->mObjectiveScore);
      gConsoleOutput.debug("  mBaseScore %d", playerScore->mBaseScore);
      gConsoleOutput.debug("  mFinalObjectiveScore %d", playerScore->mFinalObjectiveScore);
      gConsoleOutput.debug("  mFinalCombatScore %d", playerScore->mFinalCombatScore);
      gConsoleOutput.debug("  mFinalBaseScore %d", playerScore->mFinalBaseScore);
      gConsoleOutput.debug("  mFinalCombatBonus %d", playerScore->mFinalCombatBonus);
      gConsoleOutput.debug("  mFinalTimeBonus %d", playerScore->mFinalTimeBonus);
      gConsoleOutput.debug("  mFinalSkullBonus %d", playerScore->mFinalSkullBonus);
      gConsoleOutput.debug("  mFinalScore %d", playerScore->mFinalScore);
      gConsoleOutput.debug("  mFinalGrade %d", playerScore->mFinalGrade);
   }

   mFinalScoresCalculated = true;
}

//============================================================================
//============================================================================
void BScoreManager::reportSquadKilled( long playerID, BEntityID targetID, long number)
{
   BSquad *pTarget = gWorld->getSquad(targetID);
   BASSERT(pTarget);

   BScorePlayer * pPlayer = getPlayerByID(playerID);
   BScorePlayer * pPlayerTarget = getPlayerByID(pTarget->getPlayerID());

   BSimString name;
   float value = 0;
   if( pTarget->getProtoSquad() )
   {
      value = pTarget->getProtoSquad()->getCombatValue();
   }

   if (pPlayer)
   {
      pPlayer->mSquadsKilled += 1;
      pPlayer->mCombatValueKilled += value;
      pPlayer->mSkullBonusCombatValue += (value * (gScoreManager.getSkullModifier()-1.0f));
      BSimString buffer;
      //gConsoleOutput.debug("player %d  kills %d   value %f   mCombatValueKilled %f   mSkullBonusCombatValue %f  targetID %d %s", 
      //   pPlayer->mPlayerID, pPlayer->mUnitsKilled, value, pPlayer->mCombatValueKilled, pPlayer->mSkullBonusCombatValue, targetID, name.asANSI(buffer));
   }

   if (pPlayerTarget)
   {
      pPlayerTarget->mSquadsLost += 1;
      pPlayerTarget->mCombatValueLost += value;
   }
}

//============================================================================
//============================================================================
void BScoreManager::reportObjectiveCompleted( bool completed, long value )
{
   //Add objective score for all human players.
   for( int i=0; i<mPlayers.getNumber(); i++ )
   {
      BScorePlayer * pPlayerScore = &mPlayers[i];

      if (pPlayerScore)
      {
         BPlayer* pPlayer = gWorld->getPlayer(pPlayerScore->mPlayerID);
         if (pPlayer && pPlayer->isHuman())
         {
            long useValue = value;
            //If coop, halve the value.
            if (gWorld->getFlagCoop())
               useValue = value / 2;

            if (completed)
            {
               pPlayerScore->mObjectiveScore += value;
               pPlayerScore->mSkullBonusObjectives += ((float)value * (gScoreManager.getSkullModifier()-1.0f));
            }
            else
            {
               pPlayerScore->mObjectiveScore -= value;
               pPlayerScore->mSkullBonusObjectives -= ((float)value * (gScoreManager.getSkullModifier()-1.0f));
            }

            //gConsoleOutput.debug("player %d  value %d   mObjectiveScore %d   mSkullBonusObjectives %f", pPlayerScore->mPlayerID, value, pPlayerScore->mObjectiveScore, pPlayerScore->mSkullBonusObjectives);
         }
      }
   }
}

#ifndef BUILD_FINAL
//============================================================================
//============================================================================
void BScoreManager::reportBogusKill( long playerID )
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return;

   float value = 10000;

   pPlayer->mSquadsKilled += 10000;
   pPlayer->mCombatValueKilled += value;
   pPlayer->mObjectiveScore += 10000;
}
#endif

//============================================================================
//============================================================================
void BScoreManager::reportCampaignScoreImproved( long playerID )
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return;

   pPlayer->mCampaignScoreImproved = true;
}

//============================================================================
//============================================================================
bool BScoreManager::wasCampaignScoreImproved( long playerID )
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return false;


   return pPlayer->mCampaignScoreImproved;
}

//============================================================================
//============================================================================
long BScoreManager::getBaseScore(long playerID)
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mBaseScore;
}


//============================================================================
//============================================================================
bool BScoreManager::getFinalScoreValues(long playerID, long &objScore, long &combatScore, long &baseScore, long &combatBonus, float &combatBonusPercent, 
                         long &timeBonus, float &timeBonusPercent, long &skullBonus, long &finalScore, long &finalGrade)
{
   computeFinalScores();

   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return false;

   update(playerID);

   objScore =    pPlayer->mFinalObjectiveScore;
   combatScore = pPlayer->mFinalCombatScore;
   baseScore =   pPlayer->mFinalBaseScore;
   combatBonus = pPlayer->mFinalCombatBonus;
   timeBonus =   pPlayer->mFinalTimeBonus;
   skullBonus =  pPlayer->mFinalSkullBonus;
   finalScore =  pPlayer->mFinalScore;
   finalGrade =  pPlayer->mFinalGrade;
   combatBonusPercent = pPlayer->mCombatBonus;
   timeBonusPercent =   pPlayer->mTimeBonus;

   return true;
}

//============================================================================
//============================================================================
long BScoreManager::getFinalScore(long playerID)
{
   computeFinalScores();

   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mFinalScore;
}

//============================================================================
//============================================================================
long BScoreManager::getFinalGrade(long playerID)
{
   computeFinalScores();

   //BASSERT(mScenarioDataSet);
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mFinalGrade;
}

//============================================================================
//============================================================================
long BScoreManager::getScoreRequiredForGrade(long grade)
{
   //BASSERT(mScenarioDataSet);
   if((grade<1) || (grade>SCORE_NUM_GRADES))
      return 0;

   return mCurrentScenarioData.mScoreGrade[grade-1];
}

//============================================================================
//============================================================================
const BUString& BScoreManager::getStringForGrade(long grade)
{
   //BASSERT(mScenarioDataSet);
   // [10/20/2008 xemu] the awful +1 is here because there is a 4th "grade" for the Tin medals 
   if((grade<1) || (grade>SCORE_NUM_GRADES+1))
      return gDatabase.getLocStringFromID(mGradeToStringMap[0]);  //Zero is no grade

   return gDatabase.getLocStringFromID(mGradeToStringMap[grade]);
}

//============================================================================
//============================================================================
long BScoreManager::getStringIDForGrade(long grade)
{
   //BASSERT(mScenarioDataSet);
   // [10/20/2008 xemu] the awful +1 is here because there is a 4th "grade" for the Tin medals 
   if((grade<1) || (grade>SCORE_NUM_GRADES+1))
      return mGradeToStringMap[0];  //Zero is no grade
   
   return mGradeToStringMap[grade];
}

////============================================================================
////============================================================================
//float BScoreManager::getTimeBonusPercent(long playerID)
//{
//   BScorePlayer * pPlayer = getPlayerByID(playerID);
//   if (!pPlayer)
//      return 0;
//
//   update(playerID);
//   return pPlayer->mTimeBonus;
//}

//============================================================================
//============================================================================
float BScoreManager::getCombatBonusPercent(long playerID)
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mCombatBonus;
}

//============================================================================
//============================================================================
DWORD BScoreManager::getCompletionTime(long playerID)
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mMissionCompletionTime;
}

//============================================================================
//============================================================================
long BScoreManager::getHighestFinalScore()
{
   long highscore = 0;
   for (int i=0; i<mPlayers.getNumber(); i++)
   {
      // [10/27/2008 xemu] fixed an index vs player ID bug 
      long playerID = mPlayers[i].mPlayerID;
      long playerscore = getFinalScore(playerID);
      if(playerscore > highscore)
         highscore = playerscore;
   }
   return highscore;
}

//============================================================================
//============================================================================
int BScoreManager::getOptionalObjectivesTotal(long playerID)
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mNumOptionalObjectives;
}

//============================================================================
//============================================================================
int BScoreManager::getOptionalObjectivesCompleted(long playerID)
{
   BScorePlayer * pPlayer = getPlayerByID(playerID);
   if (!pPlayer)
      return 0;

   update(playerID);
   return pPlayer->mNumOptionalObjectivesCompleted;
}

//============================================================================
//============================================================================
bool BScoreManager::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSARRAY(pStream, saveType, mPlayers, uint8, 16);
   GFWRITECLASS(pStream, saveType, mCurrentScenarioData);
   // [10/20/2008 xemu] removed this, as it is static data and doesn't need to be in a save 
   //GFWRITEPTR(pStream, sizeof(long)*SCORE_NUM_GRADES+1, mGradeToStringMap);
   GFWRITEVAR(pStream, float, mSkullModifier);
   GFWRITEVAR(pStream, bool, mScenarioDataSet);
   GFWRITEVAR(pStream, bool, mScenarioDataSet);
   GFWRITEVAR(pStream, bool, mFinalScoresCalculated);
   return true;
}

//============================================================================
//============================================================================
bool BScoreManager::load(BStream* pStream, int saveType)
{
   GFREADCLASSARRAY(pStream, saveType, mPlayers, uint8, 16);
   GFREADCLASS(pStream, saveType, mCurrentScenarioData);
   if (BScoreManager::mGameFileVersion <= 3)
   {
      // [10/20/2008 xemu] note this is reading +1 even though the size is now +2, but we aren't even saving it out anymore... 
      GFREADPTR(pStream, sizeof(long)*SCORE_NUM_GRADES+1, mGradeToStringMap);
   }
   GFREADVAR(pStream, float, mSkullModifier);
   GFREADVAR(pStream, bool, mScenarioDataSet);
   GFREADVAR(pStream, bool, mScenarioDataSet);
   GFREADVAR(pStream, bool, mFinalScoresCalculated);
   return true;
}

//============================================================================
//============================================================================
bool BScorePlayer::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mPlayerID);
   GFWRITEVAR(pStream, DWORD, mLastUpdateTime);
   GFWRITEVAR(pStream, bool, mFinalScoreCalculated);
   GFWRITEVAR(pStream, bool, mCampaignScoreImproved);
   GFWRITEVAR(pStream, long, mSquadsKilled);
   GFWRITEVAR(pStream, long, mSquadsLost);
   GFWRITEVAR(pStream, float, mCombatValueKilled);
   GFWRITEVAR(pStream, float, mCombatValueLost);
   GFWRITEVAR(pStream, float, mSkullBonusCombatValue);
   GFWRITEVAR(pStream, float, mSkullBonusObjectives);
   GFWRITEVAR(pStream, DWORD, mMissionCompletionTime);
   GFWRITEVAR(pStream, int, mNumPrimaryObjectives);
   GFWRITEVAR(pStream, int, mNumPrimaryObjectivesCompleted);
   GFWRITEVAR(pStream, int, mNumOptionalObjectives);
   GFWRITEVAR(pStream, int, mNumOptionalObjectivesCompleted);
   GFWRITEVAR(pStream, float, mTimeBonus);
   GFWRITEVAR(pStream, float, mCombatBonus);
   GFWRITEVAR(pStream, long, mObjectiveScore);
   GFWRITEVAR(pStream, long, mBaseScore);
   GFWRITEVAR(pStream, long, mFinalObjectiveScore);
   GFWRITEVAR(pStream, long, mFinalCombatScore);
   GFWRITEVAR(pStream, long, mFinalBaseScore);
   GFWRITEVAR(pStream, long, mFinalCombatBonus);
   GFWRITEVAR(pStream, long, mFinalTimeBonus);
   GFWRITEVAR(pStream, long, mFinalSkullBonus);
   GFWRITEVAR(pStream, long, mFinalScore);
   GFWRITEVAR(pStream, long, mFinalGrade);
   return true;
}

//============================================================================
//============================================================================
bool BScorePlayer::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mPlayerID);
   GFREADVAR(pStream, DWORD, mLastUpdateTime);
   GFREADVAR(pStream, bool, mFinalScoreCalculated);
   if (BScoreManager::mGameFileVersion >=2)
      GFREADVAR(pStream, bool, mCampaignScoreImproved);
   GFREADVAR(pStream, long, mSquadsKilled);
   GFREADVAR(pStream, long, mSquadsLost);
   GFREADVAR(pStream, float, mCombatValueKilled);
   GFREADVAR(pStream, float, mCombatValueLost);
   if (BScoreManager::mGameFileVersion >=3)
   {
      GFREADVAR(pStream, float, mSkullBonusCombatValue);
      GFREADVAR(pStream, float, mSkullBonusObjectives);
   }
   GFREADVAR(pStream, DWORD, mMissionCompletionTime);
   GFREADVAR(pStream, int, mNumPrimaryObjectives);
   GFREADVAR(pStream, int, mNumPrimaryObjectivesCompleted);
   GFREADVAR(pStream, int, mNumOptionalObjectives);
   GFREADVAR(pStream, int, mNumOptionalObjectivesCompleted);
   GFREADVAR(pStream, float, mTimeBonus);
   GFREADVAR(pStream, float, mCombatBonus);
   GFREADVAR(pStream, long, mObjectiveScore);
   GFREADVAR(pStream, long, mBaseScore);
   if (BScoreManager::mGameFileVersion >=3)
   {
      GFREADVAR(pStream, long, mFinalObjectiveScore);
      GFREADVAR(pStream, long, mFinalCombatScore);
      GFREADVAR(pStream, long, mFinalBaseScore);
      GFREADVAR(pStream, long, mFinalCombatBonus);
      GFREADVAR(pStream, long, mFinalTimeBonus);
      GFREADVAR(pStream, long, mFinalSkullBonus);
   }
   GFREADVAR(pStream, long, mFinalScore);
   GFREADVAR(pStream, long, mFinalGrade);
   return true;
}

//============================================================================
//============================================================================
bool BScoreScenario::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mScenarioID);
   GFWRITEVAR(pStream, float, mCombatBonusMin);
   GFWRITEVAR(pStream, float, mCombatBonusMax);
   GFWRITEVAR(pStream, DWORD, mMissionMinParTime);
   GFWRITEVAR(pStream, DWORD, mMissionMaxParTime);
   GFWRITEPTR(pStream, sizeof(long)*SCORE_NUM_GRADES, mScoreGrade);
   return true;
}

//============================================================================
//============================================================================
bool BScoreScenario::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mScenarioID);
   GFREADVAR(pStream, float, mCombatBonusMin);
   GFREADVAR(pStream, float, mCombatBonusMax);
   GFREADVAR(pStream, DWORD, mMissionMinParTime);
   GFREADVAR(pStream, DWORD, mMissionMaxParTime);
   GFREADPTR(pStream, sizeof(long)*SCORE_NUM_GRADES, mScoreGrade);
   return true;
}

