//==============================================================================
// liveSystem.h
//
// Copyright (c) Ensemble Studios, 2006-2007
//==============================================================================

#pragma once

#ifndef XBOX
   //Crap out right here, if its not xbox, this cannot be built, and should not be linked
   badInstructionThatWillNotCompile!;
#endif

#include "Common.h"
#include <xonline.h>
#include "mpSession.h"     // To get the definition of PlayerID
#include "XLastGenerated.h"
#include "matchMakingHopperList.h"

// Forward declarations
class BLiveSystem;
class BLiveVoice;
class BMPSession;
class BLiveSessionLeaderboardOnly;

// Externs
extern BLiveSystem* gLiveSystem;

//Constants
const DWORD   cLiveSystemMaxContexts    = 15;
//This is the official, LIVE posted version of the product
//It MUST match the version defined in XEX.XML - it is used internally for version checks across machines
const DWORD   cProductVersion           = 0x10000001;

class leaderBoardReturnDataRow
{
public:
   leaderBoardReturnDataRow() :
   mXuid(0),
   mRank(0),
   mRating(0),
   mIntValue1(0),
   mIntValue2(0)     
   {mGamerTag[0]=0;}

   XUID     mXuid;
   uint32   mRank;
   uint64   mRating;
   char     mGamerTag[XUSER_NAME_SIZE];
   uint64   mIntValue1;
   uint64   mIntValue2;
};

enum BLeaderBoardTypes
{
   cLeaderBoardTypeMPSkill,
   cLeaderBoardTypeCampaignLifetime,
   cLeaderBoardTypeCampaignBestScore,
   cLeaderBoardTypeMPGametypeWinsLifetime,
   cLeaderBoardTypeMPGametypeWinsMonthly,
   cLeaderBoardTypeMPLeaderWinsMonthly,
   cLeaderBoardTypeMPSpecialMonthly,
   cLeaderBoardTypeVsAIMonthly
};


//==============================================================================
class BLiveSystem
{
   public:

      // Describes the current connection to the LIVE servers
      //enum liveConnectionState
      //{
      //   cLiveConnectionStateNotStarted,        // System not started yet
      //   cLiveConnectionStateStartupWaiting,    // I do not have my external IP yet
      //   cLiveConnectionStateReady,             // Ready to use, but not logged into Live
      //   cLiveConnectionStateReadyLoggedIn,     // Ready and logged into Live
      //   cLiveConnectionStateCableUnplugged     // Cable pulled
      //};

      enum BXnAddrState
      {
         cXnAddrStateInit = 0,
         cXnAddrStatePending,
         cXnAddrStateReady,
         cXnAddrStateError
      };

      enum liveSystemTestOptions
      {
         cLiveSystemTestHostOnly,
         cLiveSystemTestAutoEnd,
         cLiveSystemTestAutoStart,
         cLiveSystemTestMaxItems
      };

      enum leaderBoardStatusResult
      {
         cLeaderBoardStatusNoQueryPending,
         cLeaderBoardStatusQueryRunning,
         cLeaderBoardStatusQueryComplete         
      };

      enum leaderBoardState
      {
         cLeaderBoardStateIdle,
         cLeaderBoardStateFriendQuery,
         cLeaderBoardStateStatsQuery,
         cLeaderBoardStateCanceling,
         cLeaderBoardStateComplete
      };    


      BLiveSystem();
      ~BLiveSystem();

      static BOOL             createInstance();    // Returns true if it can create a new instance of the singleton
      static BLiveSystem*     getInstance();       // Gets a pointer to the singleton instance of this class
      static void             destroyInstance();   // Destroys the instance

      void                    startup();
      void                    shutdown();
      void                    update();

