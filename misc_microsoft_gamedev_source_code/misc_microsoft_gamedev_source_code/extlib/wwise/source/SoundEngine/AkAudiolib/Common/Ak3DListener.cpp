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
// Ak3DListener.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMath.h"
#include "Ak3DListener.h"
#include "AudiolibDefs.h"
#include "AkDefault3DParams.h"
#include "AkSpeakerPan.h"
#include "AkPBI.h"
#include "AkEnvironmentsMgr.h"
#include "AkGen3DParams.h"
#include "AkFeedbackMgr.h"

#include <math.h>

extern AkMemPoolId		g_LEngineDefaultPoolId;

CAkListener* g_pAkListener = NULL;

ListenerData CAkListener::m_listeners[ AK_NUM_LISTENERS ];
AkUInt32 CAkListener::m_uAudioMask = 0xFFFFFFFF;	//All are active in audio by default
AkUInt32 CAkListener::m_uFeedbackMask = 0;			//Listeners don't receive feedback by default.

AkReal32 CAkListener::m_matRot[3][3] = // 45 degrees around Y rotation matrix for 5.1 speaker config calculations
{
	{ 0.70710678118654752440084436210485f, 0, 0.70710678118654752440084436210485f },
	{ 0, 1, 0 },
	{ -0.70710678118654752440084436210485f, 0, 0.70710678118654752440084436210485f }
};

//====================================================================================================
//====================================================================================================
AKRESULT CAkListener::Init()
{
	// initialise them
	for(AkUInt32 i = 0 ; i < AK_NUM_LISTENERS ; ++i)
	{
		ListenerData* pListener = m_listeners + i;

		// initialise audiolib listener params
		pListener->Listener.Pos = g_DefaultListenerPosition;

		AkReal32 matTmp[3][3];
		AkReal32* pFloat = &( matTmp[0][0] );

		*pFloat++ = AK_DEFAULT_LISTENER_SIDE_X;		// [0][0]
		*pFloat++ = AK_DEFAULT_LISTENER_SIDE_Y;		// [0][1]
		*pFloat++ = AK_DEFAULT_LISTENER_SIDE_Z;		// [0][2]

		*pFloat++ = AK_DEFAULT_LISTENER_TOP_X;		// [1][0]
		*pFloat++ = AK_DEFAULT_LISTENER_TOP_Y;		// [1][1]
		*pFloat++ = AK_DEFAULT_LISTENER_TOP_Z;		// [1][2]

		*pFloat++ = AK_DEFAULT_LISTENER_FRONT_X;	// [2][0]
		*pFloat++ = AK_DEFAULT_LISTENER_FRONT_Y;	// [2][1]
		*pFloat++ = AK_DEFAULT_LISTENER_FRONT_Z;	// [2][2]

		AkMath::MatrixMul3by3( m_matRot, matTmp, pListener->Matrix );

		pListener->Listener.VolumeOffset.Zero();
		pListener->Listener.bSpatialized = true;
	}

	m_uFeedbackMask = 0;			
	m_uAudioMask = 0xFFFFFFFF;		//All are active in audio by default

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkListener::Term()
{
	return AK_Success;
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkListener::SetListenerPosition( AkUInt32 in_uListener, const AkListenerPosition & in_Position )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	ListenerData* pListener = m_listeners + in_uListener;

	AkVector	Front,Top;
	AkReal32		fDot;

	// don't mess with the input vectors
	Front = in_Position.OrientationFront;
	Top = in_Position.OrientationTop;

	pListener->Listener.Pos.Position = in_Position.Position;

	// adjust front anyway as chances are we will rarely
	// get fDot spot on 0.0f
	fDot = AkMath::DotProduct(Front,Top);

	Front.X -= Top.X * fDot;
	Front.Y -= Top.Y * fDot;
	Front.Z -= Top.Z * fDot;

	// normalise them
	AkMath::Normalise(Front);
	AkMath::Normalise(Top);
	pListener->Listener.Pos.OrientationFront = Front;
	pListener->Listener.Pos.OrientationTop = Top;
	AkVector OrientationSide = AkMath::CrossProduct(Top,Front);

	AkReal32 matTmp[3][3];
	AkReal32* pFloat = &( matTmp[0][0] );

	*pFloat++ = OrientationSide.X;	// [0][0]
	*pFloat++ = OrientationSide.Y;	// [0][1]
	*pFloat++ = OrientationSide.Z;	// [0][2]

	*pFloat++ = pListener->Listener.Pos.OrientationTop.X;	// [1][0]
	*pFloat++ = pListener->Listener.Pos.OrientationTop.Y;	// [1][1]
	*pFloat++ = pListener->Listener.Pos.OrientationTop.Z;	// [1][2]

	*pFloat++ = pListener->Listener.Pos.OrientationFront.X;	// [2][0]
	*pFloat++ = pListener->Listener.Pos.OrientationFront.Y;	// [2][1]
	*pFloat++ = pListener->Listener.Pos.OrientationFront.Z;	// [2][2]

	AkMath::MatrixMul3by3( m_matRot, matTmp, pListener->Matrix );

	return AK_Success;
}

AKRESULT CAkListener::GetListenerPosition( AkUInt32 in_uListener, AkListenerPosition& out_rPosition )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	out_rPosition = m_listeners[ in_uListener ].Listener.Pos;
	return AK_Success;
}

