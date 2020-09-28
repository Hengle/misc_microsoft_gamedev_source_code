//==============================================================================
// mpgame.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "mpgame.h"
#include "mpplayer.h"
#include "mpgameview.h"
#include "mpgamedescriptor.h"
#include "mppackets.h"
#include "DataSet.h"
#include "Channels.h"
#include "Channel.h"
#include "OrderedChannel.h"
#include "filetransfermgr.h"
#include "connectivity.h"
#include "mpsimobject.h"
#include "filetransfermgr.h"
#include "config.h"
#include "commlog.h"
#include "mpcommheaders.h"
#include "MPGameView.h"
#include "MultiplayerImpl.h"
#include "SocksHelper.h"
#include "econfigenum.h"
#include "liveSystem.h"
//#include "..\core\core.h"

//==============================================================================
// Defines
const DWORD cMaxHostTimeout            = 15000;      // how long do we wait for the host to lag out before cancelling?
const DWORD cMaxLoadingTime            = 90000;      // you'd think we could load a game in 45 seconds right?. No, apparently we need more. 1.5 mins maybe?
const DWORD cConnectTimeout            = 10000;      // 10 seconds long enough for you to connect?
const BCHAR_T* cMPGameChannelName = B("MPGAME:CHANNEL");

//==============================================================================
// BMPGame::BMPGame
//==============================================================================
BMPGame::BMPGame(BConnectivity *connectivity) : 
   BMPSyncObject(),
   mSessionConnector(NULL),
   mSession(NULL),
   mbSettingsLocked(false),
   mbUpdateLocalSettings(true),
   mConnectivity(connectivity),
   mGameSettings(NULL),
   mGameState(cGameStateInvalid),
   mChecksum(0),
   mSimObject(NULL),
   mFileTransferMgr(NULL),   
   mSessionConnected(false),
   mSettingsComplete(false),
   mGameConnected(false),
   mLockedPlayers(0),
   mControlledPlayer(-1),
   mMaxPlayers(2),
   mLaunchCountdown(0),
   mLaunchRequestTime(0),
   mLaunchLastUpdate(0),
   mChatChannelName(cMPGameChannelName),
   mJoinRequestHandler(NULL)
{
   BFATAL_ASSERT(mConnectivity);
   for (long idx=0; idx<cChannelMax; idx++)
   {
      mChannelArray[idx] = NULL;
   }

   registerMPCommHeaders();

   BMultiplayer *mp = BMultiplayer::getInstance(); BFATAL_ASSERT(mp);
   /*MPChatImpl*chat = (MPChatImpl*)mp->getChat();
   if (chat)
      chat->addChannel(this);*/
}

//==============================================================================
// BMPGame::~BMPGame
//==============================================================================
BMPGame::~BMPGame()
{
   /*BMultiplayer *mp = BMultiplayer::getInstance(); BFATAL_ASSERT(mp);
   if (mp->getChat())
      ((MPChatImpl*)mp->getChat())->removeChannel(this);*/

   if (getSession() && getSession()->isConnected())
      sessionDisconnected(BSession::cGameDeleted); // notify observers      

   cleanup();

   mMPGameObserverList.gameDestroyed();
}

//==============================================================================
// BMPGame::setJoinRequestHandler
//==============================================================================
void BMPGame::setJoinRequestHandler(BMPJoinRequestHandler* handler)
{
   // just override? Sure why not.
   mJoinRequestHandler = handler;
}

//==============================================================================
// BMPGame::createView
//==============================================================================
BMPGameView* BMPGame::createView(void)
{
   if (!mGameSettings || !mChecksum)
      return(NULL);

   BMPGameView* view = new BMPGameView(this, mGameSettings);
   if (!view)
      return(NULL);

   return(view);
}

//==============================================================================
// BMPGame::destroyView
//==============================================================================
void BMPGame::destroyView(BMPGameView *view)
{
   if (!view || !mGameSettings)
      return;

   delete view;
}

//==============================================================================
// BMPGame::initialize
//==============================================================================
void BMPGame::initialize(BDataSet *dataSet, DWORD checksum)
{
   mGameSettings = dataSet;
   mChecksum = checksum;   
}

//==============================================================================
// BMPGame::host
//==============================================================================
HRESULT BMPGame::host(const BSimString& localname, const BMPGameDescriptor &descriptor)
{
   if (!mGameSettings || !mChecksum)
      return(HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT)); // wtf? 

   cleanup();

   BMultiplayer::getInstance()->setGameState(cGameStateSetup);

   mSession = new BSession(descriptor.getChecksum(), this, this);   
   if (!getSession())
      return(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
   mSession->addObserver(this);
   mSession->setMaxClientCount(mMaxPlayers);
   
   getSession()->setLocalAddress(mConnectivity->getLocalAddress());
   getSession()->setTranslatedLocalAddress(mConnectivity->getTranslatedLocalAddress());

   HRESULT hr = getSession()->host(localname, descriptor.getName(), 0);
   if (hr != S_OK)
   {
      nlog(cMPGameCL, "BMPGame::host -- failed getSession->host. hr[0x%x]", hr);
      cleanup();
      return(hr);
   }

   // See if its LAN or if its Live
   if (BMultiplayer::getInstance()->getConnectType() == BMultiplayer::cLiveConnectType )
   {
	   //Its live - need to build a session for it and post
	   //Need to build that into xLive then expose it here + a call back 
	   gLiveSystem->getInstance()->advertiseSession();
   }
   else
   {
   
	   // advertise the game
	   if (descriptor.getAdvertiseGame())
	   {
		  if (!mSessionConnector)
			 mSessionConnector = new BSessionConnector(this);

		  hr = mSessionConnector->advertise( mConnectivity->getLocalAddress(), mConnectivity->getTranslatedLocalAddress(), 
										(void*)&descriptor, sizeof(descriptor), mChecksum );
		  if(FAILED(hr))
			 nlog(cMPGameCL, "MPGame::host -- failed to advertise game");
	   }
   }

   sessionConnected();

   nlog(cMPGameCL, "BMPGame::host -- host OK");
   return(S_OK);
}

//==============================================================================
// BMPGame::join
//==============================================================================
HRESULT BMPGame::join(const BSimString& localname, const BMPGameDescriptor &descriptor)
{
   if (!mGameSettings || !mChecksum)
      return(HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT));
   
   cleanup();

   BMultiplayer::getInstance()->setGameState(cGameStateSetup);

   mSession = new BSession(descriptor.getChecksum(), this, this);
   if (!getSession())
      return(HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY));
   mSession->addObserver(this);

   getSession()->setLocalAddress(mConnectivity->getLocalAddress());
   getSession()->setTranslatedLocalAddress(mConnectivity->getTranslatedLocalAddress());

   HRESULT hr = getSession()->connect(localname, descriptor.getAddress(), descriptor.getTranslatedAddress());
   if (hr != S_OK)
   {
      nlog(cMPGameCL, "BMPGame::Join -- failed getSession()->connect. hr[0x%x]", hr);
      cleanup();
      return(hr);
   }
   
   sessionConnected();

   nlog(cMPGameCL, "BMPGame::Join -- join OK");
   return(S_OK);
}

//==============================================================================
// BMPGame::setSlotOpen
//==============================================================================
bool BMPGame::setSlotOpen(long slot, bool open)
{
   if (!getSession())
      return(false);

   return(getSession()->setSlotOpen(slot, open));
}

//==============================================================================
// BMPGame::isHosting
//==============================================================================
bool BMPGame::isHosting(void) const
{
   if (!getSession())
      return(false);

   return(getSession()->isHosted());
}

