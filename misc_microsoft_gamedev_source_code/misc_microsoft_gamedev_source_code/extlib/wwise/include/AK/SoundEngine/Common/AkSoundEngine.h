//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkSoundEngine.h

/// \file 
/// The main sound engine interface.


#ifndef _AK_SOUNDENGINE_H_
#define _AK_SOUNDENGINE_H_

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/AkCallback.h>

struct AkPlatformInitSettings;

/// Function called on assert handling, optional
/// \sa 
/// - AkInitSettings
typedef void (*AkAssertHook)( 
						const char * in_pszExpression,	///< Expression
                        const char * in_pszFileName,	///< File Name
                        int in_lineNumber				///< Line Number
						);

/// Platform-independent initialization settings of the sound engine
/// \sa 
/// - AK::SoundEngine::Init()
/// - AK::SoundEngine::GetDefaultInitSettings()
/// - \ref soundengine_integration_init_advanced
struct AkInitSettings
{
    AkAssertHook        pfnAssertHook;				///< External assertion handling function (optional)

    AkUInt32            uMaxNumPaths;				///< Maximum number of paths for positioning
    AkUInt32            uMaxNumTransitions;			///< Maximum number of transitions
    AkUInt32            uDefaultPoolSize;			///< Size of the defaut memory pool, in bytes
	AkUInt32            uCommandQueueSize;			///< Size of the command queue, in bytes
	AkMemPoolId			uPrepareEventMemoryPoolID;	///< Memory pool where data allocated by AK::SoundEngine::PrepareEvent() and AK::SoundEngine::PrepareGameSyncs() will be done. 
	bool				bEnableGameSyncPreparation;	///< Set to true to enable AK::SoundEngine::PrepareGameSync usage.
#ifndef AK_OPTIMIZED
    AkUInt32            uMonitorPoolSize;			///< Size of the monitoring pool, in bytes
    AkUInt32            uMonitorQueuePoolSize;		///< Size of the monitoring queue pool, in bytes
#endif
};

/// Audiokinetic namespace
namespace AK
{
	/// Audiokinetic sound engine namespace
	/// \remarks The functions in this namespace are thread-safe, unless stated otherwise.
	namespace SoundEngine
	{
        ///////////////////////////////////////////////////////////////////////
		/// @name Initialization
		//@{

		/// Query whether or not the sound engine has been sucessfully initialized.
		/// \warning This function is not thread-safe. It should not be called at the same time as SoundEngine::Init() or SoundEngine::Term().
		/// \return True if the sound engine has been initialized, False otherwise
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::SoundEngine::Init()
		/// - AK::SoundEngine::Term()
		extern AKSOUNDENGINE_API bool IsInitialized();

		/// Initialize the sound engine.
		/// \warning This function is not thread-safe.
		/// \remark The initial settings should be initialized using AK::SoundEngine::GetDefaultInitSettings()
		///			and AK::SoundEngine::GetDefaultPlatformInitSettings() to fill the structures with their 
		///			default settings. This is not mandatory, but it helps avoid backward compatibility problems.
		/// \return 
		/// - AK_Success if the initialization was successful
		/// - AK_MemManagerNotInitialized if the memory manager is not available or not properly initialized
		/// - AK_StreamMgrNotInitialized if the stream manager is not available or not properly initialized
		/// - AK_SSEInstructionsNotSupported if the machine does not support SSE instruction (only on the PC)
		/// - AK_InsufficientMemory or AK_Fail if there is not enough memory available to initialize the sound engine properly
		/// - AK_InvalidParameter if some parameters are invalid
		/// - AK_Fail if the sound engine is already initialized, or if the provided settings result in insufficient 
		/// ressources for the initialization.
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::SoundEngine::Term()
		/// - AK::SoundEngine::GetDefaultInitSettings()
		/// - AK::SoundEngine::GetDefaultPlatformInitSettings()
        extern AKSOUNDENGINE_API AKRESULT Init(
            AkInitSettings *			in_pSettings,   		///< Initialization settings (can be NULL, to use the default values)
            AkPlatformInitSettings *	in_pPlatformSettings  	///< Platform-specific settings (can be NULL, to use the default values)
		    );

		/// Get the default values of the platform-independent initialization settings.
		/// \warning This function is not thread-safe.
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::SoundEngine::Init()
		/// - AK::SoundEngine::GetDefaultPlatformInitSettings()
		extern AKSOUNDENGINE_API void GetDefaultInitSettings(
            AkInitSettings &			out_settings   			///< Returned default platform-independent sound engine settings
		    );

		/// Get the default values of the platform-specific initialization settings.
		/// \warning This function is not thread-safe.
		/// \sa 
		/// - \ref soundengine_integration_init_advanced
		/// - AK::SoundEngine::Init()
		/// - AK::SoundEngine::GetDefaultInitSettings()
		extern AKSOUNDENGINE_API void GetDefaultPlatformInitSettings(
            AkPlatformInitSettings &	out_platformSettings  	///< Returned default platform-specific sound engine settings
		    );

        /// Terminate the sound engine.
		/// If some sounds are still playing or events are still being processed when this function is 
		///	called, they will be stopped.
		/// \warning This function is not thread-safe.
		/// \warning Before calling Term, you must ensure that no other thread is accessing the sound engine.
		/// \sa 
		/// - \ref soundengine_integration_init_advanced
		/// - AK::SoundEngine::Init()
        extern AKSOUNDENGINE_API void Term();

		/// Get the output speaker configuration.
		/// Call this function to get the player's speaker configuration. The configuration depends on
		/// the platform, or the user's setting.
		/// \warning Call this function only after the sound engine has been properly initialized.
		/// \return One of the supported configuration: 
		/// - AK_SPEAKER_SETUP_STEREO
		/// - AK_SPEAKER_SETUP_5POINT1
		/// - AK_SPEAKER_SETUP_SURROUND (Wii only)
		/// - AK_SPEAKER_SETUP_DPL2	(Wii only)
		/// \sa 
		/// - AkCommonDefs.h
		extern AKSOUNDENGINE_API AkChannelMask GetSpeakerConfiguration();
		
        //@}

		////////////////////////////////////////////////////////////////////////
		/// @name Rendering Audio
		//@{

