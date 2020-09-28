//-------------------------------------------------------------------------------------------------
//
// File: tcpClient.cpp
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "tcpClient.h"

int BTcpClient::mSetupCount;

BTcpClient::BTcpClient() :
   mLastWin32Error(0)
{
   clear();
}

BTcpClient::~BTcpClient()
{
   close();
}

void BTcpClient::clear(void)
{
   mSocket = INVALID_SOCKET;
   
   Utils::ClearObj(mSendOverlapped);
   Utils::ClearObj(mRecvOverlapped);

   mSendBufSize = 0;
   mSendBufOfs = 0;

   mReceivingFlag = false;
   mSendingFlag = false;

   mSendQueue.clear();
   mRecvQueue.clear();

   mRecvEvent.reset();
   mSendEvent.reset();

   mConnectionStatus = cNotOpen;
}

void BTcpClient::setBufSize(uint bufSize)
{
   mSendQueue.clear();
   mRecvQueue.clear();
   
   bool success = mSendQueue.resize(bufSize);
   BVERIFY(success);

   success = mRecvQueue.resize(bufSize);
   BVERIFY(success);

   mRecvBuf.resize(bufSize);
   mSendBuf.resize(bufSize);
}

eTcpClientError BTcpClient::connect(const char* pAddress, uint port, uint bufSize, bool enableNoDelay)
{
   BDEBUG_ASSERT(pAddress);
   
   if (mSocket != INVALID_SOCKET)
      return cTCE_NotClosed;
      
   mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (mSocket == INVALID_SOCKET)
   {
      mLastWin32Error = WSAGetLastError();
      return cTCE_SocketError;
   }
   
   if (enableNoDelay)
   {
      BOOL optVal = TRUE;
      setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)(&optVal), sizeof(optVal));
   }

   SOCKADDR_IN sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = (u_short)htons((u_short)port);
   sa.sin_addr.s_addr = inet_addr(pAddress);

   if (::connect(mSocket, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR)
   {
      mLastWin32Error = WSAGetLastError();
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      return cTCE_ConnectError;
   }
   
   setBufSize(bufSize);
   
   mConnectionStatus = cOpen;
      
   return tickOverlappedRecv();
}

eTcpClientError BTcpClient::attach(SOCKET s, uint bufSize, bool enableNoDelay)
{
   if (mSocket != INVALID_SOCKET)
      return cTCE_NotClosed;
      
   mSocket = s;
   
   if (enableNoDelay)
   {
      BOOL optVal = TRUE;
      setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)(&optVal), sizeof(optVal));
   }
   
   setBufSize(bufSize);
   
   mConnectionStatus = cOpen;

   tickOverlappedRecv();
   
   return tickOverlappedRecv();
}

eTcpClientError BTcpClient::close(void)
{
   eTcpClientError status = cTCE_Success;
   
   if (mSocket == INVALID_SOCKET) 
      return cTCE_NotOpen;
   
   if (mConnectionStatus != cFailed)
   {  
      status = sendFlush(true);
      
      if (mConnectionStatus != cFailed)
      {
         if (mSendingFlag)
            mSendEvent.wait();
         
         shutdown(mSocket, SD_SEND);
      }
      
      uchar buf[1024];
      
      const DWORD startTime = GetTickCount();
      
      for ( ; ; )
      {
         if (mConnectionStatus != cOpen)
            break;
            
         for ( ; ; )
         {
            uint receiveLen;
            if (receive(buf, sizeof(buf), receiveLen, 0) < 0)
               break;
            
            if (!receiveLen)
               break;
         }               
         
         if (mConnectionStatus != cOpen)
            break;
         
         if (mReceivingFlag)
            mRecvEvent.wait(10000);

         if ((GetTickCount() - startTime) > 60000)            
            break;
      }
   }      
   
   if (mSendingFlag)
      mSendEvent.wait();
         
   closesocket(mSocket);
   
   clear();
   
   return status;
}

void BTcpClient::emptyReceiveBuffer(uint size)
{
   if (!size)
   {
      // Connection was gracefully closed.
      mConnectionStatus = cClosedGracefully;
   }
   else
   {
      BDEBUG_ASSERT(size <= mRecvQueue.getAvail());

      mRecvQueue.pushBack(size, mRecvBuf.getPtr());
   }

   mReceivingFlag = false;
}

