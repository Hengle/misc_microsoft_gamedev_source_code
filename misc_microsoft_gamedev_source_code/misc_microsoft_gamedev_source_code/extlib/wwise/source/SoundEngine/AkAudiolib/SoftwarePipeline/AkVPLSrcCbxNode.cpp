/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

/////////////////////////////////////////////////////////////////////
//
// AkVPLSrcCbxNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkVPLSrcCbxNode.h"

#include "AkLEngine.h"

#include "AkVPLSrcNode.h"
#include "AkVPLSrcCbxNode.h"
#include "AkVPLFilterNode.h"
#include "AkVPLPitchNode.h"
#include "AkVPLLPFNode.h"
#include "AudioLibDefs.h"
#include "AkMonitor.h"
#include "AkMath.h"
#include "Ak3DListener.h"
#include "AkEnvironmentsMgr.h"
#include "AkPositionRepository.h"
#include "AkSpeakerPan.h"

#define MAX_NODES		(3+AK_NUM_EFFECTS_PER_OBJ)	// Max nodes in the cbx node list.

AkReal32 g_fVolumeThreshold = AK_OUTPUT_THRESHOLD;
AkReal32 g_fVolumeThresholdDB = AK_MINIMUM_VOLUME_LEVEL;

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the source.
//
// Return:
//	AKRESULT
//		AK_Success : Initialised successfully.
//		AK_Fail    : failed to initialize.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::Init( AkUInt32 in_iSampleRate, AkUInt16 in_usMaxFrames )
{
	m_eState = NodeStateInit;

	for ( AkUInt32 i = 0; i < MAX_NUM_SOURCES; ++i )
		m_pCbxRec[i] = NULL;

	memset( &m_PreviousDirect, 0, sizeof(m_PreviousDirect));
	memset( &m_PreviousEnv, 0, sizeof(m_PreviousEnv));
	m_bFirstBufferProcessed = false;
	m_bLastAudible = true;
	m_bPreviousSilent = true;
	m_bLastObsLPF = false;
	m_iSampleRate = in_iSampleRate;
	m_usMaxFrames = in_usMaxFrames;
	
#ifndef AK_OPTIMIZED
	m_bNotifyStarvationAtStart = false;
	m_bStreamingStarted = false;
	m_iWasStarvationSignaled = 0;
#endif

	for(int i=0; i<AK_MAX_ENVIRONMENTS_PER_OBJ; i++)
	{
		m_LastEnvironmentValues[i].EnvID		 = AK_INVALID_ENV_ID;
		m_LastEnvironmentValues[i].fControlValue = 0.0f;
	}

	return AK_Success ;
} // Init

void CAkVPLSrcCbxNode::Term()
{
	// Destroy all sources.
	for( AkUInt32 i = 0; i < MAX_NUM_SOURCES; i++ )
	{
		AkVPLSrcCbxRec * pSrcRec = m_pCbxRec[i];
		if ( !pSrcRec )
			break;

		DeleteCbxRec( pSrcRec, CtxDestroyReasonFinished );
		m_pCbxRec[i] = NULL;
	}

	m_ObstructionLPF.Term();
} // Term

void CAkVPLSrcCbxNode::ReleaseMemory()
{
	// Release Some memory.
	for( AkUInt32 i = 0; i < MAX_NUM_SOURCES; i++ )
	{
		AkVPLSrcCbxRec * pSrcRec = m_pCbxRec[i];
		if ( !pSrcRec )
			break;

		pSrcRec->m_pSrc->StopStream();
		
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
		{
			if( pSrcRec->m_pFilter[ uFXIndex ] != NULL )
				pSrcRec->m_pFilter[ uFXIndex ]->ReleaseMemory();
		}

		pSrcRec->m_Pitch.ReleaseBuffer();

	}
} // ReleaseMemory

//-----------------------------------------------------------------------------
// Name: Start
// Desc: Indication that processing will start.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------

void CAkVPLSrcCbxNode::Start()
{
	// Do not start node if not in init state.
	if( m_eState != NodeStateInit )
	{
		// Only report error if sound was not stopped... otherwise a false error.
		if( m_eState != NodeStateStop )
		{
			Stop();
			MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
		}
		return;
	}

	// Start the current node
	m_pCbxRec[0]->m_pSrc->Start();

	m_eState = NodeStatePlay;
} // Start

//-----------------------------------------------------------------------------
// Name: Stop
// Desc: Indication that processing will stop.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::Stop()
{
	// Stop the current node

	AKASSERT( m_pCbxRec[0] );
	m_pCbxRec[0]->m_pSrc->Stop();

	m_eState = NodeStateStop;
} // Stop

//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Pause the source.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void  CAkVPLSrcCbxNode::Pause()
{
	// Do not pause node if not in play state.
	if( m_eState != NodeStatePlay )
	{
		Stop();
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
		return;
	}

	// Pause the current node

	AKASSERT( m_pCbxRec[0] );
	m_pCbxRec[0]->m_pSrc->Pause();

	m_eState = NodeStatePause;
} // Pause

//-----------------------------------------------------------------------------
// Name: Resume
// Desc: Resume the source.
//
// Parameters:
//	None.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::Resume()
{
	// Do not resume node if not in pause state
    if( m_eState != NodeStatePause )
	{
		Stop();
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
		return;
	}

	// Resume the current node

	AKASSERT( m_pCbxRec[0] );
	m_pCbxRec[0]->m_pSrc->Resume( m_pCbxRec[0]->m_Pitch.GetLastRate() );

	m_eState = NodeStatePlay;
} // Resume

