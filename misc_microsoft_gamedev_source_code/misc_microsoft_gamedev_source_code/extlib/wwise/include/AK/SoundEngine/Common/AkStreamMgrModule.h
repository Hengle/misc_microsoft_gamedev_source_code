//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Audiokinetic's definitions and factory of overridable Stream Manager module.

#ifndef __AK_SOUNDENGINE_COMMON_AKSTREAMMGRMODULE_H__
#define __AK_SOUNDENGINE_COMMON_AKSTREAMMGRMODULE_H__

#ifdef WIN32
#include <AK/SoundEngine/Platforms/Windows/AkStreamMgrModule.h>
#endif

#ifdef XBOX360
#include <AK/SoundEngine/Platforms/XBox360/AkStreamMgrModule.h>
#endif

#ifdef AK_PS3
#include <AK/SoundEngine/Platforms/PS3/AkStreamMgrModule.h>
#endif

#ifdef RVL_OS
#include <AK/SoundEngine/Platforms/Wii/AkStreamMgrModule.h>
#endif

#endif // __AK_SOUNDENGINE_COMMON_AKSTREAMMGRMODULE_H__
