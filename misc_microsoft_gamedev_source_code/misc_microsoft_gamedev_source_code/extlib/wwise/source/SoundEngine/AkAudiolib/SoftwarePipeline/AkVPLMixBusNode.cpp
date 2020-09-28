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

/////////////////////////////////////////////////////////////////////
//
// AkVPLMixBusNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLMixBusNode.h"
#include "AudioLibDefs.h"
#include "AkEffectsMgr.h"
#include "AkMonitor.h"
#include "AkMath.h"
#include "AkFXMemAlloc.h"
#include "AkAudioLibTimer.h"
#include "AkVPLSrcCbxNode.h"
#include "AkLEngine.h"

#include "AkEnvironmentsMgr.h"

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the source.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::Init( AkChannelMask in_uChannelMask, AkUInt16 in_uMaxFrames )
{
	AKRESULT l_eResult = m_Mixer.Init( in_uChannelMask, in_uMaxFrames );
	AKASSERT( l_eResult == AK_Success );

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];
		fx.id = AK_INVALID_PLUGINID;
		fx.pParam = NULL;
		fx.pEffect = NULL;
		fx.pBusFXContext = NULL;
		fx.bBypass = 0;
		fx.bLastBypass = 0;
	}
	m_bBypassAllFX = 0;
	m_bLastBypassAllFX = 0;

	m_eState							= NodeStateIdle;

	ResetVolumes();
	m_fPreviousEnvVolume				= 1.0f;

	m_ulBufferOutSize					= in_uMaxFrames * GetNumChannels( in_uChannelMask ) * sizeof(AkReal32);
	m_EnvID								= AK_INVALID_ENV_ID;
	m_uConnectCount						= 0;

	m_BufferOut.Clear();
	m_BufferOut.eState					= AK_NoMoreData;
	void * pData = AkAlloc( g_LEngineDefaultPoolId, m_ulBufferOutSize );
	if ( !pData )
		return AK_InsufficientMemory;

	AKPLATFORM::AkMemSet( pData, 0, m_ulBufferOutSize );
	m_BufferOut.AttachContiguousDeinterleavedData( 
		pData,						// Buffer.
		in_uMaxFrames,				// Buffer size (in sample frames).
		0,							// Valid frames.
		in_uChannelMask );			// Chan config.
	
	AKPLATFORM::AkMemSet( &m_BufferOut.AudioMix, 0, sizeof(m_BufferOut.AudioMix) );

#ifdef AK_PS3
	m_pLastItemMix = NULL;
#endif

	return l_eResult;
} // Init


//-----------------------------------------------------------------------------
// Name: Term
// Desc: Terminate.
//
// Parameters:
//
// Return:
//	AKRESULT
//		AK_Success : terminated successfully.
//		AK_Fail    : failed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::Term()
{
	m_Mixer.Term();	 

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];

		if( fx.pEffect != NULL )
		{
			fx.pEffect->Term( AkFXMemAlloc::GetLower() );
			fx.pEffect = NULL;
			AK_DECREMENT_PLUGIN_COUNT( fx.id );
		}

		if ( fx.pBusFXContext != NULL )
		{
			AkDelete( g_LEngineDefaultPoolId, fx.pBusFXContext );
			fx.pBusFXContext = NULL;
		}

		if( fx.pParam )
		{
			// LockFxParams must not be locked when calling UnsubscribeRTPC().
			m_BusContext.UnsubscribeRTPC( fx.pParam );
			CAkLEngine::LockFxParams();
			fx.pParam->Term( AkFXMemAlloc::GetLower( ) );
			fx.pParam = NULL;
			CAkLEngine::UnlockFxParams();
		}
	}

	if( m_BufferOut.HasData() )
	{
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetContiguousDeinterleavedData() );
		m_BufferOut.ClearData();
	}

	return AK_Success;
} // Term

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to the buffer object to release.
//
// Return:
//	Ak_Success: Buffer was relesed.
//  AK_Fail:    Failed to release the buffer.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::ReleaseBuffer()
{		
	// Assume output buffer was entirely consumed by client.
	if( m_BufferOut.HasData() )
	{
		if ( m_BufferOut.eState == AK_NoMoreData )
		{
  			m_eState = NodeStateIdle;
		}
		else
		{
			m_eState = NodeStatePlay;
		}

		// Reset state in case //ConsumeBuffer does not get called again
		m_BufferOut.eState = AK_NoMoreData;

		// Assume output buffer was entirely consumed by client. Do not actually release the memory here.
		m_BufferOut.uValidFrames = 0;

		// Also clean the data for next iteration.
		AKPLATFORM::AkMemSet( m_BufferOut.GetContiguousDeinterleavedData(), 0, m_ulBufferOutSize );
	}

	return AK_Success;
} // ReleaseBuffer

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Return:
//	Ak_Success: Effect input was connected.
//  AK_Fail:    Failed to connect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::Connect(  )
{
	if ( m_eState != NodeStatePlay ) // can only do this if node is not currently outputting
	{
		SetNextVolume( m_BusContext.GetVolume() );
		SetNextLfe( m_BusContext.GetLfe() );
		TagPreviousVolumes();

		if ( m_EnvID != AK_INVALID_ENV_ID ) 
			m_fPreviousEnvVolume = g_pEnvironmentMgr->GetEnvironmentVolume( m_EnvID );
		else
			m_fPreviousEnvVolume = 1.0f;
	}

	++m_uConnectCount;

	return AK_Success;
} // Connect

