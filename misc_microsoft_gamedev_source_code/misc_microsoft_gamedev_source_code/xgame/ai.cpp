//==============================================================================
// ai.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "bidMgr.h"
#include "camera.h"
#include "configsgame.h"
#include "database.h"
#include "fontsystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "kb.h"
#include "kbbase.h"
#include "kbsquad.h"
#include "logmanager.h"
#include "player.h"
#include "protosquad.h"
#include "render.h"
#include "user.h"
#include "usermanager.h"
#include "team.h"
#include "triggermanager.h"
#include "humanPlayerAITrackingData.h"
//#include "userprofilemanager.h"
#include "world.h"

GFIMPLEMENTVERSION(BAI, 5);
GFIMPLEMENTVERSION(BAIGlobals, 3);
GFIMPLEMENTVERSION(BAIDifficultySetting, 1);

enum
{
   cSaveMarkerAIGlobals=10000,
   cSaveMarkerBidManager,
   cSaveMarkerAI,
};

//==============================================================================
//==============================================================================
bool BMissionTargetScoreSortFunctor::operator() (const BAIMissionTargetID leftTargetID, const BAIMissionTargetID rightTargetID) const
{
   const BAIMissionTarget* pLeftTarget = gWorld->getAIMissionTarget(leftTargetID);
   const BAIMissionTarget* pRightTarget = gWorld->getAIMissionTarget(rightTargetID);
   float leftScore = pLeftTarget ? pLeftTarget->getScore().getTotal() : 0.0f;
   float rightScore = pRightTarget ? pRightTarget->getScore().getTotal() : 0.0f;
   return (leftScore > rightScore);
}

//==============================================================================
//==============================================================================
float BAIGlobals::getPlayerModifier(BPlayerID playerID)
{
   if ( (playerID > cMaximumSupportedPlayers) || (playerID < 0))
      return(1.0f);
   else
      return(mPlayerModifiers[playerID]);
}

//==============================================================================
//==============================================================================
void BAIGlobals::setPlayerModifier(BPlayerID playerID, float modifier)
{
   if ( (playerID > cMaximumSupportedPlayers) || (playerID < 0))
      return;
   else
      mPlayerModifiers[playerID] = modifier;
}

//==============================================================================
//==============================================================================
DWORD BAI::getThinkCostMin() const
{
   return (mThinkCostMin);
}

//==============================================================================
//==============================================================================
DWORD BAI::getThinkCostMax() const
{
   return (mThinkCostMax);
}

//==============================================================================
//==============================================================================
DWORD BAI::getActCostMin() const
{
   return (mActCostMin);
}

//==============================================================================
//==============================================================================
DWORD BAI::getActCostMax() const
{
   return (mActCostMax);
}


//==============================================================================
//==============================================================================
DWORD BAI::getPowerEvalIntervalMin() const
{
   return (mPowerEvalIntervalMin);
}

//==============================================================================
//==============================================================================
DWORD BAI::getPowerEvalIntervalMax() const
{
   return (mPowerEvalIntervalMax);
}

//==============================================================================
//==============================================================================
BAIDifficultySetting::BAIDifficultySetting()
{
   reset();
}


//==============================================================================
//==============================================================================
BAIDifficultySetting::~BAIDifficultySetting()
{
   reset();
}


//==============================================================================
//==============================================================================
void BAIDifficultySetting::reset()
{
   mName = "";
   mInputLabel = "";
   mOutputLabel = "";
   mComment = "";
   mFunc.reset();
   mDefaults.resize(0);
}


//==============================================================================
//==============================================================================
bool BAIDifficultySetting::loadFromXML(const BXMLNode &aiDifficultySettingNode)
{
   // Validate the root node.
   reset();
   if (aiDifficultySettingNode.getName() != "AIDifficultySetting")
      return (false);

   aiDifficultySettingNode.getAttribValueAsString("Name", mName);
   aiDifficultySettingNode.getAttribValueAsString("InputLabel", mInputLabel);
   aiDifficultySettingNode.getAttribValueAsString("OutputLabel", mOutputLabel);
   aiDifficultySettingNode.getAttribValueAsString("Comment", mComment);

   long numChildren = aiDifficultySettingNode.getNumberChildren();
   for (long i=0; i<numChildren; i++)
   {
      BXMLNode childNode(aiDifficultySettingNode.getChild(i));
      if (childNode.getName() == "PiecewiseFunc")
      {
         if (!mFunc.loadFromXML(childNode))
            return (false);
      }
      else if (childNode.getName() == "DefaultValue")
      {
         BAIDifficultySettingDefault defaultToAdd;
         if (!childNode.getAttribValueAsString("Name", defaultToAdd.mName))
            return (false);
         if (defaultToAdd.mName.isEmpty())
            return (false);
         if (!childNode.getAttribValueAsFloat("Value", defaultToAdd.mValue))
            return (false);

         mDefaults.add(defaultToAdd);
      }
   }

   return (true);
}


//==============================================================================
//==============================================================================
bool BAIDifficultySetting::canGetDefaultValueByName(const BSimString& defaultName, float& defaultValue) const
{
   uint numDefaults = mDefaults.getSize();
   for (uint i=0; i<numDefaults; i++)
   {
      if (defaultName == mDefaults[i].mName)
      {
         defaultValue = mDefaults[i].mValue;
         return (true);
      }
   }
   return (false);
}

//==============================================================================
// BAIDifficultySetting::save
//==============================================================================
bool BAIDifficultySetting::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BSimString, mName, 50);
   GFWRITESTRING(pStream, BSimString, mInputLabel, 50);
   GFWRITESTRING(pStream, BSimString, mOutputLabel, 50);
   GFWRITESTRING(pStream, BSimString, mComment, 250);

   GFWRITECLASS(pStream, saveType, mFunc);
   GFWRITECLASSARRAY(pStream, saveType, mDefaults, uint8, 100);
   return true;
}

//==============================================================================
// BAIDifficultySetting::load
//==============================================================================
bool BAIDifficultySetting::load(BStream* pStream, int saveType)
{  
   GFREADSTRING(pStream, BSimString, mName, 50);
   GFREADSTRING(pStream, BSimString, mInputLabel, 50);
   GFREADSTRING(pStream, BSimString, mOutputLabel, 50);
   GFREADSTRING(pStream, BSimString, mComment, 250);

   GFREADCLASS(pStream, saveType, mFunc);
   GFREADCLASSARRAY(pStream, saveType, mDefaults, uint8, 100);
   return true;
}

//==============================================================================
//==============================================================================
bool BAIDifficultySettingDefault::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BSimString, mName, 50);
   GFWRITEVAR(pStream, float, mValue);
   return true;
}

//==============================================================================
//==============================================================================
bool BAIDifficultySettingDefault::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BSimString, mName, 50);
   GFREADVAR(pStream, float, mValue);
   return true;
}

//==============================================================================
//==============================================================================
bool BAI::getPlayerAdaptiveDifficultyValue(float& rPlayerDifficultyValue)
{
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (!pSettings)
      return (false);
   long gameType = -1;
   if (!pSettings->getLong(BGameSettings::cGameType, gameType))
      return (false);
   if (gameType != BGameSettings::cGameTypeSkirmish)
      return (false);
   long player1DifficultyType = -1;
   if (!pSettings->getLong(BGameSettings::cPlayer1DifficultyType, player1DifficultyType))
      return (false);
   if (player1DifficultyType != DifficultyType::cAutomatic)
      return (false);
   BPlayer* pPlayerOne = gWorld->getPlayer(1);
   if (!pPlayerOne)
      return (false);
   BHumanPlayerAITrackingData* pTrackingData = pPlayerOne->getTrackingData();
   if (!pTrackingData)
      return (false);

   rPlayerDifficultyValue = pTrackingData->getAutoDifficultyLevel() * 0.01f;
   return (true);
}


//==============================================================================
//==============================================================================
BAI::BAI(BPlayerID playerID, BTeamID teamID)
{
   BFATAL_ASSERT(playerID >= 0 && playerID < cMaximumSupportedPlayers);
   BFATAL_ASSERT(teamID >= 0 && teamID < cMaximumSupportedTeams);
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BFATAL_ASSERT(pPlayer);

   mPlayerID = playerID;
   mTeamID = teamID;

   mpAIGlobals = HEAP_NEW(BAIGlobals, gSimHeap);
   BASSERT(mpAIGlobals);
   mpBidManager = HEAP_NEW(BBidManager, gSimHeap);
   mpBidManager->setPlayerID(playerID);
   BASSERT(mpBidManager);

   // Init some ability IDs.
   //mAbilityIDs[AIAbility::cUnscMarineRockets] = gDatabase.getAIDUnscMarineRockets();
   //BASSERT(mAbilityIDs[AIAbility::cUnscMarineRockets] != cInvalidAbilityID);
   //mAbilityIDs[AIAbility::cUnscWolverineBarrage] = gDatabase.getAIDUnscWolverineBarrage();
   //BASSERT(mAbilityIDs[AIAbility::cUnscWolverineBarrage] != cInvalidAbilityID);
   //mAbilityIDs[AIAbility::cUnscBarrage] = gDatabase.getAIDUnscBarrage();
   //BASSERT(mAbilityIDs[AIAbility::cUnscBarrage] != cInvalidAbilityID);
   //mAbilityIDs[AIAbility::cUnscHornetSpecial] = gDatabase.getAIDUnscHornetSpecial();
   //BASSERT(mAbilityIDs[AIAbility::cUnscHornetSpecial] != cInvalidAbilityID);
   //mAbilityIDs[AIAbility::cUnscScorpionSpecial] = gDatabase.getAIDUnscScorpionSpecial();
   //BASSERT(mAbilityIDs[AIAbility::cUnscScorpionSpecial] != cInvalidAbilityID);
   //mAbilityIDs[AIAbility::cCovGruntGrenades] = gDatabase.getAIDCovGruntGrenade();
   //BASSERT(mAbilityIDs[AIAbility::cCovGruntGrenades] != cInvalidAbilityID);

   mAIQTopic = cInvalidAITopicID;
   mAITriggerScriptID = cInvalidTriggerScriptID;
   mFlagAIQ = false;
   mFlagPaused = false;

   // Load our difficulty setting functions and recalculate the ones we care about in code.
   mMaxMissionFocusTime = 60000;
   mThinkCostMin = 1000;
   mThinkCostMax = 3000;
   mActCostMin = 3000;
   mActCostMax = 5000;
   mPowerEvalIntervalMin = 5000;
   mPowerEvalIntervalMax = 30000;

   float playerDifficulty = pPlayer->getDifficulty();
   if (getPlayerAdaptiveDifficultyValue(playerDifficulty))
      pPlayer->setDifficulty(playerDifficulty);

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      long gameType = -1;
      if (pSettings->getLong(BGameSettings::cGameType, gameType))
      {
         // Halwes - 9/11/2008 - Force SPC non-human player AI difficulty to default
         #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigAIUseSPCDefault) || ((gameType == BGameSettings::cGameTypeCampaign) && !pPlayer->isHuman()))
         #else
         if ((gameType == BGameSettings::cGameTypeCampaign) && !pPlayer->isHuman())
         #endif
         {
            playerDifficulty = gDatabase.getDifficultySPCAIDefault();
            pPlayer->setDifficulty(playerDifficulty);
         }
      }
   }
   
   loadAIDifficultySettings();
   recalculateDifficultySettings(playerDifficulty);

   #ifndef BUILD_FINAL
      BSimString logFileName = "";
      mLog = -1;
      if (gConfig.isDefined("aiLog") && (pPlayer->getID() > 0))
      {
         logFileName.format("aiLogP%1d.txt",pPlayer->getID());
         mLog = gLogManager.openLogFile(logFileName, BLogManager::cPostWriteFlush, false, 0, true, false, true);      // Returns -1 on failure.
         gLogManager.createHeader("AI", mLog, BLogManager::cBaseHeader, false, false, false, false);
         //long  BLogManager::openLogFile(const BSimString &fileName, long postWriteAction, bool append, long rollingLength,
         //   bool lineNumbering, bool headerStamp, bool createFileImmediate)
         gLogManager.setLineNumbering(mLog, false);
         gLogManager.setTitleStamp(mLog, false);
         gLogManager.setGameTimeStamp(mLog, false);
      }
   #endif

   // This will initialize the script -- if appropriate.
   initializeAIScript();
}


