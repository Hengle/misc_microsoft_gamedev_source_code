#if !defined(GRANNY_ASSERT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_assert.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif


// Handy place to do some checks.

// _DEBUG is defined if the compiler is working in debug mode,
// i.e. with no optimisations. But even in an optimised build,
// we might want asserts and suchlike.
#if defined(_DEBUG) && (_DEBUG==0)
#error Either define _DEBUG, or dont define it. Never set it to 0.
#endif

// DEBUG is the one that says "I want asserts". You might still
// want asserts and checks in an optimised build for example.
#if defined(DEBUG) && (DEBUG!=1)
#error Either define DEBUG to 1, or dont define it. Never set it to 0.
#endif

#if PLATFORM_WINXX
#define GRANNY_ALLOW_IGNORE_ASSERT
#endif



BEGIN_GRANNY_NAMESPACE;

#if DEBUG
    #ifdef GRANNY_ALLOW_IGNORE_ASSERT
       void DisplayAssertion(char const * const Expression,
                             char const * const File,
                             int32x const LineNumber,
                             char const * const Function,
                             bool* IgnoreAssertion);
        #undef Assert
        #define Assert(Expression)                                                                          \
            {                                                                                               \
                USING_GRANNY_NAMESPACE;                                                                     \
                static bool IgnoreAssert = false;                                                           \
                if (!IgnoreAssert && !(Expression))                                                         \
                {                                                                                           \
                    DisplayAssertion(#Expression, __FILE__, __LINE__, "Unknown", &IgnoreAssert);            \
                }                                                                                           \
            } typedef int gr__AssertionsRequireSemiColons

    #else
        void DisplayAssertion(char const * const Expression,
                              char const * const File,
                              int32x const LineNumber,
                              char const * const Function);

        #undef Assert
        #define Assert(Expression)                                                  \
            {                                                                       \
                if (!(Expression))                                                  \
                {                                                                   \
                    DisplayAssertion(#Expression, __FILE__, __LINE__, "Unknown");   \
                }                                                                   \
            } typedef int gr__AssertionsRequireSemiColons

    #endif

#else

    #undef Assert
    #define Assert(Expression) (void)sizeof(Expression)

#endif // DEBUG


// Compile assertions are always turned on
#define CompileAssert(Expression) typedef int CompileAssert_Type__[(Expression) ? 1 : -1]

#define InvalidCodePath(Text) Assert(!Text)
#define NotImplemented InvalidCodePath("NotImplemented")

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_ASSERT_H
#endif
