//==============================================================================
// xudpsocket.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

// xnetwork
#include "Precompiled.h"

#include "xudpsocket.h"
#include "session.h"
#include "config.h"
#include "econfigenum.h"

// xcore
#include "threading\eventDispatcher.h"
#include "threading\workDistributor.h"

//============================================================================
// 
//============================================================================
BXUDPSocket::BXUDPSocket() :
   mpSendAllocator(NULL),
   mpRecvAllocator(NULL),
   mSendList(&gNetworkHeap),
   mConnectionEventHandle(cInvalidEventReceiverHandle),
   mSocket(INVALID_SOCKET),
   mSendBufferLen(0),
   mRecvBufferLen(0),
   mNumDisconnectSends(XNetwork::cNumDisconnectSends),
   mProtocol(IPPROTO_UDP),
   mThreadIndex(cInvalidIndex),
   mPort(XNetwork::cDefaultPort),
   mShutdown(false),
   mVDP(false)
{
}

//============================================================================
// 
//============================================================================
BXUDPSocket::~BXUDPSocket()
{
}

//============================================================================
// 
//============================================================================
bool BXUDPSocket::init(uint16 port, BXNetBufferAllocator* pSendAllocator, BXNetBufferAllocator* pRecvAllocator, BEventReceiverHandle connHandle, bool enableVDP, BThreadIndex threadIndex)
{
   long numDisconnectSends = 0;
   if (gConfig.get(cConfigNumDisconnectSends, &numDisconnectSends) && numDisconnectSends > 0)
      mNumDisconnectSends = static_cast<uint>(numDisconnectSends);

   mVDP = enableVDP;
   mProtocol = (enableVDP ? IPPROTO_VDP : IPPROTO_UDP);

   mPort = port;

   if (!initSocket())
      return false;

   mThreadIndex = threadIndex;

   eventReceiverInit(mThreadIndex);

   mpSendAllocator = pSendAllocator;
   mpRecvAllocator = pRecvAllocator;

   mConnectionEventHandle = connHandle;

   // initialize our socket and start the overlapped send/recv
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cUDPEventInit);

   return true;
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::deinit()
{
   if (mEventHandle != cInvalidEventReceiverHandle)
      gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cUDPEventDeinit, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);

   eventReceiverDeinit();
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::disconnect(BXNetDisconnect* pEvent)
{
   // need to pass in a list of SOCKADDR_IN for all the connections to broadcast the disconnect packet to
   //
   if (pEvent == NULL)
      return;

   if (mEventHandle == cInvalidEventReceiverHandle)
   {
      HEAP_DELETE(pEvent, gNetworkHeap);
      return;
   }

   // when the socket thread receives this event, it will attempt to create and send a disconnect packet
   // to each SOCKADDR_IN in the payload list
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cUDPEventDisconnect, 0, 0, pEvent, BEventDispatcher::cSendWaitForDelivery);
}

