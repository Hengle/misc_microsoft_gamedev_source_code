// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_file_writer.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMORY_FILE_WRITER_H)
#include "granny_memory_file_writer.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct memory_file_writer
{
    file_writer Base;

    // For now, implemented as a flat growable array, may be
    //  better to implement as a list of blocks, given that
    //  we don't really want to use the stl style 2x growth
    //  method
    bool   StolenBuffer;
    uint8* Buffer;
    int32x Capacity;
    int32x BlockSize;

    int32x CurrentPosition;
    int32x CurrentSize;
};
CompileAssert(OffsetFromType(memory_file_writer, Base) == 0);

END_GRANNY_NAMESPACE;

static bool
GrowWriterToSize(memory_file_writer& Writer,
                 int32x NewSize)
{
    Assert(NewSize > 0);
    Assert(Writer.BlockSize > 0);
    Assert(NewSize > Writer.Capacity);

    // We allow non-pow2 block sizes, so no (& ~(BS - 1)) trick
    // allowed.
    int32x const RoundedCapacity = ((NewSize + Writer.BlockSize - 1) / Writer.BlockSize) * Writer.BlockSize;
    Assert(RoundedCapacity > Writer.Capacity);
    Assert(RoundedCapacity >= NewSize);

    bool Result = false;
    uint8* NewBuffer = (uint8*)AllocateSize(RoundedCapacity);
    if (NewBuffer)
    {
        Copy(Writer.CurrentSize, Writer.Buffer, NewBuffer);

        Deallocate(Writer.Buffer);
        Writer.Buffer = NewBuffer;
        Writer.Capacity = RoundedCapacity;
        Result = true;
    }

    return Result;
}

static void
MemoryDeleteWriter(char const *SourceFileName, int32x SourceLineNumber,
                   file_writer *Writer)
{
    COUNT_BLOCK("MemoryDeleteWriter");

    memory_file_writer *MemoryWriter = (memory_file_writer*)Writer;
    if(Writer)
    {
        Assert(!MemoryWriter->Base.CRCing);

        // Perfectly ok for this to be null in certain circumstances, i.e, if
        //  someone has taken possession of the buffer, or no bytes were
        //  written
        if (MemoryWriter->Buffer != NULL)
        {
            Deallocate(MemoryWriter->Buffer);
            MemoryWriter->Buffer = NULL;
        }

        Deallocate(Writer);
    }
}

static int32x
MemorySeekWriter(char const *SourceFileName, int32x SourceLineNumber,
               file_writer &Writer, int32x OffsetInUInt8s,
               file_writer_seek_type SeekType)
{
    COUNT_BLOCK("MemorySeekWriter");

    memory_file_writer &MemoryWriter = (memory_file_writer&)Writer;
    Assert(!MemoryWriter.Base.CRCing || (SeekType == SeekCurrent && OffsetInUInt8s == 0));
    Assert(!MemoryWriter.StolenBuffer);

    int32x TargetPosition;
    switch (SeekType)
    {
        case SeekStart:
            TargetPosition = OffsetInUInt8s;
            break;
        case SeekEnd:
            TargetPosition = MemoryWriter.CurrentSize - OffsetInUInt8s;
            break;
        case SeekCurrent:
            TargetPosition = MemoryWriter.CurrentPosition + OffsetInUInt8s;
            break;

        default:
            InvalidCodePath("Invalid seek type");
            return 0;
    }

    if (TargetPosition < 0)
    {
        Log1(ErrorLogMessage, FileWritingLogMessage,
             "Attempting to seek before beginning of a memory stream [r: %d]",
             TargetPosition);
        return 0;
    }
    else if (TargetPosition > MemoryWriter.CurrentSize)
    {
        // Increase capacity to handle the new position...
        if (!GrowWriterToSize(MemoryWriter, TargetPosition))
        {
            Log2(ErrorLogMessage, FileWritingLogMessage,
                 "Unable to grow memory stream to requested size [r: %d curr: %d]",
                 TargetPosition, MemoryWriter.Capacity);
            return 0;
        }
    }

    MemoryWriter.CurrentPosition = TargetPosition;
    return TargetPosition;
}

