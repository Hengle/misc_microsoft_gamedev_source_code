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
// AkMixer.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkCommon.h"
#include "AkMixer.h"
#include "AudiolibDefs.h"
#include "AkMath.h"

//====================================================================================================
//====================================================================================================
CAkMixer::CAkMixer()
{
	m_pOutputBuffer = NULL;
	m_uChannelMask = 0;
	m_fOneOverNumFrames = 0.0f;
#ifdef XBOX360
#ifdef USE_SIMD
	m_vUnpackLo.u[0] = 0x00010203;		// a0
	m_vUnpackLo.u[1] = 0x10111213;		// b0
	m_vUnpackLo.u[2] = 0x04050607;		// a1
	m_vUnpackLo.u[3] = 0x14151617;		// b1
	m_vUnpackHi.u[0] = 0x08090A0B;		// a2
	m_vUnpackHi.u[1] = 0x18191A1B;		// b2
	m_vUnpackHi.u[2] = 0x0C0D0E0F;		// a3
	m_vUnpackHi.u[3] = 0x1C1D1E1F;		// b3
	m_vShuffle0101.u[0] = 0x00010203;	// a0
	m_vShuffle0101.u[1] = 0x04050607;	// a1
	m_vShuffle0101.u[2] = 0x10111213;	// b0
	m_vShuffle0101.u[3] = 0x14151617;	// b1
	m_vShuffle0202.u[0] = 0x00010203;	// a0
	m_vShuffle0202.u[1] = 0x08090A0B;	// a2
	m_vShuffle0202.u[2] = 0x10111213;	// b0
	m_vShuffle0202.u[3] = 0x18191A1B;	// b2
	m_vShuffle0213.u[0] = 0x00010203;	// a0
	m_vShuffle0213.u[1] = 0x08090A0B;	// a2
	m_vShuffle0213.u[2] = 0x14151617;	// b1
	m_vShuffle0213.u[3] = 0x1C1D1E1F;	// b3
	m_vShuffle1313.u[0] = 0x04050607;	// a1
	m_vShuffle1313.u[1] = 0x0C0D0E0F;	// a3
	m_vShuffle1313.u[2] = 0x14151617;	// b1
	m_vShuffle1313.u[3] = 0x1C1D1E1F;	// b3
	m_vShuffle2323.u[0] = 0x08090A0B;	// a2
	m_vShuffle2323.u[1] = 0x0C0D0E0F;	// a3
	m_vShuffle2323.u[2] = 0x18191A1B;	// b2
	m_vShuffle2323.u[3] = 0x1C1D1E1F;	// b3
#endif
#endif
}
//====================================================================================================
//====================================================================================================
CAkMixer::~CAkMixer()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkMixer::Init(AkChannelMask in_uChannelMask, AkUInt16 in_uMaxFrames)
{
	m_uChannelMask = in_uChannelMask;
	m_usMaxFrames = in_uMaxFrames;
	m_fOneOverNumFrames = (1.0f / ( ((AkReal32)in_uMaxFrames) / AK_MIX_NUM_SAMPLE_PACK) );

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
void CAkMixer::Term()
{
}
#ifndef AK_PS3
//====================================================================================================
//====================================================================================================
void CAkMixer::MixStereo(AkAudioBufferFinalMix*	in_pInputBuffer,
						AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);

	m_pOutputBuffer = in_pOutputBuffer;

	AKASSERT( m_uChannelMask == AK_SPEAKER_SETUP_STEREO );
	AKASSERT( m_pOutputBuffer->GetChannelMask() == AK_SPEAKER_SETUP_STEREO );

	MixNStereoPrev( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
	AKASSERT( m_pOutputBuffer->GetChannelMask() == m_uChannelMask );
}
//====================================================================================================
//====================================================================================================
void CAkMixer::Mix51(	AkAudioBufferFinalMix*	in_pInputBuffer,
						AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);

	m_pOutputBuffer = in_pOutputBuffer;

	// No question about speaker config here, we are strictly applying volumes to 5.1 signal (input and output)

	MixN51Prev( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
	AKASSERT( m_pOutputBuffer->GetChannelMask() == m_uChannelMask );
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::Mix71(	AkAudioBufferFinalMix*	in_pInputBuffer,
						AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);

	m_pOutputBuffer = in_pOutputBuffer;

	// No question about speaker config here, we are strictly applying volumes to 7.1 signal (input and output)

	MixN71Prev( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
	AKASSERT( m_pOutputBuffer->GetChannelMask() == m_uChannelMask );
}
#endif
//====================================================================================================
//====================================================================================================
void CAkMixer::Mix3D(	AkAudioBufferMix*	in_pInputBuffer,
						AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);

	m_pOutputBuffer = in_pOutputBuffer;

	// get the converter ready
	switch(m_uChannelMask)
	{
	case AK_SPEAKER_SETUP_STEREO:
		MixN3DStereoPrev( in_pInputBuffer );
		break;
#ifdef AK_40MIXER
	case AK_SPEAKER_SETUP_4:
		MixN3DFourPrev( in_pInputBuffer );
		break;
#endif
	case AK_SPEAKER_SETUP_5POINT1:
		MixN3DFiveOnePrev( in_pInputBuffer );
		break;
#ifdef AK_71AUDIO
	case AK_SPEAKER_SETUP_7POINT1:
		MixN3DSevenOnePrev( in_pInputBuffer );
		break;
#endif
	default:
		AKASSERT(!"Unsupported 3D mix speaker config.");
		break;
	}

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixFinalStereo(	AkAudioBufferFinalMix*	in_pInputBuffer,
								AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	m_pOutputBuffer = in_pOutputBuffer;

	MixAndInterleaveStereo( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixFinal51(	AkAudioBufferFinalMix*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	m_pOutputBuffer = in_pOutputBuffer;

	MixAndInterleave51( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixFinal71(	AkAudioBufferFinalMix*	in_pInputBuffer,
							AkPipelineBufferBase*	in_pOutputBuffer)
{
	// check that we've got what we need
	AKASSERT(in_pInputBuffer != NULL);
	AKASSERT( in_pInputBuffer->NumChannels() == in_pOutputBuffer->NumChannels() );

	m_pOutputBuffer = in_pOutputBuffer;

	MixAndInterleave71( in_pInputBuffer );

	// set the number of output bytes
	m_pOutputBuffer->uValidFrames = m_usMaxFrames;
}
#endif
#endif // !PS3
#include "AkMixerSimd.cpp"

