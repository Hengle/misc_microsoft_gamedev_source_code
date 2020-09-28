//==============================================================================
// modepartyroom2.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"
#include "maximumsupportedplayers.h"
#include "ui.h"
#include "uilist.h"
#include "mpSession.h"
#include "ScenarioList.h"
#include "renderThread.h"
#include "binkInterface.h"
#include "UIGlobals.h"
//#include "UIPlayerList.h"
#include "UILeaderPicker.h"
#include "uiMenu.h"
#include "UIButtonBar2.h"

typedef BFixedString<64> BGameSession;

class BDataSet;
class BMPGameDescriptor;
class BUIMPSetupScreen;
class BUIInputHandler;
class BUILeaderPicker;
class BUIMenu;
class BCampaignNode;


//==============================================================================
// BModePartyRoom2
//==============================================================================
class BModePartyRoom2 : 
   public BMode, 
   public BMPSession::mpSessionInterface,
   public BUIGlobals::yornHandlerInterface,
   public BBinkVideoStatus,
   public IInputControlEventHandler,
   public ILeaderPickerEventHandler
{
   public:
      enum
      {
         cFlagModeUnused,
      };

      enum 
      {
         // These are the different types of lobbies
         cPartyRoomModeMM,
         cPartyRoomModeCustom,
         cPartyRoomModeCampaign,

         cPartyRoomModeCount,

         cPartyRoomModeInvalid,
      };

      enum 
      {
         cPRInvalidSlotNumber=-1,
         cPRNumPlayerSlots=6,
         //cPRTotalPlayerSlots=12,       // 6 team, 6 center
         cPRNumPlayerSlotsMatch=3,
         cPRNumPlayerSlotsCampaign=2,
         cPRNumPlayersSlotsPerTeam=3,
      };

      enum
      {
         cYornCallbackContextLeaveParty=0,
         cYornCallbackContextDropPlayers,
         cYornCallbackContextDropPlayersLobby,
         cYornCallbackContextExitMode,
         cYornCallbackContextConditionNormal,
         cYornCallbackContextKickPlayer,
      };

      enum
      {
         cInputIncrement,
         cInputDecrement,
      };

                        BModePartyRoom2(long modeType);
      virtual           ~BModePartyRoom2();

      virtual bool      setup();
      virtual void      shutdown();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);

      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();
      virtual void      update();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      void              onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);
      void              updateVoice();

      // IInputControlEventHandler 
      bool               executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl);
      void               enterContext(const BCHAR_T* contextName);


      // BMPSession::mpSessionInterface implemented in this class
      // - This method is called when the local system sets up the the live system and it is successful.
      //    After this, hostInit() or joinInit() can be called.
      virtual void mpSessionEvent_ESOConnectComplete( BOOL restrictedNATWarning );
      // - This method is called when the local system sets up the the live system and it is NOT successful.
      virtual void mpSessionEvent_ESOConnectFailed( BMPSession::cMPSessionESOConnectCode failureCode );
      // - This method is called as a result of hostInit() or joinInit() if Successful
      virtual void mpSessionEvent_systemReady();
      // - Called if there is a failure to join a game
      virtual void mpSessionEvent_gameSessionJoinFailed( BSession::BJoinReasonCode failureCode );

      // -- Party session events I need to handle
      //virtual void mpSessionEvent_partyEvent_playerJoined(ClientID newClientID, uint newMemberIndex);
      //virtual void mpSessionEvent_partyEvent_playerLeft(ClientID newClientID, uint newMemberIndex);
      virtual void mpSessionEvent_partyEvent_playerJoined(XUID xuid);
      virtual void mpSessionEvent_partyEvent_playerLeft(XUID xuid);
      virtual void mpSessionEvent_partyEvent_customGameStartupComplete( BMPSession::cMPSessionCustomGameStartResultCode resultCode );
      virtual void mpSessionEvent_partyEvent_gameLaunched();
      virtual void mpSessionEvent_partyEvent_memberSettingsChanged(BPartySessionPartyMember* changedMember);
      virtual void mpSessionEvent_partyEvent_hostSettingsChanged(BPartySessionHostSettings* newSettings);
      virtual void mpSessionEvent_partyEvent_partySizeChanged(DWORD newMaxPartyMemberCount);
      virtual void mpSessionEvent_partyEvent_hostSubmitFinalGameSettings();
      virtual void mpSessionEvent_partyEvent_joinFailed( BSession::BJoinReasonCode failureCode );

      virtual void mpSessionEvent_settingsChanged(const BDataSet* settings, DWORD index, BYTE flags);
      virtual void mpSessionEvent_gameConnected(void);
      virtual void mpSessionEvent_gameDisconnected( long reason );
      virtual void mpSessionEvent_partyDisconnected( long reason );      
      //virtual void mpSessionEvent_joinRequestResponse(const BMPGameDescriptor* desc, long result);
      virtual void mpSessionEvent_playerJoined( PlayerID gamePlayer, ClientID clientID, const XUID xuid, const BSimString &gamertag );
      virtual void mpSessionEvent_playerLeft( PlayerID gamePlayer, BOOL local );
      //virtual void mpSessionEvent_playerNotResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect);
      //virtual void mpSessionEvent_playerResponding(PlayerID gamePlayer, DWORD lastResponseTime, bool &disconnect) {};
      //virtual void mpSessionEvent_playerPingUpdate(PlayerID gamePlayer, DWORD ping) {};
      //virtual void mpSessionEvent_allPlayersLoaded(void) {};
      virtual BOOL mpSessionEvent_queryIsInMatchmakingMode(void);
      virtual BOOL mpSessionEvent_preGameInit(BOOL playerIsHost);
      virtual void mpSessionEvent_launchStarted(void);
      virtual void mpSessionEvent_launchTimeUpdate(DWORD time);
      virtual void mpSessionEvent_launchAborted(PlayerID gamePlayer1, PlayerID gamePlayer2, long reason);
      virtual void mpSessionEvent_startGame (void);
      virtual void mpSessionEvent_initialSettingsComplete(void);
      virtual void mpSessionEvent_requestForSetLocalPlayerSettings();
      virtual void mpSessionEvent_LANGameListUpdated(void);
