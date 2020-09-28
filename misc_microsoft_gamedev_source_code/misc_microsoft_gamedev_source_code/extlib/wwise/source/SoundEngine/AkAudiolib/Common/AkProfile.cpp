/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

/***************************************************************************************************
**
** AkProfile.cpp
**
***************************************************************************************************/
#include "stdAfx.h"
#include "AkProfile.h"

#include <limits.h>

#include "AkLEngine.h"
#include "AkMath.h"
#include "AkMonitor.h"
#include "AkAudioMgr.h"         // For upper engine access.
#include "AkURenderer.h"        // For lower engine access.
#include "AkAudiolibTimer.h"
#include "AkTransitionManager.h"
#include "AkRegistryMgr.h"
#include "AkSink.h"
#include "AkAudioLib.h"
#include "AkCritical.h"
#include "AkFeedbackMgr.h"

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

// AK_PROFILE_PERF_INTERVALS in milliseconds
#define AK_PROFILE_PERF_INTERVALS 200
#define AK_PROFILE_CURSOR_UPDATE_INTERVALS 50

//-----------------------------------------------------------------------------
// Structs.
//-----------------------------------------------------------------------------
struct AkDeviceData
{
    AK::IAkDeviceProfile *  pDevice;
    unsigned int        uiNumStreams;
};

/***************************************************************************************************
**
** AkPerf
**
***************************************************************************************************/

AkUInt32 AkPerf::m_ulPerfInterval = AK_PROFILE_PERF_INTERVALS;
AkUInt32 AkPerf::m_ulCursorPositionInterval = AK_PROFILE_CURSOR_UPDATE_INTERVALS;
AkInt64	AkPerf::m_iLastUpdateAudio = {0};
AkInt64 AkPerf::m_iLastUpdateCursorPosition = {0};

AkInt64 AkPerf::m_iLastUpdatePlugins = {0};
AkInt64 AkPerf::m_iLastUpdateMemory = {0};
AkInt64 AkPerf::m_iLastUpdateStreaming = {0};
AkInt64 AkPerf::m_iLastUpdatePipeline = {0};

AkUInt32 AkPerf::m_ulPreparedEvents = 0;
AkUInt32 AkPerf::m_ulBankMemory = 0;
AkUInt32 AkPerf::m_ulPreparedEventMemory = 0;

extern CAkBusCtx g_MasterBusCtx;

void AkPerf::Init()
{
// To let an easy way for the WWise dev to adjust 
// perf rate, a registry key has been added
// It is facultative
#if defined(WIN32)
	{
		HKEY hKey = 0;
		DWORD dwBufLen = sizeof(m_ulPerfInterval);
		DWORD dwDisp = 0; 
        		
		LONG lRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Audiokinetic Inc.\\Audiolib\\PerfRate",
			0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
 
		if( lRet != ERROR_SUCCESS )
		{
			AKASSERT(!"Error while reading  Performance rate registry");
			return;
		}
		if(dwDisp == REG_OPENED_EXISTING_KEY)
		{
			lRet = RegQueryValueEx( hKey, L"PerfRate", 0, NULL,
			(BYTE*)(&(m_ulPerfInterval)), &dwBufLen);
			if( (lRet != ERROR_SUCCESS) || dwBufLen != sizeof(m_ulPerfInterval) )
			{
				AKASSERT(!"Error while reading  Performance rate registry");
				return;
			}
			if( m_ulPerfInterval == 0 )
			{
				m_ulPerfInterval = AK_PROFILE_PERF_INTERVALS;
			}
		}
		else
		{
			lRet = RegSetValueEx( hKey, L"PerfRate", 0, REG_DWORD, (BYTE*)(&(m_ulPerfInterval)), sizeof(m_ulPerfInterval) );
		}

		RegCloseKey( hKey );
	}
#endif

}

void AkPerf::Term()
{

}

