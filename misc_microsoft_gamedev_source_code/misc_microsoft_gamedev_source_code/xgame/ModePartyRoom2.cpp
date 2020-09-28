//==============================================================================
// modepartyroom2.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

// Includes
#include "common.h"
#include "ModePartyRoom2.h"

// xgame
#include "civ.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "leaders.h"
#include "modemanager.h"
#include "recordgame.h"
#include "ui.h"
#include "usermanager.h"
#include "user.h"
#include "world.h"  //Need these two to be able to query the final teams for players to set the voice channels - eric
#include "player.h"
#include "UIMPSetupScreen.h"
#include "campaignmanager.h"
#include "UIGlobals.h"
#include "modemenu.h"
#include "binkInterface.h"
#include "archiveManager.h"
#include "UIInputHandler.h"
#include "UILeaderPicker.h"
#include "GamerPicManager.h"
#include "gamemode.h"
#include "userprofilemanager.h"
#include "campaignprogress.h"

// xgamerender
#include "FontSystem2.h"

// xrender
#include "render.h"

// xmultiplayer
#include "mpSession.h"

// xsystem
#include "dataentry.h"
#include "econfigenum.h"

// xvince
#include "vincehelper.h"
#include "inputcontrol.h"

// live
#include "liveSystem.h"
#include "liveSession.h"
#include "liveVoice.h"
#include "mpSession.h"
#include "liveSessionSearch.h"
#include "matchMakingHopperList.h"
#include "partySession.h"
#include "XLastGenerated.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

// Constants
static const long  cMatchMakingDefaultCiv=1;
static const long  cMatchMakingDefaultLeader=1;

//==============================================================================
//
//==============================================================================
BModePartyRoom2::BModePartyRoom2(long modeType) :
//NOTE!!! If you are here to initialize a member variable - do it below in "initializeMemberVariables()"
//  That method is called in this constructor as well as when the system is re-entered
   BMode(modeType),
   mSettingMap(),
   mpInputHandler(NULL),
   mKickTargetXUID(0),
   mUseLANMode(false),
   mInitialPartyRoomMode(cPartyRoomModeMM)
{
   initializeMemberVariables();
   mpInputHandler = new BUIInputHandler();
   mpInputHandler->loadControls("art\\ui\\flash\\pregame\\GameSetup\\GameSetupScreenInput.xml", this);
}

//==============================================================================
//
//==============================================================================
BModePartyRoom2::~BModePartyRoom2()
{
   multiplayerShutdown();
   if (mpInputHandler)
   {
      delete mpInputHandler;
      mpInputHandler=NULL;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::initializeMemberVariables()
{
   mpPartyRoomScreen=NULL;
   mpLeaderPicker=NULL;
   mState=-1;
   mPartyRoomMode=cPartyRoomModeInvalid;
   mNextState=-1;
   mLocalPlayer=-1;
   mLaunchCountdown=-1;
   mMatchMakingSettingCiv=cMatchMakingDefaultCiv;
   mMatchMakingSettingLeader=cMatchMakingDefaultLeader;
   mSettingCoop=false;
   mSettingRecordGame=false;
   mSettingReady=false;
   mLaunchStartRequested=false;
   mUseDefaultMap=true;
   mCurrentHopperIndex=0;
   mMusicCue=NULL;
   mTeamRequestChangePending=false;
   mWaitingOnEveryoneToRejoinParty=false;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::setup()
{
   return BMode::setup();
}

//====================================================================================================================

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::onReady()
{
   // Force focus to me if I ready up
//-- FIXING PREFIX BUG ID 5126
   const BPartySessionPartyMember* pLocalMember = getLocalMember();
//--
   if (!pLocalMember)
   {
      return false;
   }
   BASSERT(pLocalMember);
   //Check that I was not trying to change teams or cancel a switch
   if ((pLocalMember->mSettings.mConnectionState >= cBPartySessionMemberConnectionStateWantToSwitchTeams) &&
       (pLocalMember->mSettings.mConnectionState <= cBPartySessionMemberConnectionStateCancelSwitchTeams))
   {
      //We were not in a good state - ignore this request
      //NOTE - this change also makes it so that that we don't try and green up (which switches states) while in transition which
      //  could cause a race condition on the state switch resolution
      if (pLocalMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateWantToSwitchTeams)
      {
         BPartySessionPlayerSettings temp;
         Utils::FastMemCpy(&temp, &pLocalMember->mSettings, sizeof(BPartySessionPlayerSettings));
         temp.mConnectionState = cBPartySessionMemberConnectionStateCancelSwitchTeams;
         // send the setting change request to the host
         getPartySession()->changeSettings(pLocalMember->mXuid, temp);
      }
      gSoundManager.playCue("play_menu_back_button");
      return false;
   }
   
   mpPartyRoomScreen->setPlayerCurrentSlotFocus(pLocalMember->mSettings.mSlot);
   mpPartyRoomScreen->setPlayerNavigationEnabled(false);
   setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
   // mpPartyRoomScreen->getMenu()->clearItemFocus();
   mpPartyRoomScreen->getMainMenu().setIndex(-1);
   toggleReady();

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::onUnready()
{
   mpPartyRoomScreen->setPlayerNavigationEnabled(true);

   if (isLocalPlayerReady())
      toggleReady();

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
/*
   //If the party session is not up yet - the ONLY thing we want to handle is the cancel
   if ((!getPartySession() ||
        !getLocalMember()) &&
       (command != "cancel"))
   {
      return false;
   }

   bool handled = false;
   if (command=="ready")
   {
      if (isLocalPlayerReady())
         return false;
      onReady();
      handled=true;
   }
   else 
*/
   if (command == "cancel")
   {
      if (isLocalPlayerReady())
      {
         onUnready();
      }
      else
      {
         exitParty();
      }
      return true;
   }

   return false;
}

//==============================================================================
// BModePartyRoom2::enterContext
//==============================================================================
void BModePartyRoom2::enterContext(const BCHAR_T* contextName)
{
   if (mpInputHandler)
      mpInputHandler->enterContext(contextName);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::shutdown()
{
   BMode::shutdown();
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::enter(BMode* lastMode)
{
   // we need to do this just in case we come here from somewhere where the pregame UI is not loaded.
   bool success = gArchiveManager.beginMenu();
   BVERIFY(success);

   // default to Live
   mUseLANMode=false;
   mInitialPartyRoomMode=cPartyRoomModeMM;

   enterContext("Main");
   if (!gLiveSystem->isPartySystemFlaggedForReenter())
   {
      initializeMemberVariables();
   }

   showWaitDialog(gDatabase.getLocStringFromID(23474)); // Initializing Party System...
   
   BSimString movie;
   if (gConfig.get(cConfigUIBackgroundMovie, movie))
   {
      BBinkInterface::BLoadParams lp;
      lp.mFilename.set(movie.getPtr());
      lp.mCaptionDirID = cDirData;
      lp.mpStatusCallback = this;
      lp.mLoopVideo = true;
      lp.mFullScreen = true;
      gBinkInterface.registerValidCallback(this);
      gBinkInterface.loadActiveVideo(lp);//gBinkInterface.loadActiveVideoFullScreen(movie.getPtr(), cDirData, "", "", this, NULL,true);
      gBinkInterface.playAllVideos();
   }

   // check that we have our flash UI up and loaded
   if (mpPartyRoomScreen == NULL)
   {
      //mpPartyRoomScreen = new BUIPartyRoom2();
      mpPartyRoomScreen = new BUIMPSetupScreen();
      BUIScreen* pUIScreen = (BUIScreen*)mpPartyRoomScreen;

      BFATAL_ASSERTM(pUIScreen->init("art\\ui\\flash\\pregame\\GameSetup\\GameSetupScreen.gfx", "art\\ui\\flash\\pregame\\GameSetup\\GameSetupScreenData.xml"), "BModePartyRoom2::enter() -- Game Setup UI Init Failed!");

      mpPartyRoomScreen->setView(BUIMPSetupScreen::cPRViewPlayerList);
            
      // fixme - shawn - until I can refactor this
      mpPartyRoomScreen->setVisible(false);

   }

   if (mpLeaderPicker == NULL)
   {
      mpLeaderPicker = new BUILeaderPicker();
      mpLeaderPicker->init("art\\ui\\flash\\pregame\\leaderPicker\\LeaderPicker.gfx", "art\\ui\\flash\\pregame\\leaderPicker\\LeaderPickerData.xml");
      mpLeaderPicker->setEventHandler(this);
   }

   // initialize the voice slots to nobody talking
   for (uint i=0; i<cPRNumPlayerSlots; i++)
      mVoiceSlots[i]=BUIGamerTagLongControl::cSpeakerOff;

   mState = cStateWaitingOnMPSessionToReady;

   //This is done in a new startup state (cStateWaitingOnMPSessionToReady) handled in the update - eric
   //initializeParty();

   //nlog(cMPModeMenuCL, "BModePartyRoom2::enter");
   gDatabase.loadLeaderIcons();

   if(!gConfig.isDefined(cConfigNoMusic))
   {  
      if( lastMode == reinterpret_cast<BMode*>(gModeManager.getModeGame()) )
         gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicPlayPreGame);

      mMusicCue = gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateSkirmishMenu);
   }

   gGame.openVinceLog();
   MVinceEventAsync_PreGameEvent("MPRoomEnter");

   if (gLiveSystem->getMPSession())
      gLiveSystem->getMPSession()->initLanDiscovery();

   return BMode::enter(lastMode);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::leave(BMode* newMode)
{

   BDEBUG_ASSERT(gLiveSystem);

   hideWaitDialog();

   // don't play the video anymore
   gBinkInterface.unregisterValidCallback(this);
   gBinkInterface.stopAllVideos(false);

   // make sure we pull any dialogs down that might be up for some reason
   if (!gLiveSystem || !gLiveSystem->getMPSession() || !gLiveSystem->getMPSession()->doNotDismissDialog())
   {
      BUIGlobals* puiGlobals = gGame.getUIGlobals();
      if (puiGlobals)
         puiGlobals->hideYorn();
   }

   if (mpLeaderPicker)
   {
      mpLeaderPicker->deinit();
      delete mpLeaderPicker;
      mpLeaderPicker=NULL;
   }

   // shut down our UI
   if (mpPartyRoomScreen)
   {
      mpPartyRoomScreen->deinit();
      delete mpPartyRoomScreen;
      mpPartyRoomScreen=NULL;
   }

   // dispose the BMPSession instance and the audio engine used for voice comms
   // but only if we're leaving this mode for something other than cModeGame
   if ((newMode == NULL) || (newMode->getModeType() != BModeManager::cModeGame))
   {
      tearDownParty();
   }
   else
   {
      //Clear the interfaces in mpSession so that this object no longer gets callbacks
      nlog(cMPModeMenuCL, "BModePartyRoom2::leave - clearing the session/player interfaces in mpSession");
      gLiveSystem->getMPSession()->clearSessionInterface();
      //gLiveSystem->setPresenceStateAction(CONTEXT_PRE_CON_ACTION_PRE_CON_ACTION_INGAME);
      if (isLocalPlayerHost())
      {
         //Host needs to reset the party back into a normal, base connected state so we are ready for rejoins after the game is over
         mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeGameRunning;
         getPartySession()->changeHostSettings(mGameOptionSettings);
         gLiveSystem->getMPSession()->getPartySession()->kickAllAIMembers();
         //Need to clear their ready up flag
         BPartySession* pParty = getPartySession();
         BASSERT(pParty);
         if (pParty)
         {
            uint partyCount = pParty->getPartyMaxSize();
            for (uint i=0; i<partyCount; i++)
            {
               BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
               if (!pPartyMember)
                  continue;

               if (pPartyMember->mXuid==0)
                  continue;

               pPartyMember->mSettings.mConnectionState = (uint8)cBPartySessionMemberConnectionStateConnected;
               pParty->broadcastChangeSettings(pPartyMember->mSettings, pPartyMember->mXuid);
            }
         }
      }
   }

   //Clear any invite info out there - this prevents the problem where if you cancel a join before it makes
   // That next time you start the party the join info is still there and it tries to hook up
   if ( !((newMode != NULL) && 
          (newMode->getModeType() == BModeManager::cModeMenu) && 
          (gModeManager.getModeMenu()->getNextState() == BModeMenu::cStateInviteFromOtherController) ) )
   {
      gLiveSystem->clearInviteInfo();
   }

   nlog(cMPModeMenuCL, "BModePartyRoom2::leave");
   gDatabase.unloadLeaderIcons();

   return BMode::leave(newMode);
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::update()
{
   BMode::update();

   //Test code for 2 peeps in one slot , 1v1 only
   /*
   BPartySession* pParty = getPartySession();
   if (pParty && (pParty->getPartyCount()==2) && (pParty->getPartyMember(0)->mSettings.mSlot!=0))
   {
      BASSERT(pParty->getPartyMember(0)->mSettings.mSlot != pParty->getPartyMember(1)->mSettings.mSlot);
   }
   */

   if (mpPartyRoomScreen)
      mpPartyRoomScreen->update(0);             // not in the game, no game time passed

   if ( gLiveSystem->isInviteInfoAvailable() &&
      (cStateWaitingOnMPSessionToReady != mState) &&
      (cStateInitializingMPSession != mState) &&
      (cStateInitializingPartySessionHosting != mState) &&
      (cStateInitializingPartySessionJoining != mState) &&
      (mState <= cStateEndOfModeError))
   {
      //FYI: I could be in cStateEndOfModeError if the current party was ended abruptly (like I was kicked from the host/etc)
      BUIGlobals* pGlobals = gGame.getUIGlobals();
      pGlobals->hideYorn();
      showWaitDialog(gDatabase.getLocStringFromID(23474)); // Initializing Party System...
      tearDownParty();   
      mState = cStateWaitingOnMPSessionToReady;
      return;
   }

   // are we transitioning to a new state?
   if (mNextState != -1)
   {
      mState = mNextState;
      mNextState = -1;
   }

   //Update presence based on my current state data 
   //Skip this if there is a game session because we do NOT want to step on its presence setting actions
   if (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession() && !gLiveSystem->getMPSession()->getGameSession())
   {
      const BPartySessionHostSettings& hostSettings = gLiveSystem->getMPSession()->getPartySession()->getCurrentHostSettings();
      gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_HWGAMEMODE, hostSettings.mGameMode);
      switch(gLiveSystem->getMPSession()->getPartySession()->getPartyMaxSize())
      {
         case (2) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_1V1);break;
         case (4) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_2V2);break;
         case (6) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_3V3);break;
      }
      if (gLiveSystem->getMPSession()->isInLANMode())
      {
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SKIRMLOCALLOBBY);
         gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, hostSettings.mDifficulty);
      }
      else
      {
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SKIRMLIVELOBBY);
         gLiveSystem->setPresenceContext(PROPERTY_LOBBYCURRENTPLAYERS, gLiveSystem->getMPSession()->getPartySession()->getPartyCount(), true);
         gLiveSystem->setPresenceContext(PROPERTY_LOBBYMAXPLAYERS, gLiveSystem->getMPSession()->getPartySession()->getPartyMaxSize(), true );
      }

      //Fill out the values based on my room
      switch (hostSettings.mPartyRoomMode)
      {
      case (cPartyRoomModeCampaign) :
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNLOBBY);
            gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, hostSettings.mDifficulty);
            BCampaign * pCampaign = gCampaignManager.getCampaign(0);
            BASSERT(pCampaign);
            if (pCampaign)
            {
               BCampaignNode* pNode = pCampaign->getNode( hostSettings.mMapIndex );
               BASSERT(pNode);
               if (pNode)
               {
                  gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, pNode->getLeaderboardLevelIndex(), true);
               }
            }
            break;
         }
      case (cPartyRoomModeMM):
         {
            BMatchMakingHopper* hopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(mGameOptionSettings.mHopperIndex);
            if (hopper)
            {
               if (gLiveSystem->getMPSession()->isMatchmakingRunning())
               {
                  gLiveSystem->setPresenceContext(X_CONTEXT_GAME_MODE, hopper->mXLastGameModeIndex );
                  gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SKIRMMATCHMAKING);
               }
               //gLiveSystem->setPresenceContext(X_CONTEXT_GAME_MODE, hopper->mXLastGameModeIndex );          
               gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_HWGAMEMODE, hopper->mGameModeIndex );
            }                   
            break;
         }
      case (cPartyRoomModeCustom) :
         {
            //gLiveSystem->setPresenceContext(X_CONTEXT_GAME_MODE, 0 ); 
            gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_HWGAMEMODE, hostSettings.mGameMode);
            break;
         }
      }
   }

   if ((mState>=cStateInSession) &&
       (mState <cStateStartGame))
   {
      //In ANY of these states, if my partysession is gone - then I need to ditch
      if (gLiveSystem->getMPSession() && !gLiveSystem->getMPSession()->getPartySession())
      {
         nlog(cMPModeMenuCL, "BModePartyRoom2::update - Im in state [%d] but no partySession - returning to mainmenu", mState);
         gModeManager.setMode(BModeManager::cModeMenu);
      }
      //Update the playerslots (if not matchmaking this will do nothing)
      updateMatchmakingSlots();
      //This updates the voice icons in the UI
      updateVoice();
      //In a particular frame, someone can green up but yet the game cannot startup yet (because it is waiting on the game session to clean up)
      // So we need to check for changes each frame
      checkForGreenUpTriggers();
   }

   switch (mState)
   {
      case cStateInSession:
         if (mWaitingOnEveryoneToRejoinParty && gLiveSystem->getMPSession()->getPartySession()->isPartyAtIdleState())
         {
            mWaitingOnEveryoneToRejoinParty=false;
         }
         break;
      case cStateEndOfModeError:          //A dead state where the only path out of here is LEAVING this mode
         {
            //TODO - for now I just made this like the normal Exit - we need to add in the "going away" dialog
            /*
            multiplayerShutdown(); 
            gModeManager.getModeMenu()->setNextState( BModeMenu::cStateMain );
            gModeManager.setMode(BModeManager::cModeMenu);
            */
            break;
         }
      case cStateExitMode:                //We are shutting down
      {
         gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateMainTheme);

         multiplayerShutdown(); 
         MVinceEventAsync_PreGameEvent("MPRoomExit");
         gModeManager.getModeMenu()->setNextState( BModeMenu::cStateMain );
         if (!mUseLANMode)
            gModeManager.getModeMenu()->setMenuItemFocus(BModeMenu::cStateGotoMultiplayer);
         else
            gModeManager.getModeMenu()->setMenuItemFocus(BModeMenu::cStateGotoSystemLink);
         gModeManager.setMode(BModeManager::cModeMenu);
         break;
      }
      case cStateStartGame:
      {
         hideWaitDialog();
         if (startGameSync())
         {
            //After game sync - teams are good - lets set voice channels
            setVoiceChannels();
         }
         break;
      }
      case cStateWaitingOnMPSessionToReady :
         {
            //Ok - here I have to do some analysis about what the situation could be:
            // - Started from the menu as the host
            // - Joined via invite/presence join
            //   - From the menu or other non MP place
            //   - From another party or another game where the mp layer needs to be reset 
            // - From end of game and I need to rehookup to the existing party layer
            
            if ((gLiveSystem->isPartySystemFlaggedForReenter()) && 
                gLiveSystem->getMPSession() &&
                (gLiveSystem->getMPSession()->getPartySession()))
            {
               if (!gLiveSystem->isPartySessionActive())
               {
                  BASSERTM(false, "ModePartyRoom2::update - cStateWaitingOnMPSessionToReady - Party is DEAD, call Eric");
               }

               //I am coming back in from the end of a game 
               gLiveSystem->getMPSession()->setSessionInterface(this);
               mState = cStateInSession; 
               //Set some defaults
               mLaunchStartRequested=false;
               gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
               setUseLanMode(gLiveSystem->getMPSession()->isInLANMode());

               const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               if (!pUser || !pUser->getFlagUserActive() || (!mUseLANMode && (!pUser->isSignedIntoLive() || pUser->hasLiveStateChanged())))
               {
                  mNextState = cStateEndOfModeError;

                  if (!mUseLANMode && (!pUser->isSignedIntoLive() || pUser->hasLiveStateChanged()))
                     showModeExitErrorMessage(gDatabase.getLocStringFromID(25443));
                  return;
               }

               if (!gLiveSystem->isPartySessionActive())
               {
                  mNextState = cStateEndOfModeError;
                  showModeExitErrorMessage(gDatabase.getLocStringFromID(25443));
                  return;
               }

               //Setup the UI
               hideWaitDialog();
               gLiveSystem->getMPSession()->BPartySessionEvent_systemReady();
               //Update my campaign bits (or any other setting you want a member to send out when he rejoins the party after a game)
               BPartySessionPartyMember* pMe = getLocalMember();

               // update the player's rank so everyone can see how uber we have just become
               if (pUser != NULL)
               {
                  const BUserProfile* pProfile = pUser->getProfile();
                  if (pProfile != NULL)
                     pMe->mSettings.mPlayerRank.mValue = pProfile->getRank().mValue;
               }

               BPartySessionPlayerSettings sendSettings = pMe->mSettings;
               sendSettings.mCampaignBits = getLocalCampaignBits();
               getPartySession()->changeSettings(pMe->mXuid, sendSettings);
               // Trigger the option update
               mGameOptionSettings = gLiveSystem->getMPSession()->getPartySession()->getCurrentHostSettings();
               mpSessionEvent_partyEvent_hostSettingsChanged(&mGameOptionSettings);
               //Trigger this to refresh the UI
               updateAllGameOptions(&mGameOptionSettings);
               //Set the focus for the UI
               mWaitingOnEveryoneToRejoinParty = true;           
               if (isLocalPlayerHost())
               {
                  setPartyRoomView(BUIMPSetupScreen::cPRViewMenu);
                  mpPartyRoomScreen->getMainMenu().setIndex(0);   
                  //Send out notice that the host is back in the party screen
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodePostGameResetUI;
                  gLiveSystem->getMPSession()->getPartySession()->changeHostSettings(mGameOptionSettings);
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
                  gLiveSystem->getMPSession()->getPartySession()->changeHostSettings(mGameOptionSettings);                  
               }
               else
               {
                  if (mGameOptionSettings.mHostStatusInformation== cBPartySessionHostStatusInformationCodeGameRunning)
                  {
                     //Show a dialog saying I've got to wait for the host to return
                     //setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
                     BUIGlobals* puiGlobals = gGame.getUIGlobals();
                     BASSERT(puiGlobals);
                     puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25672), BUIGlobals::cDialogButtonsOK, cYornCallbackContextLeaveParty); 
                  }
                  else
                  {
                     BPartySessionPartyMember* pLocalMember = getLocalMember();
                     if (pLocalMember)
                     {
                        mpPartyRoomScreen->setPlayerCurrentSlotFocus(pLocalMember->mSettings.mSlot);
                     }
                     mpPartyRoomScreen->setPlayerNavigationEnabled(true);
                     setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
                  }
               }
               //Set voice channels
               BLiveVoice* pVoice = gLiveSystem->getLiveVoice();
               if ((pVoice) 
#ifndef BUILD_FINAL
                  && (!gConfig.isDefined(cConfigNoVoice))
#endif
                  )
               {
                  pVoice->setAllChannels(BVoice::cPartySession, BVoice::cAll);
               }
               //E3 hack crap - eric
               if (gConfig.isDefined(cConfigDemo) && 
                   isLocalPlayerHost() &&
                   (mPartyRoomMode==cPartyRoomModeCustom))
               {
                  addAIPlayer(0);
                  addAIPlayer(3);
                  if (mGameOptionSettings.mNumPlayers==4)
                  {
                     addAIPlayer(3);
                     addAIPlayer(4);
                  }
               }

            }
            else if (gLiveSystem->isInviteInfoAvailable())
            {
               //Ok - I'm joining via invite   
               if (gLiveSystem->getMPSession() && !gLiveSystem->getMPSession()->isOkToStartup())
                  return;
               nlog(cMPModeMenuCL, "BModePartyRoom2::enter - I am joining an existing party");
               mUseLANMode = false;
               initializeParty();
            }
            else
            {
               /*
               //No invite - is there a party already running?
               if (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession())
                  {
                     //Yes - Then I must be rehooking up the party after an end of game 
                     if (!gLiveSystem->getMPSession()->isReadyToStartGameSession())
                        return;
                     nlog(cMPModeMenuCL, "BModePartyRoom2::enter - I am rejoining an existing party session");
                     hideWaitDialog();
                     mState = cStateInSession; 
                  }
               else
               */
               {
                  //No - then I must be needing to start hosting the game  - we are fine to start the initialization '
                  if (gLiveSystem->getMPSession() && !gLiveSystem->getMPSession()->isOkToStartup())
                     return;
                  nlog(cMPModeMenuCL, "BModePartyRoom2::enter - I am starting as a new party host");
                  initializeParty();
               }
            }
            break;
         }

   }
}


