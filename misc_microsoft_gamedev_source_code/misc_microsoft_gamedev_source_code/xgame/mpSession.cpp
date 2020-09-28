//==============================================================================
// mpSession.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#include "Common.h"
#include "mpSession.h"
#include "LiveSystem.h"
#include "liveSession.h"
#include "liveSessionSearch.h"
#include "liveVoice.h"
#include "mpGameSession.h"
#include "XLastGenerated.h"            //For the TitleID

#include "mpSimDataObject.h"
#include "mpcommheaders.h"
#include "commlog.h"

#include "session.h"
#include "Channel.h"
#include "OrderedChannel.h"      

// xsystem
#include "config.h"
#include "econfigenum.h"
#include "consoleOutput.h"
#include "notification.h"
#include "xmlreader.h"
#include "xmlwriter.h"

// game -just so I can start commlogs...
#include "game.h"
#include "modemanager.h"
#include "usermanager.h"
#include "user.h"
#include "gamesettings.h"
#include "GamerPicManager.h"

//From MP - replace at some point
#include "channels.h"

//LSP connection
#include "lspManager.h"

//==============================================================================
// 
//==============================================================================
BMPSession::BMPSession() :
   mState(cMPSessionStateNone),
   mPartySession(NULL),
   mGameSession(NULL),
   mLiveMatchMaking(NULL),
   mPort(0),
   mSession(NULL),
   mLocalChecksum(NULL),
   mpSimObject(NULL),
   mGameInterface(NULL),
   mSessionInterface(NULL),
   mGameSettings(NULL),
   mLocalControllerID(0),
   mLocalXuid(0),
   mLocalPlayerID(cMPInvalidPlayerID),
   mpVoice(NULL),
   mStartupMode(mpSessionStartupNone),
   mXNAddrRequestTime(0),
   mJoinedViaInvite(FALSE),
   mMatchMakingRunning(FALSE),
   //mTimeSinceLastAuthRequest(0),
   mpLanDiscovery(NULL),
   mResetTimer(0),
   mDoNotDismissDialog(false),
   mBelayNextGameDisconnectEvent(false),
   mBelayedNonceForGameDisconnectEvent(0),
   mResetPartySessionNextUpdate(false)
{
   registerMPCommHeaders();

   //When in debug - turn on more output spew
#ifdef BUILD_DEBUG
   if (gConfig.isDefined(cConfigMoreLiveOutputSpam))
   {      
      XDebugSetSystemOutputLevel(HXAMAPP_XGI, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XLIVEBASE, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XAM, 3);
   }   
#endif
}

//==============================================================================
//
//==============================================================================
BMPSession::~BMPSession()
{
   shutdown();

   //Kill the extra spew in case I had it on
#ifdef BUILD_DEBUG
   XDebugSetSystemOutputLevel(HXAMAPP_XGI, 1);
   XDebugSetSystemOutputLevel(HXAMAPP_XLIVEBASE, 1);
   XDebugSetSystemOutputLevel(HXAMAPP_XAM, 1);
#endif
}

//==============================================================================
//
//==============================================================================
void BMPSession::init(BLiveVoice* pVoice)
{
   mpVoice = pVoice;
}

//==============================================================================
// This initializes all the internal systems, it needs a pointer to a game interface 
//==============================================================================
void BMPSession::startUp(GameInterface* gInt, mpSessionInterface* sInt, BDataSet *settings, BmpSessionStartupMode startupMode, DWORD controllerID)
{
   gGame.initCommLogging();
   nlog(cMPGameCL, "BMPSession::startUp");

   if (!gInt || !sInt)
   {
      nlog(cMPGameCL, "BMPSession::startUp - **ERROR** There must be a GameInterface and SessionInterface to startup this object.");
      BFAIL( "BMPSession::startUp - **ERROR** There must be a GameInterface and SessionInterface to startup this object." );
      return;
   }

   if (mState != cMPSessionStateNone)
   {
      nlog(cMPGameCL, "BMPSession::startUp - **ERROR** BMPSession not in state NONE");
      BASSERTM(false, "BMPSession::startUp - **ERROR** BMPSession not in state NONE" );
      reset();
      return;
   }

   // should initialize voice here?
   BASSERT(mpVoice); 

   //Hook up the settings
   mGameSettings = settings;
   //mGameSettings->addDataListener(this);

   mState = mpSessionStartupWaitingForOKFromLiveSystem;
   mXNAddrRequestTime = timeGetTime();
   mStartupMode = startupMode;
   mGameInterface = gInt;
   mLocalChecksum = gInt->getLocalChecksum() ^ cProductVersion;
   // comm logging is initalized at the top of the method
   //mGameInterface->initCommLogging();
   mSessionInterface = sInt;
   mMatchMakingRunning = FALSE;

   //WRONG - this is done in the gameSession as it startsup
   //mGameInterface->setupSync(this);
   //gLiveSystem->setLiveRequired( !isInLANMode() );
   setControllerID(controllerID);

   mDoNotDismissDialog = false;

   //Rest of startup code happens in update when we have an XNAddr
}

//==============================================================================
// Clean up everything
//==============================================================================
void BMPSession::shutdown()
{
   reset();

   //mTimeSinceLastAuthRequest = 0;
   mState = mpSessionStateShuttingDownToDelete;
}

//==============================================================================
//
//==============================================================================
DWORD BMPSession::getCachedGameChecksum()
{
   BASSERT(mLocalChecksum);
   BASSERT(mState >= mpSessionStartupWaitingForOKFromLiveSystem);
   return mLocalChecksum;
}

//==============================================================================
// Disconnects from any current session and cleans up without destroying connection to its creator
// Will need a startup to get going again after this
//==============================================================================
void BMPSession::reset()
{
   if (mState == mpSessionStateShuttingDownToReset || mState == cMPSessionStateNone)
      return;

   nlog(cMPGameCL, "BMPSession::reset");
   mResetTimer = timeGetTime();
   abortGameSession();

   if (mLiveMatchMaking)
   {
      delete mLiveMatchMaking;
      mLiveMatchMaking = NULL;
   }
   mMatchMakingRunning = FALSE;    

   BCommLog::cycleLogFile();

   if (gLiveSystem->isPartySystemFlaggedForReenter())
   {
      //We don't want anything else reset
      mState = mpSessionStateShuttingDownToReset;
      return;
   }

   if (mPartySession)
   {
      mPartySession->shutdown();
      if (mPartySession->isShutdown())
      {
         delete mPartySession;
         mPartySession = NULL;
      }
   }

   mStartupMode = mpSessionStartupNone;
   mGameInterface = NULL;
   mSessionInterface = NULL;
   mGameSettings = NULL;
   mLocalControllerID = 0;
   mPort = 0;
   mLocalXuid = 0;
   mLocalChecksum = NULL;
   mLocalGamertag.empty();
   mBelayNextGameDisconnectEvent = false;
   mBelayedNonceForGameDisconnectEvent = 0;

   deinitLanDiscovery();

   // voice is going away after this reset?
   mpVoice = NULL;

   //mDoNotDismissDialog = false;

   mResetTimer = timeGetTime();
   mState = mpSessionStateShuttingDownToReset;
}

//==============================================================================
// 
//==============================================================================
void BMPSession::abortGameSession()
{
   nlog(cMPGameCL, "BMPSession::abortGameSession");

   if (mGameSession)
   {
      // check if the game session actually got a game to go
      // this is different than isRunning or isGameRunning because
      // isGame will return true if there was ever a game running
      // isGameRunning will return false when the game is over, but we still
      // want to know if it was a valid game at some point
      //
      // so if we never got a game going and I'm the party session host,
      // then I can tell everyone to leave, otherwise, just divorce myself from the game
      //
      // FIXME, this will require more love post-alpha because we'll want people in the party
      // to stay together from game to game
      if (!mGameSession->isGame() && mPartySession && mPartySession->isHosting())
      {
         //To be nice - if we are the party host, tell everyone to leave that target
         nlog(cMPGameCL, "BMPSession::abortGameSession - Telling party session we have left that target nonce[%I64u]", mGameSession->getNonce());
         mPartySession->partySendLeaveTargetCommand(mGameSession->getNonce());
      }
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
      nlog(cMPGameCL, "BMPSession::abortGameSession - Shutting down the game session");
   }

   mJoinedViaInvite = FALSE;
   mLocalPlayerID = cMPInvalidPlayerID;
}

//==============================================================================
// 
//==============================================================================
void BMPSession::abortPartySession()
{
   //This is a 'omg bad' cleanup and shutdown the party method
   //It will show a party going away message, start cleaning everything up, and go to the main menu
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   BASSERT(pUIGlobals);
   if (pUIGlobals)
   {
      pUIGlobals->setWaitDialogVisible(false);
      pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25443), BUIGlobals::cDialogButtonsOK);
      mDoNotDismissDialog = true;
   }
   gModeManager.setMode(BModeManager::cModeMenu);
   reset();
}


//==============================================================================
//Call this to fire off the end of game logic that posts scores, changes voice channels, etc
//==============================================================================
void BMPSession::processEndOfGame()
{
   //Live end of game processing
   if (!isInLANMode() &&
       getLiveSession())
   {
      getLiveSession()->endGame();
   }

   //Set the voice channels for everyone
   BDEBUG_ASSERT(mpVoice);
   if (mpVoice != NULL)
      mpVoice->setAllChannels(BVoice::cGameSession, BVoice::cAll);

   //No longer unlocking until the host comes back into the party and says so
   /*
   if (mPartySession)
      mPartySession->lockPartyMembers(false);
   */
}