//      virtual void mpSessionEvent_LiveGameListUpdated( BLiveSessionSearch* sessionSearch);
      virtual void mpSessionEvent_ESOSearchFailed( BMPSession::cMPSessionESOSearchCode failureCode );
      virtual void mpSessionEvent_ESOSearchComplete();
      virtual void mpSessionEvent_ESOSearchStatusChanged(const BSimString& description);
      virtual void mpSessionEvent_MatchMakingStatusChanged(BLiveMatchMaking::BLiveMatchMakingStatusCode status, uint data1, uint data2);
      
      // yorn handler
      virtual void yornResult(uint result, DWORD userContext, int port);

      // ----- ILeaderPickerEventHandler
      virtual bool leaderPickerEvent(const BSimString& command);

      void setUseLanMode(bool useLanMode) { mUseLANMode = useLanMode; }
      bool getUseLanMode() { return mUseLANMode; }

      void setInitialMode(long initialMode) { mInitialPartyRoomMode = initialMode; }

      bool isOkToShowLocalPlayerOptionsInUI();
      bool isOkToChangeGameSettingsInUI();
      bool isMatchmakingMode() { return (getHostSettings()->mPartyRoomMode == cPartyRoomModeMM); }
      BPartySessionPartyMember* getPlayerBySlot(int slot);

      long getPartyRoomMode() { return mPartyRoomMode; }


      void editPlayer(BPartySessionPartyMember* pMember);
      bool editLocalPlayer();
      bool decrementLeader();
      bool incrementLeader();
      bool changeTeamLeft();
      bool changeTeamRight();
      bool kickPlayer();
      bool kickPlayer(BPartySessionPartyMember* pMember);
      bool onReady();
      bool onUnready();
      void exitParty();
      void addAIPlayer(uint8 slot);

      const BPartySessionHostSettings* getHostSettings() const;

      void onAcceptLobby(long newPartyRoomMode);
      bool getUseDefaultMap() { return mUseDefaultMap; }
      void setUseDefaultMap(bool value) { mUseDefaultMap=value; }

      void onAcceptGameOptions();

      void setGameOptions(BPartySessionHostSettings& gameOptions) { mGameOptionSettings = gameOptions; }

      bool joinInitLan(uint descriptorIndex);

      bool hostInit();

#ifndef BUILD_FINAL
      void outputPlayerInfo();
