//==============================================================================
// world.cpp
//
// Copyright (c) Ensemble Studios, 2005-2008
//==============================================================================

// Includes
#include "common.h"
#include "world.h"

// xcore
#include "threading\workDistributor.h"


// xgame
#include "action.h"
#include "actionmanager.h"
#include "ai.h"
#include "aimanager.h"
#include "bidMgr.h"
#include "unitactionmove.h"
#include "unitactionrangedattack.h"
#include "army.h"
#include "commandmanager.h"
#include "configsGame.h"
#include "database.h"
#include "EntityGrouper.h"
#include "formationmanager.h"
#include "game.h"
#include "generaleventmanager.h"
#include "kb.h"
#include "object.h"
#include "objectmanager.h"
#include "obstructionmanager.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "platoon.h"
#include "player.h"
#include "team.h"
#include "protoobject.h"
#include "protosquad.h"
#include "SelectionManager.h"
#include "SimOrderManager.h"
#include "squadactionattack.h"
#include "squadactionmove.h"
#include "squadPlotter.h"
#include "squadlosvalidator.h"
#include "statsManager.h"
#include "syncmacros.h"
#include "techtree.h"
#include "UnitQuery.h"
#include "user.h"
#include "usermanager.h"
#include "visiblemap.h"
#include "worldsoundmanager.h"
#include "dopple.h"
#include "placementrules.h"
#include "civ.h"
#include "textvisualmanager.h"
#include "tactic.h"
#include "entityScheduler.h"
#include "triggermanager.h"
#include "lightEffectManager.h"
#include "particlegateway.h"
#include "recordgame.h"
#include "battle.h"
#include "alert.h"
#include "prototech.h"
#include "physicsinfo.h"
#include "renderControl.h"
#include "terrainsimrep.h"
#include "timermanager.h"
#include "objectAnimEventTagQueue.h"
#include "corpsemanager.h"
#include "modemanager.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"
#include "fatalitymanager.h"
#include "damagetemplatemanager.h"
#include "commands.h"
#include "gamesettings.h"
#include "econfigenum.h"
#include "transitionManager.h"
#include "powermanager.h"
#include "protoimpacteffect.h"
#include "minimap.h"
#include "timingtracker.h"
#include "mpcommheaders.h"
#include "commlog.h"
#include "unitactionscaleLOS.h"
#include "scoremanager.h"
#include "campaignmanager.h"
#include "hintengine.h"
#include "skullmanager.h"
#include "simhelper.h"
#include "modegame.h"
#include "physicsgroundvehicleaction.h"

// terrain
#include "terrain.h"
#include "TerrainDynamicAlpha.h"
#include "TerrainTexturing.h"
#include "TerrainLitDecals.h"

// xgamerender
#include "lightVisualManager.h"
#include "decalManager.h"
#include "sceneLightManager.h"
#include "terrainimpactdecal.h"

// xsystem
#include "mathutil.h"
#include "math\collision.h"
#include "Quaternion.h"

// xrender
#include "renderThread.h"
#include "renderhelperthread.h"

// xvisual
#include "xvisual.h"

// xphysics
#include "xphysics.h"
#include "physics.h"
#include "physicsworld.h"
#include "configsphysics.h"
#include "physicsCollision.h"
#include "shape.h"
#include <Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h>

//Live
#include "liveSystem.h"
#include "liveSession.h"

// xvince
#include "vincehelper.h"

// UI
#include "decalManager.h"
#include "uimanager.h"

#include "camera.h"

#include "binkInterface.h"

#ifdef SYNC_World
#include "hash\crc.h"
#include "archiveManager.h"
#endif

#ifndef BUILD_FINAL
#include "tracerecording.h"
#endif

#include "timelineprofiler.h"


GFIMPLEMENTVERSION(BWorld, 14);
enum
{
   cSaveMarkerWorld1=10000,
   cSaveMarkerWorld2,
   cSaveMarkerWorld3,
   cSaveMarkerWorld4,
   cSaveMarkerWorld5,
   cSaveMarkerPlayers,
   cSaveMarkerTeams,
   cSaveMarkerSimOrder,
   cSaveMarkerUnitOpp,
   cSaveMarkerPathMoveData,
   cSaveMarkerPlatoons,
   cSaveMarkerDopples,
   cSaveMarkerProjectiles,
   cSaveMarkerAirSpots,
   cSaveMarkerArmies,
   cSaveMarkerSquads,
   cSaveMarkerUnits,
   cSaveMarkerObjectiveManager,
   cSaveMarkerGeneralEvents,
   cSaveMarkerTriggers,
   cSaveMarkerVisibilty,
   cSaveMarkerScoreManager,
   cSaveMarkerStoredAnimEventManager,
   cSaveMarkerEntityScheduler,
   cSaveMarkerCollectiblesManager,
   cSaveMarkerObjects,
};

// Global pointer to the current world
BWorld* gWorld=NULL;
bool gWorldReset=false;

bool gEnableSubUpdating = false;
bool gDecoupledUpdate = false;

const DWORD cAmountOfSubUpdates = 3;
const long cSubUpdateTimeInMsecs = 33;

#ifdef DECOUPLED_UPDATE

enum
{
   cSubUpdateStart,

   cSubUpdatePreAsyncStart,
   cSubUpdatePreAsyncCommandProcess,
   cSubUpdatePreAsyncTeam,
   cSubUpdatePreAsyncPlayer,
   cSubUpdatePreAsyncKB,
   cSubUpdatePreAsyncAI,
   cSubUpdatePreAsyncCommandService,
   cSubUpdatePreAsyncCinematic,
   cSubUpdatePreAsyncCorpse,
   cSubUpdatePreAsyncFatality,
   cSubUpdatePreAsyncDamageOverTime,
   cSubUpdatePreAsyncArmy,
   cSubUpdatePreAsyncPlatoon,
   cSubUpdatePreAsyncSquad,
   cSubUpdatePreAsyncSquadPlotter,
   cSubUpdatePreAsyncUnit,
   cSubUpdatePreAsyncObject,
   cSubUpdatePreAsyncDopple,
   cSubUpdatePreAsyncProjectile,

   cSubUpdateAsyncStart,
   cSubUpdateAsyncFlush,

   cSubUpdatePostAsyncStart,
   cSubUpdatePostAsyncPhysicsBreakOffObjects,
   cSubUpdatePostAsyncUnits,
   cSubUpdatePostAsyncObjects,
   cSubUpdatePostAsyncDopples,
   cSubUpdatePostAsyncProjectiles,
   cSubUpdatePostAsyncMinimap,
   cSubUpdatePostAsyncEntityScheduler,
   cSubUpdatePostAsyncSoundManager,
   cSubUpdatePostAsyncBattleManager,
   cSubUpdatePostAsyncPowerManager,
   cSubUpdatePostAsyncTriggerManager,
   cSubUpdatePostAsyncPhysics,
   cSubUpdatePostAsyncLightManager,
   cSubUpdatePostAsyncMinimapBounds,
   cSubUpdatePostAsyncPathingLimiter,
   cSubUpdatePostAsyncExplorationGroups,
   cSubUpdatePostAsyncCommitMinimap,
   cSubUpdatePostAsyncEnd,

   cSubUpdateDelay,

   cSubUpdateEnd
};

#define SUBUPDATESKIP(name,label) \
   if (gDecoupledUpdate) \
   { \
      if (mDecoupledUpdateRender || canRenderInterpolatedFrame()) \
      { \
         mDecoupledUpdateRender = true; \
         goto LabelSubUpdate##label; \
      } \
      if (mSubUpdateSection < cSubUpdate##name) \
         goto LabelSubUpdate##label; \
   } \
   LabelSubUpdate##name:

#define SUBUPDATERETURN(name) \
   if (gDecoupledUpdate) \
   { \
      if (mDecoupledUpdateRender || canRenderInterpolatedFrame()) \
      { \
         mDecoupledUpdateRender = true; \
         return; \
      } \
      if (mSubUpdateSection < cSubUpdate##name) \
         return; \
   } \
   LabelSubUpdate##name:

#define SUBUPDATENEXT \
   { \
      if (gDecoupledUpdate) \
      { \
         mSubUpdateSection++; \
         mSubUpdateSectionItem = -1; \
      } \
   }

#define SUBUPDATERESUME(name) \
   if (gDecoupledUpdate && mSubUpdateSection >= cSubUpdate##name) \
      goto LabelSubUpdate##name;

#define SUBUPDATELABEL(label) \
   LabelSubUpdate##label:

#define SUBUPDATECANRENDERBREAK \
   if (gDecoupledUpdate) \
   { \
      if (canRenderInterpolatedFrame()) \
      { \
         mDecoupledUpdateRender = true; \
         break; \
      } \
   }

#define SUBUPDATECANRENDERRETURN \
   if (gDecoupledUpdate) \
   { \
      if (canRenderInterpolatedFrame()) \
      { \
         mDecoupledUpdateRender = true; \
         return; \
      } \
   }

#define SUBUPDATECANRENDERSKIP(label) \
   if (gDecoupledUpdate) \
   { \
      if (canRenderInterpolatedFrame()) \
      { \
         mDecoupledUpdateRender = true; \
         goto LabelSubUpdate##label; \
      } \
   }

#else

#define SUBUPDATESKIP(name,label)
#define SUBUPDATERETURN(name)
#define SUBUPDATENEXT { }
#define SUBUPDATERESUME(name)
#define SUBUPDATELABEL(label)
#define SUBUPDATECANRENDERBREAK
#define SUBUPDATECANRENDERRETURN
#define SUBUPDATECANRENDERSKIP(label)

#endif

#ifdef UNITOFFENDERS
const uint cNumberUnitOffenders=20;
#endif

//==============================================================================
//==============================================================================
void setWorldReset(bool val)
{
   gWorldReset=val;
}

//==============================================================================
// BWorld::BWorld
//==============================================================================
BWorld::BWorld() :
   mpObjectManager(NULL),
   mpCommandManager(NULL),
   mpPhysicsWorld(NULL),
   mEntityGrouper(NULL),
   mpSkyVisual(NULL),
   mCachedUnitsToTrack(0, 100),
   mFlagUnitCache(false),
   mFlagObstructionCache(false),
   mFlagQuickBuild(false),
   mFlagGameOver(false),
   mFlagSkipGameOverCheck(false),
   mFlagStartedUpdate(false),
   mFlagMultiplayerSim(false),
   mFlagCoop(false),
   mFlagPlayableBounds(false),
   mFlagOutsideBoundsBlocked(false),
   mFlagNoFogSim(false),
   mFlagNoFogVis(false),
   mFlagShowSkirt(false),
   mFlagAllVisible(false),
   mpObjectiveManager(NULL),
   mpChatManager(NULL),
   mpDesignObjectManager(NULL),
   mBuildingPlacementDecal(-1),
   mpCinematicManager(NULL),
   mpBattleManager(NULL),
   mpPowerManager(NULL),
   mpAIManager(NULL),
   mpTransitionManager(NULL),
   mpStoredEventTagManager(NULL),
   mNextCustomCommandID(0),
   mSimBoundsMinX(0.0f),
   mSimBoundsMinZ(0.0f),
   mSimBoundsMaxX(cMaximumFloat),
   mSimBoundsMaxZ(cMaximumFloat),
   mWorldID(cWorldNone),
   mGameOverCountdown(0.0f),
   mSubUpdate(0),
   mSubUpdateSection(-1),
   mSubUpdateSectionItem(-1),
   mPlayerColorCategory(cPlayerColorCategorySkirmish),
   mLastSubUpdateUnit(cInvalidObjectID),
   mAdjustedSubUpdateTime(cSubUpdateTimeInMsecs),
   mAccumulatedTimeError(0.0f),
   mAutoSaveTime(0.0f),
   mScenarioLoadObjectHighWaterMark(0),
   mLowDetailAnimRefCount(0),
   mTerrainMax(1.0f,1.0f,1.0f),
   mFirstUpdate(true)
{
#ifndef BUILD_FINAL
   mPlayerColorLoadCount = 0;
#endif   

   for (uint i = 0; i < cMaximumSupportedTeams; i++)
   {
      for (uint j = 0; j < cMaximumSupportedTeams; j++)
      {
         if (i == j)
         {
            mTeamRelationType[i][j] = cRelationTypeAlly;
         }
         else
         {
            mTeamRelationType[i][j] = cRelationTypeNeutral;
         }
      }
   }

   for (uint i=0; i<4; i++)
   {
      mOutsideOfBoundsBlockerCenter[i]=cInvalidVector;
      mOutsideOfBoundsBlockerSize[i]=cInvalidVector;
   }
   
   Utils::ClearObj(mPlayerColorIndices);

   #ifdef UNITOFFENDERS
   mUnitOffenders.clear();
   mFlagClearUnitOffenders=true;
   #endif

   mpTimingTracker = NULL;

   #ifdef DECOUPLED_UPDATE
   mDecoupledUpdateElapsed = 0.0;
   mDecoupledUpdateLastSubElapsed = 0.0;
   mDecoupledUpdateAvgUpdateTime = 0.0;
   mDecoupledUpdateRenderTime = 0.025;
   mDecoupledUpdateTotalUpdateTime = 0.0;
   mDecoupledUpdateStartingSubUpdateCurrent = 0;
   mDecoupledUpdateStartingSubUpdatePrevious = 0;
   mDecoupledUpdateTimesCurrent = &mDecoupledUpdateTimes1;
   mDecoupledUpdateTimesPrevious = &mDecoupledUpdateTimes2;
   mDecoupledUpdateLength = 0.0f;
   mDecoupledUpdateSkipRenders = false;
   mDecoupledUpdateEnd = false;
   mDecoupledUpdateRender = false;
   mDecoupledFinishedAnUpdate = false;
   #endif
}

//==============================================================================
// BWorld::~BWorld
//==============================================================================
BWorld::~BWorld()
{
   resetAllPhysicsBreakOffObjects();
   resetAllPhysicsVehicleTerrainCollisions();
}

//==============================================================================
// BWorld::determinePlayerColorCategory
//==============================================================================
void BWorld::determinePlayerColorCategory(void)
{
//-- FIXING PREFIX BUG ID 5915
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   if(!pSettings)
      return;

   //BMPSession* pMPSession = gLiveSystem->getMPSession();

   //const BOOL isMPRunning = (pMPSession ? pMPSession->isGameRunning() : FALSE);

   long gameType = -1;
   pSettings->getLong(BGameSettings::cGameType, gameType);

   bool forceSPCColors = false;

#ifndef BUILD_FINAL   
   if (gConfig.isDefined(cConfigForceSPCColors))
      forceSPCColors = true;
#endif   

   // If this is a campaign map, and multiplayer is not active, then enable the SPC player colors
   // (which also disables the xform textures when present).   
   if ((forceSPCColors) || (((gameType == BGameSettings::cGameTypeCampaign) || (gameType == BGameSettings::cGameTypeScenario)) /*&& (!isMPRunning)*/))
      setPlayerColorCategory(cPlayerColorCategorySPC);
   else
      setPlayerColorCategory(cPlayerColorCategorySkirmish);
}

//==============================================================================
// BWorld::setup
//==============================================================================
bool BWorld::setup()
{
   #ifndef BUILD_FINAL
   mTimeStats.init();
   #endif

   SCOPEDSAMPLE(BWorld_setup)   

   if (!gPather.init())
      return false;

   // Game timers
   gTimerManager.initialize();

   // Setup the objective manager
   mpObjectiveManager = new BObjectiveManager();
   if( !mpObjectiveManager )
   {
      BFAIL( "Unable to create BObjectiveManager!  Out of memory?" );
      return( false );
   }
   if( !mpObjectiveManager->init() )
   {
      BFAIL( "Unable to initialize BObjectiveManager!" );
      return( false );
   }

   // World sound manager
   mpWorldSoundManager=new BWorldSoundManager();
   if(!mpWorldSoundManager || !mpWorldSoundManager->setup())
   {
      BFAIL("Unable to init BWorldSoundManager");
      return false;
   }

   //-- setup the Object Manager
   mpObjectManager = new BObjectManager();
   if (!mpObjectManager)
   {
      BFAIL("Unable to create BObjectManager!  Out of memory?");
      return (false);
   }
   if (!mpObjectManager->init())
   {
      BFAIL("Unable to initialize BObjectManager!");
      return (false);
   }
   
   // Setup the chat manager
   mpChatManager = new BChatManager();
   if( !mpChatManager )
   {
      BFAIL( "Unable to create ChatManager!  Out of memory?" );
      return( false );
   }
   if( !mpChatManager->init() )
   {
      BFAIL( "Unable to initialize BChatManager!" );
      return( false );
   }

   // Clear the talking heads
   for(int i=0; i<mTalkingHeads.getNumber(); i++)
      delete mTalkingHeads[i];
   mTalkingHeads.clear();

   // Setup the Hint manager
   mpHintManager = new BHintManager();
   if( !mpHintManager )
   {
      BFAIL( "Unable to create HintManager!  Out of memory?" );
      return( false );
   }
   if( !mpHintManager->init() )
   {
      BFAIL( "Unable to initialize BHintManager!" );
      return( false );
   }

   //-- Setup the Design Objects Manager
   mpDesignObjectManager = new BDesignObjectManager();
   if( !mpDesignObjectManager )
   {
      BFAIL( "Unable to create BDesignObjectManager!  Out of memory?" );
      return( false );
   }
   if( !mpDesignObjectManager->init() )
   {
      BFAIL( "Unable to initialize BDesignObjectManager!" );
      return( false );
   }

   //-- Setup general event manager
   if( !gGeneralEventManager.init() )
   {
      BFAIL( "Unable to initialize BGeneralEventManager!" );
      return( false );
   }

   //-- Setup the Battle Mananger
   mpBattleManager = new BBattleManager();
   if (!mpBattleManager)
   {
      BFAIL( "Unable to create BBattleManager!  Out of memory?" );
      return( false );
   }
   if( !mpBattleManager->init() )
   {
      BFAIL( "Unable to initialize BBattleManager!" );
      return( false );
   }
   //-- Hookup Music Manager as listener
   mpBattleManager->setListener(reinterpret_cast<IBattleManagerListener*>(mpWorldSoundManager->getMusicManager()));

   //-- Setup the Power Manager
   mpPowerManager = new BPowerManager();
   if (!mpPowerManager)
   {
      BFAIL("Unable to create BPowerManager!  Out of memory?");
      return (false);
   }

   mpAIManager = new BAIManager();
   if (!mpAIManager)
   {
      BFAIL("Unalbe to create AI manager!  Out of memory?");
      return (false);
   }

   //-- setup the Cinematic Manager
   mpCinematicManager = new BCinematicManager();
   if (!mpCinematicManager)
   {
      BFAIL("Unable to create BCinematicManager!  Out of memory?");
      return (false);
   }

   //-- setup the Transition Manager
   mpTransitionManager = new BTransitionManager();
   if (!mpTransitionManager)
   {
      BFAIL("");
      return( false );
   }

   //-- setup the Cinematic Manager
   mpStoredEventTagManager = new BStoredAnimEventManager();
   if (!mpStoredEventTagManager)
   {
      BFAIL("Unable to create anim tag storage!  Out of memory?");
      return (false);
   }

   //-- Setup the Entity Grouper
   mEntityGrouper=new BEntityGrouper();
   if (!mEntityGrouper)
   {
      BFAIL( "Unable to create BEntityGrouper!  Out of memory?" );
      return( false );
   }

   //-- command manager
   mpCommandManager=new BCommandManager();
   if(!mpCommandManager || !mpCommandManager->setup())
      return false;

   //-- setup the Pathing Limiter
   mpPathingLimiter = new BPathingLimiter();
   if (!mpPathingLimiter)
      return (false);
   mpPathingLimiter->initPlayerPathHist();

   //-- update/timing values
   mUpdateNumber=0;
   mLastUpdateTime=0;
   mLastUpdateLength=0;
   mLastUpdateLengthFloat=0.0f;
   mGametime=0;
   mSubUpdate=0;
   #ifdef DECOUPLED_UPDATE
   mSubUpdateSection = cSubUpdateEnd;
   #else
   mSubUpdateSection = 0;
   #endif
   mSubUpdateSectionItem = -1;
   mFirstSubUpdate=0;
   mSubUpdateBeganTime=0;
   mSubUpdateLastTime=0;

   mGametimeFloat=0.0f;
   mTotalRealtime=0.0f;
   mSubTotalRealtime=0.0f;
   mLastUpdateRealtime=0;
   mLastSubUpdateRealtime=0;
   mLastUpdateRealtimeDelta=0.0;
   mLastTimerFrequency=0.0;
   mAccumulatedTimeError=0.0f;
   mAutoSaveTime=0.0f;

   mScenarioLoadObjectHighWaterMark=0;

   mLastSubUpdateUnit=cInvalidObjectID;
   mAdjustedSubUpdateTime=cSubUpdateTimeInMsecs;

   mFogStartX  = -1;
   mFogStartZ  = -1;
   mFogEndZ    = -1;
  
   //-- physics
   mpPhysicsWorld = new BPhysicsWorld();
   long numPhysicsThreads = 1;
   gConfig.get(cConfigNumPhysicsThreads, &numPhysicsThreads);
   if (!mpPhysicsWorld || !mpPhysicsWorld->setup(numPhysicsThreads > 1) )
      return false;

   // Player colors
   for (uint category = 0; category < cMaxPlayerColorCategories; category++)
      for(uint playerIndex = 0; playerIndex < cMaximumSupportedPlayers; playerIndex++)
      {
         mPlayerColors[category][playerIndex] = gDatabase.getPlayerColor(static_cast<BPlayerColorCategory>(category), playerIndex);
         mPlayerColorIndices[category][playerIndex] = playerIndex;
      }
      
   #ifndef BUILD_FINAL
   mPlayerColorLoadCount = gDatabase.getPlayerColorLoadCount();      
   #endif   

   // Hookup to visual manager
   gVisualManager.addVisualHandler(this);

   setFlagQuickBuild(false);
   #ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigQuickBuild))
      setFlagQuickBuild(true);
   #endif

   setFlagGameOver(false);
   mGameOverCountdown = 0.0f;

   if(gLiveSystem->isMultiplayerGameActive()  || (gRecordGame.isPlaying() && gRecordGame.isMultiplayer()))
      setFlagMultiplayerSim(true);
   else
      setFlagMultiplayerSim(false);

   //-- Subupdating
   if (!gModeManager.inModeGame() || (mFlagMultiplayerSim && !gConfig.isDefined(cConfigMPSubUpdating)) || gConfig.isDefined(cConfigWowRecord))
      gEnableSubUpdating = false;
   else
      gEnableSubUpdating = gConfig.isDefined(cConfigEnableSubUpdating);

   mDynamicSubUpdating = (gEnableSubUpdating && gConfig.isDefined(cConfigDynamicSubUpdating));

   #ifdef DECOUPLED_UPDATE
   gDecoupledUpdate = (gEnableSubUpdating && gConfig.isDefined(cConfigDecoupledUpdate));
   #endif

   mAlternateSubUpdating = gConfig.isDefined(cConfigAlternateSubUpdating);
   mDynamicSubUpdateTime = gConfig.isDefined(cConfigDynamicSubUpdateTime);

   mAdjustedSubUpdateTime = cSubUpdateTimeInMsecs;

   //-- Init Entity Scheduler
   gEntityScheduler.init();

   //-- Load Corpse Object
   mTempObjectID = gDatabase.getProtoObject("fx_impact_effect_01");

   if (gConfig.isDefined(cConfigFlashGameUI))
      mBuildingPlacementDecal = gDecalManager.createDecal();

   gTerrain.clearExclusionObjects();

   mFlagPlayableBounds=false;
   mFlagOutsideBoundsBlocked=false;
   mFlagNoFogSim=false;
   mFlagNoFogVis=false;
   mFlagShowSkirt=!gConfig.isDefined(cConfigNoTerrainSkirt);
   mFlagAllVisible=false;

   mNextCustomCommandID=0;

   mSimBoundsMinX=0.0f;
   mSimBoundsMinZ=0.0f;
   mSimBoundsMaxX=cMaximumFloat;
   mSimBoundsMaxZ=cMaximumFloat;

   for (uint i=0; i<4; i++)
   {
      mOutsideOfBoundsBlockerCenter[i]=cInvalidVector;
      mOutsideOfBoundsBlockerSize[i]=cInvalidVector;
   }

   gCorpseManager.reset();

   gDamageTemplateManager.init(this);

   gTerrainEffectManager.clear();

   resetExplorationGroups();
   
   determinePlayerColorCategory();

   #ifdef SYNC_World
   archiveSyncCheck();
   #endif

   mpTimingTracker = NULL;

   #ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate)
   {
      mDecoupledUpdateEnd = true;
      mDecoupledUpdateTimes1.clear();
      mDecoupledUpdateTimes2.clear();
      mDecoupledUpdateTimesCurrent = &mDecoupledUpdateTimes1;
      mDecoupledUpdateTimesPrevious = &mDecoupledUpdateTimes2;
      mDecoupledUpdateStartingSubUpdateCurrent = 0;
      mDecoupledUpdateStartingSubUpdatePrevious = 0;
      mDecoupledUpdateLength = 0.0f;
      mDecoupledUpdateElapsed = 0.0;
      mDecoupledUpdateLastSubElapsed = 0.0;
      mDecoupledUpdateAvgUpdateTime = 0.0;
      mDecoupledUpdateSkipRenders = false;
      mDecoupledUpdateTimer.start();
      mDecoupledUpdateRenderTime=0.025;
      mDecoupledUpdateTotalUpdateTime=0.0;
      mDecoupledUpdateRender=false;
   }
   #endif

   mFirstUpdate = true;

   if (gEnableSubUpdating && mDynamicSubUpdating)
      gEnableSubUpdating = false;

   if (mSubUpdateEnabledTimer.isStarted())
      mSubUpdateEnabledTimer.stop();
   mSubUpdateEnabledNumber = 0;
   mNextEnableSubUpdating = gEnableSubUpdating;
   #ifdef DECOUPLED_UPDATE
   mNextDecoupledUpdate = gDecoupledUpdate;
   #endif
   mNextAlternateSubUpdating = mAlternateSubUpdating;
   mNextDynamicSubUpdateTime = mDynamicSubUpdateTime;
   mChangeSubUpdatePendnig = false;

   return true;
}

//==============================================================================
// BWorld::initTerrain
//==============================================================================
bool BWorld::initTerrain(const char* terrainName)
{
   mTerrainName=terrainName;

   //-- create the sim terrain rep in physics
   mpPhysicsWorld->markForWrite(true);
   gTerrainSimRep.createPhysics();
   mpPhysicsWorld->markForWrite(false);

   //-- visiblemap
   gVisibleMap.initMap(gTerrainSimRep.getNumXDataTiles(), gTerrainSimRep.getNumXDataTiles());

   mSimBoundsMinX=0.0f;
   mSimBoundsMinZ=0.0f;
   mSimBoundsMaxX=cMaximumFloat;
   mSimBoundsMaxZ=cMaximumFloat;
   for (uint i=0; i<4; i++)
   {
      mOutsideOfBoundsBlockerCenter[i]=cInvalidVector;
      mOutsideOfBoundsBlockerSize[i]=cInvalidVector;
   }
   mFlagPlayableBounds=false;
   mFlagOutsideBoundsBlocked=false;

   gTerrain.setRenderSkirt(!gConfig.isDefined(cConfigNoTerrainSkirt));


   return true;
}
//==============================================================================
// BWorld::initPathing
//==============================================================================
bool BWorld::initPathing(const char* pathingName)
{
   return true;
}

//==============================================================================
// BWorld::addTalkingHead
//==============================================================================
BTalkingHead* BWorld::addTalkingHead(const char * filename, int talkingHeadID, bool preload)
{
   BTalkingHead* pTalkingHead = new BTalkingHead();
   //pTalkingHead->mFilename.set(filename);
   pTalkingHead->mFilename.format("game:\\%s", filename);
   pTalkingHead->mID = talkingHeadID;
   pTalkingHead->mPreload = preload;

   mTalkingHeads.pushBack(pTalkingHead);
   return mTalkingHeads[mTalkingHeads.getNumber() - 1];
}

//==============================================================================
// BWorld::playTalkingHead
//==============================================================================
BBinkVideoHandle BWorld::playTalkingHead(int talkingHeadID, BCueIndex soundIndex, BBinkVideoStatus* pStatusCallback)
{
   for (int i=0; i<mTalkingHeads.getNumber(); i++)
   {
      BTalkingHead* pTalkingHead = mTalkingHeads[i];
      if (pTalkingHead && (pTalkingHead->mID==talkingHeadID))
      {
         float xScale = 1.0;
         float yScale = 1.0;
         float xOffsetScale = 0.067f; //91
         float yOffsetScale = 0.073f;

         if( gRender.getAspectRatioMode() == BRender::cAspectRatioMode4x3 )
         {
            xScale = yScale = BD3D::mD3DPP.BackBufferWidth / 1280.0f;
            xOffsetScale = 0.065f;
            yOffsetScale = 0.07f;
         }

         float xOffset = BD3D::mD3DPP.BackBufferWidth * xOffsetScale;
         float yOffset = BD3D::mD3DPP.BackBufferHeight * yOffsetScale;

         BBinkInterface::BLoadParams lp;
         lp.mFilename.set(pTalkingHead->mFilename.getPtr());
         lp.mCaptionDirID = -1;
         lp.mMaskTextureFilename.set("ui\\flash\\shared\\textures\\misc\\circlemask");
         lp.mWidthScale = xScale;
         lp.mHeightScale = yScale;
         lp.mXOffset = xOffset;
         lp.mYOffset = yOffset;
         lp.mLoopVideo = false;
         lp.mFullScreen = false;
         lp.mpStatusCallback = pStatusCallback;
         lp.mSoundIndex = soundIndex;
         if (pTalkingHead->mPreloadedData)
         {
            // Preloaded videos are in memory, so we pass the address of the video in the bufSize variable to the bink player
            lp.mIOBufSize = (int)pTalkingHead->mPreloadedData;
            lp.mPreloaded = true;
            
            // We're letting bink take ownership of this data for now, so NULLing it out.
            pTalkingHead->mPreloadedData = NULL;
         }

         return gBinkInterface.loadActiveVideo(lp);//pTalkingHead->mFilename.getPtr(), -1, "", "ui\\flash\\shared\\textures\\misc\\circlemask", xScale, yScale, xOffset, YOffset);
      }
   }
   return cInvalidVideoHandle;
}

//==============================================================================
// BWorld::getTalkingHeadID
//==============================================================================
int BWorld::getTalkingHeadID(const char* pFileName) const
{
   BSimString fullname;
   fullname.format("game:\\%s", pFileName);
   for (int i=0; i<mTalkingHeads.getNumber(); i++)
   {
      const BTalkingHead* pTalkingHead = mTalkingHeads[i];
      if (pTalkingHead && pTalkingHead->mFilename == fullname)
         return pTalkingHead->mID;
   }
   return -1;
}

//==============================================================================
// BWorld::getNextTalkingHeadID
//==============================================================================
int BWorld::getNextTalkingHeadID() const
{
   int maxID = -1;
   for (int i=0; i<mTalkingHeads.getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 5917
      const BTalkingHead* pTalkingHead = mTalkingHeads[i];
//--
      if (pTalkingHead && pTalkingHead->mID > maxID)
         maxID = pTalkingHead->mID;
   }
   return (maxID + 1);
}

//==============================================================================
//==============================================================================
void BWorld::preloadTalkingHeadVideos()
{
   for (int i=0; i<mTalkingHeads.getNumber(); i++)
   {
      BTalkingHead* pTalkingHead = mTalkingHeads[i];
      if (pTalkingHead && pTalkingHead->mPreload)
      {
         BASSERT(pTalkingHead->mPreloadedData == NULL);
         pTalkingHead->mPreloadedData = gBinkInterface.preloadBinkFile(pTalkingHead->mFilename);
      }
   }
}

//==============================================================================
//==============================================================================
void BWorld::releaseTalkingHeadVideo(int talkingHeadID)
{
   for (int i=0; i<mTalkingHeads.getNumber(); i++)
   {
      BTalkingHead* pTalkingHead = mTalkingHeads[i];
      if (pTalkingHead && (pTalkingHead->mID==talkingHeadID) && pTalkingHead->mPreload)
      {
         pTalkingHead->mPreload = false;
         
         // The expectation is that this was nulled out when the video was queued for play in the first
         // place, so if it's not NULL here, something is wrong.
         BASSERT(pTalkingHead->mPreloadedData == NULL);
      }
   }
}

//==============================================================================
// BWorld::addCinematic
//==============================================================================
void BWorld::addCinematic(BCinematic* pCinematic, int cinematicID)
{
   BASSERT(mpCinematicManager);
   mpCinematicManager->addCinematic(pCinematic, cinematicID);
}

//==============================================================================
// BWorld::playCinematic
//==============================================================================
void BWorld::playCinematic(uint index)
{
   BASSERT(mpCinematicManager);
   mpCinematicManager->playCinematic(index);
}


//==============================================================================
// BWorld::initSky
//==============================================================================
void BWorld::initSky(const char* skyName)
{
   // rg [3/25/08] - Disable the sky for alpha to save memory.
   if (gConfig.isDefined(cConfigAlpha))
      return;
      
   if (mpSkyVisual)
   {
      gVisualManager.releaseVisual(mpSkyVisual);
      mpSkyVisual = NULL;
   }
   
   long skyID = gVisualManager.getOrCreateProtoVisual(skyName, true);

   BMatrix identMat; identMat.makeIdentity();
   mpSkyVisual = gVisualManager.createVisual(skyID, false, 0, cDWORDWhite, identMat, cVisualDisplayPriorityNormal);
}


//==============================================================================
// BWorld::reset
//==============================================================================
void BWorld::reset()
{
   // Make sure physics is not in the middle of updating before resetting
   if (getPhysicsWorld())
      getPhysicsWorld()->waitForUpdateCompletion();

   // Mark the physics world for write before we start removing stuff from it
   if(mpPhysicsWorld)
      mpPhysicsWorld->markForWrite(true);

   if(mpSkyVisual)
   {
      gVisualManager.releaseVisual(mpSkyVisual);
      mpSkyVisual=NULL;
   }

   gVisualManager.removeVisualHandler(this);

   if(mpCommandManager)
   {
      delete mpCommandManager;
      mpCommandManager=NULL;
   }
   
   // jce [11/20/2008] -- Cleaning up pathing limiter since this was leaking.
   if(mpPathingLimiter)
   {
      delete mpPathingLimiter;
      mpPathingLimiter = NULL;
   }

   //releaseObjects();
   releaseLights();
   
   if(mpBattleManager)
   {
      delete mpBattleManager;
      mpBattleManager=NULL;
   }

   if (mpPowerManager)
   {
      delete mpPowerManager;
      mpPowerManager = NULL;
   }

   if (mpAIManager)
   {
      delete mpAIManager;
      mpAIManager = NULL;
   }

   if(mpObjectManager)
   {
      mpObjectManager->reset();
      delete mpObjectManager;
      mpObjectManager=NULL;
   }

   // Reset objective manager
   if( mpObjectiveManager )
   {
      mpObjectiveManager->reset();
      delete mpObjectiveManager;
      mpObjectiveManager = NULL;
   }

   // Clear the talking heads
   for(int i=0; i<mTalkingHeads.getNumber(); i++)
      delete mTalkingHeads[i];
   mTalkingHeads.clear();

   // jce [11/20/2008] -- Delete the hint manager which was not being freed.
   if (mpHintManager)
   {
      delete mpHintManager;
      mpHintManager = NULL;
   }


   // Reset chat manager
   if( mpChatManager )
   {
      mpChatManager->reset();
      delete mpChatManager;
      mpChatManager = NULL;
   }

   // Reset design objects manager
   if( mpDesignObjectManager )
   {
      mpDesignObjectManager->reset();
      delete mpDesignObjectManager;
      mpDesignObjectManager = NULL;
   }

   // -- Reset Event Manager
   gGeneralEventManager.reset();

   //-- Reset Action Manager
   //DCP 04/27/07:  Note that BActionManager::reset() doesn't do anything.  Not sure what the point of it was
   //but I'm leaving that alone for now.
   gActionManager.reset();

   //-- Reset Formation Manager
   gFormationManager.reset();
   
   //Reset SimOrder Manager.
   gSimOrderManager.reset();

   //-- Reset Squad Plotter
   gSquadPlotter.reset();

   //-- Reset Squad LOS Validator
   gSquadLOSValidator.reset();

   BSquadAI::deinit();

   gVisibleMap.deinitMap();

   if (mpPhysicsWorld)
   {
      gTerrainSimRep.releasePhysics();
      delete mpPhysicsWorld;
      mpPhysicsWorld=NULL;
   }

   for(long i=0; i<mPlayers.getNumber(); i++)
   {
      if (mPlayers[i])
         mPlayers[i]->reset();
   }

   for(long i=0; i<mPlayers.getNumber(); i++)
      delete mPlayers[i];
   mPlayers.setNumber(0);

   for(long i=0; i<mTeams.getNumber(); i++)
      delete mTeams[i];
   mTeams.setNumber(0);

   if(mpWorldSoundManager)
   {
      delete mpWorldSoundManager;
      mpWorldSoundManager=NULL;
   }

   if(mpCinematicManager)
   {
      delete mpCinematicManager;
      mpCinematicManager=NULL;
   }

   if (mpTransitionManager)
   {
      delete mpTransitionManager;
      mpTransitionManager=NULL;
   }

   if(mpStoredEventTagManager)
   {
      delete mpStoredEventTagManager;
      mpStoredEventTagManager=NULL;
   }

   if(mEntityGrouper)
   {
      delete mEntityGrouper;
      mEntityGrouper=NULL;
   }

   gTriggerManager.reset();

   //-- Reset Entity Scheduler
   gEntityScheduler.reset();

   //-- kill all of the Particle Effects
   gParticleGateway.releaseAllInstances();   

   //-- kill all of the lights 
   gLightEffectManager.releaseAllInstances();

   //-- kill all of the lit decals
   gLitDecalManager.destroy();

   if (mBuildingPlacementDecal!=-1)
      gDecalManager.destroyDecal(mBuildingPlacementDecal);
   mBuildingPlacementDecal=-1;

   gImpactDecalManager.destroy();   

   mCustomCommands.clear();

   gCorpseManager.reset();

   gStatsManager.reset();

   gTerrainEffectManager.clear();

   gTextVisualManager.reset();

   mWorkUnits.clear();
   mWorldUnitUpdateWorkEntries.clear();

   // Release all "damage area over time" instances
   BDamageAreaOverTimeInstance::removeAllInstances();

   for (uint i=0; i<mTransformedTactics.getSize(); i++)
      delete mTransformedTactics[i];
   mTransformedTactics.clear();

   resetExplorationGroups();

   resetAllPhysicsBreakOffObjects();
   resetAllPhysicsVehicleTerrainCollisions();

   gPather.reset();

   gActionManager.reset();

   BPlacementRules::reset();

   mLowDetailAnimRefCount = 0;

   mTerrainMax.set(1.0f,1.0f,1.0f);

   BConcept::mFreeList.clear();
   BDamageAreaOverTimeInstance::mFreeList.clear();
   BDesignSphere::mFreeList.clear();
   BDesignLine::mFreeList.clear();
   BEntityGroup::mFreeList.clear();
   BGeneralEventSubscriber::mFreeList.clear();
   BGeneralEventSubscriber::mFreeList.clear();
   BPathMoveData::mFreeList.clear();
   BPerturbanceData::mFreeList.clear();
   BStickData::mFreeList.clear();
   BSimOrder::mFreeList.clear();
   BUnitOpp::mFreeList.clear();

   BEntityFilterIsAlive::mFreeList.clear();
   BEntityFilterIsIdle::mFreeList.clear();
   BEntityFilterInList::mFreeList.clear();
   BEntityFilterPlayers::mFreeList.clear();
   BEntityFilterTeams::mFreeList.clear();
   BEntityFilterProtoObjects::mFreeList.clear();
   BEntityFilterProtoSquads::mFreeList.clear();
   BEntityFilterObjectTypes::mFreeList.clear();
   BEntityFilterRefCount::mFreeList.clear();
   BEntityFilterRelationType::mFreeList.clear();
   BEntityFilterMaxObjectType::mFreeList.clear();

   BEventFilterEntity::mFreeList.clear();
   BEventFilterEntityList::mFreeList.clear();
   BEventFilterLocation::mFreeList.clear();
   BEventFilterType::mFreeList.clear();
   BEventFilterNumeric::mFreeList.clear();
   BEventFilterCamera::mFreeList.clear();
   BEventFilterGameState::mFreeList.clear();
   
   // jce [11/12/2008] -- Adding a clear here for this cache so that it's reset between scenarios.
   gGrannyManager.clearRenderPreparePoseCache();
}

//==============================================================================
// BWorld::releaseObjects
//==============================================================================
void BWorld::releaseObjects()
{
   
}

//==============================================================================
// BWorld::addAsyncWorkUnit
//==============================================================================
void BWorld::addAsyncWorkUnit(BEntityHandle handle)
{
   BAsyncWorkUnit newGroup;
   newGroup.mEntityHandle = handle;
   mWorkUnits.add(newGroup);
}

//==============================================================================
// BWorld::clearAsyncWorkGroups
//==============================================================================
void BWorld::clearAsyncWorkGroups()
{
   mWorkUnits.setNumber(0);
}

//==============================================================================
// BWorld::flushAsyncWorkGroups
//==============================================================================
void BWorld::flushAsyncWorkGroups()
{
   SCOPEDSAMPLE(BWorld_flushAsyncWorkGroups);  

   if(mWorkUnits.getSize()==0)
      return;

   //flush any work that is in the pipe right now
   gWorkDistributor.flush();


   //calculate optimal bucket and work sizes
   const uint cTotalNumTasks = mWorkUnits.getSize();
   const uint cNumTasksPerWorkEntryLog2 = 2;
   const uint cNumTasksPerWorkEntry = 1U << cNumTasksPerWorkEntryLog2;
   const uint cTotalNumWorkEntries = (cTotalNumTasks >> cNumTasksPerWorkEntryLog2) + 1;
   

   uint cNumWorkEntriesPerBucketLog2 = 4;
   if (cTotalNumWorkEntries <= 8)           cNumWorkEntriesPerBucketLog2 = 1;
   else if (cTotalNumWorkEntries <= 16)      cNumWorkEntriesPerBucketLog2 = 2;
   else if (cTotalNumWorkEntries <= 32)      cNumWorkEntriesPerBucketLog2 = 3;
   const uint cNumWorkEntriesPerBucket = 1U << cNumWorkEntriesPerBucketLog2;
   BDEBUG_ASSERT(cNumWorkEntriesPerBucket <= gWorkDistributor.getWorkEntryBucketSize());  

   const uint totalBuckets = (cTotalNumWorkEntries + cNumWorkEntriesPerBucket - 1) >> cNumWorkEntriesPerBucketLog2;
   mWorldUnitUpdateRemainingBuckets.set(totalBuckets);

   
   //allocate thread local storage space for our data.  Don't resize this to a smaller size since that will
   // call the destructor and free the memory for mObjectHandles every frame.
   if (mWorldUnitUpdateWorkEntries.getSize() < cTotalNumWorkEntries)
      mWorldUnitUpdateWorkEntries.resize(cTotalNumWorkEntries);
   BAsyncWorkGroupThreadData* pNextWorkEntry = mWorldUnitUpdateWorkEntries.getPtr();  //so we'll be filling our work entries.

   

   uint numBucketWorkEntries = 0;
   uint runningBucketCount = 0;
   uint workUnitIndex = 0;
   for(uint entryIndex = 0; entryIndex < cTotalNumWorkEntries; entryIndex ++)
   {
      //create a bucket entry
      BAsyncWorkGroupThreadData* currEntry = pNextWorkEntry;
      currEntry->mpWorld = this;
      currEntry->mWorldlastUpdateLengthFloat = mLastUpdateLengthFloat;
      currEntry->mObjectHandles.setNumber(0);
      if (currEntry->mObjectHandles.getCapacity() < cNumTasksPerWorkEntry)
         currEntry->mObjectHandles.reserve(cNumTasksPerWorkEntry);

      //fill the work entry with tasks
      for(uint k=0; k<cNumTasksPerWorkEntry;k++)
      {
         if(workUnitIndex >= cTotalNumTasks)
            break;

         currEntry->mObjectHandles.push_back(mWorkUnits[workUnitIndex++].mEntityHandle); 
      }

      //queue up the work entry to the bucket
      gWorkDistributor.queue(updateAsyncWorkgroupCallback, currEntry,0);

      //handle bucket management
      numBucketWorkEntries++;
      if (numBucketWorkEntries == cNumWorkEntriesPerBucket)
      {
         runningBucketCount++;
         numBucketWorkEntries = 0;            
         gWorkDistributor.flush();
      }

      //increment our frame storage pointers
      pNextWorkEntry++;
   }
   

   //flush whatever is left over

   if(numBucketWorkEntries)
   {
      runningBucketCount++;
      gWorkDistributor.flush();
      numBucketWorkEntries = 0;
   }

   BASSERT(runningBucketCount == totalBuckets);

   BDEBUG_ASSERT((pNextWorkEntry - mWorldUnitUpdateWorkEntries.getPtr()) <= static_cast<int>(mWorldUnitUpdateWorkEntries.getSize()));


   gWorkDistributor.waitSingle(mWorldUnitUpdateRemainingBuckets);

   clearAsyncWorkGroups();
}

//==============================================================================
// BWorld::updateAsyncWorkgroupCallback
//==============================================================================
void BWorld::updateAsyncWorkgroupCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BAsyncWorkGroupThreadData* dat = static_cast<BAsyncWorkGroupThreadData*>(privateData0);
   BWorld* pWorld = dat->mpWorld;

   for(uint i=0; i<dat->mObjectHandles.getSize(); i++)
   {
      const BEntityHandle handle = dat->mObjectHandles[i];
      uint entityType = handle.getType();
      switch(entityType)
      {
      case BEntity::cClassTypeUnit:
         {
            BUnit* pUnit =pWorld->getUnit(handle,true); 
            pUnit->BUnit::updateAsync(dat->mWorldlastUpdateLengthFloat);
            break;
         }
      case BEntity::cClassTypeObject:
         {
            BObject* pObject = pWorld->getObject(handle,true); 
            pObject->BObject::updateAsync(dat->mWorldlastUpdateLengthFloat);
            break;
         }
      case BEntity::cClassTypeDopple:
         {    
            BDopple* pDopple = pWorld->getDopple(handle,true); 
            pDopple->BDopple::updateAsync(dat->mWorldlastUpdateLengthFloat);
            break;
         }
      case BEntity::cClassTypeProjectile:
         {
            BProjectile* pProjectile = pWorld->getProjectile(handle,true); 
            pProjectile->BProjectile::updateAsync(dat->mWorldlastUpdateLengthFloat);
            break;
         }


      default:
         break;
      };
   }

   if (lastWorkEntryInBucket)
      pWorld->mWorldUnitUpdateRemainingBuckets.decrement();
}

//==============================================================================
// BWorld::update
//==============================================================================
void BWorld::update(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency, bool runningGame, BTimingTracker *tracker)
{
   #if defined(ENABLE_TIMELINE_PROFILER)
   // jce [10/27/2008] -- Special profile section for world update since with subupdating on it cannot be a scoped
   // sample since it will extend over multiple frames/calls to the update function.
   //static BProfileSection sWorldUpdateProfileSection("FullWorldUpdate", false);
   #endif

   #ifndef BUILD_FINAL
   BTimer simTimer;   
   simTimer.start();

   mTimeStats.clear();

   if(gConfig.isDefined(cConfigPIXTraceWorldUpdate))
   {
      BOOL traceStartOK = XTraceStartRecording("e:\\WorldUpdateTrace.pix2");
      BASSERT(traceStartOK);
   }
   #endif

   SCOPEDSAMPLE(BWorldUpdate);   

   mpTimingTracker = tracker;

   if (runningGame)
      handleDynamicSubUpdating();

   if (gConfig.isDefined(cConfigSubUpdTrace))
      gConsole.output(cChannelSim, "  BWorld::update begin, mUpdateNumber %d, mSubUpdate %d, currentUpdateTime %f, currentUpdateLength %f", mUpdateNumber, mSubUpdate, currentUpdateTime, currentUpdateLength);
      
   mLastTimerFrequency = timerFrequency;

   if (mFirstUpdate)
      handleFirstUpdate();

   if (gEnableSubUpdating && runningGame) // sub-updating turned on?
   {  
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         //------------------------------------------------------------------------------
         // Decoupled update
         //------------------------------------------------------------------------------
         mDecoupledSubUpdateTimer.start();
         mDecoupledWorkSinceLastRenderTimer.resume();

         if (mDecoupledUpdateRender || mSubUpdate == 0)
         {
            //ProfileManagerStartSample(sWorldUpdateProfileSection);
            
            // Reset the render timer
            mDecoupledUpdateRender = false;
            
            if(!mDecoupledRenderTimer.isStarted())
               mDecoupledRenderTimer.start();

            // Compute the render time 
            float targetRenderTime = 0.03f;
            gConfig.get(cConfigDecoupledRenderTime, &targetRenderTime);

            float outsideTimePercent = 1.0f;
            gConfig.get(cConfigDecoupledOutsideTimePercent, &outsideTimePercent);

            float outsideTimeAmount = 0.0f;
            gConfig.get(cConfigDecoupledOutsideTimeAmount, &outsideTimeAmount);

            float outsideTime = mpTimingTracker->getOutsideUpdateAverage()*0.001f;
            outsideTime *= outsideTimePercent;
            outsideTime += outsideTimeAmount;

            mDecoupledUpdateRenderTime = max(outsideTime, targetRenderTime);
         }

         mDecoupledUpdateSkipRenders = gConfig.isDefined(cConfigSkipDecoupledRenders);

         #ifndef BUILD_FINAL
         if (gConfig.isDefined(cConfigDecUpdTraceStart))
            trace("UpdStart - Upd# %u, SubUpd %u, Frame %8.4f, Outside %8.4f, Update %8.4f, Sect %2d, Item %10d, Render %d, Elapsed %8.4f, Pct %5.3f", mUpdateNumber, mSubUpdate, gRender.getAverageFrameTime(), ((double)mpTimingTracker->getOutsideUpdateAverage())/1000.0, currentUpdateLength, mSubUpdateSection, mSubUpdateSectionItem, (int)mDecoupledUpdateRender, mDecoupledUpdateElapsed, getUpdateCompletion(mSubUpdate));
         #endif

         if (mDecoupledUpdateEnd)
         {
            mDecoupledUpdateEnd = false;
            mSubUpdateSection = cSubUpdateStart;
            
            // Clear previous subupdate info that is now old.
            mDecoupledUpdateTimesPrevious->clear();
            
            // Swap current/previous arrays.  (Current now becomes the previous).
            bswap(mDecoupledUpdateTimesCurrent, mDecoupledUpdateTimesPrevious);
            
            // Starting subupdate for the new previous list is the old current value.
            mDecoupledUpdateStartingSubUpdatePrevious = mDecoupledUpdateStartingSubUpdateCurrent;
            
            // New current subupdate list starts on this subupdate.
            mDecoupledUpdateStartingSubUpdateCurrent = mSubUpdate;
           
            mDecoupledUpdateLength = currentUpdateLength;
            mDecoupledUpdateElapsed = 0.0;
            mDecoupledUpdateAvgUpdateTime = mpTimingTracker->getUpdateAverage()*0.001f;
            mDecoupledUpdateTotalUpdateTime = 0.0;
            mFirstSubUpdate = mSubUpdate;         
         }

         updateGrannySync(true);

         SUBUPDATERESUME(Delay)
         SUBUPDATERESUME(PostAsyncStart)
         SUBUPDATERESUME(AsyncStart)
         SUBUPDATERESUME(PreAsyncStart)

         SUBUPDATENEXT

         SUBUPDATELABEL(PreAsyncStart)
         updatePreAsync(currentUpdateTime, currentUpdateLength, timerFrequency);

         SUBUPDATESKIP(AsyncStart, Frame)
         updateAsync(currentUpdateTime, currentUpdateLength, timerFrequency);

         SUBUPDATESKIP(PostAsyncStart, Frame)
         updatePostAsync(currentUpdateTime, currentUpdateLength, timerFrequency);

         SUBUPDATESKIP(Delay, Frame)

         #ifndef BUILD_FINAL
         long minSimTime = 0;
         if (gConfig.get(cConfigMinSimTime, &minSimTime) && minSimTime > 0)
         {
            SCOPEDSAMPLE(MinSimTimeDelay)
            double minSimTimeSec = minSimTime * 0.001;
            for (;;)
            {
               if (mDecoupledUpdateTotalUpdateTime + mDecoupledSubUpdateTimer.getElapsedSeconds() > minSimTimeSec)
                  break;
               SUBUPDATECANRENDERSKIP(Frame);
               
               static bool cSleepyWait = false;
               if(cSleepyWait)
                  Sleep(1);
            }
         }
         #endif

         SUBUPDATENEXT

         SUBUPDATELABEL(Frame)

         updateFrame();

         // jce [10/27/2008] -- Mark that we've finished an update before the final call of this update to canRenderInterpolatedFrame
         if (mSubUpdateSection == cSubUpdateEnd)
            mDecoupledFinishedAnUpdate = true;
            
         if (!mDecoupledUpdateRender)
         {
            if ((!mDecoupledUpdateSkipRenders && mSubUpdateSection == cSubUpdateEnd) || canRenderInterpolatedFrame())
               mDecoupledUpdateRender = true;
         }

         mDecoupledUpdateElapsed = mDecoupledUpdateTimer.getElapsedSeconds();
         mDecoupledUpdateLastSubElapsed = mDecoupledSubUpdateTimer.getElapsedSeconds();

         if (mSubUpdateSection == cSubUpdateEnd)
         {
            mpTimingTracker->addLastUpdateLength(mDecoupledUpdateElapsed); 
            mDecoupledUpdateTimer.start();
            mDecoupledUpdateEnd = true;
            //ProfileManagerEndSample(sWorldUpdateProfileSection);
         }

         // Get current time when this subupdate is complete.         
         LARGE_INTEGER currTime;
         QueryPerformanceCounter(&currTime);
         double subUpdateRealTime = double(currTime.QuadPart / mLastTimerFrequency);
         
         // Store it in the history.
         mDecoupledUpdateTimesCurrent->add(subUpdateRealTime);
         
         // If we're going to render, reset the timer which is keeping track of how much update work we have done since the last render.
         // Otherwise, if we aren't going to render we're still going to leave the update (it's finished presumably) so we'll suspend the timer. 
         // It will get resumed next update.
         if(mDecoupledUpdateRender)
            mDecoupledWorkSinceLastRenderTimer.reset();
         else
            mDecoupledWorkSinceLastRenderTimer.stop();

         #ifndef BUILD_FINAL
         //if (gConfig.isDefined(cConfigDecUpdTraceEnd))
         //   trace("   UpdEnd - ElapsedRenderTime %8.4f, Section %d, Render %d, UpdPct %8.4f, UpdElapsed %8.4f", mDecoupledRenderTimer.getElapsedSeconds(), mSubUpdateSection, (int)mDecoupledUpdateRender, mDecoupledUpdatePercent, getUpdateCompletion(mSubUpdate));

         /*
         if (gConfig.isDefined(cConfigDecUpdTracePct))
         {
            trace("   PctAry - Array %d, Start1 %u, Start2 %u, Size1 %u, Size2 %u", mDecoupledUpdateTimesArray, mDecoupledUpdateStartingSubUpdate1, mDecoupledUpdateStartingSubUpdate2, mDecoupledUpdateTimes1.size(), mDecoupledUpdateTimes2.size());
            for (int i=0; i<mDecoupledUpdateTimes1.getNumber(); i++)
               trace("      Ary1 - Idx %2d, SubUpd %u, Pct %8.4f", i, mDecoupledUpdateStartingSubUpdate1 + i, mDecoupledUpdateTimes1[i]);
            for (int i=0; i<mDecoupledUpdateTimes2.getNumber(); i++)
               trace("      Ary2 - Idx %2d, SubUpd %u, Pct %8.4f", i, mDecoupledUpdateStartingSubUpdate2 + i, mDecoupledUpdateTimes2[i]);
         }
         */
         #endif

         mSubUpdate++;

         mDecoupledUpdateTotalUpdateTime += mDecoupledSubUpdateTimer.getElapsedSeconds();
      }
      else
      #endif
      {
         //------------------------------------------------------------------------------
         // Sub-updating
         //------------------------------------------------------------------------------
         beginSubUpdate();
         
         if (isOnFirstSubUpdate())
         {  
            if (gConfig.isDefined(cConfigSubUpdTrace))
               gConsole.output(cChannelSim, "  running preAsync sub-update %d", mSubUpdate);

            mFirstSubUpdate = mSubUpdate;         
            
            // figure out what our time error delta was on the last update 
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            if (mSubUpdateLastTime != 0)
            {            
               float deltaSecs = (currentTime.QuadPart - mSubUpdateLastTime) / mLastTimerFrequency;
               mAccumulatedTimeError += max(-0.1f, min(0.1f, (getSubUpdateTotalTimeInMsecs() / 1000.0f) - deltaSecs)); // the max error we accumulate per update is 100msecs
            }  
            mSubUpdateLastTime = currentTime.QuadPart;
         }

         for (;;)
         {
            DWORD subUpdate = mSubUpdate - mFirstSubUpdate;
            switch (subUpdate)
            {
               case 0:
                  updatePreAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
                  updateAsync(currentUpdateTime, currentUpdateLength, timerFrequency);            
                  break;

               case 1:
                  updatePostAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
                  break;

               case 2:
                  updatePostAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
                  break;
            }

            updateFrame();

            incrementSubUpdate();

            LARGE_INTEGER endTime;
            QueryPerformanceCounter(&endTime);
            int64 delta = (int64)(endTime.QuadPart - mSubUpdateBeganTime);            
            mpTimingTracker->addLastUpdateLength(delta/timerFrequency); 
            
            if (isOnFirstSubUpdate() || mDynamicSubUpdateTime || canRenderInterpolatedFrame()) // if we've completed an update, we've got to render at least one frame
            {
               if (gConfig.isDefined(cConfigSubUpdTrace))
               {
                  if (isOnFirstSubUpdate())
                     gConsole.output(cChannelSim, "  completed a full update *******");
                  else
                     gConsole.output(cChannelSim, "  dropping out to render a frame");
               }
               break;       
            }

            beginSubUpdate(); // otherwise loop around and start another sub-update            
            if (gConfig.isDefined(cConfigSubUpdTrace))
               gConsole.output(cChannelSim, "  running additional sub-update %d", mSubUpdate);
         } 
      }      
   }
   else
   {
      //------------------------------------------------------------------------------
      // Not sub-updating
      //------------------------------------------------------------------------------
      if (runningGame)
         updateGrannySync(true);

      updatePreAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
      updateAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
      updatePostAsync(currentUpdateTime, currentUpdateLength, timerFrequency);
      updateFrame();
   }

   if (gConfig.isDefined(cConfigSubUpdTrace))
      gConsole.output(cChannelSim, "  BWorld::update end");
   
   #ifdef DECOUPLED_UPDATE
   if(mDecoupledUpdateRender)
   {
      mDecoupledRenderTimer.start();
      mDecoupledFinishedAnUpdate = false;
      
      /*
      static __int64 lastTime = 0;
      __int64 currTime;
      QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
      __int64 delta = currTime - lastTime;
      trace("render elapsed from last time: %0.2f", 1000.0f*double(delta)/mLastTimerFrequency);
      lastTime = currTime;
      */
   }
   #endif

   BProtoVisual::mGenerationChanged = false;

   #ifndef BUILD_FINAL
   simTimer.stop();
   double statTime = simTimer.getElapsedSeconds();

   if (gEnableSubUpdating)
   {
      DWORD subUpdate = mSubUpdate - mFirstSubUpdate;
      switch (subUpdate)
      {
         case 1:
            mTimeStats.mSubUpdate1Time = statTime;
            mTimeStats.mAvgSubUpdate1Time.addSample(statTime);
            if (statTime > mTimeStats.mMaxSubUpdate1Time)
               mTimeStats.mMaxSubUpdate1Time = statTime;
            break;

         case 2:
            mTimeStats.mSubUpdate2Time = statTime;
            mTimeStats.mAvgSubUpdate2Time.addSample(statTime);
            if (statTime > mTimeStats.mMaxSubUpdate2Time)
               mTimeStats.mMaxSubUpdate2Time = statTime;
            break;

         case 3:
            mTimeStats.mSubUpdate3Time = statTime;
            mTimeStats.mAvgSubUpdate3Time.addSample(statTime);
            if (statTime > mTimeStats.mMaxSubUpdate3Time)
               mTimeStats.mMaxSubUpdate3Time = statTime;
            break;
      }
   }

   mTimeStats.mTotalUpdateTime = statTime;
   mTimeStats.mAvgTotalUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTotalUpdateTime)
      mTimeStats.mMaxTotalUpdateTime = statTime;

   if(gConfig.isDefined(cConfigPIXTraceWorldUpdate))
   {
      gConfig.remove(cConfigPIXTraceWorldUpdate);
      XTraceStopRecording();
   }
   #endif  

   #ifndef BUILD_FINAL
   // Auto-save
   if (!gEnableSubUpdating || isOnFirstSubUpdate())
   {
      float saveInterval = 0.0f;
      if (gConfig.get(cConfigAutoSave, &saveInterval))
      {
         mAutoSaveTime += currentUpdateLength;
         if (mAutoSaveTime >= saveInterval)
         {
            gSaveGame.saveGame("");
            mAutoSaveTime = 0.0f;
         }
      }
   }
   #endif
}

//==============================================================================
// BWorld::handleFirstUpdate
//==============================================================================
void BWorld::handleFirstUpdate()
{
   // Need to grab the terrain max value once updating starts
   const D3DXVECTOR3& terrainMax = gTerrain.getMax();
   mTerrainMax.set(terrainMax.x, terrainMax.y, terrainMax.z);

   mFirstUpdate = false;
}

//==============================================================================
// Dynamically turn sub-updating on and off based on average update time.
// Only have the host make the decision so that it can be changed sync safe
// by sending a command. Also ignore the first 30 updates to let the updates
// stabilize.
//==============================================================================
void BWorld::handleDynamicSubUpdating()
{
   // Have we received a sub update change request?
   if (mNextEnableSubUpdating != gEnableSubUpdating || 
       mNextAlternateSubUpdating != mAlternateSubUpdating ||
       mNextDynamicSubUpdateTime != mDynamicSubUpdateTime
       #ifdef DECOUPLED_UPDATE
       || mNextDecoupledUpdate != gDecoupledUpdate
       #endif
      )
   {
      if (!gEnableSubUpdating || isOnFirstSubUpdate())
      {
         // Commit the sub update change.
         mChangeSubUpdatePendnig = false;
         mSubUpdateEnabledTimer.start();

         bool enablingSubUpdating = (mNextEnableSubUpdating && !gEnableSubUpdating);

         gEnableSubUpdating = mNextEnableSubUpdating;
         #ifdef DECOUPLED_UPDATE
            gDecoupledUpdate = mNextDecoupledUpdate;
            mDecoupledUpdateRender = false;
         #endif
         mAlternateSubUpdating = mNextAlternateSubUpdating;
         mDynamicSubUpdateTime = mNextDynamicSubUpdateTime;

         if (enablingSubUpdating)
         {
            mAdjustedSubUpdateTime = cSubUpdateTimeInMsecs;
            mSubUpdate = mUpdateNumber + (getAmountOfSubUpdates() * 2);
            while (!isOnFirstSubUpdate())
               mSubUpdate++;
            mSubUpdateEnabledNumber = mSubUpdate + getAmountOfSubUpdates();
            mFirstSubUpdate = mSubUpdate;
            mSubUpdateBeganTime = 0;
            mSubUpdateLastTime = 0;
            mAccumulatedTimeError = 0;

            // Reset sub updating for all objects and squads so they will be in the correct state.
            BEntityHandle handle = cInvalidObjectID;
            BUnit* pUnit= getNextUnit(handle);
            while (pUnit)
            {
               pUnit->resetSubUpdating();
               pUnit = getNextUnit(handle);
            }

            handle = cInvalidObjectID;
            BSquad* pSquad = getNextSquad(handle);
            while (pSquad)
            {
               pSquad->resetSubUpdating();
               pSquad = getNextSquad(handle);
            }

            handle = cInvalidObjectID;
            BDopple* pDopple = getNextDopple(handle);
            while (pDopple)
            {
               pDopple->resetSubUpdating();
               pDopple = getNextDopple(handle);
            }

            handle = cInvalidObjectID;
            BProjectile* pProjectile = getNextProjectile(handle);
            while (pProjectile)
            {
               pProjectile->resetSubUpdating();
               pProjectile = getNextProjectile(handle);
            }

            handle = cInvalidObjectID;
            BObject* pObject = getNextObject(handle);
            while (pObject)
            {
               pObject->resetSubUpdating();
               pObject = getNextObject(handle);
            }
         }
      }
   }
   else
   {
      // Can we dynamically turn sub updating on and off?
      if (mDynamicSubUpdating && gModeManager.getModeGame()->getIsHost() && mUpdateNumber > 30 && !mChangeSubUpdatePendnig)
      {
         float minSubUpdateEnabledTime = 5.0f;
         gConfig.get(cConfigMinSubUpdateEnabledTime, &minSubUpdateEnabledTime);
         if (!mSubUpdateEnabledTimer.isStarted() || (float)mSubUpdateEnabledTimer.getElapsedSeconds() >= minSubUpdateEnabledTime)
         {
            bool newSubUpdating = gEnableSubUpdating;
            if (gEnableSubUpdating)
            {
               if (isOnFirstSubUpdate() && mSubUpdate > mSubUpdateEnabledNumber)
               {
                  long avgUpdOff = 10;
                  gConfig.get(cConfigSubUpdateAvgUpdOff, &avgUpdOff);
                  if (mpTimingTracker->getUpdateAverage() <= (uint32)avgUpdOff)
                  {
                     newSubUpdating = false;
                     if (gConfig.isDefined(cConfigSubUpdTrace))
                        gConsole.output(cChannelSim, "  BWorld::update turning off sub-updating, avg upd %u", mpTimingTracker->getUpdateAverage());
                  }
               }
            }
            else
            {
               long avgUpdOn = 40;
               gConfig.get(cConfigSubUpdateAvgUpdOn, &avgUpdOn);
               if (mpTimingTracker->getUpdateAverage() >= (uint32)avgUpdOn)
               {
                  newSubUpdating = true;
                  if (gConfig.isDefined(cConfigSubUpdTrace))
                     gConsole.output(cChannelSim, "  BWorld::update turning on sub-updating, avg upd %u", mpTimingTracker->getUpdateAverage());
               }
            }
            if (newSubUpdating != gEnableSubUpdating)
            {
               long playerID = gUserManager.getPrimaryUser()->getPlayerID();
               BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &playerID);
                  pCommand->setRecipientType(BCommand::cGame);
                  pCommand->setType(BGameCommand::cTypeSubUpdating);
                  pCommand->setData(newSubUpdating ? 1 : 0);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);
                  mChangeSubUpdatePendnig = true;
               }
            }
         }
      }
   }
}

//==============================================================================
// BWorld::updateGrannySync
// ajl 7/25/07 - Attempt to fix an OOS that seems to be caused by the way Granny keeps track
// of time internally. It's affected by the rendering of a unit... If a unit isn't rendered,
// then it is in a different state than if it was rendered, and that can cause a slight
// OOS when a bone position is looked up.
//==============================================================================
void BWorld::updateGrannySync(bool doSyncChecks)
{
   SCOPEDSAMPLE(BWorldupdateGrannySync);

   //Units.
   {
      SCOPEDSAMPLE(updateGrannySync_UpdateUnits);  
      BEntityHandle handle = cInvalidObjectID;
      BUnit* pUnit = getNextUnit(handle);
      while (pUnit)
      {
         pUnit->updateGrannySync(doSyncChecks);
         pUnit = getNextUnit(handle);
      }
   }

   //Dopples.
   {
      SCOPEDSAMPLE(UpdateDopples);  
      BEntityHandle handle = cInvalidObjectID;
      BDopple* pDopple = getNextDopple(handle);
      while (pDopple)
      {
         pDopple->updateGrannySync(doSyncChecks);
         pDopple = getNextDopple(handle);
      }
   }

   //Projectiles.
   {
      SCOPEDSAMPLE(UpdateProjectiles);  
      BEntityHandle handle = cInvalidObjectID;
      BProjectile* pProjectile = getNextProjectile(handle);
      while (pProjectile)
      {
         pProjectile->updateGrannySync(doSyncChecks);
         pProjectile = getNextProjectile(handle);
      }
   }
}

//==============================================================================
// BWorld::updatePreAsync
//==============================================================================
void BWorld::updatePreAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency)
{
   //Functions in this section should be the BARE MINIMUM needed to set up state for the async phase.
   SCOPEDSAMPLE(BWorldupdatePreAsync);

   //------------------------------------------------------------------------------
   // Timings/debugging.
   //------------------------------------------------------------------------------
   #ifndef BUILD_FINAL
   BTimer localTimer;   
   BTimer simTimer;
   BTimer triggerObjectsTimer;   
   double statTime = 0;

   simTimer.start();

   mLaunchedProjectilesPerFrame = 0;   

   gTerrainEffectManager.resetStats();
   #endif

   #ifdef DECOUPLED_UPDATE
   //------------------------------------------------------------------------------
   // Resume subupdating at the point we left off.
   //------------------------------------------------------------------------------
   if (gDecoupledUpdate)
   {
      SUBUPDATERESUME(PreAsyncProjectile)
      SUBUPDATERESUME(PreAsyncDopple)
      SUBUPDATERESUME(PreAsyncObject)
      SUBUPDATERESUME(PreAsyncUnit)
      SUBUPDATERESUME(PreAsyncSquadPlotter)
      SUBUPDATERESUME(PreAsyncSquad)
      SUBUPDATERESUME(PreAsyncPlatoon)
      SUBUPDATERESUME(PreAsyncArmy)
      SUBUPDATERESUME(PreAsyncDamageOverTime)
      SUBUPDATERESUME(PreAsyncFatality)
      SUBUPDATERESUME(PreAsyncCorpse)
      SUBUPDATERESUME(PreAsyncCinematic)
      SUBUPDATERESUME(PreAsyncCommandService)
      SUBUPDATERESUME(PreAsyncAI)
      SUBUPDATERESUME(PreAsyncKB)
      SUBUPDATERESUME(PreAsyncPlayer)
      SUBUPDATERESUME(PreAsyncTeam)
      SUBUPDATERESUME(PreAsyncCommandProcess)
   }
   #endif

   //------------------------------------------------------------------------------
   // Initial pre-async work.
   //------------------------------------------------------------------------------
   clearAsyncWorkGroups();

   mFlagAllVisible = (gConfig.isDefined(cConfigNoFogMask) || mFlagNoFogSim || mFlagNoFogVis);

   // Mark the physics world for write since anything from here down to the update
   // should have write access
   mpPhysicsWorld->markForWrite(true);

   mpPathingLimiter->resetFramePathingCounts();
   /* DLM 11/10/08 - this stuff no longer supported   
   mpPathingLimiter->setTotalPathTime(0);
   if (timeGetTime() > (mpPathingLimiter->getTimeOfLastMaxPathDelay() + 5000)) // Defines recent as within the past 5 sec (stats only)
      mpPathingLimiter->setMaxRecentPathingDelays(0);
   */

   //-- Reset Squad LOS Validator
   gSquadLOSValidator.reset();

   //-- Set all of the time-related variables based on the values passed in.
   mLastUpdateTime=currentUpdateTime;

   mLastUpdateLengthFloat=currentUpdateLength;
   mLastUpdateLength=(DWORD)(mLastUpdateLengthFloat*1000.0f);

   mGametimeFloat+=mLastUpdateLengthFloat;
   mGametime=(DWORD)(mGametimeFloat*1000.0f);

   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   if(mLastUpdateRealtime!=0)
   {
      int64 delta = (int64)(time.QuadPart - mLastUpdateRealtime);
      mLastUpdateRealtimeDelta = delta/timerFrequency;
      mTotalRealtime += mLastUpdateRealtimeDelta;
   }   
   mLastUpdateRealtime=time.QuadPart;

   //-- World syncing
   #ifdef SYNC_FinalRelease
   syncFinalReleaseData("FRW GT", mGametime);
   syncFinalReleaseData("FRW LUL", mLastUpdateLength);
   #endif

   #ifdef SYNC_World
   syncWorldData("BWorld::update mFlagQuickBuild", mFlagQuickBuild);
   syncWorldData("BWorld::update mFlagNoFogSim", mFlagNoFogSim);
   syncWorldData("BWorld::update cConfigNoDamage", gConfig.isDefined(cConfigNoDamage));
   syncWorldData("BWorld::update gEnableSubUpdating", gEnableSubUpdating);
   syncWorldData("BWorld::update ", gEnableSubUpdating);
   syncWorldData("BWorld::update mDynamicSubUpdating", mDynamicSubUpdating);
   syncWorldData("BWorld::update mAlternateSubUpdating", mAlternateSubUpdating);
   syncWorldData("BWorld::update mDynamicSubUpdateTime", mDynamicSubUpdateTime);
   float gameSpeed = 1.0f;
   gConfig.get(cConfigGameSpeed, &gameSpeed);
   syncWorldData("BWorld::update cConfigGameSpeed", gameSpeed);
   #endif

   //-- Game timer
   gTimerManager.update(mLastUpdateLengthFloat);

   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Command manager process.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncCommandProcess);
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(CommandManagerProcessCommands);

      mpCommandManager->processCommands();
      if (this != gWorld) // process commands can cause the world to be destroyed, so do a check here.
         return;
   }     
   SUBUPDATENEXT
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mCommandManagerUpdateTime = statTime;
   mTimeStats.mAvgCommandManagerUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxCommandManagerUpdateTime)
      mTimeStats.mMaxCommandManagerUpdateTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Teams.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncTeam);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   #ifdef DECOUPLED_UPDATE
   long startTeam = (gDecoupledUpdate ? mSubUpdateSectionItem + 1 : 0);
   #else
   long startTeam = 0;
   #endif
   for(long i=startTeam; i<mTeams.getNumber(); i++)
   {
      SCOPEDSAMPLE(TeamUpdate);
      mTeams[i]->update(mLastUpdateLengthFloat);
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         mSubUpdateSectionItem = i;
         SUBUPDATECANRENDERBREAK
      }
      #endif
   }
   #ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate && i >= mTeams.getNumber() - 1)
      SUBUPDATENEXT
   #endif
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mTeamPreAsyncTime = statTime;
   mTimeStats.mAvgTeamPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTeamPreAsyncTime)
      mTimeStats.mMaxTeamPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Players.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncPlayer);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif

   updatePlayers();

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mPlayerPreAsyncTime = statTime;
   mTimeStats.mAvgPlayerPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxPlayerPreAsyncTime)
      mTimeStats.mMaxPlayerPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // KB.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncKB);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif

   if (mpAIManager)
      mpAIManager->updateKBs();
   SUBUPDATENEXT

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mKBPreAsyncTime = statTime;
   mTimeStats.mAvgKBPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxKBPreAsyncTime)
      mTimeStats.mMaxKBPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // AI.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncAI);

   #ifndef BUILD_FINAL
      localTimer.start();
   #endif

   if (mpAIManager)
   {
      mpAIManager->updateAIs();
      mpAIManager->updateAIMissions();
   }
   SUBUPDATENEXT

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mAIPreAsyncTime = statTime;
   mTimeStats.mAvgAIPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxAIPreAsyncTime)
      mTimeStats.mMaxAIPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Command manager service.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncCommandService);
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif

   if(mpCommandManager)
   {
      SCOPEDSAMPLE(CommandManagerService);  
      mpCommandManager->service();
   }
   SUBUPDATENEXT

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mCommandManagerServiceTime = statTime;
   mTimeStats.mAvgCommandManagerServiceTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxCommandManagerServiceTime)
      mTimeStats.mMaxCommandManagerServiceTime = statTime;
   #endif

   SUBUPDATERETURN(PreAsyncCinematic);

   //------------------------------------------------------------------------------
   // Debug primitives.
   //------------------------------------------------------------------------------
   {
      SCOPEDSAMPLE(DebugPrimClear);  
      gpDebugPrimitives->clear(BDebugPrimitives::cCategoryFormations);
      gpDebugPrimitives->clear(BDebugPrimitives::cCategoryPathing);
   }

   //------------------------------------------------------------------------------
   // Cinematics.
   //------------------------------------------------------------------------------
   mpCinematicManager->update(mLastUpdateLengthFloat);
   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Corpse manager.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncCorpse);
   gCorpseManager.update();
   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Fatality manager.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncFatality);
   gFatalityManager.update();
   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Damage over time.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncDamageOverTime);
   BDamageAreaOverTimeInstance::updateAllActiveInstances();
   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Armies.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncArmy);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateArmies);  
      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BArmy *pArmy=getNextArmy(handle);
      while (pArmy)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pArmy->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         if (!pArmy->BArmy::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Squad
            syncSquadData("BWorld::update destroy ArmyID", pArmy->getID().asLong());
            #endif
            pArmy->BArmy::destroy();
            releaseObject(pArmy);
         }

         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pArmy=getNextArmy(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pArmy == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mArmyPreAsyncTime = statTime;
   mTimeStats.mAvgArmyPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxArmyPreAsyncTime)
      mTimeStats.mMaxArmyPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Platoons.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncPlatoon);

   #ifndef BUILD_FINAL
   localTimer.start();   
   #endif
   {
      SCOPEDSAMPLE(UpdatePlatoons);  
      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BPlatoon* pPlatoon=getNextPlatoon(handle);
      while (pPlatoon)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pPlatoon->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         if (!pPlatoon->BPlatoon::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Squad
            {
               SCOPEDSAMPLE(UpdatePlatoons_SYNC_Squad);  
               syncSquadData("BWorld::update destroy PlatoonID", pPlatoon->getID().asLong());
            }
            #endif

            pPlatoon->BPlatoon::destroy();
            releaseObject(pPlatoon);
         }
         
         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pPlatoon=getNextPlatoon(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pPlatoon == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }      
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mPlatoonPreAsyncTime = statTime;
   mTimeStats.mAvgPlatoonPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxPlatoonPreAsyncTime)
      mTimeStats.mMaxPlatoonPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Squads.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncSquad);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateSquads);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BSquad* pSquad = getNextSquad(handle);
      while (pSquad)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pSquad->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         if (!pSquad->BSquad::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Squad
            syncSquadData("BWorld::update destroy squadID", pSquad->getID().asLong());
            #endif
            pSquad->BSquad::destroy();
            releaseObject(pSquad);
         }

         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pSquad = getNextSquad(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pSquad == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }      
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mSquadPreAsyncTime = statTime;
   mTimeStats.mAvgSquadPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxSquadPreAsyncTime)
      mTimeStats.mMaxSquadPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Squad plotter debugging.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncSquadPlotter);

   #ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigRenderSquadPlotter))
   {
      //-- FIXING PREFIX BUG ID 5921
      const BUser* pUser = gUserManager.getPrimaryUser();
      //--
      if ((pUser != NULL) && pUser->getFlagHaveHoverPoint())
      {
         BVector hoverPoint = pUser->getHoverPoint();
         gSquadPlotter.debugPlotSquads(hoverPoint);
      }
   }
   #endif

   SUBUPDATENEXT

   //------------------------------------------------------------------------------
   // Units.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncUnit);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateUnits);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BUnit* pUnit = getNextUnit(handle);
      while (pUnit)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pUnit->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         if (pUnit->getFlagDestroy())
         {
            #ifdef SYNC_Unit
            syncUnitData("BWorld::update unitID", pUnit->getID().asLong());
            #endif
            releaseObject(pUnit);
         }
         else if (!pUnit->BUnit::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Unit
            syncUnitData("BWorld::update unitID", pUnit->getID().asLong());
            #endif
            if (!pUnit->getFlagDestroy())
               pUnit->BUnit::destroy();
            releaseObject(pUnit);
         }

         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pUnit = getNextUnit(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pUnit == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mUnitPreAsyncTime = statTime;
   mTimeStats.mAvgUnitPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxUnitPreAsyncTime)
      mTimeStats.mMaxUnitPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Objects.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncObject);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateObjects);  

      uint handle = (uint)-1;
      BObject* pObject;
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (mSubUpdateSectionItem == -1)
            pObject = mpObjectManager->getFirstUpdateObject(handle);
         else
         {
            handle = (uint)mSubUpdateSectionItem;
            pObject = mpObjectManager->getNextUpdateObject(handle);
         }
      }
      else
      #endif
         pObject = mpObjectManager->getFirstUpdateObject(handle);

      while (pObject)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pObject->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         if (!pObject->BObject::updatePreAsync(mLastUpdateLengthFloat))
         {
            pObject->BObject::destroy();
            releaseObject(pObject);
         }

         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pObject = mpObjectManager->getNextUpdateObject(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pObject == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = (int)handle;
      }
      #endif
   }
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mObjectPreAsyncTime = statTime;
   mTimeStats.mAvgObjectPreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxObjectPreAsyncTime)
      mTimeStats.mMaxObjectPreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Dopples.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncDopple);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateDopples);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BDopple* pDopple = getNextDopple(handle);
      while (pDopple)
      {
         if (!pDopple->BDopple::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Dopple
            syncDoppleData("BWorld::update doppleID", pDopple->getID().asLong());
            #endif
            pDopple->BDopple::destroy();
            releaseObject(pDopple);
         }

         SUBUPDATECANRENDERBREAK

         pDopple = getNextDopple(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pDopple == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = (int)handle;
      }
      #endif
   }
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mDopplePreAsyncTime = statTime;
   mTimeStats.mAvgDopplePreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxDopplePreAsyncTime)
      mTimeStats.mMaxDopplePreAsyncTime = statTime;
   #endif

   //------------------------------------------------------------------------------
   // Projectiles.
   //------------------------------------------------------------------------------
   SUBUPDATERETURN(PreAsyncProjectile);

   #ifndef BUILD_FINAL
   localTimer.start();
   #endif
   {
      SCOPEDSAMPLE(UpdateProjectiles);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BProjectile* pProj = getNextProjectile(handle);
      while (pProj)
      {
         if (!pProj->BProjectile::updatePreAsync(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Projectile
            syncProjectileData("BWorld::update projectileID", pProj->getID().asLong());
            #endif
            pProj->BProjectile::destroy();
            releaseObject(pProj);
         }

         SUBUPDATECANRENDERBREAK

         pProj = getNextProjectile(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pProj == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = (int)handle;
      }
      #endif
   }      
   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mProjectilePreAsyncTime = statTime;
   mTimeStats.mAvgProjectilePreAsyncTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxProjectilePreAsyncTime)
      mTimeStats.mMaxProjectilePreAsyncTime = statTime;
   #endif

   #ifndef BUILD_FINAL
   simTimer.stop();
   statTime = simTimer.getElapsedSeconds();
   mTimeStats.mTotalPreAsyncUpdateTime = statTime;
   mTimeStats.mAvgTotalPreAsyncUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTotalPreAsyncUpdateTime)
      mTimeStats.mMaxTotalPreAsyncUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateAsync
//==============================================================================
void BWorld::updateAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency)
{   
   SCOPEDSAMPLE(BWorldUpdateAsync);

   #ifndef BUILD_FINAL
   BTimer simTimer;
   simTimer.start();
   #endif

   if (gDecoupledUpdate)
   {
      SUBUPDATERESUME(AsyncFlush)
   }

   SUBUPDATENEXT

   // Because Objects and Units can SPAWN new objects during their preUpdate phase
   // we wait until here to add the totals to the list for async updating.

   //------------------------------------------------------------------------------
   // Units.
   //------------------------------------------------------------------------------
   {
      BEntityHandle handle = cInvalidObjectID;
      //-- FIXING PREFIX BUG ID 5922
      const BUnit* pUnit = getNextUnit(handle);
      //--
      while (pUnit)
      {
         addAsyncWorkUnit(handle);
         pUnit = getNextUnit(handle);
      }
   }

   //------------------------------------------------------------------------------
   // Objects.
   //------------------------------------------------------------------------------
   {
      uint handle;
      const BObject* pObject = mpObjectManager->getFirstUpdateObject(handle);
      while (pObject)
      {
         addAsyncWorkUnit(pObject->getID());
         pObject = mpObjectManager->getNextUpdateObject(handle);
      }
   }

   //------------------------------------------------------------------------------
   // Dopples.
   //------------------------------------------------------------------------------
   {
      BEntityHandle handle = cInvalidObjectID;
      const BDopple* pDopple = getNextDopple(handle);
      while (pDopple)
      {
         addAsyncWorkUnit(handle);
         pDopple = getNextDopple(handle);
      }
   }

   //------------------------------------------------------------------------------
   // Projectiles.
   //------------------------------------------------------------------------------
   {
      BEntityHandle handle = cInvalidObjectID;
      const BProjectile* pProjectile = getNextProjectile(handle);
      while (pProjectile)
      {
         addAsyncWorkUnit(handle);
         pProjectile = getNextProjectile(handle);
      }
   }

   SUBUPDATERETURN(AsyncFlush)

   if(gConfig.isDefined(cConfigAsyncWorldUpdate) && mWorkUnits.getSize() != 0)
   {
      // Threaded version.
      flushAsyncWorkGroups();
   }
   else
   {
      // Non-threaded version.
      for(uint workIndex=0; workIndex < mWorkUnits.getSize(); workIndex++)
      {
         const BEntityHandle handle = mWorkUnits[workIndex].mEntityHandle;
         uint entityType = handle.getType();
         switch(entityType)
         {
            case BEntity::cClassTypeUnit:
            {
               BUnit* pUnit =getUnit(handle,true); 
               pUnit->BUnit::updateAsync(mLastUpdateLengthFloat);
               break;
            }
            case BEntity::cClassTypeObject:
            {
               BObject* pObject = getObject(handle,true); 
               pObject->BObject::updateAsync(mLastUpdateLengthFloat);
               break;
            }
            case BEntity::cClassTypeDopple:
            {    
               BDopple* pDopple = getDopple(handle,true); 
               pDopple->BDopple::updateAsync(mLastUpdateLengthFloat);
               break;
            }
            case BEntity::cClassTypeProjectile:
            {
               BProjectile* pProjectile = getProjectile(handle,true); 
               pProjectile->BProjectile::updateAsync(mLastUpdateLengthFloat);
               break;
            }
            default:
               break;
         }
      }

      clearAsyncWorkGroups(); 
   }

   SUBUPDATENEXT

   #ifndef BUILD_FINAL
   simTimer.stop();
   double statTime = simTimer.getElapsedSeconds();
   mTimeStats.mTotalAsyncUpdateTime = statTime;
   mTimeStats.mAvgTotalAsyncUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTotalAsyncUpdateTime)
      mTimeStats.mMaxTotalAsyncUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::beginSubUpdate
//==============================================================================
void BWorld::beginSubUpdate()
{
   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   mSubUpdateBeganTime = time.QuadPart;

   updateGrannySync(isOnFirstSubUpdate());
}

//==============================================================================
// BWorld::canRenderInterpolatedFrame
//==============================================================================
bool BWorld::canRenderInterpolatedFrame()
{
   if (mUpdateNumber == 0) // it's the first update - just run it
      return false;

   #ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate)
   {
      /*
      double renderElapsed = mDecoupledRenderTimer.getElapsedSeconds();
      double outsideAvg = static_cast<double>(mpTimingTracker->getOutsideUpdateAverage()) * 0.001;
      double frameAvg = static_cast<double>(mpTimingTracker->getFrameAverage()) * 0.001;
      */
      
      //trace("timer=%0.2f  dcrt=%0.2f", 1000.0f*mDecoupledRenderTimer.getElapsedSeconds(), 1000.0f*mDecoupledUpdateRenderTime);
      
      if (mDecoupledRenderTimer.getElapsedSeconds() >= mDecoupledUpdateRenderTime)
      {
         // jce [10/24/2008] -- If the renderer can't accept more data right now, don't stop to render.
         static bool checkFrameAvail = true;
         if(checkFrameAvail && !gRenderThread.isFrameAvailable())
            return(false);
          
         // jce [10/27/2008] -- If we haven't finished an update since the last render...  
         // jce [10/27/2008] -- nevermind, this update finished check makes for fast-fast-short frame patterns
         //if(!mDecoupledFinishedAnUpdate)
         {
            // See if we've done a reasonable amount of work updating.  If we haven't, don't render.  This will occur
            // when the update submission time (and other ancillary stuff) is taking a long time and would otherwise completely
            // starve the update so time cannot move forward at all.
            static float cMinUpdateFactor = 0.25f;
            if(mDecoupledWorkSinceLastRenderTimer.getElapsedSeconds() < cMinUpdateFactor*mDecoupledUpdateAvgUpdateTime)
               return(false);
         }

         if (mDecoupledUpdateSkipRenders)
            return true;
         else
         {
            double timeLeft = mDecoupledUpdateAvgUpdateTime - (mDecoupledUpdateElapsed + mDecoupledUpdateTimer.getElapsedSeconds());
            if (timeLeft >= mDecoupledUpdateRenderTime)
               return true;
         }
      }
      return false;
   }
   #endif

   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   float elapsedRealTime = ((time.QuadPart - mLastUpdateRealtime) / mLastTimerFrequency);
   float elapsedGameTime = ((getSubGametime() - (mGametime - mLastUpdateLength))/1000.0f) + mAccumulatedTimeError;   

   if (gConfig.isDefined(cConfigSubUpdTrace))
   {
      gConsole.output(cChannelSim, "  can we render an interpolated frame?");
      gConsole.output(cChannelSim, "    getSubGametime() %d, mGametime %d, mLastUpdateLength %d, mAccumulatedTimeError %f",
         getSubGametime(), mGametime, mLastUpdateLength, mAccumulatedTimeError);
      gConsole.output(cChannelSim, "    elapsedGameTime %f, elapsedRealTime %f, outsideUpdateAverage %d",
         elapsedGameTime, elapsedRealTime, mpTimingTracker->getOutsideUpdateAverage());
   }

   if (elapsedGameTime > elapsedRealTime && elapsedGameTime - elapsedRealTime >= mpTimingTracker->getOutsideUpdateAverage() / 1000.0f) // have we pulled enough ahead to render a frame?
   {
      if (gConfig.isDefined(cConfigSubUpdTrace))
         gConsole.output(cChannelSim, "  yep");
      return true;
   }
   else
   {
      if (gConfig.isDefined(cConfigSubUpdTrace))
         gConsole.output(cChannelSim, "  nope");
      return false;
   }
}

//==============================================================================
// BWorld::updateFrame
//
// This method is for stuff that needs to be updated every frame no matter what, but don't put code here!
// It's way kinder on performance if you can put your code in updatePreAsync(), updateAsync(), or update() 
// instead which happen at 10hz, rather than this function which attempts to run at 30hz.
// Come see PaulB or Angelo if that doesn't make sense.
//==============================================================================
void BWorld::updateFrame()
{
   SCOPEDSAMPLE(UpdateFrame);  

   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   if(mLastSubUpdateRealtime!=0 && mLastTimerFrequency != 0)
   {
      int64 delta = (int64)(time.QuadPart - mLastSubUpdateRealtime);
      mSubTotalRealtime += delta/mLastTimerFrequency;
   }
   mLastSubUpdateRealtime=time.QuadPart;

   updateTextVisualManager();
}

//==============================================================================
//==============================================================================
bool BWorld::shouldEndSubUpdate()
{
   if (isOnLastSubUpdate()) // if we're already on the last sub-update, just keep going
      return false;

   // if we have already taken up enough time for this sub-update, return true
   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   DWORD deltaMsecs = ((time.QuadPart - mSubUpdateBeganTime) / mLastTimerFrequency) * 1000;
   if (deltaMsecs > mpTimingTracker->getUpdateAverage())
      return true;
   else
      return false;
}

//==============================================================================
//==============================================================================
bool BWorld::isOnFirstSubUpdate() const
{ 
   if (gEnableSubUpdating)
   {
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
         return mSubUpdateSection == cSubUpdateEnd;
      else
      #endif
         return mSubUpdate % getAmountOfSubUpdates() == 0; 
   }
   else
      return true;
}

//==============================================================================
//==============================================================================
bool BWorld::isOnLastSubUpdate() const
{ 
   if (gEnableSubUpdating)
   {
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
         return mSubUpdateSection == cSubUpdateEnd;
      else
      #endif
         return (mSubUpdate+1) % getAmountOfSubUpdates() == 0; 
   }
   else
      return true;
}

//==============================================================================
//==============================================================================
long BWorld::calcAdjustedSubUpdateTime()
{
   long time = cSubUpdateTimeInMsecs;

   long avgFrameTime = (long)(gRender.getAverageFrameTime() * 1000.0);
   if (avgFrameTime <= cSubUpdateTimeInMsecs)
      time = cSubUpdateTimeInMsecs;
   else
   {
      long maxTime = 66;
      gConfig.get(cConfigMaxSubUpdateTime, &maxTime);
      if (avgFrameTime >= maxTime)
         time = maxTime;
      else
         time = avgFrameTime;
   }

   return time;
}

//==============================================================================
//==============================================================================
void BWorld::setSubUpdating(bool on, bool alternate, bool dynamicTime, bool decoupled)
{
   mNextEnableSubUpdating = on;
   #ifdef DECOUPLED_UPDATE
   mNextDecoupledUpdate = decoupled;
   #endif
   mNextAlternateSubUpdating = alternate;
   mNextDynamicSubUpdateTime = dynamicTime;
}

//==============================================================================
// BWorld::updatePostAsync
//==============================================================================
void BWorld::updatePostAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency)
{
   #ifndef BUILD_FINAL   
   BTimer simTimer;
   simTimer.start();
   double statTime = 0;
   #endif

   SCOPEDSAMPLE(BWorldUpdatePostAsync);

   if (gEnableSubUpdating && !gDecoupledUpdate)
   {
      // Don't do anything here when alternate sub updating is on and this is the first sub update.
      // This is because alternate sub updating breaks the update into these 3 parts:
      // 1. Pre-async update and async update.
      // 2. First part of post async update and first 30% of unit update.
      // 3. Last 70% of unit update.
      if (mAlternateSubUpdating && isOnFirstSubUpdate())
         return;

      if (mFlagStartedUpdate && mLastSubUpdateUnit != cInvalidObjectID)
         updateUnits(); // continue updating units on subsequent sub-updates if we have any left to update
      
      if (!mFlagStartedUpdate) // is this our first time through updatePostAsync for this update?
      {
         mFlagStartedUpdate = true;         

         // Before we update anything, update all the physics data that must be done single threaded
         // but was set up as part of the previous multithreaded update
         updateSingleThreadedPhysicsData();

         //if (!gConfig.isDefined(cConfigFlashGameUI))
         //   gMiniMap.prep();
         
         updateUnits(); // start updating units                  
      }
       
      if (isOnLastSubUpdate()) // stuff we do on the last sub-update
      {
         updateObjects();
         updateDopples();
         updateProjectiles();      
         updateMinimap(); 
         updateEntityScheduler();
         updateSoundManager();
         updateBattleManager();
         updatePowerManager();
         updateTriggerManager();
         updatePhysics();
         updateLightManager(); 
         updateMinimapBounds();
         updatePathingLimiter();
         updateExplorationGroups();         

         gUIManager->commitMinimap();

         #ifdef UNITOFFENDERS
         //Flag to reset the offender list.
         mFlagClearUnitOffenders=true;
         #endif

         mUpdateNumber++;
         BSyncManager::getInstance()->nextUpdate();

         mFlagStartedUpdate = false;
      }           
   }
   else
   {
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         SUBUPDATERESUME(PostAsyncEnd)
         SUBUPDATERESUME(PostAsyncCommitMinimap)
         SUBUPDATERESUME(PostAsyncExplorationGroups)
         SUBUPDATERESUME(PostAsyncPathingLimiter)
         SUBUPDATERESUME(PostAsyncMinimapBounds)
         SUBUPDATERESUME(PostAsyncLightManager)
         SUBUPDATERESUME(PostAsyncPhysics)
         SUBUPDATERESUME(PostAsyncTriggerManager)
         SUBUPDATERESUME(PostAsyncPowerManager)
         SUBUPDATERESUME(PostAsyncBattleManager)
         SUBUPDATERESUME(PostAsyncSoundManager)
         SUBUPDATERESUME(PostAsyncEntityScheduler)
         SUBUPDATERESUME(PostAsyncMinimap)
         SUBUPDATERESUME(PostAsyncProjectiles)
         SUBUPDATERESUME(PostAsyncDopples)
         SUBUPDATERESUME(PostAsyncObjects)
         SUBUPDATERESUME(PostAsyncUnits)
         SUBUPDATERESUME(PostAsyncPhysicsBreakOffObjects)
      }
      #endif

      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncPhysicsBreakOffObjects);

      // Before we update anything, update all the physics data that must be done single threaded
      // but was set up as part of the previous multithreaded update
      updateSingleThreadedPhysicsData();

      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.prep();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncUnits);
      updateUnits();

      SUBUPDATERETURN(PostAsyncObjects);
      updateObjects();

      SUBUPDATERETURN(PostAsyncDopples);
      updateDopples();

      SUBUPDATERETURN(PostAsyncProjectiles);
      updateProjectiles();      

      SUBUPDATERETURN(PostAsyncMinimap);
      updateMinimap(); 
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncEntityScheduler);
      updateEntityScheduler();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncSoundManager);
      updateSoundManager();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncBattleManager);
      updateBattleManager();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncPowerManager);
      updatePowerManager();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncTriggerManager);
      updateTriggerManager();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncPhysics);
      updatePhysics();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncLightManager);
      updateLightManager(); 
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncMinimapBounds);
      updateMinimapBounds();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncPathingLimiter);
      updatePathingLimiter();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncExplorationGroups);
      updateExplorationGroups();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncCommitMinimap);
      gUIManager->commitMinimap();
      SUBUPDATENEXT

      SUBUPDATERETURN(PostAsyncEnd);

      #ifdef UNITOFFENDERS
      //Flag to reset the offender list.
      mFlagClearUnitOffenders=true;
      #endif

      mUpdateNumber++;
      BSyncManager::getInstance()->nextUpdate();
      SUBUPDATENEXT
   }

   #ifndef BUILD_FINAL
   simTimer.stop();
   statTime = simTimer.getElapsedSeconds();
   mTimeStats.mTotalPostAsyncUpdateTime = statTime;
   mTimeStats.mAvgTotalPostAsyncUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTotalPostAsyncUpdateTime)
      mTimeStats.mMaxTotalPostAsyncUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateUnits
//==============================================================================
void BWorld::updateUnits()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   BTimer unitTimer;   
   static double statTime = 0;
   BEntityID unitID;
   #endif

   #ifndef BUILD_FINAL
   checkForDirtyPlayerColors();
   #endif

   #ifdef UNITOFFENDERS
   if (mFlagClearUnitOffenders)
   {
      mUnitOffenders.clear();
      mFlagClearUnitOffenders=false;
   }
   #endif

   {
      SCOPEDSAMPLE(updateUnits_UpdateUnits);  

      long unitSubUpdateCount = 0;

      long numUnitsToUpdate = 0;

      if (gEnableSubUpdating && mAlternateSubUpdating)
      {
         long totalUnits = (long)mpObjectManager->getNumberUnits();
         numUnitsToUpdate = totalUnits;
         if (!isOnLastSubUpdate())
         {
            numUnitsToUpdate = (long)(((float)totalUnits) * 0.7f);
            if (numUnitsToUpdate < 1)
               numUnitsToUpdate = 1;
         }
      }

      //-- reset handle
      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : mLastSubUpdateUnit);
      #else
      BEntityHandle handle = mLastSubUpdateUnit;
      #endif
      BUnit* pUnit = getNextUnit(handle);
      while (pUnit)
      {
         #ifndef BUILD_FINAL
         //Start the timer for the unit update.  Track whether or not this unit is a triggered unit or not.
         bool triggerObject=pUnit->getFlagIsTriggered();
         unitTimer.start();
         unitID=pUnit->getID();
         #endif

         if (pUnit->getFlagDestroy())
         {
            #ifdef SYNC_Unit
            syncUnitData("BWorld::update unitID", pUnit->getID().asLong());
            #endif
            releaseObject(pUnit);
         }
         else if (pUnit->getFlagNoWorldUpdate())
         {
            pUnit = getNextUnit(handle);
            continue;
         }
         else if (!pUnit->BUnit::update(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Unit
            syncUnitData("BWorld::update unitID", pUnit->getID().asLong());
            #endif
            if (!pUnit->getFlagDestroy())
               pUnit->BUnit::destroy();
            releaseObject(pUnit);
         }

         #ifndef BUILD_FINAL
         //Unit Timer.
         unitTimer.stop();
         if (triggerObject)
            mTimeStats.mTriggerObjectsUpdateTime += unitTimer.getElapsedSeconds();
         #endif

         #ifdef UNITOFFENDERS
         //Add this unit into the offender tracking.
         addUnitOffender(unitID, unitTimer.getElapsedSeconds());
         #endif

         mLastSubUpdateUnit = handle;

         // should we bail out and let the rest of the units happen on the next sub-update? 
         if (gEnableSubUpdating && !gDecoupledUpdate)
         {  
            if (mAlternateSubUpdating)
            {
               if (++unitSubUpdateCount == numUnitsToUpdate)
                  break;
            }
            else
            {
               // check every 10 units                        
               if (++unitSubUpdateCount == 10)
               {
                  if (shouldEndSubUpdate())
                     break;

                  unitSubUpdateCount = 0;
               }
            }
         }

         SUBUPDATECANRENDERBREAK

         pUnit = getNextUnit(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pUnit == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      else 
      #endif
      {
         if (pUnit == NULL)
           mLastSubUpdateUnit = cInvalidObjectID;
      }
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime += localTimer.getElapsedSeconds();
   if (mLastSubUpdateUnit == cInvalidObjectID)      
   {
      mTimeStats.mUnitUpdateTime = statTime;
      mTimeStats.mAvgUnitUpdateTime.addSample(statTime);
      if (statTime > mTimeStats.mMaxUnitUpdateTime)
         mTimeStats.mMaxUnitUpdateTime = statTime;
      statTime = 0;
   }   
   #endif
}

//==============================================================================
// BWorld::updateObjects
//==============================================================================
void BWorld::updateObjects()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   BTimer triggerObjectsTimer;  
   localTimer.start();
   double statTime = 0;
   #endif
   {
      SCOPEDSAMPLE(UpdateObjects);  

      uint handle = (uint)-1;
      BObject* pObject;
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (mSubUpdateSectionItem == -1)
            pObject = mpObjectManager->getFirstUpdateObject(handle);
         else
         {
            handle = (uint)mSubUpdateSectionItem;
            pObject = mpObjectManager->getNextUpdateObject(handle);
         }
      }
      else
      #endif
         pObject = mpObjectManager->getFirstUpdateObject(handle);

      while (pObject)
      {
         #ifndef BUILD_FINAL       
         // Trigger objects
         bool triggerObject = pObject->getFlagIsTriggered();
         if (triggerObject)
            triggerObjectsTimer.start();
         #endif

         // Halwes - 10/2/2008 - If sub-updating skip beam heads and/or tails since those are updated directly in the unit's attack action.
         if (pObject->getFlagNoWorldUpdate() && !pObject->getFlagDestroy())
         {
            pObject = mpObjectManager->getNextUpdateObject(handle);
            continue;
         }

         if (!pObject->BObject::update(mLastUpdateLengthFloat))
         {
            pObject->BObject::destroy();
            releaseObject(pObject);
         }

         #ifndef BUILD_FINAL
         // Trigger objects
         if (triggerObject)
         {
            triggerObjectsTimer.stop();
            mTimeStats.mTriggerObjectsUpdateTime += triggerObjectsTimer.getElapsedSeconds();
         }
         #endif

         SUBUPDATECANRENDERBREAK

         pObject = mpObjectManager->getNextUpdateObject(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pObject == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = (int)handle;
      }
      #endif
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mObjectUpdateTime = statTime;
   mTimeStats.mAvgObjectUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxObjectUpdateTime)
      mTimeStats.mMaxObjectUpdateTime = statTime;

   // Trigger objects
   mTimeStats.mAvgTriggerObjectsUpdateTime.addSample(mTimeStats.mTriggerObjectsUpdateTime);
   if (mTimeStats.mTriggerObjectsUpdateTime > mTimeStats.mMaxTriggerObjectsUpdateTime)
      mTimeStats.mMaxTriggerObjectsUpdateTime = mTimeStats.mTriggerObjectsUpdateTime;
   #endif
}

//==============================================================================
// BWorld::updateDopples
//==============================================================================
void BWorld::updateDopples()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;   
   localTimer.start();
   double statTime = 0;
   #endif
   {
      SCOPEDSAMPLE(UpdateDopples);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BDopple* pDopple = getNextDopple(handle);
      while (pDopple)
      {
         if (!pDopple->BDopple::update(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Dopple
            syncDoppleData("BWorld::update doppleID", pDopple->getID().asLong());
            #endif
            pDopple->BDopple::destroy();
            releaseObject(pDopple);
         }

         SUBUPDATECANRENDERBREAK

         pDopple = getNextDopple(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pDopple == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mDoppleUpdateTime = statTime;
   mTimeStats.mAvgDoppleUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxDoppleUpdateTime)
      mTimeStats.mMaxDoppleUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateProjectiles
//==============================================================================
void BWorld::updateProjectiles()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif
   {
      SCOPEDSAMPLE(UpdateProjectiles);  

      #ifdef DECOUPLED_UPDATE
      BEntityHandle handle = (gDecoupledUpdate ? BEntityHandle(mSubUpdateSectionItem) : cInvalidObjectID);
      #else
      BEntityHandle handle = cInvalidObjectID;
      #endif
      BProjectile* pProj = getNextProjectile(handle);
      while (pProj)
      {
         // Halwes - 10/2/2008 - If sub-updating skip beams since those are updated directly in the unit's attack action.
         if (pProj->getFlagNoWorldUpdate() && !pProj->getFlagDestroy())
         {
            pProj = getNextProjectile(handle);
            continue;
         }

         if (!pProj->BProjectile::update(mLastUpdateLengthFloat))
         {
            #ifdef SYNC_Projectile
            syncProjectileData("BWorld::update projectileID", pProj->getID().asLong());
            #endif
            pProj->BProjectile::destroy();
            releaseObject(pProj);
         }

         SUBUPDATECANRENDERBREAK

         pProj = getNextProjectile(handle);
      }

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         if (pProj == NULL)
            SUBUPDATENEXT
         else
            mSubUpdateSectionItem = handle.asLong();
      }
      #endif
   }      

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mProjectileUpdateTime = statTime;
   mTimeStats.mAvgProjectileUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxProjectileUpdateTime)
      mTimeStats.mMaxProjectileUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateMinimap
//==============================================================================
void BWorld::updateMinimap()
{
   if (mFlagNoFogSim || mFlagNoFogVis)
   {
      float size=gTerrainSimRep.tileToWorld(gTerrainSimRep.getNumXDataTiles());
      gUIManager->revealMinimap(BVector(size*0.5f, 0.0f, size*0.5f), size, size);
   }
}

//==============================================================================
// BWorld::updateEntityScheduler
//==============================================================================
void BWorld::updateEntityScheduler()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   //Entity Scheduler.
   //DCP 05/17/07: Put the order manager in here, too.
   {
      SCOPEDSAMPLE(UpdateEntityScheduler);  
      //DCP 10/15/08: Neuter the entity scheduler now that we are doing that a different way.
      //gEntityScheduler.update();
      gSimOrderManager.update();
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mEntitySchedulerUpdateTime = statTime;
   mTimeStats.mAvgEntitySchedulerUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxEntitySchedulerUpdateTime)
      mTimeStats.mMaxEntitySchedulerUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateTextVisualManager
//==============================================================================
void BWorld::updateTextVisualManager()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   {
      SCOPEDSAMPLE(UpdateTextVisualManager);  
      gTextVisualManager.update(gWorld->getLastUpdateLength());
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mTextVisualUpdateTime = statTime;
   mTimeStats.mAvgTextVisualUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTextVisualUpdateTime)
      mTimeStats.mMaxTextVisualUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateSoundManager
//==============================================================================
void BWorld::updateSoundManager()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   {
      SCOPEDSAMPLE(UpdateWorldSoundManager);  
      mpWorldSoundManager->update(mLastUpdateLengthFloat);
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mSoundUpdateTime = statTime;
   mTimeStats.mAvgSoundUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxSoundUpdateTime)
      mTimeStats.mMaxSoundUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateBattleManager
//==============================================================================
void BWorld::updateBattleManager()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   {
      SCOPEDSAMPLE(UpdateBattleManager);  
      mpBattleManager->updateBattles();
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mBattleUpdateTime = statTime;
   mTimeStats.mAvgBattleUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxBattleUpdateTime)
      mTimeStats.mMaxBattleUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updatePowerManager
//==============================================================================
void BWorld::updatePowerManager()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   {
      SCOPEDSAMPLE(UpdatePowerManager);  
      mpPowerManager->update(mGametime, mLastUpdateLengthFloat);
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mPowerUpdateTime = statTime;
   mTimeStats.mAvgPowerUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxPowerUpdateTime)
      mTimeStats.mMaxPowerUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateTriggerManager
//==============================================================================
void BWorld::updateTriggerManager()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   // Don't update trigger manager while in the cinematic viewer mode.  This viewer will not ship with
   // the game and is just for development.  The cinematic viewer allows you to preview any wow moment, 
   // we don't want triggers at the start of a scenario to play when previewing a wow moment that may
   // actually occur half way through the scenario.
   if (!getFlagGameOver() && (gModeManager.getModeType() != BModeManager::cModeCinematic))
   {
      SCOPEDSAMPLE(UpdateTriggerManager);  
      gTriggerManager.update(mGametime);
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds();
   mTimeStats.mTriggerUpdateTime = statTime;
   mTimeStats.mAvgTriggerUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxTriggerUpdateTime)
      mTimeStats.mMaxTriggerUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updatePhysics
//==============================================================================
void BWorld::updatePhysics()
{
   #ifndef BUILD_FINAL   
   BTimer localTimer;
   localTimer.start();
   double statTime = 0;
   #endif

   // Physics - this kicks off physics update threads that assume exclusive access to physics data, so
   // nothing in the sim should touch physics once this starts until the update is complete
   #ifndef BUILD_FINAL
   localTimer.start();
   #endif

   {
      SCOPEDSAMPLE(UpdatePhysics);

      // Update havok visual debugger's camera
      #ifndef BUILD_FINAL
      {
         //-- FIXING PREFIX BUG ID 5927
         const BCamera* pCam = gUserManager.getPrimaryUser()->getCamera();
         //--
         mpPhysicsWorld->updateHavokDebuggerCamera(pCam->getCameraLoc(), 
            pCam->getCameraLoc() + pCam->getCameraDir(),
            pCam->getCameraUp(), 0.1f, 2500.0f, pCam->getFOV() * 180.0f / cPi);
      }
      #endif

      // Batch update phantoms
      batchUpdatePhantoms();

      // Updating the physics world, so unmark the world for writing
      mpPhysicsWorld->markForWrite(false);

      // Update
      mpPhysicsWorld->update(mLastUpdateLengthFloat);
   }

   #ifndef BUILD_FINAL
   localTimer.stop();
   statTime = localTimer.getElapsedSeconds() + mpPhysicsWorld->getLastWaitTime();
   mTimeStats.mPhysicsUpdateTime = statTime;
   mTimeStats.mAvgPhysicsUpdateTime.addSample(statTime);
   if (statTime > mTimeStats.mMaxPhysicsUpdateTime)
      mTimeStats.mMaxPhysicsUpdateTime = statTime;
   #endif
}

//==============================================================================
// BWorld::updateLightManager
//==============================================================================
void BWorld::updateLightManager()
{
   //Light manager
   gSimSceneLightManager.updateAnimation(   gWorld->getGametime() );
}

//==============================================================================
// BWorld::updateMinimapBounds
//==============================================================================
void BWorld::updateMinimapBounds()
{
   SCOPEDSAMPLE(UpdateMinimapBounds);

   // Update minimap with blockers for areas outside of playable bounds.
   if (mFlagOutsideBoundsBlocked)
   {
      for (uint i=0; i<4; i++)
      {
         BVector center=mOutsideOfBoundsBlockerCenter[i];
         if (center != cInvalidVector)
         {
            BVector rectSize=mOutsideOfBoundsBlockerSize[i] * 2.0f;
            gUIManager->blockMinimap(center, rectSize.x, rectSize.z);
         }
      }
   }
}

//==============================================================================
// BWorld::updatePathingLimiter
//==============================================================================
void BWorld::updatePathingLimiter()
{
   SCOPEDSAMPLE(UpdatePathingLimiter);
   // DLM 11/10/08 -This is handled internally now
   // Pathing Limiter
   /*
   for(long i=0; i<mPlayers.getNumber(); i++)
   {
      if (mpPathingLimiter->getPathsDeniedLastFrame(i) > 0)
         mpPathingLimiter->incFramesDenied(i);
      else
         mpPathingLimiter->setFramesDenied(i, 0);
   }
   */
   mpPathingLimiter->prioritizePathing();
}

//==============================================================================
// BWorld::updateExplorationGroups
//==============================================================================
void BWorld::updateExplorationGroups()
{
   SCOPEDSAMPLE(UpdateExplorationGroups);

   // Exploration Groups
   uint i = 0;
   while (i < mActiveExplorationGroups.getSize())
   {
      mActiveExplorationGroups.get(i).timeLeft -= mLastUpdateLengthFloat;

      if (mActiveExplorationGroups.get(i).timeLeft <= 0.0)
      {
         stopActiveExplorationGroup(i);
         mActiveExplorationGroups.removeIndex(i, true);
      }
      else
      {
         ++i;
      }
   }
}

//==============================================================================
// BWorld::renderVisualInstances
//==============================================================================
void BWorld::renderVisualInstances(BUser* pUser)
{
   if (!pUser)
      return;

   SCOPEDSAMPLE(BWorldRender);

   #ifndef BUILD_FINAL
      BTimer localTimer;   
      BTimer totalTimer;
      double statTime = 0;
      totalTimer.start();
      
      uint numUnitsRendered = 0;
      uint numProjectilesRendered = 0;
      uint numObjectsRendered = 0;
      uint numDopplesRendered = 0;
   #endif

   mFlagAllVisible = (gConfig.isDefined(cConfigNoFogMask) || mFlagNoFogSim || mFlagNoFogVis);

   //------------------------------------------------------------------------------
   // Sky.
   //------------------------------------------------------------------------------
   if ((mpSkyVisual) && (mFlagAllVisible))
   {
      XMMATRIX worldMatrix = XMMatrixIdentity();
      worldMatrix.r[3] = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();
      gRender.setWorldMatrix(worldMatrix);

      BVisualRenderAttributes renderAttributes;
      renderAttributes.mFarLayer = true;

      mpSkyVisual->render(&renderAttributes);
   }

   BPlayer*  pPlayer= pUser->getPlayer();
   BTeamID teamID=pPlayer->getTeamID();
   BTeam*  pTeam=pPlayer->getTeam();
   BEntityHandle handle=cInvalidObjectID;
   
   if (pTeam)
   {
      //------------------------------------------------------------------------------
      // Squads.
      //------------------------------------------------------------------------------
      #ifndef BUILD_FINAL
         localTimer.start();
      #endif
      {
         uint numObjects = (uint) getSquadsHighWaterMark();
         uint type = BEntity::cClassTypeSquad << 28;
         for (uint i = 0; i < numObjects; i++)
         {
            BSquad* pObject = getSquad(type | i, true);
            // Don't render if we're occluded or garrisoned               
            if (pObject && (mFlagAllVisible || pObject->isVisibleOnScreen() && (!pObject->getFlagGarrisoned() || pObject->getFlagInCover())))
            {
               pObject->BSquad::render();
            }
         }
      }
      #ifndef BUILD_FINAL
         localTimer.stop();
         statTime = localTimer.getElapsedSeconds();
         mTimeStats.mSimRenderSquadsTime = statTime;
         mTimeStats.mAvgSimRenderSquadsTime.addSample(statTime);
         if (statTime > mTimeStats.mMaxSimRenderSquadsTime)
            mTimeStats.mMaxSimRenderSquadsTime = statTime;
      #endif

      //------------------------------------------------------------------------------
      // Units.
      //------------------------------------------------------------------------------
      #ifndef BUILD_FINAL
         localTimer.start();
      #endif
      {
         const BBitArray*  pBitArray=pTeam->getVisibleUnits();
         uint numObjects = (uint) getUnitsHighWaterMark();
         uint type = BEntity::cClassTypeUnit << 28;
         for (uint i = 0; i < numObjects; i++)
         {
            if (mFlagAllVisible || pBitArray->isBitSet(i))
            {
               // Don't render if we're occluded or garrisoned
               BUnit* pObject = getUnit(type | i, true);
               if (pObject && !pObject->getFlagOccluded() && (!pObject->getFlagGarrisoned() || pObject->getFlagInCover()))
               {
                  const BBoundingBox*  pBoundingBox = pObject->getVisualBoundingBox();
                  if (pObject->getFlagNearLayer() || gRenderDraw.getMainActiveVolumeCuller().isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
                  {
                     pObject->BUnit::render();
                     #ifndef BUILD_FINAL
                        numUnitsRendered++;
                     #endif
                  }
               }                     
            }
         }
      }
      #ifndef BUILD_FINAL
         localTimer.stop();
         statTime = localTimer.getElapsedSeconds();
         mTimeStats.mSimRenderUnitsTime = statTime;
         mTimeStats.mAvgSimRenderUnitsTime.addSample(statTime);
         if (statTime > mTimeStats.mMaxSimRenderUnitsTime)
            mTimeStats.mMaxSimRenderUnitsTime = statTime;
      #endif

      //------------------------------------------------------------------------------
      // Projectiles.
      //------------------------------------------------------------------------------
      #ifndef BUILD_FINAL
         localTimer.start();
      #endif
      {
         const BBitArray*  pBitArray=pTeam->getVisibleProjectiles();
         uint numObjects = (uint) getProjectilesHighWaterMark();
         uint type = BEntity::cClassTypeProjectile << 28;
         for (uint i = 0; i < numObjects; i++)
         {
            if (mFlagAllVisible || pBitArray->isBitSet(i))
            {
               BProjectile *pObject = getProjectile(type | i, true);
               if (pObject)
               {
                  const BBoundingBox*  pBoundingBox = pObject->getVisualBoundingBox();
                  if (gRenderDraw.getMainActiveVolumeCuller().isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
                  {
                     pObject->BProjectile::render();
                     #ifndef BUILD_FINAL
                        numProjectilesRendered++;
                     #endif                  
                  }
               }                     
            }
         }
      }
      #ifndef BUILD_FINAL
         localTimer.stop();
         statTime = localTimer.getElapsedSeconds();
         mTimeStats.mSimRenderProjectilesTime = statTime;
         mTimeStats.mAvgSimRenderProjectilesTime.addSample(statTime);
         if (statTime > mTimeStats.mMaxSimRenderProjectilesTime)
            mTimeStats.mMaxSimRenderProjectilesTime = statTime;
      #endif

      //------------------------------------------------------------------------------
      // Objects.
      //------------------------------------------------------------------------------
      #ifndef BUILD_FINAL
         localTimer.start();
      #endif
      {
         const BBitArray*  pBitArray=pTeam->getVisibleObjects();
         uint numObjects = (uint) getObjectsHighWaterMark();
         uint type = BEntity::cClassTypeObject << 28;
         for (uint i = 0; i < numObjects; i++)
         {
            if (mFlagAllVisible || pBitArray->isBitSet(i))
            {
               BObject *pObject = getObject(type | i, true);
               if (pObject && !pObject->getFlagOccluded())
               {
                  const BBoundingBox*  pBoundingBox = pObject->getVisualBoundingBox();
                  if (gRenderDraw.getMainActiveVolumeCuller().isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
                  {
                     pObject->BObject::render();
                     #ifndef BUILD_FINAL
                        numObjectsRendered++;
                     #endif
                  }
               }                     
            }
         }
      }
      #ifndef BUILD_FINAL
         localTimer.stop();
         statTime = localTimer.getElapsedSeconds();
         mTimeStats.mSimRenderObjectsTime = statTime;
         mTimeStats.mAvgSimRenderObjectsTime.addSample(statTime);
         if (statTime > mTimeStats.mMaxSimRenderObjectsTime)
            mTimeStats.mMaxSimRenderObjectsTime = statTime;
      #endif
   }      

   //------------------------------------------------------------------------------
   // Dopples.
   //------------------------------------------------------------------------------
   if (!mFlagAllVisible)
   {
      #ifndef BUILD_FINAL
         localTimer.start();
      #endif
      handle=cInvalidObjectID;
      for (BDopple*  pObject=gWorld->getNextDopple(handle); pObject!=NULL; pObject=gWorld->getNextDopple(handle))
      {
         if(pObject->getForTeamID()!=teamID)
            continue;
         const BBoundingBox*  pBoundingBox = pObject->getVisualBoundingBox();
         if (gRenderDraw.getMainActiveVolumeCuller().isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
         {
            pObject->BDopple::render();
            #ifndef BUILD_FINAL
               numDopplesRendered++;
            #endif         
         }
      }
      #ifndef BUILD_FINAL
         localTimer.stop();
         statTime = localTimer.getElapsedSeconds();
         mTimeStats.mSimRenderDopplesTime = statTime;
         mTimeStats.mAvgSimRenderDopplesTime.addSample(statTime);
         if (statTime > mTimeStats.mMaxSimRenderDopplesTime)
            mTimeStats.mMaxSimRenderDopplesTime = statTime;
      #endif
   }

   //------------------------------------------------------------------------------
   // Cinematics.
   //------------------------------------------------------------------------------
   mpCinematicManager->render();

   #ifndef BUILD_FINAL
      localTimer.start();
      if(gConfig.isDefined(cConfigRenderAlerts))
      {
         for(uint i = 0; i < mPlayers.getSize(); i++)
         {
            if(!mPlayers[i])
               continue;
            mPlayers[i]->render();
         }
      }
      localTimer.stop();
      statTime = localTimer.getElapsedSeconds();
      mTimeStats.mSimRenderPlayersTime = statTime;
      mTimeStats.mAvgSimRenderPlayersTime.addSample(statTime);
      if (statTime > mTimeStats.mMaxSimRenderPlayersTime)
         mTimeStats.mMaxSimRenderPlayersTime = statTime;

      totalTimer.stop();
      statTime = totalTimer.getElapsedSeconds();
      mTimeStats.mTotalSimRenderTime = statTime;
      mTimeStats.mAvgSimRenderTime.addSample(statTime);
      if (statTime > mTimeStats.mMaxSimRenderTime)
         mTimeStats.mMaxSimRenderTime = statTime;
      
      mRenderedUnitsLastFrame       = numUnitsRendered;
      mRenderedProjectilesLastFrame = numProjectilesRendered;
      mRenderedObjectsLastFrame     = numObjectsRendered;
      mRenderedDopplesLastFrame     = numDopplesRendered;
   #endif 
}

//==============================================================================
// BWorld::debugRender
//==============================================================================
void BWorld::debugRender()
{
   mFlagAllVisible = (gConfig.isDefined(cConfigNoFogMask) || mFlagNoFogSim || mFlagNoFogVis);

   #ifndef BUILD_FINAL
   
   gObsManager.render(gGame.getObstructionRenderMode());

   //-- Render Pather
   gPather.render();

   //-- Render Terrain Flatten Regions
   if (gConfig.isDefined(cConfigRenderSquadPlotter))
      renderTerrainFlattenRegions();

   //-- Render Parking Lots
   if (gConfig.isDefined(cConfigRenderSquadPlotter))
      renderParkingLots();

   //-- Render Squad Plotter
   if (gConfig.isDefined(cConfigRenderSquadPlotter))
      gSquadPlotter.render();

   //-- Render Squad LOS Validator
   if (gConfig.isDefined(cConfigDebugTrueLOS))
      gSquadLOSValidator.render();

   //-- Render terrain sim rep
   if (gConfig.isDefined(cConfigRenderTerrainSimRep))
      gTerrainSimRep.debugRender();

   if (gConfig.isDefined(cConfigRenderBattles) && mpBattleManager) 
      mpBattleManager->render();

   if (gConfig.isDefined(cConfigRenderGrouper) && mEntityGrouper) 
      mEntityGrouper->render();

   if (gConfig.isDefined(cConfigDisplaySounds) && mpWorldSoundManager)
      mpWorldSoundManager->render();

   // Debug render AI stuff.
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
   {
      BAI* pAI = gWorld->getAI(i);
      if (pAI)
         pAI->debugRender();
   }

   // Debug render KB stuff.
   for (uint i=0; i<cMaximumSupportedTeams; i++)
   {
      BKB* pKB = gWorld->getKB(i);
      if (pKB)
         pKB->debugRender();
   }
   
   //Generic rendering for what's selected to show common stuff.
   long simDebugValue=0;
   if (gConfig.isDefined(cConfigRenderSimDebug) &&
      gConfig.get(cConfigRenderSimDebug, &simDebugValue) &&
      (simDebugValue > 0))
   {
      //0:  Nothing.
      //1:  All.
      //2:  Platoons.
      //3:  Squads.
      //4:  Units.
      //5:  Squad Turn Radius.
      //6:  All Non-Human.
      BEntityIDArray platoons;
      //-- FIXING PREFIX BUG ID 5928
      const BSelectionManager* pSelectionManager=gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();      
      //--
      BEntityIDArray eIDs;
      if (simDebugValue != 6)
      {
         const BEntityIDArray& eConstIDs = pSelectionManager->getSelectedSquads();
         eIDs = eConstIDs;
      }
      else
      {
         BEntityHandle h = cInvalidObjectID;
         BSquad* pSquad = getNextSquad(h);
         while (pSquad)
         {
            const BPlayer* pPlayer = pSquad->getPlayer();
            if (pPlayer && !pPlayer->isHuman())
            {
               eIDs.add(pSquad->getID());
            }
            pSquad = gWorld->getNextSquad(h);
         }
      }

      for (uint i=0; i < eIDs.getSize(); i++)
      {
         BSquad* pSquad=getSquad(eIDs[i]);
         if (pSquad)
         {
            if (simDebugValue != 2)
            {
               if (simDebugValue != 3)
               {
                  for (uint j=0; j < pSquad->getNumberChildren(); j++)
                  {
                     BUnit* pUnit = getUnit(pSquad->getChild(j));
                     if (pUnit)
                        pUnit->debugRender();
                  }
               }
               if (simDebugValue != 4)
                  pSquad->debugRender();
            }
            
            platoons.uniqueAdd(pSquad->getParentID());
         }
      }
      
      if ((simDebugValue < 3) || (simDebugValue == 6))
      {
         for (uint i=0; i < platoons.getSize(); i++)
         {
            BPlatoon* pPlatoon=getPlatoon(platoons[i]);
            if (pPlatoon)
               pPlatoon->debugRender();
         }
      }
   }

   #endif
}

//==============================================================================
// BWorld::renderTerrainFlattenRegions
//==============================================================================
void BWorld::renderTerrainFlattenRegions()
{
   const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);

   const BPlayer * pPlayer = 0;
   if(pPrimaryUser)
      pPlayer= pPrimaryUser->getPlayer();

   const BTeam * pTeam = 0;
   if(pPlayer)
      pTeam = pPlayer->getTeam();

   BEntityHandle handle=cInvalidObjectID;

   if (pTeam)
   {
      const BBitArray * const  pBitArray=pTeam->getVisibleUnits();
      uint numObjects = (uint) getUnitsHighWaterMark();
      uint type = BEntity::cClassTypeUnit << 28;
      for (uint i = 0; i < numObjects; i++)
      {
         if (mFlagAllVisible || pBitArray->isBitSet(i))
         {
            // Look for buildings and show their terrain flatten regions
            const BUnit * const pUnit = getUnit(type | i, true);
            if (pUnit && pUnit->isType(gDatabase.getObjectType("Building")))
            {
               for (uint j = 0; j < 2; j++)
               {
                  BTerrainFlattenRegion flattenRegion = pUnit->getProtoObject()->getFlattenRegion(j);
                  if (flattenRegion.mMinX != 0.0f || flattenRegion.mMaxX != 0.0f) // We have a non-zeroed region
                  {
                     float xMin = (pUnit->getPosition().x + flattenRegion.mMinX);
                     float xMax = (pUnit->getPosition().x + flattenRegion.mMaxX);
                     float zMin = (pUnit->getPosition().z + flattenRegion.mMinZ);
                     float zMax = (pUnit->getPosition().z + flattenRegion.mMaxZ);
                     BVector pts[4];
                     pts[0].x = xMin; pts[0].z = zMin; pts[0].y = pUnit->getPosition().y;
                     pts[1].x = xMin; pts[1].z = zMax; pts[1].y = pUnit->getPosition().y;
                     pts[2].x = xMax; pts[2].z = zMax; pts[2].y = pUnit->getPosition().y;
                     pts[3].x = xMax; pts[3].z = zMin; pts[3].y = pUnit->getPosition().y;
                     gTerrainSimRep.addDebugSquareOverTerrain(pts[0], pts[1], pts[2], pts[3], cDWORDOrange, 0.5f);
                  }
               }
            }                     
         }
      }
   }
}

//==============================================================================
// BWorld::renderParkingLots
//==============================================================================
void BWorld::renderParkingLots()
{
   const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);

   const BPlayer * pPlayer = 0;
   if(pPrimaryUser)
    pPlayer = pPrimaryUser->getPlayer();
   
   const BTeam * pTeam = 0;
   if(pPlayer)
      pTeam=pPlayer->getTeam();

   BEntityHandle handle=cInvalidObjectID;

   if (pTeam)
   {
      const BBitArray * const pBitArray=pTeam->getVisibleUnits();
      uint numObjects = (uint) getUnitsHighWaterMark();
      uint type = BEntity::cClassTypeUnit << 28;
      for (uint i = 0; i < numObjects; i++)
      {
         if (mFlagAllVisible || pBitArray->isBitSet(i))
         {
            // Look for buildings and show their parking lots
            const BUnit * const pUnit = getUnit(type | i, true);
            if (pUnit && pUnit->isType(gDatabase.getObjectType("Building")))
            {
               if (pUnit->getProtoObject()->getParkingMinX() != 0.0f || pUnit->getProtoObject()->getParkingMaxX() != 0.0f) // We have a non-zeroed region
               {
                  float xMin = (pUnit->getPosition().x + pUnit->getProtoObject()->getParkingMinX());
                  float xMax = (pUnit->getPosition().x + pUnit->getProtoObject()->getParkingMaxX());
                  float zMin = (pUnit->getPosition().z + pUnit->getProtoObject()->getParkingMinZ());
                  float zMax = (pUnit->getPosition().z + pUnit->getProtoObject()->getParkingMaxZ());
                  BVector pts[4];
                  pts[0].x = xMin; pts[0].z = zMin; pts[0].y = pUnit->getPosition().y;
                  pts[1].x = xMin; pts[1].z = zMax; pts[1].y = pUnit->getPosition().y;
                  pts[2].x = xMax; pts[2].z = zMax; pts[2].y = pUnit->getPosition().y;
                  pts[3].x = xMax; pts[3].z = zMin; pts[3].y = pUnit->getPosition().y;
                  gTerrainSimRep.addDebugSquareOverTerrain(pts[0], pts[1], pts[2], pts[3], cDWORDPurple, 0.5f);
               }
            }                     
         }
      }
   }
}

//==============================================================================
// BWorld::createObject
//==============================================================================
BObject* BWorld::createObject( BObjectCreateParms & parms )
{
   BASSERT(mpObjectManager);

   if (parms.mProtoObjectID==-1)
      return NULL;

   //SCOPEDSAMPLE(BWorld_createObject)

   BPlayer*  pPlayer=getPlayer(parms.mPlayerID);
   if(!pPlayer)
      return NULL;

   BProtoObject *  pProtoObject = pPlayer->getProtoObject(parms.mProtoObjectID);
   if(!pProtoObject)
      return NULL;

   parms.mType = BEntity::cClassTypeObject;

   BEntity*  pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;
  
   BObject* pObject=pEnt->getObject();

   if (!pObject->initFromProtoObject(pProtoObject, parms))
   {
      mpObjectManager->releaseObject(pEnt);
      return NULL;
   }

   // Persistent actions
   BTactic* pTactic = pProtoObject->getTactic();
   if (pTactic)
   {
      for (long i = 0; i < pTactic->getNumberPersistentActions(); i++)
      {         
//-- FIXING PREFIX BUG ID 5931
         const BProtoAction* pProtoAction = pTactic->getPersistentAction(i);
//--
         if(!pProtoAction)
            continue;

         //-- See if there is a persistent action type specified.       
         BActionType actionType = pProtoAction->getPersistentActionType();
         if(actionType == BAction::cActionTypeInvalid)
            actionType = pProtoAction->getActionType();

         BAction* pAction = NULL;
         pAction = gActionManager.createAction(actionType);
         if(pAction)
         {
            pAction->setProtoAction(pProtoAction);
            //pAction->setFlagAuto(true);
            pAction->setFlagPersistent(true);
            pAction->setFlagFromTactic(true);
            pObject->addAction(pAction);
         }
      }
   }

   return pObject;
}

//==============================================================================
// BWorld::createUnit
//==============================================================================
BUnit* BWorld::createUnit( BObjectCreateParms & parms )
{
   BASSERT(mpObjectManager);
   BASSERT(parms.mProtoObjectID!=-1);

   //SCOPEDSAMPLE(BWorld_createUnit)

   BPlayer*  pPlayer=getPlayer(parms.mPlayerID);
   if(!pPlayer)
      return NULL;

   BProtoObject *  pProtoObject = pPlayer->getProtoObject(parms.mProtoObjectID);
   if(!pProtoObject)
      return NULL;

   parms.mType = BEntity::cClassTypeUnit;

   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;

   BUnit* pUnit=pEnt->getUnit();

   if (!pUnit->initFromProtoObject(pProtoObject, parms))
   {
      mpObjectManager->releaseObject(pEnt);
      return NULL;
   }

#ifdef SYNC_Unit
   syncUnitData("BWorld::createUnit ID", pUnit->getID().asLong());
   syncUnitData("BWorld::createUnit Name", pProtoObject->getName());
   syncUnitData("BWorld::createUnit Position", pUnit->getPosition());
#endif

   return pUnit;
}

//==============================================================================
// Create a squad based on the input parameters
//==============================================================================
BSquad* BWorld::createSquad(BObjectCreateParms& parms, BEntityIDArray* existingUnits)
{
   BASSERT(mpObjectManager);
   //SCOPEDSAMPLE(BWorld_createSquad)

   BPlayer* pPlayer = getPlayer(parms.mPlayerID);
   if (!pPlayer)
   {
      return (NULL);
   }

//-- FIXING PREFIX BUG ID 5934
   const BProtoSquad* pProtoSquad = NULL;
//--
   if (parms.mProtoSquadID != -1)
      pProtoSquad = pPlayer->getProtoSquad(parms.mProtoSquadID);
   else if (parms.mProtoObjectID != -1)
   {
//-- FIXING PREFIX BUG ID 5932
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(parms.mProtoObjectID);
//--
      if (!pProtoObject)
         return (NULL);
      pProtoSquad = pPlayer->getProtoSquad(pProtoObject->getProtoSquadID());
   }
   if (!pProtoSquad)
      return (NULL);

   parms.mType = BEntity::cClassTypeSquad;

   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return (NULL);

   BSquad* pSquad = pEnt->getSquad();

   pSquad->setFlagIgnorePop(parms.mIgnorePop);
   if (!pSquad->initFromProtoSquad(pProtoSquad, parms, existingUnits))
   {
      mpObjectManager->releaseObject(pSquad);
      return (NULL);
   }
#ifdef SYNC_Squad
   syncSquadData("BWorld::createSquad ID", pSquad->getID().asLong());
   syncSquadData("BWorld::createSquad Name", pProtoSquad->getName());
   syncSquadData("BWorld::createSquad Position", pSquad->getPosition());
#endif

   // Grant squad powers
   pSquad->grantPower();

   //Create an army and a platoon for this squad if it can be ordered around the map.   
   if (pSquad->getProtoObject() && pSquad->getProtoObject()->getSelectType() == cSelectTypeUnit)
   {
      BObjectCreateParms armyObjectParms;
      armyObjectParms.mPlayerID = pPlayer->getID();
      BArmy* pArmy = gWorld->createArmy(armyObjectParms);
      if (pArmy)
      {
         BPlatoon* pPlatoon = gWorld->createPlatoon(armyObjectParms);
         if (pPlatoon)
         {
            pArmy->addChild(pPlatoon);
            pPlatoon->addChild(pSquad);
         }
      }
   }
   
   return (pSquad);
}

//==============================================================================
// BWorld::createPlatoon
//==============================================================================
BPlatoon* BWorld::createPlatoon(BObjectCreateParms &parms)
{
   BASSERT(mpObjectManager);

//-- FIXING PREFIX BUG ID 5935
   const BPlayer* pPlayer=getPlayer(parms.mPlayerID);
//--
   if(!pPlayer)
      return NULL;

   parms.mType = BEntity::cClassTypePlatoon;
   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;

   BPlatoon* platoon=pEnt->getPlatoon();

   //TODO: Init anything here?

#ifdef SYNC_Squad
   syncSquadData("BWorld::createPlatoon ID", platoon->getID().asLong());
#endif
   return(platoon);
}

//==============================================================================
// BWorld::createArmy
//==============================================================================
BArmy* BWorld::createArmy(BObjectCreateParms &parms)
{
   BASSERT(mpObjectManager);

//-- FIXING PREFIX BUG ID 5936
   const BPlayer* pPlayer=getPlayer(parms.mPlayerID);
//--
   if(!pPlayer)
      return NULL;

   parms.mType = BEntity::cClassTypeArmy;
   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;

   BArmy* Army=pEnt->getArmy();

   //TODO: Init anything here?

#ifdef SYNC_Squad
   syncSquadData("BWorld::createArmy ID", Army->getID().asLong());
#endif
   return(Army);
}

//==============================================================================
// BWorld::createDopple
//==============================================================================
BDopple* BWorld::createDopple( BObject *pObject, BTeamID teamID, bool goIdle )
{
   BASSERT(mpObjectManager);
   BASSERT(pObject);

   BObjectCreateParms parms;
   parms.mPlayerID = pObject->getPlayerID();
   parms.mProtoObjectID = pObject->getProtoID();
   parms.mStartBuilt = true;
   parms.mType = BEntity::cClassTypeDopple;
   parms.mPosition = pObject->getPosition();
   parms.mForward = pObject->getForward();
   parms.mRight = pObject->getRight();

   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;

   BDopple *pDopple = pEnt->getDopple();

   if(!pDopple->initFromObject(pObject, teamID, goIdle))
   {
      mpObjectManager->releaseObject(pEnt);
      return NULL;
   }

   // Tell parent it now has a dopple object for this team
   pObject->setDoppleObject(teamID);

#ifdef SYNC_Dopple
   syncDoppleData("BWorld::createDopple ID", pDopple->getID().asLong());
   syncDoppleData("BWorld::createDopple Name", pObject->getProtoObject()->getName());
   syncDoppleData("BWorld::createDopple Position", pDopple->getPosition());
#endif

   return pDopple;
}

//==============================================================================
// BWorld::createProjectile
//==============================================================================
BProjectile* BWorld::createProjectile( BObjectCreateParms & parms )
{
   BASSERT(mpObjectManager);
   BASSERT(parms.mProtoObjectID!=-1);

   BPlayer*  pPlayer=getPlayer(parms.mPlayerID);
   if(!pPlayer)
      return NULL;

   BProtoObject *  pProtoObject = pPlayer->getProtoObject(parms.mProtoObjectID);
   if(!pProtoObject)
      return NULL;

   parms.mType = BEntity::cClassTypeProjectile;

   BEntity* pEnt = mpObjectManager->createObject(parms);
   if (!pEnt)
      return NULL;

   BProjectile* pProjectile=pEnt->getProjectile();

   if (!pProjectile->initFromProtoObject(pProtoObject, parms))
   {
      mpObjectManager->releaseObject(pEnt);
      return NULL;
   }

   //-- force the visual display priority to combat for all projectiles.
   BVisual* pVisual = pProjectile->getVisual();
   if (pVisual)
   {
      pVisual->setDisplayPriority(cVisualDisplayPriorityCombat);
   }

#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigDebugProjectiles))
      gpDebugPrimitives->addDebugSphere(parms.mPosition, 0.1f, cDWORDPurple, BDebugPrimitives::cCategoryNone, 1.0f);
#endif

#ifdef SYNC_Projectile
   syncProjectileData("BWorld::createProjectile ID", pProjectile->getID().asLong());
   syncProjectileData("BWorld::createProjectile Name", pProtoObject->getName());
   syncProjectileData("BWorld::createProjectile Position", pProjectile->getPosition());
#endif

   return pProjectile;
}

//==============================================================================
// BWorld::createTempObject
//==============================================================================
BObject* BWorld::createTempObject(BObjectCreateParms& parms, long protoVisIndex, float lifespan, long userData, bool visibleToAll, int displayPriority)
{
   BObject *pObject;

   parms.mProtoObjectID = mTempObjectID;
   parms.mStartBuilt = true;
   parms.mType = BEntity::cClassTypeObject;
   pObject = createObject(parms);
   if (pObject)
   {
      pObject->setVisual(protoVisIndex, pObject->getProtoObject()->getVisualDisplayPriority(), (int64) userData);
      pObject->setLifespan((DWORD) lifespan*1000.0f);
      pObject->setFlagFadeOnDeath(true);
      pObject->setFlagVisibleToAll(visibleToAll);
      BVisual* pVisual = pObject->getVisual();
      if (pVisual)
         pVisual->setDisplayPriority(displayPriority);
   }
   return  pObject;
}

//==============================================================================
//==============================================================================
void BWorld::createTerrainEffectInstance(const BProtoAction* pPA, BVector pos, BVector fwd, BPlayerID playerID, bool bVisibleToAll)
{
   if (!pPA)
      return;

   const BProtoImpactEffect* pData = gDatabase.getProtoImpactEffectFromIndex(pPA->getImpactEffectProtoID());
   if (pData)
   {
      BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(pData->mTerrainEffectIndex, true);
      if (pTerrainEffect)
         pTerrainEffect->instantiateEffect(cInvalidSurfaceType, pPA->getImpactEffectSize(), pos, fwd, true, playerID, pData->mLifespan, bVisibleToAll, cVisualDisplayPriorityNormal, NULL);
   }
}

//==============================================================================
// BWorld::createImpactTerrainDecal
//==============================================================================
void BWorld::createImpactTerrainDecal(BVector pos,const BTerrainImpactDecalHandle *impactHandle,BVector forward/*=cZAxisVector*/)
{
   if(!impactHandle)
      return;

   gImpactDecalManager.createImpactDecal(pos,impactHandle, forward);
   
}
//==============================================================================
// BWorld::createRevealer
// Creates the revealer and gives it a lifespan as well.
//==============================================================================
BObject* BWorld::createRevealer(BTeamID teamID, BVector position, float los, DWORD lifespan, float finalLOS)
{
   BObject *pRevealerObject = createRevealer(teamID, position, los);
   if (pRevealerObject)
   {
      pRevealerObject->setLifespan(lifespan);

      // [7/7/2008 xemu] disabling this for now pending some further tuning & exploration 
      /*
      BUnitActionScaleLOS *pActionScaleLOS = (BUnitActionScaleLOS*)gActionManager.createAction(BAction::cActionTypeUnitScaleLOS);
      if (los != -1.0f)
      {
         los = Math::Max(los, gDatabase.getMinimumRevealerSize());
      }
      pActionScaleLOS->setStartValue(los);
      if (finalLOS != -1.0f)
      {
         finalLOS = Math::Max(finalLOS, gDatabase.getMinimumRevealerSize());
      }
      pActionScaleLOS->setFinishValue(finalLOS);
      pActionScaleLOS->setDuration(lifespan);

      pRevealerObject->addAction(pActionScaleLOS);
      */

   }
   return(pRevealerObject);
}

//==============================================================================
// BWorld::createRevealer
// Creates the revealer and gives it a lifespan as well.
//==============================================================================
BObject* BWorld::createRevealer(BTeamID teamID, BVector position, float los, DWORD lifespan)
{
   BObject *pRevealerObject = createRevealer(teamID, position, los);
   if (pRevealerObject)
   {
      pRevealerObject->setLifespan(lifespan);
   }
   return(pRevealerObject);
}

//==============================================================================
// BWorld::createRevealer
// Version that just creates the revealer (without setting a lifespan)
//==============================================================================
BObject* BWorld::createRevealer(BTeamID teamID, BVector position, float los)
{
   BTeam* pTeam = gWorld->getTeam(teamID);
   // [7/15/2008 xemu] if a team has no players though, don't bother
   if (!pTeam || (pTeam->getNumberPlayers() <= 0))
      return NULL;

   BObjectCreateParms parms;
   parms.mProtoObjectID = gDatabase.getPOIDRevealer();
   parms.mType = BEntity::cClassTypeObject;
   parms.mPosition = position;
   // [7/7/2008 xemu] ok, we actually only need to look at the first player on this team (player index = 0) because all players on a team have shared LOS 
   parms.mPlayerID = pTeam->getPlayerID(0);
   parms.mStartBuilt = true;
   
   BUnit* pRevealerUnit = createUnit(parms);
   BObject* pRevealerObject = pRevealerUnit->getObject();
   if (pRevealerObject)
   {
      pRevealerObject->setFlagLOS(true);
      pRevealerObject->setFlagIsRevealer(true);
      if (los != -1.0f)
      {
         los = Math::Max(los, gDatabase.getMinimumRevealerSize());
      }
      pRevealerObject->setLOSScalar(los);
      pRevealerObject->revealOverTime();
   }
   pRevealerUnit->setFlagDiesAtZeroHP(false);

   return(pRevealerObject);
}

//==============================================================================
// Version creates the blocker with a lifespan
//==============================================================================
BObject* BWorld::createBlocker(BTeamID teamID, BVector position, float los, DWORD lifespan)
{
   BObject* pBlockerObject = createBlocker(teamID, position, los);
   if (pBlockerObject)
   {
      pBlockerObject->setLifespan(lifespan);
   }

   return (pBlockerObject);
}

//==============================================================================
// Version that just creates the blocker (without setting a lifespan)
//==============================================================================
BObject* BWorld::createBlocker(BTeamID teamID, BVector position, float los)
{
   BTeam* pTeam = gWorld->getTeam(teamID);
   // [7/15/2008 xemu] if a team has no players though, don't bother
   if (!pTeam || (pTeam->getNumberPlayers() <= 0))
      return NULL;

   BObjectCreateParms parms;
   parms.mProtoObjectID = gDatabase.getPOIDBlocker();
   parms.mType = BEntity::cClassTypeObject;
   parms.mPosition = position;
   // [7/7/2008 xemu] ok, we actually only need to look at the first player on this team (player index = 0) because all players on a team have shared LOS 
   parms.mPlayerID = pTeam->getPlayerID(0);
   parms.mStartBuilt = true;

   BUnit* pBlockerUnit = createUnit(parms);
   BObject* pBlockerObject = pBlockerUnit->getObject();
   if (pBlockerObject)
   {
      pBlockerObject->setFlagLOS(true);
      pBlockerObject->setFlagBlockLOS(true);
      pBlockerObject->setLOSScalar(los);
      // Block LOS
      for (int i = 1; i < gWorld->getNumberTeams(); i++)
      {
         if (i != teamID)
         {
            gVisibleMap.blockCircularRegion(pBlockerObject->getSimX(), pBlockerObject->getSimZ(), pBlockerObject->getSimLOS(), i);
         }
      }
   }

   return (pBlockerObject);
}

//==============================================================================
// BWorld::createVisItemPhysicsObject
//==============================================================================
BUnit* BWorld::createVisItemPhysicsObject(BObjectCreateParms& params, BVisualItem* pVisItem, const BPhysicsInfo *pPhysicsInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool bDisregardAttachments )
{
   // Create unit
   BUnit* pUnit = gWorld->createUnit(params);
   if (!pUnit)
      return NULL;

   // Set flags
   pUnit->setAnimationEnabled(false, true);

   // Create visual from visual item
   BVisual* pVisual = BVisual::getInstance();
   if(!pVisual)
   {
      gWorld->releaseObject(pUnit);
      return NULL;
   }
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);

   if (!pVisual->clone(pVisItem, false, 0, bDisregardAttachments, playerColor, worldMatrix))
   {
      BVisual::releaseInstance(pVisual);
      gWorld->releaseObject(pUnit);
      return NULL;
   }

   // Set visual to the one we just created
   pUnit->setVisualPtr(pVisual);
   pUnit->setAnimationEnabled(false, true);

   // Create default physics object if none already created
   if (!pUnit->getPhysicsObject() && pPhysicsInfo)
   {
      pUnit->createPhysicsObject(pPhysicsInfo, pBPOverrides, true, params.mPhysicsReplacement);
   }

   return pUnit;
}

//==============================================================================
// BWorld::createVisItemPhysicsObjectDirect
//==============================================================================
BUnit* BWorld::createVisItemPhysicsObjectDirect(BObjectCreateParms& params, BVisualItem* pVisItem, const BPhysicsObjectParams &physicsparams, bool bDisregardAttachments )
{
   // Create unit
   BUnit* pUnit = gWorld->createUnit(params);
   if (!pUnit)
      return NULL;

   // Set flags
   pUnit->setAnimationEnabled(false, true);

   // Create visual from visual item
   BVisual* pVisual = BVisual::getInstance();
   if(!pVisual)
   {
      gWorld->releaseObject(pUnit);
      return NULL;
   }
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);

   if (!pVisual->clone(pVisItem, false, 0, bDisregardAttachments, playerColor, worldMatrix))
   {
      BVisual::releaseInstance(pVisual);
      gWorld->releaseObject(pUnit);
      return NULL;
   }

   // Set visual to the one we just created
   pUnit->setVisualPtr(pVisual);
   pUnit->setAnimationEnabled(false, true);

   // Create default physics object if none already created
   if (!pUnit->getPhysicsObject())
   {
      pUnit->createPhysicsObjectDirect(physicsparams, true);
   }

   return pUnit;
}


//==============================================================================
// BWorld::createVisPhysicsObjectDirect
//==============================================================================
BUnit* BWorld::createVisPhysicsObjectDirect(BObjectCreateParms& params, BVisual* pSourceVisual, const BPhysicsObjectParams &physicsparams, bool bDisregardAttachments )
{
   // Create unit
   BUnit* pUnit = gWorld->createUnit(params);
   if (!pUnit)
      return NULL;

   // Set flags
   pUnit->setAnimationEnabled(false, true);

   // Create visual from visual item
   BVisual* pVisual = BVisual::getInstance();
   if(!pVisual)
   {
      gWorld->releaseObject(pUnit);
      return NULL;
   }
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);

   if (!pVisual->clone(pSourceVisual, false, pSourceVisual->getUserData(), bDisregardAttachments, playerColor, worldMatrix))
   {
      BVisual::releaseInstance(pVisual);
      gWorld->releaseObject(pUnit);
      return NULL;
   }

   // Set visual to the one we just created
   pUnit->setVisualPtr(pVisual);
   pUnit->setAnimationEnabled(false, true);

   // Create default physics object if none already created
   if (!pUnit->getPhysicsObject())
   {
      pUnit->createPhysicsObjectDirect(physicsparams, true);
   }

   return pUnit;
}

//==============================================================================
// BWorld::validateEntityID
//==============================================================================
bool BWorld::validateEntityID(BEntityID id) const
{
   BASSERT(mpObjectManager);
   return mpObjectManager->validateEntityID(id);
}

//==============================================================================
// BWorld::validateEntityID
//==============================================================================
bool BWorld::validateEntityID(long id) const
{
   BASSERT(mpObjectManager);
   return mpObjectManager->validateEntityID(BEntityID(id));
}

//==============================================================================
// BWorld::getEntity
//==============================================================================
BEntity* BWorld::getEntity( BEntityID id, bool anyCount )
{
   if (!mpObjectManager)
      return NULL;
   return mpObjectManager->getEntity(id, anyCount);
}

//==============================================================================
// BWorld::getEntity
//==============================================================================
BEntity* BWorld::getEntity( long id, bool anyCount )
{
   if (!mpObjectManager)
      return NULL;
   return mpObjectManager->getEntity(id, anyCount);
}

//==============================================================================
// BWorld::getObject
//==============================================================================
BObject* BWorld::getObject( BEntityID id, bool anyCount )
{
   if (!mpObjectManager)
      return NULL;
   return mpObjectManager->getObject(id, anyCount);
}

//==============================================================================
// BWorld::getObject
//==============================================================================
BObject* BWorld::getObject( long id, bool anyCount)
{
   return mpObjectManager->getObject(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getUnit
//==============================================================================
BUnit* BWorld::getUnit( BEntityID id, bool anyCount )
{
   if (!mpObjectManager)
      return NULL;
    return mpObjectManager->getUnit(id, anyCount);
}

//==============================================================================
// BWorld::getUnit
//==============================================================================
BUnit* BWorld::getUnit( long id, bool anyCount )
{
   return getUnit(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getSquad
//==============================================================================
BSquad* BWorld::getSquad( BEntityID id, bool anyCount)
{
   if (!mpObjectManager)
      return NULL;
    return mpObjectManager->getSquad(id, anyCount);
}

//==============================================================================
// BWorld::getSquad
//==============================================================================
BSquad* BWorld::getSquad( long id, bool anyCount )
{
   return getSquad(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getPlatoon
//==============================================================================
BPlatoon* BWorld::getPlatoon( BEntityID id, bool anyCount)
{
   if (!mpObjectManager)
      return NULL;
    return mpObjectManager->getPlatoon(id, anyCount);
}

//==============================================================================
// BWorld::getPlatoon
//==============================================================================
BPlatoon* BWorld::getPlatoon( long id, bool anyCount )
{
   return getPlatoon(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getArmy
//==============================================================================
BArmy* BWorld::getArmy( BEntityID id, bool anyCount)
{
   if (!mpObjectManager)
      return NULL;
    return mpObjectManager->getArmy(id, anyCount);
}

//==============================================================================
// BWorld::getArmy
//==============================================================================
BArmy* BWorld::getArmy( long id, bool anyCount )
{
   return getArmy(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getDopple
//==============================================================================
BDopple* BWorld::getDopple( BEntityID id, bool anyCount)
{
   if (!mpObjectManager)
      return NULL;
   return mpObjectManager->getDopple(id, anyCount);
}

//==============================================================================
// BWorld::getDopple
//==============================================================================
BDopple* BWorld::getDopple( long id, bool anyCount )
{
   return getDopple(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::getProjectile
//==============================================================================
BProjectile* BWorld::getProjectile( BEntityID id, bool anyCount)
{
   if (!mpObjectManager)
      return NULL;
   return mpObjectManager->getProjectile(id, anyCount);
}

//==============================================================================
// BWorld::getProjectile
//==============================================================================
BProjectile* BWorld::getProjectile( long id, bool anyCount )
{
   return getProjectile(BEntityID(id), anyCount);
}

//==============================================================================
// BWorld::releaseObject
//==============================================================================
void BWorld::releaseObject(BEntityID id)
{
   if (mpObjectManager)
      mpObjectManager->releaseObject(id);
}

//==============================================================================
// BWorld::releaseObject
//==============================================================================
void BWorld::releaseObject(BEntity* pEntity)
{  
   if (mpObjectManager)
      mpObjectManager->releaseObject(pEntity);
}

//==============================================================================
// BWorld::createEntity
//==============================================================================
BEntityID BWorld::createEntity(
   long protoID, bool isSquadID, long playerID, 
   BVector position, BVector forward, BVector right, 
   bool startBuilt, bool bPhysicsReplacement, 
   bool noTieToGround, BEntityID sourceVisual, 
   BPlayerID createdByPlayerID, BEntityID builtByUnitID, BEntityID socketUnitID, 
   bool noCost, float aoTintValue, bool appearsBelowDecals, int level, 
   int visualVariationIndex, int16 exploreGroup, bool startExistSound)
{

   BObjectCreateParms objectParms;
   objectParms.mPlayerID=playerID;
   objectParms.mStartBuilt=startBuilt;
   objectParms.mNoTieToGround=noTieToGround;
   objectParms.mPhysicsReplacement=bPhysicsReplacement;
   objectParms.mPosition=position;
   objectParms.mForward=forward;
   objectParms.mRight=right;
   objectParms.mSourceVisual=sourceVisual;
   objectParms.mCreatedByPlayerID=createdByPlayerID;
   objectParms.mBuiltByUnitID=builtByUnitID;
   objectParms.mSocketUnitID=socketUnitID;
   objectParms.mNoCost=noCost;
   objectParms.mAOTintValue=aoTintValue;
   objectParms.mAppearsBelowDecals=appearsBelowDecals;
   objectParms.mLevel=level;
   objectParms.mVisualVariationIndex = visualVariationIndex;
   objectParms.mExploreGroup = exploreGroup;
   objectParms.mStartExistSound = startExistSound;

   if (socketUnitID!=cInvalidObjectID)
   {
      const BUnit* pSocketUnit=getUnit(socketUnitID);
      if (pSocketUnit)
      {
         objectParms.mPosition=pSocketUnit->getPosition();
         objectParms.mForward=pSocketUnit->getForward();
         objectParms.mRight=pSocketUnit->getRight();

         if (pSocketUnit->getProtoObject()->getFlagUseBuildRotation())
         {
//-- FIXING PREFIX BUG ID 5945
            const BPlayer* pPlayer=getPlayer(playerID);
//--
            if (pPlayer)
            {
//-- FIXING PREFIX BUG ID 5933
               const BProtoObject* pProtoObject=pPlayer->getProtoObject(protoID);
//--
               if (pProtoObject)
               {
                  float buildRotation = pProtoObject->getBuildRotation();
                  if (buildRotation != 0.0f)
                  {
                     BMatrix rotMat;
                     rotMat.makeRotateY(Math::fDegToRad(buildRotation));
                     BVector forward;
                     rotMat.transformVector(objectParms.mForward, forward);
                     objectParms.mForward = forward;
                     objectParms.mRight.assignCrossProduct(cYAxisVector, forward);
                     objectParms.mRight.normalize();
                  }

                  BVector buildOffset = pProtoObject->getBuildOffset();
                  if (buildOffset != cOriginVector)
                  {
                     BMatrix posMat;
                     posMat.makeOrient(pSocketUnit->getForward(), cYAxisVector, pSocketUnit->getRight());
                     posMat.setTranslation(pSocketUnit->getPosition());
                     posMat.transformVectorAsPoint(buildOffset, objectParms.mPosition);
                     BVector origin(objectParms.mPosition.x, objectParms.mPosition.y+1000.0f, objectParms.mPosition.z);
                     gTerrainSimRep.getHeightRaycast(origin, objectParms.mPosition.y, true);
                  }
               }
            }
         }
      }
   }

   if(isSquadID)
   {
      objectParms.mProtoSquadID=protoID;
//-- FIXING PREFIX BUG ID 5947
      const BSquad* pSquad=gWorld->createSquad(objectParms);
//--
      if(!pSquad)
         return cInvalidObjectID;
      return pSquad->getID();
   }

   objectParms.mProtoObjectID=protoID;

   BPlayer* pPlayer=getPlayer(playerID);
   if(!pPlayer)
      return cInvalidObjectID;

//-- FIXING PREFIX BUG ID 5937
   const BProtoObject* pProtoObject=pPlayer->getProtoObject(protoID);
//--
   if(!pProtoObject)
      return cInvalidObjectID;

   long objectClass=pProtoObject->getObjectClass();

   switch(objectClass)
   {
      case cObjectClassBuilding:
      case cObjectClassSquad:
      case cObjectClassUnit:
      {
         if (bPhysicsReplacement)
         {
//-- FIXING PREFIX BUG ID 5948
            const BUnit* pUnit=gWorld->createUnit(objectParms);
//--
            if(!pUnit)
               return cInvalidObjectID;
            return pUnit->getID();
         }
         else
         {
//-- FIXING PREFIX BUG ID 5949
            const BSquad* pSquad=gWorld->createSquad(objectParms);
//--
            if(!pSquad)
               return cInvalidObjectID;
            return pSquad->getID();

         }
      }
      
      default:
      {
//-- FIXING PREFIX BUG ID 5950
         const BObject* pObject=gWorld->createObject(objectParms);
//--
         if(!pObject)
            return cInvalidObjectID;
         return pObject->getID();
      }
   }
}

//==============================================================================
// BWorld::getPlayer
//==============================================================================
BPlayer* BWorld::getPlayer(long id)
{ 
   if(id<0 || id>=mPlayers.getNumber())
      return NULL;
   else
      return mPlayers[id];
}

//==============================================================================
// BWorld::getPlayer
//==============================================================================
const BPlayer* BWorld::getPlayer(long id) const
{ 
   if(id<0 || id>=mPlayers.getNumber())
      return NULL;
   else
      return mPlayers[id];
}

//==============================================================================
// 
//==============================================================================
BPlayer* BWorld::getPlayerByMPID(long mpid) const
{
   for (int i=mPlayers.getNumber()-1; i >= 0; --i)
   {
      if (mPlayers[i] && mPlayers[i]->getMPID() == mpid)
         return mPlayers[i];
   }
   return NULL;
}

//==============================================================================
// BWorld::createPlayer
//==============================================================================
BPlayer* BWorld::createPlayer(long mpid, BSimString& name, BUString& localisedName, long civID, long leaderID, float difficulty, BSmallDynamicSimArray<long>* pForbidObjects, BSmallDynamicSimArray<long>* pForbidSquads, BSmallDynamicSimArray<long>* pForbidTechs, BSmallDynamicSimArray<BLeaderPop>* pLeaderPops, uint8 playerType, int scenarioID, bool fromSave)
{
   long playerID = mPlayers.getNumber();
   BPlayer* pPlayer = new BPlayer();
   if(!pPlayer)
      return NULL;
   
   if(!pPlayer->setup(playerID, mpid, name, localisedName, civID, leaderID, difficulty, pForbidObjects, pForbidSquads, pForbidTechs, pLeaderPops, playerType, scenarioID, fromSave))
   {
      delete pPlayer;
      return NULL;
   }

   if(mPlayers.add(pPlayer)==-1)
   {
      delete pPlayer;
      return NULL;
   }

   return pPlayer;
}

//==============================================================================
// BWorld::getTeam
//==============================================================================
BTeam* BWorld::getTeam(BTeamID id)
{ 
   if(id<0 || id>=mTeams.getNumber())
      return NULL;
   else
      return mTeams[id];
}

//==============================================================================
// BWorld::getTeam
//==============================================================================
const BTeam* BWorld::getTeam(BTeamID id) const
{ 
   if(id<0 || id>=mTeams.getNumber())
      return NULL;
   else
      return mTeams[id];
}

//==============================================================================
// BWorld::createTeam
//==============================================================================
BTeam* BWorld::createTeam()
{
   long teamID = mTeams.getNumber();
   BTeam* pTeam = new BTeam();
   if(!pTeam)
      return NULL;

   if(!pTeam->setup(teamID))
   {
      delete pTeam;
      return NULL;
   }

   if(mTeams.add(pTeam)==-1)
   {
      delete pTeam;
      return NULL;
   }

   return pTeam;
}

//==============================================================================
// BWorld::addLight
//==============================================================================
bool BWorld::addLight(BLightVisualInstance* pInstance)
{
   mLights.pushBack(pInstance);
   return true;
}

//==============================================================================
// BWorld::releaseLights
//==============================================================================
void BWorld::releaseLights()
{
   uint count=mLights.getSize();
   for(uint i=0; i<count; i++)
   {
      BLightVisualInstance* pInstance=mLights[i];
      gLightVisualManager.releaseInstance(pInstance);
   }
   mLights.resize(0);

   gLightVisualManager.deinit();
}

//==============================================================================
// BWorld::getPathingLimiter
//==============================================================================
BPathingLimiter* BWorld::getPathingLimiter()
{
   if (gConfig.isDefined(cConfigDisablePathingLimits))
      return NULL;
   else
      return mpPathingLimiter;
}

//==============================================================================
// BWorld::doCollisionCheck
//==============================================================================
bool BWorld::doCollisionCheck( BObject & sourceObject, BVector &velocity, BObject **pFirstHit /*=NULL*/) const
{
   
  /* bool hit = false;
   float distance = -1.0f;

   BVector dir = velocity;
   dir.normalize();
   BVector adjustedVelocity;

   //-- see if our current movement collides with anybody
   for (uint i=0; i < mObjects.size(); i++)
   {
      BObject *pObject = mObjects[i];
      if (!pObject)
         continue;

      if (pObject->getID() == sourceObject.getID())
         continue;

      if (!pObject->isCollidable())
         continue;

      float r2 = sourceObject.getSimRadius() + pObject->getSimRadius();
      r2 = r2 * r2;

      float sx1, sx2, sz1, sz2;
      BVector pos = sourceObject.getPosition();
      sx1 = pos.x; sz1 = pos.z;
      pos += velocity;
      sx2 = pos.x; sz2 = pos.z;
      float d2 = distanceToSegmentSqr(pObject->getPosition().x, pObject->getPosition().z, sx1, sz1, sx2, sz2);

      if (d2 < r2)
      {

         if (pFirstHit)
         {
            if (distance < 0.0f || d2 < distance)
            {
               distance = d2;
               adjustedVelocity = dir * d2;
               *pFirstHit = pObject;
               hit = true;
            }
         }
      }
   }

   if (hit)
   {
      velocity = adjustedVelocity;
      return (true);
   }*/

   return (false);
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInArea(BUnitQuery* pUnitQuery, BEntityIDArray* pResults, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   BASSERT(pUnitQuery);
   BASSERT(pResults);

   if (!pUnitQuery || !pResults)
      return(0);

   BObjectManager *pObjMgr = gWorld->getObjectManager();
   if (!pObjMgr)
      return(0);

   long result = 0;
   if (pUnitQuery->isType(BUnitQuery::cQueryTypeNone))
      result = getEntitiesInAreaTypeNone(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);
   else if (pUnitQuery->isType(BUnitQuery::cQueryTypeTwoVector))
      result = getEntitiesInAreaTypeTwoVector(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);
   else if (pUnitQuery->isType(BUnitQuery::cQueryTypePointRadius))
      result = getEntitiesInAreaTypePointRadius(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);
   else if (pUnitQuery->isType(BUnitQuery::cQueryTypeConvexHull))
      result = getEntitiesInAreaTypeConvexHull(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);
   else if (pUnitQuery->isType(BUnitQuery::cQueryTypeOPQuadHull))
      result = getEntitiesInAreaTypeOPQuadHull(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);
   else if (pUnitQuery->isType(BUnitQuery::cQueryTypeOBBNoRoll))
      result = getEntitiesInAreaTypeOBBNoRoll(*pUnitQuery, *pResults, includeNonCollidableUnitObs, isSquadCheck);      
   else
   {
      BASSERTM(false, "Unknown Unit Query type used.");
      return(0);
   }
   
   // jce [12/8/2008] -- Clear the expanded hulls after we're done.  In theory this should be handled by reworking how
   // the obstruction manager is used so that it would be a more robust fix, but we're attempting to keep the change as targetted as
   // possible.  If this is not called and there are no functions using the proper begin/end bracketing functions in the
   // obstruction manager, the cache of expanded hulls just grows without bound and we run out of memory.
   gObsManager.releaseExpandedHulls();
   
   return(result);
}

//==============================================================================
//==============================================================================
bool BWorld::checkAndUpdateEntity(const BUnitQuery& unitQuery, BEntity* pEntityToTest, BEntity*& pEntityToAdd, bool isSquadCheck)
{
   // I don't think we need this, since all test entities come straight from the obstruction manager, but for safety...
   BASSERT(pEntityToTest);
   if (!pEntityToTest)
      return false;

   pEntityToAdd = pEntityToTest;

   // VAT: 11/03/08: if this is a squad check, there's a handful of special logic we need to go through
   if (isSquadCheck)
   {
      // to match the old behavior as close as possible, check the leader unit of the squad against the unit query
      BSquad* pSquad = pEntityToTest->getSquad();
      if (pSquad)
      {
         BUnit* pLeaderUnit = pSquad->getLeaderUnit();
         if (pLeaderUnit)
            return unitQuery.passesQueryFilter(*pLeaderUnit);
         return false;
      }

      // even crappier than the previous check, we need to check unit results that 
      // come back here to get their parent squads for the buildings that don't 
      // have squad obstructions and for blocks air units
      BUnit* pUnit = pEntityToTest->getUnit();
      if (pUnit)
      {
         BSquad* pSquad = pUnit->getParentSquad();
         if (pSquad)
         {
            pEntityToAdd = pSquad;
            return unitQuery.passesQueryFilter(*pUnit);
         }
         return false;
      }
   }
   else
   {
      BUnit* pUnit = pEntityToTest->getUnit();
      if (pUnit)
         return unitQuery.passesQueryFilter(*pUnit);
      return false;
   }

   return false;
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypeNone(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   BObjectManager *pObjMgr = gWorld->getObjectManager();

   BEntityHandle handle = cInvalidObjectID;
   BEntity* pAddEntity = NULL;
   if (isSquadCheck)
   {
      BSquad *pSquad;
      // Get the next object
      while ((pSquad = pObjMgr->getNextSquad(handle)) != NULL)
      {
         // Lets see if it passes the query filter.
         if (!checkAndUpdateEntity(unitQuery, pSquad, pAddEntity, isSquadCheck))
            continue;

         // Add it to the result buffer
         results.add(pAddEntity->getID());
      }
   }
   else
   {
      BUnit *pUnit;
      // Get the next object
      while ((pUnit = pObjMgr->getNextUnit(handle)) != NULL)
      {
         // Lets see if it passes the query filter.
         if (!checkAndUpdateEntity(unitQuery, pUnit, pAddEntity, isSquadCheck))
            continue;

         // Add it to the result buffer
         results.add(pAddEntity->getID());
      }
   }

   BASSERT(results.getSize() < 512);
   return(results.getSize());
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypeTwoVector(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   gObsManager.findEntityObstructions(unitQuery.getPoint0(), unitQuery.getPoint1(), includeNonCollidableUnitObs, isSquadCheck);
   BObstructionNodePtrArray &obsResults = gObsManager.getFoundObstructionResults();
   long numResultsToCheck = obsResults.getNumber();

   BEntity* pEntity = NULL;
   for (long i=0; i<numResultsToCheck; i++)
   {
      // Get the next result and see if it passes the query filter.
      if (!checkAndUpdateEntity(unitQuery, obsResults[i]->mObject, pEntity, isSquadCheck))
         continue;

      // Add it to the result buffer
      results.add(pEntity->getID());
   }

   BASSERT(results.getSize() < 512);
   return(results.getSize());
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypePointRadius(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   BVector center = unitQuery.getCenter();
   float radius = unitQuery.getRadius();
   float p0x = center.x - radius;
   float p0z = center.z - radius;
   float p1x = center.x + radius;
   float p1z = center.z + radius;
   //Square the radius to do faster check below.
   radius*=radius;   

   gObsManager.findEntityObstructions(p0x, p0z, p1x, p1z, includeNonCollidableUnitObs, isSquadCheck);
   BObstructionNodePtrArray &obsResults = gObsManager.getFoundObstructionResults();

   long numResultsToCheck = obsResults.getNumber();
   bool circular = unitQuery.getCircular();

   BEntity* pEntity = NULL;
   for (long i=0; i<numResultsToCheck; i++)
   {
      // Get the next result and see if it passes the query filter.
      if (!checkAndUpdateEntity(unitQuery, obsResults[i]->mObject, pEntity, isSquadCheck))
         continue;

      if (!circular)
      {
         // Add it to the result buffer
         results.add(pEntity->getID());
      }
      else
      {
         float distance = pEntity->calculateXZDistanceSqr(center);
         if (distance <= radius)
         {
            // Add it to the result buffer
            results.add(pEntity->getID());
         }
      }
   }

   BASSERT(results.getSize() < 512);
   return(results.getSize());
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypeConvexHull(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   gObsManager.findEntityObstructions(*unitQuery.getConvexHull(), includeNonCollidableUnitObs, isSquadCheck);
   BObstructionNodePtrArray &obsResults = gObsManager.getFoundObstructionResults();
   long numResultsToCheck = obsResults.getNumber();

   BEntity* pEntity = NULL;
   for (long i=0; i<numResultsToCheck; i++)
   {
      // Get the next result and see if it passes the query filter.
      if (!checkAndUpdateEntity(unitQuery, obsResults[i]->mObject, pEntity, isSquadCheck))
         continue;

      // Add it to the result buffer
      results.add(pEntity->getID());
   }

   BASSERT(results.getSize() < 512);
   return(results.getNumber());
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypeOPQuadHull(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   gObsManager.findEntityObstructionsQuadHull(unitQuery.getOPQuadHull(), includeNonCollidableUnitObs, isSquadCheck);
   BObstructionNodePtrArray &obsResults = gObsManager.getFoundObstructionResults();
   long numResultsToCheck = obsResults.getNumber();

   BEntity* pEntity = NULL;
   for (long i=0; i<numResultsToCheck; i++)
   {
      // Get the next result and see if it passes the query filter.
      if (!checkAndUpdateEntity(unitQuery, obsResults[i]->mObject, pEntity, isSquadCheck))
         continue;

      // Add it to the result buffer
      results.add(pEntity->getID());
   }

   BASSERT(results.getSize() < 512);
   return(results.getNumber());
}

//==============================================================================
//==============================================================================
long BWorld::getEntitiesInAreaTypeOBBNoRoll(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck)
{
   BVector center = unitQuery.getCenter();
   BVector forward = unitQuery.getPoint0();
   BVector right = unitQuery.getPoint1();
   BVector ext = unitQuery.getPoint2();
   BVector up = cYAxisVector;   
   forward.normalize();
   right.normalize();  

   // Calculate radius
   BVector maxXZ = BVector(center.x + ext.x, center.y, center.z + ext.z);   
   float radius = center.distance(maxXZ);

   //XXXHalwes - 7/24/2208 - Debug
   //gpDebugPrimitives->addDebugSphere(center, radius, cDWORDGreen);   

   // Get units that are in surround sphere
   gObsManager.findEntityObstructions(center, radius, includeNonCollidableUnitObs, isSquadCheck);
   BObstructionNodePtrArray &obsResults = gObsManager.getFoundObstructionResults();

   // Filter units in sphere
   long numResultsToCheck = obsResults.getNumber();

   BEntity* pEntity = NULL;
   for (long i=0; i<numResultsToCheck; i++)
   {
      // Get the next result and see if it passes the query filter.
      if (!checkAndUpdateEntity(unitQuery, obsResults[i]->mObject, pEntity, isSquadCheck))
         continue;

      // Add it to the result buffer
      results.add(pEntity->getID());
   }

   // Do OBB check if we still have valid units
   int numResults = results.getNumber();
   if (numResults > 0)
   {
      ATGCollision::OrientedBox obb;
      float rightAngleRads = Math::fDegToRad(90.0f);

      // Calculate pitch
      bool invert = (forward.y < 0.0f);
      BVector forwardTemp = invert ? -forward : forward;
      forwardTemp.normalize();
      float pitch = Math::fFastACos(forwardTemp.dot(cYAxisVector));            
      pitch = rightAngleRads - pitch;
      pitch = (forwardTemp.z > 0.0f) ? -pitch : pitch;      

      // Calculate yaw
      invert = (forward.z < 0.0f);
      forwardTemp = invert ? -forward : forward;
      forwardTemp.y = 0.0f;
      forwardTemp.normalize();
      float yaw = Math::fFastACos(forwardTemp.dot(cZAxisVector));
      yaw = (forwardTemp.x > 0.0f) ? yaw : -yaw;

      // Calculate matrix
      BMatrix mat;      
      mat.makeRotateYawPitchRoll(yaw, pitch, 0.0f);
      mat.multTranslate(center.x, center.y, center.z);

      // Setup OBB      
      obb.Center.x = center.x;
      obb.Center.y = center.y;
      obb.Center.z = center.z;
      obb.Extents.x = ext.x;
      obb.Extents.y = ext.y;
      obb.Extents.z = ext.z;
      BQuaternion rot(mat);
      obb.Orientation.x = rot.x;
      obb.Orientation.y = rot.y;
      obb.Orientation.z = rot.z;
      obb.Orientation.w = rot.w;

      //XXXHalwes - 7/24/2208 - Debug
      //gpDebugPrimitives->addDebugBox(mat, ext, cDWORDRed);      

      // Iterate through resulting units and test against OBB   
      for (int i = (numResults - 1); i >= 0; i--)
      {
         const BUnit* pUnit = NULL;
         
         // if we're doing a squad check, check the OBB of the leader unit
         BEntityID resultId = results.get(i);
         if (isSquadCheck && resultId.getType() == BEntity::cClassTypeSquad)
         {
            const BSquad* pSquad = gWorld->getSquad(resultId);
            if (pSquad)
               pUnit = pSquad->getLeaderUnit();
         }
         else
         {
            pUnit = gWorld->getUnit(results.get(i));
         }

         if (pUnit)
         {
            const BBoundingBox* unitBB = pUnit->getSimBoundingBox();
            ATGCollision::AxisAlignedBox aabb;
            const BVector unitBBCenter = unitBB->getCenter();
            aabb.Center.x = unitBBCenter.x;
            aabb.Center.y = unitBBCenter.y;
            aabb.Center.z = unitBBCenter.z;
            const float* unitBBExt = unitBB->getExtents();
            aabb.Extents.x = unitBBExt[0];
            aabb.Extents.y = unitBBExt[1];
            aabb.Extents.z = unitBBExt[2];
            if (!ATGCollision::IntersectAxisAlignedBoxOrientedBox(&aabb, &obb))
            {
               results.removeIndex(i);
            }
         }
      }      
   }

   BASSERT(results.getSize() < 512);
   return(results.getSize());
}

//==============================================================================
// 
//==============================================================================
void BWorld::notify2(DWORD eventType, long targetPlayerID, long targetProtoID, long targetProtoSquadID, long sourcePlayerID, long sourceProtoID, long sourceProtoSquadID, float xp, long level)
{
   gStatsManager.notify(eventType, targetPlayerID, targetProtoID, targetProtoSquadID, sourcePlayerID, sourceProtoID, sourceProtoSquadID, xp, level);
}

//==============================================================================
// BWorld::notify
//==============================================================================
void BWorld::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   gStatsManager.notify(eventType, senderID, data, data2);
   // user needs to see these events
   gUserManager.notify(eventType, senderID, data, data2);

   switch(eventType)
   {
      case BEntity::cEventBuildPercent:
      case BEntity::cEventSquadModeChanaged:
      {
         BUnit* pUnit=gWorld->getUnit(senderID);
         if(pUnit)
            pUnit->notify(eventType, senderID, data, data2);
         break;
      }

      case BEntity::cEventPlayerWon:
      case BEntity::cEventPlayerResigned:
      case BEntity::cEventPlayerDefeated:
         if (gLiveSystem)
         {
//-- FIXING PREFIX BUG ID 5962
            const BPlayer* pPlayer = getPlayer(data);
//--
            if (pPlayer)
            {
               gLiveSystem->playerLeftGameplay(pPlayer->getXUID());
            }
         }
         checkForGameOver();
         break;

      case BEntity::cEventPlayerDisconnected:
      {
         // if a player disconnects, then they'll be set to cPlayerStateDisconnected
         // however, we don't want to trigger a game over condition if I happen to be
         // the remaining player, instead wait for the network layer to send up the
         // session disconnected event which will eventually call BUser::endGameDisconnect
         break;
      }

      case BEntity::cEventBuilt:
         adjustPlayerStrength(senderID, true);
         break;
      case BEntity::cEventKilled:
         adjustPlayerStrength(senderID, false);
         break;
      case BEntity::cEventTechResearched:
         adjustPlayerStrength(data2, data);
         break;
   }
}

//==============================================================================
//==============================================================================
BAI* BWorld::createAI(BPlayerID playerID, BTeamID teamID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAI(playerID, teamID));
}


//==============================================================================
//==============================================================================
BAI* BWorld::getAI(BPlayerID playerID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAI(playerID));
}


//==============================================================================
//==============================================================================
const BAI* BWorld::getAI(BPlayerID playerID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAI(playerID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAI(BPlayerID playerID)
{
   if (mpAIManager)
      mpAIManager->deleteAI(playerID);
}


//==============================================================================
//==============================================================================
BKB* BWorld::createKB(BTeamID teamID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createKB(teamID));
}


//==============================================================================
//==============================================================================
BKB* BWorld::getKB(BTeamID teamID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKB(teamID));
}


//==============================================================================
//==============================================================================
const BKB* BWorld::getKB(BTeamID teamID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKB(teamID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteKB(BTeamID teamID)
{
   if (mpAIManager)
      mpAIManager->deleteKB(teamID);
}


//==============================================================================
//==============================================================================
BAIMission* BWorld::createAIMission(BPlayerID playerID, BAIMissionType missionType)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAIMission(playerID, missionType));
}


//==============================================================================
//==============================================================================
BAIMission* BWorld::getAIMission(BAIMissionID missionID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIMission(missionID));
}


//==============================================================================
//==============================================================================
const BAIMission* BWorld::getAIMission(BAIMissionID missionID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIMission(missionID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAIMission(BAIMissionID missionID)
{
   if (mpAIManager)
      mpAIManager->deleteAIMission(missionID);
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper* BWorld::createWrapper(BAIMissionID missionID, BAIMissionTargetID targetID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createWrapper(missionID, targetID));
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper* BWorld::getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getWrapper(missionTargetWrapperID));
}


//==============================================================================
//==============================================================================
const BAIMissionTargetWrapper* BWorld::getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getWrapper(missionTargetWrapperID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteWrapper(BAIMissionTargetWrapperID missionTargetWrapperID)
{
   if (mpAIManager)
      mpAIManager->deleteWrapper(missionTargetWrapperID);
}


//==============================================================================
//==============================================================================
BAIGroup* BWorld::createAIGroup(BAIMissionID missionID, BProtoSquadID protoSquadID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAIGroup(missionID, protoSquadID));
}


//==============================================================================
//==============================================================================
BAIGroup* BWorld::getAIGroup(BAIGroupID groupID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIGroup(groupID));
}


//==============================================================================
//==============================================================================
const BAIGroup* BWorld::getAIGroup(BAIGroupID groupID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIGroup(groupID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAIGroup(BAIGroupID groupID)
{
   if (mpAIManager)
      mpAIManager->deleteAIGroup(groupID);
}


//==============================================================================
//==============================================================================
BAIGroupTask* BWorld::createAIGroupTask(BAIGroupID groupID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAIGroupTask(groupID));
}


//==============================================================================
//==============================================================================
BAIGroupTask* BWorld::getAIGroupTask(BAIGroupTaskID groupTaskID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIGroupTask(groupTaskID));
}


//==============================================================================
//==============================================================================
const BAIGroupTask* BWorld::getAIGroupTask(BAIGroupTaskID groupTaskID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIGroupTask(groupTaskID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAIGroupTask(BAIGroupTaskID groupTaskID)
{
   if (mpAIManager)
      mpAIManager->deleteAIGroupTask(groupTaskID);
}


//==============================================================================
//==============================================================================
BAIMissionTarget* BWorld::createAIMissionTarget(BPlayerID playerID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAIMissionTarget(playerID));
}


//==============================================================================
//==============================================================================
BAIMissionTarget* BWorld::getAIMissionTarget(BAIMissionTargetID missionTargetID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIMissionTarget(missionTargetID));
}


//==============================================================================
//==============================================================================
const BAIMissionTarget* BWorld::getAIMissionTarget(BAIMissionTargetID missionTargetID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAIMissionTarget(missionTargetID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAIMissionTarget(BAIMissionTargetID missionTargetID)
{
   if (mpAIManager)
      mpAIManager->deleteAIMissionTarget(missionTargetID);
}


//==============================================================================
//==============================================================================
BAITopic* BWorld::createAITopic(BPlayerID playerID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAITopic(playerID));
}


//==============================================================================
//==============================================================================
BAITopic* BWorld::getAITopic(BAITopicID topicID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAITopic(topicID));
}


//==============================================================================
//==============================================================================
const BAITopic* BWorld::getAITopic(BAITopicID topicID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAITopic(topicID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteAITopic(BAITopicID topicID)
{
   if (mpAIManager)
      mpAIManager->deleteAITopic(topicID);
}


//==============================================================================
//==============================================================================
BKBSquad* BWorld::createKBSquad(BTeamID teamID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createKBSquad(teamID));
}


//==============================================================================
//==============================================================================
BKBSquad* BWorld::getKBSquad(BKBSquadID kbSquadID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKBSquad(kbSquadID));
}


//==============================================================================
//==============================================================================
const BKBSquad* BWorld::getKBSquad(BKBSquadID kbSquadID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKBSquad(kbSquadID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteKBSquad(BKBSquadID kbSquadID)
{
   if (mpAIManager)
      mpAIManager->deleteKBSquad(kbSquadID);
}


//==============================================================================
//==============================================================================
BKBBase* BWorld::createKBBase(BTeamID teamID, BPlayerID playerID, BVector position, float radius)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createKBBase(teamID, playerID, position, radius));
}


//==============================================================================
//==============================================================================
BKBBase* BWorld::getKBBase(BKBBaseID kbBaseID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKBBase(kbBaseID));
}


//==============================================================================
//==============================================================================
const BKBBase* BWorld::getKBBase(BKBBaseID kbBaseID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getKBBase(kbBaseID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteKBBase(BKBBaseID kbBaseID)
{
   if (mpAIManager)
      mpAIManager->deleteKBBase(kbBaseID);
}


//==============================================================================
//==============================================================================
BBid* BWorld::createBid(BPlayerID playerID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createBid(playerID));
}


//==============================================================================
//==============================================================================
BBid* BWorld::getBid(BBidID bidID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getBid(bidID));
}


//==============================================================================
//==============================================================================
const BBid* BWorld::getBid(BBidID bidID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getBid(bidID));
}


//==============================================================================
//==============================================================================
void BWorld::deleteBid(BBidID bidID)
{
   if (mpAIManager)
      mpAIManager->deleteBid(bidID);
}

//==============================================================================
//==============================================================================
BAITeleporterZone* BWorld::createAITeleporterZone(BEntityID unitID, BVector boxExtent1, BVector boxExtent2)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAITeleporterZone(unitID, boxExtent1, boxExtent2));
}

//==============================================================================
//==============================================================================
BAITeleporterZone* BWorld::createAITeleporterZone(BEntityID unitID, BVector spherePos, float sphereRad)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->createAITeleporterZone(unitID, spherePos, sphereRad));
}

//==============================================================================
//==============================================================================
BAITeleporterZone* BWorld::getAITeleporterZone(BAITeleporterZoneID zoneID)
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAITeleporterZone(zoneID));
}

//==============================================================================
//==============================================================================
const BAITeleporterZone* BWorld::getAITeleporterZone(BAITeleporterZoneID zoneID) const
{
   if (!mpAIManager)
      return (NULL);
   return (mpAIManager->getAITeleporterZone(zoneID));
}

//==============================================================================
//==============================================================================
void BWorld::deleteAITeleporterZone(BAITeleporterZoneID zoneID)
{
   if (mpAIManager)
      mpAIManager->deleteAITeleporterZone(zoneID);
}


//==============================================================================
// BWorld::adjustPlayerStrength
//==============================================================================
void BWorld::adjustPlayerStrength(BEntityID entityID, bool add)
{
   // determine the entity type
   BEntity* pEntity = gWorld->getEntity(entityID);
   if (pEntity)
   {
      // If a hero unit that does not die then do not subtract from player strength
      const BProtoObject* pProtoObject = pEntity->getProtoObject();
      if (!add && pProtoObject && pProtoObject->isType(gDatabase.getOTIDHeroDeath()))
      {
         return;
      }

      int strength=0;
      BPlayer* pPlayer = NULL;
      switch (pEntity->getClassType())
      {
         case BEntity::cClassTypeSquad:
            {
//-- FIXING PREFIX BUG ID 5963
               const BSquad* pSquad = reinterpret_cast<BSquad*>(pEntity);
//--
               pPlayer = gWorld->getPlayer( pSquad->getPlayerID() );
               const BProtoSquad* pProto = pSquad->getProtoSquad();
               if (pProto && pPlayer)
                  strength = pProto->getCost()->getTotal();
            }
            break;
      //case BEntity::cClassTypeUnit:
      //   {
      //      BUnit* pUnit=reinterpret_cast<BUnit*>(pEntity);
      //      pPlayer = pUnit->getPlayer();
      //      const BProtoObject* pProto = pUnit->getProtoObject();
      //      if (pProto && pPlayer)
      //         strength = pProto->getCost()->getTotal();
      //   }
      //   break;
      }

      if (pPlayer)
      {
         if (add)
            pPlayer->addStrength(strength);
         else
            pPlayer->subtractStrength(strength);
      }

   }
}


//==============================================================================
// BWorld::adjustPlayerStrength
//==============================================================================
void BWorld::adjustPlayerStrength(int playerID, int techID)
{
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
//-- FIXING PREFIX BUG ID 5964
      const BProtoTech* pProtoTech=pPlayer->getProtoTech(techID);
//--
      if (pProtoTech)
         pPlayer->addStrength( pProtoTech->getCost()->getTotal() );  // add the cost of the tech to the score for the player.
   }
}

//==============================================================================
// BWorld::flushCachedAnimEvents
//==============================================================================
void BWorld::flushCachedAnimEvents(BEntityID entity, bool forceFire)
{
   // This lets dopples fire tags for their parent objects
   if (forceFire)
   {
      mpStoredEventTagManager->fireAllEventTags(entity,true);
      return;
   }

   //CLM - Currently this is used for terrain alpha tags
   //Upon first visible to the game, buildings will change their visibility, causing a false-positive
   //to fire this code.
   //This code won't let a building who's under FOW to fire any tags
   //this occurs when a building is placed in the editor, and getBuildingConstructionAlphaTerrainTags is called to
   //grab all existing tags.
   //This logic MAY need to be moved somwehere else in the case that other systems start calling this function.
   BObject *pObject = gWorld->getObject(entity);

   if (!pObject)
      return;

   const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);BASSERT(pPrimaryUser);
   
   const BPlayer * pPlayer = 0;
   if(pPrimaryUser)
      pPlayer= pPrimaryUser->getPlayer();
   
   BASSERT(pPlayer);

   if(!pPlayer)
      return;

   BTeamID playerTeamID = pPlayer->getTeamID();
   const BTeam * const pTeam = gWorld->getTeam(playerTeamID);
   if (!pTeam)
      return;

   //Ask the team instead of the object
   if(pTeam->isObjectVisible(entity))
      mpStoredEventTagManager->fireAllEventTags(entity,true);
}
//==============================================================================
// BWorld::handleCachedAnimEvent
//==============================================================================
bool BWorld::handleCachedAnimEvent(long eventType, BProtoVisualTag* pTag, BVector EventLocation, void* userData)
{
   switch(eventType)
   {
   case cAnimEventAlphaTerrain:
      {
            bool value = pTag->mData0?true:false;

            float xPos =EventLocation.x;
            float zPos =EventLocation.z;

            if(!pTag->mBoolValue0)//rectangle version
            {
               float xRadius = pTag->mValue0;   //in world space
               float zRadius = pTag->mValue1;   //in world space


               //we have to rotate our rectangle in worldspace...
//-- FIXING PREFIX BUG ID 5967
               const BMatrix* rotation = reinterpret_cast<BMatrix*>(userData);
//--

               BVector v0(- xRadius,0,- zRadius);
               BVector v1(- xRadius,0,  zRadius);
               BVector v2(  xRadius,0,  zRadius);
               BVector v3(  xRadius,0,- zRadius);

               rotation->transformVector(v0,v0);
               rotation->transformVector(v1,v1);
               rotation->transformVector(v2,v2);
               rotation->transformVector(v3,v3);


               v0.x =(v0.x+xPos)/mTerrainMax.x;
               v1.x =(v1.x+xPos)/mTerrainMax.x;
               v2.x =(v2.x+xPos)/mTerrainMax.x;
               v3.x =(v3.x+xPos)/mTerrainMax.x;

               v0.z =(v0.z+zPos)/mTerrainMax.z;
               v1.z =(v1.z+zPos)/mTerrainMax.z;
               v2.z =(v2.z+zPos)/mTerrainMax.z;
               v3.z =(v3.z+zPos)/mTerrainMax.z;

               gTerrainDynamicAlpha.setToRegionToValueRectangleOriented(v0.x,v0.z,
                  v1.x,v1.z,
                  v2.x,v2.z,
                  v3.x,v3.z,
                  value);
            }
            else //circle version
            {
               float xRadius = pTag->mValue0;   //in world space

               float cX = xPos/ mTerrainMax.x;
               float cY = zPos/ mTerrainMax.z;

               gTerrainDynamicAlpha.setToRegionToValueCircle(cX,cY,xRadius,value);
            }


         }
         break;
      case cAnimEventBuildingDecal:
         {
            BTerrainStaticDecalHandle hdl;
            hdl.mSizeX = pTag->mValue0;
            hdl.mSizeZ = pTag->mValue1;
            hdl.mImpactTextureName  = pTag->mName;

//-- FIXING PREFIX BUG ID 5968
            const BVector4* v = reinterpret_cast<BVector4*>(userData);
//--
            BVector bv(v->x,v->y, v->z);

            gImpactDecalManager.createStaticBuildingDecal(EventLocation, &hdl, (int) v->w,bv);

            break;
         }
      case cAnimEventUVOffset:
         {
            break;
         }
      }
   

   return true;
}
//==============================================================================
// BWorld::handleAnimEvent
//==============================================================================
bool BWorld::handleAnimEvent(long attachmentHandle, long animType, long eventType, int64 userData, BProtoVisualTag* pTag, BVisual* pVisual)
{
   SCOPEDSAMPLE(BWorld_handleAnimEvent)

   switch(eventType)
   {
      case cAnimEventLoop:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
#ifdef SYNC_Anim
            if(pObject->getUnit())
            {
               syncAnimData("BWorld::handleAnimEvent loop unitID", pObject->getID().asLong());
               syncAnimData("BWorld::handleAnimEvent loop animType", gVisualManager.getAnimName(animType));
               if (pTag)
               {
                  syncAnimData("BWorld::handleAnimEvent loop track", pTag->mData0);
               }
            }
#endif
            if(attachmentHandle==-1)
               pObject->notify(BEntity::cEventAnimLoop, pObject->getID(), pVisual->getAnimationType(pTag->mData0), pTag->mData0);
            else
            {
//-- FIXING PREFIX BUG ID 5969
               const BVisualItem* pAttachment = pVisual->getAttachment(attachmentHandle);
//--
               if (pAttachment)
                  pObject->notify(BEntity::cEventAttachmentAnimLoop, pObject->getID(), attachmentHandle, pAttachment->getAnimationType(pTag->mData0));
            }
         }
         break;
      }

      case cAnimEventEnd:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
#ifdef SYNC_Anim
            if(pObject->getUnit())
            {
               syncAnimData("BWorld::handleAnimEvent end unitID", pObject->getID().asLong());
               syncAnimData("BWorld::handleAnimEvent end animType", gVisualManager.getAnimName(animType));
               if (pTag)
               {
                  syncAnimData("BWorld::handleAnimEvent end track", pTag->mData0);
               }
            }
#endif
            if(attachmentHandle==-1)
               pObject->notify(BEntity::cEventAnimEnd, pObject->getID(), pVisual->getAnimationType(pTag->mData0), pTag->mData0);
            else
            {
//-- FIXING PREFIX BUG ID 5970
               const BVisualItem* pAttachment = pVisual->getAttachment(attachmentHandle);
//--
               if (pAttachment)
                  pObject->notify(BEntity::cEventAttachmentAnimEnd, pObject->getID(), attachmentHandle, pAttachment->getAnimationType(pTag->mData0));
            }
         }
         break;
      }

      case cAnimEventChain:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
#ifdef SYNC_Anim
            if(pObject->getUnit())
            {
               syncAnimData("BWorld::handleAnimEvent chain unitID", pObject->getID().asLong());
               syncAnimData("BWorld::handleAnimEvent chain animType", gVisualManager.getAnimName(animType));
               if (pTag)
               {
                  syncAnimData("BWorld::handleAnimEvent chain to animType", gVisualManager.getAnimName(pTag->mValueInt1));
                  syncAnimData("BWorld::handleAnimEvent chain track", pTag->mData0);
               }
            }
#endif
            if(attachmentHandle==-1)
            {
               // TRB 9/4/08 - Do the startChain call first which only sets anim state.  The notify call can also do this and should
               // have the final word on what the anim state is changed to.
               if (pTag->mData0 == cActionAnimationTrack)
                  pObject->startChain(pVisual->getAnimationType(pTag->mData0), pTag->mValueInt1);

               pObject->notify(BEntity::cEventAnimChain, pObject->getID(), pVisual->getAnimationType(pTag->mData0), pTag->mData0);
            }
            else
            {
//-- FIXING PREFIX BUG ID 5971
               const BVisualItem* pAttachment = pVisual->getAttachment(attachmentHandle);
//--
               if (pAttachment)
                  pObject->notify(BEntity::cEventAttachmentAnimChain, pObject->getID(), attachmentHandle, pAttachment->getAnimationType(pTag->mData0));
            }
         }
         break;
      }

      case cAnimEventAttack:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
            pObject->notify(BEntity::cEventAnimAttackTag, pObject->getID(), attachmentHandle, pTag->mToBoneHandle);
         }
         break;
      }

      case cAnimEventSound:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
            bool checkFOW = pTag->getFlag(BProtoVisualTag::cFlagCheckFOW);
            bool checkVisible = pTag->getFlag(BProtoVisualTag::cFlagCheckVisible);
            BCueIndex index = gSoundManager.getCueIndex(pTag->mName);
            mpWorldSoundManager->addSound(pObject, pTag->mToBoneHandle, index, checkFOW, cInvalidCueIndex, checkVisible, (checkFOW || checkVisible));  //If the sound is checking FOW or Visible, it is not critical, so allow it to be dropped.
         }
         break;
      }

      case cAnimEventParticles:
         break;

      case cAnimEventTerrainEffect:
      {
         BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(pTag->mData0, true);
         if(pTerrainEffect)
         {
            const BObject * const pObject = gWorld->getObject(BEntityID(userData));
            
            const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);
            BASSERT(pPrimaryUser);
            
            const BPlayer * pPlayer = 0;
            if(pPrimaryUser)
               pPlayer = pPrimaryUser->getPlayer();
            BASSERT(pPlayer);

            if (pObject && pPlayer && pObject->isVisible(pPlayer->getTeamID()))
            {
               if(pTag->mToBoneHandle==-1)
               {
                  pTerrainEffect->instantiateEffect(pObject->getPosition(), pObject->getForward(), true);
               }
               else
               {
                  BMatrix worldMatrix;
                  pObject->getWorldMatrix(worldMatrix);

                  BMatrix boneMatrix;
                  pObject->getVisual()->getBone(pTag->mToBoneHandle, NULL, &boneMatrix);

                  BMatrix finalMatrix;
                  finalMatrix.mult(boneMatrix, worldMatrix);


                  BVector pos, dir;
                  finalMatrix.getTranslation(pos);
                  finalMatrix.getForward(dir);

                  pTerrainEffect->instantiateEffect(pos, dir, true);
               }
            }
         }
         break;
      }

      case cAnimEventCameraShake:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
            if (pTag->getFlag(BProtoVisualTag::cFlagCheckSelected))
            {
               if (!gUserManager.getPrimaryUser()->getSelectionManager()->isUnitSelected(pObject->getID()))
               {
                  if (!gGame.isSplitScreen())
                     break;
                  if (!gUserManager.getSecondaryUser()->getSelectionManager()->isUnitSelected(pObject->getID()))
                     break;
               }
            }

            // ajl 6/27/07 fixme - Hack for now to see if the unit's position is on-screen.
            // I'm not checking to see if the full object is on-screen because the bounding box for
            // the scarab is too big.
            // Sergio is adding a bone position parameter to the camera shake tag to fix this.
            if (gUserManager.getPrimaryUser()->isSphereVisible(pObject->getPosition(), 1.0f))
            {
               BCamera* pCamera = gUserManager.getPrimaryUser()->getCamera();
               pCamera->clearShake();
               pCamera->beginShake(pTag->mLifespan, pTag->mValue1);
            }
            if (gGame.isSplitScreen())
            {
               if (gUserManager.getSecondaryUser()->isSphereVisible(pObject->getPosition(), 1.0f))
               {
                  BCamera* pCamera = gUserManager.getSecondaryUser()->getCamera();
                  pCamera->clearShake();
                  pCamera->beginShake(pTag->mLifespan, pTag->mValue1);
               }
            }

            pObject->notify(cAnimEventCameraShake, pObject->getID(),0,0);
         }       
         break;
      }

      case cAnimEventGroundIK:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if (pObject)
         {
            pObject->notify(BEntity::cEventAnimGroundIKTag, pObject->getID(), (DWORD) pTag, 0);
         }
         break;
      }

      case cAnimEventKillAndThrow:
      {
         BEntityID unitID = BEntityID(userData);
         BUnit *pUnit = gWorld->getUnit(unitID);
         if (pUnit && pUnit->getFlagDoingFatality() && pUnit->getVisual())
         {
            const BActiveFatality* pFatality = gFatalityManager.getFatality(unitID);
            if (pFatality)
            {
//-- FIXING PREFIX BUG ID 5974
               const BUnit* pAttacker = gWorld->getUnit(pFatality->mAttackerID);
//--
               if (pAttacker && pAttacker->getFlagDoingFatality())
               {
                  BEntityID attackerUnitID = pFatality->mAttackerID;
                  BPlayerID attackerPlayerID = pAttacker->getPlayerID();

                  // Got one, kill and throw
                  if ((attackerUnitID != cInvalidObjectID) && (attackerPlayerID != cInvalidPlayerID))
                  {
                     // Compute throw velocity in world space
                     BMatrix worldMatrix;
                     BMatrix worldMatrixInv;
                     BVector velocity(pTag->mValue0, pTag->mValue1, pTag->mValue2);

                     pUnit->getWorldMatrix(worldMatrix);
                     worldMatrixInv = worldMatrix;
                     worldMatrixInv.transformVector(velocity, velocity);

                     BVector damagePos;
                     if (!pUnit->getVisual()->getBone(pTag->mToBoneHandle, &damagePos, NULL, NULL, &worldMatrix))
                        damagePos = pUnit->getPosition();

                     // Draw debug impulse line
                     #ifndef BUILD_FINAL
                        if (gConfig.isDefined(cConfigDebugRenderShape))
                        {
                           BVector dir = velocity;
                           dir.safeNormalize();
                           gpDebugPrimitives->addDebugLine(damagePos, damagePos + dir * 2.0f, cDWORDRed, cDWORDGreen, BDebugPrimitives::cCategoryNone, 2.0f);
                        }
                     #endif

                     // If the unit isn't dead and can be damaged, do that.  Otherwise do the physics stuff.
                     float dmgAmt = pUnit->getHitpoints() + pUnit->getShieldpoints();
                     if (dmgAmt > 0.0f)
                     {
                        BDamage damage;
                        damage.mDamagePos = damagePos;
                        damage.mDirection = velocity;
                        damage.mAttackerID = attackerUnitID;
                        damage.mAttackerTeamID = attackerPlayerID;
                        damage.mDamage = dmgAmt;
                        damage.mDamageMultiplier = 1.0f;
                        // Avoid division by 0.0f.
                        if (pUnit->getDamageTakenScalar() > cFloatCompareEpsilon)
                           damage.mDamageMultiplier /= pUnit->getDamageTakenScalar();
                        damage.mShieldDamageMultiplier = damage.mDamageMultiplier;
                        damage.mForceThrow = true;
                        damage.mPhysReplacementMatchBone = pTag->mToBoneHandle;

                        pUnit->damage(damage);
                     }
                     else
                     {
                        pUnit->killAndThrowPhysicsReplacement(velocity, damagePos, pTag->mToBoneHandle);
                     }
                  }
               }
            }
         }
         break;
      }

      case cAnimEventAttachTarget:
      {
         BUnit *pUnit = gWorld->getUnit(BEntityID(userData));
         if (pUnit)
         {
            BEntityID targetUnitID = pUnit->getAttackTargetID();
            BUnit *pTargetUnit = gWorld->getUnit(targetUnitID);
            if (pTargetUnit)
            {
               bool attach = pTag->getFlag(BProtoVisualTag::cFlagAttach);

               if (attach)
               {
                  pUnit->attachObject(targetUnitID, pTag->mToBoneHandle);
               }
               else
               {
                  bool toGround = true;
                  if (pUnit->getProtoObject() && pUnit->getProtoObject()->getFlagKillOnDetach())
                     toGround = false;

                  pUnit->unattachObject(targetUnitID);
                  if (toGround)
                  {
                     pTargetUnit->tieToGround();
                     pTargetUnit->orientWithGround();
                  }
               }
            }
         }
         break;
      }

      case cAnimEventSweetSpot:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if (pObject)
         {
            pObject->notify(BEntity::cEventAnimSweetSpotTag, pObject->getID(), (DWORD)pTag, NULL);
         }
         break;
      }

      case cAnimEventBuildingDecal:
         {

            BEntityID bID = BEntityID(userData);
            const BObject * const pObject = gWorld->getObject(bID);

      
            if (pObject && !pObject->isClassType(BObject::cClassTypeDopple))
            {
               const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);BASSERT(pPrimaryUser);

               const BPlayer * pPlayer = 0;
               if(pPrimaryUser)
                  pPlayer = pPrimaryUser->getPlayer();BASSERT(pPlayer);

               const BPlayer * const objectOwner = getPlayer(pObject->getPlayerID());

               BVector eventLocation= pObject->getPosition();


               if(pTag->mToBoneHandle!=-1)
               {
                  BVector bonePos;
                  pObject->getVisual()->getBone(pTag->mToBoneHandle, &bonePos);
                  eventLocation+=bonePos;
               }
               else if(pTag->mToBoneName!="")
               {
                  long hdl = pObject->getVisual()->getBoneHandle(pTag->mToBoneName);
                  BVector bonePos;
                  pObject->getVisual()->getBone(hdl, &bonePos);
                  eventLocation+=bonePos;
               }

               BVector4 forward = pObject->getForward();

               if(objectOwner)
               {
                  if(objectOwner->getCivID() == gDatabase.getCivID("UNSC"))
                     forward.w = 0;
                  else if(objectOwner->getCivID() == gDatabase.getCivID("Covenant"))
                     forward.w = 1;
               }
               
               if(pPlayer && pObject->isVisible(pPlayer->getTeamID()))
               {
                  //CLM i don't like duplicating code, so i'm calling this function
                  //internally they should act the same
                  handleCachedAnimEvent(eventType,pTag,eventLocation, &forward);
               }
               else
               {
                  //if we're not visible cache the event for later
                  mpStoredEventTagManager->addEventTag(bID, eventType,eventLocation,pTag,&forward, sizeof(forward));
               }
            }
            break;
         }
      case cAnimEventAlphaTerrain:
         {
            BEntityID bID = BEntityID(userData);
            const BObject * const pObject = gWorld->getObject(bID);

            if (pObject && !pObject->isClassType(BObject::cClassTypeDopple))
            {
               const BUser * const pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               BASSERT(pPrimaryUser);

               const BPlayer * pPlayer = 0;
               if(pPrimaryUser)
                  pPlayer = pPrimaryUser->getPlayer();
               BASSERT(pPlayer);
               
               BVector eventLocation= pObject->getPosition();
               BMatrix rot;
               pObject->getRotation(rot);
               
               if(pTag->mToBoneHandle!=-1)
               {
                  BVector bonePos;
                  pObject->getVisual()->getBone(pTag->mToBoneHandle, &bonePos);
                  rot.transformVector(bonePos,bonePos);
                  eventLocation+=bonePos;
               }
               else if(pTag->mToBoneName!="")
               {
                  long hdl = pObject->getVisual()->getBoneHandle(pTag->mToBoneName);
                  BVector bonePos;
                  pObject->getVisual()->getBone(hdl, &bonePos);
                  rot.transformVector(bonePos,bonePos);
                  eventLocation+=bonePos;
               }

               if(pPlayer && pObject->isVisible(pPlayer->getTeamID()))
               {
                  //CLM i don't like duplicating code, so i'm calling this function
                  //internally they should act the same
                  handleCachedAnimEvent(eventType,pTag,eventLocation, &rot);
               }
               else
               {
                  //if we're not visible cache the event for later
                  mpStoredEventTagManager->addEventTag(bID, eventType,eventLocation,pTag,&rot, sizeof(rot));
               }
            }
            break;
         }

      case cAnimEventRumble:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if(pObject)
         {
            if (pTag->getFlag(BProtoVisualTag::cFlagCheckSelected))
            {
               if (!gUserManager.getPrimaryUser()->getSelectionManager()->isUnitSelected(pObject->getID()))
               {
                  if (!gGame.isSplitScreen())
                     break;
                  if (!gUserManager.getSecondaryUser()->getSelectionManager()->isUnitSelected(pObject->getID()))
                     break;
               }
            }

            // ajl 6/27/07 fixme - Hack for now to see if the unit's position is on-screen.
            // I'm not checking to see if the full object is on-screen because the bounding box for
            // the scarab is too big.

            // ajl fixme 11/27/07 - rumble needs to work per user instead of globally
            if (gWorld->isSphereVisible(pObject->getPosition(), 1.0f))
               gUI.playRumbleEvent(BRumbleEvent::cTypeAnimTag, pTag->mData0, pTag->mValue0, pTag->mValueInt2, pTag->mValue1, pTag->mLifespan, false);//Don't use loop value because the rumble will never stop (pTag->mBoolValue0)
         }
         break;
      }
      case cAnimEventUVOffset:
         {
            BObject *pObject = gWorld->getObject(BEntityID(userData));
            if(pObject  )
            {
               pObject->setUV0Offset(pTag->mValue0, pTag->mValue1);
            }
            break;
         }
      case cAnimEventPhysicsImpulse:
      {
         BObject *pObject = gWorld->getObject(BEntityID(userData));
         if (pObject && pVisual)
         {
            // Get the physics object to apply the impulse to
            BPhysicsObject* pPO = NULL;
            if (pTag->mBoolValue0)
            {
               BObject* pAttachedToObject = pObject->getAttachedToObject();
               if (pAttachedToObject)
                  pPO = pAttachedToObject->getPhysicsObject();
            }
            else
               pPO = pObject->getPhysicsObject();

            // Calculate and apply impulse
            if (pPO)
            {
               // Get bone
               BVector bonePos;
               BMatrix boneMtx;
               BMatrix worldMtx;
               pObject->getWorldMatrix(worldMtx);

               bool foundBone = false;
               if (attachmentHandle == -1)
               {
                  foundBone = pVisual->getBone(pTag->mToBoneHandle, &bonePos, &boneMtx, NULL, &worldMtx);

                  // If couldn't find bone, just use the object's matrix
                  if (!foundBone)
                  {
                     worldMtx.getTranslation(bonePos);
                     boneMtx = worldMtx;
                     foundBone = true;
                  }
               }
               else
               {
                  BMatrix attachmentMat;
//-- FIXING PREFIX BUG ID 5979
                  const BVisualItem* pAttachment = pVisual->getAttachment(attachmentHandle, &attachmentMat);
//--
                  if (pAttachment)
                  {
                     BMatrix mat;
                     mat.mult(attachmentMat, worldMtx);

                     foundBone = pAttachment->getBone(pTag->mToBoneHandle, &bonePos, &boneMtx, NULL, &mat);

                     // If couldn't find bone, just use attachment's matrix
                     if (!foundBone)
                     {
                        mat.getTranslation(bonePos);
                        boneMtx = mat;
                        foundBone = true;
                     }
                  }
               }

               if (foundBone)
               {
                  // Angular impulse
                  if (pTag->mData0 == 0)
                  {
                     // TODO - this doesn't account for the inertia
                     BVector forces(pTag->mValue0, pTag->mValue1, pTag->mValue2);
                     BVector impulse;
                     boneMtx.transformVector(forces, impulse);
                     impulse *= pPO->getMass();
                     pPO->applyAngularImpulse(impulse);

                     // Debug rendering
                     #ifndef BUILD_FINAL
                        if (gConfig.isDefined(cConfigDebugPhysicsImpulseTags))
                        {
                           BVector p1, p2;
                           impulse.normalize();
                           pPO->getCenterOfMassLocation(p2);
                           p1 = p2 - impulse * 4.0f;
                           gpDebugPrimitives->addDebugLine(p1, p2, cDWORDYellow, cDWORDYellow, BDebugPrimitives::cCategoryNone, 2.0f);
                        }
                     #endif
                  }
                  // Linear impulse
                  else if (pTag->mData0 == 1)
                  {
                     BVector forces(pTag->mValue0, pTag->mValue1, pTag->mValue2);
                     BVector impulse;
                     boneMtx.transformVector(forces, impulse);
                     impulse *= pPO->getMass();
                     pPO->applyImpulse(impulse);

                     // Debug rendering
                     #ifndef BUILD_FINAL
                        if (gConfig.isDefined(cConfigDebugPhysicsImpulseTags))
                        {
                           BVector p1, p2;
                           impulse.normalize();
                           pPO->getCenterOfMassLocation(p2);
                           p1 = p2 - impulse * 4.0f;
                           gpDebugPrimitives->addDebugLine(p1, p2, cDWORDBlue, cDWORDBlue, BDebugPrimitives::cCategoryNone, 2.0f);
                        }
                     #endif
                  }
                  // TODO - Point impulse
                  else if (pTag->mData0 == 2)
                  {
                     // TODO - this doesn't account for the inertia
                     BVector forces(pTag->mValue0, pTag->mValue1, pTag->mValue2);
                     BVector impulse;
                     boneMtx.transformVector(forces, impulse);
                     impulse *= pPO->getMass();
                     BVector point = bonePos;
                     pPO->applyPointImpulse(impulse, point);

                     // Debug rendering
                     #ifndef BUILD_FINAL
                        if (gConfig.isDefined(cConfigDebugPhysicsImpulseTags))
                        {
                           BVector p1, p2;
                           impulse.normalize();
                           p1 = bonePos - impulse * 4.0f;
                           p2 = bonePos;
                           gpDebugPrimitives->addDebugLine(p1, p2, cDWORDRed, cDWORDGreen, BDebugPrimitives::cCategoryNone, 2.0f);
                        }
                     #endif
                  }
               }
            }
         }
         /*
         trace("Physics Impulse tag type=%s x=%f, y=%f, z=%f, attached=%s", 
            (pTag->mData0 == 1) ? "Linear" : (pTag->mData0 == 2) ? "Point" : "Angular",
            pTag->mValue0, pTag->mValue1, pTag->mValue2,
            pTag->mBoolValue0 ? "true" : "false");
         */
         break;
      }
   }

   return true;
}

//==============================================================================
// BWorld::handleSetAnimSync
//==============================================================================
bool BWorld::handleSetAnimSync(int64 userData, long animationTrack, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction, BVisual* pVisual)
{
#ifdef SYNC_Anim
//-- FIXING PREFIX BUG ID 5997
   const BUnit* pUnit = gWorld->getUnit(BEntityID(userData));
//--
   if (pUnit)
   {
      syncAnimData("BWorld::handleSetAnimSync track", animationTrack);
      syncAnimData("BWorld::handleSetAnimSync animType", gVisualManager.getAnimName(animType));
      syncAnimData("BWorld::handleSetAnimSync applyInstantly", applyInstantly);
      syncAnimData("BWorld::handleSetAnimSync timeIntoAnimation", timeIntoAnimation);
      syncAnimData("BWorld::handleSetAnimSync reset", reset);
      syncAnimData("BWorld::handleSetAnimSync startOnThisAttachment", (startOnThisAttachment ? true : false));
      if (pOverrideExitAction)
      {
         syncAnimData("BWorld::handleSetAnimSync mExitAction", pOverrideExitAction->mExitAction);
         syncAnimData("BWorld::handleSetAnimSync mTweenTime", pOverrideExitAction->mTweenTime);
         syncAnimData("BWorld::handleSetAnimSync mTweenToAnimation", pOverrideExitAction->mTweenToAnimation);
      }
      if (pVisual)
      {
         syncAnimData("BWorld::handleSetAnimSync visual", pVisual->getProtoVisual()->getName());
      }
   }
#endif
   return true;
}

//==============================================================================
// BWorld::handleVisualLogic
//==============================================================================
long BWorld::handleVisualLogic(BProtoVisualLogicNode* pLogicNode, long randomTag, int64 userData, BProtoVisual* pProtoVisual)
{
   long valueCount=pLogicNode->mLogicValues.getNumber();

#ifdef SYNC_Anim
   bool sync = gRandomManager.getSync(randomTag);
   if (sync)
   {                
      syncAnimData("BWorld::handleVisualLogic valueCount-1", (valueCount-1));
      syncAnimData("BWorld::handleVisualLogic pProtoVisual->getName", pProtoVisual->getName());
      syncAnimData("BWorld::handleVisualLogic randomTag", randomTag);
      syncAnimData("BWorld::handleVisualLogic seed", gRandomManager._getSeed(randomTag));
   }
#endif

   switch(pLogicNode->mLogicType)
   {
      case cVisualLogicVariation:
      {
         long retVal;

#ifdef SYNC_Anim
         if (sync)
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicVariation");
#endif

         long unitID=(long)userData;
//-- FIXING PREFIX BUG ID 5998
         const BUnit* pUnit=gWorld->getUnit(unitID);
//--
         if(pUnit)
         {
            int visVariation = pUnit->getVisualVariationIndex();

#ifdef SYNC_Anim
            if (sync)
               syncAnimData("BWorld::handleVisualLogic visVariation", visVariation);
#endif

            if(visVariation != -1) //-1 is default..
            {
               retVal =  Math::Min<int>(visVariation,valueCount-1);
#ifdef SYNC_Anim
               if (sync)
                  syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
               return retVal;
            }
         }

         retVal =  getRandMax(randomTag, valueCount-1);
#ifdef SYNC_Anim
         if (sync)
            syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
         return retVal;
      }

      case cVisualLogicTech:
      {
         long retVal;

#ifdef SYNC_Anim
         if (sync)
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicTech");
#endif

         long unitID=(long)userData;
//-- FIXING PREFIX BUG ID 6003
         const BUnit* pUnit=gWorld->getUnit(unitID);
//--
         if(pUnit)
         {
//-- FIXING PREFIX BUG ID 6000
            const BTechTree* pTechTree=pUnit->getPlayer()->getTechTree();
//--
            for(long i=0; i<valueCount; i++)
            {
//-- FIXING PREFIX BUG ID 5999
               const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
               long techID=(long)pLogicValue->mValueDWORD;
               if(techID==-1)
               {
                  retVal = i;
#ifdef SYNC_Anim
                  if (sync)
                     syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
                  return retVal;
               }
               // For units with unique proto objects, search their unique tech status first
               long status = -1;
               const BProtoObject* pPO = pUnit->getProtoObject();
               if (pPO && pPO->getUniqueTechStatus(techID, status))
               {
                  if (status == BTechTree::cStatusActive)
                  {
                     retVal = i;
#ifdef SYNC_Anim
                     if (sync)
                        syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
                     return retVal;
                  }
               }
               // Get tech tree status
               else if (pTechTree->getTechStatus(techID, unitID)==BTechTree::cStatusActive)
               {
                  retVal = i;
#ifdef SYNC_Anim
                  if (sync)
                     syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
                  return retVal;
               }
            }
         }
         else
         {
            for(long i=0; i<valueCount; i++)
            {
//-- FIXING PREFIX BUG ID 6001
               const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
               long techID=(long)pLogicValue->mValueDWORD;
               if(techID==-1)
               {
                  retVal = i;
#ifdef SYNC_Anim
                  if (sync)
                     syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
                  return retVal;
               }
            }
         }
         break;
      }

      case cVisualLogicBuildingCompletion:
      {
         long retVal;

         BUnit* pUnit=gWorld->getUnit((long)userData);
         float buildPct=(pUnit ? pUnit->getBuildPercent() : 1.0f);
#ifdef SYNC_Anim
         if (sync)
         {
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicBuildingCompletion");
            syncAnimData("BWorld::handleVisualLogic buildPct", buildPct);
         }
#endif
         for(long i=valueCount-1; i>=0; i--)
         {
//-- FIXING PREFIX BUG ID 6002
            const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
            if(buildPct>=pLogicValue->mValueFloat)
            {
               retVal = i;
#ifdef SYNC_Anim
               if (sync)
                  syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
               return retVal;
            }
         }
         break;
      }

      case cVisualLogicSquadMode:
      {
         long retVal;

         long unitID=(long)userData;
         long squadMode=BSquadAI::cModeNormal;
#ifdef SYNC_Anim
         if (sync)
         {
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicSquadMode");
            syncAnimData("BWorld::handleVisualLogic squadMode", squadMode);
         }
#endif
         BUnit* pUnit=gWorld->getUnit(unitID);
         if(pUnit)
         {
//-- FIXING PREFIX BUG ID 6004
            const BSquad* pSquad=pUnit->getParentSquad();
//--
            if(pSquad)
               squadMode=pSquad->getCurrentOrChangingMode();
         }
         for(long i=0; i<valueCount; i++)
         {
//-- FIXING PREFIX BUG ID 6005
            const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
            long logicSquadMode=(long)pLogicValue->mValueDWORD;
            if(squadMode==logicSquadMode || logicSquadMode==-1)
            {
               retVal = i;
#ifdef SYNC_Anim
               if (sync)
                  syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
               return retVal;
            }
         }
         break;
      }

      case cVisualLogicImpactSize:
      {
         long retVal;

#ifdef SYNC_Anim
         if (sync)
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicImpactSize");
#endif

         for(long i=0; i<valueCount; i++)
         {
//-- FIXING PREFIX BUG ID 6006
            const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
            if((long)pLogicValue->mValueDWORD==(long)userData)
            {
               retVal = i;
#ifdef SYNC_Anim
               if (sync)
                  syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
               return retVal;
            }
         }
         break;
      }

      case cVisualLogicDestruction:
      {
         long retVal;

         BUnit* pUnit=gWorld->getUnit((long)userData);
         float destructionPct = (pUnit ? pUnit->getHPPercentage() : 1.0f);
#ifdef SYNC_Anim
         if (sync)
         {
            syncAnimCode("BWorld::handleVisualLogic cVisualLogicDestruction");
            syncAnimData("BWorld::handleVisualLogic destructionPct", destructionPct);
         }
#endif
         for(long i=valueCount-1; i>=0; i--)
         {
//-- FIXING PREFIX BUG ID 6007
            const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
//--
            if(destructionPct<=pLogicValue->mValueFloat)
            {
               retVal = i;
#ifdef SYNC_Anim
               if (sync)
                  syncAnimData("BWorld::handleVisualLogic retVal", retVal);
#endif
               return retVal;
            }
         }
         break;
      }
   }

#ifdef SYNC_Anim
   if (sync)
      syncAnimData("BWorld::handleVisualLogic retVal", -1);
#endif

   return -1;
}

//==============================================================================
// BWorld::handleVisualPoint
//==============================================================================
long BWorld::handleVisualPoint(long pointType, const BSimString& data)
{
   switch(pointType)
   {
      case cVisualPointImpact:
         //FIXME AJL 6/8/06 - Just test logic that needs to be replaced by an actual data driven system
         // SLB: This is now obsolete. Use surface types.
         if(data=="metal")
            return 0;
         else if(data=="wood")
            return 1;
         break;
   }
   return -1;
}

//==============================================================================
// BWorld::getRandomValue
//==============================================================================
long BWorld::getRandomValue(long randomTag, long minVal, long maxVal)
{
   return getRandRange(randomTag, minVal, maxVal);
}

//==============================================================================
// BWorld::checkPlacement
//==============================================================================
bool BWorld::checkPlacement(const long protoID, const long playerID, BVector location, BVector& suggestion, const BVector forward, long losMode, DWORD flags,
                            long searchScale /*= 1*/, const BUnit* pBuilder /*= NULL*/, BEntityID* pSocketID /*= NULL*/, long placementRuleIndex /*= -1*/,
                            int protoSquadID /*= -1*/, bool useSquadObsRadii /*= false*/, float extraObstructionExpansion /*= 0.0f*/)
{
   if (pSocketID)
      *pSocketID=cInvalidObjectID;

   // Calculate axes
   const BVector up(cYAxisVector);
   const BVector dirRight = up.cross(forward);

   // Calculate matrix
   BMatrix matrix;
   matrix.makeOrient(forward, up, dirRight);
   matrix.setTranslation(location);

   // Get proto object
   const BPlayer* pPlayer = getPlayer(playerID);
//-- FIXING PREFIX BUG ID 5993
   const BProtoObject* pTempProtoObject = NULL;
//--
   if (pPlayer)
   {
      pTempProtoObject = pPlayer->getProtoObject(protoID);
   }
   else
   {
      pTempProtoObject = gDatabase.getGenericProtoObject(protoID);
   }
   const BProtoObject* pProtoObject = pTempProtoObject;

   //BASSERT(pProtoObject);
   // mrh 7/24/06 - Note: Removed the restriction of having to provide a valid pProtoObject->
   // There are instances where I just want to check the LOS of my placement without providing an object. (powers)

   // Expand radius
   float expansionRadius = ((flags & cCPExpandHull) && pPlayer) ? pPlayer->getCiv()->getHullExpansionRadius() : 0.0f;
   expansionRadius += extraObstructionExpansion;
   float protoObstructionRadiusX = (pProtoObject) ? pProtoObject->getObstructionRadiusX() : 0.0f;
   float protoObstructionRadiusZ = (pProtoObject) ? pProtoObject->getObstructionRadiusZ() : 0.0f;

   // Get squad obstruction radii if specified
   if (useSquadObsRadii)
   {
      BProtoSquad* pProtoSquad = NULL;
      if (pPlayer)
         pProtoSquad = pPlayer->getProtoSquad(protoSquadID);
      else
         pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);

      if (pProtoSquad)
         pProtoSquad->calcObstructionRadiiFromObjects(protoObstructionRadiusX, protoObstructionRadiusZ);
   }


   // Broaden the test region to account for any parking lot and flatten regions associated with the building
   float parkingMinX = (pProtoObject) ? pProtoObject->getParkingMinX() : 0.0f;
   float parkingMaxX = (pProtoObject) ? pProtoObject->getParkingMaxX() : 0.0f;
   float parkingMinZ = (pProtoObject) ? pProtoObject->getParkingMinZ() : 0.0f;
   float parkingMaxZ = (pProtoObject) ? pProtoObject->getParkingMaxZ() : 0.0f;

   float lotMinX = -protoObstructionRadiusX;
   float lotMaxX = protoObstructionRadiusX;
   float lotMinZ = -protoObstructionRadiusZ;
   float lotMaxZ = protoObstructionRadiusZ;

   if (parkingMinX < lotMinX)
      lotMinX = parkingMinX;
   if (parkingMaxX > lotMaxX)
      lotMaxX = parkingMaxX;
   if (parkingMinZ < lotMinZ)
      lotMinZ = parkingMinZ;
   if (parkingMaxZ > lotMaxZ)
      lotMaxZ = parkingMaxZ;

   float flattenMinX[2];
   float flattenMaxX[2];
   float flattenMinZ[2];
   float flattenMaxZ[2];
   for (int i=0; i<2; i++)
   {
      flattenMinX[i] = (pProtoObject) ? pProtoObject->getFlattenRegion(i).mMinX : 0.0f;
      flattenMaxX[i] = (pProtoObject) ? pProtoObject->getFlattenRegion(i).mMaxX : 0.0f;
      flattenMinZ[i] = (pProtoObject) ? pProtoObject->getFlattenRegion(i).mMinZ : 0.0f;
      flattenMaxZ[i] = (pProtoObject) ? pProtoObject->getFlattenRegion(i).mMaxZ : 0.0f;

      if (flattenMinX[i] < lotMinX)
         lotMinX = flattenMinX[i];
      if (flattenMaxX[i] > lotMaxX)
         lotMaxX = flattenMaxX[i];
      if (flattenMinZ[i] < lotMinZ)
         lotMinZ = flattenMinZ[i];
      if (flattenMaxZ[i] > lotMaxZ)
         lotMaxZ = flattenMaxZ[i];
   }

   lotMinX -= expansionRadius;
   lotMaxX += expansionRadius;
   lotMinZ -= expansionRadius;
   lotMaxZ += expansionRadius;

   // Get dimensions
   BVector p1, p2, p3, p4;
   if(protoObstructionRadiusX==0.0f)
   {
      p1=p2=p3=p4=location;
   }
   else
   {
      matrix.transformVectorAsPoint(BVector(lotMinX, 0.0f, lotMinZ), p1);
      matrix.transformVectorAsPoint(BVector(lotMaxX, 0.0f, lotMinZ), p2);
      matrix.transformVectorAsPoint(BVector(lotMaxX, 0.0f, lotMaxZ), p3);
      matrix.transformVectorAsPoint(BVector(lotMinX, 0.0f, lotMaxZ), p4);
   }

   // Special placement logic for sockets
   if (pProtoObject)
   {
      int socketID = pProtoObject->getSocketID();
      if (socketID != -1)
      {
         float range=pProtoObject->getObstructionRadius()*4.0f;
         if (range==0.0f)
            range=20.0f;

         BEntityIDArray sockets(0, 16);
         getBuildSockets(playerID, protoID, pBuilder, location, range, false, false, sockets);

         float closestDistScr=cMaximumFloat;
         BEntityID closetSocket=cInvalidObjectID;
         BVector closestPosition = cOriginVector;
         bool closestValid = false;
         uint count = sockets.getSize();
         for (uint i=0; i<count; i++)
         {
//-- FIXING PREFIX BUG ID 5983
            const BUnit* pUnit = getUnit(sockets[i]);
//--
            BVector pos = pUnit->getPosition();
            float distSqr=location.distanceSqr(pos);
            if (distSqr<closestDistScr)
            {
               if (flags & cCPEarlyExit)
                  return true;
               closestDistScr=distSqr;
               closetSocket=pUnit->getID();
               closestPosition=pos;
               closestValid = (pUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug) == NULL);
            }
         }

         bool canPlace=false;
         bool suggested=false;
         if (closetSocket!=cInvalidObjectID)
         {
            if (closestValid)
               canPlace=true;
            if (pSocketID)
               *pSocketID=closetSocket;
            if (flags & cCPSetPlacementSuggestion)
            {
               suggestion = closestPosition;
               suggested=true;
            }
         }

         if (flags & cCPRender)
            drawCheckPlacement(pProtoObject, pPlayer, canPlace, suggested, location, forward, suggestion, protoObstructionRadiusX, protoObstructionRadiusZ, expansionRadius, p1, p2, p3, p4);

         return canPlace;
      }
   }

   // If the "push off terrain" flag is set, prepare to create a list of terrain-only obstructions
   const float terrainPushOffRadius = (flags & cCPPushOffTerrain) ? pPlayer->getCiv()->getTerrainPushOffRadius() : 0.0f;
   BVector pushOffP1, pushOffP2, pushOffP3, pushOffP4;
   float pushOffMinX = 0.0f;
   float pushOffMinZ = 0.0f;
   float pushOffMaxX = 0.0f;
   float pushOffMaxZ = 0.0f;
   if (flags & cCPPushOffTerrain)
   {
      // Get push off terrain dimensions
      pushOffMinX = lotMinX - terrainPushOffRadius + expansionRadius;
      pushOffMaxX = lotMaxX + terrainPushOffRadius - expansionRadius;
      pushOffMinZ = lotMinZ - terrainPushOffRadius + expansionRadius;
      pushOffMaxZ = lotMaxZ + terrainPushOffRadius - expansionRadius;
      matrix.transformVectorAsPoint(BVector(pushOffMinX, 0.0f, pushOffMinZ), pushOffP1);
      matrix.transformVectorAsPoint(BVector(pushOffMaxX, 0.0f, pushOffMinZ), pushOffP2);
      matrix.transformVectorAsPoint(BVector(pushOffMaxX, 0.0f, pushOffMaxZ), pushOffP3);
      matrix.transformVectorAsPoint(BVector(pushOffMinX, 0.0f, pushOffMaxZ), pushOffP4);
   }

   // SLB: TODO - Assuming 90 degree turns only for now...change that later
   const BVector minCorner = __vctsxs(XMVectorFloor(XMVectorMultiply(XMVectorMin(p1, p3), XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale()))), 0);
   const BVector maxCorner = __vctsxs(XMVectorCeiling(XMVectorMultiply(XMVectorMax(p1, p3), XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale()))), 0);
   const long iX1 = minCorner.u[0];
   const long iZ1 = minCorner.u[2];
   const long iX2 = maxCorner.u[0];
   const long iZ2 = maxCorner.u[2];

   bool OOB;
   bool los = true;
   bool obstructions = false;
   bool canPlace = false;

   // Check out of bounds
   long maxTiles = gVisibleMap.getMaxXTiles();
   OOB = ((iX1 < 0) || (iZ1 < 0) || (iX2 >= maxTiles) || (iZ2 >= maxTiles)) ? true : false;
   canPlace = !OOB;
   if (!canPlace && (flags & cCPEarlyExit))
      return false;

   // Check LOS
   if (canPlace && (losMode != cCPLOSDontCare))
   {
      const BPlayer *pPlayer = getPlayer(playerID);
      BASSERT(pPlayer);
      const BTeamID teamID = pPlayer->getTeamID();

      DWORD losMask = ((losMode == cCPLOSNormal) ? 0x01 : 0x10001) << teamID;

      if(pProtoObject && pProtoObject->getFlagBlockLOS())
      {
         // Circular LOS check
         long radius=(iX2-iX1)/2;
         long radiusSqr=radius*radius;
         long centerX=iX1+radius;
         long centerZ=iZ1+((iZ2-iZ1)/2);
         for (long z = iZ1; los && (z <= iZ2); z++)
         {
            long zd=z-centerZ;
            long zdSqr=zd*zd;
            for (long x = iX1; x <= iX2; x++)
            {
               long xd=x-centerX;
               long xdSqr=xd*xd;
               long distSqr=xdSqr+zdSqr;
               if(distSqr>radiusSqr)
                  continue;
               if (((gVisibleMap.getVisibility(x, z) & losMask) != losMask) && (!BTeam::doesTeamHaveSpies(teamID) || (gVisibleMap.getVisibility(x, z) & 0xFFFF0000) == 0))
               {
                  los = false;
                  canPlace = false;
                  break;
               }
            }
         }
      }
      else
      {
         if (flags & cCPLOSCenterOnly)
         {
            // Center LOS check
            long x = (iX1 + iX2) / 2;
            long z = (iZ1 + iZ2) / 2;
            if (((gVisibleMap.getVisibility(x, z) & losMask) != losMask) && (!BTeam::doesTeamHaveSpies(teamID) || (gVisibleMap.getVisibility(x, z) & 0xFFFF0000) == 0))
            {
               canPlace = false;
            }
         }
         else
         {
            // Rectangular LOS check
            for (long z = iZ1; los && (z <= iZ2); z++)
               for (long x = iX1; x <= iX2; x++)
               {
                  if (((gVisibleMap.getVisibility(x, z) & losMask) != losMask) && (!BTeam::doesTeamHaveSpies(teamID) || (gVisibleMap.getVisibility(x, z) & 0xFFFF0000) == 0))
                  {
                     los = false;
                     canPlace = false;
                     break;
                  }
               }
         }
      }
   }
   if (!canPlace && (flags & cCPEarlyExit))
      return false;

   // Fail if terrain is too rough
   if (pProtoObject)
   {
      BVector segs[4];
      segs[0] = p1;
      segs[1] = p2;
      segs[2] = p3;
      segs[3] = p4;
      BConvexHull hull;
      hull.initialize(segs, 4, true);
      //10.0f copied out of the default value in protoobject.cpp.
      float heightTolerance=10.0f;
      if (pProtoObject)
         heightTolerance=pProtoObject->getTerrainHeightTolerance();
      float minHeight = location.y - heightTolerance;
      float maxHeight = location.y + heightTolerance;
      if (!gObsManager.getAllInHeightRange(hull, minHeight, maxHeight))
         canPlace = false;

      if (!canPlace && (flags & cCPEarlyExit))
         return false;
   }

   // Cache stuff for placement suggestion
   int iMaxTiles = 0;
   float fMaxTiles = 0.0f;
   long obstructionQuadTrees =
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit |   // Unit that can't move                                                       
      BObstructionManager::cIsNewTypeBlockLandUnits;              // Terrain that blocks any combo of movement that includes land-based movement
   if (!(flags & cCPIgnoreAirObstructions))   
      obstructionQuadTrees |= BObstructionManager::cIsNewTypeBlockAirMovement;            // Unit that blocks Air movement.
   if (flags & cCPCheckSquadObstructions)
      obstructionQuadTrees |= BObstructionManager::cIsNewTypeCollidableStationarySquad;

   if (!(flags & cCPIgnoreMoving))
   {
      obstructionQuadTrees |= 
      BObstructionManager::cIsNewTypeCollidableMovingUnit |       // Unit that can Move and *IS* currently in motion                            
      BObstructionManager::cIsNewTypeCollidableStationaryUnit;    // Unit that can Move and *IS NOT* currently in motion
      if (flags & cCPCheckSquadObstructions)
         obstructionQuadTrees |= BObstructionManager::cIsNewTypeCollidableMovingSquad;
   }

   // To be used if pushing off terrain...
   long terrainOnlyQuadTrees =
      BObstructionManager::cIsNewTypeBlockLandUnits;              // Terrain that blocks any combo of movement that includes land-based movement

   if (flags & cCPSetPlacementSuggestion)
   {
      mFlagUnitCache        = true;
      mFlagObstructionCache = true;

      if( pProtoObject )
      {
         float span = p1.distance(p3) * 0.3f;
         iMaxTiles = Math::Max( 1, Math::FloatToIntTrunc( span * gTerrainSimRep.getReciprocalDataTileScale() ) );
      }
      else
      {
         iMaxTiles = 1;
      }
      iMaxTiles *= searchScale;
      fMaxTiles =  (float)iMaxTiles;

      float distance    = fMaxTiles + 1.0f + BPlacementRules::sMaxDistance;
      float distanceSqr = distance * distance;

      // obstructions
      if(pProtoObject && pProtoObject->getFlagBlockLOS())
      {
         // FIXME TBD AJL 8/30/06 - Don't allow blockers to be placed on top of enemy units
      }
      else if(!pProtoObject || pProtoObject->getFlagCollidable())
      {
         float searchDistance = 3.0f * (fMaxTiles + 1.0f);
         const float buildingMagnetRange = (pPlayer) ? pPlayer->getCiv()->getBuildingMagnetRange() : 0.0f;
         searchDistance = Math::Max(searchDistance, buildingMagnetRange);

         float tx1 = p1.x - searchDistance;
         float tz1 = p1.z - searchDistance;
         float tx2 = p3.x + searchDistance;
         float tz2 = p3.z + searchDistance;

         gObsManager.findObstructions(tx1, tz1, tx2, tz2, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, playerID, false, mCachedObstructions);
         // Expand building obstructions to account for any parking lot and flatten regions
         int numObstructions = mCachedObstructions.getNumber();
         int numBuildingObs = 0;
         for (int i=0; i<numObstructions; i++)
         {
//-- FIXING PREFIX BUG ID 5984
            const BUnit* pUnit = reinterpret_cast<BUnit*> (gObsManager.getObject(mCachedObstructions.get(i)));
//--
            if (pUnit)
            {
               // Broaden the test region to account for any parking lot and flatten regions associated with the building
               BOPObstructionNode * theNode = mCachedObstructions.get(i);
               mExpandedObstructionData.add(*theNode);
               numBuildingObs++;

               // MPB [4/1/2008] - Disabling this for now as we aren't using it, plus it only works with axis aligned obstructions
               // which are not guaranteed.  If we need this functionality back, it will need to be rewritten to work with
               // arbitrarily oriented obstructions.
               /*
               if (!(flags & cCPIgnoreParkingAndFlatten))
               {
                  BVector buildingCenter = pUnit->getPosition();
                  const BProtoObject *pProtoObj = pUnit->getProtoObject();
                  const float protoObstructionRadiusX = (pProtoObj) ? pProtoObj->getObstructionRadiusX() : 0.0f;
                  const float protoObstructionRadiusZ = (pProtoObj) ? pProtoObj->getObstructionRadiusZ() : 0.0f;

                  // Broaden the test region to account for any parking lot and flatten regions associated with the building
                  float parkingMinX = (pProtoObj) ? pProtoObj->getParkingMinX() : 0.0f;
                  float parkingMaxX = (pProtoObj) ? pProtoObj->getParkingMaxX() : 0.0f;
                  float parkingMinZ = (pProtoObj) ? pProtoObj->getParkingMinZ() : 0.0f;
                  float parkingMaxZ = (pProtoObj) ? pProtoObj->getParkingMaxZ() : 0.0f;

                  float obsLotMinX = -protoObstructionRadiusX;
                  float obsLotMaxX = protoObstructionRadiusX;
                  float obsLotMinZ = -protoObstructionRadiusZ;
                  float obsLotMaxZ = protoObstructionRadiusZ;

                  if (parkingMinX < obsLotMinX)
                     obsLotMinX = parkingMinX;
                  if (parkingMaxX > obsLotMaxX)
                     obsLotMaxX = parkingMaxX;
                  if (parkingMinZ < obsLotMinZ)
                     obsLotMinZ = parkingMinZ;
                  if (parkingMaxZ > obsLotMaxZ)
                     obsLotMaxZ = parkingMaxZ;

                  float flattenMinX[2];
                  float flattenMaxX[2];
                  float flattenMinZ[2];
                  float flattenMaxZ[2];
                  for (int j=0; j<2; j++)
                  {
                     flattenMinX[j] = (pProtoObj) ? pProtoObj->getFlattenRegion(j).mMinX : 0.0f;
                     flattenMaxX[j] = (pProtoObj) ? pProtoObj->getFlattenRegion(j).mMaxX : 0.0f;
                     flattenMinZ[j] = (pProtoObj) ? pProtoObj->getFlattenRegion(j).mMinZ : 0.0f;
                     flattenMaxZ[j] = (pProtoObj) ? pProtoObj->getFlattenRegion(j).mMaxZ : 0.0f;

                     if (flattenMinX[j] < obsLotMinX)
                        obsLotMinX = flattenMinX[j];
                     if (flattenMaxX[j] > obsLotMaxX)
                        obsLotMaxX = flattenMaxX[j];
                     if (flattenMinZ[j] < obsLotMinZ)
                        obsLotMinZ = flattenMinZ[j];
                     if (flattenMaxZ[j] > obsLotMaxZ)
                        obsLotMaxZ = flattenMaxZ[j];
                  }

                  BOPObstructionNode * expandedNode = &mExpandedObstructionData.get(numBuildingObs-1);

                  expandedNode->mX1 = buildingCenter.x + obsLotMinX;
                  expandedNode->mZ1 = buildingCenter.z + obsLotMinZ;
                  expandedNode->mX2 = buildingCenter.x + obsLotMinX;
                  expandedNode->mZ2 = buildingCenter.z + obsLotMaxZ;
                  expandedNode->mX3 = buildingCenter.x + obsLotMaxX;
                  expandedNode->mZ3 = buildingCenter.z + obsLotMaxZ;
                  expandedNode->mX4 = buildingCenter.x + obsLotMaxX;
                  expandedNode->mZ4 = buildingCenter.z + obsLotMinZ;

                  expandedNode->mIdxMinX = 0;
                  expandedNode->mIdxMaxX = 4;
                  expandedNode->mIdxMinZ = 1;
                  expandedNode->mIdxMaxZ = 5;

                  // Draw this expanded box for debugging cues
                  if (gConfig.isDefined(cConfigRenderSquadPlotter))
                  {
                     BVector pts[4];
                     pts[0].x = buildingCenter.x + obsLotMinX; pts[0].z = buildingCenter.z + obsLotMinZ; pts[0].y = buildingCenter.y;
                     pts[1].x = buildingCenter.x + obsLotMinX; pts[1].z = buildingCenter.z + obsLotMaxZ; pts[1].y = buildingCenter.y;
                     pts[2].x = buildingCenter.x + obsLotMaxX; pts[2].z = buildingCenter.z + obsLotMaxZ; pts[2].y = buildingCenter.y;
                     pts[3].x = buildingCenter.x + obsLotMaxX; pts[3].z = buildingCenter.z + obsLotMinZ; pts[3].y = buildingCenter.y;
                     gTerrainSimRep.addDebugSquareOverTerrain(pts[0], pts[1], pts[2], pts[3], cDWORDDarkOrange, 0.5f);
                  }
               }
               */
            }
         }

         for (int i=0; i<numBuildingObs; i++)
         {
            mExpandedObstructions.add(&mExpandedObstructionData.get(i));
         }

         // Find terrain-only obstructions if pushing off terrain
         if (flags & cCPPushOffTerrain)
            gObsManager.findObstructions(tx1, tz1, tx2, tz2, 0.0f, terrainOnlyQuadTrees, BObstructionManager::cObsNodeTypeAll, playerID, false, mCachedTerrainObstructions);
      }

      // placement rules
      BEntityIDArray &unitsToTrack = BPlacementRules::getUnitsToTrack();

      long numResults = unitsToTrack.getNumber();
      for (long i = 0; i < numResults; i++)
      {
         BUnit *pUnit = gWorld->getUnit(unitsToTrack[i]);
         if (!pUnit)
            continue;

         if (pUnit->calculateXZDistanceSqr(location) <= distanceSqr)
            mCachedUnitsToTrack.add(unitsToTrack[i]);
      }
   }

   // Check for obstructions
   if (canPlace && (flags & cCPCheckObstructions))
   {
      if(pProtoObject && pProtoObject->getFlagBlockLOS())
      {
         // FIXME TBD AJL 8/30/06 - Don't allow blockers to be placed on top of enemy units
      }
      else if(!pProtoObject || pProtoObject->getFlagCollidable())
      {
         if (mFlagObstructionCache)
         {
            obstructions = gObsManager.testObstructionsInCachedList(p1.x, p1.z, p3.x, p3.z, 0.0f, mExpandedObstructions);
            // If there were no obstructions and we're pushing off of terrain, check against the terrain obstruction at the appropriate range
            if (!obstructions && (flags & cCPPushOffTerrain))
               obstructions = gObsManager.testObstructionsInCachedList(pushOffP1.x, pushOffP1.z, pushOffP3.x, pushOffP3.z, 0.0f, mCachedTerrainObstructions);
         }
         else
            obstructions = gObsManager.testObstructions(p1.x, p1.z, p3.x, p3.z, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, playerID);
         canPlace = !obstructions;

         // Check placement rules
         if (!(flags & cCPIgnorePlacementRules))
         {
//-- FIXING PREFIX BUG ID 5985
            const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
            if (placementRules)
            {
               bool followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
               canPlace &= followsRules;
            }
         }
      }
   }
   if (!canPlace && (flags & cCPEarlyExit))
      return false;

   // Check if specific placement rule designated
   if (!(flags & cCPIgnorePlacementRules) && (placementRuleIndex != -1))
   {
//-- FIXING PREFIX BUG ID 5986
      const BPlacementRules* placementRules = gDatabase.getPlacementRules(placementRuleIndex);
//--
      if (placementRules)
      {
         bool followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
         canPlace &= followsRules;
      }
   }
   if (!canPlace && (flags & cCPEarlyExit))
      return (false);

   // Placement suggestion
   bool suggested = false;
   if (flags & cCPSetPlacementSuggestion)
   {
      // If buildingMagnetRange is greater than zero, check next to closest building that the player owns if it is within buildingMagnetRange.
      const float buildingMagnetRange = (pPlayer) ? pPlayer->getCiv()->getBuildingMagnetRange() : 0.0f;
      BVector magnetLocation(0.0f, 0.0f, 0.0f);
      if (buildingMagnetRange > 0.0f)
      {
         // From the cached list, find closest, in-range building that the player owns.
         BUnit* pCheckUnit = NULL;
         BUnit* pCloseUnit = NULL;
         float  slideRangeX = buildingMagnetRange + 1.0f;
         float  slideRangeZ = buildingMagnetRange + 1.0f;
         float  minRange = buildingMagnetRange + 1.0f;

         bool   bSlideInXAxis = false;
         uint8 numObstructions = mExpandedObstructions.getNumber();
         

         for (int i=0; i<numObstructions; i++)
         {
            // Get the Obstruction to process
            if (mExpandedObstructions.get(i) && gObsManager.getObject(mExpandedObstructions.get(i)))
            {
               pCheckUnit = reinterpret_cast<BUnit*> (gObsManager.getObject(mExpandedObstructions.get(i)));

               if (pCheckUnit && (pCheckUnit->getPlayerID() == playerID)) // Valid? Same owner?
               {
                  if (pCheckUnit->getProtoObject() && (pCheckUnit->getProtoObject()->getObjectClass() == cObjectClassBuilding)) // Building?
                  {
                     // Look for an overlap in either X or Z positioning and in-range in the other dimension.
                     // We want to help the player snap objects into place without overriding him unnecessarily.
                     const BVector checkPos = pCheckUnit->getPosition();
                     BOPObstructionNode * expandedNode = mExpandedObstructions.get(i);
                     float  edgeRangeX;
                     float  edgeRangeZ;
                     if (checkPos.x >= location.x)
                        edgeRangeX = expandedNode->mX1 - (location.x + lotMaxX);
                     else
                        edgeRangeX = (location.x + lotMinX) - expandedNode->mX3;

                     if (checkPos.z >= location.z)
                        edgeRangeZ = expandedNode->mZ1 - (location.z + lotMaxZ);
                     else
                        edgeRangeZ = (location.z + lotMinZ) - expandedNode->mZ3;

                     float absEdgeRangeX = fabs(edgeRangeX);
                     float absEdgeRangeZ = fabs(edgeRangeZ);
                     if (edgeRangeX < 0.0f && (edgeRangeX < edgeRangeZ)) // Overlap in X
                     {
                        if ((absEdgeRangeZ < buildingMagnetRange) && (absEdgeRangeZ < minRange))
                        {
                           // Slide in Z axis
                           slideRangeZ = edgeRangeZ;
                           minRange = absEdgeRangeZ;
                           pCloseUnit = pCheckUnit;
                           bSlideInXAxis = false;
                        }
                     }
                     else if (edgeRangeZ < 0.0f) // Overlap in Z
                     {
                        if ((absEdgeRangeX < buildingMagnetRange) && (absEdgeRangeX < minRange))
                        {
                           // Slide in X axis
                           slideRangeX = edgeRangeX;
                           minRange = absEdgeRangeX;
                           pCloseUnit = pCheckUnit;
                           bSlideInXAxis = true;
                        }
                     }
                  }
               }
            }
         }

         // If minRange < buildingMagnetRange, we have a close Unit to stick to.
         // Slide in the appropriate axis by the edge distance and check for obstructions.
         if (minRange < buildingMagnetRange)
         {
            const BVector unitPos = pCloseUnit->getPosition();
            BVector slide(0.0f, 0.0f, 0.0f);

            float unitDist = 0.0f;
            if (bSlideInXAxis)
            {
               unitDist = unitPos.x - location.x;
               slide.x = slideRangeX - 0.01f;  // Minus small buffer to prevent overlap
               // Ensure offset is in the correct direction
               if (unitDist < 0.0f)
                  slide.x *= -1.0f;
            }
            else
            {
               unitDist = unitPos.z - location.z;
               slide.z = slideRangeZ - 0.01f;  // Minus small buffer to prevent overlap
               // Ensure offset is in the correct direction
               if (unitDist < 0.0f)
                  slide.z *= -1.0f;
            }
            if (checkPlacement(protoID, playerID, location + slide, suggestion, forward, losMode, (flags & ~(cCPSetPlacementSuggestion | cCPRender) | cCPEarlyExit), 1, pBuilder,
                               pSocketID, placementRuleIndex, protoSquadID, useSquadObsRadii))
            {
               suggested = true;
               canPlace = true;
               magnetLocation = location + slide;

               // Check placement rules
               if (!(flags & cCPIgnorePlacementRules))
               {
//-- FIXING PREFIX BUG ID 5988
                  const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
                  if (placementRules)
                  {
                     bool followsRules = placementRules->evaluate(pBuilder, protoID, playerID, magnetLocation, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
                     canPlace &= followsRules;
                     suggested &= followsRules;
                  }
               }
            }
         }
      }

      if (suggested)
         suggestion = magnetLocation;
      else if (canPlace)
         suggestion = location;
      else
      {
         const float tileScale = gTerrainSimRep.getDataTileScale();
         BVector minusOne = XMVectorReplicate(-1.0f);
         const float maxDistance = fMaxTiles * tileScale;
         if (XMVector3Equal(suggestion, minusOne) || (suggestion.distanceSqr(location) > (maxDistance * maxDistance)))
         {
            suggestion = minusOne;
            bool success = false;

            const BVector up(0.0f, 0.0f, tileScale);
            const BVector down(0.0f, 0.0f, -tileScale);
            const BVector left(-tileScale, 0.0f, 0.0f);
            const BVector right(tileScale, 0.0f, 0.0f);
            for (int level = 1; level <= iMaxTiles; level++)
            {
               int hSpan = level * 2 + 1;
               int vSpan = hSpan - 2;

               BVector bestLocation, validLocation;

               // Check top
               BVector temp = (up + left) * level;
               for (int i = 0; i < hSpan; i++)
               {
                  if (checkPlacement(protoID, playerID, location + temp, suggestion, forward, losMode, (flags & ~(cCPSetPlacementSuggestion | cCPRender) | cCPEarlyExit), 1, pBuilder, 
                                     pSocketID, placementRuleIndex, protoSquadID, useSquadObsRadii))
                  {
                     // Check placement rules
                     bool followsRules = true;
                     if (!(flags & cCPIgnorePlacementRules))
                     {
//-- FIXING PREFIX BUG ID 5989
                        const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
                        if (placementRules)
                        {
                           followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location + temp, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
                        }
                     }

                     if (followsRules)
                     {
                        validLocation = location + temp;
                        if (!success || (validLocation.distanceSqr(location) < bestLocation.distanceSqr(location)))
                           bestLocation = validLocation;
                        success = true;
                     }
                  }
                  temp += right;
               }

               // Check bottom
               temp = (down + left) * level;
               for (int i = 0; i < hSpan; i++)
               {
                  if (checkPlacement(protoID, playerID, location + temp, suggestion, forward, losMode, (flags & ~(cCPSetPlacementSuggestion | cCPRender) | cCPEarlyExit), 1, pBuilder,
                                     pSocketID, placementRuleIndex, protoSquadID, useSquadObsRadii))
                  {
                     // Check placement rules
                     bool followsRules = true;
                     if (!(flags & cCPIgnorePlacementRules))
                     {
//-- FIXING PREFIX BUG ID 5990
                        const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
                        if (placementRules)
                        {
                           followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location + temp, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
                        }
                     }

                     if (followsRules)
                     {
                        validLocation = location + temp;
                        if (!success || (validLocation.distanceSqr(location) < bestLocation.distanceSqr(location)))
                           bestLocation = validLocation;
                        success = true;
                     }
                  }
                  temp += right;
               }

               // Check left
               temp = left * level + up * (level - 1);
               for (int i = 0; i < vSpan; i++)
               {
                  if (checkPlacement(protoID, playerID, location + temp, suggestion, forward, losMode, (flags & ~(cCPSetPlacementSuggestion | cCPRender) | cCPEarlyExit), 1, pBuilder,
                                     pSocketID, placementRuleIndex, protoSquadID, useSquadObsRadii))
                  {
                     // Check placement rules
                     bool followsRules = true;
                     if (!(flags & cCPIgnorePlacementRules))
                     {
//-- FIXING PREFIX BUG ID 5991
                        const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
                        if (placementRules)
                        {
                           followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location + temp, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
                        }
                     }

                     if (followsRules)
                     {
                        validLocation = location + temp;
                        if (!success || (validLocation.distanceSqr(location) < bestLocation.distanceSqr(location)))
                           bestLocation = validLocation;
                        success = true;
                     }
                  }
                  temp += down;
               }

               // Check right
               temp = right * level + up * (level - 1);
               for (int i = 0; i < vSpan; i++)
               {
                  if (checkPlacement(protoID, playerID, location + temp, suggestion, forward, losMode, (flags & ~(cCPSetPlacementSuggestion | cCPRender) | cCPEarlyExit), 1, pBuilder,
                                     pSocketID, placementRuleIndex, protoSquadID, useSquadObsRadii))
                  {
                     // Check placement rules
                     bool followsRules = true;
                     if (!(flags & cCPIgnorePlacementRules))
                     {
//-- FIXING PREFIX BUG ID 5992
                        const BPlacementRules* placementRules = (pProtoObject) ? gDatabase.getPlacementRules(pProtoObject->getPlacementRuleIndex()) : gDatabase.getPlacementRules(placementRuleIndex);
//--
                        if (placementRules)
                        {
                           followsRules = placementRules->evaluate(pBuilder, protoID, playerID, location + temp, forward, dirRight, losMode, (flags & cCPRender) ? BPlacementRule::cRenderFlag : 0, NULL, NULL, mFlagUnitCache ? &mCachedUnitsToTrack : NULL);
                        }
                     }

                     if (followsRules)
                     {
                        validLocation = location + temp;
                        if (!success || (validLocation.distanceSqr(location) < bestLocation.distanceSqr(location)))
                           bestLocation = validLocation;
                        success = true;
                     }
                  }
                  temp += down;
               }

               if (success)
               {
                  suggestion = bestLocation;
                  suggested = true;

                  // Good suggested postion found so stop search
                  break;
               }
            }
         }
         else
            suggested = true;

         if (suggested)
            canPlace = true;
      }

      // Empty cache
      mCachedUnitsToTrack.setNumber(0);
      mCachedObstructions.setNumber(0);
      mExpandedObstructions.setNumber(0);
      mExpandedObstructionData.setNumber(0);
      mFlagUnitCache = false;
      mFlagObstructionCache = false;
   }

   if (!canPlace && (flags & cCPEarlyExit))
      return false;

   // Draw outline
   if (flags & cCPRender)
      drawCheckPlacement(pProtoObject, pPlayer, canPlace, suggested, location, forward, suggestion, protoObstructionRadiusX, protoObstructionRadiusZ, expansionRadius, p1, p2, p3, p4);

   // Draw this expanded box for debugging cues
   if (gConfig.isDefined(cConfigRenderSquadPlotter))
      gTerrainSimRep.addDebugSquareOverTerrain(p1, p2, p3, p4, cDWORDPurple, 0.5f);

   return canPlace;
}

//==============================================================================
// BWorld::drawCheckPlacement
//==============================================================================
void BWorld::drawCheckPlacement(const BProtoObject* pProtoObject, const BPlayer* pPlayer, bool canPlace, bool suggested, BVector location, BVector forward, BVector suggestion, float protoObstructionRadiusX, float protoObstructionRadiusZ, float expansionRadius, BVector p1, BVector p2, BVector p3, BVector p4)
{
   float thickness = 0.1f;
   if(pProtoObject && pProtoObject->getFlagBlockLOS())
   {
      if (suggested)
      {
         gTerrainSimRep.addDebugThickCircleOverTerrain(suggestion, protoObstructionRadiusX, thickness, canPlace ? cDWORDGreen : cDWORDRed, 0.1f);
         gTerrainSimRep.addDebugThickCircleOverTerrain(location, protoObstructionRadiusX, thickness, cDWORDWhite, 0.1f);
      }
      else
         gTerrainSimRep.addDebugThickCircleOverTerrain(location, protoObstructionRadiusX, thickness, canPlace ? cDWORDGreen : cDWORDRed, 0.1f);
   }
   else
   {         
      if (gConfig.isDefined(cConfigFlashGameUI))
      {
         float sizeX = protoObstructionRadiusX+expansionRadius;
         float yOffset = 0.0f;
         float sizeZ = protoObstructionRadiusZ+expansionRadius;
         BVector offset = XMVectorZero();
         float intensity = 3.0f;

         if (pProtoObject)
         {
            int decalID = -1;
            const BFlashProtoIcon* pProtoIcon = NULL;
            const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pProtoObject->getProtoSquadID());
            if (pProtoSquad)
            {
               pProtoIcon = gUIManager->getDecalProtoIcon(gUIManager->getDecalProtoIconIndex(pProtoSquad->getBaseType()));
               decalID = pProtoSquad->getSelectionDecalID();
            }
            else
            {
               pProtoIcon = gUIManager->getDecalProtoIcon(gUIManager->getDecalProtoIconIndex(pProtoObject->getBaseType()));
               decalID = pProtoObject->getSelectionDecalID();
            }
            if (pProtoIcon)
            {
               sizeX   = XMConvertHalfToFloat(pProtoIcon->mSize.x);               
               sizeZ   = XMConvertHalfToFloat(pProtoIcon->mSize.z);
               yOffset = XMConvertHalfToFloat(pProtoIcon->mSize.y);
               offset  = XMLoadHalf4(&pProtoIcon->mOffset);
               intensity = XMConvertHalfToFloat(pProtoIcon->mIntensity.x);
            }
            if (suggested)
            {
               drawBuildingPlacementDecal(decalID, suggestion+offset, forward, sizeX, sizeZ, canPlace ? cDWORDGreen : cDWORDRed, intensity);
            }
            else
            {
               drawBuildingPlacementDecal(decalID, location+offset, forward, sizeX, sizeZ, canPlace ? cDWORDGreen : cDWORDRed, intensity);
            }
         }
      }
      else 
      {
         if (suggested)
         {
            BVector diff = suggestion - location;
            BVector suggestedP1 = p1 + diff;
            BVector suggestedP2 = p2 + diff;
            BVector suggestedP3 = p3 + diff;
            BVector suggestedP4 = p4 + diff;
            gTerrainSimRep.addDebugThickSquareOverTerrain(suggestedP1, suggestedP2, suggestedP3, suggestedP4, thickness, canPlace ? cDWORDGreen : cDWORDRed, 0.1f);
            gTerrainSimRep.addDebugThickSquareOverTerrain(p1, p2, p3, p4, thickness, cDWORDWhite, 0.1f);
         }
         else
            gTerrainSimRep.addDebugThickSquareOverTerrain(p1, p2, p3, p4, thickness, canPlace ? cDWORDGreen : cDWORDRed, 0.01f);
      }
   }
}

//==============================================================================
// BWorld::drawBuildingPlacementDecal
//==============================================================================
void BWorld::drawBuildingPlacementDecal(int decalID, BVector pos, BVector forward, float sizeX, float sizeY, DWORD color, float intensity)
{
   if (!gUIManager)
      return;

   if (mBuildingPlacementDecal == -1)
      return;

   if (decalID == -1)
      return;

   int flashMovieIndex = gUIManager->getDecalFlashMovieIndex(decalID);
   if (flashMovieIndex == -1)
      return;

   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mBuildingPlacementDecal);
   if (!pDecalAttributes)
      return;

   // multiple children use the squads position   
   pDecalAttributes->setForward(BVec3(forward.x,forward.y,forward.z));      
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);//BDecalAttribs::cBlendOpaque);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(intensity);
   pDecalAttributes->setRenderOneFrame(true);   
   pDecalAttributes->setColor(color);   
   pDecalAttributes->setSizeX(sizeX);
   pDecalAttributes->setSizeZ(sizeY);
   pDecalAttributes->setYOffset(0.0f);         
   pDecalAttributes->setFlashMovieIndex(flashMovieIndex);
   pDecalAttributes->setConformToTerrain(true);      
   pDecalAttributes->setPos(BVec3(pos.x, pos.y, pos.z));
}

//==============================================================================
// Returns false for normal building placement or true if socket placement is required.
//
// Parameters:
//   playerID        - Player doing the building
//   protoObjectID   - Proto object of building to build
//   pBuilder        - The builder unit (such as the power node)
//   location        - The reference location if desired 
//   range           - The range to check if desired
//   onlyAvailable   - True to only return available sockets
//   ignoreLocation  - True to ignore the location and range parameters
//   sockets         - Array that gets filled in with the socket unit ID’s
//==============================================================================
bool BWorld::getBuildSockets(BPlayerID playerID, int protoObjectID, const BUnit* pBuilder, BVector location, float range, bool onlyAvailable, bool ignoreLocation, BEntityIDArray& sockets)
{
   const BPlayer* pPlayer=getPlayer(playerID);
   const BProtoObject* pProtoObject=pPlayer->getProtoObject(protoObjectID);

   if (!pProtoObject)
      return false;

   int socketID = pProtoObject->getSocketID();
   if (socketID == -1)
      return false;

   if(!pBuilder)
      return false;

   sockets.clear();

   bool canBuild = false;
   if (playerID == pBuilder->getPlayerID() )
      canBuild = true;
   else if (playerID == pBuilder->getPlayer()->getCoopID() )
      canBuild = true;
   else if (pBuilder->getProtoObject()->getFlagCommandableByAnyPlayer())
      canBuild = true;

   if (!canBuild)
      return true;

   if (!ignoreLocation && range == 0.0f)
   {
      range=pProtoObject->getObstructionRadius()*4.0f;
      if (range==0.0f)
         range=20.0f;
   }

   if (pBuilder->isType(socketID))
   {
      if (!onlyAvailable || pBuilder->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug)== NULL)
         sockets.add(pBuilder->getID());
      return true;
   }
   else
   {
      // Look for sockets linked to this building that this object can be built on.
      float rangeSqr=(ignoreLocation ? 0.0f : range*range);
      uint numRefs=pBuilder->getNumberEntityRefs();
      for (uint i=0; i<numRefs; i++)
      {
         const BEntityRef* pRef=pBuilder->getEntityRefByIndex(i);
         if (pRef->mType==BEntityRef::cTypeAssociatedSocket)
         {
            const BUnit* pUnit = getUnit(pRef->mID);
            if (pUnit)
            {
               if (!pUnit->isType(socketID))
                  continue;
               if (onlyAvailable && pUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug)!= NULL)
                  continue;
               if (!ignoreLocation && pUnit->getPosition().distanceSqr(location)>rangeSqr)
                  continue;
               sockets.add(pRef->mID);
            }
         }
      }
   }

   return (sockets.getNumber() > 0);
}

//==============================================================================
// BWorld::getChildObjects
//==============================================================================
bool BWorld::getChildObjects(BPlayerID playerID, int protoObjectID, BVector baseLocation, BVector baseDirection, BSmallDynamicSimArray<BChildObjectData>& childObjects)
{
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return false;

   const BProtoObject* pProtoObject = pPlayer->getProtoObject(protoObjectID);
   if (!pProtoObject)
      return false;

   BVector baseRight;
   baseRight.assignCrossProduct(cYAxisVector, baseDirection);
   baseRight.normalize();

   BMatrix posMat;
   posMat.makeOrient(baseDirection, cYAxisVector, baseRight);
   posMat.setTranslation(baseLocation);

   uint count = pProtoObject->getNumberChildObjects();

   childObjects.setNumber(count);

   for (uint i=0; i<count; i++)
   {
      BChildObjectData& data = childObjects[i];

      data.mChildType = pProtoObject->getChildObjectType(i);
      data.mObjectType = pProtoObject->getChildObjectID(i);
      data.mUserCiv = pProtoObject->getChildObjectUserCiv(i);
      data.mRotation = pProtoObject->getChildObjectRotation(i);
      data.mAttachBoneName = pProtoObject->getChildObjectAttachBoneName(i);

      // Calculate the position and adjust for terrain height.
      posMat.transformVectorAsPoint(pProtoObject->getChildObjectOffset(i), data.mPosition);
      BVector origin(data.mPosition.x, data.mPosition.y+1000.0f, data.mPosition.z);
      gTerrainSimRep.getHeightRaycast(origin, data.mPosition.y, true);
   }

   return true;
}

//==============================================================================
// BWorld::getProjectileDeviation
//==============================================================================
BVector BWorld::getProjectileDeviation(BVector source, BVector target, BVector targetingLead, float accuracy, float maxRange, float maxDeviation, float accuracyDistanceFactor, float accuracyDeviationFactor) const
{
   float accuracyRoll = getRandDistribution(cSimRand);

   // inaccurate
   if (accuracyRoll > accuracy)
   {
      float deviationRoll = getRandDistribution(cSimRand);
      BVector straightTrajectory = XMVectorSubtract(target + targetingLead, source);
      float range = straightTrajectory.length();
      float deviationFactor;
      float deviation;

      // Adjust deviation based on deviation model
      if (deviationRoll <= accuracyDistanceFactor)
         deviationFactor = accuracyDeviationFactor * deviationRoll / accuracyDistanceFactor;
      else
         deviationFactor = LERP(accuracyDeviationFactor, 1.0f, (deviationRoll - accuracyDistanceFactor) / (1.0f - accuracyDistanceFactor));

      // Get deviation at max range
      deviation = deviationFactor * maxDeviation;

      // Get deviation at target range
      deviation *= range / maxRange;

      // Get deviation unit vector on deviation plane
      XMVECTOR deviationVector;
      straightTrajectory /= range;
      if (XMVector3Equal(straightTrajectory, cYAxisVector))
         deviationVector = XMVector3Cross(straightTrajectory, cXAxisVector);
      else
         deviationVector = XMVector3Cross(straightTrajectory, cYAxisVector);

      // Apply an arbitrary rotation to the deviation unit vector along the straight trajectory axis
      XMMATRIX rotationMatrix = XMMatrixRotationAxis(straightTrajectory, getRandDistribution(cSimRand) * XM_2PI);
      deviationVector = XMVector3TransformNormal(deviationVector, rotationMatrix);

      // Scale and return the deviation vector
      return XMVectorScale(deviationVector, deviation);
   }

   return XMVectorZero();
}

//==============================================================================
// BWorld::calculateProjectileLaunchDirection
//==============================================================================
void BWorld::calculateProjectileLaunchDirection(const float maxRange, const BUnit* pAttacker, const BUnit *pTarget, const BProtoObject *pProjectileProtoObject, BVector launchLocation, BVector targetLocation, BVector &launchDirection, bool &useGravity, float &gravity) const
{
   const bool isWeaponAffectedByGravity = pProjectileProtoObject->getFlagIsAffectedByGravity();
   float H = 0.0f;
   if(pProjectileProtoObject)
      H = pProjectileProtoObject->getMaxProjectileHeight();

   // Calculate initial velocity
   if (isWeaponAffectedByGravity && (H > 0.0f) && (maxRange > 0.0f))
   {
      // Slant range
      BVector RS;
      RS.assignDifference(targetLocation, launchLocation);

      // Horizontal range
      BVector RH(RS);
      RH.y = 0.0f;

      const float d = RS.length(); // Total distance
      const float v = pProjectileProtoObject->getDesiredVelocity(); 

      // Horizontal distance
      const BVector attacker = pAttacker ? pAttacker->getPosition() : launchLocation;
      const BVector target = pTarget ? pTarget->getPosition() : targetLocation;
      BVector horizontalOffsetVector;
      horizontalOffsetVector.assignDifference(target, attacker);
      horizontalOffsetVector.y = 0.0f;
      const float horizontalDistance = horizontalOffsetVector.length();
      horizontalOffsetVector.normalize();

      // Horizontal launch offset
      BVector horizontalLaunchOffsetVector;
      horizontalLaunchOffsetVector.assignDifference(launchLocation, attacker);
      horizontalLaunchOffsetVector.y = 0.0f;
      const float horizontalLaunchOffset = horizontalLaunchOffsetVector.dot(horizontalOffsetVector);

      // Horizontal target radius
      float horizontalTargetRadius = 0.0f;
      if (pTarget)
      {
         BVector horizontalTargetHitOffset(RS);
         horizontalTargetHitOffset *= -pTarget->getObstructionRadius() / d;
         horizontalTargetHitOffset.y = 0.0f;
         horizontalTargetRadius = horizontalTargetHitOffset.length();
      }

      const float range = Math::Max(horizontalDistance - horizontalTargetRadius - horizontalLaunchOffset, 0.0f);
      const float T = d / v;
      const float Th = 0.5f * T;
      const float heightScale = Math::Min(((range * range) / (maxRange * maxRange)), 1.0f);
      const float scaledHeight = H * heightScale;
      const float A = -2.0f * scaledHeight / (Th * Th);
      const float Vy = (scaledHeight / Th) - (0.5f * A * Th);

      launchDirection = RS;
      launchDirection *= v / d;
      launchDirection.y += Vy;

      useGravity = true;
      gravity = -A;

      //////////////////////////////////////////////////////////////////////////
      // SLB: Debugging code
      //BVector line = RS;
      //line *= range / d;
      //gpDebugPrimitives->addDebugSphere(attacker, 1.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone, 1.0f);
      //gpDebugPrimitives->addDebugSphere(target, 1.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone, 1.0f);
      //gpDebugPrimitives->addDebugSphere(attacker, maxRange, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
      //gpDebugPrimitives->addDebugLine(launchLocation, launchLocation + line, cDWORDYellow, cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
      //if (pTarget)
      //   gpDebugPrimitives->addDebugSphere(target, pTarget->getObstructionRadius(), cDWORDYellow, BDebugPrimitives::cCategoryNone, 1.0f);
      //line *= 0.5f;
      //BVector lineMaxHeight = line;
      //BVector lineDampenedMaxHeight = line;
      //lineMaxHeight.y += H;
      //lineDampenedMaxHeight.y += H * heightScale;
      //gpDebugPrimitives->addDebugSphere(launchLocation + lineMaxHeight, 1.0f, cDWORDRed, BDebugPrimitives::cCategoryNone, 1.0f);
      //gpDebugPrimitives->addDebugSphere(launchLocation + lineDampenedMaxHeight, 0.5f, cDWORDBlue, BDebugPrimitives::cCategoryNone, 1.0f);

      //float t = T * 0.25f;
      //BVector g(0.0f, A, 0.0f);
      //BVector p = launchLocation + (launchDirection * t) + (g * (0.5f * t * t));
      //gpDebugPrimitives->addDebugSphere(p, 0.5f, cDWORDPurple, BDebugPrimitives::cCategoryNone, 1.0f);
      //t = T * 0.5f;
      //p = launchLocation + (launchDirection * t) + (g * (0.5f * t * t));
      //gpDebugPrimitives->addDebugSphere(p, 0.5f, cDWORDPurple, BDebugPrimitives::cCategoryNone, 1.0f);
      //t = T * 0.75f;
      //p = launchLocation + (launchDirection * t) + (g * (0.5f * t * t));
      //gpDebugPrimitives->addDebugSphere(p, 0.5f, cDWORDPurple, BDebugPrimitives::cCategoryNone, 1.0f);
      //t = T;
      //p = launchLocation + (launchDirection * t) + (g * (0.5f * t * t));
      //gpDebugPrimitives->addDebugSphere(p, 0.5f, cDWORDPurple, BDebugPrimitives::cCategoryNone, 1.0f);
      //////////////////////////////////////////////////////////////////////////

      return;
   }
   else
   {
      launchDirection = XMVector3Normalize(XMVectorSubtract(targetLocation, launchLocation));
      useGravity = false;
      gravity = 0.0f;
      return;
   }
}

//==============================================================================
// BWorld::launchProjectile
//==============================================================================
BEntityID BWorld::launchProjectile( BObjectCreateParms& parms, BVector targetOffset, BVector targetingLead, BVector projectileOrientation, float damage, const BProtoAction* pProtoAction, IDamageInfo* pDamageInfo, BEntityID parentID /*= cInvalidObjectID*/, BUnit* pTargetUnit /*= NULL*/, long hitZoneIndex /*= -1*/, bool doLogging /*= false*/, bool collideWithAllUnits/*=true*/, bool fromLeaderPower/*=false*/ )
{
   /*BMatrix mat;
   mat.makeTranslate(pTarget->getPosition().x, pTarget->getPosition().y, pTarget->getPosition().z);
   gpDebugPrimitives->addDebugLine(pUnit->getPosition(), pTarget->getPosition(), cDWORDYellow, cDWORDRed, 100, 0.5f);
   gpDebugPrimitives->addDebugSphere(mat, pTarget->getSimRadius(), cDWORDRed, 100, 1.0f);*/

   if(parms.mProtoObjectID == -1)
      return cInvalidObjectID;

   //=====================================================================
   // Do some special handling for in case we are shooting up or down.
   //=====================================================================
   // Get the direction our projectile is going to shoot and normalize it.
   BVector hitZonePos;
   BVector targetLocation;
   if( pTargetUnit && pTargetUnit->getHitZonePosition( hitZoneIndex, hitZonePos ) )
      targetLocation = hitZonePos + targetOffset + targetingLead;
   else if( pTargetUnit )
      targetLocation = pTargetUnit->getPosition() + targetOffset + targetingLead;
   else
      targetLocation = targetOffset + targetingLead;

   BProjectile *pProj = createProjectile(parms);
   if (!pProj)
      return cInvalidObjectID;

   // If set to air burst, set detonation on arrival flag.
   if (pProtoAction && pProtoAction->getFlagAirBurst() && pTargetUnit && pTargetUnit->getProtoObject()->getMovementType() == cMovementTypeAir)
   {
      pProj->setFlagAirBurst(true);
      targetLocation.y = pTargetUnit->getPosition().y;
   }
   else
   {
      float targetLocationGroundHeight;
      gTerrainSimRep.getHeightRaycast(targetLocation, targetLocationGroundHeight, true);
      targetLocation.y = Math::Max(targetLocation.y, targetLocationGroundHeight);
   }

#ifndef BUILD_FINAL
   mLaunchedProjectilesPerFrame++;
#endif

   if (pTargetUnit && (hitZoneIndex != -1))
   {
      pProj->setTargetObjectID(pTargetUnit->getID());
      pProj->setTargetOffset(targetOffset);
      pProj->setHitZoneIndex(hitZoneIndex);
   }
   else if (pTargetUnit)
   {
      pProj->setTargetObjectID(pTargetUnit->getID());
      pProj->setTargetOffset(targetOffset);
   }

   pProj->setTargetLocation(targetLocation);
   pProj->setDamage(damage);
   pProj->setProtoAction(pProtoAction);
   pProj->setParentID(parentID);
   pProj->setDamageInfo(pDamageInfo);
   pProj->setGravity(gDatabase.getProjectileGravity());
   pProj->setFlagCollidesWithAllUnits(collideWithAllUnits);
   pProj->setFlagFromLeaderPower(fromLeaderPower);

   // Active Scan
   const BProtoObject* pProtoProj = pProj->getProtoObject();
   if (pProtoProj && (pProtoProj->getActiveScanChance() > 0.0f))
   {
      BUnit* pParentUnit = getUnit(parentID);
      if (pParentUnit)
      {
         pParentUnit->clearInvalidMultiTargets(pTargetUnit);
      }
   }

   // BTK -- turned this off because this caused 13% decrease in perf for all builds.  Stick this on 
   // a config if this is needed for debugging.
   pProj->setFlagDoLogging(false);   

   BVector launchDirection;
   bool useGravity;
   float gravity;
   BUnit* pParentUnit = getUnit(parentID);
   const float maxRange = pProtoAction->getMaxRange(pParentUnit);
   calculateProjectileLaunchDirection(maxRange, pParentUnit, pTargetUnit, pProtoProj, parms.mPosition, pProj->getTargetLocation(), launchDirection, useGravity, gravity);

   // Forward vector of projectiles were sometimes bogus - make sure that doesn't happen (find out why it did)
   float projForwardLength = pProj->getForward().length();
   bool bGoodProjVector = true;
   if ((projForwardLength < 0.99f) || (projForwardLength > 1.01f))
      bGoodProjVector = false;

   BASSERTM((bGoodProjVector || !pProtoProj->getFlagTracking()), "Bogus projectile initial orientation vector!");
   if (pProtoProj->getFlagTracking() && !useGravity && bGoodProjVector)
      launchDirection = pProj->getForward();
   else
   {
      parms.mForward = launchDirection;
      parms.mForward.normalize();
      // Dot it with the Y axis and then self-multiply (both are normalized so value should be 0.0f - 1.0f
      float dotProduct = parms.mForward.dot(cYAxisVector);
      float dotSquared = dotProduct * dotProduct;
      // If our dot*dot + cFloatCompareEpsilon is >= 1.0f, that means we were so close to the y axis we need to use a different axis for "up" (like x)
      BVector upVector = (dotSquared + cFloatCompareEpsilon > 1.0f) ? cXAxisVector : cYAxisVector;
      parms.mRight.assignCrossProduct(upVector, parms.mForward);
      // Override yaw
      if (projectileOrientation.lengthSquared() > cFloatCompareEpsilon)
      {
         upVector.assignCrossProduct(parms.mForward, parms.mRight);
         parms.mRight.assignCrossProduct(upVector, projectileOrientation);
         parms.mForward.assignCrossProduct(parms.mRight, upVector);
         parms.mRight.normalize();
         parms.mForward.normalize();
      }

      pProj->setForward(parms.mForward);
      pProj->setRight(parms.mRight);
      pProj->calcUp();
   }
   pProj->setVelocity(launchDirection);
   pProj->setFlagIsAffectedByGravity(useGravity);
   pProj->setGravity(gravity);

//    if (gravity)
//    {
//       gpDebugPrimitives->addDebugSphere(parms.mPosition, 3.0f, cDWORDRed, BDebugPrimitives::cCategoryNone, 2.0f);
//       gpDebugPrimitives->addDebugSphere(pProj->getTargetLocation(), 3.0f, cDWORDRed, BDebugPrimitives::cCategoryNone, 2.0f);
//    }

#ifdef SYNC_Projectile
   syncProjectileData("BWorld::launchProjectile direction", launchDirection);
   if (useGravity)
   {
      syncProjectileData("BWorld::launchProjectile gravity", gravity);
   }
#endif

   return pProj->getID();
}

//==============================================================================
// BWorld::moveSquadsFromObstruction
//==============================================================================
bool BWorld::moveSquadsFromObstruction(BEntityID obsSquad, BOPQuadHull* pObsHull, bool checkOnly, int tryCount, bool& failNow)
{
   if (!pObsHull)
      return false;

   BSquad* pObsSquad = gWorld->getSquad(obsSquad);
   if (!pObsSquad)
      return false;

   // Look for other squads overlapping the obstruction squad.
   BEntityIDArray results(0, 20);
   BUnitQuery query1(pObsHull);
   query1.setFlagIgnoreDead(true);
   // DLM 11/21/08 - Ignore noncollideable unit obs
   //int numTargets = gWorld->getSquadsInArea(&query1, &results, false);
   int numTargets = gWorld->getSquadsInArea(&query1, &results, true);
   if (numTargets == 0)
      return false;

   BEntityIDArray moveSquads(0, numTargets);
   BSmallDynamicSimArray<BPlayerID> movePlayers(0, 1);

   BVector aveSquadDir;
   aveSquadDir.zero();

   bool bBlock = false;
   for (int i = 0; i < numTargets; i++)
   {
      BEntityID squadID = results[i];

      // Skip over the obstruction's squad
      if (squadID == obsSquad)
         continue;      

//-- FIXING PREFIX BUG ID 6019
      const BSquad* pSquad = gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (!pProtoObject)
            continue;

         // Ignore permanently non-mobile units such as a socket 
         if (pProtoObject->getFlagNonMobile())
            continue;

         // Ignore spartans inside a vehicle.  The vehicle itself should be in this list too and movement
         // will be controlled through it.
         if (pSquad->getFlagContainedSpartan())
            continue;

         // Temporary non-mobile units such as a locked down Cobra won't be able to be told to move
         if (!pSquad->canMove(false))
         {
            failNow = true;
            return true;
         }

         // squads that are in a modal power won't be able to be told to move
         if (pSquad->getSquadMode() == BSquadAI::cModePower)
         {
            failNow = true;
            return true;
         }

         // Hitched units or tow trucks won't move off obstructions
         if (pSquad->getFlagHitched() || (const_cast<BSquad*>(pSquad))->hasHitchedSquad())
         {
            failNow = true;
            return true;
         }

         // TRB 8/28/08 - Because the base obstruction can rotate but moving squad's obstructions don't, the above
         // obstruction check can return squads that have a corner of an obstruction overlapping, which is ok according
         // to the movement system.  Do the same obstruction to radius check the movement system uses to confirm the squad
         // really needs to move.
         if (pObsSquad->calculateXZDistance(pSquad) > cFloatCompareEpsilon)
            continue;

         // Air units don't block
//         if (pProtoObject->getMovementType() == cMovementTypeAir)
//            continue;

         if (checkOnly)
            return true;

         bBlock = true;

//-- FIXING PREFIX BUG ID 6017
         const BSquadActionMove* pMoveAction = (BSquadActionMove*)pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove);
//--
         if (pMoveAction == NULL)
         {
            moveSquads.add(squadID);
            movePlayers.uniqueAdd(pSquad->getPlayerID());
            aveSquadDir += pSquad->getPosition() - pObsSquad->getPosition();
         }
      }
   }
   aveSquadDir.safeNormalize();

   if (checkOnly)
      return false;

   int moveSquadCount = moveSquads.getNumber();
   if (moveSquadCount > 0)
   {
      // Calculate a plot position to use with the squad plotter for squads we want to move.
      BVector plotPos = pObsSquad->getPosition();
//-- FIXING PREFIX BUG ID 6020
      const BUnit* pObsUnit = pObsSquad->getLeaderUnit();
//--
      if (pObsUnit)
      {
         BEntityID base = pObsUnit->getAssociatedBase();
         if (base != cInvalidObjectID && base != obsSquad)
         {
//-- FIXING PREFIX BUG ID 6018
            const BUnit* pBase = gWorld->getUnit(base);
//--
            if (pBase)
            {
               BVector dir = pObsSquad->getPosition() - pBase->getPosition();
               dir.y = 0.0f;
               if (dir.safeNormalize())
               {
                  float plotDist = 20.0f + (tryCount * 5.0f);
                  plotPos += dir * (pObsSquad->getObstructionRadius() + plotDist);
               }
               else
               {
                  float plotDist = 60.0f + (tryCount * 5.0f);
                  plotPos += aveSquadDir * (pObsSquad->getObstructionRadius() + plotDist);
               }
            }
         }
         else // no base - this must be a loner socket
         {
            float plotDist = 60.0f + (tryCount * 5.0f);
            plotPos += aveSquadDir * (pObsSquad->getObstructionRadius() + plotDist);
         }
      }

      // Setup a command that will be used to move the squads.
      BSimTarget targetPos(plotPos);
      BWorkCommand tempCommand;
      tempCommand.setWaypoints(&plotPos, 1);
      tempCommand.setRecipientType(BCommand::cArmy);
      tempCommand.setSenderType(BCommand::cGame);

      // Send a separate command per player.
      for (int i = 0; i < movePlayers.getNumber(); i++)
      {
         BPlayerID playerID = movePlayers[i];
         BEntityIDArray playerSquads;
         for (int j = 0; j < moveSquadCount; j++)
         {
            BEntityID squadID = moveSquads[j];
            BSquad* pSquad = gWorld->getSquad(squadID);
            if (pSquad && pSquad->getPlayerID() == playerID)
               playerSquads.add(squadID);
         }
         if (playerSquads.getNumber() > 0)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = movePlayers[i];
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               pArmy->addSquads(playerSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
               pArmy->queueOrder(&tempCommand);
            }
         }               
      }
   }

   return bBlock;
}

//==============================================================================
// BWorld::movePlayerSquadsFromObstruction
//==============================================================================
bool BWorld::movePlayerSquadsFromObstruction(BEntityID obstructionSquad, BOPQuadHull* obstructionHull, float testRadiusX, float testRadiusZ, bool& bEnemyInTheWay)
{
   // 1/15/2008 [BSR]: To address the problem of large groups of units blocking building construction, this method now makes two passes through target queries.
   // The first pass looks only at the units in the obstruction area of the candidate building to determine whether there is currently a block. If there is, then
   // the second pass looks at an expanded region (3X the size of the candidate building's footprint) to make sure units on the perimeter do not block any units
   // actually on the foundation from getting out of the way.

   BDEBUG_ASSERT(obstructionHull);
   BEntityIDArray results(0, 100);
   BSquad* pObsSquad = gWorld->getSquad(obstructionSquad);
   BDEBUG_ASSERT(pObsSquad);
   BVector obsPos = pObsSquad->getPosition();
   float obsRadius = pObsSquad->getObstructionRadius();
//   BUnitQuery query1(obsPos, pObsSquad->getObstructionRadius(), true, 100);
   BUnitQuery query1(obstructionHull);

   int numTargets = gWorld->getSquadsInArea(&query1, &results, false);
   int numTargetsToMove = 0;
   bool bBlock = false;
   bEnemyInTheWay = false;
   for (int i = 0; i < numTargets; i++)
   {
      BEntityID squadID = results[i];
      BSquad* pSquad = gWorld->getSquad(squadID);
      BDEBUG_ASSERT(pSquad);

      // Skip over the obstruction's squad
      if (results[i] == obstructionSquad)
         continue;      

      const BProtoObject* pProtoObject = pSquad->getProtoObject();
//-- FIXING PREFIX BUG ID 6021
      const BSquadActionMove* pMoveAction = (BSquadActionMove*)pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove);
//--
      if (pMoveAction == NULL)
      {
         // Verify this isn't a squad that should not be moved (such as a unsc miner or a covenant worshipper)
         if (pSquad->getProtoObject() && pSquad->getProtoObject()->getSelectType() == cSelectTypeTarget)
            continue;

         // Don't try to move non-mobile units
         if (!pSquad->isMobile())
            continue;

         // Don't try to move squads that are in the process of loading/unloading, etc.
         if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
            continue;
      }

      if (pProtoObject && (pProtoObject->getMovementType() != cMovementTypeAir))
         bBlock = true;

      numTargetsToMove++;
   }

   if (numTargetsToMove > 0)
   {
      BUnitQuery query2(obsPos, obsRadius + 40.0f, true);
      numTargets = gWorld->getSquadsInArea(&query2, &results, false);

      testRadiusX *= 1.0f + numTargets/5.0f;
      testRadiusZ *= 1.0f + numTargets/5.0f;

      BEntityIDArray squadList;
      BObjectCreateParms objectParms;
      BArmy* pArmy = NULL;
      BPlatoon* pPlatoon = NULL;
      BSimTarget simTarget;

      for (int i = 0; i < numTargets; i++)
      {
         // Skip over the obstruction's squad
         if (results[i] == obstructionSquad)
            continue;
         
         BEntityID squadID = results[i];
         BSquad* pSquad = gWorld->getSquad(squadID);
         BDEBUG_ASSERT(pSquad);

         // Move all other squads that aren't moving
         BPlayerID squadPlayerID = pSquad->getPlayerID();

//-- FIXING PREFIX BUG ID 6022
         const BSquadActionMove* pMoveAction = (BSquadActionMove*)pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove);
//--
         if (pMoveAction == NULL)
         {
            // Verify this isn't a squad that should not be moved (such as a unsc miner or a covenant worshipper)            
            if (pSquad->getProtoObject() && pSquad->getProtoObject()->getSelectType() == cSelectTypeTarget)
               continue;

            // Don't try to move non-mobile units            
            if(!pSquad->isMobile())
               continue;

            // Don't try to move squads that are in the process of loading/unloading, etc.
            if (pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
               continue;

            // Don't try to move air units
            const BProtoObject* pProtoObject = pSquad->getProtoObject();
            if(pProtoObject && (pProtoObject->getMovementType() == cMovementTypeAir))
               continue;

            objectParms.mPlayerID = squadPlayerID;
            if (pArmy == NULL)
            {
               createArmy(objectParms);
               pPlatoon = pSquad->getParentPlatoon();
            }

            int searchAttempt = 0;
            int maxAttempts = 4;
            bool bSpotFound = false;

            float squadRadius = pSquad->getObstructionRadius();
            if (testRadiusX < (1.5f * squadRadius))
               testRadiusX = 1.5f * squadRadius;
            if (testRadiusZ < (1.5f * squadRadius))
               testRadiusZ = 1.5f * squadRadius;

            BVector position = pSquad->getPosition();
            position.y = 0.0f;
            BVector newPosition = cInvalidVector;

            while (!bSpotFound && (searchAttempt < maxAttempts))
            {
               testRadiusX += 2.5f * searchAttempt;
               testRadiusZ += 2.5f * searchAttempt;
               searchAttempt++;

               if (obstructionHull->suggestPlacement(position, testRadiusX, testRadiusZ, newPosition))
               {
                  bSpotFound = true;
               }
               else
                  bSpotFound = false;
            }

            BVector direction = newPosition - position;
            float d = direction.length();
            direction.normalize();
            direction.scale(squadRadius + d + 0.2f);
            newPosition = position + direction;
            simTarget.setPosition(newPosition);

            if (pArmy)
            {
               squadList.uniqueAdd(squadID);
               pArmy->addSquads(squadList, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
            }
         }
      }
      if (pPlatoon)
         pPlatoon->queueMove(simTarget);
   }

   return (bBlock);
}

//==============================================================================
// BWorld::getSquadPlacement
//==============================================================================
BVector BWorld::getSquadPlacement(BUnit*  pContainingUnit, BUnit*  pResource, BProtoSquad*  pProtoSquad, BProtoObject*  pProto, BVector *pForward, BVector *pRight, long preferredBoneHandle, bool &gotPreferredPosition)
{
   float   obstructionRadius = pProto->getObstructionRadius();
   BVector dir               = pContainingUnit->getForward();
   BVector pos;

   gotPreferredPosition = false;

   BVector origin;
   origin.set( 0.0f, 0.0f, 0.0f );
   dir.normalize();
   const BProtoObject* pContainingProtoObject = pContainingUnit->getProtoObject();
   if( pContainingProtoObject )
   {
      const long cExitFromDir = pContainingProtoObject->getExitFromDirection();
      dir.rotateAroundPoint( origin, Math::fDegToRad( 90.0f * cExitFromDir ), 0.0f, 0.0f );
      dir.y = 0.0f;
      dir.normalize();
   }

   if (pResource)
   {
      const BProtoObject *pProto = pResource->getProtoObject();
      BDEBUG_ASSERT(pProto);
      float maxGatherUnits = (float) pProto->getGathererLimit();
      float numGatherUnits = (float) pResource->getNumGatherUnits();
      dir = XMVector3Transform(dir, XMMatrixRotationY(XM_2PI * numGatherUnits / maxGatherUnits));
      pos = pResource->getPosition() + (dir * (pResource->getObstructionRadius() + obstructionRadius + 1.0f));
      *pForward = -dir;
   }
   else
   {
      if (pProtoSquad)
      {
         long size = pProtoSquad->getSquadSize();
         //DCP TODO.  This wasn't right before (as it really had nothing to do with how the
         //squads were controlled with code anymore and the data wasn't set right anyway).
         //For now, just making a quick approximation.  We'll fix for real once we figure out
         //what's going on with formations.
         /*long lineWidth = pProtoSquad->getLineWidth();
         long lineWidth=1;
         long numberRows = size / lineWidth;
         long numberCols = Math::Min(lineWidth, size);
         float padding = pProto->getObstructionRadius();

         //Make sure our row/columns aren't 0
         if(numberCols == 0)
            numberCols = 1;
         if(numberRows == 0)
            numberRows = 1;

         float width = numberCols * 2 * (obstructionRadius + padding);
         float depth = numberRows * 2 * (obstructionRadius + padding);
         obstructionRadius = Math::Max(width, depth) * 0.5f;*/

         //If size == 1, obstruction radius is just the single unit.
         //Else, if size == 2, double it.
         if (size == 2)
            obstructionRadius*=2.0f;
         //Else, if size > 2, halve the size and just use that as an approximation.
         else if (size > 2)
         {
            // Halwes - 12/19/2007 - This keeps flood infection forms from spawning inside the building's obstruction.
            bool flood = false;
            int numUnitNodes = pProtoSquad->getNumberUnitNodes();
            for (int i = 0; i < numUnitNodes; i++)
            {
               const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(pProtoSquad->getUnitNode(i).mUnitType);
               if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDFlood()))
               {
                  flood = true;
                  break;
               }
            }
            float factor = flood ? (float)size : (float)ceil((float)size / 2.0f);
            obstructionRadius *= factor;
         }
      }

      // if we have a base shield, we need to use that for the obstruction check
      BUnit* pObstructionCheckUnit = pContainingUnit;
      /*BEntityRef* pBaseShieldEntityRef = pContainingUnit->getFirstEntityRefByType(BEntityRef::cTypeBaseShield);
      if (pBaseShieldEntityRef)
      {
         BUnit* pBaseShieldUnit = getUnit(pBaseShieldEntityRef->mID);
         if (pBaseShieldUnit)
            pObstructionCheckUnit = pBaseShieldUnit;
      }*/

      bool flyingUnit = (pProto && pProto->getMovementType() == cMovementTypeAir);
      pos = getUnobstructedBirthPosition( pObstructionCheckUnit, obstructionRadius, &dir, preferredBoneHandle, gotPreferredPosition, flyingUnit );

      *pForward = dir;
   }

   pRight->assignCrossProduct(cYAxisVector, *pForward);
   pRight->normalize();

   return pos;
}

//==============================================================================
// BWorld::getUnobstructedBirthPosition
//==============================================================================
BVector BWorld::getUnobstructedBirthPosition( BUnit* pContainingUnit, const float obstructionRadius, BVector* pForward, long preferredBoneHandle, bool &gotPreferredPosition, bool flyingUnit )
{
   bool          buildOnTop   = pContainingUnit->isType(gDatabase.getOTIDBirthOnTop());
   const BVector containerPos = pContainingUnit->getPosition();
   const long    cPlayerID    = pContainingUnit->getPlayerID();
   BVector       pos;

   gotPreferredPosition = false;

   // If preferred bone specified then use it as the initial position to test
   bool usingBonePos = false;
   if (preferredBoneHandle != -1)
   {
      BUnit* pParkingLot = gWorld->getUnit(pContainingUnit->getAssociatedParkingLot());
      BUnit* pSpawnFrom = (pParkingLot ? pParkingLot : pContainingUnit);
      BVisual* pSpawnFromVisual = pSpawnFrom->getVisual();
      if (pSpawnFromVisual)
      {
         BVector tempPos;
         if (pSpawnFromVisual->getBone(preferredBoneHandle, &tempPos, NULL, NULL, NULL))
         {
            BMatrix worldMatrix;
            pSpawnFrom->getWorldMatrix(worldMatrix);
            worldMatrix.transformVectorAsPoint(tempPos, pos);
            usingBonePos = true;
         }
      }
   }

   // Default position if no bone
   if (!usingBonePos)
   {
      const float offset = buildOnTop ? 0.0f : pContainingUnit->getObstructionRadius() + obstructionRadius + 0.015f + gTerrainSimRep.getDataTileScale();   
      pos = containerPos + ( *pForward * offset );
   }


   // Define flags for quadtree 
   long obstructionQuadTrees = 
      BObstructionManager::cIsNewTypeCollidableStationaryUnit | // Unit that can move but is currently stationary (added this to handle locked down elephant blocking an elephant's exit point)
      BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
      BObstructionManager::cIsNewTypeBlockAirMovement |         // Funny enough (actually not so funny) this is what buildings are marked as
      BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement

   // if we explicitly want to build an air unit on top of us, only check block air movement - to fix PHX-15112. 
   if (buildOnTop && flyingUnit)
      obstructionQuadTrees = BObstructionManager::cIsNewTypeBlockAirMovement; // Funny enough (actually not so funny) this is what buildings are marked as

   // Test exit position
   bool result = (isOutsidePlayableBounds(pos, true) || gObsManager.testObstructions( pos, obstructionRadius, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, cPlayerID ));

   // If the initial test failed, try a position a little further out.
/*
   if (result)
   {
      // The obstruction radius defines the circle inside the obstruction box.  So multiply by sqrt of 2 to offset by diagonal of the obstruction box.
      const float offset2 = ((buildOnTop ? 0.0f : pContainingUnit->getObstructionRadius()) + obstructionRadius) * cSqrt2;
      pos = containerPos + ( *pForward * offset2 );
      result = (isOutsidePlayableBounds(pos, true) || gObsManager.testObstructions( pos, obstructionRadius, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, cPlayerID ));
   }
*/
   // Successfully got preferred position
   if (!result)
   {
      gotPreferredPosition = true;
      return pos;
   }

   BSquad* pContainingSquad = pContainingUnit->getParentSquad();
   if (!pContainingSquad)
      return pos;

   // Desired position is on the exit side of the container but not right on top of the position that already failed
   BVector desiredPos = pos;
   BVector right;
   right.assignCrossProduct(cYAxisVector, *pForward);
   right.normalize();
   desiredPos += (right * pContainingUnit->getObstructionRadius());

   // Find closest unobstructed position around containing squad.
   static BDynamicSimVectorArray instantiatePositions;
   instantiatePositions.setNumber(0);
   long closestDesiredPositionIndex;
   BSimHelper::findInstantiatePositions(1, instantiatePositions, pContainingSquad->getObstructionRadius(), pContainingSquad->getPosition(),
         pContainingSquad->getForward(), pContainingSquad->getRight(), obstructionRadius, desiredPos, closestDesiredPositionIndex, 4);

   // Position found
   if ((closestDesiredPositionIndex >= 0) && (closestDesiredPositionIndex < instantiatePositions.getNumber()))
      pos = instantiatePositions[closestDesiredPositionIndex];
   else if (instantiatePositions.getNumber() > 0)
      pos = instantiatePositions[0];

   return( pos );
}

//==============================================================================
// BWorld::checkForGameOver
//==============================================================================
void BWorld::checkForGameOver()
{
   if(getFlagGameOver())
      return;

   if (getFlagSkipGameOverCheck())
      return;

#ifndef BUILD_FINAL
// rg - HACK HACK don't check this in!
   //if (gConfig.isDefined(cConfigNoVictoryCondition))
    //  return;
#endif

   long gameType = -1;
//-- FIXING PREFIX BUG ID 6029
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   if(pSettings)
      pSettings->getLong(BGameSettings::cGameType, gameType);

   // See if the game is over (one or less teams left playing)
   long teamPlayingID=-1;
   long teamPlayingCount=0;
   bool gameIsOver=false;
   //bool lastDefeatedPlayerDisconnected = false;

   //Skirmish game logic
   if (gameType == BGameSettings::cGameTypeSkirmish)
   {
      for(long i=1; i<mTeams.getNumber(); i++)
      {
         if (gameType == BGameSettings::cGameTypeSkirmish && i>2)
            break;
//-- FIXING PREFIX BUG ID 6027
         const BTeam* pTeam=mTeams[i];
//--
         for(long j=0; j<pTeam->getNumberPlayers(); j++)
         {
//-- FIXING PREFIX BUG ID 5966
            const BPlayer* pPlayer=getPlayer(pTeam->getPlayerID(j));
//--
            if(pPlayer && (pPlayer->isPlaying() || pPlayer->getPlayerState() == BPlayer::cPlayerStateWon))
            {
               teamPlayingCount++;
               teamPlayingID=i;
               break;
            }
         }
      }

      //long checkTeamID = -1;
      //if (teamPlayingID == 1)
      //   checkTeamID = 2;
      //else if (teamPlayingID == 2)
      //   checkTeamID = 1;

      //uint lastDefeatedPlayerTime = 0;
      //long lastDefeatedPlayerTeamID = -1;

      //// need to see if the game is ending because the last player on the other team disconnected
      //if (checkTeamID != -1)
      //{
      //   for (long i=0; i < getNumberPlayers(); ++i)
      //   {
      //      const BPlayer* pPlayer = getPlayer(i);
      //      if (pPlayer && pPlayer->getTeamID() == checkTeamID)
      //      {
      //         if (pPlayer->getGamePlayedTime() > lastDefeatedPlayerTime)
      //         {
      //            lastDefeatedPlayerTime = pPlayer->getGamePlayedTime();
      //            lastDefeatedPlayerTeamID = i;
      //            lastDefeatedPlayerDisconnected = (pPlayer->getNetState() == BPlayer::cPlayerNetStateDisconnected);
      //         }
      //      }
      //   }
      //}

      //Additional logic to look for the case where there are no humans still playing the game
      if (teamPlayingCount < 2 && teamPlayingCount < mTeams.getNumber() - 1)
      {
         gameIsOver=true;
      }
      else
      {
         bool foundAHuman = false;
         for (long i=0;i<getNumberPlayers();i++)
         {
//-- FIXING PREFIX BUG ID 5973
            const BPlayer* pPlayer=getPlayer(i);
//--
            if(pPlayer)
            {
               if (pPlayer->isPlaying() && pPlayer->isHuman())
               {
                  foundAHuman=true;
                  break;
               }
            }
         }
         if (!foundAHuman)
         {
            gameIsOver=true;
         }
      }

      //if (gameIsOver && lastDefeatedPlayerDisconnected)
      //{
      //   // if the game is over and the last player from the other team disconnected, then resign the remaining players
      //   teamPlayingID = -1;
      //}
   }
   else
   {
      //Campaign games will have game logic set a winner/loser - we just need to set teamPlayingID to the winning team
      teamPlayingID = 0;
      for(long i=1; i<mTeams.getNumber(); i++)
      {
         BTeam* pTeam=mTeams[i];
         for(long j=0; j<pTeam->getNumberPlayers(); j++)
         {
//-- FIXING PREFIX BUG ID 5975
            const BPlayer* pPlayer=getPlayer(pTeam->getPlayerID(j));
//--
            if(pPlayer && pPlayer->isHuman() && !pPlayer->isPlaying())
            {
               //So we have a player who is human and not playing - then this game is OVER
               gameIsOver = true;
               if (pPlayer->getPlayerState()==BPlayer::cPlayerStateWon)
               {
                  teamPlayingID=i;
               }
            }
         }
         if (gameIsOver)
         {
            break;
         }
      }
   }

   if (gameIsOver)
   {
      // The game is over
      setFlagGameOver(true);
      mGameOverCountdown = gDatabase.getGameOverDelay();

      MVinceEventSync_MatchEnded( teamPlayingID );

      // Notify the winners
      for(long i=1; i < mPlayers.getNumber(); i++)
      {
         BPlayer* pPlayer = mPlayers[i];
         if (pPlayer->getTeamID() == teamPlayingID)
         {
            // This causes recursion in this method.
            pPlayer->setPlayerState(BPlayer::cPlayerStateWon);

            //notify(BEntity::cEventPlayerWon, cInvalidObjectID, i, 0);

            //if (pPlayer->isHuman())
            //{
            //   setPlayerEndGameConditions( pPlayer, true, true );
            //}
         }
         else
         {
            //if (pPlayer->getPlayerState() == BPlayer::cPlayerStatePlaying)
            //   pPlayer->setPlayerState((lastDefeatedPlayerDisconnected ? BPlayer::cPlayerStateDisconnected : BPlayer::cPlayerStateResigned));

            //if (pPlayer->isHuman())
            //{
            //   setPlayerEndGameConditions( pPlayer, true, false);
            //}
         }
      }

      // process the end of the game for all the players
      //
      // XXX this needs to happen AFTER we've finalized the player states otherwise the final score
      // calculations will happen before all player states are known and thus they would be denied
      // any bonuses for a win.
      for(long i=1; i < mPlayers.getNumber(); i++)
      {
         BPlayer* pPlayer = mPlayers[i];

         blog("BWorld::checkForGameOver - player[%s] xuid[%I64u] state[%d] netState[%d]", pPlayer->getName().getPtr(), pPlayer->getXUID(), pPlayer->getPlayerState(), pPlayer->getNetState());

         pPlayer->processGameEnd();
      }

      // now call setPlayerEndGameConditions because it relies on score which is not accurate until we call BPlayer::processGameEnd() for all players
      for(long i=1; i < mPlayers.getNumber(); i++)
      {
         BPlayer* pPlayer = mPlayers[i];

         if (pPlayer->isHuman())
         {
            if (pPlayer->getTeamID() == teamPlayingID)
               setPlayerEndGameConditions(pPlayer, true, true);
            else
               setPlayerEndGameConditions(pPlayer, true, false);
         }
      }

      if (gameType==BGameSettings::cGameTypeCampaign)
      {
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNPOSTLOBBY);
      }
      else
      {
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SKIRMPOSTLOBBY);
      }

      if (gLiveSystem)
      {
         gLiveSystem->endOfGameComplete();
      }

      gStatsManager.submit(mGametime, teamPlayingID);

      // Let's send a Game Over event up the chain so we can key off of that.
      gUserManager.notify(BEntity::cEventGameOver, cInvalidObjectID, 0, 0);

      // let's go to a skirmish post-game view if necessary
//-- FIXING PREFIX BUG ID 6030
      //const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   }
}

//==============================================================================
// BWorld::setPlayerEndGameConditions
//==============================================================================
//-- FIXING PREFIX BUG ID 6026
void BWorld::setPlayerEndGameConditions(BPlayer* pPlayer, bool gameCompleted, bool playerWon)
//--
{
   BASSERT(pPlayer->isHuman());
   BASSERT(pPlayer->getXUID());

   if (gLiveSystem)
   {
      //If we didn't have game settings at this point - then its a local, stats only leaderboard
      // So dont do this unless this player is local able to talk to live
      BUser* user = pPlayer->getUser();
      bool isFromASaveGame = false;
      if (user)
      {            
         if (playerWon)
         {
            XUserSetContext(user->getPort(), CONTEXT_PRE_CON_GAMERESULT, CONTEXT_PRE_CON_GAMERESULT_WON);
         }
         else
         {
            XUserSetContext(user->getPort(), CONTEXT_PRE_CON_GAMERESULT, CONTEXT_PRE_CON_GAMERESULT_LOST);
         }
      }
      
      //If this is a local user with online priv, signed into Live - then see if we need to create a leaderboard only session for them
      if (user && user->isSignedIntoLive() && user->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
      {

         //See if this game has already told the live system about itself (for leaderboard purposes)
         if (!gLiveSystem->hasLeaderboardGameSettings())
         {
            //No - so figure out the various info pieces as best we can, and feed them in
            BGameSettings* pSettings = gDatabase.getGameSettings();
            BDEBUG_ASSERT(pSettings);
            if (!pSettings)
            {
               return;
            }

            BLiveSessionGameClassification gameClass = cLiveSessionGameClassificationCustom;
            bool partyTeam = false;
            uint campaignMapIndex = 0;
            uint gameModeIndex= 0;
            uint difficulty = 0;

            //If this was a matchmade or live game, then this would have already been reported
            //So we must be either a custom skirmish or a campaign game
            long gameType;
            pSettings->getLong(BGameSettings::cGameType, gameType);
            if (gameType == BGameSettings::cGameTypeCampaign)
            {
               gameClass = cLiveSessionGameClassificationCampaign;
               //For campaign games - if there was more than one player then we need to mark it as co-op (partyTeam)
               long playerCount = 0;
               pSettings->getLong(BGameSettings::cPlayerCount, playerCount);
               if (playerCount>1)
               {
                  partyTeam = true;
               }
               //For campaign games - we need to get an index for the map
               BSimString map;
               pSettings->getString(BGameSettings::cMapName, map);
               campaignMapIndex = gCampaignManager.getLeaderboardIndex(map);
               isFromASaveGame = gCampaignManager.isCurrentCampaignGameFromASaveGame();
               //For campaign games - the difficulty is stored in the difficulty setting of the player
               long difficultyType;
               pSettings->getLong( PSINDEX(pPlayer->getID(), BGameSettings::cPlayerDifficultyType), difficultyType);
               difficulty = (uint)difficultyType+1;
            }
            else
            {
               //For custom games - we need the game mode 
               long gameMode;
               pSettings->getLong(BGameSettings::cGameMode, gameMode);
               gameModeIndex = (uint)gameMode+1;
               //TODO eric - the only thing we are supporting here is comp-stomp leaderboard tracking            
               //  - need comp-stomp mode detection
               //  - need to go through all AI players and get the highest difficulty setting
               return;
            }

            gLiveSystem->createLeaderboardSession( user->getPort(), user->getXuid(), 1, gameModeIndex);
            if (gLiveSystem->getLeaderboardOnlySession())
            {
               gLiveSystem->getLeaderboardOnlySession()->storeStatsGameSettings( gameClass, partyTeam, campaignMapIndex, gameModeIndex, difficulty, isFromASaveGame);
            }
         }
      }
      
      //Post up the score 
      uint32 score = (uint)gScoreManager.getFinalScore(pPlayer->getID()); 
      gLiveSystem->endOfGamePlayerWon(pPlayer->getName(), pPlayer->getXUID(), playerWon, pPlayer->getTeamID(), pPlayer->getLeaderID(), score, pPlayer->getGamePlayedTime());
   }
}

//==============================================================================
//==============================================================================
void BWorld::initializeKBsAndAIs()
{
   // It is important that the KB's are initialized first, then the AI's.
   // Also, call this AFTER all the players have been set up 
   // Note: we skip gaia in both cases.

   uint numTeams = mTeams.getSize();
   for (uint teamID=1; teamID<numTeams; teamID++)
   {
      const BTeam* pTeam = mTeams[teamID];
      if (!pTeam)
         continue;

      bool bTeamIsAllHuman = true;
      long numPlayers = pTeam->getNumberPlayers();
      for (long teamPlayerIndex=0; teamPlayerIndex<numPlayers; teamPlayerIndex++)
      {
         const BPlayer* pPlayer = gWorld->getPlayer(pTeam->getPlayerID(teamPlayerIndex));
         if (pPlayer && !pPlayer->isHuman())
         {
            bTeamIsAllHuman = false;
            break;
         }
      }

      // If the team is all human, doesn't require a KB.
      if (!bTeamIsAllHuman)
         createKB(teamID);
   }

   uint numPlayers = mPlayers.getSize();
   for (uint playerID=1; playerID<numPlayers; playerID++)
   {
      // Gaia doesn't need an AI at all.
      BPlayer* pPlayer = mPlayers[playerID];
      if (!pPlayer || pPlayer->isGaia())
         continue;

      // Human players need a factoid manager but no AI
      // Non human players need an AI but no factoid manager.
      if (pPlayer->isHuman())
      {
         pPlayer->initializeFactoidManager();
      }
      else
      {
         createAI(playerID, pPlayer->getTeamID());
      }    
   }
}


//==============================================================================
//==============================================================================
void BWorld::updateAllKBsForSquad(BSquad* pSquad)
{
   if (mpAIManager)
      mpAIManager->updateAllKBsForSquad(pSquad);
}


//==============================================================================
//==============================================================================
BRelationType BWorld::getTeamRelationType(BTeamID team1, BTeamID team2)
{
   if(team1<0 || team1>=cMaximumSupportedTeams || team2<0 || team2>=cMaximumSupportedTeams)
      return cRelationTypeNeutral;
   return mTeamRelationType[team1][team2];
}

//==============================================================================
//==============================================================================
void BWorld::setTeamRelationType(BTeamID team1, BTeamID team2, BRelationType relationType)
{
   if(team1<0 || team1>=cMaximumSupportedTeams || team2<0 || team2>=cMaximumSupportedTeams)
      return;
   mTeamRelationType[team1][team2] = relationType;
}

//==============================================================================
//==============================================================================
void BWorld::setOccluded(BVector position, float radius, bool occluded)
{
   if (gConfig.isDefined(cConfigNoCull))
      return;

   BVector sphere=position;
   sphere.w=radius;

   uint numObjects = (uint) getUnitsHighWaterMark();
   uint type = BEntity::cClassTypeUnit << 28;
   for (uint i = 0; i < numObjects; i++)
   {
      BUnit* pObject = getUnit(type | i, true);
      if (pObject)
      {
         const BBoundingBox* pBox=pObject->getVisualBoundingBox();
         if (pBox && pBox->spheresIntersect(position, radius))
         {
            const BProtoObject* pProtoObject=pObject->getProtoObject();
            bool noCull = (pProtoObject ? pProtoObject->getFlagNoCull() : false);
            if (!noCull)
               pObject->setFlagOccluded(occluded);
         }
      }
   }

   numObjects = (uint) getObjectsHighWaterMark();
   type = BEntity::cClassTypeObject << 28;
   for (uint i = 0; i < numObjects; i++)
   {
      BObject* pObject = getObject(type | i, true);
      if (pObject)
      {
         const BBoundingBox* pBox=pObject->getVisualBoundingBox();
         if (pBox && pBox->spheresIntersect(position, radius))
         {
            const BProtoObject* pProtoObject=pObject->getProtoObject();
            bool noCull = (pProtoObject ? pProtoObject->getFlagNoCull() : false);
            if (!noCull)
               pObject->setFlagOccluded(occluded);
         }
      }
   }

   gTerrain.setExclusionSphere(sphere,occluded);
}

//==============================================================================
//==============================================================================
int BWorld::addCustomCommand(BCustomCommand& command)
{
   command.mID=mNextCustomCommandID;
   mNextCustomCommandID++;
   mCustomCommands.add(command);
   return command.mID;
}

//==============================================================================
//==============================================================================
void BWorld::removeCustomCommand(int id)
{
   uint count=mCustomCommands.getSize();
   for (uint i=0; i<count; i++)
   {
//-- FIXING PREFIX BUG ID 6033
      const BCustomCommand& item=mCustomCommands[i];
//--
      if (item.mID==id)
      {
         mCustomCommands.removeIndex(i);
         return;
      }
   }
}

//==============================================================================
//==============================================================================
BCustomCommand* BWorld::getCustomCommand(int id)
{
   uint count=mCustomCommands.getSize();
   for (uint i=0; i<count; i++)
   {
      BCustomCommand& item=mCustomCommands[i];
      if (item.mID==id)
         return &item;
   }
   return NULL;
}

//==============================================================================
//==============================================================================
float BWorld::getAreaCombatValue(BVector location, float radius, BPlayerID playerID, int relationType, bool* playerIDInArea)
{
   if(playerIDInArea)
      *playerIDInArea = false;

   //-- Look for all the units within LOS that are visible
   BUnitQuery query(location, radius, true);   
   query.setFlagIgnoreDead(true);
   query.setRelation(playerID, relationType);

   BEntityIDArray results;
   gWorld->getSquadsInArea(&query, &results, false);

   //-- Add up all the values of the units visible to me
   float value = 0.0f;
//-- FIXING PREFIX BUG ID 6035
   const BSquad* pSquad = NULL;
//--
   for(long i=0; i < results.getNumber(); i++)
   {
      pSquad = gWorld->getSquad(results[i]);
      if(!pSquad)
         continue;

      //-- Don't take buildings into account.
      if(pSquad->getProtoObject() && (pSquad->getProtoObject()->getObjectClass() == cObjectClassBuilding))
         continue;

      if(playerIDInArea)
      {
         if(pSquad->getPlayerID() == playerID)
            *playerIDInArea = true;
      }
      
      value += pSquad->getCombatValue();
   }

   return value;
}

//==============================================================================
//==============================================================================
void BWorld::setSimBounds(float minX, float minZ, float maxX, float maxZ)
{
   // Clamp to world size
   float worldSize = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();

   if (minX < 0.0f)
      minX = 0.0f;
   if (maxX > worldSize)
      maxX = worldSize;
   if (minZ < 0.0f)
      minZ = 0.0f;
   if (maxZ > worldSize)
      maxZ = worldSize;
   if (minX > maxX)
   {
      float tempVal=minX;
      minX=maxX;
      maxX=tempVal;
   }
   if (minZ > maxZ)
   {
      float tempVal=minZ;
      minZ=maxZ;
      maxZ=tempVal;
   }

   int maxVal=gTerrainSimRep.getNumXDataTiles()-1;

   if (mFlagOutsideBoundsBlocked)
   {
      // Remove old "outside of sim bounds" blockers
      int oldMinX=gTerrainSimRep.worldToTile(mSimBoundsMinX);
      int oldMinZ=gTerrainSimRep.worldToTile(mSimBoundsMinZ);
      int oldMaxX=gTerrainSimRep.worldToTile(mSimBoundsMaxX);
      int oldMaxZ=gTerrainSimRep.worldToTile(mSimBoundsMaxZ);
      for (int i=1; i<mTeams.getNumber(); i++)
      {
         if (oldMinZ > 0)
            gVisibleMap.unblock(0, 0, maxVal, oldMinZ-1, i);
         if (oldMaxZ < maxVal)
            gVisibleMap.unblock(0, oldMaxZ+1, maxVal, maxVal, i);
         if (oldMinX > 0)
            gVisibleMap.unblock(0, oldMinZ, oldMinX-1, oldMaxZ, i);
         if (oldMaxX < maxVal)
            gVisibleMap.unblock(oldMaxX+1, oldMinZ, maxVal, oldMaxZ, i);
      }
   }

   if (minX > 0 || minZ > 0 || maxX < worldSize || maxZ < worldSize)
   {
      mFlagPlayableBounds=true;

      if (gConfig.isDefined(cConfigBlockOutsideBounds))
      {
         // Block areas outside of sim bounds
         int newMinX=gTerrainSimRep.worldToTile(minX);
         int newMaxX=gTerrainSimRep.worldToTile(maxX);
         int newMinZ=gTerrainSimRep.worldToTile(minZ);
         int newMaxZ=gTerrainSimRep.worldToTile(maxZ);
         for (int i=1; i<mTeams.getNumber(); i++)
         {
            if (newMinZ > 0)
               gVisibleMap.block(0, 0, maxVal, newMinZ-1, i);
            if (newMaxZ < maxVal)
               gVisibleMap.block(0, newMaxZ+1, maxVal, maxVal, i);
            if (newMinX > 0)
               gVisibleMap.block(0, newMinZ, newMinX-1, newMaxZ, i);
            if (newMaxX < maxVal)
               gVisibleMap.block(newMaxX+1, newMinZ, maxVal, newMaxZ, i);
         }
         mFlagOutsideBoundsBlocked=true;
      }
   }
   else
   {
      mFlagPlayableBounds=false;
      mFlagOutsideBoundsBlocked=false;
   }

   // Save the new values
   mSimBoundsMinX=minX;
   mSimBoundsMinZ=minZ;
   mSimBoundsMaxX=maxX;
   mSimBoundsMaxZ=maxZ;

   // AJL 7/16/08 - Don't call generateWorldBounderies for now since it is very slow and causes long pauses during a scenario.
   // We need to find a solution that doesn't involve changing the edge of the world bounderies.
   // Generate boundary obstructions
   // jce [10/3/2008] -- Turning this back on.  Now we're going to NOT update the pather quadtrees and add some special
   // case stuff to the lrp pathing loop to check for out of bounds on-the-fly.  This adds a bit of a runtime overhead but
   // avoids the painful recompute hitch.
   //DWORD edgeTime1 = timeGetTime();
   gObsManager.generateWorldBoundries();
   //DWORD edgeTime2 = timeGetTime();
   //DWORD edgeElapsed = edgeTime2 - edgeTime1;
   //blogtrace("generateWorldBoundries time = %u", edgeElapsed);

   // Setup minimap blocker data
   for (uint i=0; i<4; i++)
   {
      mOutsideOfBoundsBlockerCenter[i]=cInvalidVector;
      mOutsideOfBoundsBlockerSize[i]=cInvalidVector;
   }
   if (mFlagOutsideBoundsBlocked)
   {
      if (mSimBoundsMinZ > 0)
      {
         //gMiniMap.block(0, 0, maxVal, newMinZ-1, teamID);
         BVector p1(0.0f, 0.0f, 0.0f);
         BVector p2(worldSize, 0.0f, mSimBoundsMinZ);
         BVector rectSize=((p2-p1)*0.5f);
         BVector center=p1+rectSize;
         mOutsideOfBoundsBlockerSize[0]=rectSize;
         mOutsideOfBoundsBlockerCenter[0]=center;
      }
      if (mSimBoundsMaxZ < worldSize)
      {
         //gMiniMap.block(0, newMaxZ+1, newMaxVal, newMaxVal, teamID);
         BVector p1(0.0f, 0.0f, mSimBoundsMaxZ);
         BVector p2(worldSize, 0.0f, worldSize);
         BVector rectSize=((p2-p1)*0.5f);
         BVector center=p1+rectSize;
         mOutsideOfBoundsBlockerSize[1]=rectSize;
         mOutsideOfBoundsBlockerCenter[1]=center;
      }
      if (mSimBoundsMinX > 0)
      {
         //gMiniMap.block(0, newMinZ, newMinX-1, newMaxZ, teamID);
         BVector p1(0.0f, 0.0f, mSimBoundsMinZ);
         BVector p2(mSimBoundsMinX, 0.0f, mSimBoundsMaxZ);
         BVector rectSize=((p2-p1)*0.5f);
         BVector center=p1+rectSize;
         mOutsideOfBoundsBlockerSize[2]=rectSize;
         mOutsideOfBoundsBlockerCenter[2]=center;
      }
      if (mSimBoundsMaxX < worldSize)
      {
         //gMiniMap.block(mSimBoundsMaxX+1, mSimBoundsMinZ, maxVal, mSimBoundsMaxZ, teamID);
         BVector p1(mSimBoundsMaxX, 0.0f, mSimBoundsMinZ);
         BVector p2(worldSize, 0.0f, mSimBoundsMaxZ);
         BVector rectSize=((p2-p1)*0.5f);
         BVector center=p1+rectSize;
         mOutsideOfBoundsBlockerSize[3]=rectSize;
         mOutsideOfBoundsBlockerCenter[3]=center;
      }
   }
}

//==============================================================================
//==============================================================================
bool BWorld::isOutsidePlayableBounds(BVector pos, bool forceCheckWorldBoundaries) const
{
   if (!mFlagPlayableBounds)
   {
      if (forceCheckWorldBoundaries)
      {
         if (pos.x < 0.0f || pos.z < 0.0f)
            return true;
         float worldSize = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();
         if (pos.x > worldSize || pos.z > worldSize)
            return true;
      }
      return false;
   }

   if (pos.x < mSimBoundsMinX || pos.z < mSimBoundsMinZ || pos.x > mSimBoundsMaxX || pos.z > mSimBoundsMaxZ)
      return true;

   return false;
}

//==============================================================================
//==============================================================================
void BWorld::setFogOfWar(bool useFog, bool useSkirt, bool onlyVisual)
{
   if (onlyVisual)
   {
      if (useFog)
      {
         if (mFlagNoFogVis)
            mFlagNoFogVis = false;
      }
      else
      {
         if (!mFlagNoFogVis)
         {
            mFlagNoFogVis = true;
            float size=gTerrainSimRep.tileToWorld(gTerrainSimRep.getNumXDataTiles());
            gUIManager->revealMinimap(BVector(size*0.5f, 0.0f, size*0.5f), size, size);
         }
      }
   }
   else
   {
      if (useFog)
      {
         if (mFlagNoFogSim)
         {
            // Turn fog of war back on... Unexplore all tiles.
            for (int i=1; i<getNumberTeams(); i++)
               gVisibleMap.unexploreEntireMap(i);
            mFlagNoFogSim = false;
         }
      }
      else
      {
         if (!mFlagNoFogSim)
         {
            // Turn off fog of war... Explore all tiles and make them visible.
            for (int i=1; i<getNumberTeams(); i++)
               gVisibleMap.exploreEntireMap(i);
            mFlagNoFogSim = true;
         }
      }
   }

   mFlagShowSkirt = (useSkirt && !gConfig.isDefined(cConfigNoTerrainSkirt));
   gTerrain.setRenderSkirt(mFlagShowSkirt);
}

//==============================================================================
//==============================================================================
void BWorld::clearBlackMap()
{
   for (int i=1; i<getNumberTeams(); i++)
      gVisibleMap.updateEntireMap(i);
   float size=gTerrainSimRep.tileToWorld(gTerrainSimRep.getNumXDataTiles());
   gUIManager->revealMinimap(BVector(size*0.5f, 0.0f, size*0.5f), size, size);
}

//==============================================================================
//==============================================================================
bool BWorld::isSphereVisible(XMVECTOR center, float radius) const
{
   if (gUserManager.getPrimaryUser()->isSphereVisible(center, radius))
      return true;
   if (gGame.isSplitScreen())
   {
      if (gUserManager.getSecondaryUser()->isSphereVisible(center, radius))
         return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
void BWorld::addTransformedTactic(BTactic* mpTactic)
{
   mTransformedTactics.add(mpTactic);
}

//==============================================================================
//==============================================================================
void BWorld::addPhysicsBreakOffObjectAsync(const BObject* pParentObject, const hkpShape* pParentShape)
{
   BScopedLightWeightMutex lock(mPhysicsBreakOffObjectMutex);
   BPhysicsBreakOffObject& newObj = mPhysicsBreakOffObjects.grow();
   newObj.mpParentObject = pParentObject;
   newObj.mpNewShape = pParentShape;
}

//==============================================================================
//==============================================================================
void BWorld::addPhysicsVehicleTerrainCollisionAsync(BPhysicsGroundVehicleAction* pPhysicsAction)
{
   BScopedLightWeightMutex lock(mPhysicsVehicleTerrainCollisionMutex);
   mPhysicsVehicleTerrainCollisionActions.uniqueAdd(pPhysicsAction);
}

//==============================================================================
//==============================================================================
int16 BWorld::getNewExplorationGroup()
{
   ++mNumExplorationGroups;
   BASSERT(mNumExplorationGroups > 0);
   BExplorationGroupEntry entry;
   BExplorationGroupHashMap::InsertResult result = mExplorationGroups.insert(mNumExplorationGroups, entry);
   BASSERT(result.second);
   return mNumExplorationGroups;
}

//==============================================================================
//==============================================================================
void BWorld::addObjectToExplorationGroup(const BObject& object)
{
   int8 groupId = object.getExplorationGroup();
   BExplorationGroupHashMap::iterator it = mExplorationGroups.find(groupId);
   BASSERT(it != mExplorationGroups.end());
   BExplorationGroupEntry& entry = it->second;

   // auto trigger the team the object is a member of
   BTeamID objectTeamId = object.getTeamID();
   if (objectTeamId != cInvalidTeamID)
      entry.triggeredTeams.uniqueAdd(objectTeamId);
   entry.objects.uniqueAdd(object.getID());
}

//==============================================================================
//==============================================================================
void BWorld::removeObjectFromExplorationGroup(const BObject& object)
{
   int8 groupId = object.getExplorationGroup();
   BExplorationGroupHashMap::iterator it = mExplorationGroups.find(groupId);
   BASSERT(it != mExplorationGroups.end());
   BExplorationGroupEntry& entry = it->second;

   entry.objects.remove(object.getID());

   // remove the entry if there are no objects left
   if (entry.objects.getSize() == 0)
   {
      mExplorationGroups.erase(it);

      // if this is an active exploration group, remove the entry from there as well
      for (uint i = 0; i < mActiveExplorationGroups.getSize(); ++i)
      {
         if (mActiveExplorationGroups[i].explorationGroupIndex == groupId)
         {
            mActiveExplorationGroups.removeIndex(i);
            break;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BWorld::makeExplorationGroupVisible(BTeamID teamId, int16 groupId)
{
   // we need to only update exploration groups after update number 2 
   // because of dopple on start objects
   // on frame 0, the dopple of start objects have a single frame revealer added
   // on frame 1, single frame revealer is removed
   // on frame 2, the soft dopple is computed, which causes another visibility update
   // on frame 3, we're safe to update exploration groups
   if (getUpdateNumber() <= 2)
      return;

   BExplorationGroupHashMap::iterator it = mExplorationGroups.find(groupId);
   BASSERT(it != mExplorationGroups.end());
   BExplorationGroupEntry& entry = it->second;

   // if this exploration group has already been triggered, then bail
   if (entry.triggeredTeams.contains(teamId))
      return;

   entry.triggeredTeams.uniqueAdd(teamId);

   BUnit* pUnit = NULL;
   for (uint i = 0; i < entry.objects.getSize(); ++i)
   {
      pUnit = getUnit(entry.objects.get(i));
      if (pUnit)
         pUnit->addReveal(teamId);
   }  

   float showTime=0.0f;
   gConfig.get(cConfigExplorationRegionShowTime, &showTime);

   // add a timer to remove the exploration group after a certain amount of time
   BExplorationGroupTimerEntry timerEntry;
   timerEntry.team = teamId;
   timerEntry.timeLeft = showTime;
   timerEntry.explorationGroupIndex = groupId;
   mActiveExplorationGroups.push_back(timerEntry);
}

//==============================================================================
//==============================================================================
void BWorld::stopActiveExplorationGroup(int16 index)
{
   BASSERT(index >= 0 && (uint)index < mActiveExplorationGroups.getSize());
   const BExplorationGroupTimerEntry& timerEntry = mActiveExplorationGroups.get(index);

   BExplorationGroupHashMap::const_iterator it = mExplorationGroups.find(timerEntry.explorationGroupIndex);
   BASSERT(it != mExplorationGroups.end());
   const BExplorationGroupEntry& entry = it->second;

   BUnit* pUnit = NULL;
   for (uint i = 0; i < entry.objects.getSize(); ++i)
   {
      pUnit = getUnit(entry.objects.get(i));
      if (pUnit)
         pUnit->removeReveal(timerEntry.team);
   }  
}

//==============================================================================
//==============================================================================
void BWorld::resetExplorationGroupTeamData(int16 index, BPlayerID exceptionID)
{
   BExplorationGroupHashMap::iterator it = mExplorationGroups.find(index);
   BASSERT(it != mExplorationGroups.end());
   BExplorationGroupEntry& entry = it->second;

   for (uint i = 0; i < mActiveExplorationGroups.size(); i++)
   {
      const BExplorationGroupTimerEntry& timerEntry = mActiveExplorationGroups.get(i);
      if (timerEntry.explorationGroupIndex == index)
      {
         stopActiveExplorationGroup(i);
         mActiveExplorationGroups.removeIndex(i, true);
         break;
      }
   }

   entry.triggeredTeams.setNumber(0);

   if (exceptionID != cInvalidTeamID)
      entry.triggeredTeams.uniqueAdd(exceptionID); 
}

//==============================================================================
//==============================================================================
void BWorld::resetExplorationGroups()
{
   mExplorationGroups.clear();
   mNumExplorationGroups = 0;
   mActiveExplorationGroups.clear();
}

//==============================================================================
//==============================================================================
void BWorld::updateSingleThreadedPhysicsData()
{
   // Create all objects that were spawned by the last update to the  
   // physics.  Currently the only place where physics create units is when a breakable shape breaks
   // into subshapes.
   createAllPhysicsBreakOffObjects();

   // Resolve vehicle terrain collisions single-threaded
   resolveAllPhysicsVehicleTerrainCollisions();
}

//==============================================================================
//==============================================================================
int __cdecl sortPhysicsBreakOffObjects(const void* elem1, const void* elem2)
{
   const BPhysicsBreakOffObject* pPBOO1 = ((BPhysicsBreakOffObject*)elem1);
   const BPhysicsBreakOffObject* pPBOO2 = ((BPhysicsBreakOffObject*)elem2);

   if (!pPBOO1 || !pPBOO2)
   {
      // wha?
      return 0;
   }

   const BObject *pParentObject1 = pPBOO1->mpParentObject;
   const BObject *pParentObject2 = pPBOO2->mpParentObject;
   BASSERT(pParentObject1 && pParentObject2);

   // Check parent entity ID first
   if (pParentObject1->getID() < pParentObject2->getID())
      return -1;
   else if (pParentObject1->getID() > pParentObject2->getID())
      return 1;
   else
   {
      // If equal, then check mesh index
      const hkpShape* pShape1 = pPBOO1->mpNewShape;
      const hkpShape* pShape2 = pPBOO2->mpNewShape;
      BASSERT(pShape1 && pShape2);
      long meshIndex1 = (long) pShape1->getUserData();
      long meshIndex2 = (long) pShape2->getUserData();
      if (meshIndex1 < meshIndex2)
         return -1;
      else if (meshIndex1 > meshIndex2)
         return 1;
   }

   // equal - wha?
   return 0;
}


//==============================================================================
//==============================================================================
void BWorld::createAllPhysicsBreakOffObjects()
{
   // Sort list
   mPhysicsBreakOffObjects.sort(sortPhysicsBreakOffObjects);

   // Flush list of units
   long breakOffCount = mPhysicsBreakOffObjects.getNumber();
   syncUnitData("BWorld::createAllPhysicsBreakOffObjects breakOffCount", breakOffCount);
   for(long i = 0; i < breakOffCount; i++)
   {
      const BPhysicsBreakOffObject* pBreakOffObject = &(mPhysicsBreakOffObjects[i]);
      BASSERT(pBreakOffObject);
      if (!pBreakOffObject)
         continue;

      const BObject *pParentObject = pBreakOffObject->mpParentObject;
      BASSERT(pParentObject);
      if (!pParentObject)
         continue;

      syncUnitData("BWorld::createAllPhysicsBreakOffObjects pParentObject id", pParentObject->getID());
      syncUnitData("BWorld::createAllPhysicsBreakOffObjects pParentObject position", pParentObject->getPosition());
      syncUnitData("BWorld::createAllPhysicsBreakOffObjects pParentObject forward", pParentObject->getForward());

      const BPhysicsObject* pParentPhysicsObject = pParentObject->getPhysicsObject();
      BASSERT(pParentPhysicsObject);
      if (!pParentPhysicsObject)
         break;

      const hkpShape* pShape = pBreakOffObject->mpNewShape;
      if (!pShape)
         continue;
      long meshIndex = (long) pShape->getUserData();

      syncUnitData("BWorld::createAllPhysicsBreakOffObjects meshIndex", meshIndex);

      // Create a new BObject from the visual item
      //-- Set object params
      BObjectCreateParms objectparams;
      objectparams.mPosition = pParentObject->getPosition();
      objectparams.mForward = pParentObject->getForward();
      objectparams.mRight = pParentObject->getRight();
      objectparams.mNoTieToGround = true;
      objectparams.mPhysicsReplacement = true;
      objectparams.mPlayerID = pParentObject->getPlayerID();
      objectparams.mProtoObjectID = gDatabase.getPOIDPhysicsThrownObject();
      objectparams.mProtoSquadID = -1;
      objectparams.mStartBuilt = false;
      objectparams.mType = BEntity::cClassTypeObject;
      objectparams.mMultiframeTextureIndex = pParentObject->getMultiframeTextureIndex();

      BVector pos;
      pParentPhysicsObject->getPosition(pos);
      BPhysicsMatrix rot;
      pParentPhysicsObject->getRotation(rot);

      //-- Set physics params
      BPhysicsObjectParams physicsparams;
      physicsparams.position = pos;
      physicsparams.rotation = rot;
      physicsparams.centerOffset = pParentPhysicsObject->getCenterOffset();
      syncUnitData("BWorld::createAllPhysicsBreakOffObjects centerOffset", physicsparams.centerOffset);
      physicsparams.centerOfMassOffset = BVector(0.0f, 0.0f, 0.0f);
      physicsparams.pHavokShape = pShape;
      physicsparams.restitution = pParentPhysicsObject->getRestitution();
      physicsparams.friction = pParentPhysicsObject->getFriction();
      physicsparams.angularDamping = pParentPhysicsObject->getAngularDamping();
      physicsparams.linearDamping = pParentPhysicsObject->getLinearDamping();
      physicsparams.mass = pParentPhysicsObject->getMass() / 4.0f;   // Average 4 parts per multipart
      physicsparams.collisionFilterInfo = gWorld->getPhysicsWorld()->createCollisionFilterInfo();
      physicsparams.userdata = 0;
      physicsparams.breakable = false;
      physicsparams.fixed = false;


      BObject* pChildObject = gWorld->createVisPhysicsObjectDirect(objectparams, pParentObject->getVisual(), physicsparams, true);

      if(!pChildObject)
         continue;

      // Transfer linear and angular velocity from the parent to the child
      BPhysicsObject* pChildPhysicsObject = pChildObject->getPhysicsObject();

      BVector linearVelocity, angularVelocity;
      pParentPhysicsObject->getLinearVelocity(linearVelocity);
      pParentPhysicsObject->getAngularVelocity(angularVelocity);


      // MS 7/2/2008: we want the parent object to look more like it's shattering
      if(pShape->getType() == HK_SHAPE_CONVEX_TRANSLATE)
      {
         BVector dir;
         hkpConvexTranslateShape *pCTS = (hkpConvexTranslateShape*)pShape;
         gPhysics->convertPoint(pCTS->getTranslation(), dir);
         dir.normalize();
         const float cVelMult = 9.0f;
         linearVelocity += dir * cVelMult;
      }

      // MS 7/2/2008: copy lifespan
      if(pParentObject->getFlagHasLifespan())
         pChildObject->setLifespan(pParentObject->getProtoObject()->getLifespan());

      if (pChildPhysicsObject)
      {
#ifdef SYNC_World
         syncWorldData("BWorld::createAllPhysicsBreakOffObjects setLinearVelocity", linearVelocity);
#endif
         pChildPhysicsObject->setLinearVelocity(linearVelocity);
         pChildPhysicsObject->setAngularVelocity(angularVelocity);
      }

      // copy the additional textures over
      pChildObject->copyAdditionalTextures(pParentObject);

      BVisual* pParentVisual = pParentObject->getVisual();
      if (pParentVisual && pParentVisual->mModelAsset.mType == cVisualAssetGrannyModel && pParentVisual->mpInstance)
      {
         BBitArray parentRenderMask = ((BGrannyInstance*)(pParentVisual->mpInstance))->getMeshRenderMask();

         // Only enable the render mask for the part that we are spawning
         BBitArray childRenderMask = parentRenderMask;
         childRenderMask.clear();
         childRenderMask.setBit(meshIndex);

         BVisual* pChildVisual = pChildObject->getVisual();
         if (pChildVisual && pChildVisual->mModelAsset.mType == cVisualAssetGrannyModel && pChildVisual->mpInstance)
         {
            ((BGrannyInstance*)(pChildVisual->mpInstance))->setMeshRenderMask(childRenderMask);
         }

         // On the parent object disable rendering of the part that has been broken off
         //
         parentRenderMask.clearBit(meshIndex);

         ((BGrannyInstance*)(pParentVisual->mpInstance))->setMeshRenderMask(parentRenderMask);
      }


      // Copy obsurable state
      pChildObject->setFlagObscurable(pParentObject->getFlagObscurable());
   }

   resetAllPhysicsBreakOffObjects();
}

//==============================================================================
//==============================================================================
void BWorld::resetAllPhysicsBreakOffObjects()
{
   mPhysicsBreakOffObjects.clear();
}


//==============================================================================
//==============================================================================
int __cdecl sortPhysicsVehicleTerrainCollisions(const void* elem1, const void* elem2)
{
   const BPhysicsGroundVehicleAction* pPGVA1 = (BPhysicsGroundVehicleAction*) (*((int*)elem1));
   const BPhysicsGroundVehicleAction* pPGVA2 = (BPhysicsGroundVehicleAction*) (*((int*)elem2));

   if (!pPGVA1 || !pPGVA2)
   {
      // wha?
      return 0;
   }

   BEntityID id1 = pPGVA1->getSimEntityID();
   BEntityID id2 = pPGVA2->getSimEntityID();

   // Check parent entity ID first
   if (id1 < id2)
      return -1;
   else if (id1 > id2)
      return 1;

   // equal - wha?
   return 0;
}


//==============================================================================
//==============================================================================
void BWorld::resolveAllPhysicsVehicleTerrainCollisions()
{
   // Don't do anything if there are no collisions to resolve
   if (mPhysicsVehicleTerrainCollisionActions.getNumber() <= 0)
      return;

   // Sort list
   mPhysicsVehicleTerrainCollisionActions.sort(sortPhysicsVehicleTerrainCollisions);

   // For each action in the list, call the single-threaded collision resolution function
   for (int i = 0; i < mPhysicsVehicleTerrainCollisionActions.getNumber(); i++)
   {
      BPhysicsGroundVehicleAction* pAction = mPhysicsVehicleTerrainCollisionActions[i];
      if (pAction)
         pAction->mainThreadTerrainCollisionResolution();
   }

   // Reset list
   resetAllPhysicsVehicleTerrainCollisions();
}

//==============================================================================
//==============================================================================
void BWorld::resetAllPhysicsVehicleTerrainCollisions()
{
   mPhysicsVehicleTerrainCollisionActions.setNumber(0);
}

//==============================================================================
//==============================================================================
DWORD BWorld::getPlayerColor(long playerID, BPlayerColorContext context) 
{
//-- FIXING PREFIX BUG ID 6038
   const BUser* pUser = gUserManager.getPrimaryUser();  //eventually needs to be current user
//--

   const BPlayerColor* pPlayerColors;
   if (!pUser || !pUser->getOption_FriendOrFoeColorsEnabled() || (playerID == 0))  //don't use friend or foe for gaia
      pPlayerColors = &mPlayerColors[mPlayerColorCategory][playerID];
   else
   {
      long teamID = 0;
      if (gWorld->getPlayer(playerID))
         teamID = gWorld->getPlayer(playerID)->getTeamID();

      // [8/26/2008 xemu] added support for a "neutral" color 
      if (pUser->getPlayerID() == playerID)
         pPlayerColors = &gDatabase.getFOFSelfColor(mPlayerColorCategory);
      else if (pUser->getTeamID() == teamID)
         pPlayerColors = &gDatabase.getFOFAllyColor(mPlayerColorCategory);
      else if ((gWorld->getPlayer(playerID) != NULL) && gWorld->getPlayer(playerID)->isNeutral(pUser->getPlayerID()))
         pPlayerColors = &gDatabase.getFOFNeutralColor(mPlayerColorCategory);
      else
         pPlayerColors = &gDatabase.getFOFEnemyColor(mPlayerColorCategory);
   }         
      
   switch (context)
   {
      case cPlayerColorContextSelection:
         return pPlayerColors->mSelection;
      case cPlayerColorContextMinimap:
         return pPlayerColors->mMinimap;
      case cPlayerColorContextUI:
         return pPlayerColors->mUI;
   }     
   
   return pPlayerColors->mObjects;
}

//==============================================================================
//==============================================================================
DWORD BWorld::getCorpseColor(long playerID, BPlayerColorContext context) 
{
//-- FIXING PREFIX BUG ID 6039
   const BUser* pUser = gUserManager.getPrimaryUser();  //eventually needs to be current user
//--

   const BPlayerColor* pPlayerColors;
   if (!pUser || !pUser->getOption_FriendOrFoeColorsEnabled() || (playerID == 0))  //don't use friend or foe for gaia
      pPlayerColors = &mPlayerColors[mPlayerColorCategory][playerID];
   else
   {
      long teamID = 0;
      if (gWorld->getPlayer(playerID))
         teamID = gWorld->getPlayer(playerID)->getTeamID();

      // [8/26/2008 xemu] added support for neutral color 
      if (pUser->getPlayerID() == playerID)
         pPlayerColors = &gDatabase.getFOFSelfColor(mPlayerColorCategory);
      else if (pUser->getTeamID() == teamID)
         pPlayerColors = &gDatabase.getFOFAllyColor(mPlayerColorCategory);
      else if (pUser->getTeamID() == teamID)
         pPlayerColors = &gDatabase.getFOFNeutralColor(mPlayerColorCategory);
      else
         pPlayerColors = &gDatabase.getFOFEnemyColor(mPlayerColorCategory);
   }         
      
   switch (context)
   {
      case cPlayerColorContextSelection:
         return pPlayerColors->mSelection;
      case cPlayerColorContextMinimap:
         return pPlayerColors->mMinimap;
      case cPlayerColorContextUI:
         return pPlayerColors->mUI;
   }     
   
   return pPlayerColors->mCorpse;
}

//==============================================================================
//==============================================================================
void BWorld::setPlayerColor(BPlayerColorCategory category, long playerID, long colorIndex) 
{ 
   BDEBUG_ASSERT(category <= cMaxPlayerColorCategories);
   BDEBUG_ASSERT((playerID >= 0) && (playerID < cMaximumSupportedPlayers));
   BDEBUG_ASSERT((colorIndex >= 0) && (colorIndex <= UINT8_MAX));
   
   mPlayerColors[category][playerID] = gDatabase.getPlayerColor(category, colorIndex); 
         
   mPlayerColorIndices[category][playerID] = static_cast<uchar>(colorIndex);

#ifndef BUILD_FINAL
   mPlayerColorLoadCount = gDatabase.getPlayerColorLoadCount();
#endif   
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BWorld::checkForDirtyPlayerColors()
{
   // See if the player colors have been reloaded.
   if (mPlayerColorLoadCount == gDatabase.getPlayerColorLoadCount())
      return;
      
   mPlayerColorLoadCount = gDatabase.getPlayerColorLoadCount();
   
   for (uint category = 0; category < cMaxPlayerColorCategories; category++)
      for(uint playerIndex = 0; playerIndex < cMaximumSupportedPlayers; playerIndex++)
         mPlayerColors[category][playerIndex] = gDatabase.getPlayerColor(static_cast<BPlayerColorCategory>(category), mPlayerColorIndices[category][playerIndex]);

   gConsoleOutput.status("Reloaded player colors");
}
#endif

#ifdef UNITOFFENDERS
//==============================================================================
//==============================================================================
void BWorld::addUnitOffender(const BEntityID entityID, double time)
{
   bool inserted=false;
   for (uint i=0; i < mUnitOffenders.getSize(); i++)
   {
      if (mUnitOffenders[i].mID == entityID)
      {
         return;
      }
      if (mUnitOffenders[i].mTime < time)
      {
         mUnitOffenders.insertAtIndex(BOffenderEntry(entityID, time), i);
         inserted=true;
         break;
      }
   }
   //If we're under size, just add it.
   if (!inserted && (i < cNumberUnitOffenders-1))
      mUnitOffenders.add(BOffenderEntry(entityID, time));
   //If we're over the size, remove the last one since it will be the smallest one.
   if (mUnitOffenders.getSize() > cNumberUnitOffenders)
      mUnitOffenders.removeIndex(cNumberUnitOffenders);
}

#endif


//==============================================================================
//==============================================================================
void BWorld::updatePlayers()
{
   #ifdef DECOUPLED_UPDATE
   if (!getFlagGameOver() && (!gDecoupledUpdate || mSubUpdateSectionItem == -1))
   #else
   if (!getFlagGameOver())
   #endif
   {
      for(long i=1; i<mPlayers.getNumber(); i++)
      {
         if (mPlayers[i]->getFlagDefeatedDestroy())
         {
            // Kill all units owned by defeated the player.
            BEntityHandle handle=cInvalidObjectID;
            for (BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
            {
               if (pUnit->getPlayerID() != i || !pUnit->isAlive())
                  continue;

               if (pUnit->getProtoObject() == NULL || !pUnit->getProtoObject()->getFlagUngarrisonToGaia())
                  pUnit->kill(false);
            }
            mPlayers[i]->setFlagDefeatedDestroy(false);
         }
      }
   }

   #ifdef DECOUPLED_UPDATE
   long startPlayer = (gDecoupledUpdate ? mSubUpdateSectionItem + 1 : 0);
   #else
   long startPlayer = 0;
   #endif
   for(long i=startPlayer; i<mPlayers.getNumber(); i++)
   {
      SCOPEDSAMPLE(PlayerUpdate);
      mPlayers[i]->update(mLastUpdateLengthFloat);
      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         mSubUpdateSectionItem = i;
         SUBUPDATECANRENDERBREAK
      }
      #endif
   }
   #ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate && i >= mPlayers.getNumber() - 1)
      SUBUPDATENEXT
   #endif
}


//==============================================================================
//==============================================================================
void BWorld::updateWorldSoundSwitch()
{
   BCueIndex switchEvent = cInvalidCueIndex;
   BCueIndex musicSwitchEvent = cInvalidCueIndex;
   switch(mWorldID)
   {
      case cWorldHarvest:
         switchEvent =        gSoundManager.getCueIndexByEnum(BSoundManager::cSoundSwitchHarvest);
         musicSwitchEvent =   gSoundManager.getCueIndexByEnum(BSoundManager::cSoundMusicSwitchHarvest);
         break;
      case cWorldArcadia:
         switchEvent =        gSoundManager.getCueIndexByEnum(BSoundManager::cSoundSwitchArcadia);
         musicSwitchEvent =   gSoundManager.getCueIndexByEnum(BSoundManager::cSoundMusicSwitchArcadia);
         break;
      case cWorldSWI:
         switchEvent =        gSoundManager.getCueIndexByEnum(BSoundManager::cSoundSwitchSWI);
         musicSwitchEvent =   gSoundManager.getCueIndexByEnum(BSoundManager::cSoundMusicSwitchSWI);
         break;
      case cWorldSWE:
         switchEvent =        gSoundManager.getCueIndexByEnum(BSoundManager::cSoundSwitchSWE);
         musicSwitchEvent =   gSoundManager.getCueIndexByEnum(BSoundManager::cSoundMusicSwitchSWE);
         break;
   }
   
   if(switchEvent != cInvalidCueIndex)
      gSoundManager.playCue(switchEvent);
   if(musicSwitchEvent != cInvalidCueIndex && !gConfig.isDefined(cConfigNoMusic))
      gSoundManager.playCue(musicSwitchEvent);
}

//==============================================================================
//==============================================================================
bool BWorld::adjustGameOverCountdown(float elapsed)
{  
   if(mGameOverCountdown <= cFloatCompareEpsilon) //-- Early out if its already done.
      return true;

   //-- Tick the timer
   mGameOverCountdown -= elapsed; 

   //-- Is the timer done?
   if(mGameOverCountdown <= cFloatCompareEpsilon)
   {
      //-- We've hit the end of the timer, the game is over, stop all the world sounds
      gUIGame.playSound(BSoundManager::cSoundStopAllExceptMusic);
      return true;
   }

   return false;
}

//==============================================================================
double BWorld::getActualRealtime() const
{
   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);

   int64 delta;
   if(mLastUpdateRealtime==0)
      delta = 0;
   else
      delta = (int64)(time.QuadPart - mLastUpdateRealtime);   
   return mTotalRealtime + delta/mLastTimerFrequency;
}

//==============================================================================
DWORD BWorld::getSubGametime() const
{  
   if (!gEnableSubUpdating)
      return getGametime();
   else
   {
      DWORD startingGameTime = mGametime - mLastUpdateLength;
      float updateCompletion = getUpdateCompletion(mFirstSubUpdate);
      DWORD subUpdateTime = getSubUpdateTotalTimeInMsecs();
      DWORD result = startingGameTime + (DWORD)(updateCompletion * (float)subUpdateTime);
#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigSubUpdTrace))
         trace("  mGameTime=%6u, startingGameTime=%6u, mLastUpdateLength=%4u, updateCompletion=%6.4f, subUpdateTime=%4u, subGameTime=%6u, mFirstSubUpdate=%5u, mSubUpdate=%5u, mUpdateNumber=%5u", 
            mGametime, startingGameTime, mLastUpdateLength, updateCompletion, subUpdateTime, result, mFirstSubUpdate, mSubUpdate, mUpdateNumber);
#endif
      return result;
   }
}

//==============================================================================
double BWorld::getSubGametimeFloat() const
{
   if (!gEnableSubUpdating)
      return getGametimeFloat();
   else
      return getSubGametime() / 1000.0f;
}

//==============================================================================
// This method returns the percent complete (from 0-1.0) we are in our total sim update
// based on a given sub-update starting point
//==============================================================================
float BWorld::getUpdateCompletion(DWORD startingFromSubUpdate) const
{   
   if (gEnableSubUpdating)
   {
      if (startingFromSubUpdate < mSubUpdateEnabledNumber)
         return 1.0f;

      #ifdef DECOUPLED_UPDATE
      if (gDecoupledUpdate)
      {
         // Special case first update where there is no previous data.
         if(mDecoupledUpdateStartingSubUpdateCurrent == 0)
            return(1.0f);
            
         // Get current time.
         LARGE_INTEGER currTime;
         QueryPerformanceCounter(&currTime);
         double currRealTime = double(currTime.QuadPart/mLastTimerFrequency);
         
         // Sanity check that previous really is previous.
         BASSERT(mDecoupledUpdateStartingSubUpdatePrevious < mDecoupledUpdateStartingSubUpdateCurrent);
         
         // If the subupdate we're looking for is before the start of the previous list, somehow we're looking for
         // a subupdate from further in the past, which is odd.
         if(startingFromSubUpdate < mDecoupledUpdateStartingSubUpdatePrevious)
         {
            // jce [10/28/2008] -- some things never update (like the sky) so for now at least I'm letting that be ok and not
            // asserting in this case if we're asking for the the 0th subupdate
            if(startingFromSubUpdate != 0)
               BFAIL("subupdate too old");
               
            return(1.0f);
         }
         
         // If we're looking for a subupdate in the current list, grab it.
         if (startingFromSubUpdate >= mDecoupledUpdateStartingSubUpdateCurrent)
         {
            DWORD index = startingFromSubUpdate - mDecoupledUpdateStartingSubUpdateCurrent;
            
            if (index < mDecoupledUpdateTimesCurrent->size())
            {
               float pct = (float)(currRealTime - mDecoupledUpdateTimesCurrent->get(index))/mDecoupledUpdateAvgUpdateTime;
               return min(1.0f, pct);
            }
            else
               return (float)0.0f;
         }

         // Otherwise, we're in the previous update's list.
         DWORD index = startingFromSubUpdate - mDecoupledUpdateStartingSubUpdatePrevious;

         if (index < mDecoupledUpdateTimesPrevious->size())
         {
            float pct = (float)(currRealTime - mDecoupledUpdateTimesPrevious->get(index))/mDecoupledUpdateAvgUpdateTime;
            return min(1.0f, pct);
         }
         else
         {
            // If we get here it's because somehow a subupdate falls after the previous list and before the current one,
            // which is just broken, but we'll call it 1.0.
            BFAIL("subupdate is between lists");
            return(1.0f);
         }
      }
      else
      #endif
      {
         DWORD subUpdatesRun = mSubUpdate - startingFromSubUpdate;
         DWORD cappedAmount = max(1, min(subUpdatesRun, getAmountOfSubUpdates()));

         if (subUpdatesRun == 1)
            return (float)cappedAmount / (float)getAmountOfSubUpdates();
         else
            return (float)cappedAmount / (float)getAmountOfSubUpdates();
      }
   }
   else
      return 1.0f;
}

//==============================================================================
//==============================================================================
double BWorld::getSubTotalRealtime() const
{ 
   if (!gEnableSubUpdating)
      return getTotalRealtime();
   else
      return mSubTotalRealtime; 
}

//==============================================================================
//==============================================================================
float BWorld::getLastSubUpdateLengthFloat() const
{ 
   if (!gEnableSubUpdating || gDecoupledUpdate)
      return getLastUpdateLengthFloat();
   else
      return getSubUpdateTimeInMsecs() / 1000.0f; 
}

//==============================================================================
//==============================================================================
int64 BWorld::getLastSubUpdateRealtime() const 
{ 
   if (!gEnableSubUpdating)
      return getLastUpdateRealtime();
   else
      return mLastSubUpdateRealtime; 
}

//==============================================================================
// amount of sub-updates in a single update
//==============================================================================
DWORD BWorld::getAmountOfSubUpdates() const
{ 
   if (gEnableSubUpdating && !gDecoupledUpdate)
      return cAmountOfSubUpdates;
   else
      return 1;
}

//==============================================================================
//==============================================================================
long BWorld::getSubUpdateTimeInMsecs() const
{ 
   if (gEnableSubUpdating && !gDecoupledUpdate)
   {
      if (mDynamicSubUpdateTime)
         return mAdjustedSubUpdateTime;
      else
         return cSubUpdateTimeInMsecs;
   }
   #ifdef DECOUPLED_UPDATE
   else if (gDecoupledUpdate)
      return (long)(mDecoupledUpdateLastSubElapsed * 1000.0);
   #endif
   else
      return (long)mLastUpdateLength;
}

//==============================================================================
//==============================================================================
long BWorld::getSubUpdateTotalTimeInMsecs() const
{ 
   if (gEnableSubUpdating && !gDecoupledUpdate)
      return getAmountOfSubUpdates() * getSubUpdateTimeInMsecs();
   else
      return (long)mLastUpdateLength;
}

//==============================================================================
//==============================================================================
bool BWorld::save(BStream* pStream, int saveType) const
{
   gSaveGame.logStats("WorldStart");

   // Exploration groups
   GFWRITEVAR(pStream, int16, mNumExplorationGroups);
   for (BExplorationGroupHashMap::const_iterator it = mExplorationGroups.begin(); it != mExplorationGroups.end(); ++it)
   {
      GFWRITEVAR(pStream, int16, it->first);
      const BExplorationGroupEntry& entry = it->second;
      GFWRITEARRAY(pStream, BEntityID, entry.objects, uint8, 100);
      GFWRITEARRAY(pStream, BTeamID, entry.triggeredTeams, uint8, 6);
   }
   GFWRITEVAL(pStream, int16, -1);
   GFWRITEARRAY(pStream, BExplorationGroupTimerEntry, mActiveExplorationGroups, uint8, 200);
   GFWRITEMARKER(pStream, cSaveMarkerWorld1);

   // Players
   int count = mPlayers.getNumber();
   for (int i=0; i<count; i++)
   {
      if (!mPlayers[i]->save(pStream, saveType))
         return false;
   }
   GFWRITEMARKER(pStream, cSaveMarkerPlayers);
   gSaveGame.logStats("Players");

   // Player colors
   GFWRITEVAL(pStream, uint8, cMaximumSupportedPlayers);
   GFWRITEVAL(pStream, uint8, cMaxPlayerColorCategories);
   for (uint i=0; i<cMaxPlayerColorCategories; i++)
   {
      for (uint j=0; j<cMaximumSupportedPlayers; j++)
      {
         const BPlayerColor& color = mPlayerColors[i][j];
         GFWRITEVAR(pStream, DWORD, color.mObjects);
         GFWRITEVAR(pStream, DWORD, color.mCorpse);
         GFWRITEVAR(pStream, DWORD, color.mSelection);
         GFWRITEVAR(pStream, DWORD, color.mMinimap);
         GFWRITEVAR(pStream, DWORD, color.mUI);
         GFWRITEVAR(pStream, uint8, mPlayerColorIndices[i][j]);
      }
   }
   GFWRITEMARKER(pStream, cSaveMarkerWorld2);

   GFWRITEFREELIST(pStream, saveType, BSimOrder, BSimOrder::mFreeList, uint16, 20000);
   GFWRITEMARKER(pStream, cSaveMarkerSimOrder);

   GFWRITEFREELIST(pStream, saveType, BUnitOpp, BUnitOpp::mFreeList, uint16, 20000);
   GFWRITEMARKER(pStream, cSaveMarkerUnitOpp);

   GFWRITEFREELIST(pStream, saveType, BPathMoveData, BPathMoveData::mFreeList, uint16, 20000);
   GFWRITEMARKER(pStream, cSaveMarkerPathMoveData);

   if (!gActionManager.preSave(pStream, saveType))
      return false;
   gSaveGame.logStats("Actions1");

   //BObjectManager* mpObjectManager;

   // Units
   uint highWaterMark = getUnitsHighWaterMark();
   bool inUse = false;
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      const BUnit* pUnit = mpObjectManager->getUnitByIndex(i, inUse);
      BEntityID id = pUnit->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pUnit)
   }
   GFWRITEMARKER(pStream, cSaveMarkerUnits);
   gSaveGame.logStats("Units");

   // Teams
   count = mTeams.getNumber();
   for (int i=0; i<count; i++)
   {
      BTeam* pTeam = mTeams[i];
      BASSERT(pTeam);
      if (!pTeam->save(pStream, saveType))
         return false;
   }
   GFWRITEMARKER(pStream, cSaveMarkerTeams);
   gSaveGame.logStats("Teams");

   // Squads
   highWaterMark = getSquadsHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      const BSquad* pSquad = mpObjectManager->getSquadByIndex(i, inUse);
      BEntityID id = pSquad->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pSquad)
   }
   GFWRITEMARKER(pStream, cSaveMarkerSquads);
   gSaveGame.logStats("Squads");

   // Armies
   highWaterMark = getArmiesHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      BArmy* pArmy = mpObjectManager->getArmyByIndex(i, inUse);
      BEntityID id = pArmy->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pArmy)
   }
   GFWRITEMARKER(pStream, cSaveMarkerArmies);
   gSaveGame.logStats("Armies");

   // Platoons
   highWaterMark = getPlatoonsHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      BPlatoon* pPlatoon = mpObjectManager->getPlatoonByIndex(i, inUse);
      BEntityID id = pPlatoon->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pPlatoon)
   }
   GFWRITEMARKER(pStream, cSaveMarkerPlatoons);
   gSaveGame.logStats("Platoons");

   // Dopples
   highWaterMark = getDopplesHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      BDopple* pDopple = mpObjectManager->getDoppleByIndex(i, inUse);
      BEntityID id = pDopple->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pDopple)
   }
   GFWRITEMARKER(pStream, cSaveMarkerDopples);
   gSaveGame.logStats("Dopples");

   // Projectiles
   GFWRITEFREELIST(pStream, saveType, BPerturbanceData, BPerturbanceData::mFreeList, uint16, 20000);
   GFWRITEFREELIST(pStream, saveType, BStickData, BStickData::mFreeList, uint16, 20000);
   highWaterMark = getProjectilesHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      BProjectile* pProjectile = mpObjectManager->getProjectileByIndex(i, inUse);
      BEntityID id = pProjectile->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pProjectile)
   }
   GFWRITEMARKER(pStream, cSaveMarkerProjectiles);
   gSaveGame.logStats("Projectiles");

   // Objects
   highWaterMark = getObjectsHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   for (uint i=0; i<highWaterMark; i++)
   {
      BObject* pObject = mpObjectManager->getObjectByIndex(i, inUse);
      BEntityID id = pObject->getID();
      GFWRITEVAR(pStream, BEntityID, id);
      GFWRITEVAR(pStream, bool, inUse);
      if (inUse)
         GFWRITECLASSPTR(pStream, saveType, pObject)
   }
   GFWRITEMARKER(pStream, cSaveMarkerObjects);
   gSaveGame.logStats("Objects");

   // Air Spots
   highWaterMark = mpObjectManager->getAirSpotHighWaterMark();
   GFWRITEVAR(pStream, uint, highWaterMark);
   uint airSpotIndex = 0;
   BAirSpot* pAirSpot = mpObjectManager->getNextClaimedAirSpot(airSpotIndex);
   while (pAirSpot)
   {
      GFWRITEVAR(pStream, uint, airSpotIndex);
      GFWRITECLASSPTR(pStream, saveType, pAirSpot);
      airSpotIndex++;
      pAirSpot = mpObjectManager->getNextClaimedAirSpot(airSpotIndex);
   }
   GFWRITEVAL(pStream, uint, UINT_MAX);
   GFWRITEMARKER(pStream, cSaveMarkerAirSpots);
   gSaveGame.logStats("AirSpots");

   GFWRITECLASS(pStream, saveType, gFatalityManager);
   gSaveGame.logStats("Fatalities");

   GFWRITECLASS(pStream, saveType, gActionManager);
   gSaveGame.logStats("Actions2");

   /*
   BCommandManager*              mpCommandManager;
   BWorldSoundManager*           mpWorldSoundManager;
   BPhysicsWorld*                mpPhysicsWorld;
   */

   GFWRITECLASSPTR(pStream, saveType, mpObjectiveManager);
   GFWRITEMARKER(pStream, cSaveMarkerObjectiveManager);
   gSaveGame.logStats("ObjectiveManager");

   //BChatManager*                 mpChatManager;
   GFWRITECLASSPTR(pStream, saveType, mpChatManager);

   GFWRITECLASSPTR(pStream, saveType, mpHintManager);

   /*
   BDesignObjectManager*         mpDesignObjectManager;
   BBattleManager*               mpBattleManager;
   */

   GFWRITECLASSPTR(pStream, saveType, mpPowerManager);
   GFWRITECLASSPTR(pStream, saveType, mpAIManager);

   /*
   BEntityGrouper*               mEntityGrouper;
   */

   GFWRITECLASSPTR(pStream, saveType, mpPathingLimiter);

   // Times
   GFWRITEVAR(pStream, DWORD, mUpdateNumber);
   GFWRITEVAR(pStream, int64, mLastUpdateTime);
   GFWRITEVAR(pStream, DWORD, mLastUpdateLength);
   GFWRITEVAR(pStream, float, mLastUpdateLengthFloat);
   GFWRITEVAR(pStream, DWORD, mGametime);
   GFWRITEVAR(pStream, DWORD, mSubUpdate);
   GFWRITEVAR(pStream, DWORD, mFirstSubUpdate);
   GFWRITEVAR(pStream, int64, mSubUpdateBeganTime);
   GFWRITEVAR(pStream, int64, mSubUpdateLastTime);
   GFWRITEVAR(pStream, double, mGametimeFloat);
   GFWRITEVAR(pStream, double, mTotalRealtime);
   GFWRITEVAR(pStream, double, mSubTotalRealtime);
   GFWRITEVAR(pStream, int64, mLastUpdateRealtime);
   GFWRITEVAR(pStream, int64, mLastSubUpdateRealtime);
   GFWRITEVAR(pStream, double, mLastUpdateRealtimeDelta);
   GFWRITEVAR(pStream, double, mLastTimerFrequency);
   GFWRITEVAR(pStream, float, mAccumulatedTimeError);

   GFWRITEVAR(pStream, long, mFogStartX);
   GFWRITEVAR(pStream, long, mFogStartZ);
   GFWRITEVAR(pStream, long, mFogEndZ);

   /*
   BSmallDynamicSimArray<BLightVisualInstance*> mLights;
   BCinematicManager*            mpCinematicManager;
   BTransitionManager*           mpTransitionManager;
   BVisual*                      mpSkyVisual;
   int                           mBuildingPlacementDecal;
   long                          mTempObjectID;
   */

   GFWRITEVAL(pStream, uint8, cMaximumSupportedTeams);
   for (uint i=0; i<cMaximumSupportedTeams; i++)
      for (uint j=0; j<cMaximumSupportedTeams; j++)
         GFWRITEVAR(pStream, BRelationType, mTeamRelationType[i][j])
   GFWRITEMARKER(pStream, cSaveMarkerWorld3);

   /*
   BEntityIDArray                mCachedUnitsToTrack;
   BObstructionNodePtrArray      mCachedObstructions;
   BObstructionNodePtrArray      mExpandedObstructions;
   BObstructionNodePtrArray      mCachedTerrainObstructions;
   BObstructionNodeArray         mExpandedObstructionData;
   BPhysicsBreakOffObjectsArray  mPhysicsBreakOffObjects;
   BLightWeightMutex             mPhysicsBreakOffObjectMutex;
   */

   GFWRITECLASSARRAY(pStream, saveType, mCustomCommands, uint8, 200);
   GFWRITEVAR(pStream, int, mNextCustomCommandID);
   GFWRITEMARKER(pStream, cSaveMarkerWorld4);

   GFWRITECLASS(pStream, saveType, *mpStoredEventTagManager);
   GFWRITEMARKER(pStream, cSaveMarkerStoredAnimEventManager);

   GFWRITEVAR(pStream, float, mSimBoundsMinX);
   GFWRITEVAR(pStream, float, mSimBoundsMinZ);
   GFWRITEVAR(pStream, float, mSimBoundsMaxX);
   GFWRITEVAR(pStream, float, mSimBoundsMaxZ);
   GFWRITEPTR(pStream, sizeof(BVector)*4, mOutsideOfBoundsBlockerCenter);
   GFWRITEPTR(pStream, sizeof(BVector)*4, mOutsideOfBoundsBlockerSize);

   GFWRITEVAR(pStream, uint, mWorldID);
   GFWRITEVAR(pStream, float, mGameOverCountdown);
   GFWRITEVAR(pStream, BEntityHandle, mLastSubUpdateUnit);

   /*
   BSmallDynamicSimArray<BTactic*> mTransformedTactics;
   BSmallDynamicSimArray<BTalkingHead*> mTalkingHeads;
   */

   GFWRITEVAL(pStream, uint8, mPlayerColorCategory);

   //BTimer mSubUpdateEnabledTimer;
   GFWRITEVAR(pStream, bool, gEnableSubUpdating);
   GFWRITEVAR(pStream, DWORD, mSubUpdateEnabledNumber);
   GFWRITEVAR(pStream, bool, mDynamicSubUpdating);
   GFWRITEVAR(pStream, bool, mAlternateSubUpdating);
   GFWRITEVAR(pStream, bool, mDynamicSubUpdateTime);
   GFWRITEVAR(pStream, bool, mNextEnableSubUpdating);
   #ifdef DECOUPLED_UPDATE
   GFWRITEVAR(pStream, bool, mNextDecoupledUpdate);
   #else
   GFWRITEVAL(pStream, bool, false);
   #endif
   GFWRITEVAR(pStream, bool, mNextAlternateSubUpdating);
   GFWRITEVAR(pStream, bool, mNextDynamicSubUpdateTime);
   GFWRITEVAR(pStream, bool, mChangeSubUpdatePendnig);

   GFWRITEVAR(pStream, bool, mFirstUpdate);

   GFWRITEBITBOOL(pStream, mFlagUnitCache);
   GFWRITEBITBOOL(pStream, mFlagObstructionCache);
   GFWRITEBITBOOL(pStream, mFlagQuickBuild);
   GFWRITEBITBOOL(pStream, mFlagGameOver);
   GFWRITEBITBOOL(pStream, mFlagMultiplayerSim);
   GFWRITEBITBOOL(pStream, mFlagCoop);
   GFWRITEBITBOOL(pStream, mFlagPlayableBounds);
   GFWRITEBITBOOL(pStream, mFlagOutsideBoundsBlocked);
   GFWRITEBITBOOL(pStream, mFlagNoFogSim);
   GFWRITEBITBOOL(pStream, mFlagNoFogVis);
   GFWRITEBITBOOL(pStream, mFlagShowSkirt);
   GFWRITEBITBOOL(pStream, mFlagAllVisible);
   GFWRITEBITBOOL(pStream, mFlagSkipGameOverCheck);
   GFWRITEBITBOOL(pStream, mFlagStartedUpdate);

   GFWRITEMARKER(pStream, cSaveMarkerWorld5);
   gSaveGame.logStats("World2");

   // Corpse manager
   GFWRITECLASS(pStream, saveType, gCorpseManager);
   gSaveGame.logStats("CorpseManager");

   // General events
   GFWRITECLASS(pStream, saveType, gGeneralEventManager);
   GFWRITEMARKER(pStream, cSaveMarkerGeneralEvents);
   gSaveGame.logStats("GeneralEvents");

   // Triggers
   GFWRITECLASS(pStream, saveType, gTriggerManager);
   GFWRITEMARKER(pStream, cSaveMarkerTriggers);
   gSaveGame.logStats("Triggers");

   // Visibilty
   GFWRITECLASS(pStream, saveType, gVisibleMap);
   GFWRITEMARKER(pStream, cSaveMarkerVisibilty);
   gSaveGame.logStats("Visibilty");

   // Score Manager
   GFWRITECLASS(pStream, saveType, gScoreManager);
   GFWRITEMARKER(pStream, cSaveMarkerScoreManager);

   GFWRITECLASS(pStream, saveType, gEntityScheduler);
   GFWRITEMARKER(pStream, cSaveMarkerEntityScheduler);

   GFWRITECLASS(pStream, saveType, gCollectiblesManager);
   GFWRITEMARKER(pStream, cSaveMarkerCollectiblesManager);

   GFWRITEVAR(pStream, long, mLowDetailAnimRefCount);
   GFWRITEVECTOR(pStream, mTerrainMax);

   //MusicManager state
   bool musicManagerDisabled = true;
   if( mpWorldSoundManager && mpWorldSoundManager->getMusicManager() )
      musicManagerDisabled = mpWorldSoundManager->getMusicManager()->getDisabledVariable();
   GFWRITEVAR(pStream, bool, musicManagerDisabled);


   // Write out preloaded talking heads
   long numPreloadedTalkingHeads = 0;
   long numTalkingHeads = mTalkingHeads.getNumber();

   // count number of preloaded talking heads
   for (int i=0; i<numTalkingHeads; i++)
   {
      if (mTalkingHeads[i]->mPreload)
      {
         numPreloadedTalkingHeads++;
      }
   }

   GFWRITEVAR(pStream, long, numPreloadedTalkingHeads);

   for (int i=0; i<numTalkingHeads; i++)
   {
      BTalkingHead* pTalkingHead = mTalkingHeads[i];
      if (pTalkingHead->mPreload)
      {
         GFWRITESTRING(pStream, BString, pTalkingHead->mFilename, 200);
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BWorld::load(BStream* pStream, int saveType)
{
   // Exploration groups
   GFREADVAR(pStream, int16, mNumExplorationGroups);
   int16 index = -1;
   int16 loaded = 0;
   for (;;)
   {
      GFREADVAR(pStream, int16, index);
      if (index == -1)
         break;
      if (loaded >= mNumExplorationGroups || index > mNumExplorationGroups)
         return false;
      loaded++;
      BExplorationGroupEntry entry;
      GFREADARRAY(pStream, BEntityID, entry.objects, uint8, 100);
      GFREADARRAY(pStream, BTeamID, entry.triggeredTeams, uint8, 6);
      mExplorationGroups.insert(index, entry);
   }
   GFREADARRAY(pStream, BExplorationGroupTimerEntry, mActiveExplorationGroups, uint8, 200);
   GFREADMARKER(pStream, cSaveMarkerWorld1);

   // Players
   int count = mPlayers.getNumber();
   for (int i=0; i<count; i++)
   {
      if (!mPlayers[i]->load(pStream, saveType))
         return false;
   }
   GFREADMARKER(pStream, cSaveMarkerPlayers);

   if (BWorld::mGameFileVersion < 2)
   {
      // Teams - at version 2, this was moved after units 
      // due to a visibility / saved units cleanup pass
      count = mTeams.getNumber();
      for (int i=0; i<count; i++)
      {
         if (!mTeams[i]->load(pStream, saveType))
            return false;
      }
      GFREADMARKER(pStream, cSaveMarkerTeams);
   }

   // Player colors
   uint maxPlayers = 0;
   uint maxColors = 0;
   GFREADVAL(pStream, uint8, uint, maxPlayers);
   GFREADVAL(pStream, uint8, uint, maxColors);
   BPlayerColor color;
   uint8 colorIndex;
   for (uint i=0; i<maxColors; i++)
   {
      for (uint j=0; j<maxPlayers; j++)
      {
         BPlayerColor* pColor;
         uint8* pColorIndex;
         if (i < cMaxPlayerColorCategories && j < cMaximumSupportedPlayers)
         {
            pColor = &(mPlayerColors[i][j]);
            pColorIndex = &(mPlayerColorIndices[i][j]);
         }
         else
         {
            pColor = &color;
            pColorIndex = &colorIndex;
         }
         GFREADVAR(pStream, DWORD, pColor->mObjects);
         GFREADVAR(pStream, DWORD, pColor->mCorpse);
         GFREADVAR(pStream, DWORD, pColor->mSelection);
         GFREADVAR(pStream, DWORD, pColor->mMinimap);
         GFREADVAR(pStream, DWORD, pColor->mUI);
         GFREADVAR(pStream, uint8, *pColorIndex);
      }
   }
   GFREADMARKER(pStream, cSaveMarkerWorld2);

   GFREADFREELIST(pStream, saveType, BSimOrder, BSimOrder::mFreeList, uint16, 20000);
   GFREADMARKER(pStream, cSaveMarkerSimOrder);

   GFREADFREELIST(pStream, saveType, BUnitOpp, BUnitOpp::mFreeList, uint16, 20000);
   GFREADMARKER(pStream, cSaveMarkerUnitOpp);

   GFREADFREELIST(pStream, saveType, BPathMoveData, BPathMoveData::mFreeList, uint16, 20000);
   GFREADMARKER(pStream, cSaveMarkerPathMoveData);

   if (!gActionManager.preLoad(pStream, saveType))
      return false;

   //BObjectManager* mpObjectManager;

   BObjectManager* pObjectMgr = gWorld->getObjectManager();
   uint highWaterMark;
   BObjectCreateParms parms;
   uint loadedCount;
   bool inUse;

   // Units
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initUnits(highWaterMark);
   parms.mType = BEntity::cClassTypeUnit;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BUnit* pUnit = (BUnit*)mpObjectManager->createObject(parms);
         if (!pUnit)
            return false;
         GFREADCLASSPTR(pStream, saveType, pUnit)
         BASSERT(pUnit->getID()==parms.mSaveEntityID);
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BUnit* pUnit = (BUnit*)mpObjectManager->createObject(parms);
            if (!pUnit)
               return false;
            GFREADCLASSPTR(pStream, saveType, pUnit)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BUnit* pUnit = mpObjectManager->getUnitByIndex(i, inUse);
            BASSERT(pUnit);
            BASSERT(!inUse);
            if (pUnit)
               pUnit->setID(parms.mSaveEntityID);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerUnits);
   
   if (BWorld::mGameFileVersion >= 2)
   {
      count = mTeams.getNumber();
      for (int i=0; i<count; i++)
      {
         if (!mTeams[i]->load(pStream, saveType))
            return false;
      }
      GFREADMARKER(pStream, cSaveMarkerTeams);
   }

   // Squads
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initSquads(highWaterMark);
   parms.mType = BEntity::cClassTypeSquad;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BSquad* pSquad = (BSquad*)mpObjectManager->createObject(parms);
         if (!pSquad)
            return false;
         GFREADCLASSPTR(pStream, saveType, pSquad)
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BSquad* pSquad = (BSquad*)mpObjectManager->createObject(parms);
            if (!pSquad)
               return false;
            GFREADCLASSPTR(pStream, saveType, pSquad)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BSquad* pSquad = mpObjectManager->getSquadByIndex(i, inUse);
            BASSERT(pSquad);
            BASSERT(!inUse);
            if (pSquad)
            {
               pSquad->setID(parms.mSaveEntityID);
               BASSERT(!inUse);
            }
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerSquads);

   // Armies
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initArmies(highWaterMark);
   parms.mType = BEntity::cClassTypeArmy;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BArmy* pArmy = (BArmy*)mpObjectManager->createObject(parms);
         if (!pArmy)
            return false;
         GFREADCLASSPTR(pStream, saveType, pArmy)
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BArmy* pArmy = (BArmy*)mpObjectManager->createObject(parms);
            if (!pArmy)
               return false;
            GFREADCLASSPTR(pStream, saveType, pArmy)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BArmy* pArmy = mpObjectManager->getArmyByIndex(i, inUse);
            BASSERT(pArmy);
            BASSERT(!inUse);
            if (pArmy)
               pArmy->setID(parms.mSaveEntityID);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerArmies);

   // Platoons
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initPlatoons(highWaterMark);
   parms.mType = BEntity::cClassTypePlatoon;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BPlatoon* pPlatoon = (BPlatoon*)mpObjectManager->createObject(parms);
         if (!pPlatoon)
            return false;
         GFREADCLASSPTR(pStream, saveType, pPlatoon)
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BPlatoon* pPlatoon = (BPlatoon*)mpObjectManager->createObject(parms);
            if (!pPlatoon)
               return false;
            GFREADCLASSPTR(pStream, saveType, pPlatoon)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BPlatoon* pPlatoon = mpObjectManager->getPlatoonByIndex(i, inUse);
            BASSERT(pPlatoon);
            BASSERT(!inUse);
            if (pPlatoon)
               pPlatoon->setID(parms.mSaveEntityID);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerPlatoons);

   // Dopples
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initDopples(highWaterMark);
   parms.mType = BEntity::cClassTypeDopple;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BDopple* pDopple = (BDopple*)mpObjectManager->createObject(parms);
         if (!pDopple)
            return false;
         GFREADCLASSPTR(pStream, saveType, pDopple)
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BDopple* pDopple = (BDopple*)mpObjectManager->createObject(parms);
            if (!pDopple)
               return false;
            GFREADCLASSPTR(pStream, saveType, pDopple)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BDopple* pDopple = mpObjectManager->getDoppleByIndex(i, inUse);
            BASSERT(pDopple);
            BASSERT(!inUse);
            if (pDopple)
               pDopple->setID(parms.mSaveEntityID);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerDopples);

   // Projectiles
   GFREADFREELIST(pStream, saveType, BPerturbanceData, BPerturbanceData::mFreeList, uint16, 20000);
   GFREADFREELIST(pStream, saveType, BStickData, BStickData::mFreeList, uint16, 20000);
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 20000);
   pObjectMgr->initProjectiles(highWaterMark);
   parms.mType = BEntity::cClassTypeProjectile;
   parms.mFromSave = true;
   if (mGameFileVersion < 10)
   {
      GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      loadedCount = 0;
      while (parms.mSaveEntityID != cInvalidObjectID)
      {
         loadedCount++;
         if (loadedCount > highWaterMark)
            return false;
         BProjectile* pProjectile = (BProjectile*)mpObjectManager->createObject(parms);
         if (!pProjectile)
            return false;
         GFREADCLASSPTR(pStream, saveType, pProjectile)
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
      }
   }
   else
   {
      for (uint i=0; i<highWaterMark; i++)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         GFREADVAR(pStream, bool, inUse);
         if (inUse)
         {
            BProjectile* pProjectile = (BProjectile*)mpObjectManager->createObject(parms);
            if (!pProjectile)
               return false;
            GFREADCLASSPTR(pStream, saveType, pProjectile)
         }
         else
         {
            BASSERT(parms.mSaveEntityID.getIndex() == i);
            BProjectile* pProjectile = mpObjectManager->getProjectileByIndex(i, inUse);
            BASSERT(pProjectile);
            BASSERT(!inUse);
            if (pProjectile)
               pProjectile->setID(parms.mSaveEntityID);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerProjectiles);

   // Objects
   if (mGameFileVersion >= 7)
   {
      GFREADVAR(pStream, uint, highWaterMark);
      GFVERIFYCOUNT(highWaterMark, 20000);
      pObjectMgr->initObjects(highWaterMark);
      parms.mType = BEntity::cClassTypeObject;
      parms.mFromSave = true;
      if (mGameFileVersion < 10)
      {
         GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         loadedCount = 0;
         while (parms.mSaveEntityID != cInvalidObjectID)
         {
            loadedCount++;
            if (loadedCount > highWaterMark)
               return false;
            BObject* pObject = (BObject*)mpObjectManager->createObject(parms);
            if (!pObject)
               return false;
            GFREADCLASSPTR(pStream, saveType, pObject)
            GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
         }
      }
      else
      {
         for (uint i=0; i<highWaterMark; i++)
         {
            GFREADVAR(pStream, BEntityID, parms.mSaveEntityID);
            GFREADVAR(pStream, bool, inUse);
            if (inUse)
            {
               BObject* pObject = (BObject*)mpObjectManager->createObject(parms);
               if (!pObject)
                  return false;
               GFREADCLASSPTR(pStream, saveType, pObject)
            }
            else
            {
               BASSERT(parms.mSaveEntityID.getIndex() == i);
               BObject* pObject = mpObjectManager->getObjectByIndex(i, inUse);
               BASSERT(pObject);
               BASSERT(!inUse);
               if (pObject)
                  pObject->setID(parms.mSaveEntityID);
            }
         }
      }
      GFREADMARKER(pStream, cSaveMarkerObjects);
   }

   // Air Spots
   GFREADVAR(pStream, uint, highWaterMark);
   GFVERIFYCOUNT(highWaterMark, 2000);
   pObjectMgr->initAirSpots(highWaterMark);
   uint airSpotIndex;
   GFREADVAR(pStream, uint, airSpotIndex);
   loadedCount = 0;
   while (airSpotIndex != UINT_MAX)
   {
      loadedCount++;
      if (loadedCount > highWaterMark)
         return false;
      BAirSpot* pAirSpot = mpObjectManager->createClaimedAirSpotAtIndex(airSpotIndex);
      if (!pAirSpot)
         return false;
      GFREADCLASSPTR(pStream, saveType, pAirSpot)
      GFREADVAR(pStream, uint, airSpotIndex);
   }
   GFREADMARKER(pStream, cSaveMarkerAirSpots);

   // Deleted scenario objects
   if (mGameFileVersion >= 3 && mGameFileVersion < 7)
   {
      BEntityIDArray killedScenarioObjects;
      GFREADARRAY(pStream, BEntityID, killedScenarioObjects, uint16, 10000);
      uint killedCount = killedScenarioObjects.size();
      for (uint i=0; i<killedCount; i++)
      {
         BEntityID killedID = killedScenarioObjects[i];
         BObject* pObject = getObject(killedID);
         if (pObject)
            pObject->kill(true);
      }
   }

   GFREADCLASS(pStream, saveType, gFatalityManager);
   GFREADCLASS(pStream, saveType, gActionManager);

   /*
   BCommandManager*              mpCommandManager;
   BWorldSoundManager*           mpWorldSoundManager;
   BPhysicsWorld*                mpPhysicsWorld;
   */

   GFREADCLASSPTR(pStream, saveType, mpObjectiveManager);
   GFREADMARKER(pStream, cSaveMarkerObjectiveManager);

   //BChatManager*                 mpChatManager;
   if (mGameFileVersion >= 12)
      GFREADCLASSPTR(pStream, saveType, mpChatManager);

   GFREADCLASSPTR(pStream, saveType, mpHintManager);

   /*
   BDesignObjectManager*         mpDesignObjectManager;
   BBattleManager*               mpBattleManager;
   */

   GFREADCLASSPTR(pStream, saveType, mpPowerManager);
   GFREADCLASSPTR(pStream, saveType, mpAIManager);

   /*
   BEntityGrouper*               mEntityGrouper;
   */

   GFREADCLASSPTR(pStream, saveType, mpPathingLimiter);

   // Times
   GFREADVAR(pStream, DWORD, mUpdateNumber);
   GFREADVAR(pStream, int64, mLastUpdateTime);
   GFREADVAR(pStream, DWORD, mLastUpdateLength);
   GFREADVAR(pStream, float, mLastUpdateLengthFloat);
   GFREADVAR(pStream, DWORD, mGametime);
   GFREADVAR(pStream, DWORD, mSubUpdate);
   GFREADVAR(pStream, DWORD, mFirstSubUpdate);
   GFREADVAR(pStream, int64, mSubUpdateBeganTime);
   GFREADVAR(pStream, int64, mSubUpdateLastTime);
   GFREADVAR(pStream, double, mGametimeFloat);
   GFREADVAR(pStream, double, mTotalRealtime);
   GFREADVAR(pStream, double, mSubTotalRealtime);
   GFREADVAR(pStream, int64, mLastUpdateRealtime);
   GFREADVAR(pStream, int64, mLastSubUpdateRealtime);
   GFREADVAR(pStream, double, mLastUpdateRealtimeDelta);
   GFREADVAR(pStream, double, mLastTimerFrequency);
   GFREADVAR(pStream, float, mAccumulatedTimeError);

   GFREADVAR(pStream, long, mFogStartX);
   GFREADVAR(pStream, long, mFogStartZ);
   GFREADVAR(pStream, long, mFogEndZ);

   /*
   BSmallDynamicSimArray<BLightVisualInstance*> mLights;

   BCinematicManager*            mpCinematicManager;
   BTransitionManager*           mpTransitionManager;

   BVisual*                      mpSkyVisual;
   int                           mBuildingPlacementDecal;
   long                          mTempObjectID;
   */

   uint maxTeams = 0;
   GFREADVAL(pStream, uint8, uint, maxTeams);
   BRelationType relationType;
   for (uint i=0; i<maxTeams; i++)
   {
      for (uint j=0; j<maxTeams; j++)
      {
         if (i<cMaximumSupportedTeams && j<cMaximumSupportedTeams)
            GFREADVAR(pStream, BRelationType, mTeamRelationType[i][j])
         else
            GFREADVAR(pStream, BRelationType, relationType)
      }
   }
   GFREADMARKER(pStream, cSaveMarkerWorld3);

   /*
   BEntityIDArray                mCachedUnitsToTrack;                      // 8 bytes
   BObstructionNodePtrArray      mCachedObstructions;                      // 12 bytes
   BObstructionNodePtrArray      mExpandedObstructions;                    // 12 bytes
   BObstructionNodePtrArray      mCachedTerrainObstructions;               // 12 bytes
   BObstructionNodeArray         mExpandedObstructionData;

   typedef BDynamicArray<BPhysicsBreakOffObject, ALIGN_OF(BPhysicsBreakOffObject), BDynamicArrayPrimaryHeapAllocator, BDynamicArraySmallOptions, BDynamicArraySeparatePointers> BPhysicsBreakOffObjectsArray;
   BPhysicsBreakOffObjectsArray mPhysicsBreakOffObjects;
   BLightWeightMutex mPhysicsBreakOffObjectMutex;
   */

   GFREADCLASSARRAY(pStream, saveType, mCustomCommands, uint8, 200);
   GFREADVAR(pStream, int, mNextCustomCommandID);
   GFREADMARKER(pStream, cSaveMarkerWorld4);

   GFREADCLASS(pStream, saveType, *mpStoredEventTagManager);
   GFREADMARKER(pStream, cSaveMarkerStoredAnimEventManager);

   GFREADVAR(pStream, float, mSimBoundsMinX);
   GFREADVAR(pStream, float, mSimBoundsMinZ);
   GFREADVAR(pStream, float, mSimBoundsMaxX);
   GFREADVAR(pStream, float, mSimBoundsMaxZ);
   GFREADPTR(pStream, sizeof(BVector)*4, mOutsideOfBoundsBlockerCenter);
   GFREADPTR(pStream, sizeof(BVector)*4, mOutsideOfBoundsBlockerSize);

   GFREADVAR(pStream, uint, mWorldID);
   GFREADVAR(pStream, float, mGameOverCountdown);
   GFREADVAR(pStream, BEntityHandle, mLastSubUpdateUnit);

   /*
   BSmallDynamicSimArray<BTactic*> mTransformedTactics;
   BSmallDynamicSimArray<BTalkingHead*> mTalkingHeads;
   */

   GFREADVAL(pStream, uint8, BPlayerColorCategory, mPlayerColorCategory);

   if (mGameFileVersion >= 9)
   {
      //BTimer mSubUpdateEnabledTimer;
      if (mGameFileVersion >= 14)
         GFREADVAR(pStream, bool, gEnableSubUpdating);

      GFREADVAR(pStream, DWORD, mSubUpdateEnabledNumber);
      GFREADVAR(pStream, bool, mDynamicSubUpdating);
      GFREADVAR(pStream, bool, mAlternateSubUpdating);
      GFREADVAR(pStream, bool, mDynamicSubUpdateTime);
      GFREADVAR(pStream, bool, mNextEnableSubUpdating);
      #ifdef DECOUPLED_UPDATE
      GFREADVAR(pStream, bool, mNextDecoupledUpdate);
      #else
      GFREADTEMPVAL(pStream, bool);
      #endif
      GFREADVAR(pStream, bool, mNextAlternateSubUpdating);
      GFREADVAR(pStream, bool, mNextDynamicSubUpdateTime);
      GFREADVAR(pStream, bool, mChangeSubUpdatePendnig);

      GFREADVAR(pStream, bool, mFirstUpdate);
   }

   GFREADBITBOOL(pStream, mFlagUnitCache);
   GFREADBITBOOL(pStream, mFlagObstructionCache);
   GFREADBITBOOL(pStream, mFlagQuickBuild);
   GFREADBITBOOL(pStream, mFlagGameOver);
   GFREADBITBOOL(pStream, mFlagMultiplayerSim);
   GFREADBITBOOL(pStream, mFlagCoop);
   GFREADBITBOOL(pStream, mFlagPlayableBounds);
   GFREADBITBOOL(pStream, mFlagOutsideBoundsBlocked);
   GFREADBITBOOL(pStream, mFlagNoFogSim);
   GFREADBITBOOL(pStream, mFlagNoFogVis);
   GFREADBITBOOL(pStream, mFlagShowSkirt);
   GFREADBITBOOL(pStream, mFlagAllVisible);
   GFREADBITBOOL(pStream, mFlagSkipGameOverCheck);
   GFREADBITBOOL(pStream, mFlagStartedUpdate);

   GFREADMARKER(pStream, cSaveMarkerWorld5);

   // Corpse manager
   GFREADCLASS(pStream, saveType, gCorpseManager);

   // General events
   GFREADCLASS(pStream, saveType, gGeneralEventManager);
   GFREADMARKER(pStream, cSaveMarkerGeneralEvents);

   // Triggers
   GFREADCLASS(pStream, saveType, gTriggerManager);
   GFREADMARKER(pStream, cSaveMarkerTriggers);

   // Visibilty
   GFREADCLASS(pStream, saveType, gVisibleMap);
   GFREADMARKER(pStream, cSaveMarkerVisibilty);

   // Score Manager
   GFREADCLASS(pStream, saveType, gScoreManager);
   GFREADMARKER(pStream, cSaveMarkerScoreManager);

   if (mGameFileVersion >= 4)
   {
      GFREADCLASS(pStream, saveType, gEntityScheduler);
      GFREADMARKER(pStream, cSaveMarkerEntityScheduler);
   }

   if (mGameFileVersion >= 5)
   {
      GFREADCLASS(pStream, saveType, gCollectiblesManager);
      GFREADMARKER(pStream, cSaveMarkerCollectiblesManager);
   }

   if (mGameFileVersion >= 6)
      GFREADVAR(pStream, long, mLowDetailAnimRefCount)

   if (mGameFileVersion >= 8)
      GFREADVECTOR(pStream, mTerrainMax)

   //MusicManager state
   bool musicManagerDisabled = true;
   if (mGameFileVersion >= 11)
      GFREADVAR(pStream, bool, musicManagerDisabled);
   if( mpWorldSoundManager && mpWorldSoundManager->getMusicManager() )
      mpWorldSoundManager->getMusicManager()->setEnabled(!musicManagerDisabled);


   // Read preload talking heads list
   if(mGameFileVersion >= 13)
   {
      // Clear out mPreload flag in all talking heads
      long numTalkingHeads = mTalkingHeads.getNumber();
      for (int i=0; i<numTalkingHeads; i++)
      {
         mTalkingHeads[i]->mPreload = false;
      }

      long numPreloadedTalkingHeads;
      GFREADVAR(pStream, long, numPreloadedTalkingHeads);

      // Reenable mPreload flag on needed talking heads
      for (int i=0; i<numPreloadedTalkingHeads; i++)
      {
         BString preloadTalkingHeadName;
         GFREADSTRING(pStream, BString, preloadTalkingHeadName, 200);

         // Find in the list
         for (int j=0; j<numTalkingHeads; j++)
         {
            BTalkingHead* pTalkingHead = mTalkingHeads[j];
            if(preloadTalkingHeadName.compare(pTalkingHead->mFilename, false) == 0)
            {
               pTalkingHead->mPreload = true;
               break;
            }
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BWorld::postLoad(int saveType)
{
   // Post load. Also kill any objects that don't have a valid proto because it couldn't be remapped.
   BEntityHandle handle = cInvalidObjectID;
   BUnit* pUnit= getNextUnit(handle);
   while (pUnit)
   {
      if (pUnit->getProtoID() == -1)
      {
         BSquad* pSquad = pUnit->getParentSquad();
         if (pSquad)
            pSquad->removeChild(pUnit->getID(), true);
         pUnit->setFlagDestroy(true);
         pUnit->setFlagNoUpdate(false);
      }
      else if (!pUnit->postLoad(saveType))
         return false;
      pUnit = getNextUnit(handle);
   }

   handle = cInvalidObjectID;
   BSquad* pSquad = getNextSquad(handle);
   while (pSquad)
   {
      if (pSquad->getProtoID() == -1)
         pSquad->setFlagDestroy(true);
      else if (!pSquad->postLoad(saveType))
         return false;

      // Remove children that no longer exist
      pSquad->removeInvalidChildren();

      pSquad = getNextSquad(handle);
   }

   handle = cInvalidObjectID;
   BArmy* pArmy = getNextArmy(handle);
   while (pArmy)
   {
      if (!pArmy->postLoad(saveType))
         return false;
      pArmy = getNextArmy(handle);
   }

   handle = cInvalidObjectID;
   BPlatoon* pPlatoon = getNextPlatoon(handle);
   while (pPlatoon)
   {
      if (!pPlatoon->postLoad(saveType))
         return false;
      pPlatoon = getNextPlatoon(handle);
   }

   handle = cInvalidObjectID;
   BDopple* pDopple = getNextDopple(handle);
   while (pDopple)
   {
      if (pDopple->getProtoID() == -1)
      {
         pDopple->setFlagDestroy(true);
         pDopple->setFlagNoUpdate(false);
      }
      else if (!pDopple->postLoad(saveType))
         return false;
      pDopple = getNextDopple(handle);
   }

   handle = cInvalidObjectID;
   BProjectile* pProjectile = getNextProjectile(handle);
   while (pProjectile)
   {
      if (pProjectile->getProtoID() == -1)
      {
         pProjectile->setFlagDestroy(true);
         pProjectile->setFlagNoUpdate(false);
      }
      else if (!pProjectile->postLoad(saveType))
         return false;
      pProjectile = getNextProjectile(handle);
   }

   if (mGameFileVersion >= 7)
   {
      handle = cInvalidObjectID;
      BObject* pObject = getNextObject(handle);
      while (pObject)
      {
         if (pObject->getProtoID() == -1)
         {
            pObject->setFlagDestroy(true);
            pObject->setFlagNoUpdate(false);
         }
         else if (!pObject->postLoad(saveType))
            return false;
         pObject = getNextObject(handle);
      }
   }

   gUIManager->resetMinimap();

   bool flagNoFogVis = mFlagNoFogVis;
   bool flagNoFogSim = mFlagNoFogSim;
   bool flagShowSkirt = mFlagShowSkirt;
   mFlagNoFogVis = false;
   mFlagNoFogSim = false;
   mFlagShowSkirt = false;
   if (flagNoFogVis || flagNoFogSim)
   {
      bool onlyVisual = !flagNoFogSim;
      setFogOfWar(false, flagShowSkirt, onlyVisual);
   }
   else
   {
      gTerrain.setRenderSkirt(flagShowSkirt);
      mFlagShowSkirt = flagShowSkirt;
   }

   return true;
}

#ifdef SYNC_World
//==============================================================================
//==============================================================================
void BWorld::archiveSyncCheck()
{
   // Only sync check if archives are enabled
   if (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives)
   {
//-- FIXING PREFIX BUG ID 6042
      const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
      BDEBUG_ASSERT(pSettings);

      BSimString mapName;
      pSettings->getString(BGameSettings::cMapName, mapName);
      mapName += ".era";
      mapName.removePath();

      BSimString scenarioArchiveName = "d:\\";
      scenarioArchiveName += mapName;

      uint32 rootArchiveCRC = generateArchiveCRC("d:\\root.era");
      /*
      uint32 UNSCArchiveCRC = generateArchiveCRC(CIV_UNSC_ARCHIVE_FILENAME);
      uint32 covenantArchiveCRC = generateArchiveCRC(CIV_COVN_ARCHIVE_FILENAME);
      */
      uint32 scenarioArchiveCRC = generateArchiveCRC(scenarioArchiveName);

      syncWorldData("BWorld::archiveSyncCheck Root", (int) rootArchiveCRC);
      /*
      syncWorldData("BWorld::archiveSyncCheck UNSC", (int) UNSCArchiveCRC);
      syncWorldData("BWorld::archiveSyncCheck Covenant", (int) covenantArchiveCRC);
      */
      syncWorldData("BWorld::archiveSyncCheck Scenario", (int) scenarioArchiveCRC);
   }
}

//==============================================================================
//==============================================================================
uint32 BWorld::generateArchiveCRC(const char* pFilename)
{
   // Open the raw archive
   FILE* pFile = NULL;
   if (fopen_s(&pFile, pFilename, "rbS") == 0)
   {
      // Read the first KB
      uint8 buffer[1024];
      Utils::FastMemSet(buffer, 0, 1024);
      long numRead = fread(buffer, 1, 1024, pFile);
      if (ferror(pFile))
      {
         BFAIL("BWorld::generateArchiveCRC ferror");
      }
      else
      {
         BASSERT((numRead == 1024) || feof(pFile));
      }

      // Return that buffer's CRC
      return calcCRC32(buffer, 1024);
   }

   return cInitCRC32;
}
#endif

//==============================================================================
//==============================================================================
bool BWorld::createPowerAudioReactions(BPlayerID castingPlayerID, BSquadSoundType soundType, BVector worldPos, float radius, BEntityID castingSquadID)
{
   // Don't bother scanning the area for the hot drop or cheer chatter because they're only meant for the caster. This is a special case optimization.
   // It could also be done by placing CastingUnitOnly flags everywhere, but this is easier and more efficient.
   if ((soundType == cSquadSoundChatterReactHotDrop) || (soundType == cSquadSoundChatterCheer))
   {
      bool result = false;
      BSquad* pSquad = gWorld->getSquad(castingSquadID);
      if (pSquad)
      {
         result =  pSquad->playChatterSound(soundType, -1, false);
      }      
      return result;
   }

//-- FIXING PREFIX BUG ID 6028
   const BPlayer* pPlayer = gWorld->getPlayer(castingPlayerID);
//--
   if(!pPlayer)
      return false;

   long castingCivID = pPlayer->getCivID();
   BTeamID ourTeam = pPlayer->getTeamID();

   //-- Find units in area that want to react               
   BEntityIDArray results(0, 100);
   BUnitQuery query(worldPos, radius, false);            
   query.setFlagIgnoreDead(true);
   query.setRelation(castingPlayerID, cRelationTypeAny);
   bool anyChatter = false;
   long numResults = gWorld->getSquadsInArea(&query, &results, false);
   for(long i=0; i < numResults; i++)
   {
      BSquad* pSquad= gWorld->getSquad(results[i]);
      if(pSquad)
      {         
         BTeamID squadsTeam = pSquad->getTeamID();
         BRelationType relation = gWorld->getTeamRelationType(ourTeam, squadsTeam);

         // Rules:
         // 1. Allies that share your civ react
         // 1. Enemies that don't share your civ react
         if (((relation == cRelationTypeAlly) && (castingCivID == pSquad->getPlayer()->getCivID())) ||
             ((relation == cRelationTypeEnemy) && (castingCivID != pSquad->getPlayer()->getCivID())))
         {
            bool chatter = pSquad->playChatterSound(soundType, -1, false, castingSquadID);
            anyChatter = anyChatter || chatter;
         }
      }
   }
   return anyChatter;
}

//==============================================================================
//==============================================================================
void BWorld::recordScenarioLoadObjectHighWaterMark()
{
   mScenarioLoadObjectHighWaterMark = mpObjectManager->getObjectsHighWaterMark();
}
