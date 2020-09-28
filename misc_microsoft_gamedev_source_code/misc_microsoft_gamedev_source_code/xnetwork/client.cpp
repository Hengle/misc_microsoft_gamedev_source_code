//==============================================================================
// client.cpp
//
// Copyright (c) Ensemble Studios, 2001-2008
//==============================================================================

// Includes
#include "precompiled.h"
#include "client.h"
#include "Session.h"
#include "datastream.h"
//#include "Ping.h"
#include "timesync.h"

//==============================================================================
// Defines

//==============================================================================
// 
//==============================================================================
BClient::BClient() :
   mID(-1),
   mLocal(FALSE),
   //mHost(false),
   mConnected(false),
   //mPing(0),
   //mControllerID(0), // be sure to always check that we're local before using the controllerID
   mXuid(0),
   //mMuted(true),
   //mDisconnectCount(0),
   //mDisconnectClients(-1),
   //mDisconnectReports(0),
   mPingAverage(0),
   mPingStdDev(0)
{
   IGNORE_RETURN(Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR)));
   IGNORE_RETURN(Utils::FastMemSet(&mAddr, 0, sizeof(SOCKADDR_IN)));

   //mTimingHistory = new BTimingHistory();
   //mTimingHistory = HEAP_NEW(BTimingHistory, gNetworkHeap);
   //mPing = new BPing;
   //mPing->setTransport(this);
   //mPing->addObserver(this);
}

//==============================================================================
// 
//==============================================================================
//BClient::BClient(XUID xuid, const BSimString& gamertag, const SOCKADDR_IN& addr) :
//   mID(-1),
//   mLocal(false),
//   mHost(false),
//   mConnected(false),
//   //mPing(0),
//   mControllerID(0), // be sure to always check that we're local before using the controllerID
//   mXuid(xuid),
//   mMuted(true),
//   mGamertag(gamertag),
//   mDisconnectCount(0),
//   mDisconnectClients(-1),
//   mDisconnectReports(0),
//   mPingAverage(0),
//   mPingStdDev(0)
//{
//   mAddr = addr;
//
//   mTimingHistory = new BTimingHistory();
//   //mPing = new BPing;
//   //mPing->setTransport(this);
//   //mPing->addObserver(this);
//}

//==============================================================================
// 
//==============================================================================
//BClient::BClient(BUDP360Connection* conn, const DWORD clientID, const DWORD controllerID, const bool host) :
//   mConnection(conn),
//   mID(clientID),
//   mLocal(TRUE),
//   mHost(host),
//   mConnected(FALSE),
//   //mPing(0),
//   mControllerID(controllerID),
//   mXuid(0),
//   mMuted(TRUE),
//   mDisconnectCount(0),
//   mDisconnectClients(-1),
//   mDisconnectReports(0),
//   mPingAverage(0),
//   mPingStdDev(0)
//{
//   CHAR pzUserName[XUSER_NAME_SIZE];
//   XUserGetName(mControllerID, pzUserName, XUSER_NAME_SIZE);
//
//   pzUserName[XUSER_NAME_SIZE - 1] = '\0';
//   mGamertag.set(pzUserName);
//
//   XUserGetXUID(mControllerID, &mXuid);
//
//   mTimingHistory = new BTimingHistory();
//   //mPing = new BPing;
//   //mPing->setTransport(this);
//   //mPing->addObserver(this);
//}

//==============================================================================
// 
//==============================================================================
//BClient::BClient(BUDP360Connection* conn, const DWORD clientID, const XUID xuid, const BSimString& gamertag, const bool host) :
//   mConnection(conn),
//   mID(clientID),
//   mLocal(FALSE),
//   mHost(host),
//   mConnected(FALSE),
//   //mPing(0),
//   mControllerID(0), // be sure to always check that we're local before using the controllerID
//   mXuid(xuid),
//   mMuted(TRUE),
//   mGamertag(gamertag),
//   mDisconnectCount(0),
//   mDisconnectClients(-1),
//   mDisconnectReports(0),
//   mPingAverage(0),
//   mPingStdDev(0)
//{
//   mTimingHistory = new BTimingHistory();
//   //mPing = new BPing;
//   //mPing->setTransport(this);
//   //mPing->addObserver(this);
//}

//==============================================================================
// 
//==============================================================================
BClient::~BClient()
{
   //if (mTimingHistory)
   //   HEAP_DELETE(mTimingHistory, gNetworkHeap);

   //if (mPing) 
   //   delete mPing;

   //if (mConnection)
   //   delete mConnection;
}

//==============================================================================
// 
//==============================================================================
void BClient::init(int32 id, XUID xuid, const BSimString& gamertag, const XNADDR& xnAddr, const SOCKADDR_IN& addr)
{
   BDEBUG_ASSERTM(id >= 0, "Invalid Client ID");

   mID = id;
   mXuid = xuid;
   mGamertag = gamertag;
   mXnAddr = xnAddr;
   mAddr = addr;

   mLocal = (addr.sin_addr.s_addr == INADDR_LOOPBACK);
}

//==============================================================================
// 
//==============================================================================
//HRESULT BClient::_SendData(const void* data, const long size, const DWORD flags, const char* file, long line)
//{
//   // if we're disconnecting, we will silently fail.
//   //if (mDisconnectCount > 0)
//   //   return S_OK;
//
//   // make sure we haven't muted this client, at the session/client layer, muting only means 'do not send voice traffic'
//   if ((flags & cSendUnencrypted) && isMuted())
//      return S_OK;
//
//#ifndef BUILD_FINAL
//   if (file)   
//      nlog(cBandwidthNL, "BClient::_SendData size %ld, %s - %ld", size, file, line);
//   else
//      BASSERT(0); // FOOL! Who's callin _sessionSendData directly?!? Use the macros! call sessionSendData!
//#endif
//
//   BDEBUG_ASSERTM(false, "BClient::_SendData -- BUDP360Connection deprecated");
//   //if (mConnection)
//   //   return mConnection->send(data, size, flags);
//
//   return E_FAIL;
//}

