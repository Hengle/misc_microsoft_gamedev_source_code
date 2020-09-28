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
// AkBankStructs.h
//
//////////////////////////////////////////////////////////////////////
#ifndef AK_BANK_STRUCTS_H_
#define AK_BANK_STRUCTS_H_

#include "AkParameters.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include "AkCommon.h"
#pragma warning(disable:4530)
#include <vector>
#pragma warning(default:4530)
#include "AkBanks.h"
#include "AkPath.h"
#include "AkDecisionTree.h"
#include "IWAttenuation.h"
#include "IWProject.h"

namespace AkBank
{
	struct AKBKLoopingParams
	{
		AkUInt16	LoopCount; // 0 = Infinite looping, 1 = no looping
		AkUInt16	LoopCountModMin;
		AkUInt16	LoopCountModMax;
	//Constructor
		AKBKLoopingParams()
			:LoopCount(1)
			,LoopCountModMin(0)
			,LoopCountModMax(0)
		{}
	};

	struct AKBKFX
	{
		AkUInt32	FXID;
		AkUInt8	bIsBypassed;
		AkUInt8	bIsRendered;
		AkUInt8 ucIndex;

	//Constructor
		AKBKFX()
			:FXID(AK_INVALID_PLUGINID)
			,bIsBypassed(false)
			,bIsRendered(false)
			,ucIndex(0)
			,FXPresetSize(0)
			,parrayFXPreset(NULL)
		{}
		AKBKFX( const AKBKFX& in_rhs )
		{
			FXID = in_rhs.FXID;
			bIsBypassed = in_rhs.bIsBypassed;
			bIsRendered = in_rhs.bIsRendered;
			ucIndex = in_rhs.ucIndex;
			FXPresetSize = in_rhs.FXPresetSize;
			parrayFXPreset = NULL;

			SetFXPreset( in_rhs.parrayFXPreset, in_rhs.FXPresetSize );
		}

		~AKBKFX()
		{
			if ( parrayFXPreset )
			{
				free( parrayFXPreset );
			}
		}

		void SetFXPreset( AkUInt8* in_pData, AkUInt32 in_ulSize )
		{
			if ( in_ulSize && in_pData )
			{
				parrayFXPreset = (AkUInt8*)malloc( in_ulSize );
				AKPLATFORM::AkMemCpy( parrayFXPreset, in_pData, in_ulSize );
				FXPresetSize = in_ulSize;
			}
			else if ( parrayFXPreset )
			{
				free ( parrayFXPreset );
				parrayFXPreset = NULL;
				FXPresetSize = 0;
			}
		}
		AkUInt32 GetFXPresetSize() const { return FXPresetSize; }
		AkUInt8* GetFXPresetData() const { return parrayFXPreset; }

	private:
		AkUInt32	FXPresetSize;
		AkUInt8*	parrayFXPreset;
	};

	struct AKBKRTPC
	{
		AkUInt32 FXID; // FX or plug-in source ID, Set as AK_INVALID_UNIQUE_ID if it should apply directly to the sound or if not on a sound
		AkUInt32	RTPCID;
		AkUInt32	ParamID;
		AkUInt32 RTPCCurveID;
		AkCurveScaling Scaling;
		std::vector<AkRTPCGraphPoint> ConversionTable;
		AKBKRTPC()
			:FXID(AK_INVALID_UNIQUE_ID)
			,RTPCID(0)
			,ParamID(0)
			,RTPCCurveID(0)
		{}
	};

	struct AKBKFXParamSet
	{
		AkUInt32					ParamSetID;
		AKBKFX					FXParams;
		std::vector<AKBKRTPC>	RTPCList;
		AKBKFXParamSet()
			:ParamSetID(AK_INVALID_UNIQUE_ID)
		{}
	};

	struct AKBKState
	{
		AkUniqueID	ID;
		AkReal32		Volume;
		AkReal32		LFEVolume;
		AkUInt32		Pitch;
		AkLPFType	LPF;
		AkUInt8		eVolumeValueMeaning;
		AkUInt8		eLFEValueMeaning;
		AkUInt8		ePitchValueMeaning;
		AkUInt8		eLPFValueMeaning;
	//Constructor
		AKBKState()
			:ID(AK_INVALID_UNIQUE_ID)
			,Volume(0)
			,LFEVolume(0)
			,Pitch(0)
			,LPF(0)
			,eVolumeValueMeaning(AkValueMeaning_Offset)
			,eLFEValueMeaning(AkValueMeaning_Offset)
			,ePitchValueMeaning(AkValueMeaning_Offset)
			,eLPFValueMeaning(AkValueMeaning_Offset)
		{}
	};