AKRESULT CAkListener::SetListenerSpatialization( AkUInt32 in_uListener, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	m_listeners[ in_uListener ].Listener.bSpatialized = in_bSpatialized;

	if ( in_pVolumeOffsets )
	{
		AKASSERT( !in_bSpatialized && "Volume Offset not taken into account for spatialized listeners" );
		m_listeners[ in_uListener ].Listener.VolumeOffset = *in_pVolumeOffsets;
	}

	return AK_Success;
}

AKRESULT CAkListener::GetListenerSpatialization( AkUInt32 in_uListener, bool& out_rbSpatialized, AkSpeakerVolumes& out_rVolumeOffsets )
{
	if( in_uListener >= AK_NUM_LISTENERS )
		return AK_InvalidParameter;

	out_rbSpatialized  = m_listeners[ in_uListener ].Listener.bSpatialized;
	out_rVolumeOffsets = m_listeners[ in_uListener ].Listener.VolumeOffset.volumes;

	return AK_Success;
}
//====================================================================================================
//====================================================================================================
void CAkListener::TransformSoundPosition(
	const AkVector & in_vecListenerPos,
	const AkVector & in_vecPosition,
	AkReal32* AK_RESTRICT in_pMatrix,
	AkVector* AK_RESTRICT pout_Transformed )
{
	AkReal32 fTmpX = in_vecPosition.X - in_vecListenerPos.X;
	AkReal32 fTmpY = in_vecPosition.Y - in_vecListenerPos.Y;
	AkReal32 fTmpZ = in_vecPosition.Z - in_vecListenerPos.Z;

	AkReal32* AK_RESTRICT pFloat = in_pMatrix;

	// x' = m[0][0].x + m[0][1].y + m[0][2].z
	pout_Transformed->X  = pFloat[0] * fTmpX
						 + pFloat[1] * fTmpY
						 + pFloat[2] * fTmpZ;

/*	ONLY X and Z are currently used since positioning does not currently rely on height
	// y' = m[1][0].x + m[1][1].y + m[1][2].z
	pout_Transformed->Y  = pFloat[3] * fTmpX
						 + pFloat[4] * fTmpY
						 + pFloat[5] * fTmpZ;
*/
	// z' = m[2][0].x + m[2][1].y + m[2][2].z
	pout_Transformed->Z  = pFloat[6] * fTmpX
						 + pFloat[7] * fTmpY
						 + pFloat[8] * fTmpZ;
}
//====================================================================================================
// this returns a value between 0 (inside inner angle) and 1 (ouside outter angle)
//====================================================================================================
AkReal32 CAkListener::GetConeValue(
		const AkVector & in_Direction, // Normalized object->listener direction vector
		const AkVector & in_Orientation,
		const ConeParams& in_ConeParams )
{
	// compute angle between direction of sound and the [listener,sound] vector
	AkReal32 fAngle = AkMath::DotProduct( in_Direction, in_Orientation );
	// PhM : we can sometime get something slightly over 1.0
	if ( fAngle > 1.0f ) fAngle = 1.0f;
	else if ( fAngle < -1.0f ) fAngle = -1.0f;

	fAngle = AkMath::ACos(fAngle);

	// figure out what the cone will provide
	return AkMath::Interpolate( in_ConeParams.fInsideAngle, 0.0f, in_ConeParams.fOutsideAngle, 1.0f, fAngle );
}

