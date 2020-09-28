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
// AkReverbUnitCommon.h
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKREVERBUNITCOMMON_H_
#define _AKREVERBUNITCOMMON_H_

#include <math.h>

#define ALLPASSCOEF (0.7f)

#if defined(WIN32) || defined(XBOX360) // Prefetch currently not supported on other platforms
#define USEPREFETCH	// Comment out not to use prefetch on supported platforms
#endif

#ifdef USEPREFETCH
#include "AkCrossPlatformSIMD.h"
#endif

#ifdef USEPREFETCH
AkForceInline static void PrefetchFilterMem( AkReal32 * pfStart, AkReal32 * AK_RESTRICT pfCur, AkReal32 * pfEnd, AkUInt32 uPrefetchSize )
{
	AkUInt32 uBytesToPrefetch = PluginMin( uPrefetchSize, AKSIMD_ARCHMAXPREFETCHSIZE );
	AkUInt32 uBytesBeforeWrap = AkUInt32((AkUInt8*)pfEnd - (AkUInt8*) pfCur);
	AkUInt32 uPretechBlockSize = PluginMin( uBytesBeforeWrap, uBytesToPrefetch );
	for( unsigned int i = 0; i < uPretechBlockSize; i += AKSIMD_ARCHCACHELINESIZE )	
	{
		AKSIMD_PREFETCHMEMORY(i,pfCur); 
	}

	if ( uPretechBlockSize < uBytesToPrefetch )
	{
		// Wrap and continue prefetch
		uBytesBeforeWrap = AkUInt32((AkUInt8*)pfEnd - (AkUInt8*) pfStart);
		uPretechBlockSize = PluginMin( uBytesBeforeWrap, uBytesToPrefetch-uPretechBlockSize );
		for( unsigned int i = 0; i < uPretechBlockSize; i += AKSIMD_ARCHCACHELINESIZE )	
		{
			AKSIMD_PREFETCHMEMORY(i,pfCur);
		}
	}
}
#else
AkForceInline static void PrefetchFilterMem( AkReal32 * pfStart, AkReal32 * AK_RESTRICT pfCur, AkReal32 * pfEnd, AkUInt32 uPrefetchSize )
{

}
#endif

#ifndef __SPU__
// Algorithm to determine if prime number
inline static bool IsPrime( AkInt32 in_lNum )
{
	if (in_lNum == 2)
		return true;	// 2 is only even number that is prime
	if (in_lNum & 1)	
	{	// Odd	
		// Only need to compute up to square root (math theorem)
		AkInt32 lStop = (AkInt32) sqrt((AkReal64) in_lNum) + 1; 
		for (AkInt32 i = 3; i < lStop; i+=2)
		{
			if ( (in_lNum % i) == 0) 
				return false;	// Can be divided by some number so not prime
		}
		return true; // Could not find dividors so its a prime number
	}
	else // even so not prime
		return false; 
}

// MakePrime
inline static AkUInt32 MakePrime( AkUInt32 in_ulIn )
{
	if ( (in_ulIn & 1) == 0) 
		in_ulIn++;	// If it is not odd it is not prime (even 2)
	while ( !IsPrime( in_ulIn ) ) 
	{
		in_ulIn += 2;	// Try the next odd number
	}
	return in_ulIn;
}

AkForceInline static AkReal32 CombFilterProcessFrame( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT pfCFMem, AkReal32 fCFFwdCoefs, AkReal32 fCFFbkCoefs)
{
	AkReal32 fXIn = *in_pfIn;				
	AkReal32 fMemIn = *pfCFMem;				
	AkReal32 fYn = fCFFwdCoefs * fXIn;			
	AkReal32 fFbk = fCFFbkCoefs * fMemIn;	
	fYn += fFbk;		
	*pfCFMem = fYn;	
	return fYn;
}

AkForceInline static AkReal32 AllPassFilterProcessFrame( AkReal32 * AK_RESTRICT in_pfIn, AkReal32 * AK_RESTRICT pfAPFMem )
{
	AkReal32 fXIn = *in_pfIn;
	AkReal32 fFwdMemIn = *pfAPFMem;
	AkReal32 fFbkMemIn = *(pfAPFMem+1);	
	AkReal32 fYn = ALLPASSCOEF * (fXIn - fFbkMemIn) + fFwdMemIn;	
	*pfAPFMem = fXIn;
	*(pfAPFMem+1) = fYn;
	return fYn;
}
#endif
#endif
