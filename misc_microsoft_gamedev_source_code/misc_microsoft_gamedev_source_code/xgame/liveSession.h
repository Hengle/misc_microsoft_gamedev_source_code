//==============================================================================
// liveSession.h
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================
#pragma once

#include "asynctaskmanager.h"


const DWORD LIVESESSION_PUBLICSLOTS    =  6;     // Default number of public slots
const DWORD LIVESESSION_PRIVATESLOTS   =	6;     // Default number of private slots
const DWORD LIVESESSION_MAXLOCALUSERS  =  1;
const DWORD LIVESESSION_USERSLOTS      =  LIVESESSION_PUBLICSLOTS * 2;     //Need a little extra space for drop pending users
#ifdef _DEBUG
const DWORD LIVESESSION_COMMANDTIMEOUT =  15000;  // How long to wait before giving up on a command finishing
#else
const DWORD LIVESESSION_COMMANDTIMEOUT =  5000;   // How long to wait before giving up on a command finishing
#endif
const DWORD LIVESESSION_STATUSHOLDDELAY =  500;   // How long to keep a user record around that says a particular XUID dropped
class BLiveSession;

enum BLiveSessionUserStatus
{
   cLiveSessionUserStatusUserUnknown,
   cLiveSessionUserStatusRecordUnused,
   cLiveSessionUserStatusAddPending,
   cLiveSessionUserStatusAddFailed,
   cLiveSessionUserStatusInSession,
   cLiveSessionUserStatusDropPending,
   cLiveSessionUserStatusDropCompleted,
   cLiveSessionUserStatusDropFailed
};

class BLiveSessionUserEntry
{
   public:
      BLiveSessionUserEntry();

      XOVERLAPPED             mOverlapped;         // 28
      XUID                    xuid;                // 8
      BLiveSessionUserStatus  status;              // 4
      DWORD                   commandIssuedTime;   // 4
      BOOL                    privateSlots;        // 4
};

// Different types of games this handles
enum BLiveSessionGameClassification
{
   cLiveSessionGameClassificationUnknown,
   cLiveSessionGameClassificationCustom,
   cLiveSessionGameClassificationCampaign,
   cLiveSessionGameClassificationMatchmade
};

enum BLiveSessionHostingModes
{
   cBLiveSessionHostingModeOpen = 1,
   cBLiveSessionHostingModeFriendsOnly,
   cBLiveSessionHostingModeInviteOnly
};

// slot types for the session
enum BLiveSessionSlots
{
   cSessionSlotsTotalPublic = 0,
   cSessionSlotsTotalPrivate,
   cSessionSlotsFilledPublic,
   cSessionSlotsFilledPrivate,
   cSessionSlotsMax
};

//The various ASYNC tasks that this system uses
//==============================================================================
// BLiveSessionAsyncTask - Base class
//==============================================================================
class BLiveSessionAsyncTask : public BAsyncTask
{
public:
   BLiveSessionAsyncTask() :
      mOwningLiveSession(NULL),
      mStarted(false) { mCheckTimeout=true;mTimeoutLength=10000;};
   ~BLiveSessionAsyncTask();
   //  Call this first in any child classes implementations 
   virtual bool doAsyncFunction() {mStarted = true;mStartTime=timeGetTime();return true;};
   virtual void onSuccess() {};
   virtual void onFailure() {};
   BLiveSession* mOwningLiveSession;
   bool   mStarted;
};

//==============================================================================
// BLiveSessionTaskSessionJoinLocal - Join the local session
//==============================================================================
class BLiveSessionTaskSessionJoinLocal : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionJoinLocal(const DWORD userCount, const DWORD *userIndexes, const BOOL *privateSlots);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD    mUserCount;
   DWORD    mUserIndices [LIVESESSION_MAXLOCALUSERS];
   BOOL     mPrivateFlags[LIVESESSION_MAXLOCALUSERS];  
};

//==============================================================================
// BLiveSessionTaskSessionLeaveLocal - Drop the local user from the local session
//==============================================================================
class BLiveSessionTaskSessionLeaveLocal : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionLeaveLocal(const DWORD userCount, const DWORD *userIndexes);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD    mUserCount;
   DWORD    mUserIndices [LIVESESSION_MAXLOCALUSERS];
};

