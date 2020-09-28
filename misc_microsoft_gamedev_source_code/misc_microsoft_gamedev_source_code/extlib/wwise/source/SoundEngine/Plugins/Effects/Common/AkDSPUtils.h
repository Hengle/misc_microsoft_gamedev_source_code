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
// AkDSPUtils.h
//
// DSP Utils header file.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DSPUTILS_H_
#define _AK_DSPUTILS_H_

#include <AK/Tools/Common/AkPlatformFuncs.h>

#if defined WIN32
#include "xmmintrin.h"
#elif defined XBOX360
#include "ppcintrinsics.h"
static __declspec(align(16)) AkReal32 aVolumes[4];
#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA ) \
	aVolumes[0] = VOLUME; \
	aVolumes[1] = VOLUME; \
	aVolumes[2] = VOLUME + VOLUME_DELTA; \
	aVolumes[3] = VOLUME + VOLUME_DELTA; \
	__vector4 VECT = __lvx( aVolumes, 0 );
#elif defined __PPU__
#include "altivec.h"
#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA )	\
	 __attribute__ ((aligned(128))) vector float VECT;		\
	*(float*)&VECT = VOLUME;								\
	*((float*)&VECT + 1) = VOLUME;							\
	*((float*)&VECT + 2) = VOLUME + VOLUME_DELTA;			\
	*((float*)&VECT + 3) = VOLUME + VOLUME_DELTA;
#elif defined __SPU__
#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA )	\
	 __attribute__ ((aligned(128))) vec_float4 VECT;		\
	*(float*)&VECT = VOLUME;								\
	*((float*)&VECT + 1) = VOLUME;							\
	*((float*)&VECT + 2) = VOLUME + VOLUME_DELTA;			\
	*((float*)&VECT + 3) = VOLUME + VOLUME_DELTA;
#else
#error Platform not defined
#endif

void AkForceInline RemoveDenormal( AkReal32 & fVal )
{
	// if the whole calculation is done in the FPU registers, 
	// a 80-bit arithmetic may be used, with 64-bit mantissas. The
	// anti_denormal value should therefore be 2^64 times higher than 
	// FLT_MIN. On the other hand, if everything is done with a 32-bit accuracy, 
	// one may reduce the anti_denormal value to get a better
	// accuracy for the small values. (FLT_MIN == 1.175494351e-38F) 

	static const AkReal32 fAntiDenormal = 1e-18f;
	fVal += fAntiDenormal;
	fVal -= fAntiDenormal;
}

AkForceInline void AddAndMultiply(AkReal32 *in_pInBuffer1, AkReal32 *in_pInBuffer2, AkReal32 in_fConst, AkReal32 *in_pOutBuffer, AkUInt32 in_ulNumFrames )
{
	unsigned int i = in_ulNumFrames / 4;
	unsigned int iFramesRemaining = in_ulNumFrames - (i*4);

#if defined WIN32
	__m128 vVolumes = _mm_load1_ps( &in_fConst );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i]) * in_fConst;

		__m128 vSum = _mm_load_ps( in_pInBuffer1 );
		__m128 vBuffer2 = _mm_load_ps( in_pInBuffer2 );
		vSum = _mm_add_ps( vBuffer2, vSum );
		vSum = _mm_mul_ps( vSum, vVolumes );

		_mm_store_ps( in_pOutBuffer, vSum );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pOutBuffer += 4;
	}

#elif defined XBOX360

	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i]) * in_fConst;

		__vector4 vSum = __lvx( in_pInBuffer1, 0 );
		__vector4 vBuffer2 = __lvx( in_pInBuffer2, 0 );
		vSum = __vaddfp( vBuffer2, vSum );
		vSum = __vmulfp( vSum, vVolumes );

		__stvx( vSum, in_pOutBuffer, 0 );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pOutBuffer += 4;
	}

#elif defined __PPU__

	const vector float vMinusZero = { -0.0f, -0.0f, -0.0f, -0.0f };
	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i]) * in_fConst;

		register vector float vSum			= vec_ld( 0, in_pInBuffer1 );
		register vector float vBufferToAdd	= vec_ld( 0, in_pInBuffer2 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vSum								= vec_madd( vSum, vVolumes, vMinusZero );
		vec_st( vSum, 0, in_pOutBuffer );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pOutBuffer += 4;
	}

