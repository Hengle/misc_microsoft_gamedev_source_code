//==============================================================================
// xconnection.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

// xnetwork
#include "Precompiled.h"

#include "xconnection.h"
#include "xudpsocket.h"
#include "session.h"
#include "NetPackets.h"

// xcore
#include "threading\eventDispatcher.h"
#include "threading\workDistributor.h"

IMPLEMENT_FREELIST(BXRecvBuffer, 4, &gNetworkHeap);
IMPLEMENT_FREELIST(BXSendBuffer, 4, &gNetworkHeap);
IMPLEMENT_FREELIST(BXResendRequest, 4, &gNetworkHeap);

//==============================================================================
// 
//==============================================================================
BVoiceBuffer* BVoiceBufferAllocator::alloc()
{
   BVoiceBuffer* pBuf = mAllocator.alloc();
   if (pBuf != NULL)
   {
      pBuf->mReserved = 0;
      return pBuf;
   }

   //BASSERTM(pBuf, "Failed to allocate voice buffer");

   pBuf = HEAP_NEW(BVoiceBuffer, gNetworkHeap);
   pBuf->mReserved = 1;

   return pBuf;
}

//==============================================================================
// 
//==============================================================================
void BVoiceBufferAllocator::free(BVoiceBuffer* pBuf)
{
   if (pBuf == NULL)
      return;

   if (pBuf->mReserved == 0)
      mAllocator.free(pBuf);
   else
      HEAP_DELETE(pBuf, gNetworkHeap);
}

//==============================================================================
// 
//==============================================================================
BXRecvBuffer::BXRecvBuffer()
{
}

//==============================================================================
// 
//==============================================================================
BXRecvBuffer::~BXRecvBuffer()
{
}

//==============================================================================
// 
//==============================================================================
void BXRecvBuffer::onAcquire()
{
   IGNORE_RETURN(Utils::FastMemSet(mBuf, 0, sizeof(uchar)*cBufSize));
   mRecvTime = 0;
   mSize = 0;
   mType = 0;
   mSeqNum = 0;
}

//==============================================================================
// 
//==============================================================================
void BXRecvBuffer::onRelease()
{
}

//==============================================================================
// 
//==============================================================================
BXSendBuffer::BXSendBuffer()
{
}

//==============================================================================
// 
//==============================================================================
BXSendBuffer::~BXSendBuffer()
{
}

//==============================================================================
// 
//==============================================================================
void BXSendBuffer::onAcquire()
{
   IGNORE_RETURN(Utils::FastMemSet(mBuf, 0, sizeof(uchar)*cBufSize));
   mSentTime = 0;
   mSize = 0;
   mSeqNum = 0;
   mVoice = false;
   mProxied = false;
}

//==============================================================================
// 
//==============================================================================
void BXSendBuffer::onRelease()
{
}

//==============================================================================
// 
//==============================================================================
void BXResendRequest::onAcquire()
{
   mSentTime = 0;
   mSeqNum = 0;
}

