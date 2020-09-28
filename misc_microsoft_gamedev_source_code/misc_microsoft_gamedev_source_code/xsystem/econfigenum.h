//==============================================================================
// econfigenum.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma  once

#ifndef __ECONFIGENUM_H__
#define __ECONFIGENUM_H__

#include "config.h"

//==============================================================================
// START CONFIGS
//==============================================================================
extern const long& cConfigEnableXMB;
   
#ifndef BUILD_FINAL
   extern const long& cConfigLogChunker;
   extern const long& cConfigDeveloper;
   extern const long& cConfigDontTrackPerf;
   extern const long& cConfigUseAlternateLog;
   extern const long& cConfigLogFileOpens;
   
   extern const long& cConfigNoDebugSpew;
   extern const long& cConfigNoAssert;
   extern const long& cConfigNoAssertDialog;
   extern const long& cConfigMemoryStackHeapSize;
   extern const long& cConfigSystemLogging;
   extern const long& cConfigCloseLogOnWrite;
   extern const long& cConfigFlushLogOnWrite;
   extern const long& cConfigTraceLog;
   extern const long& cConfigEnableTimelineProfiler;
   extern const long& cConfigAllocationLogging;
   extern const long& cConfigAllocationLoggingSimOnly;
   extern const long& cConfigAllocationLoggingRemote;

   // XBox Live/MP Debug
   extern const long& cConfigIgnoreMPChecksum;
   extern const long& cConfigNoLive;
   extern const long& cConfigNoLiveMenu;
   extern const long& cConfigNoVoice;
   extern const long& cConfigFixedMultiplayerTiming;
   extern const long& cConfigAllowMMTestingHacks;
   extern const long& cConfigMMFilterHackCode;
   extern const long& cConfigVoiceSampleInterval;
   extern const long& cConfigForceRestrictedNAT;
   extern const long& cConfigMoreLiveOutputSpam;

   extern const long& cConfigClientTimeHistory;
   extern const long& cConfigFlushSendBuffers;

   extern const long& cConfigLanDiscoveryPort;

   extern const long& cConfigNoQOSUpdatesToUI;
   
   // Fail Report
   extern const long& cConfigDumpDirectory;
   extern const long& cConfigDisableWatermark;
#endif

extern const long& cConfigLogToCacheDrive;
extern const long& cConfigCompressLogs;
extern const long& cConfigAlpha;
extern const long& cConfigGrannySampleAnimCache;
extern const long& cConfigNoParticles;

// xnetwork configs
extern const long& cConfigProxySupport;
extern const long& cConfigProxyRequestTimeout;
extern const long& cConfigProxyPingTimeout;
extern const long& cConfigNumDisconnectSends;
extern const long& cConfigSocketUnresponsiveTimeout;
extern const long& cConfigJoinRequestTimeout;

//==============================================================================
// END CONFIGS
//==============================================================================

//==============================================================================
#endif // __ECONFIGENUM_H__