//-----------------------------------------------------------------------------
// Name: Disconnect
// Desc: Disconnects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to disconnect.
//
// Return:
//	Ak_Success: Effect input was disconnected.
//  AK_Fail:    Failed to disconnect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::Disconnect( )
{
	--m_uConnectCount;
	return AK_Success;
} // Disconnect

//-----------------------------------------------------------------------------
// Name: SetInsertFx
// Desc: Set a bus insert fx if one is not present.
//
// Parameters:
//	AkEnvID in_EnvID: Environment from which to get fx.
//
// Return:
//	AK_Succcess : the FX was correctly initiated.
//  otherwise, either the FX was not registered or there was no FX to add,
//  or there was not enough memory to instanciate the FX parameters.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLMixBusNode::SetInsertFx( AkEnvID in_EnvID )
{
	AKRESULT l_eResult = AK_Success;
	m_EnvID = in_EnvID;

	//---------------------------------------------------------------------
	// Create the bus insert effect.
	//---------------------------------------------------------------------	 

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];

		AkFXDesc fxDesc;
		m_BusContext.GetFX( uFXIndex, fxDesc );
		if( fxDesc.EffectTypeID == fx.id )
			continue; // fx slot is empty / same

		bool l_bIsEnvironmental = ( fxDesc.EffectTypeID == AK_PLUGINID_ENVIRONMENTAL );

		// Delete any existing effect
		if( fx.pEffect != NULL )
		{
			fx.pEffect->Term( AkFXMemAlloc::GetLower() );
			fx.pEffect = NULL;

			AK_DECREMENT_PLUGIN_COUNT( fx.id );
			if ( fx.pBusFXContext )
			{
				AkDelete( g_LEngineDefaultPoolId, fx.pBusFXContext );
				fx.pBusFXContext = NULL;
			}
		}

		fx.id = AK_INVALID_PLUGINID;

		if( fx.pParam )
		{
			// LockFxParams must not be locked when calling UnsubscribeRTPC().
			m_BusContext.UnsubscribeRTPC( fx.pParam );
			CAkLEngine::LockFxParams();
			fx.pParam->Term( AkFXMemAlloc::GetLower( ) );
			fx.pParam = NULL;
			CAkLEngine::UnlockFxParams();
		}	

		if( fxDesc.EffectTypeID == AK_INVALID_PLUGINID )
			continue; // no new effect

		if( l_bIsEnvironmental )
		{
			// Overwrite the effect type and the params with the ones from the preset
			l_eResult = g_pEnvironmentMgr->GetFXParameterSetParams( in_EnvID, fxDesc.EffectTypeID, fxDesc.pParam );
			if( l_eResult != AK_Success )
				return l_eResult;
		}

		l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), fxDesc.EffectTypeID, (IAkPlugin*&) fx.pEffect );
		if( l_eResult != AK_Success )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
			return l_eResult;
		}

		AK_INCREMENT_PLUGIN_COUNT( fxDesc.EffectTypeID );

		AKASSERT( fxDesc.pParam != NULL ); //Impossible as long Environmental are not in

		fx.pParam = fxDesc.pParam->Clone( AkFXMemAlloc::GetLower() );
		if ( fx.pParam == NULL )
		{
			l_eResult = AK_Fail;
		}
		else
		{
			if( l_bIsEnvironmental )
				l_eResult = m_BusContext.SubscribeRTPCforEnv( fx.pParam, in_EnvID );
			else
				l_eResult = m_BusContext.SubscribeRTPC( fx.pParam, fxDesc.EffectTypeID );
		}
		
		if( l_eResult == AK_Success )
		{
			AkAudioFormat l_Format;
			l_Format.SetAll( AK_CORE_SAMPLERATE,
							m_Mixer.GetChannelMask(), 
							AK_LE_NATIVE_BITSPERSAMPLE,
							AK_LE_NATIVE_BITSPERSAMPLE/8*GetNumChannels(m_Mixer.GetChannelMask()),
							AK_FLOAT,
							AK_NONINTERLEAVED ); 

			fx.id = fxDesc.EffectTypeID;

			fx.pBusFXContext = AkNew( g_LEngineDefaultPoolId, CAkBusFXContext( ) );
			l_eResult = AK_Fail;
			if ( fx.pBusFXContext != NULL)
			{
				fx.bBypass = fxDesc.bIsBypassed;
				fx.pBusFXContext->SetIsSendModeEffect( l_bIsEnvironmental );
				
				AkPluginInfo PluginInfo;
				l_eResult = fx.pEffect->GetPluginInfo( PluginInfo );
				if ( PluginInfo.bIsAsynchronous != 
#ifdef AK_PS3
					true
#else
					false
#endif
				)
				{
					l_eResult = AK_Fail;
					MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 			
				}

				if ( l_eResult == AK_Success )
				{
				
					l_eResult = fx.pEffect->Init( AkFXMemAlloc::GetLower(),		// Memory allocator.
													fx.pBusFXContext,			// Bus FX context.
													fx.pParam,
													l_Format );

					if ( l_eResult == AK_UnsupportedChannelConfig )
					{
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration );	
					}

					if ( l_eResult == AK_Success )
					{
						l_eResult = fx.pEffect->Reset( );
					}
					else
					{
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );	
					}
				}
			}
		}

		if( l_eResult != AK_Success )
		{
			if ( fx.pEffect )
			{
				fx.pEffect->Term( AkFXMemAlloc::GetLower() );
				fx.pEffect = NULL;
				AK_DECREMENT_PLUGIN_COUNT( fxDesc.EffectTypeID );
			}

			if ( fx.pBusFXContext )
			{
				AkDelete( g_LEngineDefaultPoolId, fx.pBusFXContext );
				fx.pBusFXContext = NULL;
			}

			fx.id = AK_INVALID_PLUGINID;
			
			if( fx.pParam )
			{
				// LockFxParams must not be locked when calling UnsubscribeRTPC().
				m_BusContext.UnsubscribeRTPC( fx.pParam );
				CAkLEngine::LockFxParams();
				fx.pParam->Term( AkFXMemAlloc::GetLower( ) );
				fx.pParam = NULL;
				CAkLEngine::UnlockFxParams();
			}
		}
	}

	m_bBypassAllFX = m_BusContext.GetBypassAllFX();

	return l_eResult;
} // SetInsertFx