		/// Process all events in the sound engine's queue.
		/// This method has to be called periodically (usually once per game frame).
		/// \sa 
		/// - \ref concept_events
		/// - \ref soundengine_events
		/// - AK::SoundEngine::PostEvent()
		/// \return Always returns AK_Success
        extern AKSOUNDENGINE_API AKRESULT RenderAudio();

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Component Registration
		//@{
		
		/// Register a plug-in with the sound engine and set the callback functions to create the 
		/// plug-in and its parameter node.
		/// \sa
		/// - \ref register_effects
		/// - \ref plugin_xml
		/// \return AK_Success if successful, AK_InvalidParameter if invalid parameters were provided or Ak_Fail otherwise. Possible reasons for an AK_Fail result are:
		/// - Insufficient memory to register the plug-in
		/// - Plug-in ID already registered
		/// \remarks
		/// Codecs and plug-ins should be registered before loading banks.\n
		/// Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
        extern AKSOUNDENGINE_API AKRESULT RegisterPlugin( 
			AkPluginType in_eType,						///< Plug-in type (for example, source or effect)
			AkUInt32 in_ulCompanyID,					///< Company identifier (as declared in the plug-in description XML file)
			AkUInt32 in_ulPluginID,						///< Plug-in identifier (as declared in the plug-in description XML file)
			AkCreatePluginCallback in_pCreateFunc,		///< Pointer to the plug-in's creation function
            AkCreateParamCallback in_pCreateParamFunc	///< Pointer to the plug-in's parameter node creation function
            );
		
		/// Register a codec type with the sound engine and set the callback functions to create the 
		/// codec's file source and bank source nodes.
		/// \sa 
		/// - \ref register_effects
		/// \return AK_Success if successful, AK_InvalidParameter if invalid parameters were provided, or Ak_Fail otherwise. Possible reasons for an AK_Fail result are:
		/// - Insufficient memory to register the codec
		/// - Codec ID already registered
		/// \remarks
		/// Codecs and plug-ins should be registered before loading banks.\n
		/// Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
        extern AKSOUNDENGINE_API AKRESULT RegisterCodec( 
			AkUInt32 in_ulCompanyID,						///< Company identifier (as declared in the plug-in description XML file)
			AkUInt32 in_ulCodecID,							///< Codec identifier (as declared in the plug-in description XML file)
			AkCreateFileSourceCallback in_pFileCreateFunc,	///< Pointer to the codec's file source node creation function
            AkCreateBankSourceCallback in_pBankCreateFunc	///< Pointer to the codec's bank source node creation function
            );

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Getting ID from strings
		//@{

		/// Universal converter from string to ID for the sound engine.
		/// This function will hash the name based on a algorithm ( provided at : /AK/Tools/Common/AkFNVHash.h )
		/// Note:
		///		This function does return a AkUInt32, which is totally compatible with:
		///		AkUniqueID, AkStateGroupID, AkStateID, AkSwitchGroupID, AkSwitchStateID, AkRtpcID, AkEnvID, and so on...
		/// \sa
		/// - AK::SoundEngine::PostEvent
		/// - AK::SoundEngine::SetRTPCValue
		/// - AK::SoundEngine::SetSwitch
		/// - AK::SoundEngine::SetState
		/// - AK::SoundEngine::PostTrigger
		/// - AK::SoundEngine::SetEnvironmentVolume
		/// - AK::SoundEngine::BypassEnvironment
		/// - AK::SoundEngine::SetGameObjectEnvironmentsValues
		/// - AK::SoundEngine::LoadBank
		/// - AK::SoundEngine::UnloadBank
		/// - AK::SoundEngine::PrepareEvent
		/// - AK::SoundEngine::PrepareGameSyncs
		extern AKSOUNDENGINE_API AkUInt32 GetIDFromString( AkLpCtstr in_pszString );

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Event Management
		//@{

		/// Asynchronously post an event to the sound engine (by event ID).
		/// The callback function can be used to be noticed when markers are reached or when the event is finished.
        /// \return The playing ID of the event launched, or AK_INVALID_PLAYING_ID if posting the event failed
		/// \sa 
		/// - \ref concept_events
		/// - AK::SoundEngine::RenderAudio()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::GetSourcePlayPosition()
        extern AKSOUNDENGINE_API AkPlayingID PostEvent(
	        AkUniqueID in_eventID,							///< Unique ID of the event
	        AkGameObjectID in_gameObjectID,					///< Associated game object ID
			AkUInt32 in_uFlags = 0,							///< Bitmask: see \ref AkCallbackType
			AkCallbackFunc in_pfnCallback = NULL,			///< Callback function
			void * in_pCookie = NULL						///< Callback cookie that will be sent to the callback function along with additional information
	        );

		/// Post an event to the sound engine (by event name), using callbacks.
		/// The callback function can be used to be noticed when markers are reached or when the event is finished.
        /// \return The playing ID of the event launched, or AK_INVALID_PLAYING_ID if posting the event failed
		/// \sa 
		/// - \ref concept_events
		/// - AK::SoundEngine::RenderAudio()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::GetSourcePlayPosition()
        extern AKSOUNDENGINE_API AkPlayingID PostEvent(
	        AkLpCtstr in_pszEventName,						///< Name of the event
	        AkGameObjectID in_gameObjectID,					///< Associated game object ID
			AkUInt32 in_uFlags = 0,							///< Bitmask: see \ref AkCallbackType
			AkCallbackFunc in_pfnCallback = NULL,			///< Callback function
			void * in_pCookie = NULL						///< Callback cookie that will be sent to the callback function along with additional information.
	        );

		/// Cancel all event callbacks associated with a specific callback cookie.\n
		/// \sa 
		/// - AK::SoundEngine::PostEvent()
		extern AKSOUNDENGINE_API void CancelEventCallbackCookie( 
			void * in_pCookie 							///< Callback cookie to be cancelled
			);

		/// Cancel all event callbacks for a specific playing ID.
		/// \sa 
		/// - AK::SoundEngine::PostEvent()
		extern AKSOUNDENGINE_API void CancelEventCallback( 
			AkPlayingID in_playingID 					///< Playing ID of the event that must not use callbacks
			);

