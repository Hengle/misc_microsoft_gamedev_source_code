//==============================================================================
// UDP360Connection.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

#pragma once

#ifndef __UDP360CONNECTION_H__
#define __UDP360CONNECTION_H__

//==============================================================================
// Includes
#include "UDP360Socket.h"
#include "LoopbackSocket.h"
#include "GarbageCollected.h"
#include "maxsendsize.h"
#include "containers\PointerList.h"

//==============================================================================
// Forward declarations

namespace BUDPConnectionHeaderType
{
   enum 
   { 
      cHello=0, 
      cDisconnect, 
      cHelloResponse, 
      cConnectVerification,
      cData,
      cDataFragment,
      cDataFragmentDone,
      cResendRequest, 
      cAck,
      cAckRemote,
      cDie,
      cAddressRequest1Packet,
      cAddressRequest2Packet,
      cRepeatedPacket,

      cNumberOfTypes 
   };
};

//==============================================================================
class BUDPConnectionHeader
{
public:
   uint8  Type;
   uint16 SeqNum;
   uint16 SocketID;
   uint16 Size;
};

//==============================================================================
class BUDPConnectionResendRequest
{
   public:
      enum { cMaxResendRequests = 128 };
      uint16  NumberOfRequests;
      uint16* Requests;
};

//==============================================================================
class BUDPConnectionConnectHeader
{
   public:
      uint8  Type;
      uint16 LocalSocketID;
      uint16 RemoteSocketID;
};

//==============================================================================
class BUDPConnectionSendBuffer : public BSendBuffer
{
   public:
      DWORD SendTime;
      BUDPConnectionHeader *Header;      
};

//==============================================================================
class BUDPConnectionRecvBuffer 
{
   public:		
      uint16         SeqNum;
      uint32         RecvTime;
      uint32         Size;
      int32          Type;
      void*          Buffer;      
      SOCKADDR_IN    RemoteAddress;
};

//==============================================================================
class BUDP360Connection : public BSocket::BObserver
{
   public:
      class BConnObserver
      {
         public:
            // Interface to be implemented by classes that are our observer  
            //TODO - mod this interface to pass the voice data length
            virtual void dataReceived (
               IN BUDP360Connection* Connection,
               IN const void * Buffer,
               IN LONG Length,
               IN CONST SOCKADDR_IN* RemoteAddress) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (Buffer);
               UNREFERENCED_PARAMETER (Length);
               UNREFERENCED_PARAMETER (RemoteAddress);
            }

            virtual void disconnected (
               IN BUDP360Connection* Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }

            virtual bool notifyUnresponsiveConnection (
               IN BUDP360Connection * Connection, DWORD lastRecvTime)
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (lastRecvTime);
               return true;
            }

            virtual bool notifyResponsiveConnection (
               IN BUDP360Connection * Connection, DWORD lastRecvTime)
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (lastRecvTime);
               return true;
            }

            virtual void connected (
               IN BUDP360Connection * Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }

            virtual void connectTimeout (
               IN BUDP360Connection * Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }
      };

      // Constructors
      BUDP360Connection(IN BConnObserver* Observer);
      // Destructors
      ~BUDP360Connection();

      // Functions      
      //Use this connect when you have a XNADDR, port, and security key id - you are initiating the connection
      HRESULT connect( BUDP360Socket* socket, const XNADDR &targetXNAddr, WORD port, XNKID* key );
      //Use this connect when you are using a loopback socket
      HRESULT connect( BLoopbackSocket* socket );
      //Use this connect when you have received a packet from the target already, so you have a sockaddr_in (from the recv callback)
      HRESULT connect( BUDP360Socket* socket, const SOCKADDR_IN &targetSOCKADDR, XNKID* key );
      //Use this when you have an XNADDR, and have been told by the host to connect - but you are NOT initialing the connection
      HRESULT connectPending( BUDP360Socket* socket, const XNADDR &targetXNAddr, WORD port, XNKID* key );
      //Call this when you have a completion for a pending connection
      HRESULT establishPendingConnection( const SOCKADDR_IN &targetSOCKADDR );

      HRESULT connect(BUDP360Socket* pSocket, const IN_ADDR& lspAddr, ushort port);

      void disconnect();

      //
      // Requests a buffer descriptor.
      HRESULT sendAllocateBuffer(IN DWORD MaximumLength, OUT BSendBuffer** ReturnSendBuffer);

      //This send takes the data and builds a buffer for it, then sends it
      HRESULT send(const void* pData, const long size, const DWORD flags=0);

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      // The implementation takes ownership of the buffer, regardless of what the return value is.
      HRESULT send(IN BSendBuffer* SendBuffer);

      //
      // Call this to free a buffer allocated using sendAllocateBuffer.
      // This is only useful to free a buffer that you have NOT submitted to send().
      HRESULT sendFreeBuffer(IN BSendBuffer* SendBuffer);

      HRESULT close();

      //BSocket::BObserver Interface implementation
      void recvd(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* RemoteAddress);
      void disconnected();

      DWORD getLastRecvTime() const { return mLastRecvDataTime; }
      //Returns true if we are connected and ready to send/recv
      bool isConnected() const { return mState==cConnected?true:false; }   
      //Returns true if we are trying to connect
      bool isConnecting() const { return (mState>cDisconnected && mState<cConnected)?true:false; }
      //Returns true if we are in a connection pending state, and if this XNADDR is the same as ours
      bool pendingXNAddrIsSameAs(XNADDR* checkXNAddr);

      XNADDR*  getXNADDR() { return &mRemoteXNADDR; }
      const XNADDR*  getXNADDR() const { return &mRemoteXNADDR; }

      ushort   getPort() const { return mPort; }

      HRESULT dispose();

      enum { cMaxFragmentSize = 768 };

      // turn the timer on/adjust the interval
      void setTimer(DWORD interval)
      {
         if (interval == 0)
         {
            mActiveTimer = cTimerNone;
            mTimerInterval = 0;
            mTimerLastTick = 0;
            mTimerModifier = 0; // the timer modifier is set by the recv logic of the connection, but we need to zero it out here and allow that logic to reset if necessary
         }
         else
         {
            mActiveTimer = cUpdateTimer;
            mTimerInterval = interval;
            mTimerLastTick = GetTickCount();
         }
      }

      void service();

      unsigned long sentPending() const { return mSentList.getSize(); }

      static void tickActiveConnections();

   protected:
      void tic(DWORD timerID);

   private:
      // Functions
      const BSocket* getSocket() const { return mSocket; };
      BSocket* getSocket() { return mSocket; };
      
      const BConnObserver* getObserver() const { return mObserver; }
      BConnObserver* getObserver() { return mObserver; }

      void allocateDataVoiceBuffers();
      void addToActiveConnections();
      void removeFromActiveConnections();

      int32 getHeaderType(const void* buf) const
      {
         return static_cast<int32>(static_cast<const uchar*>(buf)[0] & 0x7F);
      }

      BOOL getNoResends(const void* buf) const
      {
         return static_cast<BOOL>((static_cast<const uchar*>(buf)[0] & 0x80) == 0x80);
      }

      void updateResponsiveness();
      void updateRcvdList();
      //This method actually takes the buffer and sends it on the bound socket out to the remote target
      HRESULT socketLevelSend( BSendBuffer *buf );
      void calcResendRequests(int32 actualRemoteSeqNum=-1); // -1 means we don't know, so look through the rcvdlist to find out
      void addResendRequest(unsigned short req);
      HRESULT generateResendRequests(void* buffer, DWORD* size);

      bool recvHello(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN* RemoteAddress);
      bool recvHelloResponse(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN* RemoteAddress);
      bool recvConnectionVerificationResponse(IN BSocket* Socket, IN const void * Buffer, IN DWORD Length, IN CONST SOCKADDR_IN* RemoteAddress);
      bool recvData(IN BSocket* pSocket, IN const void * pBuffer, IN DWORD length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* pRemoteAddress);

      void clearSentList(unsigned short ack);
      HRESULT sendWithPiggyBacks(IN BUDPConnectionSendBuffer* SendBuffer, IN BOOL Flush=FALSE, IN BOOL Resends=FALSE);

      enum { cTimerNone = 0, cConnectTimer, cUpdateTimer };
      enum { cConnectTimerInterval=100, cUpdateTimerInterval=250 };      

      // Variables  

      enum { cResendRequestWindow = 250 };      
      class BResendRequest
      {
      public:
         BResendRequest() : SeqNum(0), SentTime(0) {}
         DWORD SentTime;
         unsigned short SeqNum;

         bool operator==(const BResendRequest &other) const
         {
            if ((other.SeqNum == this->SeqNum) && (other.SentTime == this->SentTime))
               return true;
            else
               return false;
         }
      };

      //NOTE: Do not change the order of these or add to them without updating isConnecting()
      enum BState {  cDisconnected=0,              //Connection is NOT connected or trying to connect
                     cConnectPending,              //We are waiting for the other side to initiate this connection with a hello message
                     cXNetPreConnect,              //We are waiting for the XNetConnect call to verify connectivity to other side
                     cConnecting,                  //We are sending hellos and waiting to get a hellorecvd echoed to us
                     cConnectVerification,         //We have a hellorecvd, have send a connectVerify and are waiting on a connectVerify from them
                     cConnected };                 //We have recived a connectVerify and are fully connected at this point

      enum { cQueueSize=65535/2 };
      enum { cDisconnectSleepTimer = 250 };
      enum { cDisconnectResendAmount = 10 };
      enum { cDefaultResendRequestRatePerSec = 64 }; // FIXME: Scale this back when connection gets bad
      enum { cResendRequestInterval = 1000 };
      enum { cUnresponsiveTimeoutValue = 10000 };

      uint16            mLocalSocketID;
      uint16            mRemoteSocketID;
      uint16            mLocalSeqNum;
      uint16            mRemoteSeqNum;

      ushort            mPort;

      BOOL              mDisconnecting;      
      BOOL              mResponsive;
      BOOL              mSentData; // the local seq number is not a good indication for whether I've started sending data or not
      BOOL              mFlushSendBuffers;
      BOOL              mIncludeResendRequest;
      BOOL              mBoundToLoopbackSocket;
      BOOL              mXNKeyRegistered;

      BState            mState;     
      DWORD             m_dwUnresponsiveTimeoutValue;
      DWORD             m_dwUnresponsiveCount;
      DWORD             mLastRecvDataTime;
      DWORD             mLastUnresponsiveTime;
      DWORD             mLastResponsiveTime;
      DWORD             mConnectingTimer;
      DWORD             mSendHelloTimer;
      DWORD             mResendRequestAmount;
      DWORD             mResendRequestTimer;
      DWORD             mResendRequestRatePerSec;
      DWORD             mRecvFragmentBufferPtr;

      DWORD             mBufferSet;

      DWORD             mTimerModifier;
      DWORD             mTimerInterval;
      DWORD             mTimerLastTick; 
      DWORD             mActiveTimer;
      
      BDynamicSimArray<BResendRequest>       mResendRequestVector;
      BPointerList<BUDPConnectionSendBuffer> mSentList;
      BPointerList<BUDPConnectionRecvBuffer> mRcvdList;

      BSocket*          mSocket;
      BUDP360Socket*    mUDP360Socket;       //Handle to the socket it was created on, kept for a type-safe call to a proper clean-up method
      BConnObserver*    mObserver;  

      XNADDR            mRemoteXNADDR;
      XNKID             mCommKey;
      SOCKADDR_IN       mRemoteAddress;

      uchar             mRecvFragmentBuffer[cMaxSendSize];

      static uint8 mNumActiveConnections;
      enum { cMaxActiveConnections = 32 };
      static BUDP360Connection* mpActiveConnections[cMaxActiveConnections];