//====================================================================================================
// Determines if the specified listener should listen to Audio and/or Feedback data.
//====================================================================================================
void CAkListener::SetListenerPipeline(
		AkUInt32		in_uListener,	//Listener ID.
		bool			in_bAudio,		//Is this listener used for audio data?
		bool			in_bFeedback)	//Is this listener used for feedback data?
{
	AkUInt32 uMask = 1 << in_uListener;
	m_uAudioMask = m_uAudioMask & ~uMask;
	if (in_bAudio)
		m_uAudioMask = m_uAudioMask | uMask;

	m_uFeedbackMask = m_uFeedbackMask & ~uMask;
	if (in_bFeedback)
		m_uFeedbackMask = m_uFeedbackMask | uMask;
}

//====================================================================================================
//====================================================================================================
void CAkListener::Get3DVolumes( CAkPBI*				in_pContext,
							    AkSoundParams&		in_params,
							    AkSoundPositioningParams& in_posParams,
								AkChannelMask		in_uChannelMask,
							    AkAudioMix*			out_pAudioMixWet,
								AkAudioMix*			out_pAudioMixDry, //with obs
								AkReal32*           out_pObsLPF,
								bool				in_bEnvBus )
{
	// Discard LFE, consider only fullband channels.
	AkUInt32 uNumChannels = AK::GetNumChannels( in_uChannelMask );
	bool bSourceHasLFE = AK::HasLFE( in_uChannelMask );
	AkUInt32 uNumFullBandChannels = ( bSourceHasLFE ) ? uNumChannels - 1 : uNumChannels;

	// Get volumes coming from actor-mixer structure (clamp to 0 db).
	AkReal32 fVolumeAM = AkMath::Min( in_params.Volume, 0.0f );
#ifdef AK_LFECENTER
	AkReal32 fLfeAM = AkMath::Min( in_params.LFE, 0.0f );
#endif

	AkReal32 fListenerVolumeDry;
	AkReal32 fListenerVolumeWet;

	// get the sound type
	AkPositioningType	eType = in_posParams.ePosType;

	AkReal32 fMinCone = 1.0f;
	AkReal32 fMinDist = AK_UPPER_MAX_DISTANCE;

	CAkGen3DParams * 	pGen3DParams = in_pContext->Get3DSound();
	CAkAttenuation *	pAttenuation = pGen3DParams->GetParams()->GetAttenuation();
	AkReal32			fDistance = 0.0f;

	// get a set of volumes relative to the location of the sound
	CAkAttenuation::AkAttenuationCurve* pSpreadCurve = NULL;
	CAkAttenuation::AkAttenuationCurve* pLPFCurve = NULL;
#ifdef AK_LFECENTER
	CAkAttenuation::AkAttenuationCurve* pLFECurve = NULL;
#endif
	if( pAttenuation )
	{
		pSpreadCurve  = pAttenuation->GetCurve( AttenuationCurveID_Spread );
		pLPFCurve	  = pAttenuation->GetCurve( AttenuationCurveID_LowPassFilter );
#ifdef AK_LFECENTER
		pLFECurve	  = pAttenuation->GetCurve( AttenuationCurveID_LFE );
#endif
	}
	AkReal32 fSpread = 0.0f;
#ifdef AK_LFECENTER
	AkVolumeValue fLFEVolume = AK_MAXIMUM_VOLUME_LEVEL;
#endif

	// 3D Game Defined
	if( eType == Ak3DGameDef )
	{
		AkReal32 fMinOccVal = 1.0f;
		AkReal32 fMinObsVal = 1.0f;

		for(AkUInt32 iChannel=0; iChannel<uNumChannels; iChannel++)
		{
			out_pAudioMixWet[iChannel].Next.Set( AK_SAFE_MINIMUM_VOLUME_LEVEL );
			out_pAudioMixDry[iChannel].Next.Set( AK_SAFE_MINIMUM_VOLUME_LEVEL );
		}

		AkFeedbackParams * pFeedbackParams = in_pContext->GetFeedbackParameters();

		AkUInt32 uMask = in_posParams.uListenerMask;
		for ( AkUInt32 uListener = 0; uMask; ++uListener, uMask >>= 1 )
		{
			if ( !( uMask & 1 ) )
				continue; // listener not active for this sound

			ListenerData & listener = m_listeners[ uListener ];

			GetListenerVolume( pGen3DParams, in_posParams.Position, listener.Listener.Pos.Position, fMinCone, fDistance, fListenerVolumeDry, fListenerVolumeWet );
			fMinDist = AkMath::Min( fMinDist, fDistance );

			// Occlusion
			AkReal32 fOccValue = in_pContext->GetOcclusionValue( uListener );
			AkReal32 fOccVolume = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveVol, fOccValue * 100 );

			fMinOccVal = AkMath::Min( fMinOccVal, fOccValue );

			// Obstruction
			AkReal32 fObsValue = in_pContext->GetObstructionValue( uListener );
			AkReal32 fObsVolume = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveVol, fObsValue * 100 );

			fMinObsVal = AkMath::Min( fMinObsVal, fObsValue );

			// Check corresponding values in tables (Volume and LPF)

			AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];

			if ( listener.Listener.bSpatialized )
			{
				if( pGen3DParams->GetParams()->m_bIsSpatialized )
				{
					AkVector vTransformed;
					TransformSoundPosition( 
						listener.Listener.Pos.Position, 
						in_posParams.Position.Position,
						&listener.Matrix[0][0],
						&vTransformed );

					if ( pSpreadCurve )
						fSpread = pSpreadCurve->Convert( fDistance );

					if ( pLPFCurve )
						in_params.LPF = AkMath::Max( in_params.LPF, pLPFCurve->Convert( fDistance ) );

#ifdef AK_LFECENTER
					if ( pLFECurve )
						fLFEVolume = pLFECurve->Convert( fDistance );
#endif

					CAkSpeakerPan::GetSpeakerVolumes( 
						vTransformed.X,
						vTransformed.Z,
						in_posParams.fDivergenceCenter,
						fSpread,
						volumes,
						uNumFullBandChannels );
				}
				else //3D sound is not spatialized, use 2D panner
				{
					CAkSpeakerPan::GetSpeakerVolumes2DPan( 0.5f, 0.5f, 
						in_posParams.fDivergenceCenter, 
						true,
						in_uChannelMask,
						volumes );
					for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
						volumes[iChannel].FastLinTodB();
				}
			}
			else
			{
				for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
					volumes[iChannel] = listener.Listener.VolumeOffset; //results in mono sound, positioned as the user wanted
			}
			
			CAkFeedbackDeviceMgr* pFeedbackMgr = CAkFeedbackDeviceMgr::Get();
		
			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				AkSIMDSpeakerVolumes volumeDry = volumes[iChannel]; 

				//Wet
				volumes[iChannel].Add( fOccVolume + fListenerVolumeWet );
				
				//Dry

				// If we are using these volumes for feedback, keep the attenuation and position volume separate.
				// We will normalize the position volumes and recombine the attenuation later (see AkFeedbackMgr::GetPlayerVolumes())
				// Each player has a listener assigned and they will have different positions therefore different volumes.							
				if (pFeedbackParams != NULL && (m_uFeedbackMask & (1 << uListener)) != 0)
				{
					AkUInt8 uPlayers = pFeedbackMgr->ListenerToPlayer((AkUInt8)uListener);
					if ( uPlayers != 0 )
					{
						for (AkUInt8 iBit = 1, iPlayer = 0; iPlayer < AK_MAX_PLAYERS; iPlayer++, iBit <<= 1)
						{
							if (iBit & uPlayers)
							{
								pFeedbackParams->m_fNextAttenuation[iPlayer] = AkMath::dBToLin(fOccVolume + fObsVolume + fListenerVolumeDry);
								AkUInt32 iIndex = pFeedbackParams->VolumeIndex(AkFeedbackParams::NextVolumes, iPlayer, iChannel);
								pFeedbackParams->m_Volumes[iIndex] = volumeDry;
								pFeedbackParams->m_Volumes[iIndex].dBToLin();
							}
						}
					}
				}

				volumeDry.Add( fObsVolume + fOccVolume + fListenerVolumeDry );

				//Here check if the volumes go to audio or feedback
				if (!in_pContext->IsForFeedbackPipeline() && (m_uAudioMask & (1 << uListener)) != 0)
				{
					out_pAudioMixWet[iChannel].Next.Max( volumes[iChannel] );
					out_pAudioMixDry[iChannel].Next.Max( volumeDry );
				}
			}

			// Handle LFE separately.
