/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/


#include "StdAfx.h"

#include "CommandData.h"

#include "IRendererProxy.h"
#include "IALMonitorSubProxy.h"
#include "IStateMgrProxy.h"
#include "IAudioNodeProxy.h"
#include "IParameterableProxy.h"
#include "IParameterNodeProxy.h"
#include "ISoundProxy.h"
#include "IBusProxy.h"
#include "IFeedbackBusProxy.h"
#include "IEventProxy.h"
#include "IActionProxy.h"
#include "IStateProxy.h"
#include "IAttenuationProxy.h"
#include "IHierarchicalProxy.h"
#include "IMusicNodeProxy.h"
#include "IMusicTransAwareProxy.h"
#include "IMusicRanSeqProxy.h"
#include "IMusicSwitchProxy.h"
#include "ISegmentProxy.h"
#include "IRanSeqContainerProxy.h"
#include "ISwitchContainerProxy.h"
#include "ILayerContainerProxy.h"
#include "ILayerProxy.h"
#include "ITrackProxy.h"
#include "IFeedbackNodeProxy.h"
#include "AkRanSeqBaseInfo.h"
#include "AkSound.h"

#include "CommandDataSerializer.h"


namespace ProxyCommandData
{
	AkMemPoolId CommandData::s_poolID = AK_INVALID_POOL_ID;

	CommandData::CommandData()
	{}

	CommandData::CommandData( CommandType in_eCommandType, AkUInt16 in_methodID )
		: m_commandType( (AkUInt16)in_eCommandType )
		, m_methodID( in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return in_rSerializer.Put( m_commandType )
			&& in_rSerializer.Put( m_methodID );
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return in_rSerializer.Get( m_commandType )
			&& in_rSerializer.Get( m_methodID );
	}
};

namespace RendererProxyCommandData
{
	CommandData::CommandData()
		: ProxyCommandData::CommandData()
	{}

	CommandData::CommandData( AkUInt16 in_methodID )
		: ProxyCommandData::CommandData( ProxyCommandData::TypeRendererProxy, in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	PostEvent::PostEvent()
		: CommandData( AK::Comm::IRendererProxy::MethodPostEvent )
	{}

	bool PostEvent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_eventID )
			&& in_rSerializer.Put( m_gameObjectPtr )
			&& in_rSerializer.Put( m_cookie );
	}

	bool PostEvent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_eventID )
			&& in_rSerializer.Get( m_gameObjectPtr )
			&& in_rSerializer.Get( m_cookie );
	}

	RegisterGameObj::RegisterGameObj()
		:	CommandData( AK::Comm::IRendererProxy::MethodRegisterGameObject )
	{}

	bool RegisterGameObj::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjectPtr )
			&& in_rSerializer.Put( (const char*)m_pszObjectName );
	}
	
	bool RegisterGameObj::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		AkInt32 int32Count = 0;

		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjectPtr )
			&& in_rSerializer.Get( (char*&)m_pszObjectName, int32Count );
	}

	UnregisterGameObj::UnregisterGameObj()
		:	CommandData( AK::Comm::IRendererProxy::MethodUnregisterGameObject )
	{}

	bool UnregisterGameObj::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjectPtr );
	}
	
	bool UnregisterGameObj::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjectPtr );
	}

	SetActiveListeners::SetActiveListeners()
		:	CommandData( AK::Comm::IRendererProxy::MethodSetActiveListeners )
	{}

	bool SetActiveListeners::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjectID )
			&& in_rSerializer.Put( m_uListenerMask );
	}
		
	bool SetActiveListeners::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjectID )
			&& in_rSerializer.Get( m_uListenerMask );
	}

	SetPosition::SetPosition()
		: CommandData( AK::Comm::IRendererProxy::MethodSetPosition )
	{}

	bool SetPosition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjectPtr )
			&& in_rSerializer.Put( m_position )
			&& in_rSerializer.Put( m_ulListenerIndex );
	}

	bool SetPosition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjectPtr )
			&& in_rSerializer.Get( m_position )
			&& in_rSerializer.Get( m_ulListenerIndex );
	}

	SetListenerPosition::SetListenerPosition()
		: CommandData( AK::Comm::IRendererProxy::MethodSetListenerPosition )
	{}

	bool SetListenerPosition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_position )
			&& in_rSerializer.Put( m_ulIndex );
	}

	bool SetListenerPosition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_position )
			&& in_rSerializer.Get( m_ulIndex );
	}

	SetListenerSpatialization::SetListenerSpatialization()
		:	CommandData( AK::Comm::IRendererProxy::MethodSetListenerSpatialization )
	{}

	bool SetListenerSpatialization::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uIndex )
			&& in_rSerializer.Put( m_bSpatialized )
			&& in_rSerializer.Put( m_bUseVolumeOffsets )
			&& in_rSerializer.Put( m_volumeOffsets );
	}
	
	bool SetListenerSpatialization::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uIndex )
			&& in_rSerializer.Get( m_bSpatialized )
			&& in_rSerializer.Get( m_bUseVolumeOffsets )
			&& in_rSerializer.Get( m_volumeOffsets );
	}

	SetRTPC::SetRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodSetRTPC )
	{}

	bool SetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_RTPCid )
			&& in_rSerializer.Put( m_value )
			&& in_rSerializer.Put( m_gameObj );
	}

	bool SetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_RTPCid )
			&& in_rSerializer.Get( m_value )
			&& in_rSerializer.Get( m_gameObj );
	}
	
	ResetRTPC::ResetRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodSetSwitch )
	{}

	bool ResetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObj );
	}

	bool ResetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObj );
	}

	SetSwitch::SetSwitch()
		: CommandData( AK::Comm::IRendererProxy::MethodSetSwitch )
	{}

	bool SetSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switchGroup )
			&& in_rSerializer.Put( m_switchState )
			&& in_rSerializer.Put( m_gameObj );
	}

	bool SetSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switchGroup )
			&& in_rSerializer.Get( m_switchState )
			&& in_rSerializer.Get( m_gameObj );
	}

	PostTrigger::PostTrigger()
		: CommandData( AK::Comm::IRendererProxy::MethodPostTrigger )
	{}

	bool PostTrigger::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_trigger )
			&& in_rSerializer.Put( m_gameObj );
	}

	bool PostTrigger::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_trigger )
			&& in_rSerializer.Get( m_gameObj );
	}

	ResetSwitches::ResetSwitches()
		: CommandData( AK::Comm::IRendererProxy::MethodResetSwitches )
	{}

	bool ResetSwitches::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObj );
	}

	bool ResetSwitches::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObj );
	}

	ResetAllStates::ResetAllStates()
		: CommandData( AK::Comm::IRendererProxy::MethodResetAllStates )
	{}

	bool ResetAllStates::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool ResetAllStates::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	ResetRndSeqCntrPlaylists::ResetRndSeqCntrPlaylists()
		: CommandData( AK::Comm::IRendererProxy::MethodResetRndSeqCntrPlaylists )
	{}

	bool ResetRndSeqCntrPlaylists::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool ResetRndSeqCntrPlaylists::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	AddFXParameterSet::AddFXParameterSet()
		: CommandData( AK::Comm::IRendererProxy::MethodAddFXParameterSet )
	{}

	bool AddFXParameterSet::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvInitParamsBlock, m_ulParamBlockSize )
			&& in_rSerializer.Put( m_FXParameterSetID )
			&& in_rSerializer.Put( m_EffectTypeID );
			
	}

	bool AddFXParameterSet::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvInitParamsBlock, (AkInt32&)m_ulParamBlockSize )
			&& in_rSerializer.Get( m_FXParameterSetID )
			&& in_rSerializer.Get( m_EffectTypeID );
	}

	SetFXParameterSetParam::SetFXParameterSetParam()
		: CommandData( AK::Comm::IRendererProxy::MethodSetFXParameterSetParam )
	{}

	bool SetFXParameterSetParam::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_pvParamsBlock, m_ulParamBlockSize )
		&& in_rSerializer.Put( m_FXParameterSetID )
		&& in_rSerializer.Put( m_ulParamID );	
	}

	bool SetFXParameterSetParam::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_pvParamsBlock, (AkInt32&)m_ulParamBlockSize )
		&& in_rSerializer.Get( m_FXParameterSetID )
		&& in_rSerializer.Get( m_ulParamID );
	}

	RemoveFXParameterSet::RemoveFXParameterSet()
		: CommandData( AK::Comm::IRendererProxy::MethodRemoveFXParameterSet )
	{}

	bool RemoveFXParameterSet::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FXParameterSetID );
	}

	bool RemoveFXParameterSet::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FXParameterSetID );
	}

	SetGameObjectEnvironmentsValues::SetGameObjectEnvironmentsValues()
		: CommandData( AK::Comm::IRendererProxy::MethodSetGameObjectEnvironmentsValues )
		, m_bWasDeserialized( false )
	{}

	SetGameObjectEnvironmentsValues::~SetGameObjectEnvironmentsValues()
	{
		if( m_bWasDeserialized && m_aEnvironmentValues )
		{
			AkFree( s_poolID, m_aEnvironmentValues );
		}
	}

	bool SetGameObjectEnvironmentsValues::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjectID );

		bRet = bRet && in_rSerializer.Put( m_uNumEnvValues );

		for( AkUInt32 i = 0; i < m_uNumEnvValues && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_aEnvironmentValues[i] );

		return bRet;
	}

	bool SetGameObjectEnvironmentsValues::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjectID );

		bRet = bRet && in_rSerializer.Get( m_uNumEnvValues );

		if( m_uNumEnvValues )
		{
			m_aEnvironmentValues = (AkEnvironmentValue*)AkAlloc( s_poolID, m_uNumEnvValues * sizeof( AkEnvironmentValue ) );

			if( m_aEnvironmentValues )
			{
				for( AkUInt32 i = 0; i < m_uNumEnvValues && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_aEnvironmentValues[i] );
			}
			else
			{
				bRet = false;
			}
		}

		m_bWasDeserialized = true;

		return bRet;
	}

	SetGameObjectDryLevelValue::SetGameObjectDryLevelValue()
		: CommandData( AK::Comm::IRendererProxy::MethodSetGameObjectDryLevelValue )
	{}

	bool SetGameObjectDryLevelValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_gameObjectID )
		&& in_rSerializer.Put( m_fControlValue );
	}
		
	bool SetGameObjectDryLevelValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_gameObjectID )
		&& in_rSerializer.Get( m_fControlValue );
	}

	SetEnvironmentVolume::SetEnvironmentVolume()
		: CommandData( AK::Comm::IRendererProxy::MethodSetEnvironmentVolume )	
	{}

	bool SetEnvironmentVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_FXParameterSetID )
		&& in_rSerializer.Put( m_fVolume );
	}
		
	bool SetEnvironmentVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_FXParameterSetID )
		&& in_rSerializer.Get( m_fVolume );
	}

	BypassEnvironment::BypassEnvironment()
		: CommandData( AK::Comm::IRendererProxy::MethodBypassEnvironment )	
	{}

	bool BypassEnvironment::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_FXParameterSetID )
		&& in_rSerializer.Put( m_bIsBypassed );
	}
		
	bool BypassEnvironment::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_FXParameterSetID )
		&& in_rSerializer.Get( m_bIsBypassed );
	}

	SetObjectObstructionAndOcclusion::SetObjectObstructionAndOcclusion()
		: CommandData( AK::Comm::IRendererProxy::MethodSetObjectObstructionAndOcclusion )	
	{}

	bool SetObjectObstructionAndOcclusion::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
		&& in_rSerializer.Put( m_ObjectID )
		&& in_rSerializer.Put( m_uListener )
		&& in_rSerializer.Put( m_fObstructionLevel )
		&& in_rSerializer.Put( m_fOcclusionLevel );
	}
		
	bool SetObjectObstructionAndOcclusion::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
		&& in_rSerializer.Get( m_ObjectID )
		&& in_rSerializer.Get( m_uListener )
		&& in_rSerializer.Get( m_fObstructionLevel )
		&& in_rSerializer.Get( m_fOcclusionLevel );
	}

	SetEnvRTPC::SetEnvRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodSetEnvRTPC )
		, m_bWasDeserialized( false )
	{}

	SetEnvRTPC::~SetEnvRTPC()
	{
		if( m_bWasDeserialized && m_pArrayConversion )
		{
			AkFree( s_poolID, m_pArrayConversion );
		}
	}

	bool SetEnvRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FXParameterSetID )
			&& in_rSerializer.Put( m_RTPC_ID )
			&& in_rSerializer.PutEnum( m_ParamID )
			&& in_rSerializer.Put( m_RTPCCurveID )
			&& in_rSerializer.PutEnum( m_eScaling );

		bRet = bRet && in_rSerializer.Put( m_ulConversionArraySize );

		for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayConversion[i] );

		return bRet;
	}

	bool SetEnvRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FXParameterSetID )
			&& in_rSerializer.Get( m_RTPC_ID )
			&& in_rSerializer.GetEnum( m_ParamID )
			&& in_rSerializer.Get( m_RTPCCurveID )
			&& in_rSerializer.GetEnum( m_eScaling );

		bRet = bRet && in_rSerializer.Get( m_ulConversionArraySize );

		m_pArrayConversion = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_ulConversionArraySize * sizeof( AkRTPCGraphPoint ) );

		if( m_pArrayConversion )
		{
			for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayConversion[i] );
		}
		else
		{
			bRet = false;
		}

		m_bWasDeserialized = true;

		return bRet;
	}

	UnsetEnvRTPC::UnsetEnvRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodUnsetEnvRTPC )
	{}

	bool UnsetEnvRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FXParameterSetID )
			&& in_rSerializer.PutEnum( m_ParamID )
			&& in_rSerializer.Put( m_RTPCCurveID );
	}

	bool UnsetEnvRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FXParameterSetID )
			&& in_rSerializer.GetEnum( m_ParamID )
			&& in_rSerializer.Get( m_RTPCCurveID );
	}

	SetObsOccCurve::SetObsOccCurve()
		: CommandData( AK::Comm::IRendererProxy::MethodSetObsOccCurve )
		, m_bWasDeserialized( false )
		, m_apPoints( NULL )
	{}

	SetObsOccCurve::~SetObsOccCurve()
	{
		if( m_bWasDeserialized && m_apPoints )
		{
			AkFree( s_poolID, m_apPoints );
		}
	}

	bool SetObsOccCurve::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_curveXType )
			&& in_rSerializer.Put( m_curveYType )
			&& in_rSerializer.Put( m_uNumPoints )
			&& in_rSerializer.PutEnum( m_eScaling );

		for( AkUInt32 i = 0; i < m_uNumPoints && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_apPoints[i] );

		return bRet;
	}

	bool SetObsOccCurve::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_curveXType )
			&& in_rSerializer.Get( m_curveYType )
			&& in_rSerializer.Get( m_uNumPoints )
			&& in_rSerializer.GetEnum( m_eScaling );

		m_apPoints = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_uNumPoints * sizeof( AkRTPCGraphPoint ) );

		if( m_apPoints )
		{
			for( AkUInt32 i = 0; i < m_uNumPoints && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_apPoints[i] );
		}
		else
		{
			bRet =  false;
		}

		m_bWasDeserialized = true;

		return bRet;
	}


	SetObsOccCurveEnabled::SetObsOccCurveEnabled()
		: CommandData( AK::Comm::IRendererProxy::MethodSetObsOccCurveEnabled )
	{}

	SetObsOccCurveEnabled::~SetObsOccCurveEnabled()
	{
	}

	bool SetObsOccCurveEnabled::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_curveXType )
			&& in_rSerializer.Put( m_curveYType )
			&& in_rSerializer.Put( m_bEnabled );

		return bRet;
	}

	bool SetObsOccCurveEnabled::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_curveXType )
			&& in_rSerializer.Get( m_curveYType )
			&& in_rSerializer.Get( m_bEnabled );

		return bRet;
	}

	AddSwitchRTPC::AddSwitchRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodAddSwitchRTPC )
		, m_bWasDeserialized( false )
	{}

	AddSwitchRTPC::~AddSwitchRTPC()
	{
		if( m_bWasDeserialized && m_pArrayConversion )
		{
			AkFree( s_poolID, m_pArrayConversion );
		}
	}

	bool AddSwitchRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uSwitchGroup )
			&& in_rSerializer.Put( m_RTPC_ID );

		bRet = bRet && in_rSerializer.Put( m_uArraySize );

		for( AkUInt32 i = 0; i < m_uArraySize && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayConversion[i] );;

		return bRet;
	}

	bool AddSwitchRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uSwitchGroup )
			&& in_rSerializer.Get( m_RTPC_ID );

		bRet = bRet && in_rSerializer.Get( m_uArraySize );
		if( m_uArraySize )
		{
			m_pArrayConversion = (AkRTPCGraphPointInteger*)AkAlloc( s_poolID, m_uArraySize * sizeof( AkRTPCGraphPointInteger ) );
			if( !m_pArrayConversion )
				bRet = false;
		}
		m_bWasDeserialized = true;

		for( AkUInt32 i = 0; i < m_uArraySize && bRet; ++i )
			bRet = bRet && in_rSerializer.Get( m_pArrayConversion[i] );

		return bRet;
	}

	RemoveSwitchRTPC::RemoveSwitchRTPC()
		: CommandData( AK::Comm::IRendererProxy::MethodRemoveSwitchRTPC )
	{}

	bool RemoveSwitchRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uSwitchGroup );
	}

	bool RemoveSwitchRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uSwitchGroup );
	}

	SetVolumeThreshold::SetVolumeThreshold()
		: CommandData( AK::Comm::IRendererProxy::MethodSetVolumeThreshold )
	{}

	bool SetVolumeThreshold::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fVolumeThreshold );
	}

	bool SetVolumeThreshold::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fVolumeThreshold );
	}

	PostMsgMonitor::PostMsgMonitor()
		: CommandData( AK::Comm::IRendererProxy::MethodPostMsgMonitor )
	{}

	bool PostMsgMonitor::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( (const AkTChar*)m_pszMessage );
	}
	
	bool PostMsgMonitor::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		AkInt32 int32Count = 0;

		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( (AkTChar*&)m_pszMessage, int32Count );
	}

	StopAll::StopAll()
		: CommandData( AK::Comm::IRendererProxy::MethodStopAll )
	{}

	bool StopAll::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_GameObjPtr );
	}

	bool StopAll::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_GameObjPtr );
	}

	StopPlayingID::StopPlayingID()
		: CommandData( AK::Comm::IRendererProxy::MethodStopPlayingID )
	{}

	bool StopPlayingID::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_playingID );
	}

	bool StopPlayingID::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_playingID );
	}
};

