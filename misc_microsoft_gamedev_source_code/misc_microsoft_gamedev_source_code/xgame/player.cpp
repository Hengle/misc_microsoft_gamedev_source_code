//==============================================================================
// player.cpp
//
// Copyright (c) Ensemble Studios, 2005-2008
//==============================================================================

// xgame
#include "common.h"
#include "alert.h"
#include "player.h"
#include "ai.h"
#include "aifactoid.h"
#include "civ.h"
#include "commandmanager.h" 
#include "commands.h"
#include "configsgame.h"
#include "database.h"
#include "game.h"
#include "hintengine.h"
#include "leaders.h"
#include "protoobject.h"
#include "protosquad.h"
#include "prototech.h"
#include "protopower.h"
#include "team.h"
#include "techtree.h"
#include "object.h"
#include "syncmacros.h"
#include "world.h"
#include "statsManager.h"
#include "unitactionbuilding.h"
#include "weapontype.h"
#include "savegame.h"
#include "ability.h"
#include "humanPlayerAITrackingData.h"
#include "user.h"
#include "achievementmanager.h"
#include "gamesettings.h"
#include "scoremanager.h"
#include "campaignmanager.h"
#include "tactic.h"
#include "skullmanager.h"

// xlive
#include "liveSystem.h"

// xvince
#include "vincehelper.h"

GFIMPLEMENTVERSION(BPlayer, 9);
enum
{
   cSaveMarkerPlayer1=10000,
   cSaveMarkerPlayer2,
   cSaveMarkerPlayer3,
   cSaveMarkerPlayer4,
};

//==============================================================================
// BPlayer::BPlayer
//==============================================================================
BPlayer::BPlayer() :
   mID(-1),
   mColorIndex(-1),
   mMPID( -1 ),
   mXUID(0),
   mCoopID(cInvalidPlayerID),
   mScenarioID(cInvalidPlayerID),
   mCivID(-1),
   mTeamID(-1),
   mPlayerState(cPlayerStatePlaying),
   mNetState(cPlayerNetStateConnected),
   mLeaderID(-1),
   mpUser(NULL),
   mpPops(NULL),
   mpTrackingData(NULL),
   mPopCount(0),
   mBountyResource(-1),
   mRallyObject(cInvalidObjectID),
   mTotalUnitCounts(0),
   mTotalFutureUnitCounts(0),
   mTotalDeadUnitCounts(0),
   mTotalSquadCounts(0),
   mTotalFutureSquadCounts(0),
   mTotalDeadSquadCounts(0),
   mpTechTree(NULL),
   mResources(),
   mTotalResources(),
   mResourceTrickleRate(),
   mpStats(NULL),
   mpRateAmounts(NULL),
   mpRateMultipliers(NULL),
   mpAlertManager(NULL),
   mStrength(0),
   mTributeCost(0.0),
   mRepairCost(),
   mRepairTime(0.0f),
   mHandicapMultiplier(1.0f),
   mShieldRegenRate(1.0f),
   mShieldRegenDelay(0),
   mDifficulty(0.0f),
   mPlayerType(cPlayerTypeNPC),
   mSquadSearchAttempts(0),
   mpHintEngine(NULL),
   mLeaderUnit(cInvalidObjectID),
   mpFactoidManager(NULL),
   mGamePlayedTime(0),
   mFloodPoofPlayer(cInvalidPlayerID),
   mWeaponPhysicsMultiplier(1.0f),
   mAIDamageMultiplier(1.0f),
   mAIDamageTakenMultiplier(1.0f),
   mAIBuildSpeedMultiplier(1.0f),
   mSquadAISearchIndex(-1),
   mSquadAIWorkIndex(-1),
   mSquadAISecondaryTurretScanIndex(-1)
{
   mRallyPoint.init();

   //--Init Flags
   mFlagRallyPoint=false;
   mFlagBountyResource=false;
   mFlagMinimapBlocked=false;
   mFlagLeaderPowersBlocked=false;
   // mFlagHuman=false;
   mFlagDefeatedDestroy=false;
   mFlagProcessedGameEnd=false;
}

//==============================================================================
// BPlayer::~BPlayer
//==============================================================================
BPlayer::~BPlayer()
{
   reset();
}

//==============================================================================
// BPlayer::setup
//==============================================================================
bool BPlayer::setup(long id, long mpid, BSimString& name, BUString& localisedName, long civID, long leaderID, float difficulty, BSmallDynamicSimArray<long>* pForbidObjects, BSmallDynamicSimArray<long>* pForbidSquads, BSmallDynamicSimArray<long>* pForbidTechs, BSmallDynamicSimArray<BLeaderPop>* pLeaderPops, uint8 playerType, int scenarioID, bool fromSave)
{
   mID=id;
   mMPID=mpid;
   mCoopID=cInvalidPlayerID;
   mFloodPoofPlayer=cInvalidPlayerID;
   mScenarioID=scenarioID;
   mName=name;
   mLocalisedDisplayName=localisedName;
   mCivID=civID;
   mLeaderID=leaderID;
   mStrength = 0;
   mTributeCost = gDatabase.getTributeCost();
   mShieldRegenRate = gDatabase.getShieldRegenRate();
   mShieldRegenDelay = gDatabase.getShieldRegenDelay();
   

   // get the player difficulty
   mDifficulty = difficulty;


   mPlayerState=cPlayerStatePlaying;

//-- FIXING PREFIX BUG ID 6192
   const BCiv* pCiv=gDatabase.getCiv(mCivID);
//--
   if(!pCiv)
      return false;

//-- FIXING PREFIX BUG ID 6193
   const BLeader* pLeader=gDatabase.getLeader(leaderID);
//--
   if(!pLeader)
      return false;

   mRepairCost.set(pLeader->getRepairCost());
   mRepairTime=pLeader->getRepairTime();

   //-- init resources
   mResources = *(pLeader->getStartingResources());
   mTotalResources = mResources;
   for(long i=0; i<mResources.getNumberResources(); i++)
   {
      if(pLeader->isResourceActive(i))
         mActiveResources.set(i);
      else
         mActiveResources.unset(i);
   }

   mResourceTrickleRate.zero();

   //-- init pop
   mPopCount=gDatabase.getNumberPops();
   mpPops=new BPlayerPop[mPopCount];
   if(!mpPops)
      return false;
   Utils::FastMemSet(mpPops, 0, sizeof(BPlayerPop)*mPopCount);

   const BLeaderPopArray& leaderPops = pLeader->getPops();
   uint numLeaderPops = leaderPops.getSize();
   for (uint i=0; i<numLeaderPops; i++)
   {
      const BLeaderPop& pop = leaderPops[i];
      mpPops[pop.mID].mCap = pop.mCap;
      mpPops[pop.mID].mMax = pop.mMax;
   }

   //-- initialize the proto objects
   long numProtoObjects = gDatabase.getNumberProtoObjects();
   mProtoObjects.reserve(numProtoObjects);
   for(long i=0; i<numProtoObjects; i++)
   {
      BProtoObject* pGenericProtoObject=gDatabase.getGenericProtoObject(i);
      if (!gConfig.isDefined(cConfigNoProtoObjectOptimization))
      {
         int objectClass = pGenericProtoObject->getObjectClass();
         if (objectClass==cObjectClassObject)
         {
            mProtoObjects.add(pGenericProtoObject);
            continue;
         }
      }

      BProtoObject* pProtoObject=new BProtoObject(pGenericProtoObject);
      if(!pProtoObject)
         return false;
      pProtoObject->setFlagPlayerOwned(true);
      mProtoObjects.add(pProtoObject);

      BTactic* pTactic = pProtoObject->getTactic();
      if (pTactic)
         pTactic->setPlayerID(mID);

      float protoLOS = pProtoObject->getProtoLOS();
      float recipTileScale = gTerrainSimRep.getReciprocalDataTileScale();
      if (protoLOS <= 0.0f)
      {
         pProtoObject->setProtoSimLOS(0);
      }
      else
      {
         // Calculate the protoSimLOS but don't let it be zero if we want it to be positive.
         long protoSimLOS = (long)(protoLOS * recipTileScale);
         if (protoSimLOS <= 0)
         {
            protoSimLOS = 1;
         }
         pProtoObject->setProtoSimLOS(protoSimLOS);
      }
   }

   if(pForbidObjects)
   {
      for(long i=0; i<pForbidObjects->getNumber(); i++)
      {
         long id=(*pForbidObjects)[i];
         mProtoObjects[id]->setFlagForbid(true);
      }
   }

   //-- initialize the proto squads
   long numProtoSquads = gDatabase.getNumberProtoSquads();
   mProtoSquads.reserve(numProtoSquads);
   for(long i=0; i<numProtoSquads; i++)
   {
      BProtoSquad* pProtoSquad=new BProtoSquad(gDatabase.getGenericProtoSquad(i), this);
      if(!pProtoSquad)
         return false;
      mProtoSquads.add(pProtoSquad);
   }

   if(pForbidSquads)
   {
      for(long i=0; i<pForbidSquads->getNumber(); i++)
      {
         long id=(*pForbidSquads)[i];
         mProtoSquads[id]->setFlagForbid(true);
      }
   }

   //-- initialize the proto techs
   long numProtoTechs = gDatabase.getNumberProtoTechs();
   mProtoTechs.reserve(numProtoTechs);
   for(long i=0; i<numProtoTechs; i++)
   {
      BProtoTech* pProtoTech=new BProtoTech(gDatabase.getProtoTech(i));
      if(!pProtoTech)
         return false;
      mProtoTechs.add(pProtoTech);
   }

   if(pForbidTechs)
   {
      for(long i=0; i<pForbidTechs->getNumber(); i++)
      {
         long id=(*pForbidTechs)[i];
         mProtoTechs[id]->setFlagForbid(true);
      }
   }

   if(pLeaderPops)
   {
      uint numLeaderPops = pLeaderPops->getSize();
      for (uint i=0; i<numLeaderPops; i++)
      {
         const BLeaderPop& leaderPop = (*pLeaderPops)[i];
         mpPops[leaderPop.mID].mCap = leaderPop.mCap;
         mpPops[leaderPop.mID].mMax = leaderPop.mMax;
      }
   }

   // Init weapon types
   int numWeaponTypes=gDatabase.getNumberWeaponTypes();
   mWeaponTypes.reserve(numWeaponTypes);
   for (int i=0; i<numWeaponTypes; i++)
   {
      BWeaponType* pWeaponType = new BWeaponType();
      pWeaponType->init(gDatabase.getWeaponTypeByID(i));
      mWeaponTypes.add(pWeaponType);
   }

   // Init dynamic ability data
   int numAbilities=gDatabase.getNumberAbilities();
   mAbilityRecoverTimes.reserve(numAbilities);
   for (int i=0; i<numAbilities; i++)
   {
      float recoverTime=gDatabase.getAbilityFromID(i)->getRecoverTime();
      mAbilityRecoverTimes.add(recoverTime);
   }

   // Initialize unit & future unit counts.
   mTotalUnitCounts = 0;
   mTotalFutureUnitCounts = 0;
   mTotalDeadUnitCounts = 0;
   mUnitsByProtoObject.resize(numProtoObjects);
   mUnits.resize(0);
   mFutureUnitCounts.resize(numProtoObjects);
   mDeadUnitCounts.resize(numProtoObjects);
   mTotalCombatValue = 0.0f;
   for (long i=0; i<numProtoObjects; i++)
   {
      mUnitsByProtoObject[i].clear();
      mFutureUnitCounts[i] = 0;
      mDeadUnitCounts[i] = 0;
   }

   // Initialize squad & future squad counts.
   mTotalSquadCounts = 0;
   mTotalFutureSquadCounts = 0;
   mTotalDeadSquadCounts = 0;
   mSquadsByProtoSquad.resize(numProtoSquads);
   mSquads.resize(0);
   mFutureSquadCounts.resize(numProtoSquads);
   mDeadSquadCounts.resize(numProtoSquads);
   for (long i=0; i<numProtoSquads; i++)
   {
      mSquadsByProtoSquad[i].clear();
      mFutureSquadCounts[i] = 0;
      mDeadSquadCounts[i] = 0;
   }

   //-- init modifiable power data
   int protoPowerCount = gDatabase.getNumberProtoPowers();
   if (protoPowerCount > 0)
   {
      mPowerAvailableTime.setNumber(protoPowerCount);
      mPowerRechargeTime.resize(protoPowerCount);
      mPowerUseLimit.resize(protoPowerCount);
      mPowerLevel.resize(protoPowerCount);
      for (int i=0; i<protoPowerCount; i++)
      {
//-- FIXING PREFIX BUG ID 6191
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(i);
//--
         mPowerRechargeTime[i] = pProtoPower->getAutoRechargeTime();
         mPowerUseLimit[i] = pProtoPower->getUseLimit();
         mPowerLevel[i] = 0;
      }
   }

   //-- init leader support powers
   uint powerCount = pLeader->getSupportPowers().getSize();
   if (powerCount > 0)
   {
      mSupportPowerStatus.resize(powerCount);
      for (uint i=0; i<powerCount; i++)
         mSupportPowerStatus[i]=cSupportPowerStatusUnavailable;
   }

   //-- init rates
   int rateCount = gDatabase.getNumberRates();
   mpRateAmounts = new float[rateCount];
   mpRateMultipliers = new float[rateCount];
   for (int i=0; i<rateCount; i++)
   {
      mpRateAmounts[i]=0.0f;
      mpRateMultipliers[i]=1.0f;
   }

   //-- init tech tree
   mpTechTree=new BTechTree();
   if(!mpTechTree || !mpTechTree->init(this, fromSave))
      return false;
   if (!fromSave)
   {
      mpTechTree->activateTech(pCiv->getCivTech(), -1);
      mpTechTree->activateTech(pLeader->mLeaderTechID, -1);
   }

   //setFlag(cFlagCheckDefeated, true);

   //   setFlagHuman(human );
   mPlayerType = playerType;
   mSquadSearchAttempts=0;

   mpStats = gStatsManager.createStatsPlayer(this);

   mpAlertManager = new BAlertManager(this);

   if(mPlayerType == cPlayerTypeHuman)
   {
      mpHintEngine = new BHintEngine(this);
      mpHintEngine->initialize();
   }

   return true;
}


//==============================================================================
// BPlayer::initializeFactoidManager
//==============================================================================
bool BPlayer::initializeFactoidManager()
{
   // Create a factoid manager for this human player.
   mpFactoidManager = new BAIFactoidManager(mID);
   if (mpFactoidManager)
      mpFactoidManager->setPlayerID(this->getID());

   if (mpFactoidManager)
      return (true);
   else
      return(false);
}

//==============================================================================
// BPlayer::reset
//==============================================================================
void BPlayer::reset()
{
   // If we haven't already done so, passing in true will force us to calculate
   // all the end of game data even though the game is not yet over
   //
   // When playing Xbox LIVE games that are still in progress, this could cause
   // a potential disconnect between the Xbox LIVE Leaderboards and your Service Record
   processGameEnd(true);

   //for (long i = 0; i < cMaximumSupportedPlayers; i++)
   //   mVisibleObjects[i].clear();

   mUnitsByProtoObject.clear();
   mUnits.clear();
   mSquadsByProtoSquad.clear();
   mSquads.clear();
   mFutureUnitCounts.clear();
   mFutureSquadCounts.clear();
   mDeadUnitCounts.clear();
   mDeadSquadCounts.clear();
   mTotalUnitCounts = 0;
   mTotalSquadCounts = 0;
   mTotalFutureUnitCounts = 0;
   mTotalFutureSquadCounts = 0;
   mTotalDeadUnitCounts = 0;
   mTotalDeadSquadCounts = 0;
   mTotalCombatValue = 0.0f;

   int count=mWeaponTypes.getNumber();
   for (int i=0; i<count; i++)
   {
      BWeaponType* pWeaponType=mWeaponTypes[i];
      if (pWeaponType)
         delete pWeaponType;
   }
   mWeaponTypes.clear();

   if(mpTechTree)
   {
      delete mpTechTree;
      mpTechTree=NULL;
   }

   count=mProtoTechs.getNumber();
   for(long i=0; i<count; i++)
   {
      BProtoTech* pProtoTech=mProtoTechs[i];
      if(pProtoTech)
         delete pProtoTech;
   }
   mProtoTechs.clear();

   count=mProtoSquads.getNumber();
   for(long i=0; i<count; i++)
   {
      BProtoSquad* pProtoSquad=mProtoSquads[i];
      if(pProtoSquad)
         delete pProtoSquad;
   }
   mProtoSquads.clear();

   count=mProtoObjects.getNumber();
   for(long i=0; i<count; i++)
   {
      BProtoObject* pProtoObject=mProtoObjects[i];
      if(pProtoObject && pProtoObject->getFlagPlayerOwned())
         delete pProtoObject;
   }
   mProtoObjects.clear();

   // Clear out unique proto objects/squads
   count=mUniqueProtoSquads.getNumber();
   for(long i=0; i<count; i++)
   {
      BProtoSquad* pProtoSquad=mUniqueProtoSquads[i].mProtoSquad;
      if(pProtoSquad)
         delete pProtoSquad;
   }
   mUniqueProtoSquads.clear();

   count=mUniqueProtoObjects.getNumber();
   for(long i=0; i<count; i++)
   {
      BProtoObject* pProtoObject=mUniqueProtoObjects[i].mProtoObject;
      if(pProtoObject)
         delete pProtoObject;
   }
   mUniqueProtoObjects.clear();

   if(mpPops)
   {
      delete []mpPops;
      mpPops=NULL;
      mPopCount=0;
   }

   if (mpAlertManager)
   {
      delete mpAlertManager;
      mpAlertManager = NULL;
   }

   if(mpHintEngine)
   {
      delete mpHintEngine;
      mpHintEngine = NULL;
   }

   if(mpFactoidManager)
   {
      delete mpFactoidManager;
      mpFactoidManager = NULL;
   }

   mResources.zero();
   mTotalResources.zero();
   mResourceTrickleRate.zero();
   mRepairCost.zero();

   mpStats = NULL;

   if (mpRateAmounts)
   {
      delete[] mpRateAmounts;
      mpRateAmounts=NULL;
   }

   if (mpRateMultipliers)
   {
      delete[] mpRateMultipliers;
      mpRateMultipliers=NULL;
   }

   if (mpTrackingData)
   {
      delete mpTrackingData;
      mpTrackingData = NULL;
   }
   
   mSquadAISquads.clear();
   mSquadAISearchIndex=-1;
   mSquadAIWorkIndex=-1;
   mSquadAISecondaryTurretScanIndex=-1;
}

//=============================================================================
// BPlayer::setTrackingData
//=============================================================================
void BPlayer::setTrackingData(BHumanPlayerAITrackingData* pTrackingData) 
{
   if (mpTrackingData)
      delete mpTrackingData;

   mpTrackingData = pTrackingData; 
}

//==============================================================================
// BPlayer::update
//==============================================================================
void BPlayer::update(float elapsedTime)
{
#ifdef SYNC_FinalRelease
   syncFinalReleaseData("FRP ID", mID);
   for (long i=0; i<mResources.getNumberResources(); i++)
   {
      syncFinalReleaseData("FRP RA", mResources.get(i));
   }
#endif

#ifdef SYNC_Player
   for (long i=0; i<mTotalResources.getNumberResources(); i++)
   {
      syncPlayerData("BPlayer::update mTotalResources", mTotalResources.get(i));
   }
   for (long i=0; i<mResourceTrickleRate.getNumberResources(); i++ )
   {
      syncPlayerData("BPlayer::update mResourceTrickleRate", mResourceTrickleRate.get(i));
   }
#endif

   // Power grant timers.
   updatePowerRecharge();

   // Resource trickle
   updateResourceTrickle(elapsedTime);

   // Support power status
   updateSupportPowerStatus();

   //Squad tokens.
   updateSquadAITokens();

   if (isPlaying() && !gWorld->isPlayingCinematic())
   {
      SCOPEDSAMPLE(AlertManagerUpdate);
      mpAlertManager->setNewAlertsAllowed(true);
      mpAlertManager->update();
   }
   // If alerts won't be updated anymore then don't allow new alerts to be created either
   // since this will make it eat up memory that won't be released
   else if (!isPlaying())
   {
      mpAlertManager->setNewAlertsAllowed(false);
   }

   // Update the Factoid Manager (player coach) if any
   if (mpFactoidManager)
      mpFactoidManager->update();

   if(mpHintEngine != NULL)
      mpHintEngine->update(elapsedTime);
}

