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
// AkIIRTwoPoleLPFilter.h
//
// AudioKinetic DC normalized 2nd order IIR low pass filter class
// y[n] = (1-g)^2 * x[n] + 2*g * y[n - 1] + g^2 y[n - 2]
//
// Copyright 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKIIRTWOPOLEFILTER_H_
#define _AKIIRTWOPOLEFILTER_H_

#include "AkDSPUtils.h" // RemoveDenormal
#include <assert.h>

class CAkTwoPoleLPFilter
{
public:
	CAkTwoPoleLPFilter()
	{
		m_fFwdCoef = 0.f;
		m_fFbkCoef1 = 0.f;
		m_fFbkCoef2 = 0.f;
		m_fLPMem1 = 0.f;
		m_fLPMem2 = 0.f;
	}
	~CAkTwoPoleLPFilter() { }
	AkForceInline void ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess );
	inline void SetCutoffFrequency( AkReal32 in_fCutFreq );
	inline void SetSampleRate( AkUInt32 in_uSampleRate );
	inline void Reset( );
	inline void AvoidDenormal();

private:
	AkUInt32 m_uSampleRate;
	AkReal32 m_fLPMem1;				// first order feedback memory
	AkReal32 m_fLPMem2;				// second order feedback memory
	AkReal32 m_fFwdCoef;			// Feedforward coefficient for LP filter
	AkReal32 m_fFbkCoef1;			// First order feedback coefficient for LP filter
	AkReal32 m_fFbkCoef2;			// 2nd order feedback coefficient for LP filter
};

AkForceInline void CAkTwoPoleLPFilter::ProcessBuffer( AkReal32 * AK_RESTRICT io_pfInOut, AkUInt32 in_uFramesToProcess )
{
	// Filter memory
	AkReal32 fLPyn1 = m_fLPMem1;
	AkReal32 fLPyn2 = m_fLPMem2;

	// Coefs
	AkReal32 fLPb0 = m_fFwdCoef;
	AkReal32 fLPa1 = m_fFbkCoef1;
	AkReal32 fLPa2 = m_fFbkCoef2;

	const AkReal32 * AK_RESTRICT const fEnd = io_pfInOut + in_uFramesToProcess;
	while ( io_pfInOut < fEnd )
	{
		AkReal32 fXIn = *io_pfInOut;		
		AkReal32 fFbk1 = fLPyn1 * fLPa1;
		AkReal32 fFbk2 = fLPyn2 * fLPa2;
		AkReal32 fOutLPF = fXIn * fLPb0;
		fOutLPF = fOutLPF + fFbk1 + fFbk2;	
		*io_pfInOut++ = fOutLPF;	
		fLPyn1 = fOutLPF; 
		fLPyn2 = fLPyn1;  
	}

	// Save filter memories
	m_fLPMem1 = fLPyn1;	
	m_fLPMem2 = fLPyn2;	
}

inline void CAkTwoPoleLPFilter::SetCutoffFrequency( AkReal32 in_fCutFreq )
{
	// Effectively bypass filter if cutoff frequency is >= Nyquist frequency
	if ( in_fCutFreq < (m_uSampleRate / 2.f) )
	{
		// Coefficient approximation
		AkReal32 fTmp = (1.f - 2.f * in_fCutFreq / m_uSampleRate);
		AkReal32 fG = fTmp * fTmp;
		AkReal32 fOneMinusG = (1.f - fG);
		m_fFwdCoef = fOneMinusG * fOneMinusG;
		m_fFbkCoef1 = 2.f * fG;
		m_fFbkCoef2 = -1.f * (fG * fG);
		RemoveDenormal( m_fFwdCoef );
		RemoveDenormal( m_fFbkCoef1 );
		RemoveDenormal( m_fFbkCoef2 );
	}
	else
	{
		// Passthrough filter
		m_fFwdCoef = 1.f;
		m_fFbkCoef1 = 0.f;
		m_fFbkCoef2 = 0.f;
	}
}

inline void CAkTwoPoleLPFilter::SetSampleRate( AkUInt32 in_uSampleRate )
{
	m_uSampleRate = in_uSampleRate;
}

inline void CAkTwoPoleLPFilter::Reset( )
{
	m_fLPMem1 = 0.f;
	m_fLPMem2 = 0.f;
}

inline void CAkTwoPoleLPFilter::AvoidDenormal()
{
#ifdef XBOX360
	RemoveDenormal( m_fLPMem1 );
	RemoveDenormal( m_fLPMem2 );
#endif
}

#endif // _AKIIRTWOPOLEFILTER_H_