//==============================================================================
//==============================================================================
void BAI::initializeAIScript()
{
   // Setup AI scripts
   #ifdef SYNC_World
      syncWorldCode("BAI::initializeAIScript().");
   #endif

   // Ensure we are starting with no script for this player already.
   if (mAITriggerScriptID != cInvalidTriggerScriptID)
   {
      BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScript(mAITriggerScriptID);
      if (pTriggerScript)
         pTriggerScript->markForCleanup();
      mAITriggerScriptID = cInvalidTriggerScriptID;
   }

   const BGameSettings* pSettings = gDatabase.getGameSettings();
   if (!pSettings)
   {
      BFAIL("No game settings.");
      return;
   }

   //E3 Hack so that for the walkthrough, it will not setup the AI Scripts - Eric
   BSimString mapName;
   gConfig.get(cConfigTutorialMap, mapName);
   if (!mapName.isEmpty())
   {
      //Get the loaded map name.
      BFixedStringMaxPath loadedMapName;
      pSettings->getString(BGameSettings::cMapName, loadedMapName, MAX_PATH);      
      if (loadedMapName.comparei(mapName)==0)
         return;
   }

   // Various general cases where we will not load the script.
   if (gConfig.isDefined(cConfigAIDisable))
      return;
   if (gSaveGame.isLoading())
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return;
   if (pPlayer->isGaia())
      return;
   if (!pPlayer->isComputerAI())
      return;

   long gameType = -1;
   pSettings->getLong(BGameSettings::cGameType, gameType);
   if ((gameType == BGameSettings::cGameTypeSkirmish) || ((gameType == BGameSettings::cGameTypeScenario) && (gConfig.isDefined("SkirmishAIOnScenario"))))
   {
      // Setup AI scripts
      #ifdef SYNC_World
         syncWorldData("BAI::initializeAIScript() - Initializing SkirmishAI.triggerscript for player %d", mPlayerID);
      #endif

      // Create the new AI script and register it.
      BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, "SkirmishAI.triggerscript");
      mAITriggerScriptID = newTriggerScriptID;
      gTriggerManager.addExternalPlayerID(newTriggerScriptID, mPlayerID);

      // Ugly hack, using the external cost because there is no bool or int available.
      // A non-zero cost indicates that the AI may not attack.
      BCost attackFlag;
      attackFlag.zero();
      if (gConfig.isDefined("AINoAttack"))
         attackFlag.setAll(1.0f);
      gTriggerManager.addExternalCost(newTriggerScriptID, attackFlag);

      // Activate the trigger script.
      gTriggerManager.activateTriggerScript(newTriggerScriptID);
   }
}


//==============================================================================
//==============================================================================
BAIGlobals::BAIGlobals()
{
   mScoringStartLoc = cOriginVector;
   mScoringSquadList.resize(0);

   mBiasOffense = 0.0f;
   mBiasNodes = 0.0f;
   mBiasMainBases = 0.0f;
   mHatedPlayer = cInvalidPlayerID;

   // mrh - which of these are crap now?
   mScoringMinDistance = 200.0f;
   mScoringMaxDistance = 1200.0f;
   mTargetRecalculatePeriod = 5000;
   mMaxStalenessToTargetUnit = 15000;
   mSquadAcquireRadiusSqr = 100.0f * 100.0f;
   mMaxTargetsToShiftQueue = 5;
   mRangePenaltyRate = 0.0001f;
   mAttackMissionScoreMultiplier = 1.0f;
   mDefendMissionScoreMultiplier = 1.0f;
   for (int i=0; i<cMaximumSupportedPlayers; i++)
      mPlayerModifiers[i] = 1.0f;

   mbScoringSquadList = false;
   mbScoringStartLoc = false;
   mbOKToAttack = true;
   mbOKToDefend = true;
   mbUseMinWin = false;
   mbUseMaxWin = false;

   mMinWin = 0.0f;
   mMaxWin = 5.0f;  //just some number greater than 1 should work
}

//==============================================================================
// BAIGlobals::save
//==============================================================================
bool BAIGlobals::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mScoringStartLoc);
   GFWRITEARRAY(pStream, BEntityID, mScoringSquadList, uint16, 500);
   GFWRITEARRAY(pStream, BAIMissionID, mOpportunityRequests, uint16, 500);
   GFWRITEARRAY(pStream, BAIMissionID, mTerminalMissions, uint16, 500);

   GFWRITEVAR(pStream, float, mBiasOffense);
   GFWRITEVAR(pStream, float, mBiasNodes);
   GFWRITEVAR(pStream, float, mBiasMainBases);
   GFWRITEVAR(pStream, BPlayerID, mHatedPlayer);

   GFWRITEVAR(pStream, float, mScoringMinDistance);
   GFWRITEVAR(pStream, float, mScoringMaxDistance);
   GFWRITEVAR(pStream, DWORD, mTargetRecalculatePeriod);
   GFWRITEVAR(pStream, DWORD, mMaxStalenessToTargetUnit);
   GFWRITEVAR(pStream, float, mSquadAcquireRadiusSqr);
   GFWRITEVAR(pStream, uint, mMaxTargetsToShiftQueue);
   GFWRITEVAR(pStream, float, mRangePenaltyRate);
   GFWRITEVAR(pStream, float, mAttackMissionScoreMultiplier);
   GFWRITEVAR(pStream, float, mDefendMissionScoreMultiplier);
   GFWRITEPTR(pStream, sizeof(float)*cMaximumSupportedPlayers, mPlayerModifiers);

   GFWRITEVAR(pStream, float, mMinWin);
   GFWRITEVAR(pStream, float, mMaxWin);

   GFWRITEBITBOOL(pStream, mbScoringSquadList);
   GFWRITEBITBOOL(pStream, mbScoringStartLoc);
   GFWRITEBITBOOL(pStream, mbOKToAttack);
   GFWRITEBITBOOL(pStream, mbOKToDefend);

   GFWRITEBITBOOL(pStream, mbUseMinWin);
   GFWRITEBITBOOL(pStream, mbUseMaxWin);
   return true;
}

//==============================================================================
// BAIGlobals::load
//==============================================================================
bool BAIGlobals::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mScoringStartLoc);
   GFREADARRAY(pStream, BEntityID, mScoringSquadList, uint16, 500);
   GFREADARRAY(pStream, BAIMissionID, mOpportunityRequests, uint16, 500);
   GFREADARRAY(pStream, BAIMissionID, mTerminalMissions, uint16, 500);

   GFREADVAR(pStream, float, mBiasOffense);
   GFREADVAR(pStream, float, mBiasNodes);
   GFREADVAR(pStream, float, mBiasMainBases);
   GFREADVAR(pStream, BPlayerID, mHatedPlayer);

   GFREADVAR(pStream, float, mScoringMinDistance);
   GFREADVAR(pStream, float, mScoringMaxDistance);
   GFREADVAR(pStream, DWORD, mTargetRecalculatePeriod);
   GFREADVAR(pStream, DWORD, mMaxStalenessToTargetUnit);
   GFREADVAR(pStream, float, mSquadAcquireRadiusSqr);
   GFREADVAR(pStream, uint, mMaxTargetsToShiftQueue);
   GFREADVAR(pStream, float, mRangePenaltyRate);
   GFREADVAR(pStream, float, mAttackMissionScoreMultiplier);
   GFREADVAR(pStream, float, mDefendMissionScoreMultiplier);

   if (mGameFileVersion >= 2)
      GFREADPTR(pStream, sizeof(float)*cMaximumSupportedPlayers, mPlayerModifiers);

   if (mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, float, mMinWin);
      GFREADVAR(pStream, float, mMaxWin);
   }

   GFREADBITBOOL(pStream, mbScoringSquadList);
   GFREADBITBOOL(pStream, mbScoringStartLoc);
   GFREADBITBOOL(pStream, mbOKToAttack);
   GFREADBITBOOL(pStream, mbOKToDefend);

   if (mGameFileVersion >= 3)
   {
      GFREADBITBOOL(pStream, mbUseMinWin);
      GFREADBITBOOL(pStream, mbUseMaxWin);
   }

   return true;
}

//==============================================================================
//==============================================================================
BAI::~BAI()
{
   if (mAITriggerScriptID != cInvalidTriggerScriptID)
   {
      BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScript(mAITriggerScriptID);
      if (pTriggerScript)
         pTriggerScript->markForCleanup();
      mAITriggerScriptID = cInvalidTriggerScriptID;
   }

   BAIMissionTargetIDArray tempTargetIDs = mMissionTargetIDs;
   for (uint i=0; i<tempTargetIDs.getSize(); i++)
      gWorld->deleteAIMissionTarget(tempTargetIDs[i]);

   BAIMissionIDArray tempMissionIDs = mMissionIDs;
   for (uint i=0; i<tempMissionIDs.getSize(); i++)
      gWorld->deleteAIMission(tempMissionIDs[i]);

   BAITopicIDArray tempTopicIDs = mTopicIDs;
   for (uint i=0; i<tempTopicIDs.getSize(); i++)
      gWorld->deleteAITopic(tempTopicIDs[i]);

   if (mpBidManager)
   {
      HEAP_DELETE(mpBidManager, gSimHeap);
      mpBidManager = NULL;
   }

   if (mpAIGlobals)
   {
      HEAP_DELETE(mpAIGlobals, gSimHeap);
      mpAIGlobals = NULL;
   }

   #ifndef BUILD_FINAL
   if (mLog >= 0)
      gLogManager.closeLogFile(mLog);
   #endif
}


//==============================================================================
//==============================================================================
uint BAI::getNumActiveTopics() const
{
   uint numActiveTopics = 0;
   uint numTopicIDs = mTopicIDs.getSize();
   for (uint i=0; i<numTopicIDs; i++)
   {
      const BAITopic* pTopic = gWorld->getAITopic(mTopicIDs[i]);
      if (pTopic && pTopic->getFlagActive())
         numActiveTopics++;
   }
   return (numActiveTopics);
}


//==============================================================================
//==============================================================================
uint BAI::getNumActiveMissions() const
{
   uint numActiveMissions = 0;
   uint numMissionIDs = mMissionIDs.getSize();
   for (uint i=0; i<numMissionIDs; i++)
   {
      const BAIMission* pMission = gWorld->getAIMission(mMissionIDs[i]);
      if (pMission && pMission->getFlagActive())
         numActiveMissions++;
   }
   return (numActiveMissions);
}