//==============================================================================
// BPlayer::render
//==============================================================================
void BPlayer::render()
{
   if(mpAlertManager)
      mpAlertManager->render();
}


//==============================================================================
//==============================================================================
void BPlayer::setCoopID(BPlayerID id)
{ 
   mCoopID=id;
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      if (!gWorld->getPlayer(mCoopID))
      {
         mCoopID = -1;
         return;
      }
      mResources.zero();
      mTotalResources.zero();
   }
}

//==============================================================================
// BPlayer::getCiv
//==============================================================================
BCiv* BPlayer::getCiv() const
{
   return gDatabase.getCiv(mCivID);
}

//==============================================================================
// BPlayer::getLeader
//==============================================================================
BLeader* BPlayer::getLeader() const
{
   return gDatabase.getLeader(mLeaderID);
}

//==============================================================================
// BPlayer::getProtoObject
//==============================================================================
BProtoObject* BPlayer::getProtoObject(long id) const
{
   if (id < 0)
      return NULL;

   //If we're over the generic limit, check for unique POs.
   if (id >= mProtoObjects.getNumber())
   {
      long ui=getUniqueProtoObjectIndex(id);
      if ((ui < 0) || (ui >= mUniqueProtoObjects.getNumber()) )
         return(NULL);
      return(mUniqueProtoObjects[ui].mProtoObject);
   }
   // Otherwise, return the generic
   else
      return mProtoObjects[id];
}

//==============================================================================
//==============================================================================
long BPlayer::getUniqueProtoObjectIndex(long id) const
{
   long playerID = GETUNIQUEPROTOPLAYERID(id);
   if (playerID != getID())
      return -1;
   long protoID = GETUNIQUEPROTOID(id);
   long index = protoID - mProtoObjects.getNumber();
   return index;
}

//==============================================================================
//==============================================================================
long BPlayer::getUniqueProtoObjectIDFromIndex(long index) const
{
   long id = MAKEUNIQUEPID(getID(), (index + mProtoObjects.getNumber()));
   return id;
}

//==============================================================================
// BPlayer::getProtoSquad
//==============================================================================
BProtoSquad* BPlayer::getProtoSquad(long id) const
{
   if (id < 0)
      return NULL;

   //If we're over the generic limit, check for unique POs.
   if (id >= mProtoSquads.getNumber())
   {
      long ui=getUniqueProtoSquadIndex(id);
      if ((ui < 0) || (ui >= mUniqueProtoSquads.getNumber()) )
         return(NULL);
      return(mUniqueProtoSquads[ui].mProtoSquad);
   }
   // Otherwise, return the generic
   else
      return mProtoSquads[id];
}

//==============================================================================
//==============================================================================
long BPlayer::getUniqueProtoSquadIndex(long id) const
{
   long playerID = GETUNIQUEPROTOPLAYERID(id);
   if (playerID != getID())
      return -1;
   long protoID = GETUNIQUEPROTOID(id);
   long index = protoID - mProtoSquads.getNumber();
   return index;
}

//==============================================================================
//==============================================================================
long BPlayer::getUniqueProtoSquadIDFromIndex(long index) const
{
   long id = MAKEUNIQUEPID(getID(), (index + mProtoSquads.getNumber()));
   return id;
}

//==============================================================================
// BPlayer::getProtoTech
//==============================================================================
BProtoTech* BPlayer::getProtoTech(long id) const
{
   return ((id >= 0 && id < mProtoTechs.getNumber()) ? mProtoTechs[id] : NULL);
}

//==============================================================================
// BPlayer::sendResign
//==============================================================================
void BPlayer::sendResign()
{
   if( !isPlaying() )
      return;
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeResign);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// BPlayer::doResign
//==============================================================================
void BPlayer::doResign()
{
   if( !isPlaying() )
      return;
   setPlayerState(cPlayerStateResigned);
}

//==============================================================================
// BPlayer::sendDisconnect
//==============================================================================
void BPlayer::sendDisconnect()
{
   if( !isPlaying() )
      return;
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeDisconnect);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// BPlayer::doDisconnect
//==============================================================================
void BPlayer::doDisconnect()
{
   if( !isPlaying() )
      return;
   setPlayerState(cPlayerStateDisconnected);
}

//==============================================================================
// BPlayer::setPlayerState
//==============================================================================
void BPlayer::setPlayerState(BPlayerState playerState, bool fromSave)
{
   BASSERT( (playerState>=cPlayerStatePlaying) && (playerState<cPlayerStateCount) );

   if (playerState == cPlayerStateDisconnected)
   {
      mNetState = cPlayerNetStateDisconnected;
      if (mPlayerState == cPlayerStatePlaying)
         playerState = cPlayerStateResigned;
   }

   if (playerState == mPlayerState)
      return;

   // do not allow the player to transition to the disconnected state if they are no longer playing
   // allow the previous resign, defeated or won states to prevail
   //if (playerState == cPlayerStateDisconnected && mPlayerState != cPlayerStatePlaying)
   //   return;

   if ((playerState!=cPlayerStatePlaying) &&
      (mPlayerState==cPlayerStatePlaying))
   {
      //Time (rounded to closest second) when THIS player's game was over
      mGamePlayedTime = uint32((gWorld->getGametime() + 500)/1000);
   }
   
   mPlayerState = playerState;

   if (mPlayerState == cPlayerStateResigned || mPlayerState == cPlayerStateDefeated)
      mFlagDefeatedDestroy=true;

   //if (mNetState == cPlayerNetStateDisconnected)
   //   gWorld->notify(BEntity::cEventPlayerDisconnected, cInvalidObjectID, mID, 0);

   // if a state other than playing occurred, then run it up the flag pole for 
   //    others to see.
   switch(playerState)
   {
      case cPlayerStateResigned:
         gWorld->notify(BEntity::cEventPlayerResigned, cInvalidObjectID, mID, 0);
         break;
      case cPlayerStateDefeated:
         gWorld->notify(BEntity::cEventPlayerDefeated, cInvalidObjectID, mID, 0);
         break;
      //case cPlayerStateDisconnected:
      //   gWorld->notify(BEntity::cEventPlayerDisconnected, cInvalidObjectID, mID, 0);
      //   break;
      case cPlayerStateWon:
         gWorld->notify(BEntity::cEventPlayerWon, cInvalidObjectID, mID, 0);
         break;
   }

   // If the player is no longer playing, kill his AI & script.
   if (playerState != cPlayerStatePlaying)
   {
      gWorld->deleteAI(mID);
   }

   //processGameEnd();
}

//==============================================================================
// BPlayer::processGameEnd
//==============================================================================
void BPlayer::processGameEnd(bool force)
{
   if (mFlagProcessedGameEnd)
      return;
   if (!gWorld->getFlagGameOver() && !force)
      return;

   mFlagProcessedGameEnd = true;

   if(!getUser() || !getUser()->getProfile())
      return;

   BUserProfile *profile = getUser()->getProfile();

   BGameSettings* pSettings = gDatabase.getGameSettings();
   BASSERT(pSettings);

   //campaign, skirmish, etc
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);

   long gameModeID = -1;
   result = pSettings->getLong(BGameSettings::cGameMode, gameModeID);
   BASSERT(result);

   BFixedStringMaxPath currentMap;
   result = pSettings->getString(BGameSettings::cMapName, currentMap, MAX_PATH);
   BASSERT(result);

   long leaderID = getLeaderID();
            
   bool ranked = false;
   if( gLiveSystem && gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->isMatchmadeGame() )
      ranked = true;

   gScoreManager.computeFinalScores(force);

   long score = gScoreManager.getFinalScore(getID());
   long grade = gScoreManager.getFinalGrade(getID());
   long difficulty = getDifficultyType();
   uint mode = gWorld->getFlagCoop() ? 1 : 0;
   uint time = gScoreManager.getCompletionTime(getID()) / 1000;

   // if it's a matchmaking game, update our matchmaking rank
   if(ranked)
      profile->updateMatchmakingRank(mPlayerState == BPlayer::cPlayerStateWon, score);

   //const BScoreScenario &scenarioData = gScoreManager.getScenarioData();
   uint partime = gScoreManager.getScenarioData().mMissionMaxParTime / 1000;

   //Save off the length of time playing.
   uint32 elapsedSeconds = (uint32)(gWorld->getGametime() / 1000);
   profile->addElapsedGameTime(elapsedSeconds);
   gAchievementManager.updateMisc(getUser(), cMiscATPlayedTime);

   // did we just complete a campaign scenario?  If so, update campaign progress
   if ((gameType == BGameSettings::cGameTypeCampaign) && (mPlayerState==cPlayerStateWon))
   {

      BCampaign *campaign = gCampaignManager.getCampaign(0); //hack hardcoded like everywhere else because we only have one campaign.
      if( campaign )
      {
         // [10/15/2008 xemu] ok, if we are in co-op we can't trust the campaign current node ID since that is our solo campaign progress
         // [11/6/08 SRL] while we are in-game / post game, the settings are valid. Use those to get the campaign current node. only use the
         //                current campaign node pregame.
         BGameSettings* pSettings = gDatabase.getGameSettings();
         BString mapName;
         pSettings->getString( BGameSettings::cMapName, mapName );
         long nodeID = campaign->getNodeIDByFilename(mapName);
         profile->mCampaignProgress.enterScore(nodeID, mode, difficulty, getID(), score, grade, time, partime);

         // to fix campaign end things, we are going to put all game types at this next node.
         // if we are in single player campaign, move our progress along here so that the value gets written to the profile.
//          if (!gWorld->getFlagCoop())
//          {
            campaign->incrementCurrentNode();
            campaign->saveToCurrentProfile();
//          }
      }
   }

   //Let the collection manager know we just won a skirmish game
   if( (mPlayerState==cPlayerStateWon) && (gameType == BGameSettings::cGameTypeSkirmish))
      gCollectiblesManager.updateGame(getUser(), pSettings);

   //Send the details of the game to the achievement manager for achievement tracking.
   if (gameType == BGameSettings::cGameTypeSkirmish)
      gAchievementManager.updateSkirmish(getUser(), pSettings, mPlayerState==cPlayerStateWon, ranked, difficulty);  
   else if (gameType == BGameSettings::cGameTypeCampaign)
      gAchievementManager.updateCampaign(getUser(), pSettings, mPlayerState==cPlayerStateWon, false, ranked, difficulty, grade);  

   //Send details to the service record
   if (gameType == BGameSettings::cGameTypeSkirmish)
      profile->reportSkirmishGameCompleted(mPlayerState==cPlayerStateWon, leaderID, currentMap, gameModeID, ranked);  

   if (gameType == BGameSettings::cGameTypeSkirmish)
      getUser()->processAutoDifficulty();

   gUserProfileManager.writeProfile(getUser());
}

//==============================================================================
//==============================================================================
void BPlayer::setResourceTrickleRate(long resourceID, float rate)
{ 
#ifdef SYNC_Player
   syncPlayerData("BPlayer::setResourceTrickleRate mID", mID);
   syncPlayerData("BPlayer::setResourceTrickleRate resourceID", resourceID);
   syncPlayerData("BPlayer::setResourceTrickleRate rate", rate);
#endif
   return mResourceTrickleRate.set(resourceID, rate);
}

//==============================================================================
//==============================================================================
void BPlayer::setResourceTrickleRate(BCost cost) 
{ 
#ifdef SYNC_Player
   syncPlayerData("BPlayer::setResourceTrickleRate mID", mID);
   for (long i=0; i<cost.getNumberResources(); i++)
   {
      syncPlayerData("BPlayer::setResourceTrickleRate", cost.get(i));
   }
#endif
   mResourceTrickleRate = cost; 
}

//==============================================================================
// Return the difficulty type for this player
//==============================================================================
BDifficultyType BPlayer::getDifficultyType() const
{
   float normal = gDatabase.getDifficultyNormal();
   float hard = gDatabase.getDifficultyHard();
   float legendary = gDatabase.getDifficultyLegendary();
   if (mDifficulty < normal)
   {
      return (DifficultyType::cEasy);
   }
   else if ((mDifficulty >= normal) && (mDifficulty < hard))
   {
      return (DifficultyType::cNormal);
   }
   else if ((mDifficulty >= hard) && (mDifficulty < legendary))
   {
      return (DifficultyType::cHard);
   }
   else
   {
      return (DifficultyType::cLegendary);
   }
}

//==============================================================================
//==============================================================================
float BPlayer::getResource(long resourceID) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->getResource(resourceID);
   return mResources.get(resourceID);
}

//==============================================================================
//==============================================================================
void BPlayer::setResource(long resourceID, float amount)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->setResource(resourceID, amount);
      return;
   }
   mResources.set(resourceID, amount); 
}

//==============================================================================
//==============================================================================
void BPlayer::setResources(const BCost *pResources)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->setResources(pResources);
      return;
   }
   mResources.set(pResources);
}

//==============================================================================
//==============================================================================
const BCost& BPlayer::getResources() const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->getResources();
   return(mResources);
}

//==============================================================================
//==============================================================================
void BPlayer::addResource(long resourceID, float amount, uint flags, bool noHandicap)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->addResource(resourceID, amount, flags, noHandicap);
      return;
   }
   float adjustedAmount = (noHandicap ? amount : amount * mHandicapMultiplier);
#ifdef SYNC_Player
   syncPlayerData("BPlayer::addResource mID", mID);
   syncPlayerData("BPlayer::addResource resourceID", resourceID);
   syncPlayerData("BPlayer::addResource amount", amount);
   syncPlayerData("BPlayer::addResource adjustedAmount", adjustedAmount);
#endif
   mResources.add(resourceID, adjustedAmount);
   addTotalResource( resourceID, adjustedAmount );
   if (mpStats)
      mpStats->addResource(resourceID, adjustedAmount, flags);
}

//==============================================================================
// BPlayer::checkCost
//==============================================================================
bool BPlayer::checkCost(const BCost* pCost) const
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->checkCost(pCost);
   return(mResources.isAtLeast(*pCost));
}

//==============================================================================
// BPlayer::payCost
//==============================================================================
bool BPlayer::payCost(const BCost*  pCost)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->payCost(pCost);
   if (!mResources.isAtLeast(*pCost))
      return(false);
   //mResources.subtract(pCost);
   for (int i=0; i<mResources.getNumberResources(); i++)
   {
      if (gDatabase.getResourceDeductable(i))
         mResources.add(i, -pCost->get(i));
   }
   return(true);
}

//==============================================================================
// BPlayer::refundCost
//==============================================================================
bool BPlayer::refundCost(const BCost*  pCost)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->refundCost(pCost);
   //mResources.add(pCost);
   for (int i=0; i<mResources.getNumberResources(); i++)
   {
      if (gDatabase.getResourceDeductable(i))
         mResources.add(i, pCost->get(i));
   }
   return true;
}

//==============================================================================
//==============================================================================
float BPlayer::getTotalResource(long resourceID) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->getTotalResource(resourceID);
   return (mTotalResources.get(resourceID));
}

//==============================================================================
//==============================================================================
void BPlayer::setTotalResource(long resourceID, float amount)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->setTotalResource(resourceID, amount);
      return;
   }
   mTotalResources.set(resourceID, amount); 
}

//==============================================================================
//==============================================================================
void BPlayer::setTotalResources(const BCost* pResources)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->setTotalResources(pResources);
      return;
   }
   mTotalResources.set(pResources);
}

//==============================================================================
//==============================================================================
const BCost& BPlayer::getTotalResources() const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->getTotalResources();
   return (mTotalResources);
}

//==============================================================================
//==============================================================================
void BPlayer::addTotalResource(long resourceID, float amount)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
   {
      gWorld->getPlayer(mCoopID)->addTotalResource(resourceID, amount);
      return;
   }
   mTotalResources.add(resourceID, amount); 
}

//==============================================================================
// BPlayer::checkAvail
//==============================================================================
bool BPlayer::checkAvail(const BCost*  pCost) const
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->checkAvail(pCost);
   for (int i=0; i<mResources.getNumberResources(); i++)
   {
      if (!gDatabase.getResourceDeductable(i))
      {
         if (mResources.get(i) < pCost->get(i))
            return false;
      }
   }
   return true;
}

//==============================================================================
// BPlayer::checkTotal
//==============================================================================
bool BPlayer::checkTotal(const BCost* pCost) const
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedResources))
      return gWorld->getPlayer(mCoopID)->checkTotal(pCost);
   return(mTotalResources.isAtLeast(*pCost));
}

//==============================================================================
//==============================================================================
float BPlayer::getPopCount(long id) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->getPopCount(id);

   if(id<0||id>mPopCount) 
      return 0.0f; 
   else 
      return mpPops[id].mCount;
}

//==============================================================================
//==============================================================================
void BPlayer::setPopCount(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->setPopCount(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mCount=count;
}

//==============================================================================
//==============================================================================
void BPlayer::adjustPopCount(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->adjustPopCount(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mCount=mpPops[id].mCount+count;
}

//==============================================================================
//==============================================================================
float BPlayer::getPopCap(long id) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->getPopCap(id);

   if(id<0||id>mPopCount) 
      return 0.0f; 
   else 
      return mpPops[id].mCap;
}

//==============================================================================
//==============================================================================
void BPlayer::setPopCap(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->setPopCap(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mCap=count;
}

//==============================================================================
//==============================================================================
void BPlayer::adjustPopCap(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->adjustPopCap(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mCap=mpPops[id].mCap+count;
}

//==============================================================================
//==============================================================================
float BPlayer::getPopMax(long id) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->getPopMax(id);

   if(id<0||id>mPopCount) 
      return 0.0f; 
   else 
      return mpPops[id].mMax;
}

//==============================================================================
//==============================================================================
void BPlayer::setPopMax(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->setPopMax(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mMax=count;
}

//==============================================================================
//==============================================================================
void BPlayer::adjustPopMax(long id, float count)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->adjustPopMax(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mMax=mpPops[id].mMax+count;
}

//==============================================================================
//==============================================================================
float BPlayer::getPopFuture(long id) const
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->getPopFuture(id);

   if(id<0||id>mPopCount) 
      return 0.0f; 
   else 
      return mpPops[id].mFuture;
}

//==============================================================================
//==============================================================================
void BPlayer::setPopFuture(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->setPopFuture(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mFuture=count;
}

//==============================================================================
//==============================================================================
void BPlayer::adjustPopFuture(long id, float count)
{ 
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->adjustPopFuture(id, count);
      return;
   }

   if(id<0||id>mPopCount) 
      return; 
   else 
      mpPops[id].mFuture=mpPops[id].mFuture+count;
}

//==============================================================================
// BPlayer::payPopFuture
//==============================================================================
void BPlayer::payPopFuture(const BPopArray* pPops)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->payPopFuture(pPops);
      return;
   }

   if(!pPops)
      return;
   long count=pPops->getNumber();
   for(long i=0; i<count; i++)
   {
      BPop pop=pPops->get(i);
      adjustPopFuture(pop.mID, pop.mCount);
   }
}

//==============================================================================
// BPlayer::refundPopFuture
//==============================================================================
void BPlayer::refundPopFuture(const BPopArray* pPops)
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
   {
      gWorld->getPlayer(mCoopID)->refundPopFuture(pPops);
      return;
   }

   if(!pPops)
      return;
   long count=pPops->getNumber();
   for(long i=0; i<count; i++)
   {
      BPop pop=pPops->get(i);
      adjustPopFuture(pop.mID, -pop.mCount);
   }
}

