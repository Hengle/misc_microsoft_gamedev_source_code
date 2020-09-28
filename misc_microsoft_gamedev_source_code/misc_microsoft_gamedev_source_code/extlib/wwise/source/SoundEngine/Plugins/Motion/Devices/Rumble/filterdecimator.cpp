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
// filterdecimator.cpp
// Implementation of a decimator to bring an audio stream to a sample
// rate compatible with DBOX (400 Hz).  But, for simplicity, we want to ensure that
// the number of samples per buffer is a multiple of 8 and a multiple of 
// 24000 (audio rate). Because of this, we will use a sample rate of 375 Hz.  
// This is 64 times slower than 24000 and 128 times slower than 48000.  This
// sample rate gives exactly 16 samples or 8 samples per audio buffer.
//
// The filter is done in 2 steps:
//
// First a Butterworth filter of order 2 with a cutoff at 6.5 Hz will shape the
// signal to avoid acceleration greater than 9.8 m/s.  This filter also
// has the property of getting rid of all frequancies above 1600 Hz.
// At 187.5 Hz, the filter has a gain of -58.4 dB.  To manage to decimate to 
// 375 samples per second, we must have a gain of -96.3 dB at 187.5 Hz.
//
// The second stage is tuned to get the extra 37.9 dB at 187.5 Hz. It is 
// a FIR with a cutoff at 100 Hz and tuned for a sample rate of 3750 Hz.  
// However, we compute an output at 375 Hz, skiping 10 input samples 
// (coming from the decimated output of the butterworth).
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "filterdecimator.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AkMixingDef.h>
#include <float.h>
#include "AkMath.h"


#if defined(WIN32) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) _mm_prefetch((char *) (__ptr), _MM_HINT_NTA ) 
#elif defined(XBOX360) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) __dcbt( 128, (const void *) __ptr );
#else
#define DOPREFETCH( __ptr )
#endif

#define CUTOFF_FREQ 6.5f

#define PI_D			(3.1415926535897932384626433832795)
#define ROOTTWO_D		(1.4142135623730950488016887242097)

void FindChannelsToProcess(AkPipelineBuffer &io_rInput, AkUInt16* out_pToProcess, AkUInt16& out_rCount)
{
	out_rCount = 0;
	switch(io_rInput.GetChannelMask())
	{
	case AK_SPEAKER_SETUP_MONO:		
		out_pToProcess[0] = 0;
		out_rCount = 1;
		break;
	case AK_SPEAKER_SETUP_STEREO:
		out_pToProcess[0] = 0;	//Left
		out_pToProcess[1] = 1;	//Right
		out_rCount = 2;
		break;
	case AK_SPEAKER_SETUP_5POINT1:
	///case AK_SPEAKER_SETUP_7POINT1:
		//Process only the channels we want to keep!
		out_pToProcess[0] = 0;	//FrontLeft
		out_pToProcess[1] = 1;	//FrontRight
		//out_pToProcess[2] = 3;	//LFE
		out_pToProcess[2] = 4;	//RearLeft
		out_pToProcess[3] = 5;	//RearRight
		out_rCount = 4;
		break;
	default: 
		{
			AkUInt16 uNumChannels = (AkUInt16)io_rInput.NumChannels();
			if (uNumChannels >= MAX_INPUT_CHANNELS)
			{
				AKASSERT(!"Channel count unsupported by DBOX"); 
			}
			else
			{
				for(AkUInt16 i = 0; i < uNumChannels; i++)
					out_pToProcess[i] = i;

				out_rCount = uNumChannels;
			}
		}
		break;
	}
}

AccelerationFilter::AccelerationFilter(AkUInt32 in_iSampleRate)
{	
	m_iAudioSampleRate = in_iSampleRate;

	// Butterworth Low-pass filter computations (pre-computed using Bilinear transform)
	// Computations are done in AkReal64 precision because the filter feedback loop is 
	// very sensitive to quantization errors near the DC.  Unfortunately we operate near DC.
	AkReal64 PiFcOSr			= PI_D * CUTOFF_FREQ  / in_iSampleRate;
	AkReal64 fTanPiFcSr			= tan(PiFcOSr);
	AkReal64 fAkInt32Val			= 1.0f / fTanPiFcSr;
	AkReal64 fRootTwoxAkInt32Val	= ROOTTWO_D * fAkInt32Val;
	AkReal64 fSqAkInt32Val			= fAkInt32Val * fAkInt32Val;

	AkReal64 dCoef0 = (1.0f / ( 1.0f + fRootTwoxAkInt32Val + fSqAkInt32Val));
	m_pfFiltCoefs[0] = dCoef0;
	m_pfFiltCoefs[1] = dCoef0 + dCoef0;
	m_pfFiltCoefs[2] = -2.0f * ( 1.0f - fSqAkInt32Val) * dCoef0;
	m_pfFiltCoefs[3] = ( 1.0f - fRootTwoxAkInt32Val + fSqAkInt32Val) * -dCoef0;

	Reset();
}

