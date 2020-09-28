// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
// AkQueryParameters.h

/// \file 
/// The sound engine parameter query interface.


#ifndef _AK_QUERYPARAMS_H_
#define _AK_QUERYPARAMS_H_

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>

/// Positioning information obtained from an object
struct AkPositioningInfo
{
	AkReal32			fCenterPct;			///< Center %
	AkPositioningType	positioningType;	///< Positioning Type (2D, 3D gamedef, 3D userdef)
	bool				bUpdateEachFrame;   ///< Update at each frame (valid only with game-defined)
	bool				bUseSpatialization; ///< Use spatialization
	bool				bUseAttenuation;	///< Use attenuation parameter set

	bool				bUseConeAttenuation; ///< Use the cone attenuation
	AkReal32			fInnerAngle;		///< Inner angle
	AkReal32			fOuterAngle;		///< Outer angle
	AkReal32			fConeMaxAttenuation; ///< Cone max attenuation
	AkLPFType			LPFCone;			///< Cone low pass filter value

	AkReal32			fMaxDistance;		///< Maximum distance
	AkReal32			fVolDryAtMaxDist;	///< Volume dry at maximum distance
	AkReal32			fVolWetAtMaxDist;	///< Volume wet at maximum distance (if any)
	AkLPFType			LPFValueAtMaxDist;  ///< Low pass filter value at max distance (if any)
};

/// Object information structure for QueryAudioObjectsIDs
struct AkObjectInfo
{
	AkUniqueID	objID;		///< Object ID
	AkUniqueID	parentID;	///< Object ID of the parent 
	AkInt32		iDepth;		///< Depth in tree
};

// Audiokinetic namespace
namespace AK
{
	// Audiokinetic sound engine namespace
	namespace SoundEngine
	{
		/// Query namespace
		/// \remarks The functions in this namespace are thread-safe, unless stated otherwise.
		namespace Query
		{
			////////////////////////////////////////////////////////////////////////
			/// @name Game Objects
			//@{

			/// Get the position of a game object.
			/// \return AK_Success if succeeded, or AK_IDNotFound if the game object was not registered
			/// \sa 
			/// - \ref soundengine_3dpositions
			extern AKSOUNDENGINE_API AKRESULT GetPosition( 
				AkGameObjectID in_GameObjectID,				///< Game object identifier
				AkSoundPosition& out_rPosition				///< Position to get
				);

			//@}

			////////////////////////////////////////////////////////////////////////
			/// @name Listeners
			//@{

			/// Get a game object's active listeners.
			/// \return AK_Success if succeeded, or AK_IDNotFound if the game object was not registered
			/// \sa 
			/// - \ref soundengine_listeners_multi_assignobjects
			extern AKSOUNDENGINE_API AKRESULT GetActiveListeners(
				AkGameObjectID in_GameObjectID,				///< Game object identifier
				AkUInt32& out_ruListenerMask				///< Bitmask representing the active listeners (LSB = Listener 0, set to 1 means active)
				);

			/// Get a listener's position.
			/// \return AK_Success if succeeded, or AK_InvalidParameter if the index is out of range
			/// \sa 
			/// - \ref soundengine_listeners_settingpos
			extern AKSOUNDENGINE_API AKRESULT GetListenerPosition( 
				AkUInt32 in_uIndex, 						///< Listener index (0: first listener, 7: 8th listener)
				AkListenerPosition& out_rPosition			///< Position set
				);

			/// Get a listener's spatialization parameters. 
			/// \return AK_Success if succeeded, or AK_InvalidParameter if the index is out of range
			/// \sa 
			/// - \ref soundengine_listeners_spatial
			extern AKSOUNDENGINE_API AKRESULT GetListenerSpatialization(
				AkUInt32 in_uIndex,							///< Listener index (0: first listener, 7: 8th listener)
				bool& out_rbSpatialized,					///< Spatialization state
				AkSpeakerVolumes& out_rVolumeOffsets		///< Per-speaker volume offset, in dB (Only used if in_bSpatialized == false)
				);

			//@}


			////////////////////////////////////////////////////////////////////////
			/// @name Game Syncs
			//@{

			/// Get the value of a real-time parameter control (by ID).
			/// \return AK_Success if succeeded, AK_IDNotFound if the game object was not registered, or AK_Fail if the RTPC value could not be obtained
			/// \sa 
			/// - \ref soundengine_rtpc
			extern AKSOUNDENGINE_API AKRESULT GetRTPCValue( 
				AkRtpcID in_rtpcID, 						///< ID of the RTPC
				AkGameObjectID in_gameObjectID,				///< Associated game object ID
				AkRtpcValue& out_rValue, 					///< Value returned
				bool&		out_rGlobal						///< Is this value obtained from global object? (Y/N)
				);

