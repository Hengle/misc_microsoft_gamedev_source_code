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
// AkMixerSIMD.cpp
//
//////////////////////////////////////////////////////////////////////
#include "xmmintrin.h"
static const AkUInt32	ulVectorSize = 4;

static __declspec(align(16)) AkReal32 aVolumes[4];
#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA ) \
	aVolumes[0] = VOLUME; \
	aVolumes[1] = VOLUME; \
	aVolumes[2] = VOLUME + VOLUME_DELTA; \
	aVolumes[3] = VOLUME + VOLUME_DELTA; \
	__m128 VECT = _mm_load_ps( aVolumes );
//====================================================================================================
//====================================================================================================
void CAkMixer::AddVolume( 
	AkReal32*	in_pSourceData, 
	AkReal32*	in_pDestData, 
	AkReal32	in_fVolume, 
	AkReal32	in_fVolumeDelta 
	)
{
	AKASSERT( !( m_usMaxFrames % 8 ) );

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
			// get eight samples								 
			__m128 vSum1 = _mm_load_ps( pSourceData );
			__m128 vSum2 = _mm_load_ps( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;

			// apply volume
			vSum1 = _mm_mul_ps( vSum1, vVolumes );
			vSum2 = _mm_mul_ps( vSum2, vVolumes );
			
			// get the previous ones						 
			__m128 vOutput1 = _mm_load_ps( pDestData );
			__m128 vOutput2 = _mm_load_ps( pDestData  + ulVectorSize );
			
			// add to output sample							 
			vSum1 = _mm_add_ps( vOutput1, vSum1 );
			vSum2 = _mm_add_ps( vOutput2, vSum2 );
			
			// store the result								 
			_mm_store_ps( pDestData, vSum1 );
			_mm_store_ps( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;
		}
		while ( pSourceData < pSourceEnd );
	}

	else // has volume delta
	{
		AkReal32 fVolumesDelta = in_fVolumeDelta * 2;
		__m128 vVolumesDelta = _mm_load1_ps( &fVolumesDelta );

		BUILD_VOLUME_VECTOR( vVolumes1, in_fVolume, in_fVolumeDelta );
		__m128 vVolumes2 = _mm_add_ps( vVolumes1, vVolumesDelta );

		// multiply volumes delta by 2 because the loop is unrolled by 2.
		vVolumesDelta = _mm_add_ps( vVolumesDelta, vVolumesDelta );

		do
		{
			// get eight samples								 
			__m128 vSum1 = _mm_load_ps( pSourceData );
			__m128 vSum2 = _mm_load_ps( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;

			// apply volume
			vSum1 = _mm_mul_ps( vSum1, vVolumes1 );
			vSum2 = _mm_mul_ps( vSum2, vVolumes2 );
			
			// get the previous ones						 
			__m128 vOutput1 = _mm_load_ps( pDestData );
			__m128 vOutput2 = _mm_load_ps( pDestData  + ulVectorSize );
			
			// add to output sample							 
			vSum1 = _mm_add_ps( vOutput1, vSum1 );
			vSum2 = _mm_add_ps( vOutput2, vSum2 );
			
			// store the result								 
			_mm_store_ps( pDestData, vSum1 );
			_mm_store_ps( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;

			// in_fVolume += in_fVolumeDelta;
			vVolumes1 = _mm_add_ps( vVolumes1, vVolumesDelta );
			vVolumes2 = _mm_add_ps( vVolumes2, vVolumesDelta );
		}
		while ( pSourceData < pSourceEnd );
	}
}
//====================================================================================================
//====================================================================================================
inline void CAkMixer::VolumeInterleavedStereo( 
	AkAudioBuffer*	in_pSource, 
	AkReal32*	in_pDestData, 
	AkReal32	in_fVolumeL, 
	AkReal32	in_fVolumeLDelta, 
	AkReal32	in_fVolumeR, 
	AkReal32	in_fVolumeRDelta 
	)
{
	BUILD_VOLUME_VECTOR( vVolumesL, in_fVolumeL, in_fVolumeLDelta );
	BUILD_VOLUME_VECTOR( vVolumesR, in_fVolumeR, in_fVolumeRDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_fVolumeLDelta * 2;
	__m128 vVolumesLDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_fVolumeRDelta * 2;
	__m128 vVolumesRDelta = _mm_load1_ps( &fVolumesDelta );

	AkReal32* l_pSourceDataL = in_pSource->GetChannel( AK_IDX_SETUP_2_LEFT );
	AkReal32* l_pSourceDataR = in_pSource->GetChannel( AK_IDX_SETUP_2_RIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		__m128 vVectL = _mm_load_ps( l_pSourceDataL );
		l_pSourceDataL += ulVectorSize;

		// apply volume									 
		vVectL = _mm_mul_ps( vVectL, vVolumesL );

		// increment the volumes
		vVolumesL  = _mm_add_ps( vVolumesL, vVolumesLDelta );
		
		__m128 vVectR = _mm_load_ps( l_pSourceDataR );
		l_pSourceDataR += ulVectorSize;

		// apply volume									 
		vVectR = _mm_mul_ps( vVectR, vVolumesR );
		
		// increment the volumes
		vVolumesR  = _mm_add_ps( vVolumesR, vVolumesRDelta );
		
		// vVectL = [ L3, L2, L1, L0] * volumes
		// vVectR = [ R3, R2, R1, R0] * volumes
		// we want vDest1 = [ R1, L1, R0, L0]
		__m128 vDest1 = _mm_unpacklo_ps( vVectL, vVectR );

		// we want vDest2 = [ R3, L3, R2, L2]
		__m128 vDest2 = _mm_unpackhi_ps( vVectL, vVectR );

		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// store the vDest2 result
		_mm_store_ps( in_pDestData, vDest2 );
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
	__m128 vVolumesFLDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes51.fFrontRightDelta * 2;
	__m128 vVolumesFRDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes51.fCenterDelta * 2;
	__m128 vVolumesCDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes51.fLfeDelta * 2;
	__m128 vVolumesLFEDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes51.fRearLeftDelta * 2;
	__m128 vVolumesRLDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes51.fRearRightDelta * 2;
	__m128 vVolumesRRDelta = _mm_load1_ps( &fVolumesDelta );

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

		// get four samples								 
		__m128 vVectFL = _mm_load_ps( l_pSourceDataFL );
		l_pSourceDataFL += ulVectorSize;

		// apply volume									 
		vVectFL = _mm_mul_ps( vVectFL, vVolumesFL );

		// increment the volumes
		vVolumesFL  = _mm_add_ps( vVolumesFL,  vVolumesFLDelta );
		
		__m128 vVectFR = _mm_load_ps( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;

		// apply volume									 
		vVectFR = _mm_mul_ps( vVectFR, vVolumesFR );
		
		// increment the volumes
		vVolumesFR  = _mm_add_ps( vVolumesFR,  vVolumesFRDelta );

		__m128 vVectC = _mm_load_ps( l_pSourceDataC );
		l_pSourceDataC += ulVectorSize;

		// apply volume									 
		vVectC = _mm_mul_ps( vVectC, vVolumesC );
		
		// increment the volumes
		vVolumesC  = _mm_add_ps( vVolumesC,  vVolumesCDelta );

		__m128 vVectLFE = _mm_load_ps( l_pSourceDataLFE );
		l_pSourceDataLFE += ulVectorSize;

		// apply volume									 
		vVectLFE = _mm_mul_ps( vVectLFE, vVolumesLFE );
		
		// increment the volumes
		vVolumesLFE  = _mm_add_ps( vVolumesLFE,  vVolumesLFEDelta );

		__m128 vVectRL = _mm_load_ps( l_pSourceDataRL );
		l_pSourceDataRL += ulVectorSize;

		// apply volume									 
		vVectRL = _mm_mul_ps( vVectRL, vVolumesRL );
		
		// increment the volumes
		vVolumesRL  = _mm_add_ps( vVolumesRL,  vVolumesRLDelta );

		__m128 vVectRR = _mm_load_ps( l_pSourceDataRR );
		l_pSourceDataRR += ulVectorSize;

		// apply volume									 
		vVectRR = _mm_mul_ps( vVectRR, vVolumesRR );
		
		// increment the volumes
		vVolumesRR  = _mm_add_ps( vVolumesRR,  vVolumesRRDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 _mm_shuffle_ps()
//   6 _mm_store_ps()
//----------------------------------------------------------------------------------------------------
		register __m128 vFrontLeftRight = _mm_shuffle_ps( vVectFL, vVectFR, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register __m128 vCenterLfe = _mm_shuffle_ps( vVectC, vVectLFE, _MM_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register __m128 vRearLeftRight = _mm_shuffle_ps( vVectRL, vVectRR, _MM_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		__m128 vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ FR1, FL1, RR0, RL0 ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vFrontLeftRight, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ RR1, RL1, LFE1, C1 ]
		vDest1 = _mm_shuffle_ps( vCenterLfe, vRearLeftRight, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = _mm_shuffle_ps( vVectFL, vVectFR, _MM_SHUFFLE( 3, 2, 3, 2 ) );	// FR3, FR2, FL3, FL2
		vCenterLfe = _mm_shuffle_ps( vVectC, vVectLFE, _MM_SHUFFLE( 3, 2, 3, 2 ) );			// LFE3, LFE2, C3, C2
		vRearLeftRight = _mm_shuffle_ps( vVectRL, vVectRR, _MM_SHUFFLE( 3, 2, 3, 2 ) );		// RR3, RR2, RL3, RL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ FR3, FL3, RR2, RL2 ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vFrontLeftRight, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ RR3, RL3, LFE3, C3 ]
		vDest1 = _mm_shuffle_ps( vCenterLfe, vRearLeftRight, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
	}
}
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
inline void CAkMixer::VolumeInterleaved71(	AkAudioBuffer *				in_pSource,
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
	__m128 vVolumesFLDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fFrontRightDelta * 2;
	__m128 vVolumesFRDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fCenterDelta * 2;
	__m128 vVolumesCDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fLfeDelta * 2;
	__m128 vVolumesLFEDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fRearLeftDelta * 2;
	__m128 vVolumesRLDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fRearRightDelta * 2;
	__m128 vVolumesRRDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fExtraLeftDelta * 2;
	__m128 vVolumesELDelta = _mm_load1_ps( &fVolumesDelta );

	fVolumesDelta = in_Volumes71.fExtraRightDelta * 2;
	__m128 vVolumesERDelta = _mm_load1_ps( &fVolumesDelta );

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
		__m128 vVectFL = _mm_load_ps( in_pSourceData );
		in_pSourceData += ulVectorSize;
		// apply volume									 
		vVectFL = _mm_mul_ps( vVectFL, vVolumesFL );
		// increment the volumes
		vVolumesFL  = _mm_add_ps( vVolumesFL,  vVolumesFLDelta );
		
		__m128 vVectFR = _mm_load_ps( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;
		// apply volume									 
		vVectFR = _mm_mul_ps( vVectFR, vVolumesFR );
		// increment the volumes
		vVolumesFR  = _mm_add_ps( vVolumesFR,  vVolumesFRDelta );

		__m128 vVectC = _mm_load_ps( l_pSourceDataC );
		l_pSourceDataC += ulVectorSize;
		// apply volume									 
		vVectC = _mm_mul_ps( vVectC, vVolumesC );
		// increment the volumes
		vVolumesC  = _mm_add_ps( vVolumesC,  vVolumesCDelta );

		__m128 vVectLFE = _mm_load_ps( l_pSourceDataLFE );
		l_pSourceDataLFE += ulVectorSize;
		// apply volume									 
		vVectLFE = _mm_mul_ps( vVectLFE, vVolumesLFE );
		// increment the volumes
		vVolumesLFE  = _mm_add_ps( vVolumesLFE,  vVolumesLFEDelta );

		__m128 vVectRL = _mm_load_ps( l_pSourceDataRL );
		l_pSourceDataRL += ulVectorSize;
		// apply volume									 
		vVectRL = _mm_mul_ps( vVectRL, vVolumesRL );
		// increment the volumes
		vVolumesRL  = _mm_add_ps( vVolumesRL,  vVolumesRLDelta );

		__m128 vVectRR = _mm_load_ps( l_pSourceDataRR );
		l_pSourceDataRR += ulVectorSize;
		// apply volume									 
		vVectRR = _mm_mul_ps( vVectRR, vVolumesRR );
		// increment the volumes
		vVolumesRR  = _mm_add_ps( vVolumesRR,  vVolumesRRDelta );

		__m128 vVectEL = _mm_load_ps( l_pSourceDataEL );
		l_pSourceDataEL += ulVectorSize;
		// apply volume									 
		vVectEL = _mm_mul_ps( vVectEL, vVolumesEL );
		// increment the volumes
		vVolumesEL  = _mm_add_ps( vVolumesEL,  vVolumesELDelta );

		__m128 vVectER = _mm_load_ps( l_pSourceDataER );
		l_pSourceDataER += ulVectorSize;
		// apply volume									 
		vVectER = _mm_mul_ps( vVectER, vVolumesER );
		// increment the volumes
		vVolumesER  = _mm_add_ps( vVolumesER,  vVolumesERDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 _mm_shuffle_ps()
//   6 _mm_store_ps()
//----------------------------------------------------------------------------------------------------
		register __m128 vFrontLeftRight = _mm_shuffle_ps( vVectFL, vVectFR, _MM_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register __m128 vCenterLfe = _mm_shuffle_ps( vVectC, vVectLFE, _MM_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register __m128 vRearLeftRight = _mm_shuffle_ps( vVectRL, vVectRR, _MM_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0
		register __m128 vExtraLeftRight = _mm_shuffle_ps( vVectEL, vVectER,  _MM_SHUFFLE( 1, 0, 1, 0 ) );	// ER1, EL1, ER0, EL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		register __m128 vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER0, EL0, RR0, RL0  ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vExtraLeftRight, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE1, C1, FR1, FL1 ]
		vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER1, EL1, RR1, RL1 ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vExtraLeftRight, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = _mm_shuffle_ps( vVectFL, vVectFR, _MM_SHUFFLE( 3, 2, 3, 2 ) );					// FR3, FR2, FL3, FL2
		vCenterLfe = _mm_shuffle_ps( vVectC, vVectLFE, _MM_SHUFFLE( 3, 2, 3, 2 ) );							// LFE3, LFE2, C3, C2
		vRearLeftRight = _mm_shuffle_ps( vVectRL, vVectRR, _MM_SHUFFLE( 3, 2, 3, 2 ) );						// RR3, RR2, RL3, RL2
		vExtraLeftRight = _mm_shuffle_ps( vVectEL, vVectER,  _MM_SHUFFLE( 3, 2, 3, 2 ) );					// ER3, EL2, ER3, EL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER2, EL2, RR2, RL2  ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vExtraLeftRight, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE3, C3, FR3, FL3 ]
		vDest1 = _mm_shuffle_ps( vFrontLeftRight, vCenterLfe, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER3, EL3, RR3, RL3 ]
		vDest1 = _mm_shuffle_ps( vRearLeftRight, vExtraLeftRight, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		_mm_store_ps( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
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

	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_FRONTRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_CENTER ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_CENTER ), Volumes.fCenter, Volumes.fCenterDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_LFE ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_LFE ), Volumes.fLfe, Volumes.fLfeDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_REARLEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_REARRIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_SIDELEFT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_SIDELEFT ), Volumes.fExtraLeft, Volumes.fExtraLeftDelta );
	AddVolume( in_pInputBuffer->GetChannel( AK_IDX_SETUP_7_SIDERIGHT ), m_pOutputBuffer->GetChannel( AK_IDX_SETUP_7POINT1_SIDERIGHT ), Volumes.fExtraRight, Volumes.fExtraRightDelta );
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
void CAkMixer::MixN3DFourPrev( AkAudioBufferMix * in_pInputBuffer )
{
	unsigned int uChannel = 0;
	unsigned int uNumChannels = in_pInputBuffer->NumChannels();
	do
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );

		AkSpeakerVolumesFour Volumes( in_pInputBuffer->AudioMix[uChannel], m_fOneOverNumFrames );

		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_4_FRONTLEFT ), Volumes.fFrontLeft, Volumes.fFrontLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_4_FRONTRIGHT ), Volumes.fFrontRight, Volumes.fFrontRightDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_4_REARLEFT ), Volumes.fRearLeft, Volumes.fRearLeftDelta );
		AddVolume( pInSample, m_pOutputBuffer->GetChannel( AK_IDX_SETUP_4_REARRIGHT ), Volumes.fRearRight, Volumes.fRearRightDelta );
	} while(++uChannel < uNumChannels );
}
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