//==============================================================================
// BMPGame::requestGameLaunch
//==============================================================================
bool BMPGame::requestGameLaunch(DWORD countdown)
{
   // if not hosting, or already launching.
   if (!isHosting() || mLaunchCountdown > 0)
      return(false);

   if (!mChannelArray[cChannelMessage])
      return(false);

   nlog(cMPGameCL, "BMPGame::requestGameLaunch -- enter.");

   if (mPlayers.getNumber() <= 0) //DJB 6-10-05: Changed to 0, since you can now start an MP game with just 1 player and 1 AI.
   {
      nlog(cMPGameCL, "BMPGame::requestGameLaunch -- need a player.");
      return(false);
   }

   mLaunchCountdown = countdown;
   mLaunchRequestTime = timeGetTime();
   mLaunchLastUpdate = mLaunchRequestTime - cLaunchUpdateFrequency;

   BCompleteSettingsPacket spacket(BChannelPacketType::cFinalSettingsPacket);
   fillCompleteSettings(spacket);
   mChannelArray[cChannelSettings]->SendPacket(spacket);
   nlog(cMPGameCL, "BMPGame::requestGameLaunch -- Sending cFinalSettingsPacket");

   BMessagePacket packet(1, BChannelPacketType::cLockSettingsPacket);
   nlog(cMPGameCL, "BMPGame::requestGameLaunch -- Sending cLockSettingsPacket");
   return(mChannelArray[cChannelMessage]->SendPacket(packet)==S_OK);   
}

//==============================================================================
// BMPGame::requestLaunchAbort
//==============================================================================
bool BMPGame::requestLaunchAbort(long reason)
{   
   if (!mChannelArray[cChannelMessage])
      return(false);

   nlog(cMPGameCL, "BMPGame::requestLaunchAbort -- Sending cLaunchAbortRequestPacket");
   BMessagePacket packet((long)reason, BChannelPacketType::cLaunchAbortRequestPacket);
   return(mChannelArray[cChannelMessage]->SendPacket(packet)==S_OK);   
};

//==============================================================================
// BMPGame::sendLaunchReady
//==============================================================================
bool BMPGame::sendLaunchReady(bool ready)
{
   if (!mChannelArray[cChannelMessage])
      return(false);

   nlog(cMPGameCL, "BMPGame::requestLaunchAbort -- Sending cLaunchReadyPacket");
   BMessagePacket packet((long)ready?1:0, BChannelPacketType::cLaunchReadyPacket);

   return(mChannelArray[cChannelMessage]->SendPacket(packet)==S_OK);   
}

//==============================================================================
// BMPGame::startGame
//==============================================================================
bool BMPGame::startGame(void)
{
   if (!getSession() || !getSession()->isConnected())
      return(false);
   
   nlog(cMPGameCL, "BMPGame::startGame -- Enter");

   //
   getSession()->disableAllPings(true);

   if (getSession()->getTimeSync())
      getSession()->getTimeSync()->startingGame();
   
   delete mSessionConnector;
   mSessionConnector = NULL;

   return(true);
}

//==============================================================================
// BMPGame::stopGame
//==============================================================================
bool BMPGame::stopGame(void)
{
   if (!getSession() || !getSession()->isConnected())
      return(false);

   nlog(cMPGameCL, "BMPGame::stopGame -- Enter");

   getSession()->disableAllPings(true);

   if (getSession()->getTimeSync())
      getSession()->getTimeSync()->gameStopped();

   BMultiplayer::getInstance()->setGameState(cGameStateSetup);

   // unlock settings.
   if (mChannelArray[cChannelMessage])
   {
      BMessagePacket spacket(0, BChannelPacketType::cLockSettingsPacket);
      mChannelArray[cChannelMessage]->SendPacket(spacket);
   }

   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
      if (mPlayers[idx] != NULL)
         mPlayers[idx]->reset();

   return(false);
}

//==============================================================================
// BMPGame::gameStartComplete
//==============================================================================
void BMPGame::gameStartComplete(void)
{
   nlog(cMPGameCL, "BMPGame::gameStartComplete -- Enter");
   if (getSession()->getTimeSync())
      getSession()->getTimeSync()->gameStarted();

}

//==============================================================================
// BMPGame::waitingOnOtherPlayers
// This is called when the local game is ready to finalize
//==============================================================================
bool BMPGame::waitingOnOtherPlayers(void)
{
   if (!getSession())
      return(false);

   nlog(cMPGameCL, "BMPGame::waitingOnOtherPlayers -- Send cGameFinalizePacket");

   BTypedPacket packet(BPacketType::cGameFinalizePacket);
   getSession()->SendPacket(packet); 
   
   return(true);
}

//==============================================================================
// BMPGame::finalizeGame
// This is called when game is in progress
//==============================================================================
bool BMPGame::finalizeGame(void)
{
   if (!getSession())
      return(false);

   if (!allPlayersLoaded())
      return(false);

   nlog(cMPGameCL, "BMPGame::finalizeGame -- all players loaded. setting game state cGameStateInProgress.");

   getSession()->disableAllPings(false);

   BMultiplayer::getInstance()->setGameState(cGameStateInProgress);

   return(true);
}

//==============================================================================
// BMPGame::attachFileXfer
//==============================================================================
bool BMPGame::attachFileXfer(BFileTransferGameInterface *xfer)
{
   if (!xfer || !getSession())
      return(false);

   if (!mFileTransferMgr)
      mFileTransferMgr = new BFileTransferMgr(getSession(), this);

   mFileTransferMgr->attachGameInterface(xfer);
   xfer->attachFileTransferMgr(mFileTransferMgr);

   return(true);
}

//==============================================================================
// BMPGame::getFileXfer
//==============================================================================
BFileTransferGameInterface* BMPGame::getFileXfer()
{
   if (!mFileTransferMgr)
      return NULL;

   return mFileTransferMgr->getGameInterface();
}

//==============================================================================
// BMPGame::getClientIDFromPlayerID
//==============================================================================
DWORD BMPGame::getClientIDFromPlayerID(long playerID)
{
   return (DWORD)getClientID((PlayerID)playerID);
}

//==============================================================================
// BMPGame::getPlayerIDFromClientID
//==============================================================================
long BMPGame::getPlayerIDFromClientID(DWORD clientID)
{
   return (long)getPlayerID((ClientID)clientID);
}

//==============================================================================
// BMPGame::setMaxPlayers
//==============================================================================
void BMPGame::setMaxPlayers(long maxCount)
{ 
   mMaxPlayers=maxCount;

   if (getSession())
      getSession()->setMaxClientCount(mMaxPlayers);
}

//==============================================================================
// BMPGame::getPlayerCount
//==============================================================================
long BMPGame::getPlayerCount(void) const
{
   long total = 0;
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (mPlayers[idx])
         total++;
   }
   return(total);
}

//==============================================================================
// BMPGame::getClientID
//==============================================================================
ClientID BMPGame::getClientID(PlayerID playerID) const
{
   return(mPlayerMapper.getClientID(playerID));
}

//==============================================================================
// BMPGame::getPlayerID
//==============================================================================
long BMPGame::getPlayerID(ClientID clientID) const
{
   return(mPlayerMapper.getPlayerID(clientID));
}

//==============================================================================
// BMPGame::getPlayerID
//==============================================================================
long BMPGame::getPlayerID(const BSimString &playerName) const
{
   return(mPlayerMapper.getPlayerID(playerName));
}

//==============================================================================
// BMPGame::getPlayerName
//==============================================================================
const BSimString* BMPGame::getPlayerName(ClientID clientID) const
{
   return(mPlayerMapper.getName(clientID));
}

//==============================================================================
// BMPGame::allPlayersLoaded
//==============================================================================
bool BMPGame::allPlayersLoaded(void) const
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (!mPlayers[idx])
         continue;

      if (!mPlayers[idx]->getLoaded())
         return(false);
   }

   nlog(cMPGameCL, "BMPGame::allPlayersLoaded -- true");
   return(true);
}

//==============================================================================
// BMPGame::allPlayersReadyToStart
//==============================================================================
bool BMPGame::allPlayersReadyToStart(void) const
{
   long count = mPlayers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (!mPlayers[idx])
         continue;

      if (!mPlayers[idx]->getReadyToStart())
         return(false);
   }

   nlog(cMPGameCL, "BMPGame::allPlayersReadyToStart -- true");
   return(true);
}

//==============================================================================
// BMPGame::getLocalPlayer
//==============================================================================
long BMPGame::getLocalPlayer(void) const
{
   if (!getSession())
      return(-1);

   return(getSession()->getLocalClientID());
}

//==============================================================================
// BMPGame::kickPlayer
//==============================================================================
void BMPGame::kickPlayer(PlayerID playerID)
{
   if (!getSession())
      return;

   HRESULT hr = getSession()->kickClient(getClientID(playerID));
   if(FAILED(hr))
      nlog(cMPGameCL, "MPGame::kickPlayer -- failed to kick player ID[%d] client ID[%d]", playerID, getClientID(playerID));

}

