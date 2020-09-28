#if !defined(GRANNY_PREFETCH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_prefetch.h $
// $DateTime: 2007/06/05 16:17:47 $
// $Change: 15106 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
//
//    This file is slightly unorthodox, in that it contains platform
//    specific code, which would normally go into a platform directory.
//    However, we want to make sure that these calls are inlined whenever
//    possible, so they're implemented directly.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

// Platform specific headers
#if PLATFORM_XENON
  #include <ppcintrinsics.h>
#elif PLATFORM_PS3
  #include <ppu_intrinsics.h>
#else
  // by default, nothing
#endif


BEGIN_GRANNY_NAMESPACE;

#if PLATFORM_XENON

#define PLATFORM_CACHE_LINE_SIZE 128

inline void PrefetchAddress(void const* Address, int32x Size)
{
    int32x Offset = 0;
    while (Offset < Size)
    {
        __dcbt(Offset, Address);
        Offset += 128;
    }
}

#elif PLATFORM_PS3

#define PLATFORM_CACHE_LINE_SIZE 128

inline void PrefetchAddress(void const* Address, int32x Size)
{
    uint8 const* Current = (uint8 const*)Address;
    int32x Offset = 0;
    while (Offset < Size)
    {
        __dcbt(Current + Offset);
        Offset += 128;
    }
}

#else

#define PLATFORM_CACHE_LINE_SIZE 64

// Generic prefetch is a NOP
inline void PrefetchAddress(void const* Address, int32x Size)
{
    // nop
}

#endif

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PREFETCH_H
#endif
