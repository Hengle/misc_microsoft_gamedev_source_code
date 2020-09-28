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
// AkBus.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _BUS_H_
#define _BUS_H_

#include "AkActiveParent.h"
#include "AkDuckItem.h"
#include "AkKeyList.h"
#include <AK/Tools/Common/AkLock.h>

// class corresponding to a bus
//
// Author:  alessard
class CAkBus : public CAkActiveParent<CAkParameterNodeBase>
{
public:

	enum AkDuckingState
	{
		DuckState_OFF,
		DuckState_ON,
		DuckState_PENDING,//Waiting for the notification before unducking
	};

	struct AkDuckInfo
	{
		AkVolumeValue DuckVolume;
		AkTimeMs FadeOutTime;
		AkTimeMs FadeInTime;
		AkCurveInterpolation FadeCurve;
		bool operator ==(AkDuckInfo& in_Op)
		{
			return ( (DuckVolume == in_Op.DuckVolume) 
				&& (FadeOutTime == in_Op.FadeOutTime) 
				&& (FadeInTime == in_Op.FadeInTime) 
				&& (FadeCurve == in_Op.FadeCurve) 
				);
		}
	};

	//Thread safe version of the constructor
	static CAkBus* Create(AkUniqueID in_ulID = 0);

	// Check if the specified child can be connected
    //
    // Return - bool -	AK_NotCompatible
	//					AK_Succcess
	//					AK_MaxReached
    virtual AKRESULT CanAddChild(
        CAkAudioNode * in_pAudioNode  // Audio node ID to connect on
        );

	virtual AKRESULT AddChild(
        AkUniqueID in_ulID          // Input node ID to add
		);

	virtual AKRESULT RemoveChild(
        AkUniqueID in_ulID          // Input node ID to remove
		);

	virtual AkNodeCategory NodeCategory();	

	// NOT IMPLEMENTED IN THE CAkBus
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction );

	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction );

	virtual AKRESULT GetAudioParameters(
		AkSoundParams &out_Parameters,	// Set of parameter to be filled
		AkUInt32			in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&		io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj * in_GameObject,				// Game object associated to the query
		bool			in_bIncludeRange,		// Must calculate the range too
		AkPBIModValues& io_Ranges,				// Range structure to be filled
		bool			in_bDoBusCheck = true
		);

	AkVolumeValue GetBusEffectiveVolume();

	AkVolumeValue GetBusEffectiveLfe();

	// Set the pitch
	virtual void SetPitch(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
		AkValueMeaning	in_eValueMeaning,		//Target value meaning
		AkPitchValue		in_TargetValue = 0,		//Pitch target value
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Reset the Pitch
	virtual void ResetPitch(
		AkCurveInterpolation		in_eFadeCurve = AkCurveInterpolation_Linear,
		AkTimeMs		in_lTransitionTime = 0
		);

	// Set the volume
	virtual void SetVolume(
		CAkRegisteredObj *	in_GameObj,				//Game object associated to the action
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

	// Set the Volume
	virtual void Volume(
		AkVolumeValue in_Volume
		);

	// Set the Pitch
	virtual void Pitch(
		AkPitchValue in_Pitch
		);

	// Set the LFEVolume
	virtual void LFEVolume(
		AkVolumeValue in_LFEVolume
		);

	// Set the LPF
	virtual void LPF(
		AkLPFType in_LPF
		);

	virtual void ParamNotification( NotifParams& in_rParams );

	virtual void MuteNotification(
		AkUInt8			in_cMuteLevel,
		AkMutedMapItem& in_rMutedItem,
		bool			in_bIsFromBus = false
		);

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_GameObj,
		void*	in_pExceptArray = NULL
		);

	virtual void NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );
	virtual void UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem );

	void SetRecoveryTime( AkUInt32 in_RecoveryTime );

	AKRESULT AddDuck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve
		);

	AKRESULT RemoveDuck(
		AkUniqueID in_BusID
		);

	AKRESULT RemoveAllDuck();

	AKRESULT Duck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkCurveInterpolation in_eFadeCurve
		);

	AKRESULT Unduck(
		AkUniqueID in_BusID,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve
		);

	AKRESULT PauseDuck(
		AkUniqueID in_BusID
		);

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
		
	virtual void IncrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );
	virtual void DecrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL );

	bool IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest );

	bool IsOrIsChildOf( AkUniqueID in_NodeIDToTest );

	void DuckNotif();

	bool HasEffect();

	bool IsMasterBus();

	void CheckDuck();

	AkVolumeValue GetDuckedVolume();

	// Gets the Next Mixing Bus associated to this node
	//
	// RETURN - CAkBus* - The next mixing bus pointer.
	//						NULL if no mixing bus found
	virtual CAkBus* GetMixingBus();

	virtual CAkBus* GetLimitingBus();

	virtual AkRTPCFXSubscriptionList* GetFxRTPCSubscriptionList();

#ifndef AK_OPTIMIZED
	virtual void UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32			in_ulParamBlockSize
		);

	virtual void StopMixBus();
#endif

	virtual AKRESULT GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj = NULL
		);

	bool GetBypassFX( AkUInt32 in_uFXIndex );
	virtual bool GetBypassAllFX( CAkRegisteredObj * in_pGameObj = NULL );

	virtual void ResetFXBypass( 
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

	virtual AKRESULT SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize);
	virtual AKRESULT SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly );

	bool GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes );

	static void EnableHardwareCompressor( bool in_Enable );

protected:

	virtual CAkSIS* GetSIS( CAkRegisteredObj * in_GameObj = NULL );
	virtual void RecalcNotification();

	virtual CAkRTPCMgr::SubscriberType GetRTPCSubscriberType() const;

	// Constructors
    CAkBus(AkUniqueID in_ulID);

	//Destructor
    virtual ~CAkBus();

	AKRESULT Init();

	virtual AKRESULT SetInitialParams(AkUInt8*& pData, AkUInt32& ulDataSize );

	void StartDuckTransitions(CAkDuckItem*	in_pDuckItem,
								AkReal32			in_fTargetValue,
								AkValueMeaning	in_eValueMeaning,
								AkCurveInterpolation		in_eFadeCurve,
								AkTimeMs		in_lTransitionTime);

	AKRESULT RequestDuckNotif();

	void UpdateDuckedBus();

	AkUInt32 m_RecoveryTime; // Recovery time in output samples
	AkDuckingState m_eDuckingState;

	typedef CAkKeyList<AkUniqueID, AkDuckInfo, AkAllocAndFree> AkToDuckList;
	AkToDuckList m_ToDuckList;

	typedef CAkKeyList<AkUniqueID, CAkDuckItem, AkAllocAndFree> AkDuckedVolumeList;
	AkDuckedVolumeList m_DuckedVolumeList;

	AkUInt16 m_iPlayCountValid;

#ifdef XBOX360

public:
	static void XMP_MuteBackgroundMusic();
	static void XMP_UnmuteBackgroundMusic();

// For Wwise purpose, those two functions are exposed, 
// but defined only if connected to XBOX360

	void XMP_SetAsXMPBus();
	void XMP_UnsetAsXMPBus();

private:
	void XMP_Mute();
	void XMP_Unmute();

// XBOX360 exclusive members
	static CAkBus* m_pXMPBus;
	static CAkLock m_XMPLock;

	bool m_bIsXMPBus;
	bool m_bIsXMPMuted;

#endif // XBOX360

};
#endif //_BUS_H_
