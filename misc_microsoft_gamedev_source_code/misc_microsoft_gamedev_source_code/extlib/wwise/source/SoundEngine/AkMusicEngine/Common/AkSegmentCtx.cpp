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

//////////////////////////////////////////////////////////////////////
//
// AkSegmentCtx.cpp
//
// Segment context.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkMusicTrack.h"
#include "AkSound.h"
#include "AkMusicRenderer.h"
#include "AkMusicStructs.h"
#include "AkMonitor.h"
#include "AkMatrixSequencer.h"
#include "AkMatrixAwareCtx.h"


CAkSegmentCtx::CAkSegmentCtx(
    CAkMusicSegment *   in_pSegmentNode,
    CAkMusicCtx *       in_pParentCtx      
    )
    :CAkMusicCtx( in_pParentCtx )
    ,m_pSegmentNode( in_pSegmentNode )
    ,m_pBucket( NULL )
    ,m_iStreamingLookAhead( 0 )
    ,m_bIsInitialScheduling ( true )
#ifndef AK_OPTIMIZED
	,m_PlaylistItemID( AK_INVALID_UNIQUE_ID )
#endif
{
	if( m_pSegmentNode )
		m_pSegmentNode->AddRef();
}

CAkSegmentCtx::~CAkSegmentCtx()
{
    m_sequencer.Term();
	m_arTrackRS.Term();

	if( m_pSegmentNode )
		m_pSegmentNode->Release();
}

AKRESULT CAkSegmentCtx::Init(
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams
    )
{
    CAkMusicCtx::Init( in_GameObject, in_rUserparams );

    AKRESULT eResult = m_sequencer.Init();
        
    if ( eResult == AK_Success )
	{

		AkUInt16 uNumTracks = m_pSegmentNode->NumTracks( );

		if( m_arTrackRS.Reserve( uNumTracks ) != AK_Success )
			return AK_InsufficientMemory;

		for ( AkUInt16 iTrack=0; iTrack<uNumTracks; iTrack++ )
		{
			CAkMusicTrack * pTrack = m_pSegmentNode->Track( iTrack );
			AKASSERT( pTrack );
			m_arTrackRS.AddLast( pTrack->GetNextRS() );// no error check since we did reserve
		}
	}

    return eResult;
}

void CAkSegmentCtx::AttachToBucket(
    CAkSegmentBucket * in_pBucket // Segment bucket.
    )
{
    AKASSERT( in_pBucket );
    m_pBucket = in_pBucket;
}
void CAkSegmentCtx::DetachFromBucket()
{
    AKASSERT( m_pBucket );
    m_pBucket = NULL;
}

// Overriden implementation of PerformNextFrameBehavior(): 
// post AudioSources play() cmds based on current position.
void CAkSegmentCtx::PerformNextFrameBehavior(
	AkUInt32 in_uSubframeOffset,	// Offset in frame to start playback enqueueing.
	AkInt32 in_iFrameDuration		// Number of samples to process.
	)
{
    // Check all node's tracks. For each track, see if 
    // a new source must start playing in the next frame. 
    // If so, send a play cmd.
    // Note. Tracks use absolute positions.
    ProcessSourcesPlaybackStart( m_sequencer.Now() + m_pSegmentNode->PreEntryDuration(), 
								 in_uSubframeOffset,
								 in_iFrameDuration );
    m_bIsInitialScheduling = false;

    // Execute all pending actions that should occur in the next audio frame.
    ExecuteInteractiveCmds( in_uSubframeOffset, in_iFrameDuration );
    
	m_sequencer.Tick( in_iFrameDuration );
}

