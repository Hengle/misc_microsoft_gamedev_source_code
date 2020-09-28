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
// AkAudioNode.h
//
// Declaration of the CAkAudioNode.  This is a virtual base class
// for all audio component that process audio samples
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIO_NODE_H_
#define _AUDIO_NODE_H_

#include "AkAudioLibExport.h"
#include "AkParams.h"
#include "AkPBIAware.h"
#include "AkRTPC.h"
#include "PrivateStructures.h"
#include "AkActionExcept.h"


class CAkAudioNode;
class CAkParameterNode;
class CAkContinuationList;
class CAkBus;
class CAkFeedbackBus;

struct TransParams;
struct UserParams;
struct PlayHistory;
struct AkMutedMapItem;
struct AkRTPCFXSubscription;
struct AkObjectInfo;

enum AkNodeCategory
{
	AkNodeCategory_Bus				= 0,	// The node is a bus
	AkNodeCategory_ActorMixer		= 1,	// The node is an actro-mixer
	AkNodeCategory_RanSeqCntr		= 2,	// The node is a Ran/Seq container
	AkNodeCategory_Sound			= 3,	// The node is a sound
	AkNodeCategory_SwitchCntr		= 4,	// The node is a Switch container
	AkNodeCategory_LayerCntr		= 5,	// The node is a Layer container


    AkNodeCategory_MusicTrack		= 6,	// The node is a Music Track
    AkNodeCategory_MusicSegment		= 7,	// The node is a Music Segment
    AkNodeCategory_MusicRanSeqCntr	= 8,	// The node is a Music Multi-Sequence container
    AkNodeCategory_MusicSwitchCntr	= 9,	// The node is a Music Switch container
	AkNodeCategory_FeedbackBus		= 10,	// The node is a feedback device bus
	AkNodeCategory_FeedbackNode		= 11,	// The node is a feedback multi-source node

	AkNodeCategory_None				= 1000,
};

struct AkInput
{
	AkUniqueID		ulID;				//ID of the subNode
	CAkAudioNode*	pNode;				//Pointer to the subNode 
};

enum ActionParamType
{
	ActionParamType_Stop	= 0,
	ActionParamType_Pause	= 1,
	ActionParamType_Resume	= 2,
	ActionParamType_Break	= 3  // == PlayToEnd
};

struct ActionParams
{
	ActionParamType eType;
	CAkRegisteredObj * pGameObj;
	AkPlayingID		playingID;
	TransParams     transParams;
	bool			bIsFromBus;
	bool			bIsMasterCall;
	bool			bIsMasterResume;
	AkUniqueID		targetNodeID; // usually not initialised uselessly
};

struct ActionParamsExcept
{
	ActionParamType eType;
	ExceptionList*  pExeceptionList;
	CAkRegisteredObj * pGameObj;
    TransParams     transParams;
	bool			bIsFromBus;
	bool			bIsMasterResume;
};

enum NotifParamType
{
	NotifParamType_Volume		= 0,
	NotifParamType_Pitch		= 1,
	NotifParamType_LPF			= 2,
	NotifParamType_LFE			= 3,
	NotifParamType_FeedbackVolume = 4,
	NotifParamType_FeedbackLPF	= 5,
	NotifParamType_FeedbackBusPitch = 6
};

struct NotifParams
{
	NotifParamType	eType;
	CAkRegisteredObj * pGameObj;
	bool			bIsFromBus;
	void*			pExceptObjects;
	union
	{
		AkVolumeValue	volume;
		AkPitchValue		pitch;
		AkLPFType		LPF;
		AkVolumeValue	LFE;
		AkVolumeValue	FeedbackVolume;
		AkLPFType		FeedbackLPF;
	}UnionType;
};

// class corresponding to an audionode
//
// Author:  ebegin
class CAkAudioNode : public CAkPBIAware
{
	friend class CAkAudioNodeUT;

public:

	// Constructors
    CAkAudioNode(AkUniqueID in_ulID);

    //Destructor
    virtual ~CAkAudioNode(void);

	AKRESULT Init();

	//Return the Node Category
	//
	// Return - AkNodeCategory - Type of the node
	virtual AkNodeCategory NodeCategory() = 0;

