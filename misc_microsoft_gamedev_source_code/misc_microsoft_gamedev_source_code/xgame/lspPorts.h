//==============================================================================
// lspPorts.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

enum
{
   cDefaultLSPDebugUploadPort = 1000,
   cDefaultLSPAuthPort = 1000,
   cDefaultLSPConfigPort = 1000,
   cDefaultLSPPerfPort = 1000,
   cDefaultLSPStatsUploadPort = 1000,
   cDefaultLSPMediaTransferPort = 1000,
   cDefaultLSPTickerPort = 1000,
   cDefaultLSPServiceRecordPort = 1000,
};

//enum
//{
//   cServiceIdAuth = 102,
//   cServiceIdConfig = 103,
//   cServiceIdPerf = 104,
//   cServiceIdStats = 105,
//   cServiceIdMedia = 106,
//   cServiceIdDebug = 100,
//};

enum
{
   cDefaultLSPAuthServiceID = 100,
   cDefaultLSPConfigServiceID = 101,
   cDefaultLSPPerfServiceID = 102,
   cDefaultLSPStatsServiceID = 103,
   cDefaultLSPMediaServiceID = 104,
   cDefaultLSPTickerServiceID = 105,
   cDefaultLSPServiceRecordServiceID = 106,
   cDefaultLSPDebugServiceID = 120,
};

enum
{
   cDefaultLSPTCPTimeout = 10*1000,
   cDefaultLSPSGTimeout = 30*1000,
   cDefaultLSPSGConnectTimeout = 10*1000,
   //cDefaultLSPTCPReconnects = 1,
   cDefaultLSPSGTTL = 15*60*1000, // SG connection attempts will default to a 15 minute timeout
   cDefaultLSPFailTTL = 60*1000, // if the XEnumerate, XNetConnect or TCP connect fails, then we'll try again at this TTL
};
