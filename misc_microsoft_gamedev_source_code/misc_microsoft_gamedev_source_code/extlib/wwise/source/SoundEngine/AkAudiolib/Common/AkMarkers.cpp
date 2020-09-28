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

#include "StdAfx.h"
#include "AkMarkers.h"
#include "AkPlayingMgr.h"
#include "AkPositionRepository.h"
#include "AudiolibDefs.h"
#include "AkPBI.h"

CAkMarkers::CAkMarkers( CAkPBI * in_pCtx )
	: m_pMarkers( NULL )
{
	m_hdrMarkers.uNumMarkers = 0;  // Markers header.
	g_pPlayingMgr->GetNotificationInformation( in_pCtx, &m_bWantMarkerNotifications, &m_bWantPositionInformation );
}

CAkMarkers::~CAkMarkers()
{
	AKASSERT( m_pMarkers == NULL );
}

AKRESULT CAkMarkers::Allocate( AkUInt32 in_uNumMarkers )
{
	AKASSERT( in_uNumMarkers > 0 );
	m_hdrMarkers.uNumMarkers = in_uNumMarkers;
	
	m_pMarkers = (AkAudioMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkAudioMarker) * in_uNumMarkers );
	if ( !m_pMarkers )
	{
		// Could not allocate enough cue points.
		m_hdrMarkers.uNumMarkers = 0;
		return AK_InsufficientMemory;
	}
	return AK_Success;
}

void CAkMarkers::Free()
{
	// Clean markers.
    if ( m_pMarkers )
    {
		for( AkUInt32 i=0; i<m_hdrMarkers.uNumMarkers; i++ )
		{
			if( m_pMarkers[i].strLabel )
			{
				AkFree( AK_MARKERS_POOL_ID, m_pMarkers[i].strLabel );
				m_pMarkers[i].strLabel = NULL;
			}
		}

        AkFree( AK_MARKERS_POOL_ID, m_pMarkers );
        m_pMarkers = NULL;
    }
	m_hdrMarkers.uNumMarkers = 0;
}

AKRESULT CAkMarkers::SetLabel( AkUInt32 in_idx, char * in_psLabel, AkUInt32 in_uStrSize )
{
	AKASSERT( in_uStrSize > 0 );
	char * strLabel = (char*)AkAlloc( AK_MARKERS_POOL_ID, in_uStrSize + 1 );
	if ( strLabel )
	{
		AKPLATFORM::AkMemCpy( strLabel, in_psLabel, in_uStrSize );
		strLabel[in_uStrSize] = '\0'; //append final NULL character
		m_pMarkers[in_idx].strLabel = strLabel;
		return AK_Success;
	}
	return AK_InsufficientMemory;
}