#ifdef AK_LFECENTER
			if ( bSourceHasLFE )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				out_pAudioMixWet[uChanLFE].Next.volumes.fLfe = AkMath::Max( fOccVolume + fLFEVolume, out_pAudioMixWet[uChanLFE].Next.volumes.fLfe );
				out_pAudioMixDry[uChanLFE].Next.volumes.fLfe = AkMath::Max( fOccVolume + fObsVolume + fLFEVolume, out_pAudioMixDry[uChanLFE].Next.volumes.fLfe );
			}
#endif
		}

		// Prepare out_pAudioMixes: add hierarchy volume, convert to linear, add dry level ratio.
		if ( in_bEnvBus )
		{
			// Prepare dry and wet mixes.
			AkReal32 fDryLevel = in_pContext->GetDryLevelValue();

			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				// Dry.
				out_pAudioMixDry[iChannel].Next.Add( fVolumeAM );
#ifdef AK_LFECENTER
				out_pAudioMixDry[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixDry[iChannel].Next.dBToLin();
				out_pAudioMixDry[iChannel].Next.Mul( fDryLevel );

				// Wet.
				out_pAudioMixWet[iChannel].Next.Add( fVolumeAM );
#ifdef AK_LFECENTER
				out_pAudioMixWet[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixWet[iChannel].Next.dBToLin();
			}

#ifdef AK_LFECENTER
			// Handle LFE separately.
			if ( bSourceHasLFE )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				out_pAudioMixWet[uChanLFE].Next.volumes.fLfe += fLfeAM;
				out_pAudioMixWet[uChanLFE].Next.dBToLin();
				out_pAudioMixDry[uChanLFE].Next.volumes.fLfe += fLfeAM;
				out_pAudioMixDry[uChanLFE].Next.dBToLin();
				out_pAudioMixDry[uChanLFE].Next.Mul( fDryLevel );
			}
#endif
		}
		else
		{
			// Prepare dry mix only, avoid multiplying with dry level.
			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				// Dry.
				out_pAudioMixDry[iChannel].Next.Add( fVolumeAM );
#ifdef AK_LFECENTER
				out_pAudioMixDry[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixDry[iChannel].Next.dBToLin();
			}

#ifdef AK_LFECENTER
			// Handle LFE separately.
			if ( bSourceHasLFE )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				out_pAudioMixDry[uChanLFE].Next.volumes.fLfe += fLfeAM;
				out_pAudioMixDry[uChanLFE].Next.dBToLin();
			}
