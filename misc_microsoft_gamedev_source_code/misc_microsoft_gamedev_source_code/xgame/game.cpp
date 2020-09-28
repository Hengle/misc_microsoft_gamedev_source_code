//==============================================================================
// game.cpp
//
// Copyright (c) Ensemble Studios, 2005-2007
//==============================================================================

// Includes
#include "common.h"
#include "game.h"
#include "renderControl.h"

// xcore
#include "reloadManager.h"
#include "memory\allocationLogger.h"
#include "globalObjects.h"
#include "xdb\xexInfo.h"

// xgame
#include "configsgame.h"
#include "database.h"
#include "aitypes.h"
#include "gamecallbacks.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "modemenu.h"
#include "physicsinfomanager.h"
#include "physicsrenderadapter.h"
#include "syncmanager.h"
#include "ui.h"
#include "usermanager.h"
#include "user.h"
#include "workdirsetup.h"
#include "gamedirectories.h"
#include "world.h"
#include "visiblemap.h"
#include "AsyncTaskManager.h"
#include "archiveManager.h"
#include "ContentFileManager.h"
#include "achievementmanager.h"
#include "campaignmanager.h"
#include "damageTemplateManager.h"
#include "statsManager.h"
#include "lspManager.h"
#include "recordgame.h"
#include "HPBar.h"
#include "minimap.h"
#include "LocaleManager.h"
#include "cameraEffectManager.h"
#include "savegame.h"
#include "soundinfoprovider.h"
#include "deathmanager.h"
#include "skullmanager.h"
#include "miniLoadManager.h"

// xgameRender
#include "xgameRender.h"
#include "configsGameRender.h"
#ifndef BUILD_FINAL
   #include "fatalMessageBox.h"
   #include "consoleRender.h"
#endif
#include "render.h"
#include "viewportManager.h"
#include "lightEffectManager.h"
#include "flashbackgroundplayer.h"

// xmultiplayer
//#include "connectivity.h"
//#include "xmultiplayer.h"
#include "xLiveSystem.h"
#include "liveSession.h"

// xnetwork
#include "commlog.h"
#include "SocksHelper.h"

// xsystem
#include "debugchannel.h"
#include "utilities.h"
#include "xsystemlib.h"
#include "econfigenum.h"
#include "timelineprofiler.h"
#include "notification.h"

// xvisual
#include "xvisual.h"
//#include "flashbackgroundplayer.h"

// xparticles
#include "particlegateway.h"

// xphysics
#include "xphysics.h"

// xinput
#include "configsInput.h"

// terrain
#include "TerrainTexturing.h"

#include "AsyncTaskManager.h"
#include "AsyncOtherControllerInviteTask.h"

// xvince
#include "vincehelper.h"
#ifdef _TICKET_TRACKER_
#include "TicketTrackerImplementation.h"
   #ifdef _DEBUG
      #pragma comment(lib,"TicketTrackerd.lib")
      #pragma comment(lib,"StatusScreend.lib")
   #else
      #pragma comment(lib,"TicketTracker.lib")
      #pragma comment(lib,"StatusScreen.lib")
   #endif // _DEBUG
#endif // _TICKET_TRACKER_

#ifdef USE_BUILD_INFO
   #include "build_info.inc"
#endif

// Globals
BGame gGame;

#define XBOX_FILE_CACHE_SIZE 384*1024

#ifndef BUILD_FINAL
   #define DISPLAY_PROGRESS(text) if (!showingVideo) gRender.debugPrintf(text);
#else
   #define DISPLAY_PROGRESS(text) ((void)0)
#endif

//==============================================================================
// BGame::BGame
//==============================================================================
BGame::BGame() :
   mWindowHandle(NULL),
   mLastTime(0),
   mTotalTime(0),
   mFrameTime(0),
   mBuildChecksum(0),
   mMusicCue(NULL),
   mRenderPathingQuad(0),
   mRenderLRPTreeType(0),
   mObstructionRenderMode(0),
   mpUIGlobals(NULL),
   mRootFileCacheHandle(NULL),
   mRenderShowPaths(0),
   mAIDebugType(AIDebugType::cNone),
   mGameCount(0)
{
   clearFlags();
}

//==============================================================================
// BGame::~BGame
//==============================================================================
BGame::~BGame()
{
}