//==============================================================================
//==============================================================================
void BAI::AIQUpdateLottoTickets(DWORD currentGameTime)
{
   // We should only be calling this if we have AIQ
   if (!mFlagAIQ)
      return;

   uint numTopics = mTopicIDs.getSize();
   for (uint i=0; i<numTopics; i++)
   {
      // This should never happen, but BASSERT it anyway.
      BAITopic* pAITopic = gWorld->getAITopic(mTopicIDs[i]);
      BASSERT(pAITopic);

      // Check to see if it is time to grant this topic more tickets.
      DWORD nextTicketTime = pAITopic->getTimestampNextTicketUpdate();
      if (currentGameTime >= nextTicketTime)
      {
         // We have reached or exceeded this topics next ticket time, so we are adding at least one.
         // In case we have a very short ticket interval and a very long framerate (slow), see if we 
         // have exceeded a multiple of the ticket interval and add the appropriate # of tickets.
         DWORD ticketInterval = pAITopic->getTicketInterval();
         BASSERT(ticketInterval > 0);  // If this is ever true we are dumb.
         DWORD amountTicketTimeExceededBy = currentGameTime - nextTicketTime;
         uint ticketsToAdd = 1 + (amountTicketTimeExceededBy / ticketInterval);
         // Add them.
         pAITopic->addTickets(ticketsToAdd);
         pAITopic->setNextTicketTime(nextTicketTime + (ticketInterval * ticketsToAdd));
      }
   }
}


//==============================================================================
//==============================================================================
void BAI::AIQTopicLotto(DWORD currentGameTime)
{
   // We should only be calling this if we have AIQ
   if (!mFlagAIQ)
      return;

   // Total up our tickets for the lotto, but also see if there's a pending topic that has a priority request... it gets to the front of the line...
   uint totalTickets = 0;
   uint numTopics = mTopicIDs.getSize();
   DWORD oldestRequestTime = cMaximumDWORD;
   BAITopicID newTopicID = cInvalidAITopicID;
   for (uint i=0; i<numTopics; i++)
   {
//-- FIXING PREFIX BUG ID 5201
      const BAITopic* pAITopic = gWorld->getAITopic(mTopicIDs[i]);
//--
      BASSERT(pAITopic);
      if (pAITopic->getFlagPriority() && pAITopic->getTimestampPriorityRequest() < oldestRequestTime)
      {
         newTopicID = pAITopic->getID();
         oldestRequestTime = pAITopic->getTimestampPriorityRequest();
      }
      totalTickets += pAITopic->getCurrentTickets();
   }

   // The priority part
   // Did we have a priority request?  Give it to him!
//-- FIXING PREFIX BUG ID 5202
   const BAITopic* pNewTopic = gWorld->getAITopic(newTopicID);
//--
   if (pNewTopic)
   {
      AIQSetTopic(newTopicID);
      return;
   }

   // The lotto part.
   // Did we have no tickets?  FTL.
   if (totalTickets == 0)
      return;

   uint winningTicket = getRandRange(cSimRand, 0, totalTickets-1);
   for (uint i=0; i<numTopics; i++)
   {
      BAITopic* pAITopic = gWorld->getAITopic(mTopicIDs[i]);
      BASSERT(pAITopic);
      uint currentTickets = pAITopic->getCurrentTickets();
      if (currentTickets >= winningTicket)
      {
         AIQSetTopic(mTopicIDs[i]);
         return;
      }
      else
      {
         winningTicket -= currentTickets;
      }
   }

   BASSERTM(false, "It was time to determine a new AIQ topic, but the lotto failed spectacularly!");
}


//==============================================================================
//==============================================================================
void BAI::AIQSetTopic(BAITopicID newTopic)
{
   // We should only be calling this if we have AIQ
   if (!mFlagAIQ)
      return;

   DWORD currentGameTime = gWorld->getGametime();

   BAITopic* pOldTopic = gWorld->getAITopic(mAIQTopic);
   if (pOldTopic)
      pOldTopic->toggleActive(currentGameTime, false);

   BAITopic* pNewTopic = gWorld->getAITopic(newTopic);
   if (pNewTopic)
   {
      mAIQTopic = newTopic;
      pNewTopic->disablePriorityRequest();
      pNewTopic->setCurrentTicketsToMin();
      pNewTopic->toggleActive(currentGameTime, true);
      BASSERT(getNumActiveTopics() == 1);
   }
   else
   {
      mAIQTopic = cInvalidAITopicID;
      BASSERT(getNumActiveTopics() == 0);
   }
}


//==============================================================================
//==============================================================================
void BAI::update()
{
   // Do not update if paused.
   if (mFlagPaused)
      return;
   // Do not update if not playing.
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer || !pPlayer->isPlaying())
      return;

   DWORD currentGameTime = gWorld->getGametime();

   AIQUpdateLottoTickets(currentGameTime);

//-- FIXING PREFIX BUG ID 5205
   const BAITopic* pTopic = gWorld->getAITopic(mAIQTopic);
//--
   if (!pTopic && mTopicIDs.getSize() > 0)
   {
      AIQTopicLotto(currentGameTime);
   }

   // Update the bid manager (NOTE: MOVE THIS TO SCRIPT DRIVEN)
   BASSERT(mpBidManager);
   mpBidManager->update();

   // Update the missions.
   //updateMissions(currentGameTime);

   #ifndef BUILD_FINAL
   //debugRender();
   #endif
   }


//==============================================================================
//==============================================================================
void BAI::generateBaseMissionTarget(BKBBase* pKBBase)
{
   if (!pKBBase)
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return;

   BPlayerID basePlayerID = pKBBase->getPlayerID();
   BKBBaseID baseID = pKBBase->getID();

   BAIMissionTarget* pMissionTarget = gWorld->createAIMissionTarget(mPlayerID);
   BASSERT(pMissionTarget);
   if (pMissionTarget)
   {
      pMissionTarget->setTargetType(MissionTargetType::cKBBase);
      if (pPlayer->isEnemy(basePlayerID) || pPlayer->isNeutral(basePlayerID)) // Necessary to make sure Gaia (empty) bases aren't defend missions.
      {
         pMissionTarget->setMissionType(MissionType::cAttack);
         #ifndef BUILD_FINAL
            pMissionTarget->mName.format("Attack P%1d base at (%4.0f,%4.0f)", basePlayerID, pKBBase->getPosition().x, pKBBase->getPosition().z);
         #endif
      }
      else
      {
         pMissionTarget->setMissionType(MissionType::cDefend);
         #ifndef BUILD_FINAL
            pMissionTarget->mName.format("Defend P%1d base at (%4.0f,%4.0f)", basePlayerID, pKBBase->getPosition().x, pKBBase->getPosition().z);
         #endif
      }
      pMissionTarget->setKBBaseID(baseID);
      pMissionTarget->setPosition(pKBBase->getPosition());
      pMissionTarget->setRadius(pKBBase->getRadius());
      pKBBase->addAIMissionTargetID(pMissionTarget->getID());
   }
}


//==============================================================================
//==============================================================================
void BAI::generateDesignSphereMissionTarget(BDesignSphere* pDesignSphere)
{
   BASSERT(pDesignSphere);
   if (!pDesignSphere)
      return;

   BAIMissionTarget* pMissionTarget = gWorld->createAIMissionTarget(mPlayerID);
   BASSERT(pMissionTarget);
   if (pMissionTarget)
   {
      pMissionTarget->setTargetType(MissionTargetType::cArea);
      pMissionTarget->setMissionType(MissionType::cScout);
      pMissionTarget->setPosition(pDesignSphere->mPosition);
      pMissionTarget->setRadius(pDesignSphere->mRadius);
   }
}


//==============================================================================
//==============================================================================
bool BAI::loadAIDifficultySettings()
{
   BXMLReader scriptFileReader;
   if (!scriptFileReader.load(cDirData, "aidifficultysettings.xml"))
   {
      BASSERTM(false, "Could not load aidifficultysettings.xml");
      return (false);
   }

   BXMLNode scriptFileRoot(scriptFileReader.getRootNode());
   if (scriptFileRoot.getName() != "AIDifficultySettings")
   {
      BASSERTM(false, "Could not get root node AIDifficultySettings.");
      return (false);
   }

   uint numDifficultySettingNodes = static_cast<uint>(scriptFileRoot.getNumberChildren());
   mDifficultySettings.resize(0);
   mDifficultySettings.reserve(numDifficultySettingNodes);

   for (uint i=0; i<numDifficultySettingNodes; i++)
   {
      BXMLNode difficultySettingNode(scriptFileRoot.getChild(i));
      if (difficultySettingNode.getName() != "AIDifficultySetting")
      {
         BASSERTM(false, "Could not load node AIDifficultySetting.");
         return (false);
      }

      BAIDifficultySetting settingToAdd;
      if (!settingToAdd.loadFromXML(difficultySettingNode))
      {
         BASSERTM(false, "Could not load AIDifficultySetting node.");
         return (false);
      }

      mDifficultySettings.add(settingToAdd);
   }

   return (true);
}


//==============================================================================
//==============================================================================
void BAI::recalculateDifficultySettings(float difficulty)
{
   BAIDifficultySetting* pSetting;
   
   pSetting = getDifficultySettingByName("MaxMissionFocusTime");
   BASSERT(pSetting);
   if (pSetting)
      mMaxMissionFocusTime = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("ThinkCostMin");
   BASSERT(pSetting);
   if (pSetting)
      mThinkCostMin = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("ThinkCostMax");
   BASSERT(pSetting);
   if (pSetting)
      mThinkCostMax = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("ActCostMin");
   BASSERT(pSetting);
   if (pSetting)
      mActCostMin = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("ActCostMax");
   BASSERT(pSetting);
   if (pSetting)
      mActCostMax = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("PowerEvalIntervalMin");
   BASSERT(pSetting);
   if (pSetting)
      mPowerEvalIntervalMin = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));

   pSetting = getDifficultySettingByName("PowerEvalIntervalMax");
   BASSERT(pSetting);
   if (pSetting)
      mPowerEvalIntervalMax = static_cast<DWORD>(1000.0f * pSetting->getFunc().evaluate(difficulty));
}


//==============================================================================
//==============================================================================
const BAIDifficultySetting* BAI::getDifficultySettingByName(const BSimString& name) const
{
   uint numSettings = mDifficultySettings.getSize();
   for (uint i=0; i<numSettings; i++)
   {
      if (mDifficultySettings[i].getName() == name)
         return (&mDifficultySettings[i]);
   }
   return (NULL);
}


//==============================================================================
//==============================================================================
BAIDifficultySetting* BAI::getDifficultySettingByName(const BSimString& name)
{
   uint numSettings = mDifficultySettings.getSize();
   for (uint i=0; i<numSettings; i++)
   {
      if (mDifficultySettings[i].getName() == name)
         return (&mDifficultySettings[i]);
   }
   return (NULL);
}


//==============================================================================
// Note: To-be-obsoleted version, use the one (below) that passes in a reference
// to a score object.
//==============================================================================
void BAI::scoreMissionTarget(BAIMissionTargetID missionTargetID)
{
   BAIMissionScore dummy;
   scoreMissionTarget(missionTargetID, dummy, true);
}

