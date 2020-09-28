//==============================================================================
// NetPackets.cpp
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "NetPackets.h"
#include "Session.h"
#include "stream/dynamicStream.h"

//==============================================================================
// Defines
uint32 BLSPPacket::mNextId = 0;

//==============================================================================
// 
//==============================================================================
BChannelPacket::BChannelPacket() :
   BTypedPacket(BPacketType::cChannelPacket),
   mChannel(0),
   mType(0),
   mTimeOffset(0),
   mPacketID(0)
{
}

//==============================================================================
// 
//==============================================================================
BChannelPacket::BChannelPacket(long type) :
   BTypedPacket(BPacketType::cChannelPacket),
   mChannel(0),
   mType(static_cast<uint8>(type)),
   mTimeOffset(0),
   mPacketID(0)
{
   BASSERT(type < 255);
}

//==============================================================================
// 
//==============================================================================
BChannelPacket::~BChannelPacket()
{
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::setType(uint32 type)
{
   BASSERT(type < 255);

   mType = static_cast<uint8>(type);
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::setChannel(long channel)
{
   BASSERT((channel > 0)&&(channel < 255));

   mChannel = static_cast<uint8>(channel);
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::setPacketID(uint packetID)
{
   BASSERT((packetID > 0)&&(packetID <= XNetwork::cMaxPacketID));

   mPacketID = static_cast<uint16>(packetID);
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::setTimeOffset(uint32 offset)
{
   mTimeOffset = static_cast<uint16>(offset);
}

//==============================================================================
// 
//==============================================================================
namespace ChannelOffset
{
   enum
   {
      cChannel = 1,
      cPacketID = 2,
      cTime = 4,
      cSignatureSize = 6,
   };
}

//==============================================================================
// 
//==============================================================================
long BChannelPacket::getType(const void* pData)
{
   if (BTypedPacket::getType(pData) != BPacketType::cChannelPacket)
      return -1;

   return static_cast<long>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize()]) & 0xFF;
}

//==============================================================================
// 
//==============================================================================
long BChannelPacket::getChannel(const void* pData)
{
   return static_cast<long>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize() + ChannelOffset::cChannel]) & 0xFF;
}

//==============================================================================
// 
//==============================================================================
uint BChannelPacket::getPacketID(const void* pData)
{
   return (((static_cast<uint>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize() + ChannelOffset::cPacketID]) & 0xFF) << 8) |
            (static_cast<uint>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize() + ChannelOffset::cPacketID + 1]) & 0xFF));
}

//==============================================================================
// 
//==============================================================================
uint32 BChannelPacket::getTimeOffset(const void* pData)
{
   //if (BTypedPacket::getType(pData) != BPacketType::cChannelPacket)
   //   return 0;

   const uint32 p1 = static_cast<uint32>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize() + ChannelOffset::cTime]) & 0xFF;
   const uint32 p2 = static_cast<uint32>(static_cast<const uchar*>(pData)[BTypedPacket::getSignatureSize() + ChannelOffset::cTime + 1]) & 0xFF;

   return (p1 << 8) | p2;
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::serializeSignature(BSerialBuffer& sb)
{
   BTypedPacket::serializeSignature(sb);
   sb.add(mType);
   sb.add(mChannel);
   sb.add(mPacketID);
   sb.add(mTimeOffset);
}

//==============================================================================
// 
//==============================================================================
void BChannelPacket::deserializeSignature(BSerialBuffer& sb)
{
   BTypedPacket::deserializeSignature(sb);
   sb.get(&mType);
   sb.get(&mChannel);
   sb.get(&mPacketID);
   sb.get(&mTimeOffset);
}

//==============================================================================
// 
//==============================================================================
long BChannelPacket::getSignatureSize()
{
   return BTypedPacket::getSignatureSize()+ChannelOffset::cSignatureSize;
}

//==============================================================================
// 
//==============================================================================
BAddressesPacket::BAddressesPacket(long packetType) :
   BTypedPacket(packetType),
   mAmount(0)
{
   Utils::FastMemSet(mXnAddrs, 0, sizeof(XNADDR)*XNetwork::cMaxClients);
   Utils::FastMemSet(mMachineIDs, 0, sizeof(BMachineID)*XNetwork::cMaxClients);

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
      {
         mUsers[i][j].reset();
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BAddressesPacket::addAddress(BMachineID machineID, const XNADDR& xnaddr, const BSessionUser users[])
{
   BDEBUG_ASSERT(mAmount < XNetwork::cMaxClients);
   if (mAmount >= XNetwork::cMaxClients)
      return;

   Utils::FastMemCpy(&mXnAddrs[mAmount], &xnaddr, sizeof(XNADDR));                  

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[mAmount][i] = users[i];
   }

   mMachineIDs[mAmount] = machineID;

   ++mAmount;
}

//==============================================================================
// 
//==============================================================================
void BAddressesPacket::serialize(BSerialBuffer& sb)
{ 
   BTypedPacket::serialize(sb); 

   sb.add(mAmount);
   for (uint i=0; i < mAmount; i++)                  
   {
      sb.add(static_cast<void *>(&mXnAddrs[i]), sizeof(XNADDR));
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
         mUsers[i][j].serialize(sb);
      sb.add(mMachineIDs[i]);
   }
}

//==============================================================================
// 
//==============================================================================
void BAddressesPacket::deserialize(BSerialBuffer& sb)
{ 
   BTypedPacket::deserialize(sb); 

   sb.get(&mAmount);
   for (uint i=0; i < mAmount; i++)
   {
      void *p = static_cast<void *>(&mXnAddrs[i]);
      sb.get(&p, sizeof(XNADDR));
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
         mUsers[i][j].deserialize(sb);
      sb.get(&mMachineIDs[i]);
   }
}

//==============================================================================
// 
//==============================================================================
BTimeIntervalPacket::BTimeIntervalPacket() :
   BChannelPacket(),
   mInterval(0),
   mTiming(0)
{
}

//==============================================================================
// 
//==============================================================================
BTimeIntervalPacket::BTimeIntervalPacket(long type) :
   BChannelPacket(type),
   mInterval(0),
   mTiming(0)
{
}

//==============================================================================
// 
//==============================================================================
BTimeIntervalPacket::BTimeIntervalPacket(long type, uint32 interval, uint32 timing) :
   BChannelPacket(type),
   mInterval(static_cast<uint8>(interval)),
   mTiming(static_cast<uint8>(timing))
{
}

//==============================================================================
// 
//==============================================================================
void BTimeIntervalPacket::serialize(BSerialBuffer& sb)
{
   BChannelPacket::serialize(sb);
   sb.add(mInterval);
   sb.add(mTiming);
}

//==============================================================================
// 
//==============================================================================
void BTimeIntervalPacket::deserialize(BSerialBuffer& sb)
{
   BChannelPacket::deserialize(sb);
   sb.get(&mInterval);
   sb.get(&mTiming);
}

//==============================================================================
// 
//==============================================================================
BTimeSyncPacket::BTimeSyncPacket() :
   BChannelPacket(),
   mTime(0),
   mTiming(0)
{
}

//==============================================================================
// 
//==============================================================================
BTimeSyncPacket::BTimeSyncPacket(long type) :
   BChannelPacket(type),
   mTime(0),
   mTiming(0)
{
}

//==============================================================================
// 
//==============================================================================
BTimeSyncPacket::BTimeSyncPacket(long type, uint32 time, uint32 timing) :
   BChannelPacket(type),
   mTime(time),
   mTiming(static_cast<uint8>(timing))
{
}

//==============================================================================
// 
//==============================================================================
void BTimeSyncPacket::serialize(BSerialBuffer& sb)
{
   BChannelPacket::serialize(sb);
   sb.add(mTime);
   sb.add(mTiming);
}

//==============================================================================
// 
//==============================================================================
void BTimeSyncPacket::deserialize(BSerialBuffer& sb)
{
   BChannelPacket::deserialize(sb);
   sb.get(&mTime);
   sb.get(&mTiming);
}

//==============================================================================
// 
//==============================================================================
BInitPeersResponsePacket::BInitPeersResponsePacket(long packetType) :
   BTypedPacket(packetType),
   mXnAddr(),
   mStatus(0),
   mCount(0)
{
   IGNORE_RETURN(Utils::FastMemSet(mPeers, 0, sizeof(XNADDR)*XNetwork::cMaxClients));
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();
}

//==============================================================================
// 
//==============================================================================
void BInitPeersResponsePacket::setLocalInfo(const XNADDR& xnaddr, const BSessionUser users[])
{
   mXnAddr = xnaddr;

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i] = users[i];
   }
}

