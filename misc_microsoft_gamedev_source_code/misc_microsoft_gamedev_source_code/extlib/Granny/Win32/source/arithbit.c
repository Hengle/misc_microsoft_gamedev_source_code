#include "arithbit.h"
#include "raddebug.h"

// this is a byte aligned arithmetic encoder (goofyily called a range encoder)


// we start the ptr one value early, because we'll always increment once before the next byte
//   (since we know that we'll never carry, this is safe)

RADDEFFUNC void ArithBitsPutStart( ARITHBITS* ab, void * ptr )
{
  ab->low = 0;
  ab->range = 0x80000000;
  ab->underflow = 0;
  ab->ptr = ( (U8*) ptr ) - 1;
  ab->start = (U8*) ptr;
}

#define handle_underflow( value ) \
if ( ab->underflow ) \
{ \
  U32 i; \
  i = ab->underflow; \
  while ( i ) \
  { \
    *ab->ptr++ = value; \
    --i; \
  } \
  ab->underflow = 0; \
}

// this is a little weird, because we actually the buffered output byte *in* the output stream,
//   we might just have to increment it before moving the next byte...

static void enc_renorm( ARITHBITS* ab )
{
  while ( ab->range <= 0x0800000 )    
  {
    // can we ever affect the buffered byte?
    if ( ab->low < ( 0xFF << 23 ) )
    {
      // nope, no adjustment, so move on
      ++ab->ptr;
      handle_underflow( 255 );
      *ab->ptr = (U8) ( ab->low >> 23 );
    }
    else
    {
      // yep, but are we going to carry?
      if ( ab->low & 0x80000000 )
      {
        // yup, so increment the carry, and then move on
        ++ab->ptr[ 0 ];
        ++ab->ptr;
        handle_underflow( 0 );
        *ab->ptr = (U8) ( ab->low >> 23 );
      }
      else 
      {
        ++ab->underflow;
      }
    }

    // resize the range back up
    ab->range <<= 8;
    ab->low = ( ab->low << 8 ) & 0x7fffffff;
  }
}
 

RADDEFFUNC void ArithBitsFlush( ARITHBITS* ab )
{
  U32 tmp;

  enc_renorm( ab );

  tmp = ab->low >> 23; 

  // same thing as the put - check for overflow and an increment only if necessary
  if ( tmp > 0xff )
  {
    ++ab->ptr[ 0 ];
    ++ab->ptr;
    handle_underflow( 0 );
  }
  else 
  {
    ++ab->ptr;
    handle_underflow( 255 );
  }
  *ab->ptr++ = (U8) tmp;  
  *ab->ptr++ = (U8) ( ab->low >> 15 );  
  *ab->ptr++ = (U8) ( ab->low >> 7 );  
  *ab->ptr++ = (U8) ( ab->low << 1 );  
}


RADDEFFUNC void ArithBitsPut( ARITHBITS* ab, U32 start, U32 range, U32 scale )
{
  U32 r, tmp;

  enc_renorm( ab );

  r = ab->range / scale;
  tmp = r * start;
  
  // if there is any extra range, give it to the final symbol
  if ( ( start + range ) < scale )
    ab->range = r * range;
  else
    ab->range -= tmp;

  ab->low += tmp;
}

RADDEFFUNC void ArithBitsPutBits( ARITHBITS* ab, U32 start, U32 range, U32 bits, U32 scale )
{
  U32 r, tmp;

  enc_renorm( ab );

  r = ab->range >> bits;
  tmp = r * start;
  
  // if there is any extra range, give it to the final symbol
  if ( ( start + range ) < scale )
    ab->range = r * range;
  else
    ab->range -= tmp;

  ab->low += tmp;
}

RADDEFFUNC void ArithBitsPutBitsValue ( ARITHBITS* ab, U32 value, U32 bits, U32 scale )
{
  U32 r, tmp;

  enc_renorm( ab );

  r = ab->range >> bits;
  tmp = r * value;
  
  // if there is any extra range, give it to the final symbol
  if ( ( value + 1 ) < scale )
    ab->range = r;
  else
    ab->range -= tmp;

  ab->low += tmp;
}

