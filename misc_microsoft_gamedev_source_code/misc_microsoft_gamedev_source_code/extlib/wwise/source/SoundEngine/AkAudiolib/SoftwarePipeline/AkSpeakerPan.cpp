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
// AkSpeakerPan.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkSpeakerPan.h"
#include "AkSIMDSpeakerVolumes.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#include "AkMath.h"
//#include <tchar.h> //test traces

#define MIN_ANGLE_BETWEEN_SPEAKERS (PIOVERFOUR) //45 degrees

//----------------------------------------------------------------------------------------------------
// static members
//----------------------------------------------------------------------------------------------------
AkReal32 CAkSpeakerPan::m_fSin2[PAN_CIRCLE/2];

//====================================================================================================
//====================================================================================================
void CAkSpeakerPan::Init()
{
	for(int i=0; i < PAN_CIRCLE/2; ++i )
	{
		AkReal64 dblSin = sin((double) i * (PI/(PAN_CIRCLE/2)));
		m_fSin2[i] = (AkReal32) ( dblSin * dblSin );
	}
}

//====================================================================================================
// new algorithm, conversion to log
//====================================================================================================
static AkForceInline AkReal32 SinFromCos( AkReal32 in_fCosValue )
{
	// cos(x)^2 + sin(x)^2 = 1
	return sqrtf( 1.0f - in_fCosValue*in_fCosValue );
}

//Returns an angle in range [0, 2PI]
static AkForceInline void CartesianToPolar( AkReal32 in_fX, AkReal32 in_fY, AkReal32& io_rfAngle )
{
	//convert to polar coordinates
	if( in_fX == 0.0f )
	{
		if( in_fY >= 0.0f )
			io_rfAngle = PIOVERTWO;
		else
			io_rfAngle = 3*PIOVERTWO;
	}
	else 
	{
		io_rfAngle = atan2f(in_fY, in_fX);
		if( io_rfAngle < 0.0f )
			io_rfAngle += TWOPI;
	}
}

