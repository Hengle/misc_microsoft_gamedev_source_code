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
// AkLEngine.cpp
//
// Implementation of the IAkLEngine interface. Win32 version.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkLEngine.h"

#include "Ak3DListener.h"
#include "AkAudioLibTimer.h"
#include "AkXBox360Audiolib.h"
#include "AkSink.h"
#include "AkProfile.h"
#include "AkMonitor.h"
#include "AkMath.h"
#include "Ak3dParams.h"
#include "AkDefault3DParams.h"			// g_DefaultListenerPosition.
#include "AkRegistryMgr.h"
#include "AkRTPCMgr.h"
#include "AkFXMemAlloc.h"
#include "AkSpeakerPan.h"
#include "AkEnvironmentsMgr.h"
#include "AkAudioMgr.h"
#include "AkEffectsMgr.h"
#include "AkPlayingMgr.h"
#include "AkPositionRepository.h"
#include <setjmp.h>

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

// List sizes.

#define MAX_NUM_PLAY_EVENTS			500
#define MAX_NUM_LE_COMMANDS         (2*MAX_NUM_PLAY_EVENTS) // Commands may hang in the list for a while.

#define LENGINE_DEFAULT_POOL_ID		g_LEngineDefaultPoolId

#define MINIMUM_STARVATION_NOTIFICATION_DELAY 8 //in buffers

//-----------------------------------------------------------------------------
//LEngine memory pools.
//-----------------------------------------------------------------------------
AkMemPoolId	g_LEngineDefaultPoolId	= AK_INVALID_POOL_ID;

//-----------------------------------------------------------------------------
//Lower engine global singletons.
//-----------------------------------------------------------------------------
CAkSink *				g_pAkSink		 = NULL;
AkPlatformInitSettings  g_PDSettings	 = { 0, };

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
CAkVPLFinalMixNode *		CAkLEngine::m_pFinalMixNode		= NULL;
CAkLEngine::AkArrayVPL		CAkLEngine::m_arrayVPLs;
CAkLEngine::AkEnvBusList	CAkLEngine::m_EnvBusList;
AkListVPLSrcs				CAkLEngine::m_listSrcsNotConnected;
AkUInt32					CAkLEngine::m_ulPlayID			= 0;
AkUInt32					CAkLEngine::m_ulPlayEventID		= 0;
AkChannelMask				CAkLEngine::m_uOutputChannelMask= 0;
CAkLEngine::AkListCmd		CAkLEngine::m_listCmd;
AkEvent						CAkLEngine::m_hEventPacketDone;
AkEvent						CAkLEngine::m_EventStop;
AkUniqueID					CAkLEngine::m_VPLPipelineID		= 0;	// Profiling vpl src id.
CAkLock						CAkLEngine::m_LockFxParams;
AkUInt32					CAkLEngine::m_uLastStarvationTime = 0;
AkUInt16					CAkLEngine::m_cMarkers = 0;
AkBufferMarker *			CAkLEngine::m_pMarkers = NULL;

CAkLEngine::BufferCache		CAkLEngine::m_CachedAudioBuffers[NUM_CACHED_BUFFER_SIZES];

CAkFeedbackDeviceMgr*		CAkLEngine::m_pDeviceMgr = NULL;
//-----------------------------------------------------------------------------
// Name: ApplyVolumes
// Desc: Apply a volume gain to a AkSpeakerVolumes.
//
// Parameters:
//	AkSpeakerVolumes& out_Volumes	: output volumes
//	AkSpeakerVolumes& in_Volumes	: input volumes
//	AkReal32 in_fControlValue		: modifier value
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
static AkForceInline void ApplyVolumes( AkSIMDSpeakerVolumes& out_Volumes, AkSIMDSpeakerVolumes& in_Volumes, AkReal32 in_fControlValue )
{
	__vector4 vMul = vec_loadAndSplatScalar( &in_fControlValue );
	__vector4 vTmp0 = __vmulfp( in_Volumes.vector[0], vMul );
	__vector4 vTmp1 = __vmulfp( in_Volumes.vector[1], vMul );
	out_Volumes.vector[0] = vTmp0;
	out_Volumes.vector[1] = vTmp1;
}

void CAkLEngine::GetDefaultPlatformInitSettings( 
                AkPlatformInitSettings &      out_pPlatformSettings      // Platform specific settings. Can be changed depending on hardware.
                )
{
	memset( &out_pPlatformSettings, 0, sizeof( AkPlatformInitSettings ) );
	out_pPlatformSettings.threadLEngine.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadLEngine.dwProcessor = AK_XBOX360_DEFAULT_PROCESSOR_ID;
	out_pPlatformSettings.threadLEngine.uStackSize = AK_DEFAULT_STACK_SIZE;
	out_pPlatformSettings.threadBankManager.nPriority = AK_THREAD_BANK_MANAGER_PRIORITY;
	out_pPlatformSettings.threadBankManager.dwProcessor = AK_XBOX360_BANK_MANAGER_PROCESSOR_ID;
	out_pPlatformSettings.threadBankManager.uStackSize = AK_BANK_MGR_THREAD_STACK_BYTES;
	out_pPlatformSettings.uLEngineDefaultPoolSize = LENGINE_DEFAULT_POOL_SIZE;
	out_pPlatformSettings.uNumRefillsInVoice = AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER;
#ifndef AK_OPTIMIZED
	out_pPlatformSettings.threadMonitor.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;
	out_pPlatformSettings.threadMonitor.dwProcessor = AK_XBOX360_DEFAULT_PROCESSOR_ID;
	out_pPlatformSettings.threadMonitor.uStackSize = AK_DEFAULT_STACK_SIZE;
#endif
}

AKRESULT CAkLEngine::CreateLEnginePools()
{
	// create the default pool
	if ( g_LEngineDefaultPoolId == AK_INVALID_POOL_ID )
	{
		g_LEngineDefaultPoolId = AK::MemoryMgr::CreatePool(NULL,
			g_PDSettings.uLEngineDefaultPoolSize > LENGINE_DEFAULT_POOL_BLOCK_SIZE ? g_PDSettings.uLEngineDefaultPoolSize : LENGINE_DEFAULT_POOL_SIZE,
			LENGINE_DEFAULT_POOL_BLOCK_SIZE,
			AkPhysicalAlloc,
			LENGINE_DEFAULT_POOL_ALIGN );

		if ( g_LEngineDefaultPoolId != AK_INVALID_POOL_ID )
		{
			AkFXMemAlloc::GetLower()->SetPoolId(g_LEngineDefaultPoolId);
			AK_SETPOOLNAME(g_LEngineDefaultPoolId,L"Lower Engine Default");
		}
	}

    if ( g_LEngineDefaultPoolId == AK_INVALID_POOL_ID )
    {
        return AK_InsufficientMemory;
    }

    return AK_Success;
}

