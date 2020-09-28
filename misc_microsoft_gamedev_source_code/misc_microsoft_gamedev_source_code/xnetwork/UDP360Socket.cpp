//==============================================================================
// BUDP360Socket.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "UDP360Socket.h"
#include "SocksHelper.h"

//xsystem
#include "timelineprofiler.h"

//==============================================================================
// Defines
int BUDP360Socket::mNumActiveSockets = 0;
BUDP360Socket* BUDP360Socket::mpActiveSockets[BUDP360Socket::MaxActiveSockets];

//==============================================================================
// 
//==============================================================================
BUDP360Socket::BUDP360Socket(IN BObserver* pObserver, bool enableVoice) :
   BSocket(pObserver),
   mLastRecvDataTime(0),
#ifndef BUILD_FINAL
   mpTracker(NULL),
#endif
   mEventMask(0),
   mEventHandle(WSA_INVALID_EVENT),
   mSocket(INVALID_SOCKET),
   mSendMessages(false),
   mEnableVoice(enableVoice)
{
   Utils::FastMemSet(&mLocalAddress, 0, sizeof(SOCKADDR_IN));

#ifndef BUILD_FINAL
   mpTracker = new BNetStatsTracker(cBandwidthTotalsNL, "BUDP360Socket");
#endif
}

//==============================================================================
// 
//==============================================================================
BUDP360Socket::~BUDP360Socket()
{
   dispose();

   if (WSA_INVALID_EVENT != mEventHandle)
   {  
      if (INVALID_SOCKET != mSocket)
         WSAEventSelect(mSocket, mEventHandle, 0);
      WSACloseEvent(mEventHandle);
      mEventHandle = WSA_INVALID_EVENT;
   }
}

