//-----------------------------------------------------------------------------
// File: assert_x86.h
// x86 specific assertion related macros.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef ASSERT_X86
#define ASSERT_X86

#include "common/core/core.h"

#define BreakAlways() do { if (gr::gSystem.debuggerPresent()) { _asm int 3 } } while (0)

#ifndef DEBUG_DEFINED
	#error DEBUG NOT DEFINED
#endif

#if DEBUG
  #define Break() do { if (gr::gSystem.debuggerPresent()) { _asm int 3 } } while (0)
#else
	#define Break() do { } while(0)
#endif

#if DEBUG
	#define Assert(p) \
		do { \
		  static bool ignore; \
			if ((ignore) || (p)) { } else { \
				ignore = gr::gSystem.assertEvent(__FILE__, __LINE__, #p); \
			  if (!ignore) { \
				  do { _asm int 3 } while (0); \
				} } } while (0)
#else
	#define Assert(p) 
#endif

//FIXME
#define StaticAssert Verify

#define Verify(p) \
		do { \
		  static bool ignore; \
			if ((ignore) || (p)) { } else { \
				ignore = gr::gSystem.verifyEvent(__FILE__, __LINE__, #p); \
			  if (!ignore) { \
				  do { _asm int 3 } while (0); \
				} } } while (0) 

#endif // ASSERT_X86
