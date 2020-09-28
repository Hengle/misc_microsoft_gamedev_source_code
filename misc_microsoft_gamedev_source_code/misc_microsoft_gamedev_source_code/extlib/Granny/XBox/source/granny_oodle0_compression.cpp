// ========================================================================
// $File: //jeffr/granny/rt/granny_oodle0_compression.cpp $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #13 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_OODLE0_COMPRESSION_H)
#include "granny_oodle0_compression.h"
#endif

#if !defined(GRANNY_COMPRESSION_TOOLS_H)
#include "granny_compression_tools.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif


#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#define DEBUG_DECOMPRESSOR 0
#define DEBUG_COMPRESSOR 0
#if DEBUG_DECOMPRESSOR || DEBUG_COMPRESSOR
#include "granny_file_writer.h"
#endif


#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

/* ========================================================================
   From radlz.h (Jeff's code)
   ======================================================================== */
#define OFFSET_SPLIT_SHIFT 2
#define LARGEST_POSSIBLE_OFFSET ( 65534 << OFFSET_SPLIT_SHIFT )

typedef struct LZCDATA * LZC;
typedef struct LZDDATA * LZD;

// How much memory will the decompressor need with these values
RADDEFFUNC U32 LZ_decompress_alloc_size( U32 max_byte_value,
                                         U32 uniq_byte_values,
                                         U32 max_offset );


// How much memory will the decompressor need with this header
RADDEFFUNC U32 LZ_decompress_alloc_size_from_header( void * header );

// Opens a handle to use for decompressing
RADDEFFUNC LZD LZ_decompress_open( void * ptr,
                                   U32 max_byte_value,
                                   U32 uniq_byte_values,
                                   U32 max_offset );


// Opens a handle to use for decompressing
RADDEFFUNC LZD LZ_decompress_open_from_header( void * ptr,
                                               void * header );


// Decompresses to the next set of symbols.  Returns the number of bytes
//   decompressed.
RADDEFFUNC U32 LZ_decompress( LZD l,
                              ARITHBITS* ab,
                              U8 * output );

/* ========================================================================
   From radlz.c (Jeff's code)
   ======================================================================== */
#define MAX_LENS 64

// split the offsets into two tables, so that the low bits (which are mostly
//   noise don't interfere with the compression of the other bits)
#define LMASK  ( ( 1 << OFFSET_SPLIT_SHIFT ) - 1 )

typedef struct QUICKFIND
{
  U8 const * addr;
  U32 run_count;
  struct QUICKFIND * next[ 3 ];
  struct QUICKFIND * prev[ 3 ];
} QUICKFIND;

typedef struct LZCDATA
{
  ARITH bytes, lens[ MAX_LENS + 1 ], offsl, offsh;
  U32 max_bytes, uniq_bytes, max_offs, max_offsl, max_offsh;
  QUICKFIND ** quick2;
  QUICKFIND ** quick3;
  QUICKFIND ** quick4;
  QUICKFIND * buffer;
  U32 buffer_pos;
  U32 bytes_compressed;
  U32 offsh_used;
  U32 last_len;
  U32 run, last_run;
  U32 hash2_size, hash3_size, hash4_size;
  U8 const * save_pos;
  U32 save_len;
  U32 save_bit_pat;
  void const * save_match;
} LZCDATA;

static U32 long_lengths[ 4 ] = { MAX_LENS * 2, MAX_LENS * 3, MAX_LENS * 4, MAX_LENS * 8 };

#ifdef DISPLAY_HASH_STATS
  #include <stdio.h>
  static U32 max[3]={0,0};
  static F64 tot[3]={0,0};
  static U32 num[3]={0,0};
#endif


typedef struct LZ_HEADER
{
  U32 max_offset_and_byte;
  U32 uniq_offset_and_byte;
  U32 uniq_lens;
} LZ_HEADER;


typedef struct LZDDATA
{
  ARITH bytes, lens[ MAX_LENS + 1 ], offsl, offsh;
  U32 max_bytes, max_offs, max_offsl, max_offsh;
  U32 bytes_decompressed;
  U32 last_len;
} LZDDATA;

