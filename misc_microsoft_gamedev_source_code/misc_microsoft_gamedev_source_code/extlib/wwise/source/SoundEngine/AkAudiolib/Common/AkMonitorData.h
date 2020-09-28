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
// AkMonitorData.h
//
// Public structures for monitoring.
//
//////////////////////////////////////////////////////////////////////

#ifndef _MONITORDATA_H_
#define _MONITORDATA_H_

#include "AkPrivateTypes.h"
#include "AkCommon.h"
#include "AkCntrHistory.h"
#include "AkParameters.h"		// AkValueMeaning & Co
#include "AkFeedbackStructs.h"

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/Tools/Common/AkMonitorError.h>

#include <stddef.h> // for offsetof

#define MONITOR_MSG_MAXLENGTH 80
#define MONITOR_GAMEOBJNAME_MAXLENGTH 64

enum AkValueMeaning;

namespace AkMonitorData
{
	typedef AkUInt64 MonitorDataType;

	const MonitorDataType MonitorDataObject				= 0x0000000000000001;
	const MonitorDataType MonitorDataState				= 0x0000000000000002;
	const MonitorDataType MonitorDataParamChanged		= 0x0000000000000004;
	const MonitorDataType MonitorDataBank				= 0x0000000000000008;
	
	const MonitorDataType MonitorDataEventTriggered		= 0x0000000000000010;
	const MonitorDataType MonitorDataActionDelayed		= 0x0000000000000020;
	const MonitorDataType MonitorDataActionTriggered	= 0x0000000000000040;

	const MonitorDataType MonitorDataBusNotif			= 0x0000000000000080;
	const MonitorDataType MonitorDataSetParam			= 0x0000000000000100;

	const MonitorDataType MonitorDataAudioPerf			= 0x0000000000000200; 
	const MonitorDataType MonitorDataGameObjPosition	= 0x0000000000000400; 

	const MonitorDataType MonitorDataObjRegistration	= 0x0000000000000800;

	const MonitorDataType MonitorDataPath				= 0x0000000000001000;
	const MonitorDataType MonitorDataSwitch				= 0x0000000000002000;

	const MonitorDataType MonitorDataPluginTimer		= 0x0000000000004000;
	const MonitorDataType MonitorDataMemoryPool			= 0x0000000000008000;

	const MonitorDataType MonitorDataMemoryPoolName		= 0x0000000000020000;

    const MonitorDataType MonitorDataStreaming			= 0x0000000000040000;
    const MonitorDataType MonitorDataStreamsRecord		= 0x0000000000080000;
    const MonitorDataType MonitorDevicesRecord			= 0x0000000000100000;

	const MonitorDataType MonitorDataPipeline			= 0x0000000000200000;
	const MonitorDataType MonitorDataEnvironment		= 0x0000000000400000;
	const MonitorDataType MonitorDataListeners			= 0x0000000000800000;
	const MonitorDataType MonitorDataObsOcc				= 0x0000000001000000;
	const MonitorDataType MonitorDataMarkers			= 0x0000000002000000;

	const MonitorDataType MonitorDataOutput				= 0x0000000004000000;
	const MonitorDataType MonitorDataSegmentPosition	= 0x0000000008000000;

	const MonitorDataType MonitorDataControllers		= 0x0000000010000000;
	const MonitorDataType MonitorDataRTPCValues			= 0x0000000020000000;

	const MonitorDataType MonitorDataErrorCode			= 0x0000000040000000;
	const MonitorDataType MonitorDataMessageCode		= 0x0000000080000000;

	const MonitorDataType MonitorDataPrepare			= 0x0000000100000000ULL;// ULL required for PS3 compiler to accept long long constant
	
	const MonitorDataType MonitorDataSoundBank			= 0x0000000200000000ULL;// ULL required for PS3 compiler to accept long long constant
	const MonitorDataType MonitorDataMedia				= 0x0000000400000000ULL;// ULL required for PS3 compiler to accept long long constant
	const MonitorDataType MonitorDataEvent				= 0x0000000800000000ULL;// ULL required for PS3 compiler to accept long long constant
	const MonitorDataType MonitorDataGameSync			= 0x0000001000000000ULL;// ULL required for PS3 compiler to accept long long constant
	
