//==============================================================================
// TimeSync.h
//
// Copyright (c) Ensemble Studios, 2000-2008
//==============================================================================

#pragma once

//==============================================================================
// Includes

// xcore
#include "estypes.h"

// xnetwork
#include "NetPackets.h"
#include "session.h"
#include "Serializable.h"

//==============================================================================
// Forward declarations

class BClient;
class BChannel;

//==============================================================================
// Used by BTimingLog
//==============================================================================
class BTimingLogEntry
{
   public:

      enum LogType
      {
         cTiming = 0,
         cAdvanceTime,
         cMaxTypes
      };

      BTimingLogEntry();

      void init(LogType type, uint clientID, uint sendTime, uint timing);
      void init(LogType type, uint recvTime, uint recvUpdateInterval, uint earliestRecvTime, uint earliestClientID, uint gameTime);

      LogType getType() const { return mType; }

      uint getClientID() const { return mClientID; }

      uint getSendTime() const { return mSendTime; }
      uint getTiming() const { return mTiming; }

      uint getRecvTime() const { return mRecvTime; }
      uint getRecvUpdateInterval() const { return mRecvUpdateInterval; }

      uint getEarliestAllowedRecvTime() const { return mEarliestAllowedRecvTime; }
      uint getEarliestClientTimeIndex() const { return mEarliestClientTimeIndex; }

      uint getGameTime() const { return mGameTime; }

   private:

      LogType mType;

      uint mClientID;

      // cTiming
      uint mSendTime;   // this is the value from generateSendTime
      uint mTiming;     // this should be the local client's update average

      // cAdvanceTime
      uint mRecvTime;                  // the current recv time
      uint mRecvUpdateInterval;        // the latest recv update interval

      uint mEarliestAllowedRecvTime;   // the earliest allowed recv time
      uint mEarliestClientTimeIndex;   // the client used to determine the recv time

      uint mGameTime;
};

//==============================================================================
// Used to aid in finding OOS' due to timing issues
// We'll keep the last X values stored here and when we go OOS, this will
//    get dumped to the end of the synclog
//==============================================================================
class BTimingLog
{
   public:
      BTimingLog();

      enum
      {
         cMaxEntries = 60
      };

      uint getStartingIndex() const;
      const BTimingLogEntry& getEntry(uint index) const;

      uint getGameTime() const;

      void add(BTimingLogEntry::LogType type, uint clientID, uint sendTime, uint timing);
      void add(BTimingLogEntry::LogType type, uint recvTime, uint recvUpdateInterval, uint earliestRecvTime, uint earliestClientID, uint interval);

   private:
      BTimingLogEntry mEntry[cMaxEntries];
      uint mInsertIndex;
      uint mGameTime;
};

//==============================================================================
//========= Class description: // FIXME: Out of date docs - revise
//
// Time is controlled via two variables, the send time and the receive time. The send time is the
// timemarker at which messages are placed onto the data stream, recv time is the point at 
// which they are pulled off. Therefore send time wants to be a value such that all clients will be able
// to receive a given send time marker by the time their recv time hits that mark. Therefore, send
// time wants to be at least the latency of the connection to all clients, so that they can receive it
// in time. More likely, it wants to be the latency, plus the average deviation of the latency, and
// limited by some minimum update frequency (such that the send time markers don't eat up all
// of the bandwidth.)
// 
//==============================================================================

//==============================================================================
class BTimingRecord : public BSerializable
{
   public:
      BTimingRecord() : mSendTime(0), mTiming(0) {}

      uint32 mSendTime;
      uint8  mTiming;

      virtual void serialize(BSerialBuffer& sb)
      {
         BSerializable::serialize(sb);
         sb.add(mTiming);
         sb.add(mSendTime);
      }

      virtual void deserialize(BSerialBuffer& sb) 
      {
         BSerializable::deserialize(sb);
         sb.get(&mTiming);
         sb.get(&mSendTime);
      }
};

//==============================================================================
class BTimingHistory : public BSerializable
{
   // this is the timing records for a given client, in the pregame no times are added so this
   // class just returns mMostRecentTiming=BTimeSync::cConstantUpdateInterval
   // once the session is "closed" clients start sending around timing info and this
   // class starts returning real live timing
   public:
      enum { cMaxTimingRecords = 128 };      

      BTimingHistory() :
         mRecordsHead(0),
         mRecordsTail(0),
         mMostRecentSendTime(0),
         mMostRecentTiming(0),
         mLastReportedTime(0),
         mValid(false)
      {}

      void                          addTiming(int32 timing, uint32 clientRecvTime);
      //void                          updateTiming(int32 timing, uint32 clientRecvTime); // append a timing entry if a matching one doesn't exist
      uint8                         getRecentTiming() { return mMostRecentTiming; }
      void                          timeAdvanced(uint32 newRecvTime);

