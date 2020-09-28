// ========================================================================
// $File: //jeffr/granny/rt/granny_bink0_compression.cpp $
// $DateTime: 2007/09/20 15:05:27 $
// $Change: 16033 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BINK0_COMPRESSION_H)
#include "granny_bink0_compression.h"
#endif

#if !defined(GRANNY_COMPRESSION_TOOLS_H)
#include "granny_compression_tools.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_PROCESSOR_H)
#include "granny_processor.h"
#endif

#if PLATFORM_LINUX
#include <stdlib.h>
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

/* ========================================================================
   From binktc.h (Jeff's code)
   ======================================================================== */
// set this define for GL_RGBA input (vs. GL_BGRA which is the default)
#define USING_GL_RGB

// Jeff uses __RADLITTLEENDIAN__, so bridge between that and
// PROCESSOR_LITTLE_ENDIAN
#if PROCESSOR_LITTLE_ENDIAN
#define __RADLITTLEENDIAN__
#endif

//=========================================================================
// input arrays are S16 data with pixels inside -2048 to 2047
//=========================================================================

// compresses with Bink texture compression
static U32 to_BinkTC( void* output,
                      U32 compress_to,
                      S16** input,
                      F32 const * plane_weights,
                      U32 planes,
                      U32 width,
                      U32 height,
                      void * temp, U32 temp_size );

// tells how much temp memory to allocate for compression
static U32 to_BinkTC_temp_mem( U32 width, U32 height );

// tells how much output memory to allocate
static U32 to_BinkTC_output_mem( U32 width, U32 height, U32 planes, U32 compress_to );

// check the width and height for validity (usually just make divisible by 16)
static void BinkTC_check_sizes( U32 * width, U32 * height );




//=========================================================================
// input is BinkTC formatted data - output are the S16 planes
//=========================================================================

// decompresses a Bink compressed memory block
static void from_BinkTC( S16** output,
                         U32 planes,
                         void const * bink_buf,
                         U32 width,
                         U32 height,
                         void * temp,
                         U32 temp_size );

// tells how much temp memory to allocate for decompression
static U32 from_BinkTC_temp_mem( void const * binkbuf );



/* ========================================================================
   From encode.h (Jeff's code)
   ======================================================================== */
#define ENCODE_MIN_PLANE_SIZE (64 + 8)

// encode the decomposed planes
static U32 planes_encode( void* output,
                          U32 tobytes,
                          S16 const * const * input,
                          F32 const * plane_weights,
                          U32 planes,
                          U32 width,
                          U32 height,
                          void * temp, U32 temp_size,
                          U32 * decomp_min_size );

// decode from the compressed data into a decomposed plane
static U32 plane_decode( void const * comp,
                         S16 * output,
                         U32 width,
                         U32 height,
                         U8 * row_mask,
                         void * temp );

/* ========================================================================
   From wavelet.h (Jeff's code)
   ======================================================================== */

#define SMALLEST_DWT_ROW 12
#define SMALLEST_DWT_COL 10

// apply DWT (9/7) wavelet by rows
static void fDWTrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );

// apply Harr wavelet by columns
static void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );

// apply Harr wavelet by rows
static void fHarrrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );

// apply DWT (9/7) wavelet by columns
static void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );

// perform inverse DWT (9/7) wavelet by rows
static void iDWTrow( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, U8 const * row_mask, S32 starty, S32 subheight );

// perform inverse DWT (9/7) wavelet by columns
static void iDWTcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, S32 starty, S32 subheight );

// perform Harr wavelet by rows
static void iHarrrow( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, U8 const * row_mask, S32 starty, S32 subheight );

// perform Harr wavelet by columns
static void iHarrcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, S32 starty, S32 subheight );

// apply DWT (9/7) wavelet across two dimensions (flips to Harr when too small)
static void fDWT2D( S16* buffer, S32 pitch, S32 width, S32 height, S16* temp );

// perform inverse DWT (9/7) wavelet across two dimensions with a mask (flips to Harr when too small)
static void iDWT2D( S16* buffer, S32 pitch, S32 width, S32 height, U8 const * row_mask, S16* temp );

/* ========================================================================
   From binktc.c (Jeff's code)
   ======================================================================== */
#define fWave2D fDWT2D
#define iWave2D iDWT2D
#define iWave2D_mask iDWT2D_mask


// how much temp memory to we need during compression
static U32 to_BinkTC_temp_mem( U32 width, U32 height )
{
  return( width * height * 2 + ( ( ( ( width * height ) / 8 ) + 256 + 15 ) & ~15 ) + 64 * 1024 + 4096 );
}


// how much temp memory to we need during decompression
static U32 from_BinkTC_temp_mem( void const * binkbuf )
{
  return( ( ( U32* ) binkbuf )[ 0 ] );
}


// how much output memory should we allocate
static U32 to_BinkTC_output_mem( U32 width, U32 height, U32 planes, U32 compress_to )
{
  U32 out_size;
  U32 i;

  // account for hard bit cost
  compress_to += ( ENCODE_MIN_PLANE_SIZE * planes );

  // start with twice the compress_to size
  //out_size = compress_to * 3;
  out_size = compress_to * 30;      /// Hmmm, let's try a bit bigger for luck.

  // limit to four times uncompressed
  i = width * height * planes * 4;
  if ( out_size > i )
    out_size = i;

  // figure out the size of the lowest low-pass plane
  i = ( width * height / 256 ) * planes;
  i = ( i * 3 ) / 2;  // 150%

  if ( out_size < i )
    out_size = i;


  // make sure we have at least 16K
  if ( out_size < 16384 )
    out_size = 16384;

  return( out_size );
}


// check the width and height for validity (usually just make divisible by 16)
static void BinkTC_check_sizes( U32 * width, U32 * height )
{
  *width =  ( *width + 15 ) & ~15;
  *height = ( *height + 15 ) & ~15;
}

#if 0   // Not actually used
// check the width and height for validity (usually just make divisible by 16)
static S32 BinkTC_should_compress( U32 width, U32 height )
{
  return( ( ( width * height * 2 ) <= ( ENCODE_MIN_PLANE_SIZE * 3 ) ) ? 0 : 1 );
}
#endif


//=====================================================================
// input arrays are S16 data with pixels no larger than -1024 to +1023
//=====================================================================

// compresses with Bink texture compression
static U32 to_BinkTC( void* output,
                      U32 tobytes,
                      S16** input,
                      F32 const * plane_weights,
                      U32 planes,
                      U32 width,
                      U32 height,
                      void * temp, U32 temp_size )
{
  U32 i;
  U32 size = 0;
  S16 * tplane;

  // make sure we have enough memory to perform the wavelets
  i = to_BinkTC_temp_mem( width, height );
  if ( ( temp == 0 ) || ( temp_size < i ) )
  {
    temp = 0;
    tplane = ( S16* )radmalloc( i );
    temp_size = i;
  }
  else
  {
    tplane = ( S16* ) temp;
  }


// uncomment to check wavelet before anything's been encoded
//#define WAVETEST
#ifdef WAVETEST
{  S16 *origpixels[8], *testpixels;

  testpixels = (S16*) radmalloc( width * height * 2 * ( planes + 1 ) );
  origpixels[0] = testpixels + width * height;
  memcpy( origpixels[0], input[0], width * height * 2 );

  for ( i = 1 ; i < planes ; i++ )
  {
    origpixels[i] = origpixels[i-1] + width * height;
    memcpy( origpixels[i], input[i], width * height * 2 );
  }

#endif


  // decompose the planes with the wavelet
  for ( i = 0 ; i < planes ; i++ )
  {
    fWave2D( input[ i ], width * 2, width, height, tplane );
    fWave2D( input[ i ], width * 2 * 2, width / 2, height / 2, tplane );
    fWave2D( input[ i ], width * 2 * 4, width / 4, height / 4, tplane );
    fWave2D( input[ i ], width * 2 * 8, width / 8, height / 8, tplane );
  }

  // the first dword store the memory necessary at decompression time
  ( ( U32* ) output )[ 0 ] = ( width * height * 2 );


  // encode the planes
  size = planes_encode( ( ( U32* ) output ) + 1,
                        tobytes,
                        input,
                        plane_weights,
                        planes,
                        width,
                        height,
                        tplane, temp_size,
                        ( ( U32* ) output ) );

  // add in enough to cover the row escapes at decomp time
  ( ( U32* ) output )[ 0 ] += ( ( height + 15 ) & ~15 );


  // debugging code
#ifdef WAVETEST
  for ( i = 0 ; i < planes ; i++ )
  {
    // reconstruct the planes for testing...
    memcpy( testpixels, input[i], width * height * 2 );

    iWave2D( testpixels, width * 2 * 8, width / 8, height / 8, 0, tplane );
    iWave2D( testpixels, width * 2 * 4, width / 4, height / 4, 0, tplane );
    iWave2D( testpixels, width * 2 * 2, width / 2, height / 2, 0, tplane );
    iWave2D( testpixels, width * 2, width, height, 0, tplane );

    {
      U32 x,y;
      for( y = 0 ; y < height; y++)
      {
        for( x = 0 ; x < width; x++)
        {
          if (origpixels[i][y*width+x]!=testpixels[y*width+x])
            printf("Mismatch: %i %i (%i %i %i)\n",x,y,origpixels[i][y*width+x],testpixels[y*width+x],origpixels[i][y*width+x]-testpixels[y*width+x]);
        }
      }
    }
  }

  radfree( testpixels );
  }
  #undef WAVETEST
#endif

  if ( temp == 0 )
  {
    radfree( tplane );
  }

  return( size + 4 );
}


