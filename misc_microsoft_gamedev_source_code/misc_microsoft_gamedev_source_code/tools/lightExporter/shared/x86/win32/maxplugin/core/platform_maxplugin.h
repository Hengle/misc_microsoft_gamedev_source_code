// platform_maxplugin.h
#pragma once
#ifndef PLATFORM_MAXPLUGIN_H
#define PLATFORM_MAXPLUGIN_H

#ifndef WIN32
	#error undefined WIN32 preprocessor symbol
#endif

#ifndef WINVER
	#define WINVER 0x0400
#endif

#ifndef _WIN32_WINNT 
	#define _WIN32_WINNT 0x0400
#endif

#ifndef NO_WIN32_LEAN_AND_MEAN
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
#endif

#ifndef _WIN32_WINDOWS
	#define _WIN32_WINDOWS 0x0410
#endif

#include "max.h"

#if MAX5
	#define MAX_new VC6Mem_new
	#define MAX_delete VC6Mem_delete
#endif

#include "MAX_mem.h"

#if MAX5
	#include "vc6mem.h"
#endif

#undef min
#undef max

#endif // PLATFORM_MAXPLUGIN_H