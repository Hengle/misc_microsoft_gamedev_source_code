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
// AkMatrixSequencer.cpp
//
// Multi-chain, branchable segment sequencer.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMatrixSequencer.h"
#include "AkMusicSegment.h"
#include "AkMusicRenderer.h"
#include "AkMusicSwitchCtx.h"
#include "AkMonitor.h"


CAkTimeSequencedItem::CAkTimeSequencedItem(
    AkInt32 in_iRelativeTime,
    bool    in_bIsSegment
    )
:pNextItem( NULL )
,m_iRelativeTime( in_iRelativeTime )
,m_iEarliestCmdTime( 0 )
,m_bIsSegment( in_bIsSegment )
{
}

CAkTimeSequencedItem::~CAkTimeSequencedItem()
{
}

void CAkTimeSequencedItem::ProcessActions(
	AkInt32 in_iSyncTime,				// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
	AkInt32 & io_iFrameDuration,		// Number of samples to process. If the sync point is crossed, it comes out as the remaining of the frame after sync.
	bool	in_bDoConsume				// True when sync time must be consumed. False if processed as a slave.
    )
{
    if ( IsSegment() )
    {
		// Power-up segment context.
		CAkSegmentCtx * pSegmentCtx = static_cast<CAkSegmentBucket*>(this)->SegmentCtx();
		if ( pSegmentCtx && 
			 pSegmentCtx->IsPlaying() )
		{
			AKASSERT( !pSegmentCtx->IsPaused() );
			pSegmentCtx->PerformNextFrameBehavior( 0, io_iFrameDuration );
		}

		// Perform bucket actions.
        if ( EarliestActionTime() + in_iSyncTime >= 0 )
        {
            // At least one action exists in the future: time must be consumed.
            
            if ( EarliestActionTime() + in_iSyncTime < io_iFrameDuration )
            {
                // There is at least one action that needs to be processed for the next audio frame.
                ExecuteImminentActions( in_iSyncTime, io_iFrameDuration );
            }

			if ( in_bDoConsume )
	            ConsumeTime( io_iFrameDuration );

			// Check if we synched in this frame. If we did, modify io_iFrameDuration so that it becomes
			// equal to the remaining.
            if ( in_iSyncTime >= 0 && 
				 in_iSyncTime < io_iFrameDuration )
			{				
				io_iFrameDuration -= in_iSyncTime;
			}
        }
    }
    else if ( in_bDoConsume )
    {
        ConsumeTime( io_iFrameDuration );
    }
}

CAkSegmentBucket * CAkTimeSequencedItem::GetNextBucket(
    CAkMatrixAwareCtx * in_pOwner,
    CAkSegmentChain *&  io_pChain,  // chain object passed along
    AkInt32 &           out_iTimeOffset // time offset between this item and return CAkSegmentBucket (always >= 0).
    )
{
	CAkSegmentBucket * pNextBucket;
	if ( pNextItem )
	{
		if ( pNextItem->IsSegment() )
		{
	        pNextBucket = static_cast<CAkSegmentBucket*>( pNextItem );
			out_iTimeOffset = pNextItem->SyncTime();
		}
		else
		{
			pNextBucket = pNextItem->GetNextBucket( in_pOwner, io_pChain, out_iTimeOffset );
		}
	}
    else
    {
        pNextBucket = io_pChain->ForceGrow( out_iTimeOffset );
        AKASSERT( !pNextBucket || pNextBucket->IsSegment() );
    }

    
    return pNextBucket;
}


///////////////////////////////////////////////////

CAkSegmentBucket::CAkSegmentBucket(
    AkInt32 in_iRelativeTime,
    CAkSegmentCtx * in_pSegment
    )
:CAkTimeSequencedItem( in_iRelativeTime, true )
,m_pSegment( in_pSegment )
,m_iSegmentStartPos( 0 )
,m_iOriginalStopTime( 0 )
,m_bIsVolatile( false )
{
    if ( m_pSegment )
        m_pSegment->AttachToBucket( this );
}

CAkSegmentBucket::~CAkSegmentBucket()
{
    AKASSERT( !m_pSegment );
    if ( m_pSegment )
        m_pSegment->DetachFromBucket();
    m_pSegment = NULL;

    m_arAssociatedActions.Term();
}

// Returns segment's active duration (0 if no segment).
AkInt32 CAkSegmentBucket::SegmentDuration()
{
	if ( m_pSegment )
		return m_pSegment->SegmentNode()->ActiveDuration();
	return 0;
}

// Called by segment context. Flush Play/Stop actions, detach context.
void CAkSegmentBucket::OnSegmentStoppedNotification(
	AkUInt32 in_uSubFrameOffset
	)
{
	// Must be called from segment context AFTER its flag IsPlaying was reset (in order to avoid
	// scheduling actions in this context again while cancelling actions).
	AKASSERT( m_pSegment &&
			  !m_pSegment->IsPlaying() );

    // Note: Flush play/stop actions (fake by making them "occur in the past").
    m_cmdStop.iRelativeTime = m_cmdPlay.iRelativeTime = SyncTime() - 1;
    
    // Cancel all associated actions that are supposed to occur AFTER the precise stop time.
    AssociatedActionsArray arCancelledStatesActions;
	AkInt32 iMinActionTimeToCancel;
	if ( in_uSubFrameOffset == AK_NO_IN_BUFFER_STOP_REQUESTED )
	{
		// Stopping should occur at the end of the frame.
		iMinActionTimeToCancel = AK_NUM_VOICE_REFILL_FRAMES - SyncTime();
	}
	else
	{
		// Stopping should occur inside the frame.
		iMinActionTimeToCancel = in_uSubFrameOffset - SyncTime();
	}
    PopAssociatedActionsToReschedule( arCancelledStatesActions, iMinActionTimeToCancel );
    // Process cancelled actions list.
    AssociatedActionsArray::Iterator itAction = arCancelledStatesActions.Begin();
    while ( itAction != arCancelledStatesActions.End() )
    {
        if ( (*itAction).eActionType == AssocActionTypeState )
        {
            // Reschedule State change through Music Renderer ("last chance only").
            CAkMusicRenderer::Get()->RescheduleDelayedStateChange( (*itAction).pStateChangeCookie );
        }
        ++itAction;
    }
    arCancelledStatesActions.Term();

    // Recompute EarliestActionTime since we removed play, stop and states.
    RecomputeEarliestActionTime( -SyncTime() );

    // Detach from segment.
    m_pSegment->DetachFromBucket();
    m_pSegment = NULL;
	m_bIsVolatile = true;
}