//==============================================================================
// Note: This function sets the score variables in the referenced score object, and
// returns the float value of the "total" member variable of the score.
//==============================================================================
float BAI::scoreMissionTarget(BAIMissionTargetID missionTargetID, BAIMissionScore& score, bool updateTargets)
{  // updateTargets should be set if this is the main AI Decider scan, using the reserves squad list.
   // In that case, the scores calculated will also be copied into the targets themselves for sorting and debug display.
   // In cases where we're scoring for a leftover group, or doing hypothetical scoring to consider cancelling a mission, 
   // updateTargets should be left false.

   BAIMissionTarget* pMissionTarget = gWorld->getAIMissionTarget(missionTargetID);
   BASSERTM(pMissionTarget, "The AI has a mission target ID that is not valid!");
   if (!pMissionTarget)
      return(-1.0f);

   if (!pMissionTarget->getFlagAllowScoring())
   {
      BAIMissionScore zeroScore;
      zeroScore.zero();
      pMissionTarget->setScore(zeroScore);
      return(0.0f);
   }

   if (updateTargets)
      pMissionTarget->getScore().zero();  

   score.zero();
   BAIMissionType missionType = pMissionTarget->getMissionType();

   // Set the background variables
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return(-1.0f);
   BTeamID myTeam = pPlayer->getTeamID();
   BTeamID enemyTeam = 2;
   if (myTeam == 2)
      enemyTeam = 1;
   BKB* pKB = gWorld->getKB(myTeam);
   if (!pKB)
      return(-1.0f);
   // Base pointer may be null.
   const BKBBase* pBase = gWorld->getKBBase(pMissionTarget->getKBBaseID());
   BVector targetPos = pBase ? pBase->getPosition() : pMissionTarget->getPosition();
   aiLog("\t\tTarget Position (%.0f,%.0f,%.0f)\n",targetPos.x, targetPos.y, targetPos.z);
   
   mScoringEnemyAssets = getEnemyAssetValueInArea(targetPos, cMaxBKBBaseRadius, false);
   mScoringFriendlyAssets = getAllyAssetValueInArea(targetPos, cMaxBKBBaseRadius, false, true);   // Last parm includes my assets in total

   BASSERT(mpAIGlobals);
   mScoringDistance = mpAIGlobals->getScoringMaxDistance();

   if (mpAIGlobals->useScoringStartLoc())
   {
      BVector startLocation = mpAIGlobals->getScoringStartLoc();
      mScoringDistance = targetPos.distance(startLocation);
      mScoringDistance = static_cast<float>(Math::fSelectClamp(mScoringDistance, mpAIGlobals->getScoringMinDistance(), mpAIGlobals->getScoringMaxDistance()));
   }

   float inst = mScoringEnemyAssets;
   if (missionType == MissionType::cDefend)
   {
      inst = mScoringFriendlyAssets;  

      //mScoringDistance = Math::Min(mScoringDistance, 400.0f);
      mScoringDistance *= 0.25f;
      mScoringDistance = static_cast<float>(Math::fSelectClamp(mScoringDistance, mpAIGlobals->getScoringMinDistance(), mpAIGlobals->getScoringMaxDistance()));
   }

   if (mScoringDistance > mpAIGlobals->getScoringMinDistance())
      inst /= mScoringDistance;
   else
      inst /= mpAIGlobals->getScoringMinDistance();  // Use minimum range
   aiLog("\t\tRaw score: %f\n", inst);
   // Inst is now assets / distance, need to scale it to 0..1 range, with .8 as par.  
   float parValue = 100.0f;
   //  We'll scale inst of 0..parValue to 0..0.8, and then a shallower ramp
   // up from 0.8, with a cap of 1.0.
   if (inst < parValue)
      score.setInstance((inst / parValue) * 0.8f);
   else
      score.setInstance( 0.8f + (inst-parValue) / (4.0f * parValue) );
   if (score.getInstance() > 1.0f)
      score.setInstance(1.0f);
   

// Adjust for most hated player
   BPlayerID owner = cInvalidPlayerID;
   if (pBase)
      owner = pBase->getPlayerID();
   // most hated player is now obsolete
   //if ( (owner == mpAIGlobals->getHatedPlayer()) && (owner >= 0) )
   //   score.setInstance(score.applyBias(score.getInstance(), 0.0f, 1.0f, 0.5));   // Bump up the instance value



   // Calculate class score.   For now, just 0.8f.  Later, it should be a lookup function for base/attack.
   score.setClass(0.8f);


   const BEntityIDArray mySquads = mpAIGlobals->getScoringSquadList();
   BKBSquadIDArray friendlySquadsInArea;
   BKBSquadIDArray enemySquadsInArea;
   BAISquadAnalysis friendlyAnalysis;
   BAISquadAnalysis enemyAnalysis;
   BAISquadAnalysis combinedAnalysis;  // My squads plus ally's squads

   getAllyKBSquadsInArea(targetPos, cMaxBKBBaseRadius, false, true, friendlySquadsInArea);      // Include my squads in area, otherwise time to kill excludes my buildings

   //don't count stuff in "mySquads" as already in the area!
   BEntityIDArray goodSquadsInArea;
   uint numKBSquads = friendlySquadsInArea.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(friendlySquadsInArea[i]);
      BEntityID id = pKBSquad->getSquadID();
      if (pKBSquad && pKBSquad->getCurrentlyVisible())
      {
         if(mySquads.contains(id))
            continue;
         goodSquadsInArea.add(pKBSquad->getSquadID());
      }
   }
   


   getEnemyKBSquadsInArea(targetPos, cMaxBKBBaseRadius, false, enemySquadsInArea);

   //BAI::calculateSquadAnalysis(friendlySquadsInArea, friendlyAnalysis);
   BAI::calculateSquadAnalysis(goodSquadsInArea, friendlyAnalysis);
   BAI::calculateSquadAnalysis(enemySquadsInArea, enemyAnalysis);
   BAI::calculateSquadAnalysis(mySquads, combinedAnalysis);  
   combinedAnalysis.add(friendlyAnalysis);

   float allyOdds = BAI::calculateOffenseRatioAToB(friendlyAnalysis, enemyAnalysis); // A = friendlies, B = enemies
   if ( (missionType == MissionType::cDefend) && (friendlySquadsInArea.getNumber() > 0) && (enemySquadsInArea.getNumber() == 0) )
      allyOdds = 1.0f;  // No enemy in defend target, we win
   float combinedOdds = BAI::calculateOffenseRatioAToB(combinedAnalysis, enemyAnalysis);


   bool worthlessTarget = false;
   if(mpAIGlobals->getUseMinWin() && mpAIGlobals->getMinWin() > combinedOdds)
   {
      worthlessTarget = true;
   }
   if(mpAIGlobals->getUseMaxWin() && mpAIGlobals->getMaxWin() < combinedOdds)
   {
      worthlessTarget = true;
   }

   if (  (missionType == MissionType::cDefend) && ( (mySquads.getNumber() + friendlySquadsInArea.getNumber()) > 0) && (enemySquadsInArea.getNumber() == 0) )
      combinedOdds = 1.0f;  // No enemy in defend target, we win
   float deltaOdds = combinedOdds - allyOdds;   // How much my squads improve odds
   aiLog("\t\tAlly Odds: %f\n",allyOdds);
   aiLog("\t\tCombined Odds: %f\n",combinedOdds);
   aiLog("\t\tDelta: %f\n",deltaOdds);

   // Now that we have ally odds and combined odds, we need a metric for how good it is for us to intervene.
   // The formula below measures how close we are, on a 0..1 scatter chart of CombinedOdds (x) and AllyOdds (y), to
   // the ideal of 1, 0 (guaranteed combined win, guaranteed loss without our forces.  0,0 is a lost cause.  0,1 is not possible (y <= x in all cases.)
   // 1, 1 is pointless piling on, a guaranteed ally win where we send extra troops.    So it's the distance from (1,0) that we want to measure.

   //----------------------------------------------------------------------------
   // FABS -- returns the absolute value of a number
   //----------------------------------------------------------------------------
#define FABS( a ) ( ((a) < 0.0f) ? -(a) : (a) )

   float distFromIdeal = FABS(1.0f - combinedOdds);
   distFromIdeal *= distFromIdeal;  // x squared
   distFromIdeal += allyOdds * allyOdds;  // + y squared
   distFromIdeal = sqrt(distFromIdeal);
   float rawAfford = 1.0f - distFromIdeal;
   float adjustedAfford = 0.0f;

   // Now map it so that scores below 0.5 get a very poor afford, and
   // scores 0.5 to 0.8 improve rapidly, and 0.8 and above improve slowly.
   if (rawAfford < 0.4f)
      adjustedAfford = rawAfford / 4.0f;  // 0 to .4 maps to 0 to .1
   else if (rawAfford < 0.8f)
      adjustedAfford = 0.1f + ( (rawAfford - 0.4f) * (0.8f / 0.4f) );   // .4 to .8 maps to .1 to .9
   else
      adjustedAfford = 0.9f + ( (rawAfford - 0.8f) * (0.1f / 0.2f) );   // .8 to 1.0 maps to .9 to 1.0
   if( (adjustedAfford < 0.0f) || (adjustedAfford > 1.0f) )
   {
      adjustedAfford = (adjustedAfford < 0.0f ? 0.0f : 1.0f);
   }

   if (missionType == MissionType::cDefend)
   {     // Adjust instance score if this is a defend operation.
      float enemyKillTime = BAI::calculateUnopposedTimeToKill(enemyAnalysis, combinedAnalysis, 300.0f);   // Up to 300 seconds
      aiLog("\t\tTime for enemies to kill allies: %f\n",enemyKillTime);
      float enemyKillSpeed = 0.0f;
      enemyKillSpeed = (300.0f - enemyKillTime) / 300.0f;   // 1.0 = insta-kill, 0.0 = 5 minutes or longer
      // Set the instance value to the lesser of kill speed or inst (from above)
      // If the threat is so small that it won't kill the allied target, it's not worth fighting against.
      aiLog("\t\tKill speed: %.3f\n",enemyKillSpeed);
      score.setInstance(min(score.getInstance(), enemyKillSpeed));
      aiLog("\t\tInstance: %.3f\n",score.getInstance());
   }


   // AdjustedAfford now represents the difference our forces make, with the mid range amplified.
   // (Changing odds of winning from 0 to 33%, or from 67% to 100%, is not as important as going from 33 to 67.)
   // Next, we need to handle the case of zero or very little 
   // enemy forces, which would give odds near 1.0.  We need to make sure that the force we send in is able to take out the
   // target in a reasonable time, i.e. not sending one marine squad against 10 buildings.  To do this, we'll calculate a parallel
   // rating for appropriate size, and use the lesser of that number and delta odds.  The net result will be to 
   // suppress "sure win" missions of a single squad against a large, well-defended target.

   float killSpeed = 0.0f;
   float maxTime = 300.0f;    // 5 minutes, function takes seconds instead of milliseconds
   float killTime = BAI::calculateUnopposedTimeToKill(combinedAnalysis, enemyAnalysis, maxTime);  // Make sure we can kill in under 5 min
   aiLog("\t\tTime for combined forces to kill enemy: %f\n",killTime);
   // killTime is now 0...maxTime, with maxTime indicating a likely infinite result.  Convert to killSpeed, so that time of maxTime = killSpeed 0.
   if (killTime > 0.0f)
      killSpeed = (maxTime - killTime) / maxTime;
   if (killSpeed < 0.0f)
      killSpeed = 0.0f;
   aiLog("\t\tKill speed: %.3f\n",killSpeed);
   // A killSpeed of 1 = insta-kill, 0 = maxTime kill, 0.5 = 1/2 of maxTime to kill.
   // Compare to deltaOdds, use the lower value for affordability
   if (killSpeed < 0.1f)
      killSpeed = 0.1f;    // Slow kill shouldn't totally prohibit a mission.

   score.setAfford(min(adjustedAfford, killSpeed));

   if(worthlessTarget == true)
   {
      score.setAfford(0.0);  //0.01234);
   }

   score.setPermission(0.0f);

   switch (missionType)
   {
      case MissionType::cAttack:
      {
         // Apply offense bias
         score.setClass(score.applyBias(score.getClass(), 0.0f, 1.0f, mpAIGlobals->getBiasOffense()));
         if (!mpAIGlobals->getOKToAttack())
         {  // Prevent attack missions if not permitted, unless it's a command from an ally (permission >= 1)
            score.setClass(0.0f); 
         }
         break;
      }
      case MissionType::cDefend:
      {
         // Apply offense bias as a penalty
         score.setClass(score.applyBias(score.getClass(), 0.0f, 1.0f, -1.0f * mpAIGlobals->getBiasOffense()));
         if (!mpAIGlobals->getOKToDefend())
         {  // Prevent defend missions if not permitted, unless it's a command from an ally (permission >= 1)
            score.setClass(0.0f); 
         }
         break;
      }
      default:
      {
         // NOT SUPPORTED.
         BASSERTM(false, "Mission type not supported.");
         break;
      }
   }

   score.calculateTotal(); 

   if (updateTargets)
      pMissionTarget->setScore(score);

   return(score.getTotal());
}