#ifndef BUILD_FINAL
// Undocumented function! Do not use this function in a shipping build.
extern "C" 
{
   DWORD WINAPI XGetSystemVersion(OUT LPSTR szVersionString, IN UINT cchVersionString);
};
//==============================================================================
// BGame::checkSystemVersion
//==============================================================================
bool BGame::checkSystemVersion(const char* pTitle)
{
   char buf[256];
   XGetSystemVersion(buf, sizeof(buf));
   
   trace("Flash version %s", buf);

   // "2.00.2858.1"
   //  0123456789A

   // Yes I know this string shouldn't be parsed, it's only for development to ensure the artists/designers are using the right flash.
   if ( (strlen(buf) == 12) && (buf[1] == '.') && (buf[4] == '.') && (buf[9] == '.') )
   {
      buf[9] = '\0';
      int ver = atoi(buf + 5);

      if ((ver > 0) && (ver < REQUIRED_FLASH_VERSION))
      {
         sprintf_s(buf, "Please flash your Xbox to v%i or later!", REQUIRED_FLASH_VERSION);
#define TO_STRING(x) #x      
         showFatalMessageBox(pTitle, buf);
         return false;
#undef TO_STRING         
      }
   }
   return true;
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// BGame::outputBuildInfo
//==============================================================================
void BGame::outputBuildInfo(void)
{
   //#define DEPOT_REVISION "@2007/08/07:14:01:50" 
   //#define BUILD_NUMBER "10" 
   //#define CHANGELIST_NUMBER "40184" 
      
   gConsoleOutput.printf("game.cpp built " __DATE__ " " __TIME__);
      
#ifdef USE_BUILD_INFO
   gConsoleOutput.printf("DEPOT_REVISION: " DEPOT_REVISION);
   gConsoleOutput.printf("BUILD_NUMBER: " BUILD_NUMBER);
   gConsoleOutput.printf("CHANGELIST_NUMBER: " CHANGELIST_NUMBER " (Original number, may have been changed by P4 during submit)");
#else
   gConsoleOutput.printf("USE_BUILD_INFO macro undefined, no build info available");
#endif
}
#endif

//==============================================================================
// BGame::setup
//==============================================================================
bool BGame::setup(const char* title, int commandShow, bool childWindowMode)
{
   const DWORD setupStartTime = GetTickCount();

   bool showingVideo = false;
   char str[256];

   TRACEMEM
   setFlagGameSetup(true);

#ifndef BUILD_FINAL
   if (!checkSystemVersion(title))
   {
      trace("BGame::setup: Exiting due to old Xbox flash");
      return false;
   }
#endif

#ifdef XBOX_FILE_CACHE_SIZE
   XSetFileCacheSize(XBOX_FILE_CACHE_SIZE);
#endif

   // Locale Manager 
   gLocaleManager.initEraFileFromLanguage();

   // XSystem
   XSystemInfo xsystemInfo;
   xsystemInfo.mUseXFS = true;
   xsystemInfo.mStartupDir = "Startup\\";
   xsystemInfo.mPreAssertCallback = gamePreAssert;
   xsystemInfo.mPostAssertCallback = gamePostAssert;
   xsystemInfo.mpConsoleInterface = (IConsoleInterface*)&gConsole;
   xsystemInfo.mLocEraFile = gLocaleManager.getLocFile();

   mRootFileCacheHandle = NULL;
   if (!XSystemCreate(&xsystemInfo, &mRootFileCacheHandle))
   {
      blogerrortrace("BGame::setup: XSystemCreate failed");
#ifndef BUILD_FINAL
      char buf[256];
      sprintf_s(buf, sizeof(buf), "BGame::setup: XSystemCreate failed!\nXFS probably couldn't connect to %s.\n", gXFS.getServerIP());
      showFatalMessageBox(title, buf);
#endif      
      return false;
   }
   
   setFlagSystemSetup(true);
   TRACEMEM
   
   // Game Directories
   if(!setupGameDirectories())
   {
      blogerrortrace("BGame::setup: setupGameDirectories failed");
#ifndef BUILD_FINAL
      showFatalMessageBox(title, "BGame::setup: setupGameDirectories failed. Please check your XFS Work Directory.");
#endif            
      return false;
   }

   TRACEMEM

   // Archive manager
   {
      gArchiveManager.setLocaleFilename(gLocaleManager.getLocFile());
      gArchiveManager.init(mRootFileCacheHandle);
      bool success = gArchiveManager.beginGameInit();
      BVERIFY(success);

      success = gArchiveManager.beginInGameUIPrefetch();
      BVERIFY(success);
   }

   TRACEMEM

   // Renderer
   XGameRenderInfo renderInfo;
   renderInfo.mDirArt = cDirArt;
   renderInfo.mDirEffects = cDirRenderEffects;
   renderInfo.mDirFonts = cDirFonts;
   renderInfo.mDirStartup = cDirStartup;
   renderInfo.mDirData = cDirData;
   if(!XGameRenderCreate(&renderInfo))
   {
      blogerrortrace("BGame::setup: XGameRenderCreate failed");
      return false;
   }
   
   TRACEMEM

   // DISPLAY_PROGRESS() can be used starting here (but NOT above this line)

   gRender.debugPrintClear();
      
   {
      MEMORYSTATUS status;
      GlobalMemoryStatus( &status );
      sprintf_s(str, sizeof(str), "%s Built %s %s, XFS: %s, Archives: %i, Free physical: %4.1fMB, Total Allocated: %4.1fMB", 
         title, 
         __DATE__, __TIME__, 
         gXFS.getServerIP(), gArchiveManager.getArchivesEnabled(), 
         status.dwAvailPhys / (1024.0f * 1024.0f), 
         (gInitialKernelMemoryTracker.getInitialPhysicalFree() - status.dwAvailPhys)  / (1024.0f * 1024.0f) );

      DISPLAY_PROGRESS(str);
   }

   TRACEMEM
   
   DISPLAY_PROGRESS("Initializing libraries:");
         
   // Locale Manager 
   gLocaleManager.init();  // call after config system has been loaded

   gRenderControl.init();

   #ifndef BUILD_FINAL
      gConsoleRender.init();
   #endif
         
   setFlagRenderSetup(true);
   TRACEMEM
   
   // Start intro cinematic after the render is setup
   BSimString introCinematic;
   if (!gConfig.isDefined(cConfigNoIntroCinematics) && gConfig.get(cConfigIntroCinematic, introCinematic))
   {
      gAsyncFileManager.syncAll();
      gRenderThread.blockUntilWorkerIdle();
      gRenderThread.blockUntilGPUIdle();
      TRACEMEM
      gFlashBackgroundPlayer.loadMovie(NULL, NULL, introCinematic, NULL, cDirData, true, false);
      gFlashBackgroundPlayer.startMovie();
      showingVideo = true;
   }
   TRACEMEM

   DISPLAY_PROGRESS("XVisual");

   // Visual manager
   XVisualInfo xvisualInfo;
   xvisualInfo.mDirID=cDirArt;
   xvisualInfo.mDirName="art\\";
   if(!XVisualCreate(&xvisualInfo))
   {
      DISPLAY_PROGRESS("BGame::setup: XVisualCreate failed");
      blogerrortrace("BGame::setup: XVisualCreate failed");
      return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("FontLibrary");
   gFlashGateway.initFontLibrary(gLocaleManager.getLanguageString(), cDirData);      

   setFlagVisualSetup(true);
   TRACEMEM

   DISPLAY_PROGRESS("XInputSystem");
   
   // Input system
   XInputSystemInfo inputSystemInfo;
   inputSystemInfo.mWindowHandle=mWindowHandle;
   inputSystemInfo.mRegisterConsoleFuncs=registerConsoleFuncs;
   inputSystemInfo.mpEventHandler=this;
   inputSystemInfo.mRootContext="Root";
   if(!XInputSystemCreate(&inputSystemInfo))
   {
      DISPLAY_PROGRESS("BGame::setup: XInputSystemCreate failed");
      blogerrortrace("BGame::setup: XInputSystemCreate failed");
      return false;
   }
      
#ifndef BUILD_FINAL   
   gConsoleOutput.output(cMsgConsole, "Initializing");
   outputBuildInfo();
#endif  

   setFlagInputSystemSetup(true);
   TRACEMEM
   
   DISPLAY_PROGRESS("BUI");
   
   // UI
   gUI.init();
   TRACEMEM

   DISPLAY_PROGRESS("XPhysics");
   
   // Physics
   XPhysicsInfo physicsInfo;
   physicsInfo.mDirPhysics=cDirPhysics;
   physicsInfo.mpRenderInterface=new BPhysicsRenderAdapter();
   if(!XPhysicsCreate(&physicsInfo))
   {
      DISPLAY_PROGRESS("BGame::setup: XPhysicsCreate failed");
      blogerrortrace("BGame::setup: XPhysicsCreate failed");
      return(false);
   }
   setFlagPhysicsSetup(true);
   TRACEMEM

   DISPLAY_PROGRESS("BDatabase");

   //-- Pre-setup sound so it can access xgame through interface
   XSoundSetInterface((ISoundInfoProvider*)&gSoundInfoProvider);
   
   // Compute local checksum here if archives are enabled, because the BDatabase setup will discard the files we need to read to compute the CRC.
   if (gArchiveManager.getArchivesEnabled())
      getLocalChecksum();

   // Database
   if(!gDatabase.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gDatabase.setup() failed");
      blogerrortrace("BGame::setup: gDatabase.setup() failed");
      return false;
   }
   gVisualManager.saveState();
   TRACEMEM

   // Allow intro cinematic to be stopped by the user at this point.
   if (showingVideo)
   {
      long minVideoTime = 0;
      gConfig.get(cConfigMinIntroTime, &minVideoTime);
      gFlashBackgroundPlayer.setCanQuickCancel(true, (DWORD)minVideoTime);
   }

   DISPLAY_PROGRESS("BAchievementManager");
   if (!gAchievementManager.loadAchievements())
   {
      DISPLAY_PROGRESS("BGame::setup: gAchievementManager.loadAchievements() failed");
      blogerrortrace("BGame::setup: gAchievementManager.loadAchievements() failed");
      return false;
   }

   TRACEMEM

   DISPLAY_PROGRESS("BCollectiblesManager");
   if (!gCollectiblesManager.loadCollectiblesDefinitions())
   {
      DISPLAY_PROGRESS("BGame::setup: gCollectiblesManager.loadCollectiblesDefinitions() failed");
      blogerrortrace("BGame::setup: gCollectiblesManager.loadCollectiblesDefinitions() failed");
      //return false;
   }

   TRACEMEM

   DISPLAY_PROGRESS("BSyncManager");

   // Sync Manager
   if(!BSyncManager::getInstance()->setup())
   {
      DISPLAY_PROGRESS("BGame::setup: BSyncManager::getInstance()->setup()");
      blogerrortrace("BGame::setup: BSyncManager::getInstance()->setup()");
      return false;
   }
   TRACEMEM
   
   DISPLAY_PROGRESS("XLiveSystem");

   // XBox Live - eric
   XLiveSystemInfo livesystemInfo;
   if(!XLiveSystemCreate(&livesystemInfo))
   {
      DISPLAY_PROGRESS("BGame::setup: XLiveSystemCreate()");
	   blogerrortrace("BGame::setup: XLiveSystemCreate()");
	   return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("BNotification");
   if (!gNotification.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gNotification.setup() failed");
      blogerrortrace("BGame::setup: gNotification.setup() failed");
      return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("BLSPManager");
   if (!gLSPManager.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gLSPManager.setup() failed");
      blogerrortrace("BGame::setup: gLSPManager.setup() failed");
      return false;
   }
   TRACEMEM

   // initialize the user manager after live
   DISPLAY_PROGRESS("BCampaignManager");
   // Campaign Manager Setup
   gCampaignManager.initialize();

   TRACEMEM

   // initialize the user manager after live
   DISPLAY_PROGRESS("BUserManager");

   // User manager
   if(!gUserManager.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gUserManager.setup() failed");
      blogerrortrace("BGame::setup: gUserManager.setup() failed");
      return false;
   }
   //BUser* user=gUserManager.createUser(0);
   //if(!user)
   //{
   //   DISPLAY_PROGRESS("BGame::setup: gUserManager.createUser(0) failed");
   //   blogerrortrace("BGame::setup: gUserManager.createUser(0) failed");
   //   return false;
   //}

   TRACEMEM

   // Visible map
   DISPLAY_PROGRESS("BVisibleMap");
   if(!gVisibleMap.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gVisibleMap.setup() failed");
      blogerrortrace("BGame::setup: gVisibleMap.setup() failed");
      return false;
   }
   TRACEMEM

   
   /*
   DISPLAY_PROGRESS("XMultiplayer");

   // Multiplayer
   XMultiplayerInfo multiplayerInfo;
   if(!XMultiplayerCreate(&multiplayerInfo))
   {
      DISPLAY_PROGRESS("BGame::setup: XMultiplayerCreate()");
      blogerrortrace("BGame::setup: XMultiplayerCreate()");
      return false;
   }
   setFlagMultiplayerSetup(true);
   TRACEMEM
   */

    // Sound system
   if(!gConfig.isDefined(cConfigNoSound))
   {
      DISPLAY_PROGRESS("XSound");
      
      // rg - Must wait for all archives to finish loading before we let sound access the DVD drive.
      if (gArchiveManager.getArchivesEnabled())
      {
#ifndef BUILD_FINAL      
         BTimer timer;
         timer.start();
#endif         
         
         gFileManager.waitUntilAllCachesAreLoadedOrFailed();

#ifndef BUILD_FINAL               
         double totalWaitTime = timer.getElapsedSeconds();
         trace("BGame::setup: Waited %3.3fms for archives to finish loading before sound accesses drive.", totalWaitTime * 1000.0f);
#endif         
      }

      if(!XSoundCreate())
      {
         DISPLAY_PROGRESS("BGame::setup: XSoundCreate failed");
         blogerrortrace("BGame::setup: XSoundCreate failed");
         return false;
      }
      setFlagSoundSetup(true);
      TRACEMEM
   }

   // Check for an auto-relaunch by selecting exit from the main menu in the demo. If so, then turn off the intro cinematics.
   if (gConfig.isDefined(cConfigDemo) || gConfig.isDefined(cConfigDemo2))
   {
      DWORD dataSize = 0;
      if (XGetLaunchDataSize(&dataSize) == ERROR_SUCCESS && dataSize == sizeof(int))
      {
         int data = 0;
         if (XGetLaunchData(&data, sizeof(int)) == ERROR_SUCCESS && data == 1234)
            gConfig.define(cConfigNoIntroCinematics);
      }
   }

   // Stop intro cinematic before BUIGlobals is initialized since it needs access to the renderer
   if (showingVideo)
   {
      gFlashBackgroundPlayer.stopMovie(true);
      gRenderThread.blockUntilWorkerIdle();
      gRenderThread.blockUntilGPUIdle();
      gFlashBackgroundPlayer.setCanQuickCancel(false, 0);
      showingVideo = false;
   }

   // Miniloader
   gMiniLoadManager.init();

   DISPLAY_PROGRESS("BModeManager");
   
   // Mode manager
   if(!gModeManager.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gModeManager.setup()");
      blogerrortrace("BGame::setup: gModeManager.setup()");
      return false;
   }
   TRACEMEM
  
   DISPLAY_PROGRESS("BUIGlobals");

   // UI Globals
   mpUIGlobals=new BUIGlobals();
   if (!mpUIGlobals->init("art\\ui\\flash\\pregame\\globalDialogs\\GlobalDialogs.gfx", "art\\ui\\flash\\pregame\\globalDialogs\\GlobalDialogsData.xml"))
   {
      DISPLAY_PROGRESS("BGame::setup: BUIGlobals.init()");
      blogerrortrace("BGame::setup: BUIGlobals.init()");
      return false;
   }
   TRACEMEM

   BUIScreen::install();

   DISPLAY_PROGRESS("BStatsManager");

   if (!gStatsManager.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gStatsManager.setup()");
      blogerrortrace("BGame::setup: gStatsManager.setup()");
      return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("BRecordGame");

   if (!gRecordGame.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gRecordGame.setup()");
      blogerrortrace("BGame::setup: gRecordGame.setup()");
      return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("BSaveGame");

   if (!gSaveGame.setup())
   {
      DISPLAY_PROGRESS("BGame::setup: gSaveGame.setup()");
      blogerrortrace("BGame::setup: gSaveGame.setup()");
      return false;
   }
   TRACEMEM

   DISPLAY_PROGRESS("BHPBar");

   gHPBar.init();
   //gMiniMap.gameInit();

   gCameraEffectManager.init();
   gDeathManager.init();
   gPhysicsInfoManager.init();
   
   TRACEMEM
         
   // debug info
   long mode = 0;
   if (gConfig.get(cConfigObstructionRenderMode, &mode))
      mObstructionRenderMode = mode;

   DISPLAY_PROGRESS("Setting game active");
   
   gGame.setActive(true);

   TRACEMEM
   
#if defined( _VINCE_ )
   BOOL enableLog = gConfig.isDefined( cConfigVinceEnableLog );
#else
   BOOL enableLog = false;
#endif
   sprintf_s(str, sizeof(str), "Vince Logging: %u", enableLog);
   DISPLAY_PROGRESS(str);
   MVinceInitialize( NULL, enableLog );
   MVinceEventAsync_GenericMessage("Vince is initialized");
   MVinceEventAsync_PreGameEvent("ProgramLaunched");
   MVince_CloseAndSendLog();

   trace("\n");
   double totalSetupTime = (GetTickCount() - setupStartTime) * .001f;
   sprintf_s(str, sizeof(str), "Setup Complete, Total Time: %3.2f secs", totalSetupTime);
   DISPLAY_PROGRESS(str);
   trace("Setup Complete, Total Time: %3.2f secs", totalSetupTime);

   //DCP 03/27/07: Add dumb option to auto load a scenario on launch.  I suppose this could
   //be optimized a smidge more by not going into the menu mode at all, but I'm not sure what
   //side effects that would create by not finishing the rest of this init first.
   #ifndef BUILD_FINAL
   BSimString scenarioName;
   if (gConfig.get(cConfigAutoScenarioLoad, scenarioName))
   {
      gModeManager.setMode(BModeManager::cModeMenu);
      BModeMenu* pMode=gModeManager.getModeMenu();
      if (pMode)
      {
         gDatabase.resetGameSettings();
         BGameSettings* pSettings=gDatabase.getGameSettings();
         if (pSettings)
         {
	         BSimString gameID;
		      MVince_CreateGameID(gameID);

            BUser* pUser=gUserManager.getPrimaryUser();
            BASSERT(pUser);
            pUser->setFlagUserActive(true);

            long numPlayers = 2;
            gConfig.get(cConfigAutoScenarioPlayers, &numPlayers);

            pSettings->setLong(BGameSettings::cPlayerCount, numPlayers);
            pSettings->setString(BGameSettings::cMapName, scenarioName);
            pSettings->setLong(BGameSettings::cMapIndex, -1);
            pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
		      pSettings->setString(BGameSettings::cGameID, gameID);
            pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
            pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
            pSettings->setLong(BGameSettings::cGameMode, 0);
            pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);

            if (gConfig.isDefined(cConfigAutoScenarioSkirmish))
            {
               pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
               if (gConfig.isDefined(cConfigAutoScenarioGameMode))
               {
                  BSimString gameModeName;
                  gConfig.get(cConfigAutoScenarioGameMode, gameModeName);
                  int gameMode = gDatabase.getGameModeIDByName(gameModeName);
                  if (gameMode != -1)
                     pSettings->setLong(BGameSettings::cGameMode, gameMode);
               }
               long teamNumPlayers = numPlayers / 2;
               for (long i=0; i<numPlayers; i++)
                  pSettings->setLong(PSINDEX(i+1, BGameSettings::cPlayerTeam), (i<teamNumPlayers ? 1 : 2));
            }
            else
               pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);

            if (gGame.isSplitScreenAvailable() && gUserManager.isSecondaryUserAvailable(true))
            {
//-- FIXING PREFIX BUG ID 1513
               const BUser* pUser2=gUserManager.getSecondaryUser();
//--
               gUserManager.setUserPort(BUserManager::cSecondaryUser, 1);
               gUserManager.getSecondaryUser()->setFlagUserActive(true);
               pSettings->setString(BGameSettings::cPlayer2Name, pUser2->getName());
               pSettings->setUInt64(BGameSettings::cPlayer2XUID, pUser2->getXuid());
               pSettings->setLong(BGameSettings::cPlayer2Type, BGameSettings::cPlayerHuman);
               if (gConfig.isDefined(cConfigAutoScenarioCoop))
                  pSettings->setBool(BGameSettings::cCoop, true);
            }

            pMode->setNextState(BModeMenu::cStateGotoGame);
         }
      }
   }
   //Same option for record games.
   else if (gConfig.get(cConfigAutoRecordGameLoad, scenarioName))
   {
      gModeManager.setMode(BModeManager::cModeMenu);
      BModeMenu* pMode=gModeManager.getModeMenu();
      if (pMode)
      {
         gDatabase.resetGameSettings();
         BGameSettings* pSettings=gDatabase.getGameSettings();
         if (pSettings)
         {
            BSimString recordName;
            strPathGetFilename(scenarioName, recordName);
            recordName.removeExtension();

            BUser* pUser=gUserManager.getPrimaryUser();
            BASSERT(pUser);
            pUser->setFlagUserActive(true);

            pSettings->setString(BGameSettings::cLoadName, recordName);
            pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeRecord);
            pMode->setNextState(BModeMenu::cStateGotoRecordGame);
         }
      }
   }
   else if (gConfig.get(cConfigAutoSaveGameLoad, scenarioName))
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

            BUser* pUser=gUserManager.getPrimaryUser();
            BASSERT(pUser);
            pUser->setFlagUserActive(true);

            pSettings->setLong(BGameSettings::cPlayerCount, 1);
            pSettings->setString(BGameSettings::cMapName, "");
            pSettings->setLong(BGameSettings::cMapIndex, -1);
            pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
            pSettings->setString(BGameSettings::cGameID, gameID);
            pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);
            pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
            pSettings->setLong(BGameSettings::cGameMode, 0);
            pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
            pSettings->setString(BGameSettings::cLoadName, scenarioName);

            pMode->setNextState(BModeMenu::cStateGotoGame);
         }
      }
   }
   #endif
   
   gRenderThread.blockUntilWorkerIdle();

