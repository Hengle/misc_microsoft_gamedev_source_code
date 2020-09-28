#if !defined(GRANNY_C_ARGUMENT_LIST_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_c_argument_list.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !PROCESSOR_X86
#include <stdarg.h>
typedef va_list c_argument_list;
#endif

BEGIN_GRANNY_NAMESPACE;

#if PROCESSOR_X86

typedef char *c_argument_list;

// Assume 32-bit alignment
#define ROUNDED_SIZE_OF(item) ((SizeOf(item)+SizeOf(uint32)-1) & ~(SizeOf(uint32)-1))

#define OpenArgumentList(ArgumentList, ParameterBeforeList) \
    (ArgumentList = ((c_argument_list)&ParameterBeforeList) + ROUNDED_SIZE_OF(ParameterBeforeList))

#define ArgumentListArgument(ArgumentList, type) \
    (*(type *)((ArgumentList += ROUNDED_SIZE_OF(type)) - ROUNDED_SIZE_OF(type)))

#define SkipArgumentListArgument(ArgumentList, type)    \
    ArgumentList += ROUNDED_SIZE_OF(type)

#define CloseArgumentList(ArgumentList) (ArgumentList = (c_argument_list)0)

#else
#define OpenArgumentList(ArgumentList, ParameterBeforeList) va_start(ArgumentList, ParameterBeforeList)
#define ArgumentListArgument(ArgumentList, type) va_arg(ArgumentList, type)
#define CloseArgumentList(ArgumentList) va_end(ArgumentList)

#define SkipArgumentListArgument(ArgumentList, type)            \
    do {                                                        \
        type ignore = ArgumentListArgument(ArgumentList, type); \
        ignore = ignore;                                        \
    } while (false)

#endif

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_C_ARGUMENT_LIST_H
#endif