void CAkSegmentCtx::ProcessSourcesPlaybackStart(
    AkInt32 in_iAbsPosition,
	AkUInt32 in_uSegmentSubframeOffset,		// Offset in frame to start playback enqueueing.
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
	AKASSERT( in_uSegmentSubframeOffset + in_iFrameDuration <= AK_NUM_VOICE_REFILL_FRAMES );

    AkUInt16 uNumTracks = m_pSegmentNode->NumTracks( );
    for ( AkUInt16 iTrack=0; iTrack<uNumTracks; iTrack++ )
    {
        // Get info of source that is supposed to play on this track at this time.
        CAkMusicTrack * pTrack = m_pSegmentNode->Track( iTrack );
        AKASSERT( pTrack );
		CAkMusicTrack::TrackPlaylist* pPlayList = pTrack->GetTrackPlaylist();
		CAkMusicTrack::TrackPlaylist::Iterator itPl = pPlayList->Begin();
		while( itPl != pPlayList->End() )
		{
			if( (*itPl).key == m_arTrackRS[iTrack] )
			{
                const AkTrackSrc *pSrcInfo = &( (*itPl).item );
				AKASSERT ( pSrcInfo );

                // Compute source's look ahead.
                AkInt32 iSrcLookAhead = 0;
                CAkSource * pSrc = pTrack->GetSourcePtr( pSrcInfo->id );

				if( pSrc )
				{
					AkSrcTypeInfo * pSrcTypeInfo = pSrc->GetSrcTypeInfo();
					AKASSERT( pSrcTypeInfo );

					// Look ahead is the source's look-ahead, if it is streaming, and has no prefetched data or
					// play position will not be 0.
					if ( ( pSrcTypeInfo->mediaInfo.Type == SrcTypeFile ) &&
						( !pSrc->IsZeroLatency() ||
						in_iAbsPosition > ( (AkInt32)pSrcInfo->uClipStartPosition ) &&
						pSrcInfo->iSourceTrimOffset == 0 ) )
					{
						iSrcLookAhead = pSrc->StreamingLookAhead();
					}
					AKASSERT( iSrcLookAhead >= 0 );

					// Determine whether we need to create a PBI now.
					bool bDoStartSrcPlayback = false;
					AkInt32 iSourceOffset;
					AkInt32 iEffectivePlayAt = (AkInt32)pSrcInfo->uClipStartPosition - iSrcLookAhead;

					AkUInt32 uPlayOffset = 0;

					if ( !m_bIsInitialScheduling )
					{
						// No frame offset if this is not the first call!
						AKASSERT( in_uSegmentSubframeOffset == 0 );

						// Enqueue Play cmd if playback should start in this audio frame.
						if ( iEffectivePlayAt >= in_iAbsPosition &&
							iEffectivePlayAt < in_iAbsPosition + in_iFrameDuration )
						{
							// Play.
							bDoStartSrcPlayback = true;

							// Compute source offset.
							iSourceOffset = pSrcInfo->iSourceTrimOffset;
							AKASSERT( iSourceOffset >= 0 );

							// Compute play offset (subframe offset + this source's look-ahead duration).
							AKASSERT( iEffectivePlayAt - in_iAbsPosition < in_iFrameDuration );
							uPlayOffset = iEffectivePlayAt - in_iAbsPosition + iSrcLookAhead;
						}
					}
					else
					{
						// Start everything that should be playing in the next frame (taking segment's look-ahead into account).
						AkInt32 iEffectiveStopAt = (AkInt32)( pSrcInfo->uClipStartPosition + pSrcInfo->uClipDuration );
						AkInt32 iEffectiveSegmentPosition = in_iAbsPosition + m_iStreamingLookAhead;
						if ( iEffectivePlayAt < in_iAbsPosition + in_iFrameDuration &&
							iEffectiveStopAt > iEffectiveSegmentPosition )
						{
							// Play 
							bDoStartSrcPlayback = true;

							// Compute play offset: it is the segment's global look-ahead value, plus the time
							// we have before the beginning of the clip, if applicable (below).
							uPlayOffset = m_iStreamingLookAhead;

							// Compute play offset relative to clip.
							AkInt32 iClipStartOffset = iEffectiveSegmentPosition - ( (AkInt32)pSrcInfo->uClipStartPosition );
							if ( iClipStartOffset < 0 )
							{
								// We have time before the beginning of the clip.
								// Source playback offset must be set to the beginning of the clip, 
								// but the play offset must be incremented consequently.
								uPlayOffset -= iClipStartOffset;
								iClipStartOffset = 0;	
							}

							// Compute source offset: translate clip offset into source offset.
							iSourceOffset = ( iClipStartOffset + pSrcInfo->iSourceTrimOffset ) % pSrcInfo->uSrcDuration;
							AKASSERT( iSourceOffset >= 0 );
						}
					}


					if ( bDoStartSrcPlayback )
					{
						uPlayOffset += in_uSegmentSubframeOffset;
						AKASSERT( iSourceOffset >= 0 && 
							(AkInt32)uPlayOffset >= iSrcLookAhead );

						// Compute look-ahead time to take into account for playing and stopping.

						// Trans params: No transition for sources that start playing in NextFrame processing.
						TransParams transParams;
						transParams.TransitionTime = 0;
						CAkMusicPBI * pSrcCtx = NULL;
						AKRESULT eResult = CAkMusicRenderer::Play( this,
							pTrack,
							pTrack->GetSourcePtr( pSrcInfo->id ),
							GameObjectPtr(),
							transParams,
							GetUserParams(),
							iSourceOffset,
							uPlayOffset,
							pSrcCtx );
						if ( eResult == AK_Success )
						{
							AKASSERT( pSrcCtx );

							// Enqueue an explicit Stop action.
							// Action must occur in 
							// source duration + begin_trim_offset + end_trim_offset
							AkMusicAction action;
							action.pTargetPBI = pSrcCtx;
							action.iTime = pSrcInfo->uClipStartPosition + pSrcInfo->uClipDuration - m_pSegmentNode->PreEntryDuration();
							if ( !m_sequencer.ScheduleAction( action ) )
							{
								// Cannot schedule a stop action for this PBI: do not play it.
								pSrcCtx->OnStopped( 0 );
							}
						}
					}
				}
			}
			++itPl;
		}
    }
}


