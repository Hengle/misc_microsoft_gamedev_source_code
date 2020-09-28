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
// AkPBI.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _PBI_H_
#define _PBI_H_

#include "AkParameters.h"
#include "AkMutedMap.h"
#include "AkTransition.h"
#include "PrivateStructures.h"
#include "AkContinuationList.h"
#include "AkMonitorData.h"
#include "AkActionPlay.h"
#include "AkRTPC.h"
#include "AkCntrHistory.h"
#include "AkRegisteredObj.h"
#include "AkBusCtx.h"
#include "AkRegistryMgr.h"
#include "AkSource.h"
#include "IAkTransportAware.h"
#include "AkGen3DParams.h"
#include "AkFeedbackStructs.h"

class CAkURenderer;
class CAkSoundBase;
class CAkSource;
class CAkRegisteredObj;
class CAkSrcBase;
class CAkTransition;
class CAkGen3DParams;
class CAkPath;
class CAkAttenuation;
class CAkPBIAware;

struct AkRTPCFXSubscription;
struct NotifParams;

class CAkUsageSlot;

#define AK_NO_IN_BUFFER_STOP_REQUESTED (-1)

#define AK_INVALID_SEQUENCE_ID 0

#define CODECID_FROM_PLUGINID(x) ((x>>(4+12))&0xffff)

enum AkCtxState
{
	CtxStateStop		= 0,
	CtxStatePause		= 1,
	CtxStateResume		= 2,
	CtxStatePlay		= 3,
	CtxStateToDestroy	= 4,
	CtxSetEstimate		= 5,
};

enum AkCtxDestroyReason
{
	CtxDestroyReasonFinished = 0,
	CtxDestroyReasonPlayFailed
};

struct AkSrcDescriptor
{
    AkUInt32	uiID;			// Src plugin id.
	void *	    pvPath;			// Pointer to the object's filename, filepath, or object pointer.
	AkFileID	ulFileID;		// The file ID
	AkUInt32	ulSize;			// Size of param block.
    AkUInt32 /*AkSrcType*/  Type    :SRC_TYPE_NUM_BITS; // Source type.
    AkUInt32    bIsLanguageSpecific :1;    // Language-specific source flag.
};

struct Prev2DParams
{
	BaseGenParams	prev2DParams;
	AkReal32		prevVolume;
	AkReal32		prevLfe;
	AkReal32		prevDryLevel;	

	Prev2DParams()
	{
		// Make sure the initial value is not a valid one, forcing the volume to compute 
		// the volume the first time without having to add an additionnal flag and an  another if in normal loop.
		// m_fPAN_RL range from -100 to 100. No need to init all values.
		prev2DParams.m_fPAN_RL = 101;
	}
};

struct AkPBIParams
{
	enum eType
	{
		PBI,
		ContinuousPBI,
		DynamicSequencePBI,
	};

	AkPBIParams()
	{
		//Default values.
		bTargetFeedback = false;	//Not a feedback PBI
		playHistory.Init();
	}
	
	AkPBIParams( PlayHistory& in_rPlayHist )
	{
		//Default values.
		bTargetFeedback = false;	//Not a feedback PBI
		playHistory = in_rPlayHist;
	}

	eType			    eType;
	CAkPBIAware*		pInstigator;
	CAkRegisteredObj*	pGameObj;
	TransParams*		pTransitionParameters;
	UserParams			userParams;
	PlayHistory			playHistory;
	AkPlaybackState		ePlaybackState;
	AkUInt32			uFrameOffset;

	// continuous specific member
	ContParams*			pContinuousParams;
	AkUniqueID			sequenceID;
	bool				bTargetFeedback;
	bool				bIsFirst;
};

// class corresponding to a Playback instance
//
// Author:  alessard
class CAkPBI : public CAkObject,
			   public ITransitionable,
               public IAkTransportAware
{
public:
	CAkPBI * pNextItem; // For CAkURenderer::m_listCtxs
	CAkPBI * pNextLightItem; // For sound's PBI List

public:

    // Constructor
	CAkPBI( CAkSoundBase*	in_pSound,			// Pointer to the sound.
			CAkSource*		in_pSource,
			CAkRegisteredObj * in_pGameObj,		// Game object and channel association.
			UserParams&		in_UserParams,		// User Parameters.
			PlayHistory&	in_rPlayHistory,	// History stuff.
			AkUniqueID		in_SeqID,			// Sample accurate seq id.
			AkPriority		in_Priority,
			bool			in_bTargetFeedback	// Do we send the data to the feedback pipeline?
			);

    //Destructor
	virtual ~CAkPBI(void);

	virtual AKRESULT Init( AkPathInfo* in_pPathInfo );
	virtual void Term();

	virtual void TransUpdateValue(
		TransitionTargetTypes in_eTargetType,	// Transition target type
		TransitionTarget in_unionValue,			// New Value
		bool in_bIsTerminated					// Is it the end of the transition
		);

	AKRESULT SetParam(
			AkPluginParamID in_paramID,         ///< Plug-in parameter ID
			void *			in_pParam,          ///< Parameter value pointer
			AkUInt32		in_uParamSize		///< Parameter size
			);

	virtual void SetEstimatedLength( AkTimeMs in_EstimatedLength );

	// Play the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _InitPlay();

	// Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Play( TransParams & in_transParams, bool in_bPaused, bool in_bForceIgnoreSync = false );

	// Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT _Stop( AkPBIStopMode in_eStopMode = AkPBIStopMode_Normal, bool in_bIsFromTransition = false );

	// Stop the PBI (the PBI is then destroyed)
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Stop( TransParams & in_transParams );

#ifndef AK_OPTIMIZED
	virtual AKRESULT _StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		);