//==============================================================================
// 
//==============================================================================
void BInitPeersResponsePacket::addPeer(const XNADDR& xnaddr, bool status, const XNADDR* pProxyXnAddr)
{
   mPeers[mCount] = xnaddr;
   if (pProxyXnAddr != NULL)
      mProxy.add(xnaddr, *pProxyXnAddr);
   mStatus |= (status ? 1 : 0) << mCount;
   ++mCount;
   BDEBUG_ASSERT(mCount < XNetwork::cMaxClients);
}

//==============================================================================
// 
//==============================================================================
void BInitPeersResponsePacket::serialize(BSerialBuffer& sb)   
{ 
   BTypedPacket::serialize(sb); 
   sb.add(mCount);
   sb.add(mStatus);
   for (uint i=0; i < mCount; i++)
   {
      sb.add(static_cast<void*>(&mPeers[i]), sizeof(XNADDR));
   }

   mProxy.serialize(sb);

   // serialize the xnaddr/xuid/gamertag
   // todo : JAR use namespaced encapsulation of serialbuffer
   // to extend it for more exotic types (like XNADDR on the XBox360 platform)
   // this would eliminate the ugly and type-unsafe void * casting that's 
   // going on here.
   sb.add(static_cast<void*>(&mXnAddr), sizeof(XNADDR));

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i].serialize(sb);
   }
}

//==============================================================================
// 
//==============================================================================
void BInitPeersResponsePacket::deserialize(BSerialBuffer& sb) 
{ 
   BTypedPacket::deserialize(sb);          
   sb.get(&mCount);
   sb.get(&mStatus);
   for (uint i=0; i < mCount; i++)
   {
      void* p = static_cast<void*>(&mPeers[i]);
      sb.get(&p, sizeof(XNADDR));
   }

   mProxy.deserialize(sb);

   void* p = static_cast<void*>(&mXnAddr);
   sb.get(&p, sizeof(XNADDR));

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i].deserialize(sb);
   }
}

//==============================================================================
// 
//==============================================================================
BProxyPacket::BProxyPacket(long packetType) :
   BTypedPacket(packetType),
   mNonce(0),
   mStatus(0),
   mCount(0)
{
   IGNORE_RETURN(Utils::FastMemSet(mPeers, 0, sizeof(XNADDR)*XNetwork::cMaxClients));
   IGNORE_RETURN(Utils::FastMemSet(mPings, 0, sizeof(uint32)*XNetwork::cMaxClients));
}

//==============================================================================
// 
//==============================================================================
void BProxyPacket::addPeer(const XNADDR& xnaddr, bool status, uint32 avgPing, uint32 stdDev)
{
   mStatus |= (status ? 1 : 0) << mCount;

   mPeers[mCount] = xnaddr;
   mPings[mCount] = (stdDev << 16) | (avgPing & 0xFFFF);
   ++mCount;
   BDEBUG_ASSERT(mCount < XNetwork::cMaxClients);
}

//==============================================================================
// 
//==============================================================================
void BProxyPacket::setNonce(uint32 nonce)
{
   mNonce = nonce;
}

//==============================================================================
// 
//==============================================================================
void BProxyPacket::serialize(BSerialBuffer& sb)
{ 
   BTypedPacket::serialize(sb);
   sb.add(mNonce);
   sb.add(mStatus);
   sb.add(mCount);
   for (uint i=0; i < mCount; i++)
   {
      sb.add(static_cast<void*>(&mPeers[i]), sizeof(XNADDR));
   }
   for (uint i=0; i < mCount; i++)
   {
      sb.add(static_cast<void*>(&mPings[i]), sizeof(uint32));
   }
}

//==============================================================================
// 
//==============================================================================
void BProxyPacket::deserialize(BSerialBuffer& sb)
{ 
   BTypedPacket::deserialize(sb);
   sb.get(&mNonce);
   sb.get(&mStatus);
   sb.get(&mCount);
   for (uint i=0; i < mCount; i++)
   {
      void* p = static_cast<void*>(&mPeers[i]);
      sb.get(&p, sizeof(XNADDR));
   }

   for (uint i=0; i < mCount; i++)
   {
      sb.get(&mPings[i]);
   }
}

