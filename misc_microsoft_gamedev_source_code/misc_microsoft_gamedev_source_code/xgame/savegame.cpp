//==============================================================================
// savegame.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "savegame.h"
#include "ability.h"
#include "action.h"
#include "actionmanager.h"
#include "ai.h"
#include "aimanager.h"
#include "aimission.h"
#include "aimissionscore.h"
#include "aimissiontarget.h"
#include "aimissiontargetwrapper.h"
#include "aipowermission.h"
#include "aiteleporterzone.h"
#include "aitopic.h"
#include "archiveManager.h"
#include "army.h"
#include "bid.h"
#include "bidMgr.h"
#include "camera.h"
#include "ChatManager.h"
#include "configsgame.h"
#include "convexhull.h"
#include "cost.h"
#include "damagetemplate.h"
#include "database.h"
#include "dopple.h"
#include "entity.h"
#include "fatalitymanager.h"
#include "Formation2.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "hintengine.h"
#include "HintManager.h"
#include "kb.h"
#include "kbbase.h"
#include "kbsquad.h"
#include "LiveSystem.h"
#include "modemanager.h"
#include "modemenu.h"
#include "object.h"
#include "objectanimationstate.h"
#include "path.h"
#include "pathingLimiter.h"
#include "physics.h"
#include "physicsinfomanager.h"
#include "piecewise.h"
#include "platoon.h"
#include "player.h"
#include "power.h"
#include "powermanager.h"
#include "projectile.h"
#include "protoobject.h"
#include "protopower.h"
#include "protosquad.h"
#include "prototech.h"
#include "recordgame.h"
#include "scenario.h"
#include "scoremanager.h"
#include "selectionmanager.h"
#include "shapemanager.h"
#include "SimOrder.h"
#include "SimTarget.h"
#include "simtypes.h"
#include "squad.h"
#include "squadplotter.h"
#include "statsManager.h"
#include "tactic.h"
#include "team.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggercondition.h"
#include "triggereffect.h"
#include "UICallouts.h"
#include "uimanager.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "visiblemap.h"
#include "weapontype.h"
#include "world.h"
#include "vincehelper.h"
#include "objectAnimEventTagQueue.h"
#include "game.h"
#include "file\xcontentStream.h"
#include "stream\encryptedStream.h"
#include "campaignmanager.h"
#include "entityscheduler.h"
#include "skullmanager.h"
#include "protovisual.h"
#include "visual.h"
#include "grannymanager.h"
#include "grannymodel.h"
#include "grannyanimation.h"
#include "terraineffectmanager.h"
#include "terraineffect.h"
#include "impacteffectmanager.h"
#include "protoimpacteffect.h"
#include "lightEffectManager.h"
#include "lightEffect.h"
#include "particlegateway.h"
#include "timermanager.h"
#include "UIWidgets.h"
#include "UIObjectiveProgressControl.h"
#include "grannyinstance.h"
#include "platoonactionmove.h"
#include "damagetemplatemanager.h"
#include "damagetemplate.h"
#include "math\randomUtils.h"
#include "modegame.h"

// Current versions
GFIMPLEMENTVERSION(BSaveGame, 14);
GFIMPLEMENTVERSION(BSaveDB, 9);
GFIMPLEMENTVERSION(BSaveUser, 3);
GFIMPLEMENTVERSION(BSavePlayer, 1);
GFIMPLEMENTVERSION(BSaveTeam, 1);

enum
{
   cSaveMarkerStart=10000,
   cSaveMarkerEnd,
   cSaveMarkerVersions,
   cSaveMarkerDB,
   cSaveMarkerSetupPlayer,
   cSaveMarkerSetupTeam,
   cSaveMarkerUser1,
   cSaveMarkerUser2,
   cSaveMarkerWorld,
   cSaveMarkerUI,
};

//==============================================================================
// Remap callback functions for systems outside of xgame
//==============================================================================
void remapShapeIDCallback(long& id)
{
   gSaveGame.remapShapeID(id);
}

void remapPhysicsInfoIDCallback(long& id)
{
   gSaveGame.remapPhysicsInfoID(id);
}

//==============================================================================
// BSaveGame
//==============================================================================
BSaveGame gSaveGame;

//==============================================================================
//==============================================================================
BSaveGame::BSaveGame() :
   BGameFile(),
   mStartTime(0),
   mLastTime(0),
   mDoSaveDeviceID(XCONTENTDEVICE_ANY),
   mSaveFileType(0),
   mIsSaving(false),
   mIsLoading(false),
   mDoSaveGame(false),
   mDoLoadGame(false),
   mDoAutoQuit(false),
   mDoSaveCampaign(false),
   mSavedCampaign(false),
   mAutoQuitAfterSave(false),
   mIsCampaignLoad(false),
   mDoLoadCampaignGame(false),
   mSaveFileCorrupt(false),
   mLoadDeviceRemove(false)
{
   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   mTimerFrequency = freq.QuadPart;
   mTimerFrequencyFloat = (double)mTimerFrequency;
}

//==============================================================================
//==============================================================================
BSaveGame::~BSaveGame()
{
}

//==============================================================================
//==============================================================================
bool BSaveGame::setup()
{
   if (gPhysics)
   {
      gPhysics->setShapeIDRemapFunc(remapShapeIDCallback);
      gPhysics->setPhysicsInfoIDRemapFunc(remapPhysicsInfoIDCallback);
   }
   mGameFileType = cGameFileSave;
   mGameDirID = cDirSaveGame;
   mCachePrefix = "cache:\\savegame";
   mGameFileExt = ".sav";
   mDB.mNumUniqueProtoObjects = 0;
   mDB.mNumUniqueProtoSquads = 0;
   return BGameFile::setup();
}

//==============================================================================
//==============================================================================
void BSaveGame::reset()
{
   mIsSaving = false;
   mIsLoading = false;
   mDoSaveGame = false;
   mDoLoadGame = false;
   mDoLoadCampaignGame = false;

   mSaveFileName.empty();
   BSimString temp;

   mDB.mCivMap.clear();
   mDB.mLeaderMap.clear();
   mDB.mAbilityMap.clear();
   mDB.mProtoVisualMap.clear();
   mDB.mGrannyModelMap.clear();
   mDB.mGrannyAnimMap.clear();
   mDB.mTerrainEffectMap.clear();
   mDB.mImpactEffectMap.clear();
   mDB.mLightEffectNames.clear();
   mDB.mLightEffectMap.clear();
   mDB.mParticleEffectNames.clear();
   mDB.mParticleEffectMap.clear();
   mDB.mProtoObjectMap.clear();
   mDB.mProtoSquadMap.clear();
   mDB.mProtoTechMap.clear();
   mDB.mProtoPowerMap.clear();
   mDB.mObjectTypeMap.clear();
   mDB.mResourceMap.clear();
   mDB.mRateMap.clear();
   mDB.mPopMap.clear();
   mDB.mPlacementRuleMap.clear();
   mDB.mWeaponTypeMap.clear();
   mDB.mDamageTypeMap.clear();
   mDB.mDamageTemplateMap.clear();
   mDB.mDamageTemplateNames.clear();
   mDB.mDamageTemplateModelIDs.clear();
   mDB.mAnimTypeMap.clear();
   mDB.mAttachmentTypeMap.clear();
   mDB.mActionTypeMap.clear();
   mDB.mProtoActionMap.clear();
   mDB.mWeaponMap.clear();
   mDB.mProtoIconTypes.clear();
   mDB.mProtoIconNames.clear();
   mPlayers.clear();
   mTeams.clear();

   mDB.mNumUniqueProtoObjects = 0;
   mDB.mNumUniqueProtoSquads = 0;

   BGameFile::reset();
}

//==============================================================================
//==============================================================================
void BSaveGame::saveGame(const char* pFileName, bool autoQuit)
{
   //gGame.getUIGlobals()->showWaitDialog(BUString());

   mDoSaveGame = false;
   mDoLoadGame = false;
   mDoLoadCampaignGame = false;
   mAutoQuitAfterSave = autoQuit;
   mSaveFileName = pFileName;
   mDoSaveDeviceID = XCONTENTDEVICE_ANY;
   mDoSaveGameSuccess = false;
   mSaveRequest.mInProgress = false;

   mDoSaveCampaign = false;
   if (mSaveFileName == "campaign" || mSaveFileName == "campaign.sav")
      mDoSaveCampaign  = true;

   if (mDoSaveCampaign)
   {
      // FIXME the device ID could become invalid during any operation
      // but we currently don't have support to handle that level
      // of notifications in our callbacks
      XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();
      if (deviceID != XCONTENTDEVICE_ANY)
      {
         XDEVICE_DATA deviceData;
         ZeroMemory(&deviceData, sizeof(XDEVICE_DATA));
         if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
         {
            gUserManager.getPrimaryUser()->setDefaultDevice(XCONTENTDEVICE_ANY);
            deviceID = XCONTENTDEVICE_ANY;
         }
      }
      if (deviceID == XCONTENTDEVICE_ANY)
      {
         /*
         // wait for the notify callback
         mSaveRequest.mEventID = BGameFileRequest::mNextEventID++;
         mSaveRequest.mSaveGame = true;
         gUserManager.showDeviceSelector(gUserManager.getPrimaryUser(), this, mSaveRequest.mEventID);
         */
         mAutoQuitAfterSave = false;
         mDoSaveCampaign = false;
         return;
      }
      else
         setDoSaveGame(deviceID);
   }
   else
      setDoSaveGame(XCONTENTDEVICE_ANY);
}

//==============================================================================
//==============================================================================
void BSaveGame::loadGame(const char* pFileName, bool doLoadCampaignGame)
{
   mDoLoadGame = true;
   mDoLoadCampaignGame = doLoadCampaignGame;
   mDoSaveGame = false;
   mSaveFileName = pFileName;
   mIsCampaignLoad = false;
}

//==============================================================================
//==============================================================================
void BSaveGame::setDoSaveGame(XCONTENTDEVICEID deviceID)
{
   mDoSaveGame = true;
   mDoSaveDeviceID = deviceID;
}

//==============================================================================
//==============================================================================
void BSaveGame::clearDoSaveGame()
{
   mDoSaveGame = false;
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
      pUIGlobals->setWaitDialogVisible(false);
}

//==============================================================================
//==============================================================================
void BSaveGame::doSaveGameBegin()
{
   mDoSaveGame = false;

   if (!gWorld)
      return;

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (!pSettings)
      return;

   pSettings->setString(BGameSettings::cLoadName, mSaveFileName);

   if (save())
      mDoSaveGameSuccess = true;
   else
      reset();
}

//==============================================================================
//==============================================================================
void BSaveGame::doSaveGameEnd()
{
   if (mDoSaveGameSuccess)
   {
      mSavedCampaign = mDoSaveCampaign;
      if (mAutoQuitAfterSave)
         gUserManager.getPrimaryUser()->handleUIManagerResult(BUIManager::cResult_Exit);
   }
   else
   {
      // ajl 11/18/08 - Show a save error message only if we aren't already displaying another dialog. The other dialog is 
      // likely the one about the user removing the default storage device and we don't want to overwrite that.
      if (!gGame.getUIGlobals()->isYorNBoxVisible())
         gGame.getUIGlobals()->showYornBox(NULL, gDatabase.getLocStringFromID(25686), BUIGlobals::cDialogButtonsOK, 0);
   }
}