#endif

	//Pause the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT _Pause( bool in_bIsFromTransition = false );

	//Pause the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Pause( TransParams & in_transParams );

	//Resume the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	virtual AKRESULT _Resume();

	//Resume the PBI
	//
	//Return - AKRESULT - AK_Success if succeeded
	AKRESULT _Resume( TransParams & in_transParams, bool in_bIsMasterResume );

	virtual AKRESULT PlayToEnd( CAkAudioNode * in_pNode );

	// Gives the information about the associated game object
	//
	// Return - CAkRegisteredObj * - The game object and Custom ID
	CAkRegisteredObj * GetGameObjectPtr() { return m_pGameObj; }

	void ParamNotification( NotifParams& in_rParams );

	// Notify the PBI that the Mute changed
	void MuteNotification(
		AkUInt8 in_cMuteLevel, //New mute level
		AkMutedMapItem& in_rMutedItem, //Element where mute changed
		bool in_bPrioritizeGameObjectSpecificItems = false
		);

	// Notify the PBI that a 3d param changed
	void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// New muting level
		AkRTPC_ParameterID	in_ParameterID	// RTPC ParameterID, must be a Positioning ID.
		);

	// Notify the PBI that the PBI must recalculate the parameters
	void RecalcNotification();

	void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask = 0xFFFFFFFF
		);

#ifndef AK_OPTIMIZED
	void InvalidatePaths();