namespace ALMonitorProxyCommandData
{
	CommandData::CommandData()
		: ProxyCommandData::CommandData()
	{}

	CommandData::CommandData( AkUInt16 in_methodID )
		: ProxyCommandData::CommandData( ProxyCommandData::TypeALMonitorProxy, in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	Monitor::Monitor()
		: CommandData( IALMonitorSubProxy::MethodMonitor )
	{}

	bool Monitor::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uWhatToMonitor );
	}

	bool Monitor::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uWhatToMonitor );
	}

	StopMonitor::StopMonitor()
		: CommandData( IALMonitorSubProxy::MethodStopMonitor )
	{}

	bool StopMonitor::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool StopMonitor::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	SetWatches::SetWatches()
		: CommandData( IALMonitorSubProxy::MethodSetWatches )
		, m_bWasDeserialized( false )
		, m_pWatches( NULL )
		, m_uiWatchCount( 0 )
	{}

	SetWatches::~SetWatches()
	{
		if( m_bWasDeserialized && m_pWatches )
		{
			AkFree( s_poolID, m_pWatches );
		}
	}

	bool SetWatches::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uiWatchCount );

		for( AkUInt32 i = 0; i < m_uiWatchCount && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pWatches[i] );

		return bRet;
	}

	bool SetWatches::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uiWatchCount );

		if( m_uiWatchCount )
		{
			m_pWatches = (AkMonitorData::Watch*)AkAlloc( s_poolID, m_uiWatchCount * sizeof( AkMonitorData::Watch ) );
			m_bWasDeserialized = true;
			if( !m_pWatches )
				bRet = false;
		}
		
		for( AkUInt32 i = 0; i < m_uiWatchCount && bRet; ++i )
			bRet = bRet && in_rSerializer.Get( m_pWatches[i] );

		return bRet;
	}

	SetGameSyncWatches::SetGameSyncWatches()
		: CommandData( IALMonitorSubProxy::MethodSetGameSyncWatches )
		, m_bWasDeserialized( false )
		, m_pWatches( NULL )
		, m_uiWatchCount( 0 )
	{}

	SetGameSyncWatches::~SetGameSyncWatches()
	{
		if( m_bWasDeserialized && m_pWatches )
		{
			AkFree( s_poolID, m_pWatches );
		}
	}

	bool SetGameSyncWatches::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uiWatchCount );

		for( AkUInt32 i = 0; i < m_uiWatchCount && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pWatches[i] );

		return bRet;
	}

	bool SetGameSyncWatches::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uiWatchCount );

		if( m_uiWatchCount )
		{
			m_pWatches = (AkUniqueID*)AkAlloc( s_poolID, m_uiWatchCount * sizeof( AkUniqueID ) );
			m_bWasDeserialized = true;
			if( !m_pWatches )
				bRet = false;
		}
		
		for( AkUInt32 i = 0; i < m_uiWatchCount && bRet; ++i )
			bRet = bRet && in_rSerializer.Get( m_pWatches[i] );

		return bRet;
	}
};

namespace StateMgrProxyCommandData
{
	CommandData::CommandData()
		: ProxyCommandData::CommandData()
	{}

	CommandData::CommandData( AkUInt16 in_methodID )
		: ProxyCommandData::CommandData( ProxyCommandData::TypeStateMgrProxy, in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	AddStateGroup::AddStateGroup()
		: CommandData( IStateMgrProxy::MethodAddStateGroup )
	{}

	bool AddStateGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID );
	}

	bool AddStateGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID );
	}

	RemoveStateGroup::RemoveStateGroup()
		: CommandData( IStateMgrProxy::MethodRemoveStateGroup )
	{}

	bool RemoveStateGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID );
	}

	bool RemoveStateGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID );
	}

	AddStateTransition::AddStateTransition()
		: CommandData( IStateMgrProxy::MethodAddStateTransition )
	{}

	bool AddStateTransition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_stateID1 )
			&& in_rSerializer.Put( m_stateID2 )
			&& in_rSerializer.Put( m_transitionTime )
			&& in_rSerializer.Put( m_bIsShared );
	}

	bool AddStateTransition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_stateID1 )
			&& in_rSerializer.Get( m_stateID2 )
			&& in_rSerializer.Get( m_transitionTime )
			&& in_rSerializer.Get( m_bIsShared );
	}

	RemoveStateTransition::RemoveStateTransition()
		: CommandData( IStateMgrProxy::MethodRemoveStateTransition )
	{}

	bool RemoveStateTransition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_stateID1 )
			&& in_rSerializer.Put( m_stateID2 )
			&& in_rSerializer.Put( m_bIsShared );
	}

	bool RemoveStateTransition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_stateID1 )
			&& in_rSerializer.Get( m_stateID2 )
			&& in_rSerializer.Get( m_bIsShared );
	}

	ClearStateTransitions::ClearStateTransitions()
		: CommandData( IStateMgrProxy::MethodClearStateTransitions )
	{}

	bool ClearStateTransitions::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID );
	}

	bool ClearStateTransitions::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID );
	}

	SetDefaultTransitionTime::SetDefaultTransitionTime()
		: CommandData( IStateMgrProxy::MethodSetDefaultTransitionTime )
	{}

	bool SetDefaultTransitionTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_transitionTime );
	}

	bool SetDefaultTransitionTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_transitionTime );
	}

	AddState::AddState()
		: CommandData( IStateMgrProxy::MethodAddState )
	{}

	bool AddState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_stateID )
			&& in_rSerializer.Put( m_stateUniqueID );
	}

	bool AddState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_stateID )
			&& in_rSerializer.Get( m_stateUniqueID );
	}

	RemoveState::RemoveState()
		: CommandData( IStateMgrProxy::MethodRemoveState )
	{}

	bool RemoveState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_stateID );
	}

	bool RemoveState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_stateID );
	}

	SetState::SetState()
		: CommandData( IStateMgrProxy::MethodSetState )
	{}

	bool SetState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID )
			&& in_rSerializer.Put( m_stateID );
	}

	bool SetState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID )
			&& in_rSerializer.Get( m_stateID );
	}

	GetState::GetState()
		: CommandData( IStateMgrProxy::MethodGetState )
	{}

	bool GetState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID );
	}

	bool GetState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID );
	}
};

