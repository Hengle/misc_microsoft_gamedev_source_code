// ========================================================================
// $File: //jeffr/granny/rt/granny_compression_tools.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_COMPRESSION_TOOLS_H)
#include "granny_compression_tools.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if COMPILER_MSVC
#pragma warning(disable:4035)
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

const ALIGN16(U32) GVarBitsLens[ 33 ] =
{
           0,         1,           3,          7,
         0xf,      0x1f,        0x3f,       0x7f,
        0xff,     0x1ff,       0x3ff,      0x7ff,
       0xfff,    0x1fff,      0x3fff,     0x7fff,
      0xffff,    0x1ffff,    0x3ffff,    0x7ffff,
     0xfffff,   0x1fffff,   0x3fffff,   0x7fffff,
    0xffffff,  0x1ffffff,  0x3ffffff,  0x7ffffff,
   0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
  0xffffffff
};


//                                                                                                                                                                                                                               1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
//                                           1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3 3 4 4 4 4 4 4 4 4 4 4 5 5 5 5 5 5 5 5 5 5 6 6 6 6 6 6 6 6 6 6 7 7 7 7 7 7 7 7 7 7 8 8 8 8 8 8 8 8 8 8 9 9 9 9 9 9 9 9 9 9 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2
//                       0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8
const ALIGN16(U8) _gbitlevels[129]={0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8};


const U8 bits_invert[ 16 ] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
const U8 bits_invert_8[ 8 ] = { 0, 4, 2, 6, 1, 5, 3, 7 };

#define BINS 16

typedef struct ARITHDATA* ARITH;

typedef struct ARITHDATA
{
  U16 totals[ BINS ];               // 0

  U16 number;                       // 32
  U16 temp;                         // 34

  U16 last_bin_start;               // 36
  U16 bin_shift;                    // 38

  U16 bin_size;                     // 40
  U16 max_unique;                   // 42

  U16 unique_symb;                  // 44
  U16 align;                        // 46

  U16 * values;                     // 48
  U16 * comp_reverse;               // 52
  U16 counts[ 0 ];                  // 56
} ARITHDATA;

END_GRANNY_NAMESPACE;

/* ========================================================================
   From varbits.c (Jeff's code)
   ======================================================================== */
RADDEFFUNC void GRANNY VarBitsCopy(VARBITS* dest,VARBITS* src,U32 size)
{
  U32 val;
  while (size>=8) {
    VarBitsGet(val,U32,*src,8);
    VarBitsPut(*dest,val,8);
    size-=8;
  }

  if (size) {
    VarBitsGet(val,U32,*src,size);
    VarBitsPut(*dest,val,size);
  }
}


/* ========================================================================
   From arithbit.c (Jeff's code)
   ======================================================================== */
static U32 __inline mult64anddiv( U32 mt1,U32 mt2, U32 d )
{
#if PROCESSOR_X86
    __asm
    {
        mov eax,[mt1]
        mov ecx,[mt2]
        mul ecx
        mov ecx,[d]
        div ecx
    }
#else
    return(U32)(((int64)mt1 * (int64)mt2) / (int64)d);
#endif
}

