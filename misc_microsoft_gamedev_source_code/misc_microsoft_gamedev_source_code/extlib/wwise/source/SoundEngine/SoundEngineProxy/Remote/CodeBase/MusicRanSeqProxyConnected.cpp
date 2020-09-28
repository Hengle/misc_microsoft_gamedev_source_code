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

#include "MusicRanSeqProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

#ifdef DEBUG_NEW
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


MusicRanSeqProxyConnected::MusicRanSeqProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

MusicRanSeqProxyConnected::~MusicRanSeqProxyConnected()
{
}

void MusicRanSeqProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	MusicRanSeqProxyLocal& rLocalProxy = static_cast<MusicRanSeqProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IMusicRanSeqProxy::MethodSetPlayList:
		{
			MusicRanSeqProxyCommandData::SetPlayList setPlayList;
			in_rSerializer.Get( setPlayList );

			rLocalProxy.SetPlayList( setPlayList.m_pArrayItems, setPlayList.m_NumItems );
			break;
		}
	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
