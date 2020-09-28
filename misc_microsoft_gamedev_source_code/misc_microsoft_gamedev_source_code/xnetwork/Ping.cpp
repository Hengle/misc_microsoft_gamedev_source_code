//==============================================================================
// Ping.cpp
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "Ping.h"
#include "Client.h"
#include "Session.h"
#include "NetEvents.h"

// xcore
#include "threading\eventDispatcher.h"
#include "threading\workDistributor.h"

//==============================================================================
//
//==============================================================================
BPing::BPing() :
   mPingTimeArrayIndex(0),
   mLastRecvPingIndex(0),
   mSentPingArrayIndex(0),
   mPingDeviationArrayIndex(0),
   mLastSentPingTime(0),
   mPingSum(0),
   mAveragePing(0),
   //mTransport(0),
   mPingStandardDeviation(0),
   mSessionEventHandle(cInvalidEventReceiverHandle)
   //mDisable(false)
{
   Utils::FastMemSet(mPingDeviationArray, 0, sizeof(uint32)*cPingDeviationArraySize);
   Utils::FastMemSet(mPingTimeArray, 0, sizeof(uint32)*cPingTimeArraySize);
   Utils::FastMemSet(&mAddr, 0, sizeof(SOCKADDR_IN));
}

//==============================================================================
//
//==============================================================================
BPing::~BPing()
{
}

//==============================================================================
//
//==============================================================================
void BPing::init(BEventReceiverHandle sessionEventHandle, const SOCKADDR_IN& addr)
{
   mSessionEventHandle = sessionEventHandle;
   mAddr = addr;
}

//==============================================================================
//
//==============================================================================
void BPing::service()
{
   BDEBUG_ASSERTM(false, "BPing::service -- deprecated");
   //if (mDisable)
   //   return;

   //// send ping out if the time is right
   //if ((timeGetTime() - mLastSentPingTime) > cPingInterval)
   //{
   //   if (mSentPingArray[mSentPingArrayIndex].mInUse)
   //   {
   //      // client isn't responding!
   //      mObserverList.pingNotResponding(); // notify
   //      mSentPingArray[mSentPingArrayIndex].mInUse = false;

   //      // if we've wrapped on our sent pings, then attempt to keep the last recv ping one step ahead
   //      // even though we haven't received one, it keeps the circular buffer intact
   //      if (mLastRecvPingIndex == mSentPingArrayIndex && ++mLastRecvPingIndex == cSentPingArraySize)
   //         mLastRecvPingIndex = 0;
   //   }

   //   if (mTransport)
   //   {
   //      // create ping packet
   //      uint32 sentTime = timeGetTime();
   //      BASSERT(mSentPingArrayIndex < cSentPingArraySize);

   //      BPingPongPacket& packet = mSentPingArray[mSentPingArrayIndex];

   //      packet.mID = static_cast<uint8>(mSentPingArrayIndex);
   //      packet.mSentTime = sentTime;
   //      packet.mInUse = true;

   //      if (++mSentPingArrayIndex == cSentPingArraySize)
   //         mSentPingArrayIndex = 0; // circular array

   //      mLastSentPingTime = sentTime;

   //      // send ping
   //      mTransport->sendPingPacket(packet);
   //   }
   //}
}

//==============================================================================
// 
//==============================================================================
bool BPing::query(uint8& id)
{
   // determine if we need to send another ping
   //if ((timeGetTime() - mLastSentPingTime) > cPingInterval)
   //{

      // I don't care about this, pings will not determine responsiveness
      //if (mSentPingArray[mSentPingArrayIndex].mInUse)
      //{
      //   // client isn't responding!
      //   // need to notify back up the chain
      //   // the new way will be to send a message
      //   // use the s_addr piece alone with the event class and ping data
      //   gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, cNetEventPingNotResponding, mAddr.sin_addr.s_addr);

      //   //mSentPingArray[mSentPingArrayIndex].mInUse = false;

      //   // if we've wrapped on our sent pings, then attempt to keep the last recv ping one step ahead
      //   // even though we haven't received one, it keeps the circular buffer intact
      //   if (mLastRecvPingIndex == mSentPingArrayIndex && ++mLastRecvPingIndex == cSentPingArraySize)
      //      mLastRecvPingIndex = 0;
      //}

      // create ping packet
      uint32 sentTime = timeGetTime();
      BDEBUG_ASSERT(mSentPingArrayIndex < cSentPingArraySize);

      BPingPongPacket& packet = mSentPingArray[mSentPingArrayIndex];

      packet.mID = static_cast<uint8>(mSentPingArrayIndex);
      packet.mSentTime = sentTime;
      packet.mInUse = true;

      if (++mSentPingArrayIndex == cSentPingArraySize)
         mSentPingArrayIndex = 0; // circular array

      mLastSentPingTime = sentTime;

      id = packet.mID;

      return true;
   //}

   //return false;
}

//==============================================================================
// 
//==============================================================================
uint32 BPing::calcNewPingAverage(uint32 newPing)
{
   // subtract old ping from sum
   mPingSum -= mPingTimeArray[mPingTimeArrayIndex];
   // add in new ping
   mPingSum += newPing;
   // record new ping
   mPingTimeArray[mPingTimeArrayIndex++] = newPing;
   // circular array
   if (mPingTimeArrayIndex == cPingTimeArraySize)
      mPingTimeArrayIndex = 0;
   // return new average
   return mPingSum/cPingTimeArraySize;
}

