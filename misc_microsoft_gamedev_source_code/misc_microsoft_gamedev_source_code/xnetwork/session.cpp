//==============================================================================
// session.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "session.h"
#include "client.h"
#include "TimeSync.h"
#include "config.h"
#include "econfigenum.h"

//==============================================================================
// Modified from Bungie's secure transport code
//==============================================================================
uint64 GetUniqueID(const XNADDR& xnaddr)
{
   uint64 id;

   id = *(uint64*)&xnaddr.abEnet;
   id = id >> ((sizeof(uint64) - sizeof(xnaddr.abEnet)) * 8);

   return id;
}

//==============================================================================
// Modified from Bungie's secure transport code
//==============================================================================
bool CompareXnAddr(const XNADDR& xnaddr1, const XNADDR& xnaddr2)
{
   bool equal = false;

   uint64 machineId1;
   uint64 machineId2;

   if (xnaddr1.inaOnline.s_addr != 0 &&
       xnaddr2.inaOnline.s_addr != 0 &&
       XNetXnAddrToMachineId(&xnaddr1, &machineId1) == 0 &&
       XNetXnAddrToMachineId(&xnaddr2, &machineId2) == 0)
   {
      equal = (machineId1 == machineId2);
   }
   else
   {
      machineId1 = GetUniqueID(xnaddr1);
      machineId2 = GetUniqueID(xnaddr2);
      if (machineId1 == machineId2)
      {
         equal = (xnaddr1.ina.s_addr == 0 || xnaddr2.ina.s_addr == 0 || xnaddr1.ina.s_addr == xnaddr2.ina.s_addr);
      }
   }

   return equal;
}

//==============================================================================
//
//==============================================================================
class BSessionInitPayload : public BEventPayload
{
   public:
      BSessionInitPayload() :
         mProxied(false)
      {
      }
      ~BSessionInitPayload() {}

      void init(const IN_ADDR& addr, uint16 port, const XNADDR& xnAddr)
      {
         mAddr.sin_family = AF_INET;
         mAddr.sin_port = port;
         mAddr.sin_addr = addr;
         mXnAddr = xnAddr;
         IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));
         mProxied = false;
      }

      void init(const SOCKADDR_IN& addr, const XNADDR& xnAddr)
      {
         mAddr = addr;
         mXnAddr = xnAddr;
         IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));
         mProxied = false;
      }

      void init(const IN_ADDR& addr, uint16 port, const XNADDR& xnAddr, const SOCKADDR_IN& proxyAddr)
      {
         mAddr.sin_family = AF_INET;
         mAddr.sin_port = port;
         mAddr.sin_addr = addr;
         mXnAddr = xnAddr;
         mProxyAddr = proxyAddr;
         mProxied = true;
      }

      void init(const SOCKADDR_IN& addr, const XNADDR& xnAddr, const SOCKADDR_IN& proxyAddr)
      {
         mAddr = addr;
         mXnAddr = xnAddr;
         mProxyAddr = proxyAddr;
         mProxied = true;
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      XNADDR      mXnAddr;
      SOCKADDR_IN mAddr;
      SOCKADDR_IN mProxyAddr;
      bool        mProxied;
};


IMPLEMENT_FREELIST(BChannelData, 4, &gNetworkHeap);

//==============================================================================
// 
//==============================================================================
BChannelData::BChannelData() :
   mMachineID(BMachine::cInvalidMachineID),
   mChannelID(0),
   mPacketID(0),
   mSize(0),
   mpEvent(NULL)
{
}

//==============================================================================
// 
//==============================================================================
BChannelData::~BChannelData()
{
}

//==============================================================================
// 
//==============================================================================
void BChannelData::init(uint32 machineID, long channelID, uint packetID, uint size, BSessionEvent* pEvent)
{
   mMachineID = static_cast<BMachineID>(machineID);
   mChannelID = channelID;
   mPacketID = packetID;
   mSize = size;
   mpEvent = pEvent;
}

//==============================================================================
// 
//==============================================================================
void BChannelData::onAcquire()
{
}

//==============================================================================
// 
//==============================================================================
void BChannelData::onRelease()
{
   // if we have an event and it was processed, then we're safe to remove it now
   if (mpEvent != NULL && mpEvent->isProcessed())
      HEAP_DELETE(mpEvent, gNetworkHeap);
   else if (mpEvent != NULL)
      mpEvent->mAutoRelease = true;
   mpEvent = NULL;
}

//==============================================================================
//
//==============================================================================
const void* BChannelData::getData() const
{
   if (mSize == 0 || mpEvent == NULL)
      return NULL;

   return ((char*)mpEvent) + sizeof(BSessionEvent);
}

//==============================================================================
//
//==============================================================================
bool BChannelData::isProcessed() const
{
   if (mpEvent == NULL)
      return true;

   return mpEvent->isProcessed();
}

//==============================================================================
//
//==============================================================================
BDisconnectEntry::BDisconnectEntry() :
   mState(cStateNone),
   mMachineID(BMachine::cInvalidMachineID)
{
   IGNORE_RETURN(Utils::FastMemSet(&mStates, 0, sizeof(BDisconnectState)*XNetwork::cMaxClients));
}

//==============================================================================
//
//==============================================================================
void BDisconnectEntry::init(BMachineID machineID)
{
   mMachineID = machineID;
   mState = cStateNone;
   IGNORE_RETURN(Utils::FastMemSet(&mStates, 0, sizeof(BDisconnectState)*XNetwork::cMaxClients));
}

//==============================================================================
//
//==============================================================================
void BDisconnectEntry::set(BMachineID machineID, BDisconnectState state)
{
   //BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;

   mStates[machineID] = state;
}

//==============================================================================
//
//==============================================================================
BDisconnectEntry::BDisconnectState BDisconnectEntry::getState(BMachineID machineID) const
{
   BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return cStateNone;

   return mStates[machineID];
}

//==============================================================================
//
//==============================================================================
BDisconnectTracker::BDisconnectTracker()
{
}

//==============================================================================
//
//==============================================================================
bool BDisconnectTracker::add(BMachineID machineID)
{
   BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return false;

   BDisconnectEntry& entry = mMachines[machineID];
   BASSERT(entry.getState() == BDisconnectEntry::cStateNone);

   entry.init(machineID);

   return true;
}

//==============================================================================
//
//==============================================================================
BDisconnectEntry* BDisconnectTracker::get(BMachineID machineID)
{
   BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return NULL;

   return &mMachines[machineID];
}

//==============================================================================
//
//==============================================================================
void BDisconnectTracker::set(BMachineID machineID, BDisconnectEntry::BDisconnectState state)
{
   BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      BDisconnectEntry& entry = mMachines[i];
      entry.set(machineID, state);
   }
}

//==============================================================================
//
//==============================================================================
void BDisconnectTracker::reset(BMachineID machineID)
{
   BASSERT(machineID >= 0 && machineID < XNetwork::cMaxClients);
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;

   BDisconnectEntry* pEntry = get(machineID);
   if (pEntry != NULL)
      pEntry->setState(BDisconnectEntry::cStateNone);

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      BDisconnectEntry& entry = mMachines[i];
      entry.set(machineID, BDisconnectEntry::cStateNone);
   }
}

//==============================================================================
//
//==============================================================================
BChannelInfo::BChannelInfo() :
   mChannelID(0),
   mPacketID(0),
   mIsValid(false)
{
}

//==============================================================================
//
//==============================================================================
void BChannelInfo::init(uint channelID)
{
   mChannelID = channelID;
   mIsValid = true;
}

//==============================================================================
//
//==============================================================================
void BChannelInfo::reset()
{
   mChannelID = 0;
   mPacketID = 0;
   mIsValid = false;
}

//==============================================================================
//
//==============================================================================
void BChannelInfo::serialize(BSerialBuffer& sb)
{
   uint8 channelID = static_cast<uint8>(mChannelID);
   sb.add(channelID);

   uint16 packetID = static_cast<uint16>(mPacketID);
   sb.add(packetID);
}

//==============================================================================
//
//==============================================================================
void BChannelInfo::deserialize(BSerialBuffer& sb)
{
   uint8 channelID = 0;
   sb.get(&channelID);
   mChannelID = static_cast<uint>(channelID);

   uint16 packetID;
   sb.get(&packetID);
   mPacketID = static_cast<uint>(packetID);
}

//==============================================================================
//
//==============================================================================
BChannelTracker::BChannelTracker() :
   mMachineID(BMachine::cInvalidMachineID)
{
}

//==============================================================================
//
//==============================================================================
void BChannelTracker::init(BMachineID machineID)
{
   for (uint i=0; i < cMaxChannels; ++i)
      mChannels[i].reset();

   mMachineID = machineID;
}

//==============================================================================
//
//==============================================================================
void BChannelTracker::set(uint channelID, uint packetID)
{
   BASSERT(channelID < cMaxChannels);
   if (channelID >= cMaxChannels)
      return;
   if (!mChannels[channelID].isValid())
      mChannels[channelID].init(channelID);
   mChannels[channelID].set(packetID);
}

//==============================================================================
// 
//==============================================================================
const BChannelInfo* BChannelTracker::getChannelInfo(uint channelID) const
{
   BASSERT(channelID < cMaxChannels);
   if (channelID >= cMaxChannels)
      return NULL;

   return &mChannels[channelID];
}

//==============================================================================
// 
//==============================================================================
uint BChannelTracker::getPacketID(uint channelID) const
{
   BASSERT(channelID < cMaxChannels);
   if (channelID >= cMaxChannels)
      return 0;

   return mChannels[channelID].getPacketID();
}

//==============================================================================
// 
//==============================================================================
bool BChannelTracker::isValid(uint channelID) const
{
   BASSERT(channelID < cMaxChannels);
   if (channelID >= cMaxChannels)
      return 0;

   return mChannels[channelID].isValid();
}

//==============================================================================
// 
//==============================================================================
void BChannelTracker::serialize(BSerialBuffer& sb)
{
   sb.add(mMachineID);
   for (uint i=0; i < cMaxChannels; ++i)
   {
      mChannels[i].serialize(sb);
   }
}

//==============================================================================
// 
//==============================================================================
void BChannelTracker::deserialize(BSerialBuffer& sb)
{
   sb.get(&mMachineID);
   for (uint i=0; i < cMaxChannels; ++i)
   {
      mChannels[i].deserialize(sb);
   }
}

//==============================================================================
// 
//==============================================================================
BChannelTracker& BChannelTracker::operator=(const BChannelTracker& source)
{
   if (this == &source)
      return *this;

   mMachineID = source.mMachineID;

   for (uint i=0; i < cMaxChannels; ++i)
   {
      mChannels[i] = source.mChannels[i];
   }

   return *this;
}

//==============================================================================
//
//==============================================================================
BProxy::BProxy()
{
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyXnAddr, 0, sizeof(XNADDR)));
}

//==============================================================================
//
//==============================================================================
void BProxy::init(const XNADDR& xnaddr, const XNADDR& proxyXnAddr)
{
   mXnAddr = xnaddr;
   mProxyXnAddr = proxyXnAddr;
}

//==============================================================================
//
//==============================================================================
BProxy& BProxy::operator=(const BProxy& proxy)
{
   mXnAddr = proxy.mXnAddr;
   mProxyXnAddr = proxy.mProxyXnAddr;
   return *this;
}

//==============================================================================
//
//==============================================================================
void BProxy::serialize(BSerialBuffer& sb)
{
   sb.add(static_cast<void*>(&mXnAddr), sizeof(XNADDR));
   sb.add(static_cast<void*>(&mProxyXnAddr), sizeof(XNADDR));
}

//==============================================================================
//
//==============================================================================
void BProxy::deserialize(BSerialBuffer& sb)
{
   void* p = static_cast<void*>(&mXnAddr);
   sb.get(&p, sizeof(XNADDR));
   p = static_cast<void*>(&mProxyXnAddr);
   sb.get(&p, sizeof(XNADDR));
}

//==============================================================================
//
//==============================================================================
BProxyInfo::BProxyInfo() :
   mCount(0)
{
}

//==============================================================================
//
//==============================================================================
void BProxyInfo::add(const BProxy& proxy)
{
   BASSERT(mCount < XNetwork::cMaxClients);
   if (mCount >= XNetwork::cMaxClients)
      return;
   mProxy[mCount] = proxy;
   mCount++;
}

//==============================================================================
//
//==============================================================================
void BProxyInfo::add(const XNADDR& xnaddr, const XNADDR& proxyXnAddr)
{
   BASSERT(mCount < XNetwork::cMaxClients);
   if (mCount >= XNetwork::cMaxClients)
      return;
   mProxy[mCount].init(xnaddr, proxyXnAddr);
   mCount++;
}

//==============================================================================
//
//==============================================================================
const BProxy* BProxyInfo::getProxy(uint index) const
{
   BASSERT(index < XNetwork::cMaxClients && index < mCount);
   if (index >= XNetwork::cMaxClients || index >= mCount)
      return NULL;

   return &mProxy[index];
}

//==============================================================================
//
//==============================================================================
const BProxy* BProxyInfo::find(const XNADDR& xnaddr) const
{
   for (uint i=0; i < mCount && i < XNetwork::cMaxClients; ++i)
   {
      if (CompareXnAddr(mProxy[i].getXnAddr(), xnaddr))
         return &mProxy[i];
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
BProxyInfo& BProxyInfo::operator=(const BProxyInfo& info)
{
   mCount = info.mCount;
   for (uint i=0; i < mCount; ++i)
      mProxy[i] = info.mProxy[i];
   return *this;
}

//==============================================================================
//
//==============================================================================
void BProxyInfo::serialize(BSerialBuffer& sb)
{
   sb.add(mCount);
   for (uint i=0; i < mCount; ++i)
      mProxy[i].serialize(sb);
}

//==============================================================================
//
//==============================================================================
void BProxyInfo::deserialize(BSerialBuffer& sb)
{
   sb.get(&mCount);
   for (uint i=0; i < mCount && i < XNetwork::cMaxClients; ++i)
      mProxy[i].deserialize(sb);
}

//==============================================================================
//
//==============================================================================
BSessionUser& BSessionUser::operator=(const BSessionUser& user)
{
   mXuid = user.mXuid;
   mGamertag = user.mGamertag;
   mClientID = user.mClientID;
   mControllerID = user.mControllerID;
   return *this;
}

//==============================================================================
//
//==============================================================================
void BSessionUser::serialize(BSerialBuffer& sb)
{
   sb.add(mXuid);
   sb.add(mGamertag);
   int8 index = static_cast<int8>(mClientID);
   sb.add(index);
}

//==============================================================================
//
//==============================================================================
void BSessionUser::deserialize(BSerialBuffer& sb)
{
   sb.get(&mXuid);
   sb.get(&mGamertag);
   int8 index;
   sb.get(&index);
   mClientID = index;
}

//==============================================================================
//
//==============================================================================
BProxyRequest::BProxyRequest() :
   mRequestTime(0),
   mResponseTime(0),
   mRequestTimeout(0),
   mState(cStateInit)
{
   IGNORE_RETURN(Utils::FastMemSet(&mToXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mFromAddress, 0, sizeof(SOCKADDR_IN)));
   IGNORE_RETURN(Utils::FastMemSet(&mToAddress, 0, sizeof(SOCKADDR_IN)));

   XNetRandom((BYTE*)&mNonce, sizeof(mNonce));
}

//==============================================================================
//
//==============================================================================
void BProxyRequest::init(const SOCKADDR_IN& fromAddr, const SOCKADDR_IN& toAddr, const XNADDR& toXnAddr, uint timeout)
{
   mFromAddress = fromAddr;
   mToAddress = toAddr;
   mToXnAddr = toXnAddr;
   mRequestTimeout = timeGetTime() + timeout;
}

//==============================================================================
//
//==============================================================================
void BProxyRequest::setState(BRequestState state, uint32 time)
{
   if (mState == state)
      return;

   if (state == cStatePingSent)
      mRequestTime = time;
   else if (state == cStateComplete)
      mResponseTime = time;

   mState = state;
}

//==============================================================================
//
//==============================================================================
BMachine::BMachine() :
   mID(cInvalidMachineID),
   mState(BMachine::cStateOpen),
   mPingAverage(0),
   mPingStdDev(0),
   mProxyPing(0),
   mLocal(false),
   mEnabled(false),
   mProxied(false)
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();

   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mAddress, 0, sizeof(SOCKADDR_IN)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddress, 0, sizeof(SOCKADDR_IN)));
}

//==============================================================================
//
//==============================================================================
void BMachine::init(int id, const XNADDR& xnaddr, const IN_ADDR& addr, uint16 port, BMachineState state)
{
   if (mID != -1)
      return;

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();

   mID = id;
   mXnAddr = xnaddr;

   mAddress.sin_family = AF_INET;
   mAddress.sin_addr = addr;
   mAddress.sin_port = port;

   mState = state;

   mEnabled = true;

   mLocal = (addr.s_addr == INADDR_LOOPBACK);
}

//==============================================================================
//
//==============================================================================
void BMachine::reset()
{
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();

   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mAddress, 0, sizeof(SOCKADDR_IN)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddress, 0, sizeof(SOCKADDR_IN)));
   mID = cInvalidMachineID;
   mState = BMachine::cStateOpen;
   mPingAverage = 0;
   mPingStdDev = 0;
   mProxyPing = UINT_MAX;
   mLocal = false;
   mEnabled = false;
   mProxied = false;
}

//==============================================================================
//
//==============================================================================
uint BMachine::getClientCount()
{
   uint count = 0;
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      if (mUsers[i].mXuid != 0)
         count++;
   return(count);
}

//==============================================================================
//
//==============================================================================
void BMachine::updatePing(uint32 avg, uint32 stdDev)
{
   mPingAverage = avg;
   mPingStdDev = stdDev;
}

//==============================================================================
//
//==============================================================================
void BMachine::setState(BMachineState state)
{
   mState = state;
}

//==============================================================================
//
//==============================================================================
void BMachine::enable()
{
   mEnabled = true;
}

//==============================================================================
//
//==============================================================================
void BMachine::disable()
{
   mEnabled = false;

   BNetIPString strAddr(mAddress);
   BNetIPString strProxyAddr(mProxyAddress);

   nlog(cSessionNL, "BMachine::disable -- ip[%s] proxy[%s]",
      strAddr.getPtr(), strProxyAddr.getPtr());

   // also disable the proxy
   mProxied = false;
   mProxyPing = UINT_MAX;
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddress, 0, sizeof(SOCKADDR_IN)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyXnAddr, 0, sizeof(XNADDR)));
}

//==============================================================================
//
//==============================================================================
void BMachine::setProxy(const SOCKADDR_IN& proxyAddr, uint ping, const XNADDR& proxyXnAddr)
{
   if (mProxied && ping > mProxyPing)
      return;

   BNetIPString strAddr(mAddress);
   BNetIPString strProxyAddr(proxyAddr);

   nlog(cSessionNL, "BMachine::setProxy -- ip[%s] proxy[%s] uniqueID[%I64u]",
      strAddr.getPtr(), strProxyAddr.getPtr(), BSession::getUniqueID(mXnAddr));

   mProxied = true;
   mProxyPing = ping;
   mProxyAddress = proxyAddr;
   mProxyXnAddr = proxyXnAddr;
}

//==============================================================================
//
//==============================================================================
BXConnector::BXConnector() :
   mStatus(XNET_CONNECT_STATUS_IDLE),
   mState(cStateIdle),
   mMachineID(-1),
   mProxyPing(UINT_MAX),
   mPort(0),
   mProxied(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mXnkid, 0, sizeof(XNKID)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));
}

//==============================================================================
//
//==============================================================================
BXConnector::BXConnector(const BXConnector& source) :
   mStatus(XNET_CONNECT_STATUS_IDLE),
   mState(cStateIdle),
   mMachineID(-1),
   mProxyPing(UINT_MAX),
   mPort(0),
   mProxied(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mXnkid, 0, sizeof(XNKID)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));

   *this = source;
}

//==============================================================================
//
//==============================================================================
void BXConnector::init(const XNADDR& xnaddr, uint16 port, const XNKID& xnkid)
{
   mXnAddr = xnaddr;
   mPort = port;
   mXnkid = xnkid;
   mMachineID = 0;
}

//==============================================================================
//
//==============================================================================
void BXConnector::init(BMachineID machineID, const XNADDR& xnaddr, uint16 port, const XNKID& xnkid, BSessionUser users[])
{
   mMachineID = machineID;
   mXnAddr = xnaddr;
   mPort = port;
   mXnkid = xnkid;
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i] = users[i];
}

//==============================================================================
//
//==============================================================================
void BXConnector::reset()
{
   mMachineID = -1;
   mStatus = XNET_CONNECT_STATUS_IDLE;
   mState = cStateIdle;
   mPort = 0;
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mUsers[i].reset();
}

//==============================================================================
//
//==============================================================================
void BXConnector::connect()
{
   BDEBUG_ASSERT(mState == cStateIdle);

   int retval = XNetXnAddrToInAddr(&mXnAddr, &mXnkid, &mAddr);
   if (retval != 0)
   {
      mState = cStateFailed;
      return;
   }

   retval = XNetConnect(mAddr);
   if (retval != 0)
   {
      mState = cStateFailed;
      return;
   }

   mState = cStateConnecting;
   mStatus = XNetGetConnectStatus(mAddr);
}

//==============================================================================
//
//==============================================================================
void BXConnector::service()
{
   if (mState == cStateConnecting)
   {
      mStatus = XNetGetConnectStatus(mAddr);
      if (mStatus == XNET_CONNECT_STATUS_CONNECTED)
         mState = cStateConnected;
      else if (mStatus != XNET_CONNECT_STATUS_PENDING)
         mState = cStateFailed;
   }
}

//==============================================================================
//
//==============================================================================
DWORD BXConnector::getStatus(bool recheck)
{
   if (!recheck)
      return mStatus;

   mStatus = XNetGetConnectStatus(mAddr);
   if (mStatus == XNET_CONNECT_STATUS_CONNECTED)
      mState = cStateConnected;
   else if (mStatus != XNET_CONNECT_STATUS_PENDING)
      mState = cStateFailed;

   return mStatus;
}

//==============================================================================
//
//==============================================================================
void BXConnector::terminate()
{
   if (mState == cStateConnecting || mState == cStateConnected)
   {
      XNetUnregisterInAddr(mAddr);
      mState = cStateIdle;
      mStatus = XNET_CONNECT_STATUS_LOST;
   }
}

//==============================================================================
//
//==============================================================================
void BXConnector::setProxy(const SOCKADDR_IN& proxyAddr, uint ping, const XNADDR& proxyXnAddr)
{
   if (mProxied && ping > mProxyPing)
      return;

   BNetIPString strAddr(mAddr);
   BNetIPString strProxyAddr(proxyAddr);

   nlog(cSessionNL, "BXConnector::setProxy -- ip[%s] proxy[%s] uniqueID[%I64u]",
      strAddr.getPtr(), strProxyAddr.getPtr(), BSession::getUniqueID(mXnAddr));

   mProxied = true;
   mProxyAddr = proxyAddr;
   mProxyXnAddr = proxyXnAddr;
}

//==============================================================================
//
//==============================================================================
void BXConnector::clearProxy()
{
   mProxied = false;
   mProxyPing = 0;

   IGNORE_RETURN(Utils::FastMemSet(&mProxyXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mProxyAddr, 0, sizeof(SOCKADDR_IN)));
}

