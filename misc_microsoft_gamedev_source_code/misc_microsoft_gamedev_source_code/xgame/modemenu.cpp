//==============================================================================
// modemenu.cpp
//
// Copyright (c) 2005-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modemenu.h"
#include "civ.h"
#include "database.h"
#include "fontSystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamefilemanifest.h"
#include "gamesettings.h"
#include "leaders.h"
#include "modemanager.h"
#include "ModePartyRoom2.h"
#include "recordgame.h"
#include "render.h"
#include "ui.h"
#include "usermanager.h"
#include "user.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "globalObjects.h"
#include "archiveManager.h"
#include "campaignmanager.h"
#include "UIGlobals.h"
#include "xfs.h"      
#include "gamemode.h"
#include "primDraw2D.h"

// Screens
#include "UIOptionsMenu.h"
#include "UIAttractScreen.h"
#include "UITimelineScreen.h"
#include "UILeaderboards.h"
#include "UICreditsMoviePlayer.h"

#include "player.h"
#include "humanPlayerAITrackingData.h"
#include "BackgroundMovies.h"
#include "campaignprogress.h"

#ifndef BUILD_FINAL
#include "memory\allocationLogger.h"
#include "xdb\xdbManager.h"
#include "consoleRender.h"
#include "D3DTextureManager.h"
#include "D3DTextureManager.h"
#include "grannyManager.h"
#include "particlegateway.h"
#include "lightEffectManager.h"
#include "particleTextureManager.h"
#endif

#include "lspManager.h"

// xvince
#include "vincehelper.h"
// graph
#include "graphmanager.h"
#include "world.h"

#include "protoobject.h"

#include "LiveSystem.h"

#ifdef USE_BUILD_INFO
   #include "build_info.inc"
#endif

// AJL 2/26/08 - Temp disable alpha menu so we can more easily test other options with the alpha config turned on
#define ALPHA_MENU

#include "UIMainMenuScreen.h"
#include "UIServiceRecordScreen.h"
#include "UISkirmishSetupScreen.h"

// Constants
static const float cFirstNextSelectionTime=0.5f;
static const float cFastNextSelectionTime=0.1f;
static const float cTitleOffset=0.0f;

//#define ENABLE_GRAPHTEST_CODE

//==============================================================================
// BModeMenu::BModeMenu
//==============================================================================
BModeMenu::BModeMenu(long modeType) :
   BMode(modeType),
   mSignInRequested(FALSE),
   mSignInGoldRequired(false),
   mState(-1),
   mNextState(cStateIntroCinematic),
   mNextStateAfterSignin(-1),
   mGotoGameFromState(-1),
   mScenarioFilter(0),
   mLastScenarioItem(0),
   mLastFlashItem(0),
   mLastCinematicItem(0),
   mLastModelViewItem(0),
   mSetting(-1),
   mSettingCurrentPlayer(-1),
   mCurrentPlayerSetting(-1),
   mSettingPlayers(0),
   mSettingMap(),
   mPlayerSettingTeam(0),
   mPlayerSettingDifficulty(0.0f),
   mPlayerSettingCiv(0),
   mPlayerSettingLeader(-1),
   mSettingGameMode(0),
   mSettingRecordGame(false),
   mControlling(false),
   mControlDirection(0),
   mControlTime(0.0f),
   mControlFast(false),
   mIntroVideoMinTime(0),
   mIntroVideoDone(false),
   mpMainMenuScreen(NULL),
   mpUIScreen(NULL),
   mMenuItemFocus(-1),
   mRecordingsPort(XUSER_MAX_COUNT),
   mBackgoundVideoHandle(cInvalidVideoHandle),
   mDemoVideoHandle(cInvalidVideoHandle),
   mLastRecordGameUpdate(0),
   mpBackgroundMovies(NULL)
{
}

//==============================================================================
// BModeMenu::~BModeMenu
//==============================================================================
BModeMenu::~BModeMenu()
{
}

//==============================================================================
// BModeMenu::setup
//==============================================================================
bool BModeMenu::setup()
{
   return BMode::setup();
}

//==============================================================================
// BModeMenu::shutdown
//==============================================================================
void BModeMenu::shutdown()
{
   BMode::shutdown();
}

//==============================================================================
// BModeMenu::preEnter
//==============================================================================
void BModeMenu::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeMenu::enter
//==============================================================================
void BModeMenu::enter(BMode* lastMode)
{
   bool success = gArchiveManager.beginMenu();
   BVERIFY(success);

   //gLiveSystem->setPresenceState(CONTEXT_PRESENCE_PRE_MODE_GENERAL, CONTEXT_PRE_CON_ACTION_PRE_CON_ACTION_WAITING, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_SOLO);
   if (mNextState != cStateGotoGame)
   {
      gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_MAINMENU);
   }

   mVideoPaused=false;

   if (mpBackgroundMovies==NULL)
   {
      mpBackgroundMovies=new BBackgroundMovies();
      mpBackgroundMovies->loadEvents();
   }

   if (mpMainMenuScreen==NULL)
   {
      mpMainMenuScreen = new BUIMainMenuScreen();
      BFATAL_ASSERTM(mpMainMenuScreen->init("art\\ui\\flash\\pregame\\MainMenu\\UIMainMenuScreen.gfx", "art\\ui\\flash\\pregame\\MainMenu\\UIMainMenuScreenData.xml"), "Main Menu UI Init Failed!");
      mpMainMenuScreen->setVisible(true);
      mpMainMenuScreen->enter();
   }

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigShowSafeArea))
      mpMainMenuScreen->showSafeAreaGuide(true);
#endif

   //-- Load the pregame banks
   gSoundManager.loadSoundBank(gSoundManager.getPregameBank(), false);

   gSoundManager.setGameStartDelay(false);

   // when we enter this mode, we should either be in the main state or the intro cinematic state.
   if (mNextState!=cStateIntroCinematic)
   {
      if (mNextState != cStateMain)
      {
         if (mNextState == -1)
            mNextState=cStateMain;
      }

      //loadBackgroundMovie(cModeMenuMovieTypeDefault);
      loadBackgroundMovie(cModeMenuMovieTypeCampaign);
   }


   gInputSystem.setCaptureInput(true);

   gGame.closeVinceLog();

   mRecordingsPort = XUSER_MAX_COUNT;
   mLastRecordGameUpdate = 0;
   
   return BMode::enter(lastMode);
}

//==============================================================================
// BModeMenu::leave
//==============================================================================
void BModeMenu::leave(BMode* newMode)
{
   gInputSystem.setCaptureInput(false);

   // don't play the video anymore
   gBinkInterface.unregisterValidCallback(this);
   gBinkInterface.stopAllVideos(false);
   mBackgoundVideoHandle = cInvalidVideoHandle;
   mDemoVideoHandle = cInvalidVideoHandle;

   if (mpBackgroundMovies != NULL)
   {
      delete mpBackgroundMovies;
      mpBackgroundMovies=NULL;
   }
   
   if (mpMainMenuScreen)
   {
      mpMainMenuScreen->deinit();
      delete mpMainMenuScreen;
      mpMainMenuScreen=NULL;
   }

   // delete the screen because we are leaving the mode.
   if (mpUIScreen)
   {
      mpUIScreen->deinit();
      delete mpUIScreen;
      mpUIScreen=NULL;
   }


   // clean up the cache listing of record games
   // XXX need to keep the record game cache around
   //gRecordGame.clean();

   // Make sure that when we come back, we are set up properly (unless somebody overrides this)   
   mNextState=cStateMain;
   mState=-1;
   mMenuItemFocus=-1;         // reset our index unless somebody overrides it.

   //Little block of code to fix the issue where if you try to leave the mainmenu while it is playing videos - that it would 
   //   just skip to the next video instead of letting you exit - eric
   if (mState==cStateIntroCinematic)
   {
      mState = cStateMain;
   }
   return BMode::leave(newMode);
}

//==============================================================================
// BModeMenu::loadBackgroundMovie
//==============================================================================
void BModeMenu::loadBackgroundMovie(int movieType, const char* pMovieName, bool loopVideo, const char* pCaptionFileName /*= NULL*/)
{
   BSimString movie;

   switch (movieType)
   {
      case cModeMenuMovieTypeDefault:
         {
            gConfig.get(cConfigUIBackgroundMovieMain, movie);
         }
         break;

      case cModeMenuMovieTypeCampaign:
         {
            // main menu and any other screens in this mode that want to show the campaign progress video.
            BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
            long unlockCount = 0;
            if (pProgress != NULL)
               unlockCount = pProgress->getUnlockedMissionCount();

            BASSERT(mpBackgroundMovies);
            if (mpBackgroundMovies)
            {
               // [7/31/2008 xemu] cycle through all the possible background movies, noting ones we qualify for and then when we finish, the last qualifier is the winner 
               for (int i=0; i<mpBackgroundMovies->getNumber(); i++)
               {
                  const BBackgroundMovieData* pData = mpBackgroundMovies->getData(i);
                  if (pData)
                  {
                     // look for the node that matches our current node
                     if (pData->mUnlocksAt <= unlockCount)
                        movie.set(pData->mMovieName.getPtr());
                  }
               }
            }
         }
         break;

      case cModeMenuMovieTypeSpecific:
         {
            if (pMovieName)
               movie.set(pMovieName);
         }
         break;
      default:
         {
            gConfig.get(cConfigUIBackgroundMovieMain, movie);
         }
         return;
   }

   // don't start a movie if we are asking for the one that is already running.
   if ( (mBackgoundVideoHandle != cInvalidVideoHandle) &&
         (mCurrentPlayingMovie == movie) )
   {
      return;
   }

   // cache it off
   mCurrentPlayingMovie = movie;

   gBinkInterface.stopAllVideos(false);
   mBackgoundVideoHandle=cInvalidVideoHandle;

   if (movie.length() > 0)
   {
      BBinkInterface::BLoadParams lp;
      lp.mFilename.set(movie.getPtr());
      if( pCaptionFileName )
         lp.mCaptionFilename.set(pCaptionFileName);
      lp.mCaptionDirID = cDirData;
      lp.mpStatusCallback = this;
      lp.mLoopVideo = loopVideo;
      lp.mFullScreen = true;
      gBinkInterface.registerValidCallback(this);
      mBackgoundVideoHandle = gBinkInterface.loadActiveVideo(lp);//gBinkInterface.loadActiveVideoFullScreen(movie.getPtr(), cDirData, "", "", this, NULL, true);
   }

   // make sure the videso are playing
   gBinkInterface.playAllVideos();
}


//==============================================================================
// findScenarioFiles
//==============================================================================
static bool CALLBACK findScenarioFiles(const BString& path, void* pParam)
{
   BDynamicSimArray<BSimString>* fileList = (BDynamicSimArray<BSimString>*)pParam;
   if(!fileList)
      return false;
   fileList->add(path);
   return true;
}

