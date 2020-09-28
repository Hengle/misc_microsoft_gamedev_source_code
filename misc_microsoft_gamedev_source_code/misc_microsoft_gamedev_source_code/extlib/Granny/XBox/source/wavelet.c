#include "radbase.h"
#include "wavelet.h"

#include "raddebug.h"

// uncomment this comment to break when writing unaligned - this is slow!
//#define INSURE_ALIGNMENT

#ifdef __RADPPC__

#ifdef __RAD64__

#define MOVE_8( d, s )                                      \
        ( ( U64 * )( d ) )[ 0 ] = ( ( U64* ) ( s ) )[ 0 ];

#define MOVE_8_d( d1, d2, s )                               \
        ( ( U64 * )( d1 ) )[ 0 ] = ( ( U64 * )( d2 ) )[ 0 ] = ( ( U64* ) ( s ) )[ 0 ];

#else

#define MOVE_8( d, s )                                      \
        ( ( F64 * )( d ) )[ 0 ] = ( ( F64* ) ( s ) )[ 0 ];

#define MOVE_8_d( d1, d2, s )                               \
        ( ( F64 * )( d1 ) )[ 0 ] = ( ( F64 * )( d2 ) )[ 0 ] = ( ( F64* ) ( s ) )[ 0 ];

#endif

#define MOVE_8_4a( d, s )                                   \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d_4a( d1, d2, s )                                                        \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#else

#ifdef INSURE_ALIGNMENT

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

#ifdef __RAD64__

#define MOVE_8( d, s )                                      \
        ( ( U64 * )( d ) )[ 0 ] = ( ( U64* ) ( s ) )[ 0 ];

#define MOVE_8_d( d1, d2, s )                               \
        ( ( U64 * )( d1 ) )[ 0 ] = ( ( U64 * )( d2 ) )[ 0 ] = ( ( U64* ) ( s ) )[ 0 ];

#else

#define MOVE_8( d, s )                                      \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d( d1, d2, s )                                                           \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#endif

#define MOVE_8_4a( d, s )                                   \
      { ( ( U32 * )( d ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#define MOVE_8_d_4a( d1, d2, s )                                                        \
      { ( ( U32 * )( d1 ) )[ 0 ] = ( ( U32 * )( d2 ) )[ 0 ] = ( ( U32* ) ( s ) )[ 0 ];  \
        ( ( U32 * )( d1 ) )[ 1 ] = ( ( U32 * )( d2 ) )[ 1 ] = ( ( U32* ) ( s ) )[ 1 ]; }

#endif

#endif


// forward transforms a row with the DWT
void fDWTrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight )
{
  U32 y;
  char *in, *hout, *lout, *end;
  RAD_UNUSED_VARIABLE(height);

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
    S32 next;

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
        next = (S32)-2;
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
void fDWTcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight )
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
void fHarrrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight )
{
  U32 y;
  char const * in;
  char * hout, *lout;
  RAD_UNUSED_VARIABLE(height);

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
void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight )
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
      ( *(S16*) yout ) = (S16) ( ( (S32) ( ( S16* ) yin )[ 0 ] + (S32) ( ( S16* ) ( yin + inpitch ) )[ 0 ] ) / 2 );
      yout += pitch;
      ( *(S16*) yout ) = ( ( ( S16* ) yin )[ 0 ] - ( ( S16* ) ( yin + inpitch ) )[ 0 ] );
      yout += pitch;

      yin += inpitch + inpitch;
    }

    in += 2;
    out += 2;
  }
}


typedef void (*WAVEROW)( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );
typedef void (*WAVECOL)( S16* dest, S32 pitch,  S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );


// does a forward 2D wavelet in place (using DWT until too small and then flipping to Harr)
// this routine alternates between columns and rows to keep everything in the cache
void fDWT2D( S16* input, S32 pitch, U32 width, U32 height, S16* temp )
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
void iDWTrow( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, U32 width, U32 height, U8 const * row_mask, S32 starty, S32 subheight )
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

    RAD_ALIGN( struct
    {
      S16 lp[ 4 + 3 + 1 ];  // plus 1 to maintain hp's alignment
      S16 hp[ 5 + 3 ];
    }, a, 8 );

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

    if ( ( ( ( UINTADDR ) row_mask ) <= height ) || ( *row_mask ) )
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
void iDWTcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, S32 starty, S32 subheight )
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

    RAD_ALIGN( struct
    {
      S16 lp[ 16 + 4 ][ 4 ];
      S16 hp[ 16 + 5 ][ 4 ];
    }, a, 8 );
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
void iHarrrow( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, U32 width, U32 height, U8 const * row_mask, S32 starty, S32 subheight )
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

    if ( ( ( ( UINTADDR ) row_mask ) <= height ) || ( *row_mask ) )
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
void iHarrcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, S32 starty, S32 subheight )
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


typedef void (*IWAVEROW)( S16 * dest, S32 pitch, S16 const * in, S32 ppitch, U32 width, U32 height, U8 const * row_mask, S32 starty, S32 subheight );
typedef void (*IWAVECOL)( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, S32 starty, S32 subheight );

// does a inverse 2D wavelet in place (using DWT until too small and then flipping to Harr)
// this routine alternates between rows and columns to keep everything in the cache
void iDWT2D( S16* output, S32 pitch, U32 width, U32 height, U8 const * row_mask, S16* temp )
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