void CAkLEngine::DestroyLEnginePools( void )
{
	AKASSERT( AK::MemoryMgr::IsInitialized() );

    if ( AK::MemoryMgr::IsInitialized() )
    {
	    // destroy the default pool
        if ( g_LEngineDefaultPoolId != AK_INVALID_POOL_ID )
        {
            AKVERIFY( AK::MemoryMgr::DestroyPool( g_LEngineDefaultPoolId ) == AK_Success );
            g_LEngineDefaultPoolId = AK_INVALID_POOL_ID;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: ApplyGlobalSettings
// Desc: Stores global settings in global variable g_PDSettings.
//
// Parameters: AkPlatformInitSettings * io_pPDSettings 
//-----------------------------------------------------------------------------
void CAkLEngine::ApplyGlobalSettings( AkPlatformInitSettings *   io_pPDSettings )
{
	// Settings.
    if ( io_pPDSettings == NULL )
    {
		GetDefaultPlatformInitSettings( g_PDSettings );
    }
    else
	{
        g_PDSettings = *io_pPDSettings;

		if( g_PDSettings.uNumRefillsInVoice < 2 )
			g_PDSettings.uNumRefillsInVoice = AK_DEFAULT_NUM_REFILLS_IN_VOICE_BUFFER;

		// Update client settings with actual values (might have changed due to hardware constraints).
		*io_pPDSettings = g_PDSettings;
	}
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialise the object.
//
// Parameters:
//
// Return: 
//	Ak_Success:          Object was initialised correctly.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Failed to initialise the object correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::Init()
{
	// Check memory manager.
    if ( !AK::MemoryMgr::IsInitialized() )
    {
        AKASSERT( !"Memory manager does not exist" );
        return AK_Fail;
    }
    // Check Stream manager.
    if ( AK::IAkStreamMgr::Get() == NULL )
    {
        AKASSERT( !"Stream manager does not exist" );
        return AK_Fail;
    }

    // Create LEngine memory pool(s).
	AKRESULT eResult = CAkLEngine::CreateLEnginePools();
    if ( eResult != AK_Success ) return eResult;

	m_ulPlayEventID		= 0;

	// Voice Manager.

    if( !g_pAkSink )
	{
		g_pAkSink = CAkSink::Create( g_PDSettings );
		if ( !g_pAkSink ) return AK_Fail;
	}

	// Listener.
	eResult = CAkListener::Init();
	if( eResult != AK_Success ) return eResult;

    // Effects Manager.
	eResult = CAkEffectsMgr::Init();
    if( eResult != AK_Success ) return eResult;

	eResult = m_listCmd.Init( MAX_NUM_LE_COMMANDS, MAX_NUM_LE_COMMANDS, g_LEngineDefaultPoolId );
	if( eResult != AK_Success ) return eResult;

    eResult = m_listSrcsNotConnected.Init();
	if( eResult != AK_Success ) return eResult;

	eResult = m_EnvBusList.Init( MIN_NUM_ENV_MIX_BUSSES, MAX_NUM_ENV_MIX_BUSSES, g_LEngineDefaultPoolId );
	if( eResult != AK_Success ) return eResult;

	// Create the speaker panner.
	CAkSpeakerPan::Init();

	// Final mix.
	CAkVPLFinalMixNode * pFinalMixNode;
	AkNew2( pFinalMixNode, LENGINE_DEFAULT_POOL_ID, CAkVPLFinalMixNode, CAkVPLFinalMixNode() );
	if( pFinalMixNode == NULL ) return AK_InsufficientMemory;

	m_pFinalMixNode = pFinalMixNode;

	m_uOutputChannelMask = g_pAkSink->GetSpeakerConfig();

	eResult = pFinalMixNode->Init( m_uOutputChannelMask );
	if( eResult != AK_Success ) return eResult;

	m_hEventPacketDone = ::CreateEvent( NULL, false, true, NULL );

	return AllocVoice();
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate the object.
//
// Parameters:
//	None.
//
// Return:
//	Ak_Success: Object was terminated correctly.
//  AK_Fail:    Failed to terminate correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::Term()
{
	// Clean up all VPL mix busses.
	DestroyAllVPLMixBusses();
	DestroyAllEnvMixBusses();

    m_listSrcsNotConnected.Term();
	m_arrayVPLs.Term();
	m_listCmd.Term();
	m_EnvBusList.Term();

	// Free the final mix node.
	if ( m_pFinalMixNode )
	{
		m_pFinalMixNode->Term();
		AkDelete2( LENGINE_DEFAULT_POOL_ID, CAkVPLFinalMixNode, m_pFinalMixNode );
		m_pFinalMixNode = NULL;
	}

	// Buffer Cache
	for ( AkUInt32 iSlot = 0; iSlot < NUM_CACHED_BUFFER_SIZES; ++iSlot )
	{
		for ( AkUInt32 iBuffer = 0, cBuffers = m_CachedAudioBuffers[ iSlot ].Length(); iBuffer < cBuffers; ++iBuffer )
			AkFree( g_LEngineDefaultPoolId, m_CachedAudioBuffers[ iSlot ][ iBuffer ] );

		m_CachedAudioBuffers[ iSlot ].Term();
	}

	CAkEffectsMgr::Term();

	CAkListener::Term();

	if( g_pAkSink )
	{
		g_pAkSink->Term();
		AkDelete( g_LEngineDefaultPoolId, g_pAkSink );
		g_pAkSink = NULL;
	}

	if (m_pDeviceMgr != NULL)
	{
		m_pDeviceMgr->Destroy();
		m_pDeviceMgr = NULL; 
	}

	if( m_hEventPacketDone != NULL )
	{
		::CloseHandle( m_hEventPacketDone );
		m_hEventPacketDone = NULL;
	}

	// Destroy pools.
    DestroyLEnginePools();

    return AK_Success;
} // Term

AkUInt32 CAkLEngine::GetNumBufferNeededAndSubmit()
{
	// Try to submit packet from ring buffer if we have one
	g_pAkSink->m_RBLock.Lock();

	if ( g_pAkSink->IsDataNeededVoice() && g_pAkSink->IsDataReadyRB() )
		g_pAkSink->SubmitPacketRB();

	AkUInt32 uNbBuffersNeeded = g_pAkSink->IsDataNeededRB();

	g_pAkSink->m_RBLock.Unlock();

	return uNbBuffersNeeded;
}

//-----------------------------------------------------------------------------
// Name: Perform
// Desc: Perform all VPLs.
//
// Parameters:
//	None.
//
// Return:
//	AK_Success:	VPL executed.
//  AK_Fail:		Failed to execute a VPL.
//-----------------------------------------------------------------------------
AkUInt32 CAkLEngine::Perform( AkUInt32 in_uNumBufferToFill )
{
	ExecuteCmds();
	HandleStarvation();

	AKASSERT( in_uNumBufferToFill != 0 );

	SequencerVoiceFilling( in_uNumBufferToFill );

	//ALWAYS call RenderData.  Even if we didn't process any VPL, there might be data coming from the audio pipeline.
	if(IsFeedbackEnabled())
	{
		AK_START_TIMER_FEEDBACK();
		m_pDeviceMgr->RenderData();
		AK_STOP_TIMER_FEEDBACK();
	}

	RemoveVPLMixBussesSources();

	return in_uNumBufferToFill;
} // Perform

void CAkLEngine::StartVoice()
{
	g_pAkSink->Play();
}

AkEvent & CAkLEngine::GetVoiceEvent()
{
	return m_hEventPacketDone;
}

//-----------------------------------------------------------------------------
// Name: DequeuePBI
// Desc: Destroy the specified PBI.
//
// Parameters:
//	CAkPBI * in_pPBI : Pointer to PBI to dequeue.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::DequeuePBI( CAkPBI* in_pPBI )
{
	AkListCmd::IteratorEx iter = m_listCmd.BeginEx();
	while ( iter != m_listCmd.End() )
	{
		AkLECmd & event = *iter;
		if( event.m_pCtx == in_pPBI )
		{
			// Remove all events that are associated with this context
			iter = m_listCmd.Erase( iter );
		}
		else
		{
			++iter;
		}
	}
} // DequeuePBI

AKRESULT CAkLEngine::EnqueueAction( LEState in_eState, CAkPBI * in_pContext )
{
	AkLECmd * pCmd = m_listCmd.AddLast();
	if ( !pCmd )
		return AK_Fail;

	pCmd->m_eState = in_eState;
	pCmd->m_pCtx = in_pContext;
	pCmd->m_ulSequenceNumber = m_ulPlayEventID;
	pCmd->m_pVPL = NULL;
	pCmd->m_pVPLSrc = NULL;

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: EnqueueActionStop
// Desc: Stop a specified sound.
//
// Return: 
//	Ak_Success:          Sound was scheduled to be stopped.
//	AK_InvalidParameter: Invalid parameters.
//	AK_Fail:             Failure.
//
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::EnqueueActionStop( CAkPBI * in_pCtx )
{
	// If we are doing an immediate play-stop, it's worth going through
	// the list of commands to intercept the play before the lower engine
	// creates the voice.
	
	AkListCmd::IteratorEx iter = m_listCmd.BeginEx();
	while ( iter != m_listCmd.End() )
	{
		AkLECmd & event = *iter;
		if( event.m_pCtx == in_pCtx )
		{
			if ( event.m_eState == LEStatePlay
				|| event.m_eState == LEStatePlayPause )
			{
				if( !event.m_pVPLSrc )
				{
					DequeuePBI( in_pCtx ); // make sure nothing reaches the lower engine.
					in_pCtx->Destroy( CtxDestroyReasonPlayFailed ); // ... important not to destroy the PBI here immediately -- this call will go through the context notifications.
					return AK_Success;
				}
				//else
				//{
					// If there is a source, it is too late to kill the sound, we must let the play and the stop go.
					// destroying the PBI and the entries in the queue would result in VPL leak.
				//}
			}

			break; // if there is already an action and it's not a play, we know we need to enqueue.
		}

		++iter;
	}
	
	// Voice is already playing.

	return EnqueueAction( LEStateStop, in_pCtx );
}

#ifndef AK_OPTIMIZED
void CAkLEngine::GetNumPipelines(AkUInt16 &out_rAudio, AkUInt16& out_rFeedback)
{
	out_rAudio = 0;
	out_rFeedback = 0;

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		if ((*iterVPL)->m_bFeedback)
			out_rFeedback += (*iterVPL)->m_listVPLSrcs.Length();
		else
		out_rAudio += (*iterVPL)->m_listVPLSrcs.Length();
	}
}

void CAkLEngine::GetPipelineData( AkMonitorData::PipelineData * out_pPipelineData, bool in_bGetFeedback )
{
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		if (!in_bGetFeedback && (*iterVPL)->m_bFeedback)
			continue;	//Not needed for feedback (for now)

		CAkBusCtx &l_BusCtx = (*iterVPL)->m_MixBus.m_BusContext;
		AkFXDesc busFxDesc[ AK_NUM_EFFECTS_PER_OBJ ];
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			l_BusCtx.GetFX( uFXIndex, busFxDesc[ uFXIndex ] );
		AkVolumeValue l_BusVolume = l_BusCtx.GetVolume();

		extern CAkBusCtx g_MasterBusCtx;

		if ((*iterVPL)->m_bFeedback)
		{
			l_BusVolume += CAkFeedbackBus::GetMasterBus()->GetBusEffectiveVolume();
		}
		else
		{
			if( g_MasterBusCtx.HasEffect() )
			{
				l_BusVolume += g_MasterBusCtx.GetVolume();
			}
		}
		
		AkUniqueID l_BusID = l_BusCtx.ID();

		for ( AkListVPLSrcs::Iterator iterVPLSrc = (*iterVPL)->m_listVPLSrcs.Begin(); iterVPLSrc != (*iterVPL)->m_listVPLSrcs.End(); ++iterVPLSrc )
		{
			AkVPLSrc * pVPLSrc = *iterVPLSrc;
			
			//if( pVPLSrc->m_Src.GetState() != NodeStateStop )
			{
				CAkPBI * pCtx = pVPLSrc->m_Src.GetContext();
				if ( pCtx )
				{
					out_pPipelineData->gameObjID	= pCtx->GetGameObjectPtr()->ID();
					out_pPipelineData->soundID		= pCtx->GetSoundID();
					out_pPipelineData->mixBusID		= l_BusID;
					out_pPipelineData->pipelineID	= pVPLSrc->m_ID;

					AkFeedbackParams* pFeedbackParams = pCtx->GetFeedbackParameters();

					if (pFeedbackParams != NULL)
					{
						out_pPipelineData->feedbackMixBusID = pFeedbackParams->m_pOutput->ID();
						for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
						{
							AkFXDesc fxDesc;
							pFeedbackParams->m_pOutput->GetFX( uFXIndex, fxDesc ); // FIXME
							out_pPipelineData->feedbackBusFxID[ uFXIndex ] = fxDesc.EffectTypeID;
						}
					}
					else
					{
						out_pPipelineData->feedbackMixBusID = 0;
						for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
							out_pPipelineData->feedbackBusFxID[ uFXIndex ] = 0;
					}

					AkSrcDescriptor srcDescriptor;
					pCtx->GetSrcDescriptor( &srcDescriptor );

					for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
					{
						const AkFXDesc & fxDesc = pCtx->GetFX( uFXIndex );
						out_pPipelineData->fxID[ uFXIndex ] = fxDesc.EffectTypeID;
						out_pPipelineData->busFxID[ uFXIndex ] = busFxDesc[ uFXIndex ].EffectTypeID; 
					}

					out_pPipelineData->srcType = (AkSrcType)srcDescriptor.Type;
					out_pPipelineData->priority = pCtx->GetPriority();

					out_pPipelineData->fVolume = pCtx->GetVolume() + l_BusVolume;

					out_pPipelineData->bIsStarving = false; // FIXME

					if ( pVPLSrc->m_Src.LastAudible() )
						out_pPipelineData->bIsVirtual = false;
					else
					{
						AkVirtualQueueBehavior eBehave;
						out_pPipelineData->bIsVirtual = ( pCtx->GetVirtualBehavior( eBehave ) == AkBelowThresholdBehavior_SetAsVirtualVoice );
					}
				}
				else
				{
					out_pPipelineData->gameObjID = AK_INVALID_GAME_OBJECT;
				}
			}

			out_pPipelineData++;
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Name: ForceBusVolume
// Desc: Set the volume of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_Volume : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ForceBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_Volume )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.SetNextVolume( in_Volume );
		}
	}

	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->m_BusContext.ID() == in_MixBusID )
		{
			l_pEnvBus->SetNextVolume( in_Volume );
		}
	}

} // ForceBusVolume

//-----------------------------------------------------------------------------
// Name: ForceMasterBusVolume
// Desc: Set the volume of master bus.
//
// Parameters:
//	AkVolumeValue in_Volume : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ForceMasterBusVolume( AkVolumeValue in_Volume )
{
	m_pFinalMixNode->SetNextVolume( in_Volume );
}

//-----------------------------------------------------------------------------
// Name: SetBusVolume
// Desc: Set the volume of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_VolumeOffset : Volume to set.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::SetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_VolumeOffset )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.SetVolumeOffset( in_VolumeOffset );
		}
	}

	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->m_BusContext.ID() == in_MixBusID )
		{
			l_pEnvBus->SetVolumeOffset( in_VolumeOffset );
		}
	}
} // SetBusVolume

//-----------------------------------------------------------------------------
// Name: SetMasterBusVolume
// Desc: Set the volume of master bus.
//
// Parameters:
//	AkVolumeValue in_VolumeOffset : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::SetMasterBusVolume( AkVolumeValue in_VolumeOffset )
{
	m_pFinalMixNode->SetVolumeOffset( in_VolumeOffset );
}


//-----------------------------------------------------------------------------
// Name: ForceBusLFE
// Desc: Set the LFE of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_Volume : LFE to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ForceBusLFE( AkUniqueID in_MixBusID, AkVolumeValue in_LFE )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.SetNextLfe( in_LFE );
		}
	}

	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->m_BusContext.ID() == in_MixBusID )
		{
			l_pEnvBus->SetNextLfe( in_LFE );
		}
	}
} // ForceBusLFE