namespace ObjectProxyStoreCommandData
{
	CommandData::CommandData()
		: ProxyCommandData::CommandData()
	{}

	CommandData::CommandData( AkUInt16 in_methodID ) 
		: ProxyCommandData::CommandData( ProxyCommandData::TypeObjectProxyStore, in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_proxyInstancePtr )
			&& in_rSerializer.Put( m_objectID );
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_proxyInstancePtr )
			&& in_rSerializer.Get( m_objectID );
	}

	Create::Create()
		: CommandData( MethodCreate )
	{}

	bool Create::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eObjectType )
			&& in_rSerializer.PutEnum( m_actionType )
			&& in_rSerializer.Put(m_StateGroupID);
	}

	bool Create::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eObjectType )
			&& in_rSerializer.GetEnum( m_actionType )
			&& in_rSerializer.Get( m_StateGroupID );
	}

	Release::Release()
		: CommandData( MethodRelease )
	{}

	bool Release::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool Release::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}
};

namespace ObjectProxyCommandData
{
	CommandData::CommandData()
		: ProxyCommandData::CommandData()
	{}

	CommandData::CommandData( AkUInt16 in_methodID )
		: ProxyCommandData::CommandData( ProxyCommandData::TypeObjectProxy, in_methodID )
	{}

	bool CommandData::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_proxyInstancePtr )
			&& in_rSerializer.Put( m_objectID )	;
	}

	bool CommandData::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_proxyInstancePtr )
			&& in_rSerializer.Get( m_objectID );
	}
};

namespace AudioNodeProxyCommandData
{
};

namespace ParameterableProxyCommandData
{
	Priority::Priority()
		: CommandData( IParameterableProxy::MethodPriority )
	{}

	bool Priority::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_priority );
	}

	bool Priority::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_priority );
	}

	PriorityApplyDistFactor::PriorityApplyDistFactor()
		: CommandData( IParameterableProxy::MethodPriorityApplyDistFactor )
	{}

	bool PriorityApplyDistFactor::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bApplyDistFactor );
	}

	bool PriorityApplyDistFactor::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bApplyDistFactor );
	}

	PriorityDistanceOffset::PriorityDistanceOffset()
		: CommandData( IParameterableProxy::MethodPriorityDistanceOffset )
	{}

	bool PriorityDistanceOffset::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_iDistOffset );
	}

	bool PriorityDistanceOffset::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_iDistOffset );
	}

	PriorityOverrideParent::PriorityOverrideParent()
		: CommandData( IParameterableProxy::MethodPriorityOverrideParent )
	{}

	bool PriorityOverrideParent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bOverrideParent );
	}

	bool PriorityOverrideParent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bOverrideParent );
	}

	SetStateGroup::SetStateGroup()
		: CommandData( IParameterableProxy::MethodSetStateGroup )
	{}

	bool SetStateGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_stateGroupID );
	}

	bool SetStateGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_stateGroupID );
	}

	UnsetStateGroup::UnsetStateGroup()
		: CommandData( IParameterableProxy::MethodUnsetStateGroup )
	{}

	bool UnsetStateGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool UnsetStateGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	AddState::AddState()
		: CommandData( IParameterableProxy::MethodAddState )
	{}

	bool AddState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_stateInstanceID )
			&& in_rSerializer.Put( m_stateID );
	}

	bool AddState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_stateInstanceID )
			&& in_rSerializer.Get( m_stateID );
	}

	RemoveState::RemoveState()
		: CommandData( IParameterableProxy::MethodRemoveState )
	{}

	bool RemoveState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_stateID );
	}

	bool RemoveState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_stateID );
	}

	RemoveAllStates::RemoveAllStates()
		: CommandData( IParameterableProxy::MethodRemoveAllStates )
	{}

	bool RemoveAllStates::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool RemoveAllStates::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	UseState::UseState()
		: CommandData( IParameterableProxy::MethodUseState )
	{}

	bool UseState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bUseState );
	}

	bool UseState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bUseState );
	}

	LinkStateToStateDefault::LinkStateToStateDefault()
		: CommandData( IParameterableProxy::MethodLinkStateToStateDefault )
	{}

	bool LinkStateToStateDefault::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_stateID );
	}

	bool LinkStateToStateDefault::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_stateID );
	}

	SetStateSyncType::SetStateSyncType()
		: CommandData( IParameterableProxy::MethodSetStateSyncType )
	{}

	bool SetStateSyncType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_eSyncType );
	}

	bool SetStateSyncType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_eSyncType );
	}

	SetFX::SetFX()
		: CommandData( IParameterableProxy::MethodSetFX )
	{}

	SetFX::~SetFX()
	{
	}

	bool SetFX::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvInitParamsBlock, m_ulParamBlockSize )
			&& in_rSerializer.Put( m_FXID )
			&& in_rSerializer.Put( m_uFXIndex );
	}

	bool SetFX::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvInitParamsBlock, (AkInt32&)m_ulParamBlockSize )
			&& in_rSerializer.Get( m_FXID )
			&& in_rSerializer.Get( m_uFXIndex );
	}

	SetFXParam::SetFXParam()
		: CommandData( IParameterableProxy::MethodSetFXParam )
	{}

	SetFXParam::~SetFXParam()
	{
	}

	bool SetFXParam::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvInitParamsBlock, m_ulParamBlockSize )
			&& in_rSerializer.Put( m_FXID )
			&& in_rSerializer.Put( m_uFXIndex )
			&& in_rSerializer.Put( m_paramID );
	}

	bool SetFXParam::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvInitParamsBlock, (AkInt32&)m_ulParamBlockSize )
			&& in_rSerializer.Get( m_FXID )
			&& in_rSerializer.Get( m_uFXIndex )
            && in_rSerializer.Get( m_paramID );
	}

	BypassAllFX::BypassAllFX()
		: CommandData( IParameterableProxy::MethodBypassAllFX )
	{}

	bool BypassAllFX::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bBypass );
	}

	bool BypassAllFX::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bBypass );
	}

	BypassFX::BypassFX()
		: CommandData( IParameterableProxy::MethodBypassFX )
	{}

	bool BypassFX::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uFXIndex )
			&& in_rSerializer.Put( m_bBypass );
	}

	bool BypassFX::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uFXIndex )
			&& in_rSerializer.Get( m_bBypass );
	}

	RemoveFX::RemoveFX()
		: CommandData( IParameterableProxy::MethodRemoveFX )
	{}

	bool RemoveFX::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uFXIndex );
	}

	bool RemoveFX::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uFXIndex );
	}

	SetRTPC::SetRTPC()
		: CommandData( IParameterNodeProxy::MethodSetRTPC )
		, m_bWasDeserialized( false )
	{}

	SetRTPC::~SetRTPC()
	{
		if( m_bWasDeserialized && m_pArrayConversion )
			AkFree( s_poolID, m_pArrayConversion );
	}

	bool SetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FXID )
			&& in_rSerializer.Put( m_RTPCID )
			&& in_rSerializer.PutEnum( m_paramID )
			&& in_rSerializer.Put( m_RTPCCurveID )
			&& in_rSerializer.PutEnum( m_eScaling )
			&& in_rSerializer.Put( m_ulConversionArraySize );

		// It is necessary to serialize the number of structures there is in the array beforehand.
		// This is done by serializing m_ulConversionArraySize above.
		for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
			bRet = bRet && in_rSerializer.Put( m_pArrayConversion[i] );

		return bRet;
	}

	bool SetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FXID )
			&& in_rSerializer.Get( m_RTPCID )
			&& in_rSerializer.GetEnum( m_paramID )
			&& in_rSerializer.Get( m_RTPCCurveID )
			&& in_rSerializer.GetEnum( m_eScaling )
			&& in_rSerializer.Get( m_ulConversionArraySize );

		m_pArrayConversion = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_ulConversionArraySize * sizeof( AkRTPCGraphPoint ) );
		m_bWasDeserialized = true;

		// It is necessary to deserialize the number of structures there is in the array beforehand.
		// This is done by deserializing m_ulConversionArraySize above.
		if( m_pArrayConversion )
		{
			for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayConversion[i] );
		}
		else
		{
			bRet = false;
		}

		return bRet;
	}

	UnsetRTPC::UnsetRTPC()
		: CommandData( IParameterNodeProxy::MethodUnsetRTPC )
	{}

	bool UnsetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FXID )
			&& in_rSerializer.PutEnum( m_paramID )
			&& in_rSerializer.Put( m_RTPCCurveID );
	}

	bool UnsetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FXID )
			&& in_rSerializer.GetEnum( m_paramID )
			&& in_rSerializer.Get( m_RTPCCurveID );
	}
};

namespace ParameterNodeProxyCommandData
{
	Volume::Volume()
		: CommandData( IParameterNodeProxy::MethodVolume )
	{}