//=========================================================================
// input is BinkTC formatted data - output is the S16 planes
//=========================================================================

// decompress the Bink texture compressed block
static void from_BinkTC( S16** output, U32 planes, void const * bink_buf, U32 width, U32 height, void * temp, U32 temp_size )
{
  U32 i;
  S16 * tplane;
  U8 * row;

  // make sure we have enough memory to decompress
  i = from_BinkTC_temp_mem( bink_buf );
  if ( ( temp == 0 ) || ( temp_size < i ) )
  {
    temp = 0;
    row = ( U8 *)radmalloc( i );
    temp_size = i;
  }
  else
  {
    row = ( U8* ) temp;
  }

  // skip the decompressed temp size
  bink_buf = ( ( U32 * ) bink_buf ) + 1;

  tplane = ( S16* ) ( row + ( ( height + 15 ) & ~15 ) );



  for ( i = 0 ; i < planes ; i++ )
  {
    // decode into the planes and skip to the next plane
    bink_buf = ( ( U8* ) bink_buf ) +
      plane_decode( bink_buf, output[ i ], width, height, ( U8* ) temp, tplane );

    // recompose the planes with the wavelet
    iWave2D( output[ i ], width * 2 * 8, width / 8, height / 8, 0, tplane );
    iWave2D( output[ i ], width * 2 * 4, width / 4, height / 4, 0, tplane );
    iWave2D( output[ i ], width * 2 * 2, width / 2, height / 2, 0, tplane );
    iWave2D( output[ i ], width * 2, width, height, ( U8* ) temp, tplane );
  }

  if ( temp == 0 )
  {
    radfree( row );
  }
}

/* ========================================================================
   From encode.c (Jeff's code)
   ======================================================================== */
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
#define CHECK_VAR_MARKER(vb)    { U32 v; VarBitsGet(v, U32, vb,16); radassert( v == 0x8743 ); }



