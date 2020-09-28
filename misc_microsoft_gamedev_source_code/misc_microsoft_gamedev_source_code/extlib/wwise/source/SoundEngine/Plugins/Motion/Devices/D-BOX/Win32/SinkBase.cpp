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
// SinkBase.cpp
// Implementation of the sink.  Common methods and data for the XP and Vista Sinks
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SinkBase.h"

//When upsampling from 375 hz to 8000 hz, 1 sample out of 3 needs
//to have an extra output sample to keep everything in sync.  
//The upsampling ratio is 21.33333333.

const AkUInt16 CSinkBase::g_SampleNextState[3] = {1, 2, 0};	//Next state.
__m128 CSinkBase::g_InvMultiplier[3];	//Multipliers for each state {21, 21, 22}.  Computed in the constructor.

//Debugging stuff
#if defined DEBUG_OUTPUT || defined DEBUG_INPUT
FILE *g_fOutput = NULL;
__m128 g_aBufferOutput[DBOX_OUTPUT_RATE*20];	//10 seconds of buffer
AkInt32 g_nOutput = 0;
#endif

CSinkBase::CSinkBase()
: m_fBufferTime(0)		
, m_fAverageFreeFrames(0)
, m_fTargetFreeFrames(0)
, m_bStarved(false)	
, m_uBufferMask(0)
, m_iExtraState(0)
{ 
	m_Output1 = _mm_set_ps1(0);
	m_Output2 = _mm_set_ps1(0);
#ifndef AK_OPTIMIZED
	m_mPeaks = _mm_set_ps1(0);
#endif
}

CSinkBase::~CSinkBase()
{
#if defined DEBUG_OUTPUT || defined DEBUG_INPUT
	g_fOutput = fopen("c:\\output.txt", "wt");
	for(AkInt32 i = 1; i < g_nOutput; i++)
		fprintf(g_fOutput, "%f\t%f\n", g_aBufferOutput[i].m128_f32[0], g_aBufferOutput[i].m128_f32[0] - g_aBufferOutput[i-1].m128_f32[0]);
	fclose(g_fOutput);
#endif
}

AKRESULT CSinkBase::Init(AkUInt32 in_iBufferFrames)
{
	m_oLimiter.Init(DBOX_SAMPLERATE);

	//We want to keep the buffer half full.
	m_fTargetFreeFrames = in_iBufferFrames / 2.f;
	m_fAverageFreeFrames = m_fTargetFreeFrames;
	m_fBufferTime = (AkReal32)DBOX_NUM_REFILL_FRAMES / (AkReal32)DBOX_SAMPLERATE;	

	m_uBufferMask = in_iBufferFrames - 1;

	g_InvMultiplier[0] = _mm_set_ps1(1.0f/floor(SAMPLE_MULTIPLIER));
	g_InvMultiplier[1] = _mm_set_ps1(1.0f/floor(SAMPLE_MULTIPLIER));
	g_InvMultiplier[2] = _mm_set_ps1(1.0f/ceil(SAMPLE_MULTIPLIER));

	return AK_Success;
}

void CSinkBase::Term(AK::IAkPluginMemAlloc * in_pAllocator)
{
	m_fBufferTime = 0;
	m_fAverageFreeFrames = 0;
	m_fTargetFreeFrames = 0;
	m_bStarved = false;
	m_uBufferMask = 0;
}
	
bool CSinkBase::IsStarved()
{
	return m_bStarved;
}

void CSinkBase::ResetStarved()
{
	m_bStarved = false;
}

AkInt16 CSinkBase::ManageDrift(AkUInt32 in_iFreeFrames)
{
	AkInt16 iCorrection = 0;
	//Manage clock drift between the audio clock and this device's clock.
	//Accept a variation of 16 on the average (completely arbitrary, based on empiric data)

	//Remove one contribution to the average.  We average over 1.0 second.  
	m_fAverageFreeFrames *= (1.0f - m_fBufferTime);
	m_fAverageFreeFrames += in_iFreeFrames * m_fBufferTime;

	AkReal32 fDiff = m_fTargetFreeFrames - m_fAverageFreeFrames;
	if(fDiff < -16)
	{
		//There are more frames consumed than we generate, therefore sample rate is slightly faster than desired.
		//Lengthen the next buffer by one frame.  To correct this, we will force the 
		//the resampling sequence directly to 22, regardless of the current state.
		//This means that we have 2 chances out of 3 of actually adding a frame.
		//If the sequence was already on 22, we didn't correct the drift.
		//For example, 3 consecutive cycles would look like this:
		// 21, 21, 22, 21, 22, 21, 21, 22
		//				    ^ Correction here, we skipped one state in the cycle.
		if (m_iExtraState != 2)
		{
			m_iExtraState = 2;
			iCorrection = 1;

			//Reset the average under the threshold
			m_fAverageFreeFrames -= 1;
		}
	}
	else if (fDiff > 16)
	{
		//We generate more frames than the device consumes, therefore the sample rate is slightly slower.
		//Shorten the next buffer by one frame, if we can. To correct this, we will force the 
		//the resampling sequence directly to the start of the cycle, regardless of the 
		//current state.  This means that we have 2 chances out of 3 of actually lengthening
		//the cycle, therefore skipping a "22" when there should be one.
		//For example, 3 consecutive cycles would look like this:
		//21, 21, 22, 21, 21, 21, 21, 22
		//				   ^ Correction here, we skipped one state in the cycle.
		if (m_iExtraState != 0)
		{
			m_iExtraState = 0;
			iCorrection = -1;

			//Reset the average under the threshold
			m_fAverageFreeFrames += 1;
		}
	}

	return iCorrection;
}

AkReal32 CSinkBase::GetPeak()
{
#ifndef AK_OPTIMIZED
	float fMaxAB = max(m_mPeaks.m128_f32[0], m_mPeaks.m128_f32[1]);
	float fMaxCD = max(m_mPeaks.m128_f32[2], m_mPeaks.m128_f32[3]);
	m_mPeaks = _mm_xor_ps(m_mPeaks, m_mPeaks);	//Reset to zero
	return max(fMaxAB, fMaxCD);
#else
	return 0.0f;
#endif
}