void AkPerf::TickAudio()
{
	if ( !AkMonitor::GetNotifFilter() )
		return;

	// Don't do anything if it's not time yet.	
	AkInt64 iNow;
	AKPLATFORM::PerformanceCounter( &( iNow ) );
	
	AkReal32 fIntervalCursorPosition =	(AkReal32) ( ( iNow - m_iLastUpdateCursorPosition ) / AK::g_fFreqRatio );
	if ( fIntervalCursorPosition < m_ulCursorPositionInterval )
		return;

	// Protect access to Upper Engine data -- Might not be necessary, but better be safe.
	CAkFunctionCritical GlobalLock;

	AkReal32 fInterval =				(AkReal32) ( ( iNow - m_iLastUpdateAudio ) / AK::g_fFreqRatio );

	bool bDoAudioPerf = ( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataAudioPerf ) && fInterval >= m_ulPerfInterval;

	//Stamp the feedback timer if we need it either with audio or feedback perf data.
	bool bDoFeedbackPerf = ( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback ) && fInterval >= m_ulPerfInterval;
	if (bDoAudioPerf || bDoFeedbackPerf)
		AkAudiolibTimer::timerFeedback.Stamp();

	// Don't do anything if nobody's listening to perf.
	if ( bDoAudioPerf )
	{
		// Post primary lower thread info.
		{
			AkUInt32 sizeofData = offsetof( AkMonitorData::MonitorDataItem, audioPerfData ) + sizeof( AkMonitorData::AudioPerfMonitorData );
			AkProfileDataCreator creator( sizeofData );
			if ( !creator.m_pData )
				return;

			AkAudiolibTimer::timerAudio.Stamp();

			creator.m_pData->eDataType = AkMonitorData::MonitorDataAudioPerf;
			creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

			creator.m_pData->audioPerfData.uiNotifFilter = AkMonitor::GetNotifFilter();

			// voice info
			g_pTransitionManager->GetTransitionsUsage( 
				creator.m_pData->audioPerfData.numFadeTransitionsUsed, 
				creator.m_pData->audioPerfData.maxFadeNumTransitions,
				creator.m_pData->audioPerfData.numStateTransitionsUsed, 
				creator.m_pData->audioPerfData.maxStateNumTransitions 
				);

			// global performance timers
			creator.m_pData->audioPerfData.numRegisteredObjects = (AkUInt16)( g_pRegistryMgr->NumRegisteredObject() );

			creator.m_pData->audioPerfData.timers.fInterval = fInterval;
			creator.m_pData->audioPerfData.timers.fAudioThread = AkAudiolibTimer::timerAudio.Millisecs();
		
			//Remove the portion that belongs to the feedback pipeline
			creator.m_pData->audioPerfData.timers.fAudioThread -= AkAudiolibTimer::timerFeedback.Millisecs();
		
			// Message queue stats
			creator.m_pData->audioPerfData.uCommandQueueActualSize     = g_pAudioMgr->GetActualQueueSize();
			creator.m_pData->audioPerfData.fCommandQueuePercentageUsed = g_pAudioMgr->GetPercentageQueueFilled()*100;

			// Update DSP usage
#ifdef RVL_OS
			creator.m_pData->audioPerfData.fDSPUsage = AXGetDspCycles() / (AkReal32)AXGetMaxDspCycles();
#else
			creator.m_pData->audioPerfData.fDSPUsage = 0;
#endif

			creator.m_pData->audioPerfData.uNumPreparedEvents	= m_ulPreparedEvents;
			creator.m_pData->audioPerfData.uTotalMemBanks		= m_ulBankMemory;
			creator.m_pData->audioPerfData.uTotalPreparedMemory	= m_ulPreparedEventMemory;
			creator.m_pData->audioPerfData.uTotalMediaMemmory	= m_ulPreparedEventMemory + m_ulBankMemory;
		}

		// Auxiliaries, sorted in order of importance. last ones are more likely to be skipped.

		PostMemoryStats( iNow );
		PostPluginTimers( iNow );
		PostStreamingStats( iNow );
		PostPipelineStats( iNow );
		PostFeedbackStats(fInterval);
		PostEnvironmentStats();
		PostObsOccStats();
		PostListenerStats();
		PostControllerStats();
		PostFeedbackDevicesStats();
		PostOutputStats();
		PostGameObjPositions();
		PostWatchGameSyncValues();

		m_iLastUpdateAudio = iNow;
	}

	// This is not perf information, we have to send it to Wwise, even if it is not listening specifically to perfs.
	// It is used to post position of playing segments at regular time intervals.
	PostInteractiveMusicInfo();

	m_iLastUpdateCursorPosition = iNow;
}

