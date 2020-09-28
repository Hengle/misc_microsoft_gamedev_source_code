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


#include "stdafx.h"

#include "ALMonitorProxyLocal.h"

#include "AkAudiolib.h"
#include "IALMonitor.h"
#include "AkMonitorData.h"


ALMonitorProxyLocal::ALMonitorProxyLocal( IALMonitorSink* in_pHolder )
	: m_pHolder( in_pHolder )
	, m_bRegistered( false )
	, m_bUnregistering( false )
{
	AKASSERT( m_pHolder );
}

ALMonitorProxyLocal::~ALMonitorProxyLocal()
{
	StopMonitor();
}

void ALMonitorProxyLocal::Monitor( AkMonitorData::MonitorDataType in_uWhatToMonitor )
{
	m_bUnregistering = false;

    AK::SoundEngine::GetMonitor()->Register( this, in_uWhatToMonitor );

	m_bRegistered = true;
}

void ALMonitorProxyLocal::StopMonitor()
{
	if( m_bRegistered )
	{
		m_bUnregistering = true;

        AK::SoundEngine::GetMonitor()->Unregister( this );

		m_bRegistered = false;
	}
}

void ALMonitorProxyLocal::SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount )
{
	AK::SoundEngine::GetMonitor()->SetWatches( in_pWatches, in_uiWatchCount );
}

void ALMonitorProxyLocal::SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount )
{
	AK::SoundEngine::GetMonitor()->SetGameSyncWatches( in_pWatches, in_uiWatchCount );
}

void ALMonitorProxyLocal::MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem )
{
	if( m_bUnregistering )
		return;

	m_pHolder->MonitorNotification( in_rMonitorItem );
}
