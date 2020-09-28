//==============================================================================
// Ping.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "PacketTypes.h"
#include "Packet.h"
//#include "NetPackets.h"
//#include "Ping.h"
#include "ObserverList.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BPingPongPacket : public BTypedPacket
{
   public:
      BPingPongPacket() : BTypedPacket(BPacketType::cPingPacket), mID(0), mSentTime(0), mInUse(false) {}
      BPingPongPacket(int32 type) : BTypedPacket(type), mID(0), mSentTime(0), mInUse(true) {}
      BPingPongPacket(int32 type, uint8 ID, uint32 sentTime) : BTypedPacket(type), mID(ID), mSentTime(sentTime), mInUse(true) {}
   
      virtual void               serialize(BSerialBuffer &sb) { BTypedPacket::serialize(sb); sb.add(mID); }
      virtual void               deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mID); }

   public:

      uint32                     mSentTime; // not transmitted
      uint8                      mID;
      bool                       mInUse : 1; // not transmitted
};

//==============================================================================
// Note: All times are in milliseconds
class BPing
{
   public:
      //class BPingObserver
      //{
      //   public:
      //      virtual void               pingUpdated() {}   
      //      virtual void               pingNotResponding() {}            
      //};

      //class BPingTransport
      //{
      //   public:
      //      virtual void               sendPingPacket(BPacket &packet) = 0;
      //};

  //    class BPingObserverList :
		//	public BObserverList <BPingObserver>
		//{
		//	DECLARE_OBSERVER_METHOD (pingUpdated,
		//		(void),
		//		())
  //       DECLARE_OBSERVER_METHOD (pingNotResponding,
		//		(void),
		//		())
		//}; 
      
      //void                       addObserver(BPingObserver *o) { mObserverList.Add(o); }
      //void                       removeObserver(BPingObserver *o) { mObserverList.Remove(o); } 
      //void                       setTransport(BPingTransport *transport) { mTransport = transport; }

      // Constructors
      BPing();

      // Destructors
      virtual ~BPing();

      // Functions

      uint32                     getPing() const { return getAveragePing(); }
      uint32                     getAveragePing() const { return mAveragePing; }
      uint32                     getStandardDeviation() const { return mPingStandardDeviation; }
      uint32                     getLastPing();

      // used to ignore pings during game load
      //void                       disablePing(bool v) { mDisable = v; }
      //void                       invalidateOutstandingPings();

      void                       dataAvailable(const void* data, const long size);

      void                       init(BEventReceiverHandle sessionEventHandle, const SOCKADDR_IN& addr);
      void                       service();
      bool                       query(uint8& id);
      void                       update(uint8 id, uint16 processingDelta, uint32 recvTime);

      void                       setAddr(const SOCKADDR_IN& addr) { mAddr = addr; }

      // Variables   

   private:
      enum
      {
         cPingTimeArraySize = 20,
         cPingDeviationArraySize = cPingTimeArraySize,
         cSentPingArraySize = 256,
         cPingInterval = 1000,
         cMaxPing = 2000
      };

      // Functions

      uint32                     calcNewPingAverage(uint32 newPing);
      uint32                     calcNewPingStandardDeviation(uint32 newPing);

      // Variables

      //BPingObserverList          mObserverList;

      BPingPongPacket            mSentPingArray[cSentPingArraySize];             // 9216

      uint32                     mPingDeviationArray[cPingDeviationArraySize];   // 80 - circular array
      uint32                     mPingTimeArray[cPingTimeArraySize];             // 80 - circular array

      SOCKADDR_IN                mAddr;                                          // 16

      uint32                     mLastSentPingTime;                              // 4

      int32                      mLastRecvPingIndex;                             // 4
      int32                      mSentPingArrayIndex;                            // 4

      uint32                     mPingTimeArrayIndex;                            // 4

      uint32                     mAveragePing;                                   // 4
      uint32                     mPingSum;                                       // 4
      uint32                     mPingDeviationArrayIndex;                       // 4
      uint32                     mPingStandardDeviation;                         // 4

      //BPingTransport*            mTransport;

      BEventReceiverHandle       mSessionEventHandle;                            // 8

      //bool                       mDisable : 1;
}; // BPing
