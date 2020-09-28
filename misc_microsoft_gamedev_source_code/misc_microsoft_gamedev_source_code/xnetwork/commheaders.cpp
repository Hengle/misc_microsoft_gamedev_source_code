//==============================================================================
// commheaders.cpp
//
// Copyright (c) 2003; Ensemble Studios
//==============================================================================

#include "precompiled.h"
#include "commlog.h"
#include "commheaders.h"


   DEFINE_COMMHEADER(cSessionNL);
   DEFINE_COMMHEADER(cLiveSessionNL);
   DEFINE_COMMHEADER(cTransportNL);
   DEFINE_COMMHEADER(cTimeSyncNL);
   DEFINE_COMMHEADER(cGameTimingNL);
   DEFINE_COMMHEADER(cConnectivityNL);
   DEFINE_COMMHEADER(cTimingNL);
   DEFINE_COMMHEADER(cBandwidthNL);
   DEFINE_COMMHEADER(cPerfNL);
   DEFINE_COMMHEADER(cReliableConnNL);
   DEFINE_COMMHEADER(cSessionConnAddrNL);
   DEFINE_COMMHEADER(cSessionConnJoinNL);
   DEFINE_COMMHEADER(cFileTransferNL);
   DEFINE_COMMHEADER(cBandwidthTotalsNL);


void registerCommHeaders(void)
{
   DECLARE_COMMHEADER(cSessionNL, "SESSIONL", "sessionCL");
   DECLARE_COMMHEADER(cLiveSessionNL, "LIVENL", "liveSessionCL");
   DECLARE_COMMHEADER(cTransportNL, "XPORTNL", "transportCL");
   DECLARE_COMMHEADER(cTimeSyncNL, "TSYNCNL", "timeSyncCL");
   DECLARE_COMMHEADER(cGameTimingNL, "GTIMENL", "gameTimingCL");
   DECLARE_COMMHEADER(cConnectivityNL, "CONNNL", "connectivityCL");
   DECLARE_COMMHEADER(cTimingNL, "TIMINGNL", "timingCL");
   DECLARE_COMMHEADER(cBandwidthNL, "BANDWDNL", "bandwidthCL");
   DECLARE_COMMHEADER(cPerfNL, "PERFNL", "perfCL");
   DECLARE_COMMHEADER(cReliableConnNL, "RELICONN", "reliableConnCL");
   DECLARE_COMMHEADER(cSessionConnAddrNL, "SECONNADDR", "sessionConnAddrCL");
   DECLARE_COMMHEADER(cSessionConnJoinNL, "SECONNJOIN", "sessionConnJoinCL");
   DECLARE_COMMHEADER(cFileTransferNL, "FILEXFER", "fileTransferCL");
   DECLARE_COMMHEADER(cBandwidthTotalsNL, "BANDSTATS", "bandwidthTotalsNL");
}

