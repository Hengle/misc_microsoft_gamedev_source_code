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
// AkMonitor.h
//
// alessard
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKMONITOR_H_
#define _AKMONITOR_H_

#include "IALMonitor.h"
#include <AK/Tools/Common/AkLock.h>

#include "AkMonitorData.h"

#include "AkChunkRing.h"
#include "AkKeyList.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkIDStringMap.h"
#include "AkStateMgr.h"

using namespace AKPLATFORM;

class CAkUsageSlot;
class AkMediaEntry;

class AkMonitor : public CAkObject,
				  public AK::IALMonitor
                  
{
public:
	//Singleton
	static AkMonitor* Instance();
	static AkMonitor* Get() { return m_pInstance; }
	static void Destroy();

	// IALMonitor members
    virtual void Register( AK::IALMonitorSink* in_pMonitorSink, AkMonitorData::MonitorDataType in_whatToMonitor );
    virtual void Unregister( AK::IALMonitorSink* in_pMonitorSink );
	virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount );
	virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount );

	void RecapRegisteredObjects( AK::IALMonitorSink* in_pMonitorSink );
	void RecapSwitches( AK::IALMonitorSink* in_pMonitorSink );
	void RecapMemoryPools( AK::IALMonitorSink* in_pMonitorSink );
    void RecapDevices( AK::IALMonitorSink* in_pMonitorSink );
    void RecapStreamRecords( AK::IALMonitorSink* in_pMonitorSink );

	void RecapDataSoundBank( AK::IALMonitorSink* in_pMonitorSink );
	void RecapMedia( AK::IALMonitorSink* in_pMonitorSink );
	void RecapEvents( AK::IALMonitorSink* in_pMonitorSink );
	void RecapGameSync( AK::IALMonitorSink* in_pMonitorSink );

	// AkMonitor members
	AKRESULT StartMonitoring();
	void StopMonitoring();

	static bool IsMonitoring() { return !Get()->m_sink2Filter.IsEmpty(); }
	static AkMonitorData::MonitorDataType GetNotifFilter() { return Get()->m_uiNotifFilter; } // what is being monitored

	static void PostWatchedGameObjPositions();
	static void PostWatchesRTPCValues();

	static AkTimeMs GetThreadTime(){return m_ThreadTime;}
	static void SetThreadTime(AkTimeMs in_ThreadTime){m_ThreadTime = in_ThreadTime;}

	static void Monitor_PostCode( AK::Monitor::ErrorCode in_eErrorCode, AK::Monitor::ErrorLevel in_eErrorLevel, AkUInt32 in_param1 = 0 );
	static void Monitor_PostString( AkLpCtstr in_pszError, AK::Monitor::ErrorLevel in_eErrorLevel );
	static void Monitor_ObjectNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, AkUniqueID in_targetObjectID, AkTimeMs in_fadeTime, AkUniqueID in_playlistItemID = AK_INVALID_UNIQUE_ID );
	static void Monitor_MarkersNotif( AkPlayingID in_PlayingID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam, AkMonitorData::NotificationReason in_eNotifReason, AkCntrHistArray in_cntrHistArray, AkLpCstr in_strLabel );
	static void Monitor_BankNotif( AkUniqueID in_BankID, AkUniqueID in_LanguageID, AkMonitorData::NotificationReason in_eNotifReason );
	static void Monitor_PrepareNotif( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_GameSyncorEventID, AkUInt32 in_groupID, AkGroupType in_GroupType, AkUInt32 in_NumEvents );
	static void Monitor_StateChanged( AkStateGroupID in_StateGroup, AkStateID in_PreviousState, AkStateID in_NewState );
	static void Monitor_SwitchChanged( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_Switch, AkGameObjectID in_GameObject );
	static void Monitor_ObjectRegistration( bool in_isRegistration, AkGameObjectID in_GameObject, void * in_pMonitorData );
	static void Monitor_FreeString( void * in_pMonitorData );
	static void Monitor_ParamChanged( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject );
	static void Monitor_EventTriggered( AkPlayingID in_PlayingID, AkUniqueID in_EventID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam);
	static void Monitor_ActionDelayed( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkTimeMs in_DelayTime, AkCustomParamType in_CustomParam );
	static void Monitor_ActionTriggered( AkPlayingID in_PlayingID, AkUniqueID in_ActionID, AkGameObjectID in_GameObject, AkCustomParamType in_CustomParam );
	static void Monitor_BusNotification( AkUniqueID in_BusID, AkMonitorData::BusNotification in_NotifReason, AkUInt32 in_bitsFXBypass, AkUInt32 in_bitsMask );
	static void Monitor_PathEvent( AkPlayingID _playingI_D, AkUniqueID _who_, AkMonitorData::AkPathEvent _event_, AkUInt32 _index_);

	static void Monitor_errorMsg2( AkLpCtstr in_psz1, AkLpCtstr in_psz2 );

	static void Monitor_LoadedBank( CAkUsageSlot* in_pUsageSlot, bool in_bIsDestroyed );
	static void Monitor_MediaPrepared( AkMediaEntry& in_rMediaEntry );
	static void Monitor_EventPrepared( AkUniqueID in_EventID, AkUInt32 in_RefCount );
	static void Monitor_GameSync(  AkUniqueID in_GroupID, AkUniqueID in_GameSyncID, bool in_bIsEnabled, AkGroupType in_GroupType  );
	
	static void Monitor_SetPoolName( AkMemPoolId in_PoolId, AkTChar * in_tcsPoolName );

	static void Monitor_SetParamNotif_Float( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject, AkReal32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime );
	static void Monitor_SetParamNotif_Long( AkMonitorData::NotificationReason in_eNotifReason, AkUniqueID in_ElementID, AkGameObjectID in_GameObject, AkInt32 in_TargetValue, AkValueMeaning in_ValueMeaning, AkTimeMs in_TransitionTime );
	static void Monitor_ResolveDialogue( AkUniqueID in_idDialogueEvent, AkUniqueID in_idResolved, AkUInt32 in_cPath, AkArgumentValueID * in_pPath );

	static void * Monitor_AllocateGameObjNameString( AkGameObjectID in_GameObject, const char* in_GameObjString );

	static AkMemPoolId Monitor_GetPoolId() { return Get()->m_MonitorPoolId; };

	static inline void SetLocalOutput( AkUInt32 in_uErrorLevel, AK::Monitor::LocalOutputFunc in_pMonitorFunc ) { m_uLocalOutputErrorLevel = in_uErrorLevel; m_funcLocalOutput = in_pMonitorFunc; }

