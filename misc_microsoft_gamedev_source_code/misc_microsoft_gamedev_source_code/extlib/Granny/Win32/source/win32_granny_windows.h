#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_windows.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#ifndef GRANNY_PLATFORM_H
#include "granny_platform.h"
#endif

#if !PLATFORM_WINXX
#error "This is a Windows file for windows people"
#endif


#ifndef STRICT
#define STRICT
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef WIN32_EXTRA_LEAN
#define WIN32_EXTRA_LEAN
#endif

#include <windows.h>

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
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

// NOTE: These are #defines that Windows _says_ it defines (if you
// read MSDN), but they _don't_ actually define them anywhere.  I assume
// this is because they've added the #define's since MSVC 6.x shipped,
// or something similar (either that or its just totally bogus docs).
#if !defined(INVALID_SET_FILE_POINTER)
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF
#endif

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define WIN32_GRANNY_WINDOWS_H
#endif
