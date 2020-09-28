// ========================================================================
// $File: //jeffr/granny/rt/granny_file_compressor.cpp $
// $DateTime: 2007/10/24 13:15:43 $
// $Change: 16361 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_COMPRESSOR_H)
#include "granny_file_compressor.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(GRANNY_OODLE0_COMPRESSION_H)
#include "granny_oodle0_compression.h"
#endif

#if !defined(GRANNY_OODLE1_COMPRESSION_H)
#include "granny_oodle1_compression.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
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

struct no_compression_state
{
    int32x CompressedSize;
};

struct file_compressor
{
    compression_type Type;
    file_writer *Writer;
    bool WritingForReversedPlatform;

    union
    {
        no_compression_state NoCompressionState;
        oodle1_state *Oodle1State;
    };
};

END_GRANNY_NAMESPACE;

int32x GRANNY
GetCompressedBytesPaddingSize(compression_type Format)
{
    switch(Format)
    {
        case Oodle0Compression:
        case Oodle1Compression:
        {
            return(4);
        } //break;

        default:
        {
            return(0);
        } //break;
    }
}

bool GRANNY
DecompressData(compression_type Format,
               bool FileIsByteReversed,
               int32x CompressedBytesSize,
               void *CompressedBytes,
               int32x Stop0, int32x Stop1, int32x Stop2,
               void *DecompressedBytes)
{
    COUNT_BLOCK("DecompressData");

    bool Result = false;

    switch(Format)
    {
        case NoCompression:
        {
            Copy(Stop2, CompressedBytes, DecompressedBytes);
            Result = true;
        } break;

        case Oodle0Compression:
        {
            Oodle0Decompress(FileIsByteReversed,
                             CompressedBytesSize, CompressedBytes,
                             Stop0, Stop1, Stop2, DecompressedBytes);
            Result = true;
        } break;

        case Oodle1Compression:
        {
            Result = Oodle1Decompress(FileIsByteReversed,
                                      CompressedBytesSize, CompressedBytes,
                                      Stop0, Stop1, Stop2, DecompressedBytes);
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Format);
        } break;
    }

    return Result;
}

bool GRANNY
DecompressDataChunk(compression_type Format,
                    bool FileIsByteReversed,
                    int32x CompressedBytesSize,
                    void *CompressedBytes,
                    int32x DecompressedBytesSize,
                    void *DecompressedBytes)
{
    COUNT_BLOCK("DecompressDataChunk");

    bool Result = false;

    switch(Format)
    {
        case NoCompression:
        {
            Copy(DecompressedBytesSize, CompressedBytes, DecompressedBytes);
            Result = true;
        } break;

        case Oodle0Compression:
        {
            Oodle0Decompress(FileIsByteReversed,
                             CompressedBytesSize, CompressedBytes,
                             DecompressedBytesSize, DecompressedBytes);
            Result = true;
        } break;

        case Oodle1Compression:
        {
            Result = Oodle1Decompress(FileIsByteReversed,
                                      CompressedBytesSize, CompressedBytes,
                                      DecompressedBytesSize, DecompressedBytes);
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Format);
        } break;
    }

    return(Result);
}

file_compressor *GRANNY
BeginFileCompression(uint32x ExpandedDataSize,
                     int32x ContentsCount,
                     compression_type Type,
                     bool WritingForReversedPlatform,
                     file_writer *Writer)
{
    file_compressor *Compressor = 0;

    switch(Type)
    {
        case NoCompression:
        {
            aggr_allocator Allocator;
            InitializeAggrAlloc(Allocator);

            AggrAllocPtr(Allocator, Compressor);
            if(EndAggrAlloc(Allocator))
            {
                Compressor->NoCompressionState.CompressedSize = 0;
            }
        } break;

        case Oodle0Compression:
        {
            Assert(!"Oodle0 compression no longer supported, only decompression");
        } break;

        case Oodle1Compression:
        {
            oodle1_state *Oodle1State;

            aggr_allocator Allocator;
            InitializeAggrAlloc(Allocator);

            AggrAllocPtr(Allocator, Compressor);
            AggrOodle1(Allocator, Oodle1State, ExpandedDataSize, ContentsCount);
            if(EndAggrAlloc(Allocator))
            {
                Compressor->Oodle1State = Oodle1State;
                Oodle1Begin(*Compressor->Oodle1State, ContentsCount);
            }
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Type);
        } break;
    }

    if(Compressor)
    {
        Compressor->Type = Type;
        Compressor->Writer = Writer;
        Compressor->WritingForReversedPlatform = WritingForReversedPlatform;
    }

    return(Compressor);
}