//============================================================================
// 
//============================================================================
bool BXUDPSocket::initSocket()
{
   mSocket = socket(AF_INET, SOCK_DGRAM, mProtocol);

   if (mSocket == INVALID_SOCKET)
   {
      int error = WSAGetLastError();
      nlog(cTransportNL, "BXUDPSocket::initSocket -- failed to create socket [0x%08X]", error);
      return false;
   }

   BOOL OptionValue = TRUE;
   if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&OptionValue), sizeof(OptionValue)) == SOCKET_ERROR)
   {
      int error = WSAGetLastError();
      nlog(cTransportNL, "BXUDPSocket::initSocket -- failed to set SO_REUSEADDR [0x%08X]", error);
      return false;
   }

   SOCKADDR_IN addr;
   addr.sin_family = AF_INET;
   addr.sin_port = mPort;
   addr.sin_addr.s_addr = INADDR_ANY;

   if (bind(mSocket, reinterpret_cast<SOCKADDR*>(&addr), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
   {
      int error = WSAGetLastError();
      nlog(cTransportNL, "BXUDPSocket::initSocket -- failed to bind UDP/IP socket to port %d [0x%08X]", htons(mPort), error);
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
      return false;
   }

   // SO_BROADCAST only supported for IPPROTO_UDP
   if (mProtocol == IPPROTO_UDP)
   {
      OptionValue = TRUE;
      if (setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&OptionValue), sizeof(OptionValue)) == SOCKET_ERROR)
      {
         int error = WSAGetLastError();
         nlog(cTransportNL, "BXUDPSocket::initSocket -- failed to set SO_BROADCAST [0x%08X]", error);
      }
   }

   return true;
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::processInit(const BEvent& event)
{
   mCloseEvent.reset();
   mSendEvent.reset();
   mRecvEvent.reset();

   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;

   handleEvent.mEventClass = cUDPEventClosed;
   gEventDispatcher.registerHandleWithEvent(mCloseEvent, handleEvent);

   handleEvent.mEventClass = cUDPEventSendComplete;
   gEventDispatcher.registerHandleWithEvent(mSendEvent, handleEvent);

   handleEvent.mEventClass = cUDPEventRecvComplete;
   gEventDispatcher.registerHandleWithEvent(mRecvEvent, handleEvent);

   if (WSAEventSelect(mSocket, mCloseEvent.getHandle(), FD_CLOSE) == SOCKET_ERROR)
   {
      HRESULT hr = GetLastResult();
      nlogt(cTransportNL, "BXUDPSocket::processInit -- WSAEventSelect failed, will not be able to receive network events (0x%08x)", hr);
      return;
   }

   Utils::ClearObj(mSendOverlapped);
   mSendOverlapped.hEvent = mSendEvent.getHandle();

   mRecvBufferLen = 0;

   int error = initiateOverlappedRecv();
   if (error != 0)
   {
      // socket closed/unusable
      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventSocketError, error);
   }
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::processDeinit(const BEvent& event)
{
   mShutdown = true;

   gEventDispatcher.deregisterHandle(mCloseEvent.getHandle(), mThreadIndex);
   gEventDispatcher.deregisterHandle(mSendEvent.getHandle(), mThreadIndex);
   gEventDispatcher.deregisterHandle(mRecvEvent.getHandle(), mThreadIndex);

   if (mSocket != INVALID_SOCKET)
   {
      if (WSACancelOverlappedIO(mSocket) == SOCKET_ERROR)
      {
         int retval = WSAGetLastError();
         nlogt(cTransportNL, "BXUDPSocket::processDeinit -- failed to cancel overlapped io 0x%08x", retval);
      }

      if (closesocket(mSocket) == SOCKET_ERROR)
      {
         int error = WSAGetLastError();
         nlogt(cTransportNL, "BXUDPSocket::processDeinit -- failed to close socket 0x%08x", error);
      }
   }

   mSocket = INVALID_SOCKET;

   // don't bother with the send list as all the memory is from a fixed allocator
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::processDisconnect(const BEvent& event)
{
   // send out disconnect packets to each addr in the event payload
   BXNetDisconnect* pPayload = reinterpret_cast<BXNetDisconnect*>(event.mpPayload);
   if (pPayload == NULL)
      return;

   // cancel any existing overlapped io operations
   // loop on overlapped io sends sending a disconnect packet
   // need timeouts for each send
   // if we get a socket error on a particular addr, then we can skip it
   if (mSocket == INVALID_SOCKET)
      return;

   if (WSACancelOverlappedIO(mSocket) == SOCKET_ERROR)
   {
      int retval = WSAGetLastError();
      nlogt(cTransportNL, "BXUDPSocket::processDisconnect -- failed to cancel overlapped io 0x%08x", retval);
      return;
   }

   // wait until we're ready to begin sending data again after cancelling any overlapped operations
   DWORD cbTransfer;
   DWORD dwFlags;
   if (!WSAGetOverlappedResult(mSocket, &mSendOverlapped, &cbTransfer, TRUE, &dwFlags))
   {
      int retval = WSAGetLastError();
      switch (retval)
      {
         case WSA_IO_PENDING:
            {
               nlogt(cTransportNL, "BXUDPSocket::processDisconnect (1) -- WSA_IO_PENDING");
               break;
            }
         case WSA_IO_INCOMPLETE:
            {
               nlogt(cTransportNL, "BXUDPSocket::processDisconnect (1) -- WSA_IO_INCOMPLETE");
               break;
            }
         case WSAECANCELLED:
            {
               nlogt(cTransportNL, "BXUDPSocket::processDisconnect (1) -- WSAECANCELLED");
               break;
            }
         case WSAEHOSTUNREACH:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mSendAddr);
               nlogt(cTransportNL, "BXUDPSocket::processDisconnect (1) -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
               break;
            }
         default:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mSendAddr);
               nlogt(cTransportNL, "BXUDPSocket::processDisconnect (1) -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
               break;
            }
         }
   }

   WSABUF buffer;
   buffer.len = sizeof(pPayload->mPacket);
   buffer.buf = reinterpret_cast<char*>(pPayload->mPacket);

   SOCKADDR_IN addr;

   // want to send X disconnect packets per client
   // start the outer loop with the count
   for (uint c=0; c < mNumDisconnectSends; ++c)
   {
      uint count = pPayload->mAddrs.getSize();
      for (uint i=0; i < count; ++i)
      {
         if (!pPayload->mAddrs[i].mValid)
            continue;

         DWORD bytesSent = 0;

         addr = pPayload->mAddrs[i].mAddr;

         int result = WSASendTo(mSocket, &buffer, 1, &bytesSent, 0, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr), NULL, NULL);

         if (result == SOCKET_ERROR)
         {
            int retval = WSAGetLastError();
            switch (retval)
            {
               case WSA_IO_PENDING:
                  {
                     nlogt(cTransportNL, "BXUDPSocket::processDisconnect (2) -- WSA_IO_PENDING");
                     break;
                  }
               case WSA_IO_INCOMPLETE:
                  {
                     nlogt(cTransportNL, "BXUDPSocket::processDisconnect (2) -- WSA_IO_INCOMPLETE");
                     break;
                  }
               case WSAECANCELLED:
                  {
                     nlogt(cTransportNL, "BXUDPSocket::processDisconnect (2) -- Overlapped IO cancelled");
                     break;
                  }
               case WSAEHOSTUNREACH:
                  {
#ifndef BUILD_FINAL
                     BNetIPString strAddr(addr);
                     nlogt(cTransportNL, "BXUDPSocket::processDisconnect (2) -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
                     pPayload->mAddrs[i].mValid = false;
                     break;
                  }
               default:
                  {
#ifndef BUILD_FINAL
                     BNetIPString strAddr(mSendAddr);
                     nlogt(cTransportNL, "BXUDPSocket::processDisconnect (2) -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
                     pPayload->mAddrs[i].mValid = false;
                  }
            }
         }
      }
   }
}

