// ========================================================================
// $File: //jeffr/granny/rt/granny_oodle1_compression.cpp $
// $DateTime: 2007/10/24 13:15:43 $
// $Change: 16361 $
// $Revision: #19 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_OODLE1_COMPRESSION_H)
#include "granny_oodle1_compression.h"
#endif

// NOTE!  This include has to be here, to avoid the C/C++ include conflict
// that comes from including Jeff's C code.
#include <math.h>

extern "C"
{
#if !defined(VARBITS_H)
#include "varbits.h"
#endif

#if !defined(ARITHBIT_H)
#include "arithbit.h"
#endif

#if !defined(RADLZ_H)
#include "radlz.h"
#endif
};

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

// 2007-01-17: note that the code blocks for DUMP_DATA have been removed.  Look in the
//             file history to retrieve
#define DUMP_DATA 0
#if DUMP_DATA
#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define GRANNY_OFFSET LARGEST_POSSIBLE_OFFSET

typedef struct LZ_HEADER
{
  U32 max_offset_and_byte;
  U32 uniq_offset_and_byte;
  U32 uniq_lens;
} LZ_HEADER;

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct oodle1_state
{
    int32x ExpectedBufferCount;
    int32x BufferCount;

    LZ_HEADER *Headers;

    void *TempBuffer;
    void *CompressBuffer;
    void *GiantBuffer;

    ARITHBITS ArithBits;
};

END_GRANNY_NAMESPACE;

void GRANNY
AggrOodle1(aggr_allocator &Allocator,
           oodle1_state *&Oodle1State, uint32x ExpandedDataSize,
           int32x BufferCount)
{
    AggrAllocPtr(Allocator, Oodle1State);
    AggrAllocOffsetSizePtr(Allocator, Oodle1State,
                           LZ_compress_temp_size(
                               255, 256,
                               GRANNY_OFFSET),
                           TempBuffer);
    AggrAllocOffsetSizePtr(Allocator, Oodle1State,
                           LZ_compress_alloc_size(
                               255, 256,
                               GRANNY_OFFSET),
                           CompressBuffer);
    AggrAllocOffsetSizePtr(Allocator, Oodle1State,
                           BufferCount*SizeOf(LZ_HEADER) +
                           ((((ExpandedDataSize * 9) + 7) / 8) + 4 + 32),
                           GiantBuffer);
}

void GRANNY
Oodle1Begin(oodle1_state &Oodle1State,
            int32x BufferCount)
{
    Oodle1State.ExpectedBufferCount = BufferCount;
    Oodle1State.BufferCount = 0;
    Oodle1State.Headers = (LZ_HEADER *)Oodle1State.GiantBuffer;
    ZeroArray(Oodle1State.ExpectedBufferCount,
              Oodle1State.Headers);
    ArithBitsPutStart(&Oodle1State.ArithBits,
                      Oodle1State.Headers +
                      Oodle1State.ExpectedBufferCount);
}




oodle1_state *GRANNY
Oodle1BeginSimple(uint32x ExpandedDataSize, int32x BufferCount)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    oodle1_state *Oodle1State;
    AggrOodle1(Allocator, Oodle1State, ExpandedDataSize, BufferCount);
    if(EndAggrAlloc(Allocator))
    {
        Oodle1Begin(*Oodle1State, BufferCount);
    }
    return Oodle1State;
}




int32x GRANNY
GetOodle1CompressBufferPaddingSize(void)
{
    return(8);
}

void GRANNY
Oodle1Compress(oodle1_state &Oodle1State,
                 int32x BufferSize, void const *Buffer)
{
    COUNT_BLOCK("Oodle1Compress");

    // 2007-01-17: removed DUMP_DATA: can be found in perforce

    Assert(Oodle1State.BufferCount < Oodle1State.ExpectedBufferCount);
    LZC lz = LZ_compress_open(Oodle1State.CompressBuffer,
                              Oodle1State.TempBuffer,
                              255, 256,
                              GRANNY_OFFSET);

    uint8 *i = (uint8 *)Buffer;
    int32x s = BufferSize;
#if 0
    int32x LastS = s;
#endif
    while ( s )
    {
        uint32x len = LZ_compress(lz, &Oodle1State.ArithBits,
                                  i, s );
        s -= len;
        i += len;

#if 0
        // This is for debugging only
        if((LastS - s) > 10000)
        {
            Log(NoteLogMessage, CompressorLogMessage,
                "%d of %d left", s, BufferSize);
            LastS = s;
        }
#endif
    }

    LZ_HEADER &Header = Oodle1State.Headers[Oodle1State.BufferCount++];
    LZ_compress_get_header(lz, &Header);
}

