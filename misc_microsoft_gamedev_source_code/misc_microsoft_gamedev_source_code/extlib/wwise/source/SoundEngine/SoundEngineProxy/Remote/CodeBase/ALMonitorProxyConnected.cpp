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

#include "ALMonitorProxyConnected.h"
#include "INotificationChannel.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


ALMonitorProxyConnected::ALMonitorProxyConnected()
	: m_pNotificationChannel( NULL) 
	, m_localProxy( this )
{
}

ALMonitorProxyConnected::~ALMonitorProxyConnected()
{
}

void ALMonitorProxyConnected::MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem )
{
	AKASSERT( m_pNotificationChannel );
	
	CommandDataSerializer serializer;
	serializer.Put( in_rMonitorItem );

	m_pNotificationChannel->SendNotification( serializer.GetWrittenBytes(), serializer.GetWrittenSize() );
}

void ALMonitorProxyConnected::SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel )
{
	m_pNotificationChannel = in_pNotificationChannel;
}

void ALMonitorProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rSerializer )
{
	ALMonitorProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IALMonitorSubProxy::MethodMonitor:
		{
			ALMonitorProxyCommandData::Monitor monitor;
			in_rSerializer.Get( monitor );

			m_localProxy.Monitor( monitor.m_uWhatToMonitor );
			break;
		}

	case IALMonitorSubProxy::MethodStopMonitor:
		{
			ALMonitorProxyCommandData::StopMonitor stopMonitor;
			in_rSerializer.Get( stopMonitor );
				
			m_localProxy.StopMonitor();

			out_rSerializer.Put(true);
			break;
		}

	case IALMonitorSubProxy::MethodSetWatches:
		{
			ALMonitorProxyCommandData::SetWatches setWatches;
			in_rSerializer.Get( setWatches );
				
			m_localProxy.SetWatches( setWatches.m_pWatches, setWatches.m_uiWatchCount );
			break;
		}

	case IALMonitorSubProxy::MethodSetGameSyncWatches:
		{
			ALMonitorProxyCommandData::SetGameSyncWatches setGameSyncWatches;
			in_rSerializer.Get( setGameSyncWatches );
				
			m_localProxy.SetGameSyncWatches( setGameSyncWatches.m_pWatches, setGameSyncWatches.m_uiWatchCount );
			break;
		}

	default:
		AKASSERT( !"Unsupported command." );
	}
}

ALMonitorProxyLocal& ALMonitorProxyConnected::GetLocalProxy()
{
	return m_localProxy;
}
