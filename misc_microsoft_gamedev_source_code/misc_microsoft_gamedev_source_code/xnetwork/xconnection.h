//==============================================================================
// xconnection.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "NetEvents.h"
#include "Ping.h"
#include "xnetwork.h"

// xsystem
#include "poolable.h"

// xcore
#include "threading/synchronizedBlockAllocator.h"

class BXConnectionInterface;

//==============================================================================
struct BXNetBuffer
{
   enum { cBufSize = cMaxBufferSize };
   uchar       mBuf[cBufSize];   // 1264
   SOCKADDR_IN mAddr;            // (2+2+4+8) 16
   uint64      mReserved;        // 8
   uint32      mTime;            // 4
   uint        mSize;            // 4 ==  1296.0
};

typedef BSynchronizedBlockAllocator<BXNetBuffer, 256, false> BXNetBufferAllocator;

//==============================================================================
namespace BNetHeaderType
{
   enum
   { 
      cHello=0,
      cDisconnect,
      cData,
      cDataFragment,
      cDataFragmentDone,
      cResendRequest,
      cAck,
      cAckRemote,
      cFin,
      cRepeatedPacket,
      cVoicePacket,
      cPingPacket,
      cPongPacket,
      cProxyPacket,

      cNumberOfTypes 
   };
}

#pragma pack(push, 1)
//==============================================================================
struct BNetHeader
{
   uint8  type;
};

//==============================================================================
struct BNetSeqHeader : BNetHeader
{
   uint16 size;
   uint16 seqNum;
};

//==============================================================================
struct BNetPingHeader : BNetHeader
{
   uint8 id;
   uint16 processingDelta;
};

//==============================================================================
struct BNetVoiceHeader : BNetHeader
{
   uint16 size;
   uint64 xuid;
};

//==============================================================================
struct BNetResendRequest : BNetHeader
{
   enum { cMaxResendRequests = 255 };
   uint8 numberOfRequests;
};

//==============================================================================
struct BNetAck : BNetHeader
{
   uint16 seqNum;
};

//==============================================================================
struct BNetProxyHeader : BNetHeader
{
   uint64 fromID;
   uint64 toID;
   uint16 size;
};

#pragma pack(pop)

//==============================================================================
struct BPingRequest
{
   uint32 mRecvTime; // set to the current time, used to generate a value for how long it took to process the ping
   uint8 id;
};

//==============================================================================
struct BNetBuffer
{
   uint32      flags;               // going to replace NoResends with a flag so we can also piggy back things like unencrypted traffic
   uint32      maximumLength;       // Length of memory allocated for Buffer; do not exceed this, and do not alter MaximumLength
   uint32      length;              // Length of the data actually stored in Buffer.  Update after storing data in *Buffer.
   uint32      voiceLength;         // Length of the voice data stored in the Buffer.
   void*       pBuffer;             // Buffer for message storage
};

//==============================================================================
struct BNetSendBuffer : BNetBuffer
{
   uint        sendTime;
   uint        count;
   BNetHeader* pHeader;
};

//==============================================================================
struct BNetRecvBuffer
{
   uint16         seqNum;
   uint32         recvTime;
   uint32         size;
   int32          type;
   void*          pBuffer;
   SOCKADDR_IN    remoteAddress;
};

//============================================================================
class BXRecvBuffer : public IPoolable
{
   public:
      BXRecvBuffer();
      ~BXRecvBuffer();

      // IPoolable
      void onAcquire();
      void onRelease();
      DECLARE_FREELIST(BXRecvBuffer, 4);

      enum { cBufSize = cMaxBufferSize };
      uchar          mBuf[cBufSize];
      SOCKADDR_IN    mAddr;
      uint32         mRecvTime;
      uint16         mSize;
      uint16         mSeqNum;
      uint8          mType;
};

//============================================================================
class BXSendBuffer : public IPoolable
{
   public:
      BXSendBuffer();
      ~BXSendBuffer();

      // IPoolable
      void onAcquire();
      void onRelease();
      DECLARE_FREELIST(BXSendBuffer, 4);

      // subtracting 2 from the max buffer size because
      // we need to save room for the cbGameData value that
      // takes up the first two bytes of every VDP packet
      enum { cBufSize = cMaxBufferSize-2 };
      uchar          mBuf[cBufSize];
      uint32         mSentTime;
      uint16         mSize;
      uint16         mSeqNum;
      bool           mVoice : 1; // voice data is appended to game data
      bool           mProxied : 1;
};

//============================================================================
class BXResendRequest : public IPoolable
{
   public:
      BXResendRequest() : mSentTime(0), mSeqNum(0) {}
      ~BXResendRequest() {}

      // IPoolable
      void onAcquire();
      void onRelease() {}
      DECLARE_FREELIST(BXResendRequest, 4);

      uint32         mSentTime;
      uint16         mSeqNum;
};