//==============================================================================
// 
//==============================================================================
HRESULT BMPSession::setControllerID(const uint32 controllerID)
{
//-- FIXING PREFIX BUG ID 1428
   const BUser* pUser = gUserManager.getUserByPort(controllerID);
//--
   BASSERTM(pUser, "Failed to find user for requested controller ID");
   if (pUser == NULL)
      return E_FAIL;

   mLocalControllerID = controllerID;

   mLocalGamertag = pUser->getName();
   mLocalXuid = pUser->getXuid();

   //mLocalControllerID = controllerID;

   //CHAR pzUserName[XUSER_NAME_SIZE];
   //HRESULT hr = XUserGetName( mLocalControllerID, pzUserName, XUSER_NAME_SIZE);
   //if (FAILED(hr))
   //   return hr;

   //pzUserName[ XUSER_NAME_SIZE - 1 ] = '\0';
   //mLocalGamertag.set(pzUserName);

   //hr = XUserGetXUID(mLocalControllerID, &mLocalXuid);
   //if (FAILED(hr))
   //   return hr;

   return S_OK;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::sendSyncData(long uid, uint checksum)
{
   if (!mGameSession)
   {
      return false;
   }

   return mGameSession->sendSyncData(uid, checksum);
}

//==============================================================================
//
//==============================================================================
long BMPSession::getSyncedCount(void) const
{
   if (!mGameSession)
   {
      return false;
   }

   return mGameSession->getSyncedCount();
}

//==============================================================================
//
//==============================================================================
void BMPSession::outOfSync(void) const
{
   if (!mGameSession)
   {
      return;
   }

   mGameSession->outOfSync();
}

//==============================================================================
//
//==============================================================================
//Call this to let the system know you are done loading the level
void BMPSession::gameDoneLoading()
{
   deinitLanDiscovery();

   if (!mGameSession || !mGameSession->isRunning())
   {
      nlog(cMPGameCL, "BMPSession::gameDoneLoading -- Error, no running game session");
      //TODO - add additional logic here, what happened was that while the game was loading, the network game session went away
      BASSERT(false);
      reset();
      return;
   }

   nlog(cMPGameCL, "BMPSession::gameDoneLoading - Passing to game session");

   mGameSession->gameDoneLoading();
}

//==============================================================================
// 
//==============================================================================
bool BMPSession::allPlayersLoaded() const
{
   if (mGameSession == NULL)
      return true;

   return mGameSession->allMachinesFinalized();
}

//==============================================================================
// BMPSession::requestLaunchAbort
//==============================================================================
bool BMPSession::requestLaunchAbort(long reason)
{
   if (!mGameSession)
   {
      nlog(cMPGameCL, "BMPSession::requestLaunchAbort -- Error, no running game session");
      return (false);
   }

   //Big change here - we used to decide here at a cancel if we wanted to ask for a launch abort, or just drop the game session
   //  Now - we just always shutdown the game session because a successful abort just does the same thing (after much network traffic and talking)
   //  So new way just uses a single path - no more launch abort
   //if (!mGameSession->isRunning())
   {
      nlog(cMPGameCL, "BMPSession::requestLaunchAbort -- Game session is not running to abort, shutting down that game session");
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
      return (true);
   }

   //nlog(cMPGameCL, "BMPSession::requestLaunchAbort - Passing to game session");
   //return (mGameSession->requestLaunchAbort(reason));
}

//==============================================================================
//
//==============================================================================
void BMPSession::sendLaunchReady(bool ready)
{
   //Deprecated
   BASSERT(false);

   if (!mGameSession || !mGameSession->isRunning())
   {
      nlog(cMPGameCL, "BMPSession::sendLaunchReady -- Error, no running game session");
      return;
   }

   nlog(cMPGameCL, "BMPSession::sendLaunchReady - Passing to game session");
   mGameSession->sendLaunchReady(ready);
}

//==============================================================================
//
//==============================================================================
bool BMPSession::requestGameLaunch(DWORD countdown)
{
   if (!mGameSession || !mGameSession->isRunning())
   {
      nlog(cMPGameCL, "BMPSession::requestGameLaunch -- Error, no running game session");
      return (false);
   }

   nlog(cMPGameCL, "BMPSession::requestGameLaunch - Passing to game session");
   return mGameSession->requestGameLaunch(countdown);
}

//==============================================================================
// 
//==============================================================================
void BMPSession::update()
{

   //switch (gNotification.getNotification())
   //{
   //   case XN_SYS_MUTELISTCHANGED:
   //   case XN_FRIENDS_FRIEND_ADDED:
   //   case XN_FRIENDS_FRIEND_REMOVED:
   //   case XN_SYS_SIGNINCHANGED:
   //   case XN_SYS_PROFILESETTINGCHANGED:
   //      {
   //         nlog(cMPVoiceCL, "BMPSession::update -- Got system friend/mute event, updating talker list to have it re-evaluate");
   //         updateTalkerList(); // update the talker list
   //         break;
   //      }
   //   default:
   //      break;
   //}

   switch (gNotification.getNotification())
   {
      //case XN_SYS_SIGNINCHANGED:
      //   {
      //      // check if the local controller ID's xuid has changed
      //      // and if so, we should kick the player back to the main menu
      //      // or we should set the local controller ID to the primary user's
      //      // controller ID
      //      //
      //      // XXX FIXME we need to support the notion of primary and secondary users for coop play
//-- FIXING PREFIX BUG ID 1429
      //      const BUser* pUser = gUserManager.getPrimaryUser();
//--
      //      if (pUser == NULL || pUser->getXuid() != mLocalXuid)
      //      {
      //         if (mPartySession || mLiveMatchMaking || mGameSession)
      //         {
      //            gModeManager.setMode(BModeManager::cModeMenu);
      //         }
      //      }
      //      break;
      //   }

      case XN_LIVE_CONNECTIONCHANGED:
         {
            // if I lose my connection to Live, that will affect
            // matchmaking and hosting/joining games but not
            // really affect games that are in-progress, especially
            // if we're in LAN mode, so I need to be careful here
            // and intelligently move us back to the main menu depending
            // on those factors.
            if (gNotification.getParam() != XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
            {
               // if the matchmaker is running or
               // we have a party session but no game running and we're in LAN mode
               // then kick us back to the main menu
               //
               if (isMatchmakingRunning() || (mPartySession && !isInLANMode() && !mGameSession) ||
                  (!mPartySession && gModeManager.getModeType() == BModeManager::cModePartyRoom2))
               {
                  //If they are in a LIVE party - we have to kill that party - even if the game is still going
                  if (mPartySession && !isInLANMode())
                  {
                     mPartySession->shutdown();
                  }

                  if (gModeManager.inModeGame())
                     return;

                  // 25442 You have lost your connection to Xbox LIVE.  Unable to continue matchmaking.
                  // 25443 You have lost your connection to Xbox LIVE.  Unable to maintain the party.
                  long stringID = (isMatchmakingRunning() ? 25442 : 25443);
                  BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  BASSERT(pUIGlobals);
                  if (pUIGlobals)
                  {
                     pUIGlobals->setWaitDialogVisible(false);
                     pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(stringID), BUIGlobals::cDialogButtonsOK);
                     mDoNotDismissDialog = true;
                  }

                  gModeManager.setMode(BModeManager::cModeMenu);
                  return;
               }
            }
            break;
         }

      case XN_LIVE_LINK_STATE_CHANGED:
         {
            if (static_cast<BOOL>(gNotification.getParam()) == FALSE)
            {
               // if our link state changes while we have an active party, game, in matchmaking or entering the party mode
               // then we want to inform the user that they've lost their connection
               //
               // the party is not immediately started when entering the party, so that's why we need to check for it here
               if (mPartySession || mLiveMatchMaking || mGameSession || gModeManager.getModeType() == BModeManager::cModePartyRoom2)
               {
                  // 25444 You have lost your network connection.
                  BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  BASSERT(pUIGlobals);
                  if (pUIGlobals)
                  {
                     pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25444), BUIGlobals::cDialogButtonsOK);
                     mDoNotDismissDialog = true;
                  }

                  gModeManager.setMode(BModeManager::cModeMenu);
                  return;
               }
            }
         }
   }
    
   //Quick check for the reset-me! state
   if (mState==mpSessionStateGameSessionResetNextUpdate)
   {
      nlog(cMPGameCL, "BMPSession::update -- mpSession was marked to reset.... reseting!");
      reset();
   }

   //Quick check for the reset the party state
   if (mResetPartySessionNextUpdate)
   {
      if (mPartySession)
      {
         mPartySession->shutdown();
      }
      mResetPartySessionNextUpdate = false;
   }

   if (mPartySession)
   {
      mPartySession->update();
      if (mPartySession->isShutdown())
      {
         delete mPartySession;
         mPartySession = NULL;
         nlog(cMPGameCL, "BMPSession::update -- Party session shutdown complete, deleted instance");
      }
   }

   if (mLiveMatchMaking)
   {
      mLiveMatchMaking->update();
   }

   if (mGameSession)
   {
      mGameSession->update();
      if (mGameSession->isShutDown())
      {
         delete mGameSession;
         mGameSession = NULL;
         nlog(cMPGameCL, "BMPSession::update -- Game session shutdown complete, deleted instance");
      }
   }

   //State dependent internal updates *************************************************
   switch (mState)
   {

      case (mpSessionStartupWaitingForOKFromLiveSystem) :
         {
            if (!gLiveSystem->getLeaderboardOnlySession())
            {
               mState = mpSessionStateStartingWaitingForXNAddr;
            }
            break;
         }
      case (mpSessionStateStartingWaitingForXNAddr) :
         {
            XNADDR localXnAddr;
            if (gLiveSystem->getLocalXnAddr(localXnAddr))
            {


               //I have an XNAddr - go to the next phase
               if (!isInLANMode())
               {
                  mState = mpSessionStateStartingWaitingForLoggin;
                  //Wait til we have a live login
               }
               else
               {
                  //LAN mode - need to setup a gameinfo port and start listening on it      
                  initLanDiscovery();
                  //Hopper data should be loaded from the XML file already - apply the default settings (just so the custom room is enabled)
                  if (!gLiveSystem->getHopperList()->isLoaded())
                  {
                     //It is not loaded?  Thats a total failure
                     if (mSessionInterface)
                     {
                        mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectCouldNotLoadDefaultConfig);
                     }
                     nlog(cMPGameCL, "BMPSession::update - state mpSessionStateStartingWaitingForXNAddr, hopper list failed to load, reseting");
                     gLiveSystem->setPartySystemForReenter(false);
                     reset();
                     //mState = mpSessionStateError;
                     return;
                  }
				  //Check if the party system has not been dumped (which is bad)
				  if (mPartySession)
				  {
					  BASSERT(false);
					  //Lets just flag a restart
					  if (mSessionInterface)
					  {
						  mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectLostLiveConnection);
					  }
					  nlog(cMPGameCL, "BMPSession::update - mPartySession is not NULL during a startup, reseting all MPSession");
					  gLiveSystem->setPartySystemForReenter(false);
					  reset();
					  return;
				  }
                  applyNoLSPConfigSettings();
                  setupPartySession();
                  //Tell the session interface that we have connected (so that it can display the UI to pick a LAN game or host)
                  mSessionInterface->mpSessionEvent_ESOConnectComplete( false );
                  mState = mpSessionStateLANSessionDiscovery;
               }
            }
            else
            {
               //Check if we have waited too long
               if (timeGetTime()-mXNAddrRequestTime > 5000)
               {
                  BUIGlobals* puiGlobals = gGame.getUIGlobals();
                  if (puiGlobals)
                  {
                     puiGlobals->setWaitDialogVisible(false);
                     puiGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25444), BUIGlobals::cDialogButtonsOK);
                     mDoNotDismissDialog = true;
                  }                  
                  gModeManager.setMode(BModeManager::cModeMenu);
                  reset();
               }
            }
            break;
         }

      case (mpSessionStateStartingWaitingForLoggin) :
         {
            // only entering this code for LIVE games

            //Am I done starting up?
            //Yes - If i'm logged in if I need to be
            BASSERT(!isInLANMode());        //Should never get here if Im in LAN mode
            if (gUserManager.areUsersSignedIn())
            {
               //Check if they have permissions to play online
//-- FIXING PREFIX BUG ID 1430
               const BUser* pUser = gUserManager.getPrimaryUser();
//--
               if (pUser == NULL || mSessionInterface == NULL)
               {
                  // Please sign-in with an Xbox LIVE gamer profile.
                  if (mSessionInterface)
                  {
                     mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectAccountNotSignedIn);
                  }
                  nlog(cMPGameCL, "BMPSession::update - state mpSessionStateStartingWaitingForLoggin, no primary user, reseting");
                  //mState = mpSessionStateError;
                  gLiveSystem->setPartySystemForReenter(false);
                  reset();
                  return;
               }
               else if (pUser != NULL)
               {
                  if (pUser->isLiveEnabled() && !pUser->isSignedIntoLive())
                  {
                     // the user is not signed-in but they're LIVE enabled, ask them to sign-in
                     if (mSessionInterface)
                     {
                        mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectLostLiveConnection);
                     }
                     nlog(cMPGameCL, "BMPSession::update - state mpSessionStateStartingWaitingForLoggin, primary user Live enabled but not signed in");
                     gLiveSystem->setPartySystemForReenter(false);
                     reset();
                     //mState = mpSessionStateError;
                     return;
                  }
                  else if (!pUser->isLiveEnabled() || !pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
                  {
                     // the user is signed-in and Live enabled but blocked from playing MP games
                     if (mSessionInterface)
                     {
                        mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectAccountHasLiveGamesBlocked);
                     }
                     nlog(cMPGameCL, "BMPSession::update - state mpSessionStateStartingWaitingForLoggin, primary user not Live enabled or does not have MP session priv");
                     gLiveSystem->setPartySystemForReenter(false);
                     reset();
                     //mState = mpSessionStateError;
                     return;
                  }
               }

               //We need to connect to ESO, validate user, get config, get NAT type
               //Launch login request
               if (remoteRequestESOLogin()==FALSE)
               {
                  if (mSessionInterface)
                  {
                     mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectCouldNotRequestLogin);
                  }
                  nlog(cMPGameCL, "BMPSession::update - state mpSessionStateStartingWaitingForLoggin, remoteRequestESOLogin failed immediately");
                  gLiveSystem->setPartySystemForReenter(false);
                  reset();
                  //mState = mpSessionStateError;
                  return;
               }  

               nlog(cMPGameCL, "BMPSession::update -- Live login ok, waiting for ESO login");
               mState = mpSessionMMWaitingForESOLogin;
            }
            break;
         }
      
      case (mpSessionStateLANSessionDiscovery) :
         {
            if (!isInLANMode())
            {
               //Not in LAN mode - don't need this
               BASSERT(false);
               return;
            }

            if (mpLanDiscovery && mSessionInterface)
            {
               if (mpLanDiscovery->isListUpdated())
               {
                  mpLanDiscovery->resetListUpdated();

                  mSessionInterface->mpSessionEvent_LANGameListUpdated();
               }
            }
            break;
         }
         
      case (mpSessionStateIdle) :
         {
            break;
         }

      case (mpSessionStateGameSessionShuttingDown) :
         {
            //We are just waiting for the game session to cleanup so we can go back to idle
            if (!mGameSession)
            {
               nlog(cMPGameCL, "BMPSession::update -- Game session reset, returning to state IDLE");
               mState = mpSessionStateIdle;
            }
            break;
         }

      case (mpSessionStateShuttingDownToDelete) :
         {
            //We are just waiting for the game session to cleanup so we can go back to idle
            if (!mGameSession)
            {
               nlog(cMPGameCL, "BMPSession::update -- mpSession shutdown now complete");
               mState = mpSessionStateShutdownComplete;
            }
            break;
         }

      case (mpSessionStateShuttingDownToReset) :
         {
            //We are just waiting for the game session/party session to cleanup so we can go back to idle
            if (!mGameSession &&
                !mLiveMatchMaking &&
                (gLiveSystem->isPartySystemFlaggedForReenter() || !mPartySession))
            {
               nlog(cMPGameCL, "BMPSession::update -- mpSession reset now complete");
               mState = cMPSessionStateNone;
            }
            else if (timeGetTime()-mResetTimer > 10000)
            {
               //Attempt at disaster recovery here
               //If it has just taken WAY too long for this, then lets re-fire the reset
               nlog(cMPGameCL, "BMPSession::update -- mpSessionStateShuttingDownToReset has taken too long, attempting to re-issue reset");
               BASSERTM(false, "BMPSession::update -- mpSessionStateShuttingDownToReset has taken too long, attempting to re-issue reset");
               gLiveSystem->setPartySystemForReenter(false);
               mState = mpSessionStateIdle;
               reset();
            }
            break;
         }
         

      case (mpSessionMMWaitingForESOLogin) :
         {
            //Check status on our login request
            if (gLSPManager.isInNoLSPMode())
            {
               nlog(cMPGameCL, "BMPSession::update -- We are in NOLSP mode, skip auth/config");
               mState = mpSessionMMWaitingForESOConfigurationData;
            }
            else if (!gLSPManager.isAuthorizing())
            {
               //It has finished!
               BUser* pUser = gUserManager.getPrimaryUser();
               if (pUser && pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
               {
                  BLSPResponse response = gLSPManager.isBanned(pUser->getXuid());
                  if (response == cFailed)
                  {
                     nlog(cMPGameCL, "BMPSession::update -- ESO login FAILED!");
                     if (mSessionInterface)
                     {
                        mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectSerivceDown);
                     }
                     nlog(cMPGameCL, "BMPSession::update - state mpSessionMMWaitingForESOLogin, failed auth");
                     gLiveSystem->setPartySystemForReenter(false);
                     reset();
                     //mState = mpSessionStateError;
                     break;
                  }
                  else if (response == cPending)
                     break;
                  else if (response == cYes)
                  {
                     nlog(cMPGameCL, "BMPSession::update -- ESO login BANNED!");
                     if (mSessionInterface)
                     {
                        mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectAccountBanned);
                     }
                     nlog(cMPGameCL, "BMPSession::update - state mpSessionMMWaitingForESOLogin, auth says they are banned");
                     gLiveSystem->setPartySystemForReenter(false);
                     reset();
                     //mState = mpSessionStateError;
                     break;
                  }
               }

               if (gLSPManager.isConfigDataCurrent() &&
                   gLiveSystem->getHopperList() && 
                   gLiveSystem->getHopperList()->isLoaded())
               {
                  //So if we have a base list loaded, and the data is current - don't re-request here
                  //  The reason being that the cache system will ignore the request, but the next step will be cycling
                  //  waiting on a request to finish
                  mState = mpSessionMMSkipNewESOConfigurationData;
                  nlog(cMPGameCL, "BMPSession::update -- ESO login ok, config data up-to-date");
               }
               else
               {
                  gLSPManager.requestConfigData(gLiveSystem->getHopperListUpdatedVersion());
                  BASSERT(gLSPManager.requestConfigDataRunning());
                  if (!gLSPManager.requestConfigDataRunning())
                  {
                     //I requested it but it denied running it - we'll have to just go with what is loaded
                     mState = mpSessionMMSkipNewESOConfigurationData;
                     nlog(cMPGameCL, "BMPSession::update -- ESO login ok, requested ESO config data but LSPManager denied request");
                  }
                  else
                  {
                     mState = mpSessionMMWaitingForESOConfigurationData;
                     nlog(cMPGameCL, "BMPSession::update -- ESO login ok, waiting for ESO config data");
                  }
               }               
            }
            break;
         }

      case (mpSessionMMWaitingForESOConfigurationData) :
      case (mpSessionMMSkipNewESOConfigurationData) :
         {
            //Check if the file has been pulled down and processed
            if (!gLSPManager.requestConfigDataRunning() ||
                 (mState==mpSessionMMSkipNewESOConfigurationData))
            {
               if (!mSessionInterface)
               {
                  nlog(cMPGameCL, "BMPSession::update -- state mpSessionMMWaitingForESOConfigurationData - ERROR, no session interface");
                  //mState = mpSessionStateError;
                  gLiveSystem->setPartySystemForReenter(false);
                  reset();
                  return;
               }

               //It is not running so the request is complete - did it work?
               if (gLiveSystem->getHopperListUpdatedVersion()->isLoaded())
               {
                  //Yes! Make that our hopperlist/config to use
                  nlog(cMPGameCL, "BMPSession::update -- Hopper list recv from LSP, posting as current");
                  gLiveSystem->postUpdatedVersionToBaseHopperList();
               }
               else
               {
                  //Then just use the previously updated one (maybe the one loaded from disc)
                  nlog(cMPGameCL, "BMPSession::update -- No new hopperlist/config from LSP");
                  BASSERT(gLiveSystem->getHopperList()->isLoaded());
                  if (!gLiveSystem->getHopperList()->isLoaded())
                  {
                     nlog(cMPGameCL, "BMPSession::update -- No hopperlist AT ALL - should never happen :|");
                     reset();
                  }
               }

               //Take the LSP config settings and apply them to the game config settings where applicable
               if (gLiveSystem->getHopperList()->getStaticData()->mConnectionProxyEnabled)
               {
                  gConfig.define(cConfigProxySupport);
               }
               else
               {
                  gConfig.remove(cConfigProxySupport);
               }               
               gConfig.set(cConfigSocketUnresponsiveTimeout, (long)gLiveSystem->getHopperList()->getStaticData()->mNetworkSocketUnresponsiveTimeout);
               gConfig.set(cConfigJoinRequestTimeout, (long)gLiveSystem->getHopperList()->getStaticData()->mNetworkJoinRequestTimeout);
               gConfig.set(cConfigProxyRequestTimeout, (long)gLiveSystem->getHopperList()->getStaticData()->mNetworkProxyRequestTimeout);
               gConfig.set(cConfigProxyPingTimeout, (long)gLiveSystem->getHopperList()->getStaticData()->mNetworkProxyPingTimeout);

               //Check our NAT type
               mNatType = XOnlineGetNatType();
#ifndef BUILD_FINAL
               if (gConfig.isDefined(cConfigForceRestrictedNAT))
               {
                  mNatType = XONLINE_NAT_STRICT;
               }
#endif
               switch(mNatType) 
               {
                  case (XONLINE_NAT_OPEN)     : nlog(cMPGameCL, "BMPSession::update -- Checking Nat Type: OPEN (woot)"); break;
                  case (XONLINE_NAT_MODERATE) : nlog(cMPGameCL, "BMPSession::update -- Checking Nat Type: MODERATE"); break;
                  case (XONLINE_NAT_STRICT)   : nlog(cMPGameCL, "BMPSession::update -- Checking Nat Type: STRICT (crap)"); break;
               }                  
            
               //Lets make sure we don't still have a game session hanging around from last game
               if (mGameSession)
               {
                  nlog(cMPGameCL, "BMPSession::update -- Game session was valid at BMPSession init time, shutting down that instance");
                  mGameSession->shutDown();
               }

			   //Check if the party system has not been dumped (which is bad)
			   if (mPartySession)
			   {
				   BASSERT(false);
				   //Lets just flag a restart
				   if (mSessionInterface)
					  {
						  mSessionInterface->mpSessionEvent_ESOConnectFailed(cMPSessionESOConnectLostLiveConnection);
					  }
				   nlog(cMPGameCL, "BMPSession::update - mPartySession is not NULL during a startup, reseting all MPSession");
				   gLiveSystem->setPartySystemForReenter(false);
				   reset();
				   return;
			   }

               //Start the party session up
               setupPartySession();
               mState = mpSessionStateStartingPartySession;

               //Tell the owning system above that we are done connecting/validating
               //That system will tell us to either host the party, or join a particular party
               mSessionInterface->mpSessionEvent_ESOConnectComplete( hasRestrictiveNAT() );

               //Set the hopper list to this restrictive NAT filter mode
               gLiveSystem->getHopperList()->setRestrictedNATMode(hasRestrictiveNAT());

               //When the party session is ready, it will call BPartySessionEvent_systemReady
               //On failure it will call BPartySessionEvent_systemStartupFailed
               //We are in mState = mpSessionStateStartingPartySession;
                  
               nlog(cMPGameCL, "BMPSession::update -- Got ESO config data and applied it");
            }
            break;
         }

      case (mpSessionStateStartingPartySession):
         {
            //There should always be a party session running here
            BASSERTM(mPartySession, "BMPSession::update - Party session is NULL, should never hit this message, report if you do - eric");
            break;
         }

   }     //end of switch (mState)
}

