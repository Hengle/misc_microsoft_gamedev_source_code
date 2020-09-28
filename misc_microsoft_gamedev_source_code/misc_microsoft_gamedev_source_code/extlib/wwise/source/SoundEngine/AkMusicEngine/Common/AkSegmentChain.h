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


#ifndef _SEGMENT_CHAIN_H_
#define _SEGMENT_CHAIN_H_

#include <AK/Tools/Common/AkObject.h>
#include "AkListBare.h"
#include "AkMusicStructs.h"

class CAkTimeSequencedItem;
class CAkSegmentBucket;
class CAkBranchItem;
class CAkMatrixAwareCtx;
class CAkSegmentCtx;


class CAkSegmentChain : public AkListBare<CAkTimeSequencedItem>
{
public:
    CAkSegmentChain();

    void Init();
    void Term();

	// Get active segment. 
	// Note. When the first segment of the chain has not reached its sync point yet, GetActiveSegment() still returns
	// this segment, even though it is not technically the "active" segment.
	CAkSegmentBucket * GetActiveSegment();

    // Sequencer processing.
    void ProcessChainItems(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
	// Relative processing (sync time of first item is specified from above).
	void ProcessRelativeChainItems(
		AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    
    // Insert branch item in chain: allow jump from one segment chain to another.
	// Returns the time sequenced item following branch insertion (can be another branch).
    Iterator InsertBranchItem(
        IteratorEx &    io_itPreviousBucket,	// In: bucket preceeding branch insertion.
        CAkBranchItem * in_pBranchItem        
        );

    // Virtual methods to implement: chain growth, associated MatrixAwareCtx access.
	// Default implementation does nothing, return NULL.
    virtual CAkSegmentBucket * ForceGrow(
        AkInt32 & out_iTimeOffset
        );
	// Default implementation does nothing.
	virtual void UpdateChainLength(
		const CAkSegmentChain::Iterator & in_curItem,	// Current item.
		AkInt32 in_iCurItemTime		// Current item time.
		);
    virtual CAkMatrixAwareCtx * MatrixAwareCtx() = 0;
	
	// Prepares the chain according to the destination transition rule: prepares the first bucket 
	// of the chain, adjusts the second bucket's sync time, returns the look-ahead required by the chain.
	// Returns AK_Success if an entry position could be found in the first segment.
	AKRESULT Prepare(
		const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
		AkInt32 in_iDesiredEntryPosition,		// Desired entry position (applies to SameTime rule only).
		AkInt32 &	out_iRequiredLookAhead		// Returned required look-ahead time.
		);

	// Sets "disable playback" flag to all buckets following in_itPreviousBucket.
	void DisablePlaybackAt(
		Iterator &		io_itPreviousBucket
		);
	void RevertDisabledPlaybacks();
	bool CanRevertDisabledPlaybacks();

protected:

    virtual ~CAkSegmentChain();

	// Delete all items. All segment item should be NULL (that is, properly destroyed following segment ctx notification).
    void ClearChain();

    // Enqueue a new segment at the end of the chain.
    CAkSegmentBucket * EnqueueSegment(
        AkInt32 in_iTime,
        CAkSegmentCtx * in_pSegment
        );

	// Initial setup:
	// Set active segment.
	void InitActiveSegment( CAkSegmentBucket * in_pBucket ) { m_pActiveBucket = in_pBucket; }
    // Sets IsTopLevel property. 
    AKRESULT SetAsTopLevel();

    // Last segment bucket access.
    // Chains keep a pointer to the last item which has type SegmentBucket.
    CAkSegmentBucket * LastSegmentBucket() { return m_pLastBucket; }

    void FlushAllBuckets();
    
	// Returns the required look-ahead of the chain (inspects all buckets of the chain).
	AkInt32 GetChainRequiredLookAhead();

private:

    // Chain auto-cleansing. Remove obsolete items.
    void Clean();

	void LastSegmentBucket(
        CAkSegmentBucket * in_pBucket 
        ) 
    {
        m_pLastBucket = in_pBucket; 
    }
    
	// Active segment bucket.
	CAkSegmentBucket * m_pActiveBucket;

    // Pointer to last item that is a segment bucket.
    CAkSegmentBucket * m_pLastBucket;

};


// TODO Move Null Chain implementation to NULL context's.

class CAkNullChain : public CAkSegmentChain
{
public:
    CAkNullChain();
    virtual ~CAkNullChain();

    AKRESULT Init(
        CAkSegmentBucket *& out_pFirstBucket // One and only bucket (with null segment).
        );

};

#endif //_SEGMENT_CHAIN_H_
