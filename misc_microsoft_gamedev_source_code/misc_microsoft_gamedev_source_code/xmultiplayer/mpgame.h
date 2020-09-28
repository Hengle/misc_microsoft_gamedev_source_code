//==============================================================================
// mpgame.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once

#ifndef __MPGAME_H__
#define __MPGAME_H__

//==============================================================================
// Includes
#include "Channel.h"
#include "session.h"
#include "mpplayer.h"
#include "mpplayermap.h"
#include "mpsync.h"
#include "SessionConnector.h"
#include "mptypes.h"
#include "timesync.h"
#include "mpvote.h"
#include "Multiplayer.h"
#include "filetransfermgr.h"

//==============================================================================
// Forward declarations
class BDataSet;
class BMPGameView;
class BMPGameDescriptor;
class BChannel;
class BFileTransferMgr;
class BSyncChannel;
class BConnectivity;
class BMPSimObject;
class BMPPlayerMap;
class BFileTransferGameInterface;
class BCompleteSettingsPacket;
class BSettingsPacket;

//==============================================================================
// Const declarations
extern const BCHAR_T* cMPGameChannelName;

class BMPConnectingClient
{
public:
   BMPConnectingClient()
   {
      memset(&remote, 0, sizeof(remote));
      memset(&xremote, 0, sizeof(xremote));
      requestTime = 0;
   }

   BMPConnectingClient(const SOCKADDR_IN &address, const SOCKADDR_IN &xaddress)
   {
      memcpy(&remote, &address, sizeof(remote));
      memcpy(&xremote, &xaddress, sizeof(remote));
      requestTime = timeGetTime();
   }

   const BMPConnectingClient& operator=(const BMPConnectingClient& rhs)
   {
      memcpy(&remote, &rhs.remote, sizeof(remote));
      memcpy(&xremote, &rhs.xremote, sizeof(remote));
      requestTime = rhs.requestTime;
      return *this;
   }

   SOCKADDR_IN remote;
   SOCKADDR_IN xremote;
   DWORD requestTime;
};

typedef BDynamicSimArray<BMPConnectingClient> BAddressArray;

//==============================================================================
class BMPJoinRequestHandler
{
public:
   virtual void joinRequest(const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, BSessionConnector::BSCObserver::eJoinResult &result) = 0;
};

