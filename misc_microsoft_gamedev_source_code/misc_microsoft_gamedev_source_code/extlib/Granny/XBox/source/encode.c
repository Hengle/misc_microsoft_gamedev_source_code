#include "encode.h"
#include "varbits.h"
#include "radarith.h"
#include "radbase.h"
#include "raddebug.h"
#include "radmath.h"

#include "radmem.h"
#include "radmemutils.h"

#include <stdio.h>
#include <string.h>

// gets the bitlevel of a int
#define GET_BIT_LEVEL( val )  ( (U32)getbitlevel( val ) )

// literal vs. zero-run settings
#define MIN_ZERO_LENGTH 3
#define LIT_LENGTH_BITS 6
#define ZERO_LENGTH_BITS 8
#define LIT_LENGTH_LIMIT ( ( 1 << LIT_LENGTH_BITS ) - 1 )
#define ZERO_LENGTH_LIMIT ( ( 1 << ZERO_LENGTH_BITS ) - 1 )

#define EXTRA_LENGTHS 4
static U32 extra_lit_lengths[ EXTRA_LENGTHS ] = { 128, 256, 512, 1024 };
static U32 extra_zero_lengths[ EXTRA_LENGTHS ] = { 512, 1024, 2048, 3072 };




// few small debug macros
//#define DEBUG_PLANES
#define SEND_ARITH_MARKER(ari)  { ArithBitsPutValue(ari,43,77); }
#define CHECK_ARITH_MARKER(ari) { U32 v; v = ArithBitsGetValue(ari,77); radassert( v == 43 ); }
#define SEND_VAR_MARKER(vb)     { VarBitsPut(vb,0x8743,16); }
#define CHECK_VAR_MARKER(vb)    { U32 v; VarBitsGetLE(v, U32, vb,16); radassert( v == 0x8743 ); }


#if INCLUDE_ENCODERS

// get the largest absolute difference from predicted pixel to actual pixel
static S32 get_diff_predicted_max( S16 const* inp, S32 pixel_pitch, U32 enc_width, U32 enc_height, S32* max )
{
  U32 x, y;
  S32 yadj;
  S32 maxv = -32768;
  S32 prev, first, same;
  S16 const* from;

  same = 1;

  yadj = pixel_pitch - (S32)enc_width;

  first = *inp++;

  prev = first;
  for( x = ( enc_width - 1 ) ; x-- ; )
  {
    S32 cur = radabs( *inp - prev );
    if ( cur > maxv )
      maxv = cur;
    prev = *inp++;
    if ( first != prev )
      same = 0;
  }

  // do the rest of the pixels predicting from the left and upper pixel
  for( y = ( enc_height - 1 ) ; y-- ; )
  {
    S32 cur;

    inp += yadj;
    from = inp - pixel_pitch;

    // do the first pixel in the row (predict from the top)
    cur = radabs( *inp - *from );
    if ( cur > maxv )
      maxv = cur;
    prev = *inp++;
    if ( first != prev )
      same = 0;
    ++from;

    // do the rest of the row
    for( x = ( enc_width - 1 ) ; x-- ; )
    {
      cur = radabs( *inp - ( ( prev + *from ) / 2 ) );
      if ( cur > maxv )
        maxv = cur;
      prev=*inp++;
      if ( first != prev )
        same = 0;
      ++from;
    }
  }

  if ( max )
    *max = maxv;

  return( same );
}

#endif //#if INCLUDE_ENCODERS


/*
#include <stdio.h>
static void dump_plane( int plane, int num, S16 const * p, S32 pixel_pitch, U32 width, U32 height )
{
  FILE *f;
  U32 x, y;
  char fn[32];

  strcpy(fn,"pA_rA_vals");
  fn[1]=(char)( plane +'A' );
  fn[4]=(char)( num +'A' );

  f = fopen(fn, "wb+");

  for( y = 0 ; y < height ; y++ )
  {
    for( x = 0 ; x < width; x++ )
    {
      S32 pix;
      pix = radabs( p[ y * pixel_pitch + x ] );
      fwrite(&pix,2,1,f);
    }
  }
  fclose(f);
}
*/

#if INCLUDE_ENCODERS

// encode a low-pass plane (internal plane prediction using pixel to left and above)
static void encode_low( ARITHBITS* ab, VARBITS* vb, S16 const* inp, S32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp, U32 temp_size, U32 * decomp_min_size )
{
  U32 x, y;
  S32 yadj;
  S32 max, prev;
  U32 num;
  ARITH a;
  U32 comp_temp_size, comp_size;
  U8 * buf;
  S16 const * from;
  void * context_mem;

  if ( get_diff_predicted_max( inp, pixel_pitch, enc_width, enc_height, &max ) )
  {
    // All bytes are a single value
    VarBitsPuta1( *vb );
    VarBitsPut( *vb, inp[ 0 ], 16 );
    return;
  }

  num = max + 1;
  comp_temp_size = Arith_compress_temp_size( num );
  comp_size = Arith_compress_alloc_size( max, num );

  VarBitsPuta0( *vb );
  VarBitsPut( *vb, max, 16 );

  if ( ( temp == 0 ) || ( ( comp_temp_size + comp_size ) > temp_size ) )
  {
    // we don't have enough - alloc some temp memory
    context_mem = radmalloc( comp_temp_size + comp_size );
    buf = ( U8* ) context_mem;
  }
  else
  {
    // use want we've got
    context_mem = 0;
    buf = ( U8* ) temp;
  }

  a = Arith_open( buf, buf + comp_size, max, num );

  yadj = pixel_pitch - (S32)enc_width;


  // now calculate how much decompression will need
  comp_size = Arith_decompress_alloc_size( num );
  if ( comp_size > ( *decomp_min_size ) )
  {
    *decomp_min_size = comp_size;
  }

  prev = *inp++;

  // do the first byte with no prediction
  VarBitsPut( *vb, prev, 16 );

  // do the first row just predicting from the left pixel
  for( x = ( enc_width - 1 ) ; x-- ; )
  {
    U32 cur = *inp++ - prev;

    if ( Arith_was_escaped( Arith_compress( a, ab, radabs( cur ) ) ) )
    {
      ArithBitsPutValue( ab, radabs( cur ), num );
    }

    if ( cur )
    {
      VarBitsPut1( *vb, cur & 0x80000000 );
    }

    prev = cur + prev;
  }

  // do the rest of the pixels predicting from the left and upper pixel
  for( y = ( enc_height - 1 ) ; y-- ; )
  {
    U32 cur;

    inp += yadj;
    from = inp - pixel_pitch;

    // do the first pixel in the row (predict from the top)
    cur = *inp - *from;

    if ( Arith_was_escaped( Arith_compress( a, ab, radabs( cur ) ) ) )
    {
      ArithBitsPutValue( ab, radabs( cur ), num );
    }

    if ( cur )
    {
      VarBitsPut1( *vb, cur & 0x80000000 );
    }

    prev = *inp++;
    ++from;

    // do the rest of the row
    for( x = ( enc_width - 1 ) ; x-- ; )
    {
      cur = *inp - ( ( prev + *from ) / 2 );

      if ( Arith_was_escaped( Arith_compress( a, ab, radabs( cur ) ) ) )
      {
        ArithBitsPutValue( ab, radabs( cur ), num );
      }

      if ( cur )
      {
        VarBitsPut1( *vb, cur & 0x80000000 );
      }

      prev = *inp++;
      ++from;
    }
  }

  if ( context_mem )
  {
    radfree( context_mem );
  }
}


