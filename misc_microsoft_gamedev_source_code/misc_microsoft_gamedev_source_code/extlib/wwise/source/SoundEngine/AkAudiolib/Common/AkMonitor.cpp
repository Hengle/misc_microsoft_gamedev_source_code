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

//////////////////////////////////////////////////////////////////////
//
// AkMonitor.cpp
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#define AK_MONITOR_IMPLEMENT_ERRORCODES // so that AkMonitorError.h defines the table

#include "AkMonitor.h"
#include "IALMonitorSink.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkAudiolibTimer.h"
#include "AkAudioLib.h"
#include "AkRegisteredObj.h"
#include "AkRTPCMgr.h"
#include "AkRegistryMgr.h"
#include "Ak3DListener.h"
#include "AkBankMgr.h"
#include "AkEvent.h"

#include <ctype.h>

extern AkPlatformInitSettings g_PDSettings;

#define MONITOR_SINKS_INITIAL_SIZE			4
#define MONITOR_SINKS_MAX_SIZE				AK_NO_MAX_LIST_SIZE

#define MAX_NOTIFICATIONS_PER_CALL			256

#define SIZEOF_MEMBER(s,m)   sizeof(((s *)0)->m)
#define SIZEOF_MONITORDATA(_x)	(offsetof( AkMonitorData::MonitorDataItem, _x ) + SIZEOF_MEMBER( AkMonitorData::MonitorDataItem, _x ) )

static int CompareNoCase( const char* in_s1, const char* in_s2, int in_n );
static int CompareNoCase( const char* in_s1, const char* in_s2, int in_n )
{
	int c( 0 );
	while( c < in_n )
	{
		if( tolower( in_s1[c] ) != tolower( in_s2[c] ) )
			return !0;

		++c;
	}
	
	return 0;
}

bool DoesGameObjectNameMatch( AkLpCstr in_pszGameObjectWatchName, AkLpCstr in_pszGameObjectName );
bool DoesGameObjectNameMatch( AkLpCstr in_pszGameObjectWatchName, AkLpCstr in_pszGameObjectName )
{
	// If the name is empty, we don't consider it equal
	if( in_pszGameObjectWatchName == NULL || in_pszGameObjectWatchName[0] == NULL )
		return false;

	if( in_pszGameObjectWatchName[0] == '*' )
		return true;

	if( in_pszGameObjectName == NULL || in_pszGameObjectName[0] == NULL )
		return false;

	int iWatchNameLen = (int)strlen( in_pszGameObjectWatchName );
	if( in_pszGameObjectWatchName[iWatchNameLen-1] == '*' )
		return CompareNoCase( in_pszGameObjectWatchName, in_pszGameObjectName, iWatchNameLen-1 ) == 0;
	
	return CompareNoCase( in_pszGameObjectWatchName, in_pszGameObjectName, iWatchNameLen ) == 0;
}

extern AkInitSettings g_settings;

AkMonitor*	AkMonitor::m_pInstance 	= NULL;
AkIDStringHash AkMonitor::m_idxGameObjectString;

bool AkMonitor::m_arrayListenerWatch[AK_NUM_LISTENERS] = {false};
AkMonitor::AkMapGameObjectWatch AkMonitor::m_mapGameObjectWatch;
AkMonitor::WatchesArray AkMonitor::m_watches;
AkMonitor::GameSyncWatchesArray AkMonitor::m_gameSyncWatches;

AkMemPoolId	AkMonitor::m_MonitorQueuePoolId = AK_INVALID_POOL_ID;
AkMemPoolId	AkMonitor::m_MonitorPoolId      = AK_INVALID_POOL_ID;
AkUInt32 AkMonitor::m_uLocalOutputErrorLevel = AK::Monitor::ErrorLevel_Error; 
AK::Monitor::LocalOutputFunc AkMonitor::m_funcLocalOutput = NULL;

AkMonitor::AkMonitor()
	: m_uiNotifFilter( 0 )
{
	AkClearThread(&m_hThread);
	AkClearEvent( m_hMonitorEvent );
	AkClearEvent( m_hMonitorDoneEvent );
	m_bStopThread = false;
	m_mapGameObjectWatch.Init( g_DefaultPoolId );
}

AkMonitor::~AkMonitor()
{
	StopMonitoring();

	m_mapGameObjectWatch.Term();
	m_watches.Term();
	m_gameSyncWatches.Term();
}

AkMonitor* AkMonitor::Instance()
{
    if (!m_pInstance)
    {
        m_pInstance = AkNew(g_DefaultPoolId,AkMonitor());
    }
    return m_pInstance;
}

void AkMonitor::Destroy()
{
	if (m_pInstance)
	{
		AkDelete(g_DefaultPoolId,m_pInstance);
		m_pInstance = NULL;
	}
}

void AkMonitor::Register( AK::IALMonitorSink* in_pMonitorSink, AkMonitorData::MonitorDataType in_whatToMonitor )
{
	AkAutoLock<CAkLock> gate( m_registerLock );

	// Clients can re-register themselves with added notifications. Need to do 'recap' only for
	// the part that is new.

	AkMonitorData::MonitorDataType uiNewFilters;

	AkMonitorData::MonitorDataType * pFilters = m_sink2Filter.Exists( in_pMonitorSink );
	if ( pFilters ) 
	{
		// forbidden to remove notifications through re-registration. Use Unregister instead.
		AKASSERT( !( *pFilters - ( *pFilters & in_whatToMonitor ) ) );

		uiNewFilters = in_whatToMonitor - ( *pFilters & in_whatToMonitor );
		*pFilters = in_whatToMonitor;
	}
	else
	{
		uiNewFilters = in_whatToMonitor;
		m_sink2Filter.Set( in_pMonitorSink, in_whatToMonitor );
	}

	if( uiNewFilters & AkMonitorData::MonitorDataObjRegistration )
	{
		RecapRegisteredObjects( in_pMonitorSink );
	}

	if( uiNewFilters & AkMonitorData::MonitorDataSwitch )
	{
		RecapSwitches( in_pMonitorSink );
	}

	if ( uiNewFilters & AkMonitorData::MonitorDataMemoryPoolName )
	{
        RecapMemoryPools( in_pMonitorSink );
	}

	if ( uiNewFilters & AkMonitorData::MonitorDataSoundBank )
	{
        RecapDataSoundBank( in_pMonitorSink );
	}

	if ( uiNewFilters & AkMonitorData::MonitorDataMedia )
	{
        RecapMedia( in_pMonitorSink );
	}

	if ( uiNewFilters & AkMonitorData::MonitorDataEvent )
	{
        RecapEvents( in_pMonitorSink );
	}

	if ( uiNewFilters & AkMonitorData::MonitorDataGameSync )
	{
        RecapGameSync( in_pMonitorSink );
	}

    if ( uiNewFilters & AkMonitorData::MonitorDevicesRecord )
	{
        // Note: On the stream mgr API, StartMonitoring() means "start listening",
        // which is true only when someone listens. 
        StartStreamProfiler( );

        RecapDevices( in_pMonitorSink );
	}

    if ( uiNewFilters & AkMonitorData::MonitorDataStreamsRecord )
	{
        // Note: On the stream mgr API, StartMonitoring() means "start listening",
        // which is true only when someone listens. 
        StartStreamProfiler( );

        RecapStreamRecords( in_pMonitorSink );
	}

	m_uiNotifFilter |= in_whatToMonitor;
}

void AkMonitor::Unregister( AK::IALMonitorSink* in_pMonitorSink )
{
	AkAutoLock<CAkLock> gate( m_unregisterLock );

	m_sink2Filter.Unset( in_pMonitorSink );

	m_uiNotifFilter = 0;
	for( MonitorSink2Filter::Iterator it = m_sink2Filter.Begin(); it != m_sink2Filter.End(); ++it )
		m_uiNotifFilter |= (*it).item;

	if ( !( m_uiNotifFilter & ( AkMonitorData::MonitorDevicesRecord | AkMonitorData::MonitorDataStreamsRecord ) ) )
	{
		// Stop stream profiling if no one listens.
		// (See note near StopStreamProfiler( ) implementation).
		StopStreamProfiler();
	}
}