//==============================================================================
// BModeMenu::update
//==============================================================================
void BModeMenu::update()
{   
   BMode::update();

#ifdef ENABLE_GRAPHTEST_CODE   
   gGraphManager.updateRenderThread(0.1f);
#endif

   if (mpMainMenuScreen)
      mpMainMenuScreen->update(0);
   
   if(mNextState!=-1)
   {      
      if (mNextState == cStateIntroCinematicEnded)
      {
         // loadBackgroundMovie(cModeMenuMovieTypeDefault);
         mNextState=cStateAttractMode;
      }

      // if we are going into the attract mode, start the default movie.
      if (mNextState == cStateAttractMode)
      {
         loadBackgroundMovie(cModeMenuMovieTypeDefault);
      }


      if (mNextState == cStateSignIntoLive)
      {
         // if we are going into multiplayer and the system UI is up, then wait.
         if (gUserManager.isSystemUIShowing())
            return;

         const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
         BASSERT(pUser);

         if (!pUser->isSignedIntoLive())
         {
            // the UI is not up any more, and the user failed to sign in, stay in our current state.
            mNextState = -1;
            return;
         }

         if (mSignInGoldRequired && !pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
         {
            // the UI is not up any more, and the user failed to sign in, stay in our current state.
            mNextState = -1;
            return;
         }
         // We are logged in, set our next state and go to it.
         mNextState = mNextStateAfterSignin;
         return;
      }
      
      // if our next state is multiplayer, then force a sign-in
      // we'll want to display a dialog to the user alerting them that it's required
      // if they cancel the blade, then I want to revert to my previous state
      if (mNextState == cStateGotoMultiplayer)
      {
         // do a quick check on the user.
         const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
         BASSERT(pUser);
         if (!pUser->isSignedIntoLive())
         {
            // the UI is not up any more, and the user failed to sign in, stay in our current state.
            mNextState = -1;
            return;
         }
      } 

      if (mNextState == cStateInviteFromOtherController)
      {
         BUser* pUser = gUserManager.getPrimaryUser();
         if (pUser)
         {
            // switch out the user to the one that had the invite then go to the party room.
            gUserManager.setUserPort(BUserManager::cPrimaryUser, -1);

            // remove the primary user
            pUser->setFlagUserActive(false);

            // Mark them as signed out - because they are
            // NOTE: Many systems check the isSignedIn() method, but not the getFlagUserActive to see if a user is good to go
            // Before this change those system may have been using a player who they should not have
            // Love eric
            pUser->updateSigninStatus();

            long port = gLiveSystem->getInviteAcceptersPort();

            // now connect the new user.
            gUserManager.setUserPort(BUserManager::cPrimaryUser, port);
            pUser->setFlagUserActive(true);
            gUserManager.updateSigninByPort();
            mNextState = cStateGotoMultiplayer;
            return;
         }

         mNextState = cStateMain;
         return;
      }

      if (mNextState == cStateLoadError)
      {
         // SRL 11/07/08 - Our yorn system is simple and we had competing dialog popups. This one was first
         // and was being overwritten by another one when the MU was pulled. The overwrite caused the 
         // callback to never be called and subsequently left the UI hanging. So... we pulled the callback and
         // force the mode menu to the regular menu state.
         // exit was requested, throw up a yes/no dialog
         BUIGlobals* pUIGlobals = gGame.getUIGlobals();
         if (pUIGlobals && (!pUIGlobals->isYorNBoxVisible() || gSaveGame.getLoadDeviceRemove()))
         {
            if (gSaveGame.getLoadDeviceRemove())
            {
               gSaveGame.setLoadDeviceRemove(false);
               pUIGlobals->showYornBox(this, gDatabase.getLocStringFromID(26008), BUIGlobals::cDialogButtonsOK, -1, sEmptyUString, sEmptyUString, false, true);
            }
            else
               pUIGlobals->showYornBox(this, gDatabase.getLocStringFromID(25685), BUIGlobals::cDialogButtonsOK);
         }
         mNextState = cStateMain;
      }

      if (mNextState==cStateGotoCampaign)
      {
         // if we do have a valid save device, continue, otherwise prompt for it (done in check save device)
         if (gUserManager.getPrimaryUser()->isSignedIn() && !checkSaveDevice())
         {
            // We are signed in, but our current save game device is invalid or not selected, slide open the blade to choose one.
            gUserManager.showDeviceSelector( gUserManager.getPrimaryUser(), this, 0, 0, false /*forceShow*/ );
            mNextState = cStateMain;
         }
      }

      if (mNextState == cStateExit)
         mNextState = cStateReallyExit;

      // Music
      if (!gConfig.isDefined(cConfigNoMusic))
      {
         switch( mNextState )
         {
            case cStateServiceRecord:
            case cStateLeaderboards:
            case cStateSkirmishSetup:
               gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateSkirmishMenu);
               break;

            case cStateCreditsMovie:
               {
                  gSoundManager.playCue("Set_State_Credits");
                  gSoundManager.overrideBackgroundMusic(true);
               }
               break;

            case cStateAttractMode:
            case cStateMain:
               if(gSoundManager.getPregamePlaying() == false)
               {
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicStopInGame);
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicPlayPreGame);      
               }
               gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateMainTheme);
               break;
         }
      }

      if(mState==cStateSkirmish)
         gDatabase.unloadLeaderIcons();
      else if (mState==cStatePlayerSettings)
         gDatabase.unloadLeaderIcons();
      
      long lastState=mState;
      mState=mNextState;
      mNextState=-1;

      if( ((lastState == cStateServiceRecord) || (lastState==cStateOptions) || (lastState==cStateAttractMode) || (lastState == cStateLeaderboards) ) && mState != lastState )
      {
         if (mpUIScreen)
            delete mpUIScreen;
         mpUIScreen = NULL;
      }

      if( mState == cStateSkirmish && lastState == cStateMain )
      {
         BGameSettings* pSettings = gDatabase.getGameSettings();
         if( pSettings )
         {
            pSettings->setLong(BGameSettings::cPlayerCount, 2);
            mPlayerSettingDifficultyType = gUserManager.getPrimaryUser()->getOption_DefaultAISettings();
            getDifficultyValueForType( mPlayerSettingDifficultyType, mPlayerSettingDifficulty );
            pSettings->setLong( PSINDEX(2, BGameSettings::cPlayerDifficultyType), mPlayerSettingDifficultyType );
            pSettings->setFloat( PSINDEX(2, BGameSettings::cPlayerDifficulty), mPlayerSettingDifficulty );
            //TODO - HEY CAPPS! This is where you should set the player1's SkullBits from their profile
            // Love Eric
            pSettings->setLong( PSINDEX(1, BGameSettings::cPlayerSkullBits), 0 );
         }
      }

      //Reset presence to main menu - if this is NOT correct, one of the cases below will override it
      // (values are cached - not sent immediately)
      if (mState != cStateGotoGame)
      {
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_MAINMENU);
      }

      switch(mState)
      {        
         case cStateIntroCinematic:
            {
               //if (gConfig.isDefined(cConfigNoIntroCinematics))
               {
                  // loadBackgroundMovie(cModeMenuMovieTypeDefault);
                  mNextState=cStateAttractMode;
                  break;
               }

               /*
               BSimString introCinematic;
               if (gConfig.get(cConfigIntroCinematic, introCinematic))
               {
                  mIntroVideoMinTime=timeGetTime()+5000;        // 5 seconds later
                  mIntroVideoDone=false;
                 
                  // load up the start video
                  
                  BBinkInterface::BLoadParams lp;
                  lp.mFilename.set(introCinematic.getPtr());
                  lp.mCaptionDirID = cDirData;
                  lp.mpStatusCallback = this;
                  lp.mLoopVideo = false;
                  lp.mFullScreen = true;
                  gBinkInterface.registerValidCallback(this);
                  gBinkInterface.loadActiveVideo(lp);//gBinkInterface.loadActiveVideoFullScreen(introCinematic.getPtr(), cDirData, "", "", this, false);
               }
               else
               {
                  loadBackgroundMovie(cModeMenuMovieTypeDefault);
                  mNextState=cStateMain;
               }
               */
            }
            break;
            /*  Removing this hack so that it doesn't get re-used, the way it runs the movie is bad says Colt!
         case cStatePlayDemoMovie:
            {
               //E3 hack to play a movie from the main menu - stealing from the play intro movie code here - eric
               if (getSelectedFileName() &&
                   (mDemoVideoHandle == cInvalidVideoHandle))
               {
                  BSimString movieName = getSelectedFileName();
                  mIntroVideoMinTime=timeGetTime()+500;        // 1/2 a second later just to make sure the 'start' event doesn't kill it
                  mIntroVideoDone=false;
                  // load up the start video
                  BBinkInterface::BLoadParams lp;
                  lp.mFilename.set(movieName.getPtr());
                  lp.mCaptionDirID = cDirData;
                  lp.mpStatusCallback = this;
                  lp.mLoopVideo = false;
                  lp.mFullScreen = true;
                  gSoundManager.overrideBackgroundMusic(true);
                  if (mBackgoundVideoHandle != cInvalidVideoHandle)
                  {
                     gBinkInterface.stopVideo(mBackgoundVideoHandle);
                     mBackgoundVideoHandle = cInvalidVideoHandle;
                  }
                  mDemoVideoHandle = gBinkInterface.loadActiveVideo(lp);
               }
               else
               {
                  mNextState = cStateIntroCinematicEnded;
               }
               break;
            }
            */
         //case cStateInitialSignin:
         //   if (!gConfig.isDefined(cConfigForceAuth))
         //   {
         //      mNextState = cStateMain;
         //      break;
         //   }

         //   checkAlphaSignIn();  // next state is the authorizing state
         //   break;
         //case cStateAuthorizing:
         //   {
         //      BUIGlobals* pUIGlobals=gGame.getUIGlobals();
         //      BASSERT(pUIGlobals);
         //      if (pUIGlobals)
         //      {
         //         mMessageString.set(L"Authorizing... please wait...");
         //         pUIGlobals->showWaitDialog(mMessageString);
         //      }
         //      gLSPManager.checkAuth();
         //   }
         //   break;
         case cStateMain:
         {
            mpMainMenuScreen->populateMainMenu(mMenuItemFocus);
            if (mBackgoundVideoHandle==cInvalidVideoHandle)
               loadBackgroundMovie(cModeMenuMovieTypeDefault);
            break;
         }

         case cStateScenario:
         {
            if (gConfig.isDefined(cConfigAlpha))
               mScenarioFilter=1;
            else
               mScenarioFilter=0;
            updateScenarioList();
            if(mFileList.getNumber()==0)
               mState=lastState;
            break;
         }

         case cStateAttractMode:
            {
               if (!gConfig.isDefined(cConfigEnableAttractMode))
               {
                  BUser* pUser = gUserManager.getPrimaryUser();
                  //gUserManager.setUserPort(BUserManager::cPrimaryUser, 0);
                  pUser->setFlagUserActive(true);
                  gUserManager.updateSigninByPort();

                  mState=-1;
                  mNextState = cStateMain;
                  break;
               }

               if (mpUIScreen)
                  delete mpUIScreen;

               BUIAttractScreen* pScreen = new BUIAttractScreen();
               mpUIScreen = pScreen;
               mpUIScreen->init( "art\\ui\\flash\\pregame\\Attract\\UIAttractScreen.gfx", "art\\ui\\flash\\pregame\\Attract\\UIAttractScreenData.xml" );
               mpUIScreen->setHandler( this );
               mpUIScreen->setVisible( true );
               break;
            }
            break;

         case cStateTimeline:
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_TIMELINE);
            if (mpUIScreen)
               delete mpUIScreen;
            BUITimelineScreen* pScreen = new BUITimelineScreen();
            mpUIScreen = pScreen;
            mpUIScreen->init( "art\\ui\\flash\\pregame\\Timeline\\UITimelineScreen.gfx", "art\\ui\\flash\\pregame\\Timeline\\UITimelineScreenData.xml" );
            pScreen->populate();
            mpUIScreen->setHandler( this );
            mpUIScreen->setVisible( true );

            // turn on the special background movie for this screen.
            BSimString movie;
            gConfig.get(cConfigUITimelineScreenMovie, movie);

            loadBackgroundMovie(cModeMenuMovieTypeSpecific, movie.getPtr());

         	break;
		   }

         case cStateCreditsMovie:
            {
               // Fixme - Eric a new presence string
               //gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_TIMELINE);
               if (mpUIScreen)
                  delete mpUIScreen;
               BUICreditsMoviePlayer * pScreen = new BUICreditsMoviePlayer();
               mpUIScreen = pScreen;
               mpUIScreen->init("art\\ui\\flash\\pregame\\MoviePlayer\\UIMoviePlayerScreen.gfx", "art\\ui\\flash\\pregame\\MoviePlayer\\UIMoviePlayerScreenData.xml");

               mpUIScreen->setHandler( this );
               mpUIScreen->setVisible( true );

               // turn on the special background movie for this screen.
               BSimString movie;
               gConfig.get(cConfigCreditsMovie, movie);

               BSimString subs;
               gConfig.get(cConfigCreditsSubtitles, subs);
               loadBackgroundMovie(cModeMenuMovieTypeSpecific, movie.getPtr(), false, subs);
               break;
            }

			case cStateServiceRecord:
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SERVICERECORD);
            if (mpUIScreen)
               delete mpUIScreen;
            
            BUIServiceRecordScreen* pServiceScreen = new BUIServiceRecordScreen();                        
            pServiceScreen->init( "art\\ui\\flash\\pregame\\servicerecord\\uiservicerecord.gfx", "art\\ui\\flash\\pregame\\servicerecord\\uiservicerecord.xml" );
            pServiceScreen->setHandler( this );
            pServiceScreen->setVisible( true );
            pServiceScreen->populateFromUser(gUserManager.getPrimaryUser());
            mpUIScreen = pServiceScreen;
            break;
         }

         case cStateSkirmishSetup:
            {
               if (mpUIScreen)
                  delete mpUIScreen;
               mpUIScreen = new BUISkirmishSetupScreen();
               mpUIScreen->init("art\\ui\\flash\\pregame\\GameSetup\\GameSetupScreen.gfx", "art\\ui\\flash\\pregame\\GameSetup\\GameSetupScreenData.xml");
               mpUIScreen->setHandler( this );
               mpUIScreen->setVisible( true );
               break;
            }

         case cStateLeaderboards:
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_LEADERBOARDS);
            if (mpUIScreen)
               delete mpUIScreen;
            mpUIScreen = new BUILeaderboards();
            mpUIScreen->init( "art\\ui\\flash\\pregame\\leaderboards\\uileaderboards.gfx", "art\\ui\\flash\\pregame\\leaderboards\\uileaderboards.xml" );
            mpUIScreen->setHandler( this );
            mpUIScreen->setVisible( true );
            break;
         }

         case cStateOptions:
         {
            if (mpUIScreen)
               delete mpUIScreen;
            
            BUIOptionsMenu* optionsScreen = new BUIOptionsMenu();
            // fixme : Set the desired User
            optionsScreen->setUser( gUserManager.getPrimaryUser() );
            mpUIScreen = optionsScreen;
            mpUIScreen->init( "art\\ui\\flash\\pregame\\optionsMenu\\optionsMenu.gfx", "art\\ui\\flash\\pregame\\optionsMenu\\optionsMenuData.xml" );
            mpUIScreen->setHandler( this );
            mpUIScreen->setVisible( true );
            break;
         }

         case cStateFlash:
         {
            BSimString dirName;
            eFileManagerError result = gFileManager.getDirListEntry(dirName, cDirArt);
            BVERIFY(cFME_SUCCESS == result);
            dirName+="ui\\flash\\";

            mFileList.clear();
            gFileManager.findFiles(dirName, "*.swf", BFFILE_WANT_FILES|BFFILE_RECURSE_SUBDIRS, findScenarioFiles, &mFileList);
            if(mFileList.getNumber()>0)
            {
               BHandle fontHandle=gFontManager.getFontDenmark24();
               gFontManager.setFont(fontHandle);

               float yh=max(gFontManager.getLineHeight(), 0.001f);

               mList.setFont(fontHandle);
               mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
               mList.setRowSpacing(yh);
               mList.setColors(cColorWhite, cColorCyan);
               mList.setColumnWidth(gUI.mfSafeWidth/2.0f);
               mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-2);
               mList.setMultiColumn(true);
               mList.setMaxColumns(2);
               mList.setJustifyCenter(false);

               mList.clearItems();
               BSimString name;
               for(long i=0; i<mFileList.getNumber(); i++)
               {
                  strPathGetFilename(mFileList[i], name);
                  name.removeExtension();
                  mList.addItem(name);
               }
               mList.refresh();
               mList.setCurrentItem(mLastFlashItem);
            }
            else
               mState=lastState;
            break;
         }

         case cStateCinematic:
         {
            BSimString dirName;
            eFileManagerError result = gFileManager.getDirListEntry(dirName, cDirArt);
            BVERIFY(cFME_SUCCESS == result);
            dirName+="cinematic\\cutscenes\\";

            mFileList.clear();
            gFileManager.findFiles(dirName, "*.cin", BFFILE_WANT_FILES|BFFILE_RECURSE_SUBDIRS, findScenarioFiles, &mFileList);
            if(mFileList.getNumber()>0)
            {
               BHandle fontHandle=gFontManager.getFontDenmark24();
               gFontManager.setFont(fontHandle);

               float yh=gFontManager.getLineHeight();

               mList.setFont(fontHandle);
               mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
               mList.setRowSpacing(yh);
               mList.setColors(cColorWhite, cColorCyan);
               mList.setColumnWidth(gUI.mfSafeWidth/2.0f);
               mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-2);
               mList.setMultiColumn(true);
               mList.setMaxColumns(2);
               mList.setJustifyCenter(false);

               mList.clearItems();
               BSimString name;
               for(long i=0; i<mFileList.getNumber(); i++)
               {
                  strPathGetFilename(mFileList[i], name);
                  name.removeExtension();
                  mList.addItem(name);
               }
               mList.refresh();
               mList.setCurrentItem(mLastCinematicItem);
            }
            else
               mState=lastState;
            break;
         }

         case cStateModelView:
            {
               mFileList.clear();

               long count=gDatabase.getNumberProtoObjects();
               for(long i=0; i<count; i++)
               {
                  mFileList.add(gDatabase.getGenericProtoObject(i)->getName());
               }
               
               mFileList.sort();

               if(mFileList.getNumber()>0)
               {
                  BHandle fontHandle=gFontManager.getFontDenmark24();
                  gFontManager.setFont(fontHandle);

                  float yh=gFontManager.getLineHeight();

                  mList.setFont(fontHandle);
                  mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
                  mList.setRowSpacing(yh);
                  mList.setColors(cColorWhite, cColorCyan);
                  mList.setColumnWidth(gUI.mfSafeWidth/2.0f);
                  mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-2);
                  mList.setMultiColumn(true);
                  mList.setMaxColumns(2);
                  mList.setJustifyCenter(false);

                  mList.clearItems();
                  BSimString name;
                  for(long i=0; i<mFileList.getNumber(); i++)
                     mList.addItem(mFileList[i]);

                  mList.refresh();
                  mList.setCurrentItem(mLastModelViewItem);
               }
               else
                  mState=lastState;
               break;
            }
         case cStatePlayerSettings:
            {
               gDatabase.loadLeaderIcons();

               mCurrentPlayerSetting = cPlayerSettingTeam;

               // initialize the player settings
               BGameSettings* pSettings = gDatabase.getGameSettings();
               if(pSettings)
               {
                  pSettings->getLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerTeam), mPlayerSettingTeam);
                  pSettings->getFloat( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerDifficulty), mPlayerSettingDifficulty);
                  pSettings->getLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerDifficultyType), mPlayerSettingDifficultyType);
                  pSettings->getLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerCiv), mPlayerSettingCiv);
                  pSettings->getLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerLeader), mPlayerSettingLeader);
               }

               break;
            }
         case cStateSkirmish:
         {
            if (lastState==cStateMain)
            {
               gUserManager.getSecondaryUser()->setFlagUserActive(false);
               gUserManager.setUserPort(BUserManager::cSecondaryUser, -1);
            }

            gDatabase.loadLeaderIcons();

            mControlling=false;
            if (lastState != cStatePlayerSettings)
            {
               mSetting=cSettingMap;
               mSettingCurrentPlayer = -1;
            }
            mCurrentPlayerSetting = -1;

            BGameSettings* pSettings = gDatabase.getGameSettings();
            if(pSettings)
            {
               pSettings->getString(BGameSettings::cMapName, mSettingMap);
               pSettings->getLong(BGameSettings::cPlayerCount, mSettingPlayers);
               pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
               pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
               pSettings->getLong(BGameSettings::cGameMode, mSettingGameMode);
               pSettings->getBool(BGameSettings::cRecordGame, mSettingRecordGame);
            }

            mScenarioFilter=1;
            updateScenarioList();
            bool mapFound=false;
            for(long i=0; i<mFileList.getNumber(); i++)
            {
               if(mFileList[i]==mSettingMap)
               {
                  mList.setCurrentItem(i);
                  mapFound=true;
                  break;
               }
            }
            if(!mapFound)
            {
               if(mFileList.getNumber()>0)
               {
                  mList.setCurrentItem(0);
                  mSettingMap=mFileList[0];
               }
               else
                  mSettingMap="";
            }
            break;
         }

         case cStateRecordGame:
         case cStateSaveGame:
         {
            // refresh the list for
            if (mState == cStateRecordGame)
               gRecordGame.refresh(mRecordingsPort);
            else
               gSaveGame.refresh(mRecordingsPort);

            BHandle fontHandle=gFontManager.getFontDenmark14();
            gFontManager.setFont(fontHandle);

            float yh=gFontManager.getLineHeight();

            mList.setFont(fontHandle);
            mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
            mList.setRowSpacing(yh);
            mList.setColors(cColorWhite, cColorCyan);
            mList.setColumnWidth(gUI.mfSafeWidth);
            mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-17);
            mList.setMultiColumn(true);
            mList.setMaxColumns(1);
            mList.setJustifyCenter(false);

            mList.clearItems();

            mList.refresh();

            break;
         }

         case cStateGotoGame:
         case cStateGotoRecordGame:
         case cStateGotoSaveGame:
            break;

         default:
            mList.stop();
            break;
      }
   }
   else
   {
      switch(mState)
      {
         //case cStateAuthorizing:
         //   {
         //      if (gLSPManager.isAuthorizing())
         //         break;

         //      BUIGlobals* pUIGlobals = gGame.getUIGlobals();
         //      BASSERT(pUIGlobals);
         //      if (pUIGlobals)
         //         pUIGlobals->setWaitDialogVisible(false);

         //      if (gLSPManager.isBanned(0))
         //      {
         //         pUIGlobals->showYornBox(this, gDatabase.getLocStringFromID(24105), BUIGlobals::cDialogButtonsOK, cModeMenuYornAuthSigninFail);
         //         mNextState = cStateAuthorizationFailed;
         //      }

         //      mNextState = cStateMain;
         //   }
         //   break;

         case cStateMain : 
         case cStateScenario:
         case cStateFlash:
         case cStateCinematic:
         case cStateModelView:
         case cStateRecordGame:
         case cStateSaveGame:
            mList.update();
            break;
         case cStateCreditsMovie:
         case cStateTimeline:
         case cStateAttractMode:
         case cStateSkirmishSetup:
         case cStateServiceRecord:
         case cStateLeaderboards:
         case cStateOptions:
            BASSERT( mpUIScreen );
            if (mpUIScreen)
               mpUIScreen->update( 0.0f ); // fixme : get a real dt?
            break;

         case cStateSkirmish:
         case cStatePlayerSettings:
            controlUpdate();
            break;
         case cStateGotoViewer      : mNextState=cStateMain; mState=-1; gModeManager.setMode(BModeManager::cModeViewer); break;
         case cStateGotoCalibrate   : mNextState=cStateMain; mState=-1; gModeManager.setMode(BModeManager::cModeCalibrate); break;
         case cStateGotoFlash       : mNextState=cStateFlash; gModeManager.setMode(BModeManager::cModeFlash); break;
         case cStateGotoCinematic   : mNextState=cStateCinematic; gModeManager.setMode(BModeManager::cModeCinematic); break;
         case cStateGotoModelView   : mNextState=cStateModelView; gModeManager.setMode(BModeManager::cModeModelView); break;
         case cStateGotoGame        : mNextState=cStateMain; mState=-1; gModeManager.setMode(BModeManager::cModeGame); break;
         case cStateGotoRecordGame  : mNextState=cStateRecordGame; gModeManager.setMode(BModeManager::cModeGame); break;
         case cStateGotoSaveGame    : mNextState=cStateSaveGame; gModeManager.setMode(BModeManager::cModeGame); break;         
         case cStateGotoMultiplayer : 
            {
               mNextState=cStateMain; 
               mState=-1; 
               gModeManager.setMode(BModeManager::cModePartyRoom2); 
               break;
            }
         case cStateGotoSystemLink:
            {
               mNextState=cStateMain; 
               mState=-1; 
               BModePartyRoom2* pMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);
               if (!pMode)
                  break;
               gModeManager.setMode(BModeManager::cModePartyRoom2); 
               pMode->setUseLanMode(true);
               break;
            }
         case cStateGotoCampaign: 
            {
               mNextState=cStateMain; 
               mState=-1; 

               gModeManager.setMode(BModeManager::cModeCampaign2); 

               if (gGame.getUIGlobals() && gGame.getUIGlobals()->getWaitDialogVisible())
                  gGame.getUIGlobals()->setWaitDialogVisible(false);
               break;
            }         
         case cStateStartCampaign: 
            {
               mNextState=cStateMain; 
               mState=-1; 
               BCampaign * pCampaign = gCampaignManager.getCampaign(0);
               pCampaign->setPlayContinuous(true);
               pCampaign->setCurrentNodeID(0);
               pCampaign->launchGame(true);
               gModeManager.setMode(BModeManager::cModeCampaign2); 
               break;
            }         
         case cStateContinueCampaign: 
            {
               mNextState=cStateMain; 
               mState=-1; 
               BCampaign * pCampaign = gCampaignManager.getCampaign(0);
               pCampaign->setPlayContinuous(true);
               gModeManager.setMode(BModeManager::cModeCampaign2); 
               break;
            }         
         case cStateReallyExit: 
            {
               updateExit();
               break;
            }
      }
   }
}

