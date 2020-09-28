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

#include "stdafx.h" 
#include "AkLPFCommon.h"
#include "AkSettings.h"
#include "AkMath.h"
#include <math.h>

// When the filter is in interpolating mode:
// Interpolation of the filter parameter is performed every LPFPARAMUPDATEPERIOD samples
// Every interpolation of the filter parameter requires to recompute filter coefficients

// When filter is bypassed:
// Preserve filter state if possible to be ready for unbypass
// y[n] = b0 x[n] + b1 x[n-1] + b2 x[n-2] - a1 y[n-1] - a2 y[n-2]
// While bypassed ==> b0 = 1, b1,b2,a1,a2 = 0 thus (keeping n fixed in time)
// y[n-1] = 1 x[n-1] + 0 x[n-2] + 0 x[n-3] - 0 y[n-2] - 0 y[n-3]
// y[n-2] = 1 x[n-2] + 0 x[n-3] + 0 x[n-4] - 0 y[n-3] - 0 y[n-4]

// Linear or non-linear mapping function of the LPF parameter to cutoff freq 
#define MAPPINGMODENONLINEAR

// Mapping function LPF -> CutFreq defines
#define SRCLOPASSMINFREQ 50.f		// Total frequency range
#define SRCLOPASSMAXFREQ 20000.f
#define LOWESTPARAMVAL 0.f			// LPF Parameter in [0.f,100.f] range
#define HIGHESTPARAMVAL 100.f	

// Non-linear mapping
#define BETAPARAM 1.09f							// Exponential base
#define PARAMTRANSPOINT 30.f					// Linear->Exp transition point (param)
#define RANGETRANSPOINT 7000.f					// Linear->Exp transition point (cutoff freq)

// Precomputed values
// RANGETRANSPOINT /  BETAPARAM^(HIGHESTPARAMVAL-PARAMTRANSPOINT)
#define NONLINSCALE 16.79744331443938f

// (SRCLOPASSMAXFREQ - RANGETRANSPOINT)
#define LINRANGEEND ( SRCLOPASSMAXFREQ - RANGETRANSPOINT )

#define FEEDBACKMINFREQ (1.f)		
#define FEEDBACKMAXFREQ (AK_FEEDBACK_SAMPLE_RATE * 0.5f)	// Max valid feedback frequency value

// Fill array of 4 LPF coefficients following Butterworth formula's (only if the filter return value is false), 

bool EvalLPFBypass( AkReal32 in_fCurrentLPFPar, bool in_bIsForFeedbackPipeline, AkReal32 & out_fCutFreq )
{
	bool bBypassFilter = false;

	// Non-linear mapping function with extended frequency range
	if ( !in_bIsForFeedbackPipeline )
	{
		if ( in_fCurrentLPFPar <= BYPASSMAXVAL )
		{
			// Bypass filter execution
			bBypassFilter = true;
		}
		else 
		{
			if ( in_fCurrentLPFPar < PARAMTRANSPOINT )
			{
				// Linear mapping function
				out_fCutFreq = ((LINRANGEEND/PARAMTRANSPOINT)*(PARAMTRANSPOINT-in_fCurrentLPFPar) + RANGETRANSPOINT);
			}
			else
			{
				// Exponential mapping function
				out_fCutFreq = NONLINSCALE * powf( BETAPARAM, HIGHESTPARAMVAL-in_fCurrentLPFPar );
			}

			// No point running the filter at cutoff freq above Nyquist
			if ( out_fCutFreq >= ( AK_CORE_SAMPLERATE * (0.5f * 0.9f) ) )
			{	
				bBypassFilter = true;
			}
		}
	}
	else
	{
		if ( in_fCurrentLPFPar <= BYPASSMAXVAL )
		{
			// Bypass filter execution
			bBypassFilter = true;
		}
		else
		{
			// Linear mapping function
			out_fCutFreq = FEEDBACKMAXFREQ - in_fCurrentLPFPar * ((FEEDBACKMAXFREQ - FEEDBACKMINFREQ) / HIGHESTPARAMVAL);

			// No point running the filter at cutoff freq above Nyquist
			if ( out_fCutFreq >= ( FEEDBACKMAXFREQ * 0.99f ) )
			{	
				bBypassFilter = true;
			}
		}
	}

	return bBypassFilter;
}

