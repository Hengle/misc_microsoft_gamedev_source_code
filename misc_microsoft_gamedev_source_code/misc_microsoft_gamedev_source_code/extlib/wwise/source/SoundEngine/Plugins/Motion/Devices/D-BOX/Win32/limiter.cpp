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
// Converter.cpp
// Implementation of the safety Converter for the DBOX platform.
// This signal processor will ensure that the incoming signal doesn't
// cause speeds and accelerations higher than the designed limits of
// the device.
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "limiter.h"

CLimiter::CLimiter()
{	
	//Reset everything to 0.
	memset(this, 0, sizeof(CLimiter));

	SetLimits(MAX_ACCELERATION, MAX_SPEED);
}

CLimiter::~CLimiter()
{
}

/****************************************************************
* Init
* Initialize various constants related to the sample rate.
* Params: 
* AkUInt32 in_iSampleRate: The sample rate
****************************************************************/

void CLimiter::Init(AkUInt32 in_iSampleRate)
{
	m_fSampleRate = (AkReal32)in_iSampleRate;
	m_fTimeSample = 1.0f/m_fSampleRate;
	m_fSampleSquaredHalved = m_fTimeSample * m_fTimeSample * 0.5f;
	m_fSampleRateSquared = m_fSampleRate * m_fSampleRate;
}

/****************************************************************
* SetLimits
* Sets and compute the normalized limits of the device
* Params: 
* AkReal32 in_fAccel: The sample rate
* AkReal32 in_fSpeed
****************************************************************/

void CLimiter::SetLimits(AkReal32 in_fAccel, AkReal32 in_fSpeed)
{
	m_fMaxAcceleration = NORM_DISPLACEMENT*in_fAccel/MAX_DISPLACEMENT;
	m_fMaxSpeed = NORM_DISPLACEMENT*in_fSpeed/MAX_DISPLACEMENT;
}

/****************************************************************
* Limit
* Limit the samples.
* Params: 
* AkReal32 *&in_pSrc: Source sample, interleaved
* AkInt32 in_nSamples: 
* const __m128& in_mOutput1: Previous outputed samples	(at t-1)
* const __m128& in_mOutput2: Second previous samples	(at t-2)
******************************************************************/

void CLimiter::Limit(AkReal32 *in_pSrc, AkInt32 in_nSamples, const __m128& in_mOutput1, const __m128& in_mOutput2)
{	
	__m128 * AK_RESTRICT pData = (__m128*)in_pSrc;

	__m128	mOutput1 = in_mOutput1;			//Output -1 (last output)
	__m128	mOutput2 = in_mOutput2;			//Output -2 (second previous output)

	while(in_nSamples != 0)
	{
		//Limit the accelerations and speed of the signal.  	
		//This loop can be avoided when LimitStep is transformed in SSE.  All 4 channels could be processed at the same time.

		for(AkInt32 iChannel = 0; iChannel < DBOX_CHANNELS; iChannel++)
		{
			pData->m128_f32[iChannel] = LimitStep(pData->m128_f32[iChannel], 
				m_Input1.m128_f32[iChannel], 
				mOutput1.m128_f32[iChannel], 
				mOutput2.m128_f32[iChannel], 
				m_AccumulatedError.m128_f32[iChannel]);
		}

		mOutput2 = mOutput1;
		mOutput1 = *pData;

		//Keep the last input
		m_Input1 = *pData;

		in_nSamples--;
		pData++;
	}
}

//////////////////////////////////////////////////////////////////////
// LimitStep
// Limits the signal speed and accelerations within the physical limits.
// It is done by simplifying the problem by assuming that between 2 samples
// the movement is linear.  Then we can use the traditional physical 
// model for accelerated linear movement:
// X = X0 + V0*t + 0.5*A*tsquared
// V = V0 + A*t
//////////////////////////////////////////////////////////////////////

//TODO OPTIMIZE!  We have 4 channels to process.  By coincidence, SSE operations 
//work on 4 AkReal32s at the same time!  Use that!

AkReal32 CLimiter::LimitStep(AkReal32 &X, const AkReal32& X1, const AkReal32 &Y1, const AkReal32 &Y2, AkReal32& io_fError)
{
	//Limit the signal in the range.
	if (fabs(X) > NORM_DISPLACEMENT/2.0f)
		X = (AkReal32)_copysign(NORM_DISPLACEMENT/2.0f, X);

	//Current speed of the signal.  (Current_sample - Previous_sample) / time
	AkReal32 fTargetSpeed = (X - X1) * m_fSampleRate;
	if (fabs(fTargetSpeed) > m_fMaxSpeed)
		fTargetSpeed = (AkReal32)_copysign(m_fMaxSpeed, fTargetSpeed);

	//Speed of the output at the last sample.
	AkReal32 fOutputSpeed = (Y1 - Y2) * m_fSampleRate;

	//Compute the acceleration needed to get to the target speed.
	AkReal32 fAccelBySpeed = (fTargetSpeed - fOutputSpeed) * m_fSampleRate;	
	if (fabs(fAccelBySpeed) > m_fMaxAcceleration)
		fAccelBySpeed = (AkReal32)_copysign(m_fMaxAcceleration, fAccelBySpeed) ;

	//Compute the acceleration needed to get to the target position.
	//Based on the Uniformly Accelerated Movement formula: X = X0 + V0*T 0.5*A*Tsquared
	AkReal32 fAccelByPos = 2 * (X - Y1 - fOutputSpeed * m_fTimeSample) * m_fSampleRateSquared;	
	if (fabs(fAccelByPos) > m_fMaxAcceleration)
		fAccelByPos = (AkReal32)_copysign(m_fMaxAcceleration, fAccelByPos) ;

	//Compute the weight of both accelerations.  Do we prefer following position or speed?
	//When the error between the signal and output is getting bigger, favor the position.
	//This will have the effect of bringing back the output signal average to the same
	//as the input signal, on a relatively short duration.

	io_fError += (X1 - Y1) * m_fTimeSample;	//integrated error (area under the curve)
	AkReal32 fImportance = fabsf(io_fError);

	AkReal32 fTargetAcc = fAccelBySpeed *(1-fImportance) + fAccelByPos*fImportance;	

	//Based on the Uniformly Accelerated Movement formula: X = X = x0 + V0*T 0.5*A*Tsquared
	float ret = Y1 + fOutputSpeed * m_fTimeSample + fTargetAcc * m_fSampleSquaredHalved;
	//Limit the signal in the range.
	if (fabs(ret) > NORM_DISPLACEMENT/2.0f)
		ret = (AkReal32)_copysign(NORM_DISPLACEMENT/2.0f, ret);

	return ret;
}