//==============================================================================
// BModeMenu::updateExit
//==============================================================================
void BModeMenu::updateExit()
{
   // pull down the wait dialog
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BASSERT(pUIGlobals);
   if (pUIGlobals)
      pUIGlobals->setWaitDialogVisible(false);
      
   // time to exit the game.
   gGame.exit(); 
}


//==============================================================================
// BModeMenu::RenderVideos
//==============================================================================
void BModeMenu::renderVideos()
{
   gBinkInterface.decompressAllVideos();
   gBinkInterface.renderAllVideos();
   gBinkInterface.advanceAllVideos();
}

//==============================================================================
// BModeMenu::displayDebugInfo
//==============================================================================
void BModeMenu::displayDebugInfo()
{
#ifndef BUILD_FINAL         
   BHandle fontHandle;
   fontHandle=gFontManager.getFontDenmark16();
   gFontManager.setFont(fontHandle);
   float yh = gFontManager.getLineHeight();
   
   const uint xexChecksum = gXDBManager.getXEXChecksum();
   const uint xdbLoaded = gXDBManager.getInitialized() && gXDBManager.getXDBValid();
   uint allocLogging = 0;
#ifdef ALLOCATION_LOGGER
   allocLogging = getAllocationLogger().getInitialized();
#endif         
   BFixedString256 buf;
      
   float cx = 75.0f;
   float cy = 100.0f;
  
   
#ifdef USE_BUILD_INFO
   buf.format("DEPOT_REVISION: " DEPOT_REVISION);
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;

   buf.format("BUILD_NUMBER: " BUILD_NUMBER);
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;

   buf.format("CHANGELIST_NUMBER: " CHANGELIST_NUMBER);
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
#endif   

   MEMORYSTATUS stat;
   GlobalMemoryStatus(&stat);   
   
   buf.format("Total Allocated: %3.1fMB", (gInitialKernelMemoryTracker.getInitialPhysicalFree() - stat.dwAvailPhys) / (1024.0f * 1024.0f));
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
         
   buf.format("XFS: %s, Archives: %u, Loose Files: %u", strlen(gXFS.getServerIP()) ? gXFS.getServerIP() : "Not Active", gArchiveManager.getArchivesEnabled(), (gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles) != 0);
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
      
   buf.format("XEX CRC: 0x%08X, XDB Loaded: %u, Allocation Logging: %u", xexChecksum, xdbLoaded, allocLogging);
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;

#if 0  
   buf.format("BTexManager: %u, BD3DTexManager: %u, GrnInstances: %u, GrnModels: %u Loaded: %u, GrnAnims: %u Loaded: %u", 
      gTextureManager.getStats(cTRTStatic).mTotalLive,
      gD3DTextureManager.getTotalTextures(),
      gGrannyManager.getNumInstances(), 
      gGrannyManager.getNumModels(), gGrannyManager.getNumLoadedModels(),
      gGrannyManager.getNumAnimations(), gGrannyManager.getNumLoadedAnimations());
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
   
   buf.format("Particle DataSlots: %u, Instances: %u, TextureArrays: %u", gParticleGateway.getNumDataSlotsInUse(), gParticleGateway.getTotalActiveInstances(), gPSTextureManager.getNumTextureArrays());
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
   
   buf.format("LightEffects: %u, LightEffectInstances: %u, CameraEffects: %u", gLightEffectManager.getNumLightEffects(), gLightEffectManager.getNumLightEffectInstances(), gLightEffectManager.getNumCameraEffects());
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
   
   buf.format("VisualManager ProtoVisuals: %u, Visuals: %u", gVisualManager.getNumProtoVisuals(), gVisualManager.getNumVisuals());
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
   
   buf.format("Flash Data Slots: %u, Instances: %u", gFlashGateway.getNumDataSlotsInUse(), gFlashGateway.getNumInstances());
   gFontManager.drawText(fontHandle, cx, cy, buf.getPtr());
   cy += yh;
#endif   
#endif         
}


//==============================================================================
// BModeMenu::renderBegin
//==============================================================================
void BModeMenu::renderBegin()
{
   gRender.beginFrame(1.0f/30.0f);
   gRender.beginViewport(-1);

   gRenderDraw.beginScene();
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}


