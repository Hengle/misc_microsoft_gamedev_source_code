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
// AkVPLFinalMixNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkVPLFinalMixNode.h"
#include "AudioLibDefs.h"
#include "AkLEngine.h"

// Effect
#include "AkEffectsMgr.h"
#include "AkFXMemAlloc.h"
#include "AkMonitor.h"
#include "AkAudioLibTimer.h"
#include "AkMath.h"

extern CAkBusCtx g_MasterBusCtx;

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the source.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::Init( AkUInt32 in_uChannelMask )
{
	AKRESULT l_eResult = m_Mixer.Init( in_uChannelMask, LE_MAX_FRAMES_PER_BUFFER);
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

	ResetVolumes();
	m_eState = NodeStateStop;
	m_BufferOut.Clear();
	m_BufferOut.eState = AK_NoMoreData;
#ifndef AK_PS3
	m_MasterOut.Clear();
#endif

	m_ulBufferOutSize = LE_MAX_FRAMES_PER_BUFFER * GetNumChannels( in_uChannelMask ) * sizeof(AkReal32);
	void * pData = AkAlloc( g_LEngineDefaultPoolId, m_ulBufferOutSize );
	if ( !pData )
		return AK_InsufficientMemory;
	
	AKPLATFORM::AkMemSet( pData, 0, m_ulBufferOutSize );
	m_BufferOut.AttachContiguousDeinterleavedData( 
		pData,						// Buffer.
		LE_MAX_FRAMES_PER_BUFFER,	// Buffer size (in sample frames).
		0,							// Valid frames.
		in_uChannelMask );			// Chan config.
	
#ifndef AK_PS3
	pData = AkAlloc( g_LEngineDefaultPoolId, m_ulBufferOutSize );
	if ( !pData )
		return AK_InsufficientMemory;
	
	AKPLATFORM::AkMemSet( pData, 0, m_ulBufferOutSize );
	m_MasterOut.AttachInterleavedData( 
		pData,						// Buffer.
		LE_MAX_FRAMES_PER_BUFFER,	// Buffer size (in sample frames).
		0,							// Valid frames.
		in_uChannelMask );			// Chan config.
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
AKRESULT CAkVPLFinalMixNode::Term()
{
	m_Mixer.Term();

	if( m_BufferOut.HasData() )
	{
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetContiguousDeinterleavedData() );
		m_BufferOut.ClearData();
	}

#ifndef AK_PS3
	if( m_MasterOut.HasData() )
	{
		AkFree( g_LEngineDefaultPoolId, m_MasterOut.GetContiguousDeinterleavedData() );
		m_MasterOut.ClearData();
	}
#endif

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
			g_MasterBusCtx.UnsubscribeRTPC( fx.pParam );
			CAkLEngine::LockFxParams();
			fx.pParam->Term( AkFXMemAlloc::GetLower( ) );
			fx.pParam = NULL;
			CAkLEngine::UnlockFxParams();
		}
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
AKRESULT CAkVPLFinalMixNode::ReleaseBuffer( AkPipelineBuffer * io_pBuffer )
{
	// Assume output buffer was entirely consumed by client. Do not actually release the memory here.
	// Clear markers for next iteration
	m_BufferOut.uValidFrames = 0;

	if ( m_BufferOut.eState == AK_NoMoreData )
	{
		m_eState = NodeStateStop;
	}
	else
	{
		m_eState = NodeStatePlay;
	}
		
	// Reset state in case AddAndMixBuffer does not get called again
	m_BufferOut.eState = AK_NoMoreData;

#ifndef AK_PS3
	m_MasterOut.uValidFrames = 0;
#endif

	// Also clean the data for next iteration.
	if ( m_BufferOut.HasData() )
		AKPLATFORM::AkMemSet(m_BufferOut.GetContiguousDeinterleavedData(), 0, m_ulBufferOutSize);
	// no need to clear m_MasterOut buffer since the data will be overwritten on final mix

	return AK_Success;
} // ReleaseBuffer

//-----------------------------------------------------------------------------
// Name: Connect
// Desc: Connects the specified effect as input.
//
// Parameters:
//	CAkVPLNode * in_pInput : Pointer to the effect to connect.
//
// Return:
//	Ak_Success: Effect input was connected.
//  AK_Fail:    Failed to connect to the effect.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::Connect( CAkVPLMixBusNode * in_pInput )
{
	if ( m_eState != NodeStatePlay )
	{
		if( !g_MasterBusCtx.HasEffect() )
		{
			ResetVolumes();
		}
		else
		{
			SetNextVolume( g_MasterBusCtx.GetVolume() );
			SetNextLfe( g_MasterBusCtx.GetLfe() );
			TagPreviousVolumes();
		}
	}

	return AK_Success;
} // Connect

