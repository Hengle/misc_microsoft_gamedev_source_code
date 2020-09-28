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

#include "SegmentProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

#ifdef DEBUG_NEW
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


SegmentProxyConnected::SegmentProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

SegmentProxyConnected::~SegmentProxyConnected()
{
}

void SegmentProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case ISegmentProxy::MethodDuration:
		{
			MusicSegmentProxyCommandData::Duration duration;
			in_rSerializer.Get( duration );
			
			m_proxyLocal.Duration( duration.m_fDuration );
			break;
		}
	case ISegmentProxy::MethodStartPos:
		{
			MusicSegmentProxyCommandData::StartPos startPos;
			in_rSerializer.Get( startPos );
			
			m_proxyLocal.StartPos( startPos.m_fStartPos );
			break;
		}

	case ISegmentProxy::MethodRemoveMarkers:
		{
			MusicSegmentProxyCommandData::RemoveMarkers removeMarkers;
			in_rSerializer.Get( removeMarkers );

			m_proxyLocal.RemoveMarkers();
			break;
		}

	case ISegmentProxy::MethodSetMarkers:
		{
			MusicSegmentProxyCommandData::SetMarkers setMarkers;
			in_rSerializer.Get( setMarkers );

			m_proxyLocal.SetMarkers( setMarkers.m_pArrayMarkers, setMarkers.m_ulNumMarkers );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
