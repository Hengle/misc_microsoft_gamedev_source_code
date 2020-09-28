//==============================================================================
// mpGameSession.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "Common.h"
#include "mpGameSession.h"

// xgame
#include "gamesettings.h"
#include "database.h"

#include "LiveSystem.h"
#include "XLastGenerated.h"            //For the TitleID
#include "liveSession.h"
#include "liveVoice.h"
#include "ModePartyRoom2.h"

#include "MaxSendSize.h"
#include "Channel.h"
#include "Channels.h"
#include "OrderedChannel.h"    
#include "mpSimDataObject.h"

#include "notification.h"
#include "mpcommheaders.h"
#include "commlog.h"

#include "statsManager.h"
#include "humanPlayerAITrackingData.h"

#include "usermanager.h"
#include "user.h"
#include "campaignmanager.h"

//xSystem
#include "config.h"
#include "econfigenum.h"

//==============================================================================
// 
//==============================================================================
BMPGameSession::BMPGameSession(BMPSession* mpSession, BDataSet* pSettings) :
   mState(cBMPGameSessionStateNone),
   mpMPSession(mpSession),
   mpLiveSession(NULL),
   mPort(0),
   mpSession(NULL),
   mLocalChecksum(NULL),
   mpSimObject(NULL),
   mpGameSettings(NULL),
   //mGameConnected(false),
   //mSessionConnected(FALSE),
   mLocalControllerID(0),
   mPublicSlots(0),
   mPrivateSlots(0),
   mLocalPlayerID(cMPInvalidPlayerID),
   mStartupMode(BMPSession::mpSessionStartupNone),
   mGameTypeIndex(0),
   mLockedPlayers(0),
   mQoSResponseDataSize(0),
   mLaunchCountdown(0),
   mLaunchLastUpdate(0),
   mLaunchUpdateCounter(0),
   mJoinTimer(0),
   mRanked(FALSE),
   mLanguageCode(0),
   mQoSResponding(false),
   mLanSecurityKeyRegistered(false),
   mSentSessionDisconnected(false),
   mSettingsLocked(false),
   mSettingsComplete(false),
   mGameValid(false)
{
   BASSERT(mpSession);
   BASSERT(pSettings);

#ifndef BUILD_FINAL
   long value=0;
   if (gConfig.get(cConfigMMFilterHackCode, &value))
   {
      mLanguageCode=(uint8)value;
      nlog(cMPMatchmakingCL, " BMPGameSession::BMPGameSession - config has specified a matchmaking hack code of %d", mLanguageCode);
   }
#endif

   //Hook up the settings
   mpGameSettings = pSettings;
   mpGameSettings->addDataListener(this);

   Utils::FastMemSet(&mLocalXnKey, NULL, sizeof(mLocalXnKey));
   Utils::FastMemSet(&mLocalXNKID, NULL, sizeof(mLocalXNKID));
   Utils::FastMemSet(&mChannelArray, NULL, sizeof(mChannelArray));
   Utils::FastMemSet(&mQoSResponseData, NULL, sizeof(mQoSResponseData));
   Utils::FastMemSet(&mPendingPlayers, NULL, sizeof(mPendingPlayers));

   mpMPSession->getGameInterface()->setupSync(this);

   registerMPCommHeaders();

   IGNORE_RETURN(Utils::FastMemSet(mMachineFinalize, 0, sizeof(mMachineFinalize)));
}

//==============================================================================
//
//==============================================================================
BMPGameSession::~BMPGameSession()
{
   setState(cBMPGameSessionStateDestructed);
}

//==============================================================================
// 
//==============================================================================
//Called to gracefully shutdown from any current state, 
void BMPGameSession::shutDown()
{
   setState(cBMPGameSessionStateShutdownRequested);
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::processShutDown()
{
   //Make this so it is safe to call AT ANY POINT, even on a callback from BSession
   // Thus - it is going to just have to set a state here, then do the actual releases on the next update
   if (mState >= cBMPGameSessionStateShuttingDown)
   {
      nlog(cMPGameCL, "BMPGameSession::processShutDown(1) - Waiting for live session to shutdown.");
      if (mpLiveSession)
      {
         mpLiveSession->update();
         if (!mpLiveSession->isShutdown())
            return;

         nlog(cMPGameCL, "BMPGameSession::processShutDown - live session shutdown and deleted, game session is now ready for delete");
         delete mpLiveSession;
         mpLiveSession = NULL;
      }

      setState(cBMPGameSessionStateReadyForDelete);
      return;
   }

   if (!mSentSessionDisconnected)
   {
      nlog(cMPGameCL, "BMPGameSession::processShutDown - I never sent a disconnect message to my observer (probably because I never connected) so sending one now");
      mSentSessionDisconnected = true;
      mpMPSession->gameSessionDisconnected(BSession::cFailedClientConnect);
   }

   setState(cBMPGameSessionStateShuttingDown);

   if (mpMPSession->getGameInterface())
   {
      mpMPSession->getGameInterface()->setupSync(NULL);
   }

   if (mpSession)
   {
      if (isHosting())
      {
         mpSession->disconnect(BSession::cHostCancelledGame);
      } 
      else
      {
         mpSession->disconnect(BSession::cNormal);
      }
      //Give the session a chance to dump out any events it has left
      mpSession->service();
      //Drop the listener and then the object
      mpSession->removeObserver(this);
      mpSession->dispose();
      HEAP_DELETE(mpSession, gNetworkHeap);
      mpSession = NULL;
      nlog(cMPGameCL, "BMPGameSession::processShutDown - Network session for game is shutdown and deleted");
   }

   if (mpGameSettings)
   {
      mpGameSettings->removeDataListener(this);
      mpGameSettings = NULL;
   }

   destroyChannels();

   //LAN mode key management
   if (mLanSecurityKeyRegistered)
   {
      XNetUnregisterKey( &mLocalXNKID );
      mLanSecurityKeyRegistered = false;
   }

   if (mpLiveSession)
   {
      nlog(cMPGameCL, "BMPGameSession::processShutDown(2) - Waiting for live session to shutdown.");
      mpLiveSession->deinit();
      if (!mpLiveSession->isShutdown())
         return;

      nlog(cMPGameCL, "BMPGameSession::processShutDown(2) - live session shutdown and deleted, game session is now ready for delete");
      delete mpLiveSession;
      mpLiveSession = NULL;
   }

   setState(cBMPGameSessionStateReadyForDelete);
}

//==============================================================================
// 
//==============================================================================
//Call this to see if it is safe to be deleted
bool BMPGameSession::isShutDown()
{
   return (mState == cBMPGameSessionStateReadyForDelete);
}

//==============================================================================
// 
//==============================================================================
bool BMPGameSession::hostStartupLAN(DWORD controllerID, const BSimString& gameName, DWORD gameType, UINT iPublicSlots, UINT iPrivateSlots)
{
   BASSERT(mState==cBMPGameSessionStateNone);

   if (!mpMPSession->isInLANMode())
   {
      BASSERTM(false, "ERROR:LAN host but not in LAN mode");
      return false;
   }

//-- FIXING PREFIX BUG ID 1413
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return false;

   nlog(cMPGameCL, "BMPGameSession::hostStartupLAN - Cont:%d, Type:%d, pubslots:%d, privslots:%d", controllerID, gameType, iPublicSlots, iPrivateSlots);

   //Set some variables
   mRanked = false;
   mLocalControllerID = controllerID;
   mGameTypeIndex = gameType;
   mPublicSlots = iPublicSlots;
   mPrivateSlots = iPrivateSlots;

   //Setup the client map list
   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      mClientPlayerMap[i].mClientID = cMPInvalidClientID;
      mClientPlayerMap[i].mPlayerID = i+1;
      mClientPlayerMap[i].mLiveEnabled = true;
      if (mClientPlayerMap[i].mpTrackingDataBlock != NULL)
      {
         delete(mClientPlayerMap[i].mpTrackingDataBlock);
         mClientPlayerMap[i].mpTrackingDataBlock = NULL;
      }
   }

   //Generate a local key pair
   if (XNetCreateKey(&mLocalXNKID, &mLocalXnKey) != 0)
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupLocal -- Failed, could not create key - Last Error: %d", GetLastError());
      return false;
   }
   if (XNetRegisterKey(&mLocalXNKID, &mLocalXnKey) != 0)
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupLocal -- Failed, could not register key - Last Error: %d", GetLastError());
      return false;
   }

   mLanSecurityKeyRegistered = true;

   //Make sure we have a game interface
   if (!mpMPSession->getGameInterface())
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupLocal -- Failed, mpSession has no game interface");
      return false;
   }
   mLocalChecksum = mpMPSession->getCachedGameChecksum();

   //Get our local machine's XNADDR
   if (!gLiveSystem->getLocalXnAddr(mHostXnAddr))
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupLocal -- Failed, machine had no XNADDR");
      return false;
   }

   BDEBUG_ASSERT(mpSession == NULL);
   if (mpSession != NULL)
   {
      mpSession->dispose();
      HEAP_DELETE(mpSession, gNetworkHeap);
   }

   //Start this session up
   mPort = cMPSessionLANHostingPort;
   mpSession = HEAP_NEW(BSession, gNetworkHeap);
   // XXX when voice goes threaded, we need to pass in it's event handle here
   mpSession->init(mLocalChecksum, this, this, gLiveSystem->getLiveVoice());
   mpSession->addObserver(this);
   mpSession->setLocalXnAddr(mHostXnAddr);
   mpSession->setMaxClientCount(mPublicSlots);
   mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

   gLiveSystem->getLiveVoice()->initSession(BVoice::cGameSession, mpSession->getConnectionEventHandle());

   //Start the session hosting
   HRESULT hr = mpSession->host(mLocalXNKID, mPort);
   BASSERT( hr==S_OK );
   if (hr!=S_OK)
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupLocal -- Failed, BSession/socket error");
      return false;
   }

   //Setup all the channels I need
   setupChannels();

   //Wait for the session layer to say its connected - then we are ready
   nlog(cMPGameCL, "BMPGameSession::hostStartupLAN -- Hosting is ready, waiting for self connection");
   setState(cBMPGameSessionStateLaunchHostWaitingForSelfConnection);

   return true;
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::hostStartupLive( DWORD controllerID, DWORD gameType, BOOL ranked, UINT iPublicSlots, UINT iPrivateSlots  )
{
   BASSERT(mState==cBMPGameSessionStateNone);
   BASSERT(!mpMPSession->isInLANMode());

   nlog(cMPGameCL, "BMPGameSession::hostStartupLive - Cont:%d, Type:%d, pubslots:%d, privslots:%d", controllerID, gameType, iPublicSlots, iPrivateSlots);

   //Set some variables
   setState(cBMPGameSessionStateLaunchHostWaitingForSessionCreation);
   mRanked = ranked;
   mGameTypeIndex = gameType;
   mPublicSlots = iPublicSlots;
   mPrivateSlots = iPrivateSlots;
   mPort = cMPSessionLiveHostingPort;
   mLocalControllerID = controllerID;

   //Setup the client map list
   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      mClientPlayerMap[i].mClientID = cMPInvalidClientID;
      mClientPlayerMap[i].mPlayerID = i+1;
      mClientPlayerMap[i].mLiveEnabled = true;
      if (mClientPlayerMap[i].mpTrackingDataBlock != NULL)
      {
         delete(mClientPlayerMap[i].mpTrackingDataBlock);
         mClientPlayerMap[i].mpTrackingDataBlock = NULL;
      }
   }

   //Create a LIVE session and post it up
   BASSERT(mpLiveSession==NULL);
   //Constructor for starting a Live matchmaking session
   mpLiveSession = new BLiveSession( mLocalControllerID, iPublicSlots+iPrivateSlots, mGameTypeIndex, mRanked );

   //All done - next update loop will see if we are ready to continue host processing
   return true;
}

//==============================================================================
// 
//==============================================================================
bool BMPGameSession::joinLanGame(const BLanGameInfo& lanInfo)
{
   BASSERT(mpMPSession->getGameInterface());

   XNADDR localXnAddr;
   if (!gLiveSystem->getLocalXnAddr(localXnAddr))
   {
      return false;
   }

//-- FIXING PREFIX BUG ID 1414
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return false;

   nlog(cMPGameCL, "BMPGameSession::joinLanGame");

   //Cache the checksum for this current running game
   mLocalChecksum = mpMPSession->getCachedGameChecksum();

   mPort = cMPSessionLANHostingPort;

   BDEBUG_ASSERT(mpSession == NULL);
   if (mpSession != NULL)
   {
      mpSession->dispose();
      HEAP_DELETE(mpSession, gNetworkHeap);
   }

   mpSession = HEAP_NEW(BSession, gNetworkHeap);
   // XXX when voice goes threaded, we need to pass in it's event handle here
   mpSession->init(mLocalChecksum, this, this, gLiveSystem->getLiveVoice());
   mpSession->addObserver(this);
   mpSession->setLocalXnAddr(localXnAddr);
   mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

   gLiveSystem->getLiveVoice()->initSession(BVoice::cGameSession, mpSession->getConnectionEventHandle());

   //Setup the client map list
   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      mClientPlayerMap[i].mClientID = cMPInvalidClientID;
      mClientPlayerMap[i].mPlayerID = i+1;
      mClientPlayerMap[i].mLiveEnabled = true;
      if (mClientPlayerMap[i].mpTrackingDataBlock != NULL)
      {
         delete(mClientPlayerMap[i].mpTrackingDataBlock);
         mClientPlayerMap[i].mpTrackingDataBlock = NULL;
      }
   }

   mHostXnAddr = lanInfo.getXnAddr();
   mLocalXNKID = lanInfo.getXnKID();
   mLocalXnKey = lanInfo.getXnKey();
   mNonce = lanInfo.getNonce();
   //mGameTypeIndex = Currently this is NOT passed around in the LAN descriptor because we only support ONE type of game (CUSTOM) for lan games

   //LAN mode key management
   if (mLanSecurityKeyRegistered)
   {
      XNetUnregisterKey(&mLocalXNKID);
      mLanSecurityKeyRegistered = false;
   }

   if (XNetRegisterKey(&mLocalXNKID, &mLocalXnKey) != 0)
   {
#ifndef BUILD_FINAL
      DWORD lastErr = GetLastError();
      nlog(cMPGameCL, "BMPGameSession::join -- Failed, could not register key - Last Error: %d", lastErr);
#endif
      return false;
   }
   mLanSecurityKeyRegistered = true;

   if (!mpSession->join(mHostXnAddr, mPort, mLocalXNKID))
   {
      XNetUnregisterKey(&mLocalXNKID);
      mLanSecurityKeyRegistered = true;
      return false;
   }

   //Setup all the channels we will need
   setupChannels();

   mJoinTimer = timeGetTime();
   setState(cBMPGameSessionStateJoinWaitingForConnection);

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BMPGameSession::joinLiveGame( DWORD controllerID, BLiveGameDescriptor* gameDescriptor)
{
   BASSERT( mpMPSession->getGameInterface());

   XNADDR localXnAddr;
   if (!gameDescriptor || !gLiveSystem->getLocalXnAddr(localXnAddr) )
   {
      return false;
   }

//-- FIXING PREFIX BUG ID 1415
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return false;

   nlog(cMPGameCL, "BMPGameSession::joinLiveGame");

   //Cache the checksum for this current running game
   mLocalChecksum = mpMPSession->getCachedGameChecksum();
   mLocalXNKID = gameDescriptor->getXNKID();
   mLocalXnKey = gameDescriptor->getXNKEY();
   mHostXnAddr = gameDescriptor->getXnAddr();
   mNonce = gameDescriptor->getNonce();
   mRanked = gameDescriptor->getRanked();
   mGameTypeIndex = gameDescriptor->getGameModeIndex();
   mPublicSlots = gameDescriptor->getSlots();
   mPrivateSlots = 0;
   mPort = cMPSessionLiveHostingPort;
   mLocalControllerID = controllerID;

   //Setup the client map list
   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      mClientPlayerMap[i].mClientID = cMPInvalidClientID;
      mClientPlayerMap[i].mPlayerID = i+1;
      mClientPlayerMap[i].mLiveEnabled = true;
      if (mClientPlayerMap[i].mpTrackingDataBlock != NULL)
      {
         delete(mClientPlayerMap[i].mpTrackingDataBlock);
         mClientPlayerMap[i].mpTrackingDataBlock = NULL;
      }
   }

   BDEBUG_ASSERT(mpSession == NULL);
   if (mpSession != NULL)
   {
      mpSession->dispose();
      HEAP_DELETE(mpSession, gNetworkHeap);
   }

   //Start this session up
   mPort = cMPSessionLiveHostingPort;
   mpSession = HEAP_NEW(BSession, gNetworkHeap);
   // XXX when voice goes threaded, we need to pass in it's event handle here
   mpSession->init(mLocalChecksum, this, this, gLiveSystem->getLiveVoice());
   mpSession->addObserver(this);
   mpSession->setLocalXnAddr(localXnAddr);
   mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

   gLiveSystem->getLiveVoice()->initSession(BVoice::cGameSession, mpSession->getConnectionEventHandle());

   //Live game session management (which auto-manages the key)
   //Create a LIVE session and post it up
   BDEBUG_ASSERT(mpLiveSession == NULL);
   mpLiveSession = new BLiveSession(mLocalControllerID, mPublicSlots, mGameTypeIndex, mRanked, mNonce, mHostXnAddr, mLocalXNKID, mLocalXnKey);

   //Now we need to wait for that session to be approved by Live
   setState(cBMPGameSessionStateJoinWaitingForLiveSession);

   return true;
}

