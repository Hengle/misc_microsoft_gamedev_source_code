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
// AkSrcPhysModel.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "AkSrcPhysModel.h"

#include "AkLEngine.h"

#include "AkEffectsMgr.h"
#include "AudiolibDefs.h"
#include "AkFXMemAlloc.h"

#include "AkAudiolibTimer.h"
#include "AkMonitor.h"
//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: CAkSrcPhysModel
// Desc: Constructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcPhysModel::CAkSrcPhysModel( CAkPBI * in_pCtx )
: CAkVPLSrcNode( in_pCtx )
{
	m_pEffect  = NULL;
	m_pSourceFXContext = NULL;

    // Get an instance of the physical modelling source plugin.
    IAkPlugin*     l_pIEffect = NULL;
	AkSrcDescriptor l_SrcDesc;
	AKRESULT l_eResult = m_pCtx->GetSrcDescriptor( &l_SrcDesc );
	AKASSERT( l_eResult == AK_Success );

#ifndef AK_OPTIMIZED
	m_uiID = l_SrcDesc.uiID; // Cached copy of fx id for profiling.
#endif

	l_eResult = CAkEffectsMgr::Alloc( AkFXMemAlloc::GetLower(), l_SrcDesc.uiID, l_pIEffect );
	if( l_eResult == AK_Success )
	{
	    m_pEffect = reinterpret_cast<IAkSourcePlugin*>(l_pIEffect);
	    AK_INCREMENT_PLUGIN_COUNT( m_uiID );

		const AkSourceDesc & l_SourceInfo = m_pCtx->GetSourceDesc();

		if(  m_pEffect != NULL && l_SourceInfo.EffectTypeID != AK_INVALID_PLUGINID && l_eResult == AK_Success )
		{
			m_pSourceFXContext = AkNew( g_LEngineDefaultPoolId, CAkSourceFXContext( in_pCtx, AK::IAkStreamMgr::Get( ) ) );
			if ( m_pSourceFXContext )
			{
				// Suggested format for sources is mono native for platform, they can change it if they wish
				if (m_pCtx->IsForFeedbackPipeline())
				{
					m_AudioFormat.SetAll( 
						AK_FEEDBACK_SAMPLE_RATE,
						AK_SPEAKER_SETUP_MONO,
						AK_LE_NATIVE_BITSPERSAMPLE,
						sizeof(AkReal32),	
						AK_LE_NATIVE_SAMPLETYPE,
						AK_LE_NATIVE_INTERLEAVE
						);
				}
				else
				{
					m_AudioFormat.SetAll( 
						AK_CORE_SAMPLERATE,
						AK_SPEAKER_SETUP_MONO,
						AK_LE_NATIVE_BITSPERSAMPLE,
						sizeof(AkReal32),	
						AK_LE_NATIVE_SAMPLETYPE,
						AK_LE_NATIVE_INTERLEAVE
					  );
				}

				AkPluginInfo PluginInfo;
				m_pEffect->GetPluginInfo( PluginInfo );
				if ( PluginInfo.bIsAsynchronous != false )
				{
					l_eResult = AK_Fail;
					MONITOR_ERROR( AK::Monitor::ErrorCode_PluginExecutionInvalid ); 
				}

				if ( l_eResult == AK_Success )
				{			
			        // Initialise.
			        l_eResult = m_pEffect->Init( AkFXMemAlloc::GetLower(),								// Allocator.	
													m_pSourceFXContext,
													reinterpret_cast<IAkPluginParam*>(l_SourceInfo.pParam),// Sound parameters.
													m_AudioFormat
												);

					AKASSERT( ((m_AudioFormat.GetInterleaveID() == AK_INTERLEAVED) ^ (m_AudioFormat.GetTypeID() == AK_FLOAT))  
						|| !"Invalid output format" );

					if ( !m_AudioFormat.IsChannelConfigSupported() )
					{
						l_eResult = AK_UnsupportedChannelConfig;
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginUnsupportedChannelConfiguration );	
					}
						
			        if( l_eResult == AK_Success )
					{
						l_eResult = m_pEffect->Reset( );
	
						if ( l_eResult == AK_Success )
						{
							m_pCtx->SetPluginMediaFormat( m_AudioFormat );
						}
					}
					else
					{
						MONITOR_ERROR( AK::Monitor::ErrorCode_PluginInitialisationFailed );	
					}
				}
			}
			else
			{
				l_eResult = AK_InsufficientMemory;
			}
		}
	}
	else
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_PluginAllocationFailed );	
	}

	// Failure.
	if( l_eResult != AK_Success )
	{
		if( m_pEffect != NULL )
        {
			AKVERIFY( m_pEffect->Term( AkFXMemAlloc::GetLower() ) == AK_Success );
			if ( m_pSourceFXContext )
			{
				AkDelete( g_LEngineDefaultPoolId, m_pSourceFXContext );
				m_pSourceFXContext = NULL;
			}
			AK_DECREMENT_PLUGIN_COUNT( m_uiID );
			m_pEffect  = NULL;
        }
	}
}

//-----------------------------------------------------------------------------
// Name: ~CAkSrcPhysModel
// Desc: Destructor.
//
// Return: None.
//-----------------------------------------------------------------------------
CAkSrcPhysModel::~CAkSrcPhysModel()
{
	StopStream();
}

