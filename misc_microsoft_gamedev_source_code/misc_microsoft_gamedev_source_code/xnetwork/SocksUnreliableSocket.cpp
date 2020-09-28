//==============================================================================
// SocksUnreliableSocket.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "SocksUnreliableSocket.h"
#include "SocksHelper.h"

//xsystem
#include "timelineprofiler.h"

//==============================================================================
// Defines

BSocksUnreliableSocket::BSocksUnreliableSocket (
   IN BObserver * Observer) :
   BSocksSocketBase (Observer)
{
   ZeroMemory (&mLocalAddress, sizeof SOCKADDR_IN);
   ZeroMemory (&mRemoteAddress, sizeof SOCKADDR_IN);
}


//==============================================================================
// BSocksUnreliableSocket::~BSocksUnreliableSocket
//==============================================================================
BSocksUnreliableSocket::~BSocksUnreliableSocket(void)
{
}

//==============================================================================
// 
HRESULT BSocksUnreliableSocket::dispose(void)
{
   close();

   return BSocksSocketBase::dispose();
}

HRESULT BSocksUnreliableSocket::createAndBind (
   IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
{   
   if (mSocket != INVALID_SOCKET)
   {
      //
      // Already got one.
      //
                                                      
      nlog (cTransportNL, "BSocksUnreliableSocket: cannot create a new socket, already got one");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   {
      HRESULT Result = mRecv.grow (DefaultRecvAllocationLength);
      if (FAILED (Result))
      {
         return Result;
      }
   }

#ifdef XBOX
   mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
   mSocket = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0); // WSA_FLAG_OVERLAPPED);
#endif   
   if (mSocket == INVALID_SOCKET)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: failed to create socket");
      return Result;
   }

   BOOL OptionValue = TRUE;
   if (setsockopt (mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast <char *> (&OptionValue), sizeof OptionValue) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: failed to set SO_BROADCAST on socket, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   OptionValue = TRUE;
   if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <char *> (&OptionValue), sizeof OptionValue) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: failed to set SO_REUSEADDR");
      return Result;
   }

   //
   // If the caller supplied a local address, bind to it.
   //

   if (LocalAddress != NULL)
   {
      mLocalAddress = *LocalAddress;
   }
   else
   {
      mLocalAddress.sin_family = AF_INET;
      mLocalAddress.sin_port = ntohs(0);
      mLocalAddress.sin_addr.s_addr = INADDR_ANY;
   }