	bool Volume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_volume )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool Volume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_volume )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	Pitch::Pitch()
		: CommandData( IParameterNodeProxy::MethodPitch )
	{}

	bool Pitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pitchType )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool Pitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pitchType )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	LFEVolume::LFEVolume()
		: CommandData( IParameterNodeProxy::MethodLFEVolume )
	{}

	bool LFEVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LFEVolume )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool LFEVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LFEVolume )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	LPF::LPF()
		: CommandData( IParameterNodeProxy::MethodLPF )
	{}

	bool LPF::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LPF )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool LPF::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LPF )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	PosSetPositioningType::PosSetPositioningType()
		: CommandData( IParameterNodeProxy::MethodPosSetPositioningType )
	{}

	bool PosSetPositioningType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_ePosType );
	}

	bool PosSetPositioningType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_ePosType );
	}

	PosSetSpatializationEnabled::PosSetSpatializationEnabled()
		: CommandData( IParameterNodeProxy::MethodPosSetSpatializationEnabled )
	{}

	bool PosSetSpatializationEnabled::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsSpatializationEnabled );
	}

	bool PosSetSpatializationEnabled::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsSpatializationEnabled );
	}

	PosSetAttenuationID::PosSetAttenuationID()
		: CommandData( IParameterNodeProxy::MethodPosSetAttenuationID )
	{}

	bool PosSetAttenuationID::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uAttenuationID );
	}

	bool PosSetAttenuationID::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uAttenuationID );
	}



	PosSetCenterPct::PosSetCenterPct()
		: CommandData( IParameterNodeProxy::MethodPosSetCenterPct )
	{}

	bool PosSetCenterPct::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_iCenterPct );
	}

	bool PosSetCenterPct::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_iCenterPct );
	}

	PosSetPAN_RL::PosSetPAN_RL()
		: CommandData( IParameterNodeProxy::MethodPosSetPAN_RL )
	{}

	bool PosSetPAN_RL::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fPanRL );
	}

	bool PosSetPAN_RL::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fPanRL );
	}

	PosSetPAN_FR::PosSetPAN_FR()
		: CommandData( IParameterNodeProxy::MethodPosSetPAN_FR )
	{}

	bool PosSetPAN_FR::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fPanFR );
	}

	bool PosSetPAN_FR::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fPanFR );
	}

	PosSetPannerEnabled::PosSetPannerEnabled()
		: CommandData( IParameterNodeProxy::MethodPosPosSetPannerEnabled )
	{}

	bool PosSetPannerEnabled::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsPannerEnabled );
	}

	bool PosSetPannerEnabled::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsPannerEnabled );
	}

	PosSetIsPositionDynamic::PosSetIsPositionDynamic()
		: CommandData( IParameterNodeProxy::MethodPosSetIsPositionDynamic )
	{}

	bool PosSetIsPositionDynamic::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsDynamic );
	}

	bool PosSetIsPositionDynamic::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsDynamic );
	}

	PosSetPathMode::PosSetPathMode()
		: CommandData( IParameterNodeProxy::MethodPosSetPathMode )
	{}

	bool PosSetPathMode::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_ePathMode );
	}

	bool PosSetPathMode::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_ePathMode );
	}

	PosSetIsLooping::PosSetIsLooping()
		: CommandData( IParameterNodeProxy::MethodPosSetIsLooping )
	{}

	bool PosSetIsLooping::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsLooping );
	}

	bool PosSetIsLooping::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsLooping );
	}

	PosSetTransition::PosSetTransition()
		: CommandData( IParameterNodeProxy::MethodPosSetTransition )
	{}

	bool PosSetTransition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_transitionTime );
	}

	bool PosSetTransition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_transitionTime );
	}

	PosSetPath::PosSetPath()
		: CommandData( IParameterNodeProxy::MethodPosSetPath )
		, m_bWasDeserialized( false )
	{}

	PosSetPath::~PosSetPath()
	{
		if( m_bWasDeserialized )
		{
			if( m_pArrayVertex )
				AkFree( s_poolID, m_pArrayVertex );
			if( m_pArrayPlaylist )
				AkFree( s_poolID, m_pArrayPlaylist );
		}
	}

	bool PosSetPath::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulNumVertices );

		for( AkUInt32 i = 0; i < m_ulNumVertices && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayVertex[i] );

		bRet = bRet && in_rSerializer.Put( m_ulNumPlaylistItem );

		for( AkUInt32 i = 0; i < m_ulNumPlaylistItem && bRet; ++i )
			bRet = bRet && in_rSerializer.Put( m_pArrayPlaylist[i] );

		return bRet;
	}

	bool PosSetPath::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulNumVertices );

		m_pArrayVertex = (AkPathVertex*)AkAlloc( s_poolID, m_ulNumVertices * sizeof( AkPathVertex ) );

		if( m_pArrayVertex )
		{
			for( AkUInt32 i = 0; i < m_ulNumVertices && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayVertex[i] );
		}
		else
		{
			bRet = false;
		}

		bRet = bRet 
			&& in_rSerializer.Get( m_ulNumPlaylistItem );

		m_pArrayPlaylist = (AkPathListItemOffset*)AkAlloc( s_poolID, m_ulNumPlaylistItem * sizeof( AkPathListItemOffset ) );

		if( m_pArrayPlaylist )
		{
			for( AkUInt32 i = 0; i < m_ulNumPlaylistItem && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayPlaylist[i] );
		}
		else
		{
			bRet = false;
		}

		m_bWasDeserialized = true;

		return bRet;
	}
	
	PosUpdatePathPoint::PosUpdatePathPoint()
		: CommandData( IParameterNodeProxy::MethodPosUpdatePathPoint )
	{}

	bool PosUpdatePathPoint::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulPathIndex )
			&& in_rSerializer.Put( m_ulVertexIndex )
			&& in_rSerializer.Put( m_ptPosition )
			&& in_rSerializer.Put( m_delayToNext );
	}

	bool PosUpdatePathPoint::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulPathIndex )
			&& in_rSerializer.Get( m_ulVertexIndex )
			&& in_rSerializer.Get( m_ptPosition )
			&& in_rSerializer.Get( m_delayToNext );
	}

	OverrideFXParent::OverrideFXParent()
		: CommandData( IParameterNodeProxy::MethodOverrideFXParent )
	{}

	OverrideFXParent::~OverrideFXParent()
	{
	}

	bool OverrideFXParent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsFXOverrideParent );
	}

	bool OverrideFXParent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsFXOverrideParent );
	}

	SetBelowThresholdBehavior::SetBelowThresholdBehavior()
		: CommandData( IParameterNodeProxy::MethodSetBelowThresholdBehavior )
	{}

	SetBelowThresholdBehavior::~SetBelowThresholdBehavior()
	{
	}

	bool SetBelowThresholdBehavior::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eBelowThresholdBehavior );
	}

	bool SetBelowThresholdBehavior::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eBelowThresholdBehavior );
	}

	SetMaxNumInstancesOverrideParent::SetMaxNumInstancesOverrideParent()
		: CommandData( IParameterNodeProxy::MethodSetSetMaxNumInstancesOverrideParent )
	{}

	SetMaxNumInstancesOverrideParent::~SetMaxNumInstancesOverrideParent()
	{
	}

	bool SetMaxNumInstancesOverrideParent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bOverride );
	}

	bool SetMaxNumInstancesOverrideParent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bOverride );
	}

	SetVVoicesOptOverrideParent::SetVVoicesOptOverrideParent()
		: CommandData( IParameterNodeProxy::MethodSetVVoicesOptOverrideParent )
	{}

	SetVVoicesOptOverrideParent::~SetVVoicesOptOverrideParent()
	{
	}

	bool SetVVoicesOptOverrideParent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bOverride );
	}

	bool SetVVoicesOptOverrideParent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bOverride );
	}

	SetMaxNumInstances::SetMaxNumInstances()
		: CommandData( IParameterNodeProxy::MethodSetMaxNumInstances )
	{}

	SetMaxNumInstances::~SetMaxNumInstances()
	{
	}

	bool SetMaxNumInstances::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_u16MaxNumInstance );
	}

	bool SetMaxNumInstances::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_u16MaxNumInstance );
	}

	SetMaxReachedBehavior::SetMaxReachedBehavior()
		: CommandData( IParameterNodeProxy::MethodSetMaxReachedBehavior )
	{}

	SetMaxReachedBehavior::~SetMaxReachedBehavior()
	{
	}

	bool SetMaxReachedBehavior::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bKillNewest );
	}

	bool SetMaxReachedBehavior::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bKillNewest );
	}

	SetVirtualQueueBehavior::SetVirtualQueueBehavior()
		: CommandData( IParameterNodeProxy::MethodSetVirtualQueueBehavior )
	{}

	SetVirtualQueueBehavior::~SetVirtualQueueBehavior()
	{
	}

	bool SetVirtualQueueBehavior::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eBehavior );
	}

	bool SetVirtualQueueBehavior::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eBehavior );
	}

	FeedbackVolume::FeedbackVolume()
		: CommandData( IParameterNodeProxy::MethodFeedbackVolume )
	{}

	bool FeedbackVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FeedbackVolume )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool FeedbackVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FeedbackVolume )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
	FeedbackLPF::FeedbackLPF()
		: CommandData( IParameterNodeProxy::MethodFeedbackLPF )
	{}

	bool FeedbackLPF::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FeedbackLPF )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool FeedbackLPF::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FeedbackLPF )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace BusProxyCommandData
{
	Volume::Volume()
		: CommandData( IBusProxy::MethodVolume )
	{}

	bool Volume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_volume );
	}

	bool Volume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_volume );
	}

	LFEVolume::LFEVolume()
		: CommandData( IBusProxy::MethodLFEVolume )
	{}

	bool LFEVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LFEVolume );
	}

	bool LFEVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LFEVolume );
	}

	Pitch::Pitch()
		: CommandData( IBusProxy::MethodPitch )
	{}

	bool Pitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pitch );
	}

	bool Pitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pitch );
	}

	LPF::LPF()
		: CommandData( IBusProxy::MethodLPF )
	{}

	bool LPF::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LPF );
	}

	bool LPF::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LPF );
	}

	SetMaxNumInstancesOverrideParent::SetMaxNumInstancesOverrideParent()
		: CommandData( IBusProxy::MethodSetMaxNumInstancesOverrideParent )
	{}

	bool SetMaxNumInstancesOverrideParent::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bOverride );
	}

	bool SetMaxNumInstancesOverrideParent::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bOverride );
	}

	SetMaxNumInstances::SetMaxNumInstances()
		: CommandData( IBusProxy::MethodSetMaxNumInstances )
	{}

	bool SetMaxNumInstances::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulMaxNumInstance );
	}

	bool SetMaxNumInstances::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulMaxNumInstance );
	}

	SetMaxReachedBehavior::SetMaxReachedBehavior()
		: CommandData( IBusProxy::MethodSetMaxReachedBehavior )
	{}

	bool SetMaxReachedBehavior::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bKillNewest );
	}

	bool SetMaxReachedBehavior::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bKillNewest );
	}

	AddChild::AddChild()
		: CommandData( IBusProxy::MethodAddChild )
	{}

	bool AddChild::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_id );
	}

	bool AddChild::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_id );
	}

	RemoveChild::RemoveChild()
		: CommandData( IBusProxy::MethodRemoveChild )
	{}

	bool RemoveChild::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_id );
	}

	bool RemoveChild::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_id );
	}

	RemoveAllChildren::RemoveAllChildren()
		: CommandData( IBusProxy::MethodRemoveAllChildren )
	{}

	bool RemoveAllChildren::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool RemoveAllChildren::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	SetRecoveryTime::SetRecoveryTime()
		: CommandData( IBusProxy::MethodSetRecoveryTime )
	{}

	bool SetRecoveryTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_recoveryTime );
	}

	bool SetRecoveryTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_recoveryTime );
	}

	AddDuck::AddDuck()
		: CommandData( IBusProxy::MethodAddDuck )
	{}

	bool AddDuck::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_busID )
			&& in_rSerializer.Put( m_duckVolume )
			&& in_rSerializer.Put( m_fadeOutTime )
			&& in_rSerializer.Put( m_fadeInTime )
			&& in_rSerializer.PutEnum( m_eFadeCurve );
	}

	bool AddDuck::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_busID )
			&& in_rSerializer.Get( m_duckVolume )
			&& in_rSerializer.Get( m_fadeOutTime )
			&& in_rSerializer.Get( m_fadeInTime )
			&& in_rSerializer.GetEnum( m_eFadeCurve );
	}

	RemoveDuck::RemoveDuck()
		: CommandData( IBusProxy::MethodRemoveDuck )
	{}

	bool RemoveDuck::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_busID );
	}

	bool RemoveDuck::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_busID );
	}

	RemoveAllDuck::RemoveAllDuck()
		: CommandData( IBusProxy::MethodRemoveAllDuck )
	{}

	bool RemoveAllDuck::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool RemoveAllDuck::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	SetAsBackgroundMusic::SetAsBackgroundMusic()
		: CommandData( IBusProxy::MethodSetAsBackgroundMusic )
	{}

	bool SetAsBackgroundMusic::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool SetAsBackgroundMusic::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	UnsetAsBackgroundMusic::UnsetAsBackgroundMusic()
		: CommandData( IBusProxy::MethodUnsetAsBackgroundMusic )
	{}

	bool UnsetAsBackgroundMusic::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool UnsetAsBackgroundMusic::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	EnableWiiCompressor::EnableWiiCompressor()
		: CommandData( IBusProxy::MethodEnableWiiCompressor )
	{}

	bool EnableWiiCompressor::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bEnable );
	}

	bool EnableWiiCompressor::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bEnable );
	}

	FeedbackVolume::FeedbackVolume()
		: CommandData( IBusProxy::MethodFeedbackVolume )
	{}

	bool FeedbackVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FeedbackVolume );
	}

	bool FeedbackVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FeedbackVolume );
	}	
	FeedbackLPF::FeedbackLPF()
		: CommandData( IBusProxy::MethodFeedbackLPF )
	{}
	bool FeedbackLPF::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_FeedbackLPF );
	}

	bool FeedbackLPF::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_FeedbackLPF);
	}
};

namespace SoundProxyCommandData
{
	SetSource::SetSource()
		: CommandData( ISoundProxy::MethodSetSource )
	{}

	bool SetSource::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		AKASSERT( !"Serialization functions should not be called on remote setSource" );
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( (const char*)m_pszSourceName );
	}

	bool SetSource::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		AKASSERT( !"Serialization functions should not be called on remote setSource" );
		AKASSERT( sizeof( char ) == 1 );
		
		AkInt32 iRead = 0;

		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( (char*&)m_pszSourceName, iRead );
	}

	SetSource_Plugin::SetSource_Plugin()
		: CommandData( ISoundProxy::MethodSetSource_Plugin )
	{}
	
	SetSource_Plugin::~SetSource_Plugin()
	{
	}

	bool SetSource_Plugin::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvData, m_ulSize )
			&& in_rSerializer.Put( m_fxID );
	}

	bool SetSource_Plugin::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvData, (AkInt32&)m_ulSize )
			&& in_rSerializer.Get( m_fxID );
	}

	SetSrcParam::SetSrcParam()
		: CommandData( ISoundProxy::MethodSetSrcParam )
	{}

	SetSrcParam::~SetSrcParam()
	{
	}

	bool SetSrcParam::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvData, m_ulSize )
			&& in_rSerializer.Put( m_fxID )
			&& in_rSerializer.Put( m_paramID );
	}

	bool SetSrcParam::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvData, (AkInt32&)m_ulSize )
			&& in_rSerializer.Get( m_fxID )
			&& in_rSerializer.Get( m_paramID );
	}

	Loop::Loop()
		: CommandData( ISoundProxy::MethodLoop )
	{}

	bool Loop::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsLoopEnabled )
			&& in_rSerializer.Put( m_bIsLoopInfinite )
			&& in_rSerializer.Put( m_loopCount )
			&& in_rSerializer.Put( m_countModMin )
			&& in_rSerializer.Put( m_countModMax );
	}

	bool Loop::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsLoopEnabled )
			&& in_rSerializer.Get( m_bIsLoopInfinite )
			&& in_rSerializer.Get( m_loopCount )
			&& in_rSerializer.Get( m_countModMin )
			&& in_rSerializer.Get( m_countModMax );
	}

	IsZeroLatency::IsZeroLatency()
		: CommandData( ISoundProxy::MethodIsZeroLatency )
	{}

	bool IsZeroLatency::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsZeroLatency );
	}

	bool IsZeroLatency::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsZeroLatency );
	}
};