//==============================================================================
// 
//==============================================================================
BOOL BMPSession::isHosting() const
{
   if (mGameSession && mGameSession->isHosting())
   {
      return TRUE;
   }

   return FALSE;
}

//==============================================================================
// 
//==============================================================================
BOOL BMPSession::isRunning() const
{
   if ((mState >= mpSessionStateIdle) &&
       (mState < mpSessionStateDeleting) &&
       (mGameSession) &&
       (mGameSession->isRunning()))
   {
      return TRUE;
   }
   return FALSE;
}

//==============================================================================
// 
//==============================================================================
BOOL BMPSession::isGameRunning() const
{
   if (mGameSession && mGameSession->isGameRunning())
   {
      return TRUE;
   }
   return FALSE;
}

//==============================================================================
//Call this if you are trying to figure out if the current game is a matchmade game or not
//==============================================================================
BOOL BMPSession::isMatchmadeGame()
{
   if (mGameSession && mGameSession->getLiveSession())
   {
      return mGameSession->getLiveSession()->isMatchmadeGame();
   }
   return false;
}

//==============================================================================
//Call this if you want to know if all members in a matchmaking party have joined to the current target
//==============================================================================
BOOL BMPSession::isMatchMakingPartyFullyJoinedToCurrentTarget()
{
   if (mLiveMatchMaking && mLiveMatchMaking->isRunning())
   {
      return (mLiveMatchMaking->isPartyFullyJoinedToCurrentTarget());
   }
   return false;
}

