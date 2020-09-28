//============================================================================
// partySession.h
//
// Copyright (c) Ensemble Studios, 2006-2008
//============================================================================

#pragma once

//==============================================================================
// Includes
#include "mptypes.h"

//Data types/consts
const DWORD cPartySessionMaxUsers              = 6;   
const DWORD cPartySessionSkillQueryTimeout     = 3000;         //Longest time to ever wait for a step in the skill queries to run
const DWORD cPartySessionHostingPort           = 1001;     
const DWORD cBPartySessionConnectTimeout       = 10000;       // Max seconds to fully connect
const int   cBPartySessionInvalidMemberID      = -1;

//Forward declarations
class BSession;
class BChannel;
class BClient;
class BLiveSession;
class BLiveVoice;
class BUser;

//Includes
#include "mppackets.h"
#include "liveGameDescriptor.h"
#include "liveSessionSearch.h"
#include "matchMakingHopperList.h"
#include "liveMatchMaking.h"

// xnetwork
#include "session.h"
#include "xlandiscovery.h"

struct BPartySessionHostSettings
{
   bool     mRandomTeam;         //If Yes - then all teams are randomly determined at game load time
   uint8    mMapIndex;
   uint8    mPartyRoomMode;
   uint8    mHopperIndex;        //Which matchmaking hopper the host says everyone should currently be using
   uint8    mNumPlayers;         // 2, 4, 6
   uint8    mDifficulty;
   uint8    mGameMode;           // game mode victory condition
   uint8    mLiveMode;           // Which BPartySessionLivePartyModes it is in
   uint8    mHostStatusInformation;  //Code from BPartySessionHostStatusInformationCodes below
};

enum BPartySessionMemberConnectionState 
{
   cBPartySessionMemberConnectionStateJoining=0,            //The host has control of their setting currently
   cBPartySessionMemberConnectionStateRequestingInitial,    //They are telling the host they are fully connected and waiting for initial settings,
   //cBPartySessionMemberConnectionStateSlotAssigned,         //They are connected and the slot has just been assigned. Next State => cBPartySessionMemberConnectionStateConnected
   cBPartySessionMemberConnectionStateConnected,            //They are fully connected
   cBPartySessionMemberConnectionStateWantToSwitchTeams,    //They are fully connected and the player wants to switch sides
   cBPartySessionMemberConnectionStateWantToSwitchTeamsLeft,    //They are fully connected and the player wants to switch sides
   cBPartySessionMemberConnectionStateWantToSwitchTeamsRight,    //They are fully connected and the player wants to switch sides
   cBPartySessionMemberConnectionStateCancelSwitchTeams,    //They want to cancel the team switch
   cBPartySessionMemberConnectionStateReadyToStart,         //They are fully connected and the player has "greened-up"

};

enum BPartySessionHostStatusInformationCodes
{
   cBPartySessionHostStatusInformationCodeNone=0,
   cBPartySessionHostStatusInformationCodeInitializingMatchmakingSearch,
   cBPartySessionHostStatusInformationCodeSearching,
   cBPartySessionHostStatusInformationCodePlayerCanceledSearch,
   //cBPartySessionHostStatusInformationCodeErrorCanceledSearch,
   cBPartySessionHostStatusInformationCodeGameLaunchStarting,
   cBPartySessionHostStatusInformationCodeGameLaunchAborted,
   cBPartySessionHostStatusInformationCodeGameRunning,
   cBPartySessionHostStatusInformationCodePostGameResetUI
};

enum BPartySessionPartyMemberTypes
{
   cBPartySessionPartyMemberUnknown = 0,
   cBPartySessionPartyMemberPrimaryPlayer,
   cBPartySessionPartyMemberSecondaryPlayer,
   cBPartySessionPartyMemberAI
};

//These are the different classifications of from whom controller input can come from
enum BPartySessionInputOwnerTypes
{
   cBPartySessionInputOwnerTypePrimary,
   cBPartySessionInputOwnerTypeSecondary,
   cBPartySessionInputOwnerTypeNotParty
};