// get the largest absolute value in the plane
static S32 get_abs_max( S16 const* inp, S32 pixel_pitch, U32 enc_width, U32 enc_height, U32* max )
{
  U32 x, y;
  S32 yadj;
  S32 maxv = -32768;
  S32 first, same;

  same = 1;

  yadj = pixel_pitch - (S32)enc_width;

  first = *inp;

  for( y = enc_height ; y-- ; )
  {
    S32 cur;

    // do the row
    for( x = enc_width ; x-- ; )
    {
      cur = radabs( *inp );
      if ( cur > maxv )
        maxv = cur;
      if ( first != *inp )
        same = 0;
      ++inp;
    }

    inp += yadj;
  }

  if ( max )
    *max = maxv;

  return( same );
}


// allocates the contexts for the bits and the signs
static void * create_contexts( ARITH** a, ARITH * lits, ARITH* zeros, U32 max, U32 num, U32 numl, void * temp, U32 temp_size, U32 * decomp_min_size )
{
  U32 i;
  U32 comp_temp_size, comp_size, lit_comp_size, zero_comp_size;
  U8 * buf;
  U8* comp_temp;

  num += 0;
  max += 0;

  i = num;
  if ( i < ( LIT_LENGTH_LIMIT + 1 ) )
    i = LIT_LENGTH_LIMIT + 1;
  if ( i < ( ZERO_LENGTH_LIMIT + 1 ) )
    i = ZERO_LENGTH_LIMIT + 1;

  comp_temp_size = Arith_compress_temp_size( i );
  comp_size = Arith_compress_alloc_size( max, num );
  lit_comp_size = Arith_compress_alloc_size( LIT_LENGTH_LIMIT, LIT_LENGTH_LIMIT + 1 );
  zero_comp_size = Arith_compress_alloc_size( ZERO_LENGTH_LIMIT, ZERO_LENGTH_LIMIT + 1 );

  // calculate the total memory requirement
  i = ( ( sizeof(void*) * numl ) + comp_temp_size + ( numl * comp_size ) + lit_comp_size + zero_comp_size + 15 ) & ~15;

  if ( ( temp == 0 ) || ( i > temp_size ) )
  {
    // we don't have enough, so do a malloc
    temp = 0;
    buf = ( U8* ) radmalloc( i );
  }
  else
  {
    // we've got enough, so just use temp...
    buf = (U8*) temp;
  }

  *a = (ARITH *) buf;

  buf += ( sizeof(void*) * numl );

  comp_temp = buf;
  buf += comp_temp_size;

  // initialize each of the contexts
  for( i = 0 ; i < numl ; i++ )
  {
    (*a)[ i ] = Arith_open( buf, comp_temp, max, num );
    buf += comp_size;
  }

  // do the runs of lit and runs of zero contexts
  *lits = Arith_open( buf, comp_temp, LIT_LENGTH_LIMIT, LIT_LENGTH_LIMIT + 1 );
  buf += lit_comp_size;

  *zeros = Arith_open( buf, comp_temp, ZERO_LENGTH_LIMIT, ZERO_LENGTH_LIMIT +1 );
  buf += zero_comp_size;


  // calcuate the size for decompression

  comp_size = Arith_decompress_alloc_size( num );
  lit_comp_size = Arith_decompress_alloc_size( LIT_LENGTH_LIMIT + 1 );
  zero_comp_size = Arith_decompress_alloc_size( ZERO_LENGTH_LIMIT + 1 );

  // Note that this /has/ to be 8, rather than sizeof(void*), since we're computing
  // the size that will be allocated on decode.  Since we could be writing this on
  // 32 bit, and loading on 64, we need to make sure we have room.
  i = ( ( 8 * numl ) + ( numl * comp_size ) + lit_comp_size + zero_comp_size + 15 ) & ~15;

  if ( i > *decomp_min_size )
  {
    *decomp_min_size = i;
  }

  return( ( temp == 0 ) ? ( *a ) : 0 );
}


// count the current run of zeros (can extend beyond the scanline)
static U32 zero_run_count_plane( S16 const* scan, U32 w, U32 h, S32 pixel_pitch, U32 enc_width, S16 const** new_scan, U32 * new_w, U32 * new_h)
{
  U32 tot;
  S32 adj;

  tot = 0;
  adj = ( pixel_pitch - (S32)enc_width ) * 2;

  do
  {
    do
    {
      // found a non-zero?
      if ( *scan )
      {
        goto done;
      }

      ++tot;
      --w;
      ++scan;
    } while ( w );

    --h;
    scan = ( S16 const* ) ( ( ( U8 const* ) scan ) + adj );
    w = enc_width;

  } while ( h );
  w = 0;

 done:
  *new_scan = scan;
  *new_w = w;
  *new_h = h;
  return( tot );
}



// gets the next literal length and the next zero length
static void get_lengths( S16 const* inp, S32 pixel_pitch, U32 w, U32 h, U32 enc_width, U32 * lit_full_len, U32 * zero_full_len )
{
  S16 const* scan = inp;
  U32 lits = 0;
  S32 adj;

  adj = ( pixel_pitch - (S32)enc_width ) * 2;

  for(;;)
  {
    do
    {
      U32 new_w, new_h;
      S16 const* new_scan;
      U32 zero_count = zero_run_count_plane( scan, w, h, pixel_pitch, enc_width, &new_scan, &new_w, &new_h );

      // need a run of MIN_ZERO_LENGTH to be a speed savings
      if ( zero_count <= MIN_ZERO_LENGTH )
      {
        if ( new_w )
        {
          lits += zero_count + 1;
          ++new_scan;
          --new_w;
        }
        else
        {
          lits += zero_count;
        }
      }
      else
      {
        *lit_full_len = lits;
        *zero_full_len = zero_count;
        return;
      }

      scan = new_scan;
      w = new_w;
      h = new_h;
    } while ( w );

    // are we done?
    if (h <= 1 )
      break;

    w = enc_width;
    --h;

    scan = ( S16 const* ) ( ( ( U8 const* ) scan ) + adj );
  }

  *lit_full_len = lits;
  *zero_full_len = 0;
}

#endif //#if INCLUDE_ENCODERS