static void put_symbol( ARITHBITS* ab, U32 start, U32 range, U32 scale )
{
  U32 high;
  U32 low;
  U32 tmp;
  U32 flow;

  high = ab->high;
  low = ab->low;
  flow = ab->codeflow;

  tmp = ( high - low ) + 1;
  high = low + mult64anddiv( tmp, start + range, scale ) - 1;
  low  = low + mult64anddiv( tmp, start, scale );

  assert( low <= high );

  if ( ( ( high ^ low ) & 0x40000000 ) == 0 )
  {
    if ( flow )
    {
      VarBitsPut1( ab->vb, high & 0x40000000 );

      tmp = ( ~high & 0x40000000 ) ? 0xffffffff : 0;
      while ( flow > 32 )
      {
        VarBitsPut( ab->vb, tmp, 32 );
        flow -= 32;
      }
      VarBitsPut( ab->vb, tmp, flow );
      flow = 0;

      high <<= 1;
      low <<= 1;
      high |= 1;
    }

    while ( ( ( high ^ low ) & 0x7f800000 ) == 0 )
    {
      VarBitsPut( ab->vb, (bits_invert[ ( high >> 23 ) & 0xf ] << 4 ) | bits_invert[ ( high >> 27 ) & 0xf ], 8 );

      high <<= 8;
      low <<= 8;
      high |= 0xff;
    }

    if ( ( ( high ^ low ) & 0x78000000 ) == 0 )
    {
      VarBitsPut( ab->vb, bits_invert[ ( high >> 27 ) & 0xf ], 4 );

      high <<= 4;
      low <<= 4;
      high |= 0xf;
    }

    while ( ( ( high ^ low ) & 0x40000000 ) == 0 )
    {
      VarBitsPut1( ab->vb, high & 0x40000000 );

      high <<= 1;
      low <<= 1;
      high |= 1;
    }

  }

  while ( ( low & 0x20000000 ) && ( ! ( high & 0x20000000 ) ) )
  {
    ++flow;

    high <<= 1;
    low &= 0x1fffffff;
    high |= 0x40000001;
    low <<= 1;

  }

  ab->high = high & 0x7fffffff;
  ab->low = low & 0x7fffffff;
  ab->codeflow = flow;
}


RADDEFFUNC void GRANNY ArithBitsPut( ARITHBITS* ab, U32 start, U32 range, U32 scale )
{
  put_symbol( ab, start, range, scale );
}


RADDEFFUNC void GRANNY ArithBitsPutValue ( ARITHBITS* ab, U32 value, U32 scale )
{
  put_symbol( ab, value, 1, scale );
}


static void remove_symbol( ARITHBITS * ab, U32 start, U32 range, U32 scale )
{
  U32 high;
  U32 low;
  U32 tmp;
  U32 code;

  high = ab->high;
  low = ab->low;
  code = ab->codeflow;

  tmp = ( high - low ) + 1;
  high = low + mult64anddiv( tmp, start + range, scale ) - 1;
  low  = low + mult64anddiv( tmp, start, scale );

  assert( low <= high );

  if ( ( ( high ^ low ) & 0x40000000 ) == 0 )
  {
    while ( ( ( high ^ low ) & 0x7f800000 ) == 0 )
    {
      low <<= 8;
      high <<= 8;
      high |= 0xff;

      VarBitsGet( tmp, U32, ab->vb, 8 );
      code = ( code << 8 ) | (bits_invert[ tmp & 15 ] << 4) | bits_invert[ tmp >> 4 ];
    }

    if ( ( ( high ^ low ) & 0x78000000 ) == 0 )
    {
      low <<= 4;
      high <<= 4;
      high |= 0xf;

      VarBitsGet( tmp, U32, ab->vb, 4 );
      code = ( code << 4 ) | bits_invert[ tmp ];
    }

    while ( ( ( high ^ low ) & 0x40000000 ) == 0 )
    {
      low <<= 1;

      high <<= 1;
      high |= 1;

      code = ( code << 1 ) | VarBitsGet1( ab->vb, tmp );
    }
  }

  while ( ( low & 0x20000000 ) && ( ! ( high & 0x20000000 ) ) )
  {
    code ^= 0x20000000;

    low &= 0x1fffffff;
    low <<= 1;

    high <<= 1;
    high |= 0x40000001;

    code = ( code << 1 ) | VarBitsGet1( ab->vb, tmp );
  }

  ab->high = high & 0x7fffffff;
  ab->low = low & 0x7fffffff;
  ab->codeflow = code & 0x7fffffff;
}


RADDEFFUNC void GRANNY ArithBitsRemove( ARITHBITS * ab, U32 start, U32 range, U32 scale )
{
  remove_symbol( ab, start, range, scale );
}


RADDEFFUNC U32 GRANNY ArithBitsGetValue ( ARITHBITS * ab, U32 scale )
{
  U32 count;

  // get the count (which is the symbol itself in an even distribution)
  count = ArithBitsGet( ab, scale );

  // remove the symbol
  remove_symbol( ab, count, 1, scale );

  return( count );
}