void AkPerf::PostPluginTimers( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataPluginTimer ) )
		return;

	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, pluginTimerData.pluginData ) 
		+ AkAudiolibTimer::g_PluginTimers.Length() * sizeof( AkMonitorData::PluginTimerData );
		
	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataPluginTimer;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->pluginTimerData.ulNumTimers = AkAudiolibTimer::g_PluginTimers.Length();
    creator.m_pData->pluginTimerData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdatePlugins ) / AK::g_fFreqRatio );

	int iPlugin = 0;
	for ( AkAudiolibTimer::AkTimerMap::Iterator it = AkAudiolibTimer::g_PluginTimers.Begin(); it != AkAudiolibTimer::g_PluginTimers.End(); ++it )
	{
		AkMonitorData::PluginTimerData & data = creator.m_pData->pluginTimerData.pluginData[ iPlugin++ ];

		(*it).item.timer.Stamp();

		data.uiPluginID = (*it).key;
		data.fMillisecs = (*it).item.timer.Millisecs();
		data.uiNumInstances = (*it).item.uNumInstances;
	}

	m_iLastUpdatePlugins = in_iNow;
}

void AkPerf::PostMemoryStats( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataMemoryPool ) )
		return;

	AkUInt32 ulNumPools = AK::MemoryMgr::GetMaxPools();
	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, memoryData.poolData ) 
		+ ulNumPools * sizeof( AkMonitorData::MemoryPoolData );
		
	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataMemoryPool;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->memoryData.ulNumPools = ulNumPools;

	for ( AkUInt32 ulPool = 0; ulPool < ulNumPools; ++ulPool )
	{
		MemoryMgr::PoolStats & stats = ((MemoryMgr::PoolStats *) creator.m_pData->memoryData.poolData)[ ulPool ];
        MemoryMgr::GetPoolStats( ulPool, stats );
	}

	m_iLastUpdateMemory = in_iNow;
}