bool CAkVPLSrcCbxNode::StartRun( AkVPLState & io_state )
{
	// If not playing.
	if(m_eState != NodeStatePlay)
		return false;

	CAkPBI * pSrcContext = m_pCbxRec[0]->m_pSrc->GetContext(); // remember the FIRST context to play in this frame
	AKASSERT( pSrcContext != NULL );

	AkUInt32 ulStopOffset = pSrcContext->GetAndClearStopOffset();
	if ( ulStopOffset != AK_NO_IN_BUFFER_STOP_REQUESTED )
	{
		if ( ulStopOffset == 0 )
		{
			Stop();
			return false; // stop immediately: no need to run at all.
		}

		// A stop at a specific sample has been requested
		io_state.buffer.SetRequestSize( (AkUInt16) ulStopOffset );
		io_state.bStop = true;
	}

	pSrcContext->GetParams( &m_Param );
	bool bNextSilent;
	AkAudioFormat* pAudioFormat = pSrcContext->GetMediaFormat();
	io_state.buffer.SetChannelMask( pAudioFormat->GetChannelMask() );
	GetVolumes( io_state.bIsEnvironmentalBus, pSrcContext, &io_state.buffer, bNextSilent );
	bool bAudible = !( m_bPreviousSilent && bNextSilent );

	//In the feedback pipeline, we don't need to wait for the voice to be silent to stop.  The fade out is handled by the device.
	if ( bNextSilent || pSrcContext->IsForFeedbackPipeline() )
	{
		if ( pSrcContext->WasStopped() )
			io_state.bStop = true;
		else if ( pSrcContext->WasPaused() )
			io_state.bPause = true;
	}

	io_state.bAudible = bAudible;

	bool bNeedToRun = true;

	if ( bAudible ) 
	{
		// Switching from non-audible to audible
		AkVPLSrcCbxRec * pActiveSrcRec = m_pCbxRec[0];
		CAkVPLSrcNode * l_pSrcNode = pActiveSrcRec->m_pSrc;
		if ( !m_bLastAudible || !l_pSrcNode->IsIOReady() )
		{
			if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_SetAsVirtualVoice ) 
			{
				if( !l_pSrcNode->IsIOReady() )
				{
					// Note: The pipeline is connected but streaming is not ready. This is the consequence 
					// of the "voice initially virtual faking scheme" (see AddSrc). Let's repair it now.
					AKRESULT l_StreamResult = FetchStreamedData( l_pSrcNode );
					if( l_StreamResult == AK_FormatNotReady )
					{
						// not ready, let gain some time by telling we are not audible.
						bAudible = false;
						bNeedToRun = false;
					}
					else if( l_StreamResult != AK_Success )
					{
						Stop();
						// not ready, let's gain some time by telling we are not audible.
						bAudible = false;
						bNeedToRun = false;
					}
				}
				else // yes, must check it again, may have been set to false in previous call.
				{
					AKRESULT eResult = m_pCbxRec[0]->Head()->VirtualOff( m_eVirtualBehavior );
					if ( eResult != AK_Success )
					{
						Stop();
						MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
						return false;
					}
				}
#ifndef AK_OPTIMIZED
				m_bStreamingStarted = false; // avoid starvation notifications when restarting the real voice
#endif
			}
		}
	}
	else // !audible
	{
		if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_SetAsVirtualVoice ) 
		{
			bNeedToRun = false;

			// Switching from audible to non-audible
			if ( m_bLastAudible ) 
			{
				m_pCbxRec[0]->Head()->VirtualOn( m_eVirtualBehavior );
			}

			if ( m_eVirtualBehavior == AkVirtualQueueBehavior_FromBeginning )
			{
				// Release any incomplete output buffer.
				m_pCbxRec[0]->Head()->ReleaseBuffer();
			}
			else if ( m_eVirtualBehavior == AkVirtualQueueBehavior_FromElapsedTime )
			{
				// Note. We could keep incomplete output buffers and consume them
				// before time skipping, but the pitch node does not support it.
				// Release (we dropped a few milliseconds, and perhaps a few markers).
				m_pCbxRec[0]->Head()->ReleaseBuffer();
				// OR... fix pitch node.

				AkInt32 iFrameOffset = pSrcContext->GetFrameOffset();
				if( iFrameOffset >= io_state.buffer.MaxFrames() )
				{
					// upstream nodes are not yet required to output data
					pSrcContext->ConsumeFrameOffset( io_state.buffer.MaxFrames() );
				}
				else
				{
					if ( SourceTimeSkip( io_state.buffer.MaxFrames() ) == AK_NoMoreData )
					{
						Stop();
					}
				}
			}
			else
			{
				AKASSERT( m_eVirtualBehavior == AkVirtualQueueBehavior_Resume );

				// Nothing to do here -- just don't process anything.
			}
		}
		else if ( m_eBelowThresholdBehavior == AkBelowThresholdBehavior_KillVoice )
		{
			//Setting it to no more data will directly term it.
			Stop();
			bNeedToRun = false;
			MONITOR_MSG( L"Voice killed: Volume below minimum threshold." ); 
		}
		// else we are not audible but we must consume a buffer.
	}

	m_bLastAudible = bAudible;
	m_bPreviousSilent = bNextSilent;

	if ( bAudible || bNeedToRun )
	{
		AkInt32 iFrameOffset = pSrcContext->GetFrameOffset();
		if( iFrameOffset >= io_state.buffer.MaxFrames() )
		{
			// upstream nodes are not yet required to output data
			pSrcContext->ConsumeFrameOffset( io_state.buffer.MaxFrames() );
			bNeedToRun = false;
		}
	}
	
	m_bFirstBufferProcessed = true;

	return bNeedToRun;
}

