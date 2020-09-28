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
#include "AkPath.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include "AkLayerCntr.h"
#include "AkMusicStructs.h"
#include "AkAttenuationMgr.h"
#include "AkMonitorData.h"

#include "AkPrivateTypes.h"

class CommandDataSerializer;

namespace AkMonitorData
{
	struct Watch;
}

namespace ProxyCommandData
{
	enum CommandType
	{
		TypeRendererProxy = 1,
		TypeALMonitorProxy,
		TypeStateMgrProxy,
		TypeObjectProxyStore,
		TypeObjectProxy
	};

	struct CommandData
	{
		CommandData();
		CommandData( CommandType in_eCommandType, AkUInt16 in_methodID );

		bool BaseDeserialize( CommandDataSerializer& in_rSerializer );
		
		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_commandType;
		AkUInt16 m_methodID;

		static AkMemPoolId s_poolID;
	};
};

namespace RendererProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct PostEvent : public CommandData
	{
		PostEvent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_eventID;
		AkGameObjectID m_gameObjectPtr;
		AkUInt32 m_cookie;

		DECLARE_BASECLASS( CommandData );
	};

	struct RegisterGameObj : public CommandData
	{
		RegisterGameObj();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectPtr;
		char* m_pszObjectName;

		DECLARE_BASECLASS( CommandData );
	};

	struct UnregisterGameObj : public CommandData
	{
		UnregisterGameObj();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectPtr;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetActiveListeners : public CommandData
	{
		SetActiveListeners();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectID;
		AkUInt32 m_uListenerMask;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetPosition : public CommandData
	{
		SetPosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectPtr;
		AkSoundPosition m_position;
		AkUInt32 m_ulListenerIndex;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetListenerPosition : public CommandData
	{
		SetListenerPosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkListenerPosition m_position;
		AkUInt32 m_ulIndex;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetListenerSpatialization : public CommandData
	{
		SetListenerSpatialization();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uIndex;
		bool m_bSpatialized;
		bool m_bUseVolumeOffsets;		// Use to know if the AkSpeakerVolumes structure should be used.
		AkSpeakerVolumes m_volumeOffsets;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetRTPC : public CommandData
	{
		SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCid;
		AkReal32 m_value;
		AkGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};
	
	struct ResetRTPC : public CommandData
	{
		ResetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetSwitch : public CommandData
	{
		SetSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchGroupID m_switchGroup;
		AkSwitchStateID m_switchState;
		AkGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct PostTrigger : public CommandData
	{
		PostTrigger();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTriggerID m_trigger;
		AkGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetSwitches : public CommandData
	{
		ResetSwitches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObj;

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetAllStates : public CommandData
	{
		ResetAllStates();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct ResetRndSeqCntrPlaylists : public CommandData
	{
		ResetRndSeqCntrPlaylists();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct AddFXParameterSet : public CommandData
	{
		AddFXParameterSet();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		void*		m_pvInitParamsBlock; 
		AkUInt32		m_ulParamBlockSize;
		AkUniqueID	m_FXParameterSetID; 
		AkPluginID	m_EffectTypeID; 

		DECLARE_BASECLASS( CommandData );
	};

	struct SetFXParameterSetParam : public CommandData
	{
		SetFXParameterSetParam();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		void* m_pvParamsBlock;
		AkUInt32 m_ulParamBlockSize;
		AkPluginID m_FXParameterSetID;
		AkPluginParamID m_ulParamID;
		
		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveFXParameterSet : public CommandData
	{
		RemoveFXParameterSet();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_FXParameterSetID;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameObjectEnvironmentsValues : public CommandData
	{
		SetGameObjectEnvironmentsValues();
		~SetGameObjectEnvironmentsValues();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectID; 
		AkEnvironmentValue* m_aEnvironmentValues;
		AkUInt32 m_uNumEnvValues;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameObjectDryLevelValue : public CommandData
	{
		SetGameObjectDryLevelValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjectID;
		AkReal32 m_fControlValue;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetEnvironmentVolume : public CommandData
	{
		SetEnvironmentVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkEnvID m_FXParameterSetID;
		AkReal32 m_fVolume;

		DECLARE_BASECLASS( CommandData );
	};

	struct BypassEnvironment : public CommandData
	{
		BypassEnvironment();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkEnvID m_FXParameterSetID;
		bool m_bIsBypassed;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetObjectObstructionAndOcclusion : public CommandData
	{
		SetObjectObstructionAndOcclusion();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_ObjectID;
		AkUInt32 m_uListener;
		AkReal32 m_fObstructionLevel;
		AkReal32 m_fOcclusionLevel;

		DECLARE_BASECLASS( CommandData );
	};
	
	struct SetEnvRTPC :  public CommandData
	{
		SetEnvRTPC();
		~SetEnvRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkEnvID m_FXParameterSetID;
		AkRtpcID m_RTPC_ID; 
		AkRTPC_ParameterID m_ParamID;
		AkUniqueID m_RTPCCurveID;
		AkCurveScaling m_eScaling;
		AkRTPCGraphPoint* m_pArrayConversion;
		AkUInt32 m_ulConversionArraySize;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct UnsetEnvRTPC :  public CommandData
	{
		UnsetEnvRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkEnvID m_FXParameterSetID;
		AkRTPC_ParameterID m_ParamID;
		AkUniqueID m_RTPCCurveID;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetObsOccCurve :  public CommandData
	{
		SetObsOccCurve();
		~SetObsOccCurve();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt32 m_curveXType;
		AkInt32 m_curveYType;
		AkUInt32 m_uNumPoints;
		AkRTPCGraphPoint* m_apPoints;
		AkCurveScaling m_eScaling;

	private:
		bool m_bWasDeserialized;
	
		DECLARE_BASECLASS( CommandData );
	};

	struct SetObsOccCurveEnabled :  public CommandData
	{
		SetObsOccCurveEnabled();
		~SetObsOccCurveEnabled();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt32 m_curveXType;
		AkInt32 m_curveYType;
		bool m_bEnabled;
	
		DECLARE_BASECLASS( CommandData );
	};

	struct AddSwitchRTPC : public CommandData
	{
		AddSwitchRTPC();
		~AddSwitchRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uSwitchGroup;
		AkRtpcID m_RTPC_ID;
		AkUInt32 m_uArraySize;
		AkRTPCGraphPointInteger* m_pArrayConversion;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveSwitchRTPC :  public CommandData
	{
		RemoveSwitchRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uSwitchGroup;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetVolumeThreshold :  public CommandData
	{
		SetVolumeThreshold();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fVolumeThreshold;

		DECLARE_BASECLASS( CommandData );
	};

	struct PostMsgMonitor : public CommandData
	{
		PostMsgMonitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTChar* m_pszMessage;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopAll :  public CommandData
	{
		StopAll();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_GameObjPtr;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopPlayingID :  public CommandData
	{
		StopPlayingID();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPlayingID m_playingID;

		DECLARE_BASECLASS( CommandData );
	};
};

namespace ALMonitorProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct Monitor : public CommandData
	{
		Monitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkMonitorData::MonitorDataType m_uWhatToMonitor;

		DECLARE_BASECLASS( CommandData );
	};

	struct StopMonitor : public CommandData
	{
		StopMonitor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};

	struct SetWatches : public CommandData
	{
		SetWatches();
		~SetWatches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uiWatchCount;
		AkMonitorData::Watch* m_pWatches;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetGameSyncWatches : public CommandData
	{
		SetGameSyncWatches();
		~SetGameSyncWatches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uiWatchCount;
		AkUniqueID* m_pWatches;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( CommandData );
	};
};

namespace StateMgrProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct AddStateGroup : public CommandData
	{
		AddStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveStateGroup : public CommandData
	{
		RemoveStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct AddStateTransition : public CommandData
	{
		AddStateTransition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID1;
		AkStateID m_stateID2;
		AkTimeMs m_transitionTime;
		bool m_bIsShared;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveStateTransition : public CommandData
	{
		RemoveStateTransition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID1;
		AkStateID m_stateID2;
		bool m_bIsShared;

		DECLARE_BASECLASS( CommandData );
	};

	struct ClearStateTransitions : public CommandData
	{
		ClearStateTransitions();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetDefaultTransitionTime : public CommandData
	{
		SetDefaultTransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkTimeMs m_transitionTime;

		DECLARE_BASECLASS( CommandData );
	};

	struct AddState : public CommandData
	{
		AddState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID;
		AkUniqueID m_stateUniqueID;

		DECLARE_BASECLASS( CommandData );
	};

	struct RemoveState : public CommandData
	{
		RemoveState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID;

		DECLARE_BASECLASS( CommandData );
	};

	struct SetState : public CommandData
	{
		SetState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;
		AkStateID m_stateID;

		DECLARE_BASECLASS( CommandData );
	};

	struct GetState : public CommandData
	{
		GetState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( CommandData );
	};
};


namespace ObjectProxyStoreCommandData
{
	enum MethodIDs
	{
		MethodCreate = 1,
		MethodRelease
	};

	enum ObjectType
	{
		TypeSound = 1,
		TypeEvent,
		TypeAction,
		TypeState,
		TypeCustomState,
		TypeRanSeqContainer,
		TypeSwitchContainer,
		TypeActorMixer,
		TypeBus,
		TypeLayerContainer,
		TypeLayer,
		TypeMusicTrack,
		TypeMusicSegment,
		TypeMusicRanSeq,
		TypeMusicSwitch,
		TypeAttenuation,
		TypeFeedbackBus,
		TypeFeedbackNode,
	};

	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_proxyInstancePtr;
		AkUniqueID m_objectID;

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};

	struct Create : public CommandData
	{
		Create();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		ObjectType m_eObjectType;
		AkActionType m_actionType;	// Optional
		AkStateGroupID m_StateGroupID; // Optionnal

		DECLARE_BASECLASS( CommandData );
	};

	struct Release : public CommandData
	{
		Release();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( CommandData );
	};
};

namespace ObjectProxyCommandData
{
	struct CommandData : public ProxyCommandData::CommandData
	{
		CommandData();
		CommandData( AkUInt16 in_methodID );

		bool BaseDeserialize( CommandDataSerializer& in_rSerializer );

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_proxyInstancePtr;
		AkUniqueID m_objectID;

		DECLARE_BASECLASS( ProxyCommandData::CommandData );
	};
};

namespace AudioNodeProxyCommandData
{

};

namespace ParameterableProxyCommandData
{
	struct Priority : public ObjectProxyCommandData::CommandData
	{
		Priority();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPriority m_priority;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PriorityApplyDistFactor : public ObjectProxyCommandData::CommandData
	{
		PriorityApplyDistFactor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bApplyDistFactor;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PriorityDistanceOffset : public ObjectProxyCommandData::CommandData
	{
		PriorityDistanceOffset();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt16 m_iDistOffset;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PriorityOverrideParent : public ObjectProxyCommandData::CommandData
	{
		PriorityOverrideParent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bOverrideParent;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetStateGroup : public ObjectProxyCommandData::CommandData
	{
		SetStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_stateGroupID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct UnsetStateGroup : public ObjectProxyCommandData::CommandData
	{
		UnsetStateGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddState : public ObjectProxyCommandData::CommandData
	{
		AddState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_stateInstanceID;
		AkStateID m_stateID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveState : public ObjectProxyCommandData::CommandData
	{
		RemoveState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateID m_stateID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveAllStates : public ObjectProxyCommandData::CommandData
	{
		RemoveAllStates();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct UseState : public ObjectProxyCommandData::CommandData
	{
		UseState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bUseState;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LinkStateToStateDefault : public ObjectProxyCommandData::CommandData
	{
		LinkStateToStateDefault();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateID m_stateID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetStateSyncType : public ObjectProxyCommandData::CommandData
	{
		SetStateSyncType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_eSyncType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFX : public ObjectProxyCommandData::CommandData
	{
		SetFX();
		~SetFX();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvInitParamsBlock;
		AkUInt32 m_ulParamBlockSize;
		AkPluginID m_FXID;
		AkUInt32 m_uFXIndex;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFXParam : public ObjectProxyCommandData::CommandData
	{
		SetFXParam();
		~SetFXParam();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvInitParamsBlock;
		AkUInt32 m_ulParamBlockSize;
		AkPluginID m_FXID;
		AkUInt32 m_uFXIndex;
		AkPluginParamID m_paramID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct BypassAllFX : public ObjectProxyCommandData::CommandData
	{
		BypassAllFX();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bBypass;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct BypassFX : public ObjectProxyCommandData::CommandData
	{
		BypassFX();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uFXIndex;
		bool m_bBypass;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveFX : public ObjectProxyCommandData::CommandData
	{
		RemoveFX();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uFXIndex;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetRTPC : public ObjectProxyCommandData::CommandData
	{
		SetRTPC();
		~SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPluginID m_FXID;
		AkRtpcID m_RTPCID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;
		AkCurveScaling m_eScaling;
		AkRTPCGraphPoint* m_pArrayConversion;
		AkUInt32 m_ulConversionArraySize;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct UnsetRTPC : public ObjectProxyCommandData::CommandData
	{
		UnsetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPluginID m_FXID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ParameterNodeProxyCommandData
{
	struct Volume : public ObjectProxyCommandData::CommandData
	{
		Volume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_volume;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Pitch : public ObjectProxyCommandData::CommandData
	{
		Pitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPitchValue m_pitchType;
		AkPitchValue m_rangeMin;
		AkPitchValue m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LFEVolume : public ObjectProxyCommandData::CommandData
	{
		LFEVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_LFEVolume;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LPF : public ObjectProxyCommandData::CommandData
	{
		LPF();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkLPFType m_LPF;
		AkLPFType m_rangeMin;
		AkLPFType m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPositioningType : public ObjectProxyCommandData::CommandData
	{
		PosSetPositioningType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPositioningType m_ePosType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetSpatializationEnabled : public ObjectProxyCommandData::CommandData
	{
		PosSetSpatializationEnabled();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsSpatializationEnabled;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetAttenuationID : public ObjectProxyCommandData::CommandData
	{
		PosSetAttenuationID();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_uAttenuationID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};



	struct PosSetCenterPct : public ObjectProxyCommandData::CommandData
	{
		PosSetCenterPct();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt32 m_iCenterPct;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPAN_RL : public ObjectProxyCommandData::CommandData
	{
		PosSetPAN_RL();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fPanRL;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPAN_FR : public ObjectProxyCommandData::CommandData
	{
		PosSetPAN_FR();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fPanFR;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPannerEnabled : public ObjectProxyCommandData::CommandData
	{
		PosSetPannerEnabled();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsPannerEnabled;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetIsPositionDynamic : public ObjectProxyCommandData::CommandData
	{
		PosSetIsPositionDynamic();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsDynamic;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPathMode : public ObjectProxyCommandData::CommandData
	{
		PosSetPathMode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPathMode m_ePathMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetIsLooping : public ObjectProxyCommandData::CommandData
	{
		PosSetIsLooping();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsLooping;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetTransition : public ObjectProxyCommandData::CommandData
	{
		PosSetTransition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_transitionTime;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosSetPath : public ObjectProxyCommandData::CommandData
	{
		PosSetPath();
		~PosSetPath();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPathVertex* m_pArrayVertex;
		AkUInt32 m_ulNumVertices;
		AkPathListItemOffset* m_pArrayPlaylist;
		AkUInt32 m_ulNumPlaylistItem;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PosUpdatePathPoint : public ObjectProxyCommandData::CommandData
	{
		PosUpdatePathPoint();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_ulPathIndex;
		AkUInt32 m_ulVertexIndex;
		AkVector m_ptPosition;
		AkTimeMs m_delayToNext;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct OverrideFXParent : public ObjectProxyCommandData::CommandData
	{
		OverrideFXParent();
		~OverrideFXParent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		bool m_bIsFXOverrideParent;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetBelowThresholdBehavior : public ObjectProxyCommandData::CommandData
	{
		SetBelowThresholdBehavior();
		~SetBelowThresholdBehavior();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		AkBelowThresholdBehavior m_eBelowThresholdBehavior;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxNumInstancesOverrideParent : public ObjectProxyCommandData::CommandData
	{
		SetMaxNumInstancesOverrideParent();
		~SetMaxNumInstancesOverrideParent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		bool m_bOverride;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetVVoicesOptOverrideParent : public ObjectProxyCommandData::CommandData
	{
		SetVVoicesOptOverrideParent();
		~SetVVoicesOptOverrideParent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		bool m_bOverride;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxNumInstances : public ObjectProxyCommandData::CommandData
	{
		SetMaxNumInstances();
		~SetMaxNumInstances();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		AkUInt16 m_u16MaxNumInstance;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxReachedBehavior : public ObjectProxyCommandData::CommandData
	{
		SetMaxReachedBehavior();
		~SetMaxReachedBehavior();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		bool m_bKillNewest;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetVirtualQueueBehavior : public ObjectProxyCommandData::CommandData
	{
		SetVirtualQueueBehavior();
		~SetVirtualQueueBehavior();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		AkVirtualQueueBehavior m_eBehavior;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct FeedbackVolume : public ObjectProxyCommandData::CommandData
	{
		FeedbackVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_FeedbackVolume;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct FeedbackLPF : public ObjectProxyCommandData::CommandData
	{
		FeedbackLPF();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkLPFType m_FeedbackLPF;
		AkLPFType m_rangeMin;
		AkLPFType m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace BusProxyCommandData
{
	struct Volume : public ObjectProxyCommandData::CommandData
	{
		Volume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_volume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LFEVolume : public ObjectProxyCommandData::CommandData
	{
		LFEVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_LFEVolume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Pitch : public ObjectProxyCommandData::CommandData
	{
		Pitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPitchValue m_pitch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LPF : public ObjectProxyCommandData::CommandData
	{
		LPF();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkLPFType m_LPF;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxNumInstancesOverrideParent : public ObjectProxyCommandData::CommandData
	{
		SetMaxNumInstancesOverrideParent();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bOverride;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxNumInstances : public ObjectProxyCommandData::CommandData
	{
		SetMaxNumInstances();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_ulMaxNumInstance;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMaxReachedBehavior : public ObjectProxyCommandData::CommandData
	{
		SetMaxReachedBehavior();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bKillNewest;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddChild : public ObjectProxyCommandData::CommandData
	{
		AddChild();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_id;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveChild : public ObjectProxyCommandData::CommandData
	{
		RemoveChild();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_id;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveAllChildren : public ObjectProxyCommandData::CommandData
	{
		RemoveAllChildren();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetRecoveryTime : public ObjectProxyCommandData::CommandData
	{
		SetRecoveryTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_recoveryTime;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddDuck : public ObjectProxyCommandData::CommandData
	{
		AddDuck();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_busID;
		AkVolumeValue m_duckVolume;
		AkTimeMs m_fadeOutTime;
		AkTimeMs m_fadeInTime;
		AkCurveInterpolation m_eFadeCurve;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveDuck : public ObjectProxyCommandData::CommandData
	{
		RemoveDuck();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_busID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveAllDuck : public ObjectProxyCommandData::CommandData
	{
		RemoveAllDuck();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetAsBackgroundMusic : public ObjectProxyCommandData::CommandData
	{
		SetAsBackgroundMusic();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct UnsetAsBackgroundMusic : public ObjectProxyCommandData::CommandData
	{
		UnsetAsBackgroundMusic();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct EnableWiiCompressor : public ObjectProxyCommandData::CommandData
	{
		EnableWiiCompressor();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bEnable;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct FeedbackVolume : public ObjectProxyCommandData::CommandData
	{
		FeedbackVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_FeedbackVolume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct FeedbackLPF : public ObjectProxyCommandData::CommandData
	{
		FeedbackLPF();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkLPFType m_FeedbackLPF;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

};

namespace SoundProxyCommandData
{
	struct SetSource : public ObjectProxyCommandData::CommandData
	{
		SetSource();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		char* m_pszSourceName;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSource_Plugin : public ObjectProxyCommandData::CommandData
	{
		SetSource_Plugin();
		~SetSource_Plugin();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvData;
		AkUInt32 m_ulSize;
		AkPluginID m_fxID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSrcParam : public ObjectProxyCommandData::CommandData
	{
		SetSrcParam();
		~SetSrcParam();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvData;
		AkUInt32 m_ulSize;
		AkPluginID m_fxID;
		AkPluginParamID m_paramID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Loop : public ObjectProxyCommandData::CommandData
	{
		Loop();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsLoopEnabled;
		bool m_bIsLoopInfinite;
		AkInt16 m_loopCount;
		AkInt16 m_countModMin;
		AkInt16 m_countModMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct IsZeroLatency : public ObjectProxyCommandData::CommandData
	{
		IsZeroLatency();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsZeroLatency;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace TrackProxyCommandData
{
	struct SetPlayList : public ObjectProxyCommandData::CommandData
	{
		SetPlayList();
		~SetPlayList();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32		m_uNumPlaylistItem;
		AkTrackSrcInfo* m_pArrayPlaylistItems;
		AkUInt32		m_uNumSubTrack;


	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSrcParam : public ObjectProxyCommandData::CommandData
	{
		SetSrcParam();
		~SetSrcParam();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvData;
		AkUInt32 m_ulSize;
		AkUInt32 m_sourceID;
		AkPluginID m_fxID;
		AkPluginParamID m_paramID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Loop : public ObjectProxyCommandData::CommandData
	{
		Loop();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsLoopEnabled;
		bool m_bIsLoopInfinite;
		AkInt16 m_loopCount;
		AkInt16 m_countModMin;
		AkInt16 m_countModMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct IsStreaming : public ObjectProxyCommandData::CommandData
	{
		IsStreaming();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsStreaming;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct IsZeroLatency : public ObjectProxyCommandData::CommandData
	{
		IsZeroLatency();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsZeroLatency;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LookAheadTime : public ObjectProxyCommandData::CommandData
	{
		LookAheadTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_LookAheadTime;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMusicTrackRanSeqType : public ObjectProxyCommandData::CommandData
	{
		SetMusicTrackRanSeqType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkMusicTrackRanSeqType m_eRSType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

};

namespace EventProxyCommandData
{
	struct Add : public ObjectProxyCommandData::CommandData
	{
		Add();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_actionID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Remove : public ObjectProxyCommandData::CommandData
	{
		Remove();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_actionID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Clear : public ObjectProxyCommandData::CommandData
	{
		Clear();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionProxyCommandData
{
	struct SetElementID : public ObjectProxyCommandData::CommandData
	{
		SetElementID();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_elementID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetActionType : public ObjectProxyCommandData::CommandData
	{
		SetActionType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkActionType m_actionType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Delay : public ObjectProxyCommandData::CommandData
	{
		Delay();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_delay;
		AkTimeMs m_rangeMin;
		AkTimeMs m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionPlayProxyCommandData
{
	struct TransitionTime : public ObjectProxyCommandData::CommandData
	{
		TransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_transitionTime;
		AkTimeMs m_rangeMin;
		AkTimeMs m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct CurveType : public ObjectProxyCommandData::CommandData
	{
		CurveType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkCurveInterpolation m_eCurveType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionExceptProxyCommandData
{
	struct AddException : public ObjectProxyCommandData::CommandData
	{
		AddException();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_elementID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveException : public ObjectProxyCommandData::CommandData
	{
		RemoveException();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_elementID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ClearExceptions : public ObjectProxyCommandData::CommandData
	{
		ClearExceptions();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionActiveProxyCommandData
{
	struct TransitionTime : public ObjectProxyCommandData::CommandData
	{
		TransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_transitionTime;
		AkTimeMs m_rangeMin;
		AkTimeMs m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct CurveType : public ObjectProxyCommandData::CommandData
	{
		CurveType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkCurveInterpolation m_eCurveType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionPauseProxyCommandData
{
	struct IncludePendingResume : public ObjectProxyCommandData::CommandData
	{
		IncludePendingResume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIncludePendingResume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionResumeProxyCommandData
{
	struct IsMasterResume : public ObjectProxyCommandData::CommandData
	{
		IsMasterResume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsMasterResume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetValueProxyCommandData
{
	struct TransitionTime : public ObjectProxyCommandData::CommandData
	{
		TransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_transitionTime;
		AkTimeMs m_rangeMin;
		AkTimeMs m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct CurveType : public ObjectProxyCommandData::CommandData
	{
		CurveType();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkCurveInterpolation m_eCurveType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetPitchProxyCommandData
{		
	struct SetValue : public ObjectProxyCommandData::CommandData
	{
		SetValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPitchValue m_pitchType;
		AkValueMeaning m_eValueMeaning;
		AkPitchValue m_rangeMin;
		AkPitchValue m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetVolumeProxyCommandData
{
	struct SetValue : public ObjectProxyCommandData::CommandData
	{
		SetValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_value;
		AkValueMeaning m_eValueMeaning;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetLFEProxyCommandData
{
	struct SetValue : public ObjectProxyCommandData::CommandData
	{
		SetValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_value;
		AkValueMeaning m_eValueMeaning;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetLPFProxyCommandData
{
	struct SetValue : public ObjectProxyCommandData::CommandData
	{
		SetValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_value;
		AkValueMeaning m_eValueMeaning;
		AkReal32 m_rangeMin;
		AkReal32 m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetStateProxyCommandData
{
	struct SetGroup : public ObjectProxyCommandData::CommandData
	{
		SetGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateGroupID m_groupID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetTargetState : public ObjectProxyCommandData::CommandData
	{
		SetTargetState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkStateID m_stateID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetSwitchProxyCommandData
{
	struct SetSwitchGroup : public ObjectProxyCommandData::CommandData
	{
		SetSwitchGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchGroupID m_ulSwitchGroupID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetTargetSwitch : public ObjectProxyCommandData::CommandData
	{
		SetTargetSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchStateID m_ulSwitchID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionSetRTPCProxyCommandData
{
	struct SetRTPCGroup : public ObjectProxyCommandData::CommandData
	{
		SetRTPCGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCGroupID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetRTPCValue : public ObjectProxyCommandData::CommandData
	{
		SetRTPCValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fRTPCValue;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionUseStateProxyCommandData
{
	struct UseState : public ObjectProxyCommandData::CommandData
	{
		UseState();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bUseState;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace ActionBypassFXProxyCommandData
{
	struct BypassFX : public ObjectProxyCommandData::CommandData
	{
		BypassFX();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bBypassFX;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetBypassTarget : public ObjectProxyCommandData::CommandData
	{
		SetBypassTarget();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bBypassAllFX;
		AkUInt8 m_ucEffectsMask;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace StateProxyCommandData
{
	struct Volume : public ObjectProxyCommandData::CommandData
	{
		Volume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_volume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct VolumeMeaning : public ObjectProxyCommandData::CommandData
	{
		VolumeMeaning();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkValueMeaning m_eMeaning;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Pitch : public ObjectProxyCommandData::CommandData
	{
		Pitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkPitchValue m_pitchType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct PitchMeaning : public ObjectProxyCommandData::CommandData
	{
		PitchMeaning();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkValueMeaning m_eMeaning;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LPF : public ObjectProxyCommandData::CommandData
	{
		LPF();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkLPFType m_LPF;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LPFMeaning : public ObjectProxyCommandData::CommandData
	{
		LPFMeaning();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkValueMeaning m_eMeaning;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LFEVolume : public ObjectProxyCommandData::CommandData
	{
		LFEVolume();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_LFEVolume;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LFEVolumeMeaning : public ObjectProxyCommandData::CommandData
	{
		LFEVolumeMeaning();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkValueMeaning m_eMeaning;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace AttenuationProxyCommandData
{
	struct SetAttenuationParams : public ObjectProxyCommandData::CommandData
	{
		SetAttenuationParams();
		~SetAttenuationParams();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkWwiseAttenuation m_Params;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace HierarchicalProxyCommandData
{
	struct AddChild : public ObjectProxyCommandData::CommandData
	{
		AddChild();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_id;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveChild : public ObjectProxyCommandData::CommandData
	{
		RemoveChild();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_id;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveAllChildren : public ObjectProxyCommandData::CommandData
	{
		RemoveAllChildren();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace RanSeqContainerProxyCommandData
{
	struct Mode : public ObjectProxyCommandData::CommandData
	{
		Mode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkContainerMode m_eMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct IsGlobal : public ObjectProxyCommandData::CommandData
	{
		IsGlobal();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsGlobal;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddPlaylistItem : public ObjectProxyCommandData::CommandData
	{
		AddPlaylistItem();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_elementID;
		AkUInt8 m_weight;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemovePlaylistItem : public ObjectProxyCommandData::CommandData
	{
		RemovePlaylistItem();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_elementID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ClearPlaylist : public ObjectProxyCommandData::CommandData
	{
		ClearPlaylist();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ResetPlayListAtEachPlay : public ObjectProxyCommandData::CommandData
	{
		ResetPlayListAtEachPlay();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bResetPlayListAtEachPlay;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RestartBackward : public ObjectProxyCommandData::CommandData
	{
		RestartBackward();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bRestartBackward;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Continuous : public ObjectProxyCommandData::CommandData
	{
		Continuous();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsContinuous;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ForceNextToPlay : public ObjectProxyCommandData::CommandData
	{
		ForceNextToPlay();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkInt16 m_position;
		AkGameObjectID m_gameObjPtr;
		AkPlayingID m_playingID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct NextToPlay : public ObjectProxyCommandData::CommandData
	{
		NextToPlay();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkGameObjectID m_gameObjPtr;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RandomMode : public ObjectProxyCommandData::CommandData
	{
		RandomMode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRandomMode m_eRandomMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AvoidRepeatingCount : public ObjectProxyCommandData::CommandData
	{
		AvoidRepeatingCount();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_count;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetItemWeight_withID : public ObjectProxyCommandData::CommandData
	{
		SetItemWeight_withID();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_itemID;
		AkUInt8 m_weight;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetItemWeight_withPosition : public ObjectProxyCommandData::CommandData
	{
		SetItemWeight_withPosition();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt16 m_position;
		AkUInt8 m_weight;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Loop : public ObjectProxyCommandData::CommandData
	{
		Loop();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsLoopEnabled;
		bool m_bIsLoopInfinite;
		AkInt16 m_loopCount;
		AkInt16 m_countModMin;
		AkInt16 m_countModMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct TransitionMode : public ObjectProxyCommandData::CommandData
	{
		TransitionMode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTransitionMode m_eTransitionMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct TransitionTime : public ObjectProxyCommandData::CommandData
	{
		TransitionTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs m_transitionTime;
		AkTimeMs m_rangeMin;
		AkTimeMs m_rangeMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace SwitchContainerProxyCommandData
{
	struct SetSwitchGroup : public ObjectProxyCommandData::CommandData
	{
		SetSwitchGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_ulGroup;
		AkGroupType m_eGroupType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetDefaultSwitch : public ObjectProxyCommandData::CommandData
	{
		SetDefaultSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ClearSwitches : public ObjectProxyCommandData::CommandData
	{
		ClearSwitches();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddSwitch : public ObjectProxyCommandData::CommandData
	{
		AddSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchStateID m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveSwitch : public ObjectProxyCommandData::CommandData
	{
		RemoveSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkSwitchStateID m_switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct AddNodeInSwitch : public ObjectProxyCommandData::CommandData
	{
		AddNodeInSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;
		AkUniqueID m_nodeID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveNodeFromSwitch : public ObjectProxyCommandData::CommandData
	{
		RemoveNodeFromSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_switch;
		AkUniqueID m_nodeID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetContinuousValidation : public ObjectProxyCommandData::CommandData
	{
		SetContinuousValidation();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsContinuousCheck;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetContinuePlayback : public ObjectProxyCommandData::CommandData
	{
		SetContinuePlayback();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		bool m_bContinuePlayback;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFadeInTime : public ObjectProxyCommandData::CommandData
	{
		SetFadeInTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkTimeMs m_time;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetFadeOutTime : public ObjectProxyCommandData::CommandData
	{
		SetFadeOutTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkTimeMs m_time;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetOnSwitchMode : public ObjectProxyCommandData::CommandData
	{
		SetOnSwitchMode();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		AkOnSwitchMode m_bSwitchMode;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetIsFirstOnly : public ObjectProxyCommandData::CommandData
	{
		SetIsFirstOnly();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_nodeID;
		bool m_bIsFirstOnly;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace LayerContainerProxyCommandData
{
	//
	// AddLayer
	//

	struct AddLayer : public ObjectProxyCommandData::CommandData
	{
		AddLayer();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_LayerID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// RemoveLayer
	//

	struct RemoveLayer : public ObjectProxyCommandData::CommandData
	{
		RemoveLayer();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_LayerID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace LayerProxyCommandData
{
	//
	// SetRTPC
	//

	struct SetRTPC : public ObjectProxyCommandData::CommandData
	{
		SetRTPC();
		~SetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_RTPCID;
		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;
		AkCurveScaling m_eScaling;
		AkRTPCGraphPoint* m_pArrayConversion;
		AkUInt32 m_ulConversionArraySize;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// UnsetRTPC
	//

	struct UnsetRTPC : public ObjectProxyCommandData::CommandData
	{
		UnsetRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRTPC_ParameterID m_paramID;
		AkUniqueID m_RTPCCurveID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// SetChildAssoc
	//

	struct SetChildAssoc : public ObjectProxyCommandData::CommandData
	{
		SetChildAssoc();
		~SetChildAssoc();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_ChildID;
		AkRTPCCrossfadingPoint* m_pCrossfadingCurve;
		AkUInt32 m_ulCrossfadingCurveSize;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// UnsetChildAssoc
	//

	struct UnsetChildAssoc : public ObjectProxyCommandData::CommandData
	{
		UnsetChildAssoc();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUniqueID m_ChildID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// SetCrossfadingRTPC
	//

	struct SetCrossfadingRTPC : public ObjectProxyCommandData::CommandData
	{
		SetCrossfadingRTPC();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkRtpcID m_rtpcID;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	//
	// SetCrossfadingRTPCDefaultValue
	//

	struct SetCrossfadingRTPCDefaultValue : public ObjectProxyCommandData::CommandData
	{
		SetCrossfadingRTPCDefaultValue();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 m_fValue;

	private:
		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace MusicNodeProxyCommandData
{
	struct MeterInfo : public ObjectProxyCommandData::CommandData
	{
		MeterInfo();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsOverrideParent;
		AkMeterInfo m_MeterInfo;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetStingers : public ObjectProxyCommandData::CommandData
	{
		SetStingers();
		~SetStingers();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		CAkStinger*	m_pStingers;
		AkUInt32	m_NumStingers;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace MusicTransAwareProxyCommandData
{
	struct SetRules : public ObjectProxyCommandData::CommandData
	{
		SetRules();
		~SetRules();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_NumRules;
		AkWwiseMusicTransitionRule* m_pRules;
	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace MusicRanSeqProxyCommandData
{
	struct SetPlayList : public ObjectProxyCommandData::CommandData
	{
		SetPlayList();
		~SetPlayList();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_NumItems;
		AkMusicRanSeqPlaylistItem* m_pArrayItems;
	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace MusicSwitchProxyCommandData
{
	struct SetSwitchAssocs : public ObjectProxyCommandData::CommandData
	{
		SetSwitchAssocs();
		~SetSwitchAssocs();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_uNumAssocs;
		AkMusicSwitchAssoc* m_pAssocs;
	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSwitchGroup : public ObjectProxyCommandData::CommandData
	{
		SetSwitchGroup();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_ulGroup;
        AkGroupType m_eGroupType;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetDefaultSwitch : public ObjectProxyCommandData::CommandData
	{
		SetDefaultSwitch();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkUInt32 m_Switch;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct ContinuePlayback : public ObjectProxyCommandData::CommandData
	{
		ContinuePlayback();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bContinuePlayback;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace MusicSegmentProxyCommandData
{
	struct Duration : public ObjectProxyCommandData::CommandData
	{
		Duration();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal64 m_fDuration;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct StartPos : public ObjectProxyCommandData::CommandData
	{
		StartPos();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal64 m_fStartPos;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetMarkers : public ObjectProxyCommandData::CommandData
	{
		SetMarkers();
		~SetMarkers();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkMusicMarkerWwise*     m_pArrayMarkers;
		AkUInt32                m_ulNumMarkers;

	private:
		bool m_bWasDeserialized;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct RemoveMarkers : public ObjectProxyCommandData::CommandData
	{
		RemoveMarkers();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};

namespace FeedbackBusCommandData
{
};

namespace FeedbackNodeProxyCommandData
{
	struct AddPluginSource : public ObjectProxyCommandData::CommandData
	{
		AddPluginSource();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		void *	in_pParams;
		AkUniqueID in_srcID;
		AkPluginID in_ulID;
		AkUInt32 in_uSize;
		AkUInt16 in_idDeviceCompany;
		AkUInt16 in_idDevicePlugin;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSrcParam : public ObjectProxyCommandData::CommandData
	{
		SetSrcParam();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		// Avoid chance of mis-aligment.
		void* m_pvData;
		AkUInt32 m_ulSize;
		AkPluginID m_fxID;
		AkPluginParamID m_paramID;
		AkUniqueID m_idSource;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
	struct IsZeroLatency : public ObjectProxyCommandData::CommandData
	{
		IsZeroLatency();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool in_bIsZeroLatency;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct LookAheadTime : public ObjectProxyCommandData::CommandData
	{
		LookAheadTime();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkTimeMs in_LookAheadTime;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct Loop : public ObjectProxyCommandData::CommandData
	{
		Loop();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		bool m_bIsLoopEnabled;
		bool m_bIsLoopInfinite;
		AkInt16 m_loopCount;
		AkInt16 m_countModMin;
		AkInt16 m_countModMax;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};

	struct SetSourceVolumeOffset : public ObjectProxyCommandData::CommandData
	{
		SetSourceVolumeOffset();

		bool Serialize( CommandDataSerializer& in_rSerializer ) const;
		bool Deserialize( CommandDataSerializer& in_rSerializer );

		AkReal32 in_fOffset;
		AkUInt32 in_srcID;

		DECLARE_BASECLASS( ObjectProxyCommandData::CommandData );
	};
};
