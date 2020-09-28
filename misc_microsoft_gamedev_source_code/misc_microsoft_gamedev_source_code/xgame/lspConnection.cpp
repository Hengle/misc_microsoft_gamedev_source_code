//==============================================================================
// lspConnection.cpp
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================

// Includes
#include "common.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"

#include "lspManager.h"
#include "lspConnection.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

// xsystem
#include "notification.h"

//==============================================================================
// 
//==============================================================================
BLSPConnection::BLSPConnection() :
   mpOverlapped(NULL),
   mpServerInfo(NULL),
   mServerEnumHandle(INVALID_HANDLE_VALUE),
   mServiceID(0x4D530838),
   mNumServerInfo(0),
   mEnumItemCount(0),
   mEnumPick(0),
   mConnectAttempts(0),
   mConnectMaxRetries(cDefaultLSPSGConnectTimeout/1000),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mState(cStateIdle),
   mError(cErrorSuccess)
   //mSignInRequested(false)
{
   Utils::FastMemSet(&mOverlapped, 0, sizeof(XOVERLAPPED));
   Utils::FastMemSet(&mServerAddr, 0, sizeof(IN_ADDR));

   // comment this out for synchronous operation
   mpOverlapped = &mOverlapped;
}

//==============================================================================
// 
//==============================================================================
BLSPConnection::~BLSPConnection()
{
   shutdown();

   if (mpServerInfo)
      delete[] mpServerInfo;
   mpServerInfo = NULL;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::init()
{
   setRetries(1, 1);
   setState(cStateXNetEnumerate);

   long timeout;
   if (gConfig.get(cConfigLSPDefaultSGConnectTimeout, &timeout))
      mConnectMaxRetries = static_cast<uint>(timeout) / 1000;

   // at least try once
   if (mConnectMaxRetries == 0)
      mConnectMaxRetries = 1;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::shutdown()
{
   // we're about to be deleted, so gracefully cleanup what we can
   if (mServerEnumHandle != INVALID_HANDLE_VALUE)
   {
      if (!XHasOverlappedIoCompleted(mpOverlapped))
         XCancelOverlapped(mpOverlapped);

      CloseHandle(mServerEnumHandle);
   }

   mServerEnumHandle = INVALID_HANDLE_VALUE;

   if (mServerAddr.S_un.S_addr != 0)
      disconnect();

   setState(cStateShutdown);
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::resetTimeout()
{
   if (mState == cStateXNetEnumeratePending)
   {
      setRetries(10, 1000);
   }
   else if (mState == cStateXNetConnecting)
   {
      setRetries(mConnectMaxRetries, 1000);
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::service()
{
   if (mState == cStateXNetConnected)
      return;

   switch (mState)
   {
      case cStateXNetEnumerate:
         {
            if (timeGetTime() > mRetryTime)
            {
               if (!xNetEnumerate())
               {
                  mRetryTime = timeGetTime() + mRetryInterval;
                  if (++mNumRetries > mMaxRetries)
                     setState(cStateError, cErrorTimedout);
               }
            }
         }
         break;

      case cStateXNetConnect:
         {
            if (timeGetTime() > mRetryTime)
            {
               if (!xNetConnect())
               {
                  mRetryTime = timeGetTime() + mRetryInterval;
                  if (++mNumRetries > mMaxRetries)
                     setState(cStateError, cErrorTimedout);
               }
               //else
               //{
               //   mState = cStateXNetConnected;
               //}
            }
         }
         break;

      //case cStatePendingSignin:
      //   {
      //      // if we're in the sign-in pending state, check for a notification change
      //      DWORD msg = gNotification.getNotification();
      //      switch (msg)
      //      {
      //         case XN_LIVE_CONNECTIONCHANGED:
      //            {
      //               if (gNotification.getParam() == XONLINE_S_LOGON_CONNECTION_ESTABLISHED)
      //               {
      //                  setState(cStateVerifySignin);
      //               }
      //            }
      //            break;
      //         default:
      //            {
      //               if (timeGetTime() > mRetryTime)
      //               {
      //                  if (!gNotification.isSystemUIShowing())
      //                  {
      //                     // if the user dismissed the UI but we're still pending, then
      //                     //setState(cStateDone);
      //                     setState(cStateVerifySignin);
      //                  }
      //                  else
      //                  {
      //                     mRetryTime = timeGetTime() + mRetryInterval;
      //                     if (++mNumRetries > mMaxRetries)
      //                        setState(cStateError, cErrorTimedout);
      //                  }
      //               }
      //            }
      //            break;
      //      }
      //   }
      //   break;

      //case cStateVerifySignin:
      //   {
      //      // if our sign-in was verified, then we're free to continue
      //      if (verifySignIn())
      //         setState(cStateXNetEnumerate);
      //   }
      //   break;

      case cStateXNetConnecting:
         {
            xNetCheckConnect();
         }
         break;

      //case cStateXNetConnected:
      //   {
      //   }
      //   break;

      case cStateXNetPickServer:
         {
            xNetPickServer();
         }
         break;

      case cStateXNetEnumeratePending:
         {
            xNetCheckEnumerate();
         }
         break;

      case cStateReconnect:
         {
            reconnect();
         }
         break;
   }
}

//==============================================================================
// 
//==============================================================================
bool BLSPConnection::xNetEnumerate()
{
   DWORD dwResult = 0;
   DWORD dwServerCount = 0;
   DWORD dwBufferSize = 0;
   PDWORD pcItemsReturned = NULL;

   LPCSTR pszServerInfo = "HaloWarsTitleServer";

   BSimString serverInfo;
   if (gConfig.get(cConfigLSPServerFilter, serverInfo))
      pszServerInfo = serverInfo.getPtr();

   long serviceID;
   if (gConfig.get(cConfigLSPServiceID, &serviceID))
      mServiceID = static_cast<DWORD>(serviceID);

   if (mServerEnumHandle != INVALID_HANDLE_VALUE)
   {
      if (!XHasOverlappedIoCompleted(mpOverlapped))
         XCancelOverlapped(mpOverlapped);

      CloseHandle(mServerEnumHandle);
      mServerEnumHandle = INVALID_HANDLE_VALUE;
   }

   dwResult = XTitleServerCreateEnumerator(pszServerInfo, 10, &dwBufferSize, &mServerEnumHandle);
   if (ERROR_SUCCESS != dwResult)
   {
      return false;
   }

   DWORD serverInfoCount = dwBufferSize / sizeof(XTITLE_SERVER_INFO);

   if (serverInfoCount != mNumServerInfo)
   {
      mNumServerInfo = dwBufferSize / sizeof(XTITLE_SERVER_INFO);

      // allocate a buffer for the server info structures
      if (mpServerInfo)
         delete[] mpServerInfo;
      mpServerInfo = new XTITLE_SERVER_INFO[mNumServerInfo];
   }

   if (!mpOverlapped)
      pcItemsReturned = &dwServerCount;

   dwResult = XEnumerate(mServerEnumHandle, mpServerInfo, sizeof(XTITLE_SERVER_INFO)*mNumServerInfo, pcItemsReturned, mpOverlapped);
   if (dwResult == ERROR_IO_PENDING)
   {
      setRetries(10, 1000);
      setState(cStateXNetEnumeratePending);
      return true;
   }
   else if (dwResult != ERROR_SUCCESS)
   {
      // most likely disconnected from Live, treat as error
      setState(cStateError, cErrorNotSignedIn);
      CloseHandle(mServerEnumHandle);
      mServerEnumHandle = INVALID_HANDLE_VALUE;
      return true;
   }

   mEnumItemCount = dwServerCount;

   setState(cStateXNetPickServer);

   return true;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::xNetCheckEnumerate()
{
   if (timeGetTime() < mRetryTime)
      return;

   if (!XHasOverlappedIoCompleted(mpOverlapped))
   {
      mRetryTime = timeGetTime() + mRetryInterval;
      if (++mNumRetries > mMaxRetries)
         setState(cStateError, cErrorTimedout);
      return;
   }

   // the overlapped io completed, check the return codes

   DWORD dwItemsReturned;

   DWORD dwResult = XGetOverlappedResult(mpOverlapped, &dwItemsReturned, FALSE);
   if (dwResult == ERROR_IO_INCOMPLETE)
   {
      SYSTEMTIME t;
      GetSystemTime(&t);
      nlog(cLSPCL, "BLSPConnection::xNetCheckEnumerate -- enumerate failed result[%u] dt[%04d%02d%02dT%02d%02d%02d]", dwResult, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      // the operation is still pending?
      setState(cStateXNetEnumerateFailed);
   }
   else if (dwResult == ERROR_SUCCESS || dwResult == ERROR_FUNCTION_FAILED)
   {
      mEnumItemCount = dwItemsReturned;

      // randomly pick one from the item count
      mEnumPick = (mEnumItemCount > 0 ? rand() % mEnumItemCount : 0);

      SYSTEMTIME t;
      GetSystemTime(&t);
      nlog(cLSPCL, "BLSPConnection::xNetCheckEnumerate -- enumerate finished count[%d] dt[%04d%02d%02dT%02d%02d%02d]", mEnumItemCount, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      setState(cStateXNetPickServer);
   }
   else
   {
      SYSTEMTIME t;
      GetSystemTime(&t);
      nlog(cLSPCL, "BLSPConnection::xNetCheckEnumerate -- enumerate failed result[%u] dt[%04d%02d%02dT%02d%02d%02d]", dwResult, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      setState(cStateXNetEnumerateFailed);
   }

   //DWORD extError = XGetOverlappedExtendedError(mpOverlapped);
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::xNetPickServer()
{
   // pick a server from the list of enumerated servers
   //
   // we will want a smarter system of trying the first one and if that fails,
   // round robining to the next, etc... but then also marking the previous server
   // as badzorz.

   if (mEnumItemCount == 0)
   {
      setState(cStateError, cErrorNoServers);
      return;
   }

   for (uint i=0; i < mEnumItemCount; ++i)
   {
      XTITLE_SERVER_INFO& serverInfo = mpServerInfo[mEnumPick];

      // 0x4D530838 is our assigned service ID
      // can be overridden in the config, LSPServiceID
      if (serverInfo.inaServer.s_addr != 0)
      {
         INT iResult = XNetServerToInAddr(serverInfo.inaServer, mServiceID, &mServerAddr);
         if (iResult == 0)
         {
            setState(cStateXNetConnect);
            return;
         }

         SYSTEMTIME t;
         GetSystemTime(&t);
         nlog(cLSPCL, "BLSPConnection::xNetPickServer -- XNetServerToInAddr failed result[%d] dt[%04d%02d%02dT%02d%02d%02d]", iResult, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

         serverInfo.inaServer.s_addr = 0;
      }

      mEnumPick++;
      if (mEnumPick >= mEnumItemCount)
         mEnumPick = 0;
   }

   setState(cStateError, cErrorNoServers);
}

//==============================================================================
// 
//==============================================================================
bool BLSPConnection::xNetConnect()
{
   INT iResult;

   setState(cStateXNetConnecting);

   SYSTEMTIME t;
   GetSystemTime(&t);
   BNetIPString strAddr(mServerAddr);
   nlog(cLSPCL, "BLSPConnection::xNetConnect -- server[%s] dt[%04d%02d%02dT%02d%02d%02d]", strAddr.getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

   // Connect to server
   if ((iResult = XNetConnect(mServerAddr)) != 0)
   {
      mConnectAttempts++;
      if (mConnectAttempts >= mEnumItemCount)
      {
         setState(cStateError, cErrorXNetConnect);
         return false;
      }
      else
      {
         // have more servers to choose from, let's try another one
         XTITLE_SERVER_INFO& serverInfo = mpServerInfo[mEnumPick];
         serverInfo.inaServer.s_addr = 0;

         mEnumPick++;
         if (mEnumPick >= mEnumItemCount)
            mEnumPick = 0;
         setState(cStateXNetPickServer);
         return true;
      }
   }

   setRetries(mConnectMaxRetries, 1000);

   return true;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::xNetCheckConnect()
{
   DWORD dwResult;

   if (timeGetTime() < mRetryTime)
      return;

   dwResult = XNetGetConnectStatus(mServerAddr);

   if (dwResult == XNET_CONNECT_STATUS_PENDING)
   {
      mRetryTime = timeGetTime() + mRetryInterval;
      if (++mNumRetries > mMaxRetries)
      {
         SYSTEMTIME t;
         GetSystemTime(&t);
         BNetIPString strAddr(mServerAddr);
         nlog(cLSPCL, "BLSPConnection::xNetCheckConnect -- failed due to timeout - server[%s] dt[%04d%02d%02dT%02d%02d%02d]", strAddr.getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

         setState(cStateError, cErrorTimedout);
      }
   }
   else if (dwResult == XNET_CONNECT_STATUS_CONNECTED)
   {
      SYSTEMTIME t;
      GetSystemTime(&t);
      BNetIPString strAddr(mServerAddr);
      nlog(cLSPCL, "BLSPConnection::xNetCheckConnect -- connected to server[%s] dt[%04d%02d%02dT%02d%02d%02d]", strAddr.getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      setState(cStateXNetConnected);
   }
   else if(mConnectAttempts < mEnumItemCount)
   {
      xNetConnect();
   }
   else
   {
      SYSTEMTIME t;
      GetSystemTime(&t);
      BNetIPString strAddr(mServerAddr);
      nlog(cLSPCL, "BLSPConnection::xNetCheckConnect -- connect error[%u] server[%s] dt[%04d%02d%02dT%02d%02d%02d]", dwResult, strAddr.getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      // ran through all servers in enumeration and could not
      // get a connection. 
      setState(cStateError, cErrorXNetConnect);
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::disconnect()
{
   SYSTEMTIME t;
   GetSystemTime(&t);
   BNetIPString strAddr(mServerAddr);
   nlog(cLSPCL, "BLSPConnection::disconnect -- server[%s] dt[%04d%02d%02dT%02d%02d%02d]", strAddr.getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

   INT iResult = 0;

   if ((iResult = XNetUnregisterInAddr(mServerAddr)) != 0)
   {
      // what to do?
      // retry? the xdk doesn't list the possible error conditions
   }

   mState = cStateDisconnected;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::reconnect()
{
   disconnect();

   mState = cStateXNetEnumerate;

   setRetries(1, 1);
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::setState(eState state, eError error)
{
   mState = state;
   mError = error;
}

//==============================================================================
// 
//==============================================================================
void BLSPConnection::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = timeGetTime() + mRetryInterval;
}

//==============================================================================
// TODO-LSP, how do we want to handle verification of LSP abilities when we
//           have multiple users on a single console?
//           Check both the primary and secondary users?  Or only the primary?
//==============================================================================
//bool BLSPConnection::verifySignIn()
//{
//   // use the primary user, if there is no primary user, then we'll fail
//-- FIXING PREFIX BUG ID 3052
//   const BUser* pUser = gUserManager.getPrimaryUser();
//--
//   if (!gUserManager.areUsersSignedIn() || !pUser->isSignedIn())
//   {
//      if (mSignInRequested)
//      {
//         setState(cStateError, cErrorNotSignedIn);
//         return false;
//      }
//
//      mSignInRequested = true;
//
//      gUserManager.showSignInUI();
//
//      // place us in the pending state
//      setState(cStatePendingSignin);
//
//      setRetries(60, 500);
//
//      return false;
//   }
//
//   DWORD controllerID = static_cast<DWORD>(pUser->getPort());
//
//   XUSER_SIGNIN_INFO onlineSigninInfo;
//   XUSER_SIGNIN_INFO offlineSigninInfo;
//
//   if (XUserGetSigninInfo(controllerID, 0x00000001, &onlineSigninInfo) != ERROR_SUCCESS)
//   {
//      setState(cStateError, cErrorAPIFailure);
//      return false;
//   }
//
//   if (XUserGetSigninInfo(controllerID, 0x00000002, &offlineSigninInfo) != ERROR_SUCCESS)
//   {
//      setState(cStateError, cErrorAPIFailure);
//      return false;
//   }
//
//   // insure that we're Live enabled
//   // we only need to check one of the structures (XXX verify)
//   if (onlineSigninInfo.dwInfoFlags != XUSER_INFO_FLAG_LIVE_ENABLED)
//   {
//      setState(cStateError, cErrorNoPermissions);
//      return false;
//   }
//
//   if (onlineSigninInfo.UserSigninState != eXUserSigninState_SignedInToLive)
//   {
//      if (mSignInRequested)
//      {
//         setState(cStateError, cErrorNotSignedIn);
//         return false;
//      }
//
//      mSignInRequested = true;
//
//      gUserManager.showSignInUI(XSSUI_FLAGS_SHOWONLYONLINEENABLED);
//
//      // place us in the pending state
//      setState(cStatePendingSignin);
//
//      setRetries(20, 1000);
//
//      return false;
//   }
//
//   //if (!pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
//   //{
//   //   setState(cStateError, cErrorNoPermissions);
//   //   return false;
//   //}
//
//   return true;
//}