// limit the lengths to the actual length or one of the length escapes
U32 limit_length( U32 len, U32 limit, U32* bits, U32* extras )
{
  #if EXTRA_LENGTHS!=4
    #error Need to update this routine.
  #endif

  if ( len >= extras[ 2 ] )
  {
    if ( len >= extras[ 3 ] )
    {
      *bits = limit - EXTRA_LENGTHS + 1 + 3;
      return( extras[ 3 ] );
    }
    else
    {
      *bits = limit - EXTRA_LENGTHS + 1 + 2;
      return( extras[ 2 ] );
    }
  }
  else
  {
    if ( len >= extras[ 0 ] )
    {
      if ( len >= extras[ 1 ] )
      {
        *bits = limit - EXTRA_LENGTHS + 1 + 1;
        return( extras[ 1 ] );
      }
      else
      {
        *bits = limit - EXTRA_LENGTHS + 1;
        return( extras[ 0 ] );
      }
    }
    else
    {
      if ( len > limit - EXTRA_LENGTHS )
      {
        *bits = limit - EXTRA_LENGTHS;
        return( limit - EXTRA_LENGTHS );
      }
    }
  }

  *bits = len;
  return( len );
}


// read the string of escapes
static void read_escapes( ARITHBITS* ab, U8* mask, U32 count )
{
  U32 i;
  U32 zeros;

  zeros = ArithBitsGetValue ( ab, count + 1 );

  radassert( zeros <= count );

  for( i = count ; i-- ; )
  {
    if ( ArithBitsGet( ab, count ) >= zeros )
    {
      *mask++ = 1;
      ArithBitsRemove( ab, zeros, count - zeros, count );
    }
    else
    {
      *mask++ = 0;
      ArithBitsRemove( ab, 0, zeros, count );
    }
  }
}



#if INCLUDE_ENCODERS



// encode a high-pass plane (internal plane prediction with order 1)
static void encode_high_1( ARITHBITS* ab, VARBITS* vb, S16 const* inp, S32 pixel_pitch, U32 enc_width, U32 enc_height, U32 qlevel, void * temp, U32 temp_size, U32 * decomp_min_size )
{
  U32 w, h;
  S32 yadj;
  U32 max;
  U32 num, numl;
  ARITH* a;
  ARITH lits, zeros;
  S16 const* from;
  S32 prev, above, above_left;
  U32 lit_len, zero_len;
  U32 lit_full_len, zero_full_len;
  void * context_mem;

  #ifdef _DEBUG
  S16 const* in;
  #endif

  #ifdef DEBUG_PLANES
  SEND_ARITH_MARKER( ab );
  SEND_VAR_MARKER( *vb );
  #endif

  // dump the quantization level
  VarBitsPut( *vb, qlevel, 16 );

  if ( get_abs_max( inp, pixel_pitch, enc_width, enc_height, &max ) )
  {
    // All bytes are a single value?
    VarBitsPuta1( *vb );
    VarBitsPut( *vb, inp[ 0 ], 16 );
    return;
  }

  num = max + 1;
  numl = max * qlevel;
  numl = GET_BIT_LEVEL( numl ) + 1;

  VarBitsPuta0( *vb );
  VarBitsPut( *vb, max, 16 );

  // create the arith encoders
  context_mem = create_contexts( &a, &lits, &zeros, max, num, numl, temp, temp_size, decomp_min_size );

  yadj = pixel_pitch - (S32)enc_width;
  h = enc_height;

  // send the first pixel plain
  above = *inp;
  ArithBitsPutValue( ab, radabs( above ), num );
  if ( above )
  {
    VarBitsPut1( *vb, above & 0x80000000 );
  }
  
  above_left = above;
  prev = above;

  from = inp;
  #ifdef _DEBUG
  in = inp + pixel_pitch;
  #endif
  ++inp;

  lit_len = 0;
  zero_len = 0;
  lit_full_len = 0;
  zero_full_len = 0;

  if ( enc_width == 1 )
    goto after_first;

  w = enc_width - 1;

  for(;;)
  {
    // do we another set of lengths?
    U32 v;

    // are our reserve lengths empty?
    if ( ( lit_full_len | zero_full_len ) == 0 )
    {
      // get the next reserve lengths
      get_lengths( inp, pixel_pitch, w, h, enc_width, &lit_full_len, &zero_full_len );
    }

    // limit the literal lengths
    lit_len = limit_length( lit_full_len, LIT_LENGTH_LIMIT, &v, extra_lit_lengths );
    lit_full_len -= lit_len;

    // encode the limited literal length
    if ( Arith_was_escaped( Arith_compress( lits, ab, v ) ) )
    {
      VarBitsPut( *vb, v, LIT_LENGTH_BITS );
    }

    // is there no zero reserved length or do we still have literal data left over?
    if ( ( zero_full_len == 0 ) || ( lit_full_len ) )
    {
      // don't encode a run at all.
      zero_len = 0;
      v = 0;
    }
    else
    {
      // limit the zero length
      zero_len = ( MIN_ZERO_LENGTH - 1 ) + limit_length( zero_full_len - ( MIN_ZERO_LENGTH - 1 ), ZERO_LENGTH_LIMIT, &v, extra_zero_lengths );
    }
    zero_full_len -= zero_len;

    // if the remaining zero run is smaller than the minimum, encode it as a literal next time
    if ( zero_full_len < MIN_ZERO_LENGTH )
      zero_full_len = 0;

    // encode the limited zero length
    if ( Arith_was_escaped( Arith_compress( zeros, ab, v ) ) )
    {
      VarBitsPut( *vb, v, ZERO_LENGTH_BITS );
    }

    // do we have any literals to do?
    while ( lit_len )
    {
      S32 cur, context;

      // are we at or past the last pixel?
      if ( w <= 1 )
      {
        // if we have a pixel left, do it
        if ( w )
        {
          cur = *inp++;

          context = ( ( ( U32 ) radabs( prev * 2 ) + radabs ( above_left ) + radabs( above ) ) * qlevel ) / 4;
          context = GET_BIT_LEVEL( context );

          // encode the absolute value
          if ( Arith_was_escaped( Arith_compress( a[ context ], ab, radabs( cur ) ) ) )
          {
            ArithBitsPutValue( ab, radabs( cur ), num );
          }

          // encode the sign
          if ( cur )
          {
            VarBitsPut1( *vb, cur & 0x80000000 );
          }

          --lit_len;
        }

        // wrap the scanline
       after_first:
        if ( ( --h ) == 0 )
          goto done;

        w = enc_width;
        inp += yadj;

        #ifdef _DEBUG
        radassert( inp == in );
        in += pixel_pitch;
        #endif

        from = inp - pixel_pitch;
        above = *from++;
        above_left = above;
        prev = above;
      }
      else
      {
        S32 above_right = *from;
        cur = *inp;

        context  = ( ( ( U32 ) radabs( prev ) + radabs ( above_left ) + radabs( above ) + radabs( above_right ) ) * qlevel ) / 4;
        context  = GET_BIT_LEVEL( context );

        // encode the literal
        if ( Arith_was_escaped( Arith_compress( a[ context ], ab, radabs( cur ) ) ) )
        {
          ArithBitsPutValue( ab, radabs( cur ), num);
        }

        // encode the sign
        if ( cur )
        {
          VarBitsPut1( *vb, cur & 0x80000000 );
        }

        above_left = above;
        above = above_right;
        prev = cur;

        ++inp;
        ++from;
        --w;
        --lit_len;
      }
    }

    // do the zero run
    while ( zero_len )
    {
      // will we hit the end of the scan line?
      if ( zero_len >= w )
      {
        if ( ( --h ) == 0 )
          goto done;

        zero_len -= w;
        inp += w;
        from += w;

        w = enc_width;
        inp += yadj;

        #ifdef _DEBUG
        radassert( inp == in );
        in += pixel_pitch;
        #endif

        from = inp - pixel_pitch;
        above = *from++;
        above_left = above;
        prev = above;
      }
      else
      {
        w -= zero_len;
        inp += zero_len;
        from += zero_len;
        zero_len = 0;

        prev = 0;
        above = from[ -1 ];
        above_left = from[ -2 ];
      }
    }
  }

 done:
  if ( context_mem )
  {
    radfree( context_mem );
  }

}