namespace TrackProxyCommandData
{
	SetPlayList::SetPlayList()
		: CommandData( ITrackProxy::MethodSetPlayList )
		, m_bWasDeserialized( false )
	{}

	SetPlayList::~SetPlayList()
	{
		if( m_bWasDeserialized && m_pArrayPlaylistItems )
		{
			AkFree( s_poolID, m_pArrayPlaylistItems );
		}
	}

	bool SetPlayList::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uNumSubTrack )
			&& in_rSerializer.Put( m_uNumPlaylistItem );

		for( AkUInt32 i = 0; i < m_uNumPlaylistItem && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayPlaylistItems[i] );

		return bRet;
	}

	bool SetPlayList::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uNumSubTrack )
			&& in_rSerializer.Get( m_uNumPlaylistItem );

		m_pArrayPlaylistItems = (AkTrackSrcInfo*)AkAlloc( s_poolID, m_uNumPlaylistItem * sizeof( AkTrackSrcInfo ) );

		if( m_pArrayPlaylistItems )
		{
			for( AkUInt32 i = 0; i < m_uNumPlaylistItem && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayPlaylistItems[i] );
		}
		else
		{
			bRet = false;
		}

		m_bWasDeserialized = true;

		return bRet;
	}

	SetSrcParam::SetSrcParam()
		: CommandData( ITrackProxy::MethodSetSrcParam )
	{}

	SetSrcParam::~SetSrcParam()
	{
	}

	bool SetSrcParam::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvData, m_ulSize )
			&& in_rSerializer.Put( m_sourceID )
			&& in_rSerializer.Put( m_fxID )
			&& in_rSerializer.Put( m_paramID );
	}

	bool SetSrcParam::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvData, (AkInt32&)m_ulSize )
			&& in_rSerializer.Get( m_sourceID )
			&& in_rSerializer.Get( m_fxID )
			&& in_rSerializer.Get( m_paramID );
	}

	Loop::Loop()
		: CommandData( ITrackProxy::MethodLoop )
	{}

	bool Loop::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsLoopEnabled )
			&& in_rSerializer.Put( m_bIsLoopInfinite )
			&& in_rSerializer.Put( m_loopCount )
			&& in_rSerializer.Put( m_countModMin )
			&& in_rSerializer.Put( m_countModMax );
	}

	bool Loop::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsLoopEnabled )
			&& in_rSerializer.Get( m_bIsLoopInfinite )
			&& in_rSerializer.Get( m_loopCount )
			&& in_rSerializer.Get( m_countModMin )
			&& in_rSerializer.Get( m_countModMax );
	}

	IsStreaming::IsStreaming()
		: CommandData( ITrackProxy::MethodIsStreaming )
	{}

	bool IsStreaming::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsStreaming );
	}

	bool IsStreaming::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsStreaming );
	}

	IsZeroLatency::IsZeroLatency()
		: CommandData( ITrackProxy::MethodIsZeroLatency )
	{}

	bool IsZeroLatency::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsZeroLatency );
	}

	bool IsZeroLatency::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsZeroLatency );
	}

	LookAheadTime::LookAheadTime()
		: CommandData( ITrackProxy::MethodLookAheadTime )
	{}

	bool LookAheadTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LookAheadTime );
	}

	bool LookAheadTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LookAheadTime );
	}
	SetMusicTrackRanSeqType::SetMusicTrackRanSeqType()
		: CommandData( ITrackProxy::MethodSetMusicTrackRanSeqType )
	{}

	bool SetMusicTrackRanSeqType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_eRSType );
	}

	bool SetMusicTrackRanSeqType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_eRSType );
	}
};

namespace EventProxyCommandData
{
	Add::Add()
		: CommandData( IEventProxy::MethodAdd )
	{}

	bool Add::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_actionID );
	}

	bool Add::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_actionID );
	}

	Remove::Remove()
		: CommandData( IEventProxy::MethodRemove )
	{}

	bool Remove::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_actionID );
	}

	bool Remove::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_actionID );
	}

	Clear::Clear()
		: CommandData( IEventProxy::MethodClear )
	{}

	bool Clear::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool Clear::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}
};

namespace ActionProxyCommandData
{
	SetElementID::SetElementID()
		: CommandData( IActionProxy::MethodSetElementID )
	{}

	bool SetElementID::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_elementID );
	}

	bool SetElementID::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_elementID );
	}

	SetActionType::SetActionType()
		: CommandData( IActionProxy::MethodSetActionType )
	{}

	bool SetActionType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_actionType );
	}

	bool SetActionType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_actionType );
	}

	Delay::Delay()
		: CommandData( IActionProxy::MethodDelay )
	{}

	bool Delay::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_delay )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool Delay::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_delay )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace ActionPlayProxyCommandData
{
	TransitionTime::TransitionTime()
		: CommandData( IActionPlayProxy::MethodTransitionTime )
	{}

	bool TransitionTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_transitionTime )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool TransitionTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_transitionTime )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	CurveType::CurveType()
		: CommandData( IActionPlayProxy::MethodCurveType)
	{}

	bool CurveType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eCurveType );
	}

	bool CurveType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eCurveType );
	}
};

namespace ActionExceptProxyCommandData
{
	AddException::AddException()
		: CommandData( IActionExceptProxy::MethodAddException )
	{}

	bool AddException::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_elementID );
	}

	bool AddException::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_elementID );
	}

	RemoveException::RemoveException()
		: CommandData( IActionExceptProxy::MethodRemoveException )
	{}

	bool RemoveException::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_elementID );
	}

	bool RemoveException::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_elementID );
	}

	ClearExceptions::ClearExceptions()
		: CommandData( IActionExceptProxy::MethodClearExceptions )
	{}

	bool ClearExceptions::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool ClearExceptions::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}
};

namespace ActionActiveProxyCommandData
{
	TransitionTime::TransitionTime()
		: CommandData( IActionActiveProxy::MethodTransitionTime )
	{}

	bool TransitionTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_transitionTime )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool TransitionTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_transitionTime )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	CurveType::CurveType()
		: CommandData( IActionActiveProxy::MethodCurveType )
	{}

	bool CurveType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eCurveType );
	}

	bool CurveType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eCurveType );
	}
};

namespace ActionPauseProxyCommandData
{
	IncludePendingResume::IncludePendingResume()
		: CommandData( IActionPauseProxy::MethodIncludePendingResume )
	{}

	bool IncludePendingResume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIncludePendingResume );
	}

	bool IncludePendingResume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIncludePendingResume );
	}
};

namespace ActionResumeProxyCommandData
{
	IsMasterResume::IsMasterResume()
		: CommandData( IActionResumeProxy::MethodIsMasterResume )
	{}

	bool IsMasterResume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsMasterResume );
	}

	bool IsMasterResume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsMasterResume );
	}
};

namespace ActionSetValueProxyCommandData
{
	TransitionTime::TransitionTime()
		: CommandData( IActionSetValueProxy::MethodTransitionTime )
	{}

	bool TransitionTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_transitionTime )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool TransitionTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_transitionTime )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}

	CurveType::CurveType()
		: CommandData( IActionSetValueProxy::MethodCurveType )
	{}

	bool CurveType::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eCurveType );
	}

	bool CurveType::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eCurveType );
	}
};

namespace ActionSetPitchProxyCommandData
{
	SetValue::SetValue()
		: CommandData( IActionSetPitchProxy::MethodSetValue )
	{}

	bool SetValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pitchType )
			&& in_rSerializer.PutEnum( m_eValueMeaning )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool SetValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pitchType )
			&& in_rSerializer.GetEnum( m_eValueMeaning )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace ActionSetVolumeProxyCommandData
{
	SetValue::SetValue()
		: CommandData( IActionSetVolumeProxy::MethodSetValue )
	{}

	bool SetValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_value )
			&& in_rSerializer.PutEnum( m_eValueMeaning )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool SetValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_value )
			&& in_rSerializer.GetEnum( m_eValueMeaning )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace ActionSetLFEProxyCommandData
{
	SetValue::SetValue()
		: CommandData( IActionSetLFEProxy::MethodSetValue )
	{}

	bool SetValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_value )
			&& in_rSerializer.PutEnum( m_eValueMeaning )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool SetValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_value )
			&& in_rSerializer.GetEnum( m_eValueMeaning )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace ActionSetLPFProxyCommandData
{
	SetValue::SetValue()
		: CommandData( IActionSetLPFProxy::MethodSetValue )
	{}

	bool SetValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_value )
			&& in_rSerializer.PutEnum( m_eValueMeaning )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool SetValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_value )
			&& in_rSerializer.GetEnum( m_eValueMeaning )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace ActionSetStateProxyCommandData
{
	SetGroup::SetGroup()
		: CommandData( IActionSetStateProxy::MethodSetGroup )
	{}

	bool SetGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_groupID );
	}

	bool SetGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_groupID );
	}

	SetTargetState::SetTargetState()
		: CommandData( IActionSetStateProxy::MethodSetTargetState )
	{}

	bool SetTargetState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_stateID );
	}

	bool SetTargetState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_stateID );
	}
};

namespace ActionSetSwitchProxyCommandData
{
	SetSwitchGroup::SetSwitchGroup()
		: CommandData( IActionSetSwitchProxy::MethodSetSwitchGroup )
	{}

	bool SetSwitchGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulSwitchGroupID );
	}

	bool SetSwitchGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulSwitchGroupID );
	}

	SetTargetSwitch::SetTargetSwitch()
		: CommandData( IActionSetSwitchProxy::MethodSetTargetSwitch )
	{}

	bool SetTargetSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulSwitchID );
	}

	bool SetTargetSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulSwitchID );
	}
};

namespace ActionSetRTPCProxyCommandData
{
	SetRTPCGroup::SetRTPCGroup()
		: CommandData( IActionSetRTPCProxy::MethodSetRTPCGroup )
	{}

	bool SetRTPCGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_RTPCGroupID );
	}

	bool SetRTPCGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_RTPCGroupID );
	}

	SetRTPCValue::SetRTPCValue()
		: CommandData( IActionSetRTPCProxy::MethodSetRTPCValue )
	{}

	bool SetRTPCValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fRTPCValue );
	}

	bool SetRTPCValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fRTPCValue );
	}
};

namespace ActionUseStateProxyCommandData
{
	UseState::UseState()
		: CommandData( IActionUseStateProxy::MethodUseState )
	{}

	bool UseState::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bUseState );
	}

	bool UseState::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bUseState );
	}
};

namespace ActionBypassFXProxyCommandData
{
	BypassFX::BypassFX()
		: CommandData( IActionBypassFXProxy::MethodBypassFX )
	{}

	bool BypassFX::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bBypassFX );
	}

	bool BypassFX::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bBypassFX );
	}

	SetBypassTarget::SetBypassTarget()
		: CommandData( IActionBypassFXProxy::MethodSetBypassTarget )
	{}

	bool SetBypassTarget::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bBypassAllFX )
			&& in_rSerializer.Put( m_ucEffectsMask );
	}

	bool SetBypassTarget::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bBypassAllFX )
			&& in_rSerializer.Get( m_ucEffectsMask );
	}
};

namespace StateProxyCommandData
{
	Volume::Volume()
		: CommandData( IStateProxy::MethodVolume )
	{}