#ifndef BUILD_FINAL
      unsigned char     mTempBuffer[cMaxSendSize];
#endif

      enum
      {
         cPrimaryBuffer=0,
         cSecondaryBuffer,
         cMaxBuffers
      };

      uint32            mDataIndexes[cMaxBuffers];
      uint32            mVoiceIndexes[cMaxBuffers];
      uchar*            mDataBuffers[cMaxBuffers];
      uchar*            mVoiceBuffers[cMaxBuffers];

      bool              mStopAcks : 1;

      void setUnresponsiveTimeoutValue(DWORD dwValue = cUnresponsiveTimeoutValue);

   protected:

#ifndef BUILD_FINAL
      //
      // performance counters below added by bgoodman
      //   

   public:
      DWORD getByteReceiveCount() const;
      DWORD getByteReceiveCountPerSecond() const;
      DWORD getPacketReceiveCount() const;
      DWORD getPacketResendRequestCount() const;
      DWORD getPacketResendRequestCountPerSecond() const;
      DWORD getInSequencePacketReceiveCountPerSecond() const;
      DWORD getInSequencePacketReceiveCount() const;
      DWORD getOutOfSequencePacketReceiveCountPerSecond() const;
      DWORD getOutOfSequencePacketReceiveCount() const;

   private:
      DWORD m_dwByteReceiveCount;
      DWORD m_dwPacketReceiveCount;
      DWORD m_dwPacketResendRequestCount;
      DWORD m_dwInSequencePacketReceiveCount;
      DWORD m_dwOutOfSequencePacketReceiveCount;
      DWORD m_dwPacketOneSecondDeltaBegin;
      DWORD m_dwPacketOneSecondDeltaEnd;

      enum { cAckTimeout = 60000 };
#else
      enum { cAckTimeout = 30000 };
#endif

}; // BUDP360Connection

//==============================================================================

#endif // __UDP360CONNECTION_H__
