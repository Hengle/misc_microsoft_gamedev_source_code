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

#include "LayerContainerProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


LayerContainerProxyConnected::LayerContainerProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

void LayerContainerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	LayerContainerProxyLocal& rLocalProxy = static_cast<LayerContainerProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
		case ILayerContainerProxy::MethodAddLayer:
			{
				LayerContainerProxyCommandData::AddLayer addLayer;
				in_rSerializer.Get( addLayer );
				
				rLocalProxy.AddLayer( addLayer.m_LayerID );

				break;
			}

		case ILayerContainerProxy::MethodRemoveLayer:
			{
				LayerContainerProxyCommandData::RemoveLayer removeLayer;
				in_rSerializer.Get( removeLayer );
				
				rLocalProxy.RemoveLayer( removeLayer.m_LayerID );

				break;
			}

		default:
			__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