protected:
	//Singleton
	AkMonitor();
    virtual ~AkMonitor();

private:
	void _RecapMediaEntry( AK::IALMonitorSink* in_pMonitorSink, AkMediaEntry& rMediaEntry );

	AkEvent m_hMonitorEvent;
	AkEvent m_hMonitorDoneEvent;
	bool	m_bStopThread;

	friend struct AkMonitorDataCreator;
	friend struct AkProfileDataCreator;

	bool DispatchNotification();

    void StartStreamProfiler( );
    void StopStreamProfiler( );

	static void AddWatchesByGameObjectName( AkLpCstr in_pszGameObjectName );
	static void AddWatchForGameObject( AkGameObjectID in_GameObject, AkLpCstr in_pszGameObjectName );
	static void RemoveWatchForGameObject( AkGameObjectID in_GameObject );

	static void RecapGroupHelper( AkMonitorData::MonitorDataItem& in_rMonitorDataItem, CAkStateMgr::PreparationGroups& in_Groups, AK::IALMonitorSink* in_pMonitorSink );

	static AkTimeMs m_ThreadTime;

    static AK_DECLARE_THREAD_ROUTINE( MonitorThreadFunc );

	static AkMonitor* m_pInstance; // Pointer on the unique Monitor instance

	static AkIDStringHash m_idxGameObjectString;

	AkThread		m_hThread;

	inline void SignalNotifyEvent()
	{
		AkSignalEvent( m_hMonitorEvent );
	};

    typedef CAkKeyList<AK::IALMonitorSink*, AkMonitorData::MonitorDataType, AkAllocAndFree> MonitorSink2Filter;
	MonitorSink2Filter m_sink2Filter;

	CAkLock m_registerLock;
	CAkLock m_unregisterLock;
	
	AkChunkRing m_ringItems;

	AkMonitorData::MonitorDataType m_uiNotifFilter; // Global filter 

	typedef AkHashList<AkGameObjectID, AkUInt32, 31> AkMapGameObjectWatch;
	static AkMapGameObjectWatch m_mapGameObjectWatch;
	static bool m_arrayListenerWatch[AK_NUM_LISTENERS];

	typedef AkArray<AkMonitorData::Watch,const AkMonitorData::Watch&,ArrayPoolDefault,1> WatchesArray;
	static WatchesArray m_watches;

	typedef AkArray<AkUniqueID,AkUniqueID,ArrayPoolDefault,1> GameSyncWatchesArray;
	static GameSyncWatchesArray m_gameSyncWatches;

	static AkMemPoolId m_MonitorQueuePoolId;
	static AkMemPoolId m_MonitorPoolId;
	static AkUInt32 m_uLocalOutputErrorLevel; // Bitfield of AK::Monitor::ErrorLevel
	static AK::Monitor::LocalOutputFunc m_funcLocalOutput;
};