// quantize a plane by the specified level
static void quantize( S16 * outp, S16 const * inp, S32 pixel_pitch, U32 q_width, U32 q_height, U32 level )
{
  U32 h;
  S32 yadj;

  if ( level < 1 )
    level = 1;
//    return;

  yadj = pixel_pitch - (S32)q_width;

  for( h = q_height ; h-- ; )
  {
    U32 w;
    for( w = q_width ; w-- ; )
    {
      *outp++ = ( S16 ) ( ( *inp++ / ( S32 ) level ) );
    }

    inp += yadj;
  }
}


// dump one arithmetic-encoded bit for each row
static void dump_row_escapes( ARITHBITS * ab, S16 const* inp, S32 pitch, U32 width, U32 height, void * temp, U32 temp_size )
{
  U32 h;
  U8 * mask, * mask_start;
  U32 zeros = 0;

  if ( ( temp == 0 ) || ( height > temp_size ) )
  {
    mask_start = (U8*) radmalloc ( height );
    temp = 0;
  }
  else
  {
    mask_start = ( U8* ) temp;
  }

  mask = mask_start;

  for( h = height ; h-- ; )
  {
    U32 w;
    S16 const* i = inp;

    for( w = width ; w-- ; )
    {
      if ( *i )
      {
        *mask++ = 1;
        goto non_zero;
      }
      ++i;
    }

    *mask++ = 0;
    ++zeros;

   non_zero:
    inp = ( S16* ) ( ( ( U8* ) inp ) + pitch );
  }

  ArithBitsPutValue ( ab, zeros, height + 1 );

  mask = ( U8* ) mask_start;
  for( h = height ; h-- ; )
  {
    if ( *mask++ )
      ArithBitsPut( ab, zeros, height - zeros, height );
    else
      ArithBitsPut( ab, 0, zeros, height );
  }

  if ( temp == 0 )
  {
    radfree( mask_start );
  }
}

/*  Not currently used...
// dump one arithmetic-encoded bit for each row
static void dump_col_escapes( ARITHBITS * ab, S16 const* inp, S32 pitch, U32 width, U32 height, void* temp )
{
  U32 w;
  U8* mask = ( U8* ) temp;
  U32 zeros = 0;

  width = width / 4;

  for( w = width ; w-- ; )
  {
    U32 h;
    S16 const* i = inp;

    for( h = height ; h-- ; )
    {
      if ( i[ 0 ] | i[ 1 ] | i[ 2 ] | i[ 3 ] )
      {
        *mask++ = 1;
        goto non_zero;
      }
      i = ( S16* ) ( ( ( U8* ) i ) + pitch );
    }

    *mask++ = 0;
    ++zeros;

   non_zero:
    inp += 4;
  }

  ArithBitsPutValue ( ab, zeros, width + 1 );

  mask = ( U8* ) temp;
  for( w = width ; w-- ; )
  {
    if ( *mask++ )
      ArithBitsPut( ab, zeros, width - zeros, width );
    else
      ArithBitsPut( ab, 0, zeros, width );
  }
}
*/




typedef struct subplane
{
  F64 cur_size;
  F64 next_size;
  U32 cur_error;
  U32 next_error;
  U32 cur_quant;
  U32 next_quant;
  S16 const * addr;
  S32 pitch;
  U32 width;
  U32 height;
  U32 max;
  U32 same;
  U32* histogram;
} subplane;

typedef subplane subplanes[ 13 ];


// makes a histogram from a plane
static void build_histogram( S16 const * inp, S32 pixel_pitch, U32 width, U32 height, U32 max, U32 * histogram )
{
  U32 h;
  S32 yadj;

  radmemset( histogram, 0, 4 * ( max + 1 ) );

  yadj = pixel_pitch - (S32)width;

  for( h = height ; h-- ; )
  {
    U32 w;
    for( w = width ; w-- ; )
    {
      ++histogram[ radabs( *inp ) ];
      ++inp;
    }

    inp += yadj;
  }
}



// calculates the error and size of a histogram once quantized...
static void calc_size_and_error( U32 const * histogram, U32 max, U32 total, U32 quant, F64 * out_size, U32 * out_error )
{
  U32 i = 0;
  U32 postq = quant;
  U32 preq = 0;
  F64 size = 0;
  U32 error = 0;

  while ( i <= max )
  {
    U32 qn = 0;

    while ( ( i <= max ) && ( i < postq ) )
    {
      qn += histogram[ i ];
      error += ( ( i - preq ) * histogram[ i ] );
      ++i;
    }

    if ( qn )
    {
      size += ( qn * radlog2( ( ( F64 ) total ) / ( ( F64 ) qn ) ) );
      if ( i >= ( quant + 1 ) )
      {
        // add the sign bits
        size += qn;
      }
    }
    preq += quant;
    postq += quant;
  }

  *out_size = size;
  *out_error = error;
}


// moves the subplane to the next quant level
static void get_next_level( subplane * sp )
{
  if ( sp->cur_quant <= sp->max )
  {
    U32 tryq = sp->cur_quant;

    do
    {
      ++tryq;

      calc_size_and_error(  sp->histogram,
                            sp->max,
                            sp->width * sp->height,
                            tryq,
                           &sp->next_size,
                           &sp->next_error );

      if ( ( sp->next_size <= sp->cur_size ) &&
           ( sp->next_error <= sp->cur_error ) )
      {
        sp->cur_size = sp->next_size;
        sp->cur_error = sp->next_error;
        sp->cur_quant = tryq;
        if ( sp->cur_quant <= sp->max )
          continue;
      }

    } while ( ( ( tryq ) <= sp->max ) &&
              ( sp->next_size >= sp->cur_size ) );

    sp->next_quant = tryq;
  }
}


