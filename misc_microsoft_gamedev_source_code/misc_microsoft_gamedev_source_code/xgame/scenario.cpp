//==============================================================================
// scenario.cpp
//
// Copyright (c) Ensemble Studios, 2006-2007
//==============================================================================

// Includes
#include "common.h"
#include "scenario.h"
#include "civ.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "reloadManager.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "lightEffectManager.h"
#include "lightVisualManager.h"
#include "minimap.h"
#include "modeManager.h"
#include "modegame.h"
#include "liveSystem.h"
#include "liveVoice.h"
#include "objectmanager.h"
#include "pather.h"
#include "physics.h"
#include "physicsworld.h"
#include "protoobject.h"
#include "protosquad.h"
#include "recordgame.h"
#include "render.h"
#include "sceneLightManager.h"
#include "squad.h"
#include "syncmacros.h"
#include "team.h"
#include "terrain.h"
#include "triggermanager.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "xmlreader.h"
#include "xmlwriter.h"
#include "TerrainTexturing.h"
#include "vincehelper.h"
#include "renderControl.h"
#include "objectivemanager.h"
#include "designobjectsmanager.h"
#include "squadactiontransport.h"
#include "archiveManager.h"
#include "uimanager.h"
#include "cinematic.h"
#include "statsManager.h"
#include "SimOrderManager.h"
#include "terrainimpactdecal.h"
#include "soundmanager.h"
#include "ugxGeomManager.h"
#include "game.h"
#include "timelineprofiler.h"
#include "fatalitymanager.h"
#include "gamemode.h"
#include "econfigenum.h"
#include "techtree.h"
#include "savegame.h"
#include "EntityGrouper.h"
#include "terraineffectmanager.h"
#include "particlegateway.h"
#include "humanPlayerAITrackingData.h"
#include "deathmanager.h"
#include "scoremanager.h"
#include "skullmanager.h"
#include "damagetemplatemanager.h"

#ifndef BUILD_FINAL
#include "tracerecording.h"
#endif

//==============================================================================
// constant
//============================================================================
const WCHAR* cUnicodeStringDelimeter = L"$$";

// Global
BScenario gScenario;
bool gScenarioLoad = false;

enum
{
   cPlayerPlacementFixed,
   cPlayerPlacementGrouped,
   cPlayerPlacementConsecutive,
   cPlayerPlacementRandom,
};

//==============================================================================
// BScenario::BScenario
//==============================================================================
BScenario::BScenario() :
   mLightSetDirID(-1),
   mMinimapTextureHandle(cInvalidManagedTextureHandle),
   mReloadTime(0),
   mFlagAllowVeterancy(false),
   mFlagReload(false)
#ifdef ENABLE_RELOAD_MANAGER
   ,BEventReceiver()
#endif
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
   for (int i=0; i<cMaximumSupportedPlayers; i++)
      mScenarioIDToPlayerID[i]=i;
}

//==============================================================================
// BScenario::~BScenario
//==============================================================================
BScenario::~BScenario()
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
}

//==============================================================================
// BScenario::initUGXOptions
//==============================================================================
void BScenario::initUGXOptions()
{
   BNameValueMap ugxOptions;
   
   // If this is a campaign map, and multiplayer is not active, then enable the SPC override textures 
   // (which also disables the xform textures when present).
   // rg [6/3/08] - This will not work anymore, because gr2ugx has been modified to not put the spc_ textures into the .dep files.
   // Disabling this temporarily as per Scott's request (SAT)
   //if ((forceSPCTextures) || ((gameType == BGameSettings::cGameTypeCampaign) && (!isMPRunning)))
   //   ugxOptions.add("SPCTexturePrefix", "spc_");
      
   gUGXGeomManager.setOptions(ugxOptions);
}

//==============================================================================
// BScenario::load
//==============================================================================
bool BScenario::load(bool reload, bool leaveCamera, bool leaveRecordGame)
{   
   SCOPEDSAMPLE(BScenario_load)

   gGame.incGameCount();

   mReloadTime = 0;
   mFlagReload = false;

#ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigPIXTraceWorldLoad))
      {
         BOOL traceStartOK = XTraceStartRecording("e:\\WorldLoadTrace.pix2");
         BASSERT(traceStartOK);
      }
#endif

   DWORD time=timeGetTime();
   
   gRender.startupStatus("Load scenario start");
#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load A");
#endif

   float simBoundsMinX = 0.0f;
   float simBoundsMinZ = 0.0f;
   float simBoundsMaxX = cMaximumFloat;
   float simBoundsMaxZ = cMaximumFloat;

   gModeManager.getModeGame()->setPaused(false);

   // Get the current game settings
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(!pSettings)
      return false;
      
   // Init the random seed
   long seed = -1;
   pSettings->getLong(BGameSettings::cRandomSeed, seed);
   setGlobalRandSeed(seed);

   // Load the scenario file
   
   //Get the map name.
   BFixedStringMaxPath mapName;
   pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
   
   //Put the extension on it.
   BFixedStringMaxPath mapNameWithExtension;
   mapNameWithExtension=mapName;
   mapNameWithExtension+=".scn";

//APF This code was tested and seems good to go, but the editor release has been delayed
//#ifdef NEWSCENARIOFILES
   BFixedStringMaxPath mapArtObjectsWithExtension;
   mapArtObjectsWithExtension=mapName;
   mapArtObjectsWithExtension+=".sc2";

   BFixedStringMaxPath mapSoundsWithExtension;
   mapSoundsWithExtension=mapName;
   mapSoundsWithExtension+=".sc3";
//#endif

   //mapNameWithExtension="CbtTstRange1\\CbtTstRange1.scn";
   
   gRender.startupStatus("Loading scenario %s", mapNameWithExtension.getPtr());
   gConsoleOutput.resource("Loading scenario %s\n", mapNameWithExtension.getPtr());
   
   // initUGXOptions() MUST be called before any UGX meshes are loaded!
   initUGXOptions();

#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
#endif

   BSimString fullpath;
   BSimString fullpathArtObjects;
   BSimString fullpathSounds;
   gFileManager.constructQualifiedPath(cDirScenario, mapNameWithExtension, fullpath);

//#ifdef NEWSCENARIOFILES
   gFileManager.constructQualifiedPath(cDirScenario, mapArtObjectsWithExtension, fullpathArtObjects);
   gFileManager.constructQualifiedPath(cDirScenario, mapSoundsWithExtension, fullpathSounds);
//#endif 

   BReloadManager::BPathArray paths;
   paths.pushBack(fullpath);

//#ifdef NEWSCENARIOFILES
   paths.pushBack(fullpathArtObjects);
   paths.pushBack(fullpathSounds);
//#endif
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif
   mGaiaSquadList.clear();

   if(reload)
   {            
      BSimString terrainName = gWorld->getTerrainName();

      // Unload and reset data except for terrain (most of this comes from BGameMode::deinitGame and deinitWorld)
      setWorldReset(true);

      if (gRecordGame.isPlaying() || (gRecordGame.isRecording() && !leaveRecordGame))
         gRecordGame.stop();

      gWorld->reset();

      unload();

      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.reset();            

      BUser* pUser = gUserManager.getPrimaryUser();
      if (pUser)
         pUser->gameRelease();

      pUser = gUserManager.getSecondaryUser();
      if (pUser)
         pUser->gameRelease();

      gParticleGateway.releaseAllInstances();   
      gLightEffectManager.releaseAllInstances();
      gSimSceneLightManager.resetLightSet();      

      setWorldReset(false);

      // Start the load
      /*
      BSimString fullMapName;   
      gFileManager.constructQualifiedPath(cDirScenario, mapNameWithExtension, fullMapName);
      if (!gArchiveManager.beginScenarioPrefetch(fullMapName)) 
      {
         gScenarioLoad = false;
         return false;
      }

      if (!gArchiveManager.beginScenarioLoad(fullMapName))
      {
         gScenarioLoad = false;
         return false;
      }
      */

      gWorld->setup();
      gWorld->initTerrain(terrainName);      

      gStatsManager.init(false);
      gGame.openVinceLog();
      gVisualManager.gameInit();

#ifdef ENABLE_FPS_LOG_FILE
      gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load B");
#endif

   }

   BXMLReader reader;
   {
      SCOPEDSAMPLE(BScenario_load_ReadXML)
      if(!reader.load(cDirScenario, mapNameWithExtension))
         return false;
   }
   
//#ifdef NEWSCENARIOFILES
   bool bHasArtObjects = true;
   BXMLReader readerArtObjects;
   {
      SCOPEDSAMPLE(BScenario_load_ReadXML_ArtObjects)
      if(!readerArtObjects.load(cDirScenario, mapArtObjectsWithExtension))
      {
         bHasArtObjects = false;
      }
   }

   bool bHasSoundObjects = true;
   BXMLReader readerSounds;
   {
      SCOPEDSAMPLE(BScenario_load_ReadXML_Sounds)
      if(!readerSounds.load(cDirScenario, mapSoundsWithExtension))
      {
         bHasSoundObjects = false;
      }
   }
//#endif

   // Get the base scenario directory and the individual scenario directory
   BSimString fileDir;
   strPathGetDirectory(BSimString(mapNameWithExtension), fileDir);
   BSimString scenarioDir;
   eFileManagerError result = gFileManager.getDirListEntry(scenarioDir, cDirScenario);
   BVERIFY(cFME_SUCCESS == result);
   BSimString mainDir;
   result = gFileManager.getDirListEntry(mainDir, cDirProduction);
   BVERIFY(cFME_SUCCESS == result);
   scenarioDir.remove(0, mainDir.length());

   // Prep for a new world
   gScenarioLoad = true;
   gSimSceneLightManager.resetLightSet();  
   mScenarioIDToEntityID.removeAll();
   for (int i=1; i<cMaximumSupportedPlayers; i++)
      mScenarioIDToPlayerID[i]=cInvalidPlayerID;
   
   if(!reload)
   {
      if (!gSaveGame.isLoading())
         gDamageTemplateManager.reset();

      // New world
      BDEBUG_ASSERT(!gWorld);
      gWorld=new BWorld();
      if(!gWorld->setup())
      {
         gScenarioLoad = false;
         return false;
      }

      gSimSceneLightManager.clearLightSets();      

#ifdef ENABLE_FPS_LOG_FILE
      gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load C");
#endif

   }

   //move into the if(!reload) above if we want to keep the same skulls when reloading
   gCollectiblesManager.clearSkullActivations();
            
   if (gArchiveManager.getArchivesEnabled())
   {
#ifndef BUILD_FINAL
      gVisualManager.ensureUnloaded();
      gDamageTemplateManager.ensureEmpty();
#ifdef SYNC_World
      syncWorldCode("BScenario::load process the scenario file");
      syncWorldData("BScenario::load number proto visuals", gVisualManager.getNumProtoVisuals());
      syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
      syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
#endif
#endif

#ifdef SYNC_World
      syncWorldCode("BScenario::load preload files");
#endif
      SCOPEDSAMPLE(BScenario_gArchiveManager_Preload)

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load D");
#endif

      if (gConfig.isDefined("UnloadRootArchive"))
         preloadShapeFiles();

      preloadPfxFiles("pfxFileList.txt");
      preloadVisFiles("visFileList.txt");
      preloadTfxFiles("tfxFileList.txt");

#ifndef BUILD_FINAL
#ifdef SYNC_World
      syncWorldCode("BScenario::load preload files");
      syncWorldData("BScenario::load number proto visuals", gVisualManager.getNumProtoVisuals());
      syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
      syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
#endif
#endif

      preloadUnits();

#ifdef ENABLE_FPS_LOG_FILE
      gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load E");
#endif

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::load preload units");
         syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
         syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
      }
#endif
#endif
   }

   mUnitStarts.clear();
   mRallyStarts.clear();

   mLightSetDirID = -1;
   mLightSetScenarioDir.empty();
   mLightSetName.empty();

   mTriggerScriptIDs.clear();

   long playerPlacementType=cPlayerPlacementFixed;
   long playerPlacementSpacing=0;

   // Process the scenario file
   BScopedPhysicsWrite physicsWriteMarker;
   BScenarioPositionArray positionList;
   BScenarioPlayerArray playerList;
   BRelationType relationTypes[cMaximumSupportedTeams][cMaximumSupportedTeams];
   memset(relationTypes, 0, sizeof(relationTypes));
   bool relationTypesLoaded=false;
   long localPlayerID=-1;
   long localPlayerID2=-1;

   bool isCoop=false;
   pSettings->getBool(BGameSettings::cCoop, isCoop);      

   long gameType = -1;
   pSettings->getLong(BGameSettings::cGameType, gameType);

   BXMLNode xmlRoot(reader.getRootNode());
   long nodeCount=xmlRoot.getNumberChildren();
   
   //-- Find and load sound banks first
   BASSERT(mScenarioSoundBankIDs.getSize() == 0);
   mScenarioSoundBankNames.clear();
   mScenarioSoundBankIDs.clear();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode xmlNode(xmlRoot.getChild(i));
      const BPackedString nodeName(xmlNode.getName());
      if(nodeName == "SoundBank")
      {         
         SCOPEDSAMPLE(BScenario_Node_SoundBank)
         BString bankName;
         xmlNode.getText(bankName);
         mScenarioSoundBankNames.add(bankName);
         //uint32 bankID = gSoundManager.loadSoundBank(bankName, false);
         //if(bankID != AK_INVALID_BANK_ID)
         //   mScenarioSoundBankIDs.add(bankID);
      }
   }
   
#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load F");
#endif

   if (gSaveGame.isLoading())
   {
      if (!gSaveGame.loadSetup())
      {
         gSaveGame.reset();
         gScenarioLoad = false;
         //if (reload)
         //   gArchiveManager.endScenarioLoad();
         return false;
      }
      for (int i=1; i<gWorld->getNumberPlayers(); i++)
         mScenarioIDToPlayerID[i] = gWorld->getPlayer(i)->getScenarioID();
   }

   bool recordGame = gRecordGame.isRecording();
   bool playGame = gRecordGame.isPlaying();
   
   mFlagAllowVeterancy = false;

#ifndef BUILD_FINAL
#ifdef SYNC_World
   if (gArchiveManager.getArchivesEnabled())
   {
      syncWorldCode("BScenario::load before nodeCount for loop");
      syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
      syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
   }
