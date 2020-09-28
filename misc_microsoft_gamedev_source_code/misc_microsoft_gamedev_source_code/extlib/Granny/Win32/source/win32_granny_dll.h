#if !defined(WIN32_GRANNY_DLL_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_dll.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

bool DLLIsNotInWindowsPath(HANDLE DLL);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define WIN32_GRANNY_DLL_H
#endif