//==============================================================================
// 
//==============================================================================
BLSPPacket::BLSPPacket() :
   mId(0),
   mResult(S_OK),
   mServiceId(0),
   mType(0),
   mBaseVersion(cPacketVersion)
{}

//==============================================================================
// 
//==============================================================================
BLSPPacket::BLSPPacket(uint8 serviceId, uint8 packetType) :
   mId(0),
   mResult(S_OK),
   mServiceId(serviceId),
   mType(packetType),
   mBaseVersion(cPacketVersion)
{}

//==============================================================================
// 
//==============================================================================
void BLSPPacket::serialize(BSerialBuffer& sb)
{
   mId = mNextId++;
   sb.add(mId);
   sb.add(mServiceId);
   sb.add(mType);
   sb.add(mBaseVersion);
   sb.add(mResult);
}

//==============================================================================
// 
//==============================================================================
void BLSPPacket::deserialize(BSerialBuffer& sb)
{
   sb.get(&mId);
   sb.get(&mServiceId);
   sb.get(&mType);
   sb.get(&mBaseVersion);
   sb.get(&mResult);
}

//==============================================================================
// 
//==============================================================================
uint32 BLSPPacket::getID() const
{
   return mId;
}

//==============================================================================
// 
//==============================================================================
uint8 BLSPPacket::getType() const
{
   return mType;
}

//==============================================================================
// 
//==============================================================================
uint8 BLSPPacket::getServiceId() const
{
   return mServiceId;
}

//==============================================================================
// 
//==============================================================================
uint8 BLSPPacket::getVersion() const
{
   return mBaseVersion;
}

//==============================================================================
// 
//==============================================================================
int32 BLSPPacket::getResult() const
{
   return mResult;
}

//==============================================================================
// 
//==============================================================================
void BLSPPacket::setServiceId(uint8 serviceId)
{
   mServiceId = serviceId;
}

//==============================================================================
// 
//==============================================================================
BMediaPacket::BMediaPacket() :
   BLSPPacket(0, BPacketType::cMediaCommandPacket),
   mCommand(cCommandInvalid),
   mXuid(0),
   mMediaID(0),
   mStreamID(0),
   mCRC(0),
   mTTL(0),
   mpData(NULL),
   mSize(0),
   mVersion(cMediaPacketVersion)
{}

//==============================================================================
// 
//==============================================================================
BMediaPacket::BMediaPacket(uint8 serviceId) :
   BLSPPacket(serviceId, BPacketType::cMediaCommandPacket),
   mCommand(cCommandInvalid),
   mXuid(0),
   mMediaID(0),
   mStreamID(0),
   mCRC(0),
   mTTL(0),
   mpData(NULL),
   mSize(0),
   mVersion(cMediaPacketVersion)
{
}

//==============================================================================
// 
//==============================================================================
BMediaPacket::BMediaPacket(uint8 serviceId, eCommands command) :
   BLSPPacket(serviceId, BPacketType::cMediaCommandPacket),
   mCommand(static_cast<int8>(command)),
   mXuid(0),
   mMediaID(0),
   mStreamID(0),
   mCRC(0),
   mTTL(0),
   mpData(NULL),
   mSize(0),
   mVersion(cMediaPacketVersion)
{
}

//==============================================================================
// 
//==============================================================================
BMediaPacket::BMediaPacket(uint8 serviceId, eCommands command, const XUID xuid, const uint64 mediaID) :
   BLSPPacket(serviceId, BPacketType::cMediaCommandPacket),
   mCommand(static_cast<int8>(command)),
   mXuid(xuid),
   mMediaID(mediaID),
   mStreamID(0),
   mCRC(0),
   mTTL(0),
   mpData(NULL),
   mSize(0),
   mVersion(cMediaPacketVersion)
{
}

//==============================================================================
// 
//==============================================================================
void BMediaPacket::init(eCommands command, const XUID xuid, const uint64 mediaID, const uint32 crc)
{
   mCommand = static_cast<int8>(command);
   mXuid = xuid,
   mMediaID = mediaID;
   mCRC = crc;
}

//==============================================================================
// 
//==============================================================================
uint32 BMediaPacket::getTTL() const
{
   return mTTL;
}

//==============================================================================
// 
//==============================================================================
void BMediaPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);
   sb.add(mVersion);
   sb.add(mCommand);
   sb.add(mMediaID);
   sb.add(mStreamID);
   sb.add(mXuid);
   sb.add(mCRC);
   sb.add(mTTL);
   if (mpData == NULL)
      mSize = 0;
   sb.add(mSize);
   if (mSize > 0)
      sb.add(mpData, mSize);
}

//==============================================================================
// 
//==============================================================================
void BMediaPacket::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);
   sb.get(&mVersion);
   sb.get(&mCommand);
   sb.get(&mMediaID);
   sb.get(&mStreamID);
   sb.get(&mXuid);
   sb.get(&mCRC);
   sb.get(&mTTL);
   sb.get(&mSize);
   if (mSize > 0)
   {
      int32 size = static_cast<int32>(mSize);
      sb.getPointer(&mpData, &size);
      mSize = static_cast<uint16>(size);
   }
}

//==============================================================================
// 
//==============================================================================
BServiceRecordPacket::BServiceRecordPacket() :
   BLSPPacket(0, BPacketType::cServiceRecordPacket),
   mCommand(cCommandInvalid),
   mXuid(0),
   mTTL(0),
   mpData(NULL),
   mSize(0)
{}

//==============================================================================
// 
//==============================================================================
BServiceRecordPacket::BServiceRecordPacket(uint8 serviceId) :
   BLSPPacket(serviceId, BPacketType::cServiceRecordPacket),
   mCommand(cCommandInvalid),
   mXuid(0),
   mTTL(0),
   mpData(NULL),
   mSize(0)
{
}

//==============================================================================
// 
//==============================================================================
BServiceRecordPacket::BServiceRecordPacket(uint8 serviceId, eCommands command) :
   BLSPPacket(serviceId, BPacketType::cServiceRecordPacket),
   mCommand(static_cast<int8>(command)),
   mXuid(0),
   mTTL(0),
   mpData(NULL),
   mSize(0)
{
}

//==============================================================================
// 
//==============================================================================
BServiceRecordPacket::BServiceRecordPacket(uint8 serviceId, eCommands command, const XUID xuid) :
   BLSPPacket(serviceId, BPacketType::cServiceRecordPacket),
   mCommand(static_cast<int8>(command)),
   mXuid(xuid),
   mTTL(0),
   mpData(NULL),
   mSize(0)
{
}