#endif
#endif

   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode xmlNode(xmlRoot.getChild(i));
      const BPackedString nodeName(xmlNode.getName());
      if(nodeName=="Terrain")
      {
         SCOPEDSAMPLE(BScenario_Node_Terrain)
         gObsManager.deinitialize();

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load G");
#endif

         if(!reload)
         {
            if(!loadTerrain(xmlNode, scenarioDir, fileDir, mapName))
            {
               gScenarioLoad = false;
               return false;
            }
         }

         gRender.startupStatus("Generating terrain obstructions");
         
         gObsManager.initialize();
         initObstructionRotationTables();

         if (gSaveGame.isLoading())
            gObsManager.rebuildPlayerRelationMasks();

         // Test for pre-loading obstructions
         BFixedStringMaxPath terrainName;
         terrainName=mapName;
         terrainName+=".lrp";

         gPather.loadLRPTree(terrainName);

#ifndef BUILD_FINAL   
         if (gConfig.isDefined(cConfigNoUpdatePathingQuad))
            gObsManager.disablePathingUpdates();
         else
#endif
         {
            // Generate world edge obstructions
            float size = ((float)gTerrainSimRep.getNumXDataTiles()) * gTerrainSimRep.getDataTileScale();
            gWorld->setSimBounds(0.0f, 0.0f, size, size);
            DWORD edgeTime1 = timeGetTime();
            gObsManager.generateWorldBoundries();
            DWORD edgeTime2 = timeGetTime();
            DWORD edgeElapsed = edgeTime2 - edgeTime1;
            blogtrace("generateWorldBoundries time = %u", edgeElapsed);

            gObsManager.generateTerrainObstructions(gTerrainSimRep.getObstructionMap());
            // SLB: This is causing every terrain obstruction to be duplicated in the unit obstruction quad tree.
            //      We don't need this anymore because we no longer support the free placement of buildings.
            //      If this ever needs to be re-enabled, the bug needs to be fixed first. Talk to Dusty for more info.
            //BYTE* map = gTerrainSimRep.getBuildableMap();
            //if (map)
            //   gObsManager.generateTerrainObstructions(map);
         }

         gRender.startupStatus("Finished generating terrain obstructions");

         //-- lock down the physics for writing
         physicsWriteMarker.init(gWorld->getPhysicsWorld());

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load terrain");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Pathing")
      {
         SCOPEDSAMPLE(BScenario_Node_Pathing)

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load H");
#endif

         BSimString temp;
         if (!xmlNode.getText(temp).isEmpty())
         {
            BFixedStringMaxPath fileName=scenarioDir;
            fileName+=fileDir;
            fileName+=temp;

            if(!gWorld->initPathing(fileName))
            {
               gScenarioLoad = false;
               //if (reload)
               //   gArchiveManager.endScenarioLoad();
               return false;
            }
         }      

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load pathing");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }      
      else if(nodeName=="Cinematics")
      {
#ifndef BUILD_FINAL
         if(gModeManager.getModeType() == BModeManager::cModeCinematic)
            continue;
#endif

         SCOPEDSAMPLE(BScenario_Node_Cinematics)
         if(!gWorld)
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load I");
#endif

         BSimString cinematicDir;
         eFileManagerError result = gFileManager.getDirListEntry(cinematicDir, cDirArt);
         BVERIFY(cFME_SUCCESS == result);

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Cinematic")
            {
               if(!loadCinematic(childNode, cinematicDir))
               {
                  gScenarioLoad = false;
                  //if (reload)
                  //   gArchiveManager.endScenarioLoad();
                  return false;
               }
            }
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load cinematics");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if (nodeName=="TalkingHeads")
      {
         if(!gWorld)
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load J");
#endif

         BSimString dir;
         eFileManagerError result = gFileManager.getDirListEntry(dir, cDirTalkingHead);
         BVERIFY(cFME_SUCCESS == result);

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="TalkingHead")
            {
               if(!loadTalkingHead(childNode, dir))
               {
                  gScenarioLoad = false;
                  //if (reload)
                  //   gArchiveManager.endScenarioLoad();
                  return false;
               }
            }
         }         

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load talkingheads");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Lightsets")
      {
         SCOPEDSAMPLE(BScenario_Node_Lightsets)
         if(!gWorld)
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load K");
#endif

         BSimString lightsetsDir;
         eFileManagerError result = gFileManager.getDirListEntry(lightsetsDir, cDirArt);
         BVERIFY(cFME_SUCCESS == result);


         gSimSceneLightManager.clearLightSets();


         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Lightset")
            {
               if(!loadOtherLightset(childNode, lightsetsDir))
               {
                  gScenarioLoad = false;
                  //if (reload)
                  //   gArchiveManager.endScenarioLoad();
                  return false;
               }
            }
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load lightsets");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="PlayerPlacement")
      {
         SCOPEDSAMPLE(BScenario_Node_PlayerPlacement)

#ifdef ENABLE_FPS_LOG_FILE
            gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load L");
#endif

         BSimString typeName;
         if(xmlNode.getAttribValueAsString("Type", typeName))
         {
            if(typeName=="Grouped")
            {
               playerPlacementType=cPlayerPlacementGrouped;
            }
            else if(typeName=="Consecutive")
            {
               playerPlacementType=cPlayerPlacementConsecutive;
               xmlNode.getAttribValueAsLong("Spacing", playerPlacementSpacing);
            }
            else if(typeName=="Random")
            {
               playerPlacementType=cPlayerPlacementRandom;
            }
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load playerplacement");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Positions")
      {
         SCOPEDSAMPLE(BScenario_Node_Positions)

#ifdef ENABLE_FPS_LOG_FILE
            gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load M");
#endif

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Position")
               loadPosition(childNode, positionList);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load positions");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Diplomacies")
      {
         SCOPEDSAMPLE(BScenario_Node_Diplomacies)

#ifdef ENABLE_FPS_LOG_FILE
            gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load N");
#endif

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Diplomacy")
            {
               loadRelationType(childNode, (BRelationType*)relationTypes);
               relationTypesLoaded=true;
            }
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load diplomacies");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Players")
      {
         SCOPEDSAMPLE(BScenario_Node_Players)

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load O");
#endif

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Player" && playerList.getNumber() < cMaximumSupportedPlayers-1)
               loadPlayer(childNode, pSettings, playerList);
         }
         for(long i=0; i<positionList.getNumber(); i++)
         {
            long playerID=positionList[i].mPlayerID;
            if(playerID>=1 && playerID<=playerList.getNumber())
               playerList[playerID-1].mPositionIndex=i;
         }
         if(!gSaveGame.isLoading() && !setupPlayers(pSettings, playerList, positionList, (relationTypesLoaded?(BRelationType*)relationTypes:NULL), leaveCamera, playerPlacementType, playerPlacementSpacing, isCoop, localPlayerID, localPlayerID2, recordGame, playGame))
         {
            gScenarioLoad = false;
            //if (reload)
            //   gArchiveManager.endScenarioLoad();
            return false;
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load players");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Objects")
      {
         SCOPEDSAMPLE(BScenario_Node_Objects)
         if(!gWorld)
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load P");
#endif

         // Make sure we have some players (old scenarios might not)
         if(playerList.getNumber()==0)
         {
            BScenarioPlayer player;
            player.mTeam=1;
            playerList.add(player);
            player.mTeam=2;
            playerList.add(player);
            if(!gSaveGame.isLoading() && !setupPlayers(pSettings, playerList, positionList, (relationTypesLoaded?(BRelationType*)relationTypes:NULL), leaveCamera, playerPlacementType, playerPlacementSpacing, isCoop, localPlayerID, localPlayerID2, recordGame, playGame))
            {
               gScenarioLoad = false;
               //if (reload)
               //   gArchiveManager.endScenarioLoad();
               return false;
            }
         }

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Object")
               loadObject(childNode);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load objects");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Lights")
      {
         SCOPEDSAMPLE(BScenario_Node_Lights)

#ifdef ENABLE_FPS_LOG_FILE
            gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load Q");
#endif

         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="Light")
               loadLight(childNode);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load lights");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="LightSet")
      {
         SCOPEDSAMPLE(BScenario_Node_LightSet)

#ifdef ENABLE_FPS_LOG_FILE
            gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load R");
#endif

         BSimString temp;
         if (!xmlNode.getText(temp).isEmpty())
         {
            if(!loadLightSet(cDirScenario, fileDir, temp))
            {
               gScenarioLoad = false;
               //if (reload)
               //   gArchiveManager.endScenarioLoad();
               return false;
            }
         }               

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load lightset");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="Sky")
      {
         BSimString temp;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load S");
#endif

         gWorld->initSky(xmlNode.getTextPtr(temp));

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load sky");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="TerrainEnv")
      {
         BSimString temp;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load T");
#endif

         gTerrainTexturing.setEnvMapTexture(cDirArt,xmlNode.getTextPtr(temp));

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load terrainenv");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="BuildingTextureIndexUNSC")
      {
#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load U");
#endif

         int val=0;
         xmlNode.getTextAsInt32(val);
         gTerrainTexturing.setBuildingSplatIndexUNSC(val);

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load buildingtextureindexUNSC");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if(nodeName=="BuildingTextureIndexCOVN")
      {
#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load V");
#endif

         int val=0;
         xmlNode.getTextAsInt32(val);
            gTerrainTexturing.setBuildingSplatIndexCOVN(val);

#ifndef BUILD_FINAL
#ifdef SYNC_World
            if (gArchiveManager.getArchivesEnabled())
            {
               syncWorldCode("BScenario::load buildingtextureindexCOVN");
               syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
               syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
            }
#endif
#endif
      }
      else if(nodeName=="TriggerSystem")
      {
         if (gSaveGame.isLoading())
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load W");
#endif

         SCOPEDSAMPLE(BScenario_Node_TriggerSystem)
//-- FIXING PREFIX BUG ID 2834
         const BTriggerScript* pScript = gTriggerManager.createTriggerScriptFromXML(xmlNode);
//--
         if (pScript)
         {
            BTriggerScriptID scriptID = pScript->getID();
            gTriggerManager.activateTriggerScript(scriptID);
            mTriggerScriptIDs.add(scriptID);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load triggersystem");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if (nodeName == "ExternalTriggerScript")
      {
         if (gSaveGame.isLoading())
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load X");
#endif

         SCOPEDSAMPLE(BScenario_Node_ExternalTriggerScript)
         BXMLNode fileNameNode = xmlNode.getChildNode("FileName");
         BSimString externalScriptFileName;
         fileNameNode.getText(externalScriptFileName);
         BASSERTM(!externalScriptFileName.isEmpty(), "Scenario refers to external trigger script that does not exist.  Skipping.");
         if (!externalScriptFileName.isEmpty())
         {
            BTriggerScriptID triggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, externalScriptFileName);
            gTriggerManager.activateTriggerScript(triggerScriptID);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load externaltriggerscript");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if( nodeName == "Objectives" )
      {

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load Y");
#endif

         if (!gSaveGame.isLoading())
         {
            SCOPEDSAMPLE(BScenario_Node_Objectives)
            long childCount = xmlNode.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               BXMLNode childNode(xmlNode.getChild(j));
               if(childNode.getName() == "Objective")
               {
                  loadObjective(childNode);
               }
            }
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load objectives");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if (nodeName == "SimBoundsMinX")
         xmlNode.getTextAsFloat(simBoundsMinX);
      else if (nodeName == "SimBoundsMinZ")
         xmlNode.getTextAsFloat(simBoundsMinZ);
      else if (nodeName == "SimBoundsMaxX")
         xmlNode.getTextAsFloat(simBoundsMaxX);
      else if (nodeName == "SimBoundsMaxZ")
         xmlNode.getTextAsFloat(simBoundsMaxZ);
      else if (nodeName == "DesignObjects")
      {
         SCOPEDSAMPLE(BScenario_Node_DesignObjects)
         BDesignObjectManager* pDesignObjectManager = gWorld->getDesignObjectManager();

         if( !pDesignObjectManager )
         {
            BASSERTM( pDesignObjectManager, "Invalid DesignObjectManager manager!" );            
         } 
         else
         {
            pDesignObjectManager->load(xmlNode);
         }
      }
      else if (nodeName == "ObjectGroups")
      {
         if(!gWorld)
            continue;

#ifdef ENABLE_FPS_LOG_FILE
         gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load Z");
#endif


         long childCount=xmlNode.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode.getChild(j));
            if(childNode.getName()=="ObjectGroup")
               loadObjectGroup(childNode);
         }

#ifndef BUILD_FINAL
#ifdef SYNC_World
         if (gArchiveManager.getArchivesEnabled())
         {
            syncWorldCode("BScenario::load objectgroups");
            syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
            syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
         }
#endif
#endif
      }
      else if (nodeName == "ScenarioWorld")
      {
         BSimString scenarioWorldStr;
         xmlNode.getText(scenarioWorldStr);
         gWorld->setWorldID(gDatabase.getWorldID(scenarioWorldStr));
      }
      else if (nodeName == "AllowVeterancy")
      {
         bool allowVeterancy = true;
         xmlNode.getTextAsBool(allowVeterancy);
         mFlagAllowVeterancy = allowVeterancy;
      }
   }

//#ifdef NEWSCENARIOFILES
   if(bHasArtObjects)
   {
#ifdef ENABLE_FPS_LOG_FILE
      gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load AA");
#endif

      // Load ArtObjects
      BXMLNode xmlRootArtObjects(readerArtObjects.getRootNode());
      long nodeCountArtObjects=xmlRootArtObjects.getNumberChildren();   
      for(long i=0; i<nodeCountArtObjects; i++)
      {
         BXMLNode xmlNode(xmlRootArtObjects.getChild(i));
         const BPackedString nodeName(xmlNode.getName());

         if(nodeName=="Objects")
         {
            SCOPEDSAMPLE(BScenario_Node_Objects_Art)
            if(!gWorld)
               continue;

            long childCount=xmlNode.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               BXMLNode childNode(xmlNode.getChild(j));
               if(childNode.getName()=="Object")
                  loadObject(childNode);
            }
         }
      }

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::load bHasArtObjects");
         syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
         syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
      }
#endif
#endif
   }

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load BB");
#endif

   if(bHasSoundObjects)
   {
      // Load Sounds
      BXMLNode xmlRootSounds(readerSounds.getRootNode());
      long nodeCountSounds=xmlRootSounds.getNumberChildren();   
      for(long i=0; i<nodeCountSounds; i++)
      {
         BXMLNode xmlNode(xmlRootSounds.getChild(i));
         const BPackedString nodeName(xmlNode.getName());

         if(nodeName=="Objects")
         {
            SCOPEDSAMPLE(BScenario_Node_Objects_Sound)
            if(!gWorld)
               continue;

            long childCount=xmlNode.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               BXMLNode childNode(xmlNode.getChild(j));
               if(childNode.getName()=="Object")
                  loadObject(childNode);
            }
         }
      }

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::load bHasSoundObjects");
         syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
         syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
      }
#endif
#endif
   }
//#endif

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load CC");
#endif

   // Player starting units
   if (!gSaveGame.isLoading())
   {
      #ifdef SYNC_World
         syncWorldCode("BScenario::load setupPlayerStartingUnits");
      #endif
      setupPlayerStartingUnits(pSettings, playerList, isCoop, localPlayerID, localPlayerID2, positionList, gameType, leaveCamera);

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::load player starting units");
         syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
         syncWorldData("BScenario::load damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
      }
#endif
#endif
   }

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load DD");
#endif

   // Load UI lightset
   #ifdef SYNC_World
      syncWorldCode("BScenario::load loadUILightSet");
   #endif
   loadUILightSet();

   #ifdef SYNC_World
      syncWorldCode("BScenario::load preload tables");
   #endif
   gTriggerManager.getTableManager()->preloadTables();

   // Set sim bounds
   gWorld->setSimBounds(simBoundsMinX, simBoundsMinZ, simBoundsMaxX, simBoundsMaxZ);

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load EE");
#endif

   // Alert render control that we've loaded a new scenario so it can reset the tone mapper.
   // DO NOT load any assets (currently only UGX models) that use the D3D texture manager below here!
   gRenderControl.newScenario();

   if (reload)
   {
      if (!gSaveGame.isLoading())
      {
         gUIManager->resetMinimap();
      }
      gDeathManager.reset();
      gScoreManager.reset();
      gScoreManager.initializePlayers();
      //gArchiveManager.endScenarioLoad();
   }

   gRender.startupStatus("Scenario load complete");

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load FF");
#endif

   // We're done with these so get rid of them.
   mScenarioIDToEntityID.removeAll();
   for (int i=0; i<cMaximumSupportedPlayers; i++)
      mScenarioIDToPlayerID[i]=i;
   
   initDefaultMinimapTexture();

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load GG");
#endif

   if (gameType == BGameSettings::cGameTypeSkirmish)
   {
      // Startup game mode scripts and activate game mode tech.
      #ifdef SYNC_World
         syncWorldCode("BScenario::load startup game mode scripts and activate game mode tech");
      #endif
      long gameMode = 0;
      pSettings->getLong(BGameSettings::cGameMode, gameMode);
//-- FIXING PREFIX BUG ID 2837
      const BGameMode* pGameMode = gDatabase.getGameModeByID(gameMode);
//--
      if (pGameMode)
      {
         // Create the game mode's NPC
         BSimString npc = pGameMode->getNPC();
         if (!npc.isEmpty())
         {
//-- FIXING PREFIX BUG ID 2835
            const BPlayer* pPlayer = NULL;
//--
            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer* pCheckPlayer = gWorld->getPlayer(i);
               if (pCheckPlayer->isNPC() && pCheckPlayer->getName() == npc)
               {
                  pPlayer = pCheckPlayer;
                  break;
               }
            }
            if (!pPlayer)
            {
               int playerID = gWorld->getNumberPlayers();
               if (playerID == cMaximumSupportedPlayers)
               {
                  BASSERTM(0, "Can't create game mode NPC because there are too many players");
                  return false;
               }
               int teamID = gWorld->getNumberTeams();
               if (teamID == cMaximumSupportedTeams)
               {
                  BASSERTM(0, "Can't create game mode NPC because there are too many teams");
                  return false;
               }
               // AJL FIXME 4/26/08 - Allow NPC player values such as civ, resources, color, etc to be specified in gamemodes.xml
               // fixme - loc issue?
               BUString npcDisplayName = BSimUString(npc);  //not localised
               BPlayer* pPlayer=gWorld->createPlayer(-1, npc, npcDisplayName, 0, 0, gDatabase.getDifficultyDefault(), NULL, NULL, NULL, NULL, BPlayer::cPlayerTypeNPC, gWorld->getNumberPlayers(), false);
               if(!pPlayer)
               {
                  BASSERT(0);
                  return false;
               }
               BTeam* pTeam=gWorld->createTeam();
               if(!pTeam)
               {
                  BASSERT(0);
                  return false;
               }
               pTeam->addPlayer(playerID);
               pPlayer->setTeamID(teamID);
               // AJL FIXME 4/26/08 - For now force the NPC to be an enemy to all players. In the future make this more data driven 
               // to support other game modes that we may want in the future.
               long team1=pPlayer->getTeamID();
               for(long team2=0; team2<cMaximumSupportedTeams; team2++)
               {
                  BRelationType relationType;
                  if(team1==team2)
                     relationType = cRelationTypeAlly;
                  else if(team2==0)
                     relationType = cRelationTypeNeutral;
                  else
                     relationType=cRelationTypeEnemy;
                  gWorld->setTeamRelationType(team1, team2, relationType);
                  gWorld->setTeamRelationType(team2, team1, relationType);
               }
            }
         }

         // Fire off game mode specific tech
         int gameModeTechID = pGameMode->getTechID();
         if (gameModeTechID != -1)
         {
            for (int i = 1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer* pPlayer = gWorld->getPlayer(i);
               if (pPlayer->isComputerAI() || pPlayer->isHuman())
                  pPlayer->getTechTree()->activateTech(gameModeTechID, -1);
            }
         }

         // Startup game mode world script
         if (pGameMode->getWorldScript().length() > 0 && !gSaveGame.isLoading())
         {
            BTriggerScriptID triggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, pGameMode->getWorldScript());
            gTriggerManager.activateTriggerScript(triggerScriptID);
         }

         // Startup game mode player script for each player
         if (pGameMode->getPlayerScript().length() > 0 && !gSaveGame.isLoading() && !gConfig.isDefined(cConfigNoVictoryCondition))
         {
            if (pGameMode->getPlayerScript().length() > 0)
            {
               for (int i = 1; i<gWorld->getNumberPlayers(); i++)
               {
                  BPlayer* pPlayer = gWorld->getPlayer(i);
                  if (pPlayer->isComputerAI() || pPlayer->isHuman())
                  {
                     BTriggerScriptID triggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, pGameMode->getPlayerScript());
                     gTriggerManager.addExternalPlayerID(triggerScriptID, pPlayer->getID());
                     gTriggerManager.activateTriggerScript(triggerScriptID);
                  }
               }
            }
         }
      }
   }
   else if ((gameType == BGameSettings::cGameTypeScenario) && (gConfig.isDefined("SkirmishAIOnScenario")))
   {
      // [6/30/2008 xemu] test hack, eventually this should be specifiable somehow in the scenario 
      BPlayer* pPlayer = gWorld->getPlayer(2);
      pPlayer->setPlayerType(BPlayer::cPlayerTypeComputerAI);
   }

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load HH");
#endif


   // Group all of the Gaia units in the map into their own army
   BEntityGrouper::getOrCreateCommonArmyForSquads(mGaiaSquadList, 0);

   // This should be called AFTER all players and teams are created.
   #ifdef SYNC_World
      syncWorldCode("BScenario::load initializeKBsAndAIs");
   #endif
   gWorld->initializeKBsAndAIs();



   // Fatality stuff
   bool recomputeFatalityData = gConfig.isDefined(cConfigRecomputeFatalityData);
   if (recomputeFatalityData)
   {
      gFatalityManager.reset(true);
   }
   else
   {
      // Load fatality data if not already done
      if (!gFatalityManager.areFatalityAssetsLoaded())
         gFatalityManager.loadFatalityAssets();

      gFatalityManager.reset(false);
   }

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load II");
#endif

   #ifdef SYNC_World
      syncWorldCode("BScenario::load postloadProtoObjects");
   #endif
   gDatabase.postloadProtoObjects();

   // Record the object high water mark once the scenario finishes loading all objects
   gWorld->recordScenarioLoadObjectHighWaterMark();

