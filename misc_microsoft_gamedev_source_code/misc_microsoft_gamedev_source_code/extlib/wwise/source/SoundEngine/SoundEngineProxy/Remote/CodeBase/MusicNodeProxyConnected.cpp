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

#include "CommandDataSerializer.h"
#include "MusicNodeProxyConnected.h"
#include "CommandData.h"

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


MusicNodeProxyConnected::MusicNodeProxyConnected()
{
}

MusicNodeProxyConnected::~MusicNodeProxyConnected()
{
}

void MusicNodeProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	MusicNodeProxyLocal& rLocalProxy = static_cast<MusicNodeProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IMusicNodeProxy::MethodMeterInfo:
		{
			MusicNodeProxyCommandData::MeterInfo meterInfo;
			in_rSerializer.Get( meterInfo );

			rLocalProxy.MeterInfo( meterInfo.m_bIsOverrideParent, meterInfo.m_MeterInfo );
			break;
		}

	case IMusicNodeProxy::MethodSetStingers:
		{
			MusicNodeProxyCommandData::SetStingers setStingers;
			in_rSerializer.Get( setStingers );

			rLocalProxy.SetStingers( setStingers.m_pStingers, setStingers.m_NumStingers );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