//==============================================================================
// 
//==============================================================================
void BServiceRecordPacket::init(eCommands command, const XUID xuid)
{
   mCommand = static_cast<int8>(command);
   mXuid = xuid;
}

//==============================================================================
// 
//==============================================================================
uint32 BServiceRecordPacket::getTTL() const
{
   return mTTL;
}

//==============================================================================
// 
//==============================================================================
void BServiceRecordPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);
   sb.add(mCommand);
   sb.add(mXuid);
   sb.add(mTTL);
   if (mpData == NULL)
      mSize = 0;
   sb.add(mSize);
   if (mSize > 0)
      sb.add(mpData, mSize);
}

//==============================================================================
// 
//==============================================================================
void BServiceRecordPacket::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);
   sb.get(&mCommand);
   sb.get(&mXuid);
   sb.get(&mTTL);
   sb.get(&mSize);
   if (mSize > 0)
   {
      int32 size = static_cast<int32>(mSize);
      sb.getPointer(&mpData, &size);
      mSize = static_cast<uint16>(size);
   }
}

//==============================================================================
// 
//==============================================================================
BAuthUser::BAuthUser() :
   mXuid(0),
   mBanMedia(true),
   mBanMatchMaking(true),
   mBanEverything(true)
{
}

//==============================================================================
// 
//==============================================================================
BAuthUser::BAuthUser(XUID xuid, BSimString& gamerTag) :
   mXuid(xuid),
   mGamerTag(gamerTag),
   mBanMedia(true),
   mBanMatchMaking(true),
   mBanEverything(true)
{
}

//==============================================================================
// 
//==============================================================================
void BAuthUser::serialize(BSerialBuffer& sb)
{
   sb.add(mXuid);
   sb.add<BSimString>(mGamerTag);
   sb.add(mBanMedia);
   sb.add(mBanMatchMaking);
   sb.add(mBanEverything);
}

//==============================================================================
// 
//==============================================================================
void BAuthUser::deserialize(BSerialBuffer& sb)
{
   sb.get(&mXuid);
   sb.get<BSimString>(&mGamerTag);
   sb.get(&mBanMedia);
   sb.get(&mBanMatchMaking);
   sb.get(&mBanEverything);
}

//==============================================================================
// 
//==============================================================================
void BAuthUser::setXuid(XUID xuid)
{
   mXuid = xuid;
}

//==============================================================================
// 
//==============================================================================
XUID BAuthUser::getXuid() const
{
   return mXuid;
}

//==============================================================================
// 
//==============================================================================
const BSimString& BAuthUser::getGamerTag() const
{
   return mGamerTag;
}

//==============================================================================
// 
//==============================================================================
bool BAuthUser::isBanMedia() const
{
   return mBanMedia;
}

//==============================================================================
// 
//==============================================================================
bool BAuthUser::isBanMatchMaking() const
{
   return mBanMatchMaking;
}

//==============================================================================
// 
//==============================================================================
bool BAuthUser::isBanEverything() const
{
   return mBanEverything;
}

//==============================================================================
// 
//==============================================================================
BAuthPacket::BAuthPacket() :
   BLSPPacket(0, BPacketType::cAuthPacket),
   mCommand(cCommandInvalid),
   mTTL(0)
{
}

//==============================================================================
// 
//==============================================================================
BAuthPacket::BAuthPacket(const uint8 serviceId, const eCommands command) :
   BLSPPacket(serviceId, BPacketType::cAuthPacket),
   mCommand(command),
   mTTL(0)
{
}

//==============================================================================
// 
//==============================================================================
BAuthPacket::~BAuthPacket()
{
}

//==============================================================================
// 
//==============================================================================
void BAuthPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);
   sb.add(static_cast<int8>(mCommand));
   sb.add(mTTL);

   mMachine.serialize(sb);

   sb.add(static_cast<int8>(mUsers.getSize()));
   for (uint i=0; i < mUsers.getSize(); ++i)
   {
      mUsers[i].serialize(sb);
   }
}

//==============================================================================
// 
//==============================================================================
void BAuthPacket::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);
   int8 cmd;
   sb.get(&cmd);
   mCommand = static_cast<eCommands>(cmd);

   sb.get(&mTTL);

   mMachine.deserialize(sb);

   int8 count = 0;
   sb.get(&count);
   for (int8 i=0; i < count; ++i)
   {
      BAuthUser user;
      user.deserialize(sb);
      mUsers.add(user);
   }
}

//==============================================================================
// 
//==============================================================================
BAuthPacket::eCommands BAuthPacket::getCommand() const
{
   return mCommand;
}

//==============================================================================
// 
//==============================================================================
uint BAuthPacket::getTTL() const
{
   return mTTL;
}

//==============================================================================
// 
//==============================================================================
const BAuthUser& BAuthPacket::getMachine() const
{
   return mMachine;
}

//==============================================================================
// 
//==============================================================================
const BSmallDynamicSimArray<BAuthUser>& BAuthPacket::getUsers() const
{
   return mUsers;
}

//==============================================================================
// 
//==============================================================================
void BAuthPacket::setMachine(XUID xuid)
{
   mMachine.setXuid(xuid);
}

//==============================================================================
// 
//==============================================================================
void BAuthPacket::addUser(XUID xuid, BSimString& gamerTag)
{
   mUsers.add(BAuthUser(xuid, gamerTag));
}

//==============================================================================
// 
//==============================================================================
//BConfigDataRequestPacket::BConfigDataRequestPacket() :
//   BLSPPacket(0, BPacketType::cConfigDataRequestPacket),
//   mStaticVersion(0),
//   mDynamicVersion(0)
//{
//}

//==============================================================================
// 
//==============================================================================
BConfigDataRequestPacket::BConfigDataRequestPacket(uint8 serviceId, WORD staticVersion, WORD dynamicVersion) :
   BLSPPacket(serviceId, BPacketType::cConfigDataRequestPacket),
   mStaticVersion(staticVersion),
   mDynamicVersion(dynamicVersion)
{
}

//==============================================================================
// 
//==============================================================================
BConfigDataRequestPacket::~BConfigDataRequestPacket()
{
}

//==============================================================================
// 
//==============================================================================
void BConfigDataRequestPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);
   sb.add(mStaticVersion);
   sb.add(mDynamicVersion);
}

