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

#pragma once

#include "AkCommon.h"

#define MAX_INPUT_CHANNELS 5

// Number of filter memory slots
#define AK_LPF_NUM_FILTMEM		4

// Number of Butterworth LP filter coefficients
#define AK_LPF_NUM_COEFICIENTS	4

//This filter limits the acceleration of the signal to 9.8 m/s2 or lower
//It is a second order butterworth filter with the corner frequency set to 6.5 Hz.
class AccelerationFilter
{
public:
	AccelerationFilter(AkUInt32 in_iSampleRate);
	~AccelerationFilter();

	void Execute(AkPipelineBuffer & io_rInput, AkPipelineBuffer* out_pBuffer = NULL);
	AkUInt16 PerformMonoInterp( AkPipelineBuffer * io_pInput, AkPipelineBuffer* out_pBuffer, AkUInt32 in_iInputChannel, AkUInt32 in_iOutputChannel);
	void Reset();
	
	AkReal32 m_pfFiltMem[MAX_INPUT_CHANNELS][AK_LPF_NUM_FILTMEM];		//Butterworth filter memory
	AkReal64 m_pfFiltCoefs[AK_LPF_NUM_COEFICIENTS];	// Butterworth LP filter coefficients
	AkUInt32 m_iAudioSampleRate;			
};
