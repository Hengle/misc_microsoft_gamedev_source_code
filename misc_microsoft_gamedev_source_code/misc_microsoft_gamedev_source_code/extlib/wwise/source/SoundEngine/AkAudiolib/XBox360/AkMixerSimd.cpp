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
// AkMixerSIMD.cpp (Xbox360)
//
//////////////////////////////////////////////////////////////////////

//====================================================================================================
// Altivec versions of the mixers
//====================================================================================================
#include "ppcintrinsics.h"
static const AkUInt32	ulVectorSize = 4;

#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA ) \
	__vector4 VECT; \
	VECT.v[0] = VOLUME; \
	VECT.v[1] = VOLUME; \
	VECT.v[2] = VOLUME + VOLUME_DELTA; \
	VECT.v[3] = VOLUME + VOLUME_DELTA;
//====================================================================================================
//====================================================================================================
void CAkMixer::AddVolume( 
	AkReal32*	in_pSourceData, 
	AkReal32*	in_pDestData, 
	AkReal32	in_fVolume, 
	AkReal32	in_fVolumeDelta 
	)
{
	AKASSERT( !( m_usMaxFrames % 16 ) );

	AkReal32* AK_RESTRICT pSourceData = in_pSourceData;
	AkReal32* AK_RESTRICT pDestData = in_pDestData;
	AkReal32* AK_RESTRICT pSourceEnd = pSourceData + m_usMaxFrames;

	if ( in_fVolumeDelta == 0.0f )
	{
		if ( in_fVolume == 0.0f )
		{
			//everything is done over a previous cleared buffer, so if volume=0 we have nothing to do
			return;
		}

		BUILD_VOLUME_VECTOR( vVolumes, in_fVolume, 0.0f );

		do
		{
			// get four samples								 
			__vector4 vSum1 = __lvx( pSourceData, 0 );
			__vector4 vSum2 = __lvx( pSourceData, 16 );
			__vector4 vSum3 = __lvx( pSourceData, 32 );
			__vector4 vSum4 = __lvx( pSourceData, 48 );
			pSourceData += ulVectorSize * 4;

			// get the previous ones						 
			__vector4 vOutput1 = __lvx( pDestData, 0 );
			__vector4 vOutput2 = __lvx( pDestData, 16 );
			__vector4 vOutput3 = __lvx( pDestData, 32 );
			__vector4 vOutput4 = __lvx( pDestData, 48 );
			
			// apply volume	and add to output sample
			vSum1 = __vmaddfp( vSum1, vVolumes, vOutput1 );
			vSum2 = __vmaddfp( vSum2, vVolumes, vOutput2 );
			vSum3 = __vmaddfp( vSum3, vVolumes, vOutput3 );
			vSum4 = __vmaddfp( vSum4, vVolumes, vOutput4 );
			
			// store the result								 
			__stvx( vSum1, pDestData, 0 );
			__stvx( vSum2, pDestData, 16 );
			__stvx( vSum3, pDestData, 32 );
			__stvx( vSum4, pDestData, 48 );

			pDestData += ulVectorSize * 4;
		}
		while ( pSourceData < pSourceEnd );
	}

	else // has volume delta
	{
		AkReal32 fVolumesDelta = in_fVolumeDelta * 2;
		BUILD_VOLUME_VECTOR( vVolumesDelta, fVolumesDelta, 0 );

		BUILD_VOLUME_VECTOR( vVolumes1, in_fVolume, in_fVolumeDelta );
		__vector4 vVolumes2 = __vaddfp( vVolumes1, vVolumesDelta );
		__vector4 vVolumes3 = __vaddfp( vVolumes2, vVolumesDelta );
		__vector4 vVolumes4 = __vaddfp( vVolumes3, vVolumesDelta );

		// multiply volumes delta by 4 because the loop is unrolled by 4.
		vVolumesDelta = __vaddfp( vVolumesDelta, vVolumesDelta );
		vVolumesDelta = __vaddfp( vVolumesDelta, vVolumesDelta );

		do
		{
			// get four samples								 
			__vector4 vSum1 = __lvx( pSourceData, 0 );
			__vector4 vSum2 = __lvx( pSourceData, 16 );
			__vector4 vSum3 = __lvx( pSourceData, 32 );
			__vector4 vSum4 = __lvx( pSourceData, 48 );
			pSourceData += ulVectorSize * 4;

			// get the previous ones						 
			__vector4 vOutput1 = __lvx( pDestData, 0 );
			__vector4 vOutput2 = __lvx( pDestData, 16 );
			__vector4 vOutput3 = __lvx( pDestData, 32 );
			__vector4 vOutput4 = __lvx( pDestData, 48 );
			
			// apply volume	and add to output sample
			vSum1 = __vmaddfp( vSum1, vVolumes1, vOutput1 );
			vSum2 = __vmaddfp( vSum2, vVolumes2, vOutput2 );
			vSum3 = __vmaddfp( vSum3, vVolumes3, vOutput3 );
			vSum4 = __vmaddfp( vSum4, vVolumes4, vOutput4 );
			
			// store the result								 
			__stvx( vSum1, pDestData, 0 );
			__stvx( vSum2, pDestData, 16 );
			__stvx( vSum3, pDestData, 32 );
			__stvx( vSum4, pDestData, 48 );

			pDestData += ulVectorSize * 4;

			// add delta to volumes
			vVolumes1 = __vaddfp( vVolumes1, vVolumesDelta );
			vVolumes2 = __vaddfp( vVolumes2, vVolumesDelta );
			vVolumes3 = __vaddfp( vVolumes3, vVolumesDelta );
			vVolumes4 = __vaddfp( vVolumes4, vVolumesDelta );
		}
		while ( pSourceData < pSourceEnd );
	}
}
//====================================================================================================
//====================================================================================================
inline void CAkMixer::VolumeInterleavedStereo( 
	AkAudioBuffer*		in_pSource, 
	AkReal32*			in_pDestData, 
	AkReal32			in_fVolumeL, 
	AkReal32			in_fVolumeLDelta, 
	AkReal32			in_fVolumeR, 
	AkReal32			in_fVolumeRDelta 
	)
{
	BUILD_VOLUME_VECTOR( vVolumesL, in_fVolumeL, in_fVolumeLDelta );
	BUILD_VOLUME_VECTOR( vVolumesR, in_fVolumeR, in_fVolumeRDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_fVolumeLDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesLDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_fVolumeRDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesRDelta, fVolumesDelta, 0 );

	AkReal32* l_pSourceDataL = in_pSource->GetChannel( AK_IDX_SETUP_2_LEFT );
	AkReal32* l_pSourceDataR = in_pSource->GetChannel( AK_IDX_SETUP_2_RIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		__vector4 vVectL = __lvx( l_pSourceDataL, 0 );
		l_pSourceDataL += ulVectorSize;

		// apply volume									 
		vVectL = __vmulfp( vVectL, vVolumesL );

		// increment the volumes
		vVolumesL  = __vaddfp( vVolumesL, vVolumesLDelta );
		
		__vector4 vVectR = __lvx( l_pSourceDataR, 0 );
		l_pSourceDataR += ulVectorSize;

		// apply volume									 
		vVectR = __vmulfp( vVectR, vVolumesR );
		
		// increment the volumes
		vVolumesR  = __vaddfp( vVolumesR, vVolumesRDelta );
		
		// vVectL = [ L0, L1, L2, L3 ] * volumes
		// vVectR = [ R0, R1, R2, R3] * volumes
		// we want vDest1 = [ L0, R0, L1, R1]
//		__vector4 vDest1 = __vperm( vVectL, vVectR, m_vUnpackLo );
		__vector4 vDest1 = __vmrghw( vVectL, vVectR );

		// we want vDest2 = [ L2, R2, L3, R3 ]
//		__vector4 vDest2 = __vperm( vVectL, vVectR, m_vUnpackHi );
		__vector4 vDest2 = __vmrglw( vVectL, vVectR );

		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 0 );
		in_pDestData += ulVectorSize;

		// store the vDest2 result
		__stvx( vDest2, in_pDestData, 0 );
		in_pDestData += ulVectorSize;
	}
}
//====================================================================================================
//====================================================================================================
inline void CAkMixer::VolumeInterleaved51( 
	AkAudioBuffer*	in_pSource, 
	AkReal32*	in_pDestData, 
	AkSpeakerVolumesFiveOne	in_Volumes51
	)
{
	BUILD_VOLUME_VECTOR( vVolumesFL, in_Volumes51.fFrontLeft, in_Volumes51.fFrontLeftDelta );
	BUILD_VOLUME_VECTOR( vVolumesFR, in_Volumes51.fFrontRight, in_Volumes51.fFrontRightDelta );
	BUILD_VOLUME_VECTOR( vVolumesC, in_Volumes51.fCenter, in_Volumes51.fCenterDelta );
	BUILD_VOLUME_VECTOR( vVolumesLFE, in_Volumes51.fLfe, in_Volumes51.fLfeDelta );
	BUILD_VOLUME_VECTOR( vVolumesRL, in_Volumes51.fRearLeft, in_Volumes51.fRearLeftDelta );
	BUILD_VOLUME_VECTOR( vVolumesRR, in_Volumes51.fRearRight, in_Volumes51.fRearRightDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes51.fFrontLeftDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesFLDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes51.fFrontRightDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesFRDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes51.fCenterDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesCDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes51.fLfeDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesLFEDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes51.fRearLeftDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesRLDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes51.fRearRightDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesRRDelta, fVolumesDelta, 0 );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT );
	AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_5_CENTER );
	AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_5_LFE );
	AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARLEFT );
	AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARRIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		__vector4 vVectFL = __lvx( l_pSourceDataFL, 0 );
		l_pSourceDataFL += ulVectorSize;

		// apply volume									 
		vVectFL = __vmulfp( vVectFL, vVolumesFL );

		// increment the volumes
		vVolumesFL  = __vaddfp( vVolumesFL,  vVolumesFLDelta );
		
		__vector4 vVectFR = __lvx( l_pSourceDataFR, 0 );
		l_pSourceDataFR += ulVectorSize;

		// apply volume									 
		vVectFR = __vmulfp( vVectFR, vVolumesFR );
		
		// increment the volumes
		vVolumesFR  = __vaddfp( vVolumesFR,  vVolumesFRDelta );

		__vector4 vVectC = __lvx( l_pSourceDataC, 0 );
		l_pSourceDataC += ulVectorSize;

		// apply volume									 
		vVectC = __vmulfp( vVectC, vVolumesC );
		
		// increment the volumes
		vVolumesC  = __vaddfp( vVolumesC,  vVolumesCDelta );

		__vector4 vVectLFE = __lvx( l_pSourceDataLFE, 0 );
		l_pSourceDataLFE += ulVectorSize;

		// apply volume									 
		vVectLFE = __vmulfp( vVectLFE, vVolumesLFE );
		
		// increment the volumes
		vVolumesLFE  = __vaddfp( vVolumesLFE,  vVolumesLFEDelta );

		__vector4 vVectRL = __lvx( l_pSourceDataRL, 0 );
		l_pSourceDataRL += ulVectorSize;

		// apply volume									 
		vVectRL = __vmulfp( vVectRL, vVolumesRL );
		
		// increment the volumes
		vVolumesRL  = __vaddfp( vVolumesRL,  vVolumesRLDelta );

		__vector4 vVectRR = __lvx( l_pSourceDataRR, 0 );
		l_pSourceDataRR += ulVectorSize;

		// apply volume									 
		vVectRR = __vmulfp( vVectRR, vVolumesRR );
		
		// increment the volumes
		vVolumesRR  = __vaddfp( vVolumesRR,  vVolumesRRDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 __vperm()
//   6 __stvx()
//----------------------------------------------------------------------------------------------------
		register __vector4 vFrontLeftRight = __vperm( vVectFL, vVectFR,  m_vShuffle0101 );	// FL0, FL1, FR0, FR1
		register __vector4 vCenterLfe = __vperm( vVectC, vVectLFE, m_vShuffle0101 );		// C0, C1, LFE0, LFE1
		register __vector4 vRearLeftRight = __vperm( vVectRL, vVectRR,  m_vShuffle0101 );	// RL0, RL1, RR0, RR1

		// we want vDest1 = [ FL0, FR0, C0, LFE0 ]
		register __vector4 vDest1 = __vperm( vFrontLeftRight, vCenterLfe, m_vShuffle0202 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 0 );

		// we want vDest1 = [ RL0, RR0, FL1, FR1 ]
		vDest1 = __vperm( vRearLeftRight, vFrontLeftRight, m_vShuffle0213 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 16 );

		// we want vDest1 = [ C1, LFE1, RL1, RR1 ]
		vDest1 = __vperm( vCenterLfe, vRearLeftRight, m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 32 );

		vFrontLeftRight = __vperm( vVectFL, vVectFR,  m_vShuffle2323 );						// FL2, FL3, FR2, FR3
		vCenterLfe = __vperm( vVectC, vVectLFE, m_vShuffle2323 );							// C2, C3, LFE2, LFE3
		vRearLeftRight = __vperm( vVectRL, vVectRR,  m_vShuffle2323 );						// RL2, RL3, RR2, RR3

		// we want vDest1 = [ FL2, FR2, C2, LFE2 ]
		vDest1 = __vperm( vFrontLeftRight, vCenterLfe, m_vShuffle0202 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 48 );

		// we want vDest1 = [ RL2, RR2, FL3, FR3 ]
		vDest1 = __vperm( vRearLeftRight, vFrontLeftRight, m_vShuffle0213 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 64 );

		// we want vDest1 = [ C3, LFE3, RL3, RR3 ]
		vDest1 = __vperm( vCenterLfe, vRearLeftRight, m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 80 );

		in_pDestData += ulVectorSize * 6;
	}
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
inline void CAkMixer::VolumeInterleaved71(	AkReal32*					in_pSourceData,
											AkReal32*					in_pDestData,
											AkSpeakerVolumesSevenOne	in_Volumes71 )
{
	BUILD_VOLUME_VECTOR( vVolumesFL, in_Volumes71.fFrontLeft, in_Volumes71.fFrontLeftDelta );
	BUILD_VOLUME_VECTOR( vVolumesFR, in_Volumes71.fFrontRight, in_Volumes71.fFrontRightDelta );
	BUILD_VOLUME_VECTOR( vVolumesC, in_Volumes71.fCenter, in_Volumes71.fCenterDelta );
	BUILD_VOLUME_VECTOR( vVolumesLFE, in_Volumes71.fLfe, in_Volumes71.fLfeDelta );
	BUILD_VOLUME_VECTOR( vVolumesRL, in_Volumes71.fRearLeft, in_Volumes71.fRearLeftDelta );
	BUILD_VOLUME_VECTOR( vVolumesRR, in_Volumes71.fRearRight, in_Volumes71.fRearRightDelta );
	BUILD_VOLUME_VECTOR( vVolumesEL, in_Volumes71.fExtraLeft, in_Volumes71.fExtraLeftDelta );
	BUILD_VOLUME_VECTOR( vVolumesER, in_Volumes71.fExtraRight, in_Volumes71.fExtraRightDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes71.fFrontLeftDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesFLDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fFrontRightDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesFRDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fCenterDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesCDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fLfeDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesLFEDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fRearLeftDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesRLDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fRearRightDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesRRDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fExtraLeftDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesELDelta, fVolumesDelta, 0 );

	fVolumesDelta = in_Volumes71.fExtraRightDelta * 2;
	BUILD_VOLUME_VECTOR( vVolumesERDelta, fVolumesDelta, 0 );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_FRONTRIGHT );
	AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_CENTER );
	AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_LFE );
	AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_REARLEFT );
	AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_REARRIGHT );
	AkReal32* l_pSourceDataEL  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_SIDELEFT );
	AkReal32* l_pSourceDataER  = in_pSource->GetChannel( AK_IDX_SETUP_7POINT1_SIDERIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		__vector4 vVectFL = __lvx( in_pSourceData, 0 );
		in_pSourceData += ulVectorSize;
		// apply volume									 
		vVectFL = __vmulfp( vVectFL, vVolumesFL );
		// increment the volumes
		vVolumesFL  = __vaddfp( vVolumesFL,  vVolumesFLDelta );
		
		__vector4 vVectFR = __lvx( l_pSourceDataFR, 0 );
		l_pSourceDataFR += ulVectorSize;
		// apply volume									 
		vVectFR = __vmulfp( vVectFR, vVolumesFR );
		// increment the volumes
		vVolumesFR  = __vaddfp( vVolumesFR,  vVolumesFRDelta );

		__vector4 vVectC = __lvx( l_pSourceDataC, 0 );
		l_pSourceDataC += ulVectorSize;
		// apply volume									 
		vVectC = __vmulfp( vVectC, vVolumesC );
		// increment the volumes
		vVolumesC  = __vaddfp( vVolumesC,  vVolumesCDelta );

		__vector4 vVectLFE = __lvx( l_pSourceDataLFE, 0 );
		l_pSourceDataLFE += ulVectorSize;
		// apply volume									 
		vVectLFE = __vmulfp( vVectLFE, vVolumesLFE );
		// increment the volumes
		vVolumesLFE  = __vaddfp( vVolumesLFE,  vVolumesLFEDelta );

		__vector4 vVectRL = __lvx( l_pSourceDataRL, 0 );
		l_pSourceDataRL += ulVectorSize;
		// apply volume									 
		vVectRL = __vmulfp( vVectRL, vVolumesRL );
		
		// increment the volumes
		vVolumesRL  = __vaddfp( vVolumesRL,  vVolumesRLDelta );

		__vector4 vVectRR = __lvx( l_pSourceDataRR, 0 );
		l_pSourceDataRR += ulVectorSize;
		// apply volume									 
		vVectRR = __vmulfp( vVectRR, vVolumesRR );
		// increment the volumes
		vVolumesRR  = __vaddfp( vVolumesRR,  vVolumesRRDelta );

		__vector4 vVectEL = __lvx( l_pSourceDataEL, 0 );
		l_pSourceDataEL += ulVectorSize;
		// apply volume
		vVectEL = __vmulfp( vVectEL, vVolumesEL );
		// increment the volumes
		vVolumesEL  = __vaddfp( vVolumesEL, vVolumesELDelta );

		__vector4 vVectER = __lvx( l_pSourceDataER, 0 );
		l_pSourceDataER += ulVectorSize;
		// apply volume
		vVectER = __vmulfp( vVectER, vVolumesER );
		// increment the volumes
		vVolumesER  = __vaddfp( vVolumesER, vVolumesERDelta );
///----------------------------------------------------------------------------------------------------
// Interleave the data
//
// 16 __vperm()
//  8 __stvx()
//----------------------------------------------------------------------------------------------------
		// set up enough to build the first two frames
		register __vector4 vFrontLeftRight = __vperm( vVectFL, vVectFR,  m_vShuffle0101 );	// FL0, FL1, FR0, FR1
		register __vector4 vCenterLfe = __vperm( vVectC, vVectLFE, m_vShuffle0101 );		// C0, C1, LFE0, LFE1
		register __vector4 vRearLeftRight = __vperm( vVectRL, vVectRR,  m_vShuffle0101 );	// RL0, RL1, RR0, RR1
		register __vector4 vExtraLeftRight = __vperm( vVectEL, vVectER,  m_vShuffle0101 );	// EL0, EL1, ER0, ER1

		// we want vDest1 = [ FL0, FR0, C0, LFE0 ]
		register __vector4 vDest1 = __vperm( vFrontLeftRight, vCenterLfe, m_vShuffle0202);
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 0 );

		// we want vDest1 = [ RL0, RR0, EL0, ER0 ]
		vDest1 = __vperm( vRearLeftRight, vRearLeftRight, m_vShuffle0202 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 16 );

		// we want vDest1 = [ FL1, FR1, C1, LFE1 ]
		vDest1 = __vperm( vFrontLeftRight, vCenterLfe,  m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 32 );

		// we want vDest1 = [ RL1, RR1, EL1, ER1 ]
		vDest1 = __vperm( vRearLeftRight, vRearLeftRight, m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 48 );

		// set up enough to build the next two frames
		vFrontLeftRight = __vperm( vVectFL, vVectFR,  m_vShuffle2323 );						// FL2, FL3, FR2, FR3
		vCenterLfe = __vperm( vVectC, vVectLFE, m_vShuffle2323 );							// C2, C3, LFE2, LFE3
		vRearLeftRight = __vperm( vVectRL, vVectRR,  m_vShuffle2323 );						// RL2, RL3, RR2, RR3
		vExtraLeftRight = __vperm( vVectEL, vVectER,  m_vShuffle2323 );						// EL2, EL3, ER2, ER3

		// we want vDest1 = [ FL2, FR2, C2, LFE2 ]
		vDest1 = __vperm( vFrontLeftRight, vCenterLfe, m_vShuffle0202);
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 64 );

		// we want vDest1 = [ RL2, RR2, EL2, ER2 ]
		vDest1 = __vperm( vRearLeftRight, vRearLeftRight, m_vShuffle0202 );
		// store the vDest1 result
		__stvx( vDest1 ,in_pDestData, 80 );

		// we want vDest1 = [ FL3, FR3, C3, LFE3 ]
		vDest1 = __vperm( vFrontLeftRight, vCenterLfe,  m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1, in_pDestData, 96 );

		// we want vDest1 = [ RL3, RR3, ER2, ER3 ]
		vDest1 = __vperm( vRearLeftRight, vRearLeftRight, m_vShuffle1313 );
		// store the vDest1 result
		__stvx( vDest1,in_pDestData, 112 );

		in_pDestData += ulVectorSize * 8;
	}
}
#endif
//====================================================================================================
//====================================================================================================
void CAkMixer::MixNStereoPrev( AkAudioBufferFinalMix * in_pInputBuffer )
{
	AkSpeakerVolumesStereo Volumes( in_pInputBuffer->AudioMix, m_fOneOverNumFrames );

	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_2_LEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_2_LEFT ), Volumes.fLeft, Volumes.fLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_2_RIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_2_RIGHT ), Volumes.fRight, Volumes.fRightDelta );
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixN51Prev( AkAudioBufferFinalMix * in_pInputBuffer )
{
	AkSpeakerVolumesFiveOne Volumes( in_pInputBuffer->AudioMix, m_fOneOverNumFrames );

	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_CENTER ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_CENTER ), Volumes.fCenter, Volumes.fCenterDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_LFE ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_LFE ), Volumes.fLfe, Volumes.fLfeDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_REARLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_5_REARRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixN71Prev( AkAudioBufferMix * in_pInputBuffer )
{
	AkSpeakerVolumesSevenOne Volumes( in_pInputBuffer->AudioMix[0], m_fOneOverNumFrames );

	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_CENTER ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_CENTER ), Volumes.fCenter, Volumes.fCenterDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_LFE ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_LFE ), Volumes.fLfe, Volumes.fLfeDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_REARLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_REARRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_SIDELEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_SIDELEFT ), Volumes.fExtraLeft, Volumes.fExtraLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_SIDERIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7_SIDERIGHT ), Volumes.fExtraRight, Volumes.fExtraRightDelta );
}
#endif
//====================================================================================================
//====================================================================================================
void CAkMixer::MixN3DFiveOnePrev( AkAudioBufferMix * in_pInputBuffer )
{
	unsigned int uChannel = 0;
	unsigned int uNumChannels = in_pInputBuffer->NumChannels();
	do
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );
		
		AkSpeakerVolumesFiveOne Volumes( in_pInputBuffer->AudioMix[uChannel], m_fOneOverNumFrames );

		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_CENTER ), Volumes.fCenter, Volumes.fCenterDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_LFE ), Volumes.fLfe, Volumes.fLfeDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_5_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
	} while(++uChannel < uNumChannels );
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixN3DSevenOnePrev( AkAudioBufferMix * in_pInputBuffer )
{
	unsigned int uChannel = 0;
	unsigned int uNumChannels = in_pInputBuffer->NumChannels();
	do
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );
		
		AkSpeakerVolumesSevenOne Volumes( in_pInputBuffer->AudioMix[uChannel], m_fOneOverNumFrames );

		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_CENTER ), Volumes.fCenter, Volumes.fCenterDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_LFE ), Volumes.fLfe, Volumes.fLfeDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_SIDELEFT ), Volumes.fExtraLeft, Volumes.fExtraLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_SIDERIGHT ), Volumes.fExtraRight, Volumes.fExtraRightDelta );
	} while(++uChannel < uNumChannels);
}
#endif
//====================================================================================================
//====================================================================================================
void CAkMixer::MixN3DStereoPrev( AkAudioBufferMix * in_pInputBuffer )
{
	unsigned int uChannel=0;
	unsigned int uNumChannels = in_pInputBuffer->NumChannels();
	do
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );
		
		AkSpeakerVolumesStereo3D Volumes( in_pInputBuffer->AudioMix[uChannel], m_fOneOverNumFrames );
		
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_2_LEFT ), Volumes.fLeft,	Volumes.fLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_2_RIGHT ), Volumes.fRight, Volumes.fRightDelta );
	} while(++uChannel < uNumChannels );
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixAndInterleaveStereo( AkAudioBufferFinalMix * in_pInputBuffer )
{
	AkSpeakerVolumesStereo Volumes( in_pInputBuffer->AudioMix, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)m_pOutputBuffer->GetInterleavedData();
	VolumeInterleavedStereo( in_pInputBuffer, pDryOutSample, Volumes.fLeft, Volumes.fLeftDelta, Volumes.fRight, Volumes.fRightDelta );
}
//====================================================================================================
//====================================================================================================
void CAkMixer::MixAndInterleave51( AkAudioBufferFinalMix * in_pInputBuffer )
{
	AkSpeakerVolumesFiveOne Volumes( in_pInputBuffer->AudioMix, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)m_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved51( in_pInputBuffer, pDryOutSample, Volumes );
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixAndInterleave71( AkAudioBufferFinalMix * in_pInputBuffer )
{
	AkReal32* AK_RESTRICT pInSample = (AkReal32*)in_pInputBuffer->GetContiguousDeinterleavedData();

	AkSpeakerVolumesSevenOne Volumes( in_pInputBuffer->AudioMix, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)m_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved71( pInSample, pDryOutSample, Volumes );
}
#endif