void CAkVPLSrcCbxNode::ConsumeBuffer( AkVPLState & io_state )
{
	if( io_state.buffer.posInfo.uStartPos != -1 )
	{
		g_pPositionRepository->UpdatePositionInfo( m_pCbxRec[0]->m_pSrc->GetContext()->GetPlayingID(), &io_state.buffer.posInfo, this );
	}

	if ( m_bufferMix.HasData() ) // we have a stitching buffer: append.
	{
		// Copy the first part of the second source into the merge buffer.
		AkUInt32 uNumChannels = m_bufferMix.NumChannels();
		for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
		{
			void * AK_RESTRICT pChannelStartOut = m_bufferMix.GetChannel( uChan ) + m_bufferMix.uValidFrames;
			void * AK_RESTRICT pChannelStartIn = io_state.buffer.GetChannel( uChan );
			AKPLATFORM::AkMemCpy( pChannelStartOut, pChannelStartIn, io_state.buffer.uValidFrames * sizeof(AkReal32) );
		}
		m_bufferMix.uValidFrames += io_state.buffer.uValidFrames;
		// Note: ZeroPad done in MixBus

		// Replace context for previous markers with the current context
		AkVPLSrcCbxRec * pSrcRec = m_pCbxRec[0];

		CAkPBI* l_pCtx = pSrcRec->m_pSrc->GetContext();
		for( unsigned int i=0; i<m_bufferMix.uNumMarkers; i++ )
		{
			m_bufferMix.pMarkers[i].pContext = l_pCtx;
		}

		CAkVPLNode::MergeMarkers( &io_state.buffer, m_bufferMix.uNumMarkers, m_bufferMix.pMarkers );

		pSrcRec->Head()->ReleaseBuffer();

		*( (AkPipelineBuffer *) &io_state.buffer ) = m_bufferMix;
	}
	else if ( m_pCbxRec[1] ) // we do not have a stitching buffer -- but we might need to stitch if we have a next
	{
		if ( io_state.result != AK_NoMoreData ) 
		{
			return; // passthrough in normal case
		}

		//Stiching buffers of different formats is not supported yet.
		bool bReturnAfterSwitchSources = false;
		CAkPBI* pCtx = m_pCbxRec[1]->m_pSrc->GetContext();
		if (pCtx->GetMediaFormat()->GetChannelMask() != io_state.buffer.GetChannelMask() ||
			pCtx->IsForFeedbackPipeline() != m_pCbxRec[0]->m_pSrc->GetContext()->IsForFeedbackPipeline())
		{
			io_state.result = AK_DataReady;
			MONITOR_ERROR( AK::Monitor::ErrorCode_TransitionNotAccurateChannel );
			bReturnAfterSwitchSources = true;
		}
		else // copy data into stitch buffer
		{
			if ( m_bufferMix.GetCachedBuffer( 
					io_state.buffer.MaxFrames(),
					io_state.buffer.GetChannelMask() ) != AK_Success )
			{
				io_state.result = AK_InsufficientMemory;
				return;
			}

			// from this point on, m_bufferMix has allocated memory but we don't need to expressly release it in
			// case of early return because ReleaseBuffer() will be called by the lower engine anyway.

			AkUInt32 uNumChannels = m_bufferMix.NumChannels();
			for ( unsigned int uChan = 0; uChan < uNumChannels; ++uChan )
			{
				void * AK_RESTRICT pChannelStartOut = m_bufferMix.GetChannel( uChan );
				void * AK_RESTRICT pChannelStartIn = io_state.buffer.GetChannel( uChan );
				AKPLATFORM::AkMemCpy( pChannelStartOut, pChannelStartIn, io_state.buffer.uValidFrames * sizeof(AkReal32) );
			}
			m_bufferMix.uValidFrames = io_state.buffer.uValidFrames;
			m_bufferMix.uNumMarkers = io_state.buffer.uNumMarkers;
			m_bufferMix.pMarkers = io_state.buffer.pMarkers;
		}

		AkVPLSrcCbxRec * pSrcRec = m_pCbxRec[0];
		pSrcRec->Head()->ReleaseBuffer();

		// Will transfer the internal pitch and LPF algorithm states to next source
		
		AkSharedPitchState sPitchState;
		pSrcRec->m_Pitch.GetPitchState( sPitchState );
		AkSharedLPFState sLPFState;
		pSrcRec->m_LPF.GetLPFState( sLPFState );
		
		// Release previous source
		RemoveSrc( 0, CtxDestroyReasonFinished );

		// Verify if next source exists AND is ready before proceeding.
		AkVPLSrcCbxRec * pNextSrcRec = m_pCbxRec[0];
		AKASSERT( pNextSrcRec );	// Note: since src 0 was just released, next src became src 0.
		CAkVPLSrcNode * pNextSrcNode = pNextSrcRec->m_pSrc;
		AKASSERT( pNextSrcNode );

		if ( !pNextSrcNode->IsIOReady() )
		{
			// Next source is not ready yet. Try again.
			AKRESULT eResult = FetchStreamedData( pNextSrcNode );
			if ( eResult == AK_Success )
			{
				// Source just got ready. Add pipeline.
				if ( AddPipeline( pNextSrcRec ) != AK_Success )
				{
					io_state.result = AK_Fail;
					return;
				}
			}
			else if ( eResult == AK_FormatNotReady )
			{
				// Missed the boat. Too bad. Return as NoMoreData (what we stitched so far)
				// REVIEW: this will cause this sound to be skipped, but we can't return DataReady
				// as the pipeline doesn't support running voices that are not IsIOReady() (apart from the virtual voices special case)
				MONITOR_ERROR( AK::Monitor::ErrorCode_TransitionNotAccurateStarvation );
				io_state.result = AK_NoMoreData;
				return;
			}
			else if ( eResult == AK_Fail )
			{
				io_state.result = AK_Fail;
				return;
			}
		}

		AKVERIFY( pNextSrcNode->Start() == AK_Success );

		//Must Refresh the LPF and Pitch content with the one that should be used for the new PBI used
		CAkPBI * pNextPBI = pNextSrcNode->GetContext();
		pNextPBI->GetParams( &m_Param );

		// Transfer state
		pNextSrcRec->m_Pitch.SetPitchState( sPitchState );
		AKRESULT eResult = pNextSrcRec->m_LPF.SetLPFState( sLPFState );
		if ( eResult != AK_Success )
		{
			io_state.result = eResult;
			return;
		}
		
		if ( bReturnAfterSwitchSources )
			return;

		// Check if the source returns the exact amount of data as requested.
		if( m_bufferMix.uValidFrames == m_bufferMix.MaxFrames() )
		{
			*((AkPipelineBuffer *) &io_state.buffer) = m_bufferMix;
			io_state.result = AK_DataReady;
			return;
		}

		// Check if there is a frame offset larger than the remaining portion
		AkInt32 iNextRequestFrames = m_bufferMix.MaxFrames() - m_bufferMix.uValidFrames;
		if( pNextPBI->GetFrameOffset() >= iNextRequestFrames )
		{
			pNextPBI->ConsumeFrameOffset( iNextRequestFrames );
			*((AkPipelineBuffer *) &io_state.buffer) = m_bufferMix;
			io_state.result = AK_DataReady;
			return;
		}

		io_state.buffer.Clear();
		io_state.buffer.SetRequestSize( (AkUInt16) iNextRequestFrames );
		io_state.result = AK_DataNeeded; // go up
	}
}