//==============================================================================
// BModeMenu::render
//==============================================================================
void BModeMenu::render()
{
   // renderBegin called just before this.
   /*
   if (mState == cStatePlayDemoMovie) 
   {
      renderVideos();
      return;
   }
   */
  
   BHandle fontHandle;
   fontHandle=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle);
   float yh=gFontManager.getLineHeight();

   renderVideos();

   renderSafeAreas();

   float sx=gUI.mfSafeX1;
   float sy=gUI.mfSafeY1;
   float by=gUI.mfSafeY2-yh;
   float ex=gUI.mfSafeX2;

   BHandle fontHandle2=gFontManager.getFontDenmark14();
   gFontManager.setFont(fontHandle2);
   float yh2=gFontManager.getLineHeight();

   BSimString builtString;
   builtString.format("Built: %s %s\n", __DATE__, __TIME__);

   BSimString gamerTag;
   const BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (user->isSignedIn())
      gamerTag = user->getName();
   else
      gamerTag = "<Not Signed In>";

   long renderState = mState;
   if (mState == cStateGotoGame)
   {
      if (mGotoGameFromState == cStateSkirmishSetup)
         renderState = cStateSkirmishSetup;
      else if (mGotoGameFromState == cStateMain)
         renderState = cStateMain;
   }

   switch(renderState)
   {
      case cStateMain:
      case cStateGotoCampaign:
      case cStateGotoMultiplayer:
      case cStateGotoSystemLink:
      {        
         mpMainMenuScreen->render();
         if (!gConfig.isDefined(cConfigNoUIDebug))
         {
            if(!gConfig.isDefined(cConfigDemo) && !gConfig.isDefined(cConfigDemo2))
            {
               gFontManager.drawText(fontHandle, ex, sy+cTitleOffset, builtString.getPtr(), cDWORDWhite, BFontManager2::cJustifyRight);
               gFontManager.drawText(fontHandle, ex, sy+cTitleOffset+yh, gamerTag.getPtr(), cDWORDWhite, BFontManager2::cJustifyRight);

               displayDebugInfo();
            }
         }
         break;
      }

      case cStateCreditsMovie:
	   case cStateTimeline:
      case cStateAttractMode:
      case cStateSkirmishSetup:
		case cStateServiceRecord:
      case cStateLeaderboards:
      case cStateOptions:
      {
         BASSERT( mpUIScreen );
         if (mpUIScreen)
            mpUIScreen->render();
         break;
      }

      case cStateScenario:
      {
         mList.render(0, 0);
         gFontManager.drawText(fontHandle, sx, by-yh, "A - Select", cDWORDGreen);
         gFontManager.drawText(fontHandle, sx, by, "B - Cancel", cDWORDRed);
         switch(mScenarioFilter)
         {
            case 0: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (All)", cDWORDYellow); break;
            case 1: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Playtest)", cDWORDYellow); break;
            case 2: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Development)", cDWORDYellow); break;
            default: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Unknown)", cDWORDYellow); break;
         }
         break;
      }

      case cStateFlash:
      case cStateCinematic:
      case cStateModelView:
         mList.render(0, 0);
         gFontManager.drawText(fontHandle, sx, by-yh, "A - Select", cDWORDGreen);
         gFontManager.drawText(fontHandle, sx, by, "B - Cancel", cDWORDRed);
         break;

      case cStateRecordGame:
      case cStateSaveGame:
         {
            const BSmallDynamicSimArray<BGameFileManifest*>& games = (mState == cStateRecordGame ? gRecordGame.getGameFiles() : gSaveGame.getGameFiles());

            if ((mState == cStateRecordGame && gRecordGame.getLastUpdate() != mLastRecordGameUpdate) || (mState == cStateSaveGame && gSaveGame.getLastUpdate() != mLastRecordGameUpdate))
            {
               mLastRecordGameUpdate = (mState == cStateRecordGame ? gRecordGame.getLastUpdate() : gSaveGame.getLastUpdate());

               // refresh the items in the list
               mList.clearItems();

               // FIXME-TCR, need to obey XPRIVILEGE_USER_CREATED_CONTENT and XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY
               // when attempting to display recordings/savegames from other people
               //
               // for example, if I downloaded a bunch of record/save games and then I turned on the privilege, we
               // need to display an X through the content informing them that it's restricted.
               uint count = games.getSize();
               for (uint i=0; i < count; ++i)
               {
//-- FIXING PREFIX BUG ID 3588
                  const BGameFileManifest* pManifest = games[i];
//--
                  if (pManifest)
                     mList.addItem(pManifest->mName);
               }

               mList.refresh();
            }
            mList.render(0, 0);
            if (mList.getCurrentItem() != -1)
            {
               BGameFileManifest* pManifest = NULL;
               if (mList.getCurrentItem() < games.getNumber())
                  pManifest = games[mList.getCurrentItem()];
               if (pManifest)
               {
                  DWORD dwColor = (pManifest->getFileError() ? cDWORDRed : cDWORDWhite);

                  float y = by - yh2;

                  BFixedString<128> text;

                  text.format("Filename: %s", pManifest->getFilename().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  text.format("Media ID: %I64u", pManifest->getID());
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  float fsize = static_cast<float>(pManifest->getFileSize());
                  text.format("Size: %.1f KB", fsize / 1024.0f);
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  float flen = pManifest->getLength();
                  uint32 len = static_cast<uint32>(flen * 1000.0f);
                  // convert the length from milliseconds to hour:min:sec
                  int hours = len / 3600000;
                  len -= (hours * 3600000);
                  int minutes = len / 60000;
                  len -= (minutes * 60000);
                  int seconds = len / 1000;

                  text.format("Length: %02d:%02d:%02d", hours, minutes, seconds);
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  text.format("Map Name: %s", pManifest->getMapName().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  switch (pManifest->getGameType())
                  {
                     case BGameSettings::cGameTypeSkirmish:
                        {
                           text.format("Game Type: Skirmish");
                           break;
                        }
                     case BGameSettings::cGameTypeCampaign:
                        {
                           text.format("Game Type: Campaign");
                           break;
                        }
                     case BGameSettings::cGameTypeScenario:
                        {
                           text.format("Game Type: Scenario");
                           break;
                        }
                     default:
                        {
                           text.format("Game Type: Unknown");
                           break;
                        }
                  }
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  switch (pManifest->getMapType())
                  {
                     case BScenarioMap::cScenarioTypeFinal:
                        {
                           text.format("Map Type: Final");
                           break;
                        }
                     case BScenarioMap::cScenarioTypePlaytest:
                        {
                           text.format("Map Type: Playtest");
                           break;
                        }
                     case BScenarioMap::cScenarioTypeDevelopment:
                        {
                           text.format("Map Type: Development");
                           break;
                        }
                     case BScenarioMap::cScenarioTypeCampaign:
                        {
                           text.format("Map Type: Campaign");
                           break;
                        }
                     default:
                        {
                           text.format("Map Type: Unknown");
                           break;
                        }
                  }
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  BUString utext;
                  utext.format(L"Scenario Name: %s", pManifest->getScenarioName().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, utext, dwColor);
                  y -= yh2;

                  text.format("Players: %s", pManifest->getPlayers().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  utext.format(L"Description: %s", pManifest->getDesc().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, utext, dwColor);
                  y -= yh2;

                  utext.format(L"Name: %s", pManifest->getName().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, utext, dwColor);
                  y -= yh2;

                  const SYSTEMTIME& date = pManifest->getLocalTime();
                  text.format("Date: %04d/%02d/%02d %02d:%02d:%02d", date.wYear, date.wMonth, date.wDay, date.wHour, date.wMinute, date.wSecond);
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  text.format("Owner: %s", pManifest->getAuthor().getPtr());
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  y -= yh2;

                  switch (pManifest->getStorageType())
                  {
                     case eStorageCache:
                        text.format("Location: Cache (HDD)");
                        break;
                     case eStorageHDD:
                        text.format("Location: XFS/HDD");
                        break;
                     case eStorageUser:
                        text.format("Location: User Storage (HDD/MU)");
                        break;
                     case eStorageServer:
                        text.format("Location: Server");
                        break;
                  }
                  gFontManager.drawText(fontHandle2, sx, y, text, dwColor);

                  if (pManifest->isTransferring())
                  {
                     y -= yh2;
                     if (pManifest->getStorageType() == eStorageServer)
                     {
                        // downloading
                        text.format("Downloading: %d%%", pManifest->getPercentTransferred());
                     }
                     else
                     {
                        // uploading
                        text.format("Uploading: %d%%", pManifest->getPercentTransferred());
                     }
                     gFontManager.drawText(fontHandle2, sx, y, text, dwColor);
                  }

                  // you can only edit record/save games that have been saved to User Storage
                  if (!pManifest->getFileError())
                  {
                     if (pManifest->getStorageType() != eStorageServer)
                        gFontManager.drawText(fontHandle, sx, by, "A - Select", cDWORDGreen);

                     if (pManifest->canEdit())
                        gFontManager.drawText(fontHandle, sx + 180.0f, by, "X - Edit", cDWORDBlue);
                     // any file that exists in user storage and also on some other storage medium is considered "Saved"
                     else if (pManifest->isSaved())
                        gFontManager.drawText(fontHandle, sx + 180.0f, by, "- Saved -", cDWORDBlue);
                     // any file that exists somewhere other than User Storage can be saved
                     else if (pManifest->getStorageType() != eStorageServer)
                        gFontManager.drawText(fontHandle, sx + 180.0f, by, "X - Save", cDWORDBlue);

                     if (mState == cStateRecordGame)
                     {
                        // if we can edit the item, then it's in user storage, so we're free to also upload the item
                        // provided they're the owner
                        if (!gLSPManager.isInNoLSPMode())
                        {
                           if (pManifest->canEdit())
                              gFontManager.drawText(fontHandle, sx + 540.0f, by, "Y - Upload", cDWORDCyan);
                           else if (pManifest->getStorageType() == eStorageServer)
                              gFontManager.drawText(fontHandle, sx + 540.0f, by, "Y - Dnload", cDWORDCyan);
                        }
                     }
                  }
                  else
                  {
                     gFontManager.drawText(fontHandle, sx, by, "ERROR", cDWORDRed);
                  }

                  if (pManifest->getStorageType() != eStorageHDD)
                     gFontManager.drawText(fontHandle, sx + 720.0f, by, "RB - Delete", cDWORDDarkGrey);
               }
            }
            gFontManager.drawText(fontHandle, sx + 360.0f, by, "B - Cancel", cDWORDRed);
//-- FIXING PREFIX BUG ID 3589
            const BUser* pUser = gUserManager.getUserByPort(mRecordingsPort);
//--
            if (pUser && !pUser->isSignedIn())
               gFontManager.drawText(fontHandle2, sx + 720.0f, by - yh2, "- Sign in for more -", cDWORDRed);
            break;
         }

      case cStateSkirmish:
      {
//-- FIXING PREFIX BUG ID 3594
         const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
         if(!pSettings)
            break;


         float y=sy;
         BFixedString<64> text;
         gFontManager.drawText(fontHandle, sx, y, "Skirmish");
         y+=yh;

         text.format("Players: %d", mSettingPlayers);
         gFontManager.drawText(fontHandle, sx, y, text, (mSetting==cSettingPlayers?cDWORDCyan:cDWORDWhite));
         y+=yh;

         if(mList.getCurrentItem()!=-1)
            text.format("Map: %s", mList.getItem(mList.getCurrentItem()).getPtr());
         else
            text.format("Map: None");
         gFontManager.drawText(fontHandle, sx, y, text, (mSetting==cSettingMap?cDWORDCyan:cDWORDWhite));
         y+=yh;

//-- FIXING PREFIX BUG ID 3595
         const BGameMode* pGameMode = gDatabase.getGameModeByID(mSettingGameMode);
//--
         text.format("Game Mode: %S", (pGameMode ? pGameMode->getDisplayName().getPtr() : L"Unknown"));
         gFontManager.drawText(fontHandle, sx, y, text, (mSetting==cSettingGameMode?cDWORDCyan:cDWORDWhite));
         y+=yh;

         if(mSettingRecordGame)
            text.format("Record Game: On");
         else
            text.format("Record Game: Off");
         gFontManager.drawText(fontHandle, sx, y, text, (mSetting==cSettingRecordGame?cDWORDCyan:cDWORDWhite));
         y+=yh;

         // print the data off for each of the players
		   y+=yh;         // skip another line
         long count;
         pSettings->getLong(BGameSettings::cPlayerCount, count);

         BSimString gamerTag;
         const BUser * const user = gUserManager.getUser(BUserManager::cPrimaryUser);
         if (user->isSignedIn())
            gamerTag = user->getName();
         else
            gamerTag = "<Not Signed In>";

         BSimString gamerTag2;
         if (gGame.isSplitScreenAvailable() && gUserManager.isSecondaryUserAvailable(false))
         {
            const BUser * const user2 = gUserManager.getUser(BUserManager::cSecondaryUser);
            if (user2->getFlagUserActive())
            {
               if (user2->isSignedIn())
                  gamerTag2 = user2->getName();
               else
                  gamerTag2 = "<Not Signed In>";
            }
            else
               gamerTag2 = "<Press A to Join>";
         }
         else
            gamerTag2 = "AI";

         for(long index=1; index<count+1; index++)
         {
            BSimString playerName;
            if (index == 1)
               playerName = gamerTag;
            else if (index == 2)
               playerName = gamerTag2;
            else
               playerName = "AI";

            const BColor& lineColor=(((mSetting==cSettingPlayer1)&&(index==mSettingCurrentPlayer)) ? cColorCyan : cColorWhite);

            // leader, team, difficulty, civ
            long leader;
            long team;
            float diff;
            long civ;
            BDifficultyType diffType;
            pSettings->getLong( PSINDEX(index, BGameSettings::cPlayerLeader), leader);
            pSettings->getLong( PSINDEX(index, BGameSettings::cPlayerCiv), civ);
            pSettings->getLong( PSINDEX(index, BGameSettings::cPlayerTeam), team);
            pSettings->getFloat( PSINDEX(index, BGameSettings::cPlayerDifficulty), diff);
            pSettings->getLong( PSINDEX(index, BGameSettings::cPlayerDifficultyType), diffType);

            gFontManager.drawText(fontHandle, sx, y, playerName.getPtr(), lineColor.asDWORD());

            // print it out
            if(team==0)
               text.format("?");
            else
               text.format("%d", team);
            gFontManager.drawText(fontHandle, sx+300.0f, y, text.getPtr(), lineColor.asDWORD());

//-- FIXING PREFIX BUG ID 3593
            const BCiv* pCiv=(civ==0 ? NULL : gDatabase.getCiv(civ));
//--
            gFontManager.drawText(fontHandle, sx+400.0f, y, (pCiv ? pCiv->getDisplayName().getPtr() : L"?"), lineColor.asDWORD());

            if(leader!=-1)
            {
//-- FIXING PREFIX BUG ID 3591
               const BLeader* pLeader=gDatabase.getLeader(leader);
//--
               if(pLeader)
               {
                  if(pLeader->mIconHandle!=cInvalidManagedTextureHandle)
                     gUI.renderTexture(pLeader->mIconHandle, (long)sx+600, (long)y, ((long)sx+600)+32, ((long)y)+32);
               }
            }

            text.format("?");
            if(leader!=-1)
            {
//-- FIXING PREFIX BUG ID 3592
               const BLeader* pLeader=gDatabase.getLeader(leader);
//--
               if(pLeader)
                  text.format("%S", gDatabase.getLocStringFromIndex(pLeader->mNameIndex).getPtr());
            }
            gFontManager.drawText(fontHandle, sx+668.0f, y, text, lineColor.asDWORD());

            y+=yh;
         }

         y=by-yh;
         gFontManager.drawText(fontHandle, sx, y, "A - Start", cDWORDGreen);
         if (mSetting == cSettingPlayer1)
         {
            gFontManager.drawText(fontHandle, sx+200.0f, y, "X - Player Settings", cDWORDBlue);
         }
         y+=yh;
         gFontManager.drawText(fontHandle, sx, y, "B - Cancel", cDWORDRed);
         y+=yh;


         switch(mScenarioFilter)
         {
            case 0: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (All)", cDWORDYellow); break;
            case 1: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Playtest)", cDWORDYellow); break;
            case 2: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Development)", cDWORDYellow); break;
            default: gFontManager.drawText(fontHandle, sx+200.0f, by, "Y - Map Filter (Unknown)", cDWORDYellow); break;
         }

         break;
      }

      case cStatePlayerSettings:
         {
            // are we displaying a player?
            if (mSetting != cSettingPlayer1)
               break;

//-- FIXING PREFIX BUG ID 3600
            const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
            if(!pSettings)
               break;

            // Title
            float y=sy;
            BFixedString<64> text;


            BSimString playerName;
            if (mSettingCurrentPlayer == 1)
            {
               BSimString gamerTag;
               if (user->isSignedIn())
                  playerName = user->getName();
               else
                  playerName = "<Not Signed In>";
            }
            else
               playerName.format("Player %d", mSettingCurrentPlayer);

            gFontManager.drawText(fontHandle, sx, y, playerName.getPtr(), cDWORDGreen);
            y+=yh;


            // Get Player Settings
            long leader = mPlayerSettingLeader;
            long team = mPlayerSettingTeam;
            long civ = mPlayerSettingCiv;

            // Display Team
            if(team==0)
               text.format("Team: ?");
            else
               text.format("Team: %d", team);
            gFontManager.drawText(fontHandle, sx, y, text, (mCurrentPlayerSetting==cPlayerSettingTeam?cDWORDCyan:cDWORDWhite));
            y+=yh;

   
            // Marchack todo loc this...
            BSimUString textU;
            textU.set(L"");
            if (mPlayerSettingDifficultyType == DifficultyType::cEasy)
               textU.format(L"Difficulty: Easy (%1.2f)", mPlayerSettingDifficulty);
            else if (mPlayerSettingDifficultyType == DifficultyType::cNormal)
               textU.format(L"Difficulty: Normal (%1.2f)", mPlayerSettingDifficulty);
            else if (mPlayerSettingDifficultyType == DifficultyType::cHard)
               textU.format(L"Difficulty: Hard (%1.2f)", mPlayerSettingDifficulty);
            else if (mPlayerSettingDifficultyType == DifficultyType::cLegendary)
               textU.format(L"Difficulty: Legendary (%1.2f)", mPlayerSettingDifficulty);
            else if (mPlayerSettingDifficultyType == DifficultyType::cCustom)
               textU.format(L"Difficulty: Custom (%1.2f)", mPlayerSettingDifficulty);
            else if (mPlayerSettingDifficultyType == DifficultyType::cAutomatic)
               textU.format(L"Difficulty: Automatic (%1.2f)", mPlayerSettingDifficulty);
            else
               textU.format(L"Difficulty: Default (%1.2f)", gDatabase.getDifficultyDefault());

            gFontManager.drawText(fontHandle, sx, y, textU.getPtr(), (mCurrentPlayerSetting==cPlayerSettingDifficulty?cDWORDCyan:cDWORDWhite));
            y+=yh;

            // Display Civ
            BUString civName;
            if(civ==0)
               civName.format(L"Civ: ?");
            else
               civName.format(L"Civ: %s", gDatabase.getCiv(civ)->getDisplayName().getPtr());
            gFontManager.drawText(fontHandle, sx, y, civName.getPtr(), (mCurrentPlayerSetting==cPlayerSettingCiv?cDWORDCyan:cDWORDWhite));
            y+=yh;

            // Display Leader
            text.format("Leader: ?");
            if(leader!=-1)
            {
//-- FIXING PREFIX BUG ID 3598
               const BLeader* pLeader=gDatabase.getLeader(leader);
//--
               if(pLeader)
                  text.format("Leader: %S", gDatabase.getLocStringFromIndex(pLeader->mNameIndex).getPtr());
            }
            gFontManager.drawText(fontHandle, sx, y, text.getPtr(), (mCurrentPlayerSetting==cPlayerSettingLeader?cDWORDCyan:cDWORDWhite));
            y+=yh;

            // Display Leader Details
            if(leader!=-1)
            {
//-- FIXING PREFIX BUG ID 3599
               const BLeader* pLeader=gDatabase.getLeader(leader);
//--
               if(pLeader)
               {
                  if(pLeader->mIconHandle!=cInvalidManagedTextureHandle)
                     gUI.renderTexture(pLeader->mIconHandle, (long)sx, (long)y, ((long)sx)+63, ((long)y)+63);
                  BSmallDynamicSimArray<BUString> lines;
                  uint lineCount=gUI.wordWrapText(gDatabase.getLocStringFromIndex(pLeader->mDescriptionIndex), 10, lines);
                  for(uint i=0; i<lineCount; i++)
                  {
                     gFontManager.drawText(fontHandle, sx+68, y, lines[i].getPtr());
                     y+=yh;
                  }
               }
            }

            y=by-yh;
            gFontManager.drawText(fontHandle, sx, y, "A - Accept", cDWORDGreen);
            y+=yh;
            gFontManager.drawText(fontHandle, sx, y, "B - Cancel", cDWORDRed);
            y+=yh;
         }

         break;

      case cStateGotoGame:
         if (!gConfig.isDefined(cConfigDemo) && !gConfig.isDefined(cConfigDemo2) && !gConfig.isDefined(cConfigEnableBackgroundPlayer))
         {
            float y=sy+cTitleOffset;
            BSimString text;

            gFontManager.drawText(fontHandle, sx, y, "Starting Game...");
            y+=yh;
            y+=yh;

            text="";
            BUString infoText;
            long curItem=mList.getCurrentItem();
            if (curItem>=0 && curItem<mFileList.getNumber())
            {
               text=mList.getItem(curItem);
               if (mScenarioFilter==1)
               {
                  if (mFileInfoIDList[curItem]!=0)
                     infoText=gDatabase.getLocStringFromID(mFileInfoIDList[curItem]);
               }
            }
            if (!text.isEmpty())
               gFontManager.drawText(fontHandle, sx, y, text.getPtr());
            y+=yh;
            y+=yh;

            if (!infoText.isEmpty())
            {
               BSmallDynamicSimArray<BUString> infoLines;
               gUI.wordWrapText(infoText, 10, infoLines);
               for (uint i=0; i<infoLines.getSize(); i++)
               {
                  gFontManager.drawText(fontHandle, sx, y, infoLines[i].getPtr());
                  y+=yh;
               }
            }
         }
         break;

      case cStateGotoRecordGame:
         gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Starting Playback...");
         break;

      case cStateGotoSaveGame:
         gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Loading Game...");
         break;

      case cStateGotoViewer:
         gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Starting Viewer...");
         break;

      case cStateGotoCalibrate:
         gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Starting Display Calibration...");
         break;        
      
      case cStateGotoFlash:
         gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Starting Flash Viewer...");
         break;        
      
      case cStateReallyExit:
         if (!gConfig.isDefined(cConfigDemo) && !gConfig.isDefined(cConfigDemo2))
            gFontManager.drawText(fontHandle, sx, sy+cTitleOffset, "Exiting...");
         break;
   }

      gFontManager.render2D();

   #ifndef BUILD_FINAL
      gConsoleRender.renderSubmit();
   #endif