//-----------------------------------------------------------------------------
// Name: ConsumeBuffer
// Desc: Mix input buffer with current output buffer
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to a buffer object.
//
// Return:
//	Ak_Success: Buffer was returned.
//  AK_Fail:    Failed to return a buffer.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::ConsumeBuffer( AkAudioBufferFinalMix*& io_rpBuffer)
{
	AKRESULT	l_eResult = AK_Success;
	// The tail of the mix bus from which this is coming from is not considered here on PS3
#ifndef AK_PS3
	if( io_rpBuffer->uValidFrames > 0 )
#endif	
	{
		// Set plugin audio buffer eState before execution (it will get updated by the effect chain)
		m_BufferOut.eState = AK_DataReady;
	
		// Master bus is revived. Initialize the effect on it if necessary to consider parameter changes.
		if ( m_eState == NodeStateStop )
		{
			// This can fail, but if it does we still want to mix the buffer without the effect
			// It also cleans up for itself and the node will know no effect need to be inserted
			SetInsertFx();	
		}
		m_eState = NodeStatePlay;
#ifdef AK_PS3
		if(m_Mixer.FinalExecuteSPU(io_rpBuffer, &m_BufferOut) != AK_ProcessNeeded)
		{
			l_eResult = AK_NoDataReady;
		}
#else
		// apply the volumes and final mix with previous buffer (if any)
		switch( io_rpBuffer->GetChannelMask() )
		{

		case AK_SPEAKER_SETUP_STEREO:
			m_Mixer.MixStereo( io_rpBuffer, &m_BufferOut );
			break;

		case AK_SPEAKER_SETUP_5POINT1:
			m_Mixer.Mix51( io_rpBuffer, &m_BufferOut );
			break;

/*
		case AK_SPEAKER_SETUP_7POINT1 :
			m_Mixer.Mix71( io_rpBuffer, &m_BufferOut );
			break;
*/

		default:
			AKASSERT(!"Unsupported number of channels");
			l_eResult = AK_NoDataReady;
			break;
		}	
#endif
	}
	return l_eResult;
}

//-----------------------------------------------------------------------------
// Name: GetResultingBuffer
// Desc: Get the resulting mixed buffer.
//
// Parameters:
//	AkAudioBuffer* io_pBuffer : Pointer to a buffer object.
//
// Return:
//	Ak_Success: Buffer was returned.
//  AK_Fail:    Failed to return a buffer.
//-----------------------------------------------------------------------------
void CAkVPLFinalMixNode::GetResultingBuffer( AkPipelineBufferBase*& io_rpBuffer )
{ 
	// Execute the master bus insert effect.
	if( m_eState == NodeStatePlay )
	{
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			FX & fx = m_aFX[ uFXIndex ];
			if ( fx.pEffect != NULL )
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
						CAkLEngine::QueueDspProcess(pDspProcess); // needs to be declared sync so that it happens in sequence after mixing of busses into final
						CAkLEngine::QueueSync();
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

		m_bLastBypassAllFX = m_bBypassAllFX;
		// Pad FX tail to maxFrames
		// PS3: Deal with the fact that uValidFrames is not known from plug-in execution yet
		m_BufferOut.uValidFrames = m_BufferOut.MaxFrames(); 
						
	}

	// Apply volume using mixer
	// set the next volume value
	AkReal32 l_fTemp = AkMath::Min( GetNextVolume(), 1.0f );
	AkAudioMix * pBusVolumes = &m_BufferOut.AudioMix;
	pBusVolumes->Next.Set( l_fTemp );

	// set the previous volume value
	l_fTemp = AkMath::Min( GetPreviousVolume(), 1.0f );

	pBusVolumes->Previous.Set( l_fTemp );

	// set the next lfe volume value
	pBusVolumes->Next.volumes.fLfe = AkMath::Min( GetNextLfe(), 1.0f );

	// set the previous lfe volume value
	pBusVolumes->Previous.volumes.fLfe = AkMath::Min( GetPreviousLfe(), 1.0f );
	

#ifdef AK_PS3
	// this will queue the volume job
	m_Mixer.FinalInterleaveExecuteSPU(&m_BufferOut, io_rpBuffer);
	io_rpBuffer->uValidFrames = m_BufferOut.uValidFrames;
#else
	m_MasterOut.uValidFrames = m_BufferOut.uValidFrames;

	// apply the volumes and final mix (overwriting destination buffer)
	switch( m_BufferOut.GetChannelMask() )
	{
	case AK_SPEAKER_SETUP_STEREO :
		m_Mixer.MixFinalStereo( &m_BufferOut, &m_MasterOut );
		break;

	case AK_SPEAKER_SETUP_5POINT1 :
		m_Mixer.MixFinal51( &m_BufferOut, &m_MasterOut );
		break;

#ifdef AK_71AUDIO
	case AK_SPEAKER_SETUP_7POINT1 :
		m_Mixer.MixFinal71( &m_BufferOut, &m_MasterOut );
		break;
#endif
	default:
		AKASSERT(!"Unsupported number of channels");
		break;
	}
#endif

	// get volumes ready for next time
	TagPreviousVolumes();

#ifndef AK_PS3
	// update the input
	*io_rpBuffer = m_MasterOut;
#endif
} // MixBuffers

