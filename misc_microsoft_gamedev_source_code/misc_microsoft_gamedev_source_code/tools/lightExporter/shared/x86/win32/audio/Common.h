// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

#include "common/core/core.h"

// Inlines and macros.

#define DIRECTSOUND_VERSION	0x0900

// Includes.

#include <windows.h>
#include <mmreg.h>
#include <dsound.h>
#include <mmsystem.h>

// Libraries.

#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"winmm.lib")

// Inlines and macros.

#include "Common.inl"

#endif /* _AUDIO_COMMON_H_ */
