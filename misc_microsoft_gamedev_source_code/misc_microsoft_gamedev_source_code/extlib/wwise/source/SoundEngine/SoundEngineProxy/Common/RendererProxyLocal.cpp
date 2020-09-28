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

#include "RendererProxyLocal.h"

#include "AkAudioLib.h"
#include "AkAudioLibIndex.h"
#include "AkEnvironmentsMgr.h"
#include "AkRTPCMgr.h"
#include "AkStateMgr.h"
#include "AkCritical.h"
#include "AkMonitor.h"

#ifdef WIN32
#include "AkWinAudioLib.h"
#elif defined XBOX360
#include <AK/SoundEngine/Platforms/XBox360/AkXBox360SoundEngine.h>
#endif


RendererProxyLocal::RendererProxyLocal()
{
}

RendererProxyLocal::~RendererProxyLocal()
{
}

AkPlayingID RendererProxyLocal::PostEvent( AkUniqueID in_eventID, AkGameObjectID in_gameObjectPtr, unsigned int in_cookie ) const
{
	AkCustomParamType customParam = { 0 };
	customParam.customParam = in_cookie;
	customParam.ui32Reserved = in_cookie ? AK_EVENTWITHCOOKIE_RESERVED_BIT : 0;
	customParam.ui32Reserved |= AK_EVENTFROMWWISE_RESERVED_BIT;
	AKASSERT( AK::SoundEngine::IsInitialized() );
	return AK::SoundEngine::PostEvent( in_eventID, in_gameObjectPtr, 0, NULL, NULL, &customParam );
}

AKRESULT RendererProxyLocal::RegisterGameObj( AkGameObjectID in_GameObj, const char* in_pszObjectName )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::RegisterGameObj( in_GameObj, in_pszObjectName );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::UnregisterGameObj( AkGameObjectID in_GameObj )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::UnregisterGameObj( in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetActiveListeners( AkGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetActiveListeners( in_GameObjectID, in_uListenerMask );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetPosition( AkGameObjectID in_GameObj, const AkSoundPosition& in_rPosition, AkUInt32 in_ulListenerIndex /*= AK_INVALID_LISTENER_INDEX*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetPosition( in_GameObj, in_rPosition, in_ulListenerIndex );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex /*= 0*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetListenerPosition( in_rPosition, in_ulIndex );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets /*= NULL*/ )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetListenerSpatialization( in_uIndex, in_bSpatialized, in_pVolumeOffsets );
	}
	return AK_Fail;
}

void RendererProxyLocal::SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::SetRTPCValue( in_RTPCid, in_Value, in_GameObj );
	}
}

AKRESULT RendererProxyLocal::ResetRTPC( AkGameObjectID in_GameObjectPtr )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::ResetRTPC( in_GameObjectPtr );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetSwitch( in_SwitchGroup, in_SwitchState, in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetSwitches( AkGameObjectID in_GameObjectPtr )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::ResetSwitches( in_GameObjectPtr );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::PostTrigger( AkTriggerID in_Trigger, AkGameObjectID in_GameObj  )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::PostTrigger( in_Trigger, in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetAllStates()
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		return g_pStateMgr->ResetAllStates();
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetRndSeqCntrPlaylists()
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		return g_pIndex->ResetRndSeqCntrPlaylists();
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::AddFXParameterSet( AkUniqueID in_FXParameterSetID, AkPluginID in_EffectTypeID, void* in_pvInitParamsBlock, AkUInt32 in_ulParamBlockSize)
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::AddFXParameterSet( in_FXParameterSetID, in_EffectTypeID, in_pvInitParamsBlock, in_ulParamBlockSize );
	}
	return AK_Fail;
}
AKRESULT RendererProxyLocal::SetFXParameterSetParam( AkPluginID in_FXParameterSetID, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetFXParameterSetParam( in_FXParameterSetID, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
	}
	return AK_Fail;
}
AKRESULT RendererProxyLocal::RemoveFXParameterSet( AkUniqueID in_FXParameterSetID )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::RemoveFXParameterSet( in_FXParameterSetID );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetGameObjectEnvironmentsValues( AkGameObjectID in_gameObjectID, AkEnvironmentValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues)
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetGameObjectEnvironmentsValues( in_gameObjectID, in_aEnvironmentValues, in_uNumEnvValues );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetGameObjectDryLevelValue( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetGameObjectDryLevelValue( in_gameObjectID, in_fControlValue );
	}
	return AK_Fail;
}
	
AKRESULT RendererProxyLocal::SetEnvironmentVolume( AkEnvID in_FXParameterSetID, AkReal32 in_fVolume )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetEnvironmentVolume( in_FXParameterSetID, in_fVolume );
	}
	return AK_Fail;
}
	
AKRESULT RendererProxyLocal::BypassEnvironment( AkEnvID in_FXParameterSetID, bool in_bIsBypassed )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::BypassEnvironment( in_FXParameterSetID, in_bIsBypassed );
	}
	return AK_Fail;
}
	
AKRESULT RendererProxyLocal::SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetObjectObstructionAndOcclusion( in_ObjectID, in_uListener, in_fObstructionLevel, in_fOcclusionLevel );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetEnvRTPC( AkEnvID in_FXParameterSetID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion, AkUInt32 in_ulConversionArraySize )
{
	if( g_pEnvironmentMgr )
	{
		return g_pEnvironmentMgr->SetRTPC( in_FXParameterSetID, in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::UnsetEnvRTPC( AkEnvID in_FXParameterSetID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	if( g_pEnvironmentMgr )
	{
		return g_pEnvironmentMgr->UnsetRTPC( in_FXParameterSetID, in_ParamID, in_RTPCCurveID );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint * in_apPoints, AkCurveScaling in_eScaling )
{
	if( g_pEnvironmentMgr )
	{
		return g_pEnvironmentMgr->SetObsOccCurve( (CAkEnvironmentsMgr::eCurveXType)in_curveXType, 
												  (CAkEnvironmentsMgr::eCurveYType)in_curveYType, 
												  in_uNumPoints, in_apPoints, in_eScaling );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled )
{
	if( g_pEnvironmentMgr )
	{
		g_pEnvironmentMgr->SetCurveEnabled( (CAkEnvironmentsMgr::eCurveXType)in_curveXType, 
										    (CAkEnvironmentsMgr::eCurveYType)in_curveYType, 
											in_bEnabled );
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize )
{
	if( g_pRTPCMgr )
	{
		return g_pRTPCMgr->AddSwitchRTPC( in_switchGroup, in_RTPC_ID, in_pArrayConversion, in_ulConversionArraySize );
	}
	return AK_Fail;

}

void RendererProxyLocal::RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup )
{
	if( g_pRTPCMgr )
	{
		return g_pRTPCMgr->RemoveSwitchRTPC( in_switchGroup );
	}
}

void RendererProxyLocal::SetVolumeThreshold( AkReal32 in_VolumeThreshold )
{
	AK::SoundEngine::SetVolumeThreshold( in_VolumeThreshold );
}

AKRESULT RendererProxyLocal::PostMsgMonitor( AkLpCtstr in_pszMessage )
{
	if( AkMonitor::Get() )
	{
		AkMonitor::Monitor_PostString( in_pszMessage, AK::Monitor::ErrorLevel_Message );
		return AK_Success;
	}
	return AK_Fail;
}

void RendererProxyLocal::StopAll( AkGameObjectID in_GameObjPtr )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::StopAll( in_GameObjPtr );
	}
}

void RendererProxyLocal::StopPlayingID( AkPlayingID in_playingID )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::StopPlayingID( in_playingID );
	}
}