	struct AKBKParameterNodeParams
	{
		AkReal32		Volume;
		AkReal32		VolumeModMin;
		AkReal32		VolumeModMax;

		AkReal32		Lfe;
		AkReal32		LfeModMin;
		AkReal32		LfeModMax;

		AkUInt32		Pitch;
		AkUInt32		PitchModMin;
		AkUInt32		PitchModMax;

		AkLPFType	LPF;
		AkLPFType	LPFModMin;
		AkLPFType	LPFModMax;

		AkUInt32		ulStateGroup;
	//Constructor
		AKBKParameterNodeParams()
			:Volume(0)
			,VolumeModMin(0)
			,VolumeModMax(0)
			,Lfe(0)
			,LfeModMin(0)
			,LfeModMax(0)
			,Pitch(0)
			,PitchModMin(0)
			,PitchModMax(0)
			,LPF(0)
			,LPFModMin(0)
			,LPFModMax(0)
			,ulStateGroup(0)
		{}
	};

	struct AKBKPathVertex
	{
		AkReal32			fX;
		AkReal32			fY;
		AkReal32			fZ;
		AkTimeMs		Duration;// Time to get to next one, should be zero on last point of a path
		AKBKPathVertex()
			:fX(0)
			,fY(0)
			,fZ(0)
			,Duration(0)
		{}
	};

	struct AKBKPath
	{
		std::vector<AKBKPathVertex>		listVertexes;
		AKBKPath()
		{}
	};

	struct AKBKPathPlaylistItem
	{
		AkUInt32 ulOffset;		//in vertices
		AkUInt32 ulNumVertices;
		AKBKPathPlaylistItem()
			:ulOffset(0)
			,ulNumVertices(0)
		{}
	};

	struct AKBKAttenuation
	{
		AkUniqueID			ID;

		// Cone params
		AkUInt8				bIsConeEnabled;
		AkReal32			cone_fInsideAngle;
		AkReal32			cone_fOutsideAngle;
		AkReal32			cone_fOutsideVolume;
		AkLPFType			cone_LoPass;

		IWAttenuation::CurveToUse		aCurveToUse[IWAttenuation::Curve_Max];
		AkCurveScaling					aScalingCurve[IWAttenuation::Curve_Max];
		std::vector<AkRTPCGraphPoint>	Curves[IWAttenuation::Curve_Max];
		std::vector<AKBKRTPC>			RTPCList;

		AKBKAttenuation()
			:ID(0)
			,bIsConeEnabled(false)
			,cone_fInsideAngle(0)
			,cone_fOutsideAngle(0)
			,cone_fOutsideVolume(0)
			,cone_LoPass(0)
		{}
	};

	struct AKBKPositioningInfo
	{
		//AKBK purpose flag only, the rest is not written or read if the bIs3DPositioningEnabled flag is false
		/////////////////////////////
		AkUInt8	bOverrideParent;			//AKBK purpose only,
		AkUInt8	bIs3DPositioningEnabled;	//AKBK purpose only,
		/////////////////////////////

		//Common
		AkInt32				iDivergenceCenter;

		//2D
		AkUInt8				bIsPannerEnabled;
		AkReal32			fPAN_RL; //only if panner is enabled
		AkReal32			fPAN_FR; //only if panner is enabled

		//3D (general)
		AkPositioningType	ePosType; // game-defined or user-defined
		AkUInt32			uAttenuationID;
		AkUInt8				bIsSpatialized;

		//3D (game-defined)
		AkUInt8				bIsDynamic;

		//3D (user-defined)
		AkPathMode			ePathMode;
		AkUInt8				bIsLooping;
		AkTimeMs			transitionTime;

		std::vector<AKBKPath>	listPath;
	

		AKBKPositioningInfo()
		:bIs3DPositioningEnabled(false)
		,bOverrideParent(false)
		,iDivergenceCenter(0)
		,ePosType(Ak3DGameDef)
		,bIsPannerEnabled(false)
		,fPAN_RL(0)
		,fPAN_FR(100)
		,uAttenuationID(AK_INVALID_UNIQUE_ID)
		,bIsSpatialized(false)
		,bIsDynamic(true)
		,ePathMode(AkContinuousSequence)
		,bIsLooping(false)
		,transitionTime(0)
		{}
	};
	
