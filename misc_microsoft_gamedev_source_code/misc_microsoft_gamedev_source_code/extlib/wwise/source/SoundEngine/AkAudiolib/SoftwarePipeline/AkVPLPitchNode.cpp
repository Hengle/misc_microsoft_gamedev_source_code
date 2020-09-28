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
// AkVPLPitchNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkLEngine.h"
#include "AkVPLPitchNode.h"
#include "AudioLibDefs.h"     // Pool IDs definition.
#include "math.h"

void CAkVPLPitchNode::GetBuffer( AkVPLState & io_state )
{
	AKASSERT( m_pInput != NULL );
	AKASSERT( m_SoundParams != NULL );

	m_usRequestedFrames = io_state.buffer.MaxFrames();
	m_bStartPosInfoUpdated = false;

	// Apply new pitch from context
 	m_Pitch.SetPitch( (AkReal32)( m_SoundParams->Pitch ) );

	// Consider using samples at the output buffer if we already have enough to fulfill request.
	// This can happen in the case of a stop offset following a source starvation.
	if( m_BufferOut.uValidFrames >= io_state.buffer.MaxFrames() )
	{
		// We have enough. Skip upstream processing.
		io_state.result = AK_DataReady;
		return;
	}

	// Consume what we have
	if( m_BufferIn.uValidFrames != 0 )
	{
		ConsumeBuffer( io_state );
		return;
	}

	if ( m_bLast )
	{
		// Upstream node (source) had already finished and the pitch node has no 
		// data in its input buffer. This can occur when the voice becomes physical
		// FromElapsedTime just after the source finished, but the pitch node still 
		// has "virtual buffered samples". Simply continue processing downstream
		// (with 0 valid input samples). WG-9004.
		io_state.result = AK_NoMoreData;
	}
}

