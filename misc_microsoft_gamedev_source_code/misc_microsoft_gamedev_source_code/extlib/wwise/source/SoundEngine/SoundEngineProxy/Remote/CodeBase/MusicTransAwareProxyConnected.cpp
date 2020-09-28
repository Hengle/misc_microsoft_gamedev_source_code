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
#include "MusicTransAwareProxyConnected.h"
#include "CommandData.h"

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


MusicTransAwareProxyConnected::MusicTransAwareProxyConnected()
{
}

MusicTransAwareProxyConnected::~MusicTransAwareProxyConnected()
{
}

void MusicTransAwareProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	MusicTransAwareProxyLocal& rLocalProxy = static_cast<MusicTransAwareProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IMusicTransAwareProxy::MethodSetRules:
		{
			MusicTransAwareProxyCommandData::SetRules setRules;
			in_rSerializer.Get( setRules );

			rLocalProxy.SetRules( setRules.m_NumRules, setRules.m_pRules );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