//-----------------------------------------------------------------------------
// Name: SetInsertFx
// Desc: Set a bus insert fx if one is not present.
//
// Return:
//	AK_Succcess : the FX was correctly initiated.
//  otherwise, either the FX was not registered or there was no FX to add,
//  or there was not enough memory to instanciate the FX parameters.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLFinalMixNode::SetInsertFx()
{
	AKRESULT l_eResult = AK_Success;

	//---------------------------------------------------------------------
	// Create the master bus insert effects.
	//---------------------------------------------------------------------	 
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];

		AkFXDesc	fxDesc;
		g_MasterBusCtx.GetFX( uFXIndex, fxDesc );

		// Delete any existing effect
		if( fx.pEffect != NULL )
		{
			fx.pEffect->Term( AkFXMemAlloc::GetLower() );
			AK_DECREMENT_PLUGIN_COUNT( fx.id );
			if ( fx.pBusFXContext )
			{
				AkDelete( g_LEngineDefaultPoolId, fx.pBusFXContext );
				fx.pBusFXContext = NULL;
			}

			fx.pEffect = NULL;
		}

		fx.id = AK_INVALID_PLUGINID;

		if( fx.pParam )
		{
			// LockFxParams must not be locked when calling UnsubscribeRTPC().
			g_MasterBusCtx.UnsubscribeRTPC( fx.pParam );
			CAkLEngine::LockFxParams();
			fx.pParam->Term( AkFXMemAlloc::GetLower( ) );

			fx.pParam = NULL;
			CAkLEngine::UnlockFxParams();
		}	

		if( fxDesc.EffectTypeID == AK_INVALID_PLUGINID )
			continue;

		l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), fxDesc.EffectTypeID, (IAkPlugin*&)fx.pEffect );
		if( l_eResult != AK_Success )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
			return l_eResult;
		}

		AK_INCREMENT_PLUGIN_COUNT( fxDesc.EffectTypeID );

		AKASSERT( fxDesc.pParam != NULL ); //Impossible as long Environmental are not in

		fx.pParam  = fxDesc.pParam->Clone( AkFXMemAlloc::GetLower() );
		if ( fx.pParam == NULL )
			l_eResult = AK_Fail;
		else
			l_eResult = g_MasterBusCtx.SubscribeRTPC( fx.pParam, fxDesc.EffectTypeID );
		
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
													reinterpret_cast<IAkPluginParam*>( fx.pParam ),
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
				g_MasterBusCtx.UnsubscribeRTPC( fx.pParam );
				CAkLEngine::LockFxParams();
				fx.pParam->Term( AkFXMemAlloc::GetLower( ) );

				fx.pParam = NULL;
				CAkLEngine::UnlockFxParams();
			}
		}
	}

	m_bBypassAllFX = g_MasterBusCtx.GetBypassAllFX();

	return l_eResult;
} // SetInsertFx


IAkRTPCSubscriber * CAkVPLFinalMixNode::GetFXParams( AkPluginID	in_FXID )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		FX & fx = m_aFX[ uFXIndex ];
		if ( in_FXID == fx.id )
			return fx.pParam;
	}

	return NULL; // not found
}

//-----------------------------------------------------------------------------
// Name: SetInsertFxBypass
// Desc: Bypass the effect of master bus
//
//-----------------------------------------------------------------------------
void CAkVPLFinalMixNode::SetInsertFxBypass( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if ( in_uTargetMask & ( 1 << uFXIndex ) )
			m_aFX[ uFXIndex ].bBypass = ( in_bitsFXBypass & ( 1 << uFXIndex ) ) != 0;
	}

	if ( in_uTargetMask & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) )
		m_bBypassAllFX = ( in_bitsFXBypass & ( 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) ) != 0;
} // SetInsertFxBypass
