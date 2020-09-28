/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h" 
#include "AkResamplerCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "vectorintrinsics.h"

// Platform specific defines for prefetching
#define USEPITCHPREFETCH 
#define AKSIMD_ARCHCACHELINESIZE (128)	
#define AKSIMD_ARCHMAXPREFETCHSIZE (1024)
#define AKSIMD_PREFETCHMEMORY( __offset__, __add__ ) __dcbt( __offset__, (const void *)__add__ )

/********************* BYPASS DSP ROUTINES **********************/

// Bypass (no pitch or resampling) with signed 16-bit samples vectorized for any number of channels.
// As opposed to other routines, this only does the format conversion and deinterleaving is performed once 
// whole buffer is ready to be sent downstream to the pipeline
AKRESULT Bypass_I16_NChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pIn );
#endif

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4Shift = { 2.2421e-044f /*16*/, 2.2421e-044f /*16*/, 2.2421e-044f /*16*/, 2.2421e-044f /*16*/ };
	__vector4 * AK_RESTRICT pv4In = (__vector4 * AK_RESTRICT) pIn;
	__vector4 * AK_RESTRICT pv4Out = (__vector4 * AK_RESTRICT) pOut;

	// Note: Cannot assume input data alignment is on 16 byte boundaries
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	// Note: Generated code is much slower not aligned so handle all 4 possible cases
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);
	const __vector4 * pv4InEnd = (__vector4*)(pv4In + 2*uNumIter);

	if ( !((AkUInt32)pv4In & 0x0000000F) && !((AkUInt32)pv4Out & 0x0000000F) ) // Input and output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = *pv4In++;
			__vector4 v4InSamples2 = *pv4In++;
			__vector4 v4Shuf1 = __vmrghh( v4InSamples1, v4Zero );
			__vector4 v4Shuf2 = __vmrglh( v4InSamples1, v4Zero );
			__vector4 v4Shuf3 = __vmrghh( v4InSamples2, v4Zero );
			__vector4 v4Shuf4 = __vmrglh( v4InSamples2, v4Zero );	
			__vector4 v4Shifted1 = __vsraw( v4Shuf1, v4Shift );
			__vector4 v4Shifted2 = __vsraw( v4Shuf2, v4Shift );
			__vector4 v4Shifted3 = __vsraw( v4Shuf3, v4Shift );
			__vector4 v4Shifted4 = __vsraw( v4Shuf4, v4Shift );
			__vector4 v4Out1 = __vcsxwfp( v4Shifted1, 15 );		
			__vector4 v4Out2 = __vcsxwfp( v4Shifted2, 15 );
			__vector4 v4Out3 = __vcsxwfp( v4Shifted3, 15 );
			__vector4 v4Out4 = __vcsxwfp( v4Shifted4, 15 );
			*pv4Out++ = v4Out1;
			*pv4Out++ = v4Out2;
			*pv4Out++ = v4Out3;
			*pv4Out++ = v4Out4;
		}
	}
	else if ( !((AkUInt32)pv4In & 0x0000000F)  ) // Only input aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = *pv4In++;
			__vector4 v4InSamples2 = *pv4In++;
			__vector4 v4Shuf1 = __vmrghh( v4InSamples1, v4Zero );
			__vector4 v4Shuf2 = __vmrglh( v4InSamples1, v4Zero );
			__vector4 v4Shuf3 = __vmrghh( v4InSamples2, v4Zero );
			__vector4 v4Shuf4 = __vmrglh( v4InSamples2, v4Zero );	
			__vector4 v4Shifted1 = __vsraw( v4Shuf1, v4Shift );
			__vector4 v4Shifted2 = __vsraw( v4Shuf2, v4Shift );
			__vector4 v4Shifted3 = __vsraw( v4Shuf3, v4Shift );
			__vector4 v4Shifted4 = __vsraw( v4Shuf4, v4Shift );
			__vector4 v4Out1 = __vcsxwfp( v4Shifted1, 15 );		
			__vector4 v4Out2 = __vcsxwfp( v4Shifted2, 15 );
			__vector4 v4Out3 = __vcsxwfp( v4Shifted3, 15 );
			__vector4 v4Out4 = __vcsxwfp( v4Shifted4, 15 );
			__storeunalignedvector( v4Out1, &pv4Out[0] );
			__storeunalignedvector( v4Out2, &pv4Out[1] );
			__storeunalignedvector( v4Out3, &pv4Out[2] );
			__storeunalignedvector( v4Out4, &pv4Out[3] );
			pv4Out += 4;
		}
	}
	else if ( !((AkUInt32)pv4Out & 0x0000000F) ) // Only output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = __loadunalignedvector( &pv4In[0] );
			__vector4 v4InSamples2 = __loadunalignedvector( &pv4In[1] );
			pv4In += 2;
			__vector4 v4Shuf1 = __vmrghh( v4InSamples1, v4Zero );
			__vector4 v4Shuf2 = __vmrglh( v4InSamples1, v4Zero );
			__vector4 v4Shuf3 = __vmrghh( v4InSamples2, v4Zero );
			__vector4 v4Shuf4 = __vmrglh( v4InSamples2, v4Zero );	
			__vector4 v4Shifted1 = __vsraw( v4Shuf1, v4Shift );
			__vector4 v4Shifted2 = __vsraw( v4Shuf2, v4Shift );
			__vector4 v4Shifted3 = __vsraw( v4Shuf3, v4Shift );
			__vector4 v4Shifted4 = __vsraw( v4Shuf4, v4Shift );
			__vector4 v4Out1 = __vcsxwfp( v4Shifted1, 15 );		
			__vector4 v4Out2 = __vcsxwfp( v4Shifted2, 15 );
			__vector4 v4Out3 = __vcsxwfp( v4Shifted3, 15 );
			__vector4 v4Out4 = __vcsxwfp( v4Shifted4, 15 );
			*pv4Out++ = v4Out1;
			*pv4Out++ = v4Out2;
			*pv4Out++ = v4Out3;
			*pv4Out++ = v4Out4;
		}
	}
	else // Nothing aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = __loadunalignedvector( &pv4In[0] );
			__vector4 v4InSamples2 = __loadunalignedvector( &pv4In[1] );
			pv4In += 2;
			__vector4 v4Shuf1 = __vmrghh( v4InSamples1, v4Zero );
			__vector4 v4Shuf2 = __vmrglh( v4InSamples1, v4Zero );
			__vector4 v4Shuf3 = __vmrghh( v4InSamples2, v4Zero );
			__vector4 v4Shuf4 = __vmrglh( v4InSamples2, v4Zero );	
			__vector4 v4Shifted1 = __vsraw( v4Shuf1, v4Shift );
			__vector4 v4Shifted2 = __vsraw( v4Shuf2, v4Shift );
			__vector4 v4Shifted3 = __vsraw( v4Shuf3, v4Shift );
			__vector4 v4Shifted4 = __vsraw( v4Shuf4, v4Shift );
			__vector4 v4Out1 = __vcsxwfp( v4Shifted1, 15 );		
			__vector4 v4Out2 = __vcsxwfp( v4Shifted2, 15 );
			__vector4 v4Out3 = __vcsxwfp( v4Shifted3, 15 );
			__vector4 v4Out4 = __vcsxwfp( v4Shifted4, 15 );
			__storeunalignedvector( v4Out1, &pv4Out[0] );
			__storeunalignedvector( v4Out2, &pv4Out[1] );
			__storeunalignedvector( v4Out3, &pv4Out[2] );
			__storeunalignedvector( v4Out4, &pv4Out[3] );
			pv4Out += 4;
		}
	}

	// Advance data pointers for remaining samples
	pIn = (AkInt16 * AK_RESTRICT) pv4In;
	pOut = (AkReal32 * AK_RESTRICT) pv4Out;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOut++ = INT16_TO_FLOAT( *pIn++ );
	}

	// Need to keep the last buffer value in case we start the pitch algo next buffer for each channel
	pIn -= uNumChannels;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		io_pPitchState->iLastValue[i] = pIn[i];
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}


// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals. 
AKRESULT Bypass_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOutL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pOutR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pIn );
#endif

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->iLastValue[0] = pIn[uLastSample*2];
	io_pPitchState->iLastValue[1] = pIn[uLastSample*2+1];

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4Shift = { 2.2421e-044f /*16*/, 2.2421e-044f /*16*/, 2.2421e-044f /*16*/, 2.2421e-044f /*16*/ };
	static const __vector4 v4ShuffleL = { 9.7598e-041f /*0x00011010*/, 1.56414504e-036f /*0x04051010*/, 4.12458193e-034f /*0x08091010*/, 1.08670785e-031f /*0x0C0D1010*/ };
	static const __vector4 v4ShuffleR = { 9.62896971e-038f /*0x02031010*/, 2.54024788e-035f /*0x06071010*/, 6.69562758e-033f /*0x0A0B1010*/, 1.76338447e-030f /*0x0E0F1010*/ };
	
	__vector4 * AK_RESTRICT pv4In = (__vector4 * AK_RESTRICT) pIn;
	__vector4 * AK_RESTRICT pv4OutL = (__vector4 * AK_RESTRICT) pOutL;
	__vector4 * AK_RESTRICT pv4OutR = (__vector4 * AK_RESTRICT) pOutR;

	// Note: Cannot assume input data alignment is on 16 byte boundaries
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	// Note: Generated code is much slower not aligned so handle all 4 possible cases

	AkUInt32 uNumIter = uFramesToCopy / 8;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*8);
	const __vector4 * pv4InEnd = (__vector4 *)(pv4In + 2*uNumIter);

	if ( !((AkUInt32)pv4In & 0x0000000F) && !((AkUInt32)pv4OutL & 0x0000000F) ) // Input and output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = *pv4In++;
			__vector4 v4InSamples2 = *pv4In++;
			__vector4 v4ShufL1 = __vperm( v4InSamples1, v4Zero, v4ShuffleL );
			__vector4 v4ShufR1 = __vperm( v4InSamples1, v4Zero, v4ShuffleR );
			__vector4 v4ShufL2 = __vperm( v4InSamples2, v4Zero, v4ShuffleL );
			__vector4 v4ShufR2 = __vperm( v4InSamples2, v4Zero, v4ShuffleR );	
			__vector4 v4ShiftedL1 = __vsraw( v4ShufL1, v4Shift );
			__vector4 v4ShiftedR1 = __vsraw( v4ShufR1, v4Shift );
			__vector4 v4ShiftedL2 = __vsraw( v4ShufL2, v4Shift );
			__vector4 v4ShiftedR2 = __vsraw( v4ShufR2, v4Shift );
			__vector4 v4OutL1 = __vcsxwfp( v4ShiftedL1, 15 );		
			__vector4 v4OutR1 = __vcsxwfp( v4ShiftedR1, 15 );
			__vector4 v4OutL2 = __vcsxwfp( v4ShiftedL2, 15 );
			__vector4 v4OutR2 = __vcsxwfp( v4ShiftedR2, 15 );
			*pv4OutL++ = v4OutL1;
			*pv4OutR++ = v4OutR1;
			*pv4OutL++ = v4OutL2;		
			*pv4OutR++ = v4OutR2;
		}
	}
	else if ( !((AkUInt32)pv4In & 0x0000000F)  ) // Only input aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = *pv4In++;
			__vector4 v4InSamples2 = *pv4In++;
			__vector4 v4ShufL1 = __vperm( v4InSamples1, v4Zero, v4ShuffleL );
			__vector4 v4ShufR1 = __vperm( v4InSamples1, v4Zero, v4ShuffleR );
			__vector4 v4ShufL2 = __vperm( v4InSamples2, v4Zero, v4ShuffleL );
			__vector4 v4ShufR2 = __vperm( v4InSamples2, v4Zero, v4ShuffleR );	
			__vector4 v4ShiftedL1 = __vsraw( v4ShufL1, v4Shift );
			__vector4 v4ShiftedR1 = __vsraw( v4ShufR1, v4Shift );
			__vector4 v4ShiftedL2 = __vsraw( v4ShufL2, v4Shift );
			__vector4 v4ShiftedR2 = __vsraw( v4ShufR2, v4Shift );
			__vector4 v4OutL1 = __vcsxwfp( v4ShiftedL1, 15 );		
			__vector4 v4OutR1 = __vcsxwfp( v4ShiftedR1, 15 );
			__vector4 v4OutL2 = __vcsxwfp( v4ShiftedL2, 15 );
			__vector4 v4OutR2 = __vcsxwfp( v4ShiftedR2, 15 );
			__storeunalignedvector( v4OutL1, &pv4OutL[0] );
			__storeunalignedvector( v4OutR1, &pv4OutR[0] );
			__storeunalignedvector( v4OutL2, &pv4OutL[1] );
			__storeunalignedvector( v4OutR2, &pv4OutR[1] );
			pv4OutL += 2;
			pv4OutR += 2;
		}
	}
	else if ( !((AkUInt32)pv4OutL & 0x0000000F) ) // Only output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = __loadunalignedvector( &pv4In[0] );
			__vector4 v4InSamples2 = __loadunalignedvector( &pv4In[1] );
			pv4In += 2;
			__vector4 v4ShufL1 = __vperm( v4InSamples1, v4Zero, v4ShuffleL );
			__vector4 v4ShufR1 = __vperm( v4InSamples1, v4Zero, v4ShuffleR );
			__vector4 v4ShufL2 = __vperm( v4InSamples2, v4Zero, v4ShuffleL );
			__vector4 v4ShufR2 = __vperm( v4InSamples2, v4Zero, v4ShuffleR );	
			__vector4 v4ShiftedL1 = __vsraw( v4ShufL1, v4Shift );
			__vector4 v4ShiftedR1 = __vsraw( v4ShufR1, v4Shift );
			__vector4 v4ShiftedL2 = __vsraw( v4ShufL2, v4Shift );
			__vector4 v4ShiftedR2 = __vsraw( v4ShufR2, v4Shift );
			__vector4 v4OutL1 = __vcsxwfp( v4ShiftedL1, 15 );		
			__vector4 v4OutR1 = __vcsxwfp( v4ShiftedR1, 15 );
			__vector4 v4OutL2 = __vcsxwfp( v4ShiftedL2, 15 );
			__vector4 v4OutR2 = __vcsxwfp( v4ShiftedR2, 15 );
			*pv4OutL++ = v4OutL1;
			*pv4OutL++ = v4OutL2;
			*pv4OutR++ = v4OutR1;				
			*pv4OutR++ = v4OutR2;
		}
	}
	else // Nothing aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples1 = __loadunalignedvector( &pv4In[0] );
			__vector4 v4InSamples2 = __loadunalignedvector( &pv4In[1] );
			pv4In += 2;
			__vector4 v4ShufL1 = __vperm( v4InSamples1, v4Zero, v4ShuffleL );
			__vector4 v4ShufR1 = __vperm( v4InSamples1, v4Zero, v4ShuffleR );
			__vector4 v4ShufL2 = __vperm( v4InSamples2, v4Zero, v4ShuffleL );
			__vector4 v4ShufR2 = __vperm( v4InSamples2, v4Zero, v4ShuffleR );	
			__vector4 v4ShiftedL1 = __vsraw( v4ShufL1, v4Shift );
			__vector4 v4ShiftedR1 = __vsraw( v4ShufR1, v4Shift );
			__vector4 v4ShiftedL2 = __vsraw( v4ShufL2, v4Shift );
			__vector4 v4ShiftedR2 = __vsraw( v4ShufR2, v4Shift );
			__vector4 v4OutL1 = __vcsxwfp( v4ShiftedL1, 15 );		
			__vector4 v4OutR1 = __vcsxwfp( v4ShiftedR1, 15 );
			__vector4 v4OutL2 = __vcsxwfp( v4ShiftedL2, 15 );
			__vector4 v4OutR2 = __vcsxwfp( v4ShiftedR2, 15 );
			__storeunalignedvector( v4OutL1, &pv4OutL[0] );
			__storeunalignedvector( v4OutR1, &pv4OutR[0] );
			__storeunalignedvector( v4OutL2, &pv4OutL[1] );
			__storeunalignedvector( v4OutR2, &pv4OutR[1] );
			pv4OutL += 2;
			pv4OutR += 2;
		}
	}

	// Advance data pointers for remaining samples
	pIn = (AkInt16 * AK_RESTRICT) pv4In;
	pOutL = (AkReal32 * AK_RESTRICT) pv4OutL;
	pOutR = (AkReal32 * AK_RESTRICT) pv4OutR;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOutL++ = INT16_TO_FLOAT( *pIn++ );
		*pOutR++ = INT16_TO_FLOAT( *pIn++ );
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