//==============================================================================
// 
HRESULT BUDP360Socket::dispose(void)
{
   //Dump all the mappings
   mIPMapping.clear();

   close();

   BASSERT (mSocket == INVALID_SOCKET);

   checkForEvents(false);
   mSendMessages = false;

   BASSERT (mSocket == INVALID_SOCKET);   

#ifndef BUILD_FINAL
   if (mpTracker)
      delete mpTracker;
   mpTracker=NULL;
#endif

   return BSocket::dispose();
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Socket::createAndBind(IN u_short port)
{   
   if (mSocket != INVALID_SOCKET)
   {
      //
      // Already got one.
      //

      nlog (cTransportNL, "BUDP360Socket::createAndBind: cannot create a new socket, already got one");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   HRESULT Result = mRecv.grow (DefaultRecvAllocationLength);
   if (FAILED (Result))
   {
      return Result;
   }

   //TODO - change this to _VDP - we always want that for halo wars
   mSocket = socket(AF_INET, SOCK_DGRAM, (mEnableVoice ? IPPROTO_VDP : IPPROTO_UDP));

   if (mSocket == INVALID_SOCKET)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::createAndBind failed to create socket");
      return Result;
   }


   BOOL OptionValue = TRUE;
   if (setsockopt (mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast <char *> (&OptionValue), sizeof OptionValue) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::createAndBind failed to set SO_BROADCAST on socket, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   //No more reuse on addr bind
   /*
   OptionValue = TRUE;
   if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <char *> (&OptionValue), sizeof OptionValue) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket: failed to set SO_REUSEADDR");
      return Result;
   }
   */

   //Build the address to bind againist
   ZeroMemory(&mLocalAddress, sizeof SOCKADDR_IN );
   mLocalAddress.sin_family = AF_INET;
   mLocalAddress.sin_port = port;
   mLocalAddress.sin_addr.s_addr = INADDR_ANY;
   
   if (bind (mSocket, reinterpret_cast <SOCKADDR *> (&mLocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::createAndBind failed to bind UDP/IP socket to port %d 0x%08x", htons (mLocalAddress.sin_port));
      nlogError (cTransportNL, Result);
      closesocket (mSocket);
      mSocket = INVALID_SOCKET;
      return Result;
   }

   INT SocketAddressLength = sizeof (SOCKADDR_IN);
   if (getsockname (mSocket, reinterpret_cast <SOCKADDR *> (&mLocalAddress), &SocketAddressLength) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::createAndBind failed to query local UDP/IP socket address, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   mConnected=true;

   return asyncSelect (FD_READ | FD_WRITE);

}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Socket::init(ushort port)
{   
   //if (mSocket != INVALID_SOCKET)
   //{
   //   // Already got one.
   //   nlog (cTransportNL, "BUDP360Socket::init : cannot create a new socket, already got one");
   //   return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   //}

   if (mSocket != INVALID_SOCKET)
   {
      asyncStop();

      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      mConnected = false;
   }

   HRESULT result = mRecv.grow(DefaultRecvAllocationLength);
   if (FAILED(result))
   {
      return result;
   }

   // this one can stay UDP
   mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if (mSocket == INVALID_SOCKET)
   {
      result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::init : failed to create socket");
      return result;
   }

   Utils::FastMemSet(&mLocalAddress, 0, sizeof(SOCKADDR_IN));

   mLocalAddress.sin_family      = AF_INET;
   mLocalAddress.sin_addr.s_addr = INADDR_ANY;
   mLocalAddress.sin_port        = htons(port);

   if (bind(mSocket, reinterpret_cast<SOCKADDR*>(&mLocalAddress), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
   {
      result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::init : failed to bind UDP/IP socket to port %d 0x%08x", port);
      nlogError (cTransportNL, result);
      closesocket (mSocket);
      mSocket = INVALID_SOCKET;
      return result;
   }

   DWORD dwNonBlocking = 1;
   int32 iResult = ioctlsocket(mSocket, FIONBIO, &dwNonBlocking);
   if (iResult == SOCKET_ERROR)
   {
      result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket::init : ioctlsocket failed with %d", iResult);
      return result;
   }

   mConnected = true;

   return asyncSelect(FD_READ | FD_WRITE);
}

//==============================================================================
// 
//==============================================================================
bool BUDP360Socket::addIPMappedObserver( IN_ADDR* targetIP, BObserver* targetContext )
{
   if (findIPMappedObserver(targetIP) != NULL)
   {
      //its already in the list
      return false;
   }

   mIPMapping.add( BUDP360SocketIPMapping( targetIP, targetContext ));
   return true;
}

void BUDP360Socket::removeIPMappedObserver( IN_ADDR* targetIP )
{
   for (long i=0;i<mIPMapping.getNumber();i++)
   {
      if (mIPMapping[i].mTargetIP.s_addr == targetIP->s_addr )
      {
         mIPMapping.removeIndex(i);
         return;
      }
   }
}

BSocket::BObserver* BUDP360Socket::findIPMappedObserver( IN_ADDR* targetIP )
{
   for (long i=0;i<mIPMapping.getNumber();i++)
   {
      if (mIPMapping[i].mTargetIP.s_addr == targetIP->s_addr )
      {
         return mIPMapping[i].mTargetContext;
      }
   }
   return NULL;
}



//==============================================================================
// 
HRESULT BUDP360Socket::close(void)
{
   if (mSocket == INVALID_SOCKET)
   {
      return S_OK;
   }

   //
   // This will also cancel any pending I/O.
   //

   asyncStop();

   closesocket (mSocket);
   mSocket = INVALID_SOCKET;
   mConnected=false;
   if (getObserver())
   {
      getObserver()->disconnected(this, 0);
   }

   return S_OK;
}


//
// For now we allocate a new buffer on every request.
// The API is designed to allow us to cache requests in a buffer list, eventually.
//

HRESULT BUDP360Socket::allocateBuffer (
   IN DWORD MaximumLength,
   OUT BUDP360SocketBuffer ** ReturnSendBuffer)
{

   DWORD AllocationLength;
   BUDP360SocketBuffer * SendBuffer;

   AllocationLength = MaximumLength + sizeof (BUDP360SocketBuffer);

   LPBYTE MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
   if (MemoryBlock == NULL)
   {
      BFAIL("gNetworkHeap: Out of memory");
      return E_OUTOFMEMORY;
   }

   SendBuffer = reinterpret_cast <BUDP360SocketBuffer *> (MemoryBlock);
   SendBuffer -> Buffer = MemoryBlock + sizeof (BUDP360SocketBuffer);
   SendBuffer -> MaximumLength = MaximumLength;
   SendBuffer -> Length = 0;
   SendBuffer -> Socket = this;

   *ReturnSendBuffer = SendBuffer;

   return S_OK;
}

HRESULT BUDP360Socket::freeBuffer (
   IN BUDP360SocketBuffer * SendBuffer)
{
   if (SendBuffer != NULL)
   {
      gNetworkHeap.Delete(SendBuffer);
   }

   return S_OK;
}



//
// Send State Machine
//

HRESULT BUDP360Socket::sendAllocateBuffer (
   IN DWORD MaximumLength,
   OUT BSendBuffer ** ReturnSendBuffer)
{
   BASSERT (ReturnSendBuffer != NULL);

   BUDP360SocketBuffer * Buffer;
   HRESULT Result;

   Result = allocateBuffer (MaximumLength, &Buffer);
   *ReturnSendBuffer = Buffer;

   Buffer->Flags = 0;
   Buffer->Length = 0;
   Buffer->VoiceLength = 0;

   return Result;
}

HRESULT BUDP360Socket::sendFreeBuffer (
   IN BSendBuffer * SendBuffer)
{
   return freeBuffer (static_cast <BUDP360SocketBuffer *> (SendBuffer));
}

/*
HRESULT BUDP360Socket::send (
                                      IN BSendBuffer * SendBuffer)
{
   if (SendBuffer == NULL)
   {
      return E_INVALIDARG;
   }

   if (mRemoteAddress.sin_family == AF_INET)
   {      
      return sendTo (SendBuffer, &mRemoteAddress);
   }
   else
   {
      sendFreeBuffer (SendBuffer);
      nlog (cTransportNL, "BUDP360Socket::send: remote address not yet set");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }
}
*/



HRESULT BUDP360Socket::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * DestinationAddress)
{ 
   BSerialBuffer sb;
   packet.serialize(sb);

   BSendBuffer *buf=0;
   sendAllocateBuffer(sb.getBufferSize(), &buf);
   buf->Length = sb.getBufferSize();
   memcpy(buf->Buffer, sb.getBuffer(), buf->Length);

   return sendTo(buf, DestinationAddress);
}


HRESULT BUDP360Socket::broadcastLAN( IN BSendBuffer * SendBufferBase, WORD port )
{
   if (SendBufferBase == NULL )
   {
      return E_INVALIDARG;
   }

   BUDP360SocketBuffer * SendBuffer = static_cast <BUDP360SocketBuffer *> (SendBufferBase);

   if (mSocket == INVALID_SOCKET)
   {
      nlog (cTransportNL, "BUDP360Socket: socket is closed, can't send");
      sendFreeBuffer (SendBuffer);
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   BASSERT (SendBuffer != NULL);
   BASSERT (SendBuffer->Length <= SendBuffer->MaximumLength);
   BASSERT (SendBuffer->Socket == this);

   //TODO - it would be prettier if we had an option on the socket creation to support vdp voice or be udp non voice
   // copy the buffer into a temp buffer and assign the cbGameData value
   // VDP traffic is [cbGameData][GameData][VoiceData]
   // 2 bytes for cbGameData that specifies the length of GameData
   // VoiceData can contain our headers but they must only be related to voice traffic
   int sendLen;
   XMemCpy(&mSendBuffer[2], SendBuffer->Buffer, SendBuffer->Length);
   WORD cbGameData = (WORD)(SendBuffer->Length - SendBuffer->VoiceLength);
   memcpy(mSendBuffer, &cbGameData, sizeof(WORD));
   sendLen = 2 + SendBuffer->Length;

   SOCKADDR_IN sa;
   sa.sin_family      = AF_INET;                                            
   sa.sin_addr.s_addr = INADDR_BROADCAST;   
   sa.sin_port        = htons( port );     

   INT BytesTransferred = 0;

   BytesTransferred = ::sendto (
         mSocket,
         mSendBuffer,
         sendLen,
         0,
         (const sockaddr*)&sa, 
         sizeof(sa) );
 

   nlog(cTransportNL, "BUDP360Socket: sent broadcast LAN packet, length %ld", sendLen);

   sendFreeBuffer (SendBuffer);

   if (BytesTransferred == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket: broadcast send failed - Last Error: %d", GetLastError());
      return Result;
   }

#ifndef BUILD_FINAL
   mpTracker->postSent(BytesTransferred);
#endif

   return S_OK;
}


HRESULT BUDP360Socket::sendTo ( IN BSendBuffer * SendBufferBase, IN CONST SOCKADDR_IN * DestinationAddress)
{
   if (SendBufferBase == NULL || DestinationAddress == NULL)
   {
      return E_INVALIDARG;
   }

   BUDP360SocketBuffer * SendBuffer = static_cast <BUDP360SocketBuffer *> (SendBufferBase);

   if (mSocket == INVALID_SOCKET)
   {
      nlog (cTransportNL, "BUDP360Socket: socket is closed, can't send");
      sendFreeBuffer (SendBuffer);
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   /*#ifndef BUILD_FINAL
   const long cDropPercentage = 10;
   if (rand()%100 < cDropPercentage)
   {
   // let's randomly drop some packets
   sendFreeBuffer (SendBuffer);
   return S_OK;
   }
   #endif*/

   BASSERT (SendBuffer != NULL);
   BASSERT (SendBuffer->Length <= SendBuffer->MaximumLength);
   BASSERT (SendBuffer->Socket == this);

   memcpy( &SendBuffer->RemoteAddress, DestinationAddress, sizeof( SOCKADDR_IN ));
   //SendBuffer->RemoteAddress = *DestinationAddress;

   mSend.mQueueList.addToTail(SendBuffer);

   HRESULT Result = sendNextBuffer ();

   return Result;
}

HRESULT BUDP360Socket::sendNextBuffer (void)
{
   int sendLen;

   BUDP360SocketBuffer * SendBuffer = mSend.mQueueList.removeHead();
   while (SendBuffer)
   {
      // copy the buffer into a temp buffer and assign the cbGameData value
      // VDP traffic is [cbGameData][GameData][VoiceData]
      // 2 bytes for cbGameData that specifies the length of GameData
      // VoiceData can contain our headers but they must only be related to voice traffic
      XMemCpy(&mSendBuffer[2], SendBuffer->Buffer, SendBuffer->Length);
      WORD cbGameData = (WORD)(SendBuffer->Length - SendBuffer->VoiceLength);
      memcpy(mSendBuffer, &cbGameData, sizeof(WORD));
      sendLen = 2 + SendBuffer->Length;

      INT BytesTransferred = 0;

      {
         SCOPEDSAMPLE(BSocksUnreliableSocket_sendNextBuffer_sendto)

            BytesTransferred = ::sendto (
            mSocket,
            mSendBuffer,
            sendLen,
            0,
            &SendBuffer->RemoteAddress,
            //reinterpret_cast <SOCKADDR *> (&SendBuffer->RemoteAddress),
            sizeof (SendBuffer->RemoteAddress));
      }

      nlog (cTransportNL, "BUDP360Socket: sent packet, length %ld",         
         SendBuffer -> Length);

      if (BytesTransferred == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
      {
         mSend.mQueueList.addToHead(SendBuffer);

         nlog (cTransportNL, "BUDP360Socket: can't send buffer now, will try later when buffer space is available");
         return S_OK;
      }

      sendFreeBuffer (SendBuffer);

      if (BytesTransferred == SOCKET_ERROR)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BUDP360Socket: failed to send buffer on udp socket. Last Error: %d", GetLastError());
         nlogError (cTransportNL, Result);
         return Result;
      }

#ifndef BUILD_FINAL
      mpTracker->postSent(BytesTransferred);
#endif

      SendBuffer = mSend.mQueueList.removeHead();
   }

   return S_OK;
}

void BUDP360Socket::sendReady (void)
{
   HRESULT Result = sendNextBuffer ();

   if (FAILED (Result))
   {
      networkClose (Result);
   }
}

//
// Receive State Machine
//
//

BOOL BUDP360Socket::recvReady (void)
{
   if (mRecv.Buffer == NULL)
   {
      nlog (cTransportNL, "BUDP360Socket: receive buffer is NULL, cannot receive data");
      return FALSE;
   }

   SOCKADDR_IN RemoteAddress;
   INT RemoteAddressLength = sizeof RemoteAddress;

   INT BytesTransferred = 0;

   {
      SCOPEDSAMPLE(BSocksUnreliableSocket_recvReady_recvfrom)

         BytesTransferred = ::recvfrom (
         mSocket,
         reinterpret_cast <PCHAR> (mRecv.Buffer),
         mRecv.MaximumLength,
         0,
         reinterpret_cast <SOCKADDR *> (&RemoteAddress),
         &RemoteAddressLength);
   }

   if (BytesTransferred == SOCKET_ERROR)
   {
      if (GetLastError() == WSAEWOULDBLOCK)
      {
         //This next log line just generates lots of spam
         //nlog (cTransportNL, "BUDP360Socket: winsock can't make up its mind whether we have data or not :\\");
         return FALSE;
      }

      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BUDP360Socket: receive failed (could be ICMP port unreachable message)");
      nlogError (cTransportNL, Result);      
      return FALSE;
   }

   if ( BytesTransferred == 0 )
      return FALSE;

   //I have data - update my last recv time
   mLastRecvDataTime = timeGetTime();


#ifndef BUILD_FINAL
   mpTracker->postRcvd(BytesTransferred);
#endif

   //
   // Callback can result in destruction; do not touch object after this call.
   //

   nlog (cTransportNL, "--- BUDP360Socket: recvd - Socket [%p], Length %ld, RemoteAddress %s:%ld", this, BytesTransferred, inet_ntoa(RemoteAddress.sin_addr), ntohs(RemoteAddress.sin_port));

   if (BytesTransferred < 2)
   {
      return TRUE;
   }

   // extract the cbGameData value
   WORD cbGameData = 0;
   memcpy(&cbGameData, mRecv.Buffer, sizeof(WORD));

   if (BytesTransferred < cbGameData)
   {
      return TRUE;
   }

   IN_ADDR remoteInAddr;
   remoteInAddr.s_addr = RemoteAddress.sin_addr.s_addr;
   BObserver* observer = findIPMappedObserver( &remoteInAddr );
   if (observer)
   {
      observer->recvd(this, mRecv.Buffer+2, BytesTransferred-2, BytesTransferred-cbGameData-2, &RemoteAddress);
   }
   else
   {
      //No mapped observer - use the master observer
      if (getObserver())
      {
         getObserver()->recvd(this, mRecv.Buffer+2, BytesTransferred-2, BytesTransferred-cbGameData-2, &RemoteAddress);
      }
   }

   return TRUE;
}

//==============================================================================
// 
//==============================================================================
HRESULT BUDP360Socket::asyncSelect(IN DWORD EventMask)
{
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
      if (mpActiveSockets[i] == this)
         break;

   if (i == mNumActiveSockets)
   {
      BASSERT(mNumActiveSockets < MaxActiveSockets);
      mpActiveSockets[mNumActiveSockets++] = this;
   }         

   if (mSocket != INVALID_SOCKET)
   {
      if (WSA_INVALID_EVENT == mEventHandle)
      {
         mEventHandle = WSACreateEvent();

         if (WSA_INVALID_EVENT == mEventHandle)
         {
            HRESULT Result = GetLastResult();
            nlog (cTransportNL, "BUDP360Socket::asyncSelect : WSACreateEvent failed");
            nlogError (cTransportNL, Result);
            return Result;  
         }

         if (WSAEventSelect(mSocket, mEventHandle, FD_CLOSE) == SOCKET_ERROR)
         {
            HRESULT Result = GetLastResult();
            nlog (cTransportNL, "BUDP360Socket::asyncSelect : WSAEventSelect failed, will not be able to receive network events");
            nlogError (cTransportNL, Result);
            return Result;  
         }
      }

#if 0
      // Set socket to nonblocking.
      DWORD ioctl_opt = 1;                   
      if (0 != ioctlsocket(mSocket, FIONBIO, &ioctl_opt))
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksSocketBase: ioctlsocket() select failed");
         nlogError (cTransportNL, Result);
         return Result;
      }
#endif
   }

   mSendMessages = true;
   mEventMask = EventMask;

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BUDP360Socket::asyncStop (void)
{
   checkForEvents(false);
   mSendMessages = false;

   removeFromActiveSockets();

   if (WSA_INVALID_EVENT != mEventHandle)
   {  
      if (INVALID_SOCKET != mSocket)
         WSAEventSelect(mSocket, mEventHandle, 0);
      WSACloseEvent(mEventHandle);
      mEventHandle = WSA_INVALID_EVENT;
   }

   /*
   for (int i = 0; i < MaxTimers; i++)
   {
      mTimers[i].mInterval = 0;
      mTimers[i].mLastTick = 0;
   }
   */

   mEventMask = 0;
}


void BUDP360Socket::networkClose (IN HRESULT Result)
{
   UNREFERENCED_PARAMETER (Result);
}


void BUDP360Socket::checkForEvents(bool tickTimers)
{
   if (!mSendMessages)
      return;

   const bool read = (0 != (mEventMask & FD_READ));
   const bool write = (0 != (mEventMask & FD_WRITE));
   const bool close = (0 != (mEventMask & FD_CLOSE));

   if (mSocket != INVALID_SOCKET)
   {
      if (close)
      {
         BASSERT(WSA_INVALID_EVENT != mEventHandle);

         const DWORD waitResult = WaitForSingleObject(mEventHandle, 0);
         if (WAIT_OBJECT_0 == waitResult)
         {
            WSAResetEvent(mEventHandle);
            networkClose (S_OK);
         }
      }         
   }

   if (mSocket != INVALID_SOCKET)      
   {
      if (read || write)
      {
         //for ( ; ; )
         //{
         const timeval timeout = { 0, 0 };   

         fd_set read_fd;
         FD_ZERO(&read_fd);
         if (read)
            FD_SET(mSocket, &read_fd);

         fd_set write_fd;
         FD_ZERO(&write_fd);
         if (write)
            FD_SET(mSocket, &write_fd);

         int res = SOCKET_ERROR;

         {
            SCOPEDSAMPLE(BSocksSocketBase_checkForEvents_select)

               res = select(0, read ? &read_fd : NULL, write ? &write_fd : NULL, NULL, &timeout);
         }

         //bool tryAgain = false;
         long count=0;

         if (SOCKET_ERROR != res) 
         {
            if ((read) && (FD_ISSET(mSocket, &read_fd)))
            {
               while (recvReady() && ++count < 5) {}
               //tryAgain = true;
            }
            if ((write) && (mSocket != INVALID_SOCKET) && (FD_ISSET(mSocket, &write_fd)))
               sendReady();
         }

         //   if (!tryAgain)
         //      break;
         //}
      }
   }      

   if (tickTimers)
   {
      //todo - re-implement this original timer system
      /*
      const DWORD curTickCount = GetTickCount();

      for (DWORD i = 0; i < MaxTimers; i++)
      {
         if (mTimers[i].mInterval)
         {
            if ((curTickCount - mTimers[i].mLastTick) >= mTimers[i].mInterval)
            {
               mTimers[i].mLastTick = curTickCount;
               tic(i + 1);
            }
         }
         
      }
      */
      // tick the socket so flush any buffers we have
      // be careful because the previous timer checks may have already ticked the socket
      // hopefully, the previous tick will have added to the existing buffers and
      // then we can flush them here
      //tic(0);
   }
}

void BUDP360Socket::removeFromActiveSockets(void)
{
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
   {  
      if (mpActiveSockets[i] == this)
      {
         mpActiveSockets[i] = mpActiveSockets[mNumActiveSockets - 1];
         break;
      }
   }

   if (i < mNumActiveSockets)
      mNumActiveSockets--;
}

void BUDP360Socket::tickActiveSockets(void)
{
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
      mpActiveSockets[i]->checkForEvents(true);
}
//==============================================================================
// eof: BUDP360Socket.cpp
//==============================================================================