#ifdef ENABLE_CAMERA_BOUNDARY_LINES

   // Set camera boundary lines
   //

   BUser *pMainUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BUser *pSecondaryUser = (gGame.isSplitScreen() ? gUserManager.getUser(BUserManager::cSecondaryUser) : NULL);
   
   pMainUser->clearCameraBoudnaryLines();
   if (pSecondaryUser)
      pSecondaryUser->clearCameraBoudnaryLines();


   // Add boundary lines for the world borders
   //

   float buffer = 8.0f;
   float worldMinX = gWorld->getSimBoundsMinX() + buffer;
   float worldMinZ = gWorld->getSimBoundsMinZ() + buffer;
   float worldMaxX = gWorld->getSimBoundsMaxX() - buffer;
   float worldMaxZ = gWorld->getSimBoundsMaxZ() - buffer;

   BCameraBoundaryLine mapBoundarylimitLine;
   mapBoundarylimitLine.mType = BCameraBoundaryLine::cBoundaryHover;
   mapBoundarylimitLine.mPoints.add(BVector(worldMinX, 0.0f, worldMinZ));
   mapBoundarylimitLine.mPoints.add(BVector(worldMinX, 0.0f, worldMaxZ));
   mapBoundarylimitLine.mPoints.add(BVector(worldMaxX, 0.0f, worldMaxZ));
   mapBoundarylimitLine.mPoints.add(BVector(worldMaxX, 0.0f, worldMinZ));
   mapBoundarylimitLine.mPoints.add(BVector(worldMinX, 0.0f, worldMinZ));

   pMainUser->addCameraBoundaryLine(mapBoundarylimitLine);
   if (pSecondaryUser)
      pSecondaryUser->addCameraBoundaryLine(mapBoundarylimitLine);


   // Add boundary lines from what is specified in the scenario
   // 

   BDesignObjectManager* pDesignObjectManager = gWorld->getDesignObjectManager();
   
   long numLines = pDesignObjectManager->getDesignLineCount();
   for(long i = 0; i < numLines; i++)
   {
      BDesignLine line = pDesignObjectManager->getDesignLine((uint)i);

      bool cameraBound_Hover = false;
      bool cameraBound_Camera = false;

      if(stricmp(line.mDesignData.mType, "CameraBoundary-Hover") == 0)
         cameraBound_Hover = true;
      if((stricmp(line.mDesignData.mType, "CameraBoundary-Camera") == 0) || (stricmp(line.mDesignData.mType, "CameraBoundary-HoverAndCamera") == 0))
         cameraBound_Camera = true;

      if(!cameraBound_Hover && !cameraBound_Camera)
         continue;

      BCameraBoundaryLine limitLine;

      long numPoints = line.mPoints.getNumber();
      for(long j = 0; j < numPoints; j++)
      {
         BVector point2D = line.mPoints[j];
         point2D.y = 0.0f;

         limitLine.mPoints.add(point2D);
      }

      if(cameraBound_Hover)
      {
         limitLine.mType = BCameraBoundaryLine::cBoundaryHover;

         pMainUser->addCameraBoundaryLine(limitLine);
         if (pSecondaryUser)
            pSecondaryUser->addCameraBoundaryLine(limitLine);
      }
      else
      {
         limitLine.mType = BCameraBoundaryLine::cBoundaryCamera;

         pMainUser->addCameraBoundaryLine(limitLine);
         if (pSecondaryUser)
            pSecondaryUser->addCameraBoundaryLine(limitLine);
      }
   }
#endif

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load JJ");
#endif

   if (gSaveGame.isLoading())
   {
      if (!gSaveGame.loadData())
      {
         gSaveGame.reset();
         return false;
      }
   }

#ifdef SYNC_FinalRelease
   syncFinalReleaseData("FRW BC", gGame.getLocalChecksum());
#endif

#ifdef SYNC_World
   syncWorldData("BScenario::load cConfigAIDisable", gConfig.isDefined(cConfigAIDisable));
   syncWorldData("BScenario::load cConfigAIShadow", gConfig.isDefined(cConfigAIShadow));
   syncWorldData("BScenario::load cConfigNoVismap", gConfig.isDefined(cConfigNoVismap));
   syncWorldData("BScenario::load cConfigNoRandomPlayerPlacement", gConfig.isDefined(cConfigNoRandomPlayerPlacement));;
   syncWorldData("BScenario::load cConfigDisableOneBuilding", gConfig.isDefined(cConfigDisableOneBuilding));;
   syncWorldData("BScenario::load cConfigBuildingQueue", gConfig.isDefined(cConfigBuildingQueue));;
   syncWorldData("BScenario::load cConfigUseTestLeaders", gConfig.isDefined(cConfigUseTestLeaders));;
   syncWorldData("BScenario::load cConfigEnableFlight", gConfig.isDefined(cConfigEnableFlight));;
   syncWorldData("BScenario::load cConfigNoBirthAnims", gConfig.isDefined(cConfigNoBirthAnims));;
   syncWorldData("BScenario::load cConfigVeterancy", gConfig.isDefined(cConfigVeterancy));;
   syncWorldData("BScenario::load cConfigTrueLOS", gConfig.isDefined(cConfigTrueLOS));;
   syncWorldData("BScenario::load cConfigNoDestruction", gConfig.isDefined(cConfigNoDestruction));;
   syncWorldData("BScenario::load cConfigCoopSharedResources", gConfig.isDefined(cConfigCoopSharedResources));;
   syncWorldData("BScenario::load cConfigPlayer1AI", gConfig.isDefined(cConfigPlayer1AI));;
   syncWorldData("BScenario::load cConfigAIAutoDifficulty", gConfig.isDefined(cConfigAIAutoDifficulty));;
   syncWorldData("BScenario::load cConfigAiInMP", gConfig.isDefined(cConfigAiInMP));;
   syncWorldData("BScenario::load cConfigRecoverEffectOffset", gConfig.isDefined(cConfigRecoverEffectOffset));;
   syncWorldData("BScenario::load cConfigAINoAttack", gConfig.isDefined(cConfigAINoAttack));;
   syncWorldData("BScenario::load cConfigNoVictoryCondition", gConfig.isDefined(cConfigNoVictoryCondition));;
   syncWorldData("BScenario::load cConfigPassThroughOwnVehicles", gConfig.isDefined(cConfigPassThroughOwnVehicles));;
   syncWorldData("BScenario::load cConfigEnableCapturePointResourceSharing", gConfig.isDefined(cConfigEnableCapturePointResourceSharing));;
   syncWorldData("BScenario::load cConfigNoUpdatePathingQuad", gConfig.isDefined(cConfigNoUpdatePathingQuad));
#if defined( _VINCE_ )
   syncWorldData("BScenario::load cConfigVinceEnableLog", gConfig.isDefined(cConfigVinceEnableLog));
#endif
   syncWorldData("BScenario::load cConfigSlaveUnitPosition", gConfig.isDefined(cConfigSlaveUnitPosition));;
   syncWorldData("BScenario::load cConfigTurning", gConfig.isDefined(cConfigTurning));;
   syncWorldData("BScenario::load cConfigHumanAttackMove", gConfig.isDefined(cConfigHumanAttackMove));;
   float val=0.0f;
   gConfig.get(cConfigPlatoonRadius, &val);
   syncWorldData("BScenario::load cConfigPlatoonRadius", val);;
   val = 0.0f;
   gConfig.get(cConfigProjectionTime, &val);
   syncWorldData("BScenario::load cConfigProjectionTime", val);;
   syncWorldData("BScenario::load cConfigMoreNewMovement3", gConfig.isDefined(cConfigMoreNewMovement3));;
   syncWorldData("BScenario::load cConfigOverrideGroundIK", gConfig.isDefined(cConfigOverrideGroundIK));;
   val = 0.0f;
   gConfig.get(cConfigOverrideGroundIKRange, &val);
   syncWorldData("BScenario::load cConfigOverrideGroundIKRange", val);;
   val = 0.0f;
   gConfig.get(cConfigOverrideGroundIKTiltFactor, &val);
   syncWorldData("BScenario::load cConfigOverrideGroundIKTiltFactor", val);;
   syncWorldData("BScenario::load cConfigDriveWarthog", gConfig.isDefined(cConfigDriveWarthog));;
   syncWorldData("BScenario::load cConfigEnableCorpses", gConfig.isDefined(cConfigEnableCorpses));;
   syncWorldData("BScenario::load cConfigDisablePathingLimits", gConfig.isDefined(cConfigDisablePathingLimits));;
   syncWorldData("BScenario::load cConfigDisableVelocityMatchingBySquadType", gConfig.isDefined(cConfigDisableVelocityMatchingBySquadType));;
   syncWorldData("BScenario::load cConfigActiveAbilities", gConfig.isDefined(cConfigActiveAbilities));;
   syncWorldData("BScenario::load cConfigAlpha", gConfig.isDefined(cConfigAlpha));
   syncWorldData("BScenario::load cConfigDisableWow", gConfig.isDefined(cConfigDisableWow));
   syncWorldData("BScenario::load cConfigEnableImpactLimits", gConfig.isDefined(cConfigEnableImpactLimits));
   syncWorldData("BScenario::load cConfigEnableMotionExtraction", gConfig.isDefined(cConfigEnableMotionExtraction));
   syncWorldData("BScenario::load cConfigWowRecord", gConfig.isDefined(cConfigWowRecord));
   syncWorldData("BScenario::load cConfigWowPlay", gConfig.isDefined(cConfigWowPlay));
   syncWorldData("BScenario::load cEnableHintSystem", gConfig.isDefined(cEnableHintSystem));
   syncWorldData("BScenario::load cHintSystemResetProfile", gConfig.isDefined(cHintSystemResetProfile));
#endif

   DWORD time2=timeGetTime();
   DWORD diff=time2-time;
   trace("ScenarioLoadTime=%u", diff);


#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigPIXTraceWorldLoad))
   {
      gConfig.remove(cConfigPIXTraceWorldLoad);
      XTraceStopRecording();
   }
#endif

#ifndef BUILD_FINAL
   if (gConfig.isDefined("PauseTriggers"))
      gTriggerManager.setFlagPaused(true);
#endif

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BScenario::load KK");
#endif


   // Now preload all talking heads
   gWorld->preloadTalkingHeadVideos();

   gScenarioLoad = false;
   return true;
}


//==============================================================================
// BScenario::unload
//==============================================================================
bool BScenario::unload()
{
   if (gArchiveManager.getArchivesEnabled())
   {
      unloadUnits();
   }

   //-- Unload sound banks
   uint numBanks = mScenarioSoundBankIDs.getSize();
   for(uint i = 0; i < numBanks; i++)
   {
      gSoundManager.unloadSoundBank(mScenarioSoundBankIDs[i], false);
   }
   mScenarioSoundBankIDs.clear();

   mLightSetScenarioDir.setNULL();
   mLightSetName.setNULL();

   mTriggerScriptIDs.clear();

   return true;
}


//==============================================================================
// BScenario::preloadPfxFiles
//==============================================================================
bool BScenario::preloadPfxFiles(const char* pPfxFileList)
{
   SCOPEDSAMPLE(BScenario_preloadPfxFiles)
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   
   
   BFile pfxFileList;
   if (!pfxFileList.open(cDirProduction, pPfxFileList, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BScenario::preloadPfxFiles: Unable to open %s", pPfxFileList);
      return false;
   }

   BStream* pStream = pfxFileList.getStream();

   trace("PFX preload %s begin", pPfxFileList);
      
   const int numUntilFlush = 8;
   uint currCount =0;
   for ( ; ; currCount++)
   {
      BString str;
      if (!pStream->readLine(str))
         break;
                              
      str.standardizePath();
      
      int i = str.findLeft("art\\");
      if (i != -1)
         str.crop(i + 4, str.length() - 1);
      
      if (str.isEmpty())
         continue;
         
      gConsoleOutput.resource("Preloading PFX file: %s, from %s\n", str.getPtr(), pPfxFileList);
      
      BParticleEffectDataHandle handle;
      gParticleGateway.getOrCreateData(str, handle);
      
      if (handle == -1)
         gConsoleOutput.error("Failed preloading PFX file: %s, from %s\n", str.getPtr(), pPfxFileList);

      if(currCount >= numUntilFlush)
      {
         gRenderThread.kickCommands();
         currCount=0;
      }
   }      
   
   pfxFileList.close();
   
   
   trace("PFX preload %s end", pPfxFileList);
     
#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadPfxFiles %s: %3.3f seconds", pPfxFileList, totalTime);
#endif

   return true;
}

//==============================================================================
// BScenario::preloadVisFiles
//==============================================================================
bool BScenario::preloadVisFiles(const char* pVisFileList)
{
   SCOPEDSAMPLE(BScenario_preloadVisFiles)

#ifdef SYNC_World
   syncWorldCode("BScenario::preloadVisFiles start");
#endif

#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   
   
   BFile visFileList;
   if (!visFileList.open(cDirProduction, pVisFileList, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BScenario::preloadVisFiles: Unable to open %s", pVisFileList);
#ifdef SYNC_World
      syncWorldData("BScenario::preloadVisFiles unable to open:", pVisFileList);
#endif
      return false;
   }

   BStream* pStream = visFileList.getStream();

   trace("VIS preload %s begin", pVisFileList);
      
   const int numUntilFlush = 8;
   uint currCount =0;
   for ( ; ; currCount++)
   {
      
      BString str;
      if (!pStream->readLine(str))
         break;
                              
      str.standardizePath();
      
      int i = str.findLeft("art\\");
      if (i != -1)
         str.crop(i + 4, str.length() - 1);
      
      if (str.isEmpty())
         continue;

      gConsoleOutput.resource("Preloading VIS file: %s, from %s\n", str.getPtr(), pVisFileList);
      #ifdef SYNC_World
         syncWorldData("BScenario::preloadVisFiles preloading VIS file:", str.getPtr());
      #endif

      long index = gVisualManager.getOrCreateProtoVisual(str, true);
      if (index < 0)
      {
         gConsoleOutput.error("Failed preloading VIS file: %s, from %s\n", str.getPtr(), pVisFileList);
         #ifdef SYNC_World
            syncWorldCode("BScenario::preloadVisFiles failed preloading VIS file");
         #endif
      }
      #ifdef SYNC_World
         BProtoVisual* pProto = gVisualManager.getProtoVisual(index, false);
         if (pProto)
         {
            syncWorldData("BScenario::preloadVisFiles index", index);
            if (pProto->getName().getPtr())
            {
               syncWorldData("BScenario::preloadVisFiles name", pProto->getName().getPtr());
            }
            syncWorldData("BScenario::preloadVisFiles damage template count", pProto->getDamageTemplateCount());
         }
      #endif


      if(currCount >= numUntilFlush)
      {
         gRenderThread.kickCommands();
         currCount=0;
      }
      #ifdef SYNC_World
         syncWorldData("BScenario::load number proto visuals", gVisualManager.getNumProtoVisuals());
         syncWorldData("BScenario::load visual manager damage template count", gVisualManager.getDamageTemplateCount());
      #endif
   }      
   
   
   
   visFileList.close();
   
   
   
   trace("VIS preload %s end", pVisFileList);
     
#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadVisFiles %s: %3.3f seconds", pVisFileList, totalTime);
#endif

#ifdef SYNC_World
   syncWorldCode("BScenario::preloadVisFiles end");
#endif

   return true;
}


//==============================================================================
// BScenario::preloadTfxFiles
//==============================================================================
bool BScenario::preloadTfxFiles(const char* pTfxFileList)
{
   SCOPEDSAMPLE(BScenario_preloadTfxFiles)
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   
   
   BFile tfxFileList;
   if (!tfxFileList.open(cDirProduction, pTfxFileList, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BScenario::preloadTfxFiles: Unable to open %s", pTfxFileList);
      return false;
   }

   BStream* pStream = tfxFileList.getStream();

   trace("TFX preload %s begin", pTfxFileList);
      
   const int numUntilFlush = 8;
   uint currCount =0;
   for ( ; ; currCount++)
   {
      
      BString str;
      if (!pStream->readLine(str))
         break;
                              
      str.standardizePath();
      
      int i = str.findLeft("art\\");
      if (i != -1)
         str.crop(i + 4, str.length() - 1);
      
      if (str.isEmpty())
         continue;
         
      gConsoleOutput.resource("Preloading TFX file: %s, from %s\n", str.getPtr(), pTfxFileList);
      
      if (gTerrainEffectManager.getOrCreateTerrainEffect(str, true) < 0)
         gConsoleOutput.error("Failed preloading TFX file: %s, from %s\n", str.getPtr(), pTfxFileList);

      if(currCount >= numUntilFlush)
      {
         gRenderThread.kickCommands();
         currCount=0;
      }
   }      
   
   tfxFileList.close();
   
   
   // Terrain tile type
   gTerrainEffectManager.getOrCreateTerrainEffect("effects\\terraineffects\\terrainTile", true);


   trace("TFX preload %s end", pTfxFileList);
     
#ifndef BUILD_FINAL   
   double totalTime = timer.getElapsedSeconds();
   trace("BScenario::preloadTfxFiles %s: %3.3f seconds", pTfxFileList, totalTime);
#endif

   return true;
}

//==============================================================================
// BScenario::preloadShapeFiles
//==============================================================================
bool BScenario::preloadShapeFiles()
{
   SCOPEDSAMPLE(BScenario_preloadShapes)
   trace("Shapes preload begin");

   gPhysics->getShapeManager().loadAll();

   trace("Shapes preload end");

   return true;
}

//==============================================================================
// BScenario::preloadUnits
//==============================================================================
bool BScenario::preloadUnits()
{
   SCOPEDSAMPLE(BScenario_preloadUnits)
   trace("Units preload begin");

   long numProtoObjects = gDatabase.getNumberProtoObjects();
   for(long i=0; i<numProtoObjects; i++)
   {
      BProtoObject* pProtoObject= gDatabase.getGenericProtoObject(i);
      if(!pProtoObject)
         continue;

      BProtoVisual *pProtoVisual = gVisualManager.getProtoVisual(pProtoObject->getProtoVisualIndex(), false);
      if(!pProtoVisual || !pProtoVisual->areAllAssetsLoaded())
         continue;

      pProtoObject->loadAllAssets();
   }

   trace("Units preload end");
   
   return true;
}


//==============================================================================
// BScenario::unloadUnits
//==============================================================================
bool BScenario::unloadUnits()
{
   SCOPEDSAMPLE(BScenario_preloadUnits)
   
   trace("Units unload begin");

   long numProtoObjects = gDatabase.getNumberProtoObjects();
   for(long i=0; i<numProtoObjects; i++)
   {
      BProtoObject* pProtoObject= gDatabase.getGenericProtoObject(i);
      if(!pProtoObject)
         continue;

      BProtoVisual *pProtoVisual = gVisualManager.getProtoVisual(pProtoObject->getProtoVisualIndex(), false);
      if(!pProtoVisual || !pProtoVisual->areAllAssetsLoaded())
         continue;

      pProtoObject->unloadAllAssets();
   }

   trace("Units unload end");
   
   return true;
}


//==============================================================================
// BScenario::initDefaultMinimapTexture
//==============================================================================
void BScenario::initDefaultMinimapTexture()
{      
   
   // if the scenario file did not override the texture for the minimap
   // look for a texture that has the same name as the map in the minimaps folder
//-- FIXING PREFIX BUG ID 2845
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   BSimString mapName;
   pSettings->getString(BGameSettings::cMapName, mapName);
   
   BSimString mapFilename;
   strPathGetFilename(mapName, mapFilename);

   //-- get the file path
   BSimString path("ui\\flash\\minimaps\\");
   path.append(mapFilename);

   trace("Scenario Minimap Path: %s", BStrConv::toA(path));

   //-- load the texture
   BManagedTextureHandle newMapHandle = gD3DTextureManager.getOrCreateHandle(path, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, false, false, "BScenario", false);
   
   if (newMapHandle != mMinimapTextureHandle)
   {
      //-- unload the old one
      if (mMinimapTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(mMinimapTextureHandle);

      //-- load the new one
      mMinimapTextureHandle = newMapHandle;
      if (mMinimapTextureHandle != cInvalidManagedTextureHandle)
      {      
         gD3DTextureManager.loadManagedTextureByHandle(mMinimapTextureHandle);
      }
   }   
}

//==============================================================================
// BScenario::getEntityIDFromScenarioObjectID
//==============================================================================
BEntityID BScenario::getEntityIDFromScenarioObjectID(long scenarioObjectID) const
{
   bhandle han = mScenarioIDToEntityID.find(scenarioObjectID);
   if (han != NULL)
      return (mScenarioIDToEntityID.get(han));
   else
      return (cInvalidObjectID);
}

//==============================================================================
// BScenario::getPlayerIDFromScenarioPlayerID
//==============================================================================
BPlayerID BScenario::getPlayerIDFromScenarioPlayerID(BPlayerID scenarioPlayerID) const
{
   if (scenarioPlayerID < 0 || scenarioPlayerID >= cMaximumSupportedPlayers)
      return cInvalidPlayerID;
   else
      return mScenarioIDToPlayerID[scenarioPlayerID];
}

//==============================================================================
// BScenario::loadTerrain
//==============================================================================
bool BScenario::loadTerrain(BXMLNode  xmlNode, const BSimString& scenarioDir, const BSimString& fileDir, const BFixedStringMaxPath & scnName)
{
   BFixedStringMaxPath terrainName=fileDir;
   BSimString temp;
   terrainName+=xmlNode.getText(temp);

   BFixedStringMaxPath fileName=scenarioDir;
   fileName+=terrainName;
   
   gRender.startupStatus("Loading terrain %s", fileName.getPtr());

   // loadXBOXTerrain handles the entire setup of gTerrain and appropriate subsystems
   bool loadVisRep = true;
   xmlNode.getAttribValueAsBool("LoadVisRep",loadVisRep);
   gTerrain.load(cDirProduction, fileName, cDirTerrain, loadVisRep);

   
   // Load the terrain sim rep
   BFixedStringMaxPath XSDName = scenarioDir;
   XSDName += scnName;
   if(!gTerrainSimRep.loadFromFile(cDirProduction, XSDName))
      return false;

   // Init the world terrain
   if(!gWorld->initTerrain(fileName))
      return false;

   return true;
}

//==============================================================================
// BScenario::loadObject
//==============================================================================
void BScenario::loadObject(BXMLNode  xmlNode)
{
   bool isSquad=false;
   long protoID=-1;
   if(xmlNode.getAttribValueAsBool("IsSquad", isSquad))
   {
      BSimString temp;
      if(isSquad)
         protoID=gDatabase.getProtoSquad(xmlNode.getTextPtr(temp));
      else
         protoID=gDatabase.getProtoObject(xmlNode.getTextPtr(temp));
   }
   else
   {
      BSimString temp;
      protoID=gDatabase.getProtoSquad(xmlNode.getTextPtr(temp));
      if(protoID!=-1)
         isSquad=true;
      else
         protoID=gDatabase.getProtoObject(xmlNode.getTextPtr(temp));
   }
   if(protoID==-1)
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - bad protoID");
      }
#endif
#endif
      return;
   }

   long playerID=-1;
   if(!xmlNode.getAttribValueAsLong("Player", playerID))
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - no playerID");
      }
#endif
#endif
      return;
   }
   playerID = getPlayerIDFromScenarioPlayerID(playerID);
   if(playerID<0 || playerID>=gWorld->getNumberPlayers())
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - bad playerID");
      }
