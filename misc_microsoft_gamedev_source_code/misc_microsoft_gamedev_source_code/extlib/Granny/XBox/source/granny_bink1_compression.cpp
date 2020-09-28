// ========================================================================
// $File: //jeffr/granny/rt/granny_bink1_compression.cpp $
// $DateTime: 2007/11/02 17:42:04 $
// $Change: 16424 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BINK1_COMPRESSION_H)
#include "granny_bink1_compression.h"
#endif

// NOTE!  This include has to be here, to avoid the C/C++ include conflict
// that comes from including Jeff's C code.
#include <math.h>

extern "C"
{
#if !defined(BINKTC_H)
#include "binktc.h"
#endif
}

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

/* ========================================================================
   This is the glue code
   ======================================================================== */
uint32 GRANNY
ToBinkTC1(void *output,
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
ToBinkTCTempMem1(uint32 width, uint32 height)
{
    return(to_BinkTC_temp_mem(width, height));
}

uint32 GRANNY
ToBinkTCOutputMem1(uint32 width, uint32 height,
                   uint32 planes, uint32 compress_to)
{
    return(to_BinkTC_output_mem(width, height, planes, compress_to));
}

void GRANNY
BinkTCCheckSizes1(uint32 *width, uint32 *height)
{
    U32 wtemp, htemp;
    wtemp = *width;
    htemp = *height;
    BinkTC_check_sizes(&wtemp, &htemp);
    *width = wtemp;
    *height = htemp;
}

void GRANNY
FromBinkTC1(int16 **output,
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
FromBinkTCTempMem1(void const *binkbuf)
{
    return(from_BinkTC_temp_mem(binkbuf));
}

#if HAVE_RADMALLOC
#else

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#include "radmem.h"

USING_GRANNY_NAMESPACE;

extern "C"
{
    RADDEFFUNC void PTR4* RADLINK radmalloc(U32 numbytes)
    {
        // If we're linked with bink, we need to return an allocation aligned to at least
        // 32 bytes, which bink expects.  Depending on the order of the libraries on the
        // linker command line, we might wind up being the radmalloc, rather than the bink
        // version.
        int32 const MaxAlignment =
            DefaultAllocationAlignment < 32 ? 32 : DefaultAllocationAlignment;

        return(AllocateSizeAligned(MaxAlignment, numbytes));
    }

    RADDEFFUNC void RADLINK radfree(void PTR4* ptr)
    {
        Deallocate(ptr);
    }

    RADDEFFUNC void RADLINK RADSetMemory(RADMEMALLOC a,RADMEMFREE f)
    {
    }
}
#endif
