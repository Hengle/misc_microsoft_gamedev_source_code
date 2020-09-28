// ========================================================================
// $File: //jeffr/granny/rt/granny_file_writer.cpp $
// $DateTime: 2006/11/28 16:29:15 $
// $Change: 13816 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY InitializeFileWriter(char const *SourceFileName,
                                 int32x      SourceLineNumber,
                                 delete_file_writer_callback   *DeleteFileWriterCallback,
                                 seek_file_writer_callback     *SeekWriterCallback,
                                 write_file_writer_callback    *WriteCallback,
                                 begincrc_file_writer_callback *BeginCRCCallback,
                                 endcrc_file_writer_callback   *EndCRCCallback,
                                 file_writer &Writer)
{
    Writer.SourceFileName   = SourceFileName;
    Writer.SourceLineNumber = SourceLineNumber;

    Writer.DeleteFileWriterCallback = DeleteFileWriterCallback;
    Writer.SeekWriterCallback       = SeekWriterCallback;
    Writer.WriteCallback            = WriteCallback;
    Writer.BeginCRCCallback         = BeginCRCCallback;
    Writer.EndCRCCallback           = EndCRCCallback;

    Writer.CRCing = false;
    Writer.CRC    = 0;
}

open_file_writer_callback *GRANNY
GetDefaultFileWriterOpenCallback(void)
{
    return(OpenFileWriterCallback);
}

void GRANNY
SetDefaultFileWriterOpenCallback(open_file_writer_callback *OpenFileWriterCallbackInit)
{
    OpenFileWriterCallback = OpenFileWriterCallbackInit;
}


int32x GRANNY
AlignWriter(file_writer &Writer)
{
    // Determine how far we'll have to go to get to where we want to be
    uint32 Correction = PredictWriterAlignment(GetWriterPosition(Writer));
    Assert(Correction < sizeof(uint32));

    // Only correct our position if it actually needs correcting
    if(Correction > 0)
    {
        // write out zeroes
        uint32 const ZeroBytes = 0;
        Write(Writer, Correction, &ZeroBytes);
    }

    return(Correction);
}

int32x GRANNY
PredictWriterAlignment(int32x Position)
{
    // Determine where we are, and where we'd like to be
    int32x const NewPosition = ((Position + 3) & ~3);

    // Return the difference
    return(NewPosition - Position);
}

int32x GRANNY
SeekWriterFromStartStub(char const *SourceFileName,
                        int32x SourceLineNumber,
                        file_writer& Writer,
                        int32x OffsetInUInt8s)
{
    return (*Writer.SeekWriterCallback)(SourceFileName, SourceLineNumber,
                                        Writer, OffsetInUInt8s, SeekStart);
}

int32x GRANNY
SeekWriterFromEndStub(char const *SourceFileName,
                      int32x SourceLineNumber,
                      file_writer& Writer,
                      int32x OffsetInUInt8s)
{
    return (*Writer.SeekWriterCallback)(SourceFileName, SourceLineNumber,
                                        Writer, OffsetInUInt8s, SeekEnd);
}

int32x GRANNY
SeekWriterFromCurrentPositionStub(char const *SourceFileName,
                                  int32x SourceLineNumber,
                                  file_writer& Writer,
                                  int32x OffsetInUInt8s)
{
    return (*Writer.SeekWriterCallback)(SourceFileName, SourceLineNumber,
                                        Writer, OffsetInUInt8s, SeekCurrent);
}
