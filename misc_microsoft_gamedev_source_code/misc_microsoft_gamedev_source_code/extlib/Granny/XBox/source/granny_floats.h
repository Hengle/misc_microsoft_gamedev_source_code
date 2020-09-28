#if !defined(GRANNY_FLOATS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_floats.h $
// $DateTime: 2007/09/25 14:18:08 $
// $Change: 16059 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

// Getting exceptional floating point values

#define RET_REAL32_FROM_INT(x)                  \
    union conv_union {                          \
        uint32 i;                               \
        real32 f;                               \
    };                                          \
    conv_union cu;                              \
    cu.i = (x);                                 \
    return cu.f

#define RET_REAL64_FROM_INTS(x0, x1)            \
    union conv_union {                          \
        uint32  i[2];                           \
        real64x f;                              \
    };                                          \
    conv_union cu;                              \
    cu.i[0] = (x0);                             \
    cu.i[1] = (x1);                             \
    return cu.f


inline real32 GetReal32Infinity()       { RET_REAL32_FROM_INT(0x7f800000); }
inline real32 GetReal32QuietNaN()       { RET_REAL32_FROM_INT(0xffc00000); }
inline real32 GetReal32SignalingNaN()   { RET_REAL32_FROM_INT(0xff800000); }
inline real32 GetReal32AlmostInfinity() { RET_REAL32_FROM_INT(0x7f0fffff); }

inline real64x GetReal64Infinity()     { RET_REAL64_FROM_INTS(0x00000000, 0x7ff00000); }
inline real64x GetReal64QuietNaN()     { RET_REAL64_FROM_INTS(0x00000000, 0xfff80000); }
inline real64x GetReal64SignalingNaN() { RET_REAL64_FROM_INTS(0x00000000, 0xfff00000); }

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FLOATS_H
#endif