void AkMonitor::SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount )
{
	// Clear the current watch
	for( int i = 0; i < AK_NUM_LISTENERS; ++i )
		m_arrayListenerWatch[i] = false;

	m_mapGameObjectWatch.RemoveAll();
	m_watches.RemoveAll();

	// Populate the watch list
	for( AkUInt32 i = 0; i < in_uiWatchCount; ++i )
	{
		switch( in_pWatches[i].eType )
		{
		case AkMonitorData::WatchType_GameObjectID:
		{
			CAkRegisteredObj* pObj = g_pRegistryMgr->GetObjAndAddref( in_pWatches[i].ID.gameObjectID );
			if( pObj )
			{
				pObj->Release();
				m_mapGameObjectWatch.Set( in_pWatches[i].ID.gameObjectID );
			}
			break;
		}
		case AkMonitorData::WatchType_GameObjectName:
			AddWatchesByGameObjectName( in_pWatches[i].szName );
			break;
		case AkMonitorData::WatchType_ListenerID:
			m_arrayListenerWatch[in_pWatches[i].ID.uiListenerID] = true;
			break;
		}

		m_watches.AddLast( in_pWatches[i] );
	}
}

void AkMonitor::SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount )
{
	m_gameSyncWatches.RemoveAll();

	// Populate the watch list
	for( AkUInt32 i = 0; i < in_uiWatchCount; ++i )
	{
		m_gameSyncWatches.AddLast( in_pWatches[i] );
	}
}

void AkMonitor::AddWatchesByGameObjectName( AkLpCstr in_pszGameObjectWatchName )
{
	AkMonitorData::MonitorDataItem * pData = (AkMonitorData::MonitorDataItem *)
		alloca( offsetof(AkMonitorData::MonitorDataItem, objRegistrationData.szName) + MONITOR_GAMEOBJNAME_MAXLENGTH );

	// For all existing game objects, check if they match a specific name
	for( AkIDStringHash::AkStringHash::Iterator iter = m_idxGameObjectString.m_list.Begin(); iter != m_idxGameObjectString.m_list.End(); ++iter )
	{
		if( DoesGameObjectNameMatch( in_pszGameObjectWatchName, &((*iter).item) ) )
		{
			m_mapGameObjectWatch.Set( (*iter).key );
		}
	}
}

void AkMonitor::AddWatchForGameObject( AkGameObjectID in_GameObject, AkLpCstr in_pszGameObjectName )
{
	// Check in the watch list
	for( WatchesArray::Iterator iter = m_watches.Begin(); iter != m_watches.End(); ++iter )
	{
		if( ((*iter).eType == AkMonitorData::WatchType_GameObjectID && 
			 (*iter).ID.gameObjectID == in_GameObject )
			||
			((*iter).eType == AkMonitorData::WatchType_GameObjectName &&
			 DoesGameObjectNameMatch( (*iter).szName, in_pszGameObjectName ) ) )
		{
			m_mapGameObjectWatch.Set( in_GameObject );
		}
	}
}

void AkMonitor::RemoveWatchForGameObject( AkGameObjectID in_GameObject )
{
	m_mapGameObjectWatch.Unset( in_GameObject );
}

void AkMonitor::RecapRegisteredObjects( AK::IALMonitorSink* in_pMonitorSink )
{
	AKASSERT( in_pMonitorSink );

	AkMonitorData::MonitorDataItem * pData = (AkMonitorData::MonitorDataItem *)
		alloca( offsetof(AkMonitorData::MonitorDataItem, objRegistrationData.szName) + MONITOR_GAMEOBJNAME_MAXLENGTH );

	for( AkIDStringHash::AkStringHash::Iterator iter = m_idxGameObjectString.m_list.Begin(); iter != m_idxGameObjectString.m_list.End(); ++iter )
	{
		pData->eDataType = AkMonitorData::MonitorDataObjRegistration;
		pData->timeStamp = AkMonitor::GetThreadTime();
		pData->objRegistrationData.isRegistered = true;
		pData->objRegistrationData.gameObjPtr = (*iter).key;

		pData->objRegistrationData.wStringSize = (AkUInt16) strlen( &((*iter).item) ) + 1;
		pData->objRegistrationData.wStringSize = AkMin( pData->objRegistrationData.wStringSize, MONITOR_GAMEOBJNAME_MAXLENGTH );

		strcpy( pData->objRegistrationData.szName,  &((*iter).item) );

		in_pMonitorSink->MonitorNotification( *pData );
	}
}

void AkMonitor::RecapSwitches( AK::IALMonitorSink* in_pMonitorSink )
{
	AKASSERT( in_pMonitorSink );
	AKASSERT( g_pRTPCMgr );
	g_pRTPCMgr->m_RTPCLock.Lock();
	for( CAkRTPCMgr::AkMapSwitchEntries::Iterator iter = g_pRTPCMgr->m_SwitchEntries.Begin(); iter != g_pRTPCMgr->m_SwitchEntries.End(); ++iter )
	{
		AkMonitorData::MonitorDataItem _MonitorDataItem;
		_MonitorDataItem.eDataType = AkMonitorData::MonitorDataSwitch;
		_MonitorDataItem.timeStamp = AkMonitor::GetThreadTime();
		_MonitorDataItem.switchData.gameObj = (*iter).key.m_pGameObj?(*iter).key.m_pGameObj->ID():AK_INVALID_GAME_OBJECT;
		_MonitorDataItem.switchData.switchGroupID = (*iter).key.m_SwitchGroup;
		_MonitorDataItem.switchData.switchState = (*iter).item;
		in_pMonitorSink->MonitorNotification( _MonitorDataItem );
	}
	g_pRTPCMgr->m_RTPCLock.Unlock();
}

void AkMonitor::RecapMemoryPools( AK::IALMonitorSink* in_pMonitorSink )
{
	AKASSERT( in_pMonitorSink );

	AkMonitorData::MonitorDataItem * pData = (AkMonitorData::MonitorDataItem *)
		alloca( offsetof(AkMonitorData::MonitorDataItem, memoryPoolNameData.szName) + MONITOR_MSG_MAXLENGTH * sizeof( wchar_t ) );

	AkUInt32 ulNumPools = AK::MemoryMgr::GetMaxPools();
	for ( AkUInt32 ulPool = 0; ulPool < ulNumPools; ++ulPool )
	{
		AkTChar * pszName = AK::MemoryMgr::GetPoolName(ulPool);
		if ( !pszName )
			continue;

		pData->eDataType = AkMonitorData::MonitorDataMemoryPoolName;
		pData->timeStamp = AkMonitor::GetThreadTime();
		pData->memoryPoolNameData.ulPoolId = ulPool;
		AkUInt16 stringSize = (AkUInt16)( wcslen( pszName ) + 1 );
		pData->memoryPoolNameData.wStringSize = (AkUInt16) AkMin( stringSize, MONITOR_MSG_MAXLENGTH );

		wcsncpy( pData->memoryPoolNameData.szName, pszName, pData->memoryPoolNameData.wStringSize );
		pData->memoryPoolNameData.szName[ MONITOR_MSG_MAXLENGTH - 1 ] = 0;

		in_pMonitorSink->MonitorNotification( *pData );
	}
}