//==============================================================================
// BLiveSessionTaskSessionJoinRemote - Join a remote user to the local session
//==============================================================================
class BLiveSessionTaskSessionJoinRemote : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionJoinRemote(const XUID xuid, const BOOL isPrivate);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   XUID mXuid;
   BOOL mIsPrivate;
};

//==============================================================================
// BLiveSessionTaskSessionLeaveRemote - Drop a remote user from the local session
//==============================================================================
class BLiveSessionTaskSessionLeaveRemote : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionLeaveRemote(const XUID xuid);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   XUID mXuid;
};

//==============================================================================
// BLiveSessionTaskSessionEnd - End the session
//==============================================================================
class BLiveSessionTaskSessionEnd : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionEnd();
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
};

//==============================================================================
// BLiveSessionTaskSessionDelete - Delete the session
//==============================================================================
class BLiveSessionTaskSessionDelete : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionDelete();
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
};

//==============================================================================
// BLiveSessionTaskSessionStart - Starts the session
//==============================================================================
class BLiveSessionTaskSessionStart : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionStart();
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
};

//==============================================================================
// BLiveSessionTaskSessionModify - Modify a running session
//==============================================================================
class BLiveSessionTaskSessionModify : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionModify(const DWORD flags, const DWORD maxPublicSlots, const DWORD maxPrivateSlots);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD    mFlags;
   DWORD    mMaxPublicSlots;
   DWORD    mMaxPrivateSlots; 
};

//==============================================================================
// BLiveSessionTaskSessionArbitrationRegister - Register for arbitration
//==============================================================================
class BLiveSessionTaskSessionArbitrationRegister : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionArbitrationRegister(const ULONGLONG sessionNonce, const DWORD results, XSESSION_REGISTRATION_RESULTS* pArbitrationResults );
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   ULONGLONG   mSessionNonce;
   DWORD       mResults;
   XSESSION_REGISTRATION_RESULTS* mpArbitrationResults;
};

//==============================================================================
// BLiveSessionTaskSessionWriteStats - Writes out post-game stats for a player
//==============================================================================
class BLiveSessionTaskSessionWriteStats : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionWriteStats(const XUID xuid );
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   XUID mXuid;
   uint mViewCount;
   XSESSION_VIEW_PROPERTIES mViews[6];
   XUSER_PROPERTY mPropertySkill[2];
   XUSER_PROPERTY mPropertyWins[3];
   XUSER_PROPERTY mPropertyScore[2];
   XUSER_PROPERTY mPropertyScoreByMap[2];
};

//==============================================================================
// BLiveSessionTaskSessionSetContext - Sets context values before the session is created
//==============================================================================
class BLiveSessionTaskSessionSetContext : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionSetContext(const DWORD controllerID, const DWORD contextID, const DWORD contextValue);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD mControllerID;
   DWORD mContextID;
   DWORD mContextValue;
};

//==============================================================================
// BLiveSessionTaskSessionCreate - Creates the session
//==============================================================================
class BLiveSessionTaskSessionCreate : public BLiveSessionAsyncTask
{
public:
   BLiveSessionTaskSessionCreate(const DWORD flags, const DWORD userIndex, const DWORD maxPublicSlots, const DWORD maxPrivateSlots);
   BLiveSessionTaskSessionCreate(const DWORD flags, const DWORD userIndex, const DWORD maxPublicSlots, const DWORD maxPrivateSlots, ULONGLONG nonce, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD mFlags;
   DWORD mUserIndex;
   DWORD mMaxPublicSlots;
   DWORD mMaxPrivateSlots;
   ULONGLONG mSessionNonce;
   XSESSION_INFO mSessionInfo;
   HANDLE mSessionHandle;
   UINT mSlots[cSessionSlotsMax];
};

//==============================================================================
// BLiveSessionTaskSessionModifySkill - Tells Live to update the trueskill values for the session to the average of these XUIDs
//==============================================================================
class BLiveSessionTaskSessionModifySkill : public BLiveSessionAsyncTask
{
public:
   static const DWORD cBLiveSessionTaskSessionModifySkillMaxCount = 3;
   BLiveSessionTaskSessionModifySkill(const DWORD xuidCount, const XUID* pXuidArray);
   virtual bool doAsyncFunction();
   virtual void onSuccess();
   virtual void onFailure();
public:
   DWORD mXuidCount;
   XUID  mXuidArray[cBLiveSessionTaskSessionModifySkillMaxCount];
};





//==============================================================================
// BLiveSession
//==============================================================================
class BLiveSession : BAsyncNotify
{
   public:
      //Enums
      // valid session states
      enum liveSessionState
      {
         cSessionStateNone,
         cSessionStateError,
         cSessionStateCreating,
         cSessionStateIdle,
         cSessionStateWaitingForRegistration,
         cSessionStateRegistering,
         cSessionStateRegistered,
         cSessionStateJoiningLocal,
         cSessionStateSessionRunning,
         cSessionStateArbitrationRegistrationPending,
         cSessionStateArbitrationRegistrationComplete,
         cSessionStateStartPending,
         cSessionStateInGame,
         cSessionStateEndPending,
         cSessionStateEnd,
         cSessionStateSessionDeletePending,
         cSessionStateSessionDeleted,
         //cSessionStateFinished,
         cSessionStateDeleteing
      };