// encode the decomposed planes
U32 planes_encode( void* output,
                   U32 tobytes,
                   S16 const * const * input,
                   F32 const * plane_weights,
                   U32 planes,
                   U32 width,
                   U32 height,
                   void * temp, U32 temp_size,
                   U32 * decomp_min_size )
{
  U32 i, s;
  void * out;
  void * uncomp;
  S16 * quant_plane;
  F64 cur_size;
  subplanes * sps;
  void * temp1 = 0;
  void * temp2 = 0;
  U32 size;
  F64 tobits;
  U32 tweaked_times = 0;
  F64 best_bits;
  U32 best_diff = (U32)-1;

  // account for hard bit cost
  tobytes += ( ENCODE_MIN_PLANE_SIZE * planes );

  // convert input size request to bits
  tobits = tobytes * 8;
  best_bits = tobits;

  // get the subplanes size
  i = ( ( sizeof( subplanes ) * planes ) + 15 ) & ~15;

  // get the size for the uncompressed portion of the frame
  s = ( ( ( width * height / 8 ) + 128 + 15 ) & ~15 );

  // get the size for the largest quantized plane
  size = ( width * height * 2 ) / 4;

  // do we have enough temp memory for these two chunks
  if ( ( temp == 0 ) || ( ( i + s + size ) > temp_size ) )
  {
    temp1 = radmalloc( i + s + size );
    uncomp = temp1;
    sps = ( subplanes* ) ( ( ( U8* ) temp1 ) + s );
    quant_plane = ( S16* ) ( ( ( U8* ) sps ) + i );
  }
  else
  {
    uncomp = temp;
    sps = ( subplanes* ) ( ( ( U8* ) temp ) + s );
    quant_plane = ( S16* ) ( ( ( U8* ) sps ) + i );
    temp = ( ( ( U8* ) temp ) + i + s + size );
    temp_size -= ( i + s + size );
  }

  // setup the subplane array
  for( i = 0 ; i < planes ; i++ )
  {
    sps[ i ][  0 ].addr   = input[ i ];
    sps[ i ][  1 ].addr   = input[ i ] + ( width / 16 );
    sps[ i ][  3 ].pitch  = sps[ i ][ 2 ].pitch  = sps[ i ][ 1 ].pitch  = sps[ i ][ 0 ].pitch  = width * 16;
    sps[ i ][  3 ].width  = sps[ i ][ 2 ].width  = sps[ i ][ 1 ].width  = sps[ i ][ 0 ].width  = width / 16;
    sps[ i ][  3 ].height = sps[ i ][ 2 ].height = sps[ i ][ 1 ].height = sps[ i ][ 0 ].height = height / 16;
    sps[ i ][  2 ].addr   = input[ i ] + ( width * 8 );
    sps[ i ][  3 ].addr   = input[ i ] + ( width / 16 ) + ( width * 8 );

    sps[ i ][  4 ].addr   = input[ i ] + ( width / 8 );
    sps[ i ][  6 ].pitch  = sps[ i ][ 5 ].pitch  = sps[ i ][ 4 ].pitch  = width * 8;
    sps[ i ][  6 ].width  = sps[ i ][ 5 ].width  = sps[ i ][ 4 ].width  = width / 8;
    sps[ i ][  6 ].height = sps[ i ][ 5 ].height = sps[ i ][ 4 ].height = height / 8;
    sps[ i ][  5 ].addr   = input[ i ] + ( width * 4 );
    sps[ i ][  6 ].addr   = input[ i ] + ( width / 8 ) + ( width * 4 );

    sps[ i ][  7 ].addr   = input[ i ] + ( width / 4 );
    sps[ i ][  9 ].pitch  = sps[ i ][ 8 ].pitch  = sps[ i ][ 7 ].pitch  = width * 4;
    sps[ i ][  9 ].width  = sps[ i ][ 8 ].width  = sps[ i ][ 7 ].width  = width / 4;
    sps[ i ][  9 ].height = sps[ i ][ 8 ].height = sps[ i ][ 7 ].height = height / 4;
    sps[ i ][  8 ].addr   = input[ i ] + ( width * 2 );
    sps[ i ][  9 ].addr   = input[ i ] + ( width / 4 ) + ( width * 2 );

    sps[ i ][ 10 ].addr   = input[ i ] + ( width / 2 );
    sps[ i ][ 12 ].pitch  = sps[ i ][ 11 ].pitch  = sps[ i ][ 10 ].pitch  = width * 2;
    sps[ i ][ 12 ].width  = sps[ i ][ 11 ].width  = sps[ i ][ 10 ].width  = width / 2;
    sps[ i ][ 12 ].height = sps[ i ][ 11 ].height = sps[ i ][ 10 ].height = height / 2;
    sps[ i ][ 11 ].addr   = input[ i ] + width;
    sps[ i ][ 12 ].addr   = input[ i ] + ( width / 2 ) + width;

    // calculate the max value for each plane
    for( s = 0 ; s < 13 ; s++ )
    {
      sps[ i ][ s ].same = get_abs_max(  sps[ i ][ s ].addr,
                                         sps[ i ][ s ].pitch,
                                         sps[ i ][ s ].width,
                                         sps[ i ][ s ].height,
                                        &sps[ i ][ s ].max );
    }
  }

  // find out how much temp space the histograms will use
  {
    U32 t = 0;
    for( i = 0 ; i < planes ; i++ )
    {
      for( s = 0 ; s < 13 ; s++ )
      {
        t += ( sps[ i ][ s ].max + 1 ) * 4;
      }
    }

    if ( t > temp_size )
    {
      temp2 = radmalloc( t );
      temp = temp2;
      temp_size = t;
    }
  }

again:

  // setup up the histogram pointers and fill up
  {
    U8 * buf = ( U8* ) temp;
    for( i = 0 ; i < planes ; i++ )
    {
      for( s = 0 ; s < 13 ; s++ )
      {
        U32 size = ( sps[ i ][ s ]. max + 1 ) * 4;
        sps[ i ][ s ].histogram = ( U32* ) buf;
        build_histogram( sps[ i ][ s ].addr,
                         sps[ i ][ s ].pitch,
                         sps[ i ][ s ].width,
                         sps[ i ][ s ].height,
                         sps[ i ][ s ].max,
                         sps[ i ][ s ].histogram );
        buf += size;
      }
    }
  }

  // get the starting sizes of all the planes
  cur_size = 0;
  for( i = 0 ; i < planes ; i++ )
  {
    for( s = 0 ; s < 13 ; s++ )
    {
      if ( !sps[ i ][ s ].same )
      {
        sps[ i ][ s ].cur_quant = 1;

        // get the initial size
        calc_size_and_error(  sps[ i ][ s ].histogram,
                              sps[ i ][ s ].max,
                              sps[ i ][ s ].width * sps[ i ][ s ].height,
                              1,
                             &sps[ i ][ s ].cur_size,
                             &sps[ i ][ s ].cur_error );

        cur_size += sps[ i ][ s ].cur_size;

        // get the next level
        get_next_level( &sps[ i ][ s ] );
      }
    }
  }

  // if the lossless size is less than the requested size, then don't bother tweaking the rate (it ain't gonna get bigger)
  if ( cur_size <= tobits )
  {
    tweaked_times = 100;
  }

  // search for the right quant levels
  while ( cur_size > tobits )
  {
    S32 best_sub = 0;
    S32 best_plane = -1;
    F64 best_ratio = 0.0F;

    // find the best subplane to quantize
    for( i = 0 ; i < planes ; i++ )
    {
      for( s = 1 ; s < 13 ; s++ ) // don't quantize the first plane
      {
        if ( ( sps[ i ][ s ].cur_quant <= sps[ i ][ s ].max ) && ( !sps[ i ][ s ].same ) )
        {
          // get the bang for the bit
          F64 ratio = ( sps[ i ][ s ].cur_size - sps[ i ][ s ].next_size ) / ( ( ( F64 ) ( sps[ i ][ s ].next_error - sps[ i ][ s ].cur_error ) ) * plane_weights[ i ] );

          if ( ratio > best_ratio )
          {
            best_ratio = ratio;
            best_plane = i;
            best_sub = s;
          }
        }
      }
    }

    if ( best_plane == -1 )
    {
      // no more planes to quantize, we're stuck with the total size we're currently at
      break;
    }

    // upgrade the quantization level for the found subplane
    sps[ best_plane ][best_sub].cur_quant = sps[ best_plane ][best_sub].next_quant;
    cur_size = cur_size - sps[ best_plane ][best_sub].cur_size + sps[ best_plane ][best_sub].next_size;
    sps[ best_plane ][best_sub].cur_size = sps[ best_plane ][best_sub].next_size;
    sps[ best_plane ][best_sub].cur_error = sps[ best_plane ][best_sub].next_error;

    // get the next level
    get_next_level( &sps[ best_plane ][ best_sub ] );
  }

  // set the output size to zero
  out = output;
  size = 0;

  // encode everything
  for( i = 0 ; i < planes ; i++ )
  {
    VARBITS vb;
    ARITHBITS ab;

    U32 * sizes = ( U32* ) out;

    // leave two dwords at the start
    out = ( ( U8* ) out ) + 8;

    // open the probablity bits dumper
    ArithBitsPutStart( &ab, out );

    // open the uncompressed dumper
    VarBitsOpen( vb, uncomp );

    // do the lowest low pass first
    encode_low( &ab, &vb,
                input[ i ],
                width * 16,
                width / 16,
                height / 16,
                temp, temp_size,
                decomp_min_size );

    //dump_plane( i, 0, input[ i ], width * 16, width / 16, height / 16 );

    for( s = 1 ; s < 13 ; s++ )
    {
      if ( ( sps[ i ][ s ].same ) || ( sps[ i ][ s ].cur_quant <= 1 ) )
      {
        //dump_plane( i, s, sps[ i ][ s ].addr, sps[i][s].pitch, sps[i][s].width, sps[i][s].height );

        encode_high_1( &ab, &vb,
                        sps[ i ][ s ].addr,
                        sps[ i ][ s ].pitch,
                        sps[ i ][ s ].width,
                        sps[ i ][ s ].height,
                        1,
                        temp, temp_size,
                        decomp_min_size );
      }
      else
      {
        // quantize the plane with our discovered level
        quantize( quant_plane,
                  sps[ i ][ s ].addr,
                  sps[ i ][ s ].pitch,
                  sps[ i ][ s ].width,
                  sps[ i ][ s ].height,
                  sps[ i ][ s ].cur_quant );

        //dump_plane( i, s, quant_plane, sps[i][s].width, sps[i][s].width, sps[i][s].height );

        encode_high_1( &ab, &vb,
                        quant_plane,
                        sps[ i ][ s ].width,
                        sps[ i ][ s ].width,
                        sps[ i ][ s ].height,
                        sps[ i ][ s ].cur_quant,
                        temp, temp_size,
                        decomp_min_size );
      }
    }

    // dump the quick row escapes
    dump_row_escapes( &ab, input[ i ] + ( width / 2 ), width * 2, width / 2, height, temp, temp_size );

    // currently, we don't bother doing columns because it doesn't save much time
    // dump_col_escapes( &ab, input[ i ] + ( width * height / 2 ), width * 2, width, height / 2, temp, temp_size );

    ArithBitsFlush( &ab );
    VarBitsFlush( vb );

    // store the size of the arithmetic data
    sizes[ 0 ] = ( ( ArithBitsSize( &ab ) / 8 ) + 3 ) & ~3;
    out = ( ( U8* ) out ) + sizes[ 0 ];

    // store the size of the full bit data
    sizes[ 1 ] = ( ( VarBitsSize( vb ) / 8 ) + 3 ) & ~3;

    radassert( sizes[ 1 ] <= ( ( width * height / 8 ) + 128 ) );

    // move the full bit data onto the end of the arithmetic data
    radmemcpy( out, uncomp, sizes[ 1 ] );
    out = ( ( U8* ) out ) + sizes[ 1 ];

    size += ( sizes[ 0 ] + sizes[ 1 ] + 8 );
  }

  // if we haven't tweaked the rate twice yet, check it
  if ( ++tweaked_times <= 2 )
  {
    U32 diff = radabs( ( S32 ) size - ( S32 ) tobytes );

    if ( diff < best_diff )
    {
      best_diff = diff;
      best_bits = tobits;
      tweaked_times = 0;
    }

    // if the output rate is more than 2% off the requested rate, try again
    if ( ( ( diff * 100 ) / tobytes ) >= 2 )
    {
      tobits = 8.0 * ( ( ( F64 ) tobytes * ( F64 ) tobits ) / ( ( F64 ) size * 8 ) );
      if ( ( tobits / 8.0 ) > ( ( tobytes * 5 ) / 2 ) )
        tobits = ( tobytes * 5 ) * 4;  // 5 / 2 in bits
      goto again;
    }
  }

  // do we have the best match?  if not, run it one more time...
  if ( tobits != best_bits )
  {
    tobits = best_bits;
    goto again;
  }

  if ( temp1 )
    radfree( temp1 );

  if ( temp2 )
    radfree( temp2 );

  return( size );
}