#elif defined __SPU__

	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i]) * in_fConst;

		register vec_float4 vSum			= *(vec_float4*)in_pInBuffer1;
		register vec_float4 vBufferToAdd	= *(vec_float4*)in_pInBuffer2;
		vSum								= spu_madd( vSum, vVolumes, vBufferToAdd );
		*(vec_float4*)in_pOutBuffer			= vSum;

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pOutBuffer += 4;
	}

#endif

	// process remaining samples
	while(iFramesRemaining--)
		*(in_pOutBuffer++) = ( *(in_pInBuffer1++) + *(in_pInBuffer2++) ) * in_fConst;
}

AkForceInline void AddAndMultiply(AkReal32 *in_pInBuffer1, 
								  AkReal32 *in_pInBuffer2, 
								  AkReal32 *in_pInBuffer3, 
								  AkReal32 *in_pInBuffer4, 
								  AkReal32 *in_pInBuffer5, 
								  AkReal32 in_fConst, 
								  AkReal32 *in_pOutBuffer, 
								  AkUInt32 in_ulNumFrames )
{
	unsigned int i = in_ulNumFrames / 4;
	unsigned int iFramesRemaining = in_ulNumFrames - (i*4);

#if defined WIN32
	__m128 vVolumes = _mm_load1_ps( &in_fConst );
	while(i)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i]) * in_fConst;

		__m128 vSum = _mm_load_ps( in_pInBuffer1 );
		__m128 vBufferToAdd = _mm_load_ps( in_pInBuffer2 );
		vSum = _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd = _mm_load_ps( in_pInBuffer3 );
		vSum = _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd = _mm_load_ps( in_pInBuffer4 );
		vSum = _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd = _mm_load_ps( in_pInBuffer5 );
		vSum = _mm_add_ps( vBufferToAdd, vSum );
		vSum = _mm_mul_ps( vSum, vVolumes );

		_mm_store_ps( in_pOutBuffer, vSum );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pOutBuffer += 4;
		--i;
	}

#elif defined XBOX360

	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i]) * in_fConst;

		__vector4 vSum = __lvx( in_pInBuffer1, 0 );
		__vector4 vBufferToAdd = __lvx( in_pInBuffer2, 0 );
		vSum = __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd = __lvx( in_pInBuffer3, 0 );
		vSum = __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd = __lvx( in_pInBuffer4, 0 );
		vSum = __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd = __lvx( in_pInBuffer5, 0 );
		vSum = __vaddfp( vBufferToAdd, vSum );
		vSum = __vmulfp( vSum, vVolumes );

		__stvx( vSum, in_pOutBuffer, 0 );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pOutBuffer += 4;
	}
	
#elif defined __PPU__

	const vector float vMinusZero = { -0.0f, -0.0f, -0.0f, -0.0f };
	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i]) * in_fConst;

		register vector float vSum			= vec_ld( 0, in_pInBuffer1 );
		register vector float vBufferToAdd	= vec_ld( 0, in_pInBuffer2 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer3 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer4 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer5 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vSum								= vec_madd( vSum, vVolumes, vMinusZero );
		vec_st( vSum, 0, in_pOutBuffer );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pOutBuffer += 4;
	}

#elif defined __SPU__

	const vector float vMinusZero = { -0.0f, -0.0f, -0.0f, -0.0f };
	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i]) * in_fConst;

		register vec_float4 vSum			= *(vec_float4*)in_pInBuffer1;
		register vec_float4 vBufferToAdd	= *(vec_float4*)in_pInBuffer2;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer3;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer4;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer5;
		vSum								= spu_add( vBufferToAdd, vSum );
		vSum								= spu_mul( vSum, vVolumes );
		*(vec_float4*)in_pOutBuffer = vSum;

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pOutBuffer += 4;
	}

#endif

	// process remaining samples
	while(iFramesRemaining--)
		*(in_pOutBuffer++) = ( *(in_pInBuffer1++) + *(in_pInBuffer2++) + *(in_pInBuffer3++) + *(in_pInBuffer4++) + *(in_pInBuffer5++) ) * in_fConst;

}