bool GRANNY
CompressContentsOfReader(file_compressor &Compressor,
                         int32x FileSize,
                         file_reader &Reader,
                         int32x CopyBufferSize, void *CopyBuffer)
{
    bool Result = false;

    switch(Compressor.Type)
    {
        case NoCompression:
        {
            uint32x CompressedSize = 0;
            Result = ConcatenateFileReader(*Compressor.Writer, Reader,
                                           CopyBufferSize, CopyBuffer,
                                           CompressedSize);
            Compressor.NoCompressionState.CompressedSize += CompressedSize;
        } break;

        case Oodle0Compression:
        {
            Assert(!"Oodle0 compression no longer supported, only decompression");
        } break;

        case Oodle1Compression:
        {
            int32x BufferPadding =
                GetOodle1CompressBufferPaddingSize();

            int32x FilePosition = 0;
            void *Buffer = AllocateSize(FileSize + BufferPadding);
            if(Buffer && ReadExactly(Reader, FilePosition, FileSize, Buffer))
            {
                FilePosition += FileSize;
                Oodle1Compress(*Compressor.Oodle1State,
                               FileSize, Buffer);
                Result = true;
            }
            Deallocate(Buffer);
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Compressor.Type);
        };
    }

    return(Result);
}


bool GRANNY
CompressContentsOfFile(file_compressor &Compressor,
                       int32x FileSize,
                       char const *FileName,
                       int32x CopyBufferSize, void *CopyBuffer)
{
    bool Result = false;

    switch(Compressor.Type)
    {
        case NoCompression:
        {
            uint32x CompressedSize = 0;
            Result = ConcatenateFile(*Compressor.Writer, FileName,
                                     CopyBufferSize, CopyBuffer,
                                     CompressedSize);
            Compressor.NoCompressionState.CompressedSize += CompressedSize;
        } break;

        case Oodle0Compression:
        {
            Assert(!"Oodle0 compression no longer supported, only decompression");
        } break;

        case Oodle1Compression:
        {
            file_reader *Reader = OpenFileReader(FileName);
            if(Reader)
            {
                Result = CompressContentsOfReader(Compressor, FileSize, *Reader,
                                                  CopyBufferSize, CopyBuffer);
                CloseFileReader(Reader);
            }
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Compressor.Type);
        };
    }

    return(Result);
}


bool GRANNY
CompressContentsOfMemory(file_compressor &Compressor,
                         int32x BufferSize, void const *Buffer)
{
    bool Result = false;

    switch(Compressor.Type)
    {
        case NoCompression:
        {
            Result = Write(*Compressor.Writer, BufferSize, Buffer);
            Compressor.NoCompressionState.CompressedSize += BufferSize;
        } break;

        case Oodle0Compression:
        {
            Assert(!"Oodle0 compression no longer supported, only decompression");
        } break;

        case Oodle1Compression:
        {
            // We assume that the padding has been added here...
            Oodle1Compress(*Compressor.Oodle1State, BufferSize, Buffer);
            Result = true;
        } break;

        default:
        {
            Log1(ErrorLogMessage, CompressorLogMessage,
                 "Unrecognized compression type %d", Compressor.Type);
        };
    }

    return(Result);
}



bool GRANNY
EndFileCompression(file_compressor *Compressor,
                   uint32x &CompressedSize)
{
    bool Result = false;
    CompressedSize = 0;

    if(Compressor)
    {
        switch(Compressor->Type)
        {
            case NoCompression:
            {
                Result = true;
                CompressedSize =
                    Compressor->NoCompressionState.CompressedSize;
            } break;

            case Oodle0Compression:
            {
                Assert(!"Oodle0 compression no longer supported, only decompression");
            } break;

            case Oodle1Compression:
            {
                void *Buffer;
                CompressedSize = Oodle1End(*Compressor->Oodle1State, Buffer, Compressor->WritingForReversedPlatform);
                Result = Write(*Compressor->Writer, CompressedSize, Buffer);
            } break;

            default:
            {
                Log1(ErrorLogMessage, CompressorLogMessage,
                     "Unrecognized compression type %d", Compressor->Type);
            };
        }

    }

    Deallocate(Compressor);

    return(Result);
}
