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
// filterdecimator.h
// Implementation of a decimator to bring an audio stream to a sample
// rate compatible with DBOX (400 Hz).  The filter is done in 2 steps:
//
// First a Butterworth filter of order 2 with a cutoff at 6.5 Hz will shape the
// signal to avoid acceleration greater than 9.8 m/s.  This filter also
// has the property of getting rid of all frequancies above 1600 Hz.
// At 200 Hz, the filter has a gain of -59.5 dB.  To manage to decimate to 
// 400 samples per second, we must have a gain of -96.3 dB at 200 Hz.
//
// The second stage is tuned to get the extra 36.8 dB at 200 Hz. It is 
// a FIR with a cutoff at 100 Hz and tuned for a sample rate of 4kHz.  
// However, we compute an output at 400 Hz, skiping 10 input samples 
// (coming from the decimated output of the butterworth).
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkCommon.h"

#define MAX_INPUT_CHANNELS 5

// Number of filter memory slots
#define AK_LPF_NUM_FILTMEM		4

// Number of Butterworth LP filter coefficients
#define AK_LPF_NUM_COEFICIENTS	4

#define BUFFER_SIZE 64 //64 is the closest power of 2 from 55
#define BUFFER_MASK (BUFFER_SIZE - 1)

//This filter limits the acceleration of the signal to 9.8 m/s2 or lower
//It is a second order butterworth filter with the corner frequency set to 6.5 Hz.
class AccelerationFilter
{
public:
	AccelerationFilter(AkUInt32 in_iSampleRate);
	~AccelerationFilter();

	void Execute(AkAudioBuffer & io_rInput, AkAudioBuffer* out_pBuffer = NULL);
	AkUInt16 PerformMonoInterp( AkAudioBuffer * io_pInput, AkAudioBuffer* out_pBuffer, AkUInt32 in_iInputChannel, AkUInt32 in_iOutputChannel);
	void Reset();
	
	AkReal32 m_pfFiltMem[MAX_INPUT_CHANNELS][AK_LPF_NUM_FILTMEM];		//Butterworth filter memory
	AkReal64 m_pfFiltCoefs[AK_LPF_NUM_COEFICIENTS];	// Butterworth LP filter coefficients
	AkUInt32 m_iAudioSampleRate;			
};

class FilterDecimator
{
public:
	FilterDecimator(AkUInt32 in_iSampleRate);
	~FilterDecimator();

	void Execute(AkAudioBuffer & io_rInput, AkAudioBuffer* out_pBuffer = NULL);
	void Reset();

private:
	struct ChannelInfo{
		AkReal32 m_pRingBuffer[BUFFER_SIZE];		//FIR filter memory
		AkUInt16 m_iBufferHead;						//Head of the ring buffer
		AkUInt16 m_iNextInputToBuffer;				//Next sample to put in the buffer
		AkUInt16 m_iNextOutput;						//Next sample where to compute an output.	
	};

	AkUInt16 Decimate(AkAudioBuffer *in_pInput, AkAudioBuffer* out_pBuffer, AkUInt32 in_iChannel, ChannelInfo& in_rMemory);
	
	AccelerationFilter m_AccelFilter;
	ChannelInfo m_aChannelMem[MAX_INPUT_CHANNELS];
	AkUInt32 m_iAudioSampleRate;			
};