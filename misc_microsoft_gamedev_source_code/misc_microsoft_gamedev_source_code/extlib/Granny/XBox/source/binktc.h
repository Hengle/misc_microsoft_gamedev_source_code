#include "radbase.h"

//=========================================================================
// input arrays are S16 data with pixels inside -1024 to 1023
//=========================================================================

// compresses with Bink texture compression
U32 to_BinkTC( void* output,
               U32 compress_to,
               S16** input,
               F32 const * plane_weights,
               U32 planes,
               U32 width,
               U32 height,
               void * temp, U32 temp_size );

// tells how much temp memory to allocate for compression
U32 to_BinkTC_temp_mem( U32 width, U32 height );

// tells how much output memory to allocate
U32 to_BinkTC_output_mem( U32 width, U32 height, U32 planes, U32 compress_to );

// check the width and height for validity (usually just make divisible by 16)
void BinkTC_check_sizes( U32 * width, U32 * height );

S32 BinkTC_should_compress( U32 width, U32 height );




//=========================================================================
// input is BinkTC formatted data - output are the S16 planes
//=========================================================================

// decompresses a Bink compressed memory block
void from_BinkTC( S16** output,
                  U32 planes,
                  void const * bink_buf,
                  U32 width,
                  U32 height,
                  void * temp,
                  U32 temp_size );

// tells how much temp memory to allocate for decompression
U32 from_BinkTC_temp_mem( void const * binkbuf );

