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

#ifndef _INTERNALPITCHSTATE_H_
#define _INTERNALPITCHSTATE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

// Slew time for pitch parameter to reach its target (in samples)
static const AkUInt32 PITCHRAMPLENGTH = 1024;

// Class members used at runtime required by all platforms grouped together
#ifdef XBOX360 
	__declspec(align(16)) 
#endif 
struct AkInternalPitchState
{
	// Note: Keep this first to guarantee alignement on 16 bytes for XBox360
	union									// (In/Out) Last buffer values depending on format 
	{
		AkReal32	fLastValue[AK_VOICE_MAX_NUM_CHANNELS];
		AkInt16		iLastValue[AK_VOICE_MAX_NUM_CHANNELS];
		AkUInt8		uLastValue[AK_VOICE_MAX_NUM_CHANNELS];
	};

	// Pitch internal state
	AkUInt32 uInFrameOffset;				// (In/Out) Offset in buffer currently being read
	AkUInt32 uOutFrameOffset;				// (In/Out) Offset in buffer currently being filled

	AkUInt32 uFloatIndex;					// (In/Out) Fixed point index value
	AkUInt32 uCurrentFrameSkip;				// (In/Out) Current sample frame skip
	AkUInt32 uTargetFrameSkip;				// (In) Target frame skip
	AkUInt32 uInterpolationRampCount;		// (In/Out) Sample count for pitch interpolation (interpolation finished when == PITCHRAMPLENGTH)

#ifdef AK_PS3	
	AkUInt32 uRequestedFrames;				// (In) Desired output frames (max frames)
	AkUInt32 uInOffset;						// (In) Byte offset to actual input data start (DMA aligned on 16 bytes)
	AkUInt32 uInValidFrames;				// (In/Out) Valid input frames
	void * pOutBuffer;						// (In) Where to DMA output buffer (not aligned on 16 bytes)
	AkUInt32 uOutValidFrames;				// (Out) Number of output frames produced
	AKRESULT eState;						// (Out) The return state of the algorithm
	AkChannelMask uChannelMask;					// (In) Channel mask
	AkUInt32 uOutMaxFrames;					// (In) Distance between output channels
#endif // PS3

} AK_ALIGNED_16;

#endif // _INTERNALPITCHSTATE_H_