#endif
		}

		AkReal32 fOccLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveOcc, CAkEnvironmentsMgr::CurveLPF, fMinOccVal * 100 );
		in_params.LPF = AkMath::Max( in_params.LPF, fOccLPF );

		*out_pObsLPF = g_pEnvironmentMgr->GetCurveValue( CAkEnvironmentsMgr::CurveObs, CAkEnvironmentsMgr::CurveLPF, fMinObsVal * 100 );
	}
	else // 3D User Defined
	{
		AKASSERT( eType == Ak3DUserDef );

		// Listener is always at origin for 3D User Defined
		AkVector vecListenerPos;
		vecListenerPos.X = 0;
		vecListenerPos.Y = 0;
		vecListenerPos.Z = 0;

		GetListenerVolume( pGen3DParams, in_posParams.Position, vecListenerPos, fMinCone, fDistance, fListenerVolumeDry, fListenerVolumeWet );
		fMinDist = fDistance;

		AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];

		if ( pGen3DParams->GetParams()->m_bIsSpatialized )
		{
			AkVector vTransformed;
			TransformSoundPosition( vecListenerPos, in_posParams.Position.Position, &m_matRot[0][0], &vTransformed );

			if ( pSpreadCurve )
				fSpread = pSpreadCurve->Convert( fDistance );

			if ( pLPFCurve )
				in_params.LPF = AkMath::Max( in_params.LPF, pLPFCurve->Convert( fDistance ) );

#ifdef AK_LFECENTER
			if ( pLFECurve )
				fLFEVolume = pLFECurve->Convert( fDistance );
#endif

			CAkSpeakerPan::GetSpeakerVolumes( 
				vTransformed.X,
				vTransformed.Z,
				in_posParams.fDivergenceCenter,
				fSpread,
				volumes,
				uNumFullBandChannels );
		}
		else
		{
			CAkSpeakerPan::GetSpeakerVolumes2DPan( 0.5f, 0.5f, //default position is center front 
				in_posParams.fDivergenceCenter, 
				true,
				in_uChannelMask,
				volumes );
			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
				volumes[iChannel].FastLinTodB();
			// TODO OPTI: Have separate path to avoid converting twice.
		}

		AkFeedbackParams * pFeedbackParams = in_pContext->GetFeedbackParameters();
		if (pFeedbackParams != NULL)
		{
			AkUInt32 iIndex = pFeedbackParams->VolumeIndex(AkFeedbackParams::NextVolumes, 0, 0);
			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				// If we are using these volumes for feedback, keep the attenuation and position volume separate.
				// We will normalize the position volumes and recombine the attenuation later (see AkFeedbackMgr::GetPlayerVolumes())
				// Use the "listener 0" to transfer the volumes.  All players are affected the same way, no need to copy everything uselessly.
				pFeedbackParams->m_Volumes[iIndex] = volumes[iChannel];
				pFeedbackParams->m_Volumes[iIndex].dBToLin();
				iIndex++;
				pFeedbackParams->m_fNextAttenuation[0] = AkMath::dBToLin(fListenerVolumeDry);
			}
		}

		// Prepare out_pAudioMixes: add listener volume, hierarchy volume,
		// handle LFE, convert to linear, add dry level ratio.
		AkReal32 fVolumeOffsetDry = fVolumeAM + fListenerVolumeDry;
		AkReal32 fVolumeOffsetWet = fVolumeAM + fListenerVolumeWet;

		if ( in_bEnvBus )
		{
			// Prepare dry and wet mixes.
			AkReal32 fDryLevel = in_pContext->GetDryLevelValue();

			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				// Dry.
				out_pAudioMixDry[iChannel].Next = volumes[iChannel];
				out_pAudioMixDry[iChannel].Next.Add( fVolumeOffsetDry );
#ifdef AK_LFECENTER
				out_pAudioMixDry[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixDry[iChannel].Next.dBToLin();
				out_pAudioMixDry[iChannel].Next.Mul( fDryLevel );

				// Wet.
			out_pAudioMixWet[iChannel].Next = volumes[iChannel];
				out_pAudioMixWet[iChannel].Next.Add( fVolumeOffsetWet );
#ifdef AK_LFECENTER
				out_pAudioMixWet[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixWet[iChannel].Next.dBToLin();
			}

#ifdef AK_LFECENTER
			// Handle LFE separately.
			if ( bSourceHasLFE )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				memset( &( out_pAudioMixWet[uChanLFE].Next ), 0, sizeof(AkSIMDSpeakerVolumes) );
				out_pAudioMixWet[uChanLFE].Next.volumes.fLfe = AkMath::dBToLin( fLfeAM + fLFEVolume );
				memset( &( out_pAudioMixDry[uChanLFE].Next ), 0, sizeof(AkSIMDSpeakerVolumes) );
				out_pAudioMixDry[uChanLFE].Next.volumes.fLfe = AkMath::dBToLin( fLfeAM + fLFEVolume ); 
				out_pAudioMixDry[uChanLFE].Next.Mul( fDryLevel );
			}
#endif
		}
		else
		{
			// Prepare dry mix only, avoid multiplying by dry level.
			for(AkUInt32 iChannel=0; iChannel<uNumFullBandChannels; iChannel++)
			{
				// Dry.
				out_pAudioMixDry[iChannel].Next = volumes[iChannel];
				out_pAudioMixDry[iChannel].Next.Add( fVolumeOffsetDry );
#ifdef AK_LFECENTER
				out_pAudioMixDry[iChannel].Next.volumes.fLfe = AK_SAFE_MINIMUM_VOLUME_LEVEL;
#endif
				out_pAudioMixDry[iChannel].Next.dBToLin();
			}

#ifdef AK_LFECENTER
			// Handle LFE separately.
			if ( bSourceHasLFE )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				memset( &( out_pAudioMixDry[uChanLFE].Next ), 0, sizeof(AkSIMDSpeakerVolumes) );
				out_pAudioMixDry[uChanLFE].Next.volumes.fLfe = AkMath::dBToLin( fLfeAM + fLFEVolume ); 
			}