//==============================================================================
// 
//==============================================================================
void BConfigDataRequestPacket::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);
   sb.get(&mStaticVersion);
   sb.get(&mDynamicVersion);
}

//==============================================================================
// 
//==============================================================================
BConfigDataResponsePacket::BConfigDataResponsePacket() :
   BLSPPacket(0, BPacketType::cConfigDataResponsePacket),
   mTTL(60*60*1000),
   mStaticTTL(60*60*1000),
   mDynamicTTL(60*60*1000),
   mMaxMatchmakingTimeTotal(0),
   mGlobalUserCount(0),
   mPreferredPing(0),
   mMaxPing(0),
   mStaticVersion(-1),
   mDynamicVersion(-1),
   mTwoStageQOSEnabled(false)
{
}

//==============================================================================
// 
//==============================================================================
BConfigDataResponsePacket::~BConfigDataResponsePacket()
{
}

//==============================================================================
// 
//==============================================================================
void BConfigDataResponsePacket::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);

   while (sb.getDataAvailable() > 0)
   {
      byte responseType;
      sb.get(&responseType);

      if (responseType == cStatic)
      {
         //writer.Write(Convert.ToByte(eConfigDataResponseType.cStatic));
         //writer.Write(Xbox_EndianSwap.endSwapI16(_staticVersion));
         //writer.Write(Xbox_EndianSwap.endSwapI32(mMaxMatchMakingTimeTotal));
         //writer.Write(Xbox_EndianSwap.endSwapI16(mPreferredPing));
         //writer.Write(Xbox_EndianSwap.endSwapI16(mMaxPing));
         //writer.Write(mTwoStageQOSEnabled);
         //writer.Write((byte)_entries.Count);
         //foreach (Entry entry in _entries)
         //{
         //   writer.Write(Xbox_EndianSwap.endSwapI32(entry.hostDelayTimeBase));
         //   writer.Write(Xbox_EndianSwap.endSwapI32(entry.hostDelayTimeRandom));
         //   writer.Write(Xbox_EndianSwap.endSwapI32(entry.hostTime));
         //   writer.Write(Xbox_EndianSwap.endSwapI16(entry.hopperIndex));
         //   writer.Write(entry.fastScanSearchCount);
         //   writer.Write(entry.normalSearchCount);
         //   writer.Write(entry.enabled);
         //}
         sb.get(&mStaticVersion);
         sb.get(&mStaticTTL);
         sb.get(&mMaxMatchmakingTimeTotal);
         sb.get(&mPreferredPing);
         sb.get(&mMaxPing);
         sb.get(&mTwoStageQOSEnabled);
         byte entryCount = 0;
         sb.get(&entryCount);
         for (uint i=0; i < entryCount; ++i)
         {
            BHopperEntry entry;
            sb.get(&entry.mHostDelayTimeBase);
            sb.get(&entry.mHostDelayTimeRandom);
            sb.get(&entry.mHostTime);
            sb.get(&entry.mHopperIndex);
            sb.get(&entry.mFastScanSearchCount);
            sb.get(&entry.mNormalSearchCount);
            sb.get(&entry.mEnabled);
            mEntries.add(entry);
         }
      }
      else if (responseType == cDynamic)
      {
         //writer.Write(Convert.ToByte(eConfigDataResponseType.cDynamic));
         //writer.Write(Xbox_EndianSwap.endSwapI16(_dynamicVersion));
         //writer.Write(Xbox_EndianSwap.endSwapI32(mGlobalUserCount));
         //writer.Write((byte)_entries.Count);
         //foreach (Entry entry in _entries)
         //{
         //   writer.Write(Xbox_EndianSwap.endSwapI32(entry.averageWait));
         //   writer.Write(Xbox_EndianSwap.endSwapI16(entry.hopperIndex));
         //   writer.Write(Xbox_EndianSwap.endSwapI16(entry.userCount));
         //}
         sb.get(&mDynamicVersion);
         sb.get(&mDynamicTTL);
         sb.get(&mGlobalUserCount);
         byte entryCount = 0;
         sb.get(&entryCount);
         for (uint i=0; i < entryCount; ++i)
         {
            BHopperEntry entry;
            uint averageWait = 0;
            ushort hopperIndex = 0;
            sb.get(&averageWait);
            sb.get(&hopperIndex);

            if (mEntries.getSize() > hopperIndex)
            {
               BHopperEntry& entry = mEntries.get(hopperIndex);
               entry.mAverageWait = averageWait;
               sb.get(&entry.mUserCount);
            }
            else
            {
               BHopperEntry entry;
               entry.mAverageWait = averageWait;
               entry.mHopperIndex = hopperIndex;
               sb.get(&entry.mUserCount);
               mEntries.add(entry);
            }
         }
      }
      else if (responseType == cNoData)
      {
         sb.get(&mTTL);
      }
      else
      {
         // no idea, let's stop trying to parse this buffer
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
BConfigDataResponsePacketV2::BConfigDataResponsePacketV2() :
BLSPPacket(0, BPacketType::cConfigDataResponsePacket),
mTTL(60*60*1000),
mStaticTTL(60*60*1000),
mDynamicTTL(60*60*1000),
mGlobalUserCount(0),
mStaticVersion(-1),
mStaticDataSize(0),
mStaticData(0),
mDynamicVersion(-1)
{
}

//==============================================================================
// 
//==============================================================================
BConfigDataResponsePacketV2::~BConfigDataResponsePacketV2()
{
}

//==============================================================================
// 
//==============================================================================
void BConfigDataResponsePacketV2::deserialize(BSerialBuffer& sb)
{
   BLSPPacket::deserialize(sb);

   while (sb.getDataAvailable() > 0)
   {
      byte responseType;
      sb.get(&responseType);

      if (responseType == cStatic)
      {
         sb.get(&mStaticVersion);
         sb.get(&mStaticTTL);
         sb.get(&mStaticDataSize);
         BASSERT(mStaticDataSize>0);
         BASSERT(mStaticDataSize<20000);  //Sanity check
         //There are bug reports that sometimes this fails (maybe LSP sends partial packet?
         // So lets make it safe to be sure in case it ever happens in production
         if ((mStaticDataSize<0) || (mStaticDataSize>20000))
         {
            mStaticData = NULL;
            mStaticDataSize = 0;
            return;
         }
         sb.getPointer(&mStaticData, &mStaticDataSize);
         //If the read failed the buffer will be empty, adjust the size accordingly
         if (mStaticData==NULL)
         {
            mStaticDataSize = 0;
         }
      }
      else if (responseType == cDynamic)
      {
         sb.get(&mDynamicVersion);
         sb.get(&mDynamicTTL);
         sb.get(&mGlobalUserCount);
         byte entryCount = 0;
         sb.get(&entryCount);
         for (uint i=0; i < entryCount; ++i)
         {
            BHopperEntry entry;
            sb.get(&entry.mHopperIndex);
            sb.get(&entry.mUserCount);
            mEntries.add(entry);
         }
      }
      else if (responseType == cNoData)
      {
         sb.get(&mTTL);
      }
      else
      {
         // no idea, let's stop trying to parse this buffer
         return;
      }
   }
}

//==============================================================================
//
//==============================================================================
BPerfLogPacket::BPerfLogPacket(uint8 serviceId) :
   BLSPPacket(serviceId, BPacketType::cPerfLogPacket),
   mEventCode(0),
   mHopperIndex(0),
   mPartyCount(0),
   mTotalRunTime(0),
   mHostDelayTime(0),
   mHostTime(0),
   mHostAttemptedJoins(0),
   mHostSuccessfulJoins(0),
   mAverageGameCount(0),
   mAverageSigma(0.0f),
   mAverageMu(0.0f),
   mAverageRating(0),
   mMatchMakingNonce(0)
{
   Utils::FastMemSet(&mXuids, 0, sizeof(XUID)*cPerfLogMaxXuids);
}

//==============================================================================
//
//==============================================================================
BPerfLogPacket::BPerfLogPacket() :
   BLSPPacket(0, BPacketType::cPerfLogPacket),
   mEventCode(0),
   mHopperIndex(0),
   mPartyCount(0),
   mTotalRunTime(0),
   mHostDelayTime(0),
   mHostTime(0),
   mHostAttemptedJoins(0),
   mHostSuccessfulJoins(0),
   mAverageGameCount(0),
   mAverageSigma(0.0f),
   mAverageMu(0.0f),
   mAverageRating(0),
   mMatchMakingNonce(0)
{
   Utils::FastMemSet(&mXuids, 0, sizeof(XUID)*cPerfLogMaxXuids);
}

//==============================================================================
//
//==============================================================================
BPerfLogPacket::~BPerfLogPacket()
{
}

//==============================================================================
//
//==============================================================================
void BPerfLogPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);

   sb.add(mEventCode);
   sb.add(mHopperIndex);
   sb.add(mPartyCount);
   sb.add(mTotalRunTime);     
   sb.add(mFastScanData.mResultCount);
   sb.add(mFastScanData.mPostQOSResultCount);
   sb.add(mFastScanData.mConnectionAttempts);
   sb.add(mFastScanData.mSearchTime);
   sb.add(mFastScanData.mScanTime);
   sb.add(mFullScanData.mResultCount);
   sb.add(mFullScanData.mPostQOSResultCount);
   sb.add(mFullScanData.mConnectionAttempts);
   sb.add(mFullScanData.mSearchTime);
   sb.add(mFullScanData.mScanTime);
   sb.add(mHostDelayTime);
   sb.add(mHostTime);
   sb.add(mHostAttemptedJoins);
   sb.add(mHostSuccessfulJoins);
   sb.add(mAverageGameCount);
   sb.add(mAverageSigma);
   sb.add(mAverageMu);
   sb.add(mAverageRating);
   sb.add(mMatchMakingNonce);  
   for (uint i=0; i < cPerfLogMaxXuids; ++i)
      sb.add(mXuids[i]);
}


