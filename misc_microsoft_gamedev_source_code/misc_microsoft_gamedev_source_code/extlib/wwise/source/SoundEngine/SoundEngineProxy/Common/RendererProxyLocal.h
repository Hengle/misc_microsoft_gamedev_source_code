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


#pragma once

#include "IRendererProxy.h"

class RendererProxyLocal : public AK::Comm::IRendererProxy
{
public:
	RendererProxyLocal();
	virtual ~RendererProxyLocal();

	// IRendererProxy members
	virtual AkPlayingID PostEvent( AkUniqueID in_eventID, AkGameObjectID in_gameObjectPtr, unsigned int in_cookie ) const;

	//About RTPC Manager
	virtual AKRESULT RegisterGameObj( AkGameObjectID in_GameObj, const char* in_pszObjectName = "" );
	virtual AKRESULT UnregisterGameObj( AkGameObjectID in_GameObj );
	virtual AKRESULT SetActiveListeners( AkGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask );
	virtual AKRESULT SetPosition( AkGameObjectID in_GameObj, const AkSoundPosition& in_rPosition, AkUInt32 in_ulListenerIndex = AK_INVALID_LISTENER_INDEX );
	virtual AKRESULT SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex = 0 );
	virtual AKRESULT SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL );
	virtual void SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT ResetRTPC( AkGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT ResetSwitches( AkGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT PostTrigger( AkTriggerID in_Trigger, AkGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );

	virtual AKRESULT ResetAllStates();
	virtual AKRESULT ResetRndSeqCntrPlaylists();

	virtual AKRESULT AddFXParameterSet( AkUniqueID in_FXParameterSetID, AkPluginID in_EffectTypeID, void* in_pvInitParamsBlock, AkUInt32 in_ulParamBlockSize);
	virtual AKRESULT SetFXParameterSetParam( AkPluginID in_FXParameterSetID, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize );
	virtual AKRESULT RemoveFXParameterSet( AkUniqueID in_FXParameterSetID );

	virtual AKRESULT SetGameObjectEnvironmentsValues( AkGameObjectID in_gameObjectID, AkEnvironmentValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues);
	virtual AKRESULT SetGameObjectDryLevelValue( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue );
	virtual AKRESULT SetEnvironmentVolume( AkEnvID in_FXParameterSetID, AkReal32 in_fVolume );
	virtual AKRESULT BypassEnvironment( AkEnvID in_FXParameterSetID, bool in_bIsBypassed );
	virtual AKRESULT SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel );

	virtual AKRESULT SetEnvRTPC( AkEnvID in_FXParameterSetID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion, AkUInt32 in_ulConversionArraySize );
	virtual AKRESULT UnsetEnvRTPC( AkEnvID in_FXParameterSetID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	virtual AKRESULT SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint * in_apPoints, AkCurveScaling in_eScaling );
	virtual AKRESULT SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled );

	virtual AKRESULT AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize );
	virtual void	 RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup );

	virtual void	 SetVolumeThreshold( AkReal32 in_VolumeThreshold );

	virtual AKRESULT PostMsgMonitor( AkLpCtstr in_pszMessage );

	virtual void StopAll( AkGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT );
	virtual void StopPlayingID( AkPlayingID in_playingID );
};

