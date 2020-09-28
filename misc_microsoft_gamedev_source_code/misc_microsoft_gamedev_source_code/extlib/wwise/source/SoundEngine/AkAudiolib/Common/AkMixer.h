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
// AkMixer.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_MIXER_H_
#define _AK_MIXER_H_

// Comment this line to disable SIMD optimizations
#define USE_SIMD

#include "AkCommon.h"					// AkSpeakerVolumes

#ifdef AK_PS3
#include "AkVPLNode.h"
#endif

#ifdef WIN32
#define AK_40MIXER
#endif

//Number of samples processed at the same time.  
#define AK_MIX_NUM_SAMPLE_PACK (2)

class	AkAudioBuffer;
struct	AkSpeakerVolumes;

//====================================================================================================
// structures to make things easier
//====================================================================================================
//====================================================================================================
struct AkSpeakerVolumesMono
{
	AkReal32 fVolume;
	AkReal32 fVolumeDelta;
	AkReal32 fLfe;
	AkReal32 fLfeDelta;

	AkSpeakerVolumesMono( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		fVolume = AudioMix.Previous.volumes.fFrontLeft;
		fVolumeDelta = (AudioMix.Next.volumes.fFrontLeft - AudioMix.Previous.volumes.fFrontLeft) * fOneOverNumSamples;
		fLfe = AudioMix.Previous.volumes.fLfe;
		fLfeDelta = (AudioMix.Next.volumes.fLfe - AudioMix.Previous.volumes.fLfe) * fOneOverNumSamples;
	}
};
//====================================================================================================
//====================================================================================================
struct AkSpeakerVolumesStereo
{
	AkReal32 fLeft;
	AkReal32 fLeftDelta;
	AkReal32 fRight;
	AkReal32 fRightDelta;
	AkReal32 fLfe;
	AkReal32 fLfeDelta;

	AkSpeakerVolumesStereo( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		fLeft = AudioMix.Previous.volumes.fFrontLeft;
		fLeftDelta = (AudioMix.Next.volumes.fFrontLeft - AudioMix.Previous.volumes.fFrontLeft) * fOneOverNumSamples;
		fRight = AudioMix.Previous.volumes.fFrontRight;
		fRightDelta = (AudioMix.Next.volumes.fFrontRight - AudioMix.Previous.volumes.fFrontRight) * fOneOverNumSamples;
		fLfe = AudioMix.Previous.volumes.fLfe;
		fLfeDelta = (AudioMix.Next.volumes.fLfe - AudioMix.Previous.volumes.fLfe) * fOneOverNumSamples;
	}
};

//====================================================================================================
// used for 3D -> 4.0
//====================================================================================================
#ifdef AK_40MIXER
struct AkSpeakerVolumesFour
{
	AkReal32 fFrontLeft;
	AkReal32 fFrontLeftDelta;
	AkReal32 fFrontRight;
	AkReal32 fFrontRightDelta;
	AkReal32 fRearLeft;
	AkReal32 fRearLeftDelta;
	AkReal32 fRearRight;
	AkReal32 fRearRightDelta;

	AkSpeakerVolumesFour( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		fFrontLeft = AudioMix.Previous.volumes.fFrontLeft;
		fFrontLeftDelta = (AudioMix.Next.volumes.fFrontLeft - AudioMix.Previous.volumes.fFrontLeft) * fOneOverNumSamples;
		fFrontRight = AudioMix.Previous.volumes.fFrontRight;
		fFrontRightDelta = (AudioMix.Next.volumes.fFrontRight - AudioMix.Previous.volumes.fFrontRight) * fOneOverNumSamples;
		fRearLeft = AudioMix.Previous.volumes.fRearLeft;
		fRearLeftDelta = (AudioMix.Next.volumes.fRearLeft - AudioMix.Previous.volumes.fRearLeft) * fOneOverNumSamples;
		fRearRight = AudioMix.Previous.volumes.fRearRight;
		fRearRightDelta = (AudioMix.Next.volumes.fRearRight - AudioMix.Previous.volumes.fRearRight) * fOneOverNumSamples;
	}
};
#endif

