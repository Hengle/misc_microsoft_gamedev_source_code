//==============================================================================
// BUDP360Socket.h
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
#include "Socket.h"
#include "SocksSocketBase.h"
#include "maxsendsize.h"
#include "containers\PointerList.h"
#ifndef BUILD_FINAL
#include "NetStatsTracker.h"
#endif

//==============================================================================
// Forward declarations
class BUDP360Socket;
class BSocket::BObserver;


class BUDP360SocketIPMapping
{
   public:
      BUDP360SocketIPMapping() :
         mTargetContext(NULL)
      {
         mTargetIP.s_addr = 0;
      };

      BUDP360SocketIPMapping(IN_ADDR* targetIP, BSocket::BObserver* mObserver ) :
         mTargetContext(mObserver)
      {
         mTargetIP.s_addr = targetIP->s_addr;
      };

      IN_ADDR              mTargetIP;
      BSocket::BObserver*  mTargetContext;   
};


class BUDP360SocketBuffer : public BSendBuffer
{
   public:
      SOCKADDR       RemoteAddress;
      INT            RemoteAddressLength;
      BUDP360Socket* Socket;
};

//==============================================================================
// 
//==============================================================================
class BUDP360Socket : public BSocket
{
public:
   enum { DefaultRecvAllocationLength = 0x1000 };
   enum { DefaultRecvActiveGoal = 4 };

public:
   // Constructors 
   BUDP360Socket(IN BObserver* pObserver=0, bool enableVoice=true);
   // Destructors
   ~BUDP360Socket();

   static void tickActiveSockets();

   //BSocksSocketBase interface implementation methods =========================
   HRESULT sendAllocateBuffer (IN DWORD MaximumLength, OUT BSendBuffer** ReturnSendBuffer);

   HRESULT sendFreeBuffer(IN BSendBuffer* SendBuffer);

   HRESULT sendTo(IN BSendBuffer* SendBuffer, IN CONST SOCKADDR_IN* DestinationAddress);

   HRESULT broadcastLAN(IN BSendBuffer* SendBuffer, WORD port);

   HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * DestinationAddress);

   ////Deprecated - old IP based interface ****************
   //HRESULT connect(IN CONST SOCKADDR_IN* RemoteAddress, IN CONST SOCKADDR_IN* LocalAddress OPTIONAL) { BASSERT(false); return E_FAIL; }
   ////Deprecated - old IP based interface ****************
   HRESULT send(IN BSendBuffer* SendBuffer) { BASSERT(false); return E_FAIL; }
   ////Deprecated - old IP based interface ****************
   ///*virtual HRESULT sendTo (
   //   IN BSendBuffer * SendBuffer,
   //   IN CONST SOCKADDR_IN * DestinationAddress) { BASSERT(false); return E_FAIL;}
   //   */
   ////Deprecated - old IP based interface ****************
   //HRESULT getLocalAddress(OUT SOCKADDR_IN* ReturnLocalAddress) { BASSERT(false); return E_FAIL; }
   ////Deprecated - old IP based interface ****************
   //HRESULT getRemoteAddress(OUT SOCKADDR_IN* ReturnRemoteAddress) { BASSERT(false); return E_FAIL; }

   //BUDP360Socket Methods ==================================================
   HRESULT createAndBind(IN u_short port);
   HRESULT init(ushort port);

   HRESULT close();

   bool addIPMappedObserver(IN_ADDR* targetIP, BObserver* targetContext);
   void removeIPMappedObserver(IN_ADDR* targetIP);
   BSocket::BObserver* findIPMappedObserver(IN_ADDR* targetIP);

   HRESULT dispose();

   DWORD getLastRecvTime() const { return mLastRecvDataTime; }
   SOCKET getSocket() const { return mSocket; }

private:

   HRESULT sendNextBuffer ();

   void sendReady();

   BOOL recvReady();

   void GUIDtoString(unsigned char *src, PSTR strDest, unsigned long destBufLen);

   HRESULT allocateBuffer(IN DWORD MaximumLength, OUT BUDP360SocketBuffer** ReturnBuffer);

   HRESULT freeBuffer(IN BUDP360SocketBuffer* Buffer);

   // Creates the message routing window (if necessary)
   HRESULT asyncSelect(IN DWORD EventMask);

   // Destroys the message routing window.
   void asyncStop();

   void networkClose(IN HRESULT Result);

   void checkForEvents(bool tickTimers = true);
   void removeFromActiveSockets();

private:

   char mSendBuffer[cMaxSendSize];

   //
   // Receive State Machine
   //
   struct BRecvState
   {
      LPBYTE Buffer;
      DWORD MaximumLength;

      BRecvState (void)
      {
         Buffer = NULL;
         MaximumLength = 0;
      }

      ~BRecvState (void)
      {
         if (Buffer != NULL)
         {
            delete [] Buffer;
            Buffer = NULL;
            MaximumLength = 0;
         }
      }

      HRESULT grow (
         IN DWORD RequiredLength)
      {
         if (RequiredLength <= MaximumLength)
         {
            return S_OK;
         }

         DWORD NewMaximumLength = RequiredLength * 3 / 2;
         LPBYTE NewBuffer = new BYTE [NewMaximumLength];

         if (NewBuffer == NULL)
         {
            return E_OUTOFMEMORY;
         }

         if (Buffer != NULL)
         {
            delete [] Buffer;
         }

         Buffer = NewBuffer;
         MaximumLength = RequiredLength;
         return S_OK;
      }

   } mRecv;

   //
   // Send State Machine
   //
   struct BSendState
   {
      // contains BSocksUnreliableBuffer, of buffers waiting to be transmitted
      BPointerList<BUDP360SocketBuffer> mQueueList;

      BSendState (void)
      {         
      }

      ~BSendState (void)
      {
         BASSERT (mQueueList.getSize() == 0);
      }

   } mSend;

   static int mNumActiveSockets;
   enum { MaxActiveSockets = 32 };
   static BUDP360Socket* mpActiveSockets[MaxActiveSockets];

   BDynamicSimArray<BUDP360SocketIPMapping> mIPMapping;

   SOCKADDR_IN       mLocalAddress;
   DWORD             mLastRecvDataTime;

#ifndef BUILD_FINAL
   BNetStatsTracker* mpTracker;
#endif

   DWORD             mEventMask;
   SOCKET            mSocket;
   WSAEVENT          mEventHandle;
   bool              mSendMessages : 1;
   bool              mEnableVoice : 1;

}; // BUDP360Socket

//==============================================================================
// eof: UDP360Socket.h
//==============================================================================