void CAkVPLPitchNode::ConsumeBuffer( AkVPLState & io_state )
{
	if ( io_state.result == AK_NoMoreData )
		m_bLast = true;

	if ( m_BufferIn.uValidFrames == 0 )
	{
		//AKASSERT( io_state.buffer.uValidFrames != 0 );
		// WG-9121 Source plug-ins with RTPCable duration may end up (legitimally) in this condition

		m_BufferIn = io_state.buffer;
	}

	if ( !m_BufferOut.HasData() )
	{
		// output same channel config as input
#ifndef XBOX360
		if ( m_BufferOut.GetCachedBuffer( m_usMaxFrames,
										  m_BufferIn.GetChannelMask() 
										  ) == AK_Success )
		{
#else
		AkUInt8* pData = (AkUInt8*)AkAlloc( g_LEngineDefaultPoolId, (m_usMaxFrames+4)*sizeof(AkReal32)*m_BufferIn.NumChannels() );
		if ( pData )
		{
			((AkPipelineBufferBase*)&m_BufferOut)->AttachInterleavedData( pData, m_usMaxFrames, 0, m_BufferIn.GetChannelMask() );
#endif
			// IM
			// Force to start the sound in a specific sample in the buffer.
			// Allows to start sound on non-buffer boundaries.
			if ( m_bPadFrameOffset )
			{
				AkInt32 l_iFrameOffset = m_pPBI->GetFrameOffset();
				if( l_iFrameOffset > 0 )
				{
					AKASSERT( l_iFrameOffset < m_BufferOut.MaxFrames() );
					ZeroPrePadBuffer( &m_BufferOut, l_iFrameOffset );// Set the skipped samples to zero
					m_Pitch.SetOutputBufferOffset( l_iFrameOffset );// set the starting offset of the output buffer
				}
				m_bPadFrameOffset = false;
			}
		}
		else
		{
			io_state.result = AK_InsufficientMemory;
			return;
		}
	}

	// If the source was not starting at the beginning of the file and the format was not decoded sample accurately, there
	// may be a remaining number of samples to skip so that the source starts accurately to the right sample.
	AkInt32 l_iSourceFrameOffset = m_pPBI->GetSourceOffset();
	if( l_iSourceFrameOffset )
	{
		if( m_BufferIn.uValidFrames > l_iSourceFrameOffset)
		{
			m_Pitch.SetInputBufferOffset( l_iSourceFrameOffset );
			m_BufferIn.uValidFrames -= (AkUInt16)l_iSourceFrameOffset;
			m_pPBI->SetSourceOffset( 0 );
		}
		else
		{
			m_pPBI->SetSourceOffset( l_iSourceFrameOffset - m_BufferIn.uValidFrames );
			m_BufferIn.uValidFrames = 0;
			io_state.buffer.uValidFrames = 0;
			ReleaseInputBuffer( io_state.buffer );
			io_state.result = AK_DataNeeded;
			return;
		}
	}

	AKASSERT( m_BufferIn.HasData() );

	AkUInt32 l_ulInputFrameOffset = m_Pitch.GetInputFrameOffset();
	AkUInt16 l_usConsumedInputFrames = m_BufferIn.uValidFrames;
	
#ifdef AK_PS3
	m_ulInputFrameOffsetBefore = l_ulInputFrameOffset;
	m_Pitch.ExecutePS3( &m_BufferIn, &m_BufferOut, m_usRequestedFrames, io_state );
#else

	// Note. The number of frames already present in the output buffer must NEVER exceed the 
	// number of requested frames. This situation should have been caught in VPLPitchNode::GetBuffer().
	AKASSERT( m_usRequestedFrames >= m_BufferOut.uValidFrames );
	AKRESULT eResult = m_Pitch.Execute( &m_BufferIn, &m_BufferOut, m_usRequestedFrames );

	l_usConsumedInputFrames -= m_BufferIn.uValidFrames;

	CopyRelevantMarkers( &m_BufferIn, &m_BufferOut, l_ulInputFrameOffset, l_usConsumedInputFrames );
	
	// Adjust position information
	if( ( m_BufferIn.posInfo.uStartPos != -1 ) && !m_bStartPosInfoUpdated )
	{
		m_BufferOut.posInfo = m_BufferIn.posInfo;
		m_BufferOut.posInfo.uStartPos = m_BufferIn.posInfo.uStartPos + l_ulInputFrameOffset;
		m_bStartPosInfoUpdated = true;
	}
	m_BufferOut.posInfo.fLastRate = m_Pitch.GetLastRate();

	// Input entirely consumed release it.
	if( m_BufferIn.uValidFrames == 0 )
	{
		ReleaseInputBuffer( io_state.buffer );
		
		if( m_bLast == true )
			eResult = AK_NoMoreData;
	}

	AKASSERT( m_BufferOut.MaxFrames() == m_usMaxFrames );

	if ( eResult == AK_DataReady || eResult == AK_NoMoreData )
	{
		m_Pitch.DeinterleaveAndSwapOutputIfRequired( &m_BufferOut );
		*((AkPipelineBuffer *) &io_state.buffer) = m_BufferOut;
	}

	io_state.result = eResult;
#endif
}

#ifdef AK_PS3
void CAkVPLPitchNode::ProcessDone( AkVPLState & io_state )
{
	m_BufferOut.uValidFrames = m_Pitch.m_InternalPitchState.uOutValidFrames; 
	io_state.result = m_Pitch.m_InternalPitchState.eState;

	AkUInt32 uInputFramesBefore = m_BufferIn.uValidFrames;
	m_BufferIn.uValidFrames = m_Pitch.m_InternalPitchState.uInValidFrames;
	AkUInt32 uConsumedInputFrames = uInputFramesBefore - m_BufferIn.uValidFrames;

	if ( m_Pitch.m_PitchOperationMode == PitchOperatingMode_Interpolating && m_Pitch.m_InternalPitchState.uInterpolationRampCount  >= PITCHRAMPLENGTH )
	{
		m_Pitch.m_InternalPitchState.uCurrentFrameSkip = m_Pitch.m_InternalPitchState.uTargetFrameSkip;
		m_Pitch.m_PitchOperationMode = PitchOperatingMode_Fixed;
		// Note: It is ok to go to fixed mode (even if it should have gone to bypass mode) for the remainder of this buffer
		// It will go back to bypass mode after next SetPitch() is called
	}

	// Handle the case of the interpolating pitch finishing ramping before the end of output
	if ( m_BufferIn.uValidFrames > 0 && 
		m_BufferOut.uValidFrames < m_usRequestedFrames )
	{
		m_Pitch.ExecutePS3( &m_BufferIn, &m_BufferOut, m_usRequestedFrames, io_state );
		return;
	}

	CopyRelevantMarkers( &m_BufferIn, &m_BufferOut, m_ulInputFrameOffsetBefore, uConsumedInputFrames );
	
	// Adjust position information
	if( ( m_BufferIn.posInfo.uStartPos != -1 ) && !m_bStartPosInfoUpdated )
	{
		m_BufferOut.posInfo = m_BufferIn.posInfo;
		m_BufferOut.posInfo.uStartPos = m_BufferIn.posInfo.uStartPos + m_ulInputFrameOffsetBefore;
		m_bStartPosInfoUpdated = true;
	}
	m_BufferOut.posInfo.fLastRate = m_Pitch.GetLastRate();

	// Input entirely consumed release it.
	if( m_BufferIn.uValidFrames == 0 )
	{
		ReleaseInputBuffer( io_state.buffer );

		if( m_bLast == true )
			io_state.result = AK_NoMoreData;
	}
	if ( io_state.result == AK_DataReady || io_state.result == AK_NoMoreData )
	{
		*((AkPipelineBuffer *) &io_state.buffer) = m_BufferOut;
	}
}
#endif // PS3

//-----------------------------------------------------------------------------
// Name: ReleaseBuffer
// Desc: Releases a processed buffer.
//-----------------------------------------------------------------------------
void CAkVPLPitchNode::ReleaseBuffer()
{
	AKASSERT( m_pInput != NULL );

	// Assume output buffer was entirely consumed by client.
	if( m_BufferOut.HasData() )
	{
#ifndef XBOX360
		m_BufferOut.ReleaseCachedBuffer();
#else
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();
#endif
		
		m_BufferOut.Clear();

		m_Pitch.SetOutputBufferOffset( 0 );
	}
} // ReleaseBuffer

// io_buffer is the VPL state's buffer.
void CAkVPLPitchNode::ReleaseInputBuffer( AkPipelineBuffer & io_buffer )
{
	m_pInput->ReleaseBuffer();

	if ( m_BufferIn.pMarkers )
		AkFree( AK_MARKERS_POOL_ID, m_BufferIn.pMarkers );

	m_BufferIn.Clear();

	// Note. Technically we should reassign our cleared input buffer to the
	// pipeline buffer (in this case the pipeline should only hold a reference 
	// of our input buffer), but just clearing the data does the trick: the
	// request is reset in RunVPL().
	//io_buffer = m_BufferIn;
	io_buffer.ClearData();
}

AKRESULT CAkVPLPitchNode::TimeSkip( AkUInt32 & io_uFrames )
{
	// Apply new pitch from context
 	m_Pitch.SetPitchForTimeSkip( (AkReal32)( m_SoundParams->Pitch ) );

	AkUInt32 uFramesToProduce = io_uFrames;
	m_Pitch.TimeOutputToInput( uFramesToProduce );

	AkUInt32 uProducedFrames = 0;
	AKRESULT eResult = AK_DataReady;

	while ( uFramesToProduce )
	{
		// Need to get 'more data' from source ?

		if ( !m_BufferIn.uValidFrames && 
             !m_bLast )
		{
			AkUInt32 uSrcRequest = m_usMaxFrames;

			AKRESULT eThisResult = m_pInput->TimeSkip( uSrcRequest );

			if( eThisResult != AK_DataReady && eThisResult != AK_NoMoreData )
				return eThisResult;
			else if ( eThisResult == AK_NoMoreData )
				m_bLast = true;

			m_BufferIn.uValidFrames = (AkUInt16) uSrcRequest;
		}

		// Enable this code to activate IM to use Virtual voices.
		// IM
		// Force to start the sound in a specific sample in the actual buffer.
		// Allows to start sound on non-buffer boundaries.
		//if( !m_bFirstBufferProcessed )
		//{
		//	AKASSERT( m_PBI->GetNativeSampleOffsetInBuffer() < uFramesToProduce );
		//	//if not starting at first sample, the real number to produce is less than initially demanded.
		//	uFramesToProduce -= m_PBI->GetNativeSampleOffsetInBuffer();
		//	m_bFirstBufferProcessed = true;
		//}

		AkUInt32 uFrames = AkMin( uFramesToProduce, m_BufferIn.uValidFrames );

		uProducedFrames += uFrames;
		uFramesToProduce -= uFrames;

		m_BufferIn.uValidFrames -= (AkUInt16) uFrames;

		if ( !m_BufferIn.uValidFrames ) 
		{
			if ( m_bLast ) 
			{
				eResult = AK_NoMoreData;
				break;
			}
		}
	}

	m_Pitch.TimeInputToOutput( uProducedFrames );

	io_uFrames = uProducedFrames;

	return eResult;
}

void CAkVPLPitchNode::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior != AkVirtualQueueBehavior_Resume )
	{
		// we do not support skipping some data in the input buffer and then coming back:
		// flush what's left.

		if ( m_BufferIn.HasData() )
			m_pInput->ReleaseBuffer();

		if ( m_BufferIn.pMarkers )
			AkFree( AK_MARKERS_POOL_ID, m_BufferIn.pMarkers );

		m_BufferIn.Clear();
		m_Pitch.ResetOffsets();
	}

	if( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		// WG-5935 
		// If the last buffer was drained and we restart from beginning, the flag must be resetted.
		m_bLast = false;
	}

    if ( !m_bLast )
	{
		m_pInput->VirtualOn( eBehavior );
	}
}