//==============================================================================
//
//==============================================================================
BPerfLogPacketV2::BPerfLogPacketV2(uint8 serviceId) :
BLSPPacket(serviceId, BPacketType::cPerfLogPacketV2),
mMatchMakingNonce(0),
mLaunchedMatchHostNonce(0),
mTotalRunTime(0),
mHostDelayTime(0),
mHostTime(0),
mFastScanTime(0),
mSearchTime(0),
mAverageGameCount(0),
mLaunchedMatchPingAverage(0),
mAverageRating(0),
mEventCode(0),
mHopperIndex(0),
mCycleCount(0),
mLaunchedMatchSkillAverage(0),
mLaunchedMatchRatingQuality(0)
{
   Utils::FastMemSet(&mXuids, 0, sizeof(XUID)*cPerfLogMaxXuids);
}

//==============================================================================
//
//==============================================================================
BPerfLogPacketV2::BPerfLogPacketV2() :
BLSPPacket(0, BPacketType::cPerfLogPacketV2),
mMatchMakingNonce(0),
mLaunchedMatchHostNonce(0),
mTotalRunTime(0),
mHostDelayTime(0),
mHostTime(0),
mFastScanTime(0),
mSearchTime(0),
mAverageGameCount(0),
mLaunchedMatchPingAverage(0),
mAverageRating(0),
mEventCode(0),
mHopperIndex(0),
mCycleCount(0),
mLaunchedMatchSkillAverage(0),
mLaunchedMatchRatingQuality(0)
{
   Utils::FastMemSet(&mXuids, 0, sizeof(XUID)*cPerfLogMaxXuids);
}

//==============================================================================
//
//==============================================================================
BPerfLogPacketV2::~BPerfLogPacketV2()
{
}

//==============================================================================
//
//==============================================================================
void BPerfLogPacketV2::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);

   sb.add(mEventCode);
   sb.add(mHopperIndex);   
   sb.add(mCycleCount);
   sb.add(mFastScanData.mResultCount);
   sb.add(mFastScanData.mPostQOSResultCount);
   sb.add(mFastScanData.mConnectionAttempts);
   sb.add(mSearchData.mResultCount);
   sb.add(mSearchData.mPostQOSResultCount);
   sb.add(mSearchData.mConnectionAttempts);
   sb.add(mTotalRunTime); 
   sb.add(mHostDelayTime);
   sb.add(mHostTime);
   sb.add(mFastScanTime);
   sb.add(mSearchTime);
   sb.add(mAverageGameCount);
   sb.add(mAverageRating);
   sb.add(mMatchMakingNonce);  
   sb.add(mLaunchedMatchHostNonce);
   sb.add(mLaunchedMatchPingAverage);
   sb.add(mLaunchedMatchSkillAverage);
   sb.add(mLaunchedMatchRatingQuality);
   for (uint i=0; i < cPerfLogMaxXuids; ++i)
      sb.add(mXuids[i]);
}