//==============================================================================
// BMPGame::castVote
//==============================================================================
void BMPGame::castVote(long type, long vote, long arg1)
{
   if (!getSession())
      return;

   nlog(cMPGameCL, "BMPGame::castVote -- casting type[%d] vote [%d] arg1 [%d]", type, vote, arg1);

   BVotePacket packet(BChannelPacketType::cVoteResult, type, vote, arg1);
   mChannelArray[cChannelVote]->SendPacket(packet);
}

//==============================================================================
// BMPGame::startVote
//==============================================================================
void BMPGame::startVote(long type, long arg1)
{
   if (!getSession())
      return;

   nlog(cMPGameCL, "BMPGame::startVote -- starting vote type [%d] arg1 [%d]", type, arg1);

   BVotePacket packet(BChannelPacketType::cStartVote, type, -1, arg1);
   mChannelArray[cChannelVote]->SendPacket(packet);
}

//==============================================================================
// BMPGame::startVote
//==============================================================================
void BMPGame::abortVote(long type, long arg1)
{
   if (!getSession())
      return;

   nlog(cMPGameCL, "BMPGame::abortVote -- abort vote type [%d] arg1 [%d]", type, arg1);

   BVotePacket packet(BChannelPacketType::cAbortVote, type, -1, arg1);
   mChannelArray[cChannelVote]->SendPacket(packet);
}

//==============================================================================
// BMPGame::setLocalUpdates
//==============================================================================
void BMPGame::setLocalUpdates(bool val)
{
   mbUpdateLocalSettings = val;
}

//==============================================================================
// BMPGame::setSetting
//==============================================================================
bool BMPGame::setSetting(DWORD index, void *data, long size)
{
   if (!mGameSettings || !getSession())
      return(false);

   if (mbSettingsLocked)
      return(false);   

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
      if (mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getHostClient(), packet) != S_OK)
         return(false);      
   }

   if (mbUpdateLocalSettings)
      return(mGameSettings->setData(index, data, (WORD)size));

   return(true);
}

#ifndef BUILD_FINAL
//==============================================================================
// BMPGame::setSettingOverride
// Don't use this in a final release build. 
//==============================================================================
bool BMPGame::setSettingOverride(DWORD index, void *data, long size)
{
   if (!mGameSettings || !getSession())
      return(false);

   BSettingsPacket packet(index, data, size);
   packet.mOverrideLock = true;

   if (mChannelArray[cChannelSettings]->SendPacket(packet) != S_OK)
      return(false);

   return(true);
}
#endif   

//==============================================================================
// BMPGame::service
//==============================================================================
void BMPGame::service(void)
{
   if (getSession())
      getSession()->service(); // causes events to fire
   
   for (long idx=0; idx<cChannelMax; idx++)
   {
      if (mChannelArray[idx])
         mChannelArray[idx]->service();
   }

   // timeout connect requests
   DWORD time = timeGetTime();
   long count = mConnectingAddresses.getNumber();
   for (long idx=count-1; idx>=0; idx--)
   {
      if ((time - mConnectingAddresses[idx].requestTime) > cConnectTimeout)
      {
         mConnectingAddresses.removeIndex(idx);
      }
   }

   switch (mGameState)
   {
      case cGameStateSetup:
      {
         if (mFileTransferMgr)
            mFileTransferMgr->service();

         if (mSessionConnector)
            mSessionConnector->service();

         // if the game isn't connected, try to connect it
         if (!mGameConnected)
         {
            // session is connected and settings have been received
            if (mSessionConnected && mSettingsComplete)
            {
               nlog(cMPGameCL, "BMPGame::service -- session connected && settings complete.");

               // tell the higher levels about the players
               count = mPlayers.getNumber();
               for (long player=0; player<count; player++)
               {
                  if (!mPlayers[player])
                     continue;

                  nlog(cMPGameCL, "BMPGame::service -- connect player [%d].", player);
                  connectPlayer(player);
               }

               mGameConnected = true;

               nlog(cMPGameCL, "BMPGame::service -- game connected.");
               mMPGameObserverList.gameConnected(this);
            }
         }
      }
      break;

      case cGameStateLaunching:
      {
         if (mFileTransferMgr)
            mFileTransferMgr->service();

         // only the host needs to drive this
         if (!isHosting())
            break;

         // if we are launching, keep sending launch packets
         if (mLaunchCountdown > 0)
         {
            time = timeGetTime();
            if ((time-mLaunchLastUpdate) >= cLaunchUpdateFrequency)
            {
               long value = max(0, (long)(mLaunchCountdown - (time - mLaunchRequestTime)));
               BMessagePacket packet(value, BChannelPacketType::cLaunchUpdatePacket);
               mChannelArray[cChannelMessage]->SendPacket(packet);
               mLaunchLastUpdate = time;
            }
            break;
         }

         // if all players aren't locked, keep waiting
         if (mLockedPlayers < (DWORD)getPlayerCount())
            break;

         // if all players aren't ready to launch, keep waiting
         if (!allPlayersReadyToStart())
            break;
         
         // send out the start game packet
         BChannelPacket packet(BChannelPacketType::cStartGamePacket);
         mChannelArray[cChannelMessage]->SendPacket(packet);

         BMultiplayer::getInstance()->setGameState(cGameStatePregame);
      }
      break;

      case cGameStatePregame:
      break;

      case cGameStateInProgress:
      break;

      case cGameStatePostgame:
      break;

      case cGameStateFinal:
      break;
   }
}

//==============================================================================
// BMPGame::processSessionEvent
//==============================================================================
void BMPGame::processSessionEvent(const BSessionEvent *pEvent)
{
   switch (pEvent->mEventID)
   {
      case BSession::cEventConnected:
      {         
         //sessionConnected();
         if (!isHosting())
         {
            nlog(cMPGameCL, "BMPGame::processSessionEvent -- cEventConnected requesting settings from host.");

            BChannelPacket packet(BChannelPacketType::cRequestSettingsPacket);
            mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostClient(), packet);
         }
         else
         {
            nlog(cMPGameCL, "BMPGame::processSessionEvent -- cEventConnected host settings complete.");

            mSettingsComplete = true;

            mMPGameObserverList.initialSettingsComplete();
         }
      }
      break;

      case BSession::cEventDisconnected:
         sessionDisconnected(pEvent->mData1);
      break;

      case BSession::cEventClientConnect:
         clientConnected(pEvent->mData1);
      break;

      case BSession::cEventClientDisconnect:
         clientDisconnected(pEvent);
      break;

      case BSession::cEventClientData:
         handleClientData(pEvent);
      break;

      case BSession::cEventClientNotResponding:
         clientNotResponding(pEvent->mData1, pEvent->mData2);
      break;

      case BSession::cEventClientResponding:
         clientResponding(pEvent->mData1, pEvent->mData2);
      break;

      case BSession::cEventClientPing:
         clientPingUpdated(pEvent->mData1, pEvent->mData2);
      break;

      case BSession::cEventChannelData:
         handleChannelData(pEvent);
      break;
      
      default:
      break;
   }
}

//==============================================================================
// BMPGame::connectionAttempt
//==============================================================================
bool BMPGame::connectionAttempt(const SOCKADDR_IN &remote, const SOCKADDR_IN &xremote)
{
   long index = findConnectingAddresses(remote, xremote);
   nlog(cMPGameCL, "BMPGame::connectionAttempt -- findConnectAddress ret [%d]", index);

   // if this client was told he could connect, then accept the connection
   if (index>=0)
   {
      nlog(cMPGameCL, "BMPGame::connectionAttempt -- OK from ip[%s:%d] xip[%s:%d].", 
         inet_ntoa(remote.sin_addr),
         htons(remote.sin_port),
         inet_ntoa(xremote.sin_addr),
         htons(xremote.sin_port));

      mConnectingAddresses.removeIndex(index);
      return(true);
   }   
   // otherwise, piss off!
   return(false);
}

