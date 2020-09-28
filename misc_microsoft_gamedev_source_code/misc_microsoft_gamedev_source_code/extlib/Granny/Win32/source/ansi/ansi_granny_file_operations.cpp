// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_file_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif


void GRANNY
DeleteFile(char const *FileName)
{
    remove(FileName);
}

int32x GRANNY
GetFileSize(char const *FileName)
{
    int32x FileSize = 0;

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        if(ANSISeek(File, 0, SEEK_END))
        {
            FileSize = ftell(File);
        }

        fclose(File);
    }
    else
    {
        Log1(ErrorLogMessage, FileReadingLogMessage,
             "Unable to get file size for %s.", FileName);
    }

    return(FileSize);
}

char const *GRANNY
GetTemporaryDirectory(void)
{
    // TODO: Would it perhaps be more proper to query tempnam() or something here and then
    // just rip out the path portion?
    return("/tmp/");
}

int32x GRANNY
ANSISeek(FILE *ANSIFileHandle, int32x Offset, int32x MoveMethod)
{
    if(fseek(ANSIFileHandle, (long)Offset, MoveMethod))
    {
        ANSILogErrorAsWarning(fseek);
    }

    return(ftell(ANSIFileHandle));
}



