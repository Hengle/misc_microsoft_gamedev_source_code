#if !defined(GRANNY_COMPRESSION_TOOLS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_compression_tools.h $
// $DateTime: 2007/08/08 12:28:12 $
// $Change: 15702 $
// $Revision: #19 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

#define UINTADDR uintaddrx
#define INTADDR  intaddrx
#define U64 uint64
#define U32 uint32
#define S32 int32
#define U16 uint16
#define S16 int16
#define U8  uint8
#define S8  int8
#define F32 real32
#define F64 real64x

#if !defined(RAD_NO_LOWERCASE_TYPES)
#define u64 uint64
#define u32 uint32
#define s32 int32
#define u16 uint16
#define s16 int16
#define u8  uint8
#define s8  int8
#define f32 real32
#define f64 real64x
#endif

#define BREAK_POINT() Assert(0);
#define radassert Assert
#define assert Assert
#define memset(Dest, Value, Count) SetUInt8(Count, Value, Dest)
#define radmalloc AllocateSize
#define radfree Deallocate
#define radmemcpy(Dest, Source, Size) Copy(Size, Source, Dest)

#define RADDEFFUNC
#define RADDEFSTART
#define RADDEFEND

#if PROCESSOR_POWERPC
#define __RADPPC__
#endif

/* ========================================================================
   From varbits.h (Jeff's code)
   ======================================================================== */
extern const ALIGN16(U32) GVarBitsLens[33];
extern const ALIGN16(U8) _gbitlevels[129];

typedef struct _VARBITS
{
  void* cur;
  void* init;
  U32 bits;
  U32 bitlen;
} VARBITS;

#define VarBitsOpen(vb,pointer) { (vb).cur=(vb).init=pointer; (vb).bits=(vb).bitlen=0; }
#define VarBitsPut(vb,val,size) { U32 __s=size; U32 __v=(val)&GVarBitsLens[__s]; (vb).bits|=__v<<((vb).bitlen); (vb).bitlen+=__s; if ((vb).bitlen>=32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bitlen-=32; if ((vb).bitlen) { (vb).bits=__v>>(__s-(vb).bitlen); } else (vb).bits=0;  } }
#define VarBitsPut1(vb,boolean) { if (boolean) (vb).bits|=(1<<(vb).bitlen); if ((++(vb).bitlen)==32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bits=(vb).bitlen=0; } }
#define VarBitsPutAlign(vb) { U32 __s2=(32-(vb).bitlen)&31; if (__s2) { VarBitsPut((vb),0,__s2);  } }
#define VarBitsFlush(vb) VarBitsPutAlign(vb)
#define VarBitsSize(vb) ((U32)( (((char*)(vb).cur)-((char*)(vb).init))*8 +(vb).bitlen ))

RADDEFFUNC void VarBitsCopy(VARBITS* dest,VARBITS* src,U32 size);


#ifdef __RADPPC__

#if COMPILER_GCC
//
// Count number of leading zeros
//
inline S32 __cntlzw(S32 arg)
{
  U32 val;

  asm (
    "cntlzw %0,%1"
    : "=r" (val)
    : "r" (arg) );

  return val;
}

#elif defined(COMPILER_MSVC)
// Xenon
//
// Count number of leading zeros
//

#include <ppcintrinsics.h>

#define __cntlzw _CountLeadingZeros

#endif

#define getbitlevel128(n) (U32) (32 - __cntlzw(n))
#define getbitlevel(n)    (U32) (32 - __cntlzw((n) & 0x0000FFFF))

#else

#define getbitlevel128(n) (_gbitlevels[n])

#define getbitlevel(level)      \
(                               \
  ((level)<=128)?               \
    (getbitlevel128(level))     \
  :                             \
  (                             \
    ((level)>=2048)?            \
    (                           \
      ((level)>=8192)?          \
      (                         \
        ((level)>=16384)?       \
          15                    \
        :                       \
          14                    \
      )                         \
      :                         \
      (                         \
        ((level)>=4096)?        \
          13                    \
        :                       \
          12                    \
      )                         \
    )                           \
    :                           \
    (                           \
      ((level)>=512)?           \
      (                         \
        ((level)>=1024)?        \
          11                    \
        :                       \
          10                    \
      )                         \
      :                         \
      (                         \
        ((level)>=256)?         \
          9                     \
        :                       \
          8                     \
      )                         \
    )                           \
  )                             \
)

#endif

#define VarBitsGetAlign(vb) { (vb).bitlen=0; }
#define VarBitsPos(vb) ( (((U8*)(vb).cur)-((U8*)(vb).init))*8-(vb).bitlen )

