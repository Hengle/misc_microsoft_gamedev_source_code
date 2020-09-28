// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_std.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

// Prevent deprecated warnings in VC, we don't want to use the
//  new fopen_s, etc, api.
#if COMPILER_MSVC
#pragma warning(disable : 4996)
#endif

void GRANNY
ANSILogLastError(bool IsError,
                 char const *SourceFile, int32x SourceLineNumber,
                 char const *FailedANSIFunction)
{
    if(errno)
    {
        Log4(IsError ? ErrorLogMessage : WarningLogMessage,
             ANSISubsystemLogMessage,
             "%s(%d) : "
             "%s failed with error \"%s\"",
             SourceFile, SourceLineNumber,
             FailedANSIFunction, strerror(errno));
    }
}

