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
#include "DBoxConst.h"
#include "filterdecimator.h"
#include <AK/SoundEngine/Common/AkTypes.h>
#include <float.h>


#if defined(WIN32) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) _mm_prefetch((char *) (__ptr), _MM_HINT_NTA ) 
#elif defined(XBOX360) && defined(USEPREFETCH)
#define DOPREFETCH( __ptr ) __dcbt( 128, (const void *) __ptr );
#else
#define DOPREFETCH( __ptr )
#endif

#define CUTOFF_FREQ 6.5f

//The algorithm and filters are optimized for the following values:
#define INTERMEDIATE_RATE 3750

//FIR Filter designed with http://cnx.org/content/m13115/latest/
//Parameters: 
//Freq: 3750 Hz
//Ripple: 1 dB
//Passband: 100 Hz
//Stopband: 187.5 Hz
//Attenuation: 37.9 dB	
#define NUM_COEFS 55
const AkReal32 c_fFIRCoefs[NUM_COEFS] =
	{
		-0.0081621864f, -0.0053164585f, -0.0067450050f, -0.0081802685f, -0.0095060373f,
		-0.0106377537f, -0.0114501970f,	-0.0118178376f,	-0.0116402761f,	-0.0108120869f,
		-0.0092387263f, -0.0068612716f, -0.0036482902f,  0.0004017624f,  0.0052523113f,
		 0.0108317788f,  0.0170295215f,  0.0236950100f,  0.0306507131f,  0.0376989841f,
		 0.0446250380f,  0.0512070133f,  0.0572248826f,  0.0624698483f,  0.0667579730f,
		 0.0699377046f,  0.0718934891f,  0.0725533838f,  0.0718934891f,  0.0699377046f,
		 0.0667579730f,  0.0624698483f,  0.0572248826f,  0.0512070133f,  0.0446250380f,
		 0.0376989841f,  0.0306507131f,  0.0236950100f,  0.0170295215f,  0.0108317788f,
		 0.0052523113f,  0.0004017624f, -0.0036482902f, -0.0068612716f, -0.0092387263f,
		-0.0108120869f, -0.0116402761f, -0.0118178376f, -0.0114501970f, -0.0106377537f,
		-0.0095060373f, -0.0081802685f, -0.0067450050f, -0.0053164585f, -0.0081621864f
	};


#define PI_D			(3.1415926535897932384626433832795)
#define ROOTTWO_D		(1.4142135623730950488016887242097)

