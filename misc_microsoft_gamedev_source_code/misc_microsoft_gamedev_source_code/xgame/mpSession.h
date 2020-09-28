//============================================================================
// mpSession.h
//
// Copyright (c) Ensemble Studios, 2006-2008
//============================================================================

#pragma once

//==============================================================================
// Includes
#include "mptypes.h"

//==============================================================================
//Defines
//const DWORD cMPSessionMaxHostTimeout             = 15000;      // how long do we wait for the host to lag out before canceling?
//const DWORD cMPSessionMaxLoadingTime             = 90000;      // you'd think we could load a game in 45 seconds right?. No, apparently we need more. 1.5 mins maybe?
//DEPRICATED const DWORD cMPSessionConnectTimeout             = 20000;      // Max seconds to fully connect - FIXME: remove this and rely on BSession disconnect events
const DWORD cMPSessionMaxUsers                   = 8;
const DWORD cMPSessionLiveHostingPort            = 1000;
//const DWORD cMPSessionLANDiscoveryPort           = 1002;
const DWORD cMPSessionLANHostingPort             = 1000;
const DWORD cMPSessionDefaultGameLaunchCountdown = 3000;
//const DWORD cMPSessionLanDiscoveryScanCycleTime  = 5000;    //Rescan for local hosts every 5 seconds
//const DWORD cMPSessionLanGameUpdateExpiredTime   = cMPSessionLanDiscoveryScanCycleTime + 1000;  //Gives LAN hosts a second to respond before being dropped
//const DWORD cMPSessionJoinRequestResendTime      = 30000;         //Resend join requests every 2 seconds
//const DWORD cMPSessionJoinRequestAttempts        = 1;            //Re-attempt to join X times, 1000 is for debug - TODO CHANGE to 5
const DWORD cMPSessionMaxPendingPlayers          = cMPSessionMaxUsers * 2;
//const DWORD cMPSessionLaunchUpdateFrequency      = 500;
//const DWORD cMPSessionMinimumTimeBetweenAuths    = 10000;       //Forced time to wait between auth requests
//const DWORD cMPInvalidControllerID               = MAXDWORD;

//Forward declarations
class BSession;
class BChannel;
class BClient;
class BMPSyncObject;
class BMPGameView;
class BLiveSession;
class BLiveVoice;
class BMPGameSession;
class BMPSimDataObject;
class BLiveMatchMaking;

//Includes
#include "DataSet.h"
#include "mppackets.h"
#include "liveGameDescriptor.h"
#include "liveSessionSearch.h"
#include "TimeSync.h"
#include "Channel.h"
#include "Session.h"
#include "mpsync.h"
#include "matchMakingHopperList.h"
#include "liveMatchMaking.h"
#include "partySession.h"

//Object used to pass around matchmaking request data
class BMPSessionMatchMakingData
{
public:
   BMatchMakingHopper* mSelectedHopper;
   XUID                mPlayerXUIDs[3];
   float               mPlayerRatings[3];
};

