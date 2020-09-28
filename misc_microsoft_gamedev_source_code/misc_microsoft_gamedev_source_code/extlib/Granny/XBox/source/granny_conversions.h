#if !defined(GRANNY_CONVERSIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_conversions.h $
// $DateTime: 2007/04/23 12:50:50 $
// $Change: 14828 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

BEGIN_GRANNY_NAMESPACE;

typedef uint16 rounding_mode;
inline rounding_mode
BeginNearestRoundingMode(void)
{
#if PROCESSOR_X86
    rounding_mode OldMode;
    rounding_mode NewMode;

    _asm {
        fstcw OldMode;
    };

    NewMode = OldMode;
    NewMode &= 0xF3FF;

    _asm {
        fldcw NewMode;
    };

    return(OldMode);
#else
    return(0);
#endif
}

inline void
EndNearestRoundingMode(rounding_mode OldMode)
{
#if PROCESSOR_X86
    _asm {
        fldcw OldMode;
    };
#else
    #if COMPILER_METROWERKS
    // This prevents the unused variable warning
    OldMode;
    #endif
#endif
}

#if PROCESSOR_X86
inline int32
X86RoundingOp(real32 const Float, uint16 Mode)
{
    rounding_mode OldMode;
    rounding_mode NewMode;

    _asm {
        fstcw OldMode;
    };

    NewMode = OldMode;
    NewMode &= 0xF3FF;
    NewMode |= Mode << 10;

    int32 Int32;
    _asm {
        fldcw NewMode;
        fld Float;
        fistp Int32;
        fldcw OldMode;
    };

    return(Int32);
}
#endif

inline int32
FloorReal32ToInt32(real32 const Float)
{
#if PROCESSOR_X86
    return(X86RoundingOp(Float, 1));
#else
    return((Float < 0.0f) ? (int32(Float) - 1) : int32(Float));
#endif
}

inline int32
CeilingReal32ToInt32(real32 const Float)
{
#if PROCESSOR_X86
    return(X86RoundingOp(Float, 2));
#else
    return((Float < 0.0f) ? int32(Float) : (int32(Float) + 1));
#endif
}

inline int32
TruncateReal32ToInt32(real32 const Float)
{
#if PROCESSOR_X86
    return(X86RoundingOp(Float, 3));
#else
    return(int32(Float));
#endif
}

inline int32
RoundReal32ToInt32(real32 const Float)
{
#if PROCESSOR_X86
    int32 Int32;
    _asm {
        fld Float;
        fistp Int32;
    }

    return(Int32);
#else
    return(int32(Float + 0.5f));
#endif
}

inline uint32
RoundReal64ToUInt32(real64x const Float)
{
#if PROCESSOR_X86
    uint32 UInt32;
    _asm {
        fld Float;
        fistp UInt32;
    }

    return(UInt32);
#else
    return(uint32(Float + 0.5f));
#endif
}

inline int32
RoundReal64ToInt32(real64x const Float)
{
#if PROCESSOR_X86
    int32 Int32;
    _asm {
        fld Float;
        fistp Int32;
    }

    return(Int32);
#else
    return(int32(Float + 0.5f));
#endif
}

inline uint64
RoundReal64ToUInt64(real64x const Float)
{
#if PROCESSOR_X86
    uint64 UInt64;
    _asm {
        fld Float;
        fistp UInt64;
    }

    return(UInt64);
#else
    return(uint64(Float + 0.5f));
#endif
}

inline int64
RoundReal64ToInt64(real64x const Float)
{
#if PROCESSOR_X86
    int64 Int64;
    _asm {
        fld Float;
        fistp Int64;
    }

    return(Int64);
#else
    return(int64(Float + 0.5f));
#endif
}

inline uint64
BuildUInt64(uint32 const HighUInt32, uint32 const LowUInt32)
{
#if COMPILER_MSVC
#pragma warning( disable : 4201 )
    union
    {
        uint64 UInt64;
        struct
        {
            uint32 LowUInt32;
            uint32 HighUInt32;
        };
    } Converter;

    Converter.HighUInt32 = HighUInt32;
    Converter.LowUInt32 = LowUInt32;

    return(Converter.UInt64);
#else
    return ((uint64(HighUInt32) << 32) | uint64(LowUInt32));
#endif
}

inline void
DecomposeUInt64(uint64 const Source, uint32 &HighUInt32, uint32 &LowUInt32)
{
#if COMPILER_MSVC
#pragma warning( disable : 4201 )
    union
    {
        uint64 UInt64;
        struct
        {
            uint32 LowUInt32;
            uint32 HighUInt32;
        };
    } Converter;

    Converter.UInt64 = Source;
    HighUInt32 = Converter.HighUInt32;
    LowUInt32 = Converter.LowUInt32;
#else
    HighUInt32 = (uint32)(Source >> 32);
    LowUInt32 = (uint32)(Source & 0x00000000FFFFFFFFLL);
#endif
}

inline uint8
BinormalReal32ToUInt8(real32 Real32)
{
    Assert((Real32 >= -1.0f) && (Real32 <= 1.0f));

    // Clamp to make sure we get good values.  Slightly off values can come in from real32
    // noise on occasion.
    if (Real32 <= -1.0f)
        return 0;
    else if (Real32 >= 1.0f)
        return 255;

    return((uint8)RoundReal32ToInt32(127.5f * (1.0f + Real32)));
}

inline uint8
NormalReal32ToUInt8(real32 Real32)
{
    Assert((Real32 >= 0.0f) && (Real32 <= 1.0f));

    // Clamp to make sure we get good values.  Slightly off values can come in from real32
    // noise on occasion.
    if (Real32 <= 0.f)
        return 0;
    else if (Real32 >= 1.0f)
        return 255;

    return((uint8)RoundReal32ToInt32(255.0f * Real32));
}

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CONVERSIONS_H
#endif
