// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_xtl.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
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
        Log4(IsError ? ErrorLogMessage : WarningLogMessage,
             Win32SubsystemLogMessage,
             "%s(%d) : "
             "%s failed with error %d",
             SourceFile, SourceLineNumber,
             FailedWin32Function, LastError);
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
