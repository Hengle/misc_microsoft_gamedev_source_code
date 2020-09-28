//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Audiokinetic's definitions and factory of overridable Stream Manager module.

#ifndef __AK_SOUNDENGINE_COMMON_AKMODULE_H__
#define __AK_SOUNDENGINE_COMMON_AKMODULE_H__

#ifdef WIN32
#include <AK/SoundEngine/Platforms/Windows/AkModule.h>
#endif

#ifdef XBOX360
#include <AK/SoundEngine/Platforms/XBox360/AkModule.h>
#endif

#ifdef AK_PS3
#include <AK/SoundEngine/Platforms/PS3/AkModule.h>
#endif

#ifdef RVL_OS
#include <AK/SoundEngine/Platforms/Wii/AkModule.h>
#endif

#endif // __AK_SOUNDENGINE_COMMON_AKMODULE_H__