	struct AKBKFeedbackInfo
	{
		AkUInt32	ulFeedbackBus;	//May be null
		AkReal32	fFeedbackVolume;
		AkReal32	fVolumeModMin;
		AkReal32	fVolumeModMax;

		AkLPFType	fFeedbackLPF;
		AkLPFType	fFeedbackLPFModMin;
		AkLPFType	fFeedbackLPFModMax;

		AKBKFeedbackInfo() 
			:fFeedbackVolume(0)
			,ulFeedbackBus(0)
			,fVolumeModMin(0)
			,fVolumeModMax(0)
			,fFeedbackLPF(0)
			,fFeedbackLPFModMin(0)
			,fFeedbackLPFModMax(0)
		{}
	};

	struct AKBKParameterNode
	{
		std::vector<AKBKFX>		FXList;
		AkUInt32				ulParentBusID; //0 if using parent one
		AkUInt32				ulDirectParentID;
		AkUInt8					ucPriority;
		AkUInt8					bPriorityOverrideParent;
		AkUInt8					bPriorityApplyDistFactor;
		AkInt8					iPriorityDistanceOffset;
		AKBKParameterNodeParams	ParamNodeParams;
		AKBKPositioningInfo		Positioning;

		AkUInt32				eVirtualQueueBehavior;
		AkUInt8					bKillNewest;
		AkUInt16				MaxNumInstance;// Zero being no max.
		AkUInt32				eBelowThresholdBehavior;
		AkUInt8					bIsMaxNumInstOverrideParent;
		AkUInt8					bIsVVoiceOptOverrideParent;

		AkUInt32				eStateSyncType;
		std::vector<AKBKStateItem>	StateList;
		AkUInt8						bIsFXOverrideParent;
		AkUInt8						bBypassAllFX;
		std::vector<AKBKRTPC>		RTPCList;
		
		AKBKFeedbackInfo		Feedback;

	//Constructor
		AKBKParameterNode()
            : ulParentBusID( AK_INVALID_UNIQUE_ID )
			,ulDirectParentID( AK_INVALID_UNIQUE_ID )
			,ucPriority( AK_DEFAULT_PRIORITY )
			,bPriorityOverrideParent( false )
			,bPriorityApplyDistFactor( false )
			,eVirtualQueueBehavior( AkVirtualQueueBehavior_FromBeginning )
			,bKillNewest( false )
			,MaxNumInstance( 0 )// Zero being no max.
			,eBelowThresholdBehavior( AkBelowThresholdBehavior_ContinueToPlay )
			,bIsMaxNumInstOverrideParent( false )
			,bIsVVoiceOptOverrideParent( false )
			,eStateSyncType( 0 )
			,bIsFXOverrideParent( false )
			,bBypassAllFX( false )
		{}
	};

	struct AKBKSound
	{
		AkUniqueID			ID;
		AKBKSource			SourceInfo;
		AKBKParameterNode 	ParamNode;
		AKBKLoopingParams 	LoopingParams;
	//Constructor
		AKBKSound()
			:ID(0)
		{}
	};

	struct AKBKFeedbackNode
	{
		AkUniqueID			ID;
		std::vector<AKBKFeedbackSource>	Sources;
		AKBKParameterNode 	ParamNode;
		AKBKLoopingParams 	LoopingParams;
		//Constructor
		AKBKFeedbackNode()
			:ID(0)
		{}
	};

	struct AKBKTrackSrcInfo
	{
		AkUInt32	trackID;
		AkUInt32  	sourceID;			// ID of the source 
		AkReal64    fPlayAt;            // Play At (ms).
		AkReal64    fBeginTrimOffset;   // Begin Trim offset (ms).
		AkReal64    fEndTrimOffset;     // End Trim offset (ms).
		AkReal64    fSrcDuration;       // Duration (ms).

		AKBKTrackSrcInfo()
			:trackID( AK_INVALID_UNIQUE_ID )
			,sourceID( AK_INVALID_UNIQUE_ID )
			,fPlayAt( 0 )
			,fBeginTrimOffset( 0 )
			,fEndTrimOffset( 0 )
			,fSrcDuration( 0 )
		{}
	};