//==============================================================================
// 
//==============================================================================
bool BMPSession::joinLanGame(const BLanGameInfo& lanInfo)
{
   XNADDR localXnAddr;
   if (!gLiveSystem->getLocalXnAddr(localXnAddr) )
   {
      return false;
   }

   if (mState != mpSessionStateIdle)
   {
      //TODO log/return error condition?
      BASSERT(false);
      return false;
   }

   if (!isInLANMode())
   {
      return false;
   }

   //Pre-init for the game 
   if (!mSessionInterface || !mSessionInterface->mpSessionEvent_preGameInit( FALSE ))
   {
      //Immediate failure to connect
      nlog(cMPMatchmakingCL, "BMPSession::joinLanGame -- mpSessionEvent_preGameInit failed");
      return false;
   }

   mState = mpSessionStateStartingGameSessionJoin;
   mBelayNextGameDisconnectEvent = false;

   mGameSession = new BMPGameSession(this, mGameSettings);
   return (mGameSession->joinLanGame(lanInfo));
}

//==============================================================================
// 
//==============================================================================
bool BMPSession::joinLiveGame( BLiveGameDescriptor* gameDescriptor)
{
   nlog(cMPGameCL, "BMPSession::joinLiveGame -- Requested to join to a Live game");

   XNADDR localXnAddr;
   if (!gameDescriptor || !gLiveSystem->getLocalXnAddr(localXnAddr) )
   {
      return false;
   }

   if (mState != mpSessionStateIdle)
   {
      nlog(cMPGameCL, "BMPSession::joinLiveGame -- Failed, mpSession is NOT in an idle state [%d]", mState);
      BASSERT(false);
      return false;
   }

   BASSERT(mLocalChecksum);

   //Pre-init for the game 
   if (!mSessionInterface || !mSessionInterface->mpSessionEvent_preGameInit( FALSE ))
   {
      //Immediate failure to connect
      nlog(cMPMatchmakingCL, "BMPSession::joinLiveGame -- mpSessionEvent_preGameInit failed");
      return false;
   }

   mState = mpSessionStateStartingGameSessionJoin;
   mBelayNextGameDisconnectEvent = false;

   mGameSession = new BMPGameSession(this, mGameSettings);
   return (mGameSession->joinLiveGame(mLocalControllerID, gameDescriptor));

}

//==============================================================================
// 
//==============================================================================
bool BMPSession::joinLANPartySession(uint lanGameIndex, BPartySessionPlayerSettings* playerSettings)
{
   nlog(cMPGameCL, "BMPSession::joinLANPartySession -- Requested to join to a LAN party session");

   if (mState != mpSessionStateLANSessionDiscovery)
   {
      nlog(cMPGameCL, "BMPSession::joinLANPartySession -- Failed, mpSession is NOT in a valid state [%d]", mState);
      BASSERT(false);
      return false;
   }

   BLanGameInfo* pLanInfo = getLanGame(lanGameIndex);
   if (pLanInfo == NULL)
   {
      nlog(cMPGameCL, "BMPSession::joinLANPartySession -- Failed, invalid index [%d]", lanGameIndex);
      return false;
   }

   if (pLanInfo->getBadCRC() || pLanInfo->getLocked() || pLanInfo->getFilledSlots() == pLanInfo->getMaxSlots())
   {
      nlog(cMPGameCL, "BMPSession::joinLANPartySession -- Failed badCRC[%d] locked[%d] slots[%d/%d] nonce[%I64u]",
         pLanInfo->getBadCRC(), pLanInfo->getLocked(), pLanInfo->getFilledSlots(), pLanInfo->getMaxSlots(), pLanInfo->getNonce());
      return false;
   }

   BASSERT(mLocalChecksum);
   playerSettings->mConnectionState = cBPartySessionMemberConnectionStateJoining;
   const BPartySessionPlayerSettings* temp = playerSettings;
   /*
   BPartySessionPlayerSettings hackDefaultSettings;
   hackDefaultSettings.mCiv = 1;
   hackDefaultSettings.mLeader = 1;
   hackDefaultSettings.mTeam = 0;
   hackDefaultSettings.mSlot = 0;
   hackDefaultSettings.mPartyRoomMode = 0;
   hackDefaultSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
   hackDefaultSettings.clearFlags();
   hackDefaultSettings.mPlayerRank.reset();
   hackDefaultSettings.mCampaignBits = 0;
   */
   mJoinedViaInvite = FALSE;
   mMatchMakingRunning = FALSE;                  
   mPartySession->startUpJoin(this, *temp, mLocalControllerID, *pLanInfo);

   mState = mpSessionStateStartingPartySession;
   return true;
}