//==============================================================================
//
//==============================================================================
BSession::BSession() :
   mEventFreeList(&gNetworkHeap),
   mEventList(0, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mChannelDataList(XNetwork::cChannelQueueSize, BPLIST_GROW_EXPONENTIAL, &gNetworkHeap),
   mHostMachineID(-1), 
   mLocalMachineID(-1), 
   mMaxClientAmount(XNetwork::cMaxClients),
   mChecksum(0), 
   mFlags(0), 
   mpClientConnector(NULL),
   mpVoiceAllocator(NULL),
   mpVoiceInterface(NULL),
   mpTimeSync(NULL),
   mLocalXUID(0),
   mConnectionEventHandle(cInvalidEventReceiverHandle),
   mVoiceEventHandle(cInvalidEventReceiverHandle),
   mJoinRequestTimer(0),
   mpHostConnector(NULL),
   mConnectionTimeout(XNetwork::cConnectionTimeout),
   mProxyRequestTimeout(XNetwork::cDefaultProxyRequestTimeout),
   mProxyResponseTimeout(0),
   mProxyPingTimeout(XNetwork::cDefaultProxyPingTimeout),
   mJoinRequestTimeout(XNetwork::cDefaultJoinRequestTimeout),
   mUpdateInterval(XNetwork::c3v3UpdateInterval),
   mHostPort(0),
   mRequirePeersResponse(false),
   mEventProcessing(false),
   mProxySupport(false),
   mPeerTimerSet(false),
   mConnectionTimerSet(false),
   mProxyRequestTimerSet(false),
   mProxyResponseTimerSet(false),
   mDoNotProxyToHost(false)
{
   IGNORE_RETURN(Utils::FastMemSet(&mHostXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mSessionXNKID, 0, sizeof(XNKID)));
   IGNORE_RETURN(Utils::FastMemSet(&mLocalXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mHostSecureINADDR, 0, sizeof(IN_ADDR)));
} // BSession::BSession

//==============================================================================
//
//==============================================================================
void BSession::init(DWORD checksum, BClientConnector* pClientConnector, BTimingInfo* pTimingInfo, BVoiceInterface* pVoiceInterface, bool doNotProxyToHost)
{
   nlog(cSessionNL, "BSession::BSession, mChecksum 0x%08X", checksum);

   mDoNotProxyToHost = doNotProxyToHost;

   mProxySupport = gConfig.isDefined(cConfigProxySupport);

   long proxyRequestTimeout = 0;
   if (gConfig.get(cConfigProxyRequestTimeout, &proxyRequestTimeout))
      mProxyRequestTimeout = proxyRequestTimeout;

   long proxyPingTimeout = 0;
   if (gConfig.get(cConfigProxyPingTimeout, &proxyPingTimeout))
      mProxyPingTimeout = proxyPingTimeout;

   long joinRequestTimeout = 0;
   if (gConfig.get(cConfigJoinRequestTimeout, &joinRequestTimeout))
      mJoinRequestTimeout = joinRequestTimeout;

   mChecksum = checksum;

   mpClientConnector = pClientConnector;

   if (pTimingInfo != NULL)
   {
      mpTimeSync = HEAP_NEW(BTimeSync, gNetworkHeap);
      mpTimeSync->init(this, pTimingInfo);
   }

   if (!gConfig.get(cConfigSocketUnresponsiveTimeout, reinterpret_cast<long*>(&mConnectionTimeout)))
      mConnectionTimeout = XNetwork::cConnectionTimeout;

#ifdef BUILD_DEBUG
   // debug builds are really slow when loading a level
   // allow us extra time in the even that our connection thread is blocked for awhile
   //mConnectionTimeout += 20000;
#endif

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
      mMachines[i].reset();

   mSessionBufferAllocator.clear();
   mSendBufferAllocator.clear();
   mRecvBufferAllocator.clear();

   eventReceiverInit(cThreadIndexSim);

   mConnectionEventHandle = gEventDispatcher.addClient(this, cThreadIndexSimHelper);

   if (pVoiceInterface != NULL)
   {
      pVoiceInterface->addRef();

      mpVoiceInterface = pVoiceInterface;
      mpVoiceAllocator = pVoiceInterface->getAllocator();
      mVoiceEventHandle = pVoiceInterface->getEventHandle();
   }

   BDEBUG_ASSERT(mConnectionTimer.getHandle() != NULL);

   mConnectionTimerSet = true;
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mConnectionEventHandle;
   handleEvent.mToHandle = mConnectionEventHandle;
   handleEvent.mEventClass = cSessionEventTimer;
   gEventDispatcher.registerHandleWithEvent(mConnectionTimer.getHandle(), handleEvent);

   nlog(cTransportNL, "BSession::init -- setting connection timer event handle[%I64u] timer handle[0x%08X]", mConnectionEventHandle, mConnectionTimer.getHandle());

   // update/flush our connections at 10Hz
   mConnectionTimer.set(mUpdateInterval);
}

//==============================================================================
// Can return false if we have two local users already registered
//==============================================================================
bool BSession::addUser(uint controllerID, XUID xuid, const BSimString& gamertag)
{
   if (mHostPort != 0)
   {
      return joinUser(controllerID, xuid, gamertag);
   }

   BASSERTM(mLocalMachineID == -1, "The local machineID should not be assigned yet");
   if (mLocalMachineID != -1)
      return false;

   // find an open slot for this user
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (mLocalUsers[i].mXuid == xuid)
         return true;
      else if (mLocalUsers[i].mXuid == 0)
      {
         mLocalUsers[i].mControllerID = controllerID;
         mLocalUsers[i].mXuid = xuid;
         mLocalUsers[i].mGamertag = gamertag;
         mLocalUsers[i].mClientID = -1;
         return true;
      }
   }

   return false;
}

//==============================================================================
//
//==============================================================================
bool BSession::joinUser(uint controllerID, XUID xuid, const BSimString& gamertag)
{
   if (mHostPort == 0)
   {
      return addUser(controllerID, xuid, gamertag);
   }

   BASSERTM(mLocalMachineID != -1, "The local machineID should be assigned before attempting to join a new user");
   if (mLocalMachineID == -1)
      return false;

   BASSERTM(mHostMachineID != -1, "The host machineID should be assigned before attempting to join a new user");
   if (mHostMachineID == -1)
      return false;

   // if we're the host, let's see if we have enough room in the session
   if (mHostMachineID >= 0 && mHostMachineID < XNetwork::cMaxClients && mHostMachineID == mLocalMachineID)
   {
      if (mMaxClientAmount - getConnectedClientAmount() < 1)
         return false;

      return createUser(controllerID, xuid, gamertag);
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (mLocalUsers[i].mXuid == INVALID_XUID)
      {
         mLocalUsers[i].mControllerID = controllerID;
         mLocalUsers[i].mXuid = xuid;
         mLocalUsers[i].mGamertag = gamertag;
         mLocalUsers[i].mClientID = -1;
         break;
      }
   }

   BASSERTM(i < BMachine::cMaxUsers, "Too many outstanding client join requests");
   if (i >= BMachine::cMaxUsers)
      return false;

   // I need to request to join the session from the host
   // add this user to our temp local users list and issue
   // a client join request
   BMachine& host = mMachines[mHostMachineID];

   BNetIPString strAddr(host.getSockAddr());
   nlog(cSessionNL, "BSession::joinUser : sending join request to ip[%s].", strAddr.getPtr());

   // send a packet to this newly pending client
   BSessionConnectorPacket packet(BPacketType::cClientJoinRequestPacket, mChecksum, mLocalXnAddr, mLocalUsers);

   HRESULT hr = _SendPacketTo(&host, packet);
   BASSERTM(hr == S_OK, "Failed to send client join request to host");
   return SUCCEEDED(hr);

   return false;
}

//==============================================================================
//
//==============================================================================
HRESULT BSession::dispose()
{
   if (mEventProcessing)
      BFAIL("BSession::dispose -- called during event processing.");

   stopTimer();

   if (mpTimeSync)
   {
      mpTimeSync->dispose();
      HEAP_DELETE(mpTimeSync, gNetworkHeap);
      mpTimeSync = NULL;
   }

   nlog(cSessionNL, "BSession::dispose");
   if (isConnected())
      disconnect(cNormal);

   mUDPThread.deinit();

   for (int i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mpClient != NULL)
         HEAP_DELETE(mClients[i].mpClient, gNetworkHeap);
      mClients[i].mpClient = NULL;
   }

   for (long i=0;i<mIncomingData.getNumber();i++)
      HEAP_DELETE(mIncomingData[i], gNetworkHeap);
   mIncomingData.resize(0);

   disposeConnectors();

   // shutdown the BXUDPSocket, need a blocking send for this since the memory will be deleted
   if (mConnectionEventHandle != cInvalidEventReceiverHandle)
   {
      // also shutdown our voice session
      if (mVoiceEventHandle != cInvalidEventReceiverHandle)
         gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceDeinitSession);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventDeinit, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
   }

   BHandle hItem;
   BSessionEvent* pEvent = mEventList.getHead(hItem);
   while (pEvent)
   {
      releaseEvent(pEvent);

      pEvent = mEventList.getNext(hItem);
   }
   mEventList.reset();

   BChannelData* pChannelData = mChannelDataList.getHead(hItem);
   while (pChannelData)
   {
      BChannelData::releaseInstance(pChannelData);

      pChannelData = mChannelDataList.getNext(hItem);
   }
   mChannelDataList.reset();

   if (mConnectionEventHandle != cInvalidEventReceiverHandle)
      gEventDispatcher.removeClientDeferred(mConnectionEventHandle, true);

   mConnectionEventHandle = cInvalidEventReceiverHandle;

   eventReceiverDeinit();

   if (mpVoiceInterface)
      mpVoiceInterface->release();

   return S_OK;
}

//==============================================================================
//
//==============================================================================
void BSession::disposeConnectors()
{
   uint count = mXConnectors.getSize();
   for (uint i=0; i < count; ++i)
   {
      mXConnectors[i].terminate();
   }
}

//==============================================================================
//
//==============================================================================
BSession::~BSession()
{
} // BSession::~BSession

//==============================================================================
//
//==============================================================================
HRESULT BSession::host(const XNKID& sessionkey, WORD gamePortNumber)
{
   if (isConnected()) 
      return HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING); // Call disconnect or terminate first!

   nlog(cSessionNL, "BSession::host -- Enter");

   mHostPort = gamePortNumber;
   mSessionXNKID = sessionkey;

   mHostMachineID = mLocalMachineID = 0;

   // initialize the UDP thread
   // I may start receiving messages after this (on this thread) so be prepared
   if (!mUDPThread.init(mHostPort, &mSendBufferAllocator, &mRecvBufferAllocator, mConnectionEventHandle, true))
      return E_FAIL;

   nlog(cSessionNL, "BSession::host -- connecting local loopback client");

   if (!createHost())
      return E_FAIL;

   return S_OK;
}

//==============================================================================
// need a join method that can initiate an XNetConnect to the host
// and when that join completes I can send a join request packet
//==============================================================================
bool BSession::join(const XNADDR& xnaddr, uint16 port, const XNKID& xnkid)
{
   BDEBUG_ASSERT(!isHosted());
   if (isHosted())
      return false;

   BDEBUG_ASSERT(!isConnected());
   if (isConnected())
      return false;

   mHostPort = port;

   mHostXnAddr = xnaddr;
   mSessionXNKID = xnkid;

   //DEBUG - safe to remove/comment out
   if (XNetXnKidIsOnlinePeer(&mSessionXNKID))
   {
      nlog(cSessionNL, "BSession::join -- KeyID is ONLINEPEER");
   }
   if (XNetXnKidIsOnlineServer(&mSessionXNKID))
   {
      nlog(cSessionNL, "BSession::join -- KeyID is ONLINESERVER");
   }
   if (XNetXnKidIsSystemLink(&mSessionXNKID))
   {
      nlog(cSessionNL, "BSession::join -- KeyID is SYSTEMLINK");
   }

   //Build a security translated IP for that target host's XNADDR
   int retval = XNetXnAddrToInAddr(&mHostXnAddr, &mSessionXNKID, &mHostSecureINADDR);
   if (retval != 0)
   {
      DWORD lastErr = GetLastError();
      nlog(cSessionNL, "BSession::join -- Binding security key to target host XNADDR failed - Last Error: %d [%d]", lastErr, retval);
      return false;
   }

   BNetIPString strAddr(mHostSecureINADDR);
   nlog(cSessionNL, "BSession::join - target secure address ip[%s]", strAddr.getPtr());

   // initialize the UDP thread
   // I may start receiving messages after this (on this thread) so be prepared
   if (!mUDPThread.init(mHostPort, &mSendBufferAllocator, &mRecvBufferAllocator, mConnectionEventHandle, true))
      return false;

   // start a connection to the host
   BDEBUG_ASSERT(mpHostConnector == NULL);
   mpHostConnector = HEAP_NEW(BXConnector, gNetworkHeap);
   mpHostConnector->init(xnaddr, mHostPort, mSessionXNKID);
   mpHostConnector->connect();

   // set our peer timer
   if (!mPeerTimerSet)
   {
      mPeerTimerSet = true;
      BEvent handleEvent;
      handleEvent.clear();
      handleEvent.mFromHandle = mEventHandle;
      handleEvent.mToHandle = mEventHandle;
      handleEvent.mEventClass = cSessionEventInitHost;
      gEventDispatcher.registerHandleWithEvent(mPeerTimer.getHandle(), handleEvent);

      mPeerTimer.set(500);
   }

   return true;
}

//==============================================================================
// After we've completed our XNetConnect with the host, I can attempt to join
//==============================================================================
void BSession::handleJoin()
{
   if (mpHostConnector->getState() != BXConnector::cStateConnected)
   {
      mJoinRequestTimer = 0;
      addEvent(cEventJoinFailed, cResultHostConnectFailure);
      return;
   }

   mJoinRequestTimer = timeGetTime() + mJoinRequestTimeout;

   SOCKADDR_IN addr;
   addr.sin_family = AF_INET;
   addr.sin_addr = mHostSecureINADDR;
   addr.sin_port = mHostPort;

   BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
   pPayload->init(addr, mHostXnAddr);

   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInitPending, 0, 0, pPayload);

   BNetIPString strAddr(mHostSecureINADDR);
   nlog(cSessionNL, "BSession::handleJoin : sending join request to ip[%s].", strAddr.getPtr());

   // send a packet to this newly pending client
   BSessionConnectorPacket packet(BPacketType::cJoinRequestPacket, mChecksum, mLocalXnAddr, mLocalUsers);

   SendPacketToAddr(addr, packet);
}

