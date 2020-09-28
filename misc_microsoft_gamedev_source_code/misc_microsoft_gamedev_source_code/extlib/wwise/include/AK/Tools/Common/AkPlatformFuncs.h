//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkPlatformFuncs.h

/// \file 
/// Platform-dependent functions definition.

#ifndef _AK_TOOLS_COMMON_AKPLATFORMFUNCS_H
#define _AK_TOOLS_COMMON_AKPLATFORMFUNCS_H

#include <AK/SoundEngine/Common/AkTypes.h>

#ifdef WIN32
#include <AK/Tools/Win32/AkPlatformFuncs.h>
#endif

#ifdef XBOX360
#include <AK/Tools/XBox360/AkPlatformFuncs.h>
#endif

#ifdef AK_PS3
#include <AK/Tools/PS3/AkPlatformFuncs.h>
#endif

#ifdef RVL_OS
#include <AK/Tools/Wii/AkPlatformFuncs.h>
#endif

#endif // _AK_TOOLS_COMMON_AKPLATFORMFUNCS_H