void CAkSpeakerPan::GetSpeakerVolumes( 
	AkReal32			in_fX,
	AkReal32			in_fY,
	AkReal32			in_fDivergenceCenter,
	AkReal32			in_fSpread,
	AkSIMDSpeakerVolumes* out_pVolumes,
	AkUInt32			in_uNumFullBandChannels )
{
	if ( in_uNumFullBandChannels == 0 )
		return;	// No full band channel: bail out.

	if( in_fSpread == 0.0f ||
		( in_fX == 0.0f && in_fY == 0.0f ) )
	{
		GetSpeakerVolumesdB( in_fX, in_fY, in_fDivergenceCenter, (AkSpeakerVolumes*)&out_pVolumes[0] );
		// All speakers get the same volume. To preserve power, add LinToDB(1/num_chan) dB (yes! volumes are in dB here).
		out_pVolumes[0].Add( 10 * AkMath::FastLog10( 1/(AkReal32)in_uNumFullBandChannels ) );
		for(AkUInt32 iChannel=1; iChannel<in_uNumFullBandChannels; iChannel++)
		{
			out_pVolumes[iChannel] = out_pVolumes[0];
		}
	}
	else
	{
		AkReal32 fTheta;
		CartesianToPolar( in_fX, in_fY, fTheta );

		//Spread is [0,100], we need to convert to [0,TWOPI]
		AkReal32 fTotalSpreadAngle = in_fSpread * TWOPI / 100.0f;
		AkReal32 fChannelSpreadAngle = (fTotalSpreadAngle / in_uNumFullBandChannels);
		AkReal32 fCurrChannelAngle = fTheta + ((in_uNumFullBandChannels/2) * fChannelSpreadAngle);
		if( !( in_uNumFullBandChannels & 1 ) ) //if we have an even number of channels (2,4,etc)
			fCurrChannelAngle -= (fChannelSpreadAngle / 2.0f);
		if(fCurrChannelAngle >= TWOPI)
			fCurrChannelAngle -= TWOPI;

		//Calculate the number of virtual points needed for each input channel (so the channels have a width to fill all speakers)
		AkUInt32 uNbVirtualPoints = (AkUInt32)ceilf( fChannelSpreadAngle / (MIN_ANGLE_BETWEEN_SPEAKERS * 2) ) + 1; //min 2 points
		AkReal32 fTotalAttenuationFactor = 1.0f / ( uNbVirtualPoints * in_uNumFullBandChannels );
		AkReal32 fVirtPtSpreadAngle = (fChannelSpreadAngle / uNbVirtualPoints);

		for(AkUInt32 iChannel=0; iChannel<in_uNumFullBandChannels; iChannel++)
		{
			AkReal32 fCurrVirtPtAngle = fCurrChannelAngle + ((uNbVirtualPoints/2) * fVirtPtSpreadAngle);
			if( !( uNbVirtualPoints & 1 ) ) //if we have an even number of virtual points (2,4,etc)
				fCurrVirtPtAngle -= (fVirtPtSpreadAngle / 2.0f);
			if(fCurrVirtPtAngle >= TWOPI)
				fCurrVirtPtAngle -= TWOPI;

			out_pVolumes->Set( 0.0f ); //initialize output values
			for(AkUInt32 uVirtualPoint=0; uVirtualPoint<uNbVirtualPoints; uVirtualPoint++)
			{
				AkInt32 iAngle = (AkInt32)( fCurrVirtPtAngle * (PAN_CIRCLE/2 / PI) + 0.5f );
				iAngle &= ( PAN_CIRCLE-1 );
				AddSpeakerVolumesPower( iAngle, in_fDivergenceCenter, &out_pVolumes->volumes );

				fCurrVirtPtAngle -= fVirtPtSpreadAngle;
				if(fCurrVirtPtAngle < 0.0f )
					fCurrVirtPtAngle += TWOPI;
			}

			out_pVolumes->Mul( fTotalAttenuationFactor );
			out_pVolumes->Sqrt(); //convert power back to a gain
			out_pVolumes->FastLinTodB();
			out_pVolumes++;

			fCurrChannelAngle -= fChannelSpreadAngle;
			if(fCurrChannelAngle < 0.0f )
				fCurrChannelAngle += TWOPI;
		}
	}
/*
	TCHAR strTemp[256];
	_stprintf( strTemp, L"Volumes = %f %f %f %f %f\n", 
		out_pVolumes->volumes.fFrontLeft, 
		out_pVolumes->volumes.fFrontRight, 
		out_pVolumes->volumes.fCenter, 
		out_pVolumes->volumes.fRearLeft, 
		out_pVolumes->volumes.fRearRight
		);
	OutputDebugString( strTemp );
*/	
}

