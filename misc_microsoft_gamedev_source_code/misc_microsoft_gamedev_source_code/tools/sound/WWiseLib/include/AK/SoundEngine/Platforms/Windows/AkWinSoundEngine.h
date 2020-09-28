//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkWinSoundEngine.h

/// \file 
/// Main Sound Engine interface, specific WIN32.

#ifndef _AK_WIN_SOUND_ENGINE_H_
#define _AK_WIN_SOUND_ENGINE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

/// Sound quality option
/// (available in the PC version only)
enum AkSoundQuality
{
	AkSoundQuality_High,	///< High quality (48 kHz sampling rate, default value)
	AkSoundQuality_Low,		///< Reduced quality (24 kHz sampling rate)
};

/// Thread Properties
struct AkThreadProperties
{
    int                 nPriority;		///< Thread priority
    DWORD               dwAffinityMask;	///< Affinity mask
	AkUInt32			uStackSize;		///< Thread stack size.
};

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
struct AkPlatformInitSettings
{
    // Direct sound.
    HWND			    hWnd;					///< Handle to the foreground window

    // Threading model.
    AkThreadProperties  threadLEngine;			///< Lower engine threading properties
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)

    // Memory.
    AkUInt32            uLEngineDefaultPoolSize;///< Lower Engine default memory pool size

	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.
	AkSoundQuality		eAudioQuality;			///< Quality of audio processing, default = AkSoundQuality_High.

	bool				bGlobalFocus;			///< Corresponding to DSBCAPS_GLOBALFOCUS. Sounds will be muted if set to false when the game loses the focus.
};

struct IDirectSound8;

namespace AK
{
	/// Get instance of DirectSound created by the sound engine at initialization.
	/// \return Non-addref'd pointer to DirectSound interface. NULL if sound engine is not initialized.
	extern IDirectSound8 * GetDirectSoundInstance();
};

#endif //_AK_WIN_SOUND_ENGINE_H_