void AkPerf::PostStreamingStats( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & ( AkMonitorData::MonitorDataStreaming | AkMonitorData::MonitorDataStreamsRecord | AkMonitorData::MonitorDevicesRecord ) ) )
		return;

    // ALGORITHM:
    // Try to reserve room needed to send new device records.
    // If succeeded, get them and send IF ANY.
    // Try to reserve room needed to send new streams records.
    // If succeeded, get them and send IF ANY.
    // Try yo reserve room to send stream data.
    // If succeeded, get them and send ALWAYS.

    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( pStmMgrProfile == NULL )
        return; // Profiling interface not implemented in that stream manager.


    unsigned int uiNumDevices = pStmMgrProfile->GetNumDevices( );

    AkDeviceData * pDevices = (AkDeviceData*)alloca( sizeof( AkDeviceData ) * uiNumDevices );

    unsigned int uiNumStreams = 0;

    for ( unsigned int uiDevice=0; uiDevice<uiNumDevices; uiDevice++ )
    {
        AK::IAkDeviceProfile * pDevice = pDevices[uiDevice].pDevice = pStmMgrProfile->GetDeviceProfile( uiDevice );
        AKASSERT( pDevice != NULL );

        // Get record if new.
        if ( pDevice->IsNew( ) )
        {
            // Send every new device.
            AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, deviceRecordData )
                + sizeof( AkMonitorData::DeviceRecordMonitorData );

            AkProfileDataCreator creator( sizeofItem );
	        if ( !creator.m_pData )
		        return;

            // Can send. Get actual data.
            creator.m_pData->eDataType = AkMonitorData::MonitorDevicesRecord;
	        creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

            pDevice->GetDesc( creator.m_pData->deviceRecordData );
            pDevice->ClearNew( );

            // Send now.
        }

        unsigned int uiNumStmsDevice = pDevice->GetNumStreams( );

        // Store.
        uiNumStreams += pDevices[uiDevice].uiNumStreams = uiNumStmsDevice;

        // Get number of new streams.
        unsigned int uiNumNewStreams = 0;
        IAkStreamProfile * pStmProfile;
        for ( unsigned int uiStm = 0; uiStm < uiNumStmsDevice; ++uiStm )
	    {
            pStmProfile = pDevice->GetStreamProfile( uiStm );
            AKASSERT( pStmProfile != NULL );

            // Get record if new.
            if ( pStmProfile->IsNew( ) )
            {
                ++uiNumNewStreams;
            }
	    }

        // Send new stream records if any.
        if ( uiNumNewStreams > 0 )
        {
            // Compute size needed.
            long sizeofItem = offsetof( AkMonitorData::MonitorDataItem, streamRecordData.streamRecords )
                + uiNumNewStreams * sizeof( AkMonitorData::StreamRecord );

            AkProfileDataCreator creator( sizeofItem );
	        if ( !creator.m_pData )
                return;

            // Can send. Get actual data.
            creator.m_pData->eDataType = AkMonitorData::MonitorDataStreamsRecord;
	        creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	        creator.m_pData->streamRecordData.ulNumNewRecords = uiNumNewStreams;

            AkUInt32 ulNewStream = 0;

	        for ( unsigned int uiStm = 0; uiStm < uiNumStmsDevice; ++uiStm )
	        {
                pStmProfile = pDevice->GetStreamProfile( uiStm );
                AKASSERT( pStmProfile != NULL );

                // Get record if new.
                if ( pStmProfile->IsNew( ) )
                {
                    pStmProfile->GetStreamRecord( creator.m_pData->streamRecordData.streamRecords[ ulNewStream ] );
                    pStmProfile->ClearNew( );
                    ulNewStream++;
                }
	        }

            // Send new streams now.
        }
    }

    // Send all streams' data, even if none.

    {
        AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, streamingData.streamData )
                        + uiNumStreams * sizeof( AkMonitorData::StreamData );

        AkProfileDataCreator creator( sizeofItem );
	    if ( !creator.m_pData )
		    return;

        creator.m_pData->eDataType = AkMonitorData::MonitorDataStreaming;
	    creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	    creator.m_pData->streamingData.ulNumStreams = uiNumStreams;
        creator.m_pData->streamingData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdateStreaming ) / AK::g_fFreqRatio );

        IAkStreamProfile * pStmProfile;
        for ( unsigned int uiDevice=0; uiDevice<uiNumDevices; uiDevice++ )
        {
	        for ( unsigned int ulStm = 0; ulStm < pDevices[uiDevice].uiNumStreams; ++ulStm )
	        {
                pStmProfile = pDevices[uiDevice].pDevice->GetStreamProfile( ulStm );
                AKASSERT( pStmProfile != NULL );
                pStmProfile->GetStreamData( creator.m_pData->streamingData.streamData[ ulStm ] );
	        }
        }
    }

    m_iLastUpdateStreaming = in_iNow;
}

void AkPerf::PostPipelineStats( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataPipeline ) )
		return;

	bool bIsFeedbackMonitored = ( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback ) != 0;
	AkUInt16 uFeedbackPipelines = 0;
	AkUInt16 uAudioPipelines = 0;
	CAkLEngine::GetNumPipelines(uAudioPipelines, uFeedbackPipelines);
	AkUInt16 uTotal(uAudioPipelines);
	if (bIsFeedbackMonitored)
		uTotal += uFeedbackPipelines;

    AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, pipelineData.pipelines )
                    + uTotal * sizeof( AkMonitorData::PipelineData );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataPipeline;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->pipelineData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdatePipeline ) / AK::g_fFreqRatio );
	creator.m_pData->pipelineData.numPipelines = uTotal;

	//Feedback stuff.  
	CAkFeedbackBus *pMaster = CAkFeedbackBus::GetMasterBus();
	creator.m_pData->pipelineData.numPipelinesFeedback = uFeedbackPipelines;

	AkUInt32 uFXIndex = 0;
	for ( /*Init before*/; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		AkFXDesc masterBusFxDesc;

		g_MasterBusCtx.GetFX( uFXIndex, masterBusFxDesc );
		creator.m_pData->pipelineData.masterBusFxId[uFXIndex] = masterBusFxDesc.EffectTypeID;

		if (pMaster != NULL)
		{
			pMaster->GetFX( uFXIndex, masterBusFxDesc );
			creator.m_pData->pipelineData.feedbackMasterBusFxId[ uFXIndex ] = masterBusFxDesc.EffectTypeID;
		}
		else
		{
			creator.m_pData->pipelineData.feedbackMasterBusFxId[ uFXIndex ] = AK_INVALID_PLUGINID;
		}
	}

