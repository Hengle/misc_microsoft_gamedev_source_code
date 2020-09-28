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
// AkAudioMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIO_MGR_H_
#define _AUDIO_MGR_H_

#include "AkChunkRing.h"
#include "AkMultiKeyList.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkActionExcept.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkTransition.h"
#include "AkMonitorData.h"			// NotificationReason
#include "AkMonitor.h"
#include "PrivateStructures.h"
#include "AkAudioThread.h"

class CAkPBI;
class CAkEvent;
class CAkDynamicSequence;
class CAkRegisteredObj;

using namespace AKPLATFORM;

namespace AkMonitorData
{
	enum NotificationReason;
}

enum AkQueuedMsgType
{
    QueuedMsgType_EndOfList = 0,
	QueuedMsgType_Event = 1,
	QueuedMsgType_RTPC = 2,
	QueuedMsgType_State = 3,
	QueuedMsgType_Switch = 4,
	QueuedMsgType_Trigger = 5,
	QueuedMsgType_RegisterGameObj = 6,
	QueuedMsgType_UnregisterGameObj = 7,
	QueuedMsgType_GameObjPosition = 8,
	QueuedMsgType_GameObjActiveListeners = 9,
	QueuedMsgType_GameObjActiveControllers = 10,
	QueuedMsgType_ListenerPosition = 11,
	QueuedMsgType_ListenerSpatialization = 12,
	QueuedMsgType_GameObjEnvValues = 13,
	QueuedMsgType_GameObjDryLevel = 14,
	QueuedMsgType_EnvVolume = 15,
	QueuedMsgType_EnvBypass = 16,
	QueuedMsgType_GameObjObstruction = 17,

	QueuedMsgType_ResetSwitches = 18,
	QueuedMsgType_ResetRTPC = 19,

	QueuedMsgType_ControllerVolume = 20,

	QueuedMsgType_OpenDynamicSequence = 21,
	QueuedMsgType_DynamicSequenceCmd = 22,

	QueuedMsgType_KillBank = 23,
	QueuedMsgType_StopAll = 24,

	QueuedMsgType_ListenerPipeline = 25,
	QueuedMsgType_SetPlayerListener = 26,
	QueuedMsgType_AddRemovePlayerDevice = 27,
	QueuedMsgType_SetPlayerVolume = 28,

	QueuedMsgType_StopPlayingID = 29

};

struct AkQueuedMsg_EventBase
{
	AkGameObjectID		GameObjID;		// Associated game object
	AkPlayingID			PlayingID;		// Playing ID
	AkCustomParamType	CustomParam;
};

struct AkQueuedMsg_Event
	: public AkQueuedMsg_EventBase
{
	CAkEvent*			Event;			// Pointer to the event
};