const WORD  cBPartySessionPlayerStatusFlagRestrictedNAT   = 0x1;
const WORD  cBPartySessionPlayerStatusFlagHost            = 0x2;
const WORD  cBPartySessionPlayerStatusFlagSecondaryPlayer = 0x4;
const WORD  cBPartySessionPlayerStatusFlagAIPlayer        = 0x8;

struct BPlayerRank
{
   void reset()
   {
      mValue = 0;
   }
   union {
      uint16 mValue;
      struct
      {
         uint16 mRank : 4;
         uint16 mLevel : 11;
         uint16 mServerUpdated : 1;
      };
   };
};

struct BPartySessionPlayerSettings
{
   public:

      BPartySessionPlayerSettings();

      void    clearFlags() { mStatusFlags=0; }
      void    setIsSessionHost(bool value) { mStatusFlags = value ? ( mStatusFlags | cBPartySessionPlayerStatusFlagHost ) : ( mStatusFlags & ~cBPartySessionPlayerStatusFlagHost ); }
      bool    isSessionHost() const { return( ( mStatusFlags & cBPartySessionPlayerStatusFlagHost ) ? true : false ); }
      void    setRestrictedNAT(bool value) { mStatusFlags = value ? ( mStatusFlags | cBPartySessionPlayerStatusFlagRestrictedNAT ) : ( mStatusFlags & ~cBPartySessionPlayerStatusFlagRestrictedNAT ); }
      bool    isRestrictedNAT() const { return( ( mStatusFlags & cBPartySessionPlayerStatusFlagRestrictedNAT ) ? true : false ); }
      void    setPartyMemberType(BPartySessionPartyMemberTypes value);
      BPartySessionPartyMemberTypes getPartyMemberType() const;

      int8    mCiv;
      int8    mLeader;
      int8    mTeam;
      int8    mSlot;               // this isn't a game option per se, but is used by the UI to control where the player shows up.
      uint8   mPartyRoomMode;
      uint8   mConnectionState;    //From BPartySessionMemberConnectionState above
      uint8   mStatusFlags;        //Bit code with additional information about this player
      BPlayerRank mPlayerRank;     // player rank, level
      uint32  mCampaignBits;       //Bit array used for sending around each player's campaign progress
};

enum BPartySessionMemberState
{
   cBPartySessionMemberStateIdle = 0,
   cBPartySessionMemberStateJoining,
   cBPartySessionMemberStateJoined
};

class BPartySessionPartyMember
{
   public:

      BPartySessionPartyMember();

      BSimString                    mGamerTag;
      BPartySessionPlayerSettings   mSettings; // 13 bytes
      XUID                          mXuid;
      BClientID                     mClientID;
      DWORD                         mLastTimeUpdated;
      long                          mPort;                     //Which controller port this user is on (ONLY VALID FOR LOCAL MEMBERS)
      BPartySessionMemberState      mState;
      uint8                         mMemberIndex;              // What number is this item in the list, useful if you just have the record pointer, and not the array
      uint8                         mQoSValue;                 // 1 = perfect connection, 100=crappy connection
      bool                          mLiveSessionJoined : 1;
      bool                          mLevelLoaded : 1;          // Set to TRUE after the game has started, and this client has the level completely loaded
      bool                          mRestrictedNAT : 1;        // If true, this player has a restricted NAT 
      bool                          mGamerPicLoaded : 1;       // set to true after we have finished the async load of the player's gamerpic
      bool                          mSlotChanged;              // Set to true whenever the network sees the player's slot has changed, UI reacts to this and clears the flag
};