//============================================================================
// 
//============================================================================
bool BXUDPSocket::emptyRecvBuffer()
{
   if (mShutdown)
      return false;

   // assume VDP protocol for now, if we ever want to re-use the UDP code for
   // some non-game related app, we'll need to either support the VDP protocol
   // or config it out
   uint offset = 0;
   uint len = mRecvBufferLen;
   if (mVDP)
   {
      if (mRecvBufferLen < 2)
         return false;

      const uint dlen = (mRecvBuffer[0] << 8) | mRecvBuffer[1];
      const uint vlen = mRecvBufferLen - dlen - 2;

      if (mRecvBufferLen < dlen + vlen)
         return false;

      len = dlen + vlen;
      offset = 2;
   }

   BASSERT(len <= BXNetBuffer::cBufSize);
   if (len > BXNetBuffer::cBufSize)
      return false;

   BXNetBuffer* pBuf = mpRecvAllocator->alloc();
   //BDEBUG_ASSERT(pBuf);
   if (pBuf == NULL)
      return false;

   pBuf->mTime = timeGetTime();
   pBuf->mAddr = mRecvAddr;
   pBuf->mSize = len;
   Utils::FastMemCpy(pBuf->mBuf, mRecvBuffer + offset, len);
   //pBuf->mSize = dlen;
   //Utils::FastMemCpy(pBuf->mBuf, mRecvBuffer+2, dlen);

   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventUDPRecv, (uint)pBuf);

   //if (vlen > 0)
   //{
   //   pBuf = mpRecvAllocator->alloc();
   //   BDEBUG_ASSERT(pBuf);
   //   if (pBuf == NULL)
   //      return false;

   //   pBuf->mAddr = mRecvAddr;
   //   pBuf->mSize = vlen;
   //   Utils::FastMemCpy(pBuf->mBuf, mRecvBuffer+2+dlen, vlen);

   //   gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventUDPRecv, (uint)pBuf);
   //}

   mRecvBufferLen = 0;

   return true;
}