#ifndef BUILD_FINAL   
   //gFileManager.dumpAllFileCacheInfo();
#endif
         
   {
      bool success = gArchiveManager.endGameInit();
      BVERIFY(success);
      success = gArchiveManager.endInGameUILoad();
      BVERIFY(success);
   }

   #ifdef _TICKET_TRACKER_
   LoadTicketTrackerSettings();
   #endif

   gVisualManager.registerDamageTemplateHandler(&gDamageTemplateManager);

   mFlagVerticalSplit=gConfig.isDefined(cConfigVerticalSplit);
   
   gConsoleOutput.output(cMsgConsole, "Ready\n");
   
#ifdef ALLOCATION_LOGGER
   getAllocationLogger().logSnapshot(0);
#endif      

#if !defined(BUILD_FINAL)
   if (gConfig.isDefined(cConfigRenderPathingQuad))
   {
      long value = 0;
      if (gConfig.get(cConfigRenderPathingQuad, &value) && (value >= 0) && (value <= 7))
      {
         setRenderPathingQuad(value);
      }      
   }         
#endif

   // everything is setup, register with guide 
   //const BUString & guideText = gDatabase.getLocStringFromID(24927);
   //XCustomSetAction(0, guideText, XCUSTOMACTION_FLAG_CLOSES_GUIDE);

   return true;
}

