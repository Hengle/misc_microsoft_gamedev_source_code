//////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKMONITORERROR_H
#define _AKMONITORERROR_H

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{
    // Error monitoring.

	namespace Monitor
	{
		///  ErrorLevel
		enum ErrorLevel
		{
			ErrorLevel_Message	= (1<<0), // used as bitfield
			ErrorLevel_Error	= (1<<1),
			
			ErrorLevel_All = ErrorLevel_Message | ErrorLevel_Error
		};
		/// ErrorCode
		enum ErrorCode
		{
			ErrorCode_FileNotFound = 0, // 0-based index into AK::Monitor::s_aszErrorCodes table
			ErrorCode_CannotOpenFile,
			ErrorCode_CannotStartStreamNoMemory,
			ErrorCode_IODevice,

			ErrorCode_PluginUnsupportedChannelConfiguration,
			ErrorCode_PluginInitialisationFailed,
			ErrorCode_PluginProcessingFailed,
			ErrorCode_PluginExecutionInvalid,
			ErrorCode_PluginAllocationFailed,

			ErrorCode_VorbisIMRequireSeekTable,
			ErrorCode_VorbisSourceRequireSeekTable,

			ErrorCode_InvalidAudioFileHeader,
			ErrorCode_FileFormatNotSupported,

			ErrorCode_TransitionNotAccurateChannel,
			ErrorCode_TransitionNotAccurateStarvation,
			ErrorCode_NothingToPlay, 
			ErrorCode_PlayFailed,

			ErrorCode_StingerCouldNotBeScheduled,
			ErrorCode_TooLongSegmentLookAhead,
			ErrorCode_CannotScheduleMusicSwitch,

			ErrorCode_CannotPlaySource,
			ErrorCode_VoiceStarving,
			ErrorCode_StreamingSourceStarving,

			ErrorCode_PluginNotRegistered,
			ErrorCode_CodecNotRegistered,

			ErrorCode_IDNotFound,

			ErrorCode_InvalidGroupID,
			ErrorCode_SelectedChildNotAvailable,
			ErrorCode_SelectedNodeNotAvailable,
			ErrorCode_NoValidSwitch,

			ErrorCode_SelectedNodeNotAvailablePlay,

			ErrorCode_FeedbackVoiceStarving,

			ErrorCode_BankLoadFailed,
			ErrorCode_BankUnloadFailed,
			ErrorCode_ErrorWhileLoadingBank,
			ErrorCode_InsufficientSpaceToLoadBank,

			Num_ErrorCodes // THIS STAYS AT END OF ENUM
		};

		/// Function prototype of local output function pointer.
		typedef void( *LocalOutputFunc )(
			AkLpCtstr in_pszError,		///< Message or error string to be displayed
			ErrorLevel in_eErrorLevel	///< Specifies whether it should be displayed as a message or an error
			);

		extern AkLpCtstr s_aszErrorCodes[ Num_ErrorCodes ];

		/// Post a monitoring message or error code. This will be displayed in the Wwise capture
		/// log.
		/// \return AK_Success if successful, AK_Fail if there was a problem posting the message.
		///			In optimized mode, this function returns AK_NotCompatible.
		/// \remark This function is provided as a tracking tool only. It does nothing if it is 
		///			called in the optimized/release configuration and return AK_NotCompatible.
		extern AKSOUNDENGINE_API AKRESULT PostCode( 
			ErrorCode in_eError,		///< Message or error code to be displayed
			ErrorLevel in_eErrorLevel	///< Specifies whether it should be displayed as a message or an error
			);

		/// Post a monitoring message or error string. This will be displayed in the Wwise capture
		/// log.
		/// \return AK_Success if successful, AK_Fail if there was a problem posting the message.
		///			In optimized mode, this function returns AK_NotCompatible.
		/// \remark This function is provided as a tracking tool only. It does nothing if it is 
		///			called in the optimized/release configuration and return AK_NotCompatible.
		extern AKSOUNDENGINE_API AKRESULT PostString( 
			AkLpCtstr in_pszError,		///< Message or error string to be displayed
			ErrorLevel in_eErrorLevel	///< Specifies whether it should be displayed as a message or an error
			);

		/// Enable/Disable local output of monitoring messages or errors. Pass 0 to disable,
		/// or any combination of ErrorLevel_Message and ErrorLevel_Error to enable. 
		/// \return AK_Success.
		///			In optimized/release configuration, this function returns AK_NotCompatible.
		extern AKSOUNDENGINE_API AKRESULT SetLocalOutput(
			AkUInt32 in_uErrorLevel	= ErrorLevel_All, ///< ErrorLevel(s) to enable in output. Default parameters enable all.
			LocalOutputFunc in_pMonitorFunc = NULL 	  ///< Handler for local output. If NULL, the standard platform debug output method is used.
			);

		/// Get the time stamp shown in the capture log along with monitoring messages.
		/// \return AK_Success.
		///			In optimized/release configuration, this function returns 0.
		extern AKSOUNDENGINE_API AkTimeMs GetTimeStamp();
	}
}

