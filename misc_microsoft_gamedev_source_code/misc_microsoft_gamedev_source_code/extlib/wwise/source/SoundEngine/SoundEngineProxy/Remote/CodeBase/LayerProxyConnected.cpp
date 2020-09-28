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

//////////////////////////////////////////////////////////////////////
//
// LayerProxyConnected.cpp
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "LayerProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


LayerProxyConnected::LayerProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

void LayerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	LayerProxyLocal& rLocalProxy = static_cast<LayerProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
		case ILayerProxy::MethodSetRTPC:
			{
				LayerProxyCommandData::SetRTPC setRTPC;
				in_rSerializer.Get( setRTPC );
				
				rLocalProxy.SetRTPC( setRTPC.m_RTPCID, setRTPC.m_paramID, setRTPC.m_RTPCCurveID, setRTPC.m_eScaling, setRTPC.m_pArrayConversion, setRTPC.m_ulConversionArraySize );

				break;
			}

		case ILayerProxy::MethodUnsetRTPC:
			{
				LayerProxyCommandData::UnsetRTPC unsetRTPC;
				in_rSerializer.Get( unsetRTPC );
				
				rLocalProxy.UnsetRTPC( unsetRTPC.m_paramID, unsetRTPC.m_RTPCCurveID );

				break;
			}

		case ILayerProxy::MethodSetChildAssoc:
			{
				LayerProxyCommandData::SetChildAssoc setChildAssoc;
				in_rSerializer.Get( setChildAssoc );

				rLocalProxy.SetChildAssoc( setChildAssoc.m_ChildID, setChildAssoc.m_pCrossfadingCurve, setChildAssoc.m_ulCrossfadingCurveSize );

				break;
			}

		case ILayerProxy::MethodUnsetChildAssoc:
			{
				LayerProxyCommandData::UnsetChildAssoc clearChildAssoc;
				in_rSerializer.Get( clearChildAssoc );
				
				rLocalProxy.UnsetChildAssoc( clearChildAssoc.m_ChildID );

				break;
			}

		case ILayerProxy::MethodSetCrossfadingRTPC:
			{
				LayerProxyCommandData::SetCrossfadingRTPC setCrossfadingRTPC;
				in_rSerializer.Get( setCrossfadingRTPC );
				
				rLocalProxy.SetCrossfadingRTPC( setCrossfadingRTPC.m_rtpcID );

				break;
			}

		case ILayerProxy::MethodSetCrossfadingRTPCDefaultValue:
			{
				LayerProxyCommandData::SetCrossfadingRTPCDefaultValue setCrossfadingRTPCMidValue;
				in_rSerializer.Get( setCrossfadingRTPCMidValue );
				
				rLocalProxy.SetCrossfadingRTPCDefaultValue( setCrossfadingRTPCMidValue.m_fValue );

				break;
			}

		default:
			__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

