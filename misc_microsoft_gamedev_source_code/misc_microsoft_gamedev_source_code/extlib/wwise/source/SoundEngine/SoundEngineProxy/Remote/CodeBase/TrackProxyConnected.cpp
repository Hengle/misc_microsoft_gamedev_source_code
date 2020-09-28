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

#include "TrackProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

#ifdef DEBUG_NEW
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


TrackProxyConnected::TrackProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

TrackProxyConnected::~TrackProxyConnected()
{
}

void TrackProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case ITrackProxy::MethodAddSource:
		{	
			//no remote set source
			break;
		}
	case ITrackProxy::MethodRemoveSource:
		{	
			//no remote set source
			break;
		}
	case ITrackProxy::MethodRemoveAllSources:
		{	
			//no remote set source
			break;
		}
	case ITrackProxy::MethodSetSrcParam:
		{
			TrackProxyCommandData::SetSrcParam setSrcParam;
			in_rSerializer.Get( setSrcParam );

			m_proxyLocal.SetSrcParam( setSrcParam.m_sourceID, setSrcParam.m_fxID, setSrcParam.m_paramID, setSrcParam.m_pvData, setSrcParam.m_ulSize );
			break;
		}
	case ITrackProxy::MethodSetPlayList:
		{
			//no remote set playlist due to source collapse by SB generator.
			/**
			TrackProxyCommandData::SetPlayList setPlayList;
			in_rSerializer.Get( setPlayList );

			m_proxyLocal.SetPlayList( setPlayList.m_uNumPlaylistItem, setPlayList.m_pArrayPlaylistItems, setPlayList.m_uNumSubTrack );
			**/
			break;
		}
	case ITrackProxy::MethodLoop:
		{
			TrackProxyCommandData::Loop loop;
			in_rSerializer.Get( loop );
			
			m_proxyLocal.Loop( loop.m_bIsLoopEnabled, 
								loop.m_bIsLoopInfinite, 
								loop.m_loopCount, 
								loop.m_countModMin, 
								loop.m_countModMax );
			break;
		}
	case ITrackProxy::MethodIsStreaming:
		{
			TrackProxyCommandData::IsStreaming isStreaming;
			in_rSerializer.Get( isStreaming );

			m_proxyLocal.IsStreaming( isStreaming.m_bIsStreaming );
			break;
		}

	case ITrackProxy::MethodIsZeroLatency:
		{
			TrackProxyCommandData::IsZeroLatency isZeroLatency;
			in_rSerializer.Get( isZeroLatency );

			m_proxyLocal.IsZeroLatency( isZeroLatency.m_bIsZeroLatency );
			break;
		}
	case ITrackProxy::MethodLookAheadTime:
		{
			TrackProxyCommandData::LookAheadTime lookAheadTime;
			in_rSerializer.Get( lookAheadTime );

			m_proxyLocal.LookAheadTime( lookAheadTime.m_LookAheadTime );
			break;
		}
	case ITrackProxy::MethodSetMusicTrackRanSeqType:
		{
			TrackProxyCommandData::SetMusicTrackRanSeqType setMusicTrackRanSeqType;
			in_rSerializer.Get( setMusicTrackRanSeqType );

			m_proxyLocal.SetMusicTrackRanSeqType( setMusicTrackRanSeqType.m_eRSType );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
