//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

/// Audiokinetic namespace
namespace AK
{

namespace MotionEngine
{
	/// Connects a motion device to a player.  Call this function from your game to tell the motion engine that
	/// a player is using the specified device.
	/// \return 
	/// - AK_Success if the initialization was successful
	/// - AK_Fail if the device could not be initialized.  Usually this means the drivers are not installed.
	/// \sa
	/// - \ref integrating_elements_motion
	extern AKSOUNDENGINE_API AKRESULT AddPlayerMotionDevice(
		AkUInt8 in_iPlayerID,			///< Player number, must be between 0 and 3.
		AkUInt32 in_iCompanyID,			///< Company ID providing support for the device
		AkUInt32 in_iDeviceID			///< Device ID, must be one of the currently supported devices. 
		);

	/// Disconnects a motion device from a player port.  Call this function from your game to tell the motion engine that
	/// a player is not using the specified device anymore.
	/// \sa
	/// - \ref integrating_elements_motion
	extern AKSOUNDENGINE_API void RemovePlayerMotionDevice(
		AkUInt8 in_iPlayerID,			///< Player number, must be between 0 and 3.
		AkUInt32 in_iCompanyID,			///< Company ID providing support for the device
		AkUInt32 in_iDeviceID			///< Device ID, must be one of the currently supported devices. 
		);

	/// Registers a motion device for use in the game.  
	/// \return 
	/// - AK_Success if the registration was successful
	/// - AK_Fail if the library isn't linked.
	/// \sa
	/// - \ref integrating_elements_motion
	extern AKSOUNDENGINE_API void RegisterMotionDevice(
		AkUInt32 in_ulCompanyID,				///< Company ID providing support for the device
		AkUInt32 in_ulPluginID,					///< Device ID, must be one of the currently supported devices. 
		AkCreatePluginCallback in_pCreateFunc	///< Creation function.
		);

	/// Attaches a player to a listener.  This is necessary for the player to receive motion through the connected
	/// devices.  
	/// \sa
	/// - \ref integrating_elements_motion
	/// - \ref soundengine_listeners 
	extern AKSOUNDENGINE_API void SetPlayerListener(
		AkUInt8 in_iPlayerID,					///< Player ID, between 0 and 3
		AkUInt8 in_iListener					///< Listener ID, between 0 and 7
		);

	/// Set the master volume for a player.  All devices assigned to this player will be affected by this volume.
	/// \sa
	/// - \ref integrating_elements_motion
	extern AKSOUNDENGINE_API void SetPlayerVolume(
		AkUInt8 in_iPlayerID,					///< Player ID, between 0 and 3
		AkReal32 in_fVolume						///< Master volume for the given player, in decibels (-96.3 to 0).
		);
}
}
