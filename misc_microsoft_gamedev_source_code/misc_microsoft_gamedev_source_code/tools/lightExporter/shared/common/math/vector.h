//-----------------------------------------------------------------------------
// File: vector.h
// Platform independent include file for the Vec1-Vec4 classes.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef VECTOR_H

#include "common/core/core.h"

#if PLATFORM_X86
	#include "x86/math/vector_x86.h"
#else
	#error INVALID TARGET
#endif

#endif // VECTOR_H