//==============================================================================
// 
//==============================================================================
BXConnection::BXConnection() :
   mResendList(0, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mSendList(0, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mPendingList(0, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mRecvList(0, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mSessionEventHandle(cInvalidEventReceiverHandle),
   mConnectionEventHandle(cInvalidEventReceiverHandle),
   mVoiceEventHandle(cInvalidEventReceiverHandle),
   mSocketEventHandle(cInvalidEventReceiverHandle),
   mLocalUniqueID(0),
   mUniqueID(0),
   mpVoiceInterface(NULL),
   mpVoiceAllocator(NULL),
   mLastRecvTime(0),
   mLocalSeqNum(0),
   mRemoteSeqNum(0),
   mFinSeqNum(0),
   mRecvFragmentBufferPtr(0),
   mRecvFragmentTime(0),
   mpAllocator(NULL),
   mpConnectionInterface(NULL),
   mPongCount(0),
   mPongIndex(0),
   mFlushCounter(0),
   mMuted(true),
   mProxied(false),
   mEnabled(true),
   mStopAcks(false),
   mStopAckRemote(false),
   mStopAckLocal(false),
   mXNetRegistered(false)
{
   IGNORE_RETURN(Utils::FastMemSet(mRecvFragmentBuffer, 0, sizeof(uchar)*cMaxBufferSize));

   IGNORE_RETURN(Utils::FastMemSet(mDataBuffer, 0, sizeof(uchar)*cMaxBufferSize));
   IGNORE_RETURN(Utils::FastMemSet(mVoiceBuffer, 0, sizeof(uchar)*cMaxBufferSize));

   IGNORE_RETURN(Utils::FastMemSet(&mAddr, 0, sizeof(SOCKADDR_IN)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));

   IGNORE_RETURN(Utils::FastMemSet(mPongs, 0, sizeof(BNetPingHeader)*cMaxPongs));
}

//==============================================================================
// 
//==============================================================================
BXConnection::~BXConnection()
{
}

//==============================================================================
// 
//==============================================================================
bool BXConnection::init(BXNetBufferAllocator* pAllocator, BEventReceiverHandle sessionHandle, BEventReceiverHandle connectionHandle, BVoiceInterface* pVoiceInterface, BEventReceiverHandle socketHandle, const SOCKADDR_IN& addr, BXConnectionInterface* pConnectionInterface, const XNADDR& localXnAddr, XNADDR* pRemoteXnAddr, SOCKADDR_IN* pProxyAddr)
{
   mpAllocator = pAllocator;

   mAddr = addr;

   mXNetRegistered = true;

   mSessionEventHandle = sessionHandle;
   mConnectionEventHandle = connectionHandle;
   mSocketEventHandle = socketHandle;

   mpVoiceInterface = pVoiceInterface;
   if (pVoiceInterface != NULL)
   {
      mpVoiceAllocator = pVoiceInterface->getAllocator();
      mVoiceEventHandle = pVoiceInterface->getEventHandle();
   }

   mpConnectionInterface = pConnectionInterface;

   mLocalUniqueID = BSession::getUniqueID(localXnAddr);

   if (pRemoteXnAddr == NULL)
   {
      XNADDR xnaddr;
      if (XNetInAddrToXnAddr(addr.sin_addr, &xnaddr, NULL) != 0)
      {
#ifndef BUILD_FINAL
         // asserting on the connection thread (sim helper) causes lockups to occur
         //BASSERTM(0, "Failed to derive the XNADDR from the given IN_ADDR");
         BNetIPString strAddr(mAddr);
         BNetIPString strProxyAddr(mProxyAddr);

         nlogt(cTransportNL, "BXConnection::init -- ip[%s] proxy[%s]", strAddr.getPtr(), strProxyAddr.getPtr());
#endif
         // XXX not sure why this would fail, but it does sometimes
         // probably when we lose the secure connection just before I make the call
         return false;
      }

      mUniqueID = BSession::getUniqueID(xnaddr);
   }
   else
   {
      mUniqueID = BSession::getUniqueID(*pRemoteXnAddr);
   }

   if (pProxyAddr != NULL && pProxyAddr->sin_addr.s_addr != 0 && pProxyAddr->sin_addr.s_addr != addr.sin_addr.s_addr)
   {
      mProxied = true;
      mProxyAddr = *pProxyAddr;
   }

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);
   BNetIPString strProxyAddr(mProxyAddr);

   nlogt(cTransportNL, "BXConnection::init -- ip[%s] proxy[%s] localUniqueID[%I64u] uniqueID[%I64u]",
      strAddr.getPtr(), strProxyAddr.getPtr(),
      mLocalUniqueID, mUniqueID);
#endif

   mPongCount = 0;
   mPongIndex = 0;

   mPingHandler.init(mSessionEventHandle, addr);

   mLastRecvTime = timeGetTime();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BXConnection::enable()
{
   mEnabled = true;

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);
   BNetIPString strProxyAddr(mProxyAddr);

   nlogt(cSessionNL, "BXConnection::enable -- ip[%s] proxy[%s]",
      strAddr.getPtr(), strProxyAddr.getPtr());
#endif
}

//==============================================================================
// 
//==============================================================================
void BXConnection::disable()
{
   mEnabled = false;

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);
   BNetIPString strProxyAddr(mProxyAddr);

   nlogt(cSessionNL, "BXConnection::disable -- ip[%s] proxy[%s]",
      strAddr.getPtr(), strProxyAddr.getPtr());
#endif
}

//==============================================================================
// 
//==============================================================================
void BXConnection::deinit()
{
   BHandle hItem;

   // this should only be called from the context of the sim helper thread
   // (unless we've moved the BXConnection handler elsewhere)

   // mResendList
   BXResendRequest* pResend = mResendList.getHead(hItem);
   while (pResend)
   {
      BXResendRequest::releaseInstance(pResend);

      pResend = mResendList.getNext(hItem);
   }
   mResendList.reset();

   // mSendList
   BXSendBuffer* pSendBuf = mSendList.getHead(hItem);
   while (pSendBuf)
   {
      BXSendBuffer::releaseInstance(pSendBuf);

      pSendBuf = mSendList.getNext(hItem);
   }
   mSendList.reset();

   // mPendingList
   pSendBuf = mPendingList.getHead(hItem);
   while (pSendBuf)
   {
      BXSendBuffer::releaseInstance(pSendBuf);

      pSendBuf = mPendingList.getNext(hItem);
   }
   mPendingList.reset();

   // mRecvList
   BXRecvBuffer* pRecvBuf = mRecvList.getHead(hItem);
   while (pRecvBuf)
   {
      BXRecvBuffer::releaseInstance(pRecvBuf);

      pRecvBuf = mRecvList.getNext(hItem);
   }
   mRecvList.reset();
}

//==============================================================================
// 
//==============================================================================
void BXConnection::send(const uchar* pData, const long size, const uint flags)
{
   // don't bother with loopback checks here, wait until the flush to pull data off
   // the send list and issue events

   // allocate a buffer to store the incoming data in a send list
   // the flush method will examine the send list and attempt to coalesce as
   //    much data as possible into a single packet send
   // the resulting packet is stored in the sentlist until we receive an ack
   //    from the remote endpoint

   // can I use a freelist here?  I can't use a fixed list because what if we
   // have a sudden lag spike of sorts and we run out of available packets
   //
   // I need to use something that can grow
   //
   // provided the list is only used on a single thread, I can try a freelist

   // when I receive data on the udp thread, I need to packetize and send
   // to the session event handler

   // what about when I flush data to be sent to the udp thread

   //if (flags & cSendUnencrypted)
   //{
   //   // XXX this is temporary until the voice engine goes threaded
   //   // for now, we won't have access to the XUID
   //   // as it's already contained in the data payload
   //   voice(0, pData, size);
   //   return;
   //}

   BDEBUG_ASSERT(pData);
   BDEBUG_ASSERT(size <= BXSendBuffer::cBufSize);
   if (pData == NULL)
      return;
   if (size > BXSendBuffer::cBufSize)
      return;

   // if we're a loopback connection, redirect the data back up the chain
   if (mAddr.sin_addr.s_addr == INADDR_LOOPBACK)
   {
      mLastRecvTime = timeGetTime();

      BXNetData* pNetData = HEAP_NEW(BXNetData, gNetworkHeap);
      pNetData->init(pData, size, mAddr, mLastRecvTime);

      processRecv(pNetData);
      return;
   }

   if (flags & cSendProxy)
   {
      BXSendBuffer* pSendBuffer = BXSendBuffer::getInstance();
      BDEBUG_ASSERT(pSendBuffer);
      if (pSendBuffer == NULL)
         return;

      IGNORE_RETURN(Utils::FastMemCpy(pSendBuffer->mBuf, pData, size));

      pSendBuffer->mSeqNum = 0;
      pSendBuffer->mSize = static_cast<uint16>(size);
      pSendBuffer->mProxied = true;

#ifndef BUILD_FINAL
      BNetIPString strAddr(mAddr);
      nlogt(cTransportNL, "[SEND] proxy size[%d] to [%s]", size, strAddr.getPtr());
#endif

      mSendList.addToTail(pSendBuffer);
      return;
   }

   uchar* pDataPtr = const_cast<uchar*>(pData);

   if (size > BXSendBuffer::cBufSize - sizeof(BNetSeqHeader))
   {
      // fragment the data
      uint dataDone = 0;
      uint dataLeft = size - dataDone;
      uint sendSize = min(BXSendBuffer::cBufSize - sizeof(BNetSeqHeader), dataLeft);

      while (dataLeft > 0)
      {
         BXSendBuffer* pSendBuffer = BXSendBuffer::getInstance();
         BDEBUG_ASSERT(pSendBuffer);
         if (pSendBuffer == NULL)
            return;

         BNetSeqHeader* pSeqHeader = reinterpret_cast<BNetSeqHeader*>(pSendBuffer->mBuf);
         uchar* pSeqBuf = pSendBuffer->mBuf + sizeof(BNetSeqHeader);

         IGNORE_RETURN(Utils::FastMemCpy(pSeqBuf, pDataPtr + dataDone, sendSize));
         pSeqHeader->size = static_cast<uint16>(sendSize);
         pSeqHeader->seqNum = pSendBuffer->mSeqNum = mLocalSeqNum++;

         pSendBuffer->mSize = static_cast<uint16>(sendSize + sizeof(BNetSeqHeader));

         if (dataLeft - sendSize > 0)
            pSeqHeader->type = BNetHeaderType::cDataFragment;
         else
            pSeqHeader->type = BNetHeaderType::cDataFragmentDone;

#ifndef BUILD_FINAL
         BNetIPString strAddr(mAddr);
         nlogt(cTransportNL, "[SEND] fragment type[%d], size[%d], seq[%d] to [%s]", pSeqHeader->type, pSendBuffer->mSize, pSeqHeader->seqNum, strAddr.getPtr());
#endif

         mSendList.addToTail(pSendBuffer);

         dataDone += sendSize;
         dataLeft = size - dataDone;
         sendSize = min(BXSendBuffer::cBufSize - sizeof(BNetSeqHeader), dataLeft);
      }
   }
   // otherwise just send it out
   else
   {
      BXSendBuffer* pSendBuffer = BXSendBuffer::getInstance();
      BDEBUG_ASSERT(pSendBuffer);
      if (pSendBuffer == NULL)
         return;

      BNetSeqHeader* pSeqHeader = reinterpret_cast<BNetSeqHeader*>(pSendBuffer->mBuf);
      uchar* pSeqBuf = pSendBuffer->mBuf + sizeof(BNetSeqHeader);

      IGNORE_RETURN(Utils::FastMemCpy(pSeqBuf, pDataPtr, size));
      pSeqHeader->size = static_cast<uint16>(size);
      pSeqHeader->seqNum = pSendBuffer->mSeqNum = mLocalSeqNum++;

      pSendBuffer->mSize = static_cast<uint16>(size + sizeof(BNetSeqHeader));

      pSeqHeader->type = BNetHeaderType::cData;

#ifndef BUILD_FINAL
      BNetIPString strAddr(mAddr);
      nlogt(cTransportNL, "[SEND] type[%d], size[%d], seq[%d] to [%s]", pSeqHeader->type, pSendBuffer->mSize, pSeqHeader->seqNum, strAddr.getPtr());
#endif

      mSendList.addToTail(pSendBuffer);
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::voice(const XUID xuid, const uchar* pData, const long size)
{
   if (mMuted || !mEnabled)
      return;

   // XXX also account for the voice header
   BDEBUG_ASSERT(pData);
   BDEBUG_ASSERT(size <= BXSendBuffer::cBufSize - sizeof(BNetVoiceHeader));
   if (pData == NULL)
      return;
   if (size > BXSendBuffer::cBufSize - sizeof(BNetVoiceHeader))
      return;

   // if this is the loopback socket, then simply construct a voice payload and send to the voice layer
   if (mAddr.sin_addr.s_addr == INADDR_LOOPBACK)
   {
      if (mVoiceEventHandle == cInvalidEventReceiverHandle)
         return;

      //BXNetVoice* pNetVoice = new BXNetVoice();
      //BXNetVoice* pNetVoice = HEAP_NEW(BXNetVoice, gNetworkHeap);
      //pNetVoice->init(pData, size, xuid, mAddr);
      //gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cNetEventVoiceRecv, 0, 0, pNetVoice);
      if (size > XNetwork::cMaxVoiceSendSize)
         return;
      BVoiceBuffer* pVoice = mpVoiceAllocator->alloc();
      if (pVoice == NULL)
         return;
      pVoice->init(pData, size, xuid);
      // send this event from the session event handle
      // the session event handle is what's registered with the voice system
      // for distinguishing sessions
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceRecv, (uint)pVoice);
      return;
   }

   BXSendBuffer* pSendBuffer = BXSendBuffer::getInstance();
   BDEBUG_ASSERT(pSendBuffer);
   if (pSendBuffer == NULL)
      return;

   BNetVoiceHeader* pVoiceHeader = reinterpret_cast<BNetVoiceHeader*>(pSendBuffer->mBuf);
   uchar* pVoiceBuf = pSendBuffer->mBuf + sizeof(BNetVoiceHeader);

   IGNORE_RETURN(Utils::FastMemCpy(pVoiceBuf, pData, size));
   pVoiceHeader->size = static_cast<uint16>(size);

   pSendBuffer->mSeqNum = 0;
   pSendBuffer->mSize = static_cast<uint16>(size + sizeof(BNetVoiceHeader));
   pSendBuffer->mVoice = true;

   pVoiceHeader->xuid = xuid;
   pVoiceHeader->type = BNetHeaderType::cVoicePacket;

   mSendList.addToTail(pSendBuffer);
}

//==============================================================================
// 
//==============================================================================
//void BXConnection::ping()
//{
//   BXSendBuffer* pSendBuffer = BXSendBuffer::getInstance();
//   BDEBUG_ASSERT(pSendBuffer);
//   if (pSendBuffer == NULL)
//      return;
//
//   BNetPingHeader* pPingHeader = reinterpret_cast<BNetPingHeader*>(pSendBuffer->mBuf);
//
//   // integrate the Ping.cpp code into BXConnection
//   // OR add a new xping.cpp class that BXConnection instantiates
//   // to handle timings
//   //
//   // actually, I want the pings to be wrapped into the flush so if we have to send a ping
//   // it gets automatically included in the construction of the payload we're about to send
//   //
//   // should I leave enough room to potentially include a ping in every payload?
//   // A ping will be 1 byte in the new structure of things, so I could probably reserve room.
//   // 
//   pPingHeader->id = 0;
//
//   pSendBuffer->mSize = sizeof(BNetPingHeader);
//
//   mSendList.add(pSendBuffer);
//}

//==============================================================================
// 
//==============================================================================
void BXConnection::flush(bool force)
{
   // loopback socket doesn't need to flush anything
   if (mAddr.sin_addr.s_addr == INADDR_LOOPBACK || !mEnabled)
   {
      mLastRecvTime = timeGetTime();
      return;
   }

   BDEBUG_ASSERT(mpAllocator);
   if (mpAllocator == NULL)
      return;

   if (mStopAcks)
   {
      if (!force && ++mFlushCounter < XNetwork::cReducedUpdateInterval && mSendList.getSize() == 0 && mResendList.getSize() == 0)
      {
         // if we're operating in reduced ack mode and we haven't exceeded our flush counter
         // and our send list and pending list queues are empty, then we can hang tight for a bit
         //
         // update the last recv time so we don't prematurely timeout
         mLastRecvTime = timeGetTime();
         return;
      }

      if (mFlushCounter >= XNetwork::cReducedUpdateInterval)
         mFlushCounter = 0;

      setStopAcks(false);
   }

   // flush() will only be called from the 10Hz (or whatever) update tic
   // that happens on the sim thread
   //
   // the goal of flush is to step through the send list and attempt to
   // fit as many requests into 1264 bytes
   //
   // the packet will contain data but may also have voice appended to end
   //
   // The structure of outbound payloads:
   //
   // 2 bytes - cbGameData - size of the data portion
   //
   // |--------------------|
   // | cbGameData 2 bytes |
   // |--------------------|
   // | Data               |
   // |--------------------|
   // | Voice              |
   // |--------------------|
   //
   // The data portion will consist of:
   //
   // |--------------------|
   // | Data from list     |
   // |--------------------|
   // | Resend Requests    |
   // |--------------------|
   // | Ping (optional)    |
   // |--------------------|
   // | Pong (optional)    |
   // |--------------------|
   // | Ack                |
   // |--------------------|
   // | Ack Remote         |
   // |--------------------|

   // pings are 1 byte and included if the ping timer has fired

   // pongs are included if we've received a ping from the remote client

   // pings/pongs are not guaranteed and could be dropped on the floor
   // we hope to make it up in volume by sending them frequently enough

   // if I have resend requests pending, I need to address those first
   // pull up to X number of them and reserve their place in the payload
   // pick a few of the oldest ones

   // when adding resend requests back to the send list, should I place them
   // at the front of the queue?

   // do I want to add a system to reserve/prioritize voice traffic?
   // since there's no clear strategy now, start basic, adding later will be easy

   // construct a packet/payload buffer to send to the udp thread
   BXNetBuffer* pNetBuf = mpAllocator->alloc();
   BDEBUG_ASSERT(pNetBuf);
   if (pNetBuf == NULL)
      return;

   // this is where we'd replace the mAddr with the proxy addr
   // we also need to include the routing information at the top of the payload
   // the routing information will exist for all packets
   if (mProxied)
      pNetBuf->mAddr = mProxyAddr;
   else
      pNetBuf->mAddr = mAddr;

   // current size of payload
   // * reserve room for cbGameData for VDP (2 bytes)
   // * reserve room for ack, ack remote and fin
   uint size = 2 + sizeof(BNetAck)*3;

   // maximum available space for data & voice
   uint maxSize = cMaxBufferSize;

   uchar* pData = mDataBuffer;
   uchar* pVoice = mVoiceBuffer;

   // make room for the proxy header
   if (mProxied)
   {
      BNetProxyHeader* pProxy = reinterpret_cast<BNetProxyHeader*>(pData);

      pProxy->type = BNetHeaderType::cProxyPacket;
      pProxy->fromID = mLocalUniqueID;
      pProxy->toID = mUniqueID;
      pProxy->size = 0;

      size += sizeof(BNetProxyHeader);
      pData += sizeof(BNetProxyHeader);

      // if we're sending a proxy packet, then use a smaller max buffer size
      // this way the client we're using as a proxy still has room to include
      // their traffic in future payloads
      maxSize = cMaxProxyBufferSize;
   }

   BHandle hItem;
   BXSendBuffer* pBuf = mSendList.getHead(hItem);
   while (pBuf)
   {
      // if we can't fit more data into the buffer, let's end this
      if (size + pBuf->mSize > maxSize)
         break;

      // if I'm iterating over buffers and I come across a voice buffer
      // how do I append that buffer to the end of my packet?
      // should I construct two 1264 byte arrays, one for data, one for voice
      // and then copy data into one or the other?
      if (pBuf->mVoice)
      {
         // do not send voice over proxy connections (at least not right now)
         if (!mProxied)
         {
            IGNORE_RETURN(Utils::FastMemCpy(pVoice, pBuf->mBuf, pBuf->mSize));
            pVoice += pBuf->mSize;

            size += pBuf->mSize;
         }

         BXSendBuffer::releaseInstance(pBuf);
      }
      else if (pBuf->mProxied)
      {
         IGNORE_RETURN(Utils::FastMemCpy(pData, pBuf->mBuf, pBuf->mSize));
         pData += pBuf->mSize;

         size += pBuf->mSize;

         BXSendBuffer::releaseInstance(pBuf);
      }
      else
      {
         IGNORE_RETURN(Utils::FastMemCpy(pData, pBuf->mBuf, pBuf->mSize));
         pData += pBuf->mSize;

         size += pBuf->mSize;

         // add this buffer to the sent list
         // do not add voice buffers to the pending list
         // we will not attempt to retry sending them
         mPendingList.addToTail(pBuf);
      }

      // remove us from the sending list
      pBuf = mSendList.removeAndGetNext(hItem);
   }

   uint now = timeGetTime();

   // check for resend requests
   if (mResendList.getSize() > 0)
   {
      // insure we have enough room for at least one resend request
      if (size + sizeof(BNetResendRequest) + sizeof(uint16) <= maxSize)
      {
         BNetResendRequest* pRequest = reinterpret_cast<BNetResendRequest*>(pData);

         pRequest->type = BNetHeaderType::cResendRequest;
         pRequest->numberOfRequests = 0;
         uint16* pRequests = reinterpret_cast<uint16*>(pData + sizeof(BNetResendRequest));

         size += sizeof(BNetResendRequest);
         pData += sizeof(BNetResendRequest);

         // pack in as many resend requests as we have space for
         BHandle hItem;
         BXResendRequest* pResend = mResendList.getHead(hItem);
         for (uint i=0; pResend && i < BNetResendRequest::cMaxResendRequests; ++i)
         // TEMP hack so I can build sans Incredibuild
         //for (uint i=0; pResend && i < 50; ++i)
         {
            if (size + sizeof(uint16) > maxSize)
               break;

            // we give the network a chance to catch up before spamming resends
            if (now < pResend->mSentTime)
               continue;

            nlogt(cTransportNL, "      requesting resend of %ld", pResend->mSeqNum);

            pRequests[pRequest->numberOfRequests] = pResend->mSeqNum;
            pRequest->numberOfRequests++;

            // so this isn't exactly the sent time as much as it's the next time we should attempt a send
            pResend->mSentTime = now + cResendRequestInterval;

            size += sizeof(uint16);
            pData += sizeof(uint16);

            pResend = mResendList.getNext(hItem);
         }
      }
   }

   // check the ping handler to see if we need to send another ping
   // for sending a ping, it may be better to simply query the ping handler
   // for the next available ping ID
   // should I query, or simply poke into the class and check?  I think query is cleaner
   if (size + sizeof(BNetPingHeader) <= maxSize)
   {
      uint8 pingId;
      if (mPingHandler.query(pingId))
      {
         // if the ping handler query returns true, that means we need to issue another ping
         BNetPingHeader* pPing = reinterpret_cast<BNetPingHeader*>(pData);

         pPing->type = BNetHeaderType::cPingPacket;
         pPing->id = pingId;
         pPing->processingDelta = 0;

         size += sizeof(BNetPingHeader);
         pData += sizeof(BNetPingHeader);
      }
   }

   // check the pong count and see if we have enough room to include some pongs
   while (mPongCount > 0 && size + sizeof(BNetPingHeader) <= maxSize)
   {
      BNetPingHeader* pPong = reinterpret_cast<BNetPingHeader*>(pData);

      const BPingRequest& p = mPongs[0];

      pPong->type = BNetHeaderType::cPongPacket;
      pPong->id = p.id;
      pPong->processingDelta = static_cast<uint16>(now - p.mRecvTime);

      size += sizeof(BNetPingHeader);
      pData += sizeof(BNetPingHeader);

      // decrease our pong count and index
      --mPongCount;
      --mPongIndex;
      memmove(mPongs, &mPongs[1], sizeof(BPingRequest)*(cMaxPongs-1));
   }

   uint dataSize = pData - mDataBuffer;
   uint voiceSize = pVoice - mVoiceBuffer;

   // make room for the cbGameData 2 byte header used for VDP traffic
   // we'll me setting the value after we do the acks
   uchar* pPacket = pNetBuf->mBuf + 2;

   // add our data payload
   memcpy(pPacket, mDataBuffer, dataSize);
   pPacket += dataSize;

   // append the ack
   BNetAck* pAck = reinterpret_cast<BNetAck*>(pPacket);
   pAck->type = BNetHeaderType::cAck;
   // XXX verify that we're able to also use the local seq num - 1
   // if we're at 0, then this will be 65535
   // if we're at 5, then this will be 4
   // if we're at 1, then this will be 0
   // if the other side has not received a packet with seq number 0
   // but they have received packets with an ack of 0, will they
   // think that we've sent a packet with seq number 0 and issue
   // a resend request?
   //
   // actually, that should be ok, so what if the other client
   // issues a resend request for packet 0, the packet doesn't exist
   // so we'll discard the request
   // the payload will increase a little but not cause a slowdown
   //
   // could we accidentally send a packet 0 again? hopefully not
   // as we should get "stuck" attempting resends if we get that
   // far out of whack with sequence numbers
   //
   pAck->seqNum = mLocalSeqNum - 1;

   pPacket += sizeof(BNetAck);

   pAck = reinterpret_cast<BNetAck*>(pPacket);
   pAck->type = BNetHeaderType::cAckRemote;
   pAck->seqNum = mRemoteSeqNum;

   pPacket += sizeof(BNetAck);

   // cFin is similar to cAckRemote except we track it separately from mRemoteSeqNum
   // Needed for the case when a client sends a new data payload:
   // * the sender will continue sending ack payloads
   // * the receiver will send cAckRemote and cFin
   // * the sender will continue sending ack payloads until it receives cFin that matches their mLocalSeqNum
   //
   pAck = reinterpret_cast<BNetAck*>(pPacket);
   pAck->type = BNetHeaderType::cFin;
   pAck->seqNum = mFinSeqNum;

   pPacket += sizeof(BNetAck);

   // determine the final data payload size
   // be sure to subtract out the extra 2 bytes to take the cbGameData value into account
   uint16 cbGameData = static_cast<uint16>(pPacket - pNetBuf->mBuf - 2);
   // set the cbGameData value in the buffer
   memcpy(pNetBuf->mBuf, &cbGameData, sizeof(uint16));

   // if we're proxied, then update the size value in our proxy header
   if (mProxied)
   {
      BNetProxyHeader* pProxy = reinterpret_cast<BNetProxyHeader*>(pNetBuf->mBuf + 2);
      // if we're sending proxy traffic, then the actual data size will be cbGameData - the proxy header size
      pProxy->size = cbGameData - sizeof(BNetProxyHeader);
   }

   if (voiceSize > 0)
   {
      IGNORE_RETURN(Utils::FastMemCpy(pPacket, mVoiceBuffer, voiceSize));
      pPacket += voiceSize;
   }

   pNetBuf->mSize = pPacket - pNetBuf->mBuf;

#ifndef BUILD_FINAL
   //BNetIPString strAddr(mAddr);
   //BNetIPString strProxyAddr(mProxyAddr);

   //nlogt(cTransportNL, "BXConnection::flush -- ip[%s] proxy[%s] proxied[%d] uniqueID[%I64u] localSeqNum[%d] remoteSeqNum[%d] size[%d]",
   //   strAddr.getPtr(), strProxyAddr.getPtr(), mProxied, mUniqueID, mLocalSeqNum, mRemoteSeqNum, pNetBuf->mSize);
   nlogt(cTransportNL, "BXConnection::flush -- proxied[%d] uniqueID[%I64u] localSeqNum[%d] remoteSeqNum[%d] size[%d]",
      mProxied, mUniqueID, mLocalSeqNum, mRemoteSeqNum, pNetBuf->mSize);
#endif

   gEventDispatcher.send(cInvalidEventReceiverHandle, mSocketEventHandle, cNetEventUDPSend, (uint)pNetBuf);
}

//==============================================================================
// 
//==============================================================================
void BXConnection::recv(const uchar* pData, const long size, const uint32 time)
{
   mLastRecvTime = timeGetTime();

   BDEBUG_ASSERT(pData);
   if (pData == NULL)
      return;

   IGNORE_RETURN(Utils::FastMemSet(&mFlushList, 0, sizeof(mFlushList)));

   uchar* ptr = const_cast<uchar*>(pData);
//-- FIXING PREFIX BUG ID 7513
   const uchar* endPtr = ptr+size;
//--

   BOOL forceFlushLocal = FALSE;
   BOOL forceFlushProxy = FALSE;

   mStopAckLocal = false;
   mStopAckRemote = false;

   while (ptr < endPtr)
   {
      // cast pData to a net buffer structure
//-- FIXING PREFIX BUG ID 7512
      const BNetHeader* pHeader = reinterpret_cast<BNetHeader*>(ptr);
//--
      switch (pHeader->type)
      {
         case BNetHeaderType::cAck:
            {
               // this ack represents the last known sequence number sent
               // by the remote client, so we need to check for gaps in
               // our received list (if any) and send a request for the
               // remote client to retransmit the missing packets
//-- FIXING PREFIX BUG ID 7501
               const BNetAck* pAck = reinterpret_cast<BNetAck*>(ptr);
//--
               ptr += sizeof(BNetAck);

               // calculate any resends we may need to request
               calcResendRequests(pAck->seqNum);

               if (mResendList.getSize() > 0)
                  forceFlushLocal = TRUE;
               break;
            }
         case BNetHeaderType::cAckRemote:
            {
//-- FIXING PREFIX BUG ID 7502
               const BNetAck* pAck = reinterpret_cast<BNetAck*>(ptr);
//--
               ptr += sizeof(BNetAck);

               clearSentList(pAck->seqNum);

               // set our mFinSeqNum to pAck->seqNum but only if it's greater
               if ((mFinSeqNum > pAck->seqNum) && ((mFinSeqNum - pAck->seqNum) > cQueueSize))
                  mFinSeqNum = pAck->seqNum;
               else if ((pAck->seqNum > mFinSeqNum) && ((pAck->seqNum - mFinSeqNum) < cQueueSize))
                  mFinSeqNum = pAck->seqNum;

               // if pAck->seqNum matches our mLocalSeqNum then we can stop sending local acks
               if (pAck->seqNum == mLocalSeqNum)
                  mStopAckLocal = true;
               break;
            }
         case BNetHeaderType::cFin:
            {
               const BNetAck* pAck = reinterpret_cast<BNetAck*>(ptr);
               ptr += sizeof(BNetAck);

               // if pAck->seqNum matches our mRemoteSeqNum then we can stop sending remote acks
               if (pAck->seqNum == mRemoteSeqNum)
                  mStopAckRemote = true;
               break;
            }
         case BNetHeaderType::cResendRequest:
            {
#ifndef BUILD_FINAL
               m_dwPacketResendRequestCount++;
#endif
//-- FIXING PREFIX BUG ID 7503
               const BNetResendRequest* pReq = reinterpret_cast<BNetResendRequest*>(ptr);
//--
//-- FIXING PREFIX BUG ID 7504
               const uint16* pRequests = reinterpret_cast<const uint16*>(ptr + sizeof(BNetResendRequest));
//--
               ptr += sizeof(BNetResendRequest) + (sizeof(uint16)*pReq->numberOfRequests);

               for (uint i=0; i < pReq->numberOfRequests; ++i)
               {
                  uint seqNum = pRequests[i];

                  // resend the matching sent item
                  nlogt(cTransportNL, "        resend requested for %ld", seqNum);
                  // go through the sent list and "resend" the requested packets
                  BHandle hItem;
                  BXSendBuffer* pSentBuf = mPendingList.getHead(hItem);
                  while (pSentBuf)
                  {
                     if (pSentBuf->mSeqNum == seqNum)
                     {
                        nlogt(cTransportNL, "        resending %ld", seqNum);
                        // place the buffer at the front of the line
                        mSendList.addToHead(pSentBuf);
                        // remove us from the sent list so we can be sent again, joy!
                        mPendingList.remove(hItem);
                        break;
                     }
                     pSentBuf = mPendingList.getNext(hItem);
                  }
               }

               if (pReq->numberOfRequests > 0)
                  forceFlushLocal = TRUE;
               break;
            }
         case BNetHeaderType::cData:
         case BNetHeaderType::cDataFragment:
         case BNetHeaderType::cDataFragmentDone:
            {
//-- FIXING PREFIX BUG ID 7506
               const BNetSeqHeader* pSeqHeader = reinterpret_cast<BNetSeqHeader*>(ptr);
//--
//-- FIXING PREFIX BUG ID 7507
               const uchar* pSeqBuf = ptr + sizeof(BNetSeqHeader);
//--
               ptr += sizeof(BNetSeqHeader) + pSeqHeader->size;

#ifndef BUILD_FINAL
               BNetIPString strAddr(mAddr);
               nlogt(cTransportNL, "        got data packet %ld type[%d] [%s]", pSeqHeader->seqNum, pHeader->type, strAddr.getPtr());
#endif

               // first see if we have an outstanding resend request for this packet
               BHandle hItem;
               BXResendRequest* pResend = mResendList.getHead(hItem);
               while (pResend)
               {
                  if (pResend->mSeqNum == pSeqHeader->seqNum)
                  {
                     mResendList.remove(hItem);
                     BXResendRequest::releaseInstance(pResend);
                     break;
                  }
                  pResend = mResendList.getNext(hItem);
               }

               if (pSeqHeader->seqNum == mRemoteSeqNum)
               {
                  // we got an in-sequence packet, so just release it
                  if (pSeqHeader->type == BNetHeaderType::cDataFragment || pSeqHeader->type == BNetHeaderType::cDataFragmentDone)
                  {
                     BDEBUG_ASSERT(mRecvFragmentBufferPtr < sizeof(mRecvFragmentBuffer));

                     if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < pSeqHeader->size)
                     {
                        // if we run out of space for this data packet, then the only way to recover is to
                        // re-allocate the fragment buffer, but the game should not be exceeding this
                        // size in the first place!
                        nlogt(cTransportNL, "BXConnection::recv -- not enough space for data fragment.");
                        BDEBUG_ASSERTM(false, "BXConnection::recv -- not enough space for data fragment");
                        return;
                     }

                     IGNORE_RETURN(Utils::FastMemCpy(mRecvFragmentBuffer + mRecvFragmentBufferPtr, pSeqBuf, pSeqHeader->size));
                     mRecvFragmentBufferPtr += pSeqHeader->size;
                     if (pSeqHeader->type == BNetHeaderType::cDataFragmentDone)
                     {
#ifndef BUILD_FINAL
                        BNetIPString strAddr(mAddr);
                        nlogt(cTransportNL, "          in seq %ld, release (fragment done) [%s]", pSeqHeader->seqNum, strAddr.getPtr());
#endif
                        BXNetData* pData = HEAP_NEW(BXNetData, gNetworkHeap);
                        pData->init(mRecvFragmentBuffer, mRecvFragmentBufferPtr, mAddr, time);
                        mRecvFragmentBufferPtr = 0;
                        processRecv(pData);
                     }
                     else
                     {
#ifndef BUILD_FINAL
                        BNetIPString strAddr(mAddr);
                        nlogt(cTransportNL, "          got a fragment, seq %ld [%s]", pSeqHeader->seqNum, strAddr.getPtr());
#endif
                        mRecvFragmentTime = time;
                     }
                  }
                  else
                  {
                     mRecvFragmentBufferPtr = 0;
#ifndef BUILD_FINAL
                     BNetIPString strAddr(mAddr);
                     nlogt(cTransportNL, "          in seq %ld, release [%s]", pSeqHeader->seqNum, strAddr.getPtr());
#endif
                     BXNetData* pData = HEAP_NEW(BXNetData, gNetworkHeap);
                     pData->init(pSeqBuf, pSeqHeader->size, mAddr, time);
                     processRecv(pData);
                  }

                  mRemoteSeqNum++;

#ifndef BUILD_FINAL
                  m_dwInSequencePacketReceiveCount++;
#endif

                  updateRecvdList();
               }
               else
               {
#ifndef BUILD_FINAL
                  m_dwOutOfSequencePacketReceiveCount++;
#endif

                  uint32 distance = calcDistance(mRemoteSeqNum, pSeqHeader->seqNum);

                  // is it a message in the future? If so, stuff it into rcvd list
                  if (distance < (cQueueSize)-1)
                  {
                     nlogt(cTransportNL, "          in future");
                     // allocate a recv buffer and shove this message asside for future processing
                     BXRecvBuffer* pRecvBuffer = BXRecvBuffer::getInstance();
                     BDEBUG_ASSERT(pRecvBuffer);
                     pRecvBuffer->mType = pSeqHeader->type;
                     pRecvBuffer->mSeqNum = pSeqHeader->seqNum;
                     pRecvBuffer->mSize = pSeqHeader->size;
                     pRecvBuffer->mRecvTime = time;
                     pRecvBuffer->mAddr = mAddr;
                     IGNORE_RETURN(Utils::FastMemCpy(pRecvBuffer->mBuf, pSeqBuf, pSeqHeader->size));

                     // push into rcvd list
                     bool found = false;
                     BHandle hItem;
//-- FIXING PREFIX BUG ID 7505
                     const BXRecvBuffer* pBuf = mRecvList.getHead(hItem);
//--
                     while (pBuf)
                     {
                        if (
                           (calcDistance(pSeqHeader->seqNum, pBuf->mSeqNum) < cQueueSize) &&
                           (pSeqHeader->seqNum < pBuf->mSeqNum) 
                           )
                        {
                           nlogt(cTransportNL, "            inserting before %ld", pBuf->mSeqNum);
                           mRecvList.addBefore(pRecvBuffer, hItem);
                           found = true;
                           break;
                        }
                        nlogt(cTransportNL, "            skipping %ld", pBuf->mSeqNum);
                        pBuf = mRecvList.getNext(hItem);
                     }
                     if (!found)
                     {
                        nlogt(cTransportNL, "            push_back");
                        mRecvList.addToTail(pRecvBuffer);
                     }

                     // and tell client that we need resends
                     calcResendRequests();
                  }
               }
               break;
            }

         case BNetHeaderType::cVoicePacket:
            {
//-- FIXING PREFIX BUG ID 7508
               const BNetVoiceHeader* pVoiceHeader = reinterpret_cast<BNetVoiceHeader*>(ptr);
//--
//-- FIXING PREFIX BUG ID 7509
               const uchar* pVoiceBuf = ptr + sizeof(BNetVoiceHeader);
//--
               ptr += sizeof(BNetVoiceHeader) + pVoiceHeader->size;

               if (mMuted || mVoiceEventHandle == cInvalidEventReceiverHandle)
                  break;

               // construct a voice payload to send to the voice layer
               // this means that the udp layer will no longer handle voice traffic
               // independently of the data traffic
               //BXNetVoice* pNetVoice = new BXNetVoice();
               //BXNetVoice* pNetVoice = HEAP_NEW(BXNetVoice, gNetworkHeap);
               //pNetVoice->init(pVoiceBuf, pVoiceHeader->size, pVoiceHeader->xuid, mAddr);
               //gEventDispatcher.send(cInvalidEventReceiverHandle, mVoiceEventHandle, cNetEventVoiceRecv, 0, 0, pNetVoice);

               if (pVoiceHeader->size > XNetwork::cMaxVoiceSendSize)
                  break;
               BVoiceBuffer* pVoice = mpVoiceAllocator->alloc();
               if (pVoice == NULL)
                  break;
               pVoice->init(pVoiceBuf, pVoiceHeader->size, pVoiceHeader->xuid);
               // send this event from the session event handle
               // the session event handle is what's registered with the voice system
               // for distinguishing sessions
               gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceRecv, (uint)pVoice);
               break;
            }

         case BNetHeaderType::cPingPacket:
            {
               // the ping contains only an ID, we support IDs from 0 to 15
               // pings should be sent once a second and the IDs are recycled
               // 
               // I might be handling multiple pings before I have a chance to send the pong
               //
               // do I send back all the pings or just the last one?
//-- FIXING PREFIX BUG ID 7510
               const BNetPingHeader* pPingHeader = reinterpret_cast<BNetPingHeader*>(ptr);
//--
               ptr += sizeof(BNetPingHeader);

               // if we're at the last element in the array
               if (mPongIndex >= cMaxPongs)
               {
                  mPongCount = cMaxPongs;
                  mPongIndex = cMaxPongs - 1;
                  memmove_s(mPongs, sizeof(BPingRequest)*cMaxPongs, &mPongs[1], sizeof(BPingRequest)*(cMaxPongs-1));
               }

               mPongs[mPongIndex].mRecvTime = time;
               mPongs[mPongIndex].id = pPingHeader->id;

               ++mPongIndex;
               if (++mPongCount >= cMaxPongs)
                  mPongCount = cMaxPongs;
               break;
            }

         case BNetHeaderType::cPongPacket:
            {
               // pass the pong off to our ping handler?
//-- FIXING PREFIX BUG ID 7511
               const BNetPingHeader* pPingHeader = reinterpret_cast<BNetPingHeader*>(ptr);
//--
               ptr += sizeof(BNetPingHeader);
               mPingHandler.update(pPingHeader->id, pPingHeader->processingDelta, time);
               break;
            }

         case BNetHeaderType::cProxyPacket:
            {
               BNetProxyHeader* pProxyHeader = reinterpret_cast<BNetProxyHeader*>(ptr);

#ifndef BUILD_FINAL
               BNetIPString strAddr(mAddr);
               nlogt(cTransportNL, "BXConnection::recv -- proxy packet on ip[%s] fromID[%I64u] toID[%I64u] size[%d] (%I64u:%I64u)", strAddr.getPtr(), pProxyHeader->fromID, pProxyHeader->toID, pProxyHeader->size, mLocalUniqueID, mUniqueID);
#endif

               if (pProxyHeader->toID == mLocalUniqueID)
               {
                  // this data was destined for me, lookup the connection for the fromID
                  // and let that BXConnection handle parsing the data
                  BXConnection* pConn = mpConnectionInterface->getConnection(pProxyHeader->fromID);
                  if (pConn)
                  {
                     if (!pConn->isProxied())
                        pConn->setProxy(mAddr, pProxyHeader->fromID);
                     pConn->recv(ptr + sizeof(BNetProxyHeader), pProxyHeader->size, time);
                  }
               }
               else if (mpConnectionInterface)
               {
                  // lookup the BXConnection for the machine we wish to forward this data
                  // once we know the connection we can dump the payload into that connection's
                  // outbound queue for transmission
                  BXConnection* pConn = mpConnectionInterface->getConnection(pProxyHeader->toID);
                  if (pConn)
                  {
                     pConn->send(ptr, sizeof(BNetProxyHeader) + pProxyHeader->size, cSendProxy);

                     forceFlushProxy = TRUE;

                     // I may be receiving multiple proxy packets, so how do I queue up the flush
                     // to only execute once per update... I can't cache the connection because
                     // I may be acting as a proxy for multiple clients
                     for (uint i=0; i < XNetwork::cMaxClients; ++i)
                     {
                        if (mFlushList[i] == pConn)
                           break;
                        else if (mFlushList[i] == NULL)
                        {
                           mFlushList[i] = pConn;
                           break;
                        }
                     }
                  }
                  else
                  {
                     // if the connection is missing, I need to send a disconnect packet back to the sender to let them know
                     //
                     // I need to construct the proxy wrapper with the proper from/to IDs and send back to the client
                     //
                     // the current BXConnection instance is the conduit to the client sending me information that it wants to
                     // forward to another client, so I have context to echo back a disconnect packet with the appropriate from/to IDs
                     sendProxyDisconnect(pProxyHeader->fromID, pProxyHeader->toID);
                  }
               }

               ptr += (sizeof(BNetProxyHeader) + pProxyHeader->size);
               break;
            }

         case BNetHeaderType::cDisconnect:
            {
               // received a disconnect from the given addr
               // how do I disconnect from within the connection thread?
#ifndef BUILD_FINAL
               BNetIPString strAddr(mAddr);
               nlogt(cTransportNL, "BXConnection::recv -- disconnect received ip[%s]", strAddr.getPtr());
#endif
               ptr += sizeof(BNetHeader);
               gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cNetEventSocketError, WSAEDISCON, mAddr.sin_addr.s_addr);
               break;
            }

         default:
            {
#ifndef BUILD_FINAL
               BDEBUG_ASSERTM(false, "BXConnection::recv: Unknown header type");
               BNetIPString strAddr(mAddr);
               nlogt(cTransportNL, "BXConnection::recv -- Unknown header type[%d] size[%d] ip[%s]", pHeader->type, size, strAddr.getPtr());
#endif
               return;
            }
      }
   }

   // if I receive a resend request or if I calculated a gap in my recv'd sequence numbers
   // then I should issue a flush now so the other client can get the request ASAP
   //
   // if we already have a bad connection, this could make it worse...
   //
   // check to see if our average ping is less than our current network update interval
   // if so, then flushing will improve performance in low latency games, otherwise, it doesn't buy us anything
   //
   // what about flushing proxied connections?  They would also benefit in this scenario
   // but I may be acting as a proxy for multiple machines
   if ((forceFlushLocal || forceFlushProxy) && mPingHandler.getAveragePing() < XNetwork::cMinPingForceFlush)
   {
      if (forceFlushLocal)
         flush(true);

      if (mFlushList[0] != NULL)
      {
         for (uint i=0; i < XNetwork::cMaxClients; ++i)
         {
            if (mFlushList[i] == NULL)
               break;
            mFlushList[i]->flush(true);
         }
      }
   }

   if (mStopAckLocal && mStopAckRemote)
      setStopAcks(true);
   else
      setStopAcks(false);
}