//-----------------------------------------------------------------------------
// Name: ForceMasterBusLFE
// Desc: Set the LFE of master bus.
//
// Parameters:
//	AkVolumeValue in_Volume : LFE to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ForceMasterBusLFE( AkVolumeValue in_LFE )
{
	m_pFinalMixNode->SetNextLfe( in_LFE );
}


//-----------------------------------------------------------------------------
// Name: SetBusLFE
// Desc: Set the volume of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_VolumeOffset : LFE to set.
//-----------------------------------------------------------------------------
void CAkLEngine::SetBusLFE( AkUniqueID in_MixBusID, AkVolumeValue in_LFEOffset )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.SetLFEOffset( in_LFEOffset );
		}
	}

	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->m_BusContext.ID() == in_MixBusID )
		{
			l_pEnvBus->SetLFEOffset( in_LFEOffset );
		}
	}
} // SetBusLFE

//-----------------------------------------------------------------------------
// Name: SetMasterBusLFE
// Desc: Set the volume of master bus.
//
// Parameters:
//	AkVolumeValue in_VolumeOffset : LFE to set.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::SetMasterBusLFE( AkVolumeValue in_LFEOffset )
{
	m_pFinalMixNode->SetLFEOffset( in_LFEOffset );
}

//-----------------------------------------------------------------------------
// Name: BypassBusFx
// Desc: Bypass the effect of a specified bus.
//
// Parameters:
//	AkUniqueID in_MixBusID    : ID of a specified bus.
//	bool	   in_bIsBypassed : true=bypass effect, false=do not bypass.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::BypassBusFx( AkUniqueID in_MixBusID, AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and bypass the fx.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.SetInsertFxBypass( in_bitsFXBypass, in_uTargetMask );
		}
	}

	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->m_BusContext.ID() == in_MixBusID )
		{
			l_pEnvBus->SetInsertFxBypass( in_bitsFXBypass, in_uTargetMask );
		}
	}
} // BypassBusFx

//-----------------------------------------------------------------------------
// Name: BypassMasterBusFx
// Desc: Bypass the effect the master
//
// Parameters:
//	bool	   in_bIsBypassed : true=bypass effect, false=do not bypass.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::BypassMasterBusFx( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	if ( m_pFinalMixNode )
	{
		m_pFinalMixNode->SetInsertFxBypass( in_bitsFXBypass, in_uTargetMask );
	}
} // BypassMasterBusFx

//-----------------------------------------------------------------------------
// Name: BypassEnvFx
// Desc: Bypass the effect of a specified environment.
//
// Parameters:
//	AkEnvID    in_EnvID    : environment ID.
//	bool	   in_bIsBypassed : true=bypass effect, false=do not bypass.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::BypassEnvFx( AkEnvID in_EnvID, bool in_bIsBypassed )
{
	CAkVPLMixBusNode * l_pEnvBus = NULL;
	for( AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin(); iterEnv != m_EnvBusList.End(); ++iterEnv )
	{
		l_pEnvBus = (*iterEnv).item;
		if( l_pEnvBus->GetEnvID() == in_EnvID )
		{
			l_pEnvBus->SetInsertFxBypass( 0, in_bIsBypassed ); // env fx is always at index 0
		}
	}
} // BypassEnvFx

//-----------------------------------------------------------------------------
// Name: StopMixBus
// Desc: 
//
// Parameters:
//		AkUniqueID in_MixBusID
//
// Return:
//		void
//-----------------------------------------------------------------------------
void CAkLEngine::StopMixBus( AkUniqueID in_MixBusID )
{
	AkVPL * l_pMixBus = NULL;

	// Find the bus and bypass the fx.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			l_pMixBus->m_MixBus.Stop();
		}
		// Do not break once found, there may be more than one bus with this ID, 
		// a second one may have been created before the previous one was destroyed
	}
}

//-----------------------------------------------------------------------------
// Name: StopAllMixBus
// Desc: 
//
// Return:
//		void
//-----------------------------------------------------------------------------
void CAkLEngine::StopAllMixBus( )
{
	// Find the bus and bypass the fx.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		(*iterVPL)->m_MixBus.Stop();
	}

	// Also stop the master control bus
	m_pFinalMixNode->Stop();
}

void * CAkLEngine::GetCachedAudioBuffer( AkUInt32 in_uSize )
{
	if (in_uSize < CACHED_BUFFER_SIZE_DIVISOR)
		in_uSize = CACHED_BUFFER_SIZE_DIVISOR;

	BufferCache & CachedBuffers = m_CachedAudioBuffers[ ( in_uSize - 1 ) / CACHED_BUFFER_SIZE_DIVISOR ];

	void * pvReturned;

	if ( !CachedBuffers.IsEmpty() )
	{
		pvReturned = CachedBuffers.Last();
		CachedBuffers.RemoveLast();
	}
	else
	{
		pvReturned = AkAlloc( g_LEngineDefaultPoolId, in_uSize );
	}

	return pvReturned;
}

void CAkLEngine::ReleaseCachedAudioBuffer( AkUInt32 in_uSize, void * in_pvBuffer )
{
	if (in_uSize < CACHED_BUFFER_SIZE_DIVISOR)
		in_uSize = CACHED_BUFFER_SIZE_DIVISOR;

	BufferCache & CachedBuffers = m_CachedAudioBuffers[ ( in_uSize - 1 ) / CACHED_BUFFER_SIZE_DIVISOR ];

	void ** ppvBuffer = CachedBuffers.AddLast();
	if ( ppvBuffer )
	{
		*ppvBuffer = in_pvBuffer;
	}
	else
	{
		AkFree( g_LEngineDefaultPoolId, in_pvBuffer );
	}
}

void CAkLEngine::LockFxParams()
{
	m_LockFxParams.Lock();
}

void CAkLEngine::UnlockFxParams()
{
	m_LockFxParams.Unlock();
}