	bool Volume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_volume );
	}

	bool Volume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_volume );
	}

	VolumeMeaning::VolumeMeaning()
		: CommandData( IStateProxy::MethodVolumeMeaning )
	{}

	bool VolumeMeaning::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eMeaning );
	}

	bool VolumeMeaning::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eMeaning );
	}

	Pitch::Pitch()
		: CommandData( IStateProxy::MethodPitch )
	{}

	bool Pitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pitchType );
	}

	bool Pitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pitchType );
	}

	PitchMeaning::PitchMeaning()
		: CommandData( IStateProxy::MethodPitchMeaning )
	{}

	bool PitchMeaning::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eMeaning );
	}

	bool PitchMeaning::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eMeaning );
	}

	LPF::LPF()
		: CommandData( IStateProxy::MethodLPF )
	{}

	bool LPF::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LPF );
	}

	bool LPF::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LPF );
	}

	LPFMeaning::LPFMeaning()
		: CommandData( IStateProxy::MethodLPFMeaning )
	{}

	bool LPFMeaning::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eMeaning );
	}

	bool LPFMeaning::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eMeaning );
	}

	LFEVolume::LFEVolume()
		: CommandData( IStateProxy::MethodLFEVolume )
	{}

	bool LFEVolume::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LFEVolume );
	}

	bool LFEVolume::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LFEVolume );
	}

	LFEVolumeMeaning::LFEVolumeMeaning()
		: CommandData( IStateProxy::MethodLFEVolumeMeaning )
	{}

	bool LFEVolumeMeaning::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eMeaning );
	}

	bool LFEVolumeMeaning::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eMeaning );
	}
};

namespace AttenuationProxyCommandData
{
	SetAttenuationParams::SetAttenuationParams()
		: CommandData( IAttenuationProxy::MethodSetAttenuationParams )
		, m_bWasDeserialized( false )
	{}

	SetAttenuationParams::~SetAttenuationParams()
	{
		if( m_bWasDeserialized )
		{
			if( m_Params.paCurves )
			{
				for( AkUInt32 i = 0; i < m_Params.uNumCurves; ++i )
				{
					if( m_Params.paCurves[i].m_pArrayConversion )
					{
						AkFree( s_poolID, m_Params.paCurves[i].m_pArrayConversion );
					}
				}
				AkFree( s_poolID, m_Params.paCurves );
			}

			if( m_Params.paRTPCReg )
			{
				for( AkUInt32 i = 0; i < m_Params.uNumRTPCReg; ++i )
				{
					if( m_Params.paRTPCReg[i].m_pArrayConversion )
					{
						AkFree( s_poolID, m_Params.paRTPCReg[i].m_pArrayConversion );
					}
				}
				AkFree( s_poolID, m_Params.paRTPCReg );
			}
		}
	}

	bool SetAttenuationParams::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_Params.Cone.bIsConeEnabled )
			&& in_rSerializer.Put( m_Params.Cone.cone_fInsideAngle )
			&& in_rSerializer.Put( m_Params.Cone.cone_fOutsideAngle )
			&& in_rSerializer.Put( m_Params.Cone.cone_fOutsideVolume )
			&& in_rSerializer.Put( m_Params.Cone.cone_LoPass );

		for( AkUInt32 i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
		{
			bRet = bRet && in_rSerializer.Put( m_Params.CurveIndexes[i] );
		}

		bRet = bRet && in_rSerializer.Put( m_Params.uNumCurves );

		for( AkUInt32 nCurveIndex = 0; nCurveIndex < m_Params.uNumCurves; ++nCurveIndex )
		{
			bRet = bRet && in_rSerializer.PutEnum( m_Params.paCurves[nCurveIndex].m_eScaling );
			bRet = bRet && in_rSerializer.Put( m_Params.paCurves[nCurveIndex].m_ulConversionArraySize );

			for( AkUInt32 nPointIndex = 0; nPointIndex < m_Params.paCurves[nCurveIndex].m_ulConversionArraySize && bRet; ++nPointIndex )
				bRet = bRet && in_rSerializer.Put( m_Params.paCurves[nCurveIndex].m_pArrayConversion[nPointIndex] );
		}

		bRet = bRet && in_rSerializer.Put( m_Params.uNumRTPCReg );

		for( AkUInt32 i = 0; i < m_Params.uNumRTPCReg; ++i )
		{
			bRet = bRet && in_rSerializer.Put( m_Params.paRTPCReg[i].m_FXID );
			bRet = bRet && in_rSerializer.Put( m_Params.paRTPCReg[i].m_RTPCID );
			bRet = bRet && in_rSerializer.PutEnum( m_Params.paRTPCReg[i].m_paramID );
			bRet = bRet && in_rSerializer.Put( m_Params.paRTPCReg[i].m_RTPCCurveID );
			bRet = bRet && in_rSerializer.PutEnum( m_Params.paRTPCReg[i].m_eScaling );
			bRet = bRet && in_rSerializer.Put( m_Params.paRTPCReg[i].m_ulConversionArraySize );

			for( AkUInt32 nPointIndex = 0; nPointIndex < m_Params.paRTPCReg[i].m_ulConversionArraySize && bRet; ++nPointIndex )
				bRet = bRet && in_rSerializer.Put( m_Params.paRTPCReg[i].m_pArrayConversion[nPointIndex] );
		}

		return bRet;
	}

	bool SetAttenuationParams::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		m_bWasDeserialized = true;
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_Params.Cone.bIsConeEnabled )
			&& in_rSerializer.Get( m_Params.Cone.cone_fInsideAngle )
			&& in_rSerializer.Get( m_Params.Cone.cone_fOutsideAngle )
			&& in_rSerializer.Get( m_Params.Cone.cone_fOutsideVolume )
			&& in_rSerializer.Get( m_Params.Cone.cone_LoPass );

		for( AkUInt32 i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
		{
			bRet = bRet && in_rSerializer.Get( m_Params.CurveIndexes[i] );
		}

		bRet = bRet && in_rSerializer.Get( m_Params.uNumCurves );

		AKASSERT( m_Params.uNumCurves );//cannot be 0 at this point, at least one is required.

		m_Params.paCurves = (AkWwiseGraphCurve*)AkAlloc( s_poolID, m_Params.uNumCurves * sizeof( AkWwiseGraphCurve ) );

		if( m_Params.paCurves )
		{
			for( AkUInt32 nCurveIndex = 0; nCurveIndex < m_Params.uNumCurves; ++nCurveIndex )
			{
				bRet = bRet && in_rSerializer.GetEnum( m_Params.paCurves[nCurveIndex].m_eScaling );
				bRet = bRet && in_rSerializer.Get( m_Params.paCurves[nCurveIndex].m_ulConversionArraySize );

				if( m_Params.paCurves[nCurveIndex].m_ulConversionArraySize )
				{
					m_Params.paCurves[nCurveIndex].m_pArrayConversion = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_Params.paCurves[nCurveIndex].m_ulConversionArraySize * sizeof( AkRTPCGraphPoint ) );
					if( !(m_Params.paCurves[nCurveIndex].m_pArrayConversion) )
					{
						bRet = false;
					}
				}
				else
					m_Params.paCurves[nCurveIndex].m_pArrayConversion = NULL;

				if( m_Params.paCurves[nCurveIndex].m_pArrayConversion )
				{
					for( AkUInt32 nPointIndex = 0; nPointIndex < m_Params.paCurves[nCurveIndex].m_ulConversionArraySize && bRet; ++nPointIndex )
						bRet = bRet && in_rSerializer.Get( m_Params.paCurves[nCurveIndex].m_pArrayConversion[nPointIndex] );
				}
			}
		}
		else
		{
			bRet = false;
		}

		bRet = bRet && in_rSerializer.Get( m_Params.uNumRTPCReg );

		if( m_Params.uNumRTPCReg )
		{
			m_Params.paRTPCReg = (AkWwiseRTPCreg*)AkAlloc( s_poolID, m_Params.uNumRTPCReg * sizeof( AkWwiseRTPCreg ) );
			if( !m_Params.paRTPCReg )
				bRet = false;
		}
		else
			m_Params.paRTPCReg = NULL;

		if( m_Params.paRTPCReg )
		{
			for( AkUInt32 i = 0; i < m_Params.uNumRTPCReg; ++i )
			{
				bRet = bRet && in_rSerializer.Get( m_Params.paRTPCReg[i].m_FXID );
				bRet = bRet && in_rSerializer.Get( m_Params.paRTPCReg[i].m_RTPCID );
				bRet = bRet && in_rSerializer.GetEnum( m_Params.paRTPCReg[i].m_paramID );
				bRet = bRet && in_rSerializer.Get( m_Params.paRTPCReg[i].m_RTPCCurveID );
				bRet = bRet && in_rSerializer.GetEnum( m_Params.paRTPCReg[i].m_eScaling );
				bRet = bRet && in_rSerializer.Get( m_Params.paRTPCReg[i].m_ulConversionArraySize );

				if( m_Params.paRTPCReg[i].m_ulConversionArraySize )
				{
					m_Params.paRTPCReg[i].m_pArrayConversion = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_Params.paRTPCReg[i].m_ulConversionArraySize * sizeof( AkRTPCGraphPoint ) );
					if( !m_Params.paRTPCReg[i].m_pArrayConversion )
					{
						bRet = false;
					}
				}
				else
					m_Params.paRTPCReg[i].m_pArrayConversion = NULL;

				if( m_Params.paRTPCReg[i].m_ulConversionArraySize )
				{
					for( AkUInt32 nPointIndex = 0; nPointIndex < m_Params.paRTPCReg[i].m_ulConversionArraySize && bRet; ++nPointIndex )
						bRet = bRet && in_rSerializer.Get( m_Params.paRTPCReg[i].m_pArrayConversion[nPointIndex] );
				}
			}
		}

		return bRet;
	}
};

namespace HierarchicalProxyCommandData
{
	AddChild::AddChild()
		: CommandData( IHierarchicalProxy::MethodAddChild )
	{}

	bool AddChild::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_id );
	}

	bool AddChild::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_id );
	}

	RemoveChild::RemoveChild()
		: CommandData( IHierarchicalProxy::MethodRemoveChild )
	{}

	bool RemoveChild::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_id );
	}

	bool RemoveChild::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_id );
	}

	RemoveAllChildren::RemoveAllChildren()
		: CommandData( IHierarchicalProxy::MethodRemoveAllChildren )
	{}

	bool RemoveAllChildren::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool RemoveAllChildren::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}
};

namespace RanSeqContainerProxyCommandData
{
	Mode::Mode()
		: CommandData( IRanSeqContainerProxy::MethodMode )
	{}

