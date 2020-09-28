//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Sound Frame public interfaces. These interfaces allow a client application 
/// to interact with an instance of Wwise running on the same computer.

#ifndef _AK_SOUNDFRAME_SF_H
#define _AK_SOUNDFRAME_SF_H

#include <wtypes.h>

#include "SFObjects.h"
#include <AK/SoundEngine/Common/AkTypes.h>

struct IDataObject; // OLE data object interface

/// The maximum size for a monitoring message in the Sound Frame.
/// \sa 
/// - AK::SoundFrame::PostMsgMonitor()
#define MAX_MONITORING_MSG_SIZE		256

namespace AK
{
	/// Audiokinetic Sound Frame namespace.
	namespace SoundFrame
	{
		/// Interface through which the client communicates with the instance of Wwise.
		/// \warning The functions in this class are not thread-safe, unless stated otherwise.
		/// \sa
		/// - AK::SoundFrame::Create()
		class ISoundFrame : public ISFRefCount
		{
		public:
			///////////////////////////////////////////////////////////////////////
			/// @name General Information
			//@{

			/// Request the connection status.
			/// \return	True if the Wwise application is connected to the Sound Frame, False otherwise
			virtual bool IsConnected() const = 0;

			/// Request the Sound Frame Client version. For full compatibility, the Client and Server versions
			/// should be the same.
			/// \return	The Sound Frame Client version
			/// \sa
			/// - AK::SoundFrame::ISoundFrame::GetServerVersion()
			virtual ULONG GetClientVersion() const = 0;

			/// Request the Sound Frame Server version. For full compatibility, the Client and Server version
			/// should be the same.
			/// \return	The Sound Frame Server version
			/// \sa
			/// - AK::SoundFrame::ISoundFrame::GetClientVersion()
			virtual ULONG GetServerVersion() const = 0;

			/// Get the name of the current Wwise project.
			/// \aknote
			/// The string at the pointer's location will change if a different project is loaded in Wwise.
			/// \endaknote
			/// \return	A pointer to a string, or NULL if the SoundFrame is not connected to Wwise
			/// \sa
			/// - AK::SoundFrame::ISoundFrame::GetCurrentProjectID()
			virtual const WCHAR * GetCurrentProjectName() const = 0;

			/// Get the unique ID of the current project.
			/// \aknote
			/// The project's ID will change if a different project is loaded in Wwise.
			/// \endaknote
			/// \return	The unique ID of the current project, or GUID_NULL if the SoundFrame is not connected to Wwise
			/// \sa
			/// - AK::SoundFrame::ISoundFrame::GetCurrentProjectName()
			virtual GUID GetCurrentProjectID() const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Sound Engine Operations
			//@{

			/// Trigger playback of events in Wwise (using the unique IDs of events).\n\n
			/// This function is thread-safe.
			/// \aknote
			/// Using the AK::SoundFrame::IGameObject::s_WwiseGameObject will 
			/// result in playback on the default game object in Wwise.
			/// \endaknote
			/// \akcaution
			/// Event playback in Wwise is not synchronized. Even if multiple events are sent in the same PlayEvents() call, they might 
			/// not all start at the exact same time.
			/// \endakcaution
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::PostEvent()
			virtual bool PlayEvents( 
				const AkUniqueID * in_pEvents,										///< Array of unique IDs of events
				long in_cEvents, 													///< Number of events in \e in_pEvents
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which events will be played (optional)
				) = 0;

			/// Trigger playback of events in Wwise (using event names).\n\n
			/// This function is thread-safe. 
			/// \aknote
			/// Using the AK::SoundFrame::IGameObject::s_WwiseGameObject will 
			/// result in playback on the default game object in Wwise.
			/// \endaknote
			/// \akcaution
			/// Event playback in Wwise is not synchronized. Even if multiple events are sent in the same PlayEvents() call, they might 
			/// not all start at the exact same time.
			/// \endakcaution
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::PostEvent()
			virtual bool PlayEvents( 
				LPCWSTR * in_pszEvents,												///< Array of event names
				long in_cEvents,													///< Number of events in \e in_pszEvents
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which events will be played (optional)
				) = 0;

