#ifndef __RADARITHH__
#define __RADARITHH__

#include "arithbit.h"

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

//#define TESTPROBUSAGE

#ifdef TESTPROBUSAGE
  typedef struct RADIPROBS* RADIPROB;

  // Combine two probabilities.
  RADDEFFUNC RADIPROB Arith_combine_probabilities( RADIPROB iprob1, RADIPROB iprob2 );
#else
  #define RADIPROB F32

  #define Arith_combine_probabilities( a, b )  (a) * (b)
#endif
  
// Return the inverted probabilty of a symbol.
RADDEFFUNC RADIPROB Arith_inv_probability( ARITH a, U32 value, U32 scale );

// Convert a probability to number of bits.
RADDEFFUNC F32 Arith_inv_probability_to_bits( RADIPROB iprob );

// Increments the internal probably of a symbol by one.
RADDEFFUNC void Arith_adjust_probability( ARITH a, U32 value, S32 count );

// Tells whether the last symbol returned from Arith_compress or Arith_decompress
//   was an escape.
#define Arith_was_escaped( val ) ( ( ( UINTADDR ) ( val ) ) > 65536 )

// Uses to update the decoder with the escaped symbol after you decode it.
#define Arith_set_decompressed_symbol( val, value ) ( * ( ( U16 * ) ( val ) ) ) = ( U16 ) ( value );

#endif