// Bypass (no pitch or resampling) with unsigned 8-bit samples vectorized for any number of channels.
// As opposed to other routines, this only does the format conversion and deinterleaving is performed once 
// whole buffer is ready to be sent downstream to the pipeline
AKRESULT Bypass_U8_NChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt8 * AK_RESTRICT pIn = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + io_pPitchState->uInFrameOffset*uNumChannels;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData() ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pIn );
#endif

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };
	static const __vector4 v4ShuffleA = { 2.84113185e-029f /*0x10101000*/, 2.84113215e-029f /*0x10101001*/, 2.84113246e-029f /*0x10101002*/, 2.84113276e-029f /*0x10101003*/ }; 
	static const __vector4 v4ShuffleB = { 2.84113306e-029f /*0x10101004*/, 2.84113336e-029f /*0x10101005*/, 2.84113366e-029f /*0x10101006*/, 2.84113396e-029f /*0x10101007*/ };
	static const __vector4 v4ShuffleC =	{ 2.84113426e-029f /*0x10101008*/, 2.84113456e-029f /*0x10101009*/, 2.84113486e-029f /*0x1010100A*/, 2.84113516e-029f /*0x1010100B*/ };	
	static const __vector4 v4ShuffleD = { 2.84113547e-029f /*0x1010100C*/, 2.84113577e-029f /*0x1010100D*/, 2.84113607e-029f /*0x1010100E*/, 2.84113637e-029f /*0x1010100F*/ };

	__vector4 * AK_RESTRICT pv4In = (__vector4 * AK_RESTRICT) pIn;
	__vector4 * AK_RESTRICT pv4Out = (__vector4 * AK_RESTRICT) pOut;

	// Note: Cannot assume input data alignment is on 16 byte boundaries
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	// Note: Generated code is much slower not aligned so handle all 4 possible cases
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);
	const __vector4 * pv4InEnd = (__vector4 *)(pv4In + uNumIter);

	if ( !((AkUInt32)pv4In & 0x0000000F) && !((AkUInt32)pv4Out & 0x0000000F) ) // Input and output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = *pv4In++;
			__vector4 v4Shuf1 = __vperm( v4InSamples, v4Zero, v4ShuffleA );
			__vector4 v4Shuf2 = __vperm( v4InSamples, v4Zero, v4ShuffleB );
			__vector4 v4Shuf3 = __vperm( v4InSamples, v4Zero, v4ShuffleC );
			__vector4 v4Shuf4 = __vperm( v4InSamples, v4Zero, v4ShuffleD );
			__vector4 v4Converted1 = __vcuxwfp( v4Shuf1, 7 );
			__vector4 v4Converted2 = __vcuxwfp( v4Shuf2, 7 );
			__vector4 v4Converted3 = __vcuxwfp( v4Shuf3, 7 );
			__vector4 v4Converted4 = __vcuxwfp( v4Shuf4, 7 );
			__vector4 v4Out1 = __vsubfp( v4Converted1, v4UnsignedOffset );
			__vector4 v4Out2 = __vsubfp( v4Converted2, v4UnsignedOffset );
			__vector4 v4Out3 = __vsubfp( v4Converted3, v4UnsignedOffset );
			__vector4 v4Out4 = __vsubfp( v4Converted4, v4UnsignedOffset );
			*pv4Out++ = v4Out1;
			*pv4Out++ = v4Out2;
			*pv4Out++ = v4Out3;
			*pv4Out++ = v4Out4;
		}
	}
	else if ( !((AkUInt32)pv4In & 0x0000000F)  ) // Only input aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = *pv4In++;
			__vector4 v4Shuf1 = __vperm( v4InSamples, v4Zero, v4ShuffleA );
			__vector4 v4Shuf2 = __vperm( v4InSamples, v4Zero, v4ShuffleB );
			__vector4 v4Shuf3 = __vperm( v4InSamples, v4Zero, v4ShuffleC );
			__vector4 v4Shuf4 = __vperm( v4InSamples, v4Zero, v4ShuffleD );
			__vector4 v4Converted1 = __vcuxwfp( v4Shuf1, 7 );
			__vector4 v4Converted2 = __vcuxwfp( v4Shuf2, 7 );
			__vector4 v4Converted3 = __vcuxwfp( v4Shuf3, 7 );
			__vector4 v4Converted4 = __vcuxwfp( v4Shuf4, 7 );
			__vector4 v4Out1 = __vsubfp( v4Converted1, v4UnsignedOffset );
			__vector4 v4Out2 = __vsubfp( v4Converted2, v4UnsignedOffset );
			__vector4 v4Out3 = __vsubfp( v4Converted3, v4UnsignedOffset );
			__vector4 v4Out4 = __vsubfp( v4Converted4, v4UnsignedOffset );
			__storeunalignedvector( v4Out1, &pv4Out[0] );
			__storeunalignedvector( v4Out2, &pv4Out[1] );
			__storeunalignedvector( v4Out3, &pv4Out[2] );
			__storeunalignedvector( v4Out4, &pv4Out[3] );
			pv4Out += 4;
		}
	}
	else if ( !((AkUInt32)pv4Out & 0x0000000F) ) // Only output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = __loadunalignedvector( pv4In );
			++pv4In;
			__vector4 v4Shuf1 = __vperm( v4InSamples, v4Zero, v4ShuffleA );
			__vector4 v4Shuf2 = __vperm( v4InSamples, v4Zero, v4ShuffleB );
			__vector4 v4Shuf3 = __vperm( v4InSamples, v4Zero, v4ShuffleC );
			__vector4 v4Shuf4 = __vperm( v4InSamples, v4Zero, v4ShuffleD );
			__vector4 v4Converted1 = __vcuxwfp( v4Shuf1, 7 );
			__vector4 v4Converted2 = __vcuxwfp( v4Shuf2, 7 );
			__vector4 v4Converted3 = __vcuxwfp( v4Shuf3, 7 );
			__vector4 v4Converted4 = __vcuxwfp( v4Shuf4, 7 );
			__vector4 v4Out1 = __vsubfp( v4Converted1, v4UnsignedOffset );
			__vector4 v4Out2 = __vsubfp( v4Converted2, v4UnsignedOffset );
			__vector4 v4Out3 = __vsubfp( v4Converted3, v4UnsignedOffset );
			__vector4 v4Out4 = __vsubfp( v4Converted4, v4UnsignedOffset );
			*pv4Out++ = v4Out1;
			*pv4Out++ = v4Out2;
			*pv4Out++ = v4Out3;
			*pv4Out++ = v4Out4;
		}
	}
	else // Nothing aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = __loadunalignedvector( pv4In );
			++pv4In;
			__vector4 v4Shuf1 = __vperm( v4InSamples, v4Zero, v4ShuffleA );
			__vector4 v4Shuf2 = __vperm( v4InSamples, v4Zero, v4ShuffleB );
			__vector4 v4Shuf3 = __vperm( v4InSamples, v4Zero, v4ShuffleC );
			__vector4 v4Shuf4 = __vperm( v4InSamples, v4Zero, v4ShuffleD );
			__vector4 v4Converted1 = __vcuxwfp( v4Shuf1, 7 );
			__vector4 v4Converted2 = __vcuxwfp( v4Shuf2, 7 );
			__vector4 v4Converted3 = __vcuxwfp( v4Shuf3, 7 );
			__vector4 v4Converted4 = __vcuxwfp( v4Shuf4, 7 );
			__vector4 v4Out1 = __vsubfp( v4Converted1, v4UnsignedOffset );
			__vector4 v4Out2 = __vsubfp( v4Converted2, v4UnsignedOffset );
			__vector4 v4Out3 = __vsubfp( v4Converted3, v4UnsignedOffset );
			__vector4 v4Out4 = __vsubfp( v4Converted4, v4UnsignedOffset );
			__storeunalignedvector( v4Out1, &pv4Out[0] );
			__storeunalignedvector( v4Out2, &pv4Out[1] );
			__storeunalignedvector( v4Out3, &pv4Out[2] );
			__storeunalignedvector( v4Out4, &pv4Out[3] );
			pv4Out += 4;;
		}
	}

	// Advance data pointers for remaining values
	pIn = (AkUInt8 * AK_RESTRICT) pv4In;
	pOut = (AkReal32 * AK_RESTRICT) pv4Out;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOut++ = UINT8_TO_FLOAT( *pIn++ );
	}

	// Need to keep the last buffer value in case we start the pitch algo next buffer for each channel
	pIn -= uNumChannels;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		io_pPitchState->uLastValue[i] = pIn[i];
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

