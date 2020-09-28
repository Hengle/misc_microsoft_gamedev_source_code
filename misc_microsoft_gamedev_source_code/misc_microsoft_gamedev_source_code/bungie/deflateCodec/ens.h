//============================================================================
// ens.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================

#pragma once

//----------------------------------------------------------------------------
//  External Includes
//----------------------------------------------------------------------------

#ifndef _XBOX
   //----------------------------------------------------------------------------
   //  Windows Header Setup
   //----------------------------------------------------------------------------
   #if !defined(_WIN32_WINDOWS)
      #define _WIN32_WINDOWS 0x0500
   #endif

   #undef _WIN32_WINNT
   #define _WIN32_WINNT 0x500 // Added to get QueueAPC for Network\LoopbackSocket.h
                              // And get access to GLYPHSET.

   #include <windows.h>
   #include <windowsx.h>
   
   #include <strsafe.h>
   
   #define RESTRICT
      
#else 
   #include <xtl.h>
         
//   #if _XDK_VER < 2732       
//      #error Must build against the April '06 XDK or later.
//   #endif      
   
   #define RESTRICT __restrict
            
#endif // _XBOX

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "compileTimeAssert.h"
#include "simpleTypes.h"
#include "utils.h"
#include "ensMath.h"

