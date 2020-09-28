//==============================================================================
// xlandiscovery.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

// xnetwork
#include "Precompiled.h"

#include "xlandiscovery.h"
#include "xudpsocket.h"

// xsystem
#include "config.h"
#include "econfigenum.h"

// xcore
#include "threading\eventDispatcher.h"
#include "threading\workDistributor.h"

//==============================================================================
// 
//==============================================================================
BLanGameInfo::BLanGameInfo() :
   BTypedPacket(BPacketType::cLanInfoPacket),
   mNonce(0),
   mChecksum(0),
   mGameType(0),
   mMapIndex(0),
   mDifficulty(0),
   mTitleID(0),
   mUpdateTime(0),
   mFilledSlots(0),
   mMaxSlots(0),
   mLocked(false),
   mUpdated(false),
   mBadCRC(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mXnKey, 0, sizeof(XNKEY)));
   IGNORE_RETURN(Utils::FastMemSet(&mXnKID, 0, sizeof(XNKID)));
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::init(uint titleID, uint checksum)
{
   if (mTitleID == titleID)
      return;

   mTitleID = titleID;
   mChecksum = checksum;

   XNetRandom((BYTE*)&mNonce, sizeof(mNonce));
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::reset()
{
   mTitleID = 0;
   mChecksum = 0;
   mNonce = 0;
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setInfo(const BUString& info)
{
   if (mInfo != info)
   {
      mInfo = info;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setMapIndex(uint mapIndex)
{
   if (mMapIndex != mapIndex)
   {
      mMapIndex = mapIndex;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setGameType(uint gameType)
{
   if (mGameType != gameType)
   {
      mGameType = gameType;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setDifficulty(uint difficulty)
{
   if (mDifficulty != difficulty)
   {
      mDifficulty = difficulty;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setSlots(uint availableSlots, uint maxSlots)
{
   if (mFilledSlots != availableSlots || mMaxSlots != maxSlots)
   {
      mFilledSlots = availableSlots;
      mMaxSlots = maxSlots;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setLocked(bool locked)
{
   if (mLocked != locked)
   {
      mLocked = locked;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setXnAddr(const XNADDR& xnAddr)
{
   if (memcmp(&mXnAddr, &xnAddr, sizeof(XNADDR)))
   {
      mXnAddr = xnAddr;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setXnKey(const XNKEY& xnKey)
{
   if (memcmp(&mXnKey, &xnKey, sizeof(XNKEY)))
   {
      mXnKey = xnKey;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setXnKID(const XNKID& xnKID)
{
   if (memcmp(&mXnKID, &xnKID, sizeof(xnKID)))
   {
      mXnKID = xnKID;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setNonce(uint64 nonce)
{
   if (mNonce != nonce)
   {
      mNonce = nonce;
      mUpdated = true;
   }
}

//==============================================================================
// 
//==============================================================================
bool BLanGameInfo::update(const BLanGameInfo& game)
{
   if (mNonce != game.mNonce)
      return false;

   *this = game;

   mUpdateTime = timeGetTime();

   return mUpdated;
}

//==============================================================================
// 
//==============================================================================
bool BLanGameInfo::isUpdated()
{
   if (mUpdated)
   {
      mUpdated = false;
      return true;
   }

   return false;
}

//==============================================================================
// 
//==============================================================================
BLanGameInfo& BLanGameInfo::operator=(const BLanGameInfo& source)
{
   if (this == &source)
      return *this;

   mUpdated = false;

   setInfo(source.mInfo);

   setXnAddr(source.mXnAddr);
   setXnKey(source.mXnKey);
   setXnKID(source.mXnKID);
   setNonce(source.mNonce);

   mBadCRC = source.mBadCRC;
   if (mChecksum != source.mChecksum)
      mUpdated = true;
   mChecksum = source.mChecksum;

   setGameType(source.mGameType);
   setMapIndex(source.mMapIndex);
   setDifficulty(source.mDifficulty);

   if (mTitleID != source.mTitleID)
      mUpdated = true;
   mTitleID = source.mTitleID;

   mUpdateTime = source.mUpdateTime;

   setSlots(source.mFilledSlots, source.mMaxSlots);
   setLocked(source.mLocked);

   return *this;
}

//==============================================================================
// 
//==============================================================================
bool BLanGameInfo::operator==(const BLanGameInfo& other) const
{
   if (mTitleID != other.mTitleID)
      return false;
   if (mChecksum != other.mChecksum)
      return false;
   if (mNonce != other.mNonce)
      return false;

   if (memcmp(&mXnAddr, &other.mXnAddr, sizeof(XNADDR)) != 0)
      return false;
   if (memcmp(&mXnKey, &other.mXnKey, sizeof(XNKEY)) != 0)
      return false;
   if (memcmp(&mXnKID, &other.mXnKID, sizeof(XNKID)) != 0)
      return false;

   return (mInfo == other.mInfo);
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);

   sb.add(mTitleID);
   sb.add(mChecksum);
   sb.add(mNonce);
   sb.add(mMapIndex);
   sb.add(mGameType);
   sb.add(mDifficulty);
   sb.add(mFilledSlots);
   sb.add(mMaxSlots);
   sb.add(mLocked);
   sb.add(mInfo);

   sb.add(static_cast<void*>(&mXnAddr), sizeof(XNADDR));
   sb.add(static_cast<void*>(&mXnKey), sizeof(XNKEY));
   sb.add(static_cast<void*>(&mXnKID), sizeof(XNKID));
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);

   sb.get(&mTitleID);
   sb.get(&mChecksum);
   sb.get(&mNonce);
   sb.get(&mMapIndex);
   sb.get(&mGameType);
   sb.get(&mDifficulty);
   sb.get(&mFilledSlots);
   sb.get(&mMaxSlots);
   sb.get(&mLocked);
   sb.get(&mInfo);

   void* p = static_cast<void*>(&mXnAddr);
   sb.get(&p, sizeof(XNADDR));

   p = static_cast<void*>(&mXnKey);
   sb.get(&p, sizeof(XNKEY));

   p = static_cast<void*>(&mXnKID);
   sb.get(&p, sizeof(XNKID));

   mUpdateTime = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLanGameInfo::setBadCRC()
{
   mBadCRC = true;
}

//==============================================================================
// 
//==============================================================================
BXLanDiscovery::BXLanDiscovery() :
   mTitleID(0),
   mChecksum(0),
   mPort(XNetwork::cLanDiscoveryPort),
   mBroadcastTimerSet(false),
   mListUpdated(false),
   mFirstBroadcastSent(false)
{
}

//==============================================================================
// 
//==============================================================================
BXLanDiscovery::~BXLanDiscovery()
{
}

//==============================================================================
// 
//==============================================================================
bool BXLanDiscovery::init(uint titleID, uint checksum)
{
   mTitleID = titleID;
   mChecksum = checksum;

   long port = XNetwork::cLanDiscoveryPort;

#ifndef BUILD_FINAL
   if (!gConfig.get(cConfigLanDiscoveryPort, &port))
      port = XNetwork::cLanDiscoveryPort;
#endif

   mPort = static_cast<uint16>(port);

   mBufferAllocator.clear();

   eventReceiverInit(cThreadIndexSim);

   // initialize the UDP thread
   // I may start receiving messages after this (on this thread) so be prepared
   if (!mUDPThread.init(mPort, &mBufferAllocator, &mBufferAllocator, getEventHandle(), false, cThreadIndexSimHelper))
      return false;

   initBroadcast();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::deinit()
{
   deinitBroadcast();

   mUDPThread.deinit();

   eventReceiverDeinit(true);
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::initialBroadcast()
{
   if (mFirstBroadcastSent)
      return;

   if (broadcastInfo())
      mFirstBroadcastSent = true;
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::initBroadcast()
{
   BASSERT(mBroadcastTimerSet == false);
   if (mBroadcastTimerSet)
      return;

   BASSERT(mBroadcastTimer.getHandle() != NULL);

   mBroadcastTimerSet = true;
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = getEventHandle();
   handleEvent.mToHandle = getEventHandle();
   handleEvent.mEventClass = cLanEventTimer;
   gEventDispatcher.registerHandleWithEvent(mBroadcastTimer.getHandle(), handleEvent);

   // broadcast any updated lan information every 5 seconds
   mBroadcastTimer.set(XNetwork::cLanBroadcastInterval);
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::deinitBroadcast()
{
   if (mBroadcastTimerSet)
   {
      mBroadcastTimer.cancel();
      mBroadcastTimerSet = false;
      gEventDispatcher.deregisterHandle(mBroadcastTimer.getHandle(), cThreadIndexSim);
   }
}

//==============================================================================
// 
//==============================================================================
bool BXLanDiscovery::broadcastInfo()
{
   if (mInfo.getNonce() == 0)
      return false;

   long size = mInfo.getMaxSerialSize();
   if ((size > cMaxSendSize) || (size <= 0))
   {
      nlogt(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
      return false;
   }

   BDEBUG_ASSERT(mInfo.getMaxSerialSize() <= cMaxBufferSize);

   BXNetBuffer* pBuf = mBufferAllocator.alloc();
   BDEBUG_ASSERT(pBuf);
   if (pBuf == NULL)
      return false;

   mInfo.serializeInto(pBuf->mBuf, &size);

   pBuf->mSize = static_cast<uint>(size);

   pBuf->mAddr.sin_addr.s_addr = INADDR_BROADCAST;
   pBuf->mAddr.sin_port = mPort;
   pBuf->mAddr.sin_family = AF_INET;

   gEventDispatcher.send(cInvalidEventReceiverHandle, mUDPThread.getEventHandle(), cNetEventUDPSend, (uint)pBuf);

   return true;
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::checkList()
{
   uint now = timeGetTime();

   // now see if we have this game in our list
   for (int i=mGamesList.getSize()-1; i >= 0; --i)
   {
      // check expirations at this time as well
      BLanGameInfo& game = mGamesList[i];

      if (now - game.getUpdateTime() >= XNetwork::cLanListTimeout)
      {
         mGamesList.removeIndex(i);
         mListUpdated = true;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BXLanDiscovery::addGame(const BLanGameInfo& info)
{
   if (info.getNonce() == 0 || info.getInfo().length() == 0 || info.getTitleID() != mTitleID)
      return;

   uint now = timeGetTime();

   bool found = false;
   bool updated = false;

   // now see if we have this game in our list
   for (int i=mGamesList.getNumber()-1; i >= 0; --i)
   {
      // check expirations at this time as well
      BLanGameInfo& game = mGamesList[i];

      if (game == info)
      {
         updated = game.update(info);
         found = true;
      }
      else if (now - game.getUpdateTime() >= XNetwork::cLanListTimeout)
      {
         mGamesList.removeIndex(i);
         mListUpdated = true;
      }
   }

   if (!found)
      mGamesList.add(info);

   if (!found || updated)
      mListUpdated = true;
}

//==============================================================================
// 
//==============================================================================
bool BXLanDiscovery::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cLanEventTimer:
         {
            IGNORE_RETURN(broadcastInfo());
            checkList();
            break;
         }

      case cNetEventUDPRecv:
         {
            // received a packet from the UDP thread, this is currently raw data
            // so we need to pass down to the connection so it can perform the
            // appropriate sequencing of the data
            BXNetBuffer* pBuf = reinterpret_cast<BXNetBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

            Utils::FastMemCpy(&mTempNetBuffer, pBuf, sizeof(BXNetBuffer));
            mBufferAllocator.free(pBuf);

            BNetIPString strAddr(mTempNetBuffer.mAddr);

            nlogt(cTransportNL, "BXLanDiscovery::receiveEvent cNetEventUDPRecv -- ip[%s] size[%d]", strAddr.getPtr(), mTempNetBuffer.mSize);

            long type = BTypedPacket::getType(mTempNetBuffer.mBuf);
            if (type == BPacketType::cLanInfoPacket)
            {
               // this should be a broadcast from another client hosting a game on the network
               BLanGameInfo info;
               info.deserializeFrom(mTempNetBuffer.mBuf, mTempNetBuffer.mSize);

               if (info.getChecksum() != mChecksum)
                  info.setBadCRC();

               addGame(info);
            }
            break;
         }

      case cNetEventSocketError:
         {
            // the socket had a problem sending/receiving data
            // event.mPrivateData == error value
            // event.mPrivateData2 == IN_ADDR value
            // if event.mPrivateData2 == 0, then there was an issue before we were able to assign an address
            // and therefore is probably a general socket failure so we should disconnect/return to a previous state
            // otherwise, we can trigger a socket disconnect up the chain
            if (event.mPrivateData2 == INADDR_LOOPBACK)
               break;
            break;
         }
   }

   return false;
}
