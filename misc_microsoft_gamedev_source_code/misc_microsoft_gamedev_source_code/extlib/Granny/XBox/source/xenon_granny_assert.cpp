// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_assert.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if DEBUG
#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

char AssertionBuffer[1 << 16];
void GRANNY
DisplayAssertion(char const * const Expression,
                 char const * const File,
                 int32x LineNumber,
                 char const * const Function)
{
    wsprintf(AssertionBuffer, "%s:%d \"%s\"",
             File, LineNumber, Expression);
    OutputDebugStringA(AssertionBuffer);
    DebugBreak();
}
#endif