//==============================================================================
// Returns true when the game start is synced and launched
//==============================================================================
bool BModePartyRoom2::startGame()
{

   //Is the rest of the system able to start another game session just yet?
   if (!gLiveSystem->getMPSession()->isReadyToStartGameSession())
      return false;

   mLaunchStartRequested = true;

   // this call will result the callbacks:
   //    virtual void mpSessionEvent_partyEvent_customGameStartupComplete();
   mState=cStateGameHostInit;
   BASSERT(gLiveSystem->getMPSession());
   BASSERT(gLiveSystem->getMPSession()->getPartySession());
   BASSERT(isLocalPlayerHost());

   //Lets refresh the local settings member var
   const BPartySessionHostSettings* pHostSettings = getHostSettings();
   BASSERT(pHostSettings);
   Utils::FastMemCpy(&mGameOptionSettings, pHostSettings, sizeof(BPartySessionHostSettings)); 

   //Lock the party session
   gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(true);

   //In matchmaking mode - Startup matchmaking
   if (mGameOptionSettings.mPartyRoomMode == cPartyRoomModeMM)
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::startGame - launching matchmaking");
      BMPSessionMatchMakingData configData;               
      Utils::FastMemSet(&configData,0,sizeof(BMPSessionMatchMakingData));    
      configData.mSelectedHopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(mGameOptionSettings.mHopperIndex);
      //BASSERT(configData.mSelectedHopper);
      if (!configData.mSelectedHopper)
      {
         //The hopper that we selected is no longer valid (probably due to a hopper data update from the LSP)
         mTempMessageHolder = gDatabase.getLocStringFromID(25583); 
         showModeModelEventMessage(mTempMessageHolder);    
         onUnready();
         mLaunchStartRequested = false;
         gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
         mState = cStateInSession;
         return false;
      }
      MVinceEventAsync_PreGameSettings("MPMatchmakingStart", -1, configData.mSelectedHopper->mName.getPtr(), getLocalMember());
      BOOL ret = gLiveSystem->getMPSession()->startMatchMaking(&configData);
      if (ret != TRUE)
      {
         MVinceEventAsync_PreGameEvent("MPMatchmakingFail");
         nlog(cMPModeMenuCL, "BModePartyRoom2::startGame - Matchmaking said it could not start...");
         showModeModelEventMessage(mTempMessageHolder);    
         onUnready();
         mLaunchStartRequested = false;
         gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
         mState = cStateInSession;
         return false;
      }
      else
      {
         BPartySession* pParty = getPartySession();
         BASSERT(pParty);
         mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeInitializingMatchmakingSearch;
         pParty->changeHostSettings(mGameOptionSettings);
      }
   }
   else
   {
      //Startup a game host
      MVinceEventAsync_PreGameSettings("MPCustomStart", mGameOptionSettings.mMapIndex, "Custom", getLocalMember());
      long slots = 0;
      //gDatabase.getGameSettings()->getLong(BGameSettings::cMaxPlayers, slots);
      //Change so that the max slot count is pulled from the number of connected party members
      //  This supports AI players in the mix (who are not connected)
      //  The count is bumped up right before launch and the AI players are added in then
      slots = gLiveSystem->getMPSession()->getPartySession()->getPartyCount();
      nlog(cMPModeMenuCL, "BModePartyRoom2::startGame - Launching %i player custom host from MPSession.", slots);
      if (!gLiveSystem->getMPSession()->startupCustomHost(slots))
      {
         MVinceEventAsync_PreGameEvent("MPCustomFail");
         BASSERTM(false, "ERROR - Game Host launch failed IMMEDIATELY");
         nlog(cMPModeMenuCL, "BModePartyRoom::startGame - Custom game said it could not start...");         
         mLaunchStartRequested = false;
         gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
         mState = cStateInSession;
         return false;
      }
      mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeGameLaunchStarting;
      getPartySession()->changeHostSettings(mGameOptionSettings);
   }

   //Host no longer hax in his own dialogs - but instead responds to the same ones the clients do
   //mTempMessageHolder.set(L"Countdown...");
   //showWaitDialog(mTempMessageHolder);
   return true;
}

//==============================================================================
// Returns true when the game start is synced and launched
//==============================================================================
bool BModePartyRoom2::startGameSync()
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::startGameSync called");
   if (!gLiveSystem )
      return false;

   if (!gLiveSystem->getMPSession())
      return false;

   if (!gLiveSystem->getMPSession()->getSession())
      return false;

   if (!gLiveSystem->getMPSession()->getSession()->getTimeSync())
      return false;

   //Lock the party session
   gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(true);

   //NOTE: This next call BLOCKS on the entire level loading
   if (!gGame.startGameSync())
      return false;

   //A check here to make sure its all good - HACK, because of when initGame fails it resets everything, but we don't know that here so we have a crash which is bad
   if (!gLiveSystem )
      return false;
   if (!gLiveSystem->getMPSession())
      return false;

   // XXX TODO : cleanup up the following three calls and let BSession
   // deal with it instead of micro-managing from the modemultiplayer level
   
   // tell the time sync system to await incoming game timings
   gLiveSystem->getMPSession()->getSession()->getTimeSync()->gameStarted(gEnableSubUpdating);

//   //Set the team chats
//   //BMPSession* mpSession = gLiveSystem->getMPSession();
//   BLiveVoice* pVoice = gLiveSystem->getLiveVoice();
//   if ((pVoice) 
//#ifndef BUILD_FINAL
//       && (!gConfig.isDefined(cConfigNoVoice))
//#endif
//       )
//   {
//      //Set the transport mode so that every in that transport mode gets added to the voice system
//      //mpSession->setVoiceTransportMode(BMPSession::cVoiceTransportGameSession);
   //This needs to stay in here to help test know when a start failure happens if it is a map problem or not
      BASSERTM(gWorld, "WARNING: LEVEL FAILED TO LOAD (OMG!)");
      if (!gWorld)
      {
         //This is an abort out for development only - if we expect levels to fail to load in production, we'll need a nice message here
         toggleReady();
         return false;
      }
//
//      long totalTeam1 = 0;
//      long totalTeam2 = 0;
//      long count = gWorld->getNumberPlayers();
//      for (long id=1; id < count; ++id)
//      {
//         const BPlayer* pPlayer = gWorld->getPlayer(id);
//         BASSERT(pPlayer);
//         if (pPlayer->isHuman())
//         {
//            if (pPlayer->getTeamID() == 1)
//            {
//               ++totalTeam1;
//            }
//            else if (pPlayer->getTeamID() == 2)
//            {
//               ++totalTeam2;
//            }
//            else
//            {
//               BASSERTM(FALSE, "Found a human player that is not on team 1 or 2");
//            }
//         }
//      }
//
//      // if only one team exists of human players or each team only has one player, just dump everyone into the all channel
//      if (totalTeam1 == 0 || totalTeam2 == 0 || (totalTeam1 == 1 && totalTeam2 == 1))
//         pVoice->setAllChannels(BVoice::cGameSession, BVoice::cAll);
//      else
//      {
//         for (long id=1; id < count; ++id)
//         {
//            const BPlayer* pPlayer = gWorld->getPlayer(id);
//            BASSERT(pPlayer);
//            if (pPlayer->isHuman())
//            {
//               if (pPlayer->getTeamID() == 1)
//                  pVoice->setChannel(BVoice::cGameSession, pPlayer->getXUID(), BVoice::cTeam1);
//               else if (pPlayer->getTeamID() == 2)
//                  pVoice->setChannel(BVoice::cGameSession, pPlayer->getXUID(), BVoice::cTeam2);
//            }
//         }
//      }
//
//      // informs the voice system that the given session is now the primary one
//      // this will update mute lists if necessary
//      pVoice->setSession(BVoice::cGameSession);
//   }

   nlog(cMPModeMenuCL, "BModePartyRoom2::startGameSync - returning true");
   return(true);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::renderBegin()
{
   gRender.beginFrame(1.0f/30.0f);
   gRender.beginViewport(-1);
   gRenderDraw.beginScene();
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}

//==============================================================================
// BModeMenu::RenderVideos
//==============================================================================
void BModePartyRoom2::renderVideos()
{
   gBinkInterface.decompressAllVideos();
   gBinkInterface.renderAllVideos();
   gBinkInterface.advanceAllVideos();
}

//==============================================================================
// BModePartyRoom2::onVideoEnded, BBinkVideoStatus interface method
//==============================================================================
void BModePartyRoom2::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   ASSERT_MAIN_THREAD

   // If for some reason we got preloaded data back (not expected here at the moment), delete it since otherwise
   // it would be orphaned.
   if(preloadedData)
   {
      delete preloadedData;
      preloadedData = NULL;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::render()
{
   // renderBegin called just before entering here
   renderVideos();

   // hack 
   bool showPartyRoom = true;
   if (mpLeaderPicker && mpLeaderPicker->isVisible())
      showPartyRoom = false;

   //-- render the flash UI
   if (mpPartyRoomScreen && showPartyRoom)
      mpPartyRoomScreen->render();

   if (mpLeaderPicker)
      mpLeaderPicker->render();

   BHandle fontHandle=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle);
//   float yh=gFontManager.getLineHeight();

   //float sx=gUI.mfSafeX1;
   //float sy=gUI.mfSafeY1;

   switch(mState)
   {
      case cStateInitializingMPSession:
      case cStateInitializingPartySessionHosting:
      {
         break;
      }

      case cStateInLANGameList:
         {
         }
         break;

      case cStateInSession:
      {
         break;
      }

      case cStateExitMode:
         //The time for debug drawtext is over
         //gFontManager.drawText(fontHandle, sx, sy, "Shutting Down Multiplayer...");
         break;
   }

   gFontManager.render2D();

   renderSafeAreas();


   // renderEnd called right after this.

}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::renderEnd()
{
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);
   
   gRender.endViewport();
   gRender.endFrame();

   if(mState==cStateStartGame || mState==cStateExitMode)
      gRenderThread.blockUntilGPUIdle();               
}




//==============================================================================
// BModePartyRoom2::isLocalPlayer
// FIXME-COOP - calls made to isLocalPlayer need to be aware that you could
//              have two local players
//==============================================================================
BOOL BModePartyRoom2::isLocalPlayer(BPartySessionPartyMember* pPartyMember) const
{
   BASSERT(pPartyMember);

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   if (!pParty || !pParty->getSession())
   {
      return FALSE;
   }

   //the host is the local player for AIs
   if (pPartyMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
   {
      return (isLocalPlayerHost());
   }

   // is this the local player?
   return (pParty->getSession()->isLocalClientID(pPartyMember->mClientID));
}


//==============================================================================
// BModePartyRoom2::isLocalPlayerHost
//==============================================================================
BOOL BModePartyRoom2::isLocalPlayerHost() const
{
//-- FIXING PREFIX BUG ID 5128
   const BPartySession* pParty = getPartySession();
//--
   if (pParty == NULL)
      return FALSE;
   // only a host can do this.
   return (pParty->isHosting());
}

//==============================================================================
// 
//==============================================================================
BPartySessionInputOwnerTypes BModePartyRoom2::lookupInputOwnerTypeFromPort(long port)
{
   if (!getPartySession())
   {
      return cBPartySessionInputOwnerTypeNotParty;
   }

   BPartySessionPartyMember* member = getPartySession()->findPartyMemberByPort(port);

   if (member)
   {
      if (member->mSettings.getPartyMemberType()==cBPartySessionPartyMemberPrimaryPlayer)
      {
         return cBPartySessionInputOwnerTypePrimary;
      }
      else if (member->mSettings.getPartyMemberType()==cBPartySessionPartyMemberSecondaryPlayer)
      {
         return cBPartySessionInputOwnerTypeSecondary;
      }
   }

/*
   BUser* user = gUserManager.getPrimaryUser();
   if (user->getPort() == port)
   {
      return cBPartySessionInputOwnerTypePrimary;
   }

   user = gUserManager.getSecondaryUser();
   if (user->getPort() == port)
   {
      return cBPartySessionInputOwnerTypeSecondary;
   }
   */

   return cBPartySessionInputOwnerTypeNotParty;
}

//==============================================================================
// 
//==============================================================================
bool BModePartyRoom2::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   BASSERT(gLiveSystem);

   //Hax to ignore input until I'm in a state where I can process it
   if (mState < cStateInLANGameList)
   {
      return true;
   }

   bool handled = false;

   BPartySessionInputOwnerTypes inputOwnerType = lookupInputOwnerTypeFromPort( port );

   if ((inputOwnerType==cBPartySessionInputOwnerTypeNotParty) && getPartySession())
   {
      //The only type of input we process from non-primary is PUSH A TO JOIN, and we can reject that if we already have a player in secondary slot
//-- FIXING PREFIX BUG ID 5131
      const BUser* user = gUserManager.getSecondaryUser();
//--
      if (user && (user->getPort()==port))
      {
         //Do we already have a secondary slot in our session?
//-- FIXING PREFIX BUG ID 5130
         const BPartySessionPartyMember* member = getPartySession()->findPartyMemberByPort(user->getPort());
//--
         if (!member)
         {
            //Ok - we do NOT have a secondary player for this machine in the session
            //Did they push the "A" button - if so add them
            if ((mState==cStateInSession) &&
                (event == cInputEventControlStart) &&
                (controlType == cButtonA)) 
            {
               if (user->getXuid()==0)
               {
                  //For now - only support signed-in users (IE: They have an XUID)
                  return true;
               }
               if (!gConfig.isDefined(cConfigAllowSplitScreenMP))
               {
                  return true;
               }
               bool retval = getPartySession()->getSession()->joinUser(user->getPort(), user->getXuid(), user->getName());
               BASSERTM(retval == true, "Failed to join secondary user to the current party session");
               return true;
            }
         }
      }    
   }

   if (gConfig.isDefined(cConfigDemo))
   {
      //For E3 - no leader picker, no team switching
      // So lets just eat all those events right here
      if ((controlType==cTriggerLeft) ||
          (controlType==cTriggerRight))
      {
         return handled;
      }
   }

   //Eat all input if we are matchmaking and launching
   if (mpSessionEvent_queryIsInMatchmakingMode() &&
       gLiveSystem->getMPSession()->isMatchmakingRunning() &&
       gLiveSystem->getMPSession()->isGameRunning())
   {
      return true;
   }

   if (getPartySession() && ( (mState >= cStateInLANGameList) ||
                              ( (mState >= cStateInLANGameList) && (inputOwnerType == cBPartySessionInputOwnerTypePrimary)) ) )
   {

      if (getPartyRoomView()==BUIMPSetupScreen::cPRViewPlayerEdit)
      {
         if (mpLeaderPicker)
            handled = mpLeaderPicker->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);
         return handled;
      }

      handled=mpPartyRoomScreen->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);

      if (handled)
         return handled;
   }

   // if it wasn't handled by our components, does the screen handle it?
   if ((mpInputHandler)  && 
       (inputOwnerType == cBPartySessionInputOwnerTypePrimary))
      handled = mpInputHandler->handleInput(port, event, controlType, detail);

   // ---------------------------- cStateEndOfModeError -------------------------------
   if (mState == cStateEndOfModeError)
   {
      if (event != cInputEventControlStart)
         return false;

      switch(controlType)
      {
         case cButtonB: 
         {
            // gUI.playClickSound(); 
            gSoundManager.playCue("play_menu_back_button");
            mNextState = cStateExitMode; 
            return true;
         }
      }
   }
   return false;
}


//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::tearDownParty()
{
   multiplayerShutdown();
   if (mpPartyRoomScreen)
      mpPartyRoomScreen->reset();
   if (mpLeaderPicker)
      mpLeaderPicker->hide();
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::initializeParty()
{
   // Init the rest of the stuff
   mPartyRoomMode = BModePartyRoom2::cPartyRoomModeInvalid;

   // initialize the audio engine for voice comms, safe to call multiple times
   BDEBUG_ASSERT(gLiveSystem);
   gLiveSystem->initVoice();
   mState = cStateInitializingMPSession;

   //Start up mpSession
   BMPSession* pMPSession = gLiveSystem->initMPSession();
   BUser* const user = gUserManager.getUser(BUserManager::cPrimaryUser);
   if ( !user || !user->getFlagUserActive())
   {
      BASSERTM(false, "BModePartyRoom2::initializeParty - No primary user?");
      mNextState=cStateEndOfModeError;
      return;
   }

   // one of these two methods will be called when the startup is done (async, but on SIM_Thread)
   //   virtual void mpSessionEvent_ESOConnectFailed( BMPSession::cMPSessionESOConnectCode failureCode );
   //   virtual void mpSessionEvent_ESOConnectComplete( BOOL restrictedNATWarning );
   if (mUseLANMode)
   {
      pMPSession->startUp(&gGame, this, gDatabase.getGameSettings(), BMPSession::mpSessionStartupModeLAN, user->getPort());
   }
   else
   {
      if (!user->isSignedIntoLive())
      {
         mNextState=cStateEndOfModeError;
         showModeExitErrorMessage(gDatabase.getLocStringFromID(25443));
         return;
      }
      else if (!user->isSignedIn())
      {
         mNextState=cStateEndOfModeError;
         mTempMessageHolder = gDatabase.getLocStringFromID(25247);
         showModeExitErrorMessage(mTempMessageHolder);
         return;
      }
      // reset the "Xbox LIVE state has changed" boolean on the user
      // if their Xbox LIVE state has changed when we attempt to return to the party then kick back to the main menu
      user->resetLiveStateChanged();
      pMPSession->startUp(&gGame, this, gDatabase.getGameSettings(), BMPSession::mpSessionStartupModeLive, user->getPort());
   }

   //Set up some defaults
   mMatchMakingSettingCiv=cMatchMakingDefaultCiv;
   mMatchMakingSettingLeader=cMatchMakingDefaultLeader;
   mCurrentHopperIndex = 0;                         //We can't have a default until AFTER the LSP based config file is loaded in 
}


//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::multiplayerShutdown()
{   
   nlog(cMPModeMenuCL, "BModePartyRoom2::multiplayerShutdown");

//   mList.clearItems();
   mLocalPlayer=-1;
   mLaunchCountdown=-1;
   mSettingCoop=false;
   mSettingRecordGame=false;
   mSettingReady=false;
   mLaunchStartRequested=false;
   mCurrentHopperIndex=0;

   if (!gLiveSystem)
      return;

   gLiveSystem->setPartySystemForReenter(false);
   gLiveSystem->disposeMPSession();
   gLiveSystem->disposeVoice();
   //Extra line needed in case that system has already been told to recycle and at that time we were trying 
   //  to preserve the party system.  Well... Now we are NOT trying to do that so we need to tell that system to 
   //  shutdown (its safe for that system to recieve that request multiple times).
   if (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession())
   {
      gLiveSystem->getMPSession()->getPartySession()->shutdown();
   }
   

   //Flush the comm logs
   // don't flush because BMPSession is still shutting down
   //BCommLog::closeLogFile();
}

//==============================================================================
//
//==============================================================================
BOOL BModePartyRoom2::mpSessionEvent_queryIsInMatchmakingMode(void)
{
   //TODO - ask shawn if we need to query from the game settings for this or can we just use the local value
   return (mPartyRoomMode == cPartyRoomModeMM);
}

//==============================================================================
//
//==============================================================================
BOOL BModePartyRoom2::mpSessionEvent_preGameInit(BOOL playerIsHost)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_preGameInit");

   BASSERT( gLiveSystem );
   BASSERT( gLiveSystem->getMPSession());

   gDatabase.resetGameSettings();

   if (!playerIsHost)
   {
      //Thats all for now for the client
      return TRUE;
   }

   gConfig.get(cConfigDefaultMap, mSettingMap);

   BSimString gameID;
   MVince_CreateGameID(gameID);

   gDatabase.getGameSettings()->setLong(BGameSettings::cPlayerCount, 0);
   gDatabase.getGameSettings()->setLong(BGameSettings::cRandomSeed, timeGetTime());
   gDatabase.getGameSettings()->setString(BGameSettings::cGameID, gameID);
   gDatabase.getGameSettings()->setString(BGameSettings::cMapName, mSettingMap);
   gDatabase.getGameSettings()->setBool(BGameSettings::cCoop, false);
   gDatabase.getGameSettings()->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
   gDatabase.getGameSettings()->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
   gDatabase.getGameSettings()->setLong(BGameSettings::cGameMode, 0);

   if (mUseLANMode)
      gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLan);
   else
      gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLive);

   return TRUE;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::hostInit()
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::hostInit");

   BPartySessionPlayerSettings playerSettings;
   fillInPlayerSettings(&playerSettings);
   gLiveSystem->getMPSession()->startPartySessionHost(&playerSettings);

// Next thing we'll hear from mpSession is one of these 2 callbacks.
//    virtual void mpSessionEvent_systemReady();
//    virtual void mpSessionEvent_joinFailed( BSession::BJoinReasonCode failureCode );
   mState=cStateInitializingPartySessionHosting;
   showWaitDialog(gDatabase.getLocStringFromID(23474)); // Initializing Party System...

   return true;
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::joinInitLan(uint descriptorIndex)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::joinInitLan");
   BASSERT(gLiveSystem); 
   BASSERT(gLiveSystem->getMPSession());

   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (!pMPSession)
      return false;

   BPartySessionPlayerSettings playerSettings;
   fillInPlayerSettings(&playerSettings);
   bool result = pMPSession->joinLANPartySession(descriptorIndex, &playerSettings);

   if (result)
   {
      mState=cStateInitializingPartySessionJoining;
      showWaitDialog(gDatabase.getLocStringFromID(23474));
   }
   else
   {
      mTempMessageHolder = gDatabase.getLocStringFromID(25251);
      showModeExitErrorMessage(mTempMessageHolder);
      setCurrentPartyAsDead();
      result = false;
   }

   return result;
}


