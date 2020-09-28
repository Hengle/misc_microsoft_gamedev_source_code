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
// AkResampler.h
// 
// Combines software resampling and pitch shifting opreation in one algorithm using linear interpolation. 
// Routines support AkReal32 sample type as well as AkUInt8 and AkInt16
// Assumes same thread will call both SetPitch and Execute (not locking).
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_RESAMPLER_H_
#define _AK_RESAMPLER_H_

#include "AkInternalPitchState.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

// Internal pitch state that can be passed on to following sources for seemless sample accurate playback
struct AkSharedPitchState
{		
	// Last buffer value (1 per channel). Note fixed point format routines need to convert range when retrieving/storing this value.
	AkReal32  fLastBufVal[AK_VOICE_MAX_NUM_CHANNELS];
	// Fixed point index value
	AkUInt32 uFloatIndex;
	// Pitch interpolation variables
	AkUInt32 uCurrentFrameSkip;					// Current sample frame skip
	AkUInt32 uTargetFrameSkip;					// Target frame skip
	AkUInt32 uInterpolationRampCount;			// Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)
	AkReal32 fTargetPitchVal;					// Target pitch value
	AkReal32 fSampleRateConvertRatio;
	bool	 bFirstSetPitch;					// Flags first set pitch received
};
	
enum PitchOperatingMode
{
	PitchOperatingMode_Bypass = 0,
    PitchOperatingMode_Fixed,
	PitchOperatingMode_Interpolating,
	NumPitchOperatingMode,
};


enum InputDataType
{
	I16_1Chan,
	I16_2Chan,
	I16_1To4Chan,
	I16_5To8Chan,
	U8_1Chan,
	U8_2Chan,
	U8_1To4Chan,
	U8_5To8Chan,
	Native_1Chan,
	Native_2Chan,
	Native_1To4Chan,
	Native_5To8Chan,
	NumInputDataType,
};

#define ISI16TYPE( __index__ ) (((__index__) == I16_1Chan) || ((__index__) == I16_2Chan) || ((__index__) == I16_1To4Chan) || ((__index__) == I16_5To8Chan))
#define ISU8TYPE( __index__ ) (((__index__) == U8_1Chan) || ((__index__) == U8_2Chan) || ((__index__) == U8_1To4Chan) || ((__index__) == U8_5To8Chan))
#define ISNATIVETYPE( __index__ ) (((__index__) == Native_1Chan) || ((__index__) == Native_2Chan) || ((__index__) == Native_1To4Chan) || ((__index__) == Native_5To8Chan))

#if defined(WIN32) || defined(XBOX360)
//#define PERFORMANCE_BENCHMARK
#endif

class CAkResampler
{
public:
	CAkResampler();
	~CAkResampler();

	AKRESULT Init( AkAudioFormat * io_pFormat, AkUInt32 in_usSampleRate );
	AKRESULT Term( );	

	// Internal pitch state set/get
	void	SetPitchState( AkSharedPitchState & rPitchState );
	void	GetPitchState( AkSharedPitchState & rPitchState );

	// Execute
	// Resampling and pitch shifting is performed and input size consumed is returned
	// Returns Ak_DataNeeded if output buffer is not full yet
	// Returns Ak_DataReady if it is full and ready to be passed on
#ifdef AK_PS3
	void ExecutePS3( AkAudioBuffer * io_pInBuffer, AkAudioBuffer * io_pOutBuffer, AkUInt32 uRequestedSize, struct AkVPLState & io_state  );
#else
	AKRESULT Execute( AkAudioBuffer * io_pInBuffer, AkAudioBuffer * io_pOutBuffer, AkUInt32 uRequestedSize );
	AKRESULT DeinterleaveAndSwapOutputIfRequired( AkAudioBuffer * io_pIOBuffer );
#endif

	// Adjust requested time according to pitch parameter.
	void TimeInputToOutput( AkUInt32 & io_uFrames );
	void TimeOutputToInput( AkUInt32 & io_uFrames );

	void SetPitch( AkReal32 in_fPitchVal );
	// SetPitch without triggering interpolation or even setting the right DSP routine (used for time skipping).
	void SetPitchForTimeSkip( AkReal32 in_fPitchVal );

	void SetOutputBufferOffset( AkUInt32 in_ulNumFrameOffset ){ m_InternalPitchState.uOutFrameOffset = in_ulNumFrameOffset; }
	void SetInputBufferOffset( AkUInt32 in_ulNumFrameOffset ){ m_InternalPitchState.uInFrameOffset = in_ulNumFrameOffset; }
	AkUInt32 GetInputFrameOffset() { return m_InternalPitchState.uInFrameOffset; };
	void ResetOffsets();

	AkReal32 GetLastRate();

#ifndef AK_PS3
private:
#endif
	AkInternalPitchState m_InternalPitchState;	// These only needs to be public on PS3
	PitchOperatingMode	m_PitchOperationMode;	// Bypass, Fixed or Interpolating

private:
	
	AkReal32			m_fSampleRateConvertRatio;			// Sample rate conversion factor (source/target)
	AkReal32			m_fTargetPitchVal;					// Target pitch value
	AkUInt8				m_DSPFunctionIndex;					// Index of appropriate function given numChannels and data type (same for all 3 operation modes)
	AkUInt8				m_uNumChannels;
	AkUInt8				m_uInputBlockAlign;
	bool				m_bFirstSetPitch;					// Flags first set pitch received	

#ifdef PERFORMANCE_BENCHMARK
	AkUInt32			m_uNumberCalls;
	AkReal32			m_fTotalTime;
#endif
};

#endif // _AK_RESAMPLER_H_