//==============================================================================
//
//==============================================================================
void BSession::disconnect(long reason)
{
   if (!isConnected())
      return;

   nlog(cSessionNL, "BSession::disconnect -- enter. Reason [%d]", reason);

   setConnected(false);
   setDisconnecting(true);

   // stop the flushing of our connections
   stopTimer();

   // disconnect everyone and clean up
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mpClient != NULL)
         mClients[i].mpClient->setConnected(false);

      mClients[i].mState = BClientHolder::cStateDeleteMe;
   }

   // create a disconnect event for the socket thread
   BXNetDisconnect* pDisconnectEvent = HEAP_NEW(BXNetDisconnect, gNetworkHeap);

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mMachines[i].mState == BMachine::cStateConnected && !mMachines[i].isLocal())
         pDisconnectEvent->addAddr(mMachines[i].getSockAddr());

      mMachines[i].mState = BMachine::cStateDeleteMe;
   }

   disposeConnectors();

   mUDPThread.disconnect(pDisconnectEvent);

   mUDPThread.deinit();

   addEvent(cEventDisconnected, reason);
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::_SendPacket(BPacket& packet, long* pSizeOut, uint flags, const char* pFile, long line)
{
#ifndef BUILD_FINAL
   nlogt(cTransportNL, "BSession::_SendPacket [%s(%d)]", pFile, line);
#endif

   HRESULT hr = S_OK;

   long size = packet.getMaxSerialSize();
   if ((size > cMaxSendSize) || (size <= 0))
   {
      nlogt(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
      return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
   }

   // at this point, I have to create a new buffer
   // first attempt to dequeue a buffer

   BDEBUG_ASSERT(packet.getMaxSerialSize() <= cMaxSendSize);

   BSessionBuffer* pBuf = allocBuffer();
   if (pBuf == NULL)
   {
#ifndef BUILD_FINAL
      nlogt(cTransportNL, "BSession::_SendPacket(BPacket) -- E_OUTOFMEMORY");
#endif
      return E_OUTOFMEMORY;
   }

   packet.serializeInto(pBuf->mBuf, &size);

   pBuf->mSize = size;
   pBuf->mFlags = flags;

   if (pSizeOut != NULL)
      *pSizeOut = size;

   gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cSessionEventBroadcast, (uint)pBuf, flags);

   return hr;
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::SendBroadcast2(BPacket& packet, uint flags)
{
#ifndef BUILD_FINAL
   nlogt(cTransportNL, "BSession::SendBroadcast2");
#endif

   HRESULT hr = S_OK;

   long size = packet.getMaxSerialSize();
   if ((size > cMaxSendSize) || (size <= 0))
   {
      nlogt(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
      return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
   }

   // at this point, I have to create a new buffer
   // first attempt to dequeue a buffer

   BDEBUG_ASSERT(packet.getMaxSerialSize() <= cMaxSendSize);

   BSessionBuffer* pBuf = allocBuffer();
   if (pBuf == NULL)
   {
#ifndef BUILD_FINAL
      nlogt(cTransportNL, "BSession::SendBroadcast2(BPacket) -- E_OUTOFMEMORY");
#endif
      return E_OUTOFMEMORY;
   }

   packet.serializeInto(pBuf->mBuf, &size);

   pBuf->mSize = size;
   pBuf->mFlags = flags;

   gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cSessionEventBroadcast2, (uint)pBuf, flags);

   return hr;
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::SendPacketToAddr(const SOCKADDR_IN& addr, BPacket& packet)
{
   if (addr.sin_addr.s_addr == 0)
      return S_OK;

#ifndef BUILD_FINAL
   BNetIPString strAddr(addr);
   nlog(cTransportNL, "BSession::SendPacketToAddr -- ip[%s]", strAddr.getPtr());
#endif

   HRESULT hr = S_OK;

   long size = packet.getMaxSerialSize();
   if ((size > cMaxSendSize) || (size <= 0))
   {
      nlog(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
      return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
   }

   BDEBUG_ASSERT(packet.getMaxSerialSize() <= cMaxSendSize);

   BSessionBuffer* pBuf = allocBuffer();
   if (pBuf == NULL)
   {
#ifndef BUILD_FINAL
      BNetIPString strAddr(addr);
      nlog(cTransportNL, "BSession::SendPacketToAddr(SOCKADDR_IN) -- E_OUTOFMEMORY ip[%s]", strAddr.getPtr());
#endif
      return E_OUTOFMEMORY;
   }

   packet.serializeInto(pBuf->mBuf, &size);

   pBuf->mAddr = addr;
   pBuf->mSize = size;
   pBuf->mFlags = 0;
   
   gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cSessionEventSend, (uint)pBuf);

   return hr;
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::_SendPacketTo(const SOCKADDR_IN& addr, BPacket& packet, long* pSizeOut, uint flags, const char* pFile, long line)
{
   if (addr.sin_addr.s_addr == 0)
      return S_OK;

#ifndef BUILD_FINAL
   BNetIPString strAddr(addr);
   nlog(cTransportNL, "BSession::_SendPacketTo -- ip[%s] [%s(%d)]", strAddr.getPtr(), pFile, line);
#endif

   //HRESULT hr = S_OK;

   long size = packet.getMaxSerialSize();
   if ((size > cMaxSendSize) || (size <= 0))
   {
      nlog(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
      return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
   }

   BDEBUG_ASSERT(packet.getMaxSerialSize() <= cMaxSendSize);

   BSessionBuffer* pBuf = allocBuffer();
   if (pBuf == NULL)
   {
#ifndef BUILD_FINAL
      nlog(cTransportNL, "BSession::_SendPacketTo(SOCKADDR_IN) -- E_OUTOFMEMORY ip[%s]", strAddr.getPtr());
#endif
      return E_OUTOFMEMORY;
   }

   packet.serializeInto(pBuf->mBuf, &size);

   pBuf->mAddr = addr;
   pBuf->mSize = size;
   pBuf->mFlags = flags;

   if (pSizeOut != NULL)
      *pSizeOut = size;

   bool retval = gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cSessionEventSend, (uint)pBuf, flags);
   BDEBUG_ASSERTM(retval, "Failed to issue cSessionEventSend");

   return (retval ? S_OK : E_FAIL);
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::_SendPacketTo(BClient* pTargetClient, BPacket& packet, long* pSizeOut, uint flags, const char* pFile, long line)
{
   BDEBUG_ASSERT(pTargetClient);

   return _SendPacketTo(pTargetClient->getAddr(), packet, pSizeOut, flags, pFile, line);
}

//==============================================================================
// 
//==============================================================================
HRESULT BSession::_SendPacketTo(BMachine* pTargetMachine, BPacket& packet, long* pSizeOut, uint flags, const char* pFile, long line)
{
   BDEBUG_ASSERT(pTargetMachine);

   return _SendPacketTo(pTargetMachine->getSockAddr(), packet, pSizeOut, flags, pFile, line);
}

//==============================================================================
//
//==============================================================================
void BSession::service()
{
   for (uint i=0; i < XNetwork::cMaxClients; i++)
   {
      if (mClients[i].mpClient)
      {
         if (mClients[i].mState == BClientHolder::cStateDeleteMe)
         {
            if (mClients[i].mpClient != NULL)
            {
               HEAP_DELETE(mClients[i].mpClient, gNetworkHeap);
               mClients[i].mpClient = NULL;
            }
         }
         else if (mClients[i].mState == BClientHolder::cStateDisconnected)
         {
            if (mClients[i].mpClient != NULL)
            {
               HEAP_DELETE(mClients[i].mpClient, gNetworkHeap);
               mClients[i].mpClient = NULL;
            }
         }
         else
         {
            if (mClients[i].mState == BClientHolder::cStateKicked)
            {
               beginClientDisconnect(i, cHostDecision);
            }
         }
      }
   }

   for (uint i=0; i < XNetwork::cMaxClients; i++)
   {
      if (mMachines[i].mState == BMachine::cStateDisconnected || mMachines[i].mState == BMachine::cStateDeleteMe)
      {
         for (uint j=0; j < BMachine::cMaxUsers; ++j)
         {
            BClientID clientID = mMachines[i].mUsers[j].mClientID;
            if (clientID != -1)
            {
               if (mClients[clientID].mpClient && mClients[clientID].mState < BClientHolder::cStateKicked)
               {
                  beginClientDisconnect(clientID, cHostDecision);
               }
               else if (mClients[clientID].mpClient)
               {
                  HEAP_DELETE(mClients[clientID].mpClient, gNetworkHeap);
                  mClients[clientID].mpClient = NULL;
               }

               mMachines[i].mUsers[j].reset();
            }
         }

         mMachines[i].reset();
      }
   }

   if (mpTimeSync)
      mpTimeSync->service();

   processEvents();

   if (mJoinRequestTimer > 0)
   {
      //We are waiting on a join request - see if it has timed out
      //todo move the timeout time to a constant/setable var
      if (mJoinRequestTimer < timeGetTime())
      {
         mJoinRequestTimer = 0;
         addEvent(cEventJoinFailed, cResultHostNotFound);
      }
   }
}

//==============================================================================
// 
//==============================================================================
BClient* BSession::getClient(BClientID clientID) const
{
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
      return NULL;
   return mClients[clientID].mpClient;
}

//==============================================================================
// 
//==============================================================================
BClient* BSession::getClientByXuid(XUID xuid) const
{
   for (uint i=0; i < XNetwork::cMaxClients; i++)
   {
      if ((mClients[i].mpClient) &&
          (mClients[i].mpClient->getXuid() == xuid))
      {
         return (mClients[i].mpClient);
      }
   }

   return NULL;
}

//==============================================================================
// 
//==============================================================================
bool BSession::isClient(long clientIndex)
{
   if ((clientIndex < 0) || (clientIndex >= XNetwork::cMaxClients))
      return false;
   else if (mClients[clientIndex].mpClient)
      return true;
   return false;
}

//==============================================================================
// 
//==============================================================================
bool BSession::getLocalClients(BClient* pLocalClient1, BClient* pLocalClient2) const
{
   pLocalClient1 = NULL;
   pLocalClient2 = NULL;

   BDEBUG_ASSERTM(mLocalMachineID != -1, "No Local Machine found");
   if (mLocalMachineID < 0 || mLocalMachineID >= XNetwork::cMaxClients)
      return false;

   const BMachine& machine = mMachines[mLocalMachineID];

   pLocalClient1 = getClient(machine.mUsers[0].mClientID);
   pLocalClient2 = getClient(machine.mUsers[1].mClientID);

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BSession::getHostClients(BClient* pLocalClient1, BClient* pLocalClient2) const
{
   pLocalClient1 = NULL;
   pLocalClient2 = NULL;

   BDEBUG_ASSERTM(mHostMachineID != -1, "No Host Machine found");
   if (mHostMachineID < 0 || mHostMachineID >= XNetwork::cMaxClients)
      return false;

   const BMachine& machine = mMachines[mHostMachineID];

   pLocalClient1 = getClient(machine.mUsers[0].mClientID);
   pLocalClient2 = getClient(machine.mUsers[1].mClientID);

   return true;
}

//==============================================================================
// 
//==============================================================================
BMachine* BSession::getLocalMachine()
{
   BDEBUG_ASSERTM(mLocalMachineID != -1, "No Local Machine found");
   if (mLocalMachineID < 0 || mLocalMachineID >= XNetwork::cMaxClients)
      return NULL;
   return &mMachines[mLocalMachineID];
}

//==============================================================================
// 
//==============================================================================
BMachine* BSession::getHostMachine()
{
   BDEBUG_ASSERTM(mHostMachineID != -1, "No Host Machine found");
   if (mHostMachineID < 0 || mHostMachineID >= XNetwork::cMaxClients)
      return NULL;
   return &mMachines[mHostMachineID];
}

//==============================================================================
// 
//==============================================================================
BMachine* BSession::getMachine(BMachineID machineID)
{
   BASSERTM(machineID >= 0 && machineID < XNetwork::cMaxClients, "Invalid machineID");
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return NULL;

   return &mMachines[machineID];
}

//==============================================================================
//
//==============================================================================
bool BSession::isHosted() const
{ 
   if (
      (mLocalMachineID != -1) && 
      (mHostMachineID != -1) &&
      (mLocalMachineID == mHostMachineID)
      ) 
      return true;
   else 
      return false;
}


//==============================================================================
// Depending on how often this is called, we may want to cache the two
//    local clientIDs
//==============================================================================
BOOL BSession::isLocalClientID(BClientID clientID)
{
   //TODO - Disabling this assert until after I'm done with the splitscreen network work, it trips on AI in MP - eric
   //BASSERTM(clientID >= 0 && clientID < XNetwork::cMaxClients, "Invalid clientID");
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
      return FALSE;

//-- FIXING PREFIX BUG ID 7558
   const BMachine* pMachine = getLocalMachine();
//--
   if (pMachine == NULL)
      return FALSE;

   return (pMachine->mUsers[0].mClientID == clientID || pMachine->mUsers[1].mClientID == clientID);
}

//==============================================================================
//
//==============================================================================
BOOL BSession::isHostClientID(BClientID clientID)
{
   //TODO - Disabling this assert until after I'm done with the splitscreen network work, it trips on AI in MP - eric
   //BASSERTM(clientID >= 0 && clientID < XNetwork::cMaxClients, "Invalid clientID");
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
      return FALSE;

//-- FIXING PREFIX BUG ID 7559
   const BMachine* pMachine = getHostMachine();
//--
   if (pMachine == NULL)
      return FALSE;

   return (pMachine->mUsers[0].mClientID == clientID || pMachine->mUsers[1].mClientID == clientID);
}

//==============================================================================
//
//==============================================================================
void BSession::dataReceived(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDEBUG_ASSERT(pData);
   if (pData == NULL)
      return;
   BDEBUG_ASSERT(size >= 1);
   if (size < 1)
      return;
   long type = BTypedPacket::getType(pData);
#ifndef BUILD_FINAL
   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::dataReceived type[%ld] ip[%s] incoming[%d] equeue[%d]", type, strAddr.getPtr(), mIncomingData.getSize(), mEventList.getSize());
#endif
   switch (type)
   {
      case BPacketType::cChannelPacket:
         {
            BMachineID machineID = mapAddrToMachineID(addr);
            if (isConnected() && (machineID != -1))
            {  
               addEvent(cEventChannelData, machineID, size, pData);
            }
            else
            {
               BIncomingData* pIncomingData = HEAP_NEW(BIncomingData, gNetworkHeap);
               pIncomingData->init(cEventChannelData, addr, pData, size);
               mIncomingData.add(pIncomingData);
            }      
            break;
         }

      case BPacketType::cKickPacket:
         {
            // need a way to map a SOCKADDR_IN to a client
            kickClient(mapAddrToClientIndex(addr), pData, size);
            break;
         }

      case BPacketType::cConnectionRejectedPacket:
         {
            connectionRejected(pData, size);
            break;
         }

      case BPacketType::cInitPeerPacket:
         {
            handleInitPeer(pData, size, addr);
            break;
         }

      case BPacketType::cJoinRequestPacket:
         {
            handleJoinRequest(pData, size, addr);
            break;
         }

      case BPacketType::cJoinResponsePacket:
         {
            handleJoinResponse(pData, size, addr);
            break;
         }

      case BPacketType::cInitClientPacket:
         {
            handleInitClient(pData, size, addr);
            break;
         }

      case BPacketType::cClientJoinRequestPacket:
         {
            handleClientJoinRequest(pData, size, addr);
            break;
         }

      case BPacketType::cClientJoinResponsePacket:
         {
            handleClientJoinResponse(pData, size, addr);
            break;
         }

      case BPacketType::cInitPeersRequestPacket:
         {
            handlePeersRequest(pData, size, addr);
            break;
         }

      case BPacketType::cInitPeersResponsePacket:
         {
            handlePeersResponse(pData, size, addr);
            break;
         }

      case BPacketType::cProxyRequestPacket:
         {
            handleProxyRequest(pData, size, addr);
            break;
         }

      case BPacketType::cProxyResponsePacket:
         {
            handleProxyResponse(pData, size, addr);
            break;
         }

      case BPacketType::cProxyPingPacket:
         {
            handleProxyPing(pData, size, addr);
            break;
         }

      case BPacketType::cProxyPongPacket:
         {
            handleProxyPong(pData, size, addr);
            break;
         }

      case BPacketType::cDisconnectRequest:
         {
            handleDisconnectRequest(pData, size, addr);
            break;
         }

      case BPacketType::cDisconnectSync:
         {
            handleDisconnectSync(pData, size, addr);
            break;
         }

      case BPacketType::cDisconnectResponse:
         {
            handleDisconnectResponse(pData, size, addr);
            break;
         }

      default:
         {
            BMachineID machineID = mapAddrToMachineID(addr);
            if (isConnected() && (machineID != -1))
            {
               nlog(cSessionNL, "BSession::dataReceived machineID[%ld]", machineID);
               addEvent(cEventClientData, machineID, size, pData);
            }
            else
            {
               BIncomingData* pIncomingData = HEAP_NEW(BIncomingData, gNetworkHeap);
               pIncomingData->init(cEventClientData, addr, pData, size);
               mIncomingData.add(pIncomingData);
            }
            break;
         }
   }
}

//==============================================================================
// Send an init peers packet to the given client so they can attempt a
//    connection to insure a fully connected graph
//==============================================================================
void BSession::sendPeersPacket(const SOCKADDR_IN& addr)
{
   nlog(cSessionNL, "BSession::sendPeersPacket -- sending cInitPeersRequestPacket.");

   BAddressesPacket packet(BPacketType::cInitPeersRequestPacket);

   for (int i=0; i < XNetwork::cMaxClients; i++)
   {
      const BMachine& machine = mMachines[i];
      if (machine.mState == BMachine::cStateConnected)
      {
         if (machine.mUsers[0].mClientID != -1 || machine.mUsers[1].mClientID != -1)
            packet.addAddress(mMachines[i].getID(), mMachines[i].mXnAddr, mMachines[i].mUsers);
      }
   }

   SendPacketToAddr(addr, packet);
}

//==============================================================================
// The host broadcasts an cInitPeerPacket that signals a new client needs to
//    be created.  The addr will be that of the host so it needs to be
//    ignored.  We'll have the XNADDR, XUID and Gamertag
//==============================================================================
void BSession::handleInitPeer(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BSessionConnectorPacket packet(BPacketType::cInitPeerPacket);
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleInitPeer -- for (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) from ip[%s].",
      packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
      packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
      strAddr.getPtr());

   // *** WARNING ***
   // since this is a broadcast packet, everyone will receive it, including the host and new client
   // I need to take this into account when attempting to create new clients
   //
   // the host and new client will already be created, this is intended for all the other clients
   // so the createClient code will need to check

   // if I'm receiving this packet, it's because a new client has hand-shaked correctly with the host
   // the host is now informing all the other peers that "Hey, they're good to go, let them in"
   //
   // create a new client and we'll also need to release incoming data

   // createClient will have to create a BMachine as well as 1-BMachine::cMaxUsers and their BClientHolder/BClient equivalents
   // this is very different than simply passing in the XUID, Gamertag and ClientIndex
   BMachineID machineID = -1;
   if (!createClient(packet.mXnAddr, packet.mUsers, machineID, packet.mProxy))
   {
      BASSERTM(FALSE, "Failed to create new client(s)");
      nlog(cSessionNL, "BSession::handleInitPeer -- failed to create a client for (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) from ip[%s]",
         packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
         packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
         strAddr.getPtr());
      return;
   }

   // after I've handled an init peer packet, I need to release incoming data for that addr
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;

   releaseIncomingData(mMachines[machineID]);
}

//==============================================================================
// The host broadcasts an cInitClientPacket that signals a new client needs to
//    be created.  The addr will be that of the host so it needs to be
//    ignored.  We'll have the XNADDR, XUID and Gamertag
//==============================================================================
void BSession::handleInitClient(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BSessionConnectorPacket packet(BPacketType::cInitClientPacket);
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleInitPeer -- for (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) from ip[%s].",
      packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
      packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
      strAddr.getPtr());

   BMachineID machineID = -1;
   if (!createClient(packet.mXnAddr, packet.mUsers, machineID, packet.mProxy))
   {
      BASSERTM(FALSE, "Failed to create new client(s)");
      nlog(cSessionNL, "BSession::handleInitPeer -- failed to create a client for (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) from ip[%s]",
         packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
         packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
         strAddr.getPtr());
      return;
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleJoinRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BSessionConnectorPacket packet(BPacketType::cJoinRequestPacket);
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleJoinRequest -- from (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) ip[%s].",
      packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
      packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
      strAddr.getPtr());

   BDEBUG_ASSERT(isHosted());
   //Ignore these if I'm not the host
   if (!isHosted())
      return;

   BJoinReasonCode result = cResultJoinOk;

   //Check for a CRC match - unless it is config'ed out
   if ((packet.mCRC != mChecksum)
#ifndef BUILD_FINAL
      && (!gConfig.isDefined(cConfigIgnoreMPChecksum))
#endif
      )
   {
      nlog(cSessionNL, " his CRC[%x] doesn't match local CRC[%x]. ", packet.mCRC, mChecksum);
      result = cResultRejectCRC;
   }
   else
   {
      //Check for too many clients
      if (mMaxClientAmount - getConnectedClientAmount() < 1)
      {
         result = cResultRejectFull;
      }
      else
      {
         //Check the callback to see if its ok for them to join
         if (!mpClientConnector)
         {
            nlog(cSessionNL, "BSession::handleJoinRequest -- Failed - I have no client connector");
            result = cResultRejectUnknown;
         }  
         //NOTE: the packet.XNADDR here is NOT THE ONE WE USE (we use the IN_ADDR we received this data on)- TODO: Need to remove it from the checks and the protocol
         else if (mpClientConnector->sessionConnectionRequest(packet.mUsers, &result))
         {
            nlog(cSessionNL, "BSession::handleJoinRequest sessionConnectionRequest observer called. Result: %d", result);

            // if we have enough space for him to join, then we need to send him information on the other "connected" clients
            // already in the session
            //
            // a "connected" client is defined as someone who has sucessfully XNetConnected to all the other peers
            //
            // if I send a list of peers to the client, do I (as the host) need to keep track of which ones I've sent?
            // or should I send him the known list, he attempts to connect to each peer, and sends a response with the success/failure
            // of each connection
            //
            // if someone connects in the meantime, I can check the client's response with the current list
            // and if there is a missing peer, I send the entire list again
            //
            // the client, upon receiving the list, will attempt to connect to any missing peers
            // the client response will be the status of each peer from the request
            //
            // when the host receives a initpeers response that matches the connected client list with all successes
            // then we can move the client from the pending list and initialize their BUDP360Connection/BClient/BClientHolder
            //
            // so how do I generate such a list?  Do I walk the BClientHolder list looking for "connected" clients?
            // can I assume that all BClientHolder clients are "connected"?
            //
            // for now, verify the cStateConnected setting
            sendPeersPacket(addr);
            return;
         }
      }
   }

   BDEBUG_ASSERTM(result != cResultJoinOk, "Observer callback failed, we should not be OK");

   // if we don't kick out of this method early, then we'll simply send the response now
   // this will be a rejection notice of sorts
   BSessionConnectorPacket responsePacket(BPacketType::cJoinResponsePacket, (long)result);

   HRESULT hr = SendPacketToAddr(addr, responsePacket);
   BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handleJoinRequest : Failed to send cJoinResponsePacket");
   if (FAILED(hr))
   {
      nlog(cSessionNL, "BSession::handleJoinRequest -- Failed to send cJoinResponsePacket with result %d to (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) ip[%s].",
         result,
         packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
         packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
         strAddr.getPtr());

      return;
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleClientJoinRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BSessionConnectorPacket packet(BPacketType::cClientJoinRequestPacket);
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleClientJoinRequest -- from (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) ip[%s].",
      packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
      packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
      strAddr.getPtr());

   BDEBUG_ASSERT(isHosted());
   //Ignore these if I'm not the host
   if (!isHosted())
      return;

   BJoinReasonCode result = cResultJoinOk;

   //Check for too many clients
   if (mMaxClientAmount - getConnectedClientAmount() < 1)
   {
      result = cResultRejectFull;
   }
   else
   {
      //Check the callback to see if its ok for them to join
      if (!mpClientConnector)
      {
         nlog(cSessionNL, "BSession::handleClientJoinRequest -- Failed - I have no client connector");
         result = cResultRejectUnknown;
      }  
      //NOTE: the packet.XNADDR here is NOT THE ONE WE USE (we use the IN_ADDR we received this data on)- TODO: Need to remove it from the checks and the protocol
      else if (mpClientConnector->sessionConnectionRequest(packet.mUsers, &result))
      {
         nlog(cSessionNL, "BSession::handleClientJoinRequest sessionConnectionRequest observer called. Result: %d", result);
      }
   }

   BMachineID machineID = -1;

   // attempt to create the new client locally if everything is ok
   if (result == cResultJoinOk)
   {
      if (!createClient(addr, packet.mUsers, machineID))
      {
         nlog(cSessionNL, "BSession::handleClientJoinRequest : failed to create client(s) for user1[%s], user2[%s]", packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative());
         result = cResultRejectUnknown;
      }
   }

   //BDEBUG_ASSERTM(result != cResultJoinOk, "Observer callback failed, we should not be OK");

   //Build the response to the requesting client packet
   BSessionConnectorPacket responsePacket(BPacketType::cClientJoinResponsePacket, (long)result);

   HRESULT hr = SendPacketToAddr(addr, responsePacket);
   BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handleClientJoinRequest : Failed to send cClientJoinResponsePacket");
   if (FAILED(hr))
   {
      nlog(cSessionNL, "BSession::handleClientJoinRequest -- Failed to send cClientJoinResponsePacket with result %d to (gamertag[%s], xuid[%I64u], clientID[%d]) (gamertag[%s], xuid[%I64u], clientID[%d]) ip[%s].",
         result,
         packet.mUsers[0].mGamertag.asNative(), packet.mUsers[0].mXuid, packet.mUsers[0].mClientID,
         packet.mUsers[1].mGamertag.asNative(), packet.mUsers[1].mXuid, packet.mUsers[1].mClientID,
         strAddr.getPtr());
      //TODO - we should drop the client we just created if this send failed... shouldn't we?
      // the only time this will fail is if we're OOM
      return;
   }

   if (result == cResultJoinOk)
   {
      // broadcast the new client to the rest of the world
      const BMachine& machine = mMachines[machineID];
      BSessionConnectorPacket initClientPacket(BPacketType::cInitClientPacket, machine.mXnAddr, machineID, machine.mUsers);

      hr = SendPacket(initClientPacket);
      BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handleClientJoinRequest : Failed to send cInitClientPacket");
      nlog(cSessionNL, "BSession::handleClientJoinRequest : send cInitClientPacket for user1[%s], user2[%s] [%x]", packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative(), hr);
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleJoinResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDEBUG_ASSERT(!isHosted());
   if (isHosted())
      return;

   mJoinRequestTimer = 0;

   BSessionConnectorPacket packet(BPacketType::cJoinResponsePacket);
   packet.deserializeFrom(pData, size);

   if (packet.mResult != cResultJoinOk)
   {
      nlog(cSessionNL, "BSession::handleJoinResponse - NO! - notifying observer. Result: %d", packet.mResult);
      addEvent(cEventJoinFailed, packet.mResult);
      return;
   }

   // first go through and filter out BXConnectors that have the same machineID as the one specified from the host
   //
   // this happens in the case where a client disconnects during the connection init phase
   // and since we don't send out a low level disconnect message from the host, there will be overlap
   for (int i=mXConnectors.getSize()-1; i >= 0; --i)
   {
      BXConnector& c = mXConnectors[i];

      if (c.getMachineID() == packet.mMachineID)
      {
         // if I'm removing a BXConnector, then I need to insure that another connection is not using
         // me as a proxy and simply clear our their proxy info
         for (int j=mXConnectors.getSize()-1; j >= 0; --j)
         {
            BXConnector& temp = mXConnectors[i];
            if (temp.isProxied() && temp.getProxyAddr().sin_addr.s_addr == c.getAddr().s_addr)
               temp.clearProxy();
         }

         c.terminate();
         nlog(cSessionNL, "BSession::handleJoinResponse - Removing overlapping machineID[%d]", packet.mMachineID);
         mXConnectors.removeIndex(i);
      }
   }

   // now that I've been accepted, I need to migrate all the BXConnectors into non pending connections
   // and create BClient/BClientHolders for all of them
   
   // if I disconnect during this process, they will fail on a ping and drop me from their lists
   //
   // need to fix the whole player list thing

   // first create myself
   createLocal(packet.mMachineID, packet.mUsers);

   // create clients for all the peers
   for (int i=mXConnectors.getSize()-1; i >= 0; --i)
   {
      const BXConnector& c = mXConnectors[i];

      createClient(c);
   }

   mXConnectors.clear();

#ifdef DEBUG
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      BNetIPString strMachineAddr(machine.getAddr());
      nlog(cSessionNL, "BSession::handleJoinResponse -- BMachine ID[%d], ip[%s]", i, strMachineAddr.getPtr());

      BClient* pClient = mClients[i].mpClient;
      if (pClient)
      {
         BNetIPString strClientAddr(pClient->getAddr());
         nlog(cSessionNL, "BSession::handleJoinResponse -- BClientHolder client ID[%d:%d], name[%s], ip[%s], xuid[%I64u]", i, pClient->getID(), pClient->getGamertag().asNative(), strClientAddr.getPtr(), pClient->getXuid());
      }
   }
#endif

   // now that I have clients and non-pending connections for all them
   // I can set myself into the "connected" state and trickle up the appropriate session events
   setConnected(true);
   addEvent(cEventConnected);

   releaseIncomingData();
}

//==============================================================================
// 
//==============================================================================
void BSession::handleClientJoinResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDEBUG_ASSERT(!isHosted());
   if (isHosted())
      return;

   mJoinRequestTimer = 0;

   BSessionConnectorPacket packet(BPacketType::cClientJoinResponsePacket);
   packet.deserializeFrom(pData, size);

   if (packet.mResult != cResultJoinOk)
   {
      nlog(cSessionNL, "BSession::handleClientJoinResponse - NO! - notifying observer. Result: %d", packet.mResult);
      // FIXME-COOP, change cEventJoinFailed event to specify which user(s) were rejected so we don't disconnect completely
      addEvent(cEventJoinFailed, packet.mResult);
      return;
   }

   // first create myself
   createLocal(packet.mMachineID, packet.mUsers);

#ifdef DEBUG
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      BNetIPString strMachineAddr(machine.getAddr());
      nlog(cSessionNL, "BSession::handleClientJoinResponse -- BMachine ID[%d], ip[%s]", i, strMachineAddr.getPtr());

      BClient* pClient = mClients[i].mpClient;
      if (pClient)
      {
         BNetIPString strClientAddr(pClient->getAddr());
         nlog(cSessionNL, "BSession::handleClientJoinResponse -- BClientHolder client ID[%d:%d], name[%s], ip[%s], xuid[%I64u]", i, pClient->getID(), pClient->getGamertag().asNative(), strClientAddr.getPtr(), pClient->getXuid());
      }
   }
#endif
}

//==============================================================================
//
//==============================================================================
void BSession::setLocalXnAddr(const XNADDR& xnaddr)
{
   //todo - if we are already hosting/joined or in process for that - then this call should fail
   //todo - ^^^^^^^^^^^^^^^^^^^^ why?
   mLocalXnAddr = xnaddr;
}

//==============================================================================
//
//==============================================================================
void BSession::setMaxClientCount(long count)
{
   if (count < 0)
   {
      BFAIL("BSession::setMaxClientCount -- invalid parameter.");
      return;
   }

   BASSERTM(count <= XNetwork::cMaxClients, "Invalid max client count");
   if (count > XNetwork::cMaxClients)
      return;

   //long oldCount = mMaxClientAmount;
   long oldCount = 0;
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mpClient)
         oldCount++;
   }
   // Should this just drop the connections immediately, or just let the host kick the players?

   if (!isHosted())
   {
      mMaxClientAmount = count;
      return;
   }

   // if decreasing the number of players
   HRESULT hr = E_FAIL;   
   for (long idx=count; idx<oldCount; idx++)
   {
      // kick the extras
      if (mClients[idx].mState <= BClientHolder::cStateKicked)
      {
         hr = kickClient(idx);
         if(FAILED(hr))
            nlog(cSessionNL, "BSession::setMaxClientCount -- failed kickClient.");
      }
   }

   mMaxClientAmount = count;
}

//==============================================================================
//
//==============================================================================
HRESULT BSession::kickClient(const long clientIndex)
{
   if (!isHosted())
      return E_ACCESSDENIED; // gotta be host to do this

   if ((clientIndex < 0) || (clientIndex >= XNetwork::cMaxClients) || !mClients[clientIndex].mpClient)
      return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);

   nlog(cSessionNL, "BSession::kickClient -- client ID[%ld]", clientIndex);

   BTypedMessageDataPacket packet(clientIndex, 0, 0, BPacketType::cKickPacket);
   HRESULT hr = SendPacket(packet);
   if (FAILED(hr))
      nlog(cSessionNL, "Failed to packet to client ID[%ld]", clientIndex);

   return S_OK;
}

//==============================================================================
//
//==============================================================================
void BSession::disconnectClient(const long clientIndex)
{
   beginClientDisconnect(clientIndex, cHostDecision);
}

//==============================================================================
// All disconnect paths should start here
// The other disconnect* methods will be removed
// This should help in consolidating the disconnect process
//==============================================================================
void BSession::beginClientDisconnect(const long clientIndex, BDisconnectReason reason)
{
   nlog(cSessionNL, "BSession::beginClientDisconnect -- client ID[%ld]", clientIndex);

   beginClientDisconnectInternal(clientIndex, true, reason);
}