//-----------------------------------------------------------------------------
// Name: GetMixBusFXParams
// Desc: 
//
// Parameters:
//		AkUniqueID in_MixBusID
//
// Return:
//		IAkRTPCSubscriber*
//-----------------------------------------------------------------------------
IAkRTPCSubscriber* CAkLEngine::GetMixBusFXParams( AkUniqueID in_MixBusID, AkPluginID in_FXID )
{
	AkVPL * l_pMixBus = NULL;

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_BusID == in_MixBusID )
		{
			if ( l_pMixBus->m_MixBus.GetState() == NodeStateStop )
				return NULL;
			else
				return l_pMixBus->m_MixBus.GetFXParams( in_FXID );
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Name: GetMasterBusFXParams
// Desc: 
//
// Return:
//		IAkRTPCSubscriber*
//-----------------------------------------------------------------------------
IAkRTPCSubscriber* CAkLEngine::GetMasterBusFXParams( AkPluginID	in_FXID )
{
	if ( m_pFinalMixNode )
		return m_pFinalMixNode->GetFXParams( in_FXID );

		return NULL;
}

//-----------------------------------------------------------------------------
// Name: GetEnvFXParams
// Desc: 
//
// Parameters:
//		AkEnvID in_envID
//
// Return:
//		IAkRTPCSubscriber*
//-----------------------------------------------------------------------------
IAkRTPCSubscriber* CAkLEngine::GetEnvFXParams( AkEnvID in_envID )
{
	CAkVPLMixBusNode ** l_ppEnvBus = NULL;

	// Check if bus already exists
	l_ppEnvBus = m_EnvBusList.Exists( in_envID );
	if( l_ppEnvBus )
	{
		if ( (*l_ppEnvBus)->GetState() == NodeStateStop )
			return NULL;
		else
			return (*l_ppEnvBus)->GetFXParams( 0 ); // env is at index 0
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Name: ExecuteCmds
// Desc: Execute the list of commands posted by the game.
//
// Parameters:
//		None.
//
// Return:
//		Ak_Success:	commands executed. 
//      AK_Fail:    Failed to execute.
//-----------------------------------------------------------------------------
void CAkLEngine::ExecuteCmds()
{
    // First pass. Add sounds for Play|PlayPaused commands newly added.

    // NOTE: Commands are enqueued with m_pVPLSrc and m_pVPL = NULL;
    // Then, for Play|PlayPaused commands, AddSound() is called, which sets
    // m_pVPLSrc, and IF THE SOURCE IS READY, connects it to a bus and sets m_pVPL.
    // ProcessCommands uses these fields to see if commands can be executed:
    // If m_pVPLSrc is set but not m_pVPL, it tries to connect the source.
    // If m_pVPLSrc is not set, it means it is a pause/resume/stop command and
    // it resolves it if it can (that is, if the source IS READY).
    // In all cases, commands are executed if and only if m_pVPL is set.
	
    AkListCmd::IteratorEx iter = m_listCmd.BeginEx();
	while( iter != m_listCmd.End() )
	{
		AkLECmd&  l_rCmd = *iter;

        // Add sound if it does not exist and a Play or PlayPaused was queued.
		if( !l_rCmd.m_pVPLSrc &&
            ( l_rCmd.m_eState == LEStatePlay ||
			  l_rCmd.m_eState == LEStatePlayPause ) )
		{
            if ( AddSound( l_rCmd ) != AK_Success )
            {
                // Add sound failed. Source was destroyed. Remove from list.
                iter = m_listCmd.Erase( iter );
            }
            else
                ++iter;
		}
        else
            ++iter;
    }

    // Now, execute all commands
    ProcessCommands( m_ulPlayEventID );
} // ExecuteCmds

//-----------------------------------------------------------------------------
// Name: ResolveCommandVPL
// Desc: Find a command's VPLSrc and VPL (for playing sounds, i.e. stop, pause, 
//       resume). Call only when resolution is needed (VPL and VPLSrc not set).
//       The command is resolved only if the source exists AND it is connected
//       to a bus.
//       If the source exists but it is not connected, the function returns
//       AK_Success but the command is not resolved.
//       If the source does not exist, the command is obsolete and the function 
//       returns AK_Fail.
//
// Parameter:
//	AkLECmd	& io_cmd: ( Stop, Pause, Resume ).
//
// Return:
// AK_Success:      The VPLSrc related to the command was found (but the 
//                  command is not resolved if the source is not connected).
// AK_Fail:         The VPLSrc does not exist.
// AK_InvalidParameter: The context is NULL, or the command was already resolved.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::ResolveCommandVPL( AkLECmd & io_cmd )
{
	// Check parameters. 
	AKASSERT( io_cmd.m_pCtx != NULL && 
              io_cmd.m_pVPL == NULL && 
              io_cmd.m_pVPLSrc == NULL &&
              ( io_cmd.m_eState == LEStateStop ||
                io_cmd.m_eState == LEStatePause ||
                io_cmd.m_eState == LEStateResume ||
				io_cmd.m_eState == LEStateStopLooping ) );
    if( io_cmd.m_pCtx == NULL ||
        io_cmd.m_pVPL != NULL ||
        io_cmd.m_pVPLSrc != NULL )
		return AK_InvalidParameter;

    CAkPBI * l_pCtx = io_cmd.m_pCtx;

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL *	l_pVPLMixBus = *iterVPL;
		for( AkListVPLSrcs::Iterator iterVPLSrc = l_pVPLMixBus->m_listVPLSrcs.Begin(); 
			 iterVPLSrc != l_pVPLMixBus->m_listVPLSrcs.End(); 
			 ++iterVPLSrc )
		{
			AkVPLSrc * l_pVPLSrc = *iterVPLSrc;
			AKASSERT( l_pVPLSrc != NULL );

			// Matching contexts found.
			if( l_pVPLSrc->m_Src.GetContext() == l_pCtx )
			{
                // Set info.
                io_cmd.m_pVPLSrc = l_pVPLSrc;
                io_cmd.m_pVPL = l_pVPLMixBus;
                return AK_Success;
			}
		}

	}

    // VPLSrc was not found. Maybe it is in the list of sources not connected.
    // Find it, but do not resolve. If it was not found, the command should be removed from the list.
    AkListVPLSrcs::Iterator iterVPLSrc = m_listSrcsNotConnected.Begin(); 
    while ( iterVPLSrc != m_listSrcsNotConnected.End() )
    {
        AkVPLSrc * l_pVPLSrc = *iterVPLSrc;
		AKASSERT( l_pVPLSrc != NULL );

		// Matching contexts found.
		if( l_pVPLSrc->m_Src.GetContext() == l_pCtx )
		{
            // Do not set info, but return that a matching VPLSrc was found.
            return AK_Success;
		}
        ++iterVPLSrc;
    }

    // No matching connected VPL source was found.
    return AK_Fail;
} // ResolveCommandVPL

//-----------------------------------------------------------------------------
// Name: AddSound
// Desc: Add the sound to a new VPL or an existing VPL to play. 
//
// Parameters:
//	CAkPBI * in_pContext :	Pointer to a sound context.
//	AkUInt8	      in_bMode	  : Playing mode, in-game, or application.
//	bool		  in_bPaused  : true = sound is paused on play.
//
// Return: 
//  In general, remove the command from the list if the code is not AK_Success.
//	Ak_Success:          Sound was added.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Invalid pointer.
//  AK_AlreadyConnected: Sound is part of a sample accurate container, already
//                       part of a cbx node. The Play command should be removed.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::AddSound( AkLECmd & io_cmd )
{
	// Check parameters.
    AKASSERT( io_cmd.m_pCtx != NULL && 
              io_cmd.m_pVPL == NULL && 
              io_cmd.m_pVPLSrc == NULL );
    if( io_cmd.m_pCtx == NULL ||
        io_cmd.m_pVPL != NULL || 
        io_cmd.m_pVPLSrc != NULL )
        return AK_InvalidParameter;

	AKRESULT l_eResult = AK_Success;
    CAkPBI * l_pCtx	   = io_cmd.m_pCtx;

	AkVPLSrc * l_pVPLSrcPrev = FindExistingVPLSrc( l_pCtx );
	if ( l_pVPLSrcPrev )
	{
 		l_eResult = l_pVPLSrcPrev->m_Src.AddSrc( l_pCtx, false );
		l_pCtx->NotifAddedAsSA();
        //AKASSERT( l_eResult == AK_Success );
        return AK_AlreadyConnected;
	}
	else // A new sound to play.
	{
		//---------------------------------------------------------------------------
		// Create a new source.
		//---------------------------------------------------------------------------
		l_eResult = VPLCreateSource( l_pCtx, &io_cmd.m_pVPLSrc );
		//AKASSERT( l_eResult == AK_Success );
        if( l_eResult == AK_FormatNotReady )
        {
            if( io_cmd.m_pVPLSrc == NULL ) goto ErrorAddSound;

            // Format is not ready. 
            // ProcessCommands() will resolve VPL connection when it can.
            // Store in list of sources not connected.
            m_listSrcsNotConnected.AddLast( io_cmd.m_pVPLSrc );
            return AK_Success;
        }
		if( l_eResult != AK_Success && l_eResult != AK_NoDataReady ) goto ErrorAddSound;
		if( io_cmd.m_pVPLSrc == NULL )								 goto ErrorAddSound;

		//---------------------------------------------------------------------------
		// Check if the bus associated with this sound was already created.
		//---------------------------------------------------------------------------
		io_cmd.m_pVPL = GetVPLMixBus( l_pCtx );
		//AKASSERT( l_pMixBus != NULL );
		if( io_cmd.m_pVPL == NULL )
		{
			l_eResult = AK_Fail;
			goto ErrorAddSound;
		}

		io_cmd.m_pVPLSrc->m_ulPlayID = m_ulPlayID++;

		//---------------------------------------------------------------------------
		// Connect the source to VPL mix bus.
		//---------------------------------------------------------------------------
		io_cmd.m_pVPL->m_listVPLSrcs.AddFirst( io_cmd.m_pVPLSrc );

		l_eResult = io_cmd.m_pVPL->m_MixBus.Connect( );
		//AKASSERT( l_eResult == AK_Success );
		if( l_eResult != AK_Success )
		{
			io_cmd.m_pVPL->m_listVPLSrcs.RemoveFirst(); // Undo connection made above.
			goto ErrorAddSound;
		}

ErrorAddSound :
		if( l_eResult != AK_Success )
		{
			if( io_cmd.m_pVPLSrc != NULL )
				VPLDestroySource( io_cmd.m_pVPLSrc );

			if( l_eResult != AK_PartialSuccess )
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
			}
		}
	}
	return l_eResult;

} // AddSound

//-----------------------------------------------------------------------------
// Name: VPLTryConnectSource
// Desc: After having added a sound whose VPLCreateSource returned 
// 		 AK_FormatNotReady (pipeline not created), the renderer should call this
//		 function to complete the connection. 
//		 Tries adding pipeline, if successful, connects to bus.
//
// Parameters:
//	CAkPBI * in_pContext :	Pointer to a sound context.
//	AkVPLSrc * in_pVPLSrc	  : Incomplete VPL source.
//  AkVPL *& out_pMixBus	  : Returned bus to which it is connected.
//
// Return: 
//	Ak_Success:          VPL connection is complete.
//  AK_FormatNotReady: 	 Source not ready yet.
//  AK_Fail:             Error. Source is destroyed.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::VPLTryConnectSource( CAkPBI * in_pContext,
                                          AkVPLSrc * in_pVPLSrc,                                       
                                          AkVPL *& out_pMixBus )
{
	AKASSERT( in_pVPLSrc );
	
	AKRESULT l_eResult = in_pVPLSrc->m_Src.AddPipelineDeferred( in_pContext );
	if ( l_eResult == AK_FormatNotReady )
	{
		// Streamed source is not ready or 
		// Frame offset greater than an audio frame: keep it in list of sources not connected.
        return l_eResult;
	}

    //---------------------------------------------------------------------------
	// Source ready: remove from non-connected srcs list, connect to bus.
	//---------------------------------------------------------------------------
    AKVERIFY( m_listSrcsNotConnected.Remove( in_pVPLSrc ) == AK_Success );

    // ...or there was an error.
    if ( l_eResult != AK_Success )
	{
		VPLDestroySource( in_pVPLSrc );
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
		return AK_Fail;
	}

	//---------------------------------------------------------------------------
	// Check if the bus associated with this sound was already created.
	//---------------------------------------------------------------------------
	out_pMixBus = GetVPLMixBus( in_pContext );
	//AKASSERT( l_pMixBus != NULL );
	if( out_pMixBus == NULL )
	{
		l_eResult = AK_Fail;
		goto ErrorVPLTryConnectSource;
	}

	in_pVPLSrc->m_ulPlayID = m_ulPlayID++;

	//---------------------------------------------------------------------------
	// Connect the source to VPL mix bus.
	//---------------------------------------------------------------------------
	out_pMixBus->m_listVPLSrcs.AddFirst( in_pVPLSrc );

	l_eResult = out_pMixBus->m_MixBus.Connect( );
	//AKASSERT( l_eResult == AK_Success );
	if( l_eResult != AK_Success )
	{
		out_pMixBus->m_listVPLSrcs.RemoveFirst(); // Undo connection made above.
		goto ErrorVPLTryConnectSource;
	}

ErrorVPLTryConnectSource :
	if( l_eResult != AK_Success )
	{
    	VPLDestroySource( in_pVPLSrc );
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
	}

	// Post Source Starvation notification if context requires sample accuracy.
#ifndef AK_OPTIMIZED
	if ( in_pVPLSrc->m_Src.m_bNotifyStarvationAtStart )
	{
		in_pVPLSrc->m_Src.m_bStreamingStarted = true;
		if ( in_pContext->GetFrameOffset() < 0 )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_StreamingSourceStarving );
			in_pVPLSrc->m_Src.m_iWasStarvationSignaled = 20;//~400ms hysteresis on starvation logs.
		}
	}
#endif

    return l_eResult;
} // VPLTryConnectSource

//-----------------------------------------------------------------------------
// Name: GetVPLMixBus
// Desc: Get the VPL mix bus that is associated with the source.
//
// Parameters:
//	CAkPBI * in_pCtx : Pointer to a source context.
//
// Return: 
//	AkVPL * : Pointer to a AkVPL.
//-----------------------------------------------------------------------------
AkVPL * CAkLEngine::GetVPLMixBus( CAkPBI * in_pCtx )
{
	AKASSERT( in_pCtx != NULL );

	AkVPL *			l_pMixBus	 = NULL;
	CAkBusCtx		l_BusContext = in_pCtx->GetBusContext();
	AkUniqueID		l_BusID		 = l_BusContext.ID();

	// Check if the mix bus was already created.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;

		if( l_pMixBus->m_BusID == l_BusID && l_pMixBus->m_MixBus.GetState() != NodeStateStop )
			return l_pMixBus;
	}


	if (in_pCtx->IsForFeedbackPipeline())	
	{
		//The inner mixer is NOT used, so allocate the smallest possible buffer.
		l_pMixBus = CreateVPLMixBus( l_BusContext, AK_SPEAKER_SETUP_MONO, 1 );
	}
	else
	{
		l_pMixBus = CreateVPLMixBus( l_BusContext, m_uOutputChannelMask, LE_MAX_FRAMES_PER_BUFFER );
	}

	if( l_pMixBus == NULL )
		return NULL;

	l_pMixBus->m_bFeedback = in_pCtx->IsForFeedbackPipeline();
	if (!in_pCtx->IsForFeedbackPipeline())
	{
		AKRESULT eResult = m_pFinalMixNode->Connect( &l_pMixBus->m_MixBus );
		if ( eResult != AK_Success )
		return NULL;
	}

	// Don't insert the FX if this is the environmental bus, because we want
	// to use this id to be the dry bus
	// The master bus itself calls SetInsertFx when (re)starting
	if( l_pMixBus && !l_BusContext.IsEnvironmental() && !l_BusContext.IsMasterBus() )
	{
		// This can fail, but even if it does not succeed inserting the effect, we still want the bus
		l_pMixBus->m_MixBus.SetInsertFx( AK_INVALID_ENV_ID ); // Set bus insert
	}

	return l_pMixBus;
}

