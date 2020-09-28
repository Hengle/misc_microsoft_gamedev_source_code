//==============================================================================
// econfigenum.cpp
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "econfigenum.h"

DEFINE_CONFIG(cConfigEnableXMB);

#ifndef BUILD_FINAL
   DEFINE_CONFIG(cConfigLogChunker);
   DEFINE_CONFIG(cConfigDeveloper);
   DEFINE_CONFIG(cConfigDontTrackPerf);
   DEFINE_CONFIG(cConfigUseAlternateLog);
   DEFINE_CONFIG(cConfigLogFileOpens);
   
   DEFINE_CONFIG(cConfigNoDebugSpew);
   DEFINE_CONFIG(cConfigNoAssert);
   DEFINE_CONFIG(cConfigNoAssertDialog);
   DEFINE_CONFIG(cConfigMemoryStackHeapSize);
   DEFINE_CONFIG(cConfigSystemLogging);
   DEFINE_CONFIG(cConfigCloseLogOnWrite);
   DEFINE_CONFIG(cConfigFlushLogOnWrite);
   DEFINE_CONFIG(cConfigTraceLog);
   DEFINE_CONFIG(cConfigEnableTimelineProfiler);
   DEFINE_CONFIG(cConfigAllocationLogging);
   DEFINE_CONFIG(cConfigAllocationLoggingSimOnly);
   DEFINE_CONFIG(cConfigAllocationLoggingRemote);

   // XBox Live/MP Debug
   DEFINE_CONFIG(cConfigIgnoreMPChecksum);
   DEFINE_CONFIG(cConfigNoLive);
   DEFINE_CONFIG(cConfigNoLiveMenu);
   DEFINE_CONFIG(cConfigNoVoice);
   DEFINE_CONFIG(cConfigFixedMultiplayerTiming);
   DEFINE_CONFIG(cConfigAllowMMTestingHacks);
   DEFINE_CONFIG(cConfigMMFilterHackCode);
   DEFINE_CONFIG(cConfigMoreLiveOutputSpam);
   DEFINE_CONFIG(cConfigForceRestrictedNAT);

   DEFINE_CONFIG(cConfigVoiceSampleInterval);

   DEFINE_CONFIG(cConfigClientTimeHistory);
   DEFINE_CONFIG(cConfigFlushSendBuffers);
   DEFINE_CONFIG(cConfigTimeSyncTimeout);

   DEFINE_CONFIG(cConfigLanDiscoveryPort);

   DEFINE_CONFIG(cConfigNoQOSUpdatesToUI);

   // Fail Report
   DEFINE_CONFIG( cConfigDumpDirectory );
   
   DEFINE_CONFIG(cConfigDisableWatermark);
#endif

DEFINE_CONFIG(cConfigLogToCacheDrive);
DEFINE_CONFIG(cConfigCompressLogs);
DEFINE_CONFIG(cConfigAlpha);
DEFINE_CONFIG(cConfigGrannySampleAnimCache);
DEFINE_CONFIG(cConfigNoParticles);

DEFINE_CONFIG(cConfigProxySupport);
DEFINE_CONFIG(cConfigProxyRequestTimeout);
DEFINE_CONFIG(cConfigProxyPingTimeout);
DEFINE_CONFIG(cConfigNumDisconnectSends);
DEFINE_CONFIG(cConfigSocketUnresponsiveTimeout);
DEFINE_CONFIG(cConfigJoinRequestTimeout);

