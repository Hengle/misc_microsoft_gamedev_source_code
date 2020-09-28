// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_file_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static void
DeleteFileProxy(char const *FileName)
{
    DeleteFile(FileName);
}

inline DWORD
GetFileSizeProxy(HANDLE Win32FileHandle, DWORD *HighSize)
{
    return(GetFileSize(Win32FileHandle, HighSize));
}

#undef DeleteFile
#undef GetFileSize

static bool
Win32FindFile(char const * const FileName, WIN32_FIND_DATA &Data)
{
    HANDLE Handle = FindFirstFile(FileName, &Data);
    if(Handle != INVALID_HANDLE_VALUE)
    {
        FindClose(Handle);
        return(true);
    }

    return(false);
}

char const *GRANNY
GetTemporaryDirectory(void)
{
    static char TempDirectoryBuffer[MAX_PATH] = { 0 };

    if (TempDirectoryBuffer[0] == '\0')
    {
        GetTempPath(sizeof(TempDirectoryBuffer) - 1, TempDirectoryBuffer);
        {for(char *Replace = TempDirectoryBuffer;
             *Replace;
             ++Replace)
        {
            if(*Replace == '\\')
            {
                *Replace = '/';
            }
        }}
    }

    return (TempDirectoryBuffer);
}

void GRANNY
DeleteFile(char const *FileName)
{
    DeleteFileProxy(FileName);
}

int32x GRANNY
GetFileSize(char const *FileName)
{
    int32x FileSize = 0;

    WIN32_FIND_DATA Data;
    if(Win32FindFile(FileName, Data))
    {
        DWORD HighSize = Data.nFileSizeHigh;
        DWORD LowSize = Data.nFileSizeLow;
        if(HighSize == 0)
        {
            FileSize = LowSize;
        }
        else
        {
            Log1(ErrorLogMessage, FileReadingLogMessage,
                 "File %s is too long (> 4 gigs)", FileName);
        }
    }
    else
    {
        Log1(ErrorLogMessage, FileReadingLogMessage,
             "Unable to get file size for %s", FileName);
    }

    return(FileSize);
}