void CAkMarkers::CopyRelevantMarkers(
	CAkPBI* in_pCtx,
	AkPipelineBuffer & io_buffer, 
	AkUInt32 in_ulBufferStartPos,
	AkUInt32 in_uPCMLoopStart,
	AkUInt32 in_uPCMLoopEnd,	
	bool in_bDidLoop /* = false */,
	const AkUInt32 * in_puForceNumFrames /*=NULL*/	// uses this number of frames instead of deducing it from io_buffer.
)
{
	if( m_pMarkers && m_bWantMarkerNotifications )
	{
		AkUInt32 uNumFrames = ( in_puForceNumFrames ) ? *in_puForceNumFrames : io_buffer.uValidFrames;
		AKASSERT( uNumFrames <= io_buffer.uValidFrames );

		// First, count the number of markers relevant to this buffer
		io_buffer.pMarkers = NULL;
		io_buffer.uNumMarkers = 0;

		if( in_bDidLoop )
		{
			for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
			{
				if	(		
						(
						( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) 
						&&
						( m_pMarkers[i].dwPosition < in_uPCMLoopEnd )
						)
					 ||
						(
						( m_pMarkers[i].dwPosition >= in_uPCMLoopStart ) 
						&&
						( m_pMarkers[i].dwPosition < in_uPCMLoopStart + uNumFrames - ( in_uPCMLoopEnd - in_ulBufferStartPos ) )
						)
					 )
				{
					io_buffer.uNumMarkers++;
				}
			}
		}
		else
		{
			for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
			{
				if( ( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) &&
					( m_pMarkers[i].dwPosition < in_ulBufferStartPos + uNumFrames ) )
				{
					io_buffer.uNumMarkers++;
				}
			}
		}

		// Now, copy the relevant markers
		if( io_buffer.uNumMarkers )
		{
			io_buffer.pMarkers = (AkBufferMarker*)AkAlloc( AK_MARKERS_POOL_ID, sizeof(AkBufferMarker) * io_buffer.uNumMarkers );
			if ( io_buffer.pMarkers )
			{
				AkBufferMarker* l_pCurrBufferMarker = io_buffer.pMarkers;
				if( in_bDidLoop )
				{
					for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
					{
						if	(		
							(
							( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) 
							&&
							( m_pMarkers[i].dwPosition < in_uPCMLoopEnd )
							)
						 ||
							(
							( m_pMarkers[i].dwPosition >= in_uPCMLoopStart ) 
							&&
							( m_pMarkers[i].dwPosition < in_uPCMLoopStart + uNumFrames - ( in_uPCMLoopEnd - in_ulBufferStartPos ) )
							)
						 )
						{
							l_pCurrBufferMarker->pContext = in_pCtx;
							if( m_pMarkers[i].dwPosition >= in_ulBufferStartPos )
							{
								l_pCurrBufferMarker->dwPositionInBuffer = m_pMarkers[i].dwPosition - in_ulBufferStartPos;
							}
							else
							{
								l_pCurrBufferMarker->dwPositionInBuffer = m_pMarkers[i].dwPosition + ( in_uPCMLoopEnd - in_ulBufferStartPos ) - in_uPCMLoopStart;
							}
							l_pCurrBufferMarker->marker   = m_pMarkers[i];
							l_pCurrBufferMarker++;
						}
					}
				}
				else
				{
					for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
					{
						if( ( m_pMarkers[i].dwPosition >= in_ulBufferStartPos ) &&
							( m_pMarkers[i].dwPosition < in_ulBufferStartPos + uNumFrames ) )
						{
							l_pCurrBufferMarker->pContext = in_pCtx;
							l_pCurrBufferMarker->dwPositionInBuffer = m_pMarkers[i].dwPosition - in_ulBufferStartPos;
							l_pCurrBufferMarker->marker   = m_pMarkers[i];
							l_pCurrBufferMarker++;
						}
					}
				}
			}
			else
			{
				// Failed allocating buffer for markers. They are lost forever.
				io_buffer.uNumMarkers = 0;
			}
		}
	}
}

void CAkMarkers::NotifyRelevantMarkers( CAkPBI * in_pCtx, AkUInt32 in_uStartSample, AkUInt32 in_uStopSample )
{
	if( m_pMarkers && m_bWantMarkerNotifications )
	{
		for( unsigned int i = 0; i < m_hdrMarkers.uNumMarkers; i++ )
		{
			if( ( m_pMarkers[i].dwPosition >= in_uStartSample ) &&
				( m_pMarkers[i].dwPosition < in_uStopSample ) )
			{
				g_pPlayingMgr->NotifyMarker( in_pCtx, &m_pMarkers[i] );
			}
		}
	}
}

void CAkMarkers::UpdatePositionInfo( CAkPBI * in_pCtx, AkReal32 in_fLastRate, AkUInt32 in_uStartPos, AkUInt32 in_uFileEnd )
{
	AKASSERT( m_bWantPositionInformation );

	AkBufferPosInformation bufferPosInfo;
		
	bufferPosInfo.uSampleRate = in_pCtx->GetMediaFormat()->uSampleRate;
	bufferPosInfo.uStartPos = in_uStartPos;
	bufferPosInfo.uFileEnd = in_uFileEnd;
	bufferPosInfo.fLastRate = in_fLastRate;

	g_pPositionRepository->UpdatePositionInfo( in_pCtx->GetPlayingID(), &bufferPosInfo, this );
}

void CAkMarkers::TimeSkipMarkers( CAkPBI * in_pCtx, AkReal32 in_fLastRate, AkUInt32 in_ulCurrSampleOffset, AkUInt32 in_uSkippedSamples, AkUInt32 in_uFileEnd )
{
	AkUInt32 uEffectiveFramesRequested = in_uSkippedSamples;// * GetPitchSRRatio();
/*	if ( DoLoop() )
	{
		if ( m_uCurSample + uEffectiveFramesRequested >= m_uLoopEndSample )
		{
			NotifyRelevantMarkers( m_uCurSample, m_uLoopEndSample - in_ulCurrSampleOffset );
			UpdatePositionInfo( m_uCurSample, in_uEndSample );
			m_uCurSample = m_uLoopStartSample;
			return;
		}
	}
*/
	NotifyRelevantMarkers( in_pCtx, in_ulCurrSampleOffset, in_ulCurrSampleOffset + uEffectiveFramesRequested );

	if ( m_bWantPositionInformation )
		UpdatePositionInfo( in_pCtx, in_fLastRate, in_ulCurrSampleOffset, in_uFileEnd );

}

