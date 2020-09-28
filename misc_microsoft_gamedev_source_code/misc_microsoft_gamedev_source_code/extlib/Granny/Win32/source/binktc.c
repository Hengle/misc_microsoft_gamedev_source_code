//=========================================================================
// BinkTC routines - Jeff Roberts 10/22/01
//=========================================================================

#include "radbase.h"
#include "radmem.h"
#include "radmath.h"

#include "binktc.h"
#include "wavelet.h"
#include "encode.h"

#include <stdio.h>

#define fWave2D fDWT2D
#define iWave2D iDWT2D
#define iWave2D_mask iDWT2D_mask



// how much temp memory to we need during compression
U32 to_BinkTC_temp_mem( U32 width, U32 height )
{
  return( width * height * 2 + ( ( ( ( width * height ) / 8 ) + 256 + 15 ) & ~15 ) + 64 * 1024 + 4096 );
}



// how much temp memory to we need during decompression
U32 from_BinkTC_temp_mem( void const * binkbuf )
{
    return( radloadu32ptr( binkbuf ) );
}


// how much output memory should we allocate
U32 to_BinkTC_output_mem( U32 width, U32 height, U32 planes, U32 compress_to )
{
  U32 out_size;
  U32 i;

  // account for hard bit cost
  compress_to += ( ENCODE_MIN_PLANE_SIZE * planes );
  
  // start with twice the compress_to size
  out_size = compress_to * 3;

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
void BinkTC_check_sizes( U32 * width, U32 * height )
{
  *width =  ( *width + 15 ) & ~15;
  *height = ( *height + 15 ) & ~15;
}



//=====================================================================
// input arrays are S16 data with pixels no larger than -1024 to +1023
//=====================================================================

// compresses with Bink texture compression
U32 to_BinkTC( void* output,
               U32 tobytes,
               S16** input,
               F32 const * plane_weights,
               U32 planes,
               U32 width,
               U32 height,
               void * temp, U32 temp_size )
{


#if !INCLUDE_ENCODERS
    (void)sizeof(output);
    (void)sizeof(tobytes);
    (void)sizeof(input);
    (void)sizeof(plane_weights);
    (void)sizeof(planes);
    (void)sizeof(width);
    (void)sizeof(height);
    (void)sizeof(temp);
    (void)sizeof(temp_size);

    //Assert ( "to_BinkTC: Encoders not compiled for this platform" );
    return 0;

#else //#if !INCLUDE_ENCODERS

  U32 i;
  U32 size = 0;
  S16 * tplane;

  // make sure we have enough memory to perform the wavelets
  i = to_BinkTC_temp_mem( width, height );
  if ( ( temp == 0 ) || ( temp_size < i ) )
  {
    temp = 0;
    tplane = (S16*) radmalloc( i );
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
                        (S16 const **)input,
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

#endif //#else //#if !INCLUDE_ENCODERS

}


//=========================================================================
// input is BinkTC formatted data - output is the S16 planes
//=========================================================================

// decompress the Bink texture compressed block
void from_BinkTC( S16** output, U32 planes, void const * bink_buf, U32 width, U32 height, void * temp, U32 temp_size )
{
  U32 i;
  S16 * tplane;
  U8 * row;

  // make sure we have enough memory to decompress
  i = from_BinkTC_temp_mem( bink_buf );
  if ( ( temp == 0 ) || ( temp_size < i ) )
  {
    temp = 0;
    row = (U8*) radmalloc( i );
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
