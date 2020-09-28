// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_file_writer.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
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

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct xenon_file_writer
{
    file_writer Base;
    HANDLE      XenonFileHandle;
};
CompileAssert(OffsetFromType(xenon_file_writer, Base) == 0);
CompileAssert(SizeOf(xenon_file_writer) == (SizeOf(file_writer) +
                                            SizeOf(HANDLE)));

END_GRANNY_NAMESPACE;


static void
XenonDeleteFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                      file_writer *Writer)
{
    COUNT_BLOCK("XenonDeleteFileWriter");

    xenon_file_writer *XenonWriter = (xenon_file_writer*)Writer;
    if(Writer)
    {
        Assert(!XenonWriter->Base.CRCing);

        if(!CloseHandle(XenonWriter->XenonFileHandle))
        {
            Win32LogErrorAsWarning(CloseHandle);
        }

        Deallocate(Writer);
    }
}

static int32x
XenonSeekWriter(char const *SourceFileName,
                int32x SourceLineNumber,
                file_writer &Writer,
                int32x OffsetInUInt8s,
                file_writer_seek_type SeekType)
{
    COUNT_BLOCK("XenonSeekWriter");

    xenon_file_writer &XenonWriter = (xenon_file_writer&)Writer;
    Assert(!XenonWriter.Base.CRCing || (SeekType == SeekCurrent && OffsetInUInt8s == 0));

    switch (SeekType)
    {
        case SeekStart:
            return(Win32Seek(XenonWriter.XenonFileHandle, OffsetInUInt8s, FILE_BEGIN));
        case SeekEnd:
            return(Win32Seek(XenonWriter.XenonFileHandle, OffsetInUInt8s, FILE_END));
        case SeekCurrent:
            return(Win32Seek(XenonWriter.XenonFileHandle, OffsetInUInt8s, FILE_CURRENT));

        default:
            InvalidCodePath("Invalid seek type");
            return 0;
    }
}

static bool
XenonWrite(char const *SourceFileName,
           int32x SourceLineNumber,
           file_writer &Writer,
           int32x UInt8Count,
           void const *WritePointer)
{
    COUNT_BLOCK("XenonWrite");

    xenon_file_writer &XenonWriter = (xenon_file_writer&)Writer;

    if(XenonWriter.Base.CRCing)
    {
        // We're CRCing, so we need to update our current CRC value
        AddToCRC32(XenonWriter.Base.CRC, UInt8Count, WritePointer);
    }

    DWORD UInt8sWritten = 0;
    if(WriteFile(XenonWriter.XenonFileHandle, WritePointer,
                 UInt8Count, &UInt8sWritten, 0) &&
       ((int32x)UInt8sWritten == UInt8Count))
    {
        return(true);
    }
    else
    {
        Win32LogErrorAsWarning(WriteFile);
        return(false);
    }
}

static void
XenonBeginWriterCRC(char const *SourceFileName,
                    int32x SourceLineNumber,
                    file_writer &Writer)
{
    COUNT_BLOCK("XenonBeginWriterCRC");

    xenon_file_writer &XenonWriter = (xenon_file_writer&)Writer;
    Assert(!XenonWriter.Base.CRCing);

    XenonWriter.Base.CRCing = true;
    BeginCRC32(XenonWriter.Base.CRC);
}

static uint32
XenonEndWriterCRC(char const *SourceFileName,
                  int32x SourceLineNumber,
                  file_writer &Writer)
{
    COUNT_BLOCK("XenonEndWriterCRC");

    xenon_file_writer &XenonWriter = (xenon_file_writer&)Writer;
    Assert(XenonWriter.Base.CRCing);

    XenonWriter.Base.CRCing = false;
    EndCRC32(XenonWriter.Base.CRC);

    return(XenonWriter.Base.CRC);
}

file_writer *GRANNY
CreatePlatformFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                         char const *FileNameToOpen, bool EraseExisting)
{
    COUNT_BLOCK("CreatePlatformFileWriter");

    HANDLE XenonFileHandle = CreateFile(
        FileNameToOpen, GENERIC_WRITE, 0, 0,
        EraseExisting ? CREATE_ALWAYS : OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if(XenonFileHandle != INVALID_HANDLE_VALUE)
    {
        xenon_file_writer *NewWriter = Allocate(xenon_file_writer);
        if(NewWriter)
        {
            InitializeFileWriter(SourceFileName, SourceLineNumber,
                                 XenonDeleteFileWriter,
                                 XenonSeekWriter,
                                 XenonWrite,
                                 XenonBeginWriterCRC,
                                 XenonEndWriterCRC,
                                 NewWriter->Base);
            NewWriter->XenonFileHandle = XenonFileHandle;

            return (file_writer*)NewWriter;
        }
        else
        {
            CloseHandle(XenonFileHandle);
        }
    }
    else
    {
        Win32LogErrorAsWarning(CreateFile);
    }

    return 0;
}

open_file_writer_callback *GRANNY OpenFileWriterCallback = CreatePlatformFileWriter;