static bool
MemoryWrite(char const *SourceFileName, int32x SourceLineNumber,
          file_writer &Writer, int32x UInt8Count, void const *WritePointer)
{
    COUNT_BLOCK("MemoryWrite");

    memory_file_writer &MemoryWriter = (memory_file_writer&)Writer;
    Assert(!MemoryWriter.StolenBuffer);
    if(MemoryWriter.Base.CRCing)
    {
        // We're CRCing, so we need to update our current CRC value
        AddToCRC32(MemoryWriter.Base.CRC, UInt8Count, WritePointer);
    }

    const int32x ImpliedSize = MemoryWriter.CurrentPosition + UInt8Count;
    if (ImpliedSize > MemoryWriter.Capacity)
    {
        if (!GrowWriterToSize(MemoryWriter, ImpliedSize))
        {
            Log2(ErrorLogMessage, FileWritingLogMessage,
                 "Unable to grow memory stream to requested size [r: %d curr: %d]",
                 ImpliedSize, MemoryWriter.Capacity);
            return false;
        }
    }
    Assert(MemoryWriter.Capacity >= ImpliedSize);

    Copy(UInt8Count, WritePointer,
         MemoryWriter.Buffer + MemoryWriter.CurrentPosition);
    MemoryWriter.CurrentPosition += UInt8Count;
    Assert(MemoryWriter.CurrentPosition <= MemoryWriter.Capacity);

    if (MemoryWriter.CurrentPosition > MemoryWriter.CurrentSize)
    {
        MemoryWriter.CurrentSize = MemoryWriter.CurrentPosition;
    }

    return true;
}

static void
MemoryBeginWriterCRC(char const *SourceFileName, int32x SourceLineNumber,
                     file_writer &Writer)
{
    COUNT_BLOCK("MemoryBeginWriterCRC");

    memory_file_writer &MemoryWriter = (memory_file_writer&)Writer;
    Assert(!MemoryWriter.StolenBuffer);
    Assert(!MemoryWriter.Base.CRCing);

    MemoryWriter.Base.CRCing = true;
    BeginCRC32(MemoryWriter.Base.CRC);
}

static uint32
MemoryEndWriterCRC(char const *SourceFileName, int32x SourceLineNumber,
                 file_writer &Writer)
{
    COUNT_BLOCK("MemoryBeginWriterCRC");

    memory_file_writer &MemoryWriter = (memory_file_writer&)Writer;
    Assert(!MemoryWriter.StolenBuffer);
    Assert(MemoryWriter.Base.CRCing);

    MemoryWriter.Base.CRCing = false;
    EndCRC32(MemoryWriter.Base.CRC);

    return(MemoryWriter.Base.CRC);
}

file_writer *GRANNY
CreateMemoryFileWriter(char const *SourceFileName, int32x SourceLineNumber,
                       int32x BlockSize)
{
    COUNT_BLOCK("CreateMemoryFileWriter");

    memory_file_writer *NewWriter = Allocate(memory_file_writer);
    if(NewWriter)
    {
        InitializeFileWriter(SourceFileName, SourceLineNumber,
                             MemoryDeleteWriter,
                             MemorySeekWriter,
                             MemoryWrite,
                             MemoryBeginWriterCRC,
                             MemoryEndWriterCRC,
                             NewWriter->Base);
        NewWriter->StolenBuffer    = false;
        NewWriter->Buffer          = NULL;
        NewWriter->Capacity        = 0;
        NewWriter->BlockSize       = BlockSize;
        NewWriter->CurrentPosition = 0;
        NewWriter->CurrentSize     = 0;

    }
    else
    {
        Log0(ErrorLogMessage, FileWritingLogMessage, "Unable to allocate a memory writer...");
    }

    return (file_writer*)NewWriter;
}

bool GRANNY
StealMemoryWriterBuffer(file_writer &Writer,
                        uint8  *&BufferPtr,
                        int32x &BufferSize)
{
    memory_file_writer &MemoryWriter = (memory_file_writer&)Writer;
    Assert(!MemoryWriter.StolenBuffer);

    if (MemoryWriter.StolenBuffer)
    {
        BufferPtr = NULL;
        BufferSize = 0;
        return false;
    }

    BufferPtr  = MemoryWriter.Buffer;
    BufferSize = MemoryWriter.CurrentSize;

    MemoryWriter.Buffer = NULL;
    MemoryWriter.Capacity = 0;
    MemoryWriter.CurrentSize = 0;
    MemoryWriter.CurrentPosition = 0;
    MemoryWriter.StolenBuffer = true;

    return true;
}


void GRANNY
FreeMemoryWriterBuffer(uint8* Buffer)
{
    if (Buffer)
    {
        Deallocate(Buffer);
    }
}

