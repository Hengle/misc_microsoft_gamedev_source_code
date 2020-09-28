//==============================================================================
// lspConnection.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================
#pragma once

// Includes
#include "common.h"

//==============================================================================
// 
//==============================================================================
class BLSPConnection
{
   public:

      enum eState
      {
         cStateIdle=0,
         cStatePendingSignin,
         cStateVerifySignin,
         cStateReconnect,
         cStateDisconnected,
         cStateXNetEnumerate,
         cStateXNetEnumeratePending,
         cStateXNetEnumerateFailed,
         cStateXNetPickServer,
         cStateXNetConnect,
         cStateXNetConnecting,
         cStateXNetConnected,
         cStateError,
         cStateDone,
         cStateShutdown,
         cTotalStates
      };

      enum eError
      {
         cErrorSuccess=0,
         cErrorNoServers,
         cErrorXNetConnect,
         //cErrorNoPermissions,
         cErrorNotSignedIn,
         //cErrorAPIFailure,
         cErrorTimedout,
         cTotalErrors
      };

                     BLSPConnection();
                     ~BLSPConnection();

      void           init();
      void           shutdown();

      void           resetTimeout();

      void           service();

      const eState   getState() const { return mState; }
      const eError   getError() const { return mError; }
      const IN_ADDR& getAddr() const { return mServerAddr; }

   private:

      bool           xNetEnumerate();
      void           xNetCheckEnumerate();
      void           xNetPickServer();

      bool           xNetConnect();
      void           xNetCheckConnect();

      void           disconnect();
      void           reconnect();

      void           setState(eState state, eError error=cErrorSuccess);
      void           setRetries(uint maxRetries, DWORD interval=0);
      //bool           verifySignIn();

      XOVERLAPPED          mOverlapped;
      IN_ADDR              mServerAddr;    // Secure title server address

      PXOVERLAPPED         mpOverlapped;
      PXTITLE_SERVER_INFO  mpServerInfo;
      HANDLE               mServerEnumHandle;
      DWORD                mServiceID;
      DWORD                mNumServerInfo;
      DWORD                mEnumItemCount;

      uint                 mEnumPick;
      uint                 mConnectAttempts;
      uint                 mConnectMaxRetries;

      DWORD                mLastTime;
      DWORD                mRetryTime;
      DWORD                mRetryInterval;
      uint                 mNumRetries;
      uint                 mMaxRetries;

      eState               mState;
      eError               mError;

      //bool                 mSignInRequested : 1;
};
