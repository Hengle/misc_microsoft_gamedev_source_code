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
// AkVPLMixBusNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_MIX_BUS_NODE_H_
#define _AK_VPL_MIX_BUS_NODE_H_

#include "AkLEngineDefs.h"
#include "AkLEngineStructs.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkFXContext.h"

#include "AkMixer.h"
#include "AkVPLNode.h"
#include "AkBusCtx.h"
#include "AkPBI.h"

class CAkBusVolumes
{
public:
	void SetNextVolume( AkReal32 in_dBVolume )
	{
		m_fNextVolumedB = in_dBVolume;
		m_fNextVolume = AkMath::dBToLin( in_dBVolume );
	}

	void SetNextLfe( AkReal32 in_dBLfe )
	{
		m_fNextLfedB = in_dBLfe;
		m_fNextLfe = AkMath::dBToLin( in_dBLfe );
	}

	void SetVolumeOffset( AkReal32 in_VolumeOffsetdB )
	{
		m_fNextVolumedB += in_VolumeOffsetdB;
		m_fNextVolume = AkMath::dBToLin( m_fNextVolumedB );
	}

	void SetLFEOffset( AkReal32 in_LfeOffsetdB )
	{
		m_fNextLfedB += in_LfeOffsetdB;
		m_fNextLfe = AkMath::dBToLin( m_fNextLfedB );
	}

protected:
	void TagPreviousVolumes()
	{
		m_fPreviousVolume = m_fNextVolume;
		m_fPreviousLfe = m_fNextLfe;
	}

	void ResetVolumes()
	{
		m_fPreviousVolume			= 1.0f;
		m_fNextVolume				= 1.0f;
		m_fPreviousLfe				= 1.0f;	
		m_fNextLfe					= 1.0f;
		m_fNextVolumedB				= 0.0f;
		m_fNextLfedB				= 0.0f;
	}

	AkReal32 GetNextVolume(){ return m_fNextVolume; }
	AkReal32 GetNextLfe(){ return m_fNextLfe; }
	AkReal32 GetPreviousVolume(){ return m_fPreviousVolume; }
	AkReal32 GetPreviousLfe(){ return m_fPreviousLfe; }

private:
	AkReal32				m_fNextVolume;			// Next bus volume.
	AkReal32				m_fPreviousVolume;		// Previous bus volume.
	AkReal32				m_fPreviousLfe;			// Previous bus lfe volume.
	AkReal32				m_fNextLfe;				// Next bus lfe volume.

	AkReal32				m_fNextVolumedB;
	AkReal32				m_fNextLfedB;
};

class CAkVPLMixBusNode : public CAkBusVolumes
{
public:
	AKRESULT			Init( AkChannelMask in_uChannelMask, AkUInt16 in_uMaxFrames );
	AKRESULT			Term();
	void				Stop() { m_eState = NodeStateStop; }
	AKRESULT ReleaseBuffer();
	AKRESULT			Connect( );
	AKRESULT			Disconnect( );
	AKRESULT			SetInsertFx( AkEnvID in_EnvID );
	void				SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );

	//New execution model
	void				ConsumeBuffer( AkVPLMixState & io_rMixState );
	void				ProcessDone( AkVPLState & io_state );
	void				GetResultingBuffer( AkAudioBufferFinalMix*& io_rpBuffer );
	void				ProcessAllFX();
	void				ProcessFX( AkUInt32 in_fxIndex, bool & io_bfxProcessed );
	void				PostProcessFx( AkAudioBufferFinalMix*& io_rpBuffer );

#ifdef AK_PS3
	void				ClearLastItemMix() { m_pLastItemMix = NULL; }
	AkAudioBufferFinalMix*	GetOutBuffer(){ return &m_BufferOut; }
#endif

	IAkRTPCSubscriber * GetFXParams( AkPluginID	in_FXID );
	AkEnvID				GetEnvID(){ return m_EnvID; }
	VPLNodeState		GetState(){ return m_eState; }

	CAkBusCtx				m_BusContext;
protected:
	struct FX
	{
		AkPluginID id;					// Effect unique type ID. 
		AK::IAkPluginParam * pParam;	// Parameters.
		AK::IAkEffectPlugin * pEffect;	// Pointer to a bus fx filter node.
		CAkBusFXContext * pBusFXContext;// Bus FX context
		AkUInt8 bBypass : 1;			// Bypass state
		AkUInt8 bLastBypass : 1;		// Bypass state on previous buffer
	};
	AkUInt8					m_bBypassAllFX : 1;
	AkUInt8					m_bLastBypassAllFX : 1;

	VPLNodeState			m_eState;
	CAkMixer				m_Mixer;				// Mixer.

	AkAudioBufferFinalMix	m_BufferOut;			// Final mix output buffer.	

	AkReal32				m_fPreviousEnvVolume;
	AkUInt32				m_ulBufferOutSize;		// Final mix output buffer size.
	FX						m_aFX[ AK_NUM_EFFECTS_PER_OBJ ];
	AkEnvID					m_EnvID;				// Environment ID(only used if environmental)
	AkUInt32				m_uConnectCount;		// Number of inputs actually connected

#ifdef AK_PS3
	AkVPLMixState *			m_pLastItemMix;
#endif
};

#endif //_AK_VPL_MIX_BUS_NODE_H_