#ifdef ENABLE_GRAPHTEST_CODE
   gGraphManager.simRenderGraphs();
#endif

   // renderEnd called after this.
}

//==============================================================================
// BModeMenu::renderEnd
//==============================================================================
void BModeMenu::renderEnd()
{
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);
   
   gRender.endViewport();
   gRender.endFrame();

   gEventDispatcher.sleep(16);
}

//==============================================================================
// BModeMenu::handleInput
//==============================================================================
bool BModeMenu::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; detail;

   // ajl 11/18/08 - Don't handle input while we are going to the campaign menu. This function can get called
   // while we are creating the save file since that is done in the background.
   if (mState == cStateGotoCampaign || gUserManager.getShowingDeviceSelector())
      return true;

//-- FIXING PREFIX BUG ID 3605
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (mState != cStateSkirmishSetup)
   {
      if (port != pUser->getPort())
      {
         bool allowInput = false;
         if ( (mState==cStateMain) || (mState==cStateAttractMode) || (mState==cStateIntroCinematic) )
            allowInput = true;

         if (gGame.isSplitScreenAvailable())
         {
            BUser* pUser2 = gUserManager.getSecondaryUser();
            if (mState == cStateSkirmish && event==cInputEventControlStart)
            {
               if (pUser2->getFlagUserActive())
               {
                  if (controlType == cButtonB)
                  {
                     gUI.playClickSound();
                     gUserManager.setUserPort(BUserManager::cSecondaryUser, -1);
                     pUser2->setFlagUserActive(false);
                     return true;
                  }
                  else if (controlType == cButtonX)
                  {
                     mSetting = cSettingPlayer1;
                     mSettingCurrentPlayer = 2;
                     allowInput = true;
                  }
               }
               else
               {
                  if (controlType == cButtonA)
                  {
                     gUI.playClickSound();
                     gUserManager.setUserPort(BUserManager::cSecondaryUser, port);
                     pUser2->setFlagUserActive(true);

                     // Force secondary user to be on the same team for now
                     BGameSettings* pSettings = gDatabase.getGameSettings();
                     if (pSettings)
                     {
                        long team=1;
                        pSettings->getLong(BGameSettings::cPlayer1Team, team);
                        pSettings->setLong(BGameSettings::cPlayer2Team, team);
                     }
                     return true;
                  }
               }
            }
            else if (mState == cStatePlayerSettings)
            {
               if (pUser2->getFlagUserActive() && mSettingCurrentPlayer==2)
                  allowInput=true;
            }
         }
         if (!allowInput)
            return false;
      }
   }

   switch(mState)
   {
      case cStateMain:
         {
            mRecordingsPort = port;
            bool handled = false;

            handled=mpMainMenuScreen->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);

            return handled;
         }
         break;

      case cStateCreditsMovie:
	   case cStateTimeline:
      case cStateAttractMode:
      case cStateSkirmishSetup:
      case cStateServiceRecord:
      case cStateLeaderboards:
      case cStateOptions:
         {
            BASSERT( mpUIScreen );
            if (mpUIScreen)
               return mpUIScreen->handleInput( port, (BInputEventType)event, (BInputControlType)controlType, detail );

            return false;
         }

      case cStateIntroCinematic:
         {
            DWORD currentTime = timeGetTime();
            if (mIntroVideoDone || (mIntroVideoMinTime>currentTime))
               break;
            if(event==cInputEventControlStart)
            {
               switch(controlType)
               {
                  case cButtonA:
                  case cButtonX:
                  case cButtonY:
                  case cButtonB:
                  case cButtonStart:
                  case cButtonBack:
                     gBinkInterface.stopAllVideos(true);
                     mIntroVideoDone=true;   // let the onVideoEnded load the background video
                     // mNextState=cStateMain;
               }
            }
         }
         break;
         /*
      case cStatePlayDemoMovie:
         {
            DWORD currentTime = timeGetTime();
            if (mIntroVideoDone || (mIntroVideoMinTime>currentTime))
               break;
            if(event==cInputEventControlStart)
            {
               switch(controlType)
               {
               case cButtonA:
               case cButtonX:
               case cButtonY:
               case cButtonB:
               case cButtonStart:
               case cButtonBack:
                  mIntroVideoDone=true;   // let the onVideoEnded load the background video
                  gBinkInterface.stopAllVideos();                  
               }
            }
         }
         */
      case cStateScenario:
      case cStateFlash:
      case cStateCinematic:
      case cStateModelView:
      case cStateRecordGame:
      case cStateSaveGame:
         if(event==cInputEventControlStart)
         {
            switch(controlType)
            {
               case cButtonA:
                  gUI.playClickSound();
                  if(mState==cStateScenario)
                  {
                     mLastScenarioItem=mList.getCurrentItem();
                     if(mLastScenarioItem!=-1)
                     {
                        gDatabase.resetGameSettings();
                        BGameSettings* pSettings = gDatabase.getGameSettings();
                        if(pSettings)
                        {
                           BSimString gameID;
                           MVince_CreateGameID(gameID);

                           BSimString mapName=mFileList[mLastScenarioItem];
                           
//-- FIXING PREFIX BUG ID 3612
                           const BUser* pUser=gUserManager.getPrimaryUser();
//--

                           pSettings->setLong(BGameSettings::cPlayerCount, 2);
                           pSettings->setString(BGameSettings::cMapName, mapName);
                           pSettings->setLong(BGameSettings::cMapIndex, -1);
                           pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
                           pSettings->setString(BGameSettings::cGameID, gameID);
                           pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
                           pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
                           pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
                           pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
                           pSettings->setLong(BGameSettings::cGameMode, 0);
                           pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

                           mSelectedFileName=mapName;

                           setNextState(cStateGotoGame);
                        }
                     }
                  }
                  else if(mState==cStateFlash)
                  {
                     mLastFlashItem=mList.getCurrentItem();
                     if(mLastFlashItem!=-1)
                     {
                        BSimString dirName;
                        eFileManagerError result = gFileManager.getDirListEntry(dirName, cDirProduction);
                        BVERIFY(cFME_SUCCESS == result);
                        long dirLen=dirName.length();

                        mSelectedFileName=mFileList[mLastFlashItem];
                        mSelectedFileName.remove(0, dirLen);

                        mNextState=cStateGotoFlash;
                     }
                  }
                  else if(mState==cStateCinematic)
                  {
                     mLastCinematicItem=mList.getCurrentItem();
                     if(mLastCinematicItem!=-1)
                     {
                        BSimString dirName;
                        eFileManagerError result = gFileManager.getDirListEntry(dirName, cDirProduction);
                        BVERIFY(cFME_SUCCESS == result);
                        long dirLen=dirName.length();

                        mSelectedFileName=mFileList[mLastCinematicItem];
                        mSelectedFileName.remove(0, dirLen);



                        gDatabase.resetGameSettings();
                        BGameSettings* pSettings = gDatabase.getGameSettings();
                        if(pSettings)
                        {
                           BSimString gameID;
                           MVince_CreateGameID(gameID);

                           pSettings->setLong(BGameSettings::cPlayerCount, 2);
                           pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
                           pSettings->setString(BGameSettings::cGameID, gameID);
                           pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
                           pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
                           pSettings->setLong(BGameSettings::cGameMode, 0);
                           pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
                        }


                        mNextState=cStateGotoCinematic;
                     }
                  }
                  else if(mState==cStateModelView)
                  {
                     mLastModelViewItem=mList.getCurrentItem();
                     if(mLastModelViewItem!=-1)
                     {
                        mSelectedFileName=mFileList[mLastModelViewItem];

                        gDatabase.resetGameSettings();
                        BGameSettings* pSettings = gDatabase.getGameSettings();
                        if(pSettings)
                        {
                           BSimString gameID;
                           MVince_CreateGameID(gameID);

                           pSettings->setLong(BGameSettings::cPlayerCount, 2);
                           pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
                           pSettings->setString(BGameSettings::cGameID, gameID);
                           pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
                           pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
                           pSettings->setLong(BGameSettings::cGameMode, 0);
                           pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
                        }


                        mNextState=cStateGotoModelView;
                     }
                  }
                  else if (mState==cStateRecordGame)
                  {
                     if (mList.getCurrentItem() != -1)
                     {
                        gDatabase.resetGameSettings();
                        BGameSettings* pSettings = gDatabase.getGameSettings();
                        if (pSettings)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& recGames = gRecordGame.getGameFiles();
                           if (mList.getCurrentItem() < recGames.getNumber())
                           {
//-- FIXING PREFIX BUG ID 3606
                              const BGameFileManifest* pManifest = recGames[mList.getCurrentItem()];
//--
                              if (!pManifest || pManifest->getFileError())
                              {
                                 // refresh the record games
                                 mNextState = cStateRecordGame;
                                 return true;
                              }

                              if (pManifest->getStorageType() != eStorageServer)
                              {
                                 // Convert the manifest temp ID into a string so we can add it to the game settings.
                                 // Provides backwards compatibility for auto-loading record games from the config by name.
                                 BString tempID;
                                 tempID.format("%d", pManifest->getTempID());
                                 pSettings->setString(BGameSettings::cLoadName, tempID.getPtr());
                                 pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeRecord);

                                 mNextState = cStateGotoRecordGame;
                              }
                           }
                           else
                           {
                              // refresh the record games
                              mNextState = cStateRecordGame;
                           }
                        }
                     }
                  }
                  else if (mState==cStateSaveGame)
                  {
                     if (mList.getCurrentItem() != -1)
                     {
                        gDatabase.resetGameSettings();
                        BGameSettings* pSettings = gDatabase.getGameSettings();
                        if (pSettings)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& saveGames = gSaveGame.getGameFiles();
                           if (mList.getCurrentItem() < saveGames.getNumber())
                           {
//-- FIXING PREFIX BUG ID 3607
                              const BGameFileManifest* pManifest = saveGames[mList.getCurrentItem()];
//--
                              if (!pManifest || pManifest->getFileError())
                              {
                                 // refresh the save games
                                 mNextState = cStateSaveGame;
                                 return true;
                              }

                              if (pManifest->getStorageType() != eStorageServer)
                              {
                                 // Convert the manifest temp ID into a string so we can add it to the game settings.
                                 // Provides backwards compatibility for auto-loading save games from the config by name.
                                 BString tempID;
                                 tempID.format("%d", pManifest->getTempID());
                                 pSettings->setString(BGameSettings::cLoadName, tempID.getPtr());
                                 pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);

                                 mNextState = cStateGotoSaveGame;
                              }
                           }
                           else
                           {
                              // refresh the save games
                              mNextState = cStateSaveGame;
                           }
                        }
                     }
                  }
                  return true;

               case cButtonB:
                  if(mState==cStateScenario || mState==cStateFlash || mState==cStateCinematic || mState==cStateModelView)
                  {
                     mLastScenarioItem=0;
                     mLastFlashItem=0;
                     mLastCinematicItem=0;
                     mLastModelViewItem=0;
                     gUI.playClickSound();
                     mNextState=cStateMain;
/*
                     mNextState=cStateOther;
                     mMenuItemFocus=mState;           // key for the menu focus
*/
                  }
                  else if (mState == cStateRecordGame || mState == cStateSaveGame)
                  {
                     mLastScenarioItem=0;
                     mLastFlashItem=0;
                     mLastCinematicItem=0;
                     mLastModelViewItem=0;
                     gUI.playClickSound();
                     mNextState=cStateMain;
/*
                     if (gConfig.isDefined(cConfigAlpha))
                        mNextState=cStateOther;
                     else
                        mNextState=cStateExtras;
                     mMenuItemFocus=mState;           // key for the menu focus
*/
                  }
                  return true;

               case cButtonX:
                  {
                     if (mState == cStateRecordGame)
                     {
                        // save/edit the currently selected record game
                        if (mList.getCurrentItem() != -1)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& recGames = gRecordGame.getGameFiles();
                           if (mList.getCurrentItem() < recGames.getNumber())
                           {
//-- FIXING PREFIX BUG ID 3608
                              const BGameFileManifest* pManifest = recGames[mList.getCurrentItem()];
//--
                              if (pManifest && !pManifest->getFileError())
                              {
                                 if (!pManifest->isSaved())
                                    gRecordGame.saveFile(port, pManifest->getTempID());
                                 else if (pManifest->canEdit())
                                    gRecordGame.editFile(port, pManifest->getTempID(), BRecordGame::cEditDesc);
                              }
                           }
                        }
                     }
                     else if (mState == cStateSaveGame)
                     {
                        // save/edit the currently selected save game
                        if (mList.getCurrentItem() != -1)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& saveGames = gSaveGame.getGameFiles();
                           if (mList.getCurrentItem() < saveGames.getNumber())
                           {
//-- FIXING PREFIX BUG ID 3609
                              const BGameFileManifest* pManifest = saveGames[mList.getCurrentItem()];
//--
                              if (pManifest && !pManifest->getFileError())
                              {
                                 if (!pManifest->isSaved())
                                    gSaveGame.saveFile(port, pManifest->getTempID());
                                 else if (pManifest->canEdit())
                                    gSaveGame.editFile(port, pManifest->getTempID(), BSaveGame::cEditDesc);
                              }
                           }
                        }
                     }
                     return true;
                  }

               case cButtonY:
                  {
                     if (mState==cStateScenario)
                     {
                        gUI.playClickSound();
                        if(mScenarioFilter==1)
                           mScenarioFilter=0;
                        else
                           mScenarioFilter++;
                        updateScenarioList();
                     }
                     else if (mState == cStateMain)
                     {

                     }
                     else if (mState == cStateRecordGame)
                     {
                        // upload the currently selected record game, if we can
                        if (mList.getCurrentItem() != -1)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& recGames = gRecordGame.getGameFiles();
                           if (mList.getCurrentItem() < recGames.getNumber())
                           {
                              BGameFileManifest* pManifest = recGames[mList.getCurrentItem()];
                              if (pManifest && !pManifest->getFileError())
                              {
                                 if (pManifest->canEdit())
                                    gRecordGame.uploadFile(port, pManifest->getTempID());
                                 else if (pManifest->getStorageType() == eStorageServer)
                                    gRecordGame.downloadFile(port, pManifest->getTempID());
                              }
                           }
                        }
                     }
                     return true;
                  }

               case cButtonShoulderRight:
                  {
                     if (mState == cStateRecordGame)
                     {
                        if (mList.getCurrentItem() != -1)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& recGames = gRecordGame.getGameFiles();
                           if (mList.getCurrentItem() < recGames.getNumber())
                           {
                              BGameFileManifest* pManifest = recGames[mList.getCurrentItem()];
                              if (pManifest)
                                 gRecordGame.deleteFile(port, pManifest->getTempID());
                           }
                        }
                     }
                     else if (mState == cStateSaveGame)
                     {
                        if (mList.getCurrentItem() != -1)
                        {
                           const BSmallDynamicSimArray<BGameFileManifest*>& saveGames = gSaveGame.getGameFiles();
                           if (mList.getCurrentItem() < saveGames.getNumber())
                           {
                              BGameFileManifest* pManifest = saveGames[mList.getCurrentItem()];
                              if (pManifest)
                                 gSaveGame.deleteFile(port, pManifest->getTempID());
                           }
                        }
                     }

                     break;
                  }
            }
         }

         if (mState == cStateRecordGame)
            mRecordingsPort = port;

         if(mList.handleInput(port, event, controlType, detail))
            return true;
         break;

      case cStateSkirmish:
         if(event==cInputEventControlStart)
         {
            switch(controlType)
            {
               case cButtonA : 
               {
                  gUI.playClickSound(); 

                  BUser* pUser = gUserManager.getPrimaryUser();
//-- FIXING PREFIX BUG ID 3611
                  const BUser* pUser2 = gUserManager.getSecondaryUser();
//--

                  //gDatabase.resetGameSettings();
                  BGameSettings* pSettings = gDatabase.getGameSettings();
                  if(pSettings)
                  {
                     BSimString gameID;
                     MVince_CreateGameID(gameID);

                     pSettings->setLong(BGameSettings::cPlayerCount, mSettingPlayers);
                     pSettings->setString(BGameSettings::cMapName, mSettingMap);
                     pSettings->setLong(BGameSettings::cMapIndex, -1);
                     pSettings->setBool(BGameSettings::cRecordGame, mSettingRecordGame);
                     pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
                     pSettings->setString(BGameSettings::cGameID, gameID);
                     pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
                     pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
                     pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
                     pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
                     pSettings->setLong(BGameSettings::cGameMode, mSettingGameMode);
                     pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

                     // set up human vs computer players
                     for(long index=1; index<mSettingPlayers+1; index++)
                     {
                        long playerType = (index==1)?BGameSettings::cPlayerHuman : BGameSettings::cPlayerComputer;

                        // for AI testing
                        #ifndef LTCG
                        if (gConfig.isDefined(cConfigPlayer1AI))
                           playerType = BGameSettings::cPlayerComputer;
                        #endif

                        pSettings->setLong( PSINDEX(index, BGameSettings::cPlayerType), playerType);
                     }

                     if (gGame.isSplitScreenAvailable() && pUser2->getFlagUserActive())
                     {
                        pSettings->setString(BGameSettings::cPlayer2Name, pUser2->getName());
                        pSettings->setUInt64(BGameSettings::cPlayer2XUID, pUser2->getXuid());
                        pSettings->setLong(BGameSettings::cPlayer2Type, BGameSettings::cPlayerHuman);
                     }
                  }

                  setNextState(cStateGotoGame);
                  return true;
               }

               case cButtonX:
                  // only when a player is highlighted
                  if (mSetting != cSettingPlayer1)
                  {
                     mSetting = cSettingPlayer1;
                     mSettingCurrentPlayer = 1;
                  }
                  gUI.playClickSound();
                  mNextState=cStatePlayerSettings;
                  return true;

               case cButtonB: 
                  gUI.playClickSound(); 
                  gUserManager.getSecondaryUser()->setFlagUserActive(false);
                  mNextState=cStateMain;
                  return true;

               case cButtonY:
               {
                  gUI.playClickSound();
                  if(mScenarioFilter==1)
                     mScenarioFilter=0;
                  else
                     mScenarioFilter++;
                  updateScenarioList();
                  return true;
               }

               case cDpadLeft:
               case cStickLeftLeft:
                  if(!mControlling)
                     controlStart(-2);
                  return true;

               case cDpadRight:
               case cStickLeftRight:
                  if(!mControlling)
                     controlStart(2);
                  return true;

               case cDpadDown:
               case cStickLeftDown:
                  if(!mControlling)
                     controlStart(1);
                  return true;

               case cDpadUp:
               case cStickLeftUp:
                  if(!mControlling)
                     controlStart(-1);
                  return true;
            }
         }
         else if(event==cInputEventControlStop)
         {
            switch(controlType)
            {
               case cDpadDown:
               case cDpadUp:
               case cStickLeftDown:
               case cStickLeftUp:
                  if(mControlling)
                     controlStop();
                  return true;

               case cDpadLeft:
               case cDpadRight:
               case cStickLeftLeft:
               case cStickLeftRight:
                  if(mControlling)
                     controlStop();
                  return true;
            }
         }
         break;

      case cStatePlayerSettings:
         if(event==cInputEventControlStart)
         {
            switch(controlType)
            {
            case cButtonA : 
               {
                  gUI.playClickSound(); 

                  BGameSettings* pSettings = gDatabase.getGameSettings();
                  if(pSettings)
                  {
                     // push these settings into the game settings
                     pSettings->setLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerTeam), mPlayerSettingTeam);

                     pSettings->setFloat( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerDifficulty), mPlayerSettingDifficulty);
                     pSettings->setLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerDifficultyType), mPlayerSettingDifficultyType);
                     pSettings->setLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerCiv), mPlayerSettingCiv);
                     pSettings->setLong( PSINDEX(mSettingCurrentPlayer, BGameSettings::cPlayerLeader), mPlayerSettingLeader);
                  }

                  mNextState=cStateSkirmish;
                  return true;
               }

            case cButtonB: 
               gUI.playClickSound(); 
               mNextState=cStateSkirmish;
               return true;

            case cDpadLeft:
            case cStickLeftLeft:
               if(!mControlling)
                  controlStart(-2);
               return true;

            case cDpadRight:
            case cStickLeftRight:
               if(!mControlling)
                  controlStart(2);
               return true;

            case cDpadDown:
            case cStickLeftDown:
               if(!mControlling)
                  controlStart(1);
               return true;

            case cDpadUp:
            case cStickLeftUp:
               if(!mControlling)
                  controlStart(-1);
               return true;

            case cStickRightUp:
               if(!mControlling)
                  controlStart(3);
               return true;

            case cStickRightDown:
               if(!mControlling)
                  controlStart(-3);
               return true;
            }
         }
         else if(event==cInputEventControlStop)
         {
            switch(controlType)
            {
            case cDpadDown:
            case cDpadUp:
            case cStickLeftDown:
            case cStickLeftUp:
            case cStickRightUp:
            case cStickRightDown:
               if(mControlling)
                  controlStop();
               return true;

            case cDpadLeft:
            case cDpadRight:
            case cStickLeftLeft:
            case cStickLeftRight:
               if(mControlling)
                  controlStop();
               return true;
            }
         }
         break;

   }

   return false;
}