      BTimingRecord*                getTiming(uint32 index);
      uint32                        getTimingAmount() const;
      uint8                         getMostRecentTiming() const { return mMostRecentTiming; }
      uint32                        getMostRecentSendTime() const { return mMostRecentSendTime; }
      uint32                        getLastReportedTime() const { return mLastReportedTime; }

      // Don't call this unless you know what you're doing (hah)
      //void                          setMostRecentTiming(uint8 timing) { mMostRecentTiming = timing; }

      void                          reset();

      void                          serialize(BSerialBuffer& sb);
      void                          deserialize(BSerialBuffer& sb);

      bool                          isValid() const { return mValid; }
      void                          setValid(bool valid) { mValid = valid; }

   private:
      BTimingRecord                 mTimingRecords[cMaxTimingRecords];

      uint32                        mRecordsHead;
      uint32                        mRecordsTail;

      uint32                        mLastReportedTime;

      uint32                        mMostRecentSendTime;

      uint8                         mMostRecentTiming;

      bool                          mValid : 1;
};

//==============================================================================
// this is a little different than BTimingHistory
// This class is used to help determine the earliest allowed recv time
// so we don't accidentally advance one client before receiving all
// the necessary timing data off the wire
class BClientTimeHistory : public BSerializable
{
   public:
      enum 
      { 
         cMaxClientTimeHistorySize = 32
      };

      BClientTimeHistory();

      static BOOL mChanged;

      uint32                     getLast() const { return mLastTime; }
      uint32                     getEarliest() const { return mEarliestTime; }

      void                       addInterval(uint8 interval);
      void                       addTime(uint32 time);
      void                       reset(uint32 time=0);
      uint32                     availableTimes() const;
      void                       advance(uint32 amount);

      void                       update(const BClientTimeHistory& history);

      void                       serialize(BSerialBuffer& sb);
      void                       deserialize(BSerialBuffer& sb);

      uint32 mClientTime[BClientTimeHistory::cMaxClientTimeHistorySize];
      uint32 mLastTime;
      uint32 mEarliestTime;
      uint32 mInsertIndex;
      uint32 mEarliestIndex;

#ifndef BUILD_FINAL
      uint32 mMaxHistory;
      bool   mLimitHistory : 1;
#endif

};

//==============================================================================
// the interface for the time sync system to get info back out of the game/sim
class BTimingInfo
{
   public:
      virtual HRESULT getLocalTiming(uint32& timing, uint32* deviationRemaining) = 0;
      virtual float getMSPerFrame() = 0;
};

//==============================================================================
class BTimeSync : public BSession::BSessionEventObserver
{
   public:
      enum 
      { 
         cDefaultUpdateInterval = 100,
         cDefaultTimeoutValue = 10000,    // The default timeout value checked during game start 
                                          //    to make sure all clients are issuing updates.
                                          // Can be overriden with xNet-TimeSyncTimeout in non-final builds

         cInitialSendTime = 1000, 
         cMaxUpdateInterval = 5000, 

         cMinServiceInterval = 10,        // the smallest service interval we care about

         cUpdateIntervalPingDivisor = 4,  // quarter of the ping,
         cMinimumSendFrequency = 200,     // the msecs that must pass before a new time marker is sent

         cConstantUpdateInterval=100      // used before the variable timing system kicks in (i.e. at the lobby)
      };

      // Constructors
      //BTimeSync(BSession* session, BTimingInfo* timingInfo);
      BTimeSync();

      // Destructors
      ~BTimeSync();

      void                       init(BSession* pSession, BTimingInfo* pTimingInfo);

      // Destructors
      virtual HRESULT dispose();

      // Functions

      // This is the time at which all clients are gaurunteed to be at or greater
      // any message stamped with a sendTime <= getRecvTime is safe to pull off the stream
      uint32                     getRecvTime() const { return mRecvTime; }    
      // This is the time in the future which we have told other clients we will notify them by
      uint32                     getSendTime() const { return mSendTime; }

      void                       service();

      uint32                     getEarliestAllowedRecvTime() const { return mEarliestAllowedRecvTime; }
      void                       jumpToTime(uint32 time);

      // returns the system wide average timing interval for a given receive time
      uint8                      getRecentTiming(uint32 forRecvTime);

      uint32                     getRecentTiming() const { return mRecvUpdateInterval; }

      uint32                     getSendOffset() const { return mSendOffset; }

      bool                       isTimeRolling() const { return mTimeRolling; }

      // call this method to advance time - base class just calls this regularly to advance in realtime
      uint32                     advanceRecvTime();

      uint32                     getSendUpdateInterval() const { return mSendUpdateInterval; }
      uint32                     getPingApproximation() const { return mPingApproximation; }
      uint32                     getNetworkStall() const;
      uint32                     getActualSendInterval() const { return mActualSendInterval; }

      // called from session
      //void                       startClientDisconnect(const BMachineID machineID);
      void                       clientDisconnectReported(BMachineID fromMachineID, BMachineID forMachineID);

      // diagnostics
      uint32                     getCompensationAmount() const { return mCompensationAmount; }
      uint32                     getCompensationInterval() const { return mCompensationInterval; }