//==============================================================================
// BMPGame::joinRequest
//==============================================================================
void BMPGame::joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult& result)
{
   connector;

   nlog(cMPGameCL, "BMPGame::JoinRequest -- from [%s] ip[%s:%d] xip[%s:%d].", 
      name.getPtr(), 
      inet_ntoa(remoteAddress.sin_addr),
      htons(remoteAddress.sin_port),
      inet_ntoa(translatedRemoteAddress.sin_addr),
      htons(translatedRemoteAddress.sin_port));

   if ((crc != mChecksum) 
#ifndef BUILD_FINAL
	   && (!gConfig.isDefined(cConfigIgnoreMPChecksum))
#endif
	   )
   {
      nlog(cMPGameCL, "BMPGame::JoinRequest -- CRC Mismatch");
      result = cJoinCRCMismatch;
      return;
   }

   // if we told you to join, then DO IT ALREADY! 
   if (findConnectingAddresses(remoteAddress, translatedRemoteAddress) >= 0)
   {
      nlog(cMPGameCL, "BMPGame::JoinRequest -- Join Pending");
      result = cJoinPending;
      return;
   }

   if (!getSession())
   {
      result = cJoinRejected;
      return;
   }
   
   //long open = mMaxPlayers - getSession()->getConnectedClientAmount() - mConnectingAddresses.getNumber();
   //long open = mMaxPlayers - getPlayerCount() - mConnectingAddresses.getNumber();

   // we want (open slots - people we told to connect).
   long open = getSession()->getOpenSlotAmount();// - mConnectingAddresses.getNumber();   // third time the charm?

   if (open <= 0)
   {
      nlog(cMPGameCL, "BMPGame::JoinRequest -- Game Full");
      nlog(cMPGameCL, "BMPGame::JoinRequest -- Session Open: %d", getSession()->getOpenSlotAmount());
      result = cJoinFull;
   }
   else if (mPlayerMapper.getPlayerID(name) >= 0)
   {
      nlog(cMPGameCL, "BMPGame::JoinRequest -- User Exists");
      result = cJoinUserExists;
   }
   else
   {
      result = cJoinOK;

      if (mJoinRequestHandler)
         mJoinRequestHandler->joinRequest(name, crc, remoteAddress, translatedRemoteAddress, result);

      if (result == cJoinOK)
      {
         nlog(cMPGameCL, "BMPGame::JoinRequest -- Join OK.");      
         mConnectingAddresses.add(BMPConnectingClient(remoteAddress, translatedRemoteAddress)); // store the connection request for later

         HRESULT hr = getSession()->connectPeer(name, remoteAddress, translatedRemoteAddress);
         if (hr != S_OK)
         {
            nlog(cMPGameCL, "BMPGame::JoinRequest -- Failed getSession()->connectPeer. hr[0x%x]", hr);
            // log something here
            result = cJoinRejected;
         }
      }
   }
}

/*//==============================================================================
// BMPGame::subscribe
//==============================================================================
void BMPGame::subscribe(MPChatChannelObserver *o)
{
   mChatObservers.Add(o);
}

//==============================================================================
// BMPGame::unsubscribe
//==============================================================================
void BMPGame::unsubscribe(MPChatChannelObserver *o)
{
   mChatObservers.Remove(o);
}

//==============================================================================
// BMPGame::getChannelName
//==============================================================================
const BSimString& BMPGame::getChannelName() const
{   
   return(mChatChannelName);
}

//==============================================================================
// BMPGame::setChannelName
//==============================================================================
void BMPGame::setChannelName(const BSimString& name)
{
   mChatChannelName = name;
}
*/
//==============================================================================
// BMPGame::sendChat
//==============================================================================
void BMPGame::sendChat(const BSimString& message)
{
   if (!mChannelArray[cChannelChat])
      return;

   BChannelTextPacket packet(message);
   mChannelArray[cChannelChat]->SendPacket(packet);
}

//==============================================================================
// BMPGame::sendChat
//==============================================================================
void BMPGame::sendChat(const BSimString& message, const BDynamicSimArray<BSimString>& userList)
{
   if (!mChannelArray[cChannelChat] || !getSession())
      return;

   BClient *client;
   BChannelTextPacket packet(message);

   long count = userList.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      ClientID clientID = mPlayerMapper.getClientID(userList[idx]);
      if (clientID == cMPInvalidClientID)
         continue;

      // get his client, and send the packet
      client = getSession()->getClient(clientID);
      if (!client)
         continue;

      mChannelArray[cChannelChat]->SendPacketTo(client, packet);
   }
}

void BMPGame::sendVoice( const char* voiceData, const long dataLength, const DWORD senderXUID)
{
    if (!mChannelArray[cChannelChat])
        return;

    BChannelVoicePacket packet(voiceData,dataLength,senderXUID);
    mChannelArray[cChannelChat]->SendPacket(packet);    
}
/*
//==============================================================================
// BMPGame::whisper
//==============================================================================
void BMPGame::whisper(const BSimString &user, const BSimString &message)
{
   ClientID clientID = mPlayerMapper.getClientID(user);
   if (clientID == cMPInvalidClientID)
      return;

   // get his client, and send the packet
   BClient *client = getSession()->getClient(clientID);
   if (!client)
      return;

   BChannelTextPacket packet(message, user);

   mChannelArray[cChannelChat]->SendPacketTo(client, packet);
}

//==============================================================================
// BMPGame::parseAndSendChat
//==============================================================================
void BMPGame::parseAndSendChat(const BSimString& message)
{
   if (!mChannelArray[cChannelChat] || !getSession())
      return;

   const BCHAR_T *text = message.getPtr();

   long textLen = bcslen(text);
   if (textLen <= 0)
      return;

   // send a whisper
   if (textLen > 3 && (text[0] == L'/') && !bcsncmp( B("w"), &text[1], bcslen(B("w")) ) )
   {
      // split the string into the playerName and message.
      //
      BSimString str( text );

      long spaceNdx = str.findLeft( ' ', 3 );
      if ( spaceNdx == -1 )
         return;

      BSimString playerName;
      playerName.copy(str, spaceNdx-3, 3);
      str.crop(spaceNdx+1, str.length());

      ClientID clientID = mPlayerMapper.getClientID(playerName);
      if (clientID == cMPInvalidClientID)
         return;

      // get his client, and send the packet
      BClient *client = getSession()->getClient(clientID);
      if (!client)
         return;

      BChannelTextPacket packet(str);

      mChannelArray[cChannelChat]->SendPacketTo(client, packet);
   }
   else
   {
      sendChat(message);
   }
}
*/
//==============================================================================
// BMPGame::sendSyncData
//==============================================================================
bool BMPGame::sendSyncData(long uid, void *checksum, long checksumSize)
{
   if (!mChannelArray[cChannelSync])
      return(false);

   BSyncPacket packet(uid, checksum, checksumSize);
   return ( mChannelArray[cChannelSync]->SendPacket(packet) == S_OK);
}

//==============================================================================
// BMPGame::getSyncedCount
//==============================================================================
long BMPGame::getSyncedCount(void) const
{
   if (!getSession())
      return(1);
   
   return(getSession()->getActiveClientAmount());
}

//==============================================================================
// BMPGame::outOfSync
//==============================================================================
void BMPGame::outOfSync(void) const
{
   if (getSession())
      getSession()->disconnect(BSession::cSessionTerminated);
}

//==============================================================================
// BMPGame::channelDataReceived
//==============================================================================
void BMPGame::channelDataReceived(const long fromClientIndex, const void *data, const DWORD size)
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
      BFAIL("BMPGame::channelDataReceived -- unknown channel.");
      return;
   }

   switch (idx)
   {
      case cChannelChat:
      {
          BChannelVoicePacket packet;
          packet.deserializeFrom(data,size);
          gLiveSystem->processIncommingVoiceData(packet.mVoiceData, packet.mLength, packet.mSenderXUID);
          //This is the old text chat code - no longer used, channel hijacked by voice - eric
          /*
         PlayerID playerID = getPlayerID((ClientID)fromClientIndex);
         if (playerID >= 0)
         {
            const BSimString *name = getPlayerName(fromClientIndex);
            if (name)
            {
               BChannelTextPacket packet;
               packet.deserializeFrom(data, size);
               mMPGameObserverList.incomingChat(*name, packet.mText);
            }
         }
         */
      }
      break;

      case cChannelMessage:
         handleMessage(fromClientIndex, data, size);
      break;
      
      case cChannelCommand:
         if (mSimObject)
            mSimObject->commandDataReceived((ClientID)fromClientIndex, data, size);
      break;

      case cChannelSim:
         if (mSimObject)
            mSimObject->simDataReceived((ClientID)fromClientIndex, data, size);
      break;

      case cChannelGC:
         if (mSimObject)
            mSimObject->gcDataReceived((ClientID)fromClientIndex, data, size);
      break;

      case cChannelSync:
      {
         BSyncPacket packet;
         packet.deserializeFrom(data, size);
         notifySyncData(fromClientIndex, packet.mID, packet.mChecksum, packet.mChecksumSize);      
      }
      break;

      case cChannelVote:
      {
         handleVote(fromClientIndex, data, size);
      }
      break;

      case cChannelSettings:
      {
         handleSettings(fromClientIndex, data, size);
      }
      break;
   }
}