// Prepare bucket. 
// Prepares segment context, modifies associated Play action time to look-ahead.
// The segment will be ready to play at the specified Entry Position (relative to EntryCue). They might however
// be set to begin earlier, in case in_iFadeOffset is negative, but in this case, the returned preparation
// time takes this duration into account.
AkInt32 CAkSegmentBucket::Prepare( 
    AkInt32 in_iEntryPosition,	// Entry Position, relative to Entry Cue. Always >= 0.
    const AkMusicFade & in_iFade,	// Fade. Real source offset will take fade offset into account. 
    bool    in_bPlayPreEntry	// True when the pre-entry should be heard.
    )
{
    AKASSERT( in_iEntryPosition >= 0 );

	// An empty bucket always takes 0 look-ahead time
	AkInt32 iBucketLookAhead = 0;

	if ( m_pSegment )
	{	
		// Store entry position: this is the offset between bucket time and segment time.
		m_iSegmentStartPos = in_iEntryPosition;
		
		
		// Prepare segment.
		AkInt32 iEffectiveEntryPosition = in_iEntryPosition;
		if ( in_iFade.transitionTime > 0 )
			iEffectiveEntryPosition += in_iFade.iFadeOffset;
		else if ( in_bPlayPreEntry )
			iEffectiveEntryPosition -= m_pSegment->SegmentNode()->PreEntryDuration();


		AkInt32 iSegmentLookAhead = m_pSegment->Prepare( iEffectiveEntryPosition, in_bPlayPreEntry );


		// Update bucket.
		// 
		iBucketLookAhead = iSegmentLookAhead + ( in_iEntryPosition - iEffectiveEntryPosition );
	}
	
	// Change Play action relative time to new look-ahead.
	m_cmdPlay.iRelativeTime = -iBucketLookAhead;

	// Also, clear fade on segment play command: a high-level context will take care of it.
	m_cmdPlay.fadeParams.transitionTime = 0;

	// Update EarliestActionTime.
	EarliestActionTime( AkMin( m_cmdPlay.iRelativeTime, m_cmdStop.iRelativeTime ) );

	// Change this sync time to look-ahead. This will be the default value, until switch item insertion
	// changes it, if applicable.
	ForceTimeToSync( iBucketLookAhead );

    return iBucketLookAhead;
}

AkInt32 CAkSegmentBucket::Prepare( 
    AkInt32 in_iEntryPosition	// Entry Position, relative to Entry Cue. Can be negative.
    )
{
    AKASSERT( m_pSegment );
	
	// Store entry position: this is the offset between bucket time and segment time.
	m_iSegmentStartPos = in_iEntryPosition;
	
	
	// Prepare segment.
	AkInt32 iSegmentLookAhead = m_pSegment->Prepare( in_iEntryPosition, true );

	AkInt32 iBucketLookAhead = iSegmentLookAhead;
	if ( in_iEntryPosition < 0 )
		iBucketLookAhead -= in_iEntryPosition;


    // Update bucket.
	// 
	
	// Change Play action relative time to new look-ahead.
	m_cmdPlay.iRelativeTime = -iBucketLookAhead;

	// Also, clear fade on segment play command: a high-level context will take care of it.
	m_cmdPlay.fadeParams.transitionTime = 0;

	// Update EarliestActionTime. Play cmd should always be the first command to occur on Prepare()d buckets.
	EarliestActionTime( m_cmdPlay.iRelativeTime );

	// Change this sync time to look-ahead. This will be the default value, until switch item insertion
	// changes it, if applicable.
	ForceTimeToSync( iBucketLookAhead );

    return iBucketLookAhead;
}

// Find a sync point in bucket given sync rules and time constraints
// Returns time of sync relative to the bucket's time.
// If no possible exit point is found the method returns AK_Fail. AK_Success if it was found.
// Note: Considers following branch items.
AKRESULT CAkSegmentBucket::FindSyncPosition(
	CAkMatrixAwareCtx * in_pOwner,			// Owner for branching decision.
	CAkSegmentChain *   in_pChain,			// Chain object passed along
    AkInt32         in_iTimeConstraint,     // Time constraint: minimum time before finding a valid sync point (absolute).
	AkInt32         in_iMinSegmentPosition,	// Time constraint: minimum segment position (segment relative position, default 0).
    AkSyncType      in_eSyncType,           // Sync rule.
    bool            in_bDoSkipEntryMarker,  // If true, will not consider Entry marker (returned position will be exclusively greater than 0).
	bool			in_bSucceedOnNothing,	// If true, will return a valid position if segment is <nothing>. Otherwise, will succeed only if SyncType is Immediate.
    AkInt32 &       out_iSyncPosition		// Returned Sync position, relative to bucket's time.
    )
{
	/** NOTE: (WG-8113) Although it is not natural to find a sync position over a segment that will never play, it can
	happen in some situations that involve multiple levels of switch containers. 
	The side effect is that a transition will be scheduled for nothing. See WG-8114.
	AKASSERT( !WasPlaybackSkipped() );
	**/
	
	// "Translate" time constraint in terms of segment's position.
	// Cannot return a position that is earlier than the segment's current position.
	if ( in_iTimeConstraint < 0)
		in_iTimeConstraint = 0;
	
	AkInt32 iMinSegmentPosition;
	if ( m_pSegment )
		iMinSegmentPosition = m_pSegment->Position();
	else
		iMinSegmentPosition = -SyncTime();
	/* Note. Technically it should be 

	AkInt32 iMinSegmentPosition = -SyncTime();
	if ( m_pSegment && iMinSegmentPosition < m_pSegment->Position() )
		iMinSegmentPosition = m_pSegment->Position();

	but m_pSegment->Position() can never be smaller than -SyncTime(). m_iSegmentStartPos is always >= 0.
	*/

	// Segment position at seeked sync point should be greater or equal to user defined segment-relative position.
	if ( in_iMinSegmentPosition > iMinSegmentPosition )
		iMinSegmentPosition = in_iMinSegmentPosition;
	
	// Segment position at seeked sync point should be greater or equal to segment's position at 
	// bucket's sync point.
	if ( m_iSegmentStartPos > iMinSegmentPosition )
		iMinSegmentPosition = m_iSegmentStartPos;
	
	// Offset absolute time constraint with segment position constraint.
	if ( !in_bDoSkipEntryMarker ||
		 iMinSegmentPosition > 0 )
	{
		in_iTimeConstraint += iMinSegmentPosition;
	}
    
	AKRESULT eSyncFoundResult = AK_Fail;
	if ( m_pSegment )
	{
		CAkMusicSegment * pSegmentNode = m_pSegment->SegmentNode();
		AKASSERT( pSegmentNode );

		eSyncFoundResult = pSegmentNode->GetExitSyncPos(
			in_iTimeConstraint,
			in_eSyncType,
			in_bDoSkipEntryMarker,  // consider entry marker.
			out_iSyncPosition );
	}
	else
	{
		// Only Immediate sync types can be scheduled over empty buckets.
		// Some clients might want to automatically transform sync types into Immediate (Switch transitions for e.g.).
		if ( in_bSucceedOnNothing ||
			 in_eSyncType == SyncTypeImmediate )
		{
			out_iSyncPosition = in_iTimeConstraint;
			eSyncFoundResult = AK_Success; 
		}
	}

	// Segment node's GetExitSyncPos() returns a position that is relative to the segment's Entry marker.
	// Convert it in a position that is relative to the bucket.
	out_iSyncPosition -= m_iSegmentStartPos;

	// Verify that the position returned will occur before next bucket (considering branching).
	if ( eSyncFoundResult == AK_Success )
	{
		AkInt32 iTimeBeforeNextBucket;
		if ( GetNextBucket( in_pOwner, in_pChain, iTimeBeforeNextBucket ) )
		{
			// out_iSyncPosition is relative to the beginning of this bucket, iTimeBeforeNextBucket is the time
			// remaining before the next bucket syncs. If this bucket has already synched, compare with adjusted value.
			AkInt32 iTimeBeforeSync = ConvertToAbsoluteTime( out_iSyncPosition );
			if ( iTimeBeforeSync > iTimeBeforeNextBucket )
				eSyncFoundResult = AK_Fail;
		}
	}

	return eSyncFoundResult;
}

// Convert bucket-relative positions into segment context-relative positions.
AkInt32 CAkSegmentBucket::ConvertToSegmentCtxPosition(
	AkInt32			in_iBucketRelativePos	// Bucket relative position.
	)
{
	return in_iBucketRelativePos += m_iSegmentStartPos;
}

