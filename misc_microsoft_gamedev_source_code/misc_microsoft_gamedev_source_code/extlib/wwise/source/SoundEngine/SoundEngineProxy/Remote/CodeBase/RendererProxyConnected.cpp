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

#include "RendererProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


RendererProxyConnected::RendererProxyConnected()
{
}

RendererProxyConnected::~RendererProxyConnected()
{
}

void RendererProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	RendererProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case AK::Comm::IRendererProxy::MethodPostEvent:
		{
			RendererProxyCommandData::PostEvent postEvent;
			in_rSerializer.Get( postEvent );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.PostEvent( postEvent.m_eventID, postEvent.m_gameObjectPtr, postEvent.m_cookie ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodRegisterGameObject:
		{
			RendererProxyCommandData::RegisterGameObj registerGameObject;
			in_rSerializer.Get( registerGameObject );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.RegisterGameObj( registerGameObject.m_gameObjectPtr, registerGameObject.m_pszObjectName ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodUnregisterGameObject:
		{
			RendererProxyCommandData::UnregisterGameObj unregisterGameObject;
			in_rSerializer.Get( unregisterGameObject );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.UnregisterGameObj( unregisterGameObject.m_gameObjectPtr ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetActiveListeners:
		{
			RendererProxyCommandData::SetActiveListeners setActiveListeners;
			in_rSerializer.Get( setActiveListeners );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetActiveListeners( setActiveListeners.m_gameObjectID, setActiveListeners.m_uListenerMask ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetPosition:
		{
			RendererProxyCommandData::SetPosition setPosition;
			in_rSerializer.Get( setPosition );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetPosition( setPosition.m_gameObjectPtr, setPosition.m_position, setPosition.m_ulListenerIndex ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetListenerPosition:
		{
			RendererProxyCommandData::SetListenerPosition setListenerPosition;
			in_rSerializer.Get( setListenerPosition );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetListenerPosition( setListenerPosition.m_position, setListenerPosition.m_ulIndex ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetListenerSpatialization:
		{
			RendererProxyCommandData::SetListenerSpatialization setListenerSpatialization;
			in_rSerializer.Get( setListenerSpatialization );
			in_rSerializer.Reset();

			AkSpeakerVolumes * pSpeakerVolume = setListenerSpatialization.m_bUseVolumeOffsets? &setListenerSpatialization.m_volumeOffsets : NULL;
#ifdef AK_71AUDIO
			if( pSpeakerVolume )
			{
				pSpeakerVolume->fExtraLeft = 0.0f;
				pSpeakerVolume->fExtraRight = 0.0f;
			}
#endif

			out_rReturnSerializer.Put( m_localProxy.SetListenerSpatialization( setListenerSpatialization.m_uIndex, setListenerSpatialization.m_bSpatialized, pSpeakerVolume ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetRTPC:
		{
			RendererProxyCommandData::SetRTPC setRTPC;
			in_rSerializer.Get( setRTPC );
			in_rSerializer.Reset();

			m_localProxy.SetRTPC( setRTPC.m_RTPCid, setRTPC.m_value, setRTPC.m_gameObj );

			break;
		}

	case AK::Comm::IRendererProxy::MethodResetRTPC:
		{
			RendererProxyCommandData::ResetRTPC resetRTPC;
			in_rSerializer.Get( resetRTPC );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.ResetRTPC( resetRTPC.m_gameObj ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetSwitch:
		{
			RendererProxyCommandData::SetSwitch setSwitch;
			in_rSerializer.Get( setSwitch );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetSwitch( setSwitch.m_switchGroup, setSwitch.m_switchState, setSwitch.m_gameObj ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodPostTrigger:
		{
			RendererProxyCommandData::PostTrigger trigger;
			in_rSerializer.Get( trigger );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.PostTrigger( trigger.m_trigger, trigger.m_gameObj ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodResetSwitches:
		{
			RendererProxyCommandData::ResetSwitches resetSwitches;
			in_rSerializer.Get( resetSwitches );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.ResetSwitches( resetSwitches.m_gameObj ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodResetAllStates:
		{
			RendererProxyCommandData::ResetAllStates resetAllStates;
			in_rSerializer.Get( resetAllStates );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.ResetAllStates() );

			break;
		}

	case AK::Comm::IRendererProxy::MethodResetRndSeqCntrPlaylists:
		{
			RendererProxyCommandData::ResetRndSeqCntrPlaylists resetRndSeqCntrPlaylists;
			in_rSerializer.Get( resetRndSeqCntrPlaylists );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.ResetRndSeqCntrPlaylists() );

			break;
		}

	case AK::Comm::IRendererProxy::MethodAddFXParameterSet:
		{
			RendererProxyCommandData::AddFXParameterSet addFXParameterSet;
			in_rSerializer.Get( addFXParameterSet );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.AddFXParameterSet( addFXParameterSet.m_FXParameterSetID, addFXParameterSet.m_EffectTypeID, addFXParameterSet.m_pvInitParamsBlock, addFXParameterSet.m_ulParamBlockSize ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetFXParameterSetParam:
		{
			RendererProxyCommandData::SetFXParameterSetParam setFXParameterSetParam;
			in_rSerializer.Get( setFXParameterSetParam );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetFXParameterSetParam( setFXParameterSetParam.m_FXParameterSetID, setFXParameterSetParam.m_ulParamID, setFXParameterSetParam.m_pvParamsBlock, setFXParameterSetParam.m_ulParamBlockSize ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodRemoveFXParameterSet:
		{
			RendererProxyCommandData::RemoveFXParameterSet removeFXParameterSet;
			in_rSerializer.Get( removeFXParameterSet );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.RemoveFXParameterSet( removeFXParameterSet.m_FXParameterSetID ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetGameObjectEnvironmentsValues:
		{
			RendererProxyCommandData::SetGameObjectEnvironmentsValues setGameObjectEnvironmentsValues;
			in_rSerializer.Get( setGameObjectEnvironmentsValues );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetGameObjectEnvironmentsValues( setGameObjectEnvironmentsValues.m_gameObjectID, setGameObjectEnvironmentsValues.m_aEnvironmentValues, setGameObjectEnvironmentsValues.m_uNumEnvValues ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetGameObjectDryLevelValue:
		{
			RendererProxyCommandData::SetGameObjectDryLevelValue setGameObjectDryLevelValue;
			in_rSerializer.Get( setGameObjectDryLevelValue );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetGameObjectDryLevelValue( setGameObjectDryLevelValue.m_gameObjectID, setGameObjectDryLevelValue.m_fControlValue ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetEnvironmentVolume:
		{
			RendererProxyCommandData::SetEnvironmentVolume setEnvironmentVolume;
			in_rSerializer.Get( setEnvironmentVolume );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetGameObjectDryLevelValue( setEnvironmentVolume.m_FXParameterSetID, setEnvironmentVolume.m_fVolume ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodBypassEnvironment:
		{
			RendererProxyCommandData::BypassEnvironment bypassEnvironment;
			in_rSerializer.Get( bypassEnvironment );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetGameObjectDryLevelValue( bypassEnvironment.m_FXParameterSetID, bypassEnvironment.m_bIsBypassed ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetObjectObstructionAndOcclusion:
		{
			RendererProxyCommandData::SetObjectObstructionAndOcclusion setObjectObstructionAndOcclusion;
			in_rSerializer.Get( setObjectObstructionAndOcclusion );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetObjectObstructionAndOcclusion( setObjectObstructionAndOcclusion.m_ObjectID, setObjectObstructionAndOcclusion.m_uListener, setObjectObstructionAndOcclusion.m_fObstructionLevel, setObjectObstructionAndOcclusion.m_fOcclusionLevel ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetEnvRTPC:
		{
			RendererProxyCommandData::SetEnvRTPC setEnvRTPC;
			in_rSerializer.Get( setEnvRTPC );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetEnvRTPC( setEnvRTPC.m_FXParameterSetID, setEnvRTPC.m_RTPC_ID, setEnvRTPC.m_ParamID, setEnvRTPC.m_RTPCCurveID, setEnvRTPC.m_eScaling, setEnvRTPC.m_pArrayConversion, setEnvRTPC.m_ulConversionArraySize ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodUnsetEnvRTPC:
		{
			RendererProxyCommandData::UnsetEnvRTPC unsetEnvRTPC;
			in_rSerializer.Get( unsetEnvRTPC );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.UnsetEnvRTPC( unsetEnvRTPC.m_FXParameterSetID, unsetEnvRTPC.m_ParamID, unsetEnvRTPC.m_RTPCCurveID ) );

			break;
		}
	case AK::Comm::IRendererProxy::MethodSetObsOccCurve:
		{
			RendererProxyCommandData::SetObsOccCurve setObsOccCurve;
			in_rSerializer.Get( setObsOccCurve );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetObsOccCurve( setObsOccCurve.m_curveXType, setObsOccCurve.m_curveYType, setObsOccCurve.m_uNumPoints, setObsOccCurve.m_apPoints, setObsOccCurve.m_eScaling ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodSetObsOccCurveEnabled:
		{
			RendererProxyCommandData::SetObsOccCurveEnabled setObsOccCurveEnabled;
			in_rSerializer.Get( setObsOccCurveEnabled );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.SetObsOccCurveEnabled( setObsOccCurveEnabled.m_curveXType, setObsOccCurveEnabled.m_curveYType, setObsOccCurveEnabled.m_bEnabled ) );

			break;
		}

	case AK::Comm::IRendererProxy::MethodAddSwitchRTPC:
		{
			RendererProxyCommandData::AddSwitchRTPC addSwitchRTPC;
			in_rSerializer.Get( addSwitchRTPC );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.AddSwitchRTPC( addSwitchRTPC.m_uSwitchGroup, addSwitchRTPC.m_RTPC_ID, addSwitchRTPC.m_pArrayConversion, addSwitchRTPC.m_uArraySize ) );
			break;
		}
		
	case AK::Comm::IRendererProxy::MethodRemoveSwitchRTPC:
		{
			RendererProxyCommandData::RemoveSwitchRTPC removeSwitchRTPC;
			in_rSerializer.Get( removeSwitchRTPC );
			in_rSerializer.Reset();

			m_localProxy.RemoveSwitchRTPC( removeSwitchRTPC.m_uSwitchGroup );
			break;
		}
		
	case AK::Comm::IRendererProxy::MethodSetVolumeThreshold:
		{
			RendererProxyCommandData::SetVolumeThreshold setVolumeThreshold;
			in_rSerializer.Get( setVolumeThreshold );
			in_rSerializer.Reset();

			m_localProxy.SetVolumeThreshold( setVolumeThreshold.m_fVolumeThreshold );
			break;
		}
	case AK::Comm::IRendererProxy::MethodPostMsgMonitor:
		{
			RendererProxyCommandData::PostMsgMonitor postMsgMonitor;
			in_rSerializer.Get( postMsgMonitor );
			in_rSerializer.Reset();

			out_rReturnSerializer.Put( m_localProxy.PostMsgMonitor( postMsgMonitor.m_pszMessage ) );
			break;
		}
	case AK::Comm::IRendererProxy::MethodStopAll:
		{
			RendererProxyCommandData::StopAll stopAll;
			in_rSerializer.Get( stopAll );
			in_rSerializer.Reset();

			m_localProxy.StopAll( stopAll.m_GameObjPtr );
			break;
		}
	case AK::Comm::IRendererProxy::MethodStopPlayingID:
		{
			RendererProxyCommandData::StopPlayingID stopPlayingID;
			in_rSerializer.Get( stopPlayingID );
			in_rSerializer.Reset();

			m_localProxy.StopAll( stopPlayingID.m_playingID );
			break;
		}
	default:
		AKASSERT( !"Unsupported command." );
	}
}

RendererProxyLocal& RendererProxyConnected::GetLocalProxy()
{
	return m_localProxy;
}