			/// Set the playback mode to True when many calls to AK::SoundFrame::ISoundFrame::PlayEvents() are made.\n
			/// This way, Wwise will stay in playback mode as long as you don't set the playback mode to False.\n
			/// Setting the playback mode to False will stop all playback in Wwise.\n\n
			/// This function is thread-safe.
			/// \aktip
			/// - Use this method to avoid flickering of the Wwise controls.
			/// - SetPlayBackMode( false ) can be used without a corresponding call to SetPlayBackMode( true ) to stop all 
			/// currently-playing events.
			/// \endaktip
			/// \return	True if the operation was successful, False otherwise
			virtual bool SetPlayBackMode( 
				bool in_bPlayback			///< Playback mode (True or False)
				) const = 0;

			/// Get the current state of a given state group (using the state group's unique ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetCurrentState(
				AkStateGroupID in_stateGroupID,		///< State group ID
				IState** out_ppCurrentState			///< Returned AddRef'd pointer to IState interface
				) const = 0;

			/// Get the current state of a given state group (using the state group's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetCurrentState(
				LPCWSTR	 in_szStateGroupName,		///< State group name
				IState** out_ppCurrentState			///< Returned AddRef'd pointer to IState interface
				) const = 0;

			/// Set the current state for a given state group (using IDs).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetState()
			virtual bool SetCurrentState(
				AkStateGroupID in_stateGroupID,	///< State group ID
				AkStateID in_currentStateID		///< State ID
				) = 0;

			/// Set the current state for a given state group (using strings).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetState()
			virtual bool SetCurrentState(
				LPCWSTR in_szStateGroupName,	///< State group name
				LPCWSTR in_szCurrentStateName	///< State name
				) = 0;