// get the largest absolute difference from predicted pixel to actual pixel
static S32 get_diff_predicted_max( S16 const* inp, U32 pixel_pitch, U32 enc_width, U32 enc_height, S32* max )
{
  U32 x, y, yadj;
  S32 maxv = -32768;
  S32 prev, first, same;
  S16 const* from;

  same = 1;

  yadj = pixel_pitch - enc_width;

  first = *inp++;

  prev = first;
  for( x = ( enc_width - 1 ) ; x-- ; )
  {
    S32 cur = abs( *inp - prev );
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
    cur = abs( *inp - *from );
    if ( cur > maxv )
      maxv = cur;
    prev = *inp++;
    if ( first != prev )
      same = 0;
    ++from;

    // do the rest of the row
    for( x = ( enc_width - 1 ) ; x-- ; )
    {
      cur = abs( *inp - ( ( prev + *from ) / 2 ) );
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


// encode a low-pass plane (internal plane prediction using pixel to left and above)
static void encode_low( ARITHBITS* ab, VARBITS* vb, S16 const* inp, U32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp, U32 temp_size, U32 * decomp_min_size )
{
  U32 x, y, yadj;
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
    VarBitsPut1( *vb, 1 );
    VarBitsPut( *vb, inp[ 0 ], 16 );
    return;
  }

  num = max + 1;
  comp_temp_size = Arith_compress_temp_size( num );
  comp_size = Arith_compress_alloc_size( max, num );

  VarBitsPut1( *vb, 0 );
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

  yadj = pixel_pitch - enc_width;


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

    if ( Arith_was_escaped( Arith_compress( a, ab, abs( cur ) ) ) )
    {
      ArithBitsPutValue( ab, abs( cur ), num );
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
    if ( Arith_was_escaped( Arith_compress( a, ab, abs( cur ) ) ) )
    {
      ArithBitsPutValue( ab, abs( cur ), num );
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

      if ( Arith_was_escaped( Arith_compress( a, ab, abs( cur ) ) ) )
      {
        ArithBitsPutValue( ab, abs( cur ), num );
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
static S32 get_abs_max( S16 const* inp, U32 pixel_pitch, U32 enc_width, U32 enc_height, U32* max )
{
  U32 x, y, yadj;
  S32 maxv = -32768;
  S32 first, same;

  same = 1;

  yadj = pixel_pitch - enc_width;

  first = *inp;

  for( y = enc_height ; y-- ; )
  {
    S32 cur;

    // do the row
    for( x = enc_width ; x-- ; )
    {
      cur = abs( *inp );
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
  i = ( ( 4 * numl ) + comp_temp_size + ( numl * comp_size ) + lit_comp_size + zero_comp_size + 15 ) & ~15;

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

  buf += ( 4 * numl );

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

  i = ( ( 4 * numl ) + ( numl * comp_size ) + lit_comp_size + zero_comp_size + 15 ) & ~15;

  if ( i > *decomp_min_size )
  {
    *decomp_min_size = i;
  }

  return( ( temp == 0 ) ? ( *a ) : 0 );
}


// count the current run of zeros (can extend beyond the scanline)
static U32 zero_run_count_plane( S16 const* scan, U32 w, U32 h, U32 pixel_pitch, U32 enc_width, S16 const** new_scan, U32 * new_w, U32 * new_h)
{
  U32 tot;
  U32 adj;

  tot = 0;
  adj = ( pixel_pitch - enc_width ) * 2;

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
static void get_lengths( S16 const* inp, U32 pixel_pitch, U32 w, U32 h, U32 enc_width, U32 * lit_full_len, U32 * zero_full_len )
{
  S16 const* scan = inp;
  U32 lits = 0;
  U32 adj;

  adj = ( pixel_pitch - enc_width ) * 2;

  while ( 1 )
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


// limit the lengths to the actual length or one of the length escapes
static U32 limit_length( U32 len, U32 limit, U32* bits, U32* extras )
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


// encode a high-pass plane (internal plane prediction with order 1)
static void encode_high_1( ARITHBITS* ab, VARBITS* vb, S16 const* inp, U32 pixel_pitch, U32 enc_width, U32 enc_height, U32 qlevel, void * temp, U32 temp_size, U32 * decomp_min_size )
{
  U32 w, h, yadj;
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
    VarBitsPut1( *vb, 1 );
    VarBitsPut( *vb, inp[ 0 ], 16 );
    return;
  }

  num = max + 1;
  numl = GET_BIT_LEVEL( max * qlevel ) + 1;

  VarBitsPut1( *vb, 0 );
  VarBitsPut( *vb, max, 16 );

  // create the arith encoders
  context_mem = create_contexts( &a, &lits, &zeros, max, num, numl, temp, temp_size, decomp_min_size );

  yadj = pixel_pitch - enc_width;
  h = enc_height;

  // send the first pixel plain
  above = *inp;
  ArithBitsPutValue( ab, abs( above ), num );
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

  while ( 1 )
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

          context = GET_BIT_LEVEL( ( ( ( U32 ) abs( prev * 2 ) + abs ( above_left ) + abs( above ) ) * qlevel ) / 4 );

          // encode the absolute value
          if ( Arith_was_escaped( Arith_compress( a[ context ], ab, abs( cur ) ) ) )
          {
            ArithBitsPutValue( ab, abs( cur ), num );
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

        context  = GET_BIT_LEVEL( ( ( ( U32 ) abs( prev ) + abs ( above_left ) + abs( above ) + abs( above_right ) ) * qlevel ) / 4 );

        // encode the literal
        if ( Arith_was_escaped( Arith_compress( a[ context ], ab, abs( cur ) ) ) )
        {
          ArithBitsPutValue( ab, abs( cur ), num);
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
static void quantize( S16 * outp, S16 const * inp, U32 pixel_pitch, U32 q_width, U32 q_height, U32 level )
{
  U32 h, yadj;

  if ( level < 1 )
    level = 1;
//    return;

  yadj = pixel_pitch - q_width;

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
static void dump_row_escapes( ARITHBITS * ab, S16 const* inp, S32 pitch, S32 width, S32 height, void * temp, U32 temp_size )
{
  U32 h;
  U8 * mask, * mask_start;
  U32 zeros = 0;

  if ( ( temp == 0 ) || ( (U32)height > temp_size ) )
  {
    mask_start = ( U8 *)radmalloc ( height );
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
static void dump_col_escapes( ARITHBITS * ab, S16 const* inp, U32 pitch, U32 width, U32 height, void* temp )
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


typedef struct subplane
{
  F64 cur_size;
  F64 next_size;
  U32 cur_error;
  U32 next_error;
  U32 cur_quant;
  U32 next_quant;
  S16 const * addr;
  U32 pitch;
  U32 width;
  U32 height;
  U32 max;
  U32 same;
  U32* histogram;
} subplane;

typedef subplane subplanes[ 13 ];


// makes a histogram from a plane
static void build_histogram( S16 const * inp, U32 pixel_pitch, U32 width, U32 height, U32 max, U32 * histogram )
{
  U32 h, yadj;

  memset( histogram, 0, 4 * ( max + 1 ) );

  yadj = pixel_pitch - width;

  for( h = height ; h-- ; )
  {
    U32 w;
    for( w = width ; w-- ; )
    {
      ++histogram[ abs( *inp ) ];
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
      size += ( qn * -radlog2( ( ( F64 ) qn ) / ( ( F64 ) total ) ) );
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
static U32 planes_encode( void* output,
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
  U32 tobits;
  U32 tweaked_times = 0;
  U32 best_bits, best_diff = (U32)-1;

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
  while ( ( RoundReal64ToUInt32(cur_size) ) > tobits )
  {
    S32 best_sub = -1;
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

    for( s = 1 ; s < 13 ; s++ )
    {
      if ( ( sps[ i ][ s ].same ) || ( sps[ i ][ s ].cur_quant <= 1 ) )
      {
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
    U32 diff = abs( ( S32 ) size - ( S32 ) tobytes );

    if ( diff < best_diff )
    {
      best_diff = diff;
      best_bits = tobits;
      tweaked_times = 0;
    }

    // if the output rate is more than 2% off the requested rate, try again
    if ( ( ( diff * 100 ) / tobytes ) >= 2 )
    {
      tobits = 8 * RoundReal64ToInt32 ( ( ( F64 ) tobytes * ( F64 ) tobits ) / ( ( F64 ) size * 8 ) );
      if ( ( tobits / 8 ) > ( ( tobytes * 5 ) / 2 ) )
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

#if 0
  // if we haven't tweaked the rate twice yet, check it
  if ( ++tweaked_times <= 2 )
  {
    // if the output rate is more than 2% off the requested rate, try again
    if ( abs( ( ( ( S32 ) size - ( S32 ) tobytes ) * 100 ) / ( S32 ) tobytes ) >= 2 )
    {
      tobits = 8 *  RoundReal64ToInt32( ( ( F64 ) tobytes * ( F64 ) tobits ) / ( ( F64 ) size * 8 ) );
      goto again;
    }
  }
#endif

  if ( temp1 )
    radfree( temp1 );

  if ( temp2 )
    radfree( temp2 );

  return( size );
}


// fills a rect with a particular value
static void fill_rect( S16 * outp, U32 pixel_pitch, U32 width, U32 height, S32 val )
{
  U32 h, yadj;

  yadj = pixel_pitch - width;

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
static void decode_low( ARITHBITS* ab, VARBITS* vb, S16 * outp, U32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp )
{
  U32 w, h, yadj;
  S32 max, prev;
  U32 num;
  ARITH a;
  U8 * buf;
  S16 * from;

  // See if all bytes are a single value
  if ( VarBitsGet1( *vb, prev ) )  // prev used as temporary
  {
    VarBitsGet( prev, S32, *vb, 16 );
    fill_rect( outp, pixel_pitch, enc_width, enc_height, prev );
    return;
  }

  VarBitsGet( max, S32, *vb, 16 );

  num = max + 1;

  buf = (U8*) temp;

  a = Arith_open( buf, 0, max, num );

  yadj = pixel_pitch - enc_width;

  // get the first pixel with no prediction
  VarBitsGet( prev, U32, *vb, 16 );

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
      S32 v = -( S32 ) VarBitsGet1( *vb, tmp );
      cur = (cur ^ v ) - v;
    }

    //Assert(cur <= Int32Maximum);
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
      S32 v = -( S32 ) VarBitsGet1( *vb, w ); // w used as temp
      cur = (cur ^ v ) - v;
    }

    //Assert(cur <= Int32Maximum);
    prev = (S32)cur + *from;
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
        S32 v = -( S32 ) VarBitsGet1( *vb, tmp );
        cur = (cur ^ v ) - v;
      }

      //Assert(cur < Int32Maximum);
      prev = (S32)cur + ( ( prev + *from ) / 2 );
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
static void decode_high_1( ARITHBITS* ab, VARBITS* vb, S16 * outp, U32 pixel_pitch, U32 enc_width, U32 enc_height, void * temp )
{
  U32 w, h, yadj;
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
  VarBitsGet( qlevel, U32, *vb, 16 );

  // See if all bytes are a single value
  if ( VarBitsGet1( *vb, prev ) ) // use prev as temp
  {
    VarBitsGet( prev, S32, *vb, 16 );

    fill_rect( outp, pixel_pitch, enc_width, enc_height, prev * qlevel );

    return;
  }

  VarBitsGet( max, U32, *vb, 16 );



  num = max + 1;
  numl = GET_BIT_LEVEL( max * qlevel ) + 1;

  // create the arith decoders
  create_decomp_contexts( &a, &lits, &zeros, max, num, numl, temp );

  yadj = pixel_pitch - enc_width;
  h = enc_height;

  // get the first pixel plain
  above = ArithBitsGetValue( ab, num );
  if ( above )
  {
    // get the sign and invert above if the sign was set
    U32 v = -( S32 ) VarBitsGet1( *vb, prev ); // use prev as temp
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

  while ( 1 )
  {
    lit_len = Arith_decompress( lits, ab );

    if ( Arith_was_escaped( lit_len ) )
    {
      U32 escaped;

      VarBitsGet( escaped, U32, *vb, LIT_LENGTH_BITS );
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

      VarBitsGet( escaped, U32, *vb, ZERO_LENGTH_BITS );
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
          U32 context = GET_BIT_LEVEL( ( ( U32 ) abs( prev * 2 ) + abs ( above_left ) + abs( above ) ) / 4 );

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
            S32 v = -( S32 ) VarBitsGet1( *vb, w ); // use w as temp
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

        U32 context = GET_BIT_LEVEL( ( ( U32 ) abs( prev ) + abs ( above_left ) + abs( above ) + abs( above_right ) ) / 4 );

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
          S32 v = -( S32 ) VarBitsGet1( *vb, prev ); // use prev as temp
          cur = (cur ^ v ) - v;
          cur *= qlevel;
        }

        *outp = (S16) cur;

        above_left = above;
        above = above_right;

        Assert(cur < Int32Maximum);
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
        // we know from the test that zero_len < w
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
static U32 plane_decode( void const * comp,
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

  comp = ( (U8 const*) comp ) + 8 + sizes[ 0 ];

  // open the uncompressed reader
  VarBitsOpen( vb, ( void* ) comp );


  // Tired of this falling over semi-silently every time we get a new
  // platform...  Note that some versions of gcc will generate a
  // subscript out of range warning for the third assert here.  This
  // is because it does not properly trim one branch of the ternary
  // operator.
  radassert ( GET_BIT_LEVEL ( 0x1 ) == 1 );
  radassert ( GET_BIT_LEVEL ( 0x55 ) == 7 );
  radassert ( GET_BIT_LEVEL ( 0x7fff ) == 15 );


  #ifdef _DEBUG
  memset( output, 0, width * height * 2 );
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

  return( sizes[ 0 ] + sizes[ 1 ] + 8 );
}

/* ========================================================================
   From wavelet.c (Jeff's code)
   ======================================================================== */
// uncomment this comment to break when writing unaligned - this is slow!
//#define ENSURE_ALIGNMENT

#if PROCESSOR_X86

#ifdef ENSURE_ALIGNMENT

#define MOVE_8( d, s )                                      \
      if (( ((U32)(d))&7 ) || ( ((U32)(s))&7 )) BREAK_POINT(); \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d( d1, d2, s )                                                           \
      if (( ((U32)(d2))&7 ) || (((U32)(d1))&7 ) || ( ((U32)(s))&7 )) BREAK_POINT(); \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_4a( d, s )                                   \
      if (( ((U32)(d))&3 ) || ( ((U32)(s))&3 )) BREAK_POINT(); \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d_4a( d1, d2, s )                                                        \
      if (( ((U32)(d2))&3 ) || (((U32)(d1))&3 ) || ( ((U32)(s))&3 )) BREAK_POINT(); \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#else

#define MOVE_8( d, s )                                      \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d( d1, d2, s )                                                           \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_4a( d, s )                                   \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d_4a( d1, d2, s )                                                        \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#endif

#define ALIGN_8_TYPE double __declspec(align(8))

#else

#define MOVE_8( d, s )                                      \
        ( ( F64 * )( d ) )[ 0 ] = ( ( F64* ) ( s ) )[ 0 ];

#define MOVE_8_d( d1, d2, s )                               \
        ( ( F64 * )( d1 ) )[ 0 ] = ( ( F64 * )( d2 ) )[ 0 ] = ( ( F64* ) ( s ) )[ 0 ];

#define MOVE_8_4a( d, s )                                   \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d_4a( d1, d2, s )                                                        \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define ALIGN_8_TYPE double

#endif


// forward transforms a row with the DWT
static void fDWTrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 y;
  char *in, *hout, *lout, *end;

  radassert( width >= SMALLEST_DWT_ROW );

  in = (char*) input;
  lout = (char*) dest;
  hout = ( (char*) dest ) + width;

  in += inpitch * starty;
  lout += ppitch * starty;
  hout += ppitch * starty;

  end = in + ( 2 * width );

  for( y = subheight ; y-- ; )
  {
    S32 x;
    char *xin, *xhout, *xlout;
    S32 p[ 9 ];
    U32 next;

    xin = in;
    next = 2;

    // get the row starting pixels
    p[ 0 + 4 ] = ( ( S16* ) xin )[ 0 ];
    p[ -1 + 4 ] = p[ 1 + 4 ] = ( ( S16* ) xin )[ 1 ];
    p[ -2 + 4 ] = p[ 2 + 4 ] = ( ( S16* ) xin )[ 2 ];
    p[ -3 + 4 ] = p[ 3 + 4 ] = ( ( S16* ) xin )[ 3 ];
    p[ -4 + 4 ] = p[ 4 + 4 ] = ( ( S16* ) xin )[ 4 ];

    xin += 5 * 2;
    xlout = lout;
    xhout = hout;

    for( x = ( width / 2 ); x-- ; )
    {
      S32 l, h;

      if ( xin == end )
      {
        xin = xin - 2 - 2;
        next = (U32)-2;
      }

      l =
        ( ( ( p[ -4 + 4 ] + p[ 4 + 4 ] ) * 2479 )
        - ( ( p[ -3 + 4 ] + p[ 3 + 4 ] ) * 1563 )
        - ( ( p[ -2 + 4 ] + p[ 2 + 4 ] ) * 7250 )
        + ( ( p[ -1 + 4 ] + p[ 1 + 4 ] ) * 24733 )
        +     p[ 0 + 4 ] * 55883 );

      h =
        ( ( ( p[ -2 + 4 + 1 ] + p[ 2 + 4 + 1 ] ) * 2667 )
        - ( ( p[ -3 + 4 + 1 ] + p[ 3 + 4 + 1 ] ) * 4230 )
        + ( ( p[ -1 + 4 + 1 ] + p[ 1 + 4 + 1 ] ) * 27400 )
        -     p[ 0 + 4 + 1 ] * 51674 );

      // round up and truncate back to 16-bit
      l = ( l + ( 32767 ^ ( l >> 31 ) ) ) / 65536;
      h = ( h + ( 32767 ^ ( h >> 31 ) ) ) / 65536;

      ( *(S16*) xlout ) = ( S16 ) l;
      ( *(S16*) xhout ) = ( S16 ) h;

      // rotate the pixels and read the next
      p[ 0 ] = p[ 2 ];
      p[ 1 ] = p[ 3 ];
      p[ 2 ] = p[ 4 ];
      p[ 3 ] = p[ 5 ];
      p[ 4 ] = p[ 6 ];
      p[ 5 ] = p[ 7 ];
      p[ 6 ] = p[ 8 ];
      p[ 7 ] = ( ( S16* ) xin )[ 0 ];
      xin += next;

      if ( xin == end )
      {
        xin = xin - 2 - 2;
        next = (U32)-2;
      }

      p[ 8 ] = ( ( S16* ) xin )[ 0 ];
      xin += next;

      xlout += 2;
      xhout += 2;
    }

    in += inpitch;
    hout += ppitch;
    lout += ppitch;
    end += inpitch;
  }
}


// forward transforms a column with the DWT
static void fDWTcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 x;
  char const *in;
  char const *end;
  char *out;

  radassert( height >= SMALLEST_DWT_COL );

  in = (char const *) input;
  out = (char*) dest;

  end = in + ( inpitch * height );

  if ( starty )
  {
    in += inpitch * ( starty - 4 );
    out += pitch * starty;
  }

  for( x = width ; x-- ; )
  {
    S32 y;
    char const * yin;
    char *yout;
    S32 p[ 9 ];
    S32 next;

    yin = in;
    next = inpitch;
    yout = out;

    // read the pixels into the array
    if ( starty )
    {
      // first 8 are always linear
      p[ -4 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -3 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -2 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -1 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ 0 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ 1 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ 2 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ 3 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;

      if ( yin == end )
      {
        yin = yin - next - next;
        next = -next;
      }

      p[ 4 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += next;
    }
    else
    {
      p[ 0 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -1 + 4 ] = p[ 1 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -2 + 4 ] = p[ 2 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -3 + 4 ] = p[ 3 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
      p[ -4 + 4 ] = p[ 4 + 4 ] = ( ( S16* ) yin )[ 0 ];
      yin += inpitch;
    }

    for( y = ( subheight /2 ) ; y-- ; )
    {
      S32 l, h;

      if ( yin == end )
      {
        yin = yin - next - next;
        next = -next;
      }

      l =
        ( ( ( p[ -4 + 4 ] + p[ 4 + 4 ] ) * 2479 )
        - ( ( p[ -3 + 4 ] + p[ 3 + 4 ] ) * 1563 )
        - ( ( p[ -2 + 4 ] + p[ 2 + 4 ] ) * 7250 )
        + ( ( p[ -1 + 4 ] + p[ 1 + 4 ] ) * 24733 )
        +     p[ 0 + 4 ] * 55883 );

      h =
        ( ( ( p[ -2 + 4 + 1 ] + p[ 2 + 4 + 1 ] ) * 2667 )
        - ( ( p[ -3 + 4 + 1 ] + p[ 3 + 4 + 1 ] ) * 4230 )
        + ( ( p[ -1 + 4 + 1 ] + p[ 1 + 4 + 1 ] ) * 27400 )
        -     p[ 0 + 4 + 1 ] * 51674 );

      // round up and truncate back to 16-bit
      l = ( l + ( 32767 ^ ( l >> 31 ) ) ) / 65536;
      h = ( h + ( 32767 ^ ( h >> 31 ) ) ) / 65536;

      ( *(S16*) yout ) = ( S16 ) l;
      yout += pitch;
      ( *(S16*) yout ) = ( S16 ) h;
      yout += pitch;

      // rotate and read the next pixel
      p[ 0 ] = p[ 2 ];
      p[ 1 ] = p[ 3 ];
      p[ 2 ] = p[ 4 ];
      p[ 3 ] = p[ 5 ];
      p[ 4 ] = p[ 6 ];
      p[ 5 ] = p[ 7 ];
      p[ 6 ] = p[ 8 ];
      p[ 7 ] = ( ( S16* ) yin )[ 0 ];
      yin += next;

      if ( yin == end )
      {
        yin = yin - next - next;
        next = -next;
      }

      p[ 8 ] = ( ( S16* ) yin )[ 0 ];
      yin += next;

    }

    in += 2;
    out += 2;
    end += 2;
  }
}


// forward transforms a row with the Harr wavelet
static void fHarrrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 y;
  char const * in;
  char * hout, *lout;

  radassert( ( ( width & 1 ) == 0 ) );

  in = (char const *) input;
  lout = (char*) dest;
  hout = ( (char*) dest ) + width;

  in += inpitch * starty;
  lout += ppitch * starty;
  hout += ppitch * starty;

  for( y = subheight ; y-- ; )
  {
    S32 x;
    S16 const *xin;
    S16 *xhout, *xlout;

    xin = ( S16 const * ) in;
    xlout = ( S16* ) lout;
    xhout = ( S16* ) hout;

    for( x = ( width / 2 ); x-- ; )
    {
      *xlout = ( S16 ) ( ( (S32) xin[ 0 ] + (S32) xin[ 1 ] ) / 2 );
      *xhout = ( S16 ) ( xin[ 0 ] - xin[ 1 ] );

      xin += 2;
      ++xlout;
      ++xhout;
    }

    in += inpitch;
    hout += ppitch;
    lout += ppitch;
  }
}


// forward transforms a column with the Harr wavelet
static void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 x;
  char const *in;
  char *out;

  radassert( ( ( height & 1 ) == 0 ) );

  in = (char const *) input;
  out = (char*) dest;

  in += inpitch * starty;
  out += pitch * starty;

  for( x = width ; x-- ; )
  {
    S32 y;
    char const * yin;
    char *yout;

    yin = in;
    yout = out;

    for( y = ( subheight / 2 ) ; y-- ; )
    {
      ( *(S16*) yout ) = (S16)( ( (S32) ( ( S16* ) yin )[ 0 ] + (S32) ( ( S16* ) ( yin + inpitch ) )[ 0 ] ) / 2 );
      yout += pitch;
      ( *(S16*) yout ) = (S16)( ( ( ( S16* ) yin )[ 0 ] - ( ( S16* ) ( yin + inpitch ) )[ 0 ] ) );
      yout += pitch;

      yin += inpitch + inpitch;
    }

    in += 2;
    out += 2;
  }
}

typedef void (*WAVEROW)( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );
typedef void (*WAVECOL)( S16* dest, S32 pitch, S16 const* input, S32 inpitch, S32 width, S32 height, S32 starty, S32 subheight );


// does a forward 2D wavelet in place (using DWT until too small and then flipping to Harr)
// this routine alternates between columns and rows to keep everything in the cache
static void fDWT2D( S16* input, S32 pitch, S32 width, S32 height, S16* temp )
{
  #define FLIPSIZE 8

  U32 rh, ch, ry, cy;

  WAVEROW row = ( width >= SMALLEST_DWT_ROW ) ? fDWTrow : fHarrrow;
  WAVECOL col = ( height >= SMALLEST_DWT_COL ) ? fDWTcol :fHarrcol;

  cy = ( height <= ( FLIPSIZE + 4 + 8 ) ) ? height : ( FLIPSIZE + 4 );
  ch = height - cy;
  ry = 0;
  rh = height;

  col( temp, pitch, input, pitch, width, height, 0, cy );

  do
  {
    U32 next;

    next = ( rh <= ( FLIPSIZE + 8 ) ) ? rh : FLIPSIZE;
    row( input, pitch, temp, pitch, width, height, ry, next );
    ry += next;
    rh -= next;

    if ( ch )
    {
      next = ( ch <= ( FLIPSIZE + 8 ) ) ? ch : FLIPSIZE;
      col( temp, pitch, input, pitch, width, height, cy, next );
      cy += next;
      ch -= next;
    }

  } while ( rh );

  #undef FLIPSIZE
}


// inverse transforms a row with the DWT (this routine is unrolled to do 4 pixels at once)
static void iDWTrow( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, S32 width, S32 height, U8 const * row_mask, S32 starty, S32 subheight )
{
  U32 y;
  char *out, *hin, *lin;
  U32 halfwidth;

  radassert( width >= SMALLEST_DWT_ROW );

  halfwidth = width / 2;

  out = (char*) dest;
  lin = (char*) in;
  hin = ( (char*) lin ) + width;  // really 2 * halfwidth (which is just width)

  out += starty * pitch;
  hin += starty * ppitch;
  lin += starty * ppitch;
  row_mask += starty;

  for( y = subheight ; y-- ; )
  {
    U32 x;
    S32 next;
    char *xout, *xhin, *xlin;

    struct
    {
      ALIGN_8_TYPE align1;
      S16 lp[ 4 + 3 + 1 ];  // plus 1 to maintain hp's alignment
      S16 hp[ 5 + 3 ];
    } a;

    next = 2;

    xout = out;
    xlin = lin;
    xhin = hin;

    // we read in the first 6 pixels

    // the first 6 pixels are guaranteed linear
    a.lp[ 0 + 1 ] = * ( S16* ) xlin;
    a.lp[ -1 + 1 ] = a.lp[ 1 + 1 ] = * ( S16* ) ( xlin + 2 );
    a.lp[ 2 + 1 ] = * ( S16* ) ( xlin + 4 );
    a.lp[ 2 + 1 + 1 ] = * ( S16* ) ( xlin + 6 );
    a.lp[ 2 + 1 + 2 ] = * ( S16* ) ( xlin + 8 );
    a.lp[ 2 + 1 + 3 ] = * ( S16* ) ( xlin + 10 );
    xlin += 12;

    MOVE_8_4a( &a.hp[ 0 + 2 ], xhin );
    a.hp[ 2 + 2 + 2 ] =  * ( S16* ) ( xhin + 8 );
    a.hp[ 2 + 2 + 3 ] =  * ( S16* ) ( xhin + 10 );
    a.hp[ -2 + 2 ] = a.hp[ 1 + 2 ];
    a.hp[ -1 + 2 ] = a.hp[ 0 + 2 ];
    xhin += 12;

    x = ( halfwidth < 8 ) ? 0 : ( ( halfwidth - 8 ) / 4 );

    if ( ( ( ( UINTADDR ) row_mask ) <= (U32)height ) || ( *row_mask ) )
    {
      // do groups of four pixels
      while(  x-- )
      {
        S32 e1, o1, e2, o2, e3, o3, e4, o4;

        e1 =
          (     ( a.lp[ 0 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 1 + 1 ] ) * 2667 )
            - ( ( a.hp[ -2 + 2 ] + a.hp[ 1 + 2 ] ) * 1563 )
            + ( ( a.hp[ -1 + 2 ] + a.hp[ 0 + 2 ] ) * 24733 )
          );

        o1 =
          ( (   ( a.lp[ 0 + 1 ] + a.lp[ 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 2 + 1 ] ) * 4230 )
            -   ( a.hp[ 0 + 2 ] * 55882 )
            - ( ( a.hp[ -2 + 2 ] + a.hp[ 2 + 2 ] ) * 2479 )
            + ( ( a.hp[ -1 + 2 ] + a.hp[ 1 + 2 ] ) * 7250 )
          );

        e2 =
          (     ( a.lp[ 0 + 1 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 1 ] + a.lp[ 1 + 1 + 1 ] ) * 2667 )
            - ( ( a.hp[ -2 + 2 + 1 ] + a.hp[ 1 + 2 + 1 ] ) * 1563 )
            + ( ( a.hp[ -1 + 2 + 1 ] + a.hp[ 0 + 2 + 1 ] ) * 24733 )
          );

        o2 =
          ( (   ( a.lp[ 0 + 1 + 1 ] + a.lp[ 1 + 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 1 ] + a.lp[ 2 + 1 + 1 ] ) * 4230 )
            -   ( a.hp[ 0 + 2 + 1 ] * 55882 )
            - ( ( a.hp[ -2 + 2 + 1 ] + a.hp[ 2 + 2 + 1 ] ) * 2479 )
            + ( ( a.hp[ -1 + 2 + 1 ] + a.hp[ 1 + 2 + 1 ] ) * 7250 )
          );

        e3 =
          (     ( a.lp[ 0 + 1 + 2 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 2 ] + a.lp[ 1 + 1 + 2 ] ) * 2667 )
            - ( ( a.hp[ -2 + 2 + 2 ] + a.hp[ 1 + 2 + 2 ] ) * 1563 )
            + ( ( a.hp[ -1 + 2 + 2 ] + a.hp[ 0 + 2 + 2 ] ) * 24733 )
          );

        o3 =
          ( (   ( a.lp[ 0 + 1 + 2 ] + a.lp[ 1 + 1 + 2 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 2 ] + a.lp[ 2 + 1 + 2 ] ) * 4230 )
            -   ( a.hp[ 0 + 2 + 2 ] * 55882 )
            - ( ( a.hp[ -2 + 2 + 2 ] + a.hp[ 2 + 2 + 2 ] ) * 2479 )
            + ( ( a.hp[ -1 + 2 + 2 ] + a.hp[ 1 + 2 + 2 ] ) * 7250 )
          );

        e4 =
          (     ( a.lp[ 0 + 1 + 3 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 3 ] + a.lp[ 1 + 1 + 3 ] ) * 2667 )
            - ( ( a.hp[ -2 + 2 + 3 ] + a.hp[ 1 + 2 + 3 ] ) * 1563 )
            + ( ( a.hp[ -1 + 2 + 3 ] + a.hp[ 0 + 2 + 3 ] ) * 24733 )
          );

        o4 =
          ( (   ( a.lp[ 0 + 1 + 3 ] + a.lp[ 1 + 1 + 3 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 3 ] + a.lp[ 2 + 1 + 3 ] ) * 4230 )
            -   ( a.hp[ 0 + 2 + 3 ] * 55882 )
            - ( ( a.hp[ -2 + 2 + 3 ] + a.hp[ 2 + 2 + 3 ] ) * 2479 )
            + ( ( a.hp[ -1 + 2 + 3 ] + a.hp[ 1 + 2 + 3 ] ) * 7250 )
          );

        // round up and truncate back to 16-bit
        e1 = ( e1 + ( 32767 ^ ( e1 >> 31 ) ) ) / 65536;
        o1 = ( o1 + ( 32767 ^ ( o1 >> 31 ) ) ) / 65536;
        e2 = ( e2 + ( 32767 ^ ( e2 >> 31 ) ) ) / 65536;
        o2 = ( o2 + ( 32767 ^ ( o2 >> 31 ) ) ) / 65536;
        e3 = ( e3 + ( 32767 ^ ( e3 >> 31 ) ) ) / 65536;
        o3 = ( o3 + ( 32767 ^ ( o3 >> 31 ) ) ) / 65536;
        e4 = ( e4 + ( 32767 ^ ( e4 >> 31 ) ) ) / 65536;
        o4 = ( o4 + ( 32767 ^ ( o4 >> 31 ) ) ) / 65536;

  #ifdef __RADLITTLEENDIAN__
        ( (S32*) xout )[ 0 ] = ( o1 << 16 ) | ( e1 & 0xffff );
        ( (S32*) xout )[ 1 ] = ( o2 << 16 ) | ( e2 & 0xffff );
        ( (S32*) xout )[ 2 ] = ( o3 << 16 ) | ( e3 & 0xffff );
        ( (S32*) xout )[ 3 ] = ( o4 << 16 ) | ( e4 & 0xffff );
  #else
        ( (S32*) xout )[ 0 ] = ( e1 << 16 ) | ( o1 & 0xffff );
        ( (S32*) xout )[ 1 ] = ( e2 << 16 ) | ( o2 & 0xffff );
        ( (S32*) xout )[ 2 ] = ( e3 << 16 ) | ( o3 & 0xffff );
        ( (S32*) xout )[ 3 ] = ( e4 << 16 ) | ( o4 & 0xffff );
  #endif

        xout += 16;

        ( ( S32* ) &a.lp[ 0 ] )[ 0 ] = ( ( S32* ) ( &a.lp[ 4 ] ) )[ 0 ];
        a.lp[ 2 ] = a.lp[ 6 ];
        a.lp[ 3 ] = * ( S16* ) xlin;  // can't block copy since these two addresses are unaligned
        a.lp[ 4 ] = * ( S16* ) ( xlin + 2 );
        a.lp[ 5 ] = * ( S16* ) ( xlin + 4 );
        a.lp[ 6 ] = * ( S16* ) ( xlin + 6 );

        MOVE_8( &a.hp[ 0 ], &a.hp[ 4 ] );
        MOVE_8_4a( &a.hp[ 4 ], xhin );

        xlin += 8;
        xhin += 8;
      }

      // do the remenants
      x = ( halfwidth < 8 ) ? halfwidth : ( ( halfwidth & 3 ) + 8 );

      while ( x-- )
      {
        S32 e, o;

        if ( xlin == hin )
        {
          xlin = xlin - next;
          xhin = xhin - next - next;
          next = -next;
        }

        e =
          (     ( a.lp[ 0 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 1 + 1 ] ) * 2667 )
            - ( ( a.hp[ -2 + 2 ] + a.hp[ 1 + 2 ] ) * 1563 )
            + ( ( a.hp[ -1 + 2 ] + a.hp[ 0 + 2 ] ) * 24733 )
          );

        o =
          ( (   ( a.lp[ 0 + 1 ] + a.lp[ 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 2 + 1 ] ) * 4230 )
            -   ( a.hp[ 0 + 2 ] * 55882 )
            - ( ( a.hp[ -2 + 2 ] + a.hp[ 2 + 2 ] ) * 2479 )
            + ( ( a.hp[ -1 + 2 ] + a.hp[ 1 + 2 ] ) * 7250 )
          );

        // round up and truncate back to 16-bit
        e = ( e + ( 32767 ^ ( e >> 31 ) ) ) / 65536;
        o = ( o + ( 32767 ^ ( o >> 31 ) ) ) / 65536;

  #ifdef __RADLITTLEENDIAN__
        ( (S32*) xout )[ 0 ] = ( o << 16 ) | ( e & 0xffff );
  #else
        ( (S32*) xout )[ 0 ] = ( e << 16 ) | ( o & 0xffff );
  #endif

        xout += 2 + 2;

        radassert( xlin >= lin );
        radassert( xhin >= ( hin - 2) );
        radassert( xlin < ( lin + width ) );
        radassert( xhin < ( hin + width ) );

        a.lp[ 0 ] = a.lp[ 1 ];
        a.lp[ 1 ] = a.lp[ 2 ];
        a.lp[ 2 ] = a.lp[ 3 ];
        a.lp[ 3 ] = a.lp[ 4 ];
        a.lp[ 4 ] = a.lp[ 5 ];
        a.lp[ 5 ] = a.lp[ 6 ];
        a.lp[ 6 ] = * ( S16* ) xlin;

        a.hp[ 0 ] = a.hp[ 1 ];
        a.hp[ 1 ] = a.hp[ 2 ];
        a.hp[ 2 ] = a.hp[ 3 ];
        a.hp[ 3 ] = a.hp[ 4 ];
        a.hp[ 4 ] = a.hp[ 5 ];
        a.hp[ 5 ] = a.hp[ 6 ];
        a.hp[ 6 ] = a.hp[ 7 ];
        a.hp[ 7 ] = * ( S16* ) xhin;

        xlin += next;
        xhin += next;
      }
    }
    else
    {
      // do a zero row

      // do groups of four pixels
      while(  x-- )
      {
        S32 e1, o1, e2, o2, e3, o3, e4, o4;

        e1 =
          (     ( a.lp[ 0 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 1 + 1 ] ) * 2667 )
          );

        o1 =
          ( (   ( a.lp[ 0 + 1 ] + a.lp[ 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 2 + 1 ] ) * 4230 )
          );

        e2 =
          (     ( a.lp[ 0 + 1 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 1 ] + a.lp[ 1 + 1 + 1 ] ) * 2667 )
          );

        o2 =
          ( (   ( a.lp[ 0 + 1 + 1 ] + a.lp[ 1 + 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 1 ] + a.lp[ 2 + 1 + 1 ] ) * 4230 )
          );

        e3 =
          (     ( a.lp[ 0 + 1 + 2 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 2 ] + a.lp[ 1 + 1 + 2 ] ) * 2667 )
          );

        o3 =
          ( (   ( a.lp[ 0 + 1 + 2 ] + a.lp[ 1 + 1 + 2 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 2 ] + a.lp[ 2 + 1 + 2 ] ) * 4230 )
          );

        e4 =
          (     ( a.lp[ 0 + 1 + 3 ] * 51674 )
            - ( ( a.lp[ -1 + 1 + 3 ] + a.lp[ 1 + 1 + 3 ] ) * 2667 )
          );

        o4 =
          ( (   ( a.lp[ 0 + 1 + 3 ] + a.lp[ 1 + 1 + 3 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 + 3 ] + a.lp[ 2 + 1 + 3 ] ) * 4230 )
          );


        // round up and truncate back to 16-bit
        e1 = ( e1 + ( 32767 ^ ( e1 >> 31 ) ) ) / 65536;
        o1 = ( o1 + ( 32767 ^ ( o1 >> 31 ) ) ) / 65536;
        e2 = ( e2 + ( 32767 ^ ( e2 >> 31 ) ) ) / 65536;
        o2 = ( o2 + ( 32767 ^ ( o2 >> 31 ) ) ) / 65536;
        e3 = ( e3 + ( 32767 ^ ( e3 >> 31 ) ) ) / 65536;
        o3 = ( o3 + ( 32767 ^ ( o3 >> 31 ) ) ) / 65536;
        e4 = ( e4 + ( 32767 ^ ( e4 >> 31 ) ) ) / 65536;
        o4 = ( o4 + ( 32767 ^ ( o4 >> 31 ) ) ) / 65536;

  #ifdef __RADLITTLEENDIAN__
        ( (S32*) xout )[ 0 ] = ( o1 << 16 ) | ( e1 & 0xffff );
        ( (S32*) xout )[ 1 ] = ( o2 << 16 ) | ( e2 & 0xffff );
        ( (S32*) xout )[ 2 ] = ( o3 << 16 ) | ( e3 & 0xffff );
        ( (S32*) xout )[ 3 ] = ( o4 << 16 ) | ( e4 & 0xffff );
  #else
        ( (S32*) xout )[ 0 ] = ( e1 << 16 ) | ( o1 & 0xffff );
        ( (S32*) xout )[ 1 ] = ( e2 << 16 ) | ( o2 & 0xffff );
        ( (S32*) xout )[ 2 ] = ( e3 << 16 ) | ( o3 & 0xffff );
        ( (S32*) xout )[ 3 ] = ( e4 << 16 ) | ( o4 & 0xffff );
  #endif

        xout += 16;

        ( ( S32* ) &a.lp[ 0 ] )[ 0 ] = ( ( S32* ) ( &a.lp[ 4 ] ) )[ 0 ];
        a.lp[ 2 ] = a.lp[ 6 ];
        a.lp[ 3 ] = * ( S16* ) xlin;  // can't dword copy since these two addresses are unaligned
        a.lp[ 4 ] = * ( S16* ) ( xlin + 2 );
        a.lp[ 5 ] = * ( S16* ) ( xlin + 4 );
        a.lp[ 6 ] = * ( S16* ) ( xlin + 6 );

        xlin += 8;
      }

      // do the remenants
      x = ( halfwidth < 8 ) ? halfwidth : ( ( halfwidth & 3 ) + 8 );

      while ( x-- )
      {
        S32 e, o;

        if ( xlin == hin )
        {
          xlin = xlin - 2;
          next = -2;
        }

        e =
          (     ( a.lp[ 0 + 1 ] * 51674 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 1 + 1 ] ) * 2667 )
          );

        o =
          ( (   ( a.lp[ 0 + 1 ] + a.lp[ 1 + 1 ] ) * 27400 )
            - ( ( a.lp[ -1 + 1 ] + a.lp[ 2 + 1 ] ) * 4230 )
          );

        // round up and truncate back to 16-bit
        e = ( e + ( 32767 ^ ( e >> 31 ) ) ) / 65536;
        o = ( o + ( 32767 ^ ( o >> 31 ) ) ) / 65536;

  #ifdef __RADLITTLEENDIAN__
        ( (S32*) xout )[ 0 ] = ( o << 16 ) | ( e & 0xffff );
  #else
        ( (S32*) xout )[ 0 ] = ( e << 16 ) | ( o & 0xffff );
  #endif

        xout += 2 + 2;

        a.lp[ 0 ] = a.lp[ 1 ];
        a.lp[ 1 ] = a.lp[ 2 ];
        a.lp[ 2 ] = a.lp[ 3 ];
        a.lp[ 3 ] = a.lp[ 4 ];
        a.lp[ 4 ] = a.lp[ 5 ];
        a.lp[ 5 ] = a.lp[ 6 ];
        a.lp[ 6 ] = * ( S16* ) xlin;

        xlin += next;
      }
    }

    out += pitch;
    hin += ppitch;
    lin += ppitch;
    ++row_mask;
  }
}


// inverse transforms a column with the DWT (this routine is unrolled to do 4 columns at once)
static void iDWTcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 x;
  char *out;
  char const *lin;
  char const *hin;
  char const *lend;
  U32 halfheight;

  radassert( height >= SMALLEST_DWT_COL );

  halfheight = subheight / 2;

  out = (char*) dest;
  lin = (char const *) input;
  hin = lin + ppitch;
  lend = lin + ( ppitch * height );

  ppitch *= 2;

  if ( starty )
  {
    out += starty * pitch;
    lin += ( ( starty / 2 ) - 1 ) * ppitch;
    hin += ( ( starty / 2 ) - 2 ) * ppitch;
  }

  for( x = ( width / 4 ) ; x-- ; )
  {
    U32 y;
    S32 next;
    char *yout;
    char const *yhin;
    char const *ylin;

    struct
    {
      ALIGN_8_TYPE align;
      S16 lp[ 16 + 4 ][ 4 ];
      S16 hp[ 16 + 5 ][ 4 ];
    } a;
    S16 ( * lp )[ 4 ];
    S16 ( * hp )[ 4 ];

    lp = &a.lp[ 0 ];
    hp = &a.hp[ 0 ];

    next = ppitch;

    yout = out;
    ylin = lin;
    yhin = hin;

    if ( starty )
    {
      MOVE_8( &a.lp[ -1 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8( &a.hp[ -2 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8( &a.lp[ 0 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8( &a.hp[ -1 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8( &a.lp[ 1 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8( &a.hp[ 0 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8( &a.lp[ 2 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8( &a.hp[ 1 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8( &a.hp[ 2 + 2 ][ 0 ], yhin );
      yhin += next;
    }
    else
    {
      MOVE_8( &a.lp[ 0 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8_d( &a.lp[ -1 + 1 ][ 0 ], &a.lp[ 1 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8( &a.lp[ 2 + 1 ][ 0 ], ylin );
      ylin += next;
      MOVE_8_d( &a.hp[ -1 + 2 ][ 0 ], &a.hp[ 0 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8_d( &a.hp[ -2 + 2 ][ 0 ], &a.hp[ 1 + 2 ][ 0 ], yhin );
      yhin += next;
      MOVE_8( &a.hp[ 2 + 2 ][ 0 ], yhin );
      yhin += next;
    }

    for( y = halfheight ; y-- ; )
    {
      S32 e1, o1, e2, o2, e3, o3, e4, o4;


      if ( ylin == lend )
      {
        ylin = ylin - next;
        yhin = yhin - next - next;
        next = -next;
      }

      e1 =
        (     ( ( S32 ) lp[ 0 + 1 ][ 0 ] * 51674 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 0 ] + ( S32 ) lp[ 1 + 1 ][ 0 ] ) * 2667 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 0 ] + ( S32 ) hp[ 1 + 2 ][ 0 ] ) * 1563 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 0 ] + ( S32 ) hp[ 0 + 2 ][ 0 ] ) * 24733 )
        );

      e2 =
        (     ( ( S32 ) lp[ 0 + 1 ][ 1 ] * 51674 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 1 ] + ( S32 ) lp[ 1 + 1 ][ 1 ] ) * 2667 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 1 ] + ( S32 ) hp[ 1 + 2 ][ 1 ] ) * 1563 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 1 ] + ( S32 ) hp[ 0 + 2 ][ 1 ] ) * 24733 )
        );

      e3 =
        (     ( ( S32 ) lp[ 0 + 1 ][ 2 ] * 51674 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 2 ] + ( S32 ) lp[ 1 + 1 ][ 2 ] ) * 2667 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 2 ] + ( S32 ) hp[ 1 + 2 ][ 2 ] ) * 1563 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 2 ] + ( S32 ) hp[ 0 + 2 ][ 2 ] ) * 24733 )
        );

      e4 =
        (     ( ( S32 ) lp[ 0 + 1 ][ 3 ] * 51674 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 3 ] + ( S32 ) lp[ 1 + 1 ][ 3 ] ) * 2667 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 3 ] + ( S32 ) hp[ 1 + 2 ][ 3 ] ) * 1563 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 3 ] + ( S32 ) hp[ 0 + 2 ][ 3 ] ) * 24733 )
        );

      o1 =
        ( (   ( ( S32 ) lp[ 0 + 1 ][ 0 ] + ( S32 ) lp[ 1 + 1 ][ 0 ] ) * 27400 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 0 ] + ( S32 ) lp[ 2 + 1 ][ 0 ] ) * 4230 )
          -   ( ( S32 ) hp[ 0 + 2 ][ 0 ] * 55882 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 0 ] + ( S32 ) hp[ 2 + 2 ][ 0 ] ) * 2479 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 0 ] + ( S32 ) hp[ 1 + 2 ][ 0 ] ) * 7250 )
        );

      o2 =
        ( (   ( ( S32 ) lp[ 0 + 1 ][ 1 ] + ( S32 ) lp[ 1 + 1 ][ 1 ] ) * 27400 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 1 ] + ( S32 ) lp[ 2 + 1 ][ 1 ] ) * 4230 )
          -   ( ( S32 ) hp[ 0 + 2 ][ 1 ] * 55882 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 1 ] + ( S32 ) hp[ 2 + 2 ][ 1 ] ) * 2479 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 1 ] + ( S32 ) hp[ 1 + 2 ][ 1 ] ) * 7250 )
        );

      o3 =
        ( (   ( ( S32 ) lp[ 0 + 1 ][ 2 ] + ( S32 ) lp[ 1 + 1 ][ 2 ] ) * 27400 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 2 ] + ( S32 ) lp[ 2 + 1 ][ 2 ] ) * 4230 )
          -   ( ( S32 ) hp[ 0 + 2 ][ 2 ] * 55882 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 2 ] + ( S32 ) hp[ 2 + 2 ][ 2 ] ) * 2479 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 2 ] + ( S32 ) hp[ 1 + 2 ][ 2 ] ) * 7250 )
        );

      o4 =
        ( (   ( ( S32 ) lp[ 0 + 1 ][ 3 ] + ( S32 ) lp[ 1 + 1 ][ 3 ] ) * 27400 )
          - ( ( ( S32 ) lp[ -1 + 1 ][ 3 ] + ( S32 ) lp[ 2 + 1 ][ 3 ] ) * 4230 )
          -   ( ( S32 ) hp[ 0 + 2 ][ 3 ] * 55882 )
          - ( ( ( S32 ) hp[ -2 + 2 ][ 3 ] + ( S32 ) hp[ 2 + 2 ][ 3 ] ) * 2479 )
          + ( ( ( S32 ) hp[ -1 + 2 ][ 3 ] + ( S32 ) hp[ 1 + 2 ][ 3 ] ) * 7250 )
        );

      // round up and truncate back to 16-bit
      e1 = ( e1 + ( 32767 ^ ( e1 >> 31 ) ) ) / 65536;
      e2 = ( e2 + ( 32767 ^ ( e2 >> 31 ) ) ) / 65536;
      e3 = ( e3 + ( 32767 ^ ( e3 >> 31 ) ) ) / 65536;
      e4 = ( e4 + ( 32767 ^ ( e4 >> 31 ) ) ) / 65536;

      o1 = ( o1 + ( 32767 ^ ( o1 >> 31 ) ) ) / 65536;
      o2 = ( o2 + ( 32767 ^ ( o2 >> 31 ) ) ) / 65536;
      o3 = ( o3 + ( 32767 ^ ( o3 >> 31 ) ) ) / 65536;
      o4 = ( o4 + ( 32767 ^ ( o4 >> 31 ) ) ) / 65536;