	const MonitorDataType MonitorDataFeedback			= 0x0000002000000000ULL;// ULL required for PS3 compiler to accept lon long constant
	const MonitorDataType MonitorDataFeedbackDevices	= 0x0000004000000000ULL;// ULL required for PS3 compiler to accept lon long constant
	const MonitorDataType MonitorDataFeedbackGameObjs	= 0x0000008000000000ULL;// ULL required for PS3 compiler to accept lon long constant
	
	const MonitorDataType MonitorDataErrorString		= 0x0000010000000000ULL;
	const MonitorDataType MonitorDataMessageString		= 0x0000020000000000ULL;

	const MonitorDataType MonitorDataResolveDialogue	= 0x0000040000000000ULL;

	const MonitorDataType AllMonitorData				= 0xFFFFFFFFFFFFFFFFULL;// ULL required for PS3 compiler to accept long long constant

	enum NotificationReason
	{
		NotificationReason_None						= 0,	// No apparent reason
		NotificationReason_Stopped					= 1,	// A stop command was executed
		NotificationReason_StoppedAndContinue		= 2,	// A stop command was executed on something that is not the last of its serie
		NotificationReason_Paused					= 3,	// A pause command was executed
		NotificationReason_Resumed					= 4,	// A resume command was executed
		NotificationReason_Pause_Aborted			= 5,	// A paused sound was stopped
		NotificationReason_EndReached				= 6,	// The sound reached the end and stopped by itself
		NotificationReason_EndReachedAndContinue	= 7,	// The sound reached the end, but another sound is/was/will be launched
		NotificationReason_Play						= 9,	// A PBI just been created and a sound start playing
		NotificationReason_PlayContinue				= 10,	// Specific play following a EndReachedAndContinue
		NotificationReason_Delay_Started			= 11,	// There will be a delay before the next play
		NotificationReason_Delay_Ended				= 12,
		NotificationReason_Delay_Aborted			= 13,	// Signify that a pending action was destroyed without being executed
		NotificationReason_Fade_Started				= 14,	// Sound started fading in
		NotificationReason_Fade_Completed			= 15,	// Sound completed the fade in
		NotificationReason_Fade_Aborted				= 16,	// Sound terminated before transition completed

		NotificationReason_EnterSwitchContPlayback	= 20,// Notif sent to tell there is a continuous playback on a switch container
		NotificationReason_ExitSwitchContPlayback	= 21,// Notif sent to tell there is a continuous playback on a switch container that ended
		NotificationReason_PauseSwitchContPlayback	= 22,// Notif sent to tell there is a continuous playback on a switch container that paused
		NotificationReason_ResumeSwitchContPlayback	= 23,// Notif sent to tell there is a continuous playback on a switch container that resumed
		NotificationReason_PauseSwitchContPlayback_Aborted = 24,// Notif sent to tell there is a continuous playback on a switch container that was stopped while paused

		NotificationReason_NothingToPlay			= 25,// Notif sent to tell that the specified event found nothing to play on a switch container

		NotificationReason_EventEndReached			= 26,// Notif sent to tell that the specified event is finished playing
		NotificationReason_EventMarker				= 27,// Notif sent to tell that a marker was reached while playing the specified event

		NotificationReason_PlayFailed,						// Notification meaning the play asked was not done for an out of control reason
															// For example, if The Element has a missing source file.
		NotificationReason_ContinueAborted,					// The continuity was breached by a no source found in the next item to play
		NotificationReason_PlayFailedLimit,					// Play failed due to playback limit
		NotificationReason_ContinueAbortedLimit,			// Continue aborted due to playback limit

		NotificationReason_StoppedLimit,					// Stopped due to playback limit

		//////////////////////////////////////////////
		// Notifications used in the AkParamsChanged Notification
		//////////////////////////////////////////////
		NotificationReason_Muted			= 100,		// Element is muted
		NotificationReason_Unmuted			= 101,		// Element is unmuted
		NotificationReason_PitchChanged		= 102,		// Element Pitch changed
		NotificationReason_VolumeChanged	= 103,		// Element Volume Changed
		NotificationReason_LFEChanged		= 104,		// Element LFE Changed
		NotificationReason_LPFChanged		= 105,		// Element LPF Changed
		NotificationReason_BypassFXChanged	= 106,		// Element BypassFX Changed
		NotificationReason_FeedbackVolumeChanged	= 107, // Element Feedback Volume Changed

