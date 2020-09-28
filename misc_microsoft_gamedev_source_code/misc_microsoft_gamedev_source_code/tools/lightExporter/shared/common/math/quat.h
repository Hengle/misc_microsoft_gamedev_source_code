//-----------------------------------------------------------------------------
// File: quat.h
// Platform independent include file for the Quat class.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef QUAT_H

#include "common/core/core.h"

#if PLATFORM_X86
	#include "x86/math/quat_x86.h"
#else
	#error INVALID TARGET
#endif

#endif // QUAT_H