#endif

	//Calculate the Muted/Faded effective volume
	void CalculateMutedEffectiveVolume();

	// Get Sound.
	CAkSoundBase*	GetSound() { return m_pSound; }

	bool		IsInstanceCountCompatible( AkUniqueID in_NodeIDToTest );

	// get the current play stop transition
	CAkTransition* GetPlayStopTransition();

	// get the current pause resume transition
	CAkTransition*	GetPauseResumeTransition();

    // direct access to Mute Map
    AKRESULT        SetMuteMapEntry( 
        AkMutedMapItem & in_key,
        AkUInt8 in_uFadeRatio
        );
  
	virtual void	SetPauseStateForContinuous(bool in_bIsPaused);

	// Prepare the Sample accurate next sound if possible and available.
	virtual void	PrepareSampleAccurateTransition();

	// 3D sound parameters.
	CAkGen3DParams *	Get3DSound() { return m_p3DSound; }

	const BaseGenParams& GetBasePosParams(){ return m_BasePosParams; }
	Prev2DParams& GetPrevPosParams(){ return m_Prev2DParams; }

	// Setup fx with RTPC manager.
	AKRESULT			SubscribeFxRTPC();
	void				UnsubscribeRTPC();
	AKRESULT			SubscribeAttenuationRTPC( CAkAttenuation* in_pAttenuation );

	AKRESULT			UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem );
	AKRESULT			NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	// Fx management.
	AKRESULT			CreateFx();
	void				DestroyFx();

	void				GetDataPtr( AkUInt8 *& out_pBuffer, AkUInt32 & out_uDataSize ) { out_pBuffer = m_pDataPtr; out_uDataSize = m_uDataSize; }

	void				LockParams();
	void				UnlockParams();

	// IAkAudioCtx interface implementation.
	AKRESULT			Destroy( AkCtxDestroyReason in_eReason );
	AKRESULT			Lock();
	AKRESULT			Unlock();

	AKRESULT			Play();
	AKRESULT			Stop();
	AKRESULT			Pause();
	AKRESULT			Resume();
	void				NotifAddedAsSA();

	bool                WasPaused() { return m_bWasPaused; }
	bool				WasStopped() { return m_bWasStopped; }
	
	void				ProcessContextNotif( AkCtxState in_eState, AkCtxDestroyReason in_eDestroyReason = CtxDestroyReasonFinished, AkTimeMs in_EstimatedLength = 0 );

	AkUniqueID			GetSoundID();
	AkUniqueID			GetSequenceID() { return m_SeqID; }
	AK::CAkBusCtx		GetBusContext();
	AKRESULT			SetDuration( AkTimeMs in_Duration );
	AkPriority			GetPriority() { return m_Priority; }
	bool				IsPrefetched() { return m_pSource->IsZeroLatency(); }
	AkUInt16			GetLooping() { return m_LoopCount; }
	AkAudioFormat *	    GetMediaFormat() { return m_pSource->GetMediaFormat(); }
    void				SetPluginMediaFormat( AkAudioFormat & in_rMediaFormat );		 
	AkBelowThresholdBehavior GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior );

	virtual AKRESULT	GetParams( AkSoundParams * io_Parameters );
    void				GetPositioningParams( AkSoundPositioningParams * io_Parameters );
	AkPositioningType	GetPositioningType();
	AkVolumeValue		GetVolume();
	virtual AkPitchValue GetPitch();
    void				GetEnvironmentValues( AkEnvironmentValue* AK_RESTRICT io_paEnvVal );

	AkFeedbackParams* GetFeedbackParameters();

	AkForceInline bool IsForFeedbackPipeline(){ return m_bTargetIsFeedback; }

	AkReal32 GetDryLevelValue(){ return m_pGameObj->GetDryLevelValue(); }
	AkReal32 GetObstructionValue( AkUInt32 in_uListener ){ return m_pGameObj->GetObjectObstructionValue( in_uListener ); }
	AkReal32 GetOcclusionValue( AkUInt32 in_uListener ){ return m_pGameObj->GetObjectOcclusionValue( in_uListener ); }

	AKRESULT			GetSrcDescriptor( AkSrcDescriptor * in_pSrcDesc );

	const AkFXDesc&		GetFX( AkUInt32 in_uFXIndex ) { return m_aFXInfo[ in_uFXIndex ]; }
	bool                GetBypassAllFX() { return m_bBypassAllFX; }
	const AkSourceDesc& GetSourceDesc() { return m_SourceInfo; }
	CAkSource*			GetSource(){ return m_pSource; }

	AkPathInfo*			GetPathInfo() { return &m_PathInfo; }

	bool				WasKicked(){ return m_bWasKicked; }
	AKRESULT			Kick();

	AkPlayingID			GetPlayingID() { return m_UserParams.PlayingID; };

	virtual AkUInt32	GetSourceOffset();
	virtual AkUInt32	GetAndClearStopOffset();

	void SetUsageSlotToRelease( CAkUsageSlot* in_pSlot )
	{ 
		m_pUsageSlot = in_pSlot; 
	}

	CAkUsageSlot* GetUsageSlotToRelease(){ return m_pUsageSlot; }

	virtual void SetSourceOffset( AkUInt32 in_ulSourceOffset ){}

	AkForceInline AkInt32 GetFrameOffset() { return m_iFrameOffset; }
	AkForceInline void SetFrameOffset( AkInt32 in_iFrameOffset )
    { 
        m_iFrameOffset = in_iFrameOffset; 
    }
    AkForceInline void ConsumeFrameOffset( AkInt32 in_iSamplesConsumed ) 
    { 
        m_iFrameOffset -= in_iSamplesConsumed; 
    }

	void FlagAsPlayFailed(){ m_bPlayFailed = true; }

#ifndef AK_OPTIMIZED
	AkForceInline bool DoNotifyStarvationAtStart() { return m_bNotifyStarvationAtStart; }

	void UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32		in_ulParamBlockSize
		);

	void UpdateAttenuationInfo();

#endif //AK_OPTIMIZED

#ifdef RVL_OS
	bool NeedPriorityUpdate()
	{
		bool returnedvalue = m_bDoPriority;
		m_bDoPriority = false;
		return returnedvalue;
	}
#endif

protected:
	enum PBIInitialState
	{
		PBI_InitState_Playing,
		PBI_InitState_Paused,
		PBI_InitState_Stopped,
	};

#ifndef AK_OPTIMIZED


	// Internal use only
	// Notify the monitor with the specified reason
	virtual void Monitor(
		AkMonitorData::NotificationReason in_Reason				// Reason for the notification
		);
#else
	void Monitor(
		AkMonitorData::NotificationReason in_Reason				// Reason for the notification
		)
	{/*No implementation in Optimized version*/}