//==============================================================================
// 
//==============================================================================
uint32 BPing::calcNewPingStandardDeviation(uint32 newPing)
{
   // record new deviation
   mPingDeviationArray[mPingDeviationArrayIndex++] = (mAveragePing < newPing ? newPing - mAveragePing : mAveragePing - newPing);
   // circular array
   if (mPingDeviationArrayIndex == cPingDeviationArraySize)
      mPingDeviationArrayIndex = 0;
   // calculate standard deviation
   uint32 variance = 0;
   for (uint32 i=0; i < cPingDeviationArraySize; i++)
      variance += mPingDeviationArray[i]*mPingDeviationArray[i];
   variance = variance / (cPingDeviationArraySize-1);

   return static_cast<uint32>(sqrtf(static_cast<float>(variance)));
}

//==============================================================================
// 
//==============================================================================
uint32 BPing::getLastPing()
{
   if (mPingTimeArrayIndex > 0)
      return mPingTimeArray[mPingTimeArrayIndex-1];
   else
      return mPingTimeArray[cPingTimeArraySize-1];
}

//==============================================================================
// Invalidate any outstanding pings that are waiting on the pong
//==============================================================================
//void BPing::invalidateOutstandingPings()
//{
//   for (uint32 i=0; i < cSentPingArraySize; i++)
//      mSentPingArray[i].mInUse = FALSE;
//}

//==============================================================================
// Called from the connection layer
//    The connection will own an instance of BPing
//    BClient will lose BPing and gain uint32 ping values
//==============================================================================
void BPing::update(uint8 id, uint16 processingDelta, uint32 recvTime)
{
   int32 start = mSentPingArrayIndex-1;
   if (start < 0)
      start = cSentPingArraySize - 1;
   int32 stop = mLastRecvPingIndex;

   for (int32 i=0, index=start; i < cSentPingArraySize && index != stop; i++, index--)
   {
      if (index < 0)
         index = cSentPingArraySize - 1;

      if (mSentPingArray[index].mInUse && (mSentPingArray[index].mID == id))
      {
         uint32 sentTime = mSentPingArray[index].mSentTime;

         mSentPingArray[index].mInUse = false;

         // calc new ping
         uint32 newPing = recvTime - sentTime;
         if (newPing >= processingDelta)
            newPing -= processingDelta;
         else
            newPing = 0;
         newPing = Math::Min<uint32>(cMaxPing, newPing);

//#ifndef BUILD_DEBUG
//         BNetIPString strAddr(mAddr);
//         nlogt(cTransportNL, "BPing::update ip[%s] recvTime[%d] sentTime[%d] delta[%d] ping[%d] start[%d] stop[%d]",
//            strAddr.getPtr(), recvTime, sentTime, processingDelta, newPing, start, stop);
//#endif

         mAveragePing = calcNewPingAverage(newPing);
         mPingStandardDeviation = calcNewPingStandardDeviation(newPing);

         mLastRecvPingIndex = index;

         // notify the session layer
         // the problem here is that I need the s_addr, ping average and std dev.
         // can I combine the average and std dev into a single uint32?
         // I'll cap out the pings to 65535, but the session layer will need to unwrap the two ping values into uint32
         uint32 pingValue = (mPingStandardDeviation << 16) | (mAveragePing & 0xFFFF);
         gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, cNetEventPingUpdate, mAddr.sin_addr.s_addr, pingValue);

         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BPing::dataAvailable(const void* data, const long size)
{
   BDEBUG_ASSERTM(false, "BPing::dataAvailable -- deprecated");
   //if (mDisable)
   //   return;

   //if (BTypedPacket::getType(data) == BPacketType::cPongPacket)
   //{
   //   BPingPongPacket packet(BPacketType::cPongPacket);
   //   packet.deserializeFrom(data, size);

   //   // efficiency step, try the last sent ping first
   //   int32 start = 0;
   //   int32 lastPing = mSentPingArrayIndex-1;
   //   if (lastPing < 0) lastPing = cSentPingArraySize-1;
   //   if (mSentPingArray[lastPing].mInUse && (mSentPingArray[lastPing].mID == packet.mID))
   //      start = lastPing;

   //   // find matching sent ping
   //   // if we're at the beginning of our ping array, we need to wrap around
   //   for (int32 i=0, index=start; i < cSentPingArraySize; i++, index--)
   //   {
   //      if (index < 0)
   //         index = cSentPingArraySize-1;

   //      if (mSentPingArray[index].mInUse && (mSentPingArray[index].mID == packet.mID))
   //      {
   //         uint32 sentTime = mSentPingArray[index].mSentTime;

   //         mSentPingArray[index].mInUse = FALSE;

   //         // check the index against the last recv ping index
   //         // the index needs to fall between the last and the sent indexes
   //         if ((mLastRecvPingIndex < mSentPingArrayIndex && index > mLastRecvPingIndex && index < mSentPingArrayIndex) ||
   //             (mLastRecvPingIndex > mSentPingArrayIndex && (index < mSentPingArrayIndex || index > mLastRecvPingIndex)))
   //         {
   //            // calc new ping
   //            uint32 time = timeGetTime();
   //            uint32 newPing = min(cMaxPing, time - sentTime);
   //            mAveragePing = calcNewPingAverage(newPing);
   //            mPingStandardDeviation = calcNewPingStandardDeviation(newPing);

   //            // notify client
   //            mObserverList.pingUpdated();

   //            mLastRecvPingIndex = index;
   //         }

   //         break;
   //      }
   //   }
   //} 
   //else if (BTypedPacket::getType(data) == BPacketType::cPingPacket)
   //{
   //   BPingPongPacket packet(BPacketType::cPingPacket);
   //   packet.deserializeFrom(data, size);
   //   // send pong
   //   BPingPongPacket pong(BPacketType::cPongPacket, packet.mID, packet.mSentTime);
   //   if (mTransport)
   //      mTransport->sendPingPacket(pong);
   //}
}
