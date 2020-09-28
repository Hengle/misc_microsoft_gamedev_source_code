//==============================================================================
// TimeSync.cpp
//
// Copyright (c) Ensemble Studios, 2000-2008
//==============================================================================

// Includes
#include "precompiled.h"
#include "TimeSync.h"
#include "Session.h"
#include "Channel.h"
#include "config.h"
#include "econfigenum.h"

BOOL BClientTimeHistory::mChanged = FALSE;

//==============================================================================
// 
//==============================================================================
BTimingLogEntry::BTimingLogEntry() :
   mType(cMaxTypes),
   mClientID(XNetwork::cMaxClients),
   mSendTime(0),
   mTiming(0),
   mRecvTime(0),
   mRecvUpdateInterval(0),
   mEarliestAllowedRecvTime(0),
   mEarliestClientTimeIndex(XNetwork::cMaxClients),
   mGameTime(0)
{
}

//==============================================================================
// 
//==============================================================================
void BTimingLogEntry::init(LogType type, uint clientID, uint sendTime, uint timing)
{
   mType = type;
   mClientID = clientID;
   mSendTime = sendTime;
   mTiming = timing;
}

//==============================================================================
// 
//==============================================================================
void BTimingLogEntry::init(LogType type, uint recvTime, uint recvUpdateInterval, uint earliestRecvTime, uint earliestClientID, uint gameTime)
{
   mType = type;
   mRecvTime = recvTime;
   mRecvUpdateInterval = recvUpdateInterval;
   mEarliestAllowedRecvTime = earliestRecvTime;
   mEarliestClientTimeIndex = earliestClientID;
   mGameTime = gameTime;
}

//==============================================================================
// 
//==============================================================================
BTimingLog::BTimingLog() :
   mInsertIndex(0),
   mGameTime(0)
{
}

//==============================================================================
// 
//==============================================================================
uint BTimingLog::getStartingIndex() const
{
   return mInsertIndex;
}

//==============================================================================
// 
//==============================================================================
const BTimingLogEntry& BTimingLog::getEntry(uint index) const
{
   if (index >= cMaxEntries)
      return mEntry[0];

   return mEntry[index];
}

//==============================================================================
// 
//==============================================================================
uint BTimingLog::getGameTime() const
{
   return mGameTime;
}

//==============================================================================
// 
//==============================================================================
void BTimingLog::add(BTimingLogEntry::LogType type, uint clientID, uint sendTime, uint timing)
{
   mEntry[mInsertIndex].init(type, clientID, sendTime, timing);
   if (++mInsertIndex >= cMaxEntries)
      mInsertIndex = 0;
}

//==============================================================================
// 
//==============================================================================
void BTimingLog::add(BTimingLogEntry::LogType type, uint recvTime, uint recvUpdateInterval, uint earliestRecvTime, uint earliestClientID, uint interval)
{
   interval = Math::Clamp<uint>(interval, 0, 100);

   mEntry[mInsertIndex].init(type, recvTime, recvUpdateInterval, earliestRecvTime, earliestClientID, mGameTime + interval);
   if (++mInsertIndex >= cMaxEntries)
      mInsertIndex = 0;

   mGameTime += interval;
}

//==============================================================================
// BPeerClientDisconnectPacket
//==============================================================================
//class BPeerClientDisconnectPacket : public BChannelPacket
//{
//   public:      
//      // send ctor
//      BPeerClientDisconnectPacket(BTimingHistory* pTimingHistory, BClientTimeHistory* pClientTimeHistory, uint32 clientID) :
//        BChannelPacket(BChannelPacketType::cPeerClientDisconnectPacket),
//         mpTimingHistory(pTimingHistory),
//         mpClientTimeHistory(pClientTimeHistory),
//         mClientID(clientID)
//         {}
//
//      // recv ctor
//      BPeerClientDisconnectPacket(BTimingHistory* pTimingHistory, BClientTimeHistory* pClientTimeHistory) :
//         BChannelPacket(BChannelPacketType::cPeerClientDisconnectPacket),
//         mpTimingHistory(pTimingHistory),
//         mpClientTimeHistory(pClientTimeHistory)
//         {}
//
//      virtual ~BPeerClientDisconnectPacket() {}
//
//      virtual void serialize(BSerialBuffer& sb)
//      {
//         BChannelPacket::serialize(sb);
//
//         if (mpTimingHistory)
//            mpTimingHistory->serialize(sb);
//         if (mpClientTimeHistory)
//            mpClientTimeHistory->serialize(sb);
//         sb.add(mClientID);
//      }
//
//      virtual void deserialize(BSerialBuffer& sb)
//      {
//         BChannelPacket::deserialize(sb);
//
//         if (mpTimingHistory)
//            mpTimingHistory->deserialize(sb);
//         if (mpClientTimeHistory)
//            mpClientTimeHistory->deserialize(sb);
//         sb.get(&mClientID);
//      }
//
//      BTimingHistory*         mpTimingHistory;
//      BClientTimeHistory*     mpClientTimeHistory;
//      uint32                  mClientID;
//};

//==============================================================================
// 
//==============================================================================
BTimeSync::BTimeSync() : 
   mpSession(NULL),
   mpTimingInfo(NULL),
   mLastRecvTime(0),
   mRecvTime(0),
   mWaitStartTime(0),
   mTimeSyncTimeout(cDefaultTimeoutValue),
   mEarliestAllowedRecvTime(0),
   mLastUpdateTime(0),
   mSendUpdateInterval(0),
   mSendTime(0),
   mRecvUpdateInterval(cDefaultUpdateInterval),
   mTimeRolling(false),
   //mWaitingOnClients(false),
   mRecvTimeDirty(false),
   mNetworkStallStart(0),
   mUpdateRealTime(0),
   mPingApproximation(0),
   mActualSendInterval(0),
   mCompensationAmount(0),
   mCompensationInterval(0),
   mLastSentTime(0),
   mGameState(cGameStopped),
   mEarliestClientTimeIndex(0),
   mLastSendRealTime(0),
   mSendOffset(0),
   mNumUnsyncedClients(0),
   mUpdateSendOffset(false),
   mSessionConnected(false),
   mSubUpdating(false)
{
   mAccumulatedTimeDelta = 0.0;
   mLastUpdateInterval = 0.0;
   mLastUpdateTime = 0;
} // BTimeSync::BTimeSync

//==============================================================================
// 
//==============================================================================
BTimeSync::~BTimeSync()
{
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::init(BSession* pSession, BTimingInfo* pTimingInfo)
{
   mpSession = pSession;
   mpTimingInfo = pTimingInfo;

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);

   mTimerFrequency = freq.QuadPart;
   mTimerFrequencyFloat = static_cast<double>(mTimerFrequency);

   Utils::FastMemSet(mClientState, 0, sizeof(mClientState));

   Utils::FastMemSet(mDisconnectReports, 0, sizeof(mDisconnectReports));

   mSyncPacket.setType(BChannelPacketType::cTimeSyncPacket);
   mIntervalPacket.setType(BChannelPacketType::cTimeIntervalPacket);

   // since we'll be sending this direct, need to make sure the channel ID
   // is set appropriately
   mSyncPacket.setChannel(BChannelType::cTimeSyncChannel);
   mIntervalPacket.setChannel(BChannelType::cTimeSyncChannel);

   mpChannel = HEAP_NEW(BChannel, gNetworkHeap);
   mpChannel->init(BChannelType::cTimeSyncChannel, pSession, true);
   mpSession->addObserver(this);
}