void CAkSpeakerPan::GetSpeakerVolumesdB(
	AkReal32			in_fX,
	AkReal32			in_fY,
	AkReal32			in_fDivergenceCenter,
	AkSpeakerVolumes*	out_pVolumes )
{
	// Assuming that the coordinates have already been rotated 45 degrees clockwise through the matrix transform.
	out_pVolumes->fLfe = 0.0f;
	// PhM : change these as they become available
#ifdef AK_71AUDIO
	out_pVolumes->fExtraLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
	out_pVolumes->fExtraRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif

	AkReal32 fDistance2d = sqrtf( in_fX * in_fX + in_fY * in_fY );
	if ( fDistance2d == 0.0f )
	{
		// constant power all channels
		out_pVolumes->fCenter = AK_SAFE_MINIMUM_VOLUME_LEVEL;
		out_pVolumes->fFrontLeft = -6.0f;
		out_pVolumes->fFrontRight = -6.0f;
		out_pVolumes->fRearLeft = -6.0f;
		out_pVolumes->fRearRight = -6.0f;
	}
	else
	{
		AkReal32 cosx = in_fX / fDistance2d;
		AkReal32 sinx = in_fY / fDistance2d;

		out_pVolumes->fCenter = AK_SAFE_MINIMUM_VOLUME_LEVEL;

		if ( in_fX > 0.0f )
		{
			if ( in_fY > 0.0f )
			{
				if ( in_fDivergenceCenter > 0.0f )
				{
					AkReal32 doublecosx = 2 * cosx * cosx - 1.0f; // cos(2x) = 2 * cos^2( x ) - 1;
					AkReal32 fSqrtDivCenter = sqrtf( in_fDivergenceCenter );
					AkReal32 fSqrtOneMinusDivCenter = sqrtf( 1.0f - in_fDivergenceCenter );
					out_pVolumes->fCenter = AkMath::FastLinTodB( fSqrtDivCenter * SinFromCos( doublecosx ) );
					if ( in_fX > in_fY ) 
					{
						out_pVolumes->fFrontLeft = AkMath::FastLinTodB( fSqrtOneMinusDivCenter * sinx );
						out_pVolumes->fFrontRight = AkMath::FastLinTodB( sqrtf( ( 1.0f - in_fDivergenceCenter ) * cosx * cosx
							+ in_fDivergenceCenter * doublecosx * doublecosx ) );
					}
					else
					{
						out_pVolumes->fFrontLeft = AkMath::FastLinTodB( sqrtf( ( 1.0f - in_fDivergenceCenter ) * sinx * sinx
							+ in_fDivergenceCenter * doublecosx * doublecosx ) );
						out_pVolumes->fFrontRight = AkMath::FastLinTodB( fSqrtOneMinusDivCenter * cosx );
					}
				}
				else
				{
					out_pVolumes->fFrontLeft = AkMath::FastLinTodB( sinx );
					out_pVolumes->fFrontRight = AkMath::FastLinTodB( cosx );
				}

				out_pVolumes->fRearLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fRearRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
			}
			else
			{
				out_pVolumes->fFrontLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fFrontRight = AkMath::FastLinTodB( cosx );
				out_pVolumes->fRearLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fRearRight = AkMath::FastLinTodB( -sinx );
			}
		}
		else // in_fX <= 0.0f
		{
			if ( in_fY > 0.0f )
			{
				out_pVolumes->fFrontLeft = AkMath::FastLinTodB( sinx );
				out_pVolumes->fFrontRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fRearLeft = AkMath::FastLinTodB( -cosx );
				out_pVolumes->fRearRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
			}
			else
			{
				out_pVolumes->fFrontLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fFrontRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
				out_pVolumes->fRearLeft = AkMath::FastLinTodB( -cosx );
				out_pVolumes->fRearRight = AkMath::FastLinTodB( -sinx );
			}
		}
	}

//	TCHAR str[256];
//	_stprintf(str, L"%0.9f, %0.2f, %0.2f, %0.2f, %0.2f\n", 
//		out_pVolumes->fFrontLeft, out_pVolumes->fCenter, out_pVolumes->fFrontRight, out_pVolumes->fRearLeft, out_pVolumes->fRearRight );
//	OutputDebugString(str);
}