	bool Mode::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eMode );
	}

	bool Mode::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eMode );
	}

	IsGlobal::IsGlobal()
		: CommandData( IRanSeqContainerProxy::MethodIsGlobal )
	{}

	bool IsGlobal::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsGlobal );
	}

	bool IsGlobal::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsGlobal );
	}

	AddPlaylistItem::AddPlaylistItem()
		: CommandData( IRanSeqContainerProxy::MethodAddPlaylistItem )
	{}

	bool AddPlaylistItem::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_elementID )
			&& in_rSerializer.Put( m_weight );
	}

	bool AddPlaylistItem::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_elementID )
			&& in_rSerializer.Get( m_weight );
	}

	RemovePlaylistItem::RemovePlaylistItem()
		: CommandData( IRanSeqContainerProxy::MethodRemovePlaylistItem )
	{}

	bool RemovePlaylistItem::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_elementID );
	}

	bool RemovePlaylistItem::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_elementID );
	}

	ClearPlaylist::ClearPlaylist()
		: CommandData( IRanSeqContainerProxy::MethodClearPlaylist )
	{}

	bool ClearPlaylist::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool ClearPlaylist::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	ResetPlayListAtEachPlay::ResetPlayListAtEachPlay()
		: CommandData( IRanSeqContainerProxy::MethodResetPlayListAtEachPlay )
	{}

	bool ResetPlayListAtEachPlay::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bResetPlayListAtEachPlay );
	}

	bool ResetPlayListAtEachPlay::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bResetPlayListAtEachPlay );
	}

	RestartBackward::RestartBackward()
		: CommandData( IRanSeqContainerProxy::MethodRestartBackward )
	{}

	bool RestartBackward::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bRestartBackward );
	}

	bool RestartBackward::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bRestartBackward );
	}

	Continuous::Continuous()
		: CommandData( IRanSeqContainerProxy::MethodContinuous )
	{}

	bool Continuous::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsContinuous );
	}

	bool Continuous::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsContinuous );
	}

	ForceNextToPlay::ForceNextToPlay()
		: CommandData( IRanSeqContainerProxy::MethodForceNextToPlay )
	{}

	bool ForceNextToPlay::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_position )
			&& in_rSerializer.Put( m_gameObjPtr )
			&& in_rSerializer.Put( m_playingID );
	}

	bool ForceNextToPlay::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_position )
			&& in_rSerializer.Get( m_gameObjPtr )
			&& in_rSerializer.Get( m_playingID );
	}

	NextToPlay::NextToPlay()
		: CommandData( IRanSeqContainerProxy::MethodNextToPlay )
	{}

	bool NextToPlay::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_gameObjPtr );
	}

	bool NextToPlay::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_gameObjPtr );
	}

	RandomMode::RandomMode()
		: CommandData( IRanSeqContainerProxy::MethodRandomMode )
	{}

	bool RandomMode::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eRandomMode );
	}

	bool RandomMode::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eRandomMode );
	}

	AvoidRepeatingCount::AvoidRepeatingCount()
		: CommandData( IRanSeqContainerProxy::MethodAvoidRepeatingCount )
	{}

	bool AvoidRepeatingCount::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_count );
	}

	bool AvoidRepeatingCount::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_count );
	}

	SetItemWeight_withID::SetItemWeight_withID()
		: CommandData( IRanSeqContainerProxy::MethodSetItemWeight_withID )
	{}

	bool SetItemWeight_withID::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_itemID )
			&& in_rSerializer.Put( m_weight );
	}

	bool SetItemWeight_withID::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_itemID )
			&& in_rSerializer.Get( m_weight );
	}

	SetItemWeight_withPosition::SetItemWeight_withPosition()
		: CommandData( IRanSeqContainerProxy::MethodSetItemWeight_withPosition )
	{}

	bool SetItemWeight_withPosition::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_position )
			&& in_rSerializer.Put( m_weight );
	}

	bool SetItemWeight_withPosition::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_position )
			&& in_rSerializer.Get( m_weight );
	}

	Loop::Loop()
		: CommandData( IRanSeqContainerProxy::MethodLoop )
	{}

	bool Loop::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsLoopEnabled )
			&& in_rSerializer.Put( m_bIsLoopInfinite )
			&& in_rSerializer.Put( m_loopCount )
			&& in_rSerializer.Put( m_countModMin )
			&& in_rSerializer.Put( m_countModMax );
	}

	bool Loop::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsLoopEnabled )
			&& in_rSerializer.Get( m_bIsLoopInfinite )
			&& in_rSerializer.Get( m_loopCount )
			&& in_rSerializer.Get( m_countModMin )
			&& in_rSerializer.Get( m_countModMax );
	}

	TransitionMode::TransitionMode()
		: CommandData( IRanSeqContainerProxy::MethodTransitionMode )
	{}

	bool TransitionMode::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_eTransitionMode );
	}

	bool TransitionMode::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_eTransitionMode );
	}

	TransitionTime::TransitionTime()
		: CommandData( IRanSeqContainerProxy::MethodTransitionTime )
	{}

	bool TransitionTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_transitionTime )
			&& in_rSerializer.Put( m_rangeMin )
			&& in_rSerializer.Put( m_rangeMax );
	}

	bool TransitionTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_transitionTime )
			&& in_rSerializer.Get( m_rangeMin )
			&& in_rSerializer.Get( m_rangeMax );
	}
};

namespace SwitchContainerProxyCommandData
{
	SetSwitchGroup::SetSwitchGroup()
		: CommandData( ISwitchContainerProxy::MethodSetSwitchGroup )
	{}

	bool SetSwitchGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulGroup )
			&& in_rSerializer.PutEnum( m_eGroupType );
	}

	bool SetSwitchGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulGroup )
			&& in_rSerializer.GetEnum( m_eGroupType );
	}

	SetDefaultSwitch::SetDefaultSwitch()
		: CommandData( ISwitchContainerProxy::MethodSetDefaultSwitch )
	{}

	bool SetDefaultSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switch );
	}

	bool SetDefaultSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switch );
	}

	ClearSwitches::ClearSwitches()
		: CommandData( ISwitchContainerProxy::MethodClearSwitches )
	{}

	bool ClearSwitches::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool ClearSwitches::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}

	AddSwitch::AddSwitch()
		: CommandData( ISwitchContainerProxy::MethodAddSwitch )
	{}

	bool AddSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switch );
	}

	bool AddSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switch );
	}

	RemoveSwitch::RemoveSwitch()
		: CommandData( ISwitchContainerProxy::MethodRemoveSwitch )
	{}

	bool RemoveSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switch );
	}

	bool RemoveSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switch );
	}

	AddNodeInSwitch::AddNodeInSwitch()
		: CommandData( ISwitchContainerProxy::MethodAddNodeInSwitch )
	{}

	bool AddNodeInSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switch )
			&& in_rSerializer.Put( m_nodeID );
	}

	bool AddNodeInSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switch )
			&& in_rSerializer.Get( m_nodeID );
	}

	RemoveNodeFromSwitch::RemoveNodeFromSwitch()
		: CommandData( ISwitchContainerProxy::MethodRemoveNodeFromSwitch )
	{}

	bool RemoveNodeFromSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_switch )
			&& in_rSerializer.Put( m_nodeID );
	}

	bool RemoveNodeFromSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_switch )
			&& in_rSerializer.Get( m_nodeID );
	}

	SetContinuousValidation::SetContinuousValidation()
		: CommandData( ISwitchContainerProxy::MethodSetContinuousValidation )
	{}

	bool SetContinuousValidation::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsContinuousCheck );
	}

	bool SetContinuousValidation::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsContinuousCheck );
	}

	SetContinuePlayback::SetContinuePlayback()
		: CommandData( ISwitchContainerProxy::MethodSetContinuePlayback )
	{}

	bool SetContinuePlayback::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_nodeID )
			&& in_rSerializer.Put( m_bContinuePlayback );
	}

	bool SetContinuePlayback::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_nodeID )
			&& in_rSerializer.Get( m_bContinuePlayback );
	}

	SetFadeInTime::SetFadeInTime()
		: CommandData( ISwitchContainerProxy::MethodSetFadeInTime )
	{}

	bool SetFadeInTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_nodeID )
			&& in_rSerializer.Put( m_time );
	}

	bool SetFadeInTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_nodeID )
			&& in_rSerializer.Get( m_time );
	}

	SetFadeOutTime::SetFadeOutTime()
		: CommandData( ISwitchContainerProxy::MethodSetFadeOutTime )
	{}

	bool SetFadeOutTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_nodeID )
			&& in_rSerializer.Put( m_time );
	}

	bool SetFadeOutTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_nodeID )
			&& in_rSerializer.Get( m_time );
	}

	SetOnSwitchMode::SetOnSwitchMode()
		: CommandData( ISwitchContainerProxy::MethodSetOnSwitchMode )
	{}

	bool SetOnSwitchMode::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_nodeID )
			&& in_rSerializer.PutEnum( m_bSwitchMode );
	}

	bool SetOnSwitchMode::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_nodeID )
			&& in_rSerializer.GetEnum( m_bSwitchMode );
	}

	SetIsFirstOnly::SetIsFirstOnly()
		: CommandData( ISwitchContainerProxy::MethodSetIsFirstOnly )
	{}

	bool SetIsFirstOnly::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_nodeID )
			&& in_rSerializer.Put( m_bIsFirstOnly );
	}

	bool SetIsFirstOnly::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_nodeID )
			&& in_rSerializer.Get( m_bIsFirstOnly );
	}
};

namespace LayerContainerProxyCommandData
{
	//
	// AddLayer
	//

	AddLayer::AddLayer()
		: CommandData( ILayerContainerProxy::MethodAddLayer )
		, m_LayerID( 0 )
	{}

	bool AddLayer::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LayerID );
	}

	bool AddLayer::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LayerID );
	}

	//
	// RemoveLayer
	//

	RemoveLayer::RemoveLayer()
		: CommandData( ILayerContainerProxy::MethodRemoveLayer )
		, m_LayerID( 0 )
	{}

	bool RemoveLayer::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_LayerID );
	}

	bool RemoveLayer::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_LayerID );
	}
};

namespace LayerProxyCommandData
{
	//
	// SetRTPC
	//

	SetRTPC::SetRTPC()
		: CommandData( ILayerProxy::MethodSetRTPC )
		, m_RTPCID( 0 )
		, m_paramID( (AkRTPC_ParameterID)0 )
		, m_RTPCCurveID( 0 )
		, m_eScaling( AkCurveScaling_None )
		, m_pArrayConversion( NULL )
		, m_ulConversionArraySize( 0 )
		, m_bWasDeserialized( false )
	{}

	SetRTPC::~SetRTPC()
	{
		if( m_bWasDeserialized && m_pArrayConversion )
			AkFree( s_poolID, m_pArrayConversion );
	}

	bool SetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_RTPCID )
			&& in_rSerializer.PutEnum( m_paramID )
			&& in_rSerializer.Put( m_RTPCCurveID )
			&& in_rSerializer.PutEnum( m_eScaling )
			&& in_rSerializer.Put( m_ulConversionArraySize );

		// It is necessary to serialize the number of structures there is in the array beforehand.
		// This is done by serializing m_ulConversionArraySize above.
		for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
			bRet = bRet && in_rSerializer.Put( m_pArrayConversion[i] );

		return bRet;
	}

	bool SetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_RTPCID )
			&& in_rSerializer.GetEnum( m_paramID )
			&& in_rSerializer.Get( m_RTPCCurveID )
			&& in_rSerializer.GetEnum( m_eScaling )
			&& in_rSerializer.Get( m_ulConversionArraySize );

		m_pArrayConversion = (AkRTPCGraphPoint*)AkAlloc( s_poolID, m_ulConversionArraySize * sizeof( AkRTPCGraphPoint ) );
		m_bWasDeserialized = true;

		// It is necessary to deserialize the number of structures there is in the array beforehand.
		// This is done by deserializing m_ulConversionArraySize above.
		if( m_pArrayConversion )
		{
			for( AkUInt32 i = 0; i < m_ulConversionArraySize && bRet; ++i )
				bRet = bRet && in_rSerializer.Get( m_pArrayConversion[i] );
		}
		else
		{
			bRet = false;
		}

		return bRet;
	}

	//
	// UnsetRTPC
	//

	UnsetRTPC::UnsetRTPC()
		: CommandData( ILayerProxy::MethodUnsetRTPC )
		, m_paramID( (AkRTPC_ParameterID)0 )
		, m_RTPCCurveID( 0 )
	{}

	bool UnsetRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.PutEnum( m_paramID )
			&& in_rSerializer.Put( m_RTPCCurveID );
	}

	bool UnsetRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.GetEnum( m_paramID )
			&& in_rSerializer.Get( m_RTPCCurveID );
	}

	//
	// SetChildAssoc
	//

	SetChildAssoc::SetChildAssoc()
		: CommandData( ILayerProxy::MethodSetChildAssoc )
		, m_ChildID( 0 )
		, m_pCrossfadingCurve( NULL )
		, m_ulCrossfadingCurveSize( 0 )
		, m_bWasDeserialized( false )
	{}

	SetChildAssoc::~SetChildAssoc()
	{
		if( m_bWasDeserialized && m_pCrossfadingCurve )
			AkFree( s_poolID, m_pCrossfadingCurve );
	}

	bool SetChildAssoc::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ChildID )
			&& in_rSerializer.Put( m_ulCrossfadingCurveSize );

		// It is necessary to serialize the number of structures there is in the array beforehand.
		// This is done by serializing m_ulCrossfadingCurveSize above.
		for( AkUInt32 i = 0; i < m_ulCrossfadingCurveSize && bRet; ++i )
			bRet = bRet && in_rSerializer.Put( m_pCrossfadingCurve[i] );

		return bRet;
	}

	bool SetChildAssoc::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ChildID )
			&& in_rSerializer.Get( m_ulCrossfadingCurveSize );

		AKASSERT( ! m_pCrossfadingCurve );
		if ( m_ulCrossfadingCurveSize )
		{
			m_pCrossfadingCurve = (AkRTPCCrossfadingPoint*)AkAlloc( s_poolID, m_ulCrossfadingCurveSize * sizeof( AkRTPCCrossfadingPoint ) );

			// It is necessary to deserialize the number of structures there is in the array beforehand.
			// This is done by deserializing m_ulCrossfadingCurveSize above.
			if( m_pCrossfadingCurve )
			{
				for( AkUInt32 i = 0; i < m_ulCrossfadingCurveSize && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pCrossfadingCurve[i] );
			}
			else
			{
				bRet = false;
			}
		}

		m_bWasDeserialized = true;

		return bRet;
	}

	//
	// UnsetChildAssoc
	//

	UnsetChildAssoc::UnsetChildAssoc()
		: CommandData( ILayerProxy::MethodUnsetChildAssoc )
		, m_ChildID( 0 )
	{}

	bool UnsetChildAssoc::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ChildID );
	}

	bool UnsetChildAssoc::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ChildID );
	}

	//
	// SetCrossfadingRTPC
	//

	SetCrossfadingRTPC::SetCrossfadingRTPC()
		: CommandData( ILayerProxy::MethodSetCrossfadingRTPC )
		, m_rtpcID( 0 )
	{}

	bool SetCrossfadingRTPC::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_rtpcID );
	}

	bool SetCrossfadingRTPC::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_rtpcID );
	}

	//
	// SetCrossfadingRTPCDefaultValue
	//

	SetCrossfadingRTPCDefaultValue::SetCrossfadingRTPCDefaultValue()
		: CommandData( ILayerProxy::MethodSetCrossfadingRTPCDefaultValue )
		, m_fValue( 0 )
	{}

	bool SetCrossfadingRTPCDefaultValue::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fValue );
	}

	bool SetCrossfadingRTPCDefaultValue::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fValue );
	}
};