#endif //#if INCLUDE_ENCODERS


// fills a rect with a particular value
static void fill_rect( S16 * outp, S32 pixel_pitch, U32 width, U32 height, S32 val )
{
  U32 h;
  S32 yadj;

  yadj = pixel_pitch - (S32)width;

  for( h = height ; h-- ; )
  {
    U32 w;

    for( w = width ; w-- ; )
    {
      *outp++ = (S16) val;
    }

    outp += yadj;
  }
}


//=========================================================================

// decode a low-pass plane (no prediction from other-planes)
static void decode_low( ARITHBITS* ab, VARBITS* vb, S16 * outp, S32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp )
{
  U32 w, h;
  S32 yadj;
  S32 max, prev;
  U32 num;
  ARITH a;
  U8 * buf;
  S16 * from;

  // See if all bytes are a single value
  if ( VarBitsGet1LE( *vb, prev ) )  // prev used as temporary
  {
    VarBitsGetLE( prev, S32, *vb, 16 );
    fill_rect( outp, pixel_pitch, enc_width, enc_height, prev );
    return;
  }

  VarBitsGetLE( max, S32, *vb, 16 );

  num = max + 1;

  buf = (U8*) temp;

  a = Arith_open( buf, 0, max, num );

  yadj = pixel_pitch - (S32)enc_width;

  // get the first pixel with no prediction
  VarBitsGetLE( prev, U32, *vb, 16 );

  *outp++ = (S16) prev;

  // do the first row just predicting from the left pixel
  for( w = ( enc_width - 1 ) ; w-- ; )
  {
    UINTADDR cur = Arith_decompress( a, ab );

    if ( Arith_was_escaped( cur ) )
    {
      U32 escaped;

      escaped = ArithBitsGetValue( ab, num );
      Arith_set_decompressed_symbol( cur, escaped );

      cur = escaped;
    }

    if ( cur )
    {
      U32 tmp;
      S32 v = -( S32 ) VarBitsGet1LE( *vb, tmp );
      cur = (cur ^ v ) - v;
    }

    // always a valid cast because of the Arith_was_escaped branch
    prev = (S32)cur + prev;
    *outp++ = (S16) prev;
  }

  // do the rest of the pixels predicting from the left and upper pixel
  for( h = ( enc_height - 1 ) ; h-- ; )
  {
    UINTADDR cur;

    outp += yadj;
    from = outp - pixel_pitch;

    // do the first pixel in the row (predict from the top)
    cur = Arith_decompress( a, ab );
    if ( Arith_was_escaped( cur ) )
    {
      U32 escaped;

      escaped = ArithBitsGetValue( ab, num );
      Arith_set_decompressed_symbol( cur, escaped );

      cur = escaped;
    }
    if ( cur )
    {
      S32 v = -( S32 ) VarBitsGet1LE( *vb, w ); // w used as temp
      cur = (cur ^ v ) - v;
    }

    // TODO: Fix once xenon compile fix is in place
#ifdef __RADXENON__
    {
      volatile S32 temp = *from;
      prev = (S32)cur + temp;
    }
#else
    prev = (S32)cur + *from;
#endif
    *outp++ = (S16) prev;
    ++from;

    // do the rest of the row
    for( w = ( enc_width - 1 ) ; w-- ; )
    {
      cur = Arith_decompress( a, ab );

      if ( Arith_was_escaped( cur ) )
      {
        U32 escaped;

        escaped = ArithBitsGetValue( ab, num );
        Arith_set_decompressed_symbol( cur, escaped );

        cur = escaped;
      }

      if ( cur )
      {
        U32 tmp;
        S32 v = -( S32 ) VarBitsGet1LE( *vb, tmp );
        cur = (cur ^ v ) - v;
      }

    // TODO: Fix once xenon compile fix is in place
#ifdef __RADXENON__
      {
        volatile S32 temp = *from;
        prev = (S32)cur + ( ( prev + temp ) / 2 );
      }
#else
      prev = (S32)cur + ( ( prev + *from ) / 2 );
#endif
      *outp++ = (S16) prev;
      ++from;
    }
  }
}