eTcpClientError BTcpClient::tickOverlappedRecv(void)
{
   if ((mSocket == INVALID_SOCKET) || (mConnectionStatus == cNotOpen))
      return cTCE_NotOpen;
   else if (mConnectionStatus == cFailed)
      return cTCE_ConnectionError;
   else if (mConnectionStatus == cClosedGracefully)
      return cTCE_Success;

   if (mReceivingFlag)
   {
      DWORD recvBytesRead = 0;
      DWORD recvFlags = 0;
      BOOL status = WSAGetOverlappedResult(mSocket, &mRecvOverlapped, &recvBytesRead, FALSE, &recvFlags);

      if (!status)
      {
         DWORD lastError = WSAGetLastError();
         if (lastError == WSA_IO_INCOMPLETE)
            return cTCE_Success;

         mReceivingFlag = false;
         mConnectionStatus = cFailed;
         mLastWin32Error = lastError;
         return cTCE_RecvFailed;
      }

      emptyReceiveBuffer(recvBytesRead);
      if (!recvBytesRead)
         return cTCE_Success;
   }

   for ( ; ; )
   {
      BDEBUG_ASSERT(!mReceivingFlag);

      if (!mRecvQueue.getAvail())
         break;

      Utils::ClearObj(mRecvOverlapped);
      mRecvOverlapped.hEvent = mRecvEvent.getHandle();

      WSABUF buffer;
      buffer.len = mRecvQueue.getAvail();
      buffer.buf = (char*)mRecvBuf.getPtr();

      DWORD recvBytesRead = 0;
      DWORD recvFlags = 0;

      int result = WSARecv(mSocket, &buffer, 1, &recvBytesRead, &recvFlags, &mRecvOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         DWORD lastError = WSAGetLastError();
         if (lastError == WSA_IO_PENDING)
         {
            mReceivingFlag = true;
            break;
         }

         mConnectionStatus = cFailed;
         mLastWin32Error = lastError;
         return cTCE_RecvFailed;
      }

      emptyReceiveBuffer(recvBytesRead);
      if (!recvBytesRead)
         break;
   }      

   return cTCE_Success;      
}

eTcpClientError BTcpClient::receive(void* pBuf, uint bufLen, uint& receiveLen, DWORD flags)
{
   receiveLen = 0;
   
   eTcpClientError status = tickOverlappedRecv();
   if (status < 0)
      return status;
      
   status = tickOverlappedSend();
   if (status < 0)
      return status;
      
   if ((!pBuf) || (!bufLen))   
      return cTCE_Success;
      
   if (flags & cReceiveWait)
   {
      // FIXME: The max that can be waited for is the size of the receive queue!
      const uint bytesToReceive = Math::Min<uint>(mRecvQueue.getMaxSize(), bufLen);
      
      while ((mRecvQueue.getSize() < bytesToReceive) && (mConnectionStatus == cOpen))
      {
         if (mReceivingFlag)
            mRecvEvent.wait();
         
         status = tickOverlappedRecv();
         if (status < 0)
            return status;
      }   
   }
   
   const uint bytesToCopy = Math::Min<uint>(mRecvQueue.getSize(), bufLen);
   uchar* pBufBytes = static_cast<uchar*>(pBuf);
   
   receiveLen = bytesToCopy;
      
   if (flags & cReceivePeek)
   {
      for (uint i = 0; i < bytesToCopy; i++)
         pBufBytes[i] = mRecvQueue.peekFront(i);
   }
   else
   {
      uint bytesPopped = mRecvQueue.popFront(bytesToCopy, pBufBytes);
      bytesPopped;
      BDEBUG_ASSERT(bytesPopped == bytesToCopy);
      
      status = tickOverlappedRecv();
      if (status < 0)
         return status;
   }
         
   return cTCE_Success;
}

eTcpClientError BTcpClient::getReceiveBytesAvail(uint& availSize, bool tickConnection)
{
   if (tickConnection)
   {
      eTcpClientError status = tickOverlappedRecv();
      if (status < 0)
         return status;
         
      status = tickOverlappedSend();
      if (status < 0)
         return status;
   }         
   
   availSize = mRecvQueue.getSize();
   return cTCE_Success;
}