		/// Get the current position of the source associated with this playing ID.
		/// \return AK_Success if successful.
		///			It returns AK_InvalidParameter if the provided pointer is not valid.
		///			It returns Ak_Fail if an error occured.
		/// \sa 
		/// - \ref soundengine_query_pos
		/// - \ref concept_events
		extern AKSOUNDENGINE_API AKRESULT GetSourcePlayPosition(
			AkPlayingID		in_PlayingID,				///< Playing ID returned by AK::SoundEngine::PostEvent()
			AkTimeMs*		out_puPosition				///< Position of the source (in ms) associated with that playing ID
			);

		/// Stop the current content playing associated to the specified game object ID.
		/// If no game object is specified, all sounds will be stopped.
		extern AKSOUNDENGINE_API void StopAll( 
			AkGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT ///< (Optional)Specify a game object to stop only playback associated to the provided game object ID.
			);

		/// Stop the current content playing associated to the specified playing ID.
		extern AKSOUNDENGINE_API void StopPlayingID( 
			AkPlayingID in_playingID					///< Playing ID to be stopped.
			);

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Game Objects
		//@{
		
        /// Register a game object.
		/// \return
		/// - AK_Success if successful
		///	- AK_Fail if the specified AkGameObjectID is invalid (0 and -1 are invalid)
		/// \remark Registering a game object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered.
		/// \sa 
		/// - AK::SoundEngine::UnregisterGameObj()
		/// - AK::SoundEngine::UnregisterAllGameObj()
		/// - \ref concept_gameobjects
        extern AKSOUNDENGINE_API AKRESULT RegisterGameObj(
	        AkGameObjectID in_gameObjectID				///< ID of the game object to be registered
	        );

		/// Register a game object.
		/// \return
		/// - AK_Success if successful
		///	- AK_Fail if the specified AkGameObjectID is invalid (0 and -1 are invalid)
		/// \remark Registering a game object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered.
		/// \sa 
		/// - AK::SoundEngine::UnregisterGameObj()
		/// - AK::SoundEngine::UnregisterAllGameObj()
		/// - \ref concept_gameobjects
        extern AKSOUNDENGINE_API AKRESULT RegisterGameObj(
	        AkGameObjectID in_gameObjectID,				///< ID of the game object to be registered
			const char * in_pszObjName					///< Name of the game object (for monitoring purpose)
	        );

        /// Unregister a game object.
		/// \return 
		/// - AK_Success if successful
		///	- AK_Fail if the specified AkGameObjectID is invalid (0 is an invalid ID)
		/// \remark Registering a game object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered. Unregistering a game object while it is 
		///			in use is allowed, but the control over the parameters of this game object is lost.
		///			For example, say a sound associated with this game object is a 3D moving sound. This sound will 
		///			stop moving when the game object is unregistered, and there will be no way to regain control over the game object.
		/// \sa 
		/// - AK::SoundEngine::RegisterGameObj()
		/// - AK::SoundEngine::UnregisterAllGameObj()
		/// - \ref concept_gameobjects
        extern AKSOUNDENGINE_API AKRESULT UnregisterGameObj(
	        AkGameObjectID in_gameObjectID				///< ID of the game object to be unregistered. Use 
	        											/// AK_INVALID_GAME_OBJECT to unregister all game objects.
	        );

        /// Unregister all game objects.
		/// \return Always returns AK_Success
		/// \remark Registering a game object twice does nothing. Unregistering it once unregisters it no 
		///			matter how many times it has been registered. Unregistering a game object while it is 
		///			in use is allowed, but the control over the parameters of this game object is lost.
		///			For example, if a sound associated with this game object is a 3D moving sound, it will 
		///			stop moving once the game object is unregistered, and there will be no way to recover 
		///			the control over this game object.
		/// \sa 
		/// - AK::SoundEngine::RegisterGameObj()
		/// - AK::SoundEngine::UnregisterGameObj()
		/// - \ref concept_gameobjects
        extern AKSOUNDENGINE_API AKRESULT UnregisterAllGameObj();

       	/// Set the position of a game object.
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_3dpositions
        extern AKSOUNDENGINE_API AKRESULT SetPosition( 
			AkGameObjectID in_GameObjectID,				///< Game object identifier
			const AkSoundPosition & in_Position,		///< Position to set
			AkUInt32 in_ulListenerIndex = AK_INVALID_LISTENER_INDEX	///< If the listener index is valid, the listener position is used instead of the game object position.
		    );
        
        //@}

		////////////////////////////////////////////////////////////////////////
		/// @name Bank Management
		//@{

		/// Unload all currently loaded banks.
		/// It also internally calls ClearPreparedEvents() since at least one bank must have been loaded to allow preparing events.
		/// \return 
		/// - AK_Success if successful
		///	- AK_Fail if the sound engine was not correctly initialized or if there is not enough memory to handle the command
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::LoadBank()
		/// - \ref soundengine_banks
		extern AKSOUNDENGINE_API AKRESULT ClearBanks();

		/// Set the I/O settings of the bank load and prepare event processes.
        /// The sound engine uses default values unless explicitly set by calling this method.
		/// \warning This function must be called before loading banks.
		/// \return 
		/// - AK_Success if successful
		/// - AK_Fail if the sound engine was not correctly initialized
		/// - AK_InvalidParameter if some parameters are invalid
		/// \sa 
		/// - \ref soundengine_banks
        /// - \ref streamingdevicemanager
        extern AKSOUNDENGINE_API AKRESULT SetBankLoadIOSettings(
            AkReal32            in_fThroughput,         ///< Average throughput of bank data streaming (bytes/ms) (the default value is AK_DEFAULT_BANK_THROUGHPUT)
            AkPriority          in_priority             ///< Priority of bank streaming (the default value is AK_DEFAULT_PRIORITY)
            );

		/// Load a bank synchronously (by string).\n
		/// The bank name is passed to the Stream Manager.
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// You can specify a custom pool for storage of media, or use the sound engine's default pool.
		/// A bank load request will be posted, and consumed by the Bank Manager thread.
		/// The function returns when the request has been completely processed.
		/// \return 
		/// The bank ID, which is obtained by hashing the bank name (see GetIDFromString()). 
		/// You may use this ID with UnloadBank().
		///	- AK_Success: Load or unload successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - The sound engine internally calls GetIDFromString(in_pszString) to return the correct bank ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. The path should be resolved in 
		/// your implementation of the Stream Manager, or in the Low-Level I/O module if you use the default Stream Manager's implementation.
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AK::SoundEngine::GetIDFromString
		/// - AK::MemoryMgr::CreatePool()
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_lowlevel
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        AkLpCtstr           in_pszString,		    ///< Name of the bank to load
            AkMemPoolId         in_memPoolId,			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
            AkBankID &          out_bankID				///< Returned bank ID
	        );

