//==============================================================================
// xnetwork.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

// xcore
#include "threading/synchronizedBlockAllocator.h"

namespace XNetwork
{
   enum
   {
      cMaxClients = 6,
      cMaxVoiceSendSize = 640,
      cNumDisconnectSends = 5,
      cDefaultPort = 1000,
      cDefaultProxyRequestTimeout = 8000,
      cDefaultProxyPingTimeout = 3000,
      cConnectionTimeout = 15000,
      cDefaultJoinRequestTimeout = 30000,

      c1v1UpdateInterval = 100,
      c2v2UpdateInterval = 166,
      c3v3UpdateInterval = 250,

      cMinPingForceFlush = 100, // the minimum ping required to enact forced flushes of the network connection in
                                // the event of a proxied connection or resend scenario

      // used for queueing channel data in the event that we need to sync the information
      // with other peers when a client disconnects
      cChannelQueueSize = 500,
      cMaxPacketDistance = 65535/2,
      cMaxPacketID = 65535,

      cLanDiscoveryPort = 1002,
      cLanBroadcastInterval = 3000,
      cLanListTimeout = cLanBroadcastInterval + 2000,

      cReducedUpdateInterval = 40,  // when entering a reduced update interval state, this number means that for
                                    // every X updates, we'll flush the connection
                                    // for example, if we're updating at 10Hz and this value is 10, then when the
                                    // connection detects a steady state, we'll switch to flushing once a second
   };
}

//==============================================================================
//
//==============================================================================
class BNetIPString : public BString
{
   public:
      BNetIPString(uint addr) :
         BString()
      {
         _format(addr);
      }

      BNetIPString(const IN_ADDR& addr) :
         BString()
      {
         _format(addr);
      }

      BNetIPString(const SOCKADDR_IN& addr) :
         BString()
      {
         _format(addr);
      }

      BNetIPString(const XNADDR& addr) :
         BString()
      {
         format("%d.%d.%d.%d/%d.%d.%d.%d:%d",
            addr.ina.S_un.S_un_b.s_b1, addr.ina.S_un.S_un_b.s_b2, addr.ina.S_un.S_un_b.s_b3, addr.ina.S_un.S_un_b.s_b4,
            addr.inaOnline.S_un.S_un_b.s_b1, addr.inaOnline.S_un.S_un_b.s_b2, addr.inaOnline.S_un.S_un_b.s_b3, addr.inaOnline.S_un.S_un_b.s_b4,
            addr.wPortOnline);
      }

   private:
      void _format(uint addr)
      {
         format("%d.%d.%d.%d", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
      }

      void _format(const IN_ADDR& addr)
      {
         format("%d.%d.%d.%d", addr.S_un.S_un_b.s_b1, addr.S_un.S_un_b.s_b2, addr.S_un.S_un_b.s_b3, addr.S_un.S_un_b.s_b4);
      }

      void _format(const SOCKADDR_IN& addr)
      {
         format("%d.%d.%d.%d:%d", addr.sin_addr.S_un.S_un_b.s_b1, addr.sin_addr.S_un.S_un_b.s_b2, addr.sin_addr.S_un.S_un_b.s_b3, addr.sin_addr.S_un.S_un_b.s_b4, addr.sin_port);
      }
};

//==============================================================================
struct BVoiceBuffer
{
   enum { cBufSize = XNetwork::cMaxVoiceSendSize };
   uchar       mBuf[cBufSize];   // 640
   XUID        mXuid;            // 8
   uint        mSize;            // 4
   uint16      mIndex;           // 2
   uint16      mReserved;        // 2 == 656

   BVoiceBuffer() : mXuid(0), mSize(0), mIndex(0), mReserved(0) {}

   BVoiceBuffer& operator=(const BVoiceBuffer& source)
   {
      if (this==&source)
         return *this;

      mXuid = source.mXuid;
      mSize = source.mSize;
      mIndex = source.mIndex;
      mReserved = source.mReserved;

      IGNORE_RETURN(Utils::FastMemCpy(mBuf, source.mBuf, cBufSize));

      return *this;
   }

   void init(const uchar* pData, uint size, const XUID xuid)
   {
      if (size > cBufSize)
         return;
      mXuid = xuid;
      mSize = size;
      mIndex = 0;
      //mReserved = 0;
      Utils::FastMemCpy(mBuf, pData, size);
   }
};

//==============================================================================
//typedef BSynchronizedBlockAllocator<BVoiceBuffer, 32, false> BVoiceBufferAllocator;

class BVoiceBufferAllocator
{
   public:
      BVoiceBuffer* alloc();
      void free(BVoiceBuffer* pBuf);

   private:
      BSynchronizedBlockAllocator<BVoiceBuffer, 32, false> mAllocator;
};

//==============================================================================
class BVoiceInterface
{
   public:
      virtual void                   addRef() { }
      virtual void                   release() { }
      virtual BVoiceBufferAllocator* getAllocator() { return NULL; }
      virtual BEventReceiverHandle   getEventHandle() const { return cInvalidEventReceiverHandle; }
};

//==============================================================================
// Used to communicate to the voice thread to init/deinit clients
//==============================================================================
class BVoiceRequestPayload : public BEventPayload
{
   public:
      BVoiceRequestPayload() :
         mXuid(0),
         mControllerID(XUSER_MAX_COUNT),
         mClientID(0),
         mChannel(0)
      {
      }
      ~BVoiceRequestPayload() {}

      void init(XUID xuid, uint channel=0)
      {
         mXuid = xuid;
         mChannel = channel;
      }

      void init(uint clientID, uint controllerID, XUID xuid, const SOCKADDR_IN& addr)
      {
         mClientID = clientID;
         mControllerID = controllerID;
         mXuid = xuid;
         mAddr = addr;
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      SOCKADDR_IN mAddr;
      XUID        mXuid;
      uint        mControllerID;
      uint        mClientID;
      uint        mChannel;
};

//==============================================================================
// 
//==============================================================================
class BVoiceMuteRequestPayload : public BEventPayload
{
   public:
      BVoiceMuteRequestPayload() {}
      ~BVoiceMuteRequestPayload() {}

      void init(const SOCKADDR_IN& addr, bool mute)
      {
         mAddr = addr;
         mMute = mute;
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      SOCKADDR_IN mAddr;
      bool mMute : 1;
};

//==============================================================================
// Let the party/game session layer send out the mute list
//==============================================================================
class BVoiceTalkerListPayload : public BEventPayload
{
   public:
      BVoiceTalkerListPayload() {}
      ~BVoiceTalkerListPayload() {}

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      XUID mXuids[XNetwork::cMaxClients];
      XUID mOwnerXuid;
      uint mCount;
};

//==============================================================================
// Dynamic arrays for the network heap, here for testing, until we're ready to
//    move them to the normal location
//==============================================================================
struct BDynamicArrayNetHeapAllocatorPolicy { BMemoryHeap& getHeap() const { return gNetworkHeap; } };

template<class ValueType, uint Alignment>
struct BDynamicArrayNetHeapAllocator : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArrayNetHeapAllocatorPolicy> { };

template<
   class ValueType, 
      uint Alignment                               = ALIGN_OF(ValueType), 
      template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
      template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicNetArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayNetHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   BDynamicNetArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

struct BNetFixedHeapAllocator : BFixedHeapAllocator<BDynamicArrayNetHeapAllocatorPolicy> { };
