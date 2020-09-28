// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_file_writer.cpp $
// $DateTime: 2007/01/15 14:31:41 $
// $Change: 14209 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct ansi_file_writer
{
    file_writer Base;
    FILE *ANSIFileHandle;
};
CompileAssert(OffsetFromType(ansi_file_writer, Base) == 0);
CompileAssert(SizeOf(ansi_file_writer) == (SizeOf(file_writer) +
                                           SizeOf(FILE*)));

END_GRANNY_NAMESPACE;

void
AnsiDeleteFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                     file_writer *Writer)
{
    COUNT_BLOCK("AnsiDeleteFileWriter");

    ansi_file_writer *AnsiWriter = (ansi_file_writer*)Writer;
    if(Writer)
    {
        Assert(!AnsiWriter->Base.CRCing);

        if(fclose(AnsiWriter->ANSIFileHandle))
        {
            ANSILogErrorAsWarning(fclose);
        }

        Deallocate(Writer);
    }
}

int32x
AnsiSeekWriter(char const *SourceFileName, int32x SourceLineNumber,
               file_writer &Writer, int32x OffsetInUInt8s,
               file_writer_seek_type SeekType)
{
    COUNT_BLOCK("AnsiSeekWriter");

    ansi_file_writer &AnsiWriter = (ansi_file_writer&)Writer;
    Assert(!AnsiWriter.Base.CRCing || (SeekType == SeekCurrent && OffsetInUInt8s == 0));

    switch (SeekType)
    {
        case SeekStart:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_SET));
        case SeekEnd:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_END));
        case SeekCurrent:
            return(ANSISeek(AnsiWriter.ANSIFileHandle, OffsetInUInt8s, SEEK_CUR));

        default:
            InvalidCodePath("Invalid seek type");
            return 0;
    }
}

bool
AnsiWrite(char const *SourceFileName, int32x SourceLineNumber,
          file_writer &Writer, int32x UInt8Count, void const *WritePointer)
{
    COUNT_BLOCK("AnsiWrite");

    ansi_file_writer &AnsiWriter = (ansi_file_writer&)Writer;
    if(AnsiWriter.Base.CRCing)
    {
        // We're CRCing, so we need to update our current CRC value
        AddToCRC32(AnsiWriter.Base.CRC, UInt8Count, WritePointer);
    }

    if(fwrite(WritePointer, UInt8Count, 1, AnsiWriter.ANSIFileHandle) == 1)
    {
        return(true);
    }
    else
    {
        ANSILogErrorAsWarning(fwrite);
        return(false);
    }
}

void
AnsiBeginWriterCRC(char const *SourceFileName, int32x SourceLineNumber,
                   file_writer &Writer)
{
    COUNT_BLOCK("AnsiBeginWriterCRC");

    ansi_file_writer &AnsiWriter = (ansi_file_writer&)Writer;
    Assert(!AnsiWriter.Base.CRCing);

    AnsiWriter.Base.CRCing = true;
    BeginCRC32(AnsiWriter.Base.CRC);
}

uint32
AnsiEndWriterCRC(char const *SourceFileName, int32x SourceLineNumber,
                 file_writer &Writer)
{
    COUNT_BLOCK("AnsiBeginWriterCRC");

    ansi_file_writer &AnsiWriter = (ansi_file_writer&)Writer;
    Assert(AnsiWriter.Base.CRCing);

    AnsiWriter.Base.CRCing = false;
    EndCRC32(AnsiWriter.Base.CRC);

    return(AnsiWriter.Base.CRC);
}


file_writer *GRANNY
CreatePlatformFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                         char const *FileNameToOpen, bool EraseExisting)
{
    COUNT_BLOCK("CreatePlatformFileWriter");

    FILE *ANSIFileHandle = fopen(FileNameToOpen, EraseExisting ? "wb" : "r+b");
    if(ANSIFileHandle)
    {
        ansi_file_writer *NewWriter = Allocate(ansi_file_writer);
        if(NewWriter)
        {
            InitializeFileWriter(SourceFileName, SourceLineNumber,
                                 AnsiDeleteFileWriter,
                                 AnsiSeekWriter,
                                 AnsiWrite,
                                 AnsiBeginWriterCRC,
                                 AnsiEndWriterCRC,
                                 NewWriter->Base);
            NewWriter->ANSIFileHandle = ANSIFileHandle;

            return (file_writer*)NewWriter;
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

    return 0;
}

open_file_writer_callback *GRANNY OpenFileWriterCallback = CreatePlatformFileWriter;