AKRESULT CAkVPLPitchNode::VirtualOff( AkVirtualQueueBehavior eBehavior )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )
	{
		// We used this as a marker for elapsed frames, flush it now.
		m_BufferIn.uValidFrames = 0;
	}

    if ( !m_bLast )
		return m_pInput->VirtualOff( eBehavior );

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Init
// Desc: Initializes the node.
//
// Parameters:
//	AkSoundParams* in_pSoundParams		   : Pointer to the sound parameters.
//	AkAudioFormat* io_pFormat			   : Pointer to the format of the sound.
//
// Return:
//	Ak_Success: Initialization succeeded.
//  AK_Fail:    Initialization failed.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLPitchNode::Init( AkSoundParams * in_pSoundParams,	// SoundParams.					
								AkAudioFormat * io_pFormat,			// Format.
								CAkPBI* in_pPBI, // PBI, to access the initial pbi that created the pipeline.
								AkUInt16 in_usMaxFrames,	//Maximum frames per buffer
								AkUInt32 in_usSampleRate
								)
{
	//m_pSkipCount = in_pSkipCount;
	m_pPBI = in_pPBI;
	AKASSERT( in_pSoundParams != NULL );
	m_SoundParams =	in_pSoundParams;
	m_bLast						= false;
	m_bPadFrameOffset			= true;
	m_usMaxFrames = in_usMaxFrames;

	AKRESULT l_eResult = m_Pitch.Init( io_pFormat, in_usSampleRate );
	AKASSERT( l_eResult == AK_Success );

	return l_eResult;
} // Init

