//-------------------------------------------------------------------------------------------------
//
// File: tcpClient.h
// Copyright (c) 2007, Ensemble Studios
//
//-------------------------------------------------------------------------------------------------
#pragma once
#include "threading\win32Event.h"
#include "containers\byteQueue.h"

#ifdef XBOX
   #include <winsockx.h>
#else
   #include <winsock2.h>
#endif

enum eTcpClientError
{  
   cTCE_Success         = 0,
   cTCE_NotOpen         = -1000,
   cTCE_NotClosed,
   cTCE_NotConnected,
   cTCE_SendFailed,
   cTCE_RecvFailed,
   cTCE_ListenFailed,
   cTCE_AcceptFailed,
   cTCE_ConnectionClosed,
   cTCE_ConnectionError,
   cTCE_SocketError,
   cTCE_ConnectError,
   cTCE_BindFailed,
   cTCE_WaitTimedOut,
   cTCE_GetHostNameFailed,
};

class BTcpClient
{
   BTcpClient(const BTcpClient&);
   BTcpClient& operator= (const BTcpClient&);
   
public:
   enum { cDefaultBufSize = 1024 * 1024 };
   
   BTcpClient();
   ~BTcpClient();

   DWORD             getLastWin32Error(void) { DWORD lastError = mLastWin32Error; mLastWin32Error = 0; return lastError; }
         
   eTcpClientError   connect(const char* pAddress, uint port, uint bufSize = cDefaultBufSize, bool enableNoDelay = false);
   eTcpClientError   attach(SOCKET s, uint bufSize = cDefaultBufSize, bool enableNoDelay = false);
      
   eTcpClientError   close(void);
   
   BWin32Event&      getSendEvent(bool* pSendIsPending = NULL) { if (pSendIsPending) *pSendIsPending = mSendingFlag; return mSendEvent; }
   BWin32Event&      getReceiveEvent(bool* pReceiveIsPending = NULL) { if (pReceiveIsPending) *pReceiveIsPending = mReceivingFlag; return mRecvEvent; }

   eTcpClientError   send(const void* pBuf, uint bufLen, bool flush);
   eTcpClientError   sendFlush(bool wait);
   
   eTcpClientError   getSendBytesPending(uint& pendingSize);
      
   enum eReceiveFlags
   {
      cReceiveWait = 1,
      cReceivePeek = 2
   };
   
   eTcpClientError   receive(void* pBuf, uint bufLen, uint& receiveLen, DWORD flags = 0);
   eTcpClientError   getReceiveBytesAvail(uint& availSize, bool tickConnection = true);
      
   enum eStatus
   {
      cNotOpen,
      cOpen,
      cClosedGracefully,
      cFailed
   };
   eStatus           getConnectionStatus(void) const { return mConnectionStatus; }
   
   static bool       winsockSetup(void);
   static void       winsockCleanup(void);
      
private:
   SOCKET            mSocket;
   
   DWORD             mLastWin32Error;
   
   // Sending   
   BWin32Event       mSendEvent;
   WSAOVERLAPPED     mSendOverlapped;
            
   BByteQueue        mSendQueue;
      
   BByteArray        mSendBuf;
   uint              mSendBufSize;
   uint              mSendBufOfs;
   bool              mSendingFlag;

   // Receiving
   BWin32Event       mRecvEvent;
   WSAOVERLAPPED     mRecvOverlapped;
   
   BByteArray        mRecvBuf;
   bool              mReceivingFlag;
      
   BByteQueue        mRecvQueue;
         
   eStatus           mConnectionStatus;
   
   static int        mSetupCount;
   
   void              clear(void);
   void              emptyReceiveBuffer(uint size);
   eTcpClientError   tickOverlappedRecv(void);
   eTcpClientError   tickOverlappedSend(void);
   void              setBufSize(uint bufSize);
};

class BTcpListener
{
   BTcpListener(const BTcpListener&);
   BTcpListener& operator= (const BTcpListener&);
   
public:
   BTcpListener();
   ~BTcpListener();

   DWORD             getLastWin32Error(void) { DWORD lastError = mLastWin32Error; mLastWin32Error = 0; return lastError; }

   eTcpClientError   listen(const char* pAddress, uint port, BString* pListenAddress = NULL);
   eTcpClientError   close(void);

   BWin32Event&      getEvent(void) { return mEvent; }
      
   eTcpClientError   accept(SOCKET* pSocket, sockaddr* pAddr = NULL, DWORD waitTime = 0);
      
private:
   SOCKET            mSocket;
   BWin32Event       mEvent;
   BString           mListenAddress;
   
   DWORD             mLastWin32Error;
};