void AkMonitor::RecapDataSoundBank( AK::IALMonitorSink* in_pMonitorSink )
{
	AkAutoLock<CAkLock> gate( g_pBankManager->m_BankListLock );

	CAkBankMgr::AkListLoadedBanks& rBankList =  g_pBankManager->m_ListLoadedBanks;

	AkMonitorData::MonitorDataItem monitorDataItem;

	monitorDataItem.eDataType = AkMonitorData::MonitorDataSoundBank;
	monitorDataItem.timeStamp = AkMonitor::GetThreadTime();
	monitorDataItem.loadedSoundBankData.bIsDestroyed = false;

	for( CAkBankMgr::AkListLoadedBanks::Iterator iter = rBankList.Begin(); iter != rBankList.End(); ++iter )
	{
		CAkUsageSlot* pUsageSlot = iter.pItem->item;

		monitorDataItem.loadedSoundBankData.bankID = pUsageSlot->m_BankID;
		monitorDataItem.loadedSoundBankData.memPoolID = pUsageSlot->m_memPoolId;
		monitorDataItem.loadedSoundBankData.uBankSize = pUsageSlot->m_uLoadedDataSize;
		monitorDataItem.loadedSoundBankData.uNumIndexableItems = pUsageSlot->m_listLoadedItem.Length();
		monitorDataItem.loadedSoundBankData.uNumMediaItems = pUsageSlot->m_uNumLoadedItems;
		monitorDataItem.loadedSoundBankData.bIsExplicitelyLoaded = pUsageSlot->WasLoadedAsABank();

		in_pMonitorSink->MonitorNotification( monitorDataItem );
	}
}

void AkMonitor::_RecapMediaEntry( AK::IALMonitorSink* in_pMonitorSink, AkMediaEntry& rMediaEntry )
{
	AkUInt32 numBankOptions = rMediaEntry.GetNumBankOptions();
	AkUInt32 arraysize = numBankOptions;

	if( rMediaEntry.IsDataPrepared() )
		++arraysize;

	AkMonitorData::MonitorDataItem * pData = (AkMonitorData::MonitorDataItem *)
		alloca( offsetof(AkMonitorData::MonitorDataItem, mediaPreparedData.bankIDs) + arraysize * sizeof( AkBankID ) );

	pData->eDataType = AkMonitorData::MonitorDataMedia;
	pData->timeStamp = AkMonitor::GetThreadTime();

	pData->mediaPreparedData.uMediaID = rMediaEntry.GetSourceID();
	pData->mediaPreparedData.uMediaSize = rMediaEntry.m_mediaInfo.uInMemoryDataSize;
	pData->mediaPreparedData.uArraySize = arraysize;

	AkUInt32 i = 0;
	for( ; i < numBankOptions; ++i )
	{
		pData->mediaPreparedData.bankIDs[i] = rMediaEntry.m_BankSlots[i].key->m_BankID;
	}
	if( rMediaEntry.IsDataPrepared() )
	{
		pData->mediaPreparedData.bankIDs[i] = AK_INVALID_UNIQUE_ID;
	}

	in_pMonitorSink->MonitorNotification( *pData );
}

void AkMonitor::RecapMedia( AK::IALMonitorSink* in_pMonitorSink )
{
	AkAutoLock<CAkLock> gate( g_pBankManager->m_MediaLock );

	CAkBankMgr::AkMediaHashTable::Iterator iter = g_pBankManager->m_MediaHashTable.Begin();
	while( iter != g_pBankManager->m_MediaHashTable.End() )
	{
		_RecapMediaEntry( in_pMonitorSink, iter.pItem->Assoc.item );
		
		++iter;
	}
}

void AkMonitor::RecapEvents( AK::IALMonitorSink* in_pMonitorSink )
{
	AKASSERT( g_pIndex );
	CAkIndexItem<CAkEvent*>& l_rIdx = g_pIndex->m_idxEvents;

	AkAutoLock<CAkLock> IndexLock( l_rIdx.GetLock() );

	AkMonitorData::MonitorDataItem monitorDataItem;

	monitorDataItem.eDataType = AkMonitorData::MonitorDataEvent;
	monitorDataItem.timeStamp = AkMonitor::GetThreadTime();

	AkHashListBare<AkUniqueID, CAkIndexable, 31>::Iterator iter = l_rIdx.m_mapIDToPtr.Begin();
	while( iter != l_rIdx.m_mapIDToPtr.End() )
	{
		CAkEvent* pEvent = static_cast<CAkEvent*>( *iter );

		monitorDataItem.eventPreparedData.eventID = pEvent->ID();
		monitorDataItem.eventPreparedData.uRefCount = pEvent->GetPreparationCount();

		in_pMonitorSink->MonitorNotification( monitorDataItem );
		
		++iter;
	}
}

void AkMonitor::RecapGroupHelper( AkMonitorData::MonitorDataItem& in_rMonitorDataItem, CAkStateMgr::PreparationGroups& in_Groups, AK::IALMonitorSink* in_pMonitorSink )
{
	CAkStateMgr::PreparationGroups::Iterator iter = in_Groups.Begin();
	while( iter != in_Groups.End() )
	{
		CAkStateMgr::PreparationStateItem* pItem = iter.pItem;
		// We have a group, we will then have to iterate trough each game sync.
		in_rMonitorDataItem.gameSyncData.groupID = pItem->GroupID();

		CAkPreparedContent::ContentList& rContentList = pItem->GetPreparedcontent()->m_PreparableContentList;
		CAkPreparedContent::ContentList::Iterator subIter = rContentList.Begin();
		while( subIter != rContentList.End() )
		{
			in_rMonitorDataItem.gameSyncData.syncID = *(subIter.pItem);
			in_pMonitorSink->MonitorNotification( in_rMonitorDataItem );

			++subIter;
		}
		++iter;
	}
}

void AkMonitor::RecapGameSync( AK::IALMonitorSink* in_pMonitorSink )
{
	if( g_pStateMgr )
	{
		AkMonitorData::MonitorDataItem monitorDataItem;

		monitorDataItem.eDataType = AkMonitorData::MonitorDataGameSync;
		monitorDataItem.timeStamp = AkMonitor::GetThreadTime();
		monitorDataItem.gameSyncData.bIsEnabled = true; // only enabled things are in the list.

		AkAutoLock<CAkLock> gate( g_pStateMgr->m_PrepareGameSyncLock );// to pad monitoring recaps.

		monitorDataItem.gameSyncData.eSyncType = AkGroupType_State;
		RecapGroupHelper( monitorDataItem, g_pStateMgr->m_PreparationGroupsStates, in_pMonitorSink );

		monitorDataItem.gameSyncData.eSyncType = AkGroupType_Switch;
		RecapGroupHelper( monitorDataItem, g_pStateMgr->m_PreparationGroupsSwitches, in_pMonitorSink );
	}
}

void AkMonitor::RecapDevices( AK::IALMonitorSink* in_pMonitorSink )
{
    AKASSERT( in_pMonitorSink );

    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( !pStmMgrProfile )
        return; // Profiling interface not implemented in that stream manager.

    // Get all devices.
    AkUInt32 ulNumDevices = pStmMgrProfile->GetNumDevices( );
	for ( AkUInt32 ulDevice = 0; ulDevice < ulNumDevices; ++ulDevice )
	{
        AK::IAkDeviceProfile * pDevice = pStmMgrProfile->GetDeviceProfile( ulDevice );
        AKASSERT( pDevice );
        
		AkMonitorData::MonitorDataItem _MonitorDataItem;
        _MonitorDataItem.eDataType = AkMonitorData::MonitorDevicesRecord;
		_MonitorDataItem.timeStamp = AkMonitor::GetThreadTime();
		pDevice->GetDesc( _MonitorDataItem.deviceRecordData );
        pDevice->ClearNew( );   // Whether it was new or not, it is not anymore.

		in_pMonitorSink->MonitorNotification( _MonitorDataItem );
	}
}

