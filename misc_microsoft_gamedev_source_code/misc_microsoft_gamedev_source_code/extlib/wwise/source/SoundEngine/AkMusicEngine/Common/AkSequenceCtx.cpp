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
// AkSequenceCtx.cpp
//
// Sequence context.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkSequenceCtx.h"
#include "AkMusicRanSeqCntr.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkMusicStructs.h"
#include "AkMusicRenderer.h"
#include "AkMatrixSequencer.h"
#include "AkMonitor.h"

#define MINIMAL_BUCKET_LOOK_AHEAD	(2)

CAkSequenceCtx::CAkSequenceCtx(
    CAkMusicRanSeqCntr *in_pSequenceNode,
    CAkMusicCtx *       in_pParentCtx
    )
:CAkMatrixAwareCtx( in_pParentCtx )
,m_pSequenceNode( in_pSequenceNode )
,m_PlayListIterator( in_pSequenceNode )
{
	if( m_pSequenceNode )
		m_pSequenceNode->AddRef();
}

CAkSequenceCtx::~CAkSequenceCtx()
{
    CAkSegmentChain::Term();
	if( m_pSequenceNode )
		m_pSequenceNode->Release();
}


// Initializes the context. Automatically performs a one-segment look-ahead.
// Returns first segment that was scheduled in the sequencer.
AKRESULT CAkSequenceCtx::Init(
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams,
    CAkSegmentBucket *& out_pFirstBucket    // First bucket.
    )
{
    CAkSegmentChain::Init();
    AKRESULT eResult = CAkMatrixAwareCtx::Init( in_GameObject, in_rUserparams );
    if ( eResult == AK_Success )
    {
        eResult = m_PlayListIterator.Init();
		if ( eResult == AK_Success )
		{
			// Print playlist items deterministically into scheduler until we have reached the requested look-ahead.
            out_pFirstBucket = ScheduleNextSegment();

			// Protect caller if there was an error enqueueing the first segment.
			if ( out_pFirstBucket && 
				 out_pFirstBucket->SegmentCtx() )
			{
				InitActiveSegment( out_pFirstBucket );
				UpdateChainLength( Begin(), 0 );
				if ( !m_pParentCtx )
					eResult = SetAsTopLevel();
			}
			else
			{
				eResult = AK_Fail;
			}
        }
    }

    return eResult;
}

// Called by parent (switches): completes the initialization.
// Sequences set their iterator ready.
void CAkSequenceCtx::EndInit()
{
	m_PlayListIterator.EndInit();
}


// Sequencer access.
void CAkSequenceCtx::Process(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
    AddRef();
    ProcessChainItems( in_iFrameDuration );
    Release();
}
void CAkSequenceCtx::ProcessRelative(
	AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
    ProcessRelativeChainItems( in_iSyncTime,	in_iFrameDuration );
}

CAkSegmentBucket * CAkSequenceCtx::ForceGrow(
    AkInt32 & out_iTimeOffset
    )
{
	// Grow if the end of the sequence was not reached yet, and if the last bucket is not
	// inhibated (a sequence cannot grow while it is "inhibated").
    CAkSegmentBucket * pNewBucket = NULL;
	out_iTimeOffset = AK_INFINITE_SYNC_TIME_OFFSET;
	AKASSERT( !IsEmpty() && LastSegmentBucket() );

    if ( m_PlayListIterator.IsValid() )
    {
		CAkSegmentBucket * pLastBucket = LastSegmentBucket();
		pNewBucket = ScheduleNextSegment();

		// The bucket returned could be the same as the previous LsatBucket
		// if there was a fatal error (that is, no new bucket was created).
		if ( pNewBucket != pLastBucket )
		{
			// They are different: there was no error.
			AKASSERT( pNewBucket );
			out_iTimeOffset = pNewBucket->SyncTime();
		}
		else
		{
			// There was an error.
			pNewBucket = NULL;
		}
    }
    
    return pNewBucket;
}