//==============================================================================
//==============================================================================
const BAIMissionTarget* BAI::scoreAllMissionTargets()
{
   float bestTargetTotalScore = 0.0f;
   const BAIMissionTarget* pBestScoringTarget = NULL;
   uint numMissionTargets = mMissionTargetIDs.getSize();
   for (uint i=0; i<numMissionTargets; i++)
   {
      BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(mMissionTargetIDs[i]);
      BASSERT(pTarget);


      // Check if scoring is needed...     if (pTarget->)
      if (!pTarget->getFlagAllowScoring())
      {
         pTarget->getScore().zero();
      }
      else
      {
         BAIMissionScore score;
         float thisTargetTotal = scoreMissionTarget(mMissionTargetIDs[i], score, true);
         if (thisTargetTotal > bestTargetTotalScore)
         {
            pBestScoringTarget = pTarget;
            bestTargetTotalScore = thisTargetTotal;
         }
      }
   }
   return (pBestScoringTarget);
}


//==============================================================================
//==============================================================================
uint BAI::getMissionIDsByType(BAIMissionType missionType, BAIMissionIDArray& missionIDs) const
{
   // reset our data
   missionIDs.resize(0);

   uint numMissionIDs = mMissionIDs.getSize();
   for (uint i=0; i<numMissionIDs; i++)
   {
      const BAIMission* pAIMission = gWorld->getAIMission(mMissionIDs[i]);
      if (pAIMission && pAIMission->isType(missionType))
         missionIDs.add(mMissionIDs[i]);
   }

   return (missionIDs.getSize());
}


//==============================================================================
//==============================================================================
float BAI::getAllyAssetValueInArea(BVector position, float radius, bool requireVis, bool selfAsAlly) const
{
   BKBSquadQuery squadQuery;
   BKBSquadIDArray queryResults;
   squadQuery.setPointRadius(position, radius);
   squadQuery.setPlayerRelation(mPlayerID, cRelationTypeAlly);
   squadQuery.setSelfAsAlly(selfAsAlly);
   if (requireVis)
      squadQuery.setCurrentlyVisible(true);

   float allyAssetValue = 0.0f;

   BKB* pKB = gWorld->getKB(mTeamID);
   if (pKB)
   {
      uint numSquads = pKB->executeKBSquadQuery(squadQuery, queryResults, BKB::cClearExistingResults);
      for (uint i=0; i<numSquads; i++)
      {
         const BKBSquad* pKBSquad = gWorld->getKBSquad(queryResults[i]);
         if (pKBSquad)
            allyAssetValue += (pKBSquad->getAssetValue() * mpAIGlobals->getPlayerModifier(pKBSquad->getPlayerID()));
      }
   }
   return (allyAssetValue);
}


//==============================================================================
//==============================================================================
float BAI::getEnemyAssetValueInArea(BVector position, float radius, bool requireVis) const
{
   BKBSquadQuery squadQuery;
   BKBSquadIDArray queryResults;
   squadQuery.setPointRadius(position, radius);
   squadQuery.setPlayerRelation(mPlayerID, cRelationTypeEnemy);
   if (requireVis)
      squadQuery.setCurrentlyVisible(true);

   float enemyAssetValue = 0.0f;

   BKB* pKB = gWorld->getKB(mTeamID);
   if (pKB)
   {
      uint numSquads = pKB->executeKBSquadQuery(squadQuery, queryResults, BKB::cClearExistingResults);
      for (uint i=0; i<numSquads; i++)
      {
         const BKBSquad* pKBSquad = gWorld->getKBSquad(queryResults[i]);
         if (!pKBSquad)
            continue;

         float modifier = 1.0f;
         float playerModifier = 1.0f;
         const BSquad* pSquad = pKBSquad->getSquad();
         if(pSquad)
            modifier = mScoreModifier.GetScoreModifier(pSquad);

         playerModifier = mPlayerModifier.GetPlayerModifier(pKBSquad->getPlayerID());

         enemyAssetValue += (modifier * playerModifier * pKBSquad->getAssetValue() * mpAIGlobals->getPlayerModifier(pKBSquad->getPlayerID()));
      }
   }
   return (enemyAssetValue);
}


//==============================================================================
//==============================================================================
void BAI::getAllyKBSquadsInArea(BVector position, float radius, bool requireVis, bool selfAsAlly, BKBSquadIDArray& results) const
{
   results.resize(0);
   BKBSquadQuery squadQuery;
   squadQuery.setPointRadius(position, radius);
   squadQuery.setPlayerRelation(mPlayerID, cRelationTypeAlly);
   squadQuery.setSelfAsAlly(selfAsAlly);
   if (requireVis)
      squadQuery.setCurrentlyVisible(true);
   BKB* pKB = gWorld->getKB(mTeamID);
   if (pKB)
      pKB->executeKBSquadQuery(squadQuery, results, 0);   
}


//==============================================================================
//==============================================================================
void BAI::getEnemyKBSquadsInArea(BVector position, float radius, bool requireVis, BKBSquadIDArray& results) const
{
   results.resize(0);
   BKBSquadQuery squadQuery;
   squadQuery.setPointRadius(position, radius);
   squadQuery.setPlayerRelation(mPlayerID, cRelationTypeEnemy);
   if (requireVis)
      squadQuery.setCurrentlyVisible(true);
   BKB* pKB = gWorld->getKB(mTeamID);
   if (pKB)
      pKB->executeKBSquadQuery(squadQuery, results, 0);  
}




//==============================================================================
//==============================================================================
void BAI::calculateSquadAnalysis(const BEntityIDArray& squadIDs, BAISquadAnalysis& squadAnalysis)
{
   // Pre-store these for easy access.
   BDamageTypeID cLight = gDatabase.getDamageTypeLight();
   BDamageTypeID cLightArmored = gDatabase.getDamageTypeLightArmored();
   BDamageTypeID cMedium = gDatabase.getDamageTypeMedium();
   BDamageTypeID cMediumAir = gDatabase.getDamageTypeMediumAir();
   BDamageTypeID cHeavy = gDatabase.getDamageTypeHeavy();
   BDamageTypeID cBuilding = gDatabase.getDamageTypeBuilding();

   // Fresh results.
   BAISquadAnalysis sa;

   // Total everything up.
   uint numSquads = squadIDs.getSize();
#ifdef aiCombatHack
   sa.mNumSquads = numSquads;
#endif
   for (uint i=0; i<numSquads; i++)
   {
      // Dumb.
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      // Get the CVHP of this squad.
      float cvhp = pSquad->getCombatValueHP();
      float hp = pSquad->getCurrentHP();
      float sp = pSquad->getCurrentSP();

      float vetMultiplier = 1.0;
      int vetLevel = pSquad->getVetLevel();
      if(vetLevel == 1)
         vetMultiplier = 1.15;
      else if(vetLevel == 2)
         vetMultiplier = 1.44;//1.25*1.15;      
      else if(vetLevel == 3)
         vetMultiplier = 1.94;//1.35*1.25*1.15;
      //else if(vetLevel == 4)
      //   vetMultiplier = 3.88;//you should only see lvl 4+ in skirmish
      //else if(vetLevel == 5)
      //   vetMultiplier = 7.76;
      //15*30*45*60  ...spartan tactics? (do not multiply)
      //1.9*1.9*1.6  ...lvl3 spartan in lvl3 scorp..
      cvhp = cvhp * vetMultiplier;

      if(pSquad->isInCover())
      {
         cvhp *= 3.0;
      }


      // Add to our overall makeup by type.
      if (pSquad->isDamageType(cLight))
      {
         sa.mCVLight += cvhp;
         sa.mHPLight += hp;
         sa.mSPLight += sp;
      }
      else if (pSquad->isDamageType(cLightArmored))
      {
         sa.mCVLightArmored += cvhp;
         sa.mHPLightArmored += hp;
         sa.mSPLightArmored += sp;
      }
      else if (pSquad->isDamageType(cMedium))
      {
         sa.mCVMedium += cvhp;
         sa.mHPMedium += hp;
         sa.mSPMedium += sp;
      }
      else if (pSquad->isDamageType(cMediumAir))
      {
         sa.mCVMediumAir += cvhp;
         sa.mHPMediumAir += hp;
         sa.mSPMediumAir += sp;
      }
      else if (pSquad->isDamageType(cHeavy))
      {
         sa.mCVHeavy += cvhp;
         sa.mHPHeavy += hp;
         sa.mSPHeavy += sp;
      }
      else if (pSquad->isDamageType(cBuilding))
      {
         sa.mCVBuilding += cvhp;
         sa.mHPBuilding += hp;
         sa.mSPBuilding += sp;
      }

      // Add to our buckets of PAIN by type.  :)
      sa.mCVStarsLight += cvhp * pSquad->getStars(cLight);
      sa.mCVStarsLightArmored += cvhp * pSquad->getStars(cLightArmored);
      sa.mCVStarsMedium += cvhp * pSquad->getStars(cMedium);
      sa.mCVStarsMediumAir += cvhp * pSquad->getStars(cMediumAir);
      sa.mCVStarsHeavy += cvhp * pSquad->getStars(cHeavy);
      sa.mCVStarsBuilding += cvhp * pSquad->getStars(cBuilding);

      sa.mDPSLight += pSquad->getAttackRatingDPS(cLight);
      sa.mDPSLightArmored += pSquad->getAttackRatingDPS(cLightArmored);
      sa.mDPSMedium += pSquad->getAttackRatingDPS(cMedium);
      sa.mDPSMediumAir += pSquad->getAttackRatingDPS(cMediumAir);
      sa.mDPSHeavy += pSquad->getAttackRatingDPS(cHeavy);
      sa.mDPSBuilding += pSquad->getAttackRatingDPS(cBuilding);
   }

   // Get the totals.
   sa.mCVTotal = sa.mCVLight + sa.mCVLightArmored + sa.mCVMedium + sa.mCVMediumAir + sa.mCVHeavy + sa.mCVBuilding;
   sa.mHPTotal = sa.mHPLight + sa.mHPLightArmored + sa.mHPMedium + sa.mHPMediumAir + sa.mHPHeavy + sa.mHPBuilding;
   sa.mSPTotal = sa.mSPLight + sa.mSPLightArmored + sa.mSPMedium + sa.mSPMediumAir + sa.mSPHeavy + sa.mSPBuilding;
   sa.mDPSTotal = sa.mDPSLight + sa.mDPSLightArmored + sa.mDPSMedium + sa.mDPSMediumAir + sa.mDPSHeavy + sa.mDPSBuilding;
   sa.mCVStarsTotal = sa.mCVStarsLight + sa.mCVStarsLightArmored + sa.mCVStarsMedium + sa.mCVStarsMediumAir + sa.mCVStarsHeavy + sa.mCVStarsBuilding;

   // Now we have the totals, get some normalized values.
   if (sa.mCVTotal > 0.0f)
   {
      sa.mCVPercentLight = sa.mCVLight / sa.mCVTotal;
      sa.mCVPercentLightArmored = sa.mCVLightArmored / sa.mCVTotal;
      sa.mCVPercentMedium = sa.mCVMedium / sa.mCVTotal;
      sa.mCVPercentMediumAir = sa.mCVMediumAir / sa.mCVTotal;
      sa.mCVPercentHeavy = sa.mCVHeavy/ sa.mCVTotal;
      sa.mCVPercentBuilding = sa.mCVBuilding / sa.mCVTotal;
   }
   else
   {
      sa.mCVPercentLight = 0.0f;
      sa.mCVPercentLightArmored = 0.0f;
      sa.mCVPercentMedium = 0.0f;
      sa.mCVPercentMediumAir = 0.0f;
      sa.mCVPercentHeavy = 0.0f;
      sa.mCVPercentBuilding = 0.0f;
   }

   if (sa.mHPTotal > 0.0f)
   {
      sa.mHPPercentLight = sa.mHPLight / sa.mHPTotal;
      sa.mHPPercentLightArmored = sa.mHPLightArmored / sa.mHPTotal;
      sa.mHPPercentMedium = sa.mHPMedium / sa.mHPTotal;
      sa.mHPPercentMediumAir = sa.mHPMediumAir / sa.mHPTotal;
      sa.mHPPercentHeavy = sa.mHPHeavy / sa.mHPTotal;
      sa.mHPPercentBuilding = sa.mHPBuilding / sa.mHPTotal;
   }
   else
   {
      sa.mHPPercentLight = 0.0f;
      sa.mHPPercentLightArmored = 0.0f;
      sa.mHPPercentMedium = 0.0f;
      sa.mHPPercentMediumAir = 0.0f;
      sa.mHPPercentHeavy = 0.0f;
      sa.mHPPercentBuilding = 0.0f;
   }

   if (sa.mCVStarsTotal > 0.0f)
   {
      float starsMax = sa.mCVStarsLight;
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsLightArmored));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMedium));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMediumAir));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsHeavy));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsBuilding));
      sa.mNormalizedStarsLight = sa.mCVStarsLight / starsMax;
      sa.mNormalizedStarsLightArmored = sa.mCVStarsLightArmored / starsMax;
      sa.mNormalizedStarsMedium = sa.mCVStarsMedium / starsMax;
      sa.mNormalizedStarsMediumAir = sa.mCVStarsMediumAir / starsMax;
      sa.mNormalizedStarsHeavy = sa.mCVStarsHeavy / starsMax;
      sa.mNormalizedStarsBuilding = sa.mCVStarsBuilding / starsMax;         
   }

   // Write out our local working analysis to the results.
   squadAnalysis = sa;
}