struct AkQueuedMsg_Rtpc
{
	AkRtpcID		ID;
	AkRtpcValue		Value;
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_State
{
	AkStateGroupID  StateGroupID;
	AkStateID		StateID;
	bool			bSkipTransition;
    bool			bSkipExtension;
};

struct AkQueuedMsg_Switch
{
	AkGameObjectID	GameObjID;		// Associated game object
	AkSwitchGroupID SwitchGroupID;
	AkSwitchStateID SwitchStateID;
};

struct AkQueuedMsg_Trigger
{
	AkGameObjectID	GameObjID;		// Associated game object
	AkTriggerID		TriggerID;
};

struct AkQueuedMsg_RegisterGameObj
{
	AkGameObjectID	GameObjID;
	void *			pMonitorData; // Monitor data, allocated in game thread, to be used in audio thread if registration successful
};

struct AkQueuedMsg_UnregisterGameObj
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_GameObjPosition
{
	AkGameObjectID	GameObjID;
	AkSoundPosition Position;
	AkUInt32		uListenerIndex;
};

struct AkQueuedMsg_GameObjActiveListeners
{
	AkGameObjectID	GameObjID;
	AkUInt32		uListenerMask;
};

struct AkQueuedMsg_GameObjActiveControllers
{
	AkGameObjectID	GameObjID;
	AkUInt32		uActiveControllerMask;
};

struct AkQueuedMsg_ListenerPosition
{
	AkUInt32		uListenerIndex;
	AkListenerPosition Position;
};

struct AkQueuedMsg_ListenerSpatialization
{
	AkUInt32		uListenerIndex;
	AkSpeakerVolumes Volumes;
	bool            bSpatialized;
	bool            bSetVolumes;
};

struct AkQueuedMsg_ListenerPipeline
{
	AkUInt32		uListenerIndex;
	bool			bAudio;
	bool			bFeedback;
};

struct AkQueuedMsg_SetPlayerListener
{
	AkUInt8		iPlayer;
	AkUInt8		iListener;
	AkUInt16	DummyForAlignment;	//Remove if size becomes a multiple of 4.  See AkChunkRing.cpp line 235
};

struct AkQueuedMsg_SetPlayerVolume
{
	AkReal32	fVolume;
	AkUInt32	iPlayer;	//32 bits for alignment.  See AkChunkRing.cpp line 235
};

struct AkQueuedMsg_AddRemovePlayerDevice
{
	AkUInt8		iPlayer;
	AkUInt16	idCompany;
	AkUInt16	idDevice;
	bool		bAdd;
};

struct AkQueuedMsg_GameObjEnvValues
{
	AkGameObjectID	GameObjID;
	AkUInt32		uNumValues;
	AkEnvironmentValue EnvValues[AK_MAX_ENVIRONMENTS_PER_OBJ];
};

struct AkQueuedMsg_GameObjDryLevel
{
	AkGameObjectID	GameObjID;
	AkReal32		fValue;
};

struct AkQueuedMsg_EnvVolume
{
	AkEnvID			EnvID;
	AkReal32		fVolume;
};

struct AkQueuedMsg_EnvBypass
{
	AkEnvID			EnvID;
	bool			bBypass;
};

struct AkQueuedMsg_GameObjObstruction
{
	AkGameObjectID	GameObjID;
	AkUInt32		uListenerIndex;
	AkReal32		fObstructionLevel;
	AkReal32		fOcclusionLevel;
};

struct AkQueuedMsg_ResetSwitches
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_ResetRTPC
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_ControllerVolume
{
	AkUInt32	uControllerID;
	AkReal32	fControllerVolume;
};

struct AkQueuedMsg_OpenDynamicSequence
	: public AkQueuedMsg_EventBase
{
	CAkDynamicSequence*	pDynamicSequence;
};

class CAkUsageSlot;

struct AkQueuedMsg_KillBank
{
	CAkUsageSlot* pUsageSlot;
};

struct AkQueuedMsg_StopAll
{
	AkGameObjectID	GameObjID;
};

struct AkQueuedMsg_StopPlayingID
{
	AkPlayingID playingID;
};

struct AkQueuedMsg_DynamicSequenceCmd
{
	enum Command
	{
		Play,
		Pause,
		Resume,
		Close,
		Stop,
		Break,
		ResumeWaiting,
	};

	CAkDynamicSequence*			pDynamicSequence;
	Command						eCommand;
};

#pragma pack(push, 1)
struct AkQueuedMsg
{
	AkUInt16 size;
	AkUInt16 type;

	union
	{
        AkQueuedMsg_Event	event;
		AkQueuedMsg_Rtpc	rtpc;
		AkQueuedMsg_State   setstate;
		AkQueuedMsg_Switch	setswitch;
		AkQueuedMsg_Trigger trigger;
		AkQueuedMsg_RegisterGameObj reggameobj;
		AkQueuedMsg_UnregisterGameObj unreggameobj;
		AkQueuedMsg_GameObjPosition gameobjpos;
		AkQueuedMsg_GameObjActiveListeners gameobjactlist;
		AkQueuedMsg_GameObjActiveControllers gameobjactcontroller;
		AkQueuedMsg_ListenerPosition listpos;
		AkQueuedMsg_ListenerSpatialization listspat;
		AkQueuedMsg_GameObjEnvValues gameobjenvvalues;
		AkQueuedMsg_GameObjDryLevel gameobjdrylevel;
		AkQueuedMsg_EnvVolume envvolume;
		AkQueuedMsg_EnvBypass envbypass;
		AkQueuedMsg_GameObjObstruction gameobjobstr;
		AkQueuedMsg_ResetSwitches resetswitches;
		AkQueuedMsg_ResetRTPC resetrtpc;
		AkQueuedMsg_ControllerVolume controllervolume;
		AkQueuedMsg_OpenDynamicSequence opendynamicsequence;
		AkQueuedMsg_DynamicSequenceCmd dynamicsequencecmd;
		AkQueuedMsg_KillBank killbank;
		AkQueuedMsg_ListenerPipeline listpipe;
		AkQueuedMsg_SetPlayerListener playerlistener;
		AkQueuedMsg_SetPlayerVolume playervolume;
		AkQueuedMsg_AddRemovePlayerDevice playerdevice;
		AkQueuedMsg_StopAll stopAll;
		AkQueuedMsg_StopPlayingID stopEvent;
	};

