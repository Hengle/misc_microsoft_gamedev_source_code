//==============================================================================
// liveSystem.cpp
//
// Copyright (c) Ensemble Studios, 2006-2007
//==============================================================================

// Includes

#include "Common.h"
#include "liveSystem.h"
#include "mpSession.h"
#include "mpGameSession.h"
#include "liveSession.h"
#include "liveVoice.h"
#include "SocksHelper.h"
#include "notification.h"
#include "econfigenum.h"
#include "mpcommheaders.h"
#include "commlog.h"
#include "configsgame.h"
#include "scoremanager.h"
#include "world.h"
#include "game.h"

#include "user.h"
#include "usermanager.h"
#include "gamedirectories.h"
#include "lspManager.h"

#include "../xinputsystem/inputsystem.h"
#include "../xgame/modemanager.h"

// Globals
BLiveSystem* gLiveSystem = NULL;

//Constants
const DWORD cBLiveSystemPresenceRefreshTime = 100;        //Check for presence updates every X ms
const DWORD cBLiveSystemPresenceScoreRefreshTime = 2500;  //Update the current score for presence every X ms
const DWORD cBLiveSystemLeaderBoardOnlySessionTimeout = 10000; 


//==============================================================================
// 
//==============================================================================
BLiveSystem::BLiveSystem() :
   //mStateLiveRequired(FALSE),
   //mInitialNotificationReceived(FALSE),
   //mNumSignedInUsers(0),
   mpMPSession(NULL),
   mpVoice(NULL),
   //mConnectionState(cLiveConnectionStateNotStarted),
   mLastPresenceUpdate(0),
   mLastPresenceChange(0),
   mPresenceMode(CONTEXT_PRESENCE_PRE_MODE_MAINMENU),
   mXnAddrState(cXnAddrStateInit),
   mShuttingDownMPSession(false),
   mLeaderBoardError(ERROR_SUCCESS),
   mLeaderBoardState(cLeaderBoardStateIdle),
   mLeaderBoardIndex(0),
   mLeaderBoardRequestingUser(0),
   mLeaderBoardIndexToUser(false),
   mLeaderBoardJustShowFriends(false),
   mLeaderBoardQueryTime(0),
   mpLeaderBoardReturnDataRows(NULL),
   mpLeaderBoardStatsResults(NULL),
   mLeaderBoardEnumerateResults(0),
   mLeaderBoardEnumerator(INVALID_HANDLE_VALUE),
   mpXUIDRequestList(NULL),
   mpFriendListMemory(NULL),
   mFriendCount(0),
   mpLeaderboardOnlySession(NULL),
   mPartySystemReenterFlag(false),
   mpHopperList(NULL),
   mpHopperListUpdatedVersion(NULL),
   mPresenceIsOperationPending(false),
   mPresenceValue(0),
   mPresenceScoreValue(0),
   mCurrentPresenceIndex(0),
   mLeaderBoardCreationTime(0),
   mLastPresenceScoreUpdate(0),
   mInviteAcceptersPort(XUSER_MAX_COUNT)
{
   Utils::FastMemSet(&mInviteInfo,0,sizeof(XINVITE_INFO));

   //This updates the 'sleep' presence mode so that it doesn't happen right off
   mLastPresenceChange = timeGetTime();

   for (uint i=0;i<cLiveSystemTestMaxItems;i++)
      mTestOptions[i] = FALSE;

   Utils::FastMemSet(&mLeaderBoardOverlap, 0, sizeof(XOVERLAPPED));
   Utils::FastMemSet(&mPresenceOverlap, 0, sizeof(XOVERLAPPED));
   #ifndef BUILD_FINAL
      mpTestingLBData=NULL;
      mpTestingLBCount=NULL;
      mpTestingLBTotalRows=NULL;
   #endif

   mpHopperList = new BMatchMakingHopperList();
   mpHopperListUpdatedVersion = new BMatchMakingHopperList();
}