      enum listSessionNotify
      {
         cSessionNotifyCreated,
         cSessionNotifyRegistered,
         cSessionNotifyFailedToRegister,
         cSessionNotifyStarted,
         cSessionNotifyFailedToStart,
         cSessionNotifyEnded,
         cSessionNotifyFailedToEnd,
         cSessionNotifyDeleted
      };

      // Async Task Notifications
      void notify(DWORD eventID, void * task);

      //Constructor for starting a party session
      BLiveSession(int nOwnerController, UINT slots);
      //Constructor for joining a party session
      BLiveSession(int nOwnerController, UINT slots, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY);
      //Constructor for a leaderboard only session
      BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex);     
      //Constructor for starting a Live matchmaking session
      BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex, BOOL ranked);
      //Constructor for joining a Live matchmade session
      BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex, BOOL ranked, uint64 nonce, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY);

      ~BLiveSession();

      void deinit();
      bool isShutdown();

      void update();
      bool isSessionValid() { return mSessionState == cSessionStateSessionRunning; };
      bool sessionHasError() { return mSessionState == cSessionStateError;};
      bool sessionIsRegistered() {return mSessionState == cSessionStateArbitrationRegistrationComplete;};
      bool sessionIsInGame() { return mSessionState == cSessionStateInGame;};
      bool sessionIsGameOver() {return mSessionState == cSessionStateEnd;};

      //Needed by some asynctasks when they complete
      void setState(liveSessionState newState) {mSessionState=newState;};   
      void setUserState(XUID xuid, BLiveSessionUserStatus newState);
      void localUserJoined() {mLocalUserJoined=true;};
      void arbitrationCompleted(bool succesful);
      void sessionCreated(ULONGLONG nonce, HANDLE sessionHandle, PXSESSION_INFO pSessionInfo);

      void modify(bool joinable, uint totalSlots);
      void changeLiveMode(BLiveSessionHostingModes newMode);
      void modifySkill(DWORD count, XUID* xuidArray);

      bool getSessionHostXNAddr(XNADDR& hostXNAddr);  
      bool getXNKID(XNKID& xnkid);
      bool getXNKEY(XNKEY& xnkey);
      bool isMatchmadeGame();

      void startGame();
      void endGame();
      void startEndPartySession();
      bool registerForArbitration();
      bool hostOnlySecondaryRegisterForArbitration();
      
      bool addRemoteUserToSession(XUID userXIUD, bool privateSlot = false);
      void dropRemoteUserFromSession(XUID userXUID);
      BLiveSessionUserStatus getUserStatus(XUID userXUID);
      XUID getNextUserAddFailure(uint startingIndex);
      HANDLE getSessionHandle() {return mSession;};
      uint64 getNonce() const { return mSessionNonce; }

      void setPlayerWon(XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime);
      void storeStatsGameSettings( BLiveSessionGameClassification gameClass, bool partyTeam, uint campaignMapIndex, uint gameTypeIndex, uint difficulty, bool statsSaveGameRule=false );

   protected:

      //List of remote users registered with this session
      BLiveSessionUserEntry mRemoteUsers[LIVESESSION_USERSLOTS]; // 56 * 6 * 2 == 672

      XNADDR             mHostXNAddr;                 // 36
      XNKEY              mSessionKey;                 // 16
      XNKID              mSessionKID;                 // 8

      ULONGLONG          mSessionNonce;               // 8 Nonce of the session

      HANDLE             mSession;                    //Session handle
      XSESSION_REGISTRATION_RESULTS*  mpArbitrationResults;
      DWORD              mSessionFlags;               //Session creation flags
      DWORD              mOwnerController;            //Which controller created the session
      DWORD              mCommandIssuedTime;          //Set whenever a session-wide API called is fired off
      uint               mGameModeIndex;
      BOOL               mRanked;
      BOOL               mIsHost;                     //Is hosting
      DWORD              mArbitrationRegisteredCount; //Number of times arbitration has been registered
      liveSessionState   mSessionState;
      BLiveSessionHostingModes mHostingMode;       
      uint               mMaxPublicSlots;
      uint               mMaxPrivateSlots;
      BLiveSessionGameClassification mStatsGameClass;
      uint               mStatsCampaignMapIndex;
      uint               mStatsGameTypeIndex;
      uint               mStatsDifficulty;   
      uint32             mShuttingDownStartTime;
      BDynamicSimArray<BLiveSessionAsyncTask*>  mAsyncTaskList;
      bool               mStopAcceptingAsyncTasks;

      bool               mJoinable : 1;                        // 1 (1/8)
      bool               mResendSessionModifyOnNextDrop : 1;   //   (2/8)
      bool               mDeinit : 1;                          //   (3/8)
      bool               mSessionEnded : 1;                    //   (4/8)
      bool               mShutdownDialog : 1;                  //   (5/8)
      bool               mStatsPartyTeam;
      bool               mStatsOnlySession;  
      bool               mStatsSaveGameRule;             //If true, then use the 'rule' for save games, ie: don't write out to cumulative score
      bool               mLocalUserJoined;

      void generateUserStatusSpam();
      void addAsyncTaskToQueue(BLiveSessionAsyncTask* newTask);
      void updateAsyncTaskQueue();
};