//==============================================================================
// Creates all the channels needed by the game
//==============================================================================
void BMPGameSession::setupChannels()
{
   BDEBUG_ASSERT(!mpSimObject);
   BDEBUG_ASSERT(getSession());

   mpSimObject = HEAP_NEW(BMPSimDataObject, gNetworkHeap);
   mpSimObject->init(this);

   // the Command and Sim channels will require us to synchronize their contents
   // with other clients should someone disconnect, otherwise we could potentially miss a piece
   // of information causing us to go OOS when the game resumes
   //
   // Note: timing information used to synchronize but we're replacing that system with
   // this general channel synchronization

   // Command channel
   mChannelArray[cChannelCommand] = HEAP_NEW(BPeerOrderedChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelCommand]);
   mChannelArray[cChannelCommand]->init(BChannelType::cCommandChannel, getSession(), true);
   BASSERT(mChannelArray[cChannelCommand]);
   mChannelArray[cChannelCommand]->addObserver(this);

   // Sim channel
   mChannelArray[cChannelSim] = HEAP_NEW(BPeerOrderedChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelSim]);
   mChannelArray[cChannelSim]->init(BChannelType::cSimChannel, getSession(), true);
   mChannelArray[cChannelSim]->addObserver(this);

   //Sync channel
   mChannelArray[cChannelSync] = HEAP_NEW(BPeerOrderedChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelSync]);
   mChannelArray[cChannelSync]->init(BChannelType::cSyncChannel, getSession());
   mChannelArray[cChannelSync]->addObserver(this);

   //Vote channel
   mChannelArray[cChannelVote] = HEAP_NEW(BChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelVote]);
   mChannelArray[cChannelVote]->init(BChannelType::cVoteChannel, getSession());
   mChannelArray[cChannelVote]->addObserver(this);

   //Message channel
   mChannelArray[cChannelMessage] = HEAP_NEW(BPeerOrderedChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelMessage]);
   mChannelArray[cChannelMessage]->init(BChannelType::cMessageChannel, getSession());
   mChannelArray[cChannelMessage]->addObserver(this);

   //Settings channel
   mChannelArray[cChannelSettings] = HEAP_NEW(BPeerOrderedChannel, gNetworkHeap);
   BDEBUG_ASSERT(mChannelArray[cChannelSettings]);
   mChannelArray[cChannelSettings]->init(BChannelType::cSettingsChannel, getSession());
   mChannelArray[cChannelSettings]->addObserver(this);
}