AccelerationFilter::~AccelerationFilter()
{
}

void AccelerationFilter::Execute(AkPipelineBuffer & io_rInput, AkPipelineBuffer* out_pBuffer)
{
	AkUInt16 iMaxToProcess;
	AkUInt16 aChannelsToProcess[MAX_INPUT_CHANNELS];
	FindChannelsToProcess(io_rInput, aChannelsToProcess, iMaxToProcess);

	//This parameter is optional
	if (out_pBuffer == NULL)
		out_pBuffer = &io_rInput;

	AKASSERT(out_pBuffer->GetChannelMask() == io_rInput.GetChannelMask() && 
		out_pBuffer->MaxFrames() == io_rInput.MaxFrames() );

	out_pBuffer->uValidFrames = io_rInput.uValidFrames;
	for(AkUInt32 i = 0; i < iMaxToProcess; i++)
	{
		PerformMonoInterp(&io_rInput, out_pBuffer, aChannelsToProcess[i], i);
	}
}

//Filtering with a Butterworth of order 2.
AkUInt16 AccelerationFilter::PerformMonoInterp( AkPipelineBuffer * io_pInput, AkPipelineBuffer* out_pBuffer, AkUInt32 in_iInputChannel, AkUInt32 in_iOutputChannel)
{
	/// NOTE (LX) It seems like the mono interp is in fact a one-channel interp that is applied
	/// to each channel.
	register AkReal32 * AK_RESTRICT pfBuf = io_pInput->GetChannel( in_iInputChannel );
	register AkReal32 * AK_RESTRICT pfOut = out_pBuffer->GetChannel( in_iOutputChannel );

	AkReal32 * pfMem = m_pfFiltMem[in_iOutputChannel];

	DOPREFETCH( pfBuf );

	AkReal64 xn1t = pfMem[0];	// xn1
	AkReal64 xn2t = pfMem[1];	// xn2 -> mem will be thrashed so xn2 can be used as tmp register
	AkReal64 yn1t = pfMem[2];	// yn1
	AkReal64 yn2t = pfMem[3];

	//IMPORTANT NOTE: All calculations are done in DOUBLE precision because the algorithm is
	//sensitive to numerical imprecisions near 0Hz.  Unfortunately, we operate near 0Hz.
	AkReal64 xn1 = pfMem[0];	// xn1
	AkReal64 xn2 = pfMem[1];	// xn2 -> mem will be thrashed so xn2 can be used as tmp register
	AkReal64 yn1 = pfMem[2];	// yn1
	AkReal64 yn2 = pfMem[3];	// yn2 -> mem will be thrashed so xn2 can be used as tmp register
	AkReal64 b0 = m_pfFiltCoefs[0];	// b0
	AkReal64 b1 = m_pfFiltCoefs[1];	// b1
	AkReal64 a1 = m_pfFiltCoefs[2];	// a1
	AkReal64 out;				// output accumulator register
	register AkUInt16 ulSampleFrameCount = io_pInput->uValidFrames;

	while ( ulSampleFrameCount )
	{
		DOPREFETCH( pfBuf );

		AKASSERT(_fpclass(*pfBuf) != _FPCLASS_NINF);

		// Feedforward part
		xn2 = xn2 + *pfBuf;	// xn2 = xn2 + in
		xn2 = xn2 * b0;		// xn2 = (xn2 + in) * b0 (b0 == b2) 
		out = xn1;			// will need xn1 later so copy in out
		out = out * b1;		// out = xn1 * b1
		out = out + xn2;	// out = (xn2 + in) * b2 + xn1 * b1
		xn2 = xn1;			// xn2 = xn1
		xn1 = *pfBuf;		// xn1 = in 

		// Feedback part
		yn2 = yn2 * m_pfFiltCoefs[3];	// yn2 = yn2 * a2
		out = out + yn2;		// out = (xn2 + in) * b2 + xn1 * b1 + yn2 * a2
		yn2 = yn1;				// will need yn1 later so copy in yn2		
		yn2 = yn2 * a1;			// yn2 = yn1 * a1
		out = out + yn2;		// out = (xn2 + in) * b2 + xn1 * b1 + yn2 * a2 + yn1 * a1
		yn2 = yn1;				// yn2 = yn1
		yn1 = out;				// yn1 = out

		*pfOut = (AkReal32)out;			// *pfBuf = out

		++pfBuf;
		++pfOut;
		--ulSampleFrameCount;
	}
	// save registers to memory
	pfMem[0] = (AkReal32)xn1;
	pfMem[1] = (AkReal32)xn2;
	pfMem[2] = (AkReal32)yn1;
	pfMem[3] = (AkReal32)yn2;

/*#ifdef XBOX360
	RemoveDenormal( pfMem[2] );
	RemoveDenormal( pfMem[3] );
#endif */

	return ulSampleFrameCount;
}


void AccelerationFilter::Reset()
{
	memset(m_pfFiltMem, 0, sizeof(AkReal32) * MAX_INPUT_CHANNELS*AK_LPF_NUM_FILTMEM);
}
