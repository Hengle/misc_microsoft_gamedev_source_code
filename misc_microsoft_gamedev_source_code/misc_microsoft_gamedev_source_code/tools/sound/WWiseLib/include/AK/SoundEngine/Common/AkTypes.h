//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkTypes.h

/// \file 
/// Data type definitions.

#ifndef _AK_DATA_TYPES_H_
#define _AK_DATA_TYPES_H_

// Platform-specific section.
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
// WIN32
//----------------------------------------------------------------------------------------------------
#ifdef WIN32
#include <AK/SoundEngine/Platforms/Windows/AkTypes.h>
#endif

//----------------------------------------------------------------------------------------------------
// XBOX360
//----------------------------------------------------------------------------------------------------
#ifdef XBOX360
#include <AK/SoundEngine/Platforms/XBox360/AkTypes.h>
#endif

//----------------------------------------------------------------------------------------------------
// PS3
//----------------------------------------------------------------------------------------------------
#if defined (__PPU__) || defined (__SPU__)
#include <AK/SoundEngine/Platforms/PS3/AkTypes.h>
#endif

//----------------------------------------------------------------------------------------------------
// Wii
//----------------------------------------------------------------------------------------------------
#ifdef RVL_OS
#include <AK/SoundEngine/Platforms/Wii/AkTypes.h>
#endif

//----------------------------------------------------------------------------------------------------

typedef AkUInt32		AkUniqueID;			 		///< Unique 32-bit ID
typedef AkUInt32		AkStateID;			 		///< State ID
typedef AkUInt32		AkStateGroupID;		 		///< State group ID
typedef AkUInt32		AkPlayingID;		 		///< Playing ID
typedef	AkInt32			AkTimeMs;			 		///< Time in ms
typedef AkInt32			AkPitchValue;		 		///< Pitch value
typedef AkReal32		AkVolumeValue;		 		///< Volume value( also apply to LFE )
typedef AkUInt32		AkGameObjectID;		 		///< Game object ID
typedef AkReal32		AkLPFType;			 		///< Low-pass filter type
typedef AkInt32			AkMemPoolId;		 		///< Memory pool ID
typedef AkUInt32		AkPluginID;			 		///< Source or effect plug-in ID
typedef AkUInt32		AkCodecID;			 		///< Codec plug-in ID
typedef AkUInt32		AkEnvID;			 		///< Environmental ID
typedef AkInt16			AkPluginParamID;	 		///< Source or effect plug-in parameter ID
typedef AkInt8			AkPriority;			 		///< Priority
typedef AkUInt16        AkDataCompID;		 		///< Data compression format ID
typedef AkUInt16        AkDataTypeID;		 		///< Data sample type ID
typedef AkUInt8			AkDataInterleaveID;	 		///< Data interleaved state ID
typedef AkUInt32        AkSwitchGroupID;	 		///< Switch group ID
typedef AkUInt32        AkSwitchStateID;	 		///< Switch ID
typedef AkUInt32        AkRtpcID;			 		///< Real time parameter control ID
typedef AkReal32        AkRtpcValue;		 		///< Real time parameter control value
typedef AkUInt32        AkBankID;			 		///< Run time bank ID
typedef AkUInt32        AkFileID;			 		///< Integer-type file identifier
typedef AkUInt32        AkDeviceID;			 		///< I/O device ID
typedef AkUInt32		AkTriggerID;		 		///< Trigger ID
typedef AkUInt32		AkArgumentValueID;			///< Argument value ID