//==============================================================================
// BGame::emptyThreadQueues
//==============================================================================
void BGame::emptyThreadQueues(void)
{
   for (uint i = 0; i < 10; i++)
   {
      gEventDispatcher.pumpAllThreads(100, 10);
      
      for (uint threadIndex = cThreadIndexSim; threadIndex < cThreadIndexMax; threadIndex++)
      {
         if (gEventDispatcher.getThreadId(static_cast<BThreadIndex>(threadIndex)))
            gEventDispatcher.pumpUntilThreadQueueEmpty(static_cast<BThreadIndex>(threadIndex));
      }
            
      if (getFlagRenderSetup())
         gRenderThread.blockUntilGPUIdle();
   }      
}

//==============================================================================
// BGame::shutdown
//==============================================================================
void BGame::shutdown()
{
   if (gConfig.isDefined(cConfigDemo) || gConfig.isDefined(cConfigDemo2))
   {
      BXEXInfo xexInfo;
      if (xexInfo.getValid())
      {
         int data = 1234;
         XSetLaunchData(&data, sizeof(int));
         XLaunchNewImage(xexInfo.getModuleName(), 0);
      }
   }

#ifdef BUILD_FINAL
   XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
#endif

   if(!getFlagGameSetup())
      return;

   gNotification.shutdown();

   if (getFlagRenderSetup())
   {
      gReloadManager.clear();
      gRenderThread.blockUntilGPUIdle();
   }

   emptyThreadQueues();

   stopMusic();

   gModeManager.setMode(-1);
   gModeManager.shutdown();
   
   BUIScreen::remove();
   if (mpUIGlobals)
   {
      mpUIGlobals->deinit();
      mpUIGlobals = NULL;
   }

   gRecordGame.shutdown();

   gStatsManager.shutdown();

   gUserManager.reset();

   gCampaignManager.shutdown();

   gUI.deinit();

   if (getFlagPhysicsSetup())
   {
      XPhysicsRelease();
      setFlagPhysicsSetup(false);
   }

   BSyncManager::getInstance()->shutdown();

   gLSPManager.shutdown();

   if (gLiveSystem)
      gLiveSystem->shutdown();

   /*
   if(getFlagMultiplayerSetup())
   {
      XMultiplayerRelease();
      setFlagMultiplayerSetup(false);
   }
   */

   gVisibleMap.shutdown();
   
   gHPBar.deinit();
   //gMiniMap.gameDeinit();

   gDatabase.shutdown();

   if(getFlagInputSystemSetup())
   {
      XInputSystemRelease();
      setFlagInputSystemSetup(false);
   }

   if(getFlagVisualSetup())
   {
      XVisualRelease();
      setFlagVisualSetup(false);
   }

   if(getFlagSoundSetup())
   {
      XSoundRelease();
      setFlagSoundSetup(false);
   }
   
   emptyThreadQueues();

   if(getFlagRenderSetup())
   {
      #ifndef BUILD_FINAL
         gConsoleRender.deinit();
      #endif

      gRenderControl.deinit();
      
      XGameRenderRelease();
      setFlagRenderSetup(false);
   }
   
   gArchiveManager.deinit();

   if(getFlagSystemSetup())
   {
      XSystemRelease();
      setFlagSystemSetup(false);
   }

   clearFlags();
   
   Sleep(1000);

   XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
}