//==============================================================================
//==============================================================================
void BAI::calculateSquadAnalysis(const BKBSquadIDArray& kbSquadIDs, BAISquadAnalysis& squadAnalysis)
{
   // Pre-store these for easy access.
   BDamageTypeID cLight = gDatabase.getDamageTypeLight();
   BDamageTypeID cLightArmored = gDatabase.getDamageTypeLightArmored();
   BDamageTypeID cMedium = gDatabase.getDamageTypeMedium();
   BDamageTypeID cMediumAir = gDatabase.getDamageTypeMediumAir();
   BDamageTypeID cHeavy = gDatabase.getDamageTypeHeavy();
   BDamageTypeID cBuilding = gDatabase.getDamageTypeBuilding();

   // Fresh results.
   BAISquadAnalysis sa;

   // Total everything up.
   uint numKBSquads = kbSquadIDs.getSize();
#ifdef aiCombatHack
   sa.mNumSquads = numKBSquads;
#endif
   for (uint i=0; i<numKBSquads; i++)
   {
      // Dumb.
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadIDs[i]);
      if (!pKBSquad)
         continue;
      const BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
      if (!pPlayer)
         continue;
      const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
      if (!pProtoSquad)
         continue;

      // get the CVHP of this squad.
      float cvhp = pKBSquad->getCombatValueHP();
      float hp = pKBSquad->getHitpoints();
      float sp = pKBSquad->getShieldpoints();

      float vetMultiplier = 1.0;      
      
      const BSquad* pSquad = pKBSquad->getSquad();
      if(pSquad != NULL)
      {
         int vetLevel = pSquad->getVetLevel();
         if(vetLevel == 1)
            vetMultiplier = 1.15;
         else if(vetLevel == 2)
            vetMultiplier = 1.44;//1.25*1.15;      
         else if(vetLevel == 3)
            vetMultiplier = 1.94;//1.35*1.25*1.15;
      }
      cvhp *= vetMultiplier;

      if(pSquad != NULL && pSquad->isInCover())
      {
         cvhp *= 2.0;
      }

      // Add to our overall makeup by type.
      if (pKBSquad->isDamageType(cLight))
      {
         sa.mCVLight += cvhp;
         sa.mHPLight += hp;
         sa.mSPLight += sp;
      }
      else if (pKBSquad->isDamageType(cLightArmored))
      {
         sa.mCVLightArmored += cvhp;
         sa.mHPLightArmored += hp;
         sa.mSPLightArmored += sp;
      }
      else if (pKBSquad->isDamageType(cMedium))
      {
         sa.mCVMedium += cvhp;
         sa.mHPMedium += hp;
         sa.mSPMedium += sp;
      }
      else if (pKBSquad->isDamageType(cMediumAir))
      {
         sa.mCVMediumAir += cvhp;
         sa.mHPMediumAir += hp;
         sa.mSPMediumAir += sp;
      }
      else if (pKBSquad->isDamageType(cHeavy))
      {
         sa.mCVHeavy += cvhp;
         sa.mHPHeavy += hp;
         sa.mSPHeavy += sp;
      }
      else if (pKBSquad->isDamageType(cBuilding))
      {
         sa.mCVBuilding += cvhp;
         sa.mHPBuilding += hp;
         sa.mSPBuilding += sp;
      }

      // Add to our buckets of PAIN by type.  :)
      sa.mCVStarsLight += cvhp * pKBSquad->getStars(cLight);
      sa.mCVStarsLightArmored += cvhp * pKBSquad->getStars(cLightArmored);
      sa.mCVStarsMedium += cvhp * pKBSquad->getStars(cMedium);
      sa.mCVStarsMediumAir += cvhp * pKBSquad->getStars(cMediumAir);
      sa.mCVStarsHeavy += cvhp * pKBSquad->getStars(cHeavy);
      sa.mCVStarsBuilding += cvhp * pKBSquad->getStars(cBuilding);

      sa.mDPSLight += pKBSquad->getAttackRatingDPS(cLight);
      sa.mDPSLightArmored += pKBSquad->getAttackRatingDPS(cLightArmored);
      sa.mDPSMedium += pKBSquad->getAttackRatingDPS(cMedium);
      sa.mDPSMediumAir += pKBSquad->getAttackRatingDPS(cMediumAir);
      sa.mDPSHeavy += pKBSquad->getAttackRatingDPS(cHeavy);
      sa.mDPSBuilding += pKBSquad->getAttackRatingDPS(cBuilding);
   }

   // Get the totals.
   sa.mCVTotal = sa.mCVLight + sa.mCVLightArmored + sa.mCVMedium + sa.mCVMediumAir + sa.mCVHeavy + sa.mCVBuilding;
   sa.mHPTotal = sa.mHPLight + sa.mHPLightArmored + sa.mHPMedium + sa.mHPMediumAir + sa.mHPHeavy + sa.mHPBuilding;
   sa.mSPTotal = sa.mSPLight + sa.mSPLightArmored + sa.mSPMedium + sa.mSPMediumAir + sa.mSPHeavy + sa.mSPBuilding;
   sa.mDPSTotal = sa.mDPSLight + sa.mDPSLightArmored + sa.mDPSMedium + sa.mDPSMediumAir + sa.mDPSHeavy + sa.mDPSBuilding;
   sa.mCVStarsTotal = sa.mCVStarsLight + sa.mCVStarsLightArmored + sa.mCVStarsMedium + sa.mCVStarsMediumAir + sa.mCVStarsHeavy + sa.mCVStarsBuilding;

   // Now we have the totals, get some normalized values.
   if (sa.mCVTotal > 0.0f)
   {
      sa.mCVPercentLight = sa.mCVLight / sa.mCVTotal;
      sa.mCVPercentLightArmored = sa.mCVLightArmored / sa.mCVTotal;
      sa.mCVPercentMedium = sa.mCVMedium / sa.mCVTotal;
      sa.mCVPercentMediumAir = sa.mCVMediumAir / sa.mCVTotal;
      sa.mCVPercentHeavy = sa.mCVHeavy/ sa.mCVTotal;
      sa.mCVPercentBuilding = sa.mCVBuilding / sa.mCVTotal;
   }
   else
   {
      sa.mCVPercentLight = 0.0f;
      sa.mCVPercentLightArmored = 0.0f;
      sa.mCVPercentMedium = 0.0f;
      sa.mCVPercentMediumAir = 0.0f;
      sa.mCVPercentHeavy = 0.0f;
      sa.mCVPercentBuilding = 0.0f;
   }

   if (sa.mHPTotal > 0.0f)
   {
      sa.mHPPercentLight = sa.mHPLight / sa.mHPTotal;
      sa.mHPPercentLightArmored = sa.mHPLightArmored / sa.mHPTotal;
      sa.mHPPercentMedium = sa.mHPMedium / sa.mHPTotal;
      sa.mHPPercentMediumAir = sa.mHPMediumAir / sa.mHPTotal;
      sa.mHPPercentHeavy = sa.mHPHeavy / sa.mHPTotal;
      sa.mHPPercentBuilding = sa.mHPBuilding / sa.mHPTotal;
   }
   else
   {
      sa.mHPPercentLight = 0.0f;
      sa.mHPPercentLightArmored = 0.0f;
      sa.mHPPercentMedium = 0.0f;
      sa.mHPPercentMediumAir = 0.0f;
      sa.mHPPercentHeavy = 0.0f;
      sa.mHPPercentBuilding = 0.0f;
   }

   if (sa.mCVStarsTotal > 0.0f)
   {
      float starsMax = sa.mCVStarsLight;
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsLightArmored));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMedium));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMediumAir));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsHeavy));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsBuilding));
      sa.mNormalizedStarsLight = sa.mCVStarsLight / starsMax;
      sa.mNormalizedStarsLightArmored = sa.mCVStarsLightArmored / starsMax;
      sa.mNormalizedStarsMedium = sa.mCVStarsMedium / starsMax;
      sa.mNormalizedStarsMediumAir = sa.mCVStarsMediumAir / starsMax;
      sa.mNormalizedStarsHeavy = sa.mCVStarsHeavy / starsMax;
      sa.mNormalizedStarsBuilding = sa.mCVStarsBuilding / starsMax;         
   }

   // Write out our local working analysis to the results.
   squadAnalysis = sa;
}