//==============================================================================
// 
//==============================================================================
bool BMPSession::joinLiveGameFromInvite(BPartySessionPlayerSettings* playerSettings)
{
   //Need to get the invite data
   XINVITE_INFO* inviteData = gLiveSystem->getInviteInfo();

   if (!mPartySession)
   {
      nlog(cMPGameCL, "BMPSession::joinLiveGameFromInvite -- Failed, no party session");
      BASSERT(false);
      return false;
   }

   gLiveSystem->setPartySystemForReenter(false);
   playerSettings->mConnectionState = cBPartySessionMemberConnectionStateJoining;
   const BPartySessionPlayerSettings* temp = playerSettings;
   /*
   BPartySessionPlayerSettings hackDefaultSettings;
   hackDefaultSettings.mCiv = 1;
   hackDefaultSettings.mLeader = 1;
   hackDefaultSettings.mTeam = 0;
   hackDefaultSettings.mSlot = 0;
   hackDefaultSettings.mPartyRoomMode = 0;
   hackDefaultSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
   hackDefaultSettings.clearFlags();
   hackDefaultSettings.mPlayerRank.reset();
   hackDefaultSettings.mCampaignBits = 0;
   */
   BASSERT(mLocalChecksum);
   mJoinedViaInvite = TRUE;
   mMatchMakingRunning = FALSE;
   mBelayNextGameDisconnectEvent = false;

   mPartySession->startUpJoin(this, *temp, mLocalControllerID, inviteData);
   return true;
}

