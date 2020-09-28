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
// AkVPLFinalMixNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_VPL_FINAL_MIX_NODE_H_
#define _AK_VPL_FINAL_MIX_NODE_H_

#include "AkSrcBase.h"

#include "AkMixer.h"
#include "Ak3DParams.h"
#include "AkVPLNode.h"
#include "AkVPLMixBusNode.h"

// Effect
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include "AkFXContext.h"
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include "AkBusCtx.h"

class CAkVPLFinalMixNode : public CAkBusVolumes
{
public:

	AKRESULT			Init( AkUInt32 in_uChannelMask );
	AKRESULT			Term();
	void				Stop() { m_eState = NodeStateStop; }
	AKRESULT			ReleaseBuffer( AkPipelineBuffer* in_pBuffer );
	AKRESULT			Connect( CAkVPLMixBusNode * in_pInput );

	//New execution model
	AKRESULT			ConsumeBuffer(
		AkAudioBufferFinalMix*&	io_rpBuffer
		);
	void			GetResultingBuffer(
		AkPipelineBufferBase*&	out_pBuffer
		);

	IAkRTPCSubscriber * GetFXParams( AkPluginID	in_FXID );
	
	void SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask );

	AKRESULT SetInsertFx();
	
	VPLNodeState		GetState(){ return m_eState; }

private:
	struct FX
	{
		AkPluginID id;					// Effect unique type ID. 
		AK::IAkPluginParam * pParam;	// Parameters.
		AK::IAkEffectPlugin * pEffect;	// Pointer to a bus fx filter node.
		CAkBusFXContext * pBusFXContext;// Bus FX context
		AkUInt8 bBypass : 1;			// Bypass state
		AkUInt8 bLastBypass : 1;		// Bypass state on previous buffer
	};

	VPLNodeState			m_eState;
	CAkMixer				m_Mixer;				// Mixer.

	AkAudioBufferFinalMix	m_BufferOut;			// Final mix output buffer (non-interleaved)
#ifndef AK_PS3
	AkPipelineBufferBase	m_MasterOut;			// Final mix output buffer (interleaved with master volume applied).	
#endif

	AkUInt32				m_ulBufferOutSize;

	FX						m_aFX[ AK_NUM_EFFECTS_PER_OBJ ];
	AkUInt8					m_bBypassAllFX : 1;
	AkUInt8					m_bLastBypassAllFX : 1;
};

#endif // _AK_VPL_FINAL_MIX_NODE_H_