//==============================================================================
//==============================================================================
void BAI::calculateSquadAnalysis(const BProtoSquadIDArray& protoSquadIDArray, BPlayerID playerID, BAISquadAnalysis& squadAnalysis)
{
   // Pre-store these for easy access.
   BDamageTypeID cLight = gDatabase.getDamageTypeLight();
   BDamageTypeID cLightArmored = gDatabase.getDamageTypeLightArmored();
   BDamageTypeID cMedium = gDatabase.getDamageTypeMedium();
   BDamageTypeID cMediumAir = gDatabase.getDamageTypeMediumAir();
   BDamageTypeID cHeavy = gDatabase.getDamageTypeHeavy();
   BDamageTypeID cBuilding = gDatabase.getDamageTypeBuilding();

   // Fresh results.
   BAISquadAnalysis sa;

   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      // Write out our local working analysis to the results.
      squadAnalysis = sa;
      return;
   }

   // Total everything up.
   uint numProtoSquads = protoSquadIDArray.getSize();
#ifdef aiCombatHack
   sa.mNumSquads = numProtoSquads;
#endif
   for (uint i=0; i<numProtoSquads; i++)
   {
      // Dumb.
      const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(protoSquadIDArray[i]);
      if (!pProtoSquad)
         continue;

      // Get the CVHP of this squad.
      float cvhp = pProtoSquad->getCombatValue(); // * 100% HPSP
      float hp = pProtoSquad->getMaxHP();
      float sp = pProtoSquad->getMaxSP();

      // Add to our overall makeup by type.
      if (pProtoSquad->isDamageType(cLight))
      {
         sa.mCVLight += cvhp;
         sa.mHPLight += hp;
         sa.mSPLight += sp;
      }
      else if (pProtoSquad->isDamageType(cLightArmored))
      {
         sa.mCVLightArmored += cvhp;
         sa.mHPLightArmored += hp;
         sa.mSPLightArmored += sp;
      }
      else if (pProtoSquad->isDamageType(cMedium))
      {
         sa.mCVMedium += cvhp;
         sa.mHPMedium += hp;
         sa.mSPMedium += sp;
      }
      else if (pProtoSquad->isDamageType(cMediumAir))
      {
         sa.mCVMediumAir += cvhp;
         sa.mHPMediumAir += hp;
         sa.mSPMediumAir += sp;
      }
      else if (pProtoSquad->isDamageType(cHeavy))
      {
         sa.mCVHeavy += cvhp;
         sa.mHPHeavy += hp;
         sa.mSPHeavy += sp;
      }
      else if (pProtoSquad->isDamageType(cBuilding))
      {
         sa.mCVBuilding += cvhp;
         sa.mHPBuilding += hp;
         sa.mSPBuilding += sp;
      }

      // Add to our buckets of PAIN by type.  :)
      sa.mCVStarsLight += cvhp * pProtoSquad->getStars(cLight);
      sa.mCVStarsLightArmored += cvhp * pProtoSquad->getStars(cLightArmored);
      sa.mCVStarsMedium += cvhp * pProtoSquad->getStars(cMedium);
      sa.mCVStarsMediumAir += cvhp * pProtoSquad->getStars(cMediumAir);
      sa.mCVStarsHeavy += cvhp * pProtoSquad->getStars(cHeavy);
      sa.mCVStarsBuilding += cvhp * pProtoSquad->getStars(cBuilding);

      sa.mDPSLight += pProtoSquad->getAttackRatingDPS(cLight);
      sa.mDPSLightArmored += pProtoSquad->getAttackRatingDPS(cLightArmored);
      sa.mDPSMedium += pProtoSquad->getAttackRatingDPS(cMedium);
      sa.mDPSMediumAir += pProtoSquad->getAttackRatingDPS(cMediumAir);
      sa.mDPSHeavy += pProtoSquad->getAttackRatingDPS(cHeavy);
      sa.mDPSBuilding += pProtoSquad->getAttackRatingDPS(cBuilding);
   }

   // Get the totals.
   sa.mCVTotal = sa.mCVLight + sa.mCVLightArmored + sa.mCVMedium + sa.mCVMediumAir + sa.mCVHeavy + sa.mCVBuilding;
   sa.mHPTotal = sa.mHPLight + sa.mHPLightArmored + sa.mHPMedium + sa.mHPMediumAir + sa.mHPHeavy + sa.mHPBuilding;
   sa.mSPTotal = sa.mSPLight +  sa.mSPLightArmored + sa.mSPMedium + sa.mSPMediumAir + sa.mSPHeavy + sa.mSPBuilding;
   sa.mDPSTotal = sa.mDPSLight + sa.mDPSLightArmored + sa.mDPSMedium + sa.mDPSMediumAir + sa.mDPSHeavy + sa.mDPSBuilding;
   sa.mCVStarsTotal = sa.mCVStarsLight + sa.mCVStarsLightArmored + sa.mCVStarsMedium + sa.mCVStarsMediumAir + sa.mCVStarsHeavy + sa.mCVStarsBuilding;

   // Now we have the totals, get some normalized values.
   if (sa.mCVTotal > 0.0f)
   {
      sa.mCVPercentLight = sa.mCVLight / sa.mCVTotal;
      sa.mCVPercentLightArmored = sa.mCVLightArmored / sa.mCVTotal;
      sa.mCVPercentMedium = sa.mCVMedium / sa.mCVTotal;
      sa.mCVPercentMediumAir = sa.mCVMediumAir / sa.mCVTotal;
      sa.mCVPercentHeavy = sa.mCVHeavy / sa.mCVTotal;
      sa.mCVPercentBuilding = sa.mCVBuilding / sa.mCVTotal;
   }
   else
   {
      sa.mCVPercentLight = 0.0f;
      sa.mCVPercentLightArmored = 0.0f;
      sa.mCVPercentMedium = 0.0f;
      sa.mCVPercentMediumAir = 0.0f;
      sa.mCVPercentHeavy = 0.0f;
      sa.mCVPercentBuilding = 0.0f;
   }

   if (sa.mHPTotal > 0.0f)
   {
      sa.mHPPercentLight = sa.mHPLight / sa.mHPTotal;
      sa.mHPPercentLightArmored = sa.mHPLightArmored / sa.mHPTotal;
      sa.mHPPercentMedium = sa.mHPMedium / sa.mHPTotal;
      sa.mHPPercentMediumAir = sa.mHPMediumAir / sa.mHPTotal;
      sa.mHPPercentHeavy = sa.mHPHeavy / sa.mHPTotal;
      sa.mHPPercentBuilding = sa.mHPBuilding / sa.mHPTotal;
   }
   else
   {
      sa.mHPPercentLight = 0.0f;
      sa.mHPPercentLightArmored = 0.0f;
      sa.mHPPercentMedium = 0.0f;
      sa.mHPPercentMediumAir = 0.0f;
      sa.mHPPercentHeavy = 0.0f;
      sa.mHPPercentBuilding = 0.0f;
   }

   if (sa.mCVStarsTotal > 0.0f)
   {
      float starsMax = sa.mCVStarsLight;
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsLightArmored));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMedium));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsMediumAir));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsHeavy));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, sa.mCVStarsBuilding));
      sa.mNormalizedStarsLight = sa.mCVStarsLight / starsMax;
      sa.mNormalizedStarsLightArmored = sa.mCVStarsLightArmored / starsMax;
      sa.mNormalizedStarsMedium = sa.mCVStarsMedium / starsMax;
      sa.mNormalizedStarsMediumAir = sa.mCVStarsMediumAir / starsMax;
      sa.mNormalizedStarsHeavy = sa.mCVStarsHeavy / starsMax;
      sa.mNormalizedStarsBuilding = sa.mCVStarsBuilding / starsMax;         
   }

   // Write out our local working analysis to the results.
   squadAnalysis = sa;
}


//==============================================================================
//==============================================================================
float BAI::calculateOffenseAToB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB)
{
   float offenseLightAToB = analysisA.mCVStarsLight * analysisB.mCVPercentLight;
   float offenseLightArmoredAToB = analysisA.mCVStarsLightArmored * analysisB.mCVPercentLightArmored;
   float offenseMediumAToB = analysisA.mCVStarsMedium * analysisB.mCVPercentMedium;
   float offenseMediumAirAToB = analysisA.mCVStarsMediumAir * analysisB.mCVPercentMediumAir;
   float offenseHeavyAToB = analysisA.mCVStarsHeavy * analysisB.mCVPercentHeavy;
   float offenseBuildingAToB = analysisA.mCVStarsBuilding * analysisB.mCVPercentBuilding;
   float offenseTotalAToB = offenseLightAToB + offenseLightArmoredAToB + offenseMediumAToB + offenseMediumAirAToB + offenseHeavyAToB + offenseBuildingAToB;
//#ifdef aiCombatHack
//   return (float)(analysisA.mNumSquads);   // One point for each squad here, ignore enemy.
//#endif
   return (offenseTotalAToB);
}