//==============================================================================
// 
//==============================================================================
void BXConnection::setProxy(const SOCKADDR_IN& addr, const XNADDR& xnAddr)
{
   // if we're enabled, then don't bother with the proxy
   if (mEnabled)
      return;

   // generate a unique ID so we know where to forward our data
   setProxy(addr, BSession::getUniqueID(xnAddr));
}

//==============================================================================
// 
//==============================================================================
void BXConnection::resetProxy()
{
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);

   nlogt(cTransportNL, "BXConnection::resetProxy -- ip[%s] uniqueID[%I64u]",
      strAddr.getPtr(), mUniqueID);
#endif

   mEnabled = true;
   mProxied = false;
}

//==============================================================================
// 
//==============================================================================
void BXConnection::unregister()
{
   if (!mXNetRegistered)
      return;

   mXNetRegistered = false;

   // if the connection is not enabled, then it's currently in search of a proxy or already being shutdown
   // so we don't need to issue another timeout
   int retval = XNetUnregisterInAddr(mAddr.sin_addr);
   if (retval != 0)
   {
      BNetIPString strAddr(mAddr);
      nlogt(cSessionNL, "BSession::receiveEvent -- failed to unregister client IN_ADDR[%s] error[%d]", strAddr.getPtr(), retval);

      gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cNetEventSocketError, WSAETIMEDOUT, mAddr.sin_addr.s_addr);
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::setProxy(const SOCKADDR_IN& addr, uint64 uniqueID)
{
   // do not perform the enabled check here because I may be enabled and still receive
   // proxy packets from another client that already detected the loss of the connection
   //if (mEnabled)
   //   return;

   mProxyAddr = addr;

   mUniqueID = uniqueID;

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);
   BNetIPString strProxyAddr(mProxyAddr);

   nlogt(cTransportNL, "BXConnection::setProxy -- ip[%s] proxy[%s] localUniqueID[%I64u] uniqueID[%I64u]",
      strAddr.getPtr(), strProxyAddr.getPtr(),
      mLocalUniqueID, mUniqueID);

   gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, BSession::cSessionEventProxyInfo, mAddr.sin_addr.s_addr, mProxyAddr.sin_addr.s_addr);
