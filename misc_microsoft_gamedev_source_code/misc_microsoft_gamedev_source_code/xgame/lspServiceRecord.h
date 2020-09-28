//==============================================================================
// lspServiceRecord.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "lspManager.h"
#include "lspTitleServerConnection.h"

// xnetwork
#include "XStreamConnection.h"

//==============================================================================
// Commands are queued up and executed in order and then responses from the
//    server are matched up to the requests based on an ID
//==============================================================================
class BLSPServiceRecordTask
{
   public:

      enum eState
      {
         cStateIdle = 0,
         cStatePending,
         cStateInProgress,
         cStateServerPending,
         cStateDelete
      };

      enum BServiceRecordCommand
      {
         cCommandNone = 0,
         cCommandQuery,
      };

      BLSPServiceRecordTask();
      ~BLSPServiceRecordTask() {}

      uint init(BLSPServiceRecordCache* pServiceRecordCache, BServiceRecordCommand command, uint timeout, XUID xuid);

      void queryResponse(const void* pData, int32 size, uint ttl);

      XUID getXuid() const { return mXuid; }
      uint getID() const { return mID; }

      uint getTimeout() const { return mTimeout; }

      BServiceRecordCommand getCommand() const { return mCommand; }

      uint getRequestID() const { return mRequestID; }
      void setRequestID(uint requestID) { mRequestID = requestID; }

      eState getState() const { return mState; }
      void setState(eState state) { mState = state; }

   private:

      void queryResponse(BXMLReader& reader, uint ttl);
      void queryResponse(uint level, uint ttl);

      static uint mNextID;

      XUID                    mXuid; // XUID of requesting user
      BLSPServiceRecordCache* mpServiceRecordCache;
      uint                    mID;
      uint                    mRequestID;
      uint                    mTimeout;
      BServiceRecordCommand   mCommand;
      eState                  mState;
};

//==============================================================================
// 
//==============================================================================
class BLSPServiceRecord : public BLSPTitleServerConnection
{
   public:
      enum eState
      {
         cStateIdle=0,
         cStateInit,
         cStateConnecting,
         cStateDisconnected,
         cStateSelectCommand,
         cStateRequest,
         cStateServerPending,
         cStateTimedout,
         cStateDone,
         cStateStop,
         cTotalStates
      };

      BLSPServiceRecord(BLSPServiceRecordCache* pServiceRecordCache);
      ~BLSPServiceRecord();

      // BXStreamConnection::BXStreamObserver
      virtual void   sendReady(BXStreamConnection& conn);
      virtual void   connected(BXStreamConnection&);
      virtual void   dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);
      virtual void   disconnected(uint status);
      virtual void   shutdown();

      bool           isFinished() const { return (mState == cStateDone); }
      bool           isIdle() const { return (mState == cStateIdle); }
      bool           isStopped() const { return (mState == cStateStop); }

      void           query(XUID xuid);

      void           start();
      void           stop();
      void           reset();
      void           service();

      void           setState(eState state);

   private:

      void           selectCommand();
      void           cleanup();
      void           setRetries(uint maxRetries, DWORD interval=0);
      void           setRetryInterval(DWORD interval);

      BSmallDynamicSimArray<BLSPServiceRecordTask*> mTasks;

      BLSPServiceRecordTask*  mpCurrentTask;

      BLSPServiceRecordCache* mpServiceRecordCache;

      uint                    mDefaultTTL;
      DWORD                   mLastTime;
      DWORD                   mRetryTime;
      DWORD                   mRetryInterval;
      uint                    mNumRetries;
      uint                    mMaxRetries;
      eState                  mState;
      bool                    mStopped : 1;
      bool                    mWaitingOnResponse : 1;
      bool                    mEnabled : 1;
};