#endif
#endif
      return;
   }

   // Get the ID that uniquely identifies this object from the scenario.  Should be non-negative.
   // We'll use this to map to the actual BEntityID on object creation.
   long scenarioID = -1;
   bool gotObjectID = xmlNode.getAttribValueAsLong("ID", scenarioID);
   BASSERT(gotObjectID && scenarioID >= 0);
   if (!gotObjectID || scenarioID < 0)
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - bad scenarioID");
      }
#endif
#endif
      return;
   }

   BVector position;
   if(!xmlNode.getAttribValueAsVector("Position", position))
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - no position");
      }
#endif
#endif
      return;
   }

   // Handle start unit position objects
   if(!isSquad && protoID==gDatabase.getPOIDUnitStart())
   {
      BScenarioStartUnitPos pos;
      pos.mID=scenarioID;
      pos.mPosition=position;
      mUnitStarts.add(pos);

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - start unit position object");
      }
#endif
#endif
      return;
   }

   // Handle rally point start position objects
   if(!isSquad && protoID==gDatabase.getPOIDRallyStart())
   {
      BScenarioStartUnitPos pos;
      pos.mID=scenarioID;
      pos.mPosition=position;
      mRallyStarts.add(pos);

#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - rally point start position object");
      }
#endif
#endif
      return;
   }

   // Don't load any units or squads from the scenario if loading a save game
   if (gSaveGame.isLoading())
   {
#ifndef BUILD_FINAL
#ifdef SYNC_World
      if (gArchiveManager.getArchivesEnabled())
      {
         syncWorldCode("BScenario::loadObject - save game");
      }
#endif
#endif

      // New save games include all objects.
      if (BWorld::mGameFileVersion >= 7)
         return;

      if (isSquad)
         return;
      const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
      if (pProtoObject)
      {
         int objectClass = pProtoObject->getObjectClass();
         if (objectClass != cObjectClassObject)
            return;
      }
   }

   #ifndef BUILD_FINAL
      if (gConfig.isDefined("NoScenarioObjects"))
      {
         const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
         if (pProtoObject)
         {
            int objectClass = pProtoObject->getObjectClass();
            if (objectClass == cObjectClassObject)
            {
#ifdef SYNC_World
               if (gArchiveManager.getArchivesEnabled())
               {
                  syncWorldCode("BScenario::loadObject - no scenario object");
               }
#endif
               return;
            }
         }
      }
      if (gConfig.isDefined("NoScenarioUnits"))
      {
         const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
         if (pProtoObject)
         {
            int objectClass = pProtoObject->getObjectClass();
            if (objectClass == cObjectClassUnit || objectClass == cObjectClassBuilding || objectClass == cObjectClassSquad)
            {
#ifdef SYNC_World
               if (gArchiveManager.getArchivesEnabled())
               {
                  syncWorldCode("BScenario::loadObject - no scenario unit");
               }
#endif
               return;
            }
         }
      }
   #endif

   // Optional
   float aoTintValue = 1.0f;
   xmlNode.getAttribValueAsFloat("TintValue", aoTintValue);

   BVector forward, right;
   if(xmlNode.getAttribValueAsVector("Forward", forward))
      forward.normalize();
   else
      forward=cZAxisVector;

   if(xmlNode.getAttribValueAsVector("Right", right))
      right.normalize();
   else
      right=cXAxisVector;

   // Flags
   bool noTieToGround=false;
   bool appearsBelowDecals=false;
   for(long i=0; i<xmlNode.getNumberChildren(); i++)
   {
      BXMLNode childNode(xmlNode.getChild(i));
      if(childNode.getName()=="Flag")
      {
         if(childNode.compareText("NoTieToGround") == 0)
            noTieToGround=true;
         // rg [10/2/07] - If the object was included in the simrep, it should appear below decals, not above them.
         else if(childNode.compareText("IncludeInSimRep") == 0)
            appearsBelowDecals=true;
      }
   }

   //visual variation
   int visualVariationIndex = -1;
   if(!xmlNode.getAttribValueAsInt("VisualVariationIndex", visualVariationIndex))
      visualVariationIndex=-1;

   // grab an exploration group id if we need to 
   int16 scenarioGroupId = -1;
   int16 explorationGroupId = -1;
   xmlNode.getAttribValueAsInt16("Group", scenarioGroupId);
   if (scenarioGroupId != -1)
   {
      bhandle han = mScenarioGroupIdToGroupId.find(scenarioGroupId);
      if (han)
         explorationGroupId = mScenarioGroupIdToGroupId.get(han);
   }

   // Create our object.
   BEntityID entityID = gWorld->createEntity(
      protoID, isSquad, playerID, position, forward, right, true, false, 
      noTieToGround, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, 
      cInvalidObjectID, false, aoTintValue, appearsBelowDecals, 0,
      visualVariationIndex, explorationGroupId);

#ifndef BUILD_FINAL
#ifdef SYNC_World
   if (gArchiveManager.getArchivesEnabled())
   {
      BEntity* pEntity = gWorld->getEntity(entityID);
      if (pEntity && pEntity->getName())
      {
         syncWorldData("BScenario::loadObject entity name", pEntity->getName()->getPtr());
      }
      else
      {
         syncWorldData("BScenario::loadObject entityID", entityID);
      }
      syncWorldData("BScenario::loadObject visual manager damage template count", gVisualManager.getDamageTemplateCount());
      syncWorldData("BScenario::loadObject damage template manager damage template count", gDamageTemplateManager.getDamageTemplateCount());
   }
#endif
#endif

   // Add our mapping from the scenario ID to the entity ID (squad, object, whatever) so we can fixup triggers and other systems that refer to scenario objects.
   mScenarioIDToEntityID.add(scenarioID, entityID);

   if (playerID == 0 && entityID != -1 && isSquad)
   {  
      mGaiaSquadList.add(entityID);
   }
}

//==============================================================================
// BScenario::loadPosition
//==============================================================================
void BScenario::loadObjective( BXMLNode xmlNode )
{
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();

   if( !pObjectiveManager )
   {
      BASSERTM( pObjectiveManager, "Invalid objective manager!" );
      return;
   }

   // Add an objective
   pObjectiveManager->addObjective();

   // New index
   long index = pObjectiveManager->getNumberObjectives() - 1;

   // Assign objective name
   #if defined( BUILD_DEBUG )      
      {
         BSimString temp;
         pObjectiveManager->setObjectiveName( index, xmlNode.getTextPtr(temp) );
      }
   #endif

   // Assign objective ID
   long ID = -1;
   xmlNode.getAttribValueAsLong( "id", ID );
   if( ID == -1 )
   {
      BASSERTM( false, "Scenario objective has no ID!" );
      return;
   }
   pObjectiveManager->setObjectiveID( index, ID );

   long childCount = xmlNode.getNumberChildren();
   for( long i = 0; i < childCount; i++ )
   {
      BXMLNode childNode(xmlNode.getChild( i ));
      if( childNode.getName() == "Flag" )
      {
         if( childNode.compareText("Required") == 0 )
         {
            // Set objective as required
            pObjectiveManager->setObjectiveRequired( index, true );
         }
         else if( childNode.compareText("Player1") == 0 )
         {
            // Set objective assignment to player 1
            pObjectiveManager->setObjectivePlayer( index, 1 );
         }
         else if( childNode.compareText("Player2") == 0 )
         {
            // Set objective assignment to player 2
            pObjectiveManager->setObjectivePlayer( index, 2 );
         }
         else if( childNode.compareText("Player3") == 0 )
         {
            // Set objective assignment to player 3
            pObjectiveManager->setObjectivePlayer( index, 3 );
         }
         else if( childNode.compareText("Player4") == 0 )
         {
            // Set objective assignment to player 4
            pObjectiveManager->setObjectivePlayer( index, 4 );
         }
         else if( childNode.compareText("Player5") == 0 )
         {
            // Set objective assignment to player 5
            pObjectiveManager->setObjectivePlayer( index, 5 );
         }
         else if( childNode.compareText("Player6") == 0 )
         {
            // Set objective assignment to player 6
            pObjectiveManager->setObjectivePlayer( index, 6 );
         }
      }
      else if( childNode.getName() == "Description" )
      {
         // Assign objective description
         BSimUString nodeText;
         childNode.getText(nodeText);

         long locID = decode(nodeText);
         if (locID >= 0)
         {
            pObjectiveManager->setObjectiveDescription( index, gDatabase.getLocStringFromID(locID));
         }
         else
         {
            BSimUString locString = nodeText;
            pObjectiveManager->setObjectiveDescription( index, nodeText);
         }
      }
      else if( childNode.getName() == "TrackerText" )
      {
         BSimUString nodeText;
         childNode.getText( nodeText );
         long locID = decode(nodeText);
         if( locID >= 0 )
         {
            pObjectiveManager->setObjectiveTrackerText( index, gDatabase.getLocStringFromID(locID) );
         }
      }
      else if( childNode.getName() == "TrackerDuration" )
      {
         uint duration = 8000;
         childNode.getTextAsUInt(duration);
         pObjectiveManager->setObjectiveTrackerDuration( index, duration );
      }
      else if( childNode.getName() == "MinTrackerIncrement" )
      {
         uint increment = 1;
         childNode.getTextAsUInt(increment);
         pObjectiveManager->setObjectiveMinTrackerIncrement( index, increment );
      }
      else if( childNode.getName() == "Hint" )
      {
         // Assign objective hint
         BSimUString nodeText;
         childNode.getText(nodeText);

         long locID = decode(nodeText);
         if (locID >= 0)
         {
            pObjectiveManager->setObjectiveHint( index, gDatabase.getLocStringFromID(locID));
         }
         else
         {
            BSimUString locString = nodeText;
            pObjectiveManager->setObjectiveHint( index, locString);
         }
      }
      else if (childNode.getName() == "Score")
      {
         int score = 0;
         childNode.getTextAsInt(score);
         pObjectiveManager->setObjectiveScore(index, (uint)score);
      }
      else if (childNode.getName() == "FinalCount")
      {
         int value = -1;
         if (childNode.getTextAsInt(value) && value > 0)
            pObjectiveManager->setObjectiveFinalCount(ID, value);
      }
   }   
}

//==============================================================================
// BScenario::loadObjectGroup
//==============================================================================
void BScenario::loadObjectGroup(BXMLNode  xmlNode)
{
   bool isExploreGroup = false;
   if(xmlNode.getAttribValueAsBool("ExploreGroup", isExploreGroup))
   {
      if (isExploreGroup)
      {
         int16 scenarioGroupId = -1;
         if (xmlNode.getAttribValueAsInt16("ID", scenarioGroupId))
         {
            // create a new exploration group from gWorld
            int16 worldGroupId = gWorld->getNewExplorationGroup();
            mScenarioGroupIdToGroupId.add(scenarioGroupId, worldGroupId);
         }
      }
   }
}

//==============================================================================
// BScenario::decode
//==============================================================================
long BScenario::decode(BSimUString& locString)
{
   // get the current value
   BSimUString workingString = locString;
   long ID = -1;
   // see if it is an encoded string
   long pos1 = workingString.findLeft(cUnicodeStringDelimeter);
   if (pos1 == 0)
   {
      long pos2 = workingString.findLeft(cUnicodeStringDelimeter, pos1 + 1);
      if (pos2>0)
      {
         // it is, so load the real string
         workingString.crop(2, pos2-1);
         ID = workingString.asLong();
      }
   }

   // if we have the Loc String, return the ID
   if (gDatabase.getLocStringIndex(ID) < 0)
      return -1;

   return ID;
}


//==============================================================================
// BScenario::loadMinimapData
//==============================================================================
void BScenario::loadMinimapData(BXMLNode xmlNode)
{
   for (int i = 0; i < xmlNode.getNumberChildren(); i++)
   {
      BXMLNode node = xmlNode.getChild(i);
      const BPackedString nodeName(node.getName());
      if (nodeName == "MinimapTexture")
      {                  
         //-- get the file path
         BSimString path;
         node.getText(path);
         if (path.isEmpty())
            continue;

         path.removeExtension();

         //-- load the texture
         mMinimapTextureHandle = gD3DTextureManager.getOrCreateHandle(path, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "Minimap");
      }
   }
}

