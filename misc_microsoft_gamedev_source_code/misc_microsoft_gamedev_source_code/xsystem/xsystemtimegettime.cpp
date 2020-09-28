//============================================================================
//
//  xsystemtimegettime.cpp
//  
//  Copyright (c) 2004, Ensemble Studios
//
//============================================================================

#include "xsystem.h"

#undef timeGetTime

// rg [7/21/06] - Removed hacked get time stuff for now because this method may be called
// from multiple threads.

//============================================================================
// ecoreTimeGetTime
//============================================================================
DWORD ecoreTimeGetTime(VOID)
{
#ifdef XBOX   
   return GetTickCount();
#else
//   return(timeGetTime());
   return(0);
#endif      
}

//============================================================================
// eof: ecoretimegettime.cpp
//============================================================================