/* ========================================================================
   From radarith.c (Jeff's code)
   ======================================================================== */
// adaptive arithmetic modeller

static void calc_best_shift ( ARITH a, U32 value )
{
  U32 i;
  U32 best_bin;
  U32 best_max;

  // downshift to linear mode when only a few symbols
  if ( value < 6 )
  {
    a->bin_size = 0;
    a->bin_shift = 15;
    a->last_bin_start = 0;
  }
  else
  {
    best_max = 0xffffffff;
    best_bin = 0;

    for( i = 0 ; i < 16 ; i++ )
    {
      U32 max, bins, size;

      size = 1 << i;

      bins = ( value + ( size - 1 ) ) / size;
      if ( bins > BINS ) bins = BINS;

      max = value - ( size * ( bins - 1 ) );
      if ( max < size )
        max = size;

      if ( max < best_max )
      {
        best_bin = i;
        best_max = max;
      }

      if ( size > value )
        break;
    }

    a->bin_size = (U16)( 1 << best_bin );
    a->bin_shift = (U16) best_bin;
    a->last_bin_start = (U16)( ( BINS - 1 ) * a->bin_size );
  }
}


// quick means that the amount is already doubled into the high word
//   (0x10001 for incrementing both by 1)

static void quick_increment_counts ( ARITH a, U32 value, U32 amount )
{
  if ( value >= a->last_bin_start )
  {
    a->totals[ BINS - 1 ] = ( U16 ) ( amount + a->totals[ BINS - 1 ] );
  }
  else
  {
    unsigned int b;

    b = value >> a->bin_shift;

    if ( b & 1 )
    {
      a->totals[ b ] = ( U16 ) ( amount + a->totals[ b ] );
      ++b;
    }

    assert( b < 15 );  // need additional cases below if you increase BINS

    switch ( b >> 1 )
    {
      case 0:
        ( ( U32* ) a->totals )[ 0 ] += amount;
      case 1:
        ( ( U32* ) a->totals )[ 1 ] += amount;
      case 2:
        ( ( U32* ) a->totals )[ 2 ] += amount;
      case 3:
        ( ( U32* ) a->totals )[ 3 ] += amount;
      case 4:
        ( ( U32* ) a->totals )[ 4 ] += amount;
      case 5:
        ( ( U32* ) a->totals )[ 5 ] += amount;
      case 6:
        ( ( U32* ) a->totals )[ 6 ] += amount;
      case 7:
        ( ( U32* ) a->totals )[ 7 ] += amount;
        break;
    }
  }

  a->counts[ value ] = ( U16 ) ( amount + a->counts[ value ] );
}


#define decrement_counts( a, value, amount )   \
{                                              \
  U32 __tmp = -( signed long ) ( amount );     \
  quick_increment_counts( ( a ), ( value ), ( ( __tmp - 1 ) << 16 ) | ( U32) (U16 ) __tmp); \
}


#define COUNTS_SIZE( value ) ( 2 * ( ( value + 1 + 1 + 3 ) & ~3 ) )
#define VALUES_SIZE( value ) ( 2 * ( ( value + 1 + 1 + 3 ) & ~3 ) )


RADDEFFUNC U32 GRANNY Arith_compress_alloc_size( U32 max_value, U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( max_value ) );
}

RADDEFFUNC U32 GRANNY Arith_decompress_alloc_size( U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( unique_values ) );
}

RADDEFFUNC U32 GRANNY Arith_compress_temp_size( U32 unique_values )
{
  return( VALUES_SIZE( unique_values ) );
}

RADDEFFUNC ARITH GRANNY Arith_open( void * ptr, void * compress_temp_buf, U32 max_value, U32 unique_values )
{
  ARITH a;

  a = (ARITH ) ptr;

  if ( a )
  {
    // clears the buffer plus all the counts
    memset( a, 0, ( compress_temp_buf ) ?
                    Arith_compress_alloc_size( max_value, unique_values ) :
                    Arith_decompress_alloc_size( unique_values ));

    a->values = ( U16 * ) ( ( char * ) a + ( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) ) );

    a->comp_reverse = ( U16 *) compress_temp_buf;

    a->unique_symb = ( U16 ) unique_values;

    // calculate the inital bin sizes
    calc_best_shift( a, unique_values + 1 ); // 1 for round up, 1 for escape

    // add the single code for escape
    quick_increment_counts( a, 0, 0x30003 );
  }

  return( a );
}