//==============================================================================
// BPlayer::checkPop
//==============================================================================
bool BPlayer::checkPop(long id, float count) const
{
   // [10/29/2008 xemu] apply rounding
   count = floorf(count + 0.5f);

   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->checkPop(id, count);

   if(id<0||id>mPopCount)
      return false;
   
   BPlayerPop pop=mpPops[id];

   long total=(long)(count+pop.mCount+pop.mFuture);

   return (total<=(long)pop.mCap && total<=(long)pop.mMax);
}

//==============================================================================
// BPlayer::checkPops
//==============================================================================
bool BPlayer::checkPops(const BPopArray* pPops) const
{
   if (mCoopID!=-1 && gConfig.isDefined(cConfigCoopSharedPop))
      return gWorld->getPlayer(mCoopID)->checkPops(pPops);

   if (!pPops)
      return (true);
   uint count = (uint)pPops->getNumber();
   for(uint i = 0; i < count; i++)
   {
      BPop pop = pPops->get(i);
      if (!checkPop(pop.mID, pop.mCount))
         return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
BVector BPlayer::getRallyPoint() const
{
   // Rally point attached to an entity
   if (mRallyPoint.mEntityID != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 6107
      const BEntity* pEntity = gWorld->getEntity(mRallyPoint.mEntityID);
//--
      if (pEntity)
         return (pEntity->getPosition());
   }

   BVector rallyPointPos(mRallyPoint.x, mRallyPoint.y, mRallyPoint.z);
   return rallyPointPos;
}

//==============================================================================
//==============================================================================
void BPlayer::getRallyPoint(BVector& rallyPoint, BEntityID& rallyPointEntityID)
{
   rallyPoint.set(mRallyPoint.x, mRallyPoint.y, mRallyPoint.z);
   rallyPointEntityID = mRallyPoint.mEntityID;
}

//==============================================================================
// BPlayer::setRallyPoint
//==============================================================================
void BPlayer::setRallyPoint(BVector point, BEntityID entityID)
{
   mRallyPoint.mEntityID = cInvalidObjectID;

   if (entityID != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 6109
      const BEntity* pEntity = gWorld->getEntity(entityID);
//--
      if (pEntity)
      {
         mRallyPoint.mEntityID = entityID;
         point = pEntity->getPosition();
      }
   }

   mRallyPoint.x = point.x;
   mRallyPoint.y = point.y;
   mRallyPoint.z = point.z;

   setFlagRallyPoint(true);
   BSquad* pSquad=gWorld->getSquad(mRallyObject);
   if (pSquad)
   {
      pSquad->setPosition(point);
      pSquad->settle();
   }
   else
   {
      BObject* pObject=gWorld->getObject(mRallyObject);
      if (pObject)
      {
         pObject->setPosition(point);
      }
      else
      {
         long protoID=getCiv()->getRallyPointObjectID();
         if(protoID!=-1)
            mRallyObject = gWorld->createEntity(protoID, false, mID, point, cZAxisVector, cXAxisVector, true);
      }
   }
   // Remove any building specific rally points
   uint numBases = getNumberGotoBases();
   if (numBases > 0)
   {
      for (uint i=0; i<numBases; i++)
      {
         BUnit* pBase = gWorld->getUnit(getGotoBase(i));
         if (pBase && pBase->getFlagRallyPoint())
            pBase->clearRallyPoint(getID());
      }
   }

   // If in coop - get the coop player's bases too, since you have rally flags associated with them
   if (mCoopID != -1)
   {
      BPlayer* pCoopPlayer = gWorld->getPlayer(mCoopID);
      numBases = pCoopPlayer->getNumberGotoBases();
      if (numBases > 0)
      {
         for (uint i=0; i<numBases; i++)
         {
            BUnit* pBase = gWorld->getUnit(pCoopPlayer->getGotoBase(i));
            if (pBase && pBase->getFlagRallyPoint2())
               pBase->clearRallyPoint(getID());
         }
      }
   }
}

//==============================================================================
// BPlayer::clearRallyPoint
//==============================================================================
void BPlayer::clearRallyPoint()
{
   mRallyPoint.init();
   setFlagRallyPoint(false);
   if(mRallyObject!=cInvalidObjectID)
   {
      BSquad* pSquad=gWorld->getSquad(mRallyObject);
      if (pSquad)
         pSquad->kill(true);
      else
      {
         BObject* pObject=gWorld->getObject(mRallyObject);
         if (pObject)
            pObject->kill(true);
      }
      mRallyObject=cInvalidObjectID;
   }
}

//==============================================================================
// BPlayer::setRallyPointVisible
//==============================================================================
void BPlayer::setRallyPointVisible(bool val)
{
   if(mRallyObject!=cInvalidObjectID)
   {
      BObject* pObject=NULL;
      BSquad* pSquad=gWorld->getSquad(mRallyObject);
      if (pSquad)
         pObject=pSquad->getLeaderUnit();
      else
         pObject=gWorld->getObject(mRallyObject);
      if (pObject)
         pObject->setFlagNoRender(!val);
   }
}

//==============================================================================
//==============================================================================
void BPlayer::checkRallyPoint()
{
   if (getFlagRallyPoint())
   {
      uint numBases = getNumberGotoBases();
      if (numBases > 0)
      {
         bool allHaveRally = true;
         for (uint i=0; i<numBases; i++)
         {
//-- FIXING PREFIX BUG ID 6110
            const BUnit* pBase = gWorld->getUnit(getGotoBase(i));
//--
            if (pBase && !pBase->getFlagRallyPoint())
            {
               allHaveRally = false;
               break;
            }
         }
         if (allHaveRally)
            clearRallyPoint();
      }
   }
}

//==============================================================================
// BPlayer::isEnemy
//==============================================================================
bool BPlayer::isEnemy(long playerID) const
{
//-- FIXING PREFIX BUG ID 6111
   const BPlayer* pPlayer=gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return false;
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeEnemy)
      return true;
   return false;
}

//==============================================================================
// BPlayer::isEnemy
//==============================================================================
bool BPlayer::isEnemy(const BPlayer* pPlayer) const
{
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeEnemy)
      return true;
   return false;
}

//==============================================================================
// BPlayer::isAlly
//==============================================================================
bool BPlayer::isAlly(long playerID, bool selfAsAlly) const
{
   if (!selfAsAlly && playerID==mID)
      return false;
//-- FIXING PREFIX BUG ID 6112
   const BPlayer* pPlayer=gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return false;
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeAlly)
      return true;
   return false;
}

//==============================================================================
// BPlayer::isAlly
//==============================================================================
bool BPlayer::isAlly(const BPlayer* pPlayer, bool selfAsAlly) const
{
   if (!selfAsAlly && pPlayer==this)
      return false;
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeAlly)
      return true;
   return false;
}

//==============================================================================
// BPlayer::isSelf
//==============================================================================
bool BPlayer::isSelf(long playerID) const
{
   return (playerID == mID);
}

//==============================================================================
// BPlayer::isSelf
//==============================================================================
bool BPlayer::isSelf(const BPlayer* pPlayer) const
{
   return (pPlayer == this);
}

//==============================================================================
// BPlayer::isNeutral
//==============================================================================
bool BPlayer::isNeutral(long playerID) const
{
//-- FIXING PREFIX BUG ID 6113
   const BPlayer* pPlayer=gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return false;
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeNeutral)
      return true;
   return false;
}

//==============================================================================
// BPlayer::isNeutral
//==============================================================================
bool BPlayer::isNeutral(const BPlayer* pPlayer) const
{
   if(gWorld->getTeamRelationType(mTeamID, pPlayer->getTeamID()) == cRelationTypeNeutral)
      return true;
   return false;
}

//==============================================================================
//==============================================================================
BTeam* BPlayer::getTeam()
{
   return gWorld->getTeam(mTeamID);
}

//==============================================================================
//==============================================================================
const BTeam* BPlayer::getTeam() const
{
   return gWorld->getTeam(mTeamID);
}

//==============================================================================
//==============================================================================
void BPlayer::setTeamID(BTeamID id)
{
   mTeamID = id;

   if (mpStats)
      mpStats->setTeamID(mTeamID);
}


//=============================================================================
// BPlayer::canUsePower
//=============================================================================
bool BPlayer::canUsePower(BProtoPowerID protoPowerID, BEntityID squadID) const
{
   // If not human and not CPAI then most likely a SPC player and don't need to bother checking for power costs
   if (!isHuman() && !isComputerAI())
      return (true);

   const BPowerEntry* pPowerEntry = findPowerEntry(protoPowerID);
   if (!pPowerEntry)
      return (false);

   if (!pPowerEntry->mFlagIgnoreCost && !canAffordToCastPower(protoPowerID))
      return(false);
   if (!hasAvailablePowerEntryUses(protoPowerID))
      return(false);
   if (!pPowerEntry->mFlagIgnoreTechPrereqs && !passesAllPowerTechPrereqs(protoPowerID))
      return(false);
   if (!pPowerEntry->mFlagIgnorePop && !passesPop(protoPowerID))
      return (false);

//-- FIXING PREFIX BUG ID 6115
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(pPowerEntry->mProtoPowerID);
//--
   if (!pProtoPower)
      return (false);

   if (mFlagLeaderPowersBlocked)
   {
      if (pProtoPower->getFlagLeaderPower())
         return (false);
   }

   if (squadID != cInvalidObjectID && pProtoPower->getFlagUnitPower())
   {
      int count=pPowerEntry->mItems.getNumber();
      for (int i=0; i<count; i++)
      {
//-- FIXING PREFIX BUG ID 6114
         const BPowerEntryItem& item=pPowerEntry->mItems[i];
//--
         if (item.mSquadID==squadID)
         {
            if (item.mUsesRemaining>0)
               return (true);
            else
               return (false);
         }
      }
   }

   return (true);
}


//==============================================================================
// BPlayer::canAffordToCastPower
//==============================================================================
bool BPlayer::canAffordToCastPower(BProtoPowerID protoPowerID) const
{
   if (!findPowerEntry(protoPowerID))
      return(false);
   const BProtoPower * const pPP = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pPP)
      return(false);
   return(checkCost(pPP->getCost()));
}

//==============================================================================
// BPlayer::hasAvailablePowerEntryUses
//==============================================================================
bool BPlayer::hasAvailablePowerEntryUses(BProtoPowerID protoPowerID) const
{
   const BPowerEntry * const pEntry = findPowerEntry(protoPowerID);
   if (!pEntry)
      return(false);

   long numPowerEntryItems = pEntry->mItems.getNumber();
   for(long i=0; i<numPowerEntryItems; i++)
   {
//-- FIXING PREFIX BUG ID 6116
      const BPowerEntryItem& item = pEntry->mItems[i];
//--
      //if(item.mUnitID == unitID)
      //{
      if (item.mInfiniteUses || item.mUsesRemaining > 0)
         return(true);
      //}
   }

   return(false);
}

//==============================================================================
// BPlayer::passesAllPowerTechPrereqs
//==============================================================================
bool BPlayer::passesAllPowerTechPrereqs(BProtoPowerID protoPowerID) const
{
   const BProtoPower* const pPP = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pPP)
      return(false);
   const BTechTree* const pTechTree = getTechTree();
   if (!pTechTree)
      return(false);

   long numTechPrereqs = pPP->getNumberTechPrereqs();
   for (long i=0; i<numTechPrereqs; i++)
   {
      long techStatus = pTechTree->getTechStatus(pPP->getTechPrereq(i), -1);
      if (techStatus != BTechTree::cStatusActive)
         return(false);
   }

   return(true);
}

//==============================================================================
// Check to see if the power passes the pop requirements
//==============================================================================
bool BPlayer::passesPop(BProtoPowerID protoPowerID) const
{
   if (!findPowerEntry(protoPowerID))
   {
      return (false);
   }

//-- FIXING PREFIX BUG ID 6117
   const BProtoPower* const pPP = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (!pPP)
   {
      return (false);
   }

   const BPopArray* pPops = pPP->getPop();
   return (checkPops(pPops));
}

//==============================================================================
// BPlayer::addPowerEntry
//==============================================================================
bool BPlayer::addPowerEntry(BProtoPowerID protoPowerID, BEntityID squadID, long uses, int iconLocation, bool ignoreCost /*= false*/, bool ignoreTechPrereqs /*= false*/, bool ignorePop /*= false*/)
{
//-- FIXING PREFIX BUG ID 6120
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (!pProtoPower)
      return (false);
   bool infiniteUses = pProtoPower->getFlagInfiniteUses();
   bool sequentialRecharge = pProtoPower->getFlagSequentialRecharge();

   BPowerEntry* pEntry = findPowerEntry(protoPowerID);
   if (!pEntry)
   {
      BPowerEntry entryToAdd;
      entryToAdd.mProtoPowerID = protoPowerID;
      entryToAdd.mIconLocation = iconLocation;
      entryToAdd.mFlagIgnoreCost = ignoreCost;
      entryToAdd.mFlagIgnoreTechPrereqs = ignoreTechPrereqs;
      entryToAdd.mFlagIgnorePop = ignorePop;
      long index = mPowerEntries.add(entryToAdd);
      if (index == -1)
         return (false);
      pEntry = &(mPowerEntries[index]);
   }

   if (pProtoPower->getFlagMultiRechargePower())
   {
      for (int i=0; i<uses; i++)
      {
         BPowerEntryItem item;
         item.mUsesRemaining = 1;
         long index = pEntry->mItems.add(item);
         if (index == -1)
            return (false);
      }
   }
   else
   {
      BPowerEntryItem* pItem = NULL;
      uint numItems = pEntry->mItems.getSize();
      for (uint i = 0; i < numItems; i++)
      {
         BPowerEntryItem& item = pEntry->mItems[i];
         if (item.mSquadID == squadID)
         {
            pItem = &item;
            break;
         }
      }
      if (!pItem)
      {
         BPowerEntryItem item;
         item.mSquadID = squadID;
         long index = pEntry->mItems.add(item);
         if (index == -1)
            return (false);
         pItem = &(pEntry->mItems[index]);
      }
      if (sequentialRecharge)
         pItem->mChargeCap += uses;
      else
         pItem->mChargeCap = 1;
      if (!pItem->mInfiniteUses && infiniteUses)
         pItem->mInfiniteUses = true;
      if (!pItem->mInfiniteUses && !sequentialRecharge)
         pItem->mUsesRemaining += uses;
      if (pItem->mRecharging && (pItem->mInfiniteUses || (pItem->mUsesRemaining >= pItem->mChargeCap)))
         pItem->mRecharging = false;
   }

   if (mpStats)
      mpStats->addLeaderPower(protoPowerID);

   return (true);
}

//==============================================================================
// BPlayer::removePowerEntry
//==============================================================================
void BPlayer::removePowerEntry(BProtoPowerID protoPowerID, BEntityID squadID)
{
   for(uint i = 0; i < mPowerEntries.getSize(); i++)
   {
      BPowerEntry& entry = mPowerEntries[i];
      if ((protoPowerID != -1) && (entry.mProtoPowerID == protoPowerID))
      {
//-- FIXING PREFIX BUG ID 6123
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
         if (pProtoPower && pProtoPower->getFlagMultiRechargePower())
         {
            // Remove the item that is longest away from being recharged
            uint itemIndex=UINT_MAX;
            bool itemRecharging=false;
            DWORD itemNextGrantTime=0;
            for (uint j = 0; j < entry.mItems.getSize(); j++)
            {
//-- FIXING PREFIX BUG ID 6121
               const BPowerEntryItem& item = entry.mItems[j];
//--
               if (itemIndex==UINT_MAX || (!itemRecharging && item.mRecharging) || (item.mRecharging && item.mNextGrantTime > itemNextGrantTime))
               {
                  itemIndex=j;
                  itemRecharging=item.mRecharging;
                  itemNextGrantTime=item.mNextGrantTime;
               }
            }
            if (itemIndex!=UINT_MAX)
               entry.mItems.removeIndex(itemIndex);
         }
         else if (squadID == cInvalidObjectID)
            mPowerEntries.removeIndex(i);
         else
         {
            for (uint j = 0; j < entry.mItems.getSize(); j++)
            {
//-- FIXING PREFIX BUG ID 6122
               const BPowerEntryItem& item = entry.mItems[j];
//--
               if (item.mSquadID == squadID)
               {
                  entry.mItems.removeIndex(j);
                  return;
               }
            }
         }
         return;
      }
      else if ((protoPowerID == -1) && (squadID != cInvalidObjectID))
      {
         for (uint j = 0; j < entry.mItems.getSize(); j++)
         {
//-- FIXING PREFIX BUG ID 6124
            const BPowerEntryItem& item = entry.mItems[j];
//--
            if (item.mSquadID == squadID)
            {
               entry.mItems.removeIndex(j);
               return;
            }            
         }
      }
   }
}

//==============================================================================
// Remove the power entry with the matching icon location
//==============================================================================
void BPlayer::removePowerEntryAtIconLocation(int iconLocation)
{
   if (iconLocation == -1)
   {
      return;
   }

   uint numPowerEntries = mPowerEntries.getSize();
   for (uint i = 0; i < numPowerEntries; i++)
   {
//-- FIXING PREFIX BUG ID 6126
      const BPowerEntry& powerEntry = mPowerEntries[i];
//--
      if (powerEntry.mIconLocation == -1)
      {
//-- FIXING PREFIX BUG ID 6125
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(powerEntry.mProtoPowerID);
//--
         if (pProtoPower)
         {
            uint numIconLocations = (uint)pProtoPower->getNumberIconLocations();
            for (uint j = 0; j < numIconLocations; j++)
            {
               if (pProtoPower->getIconLocation(j) == iconLocation)
               {
                  mPowerEntries.removeIndex(i);
                  return;
               }
            }            
         }
      }
      else if (powerEntry.mIconLocation == iconLocation)
      {
         mPowerEntries.removeIndex(i);
         return;
      }      
   }
}

//==============================================================================
// BPlayer::findPowerEntry
//==============================================================================
BPowerEntry* BPlayer::findPowerEntry(BProtoPowerID protoPowerID)
{
   // Brute force search.
   for(uint i=0; i<mPowerEntries.getSize(); i++)
   {
      if(mPowerEntries[i].mProtoPowerID == protoPowerID)
         return(&mPowerEntries[i]);
   }

   // Nothing found.
   return(NULL);
}

//==============================================================================
// BPlayer::findPowerEntry
//==============================================================================
const BPowerEntry* BPlayer::findPowerEntry(BProtoPowerID protoPowerID) const
{
   // Brute force search.
   for(uint i=0; i<mPowerEntries.getSize(); i++)
   {
      if(mPowerEntries[i].mProtoPowerID == protoPowerID)
         return(&mPowerEntries[i]);
   }

   // Nothing found.
   return(NULL);
}

