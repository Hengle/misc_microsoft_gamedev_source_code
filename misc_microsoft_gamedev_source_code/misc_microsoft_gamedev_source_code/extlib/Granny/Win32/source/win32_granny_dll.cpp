// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_dll.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_DLL_H)
#include "win32_granny_dll.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

bool GRANNY
DLLIsNotInWindowsPath(HANDLE DLL)
{
    bool Result = true;

    char DLLPath[MaximumSystemFileNameSize];
    GetModuleFileName((HINSTANCE)DLL, DLLPath, SizeOf(DLLPath));

    char *DLLFileName = DLLPath;
    {for(char *Scan = DLLFileName;
         *Scan;
         ++Scan)
    {
        if((*Scan == '\\') || (*Scan == '/'))
        {
            DLLFileName = Scan + 1;
        }
    }}

    char WindowsDirectory[MaximumSystemFileNameSize];
    GetWindowsDirectory(WindowsDirectory, SizeOf(WindowsDirectory));

    char SystemDirectory[MaximumSystemFileNameSize];
    GetSystemDirectory(SystemDirectory, SizeOf(SystemDirectory));

    if(StringBeginsWithLowercase(DLLPath, WindowsDirectory) ||
       StringBeginsWithLowercase(DLLPath, SystemDirectory))
    {
        static char ErrorMessage[MaximumMessageBufferSize];
        ConvertToStringVar(sizeof(ErrorMessage), ErrorMessage,
                           "%s has been incorrectly installed as "
                           "%s.  To avoid versioning problems, "
                           "%s must always be installed in an "
                           "application-specific directory, and never "
                           "in a system directory.",
                           DLLFileName, DLLPath, DLLFileName);

        MessageBox(0, ErrorMessage, "Fatal Granny Installation Error",
                   MB_OK | MB_ICONERROR);

        Result = false;
    }

    return(Result);
}
