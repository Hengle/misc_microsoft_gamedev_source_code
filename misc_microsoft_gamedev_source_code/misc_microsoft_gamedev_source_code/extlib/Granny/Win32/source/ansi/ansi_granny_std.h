#if !defined(ANSI_GRANNY_STD_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_std.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#if COMPILER_GCC
#include <sys/errno.h>
#else
#include <errno.h>
#endif

BEGIN_GRANNY_NAMESPACE;

// ANSILogErrorAsWarning and ANSILogErrorAsError will check the last
// error code.  If it is set, it will log the reported ANSI string as
// a warning or an error (respectively) in the error log.
#define ANSILogErrorAsWarning(FailedANSIFunction) \
        ANSILogLastError(false, __FILE__, __LINE__, #FailedANSIFunction);
#define ANSILogErrorAsError(FailedANSIFunction) \
        ANSILogLastError(true, __FILE__, __LINE__, #FailedANSIFunction);

// This is a more convenient form of the standard ANSI fseek call
int32x ANSISeek(FILE *ANSIFileHandle, int32x Offset, int32x MoveMethod);

// The following functions should not be called directly.  You
// should use the macros defined above.
void ANSILogLastError(bool IsError,
                      char const *SourceFile, int32x SourceLineNumber,
                      char const *FailedANSIFunction);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define ANSI_GRANNY_STD_H
#endif