// Convert bucket-relative positions to absolute time (position - NOW).
// Important: Conversion does not apply if this bucket is scheduled in the future 
// (has not synched, SyncTime()>0): position returned = position given.
AkInt32 CAkSegmentBucket::ConvertToAbsoluteTime(
	AkInt32			in_iBucketRelativePos	// Bucket relative position.
	)
{
	if ( SyncTime() < 0 )
		return in_iBucketRelativePos + SyncTime();
	return in_iBucketRelativePos;
}

// Commands scheduling.
void CAkSegmentBucket::AttachPlayCmd(
    AkTimeMs                in_iTransDuration,
    AkCurveInterpolation    in_eFadeCurve,
    AkInt32                 in_iFadeOffset,
    AkInt32                 in_iRelativeTime
    )
{
    AKASSERT( m_pSegment );
    m_cmdPlay.fadeParams.transitionTime = in_iTransDuration;
    m_cmdPlay.fadeParams.eFadeCurve     = in_eFadeCurve;
    m_cmdPlay.fadeParams.iFadeOffset    = in_iFadeOffset;
    m_cmdPlay.iRelativeTime = in_iRelativeTime;
    if ( in_iRelativeTime < EarliestActionTime() )
        EarliestActionTime( in_iRelativeTime );
}
void CAkSegmentBucket::AttachPlayCmd(
    const AkMusicFade &     in_fadeParams,
    AkInt32                 in_iRelativeTime
    )
{
    AKASSERT( m_pSegment );
    m_cmdPlay.fadeParams    = in_fadeParams;
    m_cmdPlay.iRelativeTime = in_iRelativeTime;
    if ( in_iRelativeTime < EarliestActionTime() )
        EarliestActionTime( in_iRelativeTime );
}

void CAkSegmentBucket::AttachStopCmd(
    AkTimeMs                in_iTransDuration,
    AkCurveInterpolation    in_eFadeCurve,
    AkInt32                 in_iRelativeTime
    )
{
    AKASSERT( m_pSegment );
	m_cmdStop.transParams.TransitionTime = in_iTransDuration;

	// Deal with fades that are too long, stop times that are too early.
	if ( in_iRelativeTime < m_cmdPlay.iRelativeTime )
	{
		m_cmdStop.transParams.TransitionTime += CAkTimeConv::SamplesToMilliseconds( in_iRelativeTime - m_cmdPlay.iRelativeTime );
		in_iRelativeTime = m_cmdPlay.iRelativeTime;
	}

    m_cmdStop.transParams.eFadeCurve = in_eFadeCurve;
    m_cmdStop.iRelativeTime = in_iRelativeTime;
	m_iOriginalStopTime = in_iRelativeTime;
	if ( in_iRelativeTime < EarliestActionTime() )
        EarliestActionTime( in_iRelativeTime );
}

// Modify Stop Cmd time: perform a straight stop at either the Exit Cue or end of Post-Exit.
// Notes. 1) Stop command is left untouched if a fade out is already defined.
// 2) Stop command is irrelevant if the bucket is empty.
void CAkSegmentBucket::ForcePostExit(
    bool                    in_bPlayPostExit    // Move stop at the end of Post-Exit if True, and Exit Cue if false.
    )
{
    if ( SegmentCtx() &&
		 m_cmdStop.transParams.TransitionTime == 0 )
    {
        CAkMusicSegment * pNode = SegmentCtx()->SegmentNode();
        if ( in_bPlayPostExit )
            m_cmdStop.iRelativeTime = pNode->ActiveDuration() - m_iSegmentStartPos + pNode->PostExitDuration();
        else
            m_cmdStop.iRelativeTime = pNode->ActiveDuration() - m_iSegmentStartPos;

        RecomputeEarliestActionTime( EarliestActionTime() );
    }
}

void CAkSegmentBucket::FixStopTime()
{
	m_cmdStop.iRelativeTime = m_iOriginalStopTime - m_iSegmentStartPos;
	AKASSERT( m_cmdPlay.iRelativeTime <= m_cmdStop.iRelativeTime );
}

// Inhibates playback: of bucket's segment. Play command's InhibatePlayback flag is set,
// which will block its execution. If the play command already occurred, it is stopped
// (with a fade-out, which length equals the minimum between the time elapsed since the
// command occurred and the sync time).
// Warning: must not be called after bucket has synched.
void CAkSegmentBucket::DisablePlayback()
{
	AKASSERT( SyncTime() >= 0 );
	m_cmdPlay.bIsPlaybackDisabled = true;
}
void CAkSegmentBucket::EnablePlayback()
{
	AKASSERT( !WasPlaybackSkipped() );	// Cannot re-enable playback if play command was skipped.
	m_cmdPlay.bIsPlaybackDisabled = false;
}

// Query and revert action on playback. These should be called only before Sync (they are usable to revert
// the pre-entry and/or streaming look-ahead).
bool CAkSegmentBucket::HasPlaybackStarted()
{
	return m_pSegment && m_pSegment->IsPlaying();
}
void CAkSegmentBucket::RevertPlayback()
{
	AKASSERT( HasPlaybackStarted() && SyncTime() >= 0 );
	
	// Fade out segment that started playing.
	TransParams revTransParams;
	revTransParams.eFadeCurve = AkCurveInterpolation_Linear;
	AkInt32 iTimeElapsed = -( m_cmdPlay.iRelativeTime + SyncTime() );
	revTransParams.TransitionTime = CAkTimeConv::SamplesToMilliseconds( AkMin( iTimeElapsed, SyncTime() ) );
	m_pSegment->_Stop( revTransParams );

	// Tag the bucket as 'bWasPlaybackSkipped': switch contexts must not revert to this bucket.
	m_cmdPlay.bWasPlaybackSkipped = true;
}

AKRESULT CAkSegmentBucket::AttachAssociatedAction(
    AkAssociatedAction & in_action
    )
{
    if ( m_arAssociatedActions.AddLast( in_action ) )
    {
        if ( m_pSegment )
        {
            if ( in_action.iRelativeTime < EarliestActionTime() )
                EarliestActionTime( in_action.iRelativeTime );
        }
        else
        {
            // Note. Special case for empty buckets: recompute earliest action time because it is set to 0 by default.
            RecomputeEarliestActionTime( -SyncTime() );
        }
        return AK_Success;
    }
    return AK_Fail;
}

void CAkSegmentBucket::PopAssociatedActionsToReschedule(
    AssociatedActionsArray & io_arCancelledActions,
    AkInt32 in_iMinActionTime
    )
{
    bool bHasActionsPopped = false;
    AssociatedActionsArray::Iterator itAction = m_arAssociatedActions.Begin();
    while ( itAction != m_arAssociatedActions.End() )
    {
        // Cancel if time is greater than in_iMinActionTime and action is not a playing stinger.
        if ( (*itAction).iRelativeTime > in_iMinActionTime &&
             ( (*itAction).eActionType != AssocActionTypeStinger ||
               !(*itAction).pStingerRecord->bPlaybackStarted ) )
        {
            // Remove and Push in output array.
            io_arCancelledActions.AddLast( (*itAction) );
            itAction = m_arAssociatedActions.EraseSwap( itAction );
            bHasActionsPopped = true;
        }
        else
            ++itAction;
    }

    if ( bHasActionsPopped )
        RecomputeEarliestActionTime( -SyncTime() );

}

// Condition for removal from the segment chain.
bool CAkSegmentBucket::CanUnlink()
{
    AKASSERT( SyncTime() < 0 );
    return !m_pSegment && ( EarliestActionTime() < -SyncTime() );
}

void CAkSegmentBucket::DestroySegmentChainItem()
{
    AkDelete( g_DefaultPoolId, this );
}