bool registerEcoreEnums(bool)
{
   DECLARE_CONFIG(cConfigEnableXMB,                "EnableXMB", "Enables XMB file loading", 0, NULL);
   
#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigLogChunker,               "logChunker",      "Controls chunker logging", 0, NULL);
   DECLARE_CONFIG(cConfigDeveloper,                "developer",       "engages some developer-mode cheats (separate from editing cmds)", 0, NULL);
   DECLARE_CONFIG(cConfigDontTrackPerf,            "dontTrackPerf",   "if defined, perf info is not tracked.", 0, NULL);
   DECLARE_CONFIG(cConfigUseAlternateLog,          "useAlternateLog", "if defined, alternate log naming scheme is used -- currently inserts machine name into log.", 0, NULL);
   DECLARE_CONFIG(cConfigLogFileOpens,             "logFileOpens",    "if defined, every file open will be registered into a logfile.", 0, NULL);
   
   DECLARE_CONFIG(cConfigNoDebugSpew,              "noDebugSpew",     "Turn off debug spew from various render and terrain functions", 0, NULL);
   DECLARE_CONFIG(cConfigNoAssert,                 "noAssert",                      "", 0, NULL);
   DECLARE_CONFIG(cConfigNoAssertDialog,           "NoAssertDialog",                "", 0, NULL);
   DECLARE_CONFIG(cConfigMemoryStackHeapSize,      "memoryStackHeapSize",           "", 0, NULL);
   DECLARE_CONFIG(cConfigSystemLogging,            "SystemLogging",                 "", 0, NULL);
   DECLARE_CONFIG(cConfigCloseLogOnWrite,          "CloseLogOnWrite",               "", 0, NULL);
   DECLARE_CONFIG(cConfigFlushLogOnWrite,          "FlushLogOnWrite",               "", 0, NULL);
   DECLARE_CONFIG(cConfigTraceLog,                 "TraceLog",                      "", 0, NULL);
   DECLARE_CONFIG(cConfigEnableTimelineProfiler,   "EnableTimelineProfiler",        "", 0, NULL);
   DECLARE_CONFIG(cConfigAllocationLogging,        "AllocationLogging",             "", 0, NULL);   
   DECLARE_CONFIG(cConfigAllocationLoggingSimOnly, "AllocationLoggingSimOnly",      "Still need to define AllocationLogging to turn on logging.", 0, NULL);   
   DECLARE_CONFIG(cConfigAllocationLoggingRemote, "AllocationLoggingRemote",      "Enables Remote Logging : Still need to define AllocationLogging to turn on logging.", 0, NULL);   

   // XBox Live/MP Debug
   DECLARE_CONFIG(cConfigIgnoreMPChecksum,		    "NoCrc", "", 0, NULL );
   DECLARE_CONFIG(cConfigNoLive,                    "NoLive", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoLiveMenu,                "NoLiveMenu", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoVoice,                   "NoVoice", "", 0, NULL);
   DECLARE_CONFIG(cConfigFixedMultiplayerTiming,    "FixedMultiplayerTiming", "", 0, NULL);
   DECLARE_CONFIG(cConfigAllowMMTestingHacks,       "AllowMatchMakingTestingHacks", "", 0, NULL);
   DECLARE_CONFIG(cConfigMMFilterHackCode,          "MatckMakingFilterHackCode", "", 0, NULL);   
   DECLARE_CONFIG(cConfigVoiceSampleInterval,       "VoiceSampleInterval", "Time in milliseconds between voice network submissions, default is 60", 0, NULL);
   DECLARE_CONFIG(cConfigForceRestrictedNAT,        "ForceRestrictedNAT", "", 0, NULL);
   DECLARE_CONFIG(cConfigMoreLiveOutputSpam,        "LiveOutputSpam", "", 0, NULL); 

   DECLARE_CONFIG(cConfigClientTimeHistory,         "xNet-ClientTimeHistory", "Defines the client timing history size, 0-256", 0, NULL);
   DECLARE_CONFIG(cConfigFlushSendBuffers,          "xNet-FlushSendBuffers", "Prevents the network layer from coalescing data", 0, NULL);

   DECLARE_CONFIG(cConfigLanDiscoveryPort,          "xNet-LanDiscoveryPort", "", 0, NULL);

   DECLARE_CONFIG(cConfigNoQOSUpdatesToUI,          "NoQOSUpdatesToUI", "", 0, NULL);

   // Fail Report
   DECLARE_CONFIG(cConfigDumpDirectory,             "DumpDirectory", "Fail report save directory", 0, NULL );   
   
   DECLARE_CONFIG(cConfigDisableWatermark,          "DisableWatermark", "Disable watermark rendering", 0, NULL);
#endif

   DECLARE_CONFIG(cConfigLogToCacheDrive,           "LogToCacheDrive", "Log manager will save files to the cache drive", 0, NULL );   
   DECLARE_CONFIG(cConfigCompressLogs,              "CompressLogs",                  "", 0, NULL);
   DECLARE_CONFIG(cConfigAlpha,                     "AlphaTest", "", 0, NULL);
   DECLARE_CONFIG(cConfigGrannySampleAnimCache,     "GrannySampleAnimCache", "", 0, NULL);
   DECLARE_CONFIG(cConfigNoParticles,               "NoParticles", "", 0, NULL);

   DECLARE_CONFIG(cConfigProxySupport,              "xNet-ProxySupport", "Enables proxy support to route around dead connections", 0, NULL);
   DECLARE_CONFIG(cConfigProxyRequestTimeout,       "xNet-ProxyRequestTimeout", "The time to wait for a proxy request to complete", 0, NULL);
   DECLARE_CONFIG(cConfigProxyPingTimeout,          "xNet-ProxyPingTimeout", "The time to wait for a proxy ping to complete", 0, NULL);
   DECLARE_CONFIG(cConfigNumDisconnectSends,        "xNet-NumDisconnectSends", "The number of disconnect packets to spam per client when disconnecting", 0, NULL);
   DECLARE_CONFIG(cConfigSocketUnresponsiveTimeout, "SocketUnresponsiveTimeout", "Timeout for lack of any data received by a connection", 0, NULL);
   DECLARE_CONFIG(cConfigJoinRequestTimeout,        "xNet-JoinRequestTimeout", "Timeout for joining a network game", 0, NULL);

   return true;
}

// This causes xcore to call registerGameRenderConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterECoreConfigs[] = { registerEcoreEnums };
#pragma data_seg() 