//==============================================================================
// BPlayer::usePower
//==============================================================================
bool BPlayer::usePower(BProtoPowerID protoPowerID, BEntityID squadID, BEntityID targetID, float costMultiplier /*= 1.0f*/, bool ignoreRequirements /*= false*/)
{
   if (ignoreRequirements)
   {
      // just fire the event and bail
      gGeneralEventManager.eventTrigger(BEventDefinitions::cUsedPower, mID, squadID, protoPowerID);
      return true;
   }

   if (!canUsePower(protoPowerID))
   {
      #ifdef SYNC_Command
         syncCommandCode("BPlayer::usePower !canUsePower");
      #endif
      return (false);
   }
   BPowerEntry*  pEntry = findPowerEntry(protoPowerID);
   if (!pEntry)
   {
      #ifdef SYNC_Command
         syncCommandCode("BPlayer::usePower !pEntry");
      #endif
      return (false);
   }
   BProtoPower* pPP = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pPP)
   {
      #ifdef SYNC_Command
         syncCommandCode("BPlayer::usePower !pPP");
      #endif
      return (false);
   }

   const BProtoObject* pTargetProto = NULL;
   if (targetID != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 6127
      const BSquad* pTarget = gWorld->getSquad(targetID);
//--
      if (pTarget)
         pTargetProto = pTarget->getProtoObject();
   }

   uint numPowerEntryItems = pEntry->mItems.getSize();
   for (uint i = 0; i < numPowerEntryItems; i++)
   {
      BPowerEntryItem& item = pEntry->mItems[i];
      bool foundItem = false;
      if (pPP->getFlagMultiRechargePower())
      {
         if (item.mUsesRemaining > 0)
            foundItem = true;
      }
      else if (item.mSquadID == squadID)
         foundItem=true;
      if (foundItem)
      {
         if (!item.mInfiniteUses)
         {
            if (item.mUsesRemaining <= 0)
            {
               #ifdef SYNC_Command
                  syncCommandCode("BPlayer::usePower no uses remaining");
               #endif
               return (false);
            }
            item.mUsesRemaining--;
            item.mTimesUsed++;

            DWORD rechargeTime = getPowerRechargeTime(protoPowerID);
            if (!item.mRecharging && (item.mUsesRemaining < item.mChargeCap) && (rechargeTime > 0))
            {
               item.mNextGrantTime = gWorld->getGametime() + rechargeTime;
               item.mRecharging = true;
            }
         }

         // Pay the cost.
         if (!pEntry->mFlagIgnoreCost)
         {
            BCost cost = *pPP->getCost(pTargetProto);
            cost *= costMultiplier;
            #ifdef SYNC_Command
               for (int i=0; i<mResources.getNumberResources(); i++)
               {
                  syncCommandData("BPlayer::usePower cost", cost.get(i));
               }
            #endif
            payCost(&cost);
         }

         pEntry->mTimesUsed++;

         if (mpStats)
            mpStats->usedLeaderPower(protoPowerID);
         MVinceEventAsync_UsedLeaderPower( pPP, getID() );

         gGeneralEventManager.eventTrigger(BEventDefinitions::cUsedPower, mID, squadID, protoPowerID);

         return (true);
      }
   }

   return (false);
}

//==============================================================================
// BPlayer::usePower
//==============================================================================
bool BPlayer::restartPowerRecharge(BProtoPowerID protoPowerID, BEntityID squadID)
{
   BPowerEntry*  pEntry = findPowerEntry(protoPowerID);
   if (!pEntry)
   {
#ifdef SYNC_Command
      syncCommandCode("BPlayer::usePower !pEntry");
#endif
      return (false);
   }
   BProtoPower* pPP = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pPP)
   {
#ifdef SYNC_Command
      syncCommandCode("BPlayer::usePower !pPP");
#endif
      return (false);
   }

   uint numPowerEntryItems = pEntry->mItems.getSize();
   for (uint i = 0; i < numPowerEntryItems; i++)
   {
      BPowerEntryItem& item = pEntry->mItems[i];
      bool foundItem = false;
      if (pPP->getFlagMultiRechargePower())
      {
         if (item.mUsesRemaining > 0)
            foundItem = true;
      }
      else if (item.mSquadID == squadID)
         foundItem=true;
      if (foundItem)
      {
         if (!item.mInfiniteUses)
         {
            // force into a recharge state
            DWORD rechargeTime = getPowerRechargeTime(protoPowerID);
            if (rechargeTime > 0)
            {
               item.mUsesRemaining = Math::Min(item.mUsesRemaining, item.mChargeCap - 1);
               item.mNextGrantTime = gWorld->getGametime() + rechargeTime;
               item.mRecharging = true;
            }
         }
         return (true);
      }
   }

   return (false);
}

//==============================================================================
// BPlayer::updatePowerRecharge
//==============================================================================
void BPlayer::updatePowerRecharge(void)
{
   DWORD currentTime = gWorld->getGametime();

   bool quickBuild=gWorld->getFlagQuickBuild();

   for(uint i=0; i<mPowerEntries.getSize(); i++)
   {
      BPowerEntry& entry=mPowerEntries[i];
      for(uint j=0; j<entry.mItems.getSize(); j++)
      {
         BPowerEntryItem& item=entry.mItems[j];

         // Check if auto recharge needed
         DWORD rechargeTime = getPowerRechargeTime(entry.mProtoPowerID);
         if (!item.mRecharging && (item.mUsesRemaining < item.mChargeCap) && (rechargeTime > 0))
         {
            item.mNextGrantTime = gWorld->getGametime() + rechargeTime;
            item.mRecharging = true;
         }

         if(item.mRecharging)
         {
            int useLimit = getPowerUseLimit(entry.mProtoPowerID);
            if (useLimit==0 || item.mTimesUsed<useLimit)
            {
               if(quickBuild || currentTime>=item.mNextGrantTime)
               {
                  item.mUsesRemaining++;
                  item.mRecharging=false;
               }
               gWorld->notify(BEntity::cEventPowerPercent, cInvalidObjectID, entry.mProtoPowerID, mID);
            }
         }
      }
   }
}

//==============================================================================
// BPlayer::getSupportPower
//==============================================================================
const BLeaderSupportPower* BPlayer::getSupportPower(uint index) const
{
   if (index < getLeader()->getSupportPowers().getSize())
   {
      const BLeaderSupportPowerArray& supportPowers = getLeader()->getSupportPowers();
      return (&supportPowers[index]);
   }
   else
      return (NULL);
}

//==============================================================================
// BPlayer::setSupportPowerStatus
//==============================================================================
void BPlayer::setSupportPowerStatus(uint index, BYTE status)
{ 
   mSupportPowerStatus[index]=status;
   gWorld->notify(BEntity::cEventPowerPercent, cInvalidObjectID, static_cast<DWORD>(-1), static_cast<DWORD>(mID));
}

//==============================================================================
// BPlayer::getBuildingProtoIDToTrainSquad
//==============================================================================
BProtoObjectID BPlayer::getBuildingProtoIDToTrainSquad(BProtoSquadID protoSquadID, bool requireInstance) const
{
   // Returns the protoID of the building that can train the squad type specified by the passed in protoSquadID.
   // If none found, it returns cInvalidProtoObjectID
   BProtoObjectID buildingID = cInvalidProtoObjectID;
   if (!getProtoSquad(protoSquadID)) // catch the bogus case.
      return (buildingID);

   int numProto = getNumberProtoObjects();
   for (int protoID = 0; protoID < numProto; protoID++)
   {
      if (requireInstance && this->getNumUnitsOfType(protoID) == 0)
         continue;

//-- FIXING PREFIX BUG ID 6128
      const BProtoObject* protoObj = getProtoObject(protoID);
//--
      uint numCommands = protoObj->getNumberCommands();
      for (uint commandIndex = 0; commandIndex < numCommands; commandIndex++)
      {
         BProtoObjectCommand command = protoObj->getCommand(commandIndex);
         if (command.getType() == BProtoObjectCommand::cTypeTrainSquad)
         {
            if (command.getID() == protoSquadID)
            {
               buildingID = protoID;
               return buildingID;
            }
         }
      }
   }

   return buildingID;
}

//==============================================================================
// BPlayer::getBuildingProtoIDArrayToTrainSquad
//==============================================================================
BProtoObjectIDArray BPlayer::getBuildingProtoIDArrayToTrainSquad(BProtoSquadID protoSquadID, bool requireInstance) const
{
   // Returns the protoID of the building that can train the squad type specified by the passed in protoSquadID.
   // If none found, it returns empty array
   BProtoObjectIDArray buildingIDArray;
   if (!getProtoSquad(protoSquadID)) // catch the bogus case.
      return (buildingIDArray);

   int numProto = getNumberProtoObjects();
   for (int protoID = 0; protoID < numProto; protoID++)
   {
      if (requireInstance && this->getNumUnitsOfType(protoID) == 0)
         continue;

//-- FIXING PREFIX BUG ID 6129
      const BProtoObject* protoObj = getProtoObject(protoID);
//--
      uint numCommands = protoObj->getNumberCommands();
      for (uint commandIndex = 0; commandIndex < numCommands; commandIndex++)
      {
         BProtoObjectCommand command = protoObj->getCommand(commandIndex);
         if (command.getType() == BProtoObjectCommand::cTypeTrainSquad)
         {
            if (command.getID() == protoSquadID)
            {
               buildingIDArray.add(protoID);
            }
         }
      }
   }

   return buildingIDArray;
}

 

//==============================================================================
// BPlayer::getBuildingProtoIDToResearchTech
//==============================================================================
BProtoObjectID BPlayer::getBuildingProtoIDToResearchTech(BProtoTechID protoTechID, bool requireInstance) const
{
   // Returns the protoID of the building that can research the tech type specified by the passed in techProtoID.
   // If none found, it returns cInvalidProtoObjectID
   BProtoObjectID buildingID = cInvalidProtoObjectID;
   if (!getProtoTech(protoTechID)) // catch the bogus case.
      return (buildingID);

   int numProto = getNumberProtoObjects();
   for (int protoID = 0; protoID < numProto; protoID++)
   {
      if (requireInstance && this->getNumUnitsOfType(protoID) == 0)
         continue;

//-- FIXING PREFIX BUG ID 6130
      const BProtoObject* protoObj = getProtoObject(protoID);
//--
      uint numCommands = protoObj->getNumberCommands();
      for (uint commandIndex = 0; commandIndex < numCommands; commandIndex++)
      {
         BProtoObjectCommand command = protoObj->getCommand(commandIndex);
         if (command.getType() == BProtoObjectCommand::cTypeResearch)
         {
            if (command.getID() == protoTechID)
            {
               buildingID = protoID;
               return buildingID;
            }
         }
      }
   }

   return buildingID;
}

//==============================================================================
// BPlayer::getBuildingProtoIDToBuildStructure
//==============================================================================
BProtoObjectID BPlayer::getBuildingProtoIDToBuildStructure(BProtoObjectID protoObjectID, bool requireInstance) const
{
   // Returns the protoID of the building that can build the structure type specified by the passed in techProtoID.
   // If none found, it returns cInvalidProtoObjectID
   BProtoObjectID buildingID = cInvalidProtoObjectID;
   if (!getProtoObject(protoObjectID)) // catch the bogus case.
      return (buildingID);

   int numProto = getNumberProtoObjects();
   for (int protoID = 0; protoID < numProto; protoID++)
   {
//-- FIXING PREFIX BUG ID 6131
      const BProtoObject* protoObj = getProtoObject(protoID);
//--

      if (requireInstance && this->getNumUnitsOfType(protoID) == 0)
      {
         if(protoObj->getFlagCommandableByAnyPlayer() == true)
         {
            BString name = protoObj->getName();
         }
         else
         {
            continue;         
         }
      }

      uint numCommands = protoObj->getNumberCommands();
      for (uint commandIndex = 0; commandIndex < numCommands; commandIndex++)
      {
         BProtoObjectCommand command = protoObj->getCommand(commandIndex);
         if (command.getType() == BProtoObjectCommand::cTypeBuild)
         {
            if (command.getID() == protoObjectID)
            {
               buildingID = protoID;
               return buildingID;
            }
         }
         if (command.getType() == BProtoObjectCommand::cTypeBuildOther)
         {
            if (command.getID() == protoObjectID)
            {
               buildingID = protoID;
               return buildingID;
            }
         }
      }
   }

   return buildingID;
}


//==============================================================================
//==============================================================================
uint BPlayer::getCurrentlyLegalSquadsToTrain(BProtoSquadIDArray& legalSquads) const
{
   // Clear our results.
   legalSquads.resize(0);

   BObjectTypeID buildingObjectType = gDatabase.getOTIDBuilding();
   BProtoObjectIDArray *buildingProtoObjectIDs = NULL;
   uint numBuildingProtoObjectIDs = gDatabase.decomposeObjectType(buildingObjectType, &buildingProtoObjectIDs);
   if (!buildingProtoObjectIDs)
      return 0;

   for (uint i=0; i<numBuildingProtoObjectIDs; i++)
   {
      BProtoObjectID buildingProtoObjectID = buildingProtoObjectIDs->get(i);
      uint numBuildingsOfType = getNumUnitsOfType(buildingProtoObjectID);
      if (numBuildingsOfType == 0)
         continue;
      const BProtoObject* pProtoObject = getProtoObject(buildingProtoObjectID);
      if (!pProtoObject)
         continue;

      uint numCommands = pProtoObject->getNumberCommands();
      for (uint commandIndex=0; commandIndex<numCommands; commandIndex++)
      {
         if (!pProtoObject->getCommandAvailable(commandIndex))
            continue;
         BProtoObjectCommand command = pProtoObject->getCommand(commandIndex);
         if (command.getType() != BProtoObjectCommand::cTypeTrainSquad)
            continue;
         BProtoSquadID protoSquadID = command.getID();
         const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
         if (!pProtoSquad)
            continue;
         if (pProtoSquad->getFlagForbid())
            continue;
         if (!pProtoSquad->getFlagAvailable())
            continue;

         legalSquads.add(protoSquadID);
      }
   }

   return (legalSquads.getSize());
}


//==============================================================================
//==============================================================================
uint BPlayer::getCurrentlyLegalTechsToResearch(BTechIDArray& legalTechs) const
{
   // Clear our results.
   legalTechs.resize(0);

   BObjectTypeID buildingObjectType = gDatabase.getOTIDBuilding();
   BProtoObjectIDArray *buildingProtoObjectIDs = NULL;
   uint numBuildingProtoObjectIDs = gDatabase.decomposeObjectType(buildingObjectType, &buildingProtoObjectIDs);
   if (!buildingProtoObjectIDs)
      return 0;

   for (uint i=0; i<numBuildingProtoObjectIDs; i++)
   {
      BProtoObjectID buildingProtoObjectID = buildingProtoObjectIDs->get(i);
      uint numBuildingsOfType = getNumUnitsOfType(buildingProtoObjectID);
      if (numBuildingsOfType == 0)
         continue;
      const BProtoObject* pProtoObject = getProtoObject(buildingProtoObjectID);
      if (!pProtoObject)
         continue;

      uint numCommands = pProtoObject->getNumberCommands();
      for (uint commandIndex=0; commandIndex<numCommands; commandIndex++)
      {
         if (!pProtoObject->getCommandAvailable(commandIndex))
            continue;
         BProtoObjectCommand command = pProtoObject->getCommand(commandIndex);
         if (command.getType() != BProtoObjectCommand::cTypeResearch)
            continue;
         BProtoTechID protoTechID = command.getID();
         const BProtoTech* pProtoTech = getProtoTech(protoTechID);
         if (!pProtoTech)
            continue;
         if (pProtoTech->getFlagForbid())
            continue;
         //if (!pProtoTech->getFlagAvailable())
         //   continue;

         legalTechs.add(protoTechID);
      }
   }

   return (legalTechs.getSize());
}


//==============================================================================
//==============================================================================
uint BPlayer::getCurrentlyLegalBuildingsToConstruct(BProtoObjectIDArray& legalBuildings) const
{
   // Clear our results.
   legalBuildings.resize(0);

   BObjectTypeID buildingObjectType = gDatabase.getOTIDBuilding();
   BObjectTypeID buildingSocketObjectType = gDatabase.getOTIDBuildingSocket();
   BObjectTypeID turretSocketObjectType = gDatabase.getOTIDTurretSocket();
   static BProtoObjectIDArray buildingProtoObjectIDs;
   buildingProtoObjectIDs.resize(0);
   BProtoObjectIDArray *tempBuildingProtoObjectIDs;
   BProtoObjectIDArray *buildingSocketObjectIDs = NULL;
   BProtoObjectIDArray *turretSocketObjectIDs = NULL;
   uint numBuildingProtoObjectIDs = gDatabase.decomposeObjectType(buildingObjectType, &tempBuildingProtoObjectIDs);
   uint numBuildingSocketProtoObjectIDs = gDatabase.decomposeObjectType(buildingSocketObjectType, &buildingSocketObjectIDs);
   uint numTurretSocketProtoObjectIDs = gDatabase.decomposeObjectType(turretSocketObjectType, &turretSocketObjectIDs);
   if (tempBuildingProtoObjectIDs)
      buildingProtoObjectIDs.assignNoDealloc(*tempBuildingProtoObjectIDs);
   if (buildingSocketObjectIDs)
      buildingProtoObjectIDs.append(*buildingSocketObjectIDs);
   if (turretSocketObjectIDs)
      buildingProtoObjectIDs.append(*turretSocketObjectIDs);
   numBuildingProtoObjectIDs += (numBuildingSocketProtoObjectIDs + numTurretSocketProtoObjectIDs);
   for (uint i = 0; i < numBuildingProtoObjectIDs; i++)
   {
      BProtoObjectID buildingProtoObjectID = buildingProtoObjectIDs[i];
      uint numBuildingsOfType = getNumUnitsOfType(buildingProtoObjectID);
      if (numBuildingsOfType == 0)
         continue;
      const BProtoObject* pProtoObject = getProtoObject(buildingProtoObjectID);
      if (!pProtoObject)
         continue;

      uint numCommands = pProtoObject->getNumberCommands();
      for (uint commandIndex = 0; commandIndex < numCommands; commandIndex++)
      {
         if (!pProtoObject->getCommandAvailable(commandIndex))
            continue;
         BProtoObjectCommand command = pProtoObject->getCommand(commandIndex);
         if ((command.getType() != BProtoObjectCommand::cTypeBuild) && (command.getType() != BProtoObjectCommand::cTypeBuildOther))
            continue;
         BProtoObjectID protoObjectID = command.getID();
         const BProtoObject* pProtoObject = getProtoObject(protoObjectID);
         if (!pProtoObject)
            continue;
         if (pProtoObject->getFlagForbid())
            continue;
         if (!pProtoObject->getFlagAvailable())
            continue;
         legalBuildings.add(protoObjectID);
      }
   }

   return (legalBuildings.getSize());
}


//==============================================================================
// BPlayer::updateResourceTrickle
//==============================================================================
void BPlayer::updateResourceTrickle(float elapsedTime)
{
   if (mPlayerState != cPlayerStatePlaying)
      return;
#ifdef SYNC_Player
   syncPlayerData("BPlayer::updateResourceTrickle elapsedTime", elapsedTime);
#endif
   long count=mResourceTrickleRate.getNumberResources();
   for(long i=0; i<count; i++)
   {
      float rate=mResourceTrickleRate.get(i);
      if(rate!=0.0f)
      {
#ifdef SYNC_Player
         syncPlayerData("BPlayer::updateResourceTrickle i", i);
         syncPlayerData("BPlayer::updateResourceTrickle rate", rate);
#endif
         addResource(i, rate*elapsedTime);
      }
   }
}