	// PlayAndContinue the specified node
	// Does the same as the Play() does, but have more parameters telling that the play
	// passed by a Continuous container and the PBI will have to launch another actions
	// at a given time.
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams ) = 0;

	AKRESULT Stop( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );
	AKRESULT Pause( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );
	AKRESULT Resume( CAkRegisteredObj * in_pGameObj = NULL, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID );

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction ) = 0;
	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction ) = 0;

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj, AkUniqueID in_NodeID, AkPlayingID in_PlayingID = AK_INVALID_PLAYING_ID ) = 0;

	// Set the parent of the AudioNode 
    //
    // Return - CAkAudioNode* - true if Element is playing, else false
	virtual void Parent(CAkAudioNode* in_pParent);

	CAkAudioNode* Parent() { return m_pParentNode; }

	virtual void ParentBus(CAkAudioNode* in_pParentBus);

	CAkAudioNode* ParentBus() { return m_pBusOutputNode; }

	// Adds the Node in the General indes
	void AddToIndex();

	// Removes the Node from the General indes
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	virtual void ParamNotification( NotifParams& in_rParams ) = 0;

	// Notify the Children PBIs that a PITCH variation occured
	virtual void PitchNotification(
		AkPitchValue in_Pitch,										// Pitch variation
		CAkRegisteredObj * in_pGameObj = NULL,		// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a VOLUME variation occured
	virtual void VolumeNotification(
		AkVolumeValue in_Volume,									// Volume variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);
		
	// Notify the Children PBIs that a LPF variation occured
	void LPFNotification(
		AkLPFType in_LPF,										// LPF variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);
		
	// Notify the Children PBIs that a LFE variation occured
	void LFENotification(
		AkVolumeValue in_LFE,									// LFE variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a FeedbackVolume variation occured
	void FeedbackVolumeNotification(
		AkVolumeValue in_FeedbackVolume,		// Feedback Volume variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a FeedbackPitch variation occured
	void FeedbackPitchNotification(
		AkPitchValue in_FeedbackPitch,		// Feedback Pitch variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a FeedbackLPF variation occured
	void FeedbackLPFNotification( 
		AkLPFType in_FeedbackLPF,				// Feedback LPF variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL 
		);

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkUInt8			in_cMuteLevel,	// New muting level
		AkMutedMapItem& in_rMutedItem,	// Node identifier
		bool			in_bIsFromBus = false
		)=0;

	// Notify the children PBI that a mute variation occured
	virtual void MuteNotification(
		AkUInt8 in_cMuteLevel,			// New muting level
		CAkRegisteredObj * in_pGameObj,		// Target Game Object
		AkMutedMapItem& in_rMutedItem,	// Node identifier
		bool in_bPrioritizeGameObjectSpecificItems = false
		)=0;

	// Notify the children PBI that a change int the positioning parameters occured from RTPC
	virtual void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// Value
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		)=0;

	// Notify the children PBI that a major change occured and that the 
	// Parameters must be recalculated
	virtual void RecalcNotification()=0;

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_pGameObj = NULL,
		void* in_pExceptArray = NULL
		) = 0;

	virtual void NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID ) = 0;
	virtual void UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem ) = 0;

#ifndef AK_OPTIMIZED
	virtual void InvalidatePaths() = 0;
#endif

	// Notify the Children that the game object was unregistered
	virtual void Unregister(
		CAkRegisteredObj * in_pGameObj
		);

	virtual AKRESULT AddChild(
        AkUniqueID in_ulID          // Input node ID to add
		);

    virtual AKRESULT RemoveChild(
        AkUniqueID in_ulID          // Input node ID to remove
		);

	virtual AKRESULT RemoveAllChildren();

	virtual AKRESULT GetChildren( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos, AkUInt32& index_out, AkUInt32 iDepth );

#define AK_ForwardToBusType_Normal (1)
#define AK_ForwardToBusType_Motion (2)
#define AK_ForwardToBusType_ALL (AK_ForwardToBusType_Normal | AK_ForwardToBusType_Motion)

	// Used to increment/decrement the playcount used for notifications and ducking
	virtual bool IncrementPlayCount(
		AkPriority in_Priority, 
		CAkRegisteredObj * in_GameObj,
		AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL,
		AkUInt16 in_ui16NumKicked = 0,
		bool in_bMaxConsidered = false
		) = 0;

	virtual void DecrementPlayCount(
		CAkRegisteredObj * in_GameObj,
		AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL,
		bool in_bMaxConsidered = false
		) = 0;

	// Gets the Next Mixing Bus associated to this node
	//
	// RETURN - CAkBus* - The next mixing bus pointer.
	//						NULL if no mixing bus found
	virtual CAkBus* GetMixingBus();

	virtual CAkBus* GetLimitingBus();

#ifndef AK_OPTIMIZED
	virtual void UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32			in_ulParamBlockSize
		) = 0;

	virtual void StopMixBus(){}
#endif

	virtual bool Has3DParams();

	bool IsException( AkUniqueID in_ID, ExceptionList& in_rExceptionList );

	bool	IsPlaying(){ return ( m_PlayCount != 0 ); } // This function was NOT created as an helper to get information about if something is playing, 
														// but instead have been created to avoid forwarding uselessly notifications trough the tree.

	AkUInt16 GetPlayCount(){ return m_PlayCount; }

	virtual AKRESULT PrepareData() = 0;
	virtual void UnPrepareData() = 0;

	static AKRESULT PrepareNodeData( AkUniqueID in_NodeID );
	static void UnPrepareNodeData( AkUniqueID in_NodeID );

	virtual void IncrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL ) = 0;
	virtual void DecrementActivityCount( AkUInt16 in_flagForwardToBus = AK_ForwardToBusType_ALL ) = 0;

protected:
	
	bool IsActive(){ return m_uActivityCount != 0; }
	bool IsActiveOrPlaying(){ return IsPlaying() || IsActive(); }

protected: //members

	CAkAudioNode*	m_pParentNode;		// Hirc Parent (optional), bus always have NULL
	CAkAudioNode*	m_pBusOutputNode;	// Bus Parent (optional)

	AkUInt16 m_PlayCount;				//Data path count
	AkUInt16 m_uActivityCount;			//Count for StateTransitions and SwitchPlayback, and music playback.
}; 
#endif
