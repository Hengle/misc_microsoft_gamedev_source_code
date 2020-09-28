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

#include "EventProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


EventProxyConnected::EventProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
	//Must Clear the Actions in the Event to ensure it will totally match the content of Wwise
	m_proxyLocal.Clear();
}

EventProxyConnected::~EventProxyConnected()
{
}

void EventProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IEventProxy::MethodAdd:
		{
			EventProxyCommandData::Add add;
			in_rSerializer.Get( add );
			
			m_proxyLocal.Add( add.m_actionID );

			break;
		}

	case IEventProxy::MethodRemove:
		{
			EventProxyCommandData::Remove remove;
			in_rSerializer.Get( remove );

			m_proxyLocal.Remove( remove.m_actionID );
			break;
		}

	case IEventProxy::MethodClear:
		{
			EventProxyCommandData::Clear clear;
			in_rSerializer.Get( clear );

			m_proxyLocal.Clear();
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
