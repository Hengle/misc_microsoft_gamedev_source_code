//==============================================================================
// lspDataConnection.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================
#pragma once

// Includes
#include "common.h"

// xcore
#include "containers/dynamicArray.h"

// xnetwork
#include "Socket.h"
#include "UDP360Connection.h"

//==============================================================================
// 
//==============================================================================
class BLSPDataConnection : public IPoolable,
                           public BSocket::BObserver,
                           public BUDP360Connection::BConnObserver
{
   public:

      enum eState
      {
         cStateIdle=0,
         cStateGetXnAddr,
         cStateGetMachineId,
         cStateConnect,
         cStateConnecting,
         cStateConnected,
         cStateReconnect,
         cStateDisconnected,
         cStateTimedout,
         cTotalStates
      };

      // do not remove yourself as an observer when handling these methods
      class BLSPDataObserver
      {
         public:
            virtual bool               dataReceived(int32 serviceId, int32 type, int32 version, const void* pData, long size) { return false; }
            virtual void               disconnected(uint status) {}
            virtual void               shutdown() {}
      };

      bool hasObservers() const { return (mObserverList.getSize() > 0); }
      void addObserver(BLSPDataObserver* pObserver) { mObserverList.add(pObserver); }
      void removeObserver(BLSPDataObserver* pObserver) { mObserverList.remove(pObserver); } 

      BLSPDataConnection();
      ~BLSPDataConnection();

      // IPoolable
      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BLSPDataConnection, 2);

      HRESULT connect(const IN_ADDR& lspAddr, ushort port);

      // BSocket::BObserver interface that we implement
      void   recvd(IN BSocket* pSocket, IN const void * pBuffer, IN DWORD Length, IN DWORD voiceLength, IN CONST SOCKADDR_IN* pRemoteAddress);
      void   connected(IN BSocket* pSocket);
      void   interfaceAddressChanged();
      // return true from this method to drop the socket connection
      bool   notifyUnresponsiveSocket(IN BSocket* pSocket, DWORD lastRecvTime);
      bool   notifyResponsiveSocket(IN BSocket* pSocket, DWORD lastRecvTime);
      void   disconnected(IN BSocket* pSocket, IN DWORD Status);

      // BUDP360Connection::BConnObserver interface that we implement
      void   dataReceived(IN BUDP360Connection* Connection, IN const void * Buffer, IN LONG Length, IN CONST SOCKADDR_IN* RemoteAddress);
      void   disconnected(IN BUDP360Connection* Connection);
      bool   notifyUnresponsiveConnection(IN BUDP360Connection* Connection, DWORD lastRecvTime);
      bool   notifyResponsiveConnection(IN BUDP360Connection* Connection, DWORD lastRecvTime);
      void   connected(IN BUDP360Connection* Connection);
      void   connectTimeout(IN BUDP360Connection* connection);

      //bool           isFinished() const { return (mState == cStateDone); }
      bool           enumerate();
      bool           connect();
      void           disconnect();
      void           reconnect();
      void           uploadFile();
      void           service();

      eState getState() const { return mState; }
      uint64 getNonce() const { return mNonce; }
      BUDP360Socket* getSocket() const { return mpSocket; }
      BUDP360Connection* getConn() const { return mpConn; }

   private:

      void           setState(eState state);
      void           setRetries(uint maxRetries, DWORD interval=0);
      void           setRetryInterval(DWORD interval);

      uchar                mSendBuffer[cMaxSendSize];

      BSmallDynamicSimArray<BLSPDataObserver*> mObserverList;

      XNADDR               mXnAddr;

      IN_ADDR              mServerAddr;    // Secure title server address
      uint64               mNonce;

      ULONGLONG            mMachineId;

      BUDP360Connection*   mpConn;
      BUDP360Socket*       mpSocket;

      DWORD                mXNetGetTitleXnAddrFlags;

      DWORD                mLastTime;
      DWORD                mRetryTime;
      DWORD                mRetryInterval;
      uint                 mNumRetries;
      uint                 mMaxRetries;

      eState               mState;

      ushort               mPort;

      bool                 mEnabled : 1;
};
