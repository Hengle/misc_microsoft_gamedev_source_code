//==============================================================================
// breakpoint.h
//
// Copyright (c) 2000-2006, Ensemble Studios
//==============================================================================
#pragma once

#ifndef breakpoint
   #ifdef _DEBUG
   
      #if defined(XBOX) && !defined(CODE_ANALYSIS_ENABLED)
         #define breakpoint { __asm twi 31, r0, 22 }
	  #elif defined(CODE_ANALYSIS_ENABLED)
		 #define breakpoint exit(0)
      #else
         #define breakpoint { __asm int 3 }
      #endif
      
   #else
      #define breakpoint { }
   #endif // _DEBUG
#endif // breakpoint