// Constants.
static const AkPluginID					AK_INVALID_PLUGINID					= (AkPluginID)-1;		///< Invalid FX ID
static const AkPluginID					AK_PLUGINID_ENVIRONMENTAL			= (AkPluginID)-2;		///< FX is Environmental
static const AkGameObjectID				AK_INVALID_GAME_OBJECT				= (AkGameObjectID)-1;	///< Invalid game object (may also mean all game objects)
static const AkUniqueID					AK_INVALID_UNIQUE_ID				=  0;					///< Invalid unique 32-bit ID
static const AkRtpcID					AK_INVALID_RTPC_ID					=  AK_INVALID_UNIQUE_ID;///< Invalid RTPC ID
static const AkUInt32					AK_INVALID_LISTENER_INDEX			= (AkUInt32)-1;			///< Invalid listener index
static const AkPlayingID				AK_INVALID_PLAYING_ID				=  AK_INVALID_UNIQUE_ID;///< Invalid playing ID
static const AkUInt32					AK_DEFAULT_SWITCH_STATE				=  0;					///< Switch selected if no switch has been set yet
static const AkMemPoolId				AK_INVALID_POOL_ID					= -1;					///< Invalid pool ID
static const AkMemPoolId				AK_DEFAULT_POOL_ID					= -1;					///< Default pool ID, same as AK_INVALID_POOL_ID
static const AkEnvID					AK_INVALID_ENV_ID					=  AK_INVALID_UNIQUE_ID;///< Invalid environment ID (or no environment ID)
static const AkFileID					AK_INVALID_FILE_ID					= (AkFileID)-1;			///< Invalid file ID
static const AkDeviceID					AK_INVALID_DEVICE_ID				= (AkDeviceID)-1;		///< Invalid device ID
static const AkBankID					AK_INVALID_BANK_ID					=  AK_INVALID_UNIQUE_ID;///< Invalid bank ID
static const AkArgumentValueID			AK_FALLBACK_ARGUMENTVALUE_ID		=  0;					///< Fallback argument value ID


// Priority.
static const AkPriority        AK_DEFAULT_PRIORITY          = 50;   ///< Default sound / I/O priority
static const AkPriority        AK_MIN_PRIORITY		        = 0;    ///< Minimal priority value [0,100]
static const AkPriority        AK_MAX_PRIORITY		        = 100;  ///< Maximal priority value [0,100]

// Default bank I/O settings.
static const AkPriority     AK_DEFAULT_BANK_IO_PRIORITY = AK_DEFAULT_PRIORITY; 	///<  Default bank load I/O priority
static const AkReal32       AK_DEFAULT_BANK_THROUGHPUT  = 1*1024*1024/1000.f;     	///<  Default bank load throughput (1 Mb/ms)