// Macros.
#ifndef AK_OPTIMIZED
    #define AK_MONITOR_ERROR( in_eErrorCode )\
	AK::Monitor::PostCode( in_eErrorCode, AK::Monitor::ErrorLevel_Error )
#else
    #define AK_MONITOR_ERROR( in_eErrorCode )
#endif

#ifdef AK_MONITOR_IMPLEMENT_ERRORCODES
namespace AK
{
	namespace Monitor
	{
		AkLpCtstr s_aszErrorCodes[ Num_ErrorCodes ] =
		{
			L"File not found", // ErrorCode_FileNotFound,
			L"Cannot open file", // ErrorCode_CannotOpenFile,
			L"Not enough memory to start stream", // ErrorCode_CannotStartStreamNoMemory,
			L"IO device error", // ErrorCode_IODevice,

			L"Plugin unsupported channel configuration", // ErrorCode_PluginUnsupportedChannelConfiguration,
			L"Plug-in Initialization Failure", // ErrorCode_PluginInitialisationFailed,
			L"Plug-in Execution Failure", // ErrorCode_PluginProcessingFailed,
			L"Invalid plug-in execution mode", // ErrorCode_PluginExecutionInvalid
			L"Could not allocate effect", // ErrorCode_PluginAllocationFailed

			L"Vorbis file used with interactive music requires seek table to be encoded. Please update conversion settings.", // ErrorCode_VorbisIMRequireSeekTable,
			L"Vorbis file used with virtual behavior \"Play from elapsed time\" requires seek table to be encoded. Please update conversion settings or virtual mode.", // ErrorCode_VorbisSourceRequireSeekTable,

			L"Invalid file header", // ErrorCode_InvalidAudioFileHeader,
			L"File format not supported", // ErrorCode_FileFormatNotSupported,

			L"Transition not sample-accurate due to mixed channel configurations", // ErrorCode_TransitionNotAccurateChannel,
			L"Transition not sample-accurate due to source starvation", // ErrorCode_TransitionNotAccurateStarvation,
			L"Nothing to play", // ErrorCode_NothingToPlay, 
			L"Play Failed", // ErrorCode_PlayFailed,	// Notification meaning the play asked was not done for an out of control reason
											// For example, if The Element has a missing source file.

			L"Stinger could not be scheduled or was dropped", // ErrorCode_StingerCouldNotBeScheduled,
			L"Segment look-ahead is longer than previous segment in sequence", // ErrorCode_TooLongSegmentLookAhead,
			L"Cannot schedule music switch transition in upcoming segments: using Exit Cue", // ErrorCode_CannotScheduleMusicSwitch,

			L"Cannot play source", // ErrorCode_CannotPlaySource,
			L"Voice Starvation", // ErrorCode_VoiceStarving,
			L"Source starvation", // ErrorCode_StreamingSourceStarving,

			L"Plug-in not registered: ", // ErrorCode_PluginNotRegistered,
			L"Codec plug-in not registered: Vorbis", // ErrorCode_CodecNotRegistered,

			L"ID not found", // ErrorCode_IDNotFound,

			L"Invalid Group ID", // ErrorCode_InvalidGroupID,
			L"Selected Child Not Available", // ErrorCode_SelectedChildNotAvailable,
			L"Selected Node Not Available", // ErrorCode_SelectedNodeNotAvailable,
			L"No Valid Switch", // ErrorCode_NoValidSwitch,

			L"Selected node not available. Make sure the structure associated to the event is loaded or that the event has been prepared", // ErrorCode_SelectedNodeNotAvailablePlay,

			L"Motion voice starvation", // ErrorCode_FeedbackVoiceStarving,

			L"Bank Load Failed", // ErrorCode_BankLoadFailed,
			L"Bank Unload Failed", // ErrorCode_BankUnloadFailed,
			L"Error while loading bank", // ErrorCode_ErrorWhileLoadingBank,
			L"Insufficient Space to Load Bank", // ErrorCode_InsufficientSpaceToLoadBank,
		};
	}
}
#endif // AK_MONITOR_IMPLEMENT_ERRORCODES

#endif // _AKMONITORERROR_H