RADDEFFUNC void ArithBitsPutValue ( ARITHBITS* ab, U32 value, U32 scale )
{
  U32 r, tmp;

  enc_renorm( ab );

  radassert( value < scale );

  r = ab->range / scale;
  tmp = r * value;
  
  // if there is any extra range, give it to the final symbol
  if ( ( value + 1 ) < scale )
    ab->range = r;
  else
    ab->range -= tmp;

  ab->low += tmp;
}

RADDEFFUNC U32 ArithBitsSize( ARITHBITS* ab )
{
  return (U32)( ( ( (UINTADDR) ab->ptr ) - ( (UINTADDR) ab->start ) ) * 8 );
}

//=============

RADDEFFUNC void ArithBitsGetStart( ARITHBITS* ab, void const* ptr )
{
  U32 buf;

  radassert( ( ( (UINTADDR)ptr ) & 3 ) == 0 );

  ab->ptr = ( (U8*) ptr ) + 1;
  buf = ( (U8*) ptr )[ 0 ];

  ab->start = (U8*) ( ( (UINTADDR) ptr ) + ( buf & 1 ) );
  ab->low = buf >> 1;
  ab->range = 1 << 7;
}

static void dec_renorm( ARITHBITS *ab )
{
  U32 range;

  range = ab->range;

  // is the range too small?
  if ( range <= 0x800000 )
  {
    U32 low, buf;
    U8 * ptr;

    low = ab->low;
    // we keep the last bit in the low bit of the start pointer
    buf = (U32)(( (UINTADDR) ab->start ) & 1);
    ptr = ab->ptr;

    // read the next byte and put it into the range
    do
    {
      low = ( low + low + buf ) << 7;
      buf = *ptr++;
      low |= ( buf >> 1 );
      buf &= 1;
      range <<= 8;
    } while ( range <= 0x800000 );

    ab->low = low;
    // we keep the last bit in the low bit of the start pointer
    ab->start = (U8*) ( ( ( (UINTADDR) ab->start ) & ~1 ) + buf );
    ab->range = range;
    ab->ptr = ptr;
  }
}

RADDEFFUNC U32 ArithBitsGet( ARITHBITS* ab, U32 scale )
{
  U32 tmp;

  dec_renorm( ab );

  ab->underflow = ab->range / scale;
  tmp = ab->low / ab->underflow;

  // round off goes to the last symbol
  return( ( tmp >= scale ) ? ( scale - 1 ) : tmp );
}

RADDEFFUNC void ArithBitsRemove( ARITHBITS * ab, U32 start, U32 range, U32 scale )
{
  U32 tmp;

  tmp = ab->underflow * start;
  ab->low -= tmp;

  // round off goes to the last range
  if ( ( start + range ) < scale )
    ab->range = ab->underflow * range;
  else
    ab->range -= tmp;
}

RADDEFFUNC U32 ArithBitsGetValue ( ARITHBITS * ab, U32 scale )
{
  U32 tmp, div, start;

  dec_renorm( ab );

  div = ab->range / scale;
  start = ab->low / div;

  // round off goes to the last symbol
  if ( start >= scale ) 
    start = ( scale - 1 );

  tmp = div * start;
  ab->low -= tmp;

  // round off goes to the last range
  if ( ( start + 1 ) < scale )
    ab->range = div;
  else
    ab->range -= tmp;

  return( start );
}

RADDEFFUNC U32 ArithBitsGetBits( ARITHBITS* ab, U32 bits, U32 scale )
{
  U32 tmp;

  dec_renorm( ab );

  ab->underflow = ab->range >> bits;
  tmp = ab->low / ab->underflow;

  return( ( tmp >= scale ) ? ( scale - 1 ) : tmp );
}

RADDEFFUNC U32 ArithBitsGetBitsValue ( ARITHBITS * ab, U32 bits, U32 scale )
{
  U32 tmp, div, start;

  dec_renorm( ab );

  div = ab->range >> bits;
  start = ab->low / div;

  // round off goes to the last symbol
  if ( start >= scale ) 
    start = ( scale - 1 );

  tmp = div * start;
  ab->low -= tmp;

  // round off goes to the last range
  if ( ( start + 1 ) < scale )
    ab->range = div;
  else
    ab->range -= tmp;

  return( start );
}