//==============================================================================
// BModeMenu::controlStart
//==============================================================================
void BModeMenu::controlStart(long direction)
{
   mControlling=true;
   mControlDirection=direction;
   mControlTime=0.0f;
   mControlFast=false;
   controlChangeSelection();
}

//==============================================================================
// BModeMenu::controlStop
//==============================================================================
void BModeMenu::controlStop()
{
   mControlling=false;
   mControlDirection=0;
}

//==============================================================================
// BModeMenu::controlUpdate
//==============================================================================
void BModeMenu::controlUpdate()
{
   if(!mControlling)
      return;

   DWORD time=timeGetTime();
   float elapsedTime=(time-mLastTime)*0.001f;
   if(elapsedTime>0.1f)
      elapsedTime=0.1f;
   mLastTime=time;

   if(mControlDirection==0)
      return;

   mControlTime+=elapsedTime;
   if(mControlFast)
   {
      if(mControlTime>=cFastNextSelectionTime)
      {
         controlChangeSelection();
         mControlTime=0.0f;
      }
   }
   else
   {
      if(mControlTime>=cFirstNextSelectionTime)
      {
         controlChangeSelection();
         mControlTime=0.0f;
         mControlFast=true;
      }
   }
}

//==============================================================================
// BModeMenu::controlChangeSelection
//==============================================================================
void BModeMenu::controlChangeSelection()
{
   if (mState == cStateSkirmish)
      controlChangeSkirmishGameSettingsSelection();
   else if (mState == cStatePlayerSettings)
      controlChangePlayerSettingsSelection();
}

//==============================================================================
// BModeMenu::controlChangePlayerSettingsSelection
//==============================================================================
void BModeMenu::controlChangePlayerSettingsSelection()
{
   if(mControlDirection==1)
   {
      // wrap
      if(mCurrentPlayerSetting==cPlayerSettingLeader)
         mCurrentPlayerSetting=cPlayerSettingTeam;
      else
         mCurrentPlayerSetting++;

      gUI.playRolloverSound();
   }
   else if(mControlDirection==-1)
   {
      if(mCurrentPlayerSetting==cPlayerSettingTeam)
         mCurrentPlayerSetting=cPlayerSettingLeader;
      else
         mCurrentPlayerSetting--;

      gUI.playRolloverSound();
   }
   else if(mControlDirection==2)
   {
      switch(mCurrentPlayerSetting)
      {
         case cPlayerSettingTeam:
            mPlayerSettingTeam++;
            if (mPlayerSettingTeam>2)
               mPlayerSettingTeam=0;
            gUI.playRolloverSound();
            break;

         case cPlayerSettingDifficulty:
            if( gUserManager.getPrimaryUser()->getOption_DefaultAISettings() != DifficultyType::cAutomatic )
            {
               // Change the difficulty type and clamp to valid types
               mPlayerSettingDifficultyType++;
               if (mPlayerSettingDifficultyType > DifficultyType::cCustom)
                  mPlayerSettingDifficultyType = DifficultyType::cEasy;

               getDifficultyValueForType( mPlayerSettingDifficultyType, mPlayerSettingDifficulty );

               gUI.playRolloverSound();      
            }
            break;

         case cPlayerSettingCiv:
            if(mPlayerSettingCiv==gDatabase.getNumberCivs()-1)
               mPlayerSettingCiv=0;
            else
               mPlayerSettingCiv++;
            
            clearLeaders();
            gUI.playRolloverSound();
            break;

         case cPlayerSettingLeader:
            pickNextLeader(&mPlayerSettingLeader, 1);
            gUI.playRolloverSound();
            break;
      }
   }
   else if(mControlDirection==-2)
   {

      switch(mCurrentPlayerSetting)
      {
         case cPlayerSettingTeam:
            mPlayerSettingTeam--;
            if(mPlayerSettingTeam<0)
               mPlayerSettingTeam=2;
            gUI.playRolloverSound();
         break;

      case cPlayerSettingDifficulty:
         if( gUserManager.getPrimaryUser()->getOption_DefaultAISettings() != DifficultyType::cAutomatic )
         {
            // Change the difficulty type and clamp to valid types
            mPlayerSettingDifficultyType--;
            if (mPlayerSettingDifficultyType < DifficultyType::cEasy)
               mPlayerSettingDifficultyType = DifficultyType::cCustom;

            // Set the difficulty setting from the type
            getDifficultyValueForType( mPlayerSettingDifficultyType, mPlayerSettingDifficulty );
            
            gUI.playRolloverSound();      
         }
         break;

      case cPlayerSettingCiv:
         mPlayerSettingCiv--;
         if(mPlayerSettingCiv<0)
            mPlayerSettingCiv=gDatabase.getNumberCivs()-1;
         clearLeaders();
         gUI.playRolloverSound();
         break;

      case cPlayerSettingLeader:
         pickNextLeader(&mPlayerSettingLeader, -1);
         gUI.playRolloverSound();
         break;
      }
   }
   else if(mControlDirection==3 && mCurrentPlayerSetting == cPlayerSettingDifficulty && mPlayerSettingDifficultyType == DifficultyType::cCustom)
   {
      mPlayerSettingDifficulty += 0.01f;
      if(mPlayerSettingDifficulty > 1.0f)
         mPlayerSettingDifficulty = 1.0f;
   }
   else if(mControlDirection==-3 && mCurrentPlayerSetting == cPlayerSettingDifficulty && mPlayerSettingDifficultyType == DifficultyType::cCustom)
   {
      mPlayerSettingDifficulty -= 0.01f;
      if(mPlayerSettingDifficulty < 0.0f)
         mPlayerSettingDifficulty = 0.0f;
   }
}