void AkMonitor::RecapStreamRecords( AK::IALMonitorSink* in_pMonitorSink )
{
    AKASSERT( in_pMonitorSink );

    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( !pStmMgrProfile )
        return; // Profiling interface not implemented in that stream manager.

    // Get all stream records, for all devices.
    AkUInt32 ulNumDevices = pStmMgrProfile->GetNumDevices( );
    IAkDeviceProfile * pDevice;
    for ( AkUInt32 ulDevice=0; ulDevice<ulNumDevices; ulDevice++ )
    {
        pDevice = pStmMgrProfile->GetDeviceProfile( ulDevice );
        AKASSERT( pDevice != NULL );
       
        AkUInt32 ulNumStreams = pDevice->GetNumStreams( );
        AkMonitorData::MonitorDataItem _MonitorDataItem;
        _MonitorDataItem.eDataType = AkMonitorData::MonitorDataStreamsRecord;
	    _MonitorDataItem.streamRecordData.ulNumNewRecords = 1;
        AK::IAkStreamProfile * pStream;
	    for ( AkUInt32 ulStream = 0; ulStream < ulNumStreams; ++ulStream )
	    {
            pStream = pDevice->GetStreamProfile( ulStream );
            AKASSERT( pStream );

            _MonitorDataItem.timeStamp = AkMonitor::GetThreadTime();
            pStream->GetStreamRecord( *_MonitorDataItem.streamRecordData.streamRecords );
            pStream->ClearNew( );   // Whether it was new or not, it is not anymore.		
            in_pMonitorSink->MonitorNotification( _MonitorDataItem );
	    }
    }
}

// Note: On the stream mgr API, StartMonitoring() means "start listening",
// which is false unless someone listens. It could be NOT the case even if 
// AkMonitor::StartMonitoring() had been called (see AkMonitor::IsMonitoring()
// implementation).
void AkMonitor::StartStreamProfiler( )
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    if ( AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( ) )
        AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( )->StartMonitoring( );
}

void AkMonitor::StopStreamProfiler( )
{
    AKASSERT( AK::IAkStreamMgr::Get( ) );
    if ( AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( ) )
    	AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( )->StopMonitoring( );
}

AKRESULT AkMonitor::StartMonitoring()
{
	if( AkIsValidThread(&m_hThread) )
		return AK_Success;

	AkUInt32 uQueuePoolSize = g_settings.uMonitorQueuePoolSize ? g_settings.uMonitorQueuePoolSize : MONITOR_QUEUE_POOL_SIZE;
	m_MonitorQueuePoolId = AK::MemoryMgr::CreatePool(NULL, uQueuePoolSize, uQueuePoolSize, AkMalloc | AkFixedSizeBlocksMode );
    if ( m_MonitorQueuePoolId == AK_INVALID_POOL_ID )
		return AK_Fail;

	m_ringItems.Init( m_MonitorQueuePoolId, uQueuePoolSize );

	m_MonitorPoolId = AK::MemoryMgr::CreatePool(NULL, g_settings.uMonitorPoolSize ? g_settings.uMonitorPoolSize : MONITOR_POOL_SIZE, MONITOR_POOL_BLOCK_SIZE, AkMalloc );
	if( m_MonitorPoolId == AK_INVALID_POOL_ID )
		return AK_Fail;

	if ( m_sink2Filter.Init( MONITOR_SINKS_INITIAL_SIZE, MONITOR_SINKS_MAX_SIZE, m_MonitorPoolId ) != AK_Success )
		return AK_Fail;

	if ( m_idxGameObjectString.Init( m_MonitorPoolId ) != AK_Success )
		return AK_Fail;

	AK_SETPOOLNAME(m_MonitorQueuePoolId,L"Monitor Queue");
	AK_SETPOOLNAME(m_MonitorPoolId,L"Monitor");

	m_bStopThread = false;
	if ( AkCreateEvent( m_hMonitorEvent ) != AK_Success
		|| AkCreateEvent( m_hMonitorDoneEvent ) != AK_Success )
		return AK_Fail;

	AkCreateThread(	MonitorThreadFunc,    // Start address
					this,                 // Parameter
					&g_PDSettings.threadMonitor,
					&m_hThread,
					"AK::Monitor" );	  // Handle
	if ( !AkIsValidThread(&m_hThread) )
	{
		AKASSERT( !"Could not create monitor thread" );
		return AK_Fail;
	}

	return AK_Success;
}

void AkMonitor::StopMonitoring()
{
	if(AkIsValidThread(&m_hThread))
	{
		m_bStopThread = true;
		AkSignalEvent( m_hMonitorEvent );
		AkWaitForSingleThread( &m_hThread );

		AkCloseThread( &m_hThread );
	}

	AkDestroyEvent( m_hMonitorEvent );
	AkDestroyEvent( m_hMonitorDoneEvent );

    StopStreamProfiler( );

	if ( m_MonitorPoolId != AK_INVALID_POOL_ID )
	{
		m_sink2Filter.Term();
		m_idxGameObjectString.Term();
		AK::MemoryMgr::DestroyPool( m_MonitorPoolId );
		m_MonitorPoolId = AK_INVALID_POOL_ID;
	}

	if ( m_MonitorQueuePoolId != AK_INVALID_POOL_ID )
	{
		m_ringItems.Term( m_MonitorQueuePoolId );
		AK::MemoryMgr::DestroyPool( m_MonitorQueuePoolId );
		m_MonitorQueuePoolId = AK_INVALID_POOL_ID;
	}
}

void AkMonitor::PostWatchedGameObjPositions()
{
	int i = 0;

	// Use the watched game objects only
	AkUInt32 uNumGameObjs = m_mapGameObjectWatch.Length();
	AkUInt32 uNumListeners = 0;
	for( i = 0 ; i < AK_NUM_LISTENERS; ++i )
		uNumListeners += m_arrayListenerWatch[i] ? 1 : 0;
	
    AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, gameObjPositionData.positions )
						+ (uNumGameObjs + uNumListeners) * sizeof( AkMonitorData::GameObjPositionMonitorData::Position );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataGameObjPosition;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->gameObjPositionData.ulNumGameObjPositions = uNumGameObjs;
	creator.m_pData->gameObjPositionData.ulNumListenerPositions = uNumListeners;

	int iGameObj = 0;
	for ( AkMapGameObjectWatch::Iterator iter = m_mapGameObjectWatch.Begin(); iter != m_mapGameObjectWatch.End(); ++iter )
	{
		AkMonitorData::GameObjPosition & gameObjPos = creator.m_pData->gameObjPositionData.positions[iGameObj].gameObjPosition;

		gameObjPos.gameObjID = (*iter).key;

		AkSoundPositionEntry position;
		if( g_pRegistryMgr->GetPosition( (*iter).key, position ) == AK_Success )
		{
			if ( position.uListenerIdx != AK_INVALID_LISTENER_INDEX )
			{
				AkListenerPosition listPos;
				CAkListener::GetListenerPosition( position.uListenerIdx, listPos );
				_SetSoundPosToListener( listPos, gameObjPos.position );
			}
			else
				gameObjPos.position = position.pos;
		}
		else
		{
			AKASSERT( !"Why can't we obtain a position for this object?" );
		}
		++iGameObj;
	}

	int iListener = 0;
	for( i = 0; i < AK_NUM_LISTENERS; ++i )
	{
		if( m_arrayListenerWatch[i] )
		{
			creator.m_pData->gameObjPositionData.positions[iGameObj+iListener].listenerPosition.uIndex = i;

			AkListenerPosition& position = creator.m_pData->gameObjPositionData.positions[iGameObj+iListener].listenerPosition.position;
			if( CAkListener::GetListenerPosition( i, position ) != AK_Success )
			{
				AKASSERT( !"Why can't we obtain a position for this listener?" );
			}

			++iListener;
		}
	}
}

