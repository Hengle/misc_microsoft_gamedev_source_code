// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_windows.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

// The build system sometimes tries to build this even on non-Win32 platforms.
#if PLATFORM_WINXX


#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
Win32LogLastError_(bool IsError,
                   char const *SourceFile, int32x SourceLineNumber,
                   char const *FailedWin32Function)
{
    DWORD LastError = GetLastError();
    if(LastError != NO_ERROR)
    {
        LPVOID MessageBuffer;
        if(FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            LastError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR)&MessageBuffer,
            0,
            0) == 0)
        {
            // Win32 couldn't supply us with an error code
            MessageBuffer = "unknown win32 error";
        }

        Log4(IsError ? ErrorLogMessage : WarningLogMessage,
             Win32SubsystemLogMessage,
             "%s failed with error \"%s\" at %s(%d)",
             FailedWin32Function, (char *)MessageBuffer,
             SourceFile, SourceLineNumber);

        LocalFree(MessageBuffer);
    }
}

int32x GRANNY
Win32Seek(HANDLE Win32FileHandle, int32x Offset, DWORD MoveMethod)
{
    int32x Result = SetFilePointer(Win32FileHandle, Offset, 0, MoveMethod);
    if((Result == INVALID_SET_FILE_POINTER) &&
       (GetLastError() != 0))
    {
        Win32LogErrorAsWarning(SetFilePointer);
    }

    return(Result);
}


#endif //#if !PLATFORM_WINXX