	struct AKBKMusicTrack
	{
		AkUniqueID					ID;
		std::vector<AKBKSource>		Sources;
		std::vector<AKBKTrackSrcInfo> PlayList;
		AkUInt32					uNumSubTrack;
		AKBKParameterNode 			ParamNode;
		AKBKLoopingParams 			LoopingParams;
		AkUInt32					eRSType;
		AkTimeMs					iLookAheadTime;
	//Constructor
		AKBKMusicTrack()
			:ID( AK_INVALID_UNIQUE_ID )
			,iLookAheadTime( 0 )
		{}
	};

	struct AKBKPlaylistItem
	{
		AkUniqueID		ID;			// Unique ID of the PlaylistItem
		AkUInt8			cWeight;	// Weight(only for random containers, field unused in sequence mode)
	//Constructor
		AKBKPlaylistItem()
			:ID(0)
			,cWeight(DEFAULT_RANDOM_WEIGHT)
		{}
	};

	struct AKBKRanSeqParams
	{
		AkTimeMs	TransitionTime;
		AkTimeMs	TransitionTimeModMin;
		AkTimeMs	TransitionTimeModMax;

		AkUInt16	wAvoidRepeatCount;

		AkUInt8	eTransitionMode;
		AkUInt8	eRandomMode;
		AkUInt8	eMode;
		AkUInt8	bIsUsingWeight;
		AkUInt8	bResetPlayListAtEachPlay;
		AkUInt8	bIsRestartBackward;
		AkUInt8	bIsContinuous;
		AkUInt8	bIsGlobal;
	//Constructor
		AKBKRanSeqParams()
			:TransitionTime(0)
			,TransitionTimeModMin(0)
			,TransitionTimeModMax(0)
			,wAvoidRepeatCount(0)
			,eTransitionMode(Transition_Disabled)
			,eRandomMode(RandomMode_Normal)
			,eMode(ContainerMode_Random)
			,bIsUsingWeight(false)
			,bResetPlayListAtEachPlay(false)
			,bIsRestartBackward(false)
			,bIsContinuous(false)
			,bIsGlobal(true)
		{}
	};

	struct AKBKRanSeqCntr
	{
		AkUniqueID				ID;
		AKBKParameterNode 		ParamNode;
		AkUInt16				LoopCount;	
		AKBKRanSeqParams		ParamRanSeqCntr;
		std::vector<AkUniqueID>		ChildList;
		std::vector<AKBKPlaylistItem>	PlayList;
	//Constructor
		AKBKRanSeqCntr()
			:ID(AK_INVALID_UNIQUE_ID)
			,LoopCount( AkLoopVal_NotLooping )
		{}
	};

	struct AKBKSwitchParams
	{
		AkUInt32		eGroupType;				// Is it binded to state or to switches, 0 = switch, 1 = state, internally defined in enum "AkGroupType"
		AkUInt32		ulGroupID;				// May either be a state group or a switch group
		AkUInt32		ulDefaultSwitch;		// Default value, to be used if none is available, 
												// it may be none, in which case we must play nothing...
		AkUInt8		bIsContinuousValidation;	// Is the validation continuous
	//Constructor
		AKBKSwitchParams()
			:eGroupType( AkGroupType_Switch )
			,ulGroupID( 0 )
			,ulDefaultSwitch( 0 )
			,bIsContinuousValidation( false )
		{}
	};

	struct AKBKSwitchItemParam
	{
		AkUniqueID		ID;
		AkUInt8			bIsFirstOnly;
		AkUInt8			bIsContinuousPlay;
		AkUInt32			eOnSwitchMode;
		AkTimeMs		FadeOutTime;
		AkTimeMs		FadeInTime;

	//Constructor
		AKBKSwitchItemParam()
			:ID( 0 )
			,bIsFirstOnly( false )
			,bIsContinuousPlay( false )
			,eOnSwitchMode( AkOnSwitchMode_PlayToEnd )
			,FadeOutTime( 0 )
			,FadeInTime( 0 )
		{}
	};

	struct AKBKSwitchGroup
	{
		AkUInt32 ulSwitchID;
		std::vector<AkUniqueID>		SwitchItemList;
	//Constructor
		AKBKSwitchGroup()
			:ulSwitchID(0)
		{}
	};