//=============================================================================
//=============================================================================
void BPlayer::addUnitToProtoObject(const BUnit* pUnit, BProtoObjectID protoObjectID)
{
   BASSERT(pUnit);
   if (!pUnit)
      return;
   // DO NOT TRACK physics replacements.
   if (pUnit->getFlagIsPhysicsReplacement())
      return;

   BASSERT(this->isSelf(pUnit->getPlayerID()));

   // Get base type if protoObjectID isn't one
   if (!gDatabase.isValidProtoObject(protoObjectID))
   {
//-- FIXING PREFIX BUG ID 6134
      const BProtoObject* pProtoObject = getProtoObject(protoObjectID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
         protoObjectID = pProtoObject->getBaseType();
      BASSERT(gDatabase.isValidProtoObject(protoObjectID));
   }

   BASSERT(!mUnitsByProtoObject[protoObjectID].contains(pUnit->getID()));
   mUnitsByProtoObject[protoObjectID].add(pUnit->getID());
   mTotalUnitCounts++;

   BASSERT(!mUnits.contains(pUnit->getID()));
   mUnits.add(pUnit->getID());
   BASSERT(mUnits.getSize() == mTotalUnitCounts);

   //-- Update player combat value
   mTotalCombatValue += pUnit->getProtoObject()->getCombatValue();

   // Update leader unit if needed
   if (pUnit->isType(gDatabase.getOTIDLeader()))
   {
      BASSERT(mLeaderUnit == cInvalidObjectID);
      mLeaderUnit = pUnit->getID();
   }
}


//=============================================================================
//=============================================================================
void BPlayer::removeUnitFromProtoObject(const BUnit* pUnit, BProtoObjectID protoObjectID)
{
   BASSERT(pUnit);
   if (!pUnit)
      return;
   // DO NOT TRACK physics replacements.
   if (pUnit->getFlagIsPhysicsReplacement())
      return;

   BASSERT(this->isSelf(pUnit->getPlayerID()));

   // Get base type if protoObjectID isn't one
   if (!gDatabase.isValidProtoObject(protoObjectID))
   {
//-- FIXING PREFIX BUG ID 6136
      const BProtoObject* pProtoObject = getProtoObject(protoObjectID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
         protoObjectID = pProtoObject->getBaseType();
      BASSERT(gDatabase.isValidProtoObject(protoObjectID));
   }

   BASSERT(mUnitsByProtoObject[protoObjectID].contains(pUnit->getID()));
   mUnitsByProtoObject[protoObjectID].remove(pUnit->getID(), false);
   BASSERT(mTotalUnitCounts > 0);
   mTotalUnitCounts--;

   BASSERT(mUnits.contains(pUnit->getID()));
   mUnits.remove(pUnit->getID(), false);
   BASSERT(mUnits.getSize() == mTotalUnitCounts);

   //-- Update player combat value
   mTotalCombatValue -= pUnit->getProtoObject()->getCombatValue();

      // Update leader unit if needed
   if (pUnit->isType(gDatabase.getOTIDLeader()))
   {
      BASSERT(mLeaderUnit != cInvalidObjectID);
      mLeaderUnit = cInvalidObjectID;
   }
}


//=============================================================================
//=============================================================================
void BPlayer::addSquadToProtoSquad(const BSquad* pSquad, BProtoSquadID protoSquadID)
{
   BASSERT(pSquad);
   BASSERT(this->isSelf(pSquad->getPlayerID()));

   // Get base type if protoSquadID isn't one
   if (!gDatabase.isValidProtoSquad(protoSquadID))
   {
//-- FIXING PREFIX BUG ID 6138
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
         protoSquadID = pProtoSquad->getBaseType();
      BASSERT(gDatabase.isValidProtoSquad(protoSquadID));
   }

   BASSERT(!mSquadsByProtoSquad[protoSquadID].contains(pSquad->getID()));
   mSquadsByProtoSquad[protoSquadID].add(pSquad->getID());
   mTotalSquadCounts++;

   BASSERT(!mSquads.contains(pSquad->getID()));
   mSquads.add(pSquad->getID());
   BASSERT(mSquads.getSize() == mTotalSquadCounts);
}


//=============================================================================
//=============================================================================
void BPlayer::removeSquadFromProtoSquad(const BSquad* pSquad, BProtoSquadID protoSquadID)
{
   BASSERT(pSquad);
   BASSERT(this->isSelf(pSquad->getPlayerID()));

   // Get base type if protoSquadID isn't one
   if (!gDatabase.isValidProtoSquad(protoSquadID))
   {
//-- FIXING PREFIX BUG ID 6140
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
         protoSquadID = pProtoSquad->getBaseType();
      BASSERT(gDatabase.isValidProtoSquad(protoSquadID));
   }

   BASSERT(mSquadsByProtoSquad[protoSquadID].contains(pSquad->getID()));
   mSquadsByProtoSquad[protoSquadID].remove(pSquad->getID(), false);
   BASSERT(mTotalSquadCounts > 0);
   mTotalSquadCounts--;

   BASSERT(mSquads.contains(pSquad->getID()));
   mSquads.remove(pSquad->getID(), false);
   BASSERT(mSquads.getSize() == mTotalSquadCounts);
}


//=============================================================================
//=============================================================================
BProtoObject* BPlayer::allocateUniqueProtoObject(const BProtoObject* pBasePO, const BPlayer* pBasePlayer, BEntityID entityID)
{
   // Get base proto
   if (!pBasePO)
      return NULL;

   //Get the new index.
   long newIndex=mUniqueProtoObjects.getNumber();

   //Base the POID off of the index.
   long newPOID=getUniqueProtoObjectIDFromIndex(newIndex);

   //Get space.
   if (mUniqueProtoObjects.setNumber(newIndex+1) == false)
   {
      BASSERT(0);
      return(NULL);
   }
   mUniqueProtoObjects[newIndex].mProtoObject=NULL;
   //mUniqueProtoObjects[newIndex].mBasePOID=-1;
   //mUniqueProtoObjects[newIndex].mEntityID=cInvalidObjectID;

   //Allocate it.
   BProtoObject *newPO=new BProtoObject(pBasePO);
   if (newPO == NULL)
      return(NULL);

   //Now, specialize the new PU.
   newPO->setFlagUniqueInstance(true);
   newPO->setID(newPOID);
   newPO->setBaseType(pBasePO->getBaseType());

   BTactic* pTactic = newPO->getTactic();
   if (pTactic)
   {
      pTactic->setPlayerID(mID);
      pTactic->setProtoObjectID(newPOID);
   }

   //Save it.
   mUniqueProtoObjects[newIndex].mProtoObject=newPO;
   //mUniqueProtoObjects[newIndex].mBasePOID=pBasePO->getBaseType();
   //mUniqueProtoObjects[newIndex].mEntityID=entityID;

   // Go through all the models in the protoVisual and save off unique tech status
   // for valid VisualLogicTech
   BProtoVisual* pProtoVisual = gVisualManager.getProtoVisual(newPO->getProtoVisualIndex(), true);
   if (pBasePlayer && pProtoVisual)
   {
      BTechTree* pTechTree = const_cast<BPlayer*>(pBasePlayer)->getTechTree();
      if (pTechTree)
      {
         for (int i = 0; i < pProtoVisual->mModels.getNumber(); i++)
         {
            BProtoVisualModel* pModel = pProtoVisual->mModels[i];
            if (pModel)
            {
               if (pModel->mpLogicNode && (pModel->mpLogicNode->mLogicType == cVisualLogicTech))
               {
                  int valueCount = pModel->mpLogicNode->mLogicValues.getNumber();
                  for (int j = 0; j < valueCount; j++)
                  {
//-- FIXING PREFIX BUG ID 6141
                     const BProtoVisualLogicValue* pLogicValue = &(pModel->mpLogicNode->mLogicValues[j]);
//--
                     long techID = (long)pLogicValue->mValueDWORD;
                     if(techID != -1)
                     {
                        newPO->addUniqueTechStatus(techID, pTechTree->getTechStatus(techID, entityID));
                     }
                  }
               }
            }
         }
      }
   }
   
   //Done.
   return(newPO);
}


//=============================================================================
//=============================================================================
BProtoSquad* BPlayer::allocateUniqueProtoSquad(const BProtoSquad* pBasePS, const BPlayer* pBasePlayer, BEntityID squadID)
{
   // Get base proto
   if (!pBasePS || !pBasePlayer)
      return NULL;

   //Get the new index.
   long newIndex=mUniqueProtoSquads.getNumber();

   //Base the PSID off of the index.
   long newPSID=getUniqueProtoSquadIDFromIndex(newIndex);

   //Get space.
   if (mUniqueProtoSquads.setNumber(newIndex+1) == false)
   {
      BASSERT(0);
      return(NULL);
   }
   mUniqueProtoSquads[newIndex].mProtoSquad=NULL;
   //mUniqueProtoSquads[newIndex].mBasePSID=-1;
   //mUniqueProtoSquads[newIndex].mSquadID=cInvalidObjectID;

   //Allocate it.
   BProtoSquad *newPS=new BProtoSquad(pBasePS, this);
   if (newPS == NULL)
      return(NULL);

   //Now, specialize the new PU.
   newPS->setFlagUniqueInstance(true);
   newPS->setID(newPSID);
   newPS->setBaseType(pBasePS->getBaseType());

   //Save it.
   mUniqueProtoSquads[newIndex].mProtoSquad=newPS;
   //mUniqueProtoSquads[newIndex].mBasePSID=pBasePS->getBaseType();
   //mUniqueProtoSquads[newIndex].mSquadID=squadID;
   
   // Allocate new protoobjects for all object types in protoSquad and create
   // override unitNodes in the protoSquad that point to these new protoObject IDs.
   newPS->createOverrideUnitNodeArray();

   //BSquad* pSquad = gWorld->getSquad(squadID);
   int numUnitNodes = newPS->getNumberUnitNodes();
   for (int i = 0; i < numUnitNodes; i++)
   {
      BProtoSquadUnitNode& node = const_cast<BProtoSquadUnitNode&>(newPS->getUnitNode(i));
//-- FIXING PREFIX BUG ID 6143
      const BProtoObject* pBasePO = pBasePlayer->getProtoObject(node.mUnitType);
//--
//-- FIXING PREFIX BUG ID 6144
      const BProtoObject* pNewPO = allocateUniqueProtoObject(pBasePO, pBasePlayer, cInvalidObjectID);
//--
      if (pNewPO)
      {
         node.mUnitType = pNewPO->getID();
      }
   }

   //Done.
   return(newPS);
}


//=============================================================================
//=============================================================================
uint BPlayer::getUnitsOfType(BObjectTypeID objectTypeID, BEntityIDArray& unitIDs) const
{
   // Clear out our results
   unitIDs.resize(0);

   if (gDatabase.isValidObjectType(objectTypeID))
   {
      if (gDatabase.isValidProtoObject(objectTypeID))
      {
         // Just do the fast lookup.
         unitIDs.assignNoDealloc(mUnitsByProtoObject[objectTypeID]);
      }
      else if (gDatabase.isValidAbstractType(objectTypeID))
      {
         // Decompose the abstract type into all proto objects and do multiple fast lookups
         BProtoObjectIDArray *decomposedProtoObjects = NULL;
         uint numDecomposedProtoObjects = gDatabase.decomposeObjectType(objectTypeID, &decomposedProtoObjects);
         if (decomposedProtoObjects)
         {
            for (uint i=0; i<numDecomposedProtoObjects; i++)
            {
               unitIDs.append(mUnitsByProtoObject[decomposedProtoObjects->get(i)]);
            }
         }
      }
   }
   else
   {
      // If this is a unique protoObject, use its baseType
//-- FIXING PREFIX BUG ID 6146
      const BProtoObject* pProtoObject = getProtoObject(objectTypeID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         objectTypeID = pProtoObject->getBaseType();
         BASSERT(gDatabase.isValidProtoObject(objectTypeID));
         if (gDatabase.isValidProtoObject(objectTypeID))
         {
            // Just do the fast lookup.
            unitIDs.assignNoDealloc(mUnitsByProtoObject[objectTypeID]);
         }
      }
   }

   return (unitIDs.getSize());
}


//=============================================================================
//=============================================================================
uint BPlayer::getUnits(BEntityIDArray& unitIDs) const
{
   unitIDs = mUnits;
   return (unitIDs.getSize());
}


//=============================================================================
//=============================================================================
uint BPlayer::getNumUnitsOfType(BObjectTypeID objectTypeID) const
{
   // Clear out our results
   uint numUnitsOfType = 0;

   if (gDatabase.isValidObjectType(objectTypeID))
   {
      if (gDatabase.isValidProtoObject(objectTypeID))
      {
         // Just do the fast lookup.
         numUnitsOfType += mUnitsByProtoObject[objectTypeID].getSize();
      }
      else if (gDatabase.isValidAbstractType(objectTypeID))
      {
         // Decompose the abstract type into all proto objects and do multiple fast lookups
         BProtoObjectIDArray *decomposedProtoObjects = NULL;
         uint numDecomposedProtoObjects = gDatabase.decomposeObjectType(objectTypeID, &decomposedProtoObjects);
         if (decomposedProtoObjects)
         {
            for (uint i=0; i<numDecomposedProtoObjects; i++)
            {
               numUnitsOfType += mUnitsByProtoObject[decomposedProtoObjects->get(i)].getSize();
            }
         }
      }
   }
   else
   {
      // If this is a unique protoObject, use its baseType
//-- FIXING PREFIX BUG ID 6148
      const BProtoObject* pProtoObject = getProtoObject(objectTypeID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         objectTypeID = pProtoObject->getBaseType();
         BASSERT(gDatabase.isValidProtoObject(objectTypeID));
         if (gDatabase.isValidProtoObject(objectTypeID))
         {
            // Just do the fast lookup.
            numUnitsOfType += mUnitsByProtoObject[objectTypeID].getSize();
         }
      }
   }

   return (numUnitsOfType);
}


//=============================================================================
//=============================================================================
uint BPlayer::getSquadsOfType(BProtoSquadID protoSquadID, BEntityIDArray& squadIDs) const
{
   if (gDatabase.isValidProtoSquad(protoSquadID))
      squadIDs = mSquadsByProtoSquad[protoSquadID];
   else
   {
      // Get base type if protoSquadID isn't one
//-- FIXING PREFIX BUG ID 6150
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         BASSERT(gDatabase.isValidProtoSquad(protoSquadID));
         if (gDatabase.isValidProtoSquad(protoSquadID))
            squadIDs = mSquadsByProtoSquad[protoSquadID];
         else
            squadIDs.resize(0);
      }
      else
         squadIDs.resize(0);
   }

   return (squadIDs.getSize());
}


//=============================================================================
//=============================================================================
uint BPlayer::getSquads(BEntityIDArray& squadIDs) const
{
   squadIDs = mSquads;
   return (squadIDs.getSize());
}


//=============================================================================
//=============================================================================
uint BPlayer::getNumSquadsOfType(BProtoSquadID protoSquadID) const
{
   if (gDatabase.isValidProtoSquad(protoSquadID))
      return (mSquadsByProtoSquad[protoSquadID].getSize());
   else
   {
      // Get base type if protoSquadID isn't one
//-- FIXING PREFIX BUG ID 6152
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         BASSERT(gDatabase.isValidProtoSquad(protoSquadID));
         if (gDatabase.isValidProtoSquad(protoSquadID))
            return (mSquadsByProtoSquad[protoSquadID].getSize());
         else
            return 0;
      }
      else
         return (0);
   }
}



//=============================================================================
//=============================================================================
void BPlayer::adjustFutureUnitCount(BProtoObjectID protoObjectID, int amount)
{
   bool isProtoObject = gDatabase.isValidProtoObject(protoObjectID);
   
   // Get base type if protoObjectID isn't one
   if (!isProtoObject)
   {
//-- FIXING PREFIX BUG ID 6154
      const BProtoObject* pProtoObject = getProtoObject(protoObjectID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         protoObjectID = pProtoObject->getBaseType();
         isProtoObject = gDatabase.isValidProtoSquad(protoObjectID);
      }
   }

   BASSERT(isProtoObject);
   if (isProtoObject)
   {
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mFutureUnitCounts[protoObjectID]));
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mTotalFutureUnitCounts));
      mFutureUnitCounts[protoObjectID] += amount;
      mTotalFutureUnitCounts += amount;
   }
}

//=============================================================================
//=============================================================================
uint BPlayer::getFutureUnitCount(BObjectTypeID objectTypeID) const
{
   uint numFutureUnitsOfType = 0;
   bool isValidObjectType = gDatabase.isValidObjectType(objectTypeID);
   if (isValidObjectType)
   {
      if (gDatabase.isValidProtoObject(objectTypeID))
      {
         // Just do the fast lookup.
         numFutureUnitsOfType += mFutureUnitCounts[objectTypeID];
      }
      else if (gDatabase.isValidAbstractType(objectTypeID))
      {
         // Decompose the abstract type into all proto objects and do multiple fast lookups
         BProtoObjectIDArray *decomposedProtoObjects = NULL;
         uint numDecomposedProtoObjects = gDatabase.decomposeObjectType(objectTypeID, &decomposedProtoObjects);
         if (decomposedProtoObjects)
         {
            for (uint i=0; i<numDecomposedProtoObjects; i++)
               numFutureUnitsOfType += mFutureUnitCounts[decomposedProtoObjects->get(i)];
         }
      }
   }
   else
   {
      // If this is a unique protoObject, use its baseType
//-- FIXING PREFIX BUG ID 6156
      const BProtoObject* pProtoObject = getProtoObject(objectTypeID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         objectTypeID = pProtoObject->getBaseType();
         BASSERT(gDatabase.isValidProtoObject(objectTypeID));
         if (gDatabase.isValidProtoObject(objectTypeID))
         {
            // Just do the fast lookup.
            numFutureUnitsOfType += mFutureUnitCounts[objectTypeID];
         }
      }
   }
   
   return (numFutureUnitsOfType);
}

//=============================================================================
//=============================================================================
void BPlayer::adjustDeadUnitCount(BProtoObjectID protoObjectID, int amount)
{
   bool isProtoObject = gDatabase.isValidProtoObject(protoObjectID);
   
   // Get base type if protoObjectID isn't one
   if (!isProtoObject)
   {
//-- FIXING PREFIX BUG ID 6158
      const BProtoObject* pProtoObject = getProtoObject(protoObjectID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         protoObjectID = pProtoObject->getBaseType();
         isProtoObject = gDatabase.isValidProtoSquad(protoObjectID);
      }
   }

   BASSERT(isProtoObject);
   if (isProtoObject)
   {
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mDeadUnitCounts[protoObjectID]));
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mTotalDeadUnitCounts));
      mDeadUnitCounts[protoObjectID] += amount;
      mTotalDeadUnitCounts += amount;
   }
}

//=============================================================================
//=============================================================================
uint BPlayer::getDeadUnitCount(BObjectTypeID objectTypeID) const
{
   uint numDeadUnitsOfType = 0;
   bool isValidObjectType = gDatabase.isValidObjectType(objectTypeID);
   if (isValidObjectType)
   {
      if (gDatabase.isValidProtoObject(objectTypeID))
      {
         // Just do the fast lookup.
         numDeadUnitsOfType += mDeadUnitCounts[objectTypeID];
      }
      else if (gDatabase.isValidAbstractType(objectTypeID))
      {
         // Decompose the abstract type into all proto objects and do multiple fast lookups
         BProtoObjectIDArray *decomposedProtoObjects = NULL;
         uint numDecomposedProtoObjects = gDatabase.decomposeObjectType(objectTypeID, &decomposedProtoObjects);
         if (decomposedProtoObjects)
         {
            for (uint i=0; i<numDecomposedProtoObjects; i++)
               numDeadUnitsOfType += mDeadUnitCounts[decomposedProtoObjects->get(i)];
         }
      }
   }
   else
   {
      // If this is a unique protoObject, use its baseType
//-- FIXING PREFIX BUG ID 6160
      const BProtoObject* pProtoObject = getProtoObject(objectTypeID);
//--
      BASSERT(pProtoObject);
      if (pProtoObject)
      {
         objectTypeID = pProtoObject->getBaseType();
         BASSERT(gDatabase.isValidProtoObject(objectTypeID));
         if (gDatabase.isValidProtoObject(objectTypeID))
         {
            // Just do the fast lookup.
            numDeadUnitsOfType += mDeadUnitCounts[objectTypeID];
         }
      }
   }

   return (numDeadUnitsOfType);
}

//=============================================================================
//=============================================================================
void BPlayer::adjustFutureSquadCount(BProtoSquadID protoSquadID, int amount)
{
   bool isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);
   
   // Get base type if protoSquadID isn't one
   if (!isProtoSquad)
   {
//-- FIXING PREFIX BUG ID 6162
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);
      }
   }

   BASSERT(isProtoSquad);
   if (isProtoSquad)
   {
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mFutureSquadCounts[protoSquadID]));
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mTotalFutureSquadCounts));
      mFutureSquadCounts[protoSquadID] += amount;
      mTotalFutureSquadCounts += amount;
   }
}

