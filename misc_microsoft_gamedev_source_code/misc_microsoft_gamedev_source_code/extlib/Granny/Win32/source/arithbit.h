#ifndef __ARITHBITSH__
#define __ARITHBITSH__

#include "radbase.h"

// probability bit macros

typedef struct _ARITHBITS
{
  U32 low;
  U32 range;
  U8 * ptr;
  U32 underflow;
  U8 * start;
} ARITHBITS;

RADDEFFUNC void ArithBitsPutStart( ARITHBITS* ab, void * ptr );

RADDEFFUNC void ArithBitsGetStart( ARITHBITS* ab, void const * ptr );

RADDEFFUNC U32 ArithBitsSize( ARITHBITS* ab );

RADDEFFUNC void ArithBitsFlush( ARITHBITS* ab );

RADDEFFUNC void ArithBitsPut ( ARITHBITS* ab, U32 start, U32 range, U32 scale );
RADDEFFUNC void ArithBitsPutBits( ARITHBITS* ab, U32 start, U32 range, U32 bits, U32 scale );
RADDEFFUNC void ArithBitsPutValue ( ARITHBITS* ab, U32 value, U32 scale );
RADDEFFUNC void ArithBitsPutBitsValue ( ARITHBITS* ab, U32 value, U32 bits, U32 scale );

RADDEFFUNC U32 ArithBitsGetValue ( ARITHBITS * ab, U32 scale );
RADDEFFUNC U32 ArithBitsGetBitsValue ( ARITHBITS * ab, U32 bits, U32 scale );
RADDEFFUNC U32 ArithBitsGet( ARITHBITS* ab, U32 scale );
RADDEFFUNC U32 ArithBitsGetBits( ARITHBITS* ab, U32 bits, U32 scale );

RADDEFFUNC void ArithBitsRemove ( ARITHBITS * ab, U32 start, U32 range, U32 scale );


#endif
