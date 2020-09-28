#if !defined(GRANNY_NAMESPACE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_namespace.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#ifdef GRANNY_NO_NAMESPACE
#define BEGIN_GRANNY_NAMESPACE typedef char __Ignored__
#define END_GRANNY_NAMESPACE typedef char __Ignored__
#define USING_GRANNY_NAMESPACE typedef char __Ignored__
#define GRANNY
#else
#define BEGIN_GRANNY_NAMESPACE namespace granny {typedef char __Ignored__
#define END_GRANNY_NAMESPACE }typedef char __Ignored__
#define USING_GRANNY_NAMESPACE using namespace granny
#define GRANNY granny::
#endif

#define const_mod
#define EXPAPI
#define EXPTYPE
#define EXPTYPE_EPHEMERAL
#define EXPMACRO
#define EXPCONST
#define EXPGROUP(GroupName)

#define GS_SAFE
#define GS_READ
#define GS_PARAM
#define GS_MODIFY
#define GS_SPECIAL


#if defined(_MSC_VER)
#if !GRANNY_SHOW_HARMLESS_WARNINGS
#pragma warning( disable : 4100 )   // warning C4100: 'CopySize' : unreferenced formal parameter
#pragma warning( disable : 4127 )   // "conditional expression is constant", e.g. Assert(!"Scary")
#pragma warning( disable : 4200 )   // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning( disable : 4201 )   // warning C4201: nonstandard extension used : nameless struct/union
#pragma warning( disable : 4505 )   // warning C4505: 'AccumulateModelAnimations' : unreferenced local function has been removed
#pragma warning( disable : 4514 )   // "unreferenced inline function has been removed"
#pragma warning( disable : 4725 )   // warning C4725: instruction may be inaccurate on some Pentiums (it's the FDIV bug!)

// Turn these on if you have to, but you really shouldn't need to.
// You may possibly want to put them in the particular file itself though.
//#pragma warning( disable : 4800 )   // warning C4800: 'granny::int32' : forcing value to bool 'true' or 'false' (performance warning)
//#pragma warning( disable : 4244 )   // warning C4244: '=' : conversion from 'double' to 'granny::real32', possible loss of data
//#pragma warning( disable : 4715 )   // warning C4715: 'granny::GetMaximumBinkImageSize' : not all control paths return a value
//#pragma warning( disable : 4018 )   // warning C4018: warning C4018: '<' : signed/unsigned mismatch
//#pragma warning( disable : 4065 )   // warning C4065: switch statement contains 'default' but no 'case' labels
//#pragma warning( disable : 4035 )   // warning C4035: 'multm164anddiv' : no return value

#endif
#endif

#include "header_postfix.h"
#define GRANNY_NAMESPACE_H
#endif