void CAkSpeakerPan::AddSpeakerVolumesPower(
	AkInt32 in_iAngle,
	AkReal32 in_fDivergenceCenter,
	AkSpeakerVolumes * out_pVolumes )
{
	// Assuming that the coordinates have already been rotated 45 degrees clockwise through the matrix transform.

	if ( in_iAngle >= (PAN_CIRCLE/2) )
	{
		// sin^2(x) + cos^2(x) = 1
		AkReal32 sin2x = m_fSin2[in_iAngle-(PAN_CIRCLE/2)];
		AkReal32 cos2x = 1.0f - sin2x;

		if ( in_iAngle >= (PAN_CIRCLE/4*3) )
		{
			out_pVolumes->fFrontRight += cos2x; 
		}
		else
		{
			out_pVolumes->fRearLeft += cos2x;
		}

		out_pVolumes->fRearRight += sin2x;
	}
	else
	{
		AkReal32 sin2x = m_fSin2[in_iAngle];
		AkReal32 cos2x = 1.0f - sin2x;

		if ( in_iAngle >= (PAN_CIRCLE/4) )
		{
			out_pVolumes->fFrontLeft += sin2x;
			out_pVolumes->fRearLeft += cos2x;
		}
		else
		{
			if ( in_fDivergenceCenter > 0.0f )
			{
				AkReal32 sin22x = m_fSin2[in_iAngle*2];	// sin^2(2x)
				AkReal32 cos22x = 1.0f - sin22x;			// cos^2(2x)

				out_pVolumes->fCenter += in_fDivergenceCenter * sin22x;
				if ( in_iAngle >= (PAN_CIRCLE/8 ) )
				{
					out_pVolumes->fFrontLeft += ( 1.0f - in_fDivergenceCenter ) * sin2x + in_fDivergenceCenter * cos22x;
					out_pVolumes->fFrontRight += ( 1.0f - in_fDivergenceCenter ) * cos2x;
				}
				else
				{
					out_pVolumes->fFrontLeft += ( 1.0f - in_fDivergenceCenter ) * sin2x;
					out_pVolumes->fFrontRight += ( 1.0f - in_fDivergenceCenter ) * cos2x + in_fDivergenceCenter * cos22x;
				}
			}
			else
			{
				out_pVolumes->fFrontLeft += sin2x;
				out_pVolumes->fFrontRight += cos2x;
			}
		}
	}

	//	TCHAR str[256];
	//	_stprintf(str, L"%0.9f, %0.2f, %0.2f, %0.2f, %0.2f\n", 
	//		out_pVolumes->fFrontLeft, out_pVolumes->fCenter, out_pVolumes->fFrontRight, out_pVolumes->fRearLeft, out_pVolumes->fRearRight );
	//	OutputDebugString(str);
}