#endif

   mEnabled = true;
   mProxied = true;
}

//==============================================================================
// 
//==============================================================================
void BXConnection::processRecv(BXNetData* pData)
{
   if (pData == NULL)
      return;

   if (pData->mSize > 1)
   {
      switch (BTypedPacket::getType(pData->mBuf))
      {
         case BPacketType::cTalkersPacket:
            {
               handleTalkersPacket(pData);
               HEAP_DELETE(pData, gNetworkHeap);
               return;
            }

         case BPacketType::cVoiceHeadsetPacket:
            {
               handleVoiceHeadsetPacket(pData);
               HEAP_DELETE(pData, gNetworkHeap);
               return;
            }

         default:
            break;
      }
   }

   gEventDispatcher.send(cInvalidEventReceiverHandle, mSessionEventHandle, cNetEventConnRecv, 0, 0, pData);
}

//==============================================================================
// pData pointer validation done elsewhere
//==============================================================================
void BXConnection::handleTalkersPacket(BXNetData* pData)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // ignore talkers packets from myself
   if (pData->mAddr.sin_addr.s_addr == INADDR_LOOPBACK)
      return;

   BTalkersPacket packet;
   packet.deserializeFrom(pData->mBuf, pData->mSize);

#ifndef BUILD_FINAL
   BNetIPString strAddr(pData->mAddr);
   nlogt(cSessionNL, "BXConnection::handleTalkersPacket -- got cTalkersPacket ip[%s]", strAddr.getPtr());