	struct AKBKSwitchCntr
	{
		AkUniqueID					ID;
		AKBKParameterNode 			ParamNode;
		AKBKSwitchParams			ParamSwitchCntr;
		std::vector<AkUniqueID>			ChildList;
		std::vector<AKBKSwitchGroup>		SwitchGroupList;
		std::vector<AKBKSwitchItemParam>	SwitchParamList;
	//Constructor
		AKBKSwitchCntr()
			:ID(AK_INVALID_UNIQUE_ID)
		{}
	};

	struct AKBKMeterInfo
	{
		AkReal64    fGridPeriod;        // Grid period (1/frequency) (ms).
		AkReal64    fGridOffset;        // Grid offset (ms).
		AkReal32    fTempo;             // Tempo: Number of Quarter Notes per minute.
		AkUInt8     uTimeSigNumBeatsBar;// Time signature numerator.
		AkUInt8     uTimeSigBeatValue;  // Time signature denominator.
		AkUInt8		bOverrideParent;	
	};

	struct AKBKStinger
	{
		AkUInt32	m_TriggerID;
		AkUInt32	m_SegmentID;
		AkUInt32	m_SyncPlayAt;		//AkSyncType
		AkInt32		m_DontRepeatTime;	//ms(AkTimeMs)

		AkUInt32	m_numSegmentLookAhead;
	};

	struct AKBKMusicNode
	{
		AkUniqueID					ID;
		AKBKParameterNode 			ParamNode;
		std::vector<AkUniqueID>		ChildList;
		AKBKMeterInfo				meterInfo;
		std::vector<AKBKStinger>	stingers;
	
	//Constructor
		AKBKMusicNode()
			:ID( AK_INVALID_UNIQUE_ID )
		{}
	};

	struct AKBKMusicFade
	{
		AKBKMusicFade()
			: iTransitionTime( 0 )
			, eFadeCurve( AkCurveInterpolation_Linear )
			, iFadeOffset( 0 )

		{}
		AkInt32		iTransitionTime;	// how long this should take (in ms)
		AkUInt32	eFadeCurve;			// (AkCurveInterpolation)what shape it should have
		AkInt32		iFadeOffset;		// Fade offset.
	};

	struct AKBKMusicTransitionRule
	{
		// Important to add all new members in the constructor.
		// If unused values are not set to a common correct value, 
		AKBKMusicTransitionRule()
			: srcID( 0 )
			, destID( 0 )
			, eSrcSyncType( 0 )
			, bSrcPlayPostExit( 0 )
			, uDestmarkerID( 0 )
			, uDestJumpToID( 0 )
			, eDestEntryType( 0 )
			, bDestPlayPreEntry( 0 )
			, bIsTransObjectEnabled( 0 )
			, segmentID( 0 )
			, bPlayPreEntry( 0 )
			, bPlayPostExit( 0 )
		{}

		AkUInt32		srcID;    // Source (departure) node ID.
		AkUInt32		destID;   // Destination (arrival) node ID.

		AKBKMusicFade	srcFade;
		AkUInt32		eSrcSyncType;
		AkUInt8			bSrcPlayPostExit;

		AKBKMusicFade	destFade;
		AkUInt32		uDestmarkerID;		// Marker ID. Applies to EntryTypeUserMarker entry type only.
		AkUInt32		uDestJumpToID;		// JumpTo ID (applies to Sequence Containers only).
		AkUInt16		eDestEntryType;		// Entry type. 
		AkUInt8			bDestPlayPreEntry;

		AkUInt8			bIsTransObjectEnabled;
		AkUInt32		segmentID;			// Node ID. Can only be a segment.
		AKBKMusicFade	transFadeIn;
		AKBKMusicFade	transFadeOut;
		AkUInt8			bPlayPreEntry;
		AkUInt8			bPlayPostExit;
	};

	struct AKBKMusicTransNode
	{
		AKBKMusicNode musicNode;
		std::vector<AKBKMusicTransitionRule> transRules;
	};

	struct AKBKMusicMarker
	{
		AkUInt32     markerId;
		AkReal64     fPosition;
	};

	struct AKBKMusicSegment
	{
		AKBKMusicNode musicNode;
		AkReal64 duration;
		std::vector<AKBKMusicMarker> markers;
	//Constructor
		AKBKMusicSegment()
			:duration( 0 )
		{}
	};