//============================================================================
class BXNetDisconnect : public BEventPayload
{
   public:

      struct BXNetAddr
      {
         SOCKADDR_IN mAddr;
         bool mValid : 1;

         BXNetAddr() :
            mValid(true)
         {
            Utils::FastMemSet(&mAddr, 0, sizeof(SOCKADDR_IN));
         }
         BXNetAddr(const SOCKADDR_IN& addr) :
            mValid(true)
         {
            mAddr = addr;
         }
      };

      BXNetDisconnect()
      {
         mPacket[0] = 0;
         mPacket[1] = 1;
         mPacket[2] = BNetHeaderType::cDisconnect;
      }
      ~BXNetDisconnect() {}

      void addAddr(const SOCKADDR_IN& addr)
      {
         mAddrs.add(BXNetAddr(addr));
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      BDynamicNetArray<BXNetAddr> mAddrs;
      uchar mPacket[2+sizeof(BNetHeader)];
};

//============================================================================
class BXNetData : public BEventPayload
{
   public:
      BXNetData() : 
         mSize(0),
         mTime(0)
      {
         Utils::FastMemSet(mBuf, 0, sizeof(uchar)*cBufSize);
         Utils::FastMemSet(&mAddr, 0, sizeof(mAddr));
      }
      ~BXNetData() {}

      void init(const uchar* pData, uint size, const SOCKADDR_IN& addr, uint time)
      {
         BDEBUG_ASSERT(pData);
         BDEBUG_ASSERT(size <= cBufSize);
         Utils::FastMemCpy(mBuf, pData, size);
         mSize = size;
         mAddr = addr;
         mTime = time;
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         //gNetworkHeap.Delete(this);
         //delete this;
         HEAP_DELETE(this, gNetworkHeap);
      }

      enum { cBufSize = cMaxBufferSize };
      uchar                mBuf[cBufSize];
      SOCKADDR_IN          mAddr;
      uint                 mSize;
      uint                 mTime; // time the data was received
};

//============================================================================
//class BXNetVoice : public BEventPayload
//{
//   public:
//      BXNetVoice() :
//         mXuid(0),
//         mSize(0)
//      {
//         Utils::FastMemSet(mBuf, 0, sizeof(uchar)*cBufSize);
//         Utils::FastMemSet(&mAddr, 0, sizeof(mAddr));
//      }
//      ~BXNetVoice() {}
//
//      void init(const uchar* pData, uint size, const XUID xuid, const SOCKADDR_IN& addr)
//      {
//         BDEBUG_ASSERT(pData);
//         BDEBUG_ASSERT(size <= cBufSize);
//         Utils::FastMemCpy(mBuf, pData, size);
//         mSize = size;
//         mXuid = xuid;
//         mAddr = addr;
//      }
//
//      // BEventPayload
//      void deleteThis(bool delivered)
//      {
//         //gNetworkHeap.Delete(this);
//         //delete this;
//         HEAP_DELETE(this, gNetworkHeap);
//      }
//
//      enum { cBufSize = cMaxBufferSize };
//      uchar                mBuf[cBufSize];
//      SOCKADDR_IN          mAddr;
//      XUID                 mXuid;
//      uint                 mSize;
//};

//============================================================================
class BXConnection
{
   public:
      BXConnection();
     ~BXConnection();

      enum
      {
         cXConnEventInit = cNetEventFirstUser,
         cXConnEventDeinit,
         cXConnEventSend,

         cXConnEventTotal
      };

      //-- MAIN Thread 
      bool init(BXNetBufferAllocator* pAllocator, BEventReceiverHandle sessionHandle, BEventReceiverHandle connectionHandle, BVoiceInterface* pVoiceInterface, BEventReceiverHandle socketHandle, const SOCKADDR_IN& addr, BXConnectionInterface* pConnectionInterface, const XNADDR& localXnAddr, XNADDR* pRemoteXnAddr=NULL, SOCKADDR_IN* pProxyAddr=NULL);
      void enable();
      void disable();
      void deinit();

      void send(const uchar* pData, const long size, const uint flags=0);
      void voice(const XUID xuid, const uchar* pData, const long size);
      //void ping();
      void flush(bool force=false);

      void recv(const uchar* pData, const long size, const uint32 time);

      const SOCKADDR_IN& getAddr() const { return mAddr; }
      const SOCKADDR_IN& getProxyAddr() const { return mProxyAddr; }

      bool isMuted() const { return mMuted; }
      void setMute(bool mute) { mMuted = mute; }

      uint32 getLastRecvTime() const { return mLastRecvTime; }

      // telling this connnection that it needs to send data to the proxy addr instead of
      // the original addr
      void setProxy(const SOCKADDR_IN& addr, const XNADDR& xnAddr);
      void resetProxy();

      uint64 getUniqueID() const { return mUniqueID; }

      bool isEnabled() const { return mEnabled; }
      bool isProxied() const { return mProxied; }
      bool isRegistered() const { return mXNetRegistered; }

