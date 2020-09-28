//-----------------------------------------------------------------------------
// File: platform_win32.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#ifdef _DEBUG
  #define DEBUG 1
#endif
#define DEBUG_DEFINED 1

#define FORCE_INLINE __forceinline
#define FORCE_INLINE_DEFINED 1

#pragma warning (disable:4018)
// Float conversion
#pragma warning (disable:4244)		
#pragma warning (disable:4305)		
// Browse info ID truncation
#pragma warning (disable:4786)    

#if DEBUG
	// Conditional statement is constant
	#pragma warning (disable:4127)
#endif

#pragma inline_recursion(on)
#pragma inline_depth(255)

#undef min
#undef max

#endif // PLATFORM_WIN32_H

