//--------------------------------------------------------------------------------------
// stdafx.h
//
// This is a good place to include commonly used header files that rarely change, in
// order to speed up build times.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#ifndef ATGFRAMEWORK_STDAFX_H

#ifdef _XBOX

#include <xtl.h>
#include <ppcintrinsics.h>

#endif // _XBOX

#ifdef _PC

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// Using the win32\ path prefix on the D3D include files makes sure that the Xbox 360 
// version of D3D is used, not the DirectX SDK version.
#include <win32\vs2005\d3d9.h>
#include <win32\vs2005\d3dx9.h>
#pragma warning(disable:4100)

#include "XTLOnPC.h"

#endif // _PC

#include <xgraphics.h>
#include <xboxmath.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// C4127: conditional expression is constant
#pragma warning(disable:4127)

#endif
