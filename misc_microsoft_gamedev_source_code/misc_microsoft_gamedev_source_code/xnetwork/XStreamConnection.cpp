//==============================================================================
// XStreamConnection.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// Includes
#include "Precompiled.h"
#include "XStreamConnection.h"

// xcore
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"

IMPLEMENT_FREELIST(BXTcpSendBuffer, 2, &gNetworkHeap);

//==============================================================================
// 
//==============================================================================
BXTcpSendBuffer::~BXTcpSendBuffer()
{
   if (mpBuffer)
      gNetworkHeap.Delete(mpBuffer);
   mpBuffer = NULL;
}

//==============================================================================
// 
//==============================================================================
void BXTcpSendBuffer::onAcquire()
{
   mpBuffer = NULL;
   mSize = 0;
   mIndex = 0;
}

//==============================================================================
// 
//==============================================================================
void BXTcpSendBuffer::onRelease()
{
   if (mpBuffer)
      gNetworkHeap.Delete(mpBuffer);

   mpBuffer = NULL;
   mSize = 0;
   mIndex = 0;
}

//==============================================================================
// 
//==============================================================================
BXStreamConnection::BXStreamConnection(uint flags, uint timeout) : //, uint reconnectAttempts) :
   mSendEvent(true),
   mRecvEvent(true),
   mSocket(INVALID_SOCKET),
   mpDeflateStream(NULL),
   mpInflateStream(NULL),
   mpDynamicStream(NULL),
   mpByteStream(NULL),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mTimeout(0),
   mTimeoutValue(timeout),
   //mConnectionRetries(0),
   //mReconnectAttempts(reconnectAttempts),
   mState(cStateIdle),
   mError(cErrorSuccess),
   mSentPending(false),
   mUseHeader(false)
{
   mSend.mQueue.reset();

   if (flags & eFlagUseHeader)
   {
      mUseHeader = true;
      // if we're using a header, then we have the potential to compress data
      mpDeflateStream = new(gNetworkHeap) BDeflateStream();
      mpInflateStream = new(gNetworkHeap) BInflateStream();
      mpDynamicStream = new(gNetworkHeap) BDynamicStream();
      mpByteStream = new(gNetworkHeap) BByteStream((const void*)NULL, 0);
   }

   mSend.mpBuffer = reinterpret_cast<char*>(gNetworkHeap.New(cSendBufferSize));
   mSend.mpSendHeader = reinterpret_cast<BStreamSendHeader*>(mSend.mpBuffer);

   // if we're not using a send header, then move the payload
   // pointer to the beginning of the buffer
   if (!mUseHeader)
      mSend.mpSendPayload = mSend.mpBuffer;
   else
      mSend.mpSendPayload = mSend.mpBuffer + sizeof(BStreamSendHeader);

   mRecv.mpBuffer = reinterpret_cast<char*>(gNetworkHeap.New(cRecvBufferSize));
}

