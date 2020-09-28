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
// AkIIRCombFilter.h
//
// AudioKinetic IIR comb filter class
// y[n] = x[n] + g * y[n - D]
// A variant where the output also considers delayed version of input is also possible.
// y[n] = x[n - D] + g * y[n - D]
// Allocating a maximum amount of memory may be necessary for modulating delay length later.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKIIRCOMBFILTER_H_
#define _AKIIRCOMBFILTER_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AKPrivateTypes.h>
#include "assert.h"

/*
#ifdef WIN32 
#include "xmmintrin.h"
#elif XBOX360
#include "ppcintrinsics.h"
#endif
*/


class CAkIIRCombFilter
{
public:

	AK_USE_PLUGIN_ALLOCATOR();

    // Constructor/destructor.
	CAkIIRCombFilter();	 
	~CAkIIRCombFilter();

	AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_ulDelayLineLength );
	AKRESULT SetCoefficient( AkReal32 in_fFeedbackGain );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );
	void Reset( );
	AkReal32 ProcessSample( AkReal32 in_fXn );
	void	 ProcessBuffer( AkReal32 *in_pInBuffer, AkReal32 *in_pOutBuffer, AkUInt32 in_ulNumFrames );
	void	 ProcessBufferPrev( AkReal32 *in_pInBuffer, AkReal32 *in_pOutBuffer, AkUInt32 in_ulNumFrames );

private:

	// Clipping not necessary with more aggressive normalization, make up gain provided to compensate
	// void ClipSampleGroups( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples );
	// void ClipRemainingSamples( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples );
	// AkReal32 ClipSample( AkReal32 fYn );
    
	AkUInt32	m_ulDelayLineLength;	// Delay line length
	AkReal32	m_fG;					// Filter coefficient
	AkReal32 *	m_pfY;					// Output delay line (start)
	AkReal32 *	m_pfYPtr;				// Delay line read/write pointer
	AkReal32 *	m_pfYEnd;				// Delay line back boundary
	AkReal32	m_fNorm;				// Normalization coefficient 1-abs( g )
};


AkForceInline AkReal32 CAkIIRCombFilter::ProcessSample( AkReal32 in_fXn )
{
	assert( m_pfY != NULL );	
	AkReal32 fYn = m_fNorm * in_fXn + m_fG * *m_pfYPtr;	// Compute output
	*m_pfYPtr++ = fYn;	// Write output to delay line
	// Advance delay line pointer and wrap around if necessary
	if ( m_pfYPtr == m_pfYEnd )
	{
		m_pfYPtr = m_pfY;
	}
	return fYn;
}

AkForceInline void CAkIIRCombFilter::ProcessBuffer( AkReal32 * AK_RESTRICT in_pInBuffer, AkReal32 * AK_RESTRICT in_pOutBuffer, AkUInt32 in_ulNumFrames )
{
	assert( m_pfY != NULL );	
	assert( in_pInBuffer != NULL );	
	assert( in_pOutBuffer != NULL );	
	assert( in_ulNumFrames > 0 );	

	AkReal32 l_fG = m_fG;
	AkReal32 l_fNorm = m_fNorm;
	AkReal32 *l_pfYPtr = m_pfYPtr;
	AkReal32 *l_pfYEnd = m_pfYEnd;

	unsigned int i=in_ulNumFrames;
	if ( (l_pfYPtr + in_ulNumFrames ) >= l_pfYEnd )
	{
		while(i >= 4)
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer) + l_fG * *l_pfYPtr;	// Compute output

			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
				l_pfYPtr = m_pfY;
			*(in_pOutBuffer) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+1) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
				l_pfYPtr = m_pfY;
			*(in_pOutBuffer+1) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+2) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
				l_pfYPtr = m_pfY;
			*(in_pOutBuffer+2) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+3) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
				l_pfYPtr = m_pfY;
			*(in_pOutBuffer+3) = fYn; //overwrite output with sum

			in_pInBuffer += 4;
			in_pOutBuffer += 4;
			i -= 4;
		}

		//remaining
		while(i--)
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer++) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
			{
				l_pfYPtr = m_pfY;
			}
			*(in_pOutBuffer++) = fYn; //overwrite output with sum
		}
	}
	else
	{
		while(i > 4)
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+1) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer+1) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+2) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer+2) = fYn; //overwrite output with sum

			fYn = l_fNorm * *(in_pInBuffer+3) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer+3) = fYn; //overwrite output with sum

			in_pInBuffer += 4;
			in_pOutBuffer += 4;
			i -= 4;
		}

		//remaining
		while(i--)
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer++) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer++) = fYn; //overwrite output with sum
		}
	}
/*
	// Do not let the delay line grow out of bounds with high power signals -> clip
	AkUInt32 uSampleBlocks = (in_ulNumFrames / 4) * 4;
	ClipSampleGroups( m_pfYPtr, uSampleBlocks );
	AkUInt32 uSamplesRemaining = in_ulNumFrames-uSampleBlocks;
	if ( uSamplesRemaining )
		ClipRemainingSamples( m_pfYPtr, uSamplesRemaining  );
*/
	m_pfYPtr = l_pfYPtr;
}

