//==============================================================================
// consolefuncsgame.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "consolefuncsgame.h"

#include "camera.h"
#include "commandmanager.h"
#include "commands.h"
#include "database.h"
#include "entity.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "keyboard.h"
#include "modemanager.h"
#include "modegame.h"
#include "modemenu.h"
#include "modepartyroom2.h"
#include "protopower.h"
#include "protosquad.h"
#include "scenario.h"
#include "tactic.h"
#include "techtree.h"
#include "terrainsimrep.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "weapontype.h"
#include "world.h"
#include "xssyscallmodule.h"
#include "selectionmanager.h"
#include "statsManager.h"
#include "lspManager.h"
#include "syncmanager.h"
#include "achievementmanager.h"
#include "userachievements.h"
#include "contentfile.h"
#include "contentreader.h"
#include "contentwriter.h"
#include "overlapped.h"
#include "XLastGenerated.h"
#include "team.h"
#include "timermanager.h"
#include "triggermanager.h"
#include "uimanager.h"
#include "UIWidgets.h"
#include "worldsoundmanager.h"
#include "UIGlobals.h"
#include "savegame.h"
#include "userprofilemanager.h"
#include "campaignmanager.h"
#include "scoremanager.h"
#include "skullmanager.h"
#include "uiticker.h"

// Setup XS macros. Don't change this.
#include "xsconfigmacros.h"

//xphysics
#include "physics.h"
#include "physicsobject.h"

//Live
#include "liveSystem.h"

#include "config.h"
#include "econfigenum.h"
#include "configsgamerender.h"

#include "renderHelperThread.h"

// xvince
#include "vincehelper.h"

#include "pather.h"
#include "terrain.h"

//==============================================================================
// exitGame
//==============================================================================
static void exitGame(bool confirm)
{
   confirm;
   gGame.exit();
}

//==============================================================================
// loadScenario
//==============================================================================
static void loadScenario(const char* scenarioName, int gameType)
{
   gModeManager.setMode(BModeManager::cModeMenu);
   BModeMenu* pMode=gModeManager.getModeMenu();
   if(pMode)
   {
      BGameSettings* pSettings = gDatabase.getGameSettings();
      if(pSettings)
      {
         BSimString gameID;
         MVince_CreateGameID(gameID);
         BUser* user = gUserManager.getPrimaryUser();
         user->setFlagUserActive(true);

         pSettings->setLong(BGameSettings::cPlayerCount, 2);
         pSettings->setString(BGameSettings::cMapName, scenarioName);
         pSettings->setLong(BGameSettings::cMapIndex, -1);
         pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
         pSettings->setString(BGameSettings::cGameID, gameID);
         pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
         pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
         // If we were provided a specific game type to load this scenario as, use that instead of just 'scenario'
         if (gameType >= 0)
         {
            pSettings->setLong(BGameSettings::cGameType, gameType);
         }
         pSettings->setLong(BGameSettings::cGameMode, 0);
         pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

         //gModeManager.setMode(BModeManager::cModeGame);
         pMode->setNextState(BModeMenu::cStateGotoGame);
      }
   }
}

//==============================================================================
// reloadScenario
//==============================================================================
static void reloadScenario(bool leaveCamera)
{
   if(gModeManager.inModeGame())
   {
      gConsoleOutput.output(cMsgConsole, "Reloading scenario.");
      
      gScenario.load(true, leaveCamera);
   }
}

//==============================================================================
// loadVisual
//==============================================================================
static void loadVisual(const char* visualName)
{ 
   gConsoleOutput.output(cMsgError, "Made it to loadVisual function");

   gModeManager.setMode(BModeManager::cModeMenu);
   BModeMenu* pMode=gModeManager.getModeMenu();
   if(pMode)
   {
      pMode->setSelectedFileName(visualName);
      pMode->setNextState(BModeMenu::cStateGotoModelView);
   }
}

//==============================================================================
// reloadStrings
//==============================================================================
static void reloadStrings()
{
   gDatabase.setupLocStrings(true);
}

//==============================================================================
// resetCameraDefaults
//==============================================================================
static void resetCameraDefaults()
{
   gUserManager.getPrimaryUser()->resetCameraDefaults();
}

#ifndef BUILD_FINAL
//==============================================================================
// test
//==============================================================================
static void test( void )
{
   //volatile char* p = (char*)0;
   //*p = 0;
   
   //BFATAL_FAIL("!");
   
   gConsole.output("Test function was called!");
}
#endif

//==============================================================================
// dumpConfigListConsole
//==============================================================================
void dumpConfigListConsole(const char *str)
{
   // Callback method for usage by BConfig::dumpAll to list all existing configs
   gConsoleOutput.output(cMsgConsole, "Config: %s", str);
}

//==============================================================================
// getConfigList
//==============================================================================
void getConfigList()
{
   gConfig.dumpAll(dumpConfigListConsole);
   gConsoleOutput.output(cMsgConsole, "Done listing configs.");
}

//==============================================================================
// configToggle
//==============================================================================
void configToggle(const char* pConfigName)
{
   if ((pConfigName) && (strlen(pConfigName)))
   {
      gConfig.toggleDefine(pConfigName);
      
      gConsoleOutput.output(cMsgConsole, "Config \"%s\" is now %s.", pConfigName, gConfig.isDefined(pConfigName) ? "defined" : "undefined");
   }
   else   
      gConsole.output("Error: Must specify config name!");      
}

//==============================================================================
// configSetInt
//==============================================================================
static void configSetInt(const char* configName, long v)
{
   // jce [9/22/2008] -- simpler version that allows the config to be previously undefined and still work
   gConfig.set(configName, v);
   
   /*
   BConfigData *pData = gConfig.findConfig(configName);
   if (!pData)
   {
      gConsole.output("Error: Invalid config!");
      return; 
   }

   if (pData->mType != BConfigData::cDataLong)
   {
      gConsole.output("Error: This config is not of type integer!");
      return;
   }

   gConfig.set(pData->mFormalIndex, v);
   */
   
   gConsole.output("Integer set.");
}

//==============================================================================
// configSetFloat
//==============================================================================
static void configSetFloat(const char* configName, float v)
{
   // jce [9/22/2008] -- simpler version that allows the config to be previously undefined and still work
   gConfig.set(configName, v);

   /*   
   BConfigData *pData = gConfig.findConfig(configName);
   if (!pData)
   {
      gConsole.output("Error: Invalid config!");
      return; 
   }

   if (pData->mType != BConfigData::cDataFloat)
   {
      gConsole.output("Error: This config is not of type float!");
      return;
   }

   gConfig.set(pData->mFormalIndex, v);
   */
   
   gConsole.output("Float set.");
}

//==============================================================================
// listProtoObjects
//==============================================================================
static void listProtoObjects(void)
{
   uint num = gDatabase.getNumberProtoObjects();
   BDynamicSimArray<BSimString> names;
   for (uint i = 0; i < num; i++)
      names.pushBack(gDatabase.getProtoObjectName(i));
   names.sort();
   for (uint i = 0; i < num; i++)
      gConsole.output(names[i].getPtr());
   gConsoleOutput.output(cMsgConsole, "%i total proto objects", num);
}

#ifndef BUILD_FINAL
//==============================================================================
// createSquadAtCursor
//==============================================================================
static void createSquadAtCursor(const char* szProtoName, long playerID, long num)
{
   if (!gWorld)
      return;

   long id = gDatabase.getProtoSquad(szProtoName);
   if (id == -1)
   {
      gConsole.output("Unknown proto squad name.");
      return;
   }
   
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &playerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeCreateSquad);
      pCommand->setData(id);
      pCommand->setData2(num);
      pCommand->setPosition(pUser->getHoverPoint());
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }

   gConsole.output(cMsgConsole, "Created %i squads.", num);
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// createObjectAtCursor
//==============================================================================
static void createObjectAtCursor(const char* szProtoName, long playerID, long num)
{
   if (!gWorld)
      return;

   long id = gDatabase.getProtoObject(szProtoName);
   if (id == -1)
   {
      BDynamicSimArray<BSimString> possibleMatches;
      
      uint num = gDatabase.getNumberProtoObjects();
      for (uint i = 0; i < num; i++)
      {
         if (strstr(gDatabase.getProtoObjectName(i), szProtoName) != NULL)
            possibleMatches.pushBack(BSimString(gDatabase.getProtoObjectName(i)));
      }
      
      if (possibleMatches.getSize() == 0)
      {
         gConsole.output(cMsgConsole, "Unknown proto object name \"%s\".", szProtoName);
      }
      else if (possibleMatches.getSize() == 1)
      {
         id = gDatabase.getProtoObject(possibleMatches[0]);
         
         gConsole.output(cMsgConsole, "Unknown proto object name \"%s\". However, a substring match was found: \"%s\".", szProtoName, possibleMatches[0].getPtr());
      }
      else
      {
         gConsole.output(cMsgConsole, "Unknown proto object name \"%s\", and there where too many potential matches:", szProtoName);

         for (uint i = 0; i < possibleMatches.getSize(); i++)
            gConsole.output(cMsgConsole, "%s", possibleMatches[i].getPtr());
      }
      
      if (id == -1)
         return;
   }
   
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &playerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeCreateObject);
      pCommand->setData(id);
      pCommand->setData2(num);
      pCommand->setPosition(pUser->getHoverPoint());
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }

   gConsole.output(cMsgConsole, "Created %i objects.", num);
}
#endif


#ifndef BUILD_FINAL
//==============================================================================
// launchTriggerScript
//==============================================================================
static void launchTriggerScript(const char* pTriggerScriptName)
{
   if (!gWorld)
      return;

   if (pTriggerScriptName)
   {
      BSimString scriptName(pTriggerScriptName);
      scriptName.removeExtension();
      scriptName.append(".triggerscript");
      BTriggerScriptID triggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, scriptName);
      if (triggerScriptID != cInvalidTriggerScriptID)
         gTriggerManager.activateTriggerScript(triggerScriptID);
   }
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// terminateTriggerScript
//==============================================================================
static void terminateTriggerScript(BTriggerScriptID triggerScriptID)
{
   if (!gWorld)
      return;

   BTriggerScript* pTS = gTriggerManager.getTriggerScript(triggerScriptID);
   if (pTS)
      pTS->markForCleanup();
}
#endif

