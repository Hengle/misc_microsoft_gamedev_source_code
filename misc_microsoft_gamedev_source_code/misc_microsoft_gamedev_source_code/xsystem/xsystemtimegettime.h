//============================================================================
//
//  xsystemtimegettime.h
//  
//  Copyright (c) 2004, Ensemble Studios
//
//============================================================================

#ifndef XBOX
#pragma warning( disable : 4201 )
#include <mmsystem.h>
#pragma warning( default : 4201 )
#endif

// Redefined timeGetTime to point at our function.
#define timeGetTime ecoreTimeGetTime
DWORD ecoreTimeGetTime(VOID);

//============================================================================
// eof: ecoretimegettime.h
//============================================================================
