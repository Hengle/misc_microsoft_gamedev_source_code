//==============================================================================
// lspManager.h
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================
#pragma once

// Includes
#include "common.h"
#include "NetPackets.h"

#include "lspPorts.h"
#include "lspCache.h"

// xcore
#include "containers/skiplist.h"

class BDynamicStream;

//class BLSPDataConnection;
//class BUDP360Connection;
//class BUDP360Socket;

class BXStreamConnection;
class BLSPTitleServerConnection;

class BLSPFileUpload;
class BLSPAuth;
class BLSPManager;
class BLSPConnection;
class BLSPConfigData;
class BLSPPerfLog;
class BLSPMediaTask;
class BLSPMediaTransfer;
class BLSPTicker;
class BUITicker;
class BLSPServiceRecord;

class BMatchMakingHopperList;

extern BLSPManager gLSPManager;

//==============================================================================
//typedef struct _BLSPOVERLAPPED {
//   HANDLE                  hEvent;
//   // TCP
//   //BXStreamConnection*     pXStreamConn;      // xbox specific TCP connection
//   // UDP
//   //BLSPDataConnection*     pLspDataConn;      // lsp abstract for udp connection
//   //BUDP360Connection*      pXDataConn;        // xbox specific UDP connection
//   DWORD                   dwExtendedError;
//} BLSPOVERLAPPED, *PBLSPOVERLAPPED;

//==============================================================================
enum eProtocol
{
   cTCP = 0,
   //cUDP
};

typedef enum
{
   cYes = 0,
   cNo,
   cPending,
   cFailed, // failed to initiate a request to the LSP
} BLSPResponse;

//==============================================================================
// 
//==============================================================================
class BLSPConnectionHandler : public IPoolable
{
   public:
      BLSPConnectionHandler() :
         mpXStreamConn(NULL),
         //mpTitleServerConnection(NULL),
         //mpLspDataConn(NULL),
         //mpXDataConn(NULL),
         //mpXDataSocket(NULL),
         mTimeout(0),
         mFlags(0),
         mProtocol(cTCP),
         mPort(1000),
         mPendingConnect(false) {}
      ~BLSPConnectionHandler() {}

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BLSPConnectionHandler, 2);

      void init(eProtocol protocol, ushort port, uint flags);

      //BSmallDynamicSimArray<PBLSPOVERLAPPED> mOverlapped;
      BSmallDynamicSimArray<BLSPTitleServerConnection*> mServices;

      // TCP
      BXStreamConnection*         mpXStreamConn;
      //BLSPTitleServerConnection*  mpTitleServerConnection;

      // UDP
      //BLSPDataConnection*     mpLspDataConn;
      //BUDP360Connection*      mpXDataConn;
      //BUDP360Socket*          mpXDataSocket;

      DWORD                   mTimeout;

      uint                    mFlags; // flags passed to the connection on init

      eProtocol               mProtocol;
      ushort                  mPort;

      bool                    mPendingConnect : 1;
};

//==============================================================================
// 
//==============================================================================
class BLSPManager
{
   public:

      enum eState
      {
         cStateIdle=0,
         cStateLSPPending,
         cStateLSPConnected,
         cStateLSPDisconnect,
         cStateDataPending,
         cStateTimedout,
         cTotalStates
      };

                              BLSPManager();
                              ~BLSPManager();

      bool                    setup();       // initialize the manager
      void                    shutdown();    // shutdown the manager and free all memory

      void                    update();

      void                    resetTimeout();

      //DWORD                   connect(eProtocol protocol, ushort port, PBLSPOVERLAPPED pOverlapped, uint flags, BLSPTitleServerConnection* pTitleServerConnection);
      DWORD                   connect(eProtocol protocol, ushort port, uint flags, BLSPTitleServerConnection* pTitleServerConnection);
      //void                    cancel(eProtocol protocol, ushort port, PBLSPOVERLAPPED pOverlapped);
      void                    cancel(eProtocol protocol, ushort port, BLSPTitleServerConnection* pTitleServerConnection);