//============================================================================
// 
//============================================================================
int BXUDPSocket::initiateOverlappedRecv()
{
   // change this to only allow X loops before breaking out
   // because we don't want to stall in here too long
   for (uint i=0; i < 50 && !mShutdown; ++i)
   {
      Utils::ClearObj(mRecvOverlapped);
      mRecvOverlapped.hEvent = mRecvEvent.getHandle();

      mRecvBufferLen = 0;

      WSABUF buffer;
      buffer.len = cRecvBufferSize;
      buffer.buf = (char*)mRecvBuffer;

      DWORD recvBytesRead = 0;
      DWORD recvFlags = 0;

      mRecvAddrLen = sizeof(mRecvAddr);

      int result = WSARecvFrom(mSocket, &buffer, 1, &recvBytesRead, &recvFlags, reinterpret_cast<SOCKADDR*>(&mRecvAddr), &mRecvAddrLen, &mRecvOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         int retval = WSAGetLastError();
         switch (retval)
         {
            case WSA_IO_PENDING:
            case WSA_IO_INCOMPLETE:
               return 0;
            case WSAEMSGSIZE:
               {
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedRecv -- WSAEMSGSIZE recvBytesRead[%d] mRecvBufferLen[%d] recvFlags[0x%08X]", recvBytesRead, mRecvBufferLen, recvFlags);
                  break;
               }
            case WSAECANCELLED:
               {
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedRecv -- Overlapped IO cancelled");
                  return retval;
               }
            case WSAEHOSTUNREACH:
               {
#ifndef BUILD_FINAL
                  BNetIPString strAddr(mRecvAddr);
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedRecv -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
                  return retval;
               }
            default:
               {
#ifndef BUILD_FINAL
                  BNetIPString strAddr(mRecvAddr);
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedRecv -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
                  return retval;
               }
         }
      }

      if (recvBytesRead == 0)
      {
         // Connection was closed.

         // shutdown the socket
         mShutdown = true;
         return WSAESHUTDOWN;
      }

      if (recvFlags != MSG_PARTIAL)
      {
         mRecvBufferLen = recvBytesRead;

         if (!emptyRecvBuffer())
            mRecvBufferLen = 0;
      }
      else
      {
         nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedRecv -- MSG_PARTIAL recvBytesRead[%d] mRecvBufferLen[%d] recvFlags[0x%08X]", recvBytesRead, mRecvBufferLen, recvFlags);
      }
   }

   return 0;
}

//============================================================================
// 
//============================================================================
int BXUDPSocket::processReceiveSocketData()
{
   if (mSocket == INVALID_SOCKET)
      return WSAENOTSOCK;
   if (mShutdown)
      return WSAESHUTDOWN;

   DWORD bytesTransferred = 0;
   DWORD flags = 0;
   const BOOL result = WSAGetOverlappedResult(mSocket, &mRecvOverlapped, &bytesTransferred, FALSE, &flags);

   if (!result)
   {
      int retval = WSAGetLastError();
      switch (retval)
      {
         case WSA_IO_PENDING:
         case WSA_IO_INCOMPLETE:
            return 0;
         case WSAEMSGSIZE:
            {
               nlogt(cTransportNL, "BXUDPSocket::processReceiveSocketData -- WSAEMSGSIZE bytesTransferred[%d] mRecvBufferLen[%d] flags[0x%08X]", bytesTransferred, mRecvBufferLen, flags);
               break;
            }
         case WSAECANCELLED:
            {
               nlogt(cTransportNL, "BXUDPSocket::processReceiveSocketData -- Overlapped IO cancelled");
               break;
            }
         case WSAEHOSTUNREACH:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mRecvAddr);
               nlogt(cTransportNL, "BXUDPSocket::processReceiveSocketData -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
               break;
            }
         default:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mRecvAddr);
               nlogt(cTransportNL, "BXUDPSocket::processReceiveSocketData -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
            }
      }
      return retval;
   }

   if (flags != MSG_PARTIAL)
   {
      mRecvBufferLen = bytesTransferred;
      BDEBUG_ASSERT(mRecvBufferLen <= cRecvBufferSize);

      if (!emptyRecvBuffer())
      {
         mRecvBufferLen = 0;
      }
   }
   else
   {
      nlogt(cTransportNL, "BXUDPSocket::processReceiveSocketData -- MSG_PARTIAL bytesTransferred[%d] mRecvBufferLen[%d] flags[0x%08X]", bytesTransferred, mRecvBufferLen, flags);
   }

   return initiateOverlappedRecv();
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::checkReceive()
{
   int error = processReceiveSocketData();
   if (error != 0)
   {
      clearSendList(mRecvAddr);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventSocketError, error, mRecvAddr.sin_addr.s_addr);
   }
}