//==============================================================================
//
//==============================================================================
void BModePartyRoom2::fillInPlayerSettings(BPartySessionPlayerSettings* playerSettings)
{
   playerSettings->mCiv = 1;
   playerSettings->mLeader = 1;
   playerSettings->mTeam = 0;
   playerSettings->mSlot = 0;
   playerSettings->mPartyRoomMode = 0;
   playerSettings->clearFlags();
   playerSettings->mPlayerRank.reset();
   playerSettings->mCampaignBits = getLocalCampaignBits();
}

//==============================================================================
//==============================================================================
uint32 BModePartyRoom2::getLocalCampaignBits()
{
   // get the campaign bits for the local user
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return 0;

   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   BASSERT(pProgress);
   if (!pProgress)
      return 0;

   uint32 bits=0;

   for (int i=0; i<pCampaign->getNumberNodes(); i++)
   {
      // get the node
      BCampaignNode* pNode = pCampaign->getNode(i);
      BASSERT(pNode);
      if (!pNode)
         continue;

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
         continue;

      // check to see if this campaign node is unlocked.
      bool isUnlocked = pProgress->isScenarioUnlocked(pNode->getID());

      if (isUnlocked)
      {
         long progressBit = pProgress->scenarioNodeIDToProgressIndex(pNode->getID());
         // set the bit to unlocked.
         bits |= (0x1<<progressBit);
      }
   }

   return bits;
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isScenarioUnlocked(const BCampaignNode* pNode)
{
   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   BASSERT(pProgress);
   if (!pProgress)
      return false;

   long progressBit = pProgress->scenarioNodeIDToProgressIndex(pNode->getID());

   uint32 bits = getSessionCampaignBits();

   if ( (bits & (0x1<<progressBit)) > 0)
      return true;

   return false;
}

//==============================================================================
//==============================================================================
uint32 BModePartyRoom2::getSessionCampaignBits()
{
   BPartySession* pParty = getPartySession();
   //It is possible during startup for this to be called and there be no party session
   //BASSERT(pParty);
   if (!pParty || !pParty->getSession())
      return 0;

   // OR everybody's bits together and return the result;
   uint32 bits = 0;

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // or the bits.
      bits |= pPartyMember->mSettings.mCampaignBits;
   }

   return bits;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::joinInit()
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::joinInit");
   BASSERT(gLiveSystem); 
   BASSERT(gLiveSystem->getMPSession());

   /*
   //PregameInit is only needed when we are starting up a game session
   if (!mpSessionEvent_preGameInit( FALSE ))
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::joinInit - mpSessionEvent_preGameInit FAILED");
      return false;
   }
   */

   bool result = true;
   BPartySessionPlayerSettings playerSettings;
   fillInPlayerSettings(&playerSettings);
   if (gLiveSystem->getMPSession()->joinLiveGameFromInvite(&playerSettings))
   {
//      mStartedInHostMode = FALSE;
      mState=cStateInitializingPartySessionJoining;
   }
   else
   {
      mTempMessageHolder = gDatabase.getLocStringFromID(25254);
      showModeExitErrorMessage(mTempMessageHolder);
      setCurrentPartyAsDead();
      result = false;
   }

   //Work or fail - lets clear the invite - we have handled it best we can currently
   gLiveSystem->clearInviteInfo();

   return result;
}

//==============================================================================
// Use this to set the party in a dead/over/no more joins/must restart/reset me state
//==============================================================================
void BModePartyRoom2::setCurrentPartyAsDead()
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::setCurrentPartyAsDead");
   mState = cStateEndOfModeError;
   BPartySession* pParty = getPartySession();
   if (pParty)
   {
      pParty->lockPartyMembers(true);
   }
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed( BSession::BJoinReasonCode failureCode )
{
   //We have failed to either host or join the party that this UI is build around 
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed");
   setCurrentPartyAsDead();
   switch (failureCode)
   {
      case BSession::cResultRejectFull:
         mTempMessageHolder = gDatabase.getLocStringFromID(25249);
         break;
      case BSession::cResultRejectCRC:
         mTempMessageHolder = gDatabase.getLocStringFromID(25250);
         break;
      case BSession::cResultRejectUserExists:
         BASSERTM(false,"BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed - cResultRejectUserExists");
         mTempMessageHolder = gDatabase.getLocStringFromID(25251);
         break;
      case BSession::cResultRejectUnknown:
         BASSERTM(false,"BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed - cResultRejectUnknown");
         mTempMessageHolder = gDatabase.getLocStringFromID(25251);
         break;
      case BSession::cResultHostNotFound:
         mTempMessageHolder = gDatabase.getLocStringFromID(25253);
         break;
      case BSession::cResultHostConnectFailure:
         mTempMessageHolder = gDatabase.getLocStringFromID(25253);
         break;
      case BSession::cResultRejectInternalError:
         BASSERTM(false,"BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed - cResultRejectInternalError");
         mTempMessageHolder = gDatabase.getLocStringFromID(25251);
         break;
      case BSession::cResultRejectConnectFailure:
         mTempMessageHolder = gDatabase.getLocStringFromID(25255);
         break;
      case BSession::cLiveSessionFailure:
         mTempMessageHolder = gDatabase.getLocStringFromID(25254);
         break;
      default:
         BSimString errorMessage; errorMessage.format("BModePartyRoom2::mpSessionEvent_partyEvent_joinFailed - UNMAPPED ERROR [%d]", failureCode);
         BASSERTM(false, errorMessage);
         mTempMessageHolder = gDatabase.getLocStringFromID(25251);
         break;
 
   }
   showModeExitErrorMessage(mTempMessageHolder);
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_gameSessionJoinFailed(BSession::BJoinReasonCode failureCode)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameSessionJoinFailed");
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty)
   {
      pParty->lockPartyMembers(false);
      onUnready();
   }
   // 25252 Failed to launch the game.  Please try again or you may need to leave and recreate your party.
   showModeModelEventMessage(gDatabase.getLocStringFromID(25252));
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_ESOConnectFailed( BMPSession::cMPSessionESOConnectCode failureCode )
{
   setCurrentPartyAsDead();
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_ESOConnectFailed %d", failureCode);
   switch (failureCode)
   {
      case BMPSession::cMPSessionESOConnectAccountNotSignedIn:
         mTempMessageHolder = gDatabase.getLocStringFromID(25440);
         break;
      case BMPSession::cMPSessionESOConnectLostLiveConnection:
         mTempMessageHolder = gDatabase.getLocStringFromID(25443);
         break;
      case BMPSession::cMPSessionESOConnectAccountHasLiveGamesBlocked:
         mTempMessageHolder = gDatabase.getLocStringFromID(23490);
         break;
      case BMPSession::cMPSessionESOConnectSerivceDown:
         mTempMessageHolder = gDatabase.getLocStringFromID(25257);
         break;
      case BMPSession::cMPSessionESOConnectCouldNotContactService:
         mTempMessageHolder = gDatabase.getLocStringFromID(25258);
         break;
      case BMPSession::cMPSessionESOConnectCouldNotRequestLogin:
         mTempMessageHolder = gDatabase.getLocStringFromID(25259);
         break;
      case BMPSession::cMPSessionESOConnectCouldNotRequestConfig:
         mTempMessageHolder = gDatabase.getLocStringFromID(25260);
         break;
      case BMPSession::cMPSessionESOConnectCouldNoValidHoppers:
         mTempMessageHolder = gDatabase.getLocStringFromID(25261);
         break;
      case BMPSession::cMPSessionESOConnectAccountBanned:
         mTempMessageHolder = gDatabase.getLocStringFromID(24110);      // account banned.
         //mTempMessageHolder.format(L"Account banned!");
         break;
      default:
         mTempMessageHolder = gDatabase.getLocStringFromID(25263);
         break;
   }

   showModeExitErrorMessage(mTempMessageHolder);
}

//==============================================================================
//============================================================================4==
void BModePartyRoom2::setupLanSettings()
{
   BPartySessionHostSettings hostSettings;

   switch (mInitialPartyRoomMode)
   {
      case cPartyRoomModeCampaign:
         {
            BCampaign * pCampaign = gCampaignManager.getCampaign(0);
            BASSERT(pCampaign);

            int8 nodeIndex = 0;
            while (true)
            {
               // increment the node
               if ( nodeIndex >= pCampaign->getNumberNodes() )
                  nodeIndex=0;

               // get the node
               BCampaignNode* pNode = pCampaign->getNode( nodeIndex );
               BASSERT(pNode);

               // for right now, skip cinematics
               if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
               {
                  nodeIndex++;
                  continue;
               }

               // we found a good node
               break;
            }
            hostSettings.mLiveMode = cBLiveSessionHostingModeOpen;
            hostSettings.mPartyRoomMode = cPartyRoomModeCampaign;
            hostSettings.mHopperIndex=0;
            // mGameOptionSettings.mDifficulty=DifficultyType::cNormal;
            hostSettings.mDifficulty=DifficultyType::cNormal;
            hostSettings.mMapIndex = nodeIndex;
            hostSettings.mNumPlayers=2;         // 2 for coop
            hostSettings.mRandomTeam = false;

            long gameMode = gDatabase.getGameModeIDByName("Skirmish");
            if (gameMode<0)
               gameMode = 0;
            hostSettings.mGameMode=(uint8)gameMode;

            hostSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;         
         }
         break;
      case cPartyRoomModeCustom:
      case cPartyRoomModeMM:        // the MM is not available on lan, so make it the custom view.
         {
            // get the custom game hopper
//-- FIXING PREFIX BUG ID 5134
            const BMatchMakingHopper* pValidHopper = NULL;
//--
            BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
            for (int i=0; i<pHopperList->getHopperCount(); i++)
            {
               // set up the text
               BMatchMakingHopper* pHopper = pHopperList->findHopperByID(i);
               if (!pHopper || pHopper->mTeamCode!=BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams)
                  continue;

               pValidHopper=pHopper;
               break;
            }

            // Did I at least find a hopper?
            if (!pValidHopper)
            {
               // Uhhhh - Eric what do I do here
            }

            // We've got our hopper, set up the room.
            mCurrentHopperIndex = pValidHopper->mListIndex;

            hostSettings.mHopperIndex = (uint8)pValidHopper->mListIndex;
            hostSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
            hostSettings.mRandomTeam = false;
            hostSettings.mLiveMode = cBLiveSessionHostingModeOpen;
            hostSettings.mDifficulty=DifficultyType::cNormal;
            if (gConfig.isDefined(cConfigDemo))
            {
               hostSettings.mDifficulty=DifficultyType::cEasy;
            }

            // set up for custom
            hostSettings.mPartyRoomMode = cPartyRoomModeCustom;
            hostSettings.mNumPlayers=2;

            uint8 mapIndex=0;
            while(true)
            {
               if (mapIndex >= gDatabase.getScenarioList().getMapCount())
               {
                  BASSERT(0);
               }

               const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
               BASSERT(pMap);
               if (!isValidMapType(pMap->getType()))
               {
                  mapIndex++;
                  continue;
               }

               if (!isValidMap((uint8)mapIndex, hostSettings.mNumPlayers))
                  continue;

               // found our map
               break;
            }

            hostSettings.mMapIndex = mapIndex;
            hostSettings.mGameMode = (uint8)pValidHopper->mGameModeIndex;
         }
         break;
   }


   gLiveSystem->getMPSession()->setInitialPartySettings(hostSettings);
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_ESOConnectComplete( BOOL restrictedNATWarning )
{  
   if (mUseLANMode)
   {
      mState = cStateInLANGameList;
      mpPartyRoomScreen->setView(BUIMPSetupScreen::cPRViewLanGameList);

      hideWaitDialog();
      mpPartyRoomScreen->setVisible(true);

      // set up my initial settings for LAN
      setupLanSettings();

      mpPartyRoomScreen->populateGamesList();
      mpPartyRoomScreen->getLanGamesList()->show();
      mpPartyRoomScreen->displayButtons();

      return;
   }

   if (restrictedNATWarning)
   {
      //TODO - loc for "Restricted NAT - only 1v1s allowed, if you host you are a retard, etc"
      mTempMessageHolder = gDatabase.getLocStringFromID(25264);
      showModeModelEventMessage(mTempMessageHolder);    
   }

   BPartySessionHostSettings hostSettings;

   // Now, set the room up based on the initial desired mode
   switch (mInitialPartyRoomMode)
   {
      case cPartyRoomModeMM:
         {
            // get the 1v1 standard hopper or if can't find that, then the first one.
//-- FIXING PREFIX BUG ID 5135
            const BMatchMakingHopper* pValidHopper = NULL;
//--
            BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
            for (int i=0; i<pHopperList->getHopperCount(); i++)
            {
               // set up the text
               BMatchMakingHopper* pHopper = pHopperList->findHopperByID(i);
               if (pHopper->mTeamCode!=BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams)
                  continue;

               if (pHopper->mPlayersPerTeam!=1)
                  continue;

               pValidHopper=pHopper;
               break;
            }

            // did I find a 1v1 matchmaking?
            if (!pValidHopper)
            {
               pValidHopper=gLiveSystem->getHopperList()->findFirstValidHopper(false);    
            }

            // Did I at least find a hopper?
            if (!pValidHopper)
            {
               // Uhhhh - Eric what do I do here
            }

            // We've got our hopper, set up the room.
            mCurrentHopperIndex = pValidHopper->mListIndex;

            hostSettings.mHopperIndex = (uint8)pValidHopper->mListIndex;
            hostSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
            hostSettings.mRandomTeam = false;
            hostSettings.mLiveMode = cBLiveSessionHostingModeOpen;
            hostSettings.mDifficulty=DifficultyType::cNormal;

            // It's a custom game hopper, set up for custom game
            if (pValidHopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams)
            {
               // set up for custom
               hostSettings.mPartyRoomMode = cPartyRoomModeCustom;
               hostSettings.mNumPlayers=2;

               uint8 mapIndex=0;
               while(true)
               {
                  if (mapIndex >= gDatabase.getScenarioList().getMapCount())
                  {
                     BASSERT(0);
                  }

                  const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
                  BASSERT(pMap);
                  if (!isValidMapType(pMap->getType()))
                  {
                     mapIndex++;
                     continue;
                  }

                  if (!isValidMap((uint8)mapIndex, hostSettings.mNumPlayers))
                     continue;

                  // found our map
                  break;
               }

               hostSettings.mMapIndex = mapIndex;
            }
            else
            {
               // set up for matchmaking
               hostSettings.mPartyRoomMode = cPartyRoomModeMM;
               hostSettings.mNumPlayers=pValidHopper->mPlayersPerTeam;
            }
            hostSettings.mGameMode = (uint8)pValidHopper->mGameModeIndex;

         }
         break;
      case cPartyRoomModeCustom:
         {
            // get the custom game hopper
            BMatchMakingHopper* pValidHopper = NULL;
            BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
            for (int i=0; i<pHopperList->getHopperCount(); i++)
            {
               // set up the text
               BMatchMakingHopper* pHopper = pHopperList->findHopperByID(i);
               if (!pHopper || pHopper->mTeamCode!=BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams)
                  continue;

               pValidHopper=pHopper;
               break;
            }

            // Did I at least find a hopper?
            if (!pValidHopper)
            {
               // Uhhhh - Eric what do I do here
            }

            // We've got our hopper, set up the room.
            mCurrentHopperIndex = pValidHopper->mListIndex;

            hostSettings.mHopperIndex = (uint8)pValidHopper->mListIndex;
            hostSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
            hostSettings.mRandomTeam = false;
            hostSettings.mLiveMode = cBLiveSessionHostingModeOpen;
            hostSettings.mDifficulty=DifficultyType::cNormal;

            // set up for custom
            hostSettings.mPartyRoomMode = cPartyRoomModeCustom;
            hostSettings.mNumPlayers=2;

            uint8 mapIndex=0;
            while(true)
            {
               if (mapIndex >= gDatabase.getScenarioList().getMapCount())
               {
                  BASSERT(0);
               }

               const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
               BASSERT(pMap);
               if (!isValidMapType(pMap->getType()))
               {
                  mapIndex++;
                  continue;
               }

               if (!isValidMap((uint8)mapIndex, hostSettings.mNumPlayers))
                  continue;

               // found our map
               break;
            }

            hostSettings.mMapIndex = mapIndex;
            hostSettings.mGameMode = (uint8)pValidHopper->mGameModeIndex;
         }
         break;
      case cPartyRoomModeCampaign:
         {
            BCampaign * pCampaign = gCampaignManager.getCampaign(0);
            BASSERT(pCampaign);

            int8 nodeIndex = 0;
            while (true)
            {
               // increment the node
               if ( nodeIndex >= pCampaign->getNumberNodes() )
                  nodeIndex=0;

               // get the node
               BCampaignNode* pNode = pCampaign->getNode( nodeIndex );
               BASSERT(pNode);

               // for right now, skip cinematics
               if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
               {
                  nodeIndex++;
                  continue;
               }

               // we found a good node
               break;
            }
            hostSettings.mLiveMode = cBLiveSessionHostingModeOpen;
            hostSettings.mPartyRoomMode = cPartyRoomModeCampaign;
            hostSettings.mHopperIndex=0;
            // mGameOptionSettings.mDifficulty=DifficultyType::cNormal;
            hostSettings.mDifficulty=DifficultyType::cNormal;
            hostSettings.mMapIndex = nodeIndex;
            hostSettings.mNumPlayers=2;         // 2 for coop
            hostSettings.mRandomTeam = false;

            long gameMode = gDatabase.getGameModeIDByName("Skirmish");
            if (gameMode<0)
               gameMode = 0;
            hostSettings.mGameMode=(uint8)gameMode;

            hostSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
         }
         break;

      default:
         // default is the same as MM
         break;
   }

   // Send the settings off to the MP session layer.
   gLiveSystem->getMPSession()->setInitialPartySettings(hostSettings);

   // decide whether I'm going to join or host
   if (gLiveSystem->isInviteInfoAvailable())
      joinInit();
   else
      hostInit();
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_ESOSearchFailed( BMPSession::cMPSessionESOSearchCode failureCode )
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_ESOSearchFailed - code %d", failureCode);
   //Reset system so that we are clear to try MM again
   if (gLiveSystem->getMPSession()->getPartySession())
   {
      gLiveSystem->getMPSession()->abortGameSession();
   }
   else
   {
      gLiveSystem->getMPSession()->reset();
   }

   //mLaunchStartRequested = false;

   //Hide the spinner
   //hideWaitDialog();

   //Unlock settings
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;
   //pParty->lockPartyMembers(false);


   //Only the party host gets these
   // He needs to the failure code - send that out as a matchmakingStatusInfo change so that it gets sent out to everyone
   //   (so the info text line will display the reason why the search stopped)
   // And change the state back to ready to start
   pParty->partySendMatchmakingStatusInfo( BLiveMatchMaking::cBLiveMatchMakingStatusCodeSearchStoppedCode, failureCode, 0);
   mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodePlayerCanceledSearch;

   //Update settings so clients see a msg
   pParty->changeHostSettings(mGameOptionSettings);
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_ESOSearchComplete()
{
   nlog(cMPModeMenuCL, "mpSessionEvent_ESOSearchComplete - when is this called?");
   mNextState=cStateLaunchInit;
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_MatchMakingStatusChanged(BLiveMatchMaking::BLiveMatchMakingStatusCode status, uint data1, uint data2)
{
   BUString displayString = "";
   switch (status)
   {
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeStartCountdown) :
      {
         //Probably overkill here - but I need to make sure the clients get this call at startup of search, not just the host
         updateMatchmakingSlots(true);
         if (data1>0)
         {
            //displayString.format("Matchmaking starting in %d...", data1);
            displayString.locFormat(gDatabase.getLocStringFromID(25455).getPtr(), data1);
            gSoundManager.playCue("play_countdown_timer");
         }
         else
         {
            //displayString.format("Matchmaking starting...");
            displayString = gDatabase.getLocStringFromID(25456);
         }
         break;
      }      
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeShowSkillInfo) :
      {
         displayString.locFormat(gDatabase.getLocStringFromID(25664).getPtr(), data1);
         mpPartyRoomScreen->setInfoText(displayString);
         return;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeCycleDelay) :
      {
         //displayString.format("Search restart in %d...", data1);
         displayString.locFormat(gDatabase.getLocStringFromID(25457).getPtr(), data1);
         break;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeFastScan) :
      {
         if (data1>0)
         {
            //displayString.format("Fast scan found %d/%d targets...", data2, data1);
            displayString.locFormat(gDatabase.getLocStringFromID(25458).getPtr(), data2, data1);
         }
         else
         {
            //displayString.format("Fast searching for games...");
            displayString = gDatabase.getLocStringFromID(25459);            
         }
         break;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeNormalScan) :
      {
         if (data1>0)
         {
            //displayString.format("Found %d/%d targets...", data2, data1);
            displayString.locFormat(gDatabase.getLocStringFromID(25460).getPtr(), data2, data1);
         }
         else
         {
            //displayString.format("Scanning for games...");
            displayString = gDatabase.getLocStringFromID(25461); 
         }
         break;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeAttemptingJoin) :
      {
         //displayString.format("Attempting join...");
         displayString = gDatabase.getLocStringFromID(25462); 
         break;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeWaitingForMorePlayers) :
      {
         //displayString.format("Waiting for players...");
         displayString = gDatabase.getLocStringFromID(25463); 
         break;
      }
   case (BLiveMatchMaking::cBLiveMatchMakingStatusCodeSearchStoppedCode) :
      {
         //We have stopped search - need to display the reason why to everyone
         switch (data1)
         {
            case BMPSession::cMPSessionESOSearchCodePlayerCanceled:
               displayString = gDatabase.getLocStringFromID(25265);
               break;
            case BMPSession::cMPSessionESOSearchCodeTimedOut:
              displayString = gDatabase.getLocStringFromID(25266);
               break;
            case BMPSession::cMPSessionESOSearchHopperError:
               displayString = gDatabase.getLocStringFromID(25267);
               break;
            case BMPSession::cMPSessionESOSearchLiveProblem:
               displayString = gDatabase.getLocStringFromID(25933);
               break;            
            case BMPSession::cMPSessionESOSearchWrongNumberofPlayers:
               displayString = gDatabase.getLocStringFromID(25271);
               break;   
            case BMPSession::cMPSessionESOSearchHostError:
               displayString = gDatabase.getLocStringFromID(25268);
               break;
            case BMPSession::cMPSessionESOSearchCodeESOConnectionLost:
               displayString = gDatabase.getLocStringFromID(25269);
               break;
            case BMPSession::cMPSessionESOSearchCodePartyNoLongerValid:
               displayString = gDatabase.getLocStringFromID(25270);
               break;
            default:
               BSimString errorMessage; errorMessage.format("BModePartyRoom2::mpSessionEvent_ESOSearchFailed - unmapped error code[%d]", data1);
               BASSERTM(false, errorMessage);
               break;
          }
         if (getPartySession())
         {
             getPartySession()->lockPartyMembers(false);
             onUnready();
         }
         mpPartyRoomScreen->resetSlotStates();
         mpPartyRoomScreen->setInfoText(displayString);
         mLaunchStartRequested = false;
         return;
      }
   default:
      {
         //Invalid code?
         BASSERT(false);
      }

   }
   mpPartyRoomScreen->setHelpText(displayString);
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_ESOSearchStatusChanged(const BSimString& description)
{
   //Deprecated
   //BASSERT(false);
   nlog(cMPModeMenuCL, "mpSessionEvent_ESOSearchStatusChanged - we can't display the message that is bubbled up from the MP layer - not loc'd.[%s]", description.getPtr());
   // fixme - this needs to be put in the "search dialog"?
   //mMessage.copy(description);

   //Don't show these messages for clients during matchmaking
   if (!isLocalPlayerHost())
   {
      if (gLiveSystem->getMPSession()->getPartySession()->getCurrentHostSettings().mPartyRoomMode == cPartyRoomModeMM )
      {
         return;
      }
   }
}

//==============================================================================
// Called only for the game session host right before the level loads so that he can set the map and teams
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_hostSubmitFinalGameSettings()
{
   //NOTE: We want to use the raw setXXXX methods directly from the database instead of against the mpSession layer
   //      This is because we do NOT want them sent out on the wire as we change each one
   //      As soon as this callback is over - the mpSession layer will do a full set of everything out to all players
   long slots = 0;
   gDatabase.getGameSettings()->getLong(BGameSettings::cMaxPlayers, slots);
   const BPartySessionHostSettings& hostSettings = gLiveSystem->getMPSession()->getPartySession()->getCurrentHostSettings();

   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_hostSubmitFinalGameSettings");

   if (hostSettings.mPartyRoomMode == cPartyRoomModeMM)
   {
//-- FIXING PREFIX BUG ID 5138
      const BMatchMakingHopper* hopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(hostSettings.mHopperIndex);
//--
      BASSERT(hopper);

      //Pick a map from the list at random - only if you are the game session host
      if (gLiveSystem->getMPSession()->isHosting())
      {
         //TODO - POSTSHIP PATCH - add support for DLC detection
         setRandSeed(cUnsyncedRand, GetTickCount());
         uint mapCount = hopper->mMapList.getSize(); 
         BASSERT(mapCount>0);
         uint mapIndex = getRandRange(cUnsyncedRand, 0, mapCount-1);
         BSimString mapName = "skirmish\\design\\" + hopper->mMapList[mapIndex] + "\\" + hopper->mMapList[mapIndex]; 
         gDatabase.getGameSettings()->setString(BGameSettings::cMapName, mapName);
      }

      gDatabase.getGameSettings()->setLong(BGameSettings::cGameStartContext, BGameSettings::cGameStartContextPartyMatchmaking);


      //Support for hoppers for random jimmys (no teams)
      if (hopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams)
      {
         //Everyone can just be on team 0 and let the scenario loader take care of assigning teams
         nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_hostSubmitFinalGameSettings - setting each player to team 0 because hopper is set to no teams");
         for (long i=1;i<=slots;i++)
         {
            gDatabase.getGameSettings()->setLong(PSINDEX(i, BGameSettings::cPlayerTeam), 0);
         }
      }
      else
      {
         //We have a party, and need to set everyone in our local party to team 1, rest of folks to team 2
         XUID xuid;    
         nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_hostSubmitFinalGameSettings - settings teams based on their party");
         BPartySession* pParty = getPartySession();
         BASSERT(pParty);
         uint partyCount = pParty->getPartyMaxSize();
         for (long gameSessionIndex=1;gameSessionIndex<=slots;gameSessionIndex++)
         {
            gDatabase.getGameSettings()->getUInt64( PSINDEX( gameSessionIndex, BGameSettings::cPlayerXUID ), xuid);         
            bool inParty = false;
            for (uint partySessionIndex=0; partySessionIndex<partyCount; partySessionIndex++)
            {
//-- FIXING PREFIX BUG ID 5137
               const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(partySessionIndex);
//--
               if (!pPartyMember)
                  continue;

               // skip people on my team
               if (pPartyMember->mXuid == xuid)
               {
                  inParty = true;
               }
            }
            if (inParty)
            {
               gDatabase.getGameSettings()->setLong(PSINDEX(gameSessionIndex, BGameSettings::cPlayerTeam), 1);
            }
            else
            {
               gDatabase.getGameSettings()->setLong(PSINDEX(gameSessionIndex, BGameSettings::cPlayerTeam), 2);
            }
         }
      }
   }
   else if (hostSettings.mPartyRoomMode == cPartyRoomModeCustom)
   {
      //Support for AI players - they are always 'owned' by the host and only supported in custom games
      BPartySession* pParty = getPartySession();
      BASSERT(pParty);
      BMPSession* pMPSession = gLiveSystem->getMPSession();
      BASSERT(pMPSession);
      uint partyCount = pParty->getPartyMaxSize();
      slots = hostSettings.mNumPlayers;
      pMPSession->setLong(BGameSettings::cMaxPlayers, slots);
      pMPSession->setLong(BGameSettings::cPlayerCount, slots);   
      // set up our starting context
      pMPSession->setLong(BGameSettings::cGameStartContext, BGameSettings::cGameStartContextPartySkirmish);

      for (uint i=0; i<partyCount; i++)
      {
         BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
         if (!pPartyMember)
            continue;

         if (pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
         {
            //Get them the next valid playerID
            //NOTE: This ONLY works if the party host is the game host - which is ALWAYS the case for custom games
            long playerIndex=cMPInvalidPlayerID;
            long startSeachIndex = 0;

            //And now, for the worst E3 hack I've put in for 2008 - eric
            //Justin needs the player positions FIXED for the 2v2 map
            //So the host human - MUST BE player 1
            //Human or computer AI that is with player 1 - must be player 2
            //Enemy AI - must be players 3 and 4
            if (gConfig.isDefined(cConfigDemo) &&
               (slots == 4))
            {
               if (pPartyMember->mSettings.mTeam==1)
               {
                  startSeachIndex = 1;
               }
               else
               {
                  startSeachIndex = 2;
               }
            }

            for (uint j=startSeachIndex; j<(uint)slots; j++)
            {
               XUID testXuid;
               gDatabase.getGameSettings()->getUInt64( PSINDEX(j+1, BGameSettings::cPlayerXUID), testXuid);  
               if (testXuid==0)
               {
                  //Found a slot
                  playerIndex=j+1;
                  break;
               }
            }

            if (playerIndex==cMPInvalidPlayerID)
            {
               BASSERT(false);
               //No empty slots???
               return;
            }

            gDatabase.resetPlayer(playerIndex);
            pMPSession->setUInt64(PSINDEX(playerIndex, BGameSettings::cPlayerXUID), pPartyMember->mXuid);
            pMPSession->setString( PSINDEX( playerIndex, BGameSettings::cPlayerName), pPartyMember->mGamerTag);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerType),BPlayer::cPlayerTypeComputerAI);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerTeam), pPartyMember->mSettings.mTeam);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerCiv), pPartyMember->mSettings.mCiv);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerLeader), pPartyMember->mSettings.mLeader);
            pMPSession->setBool(PSINDEX(playerIndex, BGameSettings::cPlayerReady), true);
            //TODO - add member setting for diff and difftype? Or is it global to the party? (see modeMenu.cpp for more info on these two - or Mike Kidd!)

            uint8 diffType = hostSettings.mDifficulty;
            float difficulty = gDatabase.getDifficultyNormal();

            switch (diffType)
            {
               case DifficultyType::cEasy:
                  difficulty=gDatabase.getDifficultyEasy();
                  break;
               case DifficultyType::cNormal:
                  difficulty=gDatabase.getDifficultyNormal();
                  break;
               case DifficultyType::cHard:
                  difficulty=gDatabase.getDifficultyHard();
                  break;
               case DifficultyType::cLegendary:
                  difficulty=gDatabase.getDifficultyLegendary();
                  break;
            }

            pMPSession->setFloat(PSINDEX(playerIndex, BGameSettings::cPlayerDifficulty), difficulty); //pPartyMember->mSettings.mDiff???);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerDifficultyType), diffType); //pPartyMember->mSettings.mDiff???);            

            // do we override the team and go all random?
            if ( (hostSettings.mPartyRoomMode == cPartyRoomModeCustom) && (hostSettings.mRandomTeam) )
               pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerTeam), 0); 
         }

         if (pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberSecondaryPlayer)
         {
            //Get them the next valid playerID
            long playerIndex=cMPInvalidPlayerID;
            for (uint j=0; j<(uint)slots; j++)
            {
               XUID testXuid;
               gDatabase.getGameSettings()->getUInt64( PSINDEX(j+1, BGameSettings::cPlayerXUID), testXuid);  
               if (testXuid==0)
               {
                  //Found a slot
                  playerIndex=j+1;
                  break;
               }
            }
            if (playerIndex==cMPInvalidPlayerID)
            {
               BASSERT(false);
               //No empty slots???
               return;
            }

            gDatabase.resetPlayer(playerIndex);
            pMPSession->setUInt64(PSINDEX(playerIndex, BGameSettings::cPlayerXUID), pPartyMember->mXuid);
            pMPSession->setString( PSINDEX( playerIndex, BGameSettings::cPlayerName), pPartyMember->mGamerTag);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerType),BPlayer::cPlayerTypeHuman);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerTeam), pPartyMember->mSettings.mTeam);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerCiv), pPartyMember->mSettings.mCiv);
            pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerLeader), pPartyMember->mSettings.mLeader);
            pMPSession->setBool(PSINDEX(playerIndex, BGameSettings::cPlayerReady), true);

            const BPartySessionHostSettings& hostSettings = pParty->getCurrentHostSettings();

            // do we override the team and go all random?
            if ( (hostSettings.mPartyRoomMode == cPartyRoomModeCustom) && (hostSettings.mRandomTeam) )
               pMPSession->setLong(PSINDEX(playerIndex, BGameSettings::cPlayerTeam), 0); 
         }
      }
   }
   else if (hostSettings.mPartyRoomMode == cPartyRoomModeCampaign)
   {
      //Need to set the difficulty settings on the human players to the desired value 
      BMPSession* pMPSession = gLiveSystem->getMPSession();
      BASSERT(pMPSession);
      //Figure out what to set it to
      uint8 diffType = hostSettings.mDifficulty;
      float difficulty = gDatabase.getDifficultyNormal();
      switch (diffType)
      {
         case DifficultyType::cEasy:
            difficulty=gDatabase.getDifficultyEasy();
            break;
         case DifficultyType::cNormal:
            difficulty=gDatabase.getDifficultyNormal();
            break;
         case DifficultyType::cHard:
            difficulty=gDatabase.getDifficultyHard();
            break;
         case DifficultyType::cLegendary:
            difficulty=gDatabase.getDifficultyLegendary();
            break;
      }

      // set up our starting context
      pMPSession->setLong(BGameSettings::cGameStartContext, BGameSettings::cGameStartContextPartyCampaign);

      //Go through and set all players to that
      for (long i=1; i<slots+1; i++)
      {      
         pMPSession->setFloat(PSINDEX(i, BGameSettings::cPlayerDifficulty), difficulty); 
         pMPSession->setLong(PSINDEX(i, BGameSettings::cPlayerDifficultyType), diffType); 
      }
   }   
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_partySizeChanged(DWORD newMaxPartyMemberCount)
{
   dropPlayersExcessPlayers((uint8)newMaxPartyMemberCount);
}