		NotificationReason_BankLoadRequestReceived		= 501,
		NotificationReason_BankUnloadRequestReceived	= 502,
		NotificationReason_BankLoaded					= 503,
		NotificationReason_BankUnloaded					= 504,
		NotificationReason_BankLoadCancelled			= 505,
		NotificationReason_BankUnloadCancelled			= 506,
		NotificationReason_BankLoadFailed				= 507,
		NotificationReason_BankUnloadFailed				= 508,
		NotificationReason_ErrorWhileLoadingBank		= 509,
		NotificationReason_InsuficientSpaceToLoadBank	= 510,
		NotificationReason_BankAlreadyLoaded			= 511,
		NotificationReason_ClearAllBanksRequestReceived	= 512,


		NotificationReason_PrepareEventRequestReceived  = 601,
		NotificationReason_UnPrepareEventRequestReceived= 602,
		NotificationReason_ClearPreparedEventsRequested	= 603,
		NotificationReason_PrepareGameSyncRequested		= 604,
		NotificationReason_UnPrepareGameSyncRequested	= 605,
		NotificationReason_PrepareGameSyncSuccess		= 606,
		NotificationReason_PrepareGameSyncFailure		= 607,
		NotificationReason_UnPrepareGameSyncSuccess		= 608,
		NotificationReason_UnPrepareGameSyncFailure		= 609,
		NotificationReason_EventPrepareSuccess			= 610,
		NotificationReason_EventPrepareFailure			= 611,
		NotificationReason_EventUnPrepareSuccess		= 612,
		NotificationReason_EventUnPrepareFailure		= 613,

		NotificationReason_MusicSegmentStarted			= 801,
		NotificationReason_MusicSegmentEnded			= 802,
	};

	enum BusNotification
	{
		BusNotification_None		= 0,
		BusNotification_Ducked		= 1,
		BusNotification_Unducked	= 2,
		BusNotification_FXBypass	= 3,
		BusNotification_Muted		= 5,
		BusNotification_Unmuted		= 6,
	};

	struct ObjRegistrationMonitorData
	{
		bool			isRegistered;
		AkGameObjectID	gameObjPtr;
		AkUInt16        wStringSize;   // includes terminating NULL
		AkChar			szName[1];     // variable string size
	};

	struct BusNotifMonitorData
	{
		AkUniqueID		busID;
		BusNotification notifReason;
		AkUInt8			bitsFXBypass;	// for FX bypass usage only
		AkUInt8			bitsMask;		// for FX bypass usage only
	};

	struct ObjectMonitorData
	{
		AkPlayingID			playingID;
		AkGameObjectID		gameObjPtr;
		NotificationReason	eNotificationReason;
		AkCntrHistArray		cntrHistArray;		
		AkCustomParamType	customParam;
		AkUniqueID			targetObjectID;
		AkTimeMs			fadeTime;			// Applicable only on NotificationReason_Fade_Started
		AkUniqueID			playlistItemID;		// Actually used by MusicEngine to send playlist item ID.
	};
#define UNKNOWN_FADE_TIME -1

	struct MarkersMonitorData
	{
		AkPlayingID			playingID;
		AkGameObjectID		gameObjPtr;
		NotificationReason	eNotificationReason;
		AkCntrHistArray		cntrHistArray;		
		AkCustomParamType	customParam;
		AkUniqueID			targetObjectID;
		AkUInt16			wStringSize;   // includes terminating NULL
		AkChar				szLabel[1];     // variable string size
	};

	struct StateMonitorData
	{
		AkStateGroupID	stateGroupID;
		AkStateID		stateFrom;
		AkStateID		stateTo;
	};

	struct SwitchMonitorData
	{
		AkSwitchGroupID	switchGroupID;
		AkSwitchStateID	switchState;
		AkGameObjectID	gameObj;
	};

	struct ParamChangedMonitorData
	{
		NotificationReason	eNotificationReason;
		AkGameObjectID		gameObjPtr;
		AkUniqueID			elementID;
	};

	struct SetParamMonitorData
	{
		NotificationReason	eNotificationReason;
		AkGameObjectID		gameObjPtr;
		AkUniqueID			elementID;
		union // Target Value ( for pitch or volume )
		{
			AkVolumeValue	volumeTarget;
			AkPitchValue		pitchTarget;
		};
		AkValueMeaning      valueMeaning;	// Offset or absolute
		AkTimeMs			transitionTime;
	};