//====================================================================================================
// used for 3D -> 5.1
//====================================================================================================
struct AkSpeakerVolumesFiveOne
{
	AkReal32 fFrontLeft;
	AkReal32 fFrontLeftDelta;
	AkReal32 fFrontRight;
	AkReal32 fFrontRightDelta;
	AkReal32 fCenter;
	AkReal32 fCenterDelta;
	AkReal32 fLfe;
	AkReal32 fLfeDelta;
	AkReal32 fRearLeft;
	AkReal32 fRearLeftDelta;
	AkReal32 fRearRight;
	AkReal32 fRearRightDelta;

	AkSpeakerVolumesFiveOne( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		fFrontLeft = AudioMix.Previous.volumes.fFrontLeft;
		fFrontLeftDelta = (AudioMix.Next.volumes.fFrontLeft - AudioMix.Previous.volumes.fFrontLeft) * fOneOverNumSamples;
		fFrontRight = AudioMix.Previous.volumes.fFrontRight;
		fFrontRightDelta = (AudioMix.Next.volumes.fFrontRight - AudioMix.Previous.volumes.fFrontRight) * fOneOverNumSamples;
		fCenter = AudioMix.Previous.volumes.fCenter;
		fCenterDelta = (AudioMix.Next.volumes.fCenter - AudioMix.Previous.volumes.fCenter) * fOneOverNumSamples;
		fLfe = AudioMix.Previous.volumes.fLfe;
		fLfeDelta = (AudioMix.Next.volumes.fLfe - AudioMix.Previous.volumes.fLfe) * fOneOverNumSamples;
		fRearLeft = AudioMix.Previous.volumes.fRearLeft;
		fRearLeftDelta = (AudioMix.Next.volumes.fRearLeft - AudioMix.Previous.volumes.fRearLeft) * fOneOverNumSamples;
		fRearRight = AudioMix.Previous.volumes.fRearRight;
		fRearRightDelta = (AudioMix.Next.volumes.fRearRight - AudioMix.Previous.volumes.fRearRight) * fOneOverNumSamples;
	}
};
//====================================================================================================
// used for 3D -> 7.1
//====================================================================================================
#ifdef AK_71AUDIO
struct AkSpeakerVolumesSevenOne
{
	AkReal32 fFrontLeft;
	AkReal32 fFrontLeftDelta;
	AkReal32 fFrontRight;
	AkReal32 fFrontRightDelta;
	AkReal32 fCenter;
	AkReal32 fCenterDelta;
	AkReal32 fLfe;
	AkReal32 fLfeDelta;
	AkReal32 fRearLeft;
	AkReal32 fRearLeftDelta;
	AkReal32 fRearRight;
	AkReal32 fRearRightDelta;
	// PhM : rename these as needed
	AkReal32 fExtraLeft;
	AkReal32 fExtraLeftDelta;
	AkReal32 fExtraRight;
	AkReal32 fExtraRightDelta;

