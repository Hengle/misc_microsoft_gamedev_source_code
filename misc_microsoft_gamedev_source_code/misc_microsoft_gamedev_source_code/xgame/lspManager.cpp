//==============================================================================
// lspManager.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

// Includes
#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "notification.h"

#include "lspManager.h"
#include "lspConnection.h"
//#include "lspDataConnection.h"
#include "lspFileUpload.h"
#include "lspAuth.h"
#include "lspConfigData.h"
#include "lspPerfLog.h"
#include "lspMediaTransfer.h"
#include "lspTicker.h"
#include "lspServiceRecord.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

// xcore
#include "stream\dynamicStream.h"

// Globals
BLSPManager gLSPManager;

IMPLEMENT_FREELIST(BLSPConnectionHandler, 2, &gSimHeap);

//==============================================================================
// 
//==============================================================================
void BLSPConnectionHandler::onAcquire()
{
   //if (mpLspDataConn)
   //   BLSPDataConnection::releaseInstance(mpLspDataConn);

   //mOverlapped.clear();
   mServices.clear();

   // UDP
   //mpLspDataConn = NULL;
   //mpXDataConn = NULL;
   //mpXDataSocket = NULL;

   // TCP
   mpXStreamConn = NULL;

   mTimeout = timeGetTime() + cDefaultLSPSGTimeout;

   mFlags = 0;
   mProtocol = cTCP;
   mPort = 0;

   mPendingConnect = true;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnectionHandler::onRelease()
{
   //mpTitleServerConnection = 0;
   //if (mpLspDataConn)
   //   BLSPDataConnection::releaseInstance(mpLspDataConn);

   delete mpXStreamConn;

   //uint count = mOverlapped.getSize();
   //for (uint i=0; i < count; ++i)
   //{
   //   PBLSPOVERLAPPED pOverlapped = mOverlapped[i];
   //   if (!pOverlapped)
   //      continue;

   //   pOverlapped->dwExtendedError = ERROR_INVALID_HANDLE;

   //   if (pOverlapped->hEvent != INVALID_HANDLE_VALUE)
   //      SetEvent(pOverlapped->hEvent);

   //   //pOverlapped->pLspDataConn = NULL;
   //   //pOverlapped->pXDataConn = NULL;

   //   //pOverlapped->pXStreamConn = NULL;
   //}

   uint count = mServices.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPTitleServerConnection* pService = mServices[i];
      if (!pService)
         continue;

      pService->disconnected(0);
   }

   mServices.clear();

   //mpLspDataConn = NULL;
   //mpXDataConn = NULL;
   //mpXDataSocket = NULL;

   mpXStreamConn = NULL;

   mTimeout = 0;
   mFlags = 0;
   mPort = 0;

   mPendingConnect = false;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnectionHandler::init(eProtocol protocol, ushort port, uint flags)
{
   mProtocol = protocol;
   mPort = port;
   mFlags = flags;
   //mpTitleServerConnection = titleServerConnection;
}

//==============================================================================
// 
//==============================================================================
BLSPManager::BLSPManager() :
   mpLSPConn(NULL),
   mpLSPFileUpload(NULL),
   mpLSPAuth(NULL),
   mpLSPConfigData(NULL),
   mpLSPPerfLog(NULL),
   mpLSPMediaTransfer(NULL),
   mpLSPTicker(NULL),
   mpLSPServiceRecord(NULL),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mLastSGAttempt(0),
   mSGAttemptTTL(cDefaultLSPSGTTL),
   mTCPTimeout(cDefaultLSPTCPTimeout),
   mSGTimeout(cDefaultLSPSGTimeout),
   //mTCPReconnectAttempts(cDefaultLSPTCPReconnects),
   mLIVEConnectionChangeTime(0),
   mConfigFailureTime(0),
   mLSPFailTTL(cDefaultLSPFailTTL),
   mState(cStateIdle),
   mNoLSPMode(false),
   mAuthorizationRunning(false),
   mConfigDataRequestRunning(false),
   mAuthRequestSucceeded(true),
   mXEnumerateFailed(false),
   mXNetConnectFailed(false),
   mTCPConnectFailed(false)
{
}

//==============================================================================
// 
//==============================================================================
BLSPManager::~BLSPManager()
{
   for (uint i=0; i < mLSPFileUpload.getSize(); ++i)
      delete mLSPFileUpload[i];
   mLSPFileUpload.clear();
   delete mpLSPFileUpload;
   delete mpLSPAuth;
   delete mpLSPConfigData;
   delete mpLSPPerfLog;
   delete mpLSPMediaTransfer;
   delete mpLSPTicker;
   delete mpLSPServiceRecord;
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::setup()
{
   long timeout;
   if (gConfig.get(cConfigLSPDefaultTCPTimeout, &timeout))
      mTCPTimeout = static_cast<uint>(timeout);

   if (gConfig.get(cConfigLSPDefaultSGTimeout, &timeout))
      mSGTimeout = static_cast<uint>(timeout);

   //if (gConfig.get(cConfigLSPDefaultTCPReconnects, &timeout))
   //   mTCPReconnectAttempts = static_cast<uint>(timeout);

   if (gConfig.get(cConfigLSPDefaultSGTTL, &timeout))
      mSGAttemptTTL = static_cast<uint>(timeout);

   if (gConfig.get(cConfigLSPDefaultSGFailTTL, &timeout))
      mLSPFailTTL = static_cast<uint>(timeout);

   mConfigCache.init();

   // from this point forward, we'll track LIVE connection changes
   mLIVEConnectionChangeTime = timeGetTime();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::shutdown()
{
   shutdownServices(true);

   for (uint i=0; i < mLSPFileUpload.getSize(); ++i)
      delete mLSPFileUpload[i];
   mLSPFileUpload.clear();

   delete mpLSPFileUpload;
   mpLSPFileUpload = NULL;

   delete mpLSPAuth;
   mpLSPAuth = NULL;
   mAuthorizationRunning = false;

   delete mpLSPConfigData;
   mpLSPConfigData = NULL;
   mConfigDataRequestRunning = false;

   delete mpLSPPerfLog;
   mpLSPPerfLog = NULL;

   delete mpLSPConn;
   mpLSPConn = NULL;

   delete mpLSPMediaTransfer;
   mpLSPMediaTransfer = NULL;

   delete mpLSPTicker;
   mpLSPTicker = NULL;

   delete mpLSPServiceRecord;
   mpLSPServiceRecord = NULL;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::update()
{
   SCOPEDSAMPLE(BLSPManager_update);

   // all we're looking for here is whether the Xbox LIVE connection has changed
   switch (gNotification.getNotification())
   {
      case XN_LIVE_CONNECTIONCHANGED:
         {
            if (gNotification.getParam() == XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
               mLIVEConnectionChangeTime = timeGetTime();
            break;
         }
      case XN_LIVE_LINK_STATE_CHANGED:
         {
            if (gNotification.getParam() == TRUE)
               mLIVEConnectionChangeTime = timeGetTime();
            break;
         }
      case XN_SYS_SIGNINCHANGED:
         {
            if (gUserManager.getPrimaryUser() && gUserManager.getPrimaryUser()->isSignedIntoLive())
               mLIVEConnectionChangeTime = timeGetTime();
            break;
         }
      default:
         break;
   }

   if (mpLSPConn && mState != cStateLSPDisconnect)
   {
      mpLSPConn->service();
      switch (mpLSPConn->getState())
      {
         case BLSPConnection::cStateShutdown:
         case BLSPConnection::cStateDisconnected:
            {
               // if the lsp connection was shutdown or disconnected
               // XXX eventually a disconnected state will be handled
               // by the LSP connection code to attempt to re-establish
               shutdownServices();
            }
            break;
         case BLSPConnection::cStateError:
            {
               mNoLSPMode = true;
               shutdownServices();

               switch (mpLSPConn->getError())
               {
                  case BLSPConnection::cErrorNoServers:
                     {
                        mXEnumerateFailed = true;
                        break;
                     }
                  case BLSPConnection::cErrorXNetConnect:
                     {
                        mXNetConnectFailed = true;
                        break;
                     }
                  case BLSPConnection::cErrorTimedout:
                     {
                        mXEnumerateFailed = true;
                        mXNetConnectFailed = true;
                        break;
                     }
               }
            }
            break;
      }

      if (mConnections.getSize() == 0)
         setState(cStateLSPDisconnect);
   }

   if (mpLSPFileUpload)
   {
      if (mpLSPFileUpload->isFinished())
      {
         delete mpLSPFileUpload;
         mpLSPFileUpload = NULL;

         if (mLSPFileUpload.getSize() > 0)
         {
            mpLSPFileUpload = mLSPFileUpload[0];
            mLSPFileUpload.removeIndex(0);
            mpLSPFileUpload->init();
         }
      }
   }

   if (mpLSPAuth)
   {
      if (mpLSPAuth->isFinished())
      {
         mAuthorizationRunning = false;
         mAuthRequestSucceeded = mpLSPAuth->requestSucceeded();
         delete mpLSPAuth;
         mpLSPAuth = NULL;
      }
   }

   if (mpLSPConfigData)
   {
      if (mpLSPConfigData->isFinished())
      {
         if (sgFailure())
            mConfigFailureTime = timeGetTime();

         mConfigDataRequestRunning = false;
         delete mpLSPConfigData;
         mpLSPConfigData = NULL;
      }
   }

   if (mpLSPPerfLog)
   {
      if (mpLSPPerfLog->isFinished())
      {
         delete mpLSPPerfLog;
         mpLSPPerfLog = NULL;
      }
   }

   if (mpLSPMediaTransfer)
      mpLSPMediaTransfer->service();

   if (mpLSPServiceRecord)
   {
      mpLSPServiceRecord->service();
      if (mpLSPServiceRecord->isFinished())
      {
         delete mpLSPServiceRecord;
         mpLSPServiceRecord = NULL;
      }
   }

   switch (mState)
   {
      case cStateLSPPending:
         {
            if (timeGetTime() > mRetryTime)
            {
               if (mpLSPConn && mpLSPConn->getState() == BLSPConnection::cStateXNetConnected)
               {
                  setState(cStateLSPConnected);
               }
               else
               {
                  mRetryTime = timeGetTime() + mRetryInterval;
                  if (++mNumRetries > mMaxRetries)
                     setState(cStateTimedout);
               }
            }
         }
         break;

      case cStateLSPConnected:
         {
            // now that we're connected to the lsp, we can initate the pending udp connections
            uint count = mConnections.getSize();
            for (uint i=0; i < count; ++i)
            {
               BLSPConnectionHandler* pTemp = mConnections[i];
               if (!pTemp)
                  continue;
               if ((pTemp->mServices.getSize() > 0 || pTemp->mPendingConnect) && connect(pTemp) == ERROR_SUCCESS)
               {
                  // now that we're connected successfully, clear out our pending operations list
                  //pTemp->mOverlapped.clear();
                  // actually, now that we've successfully performed our SG connection, we need to 
                  // secure our data connection, this involves a hello/response sequence to insure
                  // that the title server is running
                  // when that completes, we can inform all the pending overlapped IO of the success/failure
                  setState(cStateDataPending);
               }
            }
            // only go idle if we have no pending data connections
            if (mState != cStateDataPending)
               setState(cStateIdle);
         }
         break;

      case cStateDataPending:
         {
            if (timeGetTime() > mRetryTime)
            {
               //bool pending = false;

               uint count = mConnections.getSize();
               for (uint i=0; i < count; ++i)
               {
                  BLSPConnectionHandler* pTemp = mConnections[i];
                  if (!pTemp)
                     continue;
                  // if I have a LSP Connection and we have pending IO requests, then attempt to complete them now
                  if (pTemp->mServices.getSize() > 0)
                  {
                     //if (pTemp->mpLspDataConn || pTemp->mpXStreamConn)
                     if (pTemp->mpXStreamConn)
                     {
                        completeConnect(pTemp);
                        //mRetryTime = timeGetTime() + mRetryInterval;
                        //pending = true;
                     }
                  }
               }

               //if (!pending)
               setState(cStateIdle);
            }
         }
         break;

      case cStateLSPDisconnect:
         {
            if (timeGetTime() > mRetryTime)
            {
               if (mConnections.getSize() == 0)
               {
                  delete mpLSPConn;
                  mpLSPConn = NULL;
                  setState(cStateIdle);
               }
               else
               {
                  mRetryTime = timeGetTime() + mRetryInterval;
                  if (++mNumRetries > mMaxRetries)
                     shutdownServices(true);
               }
            }
         }
         break;
   }

   for (int32 i=mConnections.getSize()-1; i >= 0; --i)
   {
      BLSPConnectionHandler* pTemp = mConnections[i];
      if (!pTemp)
         continue;

      // for now, we'll take on the job of servicing the TCP connections
      // we'll eventually move the servicing to another thread and provide
      // a send/receive queue
      if (pTemp->mpXStreamConn)
         (pTemp->mpXStreamConn)->service();

      //if (pTemp->mpLspDataConn)
      //   (pTemp->mpLspDataConn)->service();

      // abstract the lsp connected state to a separate bool for easier checking
      if (mpLSPConn &&
            (mpLSPConn->getState() == BLSPConnection::cStateXNetConnected) &&
            (pTemp->mServices.getSize() > 0) &&
            mState == cStateIdle &&
            pTemp->mPendingConnect)
      {
         // if we're connected to the SG and have pending overlapped operations, connect them up
         connect(pTemp);
      }
      else if ((pTemp->mProtocol == cTCP && (!pTemp->mpXStreamConn || !(pTemp->mpXStreamConn)->hasObservers())) &&
               (timeGetTime() > pTemp->mTimeout))
      {
         if (pTemp->mpXStreamConn && (pTemp->mpXStreamConn)->getState() == BXStreamConnection::cStateError)
         {
            switch ((pTemp->mpXStreamConn)->getError())
            {
               case BXStreamConnection::cErrorAPIFailure:
               case BXStreamConnection::cErrorConnectionClosed:
               case BXStreamConnection::cErrorConnectionFailure:
                  {
                     mTCPConnectFailed = true;
                     break;
                  }
            }
         }

         // if this handler is marked for removal or we no longer have observers, then we're free to clean up
         mConnections.removeIndex(i);
         BLSPConnectionHandler::releaseInstance(pTemp);
      }
      else if (pTemp->mpXStreamConn && (pTemp->mpXStreamConn)->getState() == BXStreamConnection::cStateError)
      {
         switch ((pTemp->mpXStreamConn)->getError())
         {
            case BXStreamConnection::cErrorAPIFailure:
            case BXStreamConnection::cErrorConnectionClosed:
            case BXStreamConnection::cErrorConnectionFailure:
               {
                  mTCPConnectFailed = true;
                  break;
               }
         }
      }
   }
}

//==============================================================================
// Need to reset the timeouts associated with all the LSP calls since we go
// go dark on the sim thread when loading a level causing pending XLSP
// operations to immediately timeout upon the first game update.
//==============================================================================
void BLSPManager::resetTimeout()
{
   // reset the mpLSPConn timeout
   if (mpLSPConn)
      mpLSPConn->resetTimeout();

   // reset the mLSPFileUpload timeouts
   for (uint i=0; i < mLSPFileUpload.getSize(); ++i)
      mLSPFileUpload[i]->resetTimeout();

   if (mpLSPFileUpload)
      mpLSPFileUpload->resetTimeout();

   // reset the BLSPConnectionHandler and BXStreamConnection timeouts
   for (int32 i=mConnections.getSize()-1; i >= 0; --i)
   {
      BLSPConnectionHandler* pTemp = mConnections[i];
      if (!pTemp)
         continue;

      if (pTemp->mpXStreamConn)
         (pTemp->mpXStreamConn)->resetTimeout();

      pTemp->mTimeout = timeGetTime() + mSGTimeout;
   }
}

//==============================================================================
// 
//==============================================================================
//DWORD BLSPManager::connect(eProtocol protocol, ushort port, PBLSPOVERLAPPED pOverlapped, uint flags, BLSPTitleServerConnection* pTitleServerConnection)
DWORD BLSPManager::connect(eProtocol protocol, ushort port, uint flags, BLSPTitleServerConnection* pTitleServerConnection)
{
   BASSERTM(pTitleServerConnection, "Missing BLSPTitleServerConnection for connecting to the XLSP");

   // before I make a UDP connection, I need to run through the XDK APIs to
   // establish a secure tunnel to our Security Gateway
   // this is wrapped up for us in the BLSPConnection code
   // the calls will be async, so this will need to be async as well

   // if we already have a connection on the given port, then I should be able
   // to fix up the overlapped structure and return ERROR_SUCCESS
   //
   // but if the current connection is still pending because of the XNetConnect
   // then I need to cache the overlapped handler and return an ERROR_IO_PENDING
   //
   BLSPConnectionHandler* pHandler = NULL;

   uint count = mConnections.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPConnectionHandler* pTemp = mConnections[i];
      if (!pTemp)
         continue;

      // I already have a pending request for the requested port
      if (pTemp->mPort == port && pTemp->mProtocol == protocol)
      {
         pHandler = pTemp;
         //pHandler->mpTitleServerConnection = pTitleServerConnection;
         break;
      }
   }

   if (!pHandler)
   {
      pHandler = BLSPConnectionHandler::getInstance();
      pHandler->init(protocol, port, flags);
      mConnections.add(pHandler);
   }

   if (pTitleServerConnection)
   {
      // check if our pTitleServerConnection is already in the list for this handler
      for (int j=pHandler->mServices.getSize()-1; j >= 0; --j)
      {
//-- FIXING PREFIX BUG ID 1778
         const BLSPTitleServerConnection* pExisting = pHandler->mServices[j];
//--
         BDEBUG_ASSERT(pExisting);
         if (pExisting == NULL)
         {
            pHandler->mServices.removeIndex(j);
         }
         else if (pExisting == pTitleServerConnection)
            break;
      }

      if (j < 0)
         pHandler->mServices.add(pTitleServerConnection);
   }

   // if we are not yet connected to the security gateway
   // then initiate that connection now and place us in the pending state
   if (!mpLSPConn)
   {
      mLastSGAttempt = timeGetTime();

      mXEnumerateFailed = false;
      mXNetConnectFailed = false;

      mpLSPConn = new BLSPConnection();
      mpLSPConn->init();
      setState(cStateLSPPending);
      return ERROR_IO_PENDING;
   }

   // need to verify our connection to the SG
   if (mpLSPConn->getState() != BLSPConnection::cStateXNetConnected)
   {
      return ERROR_IO_PENDING;
   }

   // if I'm missing a connection but I'm connected to our XLSP
   // then complete the connection process here
   if (pHandler->mPendingConnect)
      return connect(pHandler);

   if (mState == cStateDataPending)
      return ERROR_IO_PENDING;

   completeConnect(pHandler);

   return ERROR_SUCCESS;
}

//==============================================================================
// 
//==============================================================================
//void BLSPManager::cancel(eProtocol protocol, ushort port, PBLSPOVERLAPPED pOverlapped)
void BLSPManager::cancel(eProtocol protocol, ushort port, BLSPTitleServerConnection* pTitleServerConnection)
{
   uint count = mConnections.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPConnectionHandler* pTemp = mConnections[i];
      if (!pTemp)
         continue;

      // I already have a pending request for the requested port
      if (pTemp->mPort == port && pTemp->mProtocol == protocol)
      {
         if (pTemp->mServices.remove(pTitleServerConnection))
         {
            SYSTEMTIME t;
            GetSystemTime(&t);
            nlog(cLSPCL, "BLSPManager::cancel -- protocol[%d] port[%u] dt[%04d%02d%02dT%02d%02d%02d]", protocol, port, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

            if (pTemp->mServices.getSize() == 0)
               pTemp->mTimeout = timeGetTime() + mSGTimeout; // terminate us after X seconds, something to give us enough time
                                                             // in the event another service sneaks in adds itself to this port
         }
         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
BLSPConnectionHandler* BLSPManager::getConnection(eProtocol protocol, ushort port)
{
   uint count = mConnections.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPConnectionHandler* pTemp = mConnections[i];
      if (!pTemp)
         continue;

      if (pTemp->mPort == port && pTemp->mProtocol == protocol)
      {
         pTemp->mTimeout = timeGetTime() + mSGTimeout;
         return pTemp;
      }
   }

   return NULL;
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::isInNoLSPMode()
{
   if (mNoLSPMode)
   {
      if (mLastSGAttempt + mSGAttemptTTL < timeGetTime())
         mNoLSPMode = false;
      else if ((mXEnumerateFailed || mXNetConnectFailed || mTCPConnectFailed) && (mLastSGAttempt + mLSPFailTTL < timeGetTime()))
         mNoLSPMode = false;
      else if (mLIVEConnectionChangeTime > mLastSGAttempt)
         mNoLSPMode = false;
   }

   return mNoLSPMode;
}

//==============================================================================
// The LSP code manages the stream pointer from this point forward
//==============================================================================
void BLSPManager::uploadFile(XUID xuid, const BSimString& gamerTag, const BSimString& filename, BStream* pStream)
{
   if (!gConfig.isDefined(cConfigLSPEnableFileUpload) || isInNoLSPMode())
   {
      delete pStream;
      return;
   }

   // we need the xuid of the person requesting the upload
   // we'll use that to verify that if we need to perform other actions re: Live
   // that we're matching up to the same user and not some other jimmy that just
   // sat down at the controls
   if (xuid == 0)
   {
      delete pStream;
      return;
   }

   BDEBUG_ASSERT(pStream);
   if (!pStream)
      return;

   //BDEBUG_ASSERT(mpLSPFileUpload == 0); // it is an error to send two files simultaneously

   //delete mpLSPFileUpload;

   //// open a connection to our LSP and transmit the data from the stream
   //mpLSPFileUpload = new BLSPFileUpload(xuid, gamerTag, filename, pStream, cDefaultLSPStatsUploadPort, cConfigLSPFileUploadPort);

   SYSTEMTIME t;
   GetSystemTime(&t);
   nlog(cLSPCL, "BLSPManager::uploadFile xuid[%I64u] dt[%04d%02d%02dT%02d%02d%02d]", xuid, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

   BLSPFileUpload* pFileUpload = new BLSPFileUpload(xuid, gamerTag, filename, pStream, cDefaultLSPStatsUploadPort, cConfigLSPFileUploadPort);

   if (mpLSPFileUpload == NULL)
   {
      mpLSPFileUpload = pFileUpload;
      mpLSPFileUpload->init();
   }
   else
      mLSPFileUpload.add(pFileUpload);
}

//==============================================================================
// This is only debug/testing code for a sample auth component and not
// intended for production in its current state
//==============================================================================
void BLSPManager::checkAuth()
{
   if (!gConfig.isDefined(cConfigLSPEnableAuth))
   {
      //No config for auth -then we are going to say we are always authorized
      mAuthorizationRunning = false;
      mAuthRequestSucceeded = true;
      return;
   }

   if (mpLSPAuth)
   {
      BASSERTM(false, "BLSPManager::checkAuth - previous checkAuth still running");
      delete mpLSPAuth;
      mpLSPAuth = NULL;
   }

   mAuthorizationRunning = true;

   mpLSPAuth = new BLSPAuth(&mAuthCache);
   mpLSPAuth->connect();
}

//==============================================================================
// 
//==============================================================================
BLSPResponse BLSPManager::isBanned(XUID xuid)
{
   if(!gConfig.isDefined(cConfigLSPEnableAuth))
   {
      return cNo;
   }

   PBLSPAuthUser pUser = NULL;

   BLSPResponse response = initAuthRequest(xuid, &pUser);

   if (response == cYes)
   {
      if(!mAuthRequestSucceeded)
         return cFailed;
      return (pUser->mBanEverything ? cYes : cNo);
   }

   return response;
}

//==============================================================================
// 
//==============================================================================
BLSPResponse BLSPManager::isBannedMedia(XUID xuid)
{
   PBLSPAuthUser pUser = NULL;

   BLSPResponse response = initAuthRequest(xuid, &pUser);

   if (response == cYes)
      return (pUser->mBanMedia ? cYes : cNo);

   return response;
}

//==============================================================================
// 
//==============================================================================
BLSPResponse BLSPManager::isBannedMatchMaking(XUID xuid)
{
   PBLSPAuthUser pUser = NULL;

   BLSPResponse response = initAuthRequest(xuid, &pUser);

   if (response == cYes)
      return (pUser->mBanMatchMaking ? cYes : cNo);

   return response;
}

//==============================================================================
// Starts a request the config data to be applied to a particular hopper set
//==============================================================================
void BLSPManager::requestConfigData(BMatchMakingHopperList* pHopperList)
{
   if (!gConfig.isDefined(cConfigLSPEnableConfigData) || isInNoLSPMode())
   {
      //No config for ConfigData from the LSP - then don't even try
      mConfigDataRequestRunning = false;
      return;
   }

   if (mConfigDataRequestRunning)
      return;

   // check the TTLs
   if (mConfigCache.getStatic().mLastUpdate == 0 ||
       mConfigCache.getDynamic().mLastUpdate == 0 ||
       mConfigCache.getStatic().mLastUpdate + mConfigCache.getStatic().mTTL < timeGetTime() ||
       mConfigCache.getDynamic().mLastUpdate + mConfigCache.getDynamic().mTTL < timeGetTime() ||
       (sgFailure() && mConfigFailureTime + mLSPFailTTL < timeGetTime()) ||
       (liveStatusChanged() && mConfigFailureTime < mLIVEConnectionChangeTime))
   {
      mConfigFailureTime = 0;

      if (mpLSPConfigData)
      {
         delete mpLSPConfigData;
         mpLSPConfigData = NULL;
      }

      mConfigDataRequestRunning = true;

      mpLSPConfigData = new BLSPConfigData(&mConfigCache);
      mpLSPConfigData->requestConfigData(pHopperList);
   }
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::isConfigDataCurrent()
{
   if (!gConfig.isDefined(cConfigLSPEnableConfigData) || isInNoLSPMode())
   {
      //No config for ConfigData from the LSP - then don't even try
      return true;
   }

   if (mConfigDataRequestRunning)
      return false;

   // check the TTLs
   if (mConfigCache.getStatic().mLastUpdate == 0 ||
       mConfigCache.getDynamic().mLastUpdate == 0 ||
       mConfigCache.getStatic().mLastUpdate + mConfigCache.getStatic().mTTL < timeGetTime() ||
       mConfigCache.getDynamic().mLastUpdate + mConfigCache.getDynamic().mTTL < timeGetTime() ||
       (sgFailure() && mConfigFailureTime + mLSPFailTTL < timeGetTime()) ||
       (liveStatusChanged() && mConfigFailureTime < mLIVEConnectionChangeTime))
   {
      return false;
   }

   return true;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::postPerfData(BPerfLogPacketV2* pDataPacket)
{
   if (!gConfig.isDefined(cConfigLSPEnablePerfReporting) || isInNoLSPMode())
      return;

   if (mpLSPPerfLog)
   {
      delete mpLSPPerfLog;
      mpLSPPerfLog = NULL;
   }

   mpLSPPerfLog = new BLSPPerfLog();
   mpLSPPerfLog->postPerfData(pDataPacket);
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTask* BLSPManager::getMediaTaskByID(uint taskID)
{
   BLSPMediaTransfer* pMediaTransfer = getMediaTransfer();
   if (pMediaTransfer == NULL)
      return NULL;

   return pMediaTransfer->getTaskByID(taskID);
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTask* BLSPManager::createMediaTask()
{
   BLSPMediaTransfer* pMediaTransfer = getMediaTransfer();
   if (pMediaTransfer == NULL)
      return NULL;

   return pMediaTransfer->createTask();
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::terminateMediaTask(BLSPMediaTask* pTask)
{
   if (mpLSPMediaTransfer)
      mpLSPMediaTransfer->terminateTask(pTask);
   else
      BLSPMediaTask::releaseInstance(pTask);
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::updateServiceRecord(XUID xuid)
{
   if (!gConfig.isDefined(cConfigLSPEnableServiceRecord))
      return;

   // verify the TTL
   PBLSPServiceRecordUser pUser = mServiceRecordCache.getUser(xuid);
   if (pUser && (pUser->mLastUpdate == 0 || pUser->mLastUpdate + pUser->mTTL > timeGetTime()))
      return;

   // instantiate a service record query, allow for multiple service record queries?
   // base them on XUID?
   //
   // mimic what the media transfer service did with tasks but keep the tasks internal
   if (mpLSPServiceRecord == NULL)
      mpLSPServiceRecord = new BLSPServiceRecord(&mServiceRecordCache);

   mpLSPServiceRecord->query(xuid);
}

//==============================================================================
// 
//==============================================================================
uint BLSPManager::queryServiceRecord(XUID xuid)
{
   // if we ever add more fields to the service record, then we should change this
   PBLSPServiceRecordUser pUser = mServiceRecordCache.getUser(xuid);
   if (pUser == NULL)
      return 0;

   return pUser->mLevel;
}

//==============================================================================
// 
//==============================================================================
BLSPMediaTransfer* BLSPManager::getMediaTransfer()
{
   if (!gConfig.isDefined(cConfigLSPEnableMediaTransfer) || isInNoLSPMode())
      return NULL;

   if (mpLSPMediaTransfer == NULL)
      mpLSPMediaTransfer = new BLSPMediaTransfer();

   return mpLSPMediaTransfer;
}

//==============================================================================
// Initiate the data connection and reset the timeout to give us some time
// * actually, rework the code that assumes the connect will always succeed
//   but internally fail so when we loop around to check on things, we can
//   initiate the cleanup then instead of now
// * I do not like having competing timeout values.  The timeouts for handling
//   the data connection should be done within that code and not here
//==============================================================================
DWORD BLSPManager::connect(BLSPConnectionHandler* pHandler)
{
   if (pHandler->mProtocol == cTCP)
   {
      if (pHandler->mpXStreamConn) 
      { 
         delete pHandler->mpXStreamConn; 
         pHandler->mpXStreamConn = NULL; 
      } 

      //BXStreamConnection* pConn = pHandler->mpXStreamConn = new BXStreamConnection(pHandler->mFlags, mTCPTimeout, mTCPReconnectAttempts);
      BXStreamConnection* pConn = pHandler->mpXStreamConn = new BXStreamConnection(pHandler->mFlags, mTCPTimeout);

      // go through all the BLSPTitleServerConnections for this handler and add them as observers on the BXStreamConnection
      uint count = pHandler->mServices.getSize();
      for (uint i=0; i < count; ++i)
      {
         BLSPTitleServerConnection* pService = pHandler->mServices[i];
         if (pService)
            pConn->addObserver(pService);
      }

      HRESULT hr = pConn->connect(mpLSPConn->getAddr(), pHandler->mPort);
      if (FAILED(hr))
         return hr;

      mTCPConnectFailed = false;
   }
   // if I already have a BLSPDataConnection instance, then I can simply
   // update the timeout value and return success
   //else if (!pHandler->mpLspDataConn)
   //{
   //   // create our data connection and inform all the observers that we're mostly ready
   //   // one of the observers will still need to initiate the data connection
   //   // the initiation can be called multiple times
   //   BLSPDataConnection* pConn = pHandler->mpLspDataConn = BLSPDataConnection::getInstance();

   //   // initialize the data connection with our udp connection/socket
   //   HRESULT hr = pConn->connect(mpLSPConn->getAddr(), pHandler->mPort);
   //   if (FAILED(hr))
   //      return hr;
   //}

   // no longer pending a stream/data connection
   pHandler->mPendingConnect = false;

   // bump our timeout so we give other interested parties time to react
   pHandler->mTimeout = timeGetTime() + mSGTimeout;

   return ERROR_SUCCESS;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::completeConnect(BLSPConnectionHandler* pHandler)
{
   if (pHandler->mProtocol == cTCP)
   {
      completeStreamConnect(pHandler);
   }
   //else
   //{
   //   completeDataConnect(pHandler);
   //}
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::completeStreamConnect(BLSPConnectionHandler* pHandler)
{
   BASSERT(pHandler);
   BASSERT(pHandler->mProtocol == cTCP);

//-- FIXING PREFIX BUG ID 1776
   //const BXStreamConnection* pConn = pHandler->mpXStreamConn;
//--

   // if the BXStreamConnection is in an error state, calling this with ERROR_SUCCESS
   // will attempt to re-create the connection and proceed
   //
   // the code below must have been from the first implementation that cleaned-up/retried differently than we do now
   completeConnect(pHandler, ERROR_SUCCESS);

   //switch (pConn->getState())
   //{
   //   case BXStreamConnection::cStateConnected:
   //      {
   //         completeConnect(pHandler, ERROR_SUCCESS);
   //      }
   //      break;
   //   case BXStreamConnection::cStateDisconnected:
   //   case BXStreamConnection::cStateError:
   //      {
   //         completeConnect(pHandler, ERROR_CONNECTION_UNAVAIL);
   //      }
   //      break;
   //}
}

//==============================================================================
// 
//==============================================================================
//void BLSPManager::completeDataConnect(BLSPConnectionHandler* pHandler)
//{
//   BASSERT(pHandler);
//   BASSERT(pHandler->mProtocol == cUDP);
//
//   BLSPDataConnection* pConn = pHandler->mpLspDataConn;
//
//   switch (pConn->getState())
//   {
//      case BLSPDataConnection::cStateConnected:
//         {
//            completeConnect(pHandler, ERROR_SUCCESS);
//         }
//         break;
//      case BLSPDataConnection::cStateDisconnected:
//      case BLSPDataConnection::cStateTimedout:
//         {
//            completeConnect(pHandler, ERROR_CONNECTION_UNAVAIL);
//         }
//         break;
//   }
//}

//==============================================================================
// This assumes a successful data connection, if we have a failure, other
// code will handle the cleanup/setting of events/error codes
//
// this method could be expanded for failure cases once we know how we want
// to handle them
//==============================================================================
void BLSPManager::completeConnect(BLSPConnectionHandler* pHandler, DWORD dwResult)
{
   //if (pHandler->mProtocol == cUDP)
   //{
   //   if (pHandler->mpLspDataConn)
   //   {
   //      pHandler->mpXDataConn = (pHandler->mpLspDataConn)->getConn();
   //      pHandler->mpXDataSocket = (pHandler->mpLspDataConn)->getSocket();
   //   }
   //}

   if (dwResult)
   {
      uint count = pHandler->mServices.getSize();
      for (uint i=0; i < count; ++i)
      {
         BLSPTitleServerConnection* pService = pHandler->mServices[i];
         if (!pService)
            continue;

         pService->disconnected(dwResult);
      }

      return;
   }

   // if we have a connection and it's not connected or in the process of connecting
   // then let's recreate it now
   if (pHandler->mpXStreamConn != NULL && !(pHandler->mpXStreamConn)->isConnecting())
   {
      delete pHandler->mpXStreamConn;
      pHandler->mpXStreamConn = NULL;

      connect(pHandler);
   }
   else if (pHandler->mpXStreamConn == NULL)
      connect(pHandler);
   else
   {
      uint count = pHandler->mServices.getSize();
      for (uint i=0; i < count; ++i)
      {
         BLSPTitleServerConnection* pService = pHandler->mServices[i];
         if (!pService)
            continue;

         (pHandler->mpXStreamConn)->addObserver(pService);
      }
   }

   //uint count = pHandler->mServices.getSize();
   //for (uint i=0; i < count; ++i)
   //{
   //   BLSPTitleServerConnection* pService = pHandler->mServices[i];
   //   if (!pService)
   //      continue;

   //   if (pOverlapped->hEvent != INVALID_HANDLE_VALUE)
   //      SetEvent(pOverlapped->hEvent);

   //   pOverlapped->dwExtendedError = dwResult;
   //}

   //pHandler->mOverlapped.clear();
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::shutdownServices(bool goIdle)
{
   // need to close down all data connections, pending IO, etc..

   uint count = mConnections.getSize();
   for (uint i=0; i < count; ++i)
   {
      BLSPConnectionHandler* pHandler = mConnections[i];
      if (!pHandler)
         continue;

      completeConnect(pHandler, ERROR_FUNCTION_FAILED);

      BLSPConnectionHandler::releaseInstance(pHandler);
   }

   mConnections.clear();

   if (goIdle)
      setState(cStateIdle);

   mAuthorizationRunning = false;

   if (mpLSPAuth)
      mpLSPAuth->setState(BLSPAuth::cStateDone);

   if (mConfigDataRequestRunning && sgFailure())
      mConfigFailureTime = timeGetTime();

   mConfigDataRequestRunning = false;

   if (mpLSPConfigData)
      mpLSPConfigData->setState(BLSPConfigData::cStateDone);

   if (mpLSPPerfLog)
      mpLSPPerfLog->setState(BLSPPerfLog::cStateDone);

   for (uint i=0; i < mLSPFileUpload.getSize(); ++i)
      mLSPFileUpload[i]->setState(BLSPFileUpload::cStateDone);

   if (mpLSPFileUpload)
      mpLSPFileUpload->setState(BLSPFileUpload::cStateDone);

   //if (mpLSPMediaTransfer)
   //   mpLSPMediaTransfer->setState(BLSPMediaTransfer::cStateDone);

   if (mpLSPServiceRecord)
      mpLSPServiceRecord->setState(BLSPServiceRecord::cStateDone);
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::sgFailure() const
{
   return (mXEnumerateFailed || mXNetConnectFailed || mTCPConnectFailed);
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::liveStatusChanged() const
{
   return (mLIVEConnectionChangeTime > mLastSGAttempt);
}

//==============================================================================
// 
//==============================================================================
bool BLSPManager::initAuthRequest()
{
   if (!gConfig.isDefined(cConfigLSPEnableAuth) || isInNoLSPMode())
   {
      mAuthorizationRunning = false;
      return false;
   }

   if (mAuthorizationRunning)
      return true;

   // temp hack, need to queue up another auth request for this user
   if (mpLSPAuth)
   {
      delete mpLSPAuth;
      mpLSPAuth = NULL;
   }

   mAuthorizationRunning = true;

   mpLSPAuth = new BLSPAuth(&mAuthCache);
   mpLSPAuth->connect();

   return true;
}

//==============================================================================
// Returns cNo if it was unable to handle the init
//==============================================================================
BLSPResponse BLSPManager::initAuthRequest(XUID xuid, PBLSPAuthUser* ppUser)
{
   if(!gConfig.isDefined(cConfigLSPEnableAuth))
   {
      return cYes;
   }

   const BLSPAuthUser& machine = mAuthCache.getMachine();
   if (machine.mXuid == INVALID_XUID || machine.mLastUpdate + machine.mTTL < timeGetTime())
   {
      if (!initAuthRequest())
      {
         return cFailed;
      }

      return cPending;
   }

   PBLSPAuthUser pUser = mAuthCache.getUser(xuid);

   if (pUser == NULL || pUser->mLastUpdate + pUser->mTTL < timeGetTime())
   {
      if (!initAuthRequest())
      {
         return cFailed;
      }

      return cPending;
   }

   if (ppUser)
      *ppUser = pUser;

   return cYes;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::setState(eState state)
{
   if (mState == state)
      return;

   mState = state;
   if (mState == cStateLSPPending)
      setRetries(10, 2000);
   else if (mState == cStateDataPending)
      setRetryInterval(500);
   else if (mState == cStateLSPDisconnect)
      setRetries(20, 1000);
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = timeGetTime() + mRetryInterval;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::setRetryInterval(DWORD interval)
{
   mRetryInterval = interval;
   mRetryTime = timeGetTime() + mRetryInterval;
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::requestTickerData(BUITicker * ticker)
{
   if (!gConfig.isDefined(cConfigLSPEnableTicker) || isInNoLSPMode())
      return;

   if (!mpLSPTicker)
   {
      mpLSPTicker = new BLSPTicker(ticker);
      mpLSPTicker->connect();
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPManager::removeTicker()
{
   if (mpLSPTicker)
   {
      mpLSPTicker->removeTicker();
   }
}
