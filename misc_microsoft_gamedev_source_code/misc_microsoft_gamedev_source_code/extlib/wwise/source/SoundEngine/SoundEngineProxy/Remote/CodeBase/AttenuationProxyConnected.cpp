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

#include "AttenuationProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


AttenuationProxyConnected::AttenuationProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

AttenuationProxyConnected::~AttenuationProxyConnected()
{
}

void AttenuationProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData objectData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( objectData );
	}

	switch( objectData.m_methodID )
	{
	case IAttenuationProxy::MethodSetAttenuationParams:
		{
			AttenuationProxyCommandData::SetAttenuationParams setAttenuationParams;
			in_rSerializer.Get( setAttenuationParams );

			m_proxyLocal.SetAttenuationParams( setAttenuationParams.m_Params );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