			/// Get the value of a real-time parameter control (by name).
			/// \return AK_Success if succeeded, AK_IDNotFound if the game object was not registered or the rtpc name could not be found, or AK_Fail if the RTPC value could not be obtained
			/// \sa 
			/// - \ref soundengine_rtpc
			extern AKSOUNDENGINE_API AKRESULT GetRTPCValue( 
				AkLpCtstr in_pszRtpcName,					///< Name of the RTPC
				AkGameObjectID in_gameObjectID,				///< Associated game object ID
				AkRtpcValue& out_rValue, 					///< Value returned
				bool&		out_rGlobal						///< Is this value obtained from global object? (Y/N)
				);

			/// Get the state of a switch group (by IDs).
			/// \return AK_Success if succeeded, or AK_IDNotFound if the game object was not registered
			/// \sa 
			/// - \ref soundengine_switch
			extern AKSOUNDENGINE_API AKRESULT GetSwitch( 
				AkSwitchGroupID in_switchGroup, 			///< ID of the switch group
				AkGameObjectID  in_gameObjectID,			///< Associated game object ID
				AkSwitchStateID& out_rSwitchState 			///< ID of the switch
				);

			/// Get the state of a switch group (by name).
			/// \return AK_Success if succeeded, or AK_IDNotFound if the game object was not registered or the switch group name can not be found
			/// \sa 
			/// - \ref soundengine_switch
			extern AKSOUNDENGINE_API AKRESULT GetSwitch( 
				AkLpCtstr in_pstrSwitchGroupName,			///< Name of the switch group
				AkGameObjectID in_GameObj,					///< Associated game object ID
				AkSwitchStateID& out_rSwitchState			///< ID of the switch
				);

			/// Get the state of a state group (by IDs).
			/// \return AK_Success if succeeded
			/// \sa 
			/// - \ref soundengine_states
			extern AKSOUNDENGINE_API AKRESULT GetState( 
				AkStateGroupID in_stateGroup, 				///< ID of the state group
				AkStateID& out_rState 						///< ID of the state
				);

			/// Get the state of a state group (by name).
			/// \return AK_Success if succeeded, or AK_IDNotFound if the state group name can not be found
			/// \sa 
			/// - \ref soundengine_states
			extern AKSOUNDENGINE_API AKRESULT GetState( 
				AkLpCtstr in_pstrStateGroupName,			///< Name of the state group
				AkStateID& out_rState						///< ID of the state
				);


			//@}

			////////////////////////////////////////////////////////////////////////
			/// @name Environments
			//@{

			/// Get the environmental ratios used by the specified game object.
			/// The array size cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ.
			/// To clear the game object's environments, in_uNumEnvValues must be 0.
			/// \aknote The actual maximum number of environments in which a game object can be is AK_MAX_ENVIRONMENTS_PER_OBJ. \endaknote
			/// \sa 
			/// - \ref soundengine_environments
			/// - \ref soundengine_environments_setting_environments
			/// - \ref soundengine_environments_id_vs_string
			/// \return AK_Success if succeeded, or AK_InvalidParameter if io_ruNumEnvValues is 0 or out_paEnvironmentValues is NULL, or AK_PartialSuccess if more environments exist than io_ruNumEnvValues
			/// AK_InvalidParameter
			extern AKSOUNDENGINE_API AKRESULT GetGameObjectEnvironmentsValues( 
				AkGameObjectID		in_gameObjectID,		///< Associated game object ID
				AkEnvironmentValue*	out_paEnvironmentValues,	///< Variable-size array of AkEnvironmentValue structures
																///< (it may be NULL if no environment must be set, and its size 
																///< cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
				AkUInt32&			io_ruNumEnvValues		///< The number of environments at the pointer's address
															///< (it must be 0 if no environment is set, and can not exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
				);

			/// Get the environmental dry level to be used for the specified game object
			/// The control value is a number ranging from 0.0f to 1.0f.
			/// 0.0f stands for 0% dry, while 1.0f stands for 100% dry.
			/// \aknote Reducing the dry level does not mean increasing the wet level. \endaknote
			/// \sa 
			/// - \ref soundengine_environments
			/// - \ref soundengine_environments_setting_dry_environment
			/// - \ref soundengine_environments_id_vs_string
			/// \return AK_Success if succeeded, or AK_IDNotFound if the game object was not registered
			extern AKSOUNDENGINE_API AKRESULT GetGameObjectDryLevelValue( 
				AkGameObjectID		in_gameObjectID,		///< Associated game object ID
				AkReal32&			out_rfControlValue		///< Dry level control value, ranging from 0.0f to 1.0f
															///< (0.0f stands for 0% dry, while 1.0f stands for 100% dry)
				);