//==============================================================================
// BScenario::loadPosition
//==============================================================================
void BScenario::loadPosition(BXMLNode  xmlNode, BScenarioPositionArray& positionList)
{
   BScenarioPosition pos;

   if(!xmlNode.getAttribValueAsVector("Position", pos.mPosition))
      return;

   bool defaultCamera=true;
   if(xmlNode.getAttribValueAsBool("DefaultCamera", defaultCamera) && !defaultCamera)
   {
      if(xmlNode.getAttribValueAsFloat("CameraYaw", pos.mCameraYaw) &&
         xmlNode.getAttribValueAsFloat("CameraPitch", pos.mCameraPitch) &&
         xmlNode.getAttribValueAsFloat("CameraZoom", pos.mCameraZoom))
      {
         pos.mDefaultCamera=false;
      }
   }

   xmlNode.getAttribValueAsLong("Number", pos.mNumber);
   xmlNode.getAttribValueAsLong("Player", pos.mPlayerID);
   xmlNode.getAttribValueAsInt("UnitStartObject1", pos.mUnitStartObjects[0]);
   xmlNode.getAttribValueAsInt("UnitStartObject2", pos.mUnitStartObjects[1]);
   xmlNode.getAttribValueAsInt("UnitStartObject3", pos.mUnitStartObjects[2]);
   xmlNode.getAttribValueAsInt("UnitStartObject4", pos.mUnitStartObjects[3]);
   xmlNode.getAttribValueAsInt("RallyStartObject", pos.mRallyStartObject);
   xmlNode.getAttribValueAsVector("Forward", pos.mForward);

   positionList.add(pos);  
}

//==============================================================================
//==============================================================================
void BScenario::loadRelationType(BXMLNode  xmlNode, BRelationType* pRelationType)
{
   long team1 = 0;
   if(!xmlNode.getAttribValueAsLong("Team", team1))
      return;
   if(team1>=0 && team1<cMaximumSupportedTeams)
   {
      long childCount=xmlNode.getNumberChildren();
      for(long i=0; i<childCount; i++)
      {
         BXMLNode childNode(xmlNode.getChild(i));
         const BPackedString childName(childNode.getName());
         if(childName=="Team")
         {
            long team2 = 0;
            if(childNode.getAttribValueAsLong("ID", team2))
            {
               if(team2>=0 && team2<cMaximumSupportedTeams)
               {
                  BSimString temp;
                  BRelationType relationType = gDatabase.getRelationType(childNode.getTextPtr(temp));
                  if(relationType!=-1)
                  {
                     long index=(team1*cMaximumSupportedTeams)+team2;
                     pRelationType[index]=relationType;
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
// BScenario::loadLight
//==============================================================================
void BScenario::loadLight(BXMLNode  xmlNode)
{
   bool havePos=false;
   BVector pos;
   long childCount=xmlNode.getNumberChildren();
   for(long i=0; i<childCount; i++)
   {
      BXMLNode child(xmlNode.getChild(i));
      const BPackedString childName(child.getName());
      if(childName=="Position")
      {
         if(child.getTextAsVector(pos))
            havePos=true;
         break;
      }
   }

   if(havePos)
   {
      long index=gLightVisualManager.createData(xmlNode);
      if(index!=-1)
      {
         BMatrix transform;
         transform.makeTranslate(pos);
         BLightVisualInstance* pInstance=gLightVisualManager.createInstance(index, &transform);
         if(pInstance)
         {
            if(!gWorld->addLight(pInstance))
               gLightVisualManager.releaseInstance(pInstance);
         }
      }
   }
}

//==============================================================================
// BScenario::reloadLightSetCallback
//==============================================================================
void BScenario::reloadLightSetCallback(const BString& path, uint data, uint id)
{
   // This is called whenever the GLS or FLS file has changed.
   
   if (!mLightSetName.isEmpty()) 
   {
      gConsoleOutput.status("Reloading light set %s", mLightSetName.getPtr());
      loadLightSet(mLightSetDirID, mLightSetScenarioDir, mLightSetName);
   }

   gSimSceneLightManager.reloadOtherLightSets();

}

//==============================================================================
// BScenario::reloadUILightSetCallback
//==============================================================================
void BScenario::reloadUILightSetCallback(const BString& path, uint data, uint id)
{
   // This is called whenever the UI GLS or FLS file has changed.
   gConsoleOutput.status("Reloading UI light set");
   loadUILightSet();
}

//==============================================================================
// BScenario::saveSHFillLight
//==============================================================================
bool BScenario::saveSHFillLight(const char* pFilename) const
{
   if ((mLightSetDirID == -1) || (!mLightSetName.length()))
      return false;
   
   const BSHLightParams& shLight = gSimSceneLightManager.getSHFillLight(cLCUnits);
   
   BSimString fillLightSetFilename;
   if(pFilename == NULL || strcmp(pFilename,"")==0)
   {
      fillLightSetFilename += mLightSetScenarioDir;
      fillLightSetFilename += mLightSetName;
      fillLightSetFilename += ".fls";
   }
   else
   {
      fillLightSetFilename += pFilename;
   }
   
   bool success = shLight.save(mLightSetDirID, fillLightSetFilename);
   if (!success)
      gConsoleOutput.error("Unable to save SH fill lights to file %s\n", fillLightSetFilename.getPtr());
   else
      gConsoleOutput.status("Saved SH fill lights to file %s\n", fillLightSetFilename.getPtr());
         
   return success;
}

//==============================================================================
// BScenario::reloadLightSet
//==============================================================================
void BScenario::reloadLightSet(void)
{
   if (!mLightSetName.isEmpty()) 
      loadLightSet(mLightSetDirID, mLightSetScenarioDir, mLightSetName);

   gSimSceneLightManager.reloadOtherLightSets();
}

//==============================================================================
// BScenario::loadLightSet
//==============================================================================
bool BScenario::loadLightSet(long dirID, const BSimString& scenarioDir, const BSimString& name)
{
   mLightSetDirID = dirID;
   mLightSetScenarioDir = scenarioDir;
   mLightSetName = name;

   BSimString lightSetFilename(scenarioDir);
   lightSetFilename += name;
   lightSetFilename += ".gls";
   
   BSimString fillLightSetFilename(scenarioDir);
   fillLightSetFilename += name;
   fillLightSetFilename += ".fls";
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.registerFunctor(BReloadManager::createPathArray(mLightSetDirID, lightSetFilename), BReloadManager::cFlagSynchronous, cThreadIndexSim, BReloadManager::BNotificationFunctor(this, &BScenario::reloadLightSetCallback));
   gReloadManager.registerFunctor(BReloadManager::createPathArray(mLightSetDirID, fillLightSetFilename), BReloadManager::cFlagSynchronous, cThreadIndexSim, BReloadManager::BNotificationFunctor(this, &BScenario::reloadLightSetCallback));

   BReloadManager::BPathArray paths;
   paths.pushBack("*.gls");
   gReloadManager.registerFunctor(paths,  BReloadManager::cFlagSynchronous | BReloadManager::cFlagSubDirs, cThreadIndexSim, BReloadManager::BNotificationFunctor(this, &BScenario::reloadLightSetCallback));
#endif


   BXMLReader reader;
   if (!reader.load(mLightSetDirID, lightSetFilename))
   {
      gConsoleOutput.output(cMsgError, "BScenario::loadLightSet: Error loading lightset %s\n", lightSetFilename.getPtr());
      return false;
   }
   
   gSimSceneLightManager.resetLightSet();  
   if (gWorld)
      gWorld->releaseLights();
      
   BXMLReader flsReader;
   if (!flsReader.load(mLightSetDirID, fillLightSetFilename))
      gConsoleOutput.output(cMsgError, "BScenario::loadLightSet: Error loading SH fill file %s -- using all-black SH fill\n", fillLightSetFilename.getPtr());
   else
      gConsoleOutput.output(cMsgResource, "BScenario::loadLightSet: Loading SH fill file %s\n", fillLightSetFilename.getPtr());
      
   BXMLNode rootNode(reader.getRootNode());
   if (!gSimSceneLightManager.loadLightSet(rootNode, flsReader.getValid() ? flsReader.getRootNode() : BXMLNode(NULL, 0U)))
   {
      gConsoleOutput.output(cMsgError, "BScenario::loadLightSet: Error parsing lightset %s\n", lightSetFilename.getPtr());
      return false;
   }
      
   uint totalLocalLights = 0;
   for (int childNodeIndex = 0; childNodeIndex < rootNode.getNumberChildren(); childNodeIndex++)
   {
      BXMLNode childNode(rootNode.getChild(childNodeIndex));
      
      if (childNode.getName() == "Light")      
      {
         loadLight(childNode);
         totalLocalLights++;
      }      
   }
   
   gConsoleOutput.output(cMsgResource, "BScenario::loadLightSet: Loaded light set %s, Local lights: %u\n", lightSetFilename.getPtr(), totalLocalLights);
   
   //gSimSceneLightManager.setSHFillLight(cLCTerrain, shLightParams);      
   //gSimSceneLightManager.setSHFillLight(cLCUnits, shLightParams);      
   
   return true;
}

//==============================================================================
// BScenario::loadUILightSet
//==============================================================================
bool BScenario::loadUILightSet()
{
   BSimString uiLightSetFilename("uiGlobalLights.gls");
   BSimString uiFillLightSetFilename("uiGlobalLights.fls");

#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.registerFunctor(BReloadManager::createPathArray(cDirScenario, uiLightSetFilename), BReloadManager::cFlagSynchronous, cThreadIndexSim, BReloadManager::BNotificationFunctor(this, &BScenario::reloadUILightSetCallback));
   gReloadManager.registerFunctor(BReloadManager::createPathArray(cDirScenario, uiFillLightSetFilename), BReloadManager::cFlagSynchronous, cThreadIndexSim, BReloadManager::BNotificationFunctor(this, &BScenario::reloadUILightSetCallback));
#endif

   BXMLReader reader;
   if (!reader.load(cDirScenario, uiLightSetFilename))
   {
      gConsoleOutput.output(cMsgError, "BScenario::loadUILightSet: Error loading lightset %s\n", uiLightSetFilename.getPtr());
      return false;
   }
   
   BXMLNode rootNode(reader.getRootNode());
   if (!gSimSceneLightManager.loadLightSet(rootNode, true))
   {
      gConsoleOutput.output(cMsgError, "BScenario::loadUILightSet: Error parsing lightset %s\n", uiLightSetFilename.getPtr());
      return false;
   }
      
   gConsoleOutput.output(cMsgResource, "BScenario::loadLightSet: Loaded light set %s\n", uiLightSetFilename.getPtr());
   
   BSHLightParams shLightParams;
   shLightParams.clear();
   
   bool success = shLightParams.load(cDirScenario, uiFillLightSetFilename);
   if (!success)
      gConsoleOutput.output(cMsgError, "BScenario::loadUILightSet: Error loading SH fill file %s -- using all-black SH fill\n", uiFillLightSetFilename.getPtr());
   else
      gConsoleOutput.output(cMsgResource, "BScenario::loadUILightSet: Loaded SH fill file %s\n", uiFillLightSetFilename.getPtr());
      
   gSimSceneLightManager.setSHFillLight(cLCUI, shLightParams);      
   
   return true;
}

//==============================================================================
// BScenario::loadPlayer
//==============================================================================
void BScenario::loadPlayer(BXMLNode xmlNode, BGameSettings* pSettings, BScenarioPlayerArray& playerList)
{
   BScenarioPlayer player;

   long resourceCount=gDatabase.getNumberResources();

   //const BXMLNode::BXMLAttributePtrArrayType& attrList=xmlNode.getAttributes();
   //long attrCount=attrList.getNumber();
   for(long i=0; i<xmlNode.getAttributeCount(); i++)
   {
      const BXMLAttribute  xmlAttr=xmlNode.getAttribute(i);
      const BPackedString name(xmlAttr.getName());
      
      BSimString temp;
      if(name=="Name")
      {
         //if(player.mName.length() == 0)
         {
            player.mName=xmlAttr.getValue(temp);
         }
      }         
      if(name=="LocalisedDisplayName")
      {
         long tempLong = 0;
         if(xmlAttr.getValueAsLong(tempLong))
         {            
            player.mLocalisedDisplayName = BSimUString(gDatabase.getLocStringFromID(tempLong));
         }
      }
      if(name=="Civ")
      {
         long civID=gDatabase.getCivID(xmlAttr.getValue(temp));
         if(civID!=-1)
            player.mCivID=civID;
      }
      else if(name=="Leader" || name=="Leader1")
         player.mLeaderID=gDatabase.getLeaderID(xmlAttr.getValue(temp));
      else if(name=="Color")
         xmlAttr.getValueAsLong(player.mColor);
      else if(name=="Team")
         xmlAttr.getValueAsLong(player.mTeam);
      else if(name=="UseStartingUnits")
         xmlAttr.getValueAsBool(player.mUseStartingUnits);
      else if(name=="Controllable")
         xmlAttr.getValueAsBool(player.mControllable);
      else if(name=="DefaultResources")
         xmlAttr.getValueAsBool(player.mDefaultResources);
      else if(name=="UsePlayerSettings")
         xmlAttr.getValueAsBool(player.mUsePlayerSettings);
      else
      {
         for(long j=0; j<resourceCount; j++)
         {
            long resourceID=gDatabase.getResource(name);
            if(resourceID==-1)
               continue;
            float amount = 0.0f;
            if(xmlAttr.getValueAsFloat(amount))
               player.mResources.set(resourceID, amount);
            break;
         }
      }
   }

   for(long i=0; i<xmlNode.getNumberChildren(); i++)
   {
      BXMLNode xmlNode2(xmlNode.getChild(i));
      const BPackedString nodeName(xmlNode2.getName());
      if(nodeName=="ForbidObjects")
      {
         long childCount=xmlNode2.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode2.getChild(j));
            if(childNode.getName()=="Object")
            {
               BSimString temp;
               long id=gDatabase.getProtoObject(childNode.getTextPtr(temp));
               if(id!=-1)
                  player.mForbidObjects.add(id);
            }
         }
      }
      else if(nodeName=="ForbidSquads")
      {
         long childCount=xmlNode2.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode2.getChild(j));
            if(childNode.getName()=="Squad")
            {
               BSimString temp;
               long id=gDatabase.getProtoSquad(childNode.getTextPtr(temp));
               if(id!=-1)
                  player.mForbidSquads.add(id);
            }
         }
      }
      else if(nodeName=="ForbidTechs")
      {
         long childCount=xmlNode2.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode2.getChild(j));
            if(childNode.getName()=="Tech")
            {
               BSimString temp;
               long id=gDatabase.getProtoTech(childNode.getTextPtr(temp));
               if(id!=-1)
                  player.mForbidTechs.add(id);
            }
         }
      }
      else if(nodeName=="Pops")
      {
         long childCount=xmlNode2.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            BXMLNode childNode(xmlNode2.getChild(j));
            if(childNode.getName()=="Pop")
            {
               BSimString typeName;
               if (childNode.getAttribValue("type", &typeName))
               {
                  long popID=gDatabase.getPop(typeName);
                  if(popID!=-1)
                  {
                     BLeaderPop pop;
                     pop.mID=popID;
                     childNode.getAttribValueAsFloat("max", pop.mMax);
                     childNode.getTextAsFloat(pop.mCap);
                     player.mLeaderPops.add(pop);
                  }
               }
            }
         }
      }
   }

   playerList.add(player);
}