// Commands processing.
void CAkSegmentBucket::ExecuteImminentActions(
    AkInt32 in_iSyncTime,					// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
	if ( m_pSegment )
	{
		// Play command.
		AkInt32 iPlayOffset = m_cmdPlay.iRelativeTime + in_iSyncTime;
		if ( iPlayOffset >= 0 &&
			 iPlayOffset < in_iFrameDuration )
		{
			if ( !m_cmdPlay.bIsPlaybackDisabled )
				m_pSegment->_Play( m_cmdPlay.fadeParams, iPlayOffset /*, true*/ );
			else
				m_cmdPlay.bWasPlaybackSkipped = true;
		}

		// Stop command.
		AkInt32 iStopOffset = m_cmdStop.iRelativeTime + in_iSyncTime;
		if ( iStopOffset >= 0 &&
			 iStopOffset < in_iFrameDuration )
		{
			m_pSegment->_Stop( m_cmdStop.transParams, iStopOffset );
		}
	}

    // Associated commands (stingers, states).
    AssociatedActionsArray::Iterator itActions = m_arAssociatedActions.Begin();
    while ( itActions != m_arAssociatedActions.End() )
    {
		AkInt32 iActionOffset = (*itActions).iRelativeTime + in_iSyncTime;
        if ( iActionOffset < in_iFrameDuration )
        {
            // Dequeue action for execution.
            AKASSERT( ( (*itActions).iRelativeTime + in_iSyncTime ) >= 0 ||
                        !"This action should already have been executed" );

            AkAssociatedAction & action = (*itActions);
            switch ( action.eActionType )
            {
            case AssocActionTypeStinger:
                {
                    // Stingers are always abruptly played and stopped.
                    TransParams transParams;
                    transParams.TransitionTime = 0;
					
                    if ( action.pStingerRecord->bPlaybackStarted )
                    {
                        // Playback already started: stop stinger.
                        // Note. pStingerSegment could be NULL if scheduled stinger segment was <NOTHING>.
                        if ( action.pStingerRecord->pStingerSegment )
                        {
                            AKVERIFY( action.pStingerRecord->pStingerSegment->_Stop( transParams, iActionOffset ) == AK_Success );
                            action.pStingerRecord->pStingerSegment = NULL;
                        }
                        action.pStingerRecord->bPlaybackStarted = false;
                        itActions = m_arAssociatedActions.EraseSwap( itActions );
                    }
                    else
                    {
                        // Playback has not started yet: Play stinger.
                        AKVERIFY( action.pStingerRecord->pStingerSegment->_Play( transParams, iActionOffset ) == AK_Success );
                        // Leave action there, but change its time to occur.
                        action.iRelativeTime += action.pStingerRecord->uStopRelativeTime;
                        // Mark as playing.
                        action.pStingerRecord->bPlaybackStarted = true;
                        ++itActions;
                    }
                }
                break;
            case AssocActionTypeState:
                CAkMusicRenderer::Get()->PerformDelayedStateChange( action.pStateChangeCookie );
                itActions = m_arAssociatedActions.EraseSwap( itActions );
                break;
            }
            
        }
        else
            ++itActions;
    }

    // Recompute earliest action time. Exclude actions that occurred in the past or in the current frame.
    RecomputeEarliestActionTime( in_iFrameDuration-in_iSyncTime );
}

void CAkSegmentBucket::RecomputeEarliestActionTime( 
    AkInt32 in_iMinSyncTime 
    )
{
    // Set EarliestActionTime() to the time of the latest action.
    AkInt32 iLatestActionTime = m_cmdPlay.iRelativeTime;
    if ( m_cmdStop.iRelativeTime > iLatestActionTime )
        iLatestActionTime = m_cmdStop.iRelativeTime;
    AssociatedActionsArray::Iterator it = m_arAssociatedActions.Begin();
    while ( it != m_arAssociatedActions.End() )
    {
        if ( (*it).iRelativeTime > iLatestActionTime )
            iLatestActionTime = (*it).iRelativeTime;
        ++it;
    }
    EarliestActionTime( iLatestActionTime );

    // Now, set the earliest action time to the earliest action time, excluding play/stop actions that should occur 
    // before in_iMinSyncTime (that is, that already occurred).
    if ( m_cmdPlay.iRelativeTime >= in_iMinSyncTime && 
         m_cmdPlay.iRelativeTime < EarliestActionTime() )
    {
        EarliestActionTime( m_cmdPlay.iRelativeTime );
    }
    if ( m_cmdStop.iRelativeTime >= in_iMinSyncTime && 
         m_cmdStop.iRelativeTime < EarliestActionTime() )
    {
        EarliestActionTime( m_cmdStop.iRelativeTime );
    }
    it = m_arAssociatedActions.Begin();
    while ( it != m_arAssociatedActions.End() )
    {
        if ( (*it).iRelativeTime < EarliestActionTime() )
        {
            EarliestActionTime( (*it).iRelativeTime );
        }
        ++it;
    }
}


///////////////////////////////////////////////////

CAkBranchItem::CAkBranchItem(
    AkInt32                 in_iRelativeTime,
    CAkMusicSwitchCtx *     in_pOwner,
    CAkMatrixAwareCtx *     in_pSwitchee    
    )
:CAkTimeSequencedItem( in_iRelativeTime, false )
,m_pSwitchOwner( in_pOwner )
,m_pSwitchee( in_pSwitchee )
,m_bIsLinked( false )
{
}

CAkBranchItem::~CAkBranchItem()
{
    AKASSERT( !m_bIsLinked );
}

CAkSegmentBucket * CAkBranchItem::GetNextBucket(
    CAkMatrixAwareCtx * in_pOwner,
    CAkSegmentChain *&  io_pChain,  // Chain object passed along
    AkInt32 &           out_iTimeOffset // time offset between this item and return CAkSegmentBucket (always >= 0).
    )
{
    // We switch if required owner is our owner or one of its descendent 
    // (if our owner is an ascendant of the required owner)
    if ( m_pSwitchee &&
		 ( in_pOwner == m_pSwitchOwner ||
           !IsDescendent( in_pOwner ) ) )
    {
        // Switch.
        io_pChain = m_pSwitchee->GetActiveChain();
		if ( io_pChain )
		{
			// Return the first bucket of the new chain, but the sync time of the branch item.
			out_iTimeOffset = SyncTime();
			return io_pChain->GetActiveSegment(); 
		}
		return NULL;	// End().
    }
	else
	{
		return CAkTimeSequencedItem::GetNextBucket( in_pOwner, io_pChain, out_iTimeOffset );
	}
}

bool CAkBranchItem::IsDescendent(
    CAkMusicCtx * in_pChild
    )
{
    AKASSERT( m_pSwitchOwner && in_pChild );
    do
    {
        in_pChild = in_pChild->Parent();
        if ( in_pChild == m_pSwitchOwner )
            return true;
    }
    while ( in_pChild );
    return false;
}

// Ask permission to remove from chain. Ask only after it has already synched.
bool CAkBranchItem::CanUnlink()
{
    return true;
}

void CAkBranchItem::TagAsLinked()
{ 
    m_bIsLinked = true; 
}

void CAkBranchItem::TagAsUnlinked()
{ 
    m_bIsLinked = false; 
}

// Commands processing.
void CAkBranchItem::ExecuteImminentActions(
    AkInt32 in_iSyncTime,					// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
	AKASSERT( !"Not implemented - invalid call" );
}



///////////////////////////////////////////////////

CAkMatrixSequencer::CAkMatrixSequencer(
    CAkMatrixAwareCtx * in_pOwner 
    )