#ifndef BUILD_FINAL
   //==============================================================================
   // setMenuState
   //==============================================================================
   static void setMenuState(const char* stateName)
   {
      // Translate unchanging automation/user strings into their (potentially changing) enum counterparts
      // These strings do not need to be localized since they are automation-only
      long state = -1;
      if      (!_stricmp(stateName, "main"))
         state = BModeMenu::cStateMain;
      else if (!_stricmp(stateName, "skirmish"))
         state = BModeMenu::cStateSkirmishSetup;
      else if (!_stricmp(stateName, "scenario"))
         state = BModeMenu::cStateScenario;
      else if (!_stricmp(stateName, "multiplayer"))
         state = BModeMenu::cStateGotoMultiplayer;
      else if (!_stricmp(stateName, "campaign"))
         state = BModeMenu::cStateGotoCampaign;
      else if (!_stricmp(stateName, "systemlink"))
         state = BModeMenu::cStateGotoSystemLink;
      
      if (state != -1)
      {
         gModeManager.setMode(BModeManager::cModeMenu);
         BModeMenu* pMenu=gModeManager.getModeMenu();
         if(pMenu)
         {
            pMenu->setNextState(state);
            gConsoleOutput.output(cMsgConsole, "Prepared next menu state.");
            return;
         }
      }
      gConsoleOutput.output(cMsgConsole, "Invalid menu state!");
   }
   
   //==============================================================================
   // setObjectiveState
   //==============================================================================
   static void setObjectiveState(int objectiveID, bool setDiscovered, bool setCompleted)
   {
      // Prepare to use the objective manager, if we are in-game and it exists
      BObjectiveManager* pObjectiveManager = (gWorld ? gWorld->getObjectiveManager() : NULL);
      if (!gModeManager.inModeGame() || !pObjectiveManager)
      {
         gConsoleOutput.output(cMsgConsole, "Unable to set objective state! (Not in-game)");
         return;
      }
      
      // Set the desired state information for the objective (if the objective exists)
      pObjectiveManager->setObjectiveDisplayed(objectiveID, setDiscovered);
      pObjectiveManager->setObjectiveCompleted(objectiveID, setCompleted);
      gConsoleOutput.output(cMsgConsole, "Set the objective state!");
   }
   
   //==============================================================================
   // setSettingString
   //==============================================================================
   static void setSettingString(const char* settingName, const char* value)
   {
      // Translate unchanging automation/user strings into their (potentially changing) enum counterparts
      // These strings do not need to be localized since they are automation-only
      long setting = -1;
      if (!_stricmp(settingName, "mapname"))
         setting = BGameSettings::cMapName;
      
      if (setting != -1)
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         if(pSettings && pSettings->setString(setting, value))
         {
            gConsoleOutput.output(cMsgConsole, "Settings value changed.");
            return;
         }
      }
      gConsoleOutput.output(cMsgConsole, "Settings value failed to change!");
   }
   
   //==============================================================================
   // setSettingInt
   //==============================================================================
   static void setSettingInt(const char* settingName, int value)
   {
      // Translate unchanging automation/user strings into their (potentially changing) enum counterparts
      // These strings do not need to be localized since they are automation-only
      long setting = -1;
      if      (!_stricmp(settingName, "playercount"))
         setting = BGameSettings::cPlayerCount;
      else if (!_stricmp(settingName, "maxplayers"))
         setting = BGameSettings::cMaxPlayers;
      
      if (setting != -1)
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         if(pSettings && pSettings->setLong(setting, value))
         {
            gConsoleOutput.output(cMsgConsole, "Settings value changed.");
            return;
         }
      }
      gConsoleOutput.output(cMsgConsole, "Settings value failed to change!");
   }
   
   //==============================================================================
   // setSettingPlayer
   //==============================================================================
   static void setSettingPlayer(const char* settingName, int player, int value)
   {
      // Translate unchanging automation/user strings into their (potentially changing) enum counterparts
      // These strings do not need to be localized since they are automation-only
      long setting = -1;
      if      (!_stricmp(settingName, "team"))
         setting = BGameSettings::cPlayerTeam;
      else if (!_stricmp(settingName, "civ"))
         setting = BGameSettings::cPlayerCiv;
      else if (!_stricmp(settingName, "leader"))
         setting = BGameSettings::cPlayerLeader;
      else if (!_stricmp(settingName, "type"))
         setting = BGameSettings::cPlayerType;
      // Adjust setting to be mapped to the specified player number
      setting = PSINDEX(player, setting); 
      
      if (setting >= 0 && setting < BGameSettings::cSettingCount)
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         if(pSettings && pSettings->setLong(setting, value))
         {
            gConsoleOutput.output(cMsgConsole, "Settings value changed.");
            return;
         }
      }
      gConsoleOutput.output(cMsgConsole, "Settings value failed to change!");
   }
   
   //==============================================================================
   // getBuildInfo
   //==============================================================================
   static void getBuildInfo(void)
   {
      gGame.outputBuildInfo();
   }
   
   //==============================================================================
   // getRenderedFrameInfo
   //==============================================================================
   static void getRenderedFrameInfo(void)
   {
      gConsoleOutput.output(cMsgConsole, "Last Viewport's Last Frame Rendered Stats:");
      gConsoleOutput.output(cMsgConsole, "Units: %d, Projectiles: %d, Dopples: %d, Other: %d", 
            gWorld->getNumberRenderedUnitsLastFrame(), 
            gWorld->getNumberRenderedProjectilesLastFrame(), 
            gWorld->getNumberRenderedDopplesLastFrame(), 
            gWorld->getNumberRenderedObjectsLastFrame());
   }
   
   //==============================================================================
   // getGameStateInfo
   //==============================================================================
   static void getGameStateInfo(void)
   {
      long   mode          = gModeManager.getModeType();
      BUser* pPrimaryUser  = gUserManager.getUser(BUserManager::cPrimaryUser);
      long   userMode      = (pPrimaryUser ? pPrimaryUser->getUserMode() : BUser::cUserModeNormal);
      const char*  userModeName  = NULL;
      char*  menuStateName = NULL;
      char*  modeName      = NULL;
      
      // Strings here do not need to be localized since they are automation-only
      // Try to get a friendly string for the current game mode
      switch (mode)
      {
         case BModeManager::cModeIntro:       modeName = "Intro";       break;
         case BModeManager::cModeMenu:        modeName = "Menu";        break;
         case BModeManager::cModeGame:        modeName = "Game";        break;
         case BModeManager::cModeViewer:      modeName = "Viewer";      break;
         case BModeManager::cModeCalibrate:   modeName = "Calibrate";   break;
         case BModeManager::cModeFlash:       modeName = "Flash";       break;
         case BModeManager::cModeCinematic:   modeName = "Cinematic";   break;
         case BModeManager::cModeModelView:   modeName = "ModelView";   break;
         case BModeManager::cModeCampaign2:   modeName = "Campaign2";    break;
         case BModeManager::cModePartyRoom2:  modeName = "PartyRoom2";  break;
         default:
            modeName = "Unknown";
            break;
      }
      
      // Try to get a friendly string for the current user mode (most applicable to in-game)
      userModeName = BUser::getUserModeName(userMode);
      
      // Try to get a friendly string for the menu state (if in the menu system)
      BModeMenu* pMenu = gModeManager.getModeMenu();
      if(mode == BModeManager::cModeMenu && pMenu)
      {
         long state = pMenu->getState();
         switch (state)
         {
            case BModeMenu::cStateIntroCinematic:   menuStateName = "IntroCinematic";   break;
            //case BModeMenu::cStateInitialSignin:    menuStateName = "InitialSignin";    break;
            //case BModeMenu::cStateAuthorizing:      menuStateName = "Authorizing";      break;
            //case BModeMenu::cStateAuthorizationFailed: menuStateName = "AuthorizationFailed"; break;
            case BModeMenu::cStateIntroCinematicEnded: menuStateName = "IntroCinematicEnded"; break;
            case BModeMenu::cStateAttractMode:      menuStateName = "AttractMode";      break;
            case BModeMenu::cStateMain:             menuStateName = "Main";             break;
            case BModeMenu::cStateSkirmish:         menuStateName = "Skirmish";         break;
            case BModeMenu::cStateLeaderboards:     menuStateName = "Leaderboards";     break;
            case BModeMenu::cStateOptions:          menuStateName = "Options";          break;
            case BModeMenu::cStateSkirmishSetup:    menuStateName = "SkirmishSetup";    break;
            case BModeMenu::cStateTimeline:         menuStateName = "Timeline";         break;
            case BModeMenu::cStateStartCampaign:    menuStateName = "StartCampaign";    break;
            case BModeMenu::cStateContinueCampaign: menuStateName = "ContinueCampaign"; break;
            case BModeMenu::cStateMultiplayerMenu:  menuStateName = "MultiplayerMenu";  break;
            case BModeMenu::cStateRecordGame:       menuStateName = "RecordGame";       break;
            case BModeMenu::cStateSaveGame:         menuStateName = "SaveGame";         break;
            case BModeMenu::cStateCredits:          menuStateName = "Credits";          break;
            case BModeMenu::cStateScenario:         menuStateName = "Scenario";         break;
            case BModeMenu::cStatePlayerSettings:   menuStateName = "PlayerSettings";   break;
            case BModeMenu::cStateFlash:            menuStateName = "Flash";            break;
            case BModeMenu::cStateCinematic:        menuStateName = "Cinematic";        break;
            case BModeMenu::cStateModelView:        menuStateName = "ModelView";        break;
            case BModeMenu::cStateExit:             menuStateName = "Exit";             break;
            case BModeMenu::cStateServiceRecord:    menuStateName = "ServiceRecord";    break;
            case BModeMenu::cStateGotoCampaign:
            case BModeMenu::cStateGotoMultiplayer:
            case BModeMenu::cStateGotoSystemLink:
            case BModeMenu::cStateGotoGame:
            case BModeMenu::cStateGotoViewer:
            case BModeMenu::cStateGotoCalibrate:
            case BModeMenu::cStateGotoRecordGame:
            case BModeMenu::cStateGotoSaveGame:
            case BModeMenu::cStateGotoFlash:
            case BModeMenu::cStateGotoCinematic:
            case BModeMenu::cStateGotoModelView:
               menuStateName = "Transitioning";
               break;
            default:
               menuStateName = "Unknown";
               break;
         }
      }

      // Try to get the UI Manager menu screen name, if one is active
      char* uiManagerScreenName = NULL;
      if (gUIManager && gUIManager->isNonGameUIVisible())
      {
         BUIManager::EScreen uiScreen = gUIManager->getCurrentScreenEnum();
         switch (uiScreen)
         {
            case BUIManager::cObjectivesScreen:    uiManagerScreenName = "Objectives";    break;
            case BUIManager::cGameMenu:            uiManagerScreenName = "GameMenu";      break;
            case BUIManager::cEndGameScreen:       uiManagerScreenName = "EndGame";       break;
            case BUIManager::cPostGameStatsScreen: uiManagerScreenName = "PostGameStats"; break;
            case BUIManager::cCampaignInfoDialog:  uiManagerScreenName = "CampaignInfo";  break;
            default:
               uiManagerScreenName = "Unknown";
               break;
         }
      }
      
      // If we're in-game, output a friendly string for the game type, and the current update number
      if (gModeManager.inModeGame())
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         long gameType = -1;
         char* gameTypeName = "Unknown";
         if (pSettings && pSettings->getLong(BGameSettings::cGameType, gameType))
         {
            switch (gameType)
            {
               case BGameSettings::cGameTypeCampaign: gameTypeName = "Campaign"; break;
               case BGameSettings::cGameTypeScenario: gameTypeName = "Scenario"; break;
               case BGameSettings::cGameTypeSkirmish: gameTypeName = "Skirmish"; break;
            }
         }
         gConsoleOutput.output(cMsgConsole, "In Game. GameType: %s", gameTypeName);
         gConsoleOutput.output(cMsgConsole, "UpdateNumber: %04u", gWorld->getUpdateNumber());
         gConsoleOutput.output(cMsgConsole, "Game Time: %04u", gWorld->getGametime());
      }
      
      gConsoleOutput.output(cMsgConsole, "Game Mode: %s", modeName);
      gConsoleOutput.output(cMsgConsole, "User Mode: %s", userModeName);
      gConsoleOutput.output(cMsgConsole, "Menu State: %s", menuStateName);
      gConsoleOutput.output(cMsgConsole, "UIManager Screen: %s", uiManagerScreenName);
   }
   
   //==============================================================================
   // getGameSettingsInfo
   //==============================================================================
   static void getGameSettingsInfo(void)
   {
      BUser* pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      float playerDifficulty = gDatabase.getDifficultyDefault();
      BGameSettings* pSettings = gDatabase.getGameSettings();
      long playerIndex = pPrimaryUser->getPlayerID();
      
      if (pSettings)
      {
         pSettings->getFloat(PSINDEX(playerIndex, BGameSettings::cPlayerDifficulty), playerDifficulty);
      }
      
      // Gather in-game specific settings if possible 
      if (pSettings && gModeManager.inModeGame())
      {
         bool  isCoop      = false;
         long  netType     = -1;
         char* netTypeName = "unknown";
         char* gameIsCoop  = "unknown";
         char* gameIsSplit = gGame.isSplitScreen() ? (gGame.isVerticalSplit() ? "vertical" : "horizontal") : "false";
         
         if (pSettings && pSettings->getLong(BGameSettings::cNetworkType, netType))
         {
            switch (netType)
            {
               case BGameSettings::cNetworkTypeLocal: netTypeName = "Local"; break;
               case BGameSettings::cNetworkTypeLan:   netTypeName = "Lan";   break;
               case BGameSettings::cNetworkTypeLive:  netTypeName = "Live";  break;
            }
         }
         if (pSettings && pSettings->getBool(BGameSettings::cCoop, isCoop))
         {
            gameIsCoop = (isCoop ? "true" : "false");
         }
         gConsoleOutput.output(cMsgConsole, "In Game. Coop: %s, SplitScreen: %s", gameIsCoop, gameIsSplit);
         gConsoleOutput.output(cMsgConsole, "Net Type: %s", netTypeName);
      }
      
      gConsoleOutput.output(cMsgConsole, "Difficulty: %4.2f", playerDifficulty);
   }
   
   //==============================================================================
   // getScenarioInfo
   //==============================================================================
   static void getScenarioInfo(void)
   {
      // If not currently in-game, current scenario displayed will be "(null)"
      float maxTerrain = 0.0f;
      const char* pScenarioName = NULL;
      if(gModeManager.inModeGame())
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         BFixedStringMaxPath mapName;
         pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
         pScenarioName = mapName.getPtr();
         maxTerrain = (float)gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();
      }
      gConsoleOutput.output(cMsgConsole, "Current Scenario: %s", pScenarioName);
      gConsoleOutput.output(cMsgConsole, "Map Size: %0.2f", maxTerrain);
   }
   
   //==============================================================================
   // getObjectivesInfo
   //==============================================================================
   static void getObjectivesInfo()
   {
      // Prepare to use the objective manager, if we are in-game and it exists
      BObjectiveManager* pObjectiveManager = (gWorld ? gWorld->getObjectiveManager() : NULL);
      if (!gModeManager.inModeGame() || !pObjectiveManager)
      {
         gConsoleOutput.output(cMsgConsole, "Unable to output objectives! (Not in-game)");
         return;
      }
      
      // Output information about each objective that exists, for any player
      for(long i = 0; i < pObjectiveManager->getNumberObjectives(); i++)
      {
         BObjective* pObj = pObjectiveManager->getObjective(i);
         if (!pObj) 
            continue;
         const WCHAR* pObjText = pObj->getDescription().getPtr();
         gConsoleOutput.output(cMsgConsole, "ObjectiveID: %d, IsDiscovered: %s, IsCompleted: %s, Score: %d, Flags: %d, Text: %S", 
               pObj->getID(), (pObj->isDiscovered() ? "true" : "false"), (pObj->isCompleted() ? "true" : "false"), 
               pObj->getScore(), pObj->getFlags(), pObjText);
      }
      gConsoleOutput.output(cMsgConsole, "Total objectives: %d", pObjectiveManager->getNumberObjectives());
   }
   
   //==============================================================================
   // getSelectedUnitInfo
   //==============================================================================
   static void getSelectedUnitInfo(void)
   {
      if (!gWorld)
         return;

      // Prepare to keep track of each piece of information, if applicable.  Data will be output together at end,
      // and some text will intentionally be displayed for null values where, for instance, units are not selected.
      const char* pVisualName = NULL;
      const char* pAnimName   = NULL;
      const char* pSquadName  = NULL;
      const char* pTargetName = NULL;
      float hpCurrent = 0.0f;
      float spCurrent = 0.0f;
      float hpMax     = 0.0f;
      float spMax     = 0.0f;
      float rotationX = 0.0f;
      float rotationY = 0.0f; 
      float rotationZ = 0.0f;
      uint  selectedIndex = 0;
      BVector pos;
      pos.set(0.0f, 0.0f, 0.0f);
      
      // Gather information about selected squads and units, in a similar manner to showObjectStats (displayStats.cpp)
      BSelectionManager* pSelectionMgr = gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
      long selectedCount = pSelectionMgr->getNumberSelectedUnits();
      BEntityID selectedID = cInvalidObjectID;
      if(selectedCount > 0)
         selectedID = pSelectionMgr->getSelected(0);
      if(selectedID != cInvalidObjectID)
      {
         BObject* pObject = gWorld->getObject(selectedID);
         if(pObject)
         {
            // Gather the first selected unit's animation/visual information
            BVisual* pVisual = pObject->getVisual();
            if(pVisual)
            {
               BProtoVisual* pProtoVisual=pVisual->getProtoVisual();
               if(pProtoVisual)
               {
                  long animType = pVisual->getAnimationType(cActionAnimationTrack);
                  pVisualName   = pProtoVisual->getName().getPtr();
                  pAnimName     = gVisualManager.getAnimName(animType);
               }
            }
            
            // Gather the physics information (true orientation etc)
            BPhysicsObject* pPhysics = pObject->getPhysicsObject();
            if (pPhysics)
            {
               BPhysicsMatrix rot;
               pPhysics->getRotation(rot);
               rotationX = rot.m[0][0];
               rotationY = rot.m[1][0];
               rotationZ = rot.m[2][0];
               pPhysics->getPosition(pos);
            } 
            else 
            {
               pos = pObject->getPosition();
            }
            
            // Gather pointers to the first selected squad and related base objects
            BSquad* pSquad = gWorld->getSquad(pObject->getParentID());
            const BProtoSquad* pProtoSquad = (pSquad ? pSquad->getProtoSquad() : NULL);
            const BProtoObject* pProtoObject = pObject->getProtoObject();
            long baseProtoID = -1;
            if (pProtoSquad)
               baseProtoID = pProtoSquad->getUnitNode(0).mUnitType;
            else if (pProtoObject)
               baseProtoID = pProtoObject->getBaseType();
            else
               baseProtoID = pObject->getProtoID();
            const BProtoObject* pBaseProtoObject = gDatabase.getGenericProtoObject(baseProtoID);
            
            // Output the costs associated with this proto squad
            const BCost* pCost = (pProtoSquad ? pProtoSquad->getCost() : NULL);
            if (pCost)
            {
               long numResources = pCost->getNumberResources();
               for (long resourceID = 0; resourceID < numResources; resourceID++)
               {
                  float cost = pCost->get(resourceID);
                  gConsoleOutput.output(cMsgConsole, "Squad Resource %d Cost: %.2f", resourceID, cost);
               }
            }
            
            // Output the first selected unit's weapons information, if any
            if (pProtoObject && pBaseProtoObject)
            {
               BTactic* pTactic = pProtoObject->getTactic();
               BTactic* pBaseTactic = pBaseProtoObject->getTactic();
               if (pTactic && pBaseTactic)
               {
                  long totalWeapons = pTactic->getNumberWeapons();
                  gConsoleOutput.output(cMsgConsole, "First Unit Total Weapons: %d", totalWeapons);
                  for (long weaponIndex = 0; weaponIndex < totalWeapons; weaponIndex++)
                  {
                     const BWeapon* pWeapon = pTactic->getWeapon(weaponIndex);
                     const BWeapon* pBaseWeapon = pBaseTactic->getWeapon(weaponIndex);
                     if (pWeapon && pBaseWeapon && pWeapon->mpStaticData)
                     {
                        const char*  pWeaponName = pWeapon->mpStaticData->mName.getPtr();
                        BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(pWeapon->mpStaticData->mWeaponType);
                        const char*  pWeaponTypeName = (pWeaponType != NULL) ? pWeaponType->getName().getPtr() : NULL;
                        gConsoleOutput.output(cMsgConsole, "Unit Weapon %d: %s, Weapon Type: %s, MinRng: %.1f(%.1f), MaxRng: %.1f(%.1f), DPS: %.2f(%.2f)", 
                              weaponIndex, pWeaponName, pWeaponTypeName, 
                              pWeapon->mMinRange, pBaseWeapon->mMinRange, 
                              pWeapon->mMaxRange, pBaseWeapon->mMaxRange, 
                              pWeapon->mDamagePerSecond, pBaseWeapon->mDamagePerSecond);
                     }
                  }
               }
            }
            
            if (pSquad)
            {
               selectedIndex = pSquad->getID().getIndex();
               hpCurrent = pSquad->getCurrentHP();
               spCurrent = pSquad->getCurrentSP();
               hpMax     = pSquad->getHPMax();
               spMax     = pSquad->getSPMax();
               const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
               if (pProtoSquad)
               {
                  pSquadName   = pProtoSquad->getName().getPtr();
                  
                  // Also get this squad's attack target name, if we have such a target
                  BAction* pAttackAction = pSquad->getActionByType(BAction::cActionTypeSquadAttack);
                  if (pAttackAction)
                  {
                     BEntity* pTargetEntity = gWorld->getEntity(pAttackAction->getTarget()->getID());
                     BSquad*  pTargetSquad  = (pTargetEntity ? pTargetEntity->getSquad() : NULL);
                     const BProtoSquad* pTargetProtoSquad = (pTargetSquad ? pTargetSquad->getProtoSquad() : NULL);
                     if (pTargetProtoSquad)
                     {
                        pTargetName = pTargetProtoSquad->getName().getPtr();
                     }
                  }
               }
            }
         }
      }
      
      gConsoleOutput.output(cMsgConsole, "First Squad: %s", pSquadName);
      gConsoleOutput.output(cMsgConsole, "First Squad HP: %4.2f", hpCurrent);
      gConsoleOutput.output(cMsgConsole, "First Squad SP: %4.2f", spCurrent);
      gConsoleOutput.output(cMsgConsole, "First Squad HP Max: %4.2f", hpMax);
      gConsoleOutput.output(cMsgConsole, "First Squad SP Max: %4.2f", spMax);
      gConsoleOutput.output(cMsgConsole, "First Squad Position: %4.4f, %4.4f, %4.4f", pos.x, pos.y, pos.z);
      gConsoleOutput.output(cMsgConsole, "First Squad Facing: %4.4f, %4.4f, %4.4f", rotationX, rotationY, rotationZ);
      gConsoleOutput.output(cMsgConsole, "First Squad Index: %d", selectedIndex);
      gConsoleOutput.output(cMsgConsole, "Squad Attack Target: %s", pTargetName);
      gConsoleOutput.output(cMsgConsole, "Total Units Selected: %d", selectedCount);
      gConsoleOutput.output(cMsgConsole, "Unit Visual: %s", pVisualName);
      gConsoleOutput.output(cMsgConsole, "Unit Anim: %s", pAnimName);
      gConsoleOutput.output(cMsgConsole, "Entity ID: %d", selectedID);
   }
   
   //==============================================================================
   // getAllPlayerSquadsInfo
   //==============================================================================
   static void getAllPlayerSquadsInfo(int player)
   {
      if (!gWorld)
         return;

      long totalSquads = 0;
      
      if (!gModeManager.inModeGame() || !gWorld)
      {
         gConsoleOutput.output(cMsgConsole, "Unable to get all player squads info! (Not in-game)");
         return;
      }
      
      // Gather and print some important info about each living, existing squad
      BEntityHandle handle=cInvalidObjectID;
      for(BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
      {
         // If any given squad isn't alive or doesn't belong to the specified player, ignore it.
         if(!pSquad->isAlive())
            continue;
         if (pSquad->getPlayerID() != (BPlayerID)player)
            continue;
         
         totalSquads++;
         const char*     pSquadName = NULL;
         const BVector   pos        = pSquad->getPosition();
         const BEntityID entityID   = pSquad->getID();
         const uint      squadIndex = entityID.getIndex();
         
         const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
         if (pProtoSquad)
         {
            pSquadName = pProtoSquad->getName().getPtr();
         }
         
         float curHP = pSquad->getCurrentHP();
         float curSP = pSquad->getCurrentSP();
         float maxHP = pSquad->getHPMax();
         float maxSP = pSquad->getSPMax();
         
         gConsoleOutput.output(cMsgConsole, "SquadID: %s, X: %0.3f, Y: %0.3f, Z: %0.3f, HP: %0.2f/%0.2f, SP: %0.2f/%0.2f, Index: %d, EntityID: %d", 
               pSquadName, pos.x, pos.y, pos.z, curHP, maxHP, curSP, maxSP, squadIndex, entityID);
      }
      gConsoleOutput.output(cMsgConsole, "Total Squads for Player %d: %d", player, totalSquads);
   }
   
   //==============================================================================
   // getPlayerInfo
   //==============================================================================
   static void getPlayerInfo(void)
   {
      BGameSettings* pSettings = gDatabase.getGameSettings();
      
      // Gather info about the primary user, their ID, Name, and Ability
      BUser*      pPrimaryUser = gUserManager.getPrimaryUser();
      long        playerID     = pPrimaryUser->getPlayerID();
      const char* pPlayerName  = pPrimaryUser->getName().getPtr();
      
      // Gather info about the primary player's coop partner, if applicable
      long           coopID      = pPrimaryUser->getCoopPlayerID();
      const BPlayer* pCoopPlayer = pPrimaryUser->getCoopPlayer();
      const char*    pCoopName   = (pCoopPlayer ? pCoopPlayer->getName().getPtr() : NULL);
      
      // If we failed to get coop partner info above, try getting via the secondary user
      if (pCoopName == NULL)
      {
         BUser* pSecondaryUser = gUserManager.getSecondaryUser();
         coopID = pSecondaryUser->getPlayerID();
         if (pSecondaryUser != NULL && pSecondaryUser->getFlagUserActive())
            pCoopName = pSecondaryUser->getName().getPtr();
      }
      
      if (gModeManager.getModeType() == BModeManager::cModePartyRoom2)
      {
         // If we are in Party Room we need to output information about the players in a 
         // unique way, since the world info for these players aren't set yet.
         BModePartyRoom2* pPartyMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);
         if (pPartyMode)
         {
            pPartyMode->outputPlayerInfo();
         }
      } 
      else 
      {
         // Output the list of players and information about them
         int totalPlayers = (gWorld ? gWorld->getNumberPlayers() : 0);
         gConsoleOutput.output(cMsgConsole, "Total Players: %d", totalPlayers);
         
         for (int i = 0; i < totalPlayers; i++)
         {
            const BPlayer* pPlayer = gWorld->getPlayer(i);
            if (pPlayer && pSettings)
            {
               BLeader*      pLeader = pPlayer->getLeader();
               BCiv*         pCiv    = pPlayer->getCiv();
               const long    civID   = (pCiv ? pCiv->getID() : -1);
               const BTeamID teamID  = pPlayer->getTeamID();
               const char*   pName   = pPlayer->getName().getPtr();
               const char*   pLName  = (pLeader ? pLeader->mName.getPtr() : NULL);
               char* playerStateName = "Unknown";
               
               // Determine what state this player is in (or leave unknown if not recognized)
               switch (pPlayer->getPlayerState())
               {
                  case BPlayer::cPlayerStatePlaying:  playerStateName = "Playing";  break;
                  case BPlayer::cPlayerStateDefeated: playerStateName = "Defeated"; break;
                  case BPlayer::cPlayerStateResigned: playerStateName = "Resigned"; break;
                  case BPlayer::cPlayerStateWon:      playerStateName = "Won";      break;
               }
               
               // Determine if this player is human-controlled or AI-controlled
               long ai = BGameSettings::cPlayerNotDefined;
               pSettings->getLong(PSINDEX(i, BGameSettings::cPlayerType), ai);
               bool isAI = (ai == BGameSettings::cPlayerHuman) ? false : true;
               
               gConsoleOutput.output(cMsgConsole, "Player ID: %d, Name: %s, Team: %d, Civ: %d, Leader: %s, IsAI: %d, PlayerState: %s", 
                     i, pName, teamID, civID, pLName, isAI, playerStateName);
            }
         }
      }
      
      // Point out which player(s) are the primary and coop players (if applicable)
      gConsoleOutput.output(cMsgConsole, "Primary Player ID: %d, Name: %s", playerID, pPlayerName);
      gConsoleOutput.output(cMsgConsole, "Coop Player ID: %d, Name: %s", coopID, pCoopName);
   }
   
   //==============================================================================
   // getPlayerResourceInfo
   //==============================================================================
   static void getPlayerResourceInfo(int player)
   {
      // Gather info about the primary user, their ID, Name, and Ability
      BUser*   pUser   = gUserManager.getUserByPlayerID(player);
      BPlayer* pPlayer = (pUser ? pUser->getPlayer() : NULL);
      
      if (!gModeManager.inModeGame())
      {
         gConsoleOutput.output(cMsgConsole, "Unable to output player resources! (Not in-game)");
         return;
      }
      if (!pUser || !pPlayer)
      {
         gConsoleOutput.output(cMsgConsole, "Unable to output player resources! (Not a user/player)");
         return;
      }
      
      long civID = pPlayer->getCivID();
      long playerStatCount = gUIGame.getNumberPlayerStats(civID);   
      for(long i=0; i<playerStatCount; i++)
      {
         const BUIGamePlayerStat* pStat = gUIGame.getPlayerStat(civID, i);
         if(pStat->mType == BUIGamePlayerStat::cTypeResource)
         {
            float currentStat = pPlayer->getResource(pStat->mID);
            float currentRate = pPlayer->getRate(pStat->mID);
            gConsoleOutput.output(cMsgConsole, "Player Resource %d: %0.2f",      i, currentStat);
            gConsoleOutput.output(cMsgConsole, "Player Resource %d Rate: %0.2f", i, currentRate);
         } 
         else if (pStat->mType == BUIGamePlayerStat::cTypePop) 
         {
            float curPop    = pPlayer->getPopCount(pStat->mID);
            float futurePop = pPlayer->getPopFuture(pStat->mID);
            float popMax    = min(pPlayer->getPopCap(pStat->mID), pPlayer->getPopMax(pStat->mID));
            gConsoleOutput.output(cMsgConsole, "Player Population Existing: %.0f", curPop);
            gConsoleOutput.output(cMsgConsole, "Player Population Incoming: %.0f", futurePop);
            gConsoleOutput.output(cMsgConsole, "Player Population Spent: %.0f", i, curPop + futurePop);
            gConsoleOutput.output(cMsgConsole, "Player Population Max: %.0f", i, popMax);
         }
      }
   }
   
   //==============================================================================
   // getProtoObjectList
   //==============================================================================
   static void getProtoObjectList(void)
   {
      long total = gDatabase.getNumberProtoObjects();
      for(long i=0; i<total; i++)
      {
         const char* pName = gDatabase.getProtoObjectName(i);
         gConsoleOutput.output(cMsgConsole, "ObjectID: %i: %s", i, pName);
      }
      gConsoleOutput.output(cMsgConsole, "Total Objects: %d", total);
   }
   
   //==============================================================================
   // getProtoTechList
   //==============================================================================
   static void getProtoTechList(void)
   {
      long total = gDatabase.getNumberProtoTechs();
      for(long i=0; i<total; i++)
      {
         const char* pName = gDatabase.getProtoTechName(i);
         gConsoleOutput.output(cMsgConsole, "TechID: %i: %s", i, pName);
      }
      gConsoleOutput.output(cMsgConsole, "Total Techs: %d", total);
   }
   
   //==============================================================================
   // getProtoSquadList
   //==============================================================================
   static void getProtoSquadList(void)
   {
      long total = gDatabase.getNumberProtoSquads();
      for(long i=0; i<total; i++)
      {
         const char* pName = gDatabase.getProtoSquadName(i);
         gConsoleOutput.output(cMsgConsole, "SquadID: %i: %s", i, pName);
      }
      gConsoleOutput.output(cMsgConsole, "Total Squads: %d", total);
   }
   
   //==============================================================================
   // getProtoPowerList
   //==============================================================================
   static void getProtoPowerList(void)
   {
      long total = gDatabase.getNumberProtoPowers();
      for(long i=0; i<total; i++)
      {
         const char* pName = gDatabase.getProtoPowerByID(i)->getName().getPtr();
         gConsoleOutput.output(cMsgConsole, "PowerID: %i: %s", i, pName);
      }
      gConsoleOutput.output(cMsgConsole, "Total Powers: %d", total);
   }
   
   //==============================================================================
   // getScenarioList
   //==============================================================================
   void getScenarioList()
   {
      long total = 0;
      BXMLReader reader;
      if(!reader.load(cDirData, "scenariodescriptions.xml"))
      {
         gConsoleOutput.output(cMsgConsole, "Unable to load ScenarioDescriptions.xml!");
         return;
      }
      
      BXMLNode rootNode(reader.getRootNode());
      long nodeCount=rootNode.getNumberChildren();
      for(long i=0; i<nodeCount; i++)
      {
         BXMLNode node(rootNode.getChild(i));
         const BPackedString name(node.getName());
         if(name=="ScenarioInfo")
         {
            BSimString typeStr, fileStr;
            const bool hasType = node.getAttribValue("Type", &typeStr);
            const bool hasFile = node.getAttribValue("File", &fileStr);
            if (!hasType || !hasFile)
            {
               gConsoleOutput.output(cMsgConsole, "Scenario did not have both type and file attributes!");
               continue;
            }
            int nameStringID=0;
            node.getAttribValueAsInt("NameStringID", nameStringID);
            BUString displayName;
            if (nameStringID!=0)
               displayName=gDatabase.getLocStringFromID(nameStringID);
            gConsoleOutput.output(cMsgConsole, "Scenario: %s, DisplayName: %S", fileStr.getPtr(), displayName.getPtr());
            total++;
         }
      }
      
      gConsoleOutput.output(cMsgConsole, "Total Scenarios: %d", total);
   }
   
   //==============================================================================
   // getTerrainHeight
   //==============================================================================
   static void getTerrainHeight(float x, float z)
   {
      if (!gModeManager.inModeGame())
      {
         gConsoleOutput.output(cMsgConsole, "Must be in game mode to get terrain height!");
         return;
      }
      BVector pos;
      pos.x = x;
      pos.y = 0.0f;
      pos.z = z;
      gTerrainSimRep.getHeightRaycast(pos, pos.y, true);
      gConsoleOutput.output(cMsgConsole, "Terrain height is: %0.2f", pos.y);
   }
   
   //==============================================================================
   // getCameraInfo
   //==============================================================================
   static void getCameraInfo()
   {
      if (!gModeManager.inModeGame())
      {
         gConsoleOutput.output(cMsgConsole, "Must be in game mode to get camera info!");
         return;
      }
      BUser*   pUser   = gUserManager.getPrimaryUser();
      BCamera* pCamera = (pUser ? pUser->getCamera() : NULL);
      if (!pCamera)
      {
         gConsoleOutput.output(cMsgConsole, "Camera Position could not be retrieved!");
         return;
      }
      BVector pos = pCamera->getCameraLoc();
      BVector dir = pCamera->getCameraDir();
      bool overTerrain = pUser->getFlagHoverPointOverTerrain();
      gConsoleOutput.output(cMsgConsole, "Is hover point over terrain: %s", (overTerrain ? "true" : "false"));
      gConsoleOutput.output(cMsgConsole, "Camera Position X: %0.3f, Y: %0.3f, Z: %0.3f", pos.x, pos.y, pos.z);
      gConsoleOutput.output(cMsgConsole, "Camera Direction X: %0.3f, Y: %0.3f, Z: %0.3f", dir.x, dir.y, dir.z);
   }
   
   //==============================================================================
   // rotateCamera
   //==============================================================================
   static void rotateCamera(float radians)
   {
      BUser*   pUser   = gUserManager.getPrimaryUser();
      BCamera* pCamera = (pUser ? pUser->getCamera() : NULL);
      if (!pCamera)
      {
         gConsoleOutput.output(cMsgConsole, "Camera rotation could not be performed!");
         return;
      }
      pCamera->yawWorld(radians);
      pUser->setFlagUpdateHoverPoint(true);
      gConsoleOutput.output(cMsgConsole, "Camera rotation performed!");
   }
   
   //==============================================================================
   // setCameraFacing
   //==============================================================================
   static void setCameraFacing(float x, float y, float z, float zoom)
   {
      BUser*   pUser   = gUserManager.getPrimaryUser();
      BCamera* pCamera = (pUser ? pUser->getCamera() : NULL);
      if (!pCamera)
      {
         gConsoleOutput.output(cMsgConsole, "Camera Position could not be set!");
         return;
      }
      
      // Set the camera hover point to be at the specified location.
      // The camera angle and direction remain unchanged by this hook.
      BVector pos;
      pos.x = x;
      pos.y = y;
      pos.z = z;
      pUser->setCameraHoverPoint(pos);
      // Also set the camera values the old way, to be thorough (in case of behavior regression).
      pos -= (pCamera->getCameraDir() * zoom);
      pCamera->setCameraLoc(pos);
      // Either way, ensure the camera is allowed to get there instantly upon the next update.
      pUser->setFlagTeleportCamera(true);
      gConsoleOutput.output(cMsgConsole, "Camera facing set!");
   }
   
   //==============================================================================
   // setTechActivation
   //==============================================================================
   static void setTechActivation(const char* techName, bool isActive)
   {
      BUser*       pUser   = gUserManager.getPrimaryUser();
      BPlayer*     pPlayer = (pUser ? pUser->getPlayer() : NULL);
      BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
      BProtoTechID techID  = cInvalidTechID;
      BSelectionManager* pSelectionMgr = gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
      
      if (!pTech || !pSelectionMgr)
      {
         gConsoleOutput.output(cMsgConsole, "Could not set tech activation status! (NULL)");
         return;
      }
      
      long numProtoTechs = gDatabase.getNumberProtoTechs();
      for(long i=0; i<numProtoTechs; i++)
      {
         const char* pTechName = gDatabase.getProtoTechName(i);
         if (!stricmp(pTechName, techName))
         {
            techID = i;
            break;
         }
      }
      if (techID == cInvalidTechID)
      {
         gConsoleOutput.output(cMsgConsole, "Could not set tech activation status! (Tech not found)");
         return;
      }
      
      // If a unit/building is selected, we'll try to de/activate this tech via the specified 
      // unit/building in addition to trying to generically activating it.
      BEntityID selectedID = cInvalidObjectID;
      if(pSelectionMgr->getNumberSelectedUnits() > 0)
         selectedID = pSelectionMgr->getSelected(0);
      
      if (isActive)
      {
         pTech->activateTech(techID, cInvalidObjectID, true);
         if (selectedID != cInvalidObjectID)
         {
            pTech->activateTech(techID, selectedID, true);
         }
         gConsoleOutput.output(cMsgConsole, "Set tech to be activated!");
      }
      else
      {
         pTech->deactivateTech(techID, cInvalidObjectID);
         if (selectedID != cInvalidObjectID)
         {
            pTech->deactivateTech(techID, selectedID);
         }
         gConsoleOutput.output(cMsgConsole, "Set tech to be deactivated!");
      }
   }
   
   //==============================================================================
   // keyboardInput
   //==============================================================================
   static void keyboardInput(int key, int keyModifiers)
   {
      bool shift = (keyModifiers & 1) > 0;
      bool ctrl  = (keyModifiers & 2) > 0;
      bool alt   = (keyModifiers & 4) > 0;
      BKeyboard* kb = gInputSystem.getKeyboard();
      
      if (!kb)
         return;
      
      // Simulate turning on any of shift, control, and/or alt, that were requested, and store the old 
      // (actual) state for later restoration.  This uses simulateKeyActive prior to handling the 
      // actual key, to convince handleInput that the special keys are already being held up/down.
      shift = kb->simulateKeyActive(cKeyShift, shift);
      ctrl  = kb->simulateKeyActive(cKeyCtrl, ctrl);
      alt   = kb->simulateKeyActive(cKeyAlt, alt);
      
      // Simulating turning on the keyboard key requested, then immediately turn it back off.
      // This uses handleInput to properly trigger debug cheats and such.
      BInputEventDetail detail;
      detail.mAnalog = 1.0f;
      gInputSystem.handleInput(0, cInputEventControlStart, cFirstKey+key, detail);
      gInputSystem.handleInput(0, cInputEventControlStop, cFirstKey+key, detail);
      
      // Restore previous (real) state for any shift, control, and alt 
      kb->simulateKeyActive(cKeyShift, shift);
      kb->simulateKeyActive(cKeyCtrl, ctrl);
      kb->simulateKeyActive(cKeyAlt, alt);
      
      gConsoleOutput.output(cMsgConsole, "Keyboard input simulated!");
   }
   
   //==============================================================================
   // startHavokDebugger
   //==============================================================================
   static void startHavokDebugger()
   {
      if (gWorld && gWorld->getPhysicsWorld())
         gWorld->getPhysicsWorld()->startHavokDebugger();
   }
