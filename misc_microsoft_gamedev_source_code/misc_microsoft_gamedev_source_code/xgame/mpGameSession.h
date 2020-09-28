//==============================================================================
// mpGameSession.h
//
// Copyright (c) 2008, Ensemble Studios
//==============================================================================

#pragma once

// System which manages a game session
//  This is the actual session which lives for ONE game
//  It can be LAN or Live
//  It starts with either hosting or joining to another

//==============================================================================
//Includes
//==============================================================================
#include "TimeSync.h"
#include "Channel.h"
#include "Session.h"
#include "mpSession.h"
#include "mpsync.h"

//==============================================================================
//Forward declarations
//==============================================================================
class BLiveVoice;
class BLiveSession;

//==============================================================================
//Object to track pending user connections
//==============================================================================
class BMPSessionPendingPlayer
{
   public:
      BMPSessionPendingPlayer();

      XUID     xuid;                   // 8
      ClientID clientID;               // 4
      PlayerID playerID;               // 4
      DWORD    lastTimeUpdated;        // 4
      bool     liveSessionJoined : 1;  // 1 (1/8)
};
//End of BMPSessionPendingPlayer

//==============================================================================
//Object to map network clientIDs to playerIDs (and thus slots in the gameSettings)
//==============================================================================
class BMPSessionPlayer
{
   public:
      BMPSessionPlayer();
      ~BMPSessionPlayer();

      ClientID mClientID;        // 4
      PlayerID mPlayerID;        // 4
      byte*    mpTrackingDataBlock; // 4
      bool     mLiveEnabled : 1; // 1 (1/8) Set to false for AI players
};
//End of BMPSessionPlayer

//==============================================================================
//Tracks connections who have loaded the level (MachineID driven)
//==============================================================================
class BMPSessionLoadTracker
{
   public:
      BMPSessionLoadTracker();
      ~BMPSessionLoadTracker();

      void setPreLoadReady(long machineID, bool loaded);
      void setLoaded(long machineID, bool loaded);
      void dropMachine(long machineID);
      bool getAreAllPlayerPreLoadReady();
      bool getAreAllPlayersLoaded();
      void reset();
      void initializeMembersToSessionMembers(BSession* pSession);

   private:

      class BMPSessionLoadTrackerMember 
      {
         public:
            long     mMachineID;    // 4
            bool     mLoaded : 1;   // 1 (1/8)
            bool     mPreLoadReady : 1;
      };

      BMPSessionLoadTrackerMember mMemberList[cMPSessionMaxUsers]; // 5 * 8 == 40
      bool                        mInitialized : 1;                // 1 (1/8)
};
//End of BMPSessionLoadTracker