//-----------------------------------------------------------------------------
// Name: CreateVPLMixBus
// Desc: Create a bus of specified type.
//
// Parameters:
//	bool	   in_bEnviron : true = environmental bus, false = not an env bus.
//  AkUniqueID  in_BusID   : Bus id.
//
// Return: 
//	AkVPL * : Pointer to a AkVPL.
//-----------------------------------------------------------------------------
AkVPL * CAkLEngine::CreateVPLMixBus( CAkBusCtx in_BusCtx, AkChannelMask in_uChannelMask, AkUInt16 in_usMaxFrames )
{
	AKRESULT l_eResult = AK_Success;

	// Create the VPL mix bus.
	AkVPL * l_pMixBus;
	AkNew2( l_pMixBus, LENGINE_DEFAULT_POOL_ID, AkVPL, AkVPL() );
	if( l_pMixBus == NULL )
		return NULL;

	l_pMixBus->m_BusID = in_BusCtx.ID();

	l_eResult = l_pMixBus->m_listVPLSrcs.Init();
	if( l_eResult != AK_Success ) goto ErrorCreateVPLMixBus;

	l_eResult = l_pMixBus->m_MixBus.Init( in_uChannelMask, in_usMaxFrames );
	if( l_eResult != AK_Success ) goto ErrorCreateVPLMixBus;
	l_pMixBus->m_MixBus.m_BusContext = in_BusCtx;

	if( m_arrayVPLs.AddLast( l_pMixBus ) == NULL )
		l_eResult = AK_Fail;

ErrorCreateVPLMixBus :

	if( l_eResult != AK_Success )
	{
		if( l_pMixBus != NULL )
		{
			l_pMixBus->m_listVPLSrcs.Term();
			l_pMixBus->m_MixBus.Term();
			AkDelete2( LENGINE_DEFAULT_POOL_ID, AkVPL, l_pMixBus );
			l_pMixBus = NULL;
		}
	}
	return l_pMixBus;

} // CreateVPLMixBus

//-----------------------------------------------------------------------------
// Name: DestroyVPLMixBus
// Desc: Destroy a specified bus
//
// Parameters:
//	AkVPL * in_pMixBus : Pointer to a AkVPL.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::DestroyVPLMixBus( AkVPL * in_pMixBus )
{
	AKASSERT( in_pMixBus != NULL );

	// Destroy the mixer.
	in_pMixBus->m_MixBus.Term();

	// Destroy all sources.
	AkListVPLSrcs & listVPLSrcs = in_pMixBus->m_listVPLSrcs;
	while( !listVPLSrcs.IsEmpty() )
	{
		AkVPLSrc * l_pVPLSrc  = listVPLSrcs.First();
		listVPLSrcs.RemoveFirst();

		l_pVPLSrc->m_Src.Stop(); 
		VPLDestroySource( l_pVPLSrc );

	}
	listVPLSrcs.Term();

	// Destroy the mix bus.
	AkDelete2( LENGINE_DEFAULT_POOL_ID, AkVPL, in_pMixBus );
} // DestroyVPLMixBus

//-----------------------------------------------------------------------------
// Name: DestroyAllVPLMixBusses
// Desc: Destroy all mix busses.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::DestroyAllVPLMixBusses( void )
{
	AkArrayVPL::Iterator iterVPLMixBus = m_arrayVPLs.Begin();
	while( iterVPLMixBus != m_arrayVPLs.End() )
	{
		DestroyVPLMixBus( *iterVPLMixBus );
		++iterVPLMixBus;
	}
	m_arrayVPLs.RemoveAll();
}

//-----------------------------------------------------------------------------
// Name: ProcessCommands
// Desc: Process the commands of the commands list.
// Note: When this function is called, commands in the list must be sorted by 
//       their sequence number.
//       Play and PlayPaused commands must have their m_VPLSrc set with a previous 
//       AddSound(). However, they might not be connected yet (m_pVPL == NULL).
//       Connection will be attempted for unconnected Play|Play_Paused commands.
//       The source of all Play|Play_Paused commands that have the same sequence 
//       number must be connected before the command is actually executed.
//       All other commands must have their source connected before being 
//       executed. These commands are resolved to their VPL and VPLSrc herein,
//       when it is appropriate to do so.       
//       Order of commands with the same sequence number matters.
// Note: It is not possible that other commands be enqueued before the 
//       play, because the upper engine does not send them when the PBI does
//       not exist. However, it should not be assumed at this level. Fix.
//-----------------------------------------------------------------------------
void CAkLEngine::ProcessCommands( AkUInt32 in_ulTag )
{
StartOverProcCmds:
    AkListCmd::IteratorEx iter = m_listCmd.BeginEx();
	while( iter != m_listCmd.End() )
    {
        // Verify that all VPLs with this sequence number are ready before executing any Play|Play_paused command.
        bool bDoSkipSeqNumber = false;
        AkUInt32 ulCurSeqNumber = (*iter).m_ulSequenceNumber;
        AkListCmd::IteratorEx iterProc = iter;
        
        while ( iterProc != m_listCmd.End( ) &&
                ulCurSeqNumber == (*iterProc).m_ulSequenceNumber )
        {
            AkLECmd& l_rItemCheck = *iterProc;

            // Check for Play commands with sources not ready.
            if ( !l_rItemCheck.m_pVPL &&
                 ( l_rItemCheck.m_eState == LEStatePlay ||
                   l_rItemCheck.m_eState == LEStatePlayPause ) )
            {
            	AKASSERT( l_rItemCheck.m_pVPLSrc );
                // Not ready.
                // Try connect source now. If source becomes ready, it will be connected therein and
                // l_rItemCheck.m_pVPL will be set.
                if ( VPLTryConnectSource( l_rItemCheck.m_pCtx,
 	                                      l_rItemCheck.m_pVPLSrc,                                       
 	                                      l_rItemCheck.m_pVPL ) == AK_Fail )
				{
                    // Failure: Source was destroyed. Remove from event list.
                    // Clean command lists for that context.
                    m_listCmd.Erase( iterProc );
                    // Start over.
                    goto StartOverProcCmds;
				}
                else if ( !l_rItemCheck.m_pVPL )
                {
                    // Found a source not ready to play. Skip that sequence number.
                    bDoSkipSeqNumber = true;
                }
                // Else source is ready.
            }
            ++iterProc;
        }

        if ( bDoSkipSeqNumber )
        {
            // The sequence number must be skipped. Find next command that has a different sequence number.
            do
            {
                ++iter;
            }
            while ( iter != m_listCmd.End( ) &&
                    ulCurSeqNumber == (*iter).m_ulSequenceNumber );
        }
        else
        {
            // Execute command if the source is connected.
            AkLECmd & l_rItem = *iter;
            bool l_bIsCmdValid = true;

            // Resolve stop, pause and resume commands that are not yet identified to a VPL source.
            if ( !l_rItem.m_pVPLSrc )
            {
                AKASSERT( l_rItem.m_eState == LEStateStop ||
                          l_rItem.m_eState == LEStatePause ||
                          l_rItem.m_eState == LEStateResume || 
						  l_rItem.m_eState == LEStateStopLooping );
                if ( ResolveCommandVPL( l_rItem ) != AK_Success )
                {
                    iter = m_listCmd.Erase( iter );
                    l_bIsCmdValid = false;
                }
                // Either the command was invalidated and removed from the list,
                // or it was not assigned a VPLSrc because it is not ready,
                // or the command was fully resolved.
                AKASSERT( !l_bIsCmdValid ||
                          !l_rItem.m_pVPLSrc ||
                          ( l_rItem.m_pVPLSrc && l_rItem.m_pVPL ) );
            }

            // Do not consider the command if !l_bIsCmdValid; it was removed
            if ( l_bIsCmdValid )
            {
                // Execute command if source is connected. Otherwise, skip it.
                if ( l_rItem.m_pVPL )
                {
					AKASSERT( l_rItem.m_pVPLSrc );
					CAkVPLSrcCbxNode & cbx = l_rItem.m_pVPLSrc->m_Src;

                    // Source is connected. Execute command.
                    switch( l_rItem.m_eState )
				    {
				    case LEStatePlay:
						cbx.Start();
					    break;

				    case LEStatePlayPause:
						cbx.Start();
						cbx.Pause();
					    break;

					case LEStateStop:
						cbx.Stop();
						break;

					case LEStatePause:
						cbx.Pause();
						break;

				    case LEStateResume:
					    cbx.Resume();
					    break;

					case LEStateStopLooping:
					    cbx.StopLooping();
						break;

				    default:
					    AKASSERT(!"Unsupported action type");
					    break;
				    }

                    // Command executed: dequeue.
				    iter = m_listCmd.Erase( iter );
			    }
                else
                {
                    // Source is not ready. Skip command (leave it in the queue).
                    ++iter;
                } // if source ready/connected.
            }
		} // if do skip seq number.
	} // for all commands.
} // PerformCommands

//-----------------------------------------------------------------------------
// Name: FindExistingVPLSrc
// Desc: Find an existing VPLSrc for the given PBI. Used for sample-accurate transitions.
//-----------------------------------------------------------------------------
AkVPLSrc * CAkLEngine::FindExistingVPLSrc( CAkPBI * in_pCtx )
{
	// Check parameters.
	AKASSERT( in_pCtx	!= NULL );

	AkUniqueID	l_ID	  = in_pCtx->GetSequenceID();

	if( l_ID == AK_INVALID_SEQUENCE_ID )
		return NULL;

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
    {
		AkVPL * l_pVPL = *iterVPL;

		for( AkListVPLSrcs::Iterator iterSrc = l_pVPL->m_listVPLSrcs.Begin(); iterSrc != l_pVPL->m_listVPLSrcs.End(); ++iterSrc )
		{
			// Should only need to look at the first sound to see if there is a match.
			AkVPLSrc * l_pVPLSrc = *iterSrc;
			AKASSERT( l_pVPLSrc != NULL );
			AKASSERT( l_pVPLSrc->m_Src.GetContext() != NULL );

			if( l_pVPLSrc->m_Src.GetContext()->GetSequenceID() == l_ID )
				return l_pVPLSrc;
		}
	}

	return NULL;
} // FindExistingVPLSrc

//-----------------------------------------------------------------------------
// Name: VPLCreateSource
// Desc: Create a VPL source.
//
// Parameters:
//	CAkPBI * in_pCtx	   : Pointer to a sound context.
//	AkVPLSrc *	  in_pVPLSrc   : Previous sound in list.
//	AkVPLSrc **   out_ppVPLSrc : Pointer returned to a VPL src struct.
//
// Return:
//	Ak_Success:          VPL created successfully.
//  AK_NoDataReady:		 Source is accessing hd/dvd data not ready.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Invalid pointer.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::VPLCreateSource( CAkPBI * in_pCtx,
									  AkVPLSrc **   out_ppVPLSrc )
{
    // Check parameters.
    AKASSERT( in_pCtx	   != NULL );
	AKASSERT( out_ppVPLSrc != NULL );
    if( in_pCtx == NULL || out_ppVPLSrc == NULL )
        return AK_InvalidParameter;

	AKRESULT l_eResult = AK_Success;

	*out_ppVPLSrc = NULL;

	//-------------------------------------------------------------------------
    // Create a VPL source struct.
	//-------------------------------------------------------------------------
	AkVPLSrc * l_pVPLSrc;
	AkNew2( l_pVPLSrc, LENGINE_DEFAULT_POOL_ID, AkVPLSrc, AkVPLSrc );
	if( l_pVPLSrc == NULL )
	{
		l_eResult = AK_Fail;
		goto ErrorVPLCreateSource;
	}

#ifndef AK_OPTIMIZED
	l_pVPLSrc->m_ID		= m_VPLPipelineID++;	// Profiling id.
#endif

	if (in_pCtx->IsForFeedbackPipeline())
		l_eResult = l_pVPLSrc->m_Src.Init(AK_FEEDBACK_SAMPLE_RATE, AK_FEEDBACK_MAX_FRAMES_PER_BUFFER);
	else
		l_eResult = l_pVPLSrc->m_Src.Init(AK_CORE_SAMPLERATE, LE_MAX_FRAMES_PER_BUFFER);

#ifndef AK_OPTIMIZED
	l_pVPLSrc->m_Src.m_bNotifyStarvationAtStart = in_pCtx->DoNotifyStarvationAtStart();
#endif
	AKASSERT( l_eResult == AK_Success );
	if( l_eResult == AK_Success )
	{
		// NOTE: on error the Context is destroyed in AddSrc();
		l_eResult = l_pVPLSrc->m_Src.AddSrc( in_pCtx, true );
		*out_ppVPLSrc = l_pVPLSrc;
		return l_eResult;
	}

ErrorVPLCreateSource:

    if( l_eResult != AK_Success && l_eResult != AK_NoDataReady )
    {
		if( l_pVPLSrc != NULL )
		{
			l_pVPLSrc->m_Src.Stop();
			VPLDestroySource( l_pVPLSrc );
		}
		AKVERIFY( in_pCtx->Stop() == AK_Success );
		AKVERIFY( in_pCtx->Destroy( CtxDestroyReasonPlayFailed ) == AK_Success );
		*out_ppVPLSrc = NULL;
    }

    return l_eResult;
} // VPLCreateSource