//==============================================================================
//
//==============================================================================
void BSession::beginMachineDisconnectInternal(const BMachineID machineID, bool init, BDisconnectReason reason)
{
   BASSERTM(machineID >= 0 && machineID < XNetwork::cMaxClients, "Invalid machineID");
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
      return;
   BMachine& machine = mMachines[machineID];
   if (machine.mState < BMachine::cStateDisconnecting)
      machine.mState = BMachine::cStateDisconnecting;
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (machine.mUsers[i].mClientID != -1)
         beginClientDisconnectInternal(machine.mUsers[i].mClientID, init, reason);
   }
}

//==============================================================================
//
//==============================================================================
void BSession::beginClientDisconnectInternal(const long clientID, bool init, BDisconnectReason reason)
{
   if (clientID < 0 || clientID >= XNetwork::cMaxClients)
   {
      BDEBUG_ASSERTM(0, "BSession::beginClientDisconnectInternal -- invalid clientID.");
      return;
   }

   if (mClients[clientID].mpClient == NULL || mClients[clientID].mState > BClientHolder::cStateKicked)
   {
      nlog(cSessionNL, "BSession::beginClientDisconnectInternal -- invalid client or state is not <= connected");
      return;
   }

   nlog(cSessionNL, "BSession::beginClientDisconnectInternal -- client ID[%ld]", clientID);

   // set the BClientHolder state to cStateDisconnecting
   // send a message to the connection thread to clean up this client
   //    should that be a fire and forget event?
   // I still need to unregister the inaddr
   //
   // if I call XNetUnregisterInAddr here, the socket layer could raise an error back to the connection layer
   // then the connection layer will raise an event to the session layer saying that the transport was lost
   //
   // is that the path I should take?
   //
   // so, unregister inaddr, socket throws exception when we attempt to flush
   // I'll get the transport lost/disconnect event

   // set our BClientHolder state to disconnecting
   mClients[clientID].mState = BClientHolder::cStateDisconnecting;
   mClients[clientID].mDisconnectReason = static_cast<int32>(reason);
   // set our BClient to disconnected
   // this will require testing to insure that we don't have some layers that
   // are triggering on an isConnected() check on BClient that would cause destructive behavior
   // what should happen is they simply stop sending data to the client/including them in checks
   BClient* pClient = mClients[clientID].mpClient;
   // NULL check made above
   XUID targetXuid = pClient->getXuid();
   pClient->setConnected(false);

   // this is where things get tricky
   //
   // previously we were able to unregister their IN_ADDR and then inject a socket error message
   // to allow for the teardown of the connection
   //
   // now we can have two clients on a single connection so we only want to tear down the connection
   // if both users are disconnect(ing|ed)
   //
   if (mClients[clientID].mMachineID < 0 || mClients[clientID].mMachineID >= XNetwork::cMaxClients)
      return;

   BMachine& machine = mMachines[mClients[clientID].mMachineID];

   bool disconnectMachine = true;

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (machine.mUsers[i].mClientID >= 0 && machine.mUsers[i].mClientID < XNetwork::cMaxClients)
      {
         if (mClients[machine.mUsers[i].mClientID].mState < BClientHolder::cStateDisconnecting)
         {
            disconnectMachine = false;
            break;
         }
      }
   }

   if (!disconnectMachine)
   {
      //Need to clean up this connection since it will not be hit by the finishClientDisconnect code
      addEvent(cEventClientDisconnect, clientID, (mClients[clientID].mMachineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF), NULL, targetXuid, static_cast<int32>(reason));

      // deinitialize a client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(targetXuid);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceDeinitClient, 0, 0, pVoiceRequest);

      //Moved the actual delete to the service update so that it is always safe to do
      mClients[clientID].mState = BClientHolder::cStateDisconnected;

      //Clear the entry in the machine structure
      for (uint i=0; i < BMachine::cMaxUsers; ++i)
      {
         if (machine.mUsers[i].mClientID == clientID)
         {
            machine.mUsers[i].reset();
            break;
         }
      }
      return;
   }

   if (machine.mState < BMachine::cStateDisconnecting)
      machine.mState = BMachine::cStateDisconnecting;

   const IN_ADDR& addr = machine.getAddr();

   if (addr.s_addr == INADDR_LOOPBACK)
      return;

   int retval = XNetUnregisterInAddr(addr);
   if (retval != 0)
   {
      BNetIPString strAddr(addr);
      nlog(cSessionNL, "BSession::beginClientDisconnectInternal -- failed to unregister client IN_ADDR[%s] error[%d]", strAddr.getPtr(), retval);

      // if we failed to unregister the IN_ADDR, then we need to kick off the disconnect process
      //
      // if we're initiating this process, then send a socket error to the connection layer to
      // begin the deinit process
      if (init)
         gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventSocketError, WSAEDISCON, addr.s_addr);
   }
}

//==============================================================================
//
//==============================================================================
void BSession::socketDisconnect(const BMachineID machineID, const uint error)
{
   // the socket has disconnected this machineID
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
   {
      //BDEBUG_ASSERTM(0, "BSession::socketDisconnect -- invalid machineID.");
      return;
   }

   // only want to call finishClientDisconnect ONCE
   //
   // socketDisconnect may be called with an error of WSAEDISCON multiple times from an explicit disconnect
   // 
   // socketDisconnect may be called with WSAEDISCON if an attempt to locate a proxy fails in which case our BMachine state is cStateDisconnecting
   //
   // socketDisconnect may be called if we initiate a client disconnect which sets our BMachine state to cStateDisconnecting
   // and then we receive either WSAEDISCON or WSAEHOSTUNREACH errors

   if (mMachines[machineID].mState > BMachine::cStateKicked)
   {
      if (mMachines[machineID].mState >= BMachine::cStateDisconnecting)
      {
         nlog(cSessionNL, "BSession::socketDisconnect -- machine at index [%d] is in state [%d]", machineID, mMachines[machineID].mState);
         // if BTimeSync is not running, then I need to insure that we finish the client disconnect process
         if (mpTimeSync == NULL)
         {
            finishClientDisconnect(machineID);
            //nlog(cSessionNL, "BSession::socketDisconnect -- In state disconnecting, and no timesync - I am finishClientDisconnect'ing this right now");
         }
         return;
      }
   }

   nlog(cSessionNL, "BSession::socketDisconnect -- machineID[%ld], error[%d]", machineID, error);

   BDisconnectReason reason = cNormal;
   // anything other than WSAEDISCON will be considered a non user-initiated connection loss
   if (error != WSAEDISCON)
      reason = cTransportLost;

   // this will set us to BMachine::cStateDisconnecting unless we're already cStateDisconnected or something
   beginMachineDisconnectInternal(machineID, false, reason);

   // now that we're shutdown at the socket/connection layer, I can initiate the sync shutdown
   //
   // if the time sync system is not running, then I can skip to the final disconnect
   if (mpTimeSync)
   {
      // instead of calling BTimeSync's startClientDisconnect method,
      // we need to count the number of currently connected BMachines
      // and then broadcast a cDisconnectRequest packet.
      //
      // we should be expecting back X number of cDisconnectResponse packets
      // but that number needs to be adjusted as more machines disconnect
      //
      // actually, I can't do a simple number, because if another machine
      // disconnects, then I need to start the process again and broadcast
      // another cDisconnectRequest for that machine
      //
      // so what I really need is a disconnect tracker by machine ID.
      // when I issue a disconnect request, make note for which machine it was for
      // then as responses come back, verify that we have all the responses we're going to get
      //
      // if we're expecting a response from a machine that also disconnects, should
      // I filter through all the disconnect tracker entries and mark them as never going to respond?

      if (!mDisconnectTracker.add(machineID))
      {
         BASSERTM(0, "Failed to initiate session disconnect tracking");
         nlog(cSessionNL, "BSession::socketDisconnect -- failed to init disconnect tracking for machineID[%d]", machineID);
      }
      else
      {
         // create a disconnect request packet and broadcast
         //
         // loop all connected machines and set their disconnect state to cDisconnectSent
         BDisconnectEntry* pEntry = mDisconnectTracker.get(machineID);
         BASSERT(pEntry != NULL);
         if (pEntry)
         {
            for (uint i=0; i < XNetwork::cMaxClients; ++i)
            {
               const BMachine& machine = mMachines[i];
               if (machine.isConnected())
               {
                  pEntry->set(machine.getID(), BDisconnectEntry::cStateSent);
               }
            }
            // set our tracker entry to cStateSent
            // this state is independent of the individual disconnect requests that I sent out
            pEntry->setState(BDisconnectEntry::cStateSent);
         }

         // this will iterate over all disconnect entries and bump the given machineID into the done state
         // this is for cases where we've sent out a disconnect request to the given machine ID but
         // then they disconnected while we were waiting for their disconnect response
         mDisconnectTracker.set(machineID, BDisconnectEntry::cStateDone);

         BDisconnectPacket packet(BPacketType::cDisconnectRequest);
         packet.init(mChannelTracker[machineID]);
         HRESULT hr;
         hr = SendPacket(packet);
         BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::socketDisconnect : Failed to send cDisconnectRequest");
         nlog(cSessionNL, "BSession::socketDisconnect : send cDisconnectRequest for machineID[%d] [%x]", machineID, hr);
      }
   }
   else
      finishClientDisconnect(machineID);
}

//==============================================================================
//
//==============================================================================
void BSession::checkSessionDisconnect()
{
   BMachineID lastMachineID = BMachine::cInvalidMachineID;
   uint connectedMachineCount = 0;
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mMachines[i].mState == BMachine::cStateConnected)
      {
         connectedMachineCount++;
         lastMachineID = mMachines[i].getID();
      }
   }

   // [8/26/2008 DPM] If I disconnect the last player, then I get a win by default since I'd be the last remaining player
   // what I want to have happen is I disconnect everyone to prevent people from screwing over other players
   //
   // if a player explicitly disconnects from the game via sending disconnect packets, then I do want to resign them
   // if a player loses their connection via some other means, then I want to disconnect the session FIRST
   //
   // if there is only one machine remaining, then raise cEventDisconnected, but ONLY if there's a single user for this machine
   //
   // or the last machine is gone or the last machine ID is not the host and time sync is not running (in the case of the party session)
   // or the last machine is valid and time sync is running, in which case the client count should catch the split screen session
   //
   // otherwise, if this was an actual game and we only had one machine left, then we have to disconnect the session
   if (connectedMachineCount <= 1 && (lastMachineID == BMachine::cInvalidMachineID || mpTimeSync != NULL || (lastMachineID != BMachine::cInvalidMachineID && mpTimeSync == NULL && lastMachineID != mHostMachineID)))
   {
      uint clientCount = 0;
      for (uint i=0; i < BMachine::cMaxUsers; ++i)
      {
         if (mMachines[lastMachineID].mUsers[i].mClientID != -1)
            clientCount++;
      }
      if (clientCount == 1)
      {
         BASSERTM(lastMachineID == mLocalMachineID, "mLocalMachineID was reset before it should have been");
         addEvent(cEventDisconnected, cNormal);
         nlog(cSessionNL, "BSession::finishClientDisconnect -- addEvent cEventDisconnected, cNormal");
      }
   }
}

//==============================================================================
//
//==============================================================================
void BSession::finishClientDisconnect(BMachineID machineID)
{
   if (machineID < 0 || machineID >= XNetwork::cMaxClients)
   {
      BDEBUG_ASSERTM(0, "Invalid machineID, that's unpossible");
      return;
   }

   // only want to call finishClientDisconnect once
   //
   // this is the only method that sets our BMachine state to cStateDisconnected
   if (mMachines[machineID].mState >= BMachine::cStateDisconnected)
      return;

   nlog(cSessionNL, "BSession::finishClientDisconnect  -- machineID[%ld]", machineID);

   mDisconnectTracker.reset(machineID);

   mMachines[machineID].mState = BMachine::cStateDisconnected;

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      int32 clientIndex = mMachines[machineID].mUsers[i].mClientID;
      if (clientIndex == -1)
         continue;

//-- FIXING PREFIX BUG ID 7560
      const BClient* pClient = mClients[clientIndex].mpClient;
//--
      BDEBUG_ASSERTM(pClient, "Missing BClient, this should still exist");
      XUID xuid = 0;
      if (pClient != NULL)
         xuid = pClient->getXuid();
      int32 reason = mClients[clientIndex].mDisconnectReason;

      // if the reason was anything but normal, check to see if we should disconnect the entire session
      if (reason != BSession::cNormal)
         checkSessionDisconnect();

      addEvent(cEventClientDisconnect, clientIndex, (machineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF), NULL, xuid, reason);

      // deinitialize a client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(xuid);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceDeinitClient, 0, 0, pVoiceRequest);

      //Moved the actual delete to the service update so that it is always safe to do
      mClients[clientIndex].mState = BClientHolder::cStateDisconnected;

      //Clear the entry in the machine structure
      mMachines[machineID].mUsers[i].reset();
   }

   if (mHostMachineID == machineID)
      mHostMachineID = -1;
   if (mLocalMachineID == machineID)
      mLocalMachineID = -1;

   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventDeinitPeer, WSAEDISCON, mMachines[machineID].getAddr().s_addr);
}

//==============================================================================
// Once we've received the Join OK response from the host, we can release
//    any pending incoming data
//
// The times we'll receive data like this are when we're lagging with the host
//    while other clients are not, i.e. they've received OK responses and
//    begin transmitting game data to us
//==============================================================================
void BSession::releaseIncomingData()
{
   nlog(cSessionNL, "BSession::releaseIncomingData");

   for (uint i=0; i < mIncomingData.getSize(); ++i)
   {
      BIncomingData* pIncomingData = mIncomingData[i];
      BDEBUG_ASSERT(pIncomingData);
      if (pIncomingData == NULL)
         continue;

      // lookup the client index from the given incoming data SOCKADDR_IN
      // and add a cEventClientData event for the index
      BMachineID machineID = mapAddrToMachineID(pIncomingData->mAddr);
      if (machineID != -1)
      {
         addEvent(pIncomingData->mEventID, machineID, pIncomingData->mSize, pIncomingData->mBuf);

         HEAP_DELETE(pIncomingData, gNetworkHeap);

         mIncomingData.removeIndex(i);
         i--;
      }
   }
}

//==============================================================================
//
//==============================================================================
void BSession::releaseIncomingData(const BMachine& machine)
{
   BNetIPString strAddr(machine.getSockAddr());
   nlog(cSessionNL, "BSession::releaseIncomingData for ip[%s]", strAddr.getPtr());

   for (uint i=0; i < mIncomingData.getSize(); ++i)
   {
      BIncomingData* pIncomingData = mIncomingData[i];
      BDEBUG_ASSERT(pIncomingData);
      if (pIncomingData == NULL)
         continue;

      // lookup the client index from the given incoming data SOCKADDR_IN
      // and add a cEventClientData event for the index
      if (pIncomingData->mAddr.sin_addr.s_addr == machine.getAddr().s_addr)
      {
         addEvent(pIncomingData->mEventID, machine.getID(), pIncomingData->mSize, pIncomingData->mBuf);

         HEAP_DELETE(pIncomingData, gNetworkHeap);

         mIncomingData.removeIndex(i);
         i--;
      }
   }
}

//==============================================================================
//
//==============================================================================
void BSession::setConnected(bool v)
{
   if (v)
      mFlags |= cFlagConnected;
   else
      mFlags &= ~cFlagConnected;
}

//==============================================================================
//
//==============================================================================
void BSession::setDisconnecting(bool v)
{
   if (v)
      mFlags |= cFlagDisconnecting;
   else
      mFlags &= ~cFlagDisconnecting;
}

//==============================================================================
// Use mClients instead of mMachines here because we're interested
// in how many clients are participating in the session and not
// necessarily the number of physical connections
//==============================================================================
int32 BSession::getConnectedClientAmount() const
{
   int32 a=0;
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mState == BClientHolder::cStateConnected)
         a++;
   }
   return a;
}

//==============================================================================
//
//==============================================================================
int32 BSession::getActiveClientAmount() const
{
   int32 a = 0;
   for (int i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mpClient)
         a++;
   }
   return a;
}

//==============================================================================
// 
//==============================================================================
uint BSession::getActiveMachineAmount() const
{
   uint c = 0;
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mMachines[i].isConnected())
         c++;
   }
   return c;
}

//==============================================================================
// 
//==============================================================================
uint32 BSession::getMaxPingDeviation() const
{
   uint32 dev=0, t=0;

   for (int i=getClientCount()-1; i >= 0; --i)
   {
//-- FIXING PREFIX BUG ID 7561
      const BClient* pClient = getClient(i);
//--
      if (pClient != NULL)
      {
         t = pClient->getPingStdDev();
         if (t > dev)
            dev = t;
      }
   }

   return dev;
}

//==============================================================================
// 
//==============================================================================
uint32 BSession::getMaxPing() const
{
   uint32 ping = 0;

   for (int i=getClientCount()-1; i >= 0; --i)
   {
//-- FIXING PREFIX BUG ID 7562
      const BClient* pClient = getClient(i);
//--
      if (pClient != NULL)
      {
         if (pClient->getPingAverage() > ping)
            ping = pClient->getPingAverage();
      }
   }

   return ping;
}

//==============================================================================
// 
//==============================================================================
uint32 BSession::getAveragePing() const
{
   uint32 ping = 0;
   uint32 clientCount = 0;

   for (int i=getClientCount()-1; i >= 0; --i)
   {
//-- FIXING PREFIX BUG ID 7563
      const BClient* pClient = getClient(i);
//--
      if (pClient != NULL)
      {
         ping += pClient->getPingAverage();
         clientCount++;
      }
   }

   if (clientCount<2 || ping==0)
      return 0;

   ping = uint32(ping / clientCount);

   return ping;
}

//==============================================================================
// This should only be called from the sim helper thread
//==============================================================================
BXConnection* BSession::getConnection(uint64 id)
{
   for (int i=mXConnections.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnections[i];
      if (pConn == NULL)
      {
         mXConnections.removeIndex(i, false);
      }
      else if (pConn->getUniqueID() == id)
      {
         return pConn;
      }
   }

   for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnectionsPending[i];
      if (pConn == NULL)
      {
         mXConnectionsPending.removeIndex(i, false);
      }
      else if (pConn->getUniqueID() == id)
      {
         return pConn;
      }
   }

   return NULL;
}

//==============================================================================
// This should only be called from the sim helper thread
//==============================================================================
BXConnection* BSession::getConnectionByAddr(const IN_ADDR& addr)
{
   for (int i=mXConnections.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnections[i];
      if (pConn == NULL)
      {
         mXConnections.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr.s_addr)
      {
         return pConn;
      }
   }

   for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnectionsPending[i];
      if (pConn == NULL)
      {
         mXConnectionsPending.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr.s_addr)
      {
         return pConn;
      }
   }

   return NULL;
}

//==============================================================================
// This should only be called from the sim helper thread
//==============================================================================
BXConnection* BSession::getConnectionByAddr(const uint addr)
{
   for (int i=mXConnections.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnections[i];
      if (pConn == NULL)
      {
         mXConnections.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr)
      {
         return pConn;
      }
   }

   for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnectionsPending[i];
      if (pConn == NULL)
      {
         mXConnectionsPending.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr)
      {
         return pConn;
      }
   }

   return NULL;
}

//==============================================================================
// Similar to GetUniqueID except we first attempt to resolve the MachineID
//==============================================================================
uint64 BSession::getUniqueID(const XNADDR& xnaddr)
{
   uint64 id;

   if (xnaddr.inaOnline.s_addr != 0 && XNetXnAddrToMachineId(&xnaddr, &id) == 0)
      return id;

   id = *(uint64*)&xnaddr.abEnet;
   id = id >> ((sizeof(uint64) - sizeof(xnaddr.abEnet)) * 8);

   return id;
}

//==============================================================================
//
//==============================================================================
BMachineID BSession::mapAddrToMachineID(const SOCKADDR_IN& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      if (machine.getAddr().s_addr == addr.sin_addr.s_addr)
         return i;
   }

   return -1;
}

//==============================================================================
//
//==============================================================================
BMachineID BSession::mapAddrToMachineID(const IN_ADDR& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      if (machine.getAddr().s_addr == addr.s_addr)
         return i;
   }

   return -1;
}

//==============================================================================
//
//==============================================================================
long BSession::mapAddrToClientIndex(const SOCKADDR_IN& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      if (machine.getAddr().s_addr == addr.sin_addr.s_addr)
         return i;
   }

   return -1;
}

//==============================================================================
//
//==============================================================================
long BSession::mapAddrToClientIndex(const IN_ADDR& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      if (machine.getAddr().s_addr == addr.s_addr)
         return i;
   }

   return -1;
}

//==============================================================================
// Helper function for external classes to find what machine a client belongs to
//==============================================================================
BMachineID BSession::findMachineIDFromClientID(BClientID clientID)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine& machine = mMachines[i];
      for (uint j=0; i < BMachine::cMaxUsers; ++j)
      {
         if (machine.mUsers[j].mClientID == clientID)
            return machine.mID;
      }
   }

   return BMachine::cInvalidMachineID; 
}

//==============================================================================
//
//==============================================================================
BMachine* BSession::getMachineByAddr(const IN_ADDR& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      BMachine& machine = mMachines[i];
      if (machine.getAddr().s_addr == addr.s_addr)
         return &machine;
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
BMachine* BSession::getMachineByXnAddr(const XNADDR& addr)
{
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      BMachine& machine = mMachines[i];
      if (CompareXnAddr(machine.getXnAddr(), addr))
         return &machine;
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
const BProxyRequest* BSession::getProxyRequestByAddr(const IN_ADDR& addr) const
{
   uint count = mProxyRequests.getSize();
   for (uint i=0; i < count; ++i)
   {
      const BProxyRequest* pProxy = mProxyRequests[i];
      if (pProxy->getFromAddr().s_addr == addr.s_addr)
         return pProxy;
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
const BProxyRequest* BSession::getProxyRequestByAddrAndXnAddr(const SOCKADDR_IN& addr, const XNADDR& xnaddr) const
{
   uint count = mProxyRequests.getSize();
   for (uint i=0; i < count; ++i)
   {
      const BProxyRequest* pProxy = mProxyRequests[i];
      if (pProxy->getFromAddr().s_addr == addr.sin_addr.s_addr && CompareXnAddr(xnaddr, pProxy->getToXnAddr()))
         return pProxy;
   }
   return NULL;
}

//==============================================================================
//
//==============================================================================
BMachineID BSession::findNextAvailableMachineID()
{
   // find an available BMachine slot
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mMachines[i].mID == -1)
         return i;
   }
   return -1;
}

//==============================================================================
//
//==============================================================================
int32 BSession::findNextAvailableClientID(int32 startingIndex)
{
   for (uint i=startingIndex; i < XNetwork::cMaxClients; i++)
   {
      if (mClients[i].mpClient == NULL)
         return i;

      if (mClients[i].mState == BClientHolder::cStateDeleteMe || mClients[i].mState == BClientHolder::cStateDisconnected)
      {
         if (mClients[i].mpClient != NULL)
         {
            HEAP_DELETE(mClients[i].mpClient, gNetworkHeap);
            mClients[i].mpClient = NULL;
         }
         return i;
      }
   }

   return -1;
}

//==============================================================================
//
//==============================================================================
bool BSession::findNextAvailableClientID(int32& index1, int32& index2)
{
   index1 = findNextAvailableClientID();

   if (index1 == -1)
      return false;

   index2 = findNextAvailableClientID(index1+1);

   // found at least one
   return true;
}

//==============================================================================
// Internal helper call for transition to split-screen
//
// addUser() will call this method
//
// need to fix up the API, but this is mostly for testing/proof-of-concept
// while we sort out split-screen networking issues
//==============================================================================
bool BSession::createUser(uint controllerID, XUID xuid, const BSimString& gamertag)
{
   BASSERTM(mHostPort != 0, "Host port not set");
   BASSERTM(mHostMachineID != -1, "Host index not set");
   BASSERTM(mLocalMachineID != -1, "Local index not set");

   if (mLocalMachineID == -1)
      return false;

   BSessionUser* pSessionUser = NULL;
   BMachine& machine = mMachines[mLocalMachineID];

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (machine.mUsers[i].mXuid == xuid)
         return true;
      else if (machine.mUsers[i].mXuid == 0)
      {
         pSessionUser = &machine.mUsers[i];
         break;
      }
   }

   if (pSessionUser == NULL)
      return false;

   SOCKADDR_IN addr;
   IGNORE_RETURN(Utils::FastMemSet(&addr, 0, sizeof(SOCKADDR_IN)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_LOOPBACK;
   addr.sin_port = mHostPort;

   int32 index = -1;
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mClients[i].mpClient == NULL)
      {
         index = i;
         break;
      }
   }

   BASSERTM(index != -1, "Failed to find an open BClientHolder slot");
   if (index == -1)
      return false;

   pSessionUser->mControllerID = controllerID;
   pSessionUser->mXuid = xuid;
   pSessionUser->mGamertag = gamertag;
   pSessionUser->mClientID = index;

   // broadcast the local users to all other clients
   // this needs to happen after we assign the appropriate values to BMachine::mUsers
   BSessionConnectorPacket initPeerPacket(BPacketType::cInitClientPacket, machine.mXnAddr, mLocalMachineID, machine.mUsers);
   HRESULT hr;
   hr = SendPacket(initPeerPacket);
   BASSERTM(hr == S_OK, "Failed to broadcast cInitPeerPacket for local user"); 

   mClients[index].mMachineID = mLocalMachineID;
   mClients[index].mState = BClientHolder::cStateConnected;

   BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);

   mClients[index].mpClient = pClient;

   pClient->init(index, xuid, gamertag, mLocalXnAddr, addr);
   pClient->setConnected(true);

   // initialize a new client in the voice system
   BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
   pVoiceRequest->init(index, controllerID, xuid, addr);
   gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

   nlog(cSessionNL, "BSession::createUser index[%ld], controllerID[%d], xuid[%I64u], gamertag[%s]", index, controllerID, xuid, gamertag.asNative());
   addEvent(cEventClientConnect, index, (mLocalMachineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));

   return true;
}

