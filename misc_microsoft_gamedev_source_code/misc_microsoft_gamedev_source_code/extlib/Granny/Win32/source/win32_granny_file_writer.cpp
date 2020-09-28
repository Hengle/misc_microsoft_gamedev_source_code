// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_file_writer.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
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

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
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

struct win32_file_writer
{
    file_writer Base;
    HANDLE      Win32FileHandle;
};
CompileAssert(OffsetFromType(win32_file_writer, Base) == 0);
CompileAssert(SizeOf(win32_file_writer) == (SizeOf(file_writer) +
                                            SizeOf(HANDLE)));

END_GRANNY_NAMESPACE;

static void
Win32DeleteFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                      file_writer *Writer)
{
    COUNT_BLOCK("Win32DeleteFileWriter");

    win32_file_writer *Win32Writer = (win32_file_writer*)Writer;
    if(Writer)
    {
        Assert(!Win32Writer->Base.CRCing);

        if(!CloseHandle(Win32Writer->Win32FileHandle))
        {
            Win32LogErrorAsWarning(CloseHandle);
        }

        Deallocate(Writer);
    }
}

static int32x
Win32SeekWriter(char const *SourceFileName,
                int32x SourceLineNumber,
                file_writer &Writer,
                int32x OffsetInUInt8s,
                file_writer_seek_type SeekType)
{
    COUNT_BLOCK("Win32SeekWriter");

    win32_file_writer &Win32Writer = (win32_file_writer&)Writer;
    Assert(!Win32Writer.Base.CRCing || (SeekType == SeekCurrent && OffsetInUInt8s == 0));

    switch (SeekType)
    {
        case SeekStart:
            return(Win32Seek(Win32Writer.Win32FileHandle, OffsetInUInt8s, FILE_BEGIN));
        case SeekEnd:
            return(Win32Seek(Win32Writer.Win32FileHandle, OffsetInUInt8s, FILE_END));
        case SeekCurrent:
            return(Win32Seek(Win32Writer.Win32FileHandle, OffsetInUInt8s, FILE_CURRENT));

        default:
            InvalidCodePath("Invalid seek type");
            return 0;
    }
}

static bool
Win32Write(char const *SourceFileName,
           int32x SourceLineNumber,
           file_writer &Writer,
           int32x UInt8Count,
           void const *WritePointer)
{
    COUNT_BLOCK("Win32Write");

    win32_file_writer &Win32Writer = (win32_file_writer&)Writer;

    if(Win32Writer.Base.CRCing)
    {
        // We're CRCing, so we need to update our current CRC value
        AddToCRC32(Win32Writer.Base.CRC, UInt8Count, WritePointer);
    }

    DWORD UInt8sWritten = 0;
    if(WriteFile(Win32Writer.Win32FileHandle, WritePointer,
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
Win32BeginWriterCRC(char const *SourceFileName,
                    int32x SourceLineNumber,
                    file_writer &Writer)
{
    COUNT_BLOCK("Win32BeginWriterCRC");

    win32_file_writer &Win32Writer = (win32_file_writer&)Writer;
    Assert(!Win32Writer.Base.CRCing);

    Win32Writer.Base.CRCing = true;
    BeginCRC32(Win32Writer.Base.CRC);
}

static uint32
Win32EndWriterCRC(char const *SourceFileName,
                  int32x SourceLineNumber,
                  file_writer &Writer)
{
    COUNT_BLOCK("Win32EndWriterCRC");

    win32_file_writer &Win32Writer = (win32_file_writer&)Writer;
    Assert(Win32Writer.Base.CRCing);

    Win32Writer.Base.CRCing = false;
    EndCRC32(Win32Writer.Base.CRC);

    return(Win32Writer.Base.CRC);
}

file_writer *GRANNY
CreatePlatformFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                         char const *FileNameToOpen, bool EraseExisting)
{
    COUNT_BLOCK("CreatePlatformFileWriter");

    HANDLE Win32FileHandle = CreateFile(
        FileNameToOpen, GENERIC_WRITE, 0, 0,
        EraseExisting ? CREATE_ALWAYS : OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, 0);
    if(Win32FileHandle != INVALID_HANDLE_VALUE)
    {
        win32_file_writer *NewWriter = Allocate(win32_file_writer);
        if(NewWriter)
        {
            InitializeFileWriter(SourceFileName, SourceLineNumber,
                                 Win32DeleteFileWriter,
                                 Win32SeekWriter,
                                 Win32Write,
                                 Win32BeginWriterCRC,
                                 Win32EndWriterCRC,
                                 NewWriter->Base);
            NewWriter->Win32FileHandle = Win32FileHandle;

            return (file_writer*)NewWriter;
        }
        else
        {
            CloseHandle(Win32FileHandle);
        }
    }
    else
    {
        Win32LogErrorAsWarning(CreateFile);
    }

    return 0;
}

open_file_writer_callback *GRANNY OpenFileWriterCallback = CreatePlatformFileWriter;