//-----------------------------------------------------------------------------
// Name: VPLDestroySource
// Desc: Destroy a specified source.
//
// Parameters:
//	AkVPLSrc * in_pVPLSrc : Pointer to source to stop.
//-----------------------------------------------------------------------------
void CAkLEngine::VPLDestroySource( AkVPLSrc * in_pVPLSrc )
{
	// Check parameters.
	AKASSERT( in_pVPLSrc != NULL );

	in_pVPLSrc->m_Src.Term();

	// Free the context.
	AkDelete2( LENGINE_DEFAULT_POOL_ID, AkVPLSrc, in_pVPLSrc );
} // VPLDestroySource

//-----------------------------------------------------------------------------
// Name: RemoveVPLMixBussesSources
// Desc: Remove VPL sources that are no longer playing.
//
// Parameters:
//	None.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::RemoveVPLMixBussesSources()
{
	AkArrayVPL::Iterator iterVPLMixBus = m_arrayVPLs.Begin();
	while( iterVPLMixBus != m_arrayVPLs.End() )
	{
		AkVPL * l_pVPLMixBus = *iterVPLMixBus;
		RemoveVPLMixBusSources( l_pVPLMixBus );

		// Delete the bus if it does not have active sources.
		if(   l_pVPLMixBus->m_MixBus.GetState() != NodeStatePlay &&
			  l_pVPLMixBus->m_listVPLSrcs.Length() == 0 )
		{
			DestroyVPLMixBus( l_pVPLMixBus );
			iterVPLMixBus = m_arrayVPLs.EraseSwap( iterVPLMixBus );
		}
		else
			++iterVPLMixBus;
	}

} // RemoveVPLMixBussesSources

//-----------------------------------------------------------------------------
// Name: RemoveVPLMixBusSources
// Desc: Remove VPL sources that are no longer playing.
//
// Parameters:
//	AkVPL * in_pMixBus : Pointer to mix bus of the sound.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::RemoveVPLMixBusSources( AkVPL * in_pMixBus )
{
	AKASSERT( in_pMixBus != NULL );

	// Remove sources that are not active.
	AkListVPLSrcs::IteratorEx iterSrc = in_pMixBus->m_listVPLSrcs.BeginEx();
	while( iterSrc != in_pMixBus->m_listVPLSrcs.End() )
	{
		AkVPLSrc * l_pVPLSrc = *iterSrc;
		AKASSERT( l_pVPLSrc != NULL );

		// Disconnect from the Mix bux node is source was stopped.
		if( l_pVPLSrc->m_Src.GetState() == NodeStateStop )
		{
			AKRESULT l_eResult = in_pMixBus->m_MixBus.Disconnect( );
			AKASSERT( l_eResult == AK_Success );

			// Delete any pending commands that refer to this VPL source.
			AkListCmd::IteratorEx iterCmd = m_listCmd.BeginEx();
			while( iterCmd != m_listCmd.End() )
			{
				AkLECmd& l_rCmd = *iterCmd;
		 
				if( l_rCmd.m_pCtx == l_pVPLSrc->m_Src.GetContext() )
					iterCmd = m_listCmd.Erase( iterCmd );
				else
					++iterCmd;
			}

			iterSrc = in_pMixBus->m_listVPLSrcs.Erase( iterSrc );

			// Destroy the source.
			VPLDestroySource( l_pVPLSrc );
		}
		else
			++iterSrc;
	}
} // RemoveVPLMixBusSources

//-----------------------------------------------------------------------------
// Name: AllocVoice
// Desc: Allocate a voice on the Voice Manager.
//
// Parameters:
//  AkUInt32			in_ulBufSize	 : Size of the buffer to allocate.
//	CVoiceInfo**	out_ppVoice		 : Returned pointer to an allocated voice.
//
// Return: 
//	Ak_Success:          Voice was allocated.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Voice was not allocated.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::AllocVoice()
{
    VoiceParameters	l_sVoiceParam;

	l_sVoiceParam.uNumRefills		= g_PDSettings.uNumRefillsInVoice;
	l_sVoiceParam.hEventPacketDone  = m_hEventPacketDone;

    // Setup voice audio format.
    XAUDIOSOURCEFORMAT srcFormat;

	// PCM Data.
	srcFormat.ChannelCount = (XAUDIOCHANNEL)GetNumChannels( m_uOutputChannelMask );
	srcFormat.SampleType = XAUDIOSAMPLETYPE_FLOAT;
    srcFormat.SampleRate = AK_CORE_SAMPLERATE;                         // Sample rate, in Hz

    l_sVoiceParam.pAudioFormat = &srcFormat;

	return g_pAkSink->Start( l_sVoiceParam );
}

//-----------------------------------------------------------------------------
// Name: SequencerVoiceFilling
// Desc: Manage the voice sink node.
//
// Parameters:
//	AkUInt32& io_NumBufferToRefill
//
// Return: 
//	Ak_Success:	Data passed to voice.
//  AK_Fail:    Failed to pass data to the voice.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::SequencerVoiceFilling( AkUInt32& io_NumBufferToRefill )
{
	AKASSERT( g_pAkSink != NULL );

	AkPipelineBuffer	l_sBuffer;
	l_sBuffer.Clear();
	l_sBuffer.SetRequestSize( AK_NUM_VOICE_REFILL_FRAMES );
	l_sBuffer.SetChannelMask( m_uOutputChannelMask );
	GetBuffer( &l_sBuffer );
	AkUInt16 l_usFrames = l_sBuffer.uValidFrames;

	AKRESULT l_eResult;

	if( l_usFrames > 0 )
	{
		g_pAkSink->m_RBLock.Lock();
		l_eResult = g_pAkSink->PassData( l_sBuffer );
		g_pAkSink->m_RBLock.Unlock();
		l_sBuffer.uValidFrames = l_usFrames;
		m_pFinalMixNode->ReleaseBuffer( &l_sBuffer );
	}
	else
	{
		// Pass silence buffer.
		g_pAkSink->m_RBLock.Lock();
		l_eResult = g_pAkSink->PassSilence( AK_NUM_VOICE_REFILL_FRAMES );
		g_pAkSink->m_RBLock.Unlock();
	}

	g_pAkSink->m_RBLock.Lock();

	if ( g_pAkSink->IsDataNeededVoice() && g_pAkSink->IsDataReadyRB() )
		g_pAkSink->SubmitPacketRB();

	--io_NumBufferToRefill;
	AkUInt32 uBuffersNeeded = g_pAkSink->IsDataNeededRB();
	io_NumBufferToRefill = AkMin( uBuffersNeeded, io_NumBufferToRefill );
	
	g_pAkSink->m_RBLock.Unlock();

	g_pPlayingMgr->NotifyMarkers( m_pMarkers, m_cMarkers );

    return l_eResult;
}

#include "AkVPLSrcNode.h"
#include "AkVPLSrcCbxNode.h"
#include "AkVPLFilterNode.h"
#include "AkVPLPitchNode.h"
#include "AkVPLLPFNode.h"

/*
void CAkLEngine::RunVPL( AkRunningVPL & io_runningVPL )
{
	CAkVPLSrcCbxNode * pCbx = & io_runningVPL.pSrc->m_Src;
	AkVPLSrcCbxRec * pCbxRec = & pCbx->m_ListSrc[ pCbx->m_uSrcIdx ];

	while (true)
	{
		switch ( io_runningVPL.state.result )
		{
		case AK_DataNeeded:
			io_runningVPL.state.iPos++;

			switch ( io_runningVPL.state.iPos )
			{
			case 0:
				pCbx->GetBuffer( io_runningVPL.state );
				break;
			case 1:
				pCbxRec->m_pLPF->GetBuffer( io_runningVPL.state );
				break;
			case 2:
				if ( !pCbxRec->m_pFilter )
					break; // skip this one
				pCbxRec->m_pFilter->GetBuffer( io_runningVPL.state );
				break;
			case 3:
				pCbxRec->m_pPitch->GetBuffer( io_runningVPL.state );
				break;
			case 4:
				io_runningVPL.state.buffer.MaxFrames() = LE_MAX_FRAMES_PER_BUFFER;
				pCbxRec->m_pSrc->GetBuffer( io_runningVPL.state );
				break;
			default:
				AKASSERT( false );
				io_runningVPL.state.result = AK_Fail;
				return;
			}

			break;

		case AK_DataReady:
		case AK_NoMoreData:
			AKASSERT( io_runningVPL.state.buffer.uValidFrames != 0 ); // If you're here, the node at iPos has broken the rules!

			io_runningVPL.state.iPos--;

			switch ( io_runningVPL.state.iPos )
			{
			case -3: // mixer
				io_runningVPL.pBus->m_MixBus.ConsumeBuffer( io_runningVPL.state );
				break;
			case -2: // environmental obstruction
				if ( io_runningVPL.state.bIsEnvironmentalBus )
				{
					io_runningVPL.pSrc->m_Src.ProcessBufferForObstruction( &io_runningVPL.state.buffer );
					for(int iChannel=0; iChannel<runningVPL.state.buffer.uNumChannels; iChannel++)
						io_runningVPL.state.buffer.AudioMix[iChannel] = io_runningVPL.state.DirectMix[iChannel];
				}
				break;
			case -1: // environment
				// skip the rest of the processing chain if not audible.
				if ( !io_runningVPL.state.bAudible )
				{
					return;
				}
				if ( io_runningVPL.state.bIsEnvironmentalBus )
				{
					CAkPBI * pCtx = io_runningVPL.pSrc->m_Src.GetContext();

					for( int i = 0; i < ( 2 * AK_MAX_ENVIRONMENTS_PER_OBJ ); i++ )
					{
						if( ( io_runningVPL.state.aMergedValues[i].envValue.EnvID != AK_INVALID_ENV_ID ) &&
							( io_runningVPL.state.aMergedValues[i].envValue.fControlValue >= EPSILON_CONTROL_VALUE ) )
						{
							//Check if the bus already exists (create it if needed)
							CAkVPLMixBusNode * pEnvBus = GetEnvironmentalBus( io_runningVPL.state.aMergedValues[i].envValue.EnvID, pCtx );
							if( pEnvBus )
							{
								// Set volume on buffer
								for(int iChannel=0; iChannel<io_runningVPL.state.buffer.uNumChannels; iChannel++)
								{
									ApplyVolumes(io_runningVPL.state.buffer.AudioMix[iChannel].Next, io_runningVPL.state.buffer.EnvMix[iChannel].Next, io_runningVPL.state.aMergedValues[i].envValue.fControlValue);
									ApplyVolumes(io_runningVPL.state.buffer.AudioMix[iChannel].Previous, io_runningVPL.state.buffer.EnvMix[iChannel].Previous, io_runningVPL.state.aMergedValues[i].fLastControlValue);
								}

								// Add buffer to env bus
								pEnvBus->ConsumeBuffer( io_runningVPL.state );
							}
						}
					} //for
				}
				break;
			case 0:
				pCbx->ConsumeBuffer( io_runningVPL.state );
				pCbxRec = & pCbx->m_ListSrc[ pCbx->m_uSrcIdx ]; // because m_uSrcIdx might have changed as a result of previous call
				break;
			case 1:
				pCbxRec->m_pLPF->ConsumeBuffer( io_runningVPL.state );
				break;
			case 2:
				if ( !pCbxRec->m_pFilter )
					break; // skip this one
				pCbxRec->m_pFilter->ConsumeBuffer( io_runningVPL.state );
				break;
			case 3:
				pCbxRec->m_pPitch->ConsumeBuffer( io_runningVPL.state );
				break;
			case 4:
				AKASSERT( false );
				io_runningVPL.state.result = AK_Fail;
				return;
			default:
				AKASSERT( io_runningVPL.state.iPos < 0 );
				io_runningVPL.pSrc->m_Src.ReleaseBuffer();
				return; // finished
			}
			break;

		default:
			return;
		}
	}
}
*/