        /// Load a bank synchronously (by ID).\n
		/// The bank ID is passed to the Stream Manager.
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// You can specify a custom pool for storage of media, or use the sound engine's default pool.
		/// A bank load request will be posted, and consumed by the Bank Manager thread.
		/// The function returns when the request has been completely processed.
		/// \return 
		///	- AK_Success: Load or unload successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AK::MemoryMgr::CreatePool()
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        AkBankID			in_bankID,              ///< Bank ID of the bank to load
            AkMemPoolId         in_memPoolId			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
            );

		/// Load a bank synchronously (by in-memory data).\n
		/// Use this overload when you want to manage I/O on your side. Load the bank file
		/// in a buffer and pass its address to the sound engine.
		/// In-memory loading is in-place: the memory must be valid until the bank is unloaded.
		/// A bank load request will be posted, and consumed by the Bank Manager thread.
		/// The function returns when the request has been completely processed.
		/// \return 
		/// The bank ID, which is stored in the first few bytes of the bank file. You may use this 
		/// ID with UnloadBank().
		///	- AK_Success: Load or unload successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - The memory must be aligned on platform-specific AK_BANK_PLATFORM_DATA_ALIGNMENT bytes (see AkTypes.h).
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        void *				in_pInMemoryBankPtr,	///< Pointer to the in-memory bank to load
			AkUInt32			in_uInMemoryBankSize,	///< Size of the in-memory bank to load
            AkBankID &          out_bankID				///< Returned bank ID
	        );

        /// Load a bank asynchronously (by string).\n
		/// The bank name is passed to the Stream Manager.
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// You can specify a custom pool for storage of media, or use the sound engine's default pool.
		/// A bank load request will be posted to the Bank Manager consumer thread.
		/// The function returns immediately.
		/// \return 
		/// AK_Success if the scheduling was successful, AK_Fail otherwise.
		/// Use a callback to be notified when completed, and get the status of the request.
		/// The bank ID, which is obtained by hashing the bank name (see GetIDFromString()). 
		/// You may use this ID with UnloadBank().
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - The sound engine internally calls GetIDFromString(in_pszString) to return the correct bank ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. The path should be resolved in 
		/// your implementation of the Stream Manager, or in the Low-Level I/O module if you use the default Stream Manager's implementation.
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AK::MemoryMgr::CreatePool()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_lowlevel
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        AkLpCtstr           in_pszString,           ///< Name/path of the bank to load
			AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
			void *              in_pCookie,				///< Callback cookie (reserved to user, passed to the callback function)
            AkMemPoolId         in_memPoolId,			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
			AkBankID &          out_bankID				///< Returned bank ID
	        );

        /// Load a bank asynchronously (by ID).\n
		/// The bank ID is passed to the Stream Manager.
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// You can specify a custom pool for storage of media, or use the sound engine's default pool.
		/// A bank load request will be posted to the Bank Manager consumer thread.
		/// The function returns immediately.
		/// \return 
		/// AK_Success if the scheduling was successful, AK_Fail otherwise.
		/// Use a callback to be notified when completed, and get the status of the request.
		/// The bank ID, which is obtained by hashing the bank name (see GetIDFromString()). 
		/// You may use this ID with UnloadBank().
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AK::MemoryMgr::CreatePool()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        AkBankID			in_bankID,              ///< Bank ID of the bank to load
			AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
			void *              in_pCookie,				///< Callback cookie (reserved to user, passed to the callback function)
            AkMemPoolId         in_memPoolId			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
	        );

		/// Load a bank asynchronously (by in-memory data).\n
		/// Use this overload when you want to manage I/O on your side. Load the bank file
		/// in a buffer and pass its address to the sound engine.
		/// In-memory loading is in-place: the memory must be valid until the bank is unloaded.
		/// A bank load request will be posted to the Bank Manager consumer thread.
		/// The function returns immediately.
		/// \return 
		/// AK_Success if the scheduling was successful, AK_Fail otherwise, or AK_InvalidParameter if memory alignment is not correct.
		/// Use a callback to be notified when completed, and get the status of the request.
		/// The bank ID, which is obtained by hashing the bank name (see GetIDFromString()). 
		/// You may use this ID with UnloadBank().
		/// \remarks
		/// - The initialization bank must be loaded first.
		/// - Codecs and plug-ins should be registered before loading banks.
		/// - Loading a bank referencing an unregistered plug-in or codec will result in a load bank success,
		/// but the plug-ins will not be used. More specifically, playing a sound that uses an unregistered effect plug-in 
		/// will result in audio playback without applying the said effect. If an unregistered source plug-in is used by an event's audio objects, 
		/// posting the event will fail.
		/// - The memory must be aligned on platform-specific AK_BANK_PLATFORM_DATA_ALIGNMENT bytes (see AkTypes.h).
		/// - Requesting to load a bank in a different memory pool than where the bank was previously loaded must be done only
		/// after receiving confirmation by the callback that the bank was completely unloaded or by usung synchrone versions
		/// of the UnloadBank function.
		/// \sa 
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref integrating_elements_plugins
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT LoadBank(
	        void *				in_pInMemoryBankPtr,	///< Pointer to the in-memory bank to load
			AkUInt32			in_uInMemoryBankSize,	///< Size of the in-memory bank to load
			AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
			void *              in_pCookie,				///< Callback cookie
			AkBankID &          out_bankID				///< Returned bank ID
	        );

        /// Unload a bank synchronously (by string).\n
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// \return AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.
		/// \remarks
		/// - If you provided a pool memory ID when loading this bank, it is returned as well. 
		/// Otherwise, the function returns AK_DEFAULT_POOL_ID.
		/// - The sound engine internally calls GetIDFromString(in_pszString) to retrieve the bank ID, 
		/// then it calls the synchronous version of UnloadBank() by ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. 
		/// \sa 
		/// - AK::SoundEngine::LoadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - \ref soundengine_banks
        extern AKSOUNDENGINE_API AKRESULT UnloadBank(
	        AkLpCtstr           in_pszString,           ///< Name of the bank to unload
	        AkMemPoolId *       out_pMemPoolId = NULL   ///< Returned memory pool ID used with LoadBank() (can pass NULL)
	        );

        /// Unload a bank synchronously (by ID).\n
		/// \return AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.
		/// \remarks
		/// If you provided a pool memory ID when loading this bank, it is returned as well. 
		/// Otherwise, the function returns AK_DEFAULT_POOL_ID.
		/// \sa 
		/// - AK::SoundEngine::LoadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - \ref soundengine_banks
        extern AKSOUNDENGINE_API AKRESULT UnloadBank(
	        AkBankID            in_bankID,              ///< ID of the bank to unload
            AkMemPoolId *       out_pMemPoolId = NULL   ///< Returned memory pool ID used with LoadBank() (can pass NULL)
	        );

		/// Unload a bank asynchronously (by string).\n
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// \return AK_Success if scheduling successful (use a callback to be notified when completed)
		/// \remarks
		/// The sound engine internally calls GetIDFromString(in_pszString) to retrieve the bank ID, 
		/// then it calls the synchronous version of UnloadBank() by ID.
		/// Therefore, in_pszString should be the real name of the SoundBank (with or without the "bnk" extension - it is trimmed internally),
		/// not the name of the file (if you changed it), nor the full path of the file. 
		/// \sa 
		/// - AK::SoundEngine::LoadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		extern AKSOUNDENGINE_API AKRESULT UnloadBank(
	        AkLpCtstr           in_pszString,           ///< Name of the bank to unload
			AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
			void *              in_pCookie 				///< Callback cookie (reserved to user, passed to the callback function)
	        );
	    
        /// Unload a bank asynchronously (by ID).\n
		/// Refer to \ref soundengine_banks_general for a discussion on using strings and IDs.
		/// \return AK_Success if scheduling successful (use a callback to be notified when completed)
		/// \sa 
		/// - AK::SoundEngine::LoadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		extern AKSOUNDENGINE_API AKRESULT UnloadBank(
	        AkBankID            in_bankID,				///< ID of the bank to unload
			AkBankCallbackFunc  in_pfnBankCallback,		///< Callback function
			void *              in_pCookie				///< Callback cookie (reserved to user, passed to the callback function)
	        );

		/// Cancel all event callbacks associated with a specific callback cookie specified while loading Banks of preparing events.\n
		/// \sa 
		/// - AK::SoundEngine::LoadBank()
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::UnloadBank()
		/// - AK::SoundEngine::ClearBanks()
		/// - AkBankCallbackFunc
		extern AKSOUNDENGINE_API void CancelBankCallbackCookie( 
			void * in_pCookie 							///< Callback cookie to be cancelled
			);

		/// Preparation type.
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::PrepareGameSyncs()
		enum PreparationType
		{
			Preparation_Load,	///< PrepareEvent will load required information to play the specified event.
			Preparation_Unload	///< PrepareEvent will unload required information to play the specified event.
		};

		/// Clear all previously prepared events.\n
		/// \return
		/// - AK_Success if successful.
		///	- AK_Fail if the sound engine was not correctly initialized or if there is not enough memory to handle the command.
		/// \remarks
		/// The function ClearBanks() also clears all prepared events.
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::ClearBanks()
		extern AKSOUNDENGINE_API AKRESULT ClearPreparedEvents();

		/// Prepare or un-prepare events synchronously (by string).\n
		/// The events are identified by strings, and converted to IDs internally
		/// (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The event definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully post the events specified, and load the 
		/// required banks, if applicable. 
		/// The function returns when the request is completely processed.
		/// \return 
		///	- AK_Success: Prepare/un-prepare successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_IDNotFound: At least one of the event/game sync identifiers passed to PrepareEvent() does not exist.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// Whenever at least one event fails to be resolved, the actions performed for all 
		/// other events are cancelled.
		/// \remarks
		/// To learn more about how PrepareEvent() works (including multiple calls to
		/// prepare/unprepare the same events and how this works with LoadBank()), please
		/// refer to the following Knowledge Base articles:
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=74
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=75
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=148
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::ClearPreparedEvents()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::LoadBank()
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareEvent( 
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkLpCtstr*			in_ppszString,			///< Array of event names
			AkUInt32			in_uNumEvent			///< Number of events in the array
			);

		/// Prepare or un-prepare events synchronously (by ID).
		/// The events are identified by their ID (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The event definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully post the events specified, and load the 
		/// required banks, if applicable. 
		/// The function returns when the request is completely processed.
		/// \return 
		///	- AK_Success: Prepare/un-prepare successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_IDNotFound: At least one of the event/game sync identifiers passed to PrepareEvent() does not exist.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// Whenever at least one event fails to be resolved, the actions performed for all 
		/// other events are cancelled.
		/// \remarks
		/// To learn more about how PrepareEvent() works (including multiple calls to
		/// prepare/unprepare the same events and how this works with LoadBank()), please
		/// refer to the following Knowledge Base articles:
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=74
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=75
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=148
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::ClearPreparedEvents()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::LoadBank()
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareEvent( 
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkUniqueID*			in_pEventID,			///< Array of event IDs
			AkUInt32			in_uNumEvent			///< Number of event IDs in the array
			);

		/// Prepare or un-prepare an event asynchronously (by string).
		/// The events are identified by string (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The event definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully post the events specified, and load the 
		/// required banks, if applicable. 
		/// The function returns immediately. Use a callback to be notified when the request has finished being processed.
		/// \return AK_Success if scheduling is was successful, AK_Fail otherwise.
		/// \remarks
		/// Whenever at least one event fails to be resolved, the actions performed for all 
		/// other events are cancelled.
		/// \remarks
		/// To learn more about how PrepareEvent() works (including multiple calls to
		/// prepare/unprepare the same events and how this works with LoadBank()), please
		/// refer to the following Knowledge Base articles:
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=74
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=75
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=148
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::ClearPreparedEvents()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::LoadBank()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareEvent( 
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkLpCtstr*			in_ppszString,			///< Array of event names
			AkUInt32			in_uNumEvent,			///< Number of events in the array
			AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
			void *              in_pCookie				///< Callback cookie (reserved to user, passed to the callback function)
			);

		/// Prepare or un-prepare events asynchronously (by ID).\n
		/// The events are identified by their ID (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The event definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully post the events specified, and load the 
		/// required banks, if applicable. 
		/// The function returns immediately. Use a callback to be notified when the request has finished being processed.
		/// \return AK_Success if scheduling is was successful, AK_Fail otherwise.
		/// \remarks
		/// Whenever at least one event fails to be resolved, the actions performed for all 
		/// other events are cancelled.
		/// \remarks
		/// To learn more about how PrepareEvent() works (including multiple calls to
		/// prepare/unprepare the same events and how this works with LoadBank()), please
		/// refer to the following Knowledge Base articles:
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=74
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=75
		/// - http://www.audiokinetic.com/goto.php?target=kb&article=148
		/// \sa
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::ClearPreparedEvents()
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::LoadBank()
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareEvent( 
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkUniqueID*			in_pEventID,			///< Array of event IDs
			AkUInt32			in_uNumEvent,			///< Number of event IDs in the array
			AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
			void *              in_pCookie				///< Callback cookie (reserved to user, passed to the callback function)
			);

		/// Prepare or un-prepare game syncs synchronously (by string).\n
		/// The group and game syncs are specified by string (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The game syncs definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully set this game sync group to one of the
		/// game sync values specified, and load the required banks, if applicable. 
		/// The function returns when the request has been completely processed. 
		/// \return 
		///	- AK_Success: Prepare/un-prepare successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_IDNotFound: At least one of the event/game sync identifiers passed to PrepareGameSyncs() does not exist.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// You need to call PrepareGameSyncs() if the sound engine was initialized with AkInitSettings::bEnableGameSyncPreparation 
		/// set to true. When set to false, the sound engine automatically prepares all game syncs when preparing events,
		/// so you never need to call this function.
		/// \sa 
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::LoadBank()
		/// - AkInitSettings
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareGameSyncs(
			PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkGroupType		in_eGameSyncType,			///< The type of game sync.
			AkLpCtstr		in_pszGroupName,			///< The state group Name or the Switch Group Name.
			AkLpCtstr*		in_ppszGameSyncName,		///< The specific ID of the state to either support or not support.
			AkUInt32		in_uNumGameSyncs			///< The number of game sync in the string array.
			);

		/// Prepare or un-prepare game syncs synchronously (by ID).\n
		/// The group and game syncs are specified by ID (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The game syncs definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully set this game sync group to one of the
		/// game sync values specified, and load the required banks, if applicable. 
		/// The function returns when the request has been completely processed. 
		/// \return 
		///	- AK_Success: Prepare/un-prepare successful.
		///	- AK_Cancelled: Load or unload was cancelled by user request.
		/// - AK_IDNotFound: At least one of the event/game sync identifiers passed to PrepareGameSyncs() does not exist.
		/// - AK_InsufficientMemory: Insufficient memory to store bank data.
		/// - AK_BankReadError: I/O error.
		/// - AK_WrongBankVersion: Invalid bank version: make sure the version of Wwise that 
		/// you used to generate the SoundBanks matches that of the SDK you are currently using.
		/// - AK_InvalidFile: File specified could not be opened.
		/// - AK_InvalidParameter: Invalid parameter, invalid memory alignment.		
		/// - AK_Fail: Load or unload failed for any other reason.
		/// \remarks
		/// You need to call PrepareGameSyncs() if the sound engine was initialized with AkInitSettings::bEnableGameSyncPreparation 
		/// set to true. When set to false, the sound engine automatically prepares all game syncs when preparing events,
		/// so you never need to call this function.
		/// \sa 
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::LoadBank()
		/// - AkInitSettings
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareGameSyncs(
			PreparationType	in_PreparationType,			///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkGroupType		in_eGameSyncType,			///< The type of game sync.
			AkUInt32		in_GroupID,					///< The state group ID or the Switch Group ID.
			AkUInt32*		in_paGameSyncID,			///< Array of ID of the gamesyncs to either support or not support.
			AkUInt32		in_uNumGameSyncs			///< The number of game sync ID in the array.
			);

		/// Prepare or un-prepare game syncs asynchronously (by string).\n
		/// The group and game syncs are specified by string (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The game syncs definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully set this game sync group to one of the
		/// game sync values specified, and load the required banks, if applicable. 
		/// The function returns immediately. Use a callback to be notified when the request has finished being processed.
		/// \return AK_Success if scheduling is was successful, AK_Fail otherwise.
		/// \remarks
		/// You need to call PrepareGameSyncs() if the sound engine was initialized with AkInitSettings::bEnableGameSyncPreparation 
		/// set to true. When set to false, the sound engine automatically prepares all game syncs when preparing events,
		/// so you never need to call this function.
		/// \sa 
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::LoadBank()
		/// - AkInitSettings
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareGameSyncs(
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkGroupType			in_eGameSyncType,		///< The type of game sync.
			AkLpCtstr			in_pszGroupName,		///< The state group Name or the Switch Group Name.
			AkLpCtstr*			in_ppszGameSyncName,	///< The specific ID of the state to either support or not support.
			AkUInt32			in_uNumGameSyncs,		///< The number of game sync in the string array.
			AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
			void *				in_pCookie				///< Callback cookie (reserved to user, passed to the callback function)
			);

		/// Prepare or un-prepare game syncs asynchronously (by ID).\n
		/// The group and game syncs are specified by ID (refer to \ref soundengine_banks_general for a discussion on using strings and IDs).
		/// The game syncs definitions must already exist in the sound engine by having
		/// explicitly loaded the bank(s) that contain them (with LoadBank()).
		/// A request is posted to the Bank Manager consumer thread. It will resolve all 
		/// dependencies needed to successfully set this game sync group to one of the
		/// game sync values specified, and load the required banks, if applicable. 
		/// The function returns immediately. Use a callback to be notified when the request has finished being processed.
		/// \return AK_Success if scheduling is was successful, AK_Fail otherwise.
		/// \remarks
		/// You need to call PrepareGameSyncs() if the sound engine was initialized with AkInitSettings::bEnableGameSyncPreparation 
		/// set to true. When set to false, the sound engine automatically prepares all game syncs when preparing events,
		/// so you never need to call this function.
		/// \sa 
		/// - AK::SoundEngine::GetIDFromString()
		/// - AK::SoundEngine::PrepareEvent()
		/// - AK::SoundEngine::LoadBank()
		/// - AkInitSettings
		/// - AkBankCallbackFunc
		/// - \ref soundengine_banks
		/// - \ref sdk_bank_training
		extern AKSOUNDENGINE_API AKRESULT PrepareGameSyncs(
			PreparationType		in_PreparationType,		///< Preparation type ( Preparation_Load or Preparation_Unload )
			AkGroupType			in_eGameSyncType,		///< The type of game sync.
			AkUInt32			in_GroupID,				///< The state group ID or the Switch Group ID.
			AkUInt32*			in_paGameSyncID,		///< Array of ID of the gamesyncs to either support or not support.
			AkUInt32			in_uNumGameSyncs,		///< The number of game sync ID in the array.
			AkBankCallbackFunc	in_pfnBankCallback,		///< Callback function
			void *				in_pCookie				///< Callback cookie (reserved to user, passed to the callback function)
			);

	    //@}


		////////////////////////////////////////////////////////////////////////
		/// @name Listeners
		//@{

		/// Set a game object's active listeners.
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_listeners_multi_assignobjects
		extern AKSOUNDENGINE_API AKRESULT SetActiveListeners(
			AkGameObjectID in_GameObjectID,				///< Game object identifier
			AkUInt32 in_uListenerMask					///< Bitmask representing the active listeners (LSB = Listener 0, set to 1 means active)
			);

		/// Set a listener's position.
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_listeners_settingpos
        extern AKSOUNDENGINE_API AKRESULT SetListenerPosition( 
			const AkListenerPosition & in_Position,		///< Position to set
			AkUInt32 in_uIndex = 0 						///< Listener index (0: first listener, 7: 8th listener)
		    );

		/// Set a listener's spatialization parameters. This allows you to define listener-specific 
		/// volume offsets for each audio channel.
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_listeners_spatial
		extern AKSOUNDENGINE_API AKRESULT SetListenerSpatialization(
			AkUInt32 in_uIndex,							///< Listener index (0: first listener, 7: 8th listener)
			bool in_bSpatialized,						///< Spatialization toggle (True : enable spatialization, False : disable spatialization)
			AkSpeakerVolumes * in_pVolumeOffsets = NULL	///< Per-speaker volume offset, in dB (Only used if in_bSpatialized == false)
			);

		/// Set a listener's ability to listen to audio and motion events.
		/// \return Always returns AK_Success
		/// \sa 
		extern AKSOUNDENGINE_API AKRESULT SetListenerPipeline(
			AkUInt32 in_uIndex,						///< Listener index (0: first listener, 7: 8th listener)
			bool in_bAudio,							///< True=Listens to audio events (by default it is true)
			bool in_bMotion							///< True=Listens to motion events (by default it is false)
			);
	    //@}


		////////////////////////////////////////////////////////////////////////
		/// @name Game Syncs
		//@{

		/// Set the value of a real-time parameter control (by ID).
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_rtpc
		/// - AK::SoundEngine::GetIDFromString()
        extern AKSOUNDENGINE_API AKRESULT SetRTPCValue( 
			AkRtpcID in_rtpcID, 						///< ID of the RTPC
			AkRtpcValue in_value, 						///< Value to set
			AkGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT ///< Associated game object ID
		    );

		/// Set the value of a real-time parameter control (by name).
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the RTPC name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "include string" option.
		/// \aknote Strings are case-sensitive. \endaknote
		/// \sa 
		/// - \ref soundengine_rtpc
        extern AKSOUNDENGINE_API AKRESULT SetRTPCValue( 
			AkLpCtstr in_pszRtpcName,					///< Name of the RTPC
			AkRtpcValue in_value, 						///< Value to set
			AkGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT ///< Associated game object ID
		    );
   
		/// Set the state of a switch group (by IDs).
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_switch
		/// - AK::SoundEngine::GetIDFromString()
        extern AKSOUNDENGINE_API AKRESULT SetSwitch( 
			AkSwitchGroupID in_switchGroup, 			///< ID of the switch group
			AkSwitchStateID in_switchState, 			///< ID of the switch
			AkGameObjectID in_gameObjectID				///< Associated game object ID
		    );

		/// Set the state of a switch group (by names).
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the switch or switch group name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "include string" option.
		/// \aknote Strings are case-sensitive. \endaknote
		/// \sa 
		/// - \ref soundengine_switch
        extern AKSOUNDENGINE_API AKRESULT SetSwitch( 
			AkLpCtstr in_pszSwitchGroup,				///< Name of the switch group
			AkLpCtstr in_pszSwitchState, 				///< Name of the switch
			AkGameObjectID in_gameObjectID				///< Associated game object ID
			);

		/// Post the specified trigger (by IDs).
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_triggers
		/// - AK::SoundEngine::GetIDFromString()
		extern AKSOUNDENGINE_API AKRESULT PostTrigger( 
			AkTriggerID 	in_triggerID, 				///< ID of the trigger
			AkGameObjectID 	in_gameObjectID				///< Associated game object ID
		    );

		/// Post the specified trigger (by name).
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the trigger name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "include string" option.
		/// \aknote Strings are case-sensitive. \endaknote
		/// \sa 
		/// - \ref soundengine_triggers
        extern AKSOUNDENGINE_API AKRESULT PostTrigger( 
			AkLpCtstr in_pszTrigger,					///< Name of the trigger
			AkGameObjectID in_gameObjectID				///< Associated game object ID
			);

		/// Set the state of a state group (by IDs).
		/// \return Always returns AK_Success
		/// \sa 
		/// - \ref soundengine_states
		/// - AK::SoundEngine::GetIDFromString()
        extern AKSOUNDENGINE_API AKRESULT SetState( 
			AkStateGroupID in_stateGroup, 				///< ID of the state group
			AkStateID in_state 							///< ID of the state
		    );

		/// Set the state of a state group (by names).
		/// \return 
		/// - AK_Success if successful
		/// - AK_IDNotFound if the state or state group name was not resolved to an existing ID\n
		/// Make sure that the banks were generated with the "include string" option.
		/// \aknote Strings are case-sensitive. \endaknote
		/// \sa 
		/// - \ref soundengine_states
		/// - AK::SoundEngine::GetIDFromString()
        extern AKSOUNDENGINE_API AKRESULT SetState( 
			AkLpCtstr in_pszStateGroup,					///< Name of the state group
			AkLpCtstr in_pszState 						///< Name of the state
			);

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Environments
		//@{

		/// Set the environmental ratios to be used for the specified game object
		/// The array size cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ.
		/// To clear the game object's environments, in_uNumEnvValues must be 0.
		/// \aknote The actual maximum number of environments in which a game object can be is AK_MAX_ENVIRONMENTS_PER_OBJ. \endaknote
		/// \sa 
		/// - \ref soundengine_environments
		/// - \ref soundengine_environments_setting_environments
		/// - \ref soundengine_environments_id_vs_string
		/// - AK::SoundEngine::GetIDFromString()
		/// \return 
		/// - AK_Success if successful
		///	- AK_InvalidParameter if the array size exceeds AK_MAX_ENVIRONMENTS_PER_OBJ, or if a duplicated environment is found in the array
		extern AKSOUNDENGINE_API AKRESULT SetGameObjectEnvironmentsValues( 
			AkGameObjectID		in_gameObjectID,		///< Associated game object ID
			AkEnvironmentValue*	in_aEnvironmentValues,	///< Variable-size array of AkEnvironmentValue structures
														///< (it may be NULL if no environment must be set, and its size 
														///< cannot exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
			AkUInt32			in_uNumEnvValues		///< The number of environments at the pointer's address
														///< (it must be 0 if no environment is set, and can not exceed AK_MAX_ENVIRONMENTS_PER_OBJ)
			);

		/// Set the environmental dry level to be used for the specified game object
		/// The control value is a number ranging from 0.0f to 1.0f.
		/// 0.0f stands for 0% dry, while 1.0f stands for 100% dry.
		/// \aknote Reducing the dry level does not mean increasing the wet level. \endaknote
		/// \sa 
		/// - \ref soundengine_environments
		/// - \ref soundengine_environments_setting_dry_environment
		/// - \ref soundengine_environments_id_vs_string
		/// \return Always returns AK_Success
		extern AKSOUNDENGINE_API AKRESULT SetGameObjectDryLevelValue( 
			AkGameObjectID		in_gameObjectID,		///< Associated game object ID
			AkReal32			in_fControlValue		///< Dry level control value, ranging from 0.0f to 1.0f
														///< (0.0f stands for 0% dry, while 1.0f stands for 100% dry)
			);

		/// Set the volume for the specified environment.
		/// The volume is a number ranging from 0.0f to 1.0f.
		/// 0.0f stands for 0% of the environment volume, while 1.0f stands for 100% of the environment volume.
		/// \sa 
		/// - \ref soundengine_environments
		/// - \ref soundengine_environments_setting_environment_volume
		/// - AK::SoundEngine::GetIDFromString()
		/// \return Always returns AK_Success
		extern AKSOUNDENGINE_API AKRESULT SetEnvironmentVolume( 
			AkEnvID				in_FXParameterSetID,	///< Environment ID
			AkReal32			in_fVolume				///< Volume control value, ranging from 0.0f to 1.0f.
														///< (0.0f stands for 0% of the environment volume, 1.0f stands for 100% of the environment volume)
			);

		/// Bypass an environment.
		/// The specified environment will not be computed when bypassed.
		/// This method is useful to disable environments that should not be heard, and therefore save resources.
		/// \sa 
		/// - \ref soundengine_environments
		/// - \ref soundengine_environments_bypassing_environments
		/// - \ref soundengine_environments_id_vs_string
		/// - AK::SoundEngine::GetIDFromString()
		/// \return Always returns AK_Success
		extern AKSOUNDENGINE_API AKRESULT BypassEnvironment(
			AkEnvID	in_FXParameterSetID,				///< Environment ID
			bool	in_bIsBypassed						///< True: bypass the specified environment
			);

		/// Set a game object's obstruction and occlusion levels.
		/// This method is used to affect how an object should be heard by a specific listener.
		/// \sa 
		/// - \ref soundengine_obsocc
		/// - \ref soundengine_environments
		/// \return Always returns AK_Success
		extern AKSOUNDENGINE_API AKRESULT SetObjectObstructionAndOcclusion(  
			AkGameObjectID in_ObjectID,			///< Associated game object ID
			AkUInt32 in_uListener,				///< Listener index (0: first listener, 7: 8th listener)
			AkReal32 in_fObstructionLevel,		///< ObstructionLevel: [0.0f..1.0f]
			AkReal32 in_fOcclusionLevel			///< OcclusionLevel: [0.0f..1.0f]
			);

		//@}
        
        ////////////////////////////////////////////////////////////////////////
		/// @name Audio output capture
		//@{
         

		/// Start recording the sound engine audio output. 
		/// This function is not thread-safe. 
		/// \return AK_Success if successful, AK_Fail if there was a problem starting the output capture.
		///			In optimized mode, this function returns AK_NotCompatible.
		/// \remark This function is provided as a utility tool only. It does nothing if it is 
		///			called in the optimized/release configuration and return AK_NotCompatible.
		/// \sa 
		/// - AK::SoundEngine::StopOutputCapture()
		extern AKSOUNDENGINE_API AKRESULT StartOutputCapture( 
			AkLpCtstr in_CaptureFileName				///< Name of the output capture file
			);

		/// Stop recording the sound engine audio output. 
		/// This function is not thread-safe. 
		/// \return AK_Success if successful, AK_Fail if there was a problem stopping the output capture.
		///			In optimized mode, this function returns AK_NotCompatible.
		/// \remark This function is provided as a utility tool only. It does nothing if it is 
		///			called in the optimized/release configuration and return AK_NotCompatible.
		/// \sa 
		/// - AK::SoundEngine::StartOutputCapture()
		extern AKSOUNDENGINE_API AKRESULT StopOutputCapture();
			
		//@}
	}
}
#endif // _AK_SOUNDENGINE_H_