void CAkSpeakerPan::GetSpeakerVolumes2DPan(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	bool				in_bIsPannerEnabled,
	AkChannelMask		in_uChannelMask,
	AkSIMDSpeakerVolumes*	out_pVolumes
	)
{
	// We care only about fullband channels.
	// Handling of LFE is always done outside.
	AkUInt32 uConfigNoLFE = ( in_uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY );
	switch ( uConfigNoLFE )
	{
	case 0:	// No full band channel: bail out.
		break;
	case AK_SPEAKER_SETUP_MONO:		// Mono
		CAkSpeakerPan::_GetSpeakerVolumes2DPan( in_fX, in_fY, in_fCenterPct, (AkSpeakerVolumes*)out_pVolumes );	
		break;
	case AK_SPEAKER_SETUP_STEREO:	// Stereo
		{
			memset( out_pVolumes, 0, sizeof(AkSIMDSpeakerVolumes) * 2 );
			out_pVolumes[0].volumes.fFrontLeft	= 1.0f;
			out_pVolumes[1].volumes.fFrontRight	= 1.0f;
			
			if ( in_bIsPannerEnabled )
			{
				// Fold back in rear channels.
				out_pVolumes[0].volumes.fRearLeft	= 1.0f;
				out_pVolumes[1].volumes.fRearRight	= 1.0f;

				// Compute receipe for even number of channels: use standard 2D pan with center% = 0.
				in_fCenterPct = 0;			
				AkSIMDSpeakerVolumes tempAudioMix;
				CAkSpeakerPan::_GetSpeakerVolumes2DPan( in_fX, in_fY, in_fCenterPct, &( tempAudioMix.volumes ) );

				out_pVolumes[0].Mul( tempAudioMix );
				out_pVolumes[1].Mul( tempAudioMix );
			}
			// else Direct speaker assignment: return volumes as is.
		}
		break;
	case AK_SPEAKER_SETUP_3STEREO: 
		{
			// Whether we are 2D panning or not, speakers contribute evenly.
			memset( out_pVolumes, 0, sizeof(AkSIMDSpeakerVolumes) * 3 );
			out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fFrontLeft	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fFrontRight	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fCenter		= 1.0f;
			
			if ( in_bIsPannerEnabled )
			{
				// Fold back in rear channels. 
				out_pVolumes[AK_IDX_SETUP_3_LEFT].volumes.fRearLeft		= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_RIGHT].volumes.fRearRight	= 1.0f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearLeft	= 0.5f;
				out_pVolumes[AK_IDX_SETUP_3_CENTER].volumes.fRearRight	= 0.5f;

				// Compute receipe for odd number of channels.
				AkSIMDSpeakerVolumes tempAudioMix;
				CAkSpeakerPan::_GetSpeakerVolumes2DPanSourceHasCenter( in_fX, in_fY, &( tempAudioMix.volumes ) );
				
				out_pVolumes[0].Mul( tempAudioMix );
				out_pVolumes[1].Mul( tempAudioMix );
				out_pVolumes[2].Mul( tempAudioMix );
			}

			return;
		}
		break;
	case AK_SPEAKER_SETUP_4: 
		{
			// Whether we are 2D panning or not, speakers contribute evenly.
			memset( out_pVolumes, 0, sizeof(AkSIMDSpeakerVolumes) * 4 );
			out_pVolumes[AK_IDX_SETUP_4_FRONTLEFT].volumes.fFrontLeft	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_4_FRONTRIGHT].volumes.fFrontRight	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_4_REARLEFT].volumes.fRearLeft		= 1.0f;
			out_pVolumes[AK_IDX_SETUP_4_REARRIGHT].volumes.fRearRight	= 1.0f;
			
			if ( in_bIsPannerEnabled )
			{
				// Compute receipe for even number of channels: use standard 2D pan with center% = 0.
				in_fCenterPct = 0;			
				AkSIMDSpeakerVolumes tempAudioMix;
				CAkSpeakerPan::_GetSpeakerVolumes2DPan( in_fX, in_fY, in_fCenterPct, &( tempAudioMix.volumes ) );

				out_pVolumes[0].Mul( tempAudioMix );
				out_pVolumes[1].Mul( tempAudioMix );
				out_pVolumes[2].Mul( tempAudioMix );
				out_pVolumes[3].Mul( tempAudioMix );
			}
		}
		break;
	case AK_SPEAKER_SETUP_5:
		{
			// Whether we are 2D panning or not, speakers contribute evenly.
			memset( out_pVolumes, 0, sizeof(AkSIMDSpeakerVolumes) * 5 );
			out_pVolumes[AK_IDX_SETUP_5_FRONTLEFT].volumes.fFrontLeft	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_5_FRONTRIGHT].volumes.fFrontRight	= 1.0f;
			out_pVolumes[AK_IDX_SETUP_5_CENTER].volumes.fCenter			= 1.0f;
			out_pVolumes[AK_IDX_SETUP_5_REARLEFT].volumes.fRearLeft		= 1.0f;
			out_pVolumes[AK_IDX_SETUP_5_REARRIGHT].volumes.fRearRight	= 1.0f;

			if ( in_bIsPannerEnabled )
			{
				// Compute receipe for odd number of channels.
				AkSIMDSpeakerVolumes tempAudioMix;
				CAkSpeakerPan::_GetSpeakerVolumes2DPanSourceHasCenter( in_fX, in_fY, &( tempAudioMix.volumes ) );
				
				out_pVolumes[0].Mul( tempAudioMix );
				out_pVolumes[1].Mul( tempAudioMix );
				out_pVolumes[2].Mul( tempAudioMix );
				out_pVolumes[3].Mul( tempAudioMix );
				out_pVolumes[4].Mul( tempAudioMix );
			}
		}
		break;
	default:
		AKASSERT( !"Number of channels not supported" );
		break;
	}
}