void ComputeLPFCoefs( AkReal32 in_fCutFreq, AkReal32 * out_pfFiltCoefs )
{
	// Butterworth Low-pass filter computations (pre-computed using Bilinear transform)
	AkReal32 PiFcOSr			= PI * in_fCutFreq  / AK_CORE_SAMPLERATE;
	AkReal32 fTanPiFcSr			= tan(PiFcOSr);
	AkReal32 fIntVal			= 1.0f / fTanPiFcSr;
	AkReal32 fRootTwoxIntVal	= ROOTTWO * fIntVal;
	AkReal32 fSqIntVal			= fIntVal * fIntVal;

	out_pfFiltCoefs[0] = (1.0f / ( 1.0f + fRootTwoxIntVal + fSqIntVal));
	out_pfFiltCoefs[1] = out_pfFiltCoefs[0] + out_pfFiltCoefs[0];
	out_pfFiltCoefs[2] = -2.0f * ( 1.0f - fSqIntVal) * out_pfFiltCoefs[0];
	out_pfFiltCoefs[3] = ( 1.0f - fRootTwoxIntVal + fSqIntVal) * -out_pfFiltCoefs[0];
}

// Butterworth filter DSP 1 channel with interpolation
void Perform1ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( 0 );
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;
	
	LPF_INTERPOLATION_SETUP(); 

	AkUInt32 uFramesLeft = uNumFrames;
	while ( uFramesLeft )
	{
		AkUInt32 uFramesInBlock = AkMin( LPFPARAMUPDATEPERIOD, uFramesLeft );

		LPF_INTERPOLATE_COEFFICIENTS();

		if ( !io_pLPFState->bBypassFilter )
			LPF_APPLY_CHANNEL( &io_pFiltMem[0], pfCoefs, pfBuf, uFramesInBlock );
		else
		{
			LPF_BYPASS_CHANNEL( &io_pFiltMem[0], pfBuf, uFramesInBlock );
		}

		uFramesLeft -= uFramesInBlock;
	}

	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[0] );
}

// Butterworth filter DSP 2 channels with interpolation
void Perform2ChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * AK_RESTRICT pfBufL = io_pBuffer->GetChannel( 0 );
	AkReal32 * AK_RESTRICT pfBufR = io_pBuffer->GetChannel( 1 );
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;

	LPF_INTERPOLATION_SETUP(); 

	AkUInt32 uFramesLeft = uNumFrames;
	while ( uFramesLeft )
	{
		AkUInt32 uFramesInBlock = AkMin( LPFPARAMUPDATEPERIOD, uFramesLeft );

		LPF_INTERPOLATE_COEFFICIENTS();

		if ( !io_pLPFState->bBypassFilter )
		{
			LPF_APPLY_CHANNEL( &io_pFiltMem[0], pfCoefs, pfBufL, uFramesInBlock );
			LPF_APPLY_CHANNEL( &io_pFiltMem[AKLPFNUMCOEFICIENTS], pfCoefs, pfBufR, uFramesInBlock );
		}
		else
		{
			LPF_BYPASS_CHANNEL( &io_pFiltMem[0], pfBufL, uFramesInBlock );
			LPF_BYPASS_CHANNEL( &io_pFiltMem[AKLPFNUMCOEFICIENTS], pfBufR, uFramesInBlock );
		}

		uFramesLeft -= uFramesInBlock;
	}

	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[0] );
	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[AKLPFNUMCOEFICIENTS] );
}