//==============================================================================
//==============================================================================
void BSaveGame::doLoadCampaignGame()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return;

   bool newCampaign = false;
   BGameSettings gameSettings;
   if (getCampaignSaveExists(&gameSettings))
   {
      BString mapName;
      gameSettings.getString(BGameSettings::cMapName, mapName);
      if (mapName == "NewCampaign")
         newCampaign = true;
   }
   
   // make sure the settings are filled in properly.
   pCampaign->setGameSettings(true, !newCampaign);

   if (gModeManager.inModeGame())
   {
      BModeGame* pModeGame = gModeManager.getModeGame();
      pModeGame->leave(pModeGame);
      pModeGame->enter(pModeGame);
   }
   else
   {
      // Change back to the game mode and start the game from there.
      gModeManager.setMode(BModeManager::cModeMenu);
      BModeMenu* pMode = gModeManager.getModeMenu();
      if(pMode)
         pMode->setNextState(BModeMenu::cStateGotoGame);
   }
}

//==============================================================================
//==============================================================================
void BSaveGame::doLoadGame()
{
   mDoLoadGame = false;
   mDoSaveGame = false;

   bool loadCampaign = mDoLoadCampaignGame;
   mDoLoadCampaignGame=false;
   if (loadCampaign)
   {
      // force a different way of loading the game if we are in the campaign.
      doLoadCampaignGame();
      return;
   }


   if (gLiveSystem->isMultiplayerGameActive())
      return;

   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (!pSettings)
      return;

   BSimString fileName = mSaveFileName;

   if (fileName.isEmpty())
   {
      // Look for the most recent save game file
      BFindFiles findFilesObj;
      BString dirName;
      if (gFileManager.getDirListEntry(dirName, cDirSaveGame, 0) != cFME_SUCCESS)
         return;
      BString pattern;
      pattern.format("*%s", mGameFileExt.getPtr());
      if (!findFilesObj.scan(dirName, pattern, BFindFiles::FIND_FILES_WANT_FILES))
         return;
      FILETIME newestTime;
      for (uint i = 0; i < findFilesObj.numFiles(); i++)
      {
         const BFileDesc& fileDesc = findFilesObj.getFile(i);
         FILETIME ft = Utils::UInt64ToFileTime(fileDesc.createTime());
         if (fileName.isEmpty() || CompareFileTime(&ft, &newestTime) > 0)
         {
            fileName = fileDesc.filename();
            newestTime = ft;
         }
      }
      if (fileName.isEmpty())
         return;
   }

   BSimString gameID;
   MVince_CreateGameID(gameID);

   BString currentMapName;
   pSettings->getString(BGameSettings::cMapName, currentMapName);

   pSettings->setLong(BGameSettings::cPlayerCount, 1);
   pSettings->setString(BGameSettings::cMapName, "");
   pSettings->setLong(BGameSettings::cMapIndex, -1);
   pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
   pSettings->setString(BGameSettings::cGameID, gameID);
   pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);
   pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
   pSettings->setLong(BGameSettings::cGameMode, 0);
   pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
   pSettings->setString(BGameSettings::cLoadName, fileName);

   bool record=gConfig.isDefined(cConfigRecordGames);
   pSettings->setBool(BGameSettings::cRecordGame, record);

   /* ajl 8/28/08 - this doesn't seem stable as the game has gone into a deadlock state and I've also
      seen crashes where the visual system has a pointer to a particle instance which has been freed.
      So I'm taking this out since it's not required.

   // ajl 7/31/08 - this currently doesn't work in archive mode because the game will run out of memory since
   // the full archive has to be loaded. we probably need to separate out the scenario data files and lrp tree
   // data into their own smaller archive file.
   if (gModeManager.inModeGame() && gWorld && !gArchiveManager.getArchivesEnabled())
   {
      if (gRecordGame.isRecording() || gRecordGame.isPlaying())
         gRecordGame.stop();
      if (record)
         gRecordGame.record();
      // If save game is for currently loaded scenario, do a quick load instead of a full load.
      BSimString savedMapName;
      if (getMapFromFile(fileName, savedMapName))
      {
         if (currentMapName == savedMapName)
         {
            if (load())
            {
               if (!gScenario.load(true, false, true))
                  gModeManager.setMode(BModeManager::cModeMenu);
               return;
            }
            else
            {
               gModeManager.setMode(BModeManager::cModeMenu);
               return;
            }
         }
      }
   }
   */

   gModeManager.setMode(BModeManager::cModeMenu);
   BModeMenu* pMode = gModeManager.getModeMenu();
   if(pMode)
      pMode->setNextState(BModeMenu::cStateGotoGame);
}

//==============================================================================
//==============================================================================
bool BSaveGame::save()
{
   mIsSaving = true;
   mSaveFileType = cSaveFileTypeFull;

   LARGE_INTEGER startTime;
   QueryPerformanceCounter(&startTime);
   mStartTime = startTime.QuadPart;
   mLastTime = mStartTime;

   if (!saveBase())
      return false;

   // [11-7-08 CJS] Make sure the save flag is still set--fixes savegame OOS
   mIsSaving = true;

   GFWRITEVERSION(mpStream, BSaveGame);
   GFWRITEVAR(mpStream, int, mSaveFileType);
   GFWRITEMARKER(mpStream, cSaveMarkerStart);

   if (!saveVersions())
      return false;

   if (!saveDB())
      return false;

   if (!saveSetup())
      return false;

   if (!saveData())
      return false;

   GFWRITEMARKER(mpStream, cSaveMarkerEnd);
   logStats("End");

   BSimString fileName = mGameFileName;
   gUserManager.getPrimaryUser()->setSaveGame(fileName);

   bool doClose = true;
   bool retval = true;

   if (mDoSaveCampaign)
   {
      padSaveFile();

      if (saveFile(gUserManager.getPrimaryUser()->getPort()))
         doClose = false;
      else
      {
         mSaveRequest.mInProgress = false;
         retval = false;
      }
   }

   if (doClose)
      close();

   mIsSaving = false;

   return retval;
}