// Context commands
//

// Initialize context for playback.
// Prepare the context for playback: set initial context position.
// Audible playback will start at position in_iSourceOffset (relative to Entry Cue).
// The pre-entry will be heard if in_bPlayPreEntry is true.
// Returns the exact amount of time (samples) that will elapse between call to _Play() and 
// beginning of playback at position in_iSourceOffset.
AkInt32 CAkSegmentCtx::Prepare(
    AkInt32 in_iSourceOffset,           // Position in samples, at the native sample rate, relative to the Entry Marker. Must be positive.
    bool    in_bPlayPreEntry            // True when we require the pre-entry should be heard.
    )
{
    AKASSERT( !IsPlaying() );

	// The position where streaming look-ahead is computed is generally the desired source offset, unless 
	// it is in the pre-entry region, and the client wants to avoid hearing it.
	AkInt32 iLookAheadComputationPosition = in_iSourceOffset;

	// Handle pre-entry. The difference between iLookAheadComputationPosition and in_iSourceOffset will
	// be considered below.
	AkInt32 iPreEntryDuration = m_pSegmentNode->PreEntryDuration();
	if ( !in_bPlayPreEntry )
	{
		// Desired source offset is in the pre-entry, but we do not wish to hear it. Clamp the computation 
		// position at the entry cue.
		if ( iLookAheadComputationPosition < 0 )
			iLookAheadComputationPosition = 0;
	}
	else if ( iLookAheadComputationPosition < -iPreEntryDuration )
	{
		// Desired source start is earlier than the pre-entry. Clamp the computation position at the
		// beginning of the pre-entry.
		iLookAheadComputationPosition = -iPreEntryDuration;
	}

	// Compute the look-ahead time required to begin playback exactly at iLookAheadComputationPosition.
	// Note. MinSrcLookAhead computation uses absolute positions (and it must be positive).
	AKASSERT( ( iLookAheadComputationPosition + iPreEntryDuration ) >= 0 );
    AkInt32 iRequiredLookAhead = ComputeMinSrcLookAhead( iLookAheadComputationPosition + iPreEntryDuration );	
	
	// The streaming look-ahead is the amount of time during which the sequencer is creating PBIs, but with voice
	// pipelines artificially left disconnected.
	m_iStreamingLookAhead = iRequiredLookAhead;

	// Correct the returned required look-ahead value if applicable.
	iRequiredLookAhead += ( in_iSourceOffset - iLookAheadComputationPosition );

	// Prepare the sequencer.
    m_sequencer.Now( in_iSourceOffset - iRequiredLookAhead );

    return iRequiredLookAhead;
}