// allocates the contexts for the bits and the signs
static void * create_decomp_contexts( ARITH** a, ARITH * lits, ARITH * zeros, U32 max, U32 num, U32 numl, void * temp )
{
  U32 i;
  U32 comp_size, lit_comp_size, zero_comp_size;
  U8 * buf;

  comp_size = Arith_decompress_alloc_size( num );
  lit_comp_size = Arith_decompress_alloc_size( LIT_LENGTH_LIMIT + 1 );
  zero_comp_size = Arith_decompress_alloc_size( ZERO_LENGTH_LIMIT + 1 );

  buf = (U8*) temp;

  *a = (ARITH *) buf;

  buf += ( 4 * numl );

  // initialize each of the contexts
  for( i = 0 ; i < numl ; i++ )
  {
    (*a)[ i ] = Arith_open( buf, 0, max, num );
    buf += comp_size;
  }

  // init the lit run and zero run contexts
  *lits = Arith_open( buf, 0, LIT_LENGTH_LIMIT, LIT_LENGTH_LIMIT + 1 );
  buf += lit_comp_size;

  *zeros = Arith_open( buf, 0, ZERO_LENGTH_LIMIT, ZERO_LENGTH_LIMIT +1 );
  buf += zero_comp_size;

  return( buf );
}


// decode a high-pass plane (internal plane prediction with order 1)
static void decode_high_1( ARITHBITS* ab, VARBITS* vb, S16 * outp, S32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp )
{
  U32 w, h;
  S32 yadj;
  S32 max;
  U32 num, numl;
  U32 qlevel;
  ARITH* a;
  ARITH lits, zeros;
  S16 const* from;
  S32 prev, above, above_left;
  UINTADDR lit_len = 0, zero_len = 0;

  #ifdef _DEBUG
    S16* out = outp + pixel_pitch;
  #endif

  #ifdef DEBUG_PLANES
  CHECK_ARITH_MARKER( ab );
  CHECK_VAR_MARKER( *vb );
  #endif

  // get the quantization level
  VarBitsGetLE( qlevel, U32, *vb, 16 );

  // See if all bytes are a single value
  if ( VarBitsGet1LE( *vb, prev ) ) // use prev as temp
  {
    VarBitsGetLE( prev, S32, *vb, 16 );

    fill_rect( outp, pixel_pitch, enc_width, enc_height, prev * qlevel );

    return;
  }

  VarBitsGetLE( max, U32, *vb, 16 );


  // Tired of this falling over semi-silently every time we get a new
  // platform...  Note that some versions of gcc will generate a
  // subscript out of range warning for the third assert here.  This
  // is because it does not properly trim one branch of the ternary
  // operator in the getbitlevel macro
  // radassert ( GET_BIT_LEVEL ( 0x1 ) == 1 );
  // radassert ( GET_BIT_LEVEL ( 0x55 ) == 7 );
  // radcassert ( GET_BIT_LEVEL ( 0x7fff ) == 15 );

  num = max + 1;
  numl = max * qlevel;
  numl = GET_BIT_LEVEL( numl ) + 1;

  // create the arith decoders
  create_decomp_contexts( &a, &lits, &zeros, max, num, numl, temp );

  yadj = pixel_pitch - (S32)enc_width;
  h = enc_height;

  // get the first pixel plain
  above = ArithBitsGetValue( ab, num );
  if ( above )
  {
    // get the sign and invert above if the sign was set
    U32 v = -( S32 ) VarBitsGet1LE( *vb, prev ); // use prev as temp
    above = (above ^ v ) - v;
    above *= qlevel;
  }

  *outp = (S16)above;

  above_left = above;
  prev = above;

  from = outp;
  ++outp;

  if ( enc_width == 1 )
    goto after_first;

  w = enc_width - 1;

  for(;;)
  {
    lit_len = Arith_decompress( lits, ab );

    if ( Arith_was_escaped( lit_len ) )
    {
      U32 escaped;

      VarBitsGetLE( escaped, U32, *vb, LIT_LENGTH_BITS );
      Arith_set_decompressed_symbol( lit_len, escaped );

      lit_len = escaped;
    }

    // handle the escape lengths
    if ( lit_len >= ( LIT_LENGTH_LIMIT - EXTRA_LENGTHS + 1 ) )
    {
      lit_len = extra_lit_lengths[ lit_len - ( LIT_LENGTH_LIMIT - EXTRA_LENGTHS + 1 ) ];
    }

    zero_len = Arith_decompress( zeros, ab );

    if ( Arith_was_escaped( zero_len ) )
    {
      U32 escaped;

      VarBitsGetLE( escaped, U32, *vb, ZERO_LENGTH_BITS );
      Arith_set_decompressed_symbol( zero_len, escaped );

      zero_len = escaped;
    }

    // handle the escape lengths
    if ( zero_len >= ( ZERO_LENGTH_LIMIT - EXTRA_LENGTHS + 1 ) )
    {
      zero_len = extra_zero_lengths[ zero_len - ( ZERO_LENGTH_LIMIT - EXTRA_LENGTHS + 1 ) ]
                 + MIN_ZERO_LENGTH - 1;
    }
    else
    {
      if ( zero_len )
      {
        zero_len += MIN_ZERO_LENGTH - 1;
      }
    }

    while( lit_len )
    {
      if ( w <= 1 )
      {
        if ( w )
        {
          INTADDR cur;
          U32 context;

          context = ( ( U32 ) radabs( prev * 2 ) + radabs ( above_left ) + radabs( above ) ) / 4;
          context = GET_BIT_LEVEL( context );

          // get the absolute value of the pixel
          cur = Arith_decompress( a[ context ], ab );

          if ( Arith_was_escaped( cur ) )
          {
            U32 escaped;

            escaped = ArithBitsGetValue( ab, num );
            Arith_set_decompressed_symbol( cur, escaped );

            cur = escaped;
          }

          // get the sign bit if we don't have a zero
          if ( cur )
          {
            S32 v = -( S32 ) VarBitsGet1LE( *vb, w ); // use w as temp
            cur = (cur ^ v ) - v;
            cur *= qlevel;
          }

          *outp++ = (S16) cur;
          
          --lit_len;
        }

       after_first:
        if ( ( --h ) == 0 )
          return;

        w = enc_width;
        outp += yadj;

        #ifdef _DEBUG
        radassert( outp == out );
        out += pixel_pitch;
        #endif

        from = outp - pixel_pitch;

        above = *from++;
        above_left = above;
        prev = above;
      }
      else
      {
        INTADDR cur;
        S32 above_right = *from;

        U32 context;

        context = ( ( U32 ) radabs( prev ) + radabs ( above_left ) + radabs( above ) + radabs( above_right ) ) / 4;
        context = GET_BIT_LEVEL( context );

        // send the absolute value of the pixel
        cur = Arith_decompress( a[ context ], ab );

        if ( Arith_was_escaped( cur ) )
        {
          U32 escaped;

          escaped = ArithBitsGetValue( ab, num );
          Arith_set_decompressed_symbol( cur, escaped );

          cur = escaped;
        }

        // get the sign bit if we don't have zero
        if ( cur )
        {
          S32 v = -( S32 ) VarBitsGet1LE( *vb, prev ); // use prev as temp
          cur = (cur ^ v ) - v;
          cur *= qlevel;
        }

        *outp = (S16) cur;

        above_left = above;
        above = above_right;
        prev = (S32)cur;  // prev has been used as a temp ten lines up!

        ++outp;
        ++from;
        --w;
        --lit_len;
      }
    }

    while ( zero_len )
    {
      if ( zero_len >= w )
      {
        zero_len -= w;
        from += w;

        while ( w-- )
          *outp++ = 0;

        if ( ( --h ) == 0 )
          return;

        w = enc_width;
        outp += yadj;

        #ifdef _DEBUG
        radassert( outp == out );
        out += pixel_pitch;
        #endif

        from = outp - pixel_pitch;

        above = *from++;
        above_left = above;
        prev = above;
      }
      else
      {
        w -= (U32)zero_len;
        from += zero_len;

        do
        {
          *outp++ = 0;
        } while ( --zero_len );

        prev = 0;
        above = from[ -1 ];
        above_left = from[ -2 ];
      }
    }
  }
}