	static AkForceInline AkUInt16 Sizeof_EndOfList() { return offsetof( AkQueuedMsg, event ); }
	static AkForceInline AkUInt16 Sizeof_Event() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_Event ); }
	static AkForceInline AkUInt16 Sizeof_Rtpc() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_Rtpc ); }
	static AkForceInline AkUInt16 Sizeof_State() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_State ); }
	static AkForceInline AkUInt16 Sizeof_Switch() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_Switch ); }
	static AkForceInline AkUInt16 Sizeof_Trigger() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_Trigger ); }
	static AkForceInline AkUInt16 Sizeof_RegisterGameObj() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_RegisterGameObj ); }
	static AkForceInline AkUInt16 Sizeof_UnregisterGameObj() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_UnregisterGameObj ); }
	static AkForceInline AkUInt16 Sizeof_GameObjPosition() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjPosition ); }
	static AkForceInline AkUInt16 Sizeof_GameObjActiveListeners() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjActiveListeners ); }
	static AkForceInline AkUInt16 Sizeof_GameObjActiveControllers() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjActiveControllers ); }
	static AkForceInline AkUInt16 Sizeof_ListenerPosition() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ListenerPosition ); }
	static AkForceInline AkUInt16 Sizeof_ListenerSpatialization() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ListenerSpatialization ); }
	static AkForceInline AkUInt16 Sizeof_GameObjEnvValues() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjEnvValues ); }
	static AkForceInline AkUInt16 Sizeof_GameObjDryLevel() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjDryLevel ); }
	static AkForceInline AkUInt16 Sizeof_EnvVolume() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_EnvVolume); }
	static AkForceInline AkUInt16 Sizeof_EnvBypass() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_EnvBypass); }
	static AkForceInline AkUInt16 Sizeof_GameObjObstruction() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_GameObjObstruction); }
	static AkForceInline AkUInt16 Sizeof_ResetSwitches() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ResetSwitches); }
	static AkForceInline AkUInt16 Sizeof_ResetRTPC() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ResetRTPC); }
	static AkForceInline AkUInt16 Sizeof_ControllerVolume() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ControllerVolume); }
	static AkForceInline AkUInt16 Sizeof_OpenDynamicSequence() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_OpenDynamicSequence ); }
	static AkForceInline AkUInt16 Sizeof_DynamicSequenceCmd() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_DynamicSequenceCmd ); }
	static AkForceInline AkUInt16 Sizeof_KillBank() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_KillBank ); }
	static AkForceInline AkUInt16 Sizeof_StopAll() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_StopAll ); }
	static AkForceInline AkUInt16 Sizeof_ListenerPipeline() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_ListenerPipeline ); }
	static AkForceInline AkUInt16 Sizeof_SetPlayerListener(){ return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_SetPlayerListener ); }
	static AkForceInline AkUInt16 Sizeof_SetPlayerVolume() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_SetPlayerVolume ); }
	static AkForceInline AkUInt16 Sizeof_AddRemovePlayerDevice(){ return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_AddRemovePlayerDevice ); }
	static AkForceInline AkUInt16 Sizeof_StopPlayingID() { return offsetof( AkQueuedMsg, event ) + sizeof( AkQueuedMsg_StopPlayingID ); }
};

#pragma pack(pop)

#ifdef __PPU__

#define AK_EVENT_MGR_THREAD_STACK_BYTES	(16384)

#endif

// all what we need to start an action at some later time
struct AkPendingAction :	public CAkObject,
							public ITransitionable 
							