// Used by sequences, when required look-ahead cannot be honored.
// First few audio sources will not be played back because there is not enough look-ahead.
void CAkSegmentCtx::ForceLookAhead(
    AkInt32 in_iSourceOffset,           // Start position in samples, at the native sample rate, relative to the Entry Marker. Must be positive.
    AkInt32 in_iLookAhead               // Allowed look-ahead value (in samples).
    )
{
    m_sequencer.Now( in_iSourceOffset - in_iLookAhead );
}

void CAkSegmentCtx::OnPlayed(
	AkUInt32 in_uSubFrameOffset 
	)
{
	// Run the first frame now.
	// IMPORTANT. To be consistent with this, frame call from bucket must be executed Before
	// high-level processing.
	PerformNextFrameBehavior( in_uSubFrameOffset, AK_NUM_VOICE_REFILL_FRAMES-in_uSubFrameOffset );

	CAkMusicCtx::OnPlayed( in_uSubFrameOffset );
	NotifyAction( AkMonitorData::NotificationReason_MusicSegmentStarted );
}

// Override MusicCtx OnStopped: Need to flush actions to release children.
void CAkSegmentCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
    // AddRef ourselves in case sequencer dequeuing destroys us.
    AddRef();
    m_sequencer.Flush();
    
    if ( m_pBucket )
    {
		// Note. NotifyAction must be called before CAkMusicCtx::OnStopped() so that it sends a notification.
		// "if ( m_pBucket )" ensures that the notification is sent once.
		// REVIEW. 
		NotifyAction( AkMonitorData::NotificationReason_MusicSegmentEnded );

		// Stop this context and all its child PBIs.
		CAkMusicCtx::OnStopped( in_uSubFrameOffset );
	    
		// Notify bucket. Note: It is important that notification occurs after MusicCtx's IsPlaying()
		// is set to false, so that actions don't get rescheduled in this segment.
        m_pBucket->OnSegmentStoppedNotification( in_uSubFrameOffset );
    }
	else
	{
		// No bucket: it must be a second call to OnStopped() from within the same audio frame. 
		// Just propagate the command below so that the stop offset be used if it is more restrictive.
		CAkMusicCtx::OnStopped( in_uSubFrameOffset );
	}

    Release();
}

void CAkSegmentCtx::ExecuteInteractiveCmds(
	AkUInt32 in_uSubframeOffset,	// Offset in frame to start playback enqueueing.
	AkInt32 in_iFrameDuration		// Number of samples to process.
	)
{
	AKASSERT( in_uSubframeOffset + in_iFrameDuration <= AK_NUM_VOICE_REFILL_FRAMES );

    AkMusicAction action;
    // Get next action.
    while ( m_sequencer.PopImminentAction( in_iFrameDuration, action ) == AK_DataReady )
    {
        // Execute action.
		AKASSERT( in_uSubframeOffset + action.iTime - m_sequencer.Now() < AK_NUM_VOICE_REFILL_FRAMES );
        action.pTargetPBI->_Stop( in_uSubframeOffset + action.iTime - m_sequencer.Now() );
    }
}