#ifdef XBOX
   SOCKADDR_IN fixedLocalAddress(mLocalAddress);
   fixedLocalAddress.sin_addr.s_addr = INADDR_ANY;
   if (bind (mSocket, reinterpret_cast <SOCKADDR *> (&fixedLocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
#else   
   if (bind (mSocket, reinterpret_cast <SOCKADDR *> (&mLocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
#endif   
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: failed to bind UDP/IP socket to port %d 0x%08x", htons (mLocalAddress.sin_port));
      nlogError (cTransportNL, Result);
      closesocket (mSocket);
      mSocket = INVALID_SOCKET;
      return Result;
   }

   INT SocketAddressLength = sizeof (SOCKADDR_IN);
   if (getsockname (mSocket, reinterpret_cast <SOCKADDR *> (&mLocalAddress), &SocketAddressLength) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: failed to query local UDP/IP socket address, continuing anyway");
      nlogError (cTransportNL, Result);
   }

#ifdef XBOX
   mConnected=true;

   return asyncSelect (FD_READ | FD_WRITE);
#else
   DWORD param = 1;
   if (ioctlsocket(mSocket, FIONBIO, &param) != SOCKET_ERROR)
   {
      long inBuff = 0, outBuff = 0;
      DWORD outSize = 0;   
      WSAIoctl(mSocket, SIO_ADDRESS_LIST_CHANGE, &inBuff, 0, &outBuff, 0, &outSize, NULL, NULL);
   }

   mConnected=true;

   return asyncSelect (FD_READ | FD_WRITE | FD_ADDRESS_LIST_CHANGE);
#endif
}

//==============================================================================
// 
HRESULT BSocksUnreliableSocket::close(void)
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
      getObserver()->disconnected(this, 0);

   return S_OK;
}

//==============================================================================
// BSocksUnreliableSocket::
//==============================================================================




HRESULT BSocksUnreliableSocket::connect (
   IN CONST SOCKADDR_IN * RemoteAddress,
   IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
{
   HRESULT Result;


   Result = createAndBind (LocalAddress);
   if (FAILED (Result))
   {
      return Result;
   }



   //
   // If the caller supplied a remote address, then connect to it.
   // We only support IP addresses; no DNS lookups.
   //

   if (RemoteAddress != NULL)
   {
      mRemoteAddress = *RemoteAddress;
   }

   nlog (cTransportNL, "BSocksUnreliableSocket: socket created");

   mConnected=true;
   if (getObserver())
      getObserver()->connected(this);

   return S_OK;
}

HRESULT BSocksUnreliableSocket::getLocalAddress (
   OUT SOCKADDR_IN * ReturnLocalAddress)
{
   *ReturnLocalAddress = mLocalAddress;
   return S_OK;
}

HRESULT BSocksUnreliableSocket::getRemoteAddress (
   OUT SOCKADDR_IN * ReturnRemoteAddress)
{
   *ReturnRemoteAddress = mRemoteAddress;
   return S_OK;
}


//
// For now we allocate a new buffer on every request.
// The API is designed to allow us to cache requests in a buffer list, eventually.
//

HRESULT BSocksUnreliableSocket::allocateBuffer (
   IN DWORD MaximumLength,
   OUT BSocksUnreliableBuffer ** ReturnSendBuffer)
{

   DWORD AllocationLength;
   BSocksUnreliableBuffer * SendBuffer;

   AllocationLength = MaximumLength + sizeof (BSocksUnreliableBuffer);

   LPBYTE MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
   if (MemoryBlock == NULL)
   {
      BFAIL("gNetworkHeap: Out of memory");
      return E_OUTOFMEMORY;
   }

   SendBuffer = reinterpret_cast <BSocksUnreliableBuffer *> (MemoryBlock);
   SendBuffer -> Buffer = MemoryBlock + sizeof (BSocksUnreliableBuffer);
   SendBuffer -> MaximumLength = MaximumLength;
   SendBuffer -> Length = 0;
   SendBuffer -> Socket = this;

   *ReturnSendBuffer = SendBuffer;

   return S_OK;
}

HRESULT BSocksUnreliableSocket::freeBuffer (
   IN BSocksUnreliableBuffer * SendBuffer)
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

HRESULT BSocksUnreliableSocket::sendAllocateBuffer (
   IN DWORD MaximumLength,
   OUT BSendBuffer ** ReturnSendBuffer)
{
   BASSERT (ReturnSendBuffer != NULL);

   BSocksUnreliableBuffer * Buffer;
   HRESULT Result;

   Result = allocateBuffer (MaximumLength, &Buffer);
   *ReturnSendBuffer = Buffer;

   Buffer->Flags = 0;
   Buffer->Length = 0;
   Buffer->VoiceLength = 0;

   return Result;
}

HRESULT BSocksUnreliableSocket::sendFreeBuffer (
   IN BSendBuffer * SendBuffer)
{
   return freeBuffer (static_cast <BSocksUnreliableBuffer *> (SendBuffer));
}

HRESULT BSocksUnreliableSocket::send (
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
      nlog (cTransportNL, "BSocksUnreliableSocket::send: remote address not yet set");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }
}

HRESULT BSocksUnreliableSocket::sendTo (
   IN BSendBuffer * SendBufferBase,
   IN CONST SOCKADDR_IN * DestinationAddress)
{
   if (SendBufferBase == NULL || DestinationAddress == NULL)
   {
      return E_INVALIDARG;
   }

   BSocksUnreliableBuffer * SendBuffer = static_cast <BSocksUnreliableBuffer *> (SendBufferBase);

   if (mSocket == INVALID_SOCKET)
   {
      nlog (cTransportNL, "BSocksUnreliableSocket: socket is closed, can't send");
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
   BASSERT (SendBuffer -> Length <= SendBuffer -> MaximumLength);
   BASSERT (SendBuffer -> Socket == this);

   SendBuffer -> RemoteAddress = *DestinationAddress;

   mSend.mQueueList.addToTail(SendBuffer);

   HRESULT Result = sendNextBuffer ();

   return Result;
}

HRESULT BSocksUnreliableSocket::sendNextBuffer (void)
{
   int sendLen;

   BSocksUnreliableBuffer * SendBuffer = mSend.mQueueList.removeHead();
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
            reinterpret_cast <SOCKADDR *> (&SendBuffer -> RemoteAddress),
            sizeof (SOCKADDR_IN));
      }

      nlog (cTransportNL, "BSocksUnreliableSocket: sent packet, length %ld",         
         SendBuffer -> Length);

      if (BytesTransferred == SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
      {
         mSend.mQueueList.addToHead(SendBuffer);

         nlog (cTransportNL, "BSocksUnreliableSocket: can't send buffer now, will try later when buffer space is available");
         return S_OK;
      }

      sendFreeBuffer (SendBuffer);

      if (BytesTransferred == SOCKET_ERROR)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksUnreliableSocket: failed to send buffer on udp socket. Last Error: %d", GetLastError());
         nlogError (cTransportNL, Result);
         return Result;
      }

      SendBuffer = mSend.mQueueList.removeHead();
   }

   return S_OK;
}

void BSocksUnreliableSocket::sendReady (void)
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

BOOL BSocksUnreliableSocket::recvReady (void)
{
   if (mRecv.Buffer == NULL)
   {
      nlog (cTransportNL, "BSocksUnreliableSocket: receive buffer is NULL, cannot receive data");
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
         nlog (cTransportNL, "BSocksUnreliableSocket: winsock can't make up its mind whether we have data or not :\\");
         return FALSE;
      }

      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksUnreliableSocket: receive failed (could be ICMP port unreachable message)");
      nlogError (cTransportNL, Result);      
      return FALSE;
   }

   if ( BytesTransferred == 0 )
      return FALSE;

   //
   // Callback can result in destruction; do not touch object after this call.
   //

   nlog (cTransportNL, "--- BSocksUnreliableSocket: recvd - Socket [%p], Length %ld, RemoteAddress %s:%ld", this, BytesTransferred, inet_ntoa(RemoteAddress.sin_addr), ntohs(RemoteAddress.sin_port));

   if (getObserver())
   {
      if (BytesTransferred < 2)
         return TRUE;

      // extract the cbGameData value
      WORD cbGameData = 0;
      memcpy(&cbGameData, mRecv.Buffer, sizeof(WORD));

      if (BytesTransferred < cbGameData)
         return TRUE;

      getObserver() -> recvd (this, mRecv.Buffer+2, BytesTransferred-2, BytesTransferred-cbGameData-2, &RemoteAddress);
   }

   return TRUE;
}

//==============================================================================
// eof: SocksUnreliableSocket.cpp
//==============================================================================