// Bypass (no pitch or resampling) with INTERLEAVED unsigned 8-bit samples optimized for 2 channel signals.
AKRESULT Bypass_U8_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	AkUInt8 * AK_RESTRICT pIn = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOutL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pOutR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pIn );
#endif

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->uLastValue[0] = pIn[uLastSample*2];
	io_pPitchState->uLastValue[1] = pIn[uLastSample*2+1];

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };

	static const __vector4 v4ShuffleL1 = { 2.84113185e-029f /*0x10101000*/, 2.84113246e-029f /*0x10101002*/, 2.84113306e-029f /*0x10101004*/, 2.84113366e-029f /*0x10101006*/ };
	static const __vector4 v4ShuffleL2 = { 2.84113426e-029f /*0x10101008*/, 2.84113486e-029f /*0x1010100A*/, 2.84113547e-029f /*0x1010100C*/, 2.84113607e-029f /*0x1010100E*/ };
	static const __vector4 v4ShuffleR1 = { 2.84113215e-029f /*0x10101001*/, 2.84113276e-029f /*0x10101003*/, 2.84113336e-029f /*0x10101005*/, 2.84113396e-029f /*0x10101007*/ }; 
	static const __vector4 v4ShuffleR2 = { 2.84113456e-029f /*0x10101009*/,	2.84113516e-029f /*0x1010100B*/, 2.84113577e-029f /*0x1010100D*/, 2.84113637e-029f /*0x1010100F*/ };

	__vector4 * AK_RESTRICT pv4In = (__vector4 * AK_RESTRICT) pIn;
	__vector4 * AK_RESTRICT pv4OutL = (__vector4 * AK_RESTRICT) pOutL;
	__vector4 * AK_RESTRICT pv4OutR = (__vector4 * AK_RESTRICT) pOutR;

	// Note: Cannot assume input data alignment is on 16 byte boundaries
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	// Note: Generated code is much slower not aligned so handle all 4 possible cases
	AkUInt32 uNumIter = uFramesToCopy / 8;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*8);
	const __vector4 * pv4InEnd = (__vector4 *)(pv4In + uNumIter);

	if ( !((AkUInt32)pv4In & 0x0000000F) && !((AkUInt32)pv4OutL & 0x0000000F) ) // Input and output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = *pv4In++;
			__vector4 v4ShufL1 = __vperm( v4InSamples, v4Zero, v4ShuffleL1 );
			__vector4 v4ShufR1 = __vperm( v4InSamples, v4Zero, v4ShuffleR1 );
			__vector4 v4ShufL2 = __vperm( v4InSamples, v4Zero, v4ShuffleL2 );
			__vector4 v4ShufR2 = __vperm( v4InSamples, v4Zero, v4ShuffleR2 );
			__vector4 v4ConvertedL1 = __vcuxwfp( v4ShufL1, 7 );
			__vector4 v4ConvertedR1 = __vcuxwfp( v4ShufR1, 7 );
			__vector4 v4ConvertedL2 = __vcuxwfp( v4ShufL2, 7 );
			__vector4 v4ConvertedR2 = __vcuxwfp( v4ShufR2, 7 );
			__vector4 v4OutL1 = __vsubfp( v4ConvertedL1, v4UnsignedOffset );
			__vector4 v4OutR1 = __vsubfp( v4ConvertedR1, v4UnsignedOffset );
			__vector4 v4OutL2 = __vsubfp( v4ConvertedL2, v4UnsignedOffset );
			__vector4 v4OutR2 = __vsubfp( v4ConvertedR2, v4UnsignedOffset );
			*pv4OutL++ = v4OutL1;
			*pv4OutR++ = v4OutR1;
			*pv4OutL++ = v4OutL2;
			*pv4OutR++ = v4OutR2;
		}
	}
	else if ( !((AkUInt32)pv4In & 0x0000000F)  ) // Only input aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = *pv4In++;
			__vector4 v4ShufL1 = __vperm( v4InSamples, v4Zero, v4ShuffleL1 );
			__vector4 v4ShufR1 = __vperm( v4InSamples, v4Zero, v4ShuffleR1 );
			__vector4 v4ShufL2 = __vperm( v4InSamples, v4Zero, v4ShuffleL2 );
			__vector4 v4ShufR2 = __vperm( v4InSamples, v4Zero, v4ShuffleR2 );
			__vector4 v4ConvertedL1 = __vcuxwfp( v4ShufL1, 7 );
			__vector4 v4ConvertedR1 = __vcuxwfp( v4ShufR1, 7 );
			__vector4 v4ConvertedL2 = __vcuxwfp( v4ShufL2, 7 );
			__vector4 v4ConvertedR2 = __vcuxwfp( v4ShufR2, 7 );
			__vector4 v4OutL1 = __vsubfp( v4ConvertedL1, v4UnsignedOffset );
			__vector4 v4OutR1 = __vsubfp( v4ConvertedR1, v4UnsignedOffset );
			__vector4 v4OutL2 = __vsubfp( v4ConvertedL2, v4UnsignedOffset );
			__vector4 v4OutR2 = __vsubfp( v4ConvertedR2, v4UnsignedOffset );
			__storeunalignedvector( v4OutL1, &pv4OutL[0] );
			__storeunalignedvector( v4OutR1, &pv4OutR[0] );
			__storeunalignedvector( v4OutL2, &pv4OutL[1] );
			__storeunalignedvector( v4OutR2, &pv4OutR[1] );
			pv4OutL += 2;
			pv4OutR += 2;
		}
	}
	else if ( !((AkUInt32)pv4OutL & 0x0000000F) ) // Only output aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = __loadunalignedvector( pv4In );
			++pv4In;
			__vector4 v4ShufL1 = __vperm( v4InSamples, v4Zero, v4ShuffleL1 );
			__vector4 v4ShufR1 = __vperm( v4InSamples, v4Zero, v4ShuffleR1 );
			__vector4 v4ShufL2 = __vperm( v4InSamples, v4Zero, v4ShuffleL2 );
			__vector4 v4ShufR2 = __vperm( v4InSamples, v4Zero, v4ShuffleR2 );
			__vector4 v4ConvertedL1 = __vcuxwfp( v4ShufL1, 7 );
			__vector4 v4ConvertedR1 = __vcuxwfp( v4ShufR1, 7 );
			__vector4 v4ConvertedL2 = __vcuxwfp( v4ShufL2, 7 );
			__vector4 v4ConvertedR2 = __vcuxwfp( v4ShufR2, 7 );
			__vector4 v4OutL1 = __vsubfp( v4ConvertedL1, v4UnsignedOffset );
			__vector4 v4OutR1 = __vsubfp( v4ConvertedR1, v4UnsignedOffset );
			__vector4 v4OutL2 = __vsubfp( v4ConvertedL2, v4UnsignedOffset );
			__vector4 v4OutR2 = __vsubfp( v4ConvertedR2, v4UnsignedOffset );
			*pv4OutL++ = v4OutL1;
			*pv4OutR++ = v4OutR1;
			*pv4OutL++ = v4OutL2;
			*pv4OutR++ = v4OutR2;
		}
	}
	else // Nothing aligned
	{
		// Process blocks of 16 frames
		while ( pv4In < pv4InEnd )
		{
#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, pv4In );
#endif
			__vector4 v4InSamples = __loadunalignedvector( pv4In );
			++pv4In;
			__vector4 v4ShufL1 = __vperm( v4InSamples, v4Zero, v4ShuffleL1 );
			__vector4 v4ShufR1 = __vperm( v4InSamples, v4Zero, v4ShuffleR1 );
			__vector4 v4ShufL2 = __vperm( v4InSamples, v4Zero, v4ShuffleL2 );
			__vector4 v4ShufR2 = __vperm( v4InSamples, v4Zero, v4ShuffleR2 );
			__vector4 v4ConvertedL1 = __vcuxwfp( v4ShufL1, 7 );
			__vector4 v4ConvertedR1 = __vcuxwfp( v4ShufR1, 7 );
			__vector4 v4ConvertedL2 = __vcuxwfp( v4ShufL2, 7 );
			__vector4 v4ConvertedR2 = __vcuxwfp( v4ShufR2, 7 );
			__vector4 v4OutL1 = __vsubfp( v4ConvertedL1, v4UnsignedOffset );
			__vector4 v4OutR1 = __vsubfp( v4ConvertedR1, v4UnsignedOffset );
			__vector4 v4OutL2 = __vsubfp( v4ConvertedL2, v4UnsignedOffset );
			__vector4 v4OutR2 = __vsubfp( v4ConvertedR2, v4UnsignedOffset );
			__storeunalignedvector( v4OutL1, &pv4OutL[0] );
			__storeunalignedvector( v4OutR1, &pv4OutR[0] );
			__storeunalignedvector( v4OutL2, &pv4OutL[1] );
			__storeunalignedvector( v4OutR2, &pv4OutR[1] );
			pv4OutL += 2;
			pv4OutR += 2;
		}
	}

	// Advance data pointers for remaining values
	pIn = (AkUInt8 * AK_RESTRICT) pv4In;
	pOutL = (AkReal32 * AK_RESTRICT) pv4OutL;
	pOutR = (AkReal32 * AK_RESTRICT) pv4OutR;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOutL++ = UINT8_TO_FLOAT( *pIn++ );
		*pOutR++ = UINT8_TO_FLOAT( *pIn++ );
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}


/********************* FIXED RESAMPLING DSP ROUTINES **********************/

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for one channel signals.
AKRESULT Fixed_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset - 1; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrame = *io_pPitchState->iLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		// lvlx assumes it can read 2 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 2 == 0 ); 
		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Two;
		v4Two.u[0] = 2;
		v4Two = __vspltw( v4Two, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Time 2 bytes per sample		
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Two );					// Current frame is 2 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghh( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghh( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghh( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghh( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghh( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghh( v4Cur13, v4Cur24 );
			// Sign extend and convert to normalized float
			v4Prev = __vupkhsh( v4Prev );
			v4Cur = __vupkhsh( v4Cur );
			v4Prev = __vcsxwfp( v4Prev, 15 );
			v4Cur = __vcsxwfp( v4Cur, 15 );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt16 iCurrentFrame = pInBuf[uPreviousFrameIndex+1];
		AkInt32 iSampleDiff = iCurrentFrame - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );
}

// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Fixed_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset - 2; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		FP_INDEX_ADVANCE();	
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 64 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Times 2 channels
			v4PreviousFramePos = __vadduws( v4PreviousFramePos, v4PreviousFramePos );				// Time 2 bytes per sample
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );					// Current frame is 4 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghh( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghh( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghh( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghh( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghh( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghh( v4Cur13, v4Cur24 );
			// Sign extend and convert to normalized float
			__vector4 v4PrevL = __vupkhsh( v4Prev );
			__vector4 v4PrevR = __vupklsh( v4Prev );
			__vector4 v4CurL = __vupkhsh( v4Cur );
			__vector4 v4CurR = __vupklsh( v4Cur );
			v4PrevL = __vcsxwfp( v4PrevL, 15 );
			v4PrevR = __vcsxwfp( v4PrevR, 15 );
			v4CurL = __vcsxwfp( v4CurL, 15 );
			v4CurR = __vcsxwfp( v4CurR, 15 );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*2;
		AkInt16 iPreviousFrameL = pInBuf[uPreviousFramePos];
		AkInt16 iPreviousFrameR = pInBuf[uPreviousFramePos+1];
		AkInt16 iCurrentFrameL = pInBuf[uPreviousFramePos+2];
		AkInt16 iCurrentFrameR = pInBuf[uPreviousFramePos+3];
		AkInt32 iSampleDiffL = iCurrentFrameL - iPreviousFrameL;
		AkInt32 iSampleDiffR = iCurrentFrameR - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		FP_INDEX_ADVANCE();		
	}

	PITCH_SAVE_NEXT_I16_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );
}


// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Fixed_I16_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );

	// Use stored value as left value, while right index is on the first sample
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	if ( uIterFrames )
	{
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->iLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->iLastValue, 0 );
		v4PreviousFrame = __vupkhsh( v4PreviousFrame );
		v4PreviousFrame = __vcsxwfp( v4PreviousFrame, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		v4CurrentFrame = __vupkhsh( v4CurrentFrame );
		v4CurrentFrame = __vcsxwfp( v4CurrentFrame, 15 );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += uNumChannels;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		} while( --uIterFrames );
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
	const AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Note: Vectorize over channels to avoid cast int-float conversion LHS
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, sign extend and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		v4PreviousFrame = __vupkhsh( v4PreviousFrame );
		v4PreviousFrame = __vcsxwfp( v4PreviousFrame, 15 );
		// Load current input, sign extend and convert to normalized float
		// TODO: This read could be replaced by a rotation of PreviousFrame input
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		v4CurrentFrame = __vupkhsh( v4CurrentFrame );
		v4CurrentFrame = __vcsxwfp( v4CurrentFrame, 15 );	
		// Linear interpolation
		__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
		__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
		// Store and advance
		__storeunalignedvector( v4Out, pfOutBuf );
		pfOutBuf += uNumChannels;
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
	}

	PITCH_SAVE_NEXT_I16_NCHAN( vu4IndexFP.u[0] );
	PITCH_FIXED_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Fixed_I16_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );

	// Use stored value as left value, while right index is on the first sample
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	if ( uIterFrames )
	{
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->iLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrameA = __lvlx( &io_pPitchState->iLastValue, 0 );
		__vector4 v4PreviousFrameB = __vupklsh( v4PreviousFrameA );
		v4PreviousFrameA = __vupkhsh( v4PreviousFrameA );
		v4PreviousFrameA = __vcsxwfp( v4PreviousFrameA, 15 );	
		v4PreviousFrameB = __vcsxwfp( v4PreviousFrameB, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrameA = __loadunalignedvector( &pInBuf[uNumChannels] );
		__vector4 v4CurrentFrameB = __vupklsh( v4CurrentFrameA );
		v4CurrentFrameA = __vupkhsh( v4CurrentFrameA );
		v4CurrentFrameA = __vcsxwfp( v4CurrentFrameA, 15 );
		v4CurrentFrameB = __vcsxwfp( v4CurrentFrameB, 15 );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
			__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
			__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
			__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
			// Store and advance
			__storeunalignedvector( v4OutA, pfOutBuf );
			__storeunalignedvector( v4OutB, pfOutBuf+4 );
			pfOutBuf += uNumChannels;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		} while( --uIterFrames );
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
	const AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 48 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Note: Vectorize over channels to avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, sign extend and convert to normalized float
		__vector4 v4PreviousFrameA = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		__vector4 v4PreviousFrameB = __vupklsh( v4PreviousFrameA );
		v4PreviousFrameA = __vupkhsh( v4PreviousFrameA );
		v4PreviousFrameA = __vcsxwfp( v4PreviousFrameA, 15 );
		v4PreviousFrameB = __vcsxwfp( v4PreviousFrameB, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrameA = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		__vector4 v4CurrentFrameB = __vupklsh( v4CurrentFrameA );
		v4CurrentFrameA = __vupkhsh( v4CurrentFrameA );
		v4CurrentFrameA = __vcsxwfp( v4CurrentFrameA, 15 );
		v4CurrentFrameB = __vcsxwfp( v4CurrentFrameB, 15 );	
		// Linear interpolation
		__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
		__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
		__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
		__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
		// Store and advance
		__storeunalignedvector( v4OutA, pfOutBuf );
		__storeunalignedvector( v4OutB, pfOutBuf+4 );
		pfOutBuf += uNumChannels;
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
	}

	PITCH_SAVE_NEXT_I16_NCHAN( vu4IndexFP.u[0] );
	PITCH_FIXED_DSP_TEARDOWN( vu4IndexFP.u[0] )
}

// Fixed resampling (no pitch changes) with unsigned 8-bit samples, optimized for one channel signals.
AKRESULT Fixed_U8_1Chan(		AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset - 1; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrame = *io_pPitchState->uLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_U8( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 16 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		static const __vector4 v4Zero = __vzero();
		static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };

		__vector4 v4Shuffle; 
		v4Shuffle.u[0] = 0x10101000;	v4Shuffle.u[1] = 0x10101001; 
		v4Shuffle.u[2] = 0x10101002;	v4Shuffle.u[3] = 0x10101003; 

		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4One;
		v4One.u[0] = 1;
		v4One = __vspltw( v4One, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFrameIndex, v4One );					// Current frame is 1 byte passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFrameIndex.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFrameIndex.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFrameIndex.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFrameIndex.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFrameIndex.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghb( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghb( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghb( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghb( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghb( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghb( v4Cur13, v4Cur24 );
			// Put to 32-bit container and convert to normalized float
			v4Prev = __vperm( v4Prev, v4Zero, v4Shuffle );
			v4Cur = __vperm( v4Cur, v4Zero, v4Shuffle );
			v4Prev = __vcuxwfp( v4Prev, 7 );
			v4Cur = __vcuxwfp( v4Cur, 7 );
			v4Prev = __vsubfp( v4Prev, v4UnsignedOffset );
			v4Cur = __vsubfp( v4Cur, v4UnsignedOffset );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkUInt8 iCurrentFrame = pInBuf[uPreviousFrameIndex+1];
		AkInt32 iSampleDiff = iCurrentFrame - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_U8( iPreviousFrame, iSampleDiff );
		FP_INDEX_ADVANCE();	
	}

	PITCH_SAVE_NEXT_U8_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );		
}

