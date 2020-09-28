// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_file_reader.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define SubsystemCode ANSISubsystemLogMessage

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif


USING_GRANNY_NAMESPACE;

struct ansi_file_reader
{
    file_reader Base;
    FILE *ANSIFileHandle;
};

static void
ANSICloseFileReader(char const *SourceFileName, int32x SourceLineNumber,
                    file_reader *ReaderInit)
{
    ansi_file_reader *Reader = (ansi_file_reader *)ReaderInit;

    if(Reader)
    {
        if(fclose(Reader->ANSIFileHandle))
        {
            ANSILogErrorAsWarning(fclose);
        }

        Deallocate(Reader);
    }
}

static int32x
ANSIReadAtMost(char const *SourceFileName, int32x SourceLineNumber,
               file_reader &ReaderInit, int32x FilePosition,
               int32x UInt8Count, void *Buffer)
{
    intaddrx UInt8sRead = 0;

    ansi_file_reader &Reader = (ansi_file_reader &)ReaderInit;

    if(ANSISeek(Reader.ANSIFileHandle, FilePosition, SEEK_SET) ==
       FilePosition)
    {
        UInt8sRead = fread(Buffer, 1, UInt8Count, Reader.ANSIFileHandle);
        if(UInt8sRead < UInt8Count)
        {
            if(ferror(Reader.ANSIFileHandle))
            {
                ANSILogErrorAsWarning(fread);
            }
        }
    }

    int32x RetVal;
    CheckConvertToInt32(RetVal, UInt8sRead, return -1); // serious wierdness if this fires...
    return RetVal;
}

file_reader *GRANNY
CreatePlatformFileReader(char const *SourceFileName, int32x SourceLineNumber,
                         char const *FileNameToOpen)
{
    ansi_file_reader *Reader = 0;

    FILE *ANSIFileHandle = fopen(FileNameToOpen, "rb");
    if(ANSIFileHandle)
    {
        Reader = Allocate(ansi_file_reader);
        if(Reader)
        {
            InitializeFileReader(SourceFileName, SourceLineNumber,
                                 ANSICloseFileReader,
                                 ANSIReadAtMost,
                                 Reader->Base);
            Reader->ANSIFileHandle = ANSIFileHandle;
        }
        else
        {
            fclose(ANSIFileHandle);
        }
    }
    else
    {
        ANSILogErrorAsWarning(fopen);
    }

    return((file_reader *)Reader);
}

open_file_reader_callback *GRANNY OpenFileReaderCallback = CreatePlatformFileReader;