namespace MusicNodeProxyCommandData
{
	MeterInfo::MeterInfo()
		: CommandData( IMusicNodeProxy::MethodMeterInfo )
	{}

	bool MeterInfo::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsOverrideParent )
			&& in_rSerializer.Put( m_MeterInfo );
	}

	bool MeterInfo::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsOverrideParent )
			&& in_rSerializer.Get( m_MeterInfo );
	}

	SetStingers::SetStingers()
		: CommandData( IMusicNodeProxy::MethodSetStingers )
		, m_bWasDeserialized( false )
		, m_pStingers( NULL )
	{}

	SetStingers::~SetStingers()
	{
		if( m_bWasDeserialized && m_pStingers )
		{
			AkFree( s_poolID, m_pStingers );
		}
	}

	bool SetStingers::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_NumStingers );

		for( AkUInt32 i = 0; i < m_NumStingers && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pStingers[i] );

		return bRet;
	}

	bool SetStingers::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_NumStingers );

		if(m_NumStingers != 0)
		{
			m_pStingers = (CAkStinger*)AkAlloc( s_poolID, m_NumStingers * sizeof( CAkStinger ) );

			if( m_pStingers )
			{
				for( AkUInt32 i = 0; i < m_NumStingers && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pStingers[i] );
			}
			else
			{
				bRet = false;
			}

			m_bWasDeserialized = true;
		}

		return bRet;
	}
};

namespace MusicTransAwareProxyCommandData
{
	SetRules::SetRules()
		: CommandData( IMusicTransAwareProxy::MethodSetRules )
		, m_bWasDeserialized( false )
	{}

	SetRules::~SetRules()
	{
		if( m_bWasDeserialized && m_pRules)
		{
			AkFree( s_poolID, m_pRules );
		}
	}

	bool SetRules::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_NumRules );

		for( AkUInt32 i = 0; i < m_NumRules && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pRules[i] );

		return bRet;
	}

	bool SetRules::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_NumRules );

		if(m_NumRules != 0)
		{
			m_pRules = (AkWwiseMusicTransitionRule*)AkAlloc( s_poolID, m_NumRules * sizeof( AkWwiseMusicTransitionRule ) );

			if( m_pRules )
			{
				for( AkUInt32 i = 0; i < m_NumRules && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pRules[i] );
			}
			else
			{
				bRet = false;
			}

			m_bWasDeserialized = true;
		}

		return bRet;
	}
};

namespace MusicRanSeqProxyCommandData
{
	SetPlayList::SetPlayList()
		: CommandData( IMusicRanSeqProxy::MethodSetPlayList )
		, m_bWasDeserialized( false )
	{}

	SetPlayList::~SetPlayList()
	{
		if( m_bWasDeserialized && m_pArrayItems )
		{
			AkFree( s_poolID, m_pArrayItems );
		}
	}

	bool SetPlayList::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_NumItems );

		for( AkUInt32 i = 0; i < m_NumItems && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayItems[i] );

		return bRet;
	}

	bool SetPlayList::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_NumItems );

		if(m_NumItems != 0)
		{
			m_pArrayItems = (AkMusicRanSeqPlaylistItem*)AkAlloc( s_poolID, m_NumItems * sizeof( AkMusicRanSeqPlaylistItem ) );

			if( m_pArrayItems )
			{
				for( AkUInt32 i = 0; i < m_NumItems && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pArrayItems[i] );
			}
			else
			{
				bRet = false;
			}

			m_bWasDeserialized = true;
		}

		return bRet;
	}

};

namespace MusicSwitchProxyCommandData
{
	SetSwitchAssocs::SetSwitchAssocs()
		: CommandData( IMusicSwitchProxy::MethodSetSwitchAssocs )
		, m_bWasDeserialized( false )
	{}

	SetSwitchAssocs::~SetSwitchAssocs()
	{
		if( m_bWasDeserialized && m_pAssocs )
		{
			AkFree( s_poolID, m_pAssocs );
		}
	}

	bool SetSwitchAssocs::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_uNumAssocs );

		for( AkUInt32 i = 0; i < m_uNumAssocs && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pAssocs[i] );

		return bRet;
	}

	bool SetSwitchAssocs::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_uNumAssocs );

		if(m_uNumAssocs != 0)
		{
			m_pAssocs = (AkMusicSwitchAssoc*)AkAlloc( s_poolID, m_uNumAssocs * sizeof( AkMusicSwitchAssoc ) );

			if( m_pAssocs )
			{
				for( AkUInt32 i = 0; i < m_uNumAssocs && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pAssocs[i] );
			}
			else
			{
				bRet = false;
			}

			m_bWasDeserialized = true;
		}

		return bRet;
	}

	SetSwitchGroup::SetSwitchGroup()
		: CommandData( IMusicSwitchProxy::MethodSetSwitchGroup )
	{}

	bool SetSwitchGroup::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulGroup )
			&& in_rSerializer.PutEnum( m_eGroupType );
	}

	bool SetSwitchGroup::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulGroup )
			&& in_rSerializer.GetEnum( m_eGroupType );
	}

	SetDefaultSwitch::SetDefaultSwitch()
		: CommandData( IMusicSwitchProxy::MethodSetDefaultSwitch )
	{}

	bool SetDefaultSwitch::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_Switch );
	}

	bool SetDefaultSwitch::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_Switch );
	}

	ContinuePlayback::ContinuePlayback()
		: CommandData( IMusicSwitchProxy::MethodContinuePlayback )
	{}

	bool ContinuePlayback::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bContinuePlayback );
	}

	bool ContinuePlayback::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bContinuePlayback );
	}

};

namespace MusicSegmentProxyCommandData
{
	Duration::Duration()
		: CommandData( ISegmentProxy::MethodDuration )
	{}

	bool Duration::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fDuration );
	}

	bool Duration::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fDuration );
	}

	StartPos::StartPos()
		: CommandData( ISegmentProxy::MethodStartPos )
	{}

	bool StartPos::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_fStartPos );
	}

	bool StartPos::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_fStartPos );
	}

	SetMarkers::SetMarkers()
		: CommandData( ISegmentProxy::MethodSetMarkers )
		, m_bWasDeserialized( false )
	{}

	SetMarkers::~SetMarkers()
	{
		if( m_bWasDeserialized && m_pArrayMarkers )
		{
			AkFree( s_poolID, m_pArrayMarkers );
		}
	}

	bool SetMarkers::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		bool bRet = __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_ulNumMarkers );

		for( AkUInt32 i = 0; i < m_ulNumMarkers && bRet; ++i )
            bRet = bRet && in_rSerializer.Put( m_pArrayMarkers[i] );

		return bRet;
	}

	bool SetMarkers::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		bool bRet = __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_ulNumMarkers );

		if(m_ulNumMarkers != 0)
		{
			m_pArrayMarkers = (AkMusicMarkerWwise*)AkAlloc( s_poolID, m_ulNumMarkers * sizeof( AkMusicMarkerWwise ) );

			if( m_pArrayMarkers )
			{
				for( AkUInt32 i = 0; i < m_ulNumMarkers && bRet; ++i )
					bRet = bRet && in_rSerializer.Get( m_pArrayMarkers[i] );
			}
			else
			{
				bRet = false;
			}

			m_bWasDeserialized = true;
		}

		return bRet;
	}

	RemoveMarkers::RemoveMarkers()
		: CommandData( ISegmentProxy::MethodRemoveMarkers )
	{}

	bool RemoveMarkers::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer );
	}

	bool RemoveMarkers::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer );
	}
};

namespace FeedbackBusCommandData
{
}

namespace FeedbackNodeProxyCommandData
{
	AddPluginSource::AddPluginSource()
		: ObjectProxyCommandData::CommandData( IFeedbackNodeProxy::MethodAddPluginSource )
	{
	}

	bool AddPluginSource::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( in_srcID )
			&& in_rSerializer.Put( in_ulID )
			&& in_rSerializer.Put( in_pParams, in_uSize )
			&& in_rSerializer.Put( in_idDeviceCompany )
			&& in_rSerializer.Put( in_idDevicePlugin );
	}

	bool AddPluginSource::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( in_srcID )
			&& in_rSerializer.Get( in_ulID )
			&& in_rSerializer.Get( in_pParams, (AkInt32&)in_uSize )
			&& in_rSerializer.Get( in_idDeviceCompany )
			&& in_rSerializer.Get( in_idDevicePlugin );
	}

	SetSrcParam::SetSrcParam()
		: CommandData( IFeedbackNodeProxy::MethodSetSrcParam )
	{}

	bool SetSrcParam::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_pvData, m_ulSize )
			&& in_rSerializer.Put( m_fxID )
			&& in_rSerializer.Put( m_idSource )
			&& in_rSerializer.Put( m_paramID );
	}

	bool SetSrcParam::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_pvData, (AkInt32&)m_ulSize )
			&& in_rSerializer.Get( m_fxID )
			&& in_rSerializer.Get( m_idSource )
			&& in_rSerializer.Get( m_paramID );
	}

	IsZeroLatency::IsZeroLatency()
		: ObjectProxyCommandData::CommandData( IFeedbackNodeProxy::MethodIsZeroLatency )
	{
	}

	bool IsZeroLatency::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( in_bIsZeroLatency );
	}

	bool IsZeroLatency::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( in_bIsZeroLatency );
	}

	LookAheadTime::LookAheadTime()
		: ObjectProxyCommandData::CommandData( IFeedbackNodeProxy::MethodLookAheadTime )
	{
	}

	bool LookAheadTime::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( in_LookAheadTime );
	}

	bool LookAheadTime::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( in_LookAheadTime );
	}

	Loop::Loop()
		: CommandData( IFeedbackNodeProxy::MethodLoop )
	{}

	bool Loop::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( m_bIsLoopEnabled )
			&& in_rSerializer.Put( m_bIsLoopInfinite )
			&& in_rSerializer.Put( m_loopCount )
			&& in_rSerializer.Put( m_countModMin )
			&& in_rSerializer.Put( m_countModMax );
	}

	bool Loop::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( m_bIsLoopEnabled )
			&& in_rSerializer.Get( m_bIsLoopInfinite )
			&& in_rSerializer.Get( m_loopCount )
			&& in_rSerializer.Get( m_countModMin )
			&& in_rSerializer.Get( m_countModMax );
	}

	SetSourceVolumeOffset::SetSourceVolumeOffset()
		: CommandData( IFeedbackNodeProxy::MethodSetSourceVolumeOffset )
	{}

	bool SetSourceVolumeOffset::Serialize( CommandDataSerializer& in_rSerializer ) const
	{
		return __base::Serialize( in_rSerializer )
			&& in_rSerializer.Put( in_fOffset )
			&& in_rSerializer.Put( in_srcID );
	}

	bool SetSourceVolumeOffset::Deserialize( CommandDataSerializer& in_rSerializer )
	{
		return __base::Deserialize( in_rSerializer )
			&& in_rSerializer.Get( in_fOffset )
			&& in_rSerializer.Get( in_srcID );	
	}

};
