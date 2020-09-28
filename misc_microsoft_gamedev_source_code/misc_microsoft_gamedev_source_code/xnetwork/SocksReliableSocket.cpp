//==============================================================================
// SocksReliableSocket.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "SocksReliableSocket.h"
#include "winsockinc.h"

//==============================================================================
// Defines
using namespace Etl;

//==============================================================================
// BSocksReliableSocket::BSocksReliableSocket
//==============================================================================
BSocksReliableSocket::BSocksReliableSocket (
   IN BObserver * Observer) :
   BSocksSocketBase (Observer)
{
   ZeroMemory (&mLocalAddress, sizeof (SOCKADDR_IN));
   ZeroMemory (&mRemoteAddress, sizeof (SOCKADDR_IN));
}


//==============================================================================
// BSocksReliableSocket::~BSocksReliableSocket
//==============================================================================
BSocksReliableSocket::~BSocksReliableSocket(void)
{    
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::dispose(void)
{
   close();

   return BSocksSocketBase::dispose();
}

//==============================================================================
// BSocksReliableSocket::connect
//==============================================================================
HRESULT BSocksReliableSocket::connect (
   IN CONST SOCKADDR_IN * RemoteAddress,
   IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
{
   if (RemoteAddress == NULL)
   {
      nlog (cTransportNL, "BSocksReliableSocket: RemoteAddress is required");
      return E_INVALIDARG;
   }

   mRemoteAddress = *RemoteAddress;

   //
   // Create socket
   //

#ifdef XBOX
   mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
   mSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0); // WSA_FLAG_OVERLAPPED);
#endif   
   if (mSocket == INVALID_SOCKET)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableSocket: failed to create TCP/IP socket");
      nlogError (cTransportNL, Result);
      return Result;
   }

   //
   // If caller supplied a local address, then bind to it.
   //

   if (LocalAddress != NULL)
   {
#ifdef XBOX
      SOCKADDR_IN fixedLocalAddress(*LocalAddress);
      fixedLocalAddress.sin_addr.s_addr = INADDR_ANY;
      if (bind (mSocket, reinterpret_cast <CONST SOCKADDR *> (&fixedLocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
#else 
      if (bind (mSocket, reinterpret_cast <CONST SOCKADDR *> (LocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
#endif      
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksReliableSocket: failed to bind socket to local address");
         nlogError (cTransportNL, Result);
         return Result;
      }
   }

   //
   // Connect the socket
   //

   if (::connect (mSocket, reinterpret_cast <CONST SOCKADDR *> (RemoteAddress), sizeof SOCKADDR_IN) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableSocket: failed to connect");
      nlogError (cTransportNL, Result);
      return Result;
   }

   nlog (cTransportNL, "BSocksReliableSocket: successfully connected to peer");

   HRESULT hr = startSocket ();

   if (FAILED(hr))
      return hr;

   mConnected=true;
   if (getObserver())
      getObserver()->connected(this);

   return S_OK;
}

//==============================================================================
// 
void BSocksReliableSocket::notifySocketClose (
   IN HRESULT Result)
{
   if (getObserver())
   {
      mConnected=false;
      getObserver() -> disconnected (this, Result);
   }
}

void BSocksReliableSocket::networkClose (
   IN HRESULT Result)
{
   close();
   notifySocketClose (Result);
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::createFromAccept (
   IN SOCKET ClientSocket)
{
   if (mSocket != INVALID_SOCKET)
   {
      nlog (cTransportNL, "BSocksReliableSocket: already have/am a socket");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   mSocket = ClientSocket;

   HRESULT hr = startSocket();
   if (FAILED(hr))
      return hr;

   mConnected=true;
   if (getObserver())
      getObserver()->connected(this);

   return S_OK;
}

HRESULT BSocksReliableSocket::startSocket (void)
{   
   //
   // Disable Nagle algorithm
   //

   BOOL OptionValue = 1;
   if (setsockopt (mSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&OptionValue, sizeof OptionValue) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      Result;
      nlog (cTransportNL, "BSocksReliableSocket: failed to disable Nagle algorithm on socket, will continue anyway");
      nlogError (cTransportNL, Result);
   }

   //
   // Get the local and remote addresses, now that we've connected.
   //

   INT SocketAddressLength = sizeof (SOCKADDR_IN);
   if (getsockname (mSocket, reinterpret_cast <SOCKADDR *> (&mLocalAddress), &SocketAddressLength) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      Result;
      nlog (cTransportNL, "BSocksReliableSocket: failed to query local address after connect, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   SocketAddressLength = sizeof (SOCKADDR_IN);
   if (getpeername (mSocket, reinterpret_cast <SOCKADDR *> (&mRemoteAddress), &SocketAddressLength) == SOCKET_ERROR)
   {
      HRESULT Result = GetLastResult();
      Result;
      nlog (cTransportNL, "BSocksReliableSocket: failed to query remote address after connect, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   //
   // Begin receiving network messages (via USER32)
   //

   HRESULT Result = asyncSelect (FD_READ | FD_WRITE | FD_CLOSE);
   if (FAILED (Result))
   {
      return Result;
   }

   return S_OK;
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::getLocalAddress (
   OUT SOCKADDR_IN * ReturnLocalAddress)
{
   *ReturnLocalAddress = mLocalAddress;
   return S_OK;
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::getRemoteAddress (
   OUT SOCKADDR_IN * ReturnRemoteAddress)
{
   *ReturnRemoteAddress = mRemoteAddress;
   return S_OK;
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::close (void)
{
   if (mSocket == INVALID_SOCKET)
   {
      return S_OK;
   }

   if (mSend.ActiveBuffer != NULL)
   {
      sendFreeBuffer (mSend.ActiveBuffer);
      mSend.ActiveBuffer = NULL;
      mSend.ActiveIndex = 0;
   }

   BSocksReliableSendBuffer * SendBuffer = mSend.mQueueList.removeHead();
   while (SendBuffer)
   {  
      sendFreeBuffer (SendBuffer);
      SendBuffer = mSend.mQueueList.removeHead();
   }

   if (mRecv.Buffer != NULL)
   {
      delete [] mRecv.Buffer;
      mRecv.Buffer = NULL;
   }

   mRecv.MaximumLength = 0;
   mRecv.Length = 0;


   closesocket (mSocket);
   mSocket = INVALID_SOCKET;

   asyncStop();

   return S_OK;
}

//
// Receive State Machine
//
//

//==============================================================================
// 
HRESULT BSocksReliableSocket::recvGrow (
   IN DWORD Length)
{
   if (Length <= mRecv.MaximumLength)
   {
      return S_OK;
   }

   //
   // Need to allocate more buffer space.
   // Use a silly heuristic.
   //

   DWORD NewMaximumLength = (mRecv.Length + RecvMinimumLength) * 3 / 2 + RecvMinimumLength;
   LPBYTE NewBuffer = new BYTE[NewMaximumLength];

   if (NewBuffer == NULL)
   {
      return E_OUTOFMEMORY;
   }

   BASSERT (NewMaximumLength >= mRecv.MaximumLength);

   if (mRecv.Length > 0)
   {
      CopyMemory (NewBuffer, mRecv.Buffer, mRecv.Length);
   }

   if (mRecv.Buffer != NULL)
   {
      delete [] mRecv.Buffer;
   }

   mRecv.Buffer = NewBuffer;
   mRecv.MaximumLength = NewMaximumLength;

   return S_OK;
}

//==============================================================================
// 
// Network stack is indicating that data is ready to be received.
//
BOOL BSocksReliableSocket::recvReady (void)
{
   HRESULT Result;

   if (mSocket == INVALID_SOCKET)
   {
      return FALSE;
   }

   if (mRecv.IsBusy)
   {
      mRecv.ReentrantCallOccurred = TRUE;
      nlog (cTransportNL, "[%p]BSocksReliableSocket: receive state machine is busy (reentrancy check)!");
      return FALSE;
   }

   //
   // Get some buffer space.
   //

   Result = recvGrow (mRecv.Length + RecvMinimumLength);
   if (FAILED (Result))
   {
      networkClose (Result);
      return FALSE;
   }

   //
   // Receive data into user-mode buffer.
   //

   INT BytesTransferred = recv (mSocket,
      reinterpret_cast <PCHAR> (mRecv.Buffer + mRecv.Length),
      mRecv.MaximumLength - mRecv.Length, 0);

   if (BytesTransferred == SOCKET_ERROR)
   {
      if (GetLastError() == WSAEWOULDBLOCK)
      {
         //
         // Normal / harmless.
         //

         return FALSE;
      }

      Result = GetLastResult();
      nlog (cTransportNL, "[%p]BSocksReliableSocket: failed to receive data from socket", this);
      nlogError (cTransportNL, Result);
      networkClose (Result);
      return FALSE;
   }

   if (BytesTransferred == 0)
   {
      //
      // Peer has closed connection gracefully.
      //

      nlog (cTransportNL, "[%p]BSocksReliableSocket: peer has closed connection gracefully", this);
      networkClose (HRESULT_FROM_WIN32 (ERROR_GRACEFUL_DISCONNECT));
      return FALSE;
   }

   //
   // Parse and indicate messages.
   //

   mRecv.Length += BytesTransferred;

   Result = recvParse();
   if (FAILED (Result))
   {
      nlog (cTransportNL, "[%p]BSocksReliableSocket: failed to parse network data, closing connection", this);
      networkClose (Result);
      return FALSE;
   }

   //
   // Do NOT touch the instance at this point or beyond.
   // At this point, the instance may be deleted.
   //
   return FALSE;
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::recvParse (void)
{
   BASSERT (!mRecv.IsBusy);

   DWORD ParseIndex;

   ParseIndex = 0;

   while (ParseIndex < mRecv.Length)
   {
      BASSERT (ParseIndex <= mRecv.Length);

      //
      // See if we have enough data for a complete message.
      //

      DWORD BytesRemaining = mRecv.Length - ParseIndex;

      if (BytesRemaining < sizeof (BSocksReliableMessageHeader))
      {
         //
         // Not enough for the header yet.
         //

         nlog (cTransportNL, "[%p]BSocksReliableSocket: not enough data for header yet...", this);
         break;
      }

      BSocksReliableMessageHeader * Header = reinterpret_cast <BSocksReliableMessageHeader *> (mRecv.Buffer + ParseIndex);

      //
      // The message length is the length of the payload, including the header.
      //

      DWORD MessageLength = Header -> MessageLength - sizeof(BSocksReliableMessageHeader);

      if (MessageLength > MaximumReliableMessageLength)
      {
         nlog (cTransportNL, "[%p]BSocksReliableSocket: peer wants to send us an enormous message (length 0x%08x max 0x%08x)",
            this, MessageLength, MaximumReliableMessageLength);

         return E_FAIL;
      }

      if (BytesRemaining < sizeof (BSocksReliableMessageHeader) + MessageLength)
      {
         nlog (cTransportNL, "[%p]BSocksReliableSocket: not enough data for full message yet, have 0x%08x need 0x%08x",
            this, BytesRemaining, sizeof (BSocksReliableMessageHeader) + MessageLength);
         break;
      }
     
      //
      // At least one full message has been received; rejoice.
      //

      PVOID MessageBuffer = mRecv.Buffer + ParseIndex + sizeof (BSocksReliableMessageHeader);

   // nlog (cTransportNL, "BSocksReliableSocket: received complete message, length 0x%08x %u", MessageLength, MessageLength);

      if (getObserver())
      {
         nlog (cTransportNL, "[%p]BSocksReliableSocket: received packet, calling observer recvd() method -->",
            this);

         BASSERT (!mRecv.IsBusy);
         BASSERT (mRecv.TerrifyingLocalVariable == NULL);

         BOOL IGotDeleted = FALSE;
         mRecv.IsBusy = TRUE;
         mRecv.TerrifyingLocalVariable = &IGotDeleted;

         getObserver()->recvd (this, MessageBuffer, MessageLength, 0, &mRemoteAddress);

         if (IGotDeleted)
         {
            return S_OK;
         }

         BASSERT (mRecv.IsBusy);
         mRecv.IsBusy = FALSE;
         mRecv.TerrifyingLocalVariable = NULL;

         nlog (cTransportNL, "[%p]BSocksReliableSocket: received packet , observer recvd() method has completed <--",
            this);
      }
      else
      {
         nlog (cTransportNL, "[%p]BSocksReliableSocket: received packet , but no observer interface is set -- packet will be ignored",
            this);
      }

      ParseIndex += MessageLength + sizeof (BSocksReliableMessageHeader);

      //
      // Continue for more fabulous prizes!
      //
   }

   //
   // If any unparsed data remains, move it to the head of the buffer.
   // The move will always be a small one, and this lets us make a happy assumption,
   // that the beginning of the receive buffer is aligned with a message header.
   //

   if (ParseIndex > 0)
   {
      if (ParseIndex < mRecv.Length)
      {
         mRecv.Length -= ParseIndex;
         MoveMemory (mRecv.Buffer, mRecv.Buffer + ParseIndex, mRecv.Length);
      }
      else
      {
         mRecv.Length = 0;
      }
   }

   BASSERT (!mRecv.IsBusy);

   if (mRecv.ReentrantCallOccurred)
   {
      nlog (cTransportNL, "[%p]BSocksReliableSocket: top-most instance of recvReady() has detected a reentrant call to recvReady() -- posting message to self");

      //
      // WSAAsyncSelect encodes the event in the low word, the status code in the high word.
      // We synthesize our own message, so that we don't potentially spin in this call frame forever.
      //

      mRecv.ReentrantCallOccurred = FALSE;
      // TBD: WTF is this and how do we make it work with socket events?
      // PostMessage (mWindow, SOCKS_WM_NETWORK_EVENT, 0, (ERROR_SUCCESS << 0x10) | FD_READ);
   }
   return S_OK;
}

//
// Send State Machine
//



//
// For now we allocate a new buffer on every request.
// The API is designed to allow us to cache requests in a buffer list, eventually.
//

//==============================================================================
// 
HRESULT BSocksReliableSocket::sendAllocateBuffer (
   IN DWORD MaximumLength,
   OUT BSendBuffer ** ReturnSendBuffer)
{
   BASSERT (ReturnSendBuffer != NULL);
   *ReturnSendBuffer = NULL;

   DWORD AllocationLength;
   BSocksReliableSendBuffer * SendBuffer;

   AllocationLength = MaximumLength + sizeof (BSocksReliableSendBuffer) + sizeof (BSocksReliableMessageHeader);

   LPBYTE MemoryBlock = new BYTE [AllocationLength];
   if (MemoryBlock == NULL)
   {
      return E_OUTOFMEMORY;
   }

// memset (MemoryBlock, 0xee, AllocationLength);

   SendBuffer = reinterpret_cast <BSocksReliableSendBuffer *> (MemoryBlock);
   SendBuffer -> Header = reinterpret_cast <BSocksReliableMessageHeader *> (MemoryBlock + sizeof (BSocksReliableSendBuffer));
   SendBuffer -> Buffer = MemoryBlock + sizeof (BSocksReliableSendBuffer) + sizeof (BSocksReliableMessageHeader);
   SendBuffer -> MaximumLength = MaximumLength;
   SendBuffer -> Length = 0;
   SendBuffer -> VoiceLength = 0;
   SendBuffer -> Socket = this;
   SendBuffer -> Flags = 0;

   *ReturnSendBuffer = SendBuffer;

   return S_OK;
}

//==============================================================================
// 
HRESULT BSocksReliableSocket::sendFreeBuffer (
   IN BSendBuffer * SendBuffer)
{
   if (SendBuffer != NULL)
   {
      delete [] reinterpret_cast <LPBYTE> (SendBuffer);
   }

   return S_OK;
}


//==============================================================================
// BSocksReliableSocket::send
//==============================================================================
HRESULT BSocksReliableSocket::send (
   IN BSendBuffer * SendBufferBase)
{
   if (mSocket == INVALID_SOCKET)
   {
      nlog (cTransportNL, "BSocksUnreliableSocket: socket is closed, can't send");
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   BSocksReliableSendBuffer * SendBuffer = static_cast <BSocksReliableSendBuffer *> (SendBufferBase);

   BASSERT (SendBuffer != NULL);
   BASSERT (SendBuffer -> Length <= SendBuffer -> MaximumLength);
   BASSERT (SendBuffer -> Socket == this);

   //
   // Fill in the message header.
   //

   SendBuffer -> Header -> MessageLength = SendBuffer -> Length;

   //
   // Insert the buffer into the send list, and begin sending this (or any next) buffer.
   //

   mSend.mQueueList.addToTail(SendBuffer);
   
   nlog (cTransportNL, "BSocksReliableSocket: sending packet");   

   return sendNextBuffer ();
}

HRESULT BSocksReliableSocket::sendNextBuffer (void)
{
   BSocksReliableSendBuffer * CurrentBuffer = mSend.ActiveBuffer;
   if (CurrentBuffer == NULL)
      mSend.ActiveBuffer = mSend.mQueueList.removeHead(); // get next buffer

   while (mSend.ActiveBuffer)
   {      
      BASSERT (mSend.ActiveIndex <= mSend.ActiveBuffer -> Length);

      //
      // Attempt to send the buffer.
      // WinSock may transmit none, some, or all of the buffer, as kernel-mode buffer space allows.
      //

      PCHAR SendBuffer = reinterpret_cast <PCHAR> (mSend.ActiveBuffer -> Header) + mSend.ActiveIndex;
      INT SendLength = mSend.ActiveBuffer -> Length + sizeof (BSocksReliableMessageHeader) - mSend.ActiveIndex;

      /*BString str;
      str.format(B("Sending %ld bytes"), SendLength);
      debugf(str.asANSI());*/
      INT BytesTransferred = ::send (mSocket, SendBuffer, SendLength, 0);

      if (BytesTransferred == SOCKET_ERROR)
      {
         HRESULT Result = GetLastResult();

         if (Result == HRESULT_FROM_WIN32 (WSAEWOULDBLOCK))
         {
            //
            // Cannot accept buffer right now (TCP window full).
            // Leave this buffer where it is at; we'll be notified via recvReady
            // when we should try to send it again.
            //

            nlog (cTransportNL, "BSocksReliableSocket: TCP window is full, leaving buffer on list (no data accepted)");
            break;
         }

         //
         // All other errors are considered fatal transport errors.
         //

         nlog (cTransportNL, "BSocksReliableSocket: failed to send buffer, assuming socket is dead");
         nlogError (cTransportNL, Result);
         networkClose (Result);
         return Result;
      }

      BASSERT (BytesTransferred >= 0 && BytesTransferred <= SendLength);

      if (BytesTransferred < SendLength)
      {
         //
         // WinSock accepted some, but not all, of the buffer submitted.
         // Leave the buffer active.
         //

         nlog (cTransportNL, "BSocksReliableSocket: TCP window is full, leaving buffer on list (partial send, %u of %u bytes accepted)", BytesTransferred, SendLength);
         break;
      }

      //
      // Entire buffer was consumed.  Rejoice, then free buffer, and continue for another round.
      //

      nlog (cTransportNL, "BSocksReliableSocket: entire buffer (%u bytes) accepted by winsock", BytesTransferred);
      sendFreeBuffer (mSend.ActiveBuffer);
      mSend.ActiveIndex = 0;
      mSend.ActiveBuffer = mSend.mQueueList.removeHead();      
   }

   return S_OK;
}


void BSocksReliableSocket::sendReady (void)
{
   HRESULT Result;

   Result = sendNextBuffer ();
   if (FAILED (Result))
   {
      nlog (cTransportNL, "BSocksReliableSocket: sendNextBuffer failed, closing socket");
      networkClose (Result);
   }
}

   
//==============================================================================
// BSocksReliableSocket::
//==============================================================================



//==============================================================================
// eof: SocksReliableSocket.cpp
//==============================================================================