//==============================================================================
// BScenario::setupPlayers
//
// Create controllable world players based on the number of players in the game settings
// and the number of controllable players defined in the scenario.
//
// Also create each non-controllable player defined in the scenario.
//
// Attach first user to the first controllable player.
//
// Setup starting units for players marked to use starting units in the scenario.
//==============================================================================
bool BScenario::setupPlayers(BGameSettings*  pSettings, BScenarioPlayerArray& playerList, BScenarioPositionArray& positionList, BRelationType* pRelationType, bool leaveCamera, long playerPlacementType, long playerPlacementSpacing, bool isCoop, long& localPlayerID, long& localPlayerID2, bool recordGame, bool playGame)
{
   // Create gaia player and team
   BUString gaiaUnicode = gDatabase.getLocStringFromID(25939); // "Gaia"
   BPlayer* pGaiaPlayer=gWorld->createPlayer(0, BSimString("Gaia"), gaiaUnicode, 0, 0, gDatabase.getDifficultyDefault(), NULL, NULL, NULL, NULL, BPlayer::cPlayerTypeNPC, 0, false);
   if(!pGaiaPlayer)
      return false;
   BTeam* pGaiaTeam=gWorld->createTeam();
   if(!pGaiaTeam)
      return false;
   pGaiaTeam->addPlayer(0);
   pGaiaPlayer->setTeamID(0);


   BOOL mpGame=FALSE;
   localPlayerID=-1;
   localPlayerID2=-1;

   BMPSession* pMPSession = gLiveSystem->getMPSession();

   BOOL isMPRunning = (pMPSession ? pMPSession->isGameRunning() : FALSE);

   if (isMPRunning)
   {
      mpGame=TRUE;
      localPlayerID=pMPSession->getControlledPlayer();
   }
   else if(gRecordGame.isPlaying())
   {
      mpGame=gRecordGame.isMultiplayer();
      localPlayerID=gRecordGame.getCurrentPlayerID();
   }

   long scenarioPlayerCount=playerList.getNumber();

   long settingsPlayerCount=0;
   pSettings->getLong(BGameSettings::cPlayerCount, settingsPlayerCount);

   long initPlayerID=-1;
   BVector lookAtPos(30.0f, 0.0f, 30.0f);
   bool defaultCamera=true;
   float cameraYaw=0.0f;
   float cameraPitch=0.0f;
   float cameraZoom=0.0f;

   long initPlayerID2=-1;
   BVector lookAtPos2(30.0f, 0.0f, 30.0f);
   bool defaultCamera2=true;
   float cameraYaw2=0.0f;
   float cameraPitch2=0.0f;
   float cameraZoom2=0.0f;

   // Assign teams
   long teamPlayerCount[cMaximumSupportedTeams];
   for(long i=0; i<cMaximumSupportedTeams; i++)
      teamPlayerCount[i]=0;
   long maxTeam=0;
   long settingsPlayerIndex=1;
   long aiPlayerNumber=1;
   int activePlayerCount=0;
   int calcPlayerID=1;
   for(long i=0; i<scenarioPlayerCount; i++)
   {
      BScenarioPlayer& scenarioPlayer=playerList[i];

      if ( scenarioPlayer.mControllable ) // if controllable
      {
         if (settingsPlayerIndex<=settingsPlayerCount)   // do I have a player define for slot? yes-active, no-don't create him.
         {
            scenarioPlayer.mActive = true;               // this is controllable and I have a player defined, then he's active
            if ( scenarioPlayer.mUsePlayerSettings )     // Does the scenario want to define the team, or can the settings?
               pSettings->getLong(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerTeam), scenarioPlayer.mTeam);   // scenario says the team can define the team
            activePlayerCount++;
         }
         settingsPlayerIndex++;
      }
      else
      {
         scenarioPlayer.mActive=true;  // we always create non-controllable players
      }

      if (scenarioPlayer.mActive)
      {
         // Build the scenario player ID to actual player ID mapping table
         mScenarioIDToPlayerID[i+1]=calcPlayerID;
         calcPlayerID++;
      }

      // we may not have created this player if
      if(scenarioPlayer.mActive)
      {
         long team=scenarioPlayer.mTeam;
         if(team<0 || team>=cMaximumSupportedTeams)
         {
            BASSERT(0);
            return false;
         }
         if(team>maxTeam)
            maxTeam=team;
         teamPlayerCount[team]++;
      }
   }

   if(teamPlayerCount[0]>0 && activePlayerCount>0)
   {
      long playersPerSide=activePlayerCount/2;
      if (playersPerSide==0)
         playersPerSide=1;
      long teamAvailableCount[3];
      for(long i=1; i<3; i++)
      {
         teamAvailableCount[i]=playersPerSide-teamPlayerCount[i];
         if(teamAvailableCount[i]<0)
            teamAvailableCount[i]=0;
      }

      for(long i=0; i<scenarioPlayerCount; i++)
      {
         BScenarioPlayer& scenarioPlayer=playerList[i];
         if(!scenarioPlayer.mActive)
            continue;
         long team=scenarioPlayer.mTeam;
         if(team!=0)
            continue;
         long numAvailableTeams=0;
         long teamAvailableIDs[3];
         long smallestTeam=getRandRange(cSimRand, 1, 2);
         for(long j=1; j<3; j++)
         {
            if(teamAvailableCount[j]>0)
            {
               teamAvailableIDs[numAvailableTeams]=j;
               numAvailableTeams++;
            }
            if(teamPlayerCount[j]<teamPlayerCount[smallestTeam])
               smallestTeam=j;
         }
         if(numAvailableTeams>0)
         {
            long index=getRandRange(cSimRand, 0, numAvailableTeams-1);
            team=teamAvailableIDs[index];
         }
         else
            team=smallestTeam;
         if(team>maxTeam)
            maxTeam=team;
         scenarioPlayer.mTeam=team;
         teamPlayerCount[team]++;
         teamAvailableCount[team]--;
         if(teamAvailableCount[team]<0)
            teamAvailableCount[team]=0;
      }
   }

   for(long i=1; i<=maxTeam; i++)
   {
      BTeam* pTeam=gWorld->createTeam();
      for(long j=0; j<scenarioPlayerCount; j++)
      {
//-- FIXING PREFIX BUG ID 2814
         const BScenarioPlayer& scenarioPlayer=playerList[j];
//--
         if(!scenarioPlayer.mActive)
            continue;
         if(scenarioPlayer.mTeam==i)
            pTeam->addPlayer(mScenarioIDToPlayerID[j+1]);
      }
   }

   // Diplomacy
   for(long i=0; i<gWorld->getNumberTeams(); i++)
   {
//-- FIXING PREFIX BUG ID 2817
      const BTeam* pTeam=gWorld->getTeam(i);
//--
      long team1=pTeam->getID();
      for(long team2=0; team2<cMaximumSupportedTeams; team2++)
      {
         BRelationType relationType;
         if(team1==team2)
            relationType = cRelationTypeAlly;
         else if(team1==0 || team2==0)
            relationType = cRelationTypeNeutral;
         else if(pRelationType)
         {
            long index=(team1*cMaximumSupportedTeams)+team2;
            relationType=pRelationType[index];
         }
         else
            relationType=cRelationTypeEnemy;
         gWorld->setTeamRelationType(team1, team2, relationType);
      }
   }

   // Setup player start positions
   setupPlayerPositions(playerPlacementType, playerPlacementSpacing, playerList, positionList);

   // Make sure we the localPlayerID is set
   if (localPlayerID==-1)
   {
      settingsPlayerIndex=1;
      for(long i=0; i<scenarioPlayerCount; i++)
      {
//-- FIXING PREFIX BUG ID 2818
         const BScenarioPlayer& scenarioPlayer=playerList[i];
//--
         if(!scenarioPlayer.mActive)
            continue;
         if(scenarioPlayer.mControllable)
         {
            localPlayerID=settingsPlayerIndex;
            break;
         }
      }
   }

   if (localPlayerID2==-1)
   {
      settingsPlayerIndex=1;
      for(long i=0; i<scenarioPlayerCount; i++)
      {
//-- FIXING PREFIX BUG ID 2819
         const BScenarioPlayer& scenarioPlayer=playerList[i];
//--
         if(!scenarioPlayer.mActive)
            continue;
         if(scenarioPlayer.mControllable)
         {
            if (settingsPlayerIndex != localPlayerID)
            {
               localPlayerID2=settingsPlayerIndex;
               break;
            }
            settingsPlayerIndex++;
         }
      }
   }

   // First create all the players
   settingsPlayerIndex=1;


   for(long i=0; i<scenarioPlayerCount; i++)
   {
      BScenarioPlayer& scenarioPlayer=playerList[i];
      if(!scenarioPlayer.mActive)
         continue;

      BSimString playerName;

      BPlayer* pPlayer=NULL;

      bool randomCiv = scenarioPlayer.mCivID < 1;
      bool randomLeader = scenarioPlayer.mLeaderID == -1;

      if (scenarioPlayer.mControllable)   // is this a controllable slot
      {
         if(settingsPlayerIndex<=settingsPlayerCount) // do we have settings defined for this slot? yes-set the player up, no-then no player
         {
            // Controllable player
            if(scenarioPlayer.mUsePlayerSettings)
            {
               pSettings->getLong(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerCiv), scenarioPlayer.mCivID);
               pSettings->getLong(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerLeader), scenarioPlayer.mLeaderID);

               // redo these checks based on the game settings
               randomCiv = scenarioPlayer.mCivID < 1;
               randomLeader = scenarioPlayer.mLeaderID == -1;
            }

            // Random civ
            if(scenarioPlayer.mCivID<1)
            {
               scenarioPlayer.mCivID=calcRandomCiv();

               if (isCoop)
               {
                  // Need to make the civ the same for all co-op players
                  for (long j=0; j<i; j++)
                  {
//-- FIXING PREFIX BUG ID 2815
                     const BScenarioPlayer& scenarioPlayer2=playerList[j];
//--
                     if (scenarioPlayer2.mActive && scenarioPlayer2.mControllable && scenarioPlayer2.mTeam==scenarioPlayer.mTeam)
                     {
                        scenarioPlayer.mCivID=scenarioPlayer2.mCivID;
                        break;
                     }
                  }
               }
            }

            // Random leader
            if(scenarioPlayer.mLeaderID==-1)
               scenarioPlayer.mLeaderID=calcRandomLeader(scenarioPlayer.mCivID);

            if(!pSettings->getString(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerName), playerName))
               playerName.format(   "Player %d", settingsPlayerIndex);  //not localised

            // player difficulty.
            float playerDifficulty = gDatabase.getDifficultyDefault();
            pSettings->getFloat(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerDifficulty), playerDifficulty);

            // human or ComputerAI
            uint8 playerType = BPlayer::cPlayerTypeHuman;
            long settingsPlayerType;
            pSettings->getLong( PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerType), settingsPlayerType);
            if (settingsPlayerType == BGameSettings::cPlayerComputer)
               playerType = BPlayer::cPlayerTypeComputerAI;

            // create the player            
            BUString playerNameDisplay;

            if (playerType == BPlayer::cPlayerTypeComputerAI)
            {
               playerNameDisplay.locFormat(gDatabase.getLocStringFromID(25941).getPtr(), aiPlayerNumber);
               aiPlayerNumber++;    // increment for our next AI player.
            }
            else
               playerNameDisplay = BSimUString(playerName);  // human (using gamer tag which is guaranteed to be ASCII)
            pPlayer=gWorld->createPlayer(settingsPlayerIndex, playerName, playerNameDisplay, scenarioPlayer.mCivID, scenarioPlayer.mLeaderID, playerDifficulty, &scenarioPlayer.mForbidObjects, &scenarioPlayer.mForbidSquads, &scenarioPlayer.mForbidTechs, &scenarioPlayer.mLeaderPops, playerType, i+1, false);

            if(pPlayer)
            {
               // Halwes - 6/26/2008 - Removed check for scenarioPlayer.mUseStartingUnits to be true since it can be false for co-op games that have player positions set.  
               //                      Fix for bug #6010.
               if (isCoop)
               {
                  // For scenarios that aren't setup specifically to be co-op (ones that have UseStartingUnits turned on),
                  // set this player position to the first player's position.
                  for (long j=0; j<i; j++)
                  {
//-- FIXING PREFIX BUG ID 2816
                     const BScenarioPlayer& scenarioPlayer2=playerList[j];
//--
                     if (scenarioPlayer2.mActive && scenarioPlayer2.mControllable && scenarioPlayer2.mTeam==scenarioPlayer.mTeam && scenarioPlayer2.mCivID==scenarioPlayer.mCivID)
                     {
                        scenarioPlayer.mPositionIndex=scenarioPlayer2.mPositionIndex;
                        scenarioPlayer.mPosition=scenarioPlayer2.mPosition;
                        scenarioPlayer.mDefaultCamera=scenarioPlayer2.mDefaultCamera;
                        scenarioPlayer.mCameraYaw=scenarioPlayer2.mCameraYaw;
                        scenarioPlayer.mCameraPitch=scenarioPlayer2.mCameraPitch;
                        scenarioPlayer.mCameraZoom=scenarioPlayer2.mCameraZoom;
                        break;
                     }
                  }
               }

               // If this is a network game, and this player is human controllable
               //   then fetch his AI tracking data from the network game session layer - eric
               if (isMPRunning)
               {
                  //We are going to query the Live (network) system for the block of tracking data for this playerID
                  byte* trackingMemoryBlock = gLiveSystem->getAITrackingMemoryBlockByPlayerID(settingsPlayerIndex);
                  if (trackingMemoryBlock)
                  {
                     BHumanPlayerAITrackingData* trackingData = new BHumanPlayerAITrackingData;
                     BASSERT(trackingData);
                     trackingData->loadValuesFromMemoryBlock(trackingMemoryBlock);
                     pPlayer->setTrackingData( trackingData );
                  }
               }

               // this is not pretty, but since this appears to be one of the only
               // places that we know if the civ/leader were random, I need to record
               // that information into the stats for later
               BStatsPlayer* pStats = pPlayer->getStats();
               if (pStats)
               {
                  if (randomCiv)
                     pStats->setRandomCiv();
                  if (randomLeader)
                     pStats->setRandomLeader();
               }
               
               // This is also not pretty - but I need to map the XUID out of the settings data into the player to be stored - eric
               XUID xuid = 0;
               pSettings->getUInt64( PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerXUID), xuid);
               pPlayer->setXUID(xuid);

               uint16 rank = 0;
               pSettings->getWORD(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerRank), rank);
               pPlayer->setRank(rank);

               if(settingsPlayerIndex==localPlayerID)
               {
                  if(scenarioPlayer.mPosition!=cInvalidVector && !leaveCamera)
                  {
                     lookAtPos=scenarioPlayer.mPosition;
                     lookAtPos.y+=100.0f;
                     gTerrainSimRep.getCameraHeightRaycast(lookAtPos, lookAtPos.y, true);
                     defaultCamera=scenarioPlayer.mDefaultCamera;
                     cameraYaw=scenarioPlayer.mCameraYaw;
                     cameraPitch=scenarioPlayer.mCameraPitch;
                     cameraZoom=scenarioPlayer.mCameraZoom;
                  }
                  initPlayerID=pPlayer->getID();
               }
               else if (settingsPlayerIndex==localPlayerID2)
               {
                  if(scenarioPlayer.mPosition!=cInvalidVector && !leaveCamera)
                  {
                     lookAtPos2=scenarioPlayer.mPosition;
                     lookAtPos2.y+=100.0f;
                     gTerrainSimRep.getCameraHeightRaycast(lookAtPos2, lookAtPos2.y, true);
                     defaultCamera2=scenarioPlayer.mDefaultCamera;
                     cameraYaw2=scenarioPlayer.mCameraYaw;
                     cameraPitch2=scenarioPlayer.mCameraPitch;
                     cameraZoom2=scenarioPlayer.mCameraZoom;
                  }
                  initPlayerID2=pPlayer->getID();
               }

               pPlayer->setTeamID(scenarioPlayer.mTeam);
            }
         }
         settingsPlayerIndex++;
      }
      else
      {
         // Non-controllable player

         // Random civ
         if(scenarioPlayer.mCivID<1)
            scenarioPlayer.mCivID=calcRandomCiv();

         // Random leader
         if(scenarioPlayer.mLeaderID==-1)
            scenarioPlayer.mLeaderID=calcRandomLeader(scenarioPlayer.mCivID);

         // player difficulty.
         float playerDifficulty = gDatabase.getDifficultyDefault();
         pSettings->getFloat(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerDifficulty), playerDifficulty);

         // All non-controllable player are of type BPlayer::cPlayerTypeNPC
         pPlayer=gWorld->createPlayer(-1, scenarioPlayer.mName, scenarioPlayer.mLocalisedDisplayName, scenarioPlayer.mCivID, scenarioPlayer.mLeaderID, playerDifficulty, &scenarioPlayer.mForbidObjects, &scenarioPlayer.mForbidSquads, &scenarioPlayer.mForbidTechs, &scenarioPlayer.mLeaderPops, BPlayer::cPlayerTypeNPC, i+1, false);
         if (pPlayer)
         {
            pPlayer->setTeamID(scenarioPlayer.mTeam);

            // again, need to record the random nature of the civ/leader into the stats
            BStatsPlayer* pStats = pPlayer->getStats();
            if (pStats)
            {
               if (randomCiv)
                  pStats->setRandomCiv();
               if (randomLeader)
                  pStats->setRandomLeader();
            }
         }
      }

      if(pPlayer && !scenarioPlayer.mDefaultResources)
      {
         long resourceCount=scenarioPlayer.mResources.getNumberResources();
         for(long resourceID=0; resourceID<resourceCount; resourceID++)
            pPlayer->setResource(resourceID, scenarioPlayer.mResources.get(resourceID));
      }
   }

   // Setup co-op
   if (isCoop)
   {
      gWorld->setFlagCoop(true);
      for (int i=1; i<gWorld->getNumberTeams(); i++)
      {
         BTeam* pTeam=gWorld->getTeam(i);
         if (pTeam->getNumberPlayers()>1)
         {
            int coopPlayerID=pTeam->getPlayerID(0);
            int coopCivID=gWorld->getPlayer(coopPlayerID)->getCivID();
            for (int j=1; j<pTeam->getNumberPlayers(); j++)
            {
               BPlayer* pPlayer=gWorld->getPlayer(pTeam->getPlayerID(j));
               if (pPlayer->isHuman() && pPlayer->getCivID()==coopCivID)
                  pPlayer->setCoopID(coopPlayerID);
            }
         }
      }
   }

   // Update obstruction manager player relation masks after all teams and players have been setup.
   gObsManager.rebuildPlayerRelationMasks();

   // Now finish setting up players
   
   long playerID=1;
   settingsPlayerIndex=1;
   for(long i=0; i<scenarioPlayerCount; i++)
   {
      BScenarioPlayer& scenarioPlayer=playerList[i];
      if(!scenarioPlayer.mActive)
         continue;
      BPlayer* pPlayer=NULL;
      if (scenarioPlayer.mControllable)
      {
         if (settingsPlayerIndex<=settingsPlayerCount)
         {
            pPlayer=gWorld->getPlayer(playerID);
            playerID++;
         }
         settingsPlayerIndex++;
      }
      else
      {
         pPlayer=gWorld->getPlayer(playerID);
         playerID++;
      }
      if(pPlayer)
      {
         if (!scenarioPlayer.mUsePlayerSettings && scenarioPlayer.mColor!=-1)
         {
            pPlayer->setColorIndex(scenarioPlayer.mColor);
            for (uint category = 0; category < cMaxPlayerColorCategories; category++)
               gWorld->setPlayerColor(
                  static_cast<BPlayerColorCategory>(category), 
                  pPlayer->getID(), 
                  scenarioPlayer.mColor);
                  //gDatabase.getPlayerColor(static_cast<BPlayerColorCategory>(category), scenarioPlayer.mColor));
         }
         else
         {
            int civID=pPlayer->getCivID();
            int civPlayer=1;
            for (int j=1; j<pPlayer->getID(); j++)
            {
               if (gWorld->getPlayer(j)->getCivID() == civID)
                  civPlayer++;
            }
            int colorIndex = gDatabase.getCivPlayerColor(civID, civPlayer);
            pPlayer->setColorIndex(colorIndex);
            for (uint category = 0; category < cMaxPlayerColorCategories; category++)
            {
               gWorld->setPlayerColor(
                  static_cast<BPlayerColorCategory>(category), 
                  pPlayer->getID(), 
                  colorIndex);
                  //gDatabase.getPlayerColor(static_cast<BPlayerColorCategory>(category), colorIndex));
            }
         }
      }
      float playerDifficulty = gDatabase.getDifficultyDefault();
      pSettings->getFloat(PSINDEX(settingsPlayerIndex, BGameSettings::cPlayerDifficulty), playerDifficulty);
      MVinceEventSync_PlayerJoinedMatch(pPlayer, scenarioPlayer.mPosition, playerDifficulty);
   }

   // Init primary user
   if(initPlayerID==-1)
   {
      for(long i=0; i<scenarioPlayerCount; i++)
      {
         BScenarioPlayer& scenarioPlayer=playerList[i];
         if(!scenarioPlayer.mActive)
            continue;
         if(!gWorld->getPlayer(1))
            break;
         initPlayerID=1;
         if(scenarioPlayer.mPosition!=cInvalidVector && !leaveCamera)
         {
            lookAtPos=scenarioPlayer.mPosition;
            lookAtPos.y+=100.0f;
            gTerrainSimRep.getCameraHeightRaycast(lookAtPos, lookAtPos.y, true);
            defaultCamera=scenarioPlayer.mDefaultCamera;
            cameraYaw=scenarioPlayer.mCameraYaw;
            cameraPitch=scenarioPlayer.mCameraPitch;
            cameraZoom=scenarioPlayer.mCameraZoom;
         }
         break;
      }
      if(initPlayerID==-1)
         initPlayerID=0;
   }
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   pUser->gameInit(initPlayerID, gWorld->getPlayer(initPlayerID)->getCoopID(), lookAtPos, leaveCamera, defaultCamera, cameraYaw, cameraPitch, cameraZoom);
   if (!playGame)
   {
      BHumanPlayerAITrackingData* pNewBlock = new BHumanPlayerAITrackingData();
      if (pNewBlock)
      {
         //If the player is local, he will have a profile - we need to push that profile data into that new tracking data object
         if (pUser->getProfile() && pUser->getProfile()->getAITrackingDataMemoryPointer())
            pNewBlock->loadValuesFromMemoryBlock(pUser->getProfile()->getAITrackingDataMemoryPointer());
         gWorld->getPlayer(initPlayerID)->setTrackingData(pNewBlock);
      }
   }

   // Init secondary user
   bool splitScreen = gGame.isSplitScreen();
   if (splitScreen)
   {
      if(initPlayerID2==-1)
      {
         for(long i=0; i<scenarioPlayerCount; i++)
         {
            BScenarioPlayer& scenarioPlayer=playerList[i];
            if(!scenarioPlayer.mActive)
               continue;
            if(!gWorld->getPlayer(2))
               break;
            initPlayerID2=2;
            if(scenarioPlayer.mPosition!=cInvalidVector && !leaveCamera)
            {
               lookAtPos2=scenarioPlayer.mPosition;
               lookAtPos2.y+=100.0f;
               gTerrainSimRep.getCameraHeightRaycast(lookAtPos2, lookAtPos2.y, true);
               defaultCamera2=scenarioPlayer.mDefaultCamera;
               cameraYaw2=scenarioPlayer.mCameraYaw;
               cameraPitch2=scenarioPlayer.mCameraPitch;
               cameraZoom2=scenarioPlayer.mCameraZoom;
            }
            break;
         }
         if(initPlayerID2==-1)
            initPlayerID2=0;
      }
      pUser = gUserManager.getUser(BUserManager::cSecondaryUser);
      pUser->gameInit(initPlayerID2, gWorld->getPlayer(initPlayerID2)->getCoopID(), lookAtPos2, leaveCamera, defaultCamera2, cameraYaw2, cameraPitch2, cameraZoom2);
      if (!playGame)
      {
         BHumanPlayerAITrackingData* pNewBlock = new BHumanPlayerAITrackingData();
         if (pNewBlock)
         {
            //If the player is local, he will have a profile - we need to push that profile data into that new tracking data object
            if (pUser->getProfile() && pUser->getProfile()->getAITrackingDataMemoryPointer())
               pNewBlock->loadValuesFromMemoryBlock(pUser->getProfile()->getAITrackingDataMemoryPointer());
            gWorld->getPlayer(initPlayerID)->setTrackingData(pNewBlock);
         }
      }
   }

   if (recordGame)
   {
      gRecordGame.recordPlayers();
      gRecordGame.recordUserPlayer();
   }
   else if (playGame)
   {
      gRecordGame.playPlayers();
      gRecordGame.playUserPlayer();
   }

   setupVoice();

   return true;
}