#endif
		}

		*out_pObsLPF = 0.0f; // No obstruction for 3D user defined sounds
	}


	if( pAttenuation )
	{
		///////////////////////////////////////////////////////////////////////////////////////////////
		//
		/// IMPORTANT :: READ AND UNDERSTAND THIS before touching the following code:
		//
		//////////////////////////////////////////////////////////////////////////////////////////////
		// What is the difference between these values? none has the same signification, even if they look the same:
		//
		// 1- pAttenuation->m_ConeParams.LoPass;				// Core unmodified value from attenuation.
		// 2- in_params.Cone.LoPass;						// LowPass Acquired, up to now, from others sections, not including cone effect.
		// 3- pGen3DParams->GetParams()->m_ConeParams.LoPass;	// LoPass including RTPC.
		//
		//////////////////////////////////////////////////////////////////////////////////////////////

		AkLPFType fConeLPF = pGen3DParams->GetParams()->m_ConeParams.LoPass;
		if( fConeLPF > 0 )
		{
			fConeLPF = AkMath::Interpolate( 0.0f, (AkReal32)AK_MIN_LOPASS_VALUE, 1.0f, fConeLPF, fMinCone );
			in_params.LPF = AkMath::Max( in_params.LPF, fConeLPF );
		}

		CAkAttenuation::AkAttenuationCurve* pLpfCurve = pAttenuation->GetCurve( AttenuationCurveID_LowPassFilter );
		if( pLpfCurve )
		{
			AkLPFType fRadiusLPF = pLpfCurve->Convert( fMinDist );
			in_params.LPF = AkMath::Max( in_params.LPF, fRadiusLPF );
		}
	}
}