// Optimised version of single-pipeline execution.
void CAkLEngine::RunVPL( AkRunningVPL & io_runningVPL )
{
	CAkVPLSrcCbxNode * AK_RESTRICT pCbx = & io_runningVPL.pSrc->m_Src;
	AkVPLSrcCbxRec * AK_RESTRICT pCbxRec = pCbx->m_pCbxRec[0];

	AkUInt32 uFXIndex = AK_NUM_EFFECTS_PER_OBJ;

GetFilter:
	while ( uFXIndex > 0 )
	{
		CAkVPLFilterNode * pFilter = pCbxRec->m_pFilter[ --uFXIndex ];
		if ( pFilter )
		{
			pFilter->GetBuffer( io_runningVPL.state );
		if ( io_runningVPL.state.result != AK_DataNeeded )
		{
			if ( io_runningVPL.state.result == AK_DataReady
				|| io_runningVPL.state.result == AK_NoMoreData )
			{
					++uFXIndex;
					goto ConsumeFilter;
			}
			else
			{
				return;
			}
		}
	}
	}

	// Pitch-FmtConv-Source loop

	pCbxRec->m_Pitch.GetBuffer( io_runningVPL.state );
	if ( io_runningVPL.state.result != AK_DataNeeded )
	{
		if ( io_runningVPL.state.result == AK_DataReady
			|| io_runningVPL.state.result == AK_NoMoreData )
		{
            goto ConsumeFilter;
		}
		else
		{
			return;
		}
	}

	do
	{
		if (io_runningVPL.bFeedbackVPL)
			io_runningVPL.state.buffer.SetRequestSize( AK_FEEDBACK_MAX_FRAMES_PER_BUFFER );
		else
			io_runningVPL.state.buffer.SetRequestSize( LE_MAX_FRAMES_PER_BUFFER );

		pCbxRec->m_pSrc->GetBuffer( io_runningVPL.state );
#ifndef AK_OPTIMIZED
		if ( io_runningVPL.state.result == AK_DataReady 
			|| io_runningVPL.state.result == AK_NoMoreData )
		{
			if( pCbx->m_iWasStarvationSignaled )
			{
				--pCbx->m_iWasStarvationSignaled;
			}
		}
		else if ( io_runningVPL.state.result == AK_NoDataReady )
		{
			if( pCbx->m_bStreamingStarted && !pCbx->m_iWasStarvationSignaled )
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_StreamingSourceStarving );
				pCbx->m_iWasStarvationSignaled = 20;//~400ms hysteresis on starvation logs.
			}
		}
#endif
		if ( io_runningVPL.state.result != AK_DataReady
			&& io_runningVPL.state.result != AK_NoMoreData )
		{
            return;
		}

		pCbxRec->m_Pitch.ConsumeBuffer( io_runningVPL.state );
		if ( io_runningVPL.state.result != AK_DataNeeded )
		{
			if ( io_runningVPL.state.result == AK_DataReady
				|| io_runningVPL.state.result == AK_NoMoreData )
			{
#ifndef AK_OPTIMIZED
				pCbx->m_bStreamingStarted = true;
#endif
                goto ConsumeFilter;
			}
			else
			{
				return;
			}
		}
	}
	while(1);

ConsumeFilter:
	while ( uFXIndex < AK_NUM_EFFECTS_PER_OBJ )
	{
		CAkVPLFilterNode * pFilter = pCbxRec->m_pFilter[ uFXIndex++ ];
		if ( pFilter )
		{
			pFilter->ConsumeBuffer( io_runningVPL.state );
		if ( io_runningVPL.state.result != AK_DataReady
			&& io_runningVPL.state.result != AK_NoMoreData )
		{
            return;
		}
	}
	}

	pCbxRec->m_LPF.ConsumeBuffer( io_runningVPL.state );
	AKASSERT( io_runningVPL.state.result == AK_DataReady
		|| io_runningVPL.state.result == AK_NoMoreData ); // LPF has no failure case

	pCbx->ConsumeBuffer( io_runningVPL.state );

	if ( io_runningVPL.state.result != AK_DataReady
		&& io_runningVPL.state.result != AK_NoMoreData )
	{
		if ( io_runningVPL.state.result == AK_DataNeeded )
		{
			pCbxRec = pCbx->m_pCbxRec[0]; // because m_uSrcIdx might have changed as a result of pCbx->ConsumeBuffer
			goto GetFilter;
		}
		else
		{
			return;
		}
	}

	CAkVPLNode::MergeMarkers( &io_runningVPL.state.buffer, m_cMarkers, m_pMarkers );

	//Route audio to the feedback pipeline, if needed.
	if(IsFeedbackEnabled())
	{
		if (m_pDeviceMgr->PrepareAudioProcessing(io_runningVPL))
		{
			m_pDeviceMgr->ApplyMotionLPF(io_runningVPL);
			m_pDeviceMgr->ConsumeVPL(io_runningVPL);
			m_pDeviceMgr->CleanupAudioVPL(io_runningVPL);
		}
		else
			m_pDeviceMgr->ConsumeVPL(io_runningVPL);
	}

	// skip the rest of the processing chain if not audible.
	if ( !io_runningVPL.state.bAudible )
		return;

	if ( io_runningVPL.state.bIsEnvironmentalBus )
	{
		AkAudioMix DirectMix[AK_VOICE_MAX_NUM_CHANNELS];
		AkUInt32 uNumChannels = io_runningVPL.state.buffer.NumChannels();
		for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
			DirectMix[iChannel] = io_runningVPL.state.buffer.AudioMix[iChannel]; //backup our values

		CAkPBI * AK_RESTRICT pCtx = io_runningVPL.pSrc->m_Src.GetContext();

		for( int i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; i++ )
		{
			if( io_runningVPL.state.aMergedValues[i].envValue.EnvID == AK_INVALID_ENV_ID )
				break;

			//Check if the bus already exists (create it if needed)
			CAkVPLMixBusNode * pEnvBus = GetEnvironmentalBus( io_runningVPL.state.aMergedValues[i].envValue.EnvID, pCtx );
			if( pEnvBus )
			{
				// Set volume on buffer
				for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
				{
					ApplyVolumes(io_runningVPL.state.buffer.AudioMix[iChannel].Next, io_runningVPL.state.buffer.EnvMix[iChannel].Next, io_runningVPL.state.aMergedValues[i].envValue.fControlValue);
					ApplyVolumes(io_runningVPL.state.buffer.AudioMix[iChannel].Previous, io_runningVPL.state.buffer.EnvMix[iChannel].Previous, io_runningVPL.state.aMergedValues[i].fLastControlValue);
				}

				// Add buffer to environmental bus
				pEnvBus->ConsumeBuffer( io_runningVPL.state );
			}
		} //for

		io_runningVPL.pSrc->m_Src.ProcessBufferForObstruction( &io_runningVPL.state.buffer );
		for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
			io_runningVPL.state.buffer.AudioMix[iChannel] = DirectMix[iChannel]; //restore backup'ed values
	}

	if (!io_runningVPL.bFeedbackVPL)
		io_runningVPL.pBus->m_MixBus.ConsumeBuffer( io_runningVPL.state );
}

void CAkLEngine::GetBuffer( AkPipelineBufferBase * io_pBuffer )
{
	g_pPositionRepository->UpdateTime();

    // Consume one look-ahead frame of each source that is not yet connected.
    AkListVPLSrcs::Iterator it = m_listSrcsNotConnected.Begin();
	while ( it != m_listSrcsNotConnected.End() )
	{
		CAkPBI *pPBI = (*it)->m_Src.GetContext();
		AkUInt32 uFrames = LE_MAX_FRAMES_PER_BUFFER;
		if (pPBI->IsForFeedbackPipeline())
			uFrames = AK_FEEDBACK_MAX_FRAMES_PER_BUFFER;

		pPBI->ConsumeFrameOffset( uFrames );
		++it;
	}

	for ( AkArrayVPL::Iterator itVPL = m_arrayVPLs.Begin(); itVPL != m_arrayVPLs.End(); ++itVPL )
	{
		AkVPL * pVPL = (*itVPL);
		if ( !pVPL->m_listVPLSrcs.IsEmpty() )
		{
			bool bIsEnvironmentalBus = pVPL->m_MixBus.m_BusContext.IsEnvironmental();
	
			// Process all sources for this bus
			for ( AkListVPLSrcs::Iterator itSrc = pVPL->m_listVPLSrcs.Begin(); itSrc != pVPL->m_listVPLSrcs.End(); ++itSrc )
			{
				AkRunningVPL runningVPL;
	
				//Do not call EMPTY_BUFFER() to avoid setting fields in case of virtual voice
				runningVPL.state.buffer.SetRequestSize( io_pBuffer->MaxFrames() );
				runningVPL.state.bIsEnvironmentalBus = bIsEnvironmentalBus;
				runningVPL.state.bPause = false;
				runningVPL.state.bStop = false;
				runningVPL.state.result = AK_DataNeeded;
				runningVPL.bFeedbackVPL = pVPL->m_bFeedback;
				runningVPL.pFeedbackData = NULL;
				if(runningVPL.bFeedbackVPL)
					runningVPL.state.buffer.SetRequestSize( AK_FEEDBACK_MAX_FRAMES_PER_BUFFER );
				if ( (*itSrc)->m_Src.StartRun( runningVPL.state ) )
				{
					runningVPL.pSrc = *itSrc;
					runningVPL.pBus = pVPL;
	
					// Clear only necessary fields of buffer
					// Keep request size ("max frames").
					AKASSERT( !runningVPL.state.buffer.HasData() );
					runningVPL.state.buffer.uValidFrames = 0;
					runningVPL.state.buffer.uNumMarkers = 0;
					runningVPL.state.buffer.pMarkers = NULL;
					runningVPL.state.buffer.posInfo.uStartPos = -1;
	
					if ( bIsEnvironmentalBus )
					{
						AkEnvironmentValue l_EnvironmentValues[ AK_MAX_ENVIRONMENTS_PER_OBJ ];
						runningVPL.pSrc->m_Src.GetContext()->GetEnvironmentValues( l_EnvironmentValues );
						const AkEnvironmentValue *pLastValues = runningVPL.pSrc->m_Src.GetLastEnvironmentalValues();
	
						MergeLastAndCurrentValues( pLastValues, l_EnvironmentValues, runningVPL.state.aMergedValues );
	
						runningVPL.pSrc->m_Src.SetLastEnvironmentalValues( l_EnvironmentValues ); //keep info for next call
					}
	
					if (runningVPL.bFeedbackVPL)
					{
						AK_START_TIMER_FEEDBACK();
						RunVPL( runningVPL );
						AK_STOP_TIMER_FEEDBACK();
					}
					else
						RunVPL( runningVPL );

					// Release buffer in all cases, except if the source has starved.
					// In such a case it is kept for next LEngine pass.
					if ( runningVPL.state.result != AK_NoDataReady )
						runningVPL.pSrc->m_Src.ReleaseBuffer();
				}
	
				if ( runningVPL.state.result == AK_NoMoreData || runningVPL.state.result == AK_Fail || runningVPL.state.bStop )
					(*itSrc)->m_Src.Stop();
				else if ( runningVPL.state.bPause )
					(*itSrc)->m_Src.Pause();
			}
		}
	
		// Push the normal bus buffer to the final mix
		
		if (!pVPL->m_bFeedback)
		{
			AkAudioBufferFinalMix* pBuffer;

			// Get the resulting buffer for this bus
			pVPL->m_MixBus.GetResultingBuffer( pBuffer );

			// Add this buffer to the final mix node
			m_pFinalMixNode->ConsumeBuffer( pBuffer );

			// We can release the buffer right away since the final mixer works in incremental mode (+=)
			pVPL->m_MixBus.ReleaseBuffer();
		}
	}

	// Push the environmental busses buffers to the final mix
	AkEnvBusList::IteratorEx iterEnvBus = m_EnvBusList.BeginEx();
	while( iterEnvBus != m_EnvBusList.End() )
	{
		AkAudioBufferFinalMix* pBuffer;

		CAkVPLMixBusNode * l_pEnvMixBus = (*iterEnvBus).item;
		// Get the resulting buffer for this env bus
		l_pEnvMixBus->GetResultingBuffer( pBuffer );

		// Add this buffer to the final mix node
		m_pFinalMixNode->ConsumeBuffer( pBuffer );

		l_pEnvMixBus->ReleaseBuffer();

		if ( l_pEnvMixBus->GetState() != NodeStatePlay )
		{
			DestroyEnvMixBus( l_pEnvMixBus );
			iterEnvBus = m_EnvBusList.Erase( iterEnvBus );
		}
		else
		{
			++iterEnvBus;
		}
	}

	// Final Mix!

	m_pFinalMixNode->GetResultingBuffer( io_pBuffer );
}