#endif

	// Internal use only
	// Notify the monitor with the specified reason
	virtual void MonitorFade(
		AkMonitorData::NotificationReason in_Reason,			// Reason for the notification
		AkTimeMs in_FadeTime
		);

	void CreateTransition(
		bool in_bIsPlayStopTransition,			// true if it is a PlayStop transition, false if it is PauseResume
		TransitionTargets in_transitionTarget,		// Transition target type
		TransParams in_transParams,                 // Transition parameters.
		bool in_bIsFadingTransition				// is the transition fading out(pause of stop)
		);

	// Prepare PBI for stopping with minimum transition time. Note: also used in PBI subclasses.
	void StopWithMinTransTime();

	void DecrementPlayCount();

    void RemoveAllVolatileMuteItems();

	CAkUsageSlot*		m_pUsageSlot;

	PBIInitialState		m_eInitialState;

	AkMutedMap			m_mapMutedNodes;
	AkPBIModValues		m_Ranges;

	CAkGen3DParams *	m_p3DSound;					// 3D parameters.
    
	UserParams			m_UserParams;				// User Parameters.

	PlaybackTransition	m_PBTrans;
	
	AkUniqueID			m_SeqID;					// Sample accurate seq id.
	CAkSoundBase*		m_pSound;					// Parent SoundNode
	CAkSource*			m_pSource;					// Associated Source
	AkVolumeValue		m_Volume;					// Effective volume (ignores Fades and mute).
	AkVolumeValue		m_RealEffectiveVolume;		// Effective volume (Includes mutes and fades).
	AkVolumeValue		m_Lfe;						// Lfe (ignores Fades and mute).
	AkVolumeValue		m_RealEffectiveLfe;			// Lfe (Includes mutes and fades).
	AkPitchValue		m_EffectivePitch;			// Effective Pitch.
	AkLPFType			m_EffectiveLPF;				// Effective LPF.
	CAkRegisteredObj*	m_pGameObj;		// CAkRegisteredObj to use to Desactivate itself once the associated game object were unregistered.
	
	CAkCntrHist			m_CntrHistArray;

	AkCtxState			m_State;

	AkUInt8				m_bAreParametersValid	:1;
    AkUInt8				m_b3DPositionDone :1;		// used when not dynamic.

	AkUInt8				m_bGetAudioParamsCalled	:1;

	AkUInt8				m_bNeedNotifyEndReached :1;
	AkUInt8				m_bIsNotifyEndReachedContinuous :1;

	AkUInt8				m_bTerminatedByStop :1;
	AkUInt8				m_bPlayFailed :1;
	
	//This flag is used to avoid sending commands that would be invalid anyway after sending a stop.
	AkUInt8				m_bWasStopped	:1;
	AkUInt8				m_bWasPreStopped:1;
	AkUInt8				m_bWasPaused	:1;

	AkUInt8				m_bInitPlayWasCalled : 1;

	AkUInt8				m_bWasKicked : 1;
	AkUInt8				m_bWasPlayCountDecremented : 1;

#ifndef AK_OPTIMIZED
	AkUInt8				m_bNotifyStarvationAtStart : 1;	// True for PBIs that should not starve on start up (e.g. music).
#endif

#ifdef RVL_OS
	AkUInt8				m_bDoPriority : 1;				// Only required on platforms that have to update the hardware priority
#endif

	AkUInt8				m_bFeedbackParametersValid	:1;
	AkUInt8				m_bTargetIsFeedback : 1;

	AkUInt8				m_cPlayStopFade;
	AkUInt8				m_cPauseResumeFade;

	AkPriority          m_Priority;

	AkFXDesc			m_aFXInfo[ AK_NUM_EFFECTS_PER_OBJ ];
	bool                m_bBypassAllFX;
	AkSourceDesc		m_SourceInfo;

	AkInt16				m_LoopCount;

	AkUInt32			m_ulPauseCount;

	AkInt32			    m_iFrameOffset;

	AkUInt8 *			m_pDataPtr;
	AkUInt32			m_uDataSize;

	// Back up of the first position when the position of the sound is not dynamic
	AkSoundPositionEntry m_FirstPosition;

	BaseGenParams		m_BasePosParams;
	Prev2DParams		m_Prev2DParams;

	// Feedback information.  This is an optional structure which should be present
	// only if there is information to be kept.  This means either: 
	// a) the object is connected to a feedback bus
	// b) the user set a feedback volume (even if not connected directly, children could be affected)
	AkFeedbackParams* m_pFeedbackInfo;

private:

	void PausePath(
		bool in_bPause
		);

	AKRESULT RefreshParameters( AkSoundParams & io_SoundParams );
	void ClearParameters( AkSoundParams & io_SoundParams );
	AkReal32 Scale3DUserDefRTPCValue( AkReal32 in_fValue );

	AkPathInfo			m_PathInfo;			// our path if any

// Static members:
	static CAkLock		m_csLock;			// Object lock.

#ifndef AK_OPTIMIZED
	static	CAkLock		m_csLockParams;		// Parameter lock. Only to protect from Wwise changing parameters.
#endif
};

#endif
