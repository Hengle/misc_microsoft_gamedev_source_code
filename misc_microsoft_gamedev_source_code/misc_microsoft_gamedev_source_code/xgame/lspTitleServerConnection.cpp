//==============================================================================
// lspTitleServerConnection.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h"
#include "configsgame.h"
#include "lspTitleServerConnection.h"
#include "lspManager.h"

//Logging
#include "mpcommheaders.h"
#include "commlog.h"

//==============================================================================
// 
//==============================================================================
BLSPTitleServerConnection::BLSPTitleServerConnection(const uint16 port, const long portOverrideConfigId, uint8 serviceID, const long serviceIDOverrideConfigID, const uint timeout) :
   BXStreamConnection::BXStreamObserver(),
   mServiceID(serviceID),
   mState(cStateIdle),
   mTimeoutValue(timeout),
   mTimeout(0),
   mPort(port),
   mPendingSends(),
   mCompress(false)
{
   long p;
   if (gConfig.get(portOverrideConfigId, &p))
      mPort = static_cast<ushort>(p);
   if (gConfig.get(serviceIDOverrideConfigID, &p))
      mServiceID = static_cast<uint8>(p);

   if (mTimeoutValue > 0)
      mTimeout = mTimeoutValue + timeGetTime();
}

//==============================================================================
// 
//==============================================================================
BLSPTitleServerConnection::~BLSPTitleServerConnection()
{
   shutdown();
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::connect()
{
   // the problem here is that by calling gLSPManager.connect, I may get one of the following
   // callbacks before returning from the method:
   // * observing
   // * connected
   // * disconnected
   // so if I return from this method and call setState(), I could be undoing something

   // start by assuming we're moving into the cStateConnect state
   setState(cStateConnect);

   DWORD dwResult = gLSPManager.connect(cTCP, getPort(), BXStreamConnection::eFlagUseHeader, this);
   if (dwResult == ERROR_NOT_LOGGED_ON)
   {
      // I think the upper layers will need to handle the sign-in portion
      // if anything we should attempt to sign in and if it still fails
      // then we can fail
      //
      // I'm actually pending xbox sign-in, but that's essentially the connect state
      // the difference is the retry interval
      //setState(cStateConnect);
   }
   else if (dwResult == ERROR_ACCESS_DENIED)
   {
      // I'm not allowed to upload
      // I don't see anywhere this value could be returned
      // Flagging with an ASSERT to catch if it does 
      // TODO see if we can delete this - eric
      BASSERT(false);
      if (getState() == cStateConnect)
         setState(cStateTimedout);
   }
   else if (dwResult == ERROR_IO_PENDING || dwResult == ERROR_SUCCESS)
   {
      //if (mState != cStateConnected)
      //   setState(cStateConnect);
   }
   else
   {
      // I'm logged in, so I simply need to wait on the SG connection
      //setState(cStateConnect);
   }
}

//==============================================================================
// 
//==============================================================================
bool BLSPTitleServerConnection::reconnect()
{
   if (!getConnection())
      connect();
   else if (!getConnection()->isConnecting())
   {
      disconnect();

      connect();
   }

   return (mState == cStateConnect);
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::resetTimeout()
{
   if (mTimeoutValue > 0)
      mTimeout = mTimeoutValue + timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::disconnect()
{
   if(getConnection())
      getConnection()->removeObserver(this);   

   setState(cStateDisconnected);
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::observing(BXStreamConnection* pConnection)
{
   BXStreamConnection::BXStreamObserver::observing(pConnection);
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::connected(BXStreamConnection& connection)
{
   setState(cStateConnected);

   BXStreamConnection::BXStreamObserver::connected(connection);
   // send pending data
   uint i = 0;
   for(i = 0; i < mPendingSends.getSize(); ++i)
   {
      const BSerialBuffer& sb = mPendingSends[i];
      getConnection()->send(sb.getBuffer(), sb.getBufferSize(), mCompress);
   }
   mPendingSends.clear();
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::dataReceived(uint8 serviceId, int32 type, int32 version, const void* pData, int32 size)
{
   BXStreamConnection::BXStreamObserver::dataReceived(serviceId, type, version, pData, size);

   mTimeout = mTimeoutValue + timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::disconnected(uint status)
{
   BXStreamConnection::BXStreamObserver::disconnected(status);

   setState(cStateDone);
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::shutdown()
{
   BXStreamConnection::BXStreamObserver::shutdown();
   // my connection is going away, cleanup what we can and go away
   setState(cStateDone);

   // need to remove my observer from the lsp manager
   gLSPManager.cancel(cTCP, mPort, this);
}

//==============================================================================
// 
//==============================================================================
uint8 BLSPTitleServerConnection::getServiceID() const
{
   return mServiceID;
}

//==============================================================================
// 
//==============================================================================
bool BLSPTitleServerConnection::isFinished()
{
   if (getConnection() && getConnection()->isSending())
      return false;

   if (mState == cStatePendingDone)
      setState(cStateDone);

   if (mTimeoutValue > 0 && mTimeout != 0 && mTimeout < timeGetTime())
   {
      SYSTEMTIME t;
      GetSystemTime(&t);
      nlog(cLSPCL, "BLSPTitleServerConnection::isFinished -- timed out port[%d] serviceID[%d] dt[%04d%02d%02dT%02d%02d%02d]", mPort, mServiceID, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

      setState(cStateDone);
      mTimeout = 0;
   }

   return (mState == cStateDone); 
}

//==============================================================================
// 
//==============================================================================
uint16 BLSPTitleServerConnection::getPort() const
{
   return mPort;
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::setState(eState state)
{
   mState = state;
}

//==============================================================================
// 
//==============================================================================
const BLSPTitleServerConnection::eState BLSPTitleServerConnection::getState() const
{
   return mState;
}

//==============================================================================
// 
//==============================================================================
void BLSPTitleServerConnection::send(BLSPPacket& packet, bool compress)
{
   BSerialBuffer sb;
   packet.serialize(sb);
   if (getConnection())
   {
      getConnection()->send(sb.getBuffer(), sb.getBufferSize(), compress);
   }
   else
   {
      // buffer it, this can't happen during a connect() callback since mpConn is set
      // to a non-null value passed by reference
      mCompress = compress;
      mPendingSends.add(sb);
   }
}
