// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_file_reader.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
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

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct win32_file_reader
{
    file_reader Base;
    HANDLE Win32FileHandle;
};

END_GRANNY_NAMESPACE;

static void
Win32CloseFileReader(char const *SourceFileName, int32x SourceLineNumber,
                     file_reader *ReaderInit)
{
    COUNT_BLOCK("Win32CloseFileReader");

    win32_file_reader *Reader = (win32_file_reader *)ReaderInit;

    if(Reader)
    {
        if(!CloseHandle(Reader->Win32FileHandle))
        {
            Win32LogErrorAsWarning(CloseHandle);
        }

        Deallocate(Reader);
    }
}

static int32x
Win32ReadAtMost(char const *SourceFileName, int32x SourceLineNumber,
                file_reader &ReaderInit, int32x FilePosition,
                int32x UInt8Count, void *Buffer)
{
    COUNT_BLOCK("Win32ReadAtMost");

    DWORD UInt8sRead = 0;

    win32_file_reader &Reader = (win32_file_reader &)ReaderInit;

    if(Win32Seek(Reader.Win32FileHandle, FilePosition, FILE_BEGIN)
       == FilePosition)
    {
        if(!ReadFile(Reader.Win32FileHandle, Buffer,
                     UInt8Count, &UInt8sRead, 0))
        {
            Win32LogErrorAsWarning(ReadFile);
        }
    }

    return(UInt8sRead);
}

file_reader *GRANNY
CreatePlatformFileReader(char const *SourceFileName, int32x SourceLineNumber,
                         char const *FileNameToOpen)
{
    COUNT_BLOCK("CreatePlatformFileReader");

    win32_file_reader *Reader = 0;

    HANDLE Win32FileHandle = CreateFile(
        FileNameToOpen, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 0);
    if(Win32FileHandle != INVALID_HANDLE_VALUE)
    {
        Reader = Allocate(win32_file_reader);
        if(Reader)
        {
            InitializeFileReader(SourceFileName, SourceLineNumber,
                                 Win32CloseFileReader,
                                 Win32ReadAtMost,
                                 Reader->Base);
            Reader->Win32FileHandle = Win32FileHandle;
        }
        else
        {
            CloseHandle(Win32FileHandle);
        }
    }
    else
    {
        // TODO: We decided to stop logging this, since files
        // often cannot be opened and it is not considered an
        // error condition.
//        Win32LogErrorAsWarning(CreateFile);
    }

    return((file_reader *)Reader);
}

open_file_reader_callback *GRANNY OpenFileReaderCallback = CreatePlatformFileReader;