AkForceInline void CAkIIRCombFilter::ProcessBufferPrev( AkReal32 * AK_RESTRICT in_pInBuffer, AkReal32 * AK_RESTRICT in_pOutBuffer, AkUInt32 in_ulNumFrames )
{
	assert( m_pfY != NULL );	
	assert( in_pInBuffer != NULL );	
	assert( in_pOutBuffer != NULL );	
	assert( in_ulNumFrames > 0 );	

	AkReal32 l_fG = m_fG;
	AkReal32 l_fNorm = m_fNorm;
	AkReal32 *l_pfYPtr = m_pfYPtr;
	AkReal32 *l_pfYEnd = m_pfYEnd;

	if ( (l_pfYPtr + in_ulNumFrames ) >= l_pfYEnd )
	{
		unsigned int i=in_ulNumFrames;
		do
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer++) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			// Advance delay line pointer and wrap around if necessary
			if ( l_pfYPtr == l_pfYEnd )
			{
				l_pfYPtr = m_pfY;
			}
			*(in_pOutBuffer++) += fYn; //sum with previous output
		} while(--i > 0);
	}
	else
	{
		unsigned int i=in_ulNumFrames;
		do
		{
			AkReal32 fYn = l_fNorm * *(in_pInBuffer++) + l_fG * *l_pfYPtr;	// Compute output
			*l_pfYPtr++ = fYn;	// Write output to delay line
			*(in_pOutBuffer++) += fYn; //sum with previous output
		} while(--i > 0);
	}
/*
	// Do not let the delay line grow out of bounds with high power signals -> clip
	AkUInt32 uSampleBlocks = (in_ulNumFrames / 4) * 4;
	ClipSampleGroups( m_pfYPtr, uSampleBlocks );
	AkUInt32 uSamplesRemaining = in_ulNumFrames-uSampleBlocks;
	if ( uSamplesRemaining )
		ClipRemainingSamples( m_pfYPtr, uSamplesRemaining  );	
*/
	m_pfYPtr = l_pfYPtr;
}

/*
#ifdef WIN32
AkForceInline void CAkIIRCombFilter::ClipSampleGroups( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	__m128 * pvIn = (__m128 *) pSamples;
	__m128 * pvEnd = (__m128 *) ( pSamples + uNumSamples );

	__m128 vecMin = _mm_set1_ps( -1.f );
	__m128 vecMax = _mm_set1_ps( 1.f );

	for ( ; pvIn < pvEnd; ++pvIn )
	{
		*pvIn = _mm_max_ps( vecMin, _mm_min_ps( vecMax, *pvIn ) );
	}
}
#elif XBOX360
AkForceInline void CAkIIRCombFilter::ClipSampleGroups( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	// Clip samples to [-1.0, 1.0]
	__vector4 * AK_RESTRICT pvIn = (__vector4 *) pSamples;
	__vector4 * AK_RESTRICT pvEnd = (__vector4 *) ( pSamples + uNumSamples );

	__vector4 vecMin, vecMax;
	vecMin.v[0] = vecMin.v[1] = vecMin.v[2] = vecMin.v[3] = -1.0f;
	vecMax.v[0] = vecMax.v[1] = vecMax.v[2] = vecMax.v[3] = 1.0f;

	for ( ; pvIn < pvEnd; ++pvIn )
	{
		*pvIn = __vmaxfp( vecMin, __vminfp( vecMax, *pvIn ) );
	}
}
#else
AkForceInline void CAkIIRCombFilter::ClipSampleGroups( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	// Clip samples to [-1.0, 1.0]
	AkReal32 * pEnd = pSamples + uNumSamples;
	for ( ; pSamples < pEnd; pSamples+=4 )
	{
		pSamples[0] = max( -1.f, min( 1.f, pSamples[0] ) );
		pSamples[1] = max( -1.f, min( 1.f, pSamples[1] ) );
		pSamples[2] = max( -1.f, min( 1.f, pSamples[2] ) );
		pSamples[3] = max( -1.f, min( 1.f, pSamples[3] ) );
	}
}
#endif

#ifdef WIN32
AkForceInline void CAkIIRCombFilter::ClipRemainingSamples( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	// Clip samples to [-1.0, 1.0]
	AkReal32 * pEnd = pSamples + uNumSamples;
	for ( ; pSamples < pEnd; pSamples++ )
	{
		*pSamples = max( -1.f, min( 1.f, *pSamples ) );
	}
}
#elif XBOX360
AkForceInline void CAkIIRCombFilter::ClipRemainingSamples( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	// Clip samples to [-1.0, 1.0]
	AkReal32 * pEnd = pSamples + uNumSamples;
	for ( ; pSamples < pEnd; pSamples++ )
	{
		*pSamples = fpmax( -1.f, fpmin( 1.f, *pSamples ) );
	}
}
#else
AkForceInline void CAkIIRCombFilter::ClipRemainingSamples( AkReal32 * AK_RESTRICT pSamples, AkUInt32 uNumSamples )
{
	// Clip samples to [-1.0, 1.0]
	AkReal32 * pEnd = pSamples + uNumSamples;
	for ( ; pSamples < pEnd; pSamples++ )
	{
		*pSamples = max( -1.f, min( 1.f, *pSamples ) );
	}
}
#endif

#ifdef WIN32
AkForceInline AkReal32 CAkIIRCombFilter::ClipSample( AkReal32 fYn )
{
	return max( -1.f, min( 1.f, fYn ) );
}
#elif XBOX360
AkForceInline AkReal32 CAkIIRCombFilter::ClipSample( AkReal32 fYn )
{
	return fpmax( -1.f, fpmin( 1.f, fYn ) );
}
#else
AkForceInline AkReal32 CAkIIRCombFilter::ClipSample( AkReal32 fYn )
{
	return max( -1.f, min( 1.f, fYn ) );
}
#endif
*/

#endif // _AKIIRCOMBFILTER_H_