	struct AKBKMusicSwitchParams
	{
		AkUInt32		eGroupType;				// Is it binded to state or to switches, 0 = switch, 1 = state, internally defined in enum "AkGroupType"
		AkUInt32		ulGroupID;				// May either be a state group or a switch group
		AkUInt32		ulDefaultSwitch;		// Default value, to be used if none is available, 
												// it may be none, in which case we must play nothing...
		AkUInt8			bContinuePlayback;
	//Constructor
		AKBKMusicSwitchParams()
			:eGroupType( AkGroupType_Switch )
			,ulGroupID( 0 )
			,ulDefaultSwitch( 0 )
			,bContinuePlayback( true )
		{}
	};

	struct AKBKMusicSwitchToNodeAssoc
	{
		AkUInt32 switchID;
		AkUInt32 elementID;
	};

	struct AKBKMusicSwitchCntr
	{
		AKBKMusicTransNode transNode;
		AKBKMusicSwitchParams musicSwitchParams;
		std::vector<AKBKMusicSwitchToNodeAssoc> assocs;
	};

	struct AKBKMusicRanSeqPlaylistItem
	{
		AkUInt32 m_SegmentID;
		AkUInt32 m_playlistItemID;

		AkUInt32 m_NumChildren;
		AkUInt32 m_eRSType;
		AkInt16  m_Loop;
		AkUInt16 m_Weight;
		AkUInt16 m_wAvoidRepeatCount;

		AkUInt8 m_bIsUsingWeight;
		AkUInt8 m_bIsShuffle;
	};

	struct AKBKMusicRanSeq
	{
		AKBKMusicTransNode transNode;
		std::vector<AKBKMusicRanSeqPlaylistItem> playlist;
	//Constructor
		AKBKMusicRanSeq()
		{}
	};

	struct AKBKLayerAssoc
	{
		// Constructor
		AKBKLayerAssoc()
			: ChildID(0)
		{}

		AkUniqueID							ChildID;
		std::vector<AkRTPCCrossfadingPoint>	CrossfadingCurve;
	};

	struct AKBKLayer
	{
		// Constructor
		AKBKLayer()
			: ID(AK_INVALID_UNIQUE_ID)
		{}

		AkUniqueID						ID;
		std::vector<AKBKRTPC>			RTPCList;
		AkRtpcID						CrossfadingRTPCID;
		AkReal32						CrossfadingRTPCDefaultValue;
		std::vector<AKBKLayerAssoc>		AssociatedChildren;
	};

	struct AKBKLayerCntr
	{
		// Constructor
		AKBKLayerCntr()
			:ID(AK_INVALID_UNIQUE_ID)
		{}
		
		AkUniqueID					ID;
		AKBKParameterNode 			ParamNode;
		std::vector<AkUniqueID>		ChildList;
		std::vector<AKBKLayer>		LayerList;
	};

	struct AKBKDuckInfo
	{
		AkUInt32		ulTargetBusID;
		AkReal32 	DuckVolume;
		AkTimeMs 	FadeOutTime;
		AkTimeMs 	FadeInTime;
		AkUInt8		eFadeCurveType;
	//Constructor
		AKBKDuckInfo()
			:DuckVolume(0)
			,FadeOutTime(0)
			,FadeInTime(0)
			,eFadeCurveType(AkCurveInterpolation_Linear)
		{}
	};


	struct AKBKActorMixer
	{
		AkUniqueID	ID;
		AKBKParameterNode 	ParamNode;
		std::vector<AkUniqueID>	ChildList;
	//Constructor
		AKBKActorMixer()
			:ID(AK_INVALID_UNIQUE_ID)
		{}
	};

	struct AKBKActionParams
	{

	#define ACTIONS_PARAMS_SIZE_INCLUDING_LIST( _ActionParamDummy ) ((AkUInt32)( _ActionParamDummy.ExceptionList.size() * sizeof( AkUInt32 ) ) + 33)

		AkTimeMs	TransitionTime;
		AkTimeMs	TransitionTimeModMin;
		AkTimeMs	TransitionTimeModMax;
		AkUInt8		FadeCurveType;

		//Used if it is set pitch or set volume only, if unused, let the default values
			AkUInt32		TargetValueMeaning;

			// TargetVolume or Target Pitch ... Not both of them!!!
			AkReal32		TargetVolume;
			AkReal32		TargetVolumeMin;
			AkReal32		TargetVolumeMax;

			AkUInt32		TargetPitch;
			AkUInt32		TargetPitchMin;
			AkUInt32		TargetPitchMax;

