//==============================================================================
// SocksGameSocket.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "MaxSendSize.h"
#include "SocksGameSocket.h"
#include "SocksUnreliableSocket.h"
#include "SocksMapper.h"

#ifndef XBOX
#include "objbase.h"
#endif

#include "config.h"
#include "econfigenum.h"
#include "SocksHelper.h"

// rg [7/12/05] - Do we need this on all the time in debug/playtest??
#ifndef XBOX
//#ifndef BUILD_FINAL
//   #define GUID_TRACKING 1
//#endif
#endif
 
using namespace Etl;

extern BMemoryHeap gNetworkHeap;

namespace
{
   //==============================================================================
   //
   void GUIDtoString(const unsigned char *src, PSTR strDest, unsigned long destBufLen)
   {
      const long strBufLen = 8;
      char strBuf[strBufLen];
      DWORD aByteLengths[] = {4,2,2,2,6};

      *strDest = NULL;
      for(DWORD i=0, dwAccum=0; i<5; dwAccum+=aByteLengths[i], i++) {
         for(DWORD j=0; j<aByteLengths[i]; j++) {
            StringCchPrintfA(strBuf, strBufLen, "%02x", (DWORD)src[j+dwAccum]);
            StringCchCatA(strDest, destBufLen, strBuf);
         }
      }
   }
}


//==============================================================================
// Defines

//==============================================================================
//
BSocksGameSocket::BSocksGameSocket (IN BObserver * Observer, const SOCKADDR_IN &identifierAddress)
   : BSocksSocketBase(Observer),
   mState(cDisconnected),
   mRemoteSocketID(0), 
   mLocalSocketID(0),
   mConnectingTimer(0),
   mResendRequestRatePerSec(cDefaultResendRequestRatePerSec),
   mLastRecvDataTime(0),
   mDisconnecting(FALSE),
#ifndef BUILD_FINAL
   m_dwByteReceiveCount( 0 ),
   m_dwPacketReceiveCount( 0 ),
   m_dwPacketResendRequestCount( 0 ),
   m_dwInSequencePacketReceiveCount( 0 ),
   m_dwOutOfSequencePacketReceiveCount( 0 ),
   m_dwPacketOneSecondDeltaBegin( 0 ),
   m_dwPacketOneSecondDeltaEnd( 0 ),   
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
   mIncludeResendRequest(FALSE)
{         
   memset(&mForwardingServerAddress, 0, sizeof(mForwardingServerAddress));
   memset(&mRemoteIdentifierAddress, 0, sizeof(mRemoteIdentifierAddress));
   mLocalIdentifierAddress = identifierAddress;

   if (!gConfig.get(cConfigSocketUnresponsiveTimeout, (long*)&m_dwUnresponsiveTimeoutValue))
      m_dwUnresponsiveTimeoutValue = cUnresponsiveTimeoutValue;

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigFlushSendBuffers))
   {
      mFlushSendBuffers = TRUE;
      mIncludeResendRequest = TRUE;
   }
#endif

   XMemSet(mDataIndexes, 0, sizeof(mDataIndexes));
   XMemSet(mVoiceIndexes, 0, sizeof(mVoiceIndexes));

   XMemSet(mDataBuffers, 0, sizeof(mDataBuffers));
   XMemSet(mVoiceBuffers, 0, sizeof(mVoiceBuffers));
}