//-----------------------------------------------------------------------------
// Name: SetInsertFxBypass
// Desc: Bypass the effect of a specified bus.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if ( in_uTargetMask & ( 1 << uFXIndex ) )
			m_aFX[ uFXIndex ].bBypass = ( in_bitsFXBypass & ( 1 << uFXIndex ) ) != 0;
	}

	if ( in_uTargetMask & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )
		m_bBypassAllFX = ( in_bitsFXBypass & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) ) != 0;
}

void CAkVPLMixBusNode::ConsumeBuffer( AkVPLMixState & io_rMixState )
{
	// Now, send it to its connected bus (the dry bus in the case of an environmental)
	if( io_rMixState.buffer.uValidFrames > 0 )
	{
#ifdef AK_PS3
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
	
		if ( m_eState == NodeStateIdle )
		{
			m_eState = NodeStatePlay; // Idle mode due to virtual voices ? bring it back to play
		}

		ZeroPadBuffer( &io_rMixState.buffer ); // FIXME: this needs to move on the SPU
		io_rMixState.buffer.uValidFrames = io_rMixState.buffer.MaxFrames();

		// WG-6243  PS3: game will freeze if sound is played on a game object in more than 1 environment  
		if( io_rMixState.result != AK_ProcessNeeded )
			io_rMixState.resultPrevious = io_rMixState.result;

		if ( m_pLastItemMix == NULL )
		{
			if ( !CAkLEngine::AddExecutingBus( this ) )
			{
				io_rMixState.result = AK_InsufficientMemory;
				return;
			}

			m_Mixer.ExecuteSPU(&io_rMixState, m_BufferOut); // Only one SPU job is needed for the mix list

			if ( io_rMixState.result == AK_InsufficientMemory )
				return;
		}
		else
		{
			m_pLastItemMix->pNextItemMix = &io_rMixState;
			io_rMixState.result = AK_ProcessNeeded;
		}

		m_pLastItemMix = &io_rMixState;
		io_rMixState.pNextItemMix = NULL;
#else
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
		if ( m_eState == NodeStateIdle )
			m_eState = NodeStatePlay; // Idle mode due to virtual voices ? bring it back to play

		// Mixer requires that buffer is zeroed out if buffer is not complete.
		ZeroPadBuffer( &io_rMixState.buffer );
		io_rMixState.buffer.uValidFrames = io_rMixState.buffer.MaxFrames();

		// Mix with previous buffer (if any)
		m_Mixer.Mix3D( &io_rMixState.buffer, &m_BufferOut );
#endif
	}
}

