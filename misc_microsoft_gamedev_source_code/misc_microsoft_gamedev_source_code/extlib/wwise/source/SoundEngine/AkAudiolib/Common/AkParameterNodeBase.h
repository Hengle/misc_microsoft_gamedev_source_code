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

//////////////////////////////////////////////////////////////////////
//
// AkParameterNodeBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _BASE_PARAMETER_NODE_H_
#define _BASE_PARAMETER_NODE_H_

#include "AkAudioNode.h"
#include "ITransitionable.h"
#include "AkParameters.h"
#include "AkState.h"
#include "AkCommon.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkBitArray.h"
#include "AkMutedMap.h"
#include "AkConversionTable.h"
#include "AkKeyArray.h"
#include "AkRTPCMgr.h"
#include "AkFeedbackStructs.h"
#include <math.h>
#include "AudiolibLimitations.h"

struct AkSoundParams;
class CAkSIS;

enum AkTransitionTarget
{
	TransitionTarget_Volume,
	TransitionTarget_Lfe,
	TransitionTarget_Pitch,
	TransitionTarget_LPF,
	TransitionTarget_Pan
};

struct AkRTPCFXSubscription
{
	AkPluginID			FXID;
	AkRtpcID			RTPCID;
	AkUInt32			ParamID;		//# of the param that must be notified on change
	AkUniqueID			RTPCCurveID;
	CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable;
};

typedef AkArray<AkRTPCFXSubscription, const AkRTPCFXSubscription&, ArrayPoolDefault, 2> AkRTPCFXSubscriptionList;

class CAkParameterNodeBase : public CAkAudioNode, public ITransitionable
{
protected:
	CAkParameterNodeBase(AkUniqueID in_ulID = 0);

	//Destructor
    virtual ~CAkParameterNodeBase();
private:
	void FlushStateTransitions();
public:

	//////////////////////////////////////////////////////////////////////
	// AkSyncType tools
	typedef AkArray<AkSyncType, const AkSyncType, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof( AkSyncType )> StateSyncArray;

	class CAkStateSyncArray
	{
	public:
		StateSyncArray&		GetStateSyncArray(){ return m_StateSyncArray; }

		void				Term(){ m_StateSyncArray.Term(); }
		void				RemoveAllSync() { m_StateSyncArray.RemoveAll(); }

	private:
		StateSyncArray		m_StateSyncArray;
	};
	//////////////////////////////////////////////////////////////////////