//==============================================================================
// Setup the voice channels for all the players
//==============================================================================
void BScenario::setupVoice()
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (pMPSession == NULL)
      return;

   if (!pMPSession->isGameRunning())
      return;

   BLiveVoice* pVoice = pMPSession->getVoice();
   if (pVoice == NULL)
      return;

   long totalTeam1 = 0;
   long totalTeam2 = 0;
   long count = gWorld->getNumberPlayers();
   for (long id=1; id < count; ++id)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(id);
      BASSERT(pPlayer);
      if (pPlayer->isHuman())
      {
         if (pPlayer->getTeamID() == 1)
         {
            ++totalTeam1;
         }
         else if (pPlayer->getTeamID() == 2)
         {
            ++totalTeam2;
         }
         else
         {
            BASSERTM(FALSE, "Found a human player that is not on team 1 or 2");
         }
      }
   }

   // if only one team exists of human players or each team only has one player, just dump everyone into the all channel
   if (totalTeam1 == 0 || totalTeam2 == 0 || (totalTeam1 == 1 && totalTeam2 == 1))
      pVoice->setAllChannels(BVoice::cGameSession, BVoice::cAll);
   else
   {
      for (long id=1; id < count; ++id)
      {
         const BPlayer* pPlayer = gWorld->getPlayer(id);
         BASSERT(pPlayer);
         if (pPlayer->isHuman())
         {
            if (pPlayer->getTeamID() == 1)
               pVoice->setChannel(BVoice::cGameSession, pPlayer->getXUID(), BVoice::cTeam1);
            else if (pPlayer->getTeamID() == 2)
               pVoice->setChannel(BVoice::cGameSession, pPlayer->getXUID(), BVoice::cTeam2);
         }
      }
   }

   // informs the voice system that the given session is now the primary one
   // this will update mute lists if necessary
   pVoice->setSession(BVoice::cGameSession);
}

//==============================================================================
// BScenario::setupPlayerPositions
//==============================================================================
bool BScenario::setupPlayerPositions(long playerPlacementType, long playerPlacementSpacing, BScenarioPlayerArray& playerList, BScenarioPositionArray& positionList)
{
   long playerCount=0;
   long nextPosition=-1;
   for(long i=0; i<playerList.getNumber(); i++)
   {
      if(!playerList[i].mActive)
         continue;
      playerCount++;
      long pos=-1;
      if(playerList[i].mPositionIndex!=-1 && playerList[i].mPositionIndex<positionList.getNumber())
      {
         pos=playerList[i].mPositionIndex;
      }
      else
      {
         for(;;)
         {
            nextPosition++;
            if(nextPosition>=positionList.getNumber())
               break;
            if(positionList[nextPosition].mPlayerID==-1)
            {
               pos=nextPosition;
               break;
            }
         }
      }
      if(pos!=-1)
      {
         playerList[i].mPosition=positionList[pos].mPosition;
         playerList[i].mForward=positionList[pos].mForward;
         playerList[i].mDefaultCamera=positionList[pos].mDefaultCamera;
         playerList[i].mCameraYaw=positionList[pos].mCameraYaw;
         playerList[i].mCameraPitch=positionList[pos].mCameraPitch;
         playerList[i].mCameraZoom=positionList[pos].mCameraZoom;
         playerList[i].mRallyStartObject=positionList[pos].mRallyStartObject;
         for (uint j=0; j<cMaxUnitStartObjects; j++)
            playerList[i].mUnitStartObjects[j]=positionList[pos].mUnitStartObjects[j];
      }
   }

   if(playerPlacementType==cPlayerPlacementFixed || gConfig.isDefined(cConfigNoRandomPlayerPlacement))
      return true;

   // This only works if we have enough positions passed in
   if(positionList.getNumber()<playerCount)
      return false;

   const long cMaxPositions=24;
   if(positionList.getNumber()>cMaxPositions || playerCount>cMaxPositions)
      return false;

   // Shuffle the players. Only support 1 or 2 teams.
   BSmallDynamicSimArray<long> shuffledPlayers;
   long                        shuffledTeamCount;
   if(!shufflePlayersTeamTogether(playerList, shuffledPlayers, shuffledTeamCount))
      return true;

   if(shuffledTeamCount<=0 || shuffledTeamCount>2)
      return true;

   // Get player count (should be the same as it was)
   playerCount=shuffledPlayers.getNumber();
   if(playerCount==0 || playerCount>cMaxPositions)
      return true;

   if(playerPlacementType==cPlayerPlacementConsecutive || playerPlacementType==cPlayerPlacementGrouped)
   {
      // Get the preset player positions.
      BSmallDynamicSimArray<long> positionLists[cMaxPositions];
      for(long i=0; i<positionList.getNumber(); i++)
      {
//-- FIXING PREFIX BUG ID 2821
         const BScenarioPosition& scenarioPos=positionList[i];
//--
         if(scenarioPos.mPlayerID!=-1)
            continue;
         for(long j=0; j<cMaxPositions; j++)
         {
            if(scenarioPos.mNumber==j+1)
            {
               positionLists[j].add(i);
               break;
            }
         }
      }

      if(playerPlacementType==cPlayerPlacementConsecutive)
      {
         // Build a list of consecutive positions.
         long consecutiveList[cMaxPositions];
         long consecutiveCount=0;
         for(long i=0; i<cMaxPositions; i++)
         {
            if(positionLists[i].getNumber()>0)
            {
               consecutiveList[consecutiveCount]=positionLists[i][0];
               consecutiveCount++;
            }
         }
         if(consecutiveCount<playerCount)
            return true;

         // Pick team starting positions
         long firstTeam=playerList[shuffledPlayers[0]].mTeam;

         long playersOnTeam=0;
         for(long i=0; i<playerList.getNumber(); i++)
         {
            if(!playerList[i].mActive)
               continue;
            if(playerList[i].mTeam==firstTeam)
               playersOnTeam++;
         }

         long index1=getRandMax(cRMRand, consecutiveCount-1);
         long index2=index1+playersOnTeam;

         long minGap=playerPlacementSpacing;
         long maxGap=consecutiveCount-playerCount;
         maxGap-=minGap;
         if(maxGap<0)
            maxGap=0;

         if(minGap>maxGap)
            minGap=maxGap;

         index2+=getRandRange(cRMRand, minGap, maxGap);

         if(index2>=consecutiveCount)
            index2-=consecutiveCount;

         for(long i=0; i<playerCount; i++)
         {
            long playerID=shuffledPlayers[i];
            long playerTeam=playerList[playerID].mTeam;
            long pos;
            if(playerTeam==firstTeam)
            {
               pos=consecutiveList[index1];
               index1++;
               if(index1==consecutiveCount)
                  index1=0;
            }
            else
            {
               pos=consecutiveList[index2];
               index2++;
               if(index2==consecutiveCount)
                  index2=0;
            }

            playerList[playerID].mPosition=positionList[pos].mPosition;
            playerList[playerID].mForward=positionList[pos].mForward;
            playerList[playerID].mDefaultCamera=positionList[pos].mDefaultCamera;
            playerList[playerID].mCameraYaw=positionList[pos].mCameraYaw;
            playerList[playerID].mCameraPitch=positionList[pos].mCameraPitch;
            playerList[playerID].mCameraZoom=positionList[pos].mCameraZoom;
            playerList[playerID].mRallyStartObject=positionList[pos].mRallyStartObject;
            for (uint j=0; j<cMaxUnitStartObjects; j++)
               playerList[playerID].mUnitStartObjects[j]=positionList[pos].mUnitStartObjects[j];
         }
      }
      else // playerPlacementType==cPlayerPlacementGrouped
      {
         // Place the players with one team at the first set of positions and the other team at the second set.
         long firstTeam=playerList[shuffledPlayers[0]].mTeam;
         long index1=0;
         long index2=0;
         for(long i=0; i<playerCount; i++)
         {
            long playerID=shuffledPlayers[i];
            long playerTeam=playerList[playerID].mTeam;

            long pos;
            if(playerTeam==firstTeam)
            {
               if(index1>=positionLists[0].getNumber())
                  return true;
               pos=positionLists[0][index1];
               index1++;
            }
            else
            {
               if(index2>=positionLists[1].getNumber())
                  return true;
               pos=positionLists[1][index2];
               index2++;
            }

            playerList[playerID].mPosition=positionList[pos].mPosition;
            playerList[playerID].mForward=positionList[pos].mForward;
            playerList[playerID].mDefaultCamera=positionList[pos].mDefaultCamera;
            playerList[playerID].mCameraYaw=positionList[pos].mCameraYaw;
            playerList[playerID].mCameraPitch=positionList[pos].mCameraPitch;
            playerList[playerID].mCameraZoom=positionList[pos].mCameraZoom;
            playerList[playerID].mRallyStartObject=positionList[pos].mRallyStartObject;
            for (uint j=0; j<cMaxUnitStartObjects; j++)
               playerList[playerID].mUnitStartObjects[j]=positionList[pos].mUnitStartObjects[j];
         }
      }
   }
   else // playerPlacementType==cPlayerPlacementRandom
   {
      int posCount=positionList.getNumber();
      int availPos[cMaxPositions];
      for (int i=0; i<posCount; i++)
         availPos[i]=i;
      int availCount=posCount;

      // Pick random starting positions for each player
      for(long i=0; i<playerList.getNumber(); i++)
      {
         if(!playerList[i].mActive)
            continue;
         long playerID=i;

         long availIndex=getRandRange(cRMRand, 0, availCount-1);
         long pos=availPos[availIndex];
         if (availIndex<availCount-1)
         {
            for (long j=availIndex; j<availCount; j++)
               availPos[j]=availPos[j+1];
         }
         availCount--;

         playerList[playerID].mPosition=positionList[pos].mPosition;
         playerList[playerID].mForward=positionList[pos].mForward;
         playerList[playerID].mDefaultCamera=positionList[pos].mDefaultCamera;
         playerList[playerID].mCameraYaw=positionList[pos].mCameraYaw;
         playerList[playerID].mCameraPitch=positionList[pos].mCameraPitch;
         playerList[playerID].mCameraZoom=positionList[pos].mCameraZoom;
         playerList[playerID].mRallyStartObject=positionList[pos].mRallyStartObject;
         for (uint j=0; j<cMaxUnitStartObjects; j++)
            playerList[playerID].mUnitStartObjects[j]=positionList[pos].mUnitStartObjects[j];
      }
   }

   return(true);
}

//==============================================================================
// BScenario::shufflePlayersTeamTogether
//==============================================================================
bool BScenario::shufflePlayersTeamTogether(BScenarioPlayerArray& playerList, BSmallDynamicSimArray<long>& shuffledPlayers, long& shuffledTeamCount)
{
   // Reset the shuffled players list.
   shuffledPlayers.setNumber(0);

   // Build a list of teams for players that haven't been placed.
   BSmallDynamicSimArray<long> tempTeams;
   for(long i=0; i<playerList.getNumber(); i++)
   {
      if(!playerList[i].mActive)
         continue;
      if(playerList[i].mPositionIndex==-1)
      {
         long team=playerList[i].mTeam;
         tempTeams.uniqueAdd(team);
      }
   }

   shuffledTeamCount=tempTeams.getNumber();
   if(shuffledTeamCount==0)
      return(false);

   // Shuffle the teams.
   for(long i=0; i<tempTeams.getNumber(); i++)
   {
      long index = getRandMax(cRMRand, tempTeams.getNumber()-1);
      bswap(tempTeams[i], tempTeams[index]);
   }

   for(long i=0; i<tempTeams.getNumber(); i++)
   {
      long team=tempTeams[i];

      // Save off where this team starts.
      long startIndex=shuffledPlayers.getNumber();

      // Add each player.
      for(long player=0; player<playerList.getNumber(); player++)
      {
         if(!playerList[player].mActive)
            continue;
         // Skip player if player has fixed position
         if(playerList[player].mPositionIndex!=-1)
            continue;
         // Skip player if not on the team we are processing
         if(playerList[player].mTeam!=team)
            continue;
         shuffledPlayers.add(player);
      }

      // Shuffle players within team.
      for(long j=startIndex; j<shuffledPlayers.getNumber(); j++)
         bswap(shuffledPlayers[j], shuffledPlayers[getRandRange(cRMRand, startIndex, shuffledPlayers.getNumber()-1)]);
   }

   return(true);
}