//==============================================================================
// 
//==============================================================================
//HRESULT BClient::_SendPacket(BPacket& packet, long* sizeOut, DWORD flags, const char* file, long line)
//{
//   // if we're disconnecting, we will silently fail.
//   //if (mDisconnectCount > 0)
//   //   return S_OK;
//
//   // make sure we haven't muted this client, at the session/client layer, muting only means 'do not send voice traffic'
//   if ((flags & cSendUnencrypted) && isMuted())
//      return S_OK;
//
//   BDEBUG_ASSERTM(false, "BClient::_SendPacket -- BUDP360Connection deprecated");
//
////   if (mConnection)
////   {
////      long size = packet.getMaxSerialSize();
////      if ((size > cMaxSendSize) || (size <= 0))
////      {
////         nlog(cTransportNL, "Invalid send size %ld (max send size %ld)", size, cMaxSendSize);
////         return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
////      }
////
////      XMemSet(mSendBuffer, 0, size);
////
////      void *data = mSendBuffer;
////      packet.serializeInto(&data, &size);
////
////      if (sizeOut) 
////         *sizeOut = size;
////
////#ifndef BUILD_FINAL
////      if (file)   
////         nlog(cBandwidthNL, "BClient::_SendPacket size %ld, %s - %ld", size, file, line);
////      else
////         BASSERT(0); // FOOL! Who's callin _sessionSendData directly?!? Use the macros! call sessionSendData!
////#endif
////
////      return mConnection->send(data, size, flags);
////   }
////   else
////   {
////      nlog(cBandwidthNL, "BClient::_SendPacket -- mConnection is NULL.");
////      return E_FAIL;
////   }
//   return E_FAIL;
//}

//==============================================================================
// 
//==============================================================================
//void BClient::service()
//{
   //if ((mPing) &&
   //    (mConnection) &&
   //    (mConnection->isConnected()))
   //{      
   //   mPing->service();  
   //}
   //else
   //{
   //   //Don't turn this next line on unless you want lots of spam when folks are establishing connections
   //   //nlog(cTransportNL, "BClient::service -- NOT servicing ping for client [%ld]", mID);
   //}
//}

//==============================================================================
// 
//==============================================================================
//void BClient::sendPingPacket(BPacket& packet) 
//{ 
//   HRESULT hr = SendPacketUnreliable(packet);
//   if (FAILED(hr))
//      nlog(cTransportNL, "BClient::sendPingPacket -- Failed for client [%ld]", mID);
//}

//==============================================================================
// This function turns on/off a bit that represents the client disconnect report
// Once every client that is connected has reported the disconnect, it is safe to 
// disconnect them.
//==============================================================================
//void BClient::clientDisconnectReported(DWORD fromClientID, bool add)
//{
//   if (add)
//      mDisconnectReports |= (1<<fromClientID);
//   else
//      mDisconnectReports &= ~(1<<fromClientID);
//
//   nlog(cConnectivityNL, "BClient::clientDisconnectReported -- disconnect report for client [%d] is [0x%x]", mID, mDisconnectReports);
//}

//==============================================================================
// 
//==============================================================================
//bool BClient::shouldDisconnect(DWORD connectedClients)
//{
//   nlog(cConnectivityNL, "BClient::shouldDisconnect -- disconnect report [0x%x] and connected clients [0x%x]", mDisconnectReports, connectedClients);
//   return ((mDisconnectReports&connectedClients) == connectedClients);
//}

//==============================================================================
// 
//==============================================================================
//void BClient::pingNotResponding()
//{   
//   mObserverList.clientNotResponding(this);
//}

//==============================================================================
// 
//==============================================================================
//void BClient::pingUpdated()
//{
//   mObserverList.clientPingUpdated(this, getPing()?getPing()->getPing():0);
//}

//==============================================================================
// 
//==============================================================================
//void BClient::disconnect()
//{
//   BDEBUG_ASSERTM(false, "BClient::disconnect -- BUDP360Connection deprecated");
//
//   //if (mConnection)
//   //   mConnection->disconnect();
//}

//==============================================================================
// 
//==============================================================================
//HRESULT BClient::getAddresses(SOCKADDR_IN* localAddress, SOCKADDR_IN* translatedLocalAddress)
//{
//   BDEBUG_ASSERTM(false, "BClient::getAddresses -- deprecated");
//
//   //if (!mConnection || !localAddress || !translatedLocalAddress)
//   //   return E_FAIL;
//
//   //TODO - fix up or remove
//   //*localAddress = *mConnection->getRemoteAddress();
//   //*translatedLocalAddress = *mConnection->getTranslatedRemoteAddress();
//
//   return E_FAIL;
//}

//==============================================================================
// 
//==============================================================================
//void BClient::sendPacket(BPacket& packet)
//{
//   long size;
//   HRESULT hr = SendPacketRS(packet, &size);
//   if(FAILED(hr))
//      nlog(cTransportNL, "BClient::sendPacket -- failed SendPacketRS. hr 0x%x", hr);
//}

//==============================================================================
// 
//==============================================================================
void BClient::updatePing(uint32 avg, uint32 stdDev)
{
   mPingAverage = avg;
   mPingStdDev = stdDev;
}