			/// Get a specified game object's current switch for a given switch group (using the switch group's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetCurrentSwitch(
				AkSwitchGroupID in_switchGroupID,									///< Switch group ID
				ISwitch** out_ppCurrentSwitch,										///< Returned AddRef'd pointer to an ISwitch interface
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Queried game object (optional)
				) const = 0;	

			/// Get a specified game object's current switch for a given switch group (using the switch group's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetCurrentSwitch(
				LPCWSTR in_szSwitchGroupName,										///< Switch group name
				ISwitch** out_ppCurrentSwitch,										///< Returned AddRef'd pointer to an ISwitch interface
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Queried game object (optional)
				) const = 0;

			/// Set the current switch for a given switch group (using IDs).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetSwitch()
			virtual bool SetCurrentSwitch(
				AkSwitchGroupID in_switchGroupID,									///< Switch group ID
				AkSwitchStateID in_currentSwitchID,									///< Switch ID
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which the switch will be set (optional)
				) = 0;

			/// Set the current switch for a given switch group (using strings).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetSwitch()
			virtual bool SetCurrentSwitch(
				LPCWSTR in_szSwitchGroupName,										///< Switch group name
				LPCWSTR in_szCurrentSwitchName,										///< Switch name
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which the switch will be set (optional)
				) = 0;

			/// Add a game object to the sound engine.\n\n
			/// This function is thread-safe.
			/// \akcaution
			/// Game object IDs IGameObject::s_InvalidGameObject and IGameObject::s_WwiseGameObject are reserved IDs in Wwise, and as such can not 
			/// be registered in the Sound Frame.
			/// \endakcaution
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::RegisterGameObj()
			virtual bool RegisterGameObject(
				AkGameObjectID in_gameObjectID,			///< Game object ID
				LPCWSTR in_szGameObjectName = L""		///< Game object name (optional)
				) = 0;

			/// Remove a game object from the sound engine.\n\n
			/// This function is thread-safe. 
			/// \akcaution
			/// Game object IDs IGameObject::s_InvalidGameObject and IGameObject::s_WwiseGameObject are reserved IDs in Wwise, and as such can not 
			/// be unregistered in the Sound Frame.
			/// \endakcaution
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::UnregisterGameObj()
			virtual bool UnregisterGameObject(
				AkGameObjectID in_gameObjectID			///< Game object ID
				) = 0;

			/// Set an RTPC value in the sound engine (using the RTPC's ID).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetRTPCValue()
			virtual bool SetRTPCValue(
				AkRtpcID in_gameParameterID,										///< RTPC ID
				AkRtpcValue in_value,												///< RTPC value
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which the RTPC will be set (optional)
				) = 0;

			/// Set an RTPC value in the sound engine (using the RTPC's name).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetRTPCValue()
			virtual bool SetRTPCValue(
				LPCWSTR in_szGameParameterName,										///< RTPC name
				AkRtpcValue in_value,												///< RTPC value
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object on which the RTPC will be set (optional)
				) = 0;

			/// Send a trigger to the sound engine (using the trigger ID).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::PostTrigger()
			virtual bool PostTrigger(
				AkTriggerID in_triggerID,											///< Trigger ID
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object to which the trigger will be sent (optional)
				) = 0;

			/// Send a trigger to the sound engine (using the trigger name).\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::PostTrigger()
			virtual bool PostTrigger(
				LPCWSTR in_szTriggerName,											///< Trigger name
				AkGameObjectID in_gameObjectID = IGameObject::s_WwiseGameObject		///< Game object to which the trigger will be sent (optional)
				) = 0;

			/// Set a game object's active listeners.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetActiveListeners()
			virtual bool SetActiveListeners( 
				AkGameObjectID in_gameObjectID,	///< Game object ID
				AkUInt32 in_uiListenerMask		///< Bitmask representing the active listeners (LSB = Listener 0, set to 1 means active)
				) = 0;

			/// Set the position of a game object in the sound engine.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetPosition()
			virtual bool SetPosition( 
				AkGameObjectID in_gameObjectID,							///< Game object ID
				const AkSoundPosition& in_rPosition,					///< Position to set
				AkUInt32 in_uiListenerIndex = AK_INVALID_LISTENER_INDEX	///< If the listener index is valid, the listener position is used instead of the game object position.
				) = 0;

			/// Set a listener's position in the sound engine.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetListenerPosition()
			virtual bool SetListenerPosition( 
				const AkListenerPosition& in_rPosition, ///< Position to set
				AkUInt32 in_uiIndex = 0					///< Listener index (0: first listener, 7: 8th listener)
				) = 0;
			
			/// Set a listener's spatialization parameters.\n
			/// This allows you to define listener-specific volume offsets for each audio channel.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetListenerSpatialization()
			virtual bool SetListenerSpatialization( 
				AkUInt32 in_uiIndex,						///< Listener index (0: first listener, 7: 8th listener)
				bool in_bSpatialized,						///< Spatialization toggle
				AkSpeakerVolumes * in_pVolumeOffsets = NULL	///< Per-speaker volume offset, in dB (Only used if in_bSpatialized == false)
				) = 0;

			/// Set the Environmental ratios to be used for the specified game object.\n
			/// The array size cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ.\n
			/// To clear the game object's environments, in_uNumEnvValues must be set to 0.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetGameObjectEnvironmentsValues()
			virtual bool SetGameObjectEnvironmentsValues( 
				AkGameObjectID in_gameObjectID,				///< Associated game object ID
				AkEnvironmentValue* in_aEnvironmentValues,  ///< Variable-size array of AkEnvironmentValue structures
															///< (it may be NULL if no environment must be set, and its size 
															///< cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
				AkUInt32 in_uNumEnvValues					///< The number of environments at the pointer's address
															///< (it must be 0 if no environment is set, and can not exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
				) = 0;

			/// Set the Environmental dry level to be used for the specified game object.\n
			/// The control value is a number ranging from 0.0f to 1.0f.
			/// 0.0f means 0% dry, while 1.0f means 100% dry.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetGameObjectDryLevelValue()
			virtual bool SetGameObjectDryLevelValue( 
				AkGameObjectID in_gameObjectID,			///< Associated game object ID
				AkReal32 in_fControlValue				///< Dry level control value, ranging from 0.0f to 1.0f
														///< (0.0f means 0% dry, while 1.0f means 100% dry)
				) = 0;
			
			/// Set the volume for the specified environment.\n
			/// The volume is a number ranging from 0.0f to 1.0f.
			/// 0.0f means 0% of the environment volume, while 1.0f means 100% of the environment volume.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetEnvironmentVolume()
			virtual bool SetEnvironmentVolume( 
				AkEnvID in_FXParameterSetID,	///< Environment ID
				AkReal32 in_fVolume				///< Volume control value, ranging from 0.0f to 1.0f.
												///< (0.0f means 0% of the environment volume, 1.0f means 100% of the environment volume)
				) = 0;

			/// Bypass an environment.\n
			/// The specified environment will not be computed when bypassed.\n
			/// This method is useful to disable environments that should not be heard, and therefore save resources.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::BypassEnvironment()
			virtual bool BypassEnvironment( 
				AkEnvID in_FXParameterSetID,	///< Environment ID
				bool in_bIsBypassed				///< The specified environment will be bypassed if this parameter is set to True.
				) = 0;

			/// Set a game object's obstruction and occlusion levels.\n
			/// This method is used to affect how an object should be heard by a specific listener.\n\n
			/// This function is thread-safe.
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::SetObjectObstructionAndOcclusion()
			virtual bool SetObjectObstructionAndOcclusion( 
				AkGameObjectID in_ObjectID,			///< Associated game object ID
				AkUInt32 in_uListener,				///< Listener index (0: first listener, 7: 8th listener)
				AkReal32 in_fObstructionLevel,		///< ObstructionLevel: [0.0f..1.0f]
				AkReal32 in_fOcclusionLevel			///< OcclusionLevel: [0.0f..1.0f]
				) = 0;

			/// Post a monitoring message. This allows the Sound Frame user to send a message 
			/// that will be displayed in the Wwise capture log.\n\n
			/// This function is thread-safe. 
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundEngine::PostMsgMonitor()
			virtual bool PostMsgMonitor( 
				LPCWSTR in_pszMessage	///< Null-terminated message to be sent (if its size exceeds MAX_MONITORING_MSG_SIZE,
										///< it will be truncated)
				) = 0;

			/// Stop all playing instances.
			/// \return	True if the operation was successful, False otherwise
			virtual bool StopAll( 
				AkGameObjectID in_GameObjID = AK_INVALID_GAME_OBJECT ///< (Optional) Will stop only instances associated to the specified game object if one is specified.
				) = 0;

			/// Stop all playing instances associated with the specified playing ID.
			/// Calling StopPlayingID on a playing ID that is not active will have no impact.
			/// \return	True if the operation was successful, False otherwise
			virtual bool StopPlayingID( 
				AkPlayingID in_playingID ///< Playing ID that was returned by a PostEvent command.
				) = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Wwise Object Accessor Functions
			//@{

			///////////////////////////////////////////////////////////////////////
			/// @name Events
			//@{

			/// Get all events listed in Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEventList( 
				IEventList ** out_ppEventList	///< Returned AddRef'd pointer to an IEventList interface
				) const = 0;

			/// Get an event from Wwise (using the event's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEvent( 
				AkUniqueID in_eventID,		///< Unique ID of the event
				IEvent ** out_ppEvent		///< Returned AddRef'd pointer to an IEvent interface
				) const = 0;

			/// Get an event from Wwise (using the event's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEvent( 
				LPCWSTR in_pszEvent, 		///< Name of the event
				IEvent ** out_ppEvent		///< Returned AddRef'd pointer to an IEvent interface
				) const = 0;

			/// Get the list of specified events from Wwise (using the events' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEvents( 
				const AkUniqueID * in_pEvents,	///< Array of unique IDs of events
				long in_cEvents, 				///< Number of events in \e in_pEvents
				IEventList ** out_ppEventList 	///< Returned AddRef'd pointer to an IEventList interface
				) const = 0;

			/// Get the list of specified events from Wwise (using the events' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEvents( 
				LPCWSTR * in_pszEvents, 		///< Array of event names
				long in_cEvents, 				///< Number of events in \e in_pszEvents
				IEventList ** out_ppEventList 	///< Returned AddRef'd pointer to an IEventList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Sound Objects
			//@{

			/// Get a sound object from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSoundObject( 
				AkUniqueID in_soundObjectID,		///< Unique ID of the sound object
				ISoundObject ** out_ppSoundObject 	///< Returned AddRef'd pointer to an ISoundObject interface
				) const = 0;

			/// Get the list of specified sound objects from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSoundObjects( 
				const AkUniqueID * in_pSoundObjects, 		///< Array of unique IDs of sound objects
				long in_cSoundObjects, 						///< Number of sound objects in \e in_pSoundObjects
				ISoundObjectList ** out_ppSoundObjectList 	///< Returned AddRef'd pointer to an ISoundObjectList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name State Groups
			//@{

			/// Get the list of all state groups from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetStateGroupList( 
				IStateGroupList ** out_ppStateGroupList	///< Returned AddRef'd pointer to an IStateGroupList interface
				) const = 0;

			/// Get a state group from Wwise (using the state group's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetStateGroup( 
				AkUniqueID in_stateGroupID,		///< Unique ID of the state group
				IStateGroup ** out_ppStateGroup	///< Returned AddRef'd pointer to an IStateGroup interface
				) const = 0;

			/// Get a state group from Wwise (using the state group's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetStateGroup( 
				LPCWSTR in_pszStateGroup, 				///< Name of the state group
				IStateGroup ** out_ppStateGroup			///< Returned AddRef'd pointer to an IStateGroup interface
				) const = 0;

			/// Get a list of state groups from Wwise (using the states groups' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetStateGroups( 
				const AkUniqueID * in_pStateGroups,		///< Array of unique IDs of state groups
				long in_cStateGroups, 					///< Number of state groups in \e in_pStateGroups
				IStateGroupList ** out_ppStateGroupList ///< Returned AddRef'd pointer to an IStateGroupList interface
				) const = 0;

			/// Get a list of state groups from Wwise (using the states groups' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetStateGroups( 
				LPCWSTR * in_pszStateGroups, 				///< Array of state group names
				long in_cStateGroups, 						///< Number of state groups in \e in_pszStateGroups
				IStateGroupList ** out_ppStateGroupList 	///< Returned AddRef'd pointer to an IStateGroupList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Switch Groups
			//@{

			/// Get the list of all switch groups from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSwitchGroupList( 
				ISwitchGroupList ** out_ppSwitchGroupList	///< Returned AddRef'd pointer to an ISwitchGroupList interface
				) const = 0;

			/// Get a switch group from Wwise (using the switch group's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSwitchGroup( 
				AkUniqueID in_switchGroupID,		///< Unique ID of the switch group
				ISwitchGroup ** out_ppSwitchGroup	///< Returned AddRef'd pointer to an ISwitchGroup interface
				) const = 0;

			/// Get a switch group from Wwise (using the switch group's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSwitchGroup( 
				LPCWSTR in_pszSwitchGroup, 			///< Name of SwitchGroup.
				ISwitchGroup ** out_ppSwitchGroup	///< Returned AddRef'd pointer to an ISwitchGroup interface.
				) const = 0;

			/// Get a list of switch groups from Wwise (using the switch groups' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSwitchGroups( 
				const AkUniqueID * in_pSwitchGroups,		///< Array of unique IDs of switch groups
				long in_cSwitchGroups, 						///< Number of switch groups in \e in_pSwitchGroups
				ISwitchGroupList ** out_ppSwitchGroupList 	///< Returned AddRef'd pointer to an ISwitchGroupList interface
				) const = 0;

			/// Get a list of switch groups from Wwise (using the switch groups' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetSwitchGroups( 
				LPCWSTR * in_pszSwitchGroups, 				///< Array of switch group names
				long in_cSwitchGroups, 						///< Number of switch groups in \e in_pszSwitchGroups
				ISwitchGroupList ** out_ppSwitchGroupList 	///< Returned AddRef'd pointer to an ISwitchGroupList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Game Parameters
			/// \aknote
			/// Game parameters and RTPCs are different concepts. For details on their relationship, please 
			/// refer to the documentation for the Wwise authoring tool.
			/// \endaknote
			//@{

			/// Get the list of all game parameters from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetGameParameterList( 
				IGameParameterList ** out_ppGameParameterList	///< Returned AddRef'd pointer to an IGameParameterList interface
				) const = 0;

			/// Get a game parameter from Wwise (using the game parameter's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetGameParameter( 
				AkUniqueID in_gameParameterID,			///< Unique ID of the game parameter
				IGameParameter ** out_ppGameParameter	///< Returned AddRef'd pointer to an IGameParameter interface
				) const = 0;

			/// Get a game parameter from Wwise (using the game parameter's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetGameParameter( 
				LPCWSTR in_pszGameParameter, 			///< Name of the game parameter
				IGameParameter ** out_ppGameParameter	///< Returned AddRef'd pointer to an IGameParameter interface
				) const = 0;

			/// Get a list of game parameters from Wwise (using the game parameters' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetGameParameters( 
				const AkUniqueID * in_pGameParameters,			///< Array of unique IDs of game parameters
				long in_cGameParameters, 						///< Number of game parameters in \e in_pGameParameters
				IGameParameterList ** out_ppGameParameterList 	///< Returned AddRef'd pointer to an IGameParameterList interface
				) const = 0;

			/// Get a list of game parameters from Wwise (using the game parameters' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetGameParameters( 
				LPCWSTR * in_pszGameParameters, 				///< Array of game parameter names
				long in_cGameParameters, 						///< Number of game parameters in \e in_pszGameParameters
				IGameParameterList ** out_ppGameParameterList 	///< Returned AddRef'd pointer to an IGameParameterList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Triggers
			//@{

			/// Get the list of all triggers from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetTriggerList( 
				ITriggerList ** out_ppTriggerList	///< Returned AddRef'd pointer to an ITriggerList interface
				) const = 0;

			/// Get a trigger from Wwise (using the trigger's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetTrigger( 
				AkUniqueID in_triggerID,	///< Unique ID of the trigger
				ITrigger ** out_ppTrigger	///< Returned AddRef'd pointer to an ITrigger interface
				) const = 0;

			/// Get a trigger from Wwise (using the trigger's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetTrigger( 
				LPCWSTR in_pszTrigger, 		///< Name of the trigger
				ITrigger ** out_ppTrigger	///< Returned AddRef'd pointer to an ITrigger interface
				) const = 0;

			/// Get a list of triggers from Wwise (using the triggers' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetTriggers( 
				const AkUniqueID * in_pTriggers,	///< Array of unique IDs of triggers
				long in_cTriggers, 					///< Number of triggers in \e in_pTriggers
				ITriggerList ** out_ppTriggerList 	///< Returned AddRef'd pointer to an ITriggerList interface
				) const = 0;

			/// Get a list of Triggers from Wwise (using the triggers' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetTriggers( 
				LPCWSTR * in_pszTriggers, 			///< Array of trigger names
				long in_cTriggers, 					///< Number of triggers in \e in_pszTriggers
				ITriggerList ** out_ppTriggerList 	///< Returned AddRef'd pointer to an ITriggerList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Environments
			//@{

			/// Get the list of all environments from Wwise.
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEnvironmentList( 
				IEnvironmentList ** out_ppEnvironmentList	///< Returned AddRef'd pointer to an IEnvironmentList interface
				) const = 0;

			/// Get an environment from Wwise (using the environment's ID).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEnvironment( 
				AkUniqueID in_environmentID,		///< Unique ID of the environment
				IEnvironment ** out_ppEnvironment	///< Returned AddRef'd pointer to an IEnvironment interface
				) const = 0;

			/// Get an environment from Wwise (using the environment's name).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEnvironment( 
				LPCWSTR in_pszEnvironment, 			///< Name of the environment
				IEnvironment ** out_ppEnvironment	///< Returned AddRef'd pointer to an IEnvironment interface
				) const = 0;

			/// Get a list of environments from Wwise (using the environments' IDs).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEnvironments( 
				const AkUniqueID * in_pEnvironments,		///< Array of unique IDs of environments
				long in_cEnvironments, 						///< Number of environments in \e in_pEnvironments
				IEnvironmentList ** out_ppEnvironmentList 	///< Returned AddRef'd pointer to an IEnvironmentList interface
				) const = 0;

			/// Get a list of environments from Wwise (using the environments' names).
			/// \return	True if the operation was successful, False otherwise
			virtual bool GetEnvironments( 
				LPCWSTR * in_pszEnvironments, 				///< Array of environment names
				long in_cEnvironments, 						///< Number of environments in \e in_pszEnvironments
				IEnvironmentList ** out_ppEnvironmentList 	///< Returned AddRef'd pointer to an IEnvironmentList interface
				) const = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Game Objects
			//@{

			/// Get the list of game objects in the sound engine that are known by Wwise.
			///
			/// \aknote
			/// Wwise will only be aware of game objects that have appeared in the current session's capture log.
			/// As such, the list returned by this function could vary as the Advanced Profiler receives more notifications.
			/// \endaknote
			///
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - AK::SoundFrame::IGameObjectList
			virtual bool GetGameObjectList( 
				IGameObjectList ** out_ppGameObjectList	///< Returned AddRef'd pointer to an IGameObjectList interface
				) const = 0;

			//@}

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Notifications
			//@{

			/// Tell Wwise to listen for live attenuation changes on the specified sound objects.\n
			/// Sending NULL and 0 as parameters will stop this monitoring.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ListenAttenuation( 
				const AkUniqueID * in_pSoundObjects,	///< Array of unique IDs of sound objects
				long in_cSoundObjects 					///< Number of sound objects in \e in_pSoundObjects
				) = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name Drag And Drop
			//@{

			/// Type of object contained in IDataObject
			enum DnDType
			{
				TypeUnknown = 0,		///< Unknown
				TypeEvent = 1,			///< Event
				TypeStates = 2,			///< State group
				TypeSwitches = 3,		///< Switch group
				TypeGameParameters = 4, ///< Game parameter
				TypeTriggers = 5,		///< Trigger
				TypeEnvironments = 6	///< Environment
			};

			/// Ask whether a data object contains one or more Sound Frame objects of the same type.
			/// \return The type of Sound Frame objects contained in the DataObject, or TypeUnknown if there are no Sound Frame objects
			virtual DnDType GetDnDType(
				IDataObject * in_pDataObject	///< OLE drag-and-drop data object
				) = 0;

			/// Get the list of events in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessEventDnD( 
				IDataObject * in_pDataObject,	///< OLE drag-and-drop data object
				IEventList ** out_ppEventList	///< Returned AddRef'd pointer to an IEventList interface
				) = 0;

			/// Get the list of state groups in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessStateGroupDnD( 
				IDataObject * in_pDataObject,			///< OLE drag-and-drop data object
				IStateGroupList ** out_ppStateGroupList	///< Returned AddRef'd pointer to an IStateGroupList interface
				) = 0;

			/// Get the list of switch groups in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessSwitchGroupDnD( 
				IDataObject * in_pDataObject,				///< OLE drag-and-drop data object
				ISwitchGroupList ** out_ppSwitchGroupList	///< Returned AddRef'd pointer to an ISwitchGroupList interface
				) = 0;

			/// Get the list of game parameters in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessGameParameterDnD( 
				IDataObject * in_pDataObject,					///< OLE drag-and-drop data object
				IGameParameterList ** out_ppGameParameterList	///< Returned AddRef'd pointer to an IGameParameterList interface
				) = 0;

			/// Get the list of triggers in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessTriggerDnD( 
				IDataObject * in_pDataObject,		///< OLE drag-and-drop data object
				ITriggerList ** out_ppTriggerList	///< Returned AddRef'd pointer to an ITriggerList interface
				) = 0;

			/// Get the list of environments in a data object.
			/// \return	True if the operation was successful, False otherwise
			virtual bool ProcessEnvironmentDnD( 
				IDataObject * in_pDataObject,				///< OLE drag-and-drop data object
				IEnvironmentList ** out_ppEnvironmentList	///< Returned AddRef'd pointer to an IEnvironmentList interface
				) = 0;

			//@}

			///////////////////////////////////////////////////////////////////////
			/// @name SoundBank Operations
			//@{

			/// Tell Wwise to immediately process definition files.
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - \ref soundframe_working_soundbanks
			virtual bool ProcessDefinitionFiles( 
				LPCWSTR * in_pszPaths, 	///< Array of paths to definition files
				long in_cFiles 			///< Number of paths in \e in_pszPaths
				) = 0;

			/// Tell Wwise to immediately generate SoundBanks.
			/// \return	True if the operation was successful, False otherwise
			/// \sa
			/// - \ref soundframe_working_soundbanks
			virtual bool GenerateSoundBanks( 
				LPCWSTR * in_pszBanks,		///< Array of bank names
				long in_cBanks, 			///< Number of banks in \e in_pszBanks
				LPCWSTR * in_pszPlatforms,	///< Array of platform names
				long in_cPlatforms,			///< Number of platforms in \e in_pszPlatforms
				LPCWSTR * in_pszLanguages,	///< Array of language names
				long in_cLanguages			///< Number of languages in \e in_pszLanguages
				) = 0;

			//@}
		};

		/// Interface that the Sound Frame client must implement.
		/// \sa
		/// - AK::SoundFrame::Create()
		class IClient
		{
		public:
			/// Notification type.
			enum Notif
			{
				Notif_Added,	///< An object was added
				Notif_Removed,	///< An object was removed
				Notif_Changed,	///< An object has been changed
				Notif_Reset,    ///< All objects in the specified type have been reset
				Notif_Push		///< The user asked for the object to be sent to the Sound Frame
			};

			/// Notification for connection status changes. This method is called after connection to or disconnection from the Wwise application occurs.
			virtual void OnConnect( 
				bool in_bConnect		///< True if Wwise is connected, False if it is not
				) = 0;

			/// Event notification. This method is called when an event is added, removed, changed, or pushed.
			virtual void OnEventNotif( 
				Notif in_eNotif,			///< Notification type
				AkUniqueID in_eventID		///< Unique ID of the event
				) = 0;

			/// Sound object notification. This method is called when a sound object is added, removed, or changed.
			virtual void OnSoundObjectNotif( 
				Notif in_eNotif,					///< Notification type
				AkUniqueID in_soundObjectID			///< Unique ID of the sound object
				) = 0;

			/// State notification. This method is called when a state group or a state is added, removed or changed.\n
			/// It is also called (with in_eNotif equal to Notif_Changed) when the current state of a state group changes.
			/// \aknote
			/// This notification will be sent for all state changes (through Wwise, the Sound Frame, or the sound engine).
			/// \endaknote
			virtual void OnStatesNotif( 
				Notif in_eNotif,			///< Notification type
				AkUniqueID in_stateGroupID	///< Unique ID of the state group
				) = 0;

			/// Switch notification. This method is called when a switch group or a switch is added, removed or changed.\n
			/// It is also called (with in_eNotif equal to Notif_Changed) when the current switch in a switch group changes on any game object.\n
			/// \aknote
			/// This notification will be sent for all switch changes (through Wwise, the Sound Frame, or the sound engine).
			/// \endaknote
			/// When all switches are reset from Wwise, this method will be called once with in_eNotif equal to Notif_Reset and 
			/// in_switchGroupID equal to AK_INVALID_UNIQUE_ID. This means that all switches have been reset to their default value
			/// on all game objects.
			virtual void OnSwitchesNotif( 
				Notif in_eNotif,			///< Notification type
				AkUniqueID in_switchGroupID	///< Unique ID of the switch group
				) = 0;

			/// Game parameter notification. This method is called when a game parameter is added, removed, or changed.
			virtual void OnGameParametersNotif( 
				Notif in_eNotif,				///< Notification type
				AkUniqueID in_gameParameterID	///< Unique ID of the game parameter
				) = 0;

			/// Trigger notification. This method is called when a trigger is added, removed, or changed.
			virtual void OnTriggersNotif( 
				Notif in_eNotif,			///< Notification type
				AkUniqueID in_triggerID		///< Unique ID of the trigger
				) = 0;

			/// Environment notification. This method is called when an environment is added, removed, or changed.
			virtual void OnEnvironmentsNotif( 
				Notif in_eNotif,				///< Notification type
				AkUniqueID in_environmentID		///< Unique ID of the environment
				) = 0;

			/// Game object notification. This method is called when a game object is registered or unregistered.\n
			/// The notification type will be Notif_Added when a game object is registered, and Notif_Removed 
			/// when its unregistered.
			/// \aknote
			/// - This notification will be sent for game object registration and unregistration made through the Sound Frame 
			/// or the sound engine.
			/// - The notification type will be Notif_Reset when all game objects are removed from the Sound Engine.
			/// \endaknote
			virtual void OnGameObjectsNotif( 
				Notif in_eNotif,				///< Notification type
				AkGameObjectID in_gameObjectID  ///< ID of the game object
				) = 0;
		};

		/// Create and return an object that implements ISoundFrame.
		/// \return	True if the operation was successful, False otherwise
		bool Create( 
			IClient * in_pClient,				///< Pointer to a client interface
			ISoundFrame ** out_ppSoundFrame		///< Returned AddRef'd pointer to an ISoundFrame interface
			);
	}
}

#endif // _AK_SOUNDFRAME_SF_H