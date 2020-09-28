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
#include "CommandData.h"
#include "FeedbackNodeProxyConnected.h"


FeedbackNodeProxyConnected::FeedbackNodeProxyConnected( AkUniqueID in_id )
	:m_proxy(in_id)
{
}

FeedbackNodeProxyConnected::~FeedbackNodeProxyConnected()
{
}

void FeedbackNodeProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;
	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	FeedbackNodeProxyLocal& rLocalProxy = static_cast<FeedbackNodeProxyLocal&>( GetLocalProxy() );
	switch( cmdData.m_methodID )
	{	
		case IFeedbackNodeProxy::MethodAddPluginSource:
		{
			FeedbackNodeProxyCommandData::AddPluginSource methodData;
			in_rSerializer.Get( methodData );
			rLocalProxy.AddPluginSource(methodData.in_srcID, methodData.in_ulID, methodData.in_pParams, methodData.in_uSize, methodData.in_idDeviceCompany, methodData.in_idDevicePlugin);
			break;
		}
		case IFeedbackNodeProxy::MethodSetSrcParam:
		{
			FeedbackNodeProxyCommandData::SetSrcParam methodData;
			in_rSerializer.Get( methodData );
			rLocalProxy.SetSrcParam( methodData.m_idSource, methodData.m_fxID, methodData.m_paramID, methodData.m_pvData, methodData.m_ulSize );
			break;
		}
		case IFeedbackNodeProxy::MethodIsZeroLatency:
		{
			FeedbackNodeProxyCommandData::IsZeroLatency methodData;
			in_rSerializer.Get( methodData );
			rLocalProxy.IsZeroLatency(methodData.in_bIsZeroLatency);
			break;
		}
		case IFeedbackNodeProxy::MethodLookAheadTime:
		{
			FeedbackNodeProxyCommandData::LookAheadTime methodData;
			in_rSerializer.Get( methodData );
			rLocalProxy.LookAheadTime(methodData.in_LookAheadTime);
			break;
		}
		case IFeedbackNodeProxy::MethodLoop:
			{
				FeedbackNodeProxyCommandData::Loop methodData;
				in_rSerializer.Get( methodData );
				rLocalProxy.Loop( methodData.m_bIsLoopEnabled, 
					methodData.m_bIsLoopInfinite, 
					methodData.m_loopCount, 
					methodData.m_countModMin, 
					methodData.m_countModMax );
				break;
			}
		case IFeedbackNodeProxy::MethodSetSourceVolumeOffset:
			{
				FeedbackNodeProxyCommandData::SetSourceVolumeOffset methodData;
				in_rSerializer.Get( methodData );
				rLocalProxy.SetSourceVolumeOffset(methodData.in_srcID, methodData.in_fOffset);
				break;
			}

		default:
			__base::HandleExecute(in_rSerializer, out_rReturnSerializer);
			break;
	}
}