	AkSpeakerVolumesSevenOne( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		fFrontLeft = AudioMix.Previous.volumes.fFrontLeft;
		fFrontLeftDelta = (AudioMix.Next.volumes.fFrontLeft - AudioMix.Previous.volumes.fFrontLeft) * fOneOverNumSamples;
		fFrontRight = AudioMix.Previous.volumes.fFrontRight;
		fFrontRightDelta = (AudioMix.Next.volumes.fFrontRight - AudioMix.Previous.volumes.fFrontRight) * fOneOverNumSamples;
		fCenter = AudioMix.Previous.volumes.fCenter;
		fCenterDelta = (AudioMix.Next.volumes.fCenter - AudioMix.Previous.volumes.fCenter) * fOneOverNumSamples;
		fLfe = AudioMix.Previous.volumes.fLfe;
		fLfeDelta = (AudioMix.Next.volumes.fLfe - AudioMix.Previous.volumes.fLfe) * fOneOverNumSamples;
		fRearLeft = AudioMix.Previous.volumes.fRearLeft;
		fRearLeftDelta = (AudioMix.Next.volumes.fRearLeft - AudioMix.Previous.volumes.fRearLeft) * fOneOverNumSamples;
		fRearRight = AudioMix.Previous.volumes.fRearRight;
		fRearRightDelta = (AudioMix.Next.volumes.fRearRight - AudioMix.Previous.volumes.fRearRight) * fOneOverNumSamples;
		fExtraLeft = AudioMix.Previous.volumes.fExtraLeft;
		fExtraLeftDelta = (AudioMix.Next.volumes.fExtraLeft - AudioMix.Previous.volumes.fExtraLeft) * fOneOverNumSamples;
		fExtraRight = AudioMix.Previous.volumes.fExtraRight;
		fExtraRightDelta = (AudioMix.Next.volumes.fExtraRight - AudioMix.Previous.volumes.fExtraRight) * fOneOverNumSamples;
	}
};
#endif

struct AkSpeakerVolumesStereo3D
{
	AkReal32 fLeft;
	AkReal32 fLeftDelta;
	AkReal32 fRight;
	AkReal32 fRightDelta;

	// Maths in Power to keep constant power.
	// Left	 = Left  + (Center - 3 dB) + (LeftSurround - 3 dB)
	// Right = Right + (Center - 3 dB) + (RightSurround - 3 dB)
	AkSpeakerVolumesStereo3D( AkAudioMix& AudioMix, AkReal32 fOneOverNumSamples )
	{
		AkReal32 fFrontLeft		= AudioMix.Previous.volumes.fFrontLeft;
		AkReal32 fFrontRight	= AudioMix.Previous.volumes.fFrontRight;
		AkReal32 fCenter		= AudioMix.Previous.volumes.fCenter;
		AkReal32 fRearLeft		= AudioMix.Previous.volumes.fRearLeft;
		AkReal32 fRearRight		= AudioMix.Previous.volumes.fRearRight;

		fLeft	= sqrtf( (fFrontLeft*fFrontLeft)	+ 0.5f*( (fCenter*fCenter)	+ (fRearLeft*fRearLeft) ) );
		fRight	= sqrtf( (fFrontRight*fFrontRight)	+ 0.5f*( (fCenter*fCenter)	+ (fRearRight*fRearRight) ) );

		fFrontLeft	= AudioMix.Next.volumes.fFrontLeft;
		fFrontRight	= AudioMix.Next.volumes.fFrontRight;
		fCenter		= AudioMix.Next.volumes.fCenter;
		fRearLeft	= AudioMix.Next.volumes.fRearLeft;
		fRearRight	= AudioMix.Next.volumes.fRearRight;

		fLeftDelta	= sqrtf( (fFrontLeft*fFrontLeft)	+ 0.5f*( (fCenter*fCenter) + (fRearLeft*fRearLeft) ) )	- fLeft;
		fRightDelta = sqrtf( (fFrontRight*fFrontRight)	+ 0.5f*( (fCenter*fCenter) + (fRearRight*fRearRight) ) )- fRight;
		fLeftDelta  *= fOneOverNumSamples;
		fRightDelta *= fOneOverNumSamples;
	}
};
//====================================================================================================
//====================================================================================================
class CAkMixer
{
	friend class CAkMixerUT;
public:
	CAkMixer();
	~CAkMixer();

	AKRESULT Init(
		AkUInt32 in_uNumChannels,
		AkUInt16 in_uMaxFrames
		);
	void Term();

#ifdef AK_PS3
	void ExecuteSPU(
		class AkVPLMixState * in_pMixState,				///< 
		AkPipelineBufferBase&	in_OutBuffer			///< 
		);
	AKRESULT FinalExecuteSPU(
		AkAudioBufferFinalMix*	in_pInputBuffer,		///< 
		AkPipelineBufferBase*	in_pOutputBuffer		///< 
		);
	AKRESULT FinalInterleaveExecuteSPU(
		AkAudioBufferFinalMix*	in_pInputBuffer,		///< 
		AkPipelineBufferBase*	in_pOutputBuffer		///< 
		);
#endif

