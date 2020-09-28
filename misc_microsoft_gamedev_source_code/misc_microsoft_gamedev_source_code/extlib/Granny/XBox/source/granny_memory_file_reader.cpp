// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_file_reader.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMORY_FILE_READER_H)
#include "granny_memory_file_reader.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct memory_file_reader
{
    file_reader Base;
    int32x Offset;
    int32x MemorySize;
    uint8 *Memory;
};

END_GRANNY_NAMESPACE;

static void
MemoryCloseFileReader(char const *SourceFileName,
                      int32x SourceLineNumber,
                      file_reader *Reader)
{
    Deallocate(Reader);
}

static int32x
MemoryReadAtMost(char const *SourceFileName,
                 int32x SourceLineNumber,
                 file_reader &ReaderInit,
                 int32x FilePosition,
                 int32x UInt8Count,
                 void *Buffer)
{
    memory_file_reader &Reader = (memory_file_reader &)ReaderInit;

    int32x BytesRead = 0;
    if(FilePosition >= Reader.Offset)
    {
        FilePosition -= Reader.Offset;
        if(FilePosition < Reader.MemorySize)
        {
            BytesRead = UInt8Count;
            if((FilePosition + BytesRead) > Reader.MemorySize)
            {
                BytesRead = Reader.MemorySize - FilePosition;
            }

            Copy(BytesRead, &Reader.Memory[FilePosition], Buffer);
        }
    }

    return(BytesRead);
}

file_reader *GRANNY
CreateMemoryFileReader(char const *SourceFileName, int32x SourceLineNumber,
                       int32x Offset, int32x MemorySize, void *Memory)
{
    memory_file_reader *Reader = Allocate(memory_file_reader);
    if(Reader)
    {
        InitializeFileReader(SourceFileName, SourceLineNumber,
                             MemoryCloseFileReader, MemoryReadAtMost,
                             Reader->Base);
        Reader->Offset = Offset;
        Reader->MemorySize = MemorySize;
        Reader->Memory = (uint8 *)Memory;
    }

    return((file_reader *)Reader);
}