#ifdef __RADLITTLEENDIAN__
      ( (S32*) yout )[ 0 ] = ( e2 << 16 ) | ( e1 & 0xffff );
      ( (S32*) yout )[ 1 ] = ( e4 << 16 ) | ( e3 & 0xffff );
      ( (S32*) ( yout + pitch ) )[ 0 ] = ( o2 << 16 ) | ( o1 & 0xffff );
      ( (S32*) ( yout + pitch ) )[ 1 ] = ( o4 << 16 ) | ( o3 & 0xffff );
#else
      ( (S32*) yout )[ 0 ] = ( e1 << 16 ) | ( e2 & 0xffff );
      ( (S32*) yout )[ 1 ] = ( e3 << 16 ) | ( e4 & 0xffff );
      ( (S32*) ( yout + pitch ) )[ 0 ] = ( o1 << 16 ) | ( o2 & 0xffff );
      ( (S32*) ( yout + pitch ) )[ 1 ] = ( o3 << 16 ) | ( o4 & 0xffff );
#endif

      yout += pitch + pitch;

      lp++;
      hp++;

      // if we've hit the end of our local buffer, re-center at the beginning of the buffer
      if ( &lp[ 3 ][ 0 ] == &a.hp[ 0 ][ 0 ] )
      {
        MOVE_8( &a.lp[ 0 ][ 0 ], &lp[ 1 ][ 0 ] );
        MOVE_8( &a.lp[ 1 ][ 0 ], &lp[ 2 ][ 0 ] );
        MOVE_8( &a.lp[ 2 ][ 0 ], &lp[ 3 ][ 0 ] );
        MOVE_8( &a.hp[ 0 ][ 0 ], &hp[ 1 ][ 0 ] );
        MOVE_8( &a.hp[ 1 ][ 0 ], &hp[ 2 ][ 0 ] );
        MOVE_8( &a.hp[ 2 ][ 0 ], &hp[ 3 ][ 0 ] );
        MOVE_8( &a.hp[ 3 ][ 0 ], &hp[ 4 ][ 0 ] );
        lp = &a.lp[ 0 ];
        hp = &a.hp[ 0 ];
      }

      radassert( ylin >= lin );
      radassert( yhin >= hin );

      radassert( ylin < ( lin + ( height * ppitch ) ) );
      radassert( yhin < ( hin + ( height * ppitch ) ) );

      MOVE_8( &lp[ 3 ][ 0 ], ylin );
      MOVE_8( &hp[ 4 ][ 0 ], yhin );

      ylin += next;
      yhin += next;
    }

    out += 8;
    hin += 8;
    lin += 8;
    lend += 8;
  }

  // do remaining columns
  for( x = ( width & 3 ) ; x-- ; )
  {
    U32 y;
    S32 next;
    char *yout;
    char const *ylin;
    char const *yhin;

    S32 lp[ 4 ];
    S32 hp[ 5 ];

    next = ppitch;

    yout = out;
    ylin = lin;
    yhin = hin;

    if ( starty )
    {
      lp[ -1 + 1 ] = * ( S16* ) ylin;
      ylin += next;
      lp[ 0 + 1 ] = * ( S16* ) ylin;
      ylin += next;
      lp[ 1 + 1 ] = * ( S16* ) ylin;
      ylin += next;
      lp[ 2 + 1 ] = * ( S16* ) ylin;
      ylin += next;

      hp[ -2 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ -1 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ 0 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ 1 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ 2 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
    }
    else
    {
      lp[ 0 + 1 ] = * ( S16* ) ylin;
      ylin += next;
      lp[ -1 + 1 ] = lp[ 1 + 1 ] = * ( S16* ) ylin;
      ylin += next;
      lp[ 2 + 1 ] = * ( S16* ) ylin;
      ylin += next;

      hp[ -1 + 2 ] = hp[ 0 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ -2 + 2 ] = hp[ 1 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
      hp[ 2 + 2 ] =  * ( S16* ) yhin;
      yhin += next;
    }

    for( y = halfheight ; y-- ; )
    {
      S32 e, o;

      if ( ylin == lend )
      {
        ylin = ylin - next;
        yhin = yhin - next - next;
        next = -next;
      }

      e =
        (     ( lp[ 0 + 1 ] * 51674 )
          - ( ( lp[ -1 + 1 ] + lp[ 1 + 1 ] ) * 2667 )
          - ( ( hp[ -2 + 2 ] + hp[ 1 + 2 ] ) * 1563 )
          + ( ( hp[ -1 + 2 ] + hp[ 0 + 2 ] ) * 24733 )
        );

      o =
        ( (   ( lp[ 0 + 1 ] + lp[ 1 + 1 ] ) * 27400 )
          - ( ( lp[ -1 + 1 ] + lp[ 2 + 1 ] ) * 4230 )
          -   ( hp[ 0 + 2 ] * 55882 )
          - ( ( hp[ -2 + 2 ] + hp[ 2 + 2 ] ) * 2479 )
          + ( ( hp[ -1 + 2 ] + hp[ 1 + 2 ] ) * 7250 )
        );

      // round up and truncate back to 16-bit
      e = ( e + ( 32767 ^ ( e >> 31 ) ) ) / 65536;
      o = ( o + ( 32767 ^ ( o >> 31 ) ) ) / 65536;

      ( (S16*) yout )[ 0 ] = ( S16 ) e;
      ( (S16*) ( yout + pitch ) )[ 0 ] = ( S16 ) o;

      yout += pitch + pitch;

      lp[ 0 ] = lp[ 1 ];
      lp[ 1 ] = lp[ 2 ];
      lp[ 2 ] = lp[ 3 ];
      lp[ 3 ] = * ( S16* ) ylin;

      hp[ 0 ] = hp[ 1 ];
      hp[ 1 ] = hp[ 2 ];
      hp[ 2 ] = hp[ 3 ];
      hp[ 3 ] = hp[ 4 ];
      hp[ 4 ] = * ( S16* ) yhin;

      ylin += next;
      yhin += next;
    }

    out += 2;
    hin += 2;
    lin += 2;
    lend += 2;
  }
}


// inverse transforms a row with the Harr
static void iHarrrow( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, S32 width, S32 height, U8 const * row_mask, S32 starty, S32 subheight )
{
  U32 y;
  char *out, *hin, *lin;
  U32 halfwidth;

  radassert( ( ( width & 1 ) == 0 ) );

  halfwidth = width / 2;

  out = (char*) dest;
  lin = (char*) in;
  hin = ( (char*) in ) + width;  // really 2 * halfwidth (which is just width)

  out += starty * pitch;
  hin += starty * ppitch;
  lin += starty * ppitch;
  row_mask += starty;

  for( y = subheight ; y-- ; )
  {
    U32 x;
    char *xout, *xhin, *xlin;

    xout = out;
    xlin = lin;
    xhin = hin;

    if ( ( ( ( UINTADDR ) row_mask ) <= (U32)height ) || ( *row_mask ) )
    {
      for( x = halfwidth ; x-- ; )
      {
        S32 e, o;

        e = ( (S32) ( ( S16* ) xlin )[ 0 ] * 2 + (S32) ( ( S16* ) xhin )[ 0 ] );
        o = ( (S32) ( ( S16* ) xlin )[ 0 ] * 2 - (S32) ( ( S16* ) xhin )[ 0 ] );

        // round up by 1
        e = ( e + ( 1 ^ ( e >> 31 ) ) ) / 2;
        o = ( o + ( 1 ^ ( o >> 31 ) ) ) / 2;

  #ifdef __RADLITTLEENDIAN__
        ( (S32*) xout )[ 0 ] = ( o << 16 ) | ( e & 0xffff );
  #else
        ( (S32*) xout )[ 0 ] = ( e << 16 ) | ( o & 0xffff );
  #endif

        xout += 2 + 2;

        radassert( xlin >= lin );
        radassert( xhin >= ( hin - 2) );
        radassert( xlin < ( lin + width ) );
        radassert( xhin < ( hin + width ) );

        xlin += 2;
        xhin += 2;
      }
    }
    else
    {
      // do a zero row

      for( x = halfwidth ; x-- ; )
      {
        S32 e = ( ( S16* ) xlin )[ 0 ];

        ( (S32*) xout )[ 0 ] = ( e << 16 ) | ( e & 0xffff );

        xout += 2 + 2;

        radassert( xlin >= lin );
        radassert( xlin < ( lin + width ) );

        xlin += 2;
      }
    }

    out += pitch;
    hin += ppitch;
    lin += ppitch;
    ++row_mask;
  }
}


// inverse transforms a column with the Harr
static void iHarrcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, S32 starty, S32 subheight )
{
  U32 x;
  char *out;
  char const *lin;
  char const *hin;
  U32 halfheight;

  radassert( ( ( height & 1 ) == 0 ) );

  halfheight = subheight / 2;

  out = (char*) dest;
  lin = (char const *) input;
  hin = lin + ppitch;

  ppitch *= 2;

  if ( starty )
  {
    out += starty * pitch;
    lin += ( starty / 2 ) * ppitch;
    hin += ( starty / 2 ) * ppitch;
  }

  for( x = width ; x-- ; )
  {
    U32 y;
    char *yout;
    char const *ylin;
    char const *yhin;

    yout = out;
    ylin = lin;
    yhin = hin;

    for( y = halfheight ; y-- ; )
    {
      S32 e, o;

      e = ( (S32) ( ( S16* ) ylin )[ 0 ] * 2 + (S32) ( ( S16* ) yhin )[ 0 ] );
      o = ( (S32) ( ( S16* ) ylin )[ 0 ] * 2 - (S32) ( ( S16* ) yhin )[ 0 ] );

      // round up by 1
      e = ( e + ( 1 ^ ( e >> 31 ) ) ) / 2;
      o = ( o + ( 1 ^ ( o >> 31 ) ) ) / 2;

      ( (S16*) yout )[ 0 ] = ( S16 ) e;
      ( (S16*) ( yout + pitch ) )[ 0 ] = ( S16 ) o;

      yout += pitch + pitch;
      ylin += ppitch;
      yhin += ppitch;
    }

    out += 2;
    hin += 2;
    lin += 2;
  }
}