int32x GRANNY
Oodle1End(oodle1_state &Oodle1State, void *&Buffer,
          bool WritingForReversedPlatform)
{
    Assert(Oodle1State.BufferCount <= Oodle1State.ExpectedBufferCount);
    ArithBitsFlush(&Oodle1State.ArithBits);
    Buffer = Oodle1State.GiantBuffer;

    int32x Size = (ArithBitsSize(&Oodle1State.ArithBits) / 8) +
        SizeOf(LZ_HEADER)*Oodle1State.ExpectedBufferCount;

    if (WritingForReversedPlatform)
    {
        Reverse32(Oodle1State.ExpectedBufferCount * SizeOf(LZ_HEADER), Oodle1State.Headers);
    }

    // 2007-01-17: removed DUMP_DATA: can be found in perforce

    return(Size);
}


void GRANNY
Oodle1FreeSimple(oodle1_state &Oodle1State)
{
    Deallocate ( &Oodle1State );
}


int32x GRANNY
GetOodle1DecompressBufferPaddingSize(void)
{
    return(4);
}


bool GRANNY
Oodle1Decompress(bool FileIsByteReversed,
                 int32x CompressedBytesSize,
                 void const *CompressedBytes,
                 int32x Stop0, int32x Stop1, int32x Stop2,
                 void *DecompressedBytes)
{
    COUNT_BLOCK("Oodle1Decompress w/ stops");

    // 2007-01-17: removed DUMP_DATA: can be found in perforce

#if 1
    // Note: this is a fix for when the compressor was not properly
    // flushing hanging bits, so that you need to make sure there are 0's
    // at the end.
    int32x Rounded = int32x(Align32(CompressedBytesSize) - CompressedBytesSize);
    while(Rounded--)
    {
        ((uint8 *)CompressedBytes)[CompressedBytesSize + Rounded] = 0;
    }
#endif

    // Make a copy of the headers for reversal.  Otherwise, we wind up modifying the
    // /compressed/ buffer, which is a horrendous violation of the principle of least
    // surprise.
    LZ_HEADER const *SrcHeaders = (LZ_HEADER const *)CompressedBytes;
    LZ_HEADER Headers[3];
    Copy(sizeof(Headers), SrcHeaders, &Headers[0]);
    if(FileIsByteReversed)
    {
        Reverse32(3*SizeOf(*Headers), Headers);
    }

    ARITHBITS ab;
    ArithBitsGetStart(&ab, (U8 *)(SrcHeaders + 3));
    uint32 const TempSize = LZ_decompress_alloc_size(255, 256, GRANNY_OFFSET);
    void *Temp = AllocateSize(TempSize);
    if (Temp == 0)
    {
        Log1(ErrorLogMessage, CompressorLogMessage, "Oodle1Decompress unable to alloc %d bytes", TempSize);
        return false;
    }

    int32x Size = 0;
    int32x Stops[] = {Stop0, Stop1, Stop2};
    uint8 *To = (uint8 *)DecompressedBytes;
    {for(int32x BlockIndex = 0;
         BlockIndex < 3;
         ++BlockIndex)
    {
        int32x Stop = Stops[BlockIndex];
#if 1
        LZD lz = LZ_decompress_open_from_header(Temp, &Headers[BlockIndex]);
#else
        LZD lz = LZ_decompress_open(Temp, 255, 256, GRANNY_OFFSET);
#endif

        while(Size < Stop)
        {
            U32 len = LZ_decompress(lz, &ab, To);

            Size += len;
            To += len;
        }
        Assert(Size == Stop);
    }}

    Deallocate(Temp);
    return true;

    // 2007-01-17: removed DUMP_DATA: can be found in perforce
    // 2007-01-17: removed DEBUG_DECOMPRESSOR: can be found in perforce
}

bool GRANNY
Oodle1Decompress(bool FileIsByteReversed,
                 int32x CompressedBytesSize,
                 void const *CompressedBytes,
                 int32x Stop, void *DecompressedBytes)
{
    COUNT_BLOCK("Oodle1Decompress");

    // Make a copy of the headers for reversal.  Otherwise, we wind up modifying the
    // /compressed/ buffer, which is a horrendous violation of the principle of least
    // surprise.
    LZ_HEADER const *SrcHeader = (LZ_HEADER const *)CompressedBytes;
    LZ_HEADER Header;
    Copy(sizeof(Header), SrcHeader, &Header);
    if(FileIsByteReversed)
    {
        Reverse32(SizeOf(Header), &Header);
    }

    ARITHBITS ab;
    ArithBitsGetStart(&ab, (U8 *)(SrcHeader + 1));
    uint32 const TempSize = LZ_decompress_alloc_size(255, 256, GRANNY_OFFSET);
    void *Temp = AllocateSize(TempSize);
    if (Temp == 0)
    {
        Log1(ErrorLogMessage, CompressorLogMessage, "Oodle1Decompress unable to alloc %d bytes", TempSize);
        return false;
    }

    int32x Size = 0;
    uint8 *To = (uint8 *)DecompressedBytes;
    LZD lz = LZ_decompress_open_from_header(Temp, &Header);
    while(Size < Stop)
    {
        U32 len = LZ_decompress(lz, &ab, To);

        Size += len;
        To += len;
    }
    Assert(Size == Stop);

    Deallocate(Temp);
    return true;
}
