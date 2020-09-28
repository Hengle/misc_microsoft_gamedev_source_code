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
// AkSpeakerPan.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SPEAKER_PAN_H_
#define _SPEAKER_PAN_H_

#include <AK/SoundEngine/Common/AkTypes.h>

// Number of 'degrees' in a Speaker Pan circle (360 was an unsufficient precision)
#define PAN_CIRCLE 512 // PAN_CRCLE MUST be a power of 2 since we do &= (PAN_CIRCLE-1) to allow wrapping around after exceding 511.

struct AkSIMDSpeakerVolumes;

//====================================================================================================
// speaker pan
//====================================================================================================
class CAkSpeakerPan
{
public:
	static void Init();

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	// wrapper to handle spread
	static void GetSpeakerVolumes( 
		AkReal32			in_fX,
		AkReal32			in_fY,
		AkReal32			in_fDivergenceCenter,
		AkReal32			in_fSpread,
		AkSIMDSpeakerVolumes* out_pVolumes,
		AkUInt32			in_uNumFullBandChannels );
	// calculate volumes using a pair-wise algorithm.
	static void GetSpeakerVolumesdB(
		AkReal32			in_fX,
		AkReal32			in_fY,
		AkReal32			in_fDivergenceCenter, // [0..1]
		AkSpeakerVolumes*	out_pVolumes
		);
	static void AddSpeakerVolumesPower(
		AkInt32				in_iAngle,
		AkReal32			in_fDivergenceCenter, // [0..1]
		AkSpeakerVolumes*	out_pVolumes
		);

	static void GetSpeakerVolumes2DPan(
		AkReal32			in_fX,			// [0..1] // 0 = full left, 1 = full right
		AkReal32			in_fY,			// [0..1] // 0 = full rear, 1 = full front
		AkReal32			in_fCenterPct,	// [0..1]
		bool				in_bIsPannerEnabled,
		AkUInt32			in_uChannelMask,
		AkSIMDSpeakerVolumes*	out_pVolumes
		);

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
private:
	static void _GetSpeakerVolumes2DPan(
		AkReal32			in_fX,			// [0..1] // 0 = full left, 1 = full right
		AkReal32			in_fY,			// [0..1] // 0 = full rear, 1 = full front
		AkReal32			in_fCenterPct,	// [0..1]
		AkSpeakerVolumes*	out_pVolumes
		);

	static void _GetSpeakerVolumes2DPanSourceHasCenter(
		AkReal32			in_fX,			// [0..1] // 0 = full left, 1 = full right
		AkReal32			in_fY,			// [0..1] // 0 = full rear, 1 = full front
		AkSpeakerVolumes*	out_pVolumes
		);

	//Look up tables for trigonometric functions

	static AkReal32 m_fSin2[PAN_CIRCLE/2]; // sin^2(x), for half a cycle
};

#endif
