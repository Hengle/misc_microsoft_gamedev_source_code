#if !defined(GRANNY_INTRINSICS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_intrinsics.h $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #26 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#include "granny_platform.h"

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if PLATFORM_XENON
#include <ppcintrinsics.h>
#endif


#if PLATFORM_PS2 || PLATFORM_PSP || (PLATFORM_PS3 && PROCESSOR_CELL)

// doubles!?!?!? Crazy man.
extern "C"
{
    extern GRANNY real32 radsqrt(GRANNY real32);
    extern GRANNY real32 radsin(GRANNY real32);
    extern GRANNY real32 radcos(GRANNY real32);
    extern GRANNY real32 radfabs(GRANNY real32);
    extern GRANNY real32 radlog2(GRANNY real32);
    extern GRANNY real32 radatan(GRANNY real32);
    extern GRANNY real32 radatan2(GRANNY real32, GRANNY real32);
    extern GRANNY real32 radpow(GRANNY real32, GRANNY real32);

    // PS2 has these as intrinsics, psp and ps3 need them declared as
    // of now
#if PLATFORM_PSP || PLATFORM_PS3
    extern double fabs(double);
    extern int    abs(int);
#endif
}


BEGIN_GRANNY_NAMESPACE;

inline real32
IntrinsicAbsoluteValue(real32 const Value)
{
    return(radfabs(Value));
}

inline real32
IntrinsicSquareRoot(real32 const Value)
{
    return(radsqrt(Value));
}

inline real32
IntrinsicModulus(real32 const Dividend, real32 const Divisor)
{
    return(Dividend - Divisor*((real32)(TruncateReal32ToInt32(Dividend / Divisor))));
}

inline real32
IntrinsicSin(real32 const AngleInRadians)
{
    return(radsin(AngleInRadians));
}

inline real32
IntrinsicCos(real32 const AngleInRadians)
{
    return(radcos(AngleInRadians));
}

inline void
IntrinsicSinCos(real32 const AngleInRadians, real32* SinVal, real32* CosVal)
{
    *SinVal = IntrinsicSin(AngleInRadians);
    *CosVal = IntrinsicCos(AngleInRadians);
}

inline real32
IntrinsicTan(real32 const AngleInRadians)
{
  Assert(0);
  return 0.0f;
}

inline real32
IntrinsicATan2(real32 const SinAngle, real32 const CosAngle)
{
    return(radatan2(SinAngle, CosAngle));
}

inline real32
IntrinsicASin(real32 const AngleInRadians)
{
  Assert(0);
  return 0.0f;
}

inline real32
IntrinsicACos(real32 const AngleInRadians)
{
    return((AngleInRadians == 0) ? 0 :
           radatan2(radsqrt((1.0f - AngleInRadians)*
                         (1.0f + AngleInRadians)),
                   AngleInRadians));
}

inline real32
IntrinsicATan(real32 const AngleInRadians)
{
    return(radatan(AngleInRadians));
}

inline real32
IntrinsicExp(real32 const Value)
{
  Assert(0);
  return 0.0f;
}

inline real32
IntrinsicPow(real32 X, real32 Y)
{
  return(radpow(X,Y));
}

END_GRANNY_NAMESPACE;

#else //#if PLATFORM_PS2


#if COMPILER_MSVC
extern "C"
{
    int __cdecl abs(int);
    double __cdecl log(double);
    double __cdecl fabs(double);
    double __cdecl sqrt(double);
    double __cdecl fmod(double, double);
    double __cdecl sin(double);
    double __cdecl cos(double);
    double __cdecl tan(double);
    double __cdecl asin(double);
    double __cdecl acos(double);
    double __cdecl atan(double);
    double __cdecl atan2(double, double);
    double __cdecl exp(double);
    double __cdecl pow(double,double);
};
#if !PLATFORM_XENON
#pragma intrinsic(abs, log, fabs, sqrt, fmod, sin, cos, tan, asin, acos, atan, atan2, exp, pow)
#else
#pragma intrinsic(abs, fabs, sqrt)
#endif