//==============================================================================
// BMPGameSession
//==============================================================================
class BMPGameSession  :
   public   BDataSet::BDataListener,
   public   BSession::BClientConnector,
   public   BSession::BSessionEventObserver,
   public   BChannel::BChannelObserver,
   public   BMPSyncObject,
   public   BTimingInfo
{
   public:
      enum BMPGameSessionState
      {
         cBMPGameSessionStateNone =0,
         cBMPGameSessionStateLaunchHostWaitingForSessionCreation,
         cBMPGameSessionStateLaunchHostWaitingForSelfConnection,
         //cBMPGameSessionStateHostReady,
         cBMPGameSessionStateJoinWaitingForLiveSession,
         cBMPGameSessionStateJoinWaitingForConnection,
         cBMPGameSessionStateJoinWaitingForInitialSetup,
         //cBMPGameSessionStateJoinReady,
         cBMPGameSessionStateReady,

         cBMPGameSessionStateGameSetup,
         cBMPGameSessionStateWaitingOnArbitrationRegistration,
         cBMPGameSessionStateWaitingOnSecondaryArbitrationRegistration,
         cBMPGameSessionStateLaunching,
         cBMPGameSessionStateStartPregame,
         cBMPGameSessionStateInGame,
         cBMPGameSessionStatePostGame,
         cBMPGameSessionStateEnding,
         cBMPGameSessionStateFinished,
         cBMPGameSessionStateShutdownRequested,
         cBMPGameSessionStateShuttingDown,
         cBMPGameSessionStateReadyForDelete,
         cBMPGameSessionStateDestructed,
      };

      //Host constructor
      //Joiner constructor
      BMPGameSession(BMPSession* mpSession, BDataSet* pSettings);
      ~BMPGameSession();

      // BDataSet::BDataListener interface that we implement
      virtual void OnDataChanged(const BDataSet* set, DWORD index, BYTE flags);

      //BSession::BClientConnector interface that we implement
      //virtual bool connectionAttempt( BClient* requestingClient );
      //Callback going up when we have a client requesting to connect to the host of the session.
      //  If approved, then they are assigned a client ID, and allowed to try and fully connect to the session
      virtual bool sessionConnectionRequest(const BSessionUser users[], BSession::BJoinReasonCode* reasonCode);

      //BSession::BSessionEventObserver interface that we implement
      virtual void processSessionEvent(const BSessionEvent* pEvent);

      // BTimeSync::BTimingInfo interface that we implement
      virtual HRESULT   getLocalTiming(uint32& timing, uint32* deviationRemaining);   
      virtual float     getMSPerFrame(void);

      // BChannel::BChannelObserver interface that we implement - holy crap there is alot of this
      virtual void      channelDataReceived(const long fromClientIndex, const void *data, const DWORD size);

      // BMPSyncObject interface that we implement
      virtual bool      sendSyncData(long uid, uint checksum);
      virtual long      getSyncedCount(void) const;
      virtual void      outOfSync(void) const;

      //Methods to support BMPSimDataObject
      HRESULT           sendCommandPacket(BChannelPacket& packet);
      HRESULT           sendSimPacket(BChannelPacket& packet);

      //Player management methods
      PlayerID  requestPlayerID(ClientID clientID, const XUID xuid, const BSimString& gamertag);
      void      createPlayer(PlayerID playerID, ClientID clientID, bool liveEnabled);
      void      reservePlayerID(PlayerID pid, const XUID xuid);
      void      releasePlayerID( PlayerID id);
      PlayerID  getPlayerID( ClientID clientID );
      PlayerID  getPlayerIDByGamerTag( XUID xuid );
      void      setReadyToStart(BMachineID machineID, bool setReady);
      bool      getAreAllPlayersReadyToStart();
      void      resetStartState();
      void      joinRequest(const BSessionUser users[], BSession::BJoinReasonCode &result);
      long      getPlayerCount(void);
      void      resetPlayerSystem();
      bool      isClient(PlayerID id);


      //Per frame update to be called whenever the class is instanced
      void update();

      //Called to gracefully shutdown from any current state, 
      void shutDown();
      //Call this to see if it is safe to be deleted
      bool isShutDown();

      //Call this to start up hosting a LAN session
      bool hostStartupLAN(DWORD controllerID, const BSimString& gameName, DWORD gameType, UINT iPublicSlots, UINT iPrivateSlots);

      //Call this to start up hosting a LIVE session
      bool hostStartupLive( DWORD controllerID, DWORD gameType, BOOL ranked, UINT iPublicSlots, UINT iPrivateSlots );

      //Call this to join a particular session
      bool joinLanGame(const BLanGameInfo& lanInfo);
      bool joinLiveGame(DWORD controllerID, BLiveGameDescriptor* gameDescriptor);
      bool joinLiveGameFromInvite();

      BSession*      getSession(void) const { return(mpSession); } 
      uint64         getNonce() const;
      BLiveSession*  getLiveSession() const { return mpLiveSession; }

      const XNKID&   getXNKID() const { return mLocalXNKID; }
      const XNKEY&   getXNKEY() const { return mLocalXnKey; }
      const XNADDR&  getHostXnAddr() const { return mHostXnAddr; }

      void           setPublicSlots(long maxCount) {mPublicSlots = maxCount;};
      long           getMaxPlayers() const { return (mPublicSlots + mPrivateSlots); }
      //Call this to alter the public slot count
      void           setMaxPlayerCount(uint32 maxPlayers); 
      UINT           getGameTypeIndex() {return mGameTypeIndex;};


      BMPSimDataObject*       getSimObject() const { return mpSimObject; }
      DWORD                   advanceGameTiming();

      //Returns true if system is running and ready for use
      BOOL isRunning() const;
      BOOL isGameRunning() const { return ((mState >= cBMPGameSessionStateWaitingOnArbitrationRegistration) && (mState <= cBMPGameSessionStateEnding)); }
      bool isGame() const { return mGameValid; }

      // Called to verify that all the players in the game are done loading the map and are ready to go.
      bool getAreAllPlayersLoaded() const { return (mState == cBMPGameSessionStateInGame); }

      //Returns true if I am hosting
      BOOL isHosting() const;

      //Game settings methods
      bool  setSetting(DWORD index, void *data, long size);

      bool                    requestGameLaunch(DWORD countdown);
      bool                    requestLaunchAbort(long reason);
      void                    sendLaunchReady(bool ready);
      //Called by the game system when the level is done loading
      void gameDoneLoading();
      BMPSessionPlayer* getPlayerFromPlayerId( PlayerID playerID );

      bool allMachinesFinalized() const;
      void updateBroadcastedHostData();

   protected:
      //Channel list
      enum
      {
         cChannelCommand = 0,
         cChannelSync,
         cChannelVote,
         cChannelMessage,
         cChannelSim,
         cChannelSettings,
         cChannelMax
      };

      void setupChannels();
      void destroyChannels();

      void hostStartupProcessing();
      void hostSendStartPreGame();
      void hostSendStartGame();
      void handleProcessLockSettings();
      void fillCompleteSettings(BCompleteSettingsPacket &packet);

      void setQoSNotification(bool enabled);
      void setState(BMPGameSessionState newState);
      void processShutDown();

      void              handleClientData(const BSessionEvent* pEvent);
      void              handleCompleteSettings(long type, const void* data, long size);
      void              sessionConnected(void);
      void              sessionDisconnected(BSession::BDisconnectReason reason);
      void              clientConnected(BClientID clientIndex, BMachineID machineID, BMachineID localMachineID, BMachineID hostMachineID, BOOL init=FALSE);
      void              clientDisconnected(const BSessionEvent* pEvent);
      void              clientPingUpdated(uint32 clientIndex, uint32 ping);
      void              handleSettingsPacket(BSettingsPacket &packet);
      void              handleChannelData(const BSessionEvent* pEvent);
      void              addPendingPlayer(ClientID clientID, PlayerID playerID=cMPInvalidPlayerID);
      void              removePendingPlayer(uint32 clientIndex);

      // internal only method, placing here for now though
      bool              checkAllPlayersInitialized();

      void              updatePendingPlayers();
      bool              finalizeGame(void); 
      void              handleMessage(long machineID, const void* data, long size);
      void              handleSettings(long machineID, const void* data, long size);

      BMPSessionPlayer* getPlayerFromNetworkClientId( ClientID clientID );
      void preGameDebugSpam();

   private:

      BMPSessionPendingPlayer mPendingPlayers[cMPSessionMaxPendingPlayers];   // 24 * 8 * 2 == 384

      BLiveSessionSearch::BQoSResponseData mQoSResponseData;                  // 197

      BMPSessionPlayer        mClientPlayerMap[cMPSessionMaxUsers];           // 12 * 8 == 96 A client ID can map to multiple player IDs (second player, AI support)

      BMPSessionLoadTracker   mLoadTracker;                    // 68

      XNADDR                  mHostXnAddr;                     // 36

      BChannel*               mChannelArray[cChannelMax];      // 24 Network channels array

      BOOL                    mMachineFinalize[XNetwork::cMaxClients];  // 24 - tracks each machine's reporting of BPacketType::cGameFinalizePacket

      XNKEY                   mLocalXnKey;                     // 16 Security key
      XNKID                   mLocalXNKID;                     // 8 Session key

      uint64                  mNonce;                          // 8

      BMPSession*             mpMPSession;                     // 4
      BLiveSession*           mpLiveSession;                   // 4 If Live, then this is the live session object

      BMPGameSessionState     mState;                          // 4
      BMPSession::BmpSessionStartupMode  mStartupMode;         // 4

      BSession*               mpSession;                       // 4
      BMPSimDataObject*       mpSimObject;                     // 4
      BDataSet*               mpGameSettings;                  // 4 Settings for this game

      PlayerID                mLocalPlayerID;                  // 4

      DWORD                   mLocalChecksum;                  // 4 Cached checksum of the local game (since its expensive to generate)
      DWORD                   mLocalControllerID;              // 4 Controller of who hosted or joined this session
      DWORD                   mLockedPlayers;                  // 4
      DWORD                   mLaunchCountdown;                // 4
      int32                   mLaunchLastUpdate;               // 4 must be signed
      DWORD                   mLaunchUpdateCounter;            // 4
      DWORD                   mJoinTimer;                      // 4 How long it has taken to join a particular host without success (yet)

      UINT                    mPublicSlots;                    // 4 If hosted - define what slots are available
      UINT                    mPrivateSlots;                   // 4
      UINT                    mGameTypeIndex;                  // 4
      UINT                    mQoSResponseDataSize;            // 4

      BOOL                    mRanked;                         // 4 If hosted, and true - this is a ranked game  

      WORD                    mPort;                           // 2 Port used to bind to (host) or connect to (join)

      uint8                   mLanguageCode;                   // 1

      bool                    mQoSResponding : 1;              // 1 (1/8)
      bool                    mLanSecurityKeyRegistered : 1;   //   (2/8)
      bool                    mSentSessionDisconnected : 1;    //   (3/8) Tracker so that this class ALWAYS tries to send a disconnect up the chain when it goes away (but only once)
      bool                    mSettingsLocked : 1;             //   (4/8)
      bool                    mSettingsComplete : 1;           //   (5/8) When true, we have received the initial game settings from the host
      bool                    mGameValid : 1;                  //   (6/8)
};