#endif

//==============================================================================
// physicsTest
//==============================================================================
static void physicsTestName(long numObjects, const char* protoName)
{
   if (!gWorld)
      return;

   gSimHelperThread.sync();

   BScopedPhysicsWrite writeLock(gWorld->getPhysicsWorld());

   long id = gDatabase.getProtoObject(protoName);
   if (id < 0)
      return;

   float maxTerrain = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();


   for (long i=0; i < numObjects; i++)
   {
      BVector loc;
      loc.x = getRandRangeFloat(cUnsyncedRand, maxTerrain * 0.25f, maxTerrain * 0.75f);
      loc.z = getRandRangeFloat(cUnsyncedRand, maxTerrain * 0.25f, maxTerrain * 0.75f);
      loc.y = getRandRangeFloat(cUnsyncedRand, 100.0f, 200.0f);

      BEntity* pEnt = gWorld->getEntity(gWorld->createEntity(id, false, 0, loc, cZAxisVector, cXAxisVector, true, false));
      if (!pEnt)
         return;

      BVector point, impulse;
      point.x = getRandRangeFloat(cUnsyncedRand, -1.0f, 1.0f);
      point.z = getRandRangeFloat(cUnsyncedRand, -1.0f, 1.0f);
      point.y = getRandRangeFloat(cUnsyncedRand, -1.0f, 1.0f);

      impulse = point;
      impulse.inverse();
      impulse.scale(100.0f);
      
      //pEnt->startPhysics();
      if (pEnt->getPhysicsObject())
         pEnt->getPhysicsObject()->applyPointImpulse(impulse, point);
   }
}