//==============================================================================
//This processes the startup of a hosting session
//  It is state driven as at several points (LAN or Live) the system must wait for events before continuing on
//  Generally the UPDATE method is looking for those events and then calling this to continue once those conditions have been met
//==============================================================================
void BMPGameSession::hostStartupProcessing()
{

   if (!mpMPSession->getSessionInterface())
   {
      nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, no session interface");
      mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultInternalError);
      return;
   }

   if (mState == cBMPGameSessionStateLaunchHostWaitingForSessionCreation)
   {
      if (!mpLiveSession )
      {
         //No Live session - complete failure here 
         nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, no live session");
         mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultLiveError);
        return;
      }
      if (mpLiveSession->isSessionValid())
      {
//-- FIXING PREFIX BUG ID 1416
         const BUser* pUser = gUserManager.getPrimaryUser();
//--
         if (!pUser)
            return;

         //Live session has been created and is connected
         mNonce = mpLiveSession->getNonce();

         //Check the checksum for this current running game
         mLocalChecksum = mpMPSession->getCachedGameChecksum();

         //Get our local machine's XNADDR as reported by the session
         if (!mpLiveSession->getSessionHostXNAddr(mHostXnAddr))
         {
            //No Live session local xnaddr - complete failure here
            nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, could not query live session for XNADDR");
            mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultLiveError);
            return;
         }

         if (!mpLiveSession->getXNKEY(mLocalXnKey))
         {
            //No Live session key ID - complete failure here
            nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, no live session XNKEY");
            mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultLiveError);
            return;
         }

         if (!mpLiveSession->getXNKID(mLocalXNKID))
         {
            //No Live session key - complete failure her
            nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, no live session XNKID");
            mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultLiveError);
            return;
         }

         BDEBUG_ASSERT(mpSession == NULL);
         if (mpSession != NULL)
         {
            mpSession->dispose();
            HEAP_DELETE(mpSession, gNetworkHeap);
         }

         //Start this session up
         mPort = cMPSessionLiveHostingPort;
         mpSession = HEAP_NEW(BSession, gNetworkHeap);
         // XXX when voice goes threaded, we need to pass in it's event handle here
         mpSession->init(mLocalChecksum, this, this, gLiveSystem->getLiveVoice());
         mpSession->addObserver(this);
         mpSession->setLocalXnAddr(mHostXnAddr);
         mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

         gLiveSystem->getLiveVoice()->initSession(BVoice::cGameSession, mpSession->getConnectionEventHandle());

         //Start the session hosting
         nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- Starting local host BSession");
         HRESULT hr = getSession()->host(mLocalXNKID, mPort);   //Just name the session the same as the host's name for now
         BASSERT( hr==S_OK );
         if (FAILED(hr))
         {
            nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- ERROR: Aborted, failed host creation with code %d", hr);
            mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultSessionError);
            return;
         }

         //Setup all the channels I need
         setupChannels();

         //Start QoS hosting about this session
         setQoSNotification(true);

         //Tell mpsession we are ready

         //This must be for custom game room  - let it know
         //mState = cBMPGameSessionStateReady;
         //mpMPSession->gameSessionHostStarted();   

         //Wait for the session layer to say its connected - then we are ready
         nlog(cMPGameCL, "BMPGameSession::hostStartupProcessing -- Hosting is ready, waiting for self connection");
         setState(cBMPGameSessionStateLaunchHostWaitingForSelfConnection);
      }
   }
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::destroyChannels()
{
   for (int i=0; i < cChannelMax; i++)
   {
      if (mChannelArray[i])
      {
         mChannelArray[i]->removeObserver(this);
         mChannelArray[i]->dispose();
         HEAP_DELETE(mChannelArray[i], gNetworkHeap);
         mChannelArray[i] = NULL;
      }   
   }
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::update()
{
   //No updates if we are ending
   if (mState>=cBMPGameSessionStateShutdownRequested)
   {
      processShutDown();
      return;
   }

   if (mpLiveSession)
   {
      mpLiveSession->update();
      if (mpLiveSession->sessionHasError())
      {
         //Session crashed - this game fails
         nlog(cMPGameCL, "BMPGameSession::update --LiveSession reports that it is in state ERROR, killing game session");
         shutDown();
      }
   }

   //Give the BSession a chance to update
   if (getSession())
   {
      getSession()->service();
   }

   //Session end events can cause this object to be shutdown (but not deleted, so they are safe) but we need to stop processing if that happens
   if (mState>=cBMPGameSessionStateShutdownRequested)
      return;

   //Service each channel
   for (long idx=0; idx<cChannelMax; idx++)
   {
      if (mChannelArray[idx])
      {
         mChannelArray[idx]->service();
      }
   }

   //State dependent internal updates *************************************************
   switch (mState)
   {
      case (cBMPGameSessionStateJoinWaitingForConnection) :
      {
         //Timeout trap in case join request never returns
         //Dropping this trap - completely relying on BSession raising a cEventJoinFailed if it cannot connect - eric
         /*
         if (timeGetTime() - mJoinTimer > cMPSessionConnectTimeout)
         {
            //We have waited too long for this to happen - bail
            nlog(cMPGameCL, "BMPGameSession::update -- Initial connection took too long [%d ms], we are bailing.", (timeGetTime() - mJoinTimer ));
            //Note - we need to manually trigger the callback that the session has dropped because one will not fire otherwise (because it was never connected)
            sessionDisconnected(BSession::cFailedClientConnect);
            shutDown();
            return;
         }
         */
         break;
      }
      case (cBMPGameSessionStateReady):
      case (cBMPGameSessionStateJoinWaitingForInitialSetup):
         {
            //Update for each of the pending player connections if we are in pre-game
            updatePendingPlayers();

            //Are we ready to start the game?
            if (checkAllPlayersInitialized())
            {
               // if I'm hosting, I need to fire gameSessionHostStarted to tell everyone else
               // to get going but I only want to fire that off once I'm fully connected into the session
               //
               // once the host is fully connected I can inform my party that they should also begin connecting
               //
               // once the remainder of my party connects and all players are initialized, they will call gameSessionConnected()
               mpMPSession->gameSessionReady();

               setState(cBMPGameSessionStateGameSetup);
            }

            //if (getAreAllPlayersReadyToStart())
            //{
            //   //System is now hooked into the SAME system that is used from the interface when the host 'greens up'
            //   requestGameLaunch(cMPSessionDefaultGameLaunchCountdown); 
            //}
            break;
         }

      case cBMPGameSessionStateGameSetup:
         {
            if (getAreAllPlayersReadyToStart())
            {
               //System is now hooked into the SAME system that is used from the interface when the host 'greens up'
               requestGameLaunch(cMPSessionDefaultGameLaunchCountdown); 
            }
            break;
         }

      case (cBMPGameSessionStateLaunchHostWaitingForSessionCreation) :
         {
            //LIVE game hosting - waiting for the Live-side session to be created and joined
            BASSERT(mpLiveSession);
            if (mpLiveSession->isSessionValid())
            {
               hostStartupProcessing();
            }
            break;
         }

   case (cBMPGameSessionStateWaitingOnArbitrationRegistration) :
      {
         if (!mpLiveSession)
         {
            //We were waiting on live session and it went away?
            nlog(cMPGameCL, "BMPGameSession::update -- We were waiting on the live session arbitration registration, but mpLiveSession is null.");
            BASSERT(false);
            return;
         }

         if (mpLiveSession->sessionHasError())
         {
            //BASSERT(false);
            nlog(cMPGameCL, "BMPGameSession::update -- We were waiting on the live session arbitration registration, but mpLiveSession reports that it is in state ERROR.");
            //Registration failed - generally this is from someone disconnecting during the startup/arbitration phase
            //For now (Alpha 3) just ditch and let matchmaking continue
            //TODO - we should be able to just go on with matchmaking really, look at post A3
            shutDown();
            return;
         }
         else if (mpLiveSession->sessionIsRegistered())
         {
            nlog(cMPGameCL, "BMPGameSession::update -- Live session arbitration registration is complete, getting ready to start level load");
            handleProcessLockSettings();
         }
         break;
      }

   case (cBMPGameSessionStateWaitingOnSecondaryArbitrationRegistration) :
      {
         if (!mpLiveSession)
         {
            //We were waiting on live session and it went away?
            nlog(cMPGameCL, "BMPGameSession::update -- We were waiting on the secondary live session arbitration registration, but mpLiveSession is null.");
            BASSERT(false);
            return;
         }

         if (mpLiveSession->sessionHasError())
         {
            //BASSERT(false);  No ASsert here - this is a valid condition if the arbitration fails
            nlog(cMPGameCL, "BMPGameSession::update -- We were waiting on the secondary live session arbitration registration, but mpLiveSession reports that it is in state ERROR.");
            shutDown();
            return;
         }
         else if (mpLiveSession->sessionIsRegistered())
         {
            nlog(cMPGameCL, "BMPGameSession::update -- Live session secondary arbitration registration is complete, getting ready to start level load");
            hostSendStartPreGame();
         }
         break;
      }

   case (cBMPGameSessionStateLaunching) :
      {
         // if we are launching, keep sending launch packets
         if (mLaunchCountdown > 0)
         {
            // only send one launch update for every second in the countdown
            // by default this starts at 3, so we'll be sending 3, 2, 1, 0
            if (mLaunchUpdateCounter >= 0)
            {
               // only send out updates once a second
               DWORD now = timeGetTime();
               if ((now - mLaunchLastUpdate) > 1000)
               {
                  BMessagePacket packet(mLaunchUpdateCounter*1000, BChannelPacketType::cLaunchUpdatePacket);
                  mChannelArray[cChannelMessage]->SendPacket(packet);

                  mLaunchUpdateCounter--;
                  mLaunchLastUpdate = now;
               }
            }
            return;
         }

         // if all players aren't locked, keep waiting
         // NOTE: ONLY the host processes the messages that increase this count
         //   So for everyone else mLockedPlayers is always ZERO
         if (mLockedPlayers < (DWORD)getPlayerCount())
            return;

         //Sanity check that we are in-fact, the game host - if not, bad if we got here because that means we got a lockedplayer++
         if (!isHosting())
         {
            //The only time this would get hit is if the host has left during the countdown - ie:badness, so we are done 
            nlog(cMPGameCL, "BMPGameSession::update - cBMPGameSessionStateLaunching, looks like host has disconnected");
            sessionDisconnected(BSession::cSessionTerminated);
            shutDown();
            return;
         }

         // if all players aren't ready to launch, keep waiting
         if (!getAreAllPlayersReadyToStart() )
         {
            return;
         }

         if (mpLiveSession && (mpLiveSession->hostOnlySecondaryRegisterForArbitration()))
         {
            setState(cBMPGameSessionStateWaitingOnSecondaryArbitrationRegistration);
         }
         else
         {
            hostSendStartPreGame();
         }
         //Moved this - only the host was ever calling it
         //mpMPSession->gameSessionGameLaunched();
         break;
      }

   case (cBMPGameSessionStateJoinWaitingForLiveSession) :
      {
         //I'm waiting for Live to authorize the session I'm trying to create (but I'm not hosting that session)
         BASSERT( getSession() );
         if ((!mpLiveSession) ||
             (mpLiveSession->sessionHasError()))
         {
            //There was a problem
            nlog(cMPMatchmakingCL, "BMPGameSession::update -- liveSession error");
            if (mpMPSession)
            {
               mpMPSession->gameSessionJoinFailed(BSession::cLiveSessionFailure);
            }
            //Why was this by-passing the mpSession management layer?  
            /*
            if (mpMPSession->getSessionInterface())
            {
               mpMPSession->getSessionInterface()->mpSessionEvent_gameSessionJoinFailed(BSession::cLiveSessionFailure);
            }
            */
            shutDown();
            return;
         }

         if (mpLiveSession->isSessionValid())
         {
            //Session has been validated by Live
            //Check the checksum for this current running game
            mLocalChecksum = mpMPSession->getCachedGameChecksum();
            mPort = cMPSessionLiveHostingPort;

            //Start this session up
            if (!getSession()->join(mHostXnAddr, mPort, mLocalXNKID))
            {
               //Immediate failure to connect
               nlog(cMPMatchmakingCL, "BMPGameSession::update -- join failed");
               shutDown();
               return;
            }

            //Setup all the channels we will need
            setupChannels();

            //Go ahead and start QoS response
            setQoSNotification(true);

            //Wait for the session layer to say its ready
            mJoinTimer = timeGetTime();
            setState(cBMPGameSessionStateJoinWaitingForConnection);    
         }
         break;
      }

   }
}

//==============================================================================
// 
//==============================================================================
BOOL BMPGameSession::isHosting() const
{
   if (!getSession())
      return FALSE;

   return (getSession()->isHosted());
}

//==============================================================================
// 
//==============================================================================
BOOL BMPGameSession::isRunning() const
{
   if (getSession() &&
       ((mState >= cBMPGameSessionStateReady)  &&
        (mState < cBMPGameSessionStateShutdownRequested)))
   {
      return TRUE;
   }
   return FALSE;
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::setState(BMPGameSessionState newState)
{
   //Safe way to set the state so that it never overrides a state of 'shutdown'
   if ((mState >= cBMPGameSessionStateShutdownRequested) &&
       (mState > newState))
   {
      //This will allow the state to be moved forward only once it is in shutting down mode (all modes after that one are steps towards being deleted)
      nlog(cMPGameCL, " BMPGameSession::setState - Ignoring new state, I am shutting down");
      return;
   }
   mState = newState;

   if (newState >= cBMPGameSessionStateLaunching && newState < cBMPGameSessionStateShutdownRequested)
      mGameValid = true;
}

//==============================================================================
//
//==============================================================================
uint64 BMPGameSession::getNonce() const 
{ 
   return (uint64)mNonce;
}


//==============================================================================
//
//==============================================================================
DWORD BMPGameSession::advanceGameTiming(void)
{
   if (!isRunning() || !getSimObject())
   {
      return(0);
   }
   return(getSimObject()->advanceGameTime());   
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::hostSendStartPreGame()
{
   BASSERT(isHosting());
   nlog(cMPGameCL, " BMPGameSession::hostSendStartPreGame - Host sending out the start pregame packet (which tells everyone to send profile data if needed");
   //Reset the data tracker for AItracking data, who has sent it, and who has loaded
   mLoadTracker.initializeMembersToSessionMembers(mpSession);
   //Set this to 1 if clients need to send AI profile data in 
   long clientsShouldSendProfileData = 0;
   //Are there AI involved? If so, set this to 1 and each machine will send in their data
   //Or if this is a campaign game - then send it
   long gametype;
   mpGameSettings->getLong(BGameSettings::cGameType, gametype);
   if (gametype == BGameSettings::cGameTypeCampaign)
   {
      clientsShouldSendProfileData = 1;
   }
   else
   {
      //Logic to detect AI needed or not here
      long playerCount = 0;
      mpGameSettings->getLong(BGameSettings::cMaxPlayers, playerCount);
      for (long i=1; i <= playerCount; i++)
      {
         long playerType;
         mpGameSettings->getLong(PSINDEX(i, BGameSettings::cPlayerType), playerType);
         if (playerType==BPlayer::cPlayerTypeComputerAI)
         {
            clientsShouldSendProfileData = 1;
            break;
         }
      }
   }

   //TODO - once we have a data block (and its size) that contains the pre-launch, game tuning data that we want to send round
   // Then set it here
   //NOTE - Empty size zero = no data here, however the transport packet code doesn't like NULL/0 so there is a 1 byte payload here
   //Further note: Thus any REAL payload will have to be larger than one byte or it will be ignored
   byte tuningData = 0;
   long tuningDataSize = 1;
   BPreLaunchDataPacket packet(BChannelPacketType::cPreLaunchDataHostDataPacket, clientsShouldSendProfileData, &tuningData, tuningDataSize);
   mChannelArray[cChannelMessage]->SendPacket(packet);

   setState(cBMPGameSessionStateStartPregame);
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::hostSendStartGame()
{
   BASSERT(isHosting());
   // send out the start game packet
   nlog(cMPGameCL, " BMPGameSession::hostSendStartGame - Host sending out the start game packet (which tells everyone to start level load");
   BChannelPacket packet(BChannelPacketType::cStartGamePacket);
   mChannelArray[cChannelMessage]->SendPacket(packet);

   setState(cBMPGameSessionStateStartPregame);
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::handleProcessLockSettings()
{
   setState(cBMPGameSessionStateLaunching);

   if (mpMPSession->getSessionInterface())
   {
      mpMPSession->getSessionInterface()->mpSessionEvent_launchStarted();
   }

   BChannelPacket response(BChannelPacketType::cSettingsLockedPacket);
   mChannelArray[cChannelMessage]->SendPacket(response);
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::handleClientData(const BSessionEvent* pEvent)
{
   if (pEvent == NULL)
      return;

   if (pEvent->mData2 == 0)
      return;

   const void* pData = (((char*)pEvent)+sizeof(BSessionEvent));

   switch (BTypedPacket::getType(pData))
   {
      case BPacketType::cGameFinalizePacket:
         {
            if (pEvent->mData1 < XNetwork::cMaxClients)
               mMachineFinalize[pEvent->mData1] = TRUE;

            mLoadTracker.setLoaded(pEvent->mData1, true);
            finalizeGame();
            break;
         }         
   }
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::handleSettingsPacket(BSettingsPacket &packet)
{
   // set locally, send to everyone else
   if (!mSettingsLocked
#ifndef BUILD_FINAL
      || packet.mOverrideLock
#endif
      )
   {
      nlog(cMPGameSettingsCL, "BMPGameSession::handleSettingsPacket -- settings not locked. ");
      if (!mpGameSettings->setData(packet.mIndex, packet.mData, (WORD)packet.mSize))
      {
         nlog(cMPGameSettingsCL, "BMPGameSession::handleSettingsPacket -- failed to call mpGameSettings->setData. ");
         return;
      }

      nlog(cMPGameSettingsCL, "BMPGameSession::handleSettingsPacket -- sending to all clients. setting index[%d], data[%d].", packet.mIndex, packet.mData);
      long count = getSession()->getMachineCount();
      for (long idx=0; idx < count; idx++)
      {
         if (getSession()->getHostMachineID() == idx)
            continue;

         BMachine* pMachine = getSession()->getMachine(idx);
         if (pMachine)
            mChannelArray[cChannelSettings]->SendPacketTo(pMachine, packet);
      }
   }            
}


//==============================================================================
//
//==============================================================================
void BMPGameSession::handleSettings(long machineID, const void* pData, long size)
{
   machineID;

   long type = BChannelPacket::getType(pData);
   switch (type)
   {
   case BChannelPacketType::cSettingsPacket:
      {
         //Process the packet
         BSettingsPacket packet;
         packet.deserializeFrom(pData, size);

         // this is a client sending a settings change to the host
         if (isHosting())
            handleSettingsPacket(packet);
         // else I am a client receiving a setting from the host
         // do not accept the setting until I've received the initial settings packet
         else if (mSettingsComplete)
         {
            nlog(cMPGameSettingsCL, "BMPGameSession::handleSettings -- setting change index[%d] data[%d].", packet.mIndex, packet.mData);
            mpGameSettings->setData(packet.mIndex, packet.mData, (WORD)packet.mSize);
         }
      }
      break;

      // XXX dangerous packet ID, should use a BChannelPacketType
      // FIXME post-alpha
   case BPacketType::cPlayerIndexPacket:
      {
         //Host already had this set up - he can ignore this message - the send should skip him anyways
         if (!isHosting())
         {
            //Crack it open
            BPlayerIDPacket packet;
            packet.deserializeFrom(pData, size);

            //Lets make sure we have a valid client for this playerID/XUID pair 
            //  IF we do not then we can ignore this because it is received in a reliable, ordered channel from the host
            //  And the session join (initpeers, connect peer) request comes before this 
            BDEBUG_ASSERTM(getSession(), "Missing BSession");
            if (getSession() == NULL)
               break;

            bool found = false;

            for (int i=0; i < getSession()->getClientCount(); ++i)
            {
//-- FIXING PREFIX BUG ID 1417
               const BClient* pClient = getSession()->getClient(i);
//--
               if (pClient != NULL &&
                   pClient->getXuid() == packet.mXuid &&
                   pClient->isConnected())
               {
                  found = true;
                  break;
               }
            }

            if (found)
            {
               nlog(cMPGameSettingsCL, "BMPGameSession::handleSettings -- processing player index packet, pid[%d], xuid[%I64u].", packet.mPlayerID, packet.mXuid);
               reservePlayerID(packet.mPlayerID, packet.mXuid);
            }
            else
            {
               nlog(cMPGameSettingsCL, "BMPGameSession::handleSettings -- Ignoring player index packet pid[%d], xuid[%I64u]", packet.mPlayerID, packet.mXuid);
            }
         }
      }
      break;

   case BChannelPacketType::cInitialSettingsPacket:
      {
         nlog(cMPGameSettingsCL, "BMPGameSession::handleSettings -- Initial Settings Packet.");
         handleCompleteSettings(type, pData, size);

         mSettingsComplete = true;

         if (!mpMPSession->getSessionInterface())
         {
            nlog(cMPMatchmakingCL, "BMPGameSession::handleSettings -- Initial Settings Packet but the session interface is gone");
            shutDown();
            return;
         }

         //mpMPSession->getSessionInterface()->mpSessionEvent_requestForSetLocalPlayerSettings();
         mpMPSession->getSessionInterface()->mpSessionEvent_initialSettingsComplete();

         if (!isHosting())
         {
            //I need to get the maxplayers out of the game settings
            long maxpeeps = 0;
            mpGameSettings->getLong(BGameSettings::cMaxPlayers, maxpeeps);
            mPublicSlots=(UINT)maxpeeps;
            
            //Are there session members connected that I need to hook up here?
            //long count = getSession()->getClientCount();
            //for (long idx=0; idx<count; idx++)
            //{
            //   BClient* p = getSession()->getClient(idx);
            //   if (p && p->isConnected())
            //   {                    
            //      addPendingPlayer(idx);
            //   }
            //}

            //Clients are now considered connected and ready for interaction with the session
            //mGameConnected = true;
            setState(cBMPGameSessionStateReady);
            //Lets push this notification off until updatePendingPlayers marks them as ready
            //mpMPSession->gameSessionConnected();

            // by the time I receive the initial settings packet, I will have creating all the pending players
            // and also received all their playerIDs
            //
            // players may disconnect but they cannot be replaced by someone else since this is now considered
            // a closed game session
            //
            // all that's left is to finish up processing the pending players list
         }
      }
      break;

   case BChannelPacketType::cFinalSettingsPacket:
      {
         nlog(cMPGameSettingsCL, "BMPGameSession::handleSettings -- Final Settings Packet.");
         handleCompleteSettings(type, pData, size);
      }
      break;
   }
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::handleMessage(long machineID, const void* pData, long size)
{

   switch (BChannelPacket::getType(pData))
   {
      case BChannelPacketType::cRequestSettingsPacket:
         {
            if (isHosting())
            {
               nlog(cMPGameSettingsCL, "BMPGameSession::handleMessage -- cRequestSettingsPacket process settings request.");

               BCompleteSettingsPacket spacket(BChannelPacketType::cInitialSettingsPacket);
               fillCompleteSettings(spacket);
               mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getMachine(machineID), spacket);
            }
         }
         break;

   case BChannelPacketType::cLockSettingsPacket:
      {         
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- lock settings.");
         BMessagePacket packet(BChannelPacketType::cLockSettingsPacket);
         packet.deserializeFrom(pData, size);

         mSettingsLocked = (packet.mMessage == 1);

         if (mSettingsLocked)
         {
            nlog(cMPGameCL, "BMPGameSession::handleMessage - Settings are locked");

            //Tell the live session to register for arbitration
            if (mpLiveSession && (mpLiveSession->registerForArbitration()))
            {
               setState(cBMPGameSessionStateWaitingOnArbitrationRegistration);
               nlog(cMPGameCL, "BMPGameSession::handleMessage - Waiting on Live Session to register for arbitration");
            }
            else
            {
               handleProcessLockSettings();
            }
         }
      }
      break;

   case BChannelPacketType::cSettingsLockedPacket:
      {
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- locked settings count.");
         if (isHosting())
         {
            mLockedPlayers++;
         }
      }
      break;

   case BChannelPacketType::cLaunchUpdatePacket:
      {
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- launch update.");

         BMessagePacket packet(BChannelPacketType::cLaunchUpdatePacket);
         packet.deserializeFrom(pData, size);
         if (mpMPSession->getSessionInterface())
         {
            mpMPSession->getSessionInterface()->mpSessionEvent_launchTimeUpdate((DWORD)packet.mMessage);
         }

         // we are done counting down
         if (packet.mMessage == 0 && isHosting())
            mLaunchCountdown = 0;
      }
      break;

   case BChannelPacketType::cLaunchReadyPacket:
      {
         BMessagePacket packet(BChannelPacketType::cLaunchReadyPacket);
         packet.deserializeFrom(pData, size);
         setReadyToStart(machineID, (packet.mMessage?true:false));
      }
      break;

   case BChannelPacketType::cLaunchAbortRequestPacket:
      {
         //Deprecated
         BASSERT(false);
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- launch abort request.");
         if (isHosting())
         {
            mSettingsLocked = false;

            BMessagePacket spacket(0, BChannelPacketType::cLockSettingsPacket);
            mChannelArray[cChannelMessage]->SendPacket(spacket);

            BMessagePacket rpacket(BChannelPacketType::cLaunchAbortRequestPacket);
            rpacket.deserializeFrom(pData, size);

            long messageData = 0;
//-- FIXING PREFIX BUG ID 1419
            const BMachine* pMachine = getSession()->getMachine(machineID);
//--
            if (pMachine)
               messageData = (pMachine->mUsers[0].mClientID << 16 & 0x00FF0000) | (pMachine->mUsers[1].mClientID & 0x000000FF);

            BMessageDataPacket packet(rpacket.mMessage, messageData, BChannelPacketType::cLaunchAbortPacket);
            mChannelArray[cChannelMessage]->SendPacket(packet);

            // unready the player that requested the abort
            setReadyToStart(machineID, false);

            // why am I switching to cBMPGameSessionStateReady from an abort?
            setState(cBMPGameSessionStateGameSetup);
         }
      }
      break;

   case BChannelPacketType::cLaunchAbortPacket:
      {
         //Deprecated
         BASSERT(false);

         nlog(cMPGameCL, "BMPGameSession::handleMessage -- launch aborted.");

         mLockedPlayers = 0;
         mLaunchCountdown = 0;
         mLaunchLastUpdate = 0;
         mLaunchUpdateCounter = 0;

         mLoadTracker.reset();
         resetStartState();

         BMessageDataPacket rpacket(BChannelPacketType::cLaunchAbortPacket);
         rpacket.deserializeFrom(pData, size);

         if (mpMPSession->getSessionInterface())
         {
            mpMPSession->getSessionInterface()->mpSessionEvent_launchAborted(getPlayerID(rpacket.mData >> 16 & 0xFF), getPlayerID(rpacket.mData & 0xFF), rpacket.mMessage);
         }

         // why am I switching to cBMPGameSessionStateReady from an abort?
         setState(cBMPGameSessionStateGameSetup);
         mpMPSession->gameSessionLaunchAborted();
      }
      break;

   case BChannelPacketType::cPreLaunchDataHostDataPacket:
      {
         //Let mpsession know we are launching
         mpMPSession->gameSessionGameLaunched();

         nlog(cMPGameCL, "BMPGameSession::handleMessage -- scPreLaunchDataHostDataPacket, host send pre-launch tuning data");   
         BPreLaunchDataPacket rpacket(BChannelPacketType::cPreLaunchDataHostDataPacket);
         rpacket.deserializeFrom(pData, size);
         if (size!=1)
         {
            //TODO - Here is where the processing of the pre-launch tuning data goes, accessed in
            nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataHostDataPacket - Processing tuning data of size [%d]", rpacket.mSize);
            //rpacket.mSize
            //rpacket.mData
               
         }
         if (rpacket.mPlayerID==1)
         {
            nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataHostDataPacket, host requests we send in AI profile data from all local clients");    
            //Lookup and send the primary player's data
            BUser* user = gUserManager.getPrimaryUser();
            if (user &&
                user->getProfile() && 
                user->getProfile()->getAITrackingDataMemoryPointer())
            {
//-- FIXING PREFIX BUG ID 1421
               const BClient* client = getSession()->getClientByXuid(user->getXuid());
//--
               if (client)
               {
//-- FIXING PREFIX BUG ID 1420
                  const BMPSessionPlayer* playerTrackingRecord = getPlayerFromNetworkClientId(client->getID());
//--
                  if (playerTrackingRecord && 
                      playerTrackingRecord->mPlayerID != cMPInvalidPlayerID)
                  {
                     void* pData = user->getProfile()->getAITrackingDataMemoryPointer();
                     //Pull the size out of the first 4 bytes
                     uint dataSize = *(reinterpret_cast<uint*>(pData));
                     BPreLaunchDataPacket primaryUserPacket(BChannelPacketType::cPreLaunchDataClientDataPacket, playerTrackingRecord->mPlayerID, pData, dataSize);
                     mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostMachine(), primaryUserPacket);
                     nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataHostDataPacket - Sent primary user tracking data of size [%d], playerID [%d]", primaryUserPacket.mSize, playerTrackingRecord->mPlayerID);
                  }
               }
            }

            //Lookup and send the secondary player's data (if there is one)
            /*
            user = gUserManager.getSecondaryUser();
            if (user &&
                user->getProfile() && 
                user->getProfile()->getAITrackingDataMemoryPointer())
            {
//-- FIXING PREFIX BUG ID 1423
               const BClient* client = getSession()->getClientByXuid(user->getXuid());
//--
               if (client)
               {
//-- FIXING PREFIX BUG ID 1422
                  const BMPSessionPlayer* playerTrackingRecord = getPlayerFromNetworkClientId(client->getID());
//--
                  if (playerTrackingRecord && 
                     playerTrackingRecord->mPlayerID != cMPInvalidPlayerID)
                  {
                     void* pData = user->getProfile()->getAITrackingDataMemoryPointer();
                     //Pull the size out of the first 4 bytes
                     uint dataSize = *(reinterpret_cast<uint*>(pData));
                     BPreLaunchDataPacket primaryUserPacket(BChannelPacketType::cPreLaunchDataClientDataPacket, playerTrackingRecord->mPlayerID, pData, dataSize);
                     mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostMachine(), primaryUserPacket);
                     nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataHostDataPacket - Sent secondary user tracking data of size [%d], playerID [%d]", primaryUserPacket.mSize, playerTrackingRecord->mPlayerID);
                  }
               }               
            }
            */
         }
         //Send a prelaunch data packet that is empty to indicate I'm done sending pre-launch packets
         byte oneByteNadaBuffer = 0;
         BPreLaunchDataPacket allDonePacket(BChannelPacketType::cPreLaunchDataClientDataPacket, 0, &oneByteNadaBuffer, 1);  
         mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostMachine(), allDonePacket);
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataHostDataPacket - Sent allDataSent");
         break;
      }

   case BChannelPacketType::cPreLaunchDataClientDataPacket:
      {
         //Process the incoming data
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataClientDataPacket - Recieved packet");
         BPreLaunchDataPacket rpacket(BChannelPacketType::cPreLaunchDataHostDataPacket);
         rpacket.deserializeFrom(pData, size);
         //If there is data in there - process it
         if (rpacket.mSize!=1)
         {
            //Store off this block of data to be accessed via the playerID later (during the level load process)
            BMPSessionPlayer* player = getPlayerFromPlayerId(rpacket.mPlayerID);
            BASSERT(player);
            if (!player)
               return;
            BASSERT(player->mpTrackingDataBlock == NULL);
            player->mpTrackingDataBlock = new byte[rpacket.mSize];
            BASSERT(player->mpTrackingDataBlock);
            Utils::FastMemCpy(player->mpTrackingDataBlock, rpacket.mData, rpacket.mSize);
            nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataClientDataPacket - got tracking data for player [%d]", rpacket.mPlayerID );
//             //Find the player
//             BPlayer* player = gWorld->getPlayer(rpacket.mPlayerID);
//             BASSERT(player);
//             BASSERT(player->getTrackingData());
//             if (player && player->getTrackingData())
//             {
//                //Sanity check on sizes here - the data is in there twice - but lets just check 
//                uint dataSize = *(reinterpret_cast<uint*>(pData));
//                BASSERT(dataSize==rpacket.mSize);
//                //This check skips the case where since the host echo's to itself - that it won't overwrite its own local data
//                if (!player->getTrackingData()->isLoaded())
//                {
//                   nlog(cMPGameCL, "BMPGameSession::handleMessage -- cPreLaunchDataClientDataPacket - got tracking data for player [%d}", player->getID() );
//                   player->getTrackingData()->loadValuesFromMemoryBlock(rpacket.mData, false);
//                }
//             }
         }
         if (isHosting())
         {
            //If I'm the host - echo it out to everyone (except myself) unless it is empty 
            if (rpacket.mSize == 1)
            {
               //This is the client letting me know he is DONE sending pre-launch data to me
               //Find them - mark at machine as complete and ready to go
               mLoadTracker.setPreLoadReady(machineID, true);
               //Call the method that checks if EVERYONE has sent this, also called in the update method so that it catches dropped players
               if (mLoadTracker.getAreAllPlayerPreLoadReady())
               {
                  hostSendStartGame();
               }
            }
            else
            {
               //Echo this to everyone - except the host, and except whoever send it
               long count = getSession()->getMachineCount();
               for (long i=0; i < count; ++i)
               {
                  BMachine* pMachine = getSession()->getMachine(i);
                  if (pMachine && pMachine->isConnected() && !pMachine->isLocal() && (i!=machineID))
                  {
                     mChannelArray[cChannelMessage]->SendPacketTo(pMachine, rpacket);
                  }
               }
            }  
         }

         break;
      }

   case BChannelPacketType::cStartGamePacket:
      {
         nlog(cMPGameCL, "BMPGameSession::handleMessage -- start game.");   

         //Debug spam
         preGameDebugSpam();

         if (mpLiveSession)
         {
            mpLiveSession->startGame();
         }

         //mLoadTracker.initializeMembersToSessionMembers(mpSession);

         if (!isHosting())
         {
            setState(cBMPGameSessionStateStartPregame);
         }

         if (mpMPSession->getSessionInterface())
         {
            mpMPSession->getSessionInterface()->mpSessionEvent_startGame();
         }
      }
      break;
   };

}

//==============================================================================
//
//==============================================================================
void BMPGameSession::handleCompleteSettings(long type, const void* data, long size)
{
   if (isHosting() || size == 0)
      return;

   nlog(cMPGameSettingsCL, "BMPGameSession::handleCompleteSettings -- enter.");

   BCompleteSettingsPacket packet(type);
   packet.deserializeFrom(data, size);
   if (packet.mSize == 0)
      return;

   DWORD count = 0;
   WORD wsize = 0;
   DWORD offset = 0;
   BYTE  *pData = (BYTE*)packet.mData;

   Utils::FastMemCpy(&count, &pData[offset], sizeof(count));
   offset += sizeof(count);

   // all the settings should be in this buffer. 
   if (mpGameSettings->getNumberEntries() != count)
   {
      nlog(cMPGameSettingsCL, "BMPGameSession::handleCompleteSettings -- Invalid number of settings sent - expected: %d, received: %d", mpGameSettings->getNumberEntries(), count);
      BASSERT(0);
      shutDown();
      return;
   }

   for (DWORD idx=0; idx<count; idx++)
   {
      // check again for a buffer overrun
      if ( (offset+sizeof(wsize)) > (DWORD)size)
      {
         // we didn't have enough room to read the size
         nlog(cMPGameSettingsCL, "BMPGameSession::handleCompleteSettings -- buffer overrun - index: %d.", idx);
         BASSERT(0);
         shutDown();
         return;
      }

      Utils::FastMemCpy(&wsize, &pData[offset], sizeof(wsize));
      offset += sizeof(wsize);

      // check again for a buffer overrun
      if ( (offset+wsize) > (DWORD)size)
      {
         // reading the buffer would have caused us to go over the end.
         nlog(cMPGameSettingsCL, "BMPGameSession::handleCompleteSettings -- buffer overrun - index: %d.", idx);
         BASSERT(0);
         shutDown();
         return;
      }

      if (wsize > 0)
      {
         mpGameSettings->setData(idx, &pData[offset], wsize);
         offset += wsize;
      }
   }
}

//==============================================================================
//Called when the level is done loading
//==============================================================================
void BMPGameSession::gameDoneLoading()
{
   BASSERT(mState==cBMPGameSessionStateStartPregame);
   if (!getSession())
   {
      //No network session
      nlog(cMPGameCL, "BMPGameSession::gameDoneLoading -- No network session!");
      sessionDisconnected(BSession::cSessionTerminated);
      shutDown();
      return;
   }

   nlog(cMPGameCL, "BMPGameSession::gameDoneLoading -- Sending packet to tell everyone i am done loading");
   BTypedPacket packet(BPacketType::cGameFinalizePacket);
   getSession()->SendPacket(packet); 
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::allMachinesFinalized() const
{
   if (mpSession == NULL)
      return true;

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      BMachine* pMachine = getSession()->getMachine(i);
      if (pMachine && pMachine->isConnected())
      {
         if (mMachineFinalize[i] == FALSE)
            return false;
      }
   }

   return true;
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::setSetting(DWORD index, void *data, long size)
{
   if (!mpGameSettings)
      return(false);

   if (mSettingsLocked)
      return(false);

   // isRunning was a bad check here because we can start to change/tweak
   //    some of our game settings and know they will propagate correctly
   // But outside of these states, it's a bad idea to mess with game settings because
   //    we're not up and running yet
   if ((mState < cBMPGameSessionStateJoinWaitingForInitialSetup ||
        mState >= cBMPGameSessionStateShutdownRequested) && !isHosting())
   {
      //Ok we have game settings, but we are not running yet - so just set the value but don't broadcast it out
      // this should not occur because if I set a value locally without informing the host
      // then the value will be lost when the host sends us the initial settings packet
      BDEBUG_ASSERTM(false, "Changing game settings while not running is a bad thing");
      //return(mpGameSettings->setData(index, data, (WORD)size));
      return false;
   }

   nlog(cMPGameCL, "BMPGameSession::setSetting -- requesting to change index[%d], data[0x%08X], size[%d]", index, data, size);

   // if we are hosting, our changes go out to everyone
   if (isHosting())
   {
      BSettingsPacket packet(index, data, size);
      handleSettingsPacket(packet);
   }
   // else send the packet to the host
   else
   {
      BSettingsPacket packet(index, data, size);
      if (mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getHostMachine(), packet) != S_OK)
         return(false);      
   }

   //if (mUpdateLocalSettings) //removing this check - it was ALWAYS true
   // changing locally to improve responsiveness of the value
   return (mpGameSettings->setData(index, data, (WORD)size));
}


//==============================================================================
//Call this to alter the public slot count
//==============================================================================
void BMPGameSession::setMaxPlayerCount(uint32 maxPlayers)
{
   if (mPublicSlots == maxPlayers)
   {
      //no change
      return;
   }

   //Host kicks anyone in slots beyond what we are changing it down to
   if (getSession() && (getSession()->isHosted()))
   {
      uint32 players = getSession()->getMaxClientCount();
      if (players > maxPlayers)
      {
         //We have too many players for the new number of slots
         //  First - pass of who to drop, lets go through the connected clients list 
         //    looking for those who's player index is > than the new maxPlayers.
         //    This is to try and kick those folks who are at the end of the game-managed player list
         long clientCount = getSession()->getClientCount();         
         for (long idx=clientCount-1; idx>-1; idx--)
         {
//-- FIXING PREFIX BUG ID 1403
            const BClient* p = getSession()->getClient(idx);
//--
            if (p && p->isConnected())
            {
               PlayerID pid = getPlayerID( idx );    
               if ((pid!=cMPInvalidPlayerID) &&
                  (pid > static_cast<int32>(maxPlayers)))
               {
                  //No need for this - it will happen when he disconnects
                  //mPlayerManagerInterface->releasePlayerID(pid);
                  getSession()->kickClient(idx);
               }
            }
         }
      }
   }

   setPublicSlots(maxPlayers);
   if (getSession())
   {
      getSession()->setMaxClientCount(maxPlayers);
   }
   updateBroadcastedHostData();
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::clientConnected(BClientID clientIndex, BMachineID machineID, BMachineID localMachineID, BMachineID hostMachineID, BOOL init)
{
//-- FIXING PREFIX BUG ID 1404
   const BClient* pClient = getSession()->getClient(clientIndex);
//--
   if (!pClient)
   {
      nlog(cMPGameCL, "BMPGameSession::clientConnected -- Failed to get client from session. ID[%d]", clientIndex);
      return;
   }

   BMachine* pFromMachine = getSession()->getMachine(machineID);
   if (!pFromMachine)
   {
      nlog(cMPGameCL, "BMPGameSession::clientConnected -- Failed to get machine from session. machineID[%d]", machineID);
      return;
   }

   // a new client connected, be sure that we clear their finalize settings
   if (machineID >= 0 && machineID < XNetwork::cMaxClients)
      mMachineFinalize[machineID] = FALSE;

   //Add them into the Live session if needed (if they are remote and its not LAN)
   if ((!mpMPSession->isInLANMode()) &&
      (pClient->getXuid() != mpMPSession->getLocalXUID()))
   {
      BASSERT(mpLiveSession);
      if (!mpLiveSession->addRemoteUserToSession(pClient->getXuid(), false))
      {
         //Add failed BMPGameSession...
         nlog(cMPGameCL, "BMPSession::clientConnected -- Could not add to live session - dropping them");
         getSession()->kickClient(clientIndex);
         getSession()->disconnectClient(clientIndex);
         return;
      }
   }

   // if the game is already connected, connect this player
   //if (mGameConnected)
   //{
   nlog(cMPGameCL, "BMPGameSession::clientConnected -- calling connect player clientID[%d]", clientIndex);

   if (isHosting())
   {
      //Ok - I'm the host and someone has just fully connected to the session

      //Lets get them a playerID
      PlayerID playerID = requestPlayerID(pClient->getID(), pClient->getXuid(), pClient->getGamertag());
      if (playerID == cMPInvalidPlayerID)
      {
         nlog(cMPGameCL, "BMPGameSession::clientConnected -- Failed to get locked player ID from game. [%d]", clientIndex);
         //Host slots must actually be full even through earlier he said it was ok - reject this connection
         getSession()->kickClient(clientIndex);
         getSession()->disconnectClient(clientIndex);
         return;
      }

      //Let their structures get hooked up here locally on the host
      addPendingPlayer(clientIndex, playerID);

      //Broadcast that playerID out to everyone
      //And send the new player everyone's current mapping
      nlog(cMPGameCL, "BMPGameSession::clientConnected -- Sending player index packet to everyone pid[%d], xuid[%I64u]",  playerID, pClient->getXuid());
      BPlayerIDPacket spacket(playerID, pClient->getXuid());
      long count = getSession()->getMachineCount();
      for (long i=0; i < count; ++i)
      {
         BMachine* pMachine = getSession()->getMachine(i);
         if (pMachine && pMachine->isConnected())
         {
            if (!pMachine->isLocal() && clientIndex != pMachine->mUsers[0].mClientID && clientIndex != pMachine->mUsers[1].mClientID)
            {
               //Send that client this new connector's info 
               //  - unless that client is the host (cause he already knows!)
               //  - unless that client is the NEW client (he will be told about himself down below)
               if (mChannelArray[cChannelSettings]->SendPacketTo(pMachine, spacket) != S_OK)
               {
                  //Todo - Examine issues from not being able to send to someone, have they dropped?
                  nlog(cMPGameCL, "BMPGameSession::clientConnected -- Could not send to client.");
               }
            }

            for (uint j=0; j < BMachine::cMaxUsers; ++j)
            {
               if (pMachine->mUsers[j].mXuid != INVALID_XUID)
               {
                  playerID = getPlayerID(pMachine->mUsers[j].mClientID);
                  if (playerID != cMPInvalidPlayerID)
                  {
                     BPlayerIDPacket spacketPerClient(playerID, pMachine->mUsers[j].mXuid);
                     if (mChannelArray[cChannelSettings]->SendPacketTo(pFromMachine, spacketPerClient) != S_OK)
                     {
                        //I can't send to the person i'm doing this all for?
                        nlog(cMPGameCL, "BMPGameSession::clientConnected -- Could not send to the target client - dropping them(2)");
                        getSession()->kickClient(clientIndex);
                        getSession()->disconnectClient(clientIndex);
                        break;
                     }
                  }
               }
            }
         }
      }
      //long count = getSession()->getClientCount();
      //nlog(cMPGameCL, "BMPGameSession::clientConnected -- Sending player index packet to everyone pid[%d], xuid[%I64u]",  playerID, pClient->getXuid());
      //for (long i=0; i < count; i++)
      //{
      //   BClient* pTempClient = getSession()->getClient(i);
      //   if (pTempClient && pTempClient->isConnected())
      //   {
      //      playerID = getPlayerID(i);
      //      if (playerID != cMPInvalidPlayerID) 
      //      {
      //         //This is a valid, fully connected and in-game player
      //         if (!getSession()->isLocalClientID(i) &&
      //            ((long)clientIndex != i))
      //         {
      //            //Send that client this new connector's info 
      //            //  - unless that client is the host (cause he already knows!)
      //            //  - unless that client is the NEW client (he will be told about himself down below)
      //            if (mChannelArray[cChannelSettings]->SendPacketTo(pTempClient, spacket) != S_OK)
      //            {
      //               //Todo - Examine issues from not being able to send to some, have they dropped?
      //               nlog(cMPGameCL, "BMPGameSession::clientConnected -- Could not send to a client.");
      //            }
      //         }
      //         BPlayerIDPacket spacketPerClient(playerID, pTempClient->getXuid());
      //         if (mChannelArray[cChannelSettings]->SendPacketTo(pClient, spacketPerClient) != S_OK)
      //         {
      //            //I can't send to the person i'm doing this all for?
      //            nlog(cMPGameCL, "BMPGameSession::clientConnected -- Could not send to the target client - dropping them(2)");
      //            getSession()->kickClient(clientIndex);
      //            getSession()->disconnectClient(clientIndex);
      //            break;
      //         }
      //      }
      //   }
      //}
   }
   else
   {
      //I'm not the host, just try to connect them 
      addPendingPlayer(clientIndex);
   }
   //}
   //else
   //{
   //   nlog(cMPGameCL, "BMPGameSession::clientConnected -- mGameConnected:false");
   //}
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::clientDisconnected(const BSessionEvent* pEvent)
{
   uint32 clientIndex = pEvent->mData1;
   nlog(cMPGameCL, "BMPGameSession::clientDisconnected -- ID[%d]", clientIndex);

   BMachineID machineID = pEvent->mData2 >> 16 & 0xFF;
   //BMachineID localMachineID = pEvent->mData2 >> 8 & 0xFF;
   //BMachineID hostMachineID = pEvent->mData2 & 0xFF;

   // treat the machine as if they sent a finalize packet
   if (machineID >= 0 && machineID < XNetwork::cMaxClients)
      mMachineFinalize[pEvent->mData1] = TRUE;

   //BASSERT( mPlayerManagerInterface );
   //BASSERT( mSessionInterface);

//-- FIXING PREFIX BUG ID 1405
   const BClient* client = getSession()->getClient(clientIndex);
//--

   // FIXME-COOP - clientIndex could be something other than 0 for the host if both primary and secondary users are playing
   //
   //Check for the clientIndex==0 instead of client->isHost() because the client record has had that data reset before it gets here
   //  index==0 is ALWAYS TRUE for the host
   if ((clientIndex==0) &&
       (mState<cBMPGameSessionStateGameSetup))
   {
      //If the disconnecting dude is the host, and we have not yet started the game load up, then we need to just abort this game session
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected - game host left before game load started, aborting this game session");
      shutDown();
   }

   PlayerID pid = cMPInvalidPlayerID;

   pid = getPlayerID( clientIndex );

   //Did we find a player record with a client connection up at the game level?
   if (client && pid == cMPInvalidPlayerID)
   {
      //No - try to find him by xuid
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected - Could not find playerID by using his clientID, trying by XUID");
      pid = getPlayerIDByGamerTag( client->getXuid() );
   }

   if (pid != cMPInvalidPlayerID)
   {
      //There was a player record for this dropping client
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected -- found a playerManager (game level) record for this client (player ID:%d), tell it to drop that record", pid);
      BOOL local = getSession()->isLocalClientID(clientIndex);
      //Let the session interface know
      if (mpMPSession->getSessionInterface())
      {
         mpMPSession->getSessionInterface()->mpSessionEvent_playerLeft(pid, local);
      }

      //Let the sim know
      if (mpSimObject)
         mpSimObject->playerDisconnected(pid, (pEvent->mData4 == BSession::cNormal));

      //Release his player ID
      releasePlayerID( pid );
   }
   else
   {
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected - Could not find a playerID for this clientID or XUID");
   }

   if (client && !mpMPSession->isInLANMode())
   {
      BASSERT(mpLiveSession);
      mpLiveSession->dropRemoteUserFromSession( client->getXuid());
   }

   //Drop them from the pending list in cast they were in it
   removePendingPlayer( clientIndex );

   //Update our broadcasted info
   updateBroadcastedHostData();

   //Were we already started down the game launch path but yet not in the game yet?
   if ((mState>=cBMPGameSessionStateGameSetup) &&
       (mState<cBMPGameSessionStateStartPregame))
   {
      //Game hasn't started its load process yet - lets kill it right now so that it doesn't continue on 
      //TODO - it is possible to cleanup this session enough to just continue on (for matchmaking) - but for now it is safer to just throw all out and start over
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected - game was about to launch, now missing a client so aborting this game session.");
      shutDown();
   } 
   else if (mState ==cBMPGameSessionStateStartPregame)
   {
      //If someone left at this point - we could be waiting on their finalize packet which we will never get
      //So call finalize to let it check that everyone who is still in the session has send a finalize
      nlog(cMPGameCL, "BMPGameSession::clientDisconnected - game has loaded, but not started, calling finalize to check if it can now start with this disconnect cleared");
      finalizeGame();
   }
   //Also update that logic to specifically look for games that are NOT going to start because of teams that have completely dropped, and log that

   nlog(cMPGameCL, "BMPGameSession::clientDisconnected -- player deleted");

   gStatsManager.setDisconnect(clientIndex);
}


//==============================================================================
//
//==============================================================================
void BMPGameSession::sessionDisconnected(BSession::BDisconnectReason reason)
{
   nlog(cMPGameCL, "BMPGameSession::sessionDisconnected --");

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
      mMachineFinalize[i] = TRUE;

   //mGameConnected = false;
   //mSessionConnected = false;
   mSentSessionDisconnected = true;
   mSettingsComplete = false;
   /*

   long mpreason = BMPSession::cDisconnectFailedConnection;

   switch (reason)
   {
      case BSession::cTransportLost:
      case BSession::cSessionTerminated:
      case BSession::cHostDecision:
      case BSession::cHostCancelledGame:
         mpreason = BMPSession::cDisconnectGameTerminated;
         break;

      case BSession::cConnectionRejected:
      case BSession::cFailedClientConnect:
         mpreason = BMPSession::cDisconnectFailedConnection;
         break;

      case BSession::cBuildMismatch:
         mpreason = BMPSession::cDisconnectCRCMismatch;
         break;

      case BSession::cSessionClosed:
      case BSession::cSessionFull:
         mpreason = BMPSession::cDisconnectFull;
         break;

      case BSession::cGameDeleted:
         mpreason = BMPSession::cDisconnectDeleted;
         break;

      case BSession::cNormal:
      default:
         mpreason = BMPSession::cDisconnectNormal;
         break;
   }
   */

   // tell our primary user that we've disconnected and check for end of game scenarios
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser)
      pUser->endGameDisconnect();

   mpMPSession->gameSessionDisconnected(reason);
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::updateBroadcastedHostData()
{
   //This method is used for two things:
   //1. To keep up-to-date a structure that is used to define all the connection info about this current session (people, keys, etc)
   //2. If it is a host of the game session and its LIVE - to push that data into the QoS response memory area

   if (mpMPSession->isInLANMode())
   {
      return;
   }
 
   if (isHosting())
   {
      //Update the information which is in the host's QoS response
      Utils::FastMemSet(&mQoSResponseData, 0, sizeof(mQoSResponseData));

      //If we are matchmaking, and have not yet got fully connected as a party - then send a 1 byte response that means 'not yet dude'
      if (mpMPSession->isMatchmakingRunning() && !mpMPSession->isMatchMakingPartyFullyJoinedToCurrentTarget())
      {
         mQoSResponseDataSize = 1;
      }
      else
      {
         mQoSResponseDataSize = cBQoSResponseDataBaseSize;
         mQoSResponseData.mCheckSum = mLocalChecksum;
         mQoSResponseData.mNonce = getNonce();
         /*
         mQoSResponseData.mClientCount = 0;

         BDEBUG_ASSERT(getSession());
         if (getSession() == NULL)
            return;

         for (int i=0; i < getSession()->getMachineCount(); ++i)
         {
   //-- FIXING PREFIX BUG ID 1406
            const BMachine* pMachine = getSession()->getMachine(i);
   //--
            if (pMachine != NULL && !pMachine->isLocal() && pMachine->isConnected())
            {
               mQoSResponseData.mClientXNAddrs[mQoSResponseData.mClientCount] = pMachine->getXnAddr();
               mQoSResponseData.mClientCount++;
               mQoSResponseDataSize += sizeof(XNADDR);
            }
         }
         */

         mQoSResponseData.mPublicSlotsOpen = (uint8)(mPublicSlots - getPlayerCount());
         mQoSResponseData.mPublicSlots = (uint8)mPublicSlots;
         //mQoSResponseData.mVersionCode = (uint8)mLocalChecksum;  //Hack to support version filtering in matchmaking, replace with version #s when we have them
         mQoSResponseData.mLanguageCode = mLanguageCode;
      }

      INT hr;
      hr = XNetQosListen(&mLocalXNKID, reinterpret_cast<BYTE*>(&mQoSResponseData), mQoSResponseDataSize, 0, (XNET_QOS_LISTEN_SET_DATA));
      if (hr != 0)
      {
         //QoS start failed.
         nlog(cMPGameCL, "BMPGameSession::updateBroadcastedHostData - host data update request failed, error code %d", hr);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::setQoSNotification(bool enabled)
{
   nlog(cMPGameCL, "BMPGameSession::setQoSNotification");
   if (mQoSResponding == enabled)
   {
      //I'm already doing what you want
      nlog(cMPGameCL, "BMPGameSession::setQoSNotification - already in requested state");
      return;
   }

   if (mQoSResponding)
   {
      //Stop it
      mQoSResponding = false;
      INT hr = XNetQosListen(&mLocalXNKID, NULL, 0, 0, (XNET_QOS_LISTEN_RELEASE));
      if (hr != 0)
      {
         //QoS stop failed.
         nlog(cMPGameCL, "BMPGameSession::setQoSNotification - stop request failed, error code %d", hr);
         return;
      }
      nlog(cMPGameCL, "BMPGameSession::setQoSNotification - response stopped");
   }
   else
   {
      //Start Qos Response    
      if (isHosting())
      {
         //Call refresh to fill in the state data about this host
         updateBroadcastedHostData();   
         INT hr;         
         hr = XNetQosListen(&mLocalXNKID, reinterpret_cast<BYTE*>(&mQoSResponseData), mQoSResponseDataSize, 0, (XNET_QOS_LISTEN_ENABLE|XNET_QOS_LISTEN_SET_DATA));
         if (hr != 0)
         {
            //QoS start failed.
            nlog(cMPGameCL, "BMPGameSession::setQoSNotification - host data start request failed, error code %d", hr);
            return;
         }
      }
      else
      {
         //Clients host out no additional Qos information
         INT hr;
         hr = XNetQosListen(&mLocalXNKID, NULL, 0, 0, (XNET_QOS_LISTEN_ENABLE));
         if (hr != 0)
         {
            //QoS start failed.
            nlog(cMPGameCL, "BMPGameSession::setQoSNotification - client data start request failed, error code %d", hr);
            return;
         }
      }
      nlog(cMPGameCL, "BMPGameSession::setQoSNotification - response running");
      mQoSResponding = true;
   }
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::requestGameLaunch(DWORD countdown)
{
   // if not hosting, or already launching.
   if (!isHosting() || mLaunchCountdown > 0)
      return(false);

   if (!mChannelArray[cChannelMessage])
      return(false);

   nlog(cMPGameCL, "BMPSession::requestGameLaunch -- enter.");

   mLaunchCountdown = countdown;
   mLaunchLastUpdate = 0;
   mLaunchUpdateCounter = countdown / 1000; // defaults to 3

   if (mpMPSession->getSessionInterface())
   {
      //Have the host submit the final map and team settings
      mpMPSession->getSessionInterface()->mpSessionEvent_partyEvent_hostSubmitFinalGameSettings();
   }

   BCompleteSettingsPacket spacket(BChannelPacketType::cFinalSettingsPacket);
   fillCompleteSettings(spacket);
   mChannelArray[cChannelSettings]->SendPacket(spacket);
   nlog(cMPGameCL, "BMPGame::requestGameLaunch -- Sending cFinalSettingsPacket");

   BMessagePacket packet(1, BChannelPacketType::cLockSettingsPacket);
   nlog(cMPGameCL, "BMPGame::requestGameLaunch -- Sending cLockSettingsPacket");
   return(mChannelArray[cChannelMessage]->SendPacket(packet)==S_OK);   
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::fillCompleteSettings(BCompleteSettingsPacket &packet)
{
   // send over all settings
   const DWORD cBufferSize = cMaxSendSize;
   static BYTE buffer[cBufferSize];
   static DWORD offset;
   WORD  size;

   offset = 0;

   DWORD count = mpGameSettings->getNumberEntries();
   Utils::FastMemCpy(&buffer[offset], &count, sizeof(count));
   offset += sizeof(count);

   for (DWORD idx=0; idx<count; idx++)
   {
      size = (WORD)mpGameSettings->getDataSize(idx);
      Utils::FastMemCpy(&buffer[offset], &size, sizeof(size));
      offset += sizeof(size);

      if (size > 0)
      {
         if (!mpGameSettings->getData(idx, &buffer[offset], (WORD)(cBufferSize-offset)))
         {
            BASSERT(0);
            break;
         }

         offset += size;
      }

      if (offset >= cBufferSize)
      {
         BASSERT(0);
         break;
      }
   }

   packet.mData = buffer;
   packet.mSize = offset;
}

//==============================================================================
// BMPSession::requestLaunchAbort
//==============================================================================
bool BMPGameSession::requestLaunchAbort(long reason)
{
   //Deprecated
   BASSERT(false);

   if (!mChannelArray[cChannelMessage])
      return(false);

   nlog(cMPGameCL, "BMPGameSession::requestLaunchAbort -- Sending cLaunchAbortRequestPacket");
   BMessagePacket packet((long)reason, BChannelPacketType::cLaunchAbortRequestPacket);
   return(mChannelArray[cChannelMessage]->SendPacket(packet)==S_OK);   
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::sendLaunchReady(bool ready)
{
   if (!mChannelArray[cChannelMessage])
      return;

   nlog(cMPGameCL, "BMPGameSession::sendLaunchReady -- Sending cLaunchReadyPacket");
   BMessagePacket packet((long)ready?1:0, BChannelPacketType::cLaunchReadyPacket);

   HRESULT hr = mChannelArray[cChannelMessage]->SendPacket(packet);
   if (hr!=S_OK)
   {
      nlog(cMPGameCL, "BMPGameSession::sendLaunchReady -- Send FAILED because of error %d", hr );
   }
   BASSERT (hr==S_OK);
}

//==============================================================================
// This is called when game is in progress
//==============================================================================
bool BMPGameSession::finalizeGame(void)
{
 
   if (!getSession())
      return(false);

   if (!mLoadTracker.getAreAllPlayersLoaded())
      {
        return false;
      }

   BASSERT(isHosting());

   nlog(cMPGameCL, "BMPGameSession::finalizeGame -- all players loaded. setting game state cGameStateInProgress.");

   setState(cBMPGameSessionStateInGame);

   return(true);
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::addPendingPlayer(ClientID clientID, PlayerID playerID)
{
   nlog(cMPGameCL, "BMPGameSession::addPendingPlayer -- Adding client to the pending player list clientID[%d], playerID[%d]", clientID, playerID);

   //Verify the client is in the session list
//-- FIXING PREFIX BUG ID 1407
   const BClient* pClient = getSession()->getClient(clientID);
//--
   if (!pClient)
   {
      nlog(cMPGameCL, "BMPGameSession::addPendingPlayer -- Failed to get client from session. clientID[%d], playerID[%d]", clientID, playerID);
      return;
   }

   //Get a tracker object for him
   int openSlot = -1;
   for (int i=0; i < cMPSessionMaxPendingPlayers; i++)
   {
      if (mPendingPlayers[i].clientID == clientID)
      {
         openSlot = i;
         break;
      }
      else if ((openSlot == -1) && (mPendingPlayers[i].xuid == 0))
      {
         openSlot = i;
         break;
      }
   }

   //No record?
   if (openSlot == -1)
   {
      nlog(cMPGameCL, "BMPGameSession::addPendingPlayer -- ERROR: No more open space for pending players, losing clientID[%d], playerID[%d]", clientID, playerID);
      return;
   }

   //Set the record up correctly
   mPendingPlayers[openSlot].xuid = pClient->getXuid();
   mPendingPlayers[openSlot].clientID = clientID;
   mPendingPlayers[openSlot].playerID = playerID;
   mPendingPlayers[openSlot].liveSessionJoined = false;
   mPendingPlayers[openSlot].lastTimeUpdated = timeGetTime();

   //TODO - fix this hack
   //The issue is that we need to redo things so that the session ready event is not raised to the modemultiplayer layer
   //  until AFTER our local player is completely ready from this layer and is no longer in the addPendingPlayer list
   //  Currently the issue is only really bad when we are hosting and on Live and waiting for him to join his local session
   //  This hack just goes ahead and sets that flag as if it was good
   //  This will cause a problem if ever the creating host cannot join his session.
/*
   if ((clientIndex == getSession()->getLocalClientID()) &&
      (clientIndex == getSession()->getHostID()))
   {
      mPendingPlayers[openSlot].liveSessionJoined = true;
   }
   */
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::checkAllPlayersInitialized()
{
   if (!getSession())
   {
      //No session running - no need to check pending players
      return false;
   }

   if (!mpMPSession->getSessionInterface())
   {
      //No interfaces to check with on player/session events - we are done
      return false;
   }

   long maxPlayerCount = getMaxPlayers();
   //Changed to switch this to looking at the gamedb player count - for AI player support - eric
   mpGameSettings->getLong(BGameSettings::cMaxPlayers, maxPlayerCount);

   BDEBUG_ASSERTM(maxPlayerCount <= cMPSessionMaxUsers, "Max requested players exceeds the amount allocated");
   if (maxPlayerCount > cMPSessionMaxUsers)
      return false;

   // go through all the players and check if they are all configured correctly
   for (long i=0; i < maxPlayerCount; ++i)
   {
      const BMPSessionPlayer& player = mClientPlayerMap[i];
      if (player.mClientID == cMPInvalidClientID || player.mPlayerID == cMPInvalidPlayerID)
      {
         return false;
      }
      if (mPendingPlayers[player.mClientID].xuid != 0)
      {
         //They are still in the pending players list
         return false;
      }
   }

   nlog(cMPGameCL, "BMPGameSession::checkAllPlayersInitialized -- all players have established clientIDs and playerIDs, sending party session launch request");

   //Set myself as readied (greened) up
   sendLaunchReady(true);

   return true;
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::updatePendingPlayers()
{
   if (!getSession())
   {
      //No session running - no need to check pending players
      return;
   }

   if (!mpMPSession->getSessionInterface())
   {
      //No interfaces to check with on player/session events - we are done
      return;
   }

   for (int i=0; i < cMPSessionMaxPendingPlayers; i++)
   {
      if (mPendingPlayers[i].xuid != 0)
      {
         //We have a pending player
         //First - make sure its still a valid client
//-- FIXING PREFIX BUG ID 1408
         const BClient* pClient = getSession()->getClient(mPendingPlayers[i].clientID);
//--
         if (!pClient)
         {
            //Not valid any more for whatever reason
            nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Dropping pending player (cid:%i) because they no longer have a Bclient record in BSession", mPendingPlayers[i].clientID);
            removePendingPlayer( i );
            continue;
         }

         //Check if we were waiting on a player ID
         // the assignment of the playerID in the pending players list will happen when we receive the player ID packet from the host
         //if (mPendingPlayers[i].playerID == cMPInvalidPlayerID)
         //{
         //   mPendingPlayers[i].playerID = getPlayerIDByGamerTag(mPendingPlayers[i].xuid);
         //}

         //Check if they have been added to the live session
         if ((!mpMPSession->isInLANMode()) &&
            (!mPendingPlayers[i].liveSessionJoined))
         {
            BLiveSessionUserStatus status = mpLiveSession->getUserStatus( mPendingPlayers[i].xuid );
            if (status == cLiveSessionUserStatusInSession)
            {
               //They are now in the session - we are good
               nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Requesting client %i was added to the Live Session as remote", mPendingPlayers[i].clientID);
               mPendingPlayers[i].liveSessionJoined = true;
            }               
            else if (status == cLiveSessionUserStatusUserUnknown)
            {
               //They are not in the system at all - request a session add for them
               nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Requesting client %i be added to the Live Session as remote", mPendingPlayers[i].clientID);
               mpLiveSession->addRemoteUserToSession( mPendingPlayers[i].xuid, false );
            }
            else if (status != cLiveSessionUserStatusAddPending )
            {
               //Something bad has happened to them - drop this person
               nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Client could not be added to Live Session as remote - dropping %i", mPendingPlayers[i].clientID);
               removePendingPlayer( i );
               continue;
            }
         }

         //See if they have met all the conditions to be ready to be considered a player
         if ((mPendingPlayers[i].playerID != cMPInvalidPlayerID) &&
             ((mpMPSession->isInLANMode()) ||
              (mPendingPlayers[i].liveSessionJoined)))
         {
            //Yes! they are go to go
            nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Adding pending player (cid:%i/pid:%i) to the game as a fully joined player, removing them from the pending player list", mPendingPlayers[i].clientID, mPendingPlayers[i].playerID);
            PlayerID playerID = mPendingPlayers[i].playerID;

            //Erase their pending record
            mPendingPlayers[i].xuid = 0;

            //At this point - non-hosts need to add them to the clientID to PlayerID mapping table
            if (!isHosting())
            {
               createPlayer(playerID, pClient->getID(), true);

               //BMPSessionPlayer* pPlayer = getPlayerFromPlayerId( playerID );
               //if (!pPlayer)
               //{
               //   //Oh noes - how did they get here?
               //   nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Player was assigned a playerID, but its now invalid (PID:%d), dumping them", playerID);
               //   getSession()->kickClient( pClient->getID() );
               //   continue;
               //}
               //if (pPlayer->mClientID != cMPInvalidClientID)
               //{
               //   nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Player was assigned a playerID, but it already has a clientID assigned (PID:%d, CID:%d), dumping them", playerID, pClient->getID());
               //   getSession()->kickClient( pClient->getID() );
               //   continue;
               //}
               //pPlayer->mClientID = pClient->getID();
               //pPlayer->mLiveEnabled = true; //Fix later for AI
            }

            //Add them to the game
            mpMPSession->getSessionInterface()->mpSessionEvent_playerJoined(mPendingPlayers[i].playerID, pClient->getID(), pClient->getXuid(), pClient->getName());

            //My own local player has joined - request that he send his user selected settings in now
            if (getSession()->isLocalClientID(pClient->getID()))
            {
               //Remember my local playerID - even though currently it is not queried anywhere...
               mLocalPlayerID = mPendingPlayers[i].playerID;

               //Submit my civ/leader settings
               nlog(cMPMatchmakingCL, "BMPGameSession::updatePendingPlayers -- pending local player has a complete join, requesting local settings");
               mpMPSession->getSessionInterface()->mpSessionEvent_requestForSetLocalPlayerSettings();

               //Update our broadcasted info
               updateBroadcastedHostData();

               // do not let the system know I'm ready yet because I may still be waiting on player IDs and settings for other players
               //checkForInitComplete()
               // what defines init complete?
               // no more pending players and I've received playerID information for everyone?

               //Let the system know I'm ready
               //
               // NOT yet, let's wait for all the pending players to complete their thing
               //
               // gameSessionConnected will check for hosting values, all clients will call it, few will succeed
               mpMPSession->gameSessionConnected();
               //if (isHosting())
               //{
               //   mpMPSession->gameSessionHostStarted();
               //}
               //else
               //{
               //   mpMPSession->gameSessionConnected();
               //}

               //Send a green up to the game session members
               //NOTE: We no longer do this, the party session tells mpSession to issue this command once it sees ALL its members as fully joined
               //      This prevents matchmaking disasters like 2 different 2v2 parties joining the same server at the same time and ending up with
               //      a game starting from the hosts joining - but not their other party members
               //sendLaunchReady(true);

               /*
               //If this is a party game - let everyone know that I've connected
               if (mPartySession)
               {
                  mPartySession->partySendJoinSuccessCommand(mpLiveSession->getNonce());
                  if (mMMMode==FALSE)
                  {
                     //We are in the custom game mode - send an event to the party to let it know we are hooked up
                     mSessionInterface->mpSessionEvent_partyEvent_customGameStartupComplete(cMPSessionCustomGameStartResultSuccess);
                  }
               }
               else
               {
                  //Set myself as ready if not in a party game
                  sendLaunchReady(true);
               }
               */
            }
         }
         /*
         else if ((timeGetTime() - mPendingPlayers[i].lastTimeUpdated) > cMPSessionConnectTimeout)
         {
            nlog(cMPGameCL, "BMPGameSession::updatePendingPlayers -- Client is taking too long to join - dropping %i", mPendingPlayers[i].clientID);
            removePendingPlayer( i );
         }
         */
      }
   }

}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::removePendingPlayer(uint32 clientIndex)
{
   //Is he in the list?
   if (mPendingPlayers[clientIndex].xuid == 0)
   {
      //Empty record 
      return;
   }

   //Was he in the live session?
   if ((!mpMPSession->isInLANMode()) &&
      (mPendingPlayers[clientIndex].liveSessionJoined) &&
      (mpLiveSession))
   {
      mpLiveSession->dropRemoteUserFromSession( mPendingPlayers[clientIndex].xuid );
   }

   //Did he have a playerID reserved?
   if (mPendingPlayers[clientIndex].playerID != cMPInvalidPlayerID)
   {
      releasePlayerID( mPendingPlayers[clientIndex].playerID );
   }

   //Am I removing myself?
   if (getSession()->isLocalClientID(mPendingPlayers[clientIndex].clientID))
   {
      nlog(cMPGameCL, "BMPGameSession::removePendingPlayer -- Dropping myself as a pending player(cid:%i) because I could not get fully hooked up in time", clientIndex);
      getSession()->disconnect(BSession::cFailedClientConnect);
   }
   //Am I removing the host?
   else if (getSession()->isHostClientID(mPendingPlayers[clientIndex].clientID))
   {
      nlog(cMPGameCL, "BMPGameSession::removePendingPlayer -- Dropping host as a pending player(cid:%i) - so this drops me from the session", clientIndex);
      getSession()->disconnect(BSession::cFailedClientConnect);
   }
   else
   {
      //Kick him from the session
      nlog(cMPGameCL, "BMPGameSession::removePendingPlayer -- kicking client (cid:%i)", clientIndex);
      getSession()->kickClient( mPendingPlayers[clientIndex].clientID );
      getSession()->disconnectClient( mPendingPlayers[clientIndex].clientID );
   }

   nlog(cMPGameCL, "BMPGameSession::removePendingPlayer -- Dropping pending player record (cid:%i)", clientIndex);
   //Set the record as empty so it can be reused
   mPendingPlayers[clientIndex].xuid = 0;

}




//********************  Rest of file are methods to implement external interfaces ****************

//==============================================================================
// BDataSet::BDataListener interface that we implement to look for when settings are changed
//==============================================================================
void BMPGameSession::OnDataChanged(const BDataSet* set, DWORD index, BYTE flags)
{ 
   if (mpMPSession->getSessionInterface())
   {
      mpMPSession->getSessionInterface()->mpSessionEvent_settingsChanged(set, index, flags);
   }
   else
   {
      nlog(cMPGameCL, "BMPGameSession::OnDataChanged - Error, no session interface");
   }
}

 //==============================================================================
 //
 //==============================================================================
//BMPSession::BClientConnector interface 
//bool BMPGameSession::connectionAttempt( BClient* requestingClient )
//{
//   return true;
//}

//Callback going up when we have a client requesting to connect to the host of the session.
//  If approved, then they are assigned a client ID, and allowed to try and fully connect to the session
// FIXME-COOP, check space for two potential users
bool BMPGameSession::sessionConnectionRequest(const BSessionUser users[], BSession::BJoinReasonCode* reasonCode)
{
   long open = mPublicSlots + mPrivateSlots - getPlayerCount();

   if (open <= 0)
   {
      nlog(cMPGameCL, "BMPGameSession::recvd-JoinRequest -- Rejected, Game Full");
      nlog(cMPGameCL, "BMPGameSession::recvd-JoinRequest -- Session Open: %d", open);
      *reasonCode = BSession::cResultRejectFull;
      return false;
   }

   joinRequest(users, *reasonCode);
   
   if (*reasonCode != BSession::cResultJoinOk)
   {
      return false;
   }
   return true;
}


//==============================================================================
//
//==============================================================================
//BMPSession::BSessionEventObserver interface 
void BMPGameSession::processSessionEvent(const BSessionEvent* pEvent)
{
   switch (pEvent->mEventID)
   {
      case BSession::cEventJoinFailed:
         {      
            nlog(cMPGameCL, "BMPGameSession::processSessionEvent -- cEventJoinFailed");
            mpMPSession->gameSessionJoinFailed((BSession::BJoinReasonCode)pEvent->mData1 );
            shutDown();
            break;
         }
      case BSession::cEventConnected:
         {                  
            if (!isHosting())
            {
               nlog(cMPGameCL, "BMPGameSession::processSessionEvent -- cEventConnected requesting settings from host.");
               //mGameConnected = true;
               BChannelPacket packet(BChannelPacketType::cRequestSettingsPacket);
               mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostMachine(), packet);
               setState(cBMPGameSessionStateJoinWaitingForInitialSetup);
            }
            else
            {
               if (mState==cBMPGameSessionStateShuttingDown)
               {
                  nlog(cMPGameCL, "BMPGameSession::processSessionEvent -- cEventConnected ignored, Im shutting down");
                  return;
               }
               BASSERT(mState==cBMPGameSessionStateLaunchHostWaitingForSelfConnection);

               nlog(cMPGameCL, "BMPGameSession::processSessionEvent -- cEventConnected host settings complete.");

               if (mState!=cBMPGameSessionStateLaunchHostWaitingForSelfConnection)
               {
                  nlog(cMPGameCL, "BMPGameSession::processSessionEvent -- ERROR:I'm in the wrong state [%d]", mState);
               }

               //Since as host I own the settings - then they are good to go
               mSettingsComplete = true;
               if (!mpMPSession->getSessionInterface())
               {                 
                  //No Session interface at this point?
                  nlog(cMPGameCL, "BMPGameSession::processSessionEvent - Error, I have a session event cEventConnected - but no session interface" );
                  BASSERT(false);
                  mpMPSession->gameSessionHostFailed(BMPSession::cMPSessionGameSessionHostResultSessionError);
                  shutDown();
                  return;
               }
               
               //Dropping this - eric
               // Reason: This is a hook where the host can process itself first, however it causes the logic for assigning things (playerID, registering with Live)
               //   to be somewhere else other than updatePlayers() (ie: its duplicated).  This sucks.  So dropping this to just have ONE place do this
               //   processing, and ONE place to let other layers know he is good.   This does add the contraint that the host must be allowed to full
               //   join himself BEFORE inviting other players to join ... but thats the way it works currently anyways.
               //mpMPSession->getSessionInterface()->mpSessionEvent_requestForSetLocalPlayerSettings();
               mpMPSession->getSessionInterface()->mpSessionEvent_initialSettingsComplete();

               //mSessionConnected = TRUE;
               //mGameConnected = true;

               //Are there session members connected that I need to hook up here?
               // These would be clients that somehow managed to get fully connected to the session before I did...
               // Still worth checking here because I'd have to issue clientConnected for myself anyways
               //long count = getSession()->getClientCount();
               //for (long idx=0; idx<count; idx++)
               //{
               //   BClient* p = getSession()->getClient(idx);
               //   if (p && p->isConnected())
               //   {
               //      clientConnected(idx);                  
               //   }
               //}

               //I'm ready
               setState(cBMPGameSessionStateReady);
               //Hmm - nope - not until processPendingPlayers says you are bubba
               //mpMPSession->gameSessionHostStarted();
            }
         }
         break;

      case BSession::cEventDisconnected:
         {
            nlog(cMPGameCL, "BMPSession::processSessionEvent -- cEventDisconnected");
            sessionDisconnected(static_cast<BSession::BDisconnectReason>(pEvent->mData1));
            break;
         }

      case BSession::cEventClientConnect:
         {
            // FIXME-COOP - can have multiple clientIDs on a single machineID
            //              insure clientConnected can handle this
            BClientID clientID = pEvent->mData1;
            BMachineID machineID = pEvent->mData2 >> 16 & 0xFF;
            BMachineID localMachineID = pEvent->mData2 >> 8 & 0xFF;
            BMachineID hostMachineID = pEvent->mData2 & 0xFF;

            clientConnected(clientID, machineID, localMachineID, hostMachineID, TRUE);
            break;
         }

      case BSession::cEventClientDisconnect:
         clientDisconnected(pEvent);
         break;

      case BSession::cEventClientData:
         handleClientData(pEvent);
         break;

      case BSession::cEventClientPing:
         clientPingUpdated(pEvent->mData1, pEvent->mData2);
         break;

      case BSession::cEventChannelData:
         {
            //if (isRunning())
            {
               //Don't respond to this data if we are not running (how?) or if we are shutting down (possible)
               handleChannelData(pEvent);
            }
            break;
         }

      default:
         break;
      }
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::handleChannelData(const BSessionEvent* pEvent)
{
   if (pEvent == NULL)
      return;

   const void* pData = (((char*)pEvent)+sizeof(BSessionEvent));
   long size  = pEvent->mData2;
   long fromMachineID = pEvent->mData1;

   if (size == 0)
      return;

   long channelID = BChannelPacket::getChannel(pData);

   for (long idx=0; idx<cChannelMax; idx++)
   {
      if (mChannelArray[idx] == NULL)
         continue;

      if (mChannelArray[idx]->getChannelID() == channelID)
      {
         mChannelArray[idx]->channelDataReceived(fromMachineID, pData, size);
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::clientPingUpdated(uint32 clientIndex, uint32 ping)
{

   //Note - in the new modePartyRoom - there is no interface once the game is launched.  
   if (!mpMPSession->getSessionInterface())
   {
      return;
   }

   mpMPSession->getSessionInterface()->mpSessionEvent_playerPingUpdate(getPlayerID((ClientID)clientIndex), ping);

}

//==============================================================================
//
//==============================================================================
HRESULT BMPGameSession::getLocalTiming(uint32& timing, uint32* deviationRemaining)
{
   if (mpSimObject && mpSimObject->getTimingHandler())
      return mpSimObject->getTimingHandler()->getLocalTiming(timing, deviationRemaining);
   else
      return HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);
}

//==============================================================================
//
//==============================================================================
float BMPGameSession::getMSPerFrame(void)
{
   if (mpSimObject && mpSimObject->getTimingHandler())
      return mpSimObject->getTimingHandler()->getMSPerFrame();
   else
      return 0;
}


//==============================================================================
//
//==============================================================================
void BMPGameSession::channelDataReceived(const long fromMachineID, const void *data, const DWORD size)
{
   long channelID = BChannelPacket::getChannel(data);
   long idx;
   for (idx=0; idx<cChannelMax; idx++)
   {
      if (mChannelArray[idx] == NULL)
         continue;

      if (mChannelArray[idx]->getChannelID() == channelID)
         break;
   }

   if (idx >= cChannelMax)
   {
      BFAIL("BMPGameSession::channelDataReceived -- unknown channel.");
      return;
   }

   switch (idx)
   {
      case cChannelMessage:
         handleMessage(fromMachineID, data, size);
         break;

      case cChannelCommand:
         if (mpSimObject)
            mpSimObject->commandDataReceived(fromMachineID, data, size);
         break;

      case cChannelSim:
         if (mpSimObject)
            mpSimObject->simDataReceived(data, size);
         break;

      case cChannelSync:
         {
            BSyncPacket packet;
            packet.deserializeFrom(data, size);
            notifySyncData(fromMachineID, packet.mID, packet.mChecksum);      
         }
         break;

      case cChannelVote:
         {
            BASSERT(false);
         }
         break;

      case cChannelSettings:
         {
            handleSettings(fromMachineID, data, size);
         }
         break;
   }
}

//==============================================================================
//
//==============================================================================
bool BMPGameSession::sendSyncData(long uid, uint checksum)
{
   if (!mChannelArray[cChannelSync])
      return(false);

   BSyncPacket packet(uid, checksum);
   return ( mChannelArray[cChannelSync]->SendPacket(packet) == S_OK);
}

//==============================================================================
//
//==============================================================================
long BMPGameSession::getSyncedCount(void) const
{
   if (!getSession())
      return(1);

   return(getSession()->getActiveMachineAmount());
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::outOfSync(void) const
{
   if (getSession())
      getSession()->disconnect(BSession::cSessionTerminated);
}

//==============================================================================
//
//==============================================================================
HRESULT BMPGameSession::sendCommandPacket(BChannelPacket& packet)
{
   if (!mChannelArray[cChannelCommand]) 
      return E_FAIL;

   mChannelArray[cChannelCommand]->SendPacket(packet);

   return S_OK;
}

//==============================================================================
//
//==============================================================================
HRESULT BMPGameSession::sendSimPacket(BChannelPacket &packet)
{
   if (!mChannelArray[cChannelSim]) 
      return E_FAIL;

   mChannelArray[cChannelSim]->SendPacket(packet);

   return S_OK;
}

//==============================================================================
// Base constructor for the pending player tracker
//==============================================================================
BMPSessionPendingPlayer::BMPSessionPendingPlayer() :
   xuid(0),
   clientID(cMPInvalidClientID),
   playerID(cMPInvalidPlayerID),
   liveSessionJoined(false),
   lastTimeUpdated(0)
{
}

//==============================================================================
// Base constructor for the client to player map tracker
//==============================================================================
BMPSessionPlayer::BMPSessionPlayer() :
   mClientID(cMPInvalidClientID),
   mPlayerID(cMPInvalidPlayerID),
   mpTrackingDataBlock(NULL),
   mLiveEnabled(true)
{
}

//==============================================================================
//
//==============================================================================
BMPSessionPlayer::~BMPSessionPlayer() 
{
   if (mpTrackingDataBlock)
   {
      delete mpTrackingDataBlock;
      mpTrackingDataBlock=NULL;
   }
}

//==============================================================================
// Loaded players tracker code
//==============================================================================
BMPSessionLoadTracker::BMPSessionLoadTracker()
{
   reset();
}

//==============================================================================
//
//==============================================================================
BMPSessionLoadTracker::~BMPSessionLoadTracker()
{
}

//==============================================================================
//
//==============================================================================
void BMPSessionLoadTracker::setPreLoadReady(long machineID, bool loaded)
{
   if (!mInitialized)
   {
      nlog(cMPGameCL, "BMPSessionLoadTracker::setPreLoadReady - loaded tracker not initialized - exiting");
      return;
   }

   for (uint i=0;i<cMPSessionMaxUsers;i++)
   {
      if (mMemberList[i].mMachineID == machineID)
      {
         nlog(cMPGameCL, "BMPSessionLoadTracker::setPreLoadReady - set %d for machineID %d", loaded ,machineID);
         mMemberList[i].mPreLoadReady = loaded;
         return;
      }
   }
   nlog(cMPGameCL, "BMPSessionLoadTracker::setLoaded - Could not find machine ID [%d]", machineID);
}

//==============================================================================
//
//==============================================================================
void BMPSessionLoadTracker::setLoaded(long machineID, bool loaded)
{
   if (!mInitialized)
   {
      nlog(cMPGameCL, "BMPSessionLoadTracker::setLoaded - loaded tracker not initialized - exiting");
      return;
   }

   for (uint i=0;i<cMPSessionMaxUsers;i++)
   {
      if (mMemberList[i].mMachineID == machineID)
      {
         nlog(cMPGameCL, "BMPSessionLoadTracker::setLoaded - set %d for machineID %d", loaded ,machineID);
         mMemberList[i].mLoaded = loaded;
         return;
      }
   }
   nlog(cMPGameCL, "BMPSessionLoadTracker::setLoaded - Could not find machine ID [%d]", machineID);
}

//==============================================================================
//
//==============================================================================
void BMPSessionLoadTracker::dropMachine(long machineID)
{
   if (!mInitialized)
   {
      nlog(cMPGameCL, "BMPSessionLoadTracker::dropClient - loaded tracker not initialized - exiting");
      return;
   }

   for (uint i=0;i<cMPSessionMaxUsers;i++)
   {
      if (mMemberList[i].mMachineID == machineID)
      {
         nlog(cMPGameCL, "BMPSessionLoadTracker::dropClient - found/dropped load tracking for machineID %d", machineID);
         mMemberList[i].mMachineID = BMachine::cInvalidMachineID;
         return;
      }
   }
   nlog(cMPGameCL, "BMPSessionLoadTracker::dropClient - Could not find machine ID [%d]", machineID);
}

//==============================================================================
//
//==============================================================================
bool BMPSessionLoadTracker::getAreAllPlayersLoaded()
{
   if (!mInitialized)
   {
      nlog(cMPGameCL, "BMPSessionLoadTracker::getAreAllPlayersLoaded - loaded tracker not initialized - returning false");
      return false;
   }

   uint memberCount = 0;
   uint loadedCount = 0;
   for (uint i=0;i<cMPSessionMaxUsers;i++)
   {
      if (mMemberList[i].mMachineID != BMachine::cInvalidMachineID)
      {
         memberCount++;
         if (mMemberList[i].mLoaded)
         {
            loadedCount++;
         }
      }
   }
   nlog(cMPGameCL, "BMPSessionLoadTracker::getAreAllPlayersLoaded - Members:%d  Loaded:%d", memberCount, loadedCount );
   return (memberCount == loadedCount);
}

//==============================================================================
//
//==============================================================================
bool BMPSessionLoadTracker::getAreAllPlayerPreLoadReady()
{
   if (!mInitialized)
   {
      nlog(cMPGameCL, "BMPSessionLoadTracker::getAreAllPlayerPreLoadReady - loaded tracker not initialized - returning false");
      return false;
   }

   uint memberCount = 0;
   uint loadedCount = 0;
   for (uint i=0;i<cMPSessionMaxUsers;i++)
   {
      if (mMemberList[i].mMachineID != BMachine::cInvalidMachineID)
      {
         memberCount++;
         if (mMemberList[i].mPreLoadReady)
         {
            loadedCount++;
         }
      }
   }
   nlog(cMPGameCL, "BMPSessionLoadTracker::getAreAllPlayerPreLoadReady - Members:%d  Loaded:%d", memberCount, loadedCount );
   return (memberCount == loadedCount);
}

//==============================================================================
//
//==============================================================================
void BMPSessionLoadTracker::reset()
{
   mInitialized = false;
   for (uint i=0; i < cMPSessionMaxUsers; i++)
   {
      mMemberList[i].mMachineID = BMachine::cInvalidMachineID;
      mMemberList[i].mLoaded = false;
      mMemberList[i].mPreLoadReady = false;
   }
}

//==============================================================================
//
//==============================================================================
void BMPSessionLoadTracker::initializeMembersToSessionMembers(BSession* pSession)
{
   BDEBUG_ASSERTM(pSession != NULL, "BMPSessionLoadTracker::initializeMembersToSessionMembers -- invalid BSession");

   reset();
   uint foundCount = 0;
   uint count = pSession->getMachineCount();
   for (uint i=0; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 1411
      const BMachine* pMachine = pSession->getMachine(i);
//--
      if (pMachine != NULL && pMachine->isConnected())
      {
         mMemberList[foundCount].mMachineID = i;
         foundCount++;
         BDEBUG_ASSERT(foundCount < cMPSessionMaxUsers);
      }
   }

   nlog(cMPGameCL, "BMPSessionLoadTracker::initializeMembersToSessionMembers - Machines:%d", foundCount);
   mInitialized = true;
}





//************** Player manager interface implementation ****************************

//==============================================================================
//  Utility method
//  Finds the player record that matches the requested network session layer Client ID
//==============================================================================
BMPSessionPlayer* BMPGameSession::getPlayerFromNetworkClientId( ClientID clientID )
{
   BASSERT(getMaxPlayers() <= cMPSessionMaxUsers);

   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      BMPSessionPlayer& player = mClientPlayerMap[i];
      if (player.mClientID == clientID)
      {
         return &player;
      }
   }
   return NULL;
}

//==============================================================================
//  Utility method
//  Finds the player record that matches the requested player ID
//==============================================================================
BMPSessionPlayer* BMPGameSession::getPlayerFromPlayerId( PlayerID playerID )
{
   //BASSERT( getMaxPlayers() <= cMPSessionMaxUsers);
   for(long i=0; i < cMPSessionMaxUsers; i++)
   {
      BMPSessionPlayer& player = mClientPlayerMap[i];
      if (player.mPlayerID == playerID)
      {
         return &player;
      }
   }
   return NULL;
}


//==============================================================================
// Called when the mp session has a new player and needs to lock a player id for them
//==============================================================================
PlayerID BMPGameSession::requestPlayerID(ClientID clientID, const XUID xuid, const BSimString& gamertag)
{
   nlog(cMPGameCL, "BMPGameSession::requestPlayerID for client[%d], xuid[%I64u], name[%s]", clientID, xuid, gamertag.asNative());
   //This is only ran by the host - who tells everyone else what this id is
   BASSERT(isHosting());
   BASSERT(getMaxPlayers() <= cMPSessionMaxUsers);

   //Find the next open playerindex, and do a scan for duplicate name (should never happen - hahaha)
   long playerIndex=-1;
   for (long i=0; i < getMaxPlayers(); i++)
   {
      if (mClientPlayerMap[i].mClientID == cMPInvalidClientID)
      {
         playerIndex = i;
         break;
      }
   }

   if (playerIndex < 0)
   {
      //No valid player handles
      //So at this point we have someone who we approved with joinRequest - but now we are out of handles
      nlog(cMPModeMenuCL, "BMPGameSession::requestPlayerID - no available player slots, not returning a valid player id");
      return cMPInvalidPlayerID;
   }

   //Setup this player record map
   createPlayer(playerIndex+1, clientID, true);
   //mClientPlayerMap[playerIndex].mClientID = clientID;
   //mClientPlayerMap[playerIndex].mLiveEnabled = true;       //TODO - provide a way for so that the party session can set this for AI
   gDatabase.resetPlayer( mClientPlayerMap[playerIndex].mPlayerID );    //reset his settings data

   //mGameView->setLong( PSINDEX(playerID, BGameSettings::cPlayerType), playerType);
   mpMPSession->setUInt64( PSINDEX( mClientPlayerMap[playerIndex].mPlayerID, BGameSettings::cPlayerXUID ), xuid);
   mpMPSession->setString( PSINDEX( mClientPlayerMap[playerIndex].mPlayerID, BGameSettings::cPlayerName), gamertag);

   nlog(cMPGameCL, "BMPGameSession::requestPlayerID - associating client id %d with assigned player id %d", clientID, mClientPlayerMap[playerIndex].mPlayerID);
   mpMPSession->setLong(BGameSettings::cPlayerCount, getPlayerCount() );

   return (mClientPlayerMap[playerIndex].mPlayerID) ;
}

//==============================================================================
// 
//==============================================================================
void BMPGameSession::createPlayer(PlayerID playerID, ClientID clientID, bool liveEnabled)
{
   // playerID starts from 1
   BDEBUG_ASSERTM(playerID > 0 && playerID <= cMPSessionMaxUsers, "Invalid PlayerID");
   if (playerID < 1 || playerID > cMPSessionMaxUsers)
      return;

   // clientID starts from 0
   BDEBUG_ASSERTM(clientID >= 0 && clientID < XNetwork::cMaxClients, "Invalid ClientID");
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
      return;

   nlog(cMPGameCL, "BMPGameSession::createPlayer -- playerID[%d], clientID[%d], liveEnabled[%d]", playerID, clientID, liveEnabled);

   BMPSessionPlayer& player = mClientPlayerMap[playerID-1];
   BDEBUG_ASSERTM(playerID == player.mPlayerID, "PlayerID has been reassigned");
   //player.mPlayerID = playerID;
   player.mClientID = clientID;
   player.mLiveEnabled = liveEnabled;
}

//==============================================================================
//  Called by clients to sync their player manager list with the host's playerID for that gamerTag
//==============================================================================
void BMPGameSession::reservePlayerID(PlayerID pid, const XUID xuid)
{
   //The process is this, the host sees a client as fully connected in the session, he then picks a playerID for him
   //  In the game settings for that player ID - he sets the XUID to that client's XUID
   //  That gets broadcasted out on the wire, and all clients see that
   //  A client that gets that will get this call back 
   //    - It may be they get this callback (the setting of the XUID) BEFORE that client has fully joined
   //    - Or they get the fully joined client BEFORE getting this value
   //  Either is fine


   nlog(cMPGameCL, "BMPGameSession::reservePlayerID - reserve requested for playerID[%d] xuid[%I64u]", pid, xuid);
   //Sanity check - we don't already have that gamerTag in-use do we?
   PlayerID playerSearch = getPlayerIDByGamerTag(xuid);
   if (playerSearch != cMPInvalidPlayerID)
   {
      // since the game settings are updated independently of our calls to updatePendingPlayers()
      // the PlayerID may very well be assigned and therefore we shouldn't release them here
      //Odd - he is already in the list - drop him and reassign
      nlog(cMPGameCL, "BMPGameSession::reservePlayerID - xuid[%I64u] already in the list for playerID[%d]", xuid, playerSearch);
      //releasePlayerID(playerSearch);
   }

   // need to assign this player ID to our pending players list so the updatePendingPlayers() can complete
   for (uint i=0; i < cMPSessionMaxPendingPlayers; i++)
   {
      if (mPendingPlayers[i].xuid == xuid)
      {
         nlog(cMPGameCL, "BMPGameSession::reservePlayerID -- assigning playerID to pending players list playerID[%d], clientID[%d], xuid[%I64u], liveSessionJoined[%d]", pid, mPendingPlayers[i].clientID, xuid, mPendingPlayers[i].liveSessionJoined);
         mPendingPlayers[i].playerID = pid;
         break;
      }
   }

   // *******
   // NOTE:
   // It's ok if we don't have a BMPSessionPlayer yet, we may still be performing a liveSessionJoined
   // that will be updated in updatePendingPlayers()
   // our soul purpose here was updating the pending players list with the player ID we received from the host
   // *******

   //Verify the playerID is ok
   //BMPSessionPlayer* pPlayer = getPlayerFromPlayerId(pid);
   //if (!pPlayer)
   //{
   //   //So I've been told to reserve this and I can't find that pid in my list... wtf
   //   nlog(cMPGameCL, "BMPGameSession::reservePlayerID - can not find open player slot %d - resetting system", pid);
   //   BASSERT(false);  
   //   return;
   //}

   //Ok - and we don't already have that playerID locked do we?
   //if (pPlayer->mClientID != cMPInvalidClientID)
   //{
   //   //What?  So the host told us to reserve a playerID that is already in use 
   //   nlog(cMPGameCL, "BMPGameSession::reservePlayerID - playerID %d already active with valid client ID %d, kicking that client", pPlayer->mClientID);
   //   BASSERT(false);
   //   getSession()->kickClient(pPlayer->mClientID);
   //   releasePlayerID(pid);      
   //}

   // this should be handled by the host!

   //Setup this player record map
   //player->mClientID = clientID;
   //player->mLiveEnabled = true;       //TODO - provide a way for so that the party session can set this for AI
   //gDatabase.resetPlayer( player->mPlayerID );    //reset his settings data
   //mGameView->setLong( PSINDEX(playerID, BGameSettings::cPlayerType), playerType);
   //mpMPSession->setUInt64( PSINDEX( mClientPlayerMap[pid].mPlayerID, BGameSettings::cPlayerXUID ), xuid);
   //mpMPSession->setString( PSINDEX( mClientPlayerMap[pid].mPlayerID, BGameSettings::cPlayerName), gamertag);

   //nlog(cMPGameCL, "BMPGameSession::reservePlayerID - Set up playerID[%d] for xuid[%I64u]", pid, xuid);
}

//==============================================================================
//  Called with the mp session is done with a player id and it can be released
//==============================================================================
void BMPGameSession::releasePlayerID( PlayerID id )
{
   //Safe to call multiple times
   nlog(cMPGameCL, "BMPGameSession::releasePlayerID - releasing player ID %d", id );

   BMPSessionPlayer* player = getPlayerFromPlayerId( id );
   if (player)
   {
      player->mClientID = cMPInvalidClientID;
   }
   else
   {
      nlog(cMPGameCL, "BMPGameSession::releasePlayerID - Release FAILED - could not find INUSE player ID %d", id );
   }

   if (isHosting())
   {
      mpMPSession->setLong(BGameSettings::cPlayerCount, getPlayerCount() );
   }
}

//==============================================================================
// Reset everything associated with the player list
//==============================================================================
void BMPGameSession::resetPlayerSystem()
{
   nlog(cMPModeMenuCL, "BMPGameSession::resetPlayerSystem - resetting all player slot data");
   for (uint i=0; i < cMPSessionMaxUsers; i++)
   {
      mClientPlayerMap[i].mClientID = cMPInvalidClientID;
      gDatabase.resetPlayer(mClientPlayerMap[i].mPlayerID);
      if (mClientPlayerMap[i].mpTrackingDataBlock)
      {
         delete mClientPlayerMap[i].mpTrackingDataBlock;
         mClientPlayerMap[i].mpTrackingDataBlock = NULL;
      }
   }
   if (isHosting())
   {
      mpMPSession->setLong(BGameSettings::cPlayerCount, getPlayerCount() );
   }
}

//==============================================================================
// 
//==============================================================================
bool BMPGameSession::isClient(PlayerID id)
{
//-- FIXING PREFIX BUG ID 1397
   const BMPSessionPlayer* player = getPlayerFromPlayerId(id);
//--
   if (!player || (player->mClientID==cMPInvalidClientID))
   {
      return false;
   }
   return true;
}

//==============================================================================
//  Finds the playerID that matches the requested network session layer Client ID
//==============================================================================
PlayerID BMPGameSession::getPlayerID(ClientID networkClientID)
{
//-- FIXING PREFIX BUG ID 1398
   const BMPSessionPlayer* pSessionPlayer = getPlayerFromNetworkClientId(networkClientID);
//--
   if (pSessionPlayer)
   {
      return pSessionPlayer->mPlayerID;
   }
   return cMPInvalidPlayerID;
}

//==============================================================================
//  Finds the playerID that matches the requested gamerTag
//==============================================================================
PlayerID BMPGameSession::getPlayerIDByGamerTag( XUID xuid )
{
   BASSERT( getMaxPlayers() <= cMPSessionMaxUsers);
   nlog(cMPGameCL, "BMPGameSession::getPlayerIDByGamerTag - called for xuid %I64u", xuid );
   for(long i=0; i<getMaxPlayers(); i++)
   {
      const BMPSessionPlayer& player = mClientPlayerMap[i];
      //NOTE : In this version I ALWAYS want to lookup the playerID from an XUID, whether or not it has a client connection
      //  This is because this query is used to hook up this data at session connection/startup
      //if (player->mClientID !=cMPInvalidClientID)
      {
         //valid entry - check the XUID
         XUID testXuid;
         mpGameSettings->getUInt64( PSINDEX(player.mPlayerID, BGameSettings::cPlayerXUID), testXuid);
         if (testXuid == xuid)
         {
            nlog(cMPModeMenuCL, "BMPGameSession::getPlayerIDByGamerTag - Found XUID, playerID %d", player.mPlayerID);
            return player.mPlayerID;
         }
      }
   }
   return cMPInvalidPlayerID;
}

//==============================================================================
//  Sets the ready to start game on/off for a particular player
// FIXME-COOP - need to either change this method or the upstream to convert
//              from a BMachineID to the clientIDs/playerIDs since we can
//              now have two clients per machine
//==============================================================================
void BMPGameSession::setReadyToStart(BMachineID machineID, bool setReady)
{
   BDEBUG_ASSERTM(mpMPSession, "Missing multiplayer session");
   if (mpMPSession == NULL)
      return;

   nlog(cMPGameCL, "BMPGameSession::setReadyToStart - set ready requested for machineID[%d]", machineID);

   // XXX setting a player's ready flag should only be performed for the local client
   // and then let the setting propagate to the other clients
   //
   // the host may clear all the ready flags
   //
   // if we don't have a BClient for this ClientID then force them to unready
   //
   // if the client is not local then don't bother setting the bool, instead let the game settings propagate the value
//-- FIXING PREFIX BUG ID 1399
   const BMachine* pMachine = getSession()->getMachine(machineID);
//--
   if (pMachine == NULL)
   {
      nlog(cMPGameCL, "BMPGameSession::setReadyToStart - missing machineID[%d]", machineID);
      return;
   }
   else if (!pMachine->isLocal())
   {
      nlog(cMPGameCL, "BMPGameSession::setReadyToStart - not local machineID[%d]", machineID);
      return;
   }

//-- FIXING PREFIX BUG ID 1400
   const BMPSessionPlayer* pPlayer = NULL;
//--
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      pPlayer = getPlayerFromNetworkClientId(pMachine->mUsers[i].mClientID);
      if (pPlayer)
         break;
   }
   if (pPlayer)
   {
      nlog(cMPGameCL, "BMPGameSession::setReadyToStart - setting playerID[%d] to ready", pPlayer->mPlayerID);
      mpMPSession->setBool(PSINDEX(pPlayer->mPlayerID, BGameSettings::cPlayerReady), setReady);
   }
   else
   {
      //Lots of spam here - I want to validate that this condition happens, and when - eric
#ifndef BUILD_FINAL
      nlog(cMPGameCL, "BMPGameSession::setReadyToStart - Warning, I have a ready to start event from someone I do NOT have a player record for. machineID[%d]", machineID);
#endif
   }
}

//==============================================================================
//  Returns true if all player are ready to start
//==============================================================================
bool BMPGameSession::getAreAllPlayersReadyToStart()
{
   if (!isRunning() || !getSession())
   {
      //Little sanity check for shutdown conditions
      return false;
   }

   long maxPlayerCount = getMaxPlayers();
   //Change to switch this to looking at the gamedb player count - for AI player support - eric
   mpGameSettings->getLong(BGameSettings::cMaxPlayers, maxPlayerCount);

   BASSERT(maxPlayerCount <= cMPSessionMaxUsers);

   if (maxPlayerCount == 0)
   {
      return false;
   }

   for (long i=0; i < maxPlayerCount; i++)
   {
      long playerID = i+1;
      bool ready = false;

      mpGameSettings->getBool(PSINDEX(playerID, BGameSettings::cPlayerReady), ready);

      if (!ready)
      {
         return false;
      }
   }

   nlog(cMPGameCL, "BMPGameSession::getAreAllPlayersReadyToStart - returning TRUE" );
   return true;
}

//==============================================================================
//  Resets the loaded/ready flags for all players
//==============================================================================
void BMPGameSession::resetStartState()
{
   if (!gLiveSystem->getMPSession())
   {
      //If I get this - its because I'm shutting down
      return;
   }
   nlog(cMPGameCL, "BMPGameSession::resetStartState" );
   BASSERT( getMaxPlayers() <= cMPSessionMaxUsers);
   for(long i=0; i<getMaxPlayers(); i++)
   {
      gLiveSystem->getMPSession()->setBool(PSINDEX(i+1, BGameSettings::cPlayerReady), false);
   }
}


//==============================================================================
//
//==============================================================================
void BMPGameSession::joinRequest(const BSessionUser users[], BSession::BJoinReasonCode &result)
{
   //Go through and make sure the name is unique
   BASSERT(getMaxPlayers() <= cMPSessionMaxUsers);
   nlog(cMPGameCL, "BMPGameSession::joinRequest - called for xuid[%I64u], xuid[%I64u]", users[0].mXuid, users[1].mXuid);
   uint openSlots = 0;
   for(long i=0; i<getMaxPlayers(); i++)
   {
      const BMPSessionPlayer& player = mClientPlayerMap[i];
      if (player.mClientID != cMPInvalidClientID)
      {
         //valid entry - check the XUID
         XUID testXuid;
         mpGameSettings->getUInt64( PSINDEX(player.mPlayerID, BGameSettings::cPlayerXUID), testXuid);
         if (testXuid == users[0].mXuid || testXuid == users[1].mXuid)
         {
            nlog(cMPModeMenuCL, "BMPGameSession::joinRequest - rejecting join request duplicate xuid[%I64u]", testXuid);
            result = BSession::cResultRejectUserExists;
            return;
         }
      }
      else
      {
         openSlots++;
      }
   }

   // if we don't have enough room for both players, then reject them both
   if (openSlots == 0 || (openSlots == 1 && users[0].mXuid != 0 && users[1].mXuid != 0))
   {
      nlog(cMPModeMenuCL, "BMPGameSession::joinRequest - rejecting join request FULL openSlots[%d]", openSlots);
      result = BSession::cResultRejectFull;
      return;
   }

   nlog(cMPGameCL, "BMPGameSession::joinRequest - approving join request");
   result = BSession::cResultJoinOk;
}

//==============================================================================
//
//==============================================================================
long BMPGameSession::getPlayerCount(void)
{
   //# of players are the number of entries in the client to player map
   //TODO - fix this once we have AI players because it will be wrong (for some cases)
   //  We'll have to go through and look at the callers to see who is intested in what
   uint count = 0;
   for(long i=0; i<getMaxPlayers(); i++)
   {
      if (mClientPlayerMap[i].mClientID !=cMPInvalidClientID)
      {
         count++;
      }
   }
   return count;
}

//==============================================================================
//
//==============================================================================
void BMPGameSession::preGameDebugSpam()
{
   //Report on what we have for players and game settings
   nlog(cMPGameCL, "BMPGameSession::preGameDebugSpam");

   BSimString map;
   mpGameSettings->getString(BGameSettings::cMapName, map);

   long maxPlayerCount;
   mpGameSettings->getLong(BGameSettings::cMaxPlayers, maxPlayerCount);

   long playerCount;
   mpGameSettings->getLong(BGameSettings::cPlayerCount, playerCount);

   bool coop;
   mpGameSettings->getBool(BGameSettings::cCoop, coop);

   bool rgame;
   mpGameSettings->getBool(BGameSettings::cRecordGame, rgame);

   BSimString gameID;
   mpGameSettings->getString(BGameSettings::cGameID, gameID);

   long randSeed;
   mpGameSettings->getLong(BGameSettings::cRandomSeed, randSeed);

   long gametype;
   mpGameSettings->getLong(BGameSettings::cGameType, gametype);

   long gameMode;
   mpGameSettings->getLong(BGameSettings::cGameMode, gameMode);
   
   nlog(cMPGameCL, "Players:%d Max:%d Coop:%d GameType:%d GameMode:%d RecGame:%d Map:%s VinceID:%s Rand:%d", playerCount, maxPlayerCount, coop, gametype, gameMode, rgame, map.getPtr(), gameID.getPtr(), randSeed);

   BLiveSessionGameClassification gclass = cLiveSessionGameClassificationUnknown;
   if (mpMPSession && mpMPSession->getPartySession())
   {
      switch (mpMPSession->getPartySession()->getCurrentHostSettings().mPartyRoomMode)
      {
         case BModePartyRoom2::cPartyRoomModeCustom:
            gStatsManager.setGameType(BStats::eCustom);
            gclass = cLiveSessionGameClassificationCustom;
            break;
         case BModePartyRoom2::cPartyRoomModeMM:
            gStatsManager.setGameType(BStats::eMatchMaking);
            gclass = cLiveSessionGameClassificationMatchmade;
            break;
         case BModePartyRoom2::cPartyRoomModeCampaign:
            gStatsManager.setGameType(BStats::eUnknown);
            gclass = cLiveSessionGameClassificationCampaign;
            break;
         default:
            gStatsManager.setGameType(BStats::eUnknown);
            break;
      }
   }
   long maxDifficulty = 0;

   for(long i=0; i<getMaxPlayers(); i++)
   {
      const BMPSessionPlayer& player = mClientPlayerMap[i];

      gStatsManager.setPlayerID(player.mClientID, player.mPlayerID);

      XUID testXuid;
      mpGameSettings->getUInt64( PSINDEX(player.mPlayerID, BGameSettings::cPlayerXUID ), testXuid);

      long team;
      mpGameSettings->getLong(PSINDEX(player.mPlayerID, BGameSettings::cPlayerTeam), team);

      long civ;
      mpGameSettings->getLong(PSINDEX(player.mPlayerID, BGameSettings::cPlayerCiv), civ);

      long leader;
      mpGameSettings->getLong(PSINDEX(player.mPlayerID, BGameSettings::cPlayerLeader), leader);

      bool ready;
      mpGameSettings->getBool(PSINDEX(player.mPlayerID, BGameSettings::cPlayerReady), ready);

      long playerType;
      mpGameSettings->getLong( PSINDEX(player.mPlayerID, BGameSettings::cPlayerType), playerType);

      BSimString gamertag;
      mpGameSettings->getString(PSINDEX(player.mPlayerID, BGameSettings::cPlayerName), gamertag);

      long difficulty;
      mpGameSettings->getLong( PSINDEX(player.mPlayerID, BGameSettings::cPlayerDifficultyType), difficulty);
      if (difficulty>maxDifficulty)
      {
         maxDifficulty=difficulty;
      }

      nlog(cMPGameCL, " Slot %d - ClientID %d - PlayerID %d - LiveE %d - XUID %I64u - Tag:%s", 
         i, player.mClientID, player.mPlayerID, player.mLiveEnabled, testXuid, gamertag.getPtr());
      nlog(cMPGameCL, "   Team:%d - Civ:%d - Leader:%d - Ready:%d - PType:%d:", team,civ,leader,ready,playerType);
   }

   //Some general presence contexts we can set at this point
   gLiveSystem->setPresenceContext(PROPERTY_GAMESCORE, 0, true);
   gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, maxDifficulty);
   gLiveSystem->setPresenceContext(PROPERTY_GAMETIMEMINUTES, 0, true);
   gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_HWGAMEMODE, gameMode);
   gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_PLAYINGSKIRM);
   switch (getMaxPlayers())
   {
      case (2) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_1V1);break;
      case (4) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_2V2);break;
      case (6) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_3V3);break;
   }
   
   //Let the session know some details about this about to start game, it needs them for leaderboards later
   if (gclass != cLiveSessionGameClassificationUnknown)
   {
      if (mpMPSession->isInLANMode())
      {         
         gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_NETWORK, CONTEXT_PRE_CON_NETWORK_LOCAL);
      }
      else
      {
         gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_NETWORK, CONTEXT_PRE_CON_NETWORK_LIVE);
      }

      bool partySetTeam = false;
      uint campaignMapIndex = 0;

      if (gclass==cLiveSessionGameClassificationCampaign)
      {
         //In a campaign game - if there is more than one dude in the party - its a CO-OP game
         if (mpMPSession->getPartySession()->getPartyCount() >1 )
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNCOOPPLAY);
            partySetTeam = true;
         }
         else
         {
            gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNSOLOPLAY);
         }

         //Need to figure out which of the 15 campaign maps this is
         campaignMapIndex = gCampaignManager.getLeaderboardIndex(map);
         gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, campaignMapIndex, true);
      }
      else if (gclass==cLiveSessionGameClassificationMatchmade)
      {
         //In a matchmade game - if there is more than one dude in the party - its a CO-OP game
         if (mpMPSession->getPartySession()->getPartyCount() >1 )
         {
            partySetTeam = true;
         }
      }
 
      if (mpLiveSession)
      {
         //Note: Internal game types are 0 based (0=standard), Live context is also 0, but leaderboards start at 1=standard
         mpLiveSession->storeStatsGameSettings(gclass, partySetTeam, campaignMapIndex, gameMode+1, (uint)maxDifficulty+1);
      }
   }
}
