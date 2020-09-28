//-----------------------------------------------------------------------------
// File: platform.h
// Defines platform specific definitions.
// Sets symbols: TARGET, DEBUG, etc.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef PLATFORM_H
#define PLATFORM_H

#define PLATFORM_X86 1

#if PLATFORM_X86
	#include "x86/core/platform_x86.h"
#else
  #error Invalid platform!
#endif

#endif // PLATFORM_H