void AkMonitor::PostWatchesRTPCValues()
{
	// Use the watched game objects only
	AkUInt32 uNumGameObjs = m_mapGameObjectWatch.Length();
	AkUInt32 uNumGameSyncs = m_gameSyncWatches.Length();

    AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, rtpcValuesData.rtpcValues )
						+ (uNumGameObjs*uNumGameSyncs) * sizeof( AkMonitorData::RTPCValuesPacket );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataRTPCValues;
	creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

	creator.m_pData->rtpcValuesData.ulNumRTPCValues = uNumGameObjs*uNumGameSyncs;

	int index = 0;
	for ( AkMapGameObjectWatch::Iterator iter = m_mapGameObjectWatch.Begin(); iter != m_mapGameObjectWatch.End(); ++iter )
	{
		for ( unsigned int j = 0; j < m_gameSyncWatches.Length(); ++j )
		{
			creator.m_pData->rtpcValuesData.rtpcValues[index].gameObjectID = (*iter).key;
			creator.m_pData->rtpcValuesData.rtpcValues[index].rtpcID = m_gameSyncWatches[j];

			CAkRegisteredObj* pGameObj = g_pRegistryMgr->GetObjAndAddref( (*iter).key );
			
			creator.m_pData->rtpcValuesData.rtpcValues[index].value = 0;

			if( pGameObj )
			{
				bool bGameObjectSpecificValue = false;
				creator.m_pData->rtpcValuesData.rtpcValues[index].bHasValue = g_pRTPCMgr->GetRTPCValue( 
					m_gameSyncWatches[j], 
					pGameObj, 
					creator.m_pData->rtpcValuesData.rtpcValues[index].value, 
					bGameObjectSpecificValue );

				pGameObj->Release();
			}
			else
			{
				creator.m_pData->rtpcValuesData.rtpcValues[index].bHasValue = false;
				AKASSERT( !"Why can't we obtain a game object pointer for this game object?" );
			}
			++index;
		}
	}
}

bool AkMonitor::DispatchNotification()
{
	bool bReturnVal = false;

	AkAutoLock<CAkLock> gate( m_unregisterLock );

	// First keep a local copy of the sinks -- this is to prevent problems with sinks registering from other threads
	// while we notify.

	m_registerLock.Lock();

	// Allocate copy on the stack ( CAkKeyList copy was identified as a bottleneck here )
	int cSinks = m_sink2Filter.Length();
	MapStruct<AK::IALMonitorSink*, AkMonitorData::MonitorDataType> * pSinksCopy =
		(MapStruct<AK::IALMonitorSink*, AkMonitorData::MonitorDataType> *) alloca( sizeof( MapStruct<AK::IALMonitorSink*, AkMonitorData::MonitorDataType> ) * cSinks );
	{
		int i = 0;
		for( MonitorSink2Filter::Iterator it = m_sink2Filter.Begin(); it != m_sink2Filter.End(); ++it )
			pSinksCopy[ i++ ] = *it;
	}

	m_registerLock.Unlock();

	// Next, send a maximum of MAX_NOTIFICATIONS_PER_CALL notifications in the queue
	int iNotifCount = MAX_NOTIFICATIONS_PER_CALL;
	while ( !m_ringItems.IsEmpty() )
	{
		if(--iNotifCount == 0)
		{
			bReturnVal = true; //remaining items to notify
			break; //exit while loop
		}

		AkMonitorData::MonitorDataItem * pItem = (AkMonitorData::MonitorDataItem * ) m_ringItems.BeginRead();
		AKASSERT( pItem->eDataType );

		for ( int i = 0; i < cSinks; ++i )
		{
			if ( pSinksCopy[ i ].item & pItem->eDataType )
				pSinksCopy[ i ].key->MonitorNotification( *pItem );
		}

		m_ringItems.EndRead( pItem, AkMonitorData::RealSizeof( *pItem ) );
	}

	return bReturnVal;
}

AK_DECLARE_THREAD_ROUTINE( AkMonitor::MonitorThreadFunc )
{
	AkMonitor& rThis = *reinterpret_cast<AkMonitor*>( AK_THREAD_ROUTINE_PARAMETER );

	while( true )
	{
		AkWaitForEvent( rThis.m_hMonitorEvent );

		if ( rThis.m_bStopThread )
		{
			// Stop event.  Bail out.
			break;
		}

		// Something in the queue!
		if ( !rThis.m_ringItems.IsEmpty() )
			while( rThis.DispatchNotification() ); //loop until there are no more notifications

		AkSignalEvent( rThis.m_hMonitorDoneEvent );
	}

	AkExitThread(AK_RETURN_THREAD_OK);
}

void AkMonitor::Monitor_PostCode( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, AkUInt32 in_param1 )
{
	if ( in_eErrorCode >= 0 && in_eErrorCode < AK::Monitor::Num_ErrorCodes && ( m_uLocalOutputErrorLevel & in_eErrorLevel ) )
	{
		AkLpCtstr pszError = AK::Monitor::s_aszErrorCodes[ in_eErrorCode ];
		if ( m_funcLocalOutput )
		{
			m_funcLocalOutput( pszError, in_eErrorLevel );
		}
		else
		{
			AKPLATFORM::OutputDebugMsg( in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? L"AK Message: " : L"AK Error: " );
			AKPLATFORM::OutputDebugMsg( pszError );
			AKPLATFORM::OutputDebugMsg( L"\n" );
		}
	}

	AkUInt32 sizeofData = offsetof( AkMonitorData::MonitorDataItem, errorData1 ) + sizeof( AkMonitorData::ErrorMonitorData1 );
	AkMonitorDataCreator creator( 
		in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AkMonitorData::MonitorDataMessageCode : AkMonitorData::MonitorDataErrorCode, 
		sizeofData );
	if ( !creator.m_pData )
		return;

	creator.m_pData->errorData1.eErrorCode = in_eErrorCode;
	creator.m_pData->errorData1.uParam1 = in_param1;
}

void AkMonitor::Monitor_PostString( AkLpCtstr in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel )
{
	if ( in_pszError )
	{
		if ( m_uLocalOutputErrorLevel & in_eErrorLevel )
		{
			if ( m_funcLocalOutput )
			{
				m_funcLocalOutput( in_pszError, in_eErrorLevel );
			}
			else
			{
				AKPLATFORM::OutputDebugMsg( in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? L"AK Message: " : L"AK Error: " );
				AKPLATFORM::OutputDebugMsg( in_pszError );
				AKPLATFORM::OutputDebugMsg( L"\n" );
			}
		}

		AkUInt16 wStringSize = (AkUInt16) wcslen(in_pszError) + 1;
		AkMonitorDataCreator creator( 
			in_eErrorLevel == AK::Monitor::ErrorLevel_Message ? AkMonitorData::MonitorDataMessageString : AkMonitorData::MonitorDataErrorString, 
			offsetof( AkMonitorData::MonitorDataItem, debugData.szMessage ) + wStringSize * sizeof( wchar_t ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->debugData.wStringSize = wStringSize;

		AKPLATFORM::AkMemCpy(
			&creator.m_pData->debugData.szMessage,
			(void *) in_pszError, wStringSize * sizeof( AkTChar )
			);
	}
}

void AkMonitor::Monitor_ObjectNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, AkUniqueID in_targetObjectID, AkTimeMs in_fadeTime, AkUniqueID in_playlistItemID )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataObject, SIZEOF_MONITORDATA( objectData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->objectData.eNotificationReason = in_eNotifReason ;
	creator.m_pData->objectData.gameObjPtr = in_GameObject;
	creator.m_pData->objectData.customParam = in_CustomParam;
	creator.m_pData->objectData.playingID = in_PlayingID;
	creator.m_pData->objectData.cntrHistArray = in_cntrHistArray;
	if( creator.m_pData->objectData.cntrHistArray.uiArraySize > AK_CONT_HISTORY_SIZE )
		creator.m_pData->objectData.cntrHistArray.uiArraySize = AK_CONT_HISTORY_SIZE;
	creator.m_pData->objectData.targetObjectID = in_targetObjectID;
	creator.m_pData->objectData.fadeTime = in_fadeTime;
	creator.m_pData->objectData.playlistItemID = in_playlistItemID;
}

void AkMonitor::Monitor_BankNotif( AkUniqueID in_BankID, AkUniqueID in_LanguageID, AkMonitorData::NotificationReason in_eNotifReason )
{
	const char* pBankName = g_pBankManager->GetBankFileName( in_BankID );
	AkUInt16 wStringSize = 0;
	if ( pBankName ) 
		wStringSize = (AkUInt16) ( strlen( pBankName ) + 1 );

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataBank, offsetof( AkMonitorData::MonitorDataItem, bankData.szBankName ) + wStringSize );
	if ( !creator.m_pData )
		return;

	creator.m_pData->bankData.bankID = in_BankID;
	creator.m_pData->bankData.languageID = in_LanguageID;
	creator.m_pData->bankData.eNotificationReason = in_eNotifReason;
	creator.m_pData->bankData.wStringSize = wStringSize;

	if( wStringSize )
	{
		strcpy( creator.m_pData->bankData.szBankName, pBankName );
	}
}

