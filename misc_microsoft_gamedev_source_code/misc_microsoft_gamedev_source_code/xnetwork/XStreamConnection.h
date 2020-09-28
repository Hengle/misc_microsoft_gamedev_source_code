//==============================================================================
// XStreamConnection.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

#pragma once

//==============================================================================
// Includes

// xsystem
#include "poolable.h"

// xnetwork
#include "SerialBuffer.h"

// compression
#include "compressedStream.h"

// xcore
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"

enum
{
   cStreamHeaderVersion = 0,
   cStreamHeaderSize = 4
};

//==============================================================================
class BXTcpSendBuffer : public IPoolable
{
   public:

      BXTcpSendBuffer() : mpBuffer(NULL), mSize(0), mIndex(0), mCompress(false) {}
      ~BXTcpSendBuffer();

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BXTcpSendBuffer, 2);

      PVOID    mpBuffer;
      int32    mSize;
      int32    mIndex;
      bool     mCompress : 1;
};

//==============================================================================
class BXStreamConnection
{
   public:

      static const uint8 cStreamVersion = 0;

      enum eState
      {
         cStateIdle=0,
         cStateConnecting,
         cStateConnected,
         cStateDisconnected,
         //cStateReconnect,
         cStateError,
         cStateDone,
         cStateShutdown,
         cTotalStates
      };

      enum eError
      {
         cErrorSuccess=0,
         cErrorAPIFailure,
         cErrorTimedout,
         cErrorConnectionClosed,
         cErrorConnectionFailure,
         cTotalErrors
      };

      enum eFlags
      {
         eFlagNone = 0,
         eFlagUseHeader
      };

      BXStreamConnection(uint flags=0, uint timeout=0); //, uint reconnectAttempts=0);
      ~BXStreamConnection();

      // do not remove yourself as an observer when handling these methods
      class BXStreamObserver
      {
         public:
                                       BXStreamObserver();
            virtual                    ~BXStreamObserver();
            virtual void               observing(BXStreamConnection*);
            // observer callback to indicate that the stream is capable of handling
            // more data up to the given available size (in bytes)
            // * return true if observer will be using all the available space
            virtual void               sendReady(BXStreamConnection&);
            virtual void               connected(BXStreamConnection&);
            // if you don't specify the eFlagUseHeader, then you'll receive the raw data stream via this callback
            // it's up to either callback to copy off the memory otherwise it will be invalid after returning
            virtual void               dataReceived(const void* pData, int32 size);
            virtual void               dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);
            virtual void               disconnected(uint status);
            virtual void               shutdown();

            virtual uint8              getServiceID() const;

         protected:
            BXStreamConnection * getConnection();

         private:
            BXStreamConnection* mpConn;
      };

      bool hasObservers() const { return (mObserverList.getSize() > 0); }
      void addObserver(BXStreamObserver* pObserver);
      void removeObserver(BXStreamObserver* pObserver);

      bool isConnected() const { return mState == cStateConnected ? true : false; }   
      //bool isConnecting() const { return (mState == cStateConnecting || mState == cStateConnected || mState == cStateReconnect ? true : false); }
      bool isConnecting() const { return (mState == cStateConnecting || mState == cStateConnected ? true : false); }

      bool connect(const IN_ADDR& lspAddr, ushort port);
      //bool reconnect();

      bool send(const void* pData, const int32 size, bool compress=false);

      void service();
      void connected();
      bool initiateOverlappedSend();
      bool initiateOverlappedRecv();
      void processPayload(const char* pPayload, int32 payloadSize);
      bool emptyRecvBuffer();
      void shutdown(eState state, eError error, int result);
      void serviceSend();
      void sendNextBuffer();
      void serviceRecv();
      void close();

      void resetTimeout();

      bool isSending() const;
      bool isSendReady() const;

      const eState   getState() const { return mState; }
      const eError   getError() const { return mError; }

   private:

      void           cleanup();

      void           sendReady();

      void           setState(eState state, eError error=cErrorSuccess);
      void           setRetries(uint maxRetries, DWORD interval=0);

      BSmallDynamicSimArray<BXStreamObserver*> mObserverList;

      WSAOVERLAPPED mSendOverlapped;
      WSAOVERLAPPED mRecvOverlapped;

      WSAEVENT mEventHandle;

      BWin32Event mSendEvent;
      BWin32Event mRecvEvent;

      enum
      {
         cRecvBufferSize = 65535,
         cRecvBufferMinSize = 20000,
         cSendBufferSize = 65535,
         cMaxSendSize = 1200,
         cMaxPending = 20, // maximum number of pending sends before isSendReady returns false
      };

      struct BStreamSendHeader
      {
         int32 mCompress : 1;
         int32 mReserved : 3;
         int32 mVersion  : 4;
         int32 mSize     : 24;
         BStreamSendHeader() :
            mCompress(0),
            mReserved(0),
            mVersion(cStreamHeaderVersion),
            mSize(0)
            { }
      };

      struct BSendState
      {
         //
         // List of buffers waiting to be transmitted
         //
         BPointerList<BXTcpSendBuffer> mQueue;

         BXTcpSendBuffer*     mpCurrentBuffer;
         char*                mpBuffer;
         BStreamSendHeader*   mpSendHeader;
         PVOID                mpSendPayload;
         int                  mReadIndex;
         int                  mWriteIndex;
         int                  mSize;

         BSendState()
         {
            mQueue.setHeap(&gNetworkHeap);
            mpCurrentBuffer = NULL;
            mpBuffer = NULL;
            mpSendHeader = NULL;
            mpSendPayload = NULL;
            mReadIndex = 0;
            mWriteIndex = 0;
            mSize = 0;
         }

         ~BSendState()
         {
            BASSERT(mpCurrentBuffer == NULL);
            BASSERT(mQueue.getSize() == 0);
         }
      } mSend;

      //
      // Receive State
      //
      // We only ever have a single receive pending.
      //
      struct BRecvState
      {
         char* mpBuffer;
         uint  mWriteIndex;
         uint  mReadIndex;

         BRecvState()
         {
            mpBuffer = NULL;
            mWriteIndex = 0;
            mReadIndex = 0;
         }

         ~BRecvState()
         {
            BASSERT(mpBuffer == NULL);
         }
      } mRecv;

      BSerialBuffer        mRecvSerialBuffer;

      IN_ADDR              mAddr;

      SOCKET               mSocket;

      BDeflateStream*      mpDeflateStream;
      BInflateStream*      mpInflateStream;
      BDynamicStream*      mpDynamicStream;
      BByteStream*         mpByteStream;

      DWORD                mLastTime;
      DWORD                mRetryTime;
      DWORD                mRetryInterval;
      uint                 mNumRetries;
      uint                 mMaxRetries;

      uint                 mTimeout;
      uint                 mTimeoutValue;

      //uint                 mConnectionRetries;
      //uint                 mReconnectAttempts;

      eState               mState;
      eError               mError;

      ushort               mPort;

      bool                 mSentPending : 1;
      bool                 mUseHeader : 1;

}; // BXStreamConnection