	struct ErrorMonitorData1
	{
		AK::Monitor::ErrorCode	eErrorCode;
		AkUInt32				uParam1;
	};

	struct ActionTriggeredMonitorData
	{
		AkPlayingID			playingID;
		AkUniqueID			actionID;
		AkGameObjectID		gameObjPtr;
		AkCustomParamType	customParam;
	};

	struct ActionDelayedMonitorData : public ActionTriggeredMonitorData
	{
		AkTimeMs		delayTime;
	};

	struct EventTriggeredMonitorData
	{
		AkPlayingID			playingID;
		AkUniqueID			eventID;
		AkGameObjectID		gameObjPtr;
		AkCustomParamType	customParam;
	};

	struct BankMonitorData
	{
		NotificationReason	eNotificationReason;
		AkUniqueID			bankID;
		AkUniqueID			languageID;
		AkUInt16            wStringSize;   // includes terminating NULL
		AkChar				szBankName[1]; // variable string size
	};

	struct PrepareMonitorData
	{
		NotificationReason	eNotificationReason;
		AkUInt32			gamesyncIDorEventID;// could use union instead, but would have to add additionnal codes in the ommand data serializer
		AkUInt32			groupID;
		AkGroupType			groupType;
		AkUInt32			uNumEvents;
	};

	struct AkAudioPerfTimers		// Spent time during timing period, in millisecs
	{
		AkReal32 fInterval;			// Length of timing period, in millisecs

		AkReal32 fAudioThread;		// AudioMgr thread (upper engine)
	};

	struct AudioPerfMonitorData
	{
		AkMonitorData::MonitorDataType uiNotifFilter;	// Bitfield, what is currently being sent ; see MonitorDataType
		AkUInt16 numFadeTransitionsUsed;
		AkUInt16 maxFadeNumTransitions;
		AkUInt16 numStateTransitionsUsed;
		AkUInt16 maxStateNumTransitions;
		AkUInt16 numRegisteredObjects;
		AkUInt32 uCommandQueueActualSize;		// size of queue in bytes
		AkReal32 fCommandQueuePercentageUsed;	// percentage of queue used
		AkReal32 fDSPUsage;

		// Banks memory usage, for bank profiling.
		AkUInt32 uNumPreparedEvents;
		AkUInt32 uTotalMemBanks;
		AkUInt32 uTotalPreparedMemory;
		AkUInt32 uTotalMediaMemmory;

		// Perf timers
		AkAudioPerfTimers timers;
	};

	struct GameObjPosition
	{
		AkGameObjectID gameObjID;
		AkSoundPosition position;
	};

	struct ListenerPosition
	{
		AkUInt32 uIndex;
		AkListenerPosition position;
	};

	struct GameObjPositionMonitorData
	{
		AkUInt32 ulNumGameObjPositions;
		AkUInt32 ulNumListenerPositions;

		union Position
		{
			GameObjPosition gameObjPosition;
			ListenerPosition listenerPosition;
		};

		Position positions[1];
	};

	struct DebugMonitorData
	{
		AkUInt16 wStringSize;   // includes terminating NULL
		AkTChar  szMessage[1];  // variable string size
	};

	enum AkPathEvent
	{
		AkPathEvent_Undefined		= 0,
		AkPathEvent_ListStarted		= 1,
		AkPathEvent_VertexReached	= 2
	};

	struct PathMonitorData
	{
		AkPlayingID	playingID;
		AkUniqueID	ulUniqueID;
		AkPathEvent	eEvent;
		AkUInt32		ulIndex;
	};

	struct PluginTimerData
	{
        AkUInt32	uiPluginID;
		AkReal32		fMillisecs;
		AkUInt32	uiNumInstances;
	};

	struct PluginTimerMonitorData
	{
		AkReal32 		fInterval;	   // Length of timing period, in millisecs
		AkUInt32         ulNumTimers;
		PluginTimerData pluginData[1]; // Variable array of size ulNumTimers
	};

    typedef AK::MemoryMgr::PoolStats MemoryPoolData;
	struct MemoryMonitorData
	{
        AkUInt32        ulNumPools;
		MemoryPoolData poolData[1]; // Variable array of size ulNumPools
	};