#if DEBUG_DECOMPRESSOR
static uint8 *DSource = 0;
static uint8 *DDest = 0;
static uint32 DLength = 0;
#endif

#if COMPILER_MSVC
#pragma optimize("", off)
#endif
static void copy_bytes( void* dest, void* src, U32 length)
{
#if DEBUG_DECOMPRESSOR
    DSource = (uint8 *)src;
    DDest = (uint8 *)dest;
    DLength = length;
#endif
  U8* d, *s;

  s = ( U8* ) src;
  d = ( U8* ) dest;

  while ( length )
  {
    *d++ = *s++;
    --length;
  }
}
#if COMPILER_MSVC
#pragma optimize("", on)
#endif

RADDEFFUNC U32 LZ_decompress_alloc_size( U32 max_byte_value,
                                         U32 uniq_byte_values,
                                         U32 max_offset )
{
  U32 size, h;

  size = sizeof( LZDDATA );
  size += Arith_decompress_alloc_size( uniq_byte_values ); // for bytes

  size += Arith_decompress_alloc_size( MAX_LENS + 1 ) * ( MAX_LENS + 1 ); // for lengths

  h = ( max_offset >= ( LMASK + 1 ) ) ? ( LMASK + 1 ) : max_offset;
  size += Arith_decompress_alloc_size( h ); // for low offsets

  h = ( max_offset >> OFFSET_SPLIT_SHIFT ) + 1 ;
  size += Arith_decompress_alloc_size( h ); // for high offsets

  return( size );
}


RADDEFFUNC LZD LZ_decompress_open( void * ptr,
                                   U32 max_byte_value,
                                   U32 uniq_byte_values,
                                   U32 max_offset )
{
  U32 i, size;
  U8 * addr;
  LZD l = ( LZD ) ptr;

  l->max_bytes = max_byte_value + 1;
  l->max_offs = max_offset;
  l->max_offsl = ( l->max_offs >= ( LMASK + 1 ) ) ? ( LMASK + 1 ) : l->max_offs;
  l->max_offsh = ( l->max_offs >> OFFSET_SPLIT_SHIFT ) + 1;

  l->last_len = 0;
  l->bytes_decompressed = 0;

  l->bytes = Arith_open( l + 1,
                         0,
                         max_byte_value,
                         uniq_byte_values );

  addr = ( ( U8 * ) l->bytes ) + Arith_decompress_alloc_size( uniq_byte_values );

  size =  Arith_decompress_alloc_size( MAX_LENS + 1 );

  for ( i = 0 ; i <= MAX_LENS ; i++ )
  {
    l->lens[ i ] = Arith_open( addr,
                               0,
                               MAX_LENS,
                               MAX_LENS + 1 );

    addr += size;
  }

  l->offsl = Arith_open( addr,
                         0,
                         l->max_offsl - 1,
                         l->max_offsl );

  l->offsh = Arith_open( addr + Arith_compress_alloc_size( l->max_offsl - 1, l->max_offsl ),
                         0,
                         l->max_offsh-1,
                         l->max_offsh );

  return( l );
}


RADDEFFUNC U32 LZ_decompress_alloc_size_from_header( void * header )
{
  U32 size, i;
  LZ_HEADER * h = ( LZ_HEADER * ) header;

  size = sizeof( LZDDATA );
  size += Arith_decompress_alloc_size( h->uniq_offset_and_byte & 511 ); // for bytes

  i = h->max_offset_and_byte >> 9;
  i = ( i >= ( LMASK + 1 ) ) ? ( LMASK + 1 ) : i;
  size += Arith_decompress_alloc_size( i ); // for low offsets

  size += Arith_decompress_alloc_size( h->uniq_offset_and_byte >> 9 ); // for high offsets


  for ( i = 0 ; i < 4 ; i++ )
  {
    U32 s = Arith_decompress_alloc_size( ( h->uniq_lens >> ( ( 3 - i ) * 8 ) ) & 255 );
    size += ( s * ( MAX_LENS / 4 ) );
  }

  size += ( Arith_decompress_alloc_size( h->uniq_lens & 255 ) * ( MAX_LENS - ( ( MAX_LENS / 4 ) * 4 ) + 1 ) );

  return( size );
}