//-----------------------------------------------------------------------------
// Name: Term
// Desc: Term.
//
// Parameters:
//
// Return:
//	AK_Success : Terminated correctly.
//	AK_Fail    : Failed to terminate correctly.
//-----------------------------------------------------------------------------
AKRESULT CAkVPLPitchNode::Term( )
{
	if( m_BufferOut.HasData() )
	{
#ifndef XBOX360
		m_BufferOut.ReleaseCachedBuffer();
#else
		AkFree( g_LEngineDefaultPoolId, m_BufferOut.GetInterleavedData() );
		((AkPipelineBufferBase*)&m_BufferOut)->DetachData();
#endif
	}

	// Release any input markers that could have been left there.
	if( m_BufferIn.pMarkers )
	{
		AkFree( AK_MARKERS_POOL_ID, m_BufferIn.pMarkers );
		m_BufferIn.pMarkers = NULL;
		m_BufferIn.uNumMarkers = 0;
	}

	return m_Pitch.Term();
} // Term


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAkVPLPitchNode::CopyRelevantMarkers( 
	AkPipelineBuffer*	in_pInputBuffer,
	AkPipelineBuffer*	io_pBuffer, 
	AkUInt32			in_ulBufferStartOffset, 
	AkUInt32			in_ulNumFrames
)
{
	if( in_pInputBuffer->pMarkers )
	{
		AKASSERT( in_pInputBuffer->uNumMarkers > 0 );
		
		// First, count the number of markers relevant to this buffer
		AkUInt16 l_usNbMarkersToCopy = 0;
		AkBufferMarker* l_pCurrInputMarker = in_pInputBuffer->pMarkers;
		for( unsigned int i = 0; i < in_pInputBuffer->uNumMarkers; i++ )
		{
			if( ( l_pCurrInputMarker->dwPositionInBuffer >= in_ulBufferStartOffset ) &&
				( l_pCurrInputMarker->dwPositionInBuffer < in_ulBufferStartOffset + in_ulNumFrames ) )
			{
				l_usNbMarkersToCopy++;
			}

			l_pCurrInputMarker++;
		}

		// Now, copy the relevant markers
		if( l_usNbMarkersToCopy )
		{
			AkBufferMarker* l_pNewList = (AkBufferMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkBufferMarker) * ( io_pBuffer->uNumMarkers + l_usNbMarkersToCopy ) );
			if ( l_pNewList )
			{
				if( io_pBuffer->pMarkers )
					AKPLATFORM::AkMemCpy( l_pNewList, io_pBuffer->pMarkers, sizeof(AkBufferMarker) * io_pBuffer->uNumMarkers );

				AkBufferMarker* l_pCurrBufferMarker = l_pNewList + io_pBuffer->uNumMarkers;
				l_pCurrInputMarker = in_pInputBuffer->pMarkers; //reset pointer
				for( unsigned int i = 0; i < in_pInputBuffer->uNumMarkers; i++ )
				{
					if( ( l_pCurrInputMarker->dwPositionInBuffer >= in_ulBufferStartOffset ) &&
						( l_pCurrInputMarker->dwPositionInBuffer < in_ulBufferStartOffset + in_ulNumFrames ) )
					{
						l_pCurrBufferMarker->pContext = l_pCurrInputMarker->pContext;
						l_pCurrBufferMarker->dwPositionInBuffer = 0; //TODO: Find accurate position in output buffer
						l_pCurrBufferMarker->marker   = l_pCurrInputMarker->marker;
						l_pCurrBufferMarker++;
					}

					l_pCurrInputMarker++;
				}

				if( io_pBuffer->pMarkers )
					AkFree( AK_MARKERS_POOL_ID, io_pBuffer->pMarkers );

				io_pBuffer->pMarkers = l_pNewList;
				io_pBuffer->uNumMarkers += l_usNbMarkersToCopy;
			}
			else
			{
				if( io_pBuffer->pMarkers )
					AkFree( AK_MARKERS_POOL_ID, io_pBuffer->pMarkers );
				io_pBuffer->pMarkers = NULL;
				io_pBuffer->uNumMarkers = 0;
			}
		}
	}
}