//============================================================================
// 
//============================================================================
int BXUDPSocket::initiateOverlappedSend()
{
   DWORD cbTransfer;
   DWORD dwFlags;

   if (!WSAGetOverlappedResult(mSocket, &mSendOverlapped, &cbTransfer, FALSE, &dwFlags))
   {
      int retval = WSAGetLastError();
      switch (retval)
      {
         case WSA_IO_PENDING:
         case WSA_IO_INCOMPLETE:
            return 0;
         case WSAECANCELLED:
            {
               nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (1) -- Overlapped IO cancelled");
               break;
            }
         case WSAEHOSTUNREACH:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mSendAddr);
               nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (1) -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
               break;
            }
         default:
            {
#ifndef BUILD_FINAL
               BNetIPString strAddr(mSendAddr);
               nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (1) -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
            }
      }
      //return retval;
   }

   // change this to only allow X loops before breaking out
   // because we don't want to stall in here too long
   for (uint i=0; i < 10 && !mShutdown; ++i)
   {
      BXNetBuffer* pBuf = mSendList.removeHead();
      if (pBuf == NULL)
         return 0;

      Utils::ClearObj(mSendOverlapped);
      mSendOverlapped.hEvent = mSendEvent.getHandle();

      // copy the data from the buffer into our send buffer
      Utils::FastMemCpy(mSendBuffer, pBuf->mBuf, pBuf->mSize);

      mSendBufferLen = pBuf->mSize;
      mSendAddr = pBuf->mAddr;

      mpSendAllocator->free(pBuf);

      WSABUF buffer;
      buffer.len = mSendBufferLen;
      buffer.buf = (char*)mSendBuffer;

      DWORD bytesSent = 0;

      int result = WSASendTo(mSocket, &buffer, 1, &bytesSent, 0, reinterpret_cast<SOCKADDR*>(&mSendAddr), sizeof(mSendAddr), &mSendOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         int retval = WSAGetLastError();
         switch (retval)
         {
            case WSA_IO_PENDING:
            case WSA_IO_INCOMPLETE:
               return 0;
            case WSAECANCELLED:
               {
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (2) -- Overlapped IO cancelled");
                  break;
               }
            case WSAEHOSTUNREACH:
               {
#ifndef BUILD_FINAL
                  BNetIPString strAddr(mSendAddr);
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (2) -- Host unreachable ip[%s]", strAddr.getPtr());
#endif
                  break;
               }
            default:
               {
#ifndef BUILD_FINAL
                  BNetIPString strAddr(mSendAddr);
                  nlogt(cTransportNL, "BXUDPSocket::initiateOverlappedSend (2) -- Unknown error 0x%08x ip[%s]", retval, strAddr.getPtr());
#endif
               }
         }
         // inform the session layer that we have a bad SOCKADDR
         return retval;
      }

      if (result == 0)
      {
         BDEBUG_ASSERT(bytesSent == mSendBufferLen);
      }

      // pull another buffer off the list to process
   }

   return 0;
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::checkSend()
{
   int error = initiateOverlappedSend();
   if (error != 0)
   {
      clearSendList(mSendAddr);

      gEventDispatcher.send(mEventHandle, mConnectionEventHandle, cNetEventSocketError, error, mSendAddr.sin_addr.s_addr);

      //// iterate the mSendList and remove all pending sends for this addr
      //BHandle hItem;
      //BXNetBuffer* pBuf = mSendList.getHead(hItem);
      //while (pBuf)
      //{
      //   if (pBuf->mAddr.sin_addr.s_addr == mSendAddr.sin_addr.s_addr)
      //   {
      //      mpSendAllocator->free(pBuf);

      //      pBuf = mSendList.removeAndGetNext(hItem);
      //   }
      //   else
      //      pBuf = mSendList.getNext(hItem);
      //}
   }

   checkReceive();
}

//============================================================================
// 
//============================================================================
void BXUDPSocket::clearSendList(const SOCKADDR_IN& addr)
{
   // iterate the mSendList and remove all pending sends for this addr
   BHandle hItem;
   BXNetBuffer* pBuf = mSendList.getHead(hItem);
   while (pBuf)
   {
      if (pBuf->mAddr.sin_addr.s_addr == addr.sin_addr.s_addr)
      {
         mpSendAllocator->free(pBuf);

         pBuf = mSendList.removeAndGetNext(hItem);
      }
      else
         pBuf = mSendList.getNext(hItem);
   }
}

//============================================================================
// 
//============================================================================
bool BXUDPSocket::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cNetEventUDPSend:
         {
            BXNetBuffer* pNetBuf = reinterpret_cast<BXNetBuffer*>(event.mPrivateData);
            BDEBUG_ASSERT(pNetBuf);
            if (pNetBuf == NULL)
               break;

            mSendList.addToTail(pNetBuf);

            checkSend();
            break;
         }

      case cUDPEventSendComplete:
         {
            checkSend();
            break;
         }

      case cUDPEventRecvComplete:
         {
            checkReceive();
            break;
         }

      case cUDPEventInit:
         {
            processInit(event);
            break;
         }

      case cUDPEventDeinit:
         {
            // shutdown our internals
            processDeinit(event);
            break;
         }

      case cUDPEventDisconnect:
         {
            processDisconnect(event);
            break;
         }
   }

   return false;
}
