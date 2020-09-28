//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkLock.h

/// \file 
/// Platform independent synchronization services for plug-ins.

#ifndef _AK_TOOLS_COMMON_AKLOCK_H
#define _AK_TOOLS_COMMON_AKLOCK_H

#ifdef WIN32
#include <AK/Tools/Win32/AkLock.h>
#endif

#ifdef XBOX360
#include <AK/Tools/XBox360/AkLock.h>
#endif

#ifdef AK_PS3
#include <AK/Tools/PS3/AkLock.h>
#endif

#ifdef RVL_OS
#include <AK/Tools/Wii/AkLock.h>
#endif

#endif // _AK_TOOLS_COMMON_AKLOCK_H
