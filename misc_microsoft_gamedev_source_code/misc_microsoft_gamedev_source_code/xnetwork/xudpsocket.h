//==============================================================================
// xudpsocket.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "NetEvents.h"
#include "xconnection.h"

//============================================================================
//============================================================================
class BXUDPSocket : public BEventReceiver
{
   public:
      BXUDPSocket();
     ~BXUDPSocket();

      enum
      {
         cUDPEventInit = cNetEventFirstUser,
         cUDPEventDeinit,

         cUDPEventClosed,

         cUDPEventRecvComplete,
         cUDPEventSendComplete,

         cUDPEventDisconnect,

         cUDPEventTotal
      };

      //-- MAIN Thread 
      bool  init(uint16 port, BXNetBufferAllocator* pSendAllocator, BXNetBufferAllocator* pRecvAllocator, BEventReceiverHandle connHandle, bool enableVDP, BThreadIndex threadIndex=cThreadIndexIO);
      void  deinit();

      void  disconnect(BXNetDisconnect* pEvent);

   private:

      bool  initSocket();

      void  processInit(const BEvent& event);
      void  processDeinit(const BEvent& event);
      void  processDisconnect(const BEvent& event);

      bool  emptyRecvBuffer();
      int   initiateOverlappedRecv();
      int   processReceiveSocketData();
      void  checkReceive();

      int   initiateOverlappedSend();
      void  checkSend();

      void  clearSendList(const SOCKADDR_IN& addr);

      bool  receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      enum { cSendBufferSize = 2048 };
      BYTE mSendBuffer[cSendBufferSize];

      enum { cRecvBufferSize = 2048 };
      BYTE mRecvBuffer[cRecvBufferSize];

      BPointerList<BXNetBuffer> mSendList;

      WSAOVERLAPPED mSendOverlapped;
      WSAOVERLAPPED mRecvOverlapped;

      SOCKADDR_IN mSendAddr;
      SOCKADDR_IN mRecvAddr;

      BEventReceiverHandle mConnectionEventHandle;

      SOCKET mSocket;

      uint mSendBufferLen;
      uint mRecvBufferLen;

      BWin32Event mCloseEvent;
      BWin32Event mSendEvent;
      BWin32Event mRecvEvent;

      uint mNumDisconnectSends;

      int mRecvAddrLen;
      int mProtocol;

      BXNetBufferAllocator* mpSendAllocator;
      BXNetBufferAllocator* mpRecvAllocator;

      BThreadIndex mThreadIndex;

      uint16 mPort;

      bool mShutdown : 1;
      bool mVDP : 1;
};
