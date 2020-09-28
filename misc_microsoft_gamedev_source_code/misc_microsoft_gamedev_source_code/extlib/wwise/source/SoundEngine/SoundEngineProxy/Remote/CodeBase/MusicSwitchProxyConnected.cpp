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

#include "MusicSwitchProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

#ifdef DEBUG_NEW
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


MusicSwitchProxyConnected::MusicSwitchProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

MusicSwitchProxyConnected::~MusicSwitchProxyConnected()
{
}

void MusicSwitchProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	MusicSwitchProxyLocal& rLocalProxy = static_cast<MusicSwitchProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IMusicSwitchProxy::MethodSetSwitchAssocs:
		{
			MusicSwitchProxyCommandData::SetSwitchAssocs setSwitchAssocs;
			in_rSerializer.Get( setSwitchAssocs );

			rLocalProxy.SetSwitchAssocs( setSwitchAssocs.m_uNumAssocs, setSwitchAssocs.m_pAssocs );
			break;
		}
	case IMusicSwitchProxy::MethodSetDefaultSwitch:
		{
			MusicSwitchProxyCommandData::SetDefaultSwitch setDefaultSwitch;
			in_rSerializer.Get( setDefaultSwitch );

			rLocalProxy.SetDefaultSwitch( setDefaultSwitch.m_Switch );
			break;
		}
	case IMusicSwitchProxy::MethodSetSwitchGroup:
		{
			MusicSwitchProxyCommandData::SetSwitchGroup setSwitchGroup;
			in_rSerializer.Get( setSwitchGroup );

			rLocalProxy.SetSwitchGroup( setSwitchGroup.m_ulGroup, setSwitchGroup.m_eGroupType );
			break;
		}
	case IMusicSwitchProxy::MethodContinuePlayback:
		{
			MusicSwitchProxyCommandData::ContinuePlayback continuePlayback;
			in_rSerializer.Get( continuePlayback );

			rLocalProxy.ContinuePlayback( continuePlayback.m_bContinuePlayback );
			break;
		}
	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