    struct MemoryPoolNameMonitorData
	{
		AkUInt32	ulPoolId;			// ID of pool
		AkUInt16 wStringSize;       // includes terminating NULL
		AkTChar szName[1]; // max 63 caracters + ending caracter
	};

    // To be sent at connect time.
    typedef AkDeviceDesc DeviceRecordMonitorData;

    // To be sent when a stream is created.
    typedef AkStreamRecord StreamRecord;
    struct StreamRecordMonitorData     
    {
        AkUInt32        ulNumNewRecords; // Number of records.
        StreamRecord   streamRecords[1];// Stream records.
    };

    // Periodic stream stats.
    typedef AkStreamData StreamData;
    struct StreamingMonitorData
    {
    	AkReal32 	   fInterval;		// Length of timing period, in millisecs
        AkUInt32        ulNumStreams;    // Number of streams open
        StreamData     streamData[1];   // Variable array of size ulNumStreams
    };

	struct EnvPacket
	{
		AkGameObjectID		gameObjID;
		AkReal32			fDryValue;
		AkEnvironmentValue	environments[AK_MAX_ENVIRONMENTS_PER_OBJ];
		bool				bypass[AK_MAX_ENVIRONMENTS_PER_OBJ];
	};

	struct ObsOccPacket
	{
		AkGameObjectID		gameObjID;
		AkReal32			fObsValue[AK_NUM_LISTENERS];
		AkReal32			fOccValue[AK_NUM_LISTENERS];
	};

	// Periodic Environment Info.
    struct EnvironmentMonitorData
    {
        AkUInt32		ulNumEnvPacket;	// Number of Environment Packets
        EnvPacket	envPacket[1];	// Variable array of size ulNumEnvPacket
    };

    struct ObsOccMonitorData
    {
        AkUInt32			ulNumPacket;		// Number of Obs/Occ Packets
        ObsOccPacket	obsOccPacket[1];	// Variable array of size ulNumPacket
    };

	struct SpeakerVolumes
	{
		AkReal32 fFrontLeft;				///< Front-Left volume
		AkReal32 fFrontRight;				///< Front-Right volume
		AkReal32 fCenter;					///< Center volume
		AkReal32 fLfe;						///< LFE volume
		AkReal32 fRearLeft;					///< Rear-Left volume
		AkReal32 fRearRight;				///< Rear-Right volume
	};

	struct ListenerPacket
	{
		SpeakerVolumes		VolumeOffset;	// per-speaker volume offset
		AkUInt8				iMotionPlayer ;	// Players associated to the listener for motion purposes (bitfield). 
		bool				bMotion		  ;	// true if it receives motion
		bool				bSpatialized  ;	// false: attenuation only
	};

	struct GameObjectListenerMaskPacket
	{
		AkGameObjectID  gameObject;
		AkUInt32        uListenerMask;
	};

	struct ListenerMonitorData
	{
		ListenerPacket	listeners[AK_NUM_LISTENERS];
		AkUInt32			ulNumGameObjMask;	// Number of GameObjectListenerMaskPacket Packets
		GameObjectListenerMaskPacket gameObjMask[1];
	};

	struct ControllerPacket
	{
		AkReal32	Volume;		// per-controller volume
		bool		bIsActive;	// per-controller activity
	};

	struct GameObjectControllerMaskPacket
	{
		AkGameObjectID  gameObject;
		AkUInt32        uControllerMask; // Bits 0 to 3 stands for controller 0 to 3
	};

#define AK_MAX_NUM_CONTROLLER_MONITORING 4
	struct ControllerMonitorData
	{
		ControllerPacket	controllers[AK_MAX_NUM_CONTROLLER_MONITORING];
		AkUInt32			ulNumGameObjMask;	// Number of GameObjectControllerMaskPacket Packets
		GameObjectControllerMaskPacket gameObjMask[1];
	};

	struct RTPCValuesPacket
	{
		AkUniqueID		rtpcID;	
		AkGameObjectID  gameObjectID;
		AkReal32		value;	
		bool			bHasValue;
	};

	struct RTPCValuesMonitorData
	{
		AkUInt32 ulNumRTPCValues;
		RTPCValuesPacket rtpcValues[1];
	};