      //void                    setLiveRequired(BOOL newState);  //Used to tell the live system that a live login is required
      //BOOL                    isLiveRequired() const { return mStateLiveRequired; }
      //BOOL                    isLoggedIn() const { return (mNumSignedInUsers>0); }
      BOOL                    isMultiplayerLiveGameActive() const;
      BOOL                    isInLanMode() const;
      BOOL                    isMultiplayerGameActive() const;
      BOOL                    isPartySessionActive() const;
      void                    setPartySystemForReenter(bool flagReenter);
      bool                    isPartySystemFlaggedForReenter() { return mPartySystemReenterFlag;};
      //PlayerID                getLocalControlledPlayerID() const { return 1; } //TODO!!!! REPLACE THIS STUB
      BOOL                    getLocalXnAddr(XNADDR& xnAddr);
      //Helper function so that game systems don't need to include all the crap needed to find out this information 
      byte*                   getAITrackingMemoryBlockByPlayerID(PlayerID playerID);
      //BOOL                    getLocalInfo(DWORD controllerID, XUID* pXuid, BSimString& gamerTag);

      BLiveVoice*             getLiveVoice() const;

      BMPSession*             getMPSession() const;

      BLiveVoice*             initVoice();
      void                    disposeVoice();

      BMPSession*             initMPSession();
      void                    disposeMPSession();

      void                    disposeGameSession();

      XINVITE_INFO*           getInviteInfo() {return &mInviteInfo;};
      DWORD                   getInviteAcceptersPort() {return mInviteAcceptersPort;};
      bool                    inviteReceived(DWORD controllerID);
      void                    clearInviteInfo() {Utils::FastMemSet(&mInviteInfo,0,sizeof(XINVITE_INFO));mInviteAcceptersPort=XUSER_MAX_COUNT;};
      BOOL                    isInviteInfoAvailable() {return ((mInviteInfo.xuidInviter==0)?FALSE:TRUE);};

      //Call this to turn on/off this machine responding to QoS responses
      void                    activateQoSResponse( bool enabled, XNKID* xnKid, bool hostMode = false );

      //Call this to indicate that a player is no longer in the game (but still connected to the network - used for defeated/resigned but not end of game)
      void                    playerLeftGameplay(XUID playerXUID);

      //End of game methods for easy access by the game - safe to call even it if is not an MP game
      bool                    hasLeaderboardGameSettings();
      void                    endOfGamePlayerWon(const BSimString& playerName, XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime);
      void                    endOfGameComplete();

      void                    setMPTestOptions( liveSystemTestOptions optionCode, BOOL enable );
      BOOL                    getMPTestOptions( liveSystemTestOptions optionCode );

      void                    testLeaderBoardQuery();
      void                    testLeaderBoardRead(BSimString* resultText);
      uint                    findLeaderBoardIndex(BLeaderBoardTypes lbType, uint gameSize, bool partyTeam, uint difficulty, uint campaignMapIndex, uint gameTypeIndex, uint leaderIndex );

      leaderBoardStatusResult leaderBoardQueryStatus();
      bool                    leaderBoardError() const { return (mLeaderBoardError != ERROR_SUCCESS); }
      void                    leaderBoardCancelQuery(DWORD result=ERROR_SUCCESS);
      bool                    leaderBoardLaunchQuery(uint32 leaderBoardIndex, XUID requestingUser, uint32 startAtRank, leaderBoardReturnDataRow* rowData, uint32* rowDataCount, uint32* numberOfRowsInTable );
      bool                    leaderBoardFriendFilterLaunchQuery(uint32 leaderBoardIndex, XUID requestingUser, leaderBoardReturnDataRow* rowData, uint32* rowDataCount, uint32* numberOfRowsInTable );
      BLiveSessionLeaderboardOnly*  getLeaderboardOnlySession() {return mpLeaderboardOnlySession;}
      void                    createLeaderboardSession(int nOwnerController, XUID owningXUID, uint memberCount, UINT gameModeIndex);

      void                    setPresenceContext( DWORD contextType, DWORD contextValue, BOOL isProperty = false);

      bool                    shuttingDownMPSession() const { return mShuttingDownMPSession; }

      static XUID             getMachineId();

      //Matchmaking hopper stuff
      BMatchMakingHopperList* getHopperList() {return mpHopperList;};
      BMatchMakingHopperList* getHopperListUpdatedVersion() {return mpHopperListUpdatedVersion;};
      void                    postUpdatedVersionToBaseHopperList();

      //Accessor so other external systems don't have to include lspManager just to query this value
      bool                    isInNoLSPMode();

