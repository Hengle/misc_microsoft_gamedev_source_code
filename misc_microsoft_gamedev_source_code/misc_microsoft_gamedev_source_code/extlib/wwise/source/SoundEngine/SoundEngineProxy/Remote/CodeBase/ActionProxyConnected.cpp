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

#include "ActionProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


ActionProxyConnected::ActionProxyConnected()
{
}

ActionProxyConnected::~ActionProxyConnected()
{
}

void ActionProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionProxyLocal& rLocalProxy = GetLocalProxy();

	switch( cmdData.m_methodID )
	{
	case IActionProxy::MethodSetElementID:
		{
			ActionProxyCommandData::SetElementID setElementID;
			in_rSerializer.Get( setElementID );
			
			rLocalProxy.SetElementID( setElementID.m_elementID );
			
			break;
		}

	case IActionProxy::MethodSetActionType:
		{
			ActionProxyCommandData::SetActionType setActionType;
			in_rSerializer.GetEnum( setActionType );

			rLocalProxy.SetActionType( setActionType.m_actionType );

			break;
		}

	case IActionProxy::MethodDelay:
		{
			ActionProxyCommandData::Delay delay;
			in_rSerializer.Get( delay );
			
			rLocalProxy.Delay( delay.m_delay, delay.m_rangeMin, delay.m_rangeMax );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionPlayProxyConnected::ActionPlayProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionPlayProxyConnected::~ActionPlayProxyConnected()
{
}

void ActionPlayProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionPlayProxyLocal& m_proxyLocal = static_cast<ActionPlayProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{

	case IActionPlayProxy::MethodTransitionTime:
		{
			ActionPlayProxyCommandData::TransitionTime transitionTime;
			in_rSerializer.Get( transitionTime );

			m_proxyLocal.TransitionTime( transitionTime.m_transitionTime, transitionTime.m_rangeMin, transitionTime.m_rangeMax );
			break;
		}

	case IActionPlayProxy::MethodCurveType:
		{
			ActionPlayProxyCommandData::CurveType curveType;
			in_rSerializer.Get( curveType );

			m_proxyLocal.CurveType( curveType.m_eCurveType );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionExceptProxyConnected::ActionExceptProxyConnected()
{
}

ActionExceptProxyConnected::~ActionExceptProxyConnected()
{
}

void ActionExceptProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionExceptProxyLocal& rLocalProxy = static_cast<ActionExceptProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IActionExceptProxy::MethodAddException:
		{
			ActionExceptProxyCommandData::AddException addException;
			in_rSerializer.Get( addException );

			rLocalProxy.AddException( addException.m_elementID );
			break;
		}

	case IActionExceptProxy::MethodRemoveException:
		{
			ActionExceptProxyCommandData::RemoveException removeException;
			in_rSerializer.Get( removeException );

			rLocalProxy.RemoveException( removeException.m_elementID );
			break;
		}

	case IActionExceptProxy::MethodClearExceptions:
		{
			ActionExceptProxyCommandData::ClearExceptions clearExceptions;
			in_rSerializer.Get( clearExceptions );

			rLocalProxy.ClearExceptions();
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionActiveProxyConnected::ActionActiveProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
{
}

ActionActiveProxyConnected::~ActionActiveProxyConnected()
{
}

void ActionActiveProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionActiveProxyLocal& rLocalProxy = static_cast<ActionActiveProxyLocal&>( GetLocalProxy() );
	
	switch( cmdData.m_methodID )
	{

	case IActionActiveProxy::MethodTransitionTime:
		{
			ActionActiveProxyCommandData::TransitionTime transitionTime;
			in_rSerializer.Get( transitionTime );

			rLocalProxy.TransitionTime( transitionTime.m_transitionTime, 
										transitionTime.m_rangeMin, 
										transitionTime.m_rangeMax );
			break;
		}

	case IActionActiveProxy::MethodCurveType:
		{
			ActionActiveProxyCommandData::CurveType curveType;
			in_rSerializer.Get( curveType );

			rLocalProxy.CurveType( curveType.m_eCurveType );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionStopProxyConnected::ActionStopProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: ActionActiveProxyConnected( in_actionType, in_id )
	, m_proxyLocal( in_actionType, in_id )
{
}

ActionStopProxyConnected::~ActionStopProxyConnected()
{
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionPauseProxyConnected::ActionPauseProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: ActionActiveProxyConnected( in_actionType, in_id )
	, m_proxyLocal( in_actionType, in_id )
{
}

ActionPauseProxyConnected::~ActionPauseProxyConnected()
{
}

void ActionPauseProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionPauseProxyLocal& m_proxyLocal = static_cast<ActionPauseProxyLocal&>( GetLocalProxy() );
	
	switch( cmdData.m_methodID )
	{
	case IActionPauseProxy::MethodIncludePendingResume:
		{
			ActionPauseProxyCommandData::IncludePendingResume includePendingResume;
			in_rSerializer.Get( includePendingResume );

			m_proxyLocal.IncludePendingResume( includePendingResume.m_bIncludePendingResume );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionResumeProxyConnected::ActionResumeProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: ActionActiveProxyConnected( in_actionType, in_id )
	, m_proxyLocal( in_actionType, in_id )
{
}

ActionResumeProxyConnected::~ActionResumeProxyConnected()
{
}

void ActionResumeProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionResumeProxyLocal& m_proxyLocal = static_cast<ActionResumeProxyLocal&>( GetLocalProxy() );
	
	switch( cmdData.m_methodID )
	{
	case IActionResumeProxy::MethodIsMasterResume:
		{
			ActionResumeProxyCommandData::IsMasterResume isMasterResume;
			in_rSerializer.Get( isMasterResume );

			m_proxyLocal.IsMasterResume( isMasterResume.m_bIsMasterResume );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}


// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionBreakProxyConnected::ActionBreakProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionBreakProxyConnected::~ActionBreakProxyConnected()
{
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetValueProxyConnected::ActionSetValueProxyConnected()
{
}

ActionSetValueProxyConnected::~ActionSetValueProxyConnected()
{
}

void ActionSetValueProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	ActionSetValueProxyLocal& rLocalProxy = static_cast<ActionSetValueProxyLocal&>( GetLocalProxy() );

	switch( cmdData.m_methodID )
	{
	case IActionSetValueProxy::MethodTransitionTime:
		{
			ActionSetValueProxyCommandData::TransitionTime transitionTime;
			in_rSerializer.Get( transitionTime );
			
			rLocalProxy.TransitionTime( transitionTime.m_transitionTime, transitionTime.m_rangeMin, transitionTime.m_rangeMax );
			break;
		}

	case IActionSetValueProxy::MethodCurveType:
		{
			ActionSetValueProxyCommandData::CurveType curveType;
			in_rSerializer.Get( curveType );
			
			rLocalProxy.CurveType( curveType.m_eCurveType );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionMuteProxyConnected::ActionMuteProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionMuteProxyConnected::~ActionMuteProxyConnected()
{
}

void ActionMuteProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	//ObjectProxyCommandData::CommandData cmdData;

	//{
	//	CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
	//	in_rSerializer.Get( cmdData );
	//}

	//commented switch so no more warning that only default is available.
	//switch( cmdData.m_methodID )
	//{

	//default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	//}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetPitchProxyConnected::ActionSetPitchProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetPitchProxyConnected::~ActionSetPitchProxyConnected()
{
}

void ActionSetPitchProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetPitchProxy::MethodSetValue:
		{
			ActionSetPitchProxyCommandData::SetValue setValue;
			in_rSerializer.Get( setValue );
			
			m_proxyLocal.SetValue( setValue.m_pitchType, setValue.m_eValueMeaning, setValue.m_rangeMin, setValue.m_rangeMax );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetVolumeProxyConnected::ActionSetVolumeProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetVolumeProxyConnected::~ActionSetVolumeProxyConnected()
{
}

void ActionSetVolumeProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetVolumeProxy::MethodSetValue:
		{
			ActionSetVolumeProxyCommandData::SetValue setValue;
			in_rSerializer.Get( setValue );

			m_proxyLocal.SetValue( setValue.m_value, setValue.m_eValueMeaning, setValue.m_rangeMin, setValue.m_rangeMax );
			break;
		}
	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetLFEProxyConnected::ActionSetLFEProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetLFEProxyConnected::~ActionSetLFEProxyConnected()
{
}

void ActionSetLFEProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetLFEProxy::MethodSetValue:
		{
			ActionSetLFEProxyCommandData::SetValue setValue;
			in_rSerializer.Get( setValue );

			m_proxyLocal.SetValue( setValue.m_value, setValue.m_eValueMeaning, setValue.m_rangeMin, setValue.m_rangeMax );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetLPFProxyConnected::ActionSetLPFProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetLPFProxyConnected::~ActionSetLPFProxyConnected()
{
}

void ActionSetLPFProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetLPFProxy::MethodSetValue:
		{
			ActionSetLPFProxyCommandData::SetValue setValue;
			in_rSerializer.Get( setValue );

			m_proxyLocal.SetValue( setValue.m_value, setValue.m_eValueMeaning, setValue.m_rangeMin, setValue.m_rangeMax );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetStateProxyConnected::ActionSetStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetStateProxyConnected::~ActionSetStateProxyConnected()
{
}

void ActionSetStateProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetStateProxy::MethodSetGroup:
		{
			ActionSetStateProxyCommandData::SetGroup setGroup;
			in_rSerializer.Get( setGroup );

			m_proxyLocal.SetGroup( setGroup.m_groupID );
			break;
		}

	case IActionSetStateProxy::MethodSetTargetState:
		{
			ActionSetStateProxyCommandData::SetTargetState setTargetState;
			in_rSerializer.Get( setTargetState );

			m_proxyLocal.SetTargetState( setTargetState.m_stateID );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetSwitchProxyConnected::ActionSetSwitchProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetSwitchProxyConnected::~ActionSetSwitchProxyConnected()
{
}

void ActionSetSwitchProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetSwitchProxy::MethodSetSwitchGroup:
		{
			ActionSetSwitchProxyCommandData::SetSwitchGroup setSwitchGroup;
			in_rSerializer.Get( setSwitchGroup );

			m_proxyLocal.SetSwitchGroup( setSwitchGroup.m_ulSwitchGroupID );
			break;
		}

	case IActionSetSwitchProxy::MethodSetTargetSwitch:
		{
			ActionSetSwitchProxyCommandData::SetTargetSwitch setTargetSwitch;
			in_rSerializer.Get( setTargetSwitch );

			m_proxyLocal.SetTargetSwitch( setTargetSwitch.m_ulSwitchID );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionTriggerProxyConnected::ActionTriggerProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionTriggerProxyConnected::~ActionTriggerProxyConnected()
{
}

void ActionTriggerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionSetRTPCProxyConnected::ActionSetRTPCProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionSetRTPCProxyConnected::~ActionSetRTPCProxyConnected()
{
}

void ActionSetRTPCProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionSetRTPCProxy::MethodSetRTPCGroup:
		{
			ActionSetRTPCProxyCommandData::SetRTPCGroup setRTPCGroup;
			in_rSerializer.Get( setRTPCGroup );

			m_proxyLocal.SetRTPCGroup( setRTPCGroup.m_RTPCGroupID );
			break;
		}

	case IActionSetRTPCProxy::MethodSetRTPCValue:
		{
			ActionSetRTPCProxyCommandData::SetRTPCValue setRTPCValue;
			in_rSerializer.Get( setRTPCValue );

			m_proxyLocal.SetRTPCValue( setRTPCValue.m_fRTPCValue );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionUseStateProxyConnected::ActionUseStateProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionUseStateProxyConnected::~ActionUseStateProxyConnected()
{
}

void ActionUseStateProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionUseStateProxy::MethodUseState:
		{
			ActionUseStateProxyCommandData::UseState useState;
			in_rSerializer.Get( useState );

			m_proxyLocal.UseState( useState.m_bUseState );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionBypassFXProxyConnected::ActionBypassFXProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionBypassFXProxyConnected::~ActionBypassFXProxyConnected()
{
}

void ActionBypassFXProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IActionBypassFXProxy::MethodBypassFX:
		{
			ActionBypassFXProxyCommandData::BypassFX bypassFX;
			in_rSerializer.Get( bypassFX );

			m_proxyLocal.BypassFX( bypassFX.m_bBypassFX );
			break;
		}
	case IActionBypassFXProxy::MethodSetBypassTarget:
		{
			ActionBypassFXProxyCommandData::SetBypassTarget setBypassTarget;
			in_rSerializer.Get( setBypassTarget );

			m_proxyLocal.SetBypassTarget( setBypassTarget.m_bBypassAllFX, setBypassTarget.m_ucEffectsMask );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

ActionEventProxyConnected::ActionEventProxyConnected( AkActionType in_actionType, AkUniqueID in_id )
	: m_proxyLocal( in_actionType, in_id )
{
}

ActionEventProxyConnected::~ActionEventProxyConnected()
{
}

void ActionEventProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
}