//==============================================================================
class BMPGame : public BSession::BSessionEventObserver,
                public BSession::BClientConnector,
                public BSessionConnector::BSCObserver,
                public BChannel::BChannelObserver,
                public BFileTransferPlayerInterface,
                //public MPChatChannel,
                public BMPSyncObject,
                public BTimingInfo,
                public BMPVoteHandler
{
public:
   BMPGame(BConnectivity* connectivity);
   ~BMPGame();

   class BMPGameObserver
   {
      public:
         virtual void gameDestroyed() = 0;
         virtual void gameConnected( BMPGame *pGame ) = 0;
         enum { cDisconnected=0, cMigrating, cFailedToHost };         
         virtual void gameConnectFailed(long reason) = 0;
         virtual void gameConnectTimeUpdated( long connectTime ) = 0;
         virtual void gameDisconnected( BMPGame *pGame, long reason ) = 0;
         virtual void playerJoined( ClientID mpPlayer, const BSimString& name, bool local, BMPPlayerEntry **pEntry ) = 0;
         virtual void playerLeft( PlayerID gamePlayer, bool local ) = 0;
         virtual void playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) = 0;
         virtual void playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) = 0;
         virtual void playerPingUpdate(PlayerID gamePlayer, DWORD ping) = 0;
         virtual void allPlayersLoaded( void ) = 0;
         virtual void launchStarted(void) = 0;
         virtual void launchTimeUpdate(DWORD time) = 0;
         virtual void launchAborted(PlayerID gamePlayer, long reason) = 0;
         virtual void startGame(void) = 0;
         virtual void initialSettingsComplete(void) = 0;
         virtual void incomingChat(const BSimString &user, const BSimString &message)=0;
   };

   void              addObserver(BMPGameObserver *o) { mMPGameObserverList.Add(o); }
   void              removeObserver(BMPGameObserver *o) { mMPGameObserverList.Remove(o); }
   void              setJoinRequestHandler(BMPJoinRequestHandler* handler);

   BMPGameView*      createView(void);
   void              destroyView(BMPGameView *view);
   
   void              initialize(BDataSet *dataSet, DWORD checksum);
   void              service(void);

   HRESULT           host(const BSimString& localname, const BMPGameDescriptor &descriptor);
   HRESULT           join(const BSimString& localname, const BMPGameDescriptor &descriptor);
   bool              setSlotOpen(long slot, bool open);
   bool              isHosting(void) const;
   bool              requestGameLaunch(DWORD countdown);
   bool              requestLaunchAbort(long reason);
   bool              sendLaunchReady(bool ready);
   void              setGameState(long state);
   long              getGameState(void) const { return mGameState; }

   bool              startGame(void);              // starting the game
   bool              stopGame(void);               // stop the game. Really? No shit? Won't work.
   void              gameStartComplete(void);      // start is complete
   bool              waitingOnOtherPlayers(void);  // waiting on other players, ready to finalize
   bool              finalizeGame(void);           // play the game
   BMPSimObject      *getSimObject(void) { return mSimObject; }

   bool              attachFileXfer(BFileTransferGameInterface *xfer);
   BFileTransferGameInterface *getFileXfer();
   virtual DWORD     getClientIDFromPlayerID(long playerID);
   virtual long      getPlayerIDFromClientID(DWORD clientID);


   // Player functions
   void              setMaxPlayers(long maxCount);
   long              getMaxPlayers() const { return mMaxPlayers; }
   long              getPlayerCount(void) const;
   ClientID          getClientID(PlayerID playerID) const;
   PlayerID          getPlayerID(ClientID clientID) const;
   PlayerID          getPlayerID(const BSimString &playerName) const;
   const BSimString*    getPlayerName(ClientID clientID) const;
   bool              allPlayersLoaded(void) const;   
   bool              allPlayersReadyToStart(void) const;
   long              getLocalPlayer(void) const;
   long              getControlledPlayer(void) const { return(mControlledPlayer); }
   void              setControlledPlayer(long p) { mControlledPlayer=p; }   
   void              kickPlayer(PlayerID playerID);
   
   // Vote
   virtual void      castVote(long type, long vote, long arg1 = -1);
   virtual void      startVote(long type, long arg1 = -1);
   virtual void      abortVote(long type, long arg1 = -1);
   
   // BMPGameView accessors
   void              setLocalUpdates(bool val);
   bool              setSetting(DWORD index, void *data, long size);
   #ifndef BUILD_FINAL
   bool              setSettingOverride(DWORD index, void *data, long size);
   #endif   
   
   // BSession::BSessionEventHandler
   virtual void      processSessionEvent(const BSessionEvent *pEvent);

   // BSession::BClientConnector
   virtual bool      connectionAttempt(const SOCKADDR_IN &remote, const SOCKADDR_IN &xremote);

   // From BSessionConnector::BSCObserver
   //
   virtual void      findListUpdated(BSessionConnector *connector)  { connector; }
   virtual void      joinRequest(BSessionConnector *connector, const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result);
   virtual void      joinReply(BSessionConnector *connector, long index, eJoinResult result)  { connector;index; result; }
   
   // MPChat2 accessors
   //virtual void      subscribe(MPChatChannelObserver *o);
   //virtual void      unsubscribe(MPChatChannelObserver *o);
   //virtual void      setChannelName(const BSimString& name);
   //virtual const BSimString&  getChannelName() const;
   //virtual bool      isPasswordProtected() const { return true; }
   //virtual void      setPasswordProtected(bool /*val*/) {}
   //virtual bool      isVisible() const { return(false); }
   //virtual bool      isModerated() const { return(false); }
   virtual void      sendChat(const BSimString& message);   
   virtual void      sendChat(const BSimString& message, const BDynamicSimArray<BSimString>& userList);
   virtual void      sendVoice( const char* voiceData, const long dataLength, const DWORD senderXUID);
   //virtual void      whisper(const BSimString &user, const BSimString &message);
   //virtual void      parseAndSendChat(const BSimString& message);   
   //virtual void      leaveChannel() {}

   virtual bool      canDispose() { return false; }

   // BTimeSync::BTimingInfo methods
   virtual HRESULT   getLocalTiming(unsigned char *timing, DWORD *deviationRemaining);   
   virtual float     getMSPerFrame(void);

   // BMPSyncObject methods
   virtual bool      sendSyncData(long uid, void *checksum, long checksumSize);
   virtual long      getSyncedCount(void) const;
   virtual void      outOfSync(void) const;

   // BMPSimObject methods
   HRESULT           sendCommandPacket(BChannelPacket &packet);   
   HRESULT           sendSimPacket(BChannelPacket &packet);
   HRESULT           sendGCPacket(BChannelPacket &packet);

   DWORD             getChecksum(void) { return mChecksum; }

   // BChannel::BChannelObserver
   virtual void      channelDataReceived(const long fromClientIndex, const void *data, const DWORD size);

