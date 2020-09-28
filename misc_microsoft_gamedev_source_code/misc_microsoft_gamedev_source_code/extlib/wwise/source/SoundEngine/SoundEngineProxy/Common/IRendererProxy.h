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

#include "AkRTPC.h"

class IConsoleConnector;
struct AkInitSettings;

namespace AK
{
	namespace Comm
	{
		class IRendererProxy
		{
		public:
			virtual AkPlayingID PostEvent( AkUniqueID in_eventID, AkGameObjectID in_gameObjectPtr = 0, unsigned int in_cookie = 0 ) const = 0;

			//About RTPC Manager
			virtual AKRESULT RegisterGameObj( AkGameObjectID in_GameObj, const char* in_pszObjectName = "" ) = 0;
			virtual AKRESULT UnregisterGameObj( AkGameObjectID in_GameObj ) = 0;
			virtual AKRESULT SetActiveListeners( AkGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask ) = 0;
			virtual AKRESULT SetPosition( AkGameObjectID in_GameObj, const AkSoundPosition& in_rPosition, AkUInt32 in_ulListenerIndex = AK_INVALID_LISTENER_INDEX ) = 0;
			virtual AKRESULT SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex = 0 ) = 0;
			virtual AKRESULT SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL ) = 0;
			virtual void SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT ResetRTPC( AkGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT ResetSwitches( AkGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT PostTrigger( AkTriggerID in_Trigger, AkGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT ) = 0;

			virtual AKRESULT ResetAllStates() = 0;
			virtual AKRESULT ResetRndSeqCntrPlaylists() = 0;

			virtual AKRESULT AddFXParameterSet( AkUniqueID in_FXParameterSetID, AkPluginID in_EffectTypeID, void* in_pvInitParamsBlock, AkUInt32 in_ulParamBlockSize) = 0;
			virtual AKRESULT SetFXParameterSetParam( AkPluginID in_FXParameterSetID, AkPluginParamID in_ulParamID, void* in_pvParamsBlock, AkUInt32 in_ulParamBlockSize ) = 0;
			virtual AKRESULT RemoveFXParameterSet( AkUniqueID in_FXParameterSetID ) = 0;

			virtual AKRESULT SetGameObjectEnvironmentsValues( AkGameObjectID in_gameObjectID, AkEnvironmentValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues) = 0;
			virtual AKRESULT SetGameObjectDryLevelValue( AkGameObjectID in_gameObjectID, AkReal32 in_fControlValue ) = 0;
			virtual AKRESULT SetEnvironmentVolume( AkEnvID in_FXParameterSetID, AkReal32 in_fVolume ) = 0;
			virtual AKRESULT BypassEnvironment( AkEnvID in_FXParameterSetID, bool in_bIsBypassed ) = 0;
			virtual AKRESULT SetObjectObstructionAndOcclusion( AkGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel ) = 0;

			virtual AKRESULT SetEnvRTPC( AkEnvID in_FXParameterSetID, AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion, AkUInt32 in_ulConversionArraySize ) = 0;
			virtual AKRESULT UnsetEnvRTPC( AkEnvID in_FXParameterSetID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID ) = 0;

			virtual AKRESULT SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint* in_apPoints, AkCurveScaling in_eScaling ) = 0;
			virtual AKRESULT SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled ) = 0;

			virtual AKRESULT AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize ) = 0;
			virtual void	 RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup ) = 0;

			virtual void	 SetVolumeThreshold( AkReal32 in_VolumeThreshold ) = 0;

			virtual AKRESULT PostMsgMonitor( AkLpCtstr in_pszMessage ) = 0;

			virtual void StopAll( AkGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual void StopPlayingID( AkPlayingID in_playingID ) = 0;
			
			enum MethodIDs
			{
				MethodPostEvent = 1,
				MethodRegisterGameObject,
				MethodUnregisterGameObject,
				MethodSetActiveListeners,
				MethodSetPosition,
				MethodSetListenerPosition,
				MethodSetListenerSpatialization,
				MethodSetRTPC,
				MethodResetRTPC,
				MethodSetSwitch,
				MethodResetSwitches,
				MethodPostTrigger,

				MethodResetAllStates,
				MethodResetRndSeqCntrPlaylists,

				MethodAddFXParameterSet,
				MethodSetFXParameterSetParam,
				MethodRemoveFXParameterSet,

				MethodSetGameObjectEnvironmentsValues,
				MethodSetGameObjectDryLevelValue,
				MethodSetEnvironmentVolume,
				MethodBypassEnvironment,
				MethodSetObjectObstructionAndOcclusion,

				MethodSetEnvRTPC,
				MethodUnsetEnvRTPC,

				MethodSetObsOccCurve,
				MethodSetObsOccCurveEnabled,
				
				MethodAddSwitchRTPC,
				MethodRemoveSwitchRTPC,

				MethodSetVolumeThreshold,

				MethodPostMsgMonitor,

				MethodStopAll,
				MethodStopPlayingID,

				LastMethodID
			};
		};
	};
};
