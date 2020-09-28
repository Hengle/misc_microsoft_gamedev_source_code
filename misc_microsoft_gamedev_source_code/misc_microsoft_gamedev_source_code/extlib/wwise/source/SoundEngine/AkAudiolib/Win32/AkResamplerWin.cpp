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
// AkResamplerWin.cpp
// 
// Windows specific code
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h" 
#include "AkResamplerCommon.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "xmmintrin.h"

/********************* BYPASS DSP ROUTINES **********************/

// Bypass (no pitch or resampling) with INTERLEAVED signed 16-bit samples optimized for 2 channel signals. 
AKRESULT Bypass_I16_2Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_BYPASS_DSP_SETUP( );

	// IMPORTANT: Second channel access relies on the fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.
	AkInt16 * AK_RESTRICT pIn = (AkInt16 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->iLastValue[0] = pIn[2*uLastSample];
	io_pPitchState->iLastValue[1] = pIn[2*uLastSample+1];

	AkUInt32 uNumIter = uFramesToCopy / 16;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*16);

	__m64 * AK_RESTRICT pm64In = (__m64* AK_RESTRICT) pIn;
	__m128 * AK_RESTRICT pm128Out = (__m128* AK_RESTRICT) pOut;

	const __m64 * AK_RESTRICT pm64InEnd = pm64In + 8*uNumIter;
	const __m128 m128Scale = _mm_set_ps1( NORMALIZEFACTORI16 );
	const __m128 m128Zero = _mm_setzero_ps();

	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames()/4;

	// Process blocks first
	while ( pm64In < pm64InEnd )
	{
		__m64 m64InSamples1 = pm64In[0];	
		__m64 m64InSamples2 = pm64In[1];
		__m64 m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		__m64 m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		__m64 m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		__m64 m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		__m128 m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		__m128 m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		_mm_storeu_ps( (AkReal32 *)pm128Out, _mm_mul_ps( m128OutL, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[uMaxFrames], _mm_mul_ps( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[2];	
		m64InSamples2 = pm64In[3];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1], _mm_mul_ps( m128OutL, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1+uMaxFrames], _mm_mul_ps( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[4];	
		m64InSamples2 = pm64In[5];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2], _mm_mul_ps( m128OutL, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2+uMaxFrames], _mm_mul_ps( m128OutR, m128Scale ) ); 

		m64InSamples1 = pm64In[6];	
		m64InSamples2 = pm64In[7];
		m64InSamplesLLow = _mm_slli_pi32( m64InSamples1, 16 );  // Prepare left samples for conversion
		m64InSamplesRLow = _mm_srai_pi32( m64InSamples1, 16 );  // Prepare right samples for conversion
		m64InSamplesLLow = _mm_srai_pi32( m64InSamplesLLow, 16 ); 		
		m64InSamplesLHi = _mm_slli_pi32( m64InSamples2, 16 );  // Prepare left samples for conversion
		m64InSamplesRHi = _mm_srai_pi32( m64InSamples2, 16 );  // Prepare right samples for conversion
		m64InSamplesLHi = _mm_srai_pi32( m64InSamplesLHi, 16 ); 
		m128OutL = _mm_cvtpi32_ps(m128Zero, m64InSamplesLHi );
		m128OutR = _mm_cvtpi32_ps(m128Zero, m64InSamplesRHi );
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutL, m128OutL ), m64InSamplesLLow );
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps( m128OutR, m128OutR ), m64InSamplesRLow );
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3], _mm_mul_ps( m128OutL, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3+uMaxFrames], _mm_mul_ps( m128OutR, m128Scale ) ); 

		pm64In += 8;
		pm128Out += 4;
	}

	_mm_empty();
	pIn = (AkInt16 * AK_RESTRICT) pm64In;
	pOut = (AkReal32* AK_RESTRICT) pm128Out;

	// Process blocks of 4 first
	uMaxFrames = io_pOutBuffer->MaxFrames();
	while ( uRemaining-- )
	{	
		*pOut = INT16_TO_FLOAT( *pIn++ );
		pOut[uMaxFrames] = INT16_TO_FLOAT( *pIn++ );
		++pOut;
	}

	PITCH_BYPASS_DSP_TEARDOWN( );
}

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
	
	// Note: No input data constraint (MMX)
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	const AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);
	
	__m64 * AK_RESTRICT pm64In = (__m64 * AK_RESTRICT) pIn;
	__m128 * AK_RESTRICT pm128Out = (__m128 * AK_RESTRICT) pOut;
	const __m64 * AK_RESTRICT pm64InEnd = pm64In + 4*uNumIter;

	const __m128 m128Scale = _mm_set_ps1( NORMALIZEFACTORI16 );
	const __m64 m64Zero = _mm_setzero_si64();
	const __m128 m128Zero = _mm_setzero_ps();

	// Process blocks of 16 frames
	while ( pm64In < pm64InEnd )
	{
		__m64 m64InSamples1 = pm64In[0];
		__m64 m64InSamples2 = pm64In[1];
		__m64 m64InSamples3 = pm64In[2];
		__m64 m64InSamples4 = pm64In[3];
		pm64In += 4;
		
		__m64 m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples1);									// Retrieve sign for proper sign extension	
		__m128 m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples1, m64Sign));	// Interleave to 32 bits and convert (hi)
		__m128 m128Out1 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples1, m64Sign)); // Interleave to 32 bits and convert (lo) and merge with previous result by shifting up high part

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples2);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples2, m64Sign));	
		__m128 m128Out2 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples2, m64Sign));

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples3);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples3, m64Sign));	
		__m128 m128Out3 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples3, m64Sign));

		m64Sign = _mm_cmpgt_pi16(m64Zero,m64InSamples4);				
		m128Tmp = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64InSamples4, m64Sign));	
		__m128 m128Out4 = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp, m128Tmp), _mm_unpacklo_pi16(m64InSamples4, m64Sign));

		_mm_storeu_ps( (AkReal32 *)&pm128Out[0], _mm_mul_ps( m128Out1, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1], _mm_mul_ps( m128Out2, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2], _mm_mul_ps( m128Out3, m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3], _mm_mul_ps( m128Out4, m128Scale ) ); 
		pm128Out += 4;
	}

	_mm_empty();

	// Advance data pointers for remaining samples
	pIn = (AkInt16 * AK_RESTRICT) pm64In;
	pOut = (AkReal32 * AK_RESTRICT) pm128Out;

	// Deal with remaining samples
	while ( uRemaining-- )
	{
		*pOut++ = INT16_TO_FLOAT( *pIn++ ) ;
	}

	// Need to keep the last buffer value in case we start the pitch algo next buffer for each channel
	pIn -= uNumChannels;
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		io_pPitchState->iLastValue[i] = pIn[i];
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

	// IMPORTANT: Second channel access relies on the (hidden) fact that at least the 
	// first 2 channels of AkAudioBuffer are contiguous in memory.
	AkUInt8 * AK_RESTRICT pIn = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData() ) + 2*io_pPitchState->uInFrameOffset;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetChannel( 0 ) ) + io_pPitchState->uOutFrameOffset;

	// Need to keep the last buffer value in case we start the pitch algo next buffer
	io_pPitchState->uLastValue[0] = pIn[2*uLastSample];
	io_pPitchState->uLastValue[1] = pIn[2*uLastSample+1];

	__m64 * AK_RESTRICT pm64In = (__m64 * AK_RESTRICT) pIn;
	__m128 * AK_RESTRICT pm128Out = (__m128 * AK_RESTRICT) pOut;

	const __m128 m128Offset = _mm_set_ps1( DENORMALIZEOFFSETU8 );
	const __m128 m128Scale = _mm_set_ps1( NORMALIZEFACTORU8 );
	const __m64 m64Zero = _mm_setzero_si64();
	const __m128 m128Zero = _mm_setzero_ps();
	const __m64 m64MaskLeft = _mm_set_pi32(0x00FF00FF,0x00FF00FF);

	// Note: No input data constraint (MMX)
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	AkUInt32 uNumIter = uFramesToCopy / 16;
	AkUInt32 uRemaining = uFramesToCopy - (uNumIter*16);
	const __m64 * AK_RESTRICT pm64InEnd = pm64In + 4*uNumIter;

	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames()/4;

	// Process blocks of 16 frames
	while ( pm64In < pm64InEnd )
	{
		__m64 m64InSamples1 = pm64In[0];
		__m64 m64InSamples2 = pm64In[1];
		__m64 m64InSamples3 = pm64In[2];
		__m64 m64InSamples4 = pm64In[3];
		pm64In += 4;

		__m64 m64Shuffled1 = _mm_and_si64( m64InSamples1, m64MaskLeft );	// Keep only left samples	
		__m64 m64Shuffled2 = _mm_srli_pi16( m64InSamples1, 8 );				// Keep only right samples	
		__m128 m128Tmp1 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled1, m64Zero));	// Interleave and convert hi
		__m128 m128Tmp2 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled2, m64Zero));	// Interleave and convert hi	
		__m128 m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp1, m128Tmp1), _mm_unpacklo_pi16(m64Shuffled1, m64Zero)); // Interleave low convert and merge with previous results
		__m128 m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp2, m128Tmp2), _mm_unpacklo_pi16(m64Shuffled2, m64Zero)); // Interleave low convert and merge with previous results
		_mm_storeu_ps( (AkReal32 *)&pm128Out[0], _mm_mul_ps( _mm_sub_ps( m128OutL, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[uMaxFrames], _mm_mul_ps( _mm_sub_ps( m128OutR, m128Offset ), m128Scale ) );

		m64Shuffled1 = _mm_and_si64( m64InSamples2, m64MaskLeft );	// Keep only left samples	
		m64Shuffled2 = _mm_srli_pi16( m64InSamples2, 8 );			// Keep only right samples	
		m128Tmp1 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled1, m64Zero));	// Interleave and convert hi
		m128Tmp2 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled2, m64Zero));	// Interleave and convert hi	
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp1, m128Tmp1), _mm_unpacklo_pi16(m64Shuffled1, m64Zero)); // Interleave low convert and merge with previous results
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp2, m128Tmp2), _mm_unpacklo_pi16(m64Shuffled2, m64Zero)); // Interleave low convert and merge with previous results
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1], _mm_mul_ps( _mm_sub_ps( m128OutL, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1+uMaxFrames], _mm_mul_ps( _mm_sub_ps( m128OutR, m128Offset ), m128Scale ) );

		m64Shuffled1 = _mm_and_si64( m64InSamples3, m64MaskLeft );	// Keep only left samples	
		m64Shuffled2 = _mm_srli_pi16( m64InSamples3, 8 );			// Keep only right samples	
		m128Tmp1 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled1, m64Zero));	// Interleave and convert hi
		m128Tmp2 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled2, m64Zero));	// Interleave and convert hi	
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp1, m128Tmp1), _mm_unpacklo_pi16(m64Shuffled1, m64Zero)); // Interleave low convert and merge with previous results
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp2, m128Tmp2), _mm_unpacklo_pi16(m64Shuffled2, m64Zero)); // Interleave low convert and merge with previous results
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2], _mm_mul_ps( _mm_sub_ps( m128OutL, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2+uMaxFrames], _mm_mul_ps( _mm_sub_ps( m128OutR, m128Offset ), m128Scale ) );

		m64Shuffled1 = _mm_and_si64( m64InSamples4, m64MaskLeft );	// Keep only left samples	
		m64Shuffled2 = _mm_srli_pi16( m64InSamples4, 8 );			// Keep only right samples	
		m128Tmp1 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled1, m64Zero));	// Interleave and convert hi
		m128Tmp2 = _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Shuffled2, m64Zero));	// Interleave and convert hi	
		m128OutL = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp1, m128Tmp1), _mm_unpacklo_pi16(m64Shuffled1, m64Zero)); // Interleave low convert and merge with previous results
		m128OutR = _mm_cvtpi32_ps(_mm_movelh_ps(m128Tmp2, m128Tmp2), _mm_unpacklo_pi16(m64Shuffled2, m64Zero)); // Interleave low convert and merge with previous results
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3], _mm_mul_ps( _mm_sub_ps( m128OutL, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3+uMaxFrames], _mm_mul_ps( _mm_sub_ps( m128OutR, m128Offset ), m128Scale ) );

		pm128Out += 4;
	}

	_mm_empty();

	// Advance data pointers for remaining values
	pIn = (AkUInt8 * AK_RESTRICT) pm64In;
	pOut = (AkReal32 * AK_RESTRICT) pm128Out;

	// Deal with remaining samples
	uMaxFrames = io_pOutBuffer->MaxFrames();
	while ( uRemaining-- )
	{
		*pOut = UINT8_TO_FLOAT( *pIn++ );
		pOut[uMaxFrames] = UINT8_TO_FLOAT( *pIn++ );
		++pOut;
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
	AkUInt8 * AK_RESTRICT pIn = (AkUInt8 * AK_RESTRICT)( io_pInBuffer->GetInterleavedData( ) ) + io_pPitchState->uInFrameOffset*uNumChannels;
	AkReal32 * AK_RESTRICT pOut = (AkReal32 * AK_RESTRICT)( io_pOutBuffer->GetInterleavedData( ) ) + io_pPitchState->uOutFrameOffset*uNumChannels;

	// Note: No input data constraint (MMX)
	// Note: Cannot assume output data alignment is on 16 byte boundaries
	const AkUInt32 uSamplesToCopy = uFramesToCopy*uNumChannels;
	AkUInt32 uNumIter = uSamplesToCopy / 16;
	AkUInt32 uRemaining = uSamplesToCopy - (uNumIter*16);

	__m64 * AK_RESTRICT pm64In = (__m64 * AK_RESTRICT) pIn;
	__m128 * AK_RESTRICT pm128Out = (__m128 * AK_RESTRICT) pOut;
	const __m64 * AK_RESTRICT pm64InEnd = pm64In + 2*uNumIter;

	const __m128 m128Offset = _mm_set_ps1( DENORMALIZEOFFSETU8 );
	const __m128 m128Scale = _mm_set_ps1( NORMALIZEFACTORU8 );
	const __m64 m64Zero = _mm_setzero_si64();
	const __m128 m128Zero = _mm_setzero_ps();

	// Process blocks of 16 frames
	while ( pm64In < pm64InEnd )
	{
		__m64 m64InSamples1 = pm64In[0];	
		__m64 m64InSamples2 = pm64In[1];
		pm64In += 2;
		__m64 m64Unpack1 = _mm_unpacklo_pi8(m64InSamples1, m64Zero); 
		__m64 m64Unpack2 = _mm_unpackhi_pi8(m64InSamples1, m64Zero);
		__m64 m64Unpack3 = _mm_unpacklo_pi8(m64InSamples2, m64Zero); 
		__m64 m64Unpack4 = _mm_unpackhi_pi8(m64InSamples2, m64Zero);
		__m128 m128Tmp1 = _mm_movelh_ps(_mm_cvtpi32_ps(m128Zero, _mm_unpacklo_pi16(m64Unpack1, m64Zero)), _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Unpack1, m64Zero ))); 
		__m128 m128Tmp2 = _mm_movelh_ps(_mm_cvtpi32_ps(m128Zero, _mm_unpacklo_pi16(m64Unpack2, m64Zero)), _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Unpack2, m64Zero ))); 
		__m128 m128Tmp3 = _mm_movelh_ps(_mm_cvtpi32_ps(m128Zero, _mm_unpacklo_pi16(m64Unpack3, m64Zero)), _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Unpack3, m64Zero ))); 
		__m128 m128Tmp4 = _mm_movelh_ps(_mm_cvtpi32_ps(m128Zero, _mm_unpacklo_pi16(m64Unpack4, m64Zero)), _mm_cvtpi32_ps(m128Zero, _mm_unpackhi_pi16(m64Unpack4, m64Zero ))); 
		_mm_storeu_ps( (AkReal32 *)pm128Out, _mm_mul_ps( _mm_sub_ps( m128Tmp1, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[1], _mm_mul_ps( _mm_sub_ps( m128Tmp2, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[2], _mm_mul_ps( _mm_sub_ps( m128Tmp3, m128Offset ), m128Scale ) ); 
		_mm_storeu_ps( (AkReal32 *)&pm128Out[3], _mm_mul_ps( _mm_sub_ps( m128Tmp4, m128Offset ), m128Scale ) ); 
		pm128Out += 4;
	}

	_mm_empty();

	// Advance data pointers for remaining values
	pIn = (AkUInt8 * AK_RESTRICT) pm64In;
	pOut = (AkReal32 * AK_RESTRICT) pm128Out;

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


///********************* FIXED RESAMPLING DSP ROUTINES **********************/

// Fixed resampling (no pitch changes) with signed 16-bit samples optimized for one channel signals.
AKRESULT Fixed_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 

	// Retrieve output buffer information
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

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

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;	
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
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();	
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_I16_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

// Fixed resampling (no pitch changes) with unsigned 8-bit samples, optimized for one channel signals.
AKRESULT Fixed_U8_1Chan(	AkAudioBuffer * io_pInBuffer, 
							AkAudioBuffer * io_pOutBuffer,
							AkUInt32 uRequestedSize,
							AkInternalPitchState * io_pPitchState )
{
	PITCH_FIXED_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

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

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;
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
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrameL = io_pPitchState->uLastValue[0];
	AkUInt8 iPreviousFrameR = io_pPitchState->uLastValue[1];
	AkUInt32 uIterFrames = uNumIterPreviousFrame;
	while ( uIterFrames-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	// Determine number of iterations remaining
	AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
	AkUInt32 uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_U8_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
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

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
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

	// For all other sample frames
	uIterFrames = uNumIterThisFrame;
	while ( uIterFrames-- )
	{
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
		AkReal32 fSampleDiffR = pInBuf[uPreviousFrameR+1] - fPreviousFrameR;	
		*pfOutBuf = LINEAR_INTERP_NATIVE( fPreviousFrameL, fSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_NATIVE( fPreviousFrameR, fSampleDiffR );
		++pfOutBuf;
		FP_INDEX_ADVANCE();
	}

	PITCH_SAVE_NEXT_NATIVE_2CHAN();
	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
}

/********************* INTERPOLATING RESAMPLING DSP ROUTINES **********************/

// Interpolating resampling (pitch changes) with signed 16-bit samples, optimized for one channel signals.
AKRESULT Interpolating_I16_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
									AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	
	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
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

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);			// No more than the output buffer length
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;	
		*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();	
		FP_INDEX_ADVANCE();
	}

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
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkInt16 iPreviousFrameL = io_pPitchState->iLastValue[0];
	AkInt16 iPreviousFrameR = io_pPitchState->iLastValue[1];
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Avoid inner branch by splitting in 2 cases
	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_I16( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_I16( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_I16_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

// Interpolating resampling (pitch changes) with unsigned 8-bit samples optimized for one channel signals.
AKRESULT Interpolating_U8_1Chan(	AkAudioBuffer * io_pInBuffer, 
									AkAudioBuffer * io_pOutBuffer,
								AkUInt32 uRequestedSize,
									AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT) io_pInBuffer->GetInterleavedData( ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	// Note: No interpolation necessary for the first few frames
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

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		iPreviousFrame = pInBuf[uPreviousFrameIndex];
		AkInt32 iSampleDiff = pInBuf[uPreviousFrameIndex+1] - iPreviousFrame;
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
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkUInt8 * AK_RESTRICT pInBuf = (AkUInt8 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + 2*io_pPitchState->uInFrameOffset - 2; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	AkUInt8 iPreviousFrameL = io_pPitchState->uLastValue[0];
	AkUInt8 iPreviousFrameR = io_pPitchState->uLastValue[1];
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkInt32 iSampleDiffL = pInBuf[2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		AkUInt32 uPreviousFrameSamplePosL = uPreviousFrameIndex*2;
		iPreviousFrameL = pInBuf[uPreviousFrameSamplePosL];
		iPreviousFrameR = pInBuf[uPreviousFrameSamplePosL+1];
		AkInt32 iSampleDiffL = pInBuf[uPreviousFrameSamplePosL+2] - iPreviousFrameL;
		AkInt32 iSampleDiffR = pInBuf[uPreviousFrameSamplePosL+3] - iPreviousFrameR;
		*pfOutBuf = LINEAR_INTERP_U8( iPreviousFrameL, iSampleDiffL );
		pfOutBuf[uMaxFrames] = LINEAR_INTERP_U8( iPreviousFrameR, iSampleDiffR );
		++pfOutBuf;
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_U8_2CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
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
	// Note: No interpolation necessary for the first few frames
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkReal32 fLeftSample = *io_pPitchState->fLastValue;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
	uMaxNumIter = AkMin( uMaxNumIter, PITCHRAMPLENGTH-uRampCount );	// Not longer than interpolation ramp length
	while ( uPreviousFrameIndex == 0 && uMaxNumIter-- )
	{
		AkReal32 fSampleDiff = pInBuf[1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	// Avoid inner branch by splitting in 2 cases
	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		fLeftSample = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiff = pInBuf[uPreviousFrameIndex+1] - fLeftSample;
		*pfOutBuf++ = LINEAR_INTERP_NATIVE( fLeftSample, fSampleDiff );
		RESAMPLING_FACTOR_INTERPOLATE();
		FP_INDEX_ADVANCE();
	}

	PITCH_INTERPOLATION_TEARDOWN( );
	PITCH_SAVE_NEXT_NATIVE_1CHAN();
	AkUInt32 uFramesProduced = (AkUInt32)(pfOutBuf - pfOutBufStart);
	PITCH_INTERPOLATING_DSP_TEARDOWN( uIndexFP );
}

//  Interpolating resampling (pitch changes) with DEINTERLEAVED floating point samples optimized for 2 channel signals.
AKRESULT Interpolating_Native_2Chan(	AkAudioBuffer * io_pInBuffer, 
										AkAudioBuffer * io_pOutBuffer,
										AkUInt32 uRequestedSize,
										AkInternalPitchState * io_pPitchState )
{
	PITCH_INTERPOLATING_DSP_SETUP( );
	AkUInt32 uMaxFrames = io_pOutBuffer->MaxFrames();

	// Minus one to compensate for offset of 1 due to zero == previous
	AkReal32 * AK_RESTRICT pInBuf = (AkReal32 * AK_RESTRICT) io_pInBuffer->GetChannel( 0 ) + io_pPitchState->uInFrameOffset - 1; 
	AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( 0 ) + io_pPitchState->uOutFrameOffset;
	const AkReal32 * pfOutBufStart = pfOutBuf;
	const AkReal32 * pfOutBufEnd = pfOutBuf + uOutBufferFrames;

	PITCH_INTERPOLATION_SETUP( );

	// Use stored value as left value, while right index is on the first sample
	// Note: No interpolation necessary for the first few frames
	AkReal32 fPreviousFrameL = io_pPitchState->fLastValue[0];
	AkReal32 fPreviousFrameR = io_pPitchState->fLastValue[1];
	static const AkReal32 fScale = 1.f / SINGLEFRAMEDISTANCE;
	AkUInt32 uMaxNumIter = (AkUInt32) (pfOutBufEnd - pfOutBuf);		// Not more than output frames
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

	// For all other sample frames that need interpolation
	const AkUInt32 uLastValidPreviousIndex = uInBufferFrames-1;
	AkUInt32 uIterFrames = (AkUInt32) (pfOutBufEnd - pfOutBuf);
	uIterFrames = AkMin( uIterFrames, PITCHRAMPLENGTH - uRampCount );	// No more than the interpolation ramp length
	while ( uPreviousFrameIndex <= uLastValidPreviousIndex && uIterFrames-- )
	{
		// Linear interpolation and index advance
		AkUInt32 uPreviousFrameR = uPreviousFrameIndex+uMaxFrames;
		fPreviousFrameL = pInBuf[uPreviousFrameIndex];
		AkReal32 fSampleDiffL = pInBuf[uPreviousFrameIndex+1] - fPreviousFrameL;
		fPreviousFrameR = pInBuf[uPreviousFrameR];	
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

//// Takes 2 vectores of 16 bit values and returns the result of the 8 multiplication in 2 vector of 32-bit values
//static AkForceInline void _mm_mul_ext32( __m128i * vResultLow, __m128i * vResultHigh, __m128i a , __m128i b)
//{
//	__m128i vHi = _mm_mulhi_epi16(a, b);
//	__m128i vLow = _mm_mullo_epi16(a, b);
//	*vResultHigh = _mm_unpackhi_epi16(vHi, vLow);
//	*vResultLow = _mm_unpacklo_epi16(vHi, vLow); 
//}

//static inline void PRECISION_EXTENDED_MULT_VEC( __m128i * vOutLo, __m128i * vOutHi, __m128i vi32AValue, __m128i vu16BValue )
//{
//	static const __m128i vOffset = _mm_set1_epi32( (1<<16) );
//	static const __m128i vLower16BitsMask = _mm_set1_epi32( 0xFFFF );
//
//	// Offset into unsigned range, since this is effectively a 17 bit value
//	__m128i vu32Value = _mm_add_epi32( vi32AValue, vOffset );
//	// Low part 16-bit value in 32-bit container for multiplication
//	__m128i vu32A2 = _mm_srai_epi32(vu32Value, 16);			
//	// High part 16-bit value in 32-bit container for multiplication
//	__m128i vu32A1 = _mm_and_si128(vu32Value, vLower16BitsMask);	
//	// 16-bit multiplication result held in 32-bit container with 2^16 offset since this is the high multiplication part
//	__m128i vu32A2B1_Lo, vu32A2B1_Hi;
//	_mm_mul_ext32( vu32A2B1_Lo, vu32A2B1_Hi, vu32A2, vu16BValue );
//	vu32A2B1_Hi = _mm_slli_epi32(vu32A2B1_Hi,16); 
//	vu32A2B1_Lo = _mm_slli_epi32(vu32A2B1_Lo,16);
//	// 16-bit multiplication result held in 32-bit container, no offset as this is the low part of the multiplication	
//	__m128i vu32A1B1_Lo, vu32A1B1_Hi;
//	_mm_mul_ext32( vu32A1B1_Lo, vu32A1B1_Hi, vu32A1, vu16BValue );
//	// Sum high and low multiplication parts
//	*vOutHi = _mm_add_epi32( vu32A2B1_Hi, vu32A1B1_Hi );
//	*vOutLo = _mm_add_epi32( vu32A2B1_Lo, vu32A1B1_Lo ); 
//	// Subtract unsigned offset considering the multiplication it went through
//	__m128i vScaledOffsetLo, vScaledOffsetHi;
//	_mm_mul_ext32( vScaledOffsetLo, vScaledOffsetHi, vu16BValue, vOffset );
//	*vOutHi = _mm_sub_epi32( *vOutHi, vScaledOffsetHi ); 
//	*vOutLo = _mm_sub_epi32( *vOutLo, vScaledOffsetLo ); 
//}

//static inline AkInt32 PRECISION_EXTENDED_MULT( AkInt32 i32AValue, AkUInt16 u16BValue )
//{
//	AkUInt32 u32Value = i32AValue + (1<<16);	// Offset into unsigned range, since this is effectively a 17 bit value
//	AkUInt32 u32A1 = (u32Value & 0xFFFF);	// High part 16-bit value in 32-bit container for multiplication
//	AkUInt32 u32A2 = (u32Value >> 16);		// Low part 16-bit value in 32-bit container for multiplication
//	// 16-bit multiplication result held in 32-bit container with 2^16 offset since this is the high multiplication part
//	AkUInt32 u32A2B1 = (u32A2 * u16BValue) << 16; 
//	// 16-bit multiplication result held in 32-bit container, no offset as this is the low part of the multiplication
//	AkUInt32 u32A1B1 = u32A1 * u16BValue; 
//	AkUInt32 u32MultTotal = u32A2B1 + u32A1B1; // Sum high and low multiplication parts
//	AkInt32 i32MultResult = u32MultTotal - u16BValue*(1<<16); // Subtract unsigned offset considering the multiplication it went through
//	return i32MultResult;
//}
//
//// Fixed resampling (no pitch changes) with INTERLEAVED signed 16-bit samples for any number of channels.
//AKRESULT Fixed_I16_NChan(	AkAudioBuffer * io_pInBuffer, 
//							AkAudioBuffer * io_pOutBuffer,
//							AkUInt32 uRequestedSize,
//							AkInternalPitchState * io_pPitchState )
//{
//	PITCH_FIXED_DSP_SETUP( );
//	const AkUInt32 uNumChannels = io_pInBuffer->NumChannels();
//	
//	// Minus one to compensate for offset of 1 due to zero == previous
//	AkInt16 * AK_RESTRICT pInBuf = (AkInt16 * AK_RESTRICT) io_pInBuffer->GetInterleavedData() + uNumChannels*io_pPitchState->uInFrameOffset - uNumChannels; 
//	AkUInt32 uNumIterThisFrame;
//	AkUInt32 uStartIndexFP = uIndexFP;
//	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
//	{
//		FP_INDEX_RESET( uStartIndexFP );
//
//		AkInt16 * AK_RESTRICT pInBufChan = pInBuf + i;
//		AkReal32 * AK_RESTRICT pfOutBuf = (AkReal32 * AK_RESTRICT) io_pOutBuffer->GetChannel( AkChannelRemap( i, io_pInBuffer->GetChannelMask() ) ) + io_pPitchState->uOutFrameOffset;
//		// Use stored value as left value, while right index is on the first sample
//		AkInt16 iPreviousFrame = io_pPitchState->iLastValue[i];
//		AkUInt32 uIterFrames = uNumIterPreviousFrame;
//		while ( uIterFrames-- )
//		{
//			AkInt32 iSampleDiff = pInBufChan[uNumChannels] - iPreviousFrame;
//			//*pfOutBuf++ = (AkReal32) ( ( iPreviousFrame << FPBITS ) + ( iSampleDiff * (AkInt32) uInterpLocFP ) ) * (NORMALIZEFACTORI16 / FPMUL);
//			AkInt32 i32PreviousFrame = iPreviousFrame << FPBITS;
//			AkInt32 i32MultResult = PRECISION_EXTENDED_MULT( iSampleDiff, (AkUInt16)uInterpLocFP );
//			AkInt32 i32Sum = i32PreviousFrame + i32MultResult;
//			AkReal32 fSum = (AkReal32)i32Sum;
//			AkReal32 fOut = fSum * (NORMALIZEFACTORI16 / FPMUL);
//			*pfOutBuf++ = fOut;
//
//			FP_INDEX_ADVANCE();	
//		}
//
//		// Determine number of iterations remaining
//		const AkUInt32 uPredNumIterFrames = ((uInBufferFrames << FPBITS) - uIndexFP + (uFrameSkipFP-1)) / uFrameSkipFP;
//		uNumIterThisFrame = AkMin( uOutBufferFrames-uNumIterPreviousFrame, uPredNumIterFrames );
//
//		// For all other sample frames
//		uIterFrames = uNumIterThisFrame;
//		while ( uIterFrames-- )
//		{
//			AkUInt32 uPreviousFrameSamplePos = uPreviousFrameIndex*uNumChannels;
//			iPreviousFrame = pInBufChan[uPreviousFrameSamplePos];
//			AkInt32 iSampleDiff = pInBufChan[uPreviousFrameSamplePos+uNumChannels] - iPreviousFrame;
//			*pfOutBuf++ = LINEAR_INTERP_I16( iPreviousFrame, iSampleDiff );
//			FP_INDEX_ADVANCE();
//		}
//	}
//
//	PITCH_SAVE_NEXT_I16_NCHAN( uIndexFP );
//	PITCH_FIXED_DSP_TEARDOWN( uIndexFP );	
//}

