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
// SinkBase.h
// Implementation of the sink.  Common methods and data for the XP and Vista Sinks
//////////////////////////////////////////////////////////////////////

#pragma once
#include "IAkMotionMixBus.h"
#include "limiter.h"
#include <ks.h>
#include <ksmedia.h>

class CSinkBase
{
	AK_USE_PLUGIN_ALLOCATOR();
public:
	CSinkBase();
	virtual ~CSinkBase();

	virtual AKRESULT Init(AkUInt32 in_iBufferFrames);
	virtual void Term(AK::IAkPluginMemAlloc * in_pAllocator);

	virtual AKRESULT PassData(AkPipelineBuffer& io_rXfer) = 0;
	virtual AKRESULT PassSilence( AkUInt16 in_uNumFrames ) = 0;
	virtual AKRESULT IsDataNeeded( AkUInt16 & out_uBuffersNeeded ) = 0;
	virtual void Stop() = 0;

	//Status functions
	bool	 IsStarved();
	void	 ResetStarved();
	AkReal32 GetPeak();
	
protected:
	AkInt16 ManageDrift(AkUInt32 in_iFreeFrames);

	template <class TRAITS>
	AkUInt16 Fill( AkReal32 *in_pSrc,				//Beginning of source buffer
		AkUInt16 in_nSamples,						//Number of destination samples to fill
		typename TRAITS::OutputType* in_pDst,		//Beginning of destination buffer
		AkUInt32 &io_uOffset);						//Offset in the output buffer.

	template <class TRAITS>
	bool FillFadeOut(typename TRAITS::OutputType* in_pDst, AkInt16 in_nSamples, AkInt16 &out_nNextBufferNeedExtra);

	template <class TRAITS>
	AkUInt16 FillSilence(typename TRAITS::OutputType* in_pDst, AkUInt16 in_nSamples, AkUInt32 &io_uOffset);

	CLimiter				m_oLimiter;

	__m128					m_Output1;					//Output -1 (last output)
	__m128					m_Output2;					//Output -2 (second previous output)

#ifndef AK_OPTIMIZED
	__m128					m_mPeaks;					//Peak of the signal
#endif

	//Clock drift correction stuff.
	AkReal32				m_fBufferTime;				//The time taken to play one buffer
	AkReal32				m_fAverageFreeFrames;		//The number of free samples in the buffer averaged over a second.
	AkReal32				m_fTargetFreeFrames;		//The target value for the average.

	AkUInt32				m_uBufferMask;

	//When up sampling from 375 Hz to 8000 Hz, 1 sample out of 3 needs
	//to have an extra output sample to keep everything in sync.  
	//The up sampling ratio is 21.33333333.  Therefore, 2 samples can be up sampled 
	//with a factor of 21 and a third with a factor of 22.  Note that this
	//will induce an undesired artifact at 125 Hz (375 / 3) at -90 dB.  Fortunately, this 
	//frequency is not perceived in the chair.

	AkUInt16				m_iMissing;	
	AkUInt16				m_iExtraState;
	static const AkUInt16	g_SampleNextState[3];	//Next state.
	static __m128			g_InvMultiplier[3];		//Multipliers for each state {21, 21, 22}.  Computed in the constructor.

	bool					m_bStarved;	
};

//Debugging stuff.  Uncomment ONE of the two following lines to output either the input signal 
//or the output signal to c:\output.txt.  Check it with Excel to make a graph.
//#define DEBUG_OUTPUT
//#define DEBUG_INPUT

#if defined DEBUG_OUTPUT || defined DEBUG_INPUT
#include <stdio.h>
extern FILE *g_fOutput;
extern __m128 g_aBufferOutput[DBOX_OUTPUT_RATE*20];	//10 seconds of buffer
extern AkInt32 g_nOutput;

#define DEBUG_FRAME(__Samples) \
	if (g_nOutput < DBOX_OUTPUT_RATE *20) \
{ \
	g_aBufferOutput[g_nOutput] = __Samples; \
	g_nOutput++; \
} \

#else
#define DEBUG_FRAME(__Samples)
#endif

#ifdef DEBUG_OUTPUT
#define DEBUG_OUTPUT_FRAME(__Samples) DEBUG_FRAME(__Samples)
#else
#define DEBUG_OUTPUT_FRAME(__Samples)
#endif

#ifdef DEBUG_INPUT
#define DEBUG_INPUT_FRAME(__Samples) DEBUG_FRAME(__Samples)
#else
#define DEBUG_INPUT_FRAME(__Samples)
#endif

#include "SinkBase.inl"