protected:
   friend class BMPSimObject;

   class BMPGameObserverList : public BObserverList< BMPGameObserver >
   {
      DECLARE_OBSERVER_METHOD( gameDestroyed, (), () )
      DECLARE_OBSERVER_METHOD( gameConnected, (BMPGame *game), (game) )
      DECLARE_OBSERVER_METHOD( gameConnectFailed, (long reason), (reason) )
      DECLARE_OBSERVER_METHOD( gameConnectTimeUpdated, (long connectTime), (connectTime) )
      DECLARE_OBSERVER_METHOD( gameDisconnected, (BMPGame *game, long reason), (game, reason) )
      DECLARE_OBSERVER_METHOD( playerJoined, (ClientID mpPlayer, const BSimString& name, bool local, BMPPlayerEntry **pEntry), (mpPlayer, name, local, pEntry) )
      DECLARE_OBSERVER_METHOD( playerLeft,   (PlayerID gamePlayer, bool local), (gamePlayer, local) )
      DECLARE_OBSERVER_METHOD( playerNotResponding, (PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect), (gamePlayer, lastResponseTime, disconnect))
      DECLARE_OBSERVER_METHOD( playerResponding, (PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect), (gamePlayer, lastResponseTime, disconnect))
      DECLARE_OBSERVER_METHOD( playerPingUpdate, (PlayerID gamePlayer, DWORD ping), (gamePlayer, ping) )
      DECLARE_OBSERVER_METHOD( allPlayersLoaded,  (), () )
      DECLARE_OBSERVER_METHOD( launchStarted, (), () )
      DECLARE_OBSERVER_METHOD( launchTimeUpdate, (DWORD time), (time) )
      DECLARE_OBSERVER_METHOD( launchAborted, (PlayerID player, long reason), (player, reason) )
      DECLARE_OBSERVER_METHOD( startGame, (void), () )
      DECLARE_OBSERVER_METHOD( initialSettingsComplete, (void), () )
      DECLARE_OBSERVER_METHOD( incomingChat, (const BSimString &user, const BSimString& message), (user, message))
   };

   BMPGameObserverList   mMPGameObserverList;

   void              cleanup();
   void              handleClientData(const BSessionEvent *pEvent);
   void              sessionConnected(void);
   void              sessionDisconnected(long reason);
   void              clientConnected(DWORD clientIndex);
   void              clientDisconnected(const BSessionEvent *pEvent);
   void              clientNotResponding(DWORD clientIndex, DWORD lastResponseTime);
   void              clientResponding(DWORD clientIndex, DWORD lastResponseTime);
   void              clientPingUpdated(DWORD clientIndex, DWORD ping);
   void              handleChannelData(const BSessionEvent *pEvent);

   HRESULT           createSimObject(void);
   void              destroySimObject(void);   
   bool              createSimChannels(void);
   void              destroySimChannels(void);
   bool              createTimeSyncChannel(void);
   void              destroyTimeSyncChannel(void);
   bool              createSyncChannel(void);
   void              destroySyncChannel(void);
   bool              createVoteChannel(void);
   void              destroyVoteChannel(void);
   bool              createMessageChannel(void);
   void              destroyMessageChannel(void);
   bool              createSettingsChannel(void);
   void              destroySettingsChannel(void);
   bool              createGCChannel(void);
   void              destroyGCChannel(void);
   bool              createChat(void);
   void              destroyChat(void);

   void              fillCompleteSettings(BCompleteSettingsPacket &packet);
   void              handleSettingsPacket(BSettingsPacket &packet);
   void              handleCompleteSettings(long type, const void* data, long size);
   void              handleMessage(long clientIndex, const void* data, long size);
   void              handleSettings(long clientIndex, const void* data, long size);
   void              handleVote(long clientIndex, const void* data, long size);
   void              connectPlayer(DWORD clientIndex);
   
   BMPPlayer*        getPlayerFromID(long id);

// for debugging
public:   
   BSession*         getSession(void) const { return(mSession); }
private:   

   enum
   {
      cChannelChat,
      cChannelCommand,
      cChannelSync,
      cChannelVote,
      cChannelMessage,
      cChannelSim,
      cChannelSettings,
      cChannelGC,

      cChannelMax
   };

   enum
   {
      cLaunchUpdateFrequency = 500
   };

   bool              mbGameStarted;
   bool              mbSettingsLocked;
   bool              mbUpdateLocalSettings;
   bool              mSessionConnected;
   bool              mSettingsComplete;
   bool              mGameConnected;
   bool              mGameEnded;
   long              mGameState;
   DWORD             mChecksum;
   BConnectivity     *mConnectivity;
   BDataSet          *mGameSettings;
   BSession          *mSession;
   BMPSimObject      *mSimObject;
   BMPJoinRequestHandler *mJoinRequestHandler;

   BChannel          *mChannelArray[cChannelMax];      

   BSessionConnector *mSessionConnector;
   BFileTransferMgr  *mFileTransferMgr;
   
   long              mControlledPlayer;

   long              mMaxPlayers;
   BMPPlayerMap      mPlayerMapper;
   BMPPlayerList     mPlayers;               // This is the list of clients (aka - mp players)
   BAddressArray     mConnectingAddresses;   // This is a list of addresses that have requested a connection with us
   long              findConnectingAddresses(const SOCKADDR_IN& addr, const SOCKADDR_IN& xaddr);

   DWORD             mLockedPlayers;

   DWORD             mLaunchCountdown;
   DWORD             mLaunchRequestTime;
   DWORD             mLaunchLastUpdate;
   BSimString           mChatChannelName;

   /*class MPChatChannelObserverList : public BObserverList< MPChatChannelObserver >
   {
      DECLARE_OBSERVER_METHOD(channelJoinEvent,  (const MPChatUser& user), (user))
      DECLARE_OBSERVER_METHOD(channelLeaveEvent, (const MPChatUser& user), (user))
      DECLARE_OBSERVER_METHOD(channelChatEvent,  (const BSimString& channelName, const BSimString& message, const BSimString& user), (channelName, message, user))
   };
   MPChatChannelObserverList mChatObservers;*/
};

#endif //__MPGAME_H__