//==============================================================================
// BModePartyRoom2::mpSessionEvent_partyEvent_hostSettingsChanged
// This event is received when the host settings change. Only the host should
// be changing these parameters
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_hostSettingsChanged(BPartySessionHostSettings* newSettings)
{
   mCurrentPlayersInMatchCache=0;

   // if we have a lobby change, then we need to change our screen and go into a waiting stage.
   if (newSettings->mPartyRoomMode != mPartyRoomMode)
   {
      switchLobby(newSettings);
      mpPartyRoomScreen->setPlayerNavigationEnabled(true);
      if (isLocalPlayerHost())
      {
         // mpPartyRoomScreen->getMenu()->setFocus(0);
         mpPartyRoomScreen->getMainMenu().setIndex(0);
      }
      else
      {
         // mpPartyRoomScreen->getMenu()->clearItemFocus();
         mpPartyRoomScreen->getMainMenu().setIndex(-1);
      }
      
      if (gConfig.isDefined(cConfigDemo) && isLocalPlayerHost())
      {
         if ((mPartyRoomMode==cPartyRoomModeCustom) && 
             (getPartySession()->getPartyCount()<3))
         {
            //HACK for E3 - eric
            //Forces us in 2v2, Custom, with 2 AI's cov on the Beta team            
            addAIPlayer(3);
            if (newSettings->mNumPlayers>2)
            {
               addAIPlayer(4);
            }
         }
      }

      return;
   }
   else
   {
      // is my team and slot still valid?
      // FIXME-COOP, getLocalMember, could have two
      BPartySessionPartyMember* pMe = getLocalMember();
      if (pMe != NULL && !isValidSlot(pMe->mSettings.mSlot, pMe->mSettings.mTeam))
      {
         pMe->mSettings.mPartyRoomMode = (uint8)mPartyRoomMode;
         pMe->mSettings.mTeam = 0;
         pMe->mSettings.mSlot = cPRInvalidSlotNumber;

         pMe->mSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
         sendUpdatedLocalMemberSettings(pMe);
      }
      if (isLocalPlayerHost())
      {
         //For the host - check on the status for the AI
         BPartySession* pParty = getPartySession();
         BASSERT(pParty);
         uint partyCount = pParty->getPartyMaxSize();
         for (uint i=0; i<partyCount; i++)
         {
            BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
            if (!pPartyMember)
               continue;
            if (pPartyMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
            {
               if (!isValidSlot(pPartyMember->mSettings.mSlot, pPartyMember->mSettings.mTeam))
               {
                  pPartyMember->mSettings.mPartyRoomMode = (uint8)mPartyRoomMode;
                  pPartyMember->mSettings.mTeam = 0;
                  pPartyMember->mSettings.mSlot = cPRInvalidSlotNumber;
                  pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
                  sendUpdatedLocalMemberSettings(pPartyMember);
               }
            }
         }
      }
   }

   // only update if not editing the options.
   // hack
   if (newSettings->mHostStatusInformation==cBPartySessionHostStatusInformationCodeNone)
      updateAllGameOptions(newSettings);

   //If I am a client - check to see if I should be displaying a particular status message
   //if (!isLocalPlayerHost())
   {
      hideWaitDialog(); 
      switch(newSettings->mHostStatusInformation)
      {
         case (cBPartySessionHostStatusInformationCodeNone) : 
            {
               //hideWaitDialog(); 
               break;
            }
         case (cBPartySessionHostStatusInformationCodeInitializingMatchmakingSearch) : 
            {
               setupMatchmakingUI(newSettings);

               mTempMessageHolder = gDatabase.getLocStringFromID(25273); 
               mpPartyRoomScreen->setHelpText(mTempMessageHolder);
               if (isLocalPlayerHost())
               {
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeSearching;
                  getPartySession()->changeHostSettings(mGameOptionSettings);
               }
               break;
            }            
         case (cBPartySessionHostStatusInformationCodeSearching) : 
            {
               //mTempMessageHolder = gDatabase.getLocStringFromID(25274); 
               //mpPartyRoomScreen->setHelpText(mTempMessageHolder);
               //mpSession_MatchmakingStatusChange messages will handle helptext at this phase
               break;
            }
         case (cBPartySessionHostStatusInformationCodePlayerCanceledSearch) : 
            {
               // mpPartyRoomScreen->getMenu()->clearItemFocus();
               mpPartyRoomScreen->getMainMenu().setIndex(-1);
               updateAllGameOptions(newSettings);
               //Dont set this next line - it overwrites the more informative text provided by the MMinfo update
               //mpPartyRoomScreen->setInfoText(gDatabase.getLocStringFromID(25275));
               /*
               if (isLocalPlayerHost())
               {
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
                  getPartySession()->changeHostSettings(mGameOptionSettings);
               }
               */
               break;
            }
         /*
         case (cBPartySessionHostStatusInformationCodeErrorCanceledSearch) : 
            {
             
               // mpPartyRoomScreen->getMenu()->clearItemFocus();
               mpPartyRoomScreen->getMainMenu().setIndex(-1);

               updateAllGameOptions(newSettings);
               mTempMessageHolder = gDatabase.getLocStringFromID(25276); 
               BSimString temp("Search error.");
               updateMatchmakingStatus(temp);
               //Getting rid of this pop up since we now show this same info in the Matchmaking info panel
               //showModeModelEventMessage(mTempMessageHolder); 
               if (isLocalPlayerHost())
               {
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
                  getPartySession()->changeHostSettings(mGameOptionSettings);
               } 
               break;
            }
            */
         case (cBPartySessionHostStatusInformationCodeGameLaunchStarting) :
            {
               //Added to hide the 'launch aborted' spinner if it was up
               BUIGlobals* puiGlobals = gGame.getUIGlobals();
               if (puiGlobals)
                  puiGlobals->hideYorn();
               mTempMessageHolder = gDatabase.getLocStringFromID(25277); 
               showWaitDialog(mTempMessageHolder); 
               break;
            }
         case (cBPartySessionHostStatusInformationCodeGameRunning) :
            {
               //Added to hide the 'launch aborted' spinner if it was up
               if (gLiveSystem->isMultiplayerGameActive())
               {
                  BUIGlobals* puiGlobals = gGame.getUIGlobals();
                  if (puiGlobals)
                     puiGlobals->hideYorn();
                  mTempMessageHolder = gDatabase.getLocStringFromID(25277); 
                  showWaitDialog(mTempMessageHolder); 
               }
               break;
            }
         case (cBPartySessionHostStatusInformationCodeGameLaunchAborted) : 
            {
               // mpPartyRoomScreen->getMenu()->clearItemFocus();
               mpPartyRoomScreen->getMainMenu().setIndex(-1);

               updateAllGameOptions(newSettings);
               mTempMessageHolder = gDatabase.getLocStringFromID(25278); 
               showModeModelEventMessage(mTempMessageHolder); 
               if (isLocalPlayerHost())
               {
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
                  getPartySession()->changeHostSettings(mGameOptionSettings);
               }

               //Lets UN green up everyone
               if (getPartySession())
               {
                  getPartySession()->lockPartyMembers(false);
                  onUnready();
               }
               mpPartyRoomScreen->resetSlotStates();
               mLaunchStartRequested = false;
               break;
            }
         case (cBPartySessionHostStatusInformationCodePostGameResetUI):
            {
               if (!isLocalPlayerHost())
               {
                  BUIGlobals* puiGlobals = gGame.getUIGlobals();
                  BASSERT(puiGlobals);
                  if (puiGlobals)
                  {
                     puiGlobals->hideYorn();
                  }
                  BPartySessionPartyMember* pLocalMember = getLocalMember();
                  if (pLocalMember)
                  {
                     mpPartyRoomScreen->setPlayerCurrentSlotFocus(pLocalMember->mSettings.mSlot);
                  }
                  mpPartyRoomScreen->setPlayerNavigationEnabled(true);
                  setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
               }       
               break;
            }
      }
   }
}

//==============================================================================
// BModePartyRoom2::mpSessionEvent_partyEvent_memberSettingsChanged
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_memberSettingsChanged(BPartySessionPartyMember* pPartyMember)
{
   mCurrentPlayersInMatchCache=0;

   if (!getPartySession() || !getPartySession()->getSession())
   {
      //There is no party session active - stop processing changes
      return;
   }

   //Check for need to show restricted NAT warning
   bool restricted = pPartyMember->mSettings.isRestrictedNAT();
   if (!pPartyMember->mRestrictedNAT && 
       restricted && 
       !getPartySession()->getSession()->isLocalClientID(pPartyMember->mClientID))
   {
      //Show the warning
      mTempMessageHolder.locFormat(gDatabase.getLocStringFromID(25279), pPartyMember->mGamerTag.getPtr()); 
      showModeModelEventMessage(mTempMessageHolder); 
   }
   //Note - no code here to show a happy message that they are no longer restricted but currently we only check for this condition once
   //  it is unlikely to change while they are running
   pPartyMember->mRestrictedNAT = restricted;

   //If it is data about me - then clear the team request pending flag
   if (isLocalPlayer(pPartyMember) )
   {
      mTeamRequestChangePending = false;
   }

   //If the player's slot changed - and the UI is down in the PlayerList area, then let it update its focus
   if ( isLocalPlayer(pPartyMember) &&
        pPartyMember->mSlotChanged)
   {
      pPartyMember->mSlotChanged = false;
      if (getPartyRoomView()==BUIMPSetupScreen::cPRViewPlayerList)
      {
         mpPartyRoomScreen->setPlayerCurrentSlotFocus(pPartyMember->mSettings.mSlot);
      }
   }
   /*
   if ( isLocalPlayer(pPartyMember) )
   {
      switch(pPartyMember->mSettings.mConnectionState)
      {
         case cBPartySessionMemberConnectionStateJoining:               // The host needs to send them their initial settings
            break;
         case cBPartySessionMemberConnectionStateSlotAssigned:          // The member has just been assigned a slot (and team)
            // I've just gotten my slot, let's update our focus to me
            // mCurrentPlayerSlot=pPartyMember->mSettings.mSlot;
            if (getPartyRoomView()==BUIMPSetupScreen::cPRViewPlayerList)
            {
               mpPartyRoomScreen->setPlayerCurrentSlotFocus(pPartyMember->mSettings.mSlot);
            }
            if (!isLocalPlayerHost())
               return;
            break;
         case cBPartySessionMemberConnectionStateConnected:             // They are fully connected
            break;
         case cBPartySessionMemberConnectionStateCancelSwitchTeams:     // They want to cancel their team switch
            break;                                                     
         case cBPartySessionMemberConnectionStateWantToSwitchTeams:     // They are fully connected and the player wants to switch sides
//          case cBPartySessionMemberConnectionStateWantToSwitchTeamsLeft:     // They are fully connected and the player wants to switch sides
//          case cBPartySessionMemberConnectionStateWantToSwitchTeamsRight:     // They are fully connected and the player wants to switch sides
            break;
         case cBPartySessionMemberConnectionStateReadyToStart:          // They are fully connected and the player has "greened-up"
            break;
      }
   }
   */

   //nlog(cMPModeMenuCL, "PRECHECK - %s - slot %d - State %d", pPartyMember->mGamerTag.getPtr(), pPartyMember->mSettings.mSlot, pPartyMember->mSettings.mConnectionState );
   if (isLocalPlayerHost())
   {
      switch(pPartyMember->mSettings.mConnectionState)
      {
         case cBPartySessionMemberConnectionStateJoining:               // The host needs to send them their initial settings
            assignTeamAndSlot(pPartyMember);
            return;
            break;
            /*
         case cBPartySessionMemberConnectionStateSlotAssigned:          // The member has just been assigned a slot (and team)
            {
               pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected;
               BPartySession* pParty = getPartySession();
               BASSERT(pParty);
               nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_memberSettingsChanged - Changing player %s from state SlotAssign to StateConnected", pPartyMember->mGamerTag.getPtr() );
               pParty->broadcastChangeSettings(pPartyMember->mSettings, pPartyMember->mXuid);
               return;
            }
            break;
            */
         case cBPartySessionMemberConnectionStateConnected:             // They are fully connected
            break;
         case cBPartySessionMemberConnectionStateCancelSwitchTeams:     // They want to cancel their team switch
            cancelTeamSwitchRequest(pPartyMember);
            break;                                                     
         case cBPartySessionMemberConnectionStateWantToSwitchTeams:     // They are fully connected and the player wants to switch sides
//          case cBPartySessionMemberConnectionStateWantToSwitchTeamsLeft:     // They are fully connected and the player wants to switch sides
//          case cBPartySessionMemberConnectionStateWantToSwitchTeamsRight:     // They are fully connected and the player wants to switch sides
            updateTeams();
            break;
         case cBPartySessionMemberConnectionStateReadyToStart:          // They are fully connected and the player has "greened-up"
            break;
      }

      //Moved to the update loop to be checked each frame in case of delayed responce to everyone greened up
      //Check it here as well for the event where the client unreadys/readies in the same frame - eric
      checkForGreenUpTriggers();          // now returns true if can start or stops the start if an abort has been requested.

   }

   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_memberSettingsChanged - Got updated settings for %s (slot %d)", pPartyMember->mGamerTag.getPtr(), pPartyMember->mSettings.mSlot );
   refreshPlayerList();
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_playerJoined(XUID xuid)
{
   BPartySession* pParty = getPartySession();
   if (!pParty)
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_playerJoined - Error, I have a player joined event without having a party, am I on my way to being deleted or something?" );
      return;
   }

   // Get the player
   BPartySessionPartyMember* pPartyMember = pParty->findPartyMemberByXUID(xuid);
   if (!pPartyMember)
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_playerJoined - Error, I have a player joining (XUID:%d) but cannot find a party member record for them", xuid );
      return;
   }

   // update the GamerPic in the cache
   updateGamerPic(pPartyMember);

   // if I'm not the host, just ignore this event
   //  The UI (and other clients) watch the settings changed event for folks
   if (!pParty->isHosting())
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_partyEvent_playerJoined - Ignoring this because I am NOT the host." );
      return;
   }

   if (gConfig.isDefined(cConfigDemo) && pPartyMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
   {
      //AI has been added for DEMO mode
      pPartyMember->mSettings.mPartyRoomMode = pParty->getCurrentHostSettings().mPartyRoomMode;
   }
   else
   {
      //Give him default settings - TODO - merge this with the setup default settings from BPartySession (or get rid of those there)
      //pPartyMember->mSettings.mCiv = 1;
      //pPartyMember->mSettings.mLeader = 1;
      pPartyMember->mSettings.mPartyRoomMode = pParty->getCurrentHostSettings().mPartyRoomMode;
   }

   //Changed so that now the host waits for the client to request this AFTER the client is fully connected to everyone
   //The client signals it needs it by sending a member data update to JUST THE HOST with his partyRoomMode set to joining
   //The host sees that - then assigns him to a correct room, and spams that change out to everyone
   //The only exception to this is for the host itself (since it is fully connected when it starts hosting)
   //NO EXCEPTIONS! - everyone works the same way
   /*
   BASSERT(pParty->getSession());
   if (newClientID == (ClientID)pParty->getSession()->getHostID())
   {
      assignTeamAndSlot(pPartyMember);
   }
   */
}