static void physicsTest(long numObjects)
{
   physicsTestName(numObjects, "physics_box_01");
}


//=============================================================================
// playCinematic()
//=============================================================================
static void playCinematic(long index)
{
   if(gWorld)
   {
      gWorld->playCinematic(index);
   }
}

//=============================================================================
// grantPower()
//=============================================================================
static void grantPower(const char* szPowerName)
{
   if (!gWorld)
      return;

   if (!szPowerName)
      return;
   long protoPowerID = gDatabase.getProtoPowerIDByName(szPowerName);
   if (protoPowerID < 0)
      return;
   const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   long playerID = pUser->getPlayerID();

   BPowerCommand *c = (BPowerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPower);
   if (!c)
      return;

   c->setSenders(1, &playerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cPlayer);
   c->setType(BPowerCommand::cTypeGrantPower);
   c->setProtoPowerID(protoPowerID);
   c->setNumUses(1);
   gWorld->getCommandManager()->addCommandToExecute(c);
}

#ifndef BUILD_FINAL
//=============================================================================
// setWorkRateScalar
//=============================================================================
static void setWorkRateScalar(const char* pSquadName, float rate)
{
   if (!gWorld)
      return;

   long total = 0;
   float oldWorkRate = 0.0f;
   BEntityHandle handle=cInvalidObjectID;
   
   for(BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
   {
      const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
      if (!pProtoSquad)
         continue;
      const char* pName = pProtoSquad->getName().getPtr();
      if (!stricmp(pName, pSquadName))
      {
         // Set work rate scalars for each of this squad's children (units)
         int numChildren = pSquad->getNumberChildren();
         for (int i=0; i<numChildren; i++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
            {
               oldWorkRate = pUnit->getWorkRateScalar();
               pUnit->setWorkRateScalar(rate);
            }
         }
         total++;
      }
   }
   
   if (total > 0)
   {
      gConsoleOutput.output(cMsgConsole, "Work rate scalars changed to %0.3f (at least one was %0.3f)", rate, oldWorkRate);
   }
   gConsoleOutput.output(cMsgConsole, "Total squad work rate scalars set: %d", total);
}

//=============================================================================
// setShields
//=============================================================================
static void setShields(const char* pSquadName, bool hasShield)
{
   if (!gWorld)
      return;

   long total = 0;
   BEntityHandle handle=cInvalidObjectID;
   for(BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
   {
      const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
      if (!pProtoSquad)
         continue;
      const char* pName = pProtoSquad->getName().getPtr();
      if (!stricmp(pName, pSquadName))
      {
         // Set each of this squad's child objects (units) shield values to 0 or full depending on hasShield
         int numChildren = pSquad->getNumberChildren();
         for (int i=0; i<numChildren; i++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
            {
               if (hasShield)
                  pUnit->setShieldpoints(pUnit->getSPMax());
               else
                  pUnit->setShieldpoints(0.0f);
            }
         }
         total++;
      }
   }
   gConsoleOutput.output(cMsgConsole, "Total shields [un]set: %d", total);
}

//=============================================================================
// destroySquads
//=============================================================================
static void destroySquads(const char* pDestroySquadName, bool bKillImmediately)
{
   if (!gWorld)
      return;

   long totalDestroyed = 0;
   BEntityHandle handle=cInvalidObjectID;
   for(BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
   {
      const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
      if (!pProtoSquad)
         continue;
      const char* pName = pProtoSquad->getName().getPtr();
      if (!stricmp(pName, pDestroySquadName))
      {
         pSquad->kill(bKillImmediately);
         totalDestroyed++;
      }
   }
   gConsoleOutput.output(cMsgConsole, "Total squads destroyed: %d", totalDestroyed);
}
#endif

#ifndef BUILD_FINAL
//=============================================================================
// destroySelection
//=============================================================================
static void destroySelection(bool bKillImmediately)
{
   if (!gWorld)
      return;

   BSelectionManager* pSelectionMgr = gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   BEntityID selectedID = pSelectionMgr->getSelected(0);
   BObject* pObject = (selectedID != cInvalidObjectID ? gWorld->getObject(selectedID) : NULL);
   BSquad*  pSquad  = (pObject != NULL ? gWorld->getSquad(pObject->getParentID()) : NULL);
   if (pSquad == NULL)
   {
      gConsoleOutput.output(cMsgConsole, "No selected squad destroyed.");
      return;
   }
   pSquad->kill(bKillImmediately);
   gConsoleOutput.output(cMsgConsole, "One selected squad destroyed.");
}
#endif

#ifndef BUILD_FINAL
//=============================================================================
// destroyAtCursor
//=============================================================================
static void destroyAtCursor(long val)
{
   if (!gWorld)
      return;

   BUser *pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (pUser)
      pUser->destroyAtCursor();
}
#endif

#ifndef BUILD_FINAL
//=============================================================================
// destroyUnitAtCursor
//=============================================================================
static void destroyUnitAtCursor(long val)
{
   if (!gWorld)
      return;

   BUser *pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (pUser)
      pUser->destroyUnitAtCursor();
}
#endif

#ifndef BUILD_FINAL
//=============================================================================
// setHitpoints
//=============================================================================
static void setHitpoints(float hitpoints)
{
   if (!gWorld)
      return;

   BSelectionManager* pSelectionMgr = gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   BEntityID selectedID = pSelectionMgr->getSelected(0);
   BObject* pObject = (selectedID != cInvalidObjectID ? gWorld->getObject(selectedID) : NULL);
   BSquad*  pSquad  = (pObject != NULL ? gWorld->getSquad(pObject->getParentID()) : NULL);
   if (pSquad == NULL)
   {
      gConsoleOutput.output(cMsgConsole, "No selected squad to set hitpoints for.");
      return;
   }

   if (pSquad->getNumberChildren() == 0)
      return;

   // Distribute the hitpoints to all the child units
   float unitHitpoints = hitpoints / pSquad->getNumberChildren();
   for (uint i = 0; i < pSquad->getNumberChildren(); i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
         pUnit->setHitpoints(unitHitpoints);
   }
   gConsoleOutput.output(cMsgConsole, "setHitpoints:  Each unit set to %f hitpoints.", hitpoints, unitHitpoints);
}
#endif

#ifndef BUILD_FINAL
//=============================================================================
// createAttachmentAtCursor
//=============================================================================
static void createAttachmentAtCursor(const char* pObjectName)
{
   if (!gWorld)
      return;

   long protoObjectID=gDatabase.getProtoObject(pObjectName);
   if(protoObjectID==-1)
      return;
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if(!pUser)
      return;
   BUnit* pUnit = gWorld->getUnit(pUser->getHoverObject());
   if(!pUnit)
      return;
   pUnit->addAttachment(protoObjectID);
}


//=============================================================================
// setHighlightAtCursor
//=============================================================================
static void setHighlightAtCursor(float value)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if(!pUser)
      return;

   BObject* pObject = gWorld->getObject(pUser->getHoverObject());
   if (!pObject)
      return;

   pObject->setHighlightIntensity(value);
}

#endif

#ifndef BUILD_FINAL
//=============================================================================
// removeAttachmentsAtCursor
//=============================================================================
static void removeAttachmentsAtCursor()
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if(!pUser)
      return;
   BUnit* pUnit = gWorld->getUnit(pUser->getHoverObject());
   if(!pUnit)
      return;
   pUnit->removeAllAttachments();
}
#endif

//=============================================================================
// renderPathingQuad
//=============================================================================
static void renderPathingQuad(long val)
{  
   if(val == -1)
      gGame.toggleRenderPathingQuad();
   else
      gGame.setRenderPathingQuad(val);
}

//=============================================================================
// pauseGame
//=============================================================================
static void pauseGame()
{  
   BModeGame* pMode=gModeManager.getModeGame();
   if(pMode)
   {
      bool newState = !pMode->getPaused();
      pMode->setPaused(newState);
      gConsoleOutput.output(cMsgConsole, "Game paused state set to: %s", (newState ? "true" : "false"));
   }
}

//=============================================================================
// setFixedUpdate
//=============================================================================
static void setFixedUpdate(float time)
{  
   BModeGame* pMode=gModeManager.getModeGame();
   if(pMode)
      pMode->setFixedUpdate(time*0.001f);
}

//=============================================================================
// displayStats
//=============================================================================
static void displayStats(int mode)
{
   long controlType = cKeyF2;
   BInputEventDetail detail;

   if (mode == 0) controlType = cKeyF2;
   else if (mode == 1) controlType = cKeyF4;
   else if (mode == 2) controlType = cKeyF5;

   BDisplayStats::handleInput(0, cInputEventControlStart, controlType, detail);
}

//=============================================================================
// clearFixedUpdate
//=============================================================================
static void clearFixedUpdate()
{  
   BModeGame* pMode=gModeManager.getModeGame();
   if(pMode)
      pMode->clearFixedUpdate();
}

//=============================================================================
// setFixedUpdateFPS
//=============================================================================
static void setFixedUpdateFPS(float fps)
{  
   BModeGame* pMode=gModeManager.getModeGame();
   if((pMode) && (fps > 0.0f))
      pMode->setFixedUpdate(1.0f / fps);
}

//=============================================================================
//Lets you hard kill the party session at any point (for testing)
//=============================================================================
static void killParty()
{
   if (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession())
   {
      gLiveSystem->getMPSession()->getPartySession()->shutdown();
   }
}

//=============================================================================
//
//=============================================================================
static void mpHost()
{  
   if (gLiveSystem)
   {
      gLiveSystem->setMPTestOptions( BLiveSystem::cLiveSystemTestHostOnly, TRUE);
      gConsoleOutput.output(cMsgConsole, "MP Host only mode set");
   }
}


//=============================================================================
//
//=============================================================================
static void mpLBReset()
{ 
   //Eric's hack function for clearing all difficulties of a campaign leaderboard
   /*
   if (gLiveSystem)
   {
      for (DWORD diff=1;diff<5;diff++)
      {
         DWORD index = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeCampaignBestScore,0,false,diff,2, 0, 0);
         DWORD ret = XUserResetStatsViewAllUsers(index,NULL);
      }
   } 
   */
}

//=============================================================================
// 
//=============================================================================
static void mpAutoStart()
{  
   if (gLiveSystem)
   {
      gLiveSystem->setMPTestOptions( BLiveSystem::cLiveSystemTestAutoStart, TRUE);
      gConsoleOutput.output(cMsgConsole, "MP Auto Start mode set");
   }
}

//=============================================================================
// 
//=============================================================================
static void mpAutoEnd()
{  
   if (gLiveSystem)
   {
      gLiveSystem->setMPTestOptions( BLiveSystem::cLiveSystemTestAutoEnd, TRUE);
      gConsoleOutput.output(cMsgConsole, "MP Auto End game mode set");
   }
}

#ifndef BUILD_FINAL

//=============================================================================
// 
//=============================================================================
static void testLeaderBoardQuery()
{  
   if (gLiveSystem)
   {
      gLiveSystem->testLeaderBoardQuery();
      gConsoleOutput.output(cMsgConsole, "testLeaderBoardQuery sent");
   }
}

//=============================================================================
// 
//=============================================================================
static void testLeaderBoardRead()
{  
   if (gLiveSystem)
   {
      BSimString textResult;
      gLiveSystem->testLeaderBoardRead( &textResult );
      gConsoleOutput.output(cMsgConsole, "%s", textResult.getPtr());
   }
}


//=============================================================================
// contentTest
//=============================================================================
static void contentTest()
{
   BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);

   gUserManager.showDeviceSelector(user);

}