//==============================================================================
// 
//==============================================================================
bool BMPSession::hostStartupLocal( BSimString gameName, DWORD gameType, UINT iPublicSlots, UINT iPrivateSlots  )
{
   if (mState != mpSessionStateIdle)
   {
      //TODO log/return error condition?
      BASSERT(false);
      return false;
   }

   if (!isInLANMode())
   {
      return false;
   }

   //Pre-init for the game 
   if (!mSessionInterface || !mSessionInterface->mpSessionEvent_preGameInit( TRUE ))
   {
      //Immediate failure to connect
      nlog(cMPMatchmakingCL, "BMPSession::hostStartupLocal -- mpSessionEvent_preGameInit failed");
      return false;
   }

   //FIXME CO-OP - eric - add second controler ID to the session (if there is one active)

   mState = mpSessionStateStartingGameSessionHost;
   mBelayNextGameDisconnectEvent = false;

   mGameSession = new BMPGameSession(this, mGameSettings);
   if (!mGameSession->hostStartupLAN(mLocalControllerID, gameName, gameType, iPublicSlots, iPrivateSlots))
   {
      //Failed to start host
      nlog(cMPGameCL, "BMPSession::hostStartupLocal -- Failed, game session could not startup LAN host");
      //Shutdown the socket, it will delete in the update once it is shutdown
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
   }

   return true;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::hostStartupLive( DWORD gameType, BOOL ranked, UINT iPublicSlots, UINT iPrivateSlots  )
{

   if (mState != mpSessionStateIdle)
   {
      BASSERTM(false, "BMPSession::hostStartupLive - Failed, not in idle state");
      nlog(cMPGameCL, "BMPSession::hostStartupLive -- Failed, we are not in an idle state %d", mState);
      return false;
   }

   if (isInLANMode())
   {
      BASSERTM(false, "BMPSession::hostStartupLive - Failed, in LAN mode");
      nlog(cMPGameCL, "BMPSession::hostStartupLive -- Failed, we are not in LIVE mode");
      return false;
   }

   //Pre-init for the game 
   if (!mSessionInterface || !mSessionInterface->mpSessionEvent_preGameInit( TRUE ))
   {
      //Immediate failure to connect
      nlog(cMPMatchmakingCL, "BMPSession::hostStartupLive -- mpSessionEvent_preGameInit failed");
      return false;
   }

   mState = mpSessionStateStartingGameSessionHost;
   mBelayNextGameDisconnectEvent = false;

   mGameSession = new BMPGameSession(this, mGameSettings);
   return (mGameSession->hostStartupLive(mLocalControllerID,gameType, ranked, iPublicSlots, iPrivateSlots));
}

//==============================================================================
// The Party session calls this after it has confirmed that everyone has joined the request game host
//==============================================================================
void BMPSession::allPartyMembersJoinedTarget()
{
   nlog(cMPMatchmakingCL, "BMPSession::allPartyMembersJoinedTarget");
   if (isInMatchmakingMode())
   {
      //Need to check if the matchmaking is still there, timing-wise: you could cancel matchmaking at just the right second when it joined a target
      if (mLiveMatchMaking)
      {
         nlog(cMPMatchmakingCL, "BMPSession::allPartyMembersJoinedTarget - Letting matchmaker know");
         mLiveMatchMaking->allPartyMembersJoinedTarget();
         //If we are the host - refresh our QoS response
         if (mGameSession && mGameSession->isHosting())
         {
            mGameSession->updateBroadcastedHostData();
         }         
      }
      else
      {
         nlog(cMPMatchmakingCL, "BMPSession::allPartyMembersJoinedTarget - Matchmaker is gone, ignoring information");
      }
   }
   if (getSessionInterface())
   {
      nlog(cMPMatchmakingCL, "BMPSession::allPartyMembersJoinedTarget - Letting UI session interface know");
      getSessionInterface()->mpSessionEvent_partyEvent_customGameStartupComplete(cMPSessionCustomGameStartResultSuccess);
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::resumeMatchmaking()
{
   nlog(cMPMatchmakingCL, "BMPSession::resumeMatchmaking -- Resuming");
   //Need to reset everything, but to be safe lets do it NEXT frame so that this method is safe to be called from any callback or event
   //mState = mpSessionMMResetToResume;
   BASSERT(mLiveMatchMaking);
   mLiveMatchMaking->resume();
}

//==============================================================================
//
//==============================================================================
BOOL BMPSession::isInMatchmakingMode()
{
   if (!mSessionInterface)
   {
      return FALSE;
   }
   return (mSessionInterface->mpSessionEvent_queryIsInMatchmakingMode());
}

//==============================================================================
//
//==============================================================================
BOOL BMPSession::isMatchmakingRunning()
{
   if (!mPartySession)
      return FALSE;

   if (mPartySession->isHosting())
   {
      if (mLiveMatchMaking)
      {
         return mLiveMatchMaking->isRunning();
      }
      return FALSE;
   }

   //Then I'm a client
   return (mPartySession->thisClientThinksItIsMatchmaking());
}

//==============================================================================
//
//==============================================================================
uint BMPSession::getMaxMembersPerTeamForCurrentMatchMakingSearch()
{
   if (mLiveMatchMaking)
   {
      return mLiveMatchMaking->getMaxMembersPerTeamForCurrentSearch();
   }
   return 0;
}

//==============================================================================
// Called when you want to start up the party session to then allow hosting/joining
//==============================================================================
void BMPSession::setupPartySession()
{
   //ANyone calling this should know to check that there isn't already one there
   BASSERT(mPartySession==NULL);
   /*
   if (mPartySession)
   {
      delete mPartySession;
   }
   */

   BPartySessionPlayerSettings hackDefaultSettings;
   hackDefaultSettings.mCiv = 1;
   hackDefaultSettings.mLeader = 1;
   hackDefaultSettings.mTeam = 0;
   hackDefaultSettings.mSlot = 0;
   hackDefaultSettings.mPartyRoomMode = 0;
   hackDefaultSettings.clearFlags();
   hackDefaultSettings.mPlayerRank.reset();
   hackDefaultSettings.mCampaignBits = 0;
   hackDefaultSettings.mConnectionState = cBPartySessionMemberConnectionStateJoining;
   
   mPartySession = new BPartySession(mpVoice, hackDefaultSettings);
}

//==============================================================================
//
//==============================================================================
void BMPSession::partyCommand(ClientID client, uint8 commandCode, uint64 nonce)
{
   BASSERT(false);
   //Commands handled here:
   //  From clients: join success, join failure, disconnected
   //  From host: green up, leave target
}

//==============================================================================
//
//==============================================================================
void BMPSession::partyCommandJoinTarget(ClientID client, uint8 targetHopper, const XNADDR& targetXNADDR, const XNKEY& targetXNKEY, const XNKID& targetXNKID, uint64 nonce)
{
   if (isHosting())
   {
      return;
   }

   if (!mSessionInterface)
   {
      nlog(cMPGameCL, "BMPSession::partyCommandJoinTarget -- ERROR: Aborted, no session interface");
      return;
   }

   //This is a command sent from the host to the rest of the party members for them to join a target
   XNADDR localXnAddr;
   if (!gLiveSystem->getLocalXnAddr(localXnAddr) )
   {
      return;
   }

   BASSERT(mLocalChecksum);
   mJoinRequestAttempts = 0;
 
   //Join to that target
   if (isInLANMode())
   {
      //Fill out the important data in the target (even through it is a fake descriptor)  
      BLanGameInfo lanInfo;
      lanInfo.setXnAddr(targetXNADDR);
      lanInfo.setXnKey(targetXNKEY);
      lanInfo.setXnKID(targetXNKID);
      lanInfo.setNonce(nonce);

      mPort = cMPSessionLANHostingPort;

      if (!joinLanGame(lanInfo))
      {
         //Can't join that target
         nlog(cMPGameCL, "BMPSession::partyCommandJoinTarget -- Error joining that LAN target (immediately), reporting failure to party session");
         mPartySession->partySendJoinFailureCommand(nonce);
         return;
      }
   } 
   else 
   {
      //Set my hopper to match the host
      //If this is a custom game, then we need to find out how many players should be in the session
      //Otherwise it is exactly determined by the hopper
      uint playerCount = 0;
      bool ranked = false;
      if (targetHopper==CONTEXT_GAME_MODE_PARTYHOPPER)
      {
         //Custom game
         playerCount = mPartySession->getPartyCount();
         ranked = false;
      }
      else
      {
         BMatchMakingHopper* hopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(targetHopper);
         if (!hopper || (!hopper->isValid(FALSE)))
         {
            //No valid hopper match - this *should* never happen
            BASSERT(false);
            return;
         }
         playerCount = hopper->mPlayers;
         ranked = hopper->mRanked;
      }

      //Fill out the important data in the target (even through it is a fake descriptor)  
      BLiveGameDescriptor tempTarget;
      tempTarget.setXnAddr(targetXNADDR);
      tempTarget.setXNKEY(targetXNKEY);
      tempTarget.setXNKID(targetXNKID);
      tempTarget.setNonce(nonce);
      tempTarget.setSlots((uint8)playerCount);
      tempTarget.setGameModeIndex(targetHopper);
      tempTarget.setRanked(ranked);
      mPort = cMPSessionLiveHostingPort;

      if (!joinLiveGame(&tempTarget))
      {
         //Can't join that target
         nlog(cMPGameCL, "BMPSession::partyCommandJoinTarget -- Error joining that target (immediately), reporting failure to party session");
         mPartySession->partySendJoinFailureCommand(nonce);
         return;
      }
   }
   
   mState = mpSessionStateStartingGameSessionJoin;   
}

//==============================================================================
//
//==============================================================================
bool BMPSession::startupCustomHost(uint playerCount)
{
   if (isInLANMode())
   {
      return (hostStartupLocal(L"LocalGame", CONTEXT_GAME_MODE_PARTYHOPPER, playerCount,0));
   }

   return hostStartupLive(CONTEXT_GAME_MODE_PARTYHOPPER, FALSE, playerCount, 0);
}

//==============================================================================
//
//==============================================================================
void BMPSession::abortCustomHost()
{
   BASSERT(false);
}

//==============================================================================
//
//==============================================================================
bool BMPSession::joinCustomHost()
{
   //TODO playeroptions
   return false;
}

//==============================================================================
// Called by the game session to let us know the join failed
//==============================================================================
void BMPSession::gameSessionJoinFailed(BSession::BJoinReasonCode failureCode)
{
   nlog(cMPGameCL, "BMPSession::gameSessionJoinFailed");

   //If I have a party session, then i need to let that system know the game session join failed
   if (mPartySession)
   {
      uint64 nonce = 0;
      if (mGameSession)
      {
         nonce = mGameSession->getNonce();
      }
      else
      {
         nlog(cMPGameCL, "cMPGameCL::gameSessionJoinFailed -- Warning no game session, could not report my nonce");
      }
      mPartySession->partySendJoinFailureCommand(nonce);
   }
   else if (getSessionInterface())
   {
      //DEPRICATED (i think) eric
      BASSERTM(false, "BMPSession::gameSessionJoinFailed - there is no party, but there is an interface - let me know if this is every seen - eric!");
      getSessionInterface()->mpSessionEvent_gameSessionJoinFailed( failureCode );
   } 

   //Clean up that failure
   if (mGameSession)
   {
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
   }
}

//==============================================================================
// Called by the game session to let us know the hosting failed
//==============================================================================
void BMPSession::gameSessionHostFailed(cMPSessionGameSessionHostResultCode failureCode)
{
   nlog(cMPGameCL, "BMPSession::gameSessionHostFailed - error code %d", failureCode);
   
   //If a party session requested this - let them know if failed
   if (mPartySession)
   {
      uint64 nonce = 0;
      if (mGameSession)
      {
         nonce = mGameSession->getNonce();
      }
      else
      {
         nlog(cMPGameCL, "cMPGameCL::gameSessionHostFailed -- Warning no game session, could not report my nonce");
      }
      mPartySession->partySendJoinFailureCommand(nonce);
   }
   //NOTE: No handling of LAN host failure here because it succeeds/fails on host call - no callbacks

   //TODO - move this to the party session so that it can decide when to let the mp ui interface know about this (or not if in MP)    
   if (getSessionInterface())
   {
      getSessionInterface()->mpSessionEvent_partyEvent_customGameStartupComplete(cMPSessionCustomGameStartResultHostingError);
   }
   
   //Cleanup that game session so it can be deleted
   if (mGameSession)
   {
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
   }
}

//==============================================================================
// Called by the game session to let us know the join succeeded
//==============================================================================
void BMPSession::gameSessionConnected()
{
   BASSERT(mGameSession);
   BDEBUG_ASSERTM(mState != mpSessionStateGameSessionRunning, "Game state already running?");

   nlog(cMPGameCL, "BMPSession::gameSessionConnected");

   //Add code here to check if there are 2 players on the connection, and if so don't
   //  do this logic until both have reported in as good to go
   //OR - will need to change the partycommand protocol to pass in a XUID for who is doing the reporting
   //TODO  
   mState = mpSessionStateGameSessionRunning;

   if (mPartySession)
   {
      //Send notice that I've joined successfully
      mPartySession->partySendJoinSuccessCommand(mGameSession->getNonce());
      if (mPartySession->isHosting())
      {
         //If I'm the party host - tell all my minions to join that target
//-- FIXING PREFIX BUG ID 1432
         //BMatchMakingHopper* pHopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(mGameSession->getGameTypeIndex());
         //No need to pre-check this here anymore, for LAN there is the possiblity it could fail, but LAN doesn't use the value anyways
         // So this is fine
//--
         //BASSERT(pHopper);
         mPartySession->partySendJoinTargetCommand((uint8)mGameSession->getGameTypeIndex(), mGameSession->getHostXnAddr(), mGameSession->getXNKEY(), mGameSession->getXNKID(), mGameSession->getNonce());
      }
   }

   if (getSessionInterface())
   {
      //Let the session interface know
      getSessionInterface()->mpSessionEvent_gameConnected();
   }
}

//==============================================================================
// Called by BMPGameSession when ALL players are connected and the game ready to start
//==============================================================================
void BMPSession::gameSessionReady()
{
   //None of this used any more
   /*

   if (mState == mpSessionStateGameSessionRunning)
      return;

   BASSERT(mGameSession);

   nlog(cMPGameCL, "BMPSession::gameSessionReady");

   mState = mpSessionStateGameSessionRunning;

   if (mPartySession)
   {
      mPartySession->partySendJoinSuccessCommand(mGameSession->getNonce());
   }

   if (getSessionInterface())
   {
      //Let the session interface know
      getSessionInterface()->mpSessionEvent_gameConnected();
   }
   */
}

//==============================================================================
//
//==============================================================================
void BMPSession::sendBelayedGameDisconnectEvent()
{
   nlog(cMPGameCL, "BMPSession::sendBelayedGameDisconnectEvent - Telling party session we have left that target nonce[%I64u]", mBelayedNonceForGameDisconnectEvent);
   if (mPartySession)
      mPartySession->partySendDisconnectedFromTargetCommand(mBelayedNonceForGameDisconnectEvent);  
}

//==============================================================================
// Called by the game session to let us know the session disconnected (can happen for a host or a joiner)
//==============================================================================
void BMPSession::gameSessionDisconnected(BSession::BDisconnectReason reasonCode)
{
  
   cMPSessionDisconnectReasonCode mpreason = cMPSessionDisconnectReasonFailedConnection;

   switch (reasonCode)
   {
      case BSession::cTransportLost:
      case BSession::cSessionTerminated:
      case BSession::cHostDecision:
      case BSession::cHostCancelledGame:
         mpreason = cMPSessionDisconnectReasonGameTerminated;
         break;

      case BSession::cConnectionRejected:
      case BSession::cFailedClientConnect:
         mpreason = cMPSessionDisconnectReasonFailedConnection;
         break;

      case BSession::cBuildMismatch:
         mpreason = cMPSessionDisconnectReasonCRCMismatch;
         break;

      case BSession::cSessionClosed:
      case BSession::cSessionFull:
         mpreason = cMPSessionDisconnectReasonFull;
         break;

      case BSession::cGameDeleted:
         mpreason = cMPSessionDisconnectReasonDeleted;
         break;

      default:
      case BSession::cNormal:
         mpreason = cMPSessionDisconnectReasonNormal;
         break;
   }

   nlog(cMPGameCL, "BMPSession::gameSessionDisconnected reasonCode[%d], mpreason[%d]", reasonCode, mpreason);

   if (mPartySession)
   {
      UINT64 nonce = 0;
      if (mGameSession)
      {
         nonce = mGameSession->getNonce();
      }
      else
      {
         nlog(cMPGameCL, "BMPSession::gameSessionDisconnected - game session invalid, no nonce to pass to the party session on this disconnect");
      }
      if (mBelayNextGameDisconnectEvent)
      {
         nlog(cMPGameCL, "BMPSession::gameSessionDisconnected - I am NOT sending a partySendDisconnectedFromTargetCommand because mBelayNextGameDisconnectEvent is set");
         //mBelayNextGameDisconnectEvent = false;
         mBelayedNonceForGameDisconnectEvent = nonce;
      }
      else
      {
         nlog(cMPGameCL, "BMPSession::gameSessionDisconnected - Telling party session we have left that target nonce[%I64u], reasonCode[%d]", nonce, mpreason);
         mPartySession->partySendDisconnectedFromTargetCommand(nonce);
      }
   }
   if (getSessionInterface())
   {
      getSessionInterface()->mpSessionEvent_gameDisconnected( mpreason );
   }

   //If we have the game session - then get rid of it and set ourselves to wait until it is clear
   if (mGameSession)
   {
      mGameSession->shutDown();
      //Catch-all here, if we got here directly because the game session is gone, but not end of game - then put us to return to idle
      //  But if someone has already set a shutdown/reset state - don't overwrite that.
      //TODO - Revisit this once we have the party session UI rejoining after a game finishes/disconnects
      if (mState < mpSessionStateGameSessionShuttingDown)
      {
         mState = mpSessionStateGameSessionShuttingDown;
      }
   }

   //This logic happens in the update loop - lets not do it in two places
   /*
   else
   {
      //We are in a state that is ok to continue
      if (mState < mpSessionStateShuttingDownToDelete)
      {
         mState = mpSessionStateIdle;
      }
   }
   */
}

//==============================================================================
// Called by the game session to let us know the game launch was aborted
//==============================================================================
void BMPSession::gameSessionLaunchAborted()
{
   nlog(cMPGameCL, "BMPSession::gameSessionLaunchAborted");
   abortGameSession();
   /*
   if (mGameSession)
   {
      //Delete the game session - it will re-created/re-joined when they startup the launch again
      //TODO - note, if they SPAM launch/unlaunch in the same frame (before the game session can clean-up, then it could cause issues)
      mGameSession->shutDown();
      nlog(cMPGameCL, "BMPSession::gameSessionLaunchAborted - Shutting down the game session");
   }
   */
}

//==============================================================================
// Called by the game session to let us know the game has launched
//==============================================================================
void BMPSession::gameSessionGameLaunched()
{
   nlog(cMPGameCL, "BMPSession::gameSessionGameLaunched");
   mBelayNextGameDisconnectEvent = false;
   if (mLiveMatchMaking)
   {
      mLiveMatchMaking->matchMadeGameLaunched();
      delete mLiveMatchMaking;
      mLiveMatchMaking = NULL;
   }
}

//==============================================================================
//
//==============================================================================
bool BMPSession::startPartySessionHost(BPartySessionPlayerSettings* playerSettings)
{
   if (!mPartySession)
   {
      //Party session should have been created before it got here
      return false;
   }

   //Start one up
   nlog(cMPGameCL, "BMPSession::startPartySessionHost - Starting hosting of a new party session");

   /*
   BPartySessionPlayerSettings hackDefaultSettings;
   hackDefaultSettings.mCiv = 1;
   hackDefaultSettings.mLeader = 1;
   hackDefaultSettings.mTeam = 0;
   hackDefaultSettings.mSlot = 0;
   hackDefaultSettings.mPartyRoomMode = 0;
   hackDefaultSettings.clearFlags();
   hackDefaultSettings.mPlayerRank.reset();
   hackDefaultSettings.mCampaignBits = 0;
   */
   BASSERT(mLocalChecksum);
   mJoinedViaInvite = FALSE;
   mBelayNextGameDisconnectEvent = false;
   const BPartySessionPlayerSettings* temp = playerSettings;
   mPartySession->startUpHost(this, *temp, mPartySessionInitialHostSettings, mLocalControllerID);
   mState = mpSessionStateStartingPartySession;
   return true;
}

//==============================================================================
//
//==============================================================================
BOOL BMPSession::startMatchMaking(BMPSessionMatchMakingData* matchMakingData)
{
   if (mState!=mpSessionStateIdle)
   {
      nlog(cMPMatchmakingCL, "BMPSession::startMatchMaking -- Request to start matchmaking - but I'm not in idle state. %d", mState);
      BASSERT(false);
      return FALSE;
   }

   if (!mSessionInterface)
   {
      nlog(cMPMatchmakingCL, "BMPSession::startMatchMaking -- ERROR: Aborted, no session interface");
      BASSERT(false);
      return FALSE;
   }

   if (mMatchMakingRunning==TRUE)
   {
      nlog(cMPMatchmakingCL, "BMPSession::startMatchMaking -- ERROR: Aborted, matchmaking is already running");
      BASSERT(false);
      return FALSE;
   }

   if (!mPartySession)
   {
      nlog(cMPMatchmakingCL, "BMPSession::startMatchMaking -- ERROR: Aborted, no party");
      BASSERT(false);
      return FALSE;
   }

   mMatchMakingRunning = TRUE;
   //Drop the old instance if there is one
   if (mLiveMatchMaking)
   {
      //BASSERT(false);      //I'd like to know if this ever gets hit - eric
      delete mLiveMatchMaking;
      mLiveMatchMaking = NULL;
   }
   //Start one up
   mLiveMatchMaking = new BLiveMatchMaking(this);
   //NOTE: This lockparty should be called by everyone, not just the host.  It works out ok since our matchmaking is 'full members' to start so the
   //   joinable=FALSE just because the client count is full.
   mPartySession->lockPartyMembers(true);

   BOOL result =(mLiveMatchMaking->start(matchMakingData));
   
   if (!result)
   {
      mPartySession->lockPartyMembers(false);
   }

   return (result);
}

//==============================================================================
//
//==============================================================================
BOOL BMPSession::abortMatchMaking()
{
   nlog(cMPMatchmakingCL, "BMPSession::abortMatchMaking -- Aborting matchmaking");
   //Change here - refuse to abort matchmaking if a game has already joined and is in the countdown to launching
   if (mGameSession && mGameSession->isGameRunning())
   {
      //Too late - cannot abort
      nlog(cMPMatchmakingCL, "BMPSession::abortMatchMaking -- Abort ignored, game is launching or running");
      return FALSE;
   }
   if (mLiveMatchMaking)
   {
      nlog(cMPMatchmakingCL, "BMPSession::abortMatchMaking -- Live matchmaking object found, telling it to abort()");
      mLiveMatchMaking->abort();
      delete mLiveMatchMaking;
      mLiveMatchMaking = NULL;
   }
   if (mGameSession)
   {
      nlog(cMPMatchmakingCL, "BMPSession::abortMatchMaking -- Game session object found, telling it to shutDown()");
      mGameSession->shutDown();
      mState = mpSessionStateGameSessionShuttingDown;
   }
   return TRUE;
}

//==============================================================================
//
//==============================================================================
void BMPSession::matchMakingFailed(BLiveMatchMaking::BLiveMatchMakingErrorCode reason)
{
   nlog(cMPMatchmakingCL, "BMPSession::matchMakingFailed -- live matchmaking is reporting that it has failed");

   mMatchMakingRunning = FALSE;

   if (!mSessionInterface)
   {
      nlog(cMPMatchmakingCL, "BMPSession::matchMakingFailed -- Note: No session interface to notify of this event");
      return;
   }

   if (reason==BLiveMatchMaking::cBLiveMatchMakingErrorAbortRequested)
   {
      mSessionInterface->mpSessionEvent_ESOSearchFailed(cMPSessionESOSearchCodePlayerCanceled);
   }
   else if (reason==BLiveMatchMaking::cBLiveMatchMakingErrorTimedOut)
   {
      mSessionInterface->mpSessionEvent_ESOSearchFailed(cMPSessionESOSearchCodeTimedOut);
   }
   else if (reason==BLiveMatchMaking::cBLiveMatchMakingErrorMissingSkillData)
   {
      mSessionInterface->mpSessionEvent_ESOSearchFailed(cMPSessionESOSearchLiveProblem); 
   }
   else
   {
      mSessionInterface->mpSessionEvent_ESOSearchFailed(cMPSessionESOSearchHopperError);   
   }
}

//==============================================================================
// Requests a login from ESO
//==============================================================================
BOOL BMPSession::remoteRequestESOLogin()
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser == NULL)
      return FALSE;

   gLSPManager.isBanned(pUser->getXuid());

   return TRUE;
}

//==============================================================================
// Requests the config file 
// TODO-LSP, may need fix simple true/false checks
//==============================================================================
BOOL BMPSession::remoteRequestESOConfigFile()
{
   //Starts the async request for the config data
   gLSPManager.requestConfigData(gLiveSystem->getHopperList());
   return TRUE;
}

//==============================================================================
// Apply a hard coded config file for when the LSP config cannot be acquired
//==============================================================================
void BMPSession::applyNoLSPConfigSettings()
{
   return;

   /*
   BOOL enabled = (gConfig.isDefined("MatchMakingEnableAllHoppers") ? TRUE : FALSE);

   nlog(cMPGameCL, "BMPSession::applyFixedConfigFile - Applying the hardcoded config data");
   //Enable the basic matchmaking hoppers which are marked as NOLSPMode 
   for (int i=0;i<gLiveSystem->getHopperList()->getHopperCount();i++)
   {
      BMatchMakingHopper* h = gLiveSystem->getHopperList()->findHopperByID(i);
      if (h)
      {
         h->mEnabled = h->mNOLSPModeEnabled & !enabled;
      }
   }

   //The first hopper in the list is ALWAYS the custom hopper - it should ALWAYS be enabled
   BMatchMakingHopper* h = gLiveSystem->getHopperList()->findHopperByID(0);
   if(h)
   {
      h->mEnabled = TRUE;
   }
   */
}

//==============================================================================
// 
//==============================================================================
void BMPSession::initLanDiscovery()
{
   if (mpLanDiscovery)
      return;

   if (!isInLANMode())
      return;

   mpLanDiscovery = HEAP_NEW(BXLanDiscovery, gNetworkHeap);

#ifdef TITLEID_HALO_WARS_ALPHA
   uint titleID = TITLEID_HALO_WARS_ALPHA;
#else
   uint titleID = TITLEID_HALO_WARS;
#endif

   mpLanDiscovery->init(titleID, mLocalChecksum);

   if (mPartySession)
      mPartySession->updateBroadcastedHostData();
}

//==============================================================================
// 
//==============================================================================
void BMPSession::deinitLanDiscovery()
{
   if (mpLanDiscovery)
   {
      mpLanDiscovery->deinit();
      HEAP_DELETE(mpLanDiscovery, gNetworkHeap);
      mpLanDiscovery = NULL;
   }
}

//==============================================================================
// Sends a game invite to everyone on this player's friends list who is ONLINE - DEBUG/TESTING ONLY (NOT Asyc!)
//==============================================================================
void BMPSession::bulkInvite()
{
   //First get their friends list
   HANDLE enumerateHandle;
   DWORD  bufferSize;
   DWORD result = XFriendsCreateEnumerator(mLocalControllerID, 0, MAX_FRIENDS, &bufferSize, &enumerateHandle);
   if (result != ERROR_SUCCESS )
   {
      //list was empty or some other issue
      return;
   }
   
   DWORD itemCount = bufferSize / sizeof(XONLINE_FRIEND);
   BYTE* friendMemory = new BYTE[bufferSize];

   result = XEnumerate(enumerateHandle, friendMemory, bufferSize, &itemCount, NULL);
   if( result != ERROR_SUCCESS )
   {
      delete[] friendMemory;
      CloseHandle( enumerateHandle );
      return;
   }
   
   //Now walk through it finding peeps who are online
   for (uint i=0;i<itemCount;i++)
   {      
//-- FIXING PREFIX BUG ID 1434
      const XONLINE_FRIEND* pFriend = reinterpret_cast<XONLINE_FRIEND*>(friendMemory+(i*sizeof(XONLINE_FRIEND)));
//--
      if ((pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE) &&
          !(pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING))
      {         
         //Send them an invite
         const XUID pXuid = pFriend->xuid;
         result = XInviteSend(mLocalControllerID,1,&pXuid,L"Bulk-sent invite",NULL);
      }
   }

   delete[] friendMemory;
   CloseHandle( enumerateHandle );
}

//==============================================================================
//
//==============================================================================
void BMPSession::setInitialPartySettings(const BPartySessionHostSettings& startupHostSettings)
{
   mPartySessionInitialHostSettings = startupHostSettings;
}

//==============================================================================
//
//==============================================================================
uint BMPSession::getLanGameCount()
{
   if (mpLanDiscovery == NULL)
      return 0;

   return mpLanDiscovery->getList().getSize();
}

//==============================================================================
//
//==============================================================================
BLanGameInfo* BMPSession::getLanGame(uint index)
{
   if (mpLanDiscovery == NULL)
      return NULL;

   if (mpLanDiscovery->getList().getSize() <= index)
      return NULL;

   return &(mpLanDiscovery->getList()[index]);
}

//==============================================================================
// 
//==============================================================================
BDynamicSimArray<BLanGameInfo>* BMPSession::getLanGamesList()
{
   if (mpLanDiscovery == NULL)
      return NULL;

   return &(mpLanDiscovery->getList());
}

//==============================================================================
//
//==============================================================================
BLiveSession* BMPSession::getLiveSession( void ) 
{ 
   if (!mGameSession)
   {
      return (NULL);
   }
   
   return (mGameSession->getLiveSession()); 
};

//==============================================================================
//
//==============================================================================
long BMPSession::getControlledPlayer(void)
{ 
   if (!isGameRunning())
      return -1;

   return(mLocalPlayerID);
}

//==============================================================================
//
//==============================================================================
void BMPSession::setControlledPlayer(long p) 
{ 
   mLocalPlayerID = p;
}

//==============================================================================
//
//==============================================================================
DWORD BMPSession::getGlobalUserCount()
{
   if (!gLiveSystem->getHopperList())
      return 0;

   if (!gLiveSystem->getHopperList()->isLoaded())
      return 0;

   return (gLiveSystem->getHopperList()->getDynamicData()->mGlobalUserCount);
}

//==============================================================================
//
//==============================================================================
bool BMPSession::isClient(PlayerID id)
{
   //if (!isRunning())
   //   return false;
   /*
   if (!mPlayerManagerInterface)
      return false;
      */
   if (!mGameSession)
      return false;
   return mGameSession->isClient(id);
}

//==============================================================================
//
//==============================================================================
BOOL BMPSession::hasValidGameSession() const
{
   if (mGameSession && mGameSession->isRunning()) 
      return TRUE; 
   return FALSE;
}

//==============================================================================
//
//==============================================================================
BSession* BMPSession::getSession(void) const 
{
   if (!mGameSession)
   {
      return NULL;
   }

   return(mGameSession->getSession());
} 
//==============================================================================
//
//==============================================================================
long BMPSession::getMaxPlayers() const
{ 
   if (mGameSession) 
      return mGameSession->getMaxPlayers(); 
   return 0;
}

//==============================================================================
//
//==============================================================================
uint BMPSession::getGameSessionPlayerCount(void)
{
   if (mGameSession) 
      return mGameSession->getPlayerCount(); 
   return 0;
}

//==============================================================================
//
//==============================================================================
uint64 BMPSession::getGameSessionNonce(void)
{
   if (mGameSession) 
      return mGameSession->getNonce(); 
   return 0;
}

//==============================================================================
//Call this to alter the public slot count
//==============================================================================
void BMPSession::setMaxPlayerCount(uint32 maxPlayers)
{
   if (mGameSession)
   {
      mGameSession->setMaxPlayerCount(maxPlayers);
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::clearSessionInterface()
{
   nlog(cMPGameCL, "BMPSession::clearSessionInterface");
   mSessionInterface = NULL;
}

//==============================================================================
//
//==============================================================================
void BMPSession::setSessionInterface(mpSessionInterface* pInterface)
{
   nlog(cMPGameCL, "BMPSession::setSessionInterface");
   mSessionInterface = pInterface;
}

//==============================================================================
//
//==============================================================================
BMPSimDataObject* BMPSession::getSimObject() const
{ 
   if (mGameSession)
   {
      return (mGameSession->getSimObject());
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
DWORD BMPSession::advanceGameTiming(void)
{
   if (!isGameRunning())
      return 0;

   return (mGameSession->advanceGameTiming());
}

//==============================================================================
//
//==============================================================================
void BMPSession::setGameStateToFinal(void)
{
   mState = mpSessionStateFinal;
}

//==============================================================================
//
//==============================================================================
void BMPSession::endActiveGame(void)
{
   if (mGameInterface)
   {
      mGameInterface->setupSync(NULL);
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_systemReady()
{
   nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemReady");
   if (gLiveSystem && !gLiveSystem->isPartySystemFlaggedForReenter())
   {
      //Skip this assert if we are re-entering the party session after a game
      BASSERT(mState==mpSessionStateStartingPartySession);
      if (mState!=mpSessionStateStartingPartySession)
         nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemReady - We are not in state StartingParty [%d]", mState);
   }
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_systemReady();
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemReady - Error, no session interface");
   }
   mState = mpSessionStateIdle;
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_systemDisconnected()
{
   gGamerPicManager.reset(false);
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyDisconnected(cMPSessionDisconnectReasonGameTerminated);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemDisconnected - Error, no session interface");
   }
   if (!mGameSession)
   {
      //The party session has disconnected - if we are not in a game then we should reset ourselves
      //  If we are in a game then this would happen after the game completes
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemDisconnected - No gameSession, reseting mpSession next update");
      gLiveSystem->setPartySystemForReenter(false);
      mState=mpSessionStateGameSessionResetNextUpdate;
      //mState = mpSessionStateError;
   }
   else if (!mSessionInterface)
   {
      //The case here is that our party has died while we are still in game
      //  So the mSessionInterface won't handle the clean-up - so we must
      //  However, we don't want to cleanup anything BUT just the party
      mResetPartySessionNextUpdate=true;
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_hostRunning()
{
   //TODO - delete this - no longer supported
   BASSERT(false);
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_settingsChanged()
{
   if (mSessionInterface)
   {
      //Just pass it up the chain
      mSessionInterface->mpSessionEvent_systemReady();
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_settingsChanged - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_systemStartupFailed(BSession::BJoinReasonCode failureCode)
{
   BASSERT(mState==mpSessionStateStartingPartySession);

   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_joinFailed(failureCode);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_systemStartupFailed - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_playerJoined(XUID xuid) //ClientID newClientID, uint newMemberIndex)
{
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_playerJoined(xuid); //newClientID, newMemberIndex);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_playerJoined - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_playerLeft(XUID xuid) //ClientID newClientID, uint newMemberIndex)
{
   gGamerPicManager.removeGamerPic(xuid);

   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_playerLeft(xuid); //newClientID, newMemberIndex);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_playerLeft - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_memberSettingsChanged(BPartySessionPartyMember* changedMember)
{
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_memberSettingsChanged(changedMember);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_memberSettingsChanged - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_partySizeChanged(DWORD newMaxPartyMemberCount)
{
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_partySizeChanged(newMaxPartyMemberCount);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_partySizeChanged - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_hostSettingsChanged(BPartySessionHostSettings* pNewSettings)
{
   BDEBUG_ASSERT(pNewSettings);

   // update the player count
   //BAD! We never adjust the GAME session's setting from changes in the PARTY session
   //setMaxPlayerCount(pNewSettings->mNumPlayers);

   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_partyEvent_hostSettingsChanged(pNewSettings);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_hostSettingsChanged - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
void BMPSession::BPartySessionEvent_matchMakingStatusChanged(uint8 status, uint data1, uint data2)
{
   if (mSessionInterface)
   {
      mSessionInterface->mpSessionEvent_MatchMakingStatusChanged((BLiveMatchMaking::BLiveMatchMakingStatusCode)status, data1, data2);
   }
   else
   {
      nlog(cMPGameCL, "BMPSession::BPartySessionEvent_matchMakingStatusChanged - Error, no session interface");
   }
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setSetting(DWORD index, void *data, long size)
{
   //Now - just pass these down to the game session layer
   BASSERT(mGameSession);
   return mGameSession->setSetting(index, data, size);
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setInt64(DWORD index, int64 data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setUInt64(DWORD index, uint64 data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setLong(DWORD index, long data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setDWORD(DWORD index, DWORD data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setFloat(DWORD index, float data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setShort(DWORD index, short data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setWORD(DWORD index, WORD  data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setBool(DWORD index, bool  data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setChar(DWORD index, char  data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setBYTE(DWORD index, BYTE  data)
{
   return(setSetting(index, &data, sizeof(data)));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::setString(DWORD index, const BSimString& data)
{
   BCHAR_T* pData = const_cast<BCHAR_T*>(data.getPtr());
   long size  = (data.length()+1)*sizeof(BCHAR_T);
   return(setSetting(index, pData, size));
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getInt64(DWORD index, int64 &data) 
{
   if (mGameSettings)
      return(mGameSettings->getInt64(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getUInt64(DWORD index, uint64 &data) 
{
   if (mGameSettings)
      return(mGameSettings->getUInt64(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getLong(DWORD index, long  &data) 
{
   if (mGameSettings)
      return(mGameSettings->getLong(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getDWORD(DWORD index, DWORD &data)
{
   if (mGameSettings)
      return(mGameSettings->getDWORD(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getFloat(DWORD index, float &data)
{
   if (mGameSettings)
      return(mGameSettings->getFloat(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getShort(DWORD index, short &data)
{
   if (mGameSettings)
      return(mGameSettings->getShort(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getWORD(DWORD index, WORD  &data)
{
   if (mGameSettings)
      return(mGameSettings->getWORD(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getBool(DWORD index, bool  &data)
{
   if (mGameSettings)
      return(mGameSettings->getBool(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getChar(DWORD index, char  &data)
{
   if (mGameSettings)
      return(mGameSettings->getChar(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getBYTE(DWORD index, BYTE  &data)
{
   if (mGameSettings)
      return(mGameSettings->getBYTE(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getString(DWORD index, BSimString &data)
{
   if (mGameSettings)
      return(mGameSettings->getString(index, data));
   else 
      return false;
}

//==============================================================================
//
//==============================================================================
bool BMPSession::getAreAllPlayersLoaded() const 
{
   if (mGameSession)
      return mGameSession->getAreAllPlayersLoaded();

   return true;
}