//==============================================================================
// Only the host calls this one
//==============================================================================
bool BSession::createHost()
{
   SOCKADDR_IN addr;
   IGNORE_RETURN(Utils::FastMemSet(&addr, 0, sizeof(SOCKADDR_IN)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_LOOPBACK;
   addr.sin_port = mHostPort;

   BMachine& machine = mMachines[mHostMachineID];
   machine.init(mHostMachineID, mLocalXnAddr, addr.sin_addr, mHostPort, BMachine::cStateConnected);

   // initialize the channel tracker for use in syncing channel data when a client disconnects
   mChannelTracker[mHostMachineID].init(mHostMachineID);

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      machine.mUsers[i] = mLocalUsers[i];

      if (machine.mUsers[i].mXuid == INVALID_XUID)
         continue;

      int32 index = machine.mUsers[i].mClientID = i;

      BClientHolder& client = mClients[index];

      BASSERTM(client.mpClient == NULL, "BClient already exists?");
      if (client.mpClient != NULL)
      {
         HEAP_DELETE(client.mpClient, gNetworkHeap);
         client.mpClient = NULL;
      }

      client.mMachineID = machine.getID();
      client.mState = BClientHolder::cStateConnected;

      BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);
      client.mpClient = pClient;

      pClient->init(index, machine.mUsers[i].mXuid, machine.mUsers[i].mGamertag, mLocalXnAddr, addr);
      pClient->setConnected(true);

      // initialize a new client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(index, machine.mUsers[i].mControllerID, machine.mUsers[i].mXuid, addr);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

      nlog(cSessionNL, "BSession::createHost clientID[%d], localMachineID[%d], hostMachineID[%d]", index, mLocalMachineID, mHostMachineID);
      addEvent(cEventClientConnect, index, (mHostMachineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));
   }

   BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
   pPayload->init(addr, mLocalXnAddr);

   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInit, 0, 0, pPayload);

   setConnected(true);
   addEvent(cEventConnected);

   return true;
}

//==============================================================================
//
//==============================================================================
bool BSession::createLocal(BMachineID machineID, BSessionUser users[])
{
   BMachine* pMachine = NULL;

   SOCKADDR_IN addr;
   IGNORE_RETURN(Utils::FastMemSet(&addr, 0, sizeof(SOCKADDR_IN)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_LOOPBACK;
   addr.sin_port = mHostPort;

   if (mLocalMachineID == -1)
   {
      mLocalMachineID = machineID;

      pMachine = &mMachines[mLocalMachineID];
      pMachine->init(mLocalMachineID, mLocalXnAddr, addr.sin_addr, mHostPort, BMachine::cStateConnected);

      mChannelTracker[mLocalMachineID].init(mLocalMachineID);

      BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
      pPayload->init(addr, mLocalXnAddr);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInit, 0, 0, pPayload);
   }
   else
   {
      pMachine = &mMachines[mLocalMachineID];
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (users[i].mXuid == 0 || users[i].mClientID == -1)
         continue;

      // if this user has already been created, skip it
      if (users[i].mXuid == pMachine->mUsers[0].mXuid || users[i].mXuid == pMachine->mUsers[1].mXuid)
         continue;

      // find us a slot in the local user's list
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
      {
         if (pMachine->mUsers[j].mXuid == INVALID_XUID)
         {
            pMachine->mUsers[j] = users[i];
            break;
         }
      }

      if (pMachine->mUsers[i].mXuid == mLocalUsers[0].mXuid)
         pMachine->mUsers[i].mControllerID = mLocalUsers[0].mControllerID;
      else if (pMachine->mUsers[i].mXuid == mLocalUsers[1].mXuid)
         pMachine->mUsers[i].mControllerID = mLocalUsers[1].mControllerID;

      int32 index = users[i].mClientID;

      BClientHolder& client = mClients[index];

      client.mMachineID = mLocalMachineID;
      client.mState = BClientHolder::cStateConnected;

      if (client.mpClient != NULL)
      {
         BDEBUG_ASSERTM(false, "BClient already exists");
         HEAP_DELETE(client.mpClient, gNetworkHeap);
         client.mpClient = NULL;
      }

      BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);
      client.mpClient = pClient;

      pClient->init(index, users[i].mXuid, users[i].mGamertag, mLocalXnAddr, addr);
      pClient->setConnected(true);

      // initialize a new client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(index, pMachine->mUsers[i].mControllerID, pMachine->mUsers[i].mXuid, addr);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

      nlog(cSessionNL, "BSession::createLocal %d", index);
      addEvent(cEventClientConnect, index, (mLocalMachineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
      mLocalUsers[i].reset();

   return true;
}

//==============================================================================
// The host calls this to create a new client upon a successful response
//    to the init peers request
//==============================================================================
bool BSession::createClient(const SOCKADDR_IN& addr, BSessionUser users[], BMachineID& machineID)
{
   int32 index1, index2;

   if (!findNextAvailableClientID(index1, index2))
      return false;

   //todo - possible join issue on second user if there is only one slot
   if (index1 == -1 || (index2 == -1 && users[1].mXuid != 0))
      return false;

   BMachine* pMachine = NULL;

   XNADDR xnaddr;

   // first insure that this client does not already exist in our clients list
   BMachineID existingMachineID = mapAddrToMachineID(addr);
   if (existingMachineID != -1)
   {
      machineID = existingMachineID;

      pMachine = &mMachines[machineID];

      xnaddr = pMachine->getXnAddr();
   }
   else
   {
      // find an empty client slot
      machineID = findNextAvailableMachineID();
      if (machineID == -1)
         return false;

      int retval = XNetInAddrToXnAddr(addr.sin_addr, &xnaddr, NULL);
      BDEBUG_ASSERT(retval == 0);
      if (retval != 0)
         return false;

      BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
      pPayload->init(addr, xnaddr);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInit, 0, 0, pPayload);

      pMachine = &mMachines[machineID];
      pMachine->init(machineID, xnaddr, addr.sin_addr, mHostPort, BMachine::cStateConnected);

      mChannelTracker[machineID].init(machineID);
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (users[i].mXuid == INVALID_XUID)
         continue;

      // if this user has already been created, skip it
      if (users[i].mXuid == pMachine->mUsers[0].mXuid || users[i].mXuid == pMachine->mUsers[1].mXuid)
         continue;

      int32 index = (i == 0 ? index1 : index2);

      users[i].mClientID = index;

      // add us to the machine's user list
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
      {
         if (pMachine->mUsers[j].mXuid == INVALID_XUID)
         {
            pMachine->mUsers[j] = users[i];
            break;
         }
      }

      BClientHolder& client = mClients[index];

      client.mMachineID = machineID;
      client.mState = BClientHolder::cStateConnected;

      BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);

      client.mpClient = pClient;

      pClient->init(index, users[i].mXuid, users[i].mGamertag, xnaddr, addr);
      pClient->setConnected(true);

      // initialize a new client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(index, users[i].mControllerID, users[i].mXuid, addr);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

      nlog(cSessionNL, "BSession::createClient on host, name[%s], index[%d]", users[i].mGamertag.asNative(), index);
      addEvent(cEventClientConnect, index, (machineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));
   }

   return true;
}

//==============================================================================
// Called when we're told by the host to initialize new fully connected client(s)
//==============================================================================
bool BSession::createClient(const XNADDR& xnaddr, BSessionUser users[], BMachineID& machineID, const BProxyInfo& proxyInfo)
{
   IN_ADDR addr;
   int retval = XNetXnAddrToInAddr(&xnaddr, &mSessionXNKID, &addr);
   BDEBUG_ASSERT(retval == 0);
   if (retval != 0)
      return false;

   SOCKADDR_IN sockAddr;
   sockAddr.sin_addr = addr;
   sockAddr.sin_family = AF_INET;
   sockAddr.sin_port = mHostPort;

   BMachine* pMachine = NULL;

   BMachineID existingMachineID = mapAddrToMachineID(addr);
   if (existingMachineID != -1)
   {
      machineID = existingMachineID;

      pMachine = &mMachines[machineID];
   }
   else
   {
      // find an empty client slot
      machineID = findNextAvailableMachineID();
      if (machineID == -1)
         return false;

      // determine if this new client is using someone to act as a proxy to me
      // the BProxyInfo contains a list of the XNADDRs being proxied
      // if my XNADDR is in the list, then I'll want to set my proxy to use the same proxy as theirs.

      pMachine = &mMachines[machineID];
      pMachine->init(machineID, xnaddr, addr, mHostPort, BMachine::cStateConnected);

      mChannelTracker[machineID].init(machineID);

      // if we're in the list of XNADDRs in the proxy info, then lookup the machine for the proxy's XNADDR
      // and assign this machine to that proxy
      const BProxy* pProxy = proxyInfo.find(mLocalXnAddr);
      if (pProxy != NULL)
      {
         BMachine* pProxyMachine = getMachineByXnAddr(pProxy->getProxyXnAddr());
         if (pProxyMachine != NULL)
         {
            pMachine->setProxy(pProxyMachine->getSockAddr(), pProxyMachine->getPingAverage(), pProxyMachine->getXnAddr());
         }
      }

      BASSERTM(pMachine->mState == BMachine::cStateConnected, "The machine should be in the connected state");
      if (pMachine->mState < BMachine::cStateConnected)
         pMachine->mState = BMachine::cStateConnected;

      BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
      if (pMachine->isProxied())
         pPayload->init(sockAddr, xnaddr, pMachine->getProxySockAddr());
      else
         pPayload->init(sockAddr, xnaddr);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInit, 0, 0, pPayload);
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (users[i].mXuid == 0 || users[i].mClientID == -1)
         continue;

      // if this user has already been created, skip it
      if (users[i].mXuid == pMachine->mUsers[0].mXuid || users[i].mXuid == pMachine->mUsers[1].mXuid)
         continue;

      // add us to the machine's user list
      for (uint j=0; j < BMachine::cMaxUsers; ++j)
      {
         if (pMachine->mUsers[j].mXuid == INVALID_XUID)
         {
            pMachine->mUsers[j] = users[i];
            break;
         }
      }

      if (pMachine->mUsers[i].mXuid == mLocalUsers[0].mXuid)
         pMachine->mUsers[i].mControllerID = mLocalUsers[0].mControllerID;
      else if (pMachine->mUsers[i].mXuid == mLocalUsers[1].mXuid)
         pMachine->mUsers[i].mControllerID = mLocalUsers[1].mControllerID;

      int32 index = users[i].mClientID;

      BClientHolder& client = mClients[index];

      client.mMachineID = machineID;
      client.mState = BClientHolder::cStateConnected;

      if (mClients[index].mpClient != NULL)
      {
         BDEBUG_ASSERTM(false, "BClient already exists");
         HEAP_DELETE(mClients[index].mpClient, gNetworkHeap);
         mClients[index].mpClient = NULL;
      }

      BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);
      client.mpClient = pClient;

      pClient->init(index, users[i].mXuid, users[i].mGamertag, xnaddr, sockAddr);
      pClient->setConnected(true);

      // initialize a new client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(index, pMachine->mUsers[i].mControllerID, pMachine->mUsers[i].mXuid, sockAddr);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

      BNetIPString strAddr(addr);
      nlog(cSessionNL, "BSession::createClient from XNADDR xuid[%I64u], ip[%s:%d], name[%s], index[%d]", users[i].mXuid, strAddr.getPtr(), mHostPort, users[i].mGamertag.asNative(), index);
      addEvent(cEventClientConnect, index, (machineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));
   }

   return true;
}

//==============================================================================
//
//==============================================================================
bool BSession::createClient(const BXConnector& connector)
{
   BMachineID machineID = connector.getMachineID();

   BDEBUG_ASSERTM(machineID != -1, "Failed to find an available machine ID");
   if (machineID < 0)
      return false;

   BDEBUG_ASSERTM(machineID < XNetwork::cMaxClients, "Invalid machine ID");
   if (machineID >= XNetwork::cMaxClients)
      return false;

   BMachine& machine = mMachines[machineID];
   machine.init(machineID, connector.getXnAddr(), connector.getAddr(), mHostPort, BMachine::cStateConnected);

   if (connector.isProxied())
      machine.setProxy(connector.getProxyAddr(), connector.getProxyPing(), *connector.getProxyXnAddr());

   // initialize the channel tracker for use in syncing channel data when a client disconnects
   mChannelTracker[machineID].init(machineID);

   SOCKADDR_IN sockAddr;
   sockAddr.sin_addr = connector.getAddr();
   sockAddr.sin_family = AF_INET;
   sockAddr.sin_port = mHostPort;

   if (machine.getAddr().s_addr == mHostSecureINADDR.s_addr)
   {
      BDEBUG_ASSERTM(mHostMachineID == -1, "Host index already assigned");
      mHostMachineID = machineID;
   }

   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (connector.mUsers[i].mClientID == -1)
         continue;

      int32 index = connector.mUsers[i].mClientID;

      BClientHolder& client = mClients[index];

      if (mClients[index].mpClient != NULL)
      {
         BDEBUG_ASSERTM(false, "BClient already exists");
         HEAP_DELETE(mClients[index].mpClient, gNetworkHeap);
         mClients[index].mpClient = NULL;
      }

      client.mMachineID = machine.getID();
      client.mState = BClientHolder::cStateConnected;

      machine.mUsers[i] = connector.mUsers[i];

      BClient* pClient = HEAP_NEW(BClient, gNetworkHeap);
      client.mpClient = pClient;

      pClient->init(index, connector.mUsers[i].mXuid, connector.mUsers[i].mGamertag, connector.getXnAddr(), sockAddr);
      pClient->setConnected(true);

      // initialize a new client in the voice system
      BVoiceRequestPayload* pVoiceRequest = HEAP_NEW(BVoiceRequestPayload, gNetworkHeap);
      pVoiceRequest->init(index, connector.mUsers[i].mControllerID, connector.mUsers[i].mXuid, sockAddr);
      gEventDispatcher.send(mConnectionEventHandle, mVoiceEventHandle, cNetEventVoiceInitClient, 0, 0, pVoiceRequest);

      BNetIPString strAddr(connector.getAddr());
      BNetIPString strProxyAddr(connector.getProxyAddr());

      nlog(cSessionNL, "BSession::createClient from BXConnector xuid[%I64u], ip[%s:%d], proxy[%s], name[%s], index[%d]",
         connector.mUsers[i].mXuid,
         strAddr.getPtr(), connector.getPort(),
         strProxyAddr.getPtr(),
         connector.mUsers[i].mGamertag.asNative(), index);

      addEvent(cEventClientConnect, index, (machineID << 16 & 0x00FF0000) | (mLocalMachineID << 8 & 0x0000FF00) | (mHostMachineID & 0xFF));
   }

   BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
   if (connector.isProxied())
      pPayload->init(connector.getAddr(), connector.getPort(), connector.getXnAddr(), connector.getProxyAddr());
   else
      pPayload->init(connector.getAddr(), connector.getPort(), connector.getXnAddr());

   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInit, 0, 0, pPayload);

   return true;
}

//==============================================================================
//
//==============================================================================
void BSession::kickClient(long fromIndex, const void* pData, const DWORD size)
{
   BTypedMessageDataPacket packet(BPacketType::cKickPacket);
   packet.deserializeFrom(pData, size);

   long clientIndex = packet.mMessage;
   nlog(cSessionNL, "BSession::kickClient -- received Kick client packet from machineID[%ld] for clientID[%ld]", fromIndex, clientIndex);

   if (clientIndex < 0 || clientIndex >= XNetwork::cMaxClients || mClients[clientIndex].mpClient == NULL)
   {
      nlog(cSessionNL, "BSession::kickClient -- I don't have that client yet");
      return;
   }

   bool isLocal = false;

   if (mLocalMachineID != -1)
      isLocal = (mMachines[mLocalMachineID].mUsers[0].mClientID == clientIndex || mMachines[mLocalMachineID].mUsers[1].mClientID == clientIndex);

   // if this message is from the host
   if (fromIndex == mHostMachineID)
   {
      nlog(cSessionNL, "BSession::kickClient -- from the host");
      // and I am the one to get kicked, disconnect myself
      if (isLocal)
      {
         nlog(cSessionNL, "BSession::kickClient -- it's me. call disconnect");
         disconnect(cHostDecision);
      }
      // otherwise if the client is connecting or connected, note the client that was kicked
      // and wait for him to disconnect from us. NOTE: if he was in the connecting state
      // he will not go through the normal connect procedure when he connects to us.
      else if (mClients[clientIndex].mState <= BClientHolder::cStateConnected)
      {
         nlog(cSessionNL, "BSession::kickClient -- setting client state to kicked.");
         mClients[clientIndex].mState = BClientHolder::cStateKicked;
      }
   }
}

//==============================================================================
// Trying something different with the init routines
//
// Now the connecting client will need to attempt connections to all the
//    existing peers and then rely a response to the host with the success or
//    failure of each connection
//
// The host will then verify the init peers response and make a final
//    determination on whether to accept this client into the session
//
// A failure does not necessarily mean the client will fail to join since
//    the failed peer may have disconnected from the session while we were
//    attempting to join
//==============================================================================
void BSession::handlePeersRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BAddressesPacket packet(BPacketType::cInitPeersRequestPacket);
   packet.deserializeFrom(pData, size);

   nlog(cSessionNL, "BSession::handlePeersRequest -- got cInitPeersPacket");

   mRequirePeersResponse = true;

   for (uint i=0; i < packet.mAmount; i++)
   {
      const XNADDR& xnaddr = packet.mXnAddrs[i];

      BNetIPString strXnAddr(xnaddr);

      if (CompareXnAddr(xnaddr, mLocalXnAddr))
      {
         nlog(cSessionNL, "BSession::handlePeersRequest -- received my information - skipping - clientID(1)[%d], clientID(2)[%d] xnaddr[%s]",
            packet.mUsers[i][0].mClientID, packet.mUsers[i][1].mClientID,
            strXnAddr.getPtr());
         continue;
      }

      nlog(cSessionNL, "BSession::handlePeersRequest -- connecting to clientID(1)[%d], clientID(2)[%d] xnaddr[%s]",
         packet.mUsers[i][0].mClientID, packet.mUsers[i][1].mClientID,
         strXnAddr.getPtr());

      // another problem I need to solve is that someone could have left the session after I complete this process
      // so when I receive a JOIN OK response, I'm currently assuming that everyone in my BXConnector list
      // is a client in the session and I'm going to convert then to BClient/BClientHolders
      //
      // the host does not currently send a revised list, should it?
      //
      // could I have the process complete and then when I attempt to send to now missing clients
      // I will receive an error and disconnect them?
      //
      // I still run the risk of adding them all in, they all disconnect and a full group of new
      // clients join the fray
      //
      // or is that a risk?  I mean, if a full group of new clients connected, I'd be busy
      // sending my "hello" packet to the other clients in the session
      //
      // should I rely on this packet to init the xuids/gamertags of the other peers?
      // how else would I receive the data?  The other clients would have to send it to me, but that seems clunky/laggy
      // if the server gives me the one big chunk at start, then it's up to me to tell the other clients that
      // I've been accepted into the fold and, oh btw, here's my xuid/gamertag, basically the new "hello" packet
      // but only newly connecting clients send it
      //
      // when the other clients receive that hello packet, they can add me into their BClient/BClientHolder/connections list

      // if I'm in here a second (or more) time, that means that someone else has connected to
      // the host while we were busy
      //
      // iterate the existing BXConnectors and add the missing ones
      for (int j=mXConnectors.getSize()-1; j >= 0; --j)
      {
         const BXConnector& c = mXConnectors[j];
         if (CompareXnAddr(c.getXnAddr(), xnaddr))
            break;
      }

      if (j < 0)
      {
         // so I don't really want to create a BXConnection, I just want to perform an inaddr lookup
         // and then start an XNetConnect process and check the status of the connection every so often
         // until we timeout or error out
         //
         BXConnector connector;

         connector.init(packet.mMachineIDs[i], xnaddr, addr.sin_port, mSessionXNKID, packet.mUsers[i]);

         // start our connect process
         // do not check for errors here so we can unify our error handling in elsewhere
         connector.connect();

         mXConnectors.add(connector);
      }

      // set our peer timer
      if (!mPeerTimerSet)
      {
         mPeerTimerSet = true;
         BEvent handleEvent;
         handleEvent.clear();
         handleEvent.mFromHandle = mEventHandle;
         handleEvent.mToHandle = mEventHandle;
         handleEvent.mEventClass = cSessionEventInitPeers;
         gEventDispatcher.registerHandleWithEvent(mPeerTimer.getHandle(), handleEvent);

         mPeerTimer.set(500);
      }
   }
}

