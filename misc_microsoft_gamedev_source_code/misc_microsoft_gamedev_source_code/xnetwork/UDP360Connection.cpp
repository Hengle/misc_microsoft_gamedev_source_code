//==============================================================================
// BUDP360Connection.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// Includes
#include "precompiled.h"
#include "MaxSendSize.h"
#include "UDP360Connection.h"

#include "config.h"
#include "econfigenum.h"
#include "SocksHelper.h"

using namespace Etl;

extern BMemoryHeap gNetworkHeap;

namespace
{
   //==============================================================================
   // 
   //==============================================================================
   void GUIDtoString(uchar* src, PSTR strDest, unsigned long destBufLen)
   {
      const long strBufLen = 8;
      char strBuf[strBufLen];
      DWORD aByteLengths[] = {4,2,2,2,6};

      *strDest = NULL;
      for (uint i=0, dwAccum=0; i < 5; dwAccum += aByteLengths[i], i++)
      {
         for (uint j=0; j < aByteLengths[i]; j++)
         {
            StringCchPrintfA(strBuf, strBufLen, "%02x", (uint)src[j+dwAccum]);
            StringCchCatA(strDest, destBufLen, strBuf);
         }
      }
   }

   //==============================================================================
   // 
   //==============================================================================
   uint32 calcDistance(uint16 seqNum1, uint16 seqNum2)
   {
      if (seqNum2 > seqNum1)
         return seqNum2 - seqNum1;         
      else
         return (65535 - seqNum1) + seqNum2;
   }

   //==============================================================================
   // 
   //==============================================================================
   HRESULT allocateSendBuffer(IN DWORD MaximumLength, OUT BUDPConnectionSendBuffer** ReturnSendBuffer)
   {
      BASSERT (ReturnSendBuffer != NULL);
      *ReturnSendBuffer = NULL;

      DWORD AllocationLength;
      BUDPConnectionSendBuffer * SendBuffer;

      AllocationLength = MaximumLength + sizeof(BUDPConnectionSendBuffer) + sizeof(BUDPConnectionHeader);

      // allocate this memory from the gNetworkHeap
      LPBYTE MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
      if (MemoryBlock == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
         return E_OUTOFMEMORY;
      }

      SendBuffer = reinterpret_cast<BUDPConnectionSendBuffer*>(MemoryBlock);
      SendBuffer->Header = reinterpret_cast<BUDPConnectionHeader*>(MemoryBlock + sizeof(BUDPConnectionSendBuffer));
      SendBuffer->Buffer = MemoryBlock + sizeof(BUDPConnectionSendBuffer) + sizeof(BUDPConnectionHeader);

      SendBuffer->Flags = 0;
      SendBuffer->MaximumLength = MaximumLength;
      SendBuffer->Length = 0;
      SendBuffer->VoiceLength = 0;
      SendBuffer->SendTime = timeGetTime();

      *ReturnSendBuffer = SendBuffer;

      return S_OK;
   }

   //==============================================================================
   // 
   //==============================================================================
   HRESULT freeSendBuffer(IN BUDPConnectionSendBuffer* SendBuffer)
   {
      if (SendBuffer != NULL)
      {
         gNetworkHeap.Delete(SendBuffer);
      }

      return S_OK;
   }

   //==============================================================================
   // 
   //==============================================================================
   HRESULT allocateRecvBuffer(OUT BUDPConnectionRecvBuffer** ReturnBuffer, DWORD size)
   {
      BASSERT(ReturnBuffer != NULL);

      DWORD AllocationLength = sizeof(BUDPConnectionRecvBuffer) + size;

      LPBYTE MemoryBlock = static_cast<LPBYTE>(gNetworkHeap.New(AllocationLength));
      if (MemoryBlock == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
         return E_OUTOFMEMORY;
      }

      *ReturnBuffer = reinterpret_cast<BUDPConnectionRecvBuffer*>(MemoryBlock);   
      (*ReturnBuffer)->Buffer = MemoryBlock + sizeof(BUDPConnectionRecvBuffer);

      return S_OK;
   }

   //==============================================================================
   // 
   //==============================================================================
   HRESULT freeRecvBuffer(IN BUDPConnectionRecvBuffer* Buffer)
   {
      if (Buffer != NULL)
         gNetworkHeap.Delete(Buffer);

      return S_OK;
   }

}

//==============================================================================
// Defines
uint8 BUDP360Connection::mNumActiveConnections = 0;
BUDP360Connection* BUDP360Connection::mpActiveConnections[BUDP360Connection::cMaxActiveConnections];

//==============================================================================
//
//==============================================================================
BUDP360Connection::BUDP360Connection(IN BConnObserver* Observer) :
   mState(cDisconnected),
   mRemoteSocketID(0), 
   mLocalSocketID(0),
   mConnectingTimer(0),
   mResendRequestRatePerSec(cDefaultResendRequestRatePerSec),
   mLastRecvDataTime(0),
   mDisconnecting(FALSE),
   #ifndef BUILD_FINAL
   m_dwByteReceiveCount(0),
   m_dwPacketReceiveCount(0),
   m_dwPacketResendRequestCount(0),
   m_dwInSequencePacketReceiveCount(0),
   m_dwOutOfSequencePacketReceiveCount(0),
   m_dwPacketOneSecondDeltaBegin(0),
   m_dwPacketOneSecondDeltaEnd(0),   
   #endif
   mRecvFragmentBufferPtr(0),
   m_dwUnresponsiveCount(0),
   m_dwUnresponsiveTimeoutValue(cUnresponsiveTimeoutValue),
   mLastUnresponsiveTime(0),
   mLastResponsiveTime(0),
   mResponsive(TRUE),
   mSentData(FALSE),
   mBufferSet(cPrimaryBuffer),
   mFlushSendBuffers(FALSE),
   mIncludeResendRequest(FALSE),
   mSocket(NULL),
   mUDP360Socket(NULL),
   mActiveTimer(cTimerNone),
   mObserver(Observer),
   mXNKeyRegistered(FALSE),
   mBoundToLoopbackSocket(FALSE),
   mStopAcks(false),
   mPort(0)
{

   if (!gConfig.get(cConfigSocketUnresponsiveTimeout, reinterpret_cast<long*>(&m_dwUnresponsiveTimeoutValue)))
      m_dwUnresponsiveTimeoutValue = cUnresponsiveTimeoutValue;

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigFlushSendBuffers))
   {
      mFlushSendBuffers = TRUE;
      mIncludeResendRequest = TRUE;
   }
#endif

   Utils::FastMemSet(mDataIndexes, 0, sizeof(mDataIndexes));
   Utils::FastMemSet(mVoiceIndexes, 0, sizeof(mVoiceIndexes));

   Utils::FastMemSet(mDataBuffers, 0, sizeof(mDataBuffers));
   Utils::FastMemSet(mVoiceBuffers, 0, sizeof(mVoiceBuffers));

   Utils::FastMemSet(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));
   Utils::FastMemSet(&mRemoteXNADDR, 0, sizeof(XNADDR));
   Utils::FastMemSet(&mCommKey, 0, sizeof(XNKID));
}

//==============================================================================
// Connect to a 360 that has already sent me data - and thus I know his
//    secure translated IN_ADDR already
//==============================================================================
HRESULT BUDP360Connection::connect(BUDP360Socket* socket, const SOCKADDR_IN& targetSOCKADDR, XNKID* key)
{
   nlog(cTransportNL, "BUDP360Connection[%p]::connect via known IN_ADDR", this);

   //Make sure we aren't already going
   if (mState != cDisconnected)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connect - failed, connection already started", this);
      return E_FAIL;
   }

   //Validate the parameters
   BASSERT(socket);
   BASSERT(key);
   if (!socket || !key)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connect - failed, missing parameter", this);
      return E_INVALIDARG;
   }

   //Figure out the XNADDR from the IN_ADDR
   int ret = XNetInAddrToXnAddr(targetSOCKADDR.sin_addr, &mRemoteXNADDR, &mCommKey);
   if (ret != 0)
  {
      //Binding failed
      nlog( cTransportNL, "BUDP360Connection::connect - XNADDR/XNKEY binding failed.[%d]",ret);
      return E_FAIL;
   }

   //Save locally the remote security encoded IP
   Utils::FastMemCpy(&mRemoteAddress, &targetSOCKADDR, sizeof(SOCKADDR_IN));
   nlog(cTransportNL, "BUDP360Connection::connect - Valid registered IP is %s", inet_ntoa(mRemoteAddress.sin_addr));
   mXNKeyRegistered = TRUE;

   //Little sanity check here
   //todo - check mCommKey versus key - they SHOULD be the same

   //Register so that this IP gets the callbacks
   if (!socket->addIPMappedObserver(&mRemoteAddress.sin_addr, this))
   {
      nlog( cTransportNL, "BUDP360Connection::connect - addIPMappedObserver failed");
      dispose();
      return E_FAIL;
   }

   //Store locals and state
   addToActiveConnections();
   mSocket = socket;
   mUDP360Socket = socket;
   mState = cXNetPreConnect;

   mActiveTimer = cConnectTimer;
   mTimerInterval = cConnectTimerInterval;
   mTimerModifier = 0;
   mTimerLastTick = GetTickCount();
   mConnectingTimer = timeGetTime();

   mRemoteSocketID = 0;   
   mLocalSeqNum = mRemoteSeqNum = 0;
   mLocalSocketID = static_cast<unsigned short>(timeGetTime()+rand());  

   allocateDataVoiceBuffers();

   //See if we can connect to that target with that key
   //Note - moved this to the after all the internal vars are set up because it may fail right off, or later - in either case there is 
   //  lots of stuff that needs to get cleaned up correct (socket->mappedIP for one) and unless everything is set correctly - then those things 
   //  do NOT cleanup right.  Eric
   if (XNetConnect(targetSOCKADDR.sin_addr) != 0)
   {
      //XNetconnect immediately failed - odd
      nlog( cTransportNL, "BUDP360Connection::connect - XNetConnect failed immediately");
      dispose();
      return E_FAIL;
   }

   return S_OK;

}

