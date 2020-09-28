//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkCallback.h

/// \file 
/// Declaration of callback prototypes


#ifndef _AK_CALLBACK_H_
#define _AK_CALLBACK_H_

#include <AK/SoundEngine/Common/AkTypes.h>

/// Type of callback. Used as a bitfield in methods AK::SoundEngine::PostEvent() and AK::SoundEngine::DynamicSequence::Open().
enum AkCallbackType
{
	AK_EndOfEvent		= 0x01,					///< Callback triggered when reaching the end of an event.
	AK_EndOfDynamicSequenceItem = 0x02,			///< Callback triggered when reaching the end of a dynamic sequence item.
	AK_Marker			= 0x04,					///< Callback triggered when encountering a marker during playback.

	AK_CallbackBits     = 0xff,					///< Bitmask for all callback types.

	// Not a callback type, but needs to be part of same bitfield for AK::SoundEngine::PostEvent().
	AK_EnableGetSourcePlayPosition = 0x10000	///< Enable play position information for use by AK::SoundEngine::GetSourcePlayPosition().
};

/// Callback information structure used as base for all notifications handled by AkCallbackFunc.
struct AkCallbackInfo
{
	void *			pCookie;		///< User data, passed to PostEvent()
	AkGameObjectID	gameObjID;		///< Game object ID
};

/// Callback information structure corresponding to AK_EndOfEvent.
struct AkEventCallbackInfo : public AkCallbackInfo
{
	AkPlayingID		playingID;		///< Playing ID of Event, returned by PostEvent()
	AkUniqueID		eventID;		///< Unique ID of Event, passed to PostEvent()
};

/// Callback information structure corresponding to AK_Marker.
struct AkMarkerCallbackInfo : public AkEventCallbackInfo
{
	AkUInt32	uIdentifier;		///< Cue point identifier
	AkUInt32	uPosition;			///< Position in the cue point (unit: sample frames)
	AkLpCstr	strLabel;			///< Label of the marker, read from the file
};

/// Callback information structure corresponding to AK_EndOfDynamicSequenceItem.
struct AkDynamicSequenceItemCallbackInfo : public AkCallbackInfo
{
	AkPlayingID playingID;			///< Playing ID of Dynamic Sequence, returned by AK::SoundEngine:DynamicSequence::Open()
	AkUniqueID	audioNodeID;		///< Audio Node ID of finished item
	void*		pCustomInfo;		///< Custom info passed to the DynamicSequence::Open function
};

/// Function called on completion of an event, or when a marker is reached.
/// \param in_eType Type of callback.
/// \param in_pCallbackInfo Structure containing callback information.
/// \remarks An event is considered completed once all of its actions have been executed and all the playbacks in this events are terminated.
/// \remarks This callback is executed from the audio processing thread. The processing time in the callback function should be minimal. Having too much processing time could result in slowing down the audio processing.
/// \remarks Before waiting on an AK_EndOfEvent, make sure that the event is going to end. 
/// Some events can be continuously playing or infinitely looping, and the callback will not occur unless a specific stop event is sent to terminate the event.
/// \sa 
/// - AK::SoundEngine::PostEvent()
/// - AK::SoundEngine::DynamicSequence::Open()
/// - \ref soundengine_events
/// - \ref soundengine_markers
typedef void( *AkCallbackFunc )( 
	AkCallbackType in_eType, 
	AkCallbackInfo* in_pCallbackInfo 
	);

/// Callback prototype used with asynchronous bank load/unload requests.
/// This function is called when the bank request has been processed 
/// and indicates if it was successfully executed or if an error occurred.
/// \param Identifier of the bank that was explicitly loaded/unloaded. 
/// In the case of PrepareEvent() or PrepareGameSyncs(), this value contains 
/// the AkUniqueID of the event/game sync that was prepared/unprepared, if the array contained only
/// one element. Otherwise, in_bankID equals AK_INVALID_UNIQUE_ID.
/// \param in_eLoadResult Result of the requested action.
///	- AK_Success: Load or unload successful.
///	- AK_Cancelled: Load or unload was cancelled by another user request.
/// - AK_IDNotFound: At least one of the event/game sync identifiers passed to PrepareEvent() or PrepareGameSyncs() does not exist.
/// - AK_InsufficientMemory: Insufficient memory to store bank data.
/// - AK_BankReadError: I/O error.
/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
/// you used to generate the SoundBanks matches that of the SDK you are currently using.
/// - AK_InvalidFile: File specified could not be opened.
/// - AK_InvalidParameter: Invalid parameter.
/// - AK_Fail: Load or unload failed for any other reason.
/// \param in_memPoolId ID of the memory pool in which the bank was explicitly loaded/unloaded. 
/// AK_DEFAULT_POOL_ID is returned whenever this callback is issued from an implicit bank load (PrepareEvent(), PrepareGameSyncs()), 
/// the bank memory was managed internally, an error occured, or in the case of.
/// \param in_pCookie Optional cookie that was passed to the bank request.
/// \remarks This callback is executed from the bank thread. The processing time in the callback function should be minimal. Having too much processing time could slow down the bank loading process.
/// \sa 
/// - AK::SoundEngine::LoadBank()
/// - AK::SoundEngine::UnloadBank()
/// - AK::SoundEngine::PrepareEvent()
/// - AK::SoundEngine::PrepareGameSyncs()
/// - \ref soundengine_banks
typedef void( *AkBankCallbackFunc )(
	AkBankID		in_bankID,
	AKRESULT		in_eLoadResult,
	AkMemPoolId		in_memPoolId,
	void *			in_pCookie
	);

#endif // _AK_CALLBACK_H_