//=============================================================================
//=============================================================================
uint BPlayer::getFutureSquadCount(BProtoSquadID protoSquadID) const
{
   bool isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);

   // Get base type if protoSquadID isn't one
   if (!isProtoSquad)
   {
//-- FIXING PREFIX BUG ID 6164
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);
      }
   }

   BASSERT(isProtoSquad);
   if (isProtoSquad)
      return (mFutureSquadCounts[protoSquadID]);
   else
      return (0);
}

//=============================================================================
//=============================================================================
void BPlayer::adjustDeadSquadCount(BProtoSquadID protoSquadID, int amount)
{
   bool isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);

   // Get base type if protoSquadID isn't one
   if (!isProtoSquad)
   {
//-- FIXING PREFIX BUG ID 6166
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);
      }
   }

   BASSERT(isProtoSquad);
   if (isProtoSquad)
   {
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mDeadSquadCounts[protoSquadID]));
      BASSERT((amount >= 0) || (static_cast<uint>(-amount) <= mTotalDeadSquadCounts));
      mDeadSquadCounts[protoSquadID] += amount;
      mTotalDeadSquadCounts += amount;
   }
}

//=============================================================================
//=============================================================================
uint BPlayer::getDeadSquadCount(BProtoSquadID protoSquadID) const
{
   bool isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);

   // Get base type if protoSquadID isn't one
   if (!isProtoSquad)
   {
//-- FIXING PREFIX BUG ID 6168
      const BProtoSquad* pProtoSquad = getProtoSquad(protoSquadID);
//--
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         protoSquadID = pProtoSquad->getBaseType();
         isProtoSquad = gDatabase.isValidProtoSquad(protoSquadID);
      }
   }

   BASSERT(isProtoSquad);
   if (isProtoSquad)
      return (mDeadSquadCounts[protoSquadID]);
   else
      return (0);
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getClosestUnitOfType(BObjectTypeID objectTypeID, BVector location) const
{
   BEntityIDArray results;
   float closestDist = cMaximumFloat;
   BUnit* pClosestUnit = NULL;
   uint numUnitsFound = this->getUnitsOfType(objectTypeID, results);
   for (uint i=0; i<numUnitsFound; i++)
   {
      BUnit* pTempUnit = gWorld->getUnit(results[i]);
      if (pTempUnit)
      {
         float tempDist = pTempUnit->getPosition().distanceSqr(location);
         if (tempDist < closestDist)
         {
            pClosestUnit = pTempUnit;
            closestDist = tempDist;
         }
      }
   }
   return (pClosestUnit);
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getAnyUnitOfType(BObjectTypeID objectTypeID) const
{
   BEntityIDArray results;
   uint numUnitsFound = this->getUnitsOfType(objectTypeID, results);
   for (uint i=0; i<numUnitsFound; i++)
   {
      BUnit* pUnit = gWorld->getUnit(results[i]);
      if (pUnit)
         return (pUnit);
   }
   return (NULL);
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getLeastQueuedBuildingOfType(BProtoObjectIDArray objectTypeIDs) const
{
   BEntityIDArray results;

   uint numTrainerTypes = objectTypeIDs.size();
   for(uint i=0; i<numTrainerTypes; i++)
   {  
      BEntityIDArray tempResults;

      const BObjectTypeID objectTypeID = objectTypeIDs[i];
      uint numUnitsFound = this->getUnitsOfType(objectTypeID, tempResults);
      results.add(tempResults.getPtr(), numUnitsFound);
   }

   //uint numUnitsFound = this->getUnitsOfType(objectTypeID, results);
   uint numUnitsFound = results.size();

   float fastestQueue = 10000000000.0f;
   long count;
   float queueTime;
   BUnit* pBestUnit = NULL;
   for (uint i=0; i<numUnitsFound; i++)
   {
      BUnit* pUnit = gWorld->getUnit(results[i]);
      if (pUnit)
      {
         if(pUnit->getFlagBuilt() == false)
            continue;

         const BSquad* pParentSquad = pUnit->getParentSquad();
         if (!pParentSquad)
            continue;

         if (pParentSquad->getSquadMode() != BSquadAI::cModeLockdown && !pUnit->isType(gDatabase.getOTIDBuilding()))
            continue;

         if(pUnit->getBuildPercent() > 0.8) //hard coded @ 80% until we are ready to tune this part of the AI
         {
            pUnit->getTrainQueue(getID(), &count, &queueTime);
            if(queueTime < fastestQueue)
            {
               fastestQueue = queueTime;
               pBestUnit = pUnit;
            }
         }
      }
   }
   return pBestUnit;
}

//=============================================================================
//=============================================================================
void BPlayer::getTrainQueueTimes(float *pTotalTime) const
{
   static BEntityIDArray results;
   results.resize(0);

   BObjectTypeID objectTypeID = gDatabase.getOTIDBuilding();
   uint numUnitsFound = this->getUnitsOfType(objectTypeID, results);
   
   (*pTotalTime) = 0.0f;

   long count;
   float queueTime;
   for (uint i=0; i<numUnitsFound; i++)
   {
//-- FIXING PREFIX BUG ID 6170
      const BUnit* pUnit = gWorld->getUnit(results[i]);
//--
      if (pUnit)
      {
         pUnit->getTrainQueue(getID(), &count, &queueTime);
         (*pTotalTime) += queueTime;
      }
   }


}

//=============================================================================
//=============================================================================
uint BPlayer::getAvailableUniqueTechBuildings(BProtoTechID protoTechID) const
{
   uint count = 0;
   BEntityIDArray results;

   const BProtoTech* pProtoTech = this->getProtoTech(protoTechID);
   if (pProtoTech)
   {
      BProtoObjectID buildingProtoObjectID = this->getBuildingProtoIDToResearchTech(protoTechID, true);
      if(buildingProtoObjectID == cInvalidProtoObjectID)
         return 0;

      uint numUnitsFound = this->getUnitsOfType(buildingProtoObjectID, results);
      
      for (uint i=0; i<numUnitsFound; i++)
      {
//-- FIXING PREFIX BUG ID 6171
         const BUnit* pUnit = gWorld->getUnit(results[i]);
//--
         if (pUnit)
         {
            const BUnitActionBuilding* pAction = (const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
            if(pAction)
            {
               if(pAction->hasUniqueTech() == false)
               {
                  count++;
               }
            }
         }
      }
   }
   return count;
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getBuildingToTrainSquad(BProtoSquadID protoSquadID, BVector *pLocation) const
{
   BUnit* pBuilding = NULL;
   const BProtoSquad* pProtoSquad = this->getProtoSquad(protoSquadID);
   if (pProtoSquad)
   {
      protoSquadID = pProtoSquad->getBaseType(); // get base type in case this is a unique protoSquad
      //BProtoObjectID buildingProtoObjectID = this->getBuildingProtoIDToTrainSquad(protoSquadID, true);
      BProtoObjectIDArray buildingProtoObjectIDArray = this->getBuildingProtoIDArrayToTrainSquad(protoSquadID, true);

      if (buildingProtoObjectIDArray.size() > 0 )//gDatabase.isValidProtoObject(buildingProtoObjectID))
      {
         //if (pLocation)
         //   pBuilding = this->getClosestUnitOfType(buildingProtoObjectID, *pLocation);
         //else
         {
            //pBuilding = this->getAnyUnitOfType(buildingProtoObjectID);
            pBuilding = getLeastQueuedBuildingOfType(buildingProtoObjectIDArray);

         }
      }
   }
   return (pBuilding);
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getBuildingToResearchTech(BProtoTechID protoTechID, BVector *pLocation) const
{
   BUnit* pBuilding = NULL;
   const BProtoTech* pProtoTech = this->getProtoTech(protoTechID);
   if (pProtoTech)
   {
      BProtoObjectID buildingProtoObjectID = this->getBuildingProtoIDToResearchTech(protoTechID, true);

      // Get base type in case this is a unique proto object
      if (!gDatabase.isValidProtoObject(buildingProtoObjectID))
      {
//-- FIXING PREFIX BUG ID 6172
         const BProtoObject* pPO = getProtoObject(buildingProtoObjectID);
//--
         if (pPO)
            buildingProtoObjectID = pPO->getBaseType();
      }

      if (gDatabase.isValidProtoObject(buildingProtoObjectID))
      {
         if (pLocation)
            pBuilding = this->getClosestUnitOfType(buildingProtoObjectID, *pLocation);
         else
            pBuilding = this->getAnyUnitOfType(buildingProtoObjectID);
      }
   }
   return (pBuilding);
}

//=============================================================================
//=============================================================================
BUnit* BPlayer::getBuildingToBuildStructure(BProtoObjectID protoObjectID, BVector *pLocation, const BEntityIDArray& blockedBuilders) const
{
   const BProtoObject* pProtoObject = this->getProtoObject(protoObjectID);
   if (pProtoObject)
   {
      BProtoObjectID buildingProtoObjectID = this->getBuildingProtoIDToBuildStructure(protoObjectID, true);
      //const BProtoObject* pBuilderProtoObject = this->getProtoObject(buildingProtoObjectID);

      // Get base type in case this is a unique proto object
      if (!gDatabase.isValidProtoObject(buildingProtoObjectID))
      {
//-- FIXING PREFIX BUG ID 6176
         const BProtoObject* pPO = getProtoObject(buildingProtoObjectID);
//--
         if (pPO)
            buildingProtoObjectID = pPO->getBaseType();
      }

      if (gDatabase.isValidProtoObject(buildingProtoObjectID))
      {

         bool isCommandableByAnyPlayer = false;
         const BProtoObject* pBuilderProtoObject = this->getProtoObject(buildingProtoObjectID);
         if(pBuilderProtoObject != NULL && pBuilderProtoObject->getFlagCommandableByAnyPlayer() == true)
         {
            isCommandableByAnyPlayer = true;
         }

         // The desired structure does not require sockets on the builder.
         if (pProtoObject->getSocketID() == -1)
         {
            if (pLocation)
            {
               BUnit* pBuilding = this->getClosestUnitOfType(buildingProtoObjectID, *pLocation);
               return (pBuilding);
            }
            else
            {
               BUnit* pBuilding = this->getAnyUnitOfType(buildingProtoObjectID);
               return (pBuilding);
            }
         }
         // The desired structure is a socket requiring structure, so do some special socket stuff here.
         else
         {
            BEntityIDArray potentialBuilders;
            BSmallDynamicSimArray<BUnit*> validBuilders;
            BEntityIDArray socketUnitIDs;
            uint numPotentialBuilders = 0;

            //get visible commandable objects
            if(isCommandableByAnyPlayer)
            {
               BEntityHandle h = cInvalidObjectID;
               BUnit *pUnit = gWorld->getNextUnit(h);
               while (pUnit)
               {
                  if (!pUnit->isAlive())
                  {
                     pUnit = gWorld->getNextUnit(h);
                     continue;
                  }                 
                  if (!pUnit->getProtoObject()->isType(buildingProtoObjectID))
                  {
                     pUnit = gWorld->getNextUnit(h);
                     continue;
                  }
                  if(pUnit->isVisible(getTeamID()) == false)
                  {
                     pUnit = gWorld->getNextUnit(h);
                     continue;
                  }
                  if(blockedBuilders.contains(pUnit->getID()) == true)
                  {
                     pUnit = gWorld->getNextUnit(h);
                     continue;
                  }

                  if ( pBuilderProtoObject && pBuilderProtoObject->isType(gDatabase.getOTIDSettlement()) && (pUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug) == NULL) )
                  {             
                     const BUnitActionBuilding* pBuildingAction = (const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
                     if (pBuildingAction && (pBuildingAction->getRebuildTimer() > 0.0f))
                     {
                        pUnit = gWorld->getNextUnit(h);
                        continue;
                     }                                             
                  }
   
                  numPotentialBuilders++;
                  potentialBuilders.add(pUnit->getID());
                  pUnit = gWorld->getNextUnit(h);
               }     

               for (uint i=0; i<numPotentialBuilders; i++)
               {
                  BUnit* pPotentialBuilder = gWorld->getUnit(potentialBuilders[i]);
                  if (!pPotentialBuilder)
                     continue;

                  if(blockedBuilders.contains(pPotentialBuilder->getID()) == true)
                  {
                     continue;
                  }

                  gWorld->getBuildSockets(mID, protoObjectID, pPotentialBuilder, cOriginVector, 0.0f, true, true, socketUnitIDs);
                  if (socketUnitIDs.getSize() > 0)
                  {
                     // If we don't care about location, return the first one with available sockets.
                     if (!pLocation)
                        return (pPotentialBuilder);
                     else
                        validBuilders.add(pPotentialBuilder);
                  }
               }

               uint numValidBuilders = validBuilders.getSize();
               if (numValidBuilders == 0)
                  return (NULL);

               if (pLocation)
               {
                  float closestDist = cMaximumFloat;
                  BUnit* pClosestValidBuilder = NULL;
                  for (uint i=0; i<numValidBuilders; i++)
                  {
                     float tempDist = validBuilders[i]->getPosition().distanceSqr(*pLocation);
                     if (tempDist < closestDist)
                     {
                        pClosestValidBuilder = validBuilders[i];
                        closestDist = tempDist;
                     }
                  }
                  return (pClosestValidBuilder);
               }
            }
            else  //A base building.   this will be refactored soon
            {
               BObjectTypeID otidBase = gDatabase.getOTIDBase();
               BEntityIDArray bases;
               numPotentialBuilders = getUnitsOfType(otidBase, potentialBuilders);

               if(numPotentialBuilders > 0)
               {
                  //int leastQueue = 100000;
                  float leastQueueTime = cMaximumFloat;
                  BUnit* pLeastQueuedBase = NULL;
                  BUnit* pEmptySocket = NULL;
//-- FIXING PREFIX BUG ID 6183
                  const BUnit* pBestSocket = NULL;
//--
                  for (uint i=0; i<numPotentialBuilders; i++)
                  {
                     BUnit* pBuilder = gWorld->getUnit(potentialBuilders[i]);
                     if(pBuilder == NULL)
                        continue;

                     if(blockedBuilders.contains(pBuilder->getID()) == true)
                     {
                        continue;
                     }

                     uint numEntityRefs = pBuilder->getNumberEntityRefs();

                     int numQueued = 0;
                     int numEmptySockets = 0;
                     float queueTime = 0;
                     float turretQueueTime = 0;


                     //BSimString* name = pBuilder->getName();

                     const BObjectTypeID turretBuilding = gDatabase.getOTIDTurretBuilding();

                     bool isObjectToBuildATurret = pProtoObject->isType(turretBuilding);

                     for(uint j=0; j < numEntityRefs; j++)
                     {
//-- FIXING PREFIX BUG ID 6182
                        const BEntityRef* pRef = pBuilder->getEntityRefByIndex(j);
//--
                        
                        float tempQueueTime = 0;
                        bool isTurret = false;


                        if(pRef && pRef->mType == BEntityRef::cTypeBuildQueueChild)
                        {
//-- FIXING PREFIX BUG ID 6178
                           const BUnit* pSocket = gWorld->getUnit(pRef->mID);
//--
                                             
                           long id;
                           if(pSocket)
                           {                              
                              id = pSocket->getProtoID();
                           }

                           numQueued++;
                           //Get the build time
                           BProtoObjectID obj = pRef->mData1;
                           const BProtoObject *pQueuedType = getProtoObject(obj);

                           if(pQueuedType)
                           {
                              tempQueueTime += pQueuedType->getBuildPoints();
                              isTurret = pQueuedType->isType(turretBuilding);
                           }
                        }
                        if(pRef && pRef->mType == BEntityRef::cTypeAssociatedBuilding)
                        {
                           const BUnit* buildn = gWorld->getUnit(pRef->mID);
                           float buildPercent = buildn->getBuildPercent();        
                           const BProtoObject *pQueuedType = buildn->getProtoObject();

                           if(pQueuedType)
                           {
                              isTurret = pQueuedType->isType(turretBuilding);
                           }

                           if(buildPercent != 1.0f)
                           {                                                           
                              tempQueueTime += ((1.0f -buildn->getBuildPercent()) * buildn->getProtoObject()->getBuildPoints());
                           }

                        }
                        if (pRef && pRef->mType == BEntityRef::cTypeAssociatedSocket)
                        {
                           BUnit* pSocket = gWorld->getUnit(pRef->mID);
   
                           if((pSocket != NULL) && (buildingProtoObjectID == pSocket->getProtoID()))
                           {                                                      
//-- FIXING PREFIX BUG ID 6181
                              const BEntityRef* pQueueRef = pSocket->getFirstEntityRefByType(BEntityRef::cTypeBuildQueueChild);
//--
                              if(pQueueRef && pQueueRef->mType == BEntityRef::cTypeBuildQueueChild)
                              {
                                 numQueued++;
                                 //Get the build time
                                 BProtoObjectID obj = pQueueRef->mData1;
                                 BProtoObject *pQueuedType = getProtoObject(obj);

                                 if(pQueuedType)
                                 {
                                    tempQueueTime += pQueuedType->getBuildPoints();
                                    isTurret = pQueuedType->isType(turretBuilding);
                                 }
                              }
         
                              if (pSocket && !pSocket->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug))
                              {
                                 // This socket has nothing plugged into it…
                                 pEmptySocket = pSocket;
                                 numEmptySockets++;
                                 //BSimString* name3 = pSocket->getName();
                              }
                           }
                        }

                        //only count non turret queue time
                        if(isTurret == false)
                        {
                           queueTime += tempQueueTime;
                        }
                        else
                        {
                           turretQueueTime += tempQueueTime;
                        }

                     }


                     //Temp ai queue limit...  this will be data driven later.
                     if((isObjectToBuildATurret == false) && (queueTime > 5)) //>=90)
                        continue;

                     if((isObjectToBuildATurret == true) && (turretQueueTime > 30)) //>=90)
                        continue;

                     if (queueTime < leastQueueTime && numEmptySockets > 0)
                     {
                        pLeastQueuedBase = pBuilder;
                        pBestSocket = pEmptySocket;
                        //leastQueue = numQueued;
                        leastQueueTime = queueTime;
                     }                  
                  }
                  return (pLeastQueuedBase);//pBestSocket);

               }
  
            }
         }
      }
   }

   return (NULL);
}

//=============================================================================
// BPlayer::addAbility
//=============================================================================
void BPlayer::addAbility(long v)
{
   if (gDatabase.getAbilityFromID(v))
      mAbilities.uniqueAdd(v);
}

//=============================================================================
// BPlayer::removeAbility
//=============================================================================
void BPlayer::removeAbility(long v)
{
   mAbilities.removeValue(v);
}

//=============================================================================
// BPlayer::removeAllAbilities
//=============================================================================
void BPlayer::removeAllAbilities(void)
{
   mAbilities.setNumber(0);
}

//=============================================================================
// BPlayer::canUseAbility
//=============================================================================
bool BPlayer::canUseAbility(long v)
{
   // ajl 1/4/07 - allow all abilities to be used by default for now
   v;
   return true;
   /*
   if (mAbilities.find(v) == -1)
      return(false);
   else
      return(true);
   */
}


//=============================================================================
// BPlayer::updateSupportPowerStatus
//=============================================================================
void BPlayer::updateSupportPowerStatus()
{
   uint powerCount=mSupportPowerStatus.getSize();
   if (powerCount==0)
      return;

   for (uint i=0; i<powerCount; i++)
   {
      if (mSupportPowerStatus[i]!=cSupportPowerStatusUnavailable)
         continue;

      const BLeaderSupportPower* pSupportPower = getSupportPower(i);
      if (pSupportPower->mTechPrereq==-1 || mpTechTree->getTechStatus(pSupportPower->mTechPrereq, -1)==BTechTree::cStatusActive)
      {
         //addPowerEntry(pSupportPower->mPowers, -1, 1);
         mSupportPowerStatus[i]=cSupportPowerStatusAvailable;
         gWorld->notify(BEntity::cEventSupportPowerAvailable, cInvalidObjectID, mID, i);
      }
   }
}


//=============================================================================
// BPlayer::addStrength
//=============================================================================
void BPlayer::addStrength(int32 strength)
{
   mStrength += strength;
}

//=============================================================================
// BPlayer::subtractStrength
//=============================================================================
void BPlayer::subtractStrength(int32 strength)
{
   mStrength -= strength;
}

//=============================================================================
// BPlayer::getStrength
//=============================================================================
int32 BPlayer::getStrength() const
{
   int32 strength = mStrength;

   // add in the resources
   strength += static_cast<int32>(getResources().getTotal());
   strength /= 100;           // normalize it

   return strength;
}


//=============================================================================
//=============================================================================
float BPlayer::getAIStrength() const
{
   // Total AI strength = SuppliesRate + PowerRate + Pop
   float suppliesRate = this->getRate(gDatabase.getRate("Supplies"));
   float powerRate = this->getRate(gDatabase.getRate("Power"));
   float popCount = this->getPopCount(gDatabase.getPop("Unit"));
   float popFuture = this->getPopFuture(gDatabase.getPop("Unit"));
   float totalAIStrength = suppliesRate + powerRate + popCount + popFuture;
   return (totalAIStrength);
}

//=============================================================================
//=============================================================================
void BPlayer::recomputeUnitVisuals(int protoObjectID)
{
   BEntityIDArray& units=mUnitsByProtoObject[protoObjectID];
   uint count=units.getSize();
   for (uint i=0; i<count; i++)
   {
      BUnit* pUnit=gWorld->getUnit(units[i]);
      if (pUnit)
         pUnit->recomputeVisual();
   }
}

//=============================================================================
//=============================================================================
BWeaponType* BPlayer::getWeaponTypeByID(int id) const
{
   if (id < 0 || id >= mWeaponTypes.getNumber())
      return NULL;
   return mWeaponTypes[id];
}

//=============================================================================
//=============================================================================
float BPlayer::getDamageModifier(int weaponTypeID, BEntityID targetID, BVector damageDirection, float &shieldedDamageModifier)
{
   BWeaponType* pWeaponType = getWeaponTypeByID(weaponTypeID);
   if (pWeaponType)
   {
      shieldedDamageModifier = pWeaponType->getShieldedModifier();
      return Math::Max(pWeaponType->getDamagePercentage(targetID, damageDirection), 0.0f);
   }
   else
   {
      shieldedDamageModifier = 1.0f;
      return 1.0f;
   }
}

//=============================================================================
//=============================================================================
bool BPlayer::getHaveLeaderPower() const
{
   int leaderPowerID=getLeader()->getLeaderPowerID();
   const BProtoPower* pProtoPower=gDatabase.getProtoPowerByID(leaderPowerID);
   if (!pProtoPower)
      return false;
   for (int i=0; i<getNumPowerEntries(); i++)
   {
      const BPowerEntry* pPowerEntry=const_cast<BPlayer*>(this)->getPowerEntryAtIndex(i);
      if (pPowerEntry->mProtoPowerID == leaderPowerID)
         return true;
   }
   return false;
}

//=============================================================================
//=============================================================================
float BPlayer::getLeaderPowerChargeCost(int protoObjectID) const
{
   if (!getHaveLeaderPower())
      return 0.0f;
   const BProtoPower* pProtoPower=gDatabase.getProtoPowerByID(getLeader()->getLeaderPowerID());
   if (!pProtoPower)
      return 0.0f;
   int resourceID=gDatabase.getLeaderPowerChargeResourceID();
//-- FIXING PREFIX BUG ID 6185
   const BProtoObject* pProto = getProtoObject(protoObjectID);
//--
   float cost=const_cast<BProtoPower*>(pProtoPower)->getCost(pProto)->get(resourceID);
   return cost;
}

//=============================================================================
//=============================================================================
uint BPlayer::getLeaderPowerChargeCostCount(int protoObjectID) const
{
   float baseCost = getLeaderPowerChargeCost(-1);
   float objectCost = getLeaderPowerChargeCost(protoObjectID);
   if (baseCost == 0.0f || objectCost == 0.0f)
      return 0;
   uint costCount = (uint) (objectCost / baseCost);
   return costCount;
}

//=============================================================================
//=============================================================================
uint BPlayer::getLeaderPowerChargeCount() const
{
   float cost = getLeaderPowerChargeCost();
   if (cost == 0.0f)
      return 0;
   float avail=getResource(gDatabase.getLeaderPowerChargeResourceID());
   uint numCharges = (uint)(avail / cost);
   return numCharges;
}

//=============================================================================
//=============================================================================
float BPlayer::getLeaderPowerChargePercent() const
{
   float cost = getLeaderPowerChargeCost();
   if (cost == 0.0f)
      return 0.0f;
   float avail=getResource(gDatabase.getLeaderPowerChargeResourceID());
   int numCharges = (int)(avail / cost);
   float percentToNextCharge = (avail - (numCharges*cost)) / cost * 100.0f;
   return percentToNextCharge;
}

//=============================================================================
//=============================================================================
void BPlayer::setPowerAvailableTime(BProtoPowerID protoPowerID, DWORD timeToWait)
{
   if (protoPowerID < 0 || protoPowerID >= mPowerAvailableTime.getNumber())
      return;
   
   mPowerAvailableTime[protoPowerID] = gWorld->getGametime() + timeToWait;
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxForProtoSquads()
{
   long numProtoSquads = getNumberProtoSquads();
   for (long i=0; i<numProtoSquads; i++)
   {
      recalculateMaxForProtoSquad(i);
   }
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxForProtoSquad(BProtoSquad* pProtoSquad)
{
   if (!pProtoSquad)
      return;

   float newMaxHP = 0.0f;
   float newMaxSP = 0.0f;
   float newMaxAmmo = 0.0f;
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long n=0; n<numUnitNodes; n++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(n);
      const BProtoObject* pProtoObject = getProtoObject(node.mUnitType);
      newMaxHP += (pProtoObject->getHitpoints() * node.mUnitCount);
      newMaxSP += (pProtoObject->getShieldpoints() * node.mUnitCount);
      newMaxAmmo += (pProtoObject->getMaxAmmo() * node.mUnitCount);
   }

   pProtoSquad->setMaxHP(newMaxHP);
   pProtoSquad->setMaxSP(newMaxSP);
   pProtoSquad->setMaxAmmo(newMaxAmmo);
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxHPForProtoSquad(BProtoSquad* pProtoSquad)
{
   if (!pProtoSquad)
      return;

   float newMaxHP = 0.0f;
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long n=0; n<numUnitNodes; n++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(n);
      const BProtoObject* pProtoObject = getProtoObject(node.mUnitType);
      newMaxHP += (pProtoObject->getHitpoints() * node.mUnitCount);
   }

   pProtoSquad->setMaxHP(newMaxHP);
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxSPForProtoSquad(BProtoSquad* pProtoSquad)
{
   if (!pProtoSquad)
      return;

   float newMaxSP = 0.0f;
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long n=0; n<numUnitNodes; n++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(n);
      const BProtoObject* pProtoObject = getProtoObject(node.mUnitType);
      newMaxSP += (pProtoObject->getShieldpoints() * node.mUnitCount);
   }

   pProtoSquad->setMaxSP(newMaxSP);
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxAmmoForProtoSquad(BProtoSquad* pProtoSquad)
{
   if (!pProtoSquad)
      return;

   float newMaxAmmo = 0.0f;
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long n=0; n<numUnitNodes; n++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(n);
      const BProtoObject* pProtoObject = getProtoObject(node.mUnitType);
      newMaxAmmo += (pProtoObject->getMaxAmmo() * node.mUnitCount);
   }

   pProtoSquad->setMaxAmmo(newMaxAmmo);
}

//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxHPForProtoSquads()
{
   long numProtoSquads = getNumberProtoSquads();
   for (long i=0; i<numProtoSquads; i++)
   {
      recalculateMaxHPForProtoSquad(i);
   }
}


//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxSPForProtoSquads()
{
   long numProtoSquads = getNumberProtoSquads();
   for (long i=0; i<numProtoSquads; i++)
   {
      recalculateMaxSPForProtoSquad(i);
   }
}


//=============================================================================
//=============================================================================
void BPlayer::recalculateMaxAmmoForProtoSquads()
{
   long numProtoSquads = getNumberProtoSquads();
   for (long i=0; i<numProtoSquads; i++)
   {
      recalculateMaxAmmoForProtoSquad(i);
   }
}

//==============================================================================
//==============================================================================
void BPlayer::updateSquadAITokens()
{
   //Make sure we're bounded.
   if ((mSquadAISearchIndex < 0) || (mSquadAISearchIndex >= mSquadAISquads.getNumber()))
      mSquadAISearchIndex=0;
   if ((mSquadAIWorkIndex < 0) || (mSquadAIWorkIndex >= mSquadAISquads.getNumber()))
      mSquadAIWorkIndex=0;
   if ((mSquadAISecondaryTurretScanIndex < 0) || (mSquadAISecondaryTurretScanIndex >= mSquadAISquads.getNumber()))
      mSquadAISecondaryTurretScanIndex=0;

   const uint cSearchCap=1;
   const uint cWorkCap=3;
   const uint cSecondaryTurretScanCap=1;

   //SEARCH TOKENS FIRST.
   //If we have less/equal squads to our cap, just do them all.
   if (mSquadAISquads.getSize() <= cSearchCap)
   {
      for (uint i=0; i < mSquadAISquads.getSize(); i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[i]);
         if (pSquad)
            pSquad->getSquadAI()->setFlagSearchToken(true);
      }
   }
   //Else, go through in order.
   else
   {   
      for (uint i=0; i < cSearchCap; i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[mSquadAISearchIndex]);
         if (pSquad)
            pSquad->getSquadAI()->setFlagSearchToken(true);
         //Increment and bound the index.
         mSquadAISearchIndex++;
         if (mSquadAISearchIndex >= mSquadAISquads.getNumber())
            mSquadAISearchIndex=0;
      }
   }
      
   //WORK TOKENS SECOND.
   //If we have less/equal squads to our cap, just do them all.
   if (mSquadAISquads.getSize() <= cWorkCap)
   {
      for (uint i=0; i < mSquadAISquads.getSize(); i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[i]);
         if (pSquad)
            pSquad->getSquadAI()->setFlagWorkToken(true);
      }
   }
   //Else, go through in order.
   else
   {
      for (uint i=0; i < cWorkCap; i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[mSquadAIWorkIndex]);
         if (pSquad)
            pSquad->getSquadAI()->setFlagWorkToken(true);
         //Increment and bound the index.
         mSquadAIWorkIndex++;
         if (mSquadAIWorkIndex >= mSquadAISquads.getNumber())
            mSquadAIWorkIndex=0;
      }
   }

   // now "secondary turret scan" tokens
   if (mSquadAISquads.getSize() <= cSecondaryTurretScanCap)
   {
      for (uint i=0; i < mSquadAISquads.getSize(); i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[i]);
         if (pSquad)
            pSquad->setSecondaryTurretScanTokens(true);
      }
   }
   else
   {
      for (uint i=0; i < cSecondaryTurretScanCap; i++)
      {
         BSquad *pSquad=gWorld->getSquad(mSquadAISquads[mSquadAISecondaryTurretScanIndex]);
         if (pSquad)
            pSquad->setSecondaryTurretScanTokens(true);
         //Increment and bound the index.
         mSquadAISecondaryTurretScanIndex++;
         if (mSquadAISecondaryTurretScanIndex >= mSquadAISquads.getNumber())
            mSquadAISecondaryTurretScanIndex=0;
      }
   }
}