			// Target LFE
			AkReal32		TargetLFE;
			AkReal32		TargetLFEMin;
			AkReal32		TargetLFEMax;

			// Target LPF
			AkReal32		TargetLPF;
			AkReal32		TargetLPFMin;
			AkReal32		TargetLPFMax;

		//Used only if it is a resume or a pause action
			// bool on 32 bits
			AkUInt32		IsMasterResumeOptionEnabled;//may be either master resume or include pausing resume action

		std::vector<AkUniqueID>	ExceptionList;
	//Constructor
		AKBKActionParams()
			:TransitionTime(0)
			,TransitionTimeModMin(0)
			,TransitionTimeModMax(0)
			,FadeCurveType(AkCurveInterpolation_Linear)
			,IsMasterResumeOptionEnabled(0) // false on int
            ,TargetValueMeaning( AkValueMeaning_Default )
			,TargetVolume(0)
			,TargetVolumeMin(0)
			,TargetVolumeMax(0)
            ,TargetPitch(0)
			,TargetPitchMin(0)
			,TargetPitchMax(0)
		{}
	};

	struct AKBKActionSetStatesParams
	{
	#define ACTIONS_SETSTATE_PARAMS_SIZE 8
		AkUInt32		StateGroupID;
		AkUInt32		TargetStateID;
	//Constructor
		AKBKActionSetStatesParams()
			:StateGroupID(0)
			,TargetStateID(0)
		{}
	};

	struct AKBKActionSetSwitchParams
	{
	#define ACTIONS_SETSWITCH_PARAMS_SIZE 8
		AkUInt32		SwitchGroupID;
		AkUInt32		TargetSwitchID;
	//Constructor
		AKBKActionSetSwitchParams()
			:SwitchGroupID(0)
			,TargetSwitchID(0)
		{}
	};

	struct AKBKActionSetRTPCParams
	{
	#define ACTIONS_SETRTPC_PARAMS_SIZE 8
		AkUInt32		RTPCGroupID;
		AkReal32		RTPCValue;
	//Constructor
		AKBKActionSetRTPCParams()
			:RTPCGroupID(0)
			,RTPCValue(0.f)
		{}
	};

	struct AKBKActionBypassFXParams
	{
	#define ACTIONS_BYPASSFX_PARAMS_SIZE( _ActionParamDummy ) ((AkUInt32)( (_ActionParamDummy.ExceptionList.size()+1) * sizeof( AkUInt32 ) ) + 1)
		AkUInt8 bIsBypass;
		AkUInt8 uTargetMask;
		std::vector<AkUniqueID>	ExceptionList;
		AKBKActionBypassFXParams()
			:bIsBypass( true )
			,uTargetMask( 0xFF )
		{}
	};

	struct AKBKAction
	{
		AkUniqueID		ID;
		AkUInt32		eActionType;	// Action type
		AkUniqueID		ElementID;		// Target element ID

		AkFileID		bankFileID;

		AkTimeMs		Delay;
		AkTimeMs		DelayModMin;
		AkTimeMs		DelayModMax;

		AkUInt32		SubSectionSize;


		// SubSections: only one of the following, based on action type
		AKBKActionParams			SpecificParamStd;
			//or
		AKBKActionSetStatesParams	SpecificParamSetState;	// if set state action
			//or
		AKBKActionSetSwitchParams	SpecificParamSetSwitch;	// if set switch action
			//or
		AKBKActionSetRTPCParams		SpecificParamSetRTPC;	// if set RTPC action
			//or
		AKBKActionBypassFXParams	SpecificParamBypassFX;	// if bypass FX action

		//Constructor
		AKBKAction()
			:ID(AK_INVALID_UNIQUE_ID)
			,eActionType(0)
			,ElementID(0)
			,Delay(0)
			,DelayModMin(0)
			,DelayModMax(0)
			,SubSectionSize(0)
			,bankFileID(AK_INVALID_FILE_ID)
		{}
	};

	struct AKBKEvent
	{
		AkUniqueID			ID;
		std::vector<AkUniqueID>	ActionList;
	//Constructor
		AKBKEvent()
			:ID(AK_INVALID_UNIQUE_ID)
		{}
	};

	struct DecisionTreeBankNode : public AkDecisionTree::Node
	{
		bool IsAudioNode;

	//Constructor
		DecisionTreeBankNode()
			: IsAudioNode( false )
		{
			key = AK_INVALID_UNIQUE_ID;
			children.uIdx = 0;
			children.uCount = 0;
		}
	};