//==============================================================================
// BModePartyRoom2::cancelTeamSwitchRequest
//==============================================================================
void BModePartyRoom2::cancelTeamSwitchRequest(BPartySessionPartyMember* pPartyMember)
{
   BASSERT(pPartyMember->mSettings.mConnectionState==cBPartySessionMemberConnectionStateCancelSwitchTeams);

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   if (!pParty->isHosting())
      return;

   // set this dude to connected
   // pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected;

   nlog(cMPModeMenuCL, "BModePartyRoom2::cancelTeamSwitchRequest - Changing %s to state SlotConnected?", pPartyMember->mGamerTag.getPtr() );
   pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;
   if (isLocalPlayer(pPartyMember))
   {
      pPartyMember->mSlotChanged = true;
   }

   // send these settings around to everybody
   pParty->broadcastChangeSettings(pPartyMember->mSettings, pPartyMember->mXuid);
}

//==============================================================================
// BModePartyRoom2::assignTeamAndSlot
//==============================================================================
void BModePartyRoom2::assignTeamAndSlot(BPartySessionPartyMember* pPartyMember)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   if (!pParty->isHosting())
      return;

   // if the other player is not in the same lobby as me, then don't count him. He still has to switch
   if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
      return;

   // set up the player's Team
   uint8 team = getNextTeam();

   // set up the player's slot
   uint8 slot = getOpenSlot(pPartyMember, team);

   // set this dude to ready
   pPartyMember->mSettings.mTeam = team;
   pPartyMember->mSettings.mSlot = slot;
   pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;

   // send these settings around to everybody
   pParty->broadcastChangeSettings(pPartyMember->mSettings, pPartyMember->mXuid);
}

//==============================================================================
// BModePartyRoom2::updateVoice
//==============================================================================
void BModePartyRoom2::updateVoice()
{
   BLiveVoice* pVoice = gLiveSystem->getLiveVoice();
   if (!pVoice)
      return;

   BPartySession* pParty = getPartySession();
   if (pParty == NULL)
      return;

   int slotTalking[cPRNumPlayerSlots] = { BUIGamerTagLongControl::cSpeakerOff, BUIGamerTagLongControl::cSpeakerOff, BUIGamerTagLongControl::cSpeakerOff,
                                          BUIGamerTagLongControl::cSpeakerOff, BUIGamerTagLongControl::cSpeakerOff, BUIGamerTagLongControl::cSpeakerOff};

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5139
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      if (pPartyMember->mXuid==0)
         continue;

      // is this player in the same lobby as me? No? -> don't show him, he still needs to switch
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // does he have slot assigned?
      if (pPartyMember->mSettings.mSlot == cPRInvalidSlotNumber)
         continue;

      // is this player in a "joining" state
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      // talking?
      if (pVoice->isTalking(BVoice::cPartySession, pPartyMember->mClientID))
         slotTalking[pPartyMember->mSettings.mSlot]=BUIGamerTagLongControl::cSpeakerSpeaking;

      // isMuted always returns true for yourself
      //
      // isMuted will return true in the most restrictive sense, meaning muted from the blade or because of TCR #90 or the session
      //
      // If you want to display something different in the UI for an explicit mute vs. the TCR, you can use the
      // value set in the tcr90Mute
      //
      // Or if you've simply muted them for the session, isMuted will return true and you can check the value of sessionMute
      //
      // More examples:
      // Blocked Communications:
      //    * returns true and tcr90Mute is true
      // Friends Only Communications:
      //    * returns true and tcr90Mute is true if the party member is not your friend
      // Everyone Communications:
      //    * returns true and tcr90Mute is false if you've explicitly muted the party member from the blade
      //
      // Note, for co-op split screen play we'll need to use the correct user's controller port
      BOOL tcr90Mute;
      BOOL sessionMute;
      if (pVoice->isMuted(BVoice::cPartySession, gUserManager.getPrimaryUser()->getPort(), pPartyMember->mClientID, pPartyMember->mXuid, tcr90Mute, sessionMute) ||
          !pVoice->isHeadsetPresent(BVoice::cPartySession, pPartyMember->mClientID))
      {
         // muting overrides talking status.
         slotTalking[pPartyMember->mSettings.mSlot]=BUIGamerTagLongControl::cSpeakerMuted;
      }
   }

   bool updateVoiceInFlash=false;

   // now compare against the last known status of the slots and update the ones that have changed.
   // maybe we can make a call that turns them all on / off instead of up to cPRNumPlayerSlot calls.
   for (uint i=0; i<cPRNumPlayerSlots; i++)
   {
      if (mVoiceSlots[i] != slotTalking[i])
      {
         mVoiceSlots[i] = slotTalking[i];
         updateVoiceInFlash=true;
      }
   }

   if (updateVoiceInFlash)
   {
         mpPartyRoomScreen->setPlayerVoiceStates(mVoiceSlots[0], mVoiceSlots[1], mVoiceSlots[2], mVoiceSlots[3], mVoiceSlots[4], mVoiceSlots[5]);
   }

}

//==============================================================================
// BModePartyRoom2::updatePlayerSlots
//==============================================================================
void BModePartyRoom2::updatePlayerSlots()
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint8 slotWithFocus=0;

//-- FIXING PREFIX BUG ID 5141
   const BPartySessionPartyMember* pLocalMember=getLocalMember();
//--

   bool slotUpdated[cPRNumPlayerSlots] = {false, false, false, false, false, false };

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      if (pPartyMember->mXuid==0)
         continue;

      // is this player in the same lobby as me? No? -> don't show him, he still needs to switch
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // does he have slot assigned?
      if (pPartyMember->mSettings.mSlot == cPRInvalidSlotNumber)
         continue;

      // is this player in a "joining" state
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      // indicate we updated this slot
      slotUpdated[pPartyMember->mSettings.mSlot]=true;
      displayPlayerSlot(pPartyMember);

      // is this guy local?
      if (pLocalMember && (pLocalMember->mClientID == pPartyMember->mClientID))
         slotWithFocus=pPartyMember->mSettings.mSlot;
   }

   for (uint8 i=0; i<cPRNumPlayerSlots; i++)
   {
      if (slotUpdated[i])
         continue;

      // clear the slot of any data
      clearPlayerSlot(i);
   }

   //Clear this because if this method got called, then it probably changed the UI data in the slots
   //  If matchmaking is going still - then we need to overwrite that - eric
   mCurrentPlayersInMatchCache=0;

   mpPartyRoomScreen->refreshPlayerSlots();
}

//==============================================================================
// BModePartyRoom2::displayPlayerSlot
//==============================================================================
void BModePartyRoom2::displayPlayerSlot(BPartySessionPartyMember* pMember)
{
   // this section is woefully ineffecient.
   int team0=0;
   int team1=0;
   int team2=0;

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5142
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      // is this player ready?
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      switch (pPartyMember->mSettings.mTeam)
      {
      case 0:
         team0++;
         break;
      case 1:
         team1++;
         break;
      case 2:
         team2++;
         break;
      }
   }

//    bool team1Open=(team1<(pParty->getCurrentHostSettings().mNumPlayers/2));
//    bool team2Open=(team2<(pParty->getCurrentHostSettings().mNumPlayers/2));

   BUIPlayer* pUiPlayer=mpPartyRoomScreen->getPlayer(pMember->mSettings.mSlot);
   if (!pUiPlayer)
      return;

   if (mpPartyRoomScreen)
   {
      int ping = (int)(pMember->mQoSValue/20)+1; // 1 - 100, 1 = good, 100 = crappy
      if (ping > 5)
         ping = 5;
      if (ping < 1)
         ping = 1;

      bool inCenter = (pMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateWantToSwitchTeams);

      pUiPlayer->mControllerPort=-1;
      pUiPlayer->mSlot=pMember->mSettings.mSlot;

      

      if (isLocalPlayer(pMember))
      {
         ping = 1;   // fantastic ping. :)

         // see if they are AI or human
         if (pMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
         {
            pUiPlayer->mSlotType=BUIPlayer::cAI;
            pUiPlayer->mControllerPort=4; // hack
         }
         else
         {
            pUiPlayer->mSlotType=BUIPlayer::cLocalHuman;
            pUiPlayer->mControllerPort=(int8)pMember->mPort;
         }
      }
      else
      {
         if (pMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
         {
            pUiPlayer->mSlotType=BUIPlayer::cAI;
            pUiPlayer->mControllerPort=4;
         }
         else
         {
            pUiPlayer->mSlotType=BUIPlayer::cNonLocalHuman;
            pUiPlayer->mControllerPort=5; // hack
         }
      }


      pUiPlayer->mGamerTag.set(pMember->mGamerTag.getPtr());
      pUiPlayer->mRank = pMember->mSettings.mPlayerRank.mValue;
      pUiPlayer->mLeader = pMember->mSettings.mLeader;
      pUiPlayer->mCiv = pMember->mSettings.mCiv;
      pUiPlayer->mReady = isPlayerReady(pMember);
      pUiPlayer->mTeam = pMember->mSettings.mTeam;
      pUiPlayer->mPing = (uint8)ping;
      pUiPlayer->mInCenter=inCenter;
//       pUiPlayer->mArrowLeft=team1Open;
//       pUiPlayer->mArrowRight=team2Open;
      pUiPlayer->mArrowLeft=false;
      pUiPlayer->mArrowRight=false;
      pUiPlayer->mHost=pMember->mSettings.isSessionHost();
//       pUiPlayer->mGamerPic.format("img://gamerPic:%I64x", pMember->mXuid );

//-- FIXING PREFIX BUG ID 5143
      const BLeader* pLeader=gDatabase.getLeader(pMember->mSettings.mLeader);
//--
      if(pLeader)
      {
         pUiPlayer->mLeaderStringIDIndex=pLeader->mNameIndex;
      }
      else
      {
         // random UNSC or Covenant?
         if (gDatabase.getCivID("UNSC") == pMember->mSettings.mCiv)
         {
            // random unsc
            pUiPlayer->mLeaderStringIDIndex=gDatabase.getLocStringIndex(23718);
         }
         else if (gDatabase.getCivID("Covenant") == pMember->mSettings.mCiv)
         {
            // random covenant
            pUiPlayer->mLeaderStringIDIndex=gDatabase.getLocStringIndex(23720);
         }
         else
         {
            // random random
            pUiPlayer->mLeaderStringIDIndex=gDatabase.getLocStringIndex(23722);
         }
      }



      //fixme
      //pUiPlayer->mVoice=0;              // voice state
      //pUiPlayer->mRank=0
      //pUiPlayer->mID=0;

   }
}

//==============================================================================
// BModePartyRoom2::addAIPlayer
//==============================================================================
void BModePartyRoom2::addAIPlayer(uint8 slot)
{
   BPartySession* partySession = getPartySession();
   BASSERT(partySession);

   // SRL: 10/22/08 
   // Note: Since the network layer isn't built to handle unicode strings, we will handle this on the back end display side.
   BSimString gamerTag;
   gamerTag = "NonDisplay: AI Player";    // hard-coded, but OK because the display code handles this.

   BPartySessionPlayerSettings settings;
   settings.mCiv=-1;
   settings.mLeader=-1;
   settings.mSlot=mpPartyRoomScreen->getPlayerCurrentSlotFocus();

   if (gConfig.isDefined(cConfigDemo))
   {
      settings.mSlot = slot;
      if (slot>2)
      {
         settings.mCiv = 2;
         settings.mLeader = 6;
      }
      else
      {
         settings.mCiv = 1;
         settings.mLeader = 1;
      }
   }

   uint8 team=1;
   if (mPartyRoomMode == cPartyRoomModeCustom)
   {
      // is this slot on the side of team 2?
      if (settings.mSlot>BUIMPSetupScreen::cMaxSlotNumCol1)
         team=2;
   }
   settings.mTeam=team;
   settings.mPartyRoomMode=(uint8)mPartyRoomMode;
   
   if (gConfig.isDefined(cConfigDemo))
   {
      //Hmm - lets check and make sure that slot is open
      if (!isValidSlot(settings.mSlot, settings.mTeam))
      {
         return;
      }
      if (!isOpenSlot(settings.mSlot))
      {
         return;
      }
      /*
      //Set the name based on the slot for the E3 demo - eric
      if (slot==1)
      {
         gamerTag = "Jimmy McAlly";
      }
      else if (slot==3)
      {
         gamerTag = "Cool on a Stick";
      }
      else if (slot==4)
      {
         gamerTag = "Furious Lemon";
      }
      */
   }

   // this AI player gets a specific slot at this point.
   settings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;
   partySession->addNonNetworkedPlayer(cBPartySessionPartyMemberAI, gamerTag, settings);

}


//==============================================================================
// BModePartyRoom2::clearPlayerSlot
//==============================================================================
void BModePartyRoom2::clearPlayerSlot(uint8 slot)
{
   mpPartyRoomScreen->clearPlayerSlot(slot);
}

//==============================================================================
// BModePartyRoom2::checkForGreenUpTriggers
// This is from the old canStartGame code - modified so that it checks for folks aborting the current game start/matchmaking start
//==============================================================================
bool BModePartyRoom2::checkForGreenUpTriggers()
{
   //Only the host needs to run this
   if (!isLocalPlayerHost())
      return false;

   //Get a count of how many folks are on each team
   int team0=0;
   int team1=0;
   int team2=0;

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5144
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      // is this player ready?
      if (pPartyMember->mSettings.mConnectionState != cBPartySessionMemberConnectionStateReadyToStart)
         continue;

      switch (pPartyMember->mSettings.mTeam)
      {
      case 0:
         team0++;
         break;
      case 1:
         team1++;
         break;
      case 2:
         team2++;
         break;
      }
   }

//   int totalPlayers = team1+team2+team0;
   int totalPlayers = team1+team2;

   //Analyze those counts based on the mode we are in
   switch (mPartyRoomMode)
   {
   case cPartyRoomModeCustom:
      {
         // check to see a) if we have enough players, b) if we have even teams

         // we have even teams and we have the correct amount of players
         // shawn center column fix
//         bool okToStartGame = ( (team1==team2) && (pParty->getCurrentHostSettings()->mNumPlayers == totalPlayers) );
         bool okToStartGame = ( pParty->getCurrentHostSettings().mNumPlayers == totalPlayers );
         if (okToStartGame && !mLaunchStartRequested)
         {
            startGame();
         }
         else if (!okToStartGame && mLaunchStartRequested)
         {
            //Then we need to STOP this game start
            if (!gLiveSystem->getMPSession()->requestLaunchAbort(BMPSession::cLaunchAbortUser))
            {
               nlog(cMPModeMenuCL, "BModePartyRoom2::checkForGreenUpTriggers - trying to abort game launch, but that request is being refused");
            }
            else
            {
               //Clear host UI spinner
               hideWaitDialog();
               mLaunchStartRequested=false;
            }
         }
      }
      break;
   case cPartyRoomModeCampaign:
      {
         // check to see a) if we have enough players (2), and b) if they are on the same team

         // we have even teams and we have the correct amount of players
         bool okToStartGame = ( (team1==2) && (totalPlayers==2) );

         if (okToStartGame && !mLaunchStartRequested)
         {
            //Then we need to start the game
            startGame();
         }
         else if (!okToStartGame && mLaunchStartRequested)
         {
            //Then we need to STOP this game start
            if (!gLiveSystem->getMPSession()->requestLaunchAbort(BMPSession::cLaunchAbortUser))
            {
               nlog(cMPModeMenuCL, "BModePartyRoom2::checkForGreenUpTriggers - trying to abort game launch, but that request is being refused");
            }
         }
      }
      break;
   case cPartyRoomModeMM:
      {
         //Check that everyone is on team 1, and that we have the correct number of players for this lobby
         bool okToStartMatchMaking = ((team2 == 0) && 
            (totalPlayers==team1) &&
            (pParty->getCurrentHostSettings().mNumPlayers == totalPlayers));

         if (okToStartMatchMaking && !mLaunchStartRequested)
         {
            //Then we need to start the game
            startGame();
         }
         else if (!okToStartMatchMaking && mLaunchStartRequested)
         {
            //Then we need to STOP this game start
            if (!gLiveSystem->getMPSession()->abortMatchMaking())
            {
               nlog(cMPModeMenuCL, "BModePartyRoom2::checkForGreenUpTriggers - trying to abort matchmaking, but that request is being refused");
            }
         }
      }
      break;
   }

   return false;
}


//==============================================================================
// void BModePartyRoom2::updateTeams()
//==============================================================================
void BModePartyRoom2::updateTeams()
{
   // iterate over the players and find open slots and players that want to switch
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      // is there one here?
      BPartySessionPartyMember* pMember = pParty->getPartyMember(i);
      if (!pMember)
         continue;

      // does this guy want to switch teams?
      if (pMember->mSettings.mConnectionState != cBPartySessionMemberConnectionStateWantToSwitchTeams)
         continue;

      // what team does he want to go to?
      uint8 desiredTeam = 2;
      if (pMember->mSettings.mTeam==2)
         desiredTeam=1;

      // look for an open slot first
      uint8 slot = getOpenSlot(pMember, desiredTeam);
      if (isValidSlot(slot, desiredTeam))
      {
         // found one, update their info and send the data out
         pMember->mSettings.mSlot=slot;
         pMember->mSettings.mTeam=desiredTeam;
         pMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;
         pParty->broadcastChangeSettings(pMember->mSettings, pMember->mXuid);
         if (isLocalPlayer(pMember))
         {
            pMember->mSlotChanged = true;
         }
         // continue to look for other players that want to switch
         continue;
      }

      // try find somebody on another team that wants to switch
      BPartySessionPartyMember* pOther = findPlayerToSwitchTeams(pMember);
      if (pOther)
      {
         // found one, lets switch
         uint8 temp;
         temp = pMember->mSettings.mTeam;
         pMember->mSettings.mTeam = pOther->mSettings.mTeam;
         pOther->mSettings.mTeam = temp;

         temp = pMember->mSettings.mSlot;
         pMember->mSettings.mSlot = pOther->mSettings.mSlot;
         pOther->mSettings.mSlot = temp;

         pMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;
         pOther->mSettings.mConnectionState = cBPartySessionMemberConnectionStateConnected; //cBPartySessionMemberConnectionStateSlotAssigned;

         nlog(cMPModeMenuCL, "BModePartyRoom2::updateTeams - Swapping members %s and %s", pMember->mGamerTag.getPtr(), pOther->mGamerTag.getPtr() );
         pParty->broadcastChangeSettings(pMember->mSettings, pMember->mXuid);
         pParty->broadcastChangeSettings(pOther->mSettings, pOther->mXuid);
         if (isLocalPlayer(pMember))
         {
            pMember->mSlotChanged = true;
         }
         if (isLocalPlayer(pOther))
         {
            pOther->mSlotChanged = true;
         }
      }

      // see if we have any more players that need to switch teams
   }
}