//==============================================================================
// The client has issued a response to the host stating which peers they were
//    able to connect to
//==============================================================================
void BSession::handlePeersResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   // only the host responds to peer response packets
   BASSERT(isHosted());
   if (!isHosted())
      return;

   BInitPeersResponsePacket packet;
   packet.deserializeFrom(pData, size);

   nlog(cSessionNL, "BSession::handlePeersResponse -- got cInitPeersResponsePacket");

   // in regards to BClientHolder/BClient
   // the host will always be at index 0
   // if I'm attempting to connect to the host, the host will be at index 0
   // my loopback will be at index 1
   // I need to make sure that when I'm looping I check for loopback
   //
   // if a client issues a join request and there are no other clients connected
   // I can issue an ok join response immediately
   //
   // that means I need to create a BClient/BClientHolder right away
   // and send an event to create a BXConnection that is not pending
   //

   // prepare another init peers request packet in case we have to send one
   BAddressesPacket request(BPacketType::cInitPeersRequestPacket);

   // first let's make sure we still have an available client slot before seeing if
   // they failed to connect to any of our peers
   //
   // if we don't have an available client slot then everything else is moot
   int32 index1, index2;

   findNextAvailableClientID(index1, index2);

   int32 userCount = 0;
   for (uint i=0; i < BMachine::cMaxUsers; ++i)
   {
      if (packet.mUsers[i].mXuid != 0)
         userCount++;
   }

   if (index1 == -1 || (userCount == 2 && index2 == -1))
   {
      // you missed your chance, time for the rejection notice
      // send a join OK response
      BSessionConnectorPacket responsePacket(BPacketType::cJoinResponsePacket, cResultRejectFull);

      HRESULT hr = SendPacketToAddr(addr, responsePacket);

      BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handlePeersResponse : Failed to send response packet");
      nlog(cSessionNL, "BSession::handlePeersResponse : send cJoinResponsePacket : cResultRejectFull for user1[%s], user2[%s] hr[%x]", packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative(), hr);

      // do not attempt to process the rest of the peers response.
      return;
   }

   BJoinReasonCode result = cResultJoinOk;

   if (mMaxClientAmount - getConnectedClientAmount() < 1)
   {
      result = cResultRejectFull;
   }
   else
   {
      //Check the callback to see if its ok for them to join
      if (!mpClientConnector)
      {
         nlog(cSessionNL, "BSession::handlePeersResponse -- Failed - I have no client connector");
         result = cResultRejectUnknown;
      }  
      else
      {
         mpClientConnector->sessionConnectionRequest(packet.mUsers, &result);
      }
   }

   if (result != cResultJoinOk)
   {
      BSessionConnectorPacket responsePacket(BPacketType::cJoinResponsePacket, result);

      HRESULT hr = SendPacketToAddr(addr, responsePacket);

      BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handlePeersResponse : Failed to send response packet");
      nlog(cSessionNL, "BSession::handlePeersResponse : send cJoinResponsePacket : result[%d] for user1[%s], user2[%s] hr[%x]", result, packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative(), hr);
      return;
   }

   // iterate over our currently connected clients, compare the XNADDR
   // if the client is still connected and the status is false
   // then we need to reject this new client letting them know
   //
   // if we have a new client that is not part of this response, then
   // we need to send another init peers request packet to the client
   // and have them attempt even more connections
   //
   // the new request may not contain all the same clients if some have disconnected
   //
   // the destination client will need to drop/add peers accordingly
   //
   // otherwise, if everything matches up, we can initiate a new connection for
   // this client and send them a join response OK

   bool peerFailure = false;
   bool sendRequest = false;

   for (uint i=0; i < XNetwork::cMaxClients && !peerFailure; ++i)
   {
      const BMachine& machine = mMachines[i];
      if (machine.mState == BMachine::cStateConnected)
      {
         const XNADDR& xnaddr1 = machine.mXnAddr;

         bool found = false;

         for (uint j=0; j < packet.mCount && !peerFailure && !found; ++j)
         {
            const XNADDR& xnaddr2 = packet.mPeers[j];
            bool status = ((packet.mStatus >> j & 1) == 1);

            if (CompareXnAddr(xnaddr1, xnaddr2))
            {
               found = true;
               if (!status)
                  peerFailure = true;
            }
         }

         // send a new request if we failed to find the xnaddr
         sendRequest = (sendRequest || !found);

         // if we failed to find the xnaddr, then we need to issue another
         // init peers request packet to the client
         //
         // the init peers request needs to contain a list of ALL the connected clients
         // even if the remote client has successfully connected to one of them
         //
         // the remote client will verify that they're still connected with each client, or initiate new connections
         //
         request.addAddress(machine.mID, machine.mXnAddr, machine.mUsers);
      }
   }

   if (peerFailure)
   {
      // there is a connected client that the requesting client failed to connect with
      // so now we need to inform the client with a NO join response
      BSessionConnectorPacket responsePacket(BPacketType::cJoinResponsePacket, cResultRejectConnectFailure);

      SendPacketToAddr(addr, responsePacket);
   }
   else if (sendRequest)
   {
      // the connecting client is not done yet, more peers have come online
      SendPacketToAddr(addr, request);
   }
   else
   {
      // I need to create a BClient/BClientHolder and add them to the non-pending BXConnection list
      //
      // be aware, that after calling create client, the client could start receiving broadcast
      // communications from the session before receiving the join response packet
      //
      // the remote client will need to queue up the incoming communications for later processing
      // when the OK response is received
      //
      // this is currently handled via checking isConnected() and if not, queuing the data
      BMachineID machineID = -1;
      if (!createClient(addr, packet.mUsers, machineID))
      {
         nlog(cSessionNL, "BSession::handlePeersResponse : failed to create client(s) for user1[%s], user2[%s]", packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative());
         return;
      }

      const BMachine& machine = mMachines[machineID];

      // send a join OK response
      BSessionConnectorPacket responsePacket(BPacketType::cJoinResponsePacket, cResultJoinOk, machineID, machine.mUsers);

      HRESULT hr = SendPacketToAddr(addr, responsePacket);

      // also need to broadcast an init peer packet
      //
      // do not trust the packet.mXnAddr value, use the one that was determined during our createClient() method
      // the value is available from the BClientHolder or BClient

      // need to adjust BInitPeersResponsePacket to account for potential proxies
      // if this new client is having to relay traffic via another client then we want to know that
      // so we can inform the other client of this new route and adjust the BXConnection with the proxy information
      //
      // this handles the case of initial connections, but what about errors that happen after connections have
      // been established?
      //
      // if I need to handle the latter case then why not just insure that the latter case is sufficiently robust and
      // have a single code path for these types of proxy changes?
      //
      // the latter case kicks in when I attempt to send to an invalid connection which raises a socket error
      // before I'd simply begin client disconnect procedures but now I need to intercept the error and issue
      // proxy requests for the connection

      // 

      BSessionConnectorPacket initPeerPacket(BPacketType::cInitPeerPacket, machine.mXnAddr, machineID, machine.mUsers, packet.mProxy);

      hr = SendPacket(initPeerPacket);
      BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handlePeersResponse : Failed to send cInitPeerPacket");
      nlog(cSessionNL, "BSession::handlePeersResponse : send cInitPeerPacket for user1[%s], user2[%s] [%x]", packet.mUsers[0].mGamertag.asNative(), packet.mUsers[1].mGamertag.asNative(), hr);
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleProxyRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   // since this is a broadcast request, I want to ignore them from me
   if (addr.sin_addr.s_addr == INADDR_LOOPBACK)
      return;

   BProxyPacket packet;
   packet.deserializeFrom(pData, size);

#ifndef BUILD_FINAL
   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleProxyRequestPacket -- got cProxyRequestPacket from[%s]", strAddr.getPtr());
#endif

   // the packet will contain all the peers that the requester had trouble connecting with
   //
   // need to loop all of the connections that I currently have and determine if we would
   // be able to act as a proxy for any of the requested peers
   //
   // this should only compare proxy requests against connected clients

   // actually, this is too soon for sending this response to the client
   // what we need to do is issue cProxyPingPackets to the requested peers
   // to insure that they're still alive and then send a response to the client
   // with true/false indications for each requested peer

   for (uint i=0; i < packet.mCount; ++i)
   {
      const XNADDR xnaddr1 = packet.mPeers[i];

#ifndef BUILD_FINAL
      BNetIPString strXnAddr(xnaddr1);
      nlog(cSessionNL, "BSession::handleProxyRequestPacket -- peer[%s] uniqueID[%I64u]", strXnAddr.getPtr(), BSession::getUniqueID(xnaddr1));
#endif

      const BProxyRequest* pRequest = getProxyRequestByAddrAndXnAddr(addr, xnaddr1);
      if (pRequest != NULL)
      {
         // already have a proxy request from this client for the given xnaddr, now what?
         // why would the client send another request before I've given him a response?
         // ignore?
      }
      else
      {
         // I can do a quick lookup to insure that this peer is still connected to me
         // if not, then there's no reason to issue a ping
         const BMachine* pMachine = getMachineByXnAddr(xnaddr1);
         if (pMachine == NULL || pMachine->getState() != BMachine::cStateConnected || pMachine->isLocal() || pMachine->isProxied() || !pMachine->isEnabled())
         {
#ifndef BUILD_FINAL
            nlog(cSessionNL, "BSession::handleProxyRequestPacket -- no proxy available for [%s] uniqueID[%I64u] machine?[%d] state[%d] local[%d] isProxied[%d]",
               strXnAddr.getPtr(), BSession::getUniqueID(xnaddr1),
               (pMachine != NULL ? 1 : 0), (pMachine ? pMachine->getState() : 0), (pMachine ? pMachine->isLocal() : 0), (pMachine ? pMachine->isProxied() : 0));
#endif
            // we know we can't act as a proxy for this peer
            BProxyPacket response(BPacketType::cProxyResponsePacket);
            response.addPeer(xnaddr1, false);

            SendPacketToAddr(addr, response);
         }
         else
         {
#ifndef BUILD_FINAL
            BNetIPString strMachineAddr(pMachine->getSockAddr());
            nlog(cSessionNL, "BSession::handleProxyRequestPacket -- proxy potential, sending ping [%s] uniqueID[%I64u]", strMachineAddr.getPtr(), BSession::getUniqueID(xnaddr1));
#endif

            // create a new BProxyRequest and send a ping to the requested client
            BProxyRequest* pNewRequest = HEAP_NEW(BProxyRequest, gNetworkHeap);
            pNewRequest->init(addr, pMachine->getSockAddr(), xnaddr1, mProxyPingTimeout);

            mProxyRequests.add(pNewRequest);

            if (!mProxyRequestTimerSet)
            {
               mProxyRequestTimerSet = true;

               BEvent handleEvent;
               handleEvent.clear();
               handleEvent.mFromHandle = mEventHandle;
               handleEvent.mToHandle = mEventHandle;
               handleEvent.mEventClass = cSessionEventProxyRequestTimer;
               gEventDispatcher.registerHandleWithEvent(mProxyRequestTimer.getHandle(), handleEvent);

               mProxyRequestTimer.set(1000);
            }
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleProxyResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BProxyPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleProxyResponse -- got cProxyResponsePacket from[%s]", strAddr.getPtr());

   // need to find the XNADDR that matches the given SOCKADDR_IN
   XNADDR fromXnAddr;
   uint64 fromUniqueID = 0;
   uint fromAvgPing = 0;

   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      if (mMachines[i].getAddr().s_addr == addr.sin_addr.s_addr)
      {
         fromXnAddr = mMachines[i].getXnAddr();
         fromUniqueID = BSession::getUniqueID(fromXnAddr);
         fromAvgPing = mMachines[i].getPingAverage();
         break;
      }
   }
   if (fromUniqueID == 0 && mpHostConnector != NULL && mpHostConnector->getAddr().s_addr == addr.sin_addr.s_addr)
   {
      fromXnAddr = mpHostConnector->getXnAddr();
      fromUniqueID = BSession::getUniqueID(fromXnAddr);
   }
   if (fromUniqueID == 0)
   {
      for (uint i=0; i < mXConnectors.getSize(); ++i)
      {
         if (mXConnectors[i].getAddr().s_addr == addr.sin_addr.s_addr)
         {
            fromXnAddr = mXConnectors[i].getXnAddr();
            fromUniqueID = BSession::getUniqueID(fromXnAddr);
            break;
         }
      }
   }

   // this packet contains the peers that are available to be proxied from the given addr

   // I may receive multiple of these packets depending on which clients are available to act as proxies
   // for all of my requests

   // at this point, I should still have all my BXConnectors intact and can determine which
   // ones failed to connect vs. the ones that the responding client can proxy for us

   // should I loop through my BXConnectors and if I find a connector that has failed to connect
   // then loop on the proxy packet to find an appropriate match?

   // do I want to track which peers I requested proxies for?
   // should I create BProxyRequests? or simply mark the the BXConnector as someone I issued a request for?
   // BProxyRequest will be reserved for incoming proxy requests and not for my own requests

   // loop the packet xnaddrs and attempt to find the corresponding BXConnectors

   for (uint i=0; i < packet.mCount; ++i)
   {
      const XNADDR& xnaddr = packet.mPeers[i];

      bool status = (((packet.mStatus >> i) & 1) == 1);

#ifndef BUILD_FINAL
      BNetIPString strXnAddr(xnaddr);
      nlog(cSessionNL, "BSession::handleProxyResponse -- from[%s], XNADDR[%d:%s] uniqueID[%I64u] status[%d]",
         strAddr.getPtr(), i, strXnAddr.getPtr(), BSession::getUniqueID(xnaddr), status);
#endif

      if (!status)
         continue;

      bool removeProxyPending = false;

      uint avgPing = packet.mPings[i] & 0xFFFF;

      uint count = mXConnectors.getSize();
      for (uint j=0; j < count; ++j)
      {
         BXConnector& connector = mXConnectors[j];

         // make sure we're not attempting to use the requested peer as a proxy for themselves
         if (connector.getAddr().s_addr != addr.sin_addr.s_addr && CompareXnAddr(connector.getXnAddr(), xnaddr))
         {
            removeProxyPending = true;

            // found the BXConnector for this proxy response
            //
            // set the proxy address for the BXConnector to be used later when we end up creating the actual connection
            // provided that the host allows to join the session
            //
            // what about situations where the game is in-progress and I need to set the proxy address on an existing connection?
            // isConnected() will be false during session setup, so use that as the check
            //
            // in order to set a proxy address on a connection, I need to issue an event to the connection thread
            //
            connector.setProxy(addr, fromAvgPing + avgPing, fromXnAddr);

            //BNetIPString strConnector(connector.getAddr());

            //nlog(cSessionNL, "BSession::handleProxyResponse -- BXConnector::setProxy ip[%s] proxy[%s] uniqueID[%I64u]",
            //   strConnector.getPtr(), strAddr.getPtr(), BSession::getUniqueID(connector.getXnAddr()));
         }
      }

      BMachine* pMachine = getMachineByXnAddr(xnaddr);
      if (pMachine != NULL && pMachine->getAddr().s_addr != addr.sin_addr.s_addr)
      {
         removeProxyPending = true;

         pMachine->setProxy(addr, fromAvgPing + avgPing, fromXnAddr);

         //BNetIPString strMachine(pMachine->getAddr());

         //nlog(cSessionNL, "BSession::handleProxyResponse -- BMachine::setProxy ip[%s] proxy[%s] uniqueID[%I64u]",
         //   strMachine.getPtr(), strAddr.getPtr(), BSession::getUniqueID(pMachine->getXnAddr()));
      }

      // find the pending xnaddr
      if (removeProxyPending)
      {
         for (int j=mProxyPending.getSize()-1; j >= 0; --j)
         {
            if (CompareXnAddr(xnaddr, mProxyPending[j]))
            {
               mProxyPending.removeIndex(j);
            }
         }
      }
   }

   // if I'm not connected and I'm not disconnecting, then I'm attempting to connect to a new session and
   //
   // I need to inform the host on the status of all my peer connections
   //
   // but the problem here is that I'm actually waiting to hear back from potentially multiple clients
   // so the question is how long do I wait before taking what I have and moving forward?
   //
   // I don't know how many clients that I ended up sending proxy requests to so I don't know how many
   // I'm waiting on to respond... could I add that type of tracking?
   //
   // that's sort of what the BProxyRequest stuff is for but I was using that for strictly request stuff
   // could I also use it to track outstanding requests?  That still doesn't buy us anything because
   // this response would satisfy one of the requests but other peers may also respond....
   //
   // what I really need is some sort of timeout that I could wait on before sending the final init peers response
   if (mProxyPending.getSize() == 0)
   {
      if (mProxyResponseTimerSet)
      {
         mProxyResponseTimerSet = false;
         mProxyResponseTimer.cancel();
         gEventDispatcher.deregisterHandle(mProxyResponseTimer.getHandle(), cThreadIndexSim);
      }

      // if I'm attempting to connect to the host, then issue our init peers response
      if (!isConnected() && !isDisconnecting())
      {
         if (!mRequirePeersResponse)
            return;

         mRequirePeersResponse = false;

         // create an init peers packet
         SOCKADDR_IN hostAddr;
         hostAddr.sin_family = AF_INET;
         hostAddr.sin_addr = mHostSecureINADDR;
         hostAddr.sin_port = mHostPort;

         // loop the connectors again and fill out a peer response packet with their values
         // what will the host key off of when looking up those peers to insure we're good?
         BInitPeersResponsePacket response;

         uint count = mXConnectors.getSize();
         for (uint i=0; i < count; ++i)
         {
            BXConnector& connector = mXConnectors[i];
            // if we connected or we successfully proxied our connection, then we can let the host know that we succeeded
            response.addPeer(connector.getXnAddr(), (connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED || connector.isProxied()), connector.getProxyXnAddr());
         }

         response.setLocalInfo(mLocalXnAddr, mLocalUsers);

         SendPacketToAddr(hostAddr, response);
      }
      else if (!isDisconnecting())
      {
         // otherwise, this proxy process happened during the game and I need to either initialize the proxies
         // on our connections or disconnect accordingly
         //
         // if I have a disabled BMachine but no proxy, then I need to initiate a disconnect
         // otherwise, if I have BMachine that's being proxied, I can setup that proxy now
         for (uint i=0; i < XNetwork::cMaxClients; ++i)
         {
            BMachine& machine = mMachines[i];
            if (machine.getID() != BMachine::cInvalidMachineID && !machine.isEnabled())
            {
               // because BMachine::setProxy does not re-enable the BMachine instance, we need to do that here
               // if the machine has been proxied
               //
               // if we set the proxy on the BXConnector, then we're in the process of joining a new session
               // and when it comes time to initialize a new BMachine from the BXConnector, I need to
               // set the proxy and enable the BMachine
               if (!machine.isProxied())
               {
                  socketDisconnect(machine.getID(), WSAEDISCON);
               }
               else if (machine.isProxied())
               {
                  // enable the BMachine as well, setProxy does not do this for us because
                  // I need some way of filtering out BMachines that have already gone through this process
                  machine.enable();

                  BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
                  pPayload->init(machine.getSockAddr(), machine.getXnAddr(), addr);

                  // if the machine is proxied, I need to issue an event with the new proxy addr
                  // so the BXConnection can update it's proxy information
                  gEventDispatcher.send(cInvalidEventReceiverHandle, mConnectionEventHandle, cSessionEventInitProxy, 0, 0, pPayload);
               }
            }
         }
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BSession::handleProxyPing(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BProxyPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleProxyPing -- got cProxyPingPacket from[%s] with nonce[%X]", strAddr.getPtr(), packet.getNonce());

   // echo back a pong packet

   BProxyPacket response(BPacketType::cProxyPongPacket);
   response.setNonce(packet.getNonce());

   SendPacketToAddr(addr, response);
}

//==============================================================================
// 
//==============================================================================
void BSession::handleProxyPong(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BProxyPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleProxyPong -- got cProxyPongPacket from[%s] with nonce[%X]", strAddr.getPtr(), packet.getNonce());

   // lookup our proxy request(s) for this addr
   uint count = mProxyRequests.getSize();
   for (uint i=0; i < count; ++i)
   {
      BProxyRequest* pRequest = mProxyRequests[i];
      if (pRequest->getToAddr().s_addr == addr.sin_addr.s_addr && pRequest->getNonce() == packet.getNonce())
      {
         BNetIPString strXnAddr(pRequest->getToXnAddr());

         // found a proxy request destined for this addr, respond to the requesting client
         pRequest->setState(BProxyRequest::cStateComplete);

         BProxyPacket response(BPacketType::cProxyResponsePacket);

         const BMachine* pMachine = getMachineByAddr(addr.sin_addr);
         if (pMachine == NULL || pMachine->getState() != BMachine::cStateConnected || pMachine->isProxied())
         {
            nlog(cSessionNL, "BSession::handleProxyPong -- found BProxyRequest for [%s] with nonce[%X] xnaddr[%s], missing BMachine or not connected", strAddr.getPtr(), packet.getNonce(), strXnAddr.getPtr());
            response.addPeer(pRequest->getToXnAddr(), false);
         }
         else
         {
            nlog(cSessionNL, "BSession::handleProxyPong -- found BProxyRequest for [%s] with nonce[%X] xnaddr[%s], able to proxy", strAddr.getPtr(), packet.getNonce(), strXnAddr.getPtr());
            response.addPeer(pRequest->getToXnAddr(), true, pMachine->getPingAverage(), pMachine->getPingStdDev());
         }

         BNetIPString strFromAddr(pRequest->getFromSockAddr());
         nlog(cSessionNL, "BSession::handleProxyPong -- sending cProxyResponsePacket to [%s]", strFromAddr.getPtr());

         SendPacketToAddr(pRequest->getFromSockAddr(), response);
      }
   }
}

//==============================================================================
// 
//==============================================================================
uint calcPacketDistance(uint id1, uint id2)
{
   if (id2 > id1)
      return id2 - id1;
   else
      return (XNetwork::cMaxPacketID - id1) + id2;
}

//==============================================================================
//
//==============================================================================
void BSession::handleDisconnectRequest(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDisconnectPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleDisconnectRequest -- got cDisconnectRequest from[%s]", strAddr.getPtr());

   HRESULT hr;

   const BChannelTracker& remote = packet.getTracker();

   // only process disconnect requests from other peers
   if (addr.sin_addr.s_addr != INADDR_LOOPBACK)
   {
      // lookup the channel info for the packet's machine ID and compare what we have with what is in the packet
      BChannelTracker& local = mChannelTracker[remote.getMachineID()];

      BHandle hItem;
      BChannelData* pChannelData = mChannelDataList.getHead(hItem);
      while (pChannelData)
      {
         uint channelID = pChannelData->getChannelID();

         const BChannelInfo* pLocal = local.getChannelInfo(channelID);
         const BChannelInfo* pRemote = remote.getChannelInfo(channelID);

         if (pLocal == NULL || pRemote == NULL)
         {
            pChannelData = mChannelDataList.getNext(hItem);
            continue;
         }

         if (pRemote->getPacketID() == pLocal->getPacketID())
         {
            pChannelData = mChannelDataList.getNext(hItem);
            continue;
         }

         uint distance = calcPacketDistance(pRemote->getPacketID(), pLocal->getPacketID());

         // insure that we have data that needs to be synced
         if (distance == 0 || distance >= XNetwork::cMaxPacketDistance)
         {
            pChannelData = mChannelDataList.getNext(hItem);
            continue;
         }

         distance = calcPacketDistance(pRemote->getPacketID(), pChannelData->getPacketID());

         // if this channel data matches the machineID, channelID and the packetID is valid, send it
         if (pChannelData->getMachineID() == remote.getMachineID() &&
             pChannelData->getChannelID() == pRemote->getChannelID() &&
             distance > 0 && distance < XNetwork::cMaxPacketDistance)
         {
            // if the machine, channel and packet IDs match, then we need to send this to the requesting machine
            BDisconnectSyncPacket sync;
            sync.init(pChannelData->getMachineID(), pChannelData->getSize(), pChannelData->getData());
            hr = SendPacketToAddr(addr, sync);
            BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handleDisconnectRequest : Failed to send cDisconnectSync");
            nlog(cSessionNL, "BSession::handleDisconnectRequest : send cDisconnectSync for machineID[%d] channelID[%d] packetID[%d] remotePacketID[%d] localPacketID[%d] to [%s] hr[%x]", pChannelData->getMachineID(), pChannelData->getChannelID(), pChannelData->getPacketID(), pRemote->getPacketID(), pLocal->getPacketID(), strAddr.getPtr(), hr);
         }

         pChannelData = mChannelDataList.getNext(hItem);
      }
   }

   BDisconnectPacket response(BPacketType::cDisconnectResponse);
   response.init(remote);
   hr = SendPacketToAddr(addr, response);
   BDEBUG_ASSERTM(SUCCEEDED(hr), "BSession::handleDisconnectRequest : Failed to send cDisconnectResponse");
   nlog(cSessionNL, "BSession::handleDisconnectRequest : send cDisconnectResponse for machineID[%d] to [%s] [%x]", remote.getMachineID(), strAddr.getPtr(), hr);
}

//==============================================================================
//
//==============================================================================
void BSession::handleDisconnectSync(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDisconnectSyncPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleDisconnectSync -- got cDisconnectSync from[%s]", strAddr.getPtr());

   // the disconnect sync packet will contain the missing channel data we require
   //
   // need to peek at the channel data and verify that the packet ID is one that we can use
   //
   long channelID = BChannelPacket::getChannel(packet.getData());
   uint packetID = BChannelPacket::getPacketID(packet.getData());

   BChannelTracker& tracker = mChannelTracker[packet.getMachineID()];

   uint currentPacketID = tracker.getPacketID(channelID);

   // I should not be able to receive packets in the future since I'm requesting the origination point for packetIDs
   // unless of course we overrun the cache and the other client has no choice but to send what it has in the future
   //
   // in this case should I try and cache those future packets for later processing?  What are the odds that one
   // client might be able to fill in the gap given the size of the cache?
   //
   // this is a valid packet ID, question is, are we next in the sequence?
   if (currentPacketID == 0 || ((currentPacketID + 1) == packetID) || (currentPacketID == XNetwork::cMaxPacketID && packetID == 1))
   {
      // this packet is next in our sequence, add the event now
      addEvent(cEventChannelData, packet.getMachineID(), packet.getSize(), packet.getData());
   }
}

//==============================================================================
//
//==============================================================================
void BSession::handleDisconnectResponse(const uchar* pData, const uint size, const SOCKADDR_IN& addr)
{
   BDisconnectPacket packet;
   packet.deserializeFrom(pData, size);

   BNetIPString strAddr(addr);
   nlog(cSessionNL, "BSession::handleDisconnectResponse -- got cDisconnectResponse from[%s]", strAddr.getPtr());

   // the disconnect response should indicate which machine ID request it is linked to

   // lookup the machine ID of the person who sent this packet
   BMachineID fromMachineID = mapAddrToMachineID(addr);

   BMachineID forMachineID = packet.getTracker().getMachineID();

   checkDisconnect(fromMachineID, forMachineID);

   // need to flush out any channel data we may have received
   processEvents();
}

//==============================================================================
//
//==============================================================================
void BSession::checkDisconnect(BMachineID fromMachineID, BMachineID forMachineID)
{
   // lookup the disconnect entry for the machine ID that is disconnecting
   BDisconnectEntry* pEntry = mDisconnectTracker.get(forMachineID);
   if (pEntry == NULL)
      return;

   // set our sender's state to done
   pEntry->set(fromMachineID, BDisconnectEntry::cStateDone);

   // we may/may not be waiting on other machines to report their responses

   // need to verify that we're not waiting on more disconnect responses

   // do I need to check our list of BMachines or can I simply loop the machine IDs in the BDisconnectEntry
   // to insure that nobody is still set to cStateSent?
   //
   // This state check shouldn't matter since the time sync system verifies that all connected peers
   // have reported the disconnect
   //for (uint i=0; i < XNetwork::cMaxClients; ++i)
   //{
   //   if (pEntry->getState(i) == BDisconnectEntry::cStateSent)
   //      return;
   //}

   BASSERTM(mpTimeSync, "Missing Time Sync system");

   // otherwise, all entries are either None or Done and I can continue with the disconnect procedures
   // NOTE: this same check needs to happen in socketDisconnect in case the last person I'm waiting on is the one disconnecting
   if (mpTimeSync)
   {
      //mpTimeSync->startClientDisconnect(forMachineID);

      // clientIndex is the machine ID that requested the disconnect
      // packet.mClientID is the machine ID of the peer that's disconnecting
      // the last parameter is always true right now, the old "false" functionality has been deprecated
      //clientDisconnectReported(static_cast<uint32>(clientIndex), packet.mClientID, true);
      //checkForClientDisconnect(packet.mClientID);

      mpTimeSync->clientDisconnectReported(fromMachineID, forMachineID);
   }
}

//==============================================================================
//
//==============================================================================
void BSession::connectionRejected(const void *data, const DWORD size)
{
   BTypedMessageDataPacket packet(BPacketType::cConnectionRejectedPacket);
   packet.deserializeFrom(data, size);

   nlog(cSessionNL, "BSession::connectionRejected -- reason[%d]", packet.mMessage);

   disconnect(packet.mMessage);
}

//==============================================================================
//
//==============================================================================
void BSession::processEvents()
{
   mEventProcessing = true;

   BSessionEvent* pEvent = mEventList.removeHead();
   while (pEvent)
   {
      mObserverList.processSessionEvent(pEvent);

      releaseEvent(pEvent);

      pEvent = mEventList.removeHead();
   }

   mEventProcessing = false;
}

//==============================================================================
//
//==============================================================================
void BSession::addEvent(uint32 eventID, uint32 data1, uint32 data2, const void* pData, uint64 data3, int32 data4)
{
   BSessionEvent* pEvent = NULL;

   //Extra logging goodness
   if (eventID==cEventClientConnect)
   {
      BClient* pClient = mClients[data1].mpClient;
      BASSERT(pClient);
      nlog(cSessionNL, "BSession::addEvent cEventClientConnect - Holder index[%ld], ClientID[%ld], xuid[%I64u], gamertag[%s]",
         data1, pClient->getID(), pClient->getXuid(), pClient->getGamertag().getPtr());
   }

   // cEventChannelData
   // data1 == BMachineID
   // data2 == size of data
   // pData == data
   //
   // for channel data, peek into the data to find the channel ID and packet ID
   // if the packet ID is > 0, then we need to archive the packet for a potential future disconnect request
   //
   // when archiving the packet, we need to include the machine ID and data
   // so when we go to resend the channel data to someone else, they can replay the channel event
   // with the correct machine ID
   //
   // need to prevent from double submitting events in case I receive events from a disconnect sync stage
   // that may have already been in the pipe -- only channel data should concern us
   //
   if ((eventID == cEventClientData) ||
       (eventID == cEventChannelData))
   {
      if (eventID == cEventChannelData && data2 > 0)
      {
         uint packetID = BChannelPacket::getPacketID(pData);
         if (packetID > 0)
         {
            if (data1 < 0 || data1 >= XNetwork::cMaxClients)
               return;

            long channelID = BChannelPacket::getChannel(pData);

            uint currentPacketID = mChannelTracker[data1].getPacketID(channelID);

            // will it be possible to receive an event in the future? no, because I've requested
            // the other clients to send me my missing data, so if anything, I may have already filled
            // in some of the gaps, so I need to make sure I'm not processing duplicates accidentally
            if (currentPacketID != 0 &&
                  (currentPacketID == packetID ||
                  (currentPacketID == XNetwork::cMaxPacketID && packetID != 1) ||
                  (currentPacketID != XNetwork::cMaxPacketID && currentPacketID+1 != packetID)))
            {
               // this should insure that the event I'm attempting to add is the next valid packet ID for the channel
               return;
            }

            BChannelData* pChannelData;
            while (mChannelDataList.getSize() >= XNetwork::cChannelQueueSize)
            {
               pChannelData = mChannelDataList.removeHead();

               if (pChannelData != NULL)
                  BChannelData::releaseInstance(pChannelData);
            }

            long size = sizeof(BSessionEvent)+data2;
            pEvent = (BSessionEvent*)gNetworkHeap.New(size);
            pEvent->init(eventID, data1, data2, data3, data4, pData);
            pEvent->setAutoRelease(false);

            pChannelData = BChannelData::getInstance();
            pChannelData->init(data1, channelID, packetID, data2, pEvent);

            if (data1 < XNetwork::cMaxClients)
               mChannelTracker[data1].set(channelID, packetID);

            mChannelDataList.addToTail(pChannelData);
         }
      }

      if (pEvent == NULL)
      {
         long size = sizeof(BSessionEvent)+data2;
         pEvent = (BSessionEvent*)gNetworkHeap.New(size);
         pEvent->init(eventID, data1, data2, data3, data4, pData);
      }
   }
   else
   {
      // if we have more than 50 events already pending in our list then ignore pings
      if (eventID == cEventClientPing && mEventList.getSize() > 50)
         return;

      pEvent = mEventFreeList.acquire(true);
      pEvent->init(eventID, data1, data2, data3, data4);
   }

   mEventList.addToTail(pEvent);
}

//==============================================================================
//
//==============================================================================
void BSession::releaseEvent(BSessionEvent* pEvent)
{
   if ( (pEvent->mEventID == cEventClientData) ||
        (pEvent->mEventID == cEventChannelData) )
   {
      if (pEvent->getAutoRelease())
         HEAP_DELETE(pEvent, gNetworkHeap);
      else // if we're not set to auto release, then mark us as being processed so the channel data list knows to free us
         pEvent->setProcessed();
   }
   else
   {
      mEventFreeList.release(pEvent);
   }
}

//==============================================================================
BSessionBuffer* BSession::allocBuffer()
{
   BSessionBuffer* pBuf = mSessionBufferAllocator.alloc();
   if (pBuf != NULL)
   {
      pBuf->mReserved = 0;
      return pBuf;
   }

   //BASSERTM(pBuf, "Failed to allocate send buffer");

   pBuf = HEAP_NEW(BSessionBuffer, gNetworkHeap);
   pBuf->mReserved = 1;

   return pBuf;
}

//==============================================================================
// Only call this at shutdown time
//==============================================================================
void BSession::stopTimer()
{
   if (mConnectionTimerSet)
   {
      mConnectionTimer.cancel();
      mConnectionTimerSet = false;
      gEventDispatcher.deregisterHandle(mConnectionTimer.getHandle(), cThreadIndexSimHelper);
   }

   if (mPeerTimerSet)
   {
      mPeerTimer.cancel();
      mPeerTimerSet = false;
      gEventDispatcher.deregisterHandle(mPeerTimer.getHandle(), cThreadIndexSim);
   }

   if (mProxyRequestTimerSet)
   {
      mProxyRequestTimer.cancel();
      mProxyRequestTimerSet = false;
      gEventDispatcher.deregisterHandle(mProxyRequestTimer.getHandle(), cThreadIndexSim);
   }

   if (mProxyResponseTimerSet)
   {
      mProxyResponseTimer.cancel();
      mProxyResponseTimerSet = false;
      gEventDispatcher.deregisterHandle(mProxyResponseTimer.getHandle(), cThreadIndexSim);
   }

   for (int i=mProxyRequests.getSize()-1; i >= 0; --i)
   {
      BProxyRequest* pProxy = mProxyRequests[i];
      HEAP_DELETE(pProxy, gNetworkHeap);
   }
   mProxyRequests.clear();
}

//==============================================================================
// This should only be called from the connection handling thread (sim helper)
//==============================================================================
void BSession::deinitPeer(uint addr)
{
   for (int i=mXConnections.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnections[i];
      if (pConn == NULL)
      {
         mXConnections.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr)
      {
         pConn->deinit();
         HEAP_DELETE(pConn, gNetworkHeap);
         mXConnections.removeIndex(i);
         return;
      }
   }

   for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnectionsPending[i];
      if (pConn == NULL)
      {
         mXConnectionsPending.removeIndex(i, false);
      }
      else if (pConn->getAddr().sin_addr.s_addr == addr)
      {
         pConn->deinit();
         HEAP_DELETE(pConn, gNetworkHeap);
         mXConnectionsPending.removeIndex(i);
         return;
      }
   }
}

//==============================================================================
// This should only be called from the connection handling thread (sim helper)
//==============================================================================
void BSession::disablePeer(uint addr)
{
   // disable this connection and any connections that are using this one as
   // their proxy
   for (int i=mXConnections.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnections[i];
      if (pConn == NULL)
      {
         mXConnections.removeIndex(i, false);
      }
      else
      {
         if (pConn->getAddr().sin_addr.s_addr == addr)
            pConn->disable();
         if (pConn->getProxyAddr().sin_addr.s_addr == addr)
            pConn->resetProxy();
      }
   }

   for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
   {
      BXConnection* pConn = mXConnectionsPending[i];
      if (pConn == NULL)
      {
         mXConnectionsPending.removeIndex(i, false);
      }
      else
      {
         if (pConn->getAddr().sin_addr.s_addr == addr)
            pConn->disable();
         if (pConn->getProxyAddr().sin_addr.s_addr == addr)
            pConn->resetProxy();
      }
   }
}

//==============================================================================
// 
//==============================================================================
bool BSession::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   // all these events should be coming from the network thread
   switch (event.mEventClass)
   {
      case cSessionEventTimer:
      {
         //nlogt(cTransportNL, "cSessionEventTimer -- from[%I64u] to[%I64u] index[%d]", event.mFromHandle, event.mToHandle, threadIndex);

         uint32 time = timeGetTime();

         // this is called in the context of the Sim Helper thread
         // we're meant to flush all the connections and issue pings
         for (int i=mXConnections.getSize()-1; i >=0; --i)
         {
            BXConnection* pConn = mXConnections[i];
            if (pConn == NULL)
            {
               mXConnections.removeIndex(i, false);
            }
            else if (pConn->isRegistered() && time - pConn->getLastRecvTime() > mConnectionTimeout)
            {
               pConn->unregister();
            }
            else
            {
               pConn->flush();
            }
         }
         // do I need to flush the pending connections as well?
         for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
         {
            BXConnection* pConn = mXConnectionsPending[i];
            if (pConn == NULL)
            {
               mXConnectionsPending.removeIndex(i, false);
            }
            else if (pConn->isRegistered() && time - pConn->getLastRecvTime() > mConnectionTimeout)
            {
               pConn->unregister();
            }
            else
            {
               pConn->flush();
            }
         }

         break;
      }

      case cSessionEventInit:
         {
            // create a new BXConnection for the given SOCKADDR
            // do I need anything other than the SOCKADDR?
            BSessionInitPayload* pPayload = reinterpret_cast<BSessionInitPayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;

            BNetIPString strAddr(pPayload->mAddr);
            BNetIPString strProxyAddr(pPayload->mProxyAddr);

            nlogt(cTransportNL, "cSessionEventInit ip[%s] proxy[%s]",
               strAddr.getPtr(), strProxyAddr.getPtr());

            // walk the pending connections list first
            // the connection process is that a client will connect to us
            // and send a join request packet
            // when we're receive data from an unknown client, we create a pending
            // connection to handle the reliable udp delivery aspects
            // once they get approved, we migrate their pending connection to the connections list
            // or we create a new one and add it to the connections list
            for (int i=mXConnectionsPending.getSize()-1; i >= 0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               BDEBUG_ASSERT(pConn);
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i);
               }
               else if (pConn->getAddr().sin_addr.s_addr == pPayload->mAddr.sin_addr.s_addr)
               {
                  if (pPayload->mProxied)
                  {
                     pConn->disable();
                     pConn->setProxy(pPayload->mProxyAddr, pPayload->mXnAddr);
                  }

                  mXConnections.add(pConn);
                  mXConnectionsPending.removeIndex(i);
                  return false;
               }
            }

            nlogt(cTransportNL, "cSessionEventInit -- init new connection ip[%s] proxy[%s]",
               strAddr.getPtr(), strProxyAddr.getPtr());

            // this connection was not found in the pending list
            // so let's add it now
            BXConnection* pConn = HEAP_NEW(BXConnection, gNetworkHeap);
            pConn->init(&mSendBufferAllocator, mEventHandle, mConnectionEventHandle, mpVoiceInterface, mUDPThread.getEventHandle(), pPayload->mAddr, this, mLocalXnAddr, &pPayload->mXnAddr, &pPayload->mProxyAddr);

            //if (pPayload->mProxied)
            //{
            //   pConn->disable();
            //   pConn->setProxy(pPayload->mProxyAddr.sin_addr);
            //}

            mXConnections.add(pConn);
            break;
         }

      case cSessionEventInitPending:
         {
            // create a new BXConnection for the given SOCKADDR
            // do I need anything other than the SOCKADDR?
            BSessionInitPayload* pPayload = reinterpret_cast<BSessionInitPayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;

            // first insure that the connection has not already been created
            BXConnection* pConn = getConnectionByAddr(pPayload->mAddr.sin_addr);
            if (pConn != NULL)
               break;

            BNetIPString strAddr(pPayload->mAddr);
            nlogt(cTransportNL, "cSessionEventInitPending -- init new connection ip[%s]", strAddr.getPtr());

            pConn = HEAP_NEW(BXConnection, gNetworkHeap);
            pConn->init(&mSendBufferAllocator, mEventHandle, mConnectionEventHandle, mpVoiceInterface, mUDPThread.getEventHandle(), pPayload->mAddr, this, mLocalXnAddr, &pPayload->mXnAddr, &pPayload->mProxyAddr);

            mXConnectionsPending.add(pConn);
            break;
         }

      case cSessionEventBroadcast2:
         {
            BSessionBuffer* pBuf = reinterpret_cast<BSessionBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

            IGNORE_RETURN(Utils::FastMemCpy(&mTempSessionBuffer, pBuf, sizeof(BSessionBuffer)));
            if (pBuf->mReserved == 0)
               mSessionBufferAllocator.free(pBuf);
            else
               HEAP_DELETE(pBuf, gNetworkHeap);

            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else
               {
                  pConn->send(mTempSessionBuffer.mBuf, mTempSessionBuffer.mSize, mTempSessionBuffer.mFlags);
                  //if (event.mPrivateData2 & cSendImmediate)
                  //   pConn->flush();
               }
            }

            for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i, false);
               }
               else
               {
                  pConn->send(mTempSessionBuffer.mBuf, mTempSessionBuffer.mSize, mTempSessionBuffer.mFlags);
                  //if (event.mPrivateData2 & cSendImmediate)
                  //   pConn->flush();
               }
            }
            break;
         }

      case cSessionEventBroadcast:
         {
            BSessionBuffer* pBuf = reinterpret_cast<BSessionBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

            IGNORE_RETURN(Utils::FastMemCpy(&mTempSessionBuffer, pBuf, sizeof(BSessionBuffer)));
            if (pBuf->mReserved == 0)
               mSessionBufferAllocator.free(pBuf);
            else
               HEAP_DELETE(pBuf, gNetworkHeap);

            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else
               {
                  pConn->send(mTempSessionBuffer.mBuf, mTempSessionBuffer.mSize, mTempSessionBuffer.mFlags);
                  //if (event.mPrivateData2 & cSendImmediate)
                  //   pConn->flush();
               }
            }

            // do not broadcast to pending connections

            break;
         }

      case cNetEventVoiceBroadcast:
         {
            // sent from the voice thread to the session thread
            BVoiceBuffer* pBuf = reinterpret_cast<BVoiceBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

            BDEBUG_ASSERTM(mpVoiceAllocator != NULL, "Missing Voice Allocator");
            if (mpVoiceAllocator == NULL)
               break;

            IGNORE_RETURN(Utils::FastMemCpy(&mTempVoiceBuffer, pBuf, sizeof(BVoiceBuffer)));
            mpVoiceAllocator->free(pBuf);

            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else
               {
                  pConn->voice(mTempVoiceBuffer.mXuid, mTempVoiceBuffer.mBuf, mTempVoiceBuffer.mSize);

               }
            }

            // do not broadcast to pending connections

            break;
         }

      case cSessionEventSend:
         {
            BSessionBuffer* pBuf = reinterpret_cast<BSessionBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pBuf);
            if (pBuf == NULL)
               break;