// Use this to send a monitor data item: space is initialized in constructor, 
// item is 'queued' in destructor.
struct AkMonitorDataCreator
{
	AkMonitorDataCreator( AkMonitorData::MonitorDataType in_MonitorDataType, AkInt32 in_lSize )
		: m_lSize( in_lSize )
		, m_pData( NULL )
	{
		AkMonitor * pMonitor = AkMonitor::Get();

		// Note: Insufficient memory errors can land here after the monitor manager was term()ed.
		if ( !pMonitor || pMonitor->m_sink2Filter.IsEmpty() )
			return;
	
		// Retry queuing data until there is enough space. Another thread is emptying the queue
		// and signals MonitorDone when going back to sleep.
		while ( !( m_pData = (AkMonitorData::MonitorDataItem *) pMonitor->m_ringItems.BeginWrite( m_lSize ) ) )
			AkWaitForEvent( pMonitor->m_hMonitorDoneEvent );

		m_pData->eDataType = in_MonitorDataType;
		m_pData->timeStamp = AkMonitor::GetThreadTime();
	}

	~AkMonitorDataCreator()
	{
		if ( !m_pData )
			return;

		AKASSERT( m_pData->eDataType );
		AKASSERT( m_lSize == AkMonitorData::RealSizeof( *m_pData ) );

		AkMonitor * pMonitor = AkMonitor::Get();

		pMonitor->m_ringItems.EndWrite( m_pData, m_lSize );
		pMonitor->SignalNotifyEvent();
	}

	AkMonitorData::MonitorDataItem * m_pData;

private:
	AkInt32 m_lSize;
};

// Use this to send a profiling data item: space is initialized in constructor, 
// item is 'queued' in destructor. The difference between this and AkMonitorDataCreator
// is that this one doesn't block on a full queue, it just skips the item (as profiling info
// is not 'critical')
struct AkProfileDataCreator
{
	AkProfileDataCreator( AkInt32 in_lSize )
		: m_lSize( in_lSize )
		, m_pData( NULL )
	{
		AkMonitor * pMonitor = AkMonitor::Get();

		if ( pMonitor->m_sink2Filter.IsEmpty() )
			return;
	
		// Try once to write in the queue.
		m_pData = (AkMonitorData::MonitorDataItem *) pMonitor->m_ringItems.BeginWrite( m_lSize );
	}

	~AkProfileDataCreator()
	{
		if ( !m_pData )
			return;

		AKASSERT( m_pData->eDataType );
		AKASSERT( m_lSize == AkMonitorData::RealSizeof( *m_pData ) );

		AkMonitor * pMonitor = AkMonitor::Get();

		pMonitor->m_ringItems.EndWrite( m_pData, m_lSize );
		pMonitor->SignalNotifyEvent();
	}

	AkMonitorData::MonitorDataItem * m_pData;

private:
	AkInt32 m_lSize;
};