#endif

   // update our voice system
   // need to ship this list to the voice system
   BVoiceTalkerListPayload* pPayload = HEAP_NEW(BVoiceTalkerListPayload, gNetworkHeap);
   IGNORE_RETURN(Utils::FastMemCpy(pPayload->mXuids, packet.mXuids, sizeof(XUID)*BTalkersPacket::cMaxClients));
   pPayload->mOwnerXuid = packet.mOwnerXuid;
   pPayload->mCount = packet.mCount;

   gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceTalkerList, 0, 0, pPayload);
}

//==============================================================================
// pData pointer validation done elsewhere
//==============================================================================
void BXConnection::handleVoiceHeadsetPacket(BXNetData* pData)
{
   if (mVoiceEventHandle == cInvalidEventReceiverHandle)
      return;

   // ignore headset packets from myself
   if (pData->mAddr.sin_addr.s_addr == INADDR_LOOPBACK)
      return;

   BVoiceHeadsetPacket packet;
   packet.deserializeFrom(pData->mBuf, pData->mSize);

#ifndef BUILD_FINAL
   BNetIPString strAddr(pData->mAddr);
   nlogt(cSessionNL, "BXConnection::handleVoiceHeadsetPacket -- got cVoiceHeadsetPacket ip[%s]", strAddr.getPtr());
#endif

   gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceHeadsetPresent, packet.mClientID, packet.mHeadset);
}