#ifdef RVL_OS
	// Actually Wii specific, but coded so that it works on all platforms.( but actually still useless on all others platforms.
	// the difference between AK_NUM_EFFECTS_PROFILER and AK_NUM_EFFECTS_PER_OBJ must be filled with AK_INVALID_PLUGINID
	for ( /*Init before*/; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
	{
		creator.m_pData->pipelineData.masterBusFxId[uFXIndex] = AK_INVALID_PLUGINID;
		creator.m_pData->pipelineData.feedbackMasterBusFxId[uFXIndex] = AK_INVALID_PLUGINID;
	}
#endif

	CAkLEngine::GetPipelineData( creator.m_pData->pipelineData.pipelines, bIsFeedbackMonitored );

	m_iLastUpdatePipeline = in_iNow;
}

void AkPerf::PostEnvironmentStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataEnvironment ) )
		return;

	g_pRegistryMgr->PostEnvironmentStats();
}

void AkPerf::PostObsOccStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataObsOcc ) )
		return;

	g_pRegistryMgr->PostObsOccStats();
}

void AkPerf::PostListenerStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataListeners ) )
		return;

	g_pRegistryMgr->PostListenerStats();
}

void AkPerf::PostControllerStats()
{
#ifdef RVL_OS
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataControllers ) )
		return;

	g_pRegistryMgr->PostControllerStats();
#endif
}

void AkPerf::PostOutputStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataOutput ) )
		return;

	if ( g_pAkSink->m_stats.m_uOutNum == 0 )
		return;

    AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, outputData )
                    + sizeof( AkMonitorData::OutputMonitorData );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataOutput;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->outputData.fOffset = g_pAkSink->m_stats.m_fOutSum / g_pAkSink->m_stats.m_uOutNum;
	creator.m_pData->outputData.fPeak = AkMath::Max( fabs( g_pAkSink->m_stats.m_fOutMin ), fabs( g_pAkSink->m_stats.m_fOutMax ) );
	creator.m_pData->outputData.fRMS = sqrt( g_pAkSink->m_stats.m_fOutSumOfSquares / g_pAkSink->m_stats.m_uOutNum );

	g_pAkSink->m_stats.m_fOutMin = (AkReal32) INT_MAX;
	g_pAkSink->m_stats.m_fOutMax = (AkReal32) INT_MIN;
	g_pAkSink->m_stats.m_fOutSum = 0;
	g_pAkSink->m_stats.m_fOutSumOfSquares = 0;
	g_pAkSink->m_stats.m_uOutNum = 0;
}

void AkPerf::PostGameObjPositions()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataGameObjPosition ) )
		return;

	AkMonitor::PostWatchedGameObjPositions();
}

void AkPerf::PostWatchGameSyncValues()
{
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataRTPCValues ) )
		return;

	AkMonitor::PostWatchesRTPCValues();
}

void AkPerf::PostInteractiveMusicInfo()
{
	extern AkExternalProfileHandlerCallback g_pExternalProfileHandlerCallback;
	if( g_pExternalProfileHandlerCallback )
		g_pExternalProfileHandlerCallback();
}

void AkPerf::PostFeedbackStats(AkReal32 in_fInterval)
{
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback ) || pMgr == NULL )
		return;

	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, feedbackData ) + sizeof(AkMonitorData::FeedbackMonitorData);

	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataFeedback;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();
	creator.m_pData->feedbackData.timer.fInterval = in_fInterval;
	creator.m_pData->feedbackData.timer.fAudioThread = AkAudiolibTimer::timerFeedback.Millisecs();
	creator.m_pData->feedbackData.fPeak = AkMath::FastLinTodB(pMgr->GetPeak());
}

void AkPerf::PostFeedbackDevicesStats()
{
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if ( !( AkMonitor::GetNotifFilter() & AkMonitorData::MonitorDataFeedback ) || pMgr == NULL )
		return;

	pMgr->PostDeviceMonitorData();
	
	g_pRegistryMgr->PostfeedbackGameObjStats();
}