//==============================================================================
// BGame::run
//==============================================================================
int BGame::run()
{
   for(;;)
   {
      if(!update())
         break;
   }
   return 0;
}

//==============================================================================
//==============================================================================
struct BScopedWaitForPhysicsUpdateCompletion
{
   BScopedWaitForPhysicsUpdateCompletion() {}

   ~BScopedWaitForPhysicsUpdateCompletion()
   {
      if (gWorld && gWorld->getPhysicsWorld())
         gWorld->getPhysicsWorld()->waitForUpdateCompletion();
   }

};

//==============================================================================
// BGame::udpate
//==============================================================================
bool BGame::update()
{
#if defined(ENABLE_TIMELINE_PROFILER)   
   if (gBProfileSystem.isEnabled() == false)
   {
      static long enabled = 1;
      if (enabled != -1 && gConfig.get(cConfigEnableTimelineProfiler, &enabled))
      {
         if(enabled == 1)
         {            
            gBProfileSystem.init();               
            enabled = 0;
         }
         else
         {
            enabled = -1;
         }
      }
   }
   
   BProfileManagerFrameProfHelper profileManagerFrameProfHelper;
#endif

   // When this object goes out of scope (whenever we return from this update)
   // it will wait until the physics is done processing
   BScopedWaitForPhysicsUpdateCompletion scopedPhysicsWait;

#ifdef ALLOCATION_LOGGER
   static uint allocationLoggerFrame;
   getAllocationLogger().logFrame(allocationLoggerFrame);
   allocationLoggerFrame++;
#endif

   //Check for xbox system notifications - eric
   gNotification.update();
   DWORD notification = gNotification.getNotification();
   switch(notification)
   {
   case XN_CUSTOM_ACTIONPRESSED:
      {
         // todo : start UI, initiate Title server request for data
      }
      break;
   case XN_LIVE_INVITE_ACCEPTED:
      {
//-- FIXING PREFIX BUG ID 1515
         const BUser* user = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
         //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
         if (user && user->getFlagUserActive() && user->isSignedIn() )
         {
            //Ok - there is currently already a primary user
            //Is that user the same as the user that is accepted this invite
            if (gNotification.getParam() == (DWORD)user->getPort())
            {
               //Yes - get the info
               if (gLiveSystem->inviteReceived(user->getPort()))
               {
                  //Is this an invite for the party that I am already in?
                  XNKID tempKID;
                  //Because that would be retarded to accept.
                  if (gLiveSystem->getMPSession() &&
                      !gLiveSystem->getMPSession()->isInLANMode() &&
                      gLiveSystem->getMPSession()->getPartySession() &&                     
                      gLiveSystem->getMPSession()->getPartySession()->getLiveSession() &&
                      gLiveSystem->getMPSession()->getPartySession()->getLiveSession()->getXNKID(tempKID) &&
                      (memcmp(&tempKID, &gLiveSystem->getInviteInfo()->hostInfo.sessionID, sizeof(XNKID))==0))
                  {
                     //We can ignore it
                     gLiveSystem->clearInviteInfo();
                  }
                  else
                  {
                     handleAcceptedInvite();
                  }
               }
               else
               {
                  //Invite info was not there for that port - nothing else we can do here
                  //  We got an invite on a port from the OS for which the OS now tells us there is no invite info for
                  //  Or it was an invite for another title - we can just ignore
                  //BASSERT(false);
               }
            }
            else
            {
               if (gLiveSystem->inviteReceived(gNotification.getParam()))
               {
                  //Is this an invite for the party that I am already in?
                  XNKID tempKID;
                  //Because that would be retarded to accept.
                  if (gLiveSystem->getMPSession() &&
                     !gLiveSystem->getMPSession()->isInLANMode() &&
                     gLiveSystem->getMPSession()->getPartySession() &&                     
                     gLiveSystem->getMPSession()->getPartySession()->getLiveSession() &&
                     gLiveSystem->getMPSession()->getPartySession()->getLiveSession()->getXNKID(tempKID) &&
                     (memcmp(&tempKID, &gLiveSystem->getInviteInfo()->hostInfo.sessionID, sizeof(XNKID))==0))
                  {
                     //We can ignore it
                     gLiveSystem->clearInviteInfo();
                  }
                  else
                  {
                     handleOtherControllerAcceptedInvite(); 
                  }
               }
               else
               {
                  //Invite info was not there for that port - nothing else we can do here
                  //  We got an invite on a port from the OS for which the OS now tells us there is no invite info for
                  //  Or it was an invite for another title - we can just ignore
                  //BASSERT(false);

               }
            }
         }
         else
         {
            //There is not a current primary user - make the invite requester the primary
            gUserManager.setUserPort(BUserManager::cPrimaryUser, gNotification.getParam());
            BUser* pUser = gUserManager.getPrimaryUser();
            pUser->setFlagUserActive(true);
            gUserManager.updateSigninByPort();
            //Get the info
            if (gLiveSystem->inviteReceived(pUser->getPort()))
            {
               handleAcceptedInvite();
            }
            else
            {
               //Invite info was not there for that port - nothing else we can do here
               //  We got an invite on a port from the OS for which the OS now tells us there is no invite info for
               //  Or it was an invite for another title - we can just ignore
               //We do need to go to the main menu so that it dumps any game they were in because we changed profiles
               gModeManager.setMode(BModeManager::cModeMenu); 
               //BASSERT(false);
            }
         }
      }
      break;
   default:
      break;
   }

#ifndef BUILD_FINAL
   //Matchmaking automated testing support - eric
   if (gLiveSystem->isMultiplayerGameActive())
   {      
      if ((gWorld) &&
         (gWorld->getUpdateNumber()>100) &&
         (gLiveSystem->getMPTestOptions(BLiveSystem::cLiveSystemTestAutoEnd)==TRUE))
      {
         //See if the game is loaded and running, and if so - resign and quit
         BUser* user = gUserManager.getUser(BUserManager::cPrimaryUser);
         if ((user) && (user->getPlayer()))
         {
            user->getPlayer()->sendResign();
         }
         gModeManager.setMode(BModeManager::cModeMenu);
      }      
   }
#endif

   gModeManager.frameStart();

   updateTime();

   gAsyncTaskManager.update();
   gContentFileManager.update();

#if defined(ENABLE_TIMELINE_PROFILER)
   BSocksHelper::socksTickActiveSockets();
#endif

   gLSPManager.update();

   gDebugChannel.pumpCommands();
   gSoundManager.update();
   gDatabase.update();
   gParticleGateway.update();
   if (gWorld)
      gLightEffectManager.update(gWorld->getSubGametime(), gWorld->getLastSubUpdateLengthFloat());
   gUI.update();

   // ajl 10/22/08 - Update the mode before updating the user so that the user class will be working with current world data.
   // This is important for stuff like having the camera follow a unit.
   if (!gModeManager.update())
   {
      // ajl 10/22/08 - Don't continue updating if the mode changed during the mode update otherwise a
      // render will occur for the new mode before it's updated.
      return true;
   }

   // Determine whether there is going to be a render this update
   bool isRenderFrame = true;
#ifdef DECOUPLED_UPDATE
   if(gDecoupledUpdate && gWorld && !gWorld->getDecoupledUpdateRender())
      isRenderFrame = false;
#endif

   // Only do this if we're actually going to render this time.
   if(isRenderFrame)
   {
      gInputSystem.update();
      gUserManager.update();
   }

   // need this after the user manager update because of potential changes made to the primary user
   gLiveSystem->update();

#ifdef _VINCE_
   MVinceEventSync_UnitInterval();
   MVinceEventSync_BuildingInterval();
   MVinceEventSync_ResourceInterval();
   MVinceEventSync_PathingInterval();
#endif


   updateRender();
         
   #ifndef BUILD_FINAL
      // For debugging, sync all processors so renderer draws results of current frame's game update
      if (gConfig.isDefined(cConfigSyncSimAndRenderFrames))
      {
         gRenderThread.blockUntilGPUIdle();
      }
   #endif

   if(getFlagExit())
   {
      trace("BGame::update: Exiting");
      return false;
   }
         
   gModeManager.frameEnd();

   return true;
}