eTcpClientError BTcpClient::tickOverlappedSend(void)
{
   if ((mSocket == INVALID_SOCKET) || (mConnectionStatus == cNotOpen))
      return cTCE_NotOpen;
   else if (mConnectionStatus == cFailed)
      return cTCE_ConnectionError;
   
   if (mSendingFlag)
   {
      DWORD sendBytesWritten = 0;
      DWORD sendFlags = 0;
      BOOL status = WSAGetOverlappedResult(mSocket, &mSendOverlapped, &sendBytesWritten, FALSE, &sendFlags);

      if (!status)
      {
         DWORD lastError = WSAGetLastError();
         if (lastError == WSA_IO_INCOMPLETE)
            return cTCE_Success;

         mSendingFlag = false;
         mConnectionStatus = cFailed;
         mLastWin32Error = lastError;
         return cTCE_SendFailed;
      }
      
      mSendingFlag = false;
      mSendBufOfs += sendBytesWritten;
   }

   for ( ; ; )
   {
      BDEBUG_ASSERT(!mSendingFlag);
      
      if (mSendBufOfs == mSendBufSize)
      {
         mSendBufOfs = 0;
         mSendBufSize = 0;
      }
      
      uint bytesToCopy = Math::Min<uint>(mSendBuf.getSize() - mSendBufSize, mSendQueue.getSize());
      if (bytesToCopy)
      {
         mSendQueue.popFront(bytesToCopy, mSendBuf.getPtr() + mSendBufSize);
         mSendBufSize += bytesToCopy;
      }

      if (mSendBufOfs == mSendBufSize)
         break;

      Utils::ClearObj(mSendOverlapped);
      mSendOverlapped.hEvent = mSendEvent.getHandle();

      WSABUF buffer;
      buffer.len = mSendBufSize - mSendBufOfs;
      buffer.buf = (char*)(mSendBuf.getPtr() + mSendBufOfs);

      DWORD sendBytesWritten = 0;
      
      int result = WSASend(mSocket, &buffer, 1, &sendBytesWritten, 0, &mSendOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         DWORD lastError = WSAGetLastError();
         if (lastError == WSA_IO_PENDING)
         {
            mSendingFlag = true;
            break;
         }

         mConnectionStatus = cFailed;
         mLastWin32Error = lastError;
         return cTCE_SendFailed;
      }

      mSendBufOfs += sendBytesWritten;
   }      

   return cTCE_Success;      
}

eTcpClientError BTcpClient::send(const void* pBuf, uint bufLen, bool flush)
{
   const uchar* pBufBytes = static_cast<const uchar*>(pBuf);      

   if ((!flush) && (mSendQueue.getAvail() >= bufLen))      
   {
      if ((pBufBytes) && (bufLen))
         mSendQueue.pushBack(bufLen, pBufBytes);
      return cTCE_Success;
   }
   
   eTcpClientError status;
   
   if (mConnectionStatus != cClosedGracefully)
   {
      status = tickOverlappedRecv();
      if (status < 0)
         return status;
   }
   
   status = tickOverlappedSend();
   if (status < 0)
      return status;
      
   if ((!pBuf) || (!bufLen))
      return cTCE_Success;
      
   uint bytesRemaining = bufLen;
   
   while (bytesRemaining)
   {
      uint bytesToCopy = Math::Min(mSendQueue.getAvail(), bytesRemaining);
      if (bytesToCopy)
      {
         mSendQueue.pushBack(bytesToCopy, pBufBytes);
         bytesRemaining -= bytesToCopy;
         pBufBytes      += bytesToCopy;
      }
      
      status = tickOverlappedSend();
      if (status < 0)
         return status;
      
      if (!bytesRemaining)
         break;

      if (mSendingFlag)
      {
         mSendEvent.wait();
         
         status = tickOverlappedSend();
         if (status < 0)
            return status;
      }
         
      if (mConnectionStatus != cClosedGracefully)
      {
         status = tickOverlappedRecv();
         if (status < 0)
            return status;
      }
   }

   return cTCE_Success;
}

eTcpClientError BTcpClient::getSendBytesPending(uint& pendingSize)
{
   eTcpClientError status = tickOverlappedSend();
   if (status < 0)
      return status;

   pendingSize = mSendQueue.getSize();
   pendingSize += (mSendBufSize - mSendBufOfs);
   
   return cTCE_Success;
}

