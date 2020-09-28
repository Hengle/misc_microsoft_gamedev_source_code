//==============================================================================
// lspMediaTransfer.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "lspManager.h"
#include "lspTitleServerConnection.h"

// xnetwork
#include "XStreamConnection.h"

//==============================================================================
// 
//==============================================================================
class ILSPMediaTask
{
   public:
      virtual void mediaTaskRelease(uint taskID, uint32 result) { taskID; result; BASSERTM(0, "Not Implemented"); }
      virtual void mediaListResponse(uint taskID, uint32 result, uint crc, uint ttl, const void* pData, int32 size) { taskID; result; crc; ttl; pData; size; BASSERTM(0, "Not Implemented"); }
      virtual void mediaDownloadResponse(uint taskID, uint32 result) { taskID; result; BASSERTM(0, "Not Implemented"); }
      virtual void mediaUploadResponse(uint taskID, uint32 result) { taskID; result; BASSERTM(0, "Not Implemented"); }
      virtual void mediaDeleteResponse(uint taskID, uint32 result) { taskID; result; BASSERTM(0, "Not Implemented"); }
};

//==============================================================================
// 
//==============================================================================
class BLSPMediaFriendsEnum
{
   public:
      enum
      {
         cMaxFriends = MAX_FRIENDS,
      };

      BLSPMediaFriendsEnum(uint userIndex);
      ~BLSPMediaFriendsEnum();

      bool isComplete();
      uint32 getLastError() const { return mLastError; }

      uint32 getNumFriends();

      const PXONLINE_FRIEND getFriend(uint i);

      BDynamicStream* getHeaderStream();

      XOVERLAPPED       mOverlapped;
      HANDLE            mEnumHandle;
      uint              mUserIndex;
      uchar*            mpBuffer;
      BDynamicStream*   mpHeaderStream;
      DWORD             mBufferSize;
      DWORD             mNumItems;
      DWORD             mLastError;
};

//==============================================================================
// Commands are queued up and executed in order and then responses from the
//    server are matched up to the requests based on an ID
//==============================================================================
class BLSPMediaTask : public IPoolable
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

      enum BMediaCommand
      {
         cCommandNone = 0,
         cCommandList,
         cCommandDownload,
         cCommandUpload,
         cCommandDelete,
         cCommandListFriends,
      };

      BLSPMediaTask() : mpContent(NULL) {}
      ~BLSPMediaTask() {}

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BLSPMediaTask, 2);

      uint init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint crc=0);
      uint init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint64 mediaID);
      uint init(BMediaCommand command, uint timeout, uint userIndex, XUID xuid, ILSPMediaTask* pCallback, uint64 mediaID, const XCONTENT_DATA& data, BDynamicStream* pStream);

      void restart();

      BStream* getStream();
      BStream* getTempStream();

      XUID getXuid() const { return mXuid; }
      uint64 getMediaID() const { return mMediaID; }
      uint getUserIndex() const { return mUserIndex; }
      uint getID() const { return mID; }

      uint getCRC() const { return mCRC; }

      uint getTimeout() const { return mTimeout; }

      uint getTransferred() const { return mTransferred; }

      void updateTransferred(uint amount) { mTransferred += amount; }

      uint64 getStreamID() const { return mStreamID; }
      void setStreamID(uint64 streamID) { mStreamID = streamID; }

      const PXCONTENT_DATA getContent() const { return mpContent; }
      const BDynamicStream* getHeaderStream() const { return (mpFriendsEnum != NULL ? mpFriendsEnum->getHeaderStream() : mpHeaderStream); }

      BMediaCommand getCommand() const { return mCommand; }

      BLSPMediaFriendsEnum* getFriendsEnum();

      static uint mNextID;

      XUID                    mXuid; // XUID of requesting user
      uint64                  mMediaID;
      uint64                  mStreamID;
      PXCONTENT_DATA          mpContent;
      ILSPMediaTask*          mpCallback;
      BStream*                mpFileStream;
      BDynamicStream*         mpHeaderStream;
      BLSPMediaFriendsEnum*   mpFriendsEnum;
      uint                    mUserIndex; // controller of requesting user
      uint                    mID;
      uint                    mRequestID;
      uint                    mCRC;
      uint                    mTimeout;
      uint                    mTransferred;
      BMediaCommand           mCommand;
      eState                  mState;
};

//==============================================================================
// 
//==============================================================================
class BLSPMediaTransfer : public BLSPTitleServerConnection
{
   public:
      enum eState
      {
         cStateIdle=0,
         cStateInit,
         //cStateConnect,
         cStateConnecting,
         //cStateConnected,
         cStateReconnect,
         cStateDisconnected,
         cStateSelectCommand,
         cStatePreRequest, // used to inject additional work before we actually issue a request to the server, i.e. enumerate friends
         cStateRequest,
         //cStateSend,
         //cStateSendPending,
         //cStateSendFailed,
         cStateServerPending,
         //cStateRecv,
         cStateTimedout,
         cStateDone,
         cStateStop,
         cTotalStates
      };

      BLSPMediaTransfer();
      ~BLSPMediaTransfer();

      // BXStreamConnection::BXStreamObserver
      virtual void   sendReady(BXStreamConnection& conn);
      virtual void   connected(BXStreamConnection&);
      virtual void   dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size);
      virtual void   disconnected(uint status);
      virtual void   shutdown();

      bool           isFinished() const { return (mState == cStateDone); }
      bool           isIdle() const { return (mState == cStateIdle); }
      bool           isStopped() const { return (mState == cStateStop); }

      bool           isUploading() const { return (mpCurrentTask != NULL); }

      uint           numTransfersInProgress() const { return mTasks.size(); }
      uint           percentComplete() const { return mPercent; }
      uint           currentID() const { return (mpCurrentTask != NULL ? mpCurrentTask->getID() : 0); }

      BLSPMediaTask* getTaskByID(uint taskID);

      BLSPMediaTask* createTask();
      void           terminateTask(BLSPMediaTask* pTask);

      //void           connect();
      //void           disconnect();
      void           reconnect();
      void           start();
      void           stop();
      void           reset();
      void           service();

   private:

      void           selectCommand();
      void           cleanup();
      void           setState(eState state);
      void           setRetries(uint maxRetries, DWORD interval=0);
      void           setRetryInterval(DWORD interval);

      enum
      {
         //cSendBufferSize = 1280,
         cFileBufferSize = 1024
      };

      //uchar                mSendBuffer[cSendBufferSize];
      uchar                mFileBuffer[cFileBufferSize];

      BSmallDynamicSimArray<BLSPMediaTask*> mTasks;

      //BLSPOVERLAPPED          mOverlapped;
      //IN_ADDR                 mServerAddr;    // Secure title server address

      BLSPMediaTask*          mpCurrentTask;

      //BXStreamConnection*     mpConn;

      DWORD                   mLastTime;
      DWORD                   mRetryTime;
      DWORD                   mRetryInterval;
      uint                    mNumRetries;
      uint                    mMaxRetries;
      uint                    mPercent;
      eState                  mState;
      //uint16                  mPort;
      bool                    mStopped : 1;
      bool                    mWaitingOnResponse : 1;
      //bool                    mUploading : 1;
      bool                    mEnabled : 1;
      bool                    mCompress : 1;
};