//==============================================================================
// BModeMenu::controlChangeSkirmishGameSettingsSelection
//==============================================================================
void BModeMenu::controlChangeSkirmishGameSettingsSelection()
{
   BGameSettings* pSettings = gDatabase.getGameSettings();
   BASSERT(pSettings);
   if(mControlDirection==1)
   {
      if(mSetting==cSettingPlayer1)
      {
         if (mSettingCurrentPlayer == mSettingPlayers)
            mSetting=cSettingPlayers;
         else
            mSettingCurrentPlayer++;
      }
      else 
      {
         mSetting++;
         if (mSetting == cSettingPlayer1)
            mSettingCurrentPlayer = 1;
      }
      gUI.playRolloverSound();
   }
   else if(mControlDirection==-1)
   {
      if(mSetting==cSettingPlayers)
      {
         mSetting=cSettingPlayer1;
         mSettingCurrentPlayer = mSettingPlayers;
      }
      else
      {
         if ( (mSetting == cSettingPlayer1) && (mSettingCurrentPlayer != 1) )
            mSettingCurrentPlayer--;
         else
            mSetting--;

      }
      gUI.playRolloverSound();
   }
   else if(mControlDirection==2)
   {
      switch(mSetting)
      {
         case cSettingPlayers:
            if(mSettingPlayers<cMaximumSupportedMultiplayers)
            {
               gUI.playClickSound();
               mSettingPlayers++;
               pSettings->setLong(BGameSettings::cPlayerCount, mSettingPlayers);

               // Set the new AI's difficulty to the player's chosen default
               uint8 defaultAIDifficulty = gUserManager.getPrimaryUser()->getOption_DefaultAISettings();
               float diffValue = 0.0f;
               getDifficultyValueForType( defaultAIDifficulty, diffValue );
               pSettings->setLong( PSINDEX(mSettingPlayers, BGameSettings::cPlayerDifficultyType), (long)defaultAIDifficulty );
               pSettings->setFloat( PSINDEX(mSettingPlayers, BGameSettings::cPlayerDifficulty), diffValue );
            }
            else
               gUI.playCantDoSound();
            break;

         case cSettingMap:
            {
               long curItem=mList.getCurrentItem();
               long numItems=mList.getNumberItems();
               if(curItem<numItems-1)
                  curItem++;
               else
                  curItem=0;
               mList.setCurrentItem(curItem);
               if(curItem==-1)
                  mSettingMap="";
               else
                  mSettingMap=mFileList[curItem];
               pSettings->setString(BGameSettings::cMapName, mSettingMap);
               gUI.playRolloverSound();
               break;
            }

         case cSettingGameMode:
            if(mSettingGameMode<gDatabase.getNumberGameModes()-1)
               mSettingGameMode++;
            else
               mSettingGameMode=0;
            pSettings->setLong(BGameSettings::cGameMode, mSettingGameMode);
            gUI.playRolloverSound();
            break;

         case cSettingRecordGame:
            mSettingRecordGame=!mSettingRecordGame;
            pSettings->setBool(BGameSettings::cRecordGame,mSettingRecordGame);
            gUI.playRolloverSound();
            break;
      }
   }
   else if(mControlDirection==-2)
   {
      switch(mSetting)
      {
         case cSettingPlayers:
            if(mSettingPlayers>1)
            {
               gUI.playClickSound();
               mSettingPlayers--;
               pSettings->setLong(BGameSettings::cPlayerCount, mSettingPlayers);
            }
            else
               gUI.playCantDoSound();
            break;

         case cSettingMap:
            {
               long curItem=mList.getCurrentItem();
               long numItems=mList.getNumberItems();
               if(curItem>0)
                  curItem--;
               else
                  curItem=numItems-1;
               mList.setCurrentItem(curItem);
               if(curItem==-1)
                  mSettingMap="";
               else
                  mSettingMap=mFileList[curItem];
               pSettings->setString(BGameSettings::cMapName, mSettingMap);
               gUI.playRolloverSound();
               break;
            }

         case cSettingGameMode:
            if(mSettingGameMode>0)
               mSettingGameMode--;
            else
               mSettingGameMode=gDatabase.getNumberGameModes()-1;
            pSettings->setLong(BGameSettings::cGameMode, mSettingGameMode);
            gUI.playRolloverSound();
            break;

         case cSettingRecordGame:
            mSettingRecordGame=!mSettingRecordGame;
            pSettings->setBool(BGameSettings::cRecordGame,mSettingRecordGame);
            gUI.playRolloverSound();
            break;
      }
   }
}

//==============================================================================
// BModeMenu::signIn
//==============================================================================
void BModeMenu::signIn()
{
   // sign-in UI will always display if needed

   const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BASSERT(pUser);

   // if the current user is not signed into LIVE or a gold account is required and they do not have the MP privilege
   // then show the sign-in UI
   if (!pUser->isSignedIntoLive() || (mSignInGoldRequired && !pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS)))
   {
      gUserManager.showSignInUI();

      mSignInRequested = TRUE;
   }
}

//==============================================================================
// BModeMenu::clearLeaders
//==============================================================================
void BModeMenu::clearLeaders()
{
   mPlayerSettingLeader=-1;
}

//==============================================================================
// BModeMenu::pickNextLeader
//==============================================================================
void BModeMenu::pickNextLeader(long* pLeaderID, long dir)
{
   long id=*pLeaderID;
   long numLeaders=gDatabase.getNumberLeaders();
   for(;;)
   {
      if(dir==1)
      {
         if(id==numLeaders-1)
            id=-1;
         else
            id++;
      }
      else
      {
         if(id==-1)
            id=numLeaders-1;
         else
            id--;
      }
      if(id==-1 || id==*pLeaderID)
      {
         *pLeaderID=-1;
         return;
      }
//-- FIXING PREFIX BUG ID 3614
      const BLeader* pLeader=gDatabase.getLeader(id);
//--
      if(pLeader && pLeader->mLeaderCivID==mPlayerSettingCiv && (!pLeader->mTest || gConfig.isDefined(cConfigUseTestLeaders)))
      {
         *pLeaderID=id;
         return;
      }
   }
}

//==============================================================================
// BModeMenu::updateScenarioList
//==============================================================================
void BModeMenu::updateScenarioList()
{
   mFileList.clear();
   mFileNameIDList.clear();
   mFileInfoIDList.clear();
   mList.clearItems();

   if(mScenarioFilter==1 || mScenarioFilter==2)
   {
      BXMLReader reader;
      if(reader.load(cDirData, "scenariodescriptions.xml"))
      {
         BSimString tutorialMap;
         if (gConfig.isDefined(cConfigDemo))
         {
            gConfig.get(cConfigTutorialMap, tutorialMap);
            tutorialMap+=".scn";
         }

         BXMLNode rootNode(reader.getRootNode());
         long nodeCount=rootNode.getNumberChildren();
         BSimString file;
         for(long i=0; i<nodeCount; i++)
         {
            BXMLNode node(rootNode.getChild(i));
            const BPackedString name(node.getName());
            if(name=="ScenarioInfo")
            {
               BSimString typeStr, fileStr;
               const bool hasType = node.getAttribValue("Type", &typeStr);
               const bool hasFile = node.getAttribValue("File", &fileStr);

               if (gConfig.isDefined(cConfigDemo))
               {
                  if(fileStr==tutorialMap)
                     continue;
               }

               int nameStringID=0;
               int infoStringID=0;
               node.getAttribValueAsInt("NameStringID", nameStringID);
               node.getAttribValueAsInt("InfoStringID", infoStringID);

               if(hasType && hasFile)
               {
                  switch(mScenarioFilter)
                  {
                     case 1:
                     {
                        if(typeStr!="Playtest")
                           continue;
                        break;
                     }
                     case 2:
                        if(typeStr!="Development")
                           continue;
                        break;
                  }
                  file=fileStr;
                  file.removeExtension();
                  mFileList.add(file);
                  mFileNameIDList.add(nameStringID);
                  mFileInfoIDList.add(infoStringID);
                  
                  // RG [10/08/06] - Changed this to ignore the "Name" node because it can be invalid/misleading
                  BSimString name;
                  strPathGetFilename (file, name);
                  
                  BUString displayName;
                  if (nameStringID!=0)
                     displayName=gDatabase.getLocStringFromID(nameStringID);
                  if (displayName.isEmpty())
                     mList.addItem(name);
                  else
                  {
                     BSimString text;
                     text.format("%S (%s)", displayName.getPtr(), name.getPtr());
                     mList.addItem(text);
                  }
               }
            }
         }
      }
   }
   else
   {
      BSimString dirName;
      eFileManagerError result = gFileManager.getDirListEntry(dirName, cDirScenario);
      BVERIFY(cFME_SUCCESS == result);
      long dirLen=dirName.length();

      mFileList.clear();
      mFileNameIDList.clear();
      mFileInfoIDList.clear();
      gFileManager.findFiles(dirName, "*.scn", BFFILE_WANT_FILES|BFFILE_RECURSE_SUBDIRS|BFFILE_TRY_XMB, findScenarioFiles, &mFileList);

      if (gArchiveManager.getArchivesEnabled())
      {
         // rg [8/10/07] - This is kinda a hack, the scenario mode doesn't work unless there's at least one entry.
         mFileList.pushBack("n/a");
         mFileNameIDList.add(0);
         mFileInfoIDList.add(0);
      }
      
      BSimString file;
      //BSimString name;
      for(long i=0; i<mFileList.getNumber(); i++)
      {
         //strPathGetFilename(mFileList[i], name);
            
         //name.removeExtension();
         //mList.addItem(name);

         file=mFileList[i];
         file.remove(0, dirLen);
         
         file.removeExtension();
         mFileList[i]=file;

         mFileNameIDList.add(0);
         mFileInfoIDList.add(0);
         mList.addItem(file);
      }
   }

   if(mFileList.getNumber()>0)
   {
      BHandle fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);

      float yh=gFontManager.getLineHeight();

      mList.setFont(fontHandle);
      mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
      mList.setRowSpacing(yh);
      mList.setColors(cColorWhite, cColorCyan);
      mList.setColumnWidth(gUI.mfSafeWidth);
      mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-2);
      mList.setMultiColumn(true);
      mList.setMaxColumns(1);
      mList.setJustifyCenter(false);

      mList.refresh();
      mList.setCurrentItem(0);
   }
}

//==============================================================================
// BModeMenu::onVideoEnded, BBinkVideoStatus interface method
//==============================================================================
void BModeMenu::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   ASSERT_MAIN_THREAD

   // If for some reason we got preloaded data back (not expected here at the moment), delete it since otherwise
   // it would be orphaned.
   if(preloadedData)
   {
      delete preloadedData;
      preloadedData = NULL;
   }
      
   // clean up our handle
   if (mBackgoundVideoHandle == handle)
      mBackgoundVideoHandle = cInvalidVideoHandle;

   if (mState == cStateCreditsMovie)
      mNextState = cStateMain;
   else if (mState == cStateIntroCinematic)
      mNextState = cStateIntroCinematicEnded;
}

//==============================================================================
//==============================================================================
void BModeMenu::setNextState(long state)
{ 
   mNextState=state;

   if (state == cStateGotoGame)
      mGotoGameFromState = mState;
}

//==============================================================================
//==============================================================================
void BModeMenu::setNextStateWithLiveCheck(long nextState, bool requireGold)
{
   const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BASSERT(pUser);

   BUIGlobals* puiGlobals = gGame.getUIGlobals();

   mSignInGoldRequired = requireGold;

   // 24058 An Xbox LIVE Gold Membership or permission for Online Gameplay is required. Press $$A$$ to sign into an appropriate gamer profile.
   // 25546 An Xbox LIVE gamer profile is required. Press $$A$$ to sign into an appropriate gamer profile.
   // 25566 Your gamer profile is not signed into Xbox LIVE. Press $$A$$ to sign-in now.

   if (!pUser->isSignedIn())
   {
      // show the yorn box to ask if they want to sign in.
      if (puiGlobals)
      {
         mNextStateAfterSignin = nextState;
         if (requireGold)
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornLiveSignin);
         else
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25546), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornLiveSignin);
      }

      return;
   }

   // if the user is signed-in, they still may not be signed into LIVE
   
   // if they are LIVE enabled but not signed into LIVE
   if (pUser->isLiveEnabled() && !pUser->isSignedIntoLive())
   {
      mNextStateAfterSignin = nextState;
      puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25566), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornLiveSignin);
      return;
   }
   else if (!pUser->isLiveEnabled())
   {
      // if they're not LIVE enabled, ask them to choose a different profile

      //If their current account is not ENABLED for Live access at all (either by priv or by being Silver) then ask them if they want to switch
      mNextStateAfterSignin = nextState;
      puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornLiveSignin);
      return;
   }

   // otherwise the user is signed into LIVE by default

   // if we don't require a Gold gamer profile or they have the MP privilege, then set the next state and return
   if (!requireGold || pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
   {
      mNextState = nextState;
      return;
   }

   // otherwise we require a Gold gamer profile and they lack the MP privilege
   // either because it's a Silver profile or parental controls

   mNextStateAfterSignin = nextState;
   puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornLiveSignin);
   return;
}

