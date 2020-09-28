#include "radbase.h"


// Do we want encoder support?
// I'm guessing nobody would try to do this on a console.
// And the PS2 hates the code anyway (float64 = software emulation)
#if (defined(__RADNGC__) || defined(__RADXBOX__) || defined(__RADPS2__) ||  \
     defined(__RADPSP__) || defined(__RADPS3__) || defined(__RADLINUX__) || \
     defined(__RADXENON__))
#define INCLUDE_ENCODERS 0
#else
#define INCLUDE_ENCODERS 1
#endif



#define ENCODE_MIN_PLANE_SIZE (64 + 8)



#if INCLUDE_ENCODERS


// encode the decomposed planes
U32 planes_encode( void* output,
                   U32 tobytes,
                   S16 const * const * input,
                   F32 const * plane_weights,
                   U32 planes,
                   U32 width,
                   U32 height,
                   void * temp, U32 temp_size,
                   U32 * decomp_min_size );

#endif //#if INCLUDE_ENCODERS


// decode from the compressed data into a decomposed plane
U32 plane_decode( void const * comp,
                  S16 * output,
                  U32 width,
                  U32 height,
                  U8 * row_mask,
                  void * temp );
