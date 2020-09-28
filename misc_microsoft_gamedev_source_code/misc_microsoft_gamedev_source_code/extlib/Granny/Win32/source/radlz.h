#ifndef __RADLZH__
#define __RADLZH__

#include "arithbit.h"

#define LOW_BITS 2
#define MED_BITS 8
#define TOP_BITS 8
#define LARGEST_POSSIBLE_OFFSET ( ( 1 << ( LOW_BITS + MED_BITS + TOP_BITS ) ) - 1 )

typedef struct LZCDATA * LZC;
typedef struct LZDDATA * LZD;

// How much memory will the compressor need with these values
RADDEFFUNC U32 LZ_compress_alloc_size( U32 max_byte_value,
                                       U32 uniq_byte_values,
                                       U32 max_offset );

// How much memory will the decompressor need with these values
RADDEFFUNC U32 LZ_decompress_alloc_size( U32 max_byte_value,
                                         U32 uniq_byte_values,
                                         U32 max_offset );


// How much memory will the decompressor need with this header
RADDEFFUNC U32 LZ_decompress_alloc_size_from_header( void * header );


// How much temp memory is needed?  temp memory will only be accessed inside
//   LZ_compress and the first 16-bit short will be set to zero whenever it
//   was used.
RADDEFFUNC U32 LZ_compress_temp_size( U32 max_byte_value,
                                      U32 uniq_byte_values,
                                      U32 max_offset );

// Returns how much data area to save for the header.
RADDEFFUNC U32 LZ_compress_header_size( void );


// Returns the header for the compression.
RADDEFFUNC void LZ_compress_get_header( LZC l,
                                        void * header );

// Opens a handle to use for compressing
RADDEFFUNC LZC LZ_compress_open( void * ptr,
                                 void * compress_temp_buf,
                                 U32 max_byte_value,
                                 U32 uniq_byte_values,
                                 U32 max_offset );

// Opens a handle to use for decompressing
RADDEFFUNC LZD LZ_decompress_open( void * ptr,
                                   U32 max_byte_value,
                                   U32 uniq_byte_values,
                                   U32 max_offset );


// Opens a handle to use for decompressing
RADDEFFUNC LZD LZ_decompress_open_from_header( void * ptr,
                                               void * header );


// Takes a input pointer and compresses the next set of symbols. Returns
//   the number of bytes to move forward.  this function reads back into
//   previous input buffers.  This function will also read one byte past
//   the final input_len, so pad if necessary.
RADDEFFUNC U32 LZ_compress( LZC l,
                            ARITHBITS* ab,
                            U8 const * input,
                            U32 input_len );

// Decompresses to the next set of symbols.  Returns the number of bytes
//   decompressed.
RADDEFFUNC U32 LZ_decompress( LZD l,
                              ARITHBITS* ab,
                              U8 * output );

// Returns only the number of bytes that /would/ have been decompressesed.  Advances the
// position in the stream normally.
RADDEFFUNC U32 LZ_decompress_sizeonly( LZD l, ARITHBITS* ab );

#endif
