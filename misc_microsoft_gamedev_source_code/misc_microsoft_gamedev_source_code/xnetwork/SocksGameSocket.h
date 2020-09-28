//==============================================================================
// SocksGameSocket.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#pragma once 
 

//==============================================================================
// Includes

#include "SocksUnreliableSocket.h"
#include "GarbageCollected.h"
#include "maxsendsize.h"
#include "containers\PointerList.h"

//==============================================================================
// Forward declarations

class BSocksUnreliableSocket;

namespace BSocksGameHeaderType
{
   enum 
   { 
      cHello=0, 
      cDisconnect, 
      cHelloResponse, 
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
class BSocksGameHeader
{
   public:
      unsigned char Type;
      unsigned short SeqNum;
      unsigned short SocketID;
      unsigned short Size;
};

//==============================================================================
class BSocksGameResendRequest
{
   public:
      /*unsigned short NumberOfRanges;
      class BRange
      {
         public:
            unsigned short Start;
            unsigned short End;
      };
      BRange *Ranges;*/
            
      enum { cMaxResendRequests = 128 };
      unsigned short NumberOfRequests;
      unsigned short *Requests;
};

//==============================================================================
class BSocksGameConnectHeader
{
   public:
      unsigned char Type;
      unsigned short LocalSocketID;
      unsigned short RemoteSocketID;
      SOCKADDR_IN LocalAddress;
      SOCKADDR_IN RemoteAddress;
};

//==============================================================================
class BSocksGameSendBuffer : public BSendBuffer
{
	public:      
      DWORD SendTime;
      //DWORD ResendRequestTime;
      BSocksGameHeader *Header;      
};

//==============================================================================
class BSocksGameRecvBuffer 
{
	public:		
      unsigned short SeqNum;
      DWORD RecvTime;
      DWORD Size;
      long Type;
      void *Buffer;      
      SOCKADDR_IN RemoteAddress;
};

//==============================================================================
class BSocksGameSocket : public BSocksSocketBase,
                         public BSocksUnreliableSocketUser
{
   public:
      // Constructors
      BSocksGameSocket (IN BObserver * Observer, const SOCKADDR_IN &identifierAddress);            
      
      // Functions      

      // All data is packetized - this is an automatic feature of the SocksGameSockets
      
      virtual HRESULT connect (
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);

      HRESULT connectForwarded (
         IN CONST SOCKADDR_IN * ForwarderAddress,
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);

      //
      // Requests a buffer descriptor.
      //

      virtual HRESULT sendAllocateBuffer (
         IN DWORD MaximumLength,
         OUT BSendBuffer ** ReturnSendBuffer);

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      // The implementation takes ownership of the buffer, regardless of what the return value is.
      //

      virtual HRESULT send (
         IN BSendBuffer * SendBuffer);

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      // The implementation takes ownership of the buffer, regardless of what the return value is.
      //

      virtual HRESULT sendTo (
         IN BSendBuffer * SendBuffer,
         IN CONST SOCKADDR_IN * DestinationAddress) { SendBuffer; DestinationAddress; return E_FAIL; }

      //
      // Call this to free a buffer allocated using sendAllocateBuffer.
      // This is only useful to free a buffer that you have NOT submitted to send().
      //

      virtual HRESULT sendFreeBuffer (
         IN BSendBuffer * SendBuffer);

      virtual HRESULT close (void);

      virtual HRESULT getLocalAddress (
         OUT SOCKADDR_IN * ReturnLocalAddress);

		virtual HRESULT getRemoteAddress (
         OUT SOCKADDR_IN * ReturnRemoteAddress);

      virtual bool recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress);
      

      virtual DWORD getLastRecvTime(void) const { return mLastRecvDataTime; }

      virtual HRESULT dispose(void);

      enum { cMaxFragmentSize = 768 };

   protected:      
      virtual void tic (DWORD timerID);

   private:
      // Functions

      long getHeaderType(const void *buf) const
      {
         return (long)   (    (static_cast<const unsigned char *>(buf))[0] & 0x7F    );
      }

      bool getNoResends(const void *buf) const
      {
         return (bool)   (    ((static_cast<const unsigned char*>(buf))[0] & 0x80) == 0x80     );
      }