//==============================================================================
//==============================================================================
bool BSaveGame::saveVersions()
{
   GFWRITEVERSION(mpStream, BAction);
   GFWRITEVERSION(mpStream, BActionManager);
   GFWRITEVERSION(mpStream, BAI);
   GFWRITEVERSION(mpStream, BAIGlobals);
   GFWRITEVERSION(mpStream, BAIDifficultySetting);
   GFWRITEVERSION(mpStream, BAIManager);
   GFWRITEVERSION(mpStream, BAIMission);
   GFWRITEVERSION(mpStream, BAIMissionScore);
   GFWRITEVERSION(mpStream, BAIMissionTarget);
   GFWRITEVERSION(mpStream, BAIMissionTargetWrapper);
   GFWRITEVERSION(mpStream, BAIPowerMission);
   GFWRITEVERSION(mpStream, BAIScoreModifier);
   GFWRITEVERSION(mpStream, BAISquadAnalysis);
   GFWRITEVERSION(mpStream, BAITopic);
   GFWRITEVERSION(mpStream, BArmy);
   GFWRITEVERSION(mpStream, BBid);
   GFWRITEVERSION(mpStream, BBidManager);
   GFWRITEVERSION(mpStream, BChatManager);
   GFWRITEVERSION(mpStream, BConvexHull);
   GFWRITEVERSION(mpStream, BCost);
   GFWRITEVERSION(mpStream, BCustomCommand);
   GFWRITEVERSION(mpStream, BDamageTracker);
   GFWRITEVERSION(mpStream, BDopple);
   GFWRITEVERSION(mpStream, BEntity);
   GFWRITEVERSION(mpStream, BEntityFilter);
   GFWRITEVERSION(mpStream, BEntityFilterSet);
   GFWRITEVERSION(mpStream, BFatalityManager);
   GFWRITEVERSION(mpStream, BFormation2);
   GFWRITEVERSION(mpStream, BGeneralEventManager);
   GFWRITEVERSION(mpStream, BHintEngine);
   GFWRITEVERSION(mpStream, BHintManager);
   GFWRITEVERSION(mpStream, BKB);
   GFWRITEVERSION(mpStream, BKBBase);
   GFWRITEVERSION(mpStream, BKBBaseQuery);
   GFWRITEVERSION(mpStream, BKBSquad);
   GFWRITEVERSION(mpStream, BKBSquadFilter);
   GFWRITEVERSION(mpStream, BKBSquadFilterSet);
   GFWRITEVERSION(mpStream, BKBSquadQuery);
   GFWRITEVERSION(mpStream, BObject);
   GFWRITEVERSION(mpStream, BObjectAnimationState);
   GFWRITEVERSION(mpStream, BObjectiveManager);
   GFWRITEVERSION(mpStream, BPath);
   GFWRITEVERSION(mpStream, BPathingLimiter);
   GFWRITEVERSION(mpStream, BPiecewiseDataPoint);
   GFWRITEVERSION(mpStream, BPiecewiseFunc);
   GFWRITEVERSION(mpStream, BPlatoon);
   GFWRITEVERSION(mpStream, BPlayer);
   GFWRITEVERSION(mpStream, BPower);
   GFWRITEVERSION(mpStream, BPowerEntry);
   GFWRITEVERSION(mpStream, BPowerEntryItem);
   GFWRITEVERSION(mpStream, BPowerManager);
   GFWRITEVERSION(mpStream, BPowerUser);
   GFWRITEVERSION(mpStream, BProjectile);
   GFWRITEVERSION(mpStream, BProtoAction);
   GFWRITEVERSION(mpStream, BProtoObject);
   GFWRITEVERSION(mpStream, BProtoSquad);
   GFWRITEVERSION(mpStream, BProtoTech);
   GFWRITEVERSION(mpStream, BSaveDB);
   GFWRITEVERSION(mpStream, BSavePlayer);
   GFWRITEVERSION(mpStream, BSaveTeam);
   GFWRITEVERSION(mpStream, BSaveUser);
   GFWRITEVERSION(mpStream, BScoreManager);
   GFWRITEVERSION(mpStream, BSelectionAbility);
   GFWRITEVERSION(mpStream, BSimOrder);
   GFWRITEVERSION(mpStream, BSimOrderEntry);
   GFWRITEVERSION(mpStream, BSimTarget);
   GFWRITEVERSION(mpStream, BSquad);
   GFWRITEVERSION(mpStream, BSquadAI);
   GFWRITEVERSION(mpStream, BSquadPlotterResult);
   GFWRITEVERSION(mpStream, BStatsManager);
   GFWRITEVERSION(mpStream, BTactic);
   GFWRITEVERSION(mpStream, BTeam);
   GFWRITEVERSION(mpStream, BTrigger);
   GFWRITEVERSION(mpStream, BTriggerCondition);
   GFWRITEVERSION(mpStream, BTriggerEffect);
   GFWRITEVERSION(mpStream, BTriggerGroup);
   GFWRITEVERSION(mpStream, BTriggerManager);
   GFWRITEVERSION(mpStream, BTriggerScript);
   GFWRITEVERSION(mpStream, BTriggerScriptExternals);
   GFWRITEVERSION(mpStream, BTriggerVar);
   GFWRITEVERSION(mpStream, BUICallouts);
   GFWRITEVERSION(mpStream, BUIManager);
   GFWRITEVERSION(mpStream, BUnit);
   GFWRITEVERSION(mpStream, BUser);
   GFWRITEVERSION(mpStream, BVisibleMap);
   GFWRITEVERSION(mpStream, BWeapon);
   GFWRITEVERSION(mpStream, BWorld);
   GFWRITEVERSION(mpStream, BStoredAnimEventManager);

   GFWRITEVERSION(mpStream, BAIGroup);
   GFWRITEVERSION(mpStream, BAIGroupTask);
   GFWRITEVERSION(mpStream, BAIMissionCache);
   GFWRITEVERSION(mpStream, BAITeleporterZone);
   GFWRITEVERSION(mpStream, BAIPlayerModifier);
   GFWRITEVERSION(mpStream, BEntityScheduler);
   GFWRITEVERSION(mpStream, BCollectiblesManager);
   
   GFWRITEVERSION(mpStream, BVisual);
   GFWRITEVERSION(mpStream, BVisualItem);
   GFWRITEVERSION(mpStream, BVisualAnimationData);
   GFWRITEVERSION(mpStream, BGrannyInstance);

   GFWRITEVERSION(mpStream, BTimerManager);
   GFWRITEVERSION(mpStream, BUIWidgets);
   GFWRITEVERSION(mpStream, BUIObjectiveProgressControl);
   GFWRITEVERSION(mpStream, BUITalkingHeadControl);

   GFWRITEVERSION(mpStream, BSquadActionEntry);

   GFWRITEMARKER(mpStream, cSaveMarkerVersions);
   logStats("Versions");
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::saveDB()
{
   int count = gDatabase.getNumberCivs();
   GFVERIFYCOUNT(count, 100);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getCiv(i)->getCivName(), 100);

   count = gDatabase.getNumberLeaders();
   GFVERIFYCOUNT(count, 300);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getLeader(i)->mName, 100);

   count = gDatabase.getNumberAbilities();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getAbilityFromID(i)->getName(), 100);

   count = gVisualManager.getNumProtoVisuals();
   GFVERIFYCOUNT(count, 10000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gVisualManager.getProtoVisual(i, false)->getName(), 100);

   count = gGrannyManager.getNumModels();
   GFVERIFYCOUNT(count, 10000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gGrannyManager.getModel(i, false)->getFilename(), 100);

   count = gGrannyManager.getNumAnimations();
   GFVERIFYCOUNT(count, 10000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gGrannyManager.getAnimation(i, false)->getFilename(), 100);

   count = gTerrainEffectManager.getNumberTerrainEffects();
   GFVERIFYCOUNT(count, 500);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gTerrainEffectManager.getTerrainEffect(i, false)->getName(), 100);

   count = gDatabase.getNumberImpactEffects();
   GFVERIFYCOUNT(count, 500);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getProtoImpactEffectFromIndex(i)->mName, 100)

   count = gLightEffectManager.getNumLightEffects();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gLightEffectManager.getLightEffect(i)->getName(), 100);

   count = gParticleGateway.getNumDataSlotsInUse();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, *gParticleGateway.getDataName(i), 100);

   logStats("DBMisc1");

   count = gDatabase.getNumberProtoObjects();
   GFVERIFYCOUNT(count, 20000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      GFWRITESTRING(mpStream, BSimString, gDatabase.getGenericProtoObject(i)->getName(), 100);
      GFWRITEVAL(mpStream, long, gDatabase.getGenericProtoObject(i)->getDBID());
   }

   logStats("DBProtoObjects");

   count = gDatabase.getNumberProtoSquads();
   GFVERIFYCOUNT(count, 20000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(i);
      GFWRITESTRING(mpStream, BSimString, pProtoSquad->getName(), 100);
      GFWRITEVAL(mpStream, bool, pProtoSquad->getFlagObjectProtoSquad());
   }

   logStats("DBProtoSquads");

   count = gDatabase.getNumberProtoTechs();
   GFVERIFYCOUNT(count, 10000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getProtoTech(i)->getName(), 100);

   logStats("DBProtoTechs");

   count = gDatabase.getNumberProtoPowers();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getProtoPowerByID(i)->getName(), 100);

   count = gDatabase.getNumberObjectTypes();
   GFVERIFYCOUNT(count, 20000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gDatabase.getObjectTypeName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gDatabase.getNumberResources();
   GFVERIFYCOUNT(count, 200);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gDatabase.getResourceName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gDatabase.getNumberRates();
   GFVERIFYCOUNT(count, 200);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gDatabase.getRateName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gDatabase.getNumberPops();
   GFVERIFYCOUNT(count, 200);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gDatabase.getPopName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gDatabase.getNumberWeaponTypes();
   GFVERIFYCOUNT(count, 10000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
      GFWRITESTRING(mpStream, BSimString, gDatabase.getWeaponTypeByID(i)->getName(), 100);

   count = gDatabase.getNumberDamageTypes();
   GFVERIFYCOUNT(count, 200);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gDatabase.getDamageTypeName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gDamageTemplateManager.getNumberDamageTemplates();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      const BDamageTemplate* pTemplate = gDamageTemplateManager.getDamageTemplate(i);
      BSimString name = pTemplate->getFileName();
      GFWRITESTRING(mpStream, BSimString, name, 100);
      GFWRITEVAL(mpStream, long, pTemplate->getModelIndex());
   }

   count = gVisualManager.getNumAnimTypes();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gVisualManager.getAnimName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gVisualManager.getNumAttachmentTypes();
   GFVERIFYCOUNT(count, 2000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gVisualManager.getAttachmentName(i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   count = gActionManager.getNumberActionTypes();
   GFVERIFYCOUNT(count, 250);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSimString name = gActionManager.getActionName((BActionType)i);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   logStats("DBMisc2");

   count = gDatabase.getNumberProtoObjects();
   for (int i=0; i<count; i++)
   {
      BTactic* pTactic = gDatabase.getGenericProtoObject(i)->getTactic();
      if (pTactic)
      {
         GFWRITEVAL(mpStream, int16, i);
         uint8 protoActionCount = (uint8)pTactic->getNumberProtoActions();
         GFWRITEVAR(mpStream, uint8, protoActionCount);
         GFVERIFYCOUNT(protoActionCount, 200);
         for (uint8 j=0; j<protoActionCount; j++)
            GFWRITESTRING(mpStream, BSimString, pTactic->getProtoAction(j)->getName(), 100);
         uint8 weaponCount = (uint8)pTactic->getNumberWeapons();
         GFWRITEVAR(mpStream, uint8, weaponCount);
         GFVERIFYCOUNT(weaponCount, 200);
         for (uint8 j=0; j<weaponCount; j++)
            GFWRITESTRING(mpStream, BSimString, pTactic->getWeapon(j)->mpStaticData->mName, 100);
      }
   }
   GFWRITEVAL(mpStream, int16, -1);

   logStats("DBProtoActionsAndWeapons");

   count = gWorld->getNumberPlayers();
   int numUniqueProtoObjects = 0;
   int numUniqueProtoSquads = 0;
   for (int i=0; i<count; i++)
   {
      numUniqueProtoObjects = max(numUniqueProtoObjects, gWorld->getPlayer(i)->getNumberUniqueProtoObjects());
      numUniqueProtoSquads = max(numUniqueProtoSquads, gWorld->getPlayer(i)->getNumberUniqueProtoSquads());
   }
   GFWRITEVAR(mpStream, int, numUniqueProtoObjects);
   GFVERIFYCOUNT(numUniqueProtoObjects, 100);
   GFWRITEVAR(mpStream, int, numUniqueProtoObjects);
   GFVERIFYCOUNT(numUniqueProtoObjects, 100);

   const BUStringTable<long>& shapeNameTable = gPhysics->getShapeManager().getNameTable();
   const BDynamicSimArray<BString> shapeNameTags = shapeNameTable.getTags();
   count = gPhysics->getShapeManager().getNumberShapes();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   count = shapeNameTags.getNumber();
   for (int i=0; i<count; i++)
   {
      long shapeID = -1;
      if (shapeNameTable.find(shapeNameTags[i], &shapeID))
      {
         GFWRITEVAR(mpStream, long, shapeID);
         GFWRITESTRING(mpStream, BString, shapeNameTags[i], 100);
      }
   }
   GFWRITEVAL(mpStream, long, -1);

   const BUStringTable<long>& physicsInfoNameTable = gPhysicsInfoManager.getNameTable();
   const BDynamicSimArray<BString> physicsInfoNameTags = physicsInfoNameTable.getTags();
   count = gPhysicsInfoManager.getNumberPhysicsInfos();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   count = physicsInfoNameTags.getNumber();
   for (int i=0; i<count; i++)
   {
      long physicsInfoID = -1;
      if (physicsInfoNameTable.find(physicsInfoNameTags[i], &physicsInfoID))
      {
         GFWRITEVAR(mpStream, long, physicsInfoID);
         GFWRITESTRING(mpStream, BString, physicsInfoNameTags[i], 100);
      }
   }
   GFWRITEVAL(mpStream, long, -1);

   BUIContext* pUIContext = gUserManager.getPrimaryUser()->getUIContext();
   count = pUIContext->getProtoIconCount();
   GFVERIFYCOUNT(count, 1000);
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      const BFlashProtoIcon* pIcon = pUIContext->getProtoIcon(i);
      BSimString type = pIcon->mType;
      BSimString name = pIcon->mName;
      GFWRITESTRING(mpStream, BSimString, type, 100);
      GFWRITESTRING(mpStream, BSimString, name, 100);
   }

   GFWRITEMARKER(mpStream, cSaveMarkerDB);
   logStats("DB");
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::saveSetup()
{
   // Capture player setup data
   mPlayers.clear();
   int count = gWorld->getNumberPlayers();
   if (count == 0)
      return true;
   if (!mPlayers.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      BSavePlayer& player = mPlayers[i];
//-- FIXING PREFIX BUG ID 2301
      const BPlayer* pPlayer = gWorld->getPlayer(i);
//--
      player.mName = pPlayer->getName();
      player.mDisplayName = pPlayer->getLocalisedDisplayName();
      player.mMPID = pPlayer->getMPID();
      player.mCoopID = pPlayer->getCoopID();
      player.mScenarioID = pPlayer->getScenarioID();
      player.mCivID = pPlayer->getCivID();
      player.mTeamID = pPlayer->getTeamID();
      player.mLeaderID = pPlayer->getLeaderID();
      player.mDifficulty = pPlayer->getDifficulty();
      player.mPlayerType = pPlayer->getPlayerType();
   }

   // Write player setup data
   GFWRITEVAR(mpStream, int, count);
   for (int i=0; i<count; i++)
   {
      BSavePlayer& player = mPlayers[i];
      GFWRITESTRING(mpStream, BSimString, player.mName, 100);
      GFWRITESTRING(mpStream, BUString, player.mDisplayName, 100);
      GFWRITEVAR(mpStream, long, player.mMPID);
      GFWRITEVAR(mpStream, BPlayerID, player.mScenarioID);
      GFWRITEVAR(mpStream, long, player.mCivID);
      GFWRITEVAR(mpStream, BTeamID, player.mTeamID);
      GFWRITEVAR(mpStream, long, player.mLeaderID);
      GFWRITEVAR(mpStream, BHalfFloat, player.mDifficulty);
      GFWRITEVAR(mpStream, int8, player.mPlayerType);
      GFWRITEMARKER(mpStream, cSaveMarkerSetupPlayer);
   }

   // Capture team setup data
   mTeams.clear();
   count = gWorld->getNumberTeams();
   if (count == 0)
      return true;
   if (!mTeams.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      BSaveTeam& team = mTeams[i];
//-- FIXING PREFIX BUG ID 2303
      const BTeam* pTeam = gWorld->getTeam(i);
//--
      int playerCount = pTeam->getNumberPlayers();
      if (!team.mPlayers.setNumber(playerCount))
         return false;
      for (int j=0; j<playerCount; j++)
         team.mPlayers[j] = pTeam->getPlayerID(j);
      if (!team.mRelations.setNumber(count))
         return false;
      for (int j=0; j<count; j++)
         team.mRelations[j] = gWorld->getTeamRelationType(i, j);
   }

   // Write team setup data
   count = mTeams.getNumber();
   GFWRITEVAR(mpStream, int, count);

   for (int i=0; i<count; i++)
   {
      BSaveTeam& team = mTeams[i];
      GFWRITEARRAY(mpStream, long, team.mPlayers, uint8, 20);
      GFWRITEARRAY(mpStream, BRelationType, team.mRelations, uint8, 20);
   }

   GFWRITEMARKER(mpStream, cSaveMarkerSetupTeam);

   // Capture user setup data
   BUser* pUser = gUserManager.getPrimaryUser();

   mUser.mCurrentPlayer = pUser->getPlayerID();
   mUser.mCoopPlayer = pUser->getCoopPlayerID();

//-- FIXING PREFIX BUG ID 2304
   const BCamera* pCamera = pUser->getCamera();
//--
   mUser.mCameraPosition = pCamera->getCameraLoc();
   mUser.mCameraForward = pCamera->getCameraDir();
   mUser.mCameraRight = pCamera->getCameraRight();
   mUser.mCameraUp = pCamera->getCameraUp();
   mUser.mCameraDefaultPitch = pUser->getDefaultPitch();
   mUser.mCameraDefaultYaw = pUser->getDefaultYaw();
   mUser.mCameraDefaultZoom = pUser->getDefaultZoom();
   mUser.mDefaultCamera = pUser->getDefaultCamera();
   mUser.mHaveHoverPoint = pUser->getFlagHaveHoverPoint();
   mUser.mHoverPoint = pUser->getHoverPoint();
   mUser.mCameraHoverPoint = pUser->getCameraHoverPoint();
   mUser.mCameraPitch = pUser->getCameraPitch();
   mUser.mCameraYaw = pUser->getCameraYaw();
   mUser.mCameraZoom = pUser->getCameraZoom();
   mUser.mCameraFOV = pUser->getCameraFOV();
   mUser.mCameraHoverPointOffsetHeight = pUser->getCameraHoverPointOffsetHeight();

   // Write user setup data
   GFWRITEVAR(mpStream, int, mUser.mCurrentPlayer);
   GFWRITEVAR(mpStream, int, mUser.mCoopPlayer);

   GFWRITEVECTOR(mpStream, mUser.mHoverPoint);
   GFWRITEVECTOR(mpStream, mUser.mCameraHoverPoint);
   GFWRITEVECTOR(mpStream, mUser.mCameraPosition);
   GFWRITEVECTOR(mpStream, mUser.mCameraForward);
   GFWRITEVECTOR(mpStream, mUser.mCameraRight);
   GFWRITEVECTOR(mpStream, mUser.mCameraUp);
   GFWRITEVAR(mpStream, float, mUser.mCameraDefaultPitch);
   GFWRITEVAR(mpStream, float, mUser.mCameraDefaultYaw);
   GFWRITEVAR(mpStream, float, mUser.mCameraDefaultZoom);
   GFWRITEVAR(mpStream, float, mUser.mCameraPitch);
   GFWRITEVAR(mpStream, float, mUser.mCameraYaw);
   GFWRITEVAR(mpStream, float, mUser.mCameraZoom);
   GFWRITEVAR(mpStream, float, mUser.mCameraFOV);
   GFWRITEVAR(mpStream, float, mUser.mCameraHoverPointOffsetHeight);
   GFWRITEBITBOOL(mpStream, mUser.mHaveHoverPoint);
   GFWRITEBITBOOL(mpStream, mUser.mDefaultCamera);

   GFWRITEMARKER(mpStream, cSaveMarkerUser1);

   logStats("Setup");
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::saveData()
{
   if (!gWorld->save(mpStream, mSaveFileType))
      return false;
   GFWRITEMARKER(mpStream, cSaveMarkerWorld);
   if (!gUIManager->save(mpStream, mSaveFileType))
      return false;
   GFWRITEMARKER(mpStream, cSaveMarkerUI);
   BUser* pUser = gUserManager.getPrimaryUser();
   GFWRITECLASSPTR(mpStream, mSaveFileType, pUser);
   GFWRITEMARKER(mpStream, cSaveMarkerUser2);
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::load()
{
   if (!loadBase(mIsCampaignLoad, &mCampaignContentData))
      return false;

   mIsLoading = true;

   GFREADVERSION(mpStream, BSaveGame);
   GFREADVAR(mpStream, int, mSaveFileType);
   GFREADMARKER(mpStream,cSaveMarkerStart);

   if (!loadVersions())
      return false;

   if (!loadDB())
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::loadVersions()
{
   GFREADVERSION(mpStream, BAction);
   GFREADVERSION(mpStream, BActionManager);
   GFREADVERSION(mpStream, BAI);
   GFREADVERSION(mpStream, BAIGlobals);
   GFREADVERSION(mpStream, BAIDifficultySetting);
   GFREADVERSION(mpStream, BAIManager);
   GFREADVERSION(mpStream, BAIMission);
   GFREADVERSION(mpStream, BAIMissionScore);
   GFREADVERSION(mpStream, BAIMissionTarget);
   GFREADVERSION(mpStream, BAIMissionTargetWrapper);
   GFREADVERSION(mpStream, BAIPowerMission);
   GFREADVERSION(mpStream, BAIScoreModifier);
   GFREADVERSION(mpStream, BAISquadAnalysis);
   GFREADVERSION(mpStream, BAITopic);
   GFREADVERSION(mpStream, BArmy);
   GFREADVERSION(mpStream, BBid);
   GFREADVERSION(mpStream, BBidManager);
   GFREADVERSION(mpStream, BChatManager);
   GFREADVERSION(mpStream, BConvexHull);
   GFREADVERSION(mpStream, BCost);
   GFREADVERSION(mpStream, BCustomCommand);
   GFREADVERSION(mpStream, BDamageTracker);
   GFREADVERSION(mpStream, BDopple);
   GFREADVERSION(mpStream, BEntity);
   GFREADVERSION(mpStream, BEntityFilter);
   GFREADVERSION(mpStream, BEntityFilterSet);
   GFREADVERSION(mpStream, BFatalityManager);
   GFREADVERSION(mpStream, BFormation2);
   GFREADVERSION(mpStream, BGeneralEventManager);
   GFREADVERSION(mpStream, BHintEngine);
   GFREADVERSION(mpStream, BHintManager);
   GFREADVERSION(mpStream, BKB);
   GFREADVERSION(mpStream, BKBBase);
   GFREADVERSION(mpStream, BKBBaseQuery);
   GFREADVERSION(mpStream, BKBSquad);
   GFREADVERSION(mpStream, BKBSquadFilter);
   GFREADVERSION(mpStream, BKBSquadFilterSet);
   GFREADVERSION(mpStream, BKBSquadQuery);
   GFREADVERSION(mpStream, BObject);
   GFREADVERSION(mpStream, BObjectAnimationState);
   GFREADVERSION(mpStream, BObjectiveManager);
   GFREADVERSION(mpStream, BPath);
   GFREADVERSION(mpStream, BPathingLimiter);
   GFREADVERSION(mpStream, BPiecewiseDataPoint);
   GFREADVERSION(mpStream, BPiecewiseFunc);
   GFREADVERSION(mpStream, BPlatoon);
   GFREADVERSION(mpStream, BPlayer);
   GFREADVERSION(mpStream, BPower);
   GFREADVERSION(mpStream, BPowerEntry);
   GFREADVERSION(mpStream, BPowerEntryItem);
   GFREADVERSION(mpStream, BPowerManager);
   GFREADVERSION(mpStream, BPowerUser);
   GFREADVERSION(mpStream, BProjectile);
   GFREADVERSION(mpStream, BProtoAction);
   GFREADVERSION(mpStream, BProtoObject);
   GFREADVERSION(mpStream, BProtoSquad);
   GFREADVERSION(mpStream, BProtoTech);
   GFREADVERSION(mpStream, BSaveDB);
   GFREADVERSION(mpStream, BSavePlayer);
   GFREADVERSION(mpStream, BSaveTeam);
   GFREADVERSION(mpStream, BSaveUser);
   GFREADVERSION(mpStream, BScoreManager);
   GFREADVERSION(mpStream, BSelectionAbility);
   GFREADVERSION(mpStream, BSimOrder);
   GFREADVERSION(mpStream, BSimOrderEntry);
   GFREADVERSION(mpStream, BSimTarget);
   GFREADVERSION(mpStream, BSquad);
   GFREADVERSION(mpStream, BSquadAI);
   GFREADVERSION(mpStream, BSquadPlotterResult);
   GFREADVERSION(mpStream, BStatsManager);
   GFREADVERSION(mpStream, BTactic);
   GFREADVERSION(mpStream, BTeam);
   GFREADVERSION(mpStream, BTrigger);
   GFREADVERSION(mpStream, BTriggerCondition);
   GFREADVERSION(mpStream, BTriggerEffect);
   GFREADVERSION(mpStream, BTriggerGroup);
   GFREADVERSION(mpStream, BTriggerManager);
   GFREADVERSION(mpStream, BTriggerScript);
   GFREADVERSION(mpStream, BTriggerScriptExternals);
   GFREADVERSION(mpStream, BTriggerVar);
   GFREADVERSION(mpStream, BUICallouts);
   GFREADVERSION(mpStream, BUIManager);
   GFREADVERSION(mpStream, BUnit);
   GFREADVERSION(mpStream, BUser);
   GFREADVERSION(mpStream, BVisibleMap);
   GFREADVERSION(mpStream, BWeapon);
   GFREADVERSION(mpStream, BWorld);
   GFREADVERSION(mpStream, BStoredAnimEventManager);

   if (mGameFileVersion >= 2)
   {
      GFREADVERSION(mpStream, BAIGroup);
      GFREADVERSION(mpStream, BAIGroupTask);
      GFREADVERSION(mpStream, BAIMissionCache);
      GFREADVERSION(mpStream, BAITeleporterZone);
   }
   else
   {
      BAIGroup::mGameFileVersion = 1;
      BAIGroupTask::mGameFileVersion = 1;
      BAIMissionCache::mGameFileVersion = 1;
      BAITeleporterZone::mGameFileVersion = 1;
   }

   if (mGameFileVersion >= 6)
      GFREADVERSION(mpStream, BAIPlayerModifier)
   else
      BAIPlayerModifier::mGameFileVersion = 1;

   if (mGameFileVersion >= 7)
      GFREADVERSION(mpStream, BEntityScheduler)
   else
      BEntityScheduler::mGameFileVersion = 1;

   if (mGameFileVersion >= 8)
      GFREADVERSION(mpStream, BCollectiblesManager)
   else
      BCollectiblesManager::mGameFileVersion = 1;

   if (mGameFileVersion >= 9)
   {
      GFREADVERSION(mpStream, BVisual);
      GFREADVERSION(mpStream, BVisualItem);
      GFREADVERSION(mpStream, BVisualAnimationData);
   }
   else
   {
      BVisual::mGameFileVersion = 1;
      BVisualItem::mGameFileVersion = 1;
      BVisualAnimationData::mGameFileVersion = 1;
   }

   if (mGameFileVersion >= 12)
      GFREADVERSION(mpStream, BGrannyInstance)
   else
      BGrannyInstance::mGameFileVersion = 1;

   if (mGameFileVersion >= 10)
   {
      GFREADVERSION(mpStream, BTimerManager);
      GFREADVERSION(mpStream, BUIWidgets);
   }
   else
   {
      BTimerManager::mGameFileVersion = 1;
      BUIWidgets::mGameFileVersion = 1;
   }

   if (mGameFileVersion >= 11)
   {
      GFREADVERSION(mpStream, BUIObjectiveProgressControl);
      GFREADVERSION(mpStream, BUITalkingHeadControl);
   }
   else
   {
      BUIObjectiveProgressControl::mGameFileVersion = 1;
      BUITalkingHeadControl::mGameFileVersion = 1;
   }

   if (mGameFileVersion >= 13)
   {
      GFREADVERSION(mpStream, BSquadActionEntry);
   }
	else
		BSquadActionEntry::mGameFileVersion = 0;
   

   GFREADMARKER(mpStream, cSaveMarkerVersions);
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::loadDB()
{
   int count;
   BSimString name;

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 100);
   if (!mDB.mCivMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mCivMap[i] = gDatabase.getCivID(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 300);
   if (!mDB.mLeaderMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mLeaderMap[i] = gDatabase.getLeaderID(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 1000);
   if (!mDB.mAbilityMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mAbilityMap[i] = gDatabase.getAbilityIDFromName(name);
   }

   if (BSaveDB::mGameFileVersion >= 5)
   {
      // Proto Visuals
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 10000);
      if (!mDB.mProtoVisualMap.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mProtoVisualMap[i] = gVisualManager.getOrCreateProtoVisual(name, false);
      }

      // Granny Models
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 10000);
      if (!mDB.mGrannyModelMap.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mGrannyModelMap[i] = gGrannyManager.getOrCreateModel(name, false);
      }

      // Granny Anims
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 10000);
      if (!mDB.mGrannyAnimMap.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mGrannyAnimMap[i] = gGrannyManager.getOrCreateAnimation(name, false);
      }

      if (BSaveDB::mGameFileVersion >= 7)
      {
         // Terrain Effects
         GFREADVAR(mpStream, int, count);
         GFVERIFYCOUNT(count, 500);
         if (!mDB.mTerrainEffectMap.setNumber(count))
            return false;
         for (int i=0; i<count; i++)
         {
            GFREADSTRING(mpStream, BSimString, name, 100);
            mDB.mTerrainEffectMap[i] = gTerrainEffectManager.getOrCreateTerrainEffect(name, false);
         }
      }

      if (BSaveDB::mGameFileVersion >= 9)
      {
         // Impact Effects
         GFREADVAR(mpStream, int, count);
         GFVERIFYCOUNT(count, 500);
         if (!mDB.mImpactEffectMap.setNumber(count))
            return false;
         for (int i=0; i<count; i++)
         {
            GFREADSTRING(mpStream, BSimString, name, 100);
            mDB.mImpactEffectMap[i] = gDatabase.getProtoImpactEffectIndex(name);
         }
      }

      // Light Effects
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 1000);
      //if (!mDB.mLightEffectMap.setNumber(count))
      //   return false;
      if (!mDB.mLightEffectNames.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mLightEffectNames[i] = name;
         //long dataHandle = -1;
         //gLightEffectManager.getOrCreateData(name, dataHandle);
         //mDB.mLightEffectMap[i] = dataHandle;
      }

      // Particle Effects
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 1000);
      //if (!mDB.mParticleEffectMap.setNumber(count))
      //   return false;
      if (!mDB.mParticleEffectNames.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mParticleEffectNames[i] = name;
         //BParticleEffectDataHandle dataHandle = -1;
         //gParticleGateway.getOrCreateData(name, dataHandle);
         //mDB.mParticleEffectMap[i] = dataHandle;
      }
   }

   // Proto objects by name or DBID
   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 20000);
   if (!mDB.mProtoObjectMap.setNumber(count))
      return false;
   BHashMap<long, long> protoObjectFromDBID;
   for (int i=0; i<gDatabase.getNumberProtoObjects(); i++)
   {
      const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(i);
      long dbid = pProtoObject->getDBID();
      if (dbid != -1)
         protoObjectFromDBID.insert(dbid, i);
   }
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      long dbid = -1;
      if (BSaveDB::mGameFileVersion >= 5)
         GFREADVAR(mpStream, long, dbid)
      mDB.mProtoObjectMap[i] = gDatabase.getProtoObject(name);
      if (mDB.mProtoObjectMap[i] == -1 && dbid != -1)
      {
         BHashMap<long, long>::iterator it(protoObjectFromDBID.find(dbid));
         if (it != protoObjectFromDBID.end())
            mDB.mProtoObjectMap[i] = (long)it->second;
      }
   }
   protoObjectFromDBID.clear();

   // Proto squads
   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 20000);
   if (!mDB.mProtoSquadMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      bool object = false;
      if (BSaveDB::mGameFileVersion >= 3)
         GFREADVAR(mpStream, bool, object);
      if (BSaveDB::mGameFileVersion < 2 && i > 130)
         mDB.mProtoSquadMap[i] = i;
      else
      {
         BProtoSquadID id = gDatabase.getProtoSquad(name);
         if (id == cInvalidProtoSquadID)
         {
            mDB.mProtoSquadMap[i] = i;
            if (BSaveDB::mGameFileVersion == 2)
            {
               BSimString prefix = name;
               prefix.substring(0, 7);
               if (prefix == "object_")
               {
                  name.substring(7, name.length());
                  id = gDatabase.getProtoSquad(name);
                  if (id != cInvalidProtoSquadID)
                     mDB.mProtoSquadMap[i] = id;
               }
            }
         }
         else
         {
            mDB.mProtoSquadMap[i] = id;
            if (object)
            {
               const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(gDatabase.getProtoObject(name));
               if (pProtoObject)
               {
                  id = pProtoObject->getProtoSquadID();
                  if (id != cInvalidProtoSquadID)
                     mDB.mProtoSquadMap[i] = id;
               }
            }
         }
      }
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   if (!mDB.mProtoTechMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mProtoTechMap[i] = gDatabase.getProtoTech(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 1000);
   if (!mDB.mProtoPowerMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mProtoPowerMap[i] = gDatabase.getProtoPowerIDByName(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 20000);
   if (!mDB.mObjectTypeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mObjectTypeMap[i] = gDatabase.getObjectType(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 200);
   if (!mDB.mResourceMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mResourceMap[i] = gDatabase.getResource(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 200);
   if (!mDB.mRateMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mRateMap[i] = gDatabase.getRate(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 200);
   if (!mDB.mPopMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mPopMap[i] = gDatabase.getPop(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 10000);
   if (!mDB.mWeaponTypeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mWeaponTypeMap[i] = gDatabase.getWeaponTypeIDByName(name);
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 200);
   if (!mDB.mDamageTypeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mDamageTypeMap[i] = gDatabase.getDamageType(name);
   }

   if (BSaveDB::mGameFileVersion >= 8)
   {
      gDamageTemplateManager.reset();
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 1000);
      //if (!mDB.mDamageTemplateMap.setNumber(count))
      //   return false;
      if (!mDB.mDamageTemplateNames.setNumber(count) || !mDB.mDamageTemplateModelIDs.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         long modelIndex;
         GFREADVAR(mpStream, long, modelIndex);
         mDB.mDamageTemplateNames[i] = name;
         mDB.mDamageTemplateModelIDs[i] = modelIndex;
         //remapGrannyModelID(modelIndex);
         //mDB.mDamageTemplateMap[i] = gDamageTemplateManager.getOrCreateDamageTemplate(name, modelIndex);
      }
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 1000);
   if (!mDB.mAnimTypeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mAnimTypeMap[i] = gVisualManager.getAnimType(name);
   }

   if (BSaveDB::mGameFileVersion >= 6)
   {
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 2000);
      if (!mDB.mAttachmentTypeMap.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, name, 100);
         mDB.mAttachmentTypeMap[i] = gVisualManager.getAttachmentType(name);
      }
   }

   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 250);
   if (!mDB.mActionTypeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      GFREADSTRING(mpStream, BSimString, name, 100);
      mDB.mActionTypeMap[i] = gActionManager.getActionType(name);
   }

   int maxCount = mDB.mProtoObjectMap.getNumber();
   int newProtoObjectCount = gDatabase.getNumberProtoObjects();
   if (!mDB.mProtoActionMap.setNumber(newProtoObjectCount))
      return false;
   if (!mDB.mWeaponMap.setNumber(newProtoObjectCount))
      return false;
   count = 0;
   int16 saveProtoObjectID;
   GFREADVAR(mpStream, int16, saveProtoObjectID);
   while (saveProtoObjectID != -1)
   {
      count++;
      if (count > maxCount)
         return false;
      int newProtoObjectID = mDB.mProtoObjectMap[saveProtoObjectID];
      if (newProtoObjectID != -1)
      {
         BSmallDynamicSimArray<int>& protoActionMap = mDB.mProtoActionMap[newProtoObjectID];
         BSmallDynamicSimArray<int>& weaponMap = mDB.mWeaponMap[newProtoObjectID];
         BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(newProtoObjectID);
         BTactic* pTactic = (pProtoObject ? pProtoObject->getTactic() : NULL);
         uint8 protoActionCount;
         GFREADVAR(mpStream, uint8, protoActionCount);
         GFVERIFYCOUNT(protoActionCount, 200);
         if (!protoActionMap.setNumber(protoActionCount))
            return false;
         for (uint8 j=0; j<protoActionCount; j++)
         {
            GFREADSTRING(mpStream, BSimString, name, 100);
            protoActionMap[j] = (pTactic ? pTactic->getProtoActionID(name) : -1);
         }
         uint8 weaponCount;
         GFREADVAR(mpStream, uint8, weaponCount);
         GFVERIFYCOUNT(weaponCount, 200);
         if (!weaponMap.setNumber(weaponCount))
            return false;
         for (uint8 j=0; j<weaponCount; j++)
         {
            GFREADSTRING(mpStream, BSimString, name, 100);
            weaponMap[j] = (pTactic ? pTactic->getWeaponID(name) : -1);
         }
      }
      else
      {
         uint8 protoActionCount;
         GFREADVAR(mpStream, uint8, protoActionCount);
         GFVERIFYCOUNT(protoActionCount, 200);
         for (uint8 j=0; j<protoActionCount; j++)
            GFREADSTRING(mpStream, BSimString, name, 100);
         uint8 weaponCount;
         GFREADVAR(mpStream, uint8, weaponCount);
         GFVERIFYCOUNT(weaponCount, 200);
         for (uint8 j=0; j<weaponCount; j++)
            GFREADSTRING(mpStream, BSimString, name, 100);
      }
      GFREADVAR(mpStream, int16, saveProtoObjectID);
   }

   GFREADVAR(mpStream, int, mDB.mNumUniqueProtoObjects);
   GFVERIFYCOUNT(mDB.mNumUniqueProtoObjects, 100);
   GFREADVAR(mpStream, int, mDB.mNumUniqueProtoSquads);
   GFVERIFYCOUNT(mDB.mNumUniqueProtoSquads, 100);

   count = 0;
   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 1000);
   if (!mDB.mShapeMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
      mDB.mShapeMap[i] = -1;
   int loadedCount = 0;
   BString shapeName;
   for (;;)
   {
      long shapeID = -1;
      GFREADVAR(mpStream, long, shapeID);
      if (shapeID == -1)
         break;
      loadedCount++;
      GFVERIFYCOUNT(loadedCount, count);
      GFVERIFYCOUNT(shapeID, count-1);
      GFREADSTRING(mpStream, BString, shapeName, 100);
      mDB.mShapeMap[shapeID] = gPhysics->getShapeManager().getOrCreate(shapeName, true);
   }

   count = 0;
   GFREADVAR(mpStream, int, count);
   GFVERIFYCOUNT(count, 1000);
   if (!mDB.mPhysicsInfoMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
      mDB.mPhysicsInfoMap[i] = -1;
   loadedCount = 0;
   BString physicsInfoName;
   for (;;)
   {
      long physicsInfoID = -1;
      GFREADVAR(mpStream, long, physicsInfoID);
      if (physicsInfoID == -1)
         break;
      loadedCount++;
      GFVERIFYCOUNT(loadedCount, count);
      GFVERIFYCOUNT(physicsInfoID, count-1);
      GFREADSTRING(mpStream, BString, physicsInfoName, 100);
      mDB.mPhysicsInfoMap[physicsInfoID] = gPhysicsInfoManager.getOrCreate(physicsInfoName, true);
   }

   if (BSaveDB::mGameFileVersion >= 4)
   {
      GFREADVAR(mpStream, int, count);
      GFVERIFYCOUNT(count, 1000);
      if (!mDB.mProtoIconNames.setNumber(count) || !mDB.mProtoIconTypes.setNumber(count))
         return false;
      for (int i=0; i<count; i++)
      {
         GFREADSTRING(mpStream, BSimString, mDB.mProtoIconTypes[i], 100);
         GFREADSTRING(mpStream, BSimString, mDB.mProtoIconNames[i], 100);
      }
   }

   GFREADMARKER(mpStream, cSaveMarkerDB);
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::loadSetup()
{
   // Load player setup
   int count;
   GFREADVAR(mpStream, int, count);
   if (!mPlayers.setNumber(count))
      return false;

   for (int i=0; i<count; i++)
   {
      BSavePlayer& player = mPlayers[i];
      GFREADSTRING(mpStream, BSimString, player.mName, 100);
      if (mGameFileVersion >= 3)
         GFREADSTRING(mpStream, BUString, player.mDisplayName, 100);
      GFREADVAR(mpStream, long, player.mMPID);
      GFREADVAR(mpStream, BPlayerID, player.mScenarioID);
      GFREADVAR(mpStream, long, player.mCivID);
      GFREADVAR(mpStream, BTeamID, player.mTeamID);
      GFREADVAR(mpStream, long, player.mLeaderID);
      GFREADVAR(mpStream, BHalfFloat, player.mDifficulty);
      GFREADVAR(mpStream, int8, player.mPlayerType);
      GFREADMARKER(mpStream, cSaveMarkerSetupPlayer);
   }

   // Load team setup
   GFREADVAR(mpStream, int, count);
   if (!mTeams.setNumber(count))
      return false;

   for (int i=0; i<count; i++)
   {
      BSaveTeam& team = mTeams[i];
      GFREADARRAY(mpStream, long, team.mPlayers, uint8, 20);
      GFREADARRAY(mpStream, BRelationType, team.mRelations, uint8, 20);
   }

   GFREADMARKER(mpStream, cSaveMarkerSetupTeam);

   // Create the players
   count = mPlayers.getNumber();
   for (int i=0; i<count; i++)
   {
      BSavePlayer& player = mPlayers[i];
      BPlayer* pPlayer = gWorld->createPlayer(player.mMPID, player.mName, player.mDisplayName, getCivID(player.mCivID), getLeaderID(player.mLeaderID), player.mDifficulty, NULL, NULL, NULL, NULL, player.mPlayerType, player.mScenarioID, true);
      if (!pPlayer)
         return false;
      pPlayer->setTeamID(player.mTeamID);
   }

   // Create the teams
   count = mTeams.getNumber();
   for (int i=0; i<count; i++)
   {
      BSaveTeam& team = mTeams[i];
      BTeam* pTeam = gWorld->createTeam();
      if (!pTeam)
         return false;
      for (int j=0; j<team.mPlayers.getNumber(); j++)
         pTeam->addPlayer(team.mPlayers[j]);
      for (int j=0; j<count; j++)
         gWorld->setTeamRelationType(i, j, team.mRelations[j]);
   }

   // Load user setup data
   GFREADVAR(mpStream, int, mUser.mCurrentPlayer);
   GFREADVAR(mpStream, int, mUser.mCoopPlayer);
   GFREADVECTOR(mpStream, mUser.mHoverPoint);
   GFREADVECTOR(mpStream, mUser.mCameraHoverPoint);
   GFREADVECTOR(mpStream, mUser.mCameraPosition);
   GFREADVECTOR(mpStream, mUser.mCameraForward);
   GFREADVECTOR(mpStream, mUser.mCameraRight);
   GFREADVECTOR(mpStream, mUser.mCameraUp);
   if (BSaveUser::mGameFileVersion >= 3)
   {
      GFREADVAR(mpStream, float, mUser.mCameraDefaultPitch);
      GFREADVAR(mpStream, float, mUser.mCameraDefaultYaw);
      GFREADVAR(mpStream, float, mUser.mCameraDefaultZoom);
   }
   GFREADVAR(mpStream, float, mUser.mCameraPitch);
   GFREADVAR(mpStream, float, mUser.mCameraYaw);
   GFREADVAR(mpStream, float, mUser.mCameraZoom);
   GFREADVAR(mpStream, float, mUser.mCameraFOV);
   if (BSaveUser::mGameFileVersion >= 2)
   {
      GFREADVAR(mpStream, float, mUser.mCameraHoverPointOffsetHeight);
   }
   GFREADBITBOOL(mpStream, mUser.mHaveHoverPoint);
   if (BSaveUser::mGameFileVersion >= 3)
      GFREADBITBOOL(mpStream, mUser.mDefaultCamera)
   else
      mUser.mDefaultCamera = true;
   GFREADMARKER(mpStream, cSaveMarkerUser1);

   // Apply user setup data
   BUser* pUser = gUserManager.getPrimaryUser();
   pUser->gameInit(mUser.mCurrentPlayer, mUser.mCoopPlayer, mUser.mHoverPoint, false, mUser.mDefaultCamera, mUser.mCameraDefaultYaw, mUser.mCameraDefaultPitch, mUser.mCameraDefaultZoom);

   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::loadData()
{
   if (!mIsLoading)
   {
      BASSERT(0);
      return false;
   }

   if (!gWorld->load(mpStream, mSaveFileType))
      return false;
   GFREADMARKER(mpStream, cSaveMarkerWorld);

   if (!gUIManager->initPlayerSpecific(true))
      return false;

   if (!gUIManager->load(mpStream, mSaveFileType))
      return false;
   GFREADMARKER(mpStream, cSaveMarkerUI);

   BUser* pUser = gUserManager.getPrimaryUser();
   GFREADCLASSPTR(mpStream, mSaveFileType, pUser);
   GFREADMARKER(mpStream, cSaveMarkerUser2);

   GFREADMARKER(mpStream,cSaveMarkerEnd);

   close();

   // Light Effects
   int count = mDB.mLightEffectNames.getNumber();
   if (!mDB.mLightEffectMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      long dataHandle = -1;
      gLightEffectManager.getOrCreateData(mDB.mLightEffectNames[i], dataHandle);
      mDB.mLightEffectMap[i] = dataHandle;
   }

   // Particle Effects
   count = mDB.mParticleEffectNames.getNumber();
   if (!mDB.mParticleEffectMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      BParticleEffectDataHandle dataHandle = -1;
      gParticleGateway.getOrCreateData(mDB.mParticleEffectNames[i], dataHandle);
      mDB.mParticleEffectMap[i] = dataHandle;
   }

   // Damage templates
   count = mDB.mDamageTemplateNames.getNumber();
   if (!mDB.mDamageTemplateMap.setNumber(count))
      return false;
   for (int i=0; i<count; i++)
   {
      long modelIndex = mDB.mDamageTemplateModelIDs[i];
      remapGrannyModelID(modelIndex);
      mDB.mDamageTemplateMap[i] = gDamageTemplateManager.getOrCreateDamageTemplate(mDB.mDamageTemplateNames[i], modelIndex);
   }

   if (!gWorld->postLoad(mSaveFileType))
      return false;

   // Finish setting up the user
   if (!setupUser())
      return false;

   if (mIsCampaignLoad)
      gUserManager.getPrimaryUser()->setDefaultDevice(mCampaignContentData.DeviceID);

   mIsLoading = false;

   mSavedCampaign = mIsCampaignLoad;

   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::setupUser()
{
   BUser* pUser = gUserManager.getPrimaryUser();

   if (BSaveUser::mGameFileVersion < 3)
   {
      pUser->setCameraPitch(mUser.mCameraPitch);
      pUser->setCameraYaw(mUser.mCameraYaw);
      pUser->setCameraZoom(mUser.mCameraZoom);
      pUser->setCameraFOV(mUser.mCameraFOV);
      pUser->setCameraHoverPointOffsetHeight(mUser.mCameraHoverPointOffsetHeight);

      BCamera* pCamera=pUser->getCamera();
      pCamera->setCameraLoc(mUser.mCameraPosition);
      pCamera->setCameraDir(mUser.mCameraForward);
      pCamera->setCameraRight(mUser.mCameraRight);
      pCamera->setCameraUp(mUser.mCameraUp);

      pUser->setFlagUpdateHoverPoint(true);   
   }

   BPlayer* pPlayer = gWorld->getPlayer(pUser->getPlayerID());
   if (pPlayer)
   {
      pPlayer->setXUID(pUser->getXuid());
   }


   BSimString fileName = mGameFileName;
   pUser->setSaveGame(fileName);

   return true;
}

//==============================================================================
//==============================================================================
int BSaveGame::getProtoActionID(BProtoObjectID newProtoObjectID, int saveProtoActionID) const 
{ 
   if (newProtoObjectID < 0 || newProtoObjectID >= mDB.mProtoActionMap.getNumber())
      return -1;
   const BSmallDynamicSimArray<int>& protoActionMap = mDB.mProtoActionMap[newProtoObjectID];
   if (saveProtoActionID < 0 || saveProtoActionID >= protoActionMap.getNumber())
      return -1;
   return protoActionMap[saveProtoActionID];
}

//==============================================================================
//==============================================================================
int BSaveGame::getWeaponID(BProtoObjectID newProtoObjectID, int saveWeaponID) const 
{ 
   if (newProtoObjectID < 0 || newProtoObjectID >= mDB.mWeaponMap.getNumber())
      return -1;
   const BSmallDynamicSimArray<int>& protoWeaponMap = mDB.mWeaponMap[newProtoObjectID];
   if (saveWeaponID < 0 || saveWeaponID >= protoWeaponMap.getNumber())
      return -1;
   return protoWeaponMap[saveWeaponID];
}

//==============================================================================
//==============================================================================
bool BSaveGame::remapProtoActionID(BProtoObjectID newProtoObjectID, int& saveProtoActionID) const
{
   if (newProtoObjectID < 0 || newProtoObjectID >= mDB.mProtoActionMap.getNumber())
   {
      saveProtoActionID = -1;
      return false;
   }
   const BSmallDynamicSimArray<int>& protoActionMap = mDB.mProtoActionMap[newProtoObjectID];
   if (saveProtoActionID < 0 || saveProtoActionID >= protoActionMap.getNumber())
   {
      saveProtoActionID = -1;
      return false;
   }
   saveProtoActionID = protoActionMap[saveProtoActionID];
   if (saveProtoActionID == -1)
      return false;
   return true;
}

//==============================================================================
//==============================================================================
bool BSaveGame::remapWeaponID(BProtoObjectID newProtoObjectID, int& saveWeaponID) const
{
   if (newProtoObjectID < 0 || newProtoObjectID >= mDB.mWeaponMap.getNumber())
   {
      saveWeaponID= -1;
      return false;
   }
   const BSmallDynamicSimArray<int>& protoWeaponMap = mDB.mWeaponMap[newProtoObjectID];
   if (saveWeaponID < 0 || saveWeaponID >= protoWeaponMap.getNumber())
   {
      saveWeaponID= -1;
      return false;
   }
   saveWeaponID = protoWeaponMap[saveWeaponID];
   if (saveWeaponID == -1)
      return false;
   return true;
}

//==============================================================================
//==============================================================================
void BSaveGame::logStats(const char* pName)
{
   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   int64 delta = (int64)(time.QuadPart - mStartTime);
   double totaltime = static_cast<double>(delta/mTimerFrequencyFloat);
   delta = (int64)(time.QuadPart - mLastTime);
   double elapsed = static_cast<double>(delta/mTimerFrequencyFloat);
   gConsole.output(cMsgDebug, "save game stat %s, totaltime=%f, elapsed=%f, size=%I64u", pName, totaltime, elapsed, mpStream->curOfs());
   mLastTime = time.QuadPart;
}

//==============================================================================
//==============================================================================
int BSaveGame::getProtoObjectID(int id) const
{ 
   if (id < 0)
      return -1;
   if (id >= mDB.mProtoObjectMap.getNumber())
      return getUniqueProtoObjectID(id);
   return mDB.mProtoObjectMap[id];
}

//==============================================================================
//==============================================================================
int BSaveGame::getProtoSquadID(int id) const
{ 
   if (id < 0)
      return -1;
   if (id >= mDB.mProtoSquadMap.getNumber())
      return getUniqueProtoSquadID(id);
   return mDB.mProtoSquadMap[id];
}

//==============================================================================
//==============================================================================
int BSaveGame::getUniqueProtoObjectID(int id) const
{
   int oldProtoCount = mDB.mProtoObjectMap.getNumber();
   int newProtoCount = gDatabase.getNumberProtoObjects();
   int playerID = GETUNIQUEPROTOPLAYERID(id);
   int protoID = GETUNIQUEPROTOID(id);
   if (playerID >= 0 && playerID < cMaximumSupportedPlayers && protoID >= oldProtoCount && protoID < oldProtoCount + mDB.mNumUniqueProtoObjects)
   {
      int oldIndex = protoID - oldProtoCount;
      int newIndex = newProtoCount + oldIndex;
      return MAKEUNIQUEPID(playerID, newIndex);
   }
   return -1;
}

//==============================================================================
//==============================================================================
int BSaveGame::getUniqueProtoSquadID(int id) const
{ 
   int oldProtoCount = mDB.mProtoSquadMap.getNumber();
   int newProtoCount = gDatabase.getNumberProtoSquads();
   int playerID = GETUNIQUEPROTOPLAYERID(id);
   int protoID = GETUNIQUEPROTOID(id);
   if (playerID >= 0 && playerID < cMaximumSupportedPlayers && protoID >= oldProtoCount && protoID < oldProtoCount + mDB.mNumUniqueProtoSquads)
   {
      int oldIndex = protoID - oldProtoCount;
      int newIndex = newProtoCount + oldIndex;
      return MAKEUNIQUEPID(playerID, newIndex);
   }
   return -1;
} 

//==============================================================================
//==============================================================================
void BSaveGame::postUISetupFixup()
{
   int oldIconCount = mDB.mProtoIconTypes.getNumber();
   BSmallDynamicSimArray<int> iconRemap;
   if (!iconRemap.setNumber(oldIconCount))
      return;

   BUIContext* pUIContext = gUserManager.getPrimaryUser()->getUIContext();
   int newIconCount = pUIContext->getProtoIconCount();

   for (int i=0; i<oldIconCount; i++)
   {
      if (i<newIconCount)
      {
         const BFlashProtoIcon* pIcon = pUIContext->getProtoIcon(i);
         if (pIcon->mType == mDB.mProtoIconTypes[i] && pIcon->mName == mDB.mProtoIconNames[i])
         {
            iconRemap[i] = i;
            continue;
         }
      }
      iconRemap[i] = pUIContext->lookupIconID(mDB.mProtoIconTypes[i], mDB.mProtoIconNames[i]);
   }

   for (int i=1; i<gWorld->getNumberPlayers(); i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(i);
      for (int j=0; j<pPlayer->getNumberProtoObjects(); j++)
      {
         BProtoObject* pProtoObject = pPlayer->getProtoObject(j);
         int oldIcon = pProtoObject->getCircleMenuIconID();
         if (oldIcon >= 0 && oldIcon < oldIconCount)
         {
            int newIcon = iconRemap[oldIcon];
            pProtoObject->setCircleMenuIconID(newIcon);
         }
      }
      for (int j=0; j<pPlayer->getNumberProtoSquads(); j++)
      {
         BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(j);
         int oldIcon = pProtoSquad->getCircleMenuIconID();
         if (oldIcon >= 0 && oldIcon < oldIconCount)
         {
            int newIcon = iconRemap[oldIcon];
            pProtoSquad->setCircleMenuIconID(newIcon);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSaveGame::prepCampaignLoad()
{
   mIsCampaignLoad = false;
   mSaveFileCorrupt = false;
   mLoadDeviceRemove = false;

   BUser* pUser = gUserManager.getPrimaryUser();
   DWORD userIndex = pUser->getPort();
   XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();
   if (deviceID==XCONTENTDEVICE_ANY)
   {
      mLoadDeviceRemove = true;
      return false;
   }

   bool found = false;

   HANDLE hEnum = INVALID_HANDLE_VALUE;
   XCONTENT_DATA contentData[10];
   DWORD cbBuffer = 0;
   DWORD retval = XContentCreateEnumerator(userIndex, deviceID, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_ENUM_EXCLUDECOMMON, 10, &cbBuffer, &hEnum);
   if (retval != ERROR_SUCCESS)
   {
      if (hEnum != INVALID_HANDLE_VALUE)
         CloseHandle(hEnum);
      mLoadDeviceRemove = true;
      return false;
   }

   BXContentStream contentStream;
   BSimString fileName;

   DWORD dwReturnCount;
   do 
   {
      DWORD dwRet = XEnumerate(hEnum, contentData, sizeof(contentData), &dwReturnCount, NULL);
      if (dwRet == ERROR_SUCCESS)
      {
         for (uint i=0; i < dwReturnCount; ++i)
         {
            if (_strnicmp(contentData[i].szFileName, "campaign.sav", 12) != 0)
               continue;

            memcpy(&mCampaignContentData, &(contentData[i]), sizeof(XCONTENT_DATA));
            found = true;
            break;
         }
      }
   } while (dwReturnCount == 10);

   if (hEnum != INVALID_HANDLE_VALUE)
      CloseHandle(hEnum);

   if (!found)
   {
      mLoadDeviceRemove = true;
      return false;
   }

   mSaveFileCorrupt = true;

   // Copy all the data from user storage into a memory stream.
   mpRawStream = new BDynamicStream();
   if (!mpRawStream)
      return false;
   mpStream = mpRawStream;
   BStream* pContentStream = getXContentStream(gUserManager.getPrimaryUser()->getPort(), mCampaignContentData, cSFReadable | cSFOpenExisting | cSFSeekable | cSFOptimizeForSequentialAccess);
   if (!pContentStream)
   {
      XDEVICE_DATA deviceData;
      if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
         mLoadDeviceRemove = true;
      return false;
   }
   uint64 bytesCopied = 0;
   bool copySuccess = BStream::copyStream(*pContentStream, *mpStream, &bytesCopied);
   pContentStream->close();
   delete pContentStream;
   if (!copySuccess)
   {
      XDEVICE_DATA deviceData;
      if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
         mLoadDeviceRemove = true;
      return false;
   }

   // Verify the data is valid by decrypting it.
   mpStream->seek(0);
   mDetails.mValue = 0;
   if (mpStream->readBytes(&mDetails, sizeof(BGameFileVersion)) != sizeof(BGameFileVersion) || mDetails.mVersion > cBaseVersion)
      return false;
   mBaseVersion = mDetails.mVersion;
   BSHA1Gen sha1Gen;
   sha1Gen.update(&mDetails, sizeof(BGameFileVersion));
   if (mDetails.mEncryptHeader)
   {
      BDecryptedStream* pDecryptStream = new BDecryptedStream(mpStream, cKeyPhrase);
      pDecryptStream->setBaseOfs(mpStream->curOfs());
      if (!mHeader.deserialize(pDecryptStream, sha1Gen))
      {
         delete pDecryptStream;
         return false;
      }
      delete pDecryptStream;
   }
   else
   {
      if (!mHeader.deserialize(mpStream, sha1Gen))
         return false;
   }
   if (mDetails.mEncryptData)
   {
      uint64 k1, k2, k3;
      teaCryptInitKeys(cKeyPhrase, k1, k2, k3);
      mpDecryptRecStream = new BDecryptedStream(mpStream, k1, k2, mHeader.getKey3());
      mpDecryptRecStream->setBaseOfs(mpRawStream->curOfs());
      if (mpDecryptRecStream->errorStatus())
         return false;
      const int cBufSize = 65536*2;
      BByteArray buf(cBufSize);
      for ( ; ; )
      {
         const int n = mpDecryptRecStream->readBytes(buf.getPtr(), cBufSize);
         if (n < cBufSize)
         {
            if (mpDecryptRecStream->errorStatus())
               return false;
            if (!n)
               break;
         }
      }
      if (mpDecryptRecStream->errorStatus())
         return false;
      delete mpDecryptRecStream;
      mpDecryptRecStream = NULL;
   }

   // Reset the dynamic stream back to the beginning.
   mpStream->seek(0);

   mIsCampaignLoad = true;

   mSaveFileCorrupt = false;

   return true;
}

//==============================================================================
//==============================================================================
DWORD BSaveGame::getOverlappedResult(XOVERLAPPED* pOverlapped, bool* pWaitDialog)
{
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   uint startTime = timeGetTime();
   while (!XHasOverlappedIoCompleted(pOverlapped))
   {
      if (timeGetTime() - startTime > 1000)
      {
         if (pUIGlobals && !pUIGlobals->getWaitDialogVisible())
         {
            // 25989 Please wait...
            pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(25989));
            *pWaitDialog = true;
         }
      }
      gGame.updateRender();
      Sleep(1);
   }
   return ERROR_SUCCESS;
}

//==============================================================================
//==============================================================================
bool BSaveGame::getCampaignSaveExists(BGameSettings* pSettings, BUString* pName)
{
   mSaveFileCorrupt = false;

   DWORD userIndex = gUserManager.getPrimaryUser()->getPort();
   XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();
   if (deviceID==XCONTENTDEVICE_ANY)
      return false;

   bool found = false;
   HANDLE hEnum = INVALID_HANDLE_VALUE;
   XCONTENT_DATA contentData[10];
   DWORD cbBuffer = 0;
   DWORD retval = XContentCreateEnumerator(userIndex, deviceID, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_ENUM_EXCLUDECOMMON, 10, &cbBuffer, &hEnum);
   if (retval == ERROR_SUCCESS)
   {
      bool waitdialog = false;
      XOVERLAPPED overlapped;
      DWORD dwReturnCount = 0;
      do 
      {
         Utils::FastMemSet(&overlapped, 0, sizeof(XOVERLAPPED));
         DWORD dwRet = XEnumerate(hEnum, contentData, sizeof(contentData), NULL, &overlapped);
         if (dwRet == ERROR_IO_PENDING)
         {
            getOverlappedResult(&overlapped, &waitdialog);
            dwRet = XGetOverlappedResult(&overlapped, &dwReturnCount, TRUE);
         }

         if (dwRet == ERROR_SUCCESS)
         {
            for (uint i=0; i < dwReturnCount; ++i)
            {
               if (_strnicmp(contentData[i].szFileName, "campaign.sav", 12) != 0)
                  continue;

               bool corrput = false;
               BOOL fUserIsCreator = FALSE;
               Utils::FastMemSet(&overlapped, 0, sizeof(XOVERLAPPED));
               DWORD dwRet = XContentGetCreator(userIndex, &(contentData[i]), &fUserIsCreator, NULL, &overlapped);
               if (dwRet == ERROR_IO_PENDING)
               {
                  getOverlappedResult(&overlapped, &waitdialog);
                  if (XGetOverlappedResult(&overlapped, &dwRet, TRUE) != ERROR_SUCCESS)
                     dwRet = ERROR_INVALID_FUNCTION;
               }
               if (dwRet != ERROR_SUCCESS || !fUserIsCreator)
               {
                  corrput = true;
                  mSaveFileCorrupt = true;
               }

               if (pName)
               {
                  if (corrput)
                     pName->format(L"%s", gDatabase.getLocStringFromID(25994).getPtr());
                  else
                     pName->format(L"%s", contentData[i].szDisplayName);
               }

               if (pSettings)
               {
                  if (corrput || !getHeaderAndSettings(BSimString("campaign"), true, &(contentData[i]), pSettings))
                  {
                     mSaveFileCorrupt = true;
                     // Fill out settings indicating this is an invalid campaign save file.
                     BUser* pUser = gUserManager.getPrimaryUser();
                     pSettings->setLong(BGameSettings::cPlayerCount, 1);
                     pSettings->setString(BGameSettings::cMapName, "InvalidCampaign");
                     pSettings->setLong(BGameSettings::cMapIndex, -1);
                     pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
                     pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
                     pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
                     pSettings->setLong(BGameSettings::cPlayer1Type, BGameSettings::cPlayerHuman);
                     pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);
                     pSettings->setString(BGameSettings::cLoadName, "campaign");
                     pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeCampaign);
                     pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
                     pSettings->setLong(BGameSettings::cGameMode, 0);
                  }
               }

               found = true;
               break;
            }
         }
      } while (dwReturnCount == 10);

      if (waitdialog)
      {
         BUIGlobals* pUIGlobals = gGame.getUIGlobals();
         if (pUIGlobals)
            pUIGlobals->setWaitDialogVisible(false);
      }
   }
   if (hEnum != INVALID_HANDLE_VALUE)
      CloseHandle(hEnum);
   return found;
}

//==============================================================================
//==============================================================================
XCONTENTDEVICEID BSaveGame::autoSelectStorageDevice()
{
   DWORD userIndex = gUserManager.getPrimaryUser()->getPort();

   BGameSettings settings;

   // search all the storage devices.
   XCONTENTDEVICEID deviceID = XCONTENTDEVICE_ANY;

   bool found = false;
   uint foundCount = 0;
   HANDLE hEnum = INVALID_HANDLE_VALUE;
   XCONTENT_DATA contentData[10];
   DWORD cbBuffer = 0;
   DWORD retval = XContentCreateEnumerator(userIndex, deviceID, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_ENUM_EXCLUDECOMMON, 10, &cbBuffer, &hEnum);
   if (retval == ERROR_SUCCESS)
   {
      DWORD dwReturnCount = 0;
      do 
      {
         DWORD dwRet = XEnumerate(hEnum, contentData, sizeof(contentData), &dwReturnCount, NULL);
         if (dwRet == ERROR_SUCCESS)
         {
            for (uint i=0; i < dwReturnCount; ++i)
            {
               if (_strnicmp(contentData[i].szFileName, "campaign.sav", 12) != 0)
                  continue;
               deviceID = contentData[i].DeviceID;
               found = true;
               foundCount++;
            }
         }
      } while (dwReturnCount == 10);
   }
   if (hEnum != INVALID_HANDLE_VALUE)
      CloseHandle(hEnum);

   // either XCONTENTDEVICE_ANY or the specific device from the save game.
   if (foundCount == 1)
      return deviceID;
   else
      return XCONTENTDEVICE_ANY;
}

//==============================================================================
//==============================================================================
bool BSaveGame::createNewCampaignFile()
{
   // Display the saving message.
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
   {
      pUIGlobals->showWaitDialog(gDatabase.getLocStringFromID(23501));
      gGame.updateRender();
      Sleep(1);
   }

   DWORD startTime = timeGetTime();

   BUser* pUser = gUserManager.getPrimaryUser();
   if (!pUser || !pUser->isSignedIn())
      return false;

   // ajl 12/2/08 PHX-18884 - Verify enough space is available before attempting to create save file.
   XCONTENTDEVICEID deviceID = pUser->getDefaultDevice();
   if (deviceID == XCONTENTDEVICE_ANY)
      return false;
   XDEVICE_DATA deviceData;
   if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
      return false;
   if (deviceData.ulDeviceFreeBytes < BSaveGame::MAX_SIZE_IN_BYTES)
      return false;

   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   pSettings->setLong(BGameSettings::cPlayerCount, 1);
   pSettings->setString(BGameSettings::cMapName, "NewCampaign");
   pSettings->setLong(BGameSettings::cMapIndex, -1);
   pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
   pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
   pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
   pSettings->setLong(BGameSettings::cPlayer1Type, BGameSettings::cPlayerHuman);
   pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);
   pSettings->setString(BGameSettings::cLoadName, "campaign");
   pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeCampaign);
   pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
   pSettings->setLong(BGameSettings::cGameMode, 0);

   if (!saveBase())
      return false;

   padSaveFile();

   if (!saveFile(gUserManager.getPrimaryUser()->getPort()))
      return false;

   // Make sure wait dialog is displayed for at least 3 seconds to meet TCR requirement
   if (pUIGlobals)
   {
      while (timeGetTime() - startTime < 3000)
      {
         gGame.updateRender();
         Sleep(1);
      }
   }
   
   return true;
}

//==============================================================================
// Using a fixed file size for campaign saves, so pad the file to the fixed size.
//==============================================================================
void BSaveGame::padSaveFile()
{
   // Have to close the compression stream so that we can write out a fixed # of bytes.
   if (mpDeflateRecStream != NULL)
   {
      mpDeflateRecStream->close();
      delete mpDeflateRecStream;
      mpDeflateRecStream = NULL;
      if (mpEncryptRecStream)
         mpStream = mpEncryptRecStream;
      else
         mpStream = mpRawStream;
   }

   // Write extra 0's to pad out the file.
   uint64 offset = mpStream->curOfs();

   uint64 fixedSize = FIXED_FILE_SIZE;
   uint64 calcSize = XContentCalculateSize(fixedSize, 4);
   if (calcSize > MAX_SIZE_IN_BYTES)
   {
      uint64 diff = calcSize - MAX_SIZE_IN_BYTES;
      if (diff < fixedSize)
         fixedSize -= diff;
      else
         fixedSize = 0;
   }

   uint64 newSize = 0;
   if (offset < fixedSize)
   {
      uint64 numBytes = fixedSize - offset;
      const uint cBufferCount = 10240;
      DWORD buffer[cBufferCount];
      DWORD seed = (DWORD)RandomUtils::GenerateRandomSeed();
      for (;;)
      {
         seed = RandomUtils::RandomFill32(buffer, cBufferCount, seed);
         uint writeBytes = min(numBytes, sizeof(buffer));
         mpStream->writeBytes(buffer, writeBytes);
         if (numBytes <= writeBytes)
            break;
         numBytes -= writeBytes;
      }
      //mpStream->writeDuplicateBytes(0, numBytes);
      newSize = mpStream->curOfs();
   }
}

