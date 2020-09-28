//==============================================================================
// lspDataConnection.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// Includes
#include "common.h"
#include "configsgame.h"

#include "lspManager.h"
#include "lspDataConnection.h"

// xcore
#include "containers/freelist.h"

// xnetwork
#include "NetPackets.h"
#include "MaxSendSize.h"

IMPLEMENT_FREELIST(BLSPDataConnection, 2, &gSimHeap);

//==============================================================================
// 
//==============================================================================
BLSPDataConnection::BLSPDataConnection() :
   mNonce(0),
   mMachineId(0),
   mpConn(NULL),
   mpSocket(NULL),
   mXNetGetTitleXnAddrFlags(0),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mState(cStateIdle),
   mPort(0)
{
   Utils::FastMemSet(&mXnAddr, 0, sizeof(XNADDR));
   Utils::FastMemSet(&mServerAddr, 0, sizeof(IN_ADDR));
}

//==============================================================================
// 
//==============================================================================
BLSPDataConnection::~BLSPDataConnection()
{
   onRelease();
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::onAcquire()
{
   mpConn = NULL;
   mpSocket = NULL;

   mNonce = 0;
   mLastTime = 0;
   mRetryTime = 0;
   mNumRetries = 0;
   mState = cStateIdle;

   Utils::FastMemSet(&mServerAddr, 0, sizeof(IN_ADDR));
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::onRelease()
{
   // shutdown/close the socket/connection
   delete mpConn;
   delete mpSocket;
   mpConn = NULL;
   mpSocket = NULL;

   // I will need to inform my observers that I'm going away so they can cleanup
   uint count = mObserverList.getSize();
   for (int i=count-1; i >= 0; --i)
   {
      BLSPDataObserver* pObserver = mObserverList[i];
      if (pObserver)
         pObserver->shutdown();
   }
   mObserverList.clear();
}

//==============================================================================
// 
//==============================================================================
HRESULT BLSPDataConnection::connect(const IN_ADDR& lspAddr, ushort port)
{
   mPort = port;
   mServerAddr = lspAddr;

   mpSocket = new BUDP360Socket(this);

   HRESULT hr;

   // initialize a socket on port 1100 and bind
   hr = mpSocket->init(mPort);
   if (FAILED(hr))
      return hr;

   mpConn = new BUDP360Connection(this);

   hr = mpConn->connect(mpSocket, mServerAddr, mPort);
   if (FAILED(hr))
      return hr;

   mState = cStateConnect;
   //mState = cStateGetXnAddr;

   return S_OK;
}


//==============================================================================
// BSocket::BObserver interface
//==============================================================================
void BLSPDataConnection::connected(IN BSocket* pSocket)
{
}

//==============================================================================
// BSocket::BObserver interface
//==============================================================================
void BLSPDataConnection::interfaceAddressChanged()
{
}

//==============================================================================
// BSocket::BObserver interface
//==============================================================================
bool BLSPDataConnection::notifyUnresponsiveSocket(IN BSocket* pSocket, DWORD lastRecvTime)
{
   return true;
}

//==============================================================================
// BSocket::BObserver interface
//==============================================================================
bool BLSPDataConnection::notifyResponsiveSocket(IN BSocket* pSocket, DWORD lastRecvTime)
{
   return true;
}

//==============================================================================
// BSocket::BObserver interface
//==============================================================================
void BLSPDataConnection::disconnected(IN BSocket* pSocket, IN DWORD Status)
{
}

//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
void BLSPDataConnection::dataReceived(BUDP360Connection* pConn, const void* pData, long size, CONST SOCKADDR_IN* pRemoteAddress)
{
   // pData will point to BLSPPacket
   BDEBUG_ASSERT(size >= 7);
   if (size < 7)
      return;

   BLSPPacket packet;
   packet.deserializeFrom(pData, size);

   switch (packet.getType())
   {
      case BPacketType::cLSPCommandPacket:
         {
            BLSPCommandPacket packet;
            packet.deserializeFrom(pData, size);
            switch (packet.mCommand)
            {
               case BLSPCommandPacket::cCommandReset:
                  {
                     mState = cStateReconnect;
                  }
                  break;
               case BLSPCommandPacket::cCommandHelloResponse:
                  {
                     mNonce = 0;
                     mState = cStateConnected;
                     mpConn->setTimer(5000);
                  }
                  break;
            }
         }
         break;

      default:
         {
            uint count = mObserverList.getSize();
            for (uint i=0; i < count; ++i)
            {
               BLSPDataObserver* pObserver = mObserverList[i];
               if (pObserver->dataReceived(packet.getServiceId(), packet.getType(), packet.getVersion(), pData, size))
                  break;
            }
         }
         break;
   }
}


//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
void BLSPDataConnection::disconnected(BUDP360Connection* pConn)
{
   // if we get disconnected, need to kick back to a reconnect state
   // but only if we haven't entered an idle, complete or timed out state
   // this isn't very good way of handling disconnects at this level, we need a more
   // intelligent recovery... if I was in the middle of attempting a file upload
   // and I didn't error out for some other reason, then this needs to go through the reconnect phase
   //mState = cStateReconnect;
}

//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
bool BLSPDataConnection::notifyUnresponsiveConnection(BUDP360Connection* pConn, DWORD lastRecvTime)
{
   return true;
}

//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
bool BLSPDataConnection::notifyResponsiveConnection(BUDP360Connection* pConn, DWORD lastRecvTime)
{
   return true;
}

//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
void BLSPDataConnection::connected(BUDP360Connection* pConn)
{
}

//==============================================================================
// BUDP360Connection::BConnObserver interface
//==============================================================================
void BLSPDataConnection::connectTimeout(BUDP360Connection* pConn)
{
}

//==============================================================================
// BSocket::BObserver interface
//==============================================================================
void BLSPDataConnection::recvd(IN BSocket* pSocket, IN const void * pBuffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* pRemoteAddress)
{
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::service()
{
   if (!mpConn)
      return;

   switch (mState)
   {
      case cStateGetXnAddr:
         {
            if (timeGetTime() > mRetryTime)
            {
               mXNetGetTitleXnAddrFlags = XNetGetTitleXnAddr(&mXnAddr);
               if ((mXNetGetTitleXnAddrFlags & XNET_GET_XNADDR_NONE) == 0)
               {
                  setState(cStateGetMachineId);
               }
               else if ((mXNetGetTitleXnAddrFlags & XNET_GET_XNADDR_NONE) == XNET_GET_XNADDR_NONE)
               {
                  // this is an error condition and we want to abort the service requesting this data connection
                  setState(cStateTimedout);
               }
               else
               {
                  mRetryTime = timeGetTime() + mRetryInterval;
                  if (++mNumRetries > mMaxRetries)
                     setState(cStateTimedout);
               }
            }
         }
         break;

      case cStateGetMachineId:
         {
            if (XNetXnAddrToMachineId(&mXnAddr, &mMachineId) == 0)
            {
               // create the session id, for now, use the mNonce variable
               mNonce = timeGetTime();
               mNonce = (mNonce << 32) | (mMachineId & 0xFFFFFFFF);
               setState(cStateConnected);
            }
            else
            {
               // we will not retry, simply fail
               setState(cStateTimedout);
            }
         }
         break;

      case cStateConnect:
         {
            BLSPCommandPacket packet(0, BLSPCommandPacket::cCommandHello);

            long size = packet.getMaxSerialSize();
            Utils::FastMemSet(mSendBuffer, 0, size);

            void* pData = mSendBuffer;
            packet.serializeInto(&pData, &size);

            HRESULT hr = mpConn->send(pData, size);

            // why would this fail?
            if (FAILED(hr))
            {
               BDEBUG_ASSERTM(false, "Failed to issue LSP Hello Request");
               return;
            }

            mState = cStateConnecting;
            // set our update timer to 1000ms so we have a chance
            // of issuing acks to the server and let it figure out
            // if the hello packet was lost and requires retransmission
            mpConn->setTimer(1000);
         }
         break;
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::setState(eState state)
{
   mState = state;
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = 0;
}

//==============================================================================
// 
//==============================================================================
void BLSPDataConnection::setRetryInterval(DWORD interval)
{
   mRetryInterval = interval;
   mRetryTime = timeGetTime() + mRetryInterval;
}