//==============================================================================
//Helper function to convert time in milliseconds to time in 1/10th of a second (capped to uint16 range)
//==============================================================================
void BPerfLogPacketV2::setTimeFromMSTime( uint16* targetValue, uint timeValue)
{
   uint setTime = uint((timeValue+500) / 100);
   if (setTime>UINT16_MAX)
      *targetValue = UINT16_MAX;
   else
      *targetValue = (uint16)setTime;
}

#pragma warning(push)
#pragma warning(disable : 4244) // conversion from int to uint16, possible loss of data
//==============================================================================
//Helper function to convert time in milliseconds to time in 1/10th of a second (capped to uint16 range) and add to exsisting value
//==============================================================================
void BPerfLogPacketV2::addTimeFromMSTime( uint16* targetValue, uint timeValue)
{
   uint setTime = uint((timeValue+500) / 100);
   if ((setTime+*targetValue)>UINT16_MAX)
      *targetValue = UINT16_MAX;
   else
      *targetValue += (uint16)setTime;
}
#pragma warning(pop)

//==============================================================================
//
//==============================================================================
BRequestFileUploadPacket::BRequestFileUploadPacket(const uint8 serviceID, const BSimString& gamerTag, const XUID id, const BSimString& fileName, uint fileSize) :
   BLSPPacket(serviceID, BPacketType::cRequestFileUploadPacket),
   mXuid(id),
   mGamerTag(gamerTag),
   mFileName(fileName),
   mFileSize(fileSize)
{
}

//==============================================================================
//
//==============================================================================
BRequestFileUploadPacket::~BRequestFileUploadPacket()
{
}

//==============================================================================
//
//==============================================================================
void BRequestFileUploadPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);
   sb.add(mGamerTag);
   sb.add(mXuid);
   sb.add(mFileName);
   sb.add(mFileSize);
}

//==============================================================================
//
//==============================================================================
BFileUploadBlockPacket::BFileUploadBlockPacket(uint8 serviceID, BStream& stream) :
   BLSPPacket(serviceID, BPacketType::cFileUploadBlockPacket),
   mStream(stream)
{
}

//==============================================================================
//
//==============================================================================
BFileUploadBlockPacket::~BFileUploadBlockPacket()
{
}

//==============================================================================
//
//==============================================================================
void BFileUploadBlockPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);

   uint8 sendBuffer[1024];

   // read the next 1k block from the stream
   const uint64 bytesLeft = mStream.bytesLeft();
   uint16 bytesToWrite = 1024;
   if (bytesLeft < 1024)
      bytesToWrite = static_cast<uint16>(bytesLeft);
   mStream.readBytes(&sendBuffer[0], bytesToWrite);
   sb.add(bytesToWrite);
   sb.addData(sendBuffer, bytesToWrite);
}

//==============================================================================
//
//==============================================================================
BFileUploadCompletePacket::BFileUploadCompletePacket(uint8 serviceID) :
   BLSPPacket(serviceID, BPacketType::cFileUploadCompletePacket)
{
}

//==============================================================================
//
//==============================================================================
BFileUploadCompletePacket::~BFileUploadCompletePacket()
{
}

//==============================================================================
// 
//==============================================================================
BTalkersPacket::BTalkersPacket() :
   BTypedPacket(BPacketType::cTalkersPacket),
   mOwnerXuid(0),
   mCount(0)
{
   IGNORE_RETURN(Utils::FastMemSet(mXuids, 0, sizeof(XUID)*cMaxClients));
}

//==============================================================================
// 
//==============================================================================
BTalkersPacket::BTalkersPacket(const XUID ownerXuid, const XUID* const pXuids) :
   BTypedPacket(BPacketType::cTalkersPacket),
   mOwnerXuid(ownerXuid),
   mCount(0)
{
   IGNORE_RETURN(Utils::FastMemSet(mXuids, 0, sizeof(XUID)*cMaxClients));

   long count=0;
   for (uint32 i=0; i < cMaxClients; ++i)
   {
      if (pXuids[i] != 0)
         mXuids[count++] = pXuids[i];
   }
   mCount = count;
}

//==============================================================================
// 
//==============================================================================
void BTalkersPacket::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);
   sb.add(mOwnerXuid);
   sb.add(mCount);
   for (uint32 i=0, j=0; i < cMaxClients && j < mCount; ++i)
   {
      if (mXuids[i] != 0)
      {
         sb.add(mXuids[i]);
         ++j;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BTalkersPacket::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);
   sb.get(&mOwnerXuid);
   sb.get(&mCount);
   for (uint32 i=0; i < mCount && i < cMaxClients; ++i)
      sb.get(&mXuids[i]);
}

//==============================================================================
// 
//==============================================================================
BVoiceHeadsetPacket::BVoiceHeadsetPacket() :
   BTypedPacket(BPacketType::cVoiceHeadsetPacket),
   mClientID(XNetwork::cMaxClients),
   mHeadset(FALSE)
{
}

//==============================================================================
// 
//==============================================================================
BVoiceHeadsetPacket::BVoiceHeadsetPacket(const uint clientID, const BOOL headset) :
   BTypedPacket(BPacketType::cVoiceHeadsetPacket),
   mClientID(clientID),
   mHeadset(headset)
{
}

//==============================================================================
// 
//==============================================================================
void BVoiceHeadsetPacket::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);
   sb.add(mClientID);
   sb.add(mHeadset);
}

//==============================================================================
// 
//==============================================================================
void BVoiceHeadsetPacket::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);
   sb.get(&mClientID);
   sb.get(&mHeadset);
}

//==============================================================================
// 
//==============================================================================
BSessionConnectorPacket::BSessionConnectorPacket(long packetType, long result) :
   BTypedPacket(packetType),
   mResult(result),
   mCRC(0),
   mMachineID(BMachine::cInvalidMachineID)
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();
}

//==============================================================================
// 
//==============================================================================
BSessionConnectorPacket::BSessionConnectorPacket(long packetType, long result, BMachineID machineID, const BSessionUser users[]) :
   BTypedPacket(packetType),
   mResult(result),
   mCRC(0),
   mMachineID(static_cast<int8>(machineID))
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i] = users[i];
   }
}

