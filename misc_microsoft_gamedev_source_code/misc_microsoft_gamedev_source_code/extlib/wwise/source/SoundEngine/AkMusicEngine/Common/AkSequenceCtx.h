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
// AkSequenceCtx.h
//
// Music RanSeq Container Context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _SEQUENCE_CTX_H_
#define _SEQUENCE_CTX_H_

#include "AkMatrixAwareCtx.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkRSIterator.h"

#include "AkMatrixSequencer.h"

class CAkMusicRanSeqCntr;

class CAkSequenceCtx : public CAkMatrixAwareCtx
                      ,public CAkSegmentChain
{
public:
    CAkSequenceCtx(
        CAkMusicRanSeqCntr *in_pSequenceNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkSequenceCtx();

    // Initializes the context. Automatically performs a one-segment look-ahead.
    // Returns first segment that was scheduled in the sequencer.
	// IMPORTANT. OnStops itself on failure.
    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams,
        CAkSegmentBucket *& out_pFirstBucket    // First bucket.
        );
	// Called by parent (switches): completes the initialization.
	// Sequences set their iterator ready.
	virtual void EndInit();


    // Matrix Aware Context implementation.
    // ----------------------------------------------------------

    // Sequencer access.
    virtual void Process(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    virtual void ProcessRelative(
        AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );

    virtual CAkSegmentChain * GetActiveChain();

    // For Music Renderer's music contexts look-up.
    virtual CAkMusicNode * Node();

	// Query for transition reversal. Asks segment chain if it is not too late to start playing again.
	virtual bool CanRestartPlaying();
	// Revert playback commands that were disabled. Only possible if it can restart playing.
	// Default implementation does nothing.
	virtual void RestartPlaying();

    CAkMusicRanSeqCntr * SequenceNode() { return m_pSequenceNode; }
    // ----------------------------------------------------------

    // Context commands
	//
    
    // Override MusicCtx OnStopped: Need to flush actions to release children.
    virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );

    // Initializes playlist iterator so that it points to the top-level node of the playlist that corresponds
    // to the in_uJumpToIdx. Flushes sequencer look-ahead if necessary.
    // Can only be called before the sequence starts playing.
    // Returns the new first segment bucket (of the sequence).
    CAkSegmentBucket * JumpToSegment(
        AkUniqueID in_uJumpToID                // Top-level playlist jump index.
        );

    
    // CAkSegmentChain implementation
    // ----------------------------------------------------------
	// Appends a bucket to ths chain.
    virtual CAkSegmentBucket * ForceGrow(
        AkInt32 & out_iTimeOffset
        );
	// Grows chain so that it has at least MINIMAL_BUCKET_LOOK_AHEAD buckets ahead of the current 
	// item (in_curItem). If the current bucket's position is greater than 0, chain is grown so that it has
	// at least MINIMAL_BUCKET_LOOK_AHEAD+1 buckets ahead of the current item.
	virtual void UpdateChainLength(
		const CAkSegmentChain::Iterator & in_curItem,	// Current item.
		AkInt32 in_iCurItemTime		// Current item time.
		);
    virtual CAkMatrixAwareCtx * MatrixAwareCtx();
    // ----------------------------------------------------------

private:
    // Returns new sequenced segment item printed in the scheduler.
    CAkSegmentBucket * ScheduleNextSegment();
    // Appends a bucket to the chain following a music transition rule.
    CAkSegmentBucket * AppendBucket( 
        const AkMusicTransitionRule & in_rule,  // Transition rule between source bucket (in_pSrcBucket) and next one.
        CAkSegmentBucket * in_pSrcBucket,       // Source bucket.
        AkUniqueID in_nextID,                   // Segment ID of the bucket to append to the chain.
        AkUniqueID in_playlistItemID            // Playlist item ID of the bucket to append to the chain.
        );
    void LookAheadSequence(
        AkUInt16 in_uLookAhead
        );
	CAkSegmentBucket * HandleFatalError();

private:
    CAkMusicRanSeqCntr *        m_pSequenceNode;
    
    // Current playlist iterator
	AkRSIterator				m_PlayListIterator;
};

#endif //_SEQUENCE_CTX_H_