typedef void (*IWAVEROW)( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, S32 width, S32 height, U8 const * row_mask, S32 starty, S32 subheight );
typedef void (*IWAVECOL)( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, S32 width, S32 height, S32 starty, S32 subheight );

// does a inverse 2D wavelet in place (using DWT until too small and then flipping to Harr)
// this routine alternates between rows and columns to keep everything in the cache
static void iDWT2D( S16* output, S32 pitch, S32 width, S32 height, U8 const * row_mask, S16* temp )
{
  #define FLIPSIZE 8

  U32 rh, ch, ry, cy;

  IWAVEROW row = ( width >= SMALLEST_DWT_ROW ) ? iDWTrow : iHarrrow;
  IWAVECOL col = ( height >= SMALLEST_DWT_COL ) ? iDWTcol : iHarrcol;

  ry = ( height <= ( FLIPSIZE + 4 + 4 ) ) ? height : ( FLIPSIZE + 4 );
  rh = height - ry;
  cy = 0;
  ch = height;

  row( temp, pitch, output, pitch, width, height, row_mask, 0, ry );

  do
  {
    U32 next;

      next = ( ch <= ( FLIPSIZE + 4 ) ) ? ch : FLIPSIZE;

    col( output, pitch, temp, pitch, width, height, cy, next );
    cy += next;
    ch -= next;

    if ( rh )
    {
      next = ( rh <= ( FLIPSIZE + 4 ) ) ? rh : FLIPSIZE;
      row( temp, pitch, output, pitch, width, height, row_mask, ry, next );
      ry += next;
      rh -= next;
    }

  } while ( ch );

  #undef FLIPSIZE
}