//==============================================================================
// BMPGame::cleanup
//==============================================================================
void BMPGame::cleanup()
{
   if (getSession())
   {        
      destroyChat();
      destroySimObject();
      destroySyncChannel();
      destroyVoteChannel();
      destroyMessageChannel();
      destroySettingsChannel();
      destroyGCChannel();

      if (getSession())
         getSession()->dispose();      
      
      mSession = NULL;

      long count = mPlayers.getNumber();
      for (long idx=0; idx<count; idx++)
      {
         delete mPlayers[idx];
         mPlayers[idx] = NULL;
      }
      mPlayers.setNumber(0);

      if (mSessionConnector)
         delete mSessionConnector;
      mSessionConnector = NULL;

      if (mFileTransferMgr)
         delete mFileTransferMgr;
      mFileTransferMgr = NULL;

      mLockedPlayers = 0;
   }
}

//==============================================================================
// BMPGame::
//==============================================================================
void BMPGame::handleClientData(const BSessionEvent *pEvent)
{
   const void *data = (((char*)pEvent)+sizeof(BSessionEvent));

   switch (BTypedPacket::getType(data))
   {
      case BPacketType::cGameFinalizePacket:
      {
         BMPPlayer *pPlayer = getPlayerFromID(pEvent->mData1);
         if (!pPlayer)
            break;

         pPlayer->setLoaded(true);

         if (allPlayersLoaded())
         {
            mMPGameObserverList.allPlayersLoaded();
            finalizeGame();
         }
      }
      break;
   }
}

//==============================================================================
// BMPGame::sessionConnected
//==============================================================================
void BMPGame::sessionConnected(void)
{
   nlog(cMPGameCL, "BMPGame::sessionConnected -- Enter");

   if (FAILED(createSimObject()))
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createSimObject");
      return;
   }

   if (!createChat()) 
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createChat");
      return;
   }

   if (!createSyncChannel()) 
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createSyncChannel");
      return;
   }

   if (!createVoteChannel())
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createVoteChannel");
      return;
   }

   if (!createMessageChannel())
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createMessageChannel");
      return;
   }

   if (!createSettingsChannel())
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createSettingsChannel");
      return;
   }

   if (!createGCChannel())
   {
      nlog(cMPGameCL, "BMPGame::sessionConnected -- failed createGCChannel");
      return;
   }

   nlog(cMPGameCL, "BMPGame::sessionConnected -- OK");
   mSessionConnected = true;

   /*
   if (!isHosting())
   {
      BChannelPacket packet(BChannelPacketType::cRequestSettingsPacket);
      mChannelArray[cChannelMessage]->SendPacketTo(getSession()->getHostClient(), packet);
   }
   */
}

//==============================================================================
// BMPGame::sessionDisconnected
//==============================================================================
void BMPGame::sessionDisconnected(long reason)
{
   nlog(cMPGameCL, "BMPGame::sessionDisconnected -- Enter");

   mGameConnected = false;
   mSessionConnected = false;
   mSettingsComplete = false;

   long mpreason = BMultiplayer::cDisconnectFailedConnection;

   switch (reason)
   {
      case BSession::cTransportLost:
      case BSession::cSessionTerminated:
      case BSession::cHostDecision:
      case BSession::cHostCancelledGame:
         mpreason = BMultiplayer::cDisconnectGameTerminated;
         break;

      case BSession::cConnectionRejected:
      case BSession::cFailedClientConnect:
         mpreason = BMultiplayer::cDisconnectFailedConnection;
         break;

      case BSession::cBuildMismatch:
         mpreason = BMultiplayer::cDisconnectCRCMismatch;
         break;

      case BSession::cSessionClosed:
      case BSession::cSessionFull:
         mpreason = BMultiplayer::cDisconnectFull;
         break;

      case BSession::cGameDeleted:
         mpreason = BMultiplayer::cDisconnectDeleted;
         break;

      default:
      case BSession::cNormal:
         mpreason = BMultiplayer::cDisconnectNormal;
         break;
   }

   mMPGameObserverList.gameDisconnected(this, mpreason);
}

//==============================================================================
// BMPGame::clientConnected
//==============================================================================
void BMPGame::clientConnected(DWORD clientIndex)
{
   BClient* client = getSession()->getClient(clientIndex);
   if (!client)
   {
      nlog(cMPGameCL, "BMPGame::clientConnected -- Failed to get client from session. ID[%d]", clientIndex);
      return;
   }

   nlog(cMPGameCL, "BMPGame::clientConnected -- Name[%s] ID[%d]", client->getName().getPtr(), clientIndex);
   
   // add a player
   BMPPlayer *player = new BMPPlayer(client);
   
   DWORD count = (DWORD)mPlayers.getNumber();
   if (clientIndex >= count)
   {
      mPlayers.setNumber(clientIndex+1);
      for (DWORD idx=count; idx<=clientIndex; idx++)
         mPlayers[idx] = NULL;
   }

   if (mPlayers[clientIndex] != NULL)
   {
      nlog(cMPGameCL, "BMPGame::clientConnected -- mPlayers[%d] already has a player Name [%s]", clientIndex, mPlayers[clientIndex]->getName().getPtr());
      BFAIL("BMPGame::clientConnected -- multiple client connects fromn the same client.");
      return;
   }

   mPlayers[clientIndex] = player;

/*
   if (getSession()->isHosted())
   {
      BCompleteSettingsPacket packet(BChannelPacketType::cInitialSettingsPacket);
      fillCompleteSettings(packet);
      mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getClient(clientIndex), packet);
   }
*/

   // if the game is already connected, connect this player
   if (mGameConnected)
   {
      nlog(cMPGameCL, "BMPGame::clientConnected -- mGameConnected:true calling connect player", clientIndex);
      connectPlayer(clientIndex);   
   }
   else
   {
      nlog(cMPGameCL, "BMPGame::clientConnected -- mGameConnected:false");
   }
}

//==============================================================================
// BMPGame::clientDisconnected
//==============================================================================
void BMPGame::clientDisconnected(const BSessionEvent *pEvent)
{
   DWORD clientIndex = pEvent->mData1;
   nlog(cMPGameCL, "BMPGame::clientDisconnected -- ID[%d]", clientIndex);

   // if he was in the process of connecting, remove him
   
   SOCKADDR_IN *pAddr = (SOCKADDR_IN*)(((char*)pEvent)+sizeof(BSessionEvent));
   long index = findConnectingAddresses(*pAddr, *pAddr);
   if (index >= 0)
   {
      nlog(cMPGameCL, "BMPGame::clientDisconnected -- found connecting address at index[%d]", index);
      mConnectingAddresses.removeIndex(index);
   }

   // 02/05/2005 MSC - 
   // can't use getPlayerFromID here because it calls into BSession to get the client. If we are processing
   // a disconnect event, it is likely that the session has already cleaned up the client so our function
   // will fail and return NULL and leave an invalid player sitting around. This will cause our 'multiple' connect
   // bug that we are seeing lately because we never cleaned up the last guy that disconnected from this slot.
   //
   // BMPPlayer *player = getPlayerFromID(clientIndex);
   
   // check for a valid clientIndex
   if (clientIndex>=(DWORD)mPlayers.getNumber())
   {
      nlog(cMPGameCL, "BMPGame::clientDisconnected -- invalid client index for mPlayers");
      return;
   }

   BMPPlayer *player = mPlayers[clientIndex];
   if (!player)
   {
      nlog(cMPGameCL, "BMPGame::clientDisconnected -- no player in mPlayers");
      return;
   }

   bool local = (DWORD)getSession()->getLocalClientID() == clientIndex;
   nlog(cMPGameCL, "BMPGame::clientDisconnected -- client local: %s", local?"true":"false");

   // remove a player
   PlayerID playerID = getPlayerID((ClientID)clientIndex);
   if (playerID >= 0)
   {
      nlog(cMPGameCL, "BMPGame::clientDisconnected -- player ID[%d]", playerID);
      mMPGameObserverList.playerLeft(playerID, local);
      const BSimString* name = getPlayerName((ClientID)clientIndex);
      if (name)
      {
         nlog(cMPGameCL, "BMPGame::clientDisconnected -- player Name[%s]", name->getPtr());
        /* MPChatUser user;
         user.mName = *name;
         mChatObservers.channelLeaveEvent(user);*/
      }

      notifyPlayerDrop(clientIndex);

      if (mSimObject)
         mSimObject->playerDisconnected(playerID);
   }

   // do this last in case someone needs player mapper
   mPlayerMapper.removePlayer(player->getID());
   mPlayers[clientIndex] = NULL;
   delete player;
   nlog(cMPGameCL, "BMPGame::clientDisconnected -- player deleted");
}