//==============================================================================
// Normal connect to a remote target 360 through an XNADDR
//==============================================================================
HRESULT BUDP360Connection::connect(BUDP360Socket* socket, const XNADDR& targetXNAddr, WORD port, XNKID* key)
{
   nlog(cTransportNL, "BUDP360Connection[%p]::connect via XNADDR", this);

   //Make sure we aren't already going
   if (mState != cDisconnected)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connect - failed, connection already started", this);
      return E_FAIL;
   }

   //Validate the parameters
   BASSERT(socket);
   BASSERT(key);
   if (!socket || !key)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connect - failed, missing parameter", this);
      return E_INVALIDARG;
   }

   //Bind the key/xnaddr to get a target ip
   Utils::FastMemSet(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));
   if (XNetXnAddrToInAddr(&targetXNAddr, key, &mRemoteAddress.sin_addr) != 0)
   {
      //Binding failed
      nlog( cTransportNL, "BUDP360Connection::connect - XNADDR/XNKEY binding failed.");
      return E_FAIL;
   }

   nlog(cTransportNL, "BUDP360Connection::connect - XNetInAddrToXnAddr registered %s", inet_ntoa(mRemoteAddress.sin_addr));
   mXNKeyRegistered = TRUE;

   //Register so that this IP gets the callbacks
   if (!socket->addIPMappedObserver(&mRemoteAddress.sin_addr, this))
   {
      nlog( cTransportNL, "BUDP360Connection::connect - addIPMappedObserver failed");
      dispose();
      return E_FAIL;
   }

   //See if we can connect to that target with that key
   if (XNetConnect(mRemoteAddress.sin_addr) != 0)
   {
      //XNetconnect immediately failed - odd
      nlog( cTransportNL, "BUDP360Connection::connect - XNetConnect failed immediately");
      dispose();
      return E_FAIL;
   }

   //Store locals and state
   addToActiveConnections();
   Utils::FastMemCpy(&mRemoteXNADDR, &targetXNAddr, sizeof(XNADDR));
   Utils::FastMemCpy(&mCommKey, key, sizeof(XNKID));
   mRemoteAddress.sin_family = AF_INET;
   mRemoteAddress.sin_port = port;

   mPort = port;
   mSocket = socket;
   mUDP360Socket = socket;
   mState = cXNetPreConnect;

   mActiveTimer = cConnectTimer;
   mTimerInterval = cConnectTimerInterval;
   mTimerModifier = 0;
   mTimerLastTick = GetTickCount();
   mConnectingTimer = timeGetTime();

   mRemoteSocketID = 0;
   mLocalSeqNum = mRemoteSeqNum = 0;
   mLocalSocketID = static_cast<uint16>(timeGetTime() + rand());

   allocateDataVoiceBuffers();

   return S_OK;
}

//==============================================================================
// Connect to a local loopback socket
//==============================================================================
HRESULT BUDP360Connection::connect(BLoopbackSocket* socket)
{
   nlog(cTransportNL, "BUDP360Connection[%p]::connect via loopback socket", this);

   //Make sure we aren't already going
   if (mState != cDisconnected)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connect - failed, connection already started", this);
      return E_FAIL;
   }
   
   //Hook up to this loopback
   addToActiveConnections();
   mSocket = socket;
   mBoundToLoopbackSocket = TRUE;
   mXNKeyRegistered = FALSE;
   socket->setObserver(this);
   mState = cConnected;

   return S_OK;
}

//==============================================================================
// Returns true if we are in a connection pending state, and if this XNADDR
//    is the same as ours
//==============================================================================
bool BUDP360Connection::pendingXNAddrIsSameAs(XNADDR* checkXNAddr)
{
   if (mState != cConnectPending)
   {
      return false;
   }

   return (memcmp(&mRemoteXNADDR, checkXNAddr, sizeof(XNADDR)) == 0);
}

//==============================================================================
// Use this when you have an XNADDR, and have been told by the host to connect,
//    but you are NOT initialing the connection
//==============================================================================
HRESULT BUDP360Connection::connectPending(BUDP360Socket* socket, const XNADDR& targetXNAddr, WORD port, XNKID* key)
{
   nlog(cTransportNL, "BUDP360Connection[%p]::connectPending via XNADDR", this);

   //Make sure we aren't already going
   if (mState != cDisconnected)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connectPending - failed, connection already started", this);
      return E_FAIL;
   }

   //Validate the parameters
   BASSERT(socket);
   BASSERT(key);
   if (!socket || !key)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::connectPending - failed, missing parameter", this);
      return E_INVALIDARG;
   }

   //Store locals and state
   Utils::FastMemSet(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));
   addToActiveConnections();
   mXNKeyRegistered = FALSE;
   Utils::FastMemCpy(&mRemoteXNADDR, &targetXNAddr, sizeof(XNADDR));
   Utils::FastMemCpy(&mCommKey, key, sizeof(XNKID));

   mPort = port;
   mSocket = socket;
   mUDP360Socket = socket;  
   mState = cConnectPending;

   mActiveTimer = cConnectTimer;
   mTimerInterval = cConnectTimerInterval;
   mTimerModifier = 0;
   mTimerLastTick = GetTickCount();
   mConnectingTimer = timeGetTime();

   allocateDataVoiceBuffers();

   return S_OK;
}

//==============================================================================
// Call this when you have a completion for a pending connection
//==============================================================================
HRESULT BUDP360Connection::establishPendingConnection(const SOCKADDR_IN& targetSOCKADDR)
{
   nlog(cTransportNL, "BUDP360Connection[%p]::establishPendingConnection via XNADDR", this);

   //Make sure we aren't already going
   if (mState != cConnectPending)
   {
      nlog(cTransportNL, "BUDP360Connection[%p]::establishPendingConnection - failed, connection was NOT pending", this);
      return E_FAIL;
   }

   //Bind the key/xnaddr to get a target ip
   Utils::FastMemCpy(&mRemoteAddress, &targetSOCKADDR, sizeof(SOCKADDR_IN));
   nlog(cTransportNL, "BUDP360Connection::establishPendingConnection - secure address %s", inet_ntoa(mRemoteAddress.sin_addr));
   mXNKeyRegistered = TRUE;

   //Register so that this IP gets the callbacks
   if (!mUDP360Socket->addIPMappedObserver(&mRemoteAddress.sin_addr, this))
   {
      nlog(cTransportNL, "BUDP360Connection::connect - addIPMappedObserver failed");
      dispose();
      return E_FAIL;
   }

   //Store locals and state   
   mRemoteAddress.sin_family = AF_INET;

   mState = cConnecting;

   mActiveTimer = cConnectTimer;
   mTimerInterval = cConnectTimerInterval;
   mTimerModifier = 0;
   mTimerLastTick = GetTickCount();
   mConnectingTimer = timeGetTime();

   mRemoteSocketID = 0;   
   mLocalSeqNum = mRemoteSeqNum = 0;
   mLocalSocketID = static_cast<uint16>(timeGetTime()+rand());  

   return S_OK;
}