//==============================================================================
// 
//==============================================================================
BXStreamConnection::~BXStreamConnection()
{
   close();
}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::connect(const IN_ADDR& lspAddr, ushort port)
{
   mEventHandle = WSACreateEvent();
   if (mEventHandle == WSA_INVALID_EVENT)
   {
      HRESULT hr = GetLastResult();
      nlog(cTransportNL, "BXStreamConnection: failed to create WSA Event");
      nlogError(cTransportNL, hr);
      return false;
   }

   //
   // Create socket
   //
   mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   if (mSocket == INVALID_SOCKET)
   {
      HRESULT hr = GetLastResult();
      nlog(cTransportNL, "BXStreamConnection: failed to create TCP/IP socket");
      nlogError(cTransportNL, hr);
      return false;
   }

   BOOL optionValue = TRUE;

   if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optionValue), sizeof(optionValue)) == SOCKET_ERROR)
   {
      HRESULT hr = GetLastResult();
      nlog(cTransportNL, "BXStreamConnection: failed to enable SO_REUSEADDR on socket, continuing anyway");
      nlogError(cTransportNL, hr);
   }

   if (setsockopt(mSocket, SOL_SOCKET, SO_DONTLINGER, reinterpret_cast<const char*>(&optionValue), sizeof(optionValue)) == SOCKET_ERROR)
   {
      HRESULT hr = GetLastResult();
      nlog(cTransportNL, "BXStreamConnection: failed to enable SO_DONTLINGER on socket, continuing anyway");
      nlogError(cTransportNL, hr);
   }

   DWORD ioctl_opt = 1;
   if (0 != ioctlsocket(mSocket, FIONBIO, &ioctl_opt))
   {
      HRESULT hr = GetLastResult();
      nlog(cTransportNL, "BXStreamConnection: ioctlsocket() setting FIONBIO failed");
      nlogError(cTransportNL, hr);
      return false;
   }

   mAddr = lspAddr;
   mPort = port;

   SOCKADDR_IN sa;

   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = INADDR_ANY;
   sa.sin_port = htons(port);

   if (bind(mSocket, reinterpret_cast<const SOCKADDR*>(&sa), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
   {
      HRESULT hr = GetLastResult();
      nlog (cTransportNL, "BXStreamConnection: failed to create TCP/IP socket");
      nlogError (cTransportNL, hr);
      return false;
   }

   int rc = 0;

   sa.sin_family = AF_INET;
   sa.sin_addr = lspAddr;
   sa.sin_port = htons(port);

   //
   // Connect the socket
   //
   rc = ::connect(mSocket, reinterpret_cast<const SOCKADDR*>(&sa), sizeof(SOCKADDR_IN));
   if (rc == SOCKET_ERROR)
   {
      DWORD err = GetLastError();
      if (err != WSAEWOULDBLOCK)
      {
         nlog(cTransportNL, "BXStreamConnection: failed to connect");
         nlogError(cTransportNL, err);
         return false;
      }
   }

   rc = WSAEventSelect(mSocket, mEventHandle, FD_CONNECT);
   if (rc == SOCKET_ERROR)
   {
      DWORD err = GetLastError();
      nlog(cTransportNL, "BXStreamConnection: failed to connect");
      nlogError(cTransportNL, err);
      return false;
   }

   nlog(cTransportNL, "BXStreamConnection: successfully connected");

   mState = cStateConnecting;

   if (mTimeoutValue > 0)
      mTimeout = mTimeoutValue + timeGetTime();

   return true;
}

//==============================================================================
// 
//==============================================================================
//bool BXStreamConnection::reconnect()
//{
//   // if we're already attempting to connect/reconnect, just return true and let the observers deal with it
//   if (mState == cStateConnecting || mState == cStateConnected || mState == cStateReconnect)
//      return true;
//
//   // only want to allow X number of reconnects per connection
//   // if all reconnect attempts fail then we need to bubble an error up to the UI from the consumer
//   // of the connection so we can inform the user that the service is currently unavailable and please try again later
//   if (mObserverList.getSize() > 0 && mReconnectAttempts > 0 && mConnectionRetries < mReconnectAttempts)
//   {
//      mTimeout = mTimeoutValue + timeGetTime();
//      mConnectionRetries++;
//      mState = cStateReconnect;
//      cleanup();
//      return true;
//   }
//
//   return false;
//}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::send(const void* pData, const int32 size, bool compress)
{
   PVOID    pBuffer;
   int32    bufSize;

   if (compress)
   {
      if (!mUseHeader)
      {
         BASSERTM(0, "Compressing streams requires the use of the header");
         return false;
      }
      mpDynamicStream->seek(0);
      mpDeflateStream->open(*mpDynamicStream);
      mpDeflateStream->writeBytes(pData, size);
      mpDeflateStream->close();

      int32 newSize = static_cast<int32>(mpDeflateStream->compressedSize()) + sizeof(BDeflateStreamHeader) + sizeof(cDeflateStreamEndOfStreamMagic);

      pBuffer = gNetworkHeap.New(newSize);
      if (!pBuffer)
      {
         return false;
      }

      Utils::FastMemCpy(pBuffer, mpDynamicStream->getBuf().getData(), newSize);

      bufSize = newSize;
   }
   else
   {
      pBuffer = gNetworkHeap.New(size);
      if (!pBuffer)
         return false;

      Utils::FastMemCpy(pBuffer, pData, size);

      bufSize = size;
   }

   // create a buffer for this data and place in the queue
   BXTcpSendBuffer* pSendBuffer = BXTcpSendBuffer::getInstance();
   pSendBuffer->mpBuffer = pBuffer;
   pSendBuffer->mSize = bufSize;
   pSendBuffer->mCompress = compress;

   mSend.mQueue.addToTail(pSendBuffer);

   sendNextBuffer();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::service()
{
   if (mTimeout != 0 && mTimeout < timeGetTime())
   {
      shutdown(cStateError, cErrorTimedout, 0);
      mTimeout = 0;
      return;
   }

   // check for close/connect
   if (mState == cStateConnecting)
   {
      const DWORD waitResult = WaitForSingleObject(mEventHandle, 0);
      if (waitResult == WAIT_OBJECT_0)
      {
         // check if we're closed or connected
         fd_set read_fd;
         fd_set write_fd;
         fd_set except_fd;
         const timeval timeout = { 0, 0 };
         FD_ZERO(&read_fd);
         FD_ZERO(&write_fd);
         FD_ZERO(&except_fd);
         FD_SET(mSocket, &read_fd);
         FD_SET(mSocket, &write_fd);
         FD_SET(mSocket, &except_fd);
         int result = ::select(0, &read_fd, &write_fd, &except_fd, &timeout);
         if (result == SOCKET_ERROR)
         {
            int error = WSAGetLastError();
            if (error != WSAEINPROGRESS)
               shutdown(cStateError, cErrorAPIFailure, error);
         }
         else if (result > 0)
         {
            if (FD_ISSET(mSocket, &except_fd))
            {
               // connection failed
               shutdown(cStateError, cErrorConnectionFailure, 0);
            }
            else if (FD_ISSET(mSocket, &write_fd))
            {
               // connection succeeded, let's go
               connected();
            }
            else if (FD_ISSET(mSocket, &read_fd))
            {
               // connection was closed while we were attempting to connect
               shutdown(cStateError, cErrorConnectionClosed, 0);
            }
         }
      }
   }
   else if (mState == cStateConnected && mSocket != INVALID_SOCKET)
   {
      serviceSend();
      serviceRecv();

      if (mSend.mQueue.getSize() == 0)
         sendReady();
   }
   //else if (mState == cStateReconnect)
   //{
   //   if (!connect(mAddr, mPort))
   //      setState(cStateError, cErrorConnectionFailure);
   //}
   else if (mState == cStateError && mObserverList.getSize() > 0)
   {
      shutdown(mState, mError, 0);
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::connected()
{
   setState(cStateConnected);

   if (!initiateOverlappedRecv())
   {
      setState(cStateDisconnected);
      return;
   }

   uint count = mObserverList.getSize();
   for (int i=count-1; i >= 0; --i)
   {
      BXStreamObserver* pObserver = mObserverList[i];
      if (pObserver)
         pObserver->connected(*this);
   }

   mTimeout = mTimeoutValue + timeGetTime();
}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::initiateOverlappedSend()
{
   if (mSocket == INVALID_SOCKET)
      return false;

   while (true)
   {
      if (mSend.mWriteIndex == mSend.mReadIndex)
      {
         // I can reset the buffer indices to 0
         mSend.mWriteIndex = 0;
         mSend.mReadIndex = 0;
         mSend.mSize = 0;
      }

      if (cSendBufferSize - mSend.mWriteIndex >= cMaxSendSize)
      {
         // I have enough space in my buffer for a full packet
         // check the current buffer and see if we have more data available
         // or grab the next item from the queue and fill the buffer as much as we can
         // if there's nothing in the queue, then see if we still have data to send

         if (mSend.mpCurrentBuffer && mSend.mpCurrentBuffer->mSize > mSend.mpCurrentBuffer->mIndex)
         {
            // I haven't finished sending all the data from the current buffer
            // copy what we can into the send buffer
            BXTcpSendBuffer* pBuffer = mSend.mpCurrentBuffer;

            int len = min(pBuffer->mSize - pBuffer->mIndex, cSendBufferSize - mSend.mWriteIndex);

            mSend.mpSendPayload = mSend.mpBuffer + mSend.mWriteIndex;

            Utils::FastMemCpy(mSend.mpSendPayload, pBuffer->mpBuffer, len);

            mSend.mSize += len;
            mSend.mWriteIndex += len;

            pBuffer->mIndex += len;
         }
         else
         {
            if (mSend.mpCurrentBuffer)
               BXTcpSendBuffer::releaseInstance(mSend.mpCurrentBuffer);

            BXTcpSendBuffer* pBuffer = mSend.mpCurrentBuffer = mSend.mQueue.removeHead();

            if (pBuffer)
            {
               int headerSize = (mUseHeader ? cStreamHeaderSize : 0);

               mSend.mpSendHeader = reinterpret_cast<BStreamSendHeader*>(mSend.mpBuffer + mSend.mWriteIndex);
               mSend.mpSendPayload = mSend.mpBuffer + mSend.mWriteIndex + headerSize;

               // copy what we can into the buffer
               int len = min(pBuffer->mSize - pBuffer->mIndex, cSendBufferSize - mSend.mWriteIndex - headerSize);

               if (mUseHeader)
               {
                  mSend.mpSendHeader->mVersion = cStreamHeaderVersion;
                  mSend.mpSendHeader->mSize = pBuffer->mSize;
                  mSend.mpSendHeader->mCompress = (pBuffer->mCompress ? 1 : 0);
               }

               Utils::FastMemCpy(mSend.mpSendPayload, pBuffer->mpBuffer, len);

               mSend.mSize += headerSize + len;
               mSend.mWriteIndex += headerSize + len;

               pBuffer->mIndex += len;
            }
         }
      }

      // if there's no data to send, then check if we have anything pending in the queue before returning
      if (mSend.mWriteIndex == mSend.mReadIndex)
      {
         if (mSend.mQueue.getSize() > 0)
            continue;
         break;
      }

      //if (mSend.mSize - mSend.mReadIndex == 0)
      //{
      //   BXTcpSendBuffer* pBuffer = mSend.mpCurrentBuffer = mSend.mQueue.removeHead();

      //   if (!pBuffer)
      //      return true;

      //   mSend.mpSendHeader->mVersion = cStreamHeaderVersion;
      //   mSend.mpSendHeader->mSize = pBuffer->mSize;
      //   // copy the memory to our send buffer for the WSASend
      //   Utils::FastMemCpy(mSend.mpSendPayload, pBuffer->mpBuffer, pBuffer->mSize);
      //   mSend.mSize = cStreamHeaderSize + pBuffer->mSize;
      //   mSend.mReadIndex = 0;
      //}

      Utils::ClearObj(mSendOverlapped);
      mSendOverlapped.hEvent = mSendEvent.getHandle();

      WSABUF buffer;
      buffer.len = min(cMaxSendSize, mSend.mWriteIndex - mSend.mReadIndex);
      buffer.buf = mSend.mpBuffer + mSend.mReadIndex;

      DWORD sendBytesSent = 0;

      int result = WSASend(mSocket, &buffer, 1, &sendBytesSent, 0, &mSendOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         if (GetLastError() == WSA_IO_PENDING)
         {
            mSentPending = true;
            break;
         }
         return false;
      }

      if (!sendBytesSent)
      {
         // Connection was gracefully closed.
         return false;
      }

      mSend.mReadIndex += sendBytesSent;
   }

   mTimeout = mTimeoutValue + timeGetTime();

   //if (mSend.mQueue.getSize() == 0)
   //   sendReady();

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::initiateOverlappedRecv()
{
   if (mSocket == INVALID_SOCKET)
      return false;

   while (true)
   {
      Utils::ClearObj(mRecvOverlapped);
      mRecvOverlapped.hEvent = mRecvEvent.getHandle();

      WSABUF buffer;
      buffer.len = cRecvBufferSize - mRecv.mWriteIndex;
      buffer.buf = mRecv.mpBuffer + mRecv.mWriteIndex;

      DWORD recvBytesRead = 0;
      DWORD recvFlags = 0;

      int result = WSARecv(mSocket, &buffer, 1, &recvBytesRead, &recvFlags, &mRecvOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         if (GetLastError() == WSA_IO_PENDING)
            break;
         return false;
      }

      if (!recvBytesRead)
      {
         // Connection was gracefully closed.
         return false;
      }

      mRecv.mWriteIndex += recvBytesRead;

      mTimeout = mTimeoutValue + timeGetTime();

      if (!emptyRecvBuffer())
         return false;
   }      

   return true;
}

//============================================================================
// 
//============================================================================
void BXStreamConnection::processPayload(const char* pPayload, int32 payloadSize)
{
   // if we're not using the header, then simply pass the payload and size to interested observers
   if (!mUseHeader)
   {
      uint count = mObserverList.getSize();
      for (int i=count-1; i >= 0; --i)
      {
         BXStreamObserver* pObserver = mObserverList[i];
         if (pObserver)
         {
            pObserver->dataReceived(static_cast<const void*>(pPayload), payloadSize);
         }
      }
      return;
   }

   // all payloads using the header will be of the format
   // * 4 byte message ID
   // * 1 byte service ID
   // * 1 byte type
   // * 1 byte version
   // * payload

   // we need at least 2 bytes for the payload header
   if (payloadSize < 7)
      return;

   // parse out the type and version here and then inform all the observers
   // this is the point that we'd abstract out to a multi-threaded architecture
   uint8 serviceId = pPayload[4];
   int8 type = pPayload[5];
   int8 version = pPayload[6];

   //PVOID pData = gNetworkHeap.New(payloadSize);
   //if (!pData)
   //   return;

   //Utils::FastMemCpy(pData, pPayload, payloadSize);

   uint count = mObserverList.getSize();
   for (int i=count-1; i >= 0; --i)
   {
      BXStreamObserver* pObserver = mObserverList[i];
      if (pObserver)
      {
         // if one of the observers is able to process the data, then
         // they should return true and we'll end this here
         if (pObserver->getServiceID() == 0 || pObserver->getServiceID() == serviceId)
            pObserver->dataReceived(serviceId, type, version, pPayload, payloadSize);
      }
   }

   //gNetworkHeap.Delete(pData);
}

//============================================================================
// 
//============================================================================
bool BXStreamConnection::emptyRecvBuffer()
{
   //mRecvSerialBuffer.resetDestination(mRecv.mpBuffer+mRecv.mReadIndex, mRecv.mWriteIndex - mRecv.mReadIndex);
   mRecvSerialBuffer.resetSource(mRecv.mpBuffer+mRecv.mReadIndex, mRecv.mWriteIndex - mRecv.mReadIndex);

   // the stream header is 4 bytes long
   // 1 byte version
   // 3 bytes payload size
   // if I don't have enough for the header then realign the buffer
   // and go back to waiting on recv
   if (mUseHeader)
   {
      while (mRecvSerialBuffer.getDataAvailable() > cStreamHeaderSize)
      {
         BStreamSendHeader header;

         mRecvSerialBuffer.get(reinterpret_cast<uint8*>(&header), cStreamHeaderSize);

         // if we don't recognize the stream version, then
         // we're unable to process any more data from this connection
         // and we should terminate our communication
         // XXX this should do more than simply return false
         if (header.mVersion != cStreamVersion)
            return false;

         if (mRecvSerialBuffer.getDataAvailable() >= static_cast<uint32>(header.mSize))
         {
            mRecv.mReadIndex += cStreamHeaderSize + header.mSize;

            // I have the entire data payload, copy off the data and present
            // to the observers
            const void* pPayload = NULL;
            int32 payloadSize = static_cast<int32>(header.mSize);

            mRecvSerialBuffer.getPointer(&pPayload, &payloadSize);

            if (header.mCompress)
            {
               // decompress the payload
               mpByteStream->set(const_cast<void*>(pPayload), payloadSize);
               mpInflateStream->open(*mpByteStream);
               // allocate a new buffer to decompress into and pass up the chain
               uint bufSize = static_cast<uint>(mpInflateStream->bytesLeft());
               if (bufSize < UINT_MAX)
               {
                  PVOID pBuffer = gNetworkHeap.New(bufSize);
                  uint bytesRead = mpInflateStream->readBytes(pBuffer, UINT_MAX);
                  mpInflateStream->close();

                  if (bytesRead > 0)
                     processPayload(static_cast<char*>(pBuffer), bufSize);
                  gNetworkHeap.Delete(pBuffer);
               }
            }
            else
            {
               processPayload(static_cast<const char*>(pPayload), payloadSize);
            }
         }
         else
         {
            // I don't have enough data available for the entire payload
            break;
         }
      }
   }
   else
   {
      if (mRecvSerialBuffer.getDataAvailable() > 0)
      {
         const char* pPayload = NULL;
         int32 payloadSize = 0;

         mRecvSerialBuffer.getPointer(&pPayload, &payloadSize);

         processPayload(pPayload, payloadSize);
      }
   }

   // advance our read index based on how much we consumed from the serial buffer wrapper
   //mRecv.mReadIndex += (mRecvSerialBuffer.getBufferSize() - mRecvSerialBuffer.getDataAvailable());

   // determine if we should reset our buffer pointers
   if (mRecv.mReadIndex == mRecv.mWriteIndex)
   {
      // our buffer is empty, simply reset
      mRecv.mReadIndex = 0;
      mRecv.mWriteIndex = 0;
   }
   else if ((cRecvBufferSize - mRecv.mWriteIndex) < cRecvBufferMinSize)
   {
      // have exceeded our required minimum buffer size, move the bytes to the beginning of the buffer
      // and reset our indexes
      Utils::FastMemCpy(mRecv.mpBuffer, mRecv.mpBuffer + mRecv.mReadIndex, sizeof(BYTE)*(mRecv.mWriteIndex - mRecv.mReadIndex));
      mRecv.mWriteIndex = mRecv.mWriteIndex - mRecv.mReadIndex;
      mRecv.mReadIndex = 0;
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::shutdown(eState state, eError error, int result)
{
   // do not override the last error condition
   if (mState != cStateError)
   {
      mState = state;
      mError = error;
   }

   //if (error == cErrorConnectionClosed || error == cErrorConnectionFailure)
   //{
   //   if (reconnect())
   //      return;
   //}

   cleanup();

   if (mSend.mpBuffer)
   {
      gNetworkHeap.Delete(mSend.mpBuffer);
      mSend.mpBuffer = NULL;
   }

   if (mRecv.mpBuffer)
   {
      gNetworkHeap.Delete(mRecv.mpBuffer);
      mRecv.mpBuffer = NULL;
   }

   uint count = mObserverList.getSize();
   for (int i=count-1; i >= 0; --i)
   {
      BXStreamObserver* pObserver = mObserverList[i];
      if (pObserver)
      {
         //if (error != cErrorSuccess)
         //   pObserver->disconnected(0);
         //else
         pObserver->shutdown();
      }
   }
   mObserverList.clear();

   if (mpDeflateStream)
      HEAP_DELETE(mpDeflateStream, gNetworkHeap);
   mpDeflateStream = NULL;
   if (mpInflateStream)
      HEAP_DELETE(mpInflateStream, gNetworkHeap);
   mpInflateStream = NULL;
   if (mpDynamicStream)
      HEAP_DELETE(mpDynamicStream, gNetworkHeap);
   mpDynamicStream = NULL;
   if (mpByteStream)
      HEAP_DELETE(mpByteStream, gNetworkHeap);
   mpByteStream = NULL;

   nlog(cTransportNL, "BXStreamConnection: failure [state:%d, error:%d]", state, error);
   nlogError(cTransportNL, result);
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::serviceSend()
{
   if (!mSentPending)
      return;

   BOOL result;
   DWORD cbTransfer = 0;
   DWORD dwFlags = 0;

   // only check the send if I've issued one
   result = WSAGetOverlappedResult(mSocket, &mSendOverlapped, &cbTransfer, FALSE, &dwFlags);
   if (result)
   {
      mSentPending = false;

      mTimeout = mTimeoutValue + timeGetTime();

      mSend.mReadIndex += cbTransfer;
      if (mSend.mWriteIndex - mSend.mReadIndex == 0 || cSendBufferSize - mSend.mWriteIndex >= cMaxSendSize)
      {
         if (!initiateOverlappedSend())
         {
            setState(cStateDisconnected);
         }
      }
   }
   else
   {
      int error = WSAGetLastError();
      if (error != WSA_IO_INCOMPLETE && error != WSA_IO_PENDING)
      {
         // socket gone, unrecoverable
         setState(cStateDisconnected);
         mSentPending = false;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::sendNextBuffer()
{
   // if I'm currently attempting to send data, then return
   // when the previous send has completed, it will automatically
   // pick up the next buffer in the list
   if (mSend.mSize > mSend.mReadIndex)
      return;

   if (!initiateOverlappedSend())
      setState(cStateDisconnected);
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::serviceRecv()
{
   // only service recv when I've initiated the recv

   BOOL result;
   DWORD cbTransfer = 0;
   DWORD dwFlags = 0;

   // I'm only in a receive mode once I've connected successfully
   result = WSAGetOverlappedResult(mSocket, &mRecvOverlapped, &cbTransfer, FALSE, &dwFlags);
   if (result && cbTransfer > 0)
   {
      mRecv.mWriteIndex += cbTransfer;

      mTimeout = mTimeoutValue + timeGetTime();

      if (!emptyRecvBuffer() || !initiateOverlappedRecv())
      {
         setState(cStateDisconnected);
         return;
      }
   }
   else
   {
      int error = WSAGetLastError();
      if (error != WSA_IO_INCOMPLETE && error != WSA_IO_PENDING)
      {
         // socket gone, unrecoverable
         setState(cStateDisconnected);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::close()
{
   shutdown(cStateShutdown, cErrorSuccess, 0);
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::resetTimeout()
{
   if (mTimeoutValue > 0)
      mTimeout = mTimeoutValue + timeGetTime();
}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::isSending() const
{
   if (mSend.mQueue.getSize() > 0)
      return true;

   // check if we have an overlapped send call in-progress
   if (mSentPending)
      return true;

   return (mSend.mReadIndex != mSend.mWriteIndex);
}

//==============================================================================
// 
//==============================================================================
bool BXStreamConnection::isSendReady() const
{
   return (mSend.mQueue.getSize() < cMaxPending);
}

//==============================================================================
// Only cleanup the socket portion so we can attemp a reconnect
//==============================================================================
void BXStreamConnection::cleanup()
{
   if (mSocket != INVALID_SOCKET)
   {
      ::shutdown(mSocket, SD_BOTH);
      ::closesocket(mSocket);
      mSocket = INVALID_SOCKET;
   }

   // perform cleanup on the sockets, events, etc...
   if (mEventHandle != WSA_INVALID_EVENT)
      WSACloseEvent(mEventHandle);
   mEventHandle = WSA_INVALID_EVENT;

   BHandle item;
   BXTcpSendBuffer* pBuffer = mSend.mQueue.getHead(item);
   while (pBuffer)
   {
      BXTcpSendBuffer::releaseInstance(pBuffer);
      pBuffer = mSend.mQueue.removeAndGetNext(item);
   }
   mSend.mQueue.reset();
   mSend.mpCurrentBuffer = NULL;
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::sendReady()
{
   uint count = mObserverList.getSize();
   for (int i=count-1; i >= 0; --i)
   {
      BXStreamObserver* pObserver = mObserverList[i];
      if (pObserver)
         pObserver->sendReady(*this);
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::setState(eState state, eError error)
{
   mError = error;

   if (mState == state)
      return;

   mState = state;

   if (mState == cStateDisconnected)
   {
      //if (!reconnect())
      //{
         for (int i=mObserverList.getSize()-1; i >= 0; --i)
         {
            BXStreamObserver* pObserver = mObserverList[i];
            if (pObserver)
               pObserver->disconnected(0);
         }
      //}
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = timeGetTime() + mRetryInterval;
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::addObserver(BXStreamConnection::BXStreamObserver* pObserver)
{
   if (pObserver)
   {
      if (!mObserverList.contains(pObserver))
      {
         mObserverList.add(pObserver);
         pObserver->observing(this);

         if (mState == cStateConnected)
            pObserver->connected(*this);
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BXStreamConnection::removeObserver(BXStreamObserver* pObserver) 
{ 
   if (pObserver)
   {
      if (mObserverList.contains(pObserver))
      {
         mObserverList.remove(pObserver); 
         pObserver->observing(0);
         //pObserver->disconnected(0);
      }
   }
} 

BXStreamConnection::BXStreamObserver::BXStreamObserver() :
   mpConn(0)
{
}

BXStreamConnection::BXStreamObserver::~BXStreamObserver()
{
   if (mpConn)
      mpConn->removeObserver(this);
   mpConn = 0;
}

void BXStreamConnection::BXStreamObserver::sendReady(BXStreamConnection&)
{
}

void BXStreamConnection::BXStreamObserver::connected(BXStreamConnection&)
{
}

void BXStreamConnection::BXStreamObserver::dataReceived(const void* pData, int32 size)
{
}

void BXStreamConnection::BXStreamObserver::dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size)
{
}

void BXStreamConnection::BXStreamObserver::disconnected(uint status)
{
}

void BXStreamConnection::BXStreamObserver::shutdown()
{
   if (mpConn)
      mpConn->removeObserver(this);

   mpConn = NULL;
}

uint8 BXStreamConnection::BXStreamObserver::getServiceID() const
{
   return 0;
}

void BXStreamConnection::BXStreamObserver::observing(BXStreamConnection* pConn)
{
   mpConn = pConn;
}

BXStreamConnection * BXStreamConnection::BXStreamObserver::getConnection()
{
   return mpConn;
}
