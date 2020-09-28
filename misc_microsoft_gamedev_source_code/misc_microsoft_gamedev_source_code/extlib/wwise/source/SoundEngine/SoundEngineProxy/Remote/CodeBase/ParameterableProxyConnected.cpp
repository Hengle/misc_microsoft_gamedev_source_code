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

#include "ParameterableProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


ParameterableProxyConnected::ParameterableProxyConnected()
{
}

ParameterableProxyConnected::~ParameterableProxyConnected()
{
}

void ParameterableProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ParameterableProxyLocal& rLocalProxy = static_cast<ParameterableProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IParameterableProxy::MethodPriority:
		{
			ParameterableProxyCommandData::Priority priority;
			in_rSerializer.Get( priority );
		
			rLocalProxy.Priority( priority.m_priority );
			break;
		}

	case IParameterableProxy::MethodPriorityApplyDistFactor:
		{
			ParameterableProxyCommandData::PriorityApplyDistFactor priorityApplyDistFactor;
			in_rSerializer.Get( priorityApplyDistFactor );

			rLocalProxy.PriorityApplyDistFactor( priorityApplyDistFactor.m_bApplyDistFactor );
			break;
		}

	case IParameterableProxy::MethodPriorityDistanceOffset:
		{
			ParameterableProxyCommandData::PriorityDistanceOffset priorityDistanceOffset;
			in_rSerializer.Get( priorityDistanceOffset );

			rLocalProxy.PriorityDistanceOffset( priorityDistanceOffset.m_iDistOffset );
			break;
		}

	case IParameterableProxy::MethodPriorityOverrideParent:
		{
			ParameterableProxyCommandData::PriorityOverrideParent priorityOverrideParent;
			in_rSerializer.Get( priorityOverrideParent );

			rLocalProxy.PriorityOverrideParent( priorityOverrideParent.m_bOverrideParent );
			break;
		}

	case IParameterableProxy::MethodSetStateGroup:
		{
			ParameterableProxyCommandData::SetStateGroup setStateGroup;
			in_rSerializer.Get( setStateGroup );

			rLocalProxy.SetStateGroup( setStateGroup.m_stateGroupID );
			break;
		}

	case IParameterableProxy::MethodUnsetStateGroup:
		{
			ParameterableProxyCommandData::UnsetStateGroup unsetStateGroup;
			in_rSerializer.Get( unsetStateGroup );

			rLocalProxy.UnsetStateGroup();
			break;
		}

	case IParameterableProxy::MethodAddState:
		{
			ParameterableProxyCommandData::AddState addState;
			in_rSerializer.Get( addState );
			
			rLocalProxy.AddState( addState.m_stateInstanceID, addState.m_stateID );

			break;
		}

	case IParameterableProxy::MethodRemoveState:
		{
			ParameterableProxyCommandData::RemoveState removeState;
			in_rSerializer.Get( removeState );
			
			rLocalProxy.RemoveState( removeState.m_stateID );

			break;
		}

	case IParameterableProxy::MethodRemoveAllStates:
		{
			ParameterableProxyCommandData::RemoveAllStates removeAllStates;
			in_rSerializer.Get( removeAllStates );

			rLocalProxy.RemoveAllStates();

			break;
		}

	case IParameterableProxy::MethodUseState:
		{
			ParameterableProxyCommandData::UseState useState;
			in_rSerializer.Get( useState );

			rLocalProxy.UseState( useState.m_bUseState );
			break;
		}

	case IParameterableProxy::MethodLinkStateToStateDefault:
		{
			ParameterableProxyCommandData::LinkStateToStateDefault linkStateToStateDefault;
			in_rSerializer.Get( linkStateToStateDefault );
			
			rLocalProxy.LinkStateToStateDefault( linkStateToStateDefault.m_stateID );

			break;
		}

	case IParameterableProxy::MethodSetStateSyncType:
		{
			ParameterableProxyCommandData::SetStateSyncType setStateSyncType;
			in_rSerializer.Get( setStateSyncType );
			
			rLocalProxy.SetStateSyncType( setStateSyncType.m_eSyncType );

			break;
		}

	case IParameterableProxy::MethodSetFX:
		{
			ParameterableProxyCommandData::SetFX setFX;
			in_rSerializer.Get( setFX );
			
			rLocalProxy.SetFX( setFX.m_FXID, setFX.m_uFXIndex, setFX.m_pvInitParamsBlock, setFX.m_ulParamBlockSize );

			break;
		}

	case IParameterableProxy::MethodSetFXParam:
		{
			ParameterableProxyCommandData::SetFXParam setFXParam;
			in_rSerializer.Get( setFXParam );
			
			rLocalProxy.SetFXParam( setFXParam.m_FXID, setFXParam.m_uFXIndex, setFXParam.m_paramID, setFXParam.m_pvInitParamsBlock, setFXParam.m_ulParamBlockSize );

			break;
		}

	case IParameterableProxy::MethodBypassAllFX:
		{
			ParameterableProxyCommandData::BypassAllFX bypassAllFX;
			in_rSerializer.Get( bypassAllFX );
			
			rLocalProxy.BypassAllFX( bypassAllFX.m_bBypass );

			break;
		}

	case IParameterableProxy::MethodBypassFX:
		{
			ParameterableProxyCommandData::BypassFX bypassFX;
			in_rSerializer.Get( bypassFX );
			
			rLocalProxy.BypassFX( bypassFX.m_uFXIndex, bypassFX.m_bBypass );

			break;
		}

	case IParameterableProxy::MethodRemoveFX:
		{
			ParameterableProxyCommandData::RemoveFX removeFX;
			in_rSerializer.Get( removeFX );
			
			rLocalProxy.RemoveFX( removeFX.m_uFXIndex );

			break;
		}

	case IParameterableProxy::MethodSetRTPC:
		{
			ParameterableProxyCommandData::SetRTPC setRTPC;
			in_rSerializer.Get( setRTPC );
			
			rLocalProxy.SetRTPC( setRTPC.m_FXID, setRTPC.m_RTPCID, setRTPC.m_paramID, setRTPC.m_RTPCCurveID, setRTPC.m_eScaling, setRTPC.m_pArrayConversion, setRTPC.m_ulConversionArraySize );

			break;
		}

	case IParameterableProxy::MethodUnsetRTPC:
		{
			ParameterableProxyCommandData::UnsetRTPC unsetRTPC;
			in_rSerializer.Get( unsetRTPC );
			
			rLocalProxy.UnsetRTPC( unsetRTPC.m_FXID, unsetRTPC.m_paramID, unsetRTPC.m_RTPCCurveID );

			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