RADDEFFUNC U32 GRANNY Arith_compress_unique_values( ARITH a )
{
  if ( a->number > a->max_unique )
  {
    a->max_unique = a->number;
  }

  return( a->max_unique );
}


static unsigned int get_symb_start_range_and_add ( ARITH a, U32 pos )
{
  U32 i, s;

  if ( pos < a->bin_size )
  {
    s = 0;
    i = 0;
    goto do_zero_bin;
  }
  else
  {
    if ( pos >= a->last_bin_start )
    {
      i = a->last_bin_start;
      s = a->totals[ BINS - 2 ];
      ++a->totals[ BINS - 1 ];
    }
    else
    {
      U32 b;

      b = pos >> a->bin_shift;
      s = a->totals[ b - 1 ];
      i = b << a->bin_shift;

      if ( b & 1 )
      {
        ++a->totals[ b ];
        ++b;
      }

      assert( b < 15 );  // need additional cases below if you increase BINS

      switch ( b >> 1 )
      {
        do_zero_bin:
        case 0:
          ( ( U32* ) a->totals )[ 0 ] += 0x10001;
        case 1:
          ( ( U32* ) a->totals )[ 1 ] += 0x10001;
        case 2:
          ( ( U32* ) a->totals )[ 2 ] += 0x10001;
        case 3:
          ( ( U32* ) a->totals )[ 3 ] += 0x10001;
        case 4:
          ( ( U32* ) a->totals )[ 4 ] += 0x10001;
        case 5:
          ( ( U32* ) a->totals )[ 5 ] += 0x10001;
        case 6:
          ( ( U32* ) a->totals )[ 6 ] += 0x10001;
        case 7:
          ( ( U32* ) a->totals )[ 7 ] += 0x10001;
          break;
      }

    }
  }

  for ( ; i < pos ; i++ )
  {
    s += a->counts[ i ];
  }

  return( s );
}