//==============================================================================
// BMPGame::clientNotResponding
//==============================================================================
void BMPGame::clientNotResponding(DWORD clientIndex, DWORD lastResponseTime)
{
   bool notifyGame = false;
   bool justDropHim = false;

   switch (mGameState)
   {
      // this is during the setup, or count down phase.
      case cGameStateSetup:
      case cGameStateLaunching:
      {
         // special case for host time outs
         if (clientIndex == 0)
         {
            // if the host hasn't responded in "a while", drop the game
            if ((timeGetTime() - lastResponseTime) > cMaxHostTimeout)
            {
               mMPGameObserverList.gameDisconnected(this, BMultiplayer::cDisconnectGameTerminated);
               return;
            }
         }
         // else, let the game decide
         notifyGame = true;
      }
      break;

      // this is the "after launch, before in game, loading screen" case
      case cGameStatePregame:
      {
         // if all players are loaded or it's been WAY too long
         if (allPlayersLoaded() || ((timeGetTime() - lastResponseTime) > cMaxLoadingTime))
            notifyGame = true;
      }
      break;

      // this is during a game
      case cGameStateInProgress:
      {
         // let the game decide
         notifyGame = true;
      }
      break;

      // this is when we really don't care
      case cGameStatePostgame:
      case cGameStateFinal:
         justDropHim = true;
      break;
   }

   if (notifyGame)
   {
      bool disconnect = false;
      long playerID = getPlayerID((ClientID)clientIndex);

      if (playerID < 0 || justDropHim)
      {         
         getSession()->disconnectClient(clientIndex);
         return;
      }

      nlog(cMPGameCL, "BMPGame::clientNotResponding -- client[%d] player[%d]. ", clientIndex, playerID);
      mMPGameObserverList.playerNotResponding(playerID, lastResponseTime, disconnect);

      if (disconnect)
      {
         nlog(cMPGameCL, "BMPGame::clientNotResponding -- disconnect: true. ");
         getSession()->disconnectClient(clientIndex);
      }
   }
}

//==============================================================================
// BMPGame::clientResponding
//==============================================================================
void BMPGame::clientResponding(DWORD clientIndex, DWORD lastResponseTime)
{
   bool disconnect = false;
   mMPGameObserverList.playerResponding(getPlayerID((ClientID)clientIndex), lastResponseTime, disconnect);

   if (disconnect)
   {
      nlog(cMPGameCL, "BMPGame::clientResponding -- disconnect: true. ");
      getSession()->disconnectClient(clientIndex);
   }
}

//==============================================================================
// BMPGame::clientPingUpdated
//==============================================================================
void BMPGame::clientPingUpdated(DWORD clientIndex, DWORD ping)
{
   mMPGameObserverList.playerPingUpdate(getPlayerID((ClientID)clientIndex), ping);
}

//==============================================================================
// BMPGame::handleChannelData
//==============================================================================
void BMPGame::handleChannelData(const BSessionEvent *pEvent)
{
   const void *data = (((char*)pEvent)+sizeof(BSessionEvent));
   long size  = pEvent->mData2;
   long fromClientIndex = pEvent->mData1;

   long channelID = BChannelPacket::getChannel(data);

   BSimString msg;
   for (long idx=0; idx<cChannelMax; idx++)
   {
      if (mChannelArray[idx] == NULL)
         continue;

      if (mChannelArray[idx]->getChannelID() == channelID)
      {
         mChannelArray[idx]->channelDataReceived(fromClientIndex, data, size);
         return;
      }
   }
}

//==============================================================================
// 
HRESULT BMPGame::createSimObject(void)
{
   if (mSimObject)
      return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);

   mSimObject = new BMPSimObject(this);

   if (!createSimChannels())
      return E_FAIL;

   return S_OK;
}

//==============================================================================
// 
void BMPGame::destroySimObject(void)
{
   destroySimChannels();

   if (mSimObject)
      delete mSimObject;
   mSimObject = NULL;
}

//==============================================================================
// BMPGame::createSimChannels
//==============================================================================
bool BMPGame::createSimChannels(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelCommand] = new BPeerOrderedChannel(BChannelType::cCommandChannel, getSession());
   if (!mChannelArray[cChannelCommand]) 
      return(false);

   mChannelArray[cChannelCommand]->addObserver(this);

   mChannelArray[cChannelSim] = new BPeerOrderedChannel(BChannelType::cSimChannel, getSession());
   if (!mChannelArray[cChannelSim]) 
      return(false);

   mChannelArray[cChannelSim]->addObserver(this);

   return(true);
}

//==============================================================================
// BMPGame::destroySimChannels
//==============================================================================
void BMPGame::destroySimChannels(void)
{   
   if (mChannelArray[cChannelCommand])
   {
      mChannelArray[cChannelCommand]->removeObserver(this);

      mChannelArray[cChannelCommand]->dispose();
      mChannelArray[cChannelCommand] = NULL;
   }   

   if (mChannelArray[cChannelSim])
   {
      mChannelArray[cChannelSim]->removeObserver(this);

      mChannelArray[cChannelSim]->dispose();
      mChannelArray[cChannelSim] = NULL;
   }   
}

//==============================================================================
// BMPGame::createSyncChannel
//==============================================================================
bool BMPGame::createSyncChannel(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelSync] = new BPeerOrderedChannel(BChannelType::cSyncChannel, getSession());
   if (!mChannelArray[cChannelSync]) 
      return(false);

   mChannelArray[cChannelSync]->addObserver(this);

   return(true);
}

//==============================================================================
// BMPGame::destroySyncChannel
//==============================================================================
void BMPGame::destroySyncChannel(void)
{   
   if (mChannelArray[cChannelSync])
   {
      mChannelArray[cChannelSync]->removeObserver(this);

      mChannelArray[cChannelSync]->dispose();
      mChannelArray[cChannelSync] = NULL;
   }   
}

//==============================================================================
// BMPGame::createVoteChannel
//==============================================================================
bool BMPGame::createVoteChannel(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelVote] = new BChannel(BChannelType::cVoteChannel, getSession());   
   if (!mChannelArray[cChannelVote])
      return(false);

   mChannelArray[cChannelVote]->addObserver(this);
   
   return(true);
}

//==============================================================================
// BMPGame::destroyVoteChannel
//==============================================================================
void BMPGame::destroyVoteChannel(void)
{
   if (mChannelArray[cChannelVote])
   {
      mChannelArray[cChannelVote]->removeObserver(this);

      mChannelArray[cChannelVote]->dispose();
      mChannelArray[cChannelVote] = NULL;
   }
}

//==============================================================================
// BMPGame::createMessageChannel
//==============================================================================
bool BMPGame::createMessageChannel(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelMessage] = new BPeerOrderedChannel(BChannelType::cMessageChannel, getSession());
   if (!mChannelArray[cChannelMessage])
      return(false);

   mChannelArray[cChannelMessage]->addObserver(this);

   return(true);
}

//==============================================================================
// BMPGame::destroyMessageChannel
//==============================================================================
void BMPGame::destroyMessageChannel(void)
{
   if (mChannelArray[cChannelMessage])
   {
      mChannelArray[cChannelMessage]->removeObserver(this);

      mChannelArray[cChannelMessage]->dispose();
      mChannelArray[cChannelMessage] = NULL;
   }
}

