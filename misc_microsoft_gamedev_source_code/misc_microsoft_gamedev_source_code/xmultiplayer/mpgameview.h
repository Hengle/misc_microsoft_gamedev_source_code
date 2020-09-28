//==============================================================================
// mpgameview.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once

#ifndef __MPGAMEVIEW_H__
#define __MPGAMEVIEW_H__

//==============================================================================
// Includes
#include "array.h"
#include "DataSet.h"
#include "mpgame.h"

//==============================================================================
// Forward declarations
//class BMPGame;
class BDataSet;
class BMPPlayer;
//class BMPChatRouter;
class BFileTransferGameInterface;
//==============================================================================
// Const declarations


//==============================================================================
class BMPGameView : public BDataSet::BDataListener,
                    public BMPGame::BMPGameObserver

{
public:
   BMPGameView(BMPGame *mpGame, BDataSet *settings);
   ~BMPGameView();

   enum { cLaunchAbortESOFailure, cLaunchAbortUser, cLaunchAbortHCFailed };

   class BMPGameViewObserver
   {
   public:
      virtual void onSettingsChanged(const BDataSet* settings, DWORD index, BYTE flags) {settings;index;flags;}
      virtual void gameConnected(void) {}
      virtual void gameDisconnected( long reason ) { reason; }
      virtual void playerJoined( ClientID mpPlayer, const BSimString& name, bool local, BMPPlayerEntry **pEntry ) { mpPlayer; name; local; pEntry; }
      virtual void playerLeft( PlayerID gamePlayer, bool local ) { gamePlayer; local; }
      virtual void playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) { gamePlayer; lastResponseTime; disconnect;}
      virtual void playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) { gamePlayer; lastResponseTime; disconnect;}
      virtual void playerPingUpdate(PlayerID gamePlayer, DWORD ping) { gamePlayer; ping; }
      virtual void allPlayersLoaded(void) {}
      virtual void launchStarted(void) {}
      virtual void launchTimeUpdate(DWORD time) {time;}
      virtual void launchAborted(PlayerID gamePlayer, long reason) { gamePlayer; reason; }
      virtual void startGame (void) {}
      virtual void initialSettingsComplete(void) {}
      virtual void incomingChat(const BSimString& /*user*/, const BSimString& /*message*/)      {}
   };
   
   class BGameViewObserverList :
		public BObserverList <BMPGameViewObserver>
	{
		DECLARE_OBSERVER_METHOD (onSettingsChanged,
			(const BDataSet* settings, DWORD index, BYTE flags),
			(settings, index, flags))

      DECLARE_OBSERVER_METHOD (gameConnected, (), ())

		DECLARE_OBSERVER_METHOD (gameDisconnected,
			(long reason), (reason))

		DECLARE_OBSERVER_METHOD (playerJoined,
			(ClientID player, const BSimString& name, bool local, BMPPlayerEntry **pEntry), (player, name, local, pEntry))

		DECLARE_OBSERVER_METHOD (playerLeft,
			(PlayerID player, bool local), (player, local))

      DECLARE_OBSERVER_METHOD (playerNotResponding, 
         (PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect), (gamePlayer, lastResponseTime, disconnect) )
      DECLARE_OBSERVER_METHOD (playerResponding, (PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect), (gamePlayer, lastResponseTime, disconnect) )
      DECLARE_OBSERVER_METHOD (playerPingUpdate, (PlayerID gamePlayer, DWORD ping), (gamePlayer, ping) )
      DECLARE_OBSERVER_METHOD (allPlayersLoaded, (), ())
      DECLARE_OBSERVER_METHOD (launchStarted, (), ())
      DECLARE_OBSERVER_METHOD (launchTimeUpdate, (DWORD time), (time) )
      DECLARE_OBSERVER_METHOD (launchAborted, (PlayerID player, long reason), (player, reason) )
      DECLARE_OBSERVER_METHOD (startGame, (), () )
      DECLARE_OBSERVER_METHOD (initialSettingsComplete, (), () )
      DECLARE_OBSERVER_METHOD( incomingChat, (const BSimString &user, const BSimString& message), (user, message))
	};

   
   bool  addObserver(BMPGameViewObserver* o);
   bool  removeObserver(BMPGameViewObserver* o);   

   void  setJoinRequestHandler(BMPJoinRequestHandler* handler);

   HRESULT  host(const BSimString &nickname, const BMPGameDescriptor &descriptor);
   HRESULT  join(const BSimString &nickname, const BMPGameDescriptor &descriptor);
   bool     setSlotOpen(long slot, bool open);
   bool     joinRequest(const BSimString& name, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, BSessionConnector::BSCObserver::eJoinResult &result);

   // Settings functions
   void  setLocalUpdates(bool val);
   bool  setLong(DWORD index, long  data);
   bool  setDWORD(DWORD index, DWORD data);
   bool  setFloat(DWORD index, float data);
   bool  setShort(DWORD index, short data);
   bool  setWORD(DWORD index, WORD  data);
   bool  setBool(DWORD index, bool  data);
   bool  setChar(DWORD index, char  data);
   bool  setBYTE(DWORD index, BYTE  data);
   bool  setString(DWORD index, const BSimString& data);

   #ifndef BUILD_FINAL
   bool  setBoolOverride(DWORD index, bool  data);
   #endif

   bool  getLong(DWORD index, long  &data) const;
   bool  getDWORD(DWORD index, DWORD &data) const;
   bool  getFloat(DWORD index, float &data) const;
   bool  getShort(DWORD index, short &data) const;
   bool  getWORD(DWORD index, WORD  &data) const;
   bool  getBool(DWORD index, bool  &data) const;
   bool  getChar(DWORD index, char  &data) const;
   bool  getBYTE(DWORD index, BYTE  &data) const;
   bool  getString(DWORD index, BSimString &data) const;  

   bool  attachFileXfer(BFileTransferGameInterface *xfer);
   BFileTransferGameInterface *getFileXfer();

   // Player functions
   void  setMaxPlayers(long maxCount);
   long  getMaxPlayers() const;

   long  getPlayerCount(void) const;
   ClientID getClientID(PlayerID playerID) const;
   PlayerID getPlayerID(ClientID clientID) const;
   PlayerID getPlayerID(const BSimString &playerName) const;
   PlayerID getControlledPlayerID(void) const;
   const BSimString* getPlayerName(PlayerID playerID) const;
   void  kickPlayer(PlayerID playerID);
   bool  isHosting(void) const;
   bool  requestGameLaunch(DWORD countdown);
   bool  requestLaunchAbort(long reason);
   void  sendLaunchReady(bool ready);
   bool  isLaunching() const;
   void  sendVoice( const char* voiceData, const long dataLength, const DWORD senderXUID);

   // BDataSet::BDataListener
   virtual void OnDataChanged(const BDataSet* set, DWORD index, BYTE flags);

   // BMPGame::BMPGameObserver
   virtual void gameDestroyed();
   virtual void gameConnected( BMPGame *pGame );
   virtual void gameConnectFailed(long reason);
   virtual void gameConnectTimeUpdated( long connectTime );
   virtual void gameDisconnected( BMPGame *pGame, long reason );
   virtual void playerJoined( DWORD mpPlayer, const BSimString& name, bool local, BMPPlayerEntry **pEntry );
   virtual void playerLeft( long gamePlayer, bool local );
   virtual void playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect);
   virtual void playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect);
   virtual void playerPingUpdate(PlayerID gamePlayer, DWORD ping);
   virtual void allPlayersLoaded( void );
   virtual void launchStarted(void);
   virtual void launchTimeUpdate(DWORD time);
   virtual void launchAborted(PlayerID gamePlayer, long reason);
   virtual void startGame(void);
   virtual void initialSettingsComplete(void);
   virtual void incomingChat(const BSimString &user, const BSimString &message);


   virtual void      sendChat(const BSimString& message);   
   virtual void      sendChat(const BSimString& message, const BDynamicSimArray<BSimString>& userList);

protected:

   BMPGame                    *mMPGame;   
   BGameViewObserverList      mObservers;
   BDataSet                   *mSettings;
};

#endif //__MPGAMEVIEW_H__