//Please use macros

//The monitor is started and Stopped no matter the build type

#ifndef AK_OPTIMIZED

#define MONITOR_BANKNOTIF( in_BankID, in_LanguageID, in_eNotifReason )\
		AkMonitor::Monitor_BankNotif( in_BankID, in_LanguageID, in_eNotifReason )

#define MONITOR_PREPAREEVENTNOTIF( in_eNotifReason, in_EventID )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, in_EventID, 0, AkGroupType_Switch, 0 )

#define MONITOR_PREPARENOTIFREQUESTED( in_eNotifReason, in_NumNotifs )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, 0, 0, AkGroupType_Switch, in_NumNotifs )

#define MONITOR_PREPAREGAMESYNCNOTIF( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType )\
		AkMonitor::Monitor_PrepareNotif( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType, 0 )

#define MONITOR_STATECHANGED(in_StateGroup, in_PreviousState, in_NewState)\
		AkMonitor::Monitor_StateChanged(in_StateGroup, in_PreviousState, in_NewState)

#define MONITOR_SWITCHCHANGED( in_SwitchGroup, in_Switch, in_GameObject )\
		AkMonitor::Monitor_SwitchChanged( in_SwitchGroup, in_Switch, in_GameObject )

#define MONITOR_OBJREGISTRATION( in_isRegistration, in_GameObject, in_GameObjString )\
		AkMonitor::Monitor_ObjectRegistration( in_isRegistration, in_GameObject, in_GameObjString )

#define MONITOR_FREESTRING( in_GameObjString )\
		AkMonitor::Monitor_FreeString( in_GameObjString )

#define MONITOR_OBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_fadeTime )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_fadeTime )

#define MONITOR_MUSICOBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_targetObjectID, in_playlistItemID )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, CAkCntrHist(), in_targetObjectID, 0, in_playlistItemID );

#define MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_ )\
		AkMonitor::Monitor_ObjectNotif( in_PlayingID, in_GameObject, in_CustomParam, AkMonitorData::NotificationReason_EventEndReached, CAkCntrHist(), 0, 0 );

#define MONITOR_EVENTMARKERNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_, in_strLabel )\
		AkMonitor::Monitor_MarkersNotif( in_PlayingID, in_GameObject, in_CustomParam, AkMonitorData::NotificationReason_EventMarker, CAkCntrHist(), in_strLabel );

#define MONITOR_PARAMCHANGED( in_eNotifReason, in_ElementID, in_GameObject )\
		AkMonitor::Monitor_ParamChanged( in_eNotifReason, in_ElementID, in_GameObject )

#define MONITOR_SETPARAMNOTIF_FLOAT( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) \
		AkMonitor::Monitor_SetParamNotif_Float( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime )

#define MONITOR_SETPARAMNOTIF_LONG( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) \
		AkMonitor::Monitor_SetParamNotif_Long( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime )

#define MONITOR_ERROR( in_ErrorCode )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Error )

#define MONITOR_ERROR1( in_ErrorCode, in_param1 )\
		AkMonitor::Monitor_PostCode( in_ErrorCode, AK::Monitor::ErrorLevel_Error, in_param1 )

#define MONITOR_EVENTTRIGGERED( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )\
		AkMonitor::Monitor_EventTriggered( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )

#define MONITOR_ACTIONDELAYED( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )	\
		AkMonitor::Monitor_ActionDelayed( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )

#define MONITOR_ACTIONTRIGGERED( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )	\
		AkMonitor::Monitor_ActionTriggered( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )

#define MONITOR_BUSNOTIFICATION( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )\
		AkMonitor::Monitor_BusNotification( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )	

#define MONITOR_PATH_EVENT( _playingID_, _who_, _event_, _index_ )\
		AkMonitor::Monitor_PathEvent( _playingID_, _who_, _event_, _index_ )

#define MONITOR_ERRORMSG2( _MSG1_, _MSG2_ )\
		AkMonitor::Monitor_errorMsg2( _MSG1_, _MSG2_ )