#endif

      // Campaign mission helpers
      uint32 getLocalCampaignBits();      // gets the campaign bits for the local user
      uint32 getSessionCampaignBits();    // gets the campaign bits for every party member combined
      bool isScenarioUnlocked(const BCampaignNode* pNode);  // checks to see if the scenario is unlocked for this party.

   protected:
      enum
      {
         // Startup transitional states
         cStateWaitingOnMPSessionToReady,       // - Waiting on the MP session to say it is ready so that I can start things up
         cStateInitializingMPSession,           // (1) - Waiting on systems to startup before I can be in the party state
         cStateInitializingPartySessionHosting, // (2) - Starting up the hosting of a party session
         cStateInitializingPartySessionJoining, // (2) - Joining to an existing party session

         cStateInLANGameList,                   // We are actually in the LAN Game list

         // Steady State
         cStateInSession,                       // (3) - We are in the session

         // Starting up the game session from the party session - for custom games
         cStateGameHostInit,
         cStateGameJoinInit,
         cStateGameHostReady,

         // Launching game transitional states
         cStateLaunchInit,              //User has selected to launch the game
         cStateLaunchWait,              //We are waiting on a launch countdown
         cStateStartGame,               //We have received the signal to start the game - this system is largely done

         // Error/Exit states
         cStateEndOfModeError,          //A dead state where the only path out of here is LEAVING this mode
         cStateExitMode,                //We are shutting down

      };

      const BCampaignNode* getFirstUnlockedMission();
      void checkCampaignScenarios();


      long getPartyRoomView();

      void initializeMemberVariables();
      void tearDownParty();
      void initializeParty();

      void renderVideos();

      uint8 getNextTeam();
      uint8 getOpenSlot(BPartySessionPartyMember* pMember, uint8 desiredTeam);
      void assignTeamAndSlot(BPartySessionPartyMember* pPartyMember);
      void cancelTeamSwitchRequest(BPartySessionPartyMember* pPartyMember);

      // Screen methods - player
      void setPlayerSlotsVisible(int numVisible, bool team);
      void updatePlayerSlots();
      void displayPlayerSlot(BPartySessionPartyMember* pMember);
      void clearPlayerSlot(uint8 slot);
      void refreshPlayerList();


      // map helper methods
      bool isValidMap(uint8 mapIndex, uint8 numPlayers, bool matchPlayersExactly=false);
      bool isValidMapType(long mapType);
      bool getFirstValidMap(uint8 numPlayers, uint8& mapIndex, bool matchPlayersExactly=false);


      // support functions
      void updateTeams();
      bool isVisibleSlot(int slot, BPartySessionHostSettings* pHostSettings);
      bool isValidSlot(uint8 slot, uint8 team);
      bool isOpenSlot(uint8 slot);
      BOOL isLocalPlayerHost() const;
      BOOL isLocalPlayer(BPartySessionPartyMember* pPartyMember) const;
      bool checkForGreenUpTriggers();
      BPartySessionPartyMember* findPlayerToSwitchTeams(BPartySessionPartyMember* pMember);
      void fillInPlayerSettings(BPartySessionPlayerSettings* playerSettings);

      BPartySessionPartyMember* getLocalMember();
      BPartySessionPartyMember* getMemberByXUID(XUID xuid);
      void sendUpdatedMemberSettings(BPartySessionPartyMember* pPartyMember);
      void sendUpdatedLocalMemberSettings(BPartySessionPartyMember* pPartyMember);
      bool isLocalPlayerReady();
      bool isPlayerReady(BPartySessionPartyMember* pPartyMember);

      void updateGamerPic(BPartySessionPartyMember* pPartyMember);

      // helper functions from UI input handler
      void acceptGameOptions();

      void dropPlayersExcessPlayers(uint8 maxInParty);


      void setImage(const char * imagePath);
      void updateAllGameOptions(BPartySessionHostSettings* pGameOptions);

      // All things lobby
      void switchLobby(BPartySessionHostSettings* newSettings);
      void setupCampaignLobby();
      void showCampaignLobbyScreen();
      void showMatchmakingLobbyScreen();
      void showCustomLobbyScreen();

      // player list view
      void toggleReady();
      void setPartyRoomView(int partyRoomView);
      void changeTeam(int direction);

      // player edit view
      void acceptPlayerChanges();
      void cancelPlayerChanges();

      void multiplayerShutdown();
      void setCurrentPartyAsDead();
      bool startGameSync();
      bool startGame();

      bool joinInit();

      void setupLanSettings();

      // Matchmaking status methods
      void setupMatchmakingUI(BPartySessionHostSettings* pHostSettings);
      void updateMatchmakingSlots(bool forceUpdate=false);

      void setVoiceChannels(void);

      void showYornMessageBox(const BUString& message, uint8 dialogButtons, DWORD userContext);
      void showModeExitErrorMessage(const BUString& message);
      void showModeModelEventMessage(const BUString& message);
      void showWaitDialog(const BUString& message);
      void hideWaitDialog();


      // helper methods
      BPartySession* getPartySession() const;
      BPartySessionInputOwnerTypes lookupInputOwnerTypeFromPort(long port);


      // fields
      // -- State management fields
      long mState;
      long mNextState;

      // Party Room - mode specific management fields
      long mPartyRoomViewPrevious; // used to maintain state when going into the player edit view

      long mPartyRoomMode;      // custom, campaign, matchmaking
      long mInitialPartyRoomMode;

      int mVoiceSlots[cPRNumPlayerSlots];

      // Temp Buffer - Custom/GameOptions, MM/GameOptions, Campaign/GameOption
      BPartySessionHostSettings mGameOptionSettings;
      BUString          mTempMessageHolder;
      BCueHandle        mMusicCue;

//======================================================================================//
      XUID  mEditPlayerXuid;

      BFixedString<64> mMessage;

      long mLocalPlayer;                           //The local player's player index, the index into the settings array (1-based, not 0-based)

      long mLaunchCountdown;

      uint mCurrentHopperIndex;

      BSimString mSettingMap;
      long mMatchMakingSettingCiv;
      long mMatchMakingSettingLeader;
      int  mCurrentPlayersInMatchCache;

      // Flash UI fields
      BUIMPSetupScreen* mpPartyRoomScreen;

      BUILeaderPicker*  mpLeaderPicker;

      BUIInputHandler*  mpInputHandler;
      XUID              mKickTargetXUID;    //On the host, holds the XUID a pending kick dialog is talking about

      bool mSettingCoop;
      bool mSettingRecordGame;
      bool mSettingReady:1;
      bool mLaunchStartRequested:1;
      bool mUseDefaultMap:1;
      bool mUseLANMode:1;
      bool mTeamRequestChangePending;
      bool mWaitingOnEveryoneToRejoinParty;    
};