//==============================================================================
//
HRESULT BSocksGameSocket::connect (
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
{
   BASSERT(RemoteAddress != NULL);
   
   nlog(cPerfNL, "BSocksGameSocket[%p]::connect", this);
   nlog(cTransportNL, "BSocksGameSocket[%p]::connect", this);

   BSocksMapper * mapper = BSocksMapper::getInstance();

   HRESULT hr = S_OK;
   if (mForwardingServerAddress.sin_family == AF_INET)
      hr = mapper->connectSocket(this, &mForwardingServerAddress, LocalAddress);
   else
      hr = mapper->connectSocket(this, RemoteAddress, LocalAddress);

   if (FAILED(hr))
      return hr;

   memset(&mRemoteAddress, 0, sizeof(mRemoteAddress));
   memset(&mLocalAddress, 0, sizeof(mLocalAddress));
   
   if (LocalAddress) mLocalAddress = *LocalAddress;	  
   mRemoteAddress = *RemoteAddress;
   mRemoteIdentifierAddress = *RemoteAddress;

   hr = asyncSelect(0);
   if (FAILED(hr))
      return hr;   

   mState = cConnecting;      
   nlog(cTransportNL, "BSocksGameSocket[%p] set state to %ld in connect", this, mState);
   mRemoteSocketID = 0;   
   mLocalSeqNum = mRemoteSeqNum = 0;
   mLocalSocketID = (unsigned short)(timeGetTime()+rand());   

   hr = startTimer(cConnectTimer, cConnectTimerInterval);
   BASSERT(SUCCEEDED(hr));
   hr = startTimer(cUpdateTimer, cUpdateTimerInterval);
   BASSERT(SUCCEEDED(hr));

   for (int i=0; i < cMaxBuffers; ++i)
   {
      mDataIndexes[i] = 0;
      mVoiceIndexes[i] = 0;

      if (!mDataBuffers[i])
         mDataBuffers[i] = (LPBYTE)gNetworkHeap.New(cMaxSendSize);

      if (mDataBuffers[i] == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
         return E_OUTOFMEMORY;
      }

      if (!mVoiceBuffers[i])
         mVoiceBuffers[i] = (LPBYTE)gNetworkHeap.New(cMaxSendSize);

      if (mVoiceBuffers[i] == NULL)
      {
         BFAIL("gNetworkHeap: Out of memory");
         return E_OUTOFMEMORY;
      }
   }

   return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::_sendTo(BSendBuffer *buf, const SOCKADDR_IN *DestinationAddress)
{
   if (!getSocket())
      return E_FAIL;

   nlog(cTransportNL, "      BSocksGameSocket SOCKET [%p] _sendTo %s:%ld", this, inet_ntoa(DestinationAddress->sin_addr), htons(DestinationAddress->sin_port));

   if (mForwardingServerAddress.sin_family == AF_INET)
   {
      BSendBuffer *sbuf;
      HRESULT hr = getSocket()->sendAllocateBuffer(buf->Length+1+sizeof(SOCKADDR_IN), &sbuf);    
      if (FAILED(hr))
         return hr;

      unsigned char *ptr = (unsigned char*)sbuf->Buffer;

      BYTE b = BSocksGameHeaderType::cRepeatedPacket;
      XMemCpy(ptr, &b, 1);
      XMemCpy(ptr + 1, (void *)DestinationAddress, sizeof(SOCKADDR_IN));
      XMemCpy(ptr + 1 + sizeof(SOCKADDR_IN), buf->Buffer, buf->Length);
      sbuf->Length = 1 + sizeof(SOCKADDR_IN) + buf->Length;
      sbuf->VoiceLength = 0;

      hr = getSocket()->sendFreeBuffer(buf);
      BASSERT(SUCCEEDED(hr));

      return getSocket()->sendTo(sbuf, &mForwardingServerAddress);
   }
   else   
   {
      #ifdef GUID_TRACKING
         GUID g;
#ifdef XBOX
         createGUIDXbox(&g);
#else         
         ::CoCreateGuid(&g);
#endif         

         const unsigned long length = 256;
         char str[length];
         char *p = (char *)str;
         GUIDtoString((unsigned char *)&g, p, length);

         nlog(cTransportNL, "        GUID: %s", p);

         BSendBuffer *sbuf;
         HRESULT hr = getSocket()->sendAllocateBuffer(buf->Length+sizeof(GUID), &sbuf);    
         if (FAILED(hr))
            return hr;

         CopyMemory(sbuf->Buffer, &g, sizeof(GUID));
         CopyMemory(((char *)sbuf->Buffer)+sizeof(GUID), buf->Buffer, buf->Length);         
         sbuf->Length = sizeof(GUID)+buf->Length;

         hr = getSocket()->sendFreeBuffer(buf);
         BASSERT(SUCCEEDED(hr));
      
         return getSocket()->sendTo(sbuf, DestinationAddress);
      #else
         return getSocket()->sendTo(buf, DestinationAddress);
      #endif
   }
}

//==============================================================================
//
BSocksGameSocket::~BSocksGameSocket(void)
{
}

//==============================================================================
//
HRESULT BSocksGameSocket::dispose(void)
{
   nlog(cTransportNL, "BSocksGameSocket[%p]::dispose", this);
   mDisconnecting = TRUE;

   if (getSocket() && isConnected())
   {
      for (long i=0;i<cDisconnectResendAmount;i++)
      {
         BSendBuffer *buf;
         getSocket()->sendAllocateBuffer(3, &buf);
         BYTE b = BSocksGameHeaderType::cDisconnect;
         unsigned char *ptr = (unsigned char*)buf->Buffer;
         memcpy(ptr, &b, 1);
         memcpy(ptr+1, &mLocalSocketID, 2);
         buf->Length = 3;
         buf->VoiceLength = 0;
         _sendTo(buf, &mRemoteAddress);
      }
      DWORD now = timeGetTime();
      while (timeGetTime() - now < cDisconnectSleepTimer); // Don't sleep, coz we don't want events firing off      
   }

   BSocksMapper *mapper = BSocksMapper::getInstance();
   mapper->disconnectSocket(this);

   BHandle item;
   BSocksGameSendBuffer *send = mSentList.getHead(item);
   HRESULT hr = E_FAIL;
   while (send)
   {
      hr = freeSendBuffer(send);
      BASSERT(SUCCEEDED(hr));
      send = mSentList.removeAndGetNext(item);
   }
   mSentList.reset();

   BSocksGameRecvBuffer *recv = mRcvdList.getHead(item);
   while (recv)
   {
      hr = freeRecvBuffer(recv);
      BASSERT(SUCCEEDED(hr));
      recv = mRcvdList.removeAndGetNext(item);
   }
   mRcvdList.reset();

   for (int i=0; i < cMaxBuffers; ++i)
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

   mDisconnecting = FALSE;

   return BSocksSocketBase::dispose();
}

//==============================================================================
//
void BSocksGameSocket::tic (DWORD timerID)
{
   if (timerID == cConnectTimer)
   {
      if (getSocket() && (mState == cConnecting))
      {         
         BSendBuffer *buf=0;         
         BSocksGameConnectHeader header;
         header.Type = BSocksGameHeaderType::cHello;
         header.LocalSocketID = mLocalSocketID;
         header.RemoteAddress = mRemoteIdentifierAddress;
         header.LocalAddress = mLocalIdentifierAddress;

         if (FAILED(getSocket()->sendAllocateBuffer(sizeof(header), &buf)))
         {
            close();
            return;
         }

         XMemCpy(buf->Buffer, &header, sizeof(header));
         buf->Length = sizeof(header);
         buf->VoiceLength = 0;

         const long bufSize = 64;
         char a1[bufSize];
         char a2[bufSize];

         StringCchCopyNExA(a1, bufSize, inet_ntoa(mLocalIdentifierAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a1[63] = 0;
         StringCchCopyNExA(a2, bufSize, inet_ntoa(mRemoteIdentifierAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a2[63] = 0;
         nlog(cTransportNL, "      BSocksGameSocket SOCKET [%p] Sending hello to LocalAddress %s:%ld, localSocketID %ld", this,
            a1, htons(mLocalIdentifierAddress.sin_port), mLocalSocketID);
         nlog(cTransportNL, "      , RemoteAddress %s:%ld",
            a2, htons(mRemoteIdentifierAddress.sin_port));

         nlog(cPerfNL, "BSocksGameSocket[%p]::tic sending hello", this);

         if (FAILED(_sendTo(buf, &mRemoteAddress)))
         {
            close();
            return;
         }
      }

      const long cConnectTimeout = 4000;
      //const long cConnectTimeout = 30*60*1000;
      if (mRemoteSocketID && mConnectingTimer && (mState == cConnecting) && (timeGetTime() - mConnectingTimer > cConnectTimeout))
      {
         // sorry, this guy loses - accept next connection
         mRemoteSocketID = 0;
         mConnectingTimer = 0;
      }
   }
   else if (timerID == cUpdateTimer)
   {
      HRESULT hr = sendWithPiggyBacks(0, TRUE, TRUE); // send just any resend requests we have
      if(FAILED(hr))
         nlog(cTransportNL, "BSocksGameSocket::tic -- failed sendWithPiggyBacks.");


      const long cNotifyUnresponsiveInterval = 1000;
      const long cNotifyResponsiveInterval = 2000;

      if (mConnected)
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
                  if (getObserver()->notifyUnresponsiveSocket(this, mLastRecvDataTime))
                  {               
                     if (getObserver())
                        getObserver()->disconnected(this, 0); // FIXME: Status 0?               
                     mState = cDisconnected;
                     nlog(cTransportNL, "BSocksGameSocket[%p] set state to %ld", this, mState);
                     mConnected = false;
                  }
               }
               else if (time - mLastRecvDataTime > cAckTimeout)
               {
                  // have we not heard anything in cAckTimeout time? if so, we're linkdead
                  if (getObserver())
                     getObserver()->disconnected(this, 0); // FIXME: Status 0?            
                  mState = cDisconnected;
                  nlog(cTransportNL, "BSocksGameSocket[%p] set state to %ld because of ack timeout", this, mState);
                  mConnected = false;
               }
            }
         }
         // else we are responding, has the notify interval lapsed?
         else if ((time - mLastResponsiveTime) > cNotifyResponsiveInterval)
         {
            if (getObserver())
               getObserver()->notifyResponsiveSocket(this, mLastRecvDataTime);

            mLastResponsiveTime = time;
         }
      }
   }
   else
   {
      // flush our send buffers out to the socket
      sendWithPiggyBacks(0, TRUE, FALSE);
   }
}

//==============================================================================
//
HRESULT BSocksGameSocket::sendAllocateBuffer (
         IN DWORD MaximumLength,
         OUT BSendBuffer ** ReturnSendBuffer)
{
   return allocateSendBuffer(MaximumLength, (BSocksGameSendBuffer **)ReturnSendBuffer);	
}

//==============================================================================
//
bool BSocksGameSocket::recvHello(IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress)
{
   RemoteAddress;

   nlog(cTransportNL, "      BSocksGameSocket::recvHello");

   if (Length == sizeof(BSocksGameConnectHeader))
   {
      // send a response
      BSocksGameConnectHeader header;
      header.Type = BSocksGameHeaderType::cHelloResponse;
      header.LocalSocketID = mLocalSocketID;
      BSocksGameConnectHeader *rheader;
      rheader = (BSocksGameConnectHeader *)Buffer;
      header.RemoteSocketID = rheader->LocalSocketID;

      nlog(cTransportNL, "        BSocksGameSocket SOCKET [%p] Hello request from %ld %s:%ld, we are %ld", this, rheader->LocalSocketID, inet_ntoa(RemoteAddress->sin_addr), htons(RemoteAddress->sin_port), mLocalSocketID);

      if ((mRemoteSocketID != 0) && (mRemoteSocketID != header.RemoteSocketID))
      {
         nlog(cTransportNL, "          rejecting: mRemoteSocketID %ld, headerRemoteSocketID %ld", mRemoteSocketID, header.RemoteSocketID);
         return false;
      }

      // is this guy really talking to me?
      if (
            (memcmp(&rheader->RemoteAddress, &mLocalIdentifierAddress, sizeof(SOCKADDR_IN))!=0) ||
            (memcmp(&rheader->LocalAddress, &mRemoteIdentifierAddress, sizeof(SOCKADDR_IN))!=0)
         )
      {
         const long bufSize = 64;
         char a1[bufSize];
         char a2[bufSize];

         StringCchCopyNExA(a1, bufSize, inet_ntoa(rheader->RemoteAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a1[63] = 0;
         StringCchCopyNExA(a2, bufSize, inet_ntoa(mLocalIdentifierAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a2[63] = 0;
         nlog(cTransportNL, "          rejecting: rheader->RemoteAddress %s:%ld, mLocalIdentifierAddress %s:%ld", 
            a1, htons(rheader->RemoteAddress.sin_port),           
            a2, htons(mLocalIdentifierAddress.sin_port));

         StringCchCopyNExA(a1, bufSize, inet_ntoa(rheader->LocalAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a1[63] = 0;
         StringCchCopyNExA(a2, bufSize, inet_ntoa(mRemoteIdentifierAddress.sin_addr), 63, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
         a2[63] = 0;
         nlog(cTransportNL, "          rejecting: rheader->LocalAddress %s:%ld, mRemoteIdentifierAddress %s:%ld", 
            a1, htons(rheader->LocalAddress.sin_port),
            a2, htons(mRemoteIdentifierAddress.sin_port));
         return false;
      }

      // in case port is cycling, copy in latest port if not forwarding
      if (mForwardingServerAddress.sin_family != AF_INET)
         mRemoteAddress.sin_port = RemoteAddress->sin_port;

      mRemoteSocketID = rheader->LocalSocketID;
      mConnectingTimer = timeGetTime();

      nlog(cPerfNL, "BSocksGameSocket[%p]::recvHello sending response", this);
      nlog(cTransportNL, "        sending hello response to localSocketID %ld, remoteSocketID %ld",
         header.LocalSocketID, header.RemoteSocketID);

      BSendBuffer *buf;
      HRESULT hr = Socket->sendAllocateBuffer(sizeof(BSocksGameConnectHeader), &buf);
      hr;
      BASSERT(SUCCEEDED(hr));
      XMemCpy(buf->Buffer, &header, sizeof(header));
      buf->Length = sizeof(header);
      buf->VoiceLength = 0;

      _sendTo(buf, &mRemoteAddress);

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
void BSocksGameSocket::updateResponsiveness(void)
{   
   mLastRecvDataTime = timeGetTime();

   if (!mResponsive)
   {
      mResponsive = TRUE;
      // we have a different mechanism for notifying a responsive socket now so don't rely on this one shot bound to miss event
      // if (getObserver())
      //getObserver()->notifyUnresponsiveSocket(this, mLastRecvDataTime);   
   }
}

//==============================================================================
//
bool BSocksGameSocket::recvHelloResponse (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress)
{
   RemoteAddress; Socket;

   nlog(cTransportNL, "      BSocksGameSocket::recvHelloResponse");

   if (Length == sizeof(BSocksGameConnectHeader))
   {
      BSocksGameConnectHeader *header = (BSocksGameConnectHeader *)Buffer;
      // is remote guy talking to us?
      if (header->RemoteSocketID == mLocalSocketID)
      {
         mRemoteSocketID = header->LocalSocketID;
         // if so, then we're connected                   
         nlog(cTransportNL, "        BSocksGameSocket SOCKET [%p] Connected to %ld, we are %ld", this, mRemoteSocketID, mLocalSocketID);
         nlog(cSessionNL, "BSocksGameSocket SOCKET [%p] Connected to %ld, we are %ld", this, mRemoteSocketID, mLocalSocketID);

         #ifdef _DEBUG
         {
            const long bufSize = 64;
            char a1[bufSize];
            char a2[bufSize];
            char *strA1 = inet_ntoa(mLocalIdentifierAddress.sin_addr);
            BASSERT(strA1);
            StringCchCopyNExA(a1, bufSize, strA1, bufSize - 1, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
            a1[bufSize - 1] = 0;
            char *strA2 = inet_ntoa(mRemoteIdentifierAddress.sin_addr);
            BASSERT(strA2);
            StringCchCopyNExA(a2, bufSize, strA2, bufSize - 1, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
            a2[bufSize - 1] = 0;
            nlog(cTransportNL, "  mLocalIdentifierAddress %s:%ld, mRemoteIdentifierAddress %s:%ld",
               a1, ntohs(mLocalIdentifierAddress.sin_port),
               a2, ntohs(mRemoteIdentifierAddress.sin_port));
         }
         #endif
         
         mState = cConnected;
         nlog(cTransportNL, "BSocksGameSocket[%p] set state to %ld", this, mState);
         mConnectingTimer=0;
         stopTimer(cConnectTimer);         
         mConnected = true;         
         updateResponsiveness();

         nlog(cPerfNL, "BSocksGameSocket[%p]::recvHelloResponse connected", this);

         if (getObserver())
            getObserver()->connected(this);

         return true;
      }
      else
      {
         nlog(cTransportNL, "        rejecting hello response because remotesocketID %ld, localsocketID %ld", header->RemoteSocketID, mLocalSocketID);
         return false;
      }
   }  
   else
   {
      nlog(cTransportNL, "        rejecting hello response because length %ld", Length);
      return false;
   }
}

//==============================================================================
//
void BSocksGameSocket::clearSentList(unsigned short ack)
{
   nlog(cTransportNL, "      clearSentList ack %ld, mLocalSeqNum %ld", ack, mLocalSeqNum);
   // is this an ack for the future?
   if (calcDistance(ack, mLocalSeqNum) < cQueueSize)
   {
      // if so, then clear out any sent items older than this
      BHandle item;
      HRESULT hr = E_FAIL;
      BSocksGameSendBuffer *j = mSentList.getHead(item);
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
bool BSocksGameSocket::recvData (
               IN BSocket *,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress)
{
#ifndef BUILD_FINAL
   m_dwPacketReceiveCount++;
#endif

   nlog(cTransportNL, "    BSocksGameSocket::recvData %ld", Length);

   // insure we have enough data for the header
   // Length includes the voiceLength, subtract out to get the actual data length
   if (Length < sizeof(BSocksGameHeader))
      return false;

   const unsigned char *ptr=static_cast<const unsigned char *>(Buffer);

#ifndef BUILD_FINAL
   const unsigned char * const dataPtr = ptr;
#endif
   const unsigned char * const endPtr = ptr+Length;

   // the voice data segment will require information from the headers contained in the game data
   // voiceLen will be used to help in boundary checks
   const unsigned char * const voicePtr = ptr + (Length - voiceLength);
   const unsigned char * const ackPtr = voicePtr - sizeof(short) - 1; // the trailing ack is 1 byte for the separator and 2 bytes of the value

   // First we go through and parse all the subpackets in the packet
   // We also parse any resend requests that are in here
   // and finally we process any ack attached

   // go through all the sub-packets that are in this packet
   //while (ptr < ((unsigned char *)Buffer)+Length)
   while (ptr < endPtr)
   {
      // are we at the end of the packet where the ack lives?
      //if ((DWORD)(ptr-((unsigned char *)Buffer)) == (Length-sizeof(short)))
      if (ptr == ackPtr)
      {
         unsigned short ack;
         memcpy(&ack, ptr+1, sizeof(short));
         clearSentList(ack);
         nlog(cTransportNL, "        recvData ack %ld", ack);
         ptr+=sizeof(short)+1;
      }
      else
      {
         // otherwise it's a full packet of data, parse it            
         BSocksGameHeader *header = (BSocksGameHeader *)ptr;            
         nlog(cTransportNL, "        header->SocketID %ld, mRemoteSocketID %ld", header->SocketID, mRemoteSocketID);
         if (header->SocketID != mRemoteSocketID) // FIXME: Support broadcasted packets on the same socket? Does this even make sense?
            return false;

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

         if (getHeaderType(ptr) == BSocksGameHeaderType::cResendRequest)
         {

#ifndef BUILD_FINAL
            m_dwPacketResendRequestCount++;
#endif
            BSocksGameResendRequest req;
            XMemCpy(&req.NumberOfRequests, ptr+sizeof(BSocksGameHeader), sizeof(req.NumberOfRequests));
            req.Requests = (unsigned short *)(ptr+sizeof(BSocksGameHeader)+sizeof(req.NumberOfRequests));
            for (long i=0;i<req.NumberOfRequests;i++)
            {
               // resend the matching sent item
               nlog(cTransportNL, "        resend requested for %ld", req.Requests[i]);
               BHandle item;
               BSocksGameSendBuffer *j = mSentList.getHead(item);
               while (j)
               {
                  if ((j)->Header->SeqNum == req.Requests[i])
                  {
                     nlog(cTransportNL, "        resending %ld", (j)->Header->SeqNum);
                     HRESULT hr = sendWithPiggyBacks(j, mFlushSendBuffers, mIncludeResendRequest);
                     if(FAILED(hr))
                        nlog(cTransportNL, "BSocksGameSocket::recvData -- failed sendWithPiggyBacks.");

                     //(*j)->ResendRequestTime = timeGetTime(); // FIXME: piggyback resends also someday!
                     break;
                  }

                  j = mSentList.getNext(item);
               }
            }
         }
         else if ((getHeaderType(ptr) == BSocksGameHeaderType::cData) || 
                  (getHeaderType(ptr) == BSocksGameHeaderType::cDataFragment) ||
                  (getHeaderType(ptr) == BSocksGameHeaderType::cDataFragmentDone))
         {
            nlog(cTransportNL, "        got data packet %ld", header->SeqNum);
            // first see if we have an outstanding resend request for this               
            for (long i=0;i<mResendRequestVector.getNumber();i++)
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
               if (getObserver() && getHeaderType(ptr) == BSocksGameHeaderType::cData)
               {
                  getObserver()->recvd(this, static_cast<const void *>(ptr+sizeof(BSocksGameHeader)), header->Size, 0, RemoteAddress);
               }
            }
            else if (header->SeqNum == mRemoteSeqNum)
            {
               // we got an in-sequence packet, so just release it
               if (getObserver())
               {
                  if (  (getHeaderType(ptr) == BSocksGameHeaderType::cDataFragment) ||
                        (getHeaderType(ptr) == BSocksGameHeaderType::cDataFragmentDone))
                  {
                     if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < header->Size)
                     {
                        nlog(cTransportNL, "BSocksGameSocket::recvData -- not enough buffer space for fragment.");
                        BFAIL("BSocksGameSocket::recvData -- Leave this up and call mike @ x286");
                        return false;
                     }

                     memcpy(mRecvFragmentBuffer+mRecvFragmentBufferPtr, ptr+sizeof(BSocksGameHeader), header->Size);
                     mRecvFragmentBufferPtr+=header->Size;
                     if (getHeaderType(ptr) == BSocksGameHeaderType::cDataFragmentDone)
                     {
                        nlog(cTransportNL, "          in seq %ld, release", header->SeqNum);
                        getObserver()->recvd(this, mRecvFragmentBuffer, mRecvFragmentBufferPtr, 0, RemoteAddress);
                        mRecvFragmentBufferPtr = 0;
                     }
                     else
                        nlog(cTransportNL, "          got a fragment");
                  }
                  else
                  {
                     nlog(cTransportNL, "          in seq %ld, release", header->SeqNum);
                     getObserver()->recvd(this, static_cast<const void *>(ptr+sizeof(BSocksGameHeader)), header->Size, 0, RemoteAddress);
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

               long distance = calcDistance(mRemoteSeqNum, header->SeqNum);               

               // is it a message in the future? If so, stuff it into rcvd list
               if (distance < (cQueueSize)-1)
               {
                  nlog(cTransportNL, "          in future");

                  BSocksGameRecvBuffer *rbuf=0;
                  HRESULT hr = allocateRecvBuffer(&rbuf, header->Size);
                  if (FAILED(hr))
                     return false;

                  // set up the receive buffer
                  rbuf->RecvTime = timeGetTime();
                  rbuf->SeqNum = header->SeqNum;
                  rbuf->Size = header->Size;
                  rbuf->Type = getHeaderType(ptr);
                  XMemCpy(rbuf->Buffer, ptr+sizeof(BSocksGameHeader), header->Size);
                  BASSERT(RemoteAddress);
                  if (RemoteAddress)
                     rbuf->RemoteAddress = *RemoteAddress;                     

                  // push into rcvd list
                  bool found = false;
                  BHandle item;
                  BSocksGameRecvBuffer *i = mRcvdList.getHead(item);
                  while(i)
                  {                        
                     if (
                           (calcDistance(header->SeqNum, (i)->SeqNum) < cQueueSize) &&
                           (header->SeqNum < (i)->SeqNum) 
                        )                                                      
                     {
                        nlog(cTransportNL, "            inserting before %ld", (i)->SeqNum);
                        mRcvdList.addBefore(rbuf, item);
                        found = true;
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
         else if (getHeaderType(ptr) == BSocksGameHeaderType::cAck)
         {
            // calculate any resends we may need to request
            calcResendRequests((long)header->SeqNum);
         }

         // go to next sub-packet
         ptr += sizeof(BSocksGameHeader)+header->Size;
      }
#ifndef BUILD_FINAL
      m_dwByteReceiveCount += (ptr - dataPtr);
#endif
   }

   return true;
}

#ifndef BUILD_FINAL
//
// getBytesReceiveCount
//
// returns the total number of packets that have been received.
// 
DWORD BSocksGameSocket::getByteReceiveCount() const
{
   return m_dwByteReceiveCount;
}
//
// getBytesReceiveCountPerSecond
//
// returns the total number of packets that have been received.
// 

DWORD BSocksGameSocket::getByteReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if ( dwElapsedTime < 1000 )
   {
      return 0;
   }
   return m_dwByteReceiveCount / (dwElapsedTime / 1000);
}
//
// getPacketReceiveCount
//
// returns the total number of packets that have been received.
// 
DWORD BSocksGameSocket::getPacketReceiveCount() const
{
   return m_dwPacketReceiveCount;
}
//
// getPacketResendRequestCount
//
// returns the number of resend requests made
//
DWORD BSocksGameSocket::getPacketResendRequestCount() const
{
   return m_dwPacketResendRequestCount;
}
//
// getPacketResendCountPerSecond
//
DWORD BSocksGameSocket::getPacketResendRequestCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if ( dwElapsedTime < 1000 )
   {
      return m_dwPacketReceiveCount;
   }
   return m_dwPacketResendRequestCount / (dwElapsedTime / 1000);
}
// 
// getInSequencePacketsReceivedPerSecond
//
// returns the  number of in-sequence packets that were received per second
//
DWORD BSocksGameSocket::getInSequencePacketReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if ( dwElapsedTime < 1000 )
   {
      return m_dwInSequencePacketReceiveCount;
   }
   return m_dwInSequencePacketReceiveCount / (dwElapsedTime / 1000);
}
//
// getInSequencePacketReceivedCount
//
// returns the total number of in-sequence packets received
//
DWORD BSocksGameSocket::getInSequencePacketReceiveCount() const 
{
   return m_dwInSequencePacketReceiveCount;
}
//
// getOutOfSequencePacketsReceivedPerSecond
//
// returns the total number of in sequence packets received.
//
DWORD BSocksGameSocket::getOutOfSequencePacketReceiveCountPerSecond() const
{
   DWORD dwCurrentTime = timeGetTime();
   DWORD dwElapsedTime = dwCurrentTime - m_dwPacketOneSecondDeltaBegin;

   if ( dwElapsedTime < 1000 )
   {
      return m_dwOutOfSequencePacketReceiveCount;
   }
   return m_dwOutOfSequencePacketReceiveCount / (dwElapsedTime / 1000);
}
//
// getOutOfSequencePacketsReceivedPerSecond
//
DWORD BSocksGameSocket::getOutOfSequencePacketReceiveCount() const 
{
   return m_dwOutOfSequencePacketReceiveCount;
}
#endif

//==============================================================================
//
bool BSocksGameSocket::recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress)
{   
   BASSERT(Socket == getSocket());
   BASSERT(Length > 0);

   if (RemoteAddress && (RemoteAddress->sin_addr.S_un.S_addr != mRemoteAddress.sin_addr.S_un.S_addr))
      return false;

   const void *  buf = Buffer;
   DWORD len = Length;

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

      memcpy(buf, ((char *)Buffer)+sizeof(GUID), len);
   #endif

   if ((mForwardingServerAddress.sin_family == AF_INET) && (headerType == BSocksGameHeaderType::cRepeatedPacket))
   {
      //nlog(cTransportNL, "      vv BSocksGameSocket::recvd repeated packet vv");
      // it's a repeated packet, so point to the right places in the buf
      len = len - sizeof(SOCKADDR_IN) - 1;
      RemoteAddress = (SOCKADDR_IN *)((char *)buf+1);
      buf = (char *)buf+1+sizeof(SOCKADDR_IN);
   }

   nlog(cTransportNL, "      BSocksGameSocket::recvd SOCKET[%p] Length %ld, mState %ld, getHeaderType(buf) %ld, from %s:%ld", this, Length, mState, headerType, inet_ntoa(RemoteAddress->sin_addr), htons(RemoteAddress->sin_port));

   /*IN_ADDR addr;
   addr.S_un.S_addr = RemoteAddress->sin_addr.S_un.S_addr; //ntohl(RemoteAddress->sin_addr.S_un.S_addr);

   printf("Got packet from %s, %d\n", inet_ntoa(addr), RemoteAddress->sin_port);*/

   // is this a hello request?
   if ((mState != cDisconnected) && (headerType == BSocksGameHeaderType::cHello))
      return recvHello(Socket, buf, len, RemoteAddress);
   else if ((mState == cConnecting) && (headerType == BSocksGameHeaderType::cHelloResponse))
      return recvHelloResponse(Socket, buf, len, RemoteAddress);
   else if ((mState == cConnected || mState == cConnecting) && (headerType == BSocksGameHeaderType::cDisconnect))
   {
      unsigned short *socketID = (unsigned short *)(((char *)buf)+1);
      if (*socketID != mRemoteSocketID)
         return false;

      if (mDisconnecting)
         return true;

      mState = cDisconnected;
      nlog(cTransportNL, "BSocksGameSocket[%p] set state to %ld because of cDisconnect", this, mState);

      mConnected = false;
      memset(&mForwardingServerAddress, 0, sizeof(mForwardingServerAddress));

      if (getObserver())
         getObserver()->disconnected(this, 0); // FIXME: Status 0?

      return true;
   }
   else if (
               (mState == cConnected) && 
               (
                  (headerType == BSocksGameHeaderType::cData) || 
                  (headerType == BSocksGameHeaderType::cDataFragment) || 
                  (headerType == BSocksGameHeaderType::cDataFragmentDone) || 
                  (headerType == BSocksGameHeaderType::cAck) ||
                  (headerType == BSocksGameHeaderType::cAckRemote) ||
                  (headerType == BSocksGameHeaderType::cResendRequest)
               )
           )
      return recvData(Socket, buf, len, voiceLength, RemoteAddress);

   return false;
}

//==============================================================================
//
void BSocksGameSocket::addResendRequest(unsigned short curReq)
{
   // do we already have this one?
   long j;
   for (j=0;j<mResendRequestVector.getNumber();j++)
   {
      if (mResendRequestVector[j].SeqNum == curReq)
         break;
   }

   // if not, add it
   if (j >= mResendRequestVector.getNumber())
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
void BSocksGameSocket::calcResendRequests(long actualRemoteSeqNum)
{
   // mRemoteSeqNum is the next sequence number we're expecting
   // and not the last one we've actually received
   unsigned short curReq = mRemoteSeqNum-1;
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
      long distance = 0;
      if ((curReq > actualRemoteSeqNum) && ((curReq - actualRemoteSeqNum) > cQueueSize))
      {
         // we've wrapped, need to compensate
         distance = calcDistance(curReq, (unsigned short)actualRemoteSeqNum);
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
      for (long i = 0; i < distance; ++i, ++curReq)
      {
         bool skip = false;
         // do we have this packet?
         BHandle item;
         BSocksGameRecvBuffer *rbuf = mRcvdList.getHead(item);
         while (rbuf)
         {
            if ((rbuf)->SeqNum == curReq)
            {
               skip = true;
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
      BSocksGameRecvBuffer *i = mRcvdList.getHead(item);
      while(i)
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
HRESULT BSocksGameSocket::connectForwarded (
         IN CONST SOCKADDR_IN * ForwarderAddress,
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL) 
{
   mForwardingServerAddress = *ForwarderAddress;
   return connect(RemoteAddress, LocalAddress);
}

//==============================================================================
//
HRESULT BSocksGameSocket::generateResendRequests(void *buffer, DWORD *size)
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
   BSocksGameResendRequest packet; 
   packet.Requests = (unsigned short *)(((char *)buffer)+sizeof(short));
   packet.NumberOfRequests = 0;

   // need to determine the number of requests that I can place in the buffer
   // based on the *size value
   long maxRequests = (*size - sizeof(short)) / sizeof(short);

   for (long i=0; i < mResendRequestVector.getNumber() && i < maxRequests; i++)
   {      
      if ((timeGetTime() - mResendRequestVector[i].SentTime) < cResendRequestInterval)
         continue;

      if (packet.NumberOfRequests > BSocksGameResendRequest::cMaxResendRequests)
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
   BASSERT(sizeof(packet.NumberOfRequests)+(packet.NumberOfRequests*sizeof(short)) < *size);
   if (sizeof(packet.NumberOfRequests)+(packet.NumberOfRequests*sizeof(short)) > *size)
      return E_FAIL;

   *size = sizeof(packet.NumberOfRequests)+(packet.NumberOfRequests*sizeof(short));
   XMemCpy(buffer, &packet.NumberOfRequests, sizeof(short));   

   return S_OK;
}

//==============================================================================
//
long BSocksGameSocket::calcDistance(unsigned short seqNum1, unsigned short seqNum2)
{   
   if (seqNum2 > seqNum1)
      return seqNum2 - seqNum1;         
   else
      return (65535-seqNum1)+seqNum2;
}

//==============================================================================
//
void BSocksGameSocket::updateRcvdList(void)
{
   // parse through rcvd list and free any messages that are in the correct sequence
   BHandle item;
   HRESULT hr = E_FAIL;
   BSocksGameRecvBuffer *i = mRcvdList.getHead(item);
   while(i)
   {      
      if ((i)->SeqNum == mRemoteSeqNum)
      {
         if (getObserver())
         {
            if (  ((i)->Type == BSocksGameHeaderType::cDataFragment) ||
                  ((i)->Type == BSocksGameHeaderType::cDataFragmentDone))
            {
               if ((sizeof(mRecvFragmentBuffer) - 1 - mRecvFragmentBufferPtr) < (i)->Size)
               {
                  nlog(cTransportNL, "BSocksGameSocket::updateRcvdList -- not enough buffer space for fragment.");
                  BFAIL("BSocksGameSocket::updateRcvdList -- Leave this up and call mike @ x286");
                  return;
               }

               memcpy(mRecvFragmentBuffer+mRecvFragmentBufferPtr, (i)->Buffer, (i)->Size);
               mRecvFragmentBufferPtr+=(i)->Size;
               if ((i)->Type == BSocksGameHeaderType::cDataFragmentDone)
               {
                  nlog(cTransportNL, "          in seq, release");
                  getObserver()->recvd(this, mRecvFragmentBuffer, mRecvFragmentBufferPtr, 0, &(i)->RemoteAddress);
                  mRecvFragmentBufferPtr = 0;
               }
               else
                  nlog(cTransportNL, "          got a fragment");
            }
            else
            {
               nlog(cTransportNL, "      freeing old message %ld", (i)->SeqNum);
               getObserver()->recvd(this, (i)->Buffer, (i)->Size, 0, &(i)->RemoteAddress);            
            }

            if (isDisposing())
               return;    
            else
            {
               hr = freeRecvBuffer(i);
               BASSERT(SUCCEEDED(hr));
            }
         }
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
HRESULT BSocksGameSocket::sendWithPiggyBacks (
         IN BSocksGameSendBuffer * SendBuffer,
         IN BOOL Flush,
         IN BOOL Resends)
{
   if (!getSocket())
   {
      nlog(cTransportNL, "BSocksGameSocket::sendWithPiggyBacks -- no socket.");
      return E_FAIL;   
   }

   // XXX manipulating the buffer here will require a changing of how the SendBuffer is dealt with below

   DWORD dataIndex = mDataIndexes[mBufferSet];
   DWORD voiceIndex = mVoiceIndexes[mBufferSet];

   unsigned char* dataBuffer = mDataBuffers[mBufferSet];
   unsigned char* voiceBuffer = mVoiceBuffers[mBufferSet];

   if (!dataBuffer)
      return E_FAIL;

   DWORD sendBufferLen = 0;

   if (SendBuffer)
   {
      sendBufferLen = SendBuffer->Length + sizeof(BSocksGameHeader);

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

            XMemCpy(mVoiceBuffers[bufferB] + mVoiceIndexes[bufferB], SendBuffer->Header, sendBufferLen);

            mVoiceIndexes[bufferB] += sendBufferLen;
         }
         else
         {
            // sanity check?
            if (cMaxSendSize - mDataIndexes[bufferB] < sendBufferLen)
               return E_FAIL;

            XMemCpy(mDataBuffers[bufferB] + mDataIndexes[bufferB], SendBuffer->Header, sendBufferLen);

            mDataBuffers[bufferB] += sendBufferLen;
         }

         // I need to flush the buffers now
         Flush = TRUE;
      }
      else
      {
         if (SendBuffer->Flags & cSendUnencrypted)
         {
            XMemCpy(voiceBuffer + voiceIndex, SendBuffer->Header, sendBufferLen);

            voiceIndex += sendBufferLen;

            mVoiceIndexes[mBufferSet] = voiceIndex;
         }
         else
         {
            XMemCpy(dataBuffer + dataIndex, SendBuffer->Header, sendBufferLen);

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
   DWORD size = sizeof(BSocksGameResendRequest)+(BSocksGameResendRequest::cMaxResendRequests*sizeof(short));
   // make room for two packets if needed
   DWORD length = 0;
   //if (SendBuffer)
   //   length+=SendBuffer->Length;
   length = dataIndex + voiceIndex;

   HRESULT hr = getSocket()->sendAllocateBuffer( size + length + (sizeof(BSocksGameHeader)*2) + sizeof(short), &sbuf);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "BSocksGameSocket::sendWithPiggyBacks -- failed to allocate buffer.");
      return hr;
   }

   unsigned char *ptr = (unsigned char*)sbuf->Buffer;
   const unsigned char * const dataPtr = ptr;

   if (dataIndex > 0)
   {
      XMemCpy(ptr, dataBuffer, dataIndex);

      ptr += dataIndex;

      mDataIndexes[mBufferSet] = 0;
   }
   else if (voiceIndex == 0)
   {
      // no data and no voice
      // just an ack
      BSocksGameHeader hdr;
      hdr.Type = BSocksGameHeaderType::cAck;
      hdr.Size = 0;
      hdr.SocketID = mLocalSocketID;
      // if our last packet had a seq number of 65535
      // and then we incremented that number, it would rollover to 0
      // if we then send an ack with 0, the other client(s) will think
      // we actually sent something that we haven't?
      if (mLocalSeqNum > 0)
         hdr.SeqNum = (unsigned short)(mLocalSeqNum-1);  // make sure the client knows what the last packet we sent out is, in case
                                       // he needs to request resends and we haven't sent him any real packets to let
                                       // him know that
      else if (mSentData)
         hdr.SeqNum = 65535;
      else
         hdr.SeqNum = 0;

      XMemCpy(ptr, &hdr, sizeof(BSocksGameHeader));
      ptr+=sizeof(BSocksGameHeader);
   }

   // ------------------------------------
   // #1: piggyback any resend requests
   hr = generateResendRequests(ptr+sizeof(BSocksGameHeader), &size);   
   BASSERT(SUCCEEDED(hr));

   // size is data length, plus the size of a header, plus the size of resends, plus the size of the ack at the end   
   if (size > 0)
   {
      // fill in that second header if needed
      sbuf->Length += sizeof(BSocksGameHeader);      
      BSocksGameHeader *hdr = reinterpret_cast<BSocksGameHeader *>(ptr);
      hdr->SeqNum = 0; // not used for resend requests
      hdr->Size = (unsigned short)size;
      hdr->SocketID = mLocalSocketID;
      hdr->Type = BSocksGameHeaderType::cResendRequest;

      ptr += size+sizeof(BSocksGameHeader);
   }

   // ------------------------------------
   // #2: piggyback the ack
   *ptr = BSocksGameHeaderType::cAckRemote; ptr++;
   XMemCpy(ptr, &mRemoteSeqNum, sizeof(short));
   ptr+=sizeof(short);

   // ------------------------------------
   // #3: calculate the game data size
   DWORD dataLen = ptr - dataPtr;

   // ------------------------------------
   // #4: append any unencrypted data
   if (voiceIndex > 0)
   {
      XMemCpy(ptr, voiceBuffer, voiceIndex);

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

   nlog(cTransportNL, "      BSocksGameSocket::sendWithPiggyBacks - Length %ld, acking %ld", sbuf->Length, mRemoteSeqNum);
   if (SendBuffer)
      nlog(cTransportNL, "        SendBuffer->Header->SeqNum %ld", SendBuffer->Header->SeqNum);   

   hr = _sendTo(sbuf, &mRemoteAddress);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "BSocksGameSocket::sendWithPiggyBacks -- failed _sendTo hr 0x%x", hr);
      return hr;
   }

   return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::send (
         IN BSendBuffer * SendBuffer)
{  
   HRESULT hr = E_FAIL;
   if (mState != cConnected)
   {
      hr = sendFreeBuffer(SendBuffer);
      BASSERT(SUCCEEDED(hr));
      nlog(cTransportNL, "BSocksGameSocket::send -- not connected.");
      return E_FAIL;
   }

   BASSERT(SendBuffer->Length < cMaxSendSize);
   if (SendBuffer->Length > cMaxSendSize)
   {
      nlog(cTransportNL, "      BSocksGameSocket::send ERROR Tried to send packet with size %ld", SendBuffer->Length);
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
         memcpy(tempSendBuffer->Buffer, ((char *)SendBuffer->Buffer)+dataDone, sendSize);
         BSocksGameSendBuffer *tempSocksBuffer = static_cast<BSocksGameSendBuffer *>(tempSendBuffer);
         tempSocksBuffer->Header->SeqNum = mLocalSeqNum++; mSentData = TRUE;
         tempSocksBuffer->Header->Size = (unsigned short)sendSize;
         tempSocksBuffer->Header->SocketID = mLocalSocketID;
         if ((dataLeft - sendSize) > 0)
            tempSocksBuffer->Header->Type = BSocksGameHeaderType::cDataFragment;
         else
            tempSocksBuffer->Header->Type = BSocksGameHeaderType::cDataFragmentDone;

         hr = sendWithPiggyBacks(tempSocksBuffer, mFlushSendBuffers, mIncludeResendRequest);
         if(FAILED(hr))
            nlog(cTransportNL, "BSocksGameSocket::send -- failed sendWithPiggyBacks 1.");

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
      BSocksGameSendBuffer *buf = static_cast<BSocksGameSendBuffer *>(SendBuffer);   

      mSentData = TRUE;
      buf->Header->Size = (unsigned short)SendBuffer->Length; 
      buf->Header->SocketID = mLocalSocketID;
      buf->Header->Type = BSocksGameHeaderType::cData;

      if (SendBuffer->Flags & cSendUnreliable)
      {
         buf->Header->SeqNum = 0;
         buf->Header->Type |= 0x80;
      }
      else
         buf->Header->SeqNum = mLocalSeqNum++;

      hr = sendWithPiggyBacks(buf, mFlushSendBuffers, mIncludeResendRequest);
      if(FAILED(hr))
         nlog(cTransportNL, "BSocksGameSocket::send -- failed sendWithPiggyBacks 2.");

      buf->SendTime = timeGetTime();
      mSentList.addToTail(buf);
   }

	return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::sendFreeBuffer (
         IN BSendBuffer * SendBuffer)
{
	return freeSendBuffer((BSocksGameSendBuffer *)SendBuffer);
}

//==============================================================================
//
HRESULT BSocksGameSocket::close (void)
{
   nlog(cTransportNL, "BSocksGameSocket[%p]::close", this);
   stopTimer(cConnectTimer);
   stopTimer(cUpdateTimer);

	return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::getLocalAddress (
         OUT SOCKADDR_IN * ReturnLocalAddress)
{
   *ReturnLocalAddress = mLocalAddress;
	return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::getRemoteAddress (
         OUT SOCKADDR_IN * ReturnRemoteAddress)
{
   *ReturnRemoteAddress = mRemoteAddress;
	return S_OK;
}

//
// For now we allocate a new buffer on every request.
// The API is designed to allow us to cache requests in a buffer list, eventually.
//

//==============================================================================
//
HRESULT BSocksGameSocket::allocateSendBuffer (
   IN DWORD MaximumLength,
   OUT BSocksGameSendBuffer ** ReturnSendBuffer)
{

   BASSERT (ReturnSendBuffer != NULL);
   *ReturnSendBuffer = NULL;

   DWORD AllocationLength;
   BSocksGameSendBuffer * SendBuffer;

   AllocationLength = MaximumLength + sizeof (BSocksGameSendBuffer) + sizeof (BSocksGameHeader);

   // allocate this memory from the gNetworkHeap
   LPBYTE MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
   if (MemoryBlock == NULL)
   {
      BFAIL("gNetworkHeap: Out of memory");
      return E_OUTOFMEMORY;
   }

   SendBuffer = reinterpret_cast <BSocksGameSendBuffer *> (MemoryBlock);
   SendBuffer -> Header = reinterpret_cast <BSocksGameHeader *> (MemoryBlock + sizeof (BSocksGameSendBuffer));
   SendBuffer -> Buffer = MemoryBlock + sizeof (BSocksGameSendBuffer) + sizeof (BSocksGameHeader);

   SendBuffer -> Flags = 0;
   SendBuffer -> MaximumLength = MaximumLength;
   SendBuffer -> Length = 0;
   SendBuffer -> VoiceLength = 0;
   SendBuffer -> SendTime = timeGetTime();

   *ReturnSendBuffer = SendBuffer;

   return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::freeSendBuffer (
   IN BSocksGameSendBuffer * SendBuffer)
{
   if (SendBuffer != NULL)
   {
      gNetworkHeap.Delete(SendBuffer);
   }

   return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::allocateRecvBuffer (         
         OUT BSocksGameRecvBuffer ** ReturnBuffer, DWORD size)
{
   BASSERT(ReturnBuffer != NULL);

   DWORD AllocationLength = sizeof(BSocksGameRecvBuffer) + size;

   LPBYTE MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
   if (MemoryBlock == NULL)
   {
      BFAIL("gNetworkHeap: Out of memory");
      return E_OUTOFMEMORY;
   }

   *ReturnBuffer = reinterpret_cast<BSocksGameRecvBuffer *>(MemoryBlock);   
   (*ReturnBuffer)->Buffer = MemoryBlock+sizeof(BSocksGameRecvBuffer);

   return S_OK;
}

//==============================================================================
//
HRESULT BSocksGameSocket::freeRecvBuffer (
   IN BSocksGameRecvBuffer * Buffer)
{
   if (Buffer != NULL)
      gNetworkHeap.Delete(Buffer);

   return S_OK;
}

void BSocksGameSocket::setUnresponsiveTimeoutValue(DWORD dwValue)
{
   m_dwUnresponsiveTimeoutValue = dwValue;
}