      BLSPConnectionHandler*  getConnection(eProtocol protocol, ushort port);

      void                    uploadFile(XUID xuid, const BSimString& gamerTag, const BSimString& filename, BStream* pStream);

      // XXX debug method only
      void                    checkAuth();

      BLSPResponse            isBanned(XUID xuid);
      BLSPResponse            isBannedMedia(XUID xuid);
      BLSPResponse            isBannedMatchMaking(XUID xuid);

      bool                    isAuthorizing() const { return mAuthorizationRunning; }

      void                    requestConfigData(BMatchMakingHopperList* pHopperList);
      bool                    requestConfigDataRunning() const { return mConfigDataRequestRunning; }
      bool                    isConfigDataCurrent();

      void                    postPerfData(BPerfLogPacketV2* pDataPacket);

      BLSPMediaTask*          getMediaTaskByID(uint taskID);
      BLSPMediaTask*          createMediaTask();
      void                    terminateMediaTask(BLSPMediaTask* pTask);

      void                    updateServiceRecord(XUID xuid);
      uint                    queryServiceRecord(XUID xuid);

      bool                    isInNoLSPMode();
      void                    requestTickerData(BUITicker *);
      void                    removeTicker();

   private:

      BLSPMediaTransfer*      getMediaTransfer();

      DWORD                   connect(BLSPConnectionHandler* pHandler);

      void                    completeConnect(BLSPConnectionHandler* pHandler);
      void                    completeStreamConnect(BLSPConnectionHandler* pHandler);
      //void                    completeDataConnect(BLSPConnectionHandler* pHandler);
      void                    completeConnect(BLSPConnectionHandler* pHandler, DWORD dwResult);

      void                    shutdownServices(bool goIdle=false);

      bool                    sgFailure() const;
      bool                    liveStatusChanged() const;

      bool                    initAuthRequest();
      BLSPResponse            initAuthRequest(XUID xuid, PBLSPAuthUser* ppUser);

      void                    setState(eState state);
      void                    setRetries(uint maxRetries, DWORD interval=0);
      void                    setRetryInterval(DWORD interval);

      BSmallDynamicSimArray<BLSPConnectionHandler*> mConnections;
      BSmallDynamicSimArray<BLSPFileUpload*>        mLSPFileUpload;

      BLSPAuthCache                                 mAuthCache;
      BLSPConfigCache                               mConfigCache;
      BLSPServiceRecordCache                        mServiceRecordCache;

      BLSPConnection*                               mpLSPConn;

      BLSPFileUpload*                               mpLSPFileUpload;
      BLSPAuth*                                     mpLSPAuth;
      BLSPConfigData*                               mpLSPConfigData;
      BLSPPerfLog*                                  mpLSPPerfLog;
      BLSPMediaTransfer*                            mpLSPMediaTransfer;
      BLSPTicker*                                   mpLSPTicker;
      BLSPServiceRecord*                            mpLSPServiceRecord;

      DWORD                                         mLastTime;
      DWORD                                         mRetryTime;
      DWORD                                         mRetryInterval;
      uint                                          mNumRetries;
      uint                                          mMaxRetries;

      uint                                          mLastSGAttempt;
      uint                                          mSGAttemptTTL;

      uint                                          mTCPTimeout;
      uint                                          mSGTimeout;

      //uint                                          mTCPReconnectAttempts;

      uint                                          mLIVEConnectionChangeTime;

      uint                                          mConfigFailureTime;

      uint                                          mLSPFailTTL;

      eState                                        mState;

      bool                                          mNoLSPMode : 1;
      bool                                          mAuthorizationRunning : 1;
      bool                                          mConfigDataRequestRunning : 1;
      bool                                          mAuthRequestSucceeded : 1;

      // marked as true when we have a failure in any one of these systems
      bool                                          mXEnumerateFailed : 1;
      bool                                          mXNetConnectFailed : 1;
      bool                                          mTCPConnectFailed : 1;
};