      void                       timingDataReceived(const int32 clientIndex, const void* data, const uint32 size);

      uint32                     advanceGameTime();
      void                       gameStarted(bool subUpdating);

      inline BOOL                isGameStarted() const { return (mGameState == cGameStarted); }

      uint32                     getClientSendTime(uint32 clientID) const { return mLastClientTimeHistory[clientID].getLast(); }

      uint32                     getLocalTiming(uint32* deviationRemaining=NULL);
      inline BOOL                canServiceTime() const;

      void                       clientConnected(const int machineID, const int clientID);

      uint8                      getRecentTiming(int32 clientIndex);

      // BSession::BSessionEventObserver
      virtual void               processSessionEvent(const BSessionEvent* pEvent);

      const BTimingLog&          getTimingLog() const { return mTimingLog; }

   private:

      void                       serviceTime();

      void                       updateSendOffset(uint offset);

      // send out the send time marker
      void                       sendTimeMarker(uint32 timeMarker, uint32 lastTimeMarker);
      int32                      calculatePingApproximation() const;
      void                       setTimeRolling(bool v);
      void                       setEarliestAllowedRecvTime(uint32 time);
      void                       setRecvTime(uint32 time) { mRecvTime = time; }
      bool                       canAdvanceTime();
      bool                       simpleCanAdvanceTime();

      // this method should return a new time marker whenever a time marker needs to be sent out
      // it is this method's responsibility to determine when to send time markers - if this method just
      // returns the current send time (getSendTime()) then no time marker will be sent.
      uint32                     generateSendTime(bool force=false);
      void                       timingReceived(int32 clientID, int32 timing);
      uint32                     getInitialHostRecvTime() const { return 0; }
      //uint32                     getDisconnectingClientAmount() const;
      //uint32                     getConnectedClientAmount() const;
      uint32                     getInitialClientRecvTime() const;
      int32                      calculateEarliestTime();
      void                       checkForTimeStart();
      void                       checkForClientDisconnect(int32 clientIndex);
      void                       sessionConnected();

      void                       clientDisconnectReported(uint32 fromClientID, uint32 forClientID, bool add);
      bool                       shouldDisconnect(uint32 clientID, uint32 connectedClients);

      void                       resetClient(uint32 clientID);

   // public for debug purposes
   public:

      // moving the timing history from BClient to here because in disconnect
      // scenarios I could wipe out critical timing history that must be adhered to
      // for the next update
      //
      // even if a client disconnects, we'll continue to use their timing history
      // until we run out
      //
      // when the client disconnects and we exhaust their history, then we can
      // start to skip them in updates
      BTimingHistory             mClientTimingHistory[XNetwork::cMaxClients];

      BTimingLog                 mTimingLog;

      BClientTimeHistory         mLastClientTimeHistory[XNetwork::cMaxClients];

      // state bit offsets into mClientState uint32 values
      enum { cGameState = 0, cTimeState, cSyncState, cSyncStop };
      uint32                     mClientState[XNetwork::cMaxClients];

      uint32                     mDisconnectReports[XNetwork::cMaxClients];

      BTimeSyncPacket            mSyncPacket;
      BTimeIntervalPacket        mIntervalPacket;

      double                     mAccumulatedTimeDelta;
      double                     mLastUpdateInterval;
      double                     mTimerFrequencyFloat;
      int64                      mTimerFrequency;
      int64                      mLastUpdateTime;

      uint32                     mWaitStartTime;
      uint32                     mTimeSyncTimeout;

      uint32                     mEarliestAllowedRecvTime;
      uint32                     mRecvTime;
      uint32                     mLastRecvTime;
      uint32                     mSendUpdateInterval;
      uint32                     mRecvUpdateInterval;
      uint32                     mSendTime;
      uint32                     mNetworkStallStart;
      uint32                     mUpdateRealTime;
      uint32                     mPingApproximation;
      uint32                     mActualSendInterval;
      uint32                     mCompensationAmount;
      uint32                     mCompensationInterval;
      uint32                     mLastSentTime;
      uint32                     mLastSendRealTime;

      uint32                     mSendOffset;

      uint32                     mNumUnsyncedClients;

      // game states, stopped | started... only supports 0 or 1, do not add other states
      enum { cGameStopped = 0, cGameStarted };
      uint32                     mGameState;

      int32                      mEarliestClientTimeIndex;

      BSession*                  mpSession;
      BTimingInfo*               mpTimingInfo;
      BChannel*                  mpChannel;

      bool                       mTimeRolling : 1;
      //bool                       mWaitingOnClients : 1;
      bool                       mRecvTimeDirty : 1;         // set to TRUE everytime timing data is received, FALSE when we calculate recv times

      bool                       mUpdateSendOffset : 1;

      bool                       mSessionConnected : 1; // set to true when we receive BSession::cEventConnected

      bool                       mSubUpdating : 1; // true if sub updating is enabled

}; // BTimeSync