#if 0
//=============================================================================
// loadAchievements
//=============================================================================
static void loadAchievements()
{
   gAchievementManager.loadAchievements();

   BUser * pUser = gUserManager.getPrimaryUser();
   if (pUser && pUser->isSignedIn())
   {
      BUserAchievementList* pAchievements = pUser->getAchievementList();
      if (pAchievements)
      {

         // hack to get the deviceid
         HANDLE hEventComplete = CreateEvent( NULL, FALSE, FALSE, NULL );
         XOVERLAPPED xov = {0};
         xov.hEvent = hEventComplete;

         DWORD dwRet;
         XCONTENTDEVICEID deviceID = XCONTENTDEVICE_ANY;
         ULARGE_INTEGER iBytesRequested = {0};

         dwRet = XShowDeviceSelectorUI( pUser->getPort(), // User to receive input from
            XCONTENTTYPE_SAVEDGAME,   // List only save game devices
            0,                       // No special flags
            iBytesRequested,         // Size of the device data struct
            &deviceID,            // Return selected device information
            &xov );

         if( XGetOverlappedResult( &xov, NULL, TRUE ) != ERROR_SUCCESS )
         {
            CloseHandle(hEventComplete);
            return;
         }

         // start the save, it will finish on the update loop
         // pAchievements->saveAchievements(pUser->getPort(), deviceID );
         pAchievements->loadAchievements(pUser->getPort(), deviceID );
      }
   }
}
#endif

//=============================================================================
// getAchievements
//=============================================================================
static void getAchievements(void)
{
   const BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);
   gAchievementManager.retrieveAchievements(user);
}

//=============================================================================
// grantAchievement
//=============================================================================
static void grantAchievement(int i)
{
   /*
#ifndef TITLEID_HALO_WARS_ALPHA
   if (!gConfig.isDefined(cConfigAlpha))
   {
      BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);
      //gAchievementManager.grantAchievement(user, (DWORD)i);
      gAchievementManager.updateAchievement(user, ACHIEVEMENT_ACH_TEST_01);
   }
#endif
   */
}

//=============================================================================
// submitStats
//=============================================================================
static void submitStats()
{
   gStatsManager.submit();
}

//=============================================================================
// 
//=============================================================================
static void setRank(int rank, int level, bool serverUpdated)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser == NULL)
      return;

   if (pUser->isSignedIn() == false)
      return;

   BUserProfile* pProfile = pUser->getProfile();
   if (pProfile == NULL)
      return;

   pProfile->mMatchmakingRank.mRank = rank;
   pProfile->mMatchmakingRank.mLevel = level;
   pProfile->mMatchmakingRank.mServerUpdated = (serverUpdated ? 1 : 0);
}

//=============================================================================
// 
//=============================================================================
static void setMatchmakingWins(int wins, int score)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser == NULL)
      return;

   if (pUser->isSignedIn() == false)
      return;

   BUserProfile* pProfile = pUser->getProfile();
   if (pProfile == NULL)
      return;

   pProfile->mMatchmakingRank.mWon = wins;
   pProfile->mMatchmakingScore = score;
   pProfile->updateMatchmakingRank(false, score);
}

//=============================================================================
// debugging method to test adding another user into the multiplayer party
// this is for testing until the UI is ready
//=============================================================================
static void addSecondaryUserToParty()
{
   BUser* pUser = gUserManager.getUser(BUserManager::cSecondaryUser);
   if (pUser == NULL || !pUser->isSignedIn())
      return;

   if (gLiveSystem == NULL)
      return;

   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (pMPSession == NULL)
      return;

   // insure that we don't have a game session
   if (pMPSession->getSession() != NULL)
      return;

   BPartySession* pPartySession = pMPSession->getPartySession();
   if (pPartySession == NULL)
      return;

   BSession* pSession = pPartySession->getSession();
   if (pSession == NULL)
      return;

   bool retval = pSession->joinUser(pUser->getPort(), pUser->getXuid(), pUser->getName());
   BASSERTM(retval == true, "Failed to join secondary user to the current party session");
}

//=============================================================================
// lspAuthTest
//=============================================================================
static void lspAuthTest()
{
   gLSPManager.checkAuth();
}

//=============================================================================
// showWait
//=============================================================================
static void showWait(bool bShow)
{
   BUIGlobals* uiglobal = gGame.getUIGlobals();
   if (!uiglobal)
      return;

   uiglobal->setWaitDialogVisible(bShow);

}

//=============================================================================
//=============================================================================
static void enableTicker(bool enable)
{
   if (enable)
   {
      if (!BUITicker::isEnabled())
         BUIScreen::install();
   }
   else
   {
      if (BUITicker::isEnabled())
         BUIScreen::remove();
   }
}


//=============================================================================
// timerTest
//=============================================================================
static void timerTest()
{
   // int timerID = gTimerManager.createTimer(true, 0, 2*60*1000);
   int timerID = gTimerManager.createTimer(false, 2*60*1000, 0);

   BUIWidgets * pWidgets = gUIManager->getWidgetUI();
   if (!pWidgets)
      return;

   pWidgets->setTimerVisible(true, timerID);

}

//=============================================================================
// OfferTest
// Put the offer ID in user.cfg under tag OfferID=x, then call this command to bring
//   up blade to that particular offering on partnernet
// Useful for testing private (non-visible) offers to check them out before pushing them to Live
// See Eric for questions
//=============================================================================
static void offerTest()
{
   BSimString offerIDStr = "";
   gConfig.get("OfferID", offerIDStr);
   offerIDStr += " ";
  
   BSimString buffer;
   const char* pOfferbuffer = offerIDStr.asANSI(buffer);
   if (!pOfferbuffer)
   {      
      DWORD dwResult = XShowMarketplaceUI(0, XSHOWMARKETPLACEUI_ENTRYPOINT_CONTENTLIST, 0, (DWORD)-1);
      gConsoleOutput.output(cMsgConsole,"Hex OfferID string in config - just showing top level [%d]", dwResult);
      return;
   }

   ULONGLONG offerID = _strtoui64(pOfferbuffer, NULL , 16);
   DWORD dwResult = XShowMarketplaceUI(0, XSHOWMARKETPLACEUI_ENTRYPOINT_CONTENTITEM, offerID, (DWORD)-1);
   if (dwResult==ERROR_SUCCESS)
   {
      gConsoleOutput.output(cMsgConsole,"Displaying %s", offerIDStr.getPtr());
   }
   else
   {
      gConsoleOutput.output(cMsgConsole,"Error %d displaying offer %s", dwResult, offerIDStr.getPtr());
   }
}

//=============================================================================
// help
//=============================================================================
static void help(const char* pStr)
{
   BString str(pStr ? pStr : "");
   str.toLower();
   
   BXSSyscallEntryArray& syscalls = gConsole.getConsoleSyscalls()->getSyscalls();
   BDynamicArray<BString> helpStrings;
   for (uint i = 0; i < syscalls.size(); i++)
   {
      const BXSSyscallEntry* pEntry = syscalls[i];
      
      if ((pEntry) && (pEntry->getHelp()) && (strlen(pEntry->getHelp())))
      {
         if (strstr(pEntry->getHelp(), " xs") == NULL)
         {
            BString helpText(pEntry->getHelp());
            helpText.toLower();
            
            if (!str.length() || helpText.contains(str))
               helpStrings.pushBack(pEntry->getHelp());
         }
      }
   }
   
   helpStrings.sort();
   
   for (uint i = 0; i < helpStrings.getSize(); i++)
      gConsole.output(helpStrings[i]);
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// causeOOS
//==============================================================================
static void causeOOS( int i )
{
   if (!gWorld)
      return;

   if( i == 666 )
   {
      if( gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() == 1 )
      {
         BUnit* pUnit = gWorld->getNextUnit( (BEntityHandle)cInvalidObjectID );
         if( pUnit )
         {
            BVector pos;
            pos.set( 1.0f, -1.0f, 0.2f );
            pUnit->setPosition( pos );
         }
      }
   }
   else if( i == 999 )
   {      
      BSyncManager::getInstance()->outOfSync();
   }
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// winGame
//==============================================================================
static void winGame()
{
   if (!gWorld)
      return;

   gUIManager->setScenarioResult( 4 );
   //gUIManager->showNonGameUI( BUIManager::cEndGameScreen, gUserManager.getPrimaryUser() );

   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   BPlayer* player = user->getPlayer();
   if (!player)
      return;
    
   if(gScoreManager.getBaseScore(player->getID()) == 0)
      gScoreManager.reportBogusKill(player->getID());

   BTeam* team = player->getTeam();
   if (!team)
      return;

   for (long i=1; i<gWorld->getNumberPlayers(); i++)
   {
      player = gWorld->getPlayer(i);
      if (!player)
         continue;

      if (player->getTeamID() == team->getID())
      {
         // you win
         player->setPlayerState(BPlayer::cPlayerStateWon);
      }
      else
      {
         // you lose
         player->setPlayerState(BPlayer::cPlayerStateDefeated);
      }
   }


}
//==============================================================================
// loseGame
//==============================================================================
static void loseGame()
{
   if (!gWorld)
      return;

   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   BPlayer* player = user->getPlayer();
   if (!player)
      return;
    
   BTeam* team = player->getTeam();
   if (!team)
      return;

   for (long i=1; i<gWorld->getNumberPlayers(); i++)
   {
      player = gWorld->getPlayer(i);
      if (!player)
         continue;

      if (player->getTeamID() == team->getID())
      {
         // you lose
         player->setPlayerState(BPlayer::cPlayerStateDefeated);
      }
      else
      {
         // you win
         player->setPlayerState(BPlayer::cPlayerStateWon);
      }
   }

}

//==============================================================================
// setAllSkullsCollected
//==============================================================================
static void setAllSkullsCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideSkullsCollected(user,0xFFFFFFFF);
}

//==============================================================================
// setNoSkullsCollected
//==============================================================================
static void setNoSkullsCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideSkullsCollected(user, 0x0000);
}

//==============================================================================
// setAllBlackBoxesCollected
//==============================================================================
static void setAllBlackBoxesCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_BlackBox, true);
}

//==============================================================================
// setNoBlackBoxesCollected
//==============================================================================
static void setNoBlackBoxesCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_BlackBox, false);
}

//==============================================================================
// setAllSkirmishGamesCollected
//==============================================================================
static void setAllSkirmishGamesCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Leader, true);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Map, true);
}

//==============================================================================
// setNoSkirmishGamesCollected
//==============================================================================
static void setNoSkirmishGamesCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Leader, false);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Map, false);
}

//==============================================================================
// setAllTimeLineEventsCollected
//==============================================================================
static void setAllTimeLineEventsCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Leader, true);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Map, true);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_BlackBox, true);
}

//==============================================================================
// setNoTimeLineEventsCollected
//==============================================================================
static void setNoTimeLineEventsCollected()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Leader, false);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_Map, false);
   gCollectiblesManager.overrideTimeLineEventsCollected(user, cTLET_BlackBox, false);
}

//=============================================================================
// ungrantAllFakeAchievements
//=============================================================================
static void ungrantAllFakeAchievements()
{
   BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if(!pUser)
      return;

   BUserAchievementList * pAchievementList = pUser->getAchievementList();
   if (pAchievementList)
      pAchievementList->initialize();

   gUserProfileManager.writeProfile(pUser);

   gConsoleOutput.output(cMsgConsole, "Fake achievements reset for the primary user");

}

//==============================================================================
// enterCampaignScore
//==============================================================================
static void enterCampaignScore( int scenarioID, int mode, int difficulty, int score, int grade, int time, int partime)
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   if( user->getProfile() )
      user->getProfile()->mCampaignProgress.enterScore(scenarioID, mode, difficulty, user->getPlayerID(), score, grade, time, partime);
}

//==============================================================================
// dumpCampaignProgress
//==============================================================================
static void dumpCampaignProgress()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);

   if( user->getProfile() )
      user->getProfile()->mCampaignProgress.dumpInfo();
}

//==============================================================================
// setTickerSpeed
//==============================================================================
static void setTickerSpeed(int speed)
{
   if (speed < 0) 
      speed = BUITicker::cDefaultScrollSpeed;

   BUITicker::setSpeed((float)speed);
   gConsoleOutput.output(cMsgConsole, "Ticker speed set to %d", speed);
}

#endif

