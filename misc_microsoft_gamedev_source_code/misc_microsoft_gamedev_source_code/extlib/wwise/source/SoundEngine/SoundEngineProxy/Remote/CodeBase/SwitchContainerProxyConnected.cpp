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

#include "SwitchContainerProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


SwitchContainerProxyConnected::SwitchContainerProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

SwitchContainerProxyConnected::~SwitchContainerProxyConnected()
{
}

void SwitchContainerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case ISwitchContainerProxy::MethodSetSwitchGroup:
		{
			SwitchContainerProxyCommandData::SetSwitchGroup setSwitchGroup;
			in_rSerializer.Get( setSwitchGroup );
			
			m_proxyLocal.SetSwitchGroup( setSwitchGroup.m_ulGroup, setSwitchGroup.m_eGroupType );

			break;
		}

	case ISwitchContainerProxy::MethodSetDefaultSwitch:
		{
			SwitchContainerProxyCommandData::SetDefaultSwitch setDefaultSwitch;
			in_rSerializer.Get( setDefaultSwitch );
			
			m_proxyLocal.SetDefaultSwitch( setDefaultSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodClearSwitches:
		{
			SwitchContainerProxyCommandData::ClearSwitches clearSwitches;
			in_rSerializer.Get( clearSwitches );

			m_proxyLocal.ClearSwitches();

			break;
		}

	case ISwitchContainerProxy::MethodAddSwitch:
		{
			SwitchContainerProxyCommandData::AddSwitch addSwitch;
			in_rSerializer.Get( addSwitch );
			
			m_proxyLocal.AddSwitch( addSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodRemoveSwitch:
		{
			SwitchContainerProxyCommandData::RemoveSwitch removeSwitch;
			in_rSerializer.Get( removeSwitch );
			
			m_proxyLocal.RemoveSwitch( removeSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodAddNodeInSwitch:
		{
			SwitchContainerProxyCommandData::AddNodeInSwitch addNodeInSwitch;
			in_rSerializer.Get( addNodeInSwitch );
			
			m_proxyLocal.AddNodeInSwitch( addNodeInSwitch.m_switch, addNodeInSwitch.m_nodeID );

			break;
		}

	case ISwitchContainerProxy::MethodRemoveNodeFromSwitch:
		{
			SwitchContainerProxyCommandData::RemoveNodeFromSwitch removeNodeFromSwitch;
			in_rSerializer.Get( removeNodeFromSwitch );
			
			m_proxyLocal.RemoveNodeFromSwitch( removeNodeFromSwitch.m_switch, removeNodeFromSwitch.m_nodeID );

			break;
		}

	case ISwitchContainerProxy::MethodSetContinuousValidation:
		{
			SwitchContainerProxyCommandData::SetContinuousValidation setContinuousValidation;
			in_rSerializer.Get( setContinuousValidation );
			
			m_proxyLocal.SetContinuousValidation( setContinuousValidation.m_bIsContinuousCheck );

			break;
		}

	case ISwitchContainerProxy::MethodSetContinuePlayback:
		{
			SwitchContainerProxyCommandData::SetContinuePlayback setContinuePlayback;
			in_rSerializer.Get( setContinuePlayback );
			
			m_proxyLocal.SetContinuePlayback( setContinuePlayback.m_nodeID, setContinuePlayback.m_bContinuePlayback );

			break;
		}

	case ISwitchContainerProxy::MethodSetFadeInTime:
		{
			SwitchContainerProxyCommandData::SetFadeInTime setFadeInTime;
			in_rSerializer.Get( setFadeInTime );
			
			m_proxyLocal.SetFadeInTime( setFadeInTime.m_nodeID, setFadeInTime.m_time );

			break;
		}

	case ISwitchContainerProxy::MethodSetFadeOutTime:
		{
			SwitchContainerProxyCommandData::SetFadeOutTime setFadeOutTime;
			in_rSerializer.Get( setFadeOutTime );
			
			m_proxyLocal.SetFadeOutTime( setFadeOutTime.m_nodeID, setFadeOutTime.m_time );

			break;
		}

	case ISwitchContainerProxy::MethodSetOnSwitchMode:
		{
			SwitchContainerProxyCommandData::SetOnSwitchMode setOnSwitchMode;
			in_rSerializer.Get( setOnSwitchMode );
			
			m_proxyLocal.SetOnSwitchMode( setOnSwitchMode.m_nodeID, setOnSwitchMode.m_bSwitchMode );

			break;
		}

	case ISwitchContainerProxy::MethodSetIsFirstOnly:
		{
			SwitchContainerProxyCommandData::SetIsFirstOnly setIsFirstOnly;
			in_rSerializer.Get( setIsFirstOnly );
			
			m_proxyLocal.SetIsFirstOnly( setIsFirstOnly.m_nodeID, setIsFirstOnly.m_bIsFirstOnly );

			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