    // NOTE: Pipeline data is currently under prototyping. Do not use.
    struct PipelineData
    {
		// A unique ID in time
		AkUniqueID		pipelineID;

        // Init/constant
        AkGameObjectID    gameObjID;
        AkUniqueID      soundID;
        AkUniqueID      mixBusID;

		AkPluginID      fxID[AK_NUM_EFFECTS_PROFILER];		// Sound insert effect. Q: What if more than one?
        AkPluginID      busFxID[AK_NUM_EFFECTS_PROFILER];		// Environmental or bus effect.

		AkUniqueID      feedbackMixBusID;
		AkPluginID      feedbackBusFxID[AK_NUM_EFFECTS_PROFILER];

		AkSrcType       srcType;		// Q: New enum?
        // TODO : File name / source plugin ID / (Bank name?)
        
        // Replace
        AkPriority      priority;
        AkReal32         fVolume;        // Real volume [0,1]
        bool            bIsStarving;
		bool            bIsVirtual;
        
        // Accumulate/Reset
/*
		AkUInt32         ulNumBytesSrc;  // Size read by source since last period (from file/bank slot or produced by plugin)
        AkReal32         fSourceProcTime;    
        AkReal32         fDecompressionTime; // Times in ms
        AkReal32         fFrancoisTime;  // Q: Name
        AkReal32         fMixTime;
        AkReal32         fInsertFXTime;  // Q: What if more than one effect?
        AkReal32         fSendFXTime;    // Q: What if more than one effect?
*/
	};

    struct PipelineMonitorData
    {
    	AkReal32 	    fInterval;		// Length of timing period, in millisecs
        AkUInt16        numPipelines;
		AkUniqueID		masterBusFxId[AK_NUM_EFFECTS_PROFILER];
		AkUniqueID		feedbackMasterBusFxId[AK_NUM_EFFECTS_PROFILER];
		AkUInt16        numPipelinesFeedback;
        PipelineData 	pipelines[1];   // Variable array of size ulNumPipelines
    };

	struct OutputMonitorData
	{
		AkReal32		fPeak;
		AkReal32		fOffset;
		AkReal32		fRMS;
	};

	struct SegmentPositionData
	{
		AkReal64			f64Position;
		AkPlayingID			playingID;
		AkUniqueID			segmentID;
		AkCustomParamType	customParam;
	};

	struct SegmentPositionMonitorData
	{
		AkUInt32			numPositions;
		SegmentPositionData positions[1];
	};

	/////////////////////////////////////////////
	// Bank profiling notifications
	/////////////////////////////////////////////
	struct LoadedSoundBankMonitorData
	{
		AkBankID	bankID;
		AkMemPoolId memPoolID;
		AkUInt32	uBankSize;
		AkUInt32	uNumIndexableItems;
		AkUInt32	uNumMediaItems;
		bool		bIsExplicitelyLoaded;
		bool		bIsDestroyed;
	};

	struct MediaPreparedMonitorData
	{
		AkUniqueID	uMediaID;
		AkUInt32	uMediaSize;

		AkUInt32	uArraySize;// If zero, means not prepared anymore.
		AkBankID	bankIDs[1];// Invalid ID means prepared
	};

	struct EventPreparedMonitorData
	{
		AkUniqueID	eventID;
		AkUInt32	uRefCount;
	};

	struct GameSyncMonitorData
	{
		AkUniqueID	groupID;
		AkUniqueID	syncID;
		AkGroupType eSyncType;	// enum either AkGroupType_Switch or AkGroupType_State
		bool		bIsEnabled; // true = ON, false = OFF
	};
	/////////////////////////////////////////////

	struct FeedbackMonitorData
	{
		AkAudioPerfTimers	timer;
		AkReal32			fPeak;
	};

	struct GameObjectPlayerMaskPacket
	{
		AkGameObjectID  gameObject;
		AkUInt32        uPlayerMask; // Bits 0 to 3 stands for controller 0 to 3
	};

	struct FeedbackGameObjMonitorData
	{
		AkUInt32	ulNumGameObjMask;						// Number of GameObjectPlayerMaskPacket Packets
		GameObjectPlayerMaskPacket gameObjInfo[1];
	};

	struct FeedbackDeviceIDMonitorData
	{
		AkUInt16 usCompanyID;
		AkUInt16 usDeviceID;
		AkUInt8	 ucPlayerActive;					//Bitfield to tell which player are active.  4 bits, one per player.
	};