void CAkListener::GetListenerVolume( 
			CAkGen3DParams* in_pGen3DParams, 
			const AkSoundPosition& in_soundPos, 
			const AkVector & in_vecListenerPos,
			AkReal32& io_fMinCone,
			AkReal32& io_fDistance,
			AkReal32& io_fListenerVolumeDry,
			AkReal32& io_fListenerVolumeWet
			)
{
	AkVector direction;

	direction.X = in_vecListenerPos.X - in_soundPos.Position.X;
	direction.Y = in_vecListenerPos.Y - in_soundPos.Position.Y;
	direction.Z = in_vecListenerPos.Z - in_soundPos.Position.Z;

	io_fDistance = AkMath::Magnitude( direction );

	in_pGen3DParams->Lock();

	CAkAttenuation* pAttenuation = in_pGen3DParams->GetParams()->GetAttenuation();
	CAkAttenuation::AkAttenuationCurve* pAttenuationCurveDry = NULL;
	CAkAttenuation::AkAttenuationCurve* pAttenuationCurveWet = NULL;
	io_fListenerVolumeDry = 0.0f;
	io_fListenerVolumeWet = 0.0f;
	if( pAttenuation )
	{
		pAttenuationCurveDry = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
		pAttenuationCurveWet = pAttenuation->GetCurve( AttenuationCurveID_VolumeWet );

		if( pAttenuationCurveDry )
			io_fListenerVolumeDry = pAttenuationCurveDry->Convert( io_fDistance );
		if( pAttenuationCurveWet == pAttenuationCurveDry ) //optimize when Wet is UseCurveDry
			io_fListenerVolumeWet = io_fListenerVolumeDry;
		else if( pAttenuationCurveWet )
			io_fListenerVolumeWet = pAttenuationCurveWet->Convert( io_fDistance );

		if ( io_fDistance > 0.0f && pAttenuation->m_bIsConeEnabled )
		{
			direction.X /= io_fDistance;
			direction.Y /= io_fDistance;
			direction.Z /= io_fDistance;

			AkReal32 fConeValue = GetConeValue( direction, in_soundPos.Orientation, pAttenuation->m_ConeParams );

			io_fMinCone = AkMath::Min( io_fMinCone, fConeValue );

			AkReal32 fConeOffset = ( fConeValue * in_pGen3DParams->GetParams()->m_ConeParams.fOutsideVolume );
			io_fListenerVolumeDry += fConeOffset;
			//io_fListenerVolumeWet += fConeOffset; //WG-6276: cone should not affect volume wet
		}
		else
		{
			io_fMinCone = 0.0f; // assume no cone attenuation at listener pos
		}
	}
	else
	{
		io_fMinCone = 0.0f; // assume no cone attenuation at listener pos
	}

	in_pGen3DParams->Unlock();
}
