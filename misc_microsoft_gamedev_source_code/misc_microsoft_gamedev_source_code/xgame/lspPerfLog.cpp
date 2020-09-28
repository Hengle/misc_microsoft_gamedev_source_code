//==============================================================================
// lspPerfLog.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h" // PCH
#include "configsgame.h"
#include "lspManager.h"
#include "lspPerfLog.h"
#include "NetPackets.h"
#include "mpcommheaders.h" // logging
#include "commlog.h" // logging

//==============================================================================
// 
//==============================================================================
BLSPPerfLog::BLSPPerfLog() :
   BLSPTitleServerConnection(cDefaultLSPPerfPort, cConfigLSPPerfReportingPort, cDefaultLSPPerfServiceID, cConfigLSPPerfServiceID, 0),
   mOutboundPacket(getServiceID()),
   mUnsentRecord(false),
   mWaitingOnResponse(false)
{
   nlog(cLSPCL, "BLSPPerfLog::BLSPPerfLog - Object created");
}

//==============================================================================
// 
//==============================================================================
BLSPPerfLog::~BLSPPerfLog()
{
   nlog(cLSPCL, "BLSPPerfLog::~BLSPPerfLog - Object destroyed");
   shutdown();
}

//==============================================================================
// 
//==============================================================================
void BLSPPerfLog::connected(BXStreamConnection& connection)
{
   BLSPTitleServerConnection::connected(connection);

   if (mUnsentRecord)
   {
      nlog(cLSPCL, "BLSPPerfLog::connected - Sending perf data packet");
      send(mOutboundPacket);

      mWaitingOnResponse = true;
      mUnsentRecord = false;
   }
   else
   {
      nlog(cLSPCL, "BLSPPerfLog::connected - connected, but no data waiting");
   }
   setState(cStatePendingDone);
}

//==============================================================================
// 
//==============================================================================
void BLSPPerfLog::postPerfData(BPerfLogPacketV2* pDataPacket)
{
   if (pDataPacket == NULL)
      return;

   nlog(cLSPCL, "BLSPPerfLog::postPerfData - Request to log a perf packet");
   mOutboundPacket = *pDataPacket;
   mOutboundPacket.setServiceId(getServiceID());
   mUnsentRecord = true;
   connect();
}
