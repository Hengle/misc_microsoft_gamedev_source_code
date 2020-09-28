// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_assert.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if DEBUG
#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
DisplayAssertion(char const * const Expression,
                 char const * const File,
                 int32x const LineNumber,
                 char const * const Function)
{
    assert(!"Granny assertion");
}
#endif