//==============================================================================
//==============================================================================
void BPlayer::addSquadAISquad(const BSquad *pSquad)
{
   BASSERT(pSquad);
   BASSERT(pSquad->getPlayerID() == mID);

   //Make sure we want to add this.  Easier to centralize this here than in
   //the calling spots.
   if (pSquad->getFlagIgnoreAI())
      return;

   //In non-final mode, do some checking to make sure we're not double-inserting.
   #ifndef BUILD_FINAL
   for (uint i=0; i < mSquadAISquads.getSize(); i++)
   {
      if (mSquadAISquads[i] == pSquad->getID())
      {
         BASSERT(0);
         return;
      }
   }
   #endif
   
   //This simply adds to the end of the list.
   mSquadAISquads.add(pSquad->getID());
}

//==============================================================================
//==============================================================================
void BPlayer::removeSquadAISquad(const BSquad *pSquad)
{
   BASSERT(pSquad);
   BASSERT(pSquad->getPlayerID() == mID);

   //Remove the squad.  If we're removing it from "before" our next search index,
   //then we need to decrement that, too.
   for (long i=0; i < mSquadAISquads.getNumber(); i++)
   {
      if (mSquadAISquads[i] == pSquad->getID())
      {
         mSquadAISquads.removeIndex(i);
         if (i < mSquadAISearchIndex)
            mSquadAISearchIndex--;
         if (i < mSquadAIWorkIndex)
            mSquadAIWorkIndex--;
         if (i < mSquadAISecondaryTurretScanIndex)
            mSquadAISecondaryTurretScanIndex--;
         return;
      }
   }
}

//=============================================================================
//=============================================================================
void BPlayer::addGotoBase(BEntityID id)
{
   //Uniquely add this

   long highestBaseNumber = 0;
   for( long ind=0; ind<mGotoBases.getNumber(); ind++ )
   {
      if( mGotoBases[ind] == id )
         return;
      BUnit* pUnit=gWorld->getUnit(mGotoBases.get(ind));
      BASSERT(pUnit);
      if( pUnit->getBaseNumber() > highestBaseNumber )
         highestBaseNumber = pUnit->getBaseNumber();
   }

   //If we made it here, we aren't in the list, so add it.
   mGotoBases.add(id);

   BUnit* pUnit=gWorld->getUnit(mGotoBases.get(ind));
   BASSERT(pUnit);
   pUnit->setBaseNumber((short)(highestBaseNumber+1));
}

//=============================================================================
//=============================================================================
void BPlayer::setUser(BUser* pUser)
{
   mpUser=pUser; 
}

//=============================================================================
//=============================================================================
bool BPlayer::save(BStream* pStream, int saveType) const
{
   gSaveGame.logStats("PlayerStart");

   GFWRITEVECTOR(pStream, mLookAtPos);

   GFWRITESTRING(pStream, BSimString, mName, 50);

   GFWRITEVAR(pStream, BRallyPoint, mRallyPoint);

   GFWRITECLASSPTR(pStream, saveType, mpStats);
   gSaveGame.logStats("PlayerStats");

   int16 protoCount = (int16)gDatabase.getNumberProtoObjects();
   GFWRITEVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 20000);
   for (int16 i=0; i<protoCount; i++)
   {
//-- FIXING PREFIX BUG ID 6186
      const BProtoObject* pProtoObject = mProtoObjects[i];
//--
      if (!pProtoObject->getFlagPlayerOwned())
         continue;
      int objectClass = pProtoObject->getObjectClass();
      if (objectClass==cObjectClassUnit || objectClass==cObjectClassBuilding || objectClass==cObjectClassSquad)
      {
         GFWRITEVAL(pStream, int16, pProtoObject->getID());
         GFWRITEVAL(pStream, int16, pProtoObject->getBaseType());
         if (!pProtoObject->save(pStream, saveType))
            return false;
      }
   }
   GFWRITEVAL(pStream, int16, -1);
   gSaveGame.logStats("ProtoObjects");

   protoCount = (int16)gDatabase.getNumberProtoSquads();
   GFWRITEVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 20000);
   for (int16 i=0; i<protoCount; i++)
   {
//-- FIXING PREFIX BUG ID 6187
      const BProtoSquad* pProtoSquad = mProtoSquads[i];
//--
      if (pProtoSquad->getFlagObjectProtoSquad() || pProtoSquad->getFlagMergedProtoSquad())
         continue;
      GFWRITEVAL(pStream, int16, pProtoSquad->getID());
      GFWRITEVAL(pStream, int16, pProtoSquad->getBaseType());
      if (!pProtoSquad->save(pStream, saveType))
         return false;
   }
   GFWRITEVAL(pStream, int16, -1);
   gSaveGame.logStats("ProtoSquads");

   protoCount = (int16)gDatabase.getNumberProtoTechs();
   GFWRITEVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 5000);
   for (int16 i=0; i<protoCount; i++)
   {
      BProtoTech* pProtoTech = mProtoTechs[i];
      GFWRITEVAL(pStream, int16, pProtoTech->getID());
      if (!pProtoTech->save(pStream, saveType))
         return false;
   }
   GFWRITEVAL(pStream, int16, -1);
   gSaveGame.logStats("ProtoTechs");

   protoCount = (int16)mUniqueProtoObjects.getNumber();
   GFWRITEVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 1000);
   for (int16 i=0; i<protoCount; i++)
   {
      BProtoObject* pProtoObject = mUniqueProtoObjects[i].mProtoObject;
      GFWRITEVAL(pStream, int16, pProtoObject->getBaseType());
      if (!pProtoObject->save(pStream, saveType))
         return false;
   }
   gSaveGame.logStats("UniqueProtoObjects");

   protoCount = (int16)mUniqueProtoSquads.getNumber();
   GFWRITEVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 1000);
   for (int16 i=0; i<protoCount; i++)
   {
      BProtoSquad* pProtoSquad = mUniqueProtoSquads[i].mProtoSquad;
      GFWRITEVAL(pStream, int16, pProtoSquad->getBaseType());
      if (!pProtoSquad->save(pStream, saveType))
         return false;
   }
   gSaveGame.logStats("UniqueProtoSquads");

   GFWRITECLASSARRAY(pStream, saveType, mPowerEntries, uint8, 250);
   GFWRITEARRAY(pStream, long, mAbilities, uint8, 250);

   //BSmallDynamicSimArray<BYTE> mSupportPowerStatus;

   int numProtoPowers = gDatabase.getNumberProtoPowers();
   for (int i=0; i<numProtoPowers; i++)
   {
      GFWRITEVAR(pStream, DWORD, mPowerRechargeTime[i]);
      GFWRITEVAR(pStream, int, mPowerUseLimit[i]);
      GFWRITEVAR(pStream, BPowerLevel, mPowerLevel[i]);
      GFWRITEVAR(pStream, DWORD, mPowerAvailableTime[i]);
   }

   GFWRITECLASS(pStream, saveType, mResources);

   int rateCount = gDatabase.getNumberRates();
   for (int i=0; i<rateCount; i++)
   {
      GFWRITEVAR(pStream, float, mpRateAmounts[i]);
      GFWRITEVAR(pStream, float, mpRateMultipliers[i]);
   }

   GFWRITECLASS(pStream, saveType, mTotalResources);
   GFWRITECLASS(pStream, saveType, mResourceTrickleRate);

   //XUID mXUID;

   //BUser* mpUser;

   for (int i=0; i<mPopCount; i++)
      GFWRITEVAR(pStream, BPlayerPop, mpPops[i]);

   GFWRITEMARKER(pStream, cSaveMarkerPlayer1);
   gSaveGame.logStats("Player1");

   //BAIFactoidManager* mpFactoidManager;
   //BAlertManager* mpAlertManager;
   //BHumanPlayerAITrackingData* mpTrackingData;

   GFWRITEVAL(pStream, bool, (mpHintEngine != NULL));
   if (mpHintEngine)
      GFWRITECLASSPTR(pStream, saveType, mpHintEngine);

   //BEntityIDArrayArray mUnitsByProtoObject;
   //BEntityIDArrayArray mSquadsByProtoSquad;

   protoCount = (int16)gDatabase.getNumberProtoObjects();
   for (int16 i=0; i<protoCount; i++)
   {
      if (mFutureUnitCounts[i] > 0 || mDeadUnitCounts[i] > 0)
      {
         GFWRITEVAR(pStream, int16, i);
         GFWRITEVAR(pStream, uint32, mFutureUnitCounts[i]);
         GFWRITEVAR(pStream, uint32, mDeadUnitCounts[i]);
      }
   }
   int16 protoIndex = -1;
   GFWRITEVAR(pStream, int16, protoIndex);

   protoCount = (int16)gDatabase.getNumberProtoSquads();
   for (int16 i=0; i<protoCount; i++)
   {
      if (mFutureSquadCounts[i] > 0 || mDeadSquadCounts[i] > 0)
      {
         GFWRITEVAR(pStream, int16, i);
         GFWRITEVAR(pStream, uint32, mFutureSquadCounts[i]);
         GFWRITEVAR(pStream, uint32, mDeadSquadCounts[i]);
      }
   }
   protoIndex = -1;
   GFWRITEVAR(pStream, int16, protoIndex);

   GFWRITEMARKER(pStream, cSaveMarkerPlayer2);
   gSaveGame.logStats("Player2");

   GFWRITEVAR(pStream, uint32, mTotalFutureUnitCounts);
   GFWRITEVAR(pStream, uint32, mTotalDeadUnitCounts);
   GFWRITEVAR(pStream, uint32, mTotalFutureSquadCounts);
   GFWRITEVAR(pStream, uint32, mTotalDeadSquadCounts);

   GFWRITEARRAY(pStream, BEntityID, mGotoBases, uint8, 100);

   int8 weaponTypeCount = (int8)gDatabase.getNumberWeaponTypes();
   int8 damageTypeCount = (int8)gDatabase.getNumberDamageTypes();
   for (int8 i=0; i<weaponTypeCount; i++)
   {
//-- FIXING PREFIX BUG ID 6188
      const BWeaponType* pWeaponType = getWeaponTypeByID(i);
//--
      for (int8 j=0; j<damageTypeCount; j++)
      {
         float val = pWeaponType->getDamageModifier(j);
         if (val != 1.0f)
         {
            GFWRITEVAR(pStream, int8, i);
            GFWRITEVAR(pStream, int8, j);
            GFWRITEVAR(pStream, float, val);
         }
      }
   }
   int8 weaponType = -1;
   GFWRITEVAR(pStream, int8, weaponType);

   GFWRITEARRAY(pStream, float, mAbilityRecoverTimes, uint16, 500);

   if (!mpTechTree->save(pStream, saveType))
      return false;

   GFWRITEMARKER(pStream, cSaveMarkerPlayer3);
   gSaveGame.logStats("Player3");

   GFWRITEVAR(pStream, long, mMPID);
   GFWRITEVAR(pStream, long, mColorIndex);
   GFWRITEVAR(pStream, BPlayerID, mID);
   GFWRITEVAR(pStream, BPlayerID, mCoopID);
   GFWRITEVAR(pStream, BPlayerID, mScenarioID);
   GFWRITEVAR(pStream, long, mCivID);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEVAR(pStream, BPlayerState, mPlayerState);
   GFWRITEVAR(pStream, long, mLeaderID);
   //long mPopCount;
   GFWRITEVAR(pStream, long, mBountyResource);
   GFWRITEVAR(pStream, BEntityID, mRallyObject);
   GFWRITEVAR(pStream, int32, mStrength);
   GFWRITEVAR(pStream, float, mTributeCost);
   GFWRITECLASS(pStream, saveType, mRepairCost);
   GFWRITEVAR(pStream, float, mRepairTime);
   GFWRITEVAR(pStream, float, mHandicapMultiplier);
   GFWRITEVAR(pStream, float, mShieldRegenRate);
   GFWRITEVAR(pStream, DWORD, mShieldRegenDelay);
   GFWRITEVAR(pStream, float, mTotalCombatValue);
   GFWRITEVAR(pStream, float, mDifficulty);
   GFWRITEVAR(pStream, uint32, mGamePlayedTime);
   GFWRITEVAR(pStream, BPlayerID, mFloodPoofPlayer);
   GFWRITEVAR(pStream, int8, mPlayerType);
   GFWRITEVAR(pStream, int8, mSquadSearchAttempts);
   GFWRITEVAR(pStream, float, mWeaponPhysicsMultiplier);
   GFWRITEVAR(pStream, float, mAIDamageMultiplier);
   GFWRITEVAR(pStream, float, mAIDamageTakenMultiplier);
   GFWRITEVAR(pStream, float, mAIBuildSpeedMultiplier);

   //UTBitVector<8> mActiveResources;

   GFWRITEBITBOOL(pStream, mFlagRallyPoint);
   GFWRITEBITBOOL(pStream, mFlagBountyResource);
   GFWRITEBITBOOL(pStream, mFlagMinimapBlocked);
   GFWRITEBITBOOL(pStream, mFlagLeaderPowersBlocked);
   GFWRITEBITBOOL(pStream, mFlagDefeatedDestroy);

   GFWRITEVAR(pStream, long, mSquadAISearchIndex);
   GFWRITEVAR(pStream, long, mSquadAIWorkIndex);
   GFWRITEVAR(pStream, long, mSquadAISecondaryTurretScanIndex);

   GFWRITEMARKER(pStream, cSaveMarkerPlayer4);
   gSaveGame.logStats("Player4");

   return true;
}