// don't pass zero to this function
#define GetBitsLen(val) (0xffffffffL>>(U32)(32-(val)))
// for debugging: causes crash on zero: #define GetBitsLen(val) (((val)==0)?(((U8*)val)[0]=0):(0xffffffffL>>(U32)(32-(val))))

#define VarBitsGet1(vb,i)             \
(                                     \
  ((vb).bitlen)?                      \
  (                                   \
    i=((vb).bits)&1,                  \
    ((vb).bits)>>=1,                  \
    --((vb).bitlen),                  \
    i                                 \
  ):(                                 \
    i=*((U32*)((vb).cur)),            \
    ((vb).cur)=((char*)((vb).cur))+4, \
    ((vb).bits)=((U32)i)>>1,          \
    ((vb).bitlen)=31,                 \
    i&1                               \
  )                                   \
)

#define VarBitsGet(v,typ,vb,len)                                \
{                                                               \
  if (((vb).bitlen)>=(len)) {                                   \
    v=(typ)(((vb).bits)&GetBitsLen(len));                       \
    ((vb).bits)>>=(len);                                        \
    ((vb).bitlen)-=(len);                                       \
  } else {                                                      \
    register U32 nb=*((U32*)((vb).cur));                        \
    v=(typ)((((vb).bits)|(nb<<((vb).bitlen)))&GetBitsLen(len)); \
    ((vb).bits)=nb>>((len)-((vb).bitlen));                      \
    ((vb).bitlen)=((vb).bitlen)+32-(len);                       \
    ((vb).cur)=((char*)((vb).cur))+4;                           \
  }                                                             \
}

#define VarBitsUse(vb,len)                         \
{                                                  \
  if (((vb).bitlen)>=(len)) {                      \
    ((vb).bits)>>=(len);                           \
    ((vb).bitlen)-=(len);                          \
  } else {                                         \
    register U32 nb=*((U32*)((vb).cur));           \
    ((vb).bits)=nb>>((len)-((vb).bitlen));         \
    ((vb).bitlen)=((vb).bitlen)+32-(len);          \
    ((vb).cur)=((char*)((vb).cur))+4;              \
  }                                                \
}

// classifies a signed value into: 0 = zero, 1 = neg, 2 = pos
#define CLASSIFY_SIGN( val )  ( (((U32)((S32)(val))) >> 31 ) + ((((U32)(-(S32)(val))) >> 30 ) & 2 ) )

/* ========================================================================
   From arithbit.h (Jeff's code)
   ======================================================================== */
// probability bit macros

typedef struct _ARITHBITS
{
  VARBITS vb;
  U32 high;
  U32 low;
  U32 codeflow;  // underflow on compression, input bits on decompression
} ARITHBITS;

extern const U8 bits_invert[ 16 ];
extern const U8 bits_invert_8[ 8 ];

#define ArithBitsPutStart( ab, ptr )  \
{                                       \
  VarBitsOpen( (ab)->vb, ptr )       \
  (ab)->codeflow = 0;                \
  (ab)->high = 0x7fffffff;           \
  (ab)->low = 0;                     \
}

#define ArithBitsGetStart( ab, ptr )                                          \
{                                                                               \
  U32 tmp;                                                                      \
  ArithBitsPutStart( ab, (U8*)(ptr) );                                          \
  VarBitsGet( tmp, U32, (ab)->vb, 31 );                                      \
  (ab)->codeflow = ( ( ( U32 ) bits_invert[ tmp & 15 ] ) << 27 ) |           \
                      ( ( ( U32 ) bits_invert[ (tmp >> 4 ) & 15 ] ) << 23 ) |   \
                      ( ( ( U32 ) bits_invert[ (tmp >> 8 ) & 15 ] ) << 19 ) |   \
                      ( ( ( U32 ) bits_invert[ (tmp >> 12 ) & 15 ] ) << 15 ) |  \
                      ( ( ( U32 ) bits_invert[ (tmp >> 16 ) & 15 ] ) << 11 ) |  \
                      ( ( ( U32 ) bits_invert[ (tmp >> 20 ) & 15 ] ) << 7 ) |   \
                      ( ( ( U32 ) bits_invert[ (tmp >> 24 ) & 15 ] ) << 3 ) |   \
                      ( ( ( U32 ) bits_invert_8[ (tmp >> 28 ) & 7 ] ) ) ;        \
}