void AkMonitor::Monitor_PrepareNotif( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_GameSyncorEventID, AkUInt32 in_groupID, AkGroupType in_GroupType, AkUInt32 in_NumEvents )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataPrepare, SIZEOF_MONITORDATA( prepareData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->prepareData.eNotificationReason = in_eNotifReason;
	creator.m_pData->prepareData.gamesyncIDorEventID = in_GameSyncorEventID;
	creator.m_pData->prepareData.groupID			 = in_groupID;
	creator.m_pData->prepareData.groupType			 = in_GroupType;
	creator.m_pData->prepareData.uNumEvents			 = in_NumEvents;
}

void AkMonitor::Monitor_StateChanged( AkStateGroupID in_StateGroup, AkStateID in_PreviousState, AkStateID in_NewState )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataState, SIZEOF_MONITORDATA( stateData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->stateData.stateGroupID = in_StateGroup;
	creator.m_pData->stateData.stateFrom = in_PreviousState;
	creator.m_pData->stateData.stateTo = in_NewState;
}

void AkMonitor::Monitor_SwitchChanged( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_Switch, AkGameObjectID in_GameObject )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataSwitch, SIZEOF_MONITORDATA( switchData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->switchData.switchGroupID = in_SwitchGroup;
	creator.m_pData->switchData.switchState = in_Switch;
	creator.m_pData->switchData.gameObj = in_GameObject;
}

void AkMonitor::Monitor_ObjectRegistration( bool in_isRegistration, AkGameObjectID in_GameObject, void * in_pMonitorData )
{
	AkIDStringHash::AkStringHash::Item * pItem = (AkIDStringHash::AkStringHash::Item *) in_pMonitorData;

	// Send notification
	{
		AkUInt16 wStringSize = 0;
		if ( pItem )
			wStringSize = (AkUInt16) strlen( &( pItem->Assoc.item ) ) + 1;

		AkMonitorDataCreator creator( AkMonitorData::MonitorDataObjRegistration, offsetof(AkMonitorData::MonitorDataItem, objRegistrationData.szName) + wStringSize );
		if ( creator.m_pData )
		{
			creator.m_pData->objRegistrationData.isRegistered = ( in_isRegistration );
			creator.m_pData->objRegistrationData.gameObjPtr = ( in_GameObject );
			creator.m_pData->objRegistrationData.wStringSize = wStringSize;

			if( pItem )
				AkMemCpy( creator.m_pData->objRegistrationData.szName, (void *) &( pItem->Assoc.item ), wStringSize );
		}
	}
	// Remember name in our own map
	if( in_isRegistration )
	{
		if( pItem )
		{
			if( m_idxGameObjectString.Set( pItem ) != AK_Success )
			{
				MONITOR_FREESTRING( pItem );
				pItem = NULL;
				// Game object with no name, maybe watch it if we have a "*" watch
				AddWatchForGameObject( in_GameObject, "" );
			}
			else
			{
				AddWatchForGameObject( in_GameObject, &( pItem->Assoc.item ) );
			}
			// m_idxGameObjectString. Set fails, there is nothing to do, this string will stay unknown.
		}
		else
		{
			// Game object with no name, maybe watch it if we have a "*" watch
			AddWatchForGameObject( in_GameObject, "" );
		}
	}
	else
	{
		m_idxGameObjectString.Unset( in_GameObject );
		RemoveWatchForGameObject( in_GameObject );
	}
}

void AkMonitor::Monitor_FreeString( void * in_pMonitorData )
{
	if( in_pMonitorData )
	{
		AkIDStringHash::AkStringHash::Item * pItem = (AkIDStringHash::AkStringHash::Item *) in_pMonitorData;
		m_idxGameObjectString.FreePreallocatedString( pItem );
	}
}