   private:
      
      class contextEntry
      {
      public:
         contextEntry():
            mContextType(0),
            mContextValue(0),
            mInUse(false),
            mProperty(false),
            mDirty(false) {};
            void reset() {mContextType=0;mContextValue=0;mDirty=false;mInUse=false;mProperty=false;};
         DWORD mContextType;
         DWORD mContextValue;
         BOOL  mDirty;
         BOOL  mInUse;
         BOOL  mProperty;
      };

      void loadMatchMakingHoppers();
      void leaderBoardUpdate();
      void fillInLeaderBoardRequestSettings();
      static BOOL CompareLeaderBoardRows( const XUSER_STATS_ROW& a, const XUSER_STATS_ROW& b );

      //BOOL                    mStateLiveRequired;              // Set to true if game is in a state with a Live login is required
      //BOOL                    mInitialNotificationReceived;    // If true, have we received notification on Live connection startup
      //DWORD                   mNumSignedInUsers;               // Number of signed-in users, max is 1

      BOOL                    mTestOptions[cLiveSystemTestMaxItems];

      //liveConnectionState     mConnectionState;

      XNADDR                  mLocalXnAddr;                    // This system's local XNAddr, not available immediately
      XINVITE_INFO            mInviteInfo;                     // If not NULL, then there is an invite pending that other systems need to process
      DWORD                   mInviteAcceptersPort;            // This is the port number of the user that accepted the invite

      BMatchMakingHopperList* mpHopperList;
      BMatchMakingHopperList* mpHopperListUpdatedVersion;

      BMPSession*             mpMPSession;                     
      BLiveVoice*             mpVoice;

      DWORD                   mLastPresenceUpdate;
      DWORD                   mLastPresenceChange;
      DWORD                   mPresenceMode;
      contextEntry            mPresenceContext[cLiveSystemMaxContexts];

      BXnAddrState            mXnAddrState;

      //void                    showSignInScreen();              // Shows the Live dialog for signing in
      //void                    updateSignedInUserCount();       // Goes through and see's how many users are logged in, and on what controller
      void                    updatePresence();

      bool                    mShuttingDownMPSession : 1;      // Set to true if we are waiting to delete mpsession
      //bool                    mDisposeVoice : 1;               // informs the BMPSession shutdown loop that voice is ready for destruction when it's done
      bool                    mXnAddrPending : 1;
      bool                    mPartySystemReenterFlag : 1;

      uint32                  mLeaderBoardError;

      uint32                  mLeaderBoardIndex;
      leaderBoardState        mLeaderBoardState;
      XOVERLAPPED             mLeaderBoardOverlap;
      DWORD                   mLeaderBoardEnumerateResults;
      HANDLE                  mLeaderBoardEnumerator;
      XUID                    mLeaderBoardRequestingUser;
      bool                    mLeaderBoardIndexToUser;
      bool                    mLeaderBoardJustShowFriends;
      uint32                  mLeaderBoardQueryTime;
      leaderBoardReturnDataRow* mpLeaderBoardReturnDataRows;
      uint32*                 mpLeaderBoardReturnDataCount;
      uint32*                 mpLeaderBoardReturnTotalRowsInEntireBoard;
      XUSER_STATS_SPEC        mLeaderBoardCurrentRequest[1];
      PXUSER_STATS_READ_RESULTS mpLeaderBoardStatsResults;   
      XUID*                   mpXUIDRequestList;
      BYTE*                   mpFriendListMemory;
      DWORD                   mFriendCount;
      BLiveSessionLeaderboardOnly* mpLeaderboardOnlySession;
      DWORD                   mLeaderBoardCreationTime;
      XOVERLAPPED             mPresenceOverlap;
      bool                    mPresenceIsOperationPending;
      DWORD                   mPresenceValue;
      uint64                  mPresenceScoreValue;
      DWORD                   mCurrentPresenceIndex;
      DWORD                   mLastPresenceScoreUpdate;

#ifndef BUILD_FINAL
   leaderBoardReturnDataRow*  mpTestingLBData;
   uint32*                    mpTestingLBCount;
   uint32*                    mpTestingLBTotalRows;
#endif
};