//==============================================================================
// Connection to our LSP
//==============================================================================
HRESULT BUDP360Connection::connect(BUDP360Socket* pSocket, const IN_ADDR& lspAddr, ushort port)
{
   Utils::FastMemSet(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));

   mRemoteAddress.sin_family = AF_INET;
   mRemoteAddress.sin_addr   = lspAddr;
   mRemoteAddress.sin_port   = htons(port);

   mXNKeyRegistered = FALSE;

   //Register so that this IP gets the callbacks
   if (!pSocket->addIPMappedObserver(&mRemoteAddress.sin_addr, this))
   {
      nlog(cTransportNL, "BUDP360Connection::connect - addIPMappedObserver failed");
      dispose();
      return E_FAIL;
   }

   //Store locals and state
   addToActiveConnections();

   mPort = port;
   mSocket = pSocket;
   mUDP360Socket = pSocket;
   mState = cConnected;

   mActiveTimer = cTimerNone;
   mTimerInterval = 0;
   mTimerModifier = 0;
   mTimerLastTick = 0;
   mConnectingTimer = 0;

   mRemoteSocketID = 0;
   mLocalSeqNum = mRemoteSeqNum = 0;
   mLocalSocketID = static_cast<uint16>(timeGetTime() + rand());

   allocateDataVoiceBuffers();

   // fake out our responsiveness
   updateResponsiveness();

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::disconnected()
{
   nlog(cTransportNL, "BUDP360Connection::disconnect");
   nlog(cPerfNL, "BUDP360Connection::disconnect");

   mActiveTimer = cTimerNone;
   mTimerInterval = 0;
   mTimerModifier = 0;
   mTimerLastTick = 0;
   mState = cDisconnected;

   if (getObserver())
   {
      getObserver()->disconnected(this);
   }

   dispose();
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::disconnect()
{
   nlog(cTransportNL, "BUDP360Connection::disconnect");
   nlog(cPerfNL, "BUDP360Connection::disconnect");
   
   dispose();

   if (getObserver())
   {
      getObserver()->disconnected(this);
   }
}

//==============================================================================
// This method actually takes the buffer and sends it on the bound socket out
//    to the remote target
//==============================================================================
HRESULT BUDP360Connection::socketLevelSend(BSendBuffer* buf)
{
   if (mBoundToLoopbackSocket)
   {
      //Call the loopback socket send to itself
      return getSocket()->send(buf);
   }
   else
   {
      //Otherwise have the bound socket it send it to our "connected" target
      return mUDP360Socket->sendTo(buf, &mRemoteAddress);
   }
}

//==============================================================================
// 
//==============================================================================
BUDP360Connection::~BUDP360Connection()
{
   dispose();
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::dispose()
{
   nlog(cTransportNL, "BUDP360Connection[%p]::dispose", this);
   mDisconnecting = TRUE;

   //No more updates - take me out of the static update list
   removeFromActiveConnections();

   if (mUDP360Socket && (mRemoteAddress.sin_addr.s_addr != NULL))
   {
      BASSERT(!mBoundToLoopbackSocket);
      mUDP360Socket->removeIPMappedObserver(&mRemoteAddress.sin_addr);
   }

   HRESULT hr;
   if (getSocket() && isConnected())
   {
      for (uint i=0; i < cDisconnectResendAmount; i++)
      {
         BSendBuffer* buf;
         getSocket()->sendAllocateBuffer(3, &buf);
         BYTE b = BUDPConnectionHeaderType::cDisconnect;
         uchar* ptr = static_cast<uchar*>(buf->Buffer);
         Utils::FastMemCpy(ptr, &b, 1);
         Utils::FastMemCpy(ptr+1, &mLocalSocketID, sizeof(uint16));
         buf->Length = 3;
         buf->VoiceLength = 0;
         hr = socketLevelSend(buf);
         if (hr!=S_OK)
            break;
      }
      DWORD now = timeGetTime();
      while (timeGetTime() - now < cDisconnectSleepTimer); // Don't sleep, coz we don't want events firing off      
   }

   BHandle item;
   BUDPConnectionSendBuffer* send = mSentList.getHead(item);
   hr = E_FAIL;
   while (send)
   {
      hr = freeSendBuffer(send);
      BASSERT(SUCCEEDED(hr));
      send = mSentList.removeAndGetNext(item);
   }
   mSentList.reset();

   BUDPConnectionRecvBuffer* recv = mRcvdList.getHead(item);
   while (recv)
   {
      hr = freeRecvBuffer(recv);
      BASSERT(SUCCEEDED(hr));
      recv = mRcvdList.removeAndGetNext(item);
   }
   mRcvdList.reset();

   for (uint i=0; i < cMaxBuffers; i++)
   {
      if (mDataBuffers[i])
         gNetworkHeap.Delete(mDataBuffers[i]);

      mDataIndexes[i] = 0;
      mDataBuffers[i] = NULL;

      if (mVoiceBuffers[i])
         gNetworkHeap.Delete(mVoiceBuffers[i]);

      mVoiceIndexes[i] = 0;
      mVoiceBuffers[i] = NULL;
   }

   //If this is the loop back socket - then we own it - release it as well
   if (mBoundToLoopbackSocket && mSocket)
   {
      mSocket->setObserver(NULL);
      mSocket->dispose();
      mSocket = NULL;
   }

   //Unregister the key association if it is registered
   if (mXNKeyRegistered)
   {
      nlog( cTransportNL, "BUDP360Connection::dispose - using XNetUnregisterInAddr to unregister %s", inet_ntoa(mRemoteAddress.sin_addr));
      if (XNetUnregisterInAddr(mRemoteAddress.sin_addr)!= 0)
      {
         //Unreg failed - lets spam out the status code for debugging purposes
         DWORD connStatus = XNetGetConnectStatus(mRemoteAddress.sin_addr);
         nlog( cTransportNL, "BUDP360Connection::dispose - XNetUnregisterInAddr failed, current status code %d.", connStatus);
      }
      mXNKeyRegistered = FALSE;
   }

   //Some clean up
   mDisconnecting = FALSE;
   mBoundToLoopbackSocket = FALSE;
   mActiveTimer = cTimerNone;
   mTimerInterval = 0;
   mTimerModifier = 0;
   mTimerLastTick = 0;

   memset(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));   
   memset(&mRemoteXNADDR, 0, sizeof(XNADDR));
   memset(&mCommKey, 0, sizeof(XNKID));

   mState = cDisconnected;

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::service()
{
   if (mTimerInterval && mActiveTimer)
   {
      const DWORD curTickCount = GetTickCount();

      if (mTimerInterval)
      {
         if ((curTickCount - mTimerLastTick) >= (mTimerInterval + mTimerModifier))
         {
            mTimerLastTick = curTickCount;
            tic(mActiveTimer);
         }
      }
   }

   //Call every frame to flush the buffers
   sendWithPiggyBacks(0, TRUE, FALSE);
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::tic(DWORD timerID)
{
   if (mState == cDisconnected)
      return;

   if (timerID == cUpdateTimer)
   {
      HRESULT hr = sendWithPiggyBacks(0, TRUE, TRUE); // send just any resend requests we have
      if(FAILED(hr))
         nlog(cTransportNL, "BUDP360Connection::tic -- failed sendWithPiggyBacks.");

      const long cNotifyUnresponsiveInterval = 1000;
      const long cNotifyResponsiveInterval = 2000;

      if (isConnected())
      {
         DWORD time = timeGetTime();

         // if we aren't responding
         if ((time - mLastRecvDataTime >= m_dwUnresponsiveTimeoutValue))
         {
            // and the notify interval has lapsed
            if (time - mLastUnresponsiveTime >= cNotifyUnresponsiveInterval)
            {
               nlog(cTransportNL, "      notResponding SOCKET [%p]", this);

               mLastUnresponsiveTime = time;
               // have we not heard anything in cUnresponsiveTimeout time? notify
               if (getObserver())
               {
                  mResponsive = FALSE;
                  if (getObserver()->notifyUnresponsiveConnection(this, mLastRecvDataTime))
                  {
                     if (getObserver())
                     {
                        getObserver()->disconnected(this);
                     }
                     mState = cDisconnected;
                     nlog(cTransportNL, "BUDP360Connection[%p] set state to %ld because it was unresponsive", this, mState);
                  }
               }
               else if (time - mLastRecvDataTime > cAckTimeout)
               {
                  // have we not heard anything in cAckTimeout time? if so, we're linkdead
                  if (getObserver())
                  {
                     getObserver()->disconnected(this);
                  }
                  mState = cDisconnected;
                  nlog(cTransportNL, "BUDP360Connection[%p] set state to %ld because of ack timeout", this, mState);
               }
            }
         }
         // else we are responding, has the notify interval lapsed?
         else if ((time - mLastResponsiveTime) > cNotifyResponsiveInterval)
         {
            if (getObserver())
            {
               getObserver()->notifyResponsiveConnection(this, mLastRecvDataTime);
            }

            mLastResponsiveTime = time;
         }
      }
   }
   else if (timerID == cConnectTimer)
   {
      if (mState == cXNetPreConnect)
      {
         //We are waiting to see if the 360 network system was able to securely connect to the target
         DWORD stat = XNetGetConnectStatus(mRemoteAddress.sin_addr);
         if (stat == XNET_CONNECT_STATUS_CONNECTED)
         {
            //We are now connected according to the Live layer, start our original handshake off
            mState = cConnecting;
            mConnectingTimer = timeGetTime();         //Refresh the connect timer since we've had a response
         }
         else
         {
            if (stat != XNET_CONNECT_STATUS_PENDING)
            {
               //The system gave up on this connection ever working
               nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - XNetGetConnectStatus gave up", this);
               if (getObserver())
               {
                  getObserver()->connectTimeout(this);
               }
               dispose();
               return;
            }
         }
      }
      if ((mState == cConnecting) && getSocket())
      {         
         BSendBuffer* buf=0;         
         BUDPConnectionConnectHeader header;
         header.Type = BUDPConnectionHeaderType::cHello;
         header.LocalSocketID = mLocalSocketID;

         if (FAILED(getSocket()->sendAllocateBuffer(sizeof(header), &buf)))
         {
            nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - could not allocated memory to say hello", this);
            if (getObserver())
            {
               getObserver()->connectTimeout(this);
            }
            dispose();
            return;
         }

         Utils::FastMemCpy(buf->Buffer, &header, sizeof(header));
         buf->Length = sizeof(header);
         buf->VoiceLength = 0;

         const long bufSize = 64;
         char a1[bufSize];
         char a2[bufSize];
         char a3[bufSize];

         StringCchCopyNExA(a1, bufSize, inet_ntoa(mRemoteXNADDR.ina), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a1[63] = 0;
         StringCchCopyNExA(a2, bufSize, inet_ntoa(mRemoteXNADDR.inaOnline), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a2[63] = 0;
         StringCchCopyNExA(a3, bufSize, inet_ntoa(mRemoteAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a3[63] = 0;

         nlog(cTransportNL, "      BUDP360Connection SOCKET [%p] Sending hello to LocalAddress %s/%s:%ld (%s:%d), localSocketID %ld", this,
            a1, a2, htons(mRemoteXNADDR.wPortOnline), a3, mRemoteAddress.sin_port, mLocalSocketID);

         nlog(cPerfNL, "BUDP360Connection[%p]::tic sending hello", this);

         if (FAILED(socketLevelSend(buf)))
         {
            nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - could not send hello", this);
            if (getObserver())
            {
               getObserver()->connectTimeout(this);
            }
            dispose();
            return;
         }
      }
      else if ((mState == cConnectVerification) && getSocket())
      {         
         BSendBuffer* buf=0;         
         BUDPConnectionConnectHeader header;
         header.Type = BUDPConnectionHeaderType::cConnectVerification;
         header.LocalSocketID = mLocalSocketID;
         header.RemoteSocketID = mRemoteSocketID;

         if (FAILED(getSocket()->sendAllocateBuffer(sizeof(header), &buf)))
         {
            nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - could not allocat memory for cConnectVerification", this);
            if (getObserver())
            {
               getObserver()->connectTimeout(this);
            }
            dispose();
            return;
         }

         Utils::FastMemCpy(buf->Buffer, &header, sizeof(header));
         buf->Length = sizeof(header);
         buf->VoiceLength = 0;

         const long bufSize = 64;
         char a1[bufSize];
         char a2[bufSize];
         char a3[bufSize];

         StringCchCopyNExA(a1, bufSize, inet_ntoa(mRemoteXNADDR.ina), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a1[63] = 0;
         StringCchCopyNExA(a2, bufSize, inet_ntoa(mRemoteXNADDR.inaOnline), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a2[63] = 0;
         StringCchCopyNExA(a3, bufSize, inet_ntoa(mRemoteAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a3[63] = 0;

         nlog(cTransportNL, "      BUDP360Connection SOCKET [%p] Sending cConnectVerification to LocalAddress %s/%s:%d (%s:%d), localSocketID %ld", this,
            a1, a2, htons(mRemoteXNADDR.wPortOnline), a3, mRemoteAddress.sin_port, mLocalSocketID);

         nlog(cPerfNL, "BUDP360Connection[%p]::tic sending cConnectVerification", this);

         if (FAILED(socketLevelSend(buf)))
         {
            nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - could not send cConnectVerification", this);
            if (getObserver())
            {
               getObserver()->connectTimeout(this);
            }
            dispose();
            return;
         }
      }
         
      //Check for timeout on connect
      const long cConnectTimeout = 120000;     //TODO move this into a config setting
      //const long cConnectTimeout = 30*60*1000;
      if (isConnecting() && (timeGetTime() - mConnectingTimer > cConnectTimeout))
      {
         //We have waited long enough - give up on this connection ever establishing
         nlog(cPerfNL, "BUDP360Connection[%p]::tic - Ditching connection attempt - ran out of time", this);
         if (getObserver())
         {
            getObserver()->connectTimeout(this);
         }
         dispose();
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::sendAllocateBuffer(IN DWORD MaximumLength, OUT BSendBuffer** ReturnSendBuffer)
{
   return allocateSendBuffer(MaximumLength, reinterpret_cast<BUDPConnectionSendBuffer**>(ReturnSendBuffer));
}

//==============================================================================
// 
//==============================================================================
bool BUDP360Connection::recvHello(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN* RemoteAddress)
{
   nlog(cTransportNL, "      BUDP360Connection::recvHello");

   if (Length == sizeof(BUDPConnectionConnectHeader))
   {
      // send a response
      BUDPConnectionConnectHeader header;
      header.Type = BUDPConnectionHeaderType::cHelloResponse;
      header.LocalSocketID = mLocalSocketID;

      const BUDPConnectionConnectHeader* const rheader = static_cast<const BUDPConnectionConnectHeader*>(Buffer);
      header.RemoteSocketID = rheader->LocalSocketID;

      nlog(cTransportNL, "        BUDP360Connection SOCKET [%p] Hello request from %ld %s:%ld, we are %ld", this, rheader->LocalSocketID, inet_ntoa(RemoteAddress->sin_addr), htons(RemoteAddress->sin_port), mLocalSocketID);

      if ((mRemoteSocketID != 0) && (mRemoteSocketID != header.RemoteSocketID))
      {
         nlog(cTransportNL, "          rejecting: mRemoteSocketID %ld, headerRemoteSocketID %ld", mRemoteSocketID, header.RemoteSocketID);
         return false;
      }   

      mRemoteSocketID = rheader->LocalSocketID;
      mConnectingTimer = timeGetTime();

      nlog(cPerfNL, "BUDP360Connection[%p]::recvHello sending response", this);
      nlog(cTransportNL, "        sending hello response to localSocketID %ld, remoteSocketID %ld", header.LocalSocketID, header.RemoteSocketID);

      BSendBuffer* buf;
      HRESULT hr = Socket->sendAllocateBuffer(sizeof(BUDPConnectionConnectHeader), &buf);
      hr;
      BASSERT(SUCCEEDED(hr));
      Utils::FastMemCpy(buf->Buffer, &header, sizeof(header));
      buf->Length = sizeof(header);
      buf->VoiceLength = 0;

      socketLevelSend(buf);

      return true;
   }
   else 
   {
      nlog(cTransportNL, "        bad size");
      return false;
   }
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::updateResponsiveness()
{   
   mLastRecvDataTime = timeGetTime();

   if (!mResponsive)
   {
      mResponsive = TRUE;
   }
}

//==============================================================================
// 
//==============================================================================
bool BUDP360Connection::recvHelloResponse(IN BSocket*, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN*)
{
   nlog(cTransportNL, "      BUDP360Connection::recvHelloResponse");

   if (Length == sizeof(BUDPConnectionConnectHeader))
   {
      const BUDPConnectionConnectHeader * const header = (BUDPConnectionConnectHeader *)Buffer;      
      //Do we need this AT ALL with the new routing system?
      //  Yes! Because it identifies unique connections from the same source
      //  IE: If i connect, send stuff on the wire, disconnect, reconnect - all that previous data on the wire needs to be ignored - I'm a new connection
      // is remote guy talking to us?
      if (header->RemoteSocketID != mLocalSocketID)
      {
         //This is not who we thought it should be - drop this connection
         nlog(cTransportNL, "        rejecting hello response because remotesocketID %ld, localsocketID %ld", header->RemoteSocketID, mLocalSocketID);
         return false;
      }

      mRemoteSocketID = header->LocalSocketID;
      mState = cConnectVerification;
      nlog(cTransportNL, "BUDP360Connection[%p] set state to cConnectVerification", this);

      updateResponsiveness();
      return true;

   }  
   else
   {
      nlog(cTransportNL, "        rejecting hello response because length %ld", Length);
      return false;
   }
}

//==============================================================================
// 
//==============================================================================
bool BUDP360Connection::recvConnectionVerificationResponse(IN BSocket*, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN*)
{
   nlog(cTransportNL, "      BUDP360Connection::recvConnectionVerificationResponse");

   if (Length == sizeof(BUDPConnectionConnectHeader))
   {
      BUDPConnectionConnectHeader *header = (BUDPConnectionConnectHeader *)Buffer;      
      //Verify the header's remote socket ID again just to be safe
      if (header->RemoteSocketID != mLocalSocketID)
      {
         //This is not who we thought it should be - drop this connection
         nlog(cTransportNL, "        rejecting connection verification because remotesocketID %ld, localsocketID %ld", header->RemoteSocketID, mLocalSocketID);
         return false;
      }

      mState = cConnected;
      nlog(cTransportNL, "BUDP360Connection[%p] set state to cConnected", this );
      mConnectingTimer=0;

      if (mActiveTimer == cConnectTimer)
      {
         mActiveTimer = cUpdateTimer;
         mTimerInterval = cUpdateTimerInterval;
      }       

      updateResponsiveness();

      nlog(cPerfNL, "BUDP360Connection[%p]::recvConnectionVerificationResponse connected", this);

      //Send back a connection verification msg - reliably so we know they will get it
      BSendBuffer* buf=0;

      BUDPConnectionConnectHeader outHeader;
      outHeader.Type = BUDPConnectionHeaderType::cConnectVerification;
      outHeader.LocalSocketID = mLocalSocketID;
      outHeader.RemoteSocketID = mRemoteSocketID;

      if (FAILED(getSocket()->sendAllocateBuffer(sizeof(outHeader), &buf)))
      {
         nlog(cPerfNL, "BUDP360Connection[%p]::recvConnectionVerificationResponse - could not allocate memory to send connection verification", this);
         return false;
      }

      Utils::FastMemCpy(buf->Buffer, &outHeader, sizeof(outHeader));
      buf->Length = sizeof(outHeader);
      buf->VoiceLength = 0;

      const long bufSize = 64;
      char a1[bufSize];
      char a2[bufSize];
      char a3[bufSize];
      StringCchCopyNExA(a1, bufSize, inet_ntoa(mRemoteXNADDR.ina), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
      a1[63] = 0;
      StringCchCopyNExA(a2, bufSize, inet_ntoa(mRemoteXNADDR.inaOnline), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
      a2[63] = 0;
      StringCchCopyNExA(a3, bufSize, inet_ntoa(mRemoteAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
      a3[63] = 0;
      nlog(cTransportNL, "      BUDP360Connection SOCKET [%p] Sending cConnectVerification to LocalAddress %s/%s:%d (%s:%d), localSocketID %ld", this,
         a1, a2, htons(mRemoteXNADDR.wPortOnline), a3, mRemoteAddress.sin_port, mLocalSocketID);
      nlog(cPerfNL, "BUDP360Connection[%p]::recvConnectionVerificationResponse sending cConnectVerification", this);

      if (FAILED(socketLevelSend(buf)))
      {
         nlog(cPerfNL, "BUDP360Connection[%p]::recvConnectionVerificationResponse - could not allocate memory to send connection verification", this);
         return false;
      }

      //Notify my observer that this connection is connected
      if (getObserver())
      {
         getObserver()->connected(this);
      }
      return true;
   }

   nlog(cTransportNL, "        rejecting connection verification response because length %ld", Length);
   return false;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::clearSentList(uint16 ack)
{
   nlog(cTransportNL, "      clearSentList ack %ld, mLocalSeqNum %ld", ack, mLocalSeqNum);

   // the ack given here is sent from the client and is the client's
   // tracking value for our next expected sequence number to be received,
   // so if I haven't sent anything, the other client will send me my
   // mLocalSeqNum value, since mLocalSeqNum is also the next expected seq num that I will send
   //
   // if the ack == mLocalSeqNum, then we haven't sent anything and the remote client has
   // received everything that we've sent so far
   if (ack == mLocalSeqNum && !mStopAcks)
   {
      mTimerModifier = 5000;
      mStopAcks = true;
   }

   // if the ack matches our local sequence number or the distance is within our queue size
   // that means that we're either getting an ack for previously sent packets or we're at a
   // steady state and are free to clear out the sent list
   if (ack == mLocalSeqNum || calcDistance(ack, mLocalSeqNum) < cQueueSize)
   {
      // if so, then clear out any sent items older than this
      BHandle item;
      HRESULT hr = E_FAIL;
      BUDPConnectionSendBuffer* j = mSentList.getHead(item);
      while(j)
      {
         if (
            (j) && (j)->Header &&
            ((j)->Header->SeqNum != ack) &&
            (calcDistance((j)->Header->SeqNum, ack) < cQueueSize)
            )
         {
            nlog(cTransportNL, "      clearing sent item %ld", (j)->Header->SeqNum);
            hr = freeSendBuffer(j);
            BASSERT(SUCCEEDED(hr));
            j = mSentList.removeAndGetNext(item);
         }
         else
         {
            nlog(cTransportNL, "      skipping clear of sent item %ld", (j)->Header->SeqNum);
            j = mSentList.getNext(item);
         }
      }
   }
}


//==============================================================================
// 
//==============================================================================
bool BUDP360Connection::recvData(IN BSocket*, IN const void * pBuffer, IN DWORD length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* pRemoteAddress)
{
#ifndef BUILD_FINAL
   m_dwPacketReceiveCount++;
#endif

   nlog(cTransportNL, "    BUDP360Connection::recvData %ld", length);

   //Loopback socket doesn't add connection headers in
   if (mBoundToLoopbackSocket)
   {
      updateResponsiveness();
      if (getObserver())
      {
         getObserver()->dataReceived(this, pBuffer, length, pRemoteAddress);
      }
      return true;
   }

   // insure we have enough data for the header
   // Length includes the voiceLength, subtract out to get the actual data length
   if (length < sizeof(BUDPConnectionHeader))
      return false;

   const uchar* ptr = static_cast<const uchar*>(pBuffer);

#ifndef BUILD_FINAL
   const uchar* const dataPtr = ptr;
#endif
   const uchar* const endPtr = ptr+length;

   // the voice data segment will require information from the headers contained in the game data
   // voiceLen will be used to help in boundary checks
   const uchar* const voicePtr = ptr + (length - voiceLength);
   const uchar* const ackPtr = voicePtr - sizeof(int16) - 1; // the trailing ack is 1 byte for the separator and 2 bytes of the value

   // First we go through and parse all the subpackets in the packet
   // We also parse any resend requests that are in here
   // and finally we process any ack attached

   // go through all the sub-packets that are in this packet
   while (ptr < endPtr)
   {
      // are we at the end of the packet where the ack lives?
      if (ptr == ackPtr)
      {
         uint16 ack;
         Utils::FastMemCpy(&ack, ptr+1, sizeof(uint16));
         clearSentList(ack);
         nlog(cTransportNL, "        recvData ack %ld", ack);
         ptr += sizeof(uint16)+1;
      }
      else
      {
         // otherwise it's a full packet of data, parse it            
         const BUDPConnectionHeader* const header = reinterpret_cast<const BUDPConnectionHeader*>(ptr);

         if (header->SocketID != mRemoteSocketID)
         {
            //Socket header ID does not match - drop it
            nlog(cTransportNL, "        DROPPING DATA, socket id does not match, header->SocketID %ld, mRemoteSocketID %ld", header->SocketID, mRemoteSocketID);
            return false;
         }

         nlog(cTransportNL, "        header->SocketID %ld, mRemoteSocketID %ld", header->SocketID, mRemoteSocketID);
         updateResponsiveness();

#ifndef BUILD_FINAL
         if ( m_dwPacketOneSecondDeltaBegin > timeGetTime() ) 
         {
            //
            // reset the counters because of role over
            //
            m_dwByteReceiveCount                 = 0;
            m_dwPacketReceiveCount               = 0;
            m_dwPacketResendRequestCount         = 0;
            m_dwInSequencePacketReceiveCount     = 0;
            m_dwOutOfSequencePacketReceiveCount  = 0;
            m_dwPacketOneSecondDeltaBegin        = timeGetTime();
         }
#endif

         if (getHeaderType(ptr) == BUDPConnectionHeaderType::cResendRequest)
         {
#ifndef BUILD_FINAL
            m_dwPacketResendRequestCount++;
#endif
            BUDPConnectionResendRequest req;
            Utils::FastMemCpy(&req.NumberOfRequests, ptr+sizeof(BUDPConnectionHeader), sizeof(req.NumberOfRequests));
            const uint16 * const requests = reinterpret_cast<const uint16*>(ptr + sizeof(BUDPConnectionHeader) + sizeof(req.NumberOfRequests));
            for (uint i=0; i < req.NumberOfRequests; i++)
            {
               // resend the matching sent item
               nlog(cTransportNL, "        resend requested for %ld", requests[i]);
               BHandle item;
               BUDPConnectionSendBuffer* j = mSentList.getHead(item);
               while (j)
               {
                  if ((j)->Header->SeqNum == requests[i])
                  {
                     nlog(cTransportNL, "        resending %ld", (j)->Header->SeqNum);
                     HRESULT hr = sendWithPiggyBacks(j, mFlushSendBuffers, mIncludeResendRequest);
                     if(FAILED(hr))
                        nlog(cTransportNL, "BUDP360Connection::recvData -- failed sendWithPiggyBacks.");

                     //(*j)->ResendRequestTime = timeGetTime(); // FIXME: piggyback resends also someday!
                     break;
                  }

                  j = mSentList.getNext(item);
               }
            }
         }
         else if ((getHeaderType(ptr) == BUDPConnectionHeaderType::cData) || 
                  (getHeaderType(ptr) == BUDPConnectionHeaderType::cDataFragment) ||
                  (getHeaderType(ptr) == BUDPConnectionHeaderType::cDataFragmentDone))
         {
            nlog(cTransportNL, "        got data packet %ld", header->SeqNum);
            // first see if we have an outstanding resend request for this               
            for (uint i=0; i < mResendRequestVector.getSize(); i++)
            {
               // if so, remove it
               if (mResendRequestVector[i].SeqNum == header->SeqNum)
               {
                  nlog(cTransportNL, "          removing resend request");
                  mResendRequestVector.remove(mResendRequestVector[i]);
                  break;
               }
            }

            if (getNoResends(ptr))
            {
               // the seq number is ignored when issuing noresend requests
               // we also do not support packet fragmentation
               if (getObserver() && getHeaderType(ptr) == BUDPConnectionHeaderType::cData)
               {
                  getObserver()->dataReceived(this, ptr+sizeof(BUDPConnectionHeader), header->Size, pRemoteAddress);
               }
            }
            else if (header->SeqNum == mRemoteSeqNum)
            {
               // we got an in-sequence packet, so just release it
               if (getObserver())
               {
                  if ((getHeaderType(ptr) == BUDPConnectionHeaderType::cDataFragment) ||
                      (getHeaderType(ptr) == BUDPConnectionHeaderType::cDataFragmentDone))
                  {
                     if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < header->Size)
                     {
                        nlog(cTransportNL, "BUDP360Connection::recvData -- not enough buffer space for fragment.");
                        // if we run out of space for this data packet, then the only way to recover is to
                        // re-allocate the fragment buffer, but the game should not be exceeding this
                        // size in the first place!
                        BFAIL("BUDP360Connection::recvData -- Leave this up and call doug @ x275");
                        return false;
                     }

                     Utils::FastMemCpy(mRecvFragmentBuffer + mRecvFragmentBufferPtr, ptr + sizeof(BUDPConnectionHeader), header->Size);
                     mRecvFragmentBufferPtr += header->Size;
                     if (getHeaderType(ptr) == BUDPConnectionHeaderType::cDataFragmentDone)
                     {
                        nlog(cTransportNL, "          in seq %ld, release", header->SeqNum);
                        getObserver()->dataReceived(this, mRecvFragmentBuffer, mRecvFragmentBufferPtr, pRemoteAddress);
                        mRecvFragmentBufferPtr = 0;
                     }
                     else
                        nlog(cTransportNL, "          got a fragment");
                  }
                  else
                  {
                     nlog(cTransportNL, "          in seq %ld, release", header->SeqNum);
                     getObserver()->dataReceived(this, ptr + sizeof(BUDPConnectionHeader), header->Size, pRemoteAddress);
                  }
               }

               mRemoteSeqNum++;

#ifndef BUILD_FINAL                 
               m_dwInSequencePacketReceiveCount++;
#endif                  
               updateRcvdList();
            }
            else 
            {

#ifndef BUILD_FINAL                 
               m_dwOutOfSequencePacketReceiveCount++;
#endif

               uint32 distance = calcDistance(mRemoteSeqNum, header->SeqNum);               

               // is it a message in the future? If so, stuff it into rcvd list
               if (distance < (cQueueSize)-1)
               {
                  nlog(cTransportNL, "          in future");

                  BUDPConnectionRecvBuffer* rbuf=0;
                  HRESULT hr = allocateRecvBuffer(&rbuf, header->Size);
                  if (FAILED(hr))
                     return false;

                  // set up the receive buffer
                  rbuf->RecvTime = timeGetTime();
                  rbuf->SeqNum = header->SeqNum;
                  rbuf->Size = static_cast<uint32>(header->Size);
                  rbuf->Type = getHeaderType(ptr);
                  Utils::FastMemCpy(rbuf->Buffer, ptr + sizeof(BUDPConnectionHeader), header->Size);
                  BASSERT(pRemoteAddress);
                  if (pRemoteAddress)
                     rbuf->RemoteAddress = *pRemoteAddress;                     

                  // push into rcvd list
                  BOOL found = FALSE;
                  BHandle item;
//-- FIXING PREFIX BUG ID 7595
                  const BUDPConnectionRecvBuffer* i = mRcvdList.getHead(item);
//--
                  while (i)
                  {                        
                     if (
                        (calcDistance(header->SeqNum, (i)->SeqNum) < cQueueSize) &&
                        (header->SeqNum < (i)->SeqNum) 
                        )
                     {
                        nlog(cTransportNL, "            inserting before %ld", (i)->SeqNum);
                        mRcvdList.addBefore(rbuf, item);
                        found = TRUE;
                        break;
                     }
                     nlog(cTransportNL, "            skipping %ld", (i)->SeqNum);
                     i = mRcvdList.getNext(item);
                  }
                  if (!found)
                  {
                     nlog(cTransportNL, "            push_back");
                     mRcvdList.addToTail(rbuf);              
                  }

                  // and tell client that we need resends
                  calcResendRequests();
               }
            }
         }
         else if (getHeaderType(ptr) == BUDPConnectionHeaderType::cAck)
         {
            // calculate any resends we may need to request
            calcResendRequests(static_cast<int32>(header->SeqNum));
         }

         // go to next sub-packet
         ptr += sizeof(BUDPConnectionHeader) + header->Size;
      }
#ifndef BUILD_FINAL
      m_dwByteReceiveCount += (ptr - dataPtr);
#endif
   }

   return true;
}

#ifndef BUILD_FINAL

//==============================================================================
// returns the total number of packets that have been received.
//==============================================================================
DWORD BUDP360Connection::getByteReceiveCount() const
{
   return m_dwByteReceiveCount;
}

//==============================================================================
// returns the total number of packets that have been received.
//==============================================================================
DWORD BUDP360Connection::getByteReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if (dwElapsedTime < 1000)
   {
      return 0;
   }
   return m_dwByteReceiveCount / (dwElapsedTime / 1000);
}

//==============================================================================
// returns the total number of packets that have been received.
//==============================================================================
DWORD BUDP360Connection::getPacketReceiveCount() const
{
   return m_dwPacketReceiveCount;
}

//==============================================================================
// returns the number of resend requests made
//==============================================================================
DWORD BUDP360Connection::getPacketResendRequestCount() const
{
   return m_dwPacketResendRequestCount;
}

//==============================================================================
// 
//==============================================================================
DWORD BUDP360Connection::getPacketResendRequestCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if (dwElapsedTime < 1000)
   {
      return m_dwPacketReceiveCount;
   }
   return m_dwPacketResendRequestCount / (dwElapsedTime / 1000);
}

//==============================================================================
// returns the  number of in-sequence packets that were received per second
//==============================================================================
DWORD BUDP360Connection::getInSequencePacketReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if (dwElapsedTime < 1000)
   {
      return m_dwInSequencePacketReceiveCount;
   }
   return m_dwInSequencePacketReceiveCount / (dwElapsedTime / 1000);
}

//==============================================================================
// returns the total number of in-sequence packets received
//==============================================================================
DWORD BUDP360Connection::getInSequencePacketReceiveCount() const 
{
   return m_dwInSequencePacketReceiveCount;
}

//==============================================================================
// returns the total number of in sequence packets received.
//==============================================================================
DWORD BUDP360Connection::getOutOfSequencePacketReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if (dwElapsedTime < 1000)
   {
      return m_dwOutOfSequencePacketReceiveCount;
   }
   return m_dwOutOfSequencePacketReceiveCount / (dwElapsedTime / 1000);
}

//==============================================================================
// 
//==============================================================================
DWORD BUDP360Connection::getOutOfSequencePacketReceiveCount() const 
{
   return m_dwOutOfSequencePacketReceiveCount;
}
#endif

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::recvd(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* RemoteAddress)
{   
   BASSERT(Socket == getSocket());
   BASSERT(Length > 0);

   //Under this new system, no traffic should ever get routed here that doesn't belong to me
   //TODO - but - go ahead and put some sort of sanity BASSERT in here
  // if (RemoteAddress && (RemoteAddress->sin_addr.S_un.S_addr != mRemoteAddress.sin_addr.S_un.S_addr))
  //    return;

   const void * const buf = Buffer;
   DWORD len = Length;

   //Quick out to handle loopback traffic which never has the 360connection header in there
   if (mBoundToLoopbackSocket)
   {
      recvData(Socket, buf, len, voiceLength, RemoteAddress);
      return;
   }

   long headerType = getHeaderType(buf);

#ifdef GUID_TRACKING
   BASSERT(len >= sizeof(GUID));
   len -= sizeof(GUID);

   GUID *g = (GUID *)Buffer;

   const unsigned long length = 256;
   char str[length];
   char *p = (char *)str;
   GUIDtoString((unsigned char *)g, p, length);

   memset(mTempBuffer, 0, sizeof(mTempBuffer));
   buf = (PVOID)mTempBuffer;
   BASSERT(len < sizeof(mTempBuffer));

   nlog(cTransportNL, "    GUID: %s", p);

   Utils::FastMemCpy(buf, ((char *)Buffer)+sizeof(GUID), len);
#endif


   nlog(cTransportNL, "      BUDP360Connection::recvd SOCKET[%p] Length %ld, mState %ld, getHeaderType(buf) %ld, from %s:%ld", this, Length, mState, headerType, inet_ntoa(RemoteAddress->sin_addr), htons(RemoteAddress->sin_port));

   // is this a hello - a connection level handshake?
   if ((mState != cDisconnected) && (headerType == BUDPConnectionHeaderType::cHello))
   {
      recvHello(Socket, buf, len, RemoteAddress);
      return;
   }
   // Is it a hello response - an echo from the other side in response to my hello handshake?
   else if ((mState == cConnecting) && (headerType == BUDPConnectionHeaderType::cHelloResponse))
   {
      recvHelloResponse(Socket, buf, len, RemoteAddress);
      return;
   }
   // Is it a connection verification msg letting me know that the other side has received a hello handshake?
   else if ((mState == cConnectVerification) && (headerType == BUDPConnectionHeaderType::cConnectVerification))
   {
      recvConnectionVerificationResponse(Socket, buf, len, RemoteAddress);
      return;
   }  
   else if ((isConnecting() || isConnected()) && (headerType == BUDPConnectionHeaderType::cDisconnect))
   {
      const uint16* const socketID = reinterpret_cast<const uint16*>(static_cast<const uchar*>(buf) + 1);

      if (*socketID != mRemoteSocketID)
      {
         nlog(cTransportNL, "BUDP360Connection::recvd[%p] dropping data because socket header does not match", this);
         return;
      }

      if (mDisconnecting)
         return;

      mState = cDisconnected;
      nlog(cTransportNL, "BUDP360Connection[%p] set state to %ld because of cDisconnect", this, mState);
      
      if (getObserver())
         getObserver()->disconnected(this); 

      dispose();

      return;
   }
   else if ((mState == cConnected) && 
            (
               (headerType == BUDPConnectionHeaderType::cData) || 
               (headerType == BUDPConnectionHeaderType::cDataFragment) || 
               (headerType == BUDPConnectionHeaderType::cDataFragmentDone) || 
               (headerType == BUDPConnectionHeaderType::cAck) ||
               (headerType == BUDPConnectionHeaderType::cAckRemote) ||
               (headerType == BUDPConnectionHeaderType::cResendRequest)
            )
           )
   {
      recvData(Socket, buf, len, voiceLength, RemoteAddress);
      return;
   }

   return;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::addResendRequest(uint16 curReq)
{
   // do we already have this one?
   uint j;
   for (j=0; j < mResendRequestVector.getSize(); j++)
   {
      if (mResendRequestVector[j].SeqNum == curReq)
         break;
   }

   // if not, add it
   if (j >= mResendRequestVector.getSize())
   {
      mResendRequestVector.add(BResendRequest());
      mResendRequestVector[j].SeqNum = curReq;
      // dpm 11/14/2006 - initializing the SentTime to one second into the future
      // attempting to provide some lag so we don't knee-jerk the resends
      // depending on the amount of traffic, we may have a lot of fragmentation
      // so this helps us wait a second before annoying the sender
      //mResendRequestVector[j].SentTime = 0;
      mResendRequestVector[j].SentTime = timeGetTime() + cResendRequestInterval;
      nlog(cTransportNL, "        adding resend request %ld", curReq);
   }
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::calcResendRequests(int32 actualRemoteSeqNum)
{
   // mRemoteSeqNum is the next sequence number we're expecting
   // and not the last one we've actually received
   uint16 curReq = mRemoteSeqNum-1;
   nlog(cTransportNL, "      calcResendRequests mRemoteSeqNum %ld, actualRemoteSeqNum %ld", mRemoteSeqNum, actualRemoteSeqNum);

   if (actualRemoteSeqNum != -1)
   {
      // if we know the actual remote sequence number, then we request resends for anything we haven't received yet
      // by counting up to that sequence number, and checking the rcvdlist as we go

      // actualRemoteSeqNum is the actual seq number of the last packet sent
      // mRemoteSeqNum is the next seq number I'm expecting
      // compare against mRemoteSeqNum-1
      if (actualRemoteSeqNum == curReq)
         return; // nothing to request

      // if mRemoteSeqNum is 65535 (or something large enough to cause a rollover)
      // but the value passed in is less because of that rollover
      // then we'll never request resends until we're caught up, but if we've
      // unfortunately lost a packet, then we'll never request a resend and then stall
      // and blow up once we rollover
      //
      uint32 distance = 0;
      if ((curReq > actualRemoteSeqNum) && ((curReq - actualRemoteSeqNum) > cQueueSize))
      {
         // we've wrapped, need to compensate
         distance = calcDistance(curReq, static_cast<uint16>(actualRemoteSeqNum));
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
         // do we have this packet?
         BHandle item;
         BUDPConnectionRecvBuffer *rbuf = mRcvdList.getHead(item);
         while (rbuf)
         {
            if (rbuf->SeqNum == curReq)
            {
               skip = TRUE;
               break;
            }
            rbuf = mRcvdList.getNext(item);
         }
         if (skip)
            continue; // we already have this packet

         addResendRequest(curReq); // otherwise queue up a resend request
      }
   }
   else
   {
      // otherwise we just iterate over the rcvdlist and add resend requests for any gaps in that list

      if (mRcvdList.getSize() == 0)
         return; // nothing to request

      // increment curReq to start us off at the first unknown packet
      curReq = mRemoteSeqNum;

      BHandle item;
//-- FIXING PREFIX BUG ID 7602
      const BUDPConnectionRecvBuffer* i = mRcvdList.getHead(item);
//--
      while (i)
      {
         // loop through all missing sequence numbers and add them to resendRequestList      
         while (
               // dpm 11/15/2006
               // need >= 0 instead of > 0
               // because of the edge case:
               // curReq = 65535, (i)->SeqNum = 0
               // the loop will fail but (i)->SeqNum != curReq
               // and we'll skip over the resend request
                  (calcDistance(curReq, (i)->SeqNum) >= 0) &&
                  (calcDistance(curReq, (i)->SeqNum) < cQueueSize)
               )
         {
            addResendRequest(curReq);
            curReq++;
         }

         if ((i)->SeqNum == curReq)
            curReq++;

         i = mRcvdList.getNext(item);
      }
   }
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::generateResendRequests(void* buffer, DWORD* size)
{
   BASSERT(size);
   if (!size)
      return E_FAIL;

   if (mResendRequestVector.getNumber()==0)
   {
      *size = 0;
      return S_OK;
   }

   if (!getSocket())
      return E_FAIL;

   // loop through all resend requests and send them if they're old   
   BUDPConnectionResendRequest packet; 
   packet.Requests = reinterpret_cast<uint16*>(static_cast<uchar*>(buffer) + sizeof(int16));
   packet.NumberOfRequests = 0;

   // need to determine the number of requests that I can place in the buffer
   // based on the *size value
   uint maxRequests = (*size - sizeof(int16)) / sizeof(int16);

   for (uint i=0; i < mResendRequestVector.getSize() && i < maxRequests; i++)
   {
      if ((timeGetTime() - mResendRequestVector[i].SentTime) < cResendRequestInterval)
         continue;

      if (packet.NumberOfRequests > BUDPConnectionResendRequest::cMaxResendRequests)
         break;

      packet.Requests[packet.NumberOfRequests++] = mResendRequestVector[i].SeqNum;
      nlog(cTransportNL, "      requesting resend of %ld", mResendRequestVector[i].SeqNum);
      mResendRequestVector[i].SentTime = timeGetTime();
      // dpm 11/14/2006 - the amount and timer values are never set or updated
      // this will result in an undefined behavior in release builds
      // if this is desired, a fix may be to init mResendRequestAmount to 0 in the constructor
      // and reset when we advance the mResendRequestTimer
      //mResendRequestAmount++;
      //if (((mResendRequestAmount*1000) / (timeGetTime() - mResendRequestTimer)) > mResendRequestRatePerSec)
      //   break;
   }

   // this is fine and all, but we may have already overwritten our buffer at this point.
   // adding maxRequests check into the loop above
   BASSERT(sizeof(packet.NumberOfRequests) + (packet.NumberOfRequests * sizeof(int16)) < *size);
   if (sizeof(packet.NumberOfRequests) + (packet.NumberOfRequests * sizeof(int16)) > *size)
      return E_FAIL;

   *size = sizeof(packet.NumberOfRequests) + (packet.NumberOfRequests * sizeof(int16));
   Utils::FastMemCpy(buffer, &packet.NumberOfRequests, sizeof(int16));

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::updateRcvdList()
{
   // parse through rcvd list and free any messages that are in the correct sequence
   BHandle item;
   HRESULT hr = E_FAIL;
   BUDPConnectionRecvBuffer* i = mRcvdList.getHead(item);
   while (i)
   {      
      if ((i)->SeqNum == mRemoteSeqNum)
      {
         if (getObserver())
         {
            if (((i)->Type == BUDPConnectionHeaderType::cDataFragment) ||
                ((i)->Type == BUDPConnectionHeaderType::cDataFragmentDone))
            {
               if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < (i)->Size)
               {
                  nlog(cTransportNL, "BUDP360Connection::updateRcvdList -- not enough buffer space for fragment.");
                  // this shouldn't be happening because we prevent any outbound data from exceeding the fragment buffer size
                  BFAIL("BUDP360Connection::updateRcvdList -- Leave this up and call doug @ x275");
                  return;
               }

               Utils::FastMemCpy(mRecvFragmentBuffer + mRecvFragmentBufferPtr, (i)->Buffer, (i)->Size);
               mRecvFragmentBufferPtr += (i)->Size;
               if ((i)->Type == BUDPConnectionHeaderType::cDataFragmentDone)
               {
                  nlog(cTransportNL, "          in seq, release");
                  getObserver()->dataReceived(this, mRecvFragmentBuffer, mRecvFragmentBufferPtr, &(i)->RemoteAddress);
                  mRecvFragmentBufferPtr = 0;
               }
               else
                  nlog(cTransportNL, "          got a fragment");
            }
            else
            {
               nlog(cTransportNL, "      freeing old message %ld", (i)->SeqNum);
               getObserver()->dataReceived(this, (i)->Buffer, (i)->Size, &(i)->RemoteAddress);
            }
         }

         //NOTE - not thread safe if we are current disposing
         hr = freeRecvBuffer(i);
         BASSERT(SUCCEEDED(hr));

         // if I don't have an observer, I can't do anything with this packet anyways
         i = mRcvdList.removeAndGetNext(item);
         mRemoteSeqNum++;
         continue;
      }
      i = mRcvdList.getNext(item);
   }
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::sendWithPiggyBacks(IN BUDPConnectionSendBuffer* SendBuffer, IN BOOL Flush, IN BOOL Resends)
{
   if (!getSocket())
   {
      nlog(cTransportNL, "BUDP360Connection::sendWithPiggyBacks -- no socket.");
      return E_FAIL;   
   }

   // XXX manipulating the buffer here will require a changing of how the SendBuffer is dealt with below

   DWORD dataIndex = mDataIndexes[mBufferSet];
   DWORD voiceIndex = mVoiceIndexes[mBufferSet];

   uchar* dataBuffer = mDataBuffers[mBufferSet];
   uchar* voiceBuffer = mVoiceBuffers[mBufferSet];

   if (!dataBuffer)
      return E_FAIL;

   DWORD sendBufferLen = 0;

   if (SendBuffer)
   {
      sendBufferLen = SendBuffer->Length + sizeof(BUDPConnectionHeader);

      // there should ALWAYS be enough room to queue up any buffer because
      // we cap the buffers outside of this method to cMaxFragmentSize
      //
      // the tricky part will be if the existing buffers plus the new buffer
      // are greater than our MTU size, so how much do I send now and how much do I send later?
      if (dataIndex + voiceIndex + sendBufferLen >= 960)
      {
         DWORD bufferB = (mBufferSet + 1) % cMaxBuffers;

         if (SendBuffer->Flags & cSendUnencrypted)
         {
            // sanity check?
            if (cMaxSendSize - mVoiceIndexes[bufferB] < sendBufferLen)
               return E_FAIL;

            Utils::FastMemCpy(mVoiceBuffers[bufferB] + mVoiceIndexes[bufferB], SendBuffer->Header, sendBufferLen);

            mVoiceIndexes[bufferB] += sendBufferLen;
         }
         else
         {
            // sanity check?
            if (cMaxSendSize - mDataIndexes[bufferB] < sendBufferLen)
               return E_FAIL;

            Utils::FastMemCpy(mDataBuffers[bufferB] + mDataIndexes[bufferB], SendBuffer->Header, sendBufferLen);

            mDataIndexes[bufferB] += sendBufferLen;
         }

         // I need to flush the buffers now
         Flush = TRUE;
      }
      else
      {
         if (SendBuffer->Flags & cSendUnencrypted)
         {
            Utils::FastMemCpy(voiceBuffer + voiceIndex, SendBuffer->Header, sendBufferLen);

            voiceIndex += sendBufferLen;

            mVoiceIndexes[mBufferSet] = voiceIndex;
         }
         else
         {
            Utils::FastMemCpy(dataBuffer + dataIndex, SendBuffer->Header, sendBufferLen);

            dataIndex += sendBufferLen;

            mDataIndexes[mBufferSet] = dataIndex;
         }
      }

      // I've cached off this buffer,
      // now I need to flush the buffers
      SendBuffer = NULL;
   }

   // there are actually two states I want to acknowledge:
   // 1) flush
   // 2) flush and/or resend requests
   //
   // if I want to flush, then only execute this method if there is data available
   // if I want resend requests, then flush any buffers and send resends

   // if we're not being forced and we have no data available, then return
   if (Flush && !Resends && dataIndex == 0 && voiceIndex == 0)
      return S_OK;

   // if no resends are requested and we don't want to force a buffer flush, then return
   //
   // this is the usual case as we're coalescing the buffers
   if (!Resends && !Flush)
      return S_OK;

   // about to flush the buffers, need to be prepared to swap buffers

   BSendBuffer *sbuf=0;
   DWORD size = sizeof(BUDPConnectionResendRequest)+(BUDPConnectionResendRequest::cMaxResendRequests*sizeof(int16));
   // make room for two packets if needed
   DWORD length = 0;
   //if (SendBuffer)
   //   length+=SendBuffer->Length;
   length = dataIndex + voiceIndex;

   // if there is no data to send
   // and we have no resend requests
   // and our local sequence number matches the one ack'd by the client
   // then we no longer need to send data
   //if (length == 0 && mResendRequestVector.getNumber() == 0 && mStopAcks)
   //{
   //   updateResponsiveness();
   //   return S_OK;
   //}

   //// there's either a packet of data or we need to request a resend
   //// so start up with the acks again
   //mStopAcks = false;

   // size == data size of max resend requests
   // length == size of game data + voice
   // two sizeof(BUDPConnectionHeader) == potential for ack only and resend requests
   // sizeof(int16) == remote ack value
   // missing the 1 byte for BUDPConnectionHeaderType::cAckRemote
   // the remote ack value should be 1 byte for the type separator and then 2 bytes for the value
   //
   // this is a maximum allocation
   HRESULT hr = getSocket()->sendAllocateBuffer(size + length + (sizeof(BUDPConnectionHeader)*2) + 1 + sizeof(uint16), &sbuf);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "BUDP360Connection::sendWithPiggyBacks -- failed to allocate buffer.");
      return hr;
   }

   uchar* ptr = static_cast<uchar*>(sbuf->Buffer);
   const uchar* const dataPtr = ptr;

   if (dataIndex > 0)
   {
      Utils::FastMemCpy(ptr, dataBuffer, dataIndex);

      ptr += dataIndex;

      mDataIndexes[mBufferSet] = 0;

      if (mStopAcks)
      {
         mTimerModifier = 0;
         mStopAcks = false;
      }
   }
   else if (voiceIndex == 0)
   {
      // no data and no voice
      // just an ack
      BUDPConnectionHeader hdr;
      hdr.Type = BUDPConnectionHeaderType::cAck;
      hdr.Size = 0;
      hdr.SocketID = mLocalSocketID;
      // if our last packet had a seq number of 65535
      // and then we incremented that number, it would rollover to 0
      // if we then send an ack with 0, the other client(s) will think
      // we actually sent something that we haven't?
      if (mLocalSeqNum > 0)
         hdr.SeqNum = static_cast<uint16>(mLocalSeqNum-1);  // make sure the client knows what the last packet we sent out is, in case
      // he needs to request resends and we haven't sent him any real packets to let
      // him know that
      else if (mSentData)
         hdr.SeqNum = 65535;
      else
         hdr.SeqNum = 0;

      Utils::FastMemCpy(ptr, &hdr, sizeof(BUDPConnectionHeader));
      ptr += sizeof(BUDPConnectionHeader);
   }

   // ------------------------------------
   // #1: piggyback any resend requests
   hr = generateResendRequests(ptr + sizeof(BUDPConnectionHeader), &size);   
   BASSERT(SUCCEEDED(hr));

   // size is data length, plus the size of a header, plus the size of resends, plus the size of the ack at the end   
   if (size > 0)
   {
      // fill in that second header if needed
      sbuf->Length += sizeof(BUDPConnectionHeader);      
      BUDPConnectionHeader* hdr = reinterpret_cast<BUDPConnectionHeader*>(ptr);
      hdr->SeqNum = 0; // not used for resend requests
      hdr->Size = static_cast<uint16>(size);
      hdr->SocketID = mLocalSocketID;
      hdr->Type = BUDPConnectionHeaderType::cResendRequest;

      ptr += size + sizeof(BUDPConnectionHeader);
   }

   // ------------------------------------
   // #2: piggyback the ack
   *ptr = BUDPConnectionHeaderType::cAckRemote;
   ptr++;
   Utils::FastMemCpy(ptr, &mRemoteSeqNum, sizeof(uint16));
   ptr += sizeof(uint16);

   // ------------------------------------
   // #3: calculate the game data size
   DWORD dataLen = ptr - dataPtr;

   // ------------------------------------
   // #4: append any unencrypted data
   if (voiceIndex > 0)
   {
      Utils::FastMemCpy(ptr, voiceBuffer, voiceIndex);

      ptr += voiceIndex;

      mVoiceIndexes[mBufferSet] = 0;
   }

   // ------------------------------------
   // #5: calculate the total buffer size
   sbuf->Length = ptr - dataPtr;

   // ------------------------------------
   // #6: calculate the voice buffer size
   sbuf->VoiceLength = ptr - dataPtr - dataLen;

   // ------------------------------------
   // #7: swap buffers
   mBufferSet = (mBufferSet + 1) % cMaxBuffers;

   nlog(cTransportNL, "      BUDP360Connection::sendWithPiggyBacks - Length %ld, acking %ld", sbuf->Length, mRemoteSeqNum);
   if (SendBuffer)
      nlog(cTransportNL, "        SendBuffer->Header->SeqNum %ld", SendBuffer->Header->SeqNum);   

   hr = socketLevelSend(sbuf);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "BUDP360Connection::sendWithPiggyBacks -- failed _sendTo hr 0x%x", hr);
      return hr;
   }

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::send(IN BSendBuffer* SendBuffer)
{
   HRESULT hr = E_FAIL;
   if (mState != cConnected)
   {
      hr = sendFreeBuffer(SendBuffer);
      BASSERT(SUCCEEDED(hr));
      nlog(cTransportNL, "BUDP360Connection::send -- not connected.");
      return E_FAIL;
   }

   if (mBoundToLoopbackSocket)
   {
      //Just send the buffer to the socket - no queuing
      return (getSocket()->send(SendBuffer));
   }

   BASSERT(SendBuffer->Length < cMaxSendSize);
   if (SendBuffer->Length >= cMaxSendSize)
   {
      nlog(cTransportNL, "      BUDP360Connection::send ERROR Tried to send packet with size %ld", SendBuffer->Length);
      return MEM_E_INVALID_SIZE; // er, not the best error, but I defy you to find a better one in winerror.h! ;-)
   }

   // we have to fragment the packet?
   if (SendBuffer->Length > cMaxFragmentSize)
   {
      DWORD dataDone = 0;
      DWORD dataLeft = SendBuffer->Length - dataDone;
      DWORD sendSize = min(cMaxFragmentSize, dataLeft);

      // send out all the fragments
      while (dataLeft > 0)
      {
         BSendBuffer *tempSendBuffer = 0;
         hr = sendAllocateBuffer(sendSize, &tempSendBuffer);
         BASSERT(SUCCEEDED(hr));
         tempSendBuffer->Flags = SendBuffer->Flags;
         tempSendBuffer->Length = sendSize;
         Utils::FastMemCpy(tempSendBuffer->Buffer, static_cast<uchar*>(SendBuffer->Buffer)+dataDone, sendSize);
         BUDPConnectionSendBuffer* tempSocksBuffer = static_cast<BUDPConnectionSendBuffer*>(tempSendBuffer);
         tempSocksBuffer->Header->SeqNum = mLocalSeqNum++;
         mSentData = TRUE;
         tempSocksBuffer->Header->Size = static_cast<uint16>(sendSize);
         tempSocksBuffer->Header->SocketID = mLocalSocketID;
         if ((dataLeft - sendSize) > 0)
            tempSocksBuffer->Header->Type = BUDPConnectionHeaderType::cDataFragment;
         else
            tempSocksBuffer->Header->Type = BUDPConnectionHeaderType::cDataFragmentDone;

         hr = sendWithPiggyBacks(tempSocksBuffer, mFlushSendBuffers, mIncludeResendRequest);
         if(FAILED(hr))
            nlog(cTransportNL, "BUDP360Connection::send -- failed sendWithPiggyBacks 1.");

         tempSocksBuffer->SendTime = timeGetTime();
         mSentList.addToTail(tempSocksBuffer);

         dataDone += sendSize;         
         dataLeft = SendBuffer->Length - dataDone;
         sendSize = min(cMaxFragmentSize, dataLeft);
      }

      hr = sendFreeBuffer(SendBuffer); // free the original buffer
      BASSERT(SUCCEEDED(hr));
   }
   // otherwise just send it out
   else
   {      
      BUDPConnectionSendBuffer *buf = static_cast<BUDPConnectionSendBuffer*>(SendBuffer);   

      mSentData = TRUE;
      buf->Header->Size = static_cast<uint16>(SendBuffer->Length);
      buf->Header->SocketID = mLocalSocketID;
      buf->Header->Type = BUDPConnectionHeaderType::cData;

      if (SendBuffer->Flags & cSendUnreliable)
      {
         buf->Header->SeqNum = 0;
         buf->Header->Type |= 0x80;
      }
      else
         buf->Header->SeqNum = mLocalSeqNum++;

      hr = sendWithPiggyBacks(buf, mFlushSendBuffers, mIncludeResendRequest);
      if(FAILED(hr))
         nlog(cTransportNL, "BUDP360Connection::send -- failed sendWithPiggyBacks 2.");

      buf->SendTime = timeGetTime();
      mSentList.addToTail(buf);
   }

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::send(const void* Buffer, const long Length, const DWORD flags)
{
   if (mState != cConnected)
   {
      nlog(cTransportNL, "BUDP360Connection::send -- not connected.");
      return E_FAIL;
   }

   BSendBuffer *buf;
   HRESULT hr;
   if (mBoundToLoopbackSocket)
   {
      hr = getSocket()->sendAllocateBuffer(Length, &buf);
   }
   else
   {
      hr = sendAllocateBuffer( Length, &buf );
   }

   if (FAILED(hr))
   {
      nlog(cTransportNL, "BUDP360Connection::send -- can't allocate buffer size %d", Length);
      return hr;
   }

   Utils::FastMemCpy(buf->Buffer, Buffer, Length);
   buf->Length = Length;
   buf->Flags = flags;

   hr = send(buf);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "BUDP360Connection::send -- mConnectedSocket failed send hr 0x%x", hr);
      return hr;
   }

   return S_OK;   
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::sendFreeBuffer(IN BSendBuffer* SendBuffer)
{
   return freeSendBuffer((BUDPConnectionSendBuffer *)SendBuffer);
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Connection::close()
{
   nlog(cTransportNL, "BUDP360Connection[%p]::close", this);
   mActiveTimer = cTimerNone;
   dispose();

   return S_OK;
}

//
// For now we allocate a new buffer on every request.
// The API is designed to allow us to cache requests in a buffer list, eventually.
//

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::setUnresponsiveTimeoutValue(DWORD dwValue)
{
   m_dwUnresponsiveTimeoutValue = dwValue;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::allocateDataVoiceBuffers()
{
   for (uint i=0; i < cMaxBuffers; i++)
   {
      mDataIndexes[i] = 0;
      mVoiceIndexes[i] = 0;

      if (!mDataBuffers[i])
         mDataBuffers[i] = static_cast<LPBYTE>(gNetworkHeap.New(cMaxSendSize));

      if (mDataBuffers[i] == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
      }

      if (!mVoiceBuffers[i])
         mVoiceBuffers[i] = static_cast<LPBYTE>(gNetworkHeap.New(cMaxSendSize));

      if (mVoiceBuffers[i] == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::addToActiveConnections()
{
   uint8 i;
   //Make sure I'm not already in the list somehow
   for (i = 0; i < mNumActiveConnections; i++)
      if (mpActiveConnections[i] == this)
          break;

   if (i == mNumActiveConnections)
   {
      BASSERT(mNumActiveConnections < cMaxActiveConnections);
      mpActiveConnections[mNumActiveConnections++] = this;
   } 
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::removeFromActiveConnections()
{
   uint8 i;
   for (i = 0; i < mNumActiveConnections; i++)
   {  
      if (mpActiveConnections[i] == this)
      {
         mpActiveConnections[i] = mpActiveConnections[mNumActiveConnections - 1];
         break;
      }
   }

   if (i < mNumActiveConnections)
      mNumActiveConnections--;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Connection::tickActiveConnections()
{
   for (uint8 i = 0; i < mNumActiveConnections; i++)
      mpActiveConnections[i]->service();
}