//==============================================================================
// BMPGame::createSettingsChannel
//==============================================================================
bool BMPGame::createSettingsChannel(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelSettings] = new BPeerOrderedChannel(BChannelType::cSettingsChannel, getSession());
   if (!mChannelArray[cChannelSettings])
      return(false);

   mChannelArray[cChannelSettings]->addObserver(this);

   return(true);
}

//==============================================================================
// BMPGame::destroySettingsChannel
//==============================================================================
void BMPGame::destroySettingsChannel(void)
{
   if (mChannelArray[cChannelSettings])
   {
      mChannelArray[cChannelSettings]->removeObserver(this);

      mChannelArray[cChannelSettings]->dispose();
      mChannelArray[cChannelSettings] = NULL;
   }
}

//==============================================================================
// BMPGame::createGCChannel
//==============================================================================
bool BMPGame::createGCChannel(void)
{
   if (!getSession())
      return(false);

   mChannelArray[cChannelGC] = new BPeerOrderedChannel(BChannelType::cGCChannel, getSession());
   if (!mChannelArray[cChannelGC])
      return(false);

   mChannelArray[cChannelGC]->addObserver(this);

   return(true);
}

//==============================================================================
// BMPGame::destroyGCChannel
//==============================================================================
void BMPGame::destroyGCChannel(void)
{
   if (mChannelArray[cChannelGC])
   {
      mChannelArray[cChannelGC]->removeObserver(this);

      mChannelArray[cChannelGC]->dispose();
      mChannelArray[cChannelGC] = NULL;
   }
}

//==============================================================================
// BMPGame::createChat
//==============================================================================
bool BMPGame::createChat(void)
{
   mChannelArray[cChannelChat] = new BChannel(BChannelType::cMPChatChannel, getSession());   
   if (!mChannelArray[cChannelChat]) 
      return(false);

   mChannelArray[cChannelChat]->addObserver(this);
   return(true);
}

//==============================================================================
// BMPGame::destroyChat
//==============================================================================
void BMPGame::destroyChat(void)
{
   if (mChannelArray[cChannelChat])
   {
      mChannelArray[cChannelChat]->removeObserver(this);

      mChannelArray[cChannelChat]->dispose();
      mChannelArray[cChannelChat] = NULL;
   }
}