#elif COMPILER_GCC

extern "C"
{
#if !PLATFORM_GAMECUBE && !PLATFORM_LINUX && !PLATFORM_PS2
    // We can't compile abs this way on the 'cube, cause the GCC variant
    // there defines it internally
    extern int abs(int);
#endif
    extern double fabs(double);
    extern double log(double);
    extern double fmod(double, double);
    extern double tan(double);
    extern double asin(double);
    extern double acos(double);
    extern double atan(double);
    extern double atan2(double, double);
    extern double exp(double);
    extern double sqrt(double);
    extern double sin(double);
    extern double cos(double);
    extern double fabs(double);
    extern double pow(double,double);
};

#elif COMPILER_METROWERKS
extern "C"
{
    extern int abs(int);
    extern double sqrt(double);
    extern double sin(double);
    extern double cos(double);
    extern double fabs(double);
    extern double log(double);
    extern double fmod(double, double);
    extern double tan(double);
    extern double asin(double);
    extern double acos(double);
    extern double atan(double);
    extern double atan2(double, double);
    extern double exp(double);
    extern double pow(double,double);
}
#endif

BEGIN_GRANNY_NAMESPACE;

inline double
IntrinsicAbsoluteValue(double const Value)
{
    return(fabs(Value));
}

inline double
IntrinsicSquareRoot(double const Value)
{
    return(sqrt(Value));
}

inline real32
IntrinsicModulus(real32 const Dividend, real32 const Divisor)
{
#if PROCESSOR_X86
    real32 Result;
    __asm
    {
        fld [Divisor]
        fld [Dividend]
    looper:
        fprem
        fwait
        fnstsw ax
        sahf
        jp looper
        fstp [Result]
        fstp st(0)
    }

    return(Result);
#else
    return(Dividend - Divisor*((real32)(TruncateReal32ToInt32(Dividend / Divisor))));
#endif
}

inline double
IntrinsicSin(double const AngleInRadians)
{
    return(sin(AngleInRadians));
}

inline double
IntrinsicCos(double const AngleInRadians)
{
    return(cos(AngleInRadians));
}


inline void
IntrinsicSinCos(double const AngleInRadians, real32* SinVal, real32* CosVal)
{
#if PROCESSOR_X86
    __asm
    {
        fld [AngleInRadians]
        fsincos
        mov eax, [SinVal]
        mov edx, [CosVal]
        fstp DWORD PTR [edx]
        fstp DWORD PTR [eax]
    }
#else
    *SinVal = (real32)IntrinsicSin(AngleInRadians);
    *CosVal = (real32)IntrinsicCos(AngleInRadians);
#endif
}


inline double
IntrinsicTan(double const AngleInRadians)
{
    return(tan(AngleInRadians));
}

inline double
IntrinsicATan2(double const SinAngle, double const CosAngle)
{
    return(atan2(SinAngle, CosAngle));
}

inline double
IntrinsicASin(double const AngleInRadians)
{
    return(asin(AngleInRadians));
}

inline double
IntrinsicACos(double const AngleInRadians)
{
    return((AngleInRadians == 0) ? 0 :
           atan2(sqrt((1.0f - AngleInRadians)*
                      (1.0f + AngleInRadians)),
                 AngleInRadians));
}

inline double
IntrinsicATan(double const AngleInRadians)
{
    return(atan(AngleInRadians));
}

inline double
IntrinsicExp(double const Value)
{
    return(exp(Value));
}


#define radlog2

inline real32
IntrinsicPow(real32 X, real32 Y)
{
  return((real32)pow(X,Y));
}

END_GRANNY_NAMESPACE;

#endif //#else //#if PLATFORM_PS2

#include "header_postfix.h"
#define GRANNY_INTRINSICS_H
#endif