// Computes context's look-ahead value (samples) according to the position specified.
AkInt32 CAkSegmentCtx::ComputeMinSrcLookAhead(
    AkInt32 in_iAbsPosition
    )
{
	AKASSERT( in_iAbsPosition >= 0 );
    AkInt32 iContextLookAhead = 0;

    AkUInt16 uNumTracks = m_pSegmentNode->NumTracks( );
    for ( AkUInt16 iTrack=0; iTrack<uNumTracks; iTrack++ )
    {
        // Get info from all sources on the required subtrack.
        CAkMusicTrack * pTrack = m_pSegmentNode->Track( iTrack );
        AKASSERT( pTrack );
		CAkMusicTrack::TrackPlaylist* pPlayList = pTrack->GetTrackPlaylist();
		CAkMusicTrack::TrackPlaylist::Iterator itPl = pPlayList->Begin();
		while( itPl != pPlayList->End() )
		{
			if( (*itPl).key == m_arTrackRS[iTrack] )
			{
                const AkTrackSrc *pSrcInfo = &( (*itPl).item );
				AKASSERT ( pSrcInfo );

                CAkSource * pSrc = pTrack->GetSourcePtr( pSrcInfo->id );

				if( pSrc )
				{
					AkSrcTypeInfo * pSrcTypeInfo = pSrc->GetSrcTypeInfo();
					AKASSERT( pSrcTypeInfo );

					// Get source's relative start position. If < 0, make 0.
					AkInt32 iRelativeStartPos = pSrcInfo->uClipStartPosition - in_iAbsPosition;


					// Get required look-ahead for this source at position in_iPosition.
					// Look ahead is the source's look-ahead, if it is streaming, and has no prefetched data or
					// play position will not be 0.
					AkInt32 iSrcRelLookAhead = 0;
					if ( ( pSrcTypeInfo->mediaInfo.Type == SrcTypeFile ) &&
						( !pSrc->IsZeroLatency() || iRelativeStartPos < 0 ) )
					{
						iSrcRelLookAhead = pSrc->StreamingLookAhead();
					}

					if ( iRelativeStartPos < 0 )
						iRelativeStartPos = 0;

					iSrcRelLookAhead -= iRelativeStartPos;

					if ( iContextLookAhead < iSrcRelLookAhead )
						iContextLookAhead = iSrcRelLookAhead;
				}
				else
				{
					MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedChildNotAvailable );
				}
			}
			++itPl;
		}
    }

    return iContextLookAhead;
}


// PBI notifications.
//

// Called when PBI destruction occurred from Lower engine without the higher-level hierarchy knowing it.
// Remove all references to this target from this segment's sequencer.
void CAkSegmentCtx::RemoveAllReferences( 
    CAkPBI * in_pCtx   // Context whose reference must be removed.
    )
{
    m_sequencer.ClearActionsByTarget( in_pCtx );
}

CAkRegisteredObj * CAkSegmentCtx::GameObjectPtr()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->GameObjectPtr(); 
}

AkPlayingID CAkSegmentCtx::PlayingID()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->PlayingID(); 
}

UserParams & CAkSegmentCtx::GetUserParams()
{ 
    AKASSERT( Parent() );
    return static_cast<CAkMatrixAwareCtx*>(Parent())->Sequencer()->GetUserParams(); 
}

void CAkSegmentCtx::NotifyAction( AkMonitorData::NotificationReason in_eReason )
{
#ifndef AK_OPTIMIZED
	if( IsPlaying() && GetPlayListItemID() != AK_INVALID_UNIQUE_ID )
	{
		AKASSERT( SegmentNode() && SegmentNode()->Parent() && SegmentNode()->Parent()->NodeCategory() == AkNodeCategory_MusicRanSeqCntr );
		// At this point, SegmentNode()->Parent() is a MusicRanSeqCntr
		MONITOR_MUSICOBJECTNOTIF( PlayingID(), GameObjectPtr()->ID(), GetUserParams().CustomParam, in_eReason, SegmentNode()->Parent()->ID(), GetPlayListItemID() );
	}
#endif
}