//==============================================================================
//==============================================================================
bool BAI::canAHurtB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB)
{
//#ifdef aiCombatHack
//   if(analysisA.mNumSquads > 0)
//      return(true);
//   else
//      return(false);    // Really stupid, but return true if you have squads.
//#endif
   if (analysisA.mNormalizedStarsLight * analysisB.mHPPercentLight > 0.0f)
      return (true);
   else if (analysisA.mNormalizedStarsLightArmored * analysisB.mHPPercentLightArmored > 0.0f)
      return (true);
   else if (analysisA.mNormalizedStarsMedium * analysisB.mHPPercentMedium > 0.0f)
      return (true);
   else if (analysisA.mNormalizedStarsMediumAir * analysisB.mHPPercentMediumAir > 0.0f)
      return (true);
   else if (analysisA.mNormalizedStarsHeavy * analysisB.mHPPercentHeavy > 0.0f)
      return (true);
   else if (analysisA.mNormalizedStarsBuilding * analysisB.mHPPercentBuilding > 0.0f)
      return (true);
   else
      return (false);
}


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BAI::debugRender()
{
   if (gGame.getAIDebugType() == AIDebugType::cNone)
      return;

   // Debug render our mission targets.
   debugRenderMissionTargets();

   uint numAIMissions = mMissionIDs.getSize();
   for (uint i=0; i<numAIMissions; i++)
   {
      BAIMission* pAIMission = gWorld->getAIMission(mMissionIDs[i]);
      if (pAIMission)
         pAIMission->debugRender();
   }
}
#endif


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BAI::debugRenderMissionTargets()
{
   if (gGame.getAIDebugType() != AIDebugType::cAIMissionTargets)
      return;
//-- FIXING PREFIX BUG ID 5218
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return;
   if (mPlayerID != pUser->getPlayerID())
      return;

   uint numMissionTargets = mMissionTargetIDs.getSize();
   for (uint i=0; i<numMissionTargets; i++)
   {
      const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(mMissionTargetIDs[i]);
      if (!pTarget)
         continue;

      BVector pos = pTarget->getPosition();
      float rad = pTarget->getRadius();
      DWORD playerColor = cDWORDWhite;
      if (pTarget->isTargetType(MissionTargetType::cKBBase))
      {
         const BKBBase* pKBBase = gWorld->getKBBase(pTarget->getKBBaseID());
         if (pKBBase)
         {
            playerColor = gWorld->getPlayerColor(pKBBase->getPlayerID(), BWorld::cPlayerColorContextUI);
            pos = pKBBase->getPosition();
            rad = pKBBase->getRadius();
         }
      }

      if (!gWorld->isSphereVisible(pos, rad))
         continue;

      BHandle hFont = gFontManager.getFontCourier10();
      gFontManager.setFont(hFont);
      float sx = 0.0f;
      float sy = 0.0f;
      gRender.getViewParams().calculateWorldToScreen(pTarget->getPosition(), sx, sy);
      float lineHeight = gFontManager.getLineHeight();

      BFixedString128 debugText;
      debugText.format("DebugID = %d", pTarget->mDebugID);
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;
      
      debugText.format("TargetType = %s", gDatabase.getMissionTargetTypeName(pTarget->getTargetType()));
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      debugText.format("Position = (%f, %f)", pTarget->getPosition().x, pTarget->getPosition().z);
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      debugText.format("Last Score Total = %f", pTarget->getScore().getTotal());
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      const BAIMissionTargetWrapperIDArray& wrapperIDs = pTarget->getWrapperRefs();
      uint numWrapperRefs = wrapperIDs.getSize();
      if (numWrapperRefs == 0)
      {
         gTerrainSimRep.addDebugCircleOverTerrain(pos, rad, playerColor, 1.0f, BDebugPrimitives::cCategoryAI);
         debugText.format("No Missions For This Target");
         gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
         sy += lineHeight;
      }
      else
      {
         gTerrainSimRep.addDebugThickCircleOverTerrain(pos, rad, 0.5f, playerColor, 1.0f, BDebugPrimitives::cCategoryAI);
         for (uint i=0; i<numWrapperRefs; i++)
         {
            const BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(wrapperIDs[i]);
            BASSERT(pWrapper);
            if (!pWrapper)
               continue;

            const BAIMission* pMission = gWorld->getAIMission(pWrapper->getMissionID());
            if (!pMission)
               continue;

            debugText.format("Mission DebugID = %d, Type = %s", pMission->mDebugID, gDatabase.getMissionTypeName(pMission->getType()));
            gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
            sy += lineHeight;
         }
      }
   }
}
#endif


//==============================================================================
//==============================================================================
float BAI::calculateOffenseRatioAToB(const BAISquadAnalysis& analysisA, const BAISquadAnalysis& analysisB)
{
   float offenseAToB = calculateOffenseAToB(analysisA, analysisB);
   float offenseBToA = calculateOffenseAToB(analysisB, analysisA);
   float overallOffense = offenseAToB + offenseBToA;



   // Sort of misleading but this means both analysis have someone who can hurt the other guy.
   if (overallOffense > 0.0f)
   {
      return (offenseAToB / overallOffense);
   }
   else
   {
      // If we are here, at least one of the groups should not be able to put up a fight...
      bool hurtAbilityAToB = canAHurtB(analysisA, analysisB);
      bool hurtAbilityBToA = canAHurtB(analysisB, analysisA);
      BASSERTM(!hurtAbilityAToB || !hurtAbilityBToA, "One of these should be false!");
      if (hurtAbilityAToB && !hurtAbilityBToA)
         return (1.0f);
      else
         return (0.0f);
   }
}


//==============================================================================
//==============================================================================
float BAI::calculateUnopposedTimeToKill(const BAISquadAnalysis& attackers, const BAISquadAnalysis& targets, float maxTimeLimit)
{  // Time limit in seconds, not milliseconds.
   float hpspLight = targets.mHPLight + targets.mSPLight;
   float hpspLightArmored = targets.mHPLightArmored + targets.mSPLightArmored;
   float hpspMedium = targets.mHPMedium + targets.mSPMedium;
   float hpspMediumAir = targets.mHPMediumAir + targets.mSPMediumAir;
   float hpspHeavy = targets.mHPHeavy + targets.mSPHeavy;
   float hpspBuilding = targets.mHPBuilding + targets.mSPBuilding;
   float dpsLight = attackers.mDPSLight;
   float dpsLightArmored = attackers.mDPSLightArmored;
   float dpsMedium = attackers.mDPSMedium;
   float dpsMediumAir = attackers.mDPSMediumAir;
   float dpsHeavy = attackers.mDPSHeavy;
   float dpsBuilding = attackers.mDPSBuilding;
   float longestTimeToKill = 0.0f;
#ifdef aiCombatHack
   if (attackers.mNumSquads > 0)
   {
      if (targets.mNumSquads > 0)
         return ( targets.mNumSquads / (0.05f * attackers.mNumSquads) );
      else
         return ( 0.1f );
   }
   else
      return (maxTimeLimit);
#endif
   if (hpspLight > 0.0f)
   {
      if (dpsLight > 0.0f)
      {
         float timeLight = hpspLight / dpsLight;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeLight, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   if (hpspLightArmored > 0.0f)
   {
      if (dpsLightArmored > 0.0f)
      {
         float timeLightArmored = hpspLightArmored / dpsLightArmored;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeLightArmored, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   if (hpspMedium > 0.0f)
   {
      if (dpsMedium > 0.0f)
      {
         float timeMedium = hpspMedium / dpsMedium;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeMedium, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   if (hpspMediumAir > 0.0f)
   {
      if (dpsMediumAir > 0.0f)
      {
         float timeMediumAir = hpspMediumAir / dpsMediumAir;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeMediumAir, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   if (hpspHeavy > 0.0f)
   {
      if (dpsHeavy > 0.0f)
      {
         float timeHeavy = hpspHeavy / dpsHeavy;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeHeavy, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   if (hpspBuilding > 0.0f)
   {
      if (dpsBuilding > 0.0f)
      {
         float timeBuilding = hpspBuilding / dpsBuilding;
         longestTimeToKill = static_cast<float>(Math::fSelectMax(timeBuilding, longestTimeToKill));
      }
      else
         return (maxTimeLimit);
   }

   return (longestTimeToKill);
}

//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BAI::aiLog(const char *lpszFormat, ...)
{
   if (mLog >= 0)
   {
      int length;
      const long cMaxLength = 256;
      char logText[cMaxLength+20];     // Leave room for timestamp
      va_list ap;
      va_start(ap, lpszFormat);
      length = bvsnprintf(logText, cMaxLength, lpszFormat, ap);
      va_end(ap);

      BASSERT(length >= 0);                         // no errors;
      BASSERT(length < cMaxLength);   // it fit

      BString textOut = "";      // Prepend time stamp
      long time = gWorld->getGametime();
      long hours = time / 3600000;
      long mins = (time % 3600000) / 60000;
      long secs = (time % 60000) / 1000;
      long mills = time % 1000;
      textOut.format("%2d:%02d:%02d.%03d  %s",hours, mins, secs, mills, logText);
      //textOut.asANSI(logText);      

      BString buffer;
      const char* ansiString = textOut.asANSI(buffer);

      blogh(mLog, ansiString);
   }
}
#endif



//==============================================================================
// BAI::save
//==============================================================================
bool BAI::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSPTR(pStream, saveType, mpAIGlobals);
   GFWRITEMARKER(pStream, cSaveMarkerAIGlobals);

   GFWRITECLASSPTR(pStream, saveType, mpBidManager);
   GFWRITEMARKER(pStream, cSaveMarkerBidManager);

   GFWRITEARRAY(pStream, BAIMissionTargetID, mMissionTargetIDs, uint16, 2000);
   GFWRITEARRAY(pStream, BAIMissionID, mMissionIDs, uint16, 2000);
   GFWRITEARRAY(pStream, BAITopicID, mTopicIDs, uint16, 2000);
   GFWRITECLASS(pStream, saveType, mScoreModifier);
   GFWRITECLASS(pStream, saveType, mPlayerModifier);
   GFWRITEVAR(pStream, BAITopicID, mAIQTopic);
   GFWRITEVAR(pStream, BTriggerScriptID, mAITriggerScriptID);
   GFWRITECLASSARRAY(pStream, saveType, mDifficultySettings, uint8, 64);
   GFWRITEVAR(pStream, DWORD, mMaxMissionFocusTime);
   GFWRITEVAR(pStream, DWORD, mThinkCostMin);
   GFWRITEVAR(pStream, DWORD, mThinkCostMax);
   GFWRITEVAR(pStream, DWORD, mActCostMin);
   GFWRITEVAR(pStream, DWORD, mActCostMax);
   GFWRITEVAR(pStream, DWORD, mPowerEvalIntervalMin);
   GFWRITEVAR(pStream, DWORD, mPowerEvalIntervalMax);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEBITBOOL(pStream, mFlagAIQ);
   GFWRITEBITBOOL(pStream, mFlagPaused);

   GFWRITEMARKER(pStream, cSaveMarkerAI);
   return true;
}

//==============================================================================
// BAI::load
//==============================================================================
bool BAI::load(BStream* pStream, int saveType)
{  
   GFREADCLASSPTR(pStream, saveType, mpAIGlobals);
   GFREADMARKER(pStream, cSaveMarkerAIGlobals);

   GFREADCLASSPTR(pStream, saveType, mpBidManager);
   GFREADMARKER(pStream, cSaveMarkerBidManager);

   GFREADARRAY(pStream, BAIMissionTargetID, mMissionTargetIDs, uint16, 2000);
   GFREADARRAY(pStream, BAIMissionID, mMissionIDs, uint16, 2000);
   GFREADARRAY(pStream, BAITopicID, mTopicIDs, uint16, 2000);
   GFREADCLASS(pStream, saveType, mScoreModifier);
   if (BAI::mGameFileVersion >= 3)
      GFREADCLASS(pStream, saveType, mPlayerModifier);
   GFREADVAR(pStream, BAITopicID, mAIQTopic);
   if (BAI::mGameFileVersion >= 5)
      GFREADVAR(pStream, BTriggerScriptID, mAITriggerScriptID);
   GFREADCLASSARRAY(pStream, saveType, mDifficultySettings, uint8, 64);
   GFREADVAR(pStream, DWORD, mMaxMissionFocusTime);
   GFREADVAR(pStream, DWORD, mThinkCostMin);
   GFREADVAR(pStream, DWORD, mThinkCostMax);
   GFREADVAR(pStream, DWORD, mActCostMin);
   GFREADVAR(pStream, DWORD, mActCostMax);
   if (BAI::mGameFileVersion >= 4)
   {
      GFREADVAR(pStream, DWORD, mPowerEvalIntervalMin);
      GFREADVAR(pStream, DWORD, mPowerEvalIntervalMax);
   }
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADBITBOOL(pStream, mFlagAIQ);
   GFREADBITBOOL(pStream, mFlagPaused);

   GFREADMARKER(pStream, cSaveMarkerAI);
   return true;
}

