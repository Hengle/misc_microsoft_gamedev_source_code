//==============================================================================
// mpgameview.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "mpgame.h"
#include "mpgameview.h"

#include "DataSet.h"

//==============================================================================
// Defines

//==============================================================================
// BMPGameView::BMPGameView
//==============================================================================
BMPGameView::BMPGameView(BMPGame *mpGame, BDataSet *settings)
{
   mMPGame = mpGame;
   mSettings = settings;

   mMPGame->addObserver(this);
   mSettings->addDataListener(this);
}

//==============================================================================
// BMPGameView::BMPGameView
//==============================================================================
BMPGameView::~BMPGameView()
{
   if (mSettings)
      mSettings->removeDataListener(this);
   if (mMPGame)
      mMPGame->removeObserver(this);   

   mMPGame = NULL;
   mSettings = NULL;
}

//==============================================================================
// BMPGameView::addObserver
//==============================================================================
bool BMPGameView::addObserver(BMPGameViewObserver* o)
{
   mObservers.Add(o);
   return(true);
}

//==============================================================================
// BMPGameView::removeObserver
//==============================================================================
bool BMPGameView::removeObserver(BMPGameViewObserver* o)
{
   mObservers.Remove(o);
   return(true);
}

//==============================================================================
// BMPGameView::setJoinRequestHandler
//==============================================================================
void BMPGameView::setJoinRequestHandler(BMPJoinRequestHandler* handler)
{
   if (mMPGame)
      mMPGame->setJoinRequestHandler(handler);
}

//==============================================================================
// BMPGameView::host
//==============================================================================
HRESULT BMPGameView::host(const BSimString &nickname, const BMPGameDescriptor &descriptor)
{
   if (mMPGame)
      return(mMPGame->host(nickname, descriptor));
   else 
      return E_FAIL;
}

//==============================================================================
// BMPGameView::join
//==============================================================================
HRESULT BMPGameView::join(const BSimString &nickname, const BMPGameDescriptor &descriptor)
{
   if (mMPGame)
      return(mMPGame->join(nickname, descriptor));
   else
      return E_FAIL;
}

//==============================================================================
// BMPGameView::setSlotOpen
//==============================================================================
bool BMPGameView::setSlotOpen(long slot, bool open)
{
   if (mMPGame)
      return(mMPGame->setSlotOpen(slot, open));
   else
      return false;
}

//==============================================================================
// BMPGameView::joinRequest
//==============================================================================
bool BMPGameView::joinRequest(const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, BSessionConnector::BSCObserver::eJoinResult &result)
{
   if (!mMPGame)
      return false;

   mMPGame->joinRequest(NULL, name, crc, remoteAddress, translatedRemoteAddress, result);
   return true;
}

//==============================================================================
// BMPGameView::setLocalUpdates
//==============================================================================
void BMPGameView::setLocalUpdates(bool val)
{
   if (mMPGame)
      mMPGame->setLocalUpdates(val);
}