// Grows chain so that it has at least MINIMAL_BUCKET_LOOK_AHEAD buckets ahead of the current 
// item (in_curItem). If the current bucket's position is greater than 0, chain is grown so that it has
// at least MINIMAL_BUCKET_LOOK_AHEAD+1 buckets ahead of the current item.
void CAkSequenceCtx::UpdateChainLength(
	const CAkSegmentChain::Iterator & in_curItem,	// Current item.
	AkInt32 in_iCurItemTime		// Current item time.
	)
{
	AkUInt32 uBucketLookAhead = 0;
	CAkSegmentChain::Iterator it = in_curItem;
	const AkUInt32 uRequiredBucketLookAhead = MINIMAL_BUCKET_LOOK_AHEAD + ( ( in_iCurItemTime > 0 ) ? 1 : 0 );
	while ( uBucketLookAhead < uRequiredBucketLookAhead &&
			it != End() )
	{
		if ( (*it)->IsSegment() )
		{
			if ( (*it) == LastSegmentBucket() )
			{
				AkInt32 iDummy;
				if ( !ForceGrow( iDummy ) )
					return;
			}
			
			++uBucketLookAhead;
		}
		++it;
	}
}

CAkMatrixAwareCtx * CAkSequenceCtx::MatrixAwareCtx()
{
    return this;
}

CAkSegmentChain * CAkSequenceCtx::GetActiveChain()
{
    return this;
}

bool CAkSequenceCtx::CanRestartPlaying()
{
	return CanRevertDisabledPlaybacks();
}

// Revert playback commands that were disabled. Only possible if it can restart playing.
// Default implementation does nothing.
void CAkSequenceCtx::RestartPlaying()
{
	AKASSERT( CanRevertDisabledPlaybacks() );
	RevertDisabledPlaybacks();
}


// Context commands
//

// Override MusicCtx OnStop: Clear chain now to unlink any ascendent branch items.
void CAkSequenceCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
    // Add ref ourselves locally in case child stopping destroys us.
    AddRef();

    CAkMusicCtx::OnStopped( in_uSubFrameOffset );

	ClearChain();

    Release();
}

// For Music Renderer's music contexts look-up.
CAkMusicNode * CAkSequenceCtx::Node()
{
    return m_pSequenceNode;
}

// Initializes playlist iterator so that it points to the top-level node of the playlist that corresponds
// to the in_uJumpToIdx. Flushes sequencer look-ahead if necessary.
// Can only be called before the sequence starts playing.
CAkSegmentBucket * CAkSequenceCtx::JumpToSegment(
    AkUniqueID in_uJumpToID
    )
{
    AKASSERT( !IsPlaying() );

    // NOTE. in_uJumpToID is 0 when not set in UI. 
    if ( in_uJumpToID == 0 )
    {
        AKASSERT( First() && First()->IsSegment() );
        return static_cast<CAkSegmentBucket*>( First() );
    }
    else
    {
        // Flushing first segment would result in self-destruction.
        AddRef();

        // Flush segment(s) previously scheduled.
        FlushAllBuckets();

        // Prepare iterator.
	    m_PlayListIterator.JumpTo( in_uJumpToID );

        CAkSegmentBucket * pNewFirstBucket = ScheduleNextSegment();
        AKASSERT( pNewFirstBucket );
		InitActiveSegment( pNewFirstBucket );

        Release();

        return pNewFirstBucket;
    }
}