//==============================================================================
// BModePartyRoom2::findPlayerToSwitchTeams
//==============================================================================
BPartySessionPartyMember* BModePartyRoom2::findPlayerToSwitchTeams(BPartySessionPartyMember* pMember)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   BPartySessionPartyMember* firstAIPlayer = NULL;
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      if ((pPartyMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI) &&
          (firstAIPlayer==NULL))
      {
         firstAIPlayer = pPartyMember;
      }

      // skip people on my team
      if (pPartyMember->mSettings.mTeam == pMember->mSettings.mTeam)
         continue;

      // skip people not on a team
      if (pPartyMember->mSettings.mTeam == 0)
         continue;

      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateWantToSwitchTeams)
         return pPartyMember;
   }

   //If I went through an nobody wants to switch - lets have an AI switch if there is one
   if (firstAIPlayer && gConfig.isDefined(cConfigDemo))
   {
      return firstAIPlayer;
   }

   return NULL;
}

//==============================================================================
// BModePartyRoom2::isValidSlot
//==============================================================================
bool BModePartyRoom2::isValidSlot(uint8 slot, uint8 team)
{
   const BPartySessionHostSettings* pHostSettings = getHostSettings();
   
   // JAR - if the party doesn't exist, there won't be any host 
   // settings. This probably shouldn't happen if we get to this point, so
   // assert to catch in playtest, then attempt a graceful recovery.
   BASSERT(pHostSettings);
   if(!pHostSettings)
      return false;

   uint8 numPlayersPerTeam = pHostSettings->mNumPlayers;
   if (pHostSettings->mPartyRoomMode==cPartyRoomModeCustom)
      numPlayersPerTeam /= 2;

   // slots are 0 based.
   if ( (team==1) && (slot>=numPlayersPerTeam))
      return false;

   if ( (team==2) && (slot>=(3+numPlayersPerTeam)))
      return false;

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::isOpenSlot(uint8 slot)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5146
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      // skip people not on a team
      if (pPartyMember->mSettings.mSlot == slot)
         return false;
   }

   return true;
}

//==============================================================================
//
//==============================================================================
uint8 BModePartyRoom2::getNextTeam()
{
   if (mPartyRoomMode!=cPartyRoomModeCustom)
   {
      // if we are not in a custom game, then everybody in the party is on team 1
      return 1;
   }

   int team0=0;
   int team1=0;
   int team2=0;

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5147
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      // Is this player in my lobby, no? don't count him.
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // Is this player in a connected state? No, 
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      switch (pPartyMember->mSettings.mTeam)
      {
         case 0:
            team0++;
            break;
         case 1:
            team1++;
            break;
         case 2:
            team2++;
            break;
      }
   }

   // put on team 
   uint8 nextTeam = 1;
   if (team1 > team2)
      nextTeam = 2;

   return nextTeam;
}



//==============================================================================
//==============================================================================
uint8 BModePartyRoom2::getOpenSlot(BPartySessionPartyMember* pMember, uint8 desiredTeam)
{
   // 12 total slots
   bool slotAssignments[cPRNumPlayerSlots] = {false, false, false, false, false, false };

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
//-- FIXING PREFIX BUG ID 5148
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if (!pPartyMember)
         continue;

      // skip people not on a team
      if (pPartyMember->mSettings.mTeam == 0)
         continue;

      // Is this player in my lobby, no? don't count him.
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // Is this player in a connected state? No, don't count him
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      // is this person on my team, no, don't count him
      if (pPartyMember->mSettings.mTeam != desiredTeam)
         continue;

      // mark this slot as used.
      slotAssignments[pPartyMember->mSettings.mSlot] = true;
   }

   uint8 openSlot=3;        // init to an invalid slot for team 1
   uint8 firstSlot=0;
   uint8 lastSlot=2;
   if (desiredTeam==2)
   {
      firstSlot=3;
      lastSlot=5;
      openSlot=7;             // init to an invalid slot for team 2
   }

   for (uint8 i=firstSlot; i<=lastSlot; i++)
   {
      // is this slot open?
      if (slotAssignments[i]==false)
      {
         // slot is open, assign this one
         openSlot=i;
         break;
      }
   }

   return openSlot;
}
/*
uint8 BModePartyRoom2::getOpenSlot(BPartySessionPartyMember* pMember, uint8 desiredTeam)
{
   bool slotAssignments[cPRNumPlayerSlots] = {false, false, false, false, false, false };

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // skip people not on a team
      if (pPartyMember->mSettings.mTeam == 0)
         continue;

      // Is this player in my lobby, no? don't count him.
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // Is this player in a connected state? No, don't count him
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      // is this person on my team, no, don't count him
      if (pPartyMember->mSettings.mTeam != desiredTeam)
         continue;

      // mark this slot as used.
      slotAssignments[pPartyMember->mSettings.mSlot] = true;
   }

   uint8 openSlot=3;        // init to an invalid slot for team 1
   uint8 firstSlot=0;
   uint8 lastSlot=2;
   if (desiredTeam==2)
   {
      firstSlot=3;
      lastSlot=5;
      openSlot=7;             // init to an invalid slot for team 2
   }

   for (uint8 i=firstSlot; i<=lastSlot; i++)
   {
      // is this slot open?
      if (slotAssignments[i]==false)
      {
         // slot is open, assign this one
         openSlot=i;
         break;
      }
   }

   return openSlot;
}*/


//==============================================================================
// BModePartyRoom2::mpSessionEvent_partyEvent_playerLeft
// - handle this event which signals that a new member has left the party session
//==============================================================================
//void BModePartyRoom2::mpSessionEvent_partyEvent_playerLeft(ClientID newClientID, uint newMemberIndex)
void BModePartyRoom2::mpSessionEvent_partyEvent_playerLeft(XUID xuid)
{
   checkCampaignScenarios();
   //gGamerPicManager.removeGamerPic(xuid);

   // Refresh the player list.
   refreshPlayerList();
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::checkCampaignScenarios()
{
   if (!isLocalPlayerHost())
      return;

   // only check this on the host
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (!pParty)
      return;

   if (pParty->getCurrentHostSettings().mPartyRoomMode != cPartyRoomModeCampaign)
      return;

   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   const BPartySessionHostSettings* pHostSettings = getHostSettings();
   BASSERT(pHostSettings);

   const BCampaignNode* pNode = pCampaign->getNode( pHostSettings->mMapIndex );
   BASSERT(pNode);

   bool unlocked = isScenarioUnlocked(pNode);

   if (unlocked)
      return;

   // this is locked we need to change out the map
   const BCampaignNode* pUnlockedNode = getFirstUnlockedMission();

   Utils::FastMemCpy(&mGameOptionSettings, pHostSettings, sizeof(BPartySessionHostSettings)); 

   mGameOptionSettings.mMapIndex=(uint8)pUnlockedNode->getID();

   pParty->changeHostSettings(mGameOptionSettings);

   mpPartyRoomScreen->cancelSubMenuIfUp();
}

//==============================================================================
//==============================================================================
const BCampaignNode* BModePartyRoom2::getFirstUnlockedMission()
{
   // get the campaign bits for the local user
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return NULL;

   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   BASSERT(pProgress);
   if (!pProgress)
      return NULL;

   // get our current set of unlocked bits
   uint32 bits = getSessionCampaignBits();

   for (int i=0; i<pCampaign->getNumberNodes(); i++)
   {
      // get the node
      BCampaignNode* pNode = pCampaign->getNode(i);
      BASSERT(pNode);
      if (!pNode)
         continue;

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
         continue;

      long progressBit = pProgress->scenarioNodeIDToProgressIndex(pNode->getID());

      if ( (bits & (0x1<<progressBit)) > 0)
         return pNode;
   }

   return NULL;
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_customGameStartupComplete( BMPSession::cMPSessionCustomGameStartResultCode resultCode )
{
   //TODO - handle this event which signals when the custom game startup is done (if it worked or not, and why)
   if (resultCode==BMPSession::cMPSessionCustomGameStartResultSuccess)
   {
      nlog(cMPModeMenuCL, "mpSessionEvent_partyEvent_customGameStartupComplete - The game session has said everyone is connected and ready");
      mNextState = cStateGameHostReady;
   }
   else
   {
      nlog(cMPModeMenuCL, "mpSessionEvent_partyEvent_customGameStartupComplete - Game session start failed with errorcode: %d", resultCode );
      mNextState=cStateInSession;   
      hideWaitDialog();
      mTempMessageHolder = gDatabase.getLocStringFromID(25280);
      showModeModelEventMessage(mTempMessageHolder);
   }
}

//==============================================================================
// 
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyEvent_gameLaunched()
{
   nlog(cMPModeMenuCL, "mpSessionEvent_partyEvent_gameLaunched - when is this called?");

   //TODO - this event signals that the custom game has successfully started
}

//==============================================================================
// This is a GAME session player joining the game session
//==============================================================================
void BModePartyRoom2::mpSessionEvent_playerJoined(PlayerID gamePlayer, ClientID clientID, const XUID xuid, const BSimString& gamertag)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_playerJoined - player joined playerID[%d], clientID[%d], xuid[%I64u], name[%s]", gamePlayer, clientID, xuid, gamertag.asNative());

   BOOL isLocal = (gUserManager.getPrimaryUser()->getXuid() == xuid);
   BASSERT( gLiveSystem );
   BASSERT( gLiveSystem->getMPSession() );
   BMPSession* mGameView = gLiveSystem->getMPSession();
   if (isLocal)
   {
      mLocalPlayer=gamePlayer;
      gLiveSystem->getMPSession()->setControlledPlayer( gamePlayer );

      //Set local vars
      long playerID=mLocalPlayer;

      // hack - set myself to a human player for this slot
      long playerType = BGameSettings::cPlayerHuman;
      mGameView->setLong( PSINDEX(playerID, BGameSettings::cPlayerType), playerType);
   }

   //Dont set this - the host will do it
   //mGameView->setUInt64( PSINDEX( gamePlayer, BGameSettings::cPlayerXUID ), xuid);
   //if (mGameView->isHosting())
   //{
   //   //mGameView->setLong(BGameSettings::cPlayerCount, getGameSessionPlayerCount() );
   //   mGameView->setString(PSINDEX(gamePlayer, BGameSettings::cPlayerName), gamertag);
   //}
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_playerJoined - player join complete");
   
}

//==============================================================================
// This is a GAME session player leaving the game session
//==============================================================================
void BModePartyRoom2::mpSessionEvent_playerLeft(PlayerID gamePlayer, BOOL local)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_playerLeft - game session playerID %d left", gamePlayer );
   //If he is the local player, then clear that member var
   if(local)
   {
      mLocalPlayer=-1;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_startGame()
{
   if (mGameOptionSettings.mPartyRoomMode == cPartyRoomModeMM)
   {
      MVinceEventAsync_PreGameEvent("MPMatchmakingLaunch");
   }
   else
   {
      MVinceEventAsync_PreGameEvent("MPCustomLaunch");
   }

   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_startGame");
   mNextState=cStateStartGame;

   // JAR - assuming this was just debugging
   //mTempMessageHolder.set(L"mpSessionEvent_startGame()");
   //showWaitDialog(mTempMessageHolder);
}

//==============================================================================
//
//==============================================================================
//Depricated
/*
void BModePartyRoom2::mpSessionEvent_playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool& disconnect)
{
   gamePlayer;
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_playerNotResponding - player ID %d last time was %d", gamePlayer, lastResponseTime);
   if (timeGetTime()-lastResponseTime > 3000)
      disconnect = true;
}
*/

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_settingsChanged(const BDataSet* pSet, DWORD index, BYTE flags)
{
   flags;

   switch (index)
   {
      case BGameSettings::cMapName:
         {
            pSet->getString(BGameSettings::cMapName, mSettingMap);
            break;
         }

      case BGameSettings::cCoop:
         {
            pSet->getBool(BGameSettings::cCoop, mSettingCoop);
            break;
         }

      case BGameSettings::cRecordGame:
         {
            pSet->getBool(BGameSettings::cRecordGame, mSettingRecordGame);
            break;
         }
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_gameConnected()
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameConnected");
   /*
   if(mState==cStateJoinInit || mState==cStateJoinWait || mState==cStateInviteJoinConnect)
      mNextState=cStateJoined;
   else if(mState==cStateHostInit || mState==cStateHostWait)
      mNextState=cStateHosted;
      */
}

//==============================================================================
// The game session has been disconnected
//==============================================================================
void BModePartyRoom2::mpSessionEvent_gameDisconnected(long reason)
{
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameDisconnected");
   reason;     //see mpSession cMPSessionDisconnectReasonCode if you want details

   BASSERT( gLiveSystem );
   BASSERT( gLiveSystem->getMPSession() );

   if (gLiveSystem->isPartySystemFlaggedForReenter())
   {
      return;
   }

   //In matchmaking - we need to pull down this full screen UI spinner when the game session drops (in case the 'launching' spinner was up)
   if (mpSessionEvent_queryIsInMatchmakingMode())   
   {
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameDisconnected - I am matchmaking, hiding the spinner");
      hideWaitDialog();
      return;
   }

   //For a non matchmade game - this means that the game launch was canceled for some reason, we need to cleanup and be ready to launch again
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (!pParty)
   {
      //Should never happen - but possible on an odd shutdown ordering
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameDisconnected - No party session, exiting method");
      return;
   }

   pParty->clearReactivatePartyLogicFlag();
   pParty->lockPartyMembers(false);
   //BSimString displayString = "Launch canceled";
   //updateMatchmakingStatus(displayString);
   mNextState=cStateInSession;
   hideWaitDialog();
   if (pParty->isHosting())
   {
      mLaunchStartRequested = false;
      mNextState = cStateInSession;
      //Lets refresh the local settings member var
      const BPartySessionHostSettings* pHostSettings = getHostSettings();
      BASSERT(pHostSettings);
      nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_gameDisconnected - I am the party host, sending out status change to aborted");
      Utils::FastMemCpy(&mGameOptionSettings, pHostSettings, sizeof(BPartySessionHostSettings)); 
      mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeGameLaunchAborted;
      getPartySession()->changeHostSettings(mGameOptionSettings);
      //Host no longer hax in his own dialogs - but instead responds to the same ones the clients do
      //mTempMessageHolder.format(L"Launch aborted");
      //showModeModelEventMessage(mTempMessageHolder);  
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_partyDisconnected(long reason)
{
   if (mState >= cStateEndOfModeError)
      return;
   nlog(cMPModeMenuCL, "BModePartyRoom::mpSessionEvent_partyDisconnected");
   reason;
   setCurrentPartyAsDead();
   //We are just ignoring the error code,
   showModeExitErrorMessage(gDatabase.getLocStringFromID(23494));
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::showWaitDialog(const BUString& message)
{
   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   puiGlobals->showWaitDialog(message);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::hideWaitDialog()
{
   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   puiGlobals->setWaitDialogVisible(false);
}


//==============================================================================
//
//==============================================================================
void BModePartyRoom2::showYornMessageBox(const BUString& message, uint8 dialogButtons, DWORD userContext)
{
   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   puiGlobals->showYornBox(this, message, dialogButtons, userContext);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::showModeExitErrorMessage(const BUString& message)
{
   hideWaitDialog();
   showYornMessageBox(message, BUIGlobals::cDialogButtonsOK, cYornCallbackContextLeaveParty);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::showModeModelEventMessage(const BUString& message)
{
   hideWaitDialog();
   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   puiGlobals->showYornBox(this, message, BUIGlobals::cDialogButtonsOK, cYornCallbackContextConditionNormal);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_launchStarted()
{
   // JAR - assuming this was just debugging, pulling it
   //mTempMessageHolder.set(L"mpSessionEvent_launchStarted()...");
   //showWaitDialog(mTempMessageHolder);

   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_launchStarted");
   mNextState=cStateLaunchInit;
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_launchTimeUpdate(DWORD time)
{
   mLaunchCountdown=(long)(time/1000);
   mTempMessageHolder.locFormat(gDatabase.getLocStringFromID(25281), mLaunchCountdown);
   showWaitDialog(mTempMessageHolder);
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_launchAborted(PlayerID gamePlayer1, PlayerID gamePlayer2, long reason)
{
   //Deprecated
   BASSERT(false);

   reason;

   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_launchAborted - by playerID[%d], playerID[%d]", gamePlayer1, gamePlayer2);
   BASSERT( gLiveSystem );
   BASSERT( gLiveSystem->getMPSession() );
//-- FIXING PREFIX BUG ID 5150
   const BMPSession* mpSession = gLiveSystem->getMPSession();
//--

   gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);

   hideWaitDialog();
   if (mpSession->isHosting())
   {
      mLaunchStartRequested = false;
      mNextState = cStateInSession;
      //Hide the spinner
      hideWaitDialog();
      //Lets refresh the local settings member var
      const BPartySessionHostSettings* pHostSettings = getHostSettings();
      BASSERT(pHostSettings);
      Utils::FastMemCpy(&mGameOptionSettings, pHostSettings, sizeof(BPartySessionHostSettings)); 
      mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeGameLaunchAborted;
      getPartySession()->changeHostSettings(mGameOptionSettings);
      //Host no longer hax in his own dialogs - but instead responds to the same ones the clients do
      //mTempMessageHolder.format(L"Launch aborted");
      //showModeModelEventMessage(mTempMessageHolder);  
   }
   else 
   {
      // FIXME-COOP, could have two local players
      if (gamePlayer1 == mLocalPlayer || gamePlayer2 == mLocalPlayer)
         mSettingReady=false;
      mNextState=cStateInSession;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_initialSettingsComplete()
{  
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_initialSettingsComplete");
}

//==============================================================================
//
//==============================================================================
/*
void BModePartyRoom2::clearLeaders()
{
   mSettingLeader=-1;
}
*/

//==============================================================================
// Matchmaking mode calls this to ask the game layer to set any needed player-specific game options
//
// Setup Game Options
// FIXME-COOP - we could have two local players, which one is being requested?
//==============================================================================
void BModePartyRoom2::mpSessionEvent_requestForSetLocalPlayerSettings()
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();
   BASSERT(pMPSession);
   BASSERT(mLocalPlayer>0);

   //Pulls the data from their party session settings ------------------
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
//-- FIXING PREFIX BUG ID 5152
   const BPartySessionPartyMember* pMe=NULL;
//--
   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // is this me?
      if (!pParty->getSession()->isLocalClientID(pPartyMember->mClientID))
         continue;

      pMe=pPartyMember;
      break;
   }
   BASSERT(pMe);

   const BPartySessionHostSettings& hostSettings = pParty->getCurrentHostSettings();

   //Options for all players
   nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_requestForSetLocalPlayerSettings - Setting game session values for playerID[%d], XUID[%I64u] Civ:%i Team:%i", mLocalPlayer, pMe->mXuid, pMe->mSettings.mCiv, pMe->mSettings.mTeam);
   pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerCiv), pMe->mSettings.mCiv);
   pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerLeader), pMe->mSettings.mLeader);
   pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerTeam), pMe->mSettings.mTeam);
   pMPSession->setWORD(PSINDEX(mLocalPlayer, BGameSettings::cPlayerRank), pMe->mSettings.mPlayerRank.mValue);

   // difficulty is shown through the UI from the human players.
   uint8 diffType = hostSettings.mDifficulty;
   float difficulty = gDatabase.getDifficultyNormal();

   switch (diffType)
   {
   case DifficultyType::cEasy:
      difficulty=gDatabase.getDifficultyEasy();
      break;
   case DifficultyType::cNormal:
      difficulty=gDatabase.getDifficultyNormal();
      break;
   case DifficultyType::cHard:
      difficulty=gDatabase.getDifficultyHard();
      break;
   case DifficultyType::cLegendary:
      difficulty=gDatabase.getDifficultyLegendary();
      break;
   }

   pMPSession->setFloat(PSINDEX(mLocalPlayer, BGameSettings::cPlayerDifficulty), difficulty);
   pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerDifficultyType), diffType);

   uint32 skullbits = 0;
//-- FIXING PREFIX BUG ID 5132
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   BASSERT(pUser);
   //TODO - map this for both uses once CO-OP is fully supported - eric
   if (pUser != NULL)
   {
      skullbits = pUser->getProfile()->getSkullBits();
      pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerSkullBits), skullbits);
   }

   // do we override the team and go all random?
   if ( (hostSettings.mPartyRoomMode == cPartyRoomModeCustom) && (hostSettings.mRandomTeam) )
      pMPSession->setLong(PSINDEX(mLocalPlayer, BGameSettings::cPlayerTeam), 0);

   // All players need to do this so that every machine is set to the correct campaign node.
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (hostSettings.mPartyRoomMode == cPartyRoomModeCampaign)
   {
      //-- FIXING PREFIX BUG ID 5151
      const BCampaignNode* pNode = pCampaign->getNode( hostSettings.mMapIndex );
      //--
      BASSERT(pNode);
      // set the progress of the campaign.
      pCampaign->setCurrentNodeID(pNode->getID());
   }


   //Options set by the GAME host only
   if (pMPSession->isHosting())
   {
      switch(hostSettings.mPartyRoomMode)
      {
         case cPartyRoomModeCustom:
            {
               const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(hostSettings.mMapIndex);
               BASSERT(pMap);
               BSimString tempFileName = pMap->getFilename();
               tempFileName.crop(0,tempFileName.length()-5);
               pMPSession->setString(BGameSettings::cMapName, tempFileName);
               //Change so that all HUMANS greened up in the game session will kick off the gameStart, 
               //  mpSessionEvent_partyEvent_hostSubmitFinalGameSettings() will adjust this to the correct number (adding in the AIs)
               long currentPartyCount = pParty->getSession()->getClientCount();
               nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_requestForSetLocalPlayerSettings - Host setting game session values for map:%s players:%i humans:%i",pMap->getFilename().getPtr(), hostSettings.mNumPlayers, currentPartyCount );
               pMPSession->setLong(BGameSettings::cMaxPlayers, currentPartyCount);
               //pMPSession->setLong(BGameSettings::cMaxPlayers, hostSettings.mNumPlayers);
               pMPSession->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               pMPSession->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
               pMPSession->setLong(BGameSettings::cGameMode, (long)hostSettings.mGameMode);
               pMPSession->setLong(BGameSettings::cMapIndex, (long)hostSettings.mMapIndex);
               if (mUseLANMode)
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLan);
               else
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLive);

            }
            break;
         case cPartyRoomModeMM:
            {
               // BMatchMakingHopper* hopper = gLiveSystem->getMPSession()->getHopperList()->findHopperByID(mCurrentHopperIndex);
               BMatchMakingHopper* hopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(hostSettings.mHopperIndex);
               BASSERT(hopper);
               //Just stick a map in there - but its just to make sure nothing breaks, this is picked when the host final setting are submitted
               BSimString mapName = "skirmish\\design\\" + hopper->mMapList[0] + "\\" + hopper->mMapList[0]; 
               nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_requestForSetLocalPlayerSettings - Host setting game session values for map:%s players:%i",mapName.getPtr(), hostSettings.mNumPlayers );
               pMPSession->setString(BGameSettings::cMapName, mapName);
               //Little hacky here - the issue is that the hostSettings.mPlayers that is the max players for the session refers to the PARTY session
               //  Here we are setting the value for the GAME session, which is determined by the hopper
               pMPSession->setLong(BGameSettings::cMaxPlayers, hopper->mPlayers);   
               pMPSession->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               pMPSession->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
               pMPSession->setLong(BGameSettings::cGameMode, (long)hopper->mGameModeIndex);
               pMPSession->setLong(BGameSettings::cMapIndex, -1);
               if (mUseLANMode)
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLan);
               else
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLive);
            }
            break;
         case cPartyRoomModeCampaign:
            {
               //-- FIXING PREFIX BUG ID 5151
               const BCampaignNode* pNode = pCampaign->getNode( hostSettings.mMapIndex );
               //--
               BASSERT(pNode);
               BSimString tempFileName=pNode->getFilename();
               // tempFileName.crop(0,tempFileName.length()-5);
               nlog(cMPModeMenuCL, "BModePartyRoom2::mpSessionEvent_requestForSetLocalPlayerSettings - Host setting game session values for map:%s players:%i", tempFileName.getPtr(), hostSettings.mNumPlayers );
               pMPSession->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               pMPSession->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeCampaign);
               pMPSession->setLong(BGameSettings::cGameMode, 0);

               pMPSession->setString(BGameSettings::cMapName, tempFileName);