//==============================================================================
// 
//==============================================================================
void BXConnection::clearSentList(uint16 ack)
{
   // the ack given here is sent from the client and is the client's
   // tracking value for our next expected sequence number to be received,
   // so if I haven't sent anything, the other client will send me my
   // mLocalSeqNum value, since mLocalSeqNum is also the next expected seq num that I will send
   //
   // if the ack == mLocalSeqNum, then we haven't sent anything and the remote client has
   // received everything that we've sent so far

   // if we're up to date, then we can attempt to slow down our connection update frequency to save bandwidth
   //if (ack == mLocalSeqNum)
   //{
   //   mStopAckLocal = true;
   //}

   // if the ack matches our local sequence number or the distance is within our queue size
   // that means that we're either getting an ack for previously sent packets or we're at a
   // steady state and are free to clear out the sent list
   if (ack == mLocalSeqNum || calcDistance(ack, mLocalSeqNum) < cQueueSize)
   {
      if (mPendingList.getSize() == 0)
         return;

      nlogt(cTransportNL, "      clearSentList ack %ld, mLocalSeqNum %ld", ack, mLocalSeqNum);

      // if so, then clear out any sent items older than this
      BHandle hItem;
      BXSendBuffer* pSendBuf = mPendingList.getHead(hItem);
      while (pSendBuf)
      {
         if (pSendBuf->mSeqNum != ack && calcDistance(pSendBuf->mSeqNum, ack) < cQueueSize)
         {
            nlogt(cTransportNL, "      clearing sent item %ld", pSendBuf->mSeqNum);
            BXSendBuffer::releaseInstance(pSendBuf);
            pSendBuf = mPendingList.removeAndGetNext(hItem);
         }
         else
         {
            nlogt(cTransportNL, "      skipping clear of sent item %ld", pSendBuf->mSeqNum);
            pSendBuf = mPendingList.getNext(hItem);
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::calcResendRequests()
{
   // iterate over the rcvdlist and add resend requests for any gaps in that list

   nlogt(cTransportNL, "      calcResendRequests mRemoteSeqNum %ld", mRemoteSeqNum);

   if (mRecvList.getSize() == 0)
      return; // nothing to request

   // start us off at the next sequence number that we're expecting
   uint16 curReq = mRemoteSeqNum;

   BHandle hItem;
//-- FIXING PREFIX BUG ID 7514
   const BXRecvBuffer* pBuf = mRecvList.getHead(hItem);
//--
   while (pBuf)
   {
      // loop through all missing sequence numbers and add them to resendRequestList
      while (
         // dpm 11/15/2006
         // need >= 0 instead of > 0
         // because of the edge case:
         // curReq = 65535, (i)->SeqNum = 0
         // the loop will fail but (i)->SeqNum != curReq
         // and we'll skip over the resend request
         (pBuf->mSeqNum != curReq) &&
         (calcDistance(curReq, pBuf->mSeqNum) >= 0) &&
         (calcDistance(curReq, pBuf->mSeqNum) < cQueueSize)
         )
      {
         addResendRequest(curReq);
         curReq++;
      }

      if (pBuf->mSeqNum == curReq)
         curReq++;

      pBuf = mRecvList.getNext(hItem);
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::calcResendRequests(uint16 actualRemoteSeqNum)
{
   // mRemoteSeqNum is the next sequence number we're expecting
   // and not the last one we've actually received
   uint16 curReq = mRemoteSeqNum - 1;

   // if we know the actual remote sequence number, then we request resends for anything we haven't received yet
   // by counting up to that sequence number, and checking the rcvdlist as we go

   // actualRemoteSeqNum is the actual seq number of the last packet sent
   // mRemoteSeqNum is the next seq number I'm expecting
   // compare against mRemoteSeqNum-1
   if (actualRemoteSeqNum == curReq)
      return; // nothing to request

   nlogt(cTransportNL, "      calcResendRequests mRemoteSeqNum %ld, actualRemoteSeqNum %ld", mRemoteSeqNum, actualRemoteSeqNum);

   // if mRemoteSeqNum is 65535 (or something large enough to cause a rollover)
   // but the value passed in is less because of that rollover
   // then we'll never request resends until we're caught up, but if we've
   // unfortunately lost a packet, then we'll never request a resend and then stall
   // and blow up once we rollover
   //
   uint16 distance = 0;
   if ((curReq > actualRemoteSeqNum) && ((curReq - actualRemoteSeqNum) > cQueueSize))
   {
      // we've wrapped, need to compensate
      distance = calcDistance(curReq, actualRemoteSeqNum);
   }
   else if (curReq > actualRemoteSeqNum)
   {
      // if we've received an ack that's less than
      // what we've actually received up to, then we
      // can ignore
      //
      // this would happen in the case of an out of order packet
      //distance = curReq - actualRemoteSeqNum;
      distance = 0;
   }
   else if ((actualRemoteSeqNum - curReq) > cQueueSize)
   {
      // ignore this one, again this is when we receive an old ack
      // and then wrapped around
      distance = 0;
   }
   else
   {
      // this is the more likely case
      // for example, receive an ack for seq 30
      // but we've only processed up through 20
      // leaving us 10 packets unaccounted for
      distance = actualRemoteSeqNum - curReq;
   }

   // go through our rcvd list and find the holes up to
   // this ack... increment curReq at the start of the loop
   // to start us off at the first unknown packet
   curReq = mRemoteSeqNum;
   for (uint i=0; i < distance; i++, curReq++)
   {
      BOOL skip = FALSE;

      BHandle hItem;
//-- FIXING PREFIX BUG ID 7515
      const BXRecvBuffer* pBuf = mRecvList.getHead(hItem);
//--
      while (pBuf)
      {
         if (pBuf->mSeqNum == curReq)
         {
            skip = TRUE;
            break;
         }
         pBuf = mRecvList.getNext(hItem);
      }
      if (skip)
         continue; // we already have this packet

      // queue up a resend request for this sequence number
      addResendRequest(curReq);
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::addResendRequest(uint16 seqNum)
{
   // do we already have this one?
   BHandle hItem;
   BXResendRequest* pResend = mResendList.getHead(hItem);
   while (pResend)
   {
      if (pResend->mSeqNum == seqNum)
         return;
      pResend = mResendList.getNext(hItem);
   }

   pResend = BXResendRequest::getInstance();
   BDEBUG_ASSERT(pResend);

   nlogt(cTransportNL, "        adding resend request %ld", seqNum);

   // going back to the old way of setting the initial resend time to 0
   //
   // hopefully with the new coalescing and flush interval we can smooth out
   // some of the resend requests
   pResend->mSentTime = 0;
   pResend->mSeqNum = seqNum;

   mResendList.addToTail(pResend);
}

//==============================================================================
// 
//==============================================================================
uint16 BXConnection::calcDistance(uint16 seqNum1, uint16 seqNum2) const
{
   if (seqNum2 > seqNum1)
      return seqNum2 - seqNum1;
   else
      return (65535 - seqNum1) + seqNum2;
}

//==============================================================================
// Parse through received list and free any messages that are in the
//    correct sequence
//==============================================================================
void BXConnection::updateRecvdList()
{
   BHandle hItem;
   BXRecvBuffer* pBuf = mRecvList.getHead(hItem);
   while (pBuf)
   {
      // if the buffer's sequence number is greater than the one we're looking for
      // then we can stop looking because the list is kept in order
      if (pBuf->mSeqNum > mRemoteSeqNum)
         return;

      if (pBuf->mSeqNum == mRemoteSeqNum)
      {
         if (pBuf->mType == BNetHeaderType::cDataFragment || pBuf->mType == BNetHeaderType::cDataFragmentDone)
         {
            BDEBUG_ASSERT(mRecvFragmentBufferPtr < sizeof(mRecvFragmentBuffer));

            if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < pBuf->mSize)
            {
               // this shouldn't be happening because we prevent any outbound data from exceeding the fragment buffer size
               nlogt(cTransportNL, "BXConnection::updateRecvdList -- not enough space for data fragment.");
               BDEBUG_ASSERTM(false, "BXConnection::updateRecvdList -- not enough space for data fragment");
               return;
            }

            IGNORE_RETURN(Utils::FastMemCpy(mRecvFragmentBuffer + mRecvFragmentBufferPtr, pBuf->mBuf, pBuf->mSize));
            mRecvFragmentBufferPtr += pBuf->mSize;
            if (pBuf->mType == BNetHeaderType::cDataFragmentDone)
            {
               nlogt(cTransportNL, "          in seq, release");

               BXNetData* pData = HEAP_NEW(BXNetData, gNetworkHeap);
               pData->init(pBuf->mBuf, pBuf->mSize, pBuf->mAddr, mRecvFragmentTime);

               mRecvFragmentBufferPtr = 0;

               processRecv(pData);
            }
            else
               nlogt(cTransportNL, "          got a fragment");
         }
         else
         {
            nlogt(cTransportNL, "      freeing old message %ld", pBuf->mSeqNum);

            BXNetData* pData = HEAP_NEW(BXNetData, gNetworkHeap);
            pData->init(pBuf->mBuf, pBuf->mSize, pBuf->mAddr, pBuf->mRecvTime);
            processRecv(pData);
         }

         // release the buffer back to the pool
         BXRecvBuffer::releaseInstance(pBuf);

         // remove our packet from the received list
         pBuf = mRecvList.removeAndGetNext(hItem);

         // increment the known remote sequence number
         mRemoteSeqNum++;
         continue;
      }
      pBuf = mRecvList.getNext(hItem);
   }
}

//==============================================================================
// 
//==============================================================================
void BXConnection::setStopAcks(bool value)
{
   if (mStopAcks == value)
      return;

   mStopAcks = value;

   //if (!value)
   //   mFlushCounter = 0;
}

//==============================================================================
// 
//==============================================================================
void BXConnection::sendProxyDisconnect(uint64 fromID, uint64 toID)
{
   // construct a proxy packet and bury a disconnect packet within it
   //
   // |--------------------|
   // | Proxy Header       |
   // |--------------------|
   // | Disconnect         |
   // |--------------------|
   //
   uchar data[sizeof(BNetProxyHeader) + sizeof(BNetHeader)];
   uint size = 0;
   uchar* pData = data;

   BNetProxyHeader* pProxy = reinterpret_cast<BNetProxyHeader*>(pData);

   // reverse the from/to IDs because we're going to echo on behalf of the destination client
   pProxy->type = BNetHeaderType::cProxyPacket;
   pProxy->fromID = toID;
   pProxy->toID = fromID;
   pProxy->size = sizeof(BNetHeader);

   size += sizeof(BNetProxyHeader);
   pData += sizeof(BNetProxyHeader);

   pData[0] = BNetHeaderType::cDisconnect;

   size += sizeof(BNetHeader);

#ifndef BUILD_FINAL
   BNetIPString strAddr(mAddr);
   nlogt(cTransportNL, "BXConnection::sendProxyDisconnect -- fromID[%I64u] toID[%I64u] to[%s]", toID, fromID, strAddr.getPtr());
#endif

   send(data, size, cSendProxy);
}