RADDEFFUNC LZD LZ_decompress_open_from_header( void * ptr,
                                               void * header )
{
  U32 i, j, size, uniq;
  U8 * addr;
  LZD l = ( LZD ) ptr;
  LZ_HEADER * h = ( LZ_HEADER * ) header;

  l->max_bytes = h->max_offset_and_byte & 511;
  l->max_offs = h->max_offset_and_byte >> 9;
  l->max_offsl = ( l->max_offs >= ( LMASK + 1 ) ) ? ( LMASK + 1 ) : l->max_offs;
  l->max_offsh = ( l->max_offs >> OFFSET_SPLIT_SHIFT ) + 1;

  l->last_len = 0;
  l->bytes_decompressed = 0;

  size = h->uniq_offset_and_byte & 511;
  l->bytes = Arith_open( l + 1,
                         0,
                         l->max_bytes - 1,
                         size );

  addr = ( ( U8 * ) l->bytes ) + Arith_decompress_alloc_size( size );

  uniq = 0;
  for ( j = 0 ; j < 4 ; j++ )
  {
    uniq = ( h->uniq_lens >> ( ( 3 - j ) * 8 ) ) & 255;
    size = Arith_decompress_alloc_size( uniq );
    for ( i = 0 ; i < ( MAX_LENS / 4 ) ; i++ )
    {
      l->lens[ ( j * ( MAX_LENS / 4 ) ) + i ] = Arith_open( addr, 0, MAX_LENS, uniq );
      addr += size;
    }
  }

  for ( j = ( ( MAX_LENS / 4 ) * 4 ) ; j <= MAX_LENS ; j++ )
  {
    // This use of uniq should indeed inherit the value of the last iteration of the previous loop
    l->lens[ j ] = Arith_open( addr, 0, MAX_LENS, uniq );
    addr += size;
  }

  l->offsl = Arith_open( addr,
                         0,
                         l->max_offsl - 1,
                         l->max_offsl );

  l->offsh = Arith_open( addr + Arith_decompress_alloc_size( l->max_offsl ),
                         0,
                         l->max_offsh-1,
                         h->uniq_offset_and_byte >> 9 );

  return( l );
}