//               pMPSession->setLong(BGameSettings::cMaxPlayers, hostSettings.mNumPlayers);
               pMPSession->setLong(BGameSettings::cMaxPlayers, 2);
               pMPSession->setBool(BGameSettings::cCoop, true);
               pMPSession->setLong(BGameSettings::cMapIndex, -1);

               if (mUseLANMode)
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLan);
               else
                  gDatabase.getGameSettings()->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLive);
            }
            break;

      }
   }

}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::setVoiceChannels()
{
   if (!gWorld)
      return;

//-- FIXING PREFIX BUG ID 5153
   const BLiveVoice* voice = gLiveSystem->getLiveVoice();
//--
   if (!voice)
   {
      return;
   }

   long pCount = gWorld->getNumberPlayers();
   //Remember that 0 is always gaia and is included in this number

   if (pCount<4)       
   {
      //There are only 2 peeps, lets not split them up so they can cuss at each other
      return;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::mpSessionEvent_systemReady()
{
   hideWaitDialog();

   mpPartyRoomScreen->setVisible(true);

   //This gets called when the mpSession system is ready to start being used
   //  Or if that system has been reset and is ready to go again
   
   if ( (mState == cStateInitializingPartySessionHosting) ||
        (mState == cStateInitializingPartySessionJoining) )
   {
      // At this point, we are ready to go with the UI.
      //We are started with a party we are hosting and ready for user interaction

      // if there is no LSP available then use the default hoppers
      // inform the user that "The Halo Wars servers are currently unavailable.  Using a default set of game modes." or some such.
      if (!gLiveSystem->isInLanMode() && (gLiveSystem->isInNoLSPMode() || gLiveSystem->getHopperList()->wasLoadedFromDisk()))
      {
         showModeModelEventMessage(gDatabase.getLocStringFromID(25387));
      }

      mState = cStateInSession;
   }

   mpPartyRoomScreen->initializePlayerSlots();

   switch (mPartyRoomMode)
   {
      case cPartyRoomModeCustom:
         showCustomLobbyScreen();
         break;
      case cPartyRoomModeMM:
         showMatchmakingLobbyScreen();
         break;
      case cPartyRoomModeCampaign:
         showCampaignLobbyScreen();
         break;
   }

   // set the proper view
   if (isLocalPlayerHost())
   {
      setPartyRoomView(BUIMPSetupScreen::cPRViewMenu);
   }
   else
   {
      setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
   }

   // We are set up, defaulting to host, display our buttons and set focus to us.
   mpPartyRoomScreen->displayButtons();

}

//==============================================================================
// helper method
//==============================================================================
BPartySession* BModePartyRoom2::getPartySession() const
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (!pMPSession)
   {
      return NULL;
   }
   BPartySession* pPartySession = pMPSession->getPartySession();

   return pPartySession;
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::switchLobby(BPartySessionHostSettings* newSettings)
{
   // officially switch to the new lobby
   mPartyRoomMode = newSettings->mPartyRoomMode;

   mpPartyRoomScreen->resetSlotStates();

   switch (mPartyRoomMode)
   {
      case cPartyRoomModeCustom:
         mUseDefaultMap=true;
         showCustomLobbyScreen();
         break;
      case cPartyRoomModeMM:
         showMatchmakingLobbyScreen();
         break;
      case cPartyRoomModeCampaign:
         setupCampaignLobby();
         break;
   }

   // Let the host know that I have changed my mode and waiting to join the new lobby.
   BPartySessionPartyMember* pPartyMember = getLocalMember();
   BASSERT(pPartyMember);
   if (pPartyMember == NULL)
      return;

   pPartyMember->mSettings.mPartyRoomMode = (uint8)mPartyRoomMode;
   pPartyMember->mSettings.mTeam = 0;
   pPartyMember->mSettings.mSlot = cPRInvalidSlotNumber;

   pPartyMember->mSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
   sendUpdatedLocalMemberSettings(pPartyMember);

   updateAllGameOptions(newSettings);

   // first slot now has focus
   // mpPartyRoomScreen->setCurrentSlotFocus(0);
   refreshPlayerList();
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::setupCampaignLobby()
{
   // if we have more than 2 players in the game, drop any extras

   // change the screen to reflect the new lobby
   showCampaignLobbyScreen();
}


//==============================================================================
//==============================================================================
const BPartySessionHostSettings* BModePartyRoom2::getHostSettings() const
{
//-- FIXING PREFIX BUG ID 5156
   const BPartySession* pParty = getPartySession();
//--
   BASSERT(pParty);
   if (pParty == NULL)
      return NULL;

   const BPartySessionHostSettings& hostSettings = pParty->getCurrentHostSettings();

   return &hostSettings;
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::showCampaignLobbyScreen()
{
   if (!mpPartyRoomScreen)
      return;

   // set up the title
   if (mUseLANMode)
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25559));
   else
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25558));

   // turn off the team labels
   // fixme - move to the player list 
   // mpPartyRoomScreen->setTeamLabelsState(BUIPartyRoom2::cTeamLabelStateOff);

   setPlayerSlotsVisible(cPRNumPlayerSlotsCampaign, false);

}

//==============================================================================
//==============================================================================
void BModePartyRoom2::showMatchmakingLobbyScreen()
{
   if (!mpPartyRoomScreen)
      return;

   // set up the title
   if (mUseLANMode)
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25559));
   else
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25558));

   // turn off the team labels
   // fixme - move the the player list
//   mpPartyRoomScreen->setTeamLabelsState(BUIPartyRoom2::cTeamLabelStateOff);

   setPlayerSlotsVisible(cPRNumPlayerSlotsMatch, false);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::showCustomLobbyScreen()
{
   if (!mpPartyRoomScreen)
      return;

   // set up the title
   if (mUseLANMode)
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25559));
   else
      mpPartyRoomScreen->setTitle(gDatabase.getLocStringFromID(25558));

   // turn on the team labels
   // fixme - move to the player list
//   mpPartyRoomScreen->setTeamLabelsState(BUIPartyRoom2::cTeamLabelStateActive);

   // setup the players
   const BPartySessionHostSettings* pHostSettings = getHostSettings();
   setPlayerSlotsVisible(pHostSettings->mNumPlayers, true);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::setPlayerSlotsVisible(int numVisible, bool team)
{
   if (team)
   {
      int count=numVisible/2;
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayersSlotsPerTeam; i++)
      {
         mpPartyRoomScreen->setPlayerSlotActive((uint8)(i), (i<count));
         mpPartyRoomScreen->setPlayerSlotActive((uint8)(i+3), (i<count));
      }
   }
   else
   {
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayerSlots; i++)
      {
         mpPartyRoomScreen->setPlayerSlotActive((uint8)(i), (i<numVisible));
      }
   }
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::onAcceptLobby(long newPartyRoomMode)
{
   // if we didn't select the one we are already in...
   if (mPartyRoomMode != newPartyRoomMode)
   {
      // fire off a call to notify everybody that we are switching lobbies
      if (isLocalPlayerHost())
      {
         // get the current host settings
         const BPartySessionHostSettings* pHostSettings = getHostSettings();
         BASSERT(pHostSettings);

         BPartySession* pParty = getPartySession();
         BASSERT(pParty);
         
         // copy over the existing settings to our local temp buffer
         Utils::FastMemCpy(&mGameOptionSettings, pHostSettings, sizeof(BPartySessionHostSettings)); 

         // fixme - seed the host sessions with default data for the new lobby.
         mGameOptionSettings.mPartyRoomMode=(uint8)newPartyRoomMode;     // new lobby
         switch(newPartyRoomMode)
         {
            case cPartyRoomModeCampaign:
               {
                  //Kick an AI players
                  pParty->kickAllAIMembers();

                  BCampaign * pCampaign = gCampaignManager.getCampaign(0);
                  BASSERT(pCampaign);
                  BPartySessionPartyMember* pPartyMember=getLocalMember();
                  BASSERT(pPartyMember);

                  int8 nodeIndex = 0;

                  while (true)
                  {
                     // increment the node
                     if ( nodeIndex >= pCampaign->getNumberNodes() )
                        nodeIndex=0;

                     // get the node
//-- FIXING PREFIX BUG ID 5157
                     const BCampaignNode* pNode = pCampaign->getNode( nodeIndex );
//--
                     BASSERT(pNode);

                     // for right now, skip cinematics
                     if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
                     {
                        nodeIndex++;
                        continue;
                     }

                     // we found a good node
                     break;
                  }
                  mGameOptionSettings.mHopperIndex=0;
                  // mGameOptionSettings.mDifficulty=DifficultyType::cNormal;
                  mGameOptionSettings.mMapIndex = nodeIndex;
                  mGameOptionSettings.mNumPlayers=2;         // 2 for coop
                  mGameOptionSettings.mRandomTeam = false;

                  if (pPartyMember)
                  {
                     // default to cutter.
                     pPartyMember->mSettings.mCiv = 1;
                     pPartyMember->mSettings.mLeader = 1;
                  }
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
               }
               break;
            case cPartyRoomModeMM:
               {
                  //Kick an AI players
                  pParty->kickAllAIMembers();

                  BOOL findTeamHopper = (getPartySession()->getPartyCount()>1)?TRUE:FALSE;
//-- FIXING PREFIX BUG ID 5158
                  const BMatchMakingHopper* hopper = gLiveSystem->getHopperList()->findFirstValidHopper(findTeamHopper);
//--
                  if (!hopper || ( hopper->mMapList.getSize()==0))
                  {
                     mpSessionEvent_ESOConnectFailed(BMPSession::cMPSessionESOConnectCouldNoValidHoppers);
                     return;
                  }
                  //Ok - we have valid hopper - but lets add a special case check - if I have > 2 peeps, try and get a 3 person team hopper
                  if (getPartySession()->getPartyCount()>2)
                  {
                     uint hopperIndex = hopper->mListIndex;
                     BMatchMakingHopper* betterHopper = NULL;
                     do 
                     {
                        betterHopper = gLiveSystem->getHopperList()->getNextValidHopper(hopperIndex, findTeamHopper);
                        if(betterHopper)
                        {
                           if ((betterHopper->mTeamCode == BMatchMakingHopper::cBMatchMakingHopperTeamCodeTeamsOnly) &&
                              (betterHopper->mPlayersPerTeam == 3) &&
                              (betterHopper->mMapList.getSize() > 0))
                           {
                              //Found one - done
                              hopper = betterHopper;
                              break;
                           }
                           hopperIndex = betterHopper->mListIndex;
                        }
                     }
                     while (betterHopper);                                             
                  }
                  BASSERT(hopper);
                  BASSERT(hopper->mMapList.getSize() > 0);
                  //gLiveSystem->getMPSession()->setActiveHopper(hopper->mListIndex);
                  mGameOptionSettings.mHopperIndex=(uint8)hopper->mListIndex;
                  // mGameOptionSettings.mDifficulty=0;
                  mGameOptionSettings.mMapIndex = 0;
                  mGameOptionSettings.mNumPlayers = hopper->mPlayersPerTeam;
                  mGameOptionSettings.mRandomTeam = false;
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;
               }
               break;
            case cPartyRoomModeCustom:
               {
                  uint8 numNewPlayers = 6;            // by default
                  switch (pParty->getPartyCount())
                  {
                     case 1:
                        numNewPlayers=2;
                        break;
                     case 2:
                        numNewPlayers=2;
                        break;
                     case 3:
                        numNewPlayers=4;
                        break;
                     default:
                        numNewPlayers=6;
                        break;
                  }
                  mGameOptionSettings.mRandomTeam = false;         //If Yes - then all teams are randomly determined at game load time
                  mGameOptionSettings.mHopperIndex=0;        //Which matchmaking hopper the host says everyone should currently be using
                  mGameOptionSettings.mNumPlayers=numNewPlayers;         // 2, 4, 6
                  // mGameOptionSettings.mDifficulty=0;
                  mGameOptionSettings.mHostStatusInformation = cBPartySessionHostStatusInformationCodeNone;

                  uint8 newMapIndex = 0;
                  if (!mpPartyRoomScreen->getFirstValidMap(mGameOptionSettings.mNumPlayers, newMapIndex))
                  {
                     // we couldn't find a map, throw up our hands and quit
                     return;
                  }
                  mGameOptionSettings.mMapIndex=newMapIndex;
               }
               break;
         }

         // check the player count to see if we need to drop any
         if ( mGameOptionSettings.mNumPlayers < pParty->getPartyCount() ) 
         {
            // we have too many people for the party at this point. We need to drop some. Ask if we should do this.
            BUIGlobals* puiGlobals = gGame.getUIGlobals();
            BASSERT(puiGlobals);
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(23493), BUIGlobals::cDialogButtonsOKCancel, cYornCallbackContextDropPlayersLobby);
            return;
         }

         pParty->changeHostSettings(mGameOptionSettings);
      }
   }
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isVisibleSlot(int slot, BPartySessionHostSettings* pHostSettings)
{
   uint8 numPlayersPerTeam = pHostSettings->mNumPlayers;
   bool teams=false;
   if (pHostSettings->mPartyRoomMode==cPartyRoomModeCustom)
   {
      teams=true;
      numPlayersPerTeam /= 2;
   }

   if (teams)
   {
      int firstIndex=0;
      if (slot>=cPRNumPlayersSlotsPerTeam)
         firstIndex=3;

      if (slot>=firstIndex+numPlayersPerTeam)
      {
         return false;
      }
   }
   else
   {
      // the slot is not visible
      if (slot>=numPlayersPerTeam)
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isValidMapType(long mapType)
{
   bool isValid=false;

   switch (mapType)
   {
      case BScenarioMap::cScenarioTypeDevelopment:
         if (gConfig.isDefined(cConfigShowDevMaps))
            isValid=true;
         break;
      case BScenarioMap::cScenarioTypePlaytest:
         isValid=true;
         break;
      case BScenarioMap::cScenarioTypeFinal:
         isValid=true;
         break;
   }
   return isValid;
}



//==============================================================================
//==============================================================================
void BModePartyRoom2::onAcceptGameOptions()
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   switch (mPartyRoomMode)
   {
      case cPartyRoomModeMM:         
      case cPartyRoomModeCustom:
         // validate that we have enough slots for the number of existing people in the party
         mCurrentHopperIndex = mGameOptionSettings.mHopperIndex;
         if ( mGameOptionSettings.mNumPlayers < pParty->getPartyCount() )
         {
            // we have too many people for the party at this point. We need to drop some. ASk if we should do this.
            BUIGlobals* puiGlobals = gGame.getUIGlobals();
            BASSERT(puiGlobals);
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(23493), BUIGlobals::cDialogButtonsOKCancel, cYornCallbackContextDropPlayers);
            return;
         }
         break;
      case cPartyRoomModeCampaign:
         // do nothing
         break;
   }

   acceptGameOptions();
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::acceptGameOptions()
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   // do we need to change the focus indicator?
   if (!isVisibleSlot(mpPartyRoomScreen->getPlayerCurrentSlotFocus(), &mGameOptionSettings) )
   {
      // reset the player slot focus
      mpPartyRoomScreen->setPlayerCurrentSlotFocus(0);
      refreshPlayerList();
   }

   // send these out
   pParty->changeHostSettings(mGameOptionSettings);
}


//==============================================================================
//==============================================================================
void BModePartyRoom2::setImage(const char * imagePath)
{
   if (mpPartyRoomScreen)
      mpPartyRoomScreen->setImage(imagePath);
}

/*
//==============================================================================
//==============================================================================
bool BModePartyRoom2::getFirstValidMap(uint8 numPlayers, uint8& mapIndex, bool matchPlayersExactly)
{
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap((uint8)i, numPlayers, matchPlayersExactly))
         continue;

      mapIndex=(uint8)i;
      return true;
   }

   return false;
}
*/

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isValidMap(uint8 mapIndex, uint8 numPlayers, bool matchPlayersExactly)
{
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
   BASSERT(pMap);

   if (gConfig.isDefined(cConfigDemo))
   {
      BSimString tutorialMap;
      gConfig.get(cConfigTutorialMap, tutorialMap);
      tutorialMap+=".scn";
      if (pMap->getFilename() == tutorialMap)
         return false;
   }

   uint8 mapMaxPlayers=(uint8)pMap->getMaxPlayers();

   if (matchPlayersExactly)
   {
      // do we have an exact fit for players to map?
      return (mapMaxPlayers == numPlayers);
   }

   // can the map handle that many players?
   if (mapMaxPlayers<numPlayers)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::updateAllGameOptions(BPartySessionHostSettings* pGameHostSettings)
{
   if (mState == cStateInLANGameList)
      return; 

   // XXX hack for updating the game settings with the appropriate number of players
   // FIXME: redo party session architecture to fix interdependencies

   //No longer needed - the game settings are only checked/set when there is a game session, at that point - there are NO more party session changes
   //gDatabase.getGameSettings()->setLong(BGameSettings::cMaxPlayers, pGameHostSettings->mNumPlayers);

   nlog(cMPModeMenuCL, "BModePartyRoom2::updateAllGameOptions - updating [host says %i players currently]", pGameHostSettings->mNumPlayers);

   switch (mPartyRoomMode)
   {
      case cPartyRoomModeCustom:
         {
            mpPartyRoomScreen->populateCustomModeMenu(pGameHostSettings);
            setPlayerSlotsVisible(pGameHostSettings->mNumPlayers, true);
            // set the map image
            const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(pGameHostSettings->mMapIndex);
            setImage(pMap->getMapKeyFrame().getPtr());
         }
         break;
      case cPartyRoomModeMM:
         {
            mpPartyRoomScreen->populateMatchmakingModeMenu(pGameHostSettings);
            setPlayerSlotsVisible(pGameHostSettings->mNumPlayers, false);
            // fixme - resolve this
            setImage("img://art\\ui\\flash\\shared\\textures\\pregame\\mapimages\\temp.ddx");
         }
         break;
      case cPartyRoomModeCampaign:
         {
            mpPartyRoomScreen->populateCampaignModeMenu(pGameHostSettings);
            BCampaign * pCampaign = gCampaignManager.getCampaign(0);
            BASSERT(pCampaign);
            BCampaignNode* pNode = pCampaign->getNode( pGameHostSettings->mMapIndex );
            BASSERT(pNode);
            if (pNode)
            {
               const BSimString& mapImage = pNode->getImageIntro();
               setImage(mapImage.getPtr());
            }
            else
               setImage("img://art\\ui\\flash\\shared\\textures\\pregame\\mapimages\\temp.ddx");
         }
         break;
   }

   // not sure if this will break things.
   refreshPlayerList();
}

//==============================================================================
// BModePartyRoom2::setupMatchmakingUI
//==============================================================================
void BModePartyRoom2::setupMatchmakingUI(BPartySessionHostSettings* pHostSettings)
{
   // turn on any status dialogs

   // set up the player slots
   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
//-- FIXING PREFIX BUG ID 5159
   const BMatchMakingHopper* pHopper = pHopperList->findHopperByHopperIndex(pHostSettings->mHopperIndex);
//--
  
   switch (pHopper->mTeamCode)
   {
      case BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams:
         break;

      case BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams:
         setPlayerSlotsVisible(pHopper->mPlayers, true);
         break;

      case BMatchMakingHopper::cBMatchMakingHopperTeamCodeTeamsOnly:
         setPlayerSlotsVisible(pHopper->mPlayers, true);
         break;
   }

   updateMatchmakingSlots(true);

   mCurrentPlayersInMatchCache=0;
}

//==============================================================================
// BModePartyRoom2::updateMatchmakingSlots
//==============================================================================
void BModePartyRoom2::updateMatchmakingSlots(bool forceUpdate)
{
   if (!forceUpdate)
   {
      if (!mpSessionEvent_queryIsInMatchmakingMode())
         return;

      if (!gLiveSystem->getMPSession()->isMatchmakingRunning())
         return;
   }

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;

   const BPartySessionHostSettings& hostSettings=pParty->getCurrentHostSettings();

   int numPlayers=0;

//   BPartySessionPartyMember* pLocalMember=getLocalMember();

   bool slotUpdated[cPRNumPlayerSlots] = {false, false, false, false, false, false};

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      if (pPartyMember->mXuid==0)
         continue;

      // is this player in the same lobby as me? No? -> don't show him, he still needs to switch
      if (pPartyMember->mSettings.mPartyRoomMode != mPartyRoomMode)
         continue;

      // does he have slot assigned?
      if (pPartyMember->mSettings.mSlot == cPRInvalidSlotNumber)
         continue;

      // is this player in a "joining" state
      if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
         continue;

      numPlayers++;

      // indicate we updated this slot
      slotUpdated[pPartyMember->mSettings.mSlot]=true;
      displayPlayerSlot(pPartyMember);

      // is this guy local?
/*
      if (pLocalMember && (pLocalMember->mClientID == pPartyMember->mClientID))
         slotWithFocus=pPartyMember->mSettings.mSlot;
*/
   }

   int currentPlayersInMatch = gLiveSystem->getMPSession()->getGameSessionPlayerCount();
   if (currentPlayersInMatch==0)
      currentPlayersInMatch=numPlayers;
   if (mCurrentPlayersInMatchCache==currentPlayersInMatch)
      return;
   mCurrentPlayersInMatchCache=currentPlayersInMatch;

   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
//-- FIXING PREFIX BUG ID 5160
   const BMatchMakingHopper* pHopper = pHopperList->findHopperByHopperIndex(hostSettings.mHopperIndex);
//--

   int totalSlots = pHopper->mPlayers;
   int nextSlot=0;
   int playersPerTeam=pHopper->mPlayersPerTeam;

   // populate the rest of the slots with their status
   for (int i=0; i<totalSlots; i++)
   {
      if (i<numPlayers)
         continue;               // did we already fill this one in above?

      nextSlot = i;
      if (i>=playersPerTeam)
      {
         int extra = i-playersPerTeam;           // find out how many into the other side we are
         nextSlot=cPRNumPlayerSlotsMatch+extra;
      }

      BUIPlayer* pUiPlayer=mpPartyRoomScreen->getPlayer((uint8)nextSlot);
      if (!pUiPlayer)
         continue;

      // set up the type
      pUiPlayer->mSlotType=BUIPlayer::cMatchmaking;

      // set up the display string
      if (i<currentPlayersInMatch)
      {
         pUiPlayer->mReady=true;
         pUiPlayer->mGamerTag.set(gDatabase.getLocStringFromID(25466).getPtr());
      }
      else
      {
         pUiPlayer->mReady=false;
         pUiPlayer->mGamerTag.set(gDatabase.getLocStringFromID(25467).getPtr());
      }
   }

/*
   for (uint8 i=0; i<cPRTotalPlayerSlots; i++)
   {
      if (slotUpdated[i])
         continue;

      // clear the slot of any data
      clearPlayerSlot(i);
   }
*/

   mpPartyRoomScreen->refreshPlayerSlots();
}