//-----------------------------------------------------------------------------
// Name: ProcessBufferForObstruction
// Desc: Sends the buffer through LPF2 for obstruction.
//
// Parameters:
//	AkAudioBufferCbx*	io_pBuffer	 : Pointer to the buffer object.
//
// Return:
//	Ak_Success		: Buffer was processed.
//	AK_Fail			: Failed to process the buffer.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::ProcessBufferForObstruction( 
	AkAudioBufferCbx* io_pBuffer
	)
{
	bool bObsLPF = io_pBuffer->fObsLPF != 0.0f;

	if( bObsLPF || m_bLastObsLPF )
	{
		m_bLastObsLPF = bObsLPF;
		return m_ObstructionLPF.ProcessBuffer( io_pBuffer, io_pBuffer->fObsLPF );
	}

	m_bLastObsLPF = false;

	return AK_Success; //nothing to do!

} //ProcessBufferForObstruction


//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::ReleaseBuffer()
{
	// Free the sample accurate mixing buffer.
	if( m_bufferMix.HasData() )
	{
		m_bufferMix.ReleaseCachedBuffer();
		return;
	}

	// Otherwise we need to release an upstream node's buffer.

	m_pCbxRec[0]->Head()->ReleaseBuffer();
} // ReleaseBuffer

//-----------------------------------------------------------------------------
// Name: AddSrc
// Desc: Add a source.
//
// Parameters:
//	CAkPBI * in_pCtx    : Context to add.
//	bool			  in_bActive : true=source should be played. 
//
// Return:
//	Ak_Success :		    Source was added and pipeline created.
//  AK_InsufficientMemory : Error memory not available.
//  AK_NoDataReady :        IO not completed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::AddSrc( CAkPBI * in_pCtx, bool in_bActive )
{
	AKASSERT( in_pCtx != NULL );

	//---------------------------------------------------------------------
	// Create the source node.
	//---------------------------------------------------------------------
	CAkVPLSrcNode * pSrc = CAkVPLSrcNode::Create( in_pCtx, in_bActive );
	if( pSrc == NULL )
	{
		return AK_Fail;
	}

	// Dont get this info if we are the sequel of a sample accurate container, they must all share the behavior of the first sound.
	if( in_bActive )
	{
		m_eBelowThresholdBehavior = in_pCtx->GetVirtualBehavior( m_eVirtualBehavior );
	}
	//Do check here to start stream only if required
	bool bIsUnderThreshold = false;
	if( m_eBelowThresholdBehavior != AkBelowThresholdBehavior_ContinueToPlay )
	{
		bIsUnderThreshold = IsInitiallyUnderThreshold( in_pCtx );
	}

	AKRESULT l_eResult;

	if( bIsUnderThreshold && m_eBelowThresholdBehavior == AkBelowThresholdBehavior_KillVoice )
	{
		MONITOR_MSG( L"Voice killed: Volume below minimum threshold." ); 
		l_eResult = AK_PartialSuccess;
	}
	// Must check in_bActive here, if not active, it is because we are in sample accurate mode, and in this mode, we have no choice but to keep streaming
	else if( bIsUnderThreshold && m_eVirtualBehavior == AkVirtualQueueBehavior_FromBeginning && in_bActive )
	{
		l_eResult = AK_Success;//We fake success so the pipeline gets connected.

        // IMPORTANT. m_bLastAudible is used to know when we need to become virtual (call VirtualOn()). 
        // In this case - PlayFromBeginning mode, starting virtual - we skip the virtual handling mechanism:
        // do not start stream. 
        m_bLastAudible = false;
	}
	else
	{
		l_eResult = FetchStreamedData( pSrc );
	}

	AkVPLSrcCbxRec * pCbxRec = NULL;

	if( l_eResult == AK_Success || l_eResult == AK_FormatNotReady )
	{
		// The source was created/initialized successfully, but I/O could still be pending.
		// Instantiate its CbxRec, plug, then AddPipeline if applicable.
		AkNew2( pCbxRec, g_LEngineDefaultPoolId, AkVPLSrcCbxRec, AkVPLSrcCbxRec );
		if ( pCbxRec ) 
		{
			pCbxRec->m_pSrc	= pSrc;

			for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
				pCbxRec->m_pFilter[ uFXIndex ] = NULL;

			// Plug (this must succeed).
			for ( AkUInt32 iSrc = 0; iSrc < MAX_NUM_SOURCES; ++iSrc )
			{
				if ( m_pCbxRec[ iSrc ] == NULL )
				{
					m_pCbxRec[ iSrc ] = pCbxRec;
					break;
				}
			}
		}
		else
		{
			l_eResult = AK_InsufficientMemory;
		}
	}

	if( l_eResult == AK_Success )
	{
		return AddPipeline( pCbxRec );
	}
	else if ( l_eResult == AK_FormatNotReady )
	{
		// Success, but pipeline was not created.
		return l_eResult;
	}

	// Failure case: delete CbxRec if it was created, otherwise only the source. 
	if ( pCbxRec )
	{
		DeleteCbxRec( pCbxRec, CtxDestroyReasonPlayFailed );
	}
	else
	{
		// Clear source.
		AKASSERT( pSrc );
		pSrc->Term( CtxDestroyReasonPlayFailed );
		AkDelete( g_LEngineDefaultPoolId, pSrc );
	}

	return l_eResult;
} // AddSrc