:m_pOwner( in_pOwner )
,m_pGameObj( NULL )
{
}

CAkMatrixSequencer::~CAkMatrixSequencer()
{
	if ( m_pGameObj )
	{
		m_pGameObj->Release();
		m_pGameObj = NULL;
	}
}

AKRESULT CAkMatrixSequencer::Init(
    UserParams &    in_rUserparams,
    CAkRegisteredObj *  in_pGameObj
    )
{
    m_UserParams    = in_rUserparams;
    m_pGameObj      = in_pGameObj;
	m_pGameObj->AddRef();

    AKRESULT eResult = m_listPendingStingers.Init( 0, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
    // Query triggers and register to state mgr.
    RegisterTriggers();
    
    return eResult;
}

void CAkMatrixSequencer::Term()
{
    // Unregister triggers if applicable.
    UnregisterTriggers();

    AKVERIFY( m_listPendingStingers.Term() == AK_Success );
}

// Used by Renderer:
//
void CAkMatrixSequencer::Execute()
{
	m_pOwner->AddRef();

    // IsPlaying() needed because destruction propagation might have to wait for
    // the lower engine (one buffer), and the system is not built to pad the case.
    if ( m_pOwner->IsPlaying() )
    {
		if ( !m_pOwner->IsPaused() )   // do not perform behavior when (completely) paused
		{
			// Handle pending/playing Stingers.
			ConsumePlayingStingersTime();

			// Process concurrent segment chains for next audio frame.
			m_pOwner->Process( AK_NUM_VOICE_REFILL_FRAMES );
		}
    }
    else
        ClearAllPendingStingers();

	m_pOwner->Release();
}

AkInt32 CAkMatrixSequencer::GetCurSegmentPosition()
{
	AKASSERT( m_pOwner->Node()->NodeCategory() == AkNodeCategory_MusicSegment );

    CAkSegmentChain * pChain = m_pOwner->GetActiveChain();

	// WG-11783 - Padding for the fact that GetActiveChain() can return NULL in some
	// cases. This kind of thing caused crashes for some customers, but we did not
	// repro these crashes internally so this may not be enough to fix the problems...
	if ( !pChain )
	{
		return 0;
	}

    CAkSegmentBucket * pBucket = static_cast<CAkSegmentBucket*>(pChain->First());

	if( !pBucket )
	{
		return 0;
	}

    Iterator it;
    it.pChain = pChain;
    it.pBucket = pBucket;
	CAkSegmentCtx * pCtx = pBucket->SegmentCtx();
    AkInt32 iDummy;

	// Find first bucket whose context is not NULL.
	while ( !pCtx )
    {
        it.NextBucket( m_pOwner, iDummy );
		if ( it != End() )
		{
			pCtx = (*it)->SegmentCtx();
		}
		else
		{
			// We reached the last bucket of the chain, and we did not find any segment:
			// there must be a stinger that is playing, that refrains us from stopping.
			// Return the end of the segment.
			return static_cast<CAkMusicSegment*>(m_pOwner->Node())->Duration();
		}
    }
    
    AKASSERT( pCtx );

	return pCtx->Position() + pCtx->SegmentNode()->PreEntryDuration();
}

CAkMatrixSequencer::Iterator CAkMatrixSequencer::GetCurBucket(
    CAkMatrixAwareCtx * in_pOwnerCtx    // Owner context determines path to follow.
    )
{
    // Get current chain.
    CAkSegmentChain * pChain = in_pOwnerCtx->GetActiveChain();
	
    // Create and initialize a matrix iterator.
    Iterator it;
    it.pChain = pChain;
	it.pBucket = ( pChain ) ? pChain->GetActiveSegment() : NULL;
    return it;
}


////////////////////////////////////////////////////////////


// Active chain listener implementation:
// ----------------------------------------
void CAkMatrixSequencer::OnPathChange(
    CAkSegmentBucket * in_pCurBucket,       // Current bucket.
    CAkSegmentBucket * in_pOldNextBucket,   // Previously expected next bucket.
    AkInt32            in_iTimeOffset       // Time offset between current and new next bucket.
    )
{
    // For all actions scheduled to occur in old next bucket, and later in current bucket, 
    // Flush, get target, recompute.
    AKASSERT( in_pCurBucket );

    // Get actions to be rescheduled.
    AssociatedActionsArray arActionsToReschedule;

    // Current bucket: cancel all actions whose time will occur after sync of new next segment.
	in_pCurBucket->PopAssociatedActionsToReschedule( arActionsToReschedule, 
    												 -in_pCurBucket->ConvertToAbsoluteTime( -in_iTimeOffset ) );

    // Old next bucket: cancel ALL actions, including those that should occur "now".
    if ( in_pOldNextBucket )
        in_pOldNextBucket->PopAssociatedActionsToReschedule( arActionsToReschedule, 
                                                             -(in_pOldNextBucket->SyncTime()+1) );

    // Process cancelled actions list.
    AssociatedActionsArray::Iterator itAction = arActionsToReschedule.Begin();
    while ( itAction != arActionsToReschedule.End() )
    {
        AkAssociatedAction & action = (*itAction);
        switch ( action.eActionType )
        {
        case AssocActionTypeStinger:
            // Recovered stingers should not have already started playing.
            AKASSERT( !action.pStingerRecord->bPlaybackStarted );
            
            // reschedule if they were scheduled in first bucket and they have look-ahead property 
            // (record::bCanBeRescheduled is true).
            // destroy all the others and log.

            {
                AkTriggerID triggerID = action.pStingerRecord->triggerID;
				bool bDoReschedule = action.pStingerRecord->bCanBeRescheduled;
                ClearStingerRecord( action.pStingerRecord );

                if ( bDoReschedule )
                {   
                    HandleTrigger( triggerID, true );
                }
                else
                {
                    MONITOR_ERROR( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
                }
            }
            break;
        case AssocActionTypeState:
            // Reschedule State change through Music Renderer ("last chance only").
            CAkMusicRenderer::Get()->RescheduleDelayedStateChange( action.pStateChangeCookie );
            break;

        default:
            AKASSERT( !"Unhandled associated action type" );
        }
        ++itAction;
    }

    arActionsToReschedule.Term();
}


// IAkTriggerAware implementation:
// ----------------------------------------
void CAkMatrixSequencer::Trigger( 
    AkTriggerID in_triggerID 
    )
{
    HandleTrigger( in_triggerID, false );
}


// Stingers management.
// ---------------------------------------

// Handle trigger event.
void CAkMatrixSequencer::HandleTrigger( 
    AkTriggerID in_triggerID, 
    bool in_bReschedule
    )
{
    // Get stingers list from active segment.
    Iterator it = GetCurBucket( m_pOwner );
    if ( it == End() ) return;	// Triggered while top-level context is being destroyed.
    AkInt32 iCumulSrcLookAheadTime = 0;
    AkUInt32 uLookAheadIdx = 0; // look ahead index in segment chain.

    // Get next segment, and its sync time.
    AkInt32 iTimeBeforeNextSegment;
    Iterator itNext = it;
    itNext.NextBucket( m_pOwner, iTimeBeforeNextSegment );

	// Get next segment if this trigger must be rescheduled.
	if ( in_bReschedule )
	{
		// Get next segment.
		if ( itNext != End() )
		{
			it = itNext;
			iCumulSrcLookAheadTime = iTimeBeforeNextSegment;
			itNext.NextBucket( m_pOwner, iTimeBeforeNextSegment );
            
			++uLookAheadIdx;
		}
		else
		{
			// Log
			MONITOR_ERROR( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
			return;
		}
	}

    // Inspect buckets.
    CAkStinger * pStingerData;
    CAkSegmentCtx * pStingerCtx;
    AKRESULT eSyncFoundResult = AK_Fail;

    do
    {
        pStingerData = NULL;
        pStingerCtx = NULL;

        CAkMusicNode * pNode = GetNodeFromBucket( it );
        AKASSERT( pNode ); 

        // Find THE stinger for that trigger.
        CAkMusicNode::CAkStingers stingers;
        pNode->GetStingers( &stingers );
        CAkMusicNode::StingerArray & arStingers = stingers.GetStingerArray();
        CAkMusicNode::StingerArray::Iterator itStinger = arStingers.Begin();
        while ( itStinger != arStingers.End() )
        {
            // Check match, and inhibation.
            if ( (*itStinger).TriggerID() == in_triggerID &&
                CanPlayStinger( &(*itStinger) ) )
            {
                pStingerData = &(*itStinger);
                break;
            }
            // FUNC (LX) LOG if cannot play stinger (because of DontRepeatTime)?
            ++itStinger;
        }
        
        
        // If we found stinger data that matches the input trigger, and that this trigger can be played.
        if ( pStingerData )
        {
            // Found a stinger that could be played. Try to schedule it.
                    
            // Create stinger-segment context and prepare.
            AkInt32 iRequiredDestLookAhead;
            pStingerCtx = CreateStingerSegmentAndAddRef( pStingerData->SegmentID(), iRequiredDestLookAhead );

			if ( !pStingerCtx && 
                 pStingerData->SegmentID() != AK_INVALID_UNIQUE_ID )
            {
                // Failed creating non-<nothing> stinger segment context.
                // Almost certain it will fail on next attempt so let's leave now.
                stingers.Term();
                return;
            }

            // Try to schedule it now. 
            AkInt32 iSyncPos;
            
			// Sync time constraint is the required destination look-ahead minus the cumulative time of 
			// previous segments inspected.
			AkInt32 iSyncTimeConstraint = iRequiredDestLookAhead - iCumulSrcLookAheadTime;

			eSyncFoundResult = (*it)->FindSyncPosition( 
				m_pOwner,					// Owner for branching decision.
				it.pChain,					// Chain.
				iSyncTimeConstraint,		// Time constraint. 
				0,							// No constraint on segment position.
				pStingerData->SyncPlayAt(),	// Sync rule.
				false,						// false = allow sync at entry marker.
				false,						// false = fail if !Immediate rule on <nothing>.
				iSyncPos					// Returned Sync position, relative to bucket's time.
				);
        
			/** NOTE: This feature was disabled in 2007.1
            if ( eSyncFoundResult == AK_Success )
            {
                // Verify that desired sync position occurs before next segment becomes active.
                AKASSERT( iTimeBeforeNextSegment >= 0 );

                AkInt32 iSyncOffset = iSyncPos + (*it)->SyncTime();
                AKASSERT( iSyncOffset <= ( iCumulSrcLookAheadTime + iTimeBeforeNextSegment ) );
				
				// We need to also evaluate if next segment's stinger will fall on Entry point.
                else if ( iSyncOffset == iTimeBeforeNextSegment && 
                          uLookAheadIdx == 0 && 
                          pStingerData->m_numSegmentLookAhead == 1 )
                {
                    // Computed sync offset will occur exactly when next segment will become active.
                    // If we are not looking ahead, and the stinger has look-ahead property, we want instead to 
                    // schedule it in the next segment, because a more appropriate stinger could be chosen.
                    eSyncFoundResult = AK_Fail;
                }
                // otherwise we are OK to proceed.
            }
			**/

            if ( eSyncFoundResult == AK_Success )
            {
                // Stinger can start playing within this segment.

                AKRESULT eResult = ScheduleStingerForPlayback(
                    (*it),  //pInspectedBucket,
                    pStingerCtx,
                    pStingerData,
                    iSyncPos,
                    iRequiredDestLookAhead,
                    ( uLookAheadIdx == 0 ) );

				// Release addref'ed stinger context once it has been scheduled (that is, attached by a smart pointer 
				// in a stinger record).
				// If scheduling failed, then refcount=1 and destruction will occur here.
				if ( pStingerCtx )
					pStingerCtx->Release();

                if ( eResult != AK_Success )
                {
                    // Failed scheduling stinger. Almost certain it will fail on next attempt so let's leave now.
                    // Destroy created context if applicable.
                    stingers.Term();
                    return;
                }
            }
            else
            {
                // Stinger cannot/shouldn't start playing within this segment.
                // Either prepare to inspect next segment (if it exists), or leave and log dropped stinger.

				if ( pStingerCtx )
					pStingerCtx->Release();

                if ( itNext != End() && 
					 pStingerData->m_numSegmentLookAhead == 1 &&
					 uLookAheadIdx == 0 )
                {
                    // Stinger cannot start playing in this segment, but it has LookAhead property:
                    // Check in next segment.

                    // Get next segment.
                    it = itNext;
                    iCumulSrcLookAheadTime = iTimeBeforeNextSegment;

                    // Get the subsequent segment.
                    itNext.NextBucket( m_pOwner, iTimeBeforeNextSegment );
                    
                    ++uLookAheadIdx;
                }
                else
                {
                    // Cannot be scheduled.
					// Log
                    MONITOR_ERROR( AK::Monitor::ErrorCode_StingerCouldNotBeScheduled );
                    
                    // Set sync result to AK_Success to break out.
                    eSyncFoundResult = AK_Success;
                }
            }
        }
        stingers.Term();
    }
    while ( eSyncFoundResult != AK_Success && pStingerData );
}

// Trigger registration.
void CAkMatrixSequencer::RegisterTriggers()
{
    if ( g_pStateMgr->RegisterTrigger( this, m_pGameObj ) != AK_Success )
    {
        // TODO (LX) Log Cannot register to triggers.
    }
}

void CAkMatrixSequencer::UnregisterTriggers()
{
    ClearAllPendingStingers();

    // Unregister all triggers associated with 'this' TriggerAware.
    g_pStateMgr->UnregisterTrigger( this );
}

// Get first music node from which we can get some stingers.
// Usually it will be the segment node associated with the segment context of the inspected bucket.
// However, if it is an empty bucket, we use the node associated with the segment chain.
CAkMusicNode * CAkMatrixSequencer::GetNodeFromBucket( 
    const CAkMatrixSequencer::Iterator & in_iter
    )
{
    CAkMusicNode * pNode;
    if ( (*in_iter)->SegmentCtx() )
        pNode = (*in_iter)->SegmentCtx()->SegmentNode();
    else
    {
        CAkMatrixAwareCtx * pParentCtx = in_iter.pChain->MatrixAwareCtx();
        AKASSERT( pParentCtx );
        pNode = pParentCtx->Node();
        if ( !pNode )
        {
            // If the chain is a null chain, get the node of its parent (must be a SwitchCtx, must succeed).
            pParentCtx = static_cast<CAkMatrixAwareCtx*>( pParentCtx->Parent() );
            AKASSERT( pParentCtx );
            pNode = pParentCtx->Node();
        }
    }
    AKASSERT( pNode );
    return pNode;
}        

// Schedule stinger playback.
AKRESULT CAkMatrixSequencer::ScheduleStingerForPlayback(
    CAkSegmentBucket *  in_pHostBucket,
    CAkSegmentCtx *     in_pStingerCtx,     // Can be NULL (<NOTHING>).
    const CAkStinger *  in_pStingerData,
    AkInt32             in_iSyncTime,
    AkInt32             in_iLookAheadDuration,
    bool                in_bScheduledInCurrentSegment
    )
{
    AKASSERT( in_pHostBucket );

    AKASSERT( in_iLookAheadDuration >= 0 );

    // Keep stinger record.
    AkStingerRecord * pStingerRecord = EnqueueStingerRecord( in_pStingerData, 
                                                             in_pStingerCtx, 
                                                             in_iLookAheadDuration,
                                                             in_bScheduledInCurrentSegment );

    if ( pStingerRecord )
    {
        // Attach stinger action to active segment.
        AkAssociatedAction action;

        // Play.
        action.eActionType = AssocActionTypeStinger;
        action.iRelativeTime = in_iSyncTime - in_iLookAheadDuration;
        action.pStingerRecord = pStingerRecord;

        if ( in_pHostBucket->AttachAssociatedAction( action ) == AK_Success )
        {
            return AK_Success;
        }
        else
        {
            ClearStingerRecord( pStingerRecord );            
        }
    }
    return AK_Fail;
}

// Create a stinger-segment context.
CAkSegmentCtx * CAkMatrixSequencer::CreateStingerSegmentAndAddRef(
    AkUniqueID in_segmentID,
    AkInt32 & out_iLookAheadDuration
    )
{
    CAkMusicSegment * pStingerSegment = static_cast<CAkMusicSegment*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_segmentID ));
    CAkSegmentCtx * pStingerCtx = NULL;
    
    if ( pStingerSegment )
    {
        // Create a context, child of top-level context (owner).
        pStingerCtx = pStingerSegment->CreateSegmentCtxAndAddRef( m_pOwner, m_pGameObj, m_UserParams );
        if ( pStingerCtx )
        {
            out_iLookAheadDuration = pStingerCtx->Prepare( 0, true );
        }
        pStingerSegment->Release();
    }
    else
        out_iLookAheadDuration = 0;
    return pStingerCtx;
}

AkStingerRecord * CAkMatrixSequencer::EnqueueStingerRecord( 
    const CAkStinger *  in_pStingerData,
    CAkSegmentCtx *     in_pStingerCtx,
    AkInt32             in_iLookAheadDuration,
    bool                in_bScheduledInCurrentSegment
    )
{
    AKASSERT( in_pStingerData );

    // Stinger should not already exist in list if previous DontRepeatTime is positive.
    AKASSERT( CanPlayStinger( in_pStingerData ) );

    AkStingerRecord * pStingerRecord = m_listPendingStingers.AddLast();
    if ( pStingerRecord )
    {
        pStingerRecord->triggerID           = in_pStingerData->TriggerID();
        pStingerRecord->segmentID           = in_pStingerData->SegmentID();
        pStingerRecord->iDontRepeatTime     = in_pStingerData->DontRepeatTime();

        if ( in_pStingerCtx )
        {
            pStingerRecord->uStopRelativeTime   = in_iLookAheadDuration 
                + in_pStingerCtx->SegmentNode()->ActiveDuration() 
                + in_pStingerCtx->SegmentNode()->PostExitDuration();
            pStingerRecord->bPlaybackStarted    = false;
        }
        else
        {
            pStingerRecord->uStopRelativeTime   = AK_INFINITE_SYNC_TIME_OFFSET;
            pStingerRecord->bPlaybackStarted    = true;
        }
        pStingerRecord->pStingerSegment     = in_pStingerCtx;
        
        // Can be rescheduled if stinger has look-ahead property, and was scheduled in current segment.
        pStingerRecord->bCanBeRescheduled   = in_bScheduledInCurrentSegment && ( in_pStingerData->m_numSegmentLookAhead > 0 );

        return pStingerRecord;
    }
    return NULL;
}

// Can play stinger. False if another stinger is there, with positive don't repeat time.
bool CAkMatrixSequencer::CanPlayStinger( 
    const CAkStinger *  in_pStingerData
    )
{
    AKASSERT( in_pStingerData );

    PendingStingersList::Iterator it = m_listPendingStingers.Begin();
    while ( it != m_listPendingStingers.End() )
    {
        // "Trigger" is inhibated by DontRepeatTime.
        if ( (*it).triggerID == in_pStingerData->TriggerID() &&
             //(*it).segmentID == in_stingerData.SegmentID() &&
             (*it).iDontRepeatTime > 0 )
        {
            // Cannot play this stinger.
            return false;
        }
        ++it;
    }
    return true;
}

// Consumes don't repeat time of all playing stingers.
// Cleans up pending stingers.
void CAkMatrixSequencer::ConsumePlayingStingersTime()
{
    PendingStingersList::IteratorEx it = m_listPendingStingers.BeginEx();
    while ( it != m_listPendingStingers.End() )
    {
        AkStingerRecord & record = (*it);
        record.iDontRepeatTime -= AK_NUM_VOICE_REFILL_FRAMES;

		// Power up segment if it exists.
		if ( record.pStingerSegment )
		{
			if ( record.pStingerSegment->IsPlaying() )
			{
				AKASSERT( !record.pStingerSegment->IsPaused() );
				record.pStingerSegment->PerformNextFrameBehavior( 0, AK_NUM_VOICE_REFILL_FRAMES );
			}
			++it;
		}
		else if ( !record.bPlaybackStarted &&
				  record.iDontRepeatTime < 0 )
        {
            it = m_listPendingStingers.Erase( it );
        }
        else 
            ++it;
    }
}

void CAkMatrixSequencer::ClearAllPendingStingers()
{
    PendingStingersList::Iterator it = m_listPendingStingers.Begin();
    while ( it != m_listPendingStingers.End() )
    {
        if ( (*it).pStingerSegment )
        {
            (*it).pStingerSegment->OnStopped( 0 );
            (*it).pStingerSegment = NULL;
        }
        ++it;
    }
    m_listPendingStingers.RemoveAll();
}

void CAkMatrixSequencer::ClearStingerRecord( 
    const AkStingerRecord * in_pStingerRecord
    )
{
    AKASSERT( !in_pStingerRecord->bPlaybackStarted );

    PendingStingersList::IteratorEx it = m_listPendingStingers.BeginEx();
    while ( it != m_listPendingStingers.End() )
    {
        if ( &(*it) == in_pStingerRecord )
        {
            (*it).pStingerSegment = NULL;
            m_listPendingStingers.Erase( it );
            break;
        }
        ++it;
    }
}


// Delayed states management:
// ----------------------------------------

// See if someone in the hierarchy of the current segment (or the next segment if it playing) 
// is registered to in_stateGroupID, and if it is not immediate.
// Returns the absolute number of samples in which state change should be effective.
// Also, returns 
// the segment look-ahead index which responded to the state group 
// (0=current segment, 1=next segment, etc.);
// the sync time relative to this segment.
AkInt32 CAkMatrixSequencer::QueryStateChangeDelay( 
    AkStateGroupID  in_stateGroupID,
    AkUInt32 &      out_uSegmentLookAhead,  // returned segment look-ahead index which responded to the state group.
    AkInt32 &       out_iRelativeSyncTime   // returned sync time relative to this segment.
    )
{
    // Ask node of current segment if it exists.
    Iterator it = GetCurBucket( m_pOwner );
    if ( it == End() ) return 0;	// Top-level context is being destroyed: change state immediately.
    CAkSegmentCtx * pCtx = (*it)->SegmentCtx();

    AkDelayedStateHandlingResult eFirstSegmentResult = AK_DelayedStateNotHandled;
	out_uSegmentLookAhead = 0;

    if ( pCtx &&
		 pCtx->IsPlaying() )
    {
        // There is a segment context. Get the earliest state change position, given the context's
        // current position. 
        eFirstSegmentResult = GetEarliestStateSyncTime( it, 
                                                        in_stateGroupID, 
                                                        out_iRelativeSyncTime );

        if ( eFirstSegmentResult == AK_DelayedStateImmediate )
        {
            return 0;   // Return 0. Will be considered as immediate by Renderer.
        }
        else if ( eFirstSegmentResult == AK_DelayedStateSyncFound )
        {
            // Return absolute delay.
            return out_iRelativeSyncTime + (*it)->SyncTime();
        }
        /** else
            Sync not found or StateGroup not handled: Check next segment.
        **/
    }   

	// Still here. Ask next segment if it exists, AND 
    // if it is playing OR first segment was registered to this state group.
    AkInt32 iNextBucketTimeToSync;
    Iterator itNext = it;
    itNext.NextBucket( m_pOwner, iNextBucketTimeToSync );
	if ( itNext != End() )
	{
		
		CAkSegmentCtx * pNextCtx = (*itNext)->SegmentCtx();
		if ( !pNextCtx )
		{
			// Next segment is <nothing>. Will sound best at sync point.
			// Note: attach state change action on current - previous - bucket (leave out_uSegmentLookAhead=0),
			// if it is playing. (State change action cannot be attached to empty buckets).
			if ( pCtx &&
				 pCtx->IsPlaying() )
			{
				out_iRelativeSyncTime = iNextBucketTimeToSync - (*it)->SyncTime();
				return iNextBucketTimeToSync;
			}
			return 0;
		}
	    
		if ( eFirstSegmentResult != AK_DelayedStateNotHandled || // 1st segment registered to state group.
			 pNextCtx->IsPlaying() )
		{
			// Inspecting next segment. Last chance to handle it: we do not look-ahead further.
			out_uSegmentLookAhead = 1;

			AkInt32 iAbsoluteDelay;
			AkDelayedStateHandlingResult eHandlingResult = GetEarliestStateSyncTime( itNext, 
																					 in_stateGroupID, 
																					 out_iRelativeSyncTime );
			switch ( eHandlingResult )
			{
			case AK_DelayedStateImmediate:
			case AK_DelayedStateNotHandled:
				if ( eFirstSegmentResult == AK_DelayedStateNotHandled )
				{
					// If first segment does not handle this state group, change immediately.
					iAbsoluteDelay = 0;
				}
				else
				{
					// Otherwise, it will sound best at sync.
					AKASSERT( eFirstSegmentResult == AK_DelayedStateCannotSync );
					out_iRelativeSyncTime = 0;
					iAbsoluteDelay = iNextBucketTimeToSync;
				}
				break;

			case AK_DelayedStateSyncFound:
				// Return absolute delay.
				// Convert sync time relative to iterator.
				iAbsoluteDelay = out_iRelativeSyncTime + iNextBucketTimeToSync;
				break;

			case AK_DelayedStateCannotSync:
				//if ( eFirstSegmentTriggerResult == AK_TriggerNotHandled )
					// If first segment does not handle this state group, change immediately.

					// Otherwise, too bad, also change immediately...
				iAbsoluteDelay = 0;
				break;

			default:
				AKASSERT( !"Invalid delayed state change handling return value" );
				iAbsoluteDelay = 0;
			}

			return iAbsoluteDelay;
		}
	}    
    
    // First segment did not answer, and next segment was not asked. Change immmediately.
    return 0;
}

// Handle delayed state change. Compute and schedule state change in sequencer.
AKRESULT CAkMatrixSequencer::ProcessDelayedStateChange(
    void *          in_pCookie,
    AkUInt32        in_uSegmentLookAhead,   // handling segment index (>=0).
    AkInt32         in_iRelativeSyncTime    // sync time relative to this segment.
    )
{
    // Get sequence item.
    Iterator it = GetCurBucket( m_pOwner );
    if ( it == End() ) return AK_Fail;
    
    AkInt32 iDummy;
    while ( in_uSegmentLookAhead > 0 )
    {
        it.NextBucket( m_pOwner, iDummy  );
        --in_uSegmentLookAhead;
    }

    // There must be a segment context attached to this bucket, otherwise we wouldn't have required a delayed state change.
    AKASSERT( (*it)->SegmentCtx() );

    // Create action.
    AkAssociatedAction stateChangeAction;
    stateChangeAction.eActionType = AssocActionTypeState;
    stateChangeAction.iRelativeTime = in_iRelativeSyncTime;
    stateChangeAction.pStateChangeCookie = in_pCookie;
    
    return (*it)->AttachAssociatedAction( stateChangeAction );
}

// Helpers for state management.

// If at least one node of the segment's hierarchy is registered to the state group,
// - returns AK_TriggerImmediate if it requires an immediate state change,
//  if it also requires a delayed state processing, 
//  - returns AK_TriggerSyncFound if it was able to schedule it,
//  - returns AK_TriggerCannotSync if it was not.
// Otherwise, 
// - returns AK_TriggerNotHandled.
// Returned delay is relative to now (segment's position).
CAkMatrixSequencer::AkDelayedStateHandlingResult CAkMatrixSequencer::GetEarliestStateSyncTime( 
    const CAkMatrixSequencer::Iterator & in_itBucket, 
    AkStateGroupID  in_stateGroupID, 
    AkInt32 &       out_iRelativeSyncTime	// Returned sync time relative to in_pCtx's entry cue.
    )
{
    out_iRelativeSyncTime = 0;

    // Ask this context's segment node for all its hierarchy's state change syncs for that state group.
    AkDelayedStateHandlingResult eHandlingResult;
    CAkParameterNodeBase::CAkStateSyncArray stateSyncs;
	AKASSERT( in_itBucket.SegmentNode() );
    in_itBucket.SegmentNode()->GetStateSyncTypes( in_stateGroupID, &stateSyncs );
    
    CAkParameterNodeBase::StateSyncArray & arStateSyncs = stateSyncs.GetStateSyncArray();
    
    if ( arStateSyncs.Length() > 0 )
    {
        // Array has syncs. At least someone is registered to this state group.

        // Now that we gathered the state syncs, we need to compute the earliest time to sync.
        CAkMusicNode::StateSyncArray::Iterator it = arStateSyncs.Begin();

        // Hierarchy is registered to that state group. 
        if ( (*it) == SyncTypeImmediate ) 
        {
            AKASSERT( arStateSyncs.Length() == 1 );
            // Immediate: leave now.
            eHandlingResult = AK_DelayedStateImmediate;
        }
        else
        {
             // Delayed processing is required.
            eHandlingResult = AK_DelayedStateCannotSync;

            // Find earliest sync position.
            while ( it != arStateSyncs.End() )
            {
                AKASSERT( (*it) != SyncTypeImmediate );                

                AkInt32 iSyncTime;
                if ( (*in_itBucket)->FindSyncPosition( 
						m_pOwner,
						in_itBucket.pChain,
						0,		// no time constraint.
						0,		// no constraint on segment position.
						(*it), 
						false,	// false = allow sync at entry marker.
						false,	// false = fail if !Immediate rule on <nothing>.
						iSyncTime ) == AK_Success && 
                    ( eHandlingResult == AK_DelayedStateCannotSync ||
                    iSyncTime < out_iRelativeSyncTime ) )
                {
                    out_iRelativeSyncTime = iSyncTime;
                    eHandlingResult = AK_DelayedStateSyncFound;
                }

                ++it;
            }
        }

    }  
    else
    {
        // Hierarchy is not registered to that state group. 
        eHandlingResult = AK_DelayedStateNotHandled;
    }

    stateSyncs.Term();

    return eHandlingResult;
}