void CAkVPLMixBusNode::ProcessDone( AkVPLState & io_state )
{
#ifdef AK_PS3
	io_state.result = io_state.resultPrevious;
#endif
}

//-----------------------------------------------------------------------------
// Name: GetResultingBuffer
// Desc: Returns the resulting mixed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to a buffer object.
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::GetResultingBuffer( AkAudioBufferFinalMix*& io_rpBuffer )
{
	ProcessAllFX();
	PostProcessFx( io_rpBuffer );
} // MixBuffers

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::ProcessAllFX()
{
	bool bFxProcessedUnused;
	if ( m_eState == NodeStatePlay )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			ProcessFX( uFXIndex, bFxProcessedUnused );
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::ProcessFX( AkUInt32 in_fxIndex, bool &io_bfxProcessed )
{
	if ( m_eState == NodeStatePlay 
#ifdef AK_PS3
		|| m_eState == NodeStateProcessing
#endif
		)
	{
		FX & fx = m_aFX[ in_fxIndex ];
		if( fx.pEffect != NULL )
		{
			if ( ( fx.bBypass | m_bBypassAllFX ) == 0 )
			{
				// Ensure SIMD can be used without additional considerations
				AKASSERT( m_BufferOut.MaxFrames() % 4 == 0 );
							
#ifdef AK_PS3
				AK::MultiCoreServices::DspProcess* pDspProcess = NULL;
				fx.pEffect->Execute( &m_BufferOut, pDspProcess );

				if ( pDspProcess )
				{
					CAkLEngine::QueueDspProcess(pDspProcess);
					m_eState = NodeStateProcessing;
					io_bfxProcessed = true;
				}
#else
				AK_START_PLUGIN_TIMER( fx.id );
				fx.pEffect->Execute( &m_BufferOut );
				AK_STOP_PLUGIN_TIMER( fx.id );
				AKASSERT( m_BufferOut.uValidFrames <= m_BufferOut.MaxFrames() );	// Produce <= than requested
#endif
				
			}
			else
			{
				if ( ( fx.bLastBypass | m_bLastBypassAllFX ) == 0 )
				{
					fx.pEffect->Reset( );
				}
			}

			fx.bLastBypass = fx.bBypass;
		}
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CAkVPLMixBusNode::PostProcessFx( AkAudioBufferFinalMix*& io_rpBuffer )
{
	if ( m_eState == NodeStatePlay 
#ifdef AK_PS3
		|| m_eState == NodeStateProcessing
#endif
		)
	{
		m_bLastBypassAllFX = m_bBypassAllFX;
	}
	
	AkReal32 fNextEnvVolume;
	if ( m_EnvID != AK_INVALID_ENV_ID )
	{
		fNextEnvVolume = g_pEnvironmentMgr->GetEnvironmentVolume( m_EnvID );
	}
	else
	{
		fNextEnvVolume = 1.0f;
	}

	// set the next volume value
	AkReal32 l_fTemp = AkMath::Min( GetNextVolume() * fNextEnvVolume, 1.0f );

	AkAudioMix* pBusVolumes = &m_BufferOut.AudioMix;

	pBusVolumes->Next.Set( l_fTemp );

	// set the previous volume value
	l_fTemp = AkMath::Min( GetPreviousVolume() * m_fPreviousEnvVolume, 1.0f );

	pBusVolumes->Previous.Set( l_fTemp );

	// set the next lfe volume value
	pBusVolumes->Next.volumes.fLfe = AkMath::Min( GetNextLfe() * fNextEnvVolume, 1.0f );

	// set the previous lfe volume value
	pBusVolumes->Previous.volumes.fLfe = AkMath::Min( GetPreviousLfe() * m_fPreviousEnvVolume, 1.0f );

	// get volumes ready for next time
	m_fPreviousEnvVolume = fNextEnvVolume;
	TagPreviousVolumes();

	// Watchout uValidFrames is not known yet for PS3 asynchronous processing if there is a bus effect
	// Note: Effects eState result are directly passed to the pipeline by the following
	io_rpBuffer = &m_BufferOut;
}

IAkRTPCSubscriber * CAkVPLMixBusNode::GetFXParams( AkPluginID	in_FXID )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];
		if ( in_FXID == fx.id )
			return fx.pParam;
	}

	return NULL; // not found
}