class BMPSession :
   public   BPartySession::BPartySessionInterface,
   public   BMPSyncObject
{
public:
   //Enumerations **********************************************************************
   //Reasons for why a launch was aborted
   enum { cLaunchAbortUser };

   //Reasons why someone was disconnected
   enum cMPSessionDisconnectReasonCode
   {
      cMPSessionDisconnectReasonNormal,
      cMPSessionDisconnectReasonGameTerminated,
      cMPSessionDisconnectReasonFailedConnection,
      cMPSessionDisconnectReasonCRCMismatch,
      cMPSessionDisconnectReasonFull,
      cMPSessionDisconnectReasonDeleted,
      cMPSessionDisconnectReasonMax
   };

   //Reasons why the connect to ESO process for Live Matchmaking might fail
   enum cMPSessionESOConnectCode
   {
      cMPSessionESOConnectNormal,
      cMPSessionESOConnectAccountNotSignedIn,
      cMPSessionESOConnectLostLiveConnection,
      cMPSessionESOConnectAccountHasLiveGamesBlocked,
      cMPSessionESOConnectSerivceDown,
      cMPSessionESOConnectCouldNotContactService,
      cMPSessionESOConnectCouldNotRequestLogin,
      cMPSessionESOConnectCouldNotRequestConfig,
      cMPSessionESOConnectCouldNotLoadDefaultConfig,
      cMPSessionESOConnectCouldNotParseConfig,
      cMPSessionESOConnectCouldNoValidHoppers,
      //cMPSessionESOConnectPartySystemError,
      cMPSessionESOConnectAccountBanned
   };

   //Reasons why a matchmaking search might fail
   enum cMPSessionESOSearchCode
   {
      cMPSessionESOSearchNormal,
      cMPSessionESOSearchCodePlayerCanceled,
      cMPSessionESOSearchHopperError,
      cMPSessionESOSearchLiveProblem,
      cMPSessionESOSearchWrongNumberofPlayers,
      cMPSessionESOSearchCodeTimedOut,
      cMPSessionESOSearchHostError,
      cMPSessionESOSearchCodeESOConnectionLost,
      cMPSessionESOSearchCodePartyNoLongerValid
   };

   //Reasons why a party-based custom game startup might fail
   enum cMPSessionCustomGameStartResultCode
   {
      cMPSessionCustomGameStartResultSuccess,             //Worked fine :)
      cMPSessionCustomGameStartResultLiveSessionFailed,  
      cMPSessionCustomGameStartResultHostingError,
      cMPSessionCustomGameStartResultClientJoinError
   };

   //Reasons why a game host might fail to start
   enum cMPSessionGameSessionHostResultCode
   {
      cMPSessionGameSessionHostResultSuccess,
      cMPSessionGameSessionHostResultInternalError,
      cMPSessionGameSessionHostResultSessionError,
      cMPSessionGameSessionHostResultLiveError
   };

   //Interfaces provided to other classes ********************************************
   class mpSessionInterface
      //This is the interface that connects the game to the multiplayer system
      //This system will activate these callbacks as the events arise
      //Note: I pulled this as an observer because only 1 system used it, and I needed to add methods that are not appropriate for the observer system
   {
   public:
      virtual void mpSessionEvent_systemReady() {};
      virtual void mpSessionEvent_settingsChanged(const BDataSet* settings, DWORD index, BYTE flags) {settings;index;flags;}
      virtual void mpSessionEvent_gameConnected(void) {}
      virtual void mpSessionEvent_gameDisconnected( long reason ) { reason; }
      virtual void mpSessionEvent_partyDisconnected( long reason ) { reason; }
      virtual void mpSessionEvent_playerJoined( PlayerID gamePlayer, ClientID clientID, const XUID xuid, const BSimString &playerName ) { gamePlayer; clientID; xuid; playerName; }
      virtual void mpSessionEvent_playerLeft( PlayerID gamePlayer, BOOL local ) { gamePlayer; local; }
      //Depricating these two, they are no longer useful to the party - eric
      //virtual void mpSessionEvent_playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) { gamePlayer; lastResponseTime; disconnect;}
      //virtual void mpSessionEvent_playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) { gamePlayer; lastResponseTime; disconnect;}
      virtual void mpSessionEvent_playerPingUpdate(PlayerID gamePlayer, DWORD ping) { gamePlayer; ping; }
      virtual void mpSessionEvent_allPlayersLoaded(void) {}
      virtual BOOL mpSessionEvent_queryIsInMatchmakingMode(void) {return FALSE;};
      virtual BOOL mpSessionEvent_preGameInit(BOOL playerIsHost) {playerIsHost; return FALSE;}
      virtual void mpSessionEvent_launchStarted(void) {}
      virtual void mpSessionEvent_launchTimeUpdate(DWORD time) {time;}
      virtual void mpSessionEvent_launchAborted(PlayerID gamePlayer1, PlayerID gamePlayer2, long reason) { gamePlayer1; gamePlayer2; reason; }
      virtual void mpSessionEvent_startGame (void) {}
      virtual void mpSessionEvent_initialSettingsComplete(void) {} 
      virtual void mpSessionEvent_requestForSetLocalPlayerSettings() {}
      virtual void mpSessionEvent_LANGameListUpdated(void) {}
      virtual void mpSessionEvent_LiveGameListUpdated( BLiveSessionSearch* sessionSearch) {sessionSearch;}
      virtual void mpSessionEvent_gameSessionJoinFailed( BSession::BJoinReasonCode failureCode ) {failureCode;}
      //LSP related events
      virtual void mpSessionEvent_ESOConnectFailed( cMPSessionESOConnectCode failureCode ) {failureCode;}
      virtual void mpSessionEvent_ESOConnectComplete( BOOL restrictedNATWarning ) {restrictedNATWarning;}
      virtual void mpSessionEvent_ESOSearchStatusChanged(const BSimString& description) {description;}
      virtual void mpSessionEvent_MatchMakingStatusChanged(BLiveMatchMaking::BLiveMatchMakingStatusCode status, uint data1, uint data2) {status;data1;data2;}
      virtual void mpSessionEvent_ESOSearchFailed(cMPSessionESOSearchCode failureCode) {failureCode;}
      virtual void mpSessionEvent_ESOSearchComplete() {}
      //Party session related events
      //virtual void mpSessionEvent_partyEvent_playerJoined(ClientID newClientID, uint newMemberIndex) {newClientID;newMemberIndex;};
      //virtual void mpSessionEvent_partyEvent_playerLeft(ClientID newClientID, uint newMemberIndex) {newClientID;newMemberIndex;};
      virtual void mpSessionEvent_partyEvent_playerJoined(XUID xuid) {xuid;};
      virtual void mpSessionEvent_partyEvent_playerLeft(XUID xuid) {xuid;};
      virtual void mpSessionEvent_partyEvent_customGameStartupComplete( BMPSession::cMPSessionCustomGameStartResultCode resultCode ) {resultCode;};
      virtual void mpSessionEvent_partyEvent_gameLaunched() {};
      virtual void mpSessionEvent_partyEvent_memberSettingsChanged(BPartySessionPartyMember* changedMember) {changedMember;};
      virtual void mpSessionEvent_partyEvent_hostSettingsChanged(BPartySessionHostSettings* newSettings) {newSettings;};
      virtual void mpSessionEvent_partyEvent_partySizeChanged(DWORD newMaxPartyMemberCount) {newMaxPartyMemberCount;};
      virtual void mpSessionEvent_partyEvent_hostSubmitFinalGameSettings() {};
      virtual void mpSessionEvent_partyEvent_joinFailed( BSession::BJoinReasonCode failureCode ) {failureCode;}
   };


   class GameInterface
      //This is the interface that defines the callbacks to the game that need data returned and decisions made
      //TODO - look and see if we have more than one observer, if not - collapse those into this one interface
   {
   public:
      virtual void setupSync(BMPSyncObject *object) {};
      virtual BDataSet *getGameDataSet(void) {return NULL;};
      virtual DWORD getLocalChecksum(void) {return 0;};
      virtual bool populateDefaultGameInfo(BMPGameView *gameView) {return false;};
      virtual bool startGameSync(void) {return false;};
      //virtual void initCommLogging(void) {};
      virtual bool isOOS(void) {return false;};
      virtual void networkDisabled(void){};   
      virtual long getDataDirID(void) {return 0;};
      virtual long getGametime(void) {return 0;};
   };

   // Interfaces from other classes that this class implements **************************************

   //Interface that mpSession implements to manage events from the party session
   virtual void BPartySessionEvent_systemReady();
   virtual void BPartySessionEvent_hostRunning();
   virtual void BPartySessionEvent_settingsChanged();
   virtual void BPartySessionEvent_systemDisconnected();
   virtual void BPartySessionEvent_systemStartupFailed(BSession::BJoinReasonCode failureCode);
   //virtual void BPartySessionEvent_playerJoined(ClientID newClientID, uint newMemberIndex);
   //virtual void BPartySessionEvent_playerLeft(ClientID newClientID, uint newMemberIndex);
   virtual void BPartySessionEvent_playerJoined(XUID xuid);
   virtual void BPartySessionEvent_playerLeft(XUID xuid);
   virtual void BPartySessionEvent_memberSettingsChanged(BPartySessionPartyMember* changedMember);
   virtual void BPartySessionEvent_hostSettingsChanged(BPartySessionHostSettings* pNewSettings);
   virtual void BPartySessionEvent_partySizeChanged(DWORD newMaxPartyMemberCount);
   virtual void BPartySessionEvent_matchMakingStatusChanged(uint8 status, uint data1, uint data2);

   // BMPSyncObject interface that we implement
   virtual bool      sendSyncData(long uid, uint checksum);
   virtual long      getSyncedCount(void) const;
   virtual void      outOfSync(void) const;

   // **************************************************************************************

   // Startup modes
   enum BmpSessionStartupMode
   {
      mpSessionStartupNone,
      mpSessionStartupModeLAN,
      mpSessionStartupModeLive
      //mpSessionStartupModeLiveGamesList
   };

   // valid mp session states
   enum liveMPSessionState
   {
      cMPSessionStateNone,
      mpSessionStateError,

      //Startup states
      mpSessionStartupWaitingForOKFromLiveSystem,
      mpSessionStateStartingWaitingForXNAddr,
      mpSessionStateStartingWaitingForLoggin,
      mpSessionMMWaitingForESOLogin,
      mpSessionMMWaitingForESOConfigurationData,
      mpSessionMMSkipNewESOConfigurationData,
      mpSessionStateLANSessionDiscovery,
      mpSessionStateStartingPartySession,

      //System is ready, no game session running
      mpSessionStateIdle,

      //Game session is in process of being hosted
      mpSessionStateStartingGameSessionHost,

      //Game session is in process of being joined
      mpSessionStateStartingGameSessionJoin,

      //Game session is running
      mpSessionStateGameSessionRunning,

      //Waiting for game session cleanup so that I can return to Idle
      mpSessionStateGameSessionShuttingDown,

      //Going to reset next update
      mpSessionStateGameSessionResetNextUpdate,

      //Waiting for game, party, matchmaking cleanup so that I can return to stateNone (ie: ready to be startUp())
      mpSessionStateShuttingDownToReset,

      //Waiting for cleanup so that I can shutdown to be released
      mpSessionStateShuttingDownToDelete,

      //Delete me ok!
      mpSessionStateShutdownComplete,

      /*
      //Matchmaking states
      mpSessionMMCreatingPartySession,
      mpSessionMMReadyToStartMatchmaking,
      mpSessionMMChangingSettings,
      mpSessionMMSearching,
      mpSessionMMHosting,
      mpSessionMMResetToResume,
      mpSessionMMAborting,
      mpSessionMMGamePending,

      //Hosting states - party-based custom games
      mpSessionStateLaunchHostWaitingForSessionCreation,
      mpSessionStateReady,

      //Client states
      mpSessionStateJoiningWaitingForLiveSession,
      mpSessionStateJoiningWaitingForConnection,
      mpSessionStateJoiningWaitingForInitialSetup,
      mpSessionStateJoiningReady,
      
      //In-game states
      mpSessionStateGameSetup,
      mpSessionStateGameLaunching,
      mpSessionStateGameWaitingOnArbitrationRegistration,
      mpSessionStateGameWaitingOnSecondaryArbitrationRegistration,
      mpSessionStateGameStartPregame,              //Launch countdown is over, waiting for start game packets to go out
      mpSessionStateInGame,
      mpSessionStatePostGame,
      */
      mpSessionStateFinal,
      mpSessionStateEnding,
      mpSessionStateFinished,
      mpSessionStateDeleting

   };

   //Constructor/destructor
   BMPSession();
   ~BMPSession();

   void init(BLiveVoice* pVoice);

   //Starts up the session, getting it ready to start joining or hosting
   void startUp(GameInterface* gInt, mpSessionInterface* sInt, BDataSet *settings, BmpSessionStartupMode startupMode, DWORD controllerID);
   //Shut down any running session and clean up
   void shutdown();
   //Call this to query if the system can be shutdown and deleted immediately
   bool isOkToDelete() {return (mState==mpSessionStateShutdownComplete || mState==cMPSessionStateNone);};
   //Call this to query if the system started up
   bool isOkToStartup() {return (mState==cMPSessionStateNone);};  
   //Per-frame update for this system
   void update();
   //Lets other system query what mode I'm in
   bool isInLANMode( void ) const {return (mStartupMode==mpSessionStartupModeLAN); }
   //Returns true if the system is idle and ready to start joining/hosting a game session
   bool isReadyToStartGameSession(void) {return mState==mpSessionStateIdle;};
   //Returns true if system is running, and a game is running, and ready for use
   BOOL isRunning() const;
   //Returns true if the system is currently in-game
   BOOL isGameRunning() const;
   //Call this if you are trying to figure out if the current game is a matchmade game or not
   BOOL isMatchmadeGame();
   //Call this if you want to know if all members in a matchmaking party have joined to the current target
   BOOL isMatchMakingPartyFullyJoinedToCurrentTarget();
   //Call this to fire off the end of game logic that posts scores, changes voice channels, etc
   void processEndOfGame();
   //Returns true if I am hosting
   BOOL isHosting() const;
   //Returns true if I am behind a restrictive NAT
   bool hasRestrictiveNAT() const {return (mNatType==XONLINE_NAT_STRICT?true:false);};
   //Returns true if I started up my current session through an invite
   BOOL isSessionFromInvite() {return mJoinedViaInvite;};
   BSession* getSession(void) const;
   GameInterface* getGameInterface() const { return(mGameInterface);};
   //Get a pointer to the active Live game session object
   BLiveSession* getLiveSession( void );
   BMPGameSession*  getGameSession() const {return (mGameSession);};
   //Call this to alter the public slot count
   void setMaxPlayerCount(uint32 maxPlayers);   
   //Call this to see if we are in no LSP mode
   //BOOL isInNoLSPMode() {return mNoLSPMode;};
   //Outside systems can use this to set us in the no LSP mode
   //void setInNoLSPMode() {mNoLSPMode=TRUE;};

   //Clears the session interface so the callbacks don't fire any more
   void                    clearSessionInterface(); // {mSessionInterface=NULL;};
   //Sets the session interface to a new target
   void                    setSessionInterface(mpSessionInterface* pInterface); // {mSessionInterface=newInterface;};
   mpSessionInterface*     getSessionInterface() const { return mSessionInterface; }
   bool                    isSessionInterfaceActive() {return (mSessionInterface!=0);};

   //Call this to start up hosting a LAN session
   bool hostStartupLocal( BSimString gameName, DWORD gameType, UINT iPublicSlots, UINT iPrivateSlots  );

   //Call this to start up hosting a LIVE session
   bool hostStartupLive( DWORD gameType, BOOL ranked, UINT iPublicSlots, UINT iPrivateSlots );

   //Call this to join a particular session
   bool joinLanGame(const BLanGameInfo& lanInfo);
   bool joinLiveGame(BLiveGameDescriptor* gameDescriptor);
   bool joinLiveGameFromInvite(BPartySessionPlayerSettings* playerSettings);

   //Methods used by the game session to let us know certain events
   void gameSessionJoinFailed(BSession::BJoinReasonCode failureCode);
   void gameSessionHostFailed(cMPSessionGameSessionHostResultCode failureCode);
   void gameSessionConnected();
   void gameSessionReady();
   //void gameSessionConnected();
   //void gameSessionHostStarted();
   void gameSessionDisconnected(BSession::BDisconnectReason reasonCode);
   void gameSessionLaunchAborted();
   void gameSessionGameLaunched();

   //Matchmaking methods 
   BOOL startMatchMaking(BMPSessionMatchMakingData* matchMakingData);
   BOOL abortMatchMaking();
   void allPartyMembersJoinedTarget();
   void resumeMatchmaking();
   BOOL isInMatchmakingMode();
   BOOL isMatchmakingRunning();
   void matchMakingFailed(BLiveMatchMaking::BLiveMatchMakingErrorCode reason);
   uint getMaxMembersPerTeamForCurrentMatchMakingSearch();
   BLiveMatchMaking* getMatchmaker() {return mLiveMatchMaking;};

   void  setInitialPartySettings(const BPartySessionHostSettings& startupHostSettings);

   //Call this to disconnect from any current session and reset the entire system
   void reset();
   //Call this to disconnect from any current game session, but keep matchmaking and the party session alive
   void abortGameSession();
   void abortPartySession();

   //Call this to let the system know you are done loading the level
   void gameDoneLoading();

   bool allPlayersLoaded() const;

   //Use to query available LAN games
   uint                             getLanGameCount();
   BLanGameInfo*                    getLanGame(uint index);
   BDynamicSimArray<BLanGameInfo>*  getLanGamesList();

   bool startPartySessionHost(BPartySessionPlayerSettings* playerSettings);
   bool joinLANPartySession(uint lanGameIndex, BPartySessionPlayerSettings* playerSettings);

   XUID getLocalXUID() {return mLocalXuid;};
   const BSimString& getLocalGamerTag() const { return mLocalGamertag; }
   //This is queried from the game when this system is running, checking this cached value is faster than rechecking the game
   DWORD   getCachedGameChecksum();

   //Calls used by the partySession to control the game to join/leave/etc for matchmaking
   void partyCommand(ClientID client, uint8 commandCode, uint64 nonce);
   void partyCommandJoinTarget(ClientID client, uint8 targetHopper, const XNADDR& targetXNADDR, const XNKEY& targetXNKEY, const XNKID& targetXNKID, uint64 nonce);
   
   //Calls used by ModePartyRoom and the partySession to manage a custom game
   bool startupCustomHost(uint playerCount); //TODO hostoptions, player options
   void abortCustomHost();
   bool joinCustomHost();  //TODO playeroptions

   BLiveVoice* getVoice() const { return mpVoice; }

   // Called to verify that all the players in the game are done loading the map and are ready to go.
   bool getAreAllPlayersLoaded() const;

   //Sends a game invite to everyone on this player's friends list who is ONLINE - DEBUG/TESTING ONLY
   void bulkInvite();
   
   BOOL     hasValidGameSession() const;
   long     getMaxPlayers() const;
   uint     getGameSessionPlayerCount(void);
   uint64   getGameSessionNonce(void);
   BPartySession*          getPartySession() const { return mPartySession; }
   void                    setupPartySession();

   virtual long            getControlledPlayer(void);       //Returns the local player ID
   virtual void            setControlledPlayer(long p);     //Provides a store for this that is persistent outside of the game session

   DWORD                   getGlobalUserCount();   //Returns a cached value from the LSP of how many people are logged in currently

   bool                    isClient(PlayerID id);

   bool                    requestLaunchAbort(long reason);
   void                    sendLaunchReady(bool ready);
   bool                    requestGameLaunch(DWORD countdown); 
   BOOL                    remoteRequestESOConfigFile();

   BMPSimDataObject*       getSimObject() const;
   DWORD                   advanceGameTiming(void);
   void                    setGameStateToFinal(void);
   void                    endActiveGame(void);

   void                    initLanDiscovery();
   void                    deinitLanDiscovery();
   BXLanDiscovery*         getLanDiscovery() const { return mpLanDiscovery; }

   bool                    doNotDismissDialog() const { return mDoNotDismissDialog; }
   void                    belayNextGameDisconnectEvent() {mBelayNextGameDisconnectEvent=true;};
   void                    sendBelayedGameDisconnectEvent();

   //Game settings methods
   bool  setSetting(DWORD index, void *data, long size);
   bool  setInt64  (DWORD index, int64 data);
   bool  setUInt64 (DWORD index, uint64 data);
   bool  setLong   (DWORD index, long  data);
   bool  setDWORD  (DWORD index, DWORD data);
   bool  setFloat  (DWORD index, float data);
   bool  setShort  (DWORD index, short data);
   bool  setWORD   (DWORD index, WORD  data);
   bool  setBool   (DWORD index, bool  data);
   bool  setChar   (DWORD index, char  data);
   bool  setBYTE   (DWORD index, BYTE  data);
   bool  setString (DWORD index, const BSimString& data);

   bool  getInt64 (DWORD index, int64 &data);
   bool  getUInt64(DWORD index, uint64 &data);
   bool  getLong  (DWORD index, long  &data);
   bool  getDWORD (DWORD index, DWORD &data);
   bool  getFloat (DWORD index, float &data);
   bool  getShort (DWORD index, short &data);
   bool  getWORD  (DWORD index, WORD  &data);
   bool  getBool  (DWORD index, bool  &data);
   bool  getChar  (DWORD index, char  &data);
   bool  getBYTE  (DWORD index, BYTE  &data);
   bool  getString(DWORD index, BSimString &data);  


private:
   HRESULT           setControllerID(const uint32 controllerID);
   BOOL              remoteRequestESOLogin();
   void              applyNoLSPConfigSettings();

   liveMPSessionState            mState;                             // 4 Current state of this network session
   BmpSessionStartupMode         mStartupMode;                       // 4 Which mode this system was started up in
   BPartySession*                mPartySession;                      // 4 If Live, matchmaking - then this is the party
   BPartySessionHostSettings     mPartySessionInitialHostSettings;   // 8 Default settings with which to startup any party session
   BLiveMatchMaking*             mLiveMatchMaking;                   // 4 Match making class for Live games
   BMPGameSession*               mGameSession;                       // 4
   WORD                          mPort;                              // 2 Port used to bind to (host) or connect to (join)
   DWORD                         mLocalChecksum;                     // 4 Cached checksum of the local game (since its expensive to generate)   
   DWORD                         mLocalControllerID;                 // 4 Controller of who hosted or joined this session
   XONLINE_NAT_TYPE              mNatType;                           // 4
   GameInterface*                mGameInterface;                     // 4 So that we can interact with the game itself
   mpSessionInterface*           mSessionInterface;                  // 4 Callbacks for events and queries to the mp layer in the game
   BSession*                     mSession;                           // 4
   BMPSimDataObject*             mpSimObject;                        // 4
   BDataSet*                     mGameSettings;                      // 4 Settings for this game 
   DWORD                         mResetTimer;   
   DWORD                         mJoinRequestTimer;                  // 4
   DWORD                         mJoinRequestAttempts;               // 4
   DWORD                         mXNAddrRequestTime;
   //DWORD                         mTimeSinceLastAuthRequest;          // 4 To prevent in/out spamming

   //Matchmaking related member variables
   BOOL                          mMatchMakingRunning;                // 4 Set to TRUE if we are in match making

   BSimString                    mLocalGamertag;                     // 16
   PlayerID                      mLocalPlayerID;                     // 4
   BOOL                          mJoinedViaInvite;                   // 4
   XUID                          mLocalXuid;                         // 8

   BLiveVoice*                   mpVoice;                            // 4

   BXLanDiscovery*               mpLanDiscovery;                     // 4

   bool                          mDoNotDismissDialog : 1;            // 1 (1/8)
   bool                          mResetPartySessionNextUpdate;
   bool                          mBelayNextGameDisconnectEvent;
   UINT64                        mBelayedNonceForGameDisconnectEvent;
};