void CAkSpeakerPan::_GetSpeakerVolumes2DPan(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkReal32			in_fCenterPct,	// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
//use linear power interpolation with new Center% implementation

	AkReal32 Left;
	AkReal32 Right;	

	Left		= ( 1.0f - in_fX )	* in_fY;
	Right		= in_fX				* in_fY;

	AkReal32 Center;
	if( in_fCenterPct > 0.0f )
	{
		if( in_fX >= 0.5 ) 
		{
			AkReal32 TmpC		= 2.0f * ( 1.0f - in_fX )		* in_fY;
			AkReal32 TmpR_withC	= ( 2.0f * ( in_fX - 0.5f ) )	* in_fY;

			Center		= in_fCenterPct * TmpC;
			Left		= Left  * ( 1.0f - in_fCenterPct );
			Right		= Right * ( 1.0f - in_fCenterPct ) + in_fCenterPct * TmpR_withC;
		}
		else
		{
			AkReal32 TmpC		= 2.0f * in_fX					* in_fY;
			AkReal32 TmpL_withC	= ( 1.0f - ( 2.0f * in_fX )	)	* in_fY;

 			Center		= in_fCenterPct * TmpC;
			Left		= Left  * ( 1.0f - in_fCenterPct ) + in_fCenterPct * TmpL_withC;
			Right		= Right * ( 1.0f - in_fCenterPct );
		}
	}
	else
	{
		Center		= 0.0f;
	}

	AkReal32 RearLeft	= ( 1.0f - in_fX )	* ( 1.0f - in_fY );
	AkReal32 RearRight	= in_fX				* ( 1.0f - in_fY );

	out_pVolumes->fLfe	= 0.0f;

	//convert power to speaker gains
	out_pVolumes->fCenter = sqrtf( Center );
	out_pVolumes->fFrontLeft = sqrtf( Left );
	out_pVolumes->fFrontRight = sqrtf( Right );
	out_pVolumes->fRearLeft = sqrtf( RearLeft );
	out_pVolumes->fRearRight = sqrtf( RearRight );

#ifdef AK_71AUDIO
	out_pVolumes->fExtraLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
	out_pVolumes->fExtraRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
}

void CAkSpeakerPan::_GetSpeakerVolumes2DPanSourceHasCenter(
	AkReal32			in_fX,			// [0..1]
	AkReal32			in_fY,			// [0..1]
	AkSpeakerVolumes*	out_pVolumes
	)
{
//use linear power interpolation without Center%: center is part of the interpolated channels.

	// Balance left-right.
	AkReal32 fFrontLeft = ( 2 - 2*in_fX )/3.f;
	AkReal32 fFrontCenter = 1/3.f;
	AkReal32 fFrontRight = 1 - ( fFrontLeft + fFrontCenter );

	AkReal32 fRearRight = in_fX;
	AkReal32 fRearLeft = 1 - in_fX;

	// Balance front-rear.
	// Note: Because our pad is square but the channels are not symmetric, the linear interpolation
	// has a different slope whether it is in the top or bottom half. At y = 0.5, power is evenly distributed.
	AkReal32 fRearBalance = ( in_fY >= 0.5 ) ? ( ( 4 - 4 * in_fY ) / 5.f ) : ( 1 - 6 * in_fY / 5.f );
	AkReal32 fFrontBalance = 1 - fRearBalance;

	fFrontLeft *= fFrontBalance;
	fFrontCenter *= fFrontBalance;
	fFrontRight *= fFrontBalance;	

	fRearLeft *= fRearBalance;
	fRearRight *= fRearBalance;

	AKASSERT( fFrontLeft + fFrontCenter + fFrontRight + fRearLeft + fRearRight > 1 - 0.00001 
			&& fFrontLeft + fFrontCenter + fFrontRight + fRearLeft + fRearRight < 1 + 0.00001 );

	out_pVolumes->fLfe	= 0.0f;

	//convert power to speaker gains
	out_pVolumes->fCenter = sqrtf( fFrontCenter );
	out_pVolumes->fFrontLeft = sqrtf( fFrontLeft );
	out_pVolumes->fFrontRight = sqrtf( fFrontRight );
	out_pVolumes->fRearLeft = sqrtf( fRearLeft );
	out_pVolumes->fRearRight = sqrtf( fRearRight );

#ifdef AK_71AUDIO
	out_pVolumes->fExtraLeft = AK_SAFE_MINIMUM_VOLUME_LEVEL;
	out_pVolumes->fExtraRight = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
}

