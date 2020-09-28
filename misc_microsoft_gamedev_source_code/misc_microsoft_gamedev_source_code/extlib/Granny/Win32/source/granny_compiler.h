#if !defined(GRANNY_COMPILER_H)
// #include "header_preamble.h" nb: granny_compiler is included in header_preamble!
// ========================================================================
// $File: //jeffr/granny/rt/granny_compiler.h $
// $DateTime: 2007/08/29 13:58:19 $
// $Change: 15855 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// Clear all flags
#define COMPILER_MSVC 0
#define COMPILER_WATCOM 0
#define COMPILER_IBM 0
#define COMPILER_METROWERKS 0
#define COMPILER_SYMANTEC 0
#define COMPILER_METAWARE 0
#define COMPILER_GCC 0


// Microsoft VC++ (version 4.0 defines this as 1000)
#ifdef _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif

// Watcom C/C++ (version 10.5)
#ifdef __WATCOMC__
#undef COMPILER_WATCOM
#define COMPILER_WATCOM 1
#endif

// IBM Visual Age C++ (defines this to 1)
#ifdef __IBMCPP__
#undef COMPILER_IBM
#define COMPILER_IBM 1
#endif

// Metrowerks (CodeWarrior)
#ifdef __MWERKS__
#undef COMPILER_METROWERKS
#define COMPILER_METROWERKS 1
#endif

// Symantec C/C++ (version 8 defines this to 0x800)
#ifdef __SC__
#undef COMPILER_SYMANTEC
#define COMPILER_SYMANTEC 1
#endif

// MetaWare High C/C++
#ifdef __HIGHC__
#undef COMPILER_METAWARE
#define COMPILER_METAWARE 1
#endif

// Gnu GCC
#if defined(__GNUG__) || defined(__GNUC__)
#undef COMPILER_GCC
#define COMPILER_GCC 1
#endif

#define IS_ALIGNED_N(A, n) ((((uintaddrx)A) % (n)) == 0)
#define IS_ALIGNED_16(A)   IS_ALIGNED_N((A), 16)

#if COMPILER_MSVC

#define ALIGN_N(type, n) __declspec(align(n)) type
#define ALIGN16(type) ALIGN_N(type, 16)

#if defined(_XENON)
  #define NOALIAS __restrict
#else
  #define NOALIAS
#endif

#pragma warning(disable : 4121) // alignment of a member was sensitive to packing
#pragma warning(disable : 4103) // used #pragma pack to change alignment

#else

#define ALIGN_N(type, n) __attribute__ ((aligned (n))) type
#define ALIGN16(type) ALIGN_N(type, 16)
#define NOALIAS __restrict

#endif

#define UNUSED_VARIABLE(x) (void)(sizeof(x))

#if !COMPILER_MSVC && !COMPILER_WATCOM && !COMPILER_IBM && !COMPILER_METROWERKS && !COMPILER_SYMANTEC && !COMPILER_METAWARE && !COMPILER_GCC
#error Unrecognized compiler (see compiler.h)
#endif

// #include "header_postfix.h" nb: granny_compiler is included in header_preamble!
#define GRANNY_COMPILER_H
#endif
