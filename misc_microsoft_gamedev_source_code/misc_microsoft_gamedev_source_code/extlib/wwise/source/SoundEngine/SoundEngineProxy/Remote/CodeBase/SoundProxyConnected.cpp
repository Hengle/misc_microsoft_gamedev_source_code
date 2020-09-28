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

#include "SoundProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


SoundProxyConnected::SoundProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

SoundProxyConnected::~SoundProxyConnected()
{
}

void SoundProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case ISoundProxy::MethodSetSource:
		{
			SoundProxyCommandData::SetSource setSource;
			in_rSerializer.Get( setSource );
			
			SetSource( setSource.m_pszSourceName );
			break;
		}

	case ISoundProxy::MethodSetSource_Plugin:
		{
			SoundProxyCommandData::SetSource_Plugin setSource_Plugin;
			in_rSerializer.Get( setSource_Plugin );

			m_proxyLocal.SetSource( setSource_Plugin.m_fxID, setSource_Plugin.m_pvData, setSource_Plugin.m_ulSize );
			break;
		}

	case ISoundProxy::MethodSetSrcParam:
		{
			SoundProxyCommandData::SetSrcParam setSrcParam;
			in_rSerializer.Get( setSrcParam );

			m_proxyLocal.SetSrcParam( setSrcParam.m_fxID, setSrcParam.m_paramID, setSrcParam.m_pvData, setSrcParam.m_ulSize );
			break;
		}

	case ISoundProxy::MethodLoop:
		{
			SoundProxyCommandData::Loop loop;
			in_rSerializer.Get( loop );
			
			m_proxyLocal.Loop( loop.m_bIsLoopEnabled, 
								loop.m_bIsLoopInfinite, 
								loop.m_loopCount, 
								loop.m_countModMin, 
								loop.m_countModMax );
			break;
		}

	case ISoundProxy::MethodIsZeroLatency:
		{
			SoundProxyCommandData::IsZeroLatency isZeroLatency;
			in_rSerializer.Get( isZeroLatency );

			m_proxyLocal.IsZeroLatency( isZeroLatency.m_bIsZeroLatency );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

void SoundProxyConnected::SetSource( char* in_pszSource )
{
//	static_cast<SoundProxyLocal&>( GetLocalProxy() ).SetSource( CString( "w:\\TestSounds\\" ) + in_pszSource );
}