//Class used to manage leaderboard only sessions
//  It creates the special board, bound to just one local controller, joins then, starts it, 
//  Takes the user data, posts, ends, and closes itself

class BLiveSessionLeaderboardOnly
{
public:
   enum BLiveSessionLeaderboardOnlyState
   {
      cLiveSessionLeaderboardOnlyStateError,
      cLiveSessionLeaderboardOnlyStateNoSession,
      cLiveSessionLeaderboardOnlyStateCreatingSession,
      cLiveSessionLeaderboardOnlyStateStarting,
      cLiveSessionLeaderboardOnlyStateIdle,      
      cLiveSessionLeaderboardOnlyStateEndPending,
      cLiveSessionLeaderboardOnlyStateDeleteing,
      cLiveSessionLeaderboardOnlyStateComplete
   };

   BLiveSessionLeaderboardOnly(int nOwnerController, XUID owningXUID, uint memberCount, UINT gameModeIndex);
   ~BLiveSessionLeaderboardOnly();
   BLiveSession*  getLiveSession() {return mpLiveSession;}
   void           update();
   void           shutDown();
   bool           canBeDeleted();
   void storeStatsGameSettings( BLiveSessionGameClassification gameClass, bool partyTeam, uint campaignMapIndex, uint gameTypeIndex, uint difficulty, bool statsSaveGameRule=false );
   void setPlayerWon(XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime);

private:
   BLiveSession*                    mpLiveSession;
   BLiveSessionLeaderboardOnlyState mState;
   long                             mPostersControllerID;
   XUID                             mPostersXUID;
   uint                             mPlayerCount;
   uint                             mGameModeIndex;
   uint                             mPlayersReportingEndOfGame;
   XUID                             mData_playerXUID;
   BOOL                             mData_playerWonGame;
   long                             mData_teamID;
   uint                             mData_leaderIndex;
   uint                             mData_score;
   uint                             mData_gamePlayTime;
};