// ========================================================================
// $File: //jeffr/granny/rt/granny_file_reader.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
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
InitializeFileReader(char const *SourceFileName, int32x SourceLineNumber,
                     close_file_reader_callback *CloseFileReaderCallback,
                     read_at_most_callback *ReadAtMostCallback,
                     file_reader &Reader)
{
    Reader.SourceFileName = SourceFileName;
    Reader.SourceLineNumber = SourceLineNumber;
    Reader.CloseFileReaderCallback = CloseFileReaderCallback;
    Reader.ReadAtMostCallback = ReadAtMostCallback;
}

open_file_reader_callback *GRANNY
GetDefaultFileReaderOpenCallback(void)
{
    return(OpenFileReaderCallback);
}

void GRANNY
SetDefaultFileReaderOpenCallback(
    open_file_reader_callback *OpenFileReaderCallbackInit)
{
    OpenFileReaderCallback = OpenFileReaderCallbackInit;
}