#define ArithBitsFlush( ab )                           \
{                                                        \
  U32 __tmp;                                             \
  VarBitsPut1( (ab)->vb, (ab)->low & 0x20000000 ); \
  ++(ab)->codeflow;                                   \
                                                         \
  __tmp = ( ~(ab)->low & 0x20000000 ) ? 0xffffffff : 0;\
  while ( (ab)->codeflow > 32 )                       \
  {                                                      \
    VarBitsPut( (ab)->vb, __tmp, 32 );                \
    (ab)->codeflow -= 32;                             \
  }                                                      \
  VarBitsPut( (ab)->vb, __tmp, (ab)->codeflow );   \
  (ab)->codeflow = 0;                                 \
  VarBitsFlush( (ab)->vb );                           \
}


#define ArithBitsSize( ab )                          \
  VarBitsSize( (ab)->vb )


#if defined(_MSC_VER) && !GRANNY_SHOW_HARMLESS_WARNINGS
#pragma warning(push)
#pragma warning( disable : 4035 ) // warning C4035: 'multm164anddiv' : no return value
#endif

inline U32 multm164anddiv( U32 mt1, U32 mt2, U32 d )
{
#if PROCESSOR_X86
    __asm
    {
        mov eax,[mt1]
        mov ecx,[mt2]
        mul ecx
        sub eax, 1
        sbb edx, 0
        mov ecx,[d]
        div ecx
    }
#else
    return(U32)(((int64)mt1 * (int64)mt2 - (int64)1) / (int64)d);
#endif
}

#if defined(_MSC_VER) && !GRANNY_SHOW_HARMLESS_WARNINGS
#pragma warning(pop)
#endif



#define ArithBitsGet( ab, scale ) \
  ( multm164anddiv( ( (ab)->codeflow - (ab)->low ) + 1, scale, ( (ab)->high - (ab)->low ) + 1 ) )

RADDEFFUNC void ArithBitsPut ( ARITHBITS* ab, U32 start, U32 range, U32 scale );
RADDEFFUNC void ArithBitsRemove ( ARITHBITS * ab, U32 start, U32 range, U32 scale );

RADDEFFUNC void ArithBitsPutValue ( ARITHBITS* ab, U32 value, U32 scale );
RADDEFFUNC U32 ArithBitsGetValue ( ARITHBITS * ab, U32 scale );

/* ========================================================================
   From radarith.h (Jeff's code)
   ======================================================================== */
typedef struct ARITHDATA* ARITH;

// How much memory will the compressor need with these values
RADDEFFUNC U32 Arith_compress_alloc_size( U32 max_value, U32 unique_values );

// How much memory will the decompressor need with these values
RADDEFFUNC U32 Arith_decompress_alloc_size( U32 unique_values );

// How much temp memory is needed?  temp memory will only be accessed inside
//   Arith_compress and the first 16-bit short will be set to zero whenever it
//   was used.
RADDEFFUNC U32 Arith_compress_temp_size( U32 unique_values );

// Opens a handle to use for compressing or decompressing
RADDEFFUNC ARITH Arith_open( void * ptr, void * compress_temp_buf, U32 max_value, U32 unique_values );

// Compresses one symbol - returns a flag to be checked for an ESCAPE with
//   the Arith_was_escape macro.
RADDEFFUNC U32 Arith_compress( ARITH a, ARITHBITS* ab, U32 value );

// Decompresses one symbol - returns a flag to be checked for an ESCAPE with
//   the Arith_was_escape macro.  If it was an escape, you need to find the
//   actual value (dropping a context, fixed encoding or whatever) and then
//   call Arith_set_decompressed_symbol with the result of Arith_decompress
//   and the new symbol.
RADDEFFUNC UINTADDR Arith_decompress( ARITH a, ARITHBITS* ab );

// What is the high point for simultaneous unique symbols?
RADDEFFUNC U32 Arith_compress_unique_values( ARITH a );

// Makes a guess of the number of bits that this symbol will need.
RADDEFFUNC F64 Arith_estimated_compressed_size( ARITH a, U32 value, U32 scale );

// Increments the internal probably of a symbol by one.
RADDEFFUNC void Arith_adjust_probability( ARITH a, U32 value, S32 count );

// Tells whether the last symbol returned from Arith_compress or Arith_decompress
//   was an escape.
#define Arith_was_escaped( val ) ( ( ( UINTADDR ) ( val ) ) > 65536 )

// Uses to update the decoder with the escaped symbol after you decode it.
#define Arith_set_decompressed_symbol( val, value ) ( * ( ( U16 * ) ( val ) ) ) = ( U16 ) ( value );

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_COMPRESSION_TOOLS_H
#endif