class BPartySession :
   public   BSession::BClientConnector,
   public   BSession::BSessionEventObserver    
{
public:
   //Enumerations **********************************************************************

   //Reasons why someone was disconnected
   enum BPartySessionDisconnectCode
   {
      cBPartySessionDisconnectNormal,
      cBPartySessionDisconnected,
      cBPartySessionDisconnectFailedConnection,
      cBPartySessionDisconnectCRCMismatch,
      cBPartySessionDisconnectFull,
      cBPartySessionDisconnectDeleted,
      cBPartySessionDisconnectMax
   };



   //Interfaces provided to other classes ********************************************
   class BPartySessionInterface
      //This is the interface that connects the party session to the layer above it (mpSession)
      //This system will activate these callbacks as the events arise
   {
   public:
      virtual void BPartySessionEvent_systemReady() {};
      virtual void BPartySessionEvent_systemDisconnected() {};
      virtual void BPartySessionEvent_systemStartupFailed(BSession::BJoinReasonCode failureCode) {};
      virtual void BPartySessionEvent_memberSettingsChanged(BPartySessionPartyMember* changedMember) {};
      virtual void BPartySessionEvent_hostSettingsChanged(BPartySessionHostSettings* pNewSettings) {};
      virtual void BPartySessionEvent_partySizeChanged(DWORD newMaxPartyMemberCount) {newMaxPartyMemberCount;};
      //virtual void BPartySessionEvent_playerJoined(ClientID newClientID, uint newMemberIndex) {};
      //virtual void BPartySessionEvent_playerLeft(ClientID newClientID, uint newMemberIndex) {};
      virtual void BPartySessionEvent_playerJoined(XUID xuid) {};
      virtual void BPartySessionEvent_playerLeft(XUID xuid) {};
      virtual void BPartySessionEvent_matchMakingStatusChanged(uint8 status, uint data1, uint data2) {};
   };



   // Interfaces from other classes that this class implements **************************************

   //BSession::BClientConnector interface that we implement
   //virtual bool connectionAttempt( BClient* requestingClient );
   //Callback going up when we have a client requesting to connect to the host of the session.
   //  If approved, then they are assigned a client ID, and allowed to try and fully connect to the session
   virtual bool sessionConnectionRequest(const BSessionUser users[], BSession::BJoinReasonCode* reasonCode);

   //BSession::BSessionEventObserver interface that we implement
   virtual void processSessionEvent(const BSessionEvent* pEvent);

   // **************************************************************************************
   PlayerID getPlayerID(long fromClientID);

   
   // Startup modes
   enum BPartySessionStartupMode
   {
      cBPartySessionStartupNone,
      cBPartySessionStartupHosting,
      cBPartySessionStartupJoining
   };

   // valid mp session states
   enum BPartySessionState
   {
      cBPartySessionStateNone,
      cBPartySessionStateError,

      //Startup states
      cBPartySessionStateStartingWaitingForXNAddr,
      cBPartySessionStateStartingWaitingForLoggin,
      cBPartySessionStateIdle,

      //Hosting states
      cBPartySessionStateLaunchHostWaitingForSessionCreation,
      cBPartySessionStateReady,

      //Client states
      cBPartySessionStateJoiningWaitingForLiveSession,
      cBPartySessionStateJoiningWaitingForConnection,
      cBPartySessionStateJoiningWaitingForInitialSetup,
      cBPartySessionStateJoiningReady,

      cBPartySessionStateShuttingDown,          //When done reseting, next state is cBPartySessionStateFinished
      cBPartySessionStateShuttingDownToReset,   //When done reseting, next state is cBPartySessionStateStartingWaitingForXNAddr
      cBPartySessionStateFinished,
      cBPartySessionStateDeleting
   };

   enum BPartySessionCommandCodes
   {
      cBPartySessionCommandJoinTarget = 0,
      cBPartySessionCommandLeaveTarget,
      cBPartySessionCommandJoinSuccess,
      cBPartySessionCommandJoinFailure,
      cBPartySessionCommandDisconnected,
      cBPartySessionCommandGreenUp,
      cBPartySessionCommandMatchmakingStatusInfo,
      cBPartySessionCommandCount
   };


 
   //Constructor/destructor
   BPartySession(BLiveVoice* pVoice, const BPartySessionPlayerSettings& defaultMemberSettings);
   ~BPartySession();

   //Starts up the party session, getting it ready to start joining or hosting
   void startUpHost(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, const BPartySessionHostSettings& initialHostSettings, DWORD controllerID);
   void startUpJoin(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, DWORD controllerID, XINVITE_INFO* inviteInfo);
   void startUpJoin(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, DWORD controllerID, const BLanGameInfo& lanInfo);

   //Shut down any running session and clean up
   void shutdown();
   //Is ready to be deleted
   bool isShutdown() {return mState==cBPartySessionStateFinished;};
   //Per-frame update for this system
   void update();
   //Call this to disconnect from any current session and reset the system
   void reset();
   //Returns true if system is running and ready for use
   BOOL isRunning() const;
   //Returns true if I am hosting
   BOOL isHosting() const;
   //Get the BSession pointer
   BSession* getSession() const { return(mpSession); } 
   BLiveSession* getLiveSession() const {return(mpLiveSession);}
   //Call this to change your settings to new values - it if returns false, then no changes were needed and it was not submitted
   bool changeSettings(BPartySessionPlayerSettings& playerSettings);
   bool changeSettings(XUID xuid, BPartySessionPlayerSettings& playerSettings);
   bool changeAIPlayerSettings(XUID aiXuid, const BPartySessionPlayerSettings& playerSettings);
   //The host calls this to broadcast out to everyone a particular member's settings
   bool broadcastChangeSettings(const BPartySessionPlayerSettings& playerSettings, XUID changedXUID);
   //Host calls this to change the settings for the host values
   void changeHostSettings(const BPartySessionHostSettings& newSettings);
   const BPartySessionHostSettings& getCurrentHostSettings() const { return mCurrentHostSettings; }
   //Returns the number of connected people in the party
   uint getPartyCount();
   //Returns the maximum size of the party currently
   uint getPartyMaxSize() {return mPublicSlots;};
   // check to see if more people can join the party.
   bool isPartyFull() { return getPartyCount() >= getPartyMaxSize(); };
   //Resets everyone's join state to idle - useful after you dump the host
   void partyResetJoinState();
   //Has everyone returned to an idle state after a game launch
   bool isPartyAtIdleState();
   //Returns the party memeber's data for a particular member index (used to iterate through the valid, in-use entries)
   BPartySessionPartyMember*    getPartyMember(uint index);
   //Returns the player settings for a particular member number (used to iterate through the valid, in-use entries)
   BPartySessionPlayerSettings* getPlayerSettings(uint index);
   //For a client, if they think they are currently matchmaking (via a msg from the party host) then this is valid
   bool thisClientThinksItIsMatchmaking();

   BOOL addNonNetworkedPlayer(BPartySessionPartyMemberTypes memberType, const BSimString& gamerTag, const BPartySessionPlayerSettings& settings);

   //Drops a dude from the party
   void dropPartyMemberByClientID(BClientID id, bool notifyEveryone=true);
   //Drop a member via his index
   void handleClientDisconnect(const BSessionEvent& event);
   void dropPartyMemberByIndex(uint index, bool notifyEveryone=true);
   void dropPartyMemberByIndex(uint index, ClientID clientId, XUID xuid, BOOL isHost, BOOL isLocal, bool notifyEveryone, bool noMemberRecord=false);
   void kickAllAIMembers();
   //Find a member by his client ID
   BPartySessionPartyMember* findPartyMemberByClientID(BClientID id);
   //Given the pointer to a primary member, find any secondary member on that same connection
   BPartySessionPartyMember* findSecondaryMemberFromThisPrimaryMember(BPartySessionPartyMember* primaryMember);
   //Find a member by his XUID
   BPartySessionPartyMember* findPartyMemberByXUID(XUID xuid);
   //Find a member by his controller port
   BPartySessionPartyMember* findPartyMemberByPort(long port);
   //Return all the party member's XUIDs
   void getPartyXUIDs(XUID *xuidArray);
   //Locks the party from anyone new joining
   void lockPartyMembers(bool lock);
   //Lets you change the max number of people who can be in the party
   void changeMaxMemberCount(DWORD newMaxMemberCount);
   //void updateTalkerList(long toClientID=-1, BOOL force=FALSE);
   void clearReactivatePartyLogicFlag() {mReactivatePartyLogic=false;};

   //Commands sent from mpSession to control matchmaking joins/leaves/etc
   //It is by will alone that I set my mind in motion
   void partySendJoinTargetCommand(uint8 targetHopper, const XNADDR& targetXNADDR, const XNKEY& targetXNKEY, const XNKID& targetXNKID, uint64 nonce);
   void partySendLeaveTargetCommand(uint64 nonce);
   void partySendJoinSuccessCommand(uint64 nonce);
   void partySendJoinFailureCommand(uint64 nonce);
   void partySendDisconnectedFromTargetCommand(uint64 nonce);
   void partySendGreenUpCommand(uint64 nonce);
   void partySendMatchmakingStatusInfo(uint8 status, uint data1, uint data2);

   void updateBroadcastedHostData();

protected:

   //Call this to start up hosting a party session
   void hostPartyStartup();
   //Call this to join a party session
   void joinPartyStartup();

   void joinStartupProcessing();
   void hostStartupProcessing(void);

   void newMemberJoined(ClientID newClientID);

   void handleClientData(const BSessionEvent* pEvent);
   void handlePartyMatchMakingCommand(ClientID client, uint8 commandCode, uint64 nonce);

   HRESULT           setControllerID(const uint32 controllerID);
   void              setState(BPartySessionState newState);
   
   XUID              getFakeXUID();
   void              setQoSNotification(BOOL enabled);

   BPartySessionState            mState;                          // 4 Current state of this network session
   BPartySessionStartupMode      mStartupMode;                    // 4 Which mode this system was started up in
   BLiveSession*                 mpLiveSession;                   // 4 If Live, then this is the live session object
   BLanGameInfo                  mLanInfo;                        // 144
   XINVITE_INFO                  mHostTarget;                     // 84
   WORD                          mPort;                           // 2 Port used to bind to (host) or connect to (join)
   DWORD                         mLocalChecksum;                  // 4 Cached checksum of the local game (since its expensive to generate)
   uint                          mGameTypeIndex;                  // 4
   DWORD                         mLocalControllerID;              // 4 Controller of who hosted or joined this session
   UINT                          mPublicSlots;                    // 4 If hosted - define what slots are available
   XNKEY                         mLocalXNKey;                     // 16 Security key
   XNKID                         mLocalXNKID;                     // 8 Session key
   BPartySessionInterface*       mSessionInterface;               // 4 So that we can interact with our owner
   BSession*                     mpSession;                       // 4
   BLiveGameDescriptor           mLiveJoinGameTarget;             // 384 Descriptor of the live game I am trying to join
   DWORD                         mJoinRequestTimer;               // 4
   DWORD                         mJoinRequestAttempts;            // 4

   uint                                   mQoSResponseDataSize;   // 4
   BLiveSessionSearch::BQoSResponseData   mQoSResponseData;       // 197
   BOOL                                   mQoSResponding;         // 4

   BSimString                    mLocalGamertag;                  // 8
   BPartySessionHostSettings     mCurrentHostSettings;            // 8
   BPartySessionPlayerSettings   mDefaultPlayerSetting;           // 8
   BPartySessionPlayerSettings   mLocalMemberInitialSettings;     //Settings they want when they first startup/join the party
   BPartySessionPartyMember      mMembers[cPartySessionMaxUsers]; // 48 * 6 == 288
   int                           mLocalMemberIndex;               // 4 for this local player, this is the index for their entry in mMembers
   BOOL                          mAbort;                          // 4 A fatal error occurred and we need to stop the multiplayer session and cleanup

   DWORD                         mLockedPlayers;                  // 4
   DWORD                         mLaunchCountdown;                // 4
   DWORD                         mLaunchRequestTime;              // 4
   DWORD                         mLaunchLastUpdate;               // 4
   BLiveMatchMaking::BLiveMatchMakingStatusCode    mCurrentMatchMakingStatusCode;

   XUID                          mLocalXuid;                      // 8

   BLiveVoice*                   mpVoice;                         // 4

   bool                          mUpdateLocalSettings : 1;      // 1 (1/8)
   bool                          mSettingsComplete : 1;         //   (2/8) When true, we have received the initial game settings from the host
   bool                          mSessionConnected : 1;         //   (3/8)
   bool                          mGameConnected : 1;            //   (4/8)
   bool                          mGameEnded : 1;                //   (5/8)
   bool                          mPartyLocked : 1;              //   (6/8)
   bool                          mLanSecurityKeyRegistered : 1; //   (7/8)
   bool                          mReactivatePartyLogic;
};