//==============================================================================
// BMPGame::fillCompleteSettings
//==============================================================================
void BMPGame::fillCompleteSettings(BCompleteSettingsPacket &packet)
{
   // send over all settings
   const DWORD cBufferSize = 8192;
   static BYTE buffer[cBufferSize];
   static DWORD offset;
   WORD  size;

   offset = 0;

   DWORD count = mGameSettings->getNumberEntries();
   memcpy(&buffer[offset], &count, sizeof(count));
   offset += sizeof(count);

   for (DWORD idx=0; idx<count; idx++)
   {
      size = (WORD)mGameSettings->getDataSize(idx);
      memcpy(&buffer[offset], &size, sizeof(size));
      offset += sizeof(size);

      if (size > 0)
      {
         if (!mGameSettings->getData(idx, &buffer[offset], (WORD)(cBufferSize-offset)))
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
// BMPGame::handleSettingsPacket
//==============================================================================
void BMPGame::handleSettingsPacket(BSettingsPacket &packet)
{
   // set locally, send to everyone else
   if (!mbSettingsLocked
         #ifndef BUILD_FINAL
         || packet.mOverrideLock
         #endif
      )
   {
      nlog(cMPGameSettingsCL, "BMPGame::handleSettingsPacket -- settings not locked. ");
      if (!mGameSettings->setData(packet.mIndex, packet.mData, (WORD)packet.mSize))
      {
         nlog(cMPGameSettingsCL, "BMPGame::handleSettingsPacket -- failed to call mGameSettings->setData. ");
         return;
      }

      nlog(cMPGameSettingsCL, "BMPGame::handleSettingsPacket -- sending to all clients. setting index[%d], data[%d].", packet.mIndex, packet.mData);
      long count = getSession()->getClientCount();
      for (long idx=1; idx<count; idx++)
         mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getClient(idx), packet);
   }            
}

//==============================================================================
// BMPGame::handleCompleteSettings
//==============================================================================
void BMPGame::handleCompleteSettings(long type, const void* data, long size)
{
   if (isHosting() || size == 0)
      return;
   
   nlog(cMPGameSettingsCL, "BMPGame::handleCompleteSettings -- enter.");

   BCompleteSettingsPacket packet(type);
   packet.deserializeFrom(data, size);
   if (packet.mSize == 0)
      return;

   DWORD count = 0;
   WORD wsize = 0;
   DWORD offset = 0;
   BYTE  *pData = (BYTE*)packet.mData;

   memcpy(&count, &pData[offset], sizeof(count));
   offset += sizeof(count);

   for (DWORD idx=0; idx<count; idx++)
   {
      memcpy(&wsize, &pData[offset], sizeof(wsize));
      offset += sizeof(wsize);

      if (wsize > 0)
      {
         mGameSettings->setData(idx, &pData[offset], wsize);
         offset += wsize;
      }
   }
}

//==============================================================================
// BMPGame::handleMessage
//==============================================================================
void BMPGame::handleMessage(long clientIndex, const void* data, long size)
{
   switch (BChannelPacket::getType(data))
   {
      case BChannelPacketType::cRequestSettingsPacket:
      {
         if (isHosting())
         {
            nlog(cMPGameSettingsCL, "BMPGame::handleMessage -- cRequestSettingsPacket process settings request.");

            BCompleteSettingsPacket spacket(BChannelPacketType::cInitialSettingsPacket);
            fillCompleteSettings(spacket);
            mChannelArray[cChannelSettings]->SendPacketTo(getSession()->getClient(clientIndex), spacket);
         }
      }
      break;

      case BChannelPacketType::cLockSettingsPacket:
      {         
         nlog(cMPGameSettingsCL, "BMPGame::handleMessage -- lock settings.");
         BMessagePacket packet(BChannelPacketType::cLockSettingsPacket);
         packet.deserializeFrom(data, size);

         mbSettingsLocked = packet.mMessage == 1;

         if (mbSettingsLocked)
         {
            BMultiplayer::getInstance()->setGameState(cGameStateLaunching);

            getSession()->setOpenSync(false);

            mMPGameObserverList.launchStarted();

            BChannelPacket response(BChannelPacketType::cSettingsLockedPacket);
            mChannelArray[cChannelMessage]->SendPacket(response);
         }
      }
      break;

      case BChannelPacketType::cSettingsLockedPacket:
      {
         nlog(cMPGameSettingsCL, "BMPGame::handleMessage -- locked settings count.");
         if (isHosting())
         {
            mLockedPlayers++;
         }
      }
      break;

      case BChannelPacketType::cLaunchUpdatePacket:
      {
         nlog(cMPGameCL, "BMPGame::handleMessage -- launch update.");

         BMessagePacket packet(BChannelPacketType::cLaunchUpdatePacket);
         packet.deserializeFrom(data, size);
         mMPGameObserverList.launchTimeUpdate((DWORD)packet.mMessage);

         // we are done counting down
         if (packet.mMessage == 0 && isHosting())
            mLaunchCountdown = 0;
      }
      break;

      case BChannelPacketType::cLaunchReadyPacket:
      {
         BMessagePacket packet(BChannelPacketType::cLaunchReadyPacket);
         packet.deserializeFrom(data, size);

         BMPPlayer *pPlayer = getPlayerFromID(clientIndex);
         if (!pPlayer)
            break;

         pPlayer->setReadyToStart(packet.mMessage?true:false);
      }
      break;

      case BChannelPacketType::cLaunchAbortRequestPacket:
      {
         nlog(cMPGameCL, "BMPGame::handleMessage -- launch abort request.");
         if (isHosting())
         {
            mbSettingsLocked = false;
            
            getSession()->setOpenSync(true);

            BMessagePacket spacket(0, BChannelPacketType::cLockSettingsPacket);
            mChannelArray[cChannelMessage]->SendPacket(spacket);

            BMessagePacket rpacket(BChannelPacketType::cLaunchAbortRequestPacket);
            rpacket.deserializeFrom(data, size);

            BMessageDataPacket packet(rpacket.mMessage, clientIndex, BChannelPacketType::cLaunchAbortPacket);
            mChannelArray[cChannelMessage]->SendPacket(packet);

            BMultiplayer::getInstance()->setGameState(cGameStateSetup);
         }
      }
      break;

      case BChannelPacketType::cLaunchAbortPacket:
      {
         nlog(cMPGameCL, "BMPGame::handleMessage -- launch aborted.");

         mLockedPlayers = 0;
         mLaunchCountdown = 0;
         mLaunchRequestTime = 0;
         mLaunchLastUpdate = 0;

         long count = mPlayers.getNumber();
         for (long idx=0; idx<count; idx++)
            if (mPlayers[idx] != NULL)
               mPlayers[idx]->reset();

         BMessageDataPacket rpacket(BChannelPacketType::cLaunchAbortPacket);
         rpacket.deserializeFrom(data, size);

         mMPGameObserverList.launchAborted(mPlayerMapper.getPlayerID(rpacket.mData), rpacket.mMessage);
      }
      break;

      case BChannelPacketType::cStartGamePacket:
      {
         nlog(cMPGameCL, "BMPGame::handleMessage -- start game.");         
         
         if (!isHosting())
            BMultiplayer::getInstance()->setGameState(cGameStatePregame);

         mMPGameObserverList.startGame(); // tell the game to start already!
      }
      break;
   };
}

//==============================================================================
// BMPGame::handleSettings
//==============================================================================
void BMPGame::handleSettings(long clientIndex, const void* data, long size)
{
   clientIndex;
   
   long type = BChannelPacket::getType(data);
   switch (type)
   {
      case BChannelPacketType::cSettingsPacket:
      {
         BSettingsPacket packet;
         packet.deserializeFrom(data, size);

         // this is a client sending a settings change to the host
         if (isHosting())
            handleSettingsPacket(packet);
         // else I am a client receiving a setting from the host
         else
         {
            mGameSettings->setData(packet.mIndex, packet.mData, (WORD)packet.mSize);
            nlog(cMPGameSettingsCL, "BMPGame::handleSettings -- setting change index[%d] data[%d].", packet.mIndex, packet.mData);
         }
      }
      break;

      case BChannelPacketType::cInitialSettingsPacket:
      {
         nlog(cMPGameSettingsCL, "BMPGame::handleSettings -- Initial Settings Packet.");
         handleCompleteSettings(type, data, size);

         mSettingsComplete = true;

         mMPGameObserverList.initialSettingsComplete();
      }
      break;

      case BChannelPacketType::cFinalSettingsPacket:
      {
         nlog(cMPGameSettingsCL, "BMPGame::handleSettings -- Final Settings Packet.");
         handleCompleteSettings(type, data, size);
      }
      break;
   }
}

//==============================================================================
// BMPGame::handleVote
//==============================================================================
void BMPGame::handleVote(long clientIndex, const void* data, long size)
{
   switch(BChannelPacket::getType(data))
   {
      case BChannelPacketType::cVoteResult:
      {
         BVotePacket packet(BChannelPacketType::cVoteResult);
         packet.deserializeFrom(data, size);

         nlog(cMPGameCL, "BMPGame::handleVote -- vote result type[%d] vote[%d] arg1[%d] from[%d]", packet.mVoteType, packet.mVote, packet.mArg1, clientIndex);
         notifyVoteResults(packet.mVoteType, getPlayerID((ClientID)clientIndex), packet.mVote, packet.mArg1);
      }
      break;

      case BChannelPacketType::cStartVote:
      {
         BVotePacket packet(BChannelPacketType::cStartVote);
         packet.deserializeFrom(data, size);

         nlog(cMPGameCL, "BMPGame::handleVote -- vote start type[%d] arg1[%d] from[%d]", packet.mVoteType, packet.mArg1, getPlayerID((ClientID)clientIndex));
         notifyVoteStart(packet.mVoteType, packet.mArg1);
      }
      break;

      case BChannelPacketType::cAbortVote:
      {
         BVotePacket packet(BChannelPacketType::cAbortVote);
         packet.deserializeFrom(data, size);

         nlog(cMPGameCL, "BMPGame::handleVote -- vote abort type arg1[%d] from[%d]", packet.mVoteType, packet.mArg1, getPlayerID((ClientID)clientIndex));
         notifyVoteAbort(packet.mVoteType, packet.mArg1);
      }
      break;
   }
}

//==============================================================================
// BMPGame::connectPlayer
//==============================================================================
void BMPGame::connectPlayer(DWORD clientIndex)
{
   BMPPlayerEntry* entry = NULL;
   DWORD localPlayer = (DWORD)getSession()->getLocalClientID();
   BClient *client = getSession()->getClient(clientIndex);
   BFATAL_ASSERT(client);
   if (!client)
      return;

   mMPGameObserverList.playerJoined((ClientID)clientIndex, client->getName(), localPlayer == clientIndex, &entry);
   /*MPChatUser user;
   user.mName = client->getName();   
   mChatObservers.channelJoinEvent(user);*/

   if (entry!= NULL)
      mPlayerMapper.addPlayer(entry);
}

//==============================================================================
// BMPGame::getPlayerFromID
//==============================================================================
BMPPlayer* BMPGame::getPlayerFromID(long id)
{
   if (id<0 || id>=mPlayers.getNumber())
      return(NULL);

   if (!getSession())
      return(NULL);
   
   BClient *client = getSession()->getClient(id);
   if (!client)
      return(NULL);

   return(mPlayers[id]);
}

//==============================================================================
// 
HRESULT BMPGame::sendCommandPacket(BChannelPacket &packet)
{
   if (!mChannelArray[cChannelCommand]) 
      return E_FAIL;

   mChannelArray[cChannelCommand]->SendPacket(packet);
      
   return S_OK;
}

//==============================================================================
// 
HRESULT BMPGame::sendSimPacket(BChannelPacket &packet)
{
   if (!mChannelArray[cChannelSim]) 
      return E_FAIL;

   mChannelArray[cChannelSim]->SendPacket(packet);
      
   return S_OK;
}

//==============================================================================
// 
HRESULT BMPGame::sendGCPacket(BChannelPacket &packet)
{
   if (!mChannelArray[cChannelGC])
      return E_FAIL;

   return mChannelArray[cChannelGC]->SendPacket(packet);
}

//==============================================================================
// 
HRESULT BMPGame::getLocalTiming(unsigned char *timing, DWORD *deviationRemaining)
{
   if (mSimObject && mSimObject->getTimingHandler())
      return mSimObject->getTimingHandler()->getLocalTiming(timing, deviationRemaining);
   else
      return HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);
}

//==============================================================================
// 
float BMPGame::getMSPerFrame(void)
{
   if (mSimObject && mSimObject->getTimingHandler())
      return mSimObject->getTimingHandler()->getMSPerFrame();
   else
      return 0;
}

//==============================================================================
void BMPGame::setGameState(long state)
{
   if (mGameState == state)
      return;

   nlog(cMPGameCL, "BMPGame::setGameState -- state [%d]", state);
   switch (state)
   {
      case cGameStateSetup:
      {
      }
      break;

      case cGameStateLaunching:
      {
      }
      break;

      case cGameStatePregame:
      {
         // this just feels wrong, but whatever. Unlock the settings during pregame
         BMessagePacket packet(0, BChannelPacketType::cLockSettingsPacket);
         mChannelArray[cChannelMessage]->SendPacket(packet);   
      }
      break;

      case cGameStateInProgress:
      {
      }
      break;

      case cGameStatePostgame:
      case cGameStateFinal:
      {
      }
      break;

      default:
         BFAIL("BMPGame::setGameState -- unsupported game state.");
         return;
   }

   mGameState = state;
}


//==============================================================================
// BMPGame::findConnectingAddresses
//==============================================================================
long BMPGame::findConnectingAddresses(const SOCKADDR_IN& addr, const SOCKADDR_IN& xaddr)
{
   long count = mConnectingAddresses.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if ((memcmp(&mConnectingAddresses[idx].remote, &addr, sizeof(addr)) == 0) &&
          (memcmp(&mConnectingAddresses[idx].xremote, &xaddr, sizeof(xaddr)) == 0) )
      {
         return(idx);
      }
   }
   return(-1);
}

//==============================================================================
// BMPGame::
//==============================================================================
