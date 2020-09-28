//============================================================================
//
//  xcore.h
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================

#pragma once

//----------------------------------------------------------------------------
//  Warnings we hate.
//----------------------------------------------------------------------------
#pragma warning(disable: 4710) // Dont care about "not inlined" warnings.
#pragma warning(disable: 4239) // nonstandard extension used : 'argument' : conversion from 'class foo' to 'class foo &'
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4710) // function 'bool __thiscall BPointerArray<class BFoo *>::resize(int)' not inlined
#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable: 4291) // no matching operator delete found; memory will not be freed if initialization throws an exception
#pragma warning(disable: 4786) // identifier was truncated to '255' characters in the debug information
#pragma warning(disable: 4127) // Turn off the conditional expression is constant for chunker.h MACROs. TODO FIXME HACK: this sucks
#pragma warning(disable: 4288) // VC7TODO: For-loop scoping ... fix this
#pragma warning(disable: 4353) // VC7TODO: __noop thing ... fix this
#pragma warning(disable: 4995) // turn off #pragma deprecated, since strsafe.h complains about SOOO much stuff right now... fix this
#pragma warning(disable: 4652) // turn off compiler option 'C++ Exception Handling Unwinding' inconsistent with precompiled header; current command-line option will override that defined in the precompiled header

//----------------------------------------------------------------------------
//  External Includes
//----------------------------------------------------------------------------

#ifndef XBOX
   #pragma warning(disable: 4995)
   
   //----------------------------------------------------------------------------
   //  Windows Header Setup
   //----------------------------------------------------------------------------
   #if !defined(_WIN32_WINDOWS)
      #define _WIN32_WINDOWS 0x0500
   #endif

   #define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */

   #undef _WIN32_WINNT
   #define _WIN32_WINNT 0x500 // Added to get QueueAPC for Network\LoopbackSocket.h
                              // And get access to GLYPHSET.

   #define MEMORY_SYSTEM_ENABLE 0
   
   // Disable STL exceptions
   // #define _HAS_EXCEPTIONS 0
   
   #include <windows.h>
   #include <windowsx.h>

   #pragma warning(push)
   #pragma warning(disable:4996)
      #include <strsafe.h>
   #pragma warning(pop)

   #ifdef _NETAPI
      #include <winsock.h>
   #endif
   
   #define PC
   
   // rg [1/17/05] - FIXME I have to include this to get the old vector/matrix classes to build. Ugh.
   #include <d3dx9.h>
   #include <d3dx9math.h>
   
   #include "utils\XTLOnPC.h"

   // Don't show warnings about expressions with no effect.
   #pragma warning(disable:4548)		
   #pragma warning(disable:4555)
   
   #define RESTRICT
      
#else // XBOX
   #define D3DCOMPILE_USEVOIDS 
   
   #if (defined(BUILD_DEBUG) || (defined(BUILD_PROFILE) && !defined(FASTCAP)))
      #define USE_PIX
   #endif
   
   #define MEMORY_SYSTEM_ENABLE 1
   
   #if MEMORY_SYSTEM_ENABLE
      #define _MFC_OVERRIDES_NEW
   #endif      
   
   // Disable STL exceptions
   #define _HAS_EXCEPTIONS 0
   
   #define _XBOX_CRT_DEPRECATE_INSECURE 1
   
   #include <xtl.h>
               
   #define REQUIRED_XDK_VERSION 7776
   #define REQUIRED_FLASH_VERSION 6719

   #if _XDK_VER < REQUIRED_XDK_VERSION
      #error Please update your XDK.
   #endif      
   
   // rg [10/08/06] - Disabling all uses of RESTRICT because the compiler can't handle it reliably!
   //#define RESTRICT __restrict
   #define RESTRICT 
                     
#endif // XBOX

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <process.h>
#include <time.h>
#include <assert.h>
#include <malloc.h>
#include <ctype.h>
#include <direct.h>

// According to ATG, on 360 the <algorithm> header does not require the STL or exceptions.
#include <algorithm>

#ifdef BUILD_CHECKED
   // rg [6/12/2008] - Make sure BUILD_DEBUG is also defined in checked builds, because we want debug checking enabled too.
   #define BUILD_DEBUG
#endif

#include "threading\lightWeightMutex.h"
#include "memory\xmemory.h"

// Tread carefully here when modifying the order of these includes.

#include "PIXHelpers.h"
#include "compileTimeAssert.h"
#include "estypes.h"
#include "simpleTypes.h"
#include "utils\builtInTypes.h"

#include "buildOptions.h"

#include "threading\criticalSection.h"

#include "breakpoint.h"
#include "scalarConsts.h"
#include "commonMacros.h"
#include "trace.h"
#include "bassert.h"
#include "compileTimeAssert.h"
#include "rangeCheck.h"

#include "memory\memoryHeapMacros.h"

//#include "threading\lightWeightMutex.h"

#include "memory\AllocFixed.h"
#include "utils\endianSwitch.h"
#include "utils\textDispatcher.h"
#include "utils\utils.h"
#include "memory\mallocWrappers.h"
#include "memory\allocators.h"
#include "string\bstring.h"
#include "utils\dataBuffer.h"
#include "containers\dynamicArray.h"
#include "math\math.h"
#include "stream\stream.h"
#include "string\fixedstring.h"
#include "memory\alignedAlloc.h"

typedef bool (*BXCoreInitFuncPtr)(bool init);
extern void XCoreInitialize(void);
extern void XCoreDeinitialize(void);

