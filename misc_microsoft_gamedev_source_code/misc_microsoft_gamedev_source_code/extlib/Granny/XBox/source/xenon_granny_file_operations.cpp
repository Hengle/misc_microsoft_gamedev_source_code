// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_file_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
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
GetFileSizeProxy(HANDLE XenonFileHandle, DWORD *HighSize)
{
    return(GetFileSize(XenonFileHandle, HighSize));
}

#undef DeleteFile
#undef GetFileSize

static bool
XenonFindFile(char const * const FileName, WIN32_FIND_DATA &Data)
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
    return("./");
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
    if(XenonFindFile(FileName, Data))
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