// Fixed resampling (no pitch changes) with INTERLEAVED unsigned 8-bit samples optimized for 2 channel signals.
AKRESULT Fixed_U8_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset - 2; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrameL = io_pPitchState->uLastValue[0];
	AkUInt8 iPreviousFrameR = io_pPitchState->uLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		static const __vector4 v4Zero = __vzero();
		static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };

		__vector4 v4ShuffleL; 
		v4ShuffleL.u[0] = 0x10101000;	v4ShuffleL.u[1] = 0x10101001; 
		v4ShuffleL.u[2] = 0x10101002;	v4ShuffleL.u[3] = 0x10101003; 

		__vector4 v4ShuffleR; 
		v4ShuffleR.u[0] = 0x10101004;	v4ShuffleR.u[1] = 0x10101005; 
		v4ShuffleR.u[2] = 0x10101006;	v4ShuffleR.u[3] = 0x10101007;

		// lvlx assumes it can read 2 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 2 == 0 ); 
		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Two;
		v4Two.u[0] = 2;
		v4Two = __vspltw( v4Two, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Times 2 channels
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Two );					// Current frame is 2 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghb( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghb( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghb( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghb( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghb( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghb( v4Cur13, v4Cur24 );
			
			// Put to 32-bit container and convert to normalized float
			__vector4 v4PrevL = __vperm( v4Prev, v4Zero, v4ShuffleL );
			__vector4 v4PrevR = __vperm( v4Prev, v4Zero, v4ShuffleR );
			__vector4 v4CurL = __vperm( v4Cur, v4Zero, v4ShuffleL );
			__vector4 v4CurR = __vperm( v4Cur, v4Zero, v4ShuffleR );
			v4PrevL = __vcuxwfp( v4PrevL, 7 );
			v4PrevR = __vcuxwfp( v4PrevR, 7 );
			v4CurL = __vcuxwfp( v4CurL, 7 );
			v4CurR = __vcuxwfp( v4CurR, 7 );
			v4PrevL = __vsubfp( v4PrevL, v4UnsignedOffset );
			v4PrevR = __vsubfp( v4PrevR, v4UnsignedOffset );
			v4CurL = __vsubfp( v4CurL, v4UnsignedOffset );
			v4CurR = __vsubfp( v4CurR, v4UnsignedOffset );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*2;
		AkUInt8 iPreviousFrameL = pInBuf[uPreviousFramePos];
		AkUInt8 iPreviousFrameR = pInBuf[uPreviousFramePos+1];
		AkUInt8 iCurrentFrameL = pInBuf[uPreviousFramePos+2];
		AkUInt8 iCurrentFrameR = pInBuf[uPreviousFramePos+3];
		AkInt32 iSampleDiffL = iCurrentFrameL - iPreviousFrameL;
		AkInt32 iSampleDiffR = iCurrentFrameR - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		FP_INDEX_ADVANCE();	
	}

	PITCH_SAVE_NEXT_U8_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with unsigned 8-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Fixed_U8_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };
	static const __vector4 v4Shuffle = { 2.84113185e-029f /*0x10101000*/, 2.84113215e-029f /*0x10101001*/, 2.84113246e-029f /*0x10101002*/, 2.84113276e-029f /*0x10101003*/  };

	// Use stored value as left value, while right index is on the first sample
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	if ( uIterFrames )
	{
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->uLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->uLastValue, 0 );
		v4PreviousFrame = __vperm( v4PreviousFrame, v4Zero, v4Shuffle );
		v4PreviousFrame = __vcuxwfp( v4PreviousFrame, 7 );
		v4PreviousFrame = __vsubfp( v4PreviousFrame, v4UnsignedOffset );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		v4CurrentFrame = __vperm( v4CurrentFrame, v4Zero, v4Shuffle );
		v4CurrentFrame = __vcuxwfp( v4CurrentFrame, 7 );
		v4CurrentFrame = __vsubfp( v4CurrentFrame, v4UnsignedOffset );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += uNumChannels;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		} while( --uIterFrames );
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
	const AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 16 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Note: Vectorize over channels to avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, put to 32-bit container and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		v4PreviousFrame = __vperm( v4PreviousFrame, v4Zero, v4Shuffle );
		v4PreviousFrame = __vcuxwfp( v4PreviousFrame, 7 );
		v4PreviousFrame = __vsubfp( v4PreviousFrame, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		// TODO: This read could be replaced by a rotation of PreviousFrame input
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		v4CurrentFrame = __vperm( v4CurrentFrame, v4Zero, v4Shuffle );
		v4CurrentFrame = __vcuxwfp( v4CurrentFrame, 7 );
		v4CurrentFrame = __vsubfp( v4CurrentFrame, v4UnsignedOffset );
		// Linear interpolation
		__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
		__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
		// Store and advance
		__storeunalignedvector( v4Out, pfOutBuf );
		pfOutBuf += uNumChannels;
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
	}

	PITCH_SAVE_NEXT_U8_NCHAN( vu4IndexFP.u[0] );
	PITCH_FIXED_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Fixed_U8_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );
	
	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };
	static const __vector4 v4ShuffleA = { 2.84113185e-029f /*0x10101000*/, 2.84113215e-029f /*0x10101001*/, 2.84113246e-029f /*0x10101002*/, 2.84113276e-029f /*0x10101003*/ } ;
	static const __vector4 v4ShuffleB = { 2.84113306e-029f /*0x10101004*/, 2.84113336e-029f /*0x10101005*/, 2.84113366e-029f /*0x10101006*/, 2.84113396e-029f /*0x10101007*/ };

	// Use stored value as left value, while right index is on the first sample
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	if ( uIterFrames )
	{
		// Load past input, put to 32-bit container and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->uLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->uLastValue, 0 );
		__vector4 v4PreviousFrameA = __vperm( v4PreviousFrame, v4Zero, v4ShuffleA );
		v4PreviousFrameA = __vcuxwfp( v4PreviousFrameA, 7 );
		v4PreviousFrameA = __vsubfp( v4PreviousFrameA, v4UnsignedOffset );
		__vector4 v4PreviousFrameB = __vperm( v4PreviousFrame, v4Zero, v4ShuffleB );
		v4PreviousFrameB = __vcuxwfp( v4PreviousFrameB, 7 );
		v4PreviousFrameB = __vsubfp( v4PreviousFrameB, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		__vector4 v4CurrentFrameA = __vperm( v4CurrentFrame, v4Zero, v4ShuffleA );
		v4CurrentFrameA = __vcuxwfp( v4CurrentFrameA, 7 );
		v4CurrentFrameA = __vsubfp( v4CurrentFrameA, v4UnsignedOffset );
		__vector4 v4CurrentFrameB = __vperm( v4CurrentFrame, v4Zero, v4ShuffleB );
		v4CurrentFrameB = __vcuxwfp( v4CurrentFrameB, 7 );
		v4CurrentFrameB = __vsubfp( v4CurrentFrameB, v4UnsignedOffset );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
			__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
			__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
			__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
			// Store and advance
			__storeunalignedvector( v4OutA, pfOutBuf );
			__storeunalignedvector( v4OutB, pfOutBuf+4 );
			pfOutBuf += uNumChannels;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		} while( --uIterFrames );
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
	const AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Note: Vectorize over channels to avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, put to 32-bit container and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		__vector4 v4PreviousFrameA = __vperm( v4PreviousFrame, v4Zero, v4ShuffleA );
		v4PreviousFrameA = __vcuxwfp( v4PreviousFrameA, 7 );
		v4PreviousFrameA = __vsubfp( v4PreviousFrameA, v4UnsignedOffset );
		__vector4 v4PreviousFrameB = __vperm( v4PreviousFrame, v4Zero, v4ShuffleB );
		v4PreviousFrameB = __vcuxwfp( v4PreviousFrameB, 7 );
		v4PreviousFrameB = __vsubfp( v4PreviousFrameB, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		__vector4 v4CurrentFrameA = __vperm( v4CurrentFrame, v4Zero, v4ShuffleA );
		v4CurrentFrameA = __vcuxwfp( v4CurrentFrameA, 7 );
		v4CurrentFrameA = __vsubfp( v4CurrentFrameA, v4UnsignedOffset );
		__vector4 v4CurrentFrameB = __vperm( v4CurrentFrame, v4Zero, v4ShuffleB );
		v4CurrentFrameB = __vcuxwfp( v4CurrentFrameB, 7 );
		v4CurrentFrameB = __vsubfp( v4CurrentFrameB, v4UnsignedOffset );
		// Linear interpolation
		__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
		__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
		__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
		__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
		// Store and advance
		__storeunalignedvector( v4OutA, pfOutBuf );
		__storeunalignedvector( v4OutB, pfOutBuf+4 );
		pfOutBuf += uNumChannels;
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
	}

	PITCH_SAVE_NEXT_U8_NCHAN( vu4IndexFP.u[0] );
	PITCH_FIXED_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Fixed resampling (no pitch changes) with floating point samples, optimized for one channel signals.
AKRESULT Fixed_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 fLeftSample = *io_pPitchState->fLastValue;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkReal32 fSampleDiff = pInBuf[1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 64 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		__vector4 v4Mult4Shift;
		v4Mult4Shift.u[0] = 2;
		v4Mult4Shift = __vspltw( v4Mult4Shift, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vslw( v4PreviousFrameIndex, v4Mult4Shift ); // Time 4 bytes per sample		
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );		 // Current frame is 4 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghw( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghw( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghw( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghw( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghw( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghw( v4Cur13, v4Cur24 );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		fLeftSample = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_NATIVE_1CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples, optimized for 2 channel signals.
AKRESULT Fixed_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// IMPORTANT: Second channel access relies on the fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkReal32 fPreviousFrameL = io_pPitchState->fLastValue[0];
	AkReal32 fPreviousFrameR = io_pPitchState->fLastValue[1];
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkReal32 fSampleDiffL = pInBuf[1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[1+uMaxFrames] - fPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 128 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	uIterFrames = uNumIterThisFrame;
	if ( uIterFrames >= 4 )
	{
		AkReal32 * AK_RESTRICT pInBufL = (AkReal32 * AK_RESTRICT) pInBuf;
		AkReal32 * AK_RESTRICT pInBufR = (AkReal32 * AK_RESTRICT) (pInBuf + uMaxFrames);
		AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT) pfOutBuf;
		AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT) (pfOutBuf + uMaxFrames);

		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumBlocks = uIterFrames / 4;
		uIterFrames -= uNumBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP;
		v4IndexFP.u[1] = uIndexFP + uFrameSkipFP;
		v4IndexFP.u[2] = uIndexFP + 2*uFrameSkipFP;
		v4IndexFP.u[3] = uIndexFP+ 3*uFrameSkipFP;
		__vector4 v4FrameSkip;
		v4FrameSkip.u[0] = 4 * uFrameSkipFP;
		v4FrameSkip = __vspltw( v4FrameSkip, 0 );
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		__vector4 v4Mult4Shift;
		v4Mult4Shift.u[0] = 2;
		v4Mult4Shift = __vspltw( v4Mult4Shift, 0 );
		while ( uNumBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vslw( v4PreviousFrameIndex, v4Mult4Shift );	// Time 4 bytes per sample	
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );			// Current frame 4 bytes passed previous	

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			// Note: Possibly saturating load unit here, should compare if looping for each channel is better
			// It will put less stress on loading but will need to recompute sample index twice as much...
			__vector4 v4PrevL1 = __lvlx( pInBufL, v4PreviousFramePos.u[0] );
			__vector4 v4CurL1 = __lvlx( pInBufL, v4CurrentFramePos.u[0] );
			__vector4 v4PrevR1 = __lvlx( pInBufR, v4PreviousFramePos.u[0] );
			__vector4 v4CurR1 = __lvlx( pInBufR, v4CurrentFramePos.u[0] );
			__vector4 v4PrevL2 = __lvlx( pInBufL, v4PreviousFramePos.u[1] );
			__vector4 v4CurL2 = __lvlx( pInBufL, v4CurrentFramePos.u[1] );
			__vector4 v4PrevR2 = __lvlx( pInBufR, v4PreviousFramePos.u[1] );
			__vector4 v4CurR2 = __lvlx( pInBufR, v4CurrentFramePos.u[1] );
			__vector4 v4PrevL3 = __lvlx( pInBufL, v4PreviousFramePos.u[2] );
			__vector4 v4CurL3 = __lvlx( pInBufL, v4CurrentFramePos.u[2] );
			__vector4 v4PrevR3 = __lvlx( pInBufR, v4PreviousFramePos.u[2] );
			__vector4 v4CurR3 = __lvlx( pInBufR, v4CurrentFramePos.u[2] );
			__vector4 v4PrevL4 = __lvlx( pInBufL, v4PreviousFramePos.u[3] );
			__vector4 v4CurL4 = __lvlx( pInBufL, v4CurrentFramePos.u[3] );
			__vector4 v4PrevR4 = __lvlx( pInBufR, v4PreviousFramePos.u[3] );
			__vector4 v4CurR4 = __lvlx( pInBufR, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4PrevL13 = __vmrghw( v4PrevL1, v4PrevL3 );
			__vector4 v4PrevR13 = __vmrghw( v4PrevR1, v4PrevR3 );
			__vector4 v4PrevL24 = __vmrghw( v4PrevL2, v4PrevL4 );
			__vector4 v4PrevR24 = __vmrghw( v4PrevR2, v4PrevR4 );
			__vector4 v4CurL13 = __vmrghw( v4CurL1, v4CurL3 );
			__vector4 v4CurR13 = __vmrghw( v4CurR1, v4CurR3 );
			__vector4 v4CurL24 = __vmrghw( v4CurL2, v4CurL4 );
			__vector4 v4CurR24 = __vmrghw( v4CurR2, v4CurR4 );
			__vector4 v4PrevL = __vmrghw( v4PrevL13, v4PrevL24 );
			__vector4 v4PrevR = __vmrghw( v4PrevR13, v4PrevR24 );
			__vector4 v4CurL = __vmrghw( v4CurL13, v4CurL24 );
			__vector4 v4CurR = __vmrghw( v4CurR13, v4CurR24 );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			v4IndexFP = __vadduws( v4IndexFP, v4FrameSkip );

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
		pInBuf = pInBufL;
		pfOutBuf = pfOutBufL;
	}

	// For all other sample frames
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[uPreviousFrameR+1] - fPreviousFrameR;	
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_NATIVE_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with DEINTERLEAVED floating point samples optimized for any number of channels.
AKRESULT Fixed_Native_NChan_Vec(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	// PITCH_FIXED_DSP_SETUP_VEC( ) but delaying some calculation for each channel
	AKASSERT( io_pOutBuffer->MaxFrames() >= io_pPitchState->uOutFrameOffset );
	const AkUInt32 uInBufferFrames = io_pInBuffer->uValidFrames;	
	const AkUInt32 uOutBufferFrames = uRequestedSize - io_pPitchState->uOutFrameOffset;
	const AkUInt32 uFrameSkipFP = io_pPitchState->uCurrentFrameSkip;
	const AkUInt32 uNumIterPreviousFrame = AkMin( (SINGLEFRAMEDISTANCE - io_pPitchState->uFloatIndex + (uFrameSkipFP-1)) / uFrameSkipFP, uOutBufferFrames);
	__vector4 vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
	vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
	static const __vector4 vu4AndFPMASK = { 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/ };
	
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt32 uNumIterThisFrame;	
	__vector4 vu4IndexFP;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		vu4IndexFP = __lvlx( &io_pPitchState->uFloatIndex, 0 );
		vu4IndexFP = __vspltw( vu4IndexFP, 0 );
		__vector4 vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		__vector4 vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	

		// Minus one to compensate for offset of 1 due to zero == previous
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ) + io_pPitchState->uInFrameOffset - 1; 
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i ) + io_pPitchState->uOutFrameOffset;

#ifdef USEPITCHPREFETCH
	for( int j = 0; j < AKSIMD_ARCHMAXPREFETCHSIZE; j += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( j, pInBuf );
#endif
		// Use stored value as left value, while right index is on the first sample
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->fLastValue[i], 0 );
		__vector4 v4CurrentFrame = __lvlx( &pInBuf[1], 0 );
		AkUInt32 uIterFrames = uNumIterPreviousFrame;
		while ( uIterFrames-- )
		{	
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__stvlx( v4Out, pfOutBuf, 0 );
			++pfOutBuf;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		}

		// Determine number of iterations remaining
		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - vu4IndexFP.u[0] + (uFrameSkipFP-1)) / uFrameSkipFP;
		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

		// For all other sample frames
		uIterFrames = uNumIterThisFrame;
		while ( uIterFrames-- )
		{
			AkUInt32 uPreviousFrameIndex = (vu4IndexFP.u[0] >> FPBITS);
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFrameIndex] );
#endif
			v4PreviousFrame = __lvlx( &pInBuf[uPreviousFrameIndex], 0 );
			v4CurrentFrame = __lvlx( &pInBuf[uPreviousFrameIndex+1], 0 );
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__stvlx( v4Out, pfOutBuf, 0 );
			++pfOutBuf;
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp( vu4InterpLocFP, FPBITS );	
		}
	}

	PITCH_SAVE_NEXT_NATIVE_NCHAN( vu4IndexFP.u[0] );
	PITCH_FIXED_DSP_TEARDOWN( vu4IndexFP.u[0] );	
}

/********************* INTERPOLATING RESAMPLING DSP ROUTINES **********************/

// Fixed resampling (pitch changes) with signed 16-bit samples optimized for one channel signals.
AKRESULT Interpolating_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset - 1; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrame = *io_pPitchState->iLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor	
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;		
	}
	
	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		AKASSERT( (AkUInt32)pInBuf % 2 == 0 ); 
		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Two;
		v4Two.u[0] = 2;
		v4Two = __vspltw( v4Two, 0 );
		// For all blocks that require interpolation
		while ( uNumIterBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Time 2 bytes per sample		
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Two );					// Current frame is 2 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghh( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghh( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghh( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghh( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghh( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghh( v4Cur13, v4Cur24 );
			// Sign extend and convert to normalized float
			v4Prev = __vupkhsh( v4Prev );
			v4Cur = __vupkhsh( v4Cur );
			v4Prev = __vcsxwfp( v4Prev, 15 );
			v4Cur = __vcsxwfp( v4Cur, 15 );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			uRampCount += 4;
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}

		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt16 iCurrentFrame = pInBuf[uPreviousFrameIndex+1];
		AkInt32 iSampleDiff = iCurrentFrame - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();	
		FP_INDEX_ADVANCE();
	}

	AKASSERT( uSimPreviousFrameIndex == uPreviousFrameIndex ); // Simulation was accurate

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals.
AKRESULT Interpolating_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset - 2; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBufL;
	const AkReal32 * pfOutBufEnd = pfOutBufL + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBufL);	// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBufL);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;			
	}

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 64 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		while ( uNumIterBlocks-- )
		{
			AKASSERT( v4PreviousFrameIndex.u[0] <= uLastValidPreviousIndex );
			AKASSERT( v4PreviousFrameIndex.u[1] <= uLastValidPreviousIndex );
			AKASSERT( v4PreviousFrameIndex.u[2] <= uLastValidPreviousIndex );
			AKASSERT( v4PreviousFrameIndex.u[3] <= uLastValidPreviousIndex );
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Times 2 channels
			v4PreviousFramePos = __vadduws( v4PreviousFramePos, v4PreviousFramePos );				// Time 2 bytes per sample
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );					// Current frame is 4 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghh( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghh( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghh( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghh( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghh( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghh( v4Cur13, v4Cur24 );
			// Sign extend and convert to normalized float
			__vector4 v4PrevL = __vupkhsh( v4Prev );
			__vector4 v4PrevR = __vupklsh( v4Prev );
			__vector4 v4CurL = __vupkhsh( v4Cur );
			__vector4 v4CurR = __vupklsh( v4Cur );
			v4PrevL = __vcsxwfp( v4PrevL, 15 );
			v4PrevR = __vcsxwfp( v4PrevR, 15 );
			v4CurL = __vcsxwfp( v4CurL, 15 );
			v4CurR = __vcsxwfp( v4CurR, 15 );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			uRampCount += 4;
			AKASSERT( uRampCount <= PITCHRAMPLENGTH );
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*2;
		AkInt16 iPreviousFrameL = pInBuf[uPreviousFramePos];
		AkInt16 iPreviousFrameR = pInBuf[uPreviousFramePos+1];
		AkInt16 iCurrentFrameL = pInBuf[uPreviousFramePos+2];
		AkInt16 iCurrentFrameR = pInBuf[uPreviousFramePos+3];
		AkInt32 iSampleDiffL = iCurrentFrameL - iPreviousFrameL;
		AkInt32 iSampleDiffR = iCurrentFrameR - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	AKASSERT( uSimPreviousFrameIndex == uPreviousFrameIndex ); // Simulation was accurate

	AkReal32 * pfOutBuf = pfOutBufL;
	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with signed 16-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Interpolating_I16_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	
