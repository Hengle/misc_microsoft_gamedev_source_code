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
// AkMacros24Bits.h
//
// Helpful macros for processing 24-bit audio data into 32 bit container
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFX_24BITMACROS_H_
#define _AKFX_24BITMACROS_H_

// Macros for 24-bit support
#ifdef WIN32
// Reconstruct 24-bit value inside 32 bit container (little endian)
#pragma warning( disable : 4244 )
#define READTRIBYTEINTOINT( plOut, pucIn )										\
	( (*(plOut)) = ( (*pucIn) | ((*(pucIn+1)) << 8) | ((*(pucIn+2)) << 16) ) );	\
	if ( *(plOut) & 0x00800000)													\
	{																			\
		*(plOut) = *(plOut) | 0xFF000000;										\
	}
#define WRITEINTINTOTRIBYTE( pucOut, plIn )										\
	(*pucOut) = ((*plIn) & 0xFF);												\
	(*(pucOut+1)) = (((*plIn) >> 8) & 0xFF);									\
	(*(pucOut+2)) = (((*plIn) >> 16) & 0xFF);	
#endif
	
#ifdef XBOX360
// Reconstruct 24-bit value inside 32 bit container (big endian)
#pragma warning( disable : 4244 )
#define READTRIBYTEINTOINT( plOut, pucIn )											\
	( (*(plOut)) = ( ((*(pucIn)) << 16) | ((*(pucIn+1)) << 8) | (*(pucIn+2)) ) );	\
	if ( *(plOut) & 0x00800000)														\
	{																				\
		*(plOut) = *(plOut) | 0xFF000000;											\
	}
#define WRITEINTINTOTRIBYTE( pucOut, plIn )											\
	(*pucOut) = (((*plIn) >> 16) & 0xFF);											\
	(*(pucOut+1)) = (((*plIn) >> 8) & 0xFF);										\
	(*(pucOut+2)) = ((*plIn) & 0xFF);	
#endif

#endif // _AKFX_24BITMACROS_H_