//==============================================================================
// BModePartyRoom2::getLocalMember
//==============================================================================
BPartySessionPartyMember* BModePartyRoom2::getMemberByXUID(XUID xuid)
{
   BPartySession* pParty = getPartySession();
   //It is possible during startup for this to be called and there be no party session
   //BASSERT(pParty);
   if (!pParty || !pParty->getSession())
   {
      return NULL;
   }

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      if (pPartyMember->mXuid != xuid)
         continue;

      return pPartyMember;
   }

   return NULL;
}


//==============================================================================
// BModePartyRoom2::getLocalMember
// FIXME-COOP - we could have two local players
//==============================================================================
BPartySessionPartyMember* BModePartyRoom2::getLocalMember()
{
   BPartySession* pParty = getPartySession();
   //It is possible during startup for this to be called and there be no party session
   //BASSERT(pParty);
   if (!pParty || !pParty->getSession())
   {
      return NULL;
   }

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // is this me?
      if (!pParty->getSession()->isLocalClientID(pPartyMember->mClientID))
         continue;

      return pPartyMember;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isPlayerReady(BPartySessionPartyMember* pPartyMember)
{
   return (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateReadyToStart);
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::isOkToChangeGameSettingsInUI()
{
   return !mWaitingOnEveryoneToRejoinParty;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::isOkToShowLocalPlayerOptionsInUI()
{
   if (mTeamRequestChangePending)
      return false;

   const BPartySessionPartyMember* pPartyMember = getLocalMember();
   if (!pPartyMember)
      return false;

    return (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateConnected);  
}

//==============================================================================
//==============================================================================
bool BModePartyRoom2::isLocalPlayerReady()
{
//-- FIXING PREFIX BUG ID 5161
   const BPartySessionPartyMember* pPartyMember = getLocalMember();
//--
   if (!pPartyMember)
      return false;

   // am I ready
   return (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateReadyToStart);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::toggleReady()
{
   BPartySessionPartyMember* pPartyMember = getLocalMember();
   if (!pPartyMember)
      return;

   //New state to go to
   BPartySessionMemberConnectionState newState = cBPartySessionMemberConnectionStateConnected;
   // ready myself up.
   if (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateConnected)
   {
      newState = cBPartySessionMemberConnectionStateReadyToStart;
      //Clear the re-entrant party system bit so that we once again start caring about game session events
      gLiveSystem->setPartySystemForReenter(false);
   }
   pPartyMember->mSettings.mConnectionState = (uint8)newState;
   sendUpdatedLocalMemberSettings(pPartyMember);

   //See if I control any AIs
   if (isLocalPlayerHost())
   {
      //Switch state for all AI members
      BPartySession* pParty = getPartySession();
      if (!pParty)
      {
         return;
      }
      uint partyCount = pParty->getPartyMaxSize();
      for (uint i=0; i<partyCount; i++)
      {
         pPartyMember = pParty->getPartyMember(i);
         if (!pPartyMember)
            continue;

         // is this player ready?
         if (pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
         {
            pPartyMember->mSettings.mConnectionState = (uint8)newState;            
            sendUpdatedMemberSettings(pPartyMember);  
         }
      }
   }

   //HAX until we have second controller UI interaction in
   //  This allows the primary player to green up everyone
   //  Go through and toggle any secondary players that have a port number (that means they are secondary, and local)
   //Switch state for all AI members
   BPartySession* pParty = getPartySession();
   if (!pParty)
   {
      return;
   }
   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // is this player ready?
      if ((pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberSecondaryPlayer) &&
          (pPartyMember->mPort != -1))
      {
         pPartyMember->mSettings.mConnectionState = (uint8)newState;            
         sendUpdatedMemberSettings(pPartyMember);  
      }
   }

   return;
}


//==============================================================================
// BModePartyRoom2::sendUpdatedMemberSettings
//==============================================================================
void BModePartyRoom2::sendUpdatedMemberSettings(BPartySessionPartyMember* pPartyMember)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;

   if (pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
   {
      if (isLocalPlayerHost())
      {
         pParty->changeAIPlayerSettings(pPartyMember->mXuid, pPartyMember->mSettings);
      }
   }
   else
   {
      sendUpdatedLocalMemberSettings(pPartyMember);
   }
}

//==============================================================================
// BModePartyRoom2::sendUpdatedLocalMemberSettings
//==============================================================================
void BModePartyRoom2::sendUpdatedLocalMemberSettings(BPartySessionPartyMember* pPartyMember)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;

   pParty->changeSettings(pPartyMember->mXuid, pPartyMember->mSettings);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::exitParty()
{
   // change the mode to exit the lobby
   //mNextState = cStateExitMode; 
   
   //If we have not yet started up the system - lets just bail without a warning box - eric
   if (mState < cStateInLANGameList)
   {
      hideWaitDialog();
      mNextState=cStateExitMode;
      return;
   }

   if (mState == cStateInLANGameList)
   {
      hideWaitDialog();
      mNextState = cStateExitMode;
      return;
   }

   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   if (puiGlobals)
   {
      puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(23488), BUIGlobals::cDialogButtonsOKCancel, cYornCallbackContextLeaveParty);
   }
   else
   {
      // we should be able to get the globals, but just in case...
      mNextState=cStateExitMode;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::yornResult(uint result, DWORD userContext, int port)
{
   switch (result)
   {
      // if we cancel, we do nothing
      case BUIGlobals::cDialogResultCancel:
         break;

      // if we hit OK, do something depending on the user context.
      case BUIGlobals::cDialogResultOK:
         {
            switch (userContext)
            {
               case cYornCallbackContextKickPlayer:
                  {
                     BPartySession* partySession = getPartySession();
                     if (!partySession)
                        break;

                     // Get the player
                     //BPartySessionPartyMember* pFocusMember = getPlayerBySlot(mpPartyRoomScreen->getCurrentSlotFocus());
                     BPartySessionPartyMember* pFocusMember = partySession->findPartyMemberByXUID(mKickTargetXUID);
                     mKickTargetXUID = 0;
                     if (!pFocusMember)
                        break;

                     if (isLocalPlayer(pFocusMember) &&
                        (pFocusMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberPrimaryPlayer))
                        break;;

                     partySession->dropPartyMemberByIndex(pFocusMember->mMemberIndex, true);
                  }
                  break;
               case cYornCallbackContextConditionNormal:
               {
                  //This one just leaves the system in its current running state
                  break;
               }
               case cYornCallbackContextExitMode:
                  {
                     mNextState = cStateExitMode; 
                  }
                  break;

               case cYornCallbackContextLeaveParty:
                  {
                     mNextState=cStateExitMode;
                  }
                  break;
               case cYornCallbackContextDropPlayers:
                  {
                     // drop the excess players
                     dropPlayersExcessPlayers(mGameOptionSettings.mNumPlayers);
                     acceptGameOptions();
                  }
                  break;
               case cYornCallbackContextDropPlayersLobby:
                  {
                     BPartySession* pParty = getPartySession();
                     BASSERT(pParty);
                     if (pParty == NULL)
                        break;
                     // drop the excess players
                     dropPlayersExcessPlayers(mGameOptionSettings.mNumPlayers);

                     // Turn off the menu
                     // setLobbyMenuVisible(false);
                     pParty->changeHostSettings(mGameOptionSettings);
                     if (!isLocalPlayerHost())
                        setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerList);
                  }
                  break;            
            }
         }
         break;
   }
}

//==============================================================================
//
//==============================================================================
void BModePartyRoom2::dropPlayersExcessPlayers(uint8 maxInParty)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;
   uint partyCount = pParty->getPartyCount();

   int needToKickCount = partyCount-maxInParty;
   if (needToKickCount<=0)
   {
      return;
   }

   //Go through first and ditch AI players out before a normal processing
   nlog(cMPModeMenuCL, "BModePartyRoom2::dropPlayersExcessPlayers - Need to kick [%d] peeps", needToKickCount);
   int kicked=0;
   for (int i=(partyCount-1); i>=0; i--)
   {
//-- FIXING PREFIX BUG ID 5162
      const BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
//--
      if ((!pPartyMember) ||
          (pPartyMember->mSettings.getPartyMemberType()!=cBPartySessionPartyMemberAI))
      {
         continue;
      }

      // kick the player
      nlog(cMPModeMenuCL, "BModePartyRoom2::dropPlayersExcessPlayers - Kicking AI at party member index [%d]", i);
      pParty->dropPartyMemberByIndex(pPartyMember->mMemberIndex, true);
      kicked++;

      if (kicked>=needToKickCount)
      {
         break;
      }
   }

   //NOTE: We need to count backwards in this list because the calls to dropPartyMemberByIndex remove that index RIGHT THEN
   //So if you remove index 2, it drops player 2, then moves player 3 down to index 2 and so when it next called drop 3, there 
   //  would not be a player at index 3.
   partyCount = pParty->getPartyCount();
   for (int i=(partyCount-1); i>=maxInParty; i--)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // kick the player
      nlog(cMPModeMenuCL, "BModePartyRoom2::dropPlayersExcessPlayers - Kicking player at party member index [%d]", i);
      pParty->dropPartyMemberByIndex(pPartyMember->mMemberIndex, true);
   }
}


//==============================================================================
//
//==============================================================================
void BModePartyRoom2::editPlayer(BPartySessionPartyMember* pMember)
{
   if (!pMember)
      return;

   mPartyRoomViewPrevious=getPartyRoomView();
   setPartyRoomView(BUIMPSetupScreen::cPRViewPlayerEdit);

   if (mpLeaderPicker)
   {
      mEditPlayerXuid=pMember->mXuid;
      mpLeaderPicker->setCurrentLeader(pMember->mSettings.mCiv, pMember->mSettings.mLeader);
      mpLeaderPicker->show();
   }
}



//==============================================================================
//
//==============================================================================
/*
void BModePartyRoom2::changeTeam(int direction)
{
   direction;
   // find my player
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   BPartySessionPartyMember* pMe=getLocalMember();
   BASSERT(pMe);


   uint8 connectionState = pMe->mSettings.mConnectionState;
   uint8 newConnectionState = connectionState;

   switch (direction)
   {
   case cInputIncrement:
      if (pMe->mSettings.mTeam==2)
         break;
      
      if (connectionState!=cBPartySessionMemberConnectionStateWantToSwitchTeamsRight)
         newConnectionState=cBPartySessionMemberConnectionStateWantToSwitchTeamsRight;
      break;
   case cInputDecrement:
      if (pMe->mSettings.mTeam==1)
         break;

      if (connectionState!=cBPartySessionMemberConnectionStateWantToSwitchTeamsLeft)
         newConnectionState=cBPartySessionMemberConnectionStateWantToSwitchTeamsLeft;
      break;
   }

   // did the connection state change?
   if (newConnectionState!=connectionState)
   {
      // send the request out
      pMe->mSettings.mConnectionState = newConnectionState;

      // send the settings around
      pParty->changeSettings(pMe->mXuid, pMe->mSettings);
   }

}
*/


//==============================================================================
//
//==============================================================================
void BModePartyRoom2::changeTeam(int direction)
{
   direction;
   // find my player
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (pParty == NULL)
      return;

   BPartySessionPartyMember* pMe=getLocalMember();
   BASSERT(pMe);


   uint8 connectionState = pMe->mSettings.mConnectionState;
   uint8 newConnectionState = connectionState;

   uint8 team = pMe->mSettings.mTeam;
   switch (direction)
   {
      case cInputIncrement:
         if (team==2)
         {
            // cancel if one is pending
            if (connectionState==cBPartySessionMemberConnectionStateWantToSwitchTeams)
               newConnectionState=cBPartySessionMemberConnectionStateCancelSwitchTeams;
         }
         else
         {
            // request a team change if one not pending
            if (connectionState!=cBPartySessionMemberConnectionStateWantToSwitchTeams)
               newConnectionState=cBPartySessionMemberConnectionStateWantToSwitchTeams;
            team=2;
         }
         break;
      case cInputDecrement:
         if (team==1)
         {
            // cancel if one is pending
            if (connectionState==cBPartySessionMemberConnectionStateWantToSwitchTeams)
               newConnectionState=cBPartySessionMemberConnectionStateCancelSwitchTeams;
         }
         else
         {
            // request a team change if one is not pending
            if (connectionState!=cBPartySessionMemberConnectionStateWantToSwitchTeams)
               newConnectionState=cBPartySessionMemberConnectionStateWantToSwitchTeams;
            team=1;
         }
         break;
   }
   if (newConnectionState!=connectionState)
   {
      // send the request out
      //pMe->mSettings.mConnectionState = newConnectionState;

      //On any data change which is 'server approved' we need to just send the change to the host and let him resolve it
      //If we set the state locally - then we could override the host's decision (which might be to NOT allow the change)
      // The above way of setting it was causing a bug where two people could mash buttons and end up on the same team
      BPartySessionPlayerSettings temp;
      Utils::FastMemCpy(&temp, &pMe->mSettings, sizeof(BPartySessionPlayerSettings));
      temp.mConnectionState = newConnectionState;

      // send the settings around
      pParty->changeSettings(pMe->mXuid, temp);
      gSoundManager.playCue( "play_ui_menu_choose_team" );
      mTeamRequestChangePending = true;
   }

}



#ifndef BUILD_FINAL
//==============================================================================
// BModePArtyRoom2::outputPlayerInfo
//==============================================================================
void BModePartyRoom2::outputPlayerInfo()
{
   // This reports information about the players in the current hopper via console output.
   // This is used for automation via the getPlayerInfo() hook in consoleFuncsGame.cpp
   
   // Find the current party's host settings
//-- FIXING PREFIX BUG ID 5167
   const BPartySession* pParty = getPartySession();
//--
   if (!pParty)
      return;
   const BPartySessionHostSettings& hostSettings = pParty->getCurrentHostSettings();
   
   // Find the current party player hopper
   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
   if (!pHopperList)
      return;
//-- FIXING PREFIX BUG ID 5168
   const BMatchMakingHopper* pHopper = pHopperList->findHopperByHopperIndex(hostSettings.mHopperIndex);
//--
   if (!pHopper)
      return;
   
   // Output the total slots.  For compatibility with other (IE in-game) player 
   // list output, this reads "Total Players" instead of "Total Slots"
   gConsoleOutput.output(cMsgConsole, "Total Players: %d", pHopper->mPlayers);
   for (uint8 i = 0; i < pHopper->mPlayers; i++)
   {
      // Output information about the (potential) player in this slot.  The same format should  
      // be output for empty slots, for automation which consumes this information.
//-- FIXING PREFIX BUG ID 5166
      const BUIPlayer* pPlayer = mpPartyRoomScreen->getPlayer(i);
//--
      if (!pPlayer || pPlayer->mSlotType == BUIPlayer::cEmpty)
      {
         gConsoleOutput.output(cMsgConsole, "Player ID: %d, Name: , Team: -1, Civ: -1, Leader: , IsAI: 0", i);
      } 
      else 
      {
         long leaderStringID = pPlayer->mLeaderStringIDIndex;
         const WCHAR* pName  = pPlayer->mGamerTag.getPtr();
         const WCHAR*   pLName = gDatabase.getLocStringFromIndex(leaderStringID).getPtr();
         bool isAI = (pPlayer->mSlotType == BUIPlayer::cAI) ? true : false;
         gConsoleOutput.output(cMsgConsole, "Player ID: %d, Name: %S, Team: %d, Civ: %d, Leader: %S, IsAI: %d", 
               i, pName, pPlayer->mTeam, pPlayer->mCiv, pLName, isAI);
      }
   }
}
#endif


//==============================================================================
//==============================================================================
BPartySessionPartyMember* BModePartyRoom2::getPlayerBySlot(int slot)
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);
   if (!pParty)
      return NULL;

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // The slot I'm interested in?
      if (pPartyMember->mSettings.mSlot != slot)
         continue;

      return pPartyMember;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::acceptPlayerChanges()
{
   // mPartyRoomViewPrevious needs to set before calling this method.
   setPartyRoomView(mPartyRoomViewPrevious);

   // push the new civ/leader settings into my settings and send out
   // BPartySessionPartyMember* pMember = getLocalMember();
   BPartySessionPartyMember* pMember = getMemberByXUID(mEditPlayerXuid);
   if (!pMember)
      return;

   if (mpLeaderPicker)
   {
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         pMember->mSettings.mCiv = pLeader->mCiv;
         pMember->mSettings.mLeader = pLeader->mLeader;
      }
   }

   sendUpdatedMemberSettings(pMember);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::cancelPlayerChanges()
{
   // mPartyRoomViewPrevious needs to set before calling this method.
   setPartyRoomView(mPartyRoomViewPrevious);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::updateGamerPic(BPartySessionPartyMember* pPartyMember)
{
   // no GamerPic for AIs
   if (pPartyMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
      return;

   BOOL isLocal = isLocalPlayer(pPartyMember);

   // do we already have this gamer pic?
   BGamerPic *pPic = gGamerPicManager.getGamerPic(pPartyMember->mXuid);
   if (pPic != NULL)
      return;

   // add this gamer pic to our cache
   gGamerPicManager.readGamerPic(pPartyMember->mXuid, gUserManager.getPrimaryUser()->getPort(), isLocal);
}

//==============================================================================
//==============================================================================
void BModePartyRoom2::setPartyRoomView(int partyRoomView)
{
   if (mpPartyRoomScreen)
      mpPartyRoomScreen->setView(partyRoomView);

   // display the right set of buttons based on the mode we are in  
   mpPartyRoomScreen->displayButtons();
}



//==============================================================================
//==============================================================================
void BModePartyRoom2::refreshPlayerList()
{
   if ((mpSessionEvent_queryIsInMatchmakingMode()) &&
       (gLiveSystem->getMPSession()->isMatchmakingRunning()))
   {
      updateMatchmakingSlots();
   }
   else
   {
      updatePlayerSlots();
   }
   mpPartyRoomScreen->updatePlayerListHelp();
   mpPartyRoomScreen->displayButtons();
}


//==============================================================================
// BModePartyRoom2::leaderPickerEvent
//==============================================================================
bool BModePartyRoom2::leaderPickerEvent(const BSimString& command)
{
   if (command == "accept")
   {
      if (mpLeaderPicker)
      {
         // grab the new leader info here.
         mpLeaderPicker->hide();
         acceptPlayerChanges();
      }
   }
   else if (command == "cancel")
   {
      if (mpLeaderPicker)
      {
         mpLeaderPicker->hide();
         cancelPlayerChanges();
      }
   }
   else 
   {
      return false;
   }

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::editLocalPlayer()
{
   if (isLocalPlayerReady())
      return false;

   // either a) nothing, b) edit my options, c) bring up gamertag window
   BPartySessionPartyMember* pLocalMember = getLocalMember();
   if (pLocalMember)
   {
      editPlayer(pLocalMember);
      return true;
   }

   return false;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::decrementLeader()
{
   if (isLocalPlayerReady())
      return false;

   // push the new civ/leader settings into my settings and send out
   BPartySessionPartyMember* pLocalMember = getLocalMember();
   BASSERT(pLocalMember);

   if (pLocalMember == NULL)
      return false;

   if (mpLeaderPicker)
   {
      mpLeaderPicker->setPreviousLeader(pLocalMember->mSettings.mCiv, pLocalMember->mSettings.mLeader);
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         pLocalMember->mSettings.mCiv = pLeader->mCiv;
         pLocalMember->mSettings.mLeader = pLeader->mLeader;
      }
   }
   sendUpdatedLocalMemberSettings(pLocalMember);
   
   return true;

}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::incrementLeader()
{
   if (isLocalPlayerReady())
      return false;

   // push the new civ/leader settings into my settings and send out
   BPartySessionPartyMember* pLocalMember = getLocalMember();
   BASSERT(pLocalMember);

   if (mpLeaderPicker)
   {
      mpLeaderPicker->setNextLeader(pLocalMember->mSettings.mCiv, pLocalMember->mSettings.mLeader);
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         pLocalMember->mSettings.mCiv = pLeader->mCiv;
         pLocalMember->mSettings.mLeader = pLeader->mLeader;
      }
   }

   sendUpdatedLocalMemberSettings(pLocalMember);
   return true;

}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::changeTeamLeft()
{
   if (mPartyRoomMode!=cPartyRoomModeCustom)
      return false;
   if (isLocalPlayerReady())
      return false;
   changeTeam(cInputDecrement);

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::changeTeamRight()
{
   if (mPartyRoomMode!=cPartyRoomModeCustom)
      return false;
   if (isLocalPlayerReady())
      return false;
   changeTeam(cInputIncrement);

   return true;
}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::kickPlayer(BPartySessionPartyMember* pMember)
{
   if (!isLocalPlayerHost())
      return false;

   if (!pMember)
      return false;

   if (isLocalPlayer(pMember) &&
      (pMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberPrimaryPlayer))
   {
      return false;
   }

   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   mKickTargetXUID = pMember->mXuid;
   puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24039), BUIGlobals::cDialogButtonsOKCancel, cYornCallbackContextKickPlayer);

   return true;

}

//==============================================================================
//
//==============================================================================
bool BModePartyRoom2::kickPlayer()
{
   if (!isLocalPlayerHost())
      return false;

   BPartySessionPartyMember* pFocusMember = getPlayerBySlot(mpPartyRoomScreen->getPlayerCurrentSlotFocus());
   if (!pFocusMember)
      return false;


   if (isLocalPlayer(pFocusMember) &&
      (pFocusMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberPrimaryPlayer))
   {
      return false;
   }

   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   BASSERT(puiGlobals);
   mKickTargetXUID = pFocusMember->mXuid;
   puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24039), BUIGlobals::cDialogButtonsOKCancel, cYornCallbackContextKickPlayer);

   return true;

}

//==============================================================================
// BModePartyRoom2::mpSessionEvent_LANGameListUpdated
//    - called when the LAN list of games is updated.
//==============================================================================
void BModePartyRoom2::mpSessionEvent_LANGameListUpdated(void)
{
   if (mState != cStateInLANGameList)
      return;

   mpPartyRoomScreen->populateGamesList();
   mpPartyRoomScreen->displayButtons();
}

//==============================================================================
//
//==============================================================================
long BModePartyRoom2::getPartyRoomView()
{
   long view = BUIMPSetupScreen::cPRViewPlayerList;
   if (mpPartyRoomScreen)
      view = mpPartyRoomScreen->getView();

   return view;
}

