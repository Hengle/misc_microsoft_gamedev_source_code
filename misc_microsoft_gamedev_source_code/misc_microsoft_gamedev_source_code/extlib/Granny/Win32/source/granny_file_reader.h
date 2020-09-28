#if !defined(GRANNY_FILE_READER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_reader.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileReaderGroup);

struct file_reader;

EXPAPI typedef file_reader *open_file_reader_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    char const *FileName);
EXPAPI typedef void close_file_reader_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_reader *Reader);
EXPAPI typedef int32x read_at_most_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_reader &Reader,
    int32x FilePosition,
    int32x UInt8Count,
    void *Buffer);

EXPTYPE_EPHEMERAL struct file_reader
{
    char const *SourceFileName;
    int32x SourceLineNumber;

    close_file_reader_callback *CloseFileReaderCallback;
    read_at_most_callback *ReadAtMostCallback;
};

EXPAPI GS_SAFE void InitializeFileReader(char const *SourceFileName, int32x SourceLineNumber,
                                         close_file_reader_callback *CloseFileReaderCallback,
                                         read_at_most_callback *ReadAtMostCallback,
                                         file_reader &Reader);

EXPAPI GS_PARAM file_reader *CreatePlatformFileReader(char const *SourceFileName, int32x SourceLineNumber,
                                                      char const *FileNameToOpen);

#define OpenFileReader(FileName) (*OpenFileReaderCallback)(__FILE__, __LINE__, FileName)
#define CloseFileReader(Reader) if(Reader) {(*(Reader)->CloseFileReaderCallback)(__FILE__, __LINE__, Reader);} EXPMACRO
#define ReadAtMost(Reader, Pos, Count, Buffer) (*(Reader).ReadAtMostCallback)(__FILE__, __LINE__, Reader, Pos, Count, Buffer) EXPMACRO
#define ReadExactly(Reader, Pos, Count, Buffer) ((*(Reader).ReadAtMostCallback)(__FILE__, __LINE__, Reader, Pos, Count, Buffer) == Count) EXPMACRO

// TODO: Access to some sort of list of open file handles?

extern open_file_reader_callback *OpenFileReaderCallback;

EXPAPI GS_READ open_file_reader_callback *GetDefaultFileReaderOpenCallback(void);
EXPAPI GS_MODIFY void SetDefaultFileReaderOpenCallback(open_file_reader_callback *OpenFileReaderCallback);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_READER_H
#endif