//---------------------------------------------------------------------
// Check if the Voice is starving, kill a voice if it needs to.
//---------------------------------------------------------------------
void CAkLEngine::HandleStarvation()
{
	if( g_pAkSink->IsStarved() == true )
	{
		g_pAkSink->ResetStarved();

		AkUInt32 uTimeNow = g_pAudioMgr->GetBufferTick();
		if( m_uLastStarvationTime == 0 ||
			uTimeNow - m_uLastStarvationTime > MINIMUM_STARVATION_NOTIFICATION_DELAY )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_VoiceStarving );
			m_uLastStarvationTime = uTimeNow;
		}
	}

	//Go through all the feedback devices too
	if (IsFeedbackEnabled())
		m_pDeviceMgr->HandleStarvation();
}

//-----------------------------------------------------------------------------
// Name: GetEnvironmentalBus
// Desc: Returns (or create if needed) the wanted environmental bus.
//
// Parameters:
//  AkEnvID  in_EnvID		  : Environmental id.
//  CAkPBI * in_pCtx : Audio context
//
// Return: 
//	CAkVPLMixBusNode * : Pointer to a CAkVPLMixBusNode.
//-----------------------------------------------------------------------------
CAkVPLMixBusNode * CAkLEngine::GetEnvironmentalBus(AkEnvID			 in_EnvID, 
												   CAkPBI * in_pCtx )
{
	CAkVPLMixBusNode ** l_ppEnvBus = NULL;

	// Check if bus already exists
	l_ppEnvBus = m_EnvBusList.Exists( in_EnvID );
	if(l_ppEnvBus == NULL)
	{
		CAkVPLMixBusNode * l_pNewEnvBus = NULL;
		CAkBusCtx		   l_BusContext = in_pCtx->GetBusContext();

		// Create bus
		l_pNewEnvBus = CreateEnvMixBus( in_EnvID, l_BusContext );
		if( l_pNewEnvBus )
		{
			if( l_pNewEnvBus->SetInsertFx( in_EnvID ) != AK_Success )
			{
				l_pNewEnvBus->ReleaseBuffer();
				m_EnvBusList.Unset( l_pNewEnvBus->GetEnvID() );
				DestroyEnvMixBus( l_pNewEnvBus );
				return NULL;
			}
			// Setup previous volumes for environement and disconnect immediately because it must not be tracked 
			// in connection count which does not apply to environemental mechanism.
			l_pNewEnvBus->Connect();
			l_pNewEnvBus->Disconnect();
		}

		return l_pNewEnvBus;
	}

	return *l_ppEnvBus;
}


//-----------------------------------------------------------------------------
// Name: CreateEnvMixBus
// Desc: Create an environmental bus.
//
// Parameters:
//  AkEnvID  in_EnvID   : Environmental id.
//
// Return: 
//	CAkVPLMixBusNode * : Pointer to a CAkVPLMixBusNode.
//-----------------------------------------------------------------------------
CAkVPLMixBusNode * CAkLEngine::CreateEnvMixBus( AkEnvID   in_EnvID,
											    CAkBusCtx in_BusContext )
{
	AKRESULT l_eResult = AK_Success;

	// Create the VPL mix bus.
	CAkVPLMixBusNode * l_pMixBus;
	AkNew2( l_pMixBus, LENGINE_DEFAULT_POOL_ID, CAkVPLMixBusNode, CAkVPLMixBusNode() );
	if( l_pMixBus == NULL )
		return NULL;

	l_eResult = l_pMixBus->Init( m_uOutputChannelMask, LE_MAX_FRAMES_PER_BUFFER );
	if( l_eResult != AK_Success ) goto ErrorCreateEnvMixBus;
	l_pMixBus->m_BusContext = in_BusContext;

	// Add the bus to the list
	if( !m_EnvBusList.Set( in_EnvID, l_pMixBus ) )
	{
		l_eResult = AK_Fail;
		goto ErrorCreateEnvMixBus;
	}


ErrorCreateEnvMixBus :

	if( l_eResult != AK_Success )
	{
		if( l_pMixBus != NULL )
		{
			l_pMixBus->Term();
			AkDelete2( LENGINE_DEFAULT_POOL_ID, CAkVPLMixBusNode, l_pMixBus );
			l_pMixBus = NULL;
		}
	}
	return l_pMixBus;

} // CreateEnvMixBus

//-----------------------------------------------------------------------------
// Name: DestroyEnvMixBus
// Desc: Destroy a specified bus
//
// Parameters:
//	AkVPL * in_pMixBus : Pointer to a AkVPL.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::DestroyEnvMixBus( CAkVPLMixBusNode * in_pMixBus )
{
	AKASSERT( in_pMixBus != NULL );

	// Destroy the mixer.
	in_pMixBus->Term();

	// Destroy the mix bus.
	AkDelete2( LENGINE_DEFAULT_POOL_ID, CAkVPLMixBusNode, in_pMixBus );
} // DestroyEnvMixBus

//-----------------------------------------------------------------------------
// Name: DestroyAllEnvMixBusses
// Desc: Destroy all environmental mix busses.
//
// Parameters:
//	None.
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::DestroyAllEnvMixBusses( void )
{
	AkEnvBusList::Iterator iterEnv = m_EnvBusList.Begin();
	while( iterEnv != m_EnvBusList.End() )
	{
		DestroyEnvMixBus((*iterEnv).item);
		++iterEnv;
	}
}

//-----------------------------------------------------------------------------
// Name: MergeLastAndCurrentValues
// Desc: Merge two sets of environmental values.
//
// Parameters:
//	const AkEnvironmentValue *in_pLastValues	: Old values
//	const AkEnvironmentValue *in_pNewValues		: New values
//	AkMergedEnvironmentValue *io_paMergedValues : Merged values
//
// Return: 
//	None.
//-----------------------------------------------------------------------------
void MergeLastAndCurrentValues(const AkEnvironmentValue * AK_RESTRICT in_pLastValues, 
							   const AkEnvironmentValue * AK_RESTRICT in_pNewValues,
							   AkMergedEnvironmentValue * AK_RESTRICT io_paMergedValues)
{
	AKASSERT( in_pNewValues );
	AKASSERT( in_pLastValues );
	AKASSERT( io_paMergedValues );

	// copy new values into merged
	AkUInt32 cMergedValues = 0;
	while( cMergedValues < AK_MAX_ENVIRONMENTS_PER_OBJ && in_pNewValues[cMergedValues].EnvID != AK_INVALID_ENV_ID )
	{
		io_paMergedValues[cMergedValues].envValue = in_pNewValues[cMergedValues];
		io_paMergedValues[cMergedValues].fLastControlValue = 0.0f;
		++cMergedValues;
	}

	// merge old values with new values
	for ( AkUInt32 uIterLastValues = 0; uIterLastValues < AK_MAX_ENVIRONMENTS_PER_OBJ; ++uIterLastValues )
	{
		AkUInt32 eEnvID = in_pLastValues[uIterLastValues].EnvID;
        if ( eEnvID == AK_INVALID_ENV_ID )
			break;

		bool bFound = false;

		for( AkUInt32 uIterMergedValues = 0; uIterMergedValues < cMergedValues; uIterMergedValues++ )
		{
			if(io_paMergedValues[uIterMergedValues].envValue.EnvID == eEnvID)
			{
				io_paMergedValues[uIterMergedValues].fLastControlValue = in_pLastValues[uIterLastValues].fControlValue;
				bFound = true;
				break;
			}
		}
		
		// put 'dying' value into merged values, but only if we have space -- new values have priority.
		if ( !bFound && cMergedValues < AK_MAX_ENVIRONMENTS_PER_OBJ )
		{
			io_paMergedValues[cMergedValues].envValue.EnvID = eEnvID;
			io_paMergedValues[cMergedValues].envValue.fControlValue = 0.0f;
			io_paMergedValues[cMergedValues].fLastControlValue = in_pLastValues[uIterLastValues].fControlValue;
			++cMergedValues;
		}
	}

	// terminate values array
	if ( cMergedValues < AK_MAX_ENVIRONMENTS_PER_OBJ )
		io_paMergedValues[cMergedValues].envValue.EnvID = AK_INVALID_ENV_ID;
}

//-----------------------------------------------------------------------------
// Name: EnableFeedbackPipeline
// Desc: Enable the feedback pipeline.  This should be called only if there are
//		 devices to drive.  This can be called many times and once enabled, it
//		 stays enabled.
//
// Parameters:None.
// Return: None.
//-----------------------------------------------------------------------------

void CAkLEngine::EnableFeedbackPipeline()
{
	m_pDeviceMgr = CAkFeedbackDeviceMgr::Get();
}

bool CAkLEngine::IsFeedbackEnabled()
{
	return m_pDeviceMgr != NULL && m_pDeviceMgr->IsFeedbackEnabled();
}