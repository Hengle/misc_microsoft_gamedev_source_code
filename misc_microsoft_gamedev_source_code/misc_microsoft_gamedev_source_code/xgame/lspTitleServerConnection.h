//==============================================================================
// lspTitleConnection.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma  once

#include "XStreamConnection.h"

class BLSPPacket;

//==============================================================================
// 
//==============================================================================
class BLSPTitleServerConnection : public BXStreamConnection::BXStreamObserver
{
   public:
      enum eState
      {
         cStateIdle=0,
         cStateConnect,
         //cStateConnecting,
         cStateConnected,
         cStateDisconnected,
         cStateRequest,
         cStateTimedout,
         cStatePendingDone,
         cStateDone,
         cTotalStates
      };

      explicit BLSPTitleServerConnection(uint16 port, const long portOverrideConfigId, uint8 serviceID, const long serviceIDOverrideConfigID, const uint timeout=30000);
      virtual ~BLSPTitleServerConnection() = 0;

      // override BXStreamConnection::BLSPDataObserver notifications
      virtual void   observing(BXStreamConnection*);
      virtual void   connected(BXStreamConnection&);
      virtual void   dataReceived(uint8 serviceID, int32 type, int32 version, const void* pData, int32 size);
      virtual void   disconnected(uint status);
      virtual void   shutdown();
      virtual uint8  getServiceID() const;

      bool           isFinished();

      uint16         getPort() const;

      void           setState(eState state);
      const eState   getState() const;

      void           connect();
      bool           reconnect();

      void           resetTimeout();

   protected:
      // allow derived classes to update this in the 
      // constructor
      //void setPort(uint16 port);
      void send(BLSPPacket& packet, bool compress=false);
      void disconnect();

   private:
      BLSPTitleServerConnection();
      BLSPTitleServerConnection(const BLSPTitleServerConnection&);
      BLSPTitleServerConnection& operator= (const BLSPTitleServerConnection&);

   private:
      BDynamicArray<BSerialBuffer> mPendingSends;

      eState mState;
      uint   mTimeoutValue;
      uint   mTimeout;
      uint16 mPort;
      uint8  mServiceID;
      bool   mCompress : 1;
};
