//============================================================================
//
//  CommonMacros.h
//  
// Copyright (c) 2000-2006, Ensemble Studios
//
//============================================================================
#pragma once

#if defined(__cplusplus)
extern "C++"
{
#ifndef __nothrow
#define __nothrow __declspec( nothrow )
#endif

#else

#ifndef __nothrow
#define __nothrow
#endif

#endif // defined(__cplusplus)

#define __noreturn __declspec( noreturn )
#ifndef __fallthrough
// use __fallthrough in switch statement labels where you want the desired behaviour to
// distinguish your code from forgotten break statements.
#if( OACR ) || (_PREFAST_)
__inline __nothrow void __FallThrough(){}
#define __fallthrough __FallThrough();
#else
#define __fallthrough
#endif
#endif // __fallthrough

#if defined(__cplusplus)
} // extern "c++" {
#endif

//==============================================================================
// 
//==============================================================================
#ifdef _DEBUG
   #define DEBUG_STRING(s) s
#else
   #define DEBUG_STRING(s) 0
#endif

//==============================================================================
// 
//==============================================================================
//Delete simplification.
#define BDELETE(p)   \
if (p) \
{  \
   delete p; \
   p=NULL; \
}

#define BDELETEARRAY(p)   \
if (p) \
{  \
   delete [] p; \
   p=NULL; \
}

//Fileclose simplification.
#define BFILECLOSE(p)   \
if (p) \
{  \
   p->close(); \
   p=NULL; \
}

#define BRELEASE(p)   \
if (p) \
{  \
   p->Release(); \
   p=NULL; \
}

//==============================================================================
// bswap
//==============================================================================
template <class Type> void bswap(Type &a, Type &b) { Type c = a; a = b; b = c; }

//==============================================================================
//  BCLAMP
//==============================================================================
#define BCLAMP(val, min, max) { if (val < (min)) val = (min); if (val > (max)) val = (max); }


//----------------------------------------------------------------------------
//  String related macros
//----------------------------------------------------------------------------
#ifdef UNICODE
   #define B(text) (L ## text)
   #define BTEXT(text) (L ## text)
#else
   #define B(text) (text)
   #define BTEXT(text) (text)
#endif   

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*a))
#endif

#define floatDWORD(x) *((DWORD*)& x)
#define DWORDfloat(x) *((float*)& x)

inline float _fabs(float x) { return fabs(x); }

//----------------------------------------------------------------------------
//  Tell LINT programs and other programmers that "Yes, I know I am calling
//  a method that returns a value, and yes, I REALLY don't care about it and
//  yes, it is REALLY safe to ignore the return value".
//----------------------------------------------------------------------------
#define IGNORE_RETURN(x) x

//==============================================================================
// eof: commonMacros.h
//==============================================================================