{
	CAkAction*			pAction;				// the action itself
	AkUInt32			LaunchTick;				// when it should run
	AkUInt32			LaunchFrameOffset;		// frame offset
	AkUInt32			PausedTick;				// when it got paused
	UserParams			UserParam;
	AkUInt32			ulPauseCount;			// Pause Count

	AkPendingAction( CAkRegisteredObj * in_pGameObj );
	~AkPendingAction();

	AkGameObjectID GameObjID();
	CAkRegisteredObj * GameObj() { return pGameObj; }

	virtual void TransUpdateValue( TransitionTargetTypes in_eTargetType, TransitionTarget in_unionValue, bool in_bIsTerminated );

private:
	CAkRegisteredObj	* pGameObj;	// Game object pointer, made private to enforce refcounting
};

//CAkAudioThread class
//This class contains what is specificly associated to the audiothread
class CAkAudioMgr : public CAkObject
{
	friend class CAkAudioThread;

public:
	//Constructor
	CAkAudioMgr();
	//Destructor
	virtual ~CAkAudioMgr();

	AKRESULT	Init();
	void		Term();

	//Start the AudioThread
	AKRESULT Start();

	//Stop the AudioThread 
	void Stop();

	//Enqueues an End-of-list item if there are events to process and wakes up audio thread.
	AKRESULT RenderAudio();
	
	AkForceInline void WakeupEventsConsumer() { return m_audioThread.WakeupEventsConsumer(); }

	//Add the specified item
	AKRESULT Enqueue( AkQueuedMsg& in_rItem, AkUInt32 in_uSize );

	AkForceInline void IncrementBufferTick()
	{ 
		++m_uBufferTick; 
#ifndef AK_OPTIMIZED
		AkMonitor::SetThreadTime( (AkTimeMs) ( (AkReal64) m_uBufferTick * 1000.0 * AK_NUM_VOICE_REFILL_FRAMES / AK_CORE_SAMPLERATE ) ); 
#endif
	}
	AkUInt32 GetBufferTick(){ return m_uBufferTick; }

	template< typename T >
	typename T::IteratorEx FlushPendingItem( AkPendingAction* in_pPA, T & in_List, typename T::IteratorEx in_Iter )
	{
		in_pPA->pAction->Release();
		AkDelete( g_DefaultPoolId, in_pPA );
		return in_List.Erase( in_Iter );
	}

	AKRESULT BreakPendingAction( AkUniqueID in_TargetID, CAkRegisteredObj * in_GameObj );

	AKRESULT StopPendingAction( AkUniqueID in_TargetID, CAkRegisteredObj * in_GameObj );

	AKRESULT StopPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList );

	// move actions from the pending list to the paused pending list
	AKRESULT PausePendingAction(AkUniqueID in_ulElementID, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume );
	AKRESULT PausePendingItems( AkPlayingID in_PlayingID );

	// move actions from the paused pending list to the pending list
	AKRESULT ResumePausedPendingAction(AkUniqueID in_ulElementID, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume );
	AKRESULT ResumePausedPendingItems( AkPlayingID in_playingID );

	// Cancel eventual pause transition in the pending list( not paused pending)
	AKRESULT ResumeNotPausedPendingAction(AkUniqueID in_ulElementID, CAkRegisteredObj * in_GameObj, bool in_bIsMasterOnResume );

	AKRESULT PausePendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume );

	AKRESULT ResumePausedPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume );

	// Cancel eventual pause transition in the pending list( not paused pending)
	AKRESULT ResumeNotPausedPendingActionAllExcept( CAkRegisteredObj * in_GameObj, ExceptionList* in_pExceptionList, bool in_bIsMasterOnResume );

	// remove some actions from the pending list
	AKRESULT RemovePendingAction(AkUniqueID in_ulElementID);

	// remove some actions from the paused pending list
	AKRESULT RemovePausedPendingAction(AkUniqueID in_ulElementID);

	AKRESULT PausePending( AkPendingAction* in_pPA );
	AKRESULT StopPending( AkPendingAction* in_pPA );

	// figure out if anything has to come out of this list
	void ProcessPendingList();

	void RemoveAllPausedPendingAction();
	void RemoveAllPendingAction();
	void RemoveAllPreallocAndReferences();

	void InsertAsPaused( AkUniqueID in_ElementID, AkPendingAction* in_pPendingAction, AkUInt32 in_ulPauseCount = 0 );
	void TransferToPending( AkPendingAction* in_pPendingAction );

	// This function return a pointer to the first action that is pending that corresponds to the
	// specified Playing ID. This is used to Recycle the content of a playing instance that
	// currently do not have PBI.
	//
	// Return - AkPendingAction* - pointer to the first action that is pending that corresponds to the
	// specified Playing ID. NULL if not found.
	AkPendingAction* GetActionMatchingPlayingID(
		AkPlayingID in_PlayingID	// Searched Playing ID
		);

	// Force the destruction of pending items associated to the specified PlayingID.
	void ClearPendingItems(
		AkPlayingID in_PlayingID
		);

	// Force the destruction of pending items associated to the specified PlayingID, but one of them do not return Abort
	void ClearPendingItemsExemptOne(
		AkPlayingID in_PlayingID
		);

	void ClearCrossFadeOccurence(
		CAkPBI* in_pPBIToCheck
		);

	// Execute an action or enqueue it in the pending queue depending if there is a delay
	void EnqueueOrExecuteAction(
		AkPendingAction* in_pActionItem
		);

	// PhM : these are used at least in AkActionEvent regardless of the definition of AK_OPTIMIZED