//-----------------------------------------------------------------------------
// Name: FetchStreamedData
// Desc: Performs I/O on streamed source in order to get audio format and 
//		 first data buffer.
// Note: Applies on source. Sets its IOReady flag.
//
// Return:
//	Ak_Success: Source is ready to be connected to a pipeline.
//	Ak_FormatNotReady: Source is not ready.
//  AK_Fail:    I/O error.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::FetchStreamedData(
	CAkVPLSrcNode * in_pSrc
	)
{
	AKASSERT( in_pSrc );

	if ( !in_pSrc->IsIOReady() )
	{	
		AKRESULT l_eResult = in_pSrc->StartStream( );
		if ( l_eResult == AK_Success )
		{
			in_pSrc->SetIOReady();
		}
		return l_eResult;
	}
	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: AddPipelineDeferred
// Desc: Fetches streaming data of source record 0. 
//		 If the source is ready and the context is ready to be connected, 
//		 adds pipeline.
//
// Return:
//	Ak_Success: Pipeline was created.
//	Ak_FormatNotReady: Pipeline was created.
//  AK_Fail:    Failed to create the pipeline.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::AddPipelineDeferred(
	CAkPBI * in_pCtx 
	)
{
	AKASSERT( m_pCbxRec[0] && m_pCbxRec[0]->m_pSrc );

	AKRESULT l_eResult = FetchStreamedData( m_pCbxRec[0]->m_pSrc );

	AkInt32 iMaxOffset = AK_NUM_VOICE_REFILL_FRAMES;
	if (in_pCtx->IsForFeedbackPipeline())
		iMaxOffset = AK_FEEDBACK_MAX_FRAMES_PER_BUFFER;

	if ( l_eResult == AK_FormatNotReady || 
		( l_eResult == AK_Success 
		&& in_pCtx->GetFrameOffset() >= iMaxOffset ) )
	{
		// Streamed source is not ready or 
		// Frame offset greater than an audio frame: keep it in list of sources not connected.
		return AK_FormatNotReady;
	}
	
	if ( l_eResult == AK_Success )
	{
		// Source is ready: Add pipeline.
		AKASSERT( m_pCbxRec[0]->m_pSrc->IsIOReady() );
		l_eResult = AddPipeline( m_pCbxRec[0] );
	}
	if ( l_eResult != AK_Success )
	{
		// Failed. Clean the source record.
		RemoveSrc( 0, CtxDestroyReasonPlayFailed );
		return AK_Fail;
	}
	return l_eResult;
}


//-----------------------------------------------------------------------------
// Name: AddPipeline
// Desc: Create the pipeline for the source.
//
// Parameters:
//	AkVPLSrcCbxRec * in_pSrcRec : Pointer to a source record to complete.
//
// Return:
//	Ak_Success: Pipeline was created.
//  AK_Fail:    Failed to create the pipeline.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLSrcCbxNode::AddPipeline( AkVPLSrcCbxRec * in_pSrcRec )
{
	AKASSERT( in_pSrcRec != NULL );
	/*AKASSERT( in_pSrcRec->m_pSrc->IsIOReady() ); */ // could be "not IO ready" due to handling of voices initially virtual

	CAkVPLNode *	  l_pNode[MAX_NODES];
	AkChar			  l_cCnt	= 0;
	CAkPBI * l_pCtx	= NULL;
	AKRESULT		  l_eResult = AK_Success;

	l_pCtx = in_pSrcRec->m_pSrc->GetContext();
	AKASSERT( l_pCtx != NULL );

	//---------------------------------------------------------------------
	// Create the source node.
	//---------------------------------------------------------------------	 
	l_pNode[l_cCnt++] = in_pSrcRec->m_pSrc;
	AkAudioFormat l_Format = *l_pCtx->GetMediaFormat();

	//---------------------------------------------------------------------
	// Create the format converter node.
	//---------------------------------------------------------------------
	
	// PhM : stuff this one here before the gotos to make the PS3 compiler happy
	IAkPlugin*    l_pIEffect = NULL;

	//---------------------------------------------------------------------
	// Create the resampler/pitch node.
	//---------------------------------------------------------------------

	l_eResult = in_pSrcRec->m_Pitch.Init( &m_Param, &l_Format, l_pCtx, m_usMaxFrames, m_iSampleRate );
	AKASSERT( l_eResult == AK_Success );

	if( l_eResult != AK_Success ) 
		goto AddPipelineError;

	l_pNode[l_cCnt++] = &in_pSrcRec->m_Pitch;

	// now that the sample conversion stage is passed, the sample rate must be the native sample rate for others components.
	l_Format.uSampleRate = m_iSampleRate;

	//---------------------------------------------------------------------
	// Create the insert effect.
	//---------------------------------------------------------------------

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		const AkFXDesc & l_FXInfo = l_pCtx->GetFX( uFXIndex );

		if( l_FXInfo.EffectTypeID != AK_INVALID_PLUGINID )
		{
			CAkVPLFilterNode * pFilter = AkNew( g_LEngineDefaultPoolId, CAkVPLFilterNode() );
			if( pFilter == NULL )
			{ 
				l_eResult = AK_Fail; 
				goto AddPipelineError; 
			}

			l_eResult = pFilter->Init( l_FXInfo, uFXIndex, l_pCtx, l_Format );

			// Note: Effect can't be played for some reason but don't kill the sound itself yet...
			if ( l_eResult != AK_Success )
			{
				AKASSERT( pFilter );
				pFilter->Term();
				AkDelete( g_LEngineDefaultPoolId, pFilter );
				pFilter = NULL;	
				continue;
			}

			in_pSrcRec->m_pFilter[ uFXIndex ] = pFilter;
			l_pNode[l_cCnt++] = pFilter;
		}
	}

	//---------------------------------------------------------------------
	// Create the lpf node.
	//---------------------------------------------------------------------
	l_eResult = in_pSrcRec->m_LPF.Init( &m_Param, &l_Format );
	if( l_eResult != AK_Success ) 
		goto AddPipelineError;

	l_pNode[l_cCnt++] = &in_pSrcRec->m_LPF;

	//---------------------------------------------------------------------
	// Create another lpf node for obstruction (only 1 for all pipelines)
	// [not connected]
	//---------------------------------------------------------------------
	if( !m_ObstructionLPF.IsInitialized() )
	{
		l_eResult = m_ObstructionLPF.Init( &m_Param, &l_Format );
		if( l_eResult != AK_Success ) 
			goto AddPipelineError;
	}
	
	//---------------------------------------------------------------------
	// Connect the nodes.
	//---------------------------------------------------------------------
	while( --l_cCnt )
	{
		l_eResult = l_pNode[l_cCnt]->Connect( l_pNode[l_cCnt-1]) ;
		AKASSERT( l_eResult == AK_Success );
	}