	struct AKBKDialogueEvent
	{
		AkUniqueID			ID;

		AkInt32				TreeDepth;	// Is also the size of the ArgumentList
		std::vector<AkUniqueID> ArgumentList;	// Won't add the size of the vector

		AkInt32				DecisionTreeBufferSize;	// Buffer size not number of nodes
		std::vector<DecisionTreeBankNode> DecisionTree;	// Won't add the size of the vector

	//Constructor
		AKBKDialogueEvent()
			:ID(AK_INVALID_UNIQUE_ID)
			,TreeDepth(0)
			,DecisionTreeBufferSize(0)
		{}

	};

	struct AKBKBus
	{
		AkUniqueID			ID;

		AkUniqueID			ParentBusID;

		AkReal32			Volume;
		AkReal32			Lfe;
		AkInt32				Pitch;
		AkLPFType			LPF;
		AkUInt8				bKillNewest;
		AkUInt16			MaxNumInstance;// Zero being no max.
		AkUInt8				bIsMaxNumInstOverrideParent;
		AkUInt8				bIsBackgroundMusic;
		AkUInt8				bEnableWiiCompressor;
		AkUInt32			ulStateGroup;

		AkTimeMs			DuckRecoveryTime;
		AkUInt32			eStateSyncType;
		AkUInt8				bBypassAllFX;

		std::vector<AKBKDuckInfo>	DuckList;
		std::vector<AKBKFX>			FXList;
		std::vector<AKBKRTPC>		RTPCList;
		std::vector<AKBKStateItem>	StateList;
	
		AKBKFeedbackInfo	Feedback;

	//Constructor
		AKBKBus()
			:ID(AK_INVALID_UNIQUE_ID)
			,ParentBusID(AK_INVALID_UNIQUE_ID)
			,Volume(0)
			,Lfe(0)
			,Pitch(0)
			,LPF(0)
			,bKillNewest(false)
			,MaxNumInstance(0)
			,bIsMaxNumInstOverrideParent(false)
			,bIsBackgroundMusic(false)
			,bEnableWiiCompressor(false)
			,ulStateGroup(0)
			,DuckRecoveryTime(0)
			,eStateSyncType( 0 )
		{}
	};

////////////////////////////////////////////////////////
// StateMgr specific structures
////////////////////////////////////////////////////////

	struct AKBKStateTransition
	{
		AkUInt32		StateFrom;
		AkUInt32		StateTo;
		AkTimeMs	TransitionTime;
		AKBKStateTransition()
			:StateFrom( 0 )
			,StateTo( 0 )
			,TransitionTime( 0 )
		{}
	};

	struct AKBKCustomState
	{
		AkUInt32		StateType;

		AKBKState	State;
		AKBKCustomState()
			:StateType( 0 )
		{}
	};

	struct AKBKStateGroup
	{
		AkUInt32		StateGroupID;
		AkTimeMs	DefaultTransitionTime;
		std::vector<AKBKCustomState>		CustomStates;
		std::vector<AKBKStateTransition>	StateTransitions;
		AKBKStateGroup()
			:StateGroupID( AK_INVALID_UNIQUE_ID )
			,DefaultTransitionTime( 0 )
		{}
	};
	
	struct AKBKSwitchRTPCGroup
	{
		AkUInt32		SwitchGroupID;
		AkUInt32		RTPC_ID;
		std::vector<AkRTPCGraphPointInteger> ConversionTable;
		AKBKSwitchRTPCGroup()
			:SwitchGroupID( 0 )
			,RTPC_ID( 0 )
		{}
	};
	
	struct AKBKStateMgr
	{
		AkReal32	VolumeThreshold;
		std::vector<AKBKStateGroup>			StateGroups;
		std::vector<AKBKSwitchRTPCGroup>	SwitchGroups;
		AKBKStateMgr()
		{}
	};

	struct AKBKEnvSettings
	{
		struct CurveData
		{
			CurveData()
				: bUseCurve( false )
				, eScaling( AkCurveScaling_None )
			{}

			bool bUseCurve;
			AkCurveScaling eScaling;
			std::vector<AkRTPCGraphPoint> Curve;
		};

		CurveData Curves[IWProject::MAX_CURVE_X_TYPES][IWProject::MAX_CURVE_Y_TYPES];
	};

}//end namespace "AkBank"

#endif