#ifndef BUILD_FINAL
            BNetIPString strAddr(pBuf->mAddr);
            nlogt(cTransportNL, "cSessionEventSend -- sending packet to ip[%s]", strAddr.getPtr());
#endif

            IGNORE_RETURN(Utils::FastMemCpy(&mTempSessionBuffer, pBuf, sizeof(BSessionBuffer)));
            if (pBuf->mReserved == 0)
               mSessionBufferAllocator.free(pBuf);
            else
               HEAP_DELETE(pBuf, gNetworkHeap);

//#ifndef BUILD_FINAL
//            BNetIPString strAddr(mTempSessionBuffer.mAddr);
//            nlogt(cTransportNL, "cSessionEventSend -- sending packet to ip[%s]", strAddr.getPtr());
//#endif

            // find the BXConnection that matches the given SOCKADDR_IN
            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == mTempSessionBuffer.mAddr.sin_addr.s_addr)
               {
                  pConn->send(mTempSessionBuffer.mBuf, mTempSessionBuffer.mSize, mTempSessionBuffer.mFlags);
                  //if (event.mPrivateData2 & cSendImmediate)
                  //   pConn->flush();
                  return false;
               }
            }

            // if we failed to find a current connection, check the pending list
            for (int i=mXConnectionsPending.getSize()-1; i >= 0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == mTempSessionBuffer.mAddr.sin_addr.s_addr)
               {
                  pConn->send(mTempSessionBuffer.mBuf, mTempSessionBuffer.mSize, mTempSessionBuffer.mFlags);
                  //if (event.mPrivateData2 & cSendImmediate)
                  //   pConn->flush();
                  return false;
               }
            }

#ifndef BUILD_FINAL
            nlogt(cTransportNL, "cSessionEventSend -- Failed to find connection for ip[%s]", strAddr.getPtr());