AddPipelineError:

	return l_eResult;
} // AddPipeline

//-----------------------------------------------------------------------------
// Name: RemoveSrc
// Desc: Remove a specified source node from the list.
//
// Parameters:
//	AkVPLSrcCbxRec * in_pSrcRec : Pointer to a source record.
//
// Return:
//	Ak_Success: Source was removed from the list.
//  AK_Fail:    Failed to remove source from list.
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::RemoveSrc( AkUInt32 in_uSrcIdx, AkCtxDestroyReason in_eReason )
{
	DeleteCbxRec( m_pCbxRec[in_uSrcIdx], in_eReason );

	for ( AkUInt32 iSrc = in_uSrcIdx; iSrc < MAX_NUM_SOURCES-1; ++iSrc )
		m_pCbxRec[ iSrc ] = m_pCbxRec[ iSrc+1 ];

	m_pCbxRec[ MAX_NUM_SOURCES-1 ] = NULL;
}

//-----------------------------------------------------------------------------
// Name: DeleteCbxRec
// Desc: Delete a source record.
//
// Parameters:
//	AkVPLSrcCbxRec * in_pSrcRec : Pointer to a source record.
//
//-----------------------------------------------------------------------------
void CAkVPLSrcCbxNode::DeleteCbxRec( AkVPLSrcCbxRec * in_pSrcRec, AkCtxDestroyReason in_eReason )
{
	AKASSERT( in_pSrcRec != NULL );

	in_pSrcRec->m_pSrc->Term( in_eReason );
	AkDelete( g_LEngineDefaultPoolId, in_pSrcRec->m_pSrc );

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
	{
		if( in_pSrcRec->m_pFilter[ uFXIndex ] != NULL )
		{
			in_pSrcRec->m_pFilter[ uFXIndex ]->Term();
			AkDelete( g_LEngineDefaultPoolId, in_pSrcRec->m_pFilter[ uFXIndex ] );
		}
	}

	in_pSrcRec->m_Pitch.Term();
	in_pSrcRec->m_LPF.Term();

	AkDelete2( g_LEngineDefaultPoolId, AkVPLSrcCbxRec, in_pSrcRec );
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CAkVPLSrcCbxNode::GetVolumes( bool in_bEnvBus, CAkPBI* AK_RESTRICT in_pContext, AkAudioBufferCbx* AK_RESTRICT io_pBuffer, bool & out_bNextSilent )
{
	AkUInt32 uNumChannels = io_pBuffer->NumChannels();
	
	AkSoundPositioningParams posParams;
	in_pContext->GetPositioningParams( &posParams );

	// without 3D
	if( posParams.ePosType == Ak2DPositioning )
	{
		AkReal32 fDryLevel = in_bEnvBus ? GetContext()->GetDryLevelValue() : 0;

		out_bNextSilent = ( m_Param.Volume <= g_fVolumeThresholdDB ) && ( !io_pBuffer->HasLFE() || ( m_Param.LFE <= g_fVolumeThresholdDB ) );

		io_pBuffer->fObsLPF = 0.f; // No obstruction in 2D

		const BaseGenParams& basePosParams = in_pContext->GetBasePosParams();
		Prev2DParams& l_prev2DParams = in_pContext->GetPrevPosParams();

		if( l_prev2DParams.prev2DParams		== basePosParams
			&& l_prev2DParams.prevVolume	== m_Param.Volume 
			&& l_prev2DParams.prevLfe		== m_Param.LFE
			&& l_prev2DParams.prevDryLevel	== fDryLevel
			)
		{
			for( AkUInt32 i = 0; i < uNumChannels; ++i )
			{
				io_pBuffer->AudioMix[i].Next = m_PreviousDirect[i];
				if ( in_bEnvBus )
				{
					io_pBuffer->EnvMix[i].Next = m_PreviousEnv[i];
				}
			}
		}
		else
		{
			l_prev2DParams.prev2DParams = basePosParams;
			l_prev2DParams.prevVolume	= m_Param.Volume;
			l_prev2DParams.prevLfe		= m_Param.LFE;
			l_prev2DParams.prevDryLevel	= fDryLevel;

			AkReal32 fVolume = AkMath::dBToLin( m_Param.Volume );
			AkReal32 fLfe = AkMath::dBToLin( m_Param.LFE );
			//transform -100 +100 values to 0..1 float
			AkReal32 fX = ( basePosParams.m_fPAN_RL + 100 ) * 0.005f;// /200
			AkReal32 fY = ( basePosParams.m_fPAN_FR + 100 ) * 0.005f;// /200

			AkSIMDSpeakerVolumes volumes[AK_VOICE_MAX_NUM_CHANNELS];
			CAkSpeakerPan::GetSpeakerVolumes2DPan( fX, fY, basePosParams.m_fCenterPct, basePosParams.bIsPannerEnabled, io_pBuffer->GetChannelMask(), volumes );
			AkUInt32 uNumFullBandChannels = AK::GetNumChannels( io_pBuffer->GetChannelMask() & ~AK_SPEAKER_LOW_FREQUENCY );

			//Keep the positioning volume independent of the rest for feedback.  We will recombine this later.
			AkFeedbackParams *pFeedbackParam = in_pContext->GetFeedbackParameters();
			if (pFeedbackParam != NULL)
			{
				AkUInt32 iIndex = pFeedbackParam->VolumeIndex(AkFeedbackParams::NextVolumes, 0, 0);
				for (AkUInt32 uChan=0; uChan < uNumFullBandChannels; uChan++ )
				{
					pFeedbackParam->m_Volumes[iIndex] = volumes[uChan];
					iIndex++;
				}
			}

			for (AkUInt32 uChan=0; uChan<uNumFullBandChannels; uChan++ )
			{
				io_pBuffer->AudioMix[uChan].Next = volumes[uChan];
				io_pBuffer->AudioMix[uChan].Next.Mul( fVolume );		// MUL LFE uselessly, but faster than trying to not do it.
			}

			// Treat LFE separately.
			if ( io_pBuffer->HasLFE() )
			{
				AkUInt32 uChanLFE = uNumChannels - 1;
				memset( &( io_pBuffer->AudioMix[uChanLFE].Next ), 0, sizeof(AkSIMDSpeakerVolumes) );
				io_pBuffer->AudioMix[uChanLFE].Next.volumes.fLfe = fLfe;
			}

			if ( in_bEnvBus )// (env == base) & ( base = base * dry )
			{
				for( AkUInt32 i = 0; i < uNumChannels; ++i )
				{
					io_pBuffer->EnvMix[i].Next = io_pBuffer->AudioMix[i].Next;
					io_pBuffer->AudioMix[i].Next.Mul( fDryLevel );
				}
			}
		}
	}
	// with 3D
	else
	{
		// get the 3D volumes
		CAkListener::Get3DVolumes( in_pContext, 
									m_Param,
									posParams, 
									io_pBuffer->GetChannelMask(), 
									io_pBuffer->EnvMix, 
									io_pBuffer->AudioMix, 
									&io_pBuffer->fObsLPF,
									in_bEnvBus );

		// Determine if buffer will be silent -- that is, if the volumes of all channels is below
		// a certain threshold.
		out_bNextSilent = true;

		AkUInt32 uNumChannels = AK::GetNumChannels( io_pBuffer->GetChannelMask() );
		if ( in_bEnvBus )
		{
			for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
			{
				out_bNextSilent &= io_pBuffer->EnvMix[iChannel].Next.IsLessOrEqual( &g_fVolumeThreshold );
				out_bNextSilent &= io_pBuffer->AudioMix[iChannel].Next.IsLessOrEqual( &g_fVolumeThreshold );
			}
		}
		else
		{
			for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
			{
				out_bNextSilent &= io_pBuffer->AudioMix[iChannel].Next.IsLessOrEqual( &g_fVolumeThreshold );
			}
		}
	}

	// what about the previous ones ?
	if ( !m_bFirstBufferProcessed )
	{
		// set the previous volume values
		for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
		{
			io_pBuffer->AudioMix[iChannel].Previous = io_pBuffer->AudioMix[iChannel].Next;
			io_pBuffer->EnvMix[iChannel].Previous = io_pBuffer->EnvMix[iChannel].Next;
		}
	}
	else
	{
		for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
		{
			io_pBuffer->AudioMix[iChannel].Previous = m_PreviousDirect[iChannel];
			io_pBuffer->EnvMix[iChannel].Previous = m_PreviousEnv[iChannel];
		}
	}

	for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
	{
		io_pBuffer->AudioMix[iChannel].Next.CopyTo( m_PreviousDirect[iChannel] );
		io_pBuffer->EnvMix[iChannel].Next.CopyTo( m_PreviousEnv[iChannel] );
	}
} //GetVolumes

bool CAkVPLSrcCbxNode::IsInitiallyUnderThreshold( CAkPBI* in_pCtx )
{
	AKASSERT( in_pCtx );
	
	//must gather params since that have never been queried yet
	AkSoundPositioningParams posParams;
	in_pCtx->GetPositioningParams( &posParams );
	in_pCtx->GetParams( &m_Param );

	AkAudioFormat * pMediaFormat = in_pCtx->GetMediaFormat();

	// without 3D
	if( posParams.ePosType == Ak2DPositioning )
	{
		return ( m_Param.Volume <= g_fVolumeThresholdDB ) && ( !pMediaFormat->HasLFE() || ( m_Param.LFE <= g_fVolumeThresholdDB ) );
	}
	// with 3D
	else
	{
		//local volume set
		AkAudioMix AudioMixWet[2]; // 2 == one full-band channel + LFE
		AkAudioMix AudioMixDry[2];
		AkReal32 fObsLPF;

		// Create channel mask containing only one fullband channel + LFE (if necessary)

		AkChannelMask uMask = 0; 

		if ( pMediaFormat->GetChannelMask() & ( ~AK_SPEAKER_LOW_FREQUENCY ) )
			uMask |= AK_SPEAKER_FRONT_CENTER;

		uMask |= ( pMediaFormat->GetChannelMask() & AK_SPEAKER_LOW_FREQUENCY );

		bool bIsEnvironmental = in_pCtx->GetBusContext().IsEnvironmental();

		CAkListener::Get3DVolumes( in_pCtx, m_Param, posParams, uMask, AudioMixWet, AudioMixDry, &fObsLPF, bIsEnvironmental );

		bool bUnderThreshold = AudioMixDry[0].Next.IsLessOrEqual( &g_fVolumeThreshold );// dry Main
		if( bUnderThreshold )
		{
			bool bHasTwoChannels = AK::GetNumChannels( uMask ) > 1;
			if( bHasTwoChannels )
			{
				bUnderThreshold = AudioMixDry[1].Next.IsLessOrEqual( &g_fVolumeThreshold ); // dry LFE
			}
			if( bUnderThreshold && bIsEnvironmental )
			{
				bUnderThreshold = AudioMixWet[0].Next.IsLessOrEqual( &g_fVolumeThreshold ); // wet Main
				if( bUnderThreshold && bHasTwoChannels )
				{
					bUnderThreshold = AudioMixWet[1].Next.IsLessOrEqual( &g_fVolumeThreshold ); // wet LFE
				}
			}
		}

		return bUnderThreshold;
	}
}

//-----------------------------------------------------------------------------
// Name: GetContext
// Desc: Returns the active context.
//
// Parameters:
//	None.
//
// Return:
//	CAkPBI * Pointer to active context.
//-----------------------------------------------------------------------------
CAkPBI * CAkVPLSrcCbxNode::GetContext()
{
	if( m_pCbxRec[0] )
		return m_pCbxRec[0]->m_pSrc->GetContext();

	return NULL;
} // GetContext

void CAkVPLSrcCbxNode::StopLooping()
{
	for( AkUInt32 i = 0; i < MAX_NUM_SOURCES; i++ )
	{
		if( m_pCbxRec[i] )
		{
			if( m_pCbxRec[i]->m_pSrc->StopLooping() != AK_Success )
			{
				// When StopLooping return an error, it means that the source must be stopped.
				// ( occurs with audio input plug-in only actually )
				Stop();
				return;
			}
			if ( i > 0 )
				RemoveSrc( i, CtxDestroyReasonFinished );
		}
	}
}

AKRESULT CAkVPLSrcCbxNode::SourceTimeSkip( AkUInt32 in_uMaxFrames )
{
	AkUInt32 uNeededFrames = in_uMaxFrames;
	AKRESULT eReturn = m_pCbxRec[0]->Head()->TimeSkip( in_uMaxFrames ); // TimeLapse modified param according to the number of frames actually elapsed.
	uNeededFrames -= in_uMaxFrames;

	switch( eReturn )
	{
	case AK_DataReady :
		break;

	case AK_NoMoreData :
		if ( m_pCbxRec[ 1 ] )
		{
			do
			{
				// Release previous source
		
				RemoveSrc( 0, CtxDestroyReasonFinished );

				// Verify if next source exists AND is ready before proceeding.

				AkVPLSrcCbxRec * pNextSrcRec = m_pCbxRec[0];
				CAkVPLSrcNode * pNextSrcNode = pNextSrcRec->m_pSrc;
				AKASSERT( pNextSrcNode );

				if ( !pNextSrcNode->IsIOReady() )
				{
					// Next source is not ready yet. Try again.
					AKRESULT eResult = FetchStreamedData( pNextSrcNode );
					if ( eResult == AK_Success )
					{
						// Source just got ready. Add pipeline.
						if ( AddPipeline( pNextSrcRec ) != AK_Success )
							return AK_Fail;
					}
					else if ( eResult == AK_FormatNotReady )
					{
						// Missed the boat. Too bad. Return as NoMoreData (what we stitched so far)
						// REVIEW: this will cause this sound to be skipped, but we can't return DataReady
						MONITOR_ERROR( AK::Monitor::ErrorCode_TransitionNotAccurateStarvation );
						break;
					}
					else if ( eResult == AK_Fail )
						return AK_Fail;
				}

				AKVERIFY( pNextSrcNode->Start() == AK_Success );

				//Must Refresh the LPF and Pitch content with the one that should be used for the new PBI used
				pNextSrcNode->GetContext()->GetParams( &m_Param );

				// Check if the source returns the exact amount of data as requested.
				
				if( uNeededFrames == 0 )
				{
					eReturn = AK_DataReady;
					break;
				}

				AkUInt32 uReqFrames = uNeededFrames;
				eReturn = pNextSrcRec->Head()->TimeSkip( uReqFrames );
				uNeededFrames -= uReqFrames;

				if( eReturn != AK_DataReady && eReturn != AK_NoMoreData )
				{
					// This should not happen, since sources not ready should have been
					// caught by IOReady test.
					AKASSERT( !"ERROR: Data not available." );
					return AK_Fail;
				}
			} 
			while ( eReturn == AK_NoMoreData && uNeededFrames && m_pCbxRec[ 1 ] );
		}
		break;
	}

	if( eReturn == AK_Fail )
	{
		eReturn	 = AK_NoDataReady;
		ReleaseMemory(); // this might have become redundant now that we call Stop immediately
		Stop();

		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotPlaySource );
	}

	return eReturn;
}

void CAkVPLSrcCbxNode::SetLastEnvironmentalValues( const AkEnvironmentValue *in_pValues )
{
	AKASSERT( in_pValues );
	if(in_pValues)
		AKPLATFORM::AkMemCpy( m_LastEnvironmentValues, (void * )in_pValues, AK_MAX_ENVIRONMENTS_PER_OBJ * sizeof(AkEnvironmentValue) );
}