//=============================================================================
//=============================================================================
bool BPlayer::load(BStream* pStream, int saveType)
{

   GFREADVECTOR(pStream, mLookAtPos);

   GFREADSTRING(pStream, BSimString, mName, 50);

   GFREADVAR(pStream, BRallyPoint, mRallyPoint);

   if (BPlayer::mGameFileVersion >= 2)
      GFREADCLASSPTR(pStream, saveType, mpStats);

   // Proto Objects
   int16 protoCount;
   GFREADVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 20000);
   BProtoObject tempProtoObject(cInvalidProtoObjectID);
   int16 loadedCount = 0;
   BProtoObjectID protoObjectID;
   GFREADVAL(pStream, int16, BProtoObjectID, protoObjectID);
   while (protoObjectID != -1)
   {
      loadedCount++;
      if (loadedCount > protoCount)
      {
         GFERROR("GameFile Error: protoObject loaded count");
         return false;
      }
      BProtoObjectID baseType;
      GFREADVAL(pStream, int16, BProtoObjectID, baseType);
      BProtoObject* pProtoObject = NULL;
      if (!gSaveGame.remapProtoObjectID(protoObjectID))
         pProtoObject = &tempProtoObject;
      else
         pProtoObject = mProtoObjects[protoObjectID];
      if (!pProtoObject->load(pStream, saveType, this))
         return false;
      if (!gSaveGame.remapProtoObjectID(baseType))
         baseType = protoObjectID;
      pProtoObject->setID(protoObjectID);
      pProtoObject->setBaseType(baseType);
      GFREADVAL(pStream, int16, BProtoObjectID, protoObjectID);
   }

   // Proto Squads
   GFREADVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 20000);
   BProtoSquad tempProtoSquad(cInvalidProtoSquadID);
   loadedCount = 0;
   BProtoSquadID protoSquadID;
   GFREADVAL(pStream, int16, BProtoSquadID, protoSquadID);
   while (protoSquadID != -1)
   {
      loadedCount++;
      if (loadedCount > protoCount)
      {
         GFERROR("GameFile Error: protoSquad loaded count");
         return false;
      }
      BProtoSquadID baseType;
      GFREADVAL(pStream, int16, BProtoSquadID, baseType);
      BProtoSquad* pProtoSquad = NULL;
      if (!gSaveGame.remapProtoSquadID(protoSquadID))
         pProtoSquad = &tempProtoSquad;
      else
         pProtoSquad = mProtoSquads[protoSquadID];
      if (!pProtoSquad->load(pStream, saveType, this))
         return false;
      if (!gSaveGame.remapProtoSquadID(baseType))
         baseType = protoSquadID;
      pProtoSquad->setID(protoSquadID);
      pProtoSquad->setBaseType(baseType);
      GFREADVAL(pStream, int16, BProtoSquadID, protoSquadID);
   }

   GFREADVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 5000);
   BProtoTech tempProtoTech;
   loadedCount = 0;
   BProtoTechID protoTechID;
   GFREADVAL(pStream, int16, BProtoTechID, protoTechID);
   while (protoTechID != -1)
   {
      loadedCount++;
      if (loadedCount > protoCount)
      {
         GFERROR("GameFile Error: protoTech loaded count");
         return false;
      }
      BProtoTech* pProtoTech = NULL;
      if (!gSaveGame.remapProtoTechID(protoTechID))
         pProtoTech = &tempProtoTech;
      else
         pProtoTech = mProtoTechs[protoTechID];
      if (!pProtoTech->load(pStream, saveType))
         return false;
      GFREADVAL(pStream, int16, BProtoTechID, protoTechID);
   }

   // Unique proto objects
   GFREADVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 1000);
   if (protoCount > 0)
   {
      if (!mUniqueProtoObjects.setNumber(protoCount))
         return false;
      for (int16 i=0; i<protoCount; i++)
         mUniqueProtoObjects[i].mProtoObject = NULL;
      for (int16 i=0; i<protoCount; i++)
      {
         int16 baseType;
         GFREADVAR(pStream, int16, baseType);
         if (!gSaveGame.remapProtoObjectID(baseType))
         {
            GFERROR("GameFile Error: invalid proto object base type %d", (int)baseType);
            return false;
         }
         BProtoObject* pBaseProto = getProtoObject(baseType);
         if (!pBaseProto)
         {
            GFERROR("GameFile Error: invalid proto object base type %d", (int)baseType);
            return false;
         }
         BProtoObject* pProtoObject = new BProtoObject(pBaseProto);
         if (!pProtoObject)
             return false;
         pProtoObject->setFlagUniqueInstance(true);
         pProtoObject->setID(getUniqueProtoObjectIDFromIndex(i));
         pProtoObject->setBaseType(baseType);
         BTactic* pTactic = pProtoObject->getTactic();
         if (pTactic)
         {
            pTactic->setPlayerID(mID);
            pTactic->setProtoObjectID(pProtoObject->getID());
         }
         mUniqueProtoObjects[i].mProtoObject = pProtoObject;
         if (!pProtoObject->load(pStream, saveType, this))
            return false;
      }
   }

   // Unique proto squads
   GFREADVAR(pStream, int16, protoCount);
   GFVERIFYCOUNT(protoCount, 1000);
   if (protoCount > 0)
   {
      if (!mUniqueProtoSquads.setNumber(protoCount))
         return false;
      for (int16 i=0; i<protoCount; i++)
         mUniqueProtoSquads[i].mProtoSquad=NULL;
      for (int16 i=0; i<protoCount; i++)
      {
         int16 baseType;
         GFREADVAR(pStream, int16, baseType);
         if (!gSaveGame.remapProtoSquadID(baseType))
         {
            GFERROR("GameFile Error: invalid proto squad base type %d", (int)baseType);
            return false;
         }
         BProtoSquad* pBaseProto = getProtoSquad(baseType);
         if (!pBaseProto)
         {
            GFERROR("GameFile Error: invalid proto squad base type %d", (int)baseType);
            return false;
         }
         mUniqueProtoSquads[i].mProtoSquad = NULL;
         BProtoSquad* pProtoSquad = new BProtoSquad(pBaseProto, this);
         if (!pProtoSquad)
            return false;
         pProtoSquad->setFlagUniqueInstance(true);
         pProtoSquad->setID(getUniqueProtoSquadIDFromIndex(i));
         pProtoSquad->setBaseType(baseType);
         mUniqueProtoSquads[i].mProtoSquad = pProtoSquad;
         if (!pProtoSquad->load(pStream, saveType, this))
            return false;
      }
   }

   GFREADCLASSARRAY(pStream, saveType, mPowerEntries, uint8, 250);

   GFREADARRAY(pStream, long, mAbilities, uint8, 250);
   for (uint i=0; i<mAbilities.size(); i++)
      gSaveGame.remapAbilityID(mAbilities[i]);

   //BSmallDynamicSimArray<BYTE> mSupportPowerStatus;

   int numProtoPowers = gSaveGame.getNumberProtoPowers();
   for (int i=0; i<numProtoPowers; i++)
   {
      DWORD rechargeTime;
      DWORD timeAvailable = 0;
      int useLimit;
      BPowerLevel powerLevel;
      GFREADVAR(pStream, DWORD, rechargeTime);
      GFREADVAR(pStream, int, useLimit);
      GFREADVAR(pStream, BPowerLevel, powerLevel);
      if (BPlayer::mGameFileVersion >= 5)
      {
         GFREADVAR(pStream, DWORD, timeAvailable);
      }
      int id = gSaveGame.getProtoPowerID(i);
      if (id != -1)
      {
         mPowerRechargeTime[id] = rechargeTime;
         mPowerUseLimit[id] = useLimit;
         mPowerLevel[id] = powerLevel;
         mPowerAvailableTime[id] = timeAvailable;
      }
   }

   GFREADCLASS(pStream, saveType, mResources);

   int rateCount = gSaveGame.getNumberRates();
   for (int i=0; i<rateCount; i++)
   {
      float amount, multiplier;
      GFREADVAR(pStream, float, amount);
      GFREADVAR(pStream, float, multiplier);
      int id = gSaveGame.getRateID(i);
      if (id != -1)
      {
         mpRateAmounts[id] = amount;
         mpRateMultipliers[id] = multiplier;
      }
   }

   GFREADCLASS(pStream, saveType, mTotalResources);
   GFREADCLASS(pStream, saveType, mResourceTrickleRate);

   //XUID mXUID;

   //BUser* mpUser;

   int popCount = gSaveGame.getNumberPops();
   for (int i=0; i<popCount; i++)
   {
      BPlayerPop pop;
      GFREADVAR(pStream, BPlayerPop, pop);
      int popID = gSaveGame.getPopID(i);
      if (popID != -1)
         mpPops[popID] = pop;
   }

   GFREADMARKER(pStream, cSaveMarkerPlayer1);

   //BAIFactoidManager* mpFactoidManager;
   //BAlertManager* mpAlertManager;
   //BHumanPlayerAITrackingData* mpTrackingData;

   bool hintEngine;
   GFREADVAR(pStream, bool, hintEngine);
   if (hintEngine)
   {
      if (!mpHintEngine)
      {
         mpHintEngine = new BHintEngine(this);
         mpHintEngine->initialize();
      }
      GFREADCLASSPTR(pStream, saveType, mpHintEngine);
   }

   //BEntityIDArrayArray mUnitsByProtoObject;
   //BEntityIDArrayArray mSquadsByProtoSquad;

   int16 protoIndex;
   GFREADVAR(pStream, int16, protoIndex);
   while (protoIndex != -1)
   {
      uint32 futureCount, deadCount;
      GFREADVAR(pStream, uint32, futureCount);
      GFREADVAR(pStream, uint32, deadCount);
      int id = gSaveGame.getProtoObjectID(protoIndex);
      if (id != -1)
      {
         mFutureUnitCounts[id] = futureCount;
         mDeadUnitCounts[id] = deadCount;
      }
      GFREADVAR(pStream, int16, protoIndex);
   }

   GFREADVAR(pStream, int16, protoIndex);
   while (protoIndex != -1)
   {
      uint32 futureCount, deadCount;
      GFREADVAR(pStream, uint32, futureCount);
      GFREADVAR(pStream, uint32, deadCount);
      int id = gSaveGame.getProtoSquadID(protoIndex);
      if (id != -1)
      {
         mFutureSquadCounts[id] = futureCount;
         mDeadSquadCounts[id] = deadCount;
      }
      GFREADVAR(pStream, int16, protoIndex);
   }

   GFREADMARKER(pStream, cSaveMarkerPlayer2);

   GFREADVAR(pStream, uint32, mTotalFutureUnitCounts);
   GFREADVAR(pStream, uint32, mTotalDeadUnitCounts);
   GFREADVAR(pStream, uint32, mTotalFutureSquadCounts);
   GFREADVAR(pStream, uint32, mTotalDeadSquadCounts);

   GFREADARRAY(pStream, BEntityID, mGotoBases, uint8, 100);

   int8 weaponType = -1;
   GFREADVAR(pStream, int8, weaponType);
   while (weaponType != -1)
   {
      int8 damageType;
      GFREADVAR(pStream, int8, damageType);

      float modifier;
      GFREADVAR(pStream, float, modifier);

      BWeaponType* pWeaponType = getWeaponTypeByID(gSaveGame.getWeaponType(weaponType));
      if (pWeaponType)
         pWeaponType->setDamageModifier(gSaveGame.getDamageType(damageType), modifier);

      GFREADVAR(pStream, int8, weaponType);
   }

   GFREADARRAY(pStream, float, mAbilityRecoverTimes, uint16, 500);

   if (!mpTechTree->load(pStream, saveType))
      return false;

   GFREADMARKER(pStream, cSaveMarkerPlayer3);

   GFREADVAR(pStream, long, mMPID);
   GFREADVAR(pStream, long, mColorIndex);
   GFREADVAR(pStream, BPlayerID, mID);
   GFREADVAR(pStream, BPlayerID, mCoopID);
   GFREADVAR(pStream, BPlayerID, mScenarioID);

   GFREADVAR(pStream, long, mCivID);
   if (mCivID != -1)
   {
      gSaveGame.remapCivID(mCivID);
      if (mCivID == -1)
      {
         GFERROR("GameFile Error: invalid civ");
         return false;
      }
   }
   
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADVAR(pStream, BPlayerState, mPlayerState);

   GFREADVAR(pStream, long, mLeaderID);
   if (mLeaderID != -1)
   {
      gSaveGame.remapLeaderID(mLeaderID);
      if (mLeaderID == -1)
      {
         GFERROR("GameFile Error: invalid leader");
         return false;
      }
   }

   //long mPopCount;
   GFREADVAR(pStream, long, mBountyResource);
   gSaveGame.remapResourceID(mBountyResource);

   GFREADVAR(pStream, BEntityID, mRallyObject);
   GFREADVAR(pStream, int32, mStrength);
   GFREADVAR(pStream, float, mTributeCost);
   GFREADCLASS(pStream, saveType, mRepairCost);
   GFREADVAR(pStream, float, mRepairTime);
   GFREADVAR(pStream, float, mHandicapMultiplier);
   GFREADVAR(pStream, float, mShieldRegenRate);
   GFREADVAR(pStream, DWORD, mShieldRegenDelay);
   GFREADVAR(pStream, float, mTotalCombatValue);
   GFREADVAR(pStream, float, mDifficulty);
   GFREADVAR(pStream, uint32, mGamePlayedTime);
   GFREADVAR(pStream, BPlayerID, mFloodPoofPlayer);
   GFREADVAR(pStream, int8, mPlayerType);
   if (BPlayer::mGameFileVersion >= 6)
      GFREADVAR(pStream, int8, mSquadSearchAttempts);
   if (BPlayer::mGameFileVersion >= 3)
      GFREADVAR(pStream, float, mWeaponPhysicsMultiplier);
   if (BPlayer::mGameFileVersion >= 4)
   {
      GFREADVAR(pStream, float, mAIDamageMultiplier);
      GFREADVAR(pStream, float, mAIDamageTakenMultiplier);
   }
   if (BPlayer::mGameFileVersion >= 7)
      GFREADVAR(pStream, float, mAIBuildSpeedMultiplier);

   //UTBitVector<8> mActiveResources;

   GFREADBITBOOL(pStream, mFlagRallyPoint);
   GFREADBITBOOL(pStream, mFlagBountyResource);
   GFREADBITBOOL(pStream, mFlagMinimapBlocked);
   GFREADBITBOOL(pStream, mFlagLeaderPowersBlocked);
   GFREADBITBOOL(pStream, mFlagDefeatedDestroy);

   if (BPlayer::mGameFileVersion >= 8)
   {
      GFREADVAR(pStream, long, mSquadAISearchIndex);
      GFREADVAR(pStream, long, mSquadAIWorkIndex);
   }

   if (BPlayer::mGameFileVersion >= 9)
   {
      GFREADVAR(pStream, long, mSquadAISecondaryTurretScanIndex);
   }

   GFREADMARKER(pStream, cSaveMarkerPlayer4);

   return true;
}