      //void GUIDtoString(const unsigned char *src, PSTR strDest, unsigned long destBufLen);
      void updateResponsiveness(void);
      void updateRcvdList(void);
      HRESULT _sendTo(BSendBuffer *buf, const SOCKADDR_IN *DestinationAddress);
      long calcDistance(unsigned short seqNum1, unsigned short seqNum2);
      void calcResendRequests(long actualRemoteSeqNum=-1); // -1 means we don't know, so look through the rcvdlist to find out
      void addResendRequest(unsigned short req);
      HRESULT generateResendRequests(void *buffer, DWORD *size);
      bool recvHello (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress);
      bool recvHelloResponse (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress);
      bool recvData (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress);
      void clearSentList(unsigned short ack);
      HRESULT sendWithPiggyBacks (
         IN BSocksGameSendBuffer * SendBuffer,
         IN BOOL Flush=FALSE,
         IN BOOL Resends=FALSE);

      enum { cConnectTimer=1, cUpdateTimer };
      enum { cConnectTimerInterval=100, cUpdateTimerInterval=250 };      

	   HRESULT allocateSendBuffer (
         IN DWORD MaximumLength,
         OUT BSocksGameSendBuffer ** ReturnBuffer);

		HRESULT freeSendBuffer (
         IN BSocksGameSendBuffer * Buffer);

      HRESULT allocateRecvBuffer (         
         OUT BSocksGameRecvBuffer ** ReturnBuffer, DWORD size);

		HRESULT freeRecvBuffer (
         IN BSocksGameRecvBuffer * Buffer);

      // Variables  

      enum { cResendRequestWindow = 250 };      
      class BResendRequest
      {
         public:
            BResendRequest() : SeqNum(0), SentTime(0) {}
            unsigned short SeqNum;
            DWORD SentTime;

            bool operator==(const BResendRequest &other)
            {
               if ((other.SeqNum == this->SeqNum) && (other.SentTime == this->SentTime))
                  return true;
               else
                  return false;
            }
      };

      enum BState { cDisconnected=0, cConnecting, cConnected };
      enum { cQueueSize=65535/2 };
      enum { cDisconnectSleepTimer = 250 };
      enum { cDisconnectResendAmount = 10 };
      enum { cDefaultResendRequestRatePerSec = 64 }; // FIXME: Scale this back when connection gets bad
      enum { cResendRequestInterval = 1000 };

      unsigned short    mLocalSocketID;
      unsigned short    mRemoteSocketID;
      unsigned short    mLocalSeqNum;
      unsigned short    mRemoteSeqNum;

      BOOL              mDisconnecting;      
      BOOL              mResponsive;
      BOOL              mSentData; // the local seq number is not a good indication for whether I've started sending data or not
      BOOL              mFlushSendBuffers;
      BOOL              mIncludeResendRequest;

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

      BDynamicSimArray<BResendRequest>       mResendRequestVector;
      BPointerList<BSocksGameSendBuffer> mSentList;
      BPointerList<BSocksGameRecvBuffer> mRcvdList;

      SOCKADDR_IN       mLocalAddress;
      SOCKADDR_IN       mLocalIdentifierAddress;
      SOCKADDR_IN       mRemoteAddress;
      SOCKADDR_IN       mRemoteIdentifierAddress;
      SOCKADDR_IN       mForwardingServerAddress;

      unsigned char     mRecvFragmentBuffer[cMaxSendSize];

      #ifndef BUILD_FINAL
      unsigned char     mTempBuffer[cMaxSendSize];
      #endif

      enum
      {
         cPrimaryBuffer=0,
         cSecondaryBuffer,
         cMaxBuffers
      };

      DWORD             mDataIndexes[cMaxBuffers];
      DWORD             mVoiceIndexes[cMaxBuffers];
      unsigned char*    mDataBuffers[cMaxBuffers];
      unsigned char*    mVoiceBuffers[cMaxBuffers];

      void setUnresponsiveTimeoutValue(DWORD dwValue = cUnresponsiveTimeoutValue);


   protected:
      // Destructors
      virtual ~BSocksGameSocket(void);

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

}; // BSocksGameSocket

//==============================================================================

//==============================================================================
// eof: SocksGameSocket.h
//==============================================================================