	AKRESULT Init(){ return CAkAudioNode::Init(); }

public:
	virtual AKRESULT GetAudioParameters(
		AkSoundParams &out_Parameters,			// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		) = 0;

	// pause (in_bPause = true) or resume (in_bPause = fasle) the transitions, if any
	virtual void PauseTransitions(bool in_bPause);

	void SetTransition(CAkTransition* in_pTransition, AkTransitionTarget in_eTransitionTarget);

	CAkTransition* GetTransition(AkTransitionTarget in_eTransitionTarget);

	bool SetTransVolume( AkVolumeValue in_Volume );
	bool SetTransPitch( AkPitchValue in_Volume );
	bool SetTransLfe( AkVolumeValue in_Lfe );
	bool SetTransLPF( AkLPFType in_LPF );

	bool DoesChangeMustBeNotified( ParamType in_ParamType );

	// Set the pitch
	virtual void SetPitch(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkPitchValue		in_TargetValue = 0,		//Pitch target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Reset the Pitch
	virtual void ResetPitch(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Set the volume
	virtual void SetVolume(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// Volume target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Reset the Volume
	virtual void ResetVolume(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Set the LFE
	virtual void SetLFE(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// LFE target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Reset the LFE
	virtual void ResetLFE(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Set the LPF
	virtual void SetLPF(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// LPF target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;


	// Reset the LPF
	virtual void ResetLPF(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Mute the element
	virtual void Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;
	
	// Unmute the element for the specified game object
	virtual void Unmute(
		CAkRegisteredObj *	in_pGameObj,					//Game object associated to the action
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// Un-Mute the element(per object and main)
	virtual void UnmuteAll(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		) = 0;

	// starts a volume transition on a given SIS
	void StartSisVolumeTransitions(CAkSIS*			pSIS,
									AkReal32			fTargetValue,
									AkValueMeaning	eValueMeaning,
									AkCurveInterpolation		eFadeCurve,
									AkTimeMs		lTransitionTime);

	// starts a LFE transition on a given SIS
	void StartSisLFETransitions(	CAkSIS*			pSIS,
									AkReal32			fTargetValue,
									AkValueMeaning	eValueMeaning,
									AkCurveInterpolation		eFadeCurve,
									AkTimeMs		lTransitionTime);

	// starts a LPF transition on a given SIS
	void StartSisLPFTransitions(	CAkSIS*			pSIS,
									AkLPFType		fTargetValue,
									AkValueMeaning	eValueMeaning,
									AkCurveInterpolation		eFadeCurve,
									AkTimeMs		lTransitionTime);

	// starts a pitch transition on a given SIS
	void StartSisPitchTransitions(CAkSIS*			pSIS,
									AkPitchValue		TargetValue,
									AkValueMeaning	eValueMeaning,
									AkCurveInterpolation		eFadeCurve,
									AkTimeMs		lTransitionTime);

	// starts a mute trasnition on a given SIS
	void StartSisMuteTransitions(CAkSIS*		in_pSIS,
								AkUInt8			in_cTargetValue,
								AkCurveInterpolation		in_eFadeCurve,
								AkTimeMs		in_lTransitionTime);

	// starts a feedback volume transition on a given SIS
	void StartSisFeedbackVolumeTransitions(CAkSIS*	in_pSIS,
										 AkReal32		in_fTargetValue,
										 AkValueMeaning	in_eValueMeaning,
										 AkCurveInterpolation	in_eFadeCurve,
										 AkTimeMs		in_lTransitionTime);

	virtual void RecalcNotification();

	virtual void TransUpdateValue(
		TransitionTargetTypes in_eTargetType,
		TransitionTarget in_unionValue,
		bool in_bIsTerminated
		);

///////////////////////////////////////////////////////////////////////////
//  STATES
///////////////////////////////////////////////////////////////////////////

	//Set the associated channel of this node
	//It replaces the existing one if it is already existing
	virtual void SetStateGroup(AkStateGroupID in_ulStateGroupID);

	//Removes the associated channel of this node
	//After thet the channel Do not respond to any state channel untill a new channel is set
	virtual void UnsetStateGroup();

	//Get the channel associated to this node
	//
	// Return - AkStateGroupID - ID of the channel
	virtual AkStateGroupID GetStateGroup();

	// Get the pointer to the actually selected state
	//
	// Return - CAkState* - Pointer to the actual state
	virtual CAkState* GetState();

	// Get the pointer to the specified state
	//
	// Return - CAkState* - Pointer to specified state
	virtual CAkState* GetState(AkStateID in_StateTypeID);

	// Get the ID of the actually selected state
	//
	// Return - AkStateID* - ID of the actual state
	virtual AkStateID ActualState();

	// Set the ID of the actually selected state
	virtual void SetActualState(
		AkStateID in_ulStateID//ID type to be set
		);

	// Return if the States modifications as to be calculated
	virtual bool UseState() const;

	// Sets the Use State flag
	virtual void UseState(
		bool in_bUseState // Is using state
		);

	// Set the given state to the default state properties contained in the AudioLib state manager
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT LinkStateToStateDefault(
		AkStateID in_ulStateID
		);

	// Add a state to the sound object
    //
    // Return - AKRESULT - AK_Success if everything succeeded
	virtual AKRESULT AddState(
		AkUniqueID in_ulStateInstanceID,//StateInstanceID to be added
		AkStateID in_ulStateID//StateID
		);

	// Get the pointer of a specified state
    //
    // Return - AkState* - Pointer to the state, 
	//						null if not found
	virtual CAkState* State(
		AkStateID in_ulStateID //SwitchState
		);

    // Remove all the States
    //
    // Return - AKRESULT - AK_Success
    virtual AKRESULT RemoveAllStates();

	// Remove the specified State
    //
    // Return - AKRESULT - AK_Success
	virtual AKRESULT RemoveState(
		AkStateID in_ulStateID //SwitchState
		);

	// This function is called on the ParameterNode by the state manager once all state transitions 
	// have been launched
	void NotifyStateModified();

	// This function is called on the ParameterNode from the state it is using to signify that the
	// Currently in use state setttings were modified and that existing PBIs have to be informed
	virtual void NotifyStateParametersModified();

	void SetMaxReachedBehavior( bool in_bKillNewest );
	void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance );
	void SetMaxNumInstOverrideParent( bool in_bOverride );
	void SetVVoicesOptOverrideParent( bool in_bOverride );

	AkPriority GetPriority( AkPriority& out_iDistOffset );

	void SetPriority( AkPriority in_ucPriority );
	void SetPriorityApplyDistFactor( bool in_bApplyDistFactor );
	void SetPriorityDistanceOffset( AkPriority in_iDistOffset );
	void SetPriorityOverrideParent( bool in_bOverrideParent );

	// Set or replace an FX in the Node.
	// The emplacement is important
	AKRESULT SetFX( 
		AkPluginID in_FXID,						// FX ID, associated to a local FX or a plug-in
		AkUInt32 in_uFXIndex,				// Position of the FX in the array
		void* in_pvInitParamsBlock = NULL,// Pointer to the Param block	
		AkUInt32 in_ulParamBlockSize = 0		// BLOB size
		);

	AKRESULT RemoveFX( 
		AkUInt32 in_uFXIndex					// Position of the FX in the array
		);

	AKRESULT SetFXParam( 
		AkPluginID      in_FXID,				// FX ID, associated to a local FX or a plug-in
		AkUInt32      	in_uFXIndex,			// Position of the FX in the array
		AkPluginParamID in_ulParamID,			// ID of the param to modify, will be done by the plug-in itself
		void*     	in_pvParamsBlock,		// Pointer to the Param block
		AkUInt32     	in_ulParamBlockSize		// BLOB size
		);

	AKRESULT RenderedFX(
		AkUInt32		in_uFXIndex,
		bool			in_bRendered
		);

	AKRESULT MainBypassFX(
		AkUInt32		in_bitsFXBypass,
		AkUInt32        in_uTargetMask = 0xFFFFFFFF
		);

	virtual void ResetFXBypass( 
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		) = 0;

	AKRESULT BypassFX(
		AkUInt32			in_bitsFXBypass,
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj = NULL,
		bool			in_bIsFromReset = false
		);

	AKRESULT ResetBypassFX(
		AkUInt32			in_uTargetMask,
		CAkRegisteredObj *	in_pGameObj = NULL
		);

	virtual AKRESULT GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj = NULL
		) = 0;

	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj ) = 0;

	AKRESULT SetParamComplexFromRTPCManager( 
		void * in_pToken,
		AkUInt32 in_Param_id, 
		AkRtpcID in_RTPCid,
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj = NULL,
		void* in_pGameObjExceptArray = NULL // Actually a GameObjExceptArray pointer
		);

	AKRESULT SetParamComplex( 
		AkUInt32 in_Param_id, 
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj = NULL,
		void* in_pGameObjExceptArray = NULL // Actually a GameObjExceptArray pointer
		);

	// Register a given parameter to an RTPC ID
	void SetRTPC(
		AkPluginID					in_FXID,		// If invalid, means that the RTPC is directly on sound parameters
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion = NULL,		// NULL if none
		AkUInt32						in_ulConversionArraySize = 0	// 0 if none
		);

	virtual AKRESULT UnsetRTPC(
		AkPluginID in_FXID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	virtual AKRESULT SetRTPCforFX(
		AkPluginID					in_FXID,		// If invalid, means that the RTPC is directly on sound parameters
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion = NULL,		// NULL if none
		AkUInt32						in_ulConversionArraySize = 0	// 0 if none
		);

	virtual AKRESULT UnsetRTPCforFX(
		AkPluginID in_FXID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	virtual AkRTPCFXSubscriptionList* GetFxRTPCSubscriptionList() = 0;

	AkSyncType GetStateSyncType(){ return (AkSyncType)m_eStateSyncType; }
	void SetStateSyncType( AkUInt32/*AkSyncType*/ in_eSyncType ){ m_eStateSyncType = (AkUInt8)in_eSyncType; }

	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes, bool in_bBusChecked = false );
	// return true means we are done checking
	bool CheckSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

	// Feedback support
	// Get the compounded feedback parameters.  There is currenly only the volume.
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetFeedbackParameters( 
		AkFeedbackParams &io_Params,			// Parameters
		CAkSource* in_pSource,
		CAkRegisteredObj * in_GameObjPtr,			// Game object associated to the query
		bool in_bDoBusCheck = true );

	// Set the feedback volume
	virtual void SetFeedbackVolume(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// Volume target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	virtual void FeedbackVolume( AkVolumeValue in_Volume, AkVolumeValue in_MinRangeValue = 0.0f, AkVolumeValue in_MaxRangeValue = 0.0f);
	AkVolumeValue FeedbackVolume();
	virtual void FeedbackParentBus(CAkFeedbackBus* in_pParent);
	virtual CAkFeedbackBus* FeedbackParentBus();
	CAkFeedbackBus* GetFeedbackParentBusOrDefault();
	virtual AkVolumeValue GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr );
	virtual AkLPFType GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr );

	virtual void FeedbackLPF( AkLPFType in_feedbackLPF, AkLPFType in_MinRangeValue = 0.0, AkLPFType in_MaxRangeValue = 0.0);

	virtual void IncrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );
	virtual void DecrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );

protected:

	virtual void NotifyRTPCChanged();

	virtual AKRESULT SetInitialParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize ) = 0;
	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly ) = 0;
	AKRESULT SetInitialRTPC(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	AKRESULT SetNodeBaseParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );
	virtual AKRESULT SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	virtual void ReadFeedbackInfo(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize);

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj ) = 0;

	virtual CAkRTPCMgr::SubscriberType GetRTPCSubscriberType() const;

private:
	bool EnableStateTransitionInfo();
//members
protected:

	struct AkStateTransitionInfo : public CAkObject
	{
		AkVolumeValue	m_Volume;
		AkVolumeValue	m_Lfe;
		AkPitchValue		m_Pitch;
		AkLPFType		m_LPF;

		CAkTransition*	m_pvVolumeTransition;
		CAkTransition*	m_pvPitchTransition;
		CAkTransition*	m_pvLfeTransition;
		CAkTransition*	m_pvLPFTransition;
		AkStateTransitionInfo()
			:m_Volume	( 0.0f )
			,m_Lfe		( 0.0f )
			,m_Pitch	( 0 )
			,m_LPF		( 0 )
			,m_pvVolumeTransition	( NULL )
			,m_pvPitchTransition	( NULL )
			,m_pvLfeTransition		( NULL )
			,m_pvLPFTransition		( NULL )
		{}
	};

	AkStateTransitionInfo* m_pStateTransitionInfo;

	typedef CAkKeyArray<AkStateID, AkStateLink> AkMapStates;
	AkMapStates		m_mapStates;	// Map of States available for this sound

	AkStateGroupID	m_ulStateGroup;
	AkStateID		m_ulActualState;	//Actual state of the ParameterNode

	CAkSIS*			m_pGlobalSIS;

	struct FX
	{
		AkPluginID id; // Effect unique type ID. 
		AK::IAkPluginParam * pParam; // Parameters.
		bool bRendered;
	};

	struct FXChunk
	{
		FXChunk();
		~FXChunk();

		AkRTPCFXSubscriptionList listFXRTPCSubscriptions;
		FX aFX[ AK_NUM_EFFECTS_PER_OBJ ];
		AkUInt8 bitsMainFXBypass; // original bypass params | 0-3 is effect-specific, 4 is bypass all
	};

	FXChunk * m_pFXChunk;

	static AkReal32 roundReal32( const AkReal32& in_rfValue )
	{
		return in_rfValue < 0 ? ceilf( in_rfValue - 0.5f ) : floorf( in_rfValue + 0.5f );
	}

#pragma pack(push, 1)
	// Not Any volume can use the Compacted volume, 
	// Offset CANNOT use it since it may exceed 16 bits boundaries
	struct AkCompactedVolume
	{
	public:
		AkCompactedVolume():m_CompactedValue((AkInt16)AK_MAXIMUM_VOLUME_LEVEL){}
		AkVolumeValue GetValue()
		{ 
			return ((AkVolumeValue)m_CompactedValue)/10.f; 
		}
		void SetValue( AkVolumeValue in_Value )
		{
			m_CompactedValue = (AkInt16)( roundReal32( in_Value*10.f ) );
		}
		bool operator ==( AkCompactedVolume& in_Op )
		{
			return m_CompactedValue == in_Op.m_CompactedValue;
		}
		bool operator !=( AkCompactedVolume& in_Op )
		{
			return m_CompactedValue != in_Op.m_CompactedValue;
		}
	private:
		AkInt16 m_CompactedValue;
	};
#pragma pack(pop)

	AkCompactedVolume	m_VolumeMain;		// Volume
	AkCompactedVolume	m_LFEVolumeMain;	// Low frequency effect Volume
	AkInt16				m_PitchMain;		// Pitch
	AkUInt8				m_LPFMain;			// LPF

	AkUInt16		GetMaxNumInstances(){ return m_u16MaxNumInstance; }
	bool			IsMaxNumInstancesActivated(){ return GetMaxNumInstances() != 0; }

	AkUInt16		m_u16MaxNumInstance;				// Zero being no max.
	AkPriority      m_ucPriority;						// Priority 0-100.
	AkInt8          m_iPriorityDistanceOffset;          // Offset to priority at max radius if m_bPriorityApplyDistFactor.
	AkUInt8			m_bKillNewest					:1;
	AkUInt8			m_bIsVVoicesOptOverrideParent	:1;
	AkUInt8			m_bIsMaxNumInstOverrideParent	:1;

	AkUInt8			m_bPriorityApplyDistFactor	: 1;
	AkUInt8			m_bPriorityOverrideParent	: 1;
	AkUInt8			m_bUseState					: 1;	// Enable and disable the use of the state
	AkUInt8			m_bIsInDestructor			: 1;	// Enable and disable the use of the state
	AkUInt8			/*AkSyncType*/m_eStateSyncType	: NUM_BITS_SYNC_TYPE;

	CAkBitArrayMax32	m_RTPCBitArrayMax32;

	// Feedback information.  This is an optional structure which should be present
	// only if there is information to be kept.  This means either: 
	// a) the object is connected to a feedback bus
	// b) the user set a feedback volume (even if not connected directly, children could be affected)
	struct AkFeedbackInfo{
		AkFeedbackInfo()
		{
			m_pFeedbackBus = NULL;
		}
		AkCompactedVolume	m_VolumeFeedback;	// Feedback volume
		CAkFeedbackBus*		m_pFeedbackBus;		// Output bus
		AkLPFType			m_FeedbackLPF;		// Feedback low pass filter
	};

	AkFeedbackInfo* m_pFeedbackInfo;
};

#endif // _PARAMETER_NODE_BASE_H_
