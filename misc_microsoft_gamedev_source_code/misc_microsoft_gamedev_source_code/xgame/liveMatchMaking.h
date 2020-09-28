//==============================================================================
// liveMatchMaking.h
//
// Copyright (c) 2008, Ensemble Studios
//==============================================================================

// System which manages Live matchmaking
// Note: This object is designed to be used once then discarded
//   So you create it, and start it - then if it fails, or succeeds you delete it
//   It will not allow you to launch another search from it 

#pragma once

#include "liveSessionSearch.h"
#include "matchMakingHopperList.h"

// xsystem
#include "xmlreader.h"

#include "NetPackets.h"    //For the max member count of a party
#define cBLiveMatchMakingMaxBadTargets 20

//Forward declarations
class BMPSession;
class BMPSessionMatchMakingData;

class BLiveMatchMakingMember
{
   public:
      XUID  mXuid;
      double mMu;
      double mSigma;
      uint64 mGamesPlayed;
      uint64 mRating;
      BLiveMatchMakingMember();
};

class BLiveMatchMaking
{
   public:
      //These are codes reported up to the UI so that it can display what is happening
      enum BLiveMatchMakingStatusCode
      {
         cBLiveMatchMakingStatusCodeNone = 0,
         cBLiveMatchMakingStatusCodeShowSkillInfo,
         cBLiveMatchMakingStatusCodeStartCountdown,
         cBLiveMatchMakingStatusCodeCycleDelay,
         cBLiveMatchMakingStatusCodeFastScan,
         cBLiveMatchMakingStatusCodeNormalScan,
         cBLiveMatchMakingStatusCodeAttemptingJoin,
         cBLiveMatchMakingStatusCodeWaitingForMorePlayers,
         cBLiveMatchMakingStatusCodeSearchStoppedCode
         //cBLiveMatchMakingStatusCodeReseting
      };

      enum BLiveMatchMakingState
      {
         cBLiveMatchMakingStateNone =0,
         cBLiveMatchMakingStateReadyToStart,
         cBLiveMatchMakingStatePreLaunch,
         cBLiveMatchMakingStatePreLaunchQuerysActive,
         cBLiveMatchMakingStatePreLaunchQuerysWaiting,
         cBLiveMatchMakingStateFastSearch,
         cBLiveMatchMakingStatePreHostRandomWait,
         cBLiveMatchMakingStateHosting,
         cBLiveMatchMakingStateDetailedSearch,
         cBLiveMatchMakingStateEndOfCycle,
         cBLiveMatchMakingStateResetToResume,
         cBLiveMatchMakingStateAborting,
         cBLiveMatchMakingStateGamePending,
         cBLiveMatchMakingStateComplete,
         cBLiveMatchMakingStateError,
         cBLiveMatchMakingStateDeleted
      };

      //Codes used to report up perflogs to the LSP
      enum BLiveMatchMakingEventCode
      {  cBLiveMatchMakingEventCodeNone, 
         cBLiveMatchMakingEventCodeStarting,
         cBLiveMatchMakingEventCodeEndOfCycle,
         cBLiveMatchMakingEventCodeGameFound,
         cBLiveMatchMakingEventCodeLastEntry
      };

      enum BLiveMatchMakingErrorCode
      {
         cBLiveMatchMakingErrorNone = cBLiveMatchMakingEventCodeLastEntry,
         cBLiveMatchMakingErrorAbortRequested,
         cBLiveMatchMakingErrorOutOfMemory,
         cBLiveMatchMakingErrorSkillCallFailed1,
         cBLiveMatchMakingErrorSkillCallFailed2,
         cBLiveMatchMakingErrorNoInterface,
         cBLiveMatchMakingErrorMissingSkillData,
         cBLiveMatchMakingErrorWrongNumberOfPartyMembers,
         cBLiveMatchMakingErrorInvalidHopper,
         cBLiveMatchMakingErrorTimedOut,
         cBLiveMatchMakingErrorHostingFailed,
      };

      BLiveMatchMaking(BMPSession* mpSession);
      ~BLiveMatchMaking();

      BOOL start( BMPSessionMatchMakingData* configData );
      BOOL abort();
      BOOL isRunning() {return ((mState>=cBLiveMatchMakingStatePreLaunch) && (mState<cBLiveMatchMakingStateError));};
      void update();
      void resume();
      void allPartyMembersJoinedTarget();
      void matchMadeGameLaunched();
      uint getMaxMembersPerTeamForCurrentSearch();
      void addBadTarget(XNADDR xnaddr);
      uint getBadTargetCount() {return mBadTargetList.getSize();};
      bool isBadTarget(XNADDR xnaddr);
      BOOL isPartyFullyJoinedToCurrentTarget() {return mPartyFullyJoined;};
      BLiveSessionSearch* getCurrentSessionSearch() {return mpLiveSessionSearch;}

   private:

      XUSER_STATS_SPEC           mSkillQueryRequest[1];        // 136 * 1

      //Depricated - we now only post ONCE per search total
      //BPerfLogPacket             mPerfLogDataPacketBase;       // 128 This is a base, filled out perflog post - but without anything specific for a search cycle
      BPerfLogPacketV2           mPerfLogDataPacketCurrent;    // 128 This is the perflog packet to post data to as the cycle processes, it is posted on end of cycle, cancel, or error

      BLiveMatchMakingMember     mMembers[BPerfLogPacketV2::cPerfLogMaxXuids];  // 40 * 3 == 120

      BLiveMatchMakingMember     mMemberSkillAverage;          // 40

      XOVERLAPPED                mSkillQueryOverlapped;        // 28

      XUID                       mSkillQueryXuidList[BPerfLogPacketV2::cPerfLogMaxXuids]; // 8 * 3 == 24

      BLiveMatchMakingState      mState;                       // 4
      BMPSession*                mpMPSession;                  // 4
      BLiveMatchMakingErrorCode  mLastError;                   // 4
      BMPSessionMatchMakingData* mpSearchRequest;              // 4
      BLiveSessionSearch*        mpLiveSessionSearch;          // 4
      BDynamicSimArray<XNADDR>   mBadTargetList; 

      PXUSER_STATS_READ_RESULTS  mpSkillQueryResults;          // 4
      XNADDR                     mLastJoinTarget;
      DWORD                      mSkillQueryResultsSize;       // 4

      BOOL                       mJoinAttemptInProgress;       // 4 The MM system is attempting to join to a particular host
      BOOL                       mPartyFullyJoined;            // 4 Everyone in the party has joined a particular host
      uint                       mStartTime;                   // 4 Tracks how long we have been running this matchmaking process as a whole
      uint                       mCountdownDisplayDigit;       // 4 Used to track the 'starting in X'
      uint                       mStateTimer;                  // 4 Tracks how long we've been in our current state
      uint                       mSearchCount;                 // 4 How many times we've been through the host/search process
      uint                       mPrimaryJoinFailures;         // 4 How many times has this host tried to join a target and failed
      uint                       mClientJoinFailures;          // 4 How many times have party members with this host tried to join a target and failed
      uint8                      mLanguageCode;                // 1
      //uint8                      mVersionCode;                 // 1
      byte                       mSearchResultsPossibleCount;  // 1 For the current search - how many results were returned from Live
      byte                       mSearchResultsValidCount;     // 1 For the current search - how many results have valid QoS responses

      void setState(BLiveMatchMakingState newState);
      BOOL processHopperSearchResults();
      void stopWithError(BLiveMatchMakingErrorCode reason);
};