static U32 find_pos_from_count_and_add ( ARITH a, U32 count, U32* start )
{
  U32 pos;
  U32 tmp;
  U32 s;

  if ( ( U16 ) count < a->totals[ 7 ] )
    if ( ( U16 ) count < a->totals[ 3 ] )
      if ( ( U16 ) count < a->totals[ 1 ] )
        if ( ( U16 ) count < a->totals[ 0 ] )
        {
          s = 0;
          pos = 0;
          goto do_0_bin;
        }
        else
        {
          pos = a->bin_size;
          s = a->totals[ 0 ];
          ++a->totals[ 1 ];
          goto do_2_bin;
        }
      else
        if ( ( U16 ) count < a->totals[ 2 ] )
        {
          s = a->totals[ 1 ];
          pos = 2 << a->bin_shift;
          goto do_2_bin;
        }
        else
        {
          s = a->totals[ 2 ];
          pos = 3 << a->bin_shift;
          ++a->totals[ 3 ];
          goto do_4_bin;
        }
    else
      if ( ( U16 ) count < a->totals[ 5 ] )
        if ( ( U16 ) count < a->totals[ 4 ] )
        {
          s = a->totals[ 3 ];
          pos = 4 << a->bin_shift;
          goto do_4_bin;
        }
        else
        {
          s = a->totals[ 4 ];
          pos = 5 << a->bin_shift;
          ++a->totals[ 5 ];
          goto do_6_bin;
        }
      else
        if ( ( U16 ) count < a->totals[ 6 ] )
        {
          s = a->totals[ 5 ];
          pos = 6 << a->bin_shift;
          goto do_6_bin;
        }
        else
        {
          s = a->totals[ 6 ];
          pos = 7 << a->bin_shift;
          ++a->totals[ 7 ];
          goto do_8_bin;
        }
  else
    if ( ( U16 ) count < a->totals[ 11 ] )
      if ( ( U16 ) count < a->totals[ 9 ] )
        if ( ( U16 ) count < a->totals[ 8 ] )
        {
          s = a->totals[ 7 ];
          pos = 8 << a->bin_shift;
          goto do_8_bin;
        }
        else
        {
          s = a->totals[ 8 ];
          pos = 9 << a->bin_shift;
          ++a->totals[ 9 ];
          goto do_10_bin;
        }
      else
        if ( ( U16 ) count < a->totals[ 10 ] )
        {
          s = a->totals[ 9 ];
          pos = 10 << a->bin_shift;
          goto do_10_bin;
        }
        else
        {
          s = a->totals[ 10 ];
          pos = 11 << a->bin_shift;
          ++a->totals[ 11 ];
          goto do_12_bin;
        }
    else
      if ( ( U16 ) count < a->totals[ 13 ] )
        if ( ( U16 ) count < a->totals[ 12 ] )
        {
          s = a->totals[ 11 ];
          pos = 12 << a->bin_shift;
          goto do_12_bin;
        }
        else
        {
          s = a->totals[ 12 ];
          pos = 13 << a->bin_shift;
          ++a->totals[ 13 ];
          goto do_14_bin;
        }
      else
        if ( ( U16 )count < a->totals[ 14 ] )
        {
          s = a->totals[ 13 ];
          pos = 14 << a->bin_shift;
          goto do_14_bin;
        }
        else
        {
          pos = a->last_bin_start;
          s = a->totals[ 14 ];
          ++a->totals[ BINS - 1 ];
          goto noinc;
        }

  do_0_bin:
   ( ( U32* ) a->totals )[ 0 ] += 0x10001;
  do_2_bin:
   ( ( U32* ) a->totals )[ 1 ] += 0x10001;
  do_4_bin:
   ( ( U32* ) a->totals )[ 2 ] += 0x10001;
  do_6_bin:
   ( ( U32* ) a->totals )[ 3 ] += 0x10001;
  do_8_bin:
   ( ( U32* ) a->totals )[ 4 ] += 0x10001;
  do_10_bin:
   ( ( U32* ) a->totals )[ 5 ] += 0x10001;
  do_12_bin:
   ( ( U32* ) a->totals )[ 6 ] += 0x10001;
  do_14_bin:
   ( ( U32* ) a->totals )[ 7 ] += 0x10001;

 noinc:

  while ( 1 )
  {
    tmp = s + a->counts[ pos ];
    if ( count < tmp )
    {
      *start = s;
      return( pos );
    }
    ++pos;

    s = tmp + a->counts[ pos ];
    if ( count < s )
    {
      *start = tmp;
      return( pos );
    }
    ++pos;
  }
}


static void finish_rescale ( ARITH a, U32 * tots )
{
  unsigned long i;

  // if there are any symbols left, make sure the escape code is defined
  if ( ( a->number != a->unique_symb ) && ( a->counts[ 0 ] == 0 ) )
  {
    a->counts[ 0 ] += 2;
    tots[ ( 0 < a->last_bin_start ) ? 0 : ( BINS - 1 ) ] += 2;
  }

  // calculate from the bins into the total
  a->totals[ 0 ] = ( U16 ) tots[ 0 ];

  for ( i = 1 ; i < BINS ; i++ )
  {
    a->totals[ i ] = ( U16 ) ( a->totals[ i - 1 ] + tots[ i ] );
  }

}


// These functions rescale the counts every 16384 symbols.  Because they are
//   called so infrequently, they contribute less than 1 cycle per symbol
//   even though they has quite a bit of work to do.

