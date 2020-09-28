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
// AkIIRAllPassFilter.h
//
// AudioKinetic IIR AllPass filter class
// y[n] = g * x[n] + x[n - D] - g * y[n - D]
// Allocating a maximum amount of memory may be necessary for modulating delay length later.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKIIRALLPASSFILTER_H_
#define _AKIIRALLPASSFILTER_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AkPrivateTypes.h> //for AkForceInline
#include "assert.h"

class CAkIIRAllPassFilter
{
public:

	AK_USE_PLUGIN_ALLOCATOR();

    // Constructor/destructor.
	CAkIIRAllPassFilter();	 
	~CAkIIRAllPassFilter();

	AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_ulDelayLineLength );
	AKRESULT SetCoefficient( AkReal32 in_fFeedbackGain );
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );
	void Reset( );
	AkReal32 ProcessSample( AkReal32 in_fXn );
	void	 ProcessBuffer( AkReal32 *in_pInBuffer, AkReal32 *in_pOutBuffer, AkUInt32 in_ulNumFrames );

private:
    
	AkUInt32	m_ulDelayLineLength;	// Delay line length
	AkReal32	m_fG;					// Filter coefficient
	AkReal32 *	m_pfX;					// Input delay line (start)
	AkReal32 *	m_pfXPtr;				// Input delay line read/write pointer
	AkReal32 *	m_pfXEnd;				// Input delay line back boundary
	AkReal32 *	m_pfY;					// Output delay line (start)
	AkReal32 *	m_pfYPtr;				// Output delay line read/write pointer
	AkReal32 *	m_pfYEnd;				// Output delay line back boundary
};

AkForceInline AkReal32 CAkIIRAllPassFilter::ProcessSample( AkReal32 in_fXn )
{
	assert( m_pfX != NULL );
	assert( m_pfY != NULL );
	AkReal32 fYn = m_fG * ( in_fXn - *m_pfYPtr) + *m_pfXPtr;	// Compute output
	*m_pfXPtr++ = in_fXn;	// Write input to delay line
	*m_pfYPtr++ = fYn;		// Write output to delay line
	// Advance delay line pointers and wrap around if necessary
	if ( m_pfXPtr == m_pfXEnd )
	{
		m_pfXPtr = m_pfX;
	}
	if ( m_pfYPtr == m_pfYEnd )
	{
		m_pfYPtr = m_pfY;
	}
	return fYn;
}

AkForceInline void CAkIIRAllPassFilter::ProcessBuffer( AkReal32 *in_pInBuffer, AkReal32 *in_pOutBuffer, AkUInt32 in_ulNumFrames )
{
	assert( in_pInBuffer != NULL );	
	assert( in_pOutBuffer != NULL );	

	AkReal32 *l_pfXPtr = m_pfXPtr;
	AkReal32 *l_pfYPtr = m_pfYPtr;
	AkReal32 *l_pfXEnd = m_pfXEnd;
	AkReal32 *l_pfYEnd = m_pfYEnd;
	AkReal32 l_fG = m_fG;

	unsigned int i=in_ulNumFrames;
	while(i--)
	{
		AkReal32 l_fInSample = *(in_pInBuffer++);
		AkReal32 fYn = l_fG * ( l_fInSample - *l_pfYPtr) + *l_pfXPtr;	// Compute output
		*l_pfXPtr++ = l_fInSample;	// Write input to delay line
		*l_pfYPtr++ = fYn;		// Write output to delay line
		// Advance delay line pointers and wrap around if necessary
		if ( l_pfXPtr == l_pfXEnd )
		{
			l_pfXPtr = m_pfX;
		}
		if ( l_pfYPtr == l_pfYEnd )
		{
			l_pfYPtr = m_pfY;
		}
		*(in_pOutBuffer++) = fYn;
	}

	m_pfXPtr = l_pfXPtr;
	m_pfYPtr = l_pfYPtr;
}

#endif // _AKIIRALLPASSFILTER_H_