#define MONITOR_MSG( _MSG_ )\
		AkMonitor::Monitor_PostString( _MSG_, AK::Monitor::ErrorLevel_Message );

#define MONITOR_LOADEDBANK( _BankSlot_, _IsDestroyed_ )\
		AkMonitor::Monitor_LoadedBank( _BankSlot_, _IsDestroyed_ )

#define MONITOR_MEDIAPREPARED( _MediaItem_ )\
		AkMonitor::Monitor_MediaPrepared( _MediaItem_ )

#define MONITOR_EVENTPREPARED( _EventID_, _RefCount_ )\
		AkMonitor::Monitor_EventPrepared( _EventID_, _RefCount_ )

#define MONITOR_GAMESYNC( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )\
		AkMonitor::Monitor_GameSync( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )

#define MONITOR_RESOLVEDIALOGUE( in_idDialogueEvent, in_idResolved, in_cPath, in_pPath )\
		AkMonitor::Monitor_ResolveDialogue( in_idDialogueEvent, in_idResolved, in_cPath, in_pPath )
#else

#define MONITOR_BANKNOTIF( in_BankID, in_LanguageID, in_eNotifReason )

#define MONITOR_PREPAREEVENTNOTIF( in_eNotifReason, in_EventID )

#define MONITOR_PREPARENOTIFREQUESTED( in_eNotifReason, in_NumNotifs )

#define MONITOR_PREPAREGAMESYNCNOTIF( in_eNotifReason, in_GameSyncID, in_GroupID, in_GroupType )

#define MONITOR_STATECHANGED(in_StateGroup, in_PreviousState, in_NewState)

#define MONITOR_SWITCHCHANGED( in_SwitchGroup, in_Switch, in_GameObject )

#define MONITOR_OBJREGISTRATION( in_isRegistration, in_GameObject, in_GameObjString )

#define MONITOR_FREESTRING( in_GameObjString )

#define MONITOR_OBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_cntrHistArray, in_targetObjectID, in_fadeTime )

#define MONITOR_MUSICOBJECTNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_eNotifReason, in_targetObjectID, in_playlistItemID )

#define MONITOR_EVENTENDREACHEDNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_  )

#define MONITOR_EVENTMARKERNOTIF( in_PlayingID, in_GameObject, in_CustomParam, in_EventID_, in_strLabel  )

#define MONITOR_PARAMCHANGED( in_eNotifReason, in_ElementID, in_GameObject )

#define MONITOR_SETPARAMNOTIF_FLOAT( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) 

#define MONITOR_SETPARAMNOTIF_LONG( in_eNotifReason, in_ElementID, in_GameObject, in_TargetValue, in_ValueMeaning, in_TransitionTime ) 

#define MONITOR_ERROR( in_ErrorCode )

#define MONITOR_ERROR1( in_ErrorCode, in_param1 )

#define MONITOR_EVENTTRIGGERED( in_PlayingID, in_EventID, in_GameObject, in_CustomParam )

#define MONITOR_ACTIONDELAYED( in_PlayingID, in_ActionID, in_GameObject, in_DelayTime, in_CustomParam )

#define MONITOR_ACTIONTRIGGERED( in_PlayingID, in_ActionID, in_GameObject, in_CustomParam )

#define MONITOR_BUSNOTIFICATION( in_BusID, in_NotifReason, in_bitsFXBypass, in_bitsMask )

#define MONITOR_PATH_EVENT( _playingID_, _who_, _event_, _index_ )

#define MONITOR_ERRORMSG2( _MSG1_, _MSG2_ )

#define MONITOR_MSG( _MSG_ )

#define MONITOR_LOADEDBANK( _BankSlot_, _IsDestroyed_ )

#define MONITOR_MEDIAPREPARED( _MediaItem_ )

#define MONITOR_EVENTPREPARED( _EventID_, _RefCount_ )

#define MONITOR_GAMESYNC( _GroupID_, _SyncID_, _IsEnabled_, _SyncType_ )

#define MONITOR_RESOLVEDIALOGUE( in_idDialogueEvent, in_idResolved, in_cPath, in_pPath )

#endif

#endif	// _AKMONITOR_H_