/// Standard function call result.
enum AKRESULT
{
    AK_NotImplemented			= 0,	///< This feature is not implemented.
    AK_Success					= 1,	///< The operation was successful.
    AK_Fail						= 2,	///< The operation failed.
    AK_PartialSuccess			= 3,	///< The operation succeeded partially.
    AK_NotCompatible			= 4,	///< Incompatible formats
    AK_AlreadyConnected			= 5,	///< The stream is already connected to another node.
    AK_NameNotSet				= 6,	///< Trying to open a file when its name was not set
    AK_InvalidFile				= 7,	///< An unexpected value causes the file to be invalid.
    AK_CorruptedFile			= 8,	///< The file is missing some exprected data.
    AK_MaxReached				= 9,	///< The maximum was reached.
    AK_InputsInUsed				= 10,	///< Inputs are currently used.
    AK_OutputsInUsed			= 11,	///< Outputs are currently used.
    AK_InvalidName				= 12,	///< The name is invalid.
    AK_NameAlreadyInUse			= 13,	///< The name is already in use.
    AK_InvalidID				= 14,	///< The ID is invalid.
    AK_IDNotFound				= 15,	///< The ID was not found.
    AK_InvalidInstanceID		= 16,	///< The InstanceID is invalid.
    AK_NoMoreData				= 17,	///< No more data is available from the source.
    AK_NoSourceAvailable		= 18,	///< There is no child (source) associated with the node.
	AK_StateGroupAlreadyExists	= 19,	///< The StateGroup already exists.
	AK_InvalidStateGroup		= 20,	///< The StateGroup is not a valid channel.
	AK_ChildAlreadyHasAParent	= 21,	///< The child already has a parent.
	AK_TimeSlotAlreadyExists	= 22,	///< The given ID is already in the list.
	AK_CannotAddItseflAsAChild	= 23,	///< It is not possible to add itself as its own child.
	AK_TransitionNotFound		= 24,	///< The transition is not in the list.
	AK_TransitionNotStartable	= 25,	///< Start allowed in the Running and Done states.
	AK_TransitionNotRemovable	= 26,	///< Must not be in the Computing state.
	AK_UsersListFull			= 27,	///< No one can be added any more, could be AK_MaxReached.
	AK_UserAlreadyInList		= 28,	///< This user is already there.
	AK_UserNotInList			= 29,	///< This user is not there.
	AK_NoTransitionPoint		= 30,	///< Not in use.
	AK_InvalidParameter			= 31,	///< Something is not within bounds.
	AK_ParameterAdjusted		= 32,	///< Something was not within bounds and was relocated to the nearest OK value.
	AK_IsA3DSound				= 33,	///< The sound has 3D parameters.
	AK_NotA3DSound				= 34,	///< The sound does not have 3D parameters.
	AK_ElementAlreadyInList		= 35,	///< The item could not be added because it was already in the list.
	AK_PathNotFound				= 36,	///< This path is not known.
	AK_PathNoVertices			= 37,	///< Stuff in vertices before trying to start it
	AK_PathNotRunning			= 38,	///< Only a running path can be paused.
	AK_PathNotPaused			= 39,	///< Only a paused path can be resumed.
	AK_PathNodeAlreadyInList	= 40,	///< This path is already there.
	AK_PathNodeNotInList		= 41,	///< This path is not there.
	AK_VoiceNotFound			= 42,	///< Unknown in our voices list
	AK_DataNeeded				= 43,	///< The consumer needs more.
	AK_NoDataNeeded				= 44,	///< The consumer does not need more.
	AK_DataReady				= 45,	///< The provider has available data.
	AK_NoDataReady				= 46,	///< The provider does not have available data.
	AK_NoMoreSlotAvailable		= 47,	///< Not enough space to load bank.
	AK_SlotNotFound				= 48,	///< Bank error.
	AK_ProcessingOnly			= 49,	///< No need to fetch new data.
	AK_MemoryLeak				= 50,	///< Debug mode only.
	AK_CorruptedBlockList		= 51,	///< The memory manager's block list has been corrupted.
	AK_InsufficientMemory		= 52,	///< Memory error.
	AK_Cancelled				= 53,	///< The requested action was cancelled (not an error).
	AK_UnknownBankID			= 54,	///< Trying to load a bank using an ID which is not defined.
    AK_IsProcessing             = 55,   ///< Asynchronous pipeline component is processing.
	AK_BankReadError			= 56,	///< Error while reading a bank.
	AK_InvalidSwitchType		= 57,	///< Invalid switch type (used with the switch container)
	AK_VoiceDone				= 58,	///< Internal use only.
	AK_UnknownEnvironment		= 59,	///< This environment is not defined.
	AK_EnvironmentInUse			= 60,	///< This environment is used by an object.
	AK_UnknownObject			= 61,	///< This object is not defined.
	AK_NoConversionNeeded		= 62,	///< Audio data already in target format, no conversion to perform.
    AK_FormatNotReady           = 63,   ///< Source format not known yet.
	AK_WrongBankVersion			= 64,	///< The bank version is not compatible with the current bank reader.
	AK_DataReadyNoProcess		= 65,	///< The provider has some data but does not process it (virtual voices).
    AK_FileNotFound             = 66,   ///< File not found.
    AK_DeviceNotReady           = 67,   ///< IO device not ready (may be because the tray is open)
    AK_CouldNotCreateSecBuffer	= 68,   ///< The direct sound secondary buffer creation failed.
	AK_BankAlreadyLoaded		= 69,	///< The bank load failed because the bank is already loaded.
	AK_RenderedFX				= 71,	///< The effect on the node is rendered.
	AK_ProcessNeeded			= 72,	///< A routine needs to be executed on some CPU.
	AK_ProcessDone				= 73,	///< The executed routine has finished its execution.
	AK_MemManagerNotInitialized	= 74,	///< The memory manager should have been initialized at this point.
	AK_StreamMgrNotInitialized	= 75,	///< The stream manager should have been initialized at this point.
	AK_SSEInstructionsNotSupported = 76,///< The machine does not support SSE instructions (required on PC).
	AK_Busy						= 77	///< The system is busy and could not process the request.
};

/// Game sync group type
enum AkGroupType
{
	// should stay set as Switch = 0 and State = 1
	AkGroupType_Switch	= 0, ///< Type switch
	AkGroupType_State	= 1  ///< Type state
};

/// Optional parameter.
struct AkCustomParamType
{
	AkInt64	    customParam;	///< Reserved, must be 0
	AkUInt32	ui32Reserved;	///< Reserved, must be 0
};

/// 3D vector.
struct AkVector
{
    AkReal32		X;	///< X Position
    AkReal32		Y;	///< Y Position
    AkReal32		Z;	///< Z Position
};

/// Positioning information for a sound.
struct AkSoundPosition
{
	AkVector		Position;		///< Position of the sound
	AkVector		Velocity;		///< Velocity of the sound
	AkVector		Orientation;	///< Orientation of the sound
};