#endif

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

            IGNORE_RETURN(Utils::FastMemCpy(&mTempNetBuffer, pBuf, sizeof(BXNetBuffer)));
            //mTempNetBuffer = *pBuf;
            mRecvBufferAllocator.free(pBuf);

            BNetIPString strAddr(mTempNetBuffer.mAddr);

            // find the BXConnection that matches the given SOCKADDR_IN
            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == mTempNetBuffer.mAddr.sin_addr.s_addr)
               {
                  nlogt(cTransportNL, "cNetEventUDPRecv -- Connection ip[%s] size[%d]", strAddr.getPtr(), mTempNetBuffer.mSize);
                  pConn->recv(mTempNetBuffer.mBuf, mTempNetBuffer.mSize, mTempNetBuffer.mTime);
                  return false;
               }
            }
            // if I can't find a connection for this buffer, do I send it to the BSession
            // layer for parsing?
            // this event is currently being handled by the connection thread (sim helper)
            // so I could queue up the data for a possible future connection here
            // or I could pass this event up to the session receive layer (recvd)
            // so it can crack open the packet and look for the join request/response information
            //
            // actually
            //
            // create a BXConnection for this new SOCKADDR_IN, and place in a pending connection list
            //
            // I do NOT want the connection to be part of our normal broadcast traffic, thus the reason for the pending list
            //
            // I DO want the connect to be available for "flush" events and direct sends
            for (int i=mXConnectionsPending.getSize()-1; i >= 0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == mTempNetBuffer.mAddr.sin_addr.s_addr)
               {
                  nlogt(cTransportNL, "cNetEventUDPRecv -- Pending connection ip[%s] size[%d]", strAddr.getPtr(), mTempNetBuffer.mSize);
                  pConn->recv(mTempNetBuffer.mBuf, mTempNetBuffer.mSize, mTempNetBuffer.mTime);
                  return false;
               }
            }

            // early out check for disconnect messages
            // this isn't perfect but does help a little in skipping connection churn
            // if the disconnect packet was part of a larger payload and we still skip this check
            // then the connection will be shutdown normally later on
            if (mTempNetBuffer.mSize > 0)
            {
               BNetHeader* pHeader = reinterpret_cast<BNetHeader*>(mTempNetBuffer.mBuf);
               if (pHeader->type == BNetHeaderType::cDisconnect)
                  break;
            }

            nlogt(cTransportNL, "cNetEventUDPRecv -- Unknown connection ip[%s] size[%d]", strAddr.getPtr(), mTempNetBuffer.mSize);

            // another client established a connection to us and received
            // approval from the host to join the session, yet we're still pending
            //
            // create a pending connection for this client and begin sequence ordering and pings
            //
            // if we start receiving data, the session will queue the data until we're good to go.
            BXConnection* pConn = HEAP_NEW(BXConnection, gNetworkHeap);

            pConn->init(&mSendBufferAllocator, mEventHandle, mConnectionEventHandle, mpVoiceInterface, mUDPThread.getEventHandle(), mTempNetBuffer.mAddr, this, mLocalXnAddr);
            pConn->recv(mTempNetBuffer.mBuf, mTempNetBuffer.mSize, mTempNetBuffer.mTime);

            mXConnectionsPending.add(pConn);

            break;
         }

      case cNetEventConnRecv:
         {
            // received a completed data packet from the connection
//-- FIXING PREFIX BUG ID 7567
            const BXNetData* pData = reinterpret_cast<BXNetData*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pData);
            if (pData == NULL)
               break;

            dataReceived(pData->mBuf, pData->mSize, pData->mAddr);
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
            //
            // need to forward this event up the chain to the session layer (running on the sim thread)

            if (event.mPrivateData2 == INADDR_LOOPBACK)
               break;

            if (threadIndex == cThreadIndexSim)
            {
               // lookup the BMachineID for the addr stored in mPrivateData2
               IN_ADDR addr;
               addr.s_addr = event.mPrivateData2;
               BMachine* pMachine = getMachineByAddr(addr);
               // if we've failed to find a machine ID, we should probably still do our best to cleanup any
               // pending BXConnector, BXConnection for the given addr
               if (pMachine == NULL)
               {
                  uint count = mXConnectors.getSize();
                  for (uint i=0; i < count; ++i)
                  {
                     BXConnector& connector = mXConnectors[i];

                     if (connector.getAddr().s_addr == addr.s_addr)
                     {
                        connector.terminate();
                        mXConnectors.removeIndex(i);
                        break;
                     }
                  }
                  // if I don't have a BMachine for the addr in question
                  // send a deinit peer event to insure any BXConnections are cleaned up
                  gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventDeinitPeer, event.mPrivateData, addr.s_addr);
               }
               else
               {
                  // we're only going to allow re-routing of dead connections for connected clients
                  //
                  // if we happen to get a socket error on a pending connection, then we'll simply allow that connection to terminate and be done with it
                  //
                  // before calling socketDisconnect() we need to issue a proxy request packet
                  //
                  // if proxy support is disabled, or this is an explicit WSAEDISCON or the machine has already been marked as disconnected
                  // then I want to issue the socket error immediately and skip any proxy work
                  if (!mProxySupport || event.mPrivateData == WSAEDISCON || pMachine->getState() > BMachine::cStateConnected)
                  {
                     // quick out
                     socketDisconnect(pMachine->getID(), event.mPrivateData);
                     break;
                  }

                  // If we lost connection to the host and the mDoNotProxyToHost flag is set then disconnect now.
                  //
                  // This is for the case where we lose connection to the host of the party session which means
                  // we will most likely also be unable to establish a connection to that host on game launch,
                  // so by disconnecting now we'll save some pain down the road
                  // 
                  if (mDoNotProxyToHost && (CompareXnAddr(pMachine->getXnAddr(), mHostXnAddr) || isHosted()))
                  {
                     socketDisconnect(pMachine->getID(), event.mPrivateData);
                     break;
                  }

                  // if the machine is already disabled, do I still need to execute the remaining code?
                  if (!pMachine->isEnabled())
                     break;

                  pMachine->disable();

                  bool stillAlive = false;

                  // do I have any BMachines left that are enabled?  If not, then our attempts at locating a proxy for this machine will fail
                  for (uint i=0; i < XNetwork::cMaxClients; ++i)
                  {
                     const BMachine& machine = mMachines[i];
                     if (!machine.isLocal() && machine.isEnabled())
                     {
                        stillAlive = true;
                        break;
                     }
                  }

                  if (!stillAlive)
                  {
                     // we have no more machines left, so let's end it now
                     socketDisconnect(pMachine->getID(), event.mPrivateData);

                     // should also go through and call disconnects on the remaining disabled machines
                     // if the proxy response timer is running, then I may as well end that now and disconnect the remaining machines?
                     break;
                  }

                  // check to see if we already have a proxy request pending for this xnaddr
                  for (uint i=0; i < mProxyPending.getSize(); ++i)
                  {
                     if (CompareXnAddr(mProxyPending[i], pMachine->getXnAddr()))
                        return false;
                  }

                  // add our xnaddr to the list of xnaddrs that are pending proxies
                  mProxyPending.add(pMachine->getXnAddr());

                  // there's at least one machine out there that might be able to proxy for us, let's try them
                  BProxyPacket packet(BPacketType::cProxyRequestPacket);

                  packet.addPeer(pMachine->getXnAddr(), false);

#ifndef BUILD_FINAL
                  BNetIPString strXnAddr(pMachine->getXnAddr());
                  nlog(cSessionNL, "Broadcasting cProxyRequestPacket to non-pending connections for peer[%s] uniqueID[%I64u]", strXnAddr.getPtr(), BSession::getUniqueID(pMachine->getXnAddr()));
#endif

                  SendPacket(packet);

                  mProxyResponseTimeout = timeGetTime() + mProxyRequestTimeout;

                  if (!mProxyResponseTimerSet)
                  {
                     mProxyResponseTimerSet = true;

                     BEvent handleEvent;
                     handleEvent.clear();
                     handleEvent.mFromHandle = mEventHandle;
                     handleEvent.mToHandle = mEventHandle;
                     handleEvent.mEventClass = cSessionEventProxyResponseTimer;
                     gEventDispatcher.registerHandleWithEvent(mProxyResponseTimer.getHandle(), handleEvent);

                     mProxyResponseTimer.set(1000);
                  }

                  //socketDisconnect(pMachine->getID(), event.mPrivateData);
               }
            }
            else
            {
               BNetIPString strAddr(event.mPrivateData2);
               nlogt(cSessionNL, "cNetEventSocketError -- ip[%s:%08X] error[%d]", strAddr.getPtr(), event.mPrivateData2, event.mPrivateData);

               // need to intercept the socket error and attempt to init a proxy route
               // since a proxy request may take a little bit, I need to track outstanding proxy requests
               // somehow so that if they fail I can complete the peer disconnect
               //
               // if I have no more connections, then the proxy request will most certainly fail
               //
               // first step will be to disable the flushing of the connection so we don't spin out of control with socket errors
               //
               // we may receive multiple socket errors in a row because of multiple pending sends on the socket
               // so whatever we do here needs to take that into account
               if (event.mPrivateData == WSAEDISCON || event.mPrivateData == WSAETIMEDOUT)
               {
                  // if my connection is already being proxied, then the only time I want to go through this is if
                  // I'm disconnecting/timing out the connection.
                  disablePeer(event.mPrivateData2);
               }
               else
               {
                  BXConnection* pConn = getConnectionByAddr(event.mPrivateData2);
                  if (pConn == NULL || !pConn->isProxied())
                  {
                     disablePeer(event.mPrivateData2);
                  }
               }

               if (event.mPrivateData == WSAEDISCON)
                  deinitPeer(event.mPrivateData2);

               // if all connections are disabled and the local connection has only a single client, then we can skip the proxy process
               // but how does the session layer check for disabled connections?
               // do I disable the BMachines as well?  Basically mimic the proxy enable/disable/addr functionality of the BXConnection?

               // now that the connection is disabled (no voice or flushing of the queue)
               // I can issue proxy requests?

               // deinit the peer now?
               // this will remove the BXConnection from our list
               // XXX allow the proxy discovery to take place first
               //deinitPeer(event.mPrivateData2);

               // need to forward on to the sim thread
               BEvent forward = event;
               forward.mFromHandle = event.mToHandle;
               forward.mToHandle = mEventHandle;
               gEventDispatcher.send(forward);
            }
            break;
         }

      case cNetEventPingUpdate:
         {
            // update the client's ping values
            IN_ADDR addr;
            addr.s_addr = event.mPrivateData;
            BMachine* pMachine = getMachineByAddr(addr);
            if (pMachine != NULL)
            {
               uint32 avgPing = event.mPrivateData2 & 0xFFFF;
               uint32 stdDev = (event.mPrivateData2 >> 16) & 0xFFFF;

               pMachine->updatePing(avgPing, stdDev);

               for (uint i=0; i < BMachine::cMaxUsers; ++i)
               {
                  BClient* pClient = getClient(pMachine->mUsers[i].mClientID);
                  if (pClient)
                  {
                     pClient->updatePing(avgPing, stdDev);
                     addEvent(cEventClientPing, pClient->getID(), avgPing);
                  }
               }
            }

            break;
         }

      case cNetEventPingNotResponding:
         {
            // the ping handler has exceeded the ping attempts (currently around 15 seconds, todo: reduce)
            //
            // inform the upper layers and allow them to terminate the client
            //
            // this will happen in the event that the xbox is able to remain connected somehow but
            // fails to process traffic
            break;
         }

      case cSessionEventInitHost:
         {
            BDEBUG_ASSERT(mpHostConnector);
            if (mpHostConnector == NULL)
               return false;

            mpHostConnector->service();

            if (mpHostConnector->getState() == BXConnector::cStateConnecting)
               return false;

            if (mPeerTimerSet)
            {
               mPeerTimer.cancel();
               mPeerTimerSet = false;
               gEventDispatcher.deregisterHandle(mPeerTimer.getHandle(), cThreadIndexSim);
            }

            handleJoin();

            break;
         }

      case cSessionEventInitPeers:
         {
            // XXX this needs to run on the sim thread
            // XXX check a shutdown bool

            // loop through our BXConnectors and insure that they're either all connected or failed
            // and then we can send a packet to the host with the state of all the connections
            //
            // this packet need only be sent once
            //
            // how do I start/stop the timer for this?  Start when I receive a peers packet
            // and then stop when they're all complete?
            // 
            uint count = mXConnectors.getSize();
            if (count > 0)
            {
               bool stillConnecting = false;
               for (uint i=0; i < count; ++i)
               {
                  BXConnector& connector = mXConnectors[i];

                  connector.service();

                  if (connector.getState() == BXConnector::cStateConnecting)
                     stillConnecting = true;
               }
               if (!stillConnecting)
               {
                  // all the connections are complete in one form or another
                  // send a init peers response packet letting the host know
                  // where we stand with the connections
                  if (mPeerTimerSet)
                  {
                     mPeerTimer.cancel();
                     mPeerTimerSet = false;
                     gEventDispatcher.deregisterHandle(mPeerTimer.getHandle(), cThreadIndexSim);
                  }

                  // check for peer failures, if we have a failure, then we'll need to initiate pending connections
                  // and broadcast a request for proxies.
                  //
                  // all proxy communication will need to be done via pending connections so we don't pick up other traffic
                  // in cases where we need to elect a new proxy during a game
                  bool sendProxyRequest = false;
                  bool tooManyProxyRequests = false;

                  uint totalConnections = 0;

                  for (uint i=0; i < count; ++i)
                  {
                     BXConnector& connector = mXConnectors[i];

                     totalConnections += (connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED ? 1 : 0);
                  }

                  // if our total successful connections equals the number of requested connections
                  // then I'm safe and don't need to worry about proxy scenarios (for now)
                  if (count == totalConnections)
                  {
                  }
                  else if (count > 0 && totalConnections == 0)
                  {
                     tooManyProxyRequests = true;
                  }
                  // allow count == 2 && total == 1
                  else if (count == 3 && totalConnections < 2)
                  {
                     // allow if count == 3 and total >= 2
                     tooManyProxyRequests = true;
                  }
                  else if (count > 3 && totalConnections < 3)
                  {
                     // allow if count == 4 or 5 and total >= 3
                     tooManyProxyRequests = true;
                  }

                  if (mProxySupport && !tooManyProxyRequests)
                  {
                     BProxyPacket packet(BPacketType::cProxyRequestPacket);

                     for (uint i=0; i < count; ++i)
                     {
                        BXConnector& connector = mXConnectors[i];

                        if (connector.getStatus() != XNET_CONNECT_STATUS_CONNECTED)
                        {
                           sendProxyRequest = true;

                           // I do not want to create a pending connection yet until someone is able to act as a proxy
                           mProxyPending.add(connector.getXnAddr());

                           packet.addPeer(connector.getXnAddr(), connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED);
                        }
                        else if (connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED)
                        {
                           // create a pending connection so we can broadcast our proxy request
                           BSessionInitPayload* pPayload = HEAP_NEW(BSessionInitPayload, gNetworkHeap);
                           pPayload->init(connector.getAddr(), connector.getPort(), connector.getXnAddr());

                           gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cSessionEventInitPending, 0, 0, pPayload);
                        }
                     }

                     if (sendProxyRequest)
                     {
                        mProxyResponseTimeout = timeGetTime() + mProxyRequestTimeout;

                        if (!mProxyResponseTimerSet)
                        {
                           mProxyResponseTimerSet = true;

                           BEvent handleEvent;
                           handleEvent.clear();
                           handleEvent.mFromHandle = mEventHandle;
                           handleEvent.mToHandle = mEventHandle;
                           handleEvent.mEventClass = cSessionEventProxyResponseTimer;
                           gEventDispatcher.registerHandleWithEvent(mProxyResponseTimer.getHandle(), handleEvent);

                           mProxyResponseTimer.set(1000);
                        }

#ifndef BUILD_FINAL
                        nlog(cSessionNL, "Broadcasting cProxyRequestPacket to all connections");
                        for (uint i=0; i < packet.mCount; ++i)
                        {
                           BNetIPString strXnAddr(packet.mPeers[i]);
                           nlog(cSessionNL, "cProxyRequestPacket peer[%s] uniqueID[%I64u]", strXnAddr.getPtr(), BSession::getUniqueID(packet.mPeers[i]));
                        }
#endif

                        // broadcast the proxy request to all connections (including pending ones)
                        SendBroadcast2(packet);
                     }
                  }

                  // if proxy support is disabled or we did not need to issue proxy requests,
                  // then simply reply to the host with our results of the connections
                  if (!mProxySupport || !sendProxyRequest)
                  {

                     // after we've stopped the timer, we can issue the packet
                     SOCKADDR_IN addr;
                     addr.sin_family = AF_INET;
                     addr.sin_addr = mHostSecureINADDR;
                     addr.sin_port = mHostPort;

                     // loop the connectors again and fill out a peer response packet with their values
                     // what will the host key off of when looking up those peers to insure we're good?
                     BInitPeersResponsePacket packet;

                     for (uint i=0; i < count; ++i)
                     {
                        BXConnector& connector = mXConnectors[i];
                        packet.addPeer(connector.getXnAddr(), connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED);
                     }

                     packet.setLocalInfo(mLocalXnAddr, mLocalUsers);

                     SendPacketToAddr(addr, packet);
                  }
               }
            }

            break;
         }

      case cSessionEventDeinitPeer:
         {
            deinitPeer(event.mPrivateData2);
            break;
         }

      case cSessionEventDeinit:
         {
            // the connection handler is running on the sim helper thread by default
            // this event is raised from the sim thread when we're disposing the session layer
            // clean-up the connections and get out as fast as possible
            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else
               {
                  pConn->deinit();
                  HEAP_DELETE(pConn, gNetworkHeap);
               }
            }
            mXConnections.clear();

            for (int i=mXConnectionsPending.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i, false);
               }
               else
               {
                  pConn->deinit();
                  HEAP_DELETE(pConn, gNetworkHeap);
               }
            }
            mXConnectionsPending.clear();
            break;
         }

      case cNetEventVoiceTalkerList:
         {
            // this will be handled on the sim helper thread, so we have to be careful in sending packets
//-- FIXING PREFIX BUG ID 7568
            const BVoiceTalkerListPayload* pPayload = reinterpret_cast<BVoiceTalkerListPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;

            // create a talker packet and broadcast to everyone
            BTalkersPacket packet(pPayload->mOwnerXuid, pPayload->mXuids);
            SendPacket(packet);
            break;
         }

      case cNetEventVoiceHeadsetPresent:
         {
            // handled on the connection thread (sim helper)
            BVoiceHeadsetPacket packet(event.mPrivateData, event.mPrivateData2);
            SendPacket(packet);
            break;
         }

      case cNetEventVoiceMuteClient:
         {
//-- FIXING PREFIX BUG ID 7569
            const BVoiceMuteRequestPayload* pRequest = reinterpret_cast<BVoiceMuteRequestPayload*>(event.mpPayload);
//--
            BDEBUG_ASSERT(pRequest);
            if (pRequest == NULL)
               break;

            for (int i=mXConnections.getSize()-1; i >=0; --i)
            {
               BXConnection* pConn = mXConnections[i];
               if (pConn == NULL)
               {
                  mXConnections.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == pRequest->mAddr.sin_addr.s_addr)
               {
                  pConn->setMute(pRequest->mMute);
                  return false;
               }
            }

            // if we failed to find a current connection, check the pending list
            for (int i=mXConnectionsPending.getSize()-1; i >= 0; --i)
            {
               BXConnection* pConn = mXConnectionsPending[i];
               if (pConn == NULL)
               {
                  mXConnectionsPending.removeIndex(i, false);
               }
               else if (pConn->getAddr().sin_addr.s_addr == pRequest->mAddr.sin_addr.s_addr)
               {
                  pConn->setMute(pRequest->mMute);
                  return false;
               }
            }

            break;
         }

      case cSessionEventInitProxy:
         {
            // executed on the connection thread
            BSessionInitPayload* pPayload = reinterpret_cast<BSessionInitPayload*>(event.mpPayload);
            BDEBUG_ASSERT(pPayload);
            if (pPayload == NULL)
               break;

            BXConnection* pConn = getConnectionByAddr(pPayload->mAddr.sin_addr);
            if (pConn != NULL)
            {
               //BNetIPString strAddr(pPayload->mAddr);
               //BNetIPString strProxyAddr(pPayload->mProxyAddr);
               //nlogt(cSessionNL, "cSessionEventInitProxy -- BXConnection::setProxy ip[%s] proxy[%s] uniqueID[%I64u]",
               //   strAddr.getPtr(), strProxyAddr.getPtr(), pConn->getUniqueID());

               pConn->setProxy(pPayload->mProxyAddr, pPayload->mXnAddr);
            }

            break;
         }

      case cSessionEventProxyRequestTimer:
         {
            // executed on the sim thread
            //
            // handles the polling of outstanding proxy requests so that we can time them out if need be
            if (mProxyRequests.getSize() == 0)
            {
               mProxyRequestTimerSet = false;
               mProxyRequestTimer.cancel();
               gEventDispatcher.deregisterHandle(mProxyRequestTimer.getHandle(), cThreadIndexSim);
               break;
            }

            uint now = timeGetTime();
            for (int i=mProxyRequests.getSize()-1; i >= 0; --i)
            {
               BProxyRequest* pProxy = mProxyRequests[i];
               if (pProxy->getState() == BProxyRequest::cStateInit)
               {
                  // first check to insure we're still connected to this machine
                  // and that the machine is not already proxied (prevents proxy to proxy)
                  const BMachine* pMachine = getMachineByXnAddr(pProxy->getToXnAddr());
                  if (pMachine == NULL || pMachine->getState() != BMachine::cStateConnected || pMachine->isProxied())
                  {
                     // no sense sending a ping, as we're now disconnected from this peer
                     BProxyPacket response(BPacketType::cProxyResponsePacket);
                     response.addPeer(pProxy->getToXnAddr(), false);
                     SendPacketToAddr(pProxy->getFromSockAddr(), response);

                     HEAP_DELETE(pProxy, gNetworkHeap);
                     mProxyRequests.removeIndex(i);
                  }
                  else
                  {
                     pProxy->setState(BProxyRequest::cStatePingSent);

                     // send a ping to insure this machine is still alive
                     BProxyPacket ping(BPacketType::cProxyPingPacket);
                     ping.setNonce(pProxy->getNonce());
                     SendPacketToAddr(pMachine->getSockAddr(), ping);
                  }
               }
               else if (pProxy->getState() == BProxyRequest::cStateComplete)
               {
                  HEAP_DELETE(pProxy, gNetworkHeap);
                  mProxyRequests.removeIndex(i);
               }
               else if (pProxy->getRequestTimeout() < now)
               {
                  // request timed out, now what?
                  //
                  // the client that requested a proxy to this peer needs to be notified
                  // that we're also unable to reach them
                  BProxyPacket response(BPacketType::cProxyResponsePacket);
                  response.addPeer(pProxy->getToXnAddr(), false);
                  SendPacketToAddr(pProxy->getFromSockAddr(), response);

                  HEAP_DELETE(pProxy, gNetworkHeap);
                  mProxyRequests.removeIndex(i);
               }
            }

            break;
         }

      case cSessionEventProxyResponseTimer:
         {
            // if we timeout waiting for proxy responses, then we need to take
            // steps to respond to the host if we're attempting a connection
            // or finish the socket disconnect for the peers
            if (mProxyPending.getSize() == 0 || mProxyResponseTimeout < timeGetTime())
            {
               mProxyResponseTimerSet = false;
               mProxyResponseTimer.cancel();
               gEventDispatcher.deregisterHandle(mProxyResponseTimer.getHandle(), cThreadIndexSim);

               // if we're not connecting and we're not disconnecting, then let's inform the host
               if (!isConnected() && !isDisconnecting())
               {
                  if (!mRequirePeersResponse)
                     break;

                  mRequirePeersResponse = false;

                  // create an init peers packet
                  SOCKADDR_IN hostAddr;
                  hostAddr.sin_family = AF_INET;
                  hostAddr.sin_addr = mHostSecureINADDR;
                  hostAddr.sin_port = mHostPort;

                  // loop the connectors again and fill out a peer response packet with their values
                  // what will the host key off of when looking up those peers to insure we're good?
                  BInitPeersResponsePacket response;

                  uint count = mXConnectors.getSize();
                  for (uint i=0; i < count; ++i)
                  {
                     BXConnector& connector = mXConnectors[i];
                     // if we connected or we successfully proxied our connection, then we can let the host know that we succeeded
                     response.addPeer(connector.getXnAddr(), (connector.getStatus() == XNET_CONNECT_STATUS_CONNECTED || connector.isProxied()), connector.getProxyXnAddr());
                  }

                  response.setLocalInfo(mLocalXnAddr, mLocalUsers);

                  SendPacketToAddr(hostAddr, response);
               }
               else
               {
                  // disconnect the pending xnaddrs
                  uint count = mProxyPending.getSize();
                  for (uint i=0; i < count; ++i)
                  {
                     const BMachine* pMachine = getMachineByXnAddr(mProxyPending[i]);

                     if (pMachine != NULL)
                        socketDisconnect(pMachine->getID(), WSAETIMEDOUT);
                  }
                  mProxyPending.clear();
               }
            }
            break;
         }

#ifndef BUILD_FINAL
      case cSessionEventProxyInfo:
         {
            IN_ADDR addr;
            addr.s_addr = event.mPrivateData;
            BMachine* pMachine = getMachineByAddr(addr);

            IN_ADDR proxyAddr;
            proxyAddr.s_addr = event.mPrivateData2;
            BMachine* pProxyMachine = getMachineByAddr(proxyAddr);

            if (pMachine && pMachine->mID >= 0 && pMachine->mID < XNetwork::cMaxClients)
            {
               mMachineProxyInfo[pMachine->mID] = (pProxyMachine ? pProxyMachine->mID : -1);
               mUserProxyInfo[pMachine->mID] = pMachine->mUsers[0].mGamertag;
               if (pProxyMachine && pProxyMachine->mID >= 0 && pProxyMachine->mID < XNetwork::cMaxClients)
                  mUserProxyInfo[pMachine->mID+XNetwork::cMaxClients] = pProxyMachine->mUsers[0].mGamertag;
            }

            break;
         }
#endif
   }

   return false;
}