//==============================================================================
//==============================================================================
void BModeMenu::yornResult(uint result, DWORD userContext, int port)
{
   switch (userContext)
   {
      case cModeMenuYornChooseDeviceNone:
         gModeManager.setMode(BModeManager::cModeCampaign2); 
         break;
      case cModeMenuYornChooseDevice:
         {
            switch(result)
            {
            case BUIGlobals::cDialogResultOK:
               gModeManager.setMode(BModeManager::cModeCampaign2); 
               break;
            case BUIGlobals::cDialogResultCancel:
               // show the device selector here, forcing it to pop, starting the cycle over.
               gUserManager.showDeviceSelector( gUserManager.getPrimaryUser(), this, 0, 0, false /*forceShow*/ );

               // gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(25986), BUIGlobals::cDialogButtonsOK, cModeMenuYornChooseDeviceNone);
               break;
            }
         }
         break;
      case cModeMenuYornLiveSignin:
         {
            switch(result)
            {
               case BUIGlobals::cDialogResultOK:
                  signIn();
                  mNextState = cStateSignIntoLive;
                  break;
               case BUIGlobals::cDialogResultCancel:
                  break;
            }
         }
         break;
      //case cModeMenuYornAuthSignin:
      //   {
      //      switch(result)
      //      {
      //         case BUIGlobals::cDialogResultOK:
      //            signIn();
      //            mNextState = cStateInitialSignin;
      //            break;
      //         case BUIGlobals::cDialogResultCancel:
      //            mNextState = cStateReallyExit;
      //            break;
      //      }
      //   }
      //   break;
      //case cModeMenuYornAuthSigninFail:
      //   {
      //      mNextState = cStateReallyExit;
      //   }
      //   break;
      case cModeMenuYornExit:
         {
            switch (result)
            {
               case BUIGlobals::cDialogResultOK:
                  mNextState = cStateReallyExit;
                  break;
            }
         }
         break;
      case cModeMenuOkayLoadError:
      {
         mNextState = cStateMain;
         BGameSettings* pSettings = gDatabase.getGameSettings();
         if (pSettings)
         {
            BFixedStringMaxPath name;
            pSettings->getString(BGameSettings::cLoadName, name, MAX_PATH);
            if (name == "campaign" || name == "campaign.sav")
            {
               gCampaignManager.getCampaign(0)->setPlayContinuous(false);
               mNextState = cStateGotoCampaign;
            }
         }
         break;
      }
   }

}

//==============================================================================
//==============================================================================
void BModeMenu::handleUIScreenResult( BUIScreen* pScreen, long result )
{
   if( pScreen == mpUIScreen )
   {
      if ( (pScreen->getScreenName() == "UITimelineScreen") || 
           (pScreen->getScreenName() == "UIAttractScreen") || 
           (pScreen->getScreenName() == "UICreditsMoviePlayer") )
      {
         gSoundManager.overrideBackgroundMusic(false);
         loadBackgroundMovie(cModeMenuMovieTypeCampaign);
      }

      setNextState( BModeMenu::cStateMain );
   }
}

//==============================================================================
//==============================================================================
void BModeMenu::startTutorial()
{
   // tutorial
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {
      BSimString gameID;
      MVince_CreateGameID(gameID);

      BSimString mapName;
      gConfig.get(cConfigTutorialMap, mapName);
      if (mapName.isEmpty())
         return;

//-- FIXING PREFIX BUG ID 3618
      const BUser* pUser=gUserManager.getPrimaryUser();
//--

      pSettings->setLong(BGameSettings::cPlayerCount, 4);
      pSettings->setString(BGameSettings::cMapName, mapName);
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);

      pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
      pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
      pSettings->setLong(BGameSettings::cPlayer1Team, 1);
      pSettings->setLong(BGameSettings::cPlayer1Civ, 1);
      pSettings->setLong(BGameSettings::cPlayer1Leader, 1);
      pSettings->setLong(BGameSettings::cPlayer1Type, BGameSettings::cPlayerHuman);

      pSettings->setString(BGameSettings::cPlayer2Name, "Jimmy McAlly");
      pSettings->setUInt64(BGameSettings::cPlayer2XUID, 0);
      pSettings->setLong(BGameSettings::cPlayer2Team, 1);
      pSettings->setLong(BGameSettings::cPlayer2Civ, 1);
      pSettings->setLong(BGameSettings::cPlayer2Leader, 1);
      pSettings->setLong(BGameSettings::cPlayer2Type, BGameSettings::cPlayerComputer);
      pSettings->setFloat(BGameSettings::cPlayer2Difficulty, gDatabase.getDifficultyEasy());
      pSettings->setLong(BGameSettings::cPlayer2DifficultyType, DifficultyType::cEasy);
      
      pSettings->setString(BGameSettings::cPlayer3Name, "Cool on a Stick");
      pSettings->setUInt64(BGameSettings::cPlayer3XUID, 0);
      pSettings->setLong(BGameSettings::cPlayer3Team, 2);
      pSettings->setLong(BGameSettings::cPlayer3Civ, 2);
      pSettings->setLong(BGameSettings::cPlayer3Leader, 6);
      pSettings->setLong(BGameSettings::cPlayer3Type, BGameSettings::cPlayerComputer);
      pSettings->setFloat(BGameSettings::cPlayer3Difficulty, gDatabase.getDifficultyEasy());
      pSettings->setLong(BGameSettings::cPlayer3DifficultyType, DifficultyType::cEasy);

      pSettings->setString(BGameSettings::cPlayer4Name, "Furious Lemon");
      pSettings->setUInt64(BGameSettings::cPlayer4XUID, 0);
      pSettings->setLong(BGameSettings::cPlayer4Team, 2);
      pSettings->setLong(BGameSettings::cPlayer4Civ, 2);
      pSettings->setLong(BGameSettings::cPlayer4Leader, 6);
      pSettings->setLong(BGameSettings::cPlayer4Type, BGameSettings::cPlayerComputer);
      pSettings->setFloat(BGameSettings::cPlayer4Difficulty, gDatabase.getDifficultyEasy());
      pSettings->setLong(BGameSettings::cPlayer4DifficultyType, DifficultyType::cEasy);

      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
      pSettings->setLong(BGameSettings::cGameMode, 4);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      mSelectedFileName=mapName;
      //gLiveSystem->setPresenceState(CONTEXT_PRESENCE_PRE_MODE_TUTORIAL, 0 ,0);
      setNextState(cStateGotoGame);
   }
}

//============================================================================
//============================================================================
void BModeMenu::playTutorial( ETutorial cTutorial, long cStartContext )
{
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {
      BSimString gameID;
      MVince_CreateGameID(gameID);

      BUser* pUser = gUserManager.getPrimaryUser();

      pSettings->setLong(BGameSettings::cPlayerCount, 1);
      switch( cTutorial )
      {
         case cTutorialBasic:
            pSettings->setString(BGameSettings::cMapName, "CampaignUNSC\\design\\campaignTutorial\\campaignTutorial");
            gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, CONTEXT_PRE_CON_DIFFICULTY_PRE_CON_DIFF_EASY);
            break;

         case cTutorialAdvanced:
            pSettings->setString(BGameSettings::cMapName, "CampaignUNSC\\design\\campaignTutorialAdvanced\\campaignTutorialAdvanced");
            gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, CONTEXT_PRE_CON_DIFFICULTY_PRE_CON_DIFF_NORMAL);
            break;
      }
      
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);
      pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
      pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
      pSettings->setLong(BGameSettings::cGameStartContext, cStartContext );
      pSettings->setLong(BGameSettings::cGameMode, 0);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      //Update presence
      gLiveSystem->setPresenceContext(PROPERTY_GAMETIMEMINUTES, 0, true);
      gLiveSystem->setPresenceContext(PROPERTY_GAMESCORE, 0, true);
      gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, 0, true);      
      gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNSOLOPLAY);

      setNextState( cStateGotoGame );
   }
}

//==============================================================================
//==============================================================================
void BModeMenu::startDemo()
{
   //Reusing this for E3 demo 2008 - eric
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {
      BSimString gameID;
      MVince_CreateGameID(gameID);

      BSimString mapName = "skirmish\\design\\beta_1v1_cross\\beta_1v1_cross";
  
//-- FIXING PREFIX BUG ID 3619
      const BUser* pUser=gUserManager.getPrimaryUser();
//--

      pSettings->setLong(BGameSettings::cPlayerCount, 2);
      pSettings->setString(BGameSettings::cMapName, mapName);
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);

      pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
      pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
      pSettings->setLong(BGameSettings::cPlayer1Team, 1);
      pSettings->setLong(BGameSettings::cPlayer1Civ, 1);
      pSettings->setLong(BGameSettings::cPlayer1Leader, 1);
      pSettings->setLong(BGameSettings::cPlayer1Type, BGameSettings::cPlayerHuman);

      pSettings->setString(BGameSettings::cPlayer2Name, "Furious Lemon");
      pSettings->setUInt64(BGameSettings::cPlayer2XUID, 0);
      pSettings->setLong(BGameSettings::cPlayer2Team, 2);
      pSettings->setLong(BGameSettings::cPlayer2Civ, 2);
      pSettings->setLong(BGameSettings::cPlayer2Leader, 6);
      pSettings->setLong(BGameSettings::cPlayer2Type, BGameSettings::cPlayerComputer);
      pSettings->setFloat(BGameSettings::cPlayer2Difficulty, gDatabase.getDifficultyEasy());
      pSettings->setLong(BGameSettings::cPlayer2DifficultyType, DifficultyType::cEasy);

      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
      pSettings->setLong(BGameSettings::cGameMode, 4);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      mSelectedFileName=mapName;
      setNextState(cStateGotoGame);
   }

   /*
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {
      BSimString gameID;
      MVince_CreateGameID(gameID);

      BSimString mapName="demo\\art\\e32007\\e32007";

      BUser* pUser=gUserManager.getPrimaryUser();

      pSettings->setLong(BGameSettings::cPlayerCount, 2);
      pSettings->setString(BGameSettings::cMapName, mapName);
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);
      pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
      pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
      pSettings->setLong(BGameSettings::cGameMode, 0);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      mSelectedFileName=mapName;

      setNextState(cStateGotoGame);
   }
   */
}

bool BModeMenu::getDifficultyValueForType( long type, float& value )
{
   switch( type )
   {
      case DifficultyType::cAutomatic:
      {
         BHumanPlayerAITrackingData trackingData;
         if( trackingData.loadValuesFromMemoryBlock( gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer() ) )
            value = trackingData.getAutoDifficultyLevel() / 100.0f;
         else
            value = gDatabase.getDifficultyDefault();
         return true;
      }

      case DifficultyType::cEasy:
         value = gDatabase.getDifficultyEasy();
         return true;

      case DifficultyType::cNormal:
         value = gDatabase.getDifficultyNormal();
         return true;

      case DifficultyType::cHard:
         value = gDatabase.getDifficultyHard();
         return true;

      case DifficultyType::cLegendary:
         value = gDatabase.getDifficultyLegendary();
         return true;

      case DifficultyType::cCustom:
         return false;

      default:
         BASSERTM(false, "Invalid difficulty type specified.  Setting Default Difficulty Value.");
         value = gDatabase.getDifficultyDefault();
         return false;
   }
}


//============================================================================
//============================================================================
void BModeMenu::play()
{
   if (mBackgoundVideoHandle == cInvalidVideoHandle)
      return;

   gBinkInterface.playVideo(mBackgoundVideoHandle);
   mVideoPaused = false;
}

//============================================================================
//============================================================================
void BModeMenu::pause()
{
   if (mBackgoundVideoHandle == cInvalidVideoHandle)
      return;

   mVideoPaused = true;
   gBinkInterface.pauseVideo(mBackgoundVideoHandle);
}

//============================================================================
//============================================================================
void BModeMenu::togglePause()
{
   if (!mVideoPaused)
      pause();
   else
      play();
}

//============================================================================
//============================================================================
void BModeMenu::notify( DWORD eventID, void* pTask )
{
   // default
   if( !pTask )
   {
      mNextState = cStateMain;
      return;
   }

   BSelectDeviceAsyncTask* pSelectDeviceTask = reinterpret_cast<BSelectDeviceAsyncTask*>( pTask );
   XCONTENTDEVICEID deviceID = pSelectDeviceTask->getDeviceID();

   if( deviceID == XCONTENTDEVICE_ANY )
   {
      // Show a dialog confirming that they don't want to select a device
      gGame.getUIGlobals()->showYornBox( this, gDatabase.getLocStringFromID(25953), BUIGlobals::cDialogButtonsOKCancel, cModeMenuYornChooseDevice, gDatabase.getLocStringFromID(25578), gDatabase.getLocStringFromID(25579) );
      return;
   }

   // Give the blade some time to animate closing
   DWORD startTime = timeGetTime();
   while (timeGetTime() - startTime < 300)
   {
      gGame.updateRender();
      Sleep(1);
   }

   if (!gSaveGame.getCampaignSaveExists(NULL))
   {
      if (!gSaveGame.createNewCampaignFile())
      {
         // Show a dialog warning save file could not be created.
         gGame.getUIGlobals()->showYornBox( this, gDatabase.getLocStringFromID(25997), BUIGlobals::cDialogButtonsOK );
         mNextState = cStateGotoCampaign;
         return;
      }
   }

   // let's go to the campaign mode
   mNextState = cStateGotoCampaign;
}

//============================================================================
// returns true if the current selected device is valid, otherwise, pops a yorn box and goes on.
// 
//============================================================================
bool BModeMenu::checkSaveDevice()
{
   XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();
   if (deviceID != XCONTENTDEVICE_ANY)
   {
      XDEVICE_DATA deviceData;
      ZeroMemory(&deviceData, sizeof(XDEVICE_DATA));

      // Check to see that the device is currently usable.
      if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
      {
         gUserManager.getPrimaryUser()->setDefaultDevice(XCONTENTDEVICE_ANY);
         deviceID = XCONTENTDEVICE_ANY;
      }
   }

   if( deviceID != XCONTENTDEVICE_ANY )
      return true;

   // see if we can auto detect the save storage device.
   deviceID = gSaveGame.autoSelectStorageDevice();
   if (deviceID != XCONTENTDEVICE_ANY)
   {
      gUserManager.getPrimaryUser()->setDefaultDevice(deviceID);
      return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
void BModeMenu::onVideoCaptions(BBinkVideoHandle handle, bool enableCaptions, const BUString& caption)
{
   if( mState == cStateCreditsMovie )
   {
      // if there is a movie player screen up, then send the captioning data to it
      if( mpUIScreen && mpUIScreen->getScreenName() == "UICreditsMoviePlayer" )
      {
         BUICreditsMoviePlayer* pCreditsMoviePlayer = reinterpret_cast<BUICreditsMoviePlayer*>(mpUIScreen);
         pCreditsMoviePlayer->setCaptionData(enableCaptions, caption);
      }
   }
}