#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames*uNumChannels;

	PITCH_INTERPOLATION_SETUP( );
	
	AkUInt32 uMaxNumIter = AkMin( uOutBufferFrames, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	AkUInt32 uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	if ( uPreviousFrameIndex  == 0 && uMaxNumIter )
	{
		// Use stored value as left value, while right index is on the first sample
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->iLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->iLastValue, 0 );
		v4PreviousFrame = __vupkhsh( v4PreviousFrame );
		v4PreviousFrame = __vcsxwfp( v4PreviousFrame, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		v4CurrentFrame = __vupkhsh( v4CurrentFrame );
		v4CurrentFrame = __vcsxwfp( v4CurrentFrame, 15 );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += uNumChannels;
			// Resampling factor interpolate
			++uRampCount;	
			uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
			vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
			vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
			uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
		} while ( uPreviousFrameIndex  == 0 && --uMaxNumIter );
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = AkMin( (AkUInt32) (pfOutBufEnd - pfOutBuf)/uNumChannels, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, sign extend and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		v4PreviousFrame = __vupkhsh( v4PreviousFrame );
		v4PreviousFrame = __vcsxwfp( v4PreviousFrame, 15 );
		// Load current input, sign extend and convert to normalized float
		// TODO: This read could be replaced by a rotation of PreviousFrame input
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		v4CurrentFrame = __vupkhsh( v4CurrentFrame );
		v4CurrentFrame = __vcsxwfp( v4CurrentFrame, 15 );	
		// Linear interpolation
		__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
		__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
		// Store and advance
		__storeunalignedvector( v4Out, pfOutBuf );
		pfOutBuf += uNumChannels;
		// Resampling factor interpolate
		++uRampCount;	
		uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
		vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_NCHAN( vu4IndexFP.u[0] );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart)/uNumChannels;
	PITCH_INTERPOLATING_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Interpolating resampling (pitch changes) with signed 16-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Interpolating_I16_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	
#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames*uNumChannels;

	PITCH_INTERPOLATION_SETUP( );
	
	AkUInt32 uMaxNumIter = AkMin( uOutBufferFrames, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	AkUInt32 uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	if ( uPreviousFrameIndex  == 0 && uMaxNumIter )
	{
		// Use stored value as left value, while right index is on the first sample
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->iLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrameA = __lvlx( &io_pPitchState->iLastValue, 0 );
		__vector4 v4PreviousFrameB = __vupklsh( v4PreviousFrameA );
		v4PreviousFrameA = __vupkhsh( v4PreviousFrameA );
		v4PreviousFrameA = __vcsxwfp( v4PreviousFrameA, 15 );	
		v4PreviousFrameB = __vcsxwfp( v4PreviousFrameB, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrameA = __loadunalignedvector( &pInBuf[uNumChannels] );
		__vector4 v4CurrentFrameB = __vupklsh( v4CurrentFrameA );
		v4CurrentFrameA = __vupkhsh( v4CurrentFrameA );
		v4CurrentFrameA = __vcsxwfp( v4CurrentFrameA, 15 );
		v4CurrentFrameB = __vcsxwfp( v4CurrentFrameB, 15 );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
			__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
			__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
			__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
			// Store and advance
			__storeunalignedvector( v4OutA, pfOutBuf );
			__storeunalignedvector( v4OutB, pfOutBuf+4 );
			pfOutBuf += uNumChannels;
			// Resampling factor interpolate
			++uRampCount;	
			uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
			vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
			vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
			uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;	
		} while ( uPreviousFrameIndex  == 0 && --uMaxNumIter );
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = AkMin( (AkUInt32) (pfOutBufEnd - pfOutBuf)/uNumChannels, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, sign extend and convert to normalized float
		__vector4 v4PreviousFrameA = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		__vector4 v4PreviousFrameB = __vupklsh( v4PreviousFrameA );
		v4PreviousFrameA = __vupkhsh( v4PreviousFrameA );
		v4PreviousFrameA = __vcsxwfp( v4PreviousFrameA, 15 );
		v4PreviousFrameB = __vcsxwfp( v4PreviousFrameB, 15 );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrameA = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		__vector4 v4CurrentFrameB = __vupklsh( v4CurrentFrameA );
		v4CurrentFrameA = __vupkhsh( v4CurrentFrameA );
		v4CurrentFrameA = __vcsxwfp( v4CurrentFrameA, 15 );
		v4CurrentFrameB = __vcsxwfp( v4CurrentFrameB, 15 );	
		// Linear interpolation
		__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
		__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
		__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
		__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
		// Store and advance
		__storeunalignedvector( v4OutA, pfOutBuf );
		__storeunalignedvector( v4OutB, pfOutBuf+4 );
		pfOutBuf += uNumChannels;
		// Resampling factor interpolate
		++uRampCount;	
		uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
		vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_NCHAN( vu4IndexFP.u[0] );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart)/uNumChannels;
	PITCH_INTERPOLATING_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Interpolating resampling (pitch changes) with unsigned 8-bit samples optimized for one channel signals.
AKRESULT Interpolating_U8_1Chan(	AkAudioBuffer * io_pInBuffer, 
								AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
								AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset - 1; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrame = *io_pPitchState->uLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiff = pInBuf[1] - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_U8( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);				// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor	
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;	
	}

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 16 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		static const __vector4 v4Zero = __vzero();
		static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };

		__vector4 v4Shuffle; 
		v4Shuffle.u[0] = 0x10101000;	v4Shuffle.u[1] = 0x10101001; 
		v4Shuffle.u[2] = 0x10101002;	v4Shuffle.u[3] = 0x10101003; 

		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4One;
		v4One.u[0] = 1;
		v4One = __vspltw( v4One, 0 );
		while ( uNumIterBlocks-- )
		{
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFrameIndex, v4One );					// Current frame is 1 byte passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFrameIndex.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFrameIndex.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFrameIndex.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFrameIndex.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFrameIndex.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghb( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghb( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghb( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghb( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghb( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghb( v4Cur13, v4Cur24 );
			// Put to 32-bit container and convert to normalized float
			v4Prev = __vperm( v4Prev, v4Zero, v4Shuffle );
			v4Cur = __vperm( v4Cur, v4Zero, v4Shuffle );
			v4Prev = __vcuxwfp( v4Prev, 7 );
			v4Cur = __vcuxwfp( v4Cur, 7 );
			v4Prev = __vsubfp( v4Prev, v4UnsignedOffset );
			v4Cur = __vsubfp( v4Cur, v4UnsignedOffset );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			uRampCount += 4;
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		// Linear interpolation
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkUInt8 iCurrentFrame = pInBuf[uPreviousFrameIndex+1];
		AkInt32 iSampleDiff = iCurrentFrame - iPreviousFrame;
		*pfOutBuf++ = LINEAR_INTERP_U8( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_U8_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with INTERLEAVED unsigned 8-bit samples optimized for 2 channel signals.
AKRESULT Interpolating_U8_2Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + 2*io_pPitchState->uInFrameOffset - 2; 

#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	// Retrieve output buffer information
	AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;
	AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 1 ) ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBufL;
	const AkReal32 * pfOutBufEnd = pfOutBufL + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrameL = io_pPitchState->uLastValue[0];
	AkUInt8 iPreviousFrameR = io_pPitchState->uLastValue[1];
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBufL);	// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBufL);				// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor	
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;	
	}

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		static const __vector4 v4Zero = __vzero();
		static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };

		__vector4 v4ShuffleL; 
		v4ShuffleL.u[0] = 0x10101000;	v4ShuffleL.u[1] = 0x10101001; 
		v4ShuffleL.u[2] = 0x10101002;	v4ShuffleL.u[3] = 0x10101003; 

		__vector4 v4ShuffleR; 
		v4ShuffleR.u[0] = 0x10101004;	v4ShuffleR.u[1] = 0x10101005; 
		v4ShuffleR.u[2] = 0x10101006;	v4ShuffleR.u[3] = 0x10101007;

		// lvlx assumes it can read 2 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 2 == 0 ); 
		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Two;
		v4Two.u[0] = 2;
		v4Two = __vspltw( v4Two, 0 );
		while ( uNumIterBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vadduws( v4PreviousFrameIndex, v4PreviousFrameIndex ); // Times 2 channels
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Two );					// Current frame is 2 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghb( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghb( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghb( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghb( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghb( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghb( v4Cur13, v4Cur24 );
			
			// Put to 32-bit container and convert to normalized float
			__vector4 v4PrevL = __vperm( v4Prev, v4Zero, v4ShuffleL );
			__vector4 v4PrevR = __vperm( v4Prev, v4Zero, v4ShuffleR );
			__vector4 v4CurL = __vperm( v4Cur, v4Zero, v4ShuffleL );
			__vector4 v4CurR = __vperm( v4Cur, v4Zero, v4ShuffleR );
			v4PrevL = __vcuxwfp( v4PrevL, 7 );
			v4PrevR = __vcuxwfp( v4PrevR, 7 );
			v4CurL = __vcuxwfp( v4CurL, 7 );
			v4CurR = __vcuxwfp( v4CurR, 7 );
			v4PrevL = __vsubfp( v4PrevL, v4UnsignedOffset );
			v4PrevR = __vsubfp( v4PrevR, v4UnsignedOffset );
			v4CurL = __vsubfp( v4CurL, v4UnsignedOffset );
			v4CurR = __vsubfp( v4CurR, v4UnsignedOffset );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			uRampCount += 4;
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*2;
		AkUInt8 iPreviousFrameL = pInBuf[uPreviousFramePos];
		AkUInt8 iPreviousFrameR = pInBuf[uPreviousFramePos+1];
		AkUInt8 iCurrentFrameL = pInBuf[uPreviousFramePos+2];
		AkUInt8 iCurrentFrameR = pInBuf[uPreviousFramePos+3];
		AkInt32 iSampleDiffL = iCurrentFrameL - iPreviousFrameL;
		AkInt32 iSampleDiffR = iCurrentFrameR - iPreviousFrameR;
		*pfOutBufL++ = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		*pfOutBufR++ = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	AKASSERT( uSimPreviousFrameIndex == uPreviousFrameIndex ); // Simulation was accurate

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_U8_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBufL - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );	
}