#ifndef BUILD_FINAL
//==============================================================================
// setSoundScapeScale
//==============================================================================
static void setSoundScapeScale( const char* input )
{
   float i = 0;
   sscanf_s(input, "%f", &i);
   
   if(i > 0)
   {
      gSoundManager.setSoundScapeScale(i);
   }
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// setListenerFactor
//==============================================================================
static void setListenerFactor(const char* input)
{
   if(gWorld)
   {           
      float offset;
      sscanf_s(input, "%f", &offset);
      gWorld->getWorldSoundManager()->setListenerFactor(offset);
   }   
}

//==============================================================================
// setRearAdjustmentFactor
//==============================================================================
static void setRearAdjustmentFactor(const char* input)
{
   if(gWorld)
   {           
      float offset;
      sscanf_s(input, "%f", &offset);
      gWorld->getWorldSoundManager()->setRearAdjustmentFactor(offset);
   }   
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// playSoundCue
//==============================================================================
static void playSoundCue(const char* input)
{
   gSoundManager.playCue(input);
}

//==============================================================================
// reloadSoundBanks
//==============================================================================
static void reloadSoundBanks()
{
   //-- Reset the world sound manager, and unload the extended sound banks
   if(gWorld && gWorld->getWorldSoundManager())
      gWorld->getWorldSoundManager()->reset();

   //-- Get the list of remaining banks currently loaded.
   BDynamicArray<BString> loadedBanks;
   loadedBanks = gSoundManager.getLoadedSoundBanks();
   gSoundManager.resetSoundManager();

   //-- Reload those banks
   for(uint i=0; i < loadedBanks.getSize(); i++)
   {
      gSoundManager.loadSoundBank(loadedBanks[i], false);
   }
}

//==============================================================================
// loadSoundBank
//==============================================================================
static void loadSoundBank(const char* input)
{
   if (gSoundManager.loadSoundBank(input, false) != AK_INVALID_BANK_ID)
   {
      gConsoleOutput.output(cMsgConsole, "Loaded sound bank: %s", input);
   }
}

//==============================================================================
// unloadSoundBank
//==============================================================================
static void unloadSoundBank(const char* input)
{
   if (gSoundManager.unloadSoundBank(input, false))
   {
      gConsoleOutput.output(cMsgConsole, "Unloaded sound bank: %s", input);
   }
}

//==============================================================================
// playSoundList
//==============================================================================
static void playSoundList(const char* input)
{
   gSoundManager.startSoundPlaybackFile(input);
}

//==============================================================================
// toggleSoundCueOutput
//==============================================================================
static void toggleSoundCueOutput()
{
   bool newval = gSoundManager.toggleSoundCueOutput();
   gConsoleOutput.output(cMsgConsole, "Sound cue output is now %s.", newval ? "on" : "off");
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// cycleConsoleRenderMode
//==============================================================================
static void cycleConsoleRenderMode(bool dir)
{
   if (gConfig.isDefined(cConfigConsoleRenderEnable))
   {
      long mode;
      gConfig.get(cConfigConsoleRenderMode, &mode);

      if (dir)
         mode++;
      else
         mode--;

      if (mode >= cChannelMax)
         mode = 0;
      else if (mode < 0)
         mode = cChannelMax - 1;

      gConfig.set(cConfigConsoleRenderMode, mode);
   }
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// revealMap
//==============================================================================
static void revealMap(bool reveal)
{
   if (!gWorld)
      return;

   const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   long playerID = pUser->getPlayerID();
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &playerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeRevealMap);
      pCommand->setData(reveal ? 1 : 0);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}
#endif

//==============================================================================
// pathingCam
//==============================================================================
static void pathingCam(void)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (!pUser)
      return;

   BCamera* pCamera = pUser->getCamera();

   static float z = 1.0f;
   BVector cameraDirection(0,-1.0f,0);   
   BVector cameraUp(0,0,1);
   BVector cameraPos(pCamera->getCameraLoc().x,100.0f,pCamera->getCameraLoc().z);
   pCamera->setCameraUp(cameraUp);
   pCamera->setCameraDir(cameraDirection);
   pCamera->setCameraLoc(cameraPos);
   pCamera->calcCameraRight();

   //-- disable fog
   BFogParams fogParams;
   fogParams.mZColor = BVec3(0,0,0); 
   fogParams.mZStart = 1000000.0f;
   fogParams.mZDensity = 0.0f;   

   fogParams.mPlanarColor = BVec3(0,0,0);
   fogParams.mPlanarStart = 0.0f;
   fogParams.mPlanarDensity = 0.0f;

   gSimSceneLightManager.setFogParams(cLCTerrain, fogParams);
   gSimSceneLightManager.setFogParams(cLCUnits, fogParams);
}

#ifndef BUILD_FINAL
//==============================================================================
// lookAtEntity
//==============================================================================
static void lookAtEntity(int entityID)
{
   if (!gWorld)
      return;

   BEntity* pEntity = gWorld->getEntity(BEntityID(entityID), true);
   if (pEntity == NULL)
      return;

   // Position camera
   BVector position = pEntity->getPosition();
   setCameraFacing(position.x, position.y, position.z, 80.0f);

   // Select the entity
   if ((gUserManager.getPrimaryUser() == NULL) || (gUserManager.getPrimaryUser()->getSelectionManager() == NULL))
      return;
   BSelectionManager* pSelectionManager = gUserManager.getPrimaryUser()->getSelectionManager();

   pSelectionManager->clearSelections();
   if (pEntity->getClassType() == BEntity::cClassTypeSquad)
      pSelectionManager->selectSquad(pEntity->getID());
   else if (pEntity->getClassType() == BEntity::cClassTypeUnit)
      pSelectionManager->selectUnit(pEntity->getID());
}

//==============================================================================
//==============================================================================
static BCameraEffectData gCamEffectData;
static void beginCamEffect(float duration, float saturation, float blurFactor, float blurX, float blurY, bool radialBlur)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (!pUser)
      return;

   BCamera* pCamera = pUser->getCamera();
   if (!pCamera)
      return;

   static float radialBlurScale = 0.2f;
   static float radialBlurMax = 0.1f;

   float endTime = duration;

   gCamEffectData.mColorTransformFactorTable.clear();
   gCamEffectData.mColorTransformFactorTable.addKeyValue(0.0f, 0.0f);
   gCamEffectData.mColorTransformFactorTable.addKeyValue(endTime, saturation);

   gCamEffectData.mColorTransformRTable.clear();
   gCamEffectData.mColorTransformRTable.addKeyValue(0.0f, BVector(1.0f, 0.0f, 0.0f));
   gCamEffectData.mColorTransformRTable.addKeyValue(endTime, BVector(0.213f, 0.715f, 0.072f));
   gCamEffectData.mColorTransformGTable.clear();
   gCamEffectData.mColorTransformGTable.addKeyValue(0.0f, BVector(0.0f, 1.0f, 0.0f));
   gCamEffectData.mColorTransformGTable.addKeyValue(endTime, BVector(0.213f, 0.715f, 0.072f));
   gCamEffectData.mColorTransformBTable.clear();
   gCamEffectData.mColorTransformBTable.addKeyValue(0.0f, BVector(0.0f, 0.0f, 1.0f));
   gCamEffectData.mColorTransformBTable.addKeyValue(endTime, BVector(0.213f, 0.715f, 0.072f));

   gCamEffectData.mBlurFactorTable.clear();
   gCamEffectData.mBlurFactorTable.addKeyValue(0.0f, 0.0f);
   gCamEffectData.mBlurFactorTable.addKeyValue(endTime, blurFactor);

   gCamEffectData.mBlurXYTable.clear();
   gCamEffectData.mBlurXYTable.addKeyValue(0.0f, BVector2(0.0f, 0.0f));
   gCamEffectData.mBlurXYTable.addKeyValue(endTime, BVector2(blurX, blurY));

   gCamEffectData.mRadialBlurFactorTable.clear();
   gCamEffectData.mRadialBlurFactorTable.addKeyValue(0.0f, BVector2(radialBlurScale, radialBlurMax));

   gCamEffectData.mRadialBlur = radialBlur;

   pCamera->beginCameraEffect(gWorld->getSubGametime(), &gCamEffectData, NULL, 0);
}

//==============================================================================
//==============================================================================
static void clearCamEffect(void)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (!pUser)
      return;

   BCamera* pCamera = pUser->getCamera();
   if (!pCamera)
      return;

   pCamera->clearCameraEffect(gWorld->getSubGametime(), 0, 0);
}


//==============================================================================
//==============================================================================
void patherTest(long lPathType, long lIterations, long lNoBackPath)
{
   if(!gWorld)
      return;

   //Actually run a path.
   static BPath tempPath;
   tempPath.reset();

   setRandSeed(cRMRand, 1234);

   // Init Pather Stats
   gPather.initStats();

   BVector start;
   BVector goal;
   float dist;
   float angle;
   float fRange = cMaximumLowLevelDist;
   long lPathRange = BPather::cShortRange;
   if (lPathType != 0)
   {
      lPathRange = BPather::cLongRange;
      fRange = 500.0f;
   }

   DWORD startTime = timeGetTime();
   
   if (lNoBackPath)
      gPather.setBackPathOption(BPather::cBackPathOff);

   for(long i=0; i<lIterations; i++)
   {
      start.x=getRandRangeFloat(cRMRand, gTerrain.getMin().x, gTerrain.getMax().x);
      start.z=getRandRangeFloat(cRMRand, gTerrain.getMin().z, gTerrain.getMax().z);

      if (lPathType != 0)
      {
         goal.x=getRandRangeFloat(cRMRand, gTerrain.getMin().x, gTerrain.getMax().x);
         goal.z=getRandRangeFloat(cRMRand, gTerrain.getMin().z, gTerrain.getMax().z);
      }
      else
      {
         dist=getRandRangeFloat(cRMRand, 5.0f, fRange);
         angle=getRandRangeFloat(cRMRand, 0.0f, cTwoPi);
         goal.x=start.x+float(sin(angle))*dist;
         goal.z=start.z+float(cos(angle))*dist;
      }

      gPather.findPath(&gObsManager, start, goal, 1.0f, 0.3f, NULL, &tempPath, false, false, false, -1, lPathRange, BPather::cLandPath);
   }
   if (lNoBackPath)
      gPather.setBackPathOption(BPather::cBackPathOn);

   
   DWORD endTime = timeGetTime();
   gConsoleOutput.output(cMsgConsole, "pathertest: %d ms", endTime-startTime);
   
   gPather.dumpStats();
}

//==============================================================================
//==============================================================================
void upgradeLevel()
{
   if(!gWorld || gLiveSystem->isMultiplayerGameActive())
      return;
   BSelectionManager* pSelectionMgr = gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   int selectedCount = pSelectionMgr->getNumberSelectedSquads();
   for (int i=0; i<selectedCount; i++)
   {
      BSquad* pSquad = gWorld->getSquad(pSelectionMgr->getSelectedSquad(i));
      if (pSquad)
      {
         pSquad->upgradeLevel(pSquad->getLevel()+1, true);
         gConsoleOutput.output(cMsgConsole, "New squad level: %d", pSquad->getLevel());
      }
   }
}

#endif

//==============================================================================
//==============================================================================
void saveGame(const char* pFileName)
{
   gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "Saving Game...");
   gSaveGame.saveGame(pFileName);
}

//==============================================================================
//==============================================================================
void loadGame(const char* pFileName)
{
   gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "Loading Game...");
   BUser* user = gUserManager.getPrimaryUser();
   BASSERT(user);
   user->setFlagUserActive(true);

   gSaveGame.loadGame(pFileName);
   if (!gModeManager.inModeGame())
      gSaveGame.doLoadGame();
}

//==============================================================================
//==============================================================================
void uiSelectAll(int port, bool global, bool reverse, bool autoGlobal, bool autoCycle)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiSelectAll(global, reverse, autoGlobal, autoCycle, false, false);
}

//==============================================================================
//==============================================================================
void uiCycleGroup(int port, bool reverse, bool wrap)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiCycleGroup(reverse, wrap);
}

//==============================================================================
//==============================================================================
void uiCycleTarget(int port, bool reverse)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiCycleTarget(reverse, true);
}

//==============================================================================
//==============================================================================
void uiSelectTarget(int port)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiSelectTarget();
}

//==============================================================================
//==============================================================================
void uiCancel(int port)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiCancel();
}

//==============================================================================
//==============================================================================
void uiModifierAction(int port, bool on)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiModifierAction(on);
}

//==============================================================================
//==============================================================================
void uiModifierSpeed(int port, bool on)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiModifierSpeed(on);
}

//==============================================================================
//==============================================================================
void uiFlare(int port, float x, float y)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiFlare(x, y);
}

//==============================================================================
//==============================================================================
void uiMenu(int port)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiMenu();
}

//==============================================================================
//==============================================================================
void uiObjectives(int port)
{
   if (!gWorld)
      return;

   BUser* pUser = gUserManager.getUserByPort(port);
   if (pUser)
      pUser->uiObjectives();
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void saveToCurrentProfile()
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if(pUser == NULL)
      return;

   if( pUser->isSignedIn() == false)
      return;

   BUserProfile* pProfile = pUser->getProfile();
   if(pProfile == NULL)
      return;

   gUserProfileManager.writeProfile(pUser);
}

//==============================================================================
//==============================================================================
void resetCampaignProgress()
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if(pUser == NULL)
      return;

   if( pUser->isSignedIn() == false)
      return;

   BUserProfile* pProfile = pUser->getProfile();
   if(pProfile == NULL)
      return;

   pProfile->mCampaignProgress.clear();
   gUserProfileManager.writeProfile(pUser);
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   if( pCampaign )
      pCampaign->setCurrentNodeID(0);
   gConsoleOutput.output(cMsgConsole, "Campaign progress reset for the primary user");
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void pauseTriggers(bool v)
{
   gTriggerManager.setFlagPaused(v);
}

//==============================================================================
//==============================================================================
void stopSelected(void)
{
   // jce [9/25/2008] -- This is not done through a command since it's just a debug hack and there is
   // no existing stop command.  As such, I am just preventing it in multiplayer since if you did it by
   // accident or something you'd go OOS.
   if(gLiveSystem && gLiveSystem->isMultiplayerGameActive())
      return;
   
   // Need a world.
   if(!gWorld)
      return;
      
   // Just use the primary user since there's no obvious way to know which user to use.
   BUser *primaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if(!primaryUser)
      return;
   
   // Get selection manager.
   BSelectionManager *selectionManager = primaryUser->getSelectionManager();
   if(!selectionManager)
      return;
   
   // Number of squads selected.
   long selectedSquads = selectionManager->getNumberSelectedSquads();
   
   // Walk them.
   for(long i=0; i<selectedSquads; i++)
   {
      // Get squad.
      BSquad *squad = gWorld->getSquad(selectionManager->getSelectedSquad(i));
      if(!squad)
         continue;
      
      // Get parent platoon.
      BPlatoon *platoon = squad->getParentPlatoon();

      // If we have a platoon, tell it to stop, which should also stop us.
      if(platoon)
         platoon->removeAllOrders();

      // Stop squad -- probably duplicating work, but let's make sure since this is just a debug command.
      squad->removeAllOrders();
   }

   // jce [9/25/2008] -- Probably overkill, but hit the units too.  Maybe this will stop buildings from working etc properly :)
   // Number of units selected.
   long selectedUnits = selectionManager->getNumberSelectedUnits();
   
   // Walk them.
   for(long i=0; i<selectedUnits; i++)
   {
      // Get squad.
      BUnit *unit = gWorld->getUnit(selectionManager->getSelected(i));
      if(!unit)
         continue;
      
      // Stop unit.
      unit->removeOpps();
  }
      
}


//==============================================================================
//==============================================================================
void setPlayableBounds(float minX, float minZ, float maxX, float maxZ)
{
   // Sanity check.
   if(!gWorld)
   {
      gConsoleOutput.output(cMsgConsole, "No world.");
      return;
   }
   
   // Set it.
   gWorld->setSimBounds(minX, minZ, maxX, maxZ);

   // Let the user know we did it.   
   gConsoleOutput.output(cMsgConsole, "Set playableable bounds to (%0.1f, %0.1f) to (%0.1f, %0.1f)", minX, minZ, maxX, maxZ);
}

void rebuildLRP()
{
   // Sanity check.
   if(!gWorld)
   {
      gConsoleOutput.output(cMsgConsole, "No world.");
      return;
   }
   
   DWORD startTime = timeGetTime();
   gPather.buildPathingQuads(&gObsManager);
   DWORD delta = timeGetTime() - startTime;
   gConsoleOutput.output(cMsgConsole, "rebuildLRP %d ms", delta);
}

void subUpdate(const char* pType)
{
   if (stricmp(pType, "off") == 0)
      gWorld->setSubUpdating(false, false, false, false);
   else if (stricmp(pType, "on") == 0)
      gWorld->setSubUpdating(true, false, false, false);
   else if (stricmp(pType, "alternate") == 0)
      gWorld->setSubUpdating(true, true, false, false);
   else if (stricmp(pType, "dynamic") == 0)
      gWorld->setSubUpdating(true, true, true, false);
   else if (stricmp(pType, "decoupled") == 0)
      gWorld->setSubUpdating(true, false, false, true);
}

