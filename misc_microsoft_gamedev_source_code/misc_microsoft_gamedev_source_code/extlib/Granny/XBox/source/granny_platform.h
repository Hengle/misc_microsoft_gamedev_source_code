#if !defined(GRANNY_PLATFORM_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_platform.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// Clear all windowing system flags
#define PLATFORM_WIN32 0
#define PLATFORM_WIN64 0
#define PLATFORM_WINXX 0
#define PLATFORM_MACOSX 0
#define PLATFORM_OS2 0
#define PLATFORM_X 0
#define PLATFORM_XBOX 0
#define PLATFORM_XENON 0
#define PLATFORM_PS2 0
#define PLATFORM_PSP 0
#define PLATFORM_PS3 0
#define PLATFORM_GAMECUBE 0
#define PLATFORM_WII 0
#define PLATFORM_LINUX 0

#define PLATFORM_PATH_SEPARATOR "/"

// Clear all underlying OS flags
#define PLATFORM_LINUX 0


// See if we're Windows
#ifdef _WIN32
// Cunningly, both _WIN64 and _WIN32 are defined for X64 stuff.
#ifdef _WIN64

#undef PLATFORM_WIN64
#define PLATFORM_WIN64 1
#undef PLATFORM_WINXX
#define PLATFORM_WINXX 1

#else //#ifdef _WIN64

#undef PLATFORM_WIN32
#define PLATFORM_WIN32 1
#undef PLATFORM_WINXX
#define PLATFORM_WINXX 1

#endif //#else //#ifdef _WIN64

#endif // _WIN32


// See if we're Macintosh
// TODO: We should really distinguish between Mac OS X and OS < X here.
#if defined(_MACOSX) || defined(macintosh)
#undef PLATFORM_MACOSX
#define PLATFORM_MACOSX 1
#endif

// See if we're Linux (specifically)
#if defined(_LINUX) || defined(linux) || defined(__linux__)
#undef PLATFORM_LINUX
#define PLATFORM_LINUX 1
#endif

/* #endif */

// See if we're OS2
#ifdef _OS2
#undef PLATFORM_OS2
#define PLATFORM_OS2 1
#endif

// See if we're on Xenon
#ifdef _XENON
// TODO: Why does Microsoft's compiler insist on defining _WIN32 on Xenon??
#undef PLATFORM_WIN32
#define PLATFORM_WIN32 0
#undef PLATFORM_WINXX
#define PLATFORM_WINXX 0
#undef PLATFORM_XENON
#define PLATFORM_XENON 1
#undef PLATFORM_PATH_SEPARATOR
#define PLATFORM_PATH_SEPARATOR "\\"
#endif

// See if we're on XBox
// Annoyingly, Xenon defines _XBOX as well...
#if defined(_XBOX) && !defined(_XENON)
// TODO: Why does Microsoft's compiler insist on defining _WIN32 on XBox??
#undef PLATFORM_WIN32
#define PLATFORM_WIN32 0
#undef PLATFORM_WINXX
#define PLATFORM_WINXX 0
#undef PLATFORM_XBOX
#define PLATFORM_XBOX 1
#undef PLATFORM_PATH_SEPARATOR
#define PLATFORM_PATH_SEPARATOR "\\"
#endif



// See if we're PS2
#ifdef _PSX2
#undef PLATFORM_PS2
#define PLATFORM_PS2 1
#endif


// See if we're PSP
#ifdef __psp__
#undef PLATFORM_PSP
#define PLATFORM_PSP 1
#endif


// See if we're PS3
#ifdef __CELLOS_LV2__
#undef PLATFORM_PS3
#define PLATFORM_PS3 1

    // Double check that pointers are 32-bit
    #ifndef __LP32__
    #error "Granny supports only the 32bit PS3 ABI"
    #endif
#endif


// See if we're GAMECUBE
#ifdef _GAMECUBE
#undef PLATFORM_GAMECUBE
#define PLATFORM_GAMECUBE 1
#endif

#ifdef REVOLUTION
#undef PLATFORM_WII
#define PLATFORM_WII 1
#endif

#if (!PLATFORM_WIN32 && !PLATFORM_WIN64 && !PLATFORM_MACOSX && !PLATFORM_OS2 && \
     !PLATFORM_X && !PLATFORM_XBOX && !PLATFORM_XENON &&     \
     !PLATFORM_PS2 && !PLATFORM_PSP && !PLATFORM_PS3 &&      \
     !PLATFORM_GAMECUBE && !PLATFORM_WII && !PLATFORM_LINUX)
#error Unrecognized platform (see platform.h)
#endif

#if ((PLATFORM_WIN32 + PLATFORM_WIN64 + PLATFORM_MACOSX +          \
      PLATFORM_OS2 + PLATFORM_X + PLATFORM_XBOX + PLATFORM_XENON + \
      PLATFORM_PS2 + PLATFORM_PSP + PLATFORM_PS3 +                 \
      PLATFORM_GAMECUBE + PLATFORM_WII + PLATFORM_LINUX) > 1)
#error Multiple platforms defined! (see platform.h)
#endif

// PLATFORM_WINXX means either WIN32 or WIN64
#if PLATFORM_WIN32 || PLATFORM_WIN64
#if !PLATFORM_WINXX
#error PLATFORM_WINXX not defined correctly (see platform.h)
#endif
#else
#if PLATFORM_WINXX
#error PLATFORM_WINXX not defined correctly (see platform.h)
#endif
#endif

#include "header_postfix.h"
#define GRANNY_PLATFORM_H
#endif