// Interpolating resampling (pitch changes) with unsigned 8-bit samples optimized for 1 to 4 channel signals (vectorized across channels)
AKRESULT Interpolating_U8_1To4ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	
#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames*uNumChannels;

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };
	static const __vector4 v4Shuffle = { 2.84113185e-029f /*0x10101000*/, 2.84113215e-029f /*0x10101001*/, 2.84113246e-029f /*0x10101002*/, 2.84113276e-029f /*0x10101003*/ };

	PITCH_INTERPOLATION_SETUP( );
	
	AkUInt32 uMaxNumIter = AkMin( uOutBufferFrames, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	AkUInt32 uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	if ( uPreviousFrameIndex  == 0 && uMaxNumIter )
	{
		// Use stored value as left value, while right index is on the first sample
		// Load past input, sign extend and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->uLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->uLastValue, 0 );
		v4PreviousFrame = __vperm( v4PreviousFrame, v4Zero, v4Shuffle );
		v4PreviousFrame = __vcuxwfp( v4PreviousFrame, 7 );
		v4PreviousFrame = __vsubfp( v4PreviousFrame, v4UnsignedOffset );
		// Load current input, sign extend and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		v4CurrentFrame = __vperm( v4CurrentFrame, v4Zero, v4Shuffle );
		v4CurrentFrame = __vcuxwfp( v4CurrentFrame, 7 );
		v4CurrentFrame = __vsubfp( v4CurrentFrame, v4UnsignedOffset );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += uNumChannels;
			// Resampling factor interpolate
			++uRampCount;	
			uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
			vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
			vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
			uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
		} while ( uPreviousFrameIndex  == 0 && --uMaxNumIter );
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = AkMin( (AkUInt32) (pfOutBufEnd - pfOutBuf)/uNumChannels, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		AkUInt32 uPreviousFramePos = uPreviousFrameIndex*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, put to 32-bit container and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		v4PreviousFrame = __vperm( v4PreviousFrame, v4Zero, v4Shuffle );
		v4PreviousFrame = __vcuxwfp( v4PreviousFrame, 7 );
		v4PreviousFrame = __vsubfp( v4PreviousFrame, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		// TODO: This read could be replaced by a rotation of PreviousFrame input
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		v4CurrentFrame = __vperm( v4CurrentFrame, v4Zero, v4Shuffle );
		v4CurrentFrame = __vcuxwfp( v4CurrentFrame, 7 );
		v4CurrentFrame = __vsubfp( v4CurrentFrame, v4UnsignedOffset );
		// Linear interpolation
		__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
		__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
		// Store and advance
		__storeunalignedvector( v4Out, pfOutBuf );
		pfOutBuf += uNumChannels;
		// Resampling factor interpolate
		++uRampCount;	
		uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
		vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_U8_NCHAN( vu4IndexFP.u[0] );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart)/uNumChannels;
	PITCH_INTERPOLATING_DSP_TEARDOWN( vu4IndexFP.u[0] );
}

// Interpolating resampling (pitch changes) with unsigned 8-bit samples optimized for 5 to 8 channel signals (vectorized across channels)
AKRESULT Interpolating_U8_5To8ChanVec(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	// Minus one frame to compensate for offset of 1 due to zero == previous
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels - uNumChannels; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;
	
#ifdef USEPITCHPREFETCH
	for( int i = 0; i < AKSIMD_ARCHMAXPREFETCHSIZE; i += AKSIMD_ARCHCACHELINESIZE )
		AKSIMD_PREFETCHMEMORY( i, pInBuf );
#endif

	PITCH_FIXED_DSP_SETUP_VEC( );
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames*uNumChannels;

	static const __vector4 v4Zero = __vzero();
	static const __vector4 v4UnsignedOffset = { 1.f, 1.f, 1.f, 1.f };
	static const __vector4 v4ShuffleA = { 2.84113185e-029f /*0x10101000*/, 2.84113215e-029f /*0x10101001*/, 2.84113246e-029f /*0x10101002*/, 2.84113276e-029f /*0x10101003*/  };
	static const __vector4 v4ShuffleB = { 2.84113306e-029f /*0x10101004*/, 2.84113336e-029f /*0x10101005*/, 2.84113366e-029f /*0x10101006*/, 2.84113396e-029f /*0x10101007*/  };

	PITCH_INTERPOLATION_SETUP( );
	
	AkUInt32 uMaxNumIter = AkMin( uOutBufferFrames, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	AkUInt32 uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	if ( uPreviousFrameIndex  == 0 && uMaxNumIter )
	{
		// Use stored value as left value, while right index is on the first sample
		// Load past input, put to 32-bit container and convert to normalized float
		AKASSERT( (((AkUInt32)io_pPitchState->uLastValue) & 0xF) == 0 ); 
		__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->uLastValue, 0 );
		__vector4 v4PreviousFrameA = __vperm( v4PreviousFrame, v4Zero, v4ShuffleA );
		v4PreviousFrameA = __vcuxwfp( v4PreviousFrameA, 7 );
		v4PreviousFrameA = __vsubfp( v4PreviousFrameA, v4UnsignedOffset );
		__vector4 v4PreviousFrameB = __vperm( v4PreviousFrame, v4Zero, v4ShuffleB );
		v4PreviousFrameB = __vcuxwfp( v4PreviousFrameB, 7 );
		v4PreviousFrameB = __vsubfp( v4PreviousFrameB, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uNumChannels] );
		__vector4 v4CurrentFrameA = __vperm( v4CurrentFrame, v4Zero, v4ShuffleA );
		v4CurrentFrameA = __vcuxwfp( v4CurrentFrameA, 7 );
		v4CurrentFrameA = __vsubfp( v4CurrentFrameA, v4UnsignedOffset );
		__vector4 v4CurrentFrameB = __vperm( v4CurrentFrame, v4Zero, v4ShuffleB );
		v4CurrentFrameB = __vcuxwfp( v4CurrentFrameB, 7 );
		v4CurrentFrameB = __vsubfp( v4CurrentFrameB, v4UnsignedOffset );
		do
		{		
			// Linear interpolation
			__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
			__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
			__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
			__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
			// Store and advance
			__storeunalignedvector( v4OutA, pfOutBuf );
			__storeunalignedvector( v4OutB, pfOutBuf+4 );
			pfOutBuf += uNumChannels;
			// Resampling factor interpolate
			++uRampCount;	
			uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
			vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
			vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
			uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;	
		} while ( uPreviousFrameIndex  == 0 && --uMaxNumIter );
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
	// 4 frames * blockalign = 32 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = AkMin( (AkUInt32) (pfOutBufEnd - pfOutBuf)/uNumChannels, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		AkUInt32 uPreviousFramePos = (vu4IndexFP.u[0] >> FPBITS)*uNumChannels;
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFramePos] );
#endif
		// Load past input, put to 32-bit container and convert to normalized float
		__vector4 v4PreviousFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos] );
		__vector4 v4PreviousFrameA = __vperm( v4PreviousFrame, v4Zero, v4ShuffleA );
		v4PreviousFrameA = __vcuxwfp( v4PreviousFrameA, 7 );
		v4PreviousFrameA = __vsubfp( v4PreviousFrameA, v4UnsignedOffset );
		__vector4 v4PreviousFrameB = __vperm( v4PreviousFrame, v4Zero, v4ShuffleB );
		v4PreviousFrameB = __vcuxwfp( v4PreviousFrameB, 7 );
		v4PreviousFrameB = __vsubfp( v4PreviousFrameB, v4UnsignedOffset );
		// Load current input, put to 32-bit container and convert to normalized float
		__vector4 v4CurrentFrame = __loadunalignedvector( &pInBuf[uPreviousFramePos+uNumChannels] );
		__vector4 v4CurrentFrameA = __vperm( v4CurrentFrame, v4Zero, v4ShuffleA );
		v4CurrentFrameA = __vcuxwfp( v4CurrentFrameA, 7 );
		v4CurrentFrameA = __vsubfp( v4CurrentFrameA, v4UnsignedOffset );
		__vector4 v4CurrentFrameB = __vperm( v4CurrentFrame, v4Zero, v4ShuffleB );
		v4CurrentFrameB = __vcuxwfp( v4CurrentFrameB, 7 );
		v4CurrentFrameB = __vsubfp( v4CurrentFrameB, v4UnsignedOffset );
		// Linear interpolation
		__vector4 v4FrameDiffA = __vsubfp( v4CurrentFrameA, v4PreviousFrameA );
		__vector4 v4FrameDiffB = __vsubfp( v4CurrentFrameB, v4PreviousFrameB );
		__vector4 v4OutA = __vmaddfp( v4FrameDiffA, vf4InterpLoc, v4PreviousFrameA );
		__vector4 v4OutB = __vmaddfp( v4FrameDiffB, vf4InterpLoc, v4PreviousFrameB );
		// Store and advance
		__storeunalignedvector( v4OutA, pfOutBuf );
		__storeunalignedvector( v4OutB, pfOutBuf+4 );
		pfOutBuf += uNumChannels;
		// Resampling factor interpolate
		++uRampCount;	
		uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
		vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		// Index advance and interpolation factor computation
		vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
		vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_U8_NCHAN( vu4IndexFP.u[0] );
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart)/uNumChannels;
	PITCH_INTERPOLATING_DSP_TEARDOWN( vu4IndexFP.u[0] );
}