//==============================================================================
// BMPGameView::setLong
//==============================================================================
bool BMPGameView::setLong(DWORD index, long data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setDWORD(DWORD index, DWORD data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setFloat(DWORD index, float data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setShort(DWORD index, short data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setWORD(DWORD index, WORD  data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setBool(DWORD index, bool  data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setChar(DWORD index, char  data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setBYTE(DWORD index, BYTE  data)
{
   if (mMPGame)
      return(mMPGame->setSetting(index, &data, sizeof(data)));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::setString(DWORD index, const BSimString& data)
{
   BCHAR_T* pData = const_cast<BCHAR_T*>(data.getPtr());
   long size  = (data.length()+1)*sizeof(BCHAR_T);

   if (mMPGame)
      return(mMPGame->setSetting(index, pData, size));
   else 
      return false;
}

#ifndef BUILD_FINAL
//==============================================================================
// BMPGameView::setBoolOverride
// This is really only used to turn on cheats, and should not be in a final release
//==============================================================================
bool BMPGameView::setBoolOverride(DWORD index, bool data)
{
   if (mMPGame)
      return(mMPGame->setSettingOverride(index, &data, sizeof(data)));
   return false;
}
#endif


//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getLong(DWORD index, long  &data) const
{
   if (mMPGame)
      return(mSettings->getLong(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getDWORD(DWORD index, DWORD &data) const
{
   if (mMPGame)
      return(mSettings->getDWORD(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getFloat(DWORD index, float &data) const
{
   if (mMPGame)
      return(mSettings->getFloat(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getShort(DWORD index, short &data) const
{
   if (mMPGame)
      return(mSettings->getShort(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getWORD(DWORD index, WORD  &data) const
{
   if (mMPGame)
      return(mSettings->getWORD(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getBool(DWORD index, bool  &data) const
{
   if (mMPGame)
      return(mSettings->getBool(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getChar(DWORD index, char  &data) const
{
   if (mMPGame)
      return(mSettings->getChar(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getBYTE(DWORD index, BYTE  &data) const
{
   if (mMPGame)
      return(mSettings->getBYTE(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::
//==============================================================================
bool BMPGameView::getString(DWORD index, BSimString &data) const
{
   if (mMPGame)
      return(mSettings->getString(index, data));
   else 
      return false;
}

//==============================================================================
// BMPGameView::attachFileXfer
//==============================================================================
bool BMPGameView::attachFileXfer(BFileTransferGameInterface *xfer)
{
   if (!mMPGame)
      return false;

   return mMPGame->attachFileXfer(xfer);
}

//==============================================================================
// BMPGameView::getFileXfer
//==============================================================================
BFileTransferGameInterface* BMPGameView::getFileXfer()
{
   if (!mMPGame)
      return NULL;

   return mMPGame->getFileXfer();
}

//==============================================================================
// BMPGameView::setMaxPlayers
//==============================================================================
void BMPGameView::setMaxPlayers(long maxCount)
{
   if (mMPGame)
      mMPGame->setMaxPlayers(maxCount);
}

//==============================================================================
// BMPGameView::getMaxPlayers
//==============================================================================
long BMPGameView::getMaxPlayers() const 
{
   if (mMPGame)
      return(mMPGame->getMaxPlayers()); 
   return 0;
}

//==============================================================================
// BMPGameView::getPlayerCount
//==============================================================================
long BMPGameView::getPlayerCount(void) const
{
   if (mMPGame)
      return(mMPGame->getPlayerCount());
   else 
      return 0;
}

//==============================================================================
// BMPGameView::getClientID
//==============================================================================
ClientID BMPGameView::getClientID(PlayerID playerID) const
{
   if (mMPGame)
      return(mMPGame->getClientID(playerID));
   else 
      return cMPInvalidClientID;
}

//==============================================================================
// BMPGameView::getPlayerID
//==============================================================================
PlayerID BMPGameView::getPlayerID(ClientID clientID) const
{
   if (mMPGame)
      return(mMPGame->getPlayerID(clientID));
   else 
      return cMPInvalidPlayerID;
}

//==============================================================================
// BMPGameView::getPlayerID
//==============================================================================
PlayerID BMPGameView::getPlayerID(const BSimString &playerName) const
{
   if (mMPGame)
      return (mMPGame->getPlayerID(playerName));
   else
      return cMPInvalidPlayerID;
}

//==============================================================================
// BMPGameView::getControlledPlayerID
//==============================================================================
PlayerID BMPGameView::getControlledPlayerID(void) const
{
   if (mMPGame)
      return mMPGame->getControlledPlayer();
   else
      return cMPInvalidPlayerID;
}

//==============================================================================
// BMPGameView::getPlayerName
//==============================================================================
const BSimString* BMPGameView::getPlayerName(PlayerID playerID) const
{
   if (mMPGame)
      return mMPGame->getPlayerName(getClientID(playerID));
   else
      return NULL;
}

//==============================================================================
// BMPGameView::kickPlayer
//==============================================================================
void BMPGameView::kickPlayer(PlayerID playerID)
{
   if (mMPGame)
      mMPGame->kickPlayer(playerID);
}

//==============================================================================
// BMPGameView::isHosting
//==============================================================================
bool BMPGameView::isHosting(void) const
{
   if (mMPGame)
      return(mMPGame->isHosting());
   else 
      return false;
}

//==============================================================================
// BMPGameView::requestGameLaunch
//==============================================================================
bool BMPGameView::requestGameLaunch(DWORD countdown)
{
   if (mMPGame)
      return(mMPGame->requestGameLaunch(countdown));
   else 
      return false;
}

//==============================================================================
// BMPGameView::requestLaunchAbort
//==============================================================================
bool BMPGameView::requestLaunchAbort(long reason)
{
   if (mMPGame)
      return(mMPGame->requestLaunchAbort(reason));
   else 
      return false;
}

//==============================================================================
// BMPGameView::sendLaunchReady
//==============================================================================
void BMPGameView::sendLaunchReady(bool ready)
{
   if (!mMPGame)
      return;

   mMPGame->sendLaunchReady(ready);
}

//==============================================================================
// BMPGameView::sendLaunchReady
//==============================================================================
bool BMPGameView::isLaunching() const
{
   if (!mMPGame)
      return false;

   return mMPGame->getGameState() == cGameStateLaunching;
}

//==============================================================================
// BMPGameView::sendVoice
//==============================================================================
void BMPGameView::sendVoice( const char* voiceData, const long dataLength, const DWORD senderXUID)
{
    if (!mMPGame)
        return;

    return mMPGame->sendVoice( voiceData, dataLength, senderXUID);
}

//==============================================================================
// BMPGameView::OnDataChanged
//==============================================================================
void BMPGameView::OnDataChanged(const BDataSet* set, DWORD index, BYTE flags)
{
   mObservers.onSettingsChanged(set, index, flags);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::gameDestroyed()
{
   mMPGame = NULL;
   mObservers.gameDisconnected(-1);
   if (mSettings)
      mSettings->removeDataListener(this);
   mSettings = NULL;
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::gameConnected( BMPGame * /*pGame*/ )
{
   mObservers.gameConnected();
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::gameConnectFailed(long /*reason*/)
{
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::gameConnectTimeUpdated( long /*connectTime*/ )
{
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::gameDisconnected( BMPGame * /*pGame*/, long reason )
{
   if (mMPGame)
      mMPGame->removeObserver(this);
   mObservers.gameDisconnected(reason);
   mMPGame = NULL;
   if (mSettings)
      mSettings->removeDataListener(this);
   mSettings = NULL;
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::playerJoined( ClientID player, const BSimString& name, bool local, BMPPlayerEntry **pEntry )
{
   mObservers.playerJoined(player, name, local, pEntry);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::playerLeft( PlayerID player, bool local )
{
   mObservers.playerLeft(player, local);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect)
{
   mObservers.playerNotResponding(gamePlayer, lastResponseTime, disconnect);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect)
{
   mObservers.playerResponding(gamePlayer, lastResponseTime, disconnect);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::playerPingUpdate(PlayerID gamePlayer, DWORD ping)
{
   mObservers.playerPingUpdate(gamePlayer, ping);
}

//==============================================================================
// BMPGameView::
//==============================================================================
void BMPGameView::allPlayersLoaded( void )
{
   mObservers.allPlayersLoaded();
}

//==============================================================================
// BMPGameView::launchStarted
//==============================================================================
void BMPGameView::launchStarted(void)
{
   mObservers.launchStarted();
}

//==============================================================================
// BMPGameView::launchTimeUpdate
//==============================================================================
void BMPGameView::launchTimeUpdate(DWORD time)
{
   mObservers.launchTimeUpdate(time);
}

//==============================================================================
// BMPGameView::launchAborted
//==============================================================================
void BMPGameView::launchAborted(PlayerID gamePlayer, long reason)
{
   mObservers.launchAborted(gamePlayer, reason);
}

//==============================================================================
// BMPGameView::startGame
//==============================================================================
void BMPGameView::startGame(void)
{
   mObservers.startGame();
}

//==============================================================================
// BMPGameView::initialSettingsComplete
//==============================================================================
void BMPGameView::initialSettingsComplete(void)
{
   mObservers.initialSettingsComplete();
}


//==============================================================================
// BMPGameView::sendChat
//==============================================================================
void BMPGameView::sendChat(const BSimString& message)
{
   if (!mMPGame)
      return;

   mMPGame->sendChat(message);
}


//==============================================================================
// BMPGameView::sendChat
//==============================================================================
void BMPGameView::sendChat(const BSimString& message, const BDynamicSimArray<BSimString>& userList)
{
   if (!mMPGame)
      return;

   mMPGame->sendChat(message, userList);
}


//==============================================================================
// BMPGameView::incomingChat
//==============================================================================
 void BMPGameView::incomingChat(const BSimString &user, const BSimString &message)
 {
    mObservers.incomingChat(user, message);
 }


//==============================================================================
// BMPGameView::
//==============================================================================