	void MixStereo(
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

	void Mix3D(
		AkAudioBufferMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

	void Mix51(	
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

#ifdef AK_71AUDIO
	void Mix71(	
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif

	void MixFinalStereo(
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

	void MixFinal51(
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);

#ifdef AK_71AUDIO
	void MixFinal71(
		AkAudioBufferFinalMix*	in_pInputBuffer,
		AkPipelineBufferBase*	in_pOutputBuffer
		);
#endif

	AkForceInline AkChannelMask GetChannelMask() { return m_uChannelMask; }

private:

	void MixNStereoPrev( AkAudioBufferFinalMix* in_pInputBuffer );
	void MixN51Prev( AkAudioBufferFinalMix* in_pInputBuffer );

#ifdef AK_71AUDIO
	void MixN71Prev( AkAudioBufferMix* in_pInputBuffer );
#endif

	void MixN3DFiveOnePrev( AkAudioBufferMix* in_pInputBuffer );

#ifdef AK_71AUDIO
	void MixN3DSevenOnePrev( AkAudioBufferMix* in_pInputBuffer );
#endif

	void MixN3DStereoPrev( AkAudioBufferMix* in_pInputBuffer );

#ifdef AK_40MIXER
	void MixN3DFourPrev( AkAudioBufferMix * in_pInputBuffer );
#endif

	void MixAndInterleaveStereo( AkAudioBufferFinalMix* in_pInputBuffer );

	void MixAndInterleave51( AkAudioBufferFinalMix* in_pInputBuffer );

#ifdef AK_71AUDIO
	void MixAndInterleave71( AkAudioBufferFinalMix* in_pInputBuffer );
#endif

	// functions (were macros)
	void AddVolume(
		AkReal32*	in_pSourceData,
		AkReal32*	in_pDestData,
		AkReal32	in_fVolume,
		AkReal32	in_fVolumeDelta
		);
#ifdef USE_SIMD
	AkForceInline void VolumeInterleavedStereo( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkReal32 in_fVolumeL, AkReal32 in_fVolumeLDelta, AkReal32 in_fVolumeR, AkReal32 in_fVolumeRDelta );
	AkForceInline void VolumeInterleaved51( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesFiveOne in_Volumes51 );
#ifdef AK_71AUDIO
	AkForceInline void VolumeInterleaved71( AkAudioBuffer*	in_pSource, AkReal32* in_pDestData, AkSpeakerVolumesSevenOne in_Volumes71 );
#endif
#else
	AkForceInline void VolumeInterleaved( AkReal32* in_pSourceData, AkReal32* in_pDestData, AkReal32 in_fVolume, AkReal32 in_fVolumeDelta );
#endif


	// the useful things

	AkPipelineBufferBase*	m_pOutputBuffer;

	/// NOTE. Technically, keeping the channel mask, or even the number of channels,
	/// should not be required here. However, inconsistencies between the platform-specific
	/// implementations of busses make it dangerous to rely only on the output buffer's 
	/// number of channels (sometimes it is not set prior to calls to mixer).
	AkChannelMask	m_uChannelMask;

	AkUInt16		m_usMaxFrames;
	AkReal32		m_fOneOverNumFrames;

#ifdef USE_SIMD
#ifdef XBOX360
	__vector4		m_vUnpackLo;
	__vector4		m_vUnpackHi;
	__vector4		m_vShuffle0101;
	__vector4		m_vShuffle0202;
	__vector4		m_vShuffle0213;
	__vector4		m_vShuffle1313;
	__vector4		m_vShuffle2323;
#endif
#endif
};
#endif
