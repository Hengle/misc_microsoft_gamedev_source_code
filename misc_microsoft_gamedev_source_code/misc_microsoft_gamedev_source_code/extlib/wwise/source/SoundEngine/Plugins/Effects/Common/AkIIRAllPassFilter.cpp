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
// AkIIRAllPassFilter.cpp
//
// AudioKinetic IIR AllPass filter class
// y[n] = g * x[n] + x[n - D] - g * y[n - D]
// Allocating a maximum amount of memory may be necessary for modulating delay length later.
//
// Copyright 2005 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "AkIIRAllPassFilter.h"

CAkIIRAllPassFilter::CAkIIRAllPassFilter( )
{
	m_ulDelayLineLength = 0;	// Delay line length
	m_fG = 0.f;					// Filter coefficient
	m_pfX = NULL;				// Input delay line (start)
	m_pfXPtr = NULL;			// Input delay line read/write pointer
	m_pfXEnd = NULL;			// Input delay line back boundary
	m_pfY = NULL;				// Output delay line (start)
	m_pfYPtr = NULL;			// Output delay line read/write pointer
	m_pfYEnd = NULL;			// Output delay line back boundary

	// Filter is not ready to be used untill initialized
}

CAkIIRAllPassFilter::~CAkIIRAllPassFilter(  )
{

}

AKRESULT CAkIIRAllPassFilter::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_ulDelayLineLength )
{
	assert( in_pAllocator != NULL );
	assert( in_ulDelayLineLength != 0);
	m_ulDelayLineLength = in_ulDelayLineLength;

	m_pfX = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * m_ulDelayLineLength );	
	if ( m_pfX == NULL )
		return AK_InsufficientMemory;

	m_pfXPtr = m_pfX;
	m_pfXEnd = m_pfXPtr + m_ulDelayLineLength;
	m_pfY = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, sizeof(AkReal32) * m_ulDelayLineLength );	
	if ( m_pfY == NULL )
		return AK_InsufficientMemory;

	m_pfYPtr = m_pfY;
	m_pfYEnd = m_pfY + m_ulDelayLineLength;
	Reset();
	return AK_Success;
}

AKRESULT CAkIIRAllPassFilter::SetCoefficient( AkReal32 in_fFeedbackGain )
{
	if( in_fFeedbackGain < 0.f || in_fFeedbackGain > 1.f )
	{
		assert( !" Invalid feedback gain" );
		return AK_InvalidParameter;
	}
	m_fG = in_fFeedbackGain;
	return AK_Success;
}

AKRESULT CAkIIRAllPassFilter::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	assert( in_pAllocator != NULL );
	if ( m_pfX != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfX );
		m_pfX = NULL;
	}
	if ( m_pfY != NULL )
	{
		AK_PLUGIN_FREE( in_pAllocator, m_pfY );
		m_pfY = NULL;
	}
	return AK_Success;
}

void CAkIIRAllPassFilter::Reset( )
{
	if ( m_pfX != NULL )
		memset( m_pfX, 0, sizeof(AkReal32) * m_ulDelayLineLength );
	if ( m_pfY != NULL )
		memset( m_pfY, 0, sizeof(AkReal32) * m_ulDelayLineLength );
}