      void unregister();

   private:

      void setProxy(const SOCKADDR_IN& addr, uint64 uniqueID);

      void processRecv(BXNetData* pData);
      void handleTalkersPacket(BXNetData* pData);
      void handleVoiceHeadsetPacket(BXNetData* pData);

      enum { cQueueSize=65535/2 };
      enum { cResendRequestInterval = 200 };

      void clearSentList(uint16 ack);
      void calcResendRequests();
      void calcResendRequests(uint16 actualRemoteSeqNum);
      void addResendRequest(uint16 seqNum);

      uint16 calcDistance(uint16 seqNum1, uint16 seqNum2) const;

      void updateRecvdList();

      void setStopAcks(bool value);

      void sendProxyDisconnect(uint64 fromID, uint64 toID);

      enum { cMaxPongs = 4 };

      BPing mPingHandler;                                // 9440

      uchar mRecvFragmentBuffer[cMaxBufferSize];         // 1264

      uchar mDataBuffer[cMaxBufferSize];                 // 1264
      uchar mVoiceBuffer[cMaxBufferSize];                // 1264

      SOCKADDR_IN mAddr;                                 // 16
      SOCKADDR_IN mProxyAddr;                            // 16

      //BDynamicArray<BXResendRequest*> mResendList;
      //BDynamicArray<BXSendBuffer*> mSendList;
      //BDynamicArray<BXSendBuffer*> mPendingList;
      //BDynamicArray<BXRecvBuffer*> mRecvList;

      BPointerList<BXResendRequest> mResendList;         // 36
      BPointerList<BXSendBuffer>    mSendList;           // 36
      BPointerList<BXSendBuffer>    mPendingList;        // 36
      BPointerList<BXRecvBuffer>    mRecvList;           // 36

      BXConnection* mFlushList[XNetwork::cMaxClients];   // 24

      BPingRequest mPongs[cMaxPongs];

      BEventReceiverHandle mSessionEventHandle;          // 8
      BEventReceiverHandle mConnectionEventHandle;       // 8
      BEventReceiverHandle mVoiceEventHandle;            // 8
      BEventReceiverHandle mSocketEventHandle;           // 8

      // calculated from the XNADDR/IN_ADDR
      uint64 mLocalUniqueID;                             // 8
      uint64 mUniqueID;                                  // 8

      BVoiceInterface* mpVoiceInterface;                 // 4

      BVoiceBufferAllocator* mpVoiceAllocator;           // 4

      uint32 mLastRecvTime;                              // 4

      uint16 mLocalSeqNum;                               // 2
      uint16 mRemoteSeqNum;                              // 2
      uint16 mFinSeqNum;                                 // 2

      uint  mRecvFragmentBufferPtr;                      // 4
      uint  mRecvFragmentTime;                           // 4

      BXNetBufferAllocator* mpAllocator;                 // 4

      BXConnectionInterface* mpConnectionInterface;      // 4

      uint mPongCount;                                   // 4
      uint mPongIndex;                                   // 4

      uint mFlushCounter;                                // 4

      bool mMuted : 1;                                   // 1 (1/8)
      bool mProxied : 1;                                 //   (2/8)
      bool mEnabled : 1;                                 //   (3/8)
      bool mStopAcks : 1;                                //   (4/8)
      bool mStopAckRemote : 1;                           //   (5/8)
      bool mStopAckLocal : 1;                            //   (6/8)
      bool mXNetRegistered : 1;                          //   (7/8)

#ifndef BUILD_FINAL
      //
      // performance counters below added by bgoodman
      //   

   //public:
   //   DWORD getByteReceiveCount() const;
   //   DWORD getByteReceiveCountPerSecond() const;
   //   DWORD getPacketReceiveCount() const;
   //   DWORD getPacketResendRequestCount() const;
   //   DWORD getPacketResendRequestCountPerSecond() const;
   //   DWORD getInSequencePacketReceiveCountPerSecond() const;
   //   DWORD getInSequencePacketReceiveCount() const;
   //   DWORD getOutOfSequencePacketReceiveCountPerSecond() const;
   //   DWORD getOutOfSequencePacketReceiveCount() const;

   private:
      DWORD m_dwByteReceiveCount;
      DWORD m_dwPacketReceiveCount;
      DWORD m_dwPacketResendRequestCount;
      DWORD m_dwInSequencePacketReceiveCount;
      DWORD m_dwOutOfSequencePacketReceiveCount;
      DWORD m_dwPacketOneSecondDeltaBegin;
      DWORD m_dwPacketOneSecondDeltaEnd;

      enum { cAckTimeout = 60000 };
#else
      enum { cAckTimeout = 30000 };
#endif
};

//==============================================================================
class BXConnectionInterface
{
   public:
      virtual BXConnection* getConnection(uint64 id) = 0;
};