#endif

//==============================================================================
// addGameFuncs
//==============================================================================
bool addGameFuncs(BXSSyscallModule *sm)
{
   if (sm == NULL)
      return(false);

   XS_SYSCALL("exit", BXSVariableEntry::cVoidVariable, &exitGame, 0)
      XS_BOOL_PARM(false)
      XS_HELP("exit([confirm])")

   XS_SYSCALL("loadScenario", BXSVariableEntry::cVoidVariable, &loadScenario, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(-1)
      XS_HELP("loadScenario(scenarioName, gameType)")

   XS_SYSCALL("reloadScenario", BXSVariableEntry::cVoidVariable, &reloadScenario, 0)
      XS_BOOL_PARM(false)
      XS_HELP("reloadScenario(leaveCamera)")
   
   XS_SYSCALL("loadVisual", BXSVariableEntry::cVoidVariable, &loadVisual, 0)
      XS_STRING_PARM("")
      XS_HELP("loadVisual(visualName)")

   XS_SYSCALL("reloadStrings", BXSVariableEntry::cVoidVariable, &reloadStrings, 0)
      XS_HELP("reloadStrings")

   XS_SYSCALL("resetCameraDefaults", BXSVariableEntry::cVoidVariable, &resetCameraDefaults, 0)
      XS_HELP("resetCameraDefaults")
      
   #ifndef BUILD_FINAL
   XS_SYSCALL("setWorkRateScalar", BXSVariableEntry::cVoidVariable, &setWorkRateScalar, 0)
      XS_STRING_PARM("")
      XS_FLOAT_PARM(0.0f)
      XS_HELP("setWorkRateScalar(squadName, rate) : set work rate scalar for all existing squads of given name")
   
   XS_SYSCALL("setShields", BXSVariableEntry::cVoidVariable, &setShields, 0)
      XS_STRING_PARM("")
      XS_BOOL_PARM(true)
      XS_HELP("setShields(squadName, setShieldEnabled) : (un)enable shields for all existing squads of given name")
   
   XS_SYSCALL("destroySquads", BXSVariableEntry::cVoidVariable, &destroySquads, 0)
      XS_STRING_PARM("")
      XS_BOOL_PARM(true)
      XS_HELP("destroySquads(name, bKillImmediate) : destroys all existing squads of the given name")
   
   XS_SYSCALL("destroySelection", BXSVariableEntry::cVoidVariable, &destroySelection, 0)
      XS_BOOL_PARM(true)
      XS_HELP("destroySelection() : destroys the first currently selected squad")
   
   XS_SYSCALL("destroyAtCursor", BXSVariableEntry::cVoidVariable, &destroyAtCursor, 0)
      XS_HELP("destroyAtCursor() : destroys the selection under the current cursor location")

   XS_SYSCALL("destroyUnitAtCursor", BXSVariableEntry::cVoidVariable, &destroyUnitAtCursor, 0)
      XS_HELP("destroyUnitAtCursor() : destroys the selection under the current cursor location")

   XS_SYSCALL("setHitpoints", BXSVariableEntry::cVoidVariable, &setHitpoints, 0)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("setHitpoints() : sets squad hitpoints by dividing them among its units")

   XS_SYSCALL("test", BXSVariableEntry::cVoidVariable, &test, 0)
      XS_HELP("test()")
      
   XS_SYSCALL("createSquadAtCursor", BXSVariableEntry::cVoidVariable, &createSquadAtCursor, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(1)
      XS_INTEGER_PARM(1)
      XS_HELP("createSquadAtCursor( typeName, playerID, num ) : create a squad for the current type at the cursor location")

   XS_SYSCALL("createObjectAtCursor", BXSVariableEntry::cVoidVariable, &createObjectAtCursor, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(1)
      XS_INTEGER_PARM(1)
      XS_HELP("createObjectAtCursor( typeName, playerID, num ) : create an object of the current type at the cursor location")

   XS_SYSCALL("launchTriggerScript", BXSVariableEntry::cVoidVariable, &launchTriggerScript, 0)
      XS_STRING_PARM("")
      XS_HELP("launchTriggerScript( scriptName ) : load the trigger script and activate it.")

   XS_SYSCALL("terminateTriggerScript", BXSVariableEntry::cVoidVariable, &terminateTriggerScript, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("terminateTriggerScript( triggerScriptID ) : Terminate the trigger script.")

   XS_SYSCALL("createAttachmentAtCursor", BXSVariableEntry::cVoidVariable, &createAttachmentAtCursor, 0)
      XS_STRING_PARM("")
      XS_HELP("createAttachmentAtCursor( typeName ) : create an attachment for the object being pointed at")

   XS_SYSCALL("setHighlightAtCursor", BXSVariableEntry::cVoidVariable, &setHighlightAtCursor, 0)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("setHighlightAtCursor( intensity ) : set the highlight intensity for the object being pointed at")

   XS_SYSCALL("removeAttachmentsAtCursor", BXSVariableEntry::cVoidVariable, &removeAttachmentsAtCursor, 0)
      XS_HELP("removeAttachmentsAtCursor() : removes all attachments from the object being pointed at")
   #endif

   XS_SYSCALL("listProtoObjects", BXSVariableEntry::cVoidVariable, &listProtoObjects, 0)
      XS_HELP("listProtoObjects() : list the names of all proto objects");

   XS_SYSCALL("killParty", BXSVariableEntry::cVoidVariable, &killParty, 0)
      XS_HELP("killParty() : Shuts down the party session");
   
   XS_SYSCALL("mpHost", BXSVariableEntry::cVoidVariable, &mpHost, 0)
      XS_HELP("mpHost() : Forces the live matchmaking to always try to host");
   
   XS_SYSCALL("mpAutoStart", BXSVariableEntry::cVoidVariable, &mpAutoStart, 0)
      XS_HELP("mpAutoStart() : Automatically looks for a Live multiplayer game from the main menu");

   XS_SYSCALL("mpAutoEnd", BXSVariableEntry::cVoidVariable, &mpAutoEnd, 0)
      XS_HELP("mpAutoEnd() : Automatically resigns/returns to main menu from multiplayer games");

   XS_SYSCALL("mpLBReset", BXSVariableEntry::cVoidVariable, &mpLBReset, 0)
      XS_HELP("mpLBReset() : Resets leaderboard entries - partnernet only");

#ifndef BUILD_FINAL
   XS_SYSCALL("setMenuState", BXSVariableEntry::cVoidVariable, &setMenuState, 0)
      XS_STRING_PARM("")
      XS_HELP("setMenuState(stateName)")
   
   XS_SYSCALL("setObjectiveState", BXSVariableEntry::cVoidVariable, &setObjectiveState, 0)
      XS_INTEGER_PARM(1)
      XS_BOOL_PARM(false)
      XS_BOOL_PARM(false)
      XS_HELP("setObjectiveState(objectiveID, setDiscovered, setCompleted)")
   
   XS_SYSCALL("setSettingString", BXSVariableEntry::cVoidVariable, &setSettingString, 0)
      XS_STRING_PARM("")
      XS_STRING_PARM("")
      XS_HELP("setSettingString(settingName, newValue)")
   
   XS_SYSCALL("setSettingInt", BXSVariableEntry::cVoidVariable, &setSettingInt, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(0)
      XS_HELP("setSettingInt(settingName, newValue)")
   
   XS_SYSCALL("setSettingPlayer", BXSVariableEntry::cVoidVariable, &setSettingPlayer, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(0)
      XS_INTEGER_PARM(0)
      XS_HELP("setSettingPlayer(settingName, playerNumber, newValue)")
   
   XS_SYSCALL("getBuildInfo", BXSVariableEntry::cVoidVariable, &getBuildInfo, 0)
      XS_HELP("getBuildInfo() : retrieve info about the build, such as changelist number")
   
   XS_SYSCALL("getRenderedFrameInfo", BXSVariableEntry::cVoidVariable, &getRenderedFrameInfo, 0)
      XS_HELP("getRenderedFrameInfo(): get information about the last rendered frame (or splitscreen viewport)")      
   
   XS_SYSCALL("getGameStateInfo", BXSVariableEntry::cVoidVariable, &getGameStateInfo, 0)
      XS_HELP("getGameStateInfo() : retrieve info about the game state, such as UI screen/mode")
   
   XS_SYSCALL("getGameSettingsInfo", BXSVariableEntry::cVoidVariable, &getGameSettingsInfo, 0)
      XS_HELP("getGameSettingsInfo() : retrieve info about the current game settings, such as difficulty")
   
   XS_SYSCALL("getScenarioInfo", BXSVariableEntry::cVoidVariable, &getScenarioInfo, 0)
      XS_HELP("getScenarioInfo() : retrieve info about the current scenario, such as scenario name")
   
   XS_SYSCALL("getObjectivesInfo", BXSVariableEntry::cVoidVariable, &getObjectivesInfo, 0)
      XS_HELP("getObjectivesInfo() : retrieve info about the current objectives for all players")
   
   XS_SYSCALL("getSelectedUnitInfo", BXSVariableEntry::cVoidVariable, &getSelectedUnitInfo, 0)
      XS_HELP("getSelectedUnitInfo() : retrieve info about the selected unit(s), such as unit name")
   
   XS_SYSCALL("getAllPlayerSquadsInfo", BXSVariableEntry::cVoidVariable, &getAllPlayerSquadsInfo, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("getAllPlayerSquadsInfo(player) : retrieve info about all the specified player's existing squads")
   
   XS_SYSCALL("getPlayerInfo", BXSVariableEntry::cVoidVariable, &getPlayerInfo, 0)
      XS_HELP("getPlayerInfo() : retrieve info about the primary player(s)")
   
   XS_SYSCALL("getPlayerResourceInfo", BXSVariableEntry::cVoidVariable, &getPlayerResourceInfo, 0)
      XS_INTEGER_PARM(1)
      XS_HELP("getPlayerResourceInfo(player) : retrieve info about the specified player's current resources in-game")
   
   XS_SYSCALL("getProtoObjectList", BXSVariableEntry::cVoidVariable, &getProtoObjectList, 0)
      XS_HELP("getProtoObjectList() : retrieve list of all objects and IDs")
   
   XS_SYSCALL("getProtoTechList", BXSVariableEntry::cVoidVariable, &getProtoTechList, 0)
      XS_HELP("getProtoTechList() : retrieve list of all techs and IDs")
   
   XS_SYSCALL("getProtoSquadList", BXSVariableEntry::cVoidVariable, &getProtoSquadList, 0)
      XS_HELP("getProtoSquadList() : retrieve list of all squads and IDs")
   
   XS_SYSCALL("getProtoPowerList", BXSVariableEntry::cVoidVariable, &getProtoPowerList, 0)
      XS_HELP("getProtoPowerList() : retrieve list of all powers and IDs")
   
   XS_SYSCALL("getScenarioList", BXSVariableEntry::cVoidVariable, &getScenarioList, 0)
      XS_HELP("getScenarioList() : retrieve list of all scenarios")
   
   XS_SYSCALL("getTerrainHeight", BXSVariableEntry::cVoidVariable, &getTerrainHeight, 0)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("getTerrainHeight(x, z) : retrieve the y coordinate (height) of the terrain at (x, z)")
   
   XS_SYSCALL("getCameraInfo", BXSVariableEntry::cVoidVariable, &getCameraInfo, 0)
      XS_HELP("getCameraInfo() : retrieve info about the camera such as position and direction")
   
   XS_SYSCALL("rotateCamera", BXSVariableEntry::cVoidVariable, &rotateCamera, 0)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("rotateCamera(radians) : rotate the world camera the specified amount")
   
   XS_SYSCALL("setCameraFacing", BXSVariableEntry::cVoidVariable, &setCameraFacing, 0)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(50.0f)
      XS_HELP("setCameraFacing(x, y, z, zoom) : move the camera to point at the specified coordinates, from a specified zoom level")
   
   XS_SYSCALL("setTechActivation", BXSVariableEntry::cVoidVariable, &setTechActivation, 0)
      XS_STRING_PARM("")
      XS_BOOL_PARM(true)
      XS_HELP("setTechActivation(techName, isActive) : set the specified tech to activated/deactivated")
   
   XS_SYSCALL("keyboardInput", BXSVariableEntry::cVoidVariable, &keyboardInput, 0)
      XS_INTEGER_PARM(0)
      XS_INTEGER_PARM(0)
      XS_HELP("keyboardInput(key, keyModifiers) : simulates keyboard input to activate debug commands")
   
   XS_SYSCALL("startHavokDebugger", BXSVariableEntry::cVoidVariable, &startHavokDebugger, 0)
      XS_HELP("startHavokDebugger() : starts the internal havok visual debugger server")

   XS_SYSCALL("testLBQ", BXSVariableEntry::cVoidVariable, &testLeaderBoardQuery, 0)
     XS_HELP("testLBQ() : Fires off a query to the leaderboard system")

   XS_SYSCALL("testLBR", BXSVariableEntry::cVoidVariable, &testLeaderBoardRead, 0)
     XS_HELP("testLBR() : Checks on the statys ofr a leaderboard system query");

#endif

   XS_SYSCALL("physicsTest", BXSVariableEntry::cVoidVariable, &physicsTest, 0)
      XS_INTEGER_PARM(100)
      XS_HELP("physicsTest( number ) : test the physics system")

   XS_SYSCALL("physicsTest2", BXSVariableEntry::cVoidVariable, &physicsTestName, 0)
      XS_INTEGER_PARM(100)
      XS_STRING_PARM("")
      XS_HELP("physicsTest2( number, name ) : test the physics system")

   XS_SYSCALL("playCinematic", BXSVariableEntry::cVoidVariable, &playCinematic, 0)
      XS_INTEGER_PARM(100)
      XS_HELP("playCinematic( number ) : play the given cinematic if it exists")

   XS_SYSCALL("grantPower", BXSVariableEntry::cVoidVariable, &grantPower, 0)
      XS_STRING_PARM("")
      XS_HELP("grantPower(szPowerName) : Grants the specified power.")

   XS_SYSCALL("configSetInt", BXSVariableEntry::cVoidVariable, &configSetInt, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(-1)
      XS_HELP("configSetInt( name, value) : sets an integer config value at runtime.")

   XS_SYSCALL("configSetFloat", BXSVariableEntry::cVoidVariable, &configSetFloat, 0)
      XS_STRING_PARM("")
      XS_FLOAT_PARM(-1)
      XS_HELP("configSetFloat( name, value) : sets a float config value at runtime.")

   XS_SYSCALL("configToggle", BXSVariableEntry::cVoidVariable, &configToggle, 0)
      XS_STRING_PARM("")
      XS_HELP("configToggle( name ) : toggles the config on and off.")

   XS_SYSCALL("getConfigList", BXSVariableEntry::cVoidVariable, &getConfigList, 0)
      XS_STRING_PARM("")
      XS_HELP("getConfigList() : display a list of all current configs.")

   XS_SYSCALL("renderPathingQuad", BXSVariableEntry::cVoidVariable, &renderPathingQuad, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("renderPathingQuad( number ) : Set or toggle the debugging lines for pathing quads")

   XS_SYSCALL("pauseGame", BXSVariableEntry::cVoidVariable, &pauseGame, 0)
      XS_HELP("pauseGame")

   XS_SYSCALL("setFixedUpdate", BXSVariableEntry::cVoidVariable, &setFixedUpdate, 0)
      XS_FLOAT_PARM(1.0f/30.0f)
      XS_HELP("setFixedUpdate(time)")

   XS_SYSCALL("displayStats", BXSVariableEntry::cVoidVariable, &displayStats, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("displayStats( mode ) : Turn on stats display - mode: 0 = sim, 1 = render, 2 = timings")
   
   XS_SYSCALL("clearFixedUpdate", BXSVariableEntry::cVoidVariable, &clearFixedUpdate, 0)
      XS_HELP("clearFixedUpdate")

   XS_SYSCALL("setFixedUpdateFPS", BXSVariableEntry::cVoidVariable, &setFixedUpdateFPS, 0)
      XS_FLOAT_PARM(30.0f)
   XS_HELP("setFixedUpdateFPS(FPS)")
            
#ifndef BUILD_FINAL      
   XS_SYSCALL( "oos", BXSVariableEntry::cVoidVariable, &causeOOS, 0 )
      XS_INTEGER_PARM( 0 )
      XS_HELP( "oos( 666 ): cause an out of synch for testing purposes" )
      
   XS_SYSCALL("help", BXSVariableEntry::cVoidVariable, &help, 0)
      XS_STRING_PARM("")
      XS_HELP("help(): displays this help")      

   XS_SYSCALL("winGame", BXSVariableEntry::cVoidVariable, &winGame, 0)
      XS_HELP("winGame(): the primary user and his team wins the game.")      

   XS_SYSCALL("loseGame", BXSVariableEntry::cVoidVariable, &loseGame, 0)
      XS_HELP("loseGame(): the primary user and his team loses the game.")      

   XS_SYSCALL("setAllSkullsCollected", BXSVariableEntry::cVoidVariable, &setAllSkullsCollected, 0)
      XS_HELP("setAllSkullsCollected(): set all skulls as collected for the primary user.")      

   XS_SYSCALL("setNoSkullsCollected", BXSVariableEntry::cVoidVariable, &setNoSkullsCollected, 0)
      XS_HELP("setNoSkullsCollected(): set all skulls as uncollected for the primary user.")      

   XS_SYSCALL("setAllBlackBoxesCollected", BXSVariableEntry::cVoidVariable, &setAllBlackBoxesCollected, 0)
      XS_HELP("setAllBlackBoxesCollected(): set all black boxes as collected for the primary user.")      

   XS_SYSCALL("setNoBlackBoxesCollected", BXSVariableEntry::cVoidVariable, &setNoBlackBoxesCollected, 0)
      XS_HELP("setNoBlackBoxesCollected(): set all black boxes as uncollected for the primary user.")      

   XS_SYSCALL("setAllSkirmishGamesCollected", BXSVariableEntry::cVoidVariable, &setAllSkirmishGamesCollected, 0)
      XS_HELP("setAllSkirmishGamesCollected(): set all skirmish maps and leaders as collected for the primary user.")      

   XS_SYSCALL("setNoSkirmishGamesCollected", BXSVariableEntry::cVoidVariable, &setNoSkirmishGamesCollected, 0)
      XS_HELP("setNoSkirmishGamesCollected(): set all skirmish maps and leaders as uncollected for the primary user.")      

   XS_SYSCALL("setAllTimeLineEventsCollected", BXSVariableEntry::cVoidVariable, &setAllTimeLineEventsCollected, 0)
      XS_HELP("setAllTimeLineEventsCollected(): set all timeline events (black boxes, skirmish maps and leaders) as collected for the primary user.")      

   XS_SYSCALL("setNoTimeLineEventsCollected", BXSVariableEntry::cVoidVariable, &setNoTimeLineEventsCollected, 0)
      XS_HELP("setNoTimeLineEventsCollected(): set all timeline events (black boxes, skirmish maps and leaders) as uncollected for the primary user.")      

   XS_SYSCALL("ungrantAllFakeAchievements", BXSVariableEntry::cVoidVariable, &ungrantAllFakeAchievements, 0)
      XS_HELP("ungrantAllFakeAchievements(): reset all achievements as ungranted for the primary user.")      

   XS_SYSCALL("enterCampaignScore", BXSVariableEntry::cVoidVariable, &enterCampaignScore, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("enterCampaignScore(int scenarioID, int mode, int difficulty, int score, int grade, int time, int partime): fake your campaign record.")      

   XS_SYSCALL("dumpCampaignProgress", BXSVariableEntry::cVoidVariable, &dumpCampaignProgress, 0)
      XS_HELP("dumpCampaignProgress(): dump out the campaign progress information to the console.")      

   XS_SYSCALL("setTickerSpeed", BXSVariableEntry::cVoidVariable, &setTickerSpeed, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("setTickerSpeed(): Change the scroll speed of the ticker. Set to -1 for default.  Default is around 75.")      

   XS_SYSCALL("setSoundScapeScale", BXSVariableEntry::cVoidVariable, &setSoundScapeScale, 0)
      XS_STRING_PARM("")
      XS_HELP("setSoundScapeScale(int val): adjusts the scale of the sound scape.")      

   XS_SYSCALL("setListenerFactor", BXSVariableEntry::cVoidVariable, &setListenerFactor, 0)
      XS_STRING_PARM("")
      XS_HELP("setListenerFactor(float x): adjusts the offset of the listener relative to the camera.")      

   XS_SYSCALL("setRearAdjustmentFactor", BXSVariableEntry::cVoidVariable, &setRearAdjustmentFactor, 0)
      XS_STRING_PARM("")
      XS_HELP("setRearAdjustmentFactor(float x): adjusts the offset of the listener relative to the camera.")      
   
   XS_SYSCALL("playSoundCue", BXSVariableEntry::cVoidVariable, &playSoundCue, 0)
      XS_STRING_PARM("")
      XS_HELP("playSoundCue(cue):")      

   XS_SYSCALL("reloadSoundBanks", BXSVariableEntry::cVoidVariable, &reloadSoundBanks, 0)
      XS_HELP("reloadSoundBanks()")      

   XS_SYSCALL("loadSoundBank", BXSVariableEntry::cVoidVariable, &loadSoundBank, 0)
      XS_STRING_PARM("")
      XS_HELP("loadSoundBank(bank)")      

   XS_SYSCALL("unloadSoundBank", BXSVariableEntry::cVoidVariable, &unloadSoundBank, 0)
      XS_STRING_PARM("")
      XS_HELP("unloadSoundBank(bank)")      

   XS_SYSCALL("playSoundList", BXSVariableEntry::cVoidVariable, &playSoundList, 0)
      XS_STRING_PARM("")
      XS_HELP("playSoundList(file)")      

   XS_SYSCALL("toggleSoundCueOutput", BXSVariableEntry::cVoidVariable, &toggleSoundCueOutput, 0)
   XS_HELP("toggleSoundCueOutput(): output will show up in the xfs output file manager window")      

   XS_SYSCALL("cycleConsoleRenderMode", BXSVariableEntry::cVoidVariable, &cycleConsoleRenderMode, 0)
      XS_BOOL_PARM(true)
      XS_HELP("cycleConsoleRenderMode(bool dir): Move to next console output category in the specified direction")
      
   XS_SYSCALL("contentTest", BXSVariableEntry::cVoidVariable, &contentTest, 0)
      XS_HELP("contentTest(): testing the content APIs")      

   //XS_SYSCALL("loadAchievements", BXSVariableEntry::cVoidVariable, &loadAchievements, 0)
   //   XS_HELP("loadAchievements(): loads the achievement list")      

   XS_SYSCALL("getAchievements", BXSVariableEntry::cVoidVariable, &getAchievements, 0)
      XS_HELP("getAchievements(): retrieves the list of user achievements")      

   XS_SYSCALL("grantAchievement", BXSVariableEntry::cVoidVariable, &grantAchievement, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("grantAchievement(int achievementID): grants the primary use the achievement.")      

   XS_SYSCALL("revealMap" , BXSVariableEntry::cVoidVariable, &revealMap, 0)
      XS_BOOL_PARM(true)
      XS_HELP("revealMap([bool]): Reveals entire map or turns fog of war back on");

   XS_SYSCALL("submitStats", BXSVariableEntry::cVoidVariable, &submitStats, 0)
      XS_HELP("submitStats(): submits the current game stats as is.")

   XS_SYSCALL("setRank", BXSVariableEntry::cVoidVariable, &setRank, 0)
      XS_INTEGER_PARM(0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(true)
      XS_HELP("setRank(int rank, int level, bool serverUpdated): sets the primary user rank (0-8), level (0-50) and whether we were updated from the server.")

   XS_SYSCALL("setMatchmakingWins", BXSVariableEntry::cVoidVariable, &setMatchmakingWins, 0)
      XS_INTEGER_PARM(0)
      XS_INTEGER_PARM(0)
      XS_HELP("setMatchmakingWins(int wins, int score): sets the number of matchmaking wins and score on the primary user.")

   XS_SYSCALL("addSecondaryUserToParty", BXSVariableEntry::cVoidVariable, &addSecondaryUserToParty, 0)
      XS_HELP("addSecondaryUserToParty(): attempts to add a secondary signed-in user to the party session.")

   XS_SYSCALL("lspAuthTest", BXSVariableEntry::cVoidVariable, &lspAuthTest, 0)
      XS_HELP("lspAuthTest(): initiates a connection to the lsp and tests the auth sequence.")

   XS_SYSCALL("enableTicker", BXSVariableEntry::cVoidVariable, &enableTicker, 0)
      XS_BOOL_PARM(true)
      XS_HELP("enableTicker([bool]): enables/disables the Ticker.")

   XS_SYSCALL("timerTest", BXSVariableEntry::cVoidVariable, &timerTest, 0)
      XS_HELP("timerTest(): kicks off a game timer and enables the display.")

   XS_SYSCALL("offerTest", BXSVariableEntry::cVoidVariable, &offerTest, 0)
      XS_HELP("offerTest(): Brings up the blade to OfferID from user.cfg")

   XS_SYSCALL("showWait", BXSVariableEntry::cVoidVariable, &showWait, 0)
      XS_BOOL_PARM(true)
      XS_HELP("showWait([bShow]): tests the wait dialog popup.")

   XS_SYSCALL("pathingCam", BXSVariableEntry::cVoidVariable, &pathingCam, 0)
      XS_HELP("pathingCam(): places the camera in a position amenable for debugging pathing.")

   XS_SYSCALL("lookAtEntity", BXSVariableEntry::cVoidVariable, &lookAtEntity, 0)
      XS_INTEGER_PARM( 0 )
      XS_HELP("lookAtEntity(int entityID): positions the camera to look at an entity.")

   XS_SYSCALL("beginCamEffect", BXSVariableEntry::cVoidVariable, &beginCamEffect, 0)
      XS_FLOAT_PARM( 1.0 )
      XS_FLOAT_PARM( 1.0 )
      XS_FLOAT_PARM( 1.0 )
      XS_FLOAT_PARM( 0.0 )
      XS_FLOAT_PARM( 0.0 )
      XS_BOOL_PARM( false )
      XS_HELP("beginCamEffect(float duration, float saturation, float blurFactor, float blurX, float blurY, bool radialBlur): starts camera screen effect.")

   XS_SYSCALL("clearCamEffect", BXSVariableEntry::cVoidVariable, &clearCamEffect, 0)
      XS_HELP("clearCamEffect(): clears current camera screen effect.")

   XS_SYSCALL("patherTest", BXSVariableEntry::cVoidVariable, &patherTest, 0);
      XS_INTEGER_PARM(0)
      XS_INTEGER_PARM(5000)
      XS_INTEGER_PARM(0)
      XS_HELP("patherTest(pathType, pathCount, CanPath): pathType 0=short, 1=long, pathCount=#iterations, CanPath 0=normal, 1=canpath")

   XS_SYSCALL("upgradeLevel", BXSVariableEntry::cVoidVariable, &upgradeLevel, 0);
      XS_HELP("upgradeLevel(): Upgrade veterancy level of the selected squad(s)")
#endif      

   XS_SYSCALL("saveGame", BXSVariableEntry::cVoidVariable, &saveGame, 0)
      XS_STRING_PARM("")
      XS_HELP("saveGame(fileName)")

   XS_SYSCALL("loadGame", BXSVariableEntry::cVoidVariable, &loadGame, 0)
      XS_STRING_PARM("")
      XS_HELP("loadGame(fileName)")

   XS_SYSCALL("uiSelectAll", BXSVariableEntry::cVoidVariable, &uiSelectAll, 0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(false)
      XS_BOOL_PARM(false)
      XS_BOOL_PARM(false)
      XS_BOOL_PARM(false)
      XS_HELP("uiSelectAll(int port, bool global, bool reverse, bool autoGlobal, bool autoCycle)")

   XS_SYSCALL("uiCycleGroup", BXSVariableEntry::cVoidVariable, &uiCycleGroup, 0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(false)
      XS_BOOL_PARM(true)
      XS_HELP("uiCycleGroup(int port, bool reverse, bool wrap)")

   XS_SYSCALL("uiCycleTarget", BXSVariableEntry::cVoidVariable, &uiCycleTarget, 0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(false)
      XS_HELP("uiCycleTarget(int port, bool reverse)")

   XS_SYSCALL("uiSelectTarget", BXSVariableEntry::cVoidVariable, &uiSelectTarget, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("uiSelectTarget(int port)")

   XS_SYSCALL("uiCancel", BXSVariableEntry::cVoidVariable, &uiCancel, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("uiCancel(int port)")

   XS_SYSCALL("uiModifierAction", BXSVariableEntry::cVoidVariable, &uiModifierAction, 0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(false)
      XS_HELP("uiModifierAction(int port, bool on)")

   XS_SYSCALL("uiModifierSpeed", BXSVariableEntry::cVoidVariable, &uiModifierSpeed, 0)
      XS_INTEGER_PARM(0)
      XS_BOOL_PARM(false)
      XS_HELP("uiModifierSpeed(int port, bool on)")

   XS_SYSCALL("uiFlare", BXSVariableEntry::cVoidVariable, &uiFlare, 0)
      XS_INTEGER_PARM(0)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("uiFlare(int port, float x, float y)")

   XS_SYSCALL("uiMenu", BXSVariableEntry::cVoidVariable, &uiMenu, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("uiMenu(int port)")

   XS_SYSCALL("uiObjectives", BXSVariableEntry::cVoidVariable, &uiObjectives, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("uiObjectives(int port)")

#ifndef BUILD_FINAL
   XS_SYSCALL("saveToCurrentProfile", BXSVariableEntry::cVoidVariable, &saveToCurrentProfile, 0)
      XS_HELP("saveToCurrentProfile(void)")

   XS_SYSCALL("resetCampaignProgress", BXSVariableEntry::cVoidVariable, &resetCampaignProgress, 0)
      XS_HELP("resetCampaignProgress(void)")
#endif

#ifndef BUILD_FINAL
   XS_SYSCALL("pauseTriggers", BXSVariableEntry::cVoidVariable, &pauseTriggers, 0)
      XS_BOOL_PARM(true)
      XS_HELP("pauseTriggers(void)")

   XS_SYSCALL("stopSelected", BXSVariableEntry::cVoidVariable, &stopSelected, 0)
      XS_HELP("stopSelected(void): stops the selected squads")

   XS_SYSCALL("setPlayableBounds", BXSVariableEntry::cVoidVariable, &setPlayableBounds, 0)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("setPlayableBounds(float minX, float minZ, float maxX, float maxZ): sets the playable bounds.")

   XS_SYSCALL("rebuildLRP", BXSVariableEntry::cVoidVariable, &rebuildLRP, 0)
      XS_HELP("rebuildLRP: rebuild the LRP trees.")

   XS_SYSCALL("subUpdate", BXSVariableEntry::cVoidVariable, &subUpdate, 0)
      XS_STRING_PARM("on")
      XS_HELP("subUpdate(type: on, off, alternate, dynamic, decoupled)")
#endif

   return(true);
}