// Butterworth filter DSP 1 channel no interpolation
void Perform1Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;
	AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( 0 );

	if ( !io_pLPFState->bBypassFilter )
		LPF_APPLY_CHANNEL( &io_pFiltMem[0], pfCoefs, pfBuf, uNumFrames );
	else
		LPF_BYPASS_CHANNEL( &io_pFiltMem[0], pfBuf, uNumFrames );

	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[0] );
}

// Butterworth filter DSP 2 channels no interpolation
void Perform2Chan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkReal32 * AK_RESTRICT pfBufL = io_pBuffer->GetChannel( 0 );
	AkReal32 * AK_RESTRICT pfBufR = io_pBuffer->GetChannel( 1 );
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;

	if ( !io_pLPFState->bBypassFilter )
	{
		LPF_APPLY_CHANNEL( &io_pFiltMem[0], pfCoefs, pfBufL, uNumFrames );
		LPF_APPLY_CHANNEL( &io_pFiltMem[AKLPFNUMCOEFICIENTS], pfCoefs, pfBufR, uNumFrames );
	}
	else
	{
		LPF_BYPASS_CHANNEL( &io_pFiltMem[0], pfBufL, uNumFrames );
		LPF_BYPASS_CHANNEL( &io_pFiltMem[AKLPFNUMCOEFICIENTS], pfBufR, uNumFrames );
	}

	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[0] );
	LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[AKLPFNUMCOEFICIENTS] );
}

// Butterworth filter DSP N channels with interpolation
void PerformNChanInterp( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkUInt32 uNumChannels = AK::GetNumChannels( io_pLPFState->uChannelMask );
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;

	LPF_INTERPOLATION_SETUP(); 

	AkUInt32 uFramesProduced = 0;
	while ( uFramesProduced < uNumFrames )
	{
		AkUInt32 uFramesInBlock = AkMin( LPFPARAMUPDATEPERIOD, uNumFrames-uFramesProduced );

		LPF_INTERPOLATE_COEFFICIENTS();

		for ( AkUInt32 i = 0; i < uNumChannels; ++i )
		{
			AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( i ) + uFramesProduced;
			
			if ( !io_pLPFState->bBypassFilter )
				LPF_APPLY_CHANNEL( &io_pFiltMem[i*AKLPFNUMCOEFICIENTS], pfCoefs, pfBuf, uFramesInBlock );
			else
				LPF_BYPASS_CHANNEL( &io_pFiltMem[i*AKLPFNUMCOEFICIENTS], pfBuf, uFramesInBlock );
		}

		uFramesProduced += uFramesInBlock;
	}

	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
		LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[i*AKLPFNUMCOEFICIENTS] );

}

// Butterworth filter DSP N channel no interpolation
void PerformNChan( AkAudioBuffer * io_pBuffer, AkInternalLPFState * io_pLPFState, AkReal32 * io_pFiltMem )
{
	AkUInt32 uNumFrames = io_pBuffer->uValidFrames;
	AkUInt32 uNumChannels = AK::GetNumChannels( io_pLPFState->uChannelMask );
	AkReal32 * pfCoefs = io_pLPFState->fFiltCoefs;

	for ( AkUInt32 i = 0; i < uNumChannels; ++i )
	{
		AkUInt32 uIndexOffset = i*AKLPFNUMCOEFICIENTS;
		AkReal32 * AK_RESTRICT pfBuf = io_pBuffer->GetChannel( i );

		if ( !io_pLPFState->bBypassFilter )
			LPF_APPLY_CHANNEL( &io_pFiltMem[uIndexOffset], pfCoefs, pfBuf, uNumFrames );
		else
			LPF_BYPASS_CHANNEL( &io_pFiltMem[uIndexOffset], pfBuf, uNumFrames );

		LPF_CHANNEL_DENORMAL_REMOVE( &io_pFiltMem[uIndexOffset] );
	}
}