eTcpClientError BTcpClient::sendFlush(bool wait)
{
   eTcpClientError status = send(NULL, 0, true);

   if ((status == cTCE_Success) && (wait))
   {
      for ( ; ; )
      {
         uint pendingBytes;   
         status = getSendBytesPending(pendingBytes);
         if (status < 0)
            break;

         if (mSendingFlag)
            mSendEvent.wait();

         if (!pendingBytes)
            break;
      }
   }      

   return status;
}

bool BTcpClient::winsockSetup(void)
{
   if (!mSetupCount)
   {
      WORD wVersionRequested;
      WSADATA wsaData;
      int err;

      wVersionRequested = MAKEWORD(2, 2);

      err = WSAStartup(wVersionRequested, &wsaData);
      if (err != 0)
         return false;

      if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
      {
         WSACleanup();
         return false; 
      }
      
      mSetupCount++;
   }  
   
   return true;    
}

void BTcpClient::winsockCleanup(void)
{
   if (mSetupCount)
   {
      mSetupCount--;
      
      if (!mSetupCount)
         WSACleanup();
   }         
}

BTcpListener::BTcpListener()
{
   mSocket = INVALID_SOCKET;
   mLastWin32Error = 0;
}

BTcpListener::~BTcpListener()
{
   close();
}

eTcpClientError BTcpListener::listen(const char* pAddress, uint port, BString* pListenAddress)
{
   if (mSocket != INVALID_SOCKET)
      return cTCE_NotConnected;
      
   mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (mSocket == INVALID_SOCKET)
   {
      mLastWin32Error = WSAGetLastError();
      return cTCE_SocketError;
   }
   
   SOCKADDR_IN sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = (u_short)htons((u_short)port);
   if (pAddress)
      sa.sin_addr.s_addr = inet_addr(pAddress);
   else
   {
      char buf[256];
      if (gethostname(buf, sizeof(buf)))
         return cTCE_GetHostNameFailed;
                  
      struct hostent* p = gethostbyname(buf);
      if (!p)
         return cTCE_GetHostNameFailed;
         
      sa.sin_addr.s_addr = *(u_long*)p->h_addr_list[0];
   }
   
   if (pListenAddress)
      pListenAddress->set(inet_ntoa(sa.sin_addr));
   
   int status = bind(mSocket, (const sockaddr*)(&sa), sizeof sa);
   if (status)
   {
      mLastWin32Error = WSAGetLastError();
      
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      
      return cTCE_BindFailed;
   }
   
   status = ::listen(mSocket, SOMAXCONN);
   if (status)
   {
      mLastWin32Error = WSAGetLastError();
      
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      
      return cTCE_ListenFailed;
   }
   
   u_long val = TRUE;
   ioctlsocket(mSocket, FIONBIO, &val);
   
   mEvent.reset();
   
   status = WSAEventSelect(mSocket, mEvent.getHandle(), FD_ACCEPT);
   if (status)
   {
      mLastWin32Error = WSAGetLastError();
      
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      
      return cTCE_SocketError;
   }
   
   return cTCE_Success;
}

eTcpClientError BTcpListener::close(void)
{
   if (mSocket == INVALID_SOCKET)
      return cTCE_NotOpen;
      
   closesocket(mSocket);
   mSocket = INVALID_SOCKET;
      
   return cTCE_Success;
}

eTcpClientError BTcpListener::accept(SOCKET* pSocket, sockaddr* pAddr, DWORD waitTime)
{
   if (mSocket == INVALID_SOCKET)
      return cTCE_NotConnected;
   
   for ( ; ; )
   {
      int addrin = sizeof(SOCKADDR_IN);
      SOCKET s = ::accept(mSocket, pAddr, &addrin);
      
      if (s != INVALID_SOCKET)
      {
         *pSocket = s;
         break;
      }
      
      DWORD lastError = WSAGetLastError();
      if ((waitTime) && (lastError == WSAEWOULDBLOCK))
      {
         if (!mEvent.wait(waitTime))
            return cTCE_WaitTimedOut;
      }
      else
      {
         mLastWin32Error = lastError;
         return cTCE_AcceptFailed;
      }
   }  
   
   return cTCE_Success;    
}
