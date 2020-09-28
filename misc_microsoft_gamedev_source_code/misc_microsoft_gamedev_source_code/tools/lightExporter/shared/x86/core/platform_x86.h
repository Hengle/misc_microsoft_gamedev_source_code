//-----------------------------------------------------------------------------
// File: platform_x86.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef PLATFORM_X86_H
#define PLATFORM_X86_H

#include "x86/win32/core/platform_win32.h"

#if PLATFORM_WIN32

#elif PLATFORM_MAXPLUGIN
	#include "x86/win32/maxplugin/core/platform_maxplugin.h"
#elif PLATFORM_DX9

#elif PLATFORM_CONSOLE

#else
	#error Platform undefined!
#endif

#include "assert_x86.h"

#endif // PLATFORM_X86_H