void AkMonitor::Monitor_ParamChanged( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataParamChanged, SIZEOF_MONITORDATA( paramChangedData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->paramChangedData.eNotificationReason = in_eNotifReason;
	creator.m_pData->paramChangedData.elementID = in_ElementID;
	creator.m_pData->paramChangedData.gameObjPtr = in_GameObject;
}

void AkMonitor::Monitor_EventTriggered( AkPlayingID in_PlayingID, AkUniqueID in_EventID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam)
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataEventTriggered, SIZEOF_MONITORDATA( eventTriggeredData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eventTriggeredData.playingID = in_PlayingID;
	creator.m_pData->eventTriggeredData.eventID = in_EventID;
	creator.m_pData->eventTriggeredData.gameObjPtr = in_GameObject;
	creator.m_pData->eventTriggeredData.customParam = in_CustomParam;
}

void AkMonitor::Monitor_ActionDelayed( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkTimeMs in_DelayTime, AkCustomParamType in_CustomParam )
{
	if( in_ActionID != AK_INVALID_UNIQUE_ID )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataActionDelayed, SIZEOF_MONITORDATA( actionDelayedData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->actionDelayedData.playingID = in_PlayingID;
		creator.m_pData->actionDelayedData.actionID = in_ActionID;
		creator.m_pData->actionDelayedData.gameObjPtr = in_GameObject;
		creator.m_pData->actionDelayedData.delayTime = in_DelayTime;
		creator.m_pData->actionDelayedData.customParam = in_CustomParam;
	}
}

void AkMonitor::Monitor_ActionTriggered( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam )
{
	if( in_ActionID != AK_INVALID_UNIQUE_ID )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataActionTriggered, SIZEOF_MONITORDATA( actionTriggeredData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->actionTriggeredData.playingID = in_PlayingID;
		creator.m_pData->actionTriggeredData.actionID = in_ActionID;
		creator.m_pData->actionTriggeredData.gameObjPtr = in_GameObject;
		creator.m_pData->actionTriggeredData.customParam = in_CustomParam;
	}
}

void AkMonitor::Monitor_BusNotification( AkUniqueID in_BusID, AkMonitorData::BusNotification in_NotifReason, AkUInt32 in_bitsFXBypass, AkUInt32 in_bitsMask )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataBusNotif, SIZEOF_MONITORDATA( busNotifData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->busNotifData.busID = in_BusID;
	creator.m_pData->busNotifData.notifReason = in_NotifReason;
	creator.m_pData->busNotifData.bitsFXBypass = (AkUInt8) in_bitsFXBypass;
	creator.m_pData->busNotifData.bitsMask = (AkUInt8) in_bitsMask;
}

void AkMonitor::Monitor_PathEvent( AkPlayingID in_playingID, AkUniqueID in_who, AkMonitorData::AkPathEvent in_eEvent, AkUInt32 in_index)
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataPath, SIZEOF_MONITORDATA( pathData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->pathData.playingID = (in_playingID);
	creator.m_pData->pathData.ulUniqueID = (in_who);
	creator.m_pData->pathData.eEvent =(in_eEvent);
	creator.m_pData->pathData.ulIndex = (in_index);
}

void AkMonitor::Monitor_errorMsg2( AkLpCtstr in_psz1, AkLpCtstr in_psz2 )
{
	if( in_psz1 && in_psz2 )
	{
		size_t totalsize = wcslen( in_psz1 ) + wcslen( in_psz2 ) + 1;
		AkTChar * tszBuffer = (AkTChar *) alloca( totalsize * sizeof( AkTChar ) );

		wcscpy( tszBuffer, in_psz1 );
		wcscat( tszBuffer, in_psz2 );

		Monitor_PostString( tszBuffer, AK::Monitor::ErrorLevel_Error );
	}
}

void AkMonitor::Monitor_LoadedBank( CAkUsageSlot* in_pUsageSlot, bool in_bIsDestroyed )
{
	if( in_pUsageSlot )// In some situations, NULL will be received, and it means that nothing must be notified.( keeping the if inside the MACRO )
	{
		AkMonitorDataCreator creator( AkMonitorData::MonitorDataSoundBank, SIZEOF_MONITORDATA( loadedSoundBankData ) );
		if ( !creator.m_pData )
			return;

		creator.m_pData->loadedSoundBankData.bankID = in_pUsageSlot->m_BankID;
		creator.m_pData->loadedSoundBankData.memPoolID = in_pUsageSlot->m_memPoolId;
		creator.m_pData->loadedSoundBankData.uBankSize = in_pUsageSlot->m_uLoadedDataSize;
		creator.m_pData->loadedSoundBankData.uNumIndexableItems = in_pUsageSlot->m_listLoadedItem.Length();
		creator.m_pData->loadedSoundBankData.uNumMediaItems = in_pUsageSlot->m_uNumLoadedItems;
		creator.m_pData->loadedSoundBankData.bIsExplicitelyLoaded = in_pUsageSlot->WasLoadedAsABank();

		creator.m_pData->loadedSoundBankData.bIsDestroyed = in_bIsDestroyed;
	}
}

void AkMonitor::Monitor_MediaPrepared( AkMediaEntry& in_rMediaEntry )
{
	// One entry per bank + one if the media was explicitely prepared too.
	AkUInt32 numBankOptions = in_rMediaEntry.GetNumBankOptions();
	AkUInt32 arraysize = numBankOptions;

	if( in_rMediaEntry.IsDataPrepared() )
		++arraysize;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMedia, offsetof( AkMonitorData::MonitorDataItem, mediaPreparedData.bankIDs ) + ( arraysize * sizeof( AkBankID ) ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->mediaPreparedData.uMediaID = in_rMediaEntry.GetSourceID();
	creator.m_pData->mediaPreparedData.uMediaSize = in_rMediaEntry.m_mediaInfo.uInMemoryDataSize;
	creator.m_pData->mediaPreparedData.uArraySize = arraysize;

	AkUInt32 i = 0;
	for( ; i < numBankOptions; ++i )
	{
		creator.m_pData->mediaPreparedData.bankIDs[i] = in_rMediaEntry.m_BankSlots[i].key->m_BankID;
	}
	if( in_rMediaEntry.IsDataPrepared() )
	{
		creator.m_pData->mediaPreparedData.bankIDs[i] = AK_INVALID_UNIQUE_ID;
	}
}

void AkMonitor::Monitor_EventPrepared( AkUniqueID in_EventID, AkUInt32 in_RefCount )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataEvent, SIZEOF_MONITORDATA( eventPreparedData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eventPreparedData.eventID = in_EventID;
	creator.m_pData->eventPreparedData.uRefCount = in_RefCount;
}

void AkMonitor::Monitor_GameSync( AkUniqueID in_GroupID, AkUniqueID in_GameSyncID, bool in_bIsEnabled, AkGroupType in_GroupType )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataGameSync, SIZEOF_MONITORDATA( gameSyncData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->gameSyncData.groupID = in_GroupID;
	creator.m_pData->gameSyncData.syncID = in_GameSyncID;
	creator.m_pData->gameSyncData.bIsEnabled = in_bIsEnabled;
	creator.m_pData->gameSyncData.eSyncType = in_GroupType;
}

void AkMonitor::Monitor_SetParamNotif_Float( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject, AkReal32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataSetParam, SIZEOF_MONITORDATA( setParamData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->setParamData.eNotificationReason = in_eNotifReason;
	creator.m_pData->setParamData.elementID = in_ElementID;
	creator.m_pData->setParamData.gameObjPtr = in_GameObject;
	creator.m_pData->setParamData.valueMeaning = in_ValueMeaning;
	creator.m_pData->setParamData.volumeTarget = in_TargetValue;
	creator.m_pData->setParamData.transitionTime = in_TransitionTime;
}

void AkMonitor::Monitor_SetParamNotif_Long( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject, AkInt32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataSetParam, SIZEOF_MONITORDATA( setParamData ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->setParamData.eNotificationReason = in_eNotifReason;
	creator.m_pData->setParamData.elementID = in_ElementID;
	creator.m_pData->setParamData.gameObjPtr = in_GameObject;
	creator.m_pData->setParamData.valueMeaning = in_ValueMeaning;
	AKASSERT( in_eNotifReason == AkMonitorData::NotificationReason_PitchChanged || in_TargetValue == 0);
	creator.m_pData->setParamData.pitchTarget = in_TargetValue ;
	creator.m_pData->setParamData.transitionTime = ( in_TransitionTime );
}

void AkMonitor::Monitor_ResolveDialogue( AkUniqueID in_idDialogueEvent, AkUniqueID in_idResolved, AkUInt32 in_cPath, AkArgumentValueID * in_pPath )
{
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataResolveDialogue, offsetof( AkMonitorData::MonitorDataItem, resolveDialogueData.aPath ) + ( in_cPath * sizeof( AkArgumentValueID ) ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->resolveDialogueData.idDialogueEvent = in_idDialogueEvent;
	creator.m_pData->resolveDialogueData.idResolved = in_idResolved;
	creator.m_pData->resolveDialogueData.uPathSize = in_cPath;

	for ( AkUInt32 i = 0; i < in_cPath; ++i )
		creator.m_pData->resolveDialogueData.aPath[ i ] = in_pPath[ i ];
}

void * AkMonitor::Monitor_AllocateGameObjNameString( AkGameObjectID in_GameObject, const char* in_GameObjString )
{
	return m_idxGameObjectString.Preallocate( in_GameObject, in_GameObjString );
}

void AkMonitor::Monitor_SetPoolName( AkMemPoolId in_PoolId, AkTChar * in_tcsPoolName )
{
	// Monitor is not yet instantiated when some basic pools get created -- just skip the
	// notification, no one is possibly connected anyway.
	if ( !AkMonitor::Get() || !in_tcsPoolName )
		return;

	AkUInt16 wStringSize = (AkUInt16) wcslen( in_tcsPoolName ) + 1;

	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMemoryPoolName, offsetof(AkMonitorData::MonitorDataItem, memoryPoolNameData.szName) + wStringSize * sizeof( wchar_t ) );
	if ( !creator.m_pData )
		return;

	creator.m_pData->memoryPoolNameData.ulPoolId = in_PoolId;
	creator.m_pData->memoryPoolNameData.wStringSize = wStringSize;
	AkMemCpy( creator.m_pData->memoryPoolNameData.szName, in_tcsPoolName, wStringSize * sizeof( wchar_t ) );
}

void AkMonitor::Monitor_MarkersNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, AkLpCstr in_strLabel )
{
	AkUInt16 wStringSize = 0;
	if( in_strLabel )
		wStringSize = (AkUInt16)strlen( in_strLabel )+1;

	AkUInt32 sizeofData = offsetof( AkMonitorData::MonitorDataItem, markersData.szLabel ) + wStringSize;
	AkMonitorDataCreator creator( AkMonitorData::MonitorDataMarkers, sizeofData );
	if ( !creator.m_pData )
		return;

	creator.m_pData->markersData.eNotificationReason = in_eNotifReason ;
	creator.m_pData->markersData.gameObjPtr = in_GameObject;
	creator.m_pData->markersData.customParam = in_CustomParam;
	creator.m_pData->markersData.playingID = in_PlayingID;
	creator.m_pData->markersData.cntrHistArray = in_cntrHistArray;
	creator.m_pData->markersData.targetObjectID = 0;
	creator.m_pData->markersData.wStringSize = wStringSize;
	if( wStringSize )
		strcpy( creator.m_pData->markersData.szLabel, in_strLabel );
}

// This function was previously inlined, but the 360 compiler had problems inlining it.
AkUInt32 AkMonitorData::RealSizeof( const MonitorDataItem & in_rItem )
{
	switch ( in_rItem.eDataType )
	{
	case MonitorDataPluginTimer:
		return offsetof(MonitorDataItem, pluginTimerData.pluginData ) 
			+ in_rItem.pluginTimerData.ulNumTimers * sizeof( PluginTimerData );

	case MonitorDataMemoryPool:
		return offsetof(MonitorDataItem, memoryData.poolData ) 
			+ in_rItem.memoryData.ulNumPools * sizeof( MemoryPoolData );

	case MonitorDataEnvironment:
		return offsetof( MonitorDataItem, environmentData.envPacket ) 
			+ in_rItem.environmentData.ulNumEnvPacket * sizeof( EnvPacket );

	case MonitorDataObsOcc:
		return offsetof( MonitorDataItem, obsOccData.obsOccPacket ) 
			+ in_rItem.obsOccData.ulNumPacket * sizeof( ObsOccPacket );

	case MonitorDataListeners:
		return offsetof( MonitorDataItem, listenerData.gameObjMask ) 
			+ in_rItem.listenerData.ulNumGameObjMask * sizeof( GameObjectListenerMaskPacket );

    case MonitorDataStreaming:
		return offsetof(MonitorDataItem, streamingData.streamData ) 
			+ in_rItem.streamingData.ulNumStreams * sizeof( StreamData );

    case MonitorDataStreamsRecord:
		return offsetof(MonitorDataItem, streamRecordData.streamRecords ) 
			+ in_rItem.streamRecordData.ulNumNewRecords * sizeof( StreamRecord );

    case MonitorDevicesRecord:
        return offsetof(MonitorDataItem, deviceRecordData ) 
			+ sizeof( DeviceRecordMonitorData );

	case MonitorDataObject:
		return SIZEOF_MONITORDATA( objectData );

	case MonitorDataAudioPerf:
		return SIZEOF_MONITORDATA( audioPerfData );

	case MonitorDataGameObjPosition:
		return offsetof(MonitorDataItem, gameObjPositionData.positions )
			+ (in_rItem.gameObjPositionData.ulNumGameObjPositions + 
			   in_rItem.gameObjPositionData.ulNumListenerPositions) 
			   * sizeof( GameObjPositionMonitorData::Position );

	case MonitorDataBank:
		return offsetof(MonitorDataItem, bankData.szBankName )
			+ in_rItem.bankData.wStringSize;

	case MonitorDataPrepare:
		return SIZEOF_MONITORDATA( prepareData );

	case MonitorDataState:
		return SIZEOF_MONITORDATA( stateData );

	case MonitorDataSwitch:
		return SIZEOF_MONITORDATA( switchData );

	case MonitorDataParamChanged:
		return SIZEOF_MONITORDATA( paramChangedData );

	case MonitorDataEventTriggered:
		return SIZEOF_MONITORDATA( eventTriggeredData );

	case MonitorDataActionDelayed:
		return SIZEOF_MONITORDATA( actionDelayedData );

	case MonitorDataActionTriggered:
		return SIZEOF_MONITORDATA( actionTriggeredData );

	case MonitorDataBusNotif:
		return SIZEOF_MONITORDATA( busNotifData );

	case MonitorDataPath:
		return SIZEOF_MONITORDATA( pathData );

	case MonitorDataSoundBank:
		return SIZEOF_MONITORDATA( loadedSoundBankData );

	case MonitorDataEvent:
		return SIZEOF_MONITORDATA( eventPreparedData );

	case MonitorDataGameSync:
		return SIZEOF_MONITORDATA( gameSyncData );

	case MonitorDataSetParam:
		return SIZEOF_MONITORDATA( setParamData );

	case MonitorDataObjRegistration:
		return offsetof(MonitorDataItem, objRegistrationData.szName )
			+ in_rItem.objRegistrationData.wStringSize;

	case MonitorDataErrorCode:
	case MonitorDataMessageCode:
		return offsetof(MonitorDataItem, errorData1 )
			+ sizeof( ErrorMonitorData1 );

	case MonitorDataErrorString:
	case MonitorDataMessageString:
		return offsetof(MonitorDataItem, debugData.szMessage)
			+ in_rItem.debugData.wStringSize * sizeof( wchar_t );

	case MonitorDataMemoryPoolName:
		return offsetof(MonitorDataItem, memoryPoolNameData.szName)
			+ in_rItem.memoryPoolNameData.wStringSize * sizeof( wchar_t );

	case MonitorDataPipeline:
		return offsetof(MonitorDataItem, pipelineData.pipelines)
			+ in_rItem.pipelineData.numPipelines * sizeof( PipelineData );

	case MonitorDataMarkers:
		return offsetof(MonitorDataItem, markersData.szLabel )
			+ in_rItem.markersData.wStringSize;

	case MonitorDataOutput:
		return SIZEOF_MONITORDATA( outputData );

	case MonitorDataSegmentPosition:
		return offsetof(MonitorDataItem, segmentPositionData.positions)
			+ in_rItem.segmentPositionData.numPositions * sizeof( SegmentPositionData );

	case MonitorDataControllers:
		return offsetof( MonitorDataItem, controllerData.gameObjMask ) 
			+ in_rItem.controllerData.ulNumGameObjMask * sizeof( GameObjectControllerMaskPacket );
			
	case MonitorDataRTPCValues:
		return offsetof(MonitorDataItem, rtpcValuesData.rtpcValues )
			+ in_rItem.rtpcValuesData.ulNumRTPCValues * sizeof( RTPCValuesPacket );

	case MonitorDataMedia:
		return offsetof(MonitorDataItem, mediaPreparedData.bankIDs )
			+ in_rItem.mediaPreparedData.uArraySize * sizeof( AkBankID );
	case MonitorDataFeedback:
		return offsetof( AkMonitorData::MonitorDataItem, feedbackData ) + sizeof(AkMonitorData::FeedbackMonitorData);

	case MonitorDataFeedbackDevices:
		return offsetof( AkMonitorData::MonitorDataItem, feedbackDevicesData ) + sizeof(AkMonitorData::FeedbackDevicesMonitorData) 
			+ (in_rItem.feedbackDevicesData.usDeviceCount - 1) * sizeof(AkMonitorData::FeedbackDeviceIDMonitorData);

	case MonitorDataFeedbackGameObjs:
		return offsetof( AkMonitorData::MonitorDataItem, feedbackGameObjData ) + sizeof(AkMonitorData::FeedbackGameObjMonitorData) 
			+ (in_rItem.feedbackGameObjData.ulNumGameObjMask - 1) * sizeof(AkMonitorData::GameObjectPlayerMaskPacket);

	case MonitorDataResolveDialogue:
		return offsetof( AkMonitorData::MonitorDataItem, resolveDialogueData.aPath )
			+ in_rItem.resolveDialogueData.uPathSize * sizeof( AkArgumentValueID );

	default:
		AKASSERT( false && "Need accurate size for MonitorDataItem member" );
		return sizeof( MonitorDataItem );
	}
}