static void rescale_compress( ARITH a )
{
  unsigned long i, j;
  U32 tots[ BINS ];
  U32 max;
  U32 pos;

  if ( a->number > a->max_unique )
  {
    a->max_unique = a->number;
  }

  calc_best_shift( a, ( a->number + 1 ) );

  memset( tots, 0, sizeof( tots ) );

  // rescale the escape count
  a->counts[ 0 ] >>= 1;
  tots[ ( 0 < a->last_bin_start ) ? 0 : ( BINS - 1 ) ] = a->counts[ 0 ];

  i = 0;
  max = 0;
  pos = 0;

  for ( j = a->number ; j-- ;  )
  {
    while ( a->values[ i ] == 0 )
    {
      ++i;
    }

    a->comp_reverse[ a->values[ i ] ] = ( U16 ) i;
    ++i;
  }

  for ( i = 1 ; i <= a->number ; i++ )
  {
    while ( a->counts[ i ] <= 1 )
    {
      if ( i < a->number )
      {
        a->counts[ i ] = a->counts[ a->number ];
        a->counts[ a->number ] = 0;

        assert( a->values[ a->comp_reverse[ i ] ] == i );

        a->values[ a->comp_reverse[ i ] ] = ( U16 ) 0;
        a->values[ a->comp_reverse[ a->number ] ] = ( U16 ) i;

        a->comp_reverse[ i ] = a->comp_reverse[ a->number ];

        --a->number;
      }
      else
      {
        a->counts[ i ] = 0;
        a->values[ a->comp_reverse[ i ] ] = ( U16 ) 0;
        --a->number;
        goto done;
      }
    }

    a->counts[ i ] >>= 1;

    if ( a->counts[ i ] > max )
    {
      max = a->counts[ i ];
      pos = i;
    }

    tots[ ( i < a->last_bin_start ) ? ( i >> a->bin_shift ) : ( BINS - 1 ) ] += a->counts[ i ];
  }

done:

  // place the new greatest value into the final bin position (minimize total increments)
  if ( max )
  {
    j = ( a->number < a->last_bin_start ) ? ( ( a->number >> a->bin_shift ) << a->bin_shift ) : a->last_bin_start;
    if ( j == 0 )
      ++j;

    if ( pos != j )
    {
      i = a->counts[ j ];
      a->counts[ j ] = a->counts[ pos ];
      tots[ ( j < a->last_bin_start ) ? ( j >> a->bin_shift ) : ( BINS - 1 ) ] += ( - ( unsigned short ) i + a->counts[ j ] );
      tots[ ( pos < a->last_bin_start ) ? ( pos >> a->bin_shift ) : ( BINS - 1 )  ] += ( unsigned short ) i - a->counts[ j ];
      a->counts[ pos ] = ( unsigned short ) i;
      i = a->values[ a->comp_reverse[ j ] ];
      a->values[ a->comp_reverse[ j ] ] = a->values[ a->comp_reverse[ pos ] ];
      a->values[ a->comp_reverse[ pos ] ] = ( unsigned short ) i;
    }
  }

  // mark temp as touched
  a->comp_reverse[ 0 ] = 0;

  finish_rescale( a, tots );
}


static void rescale_decompress( ARITH a )
{
  unsigned long i;
  U32 tots[ BINS ];

  U32 max;
  U32 pos;

  max = 0;
  pos = 0;

  calc_best_shift( a, ( a->number + 1 ) );

  memset( tots, 0, sizeof( tots ) );

  // rescale the escape count
  a->counts[ 0 ] >>= 1;
  tots[ ( 0 < a->last_bin_start ) ? 0 : ( BINS - 1 ) ] = a->counts[ 0 ];

  for ( i = 1 ; i <= a->number ; i++ )
  {
    while ( a->counts[ i ] <= 1 )
    {
      if ( i < a->number )
      {
        a->counts[ i ] = a->counts[ a->number ];
        a->counts[ a->number ] = 0;
        a->values[ i ] = a->values[ a->number ];
        --a->number;
      }
      else
      {
        a->counts[ i ] = 0;
        --a->number;
        goto done;
      }
    }

    a->counts[ i ] >>= 1;

    if ( a->counts[ i ] > max )
    {
      max = a->counts[ i ];
      pos = i;
    }

    tots[ ( i < a->last_bin_start ) ? ( i >> a->bin_shift ) : ( BINS - 1 ) ] += a->counts[ i ];
  }

 done:

  if ( max )
  {
    unsigned long j;

    j = ( a->number < a->last_bin_start ) ? ( ( a->number >> a->bin_shift ) << a->bin_shift ) : a->last_bin_start;
    if ( j == 0 )
      ++j;

    // place the new greatest value into the final bin position (minimize total increments)
    if ( pos != j )
    {
      i = a->counts[ j ];
      a->counts[ j ] = a->counts[ pos ];
      tots[ ( j < a->last_bin_start ) ? ( j >> a->bin_shift ) : ( BINS - 1 ) ] += ( - ( unsigned short ) i + a->counts[ j ] );
      tots[ ( pos < a->last_bin_start ) ? ( pos >> a->bin_shift ) : ( BINS - 1 )  ] += ( unsigned short ) i - a->counts[ j ];
      a->counts[ pos ] = ( unsigned short ) i;
      i = a->values[ j ];
      a->values[ j ] = a->values[ pos ];
      a->values[ pos ] = ( unsigned short ) i;
    }
  }

  finish_rescale( a, tots );
}


