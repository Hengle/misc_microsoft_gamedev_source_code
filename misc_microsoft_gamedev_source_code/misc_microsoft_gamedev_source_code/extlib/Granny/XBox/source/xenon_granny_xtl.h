#if !defined(XENON_GRANNY_XTL_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_xtl.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#ifndef STRICT
#define STRICT
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef WIN32_EXTRA_LEAN
#define WIN32_EXTRA_LEAN
#endif

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

// NOTE: We switch on Xenon here for the convenience of people who build
// some portion of the Xenon Granny on Windows alone (for tools, etc.)
#if PLATFORM_XENON
#include <xtl.h>
#else
#include <windows.h>
#endif

BEGIN_GRANNY_NAMESPACE;

// Win32LogErrorAsWarning and Win32LogErrorAsError will check the last
// error code.  If it is set, it will log the reported Win32 string as
// a warning or an error (respectively) in the error log.
#define Win32LogErrorAsWarning(FailedWin32Function) \
        Win32LogLastError_(false, __FILE__, __LINE__, #FailedWin32Function);
#define Win32LogErrorAsError(FailedWin32Function) \
        Win32LogLastError_(true, __FILE__, __LINE__, #FailedWin32Function);

// This is a more convenient form of the standard Win32 SetFilePointer call
int32x Win32Seek(HANDLE Win32FileHandle, int32x Offset, DWORD MoveMethod);

// The following functions should not be called directly.  You
// should use the macros defined above.
void Win32LogLastError_(bool IsError,
                        char const *SourceFile, int32x SourceLineNumber,
                        char const *FailedWin32Function);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define XENON_GRANNY_XTL_H
#endif