//==============================================================================
// 
//==============================================================================
BLiveSystem::~BLiveSystem()
{
   if (mpMPSession)
   {
      //delete mpMPSession;
      HEAP_DELETE(mpMPSession, gNetworkHeap);
      mpMPSession = NULL;
   }

   mXnAddrState = cXnAddrStateInit;
   //mConnectionState = cLiveConnectionStateNotStarted;
   shutdown();

   if (mpVoice)
   {
      mpVoice->release();
      //HEAP_DELETE(mpVoice, gNetworkHeap);
      mpVoice = NULL;
   }

   Utils::FastMemSet(&mInviteInfo,0,sizeof(XINVITE_INFO));
   mInviteAcceptersPort=XUSER_MAX_COUNT;

#ifndef BUILD_FINAL
   if (mpTestingLBData)
   {
      delete mpTestingLBData;
      mpTestingLBData = NULL;
   }
   if (mpTestingLBCount)
   {
      delete mpTestingLBCount;
      mpTestingLBCount = NULL;
   } 
   if (mpTestingLBTotalRows)
   {
      delete mpTestingLBTotalRows;
      mpTestingLBTotalRows = NULL;
   }


#endif

   if (mpLeaderboardOnlySession)
   {
      delete mpLeaderboardOnlySession;
      mpLeaderboardOnlySession = NULL;
   }

   leaderBoardCancelQuery();

   if (mpXUIDRequestList)
   {
      delete[] mpXUIDRequestList;
      mpXUIDRequestList = NULL;
   }

   if (mpFriendListMemory)
   {
      delete[] mpFriendListMemory;
      mpFriendListMemory = NULL;
   }

   if (mpLeaderBoardStatsResults)
   {
      delete mpLeaderBoardStatsResults;
      mpLeaderBoardStatsResults = NULL;
   }

   delete mpHopperList;
   delete mpHopperListUpdatedVersion;

   gLiveSystem = NULL;

   // we're really done, close the log file
   BCommLog::closeLogFile();
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::createInstance()
{
   //Don't create twice
   if (gLiveSystem != NULL)
      return FALSE;

   //Create the singleton
   //gLiveSystem = new BLiveSystem();
   gLiveSystem = HEAP_NEW(BLiveSystem, gNetworkHeap);
   BFATAL_ASSERT(gLiveSystem);
   if (!gLiveSystem)
      return FALSE;

   return TRUE;
}

//==============================================================================
// 
//==============================================================================
BLiveSystem* BLiveSystem::getInstance()
{
   BFATAL_ASSERT(gLiveSystem);
   return gLiveSystem;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::destroyInstance()
{
   if (gLiveSystem)
   {
      //delete gLiveSystem;
      HEAP_DELETE(gLiveSystem, gNetworkHeap);
      gLiveSystem = NULL;
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::startup()
{
   // Start up Xbox Live functionality using default Secure Network Layer settings
   //mConnectionState = cLiveConnectionStateStartupWaiting;
   mXnAddrState = cXnAddrStateInit;

   // Start up the network layer socket system (this adds a ref count to it as well for us)
   if (BSocksHelper::socksStartup() != S_OK)
   {
      BFAIL("BLiveSystem::startUp -- Failed to startup socks layer.");
      return;
   }

   loadMatchMakingHoppers();
   //updateSignedInUserCount();

   if (mpVoice)
      mpVoice->init();

   //This is currently being done in system\netutil::networkStartup - triggered by the above socksStartup call
   // if( XOnlineStartup() != ERROR_SUCCESS )
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::shutdown()
{
   if (mpVoice)
      mpVoice->shutdown();

   BSocksHelper::socksCleanup();
}

//==============================================================================
// 
//==============================================================================
bool BLiveSystem::inviteReceived(DWORD controllerID)
{
   //A user has accepted an invite (or launched a join via pres)
   DWORD ret = XInviteGetAcceptedInfo(controllerID, &mInviteInfo);
   if (ret != ERROR_SUCCESS)
   {
      //Checking the invite structure failed - invite has probably gone away
      Utils::FastMemSet(&mInviteInfo, 0, sizeof(XINVITE_INFO));
      return false;
   }
   //Check that the invite is for Halo Wars - if not, we can ignore it
   if (mInviteInfo.dwTitleID!=TITLEID_HALO_WARS)
   {
      //Not us!
      Utils::FastMemSet(&mInviteInfo, 0, sizeof(XINVITE_INFO));
      return false;
   }
   mInviteAcceptersPort=controllerID;
   return true;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::setPartySystemForReenter(bool flagReenter)
{
   //If the config is not defined - then never set this bit (and thus none of the new logic should get processed)
   if (!gConfig.isDefined(cConfigRejoinParty))
   {
      return;
   }

   //Don't let this get changed to true if we have accepted an invite 
   if (!flagReenter || !isInviteInfoAvailable())
   {
      mPartySystemReenterFlag=flagReenter;
   }
}

//==============================================================================
// 
//==============================================================================
bool BLiveSystem::isInNoLSPMode()
{
   return gLSPManager.isInNoLSPMode();
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::update()
{
   SCOPEDSAMPLE(BLiveSystem_update)

   //Give the socket layer a chance to update any active socket connections
   //BSocksHelper::serviceSockets();

   //Leaderboard system hook
   leaderBoardUpdate();

   if (mXnAddrState != cXnAddrStateReady)
   {
      switch (mXnAddrState)
      {
         case cXnAddrStateError:
            {
               break;
            }
         case cXnAddrStateInit:
         case cXnAddrStatePending:
            {
               DWORD dwRet = XNetGetTitleXnAddr(&mLocalXnAddr);
               if (dwRet & XNET_GET_XNADDR_PENDING)
               {
               }
               else if (dwRet & XNET_GET_XNADDR_NONE)
               {
                  mXnAddrState = cXnAddrStateError;
               }
               else if (dwRet & XNET_GET_XNADDR_TROUBLESHOOT)
               {
                  mXnAddrState = cXnAddrStateError;
               }
               else if (mLocalXnAddr.ina.s_addr == 0)
               {
                  //We should always have a local IP - if not, just keep spinning to give it a chance to acquire                  
               }
               else
               {
                  mXnAddrState = cXnAddrStateReady;
                  
                  BUser * primary = gUserManager.getPrimaryUser();
                  if(primary)
                  {
                     primary->updateFriends();
                  }

                  BUser * secondary = gUserManager.getSecondaryUser();
                  if(secondary)
                  {
                     secondary->updateFriends();
                  }
               }
               break;
            }
      }
   }

   //If we have a current network session - update
   if (mpMPSession)
      mpMPSession->update();

   //See if I was waiting to delete that
   //if (mShuttingDownMPSession)
   //{
   //   disposeMPSession();
  // }

   updatePresence();

   //Check with the notification system
   DWORD msg = gNotification.getNotification();
   switch (msg)
   {
      case XN_LIVE_CONNECTIONCHANGED:
         {
            // if we establish a connection, we should only attempt to requery the XNADDR if it's in an error state
            if (gNotification.getParam() == XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
            {
               if (mXnAddrState == cXnAddrStateError)
                  mXnAddrState = cXnAddrStateInit;
            }
            break;
         }

      case XN_LIVE_LINK_STATE_CHANGED:
         {
            //Link state changed - have the XNADDR re-evaluated
            //Any system that was dependent on the XNADDR should be watching this same event to make its own smart choise about aborting or not
            mXnAddrState = cXnAddrStateInit;
            break;
         }
      //Sent upon any change of the user signin state. The accompanying parameter indicates which player 
      // indices are now valid and only those profiles that were selected during the current sign-in process.
      //case XN_SYS_SIGNINCHANGED:
      //   {
      //      //DWORD previousUserCount = mNumSignedInUsers;
      //      updateSignedInUserCount();
      //      if (mNumSignedInUsers > 0)
      //      {
      //         mConnectionState = cLiveConnectionStateReadyLoggedIn;
      //      }
      //
      //      //Do we need to show the login UI?
      //      if ((mStateLiveRequired) &&
      //         (mNumSignedInUsers < 1))
      //      {
      //         mConnectionState = cLiveConnectionStateReady;
      //         if (mInitialNotificationReceived)
      //         {
      //            //They have already been through the signin part and it failed/they declines
      //            //TODO find the command to kick them to the main menu no matter what they are doing...
      //            mStateLiveRequired = FALSE;
      //            gModeManager.setMode(BModeManager::cModeMenu);
      //         }
      //         else
      //         {
      //            showSignInScreen();
      //         }
      //      }

      //      mInitialNotificationReceived = TRUE;
      //      break;
      //   }

         //Broadcast when the connection to the Xbox Live server changes state.
      //case XN_LIVE_CONNECTIONCHANGED:
      //   {
      //      if ((mStateLiveRequired) && 
      //         (gNotification.getParam() != XONLINE_S_LOGON_CONNECTION_ESTABLISHED))
      //      {
      //         //We have lost our connection to live, kick out to main menu
      //         //TODO - nicer handling + message to the user + log actual error code 
      //         mConnectionState = cLiveConnectionStateReady;
      //         mStateLiveRequired = FALSE;
      //         gModeManager.setMode(BModeManager::cModeMenu);
      //      }
      //      break;
      //   }
   }

	/*
	DWORD dwRet = 0;

	// If there are not enough or too many profiles signed in, display an 
	// error message box prompting the user to either sign in again or exit the sample
	if( !m_bMessageBoxShowing && !m_bSystemUIShowing && m_bSigninUIWasShown && !AreUsersSignedIn() )
	{
		DWORD dwRet;

		ZeroMemory( &m_Overlapped, sizeof( XOVERLAPPED ) );

		WCHAR strMessage[512];
		swprintf_s( strMessage, L"Incorrect number of profiles signed in. You must sign in at least %d"
			L" and at most %d profiles. Currently there are %d profiles signed in.",
			m_dwMinUsers, m_dwMaxUsers, m_dwNumSignedInUsers );

		dwRet = XShowMessageBoxUI( XUSER_INDEX_ANY,
			L"Signin Error",   // Message box title
			strMessage,                 // Message
			ARRAYSIZE( m_pwstrButtons ),// Number of buttons
			m_pwstrButtons,             // Button captions
			0,                          // Button that gets focus
			XMB_ERRORICON,              // Icon to display
			&m_MessageBoxResult,        // Button pressed result
			&m_Overlapped );

		if( dwRet != ERROR_IO_PENDING )
			ATG::FatalError( "Failed to invoke message box UI, error %d\n", dwRet );

		m_bMessageBoxShowing = TRUE;
	}

	// Wait until the message box is discarded, then either exit or show the signin UI again
	if( m_bMessageBoxShowing && XHasOverlappedIoCompleted( &m_Overlapped ) )
	{
		m_bMessageBoxShowing = FALSE;

		if( XGetOverlappedResult( &m_Overlapped, NULL, TRUE ) == ERROR_SUCCESS )
		{
			switch( m_MessageBoxResult.dwButtonPressed )
			{
			case 0:     // Reboot to the launcher
				XLaunchNewImage( "", 0 );
				break;

			case 1:     // Show the signin UI again
				ShowSignInUI();
				m_bSigninUIWasShown = FALSE;
				break;
			}
		}
	}

	// Check to see if we need to invoke the signin UI
	if( !m_bMessageBoxShowing && m_bNeedToShowSignInUI && !m_bSystemUIShowing && m_bInitialNotificationReceived )
	{
		m_bNeedToShowSignInUI = FALSE;

		DWORD ret = XShowSigninUI( 
			m_dwSignInPanes, 
			m_bRequireOnlineUsers ? XSSUI_FLAGS_SHOWONLYONLINEENABLED : 0 );

		if( ret != ERROR_SUCCESS )
		{
			ATG::FatalError( "Failed to invoke signin UI, error %d\n", ret );
		} 
		else
		{
			m_bSystemUIShowing = TRUE;
			m_bSigninUIWasShown = TRUE;
		}
	}
	*/
}

//==============================================================================
// Used to tell the live system that a live login is required
//==============================================================================
//void BLiveSystem::setLiveRequired(BOOL newState)
//{
//   if (mStateLiveRequired == newState)
//   {
//      return;
//   }
//
//   mStateLiveRequired = newState;
//
//   if (mStateLiveRequired == TRUE)
//   {
//      // Check if we are already logged in
//      if (mNumSignedInUsers > 0)
//      {
//         return;
//      }
//
//      // If not, trigger off the login screen
//      //showSignInScreen();
//   }
//}

//==============================================================================
// 
//==============================================================================
BLiveVoice* BLiveSystem::getLiveVoice() const
{
   return mpVoice;
}

//==============================================================================
// 
//==============================================================================
BMPSession* BLiveSystem::getMPSession() const
{
    //if (!mpMPSession)
    //{
    //    mpMPSession = new BMPSession(mpVoice);
    //    BDEBUG_ASSERT(mpMPSession);
    //}
    return mpMPSession;
}

//==============================================================================
// 
//==============================================================================
BLiveVoice* BLiveSystem::initVoice()
{
   // intialize the audio engine for voice
   //
   // this should stay de-coupled from BMPSession instances so we avoid
   // the overhead of restarting the audio engine
   if (mpVoice)
      return mpVoice;

   //mpVoice = new BLiveVoice();
   //mpVoice = HEAP_NEW(BLiveVoice, gNetworkHeap);
   //mpVoice->init();
   mpVoice = BLiveVoice::getInstance();

   return mpVoice;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::disposeVoice()
{
   if (!mpVoice)
      return;

   //if (mShuttingDownMPSession)
   //{
   //   mDisposeVoice = true;
   //   return;
   //}

   mpVoice->release();
   mpVoice = NULL;

   //mpVoice->shutdown();
   //HEAP_DELETE(mpVoice, gNetworkHeap);
   //mpVoice = NULL;
}

//==============================================================================
// 
//==============================================================================
BMPSession* BLiveSystem::initMPSession()
{
   BASSERT(mShuttingDownMPSession==false);

   // dispose the previous session
   //NOTE - I dont do this any more because the game NEEDS to keep that thing cleaned up and in a reliable state, no more guessing at what state it is at
   //disposeMPSession();

   // mpVoice should be initialized by now, or we've changed
   // something in the supporting code.  Fix the supporting code.
   BDEBUG_ASSERT(mpVoice);
   
   if (!mpMPSession)
   {
      //BDEBUG_ASSERT(!mpMPSession);
      //mpMPSession = new BMPSession(mpVoice);
      mpMPSession = HEAP_NEW(BMPSession, gNetworkHeap);
      mpMPSession->init(mpVoice);
   }
   else
   {
      mpMPSession->init(mpVoice);
   }

   return mpMPSession;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::disposeMPSession()
{
   //Big change here, I'm going to NEVER dispose MP session, just reset it from now on
   if (mpMPSession)
   {
      mpMPSession->reset();
   }

   /*
   if (mpMPSession && !mShuttingDownMPSession)
   {
      mpMPSession->shutdown();
      mShuttingDownMPSession = true;
      return;
   }

   // make sure we're ok to delete BMPSession
   if (mpMPSession && !mpMPSession->isOkToDelete())
      return;

   mShuttingDownMPSession = false;

   //delete mpMPSession;
   if (mpMPSession != NULL)
   {
      HEAP_DELETE(mpMPSession, gNetworkHeap);
      mpMPSession = NULL;
   }
   */

   // only end the current voice's session
   // do not de-init the audio engine to save time on starting a new game
   //if (mpVoice)
   //   mpVoice->endSession();

   //if (mDisposeVoice)
   //   disposeVoice();

   // close any existing comm log and open a new one
   //BCommLog::cycleLogFile();
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::isInLanMode() const
{
   if (!mpMPSession)
      return FALSE;

   return mpMPSession->isInLANMode();
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::isMultiplayerLiveGameActive() const
{
   // if we are in LAN mode, then we definitely don't have a live game active
   if (isInLanMode())
      return false;

   return isMultiplayerGameActive();
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::isMultiplayerGameActive() const
{
   if (!mpMPSession)
   {
      return FALSE;
   }

   return (mpMPSession->isGameRunning());
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::isPartySessionActive() const
{
   if (!mpMPSession)
   {
      return FALSE;
   }

   if (!mpMPSession->getPartySession())
   {
      return FALSE;
   }

   if (!mpMPSession->getPartySession()->getSession())
   {
      return FALSE;
   }

   if (!mpMPSession->getPartySession()->getSession()->isConnected())
   {
      return FALSE;
   }
   
   return TRUE;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::disposeGameSession()
{
   //if (!mPartySystemReenterFlag)
   {
      disposeMPSession();
   }
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::getLocalXnAddr(XNADDR& xnAddr)
{
   if (mXnAddrState == cXnAddrStateReady)
   {
      xnAddr = mLocalXnAddr;
      return TRUE;
   }

   return FALSE;
}

//==============================================================================
// 
//==============================================================================
byte* BLiveSystem::getAITrackingMemoryBlockByPlayerID(PlayerID playerID)
{
   if ((mpMPSession) &&
       (mpMPSession->getGameSession()))
   {
      BMPSessionPlayer* player = mpMPSession->getGameSession()->getPlayerFromPlayerId(playerID);
      if (player)
      {
         return player->mpTrackingDataBlock;
      }
   }
   return NULL;
}

//==============================================================================
//Call this to indicate that a player is no longer in the game (but still connected to the network - used for defeated/resigned but not end of game)
//==============================================================================
void BLiveSystem::playerLeftGameplay(XUID playerXuid)
{
   //Yes this call has very little value but to pass on the call
   // However it helps isolate these checks and headers from having to be in the game code

   // per http://esbug01/browse/PHX-14881 - Keep losing players in Team voice channel and do NOT reveal the map in 2v2s and 3v3s
   //
   // By commenting the following lines out we will no longer place this player in the BVoice::cAll channel meaning they will
   // remain in their current channel

   //if ((mpMPSession) &&
   //    (mpMPSession->isGameRunning()) &&
   //    (mpMPSession->getSession()) &&
   //    (mpVoice))
   //{
   //   BClient* pClient = mpMPSession->getSession()->getClientByXuid(playerXuid);
   //   if (pClient)
   //   {
   //      mpVoice->setChannel(BVoice::cGameSession, playerXuid, BVoice::cAll);
   //   }
   //   else
   //   {
   //      nlog(cMPVoiceCL, "BLiveSystem::playerLeftGameplay - Cannot find client for xuid[%I64u]", playerXuid);
   //   }
   //}
}

//==============================================================================
// 
//==============================================================================
bool BLiveSystem::hasLeaderboardGameSettings()
{
   //Call this at any time to figure out if the current game has set its leaderboard needed global values yet or not
   //  If not game needs to call storeStatsGameSettings(), must be done before endOfGameComplete() is called
   if (mpMPSession && mpMPSession->getLiveSession())
   {
      //This is the multiplayer game setup way, and these values have been set by mpGameSession right before the game was launched
      return true;
   }
   if (mpLeaderboardOnlySession)
   {
      //This is the stand-alone method
      return true;
   }
   return false;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::endOfGamePlayerWon(const BSimString& playerName, XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime)
{
   if (playerXUID == 0)
   {
      nlog(cMPGameCL, "BLiveSystem::endOfGamePlayerWon - No XUID, ignoring request");
      return;
   }

   if ((mpMPSession) &&
       (mpMPSession->isGameRunning()) &&
       (mpMPSession->getSession()) &&
       (!mpMPSession->isInLANMode()) &&
       (mpMPSession->getLiveSession()))
   {
      if (playerWonGame)
      {
         nlog(cMPGameCL, "BLiveSystem::endOfGamePlayerWon - recording the win to Live for %s xuid[%I64u]", playerName.getPtr(), playerXUID);
      }
      else
      {
         nlog(cMPGameCL, "BLiveSystem::endOfGamePlayerWon - recording the loss to Live for %s xuid[%I64u]", playerName.getPtr(), playerXUID);
      }
      mpMPSession->getLiveSession()->setPlayerWon(playerXUID, playerWonGame, teamID, leaderIndex, score, gamePlayTime);
      return;
   }
   //See if there is a leaderboard only session to post to
   if (mpLeaderboardOnlySession)
   {
      nlog(cMPGameCL, "BLiveSystem::endOfGamePlayerWon - leaderboard posting the results to Live for %s xuid[%I64u]", playerName.getPtr(), playerXUID);
      mpLeaderboardOnlySession->setPlayerWon(playerXUID, playerWonGame, teamID, leaderIndex, score, gamePlayTime);
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::endOfGameComplete()
{
   if ((mpMPSession) &&
       (mpMPSession->isGameRunning()))
   {
      mpMPSession->processEndOfGame();
   }

   if (mpVoice)
      mpVoice->migrateSessionIfAble(BVoice::cGameSession, BVoice::cPartySession, BVoice::cAll);
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::setMPTestOptions( liveSystemTestOptions optionCode, BOOL enable )
{
   mTestOptions[optionCode] = enable;
}

//==============================================================================
// 
//==============================================================================
BOOL BLiveSystem::getMPTestOptions( liveSystemTestOptions optionCode )
{  
   return mTestOptions[optionCode];
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::testLeaderBoardQuery()
{
#ifndef BUILD_FINAL

   //Alloc memory if not already acquired
   if (!mpTestingLBData)
   {
      mpTestingLBData = new leaderBoardReturnDataRow[10];
      BASSERT(mpTestingLBData);
   }
   if (!mpTestingLBCount)
   {
      mpTestingLBCount = new uint32;
      *mpTestingLBCount = 10;
   }
   if (!mpTestingLBTotalRows)
   {
      mpTestingLBTotalRows = new uint32;
      *mpTestingLBTotalRows = 0;
   }
   
   XUID testXUID = 0;
   BUser* user = gUserManager.getPrimaryUser();
   if (user)
   {
      testXUID = user->getXuid();
   }

#ifdef BUILD_DEBUG
   if (gConfig.isDefined(cConfigMoreLiveOutputSpam))
   {      
      XDebugSetSystemOutputLevel(HXAMAPP_XGI, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XLIVEBASE, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XAM, 3);
   }   
#endif

   //Launch new request
   //STATS_VIEW_MPWINSLIFETIME_1V1_GT1   STATS_VIEW_SKILL_RANKED_STANDARD_1V1   
   if (!leaderBoardLaunchQuery(STATS_VIEW_MPWINSLIFETIME_1V1_GT1, 0, 1, mpTestingLBData, mpTestingLBCount, mpTestingLBTotalRows))
   //if (!leaderBoardFriendFilterLaunchQuery(STATS_VIEW_CAMPSCORELIFETIME_SOLO, testXUID, mpTestingLBData, mpTestingLBCount, mpTestingLBTotalRows))   
   {
      BASSERT(false);
   }


#endif
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::testLeaderBoardRead(BSimString* resultText)
{
#ifndef BUILD_FINAL
   if (leaderBoardQueryStatus()==cLeaderBoardStatusNoQueryPending)
   {
      resultText->set("Not running");
   }
   else if (leaderBoardQueryStatus()==cLeaderBoardStatusQueryRunning)
   {
      resultText->set("Running query");
   }
   else if (leaderBoardQueryStatus()==cLeaderBoardStatusQueryComplete)
   {
      resultText->format("Done:%d/%d rows available",*mpTestingLBCount, *mpTestingLBTotalRows);
   }
#endif
}

//==============================================================================
// 
//==============================================================================
uint BLiveSystem::findLeaderBoardIndex(BLeaderBoardTypes lbType, uint gameSize, bool partyTeam, uint difficulty, uint campaignMapIndex, uint gameTypeIndex, uint leaderIndex )
{
   //This is just a big block of code which maps the various types of information into a single leaderboard index
   switch (lbType)
   {
      //Base skill boards for each skill matchmaking hopper
      //5 boards
      //Requires: gameSize, partyTeam
      case(cLeaderBoardTypeMPSkill):
      {
         BASSERT(gameSize);
         BASSERT(gameSize<=6);
         if (gameTypeIndex==1)
         {
            if (partyTeam)
            {
               switch(gameSize)
               {
                  case(4): return STATS_VIEW_SKILL_RANKED_TEAM_2V2;
                  case(6): return STATS_VIEW_SKILL_RANKED_TEAM_3V3;
               }
            }
            else
            {
               switch(gameSize)
               {
                  case(2): return STATS_VIEW_SKILL_RANKED_STANDARD_1V1;
                  case(4): return STATS_VIEW_SKILL_RANKED_STANDARD_2V2;
                  case(6): return STATS_VIEW_SKILL_RANKED_STANDARD_3V3;            
               }
            }
         }
         else if (gameTypeIndex==2)
         {
            if (partyTeam)
            {
               switch(gameSize)
               {
               case(4): return STATS_VIEW_SKILL_RANKED_DM_TEAM_2V2;
               case(6): return STATS_VIEW_SKILL_RANKED_DM_TEAM_3V3;
               }
            }
            else
            {
               switch(gameSize)
               {
               case(2): return STATS_VIEW_SKILL_RANKED_DM_1V1;
               case(4): return STATS_VIEW_SKILL_RANKED_DM_2V2;
               case(6): return STATS_VIEW_SKILL_RANKED_DM_3V3;            
               }
            }
         }
         BASSERTM(false, "No cLeaderBoardTypeMPSkill index found!");
         return 0;
      }

      //Lifetime campaign boards - individual and co-op 
      //2 boards
      //Requires: partyTeam
      case(cLeaderBoardTypeCampaignLifetime):
      {
         if (partyTeam)
            return STATS_VIEW_CAMPSCORELIFETIME_COOP;
         else
            return STATS_VIEW_CAMPSCORELIFETIME_SOLO;
      }

      //Best campaign score per level per difficulty
      //60 boards
      //Requires: difficulty, campaignMapIndex
      case(cLeaderBoardTypeCampaignBestScore):
      {
         BASSERT(difficulty);
         BASSERT(difficulty<=4);
         BASSERT(campaignMapIndex);
         BASSERT(campaignMapIndex<=15);
         uint32 value = STATS_VIEW_CAMPSCOREBYMISSION_M01_D1 + ((difficulty-1)*15) + (campaignMapIndex-1);
         BASSERT(value>=STATS_VIEW_CAMPSCOREBYMISSION_M01_D1);
         BASSERT(value<=STATS_VIEW_CAMPSCOREBYMISSION_M15_D4);
         return (value);
      }

      //Total wins in MP per ranked game hopper and game type
      //30 boards
      //Requires: gameSize, partyTeam, gameTypeIndex
      case (cLeaderBoardTypeMPGametypeWinsLifetime):
      {
         BASSERT(gameTypeIndex);
         BASSERT(gameTypeIndex<=6);
         BASSERT(gameSize);
         BASSERT(gameSize<=6);
         uint gameTypeOffset = 5 * (gameTypeIndex-1);
         if (partyTeam)
         {
            switch(gameSize)
            {
            case(4): return STATS_VIEW_MPWINSLIFETIME_2V2T_GT1+gameTypeOffset;
            case(6): return STATS_VIEW_MPWINSLIFETIME_3V3T_GT1+gameTypeOffset;
            }
         }
         else
         {
            switch(gameSize)
            {
            case(2): return STATS_VIEW_MPWINSLIFETIME_1V1_GT1+gameTypeOffset;
            case(4): return STATS_VIEW_MPWINSLIFETIME_2V2_GT1+gameTypeOffset;
            case(6): return STATS_VIEW_MPWINSLIFETIME_3V3_GT1+gameTypeOffset;            
            }
         }
      }

      //Monthly wins in MP per ranked game hopper and game type
      //30 boards
      //Requires: gameSize, partyTeam, gameTypeIndex
      case (cLeaderBoardTypeMPGametypeWinsMonthly):
      {
         BASSERT(gameTypeIndex);
         BASSERT(gameTypeIndex<=6);
         BASSERT(gameSize);
         BASSERT(gameSize<=6);
         uint gameTypeOffset = 5 * (gameTypeIndex-1);
         if (partyTeam)
         {
            switch(gameSize)
            {
            case(4): return STATS_VIEW_MPWINSMONTHLY_2V2T_GT1+gameTypeOffset;
            case(6): return STATS_VIEW_MPWINSMONTHLY_3V3T_GT1+gameTypeOffset;
            }
         }
         else
         {
            switch(gameSize)
            {
            case(2): return STATS_VIEW_MPWINSMONTHLY_1V1_GT1+gameTypeOffset;
            case(4): return STATS_VIEW_MPWINSMONTHLY_2V2_GT1+gameTypeOffset;
            case(6): return STATS_VIEW_MPWINSMONTHLY_3V3_GT1+gameTypeOffset;            
            }
         }
      }

      //Monthly wins in MP per leader and gametype  (10 leaders max)
      //60 boards
      //Requires: leaderIndex, gameTypeIndex
      case (cLeaderBoardTypeMPLeaderWinsMonthly):
      {
         BASSERT(leaderIndex);
         BASSERT(leaderIndex<=10);
         BASSERT(gameTypeIndex);
         BASSERT(gameTypeIndex<=6);
         return STATS_VIEW_MPWINSMONTHLY_L1_GT1 + ((gameTypeIndex-1)*10) + (leaderIndex-1);        
      }

      //Monthly in MP per gametype per gamesize(special), each board is tracked special based on the gametype
      //18 boards
      //Requires:  gameSize, partyTeam, gameTypeIndex
      case (cLeaderBoardTypeMPSpecialMonthly):
      {
         //TODO - NOT FUNCTIONAL YET
         BASSERT(false);
         BASSERT(gameTypeIndex);
         BASSERT(gameTypeIndex<=6);
         BASSERT(gameSize);
         BASSERT(gameSize<=6);
         BASSERT(partyTeam || (gameSize==2));   //If it is a 2v2 or 3v3, then we are only tracking for TEAMS, not random jimmys
         uint gameTypeOffset = 3 * (gameTypeIndex-1);
         switch(gameSize)
         {
            case(2): return STATS_VIEW_SKILL_RANKED_STANDARD_1V1+gameTypeOffset;
            case(4): return STATS_VIEW_SKILL_RANKED_STANDARD_2V2+gameTypeOffset;
            case(6): return STATS_VIEW_SKILL_RANKED_STANDARD_3V3+gameTypeOffset;            
         }
      }

      //Monthly boards per gamesize per gametype, fastest time to victory versus AI
      //18 boards
      //Requires:  gameSize, partyTeam, gameTypeIndex
      case (cLeaderBoardTypeVsAIMonthly):
      {
         //TODO - NOT FUNCTIONAL YET
         BASSERT(false);
         BASSERT(gameTypeIndex);
         BASSERT(gameTypeIndex<=6);
         BASSERT(gameSize);
         BASSERT(gameSize<=6);
         BASSERT(partyTeam || (gameSize==2));   //If it is a 2v2 or 3v3, then we are only tracking for TEAMS, not random jimmys
         uint gameTypeOffset = 3 * (gameTypeIndex-1);
         switch(gameSize)
         {
            case(2): return STATS_VIEW_SKILL_RANKED_STANDARD_1V1+gameTypeOffset;
            case(4): return STATS_VIEW_SKILL_RANKED_STANDARD_2V2+gameTypeOffset;
            case(6): return STATS_VIEW_SKILL_RANKED_STANDARD_3V3+gameTypeOffset;            
         }     
      }

   }
   BASSERTM(false, "No leaderboard index found!");
   return 0;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::fillInLeaderBoardRequestSettings()
{
   //Based on the leaderboard index being requested - we have to ask for different column data - this next section does all that mapping
   if ((mLeaderBoardIndex>=STATS_VIEW_SKILL_RANKED_STANDARD_1V1) &&
       (mLeaderBoardIndex<=STATS_VIEW_SKILL_RANKED_DM_TEAM_3V3))
   {
      mLeaderBoardCurrentRequest[0].dwNumColumnIds = 2;
      mLeaderBoardCurrentRequest[0].dwViewId = mLeaderBoardIndex;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[0] = X_STATS_COLUMN_SKILL_SKILL;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[1] = X_STATS_COLUMN_SKILL_GAMESPLAYED;
      //We can request these as well if we ever want them
      //mLeaderBoardCurrentRequest[0].rgwColumnIds[2] = X_STATS_COLUMN_SKILL_MU;
      //mLeaderBoardCurrentRequest[0].rgwColumnIds[3] = X_STATS_COLUMN_SKILL_SIGMA;
   }
   else if ((mLeaderBoardIndex>=STATS_VIEW_MPWINSLIFETIME_1V1_GT1) && 
            (mLeaderBoardIndex<=STATS_VIEW_MPWINSMONTHLY_L0_GT6))
   {
      mLeaderBoardCurrentRequest[0].dwNumColumnIds = 2;
      mLeaderBoardCurrentRequest[0].dwViewId = mLeaderBoardIndex;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[0] = STATS_COLUMN_MPWINSLIFETIME_1V1_GT1_GAMESPLAYED;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[1] = STATS_COLUMN_MPWINSLIFETIME_1V1_GT1_TOTALSCORE;
   }
   else if ((mLeaderBoardIndex>=STATS_VIEW_CAMPSCORELIFETIME_SOLO) && 
            (mLeaderBoardIndex<=STATS_VIEW_CAMPSCORELIFETIME_COOP))
   {
      mLeaderBoardCurrentRequest[0].dwNumColumnIds = 1;
      mLeaderBoardCurrentRequest[0].dwViewId = mLeaderBoardIndex;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[0] = STATS_COLUMN_CAMPSCORELIFETIME_SOLO_GAMESPLAYED;
   }
   else if ((mLeaderBoardIndex>=STATS_VIEW_CAMPSCOREBYMISSION_M01_D1) && 
            (mLeaderBoardIndex<=STATS_VIEW_CAMPSCOREBYMISSION_M15_D4))
   {
      mLeaderBoardCurrentRequest[0].dwNumColumnIds = 1;
      mLeaderBoardCurrentRequest[0].dwViewId = mLeaderBoardIndex;
      mLeaderBoardCurrentRequest[0].rgwColumnIds[0] = STATS_COLUMN_CAMPSCOREBYMISSION_M01_D1_WASCOOP;
   }
   else
   {
      //Not a supported leaderboard index
      BASSERT(false);
      return;
   }
}

//==============================================================================
// 
//==============================================================================
bool BLiveSystem::leaderBoardFriendFilterLaunchQuery(uint32 leaderBoardIndex, XUID requestingUser, leaderBoardReturnDataRow* rowData, uint32* rowDataCount, uint32* numberOfRowsInTable )
{
   if ((mLeaderBoardState!=cLeaderBoardStateIdle) &&
      (mLeaderBoardState!=cLeaderBoardStateComplete))
   {
      //BASSERTM(false, "BLiveSystem::leaderBoardLaunchQuery - Not in a state where I can launch another query.");
      return false;
   }

   mLeaderBoardError = ERROR_SUCCESS;

   BASSERT(requestingUser);

   //Store the query data
   mLeaderBoardIndex = leaderBoardIndex;
   mLeaderBoardRequestingUser = requestingUser;
   mLeaderBoardIndexToUser = false;
   mLeaderBoardJustShowFriends = true;
   mpLeaderBoardReturnDataRows = rowData;
   mpLeaderBoardReturnDataCount = rowDataCount;
   mpLeaderBoardReturnTotalRowsInEntireBoard = numberOfRowsInTable;
   mLeaderBoardQueryTime = timeGetTime();
   Utils::FastMemSet(&mLeaderBoardCurrentRequest[0], 0, sizeof(XUSER_STATS_SPEC));  
 
   //Find the controllerID for that XUID
   long controllerID = -1;
   BUser* user = gUserManager.getPrimaryUser();
   if (user && user->getXuid()==requestingUser)
   {
      controllerID = user->getPort();
   }
   else
   {
      user = gUserManager.getSecondaryUser();
      if (user && user->getXuid()==requestingUser)
      {
         controllerID = user->getPort();
      }
   }
   BASSERT(controllerID!=-1);
   if (controllerID==-1)
   {
      return false;
   }

   if (mLeaderBoardEnumerator != INVALID_HANDLE_VALUE)
   {
      CloseHandle(mLeaderBoardEnumerator);
      mLeaderBoardEnumerator = INVALID_HANDLE_VALUE;
   }

   DWORD bufferSize;
   DWORD result = XFriendsCreateEnumerator(controllerID, 0, MAX_FRIENDS, &bufferSize, &mLeaderBoardEnumerator);
   if (result != ERROR_SUCCESS )
   {
      //Call failed
      leaderBoardCancelQuery(result);
      //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      //BASSERT(pUIGlobals);
      //if (pUIGlobals)
      //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
      return false;
   }
   mFriendCount = 0;
   if (mpFriendListMemory)
   {
      delete[] mpFriendListMemory;
      mpFriendListMemory = NULL;
   }
   mpFriendListMemory = new BYTE[bufferSize];

   //Call the friends list request async
   Utils::FastMemSet(&mLeaderBoardOverlap, 0, sizeof(XOVERLAPPED));
   result = XEnumerate(mLeaderBoardEnumerator, mpFriendListMemory, bufferSize, NULL, &mLeaderBoardOverlap );
   if( result != ERROR_IO_PENDING )
   {
      leaderBoardCancelQuery(result);
      //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      //BASSERT(pUIGlobals);
      //if (pUIGlobals)
      //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
      return false;
   }

   mLeaderBoardState = cLeaderBoardStateFriendQuery;
   return true;
}

//==============================================================================
// 
//==============================================================================
// If you set requestingUser to non zero, then it will pivot to their location
// This is incompatible with setting the startAtRank value
bool BLiveSystem::leaderBoardLaunchQuery(uint32 leaderBoardIndex, XUID requestingUser, uint32 startAtRank, leaderBoardReturnDataRow* rowData, uint32* rowDataCount, uint32* numberOfRowsInTable )
{
   if ((mLeaderBoardState!=cLeaderBoardStateIdle) &&
       (mLeaderBoardState!=cLeaderBoardStateComplete))
   {
      //BASSERTM(false, "BLiveSystem::leaderBoardLaunchQuery - Not in a state where I can launch another query.");
      return false;
   }

   mLeaderBoardError = ERROR_SUCCESS;

   BASSERT(startAtRank>0);
   BASSERT(leaderBoardIndex>0);

   //Store the query data
   mLeaderBoardIndex = leaderBoardIndex;
   mLeaderBoardRequestingUser = requestingUser;
   mLeaderBoardIndexToUser = (requestingUser!=0);
   mLeaderBoardJustShowFriends = false;
   mpLeaderBoardReturnDataRows = rowData;
   mpLeaderBoardReturnDataCount = rowDataCount;
   mpLeaderBoardReturnTotalRowsInEntireBoard = numberOfRowsInTable;
   mLeaderBoardQueryTime = timeGetTime();
   Utils::FastMemSet(&mLeaderBoardCurrentRequest[0], 0, sizeof(XUSER_STATS_SPEC));    
   fillInLeaderBoardRequestSettings();

   if (mLeaderBoardEnumerator != INVALID_HANDLE_VALUE)
   {
      CloseHandle(mLeaderBoardEnumerator);
      mLeaderBoardEnumerator = INVALID_HANDLE_VALUE;
   }

   DWORD apiResult = 0;
   if (mLeaderBoardIndexToUser)
   {
      BASSERT(requestingUser);
      apiResult = XUserCreateStatsEnumeratorByXuid( TITLEID_HALO_WARS, requestingUser, *mpLeaderBoardReturnDataCount, 1, mLeaderBoardCurrentRequest, &mLeaderBoardEnumerateResults, &mLeaderBoardEnumerator );
   }
   else
   {
      apiResult = XUserCreateStatsEnumeratorByRank( TITLEID_HALO_WARS, startAtRank, *mpLeaderBoardReturnDataCount, 1, mLeaderBoardCurrentRequest, &mLeaderBoardEnumerateResults, &mLeaderBoardEnumerator );
   }

   if( apiResult != ERROR_SUCCESS )
   {
      leaderBoardCancelQuery(apiResult);
      //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      //BASSERT(pUIGlobals);
      //if (pUIGlobals)
      //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
      return false;
   }

   // Allocate the buffer
   if (mpLeaderBoardStatsResults)
   {
      delete mpLeaderBoardStatsResults;
      mpLeaderBoardStatsResults = NULL;
   }
   mpLeaderBoardStatsResults = ( PXUSER_STATS_READ_RESULTS ) new BYTE[ mLeaderBoardEnumerateResults ];
   BASSERT(mpLeaderBoardStatsResults);
   Utils::FastMemSet(mpLeaderBoardStatsResults, 0, mLeaderBoardEnumerateResults);
   Utils::FastMemSet(&mLeaderBoardOverlap, 0, sizeof(XOVERLAPPED));

   apiResult = XEnumerate( mLeaderBoardEnumerator, mpLeaderBoardStatsResults, mLeaderBoardEnumerateResults, NULL, &mLeaderBoardOverlap);

   if( apiResult != ERROR_IO_PENDING )
   {
      leaderBoardCancelQuery(apiResult);
      //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      //BASSERT(pUIGlobals);
      //if (pUIGlobals)
      //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
      return false;
   }

   mLeaderBoardState = cLeaderBoardStateStatsQuery;
   return true;
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::leaderBoardCancelQuery(DWORD result)
{
   if ((mLeaderBoardState==cLeaderBoardStateStatsQuery) ||
       (mLeaderBoardState==cLeaderBoardStateFriendQuery))
   {
      //Make sure i'm not in some wierd state where we aren't waiting on a call to compete
      if (XHasOverlappedIoCompleted(&mLeaderBoardOverlap)) 
      {
         //We are canceled
         if (mpLeaderBoardReturnDataCount)
            *mpLeaderBoardReturnDataCount = 0;
         mLeaderBoardState = cLeaderBoardStateIdle;

         //Clean up
         if (mpLeaderBoardStatsResults)
         {
            delete mpLeaderBoardStatsResults;
            mpLeaderBoardStatsResults = NULL;
         }
         if (mpFriendListMemory)
         {
            delete[] mpFriendListMemory;
            mpFriendListMemory = NULL;
         }
         if (mpXUIDRequestList)
         {
            delete[] mpXUIDRequestList;
            mpXUIDRequestList = NULL;
         }

         if (mLeaderBoardEnumerator != INVALID_HANDLE_VALUE)
         {
            CloseHandle(mLeaderBoardEnumerator);
            mLeaderBoardEnumerator = INVALID_HANDLE_VALUE;
         }

         //We are cleaned up - just go to state complete - it is safe for someone to issue another query now
      }
      else
      {
         XCancelOverlapped( &mLeaderBoardOverlap );
         mLeaderBoardState = cLeaderBoardStateCanceling;
      }
   }
   else if (mLeaderBoardState==cLeaderBoardStateComplete)
   {
      //We have nothing pending, just move to state idle
      mLeaderBoardState = cLeaderBoardStateIdle;
   }

   mLeaderBoardError = result;
}

//==============================================================================
//
//==============================================================================
BOOL BLiveSystem::CompareLeaderBoardRows( const XUSER_STATS_ROW& a, const XUSER_STATS_ROW& b )
{
   // Special case out unranked players
   if( a.dwRank == 0 && b.dwRank == 0 ) return FALSE; // If neither has played, they're equal
   else if( a.dwRank == 0 ) return FALSE;// If A hasn't played, return B first
   else if( b.dwRank == 0 ) return TRUE; // If B hasn't played, return A first

   // If A's rank is lower ( a better score ), this will be negative and A will be first in the list
   return ( a.dwRank < b.dwRank );
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::createLeaderboardSession(int nOwnerController, XUID owningXUID, uint memberCount, UINT gameModeIndex)
{
   if (mpLeaderboardOnlySession)
   {
      nlog(cMPGameCL, "BLiveSystem::createLeaderboardSession - ERROR leaderboard session NOT CREATED, old one was there still");
      BASSERT(false);
      return;
   }

   nlog(cMPGameCL, "BLiveSystem::createLeaderboardSession - leaderboard session is created");
   mpLeaderboardOnlySession = new BLiveSessionLeaderboardOnly(nOwnerController, owningXUID, memberCount, gameModeIndex);
   mLeaderBoardCreationTime = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::leaderBoardUpdate()
{
   if (mpLeaderboardOnlySession)
   {      
      mpLeaderboardOnlySession->update();
      if (mpLeaderboardOnlySession->canBeDeleted())
      {
         nlog(cMPGameCL, "BLiveSystem::leaderBoardUpdate - leaderboard is finished closing, deleting it");
         delete mpLeaderboardOnlySession;
         mpLeaderboardOnlySession = NULL;
      }
      else if ((timeGetTime()-mLeaderBoardCreationTime) > cBLiveSystemLeaderBoardOnlySessionTimeout)
      {
         //We waited too long for this - kill it
         nlog(cMPGameCL, "BLiveSystem::leaderBoardUpdate - leaderboard should be gone already, killing it");
         mpLeaderboardOnlySession->shutDown();
         mLeaderBoardCreationTime=timeGetTime();
      }
   }

   switch(mLeaderBoardState) 
   {
      case(cLeaderBoardStateIdle):
         {
            return;
         }
      case (cLeaderBoardStateFriendQuery):
         {
            //Check to see if that first query is complete
            if (XHasOverlappedIoCompleted(&mLeaderBoardOverlap))
            {
               HRESULT hr = XGetOverlappedExtendedError(&mLeaderBoardOverlap);
               if (FAILED(hr))
               {
                  //We can get a valid return here of ERROR_NO_MORE_FILES
                  // which means they have no friends.
                  //However lets just handle all errors as if the list was empty 
                  //That way they will always at least have themselves as a friend (lol)
                  *mpLeaderBoardReturnDataCount=1;
                  mFriendCount = 0;
               }
               else
               {
                  mFriendCount = mLeaderBoardOverlap.InternalHigh;
               }

               //Pack those friend XUIDs into the request structure
               if (mpXUIDRequestList)
               {
                  delete mpXUIDRequestList;
                  mpXUIDRequestList = NULL;
               }

               mpXUIDRequestList = new XUID[mFriendCount+1];

               uint friendIndex=0;
               for (uint i=0;i<mFriendCount;i++)
               {      
                  XONLINE_FRIEND* pFriend = reinterpret_cast<XONLINE_FRIEND*>(mpFriendListMemory+(i*sizeof(XONLINE_FRIEND)));

                  if  ( ( (pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST) == 0 ) &&
                     ( (pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST ) == 0 ))
                  {
                     mpXUIDRequestList[friendIndex] = pFriend->xuid;
                     friendIndex++;
                  }
               }

               //Release that memory and handle
               delete[] mpFriendListMemory;
               mpFriendListMemory = NULL;
               if (mLeaderBoardEnumerator != INVALID_HANDLE_VALUE)
               {
                  CloseHandle(mLeaderBoardEnumerator);
                  mLeaderBoardEnumerator = INVALID_HANDLE_VALUE;
               }

               //Add in the requestor's XUID
               mpXUIDRequestList[friendIndex] = mLeaderBoardRequestingUser;
               friendIndex++;

               //Don't request more than we need
               if (friendIndex>*mpLeaderBoardReturnDataCount)
               {
                  friendIndex=*mpLeaderBoardReturnDataCount;
               }
               else
               {
                  *mpLeaderBoardReturnDataCount=friendIndex;
               }

               fillInLeaderBoardRequestSettings();

               //Dump the old buffer if there was one
               mLeaderBoardEnumerateResults=0;
               if (mpLeaderBoardStatsResults)
               {
                  delete mpLeaderBoardStatsResults;
                  mpLeaderBoardStatsResults = NULL;
               }

               DWORD apiResult = XUserReadStats( TITLEID_HALO_WARS, friendIndex, mpXUIDRequestList, 1, mLeaderBoardCurrentRequest, &mLeaderBoardEnumerateResults,  mpLeaderBoardStatsResults, NULL );

               if ((apiResult != ERROR_INSUFFICIENT_BUFFER) ||
                  (mLeaderBoardEnumerateResults ==0))
               {
                  leaderBoardCancelQuery(apiResult);
                  //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  //BASSERT(pUIGlobals);
                  //if (pUIGlobals)
                  //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
                  return;
               }

               // Allocate the buffer
               mpLeaderBoardStatsResults = ( PXUSER_STATS_READ_RESULTS ) new BYTE[ mLeaderBoardEnumerateResults ];

               if (!mpLeaderBoardStatsResults)
               {
                  BASSERTM(false,"BLiveSystem::leaderBoardUpdate - not enough memory to alloc buffer!");
                  leaderBoardCancelQuery(ERROR_OUTOFMEMORY);
                  return;
               }

               Utils::FastMemSet(mpLeaderBoardStatsResults, 0, mLeaderBoardEnumerateResults);
               Utils::FastMemSet(&mLeaderBoardOverlap, 0, sizeof(XOVERLAPPED));

               apiResult = XUserReadStats( TITLEID_HALO_WARS, friendIndex, mpXUIDRequestList, 1, &mLeaderBoardCurrentRequest[0], &mLeaderBoardEnumerateResults,  mpLeaderBoardStatsResults, &mLeaderBoardOverlap );
               //ret = XUserReadStats( titleID, partySize,        &mSkillQueryXuidList[0], 1, &mSkillQueryRequest[0],         &mSkillQueryResultsSize,        mpSkillQueryResults,       &mSkillQueryOverlapped );
               if( apiResult != ERROR_IO_PENDING )
               {
                  //Valid errors here can happen when you lose your connection to Live                  
                  leaderBoardCancelQuery(apiResult);
                  //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  //BASSERT(pUIGlobals);
                  //if (pUIGlobals)
                  //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
                  return;
               }

               mLeaderBoardState = cLeaderBoardStateStatsQuery;
               break;
            }
         }
      case(cLeaderBoardStateStatsQuery):
         {
            //Check to see if that first query is complete
            if (XHasOverlappedIoCompleted(&mLeaderBoardOverlap))
            {
               //Drop the memory for the XUID request list if it was there
               if (mpXUIDRequestList)
               {
                  delete[] mpXUIDRequestList;
                  mpXUIDRequestList = NULL;
               }

               HRESULT hr = XGetOverlappedExtendedError(&mLeaderBoardOverlap);
               if (FAILED(hr))
               {
                  leaderBoardCancelQuery(HRESULT_CODE(hr));
                  //BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  //BASSERT(pUIGlobals);
                  //if (pUIGlobals)
                  //   pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
                  return;
               }

               //For friends results - sort them
               if (mLeaderBoardJustShowFriends)
               {
                  std::sort( mpLeaderBoardStatsResults->pViews[0].pRows, &mpLeaderBoardStatsResults->pViews[0].pRows[ mpLeaderBoardStatsResults->pViews[0].dwNumRows ], &CompareLeaderBoardRows );
               }               

               //It is finished - parse the data from mpLeaderBoardStatsResults into mpLeaderBoardReturnDataRows
               uint rowCount = mpLeaderBoardStatsResults->pViews[0].dwNumRows;
               *mpLeaderBoardReturnTotalRowsInEntireBoard = mpLeaderBoardStatsResults->pViews[0].dwTotalViewRows;
               const DWORD cColumnCount = mLeaderBoardCurrentRequest[0].dwNumColumnIds;
               int rankedCount = 0;
               for (uint i=0;i<rowCount;i++)
               {
                  mpLeaderBoardReturnDataRows[i].mXuid = mpLeaderBoardStatsResults->pViews[0].pRows[i].xuid;
                  strcpy_s( mpLeaderBoardReturnDataRows[i].mGamerTag, XUSER_NAME_SIZE, mpLeaderBoardStatsResults->pViews[0].pRows[i].szGamertag);
                  mpLeaderBoardReturnDataRows[i].mRank = mpLeaderBoardStatsResults->pViews[0].pRows[i].dwRank;
                  if (mpLeaderBoardReturnDataRows[i].mRank!=0)
                  {
                     rankedCount++;
                  }
                  mpLeaderBoardReturnDataRows[i].mRating = mpLeaderBoardStatsResults->pViews[0].pRows[i].i64Rating;
                  if (cColumnCount>0)
                  {
                     mpLeaderBoardReturnDataRows[i].mIntValue1 = mpLeaderBoardStatsResults->pViews[0].pRows[i].pColumns[0].Value.i64Data;
                  }
                  if (cColumnCount>1)
                  {
                     mpLeaderBoardReturnDataRows[i].mIntValue2 = mpLeaderBoardStatsResults->pViews[0].pRows[i].pColumns[1].Value.i64Data;
                  }
               }
               *mpLeaderBoardReturnDataCount = rankedCount;
               delete mpLeaderBoardStatsResults;
               mpLeaderBoardStatsResults = NULL;
               mLeaderBoardState=cLeaderBoardStateComplete;
            }
            else
            {
               //Still not done - check for global timeout
               //TODO mLeaderBoardQueryTime
            }
            break;
         }
      case (cLeaderBoardStateCanceling):
         {
            if (XHasOverlappedIoCompleted(&mLeaderBoardOverlap))
            {
               mLeaderBoardState=cLeaderBoardStateIdle;
            }
            break;
         }
   }   
}

//==============================================================================
// 
//==============================================================================
BLiveSystem::leaderBoardStatusResult BLiveSystem::leaderBoardQueryStatus()
{
   switch (mLeaderBoardState)
   {
      case(cLeaderBoardStateIdle) : return cLeaderBoardStatusNoQueryPending;
      case(cLeaderBoardStateStatsQuery) : return cLeaderBoardStatusQueryRunning;
      case(cLeaderBoardStateFriendQuery) : return cLeaderBoardStatusQueryRunning;
      case(cLeaderBoardStateCanceling) : return cLeaderBoardStatusQueryRunning;
      case(cLeaderBoardStateComplete) : return cLeaderBoardStatusQueryComplete;
   }
     
   return cLeaderBoardStatusNoQueryPending;
};

//==============================================================================
// This also works for property values as well - 
//    - You need to set isProperty to true
//    - Currently it only supports DWORDS (or int32 if you don't mind a straight cast)
//==============================================================================
void BLiveSystem::setPresenceContext( DWORD contextType, DWORD contextValue, BOOL isProperty )
{

   //Find the entry
   int32 firstEmpty = -1;
   int32 index = -1;
   for (int32 i=0;i<cLiveSystemMaxContexts;i++)
   {
      if (mPresenceContext[i].mContextType==contextType)
      {
         index = i;
         break;
      }
      if ((!mPresenceContext[i].mInUse) &&
          (firstEmpty==-1))
      {
         firstEmpty = i;
      }
   }
   if (index == -1)
   {
      //Not there yet
      if (firstEmpty == -1)
      {
         //Out of entries - need to increase cLiveSystemMaxContexts
         BASSERTM(false, "BLiveSystem::setPresenceContext - Out of slots, need to increase cLiveSystemMaxContexts");
         return;
      }
      index = firstEmpty;

      //Set its mapped values
      mPresenceContext[index].mContextType = contextType;
      mPresenceContext[index].mInUse = true;
      mPresenceContext[index].mProperty = isProperty;
   }
   else
   {
      //Existing node - make sure I'm not setting it to just the same value
      if (mPresenceContext[index].mContextValue == contextValue)
      {
         return;
      }
   }

   //Set its value
   mPresenceContext[index].mContextValue = contextValue;
   mPresenceContext[index].mDirty = true;

}

//==============================================================================
// 
//==============================================================================
void BLiveSystem::updatePresence()
{
   //If we have an async operation pending and it is complete, mark this and reset the overlapped structure
   if (mPresenceIsOperationPending && XHasOverlappedIoCompleted(&mPresenceOverlap))
   {
      mPresenceIsOperationPending = false;
      Utils::FastMemSet(&mPresenceOverlap, 0, sizeof(XOVERLAPPED));
   }

   //Do not update unless we don't have an async operation pending
   if (mPresenceIsOperationPending)
      return;

   //Check to see if we should update
   if ((timeGetTime() - mLastPresenceUpdate) > cBLiveSystemPresenceRefreshTime)
   {
      //Lets quick post a game update time
      if (gWorld)
      {
         setPresenceContext(PROPERTY_GAMETIMEMINUTES, ((DWORD)gWorld->getGametime() / 60000)+1, true);
      }

      //Set the current time as the last presence update time
      mLastPresenceUpdate = timeGetTime();

//-- FIXING PREFIX BUG ID 5125
      const BUser* user = gUserManager.getPrimaryUser();
//--
      if (user && (user->getSigninState()!=eXUserSigninState_NotSignedIn))  //user->isSignedIn())
      {
         //Check for updating score
         if (gWorld && (user->getPlayerID()!=-1) && gScoreManager.arePlayersInitialized())
         {
            //Score updates on a different refresh time then the other presence values
            if ((timeGetTime() - mLastPresenceScoreUpdate) > cBLiveSystemPresenceScoreRefreshTime)
            {
               //Set the current time as the last score update time
               mLastPresenceScoreUpdate = timeGetTime();

               //Get the score value we are going to update
               if( gWorld->getFlagGameOver() )
               {
                  mPresenceScoreValue = (uint64)gScoreManager.getFinalScore(user->getPlayer()->getID());
               }
               else
               {
                  mPresenceScoreValue = (uint64)gScoreManager.getBaseScore(user->getPlayer()->getID());
               }

               //Set the new score value and mark that we have an async operation pending
               XUserSetPropertyEx(user->getPort(), PROPERTY_GAMESCORE, 8, &mPresenceScoreValue, &mPresenceOverlap);
               mPresenceIsOperationPending = true;
               return;
            }
         }

         //Set any changed context states
         if (mCurrentPresenceIndex>=cLiveSystemMaxContexts)
         {
            mCurrentPresenceIndex=0;
         }
         DWORD j = mCurrentPresenceIndex;
         for(;;)
         //for (int32 j=0;j<cLiveSystemMaxContexts;j++)
         {
            if (mPresenceContext[j].mDirty)
            {
               //Save the value we are going to update to it's own member variable, since mPresenceContext[j].mContextValue
               //could theoretically be modified while our async operation is occuring
               mPresenceValue = mPresenceContext[j].mContextValue;

               //Set the new property or context value
               if (mPresenceContext[j].mProperty)
               {
                  XUserSetPropertyEx(user->getPort(), mPresenceContext[j].mContextType, sizeof(mPresenceValue), &mPresenceValue, &mPresenceOverlap);
               }
               else
               {
                  XUserSetContextEx(user->getPort(), mPresenceContext[j].mContextType, mPresenceValue, &mPresenceOverlap);
               }

               //Mark that we have an async operation pending
               mPresenceIsOperationPending = true;

               //Clear the dirty flag since we've taken care of this update
               mPresenceContext[j].mDirty = false;

               //Can't make any more updates untill our async operation is done, so exit
               mCurrentPresenceIndex=j+1;
               return;
            }
            j++;
            if (j>=cLiveSystemMaxContexts)
            {
               j=0;
            }
            if (j==mCurrentPresenceIndex)
            {
               break;
            }
         }
      }
      else
      {
         //There is no primary user here - we need to dump all the current cached values
         for (int32 i=0;i<cLiveSystemMaxContexts;i++)
         {
            mPresenceContext[i].reset();
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
XUID BLiveSystem::getMachineId()
{
   static XUID machineId = 0;

   // the machineId might not have been resolved yet (no xnaddr)
   // so make the system calls to get it if it hasn't been
   // determined. If it has, don't pester the system since it will
   // simply return the same value each time.
   if(machineId == 0)
   {
      XNADDR addr;
      if (XNetGetTitleXnAddr(&addr) != XNET_GET_XNADDR_PENDING)
      {
         XNetXnAddrToMachineId(&addr, &machineId);
      }
   }
   return machineId;
}

//==============================================================================
//
//==============================================================================
void BLiveSystem::postUpdatedVersionToBaseHopperList()
{
   delete mpHopperList;
   mpHopperList = mpHopperListUpdatedVersion;
   mpHopperListUpdatedVersion = new BMatchMakingHopperList;
}

//==============================================================================
// Loads in the xml file with the sets of match making data
//==============================================================================
void BLiveSystem::loadMatchMakingHoppers()
{
   BXMLReader reader;
   if(!reader.load(cDirData, "mpGameSetsBase.xml", NULL))
   {
      BASSERTM(false, "BLiveSystem::loadMatchMakingHoppers - could not load mpGameSetsBase.xml!!!");
      nlog(cMPGameCL, "BLiveSystem::loadMatchMakingHoppers - no game set file (mpGameSetsBase.xml) found");
      return;
   }
   BXMLNode rootNode(reader.getRootNode());
   getHopperList()->clear();
   getHopperList()->loadFromXML(rootNode, true);
}