//-----------------------------------------------------------------------------
// Name: StartStream
// Desc: Start to stream data.
//
// Return: Ak_Success:          Stream was started.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to start streaming data.
//-----------------------------------------------------------------------------
AKRESULT CAkSrcPhysModel::StartStream()
{
	if ( m_pEffect == NULL ) 
		return AK_Fail;

	return m_pEffect->Reset( );
}

//-----------------------------------------------------------------------------
// Name: StopStream
// Desc: Stop streaming data.
//
// Return: Ak_Success:          Stream was stopped.
//         AK_InvalidParameter: Invalid parameters.
//         AK_Fail:             Failed to stop the stream.
//-----------------------------------------------------------------------------
void CAkSrcPhysModel::StopStream()
{
	ReleaseBuffer();

	if( m_pEffect != NULL )
    {
		AKVERIFY( m_pEffect->Term( AkFXMemAlloc::GetLower() ) == AK_Success );
		AK_DECREMENT_PLUGIN_COUNT( m_uiID );
    }
	if ( m_pSourceFXContext )
	{
		AkDelete( g_LEngineDefaultPoolId, m_pSourceFXContext );
		m_pSourceFXContext = NULL;
	}

	m_pEffect = NULL;
}

void CAkSrcPhysModel::GetBuffer( AkVPLState & io_state )
{
	if( m_pEffect != NULL )
	{
		if( io_state.buffer.MaxFrames() == 0 )
		{
			AKASSERT( !"Physical modeling source called with zeros size request" );
			io_state.result = AK_NoMoreData;
			return;
		}

		AkChannelMask uChannelMask = m_AudioFormat.GetChannelMask();
		
		// Allocate working buffer for effects.
		AKRESULT eResult = AK_Fail;
		if ( m_AudioFormat.GetInterleaveID() == AK_NONINTERLEAVED )
		{		
			eResult = io_state.buffer.GetCachedBuffer( io_state.buffer.MaxFrames(), uChannelMask );
		}
		else
		{
			// Note: With interleaved data, the plugin source is free to use the sample size 
			// it wants. 
			AkUInt32 uSampleFrameSize = m_AudioFormat.GetBlockAlign();
			AKASSERT( uSampleFrameSize >= GetNumChannels(uChannelMask) * m_AudioFormat.GetBitsPerSample() / 8 );
			void * pBuffer = CAkLEngine::GetCachedAudioBuffer( uSampleFrameSize * io_state.buffer.MaxFrames() );
			if ( pBuffer )
			{
				io_state.buffer.AttachInterleavedData( pBuffer, io_state.buffer.MaxFrames(), 0, uChannelMask );
				eResult = AK_Success;
			}
		}

		if ( eResult != AK_Success )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_PluginProcessingFailed );
			io_state.result = AK_Fail;
			return;
		}


		// Ensure SIMD processing is possible without extra considerations
		AKASSERT( io_state.buffer.MaxFrames() % 4 == 0 );	
		
		io_state.buffer.eState = AK_DataNeeded;

		//////////////////////////////////////////////////////////
		AK_START_PLUGIN_TIMER( m_uiID );
#ifdef AK_PS3		
		AK::MultiCoreServices::DspProcess* pDummy = NULL;
		m_pEffect->Execute( &io_state.buffer, pDummy );
		AKASSERT( pDummy == NULL && "SPU source plug-ins currently unsupported." ); 
#else
		m_pEffect->Execute( &io_state.buffer );
#endif		
		AK_STOP_PLUGIN_TIMER( m_uiID );
		//////////////////////////////////////////////////////////

		io_state.result = io_state.buffer.eState;

		// Copy buffer data for later release.
		m_pluginBuffer = io_state.buffer;
	}
	else
	{
		io_state.buffer.Clear();
		io_state.result = AK_Fail;
	}
	
	// Graceful plugin error handling.
	if ( io_state.result == AK_Fail )
	{
		MONITOR_ERROR( AK::Monitor::ErrorCode_PluginProcessingFailed );
		return;
	}
}

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Release a specified buffer.
//-----------------------------------------------------------------------------
void CAkSrcPhysModel::ReleaseBuffer()
{
	if( m_pluginBuffer.HasData() )
	{
		if ( m_AudioFormat.GetInterleaveID() == AK_NONINTERLEAVED )
		{
			m_pluginBuffer.ReleaseCachedBuffer();
		}
		else
		{
			CAkLEngine::ReleaseCachedAudioBuffer( m_AudioFormat.GetBlockAlign() * LE_MAX_FRAMES_PER_BUFFER, m_pluginBuffer.GetInterleavedData() );
			m_pluginBuffer.ClearData();
		}
	}
}

void CAkSrcPhysModel::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		if ( m_pEffect != NULL ) 
			m_pEffect->Reset( );
	}
}

//-----------------------------------------------------------------------------
// Name: GetDuration()
// Desc: Get the duration of the source.
//
// Return: AkTimeMs : duration of the source.
//
//-----------------------------------------------------------------------------
AkTimeMs CAkSrcPhysModel::GetDuration( void ) const
{
	return m_pEffect->GetDuration();
}

//-----------------------------------------------------------------------------
// Name: StopLooping()
// Desc: Called on actions of type break
//-----------------------------------------------------------------------------
AKRESULT CAkSrcPhysModel::StopLooping()
{
	return m_pEffect->StopLooping();
}