//==============================================================================
// 
//==============================================================================
BSessionConnectorPacket::BSessionConnectorPacket(long packetType, const XNADDR& xnaddr, BMachineID machineID, const BSessionUser users[]) :
   BTypedPacket(packetType),
   mXnAddr(xnaddr),
   mResult(0),
   mCRC(0),
   mMachineID(static_cast<int8>(machineID))
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i] = users[i];
   }
}

//==============================================================================
// 
//==============================================================================
BSessionConnectorPacket::BSessionConnectorPacket(long packetType, const XNADDR& xnaddr, BMachineID machineID, const BSessionUser users[], const BProxyInfo& proxyInfo) :
   BTypedPacket(packetType),
   mXnAddr(xnaddr),
   mResult(0),
   mCRC(0),
   mMachineID(static_cast<int8>(machineID))
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i] = users[i];
   }

   mProxy = proxyInfo;
}

//==============================================================================
// 
//==============================================================================
BSessionConnectorPacket::BSessionConnectorPacket(long packetType, DWORD crc, const XNADDR& xnaddr, const BSessionUser users[]) :
   BTypedPacket(packetType),
   mXnAddr(xnaddr),
   mResult(0),
   mCRC(crc),
   mMachineID(BMachine::cInvalidMachineID)
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      mUsers[i] = users[i];
   }
}

//==============================================================================
// 
//==============================================================================
void BSessionConnectorPacket::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);

   sb.add((void *)&mXnAddr, sizeof(XNADDR));
   sb.add(mResult);
   sb.add(mCRC);
   sb.add(mMachineID);

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].serialize(sb);

   mProxy.serialize(sb);
}

//==============================================================================
// 
//==============================================================================
void BSessionConnectorPacket::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);

   void* p = (void *)&mXnAddr;
   sb.get(&p, sizeof(XNADDR));
   sb.get(&mResult);
   sb.get(&mCRC);
   sb.get(&mMachineID);

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].deserialize(sb);

   mProxy.deserialize(sb);
}

//==============================================================================
// 
//==============================================================================
BDisconnectPacket::BDisconnectPacket() :
   BTypedPacket()
{
}

//==============================================================================
// 
//==============================================================================
BDisconnectPacket::BDisconnectPacket(long packetType) :
   BTypedPacket(packetType)
{
}

//==============================================================================
// 
//==============================================================================
void BDisconnectPacket::init(const BChannelTracker& tracker)
{
   mChannelTracker = tracker;
}

//==============================================================================
// 
//==============================================================================
void BDisconnectPacket::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);

   mChannelTracker.serialize(sb);
}

//==============================================================================
// 
//==============================================================================
void BDisconnectPacket::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);

   mChannelTracker.deserialize(sb);
}

//==============================================================================
// 
//==============================================================================
BDisconnectSyncPacket::BDisconnectSyncPacket() :
   BTypedPacket(BPacketType::cDisconnectSync),
   mMachineID(BMachine::cInvalidMachineID),
   mSize(0),
   mpData(NULL)
{
}

//==============================================================================
// 
//==============================================================================
void BDisconnectSyncPacket::init(BMachineID machineID, uint size, const void* pData)
{
   mMachineID = machineID;
   mSize = static_cast<uint16>(size);
   mpData = pData;
}

//==============================================================================
// 
//==============================================================================
void BDisconnectSyncPacket::serialize(BSerialBuffer& sb)
{
   BTypedPacket::serialize(sb);

   sb.add(mMachineID);
   if (mpData == NULL)
      mSize = 0;
   sb.add(mSize);
   if (mSize > 0)
      sb.add(mpData, mSize);
}

//==============================================================================
// 
//==============================================================================
void BDisconnectSyncPacket::deserialize(BSerialBuffer& sb)
{
   BTypedPacket::deserialize(sb);

   sb.get(&mMachineID);
   sb.get(&mSize);
   if (mSize > 0)
   {
      int32 size = static_cast<int32>(mSize);
      sb.getPointer(&mpData, &size);
      mSize = static_cast<uint16>(size);
   }
}

//==============================================================================
// 
//==============================================================================
BMessagePacket::BMessagePacket(long packetType) :
   BChannelPacket(packetType),
   mMessage(0),
   mPlayerID(-1)
{
}

//==============================================================================
// 
//==============================================================================
BMessagePacket::BMessagePacket(long message, long packetType, int32 playerID) :
   BChannelPacket(packetType),
   mMessage(message),
   mPlayerID(playerID)
{
}

//==============================================================================
// 
//==============================================================================
void BMessagePacket::serialize(BSerialBuffer& sb)
{
   BChannelPacket::serialize(sb);
   sb.add(mMessage);
   sb.add(mPlayerID);
}

//==============================================================================
// 
//==============================================================================
void BMessagePacket::deserialize(BSerialBuffer& sb)
{ 
   BChannelPacket::deserialize(sb);
   sb.get(&mMessage);
   sb.get(&mPlayerID);
}

//==============================================================================
// 
//==============================================================================
BTickerRequestPacket::BTickerRequestPacket(const uint8 serviceId) :
BLSPPacket(serviceId, BPacketType::cTickerRequestPacket)
, mLanguage(static_cast<uint8>(XGetLanguage()))
{
}

//==============================================================================
// 
//==============================================================================
BTickerRequestPacket::~BTickerRequestPacket()
{
}

//==============================================================================
// 
//==============================================================================
void BTickerRequestPacket::serialize(BSerialBuffer& sb)
{
   BLSPPacket::serialize(sb);

   sb.add(static_cast<int8>(mUsers.getSize()));
   for (uint i=0; i < mUsers.getSize(); ++i)
   {
      sb.add(mUsers[i]);
   }
   sb.add(static_cast<uint8>(XGetLanguage()));
}

void BTickerRequestPacket::addUser(const XUID id)
{
   mUsers.add(id);
}

BTickerResponsePacket::BTickerResponsePacket()
{
}

BTickerResponsePacket::~BTickerResponsePacket()
{
}

void BTickerResponsePacket::deserialize(BSerialBuffer & sb)
{
   BLSPPacket::deserialize(sb);
   sb.get(&mTickerText);
   sb.get(&mPriority);
   sb.get(&mLifetime);
}

const BString & BTickerResponsePacket::getTickerText() const
{
   return mTickerText;
}

const uint8 BTickerResponsePacket::getPriority() const
{
   return mPriority;
}

const int BTickerResponsePacket::getLifetime() const
{
   return mLifetime;
}