//==============================================================================
// BScenario::setupPlayerStartingUnits
//==============================================================================
void BScenario::setupPlayerStartingUnits(BGameSettings*  pSettings, BScenarioPlayerArray& playerList, bool isCoop, long localPlayerID, long localPlayerID2, BScenarioPositionArray& positionList, int gameType, bool leaveCamera)
{
   SCOPEDSAMPLE(BScenario_setupPlayerStartingUnits)
   // Now finish setting up players and create starting units
   long playerID=1;
   long settingsPlayerIndex=1;
   long scenarioPlayerCount=playerList.getNumber();

   long settingsPlayerCount=0;
   pSettings->getLong(BGameSettings::cPlayerCount, settingsPlayerCount);

   for(long i=0; i<scenarioPlayerCount; i++)
   {
      BScenarioPlayer& scenarioPlayer=playerList[i];
      if(!scenarioPlayer.mActive)
         continue;

      BPlayer* pPlayer=NULL;

      if (scenarioPlayer.mControllable)
      {
         if (settingsPlayerIndex<=settingsPlayerCount)
         {
            pPlayer=gWorld->getPlayer(playerID);
            playerID++;
         }
         settingsPlayerIndex++;
      }
      else
      {
         pPlayer=gWorld->getPlayer(playerID);
         playerID++;
      }

      if(pPlayer)
      {
         // Mark position used by this player
         if (gameType==BGameSettings::cGameTypeSkirmish && scenarioPlayer.mPosition!=cInvalidVector)
         {
            for (int j=0; j<positionList.getNumber(); j++)
            {
               BScenarioPosition& pos=positionList[j];
               if (pos.mPosition == scenarioPlayer.mPosition)
               {
                  pos.mUsed=true;
                  break;
               }
            }
         }

         // Look at pos.
         pPlayer->setLookAtPos(scenarioPlayer.mPosition);

         // Starting units
//-- FIXING PREFIX BUG ID 2828
         const BCiv* pCiv=pPlayer->getCiv();
//--
//-- FIXING PREFIX BUG ID 2829
         const BLeader* pLeader=pPlayer->getLeader();
//--
         bool createStartingUnits = false;
         if(scenarioPlayer.mUseStartingUnits && scenarioPlayer.mPosition!=cInvalidVector && pCiv && pLeader)
         {
            createStartingUnits = true;
            BVector forward=scenarioPlayer.mForward;
            BVector right;
            right.assignCrossProduct(cYAxisVector, forward);
            right.normalize();

            BMatrix mat;
            mat.makeOrient(forward, cYAxisVector, right);
            mat.setTranslation(scenarioPlayer.mPosition);

            syncWorldData("BScenario::setupPlayerStartingUnits playerID", pPlayer->getID());
            syncWorldData("BScenario::setupPlayerStartingUnits forward", forward);
            syncWorldData("BScenario::setupPlayerStartingUnits right", right);
            syncWorldData("BScenario::setupPlayerStartingUnits position", scenarioPlayer.mPosition);

            // Override positions
            BVector overridePos[cMaxUnitStartObjects+1];
            overridePos[0]=scenarioPlayer.mPosition;
            uint overridePosCount=1;
            for (uint j=0; j<cMaxUnitStartObjects; j++)
            {
               int startUnitPosID=scenarioPlayer.mUnitStartObjects[j];
               if (startUnitPosID!=-1)
               {
                  for (uint k=0; k<mUnitStarts.getSize(); k++)
                  {
//-- FIXING PREFIX BUG ID 2823
                     const BScenarioStartUnitPos& startUnitPos=mUnitStarts[k];
//--
                     if (startUnitPos.mID==startUnitPosID)
                     {
                        overridePos[overridePosCount]=startUnitPos.mPosition;
                        overridePosCount++;
                        break;
                     }
                  }
               }
            }

            // Starting Units
            uint overridePosNumber=0;
            const BStartingUnitArray& startingUnits = pLeader->getStartingUnits();
            uint numStartingUnits = startingUnits.getSize();
            for (uint i=0; i<numStartingUnits; i++)
            {
               // Skip creating starting buildings for coop players.
               bool skipCreate = false;
               if (isCoop && pPlayer->getCoopID() != cInvalidPlayerID)
               {
//-- FIXING PREFIX BUG ID 2824
                  const BProtoObject* pProtoObject = pPlayer->getProtoObject(startingUnits[i].mProtoObjectID);
//--
                  if (!pProtoObject)
                     skipCreate = true;
                  else if (pProtoObject->getObjectClass() == cObjectClassBuilding)
                     skipCreate = true;
               }

               syncWorldData("BScenario::setupPlayerStartingUnits unit protoID", startingUnits[i].mProtoObjectID);

               BVector unitPos;
               if (overridePosNumber < overridePosCount)
               {
                  syncWorldData("BScenario::setupPlayerStartingUnits overridePosNumber", (DWORD)overridePosNumber);
                  unitPos = overridePos[overridePosNumber];
                  overridePosNumber++;
               }
               else
               {
                  syncWorldData("BScenario::setupPlayerStartingUnits offset", startingUnits[i].mOffset);
                  //unitPos = scenarioPlayer.mPosition + startingUnits[i].mOffset;
                  mat.transformVectorAsPoint(startingUnits[i].mOffset, unitPos);
               }
               syncWorldData("BScenario::setupPlayerStartingUnits unitPos", unitPos);
               if (!skipCreate)
               {
                  gTerrainSimRep.getHeightRaycast(unitPos, unitPos.y, true);
                  BEntityID createdID = gWorld->createEntity(startingUnits[i].mProtoObjectID, false, pPlayer->getID(), unitPos, forward, right, true);
                  if (startingUnits[i].mBuildOtherID != cInvalidProtoObjectID)
                  {
                     BSquad* pCreatedSquad = gWorld->getSquad(createdID);
                     if (pCreatedSquad)
                     {
                        BUnit* pCreatedUnit = pCreatedSquad->getLeaderUnit();
                        if (pCreatedUnit)
                           pCreatedUnit->doBuildOther(pPlayer->getID(), pPlayer->getID(), startingUnits[i].mBuildOtherID, false, true, startingUnits[i].mDoppleOnStart);
                     }
                  }
               }
            }

            // Starting Squads
            const BStartingSquadArray& startingSquads = pLeader->getStartingSquads();
            uint numStartingSquads = startingSquads.getSize();
            for (uint i=0; i<numStartingSquads; i++)
            {
               syncWorldData("BScenario::setupPlayerStartingUnits squad protoID", startingSquads[i].mProtoSquadID);
               BVector squadPos;
               if (overridePosNumber < overridePosCount)
               {
                  syncWorldData("BScenario::setupPlayerStartingUnits overridePosNumber", (DWORD)overridePosNumber);
                  squadPos = overridePos[overridePosNumber];
                  overridePosNumber++;
               }
               else
               {
                  syncWorldData("BScenario::setupPlayerStartingUnits offset", startingSquads[i].mOffset);
                  //squadPos = scenarioPlayer.mPosition + startingSquads[i].mOffset;
                  mat.transformVectorAsPoint(startingSquads[i].mOffset, squadPos);
               }
               syncWorldData("BScenario::setupPlayerStartingUnits squadPos", squadPos);
               gTerrainSimRep.getHeightRaycast(squadPos, squadPos.y, true);
               BEntityID squadID = gWorld->createEntity(startingSquads[i].mProtoSquadID, true, pPlayer->getID(), squadPos, forward, right, true);
               BSquad* pSquad = gWorld->getSquad(squadID);
               if (!gConfig.isDefined(cConfigNoBirthAnims) && pSquad && startingSquads[i].mbFlyInSquad)
               {
                  const BProtoObject* pProtoObject = pSquad->getProtoObject();
                  BCueIndex cue = (pProtoObject && (pSquad->getPlayerID() == localPlayerID)) ? pProtoObject->getSound(cObjectSoundCreate) : cInvalidCueIndex;
                  BSimOrder* pOrder = gSimOrderManager.createOrder();
                  if (pOrder)
                     pOrder->setPriority(BSimOrder::cPrioritySim);
                  BSquadActionTransport::flyInSquad(pOrder, pSquad, NULL, squadPos, forward, right, NULL, pPlayer->getID(), pCiv->getTransportProtoID(), false, cInvalidVector, false, cue, false, false, cInvalidVector);
               }
            }
         }

         if (pPlayer)
         {
            // Rally point
            BVector rallyPos=cInvalidVector;
            if (scenarioPlayer.mRallyStartObject != -1)
            {
               for (uint k=0; k<mRallyStarts.getSize(); k++)
               {
//-- FIXING PREFIX BUG ID 2825
                  const BScenarioStartUnitPos& rallyStart=mRallyStarts[k];
//--
                  if (rallyStart.mID==scenarioPlayer.mRallyStartObject)
                  {
                     rallyPos=rallyStart.mPosition;
                     break;
                  }
               }
            }

            if (rallyPos!=cInvalidVector)
            {
               gTerrainSimRep.getHeight(rallyPos, true);
               bool baseRallySet = false;
               if (createStartingUnits)
               {
                  BEntityIDArray bases;
                  if (pPlayer->getUnitsOfType(gDatabase.getOTIDBase(), bases) == 1)
                  {
                     BUnit* pBase = gWorld->getUnit(bases[0]);
                     if (pBase)
                     {
                        BEntityID parkingLot = pBase->getAssociatedParkingLot();
                        if (parkingLot)
                        {
                           BUnit* pParkingLot = gWorld->getUnit(parkingLot);
                           if (pParkingLot)
                              pParkingLot->setRallyPoint(rallyPos, cInvalidObjectID, pPlayer->getID());
                           else
                              pBase->setRallyPoint(rallyPos, cInvalidObjectID, pPlayer->getID());
                           baseRallySet = true;
                        }
                     }
                  }
               }
               if (!baseRallySet)
                  pPlayer->setRallyPoint(rallyPos, cInvalidObjectID);
            }

            // Set camera to look at rally pos
            if (!leaveCamera && pPlayer->getUser())
            {
               BVector lookAtPos = cInvalidVector;
               if (pPlayer->haveRallyPoint())
                  lookAtPos = pPlayer->getRallyPoint();
               if (createStartingUnits)
               {
                  BEntityIDArray bases;
                  if (pPlayer->getUnitsOfType(gDatabase.getOTIDBase(), bases) == 1)
                  {
                     BUnit* pBase = gWorld->getUnit(bases[0]);
                     if (pBase)
                     {
                        BEntityID parkingLot = pBase->getAssociatedParkingLot();
                        if (parkingLot)
                        {
                           BUnit* pParkingLot = gWorld->getUnit(parkingLot);
                           if (pParkingLot && pParkingLot->haveRallyPoint(playerID))
                              lookAtPos = pParkingLot->getRallyPoint(playerID);
                           else if (pBase->haveRallyPoint(playerID))
                              lookAtPos = pBase->getRallyPoint(playerID);
                        }
                     }
                  }
               }
               if (lookAtPos != cInvalidVector)
               {
                  BUser* pUser = pPlayer->getUser();
                  if(pUser)
                  {
                     pUser->computeClosestCameraHoverPointVertical(lookAtPos, lookAtPos);
                     pUser->setCameraHoverPoint(lookAtPos);
                     pUser->setFlagTeleportCamera(true);
                  }
               }
            }
         }
      }
   }

   // Create objects for empty player start locations
   if (gameType==BGameSettings::cGameTypeSkirmish)
   {
      int emptyBaseObject = gDatabase.getSkirmishEmptyBaseObject();
      if (emptyBaseObject != -1)
      {
         for (int j=0; j<positionList.getNumber(); j++)
         {
            BScenarioPosition& pos=positionList[j];
            if (!pos.mUsed)
            {
               BVector unitPos=pos.mPosition;
               BVector forward=pos.mForward;
               BVector right;
               right.assignCrossProduct(cYAxisVector, forward);
               right.normalize();

               syncWorldData("BScenario::setupPlayerStartingUnits empty start loc index", j);
               syncWorldData("BScenario::setupPlayerStartingUnits empty start loc pos", unitPos);
               gTerrainSimRep.getHeightRaycast(unitPos, unitPos.y, true);
               BEntityID createdID = gWorld->createEntity(emptyBaseObject, false, 0, unitPos, forward, right, true);
            }
         }
      }
   }
}

//==============================================================================
// BScenario::save
//==============================================================================
bool BScenario::save(const char*  pFileName)
{
   BXMLWriter writer;
   if(!writer.create(cDirScenario, pFileName))
      return false;

   writer.startItem("Scenario");
   writer.addItem("Terrain", gWorld->getTerrainName());

   BSimString lightSetFile=mLightSetName;

   BSimString fileDir;
   strPathGetDirectory(lightSetFile, fileDir);
   BSimString scenarioDir;
   eFileManagerError result = gFileManager.getDirListEntry(scenarioDir, cDirScenario);
   BVERIFY(cFME_SUCCESS == result);
   BSimString mainDir;
   result = gFileManager.getDirListEntry(mainDir, cDirProduction);
   BVERIFY(cFME_SUCCESS == result);
   scenarioDir.remove(0, mainDir.length());

   if(lightSetFile.compare(scenarioDir, false, scenarioDir.length())==0)
      lightSetFile.remove(0, scenarioDir.length());

   writer.addItem("Lightset", lightSetFile);

   writer.startItem("Objects");

   writer.endItem(); // Objects

   writer.endItem(); // Scenario

   return true;
}

//==============================================================================
// BScenario::saveObject
//==============================================================================
void BScenario::saveObject(BXMLWriter*  pWriter, BObject*  pObject, const BProtoObject*  pProtoObject, BVector position, BVector forward, BVector right, long count)
{
   pWriter->startItem("Object", pProtoObject->getName());
   pWriter->addAttribute("Player", pObject->getPlayerID());
   pWriter->addAttribute("Position", position);
   pWriter->addAttribute("Forward", forward);
   pWriter->addAttribute("Right", right);
   if(count>0)
      pWriter->addAttribute("Count", count);
   pWriter->endItem(); // Object
}


//==============================================================================
// BScenario::loadOtherLightset
//==============================================================================
bool BScenario::loadOtherLightset(BXMLNode xmlNode, const BSimString& scenarioDir)
{
   long lightsetID = -1;
   bool gotLightsetID = xmlNode.getAttribValueAsLong("ID", lightsetID);

   if(!gotLightsetID)
      lightsetID = 0;

   if (lightsetID < 0)
      return false;

   BSimString temp;
   if (!xmlNode.getText(temp).isEmpty())
   {
      BSimString fileName=scenarioDir;
      fileName+=temp;
      strPathAddExtension(fileName, "gls");

      // Create new lightset
      BRawLightSettings *pLightSet = new BRawLightSettings();

      BXMLReader reader;
      if(!reader.load(0, fileName.getPtr()))
      {
         return false;
      }

      BXMLNode rootNode(reader.getRootNode());
      pLightSet->loadLightSet(fileName, rootNode);

      //load our fls for animation
      fileName=scenarioDir;
      fileName+=temp;
      strPathAddExtension(fileName, "fls");
      pLightSet->mSHFillLights.clear();
      pLightSet->mSHFillLights.load(-1,fileName);

      pLightSet->mId = lightsetID;

      gSimSceneLightManager.addLightSet(pLightSet);

   }
   
   return true;
}

//==============================================================================
// BScenario::loadCinematic
//==============================================================================
bool BScenario::loadCinematic(BXMLNode xmlNode, const BSimString& scenarioDir)
{
   long cinematicID = -1;
   bool gotCinematicID = xmlNode.getAttribValueAsLong("ID", cinematicID);

   if(!gotCinematicID)
      cinematicID = 0;

   if (cinematicID < 0)
      return false;

   BSimString temp;
   if (!xmlNode.getText(temp).isEmpty())
   {
      BSimString fileName=scenarioDir;
      fileName+=temp;

      strPathAddExtension(fileName, "cin");

      // Create new cinematic
      BCinematic *pCinematic = new BCinematic();

      pCinematic->init(cDirProduction, fileName.getPtr());
      if(!pCinematic->load())
      {
         delete pCinematic;
         gConsoleOutput.error("BScenario::loadCinematic: Unable to load %s\n", fileName.getPtr());
         return true;
      }

      gWorld->addCinematic(pCinematic, cinematicID);
   }
   
   return true;
}

//==============================================================================
// BScenario::loadTalkingHead
//==============================================================================
bool BScenario::loadTalkingHead(BXMLNode xmlNode, const BSimString& dir)
{
   long id = -1;
   bool gotID = xmlNode.getAttribValueAsLong("ID", id);

   if(!gotID)
      id = 0;

   if (id < 0)
      return false;

   BSimString temp;
   if (!xmlNode.getText(temp).isEmpty())
   {
      BSimString fileName=dir;
      fileName+=temp;
      strPathAddExtension(fileName, "bik");

      gWorld->addTalkingHead(fileName.getPtr(), id, true);
   }

   return true;
}

//==============================================================================
// BScenario::calcRandomCiv
//==============================================================================
long BScenario::calcRandomCiv()
{
   long civ=getRandRange(cSimRand, 1, gDatabase.getNumberCivs()-1);
   return civ;
}

//==============================================================================
// BScenario::calcRandomLeader
//==============================================================================
long BScenario::calcRandomLeader(long civ)
{
   long leaderCount=0;
   for(long j=0; j<gDatabase.getNumberLeaders(); j++)
   {
//-- FIXING PREFIX BUG ID 2832
      const BLeader* pLeader=gDatabase.getLeader(j);
//--
      if(pLeader->mLeaderCivID==civ)
      {
         if(pLeader->mTest)
         {
            if(gConfig.isDefined(cConfigUseTestLeaders))
               return j;                                                  
            continue;
         }
         leaderCount++;
      }
   }
   if(leaderCount>0)
   {
      long leaderIndex=getRandRange(cSimRand, 0, leaderCount-1);
      leaderCount=0;
      for(long j=0; j<gDatabase.getNumberLeaders(); j++)
      {
//-- FIXING PREFIX BUG ID 2833
         const BLeader* pLeader=gDatabase.getLeader(j);
//--
         if(pLeader->mLeaderCivID==civ && !pLeader->mTest)
         {
            if(leaderCount==leaderIndex)
               return j;
            leaderCount++;
         }
      }
   }
   return -1;
}

//==============================================================================
// BScenario::receiveEvent
//==============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BScenario::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
      if(gModeManager.inModeGame())   
      {
         //load(true, true);
         float ignoreTime = 0.0f;
         gConfig.get(cConfigReloadScenarioIgnoreTime, &ignoreTime);
         if (!mFlagReload && gWorld && gWorld->getGametimeFloat() > ignoreTime)
         {
            mReloadTime = timeGetTime();
            mFlagReload = true;
         }
      }
   }
      
   return false;
}
#endif

//==============================================================================
// BScenario::loadSoundBanks
//==============================================================================
void BScenario::loadSoundBanks()
{
   uint count = mScenarioSoundBankNames.size();
   for (uint i=0; i<count; i++)
   {
      uint32 bankID = gSoundManager.loadSoundBank(mScenarioSoundBankNames[i], false);
      if (bankID != AK_INVALID_BANK_ID)
         mScenarioSoundBankIDs.add(bankID);
   }
   mScenarioSoundBankNames.clear();
}
