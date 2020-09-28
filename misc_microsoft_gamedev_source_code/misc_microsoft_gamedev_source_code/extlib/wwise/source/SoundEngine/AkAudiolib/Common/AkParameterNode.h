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

//////////////////////////////////////////////////////////////////////
//
// AkParameterNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PARAMETER_NODE_H_
#define _PARAMETER_NODE_H_

#include "AkAudioNode.h"
#include "AkKeyArray.h"
#include "AkParameterNodeBase.h"
#include "AkPathManager.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkGen3DParams.h"
#include <AK/SoundEngine/Common/AkQueryParameters.h>

class CAkBus;
class CAkGen3DParams;
class CAkLayer;

enum AkPathMode;

struct Params3DSound;
struct AkPathVertex;
struct AkPathListItemOffset;

// class corresponding to node having parameters and states
//
// Author:  alessard
class CAkParameterNode : public CAkParameterNodeBase
{
	friend class CAkPBI;
	friend class CAkSIS;
	friend class CEventsUT;

public:
	// Constructors
    CAkParameterNode(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkParameterNode(void);

	AKRESULT Init(){ return CAkParameterNodeBase::Init(); }

	// Set the Volume
	virtual void Volume(
		AkVolumeValue in_Volume,//Volume
		AkVolumeValue in_MinRangeValue = 0.0f,
		AkVolumeValue in_MaxRangeValue = 0.0f
		);

	// Set the LFEVolume
	virtual void LFEVolume(
		AkVolumeValue in_LFEVolume,//LFEVolume
		AkVolumeValue in_MinRangeValue = 0.0f,
		AkVolumeValue in_MaxRangeValue = 0.0f
		);

	// Set the pitch
	virtual void Pitch(
		AkPitchValue in_Pitch,//Pitch
		AkPitchValue in_MinRangeValue = 0,
		AkPitchValue in_MaxRangeValue = 0
		);

	// Set the LPF
	virtual void LPF(
		AkLPFType in_LPF,//LPF
		AkLPFType in_MinRangeValue = 0,
		AkLPFType in_MaxRangeValue = 0
		);

	// Set the FeedBackVolume
	virtual void FeedbackVolume(
		AkVolumeValue in_Volume,
		AkVolumeValue in_MinRangeValue,
		AkVolumeValue in_MaxRangeValue
		);

	virtual void FeedbackLPF(
		AkLPFType in_FeedbackLPF,//Feedback LPF
		AkLPFType in_MinRangeValue,
		AkLPFType in_MaxRangeValue
		);

	// Set the pitch
	virtual void SetPitch(
		CAkRegisteredObj *	in_pGameObj,			//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkPitchValue		in_TargetValue = 0,		//Pitch target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Reset the pitch
	virtual void ResetPitch(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Set the volume
	virtual void SetVolume(
		CAkRegisteredObj *	in_pGameObj,			//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// Volume target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Reset the Volume
	virtual void ResetVolume(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Set the LFE
	virtual void SetLFE(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// LFE target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Reset the LFE
	virtual void ResetLFE(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Set the LPF
	virtual void SetLPF(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkReal32			in_fTargetValue = 0.0f,	// LPF target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Reset the LPF
	virtual void ResetLPF(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Mute the element
	virtual void Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);
	
	// Unmute the element for the specified game object
	virtual void Unmute(
		CAkRegisteredObj *	in_pGameObj,					//Game object associated to the action
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Un-Mute the element(per object and main)
	virtual void UnmuteAll(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	//Unmute all per object elements
	virtual void UnmuteAllObj(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Notify the children that the associated object was unregistered
	virtual void Unregister(
		CAkRegisteredObj * in_pGameObj //Game object associated to the action
		);

	// PhM
	void Get3DParams(
		CAkGen3DParams*& in_rp3DParams,
		CAkRegisteredObj * in_GameObj,
		bool in_bUpdateOnly,
		BaseGenParams * io_pBasePosParams
		);

	AKRESULT GetStatic3DParams( AkPositioningInfo& out_rPosInfo );

	// return true if using RTPCs
	bool UpdateBaseParams(
		CAkRegisteredObj * in_GameObj,
		BaseGenParams * io_pBasePosParams
		);

	// Fill the parameters structures with the new parameters
	//
	// Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT GetAudioParameters(
		AkSoundParams &out_Parameters,	// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		);

	AKRESULT GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck = true);

	// Reset the Volume
	virtual void ResetFeedbackVolume(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	virtual AkVolumeValue GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr );
	virtual AkLPFType GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr );

	virtual AKRESULT PlayAndContinueAlternate( AkPBIParams& in_rPBIParams );

	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );
	void OverrideFXParent( bool in_bIsFXOverrideParent );

	virtual AKRESULT GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj * in_GameObj = NULL
		);

	bool GetBypassFX( 
		AkUInt32	in_uFXIndex,
		CAkRegisteredObj * in_pGameObj );

	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj );

	virtual void ResetFXBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

//////////////////////////////////////////////////////////////////////////////
//Positioning information setting
//////////////////////////////////////////////////////////////////////////////

	AKRESULT PosSetPositioningType( AkPositioningType in_ePosType );
	AKRESULT PosSetConeUsage( bool in_bIsConeEnabled );
	
	AKRESULT PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled );
	AKRESULT PosSetAttenuationID( AkUniqueID in_AttenuationID );

	AKRESULT PosSetCenterPct( AkInt in_iCenterPct );

	AKRESULT PosSetPAN_RL( AkReal32 in_fPanRL );
	AKRESULT PosSetPAN_FR( AkReal32 in_fPanFR );
	AKRESULT PosSetPannerEnabled( bool in_bIsPannerEnabled );

	AKRESULT PosSetIsPositionDynamic( bool in_bIsDynamic );

	AKRESULT PosSetPathMode( AkPathMode in_ePathMode );
	AKRESULT PosSetIsLooping( bool in_bIsLooping );
	AKRESULT PosSetTransition( AkTimeMs in_TransitionTime );

	AKRESULT PosSetPath(
		AkPathVertex*           in_pArrayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

#ifndef AK_OPTIMIZED
	void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		);
#endif

	bool GetMaxRadius( AkReal32 & out_fRadius );
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

	inline void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior )
	{
		m_eBelowThresholdBehavior = in_eBelowThresholdBehavior;
	}
	inline void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior )
	{
		m_eVirtualQueueBehavior = in_eBehavior;
	}

	// Used to increment/decrement the playcount used for notifications and ducking
	virtual bool IncrementPlayCount(
		AkPriority in_Priority,
		CAkRegisteredObj * in_GameObj,
		AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL,
		AkUInt16 in_ui16NumKicked = 0,
		bool in_bMaxConsidered = false
		);

	virtual void DecrementPlayCount(
		CAkRegisteredObj * in_GameObj,
		AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL,
		bool in_bMaxConsidered = false
		);

	bool IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest, bool in_bIsBusChecked = false, bool in_bIgnoreNonBus = false );

	bool IsOrIsChildOf( AkUniqueID in_NodeIDToTest, bool in_bIsBusChecked = false );

	// Returns true if the Context may jump to virtual, false otherwise.
	AkBelowThresholdBehavior GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior );

	virtual bool Has3DParams();

	virtual AKRESULT UnsetRTPC(
		AkPluginID in_FXID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	virtual AkRTPCFXSubscriptionList* GetFxRTPCSubscriptionList();

	AKRESULT AssociateLayer( CAkLayer* in_pLayer );
	AKRESULT DissociateLayer( CAkLayer* in_pLayer );

	bool GetFxParentOverride(){ return m_bIsFXOverrideParent; }

protected:

	virtual void NotifyRTPCChanged();

#ifndef AK_OPTIMIZED
	virtual void InvalidatePaths();
#endif

	virtual AKRESULT SetInitialParams( AkUInt8*& pData, AkUInt32& ulDataSize );
	
	virtual AKRESULT SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	void Get3DCloneForObject( CAkRegisteredObj * in_GameObj, CAkGen3DParams*& in_rp3DParams, bool in_bUpdateOnly );

	//return true if using RTPC
	bool UpdateBaseParamsFromRTPC( CAkRegisteredObj * in_GameObj, BaseGenParams* in_pBasePosParams );

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj );

	AkPathState* GetPathState();

	AKRESULT Enable3DPosParams();
	AKRESULT Enable2DPosParams();
	void DisablePosParams();

private:
	bool EnableRangeParams();

	struct AkRangedParameters : public CAkObject
	{
		RANGED_MODIFIERS<AkVolumeValue>	Volume;
		RANGED_MODIFIERS<AkVolumeValue>	LFE;
		RANGED_MODIFIERS<AkPitchValue>	Pitch;
		RANGED_MODIFIERS<AkLPFType>		LPF;
		RANGED_MODIFIERS<AkVolumeValue> FeedbackVolume;
		RANGED_MODIFIERS<AkVolumeValue> FeedbackLPF;
	};

// members
private:
	typedef CAkKeyArray<CAkRegisteredObj *, AkUInt16> AkPerObjPlayCount;
	AkPerObjPlayCount m_ListPlayCountPerObj;

	typedef CAkKeyArray<CAkRegisteredObj *, CAkSIS*> AkMapSIS;
	AkMapSIS m_mapSIS;		// Map of specific parameters associated to an object

	BaseGenParams		m_BaseGenParams;

	CAkGen3DParams* m_p3DParameters;

	AkRangedParameters*	m_pRangedParams;

	// Advanced Settings
	AkUInt16				m_eVirtualQueueBehavior;
	AkUInt16				m_eBelowThresholdBehavior;

	//Others settings
	AkUInt8					m_bPositioningInfoOverrideParent	:1;
	AkUInt8					m_bIsFXOverrideParent				:1;
	AkUInt8					m_bIsSendOverrideParent				:1;

	// PhM : path state save structure
	AkPathState					m_PathState;

	typedef AkArray<CAkLayer*, CAkLayer*, ArrayPoolDefault, LIST_POOL_BLOCK_SIZE/sizeof(CAkLayer*)> LayerList;
	LayerList m_associatedLayers;
};

enum AkLoopValue
{
	AkLoopVal_Infinite		= 0,
	AkLoopVal_NotLooping	= 1,
};

#endif
