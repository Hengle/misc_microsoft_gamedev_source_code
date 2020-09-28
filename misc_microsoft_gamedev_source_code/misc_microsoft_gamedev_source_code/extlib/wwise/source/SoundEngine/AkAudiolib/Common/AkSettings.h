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
// AkSettings.h
//
// Platform dependent part
// Allowing implementation tu run with different "native" platform settings
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_SETTINGS_H_
#define _AUDIOLIB_SETTINGS_H_

#ifdef RVL_OS
#define DEFAULT_NATIVE_FREQUENCY 32000

//On the Wii, its only for the Wiimote motor.
#define AK_FEEDBACK_SAMPLE_RATE 83		
#else
#define DEFAULT_NATIVE_FREQUENCY 48000
#define AK_FEEDBACK_SAMPLE_RATE 375
#endif

// Slew time (time to reach target) ==  LPFPARAMUPDATEPERIOD * NUMBLOCKTOREACHTARGET sample frames
// We wish to minimize NUMBLOCKTOREACHTARGET to process bigger blocks and reduce the performance costs of parameter
// interpolation which requires a recomputation of the LPF coefficients.
// Number of samples between each LPF parameter interpolation ( triggers coef recalculate )
#define DEFAULT_LPF_UPDATE_PERIOD 128

// actually Only Win32 can have different native define.
#ifdef WIN32

namespace AkAudioLibSettings
{
	extern AkUInt32 g_pipelineCoreFrequency;
	extern AkTimeMs g_msPerBufferTick;
	extern AkUInt32 g_pcWaitTime;
	extern AkUInt32 g_uLpfUpdatePeriod;

	void SetSampleFrequency( AkUInt32 in_uSampleFrequency );
};

#define AK_MS_PER_BUFFER_TICK	AkAudioLibSettings::g_msPerBufferTick
#define AK_CORE_SAMPLERATE		AkAudioLibSettings::g_pipelineCoreFrequency
#define AK_PC_WAIT_TIME			AkAudioLibSettings::g_pcWaitTime
#define LPFPARAMUPDATEPERIOD	AkAudioLibSettings::g_uLpfUpdatePeriod

#else

#define AK_MS_PER_BUFFER_TICK	((AkUInt32)( LE_MAX_FRAMES_PER_BUFFER / ( DEFAULT_NATIVE_FREQUENCY / 1000.0f ) ))
#define AK_CORE_SAMPLERATE		DEFAULT_NATIVE_FREQUENCY
#define AK_PC_WAIT_TIME			((AkUInt32) ( 1000.0f * AK_NUM_VOICE_REFILL_FRAMES / DEFAULT_NATIVE_FREQUENCY / 2.0f ))
#define LPFPARAMUPDATEPERIOD	DEFAULT_LPF_UPDATE_PERIOD

#endif


#endif //_AUDIOLIB_SETTINGS_H_