RADDEFFUNC U32 GRANNY Arith_compress( ARITH a, ARITHBITS* ab, U32 value )
{
  U32 pos;
  U32 start;

  // do we need to rescale?
  if ( a->totals[ BINS - 1 ] >= 16384 )
  {
    rescale_compress( a );
  }

  pos = a->values[ value ];

  start = get_symb_start_range_and_add( a, pos );

  ArithBitsPut( ab, start, a->counts[ pos ], a->totals[ BINS - 1 ] - 1 );

  ++a->counts[ pos ];

  // was it an escape character?
  if ( pos == 0 )
  {
    assert( a->number < a->unique_symb );

    // add new position
    ++a->number;

    assert( ( a->counts[ a->number ] == 0 ) && ( a->values[ value ] == 0 ) );

    // mark new position
    a->values[ value ] = ( U16 ) a->number;

    // increment the counts
    quick_increment_counts( a, a->number, 0x20002 );

    // have we seen all the symbols?  if so, remove the escape
    if ( a->number == a->unique_symb )
    {
      decrement_counts( a, 0, a->counts[ 0 ] );
    }

    return( 0x20000 );
  }

  return( value );
}


RADDEFFUNC UINTADDR GRANNY Arith_decompress( ARITH a, ARITHBITS* ab )
{
  U32 count;
  U32 pos;
  U32 start;

  // do we need to rescale?
  if ( a->totals[ BINS - 1 ] >= 16384 )
  {
    rescale_decompress( a );
  }

  // get the count position
  count = ArithBitsGet( ab, a->totals[ BINS - 1 ] );

  // get position and start and update symbol count
  pos = find_pos_from_count_and_add( a, count, &start );

  // remove the symbol we just found
  ArithBitsRemove( ab, start, a->counts[ pos ], a->totals[ BINS - 1 ] - 1 );

  ++a->counts[ pos ];

  // was it an escape character
  if ( pos == 0 )
  {
    assert( a->number < a->unique_symb );

    // add new position
    ++a->number;

    assert( a->counts[ a->number ] == 0 );

    // increment counts
    quick_increment_counts( a, a->number, 0x20002 );

    // have we seen all the symbols?  if so, remove the escape
    if ( a->number == a->unique_symb )
    {
      decrement_counts( a, 0, a->counts[ 0 ] );
    }

    return( ( UINTADDR ) &a->values[ a->number ] );
  }

  return( a->values[ pos ] );
}


RADDEFFUNC void GRANNY Arith_adjust_probability( ARITH a, U32 value, S32 count )
{
  U32 pos;

  pos = a->values[ value ];

  if ( pos )
  {
    if ( count >= 0 )
      quick_increment_counts( a, pos, (count << 16 ) | count );
    else
      decrement_counts( a, pos, -count );
  }
}

RADDEFFUNC F64 GRANNY Arith_estimated_compressed_size( ARITH a, U32 value, U32 scale )
{
  U32 pos;

  pos = a->values[ value ];

  if ( pos == 0 )
  {
    return( - radlog2( 1.0 / ( F64 ) scale ) );
  }
  else
  {
    return( - radlog2( ( ( F64 ) ( a->counts[ pos ] + 1 ) ) / ( F64 ) ( a->totals[ BINS - 1 ] + 1 ) ) );
  }
}