			/// Get the volume for the specified environment.
			/// The volume is a number ranging from 0.0f to 1.0f.
			/// 0.0f stands for 0% of the environment volume, while 1.0f stands for 100% of the environment volume.
			/// \sa 
			/// - \ref soundengine_environments
			/// - \ref soundengine_environments_setting_environment_volume
			/// \return AK_Success if succeeded
			extern AKSOUNDENGINE_API AKRESULT GetEnvironmentVolume( 
				AkEnvID				in_FXParameterSetID,	///< Environment ID
				AkReal32&			out_rfVolume			///< Volume control value, ranging from 0.0f to 1.0f.
															///< (0.0f stands for 0% of the environment volume, 1.0f stands for 100% of the environment volume)
				);

			/// Get the Bypass value for an environment.
			/// \sa 
			/// - \ref soundengine_environments
			/// - \ref soundengine_environments_bypassing_environments
			/// - \ref soundengine_environments_id_vs_string
			/// \return AK_Success if succeeded
			extern AKSOUNDENGINE_API AKRESULT GetEnvironmentBypass(
				AkEnvID	in_FXParameterSetID,				///< Environment ID
				bool&	out_rbIsBypassed					///< True if bypass the specified environment
				);

			/// Get a game object's obstruction and occlusion levels.
			/// \sa 
			/// - \ref soundengine_obsocc
			/// - \ref soundengine_environments
			/// \return AK_Success if succeeded, AK_IDNotFound if the game object was not registered
			extern AKSOUNDENGINE_API AKRESULT GetObjectObstructionAndOcclusion(  
				AkGameObjectID in_ObjectID,			///< Associated game object ID
				AkUInt32 in_uListener,				///< Listener index (0: first listener, 7: 8th listener)
				AkReal32& out_rfObstructionLevel,		///< ObstructionLevel: [0.0f..1.0f]
				AkReal32& out_rfOcclusionLevel			///< OcclusionLevel: [0.0f..1.0f]
				);

			//@}

			/// Get the list of audio object IDs associated to an event.
			/// \aknote It is possible to call QueryAudioObjectIDs with io_ruNumItems = 0 to get the total size of the
			/// structure that should be allocated for out_aObjectInfos.
			/// \return AK_Success if succeeded, AK_IDNotFound if the eventID cannot be found, AK_InvalidParameter if out_aObjectInfos is NULL while io_ruNumItems > 0
			/// or AK_UnknownObject if the event contains an unknown audio object, 
			/// or AK_PartialSuccess if io_ruNumItems was set to 0 to query the number of available items.
			extern AKSOUNDENGINE_API AKRESULT QueryAudioObjectIDs(
				AkUniqueID in_eventID,				///< Event ID
				AkUInt32& io_ruNumItems,			///< Number of items in array provided / Number of items filled in array
				AkObjectInfo* out_aObjectInfos		///< Array of AkObjectInfo items to fill
				);

			/// Get the list of audio object IDs associated to an event name.
			/// \aknote It is possible to call QueryAudioObjectIDs with io_ruNumItems = 0 to get the total size of the
			/// structure that should be allocated for out_aObjectInfos.
			/// \return AK_Success if succeeded, AK_IDNotFound if the event name cannot be found, AK_InvalidParameter if out_aObjectInfos is NULL while io_ruNumItems > 0
			/// or AK_UnknownObject if the event contains an unknown audio object, 
			/// or AK_PartialSuccess if io_ruNumItems was set to 0 to query the number of available items.
			extern AKSOUNDENGINE_API AKRESULT QueryAudioObjectIDs(
				AkLpCtstr in_pszEventName,			///< Event name
				AkUInt32& io_ruNumItems,			///< Number of items in array provided / Number of items filled in array
				AkObjectInfo* out_aObjectInfos		///< Array of AkObjectInfo items to fill
				);

			/// Get positioning information associated to an audio object.
			/// \return AK_Success if succeeded, AK_IDNotFound if the object ID cannot be found, AK_NotCompatible if the audio object cannot expose positioning
			extern AKSOUNDENGINE_API AKRESULT GetPositioningInfo( 
				AkUniqueID in_ObjectID,						///< Audio object ID
				AkPositioningInfo& out_rPositioningInfo		///< Positioning information structure to be filled
				);

		} //namespace Query
	} //namespace SoundEngine
} //namespace AK

#endif // _AK_QUERYPARAMS_H_
