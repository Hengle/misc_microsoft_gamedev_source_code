//==============================================================================
// mpsimobject.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "mpsimobject.h"
#include "mpgame.h"
#include "netpackets.h"
#include "mpcommheaders.h"
#include "commlog.h"

//==============================================================================
// Defines


//==============================================================================
// BMPSimObject::BMPSimObject
//==============================================================================
BMPSimObject::BMPSimObject(BMPGame *game) : 
   mMPGame(game), mPauseHandler(NULL), mTimingHandler(NULL), mPlayerHandler(NULL), mGCHandler(NULL)
{

} // BMPSimObject::BMPSimObject 

//==============================================================================
// 
void BMPSimObject::sendCommand(BChannelPacket &commandPacket)
{
   HRESULT hr = mMPGame->sendCommandPacket(commandPacket);
   if(FAILED(hr))
      nlog(cMPGameCL, "BMPSimObject::sendCommand -- failed to send command packet. hr: 0x%x", hr);
}

//==============================================================================
// 
void BMPSimObject::sendGCData(BChannelPacket &gcPacket)
{
   HRESULT hr = mMPGame->sendGCPacket(gcPacket);
   hr;
   BASSERT(SUCCEEDED(hr));
}

//==============================================================================
// 
void BMPSimObject::commandDataReceived(ClientID fromClientID, const void *data, DWORD size)
{
   mObserverList.commandReceived(mMPGame->getPlayerID(fromClientID), data, size);
}

//==============================================================================
// 
void BMPSimObject::simDataReceived(ClientID fromClientID, const void *data, DWORD size)
{
   if (BChannelPacket::getType(data) == BChannelPacketType::cPausePacket)
   {
      if (!mPauseHandler)
         return;

      BMessagePacket packet(BChannelPacketType::cPausePacket);
      packet.deserializeFrom(data, size);

      PlayerID lPlayerID = mMPGame->getPlayerID(fromClientID);
      
      if (packet.mMessage > 0)
      {
         mPauseHandler->setPaused(true, true, lPlayerID);
      }
      else
         mPauseHandler->setPaused(false, true, lPlayerID);
   }  
}

//==============================================================================
// 
void BMPSimObject::gcDataReceived(ClientID fromClientID, const void *data, DWORD size)
{
   if (mGCHandler)
      mGCHandler->gcDataReceived(mMPGame->getPlayerID(fromClientID), data, size);
}

//==============================================================================
// 
void BMPSimObject::playerDisconnected(PlayerID playerID)
{
   if (mPlayerHandler)
      mPlayerHandler->playerDisconnected(playerID);
}

//==============================================================================
// 
void BMPSimObject::setPaused(bool v)
{
   BMessagePacket packet(v, BChannelPacketType::cPausePacket);
   HRESULT hr = mMPGame->sendSimPacket(packet);
   if(FAILED(hr))
      nlog(cMPGameCL, "BMPSimObject::setPaused -- failed to send sim packet. hr: 0x%x", hr);
}

//==============================================================================
// 
DWORD BMPSimObject::advanceGameTime(void)
{
   if (mMPGame && mMPGame->getSession() && mMPGame->getSession()->getTimeSync())
      return(mMPGame->getSession()->getTimeSync()->advanceGameTime());
   else
      return 0;
}

//==============================================================================
// BMPSimObject::getActiveClientCount
//==============================================================================
long BMPSimObject::getActiveClientCount(void) const
{
   return(mMPGame->getPlayerCount());
}

//==============================================================================
// BMPSimObject::getTimingCounters
//==============================================================================
void BMPSimObject::getTimingCounters(DWORD &compensationAmount, DWORD &compensationInterval, DWORD &sendUpdateInterval, 
                                     DWORD &actualSendUpdateInterval, DWORD &pingApprox, DWORD &networkStall, 
                                     DWORD &totalSendInterval, BYTE &sessionRecentTiming, BYTE &localRecentTiming)
{

   if (!mMPGame || !mMPGame->getSession() || !mMPGame->getSession()->getTimeSync())
   {
      compensationAmount = 0;
      compensationInterval = 0;
      sendUpdateInterval = 0;
      actualSendUpdateInterval = 0;
      pingApprox = 0;
      networkStall = 0;
      totalSendInterval = 0;
      sessionRecentTiming = 0;
      localRecentTiming = 0;
      return;
   }

   BTimeSync *timesync = mMPGame->getSession()->getTimeSync();

   compensationAmount = timesync->getCompensationAmount();
   compensationInterval = timesync->getCompensationInterval();
   sendUpdateInterval = timesync->getSendUpdateInterval();
   actualSendUpdateInterval = timesync->getActualSendInterval();
   pingApprox = timesync->getPingApproximation();
   networkStall = timesync->getNetworkStall();
   totalSendInterval = sendUpdateInterval+pingApprox+networkStall;   
   sessionRecentTiming = (unsigned char)timesync->mRecvUpdateInterval;
   if (mMPGame->getSession()->getLocalClient())
      localRecentTiming = mMPGame->getSession()->getLocalClient()->getTimingHistory()->getRecentTiming();
   else
      localRecentTiming = 0;
}

//==============================================================================
// eof: mpsimobject.cpp
//==============================================================================