// Returns the first new bucket printed in the scheduler.
CAkSegmentBucket * CAkSequenceCtx::ScheduleNextSegment()
{
    // Get next segment to be played from Playlist.

    AkUniqueID nextID = *m_PlayListIterator;
#ifndef AK_OPTIMIZED
	AkUniqueID playlistItemID = m_PlayListIterator.GetPlaylistItem();
#else
    AkUniqueID playlistItemID = AK_INVALID_UNIQUE_ID;
#endif
	
	if( !m_PlayListIterator.IsValid() )// reached end of PlayList
		return NULL;

	++m_PlayListIterator;


    // Get source context.
    CAkSegmentBucket * pSrcBucket = LastSegmentBucket();
    
    // This method should not have been called after we reached the end of the playlist.
    // Note. The following algorithm does not support <NONE> to <NONE> playlist transitions.
    AKASSERT( ( pSrcBucket && pSrcBucket->SegmentCtx() ) || nextID != AK_MUSIC_TRANSITION_RULE_ID_NONE );
    AKASSERT( !pSrcBucket || pSrcBucket->SegmentCtx() ); // if there is a bucket, there must be a segment in it.

    // Query transition rule.
    // (Handle <NONE>)
    AkUniqueID curID = ( pSrcBucket ) ? pSrcBucket->SegmentCtx()->SegmentNode()->ID() : AK_MUSIC_TRANSITION_RULE_ID_NONE;
    const AkMusicTransitionRule & rule = m_pSequenceNode->GetTransitionRule( curID, nextID );

    if ( rule.pTransObj )
    {
        // There is a transition segment. Schedule it, and then schedule the "real" next one.
        // Create a new rule, using the original source rule and the Entry parameters of the transition segment.
        AkMusicTransitionRule transRule;
        transRule.srcRule = rule.srcRule;
        // Not used. transRule.srcID
        transRule.pTransObj = NULL;
        ///
        transRule.destRule.fadeParams = rule.pTransObj->fadeInParams;
        transRule.destRule.markerID = AK_INVALID_UNIQUE_ID;
        transRule.destRule.eEntryType = EntryTypeEntryMarker;
        transRule.destRule.bPlayPreEntry = rule.pTransObj->bPlayPreEntry;
        // Not used. transRule.destID
        ///
        pSrcBucket = AppendBucket( transRule, pSrcBucket, rule.pTransObj->segmentID, AK_INVALID_UNIQUE_ID );
		// Being unable to append an empty bucket is a fatal error.
        if ( pSrcBucket )
		{
			// However, for a transition segment, enqueueuing a transition bucket that is empty is also
			// a fatal error.
			if ( !pSrcBucket->SegmentCtx() )
			{
				return HandleFatalError();
			}

			// Now, append the "real" next bucket.
			// Compute a rule using the original destination rule, and the Exit parameters of the transition segment.
			///
			transRule.srcRule.fadeParams = rule.pTransObj->fadeOutParams;
			transRule.srcRule.eSyncType = SyncTypeExitMarker;
			transRule.srcRule.bPlayPostExit = rule.pTransObj->bPlayPostExit;
			// Not used. transRule.srcID = rule.srcID;
			///
			transRule.destRule = rule.destRule;
			// Not used. transRule.destID

			AppendBucket( transRule, pSrcBucket, nextID, playlistItemID );
		}

        // Note that we return the first bucket that was scheduled, not the last one.
        return pSrcBucket;
    }
    else
        return AppendBucket( rule, pSrcBucket, nextID, playlistItemID );
}

