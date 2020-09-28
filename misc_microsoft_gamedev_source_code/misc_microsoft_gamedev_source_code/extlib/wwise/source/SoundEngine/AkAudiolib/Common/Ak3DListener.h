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
// Ak3DListener.h
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "AkCommon.h"
#include <AK/Tools/Common/AkObject.h>
#include "AkFeedbackStructs.h"

class AkAudioBuffer;
struct AkSoundParams;
struct AkSoundPositioningParams;
struct AkSpeakerVolumes;
class  CAkPBI;
class  CAkGen3DParams;
struct ConeParams;

struct ListenerData
{
	// keep these here so we don't have to fetch them every time
	AkListenerData      Listener;
	AkReal32			Matrix[3][3];			// world to listener coordinates transformation
};
//====================================================================================================
//====================================================================================================
class CAkListener
{
public:

	static AKRESULT Init();

	static AKRESULT Term();

	static AkForceInline const AkListenerData & GetListenerData( 
		AkUInt32 in_uListener
		)
	{
		return m_listeners[ in_uListener ].Listener;
	}

	static AKRESULT SetListenerPosition( 
		AkUInt32 in_uListener,
		const AkListenerPosition & in_Position
		);

	static AKRESULT GetListenerPosition( 
		AkUInt32 in_uListener,
		AkListenerPosition& out_rPosition
		);

	static AKRESULT SetListenerSpatialization(
		AkUInt32 in_uListener,
		bool in_bSpatialized,
		AkSpeakerVolumes * in_pVolumeOffsets
		);

	static AKRESULT GetListenerSpatialization(
		AkUInt32 in_uListener,
		bool& out_rbSpatialized,
		AkSpeakerVolumes& out_rVolumeOffsets
		);

	static AkReal32 GetDistanceFactor();

	static AKRESULT SetDistanceFactor(
		AkReal32	in_fDistanceFactor
		);

	// move a sound in the listeners coordinates
	static void TransformSoundPosition(
		const AkVector & in_vecListenerPos,
		const AkVector & in_vecPosition,
		AkReal32* AK_RESTRICT in_pMatrix,
		AkVector* AK_RESTRICT pout_Transformed
		);

	static void Get3DVolumes(
		CAkPBI*				in_pContext,
	    AkSoundParams&		in_params,
	    AkSoundPositioningParams& in_posParams,
		AkChannelMask		in_uChannelMask,
		AkAudioMix*			out_pAudioMixWet,
		AkAudioMix*			out_pAudioMixDry, //with obs
		AkReal32*           out_pObsLPF,
		bool				in_bEnvBus
		);

	static AkReal32 GetConeValue(
		const AkVector & in_Direction, // Normalized object->listener direction vector
		const AkVector & in_Orientation,
		const ConeParams& in_ConeParams
		);
	
	static void SetListenerPipeline(
		AkUInt32		in_uListener,	//Listener ID.
		bool			in_bAudio,		//Is this listener used for audio data?
		bool			in_bFeedback	//Is this listener used for feedback data?
		);

	static AkForceInline AkUInt32 GetFeedbackMask() 
	{
		return m_uFeedbackMask;
	}

//----------------------------------------------------------------------------------------------------
// audiolib only variables
//----------------------------------------------------------------------------------------------------
private:

	static void GetListenerVolume( 
		CAkGen3DParams*			in_pGen3DParams, 
		const AkSoundPosition& in_soundPos,
		const AkVector & in_vecListenerPos,
		AkReal32& io_fMinCone,
		AkReal32& io_fDistance,
		AkReal32& io_fListenerVolumeDry,
		AkReal32& io_fListenerVolumeWet	);

	// the listeners we know about
	static ListenerData	m_listeners[ AK_NUM_LISTENERS ];
	static AkUInt32	m_uAudioMask;
	static AkUInt32 m_uFeedbackMask;

	static AkReal32 m_matRot[3][3]; // 45 degrees around Y rotation matrix for 5.1 speaker config calculations
};