// Interpolating resampling (pitch changes) with floating point samples optimized for one channel signals.
AKRESULT Interpolating_Native_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 

	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 fLeftSample = *io_pPitchState->fLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);	// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkReal32 fSampleDiff = pInBuf[1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);				// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;	
	}

	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 64 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		__vector4 v4Mult4Shift;
		v4Mult4Shift.u[0] = 2;
		v4Mult4Shift = __vspltw( v4Mult4Shift, 0 );
		while ( uNumIterBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vslw( v4PreviousFrameIndex, v4Mult4Shift ); // Time 4 bytes per sample		
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );		 // Current frame is 4 bytes passed previous frame

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif

			// Load all necessary inputs (unrolled 4x)
			__vector4 v4Prev1 = __lvlx( pInBuf, v4PreviousFramePos.u[0] );
			__vector4 v4Cur1 = __lvlx( pInBuf, v4CurrentFramePos.u[0] );
			__vector4 v4Prev2 = __lvlx( pInBuf, v4PreviousFramePos.u[1] );
			__vector4 v4Cur2 = __lvlx( pInBuf, v4CurrentFramePos.u[1] );
			__vector4 v4Prev3 = __lvlx( pInBuf, v4PreviousFramePos.u[2] );
			__vector4 v4Cur3 = __lvlx( pInBuf, v4CurrentFramePos.u[2] );
			__vector4 v4Prev4 = __lvlx( pInBuf, v4PreviousFramePos.u[3] );
			__vector4 v4Cur4 = __lvlx( pInBuf, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4Prev13 = __vmrghw( v4Prev1, v4Prev3 );
			__vector4 v4Prev24 = __vmrghw( v4Prev2, v4Prev4 );
			__vector4 v4Cur13 = __vmrghw( v4Cur1, v4Cur3 );
			__vector4 v4Cur24 = __vmrghw( v4Cur2, v4Cur4 );
			__vector4 v4Prev = __vmrghw( v4Prev13, v4Prev24 );
			__vector4 v4Cur = __vmrghw( v4Cur13, v4Cur24 );

			// Linear interpolation
			__vector4 v4Diff = __vsubfp( v4Cur, v4Prev );
			__vector4 v4ScaledDiff = __vmulfp( v4Diff, v4InterpLoc );
			__vector4 v4Out = __vaddfp( v4Prev, v4ScaledDiff );
		
			// Index advance
			uRampCount += 4;
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4Out, pfOutBuf );
			pfOutBuf += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		fLeftSample = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	AKASSERT( uSimPreviousFrameIndex == uPreviousFrameIndex ); // Simulation was accurate

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for 2 channel signals.
AKRESULT Interpolating_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// IMPORTANT: Second channel access relies on the fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkReal32 fPreviousFrameL = io_pPitchState->fLastValue[0];
	AkReal32 fPreviousFrameR = io_pPitchState->fLastValue[1];
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);	// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkReal32 fSampleDiffL = pInBuf[1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[1+uMaxFrames] - fPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
	// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
	// another call to fixed mode once target resampling factor has been reached.

	// Figure out how many frames by simulating loop considering interpolated resampling factor.
	// Note: Not storing result here because worst case scenario requires to much memory
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
	uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);				// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	AkUInt32 uNumInterpFrames = 0;
	AkUInt32 uSimRampCount = uRampCount;
	AkUInt32 uSimIndexFP = uIndexFP;
	AkUInt32 uSimPreviousFrameIndex = uPreviousFrameIndex;
	while ( uSimPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
	{	
		++uNumInterpFrames;	
		// Interpolate resampling factor	
		++uSimRampCount;
		AkUInt32 uSimFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uSimRampCount)/PITCHRAMPLENGTH;	
		// Index advance
		uSimIndexFP += uSimFrameSkipFP;
		uSimPreviousFrameIndex = uSimIndexFP >> FPBITS;			
	}
	
	// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x
	// 4 frames * 4 times unrolled * blockalign = 128 bytes per iteration consumed max. 
	// Since cache line is 128 bytes, single prefetch per iteration is ok

	// Loop unroll to be able to vectorize and avoid cast LHS
	if ( uNumInterpFrames >= 4 )
	{
		AkReal32 * AK_RESTRICT pInBufL = (AkReal32 * AK_RESTRICT) pInBuf;
		AkReal32 * AK_RESTRICT pInBufR = (AkReal32 * AK_RESTRICT) (pInBuf + uMaxFrames);
		AkReal32 * AK_RESTRICT pfOutBufL = (AkReal32 * AK_RESTRICT) pfOutBuf;
		AkReal32 * AK_RESTRICT pfOutBufR = (AkReal32 * AK_RESTRICT) (pfOutBuf + uMaxFrames);

		// lvlx assumes it can read 4 bytes in the same load operation
		AKASSERT( (AkUInt32)pInBuf % 4 == 0 ); 
		AkUInt32 uNumIterBlocks = uNumInterpFrames / 4;
		uNumInterpFrames -= uNumIterBlocks * 4;
		__vector4 v4IndexFP;
		v4IndexFP.u[0] = uIndexFP; // Already computed
		v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
		v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
		v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;
		__vector4 v4ShiftFPBITS;
		v4ShiftFPBITS.u[0] = FPBITS;
		v4ShiftFPBITS = __vspltw( v4ShiftFPBITS, 0 );
		__vector4 v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
		__vector4 v4AndFPMASK;
		v4AndFPMASK.u[0] = FPMASK;
		v4AndFPMASK = __vspltw( v4AndFPMASK, 0 );
		__vector4 v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
		__vector4 v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		__vector4 v4Four;
		v4Four.u[0] = 4;
		v4Four = __vspltw( v4Four, 0 );
		__vector4 v4Mult4Shift;
		v4Mult4Shift.u[0] = 2;
		v4Mult4Shift = __vspltw( v4Mult4Shift, 0 );
		while ( uNumIterBlocks-- )
		{
			__vector4 v4PreviousFramePos = __vslw( v4PreviousFrameIndex, v4Mult4Shift );	// Time 4 bytes per sample	
			__vector4 v4CurrentFramePos = __vadduws( v4PreviousFramePos, v4Four );			// Current frame 4 bytes passed previous	

#ifdef USEPITCHPREFETCH
			AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[v4PreviousFramePos.u[0]] );
#endif		
			
			// Load all necessary inputs (unrolled 4x)
			// Note: Possibly saturating load unit here, should compare if looping for each channel is better
			// It will put less stress on loading but will need to recompute sample index twice as much...
			__vector4 v4PrevL1 = __lvlx( pInBufL, v4PreviousFramePos.u[0] );
			__vector4 v4CurL1 = __lvlx( pInBufL, v4CurrentFramePos.u[0] );
			__vector4 v4PrevR1 = __lvlx( pInBufR, v4PreviousFramePos.u[0] );
			__vector4 v4CurR1 = __lvlx( pInBufR, v4CurrentFramePos.u[0] );
			__vector4 v4PrevL2 = __lvlx( pInBufL, v4PreviousFramePos.u[1] );
			__vector4 v4CurL2 = __lvlx( pInBufL, v4CurrentFramePos.u[1] );
			__vector4 v4PrevR2 = __lvlx( pInBufR, v4PreviousFramePos.u[1] );
			__vector4 v4CurR2 = __lvlx( pInBufR, v4CurrentFramePos.u[1] );
			__vector4 v4PrevL3 = __lvlx( pInBufL, v4PreviousFramePos.u[2] );
			__vector4 v4CurL3 = __lvlx( pInBufL, v4CurrentFramePos.u[2] );
			__vector4 v4PrevR3 = __lvlx( pInBufR, v4PreviousFramePos.u[2] );
			__vector4 v4CurR3 = __lvlx( pInBufR, v4CurrentFramePos.u[2] );
			__vector4 v4PrevL4 = __lvlx( pInBufL, v4PreviousFramePos.u[3] );
			__vector4 v4CurL4 = __lvlx( pInBufL, v4CurrentFramePos.u[3] );
			__vector4 v4PrevR4 = __lvlx( pInBufR, v4PreviousFramePos.u[3] );
			__vector4 v4CurR4 = __lvlx( pInBufR, v4CurrentFramePos.u[3] );

			// Merge 1 and 3, 2 and 4 and merge results to get 1,2,3,4
			__vector4 v4PrevL13 = __vmrghw( v4PrevL1, v4PrevL3 );
			__vector4 v4PrevR13 = __vmrghw( v4PrevR1, v4PrevR3 );
			__vector4 v4PrevL24 = __vmrghw( v4PrevL2, v4PrevL4 );
			__vector4 v4PrevR24 = __vmrghw( v4PrevR2, v4PrevR4 );
			__vector4 v4CurL13 = __vmrghw( v4CurL1, v4CurL3 );
			__vector4 v4CurR13 = __vmrghw( v4CurR1, v4CurR3 );
			__vector4 v4CurL24 = __vmrghw( v4CurL2, v4CurL4 );
			__vector4 v4CurR24 = __vmrghw( v4CurR2, v4CurR4 );
			__vector4 v4PrevL = __vmrghw( v4PrevL13, v4PrevL24 );
			__vector4 v4PrevR = __vmrghw( v4PrevR13, v4PrevR24 );
			__vector4 v4CurL = __vmrghw( v4CurL13, v4CurL24 );
			__vector4 v4CurR = __vmrghw( v4CurR13, v4CurR24 );

			// Linear interpolation
			__vector4 v4DiffL = __vsubfp( v4CurL, v4PrevL );
			__vector4 v4DiffR = __vsubfp( v4CurR, v4PrevR );
			__vector4 v4ScaledDiffL = __vmulfp( v4DiffL, v4InterpLoc );
			__vector4 v4ScaledDiffR = __vmulfp( v4DiffR, v4InterpLoc );
			__vector4 v4OutL = __vaddfp( v4PrevL, v4ScaledDiffL );
			__vector4 v4OutR = __vaddfp( v4PrevR, v4ScaledDiffR );
		
			// Index advance
			uRampCount += 4;
			v4IndexFP.u[0] = v4IndexFP.u[3] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount))/PITCHRAMPLENGTH;
			v4IndexFP.u[1] = v4IndexFP.u[0] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+1))/PITCHRAMPLENGTH;
			v4IndexFP.u[2] = v4IndexFP.u[1] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+2))/PITCHRAMPLENGTH;
			v4IndexFP.u[3] = v4IndexFP.u[2] + (iScaledStartFrameSkip + iFrameSkipDiff*(uRampCount+3))/PITCHRAMPLENGTH;

			// Store output
			__storeunalignedvector( v4OutL, pfOutBufL );
			__storeunalignedvector( v4OutR, pfOutBufR );
			pfOutBufL += 4;
			pfOutBufR += 4;

			// Update indexes and interpolation factor
			v4PreviousFrameIndex = __vsrw( v4IndexFP, v4ShiftFPBITS );
			v4InterpLocFP = __vand(  v4IndexFP, v4AndFPMASK );
			v4InterpLoc = __vcuxwfp ( v4InterpLocFP, FPBITS );
		}
		uIndexFP = v4IndexFP.u[0];
		uPreviousFrameIndex = uIndexFP >> FPBITS;
		uInterpLocFP = uIndexFP & FPMASK;
		pInBuf = pInBufL;
		pfOutBuf = pfOutBufL;
	}

	// For all other sample frames that require interpolation
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uNumInterpFrames-- )
	{
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		AkReal32 fSampleDiffR = pInBuf[uPreviousFrameR+1] - fPreviousFrameR;	
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for any number of channels.
AKRESULT Interpolating_Native_NChan_Vec(	AkAudioBuffer * io_pInBuffer, 
											AkAudioBuffer * io_pOutBuffer,
											AkUInt32 uRequestedSize,
											AkInternalPitchState * io_pPitchState )
{
	// PITCH_FIXED_DSP_SETUP_VEC( ) but delaying some calculation for each channel
	AKASSERT( io_pOutBuffer->MaxFrames() >= io_pPitchState->uOutFrameOffset );
	const AkUInt32 uInBufferFrames = io_pInBuffer->uValidFrames;	
	const AkUInt32 uOutBufferFrames = uRequestedSize - io_pPitchState->uOutFrameOffset;
	static const __vector4 vu4AndFPMASK = { 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/, 9.1834e-041 /*FPMASK*/ };
	AkUInt32 uFrameSkipFP = io_pPitchState->uCurrentFrameSkip;

	PITCH_INTERPOLATION_SETUP( );
	
	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
	AkUInt32 uFramesProduced;
	__vector4 vu4IndexFP;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		vu4IndexFP = __lvlx( &io_pPitchState->uFloatIndex, 0 );
		vu4IndexFP = __vspltw( vu4IndexFP, 0 );
		__vector4 vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
		__vector4 vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
		uFrameSkipFP = io_pPitchState->uCurrentFrameSkip;
		__vector4 vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
		vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
		uRampCount = io_pPitchState->uInterpolationRampCount; 

		// Minus one to compensate for offset of 1 due to zero == previous
		AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( i ) + io_pPitchState->uInFrameOffset - 1; 
		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( i ) + io_pPitchState->uOutFrameOffset;
		const AkReal32 * pfOutBufStart = pfOutBuf;
		const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

#ifdef USEPITCHPREFETCH
		for( int j = 0; j < AKSIMD_ARCHMAXPREFETCHSIZE; j += AKSIMD_ARCHCACHELINESIZE )
			AKSIMD_PREFETCHMEMORY( j, pInBuf );
#endif

		AkUInt32 uMaxNumIter = AkMin( uOutBufferFrames, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
		AkUInt32 uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
		if ( uPreviousFrameIndex  == 0 && uMaxNumIter )
		{	
			// Use stored value as left value, while right index is on the first sample
			__vector4 v4PreviousFrame = __lvlx( &io_pPitchState->fLastValue[i], 0 );
			__vector4 v4CurrentFrame = __lvlx( &pInBuf[1], 0 );
			do
			{
				// Linear interpolation
				__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
				__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
				// Store and advance
				__stvlx( v4Out, pfOutBuf, 0 );
				++pfOutBuf;
				// Resampling factor interpolate
				++uRampCount;	
				uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
				vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
				vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
				// Index advance and interpolation factor computation
				vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
				vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
				vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
				uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
			} while ( uPreviousFrameIndex  == 0 && --uMaxNumIter );
		}

		// Note: We want to leave the interpolating mode as soon as possible because it is less efficient than fixed mode. 
		// For this reason we do not try to fullfil the request event if we can because we'd rather have the pitch node make
		// another call to fixed mode once target resampling factor has been reached.

		// Note: Prefetching is based on the fact that the fastest input consumption rate occurs when effective resample ratio is 4x (48 kHz, +2400)
		// 4 frames * blockalign = 128 bytes per iteration consumed max. 
		// Since cache line is 128 bytes, single prefetch per iteration is ok

		// Figure out how many frames by simulating loop considering interpolated resampling factor.
		// Note: Not storing result here because worst case scenario requires to much memory
		const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;					
		uMaxNumIter = AkMin( (AkUInt32) (pfOutBufEnd - pfOutBuf)/uNumChannels, PITCHRAMPLENGTH-uRampCount );	// Not more than output frames and not longer than interpolation ramp length
		while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uMaxNumIter-- )
		{
#ifdef USEPITCHPREFETCH
		AKSIMD_PREFETCHMEMORY( AKSIMD_ARCHMAXPREFETCHSIZE, &pInBuf[uPreviousFrameIndex] );
#endif
			__vector4 v4PreviousFrame = __lvlx( &pInBuf[uPreviousFrameIndex], 0 );
			__vector4 v4CurrentFrame = __lvlx( &pInBuf[uPreviousFrameIndex+1], 0 );
			// Linear interpolation
			__vector4 v4FrameDiff = __vsubfp( v4CurrentFrame, v4PreviousFrame );
			__vector4 v4Out = __vmaddfp( v4FrameDiff, vf4InterpLoc, v4PreviousFrame );
			// Store and advance
			__stvlx( v4Out, pfOutBuf, 0 );
			++pfOutBuf;
			// Resampling factor interpolate
			++uRampCount;	
			uFrameSkipFP = (iScaledStartFrameSkip + iFrameSkipDiff*uRampCount)/PITCHRAMPLENGTH;
			vu4FrameSkipFP = __lvlx( &uFrameSkipFP, 0 );
			vu4FrameSkipFP = __vspltw( vu4FrameSkipFP, 0 );
			// Index advance and interpolation factor computation
			vu4IndexFP = __vadduws( vu4IndexFP, vu4FrameSkipFP );
			vu4InterpLocFP = __vand(  vu4IndexFP, vu4AndFPMASK );
			vf4InterpLoc = __vcuxwfp ( vu4InterpLocFP, FPBITS );	
			uPreviousFrameIndex = vu4IndexFP.u[0] >> FPBITS;
		}

		uFramesProduced = AkUInt32(pfOutBuf - pfOutBufStart);
	} // for all channels

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_NCHAN( vu4IndexFP.u[0] );
	PITCH_INTERPOLATING_DSP_TEARDOWN( vu4IndexFP.u[0] );
}


