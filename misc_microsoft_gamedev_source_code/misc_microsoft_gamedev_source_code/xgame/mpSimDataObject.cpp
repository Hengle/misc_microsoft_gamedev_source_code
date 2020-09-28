//==============================================================================
// mpSimDataObject.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

// Includes
#include "Common.h"
#include "mpSimDataObject.h"
#include "mpGameSession.h"
#include "command.h"

#include "netpackets.h"
#include "mpcommheaders.h"
#include "commlog.h"

//==============================================================================
// 
//==============================================================================
BMPSimDataObject::BMPSimDataObject() : 
   mpMPSession(NULL),
   mpPauseHandler(NULL),
   mpTimingHandler(NULL),
   mpPlayerHandler(NULL)
{
}

//==============================================================================
// 
//==============================================================================
BMPSimDataObject::BMPSimDataObject(BMPGameSession* pSessionObject) :
   mpMPSession(pSessionObject),
   mpPauseHandler(NULL),
   mpTimingHandler(NULL),
   mpPlayerHandler(NULL)
{
}

//==============================================================================
// 
//==============================================================================
void BMPSimDataObject::init(BMPGameSession* pSessionObject)
{
   mpMPSession = pSessionObject;
}

//==============================================================================
// 
void BMPSimDataObject::sendCommand(BChannelPacket& commandPacket)
{
   //SCOPEDSAMPLE(BMPSimDataObject_sendCommand);
   HRESULT hr = mpMPSession->sendCommandPacket(commandPacket);
   if(FAILED(hr))
      nlog(cMPGameCL, "BMPSimDataObject::sendCommand -- failed to send command packet. hr: 0x%x", hr);
}

//==============================================================================
// 
void BMPSimDataObject::commandDataReceived(BMachineID fromMachineID, const void* pData, DWORD size)
{
   nlog(cTransportCL, "deserializing command - type[%d] from machineID[%d]", BCommand::getCommandType(pData), fromMachineID);

   //SCOPEDSAMPLE(BMPSimDataObject_commandDataReceived);
   mObserverList.commandReceived(pData, size);
}

//==============================================================================
// 
void BMPSimDataObject::simDataReceived(const void* pData, DWORD size)
{
   if (BChannelPacket::getType(pData) == BChannelPacketType::cPausePacket)
   {
      if (!mpPauseHandler)
         return;

      BMessagePacket packet(BChannelPacketType::cPausePacket);
      packet.deserializeFrom(pData, size);

      nlog(cTransportCL, "pause command - message[%d] from playerID[%d]", packet.mMessage, packet.mPlayerID);

      if (packet.mMessage > 0)
      {
         mpPauseHandler->netSetPaused(true, packet.mPlayerID);
      }
      else
         mpPauseHandler->netSetPaused(false, packet.mPlayerID);
   }
   else if (BChannelPacket::getType(pData) == BChannelPacketType::cSingleStepPacket)
   {
      if (!mpPauseHandler)
         return;

      BMessagePacket packet(BChannelPacketType::cSingleStepPacket);
      packet.deserializeFrom(pData, size);

      nlog(cTransportCL, "single step command - message[%d]", packet.mMessage);

      mpPauseHandler->netSingleStep();
   }
}

//==============================================================================
// 
void BMPSimDataObject::playerDisconnected(PlayerID playerID, bool userInitiated)
{
   if (mpPlayerHandler)
      mpPlayerHandler->playerDisconnected(playerID, userInitiated);
}

//==============================================================================
// 
void BMPSimDataObject::setPaused(bool v, PlayerID playerID)
{
   BMessagePacket packet(v, BChannelPacketType::cPausePacket, playerID);
   HRESULT hr = mpMPSession->sendSimPacket(packet);
   if(FAILED(hr))
      nlog(cMPGameCL, "BMPSimObject::setPaused -- failed to send sim packet. hr: 0x%x", hr);
}

//==============================================================================
// 
void BMPSimDataObject::singleStep()
{
   BMessagePacket packet(BChannelPacketType::cSingleStepPacket);
   HRESULT hr = mpMPSession->sendSimPacket(packet);
   if(FAILED(hr))
      nlog(cMPGameCL, "BMPSimObject::netSingleStep -- failed to send sim packet. hr: 0x%x", hr);
}

//==============================================================================
// 
DWORD BMPSimDataObject::advanceGameTime()
{   
   if (mpMPSession && mpMPSession->getSession() && mpMPSession->getSession()->getTimeSync())
      return(mpMPSession->getSession()->getTimeSync()->advanceGameTime());
   else
      return 0;
}

//==============================================================================
// BMPSimDataObject::getActiveClientCount
//==============================================================================
long BMPSimDataObject::getActiveClientCount() const
{
   return (mpMPSession->getPlayerCount());
}

//==============================================================================
// BMPSimDataObject::getTimingCounters
//==============================================================================
void BMPSimDataObject::getTimingCounters(DWORD &compensationAmount, DWORD &compensationInterval, DWORD &sendUpdateInterval, 
                                     DWORD &actualSendUpdateInterval, DWORD &pingApprox, DWORD &networkStall, 
                                     DWORD &totalSendInterval, BYTE &sessionRecentTiming, BYTE &localRecentTiming)
{

   if (!mpMPSession || !mpMPSession->getSession() || !mpMPSession->getSession()->getTimeSync())
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

   BTimeSync* pTimesync = mpMPSession->getSession()->getTimeSync();

   compensationAmount = pTimesync->getCompensationAmount();
   compensationInterval = pTimesync->getCompensationInterval();
   sendUpdateInterval = pTimesync->getSendUpdateInterval();
   actualSendUpdateInterval = pTimesync->getActualSendInterval();
   pingApprox = pTimesync->getPingApproximation();
   networkStall = pTimesync->getNetworkStall();
   totalSendInterval = sendUpdateInterval+pingApprox+networkStall;
   sessionRecentTiming = pTimesync->getRecentTiming(pTimesync->getRecvTime());
   localRecentTiming = pTimesync->getRecentTiming(mpMPSession->getSession()->getLocalMachineID());
}