void FindChannelsToProcess(AkAudioBuffer &io_rInput, AkUInt16* out_pToProcess, AkUInt16& out_rCount)
{
	out_rCount = 0;
	AkChannelMask uChannelMask = io_rInput.GetChannelMask();
	switch(uChannelMask)
	{
	case AK_SPEAKER_SETUP_MONO:		
		out_pToProcess[0] = 0;
		out_rCount = 1;
		break;
	case AK_SPEAKER_SETUP_STEREO:
		out_pToProcess[0] = AK_IDX_SETUP_2_LEFT;	//Left
		out_pToProcess[1] = AK_IDX_SETUP_2_RIGHT;	//Right
		out_rCount = 2;
		break;
	case AK_SPEAKER_SETUP_5POINT1:
#ifdef AK_71AUDIO
	case AK_SPEAKER_SETUP_7POINT1:
#endif
		//Process only the channels we want to keep!
		out_pToProcess[0] = AK_IDX_SETUP_5_FRONTLEFT;	//FrontLeft
		out_pToProcess[1] = AK_IDX_SETUP_5_FRONTRIGHT;	//FrontRight
		//out_pToProcess[2] = 3;	//LFE
		out_pToProcess[2] = AK_IDX_SETUP_5_REARLEFT;	//RearLeft
		out_pToProcess[3] = AK_IDX_SETUP_5_REARRIGHT;	//RearRight
		out_rCount = 4;
		break;
	default: 
		{
			AkUInt32 uNumChannels = AK::GetNumChannels( uChannelMask );
			if (uNumChannels >= MAX_INPUT_CHANNELS)
			{
				AKASSERT(!"Channel count unsupported by D-BOX™"); 
			}
			else
			{
				for(AkUInt16 i = 0; i < uNumChannels; i++)
					out_pToProcess[i] = i;
	
				out_rCount = (AkUInt16)uNumChannels;
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

void AccelerationFilter::Execute(AkAudioBuffer & io_rInput, AkAudioBuffer* out_pBuffer)
{
	AkUInt16 iMaxToProcess;
	AkUInt16 aChannelsToProcess[MAX_INPUT_CHANNELS];
	FindChannelsToProcess(io_rInput, aChannelsToProcess, iMaxToProcess);

	//This parameter is optional
	if (out_pBuffer == NULL)
		out_pBuffer = &io_rInput;

	AKASSERT(out_pBuffer->NumChannels() == io_rInput.NumChannels() && 
		out_pBuffer->MaxFrames() == io_rInput.MaxFrames());

	out_pBuffer->uValidFrames = io_rInput.uValidFrames;
	for(AkUInt32 i = 0; i < iMaxToProcess; i++)
	{
		PerformMonoInterp(&io_rInput, out_pBuffer, aChannelsToProcess[i], i);
	}
}

//Filtering with a Butterworth of order 2.
AkUInt16 AccelerationFilter::PerformMonoInterp( AkAudioBuffer * io_pInput, AkAudioBuffer* out_pBuffer, AkUInt32 in_iInputChannel, AkUInt32 in_iOutputChannel)
{
	register AkReal32 * AK_RESTRICT pfBuf = io_pInput->GetChannel( in_iInputChannel );
	register AkReal32 * AK_RESTRICT pfOut = io_pInput->GetChannel( in_iInputChannel );

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

#ifdef XBOX360
	RemoveDenormal( pfMem[2] );
	RemoveDenormal( pfMem[3] );
#endif 

	return ulSampleFrameCount;
}


void AccelerationFilter::Reset()
{
	memset(m_pfFiltMem, 0, sizeof(AkReal32) * MAX_INPUT_CHANNELS*AK_LPF_NUM_FILTMEM);
}

/************************************************************************/
/* FilterDecimator class 
/************************************************************************/

FilterDecimator::FilterDecimator(AkUInt32 in_iSampleRate) 
: m_AccelFilter(in_iSampleRate)
{	
	m_iAudioSampleRate = in_iSampleRate;
	Reset();
}

FilterDecimator::~FilterDecimator()
{
}

void FilterDecimator::Execute(AkAudioBuffer & io_rInput, AkAudioBuffer* out_pBuffer)
{
	AkUInt16 iMaxToProcess;
	AkUInt16 aChannelsToProcess[MAX_INPUT_CHANNELS];
	FindChannelsToProcess(io_rInput, aChannelsToProcess, iMaxToProcess);
	
	//This parameter is optional
	if (out_pBuffer == NULL)
		out_pBuffer = &io_rInput;

	AKASSERT(out_pBuffer->NumChannels() == io_rInput.NumChannels() && 
		out_pBuffer->MaxFrames() == io_rInput.MaxFrames());

	out_pBuffer->uValidFrames = io_rInput.uValidFrames;
	
	AkUInt16 iSamples = 0;
	for(AkUInt32 i = 0; i < iMaxToProcess; i++)
	{
		m_AccelFilter.PerformMonoInterp(&io_rInput, out_pBuffer, aChannelsToProcess[i], i);
		iSamples = Decimate(out_pBuffer, out_pBuffer, aChannelsToProcess[i], m_aChannelMem[i]);
	}

	out_pBuffer->uValidFrames = iSamples;
}

void FilterDecimator::Reset()
{
	memset(m_aChannelMem, 0, sizeof(ChannelInfo) * MAX_INPUT_CHANNELS);
	m_AccelFilter.Reset();
}

AkUInt16 FilterDecimator::Decimate( AkAudioBuffer *in_pInput, AkAudioBuffer* out_pBuffer, AkUInt32 in_iChannel, ChannelInfo& in_rMemory )
{
	//We want to decimate to 375 Hz.  We need to remove any component above 187.5 Hz 
	//while keeping everything below 100 Hz.
	
	//After the Butterworth filter, there is no power after 1664 Hz (gain is lower than 96.3 dB)
	//Therefore we can decimate to 3328 Hz.  To simplify things, lets say 3750.
	//The Butterworth filter has already attenuated the signal by -58.4 dB at 187.5 Hz.  We need an
	//extra filtering step to reduce to -96.3 dB by attenuating 37.9 dB.

	//This will be done with a FIR which allows us to compute only the output samples (so only 375 computations).
	
	//So the input buffer can be considered running at 3750 Hz.  The output is running at 375.
	AkReal32 *pInput = in_pInput->GetChannel( in_iChannel );
	AkReal32 *pOutput = out_pBuffer->GetChannel( in_iChannel );
	
	const AkUInt16 iSkipOutput = (AkUInt16)(m_iAudioSampleRate/DBOX_SAMPLERATE);
	const AkUInt16 iSkipInput = (AkUInt16)(m_iAudioSampleRate/INTERMEDIATE_RATE);
	AkUInt16 iOutput = 0;

	for(; in_rMemory.m_iNextOutput < in_pInput->uValidFrames; in_rMemory.m_iNextOutput += iSkipOutput, iOutput++)
	{
		AkReal32 fOutput = 0.0f;

		//Copy the input values in the ring buffer
		for(; in_rMemory.m_iNextInputToBuffer <= in_rMemory.m_iNextOutput; in_rMemory.m_iNextInputToBuffer += iSkipInput)
		{
			in_rMemory.m_iBufferHead = (in_rMemory.m_iBufferHead + 1) & BUFFER_MASK;
			in_rMemory.m_pRingBuffer[in_rMemory.m_iBufferHead] = pInput[in_rMemory.m_iNextInputToBuffer];
		}

		//Compute the value
		AkUInt16 iBufferIndex = (in_rMemory.m_iBufferHead - NUM_COEFS) & BUFFER_MASK;
		for(AkUInt32 j = 0; j < NUM_COEFS; j++)
		{
			fOutput += c_fFIRCoefs[j] * in_rMemory.m_pRingBuffer[iBufferIndex];
			iBufferIndex = (iBufferIndex + 1 )& BUFFER_MASK;
		}

		pOutput[iOutput] = fOutput;
	}

	//Copy remaining values
	for(; in_rMemory.m_iNextInputToBuffer < in_pInput->uValidFrames; in_rMemory.m_iNextInputToBuffer += iSkipInput)
	{
		in_rMemory.m_iBufferHead = (in_rMemory.m_iBufferHead + 1) & BUFFER_MASK;
		in_rMemory.m_pRingBuffer[in_rMemory.m_iBufferHead] = pInput[in_rMemory.m_iNextInputToBuffer];
	}
	in_rMemory.m_iNextInputToBuffer -= in_pInput->uValidFrames;
	in_rMemory.m_iNextOutput -= in_pInput->uValidFrames;
	
	return iOutput;
}