// decode the compressed data into decomposed planes
U32 plane_decode( void const * comp, 
                  S16 * output, 
                  U32 width, 
                  U32 height,
                  U8* row_mask, 
                  void * temp )
{
  ARITHBITS ab;
  VARBITS vb;
  U32 const * sizes = ( U32 const * ) comp;

  // open the probablity bits dumper
  ArithBitsGetStart( &ab, sizes + 2 );

  comp = ( (U8 const*) comp ) + 8 + radloadu32ptr(sizes);

  // open the uncompressed reader
  VarBitsOpen( vb, ( void* ) comp );

  #ifdef _DEBUG
  radmemset( output, 0, width * height * 2 );
  #endif

  // do the lowest low pass first
  decode_low( &ab, &vb, output, width * 16, width / 16, height / 16, temp );

  decode_high_1( &ab, &vb, output + ( width / 16 ),                 width * 16, width / 16, height / 16, temp );
  decode_high_1( &ab, &vb, output + ( width * 8 ),                  width * 16, width / 16, height / 16, temp );
  decode_high_1( &ab, &vb, output + ( width / 16 ) + ( width * 8 ), width * 16, width / 16, height / 16, temp );

  decode_high_1( &ab, &vb, output + ( width / 8 ),                 width * 8, width / 8, height / 8, temp );
  decode_high_1( &ab, &vb, output + ( width * 4 ),                 width * 8, width / 8, height / 8, temp );
  decode_high_1( &ab, &vb, output + ( width / 8 ) + ( width * 4 ), width * 8, width / 8, height / 8, temp );

  decode_high_1( &ab, &vb, output + ( width / 4 ),                 width * 4, width / 4, height / 4, temp );
  decode_high_1( &ab, &vb, output + ( width * 2 ),                 width * 4, width / 4, height / 4, temp );
  decode_high_1( &ab, &vb, output + ( width / 4 ) + ( width * 2 ), width * 4, width / 4, height / 4, temp );

  decode_high_1( &ab, &vb, output + ( width / 2 ),                 width * 2, width / 2, height / 2, temp );
  decode_high_1( &ab, &vb, output + width,                         width * 2, width / 2, height / 2, temp );
  decode_high_1( &ab, &vb, output + ( width / 2 ) + width,         width * 2, width / 2, height / 2, temp );

  if ( row_mask )
  {
    read_escapes( &ab, row_mask, height );
  }

  return( radloadu32ptr(sizes) + radloadu32ptr(sizes + 1) + 8 );
}
