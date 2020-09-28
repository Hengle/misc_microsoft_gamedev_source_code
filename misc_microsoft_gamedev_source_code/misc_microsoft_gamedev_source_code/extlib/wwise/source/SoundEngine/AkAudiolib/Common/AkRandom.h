/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkRandom.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RANDOM_H_
#define _RANDOM_H_

/////////////////////////////////////////////////////////
// NOTE:
// Might have to implement the AkRandom on each platform
/////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef __PPU__
#include "sys_time.h"
#endif

namespace AKRANDOM
{
	// The function AkRandomInit must be called once on every 
	// thread that will use the AkRandom function
#ifdef __PPU__
	static const AkInt AK_RANDOM_MAX = 0x7FFFFFFF;

	inline void AkRandomInit()
	{
		sys_time_sec_t Seconds, NanoSeconds;
		sys_time_get_current_time(&Seconds,&NanoSeconds);
		init_TT800(NanoSeconds);
	}
	inline AkInt AkRandom()
	{
		return rand_int31_TT800();
	}
#else
	static const AkInt AK_RANDOM_MAX = RAND_MAX;

	inline void AkRandomInit()
	{
		srand( (AkUInt32)time( NULL ) );
	}
	inline AkInt AkRandom()
	{
		return rand();
	}
#endif
}

#endif