	struct FeedbackDevicesMonitorData
	{
		AkUInt16 usDeviceCount;
		FeedbackDeviceIDMonitorData deviceIDs[1];	// Dynamic array of device IDs
	};

	struct ResolveDialogueMonitorData
	{
		AkUniqueID			idDialogueEvent;
		AkUniqueID			idResolved;
		AkUInt32			uPathSize;
		AkArgumentValueID	aPath[1];						// Variable-sized
	};
	
	struct MonitorDataItem
	{
		MonitorDataType eDataType;
		AkTimeMs timeStamp;

		union
		{
			ObjectMonitorData			objectData;			// Notif
			StateMonitorData			stateData;			// Notif
			SwitchMonitorData			switchData;			// Notif
			ParamChangedMonitorData		paramChangedData;	// Notif
			MarkersMonitorData			markersData;		// Notif
			
			SetParamMonitorData			setParamData;		// Log only
			ActionTriggeredMonitorData	actionTriggeredData;// Log only
			ActionDelayedMonitorData	actionDelayedData;	// Log only
			EventTriggeredMonitorData	eventTriggeredData;	// Log only

			BankMonitorData				bankData;			// Bank loading
			PrepareMonitorData			prepareData;		// PrepareEvent/PrepareGameSync

			BusNotifMonitorData			busNotifData;		// Bus notification

			AudioPerfMonitorData		audioPerfData;		// Performance information

			GameObjPositionMonitorData	gameObjPositionData;// Game Object position in 3D

			ObjRegistrationMonitorData	objRegistrationData;// Log helper
	
			ErrorMonitorData1			errorData1;			// Error

			DebugMonitorData			debugData;			// Audiolib User Message

			PathMonitorData				pathData;

			PluginTimerMonitorData      pluginTimerData;

			MemoryMonitorData           memoryData;
			MemoryPoolNameMonitorData   memoryPoolNameData;

			EnvironmentMonitorData		environmentData;

			ListenerMonitorData			listenerData;

			ObsOccMonitorData			obsOccData;

            StreamingMonitorData  		streamingData;
            StreamRecordMonitorData     streamRecordData;
            DeviceRecordMonitorData   	deviceRecordData;

			PipelineMonitorData         pipelineData;
			OutputMonitorData			outputData;
			SegmentPositionMonitorData	segmentPositionData;

			ControllerMonitorData		controllerData;
			RTPCValuesMonitorData		rtpcValuesData;

			LoadedSoundBankMonitorData	loadedSoundBankData;
			MediaPreparedMonitorData	mediaPreparedData;
			EventPreparedMonitorData	eventPreparedData;
			GameSyncMonitorData			gameSyncData;
			FeedbackMonitorData			feedbackData;
			FeedbackGameObjMonitorData	feedbackGameObjData;
			FeedbackDevicesMonitorData	feedbackDevicesData;
			ResolveDialogueMonitorData  resolveDialogueData;
		};

	private:
		MonitorDataItem & operator=( const MonitorDataItem & /*in_other*/ )
		{
			// Cannot copy this struct with operator= because of variable size elements ( pluginTimerData, etc. )
			return *this;
		}
	};

	// Return the real size of a MonitorDataItem ( for allocation, copy purpose )
	AKSOUNDENGINE_API AkUInt32 RealSizeof( const MonitorDataItem & in_rItem );

	// The watches define the game objects and listeners that are monitored by the game engine
	// There are several watch types
	enum WatchType
	{
		// NOTE: don't change the IDs because those values are persisted
		WatchType_Unknown = -1,
		WatchType_GameObjectName = 0,
		WatchType_GameObjectID = 1,
		WatchType_ListenerID = 2,
	};

	// WARNING: This structure is persisted into the registry
	// If the content of it is modified, update MonitoringMgrUserPref::k_lWatchVersion
	struct Watch
	{
		Watch(): eType( WatchType_Unknown ), wNameSize( 0 ){ ID.uiListenerID = 0; szName[0] = NULL; }
		WatchType eType;
		union 
		{
			AkUInt32 uiListenerID;
			AkGameObjectID gameObjectID;
		} ID;
		AkUInt16 wNameSize;
		AkChar szName[128];
	};
};


#endif	// _MONITORDATA_H_