// Appends a bucket to the chain following a music transition rule.
CAkSegmentBucket * CAkSequenceCtx::AppendBucket( 
    const AkMusicTransitionRule & in_rule,  // Transition rule between source bucket (in_pSrcBucket) and next one.
    CAkSegmentBucket * in_pSrcBucket,       // Source bucket.
    AkUniqueID in_nextID,                   // Segment ID of the bucket to append to the chain.
    AkUniqueID in_playlistItemID            // Playlist item ID of the bucket to append to the chain.
    )
{
    AKASSERT( !in_rule.pTransObj );

	bool bDisableNextBucketPlayback = false;
    AkInt32 iTimeToSync = 0;

    // Source (if not <NONE>):
	if ( in_pSrcBucket )
    {
        // Attach stop action for active segment.

        const AkMusicTransSrcRule & srcRule = in_rule.srcRule;
        AKASSERT( srcRule.eSyncType != SyncTypeImmediate ||
                  !"Sequence containers do not support Immediate transitions" );

        // Get time to sync.
        CAkMusicSegment * pSrcSegmentNode = static_cast<CAkMusicSegment*>(in_pSrcBucket->SegmentCtx()->SegmentNode());
        AKRESULT eResult = pSrcSegmentNode->GetExitSyncPos( 
            0,  // Minimal time-to-sync constraint related to source: 0, will be truncated if too long.
            (AkSyncType)srcRule.eSyncType,
            true,  // skip entry marker.
            iTimeToSync
            );

        AKASSERT( eResult == AK_Success );



        // Prepare and attach Stop action.
		AkInt32 iFadeOutDuration = srcRule.fadeParams.transitionTime;
        AkInt32 iStopOffset = srcRule.fadeParams.iFadeOffset -
                              CAkTimeConv::MillisecondsToSamples( iFadeOutDuration );
        
		// Deal with fades that are too long.
		if ( iStopOffset < -iTimeToSync )
		{
			iStopOffset = -iTimeToSync;
			iFadeOutDuration = CAkTimeConv::SamplesToMilliseconds( srcRule.fadeParams.iFadeOffset - iStopOffset );
		}

		// Consider post-exit.
        if ( iStopOffset == 0 &&
             srcRule.bPlayPostExit )
        {
            iStopOffset = pSrcSegmentNode->PostExitDuration();
        }

        iStopOffset += pSrcSegmentNode->ActiveDuration();
        
        in_pSrcBucket->AttachStopCmd( iFadeOutDuration, 
                                      srcRule.fadeParams.eFadeCurve, 
                                      iStopOffset );

		bDisableNextBucketPlayback = in_pSrcBucket->IsPlaybackDisabled();
    }


    // Instantiate next segment.
    CAkSegmentCtx * pNextCtx = NULL;
    const AkMusicTransDestRule & destRule = in_rule.destRule;
    AkInt32 iEntrySyncPos;
    if ( in_nextID != AK_MUSIC_TRANSITION_RULE_ID_NONE )
    {
        // Destination: 

        // Get next segment.
        CAkMusicSegment * pNextSegment = static_cast<CAkMusicSegment*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_nextID ));
        
		if ( pNextSegment )
		{
			AKVERIFY( pNextSegment->GetEntrySyncPos( destRule, 0, iEntrySyncPos ) == AK_Success );

			// Create a context for next segment.
			pNextCtx = pNextSegment->CreateSegmentCtxAndAddRef(
				this,
				Sequencer()->GameObjectPtr(),
				Sequencer()->GetUserParams() );

			if ( pNextCtx )
			{
#ifndef AK_OPTIMIZED
				pNextCtx->SetPlayListItemID( in_playlistItemID );
#endif
			}
			else
			{
				// If the next segment context cannot be created, we will enqueue a NULL bucket in the chain.
				// This sequence will be considered as finished.
				// If it is under a switch context, it will remain until there is a switch change. Otherwise,
				// it is a top-level and it will auto-clean (stop).
				// Set the iterator as invalid so that it does not try to grow again.
				m_PlayListIterator.SetAsInvalid();
			}
			
			pNextSegment->Release();
		}
		else
			m_PlayListIterator.SetAsInvalid();
    }

    // Append new sequenced segment.
    CAkSegmentBucket * pNextBucket = EnqueueSegment( iTimeToSync, pNextCtx );

	// Release new segment context (only) once it has been enqueued in the chain.
	if ( pNextCtx )
		pNextCtx->Release();

    if ( !pNextBucket )
	{
		// Cannot enqueue a new bucket. This is a fatal error.
		return HandleFatalError();
	}


    // Attach Play on next segment if applicable.
    if ( pNextCtx )
    {
        bool bPlayPreEntry = ( destRule.eEntryType == EntryTypeEntryMarker ) && destRule.bPlayPreEntry;

		// Prepare segment.
        // Set segment's source offset, get required look-ahead.
        // iPlayOffset is the offset where Play should start, relative to the EntrySync position.
        AkInt32 iPlayOffset = pNextBucket->Prepare( iEntrySyncPos, destRule.fadeParams, bPlayPreEntry );
		pNextBucket->ForceTimeToSync( iTimeToSync );
		AkInt32 iEffectiveFadeOffset = destRule.fadeParams.iFadeOffset + pNextCtx->GetStreamingLookAheadTime();
		
		
		// Prepare Play action.

        // Handle look-ahead that cannot be met.
#ifndef AK_OPTIMIZED
		if ( iPlayOffset > iTimeToSync && 
			 iTimeToSync > 0 )
		{
            // Might not have enough time to honor look-ahead. Post log.
            MONITOR_ERROR( AK::Monitor::ErrorCode_TooLongSegmentLookAhead );
        }
#endif

        pNextBucket->AttachPlayCmd( destRule.fadeParams.transitionTime, 
                                    destRule.fadeParams.eFadeCurve,
                                    iEffectiveFadeOffset, 
                                    -iPlayOffset );
    }

	if ( bDisableNextBucketPlayback )
		pNextBucket->DisablePlayback();

    return pNextBucket;
}

CAkSegmentBucket * CAkSequenceCtx::HandleFatalError()
{
	// A fatal error occurred. Get the last bucket and stop/clear it abruptly, so that it becomes the ending
	// Null bucket.
	CAkSegmentBucket * pLastBucket = LastSegmentBucket();
	
	// Set iterator as invalid.
	m_PlayListIterator.SetAsInvalid();

	return pLastBucket;
}