RADDEFFUNC U32 LZ_decompress( LZD l,
                              ARITHBITS* ab,
                              U8 * output )
{
  UINTADDR v;
  U32 escaped;

  // get the length count/type bit

  v = Arith_decompress( l->lens[ l->last_len ], ab );
  if ( Arith_was_escaped( v ) )
  {
    // get the escaped value
    escaped = ArithBitsGetValue( ab, MAX_LENS + 1 );

    // and save it
    Arith_set_decompressed_symbol( v, escaped );

    v = escaped;
  }

  Assert(v <= UInt32Maximum);
  l->last_len = (U32)v;

  // is this a length/offset pair?
  if ( v )
  {
    U32 len, off;

    Assert(v+1 <= UInt32Maximum);
    len = ( v >= ( MAX_LENS - 3 ) ) ? long_lengths[ v - ( MAX_LENS - 3 ) ] : (U32)( v + 1 );

    v = Arith_decompress( l->offsl, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      escaped = ArithBitsGetValue( ab, l->max_offsl );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    Assert(v+1 <= UInt32Maximum);
    off = (U32)(v + 1);

    v = Arith_decompress( l->offsh, ab );
    if ( Arith_was_escaped( v ) )
    {
      U32 offsh_used;

      offsh_used = l->max_offs;
      if ( offsh_used > l->bytes_decompressed )
        offsh_used = l->bytes_decompressed;
      offsh_used = ( offsh_used >> OFFSET_SPLIT_SHIFT ) + 1;

      // get the escaped value
      escaped = ArithBitsGetValue( ab, offsh_used );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    Assert((v << OFFSET_SPLIT_SHIFT) <= UInt32Maximum);
    off = off + (U32)( v << OFFSET_SPLIT_SHIFT );

    //printf("%i: %i %i\n",l->bytes_decompressed, len, off );

    l->bytes_decompressed += len;

    copy_bytes( output, output - off, len );

    return( len );
  }
  else
  {
    // nope, just decode a literal

    v = Arith_decompress( l->bytes, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      escaped = ArithBitsGetValue( ab, l->max_bytes );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    *output = ( U8 ) v;

    ++l->bytes_decompressed;

    return( 1 );
  }
}

/* ========================================================================
   (the tidbits below are functions I wrote so I could easily integrate
    Jeff's code with Granny)
   ======================================================================== */

#define GRANNY_OFFSET LARGEST_POSSIBLE_OFFSET

BEGIN_GRANNY_NAMESPACE;

struct oodle0_state
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



// void GRANNY
// AggrOodle0(oodle0_state *&Oodle0State, uint32x ExpandedDataSize,
//              int32x BufferCount)
// {
//     AggrAllocPtr(Oodle0State);
//     AggrAllocOffsetSizePtr(Oodle0State,
//                            LZ_compress_temp_size(
//                                255, 256,
//                                GRANNY_OFFSET),
//                            TempBuffer);
//     AggrAllocOffsetSizePtr(Oodle0State,
//                            LZ_compress_alloc_size(
//                                255, 256,
//                                GRANNY_OFFSET),
//                            CompressBuffer);
//     AggrAllocOffsetSizePtr(Oodle0State,
//                            BufferCount*SizeOf(LZ_HEADER) +
//                            ((((ExpandedDataSize * 9) + 7) / 8) + 4 + 32),
//                            GiantBuffer);
// }

// void GRANNY
// Oodle0Begin(oodle0_state &Oodle0State,
//               int32x BufferCount)
// {
//     Oodle0State.ExpectedBufferCount = BufferCount;
//     Oodle0State.BufferCount = 0;
//     Oodle0State.Headers = (LZ_HEADER *)Oodle0State.GiantBuffer;
// #if 0 // Crazy compiler problems...
//     ZeroArray((Oodle0State.ExpectedBufferCount),
//               Oodle0State.Headers);
// #else
//  Oodle0SetUInt8( Oodle0State.ExpectedBufferCount * sizeof ( Oodle0State.Headers[0] ), 0, Oodle0State.Headers );
// #endif

//     ArithBitsPutStart(&Oodle0State.ArithBits,
//                       Oodle0State.Headers +
//                       Oodle0State.ExpectedBufferCount);
// }

// int32x GRANNY
// GetOodle0CompressBufferPaddingSize(void)
// {
//     return(8);
// }

// void GRANNY
// Oodle0Compress(oodle0_state &Oodle0State,
//                  int32x BufferSize, void const *Buffer)
// {
//     COUNT_BLOCK("Oodle0Compress");


// #if DEBUG_COMPRESSOR
//  // Debugging only

//     file_writer *DebugWriter;

//     DebugWriter = NewFileWriter("c:\\oodle0debug.oo0", true);
//     if(DebugWriter)
//     {
//         Write(*DebugWriter, BufferSize, Buffer);
//         DeleteFileWriter(DebugWriter);
//     }
// #endif

//     Assert(Oodle0State.BufferCount < Oodle0State.ExpectedBufferCount);
//     LZC lz = LZ_compress_open(Oodle0State.CompressBuffer,
//                               Oodle0State.TempBuffer,
//                               255, 256,
//                               GRANNY_OFFSET);

//     uint8 *i = (uint8 *)Buffer;
//     int32x s = BufferSize;
// #if DEBUG_COMPRESSOR
//     int32x LastS = s;
// #endif
//     while ( s )
//     {
//         uint32x len = LZ_compress(lz, &Oodle0State.ArithBits,
//                                   i, s );
//         s -= len;
//         i += len;

// #if DEBUG_COMPRESSOR
//         // This is for debugging only
//         if((LastS - s) > 10000)
//         {
//             Log2(NoteLogMessage, CompressorLogMessage,
//                 "%d of %d left", s, BufferSize);
//             LastS = s;
//         }
// #endif
//     }

//     LZ_HEADER &Header = Oodle0State.Headers[Oodle0State.BufferCount++];
//     LZ_compress_get_header(lz, &Header);

// }

// int32x GRANNY
// Oodle0End(oodle0_state &Oodle0State, void *&Buffer)
// {
//     Assert(Oodle0State.BufferCount <= Oodle0State.ExpectedBufferCount);
//     ArithBitsFlush(&Oodle0State.ArithBits);
//     Buffer = Oodle0State.GiantBuffer;
//     return((ArithBitsSize(&Oodle0State.ArithBits) / 8) +
//            SizeOf(LZ_HEADER)*Oodle0State.ExpectedBufferCount);
// }

#if DEBUG_DECOMPRESSOR
static uint8 *DBuffer = 0;
#endif

int32x GRANNY
GetOodle0DecompressBufferPaddingSize(void)
{
    return(4);
}


void GRANNY
Oodle0Decompress(bool FileIsByteReversed,
                 int32x CompressedBytesSize,
                 void *CompressedBytes,
                 int32x Stop0, int32x Stop1, int32x Stop2,
                 void *DecompressedBytes)
{
    COUNT_BLOCK("Oodle0Decompress w/ stops");

    if(FileIsByteReversed)
    {
        Reverse32(CompressedBytesSize, CompressedBytes);
    }

    LZ_HEADER *Headers = (LZ_HEADER *)CompressedBytes;

    ARITHBITS ab;
    ArithBitsGetStart(&ab, (U8 *)(Headers + 3));
    void *Temp = AllocateSize(LZ_decompress_alloc_size(
        255, 256, GRANNY_OFFSET));

#if DEBUG_DECOMPRESSOR
    //SetUInt8(Stop2, (uint8)0xAA, DecompressedBytes);
    DBuffer = (uint8 *)DecompressedBytes;
#endif

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
#if DEBUG_DECOMPRESSOR
            if(Size == (0x5c - 2))
            {
                _asm {int 3};
            }
#endif
            U32 len = LZ_decompress(lz, &ab, To);

            Size += len;
            To += len;
        }
        Assert(Size == Stop);
    }}

    Deallocate(Temp);

#if DEBUG_DECOMPRESSOR
    file_writer *DebugWriter;

    DebugWriter = NewFileWriter("g:/debug/Oodle0Decompress.const", true);
    if(DebugWriter)
    {
        Write(*DebugWriter, SizeOf(VarBitsLens), VarBitsLens);
        Write(*DebugWriter, SizeOf(_bitlevels), _bitlevels);
        Write(*DebugWriter, SizeOf(_bitlevelsalign), &_bitlevelsalign);
        Write(*DebugWriter, SizeOf(bits_invert), bits_invert);
        Write(*DebugWriter, SizeOf(bits_invert_8), bits_invert_8);
        DeleteFileWriter(DebugWriter);
    }

    DebugWriter = NewFileWriter("g:/debug/Oodle0Decompress.in", true);
    if(DebugWriter)
    {
        Write(*DebugWriter, SizeOf(Stops), Stops);
        Write(*DebugWriter, CompressedBytesSize, Headers);
        DeleteFileWriter(DebugWriter);
    }

    DebugWriter = NewFileWriter("g:/debug/Oodle0Decompress.out", true);
    if(DebugWriter)
    {
        Write(*DebugWriter, Stop2, DecompressedBytes);
        DeleteFileWriter(DebugWriter);
    }
#endif
}

void GRANNY
Oodle0Decompress(bool FileIsByteReversed,
                 int32x CompressedBytesSize,
                 void *CompressedBytes,
                 int32x Stop,
                 void *DecompressedBytes)
{
    COUNT_BLOCK("Oodle0Decompress");

    if(FileIsByteReversed)
    {
        Reverse32(CompressedBytesSize, CompressedBytes);
    }

    LZ_HEADER *Header = (LZ_HEADER *)CompressedBytes;

    ARITHBITS ab;
    ArithBitsGetStart(&ab, (U8 *)(Header + 1));
    void *Temp = AllocateSize(LZ_decompress_alloc_size(
        255, 256, GRANNY_OFFSET));

    int32x Size = 0;
    uint8 *To = (uint8 *)DecompressedBytes;
    LZD lz = LZ_decompress_open_from_header(Temp, Header);
    while(Size < Stop)
    {
        U32 len = LZ_decompress(lz, &ab, To);

        Size += len;
        To += len;
    }
    Assert(Size == Stop);

    Deallocate(Temp);
}
