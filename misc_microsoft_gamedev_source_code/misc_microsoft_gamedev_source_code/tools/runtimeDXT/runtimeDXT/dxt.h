// From:
//    Real-Time DXT Compression
//    May 20th 2006 J.M.P. van Waveren
//    (c) 2006, Id Software, Inc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <math.h>

#if defined(WIN32)
float drand48(void);
#endif

#if defined(__APPLE__)
#define memalign(x,y) malloc((y))
#else
#include <malloc.h>
#endif


#if defined(WIN32)
void *aligned_malloc(size_t size, size_t align_size);
#define memalign(x,y) aligned_malloc(y, x)
void aligned_free(void *ptr);
#define memfree(x) aligned_free(x)
#else
#define memfree(x) free(x)
#endif


typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;

#define C565_5_MASK 0xF8 // 0xFF minus last three bits
#define C565_6_MASK 0xFC // 0xFF minus last two bits

#define INSET_SHIFT 4 // inset the bounding box with ( range >> shift )

#if !defined(MAX_INT)
#define       MAX_INT         2147483647      /* max value for an int 32 */
#define       MIN_INT         (-2147483647-1) /* min value for an int 32 */
#endif


#if defined(__GNUC__)
#define   ALIGN16(_x)   _x __attribute((aligned(16)))
#else
#define   ALIGN16( x ) __declspec(align(16)) x
#endif


// Compress to DXT1 format
void CompressImageDXT1( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );

// Compress to DXT5 format
void CompressImageDXT5( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );

// Compress to DXT5 format, first convert to YCoCg color space
void CompressImageDXT5YCoCg( const byte *inBuf, byte *outBuf, int width, int height, int &outputBytes );

// Compute error between two images
double ComputeError( const byte *original, const byte *dxt, int width, int height);
