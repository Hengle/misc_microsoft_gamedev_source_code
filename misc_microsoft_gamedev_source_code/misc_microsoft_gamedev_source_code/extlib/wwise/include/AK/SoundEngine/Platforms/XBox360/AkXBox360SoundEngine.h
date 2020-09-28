//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkXBox360SoundEngine.h

/// \file 
/// Main Sound Engine interface, specific XBox360

#ifndef _AK_XBOX360_SOUND_ENGINE_H_
#define _AK_XBOX360_SOUND_ENGINE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

/// Thread Properties
struct AkThreadProperties
{
    int                 nPriority;	///< Thread Priority
    DWORD               dwProcessor;///< Processor ID 
	AkUInt32			uStackSize;	///< Thread stack size.
};

/// Platform specific initialization settings
/// \sa AK::SoundEngine::Init
/// \sa AK::SoundEngine::GetDefaultPlatformInitSettings
struct AkPlatformInitSettings
{
    // Threading model.
    AkThreadProperties  threadLEngine;			///< Lower engine threading properties	
	AkThreadProperties  threadBankManager;		///< Bank manager threading properties (its default priority is AK_THREAD_PRIORITY_NORMAL)

    // Memory.
    AkUInt32            uLEngineDefaultPoolSize;///< Lower Engine default memory pool size

	// Voices.
	AkUInt16            uNumRefillsInVoice;		///< Number of refill buffers in voice buffer. 2 == double-buffered, defaults to 4.

#ifndef AK_OPTIMIZED
	AkThreadProperties  threadMonitor;			///< Monitor threading properties (its default priority is AK_THREAD_PRIORITY_ABOVENORMAL)
#endif
};


#endif //_AK_XBOX360_SOUND_ENGINE_H_