AkForceInline void AddAndMultiply(AkReal32 *in_pInBuffer1,
								  AkReal32 *in_pInBuffer2,
								  AkReal32 *in_pInBuffer3,
								  AkReal32 *in_pInBuffer4,
								  AkReal32 *in_pInBuffer5,
								  AkReal32 *in_pInBuffer6,
								  AkReal32 in_fConst, 
								  AkReal32 *in_pOutBuffer, 
								  AkUInt32 in_ulNumFrames )
{
	unsigned int i = in_ulNumFrames / 4;
	unsigned int iFramesRemaining = in_ulNumFrames - (i*4);

#if defined WIN32
	
	__m128 vVolumes = _mm_load1_ps( &in_fConst );
	while(i)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i] + in_pInBuffer6[i] + in_pInBuffer7[i]) * in_fConst;

		__m128 vSum			= _mm_load_ps( in_pInBuffer1 );
		__m128 vBufferToAdd	= _mm_load_ps( in_pInBuffer2 );
		vSum				= _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd		= _mm_load_ps( in_pInBuffer3 );
		vSum				= _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd		= _mm_load_ps( in_pInBuffer4 );
		vSum				= _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd		= _mm_load_ps( in_pInBuffer5 );
		vSum				= _mm_add_ps( vBufferToAdd, vSum );
		vBufferToAdd		= _mm_load_ps( in_pInBuffer6 );
		vSum				= _mm_add_ps( vBufferToAdd, vSum );
		vSum				= _mm_mul_ps( vSum, vVolumes );
		_mm_store_ps( in_pOutBuffer, vSum );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pInBuffer6 += 4;
		in_pOutBuffer += 4;
		--i;
	}

#elif defined XBOX360

	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i] + in_pInBuffer6[i] + in_pInBuffer7[i]) * in_fConst;

		__vector4 vSum			= __lvx( in_pInBuffer1, 0 );
		__vector4 vBufferToAdd	= __lvx( in_pInBuffer2, 0 );
		vSum					= __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd			= __lvx( in_pInBuffer3, 0 );
		vSum					= __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd			= __lvx( in_pInBuffer4, 0 );
		vSum					= __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd			= __lvx( in_pInBuffer5, 0 );
		vSum					= __vaddfp( vBufferToAdd, vSum );
		vBufferToAdd			= __lvx( in_pInBuffer6, 0 );
		vSum					= __vaddfp( vBufferToAdd, vSum );
		vSum					= __vmulfp( vSum, vVolumes );
		__stvx( vSum, in_pOutBuffer, 0 );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pInBuffer6 += 4;
		in_pOutBuffer += 4;
	}

#elif defined __PPU__

	const vector float vMinusZero = { -0.0f, -0.0f, -0.0f, -0.0f };
	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i] + in_pInBuffer6[i] + in_pInBuffer7[i]) * in_fConst;

		register vector float vSum			= vec_ld( 0, in_pInBuffer1 );
		register vector float vBufferToAdd	= vec_ld( 0, in_pInBuffer2 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer3 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer4 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer5 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vBufferToAdd						= vec_ld( 0, in_pInBuffer6 );
		vSum								= vec_add( vBufferToAdd, vSum );
		vSum								= vec_madd( vSum, vVolumes, vMinusZero );
		vec_st( vSum, 0, in_pOutBuffer );

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pInBuffer6 += 4;
		in_pOutBuffer += 4;
	}

#elif defined __SPU__

	BUILD_VOLUME_VECTOR( vVolumes, in_fConst, 0 );
	while(i--)
	{
		//in_pOutBuffer[i] = (in_pInBuffer1[i] + in_pInBuffer2[i] + in_pInBuffer3[i] + in_pInBuffer4[i] + in_pInBuffer5[i] + in_pInBuffer6[i] + in_pInBuffer7[i]) * in_fConst;

		register vec_float4 vSum			= *(vec_float4*)in_pInBuffer1;
		register vec_float4 vBufferToAdd	= *(vec_float4*)in_pInBuffer2;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer3;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer4;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer5;
		vSum								= spu_add( vBufferToAdd, vSum );
		vBufferToAdd						= *(vec_float4*)in_pInBuffer6;
		vSum								= spu_add( vBufferToAdd, vSum );
		vSum								= spu_mul( vSum, vVolumes );
		*(vec_float4*)in_pOutBuffer = vSum;

		in_pInBuffer1 += 4;
		in_pInBuffer2 += 4;
		in_pInBuffer3 += 4;
		in_pInBuffer4 += 4;
		in_pInBuffer5 += 4;
		in_pInBuffer6 += 4;
		in_pOutBuffer += 4;
	}

#endif

	// process remaining samples
	while(iFramesRemaining--)
	{
		*(in_pOutBuffer++) = ( *(in_pInBuffer1++)
								+ *(in_pInBuffer2++)
								+ *(in_pInBuffer3++)
								+ *(in_pInBuffer4++)
								+ *(in_pInBuffer5++)
								+ *(in_pInBuffer6++) ) * in_fConst;
	}
}

#endif //_AK_DSPUTILS_H_