//#ifndef AK_OPTIMIZED
	AKRESULT StopAction(AkUniqueID in_ActionID);
	AKRESULT PauseAction(AkUniqueID in_ActionID);
	AKRESULT ResumeAction(AkUniqueID in_ActionID);
//#endif

	// the total size of the queue (in bytes)
	AkUInt32 GetActualQueueSize() {return m_MsgQueueActualSize;}
	
	// 0.0 -> 1.0 (returns the greatest value since the previous query)
	AkReal32 GetPercentageQueueFilled() 
	{AkReal32 tmp = m_MsgQueuePercentageFilled; m_MsgQueuePercentageFilled = 0.0f; return tmp;}	
private:

	void Perform();

	// Add an action in the delayed action queue
	
	void FlushAndCleanPendingAction( 
		AkPendingAction* in_pPendingAction 
		);

	CAkLock m_queueLock;

	// the messages we get from the game side
	AkSparseChunkRing m_MsgQueue;

	// when set drain the message queue
	bool m_bDrainMsgQueue;

	// actual size of the message queue
	AkUInt32 m_MsgQueueActualSize;

	// how much of the queue is filled
	AkReal32 m_MsgQueuePercentageFilled;

	// the things that are not ready to be done
	typedef CAkMultiKeyList<AkUInt32, AkPendingAction*, AkAllocAndKeep> AkMultimapPending;
	AkMultimapPending m_mmapPending;

	// the things that are not ready to be done that got paused
	typedef CAkMultiKeyList<AkUniqueID, AkPendingAction*, AkAllocAndKeep> AkMultimapPausedPending;
	AkMultimapPausedPending m_mmapPausedPending;

	// Time actually in use by the Audio Mgr
	AkUInt32 m_uBufferTick;

	AkUInt32 m_ulWriterFlag; //Counter incremented at everytime the RenderAudio() function is called
	AkUInt32 m_ulReaderFlag; //Counter incremented at everytime the audiothread reaches an EOL element in the queue

	// Take all the Events in the Event queue untill it reaches 
	// the End Of List flag and process them all
	//
	// Return - AkResult - Ak_Success if succeeded
	AKRESULT ProcessMsgQueue();

	// execute those ready
	void ProcessAction( AkPendingAction * in_pAction );

	// Monitor notif
	void NotifyDelayStarted(
		AkPendingAction* in_pPending//Pointer to the pending action that was delayed
		);

	// Monitor notif
	void NotifyDelayAborted(
		AkPendingAction* in_pPending, //Pointer to the pending action that was aborted
		bool in_bWasPaused
		);

	// Monitor notif
	void NotifyDelayEnded(
		AkPendingAction* in_pPending, //Pointer to the pending action that just ended
		bool in_bWasPaused = false
		);

	void NotifyImminentAborted( 
		AkPendingAction* in_pPending 
		);

	void NotifyDelay(
		AkPendingAction* in_pPending, 
		AkMonitorData::NotificationReason in_Reason,
		bool in_bWasPaused
		);

	//Internal tool, allow to know it the specified action should be excepted by the call.
	bool IsAnException( CAkAction* in_pAction, ExceptionList* in_pExceptionList );

	bool IsElementOf( AkUniqueID in_TargetID, AkUniqueID in_IDToCheck );

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

	CAkAudioThread	m_audioThread;
	AkEvent			m_hEventMgrThreadDrainEvent;

};

extern CAkAudioMgr* g_pAudioMgr;
#endif