//==============================================================================
// 
//==============================================================================
HRESULT BTimeSync::dispose()
{
   // mpSession must exist for TimeSync creation/lifetime
   mpSession->removeObserver(this);

   // mpChannel must exist or the class would have failed the constructor
   if (mpChannel != NULL)
   {
      HEAP_DELETE(mpChannel, gNetworkHeap);
      mpChannel = NULL;
   }

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::updateSendOffset(uint offset)
{
   // what this will do is provide an execution offset to
   // commands sent to the other peers
   //
   // I want the first batch of commands after a time marker to
   // be executed at that marker, subsequent advancements in time
   // will fall on the offset

   // if I'm resetting the offset, I'll pass in 0
   if (offset == 0)
   {
      mSendOffset = 0;
      mUpdateSendOffset = true;
   }
   else if (mUpdateSendOffset)
   {
      // if I've already reset the offset to 0
      // I want the first update to still use 0
      mUpdateSendOffset = false;
   }
   else
   {
      // otherwise I can begin adding the offsets
      mSendOffset += offset;
   }
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::sendTimeMarker(uint32 timeMarker, uint32 lastTimeMarker)
{
   if (timeMarker > lastTimeMarker)
   {
      uint32 localTime = getLocalTiming();

      BChannelPacket* pPacket = NULL;

      // we have to send out a full time sync
      if ((timeMarker - lastTimeMarker) >= 256 || lastTimeMarker == 0)
      {
         nlog(cTimeSyncNL, "BTimeSync::sendTimeMarker -- Sending out new timeMarker %ld, lastTimeMarker %ld, localTiming %ld", timeMarker, lastTimeMarker, localTime);

         mSyncPacket.mTime = timeMarker;
         mSyncPacket.mTiming = static_cast<uint8>(localTime);

         pPacket = &mSyncPacket;
      }
      else // we can send out just an interval
      {
         nlog(cTimeSyncNL, "BTimeSync::sendTimeMarker -- Sending out interval %ld for timeMarker %ld, lastTimeMarker %ld, localTiming %ld", timeMarker-lastTimeMarker, timeMarker, lastTimeMarker, localTime);

         mIntervalPacket.mInterval = static_cast<uint8>(timeMarker - lastTimeMarker);
         mIntervalPacket.mTiming = static_cast<uint8>(localTime);

         pPacket = &mIntervalPacket;
      }

      //blog("BTimeSync::sendTimeMarker:%d,%d", timeMarker, localTime);
      if (mGameState == cGameStarted)
      {
         // There is an assumption being made here.
         //    * All clients should have received their initial timing data by
         //       now during the connection phase, so we need not bother with
         //       individual client checks
         // 
         mpChannel->SendPacketImmediate(*pPacket);
      }
      else
      {
         // Limit the timing updates to only send to clients that we've sent a
         //    time sync init packet to.
         // Once a game is started, I can switch to sending channel broadcasts
         //
         pPacket->setPacketID(mpChannel->getNextPacketID());
         for (int i=0; i < mpSession->getMachineCount(); ++i)
         {
            BMachine* pMachine = mpSession->getMachine(i);
#ifndef BUILD_FINAL
            if (pMachine == NULL)
            {
               nlog(cTimeSyncNL, "BTimeSync::sendTimeMarker -- Missing BMachine at id[%d]", i);
            }
            else
            {
               BNetIPString strAddr(pMachine->getSockAddr());
               nlog(cTimeSyncNL, "BTimeSync::sendTimeMarker -- machineID[%d] isConnected[%d] cTimeState[%d] ip[%s]", pMachine->getID(), pMachine->isConnected(), (mClientState[i] >> cTimeState & 1), strAddr.getPtr());
            }
#endif
            if (pMachine && pMachine->isConnected() && (mClientState[i] >> cTimeState & 1))
            {
               HRESULT hr;
               //hr = mpSession->SendPacketImmediateTo(pMachine, *pPacket);
               hr = mpChannel->SendPacketImmediateTo(pMachine, *pPacket);
               BDEBUG_ASSERTM(SUCCEEDED(hr), "Failed to sendTimeMarker");
            }
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::processSessionEvent(const BSessionEvent* pEvent)
{
   switch (pEvent->mEventID)
   {
      case BSession::cEventChannelData:
      {
         const void* data = (((int8*)pEvent)+sizeof(BSessionEvent));
         int32 channelID = BChannelPacket::getChannel(data);
         if (channelID == BChannelType::cTimeSyncChannel)
         {
            int32 size  = pEvent->mData2;
            int32 fromMachineID = pEvent->mData1;

            timingDataReceived(fromMachineID, data, size);
         }
         break;
      }
      case BSession::cEventClientConnect:
      {
         BClientID clientID = pEvent->mData1;
         BMachineID machineID = pEvent->mData2 >> 16 & 0xFF;
         //BMachineID localMachineID = pEvent->mData2 >> 8 & 0xFF;
         //BMachineID hostMachineID = pEvent->mData2 & 0xFF;

         clientConnected(machineID, clientID);
         break;
      }
      case BSession::cEventConnected:
      {
         sessionConnected();
         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::advanceRecvTime()
{
   if (getRecvTime() > getEarliestAllowedRecvTime()) // This has been in err before.. this is a sanity check
   {
      blog("BTimeSync::advanceRecvTime -- time rolling: %s", mTimeRolling?"true":"false");
      blog("BTimeSync::advanceRecvTime -- Recv Update Interval: %d", mRecvUpdateInterval);
      blog("BTimeSync::advanceRecvTime -- Recv: %d Earliest: %d", getRecvTime(), getEarliestAllowedRecvTime());
      for (uint32 idx=0; idx < XNetwork::cMaxClients; idx++)
         blog("BTimeSync::advanceRecvTime -- Client %d: last recv time: %d", idx+1, mLastClientTimeHistory[idx].getLast());
      // XXX todo, test/remove
      BFAIL("BTimeSync::advanceRecvTime -- PLEASE CALL DOUG @ x275!");
   }

   uint32 otime = getRecvTime();
   uint32 ntime = otime;

   mRecvUpdateInterval = getRecentTiming(otime);

   mTimingLog.add(BTimingLogEntry::cAdvanceTime, otime, mRecvUpdateInterval, mEarliestAllowedRecvTime, mEarliestClientTimeIndex, 0);

   BOOL canAdvance = canAdvanceTime();

   uint32 now = timeGetTime();

   // record stall, adjust time delta
   if (mNetworkStallStart && canAdvance)
   {
      nlog(cTimeSyncNL, "BTimeSync::advanceRecvTime -- Network time block, stalled for %ld", now - mNetworkStallStart);
      mNetworkStallStart = 0;
   }

   // can we run forward?
   if (canAdvance)
   {
      ntime = getRecvTime()+mRecvUpdateInterval;
      updateSendOffset(mRecvUpdateInterval);
   }
   else
   {
      if (!canAdvance && !mNetworkStallStart)
         mNetworkStallStart = now;

      ntime = otime;
   }

   nlog(cGameTimingNL, "BTimeSync::advanceRecvTime -- canAdvance:%d:oldrt:%ld:ri:%ld:newrt:%ld:earliest:%ld", canAdvance, otime, mRecvUpdateInterval, ntime, getEarliestAllowedRecvTime());

   setRecvTime(ntime);
   return ntime;
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::getNetworkStall() const
{
   if (mNetworkStallStart)
      return timeGetTime() - mNetworkStallStart;
   else
      return 0;
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::setTimeRolling(bool v) 
{ 
   mTimeRolling = v;
}      

//==============================================================================
// 
//==============================================================================
void BTimeSync::setEarliestAllowedRecvTime(uint32 time)
{
   if (time < getRecvTime())
   {
      nlog(cTimeSyncNL, "BTimeSync::setEarliestAllowedRecvTime -- current:%ld:recvTime:%ld:new:%ld", mEarliestAllowedRecvTime, getRecvTime(), time);
      mEarliestAllowedRecvTime = getRecvTime();
      return;
   }

   if (time != mEarliestAllowedRecvTime)
      nlog(cTimeSyncNL, "BTimeSync::setEarliestAllowedRecvTime -- current:%ld:recvTime:%ld:new:%ld", mEarliestAllowedRecvTime, getRecvTime(), time);

   mEarliestAllowedRecvTime = time;
}

//==============================================================================
// 
//==============================================================================
bool BTimeSync::simpleCanAdvanceTime()
{
   if (mRecvUpdateInterval)
      return (getRecvTime()+mRecvUpdateInterval <= getEarliestAllowedRecvTime());
   return false;
}

//==============================================================================
// 
//==============================================================================
bool BTimeSync::canAdvanceTime()
{
   // can we update into the next interval according to network time?
   if (mRecvUpdateInterval)
   {
      if (getRecvTime()+mRecvUpdateInterval <= getEarliestAllowedRecvTime())
         return true;
      else if (mRecvTimeDirty || (mEarliestAllowedRecvTime >= mRecvTime && mLastClientTimeHistory[mEarliestClientTimeIndex].availableTimes() > 0))
      {
         mRecvTimeDirty = false;

         // I thought this was the cause of OOS's when we were burning through all
         // our available times waiting until the current mRecvUpdateInterval was able to
         // be used.
         //
         // However, I believe now that this was more a side-effect of a couple other
         // bugs that were fixed.
         //
         // The original thought was that the value of mRecvUpdateInterval was calculated based on a certain set of timing values
         // and if we start advancing through our timing history we lose the context of where the current mRecvUpdateInterval
         // was taken from.
         //
         // The value of mRecvUpdateInterval is taken from the largest update interval that comes before the current mRecvTime.
         // The mEarliestAllowedRecvTime is in the future of mRecvTime so even if I were to burn through times to make
         // the mRecvUpdateInterval "fit" and allow us to advance time forward while another client was unable to do this due to
         // a lack of timing information, then when the other client finally received enough timing data to advance the earliest recv time
         // forward, the mRecvUpdateInterval should still be the same one that was used on the other machine that was able to
         // burn through the timing history to find a "fit".
         uint32 tempEarliest = mEarliestAllowedRecvTime;
         setEarliestAllowedRecvTime(calculateEarliestTime());
         if (tempEarliest != mEarliestAllowedRecvTime)
            return canAdvanceTime();
            //return simpleCanAdvanceTime();
      }
   }

   return false;
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::generateSendTime(bool force)
{
   int32 timing = mRecvUpdateInterval;
   uint32 now = timeGetTime();

   BOOL mustStall = FALSE;
   if (mpSession->isHosted())
   {
      mustStall = (mNumUnsyncedClients > 0);
      if (mustStall)
         nlog(cTimeSyncNL, "BTimeSync::generateSendTime -- host stall waiting on client sync.");
   }

   // time to update?
   if (
         // we're forced
         (force)
         ||
         // or if we don't have to stall waiting on a client that is trying to sync up
         (!mustStall
         &&
         (
            ((getRecvTime() - mLastRecvTime) >= (mSendUpdateInterval - mRecvUpdateInterval)) ||
            ((getSendTime() - getRecvTime()) <= mRecvUpdateInterval)
         )
         //&&
         //// and we're not sending out sendTime markers too frequently
         //(now - mLastSendRealTime > 100)
         )
         //
         // don't limit our sending of time markers because sometimes
         // we may be just on the edge of the minimum send frequency
         //
         // the recv/send time calculation above should be good enough
         // in determining when the time is right to initiate another send
      )
   {
      mLastSendRealTime = now;

      uint32 deviationRemaining=0;
      // localTiming is the current update average + frame average + update std. dev.
      int32 localTiming = static_cast<int32>(getLocalTiming(&deviationRemaining));

      // subtract out twice the local timing to get a better approximation
      // this is because we issue pings at the beginning of the main loop
      // but they're coalesced and actually sent at the beginning of the next loop
      // so this artificially adds two full updates to the actual ping time
      // it's not perfect, but it's closer to the actual ping time
      //int32 pingApprox = max(0, calculatePingApproximation() - localTiming*2);

      // now that the pings are happening on the sim thread, they're no longer
      // affected by the latency of the game loop and we can simply take the ping
      int32 pingApprox = calculatePingApproximation();

      int32 serviceInterval = max(localTiming, cMinServiceInterval); // at least 10ms
      // timing is set to our current mRecvUpdateInterval value which is the rate at
      // which the game is currently advancing the sim per frame
      //
      // if our serviceInterval is less than our current network agreed upon interval
      // then let's go with our network interval
      serviceInterval = max(serviceInterval, timing); // at least whatever our local timing is

      mSendUpdateInterval = min(
                           // at most, the cMaxUpdateInterval (5000)
                           cMaxUpdateInterval,
                           // or
                           max(
                              // at least cMinimumSendFrequency (200)
                              cMinimumSendFrequency,
                              //or
                              max(
                                 // at least twice the service interval (basically 2 frames worth of updates)
                                 serviceInterval*2,
                                 // or a quarter of the ping
                                 (pingApprox/cUpdateIntervalPingDivisor)
                                 )
                              )
                           );

      // now, do we want to increase the command latency by some amount to try and account for any update deviation we have?
      if (deviationRemaining > 0)
      {
         // how much time would we need to run to compenstate for our deviation?
         // hack calculation here :-)
         const int32 cDeviationDivisor = 5;
         const int32 cMaxCompensationInterval = 1000; // max of one second of latency allowed to smooth out deviations
         mCompensationAmount = deviationRemaining / cDeviationDivisor;
         mCompensationInterval = min(cMaxCompensationInterval, mCompensationAmount * localTiming);

         // not now, run some tests
         mSendUpdateInterval = max(mSendUpdateInterval, mCompensationInterval);
      }
      else
      {
         mCompensationAmount = 0;
         mCompensationInterval = 0;
      }

      mPingApproximation = (uint32)(pingApprox > 1 ? (pingApprox >> 1) : 0); // we mostly only care about one-way time

      nlog(cTimeSyncNL, "BTimeSync::generateSendTime -- r:%ld:l:%ld:si:%ld:s:%ld:ri:%ld:p:%ld:f:%ld:stall:%ld:svc:%ld:now:%ld", getRecvTime(), mLastRecvTime, mSendUpdateInterval, getSendTime(), mRecvUpdateInterval, mPingApproximation, force, mustStall, serviceInterval, now);

      mLastRecvTime = getRecvTime();

      //uint32 sendRecvDiff = mSendTime - mRecvTime;
      //uint32 interval = mSendUpdateInterval + mPingApproximation + (serviceInterval * 2);
      //if (sendRecvDiff < interval)
      //   interval += (interval - sendRecvDiff);

      //return getRecvTime() + interval;

      // why am I taking the current recv time and then offsetting from there?
      // shouldn't I be using the current send time as the base line because that's
      // what I've previous sent to the other clients?
      //
      // if I'm forcing a new time, that means we're jumping ahead and I need to offset
      // my send time from the new recv time
      //
      // if we're not forcing a new time, then I should offset from my current send time
      //if (force)
      //   return getRecvTime() + mSendUpdateInterval + mPingApproximation + serviceInterval;

      return getRecvTime() + mSendUpdateInterval + mPingApproximation + serviceInterval + mpSession->getUpdateInterval();
   }
   else
      return getSendTime();
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::jumpToTime(uint32 time)
{
   setEarliestAllowedRecvTime(calculateEarliestTime());

   setRecvTime(time);
   nlog(cTimeSyncNL, "BTimeSync::jumpToTime -- time %ld", time);
   BASSERT((getRecvTime() <= getEarliestAllowedRecvTime()) || (getEarliestAllowedRecvTime() == 0));

   mSendTime = generateSendTime(TRUE);
   updateSendOffset(0);
   sendTimeMarker(mSendTime, 0); // force a send of the time marker (and an advance of mSendTime)
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::getInitialClientRecvTime() const
{  
   nlog(cTimeSyncNL, "BTimeSync::getInitialClientRecvTime -- Enter");

   // mpSession must exist for the lifetime of this instance
   //if (!mpSession)
   //{
   //   nlog(cTimeSyncNL, "BTimeSync::getInitialClientRecvTime -- no session");
   //   return 0;
   //}

   uint32 earliest = 0;   
   // check all clients except for us
   for (int i=0; i < mpSession->getMachineCount(); ++i)
   {
      nlog(cTimeSyncNL, "BTimeSync::getInitialClientRecvTime -- client %ld", i);
      if (i != mpSession->getLocalMachineID() &&
          mClientState[i] >> cTimeState & 1)
      //if (
      //      (mpSession->getClient(i) != mpSession->getLocalClient()) &&
      //      (mpSession->getClient(i)) && 
      //      (mpSession->getClient(i)->isConnected())// &&
      //      //((mClientState[i] >> cGameState & 1) == mGameState))
      //   )
      {
         //uint32 tempTime = mLastClientTimeHistory[i].getLast();
         uint32 tempTime = mLastClientTimeHistory[i].getEarliest();

         nlog(cTimeSyncNL, "BTimeSync::getInitialClientRecvTime -- mLastClientTimeHistory[%ld].getLast() = %ld", i, tempTime);

         if (!earliest)
            earliest = tempTime;
         else if (tempTime < earliest)
            earliest = tempTime;
      }
   }

   nlog(cTimeSyncNL, "BTimeSync::getInitialClientRecvTime -- (earliest-(earliest mod cConstantUpdateInterval)) = %ld", earliest - (earliest % cConstantUpdateInterval));

   // truncate to nearest update interval
   return earliest - (earliest % cConstantUpdateInterval);
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::serviceTime()
{
   if (!canServiceTime())
      return;

   uint32 now = timeGetTime();

   if ((now - mUpdateRealTime) >= mRecvUpdateInterval)
   {
      uint32 otime = getRecvTime();
      advanceRecvTime();
      if (getRecvTime() != otime)
      {
         mUpdateRealTime = now;
      }
      else
         mRecvTimeDirty = true; // force a new time calculation if I failed to advance
   }
}

//==============================================================================
// 
//==============================================================================
BOOL BTimeSync::canServiceTime() const
{
   return (!isGameStarted());
}

//==============================================================================
// 
//==============================================================================
int32 BTimeSync::calculateEarliestTime()
{
   if (mpSession->isDisconnecting())
      return getEarliestAllowedRecvTime();

   // first try and advance the client time history forward
   for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
   {
      if (mClientState[i] >> cTimeState & 1)
      //if (
      //      (mpSession->getClient(i)) &&
      //      (mpSession->getClient(i)->isConnected()) &&
      //      //((mClientState[i] >> cGameState & 1) == mGameState) && // and the client game state is the same as our game state
      //   )
      {
         if (mLastClientTimeHistory[i].getLast() == 0)
            continue;

         // if this client's earliest time is the same as our calculated earliest allowed time
         if (mLastClientTimeHistory[i].getEarliest() == mEarliestAllowedRecvTime)
         {
            // and we're stalling or about to stall and there are more timings availble
            // then advance by one and allow the earliest to be re-calculated
            if (mEarliestAllowedRecvTime - mRecvTime < mRecvUpdateInterval && mLastClientTimeHistory[i].availableTimes() > 0)
               mLastClientTimeHistory[i].advance(1);
         }
      }
   }

   BOOL logEarliest = BClientTimeHistory::mChanged;

   BOOL connecting = FALSE;
   uint32 earliest = 0;
   uint32 earliestClient=UINT32_MAX;
   // check all clients
   for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
   {
      if (mClientState[i] >> cTimeState & 1)
      //if (
      //      (mpSession->getClient(i)) &&
      //      (mpSession->getClient(i)->isConnected()) &&
      //      //((mClientState[i] >> cGameState & 1) == mGameState) && // and the client game state is the same as our game state
      //   )
      {
         // means that someone is connected and hasn't told us their time yet
         if (mLastClientTimeHistory[i].getEarliest() == 0)
         {
            connecting = TRUE;
            continue;
         }

         if (logEarliest)
            nlog(cTimeSyncNL, "BTimeSync::calculateEarliestTime -- mLastClientTimeHistory[%ld].getEarliest() = %ld, available = %ld", i, mLastClientTimeHistory[i].getEarliest(), mLastClientTimeHistory[i].availableTimes());

         uint32 tempTime = mLastClientTimeHistory[i].getEarliest();
         if (!earliest)
         {
            earliest = tempTime;
            earliestClient = i;
         }
         else if (tempTime < earliest)
         {
            earliest = tempTime;
            earliestClient = i;
         }
      }
   }

   if (earliestClient != UINT32_MAX)
   {
      mEarliestClientTimeIndex = earliestClient;

      checkForClientDisconnect(earliestClient);
   }

   // advance time
   if (logEarliest)
      nlog(cTimeSyncNL, "BTimeSync::calculateEarliestTime -- Earliest time calculation %ld", earliest);

   BClientTimeHistory::mChanged = FALSE;

   if (connecting)
      return getEarliestAllowedRecvTime();
   else
      return earliest;
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::timingDataReceived(const int32 clientIndex, const void* data, const uint32 size)
{
   // we'll be in a cSyncStop state for this client if we received a peer client disconnect
   // packet from another client.
   //
   // we're relying on other clients to help us fill in gaps in our timing history and we
   // do not want to let a stray timing packet that's in the channel queue stray into our data
   if (mClientState[clientIndex] >> cSyncStop & 1)
      return;

   int32 packetType = BChannelPacket::getType(data);

   //if (packetType == BChannelPacketType::cTimeIntervalGamePacket)
   //{
   //   // if the game has not started yet, then I need to flush/mark this client's data as pending
   //   // and not to be used until the game has entered the started state for this client
   //   if ((mClientState[clientIndex] >> cGameState & 1) != cGameStarted)
   //   {
   //      // need to flush the client time history and the timing data history
   //      // they're used for slightly different tracking methods so we need check them both
   //      mLastClientTimeHistory[clientIndex].reset();

   //      if (mpSession->getClient(clientIndex))
   //         mpSession->getClient(clientIndex)->getTimingHistory()->reset();

   //      mClientState[clientIndex] |= cGameStarted << cGameState;

   //      // also perform a check to see if we can begin sending pings again
   //      checkForPingRestart();
   //   }

   //   BTimeIntervalPacket packet(packetType);
   //   packet.deserializeFrom(data, size);

   //   mLastClientTimeHistory[clientIndex].addInterval(packet.mInterval);

   //   nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- mLastClientTimeHistory[%ld].getLast()=%ld (interval %ld)", clientIndex, mLastClientTimeHistory[clientIndex].getLast(), packet.mInterval);

   //   mRecvTimeDirty = TRUE;
   //   timingReceived(clientIndex, static_cast<int32>(packet.mTiming));
   //}
   //else if (packetType == BChannelPacketType::cTimeSyncGamePacket)
   //{
   //   if ((mClientState[clientIndex] >> cGameState & 1) != cGameStarted)
   //   {
   //      // need to flush the client time history and the timing data history
   //      // they're used for slightly different tracking methods so we need check them both
   //      mLastClientTimeHistory[clientIndex].reset();

   //      if (mpSession->getClient(clientIndex))
   //         mpSession->getClient(clientIndex)->getTimingHistory()->reset();

   //      mClientState[clientIndex] |= cGameStarted << cGameState;

   //      // also perform a check to see if we can begin sending pings again
   //      checkForPingRestart();
   //   }

   //   BTimeSyncPacket packet(packetType);
   //   packet.deserializeFrom(data, size);

   //   nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- Got client %ld time %ld", clientIndex, packet.mTime);

   //   mLastClientTimeHistory[clientIndex].addTime(packet.mTime);

   //   mRecvTimeDirty = TRUE;
   //   timingReceived(clientIndex, static_cast<int32>(packet.mTiming));
   //}
   //else 
   if (packetType == BChannelPacketType::cTimeIntervalPacket)
   {
      // if game started, ignore this data
      //if (isGameStarted())
      //{
      //   // ignore this data
      //   return;
      //}

      BTimeIntervalPacket packet(packetType);
      packet.deserializeFrom(data, size);

      // add interval to client's time
      BDEBUG_ASSERT(mLastClientTimeHistory[clientIndex].getLast() != 0); // we should have heard an initial time update from this guy by now!
      if (mLastClientTimeHistory[clientIndex].getLast() == 0)
      {
         nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- ERROR: Adding an interval without an initial time - fromClient %ld!", clientIndex);
      }

      mLastClientTimeHistory[clientIndex].addInterval(packet.mInterval);

      nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- mLastClientTimeHistory[%ld].getLast()=%ld (interval %ld)", clientIndex, mLastClientTimeHistory[clientIndex].getLast(), packet.mInterval);

      mRecvTimeDirty = true;
      timingReceived(clientIndex, static_cast<int32>(packet.mTiming));

      mTimingLog.add(BTimingLogEntry::cTiming, clientIndex, mLastClientTimeHistory[clientIndex].getLast(), packet.mTiming);
   }
   else if (packetType == BChannelPacketType::cTimeSyncInitPacket)
   {
      if (isGameStarted())
      {
         //blog("BTimeSync::timingDataReceived -- Client %d sent sync init");
         // XXX todo, test/remove
         BFAIL("BTimeSync::timingDataReceived -- received time sync init, game already started -- PLEASE CALL DOUG @ x275!");
      }

      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket);
      packet.deserializeFrom(data, size);

      // init both the sender and myself
      mClientState[clientIndex] |= 1 << cTimeState;
      mClientState[mpSession->getLocalMachineID()] |= 1 << cTimeState;

      // set client time
      //BASSERT(mLastClientTimeHistory[clientIndex].getLast() <= packet.mTime);
      if (mpSession->isHosted() && (mLastClientTimeHistory[clientIndex].getLast() == 0))
      {
         // this is a newly joined client, so notify the session that he's all synced up
         //mpSession->clientTimeSynced(clientIndex);
         if (mClientState[clientIndex] >> cSyncState & 1)
            --mNumUnsyncedClients;
         mClientState[clientIndex] &= ~(1 << cSyncState);
      }

      nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- Got client %ld init time %ld", clientIndex, packet.mTime);

      mLastClientTimeHistory[clientIndex].addTime(packet.mTime);

      checkForTimeStart();

      timingReceived(clientIndex, static_cast<int32>(packet.mTiming));
   }
   else if (packetType == BChannelPacketType::cTimeSyncPacket)
   {
      // if game started, ignore this data
      //if (isGameStarted())
      //{
      //   // ignore this data
      //   return;
      //}

      BTimeSyncPacket packet(packetType);
      packet.deserializeFrom(data, size);

      // set client time
      //BASSERT(mLastClientTimeHistory[clientIndex].getLast() <= packet.mTime);
      if (mpSession->isHosted() && (mLastClientTimeHistory[clientIndex].getLast() == 0))
      {
         // this is a newly joined client, so notify the session that he's all synced up
         //mpSession->clientTimeSynced(clientIndex);
         if (mClientState[clientIndex] >> cSyncState & 1)
            --mNumUnsyncedClients;
         mClientState[clientIndex] &= ~(1 << cSyncState);
      }

      nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- Got client %ld time %ld", clientIndex, packet.mTime);

      mLastClientTimeHistory[clientIndex].addTime(packet.mTime);

      mRecvTimeDirty = true;
      timingReceived(clientIndex, static_cast<int32>(packet.mTiming));

      mTimingLog.add(BTimingLogEntry::cTiming, clientIndex, packet.mTime, packet.mTiming);
   }
   //else if (packetType == BChannelPacketType::cPeerClientDisconnectPacket)
   //{
   //   BTimingHistory timingHistory;
   //   BClientTimeHistory clientTimeHistory;

   //   BPeerClientDisconnectPacket packet(&timingHistory, &clientTimeHistory);
   //   packet.deserializeFrom(data, size);     

   //   clientDisconnectReported(static_cast<uint32>(clientIndex), packet.mClientID, true);

//-- FIXING PREFIX BUG ID 7495
   //   const BMachine* pMachine = mpSession->getMachine(packet.mClientID);
//--

   //   nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- got peerClientDisconnectPacket from client %ld, client index %ld, mLastClientTime %ld, earliest time from other client %ld", clientIndex, packet.mClientID, mLastClientTimeHistory[packet.mClientID].getLast(), clientTimeHistory.getEarliest());

   //   // bring me up to date with latest client 
   //   if (pMachine != NULL)
   //   {
   //      // we do not want to process any more sync information from this client
   //      // and instead rely upon other peers to supply us with information
   //      mClientState[packet.mClientID] |= 1 << cSyncStop;

   //      // the history we're iterating over will be the known timing history
   //      // from the client that sent us the packet
   //      //
   //      // we're trying to fill in any gaps in our timing history for the given client
   //      for (uint32 i=0; i < timingHistory.getTimingAmount(); i++)
   //      {
//-- FIXING PREFIX BUG ID 7494
   //         const BTimingRecord* pTimingRecord = timingHistory.getTiming(i);
//--
   //         //if (pTimingRecord)
   //         //   pClient->getTimingHistory()->updateTiming(pTimingRecord->mTiming, pTimingRecord->mSendTime);
   //         if (pTimingRecord)
   //            mClientTimingHistory[packet.mClientID].updateTiming(pTimingRecord->mTiming, pTimingRecord->mSendTime);
   //      }

   //      // I do not want to stamp the most recent timing value here!
   //      // the most recent timing value should be calculated based on the gaps I was able to fill in
   //      //client->getTimingHistory()->setMostRecentTiming(history.getMostRecentTiming());

   //      // if we haven't heard a time from this guy, then he isn't all connected yet
   //      // So, just drop him out immediately
   //      //
   //      // 4/4/08 DPM - do not immediately disconnect a client based on lack of timing history,
   //      //              instead we're going to wait for the normal process of disconnection
   //      //              which is checking for client disconnects during our time advancement
   //      //if (mLastClientTimeHistory[packet.mClientID].getLast() == 0)
   //      //{
   //      //   // tell everyone that this guy is disconnected from me
   //      //   // in case they are waiting on me to send out a disconnect packet
   //      //   //BPeerClientDisconnectPacket dpacket(pClient->getTimingHistory(), &mLastClientTimeHistory[packet.mClientID], packet.mClientID);
   //      //   BPeerClientDisconnectPacket dpacket(&mClientTimingHistory[packet.mClientID], &mLastClientTimeHistory[packet.mClientID], packet.mClientID);
   //      //   mpChannel->SendPacket(dpacket);

   //      //   // this will make the client-> go to NULL, so make sure the packet is sent above prior to deleting the client.
   //      //   nlog(cTimeSyncNL, "BTimeSync::timingDataReceived -- we haven't heard a time from this guy, so we're just gonna drop him");
   //      //   mpSession->finishClientDisconnect(packet.mClientID);
   //      //}
   //      //else
   //      {
   //         // this section is the cause of bug PHX-1037
   //         // by resetting the values stored in mLastClientTimeHistory, we might skip one
   //         //
   //         // part of the problem is that the client disconnect packet only sends mLastSendTime
   //         // and not the entire history.  We need to change the peer client disconnect packet to
   //         // include the client time history and then we need to update it here to fill in any
   //         // missing gaps... 
   //         //
   //         // the history of client times should be the same for all machines.
   //         // dropped packets are resent so this will only be a problem if the user's client dies
   //         // before it has a chance to send it's last timing data to all other clients.
   //         //
   //         // I should be able to merge the time history from the client that sent me the information
   //         // with my own time history.  Missing data is defined as a send time that's greater than
   //         // my current known send time.

   //         // reset the time history for this client and start the client off at the given send time
   //         mLastClientTimeHistory[packet.mClientID].update(clientTimeHistory);
   //         //if (packet.mLastSendTime > mLastClientTimeHistory[packet.mClientID].getLast())
   //         //   mLastClientTimeHistory[packet.mClientID].reset(packet.mLastSendTime);

   //         if (!pMachine->isLocal())
   //         {
   //            //pClient->clientDisconnectReported(static_cast<uint32>(clientIndex), true);

   //            pMachine = NULL; // client could no longer be valid after checkForClientDisconnect.
   //            checkForClientDisconnect(packet.mClientID);
   //         }

   //         // XXX temp hack until better fix in place
   //         mpSession->disconnectMachine(packet.mClientID);

   //         mRecvTimeDirty = true;
   //      }
   //   }

   //   // if a client disconnected during game start but before we've had a chance
   //   // to receive timesync updates, then we need to attempt a ping start
   //   //checkForPingRestart();
   //}
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::checkForTimeStart()
{
   nlog(cTimeSyncNL, "BTimeSync::checkForTimeStart");

   if (!isTimeRolling() && mSessionConnected)
   {
      // Check if we have heard times for all clients in the session
      // if so, jump us forward in time
      // and send out our time

      for (uint i=0; i < mpSession->getMaxMachineCount(); i++)
      {
         if (mpSession->getMachine(i))
         {
            nlog(cTimeSyncNL, "BTimeSync::checkForTimeStart -- client %ld, local client connected? %ld, other client connected? %ld, local client? %ld, lastClientTime %ld",
               i,
               mpSession->getLocalMachine()?mpSession->getLocalMachine()->isConnected():0,            
               mpSession->getMachine(i)?mpSession->getMachine(i)->isConnected():0,
               (mpSession->getMachine(i) == mpSession->getLocalMachine()),
               mLastClientTimeHistory[i].getLast());
         }

         if (  
               mpSession->getLocalMachine() &&
               mpSession->getLocalMachine()->isConnected() && // we're connected, and
               mpSession->getMachine(i) &&  // is client valid, and
               mpSession->getMachine(i)->isConnected() && // client connected, and 
               ((mClientState[i] >> cTimeState & 1) == 0) && // client is not in the time sync state
               (mpSession->getMachine(i) != mpSession->getLocalMachine()) && // client isn't us, and
               !mLastClientTimeHistory[i].getLast() // client hasn't told us his time yet?          
            )
            return; // if so, drop out. We haven't heard from all clients yet
      }

      setTimeRolling(true);

      uint32 initialRecvTime = getInitialClientRecvTime();

      // the host will not be calling into here right now (until we fix the joining code)
      //
      // for the host, and all other clients, there will be code in the time interval/sync packet receive code
      // that will insert the new timing value into the history
      //
      // the new index is determined in ::calculateEarliestTime. if I'm at index 0 but I have timing data from all clients for index+1
      // then I can advance the earliest time to the next index and determine the earliest allowed receive time from that
      //
      // once all clients have reported a new time, I can advance the index from the baseline of 0 here, to 1
      //
      // the indexes are tracked on a per client basis
      //
      // if I've wrapped around and am about to overwrite a time that I haven't been able to move past,
      // one or more of the clients are not responding.  how to handle that case?
      // it could be that a client disconnected and I'm still receiving timing data from everyone except that particular client index
      //
      // check for a client disconnect at a certain threshold, if we are about to overwrite, then start the disconnect process
      //
      // so how should I do the data structures for these indexes
      mLastClientTimeHistory[mpSession->getLocalMachineID()].reset(initialRecvTime);

      // initialize our timing value to something other than 0
      mClientTimingHistory[mpSession->getLocalMachineID()].addTiming(getLocalTiming(), initialRecvTime);

      // calculate fresh values and assign them
      setEarliestAllowedRecvTime(initialRecvTime);
      jumpToTime(initialRecvTime);
      nlog(cTimeSyncNL, "BTimeSync::checkForTimeStart -- Client initial time info: getRecvTime() %ld, getInitialClientRecvTime() %ld", getRecvTime(), initialRecvTime);
   }
   else
   {
      // BTimeSync is only initialized once we have a full game and we're launching
      setEarliestAllowedRecvTime(calculateEarliestTime());
   }
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::service()
{
   if (isTimeRolling())
   {
      serviceTime();
      //BASSERT((getRecvTime() <= getEarliestAllowedRecvTime()) || (getEarliestAllowedRecvTime() == 0));

      uint32 oldSendTime = getSendTime();
      uint32 newSendTime = generateSendTime();
      if (newSendTime > oldSendTime)
      {
         mSendTime = newSendTime;
         updateSendOffset(0);

         static uint32 lastSentTime = timeGetTime();
         nlog(cTimeSyncNL, "BTimeSync::service -- mSendTime %ld - last projected interval %ld - actual interval %ld", mSendTime, mSendUpdateInterval, timeGetTime()-lastSentTime);
         mActualSendInterval = timeGetTime()-lastSentTime;
         lastSentTime = timeGetTime();

         sendTimeMarker(mSendTime, oldSendTime);
      }
   }
   else if (mSessionConnected && mpSession->isHosted())
   {
      // we're time sync'd, let's roll
      mClientState[mpSession->getHostMachineID()] |= 1 << cTimeState;

      jumpToTime(getInitialHostRecvTime());
      mLastClientTimeHistory[mpSession->getHostMachineID()].reset(getSendTime());
      nlog(cTimeSyncNL, "BTimeSync::service -- Host initial time info: getRecvTime() %ld", getRecvTime());
      //mpSession->clientTimeSynced(mpSession->getHostID());
      setTimeRolling(true);
   }

   //if (mWaitingOnClients &&                                  // if we're still waiting on clients to time sync with us
   //    (mGameState == cGameStarted) &&                       // and the game is loaded
   //    (timeGetTime() - mWaitStartTime) > mTimeSyncTimeout)  // and we've exceeded our timeout value
   //{
   //   // start by assuming all clients have responded and then attempt to find a negative
   //   BOOL allClientsResponding = TRUE;

   //   // notify the session layer of the client(s) that have not responded yet
   //   for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
   //   {
   //      if (mpSession->getLocalClient() &&                  // we better have a local client and
   //          mpSession->getLocalClient()->isConnected() &&   // we're connected, and
   //          mpSession->getClient(i) &&                      // is client valid, and
   //          mpSession->getClient(i)->isConnected() &&       // client connected, and 
   //          (mpSession->getClient(i) != mpSession->getLocalClient())) // client isn't us
   //      {
   //         // the client has not yet sent us in-game sync traffic
   //         if ((mClientState[i] >> cGameState & 1) != cGameStarted)
   //         {
   //            allClientsResponding = FALSE;

   //            mpSession->notifyUnresponsiveClient(i, mWaitStartTime);
   //         }
   //         else
   //            mpSession->notifyResponsiveClient(i, timeGetTime()); // the client is responding, this may be called unecessarily, the upper layers should handle that
   //      }
   //   }

   //   // if all connected clients are responding, then it's safe to shutoff our timeout check
   //   if (allClientsResponding)
   //      mWaitingOnClients = FALSE;
   //   else
   //   {
   //      // if we've reached our initial timeout, shrink the timeout window to no less than 2 seconds
   //      mWaitStartTime = timeGetTime();
   //      mTimeSyncTimeout = max(2000, mTimeSyncTimeout >> 1);
   //   }
   //}
}

//==============================================================================
// 
//==============================================================================
int32 BTimeSync::calculatePingApproximation() const
{
   uint32 maxPing = mpSession->getMaxPing();
   uint32 dev = mpSession->getMaxPingDeviation();

   return max(1, static_cast<int32>(maxPing+dev));
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::timingReceived(int32 clientID, int32 timing)
{
   // if the session is still open, ignore timing   
   //if (mpSession->getClient(clientID))
   //   mpSession->getClient(clientID)->getTimingHistory()->addTiming(timing, getClientSendTime(clientID));   

   mClientTimingHistory[clientID].addTiming(timing, getClientSendTime(clientID));
}

//==============================================================================
// 
//==============================================================================
//uint32 BTimeSync::getConnectedClientAmount() const
//{
//   // mpSession must exist for the lifetime of this instance
//   //if (!mpSession)
//   //   return 0;
//
//   uint32 a = 0;
//
//   for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
//   {
//      if (  mpSession->getClient(i) && 
//            mpSession->getClient(i)->isConnected() &&
//            (mpSession->getClient(i)->getDisconnectCount() <= 0) &&
//            (mLastClientTimeHistory[i].getLast() > 0))
//         a++;
//   }
//
//   return a;
//}

//==============================================================================
// 
//==============================================================================
void BTimeSync::clientConnected(const int32 machineID, const int32 clientID)
{
   BASSERTM(machineID >=0 && machineID < XNetwork::cMaxClients, "Invalid machineID");
   BASSERTM(clientID >=0 && clientID < XNetwork::cMaxClients, "Invalid clientID");

   // if this machine is already connected, then I can ignore the time sync setup process
   // but still log the new clientID
   nlog(cTimeSyncNL, "BTimeSync::clientConnected -- machine[%d], client[%d] connect.", machineID, clientID);

   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
      return;

   if (mClientState[machineID] >> cTimeState & 1)
      return;

   // reset the last client time history and the client timing history
   //
   // wow confusing, ok, last client time history is the generated send times from other clients
   // the client timing history is the update interval, it got moved from BClient::mTimingHistory to here
   //
   mLastClientTimeHistory[machineID].reset();
   mClientTimingHistory[machineID].reset();
   mClientTimingHistory[machineID].setValid(true);
   mClientState[machineID] &= ~(1 << cSyncStop);

   // if we are hosting, and this isn't US connecting, save unsynced client ID
   if (mpSession->isHosted() && (machineID != mpSession->getLocalMachineID()))
   {
      // place this client in the unsync state
      if ((mClientState[machineID] >> cSyncState & 1) == 0)
      {
         mClientState[machineID] |= 1 << cSyncState;
         ++mNumUnsyncedClients;
      }
   }

   BMachine* pMachine = mpSession->getMachine(machineID);
   if (!isTimeRolling())
   {
      nlog(cTimeSyncNL, "BTimeSync::clientConnected -- time not rolling.");
      // This code was commented out because it was thought to be the cause of the game setting a recv time in the past.
      // Basically when we connect to the game, the host will stall time advancing until we send out our time. If we send 
      // out a fake time, then we are essentially telling the host we are ready when it is possible that we are still connecting
      // to other clients. We should wait until all clients are connected before calculating our send time and telling everyone.
      // That happens as a result of calling checkfortimestart when the session connected event comes in.
/*
      if (mLastClientTime[mpSession->getHostID()] > 0)
      {
         BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket, mLastClientTime[mpSession->getHostID()], getLocalTiming());
         mpChannel->SendPacketTo(client, packet);
         nlog(cTimeSyncNL, "BTimeSync::clientConnected -- sending out fake time init to get the ball rolling.");
         blog("BTimeSync::clientConnected -- sending out fake time init to get the ball rolling.");
      }
*/
      return;
   }

   if (machineID != mpSession->getLocalMachineID())
   {
      uint32 timing = getLocalTiming();
      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket, getSendTime(), timing);
      packet.setChannel(BChannelType::cTimeSyncChannel);
      // sending via BSession because we need to bypass the sync code within BChannel for this message
      //mpChannel->SendPacketTo(pMachine, packet);
      mpSession->SendPacketTo(pMachine, packet);

      // we're now ready to start sending timing data to this client
      mClientState[machineID] |= 1 << cTimeState;

      nlog(cTimeSyncNL, "BTimeSync::clientConnected -- Sending out new mSendTime[%ld] to new machine[%d], client[%d]", getSendTime(), machineID, clientID);
   }
}

//==============================================================================
// 
//==============================================================================
uint8 BTimeSync::getRecentTiming(int32 clientIndex)
{
   if (clientIndex < 0 || clientIndex >= XNetwork::cMaxClients)
      return 0;

   return mClientTimingHistory[clientIndex].getRecentTiming();
}

//==============================================================================
// 
//==============================================================================
//void BTimeSync::startClientDisconnect(const BMachineID machineID)
//{
//   if (machineID >= XNetwork::cMaxClients)
//      return;
//
//   //BClient* pClient = mpSession->getClient(clientIndex);   
//   //if (pClient == NULL)
//   //   return;
//
//   //int32 count = getConnectedClientAmount()-1;
//
//   //if (pClient->getDisconnectClients() < 0)
//   //   pClient->setDisconnectClients(count);
//   //else
//   //   count = pClient->getDisconnectClients();
//
//   //nlog(cTimeSyncNL, "BTimeSync::startClientDisconnect -- linkDisconnected index %ld set disconnect count %ld", clientIndex, count);
//   nlog(cTimeSyncNL, "BTimeSync::startClientDisconnect -- linkDisconnected index %ld", machineID);
//
//   // tell everyone that this guy is disconnected from me
//   // packet(pClient->getTimingHistory(), &mLastClientTimeHistory[clientIndex], clientIndex);
//
//   //BPeerClientDisconnectPacket packet(&mClientTimingHistory[machineID], &mLastClientTimeHistory[machineID], machineID);
//   //mpChannel->SendPacket(packet);
//
//   if (mClientState[machineID] >> cSyncState & 1)
//      --mNumUnsyncedClients;
//
//   // clear all states for this client
//   //mClientState[clientIndex] = 0;
//   //Instead - lets mark him as SYNCSTOPPED - just as if we had recieved a peerClientDisconnect packet
//   //mClientState[machineID] |= 1 << cSyncStop;
//}

//==============================================================================
// 
//==============================================================================
void BTimeSync::clientDisconnectReported(BMachineID fromMachineID, BMachineID forMachineID)
{
   if (forMachineID >= XNetwork::cMaxClients)
      return;

   //nlog(cTimeSyncNL, "BTimeSync::clientDisconnectReported -- for machineID[%d] fromMachineID[%d]", machineID, fromMachineID);

   clientDisconnectReported(fromMachineID, forMachineID, true);

   // if checkForClientDisconnect returns true, then we're all done with this machine and
   // can safely reduce the number of unsynced clients if need be
   //if (checkForClientDisconnect(forMachineID))
   //{
   //   if (mClientState[forMachineID] >> cSyncState & 1)
   //      --mNumUnsyncedClients;
   //}
}

//==============================================================================
// 
//==============================================================================
//uint32 BTimeSync::getDisconnectingClientAmount() const
//{
//   // mpSession must exist for the lifetime of this instance
//   //if (!mpSession)
//   //   return 0;
//
//   uint32 a = 0;
//
//   for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
//   {
//      if (mpSession->getClient(i) && (mpSession->getClient(i)->getDisconnectCount() > 0))
//         a++;
//   }
//
//   return a;
//}

//==============================================================================
// 
//==============================================================================
uint8 BTimeSync::getRecentTiming(uint32 forRecvTime)
{
   //uint recvTime = forRecvTime;

   // once all the clients have reported a time to me
   // I can start advancing the recv time forward
   //if (forRecvTime == 0)
   //{
   //   // first, make sure all clients have reported
   //   //for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
   //   //{
   //   //   if (
   //   //      mpSession->getLocalClient() &&
   //   //      mpSession->getLocalClient()->isConnected() && // we're connected, and
   //   //      mpSession->getClient(i) &&  // is client valid, and
   //   //      mpSession->getClient(i)->isConnected() && // client connected, and 
   //   //      (mpSession->getClient(i) != mpSession->getLocalClient()) && // client isn't us, and
   //   //      ((mClientState[i] >> cGameState & 1) != mGameState)
   //   //      )
   //   //      return 0; // if so, drop out. We haven't heard from all clients yet
   //   //}

   //   // if recv time == 0 and the earliest time == 0 then we've just hosted
   //   // or the game has just started.
   //   if (mEarliestAllowedRecvTime == 0)
   //   {
   //      setEarliestAllowedRecvTime(calculateEarliestTime());

   //      // starting point will be the earliest known time
   //      forRecvTime = mEarliestAllowedRecvTime;
   //   }
   //   else
   //   {
   //      // could this ever be the case?
   //      //BDEBUG_ASSERTM(mEarliestAllowedRecvTime==0, "BTimeSync::getRecentTiming -- recv time == earliest recv == 0, call doug x275");

   //      // the following was old code that was used in the time before we reset everything upon game start
   //      // assert added and code commented out so we can monitor for problems
   //      //
   //      // pick the timing value of the client that matches the earliest allowed recv time
   //      //BClient *client = mpSession->getClient(mEarliestClientTimeIndex);
   //      //if (client)
   //      //{
   //      //   forRecvTime = mEarliestAllowedRecvTime+1;
   //      //}
   //   }
   //}

   // loop through all clients and get recent timing to
   // figure out a new update system wide timing
   uint8 timing = 0;

   for (uint32 i=0; i < XNetwork::cMaxClients; ++i)
   {
      // need to filter out disconnected AND stalled clients from
      // this calculation, don't know quite how yet
      //
      // perhaps a bool on BTimingHistory that gets set when we exhaust a client
      BTimingHistory& history = mClientTimingHistory[i];

      // verify that this history is still valid
      if (!history.isValid())
         continue;

      history.timeAdvanced(forRecvTime);

      // if the original recv time we're given is 0 then we'll want to pick the largest timing value regardless of the new recv time
      //if ((recvTime == 0 || history.getMostRecentSendTime() < forRecvTime) && history.getMostRecentTiming() > timing)
      if ((forRecvTime == 0 || history.getMostRecentSendTime() <= mEarliestAllowedRecvTime) && history.getMostRecentTiming() > timing)
      {
         timing = history.getMostRecentTiming();
      }
   }

   nlog(cTimeSyncNL, "BTimeSync::getRecentTiming -- forRecvTime[%u] timing[%u] mEarliestAllowedRecvTime[%u] mEarliestClientTimeIndex[%d]", forRecvTime, timing, mEarliestAllowedRecvTime, mEarliestClientTimeIndex);

   //for (uint32 i=0; i < mpSession->getMaxMachineCount(); i++)
   //{
   //   BClient* pClient = mpSession->getClient(i);

   //   // insure that we're only processing timing data for clients that match our game state
   //   //if (client && ((mClientState[i] >> cGameState & 1) == mGameState))
   //   //{
   //   //   BTimingHistory* history = client->getTimingHistory();

   //   //   // advance our time window up to forRecvTime
   //   //   history->timeAdvanced(forRecvTime);

   //   //   timing = max(timing, history->getMostRecentTiming());
   //   //}
   //   if (pClient != NULL)
   //   {
   //      BTimingHistory* history = pClient->getTimingHistory();

   //      // advance our time window up to forRecvTime
   //      history->timeAdvanced(forRecvTime);

   //      if (history->getMostRecentTiming() > timing)
   //      {
   //         timing = history->getMostRecentTiming();
   //      }
   //   }
   //}

   return min(timing, 255);
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::gameStarted(bool subUpdating)
{   
   nlog(cGameTimingNL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Game Start ");   

   mGameState = cGameStarted;
   mSubUpdating = subUpdating;

   //mWaitingOnClients = TRUE;
   //mWaitStartTime = timeGetTime();

   // reset our internal timing variables and start fresh with the game timings
   // need checks in processing received timing data to make sure that once we're in
   // this state that we do not process old timing data
   //reset();
   mAccumulatedTimeDelta = 0.0;

   //mSyncPacket.setType(BChannelPacketType::cTimeSyncGamePacket);
   //mIntervalPacket.setType(BChannelPacketType::cTimeIntervalGamePacket);
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::getLocalTiming(uint32* deviationRemaining)
{
   uint32 timing = 0;
   if (isGameStarted() && mpTimingInfo)
   {
      HRESULT hr = mpTimingInfo->getLocalTiming(timing, deviationRemaining);
      if (hr == HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE))
         return cConstantUpdateInterval;
      else
         return timing;
   }
   else
      return cConstantUpdateInterval;
}

//==============================================================================
// 
//==============================================================================
uint32 BTimeSync::advanceGameTime()
{
   uint32 interval = 0;

   if (isGameStarted())
   {
      // should we skip a frame rather than update?
      LARGE_INTEGER time;
      QueryPerformanceCounter(&time);

      int64 delta = static_cast<int64>(time.QuadPart - mLastUpdateTime);
      double diff = static_cast<double>(delta/mTimerFrequencyFloat);

      if (mSubUpdating || diff >= mAccumulatedTimeDelta+mLastUpdateInterval)
      {
         uint32 otime = getRecvTime();
         uint32 ntime = advanceRecvTime();
         if (ntime > otime)
         {
            // don't use the interval from the network, use the interval based on the recv update interval
            interval = ntime - otime;
            float finterval = static_cast<float>(interval)*0.001f;
            mAccumulatedTimeDelta += mLastUpdateInterval-diff;
            mAccumulatedTimeDelta = min(mAccumulatedTimeDelta, finterval);
            mAccumulatedTimeDelta = max(mAccumulatedTimeDelta, -finterval);
            mLastUpdateTime = time.QuadPart;
            mLastUpdateInterval = finterval;

            // this is to catch the case where we've changed our earliest allowed recv time
            mTimingLog.add(BTimingLogEntry::cAdvanceTime, ntime, mRecvUpdateInterval, mEarliestAllowedRecvTime, mEarliestClientTimeIndex, interval);

            return interval;
         }
         else if (ntime < otime)
         {
            BDEBUG_ASSERTM(0, "BTimeSync::advanceGameTime -- New recv time is less than the old recv time");
         }
         else
            mRecvTimeDirty = true; // force a new time calculation if I failed to advance
      }
   }

   return interval;
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::checkForClientDisconnect(BMachineID machineID)
{
   static BOOL alreadyChecking = FALSE;

   if (alreadyChecking)
      return;

   alreadyChecking = TRUE;

//-- FIXING PREFIX BUG ID 7496
   const BMachine* pMachine = mpSession->getMachine(machineID);
//--
   if (pMachine == NULL)
   {
      alreadyChecking = FALSE;
      return;
   }

   uint32 connectedMachines = 0;
   for (int i=0; i < mpSession->getMachineCount(); ++i)
   {
      if (i == machineID)
         continue;

      if (mpSession->getMachine(i) != NULL && mpSession->getMachine(i)->isConnected())
         connectedMachines |= (1<<i);
   }

   // if all clients have reported that we can disconnect the given client
   // AND we're the client causing time to stall
   // AND we have no more timings available
   // AND we're unable to advance time forward, then finish the disconnect here
   //
   // this hopefully insures that we exhaust all the potential timing data for the disconnecting client
   if (connectedMachines > 0 &&
       shouldDisconnect(machineID, connectedMachines) &&
       mEarliestClientTimeIndex == machineID &&
       mLastClientTimeHistory[machineID].availableTimes() == 0 &&
       !simpleCanAdvanceTime())
   {
      nlog(cTimeSyncNL, "BTimeSync::checkForClientDisconnect -- machineID[%ld], connectedClients[0x%08x]", machineID, connectedMachines);

      // this will disconnect the client from time sync updates
      mClientState[machineID] &= ~(1 << cTimeState);

      mpSession->finishClientDisconnect(machineID);

      // insure that we no longer processing timing history values for this machine
      //mClientTimingHistory[machineID].setValid(false);

      resetClient(machineID);

      if (mClientState[machineID] >> cSyncState & 1)
         --mNumUnsyncedClients;
   }

   alreadyChecking = FALSE;
}

//==============================================================================
// 
//==============================================================================
void BTimeSync::sessionConnected()
{
   // called when I'm hosting or when I've been accepted via a join response OK from the host

   // XXX WARNING
   // due to the new startup, this method actually does nothing because my session is connected
   // when the host sends me a join OK response which means that the join OK response will
   // be received and processed before any time packets and so the host's last time value better be 0
   // or something is f'd with our ordering code

   if (mSessionConnected)
      return;

   mSessionConnected = true;

   nlog(cTimeSyncNL, "BTimeSync::sessionConnected -- time rolling[%d], mSessionConnected[%d]", mTimeRolling, mSessionConnected);

   // why are we getting this event if it isn't connected?
   //if (!mpSession->isConnected())
   //{
   //   BASSERTM(FALSE, "BTimeSync::sessionConnected -- BSession is not connected, are you shutting down?");
   //   return;
   //}

   // if we are magically rolling time somehow, we really don't need to do anything
   if (isTimeRolling())
      return;

   // here is where we lie to everyone and tell them our time. In theory it is ok to lie here,
   // because all of the other clients are connected by the time we get the session connected call.
   // So we'll lie to everyone and tell them we're going to use the hosts time, and then in turn they'll
   // recalculate the next time interval and send it out and everyone will be happy. Right? I hope to hell so.
   //
   // we shouldn't be able to tell people a time in the past because the host will still stall waiting for us
   // until he gets this message we are about to send right here ya. If we are waiting on other people to send us 
   // their time, then it is likely they are connecting at the same time we are and will end up sending us the host
   // time as their time and we'll take it and be happy and all that.
   //
   // XXX actually, this should never be called because the last client time from the host better be 0
   if (mLastClientTimeHistory[mpSession->getHostMachineID()].getLast() > 0)
   {
      // do I want to send the last time received from the host or the earliest calculated time? XXX TEST
      uint32 timing = getLocalTiming();
      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket, mLastClientTimeHistory[mpSession->getHostMachineID()].getLast(), timing);
      mpChannel->SendPacket(packet);

      // init all connected clients
      for (int i=0; i < mpSession->getMachineCount(); ++i)
      {
         // we're now ready to start sending timing data to this client
         if (mpSession->getMachine(i) && mpSession->getMachine(i)->isConnected())
            mClientState[i] |= 1 << cTimeState;
      }

      nlog(cTimeSyncNL, "BTimeSync::sessionConnected -- sending out fake time init to get the ball rolling.");
   }
}

//==============================================================================
// This function turns on/off a bit that represents the client disconnect report
// Once every client that is connected has reported the disconnect, it is safe to 
// disconnect them.
//==============================================================================
void BTimeSync::clientDisconnectReported(uint32 fromClientID, uint32 forClientID, bool add)
{
   if (fromClientID >= XNetwork::cMaxClients)
      return;
   if (forClientID >= XNetwork::cMaxClients)
      return;

   if (add)
      mDisconnectReports[forClientID] |= (1 << fromClientID);
   else
      mDisconnectReports[forClientID] &= ~(1 << fromClientID);

   nlog(cTimeSyncNL, "BTimeSync::clientDisconnectReported -- disconnect reported by [%d] for client [%d], reports[0x%x]", fromClientID, forClientID, mDisconnectReports[forClientID]);
}

//==============================================================================
// 
//==============================================================================
bool BTimeSync::shouldDisconnect(uint32 clientID, uint32 connectedClients)
{
   if (clientID >= XNetwork::cMaxClients)
      return false;

   nlog(cTimeSyncNL, "BTimeSync::shouldDisconnect -- disconnect report [0x%x] and connected clients [0x%x]", mDisconnectReports[clientID], connectedClients);
   return ((mDisconnectReports[clientID] & connectedClients) == connectedClients);
}

//==============================================================================
// Resets the client time history and disconnect reports.
// Do not call this unless you know what you're doing.
//==============================================================================
void BTimeSync::resetClient(uint32 clientID)
{
   if (clientID >= XNetwork::cMaxClients)
      return;

   mLastClientTimeHistory[clientID].reset();
   mClientTimingHistory[clientID].reset();

   mDisconnectReports[clientID] = 0;
}

//==============================================================================
// 
//==============================================================================
BClientTimeHistory::BClientTimeHistory() :
   mInsertIndex(0),
   mEarliestIndex(0),
   mLastTime(0),
   mEarliestTime(0)
{

#ifndef BUILD_FINAL
   long maxHistory = 0;
   if (gConfig.get(cConfigClientTimeHistory, &maxHistory))
   {
      mLimitHistory = true;
      mMaxHistory = static_cast<uint32>(maxHistory);
      mMaxHistory = min(mMaxHistory, BClientTimeHistory::cMaxClientTimeHistorySize);
   }
   else
   {
      mLimitHistory = false;
      mMaxHistory = BClientTimeHistory::cMaxClientTimeHistorySize;
   }
#endif

   Utils::FastMemSet(mClientTime, 0, sizeof(uint32)*BClientTimeHistory::cMaxClientTimeHistorySize);
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::addInterval(uint8 interval)
{
   // the problem here is that I could process a peer client disconnect and then
   // process an interval packet from that disconnected client
   // shouldn't I be prevented from that?
   //
   // if not, then I could potentially add an interval onto a fixed time history

   mChanged = TRUE;

   // this will need to add the interval to the previously received time
   mLastTime += interval;

   mClientTime[mInsertIndex] = mLastTime;

   if (mInsertIndex == mEarliestIndex)
      mEarliestTime = mLastTime;

#ifndef BUILD_FINAL
   if (++mInsertIndex >= mMaxHistory)
      mInsertIndex = 0;
#else
   if (++mInsertIndex >= BClientTimeHistory::cMaxClientTimeHistorySize)
      mInsertIndex = 0;
#endif
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::addTime(uint32 time)
{
   mChanged = TRUE;

   mLastTime = time;

   mClientTime[mInsertIndex] = mLastTime;

   if (mInsertIndex == mEarliestIndex)
      mEarliestTime = mLastTime;

#ifndef BUILD_FINAL
   if (++mInsertIndex >= mMaxHistory)
      mInsertIndex = 0;
#else
   if (++mInsertIndex >= BClientTimeHistory::cMaxClientTimeHistorySize)
      mInsertIndex = 0;
#endif
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::reset(uint32 time)
{
   mChanged = TRUE;

   Utils::FastMemSet(mClientTime, 0, sizeof(uint32)*BClientTimeHistory::cMaxClientTimeHistorySize);

   mEarliestTime = time;
   mLastTime = time;
   mInsertIndex = (time == 0 ? 0 : 1);
   mEarliestIndex = 0;
   mClientTime[0] = time;

#ifndef BUILD_FINAL
   if (mMaxHistory == 0)
      mInsertIndex = 0;
#endif
}

//==============================================================================
// Return the number of client times we have available to us
//==============================================================================
uint32 BClientTimeHistory::availableTimes() const
{
   if (mInsertIndex > mEarliestIndex)
      return mInsertIndex - mEarliestIndex - 1;
   else if (mInsertIndex == 0 && mEarliestIndex == 0) // if the insert and earliest indexes are both 0, there are no times available
      return 0;

#ifndef BUILD_FINAL
   if (mMaxHistory == 0)
      return 1;
   return (mMaxHistory - mEarliestIndex) + mInsertIndex - 1;
#else
   return (BClientTimeHistory::cMaxClientTimeHistorySize - mEarliestIndex) + mInsertIndex - 1;
#endif
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::advance(uint32 amount)
{
   mChanged = TRUE;

   // advance our time history by the given amount
   mEarliestIndex += amount;

   // check for the wrap
#ifndef BUILD_FINAL
   if (mEarliestIndex >= mMaxHistory)
      mEarliestIndex = (mMaxHistory > 0 ? mEarliestIndex - mMaxHistory : 0);
#else
   if (mEarliestIndex >= BClientTimeHistory::cMaxClientTimeHistorySize)
      mEarliestIndex = mEarliestIndex - BClientTimeHistory::cMaxClientTimeHistorySize;
#endif

   mEarliestTime = mClientTime[mEarliestIndex];
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::update(const BClientTimeHistory& history)
{
   // fill in our history gaps with those from a peer client disconnect packet
   // we will only take times that are greater than our own times
   //
   // if we're further ahead, then don't bother
   if (history.mLastTime <= mLastTime)
      return;

   // we have gaps in our client time history, need to fill in what we can
   uint32 x = history.mEarliestIndex;
   while (x != history.mInsertIndex)
   {
      if (history.mClientTime[x] > mLastTime)
         addTime(history.mClientTime[x]);

#ifndef BUILD_FINAL
      if (++x >= mMaxHistory)
         x = 0;
#else
      if (++x >= BClientTimeHistory::cMaxClientTimeHistorySize)
         x = 0;
#endif
   }
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::serialize(BSerialBuffer& sb)
{
   BSerializable::serialize(sb);

   // if my insert index is 0 then I'll be wrapping around to 
   // if my currentIndex == mEarliestIndex, then I'm all caught up
   // and only need to serialize one value
   uint32 a = availableTimes();

   // the number of available times excludes the earliest time
   // however when we want to notify other clients of this history
   // we need to include all known times
   if (a != 0)
      a -= 1;

   sb.add(a);

   for (uint32 i = 0, x = mEarliestIndex; i < a; ++i)
   {
      sb.add(mClientTime[x]);
#ifndef BUILD_FINAL
      if (++x >= mMaxHistory)
         x = 0;
#else
      if (++x >= BClientTimeHistory::cMaxClientTimeHistorySize)
         x = 0;
#endif
   }
}

//==============================================================================
// 
//==============================================================================
void BClientTimeHistory::deserialize(BSerialBuffer& sb) 
{ 
   BSerializable::deserialize(sb);

   nlog(cTimingNL, "BClientTimeHistory::deserialize");

   uint32 a=0;
   sb.get(&a);

   // NOTE: This does not preserve exact positioning of elements in the circular array (though it does preserve ordering)
   for (uint32 x=0; x < a; ++x)    
   {
      uint32 t;
      sb.get(&t);

      mClientTime[x] = t;

      mLastTime = t;

      nlog(cTimingNL, "BClientTimeHistory::deserialize -- mClientTime[%ld] = %ld", x, mClientTime[x]);
   }

   mEarliestTime = mClientTime[0];
   mEarliestIndex = 0;
   mInsertIndex = x;

#ifndef BUILD_FINAL
   if (mInsertIndex >= mMaxHistory)
      mInsertIndex = 0;
#else
   if (mInsertIndex >= BClientTimeHistory::cMaxClientTimeHistorySize)
      mInsertIndex = 0;
#endif

   nlog(cTimingNL, "BClientTimeHistory::deserialize -- mInsertIndex = %ld", mInsertIndex);
}

//==============================================================================
// 
//==============================================================================
void BTimingHistory::timeAdvanced(uint32 newRecvTime)
{
   // loop through all timing records and find the most recent timing that does
   // not exceed the given recv time 
   //
   // since we reset all timing values upon game start, we need to check for
   // the most recent timing being set to zero
   //
   // NOTE: timeAdvanced should only be called when we've received timing data
   // from ALL clients otherwise we'll be operating on incomplete timing data sets
   while (mRecordsTail != mRecordsHead && (mTimingRecords[mRecordsTail].mSendTime < newRecvTime || mMostRecentTiming == 0))
   {
      mMostRecentSendTime = mTimingRecords[mRecordsTail].mSendTime;
      mMostRecentTiming = mTimingRecords[mRecordsTail].mTiming;

      if (++mRecordsTail >= cMaxTimingRecords)
         mRecordsTail = 0;
   }

   nlog(cTimingNL, "BTimingHistory::timeAdvanced -- newRecvTime %ld, mMostRecentTiming %ld", newRecvTime, mMostRecentTiming);
}

//==============================================================================
// 
//==============================================================================
//void BTimingHistory::updateTiming(int32 timing, uint32 clientSendTime)
//{
//   // does a timing entry for this already exist? (if so, return)
//   for (uint32 i=0; i < cMaxTimingRecords; i++)
//   {
//      if ((mTimingRecords[i].mSendTime == clientSendTime) && (mTimingRecords[i].mTiming == timing))
//         return;
//   }
//
//   // if not, add it
//   addTiming(timing, clientSendTime);
//}

//==============================================================================
// 
//==============================================================================
void BTimingHistory::addTiming(int32 timing, uint32 clientSendTime)
{
   nlog(cTimingNL, "BTimingHistory::addTiming -- timing:%ld:clientSendTime:%ld", timing, clientSendTime);

   // don't bother with this clamping, I'm already doing it on the outbound
   //timing = min(timing, 255); // clamp to unsigned char max
   timing = max(timing, BTimeSync::cMinServiceInterval); // clamp to min interval

   // the problem here is that I might receive a time value for something that
   // comes before my history of values?  Or is that even possible?
   // what would cause that?
   //
   // I need to insure that I am NOT adding a timing value that's older than
   // my current timing value.  This could happen if a client disconnects
   // and I receive a disconnect packet from another (still connected) client
   // with the disconnected client's timing history and one of the timing history
   // values falls outside my history window but is still less than my current time
   //
   int32 prevIndex = mRecordsHead - 1;
   if (prevIndex < 0)
      prevIndex = cMaxTimingRecords - 1;

   // I need to ignore additions that are from the past
   if (mTimingRecords[prevIndex].mSendTime >= clientSendTime)
      return;

   mTimingRecords[mRecordsHead].mTiming = static_cast<uint8>(timing);
   mTimingRecords[mRecordsHead].mSendTime = clientSendTime;

   if (++mRecordsHead >= cMaxTimingRecords)
      mRecordsHead = 0;
   if (mRecordsHead == mRecordsTail)
      mRecordsTail++;
   if (mRecordsTail >= cMaxTimingRecords)
      mRecordsTail = 0;

   mLastReportedTime = timeGetTime(); // the last realtime that we heard timing information from a client.
}

//==============================================================================
// 
//==============================================================================
BTimingRecord* BTimingHistory::getTiming(uint32 index)
{
   if (index >= cMaxTimingRecords)
   {
      BASSERT(0);
      return 0;
   }

   index += mRecordsTail;
   if (index >= cMaxTimingRecords)
      index = index - cMaxTimingRecords;

   return &mTimingRecords[index];
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingHistory::getTimingAmount() const
{
   uint32 a;
   if (mRecordsHead >= mRecordsTail)
      a = mRecordsHead-mRecordsTail;
   else 
      a = (cMaxTimingRecords-mRecordsTail)+mRecordsHead;

   return a;
}

//==============================================================================
// 
//==============================================================================
void BTimingHistory::reset()
{
   mRecordsHead = 0;
   mRecordsTail = 0;
   mMostRecentTiming = 0;
   mLastReportedTime = 0;

   mValid = false;

   for (uint32 i=0; i < cMaxTimingRecords; i++)
   {
      mTimingRecords[i].mTiming = 0;
      mTimingRecords[i].mSendTime = 0;
   }
}

//==============================================================================
// 
//==============================================================================
void BTimingHistory::serialize(BSerialBuffer &sb)
{
   BSerializable::serialize(sb);

   uint32 a = getTimingAmount();

   sb.add(a);

   uint32 p = mRecordsTail;

   while (p != mRecordsHead)
   {
      mTimingRecords[p].serialize(sb);
      if (++p >= cMaxTimingRecords)
         p = 0;
   }

   sb.add(mMostRecentTiming);
}

//==============================================================================
// 
//==============================================================================
void BTimingHistory::deserialize(BSerialBuffer &sb) 
{ 
   BSerializable::deserialize(sb);

   nlog(cTimingNL, "BTimingHistory::deserialize");

   mRecordsHead = mRecordsTail = 0;
   
   uint32 a=0;
   sb.get(&a);
   
   // NOTE: This does not preserve exact positioning of elements in the circular array (though it does preserve ordering)
   for (uint32 x=0; x < a; x++)    
   {
      mTimingRecords[x].deserialize(sb);
      nlog(cTimingNL, "BTimingHistory::deserialize -- mTimingRecords[%ld] mTiming %ld, mSendTime %ld", x, mTimingRecords[x].mTiming, mTimingRecords[x].mSendTime);
   }

   mRecordsHead = a;
   nlog(cTimingNL, "BTimingHistory::deserialize -- mRecordsHead %ld", mRecordsHead);

   sb.get(&mMostRecentTiming);
   nlog(cTimingNL, "BTimingHistory::deserialize -- mMostRecentTiming %ld", mMostRecentTiming);
}
