//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkMusicEngine.h

/// \file 
/// The main music engine interface.


#ifndef _AK_MUSICENGINE_H_
#define _AK_MUSICENGINE_H_

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>

/// Platform-independent initialization settings of the music engine
/// \sa 
/// - AK::MusicEngine::Init()
/// - \ref soundengine_integration_init_advanced
struct AkMusicSettings
{
	AkReal32 fStreamingLookAheadRatio;	///< Multiplication factor for all streaming look-ahead heuristic values.
};

// Audiokinetic namespace
namespace AK
{
	/// Music engine namespace
	/// \warning The functions in this namespace are not thread-safe, unless stated otherwise.
	namespace MusicEngine
	{
        ///////////////////////////////////////////////////////////////////////
		/// @name Initialization
		//@{

		/// Initializes the music engine.
		/// \warning This function must be called after the base sound engine has been properly initialized.
		/// \return AK_Success if the Init was successful, AK_Fail otherwise.
		/// \sa
		/// - \ref workingwithsdks_initialization
        extern AKSOUNDENGINE_API AKRESULT Init(
			AkMusicSettings *	in_pSettings	///< Initialization settings (can be NULL, to use the default values)
			);

		/// Gets the music engine's default initialization settings values
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::MusicEngine::Init()
		extern AKSOUNDENGINE_API void GetDefaultInitSettings(
            AkMusicSettings &	out_settings	///< Returned default platform-independent music engine settings
		    );

		/// Terminates the music engine.
		/// \warning This function must be called before calling Term() on the base sound engine.
		/// \sa
		/// - \ref workingwithsdks_termination
		extern AKSOUNDENGINE_API void Term(
			);
		
        //@}
    }

	/// Music proxy namespace
	/// \warning The functions in this namespace are not thread-safe, unless stated otherwise.
	namespace ProxyMusic
	{
		/// Initialize the music engine proxy.
		/// The music engine proxy is required for Wwise to connect to a game that uses interactive music.
		/// \warning This function must be called after the music engine has been properly initialized.
		/// \sa
		/// - \ref workingwithsdks_initialization
		/// - AK::MusicEngine::Init()
		extern AKSOUNDENGINE_API void Init(
			);
	}
}

#endif // _AK_MUSICENGINE_H_