//==============================================================================
// BGame::yornResult
//==============================================================================
void BGame::yornResult(uint result, DWORD userContext, int port)
{
   switch (userContext)
   {
   case cGameYornAcceptInvite:
      {
         switch(result)
         {
         case BUIGlobals::cDialogResultOK:
            gLiveSystem->setPartySystemForReenter(false);
            gModeManager.setMode(BModeManager::cModePartyRoom2); 
            break;
         case BUIGlobals::cDialogResultCancel:
            break;
         }
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BGame::acceptOtherControllerInvite()
{
   gLiveSystem->setPartySystemForReenter(false);
   BModeMenu* pMode=gModeManager.getModeMenu();
   if (pMode)
   {
      // switch out to the main menu so the user can be cleaned up there before going into the party room.

      // Note:: if you change this mechanism, you need to make sure you look at BModePartyRoom2::leave(). It clears out 
      //    the invite except when this situation occurs. That will have to be changed too.
      pMode->setNextState(BModeMenu::cStateInviteFromOtherController);
      gModeManager.setMode(BModeManager::cModeMenu); 
   }
}


//==============================================================================
//==============================================================================
void BGame::handleOtherControllerAcceptedInvite()
{
   // if we are not in mode game, then just accept the invite outright.
   //If they are in the middle of a game, confirm that they want to accept the invite
   // for testing
   if (gModeManager.getModeType() != BModeManager::cModeGame)
   {
      acceptOtherControllerInvite();
      return;
   }

   // create an async task to pop a dialog and wait for the result.
   BAsyncOtherControllerInviteTask* pTask = new BAsyncOtherControllerInviteTask();
   pTask->confirmInvite(gLiveSystem->getInviteAcceptersPort());
   gAsyncTaskManager.addTask(pTask);
}


//==============================================================================
// BGame::handleAcceptedInvite
//==============================================================================
void BGame::handleAcceptedInvite()
{
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
   {
      pUIGlobals->hideYorn();

      //If they are in the middle of a game, confirm that they want to accept the invite
      if (gModeManager.getModeType() == BModeManager::cModeGame)
      {
         pUIGlobals->showYornBox(this, gDatabase.getLocStringFromID(25993), BUIGlobals::cDialogButtonsOKCancel, cGameYornAcceptInvite);
      }
      //If they are not in the middle of a game, act as though they confirmed they want to accept the invite
      else
      {
         BUser* pUser = gUserManager.getPrimaryUser();
         yornResult(BUIGlobals::cDialogResultOK, cGameYornAcceptInvite, pUser->getPort());
      }
   }
}

//==============================================================================
// BGame::updateTime
//==============================================================================
void BGame::updateTime()
{
   DWORD newTime=timeGetTime();
   if(mLastTime!=0)
   {
      mFrameTime=newTime-mLastTime;
      mTotalTime+=mFrameTime;
   }
   mLastTime=newTime;
}

//==============================================================================
// BGame::updateRender
//==============================================================================
void BGame::updateRender()
{
   SCOPEDSAMPLE(updateRender);

   if(!getFlagActive())
      return;

   gModeManager.renderBegin();
   gModeManager.render();


   if (mpUIGlobals && ( gModeManager.getModeType() != BModeManager::cModeGame) )
      mpUIGlobals->render();

   gModeManager.renderEnd();
}

//==============================================================================
// BGame::exit
//==============================================================================
void BGame::exit()
{
   setFlagExit(true);
}

//==============================================================================
// BGame::handleInput
//==============================================================================
bool BGame::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (mpUIGlobals)
   {
      if (mpUIGlobals->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail))
         return true;
   }

   if(gUserManager.handleInput(port, event, controlType, detail))
      return true;
   return gModeManager.handleInput(port, event, controlType, detail);
}

//==============================================================================
// BGame::startMusic
//==============================================================================
void BGame::startMusic()
{
   if(!gConfig.isDefined(cConfigNoMusic) && !mMusicCue)
   {
      mMusicCue=gSoundManager.playCue("play_menu");
   }
}

//==============================================================================
// BGame::stopMusic
//==============================================================================
void BGame::stopMusic()
{
   if(mMusicCue)
   {
      gSoundManager.playCue("stop_menu");
      mMusicCue=NULL;
   }
}

//==============================================================================
// BGame::setupSync
//==============================================================================
void BGame::setupSync(BMPSyncObject *object)
{
   BSyncManager::getInstance()->reset();
   BSyncManager::getInstance()->setSyncedObject(object);
}

//==============================================================================
// BGame::getGameDataSet
//==============================================================================
BDataSet* BGame::getGameDataSet()
{ 
   return (BDataSet*)gDatabase.getGameSettings();
}

//==============================================================================
// BGame::getLocalChecksum
//==============================================================================
DWORD BGame::getLocalChecksum()
{
   if(mBuildChecksum!=0)
      return mBuildChecksum;

   DWORD crc=0;

#ifndef BUILD_FINAL               
   if (!gConfig.isDefined(cConfigNoExeCrc))
#endif   
   {
#ifndef BUILD_FINAL   
	   if (!gConfig.isDefined(cConfigIgnoreMPChecksum))
#endif	   
	   {
         BXEXInfo xexInfo;
         if (xexInfo.getValid())   
	         crc=getLocalFileCRC32(xexInfo.getModuleFullName().getPtr(), crc);
	      else
	         gConsoleOutput.error("BXEXInfo failed");
	         
	      gConsoleOutput.printf("BGame::getLocalChecksum: XEX CRC: 0x%08X", crc);
	   }
	}
#ifndef BUILD_FINAL
   crc=getFileCRC32(cDirProduction, "tools\\build\\build_number.txt", crc);
#endif

   crc=getFileCRC32(cDirStartup, "game.cfg", crc);
   
   const char* pDataFilenames[] = 
   {
      "civs.xml",
      "civs.xml.xmb",
      "gamedata.xml", 
      "gamedata.xml.xmb", 
      "leaders.xml", 
      "leaders.xml.xmb", 
      "objects.xml", 
      "objects.xml.xmb", 
      "powers.xml", 
      "powers.xml.xmb", 
      "squads.xml", 
      "squads.xml.xmb", 
      "techs.xml", 
      "techs.xml.xmb"
   };
   
   const uint cNumDataFilenames = sizeof(pDataFilenames)/sizeof(pDataFilenames[0]);
   for (uint i = 0; i < cNumDataFilenames; i++)
      crc=getFileCRC32(cDirData, pDataFilenames[i], crc);
      
   gConsoleOutput.printf("BGame::getLocalChecksum: BuildChecksum: 0x%08X", crc);

   // This method can't be executed again when archives are enabled, so make sure the crc is nonzero.
   if (!crc) 
      crc++;
   
   mBuildChecksum=crc;
   return mBuildChecksum;
}

//=============================================================================
// BGame::startGameSync
//=============================================================================
bool BGame::startGameSync()
{
   gModeManager.setMode(BModeManager::cModeGame);
   return true;
}

//==============================================================================
// BGame::initCommLogging
//==============================================================================
void BGame::initCommLogging()
{
//#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigCommLogging))
   {
      BCommLog::initialize(COMM_LOG_NAME);
      long commHistorySize=0;
      if (gConfig.get(cConfigCommLogHistorySize, &commHistorySize))
         BCommLog::setUseHistory(true, commHistorySize);
   }
//#endif
}

//==============================================================================
// BGame::isOOS
//==============================================================================
bool BGame::isOOS()
{
   return BSyncManager::getInstance()->isOOS();
}

//==============================================================================
// BGame::networkDisabled
//==============================================================================
void BGame::networkDisabled()
{
   // if this function gets called we have lost connectivity and the network has been disabled.
   if (gLiveSystem)
      gLiveSystem->disposeMPSession();
   gModeManager.setMode(BModeManager::cModeMenu);
}

//==============================================================================
// BGame::getDataDirID
//==============================================================================
long BGame::getDataDirID()
{
   return cDirData;
}

//==============================================================================
// BGame::getGametime
//==============================================================================
long BGame::getGametime()
{
   if(gWorld)
      return gWorld->getGametime();
   else
      return 0;
}

void BGame::setAIDebugType(int v)
{
   if (v >= AIDebugType::cMin && v <= AIDebugType::cMax)
      mAIDebugType = v;
}

void BGame::incAIDebugType()
{
   if (mAIDebugType == AIDebugType::cMax)
      mAIDebugType = AIDebugType::cMin;
   else
      mAIDebugType++;
}

void BGame::decAIDebugType()
{
   if (mAIDebugType == AIDebugType::cMin)
      mAIDebugType = AIDebugType::cMax;
   else
      mAIDebugType--;
}

//==============================================================================
// BGame::clearFlags
//==============================================================================
void BGame::clearFlags()
{
   mFlagActive=false;
   mFlagExit=false;
   mFlagGameSetup=false;
   mFlagSystemSetup=false;
   mFlagRenderSetup=false;
   mFlagSoundSetup=false;
   mFlagInputSystemSetup=false;
   mFlagMultiplayerSetup=false;
   mFlagVisualSetup=false;
   mFlagPhysicsSetup=false;
   mFlagSplitScreen=false;
   mFlagVerticalSplit=false;
   mFlagVinceLogOpen=false;
}

//==============================================================================
// BGame::setSplitScreen
//==============================================================================
bool BGame::isSplitScreenAvailable()
{
   if (!gConfig.isDefined(cConfigSplitScreen))
      return false;

   // ajl 11/27/07 - Only allow split screen if widescreen for now
   if (gRender.getWidth() >= 1280)
      return true;
   else
      return false;
}

//==============================================================================
// BGame::setSplitScreen
//==============================================================================
void BGame::setSplitScreen(bool val)
{
   if (val == mFlagSplitScreen)
      return;

   mFlagSplitScreen = val;

   if (val)
      gTerrainTexturing.setMaxTextureLOD(1);
   else
      gTerrainTexturing.setMaxTextureLOD(0);

   gSoundManager.setSplitScreen(val);
   
   gViewportManager.set(val, mFlagVerticalSplit, gViewportManager.getMasterViewportWidth(), gViewportManager.getMasterViewportHeight());
}

//==============================================================================
//==============================================================================
void BGame::openVinceLog()
{
   if (!mFlagVinceLogOpen)
   {
      MVince_OpenNewLog();
      mFlagVinceLogOpen=true;
   }
}

//==============================================================================
//==============================================================================
void BGame::closeVinceLog()
{
   if (mFlagVinceLogOpen)
   {
      MVince_CloseAndSendLog();
      mFlagVinceLogOpen=false;
   }
}