/* ========================================================================
   This is the glue code
   ======================================================================== */
uint32 GRANNY
ToBinkTC0(void *output,
          uint32 compress_to,
          int16 **input,
          real32 const *plane_weights,
          uint32 planes,
          uint32 width,
          uint32 height,
          void *temp, uint32 temp_size )
{
    return(to_BinkTC(output, compress_to, input, plane_weights, planes,
                     width, height, temp, temp_size));
}

uint32 GRANNY
ToBinkTCTempMem0(uint32 width, uint32 height)
{
    return(to_BinkTC_temp_mem(width, height));
}

uint32 GRANNY
ToBinkTCOutputMem0(uint32 width, uint32 height,
                   uint32 planes, uint32 compress_to)
{
    return(to_BinkTC_output_mem(width, height, planes, compress_to));
}

void GRANNY
BinkTCCheckSizes0(uint32 *width, uint32 *height)
{
    BinkTC_check_sizes(width, height);
}

void GRANNY
FromBinkTC0(int16 **output,
            uint32 planes,
            void const * bink_buf,
            uint32 width,
            uint32 height,
            void *temp,
            uint32 temp_size)
{
    from_BinkTC(output, planes, bink_buf, width, height, temp, temp_size);
}

uint32 GRANNY
FromBinkTCTempMem0(void const *binkbuf)
{
    return(from_BinkTC_temp_mem(binkbuf));
}
