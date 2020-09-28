//==============================================================================
// mpcommheaders.cpp
//
// Copyright (c) 2003; Ensemble Studios
//==============================================================================

#include "Common.h"
#include "commlog.h"
#include "mpcommheaders.h"


   DEFINE_COMMHEADER(cTransportCL);
   DEFINE_COMMHEADER(cGameTimingCL);
   DEFINE_COMMHEADER(cSyncCL);
   DEFINE_COMMHEADER(cLowLevelSyncCL);
   DEFINE_COMMHEADER(cBroadcasterCL);
   DEFINE_COMMHEADER(cReceiverCL);
   DEFINE_COMMHEADER(cMPGameCL);
   DEFINE_COMMHEADER(cMPGameSettingsCL);
   DEFINE_COMMHEADER(cMPStorageCL);
   DEFINE_COMMHEADER(cMPPerfCL);
   DEFINE_COMMHEADER(cMPMatchmakingCL);
   DEFINE_COMMHEADER(cMPPartySystemCL);
   DEFINE_COMMHEADER(cMPVoiceCL);
   DEFINE_COMMHEADER(cMPModeMenuCL);
   DEFINE_COMMHEADER(cLSPCL);

// don't for get to setup an config in the config enum file for the log.

void registerMPCommHeaders(void)
{
   DECLARE_COMMHEADER(cTransportCL, "XPORTCL", "transportCL");
   DECLARE_COMMHEADER(cGameTimingCL, "GTIMECL", "gameTimingCL");
   DECLARE_COMMHEADER(cSyncCL, "SYNCCL", "syncCL");
   DECLARE_COMMHEADER(cLowLevelSyncCL, "LLSYNCCL", "lowLevelSyncCL");
   DECLARE_COMMHEADER(cBroadcasterCL, "BROADCL", "broadcasterCL");
   DECLARE_COMMHEADER(cReceiverCL, "RECVCL", "receiverCL");
   DECLARE_COMMHEADER(cMPGameCL, "MPGAMECL", "mpGameCL");
   DECLARE_COMMHEADER(cMPGameSettingsCL, "MPGAMESETTINGSCL", "mpGameSettingsCL");
   DECLARE_COMMHEADER(cMPStorageCL, "MPSTORAGECL", "mpStorageCL");
   DECLARE_COMMHEADER(cMPPerfCL, "MPPERFCL", "mpPerfCL");
   DECLARE_COMMHEADER(cMPMatchmakingCL, "MPMATCHMAKINGCL", "mpMatchmakingCL");
   DECLARE_COMMHEADER(cMPPartySystemCL, "MPPARTYSYSTEMCL", "mpPartySystemCL");
   DECLARE_COMMHEADER(cMPVoiceCL, "MPVOICECL", "mpVoiceCL");
   DECLARE_COMMHEADER(cMPModeMenuCL, "MPMENUCL", "mpModeMenuCL");
   DECLARE_COMMHEADER(cLSPCL, "LSPCL", "LSPCL");
}