/// Positioning information for a listener.
struct AkListenerPosition
{
	AkVector		OrientationFront;	///< Orientation of the listener
	AkVector		OrientationTop;		///< Top orientation of the listener
	AkVector		Position;			///< Position of the listener
	AkVector		Velocity;			///< Velocity of the listener
};

/// Per-speaker volume information for up to 7.1 audio.
struct AkSpeakerVolumes
{
	AkReal32 fFrontLeft;				///< Front-Left volume
	AkReal32 fFrontRight;				///< Front-Right volume
#ifdef AK_LFECENTER
	AkReal32 fCenter;					///< Center volume
	AkReal32 fLfe;						///< LFE volume
#endif
	AkReal32 fRearLeft;					///< Rear-Left volume
	AkReal32 fRearRight;				///< Rear-Right volume
#ifdef AK_71AUDIO
	AkReal32 fExtraLeft;				///< Extra-Left volume
	AkReal32 fExtraRight;				///< Extra-Right volume
#endif
};

#ifdef AK_71AUDIO
#define AK_NUM_SPEAKER_VOLUMES	(8)		///< Number of audio channels
#else
#define AK_NUM_SPEAKER_VOLUMES	(6)		///< Number of audio channels
#endif

#define AK_MAX_ENVIRONMENTS_PER_OBJ				(4) ///< Maximum number of environments in which a single game object may be located at a given time.
#define AK_NUM_LISTENERS						(8)	///< Number of listeners that can be used.

/// Environmental information per game object per given environment.
struct AkEnvironmentValue
{
	AkEnvID EnvID;			///< Unique environment identifier
	AkReal32 fControlValue; ///< Value in the range [0.0f:1.0f], send level to environment
#ifndef AK_OPTIMIZED	
	AkReal32 fUserData;     ///< User data returned by the profiler (opt)
#endif
};

// ---------------------------------------------------------------
// File Type ID Definitions
// ---------------------------------------------------------------

// These correspond to IDs specified in the conversion plug-ins' XML
// files. Audio sources persist them to "remember" their format.
// DO NOT CHANGE THEM without talking to someone in charge of persistence!

// Vendor ID.
#define AKCOMPANYID_AUDIOKINETIC        (0)     ///< Audiokinetic inc.

// File/encoding types of Audiokinetic.
#define AKCODECID_BANK                  (0)		///< Bank encoding
#define AKCODECID_PCM                   (1)		///< PCM encoding
#define AKCODECID_ADPCM                 (2)		///< ADPCM encoding
#define AKCODECID_XMA                   (3)		///< XMA encoding
// Vorbis encoding, this is pluggable so its ID is defined in the Vorbis SDK factory file
//#define AKCODECID_VORBIS                (4)
#define AKCODECID_WIIADPCM              (5)		///< ADPCM encoding on the Wii

/// Registered file source creation function prototype.
typedef	void* (*AkCreateFileSourceCallback)( void* in_pCtx );
/// Registered bank source node creation function prototype.
typedef void* (*AkCreateBankSourceCallback)( void* in_pCtx );

//-----------------------------------------------------------------------------
// Positioning
//-----------------------------------------------------------------------------

/// 3D Positioning type.
enum AkPositioningType
{
	AkUndefined			= 0,	///< Not defined
	Ak2DPositioning		= 1,	///< 2D
	Ak3DUserDef			= 2,	///< 3D user-defined
	Ak3DGameDef			= 3		///< 3D game-defined
};

//----------------------------------------------------------------------------------------------------
// WIN32
//----------------------------------------------------------------------------------------------------
#ifdef WIN32
#include <AK/SoundEngine/Platforms/Windows/AkWinSoundEngine.h>
#endif

//----------------------------------------------------------------------------------------------------
// XBOX360
//----------------------------------------------------------------------------------------------------
#ifdef XBOX360
#include <AK/SoundEngine/Platforms/XBox360/AkXBox360SoundEngine.h>
#endif

//----------------------------------------------------------------------------------------------------
// PS3
//----------------------------------------------------------------------------------------------------
#if defined (__PPU__) || defined (__SPU__)
#include <AK/SoundEngine/Platforms/PS3/AkPs3SoundEngine.h>
#endif

//----------------------------------------------------------------------------------------------------
// Wii
//----------------------------------------------------------------------------------------------------
#ifdef RVL_OS
#include <AK/SoundEngine/Platforms/Wii/AkWiiSoundEngine.h>
#endif

#endif  //_AK_DATA_TYPES_H_
