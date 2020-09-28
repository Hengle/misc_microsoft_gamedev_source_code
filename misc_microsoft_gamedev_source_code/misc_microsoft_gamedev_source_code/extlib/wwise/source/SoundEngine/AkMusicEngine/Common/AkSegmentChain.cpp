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

#include "stdafx.h"
#include "AkSegmentChain.h"
#include "AkMusicSegment.h"
#include "AkMatrixSequencer.h"


#define NULL_ITEM_TIME -1


CAkSegmentChain::CAkSegmentChain()
:m_pLastBucket( NULL )
,m_pActiveBucket( NULL )
{
}

CAkSegmentChain::~CAkSegmentChain()
{
}

void CAkSegmentChain::Init()
{
    AkListBare<CAkTimeSequencedItem>::Init();
}

void CAkSegmentChain::Term()
{
    ClearChain();
    AkListBare<CAkTimeSequencedItem>::Term();
}

// Note. When the first segment of the chain has not reached its sync point yet, GetActiveSegment() still returns
// this segment, even though it is not technically the "active" segment.
CAkSegmentBucket * CAkSegmentChain::GetActiveSegment()
{
	return m_pActiveBucket;
}

// Delete all items. All segment item should be NULL (that is, properly destroyed following segment ctx notification).
void CAkSegmentChain::ClearChain()
{
    while ( !IsEmpty() )
    {
        CAkTimeSequencedItem * pItem = First();
        AKVERIFY( RemoveFirst() == AK_Success );

        pItem->DestroySegmentChainItem();
    }
	m_pActiveBucket = NULL;
}

// Insert branch item in chain: allow jump from one segment chain to another.
CAkSegmentChain::Iterator CAkSegmentChain::InsertBranchItem(
    IteratorEx &    in_itPreviousBucket,
    CAkBranchItem * in_pBranchItem        
    )
{
    AKASSERT( in_pBranchItem && in_itPreviousBucket != End() );
    ++in_itPreviousBucket;
    Insert( in_itPreviousBucket, in_pBranchItem );
    in_pBranchItem->TagAsLinked();
	return in_itPreviousBucket;
}

CAkSegmentBucket * CAkSegmentChain::EnqueueSegment(
    AkInt32 in_iTime,
    CAkSegmentCtx * in_pSegment
    )
{
    CAkSegmentBucket * pNewSequencedItem = AkNew( g_DefaultPoolId, CAkSegmentBucket( in_iTime, in_pSegment ) );
    if ( pNewSequencedItem )
    {
        AddLast( pNewSequencedItem );
        LastSegmentBucket( pNewSequencedItem );
    }
    return pNewSequencedItem;
}

// Changes sync time of first relevant segment so that its play action be scheduled to occur now.
AKRESULT CAkSegmentChain::SetAsTopLevel()
{
    AKASSERT( First() );

    // Create mandatory initial null segment.
    CAkSegmentBucket * pEmptySegment = AkNew( g_DefaultPoolId, CAkSegmentBucket( NULL_ITEM_TIME, NULL ) );
    if ( pEmptySegment )
    {
		pEmptySegment->MakeVolatile();

        CAkTimeSequencedItem * pFirst = First();

        // Adjust "real" first segment's time to sync to length of look-ahead.
        pFirst->ForceTimeToSync( GetChainRequiredLookAhead() );
		
		AddFirst( pEmptySegment );
		
		// The first relevant segment item should already be registered as first item of the sequence.
        AKASSERT( First() &&
                  First()->pNextItem );
		m_pActiveBucket = pEmptySegment;
        
        return AK_Success;
    }
    return AK_Fail;
}

void CAkSegmentChain::ProcessChainItems(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
    // Process actions of all chained items.
	// Consume time of all items that Synched in the past, plus the first one to Synch in the future.
	// Subsequent branch items should also consume time until a segment bucket is encountered.

    Iterator it = Begin();
    
	bool bDoConsume = true;
	AkInt32 iCumulTime = 0;

    while ( it != End() )
    {
		AKASSERT( in_iFrameDuration > 0 && in_iFrameDuration <= AK_NUM_VOICE_REFILL_FRAMES );

		AkInt32 iSyncTime = (*it)->SyncTime();

        // Update active bucket and chain length whenever a bucket reaches its sync point.
		AkInt32 iAbsSyncTime = iCumulTime + iSyncTime;
        if ( iAbsSyncTime >= 0
			&& iAbsSyncTime < in_iFrameDuration 
			&& (*it)->IsSegment() )
        {
			m_pActiveBucket = static_cast<CAkSegmentBucket*>(*it);
			UpdateChainLength( it, 0 );
        }

        // Execute imminent actions list. If the Sync point is crossed and the item is a bucket, 
		// in_iFrameDuration will come out as what is left of the frame after the Sync point.
		(*it)->ProcessActions( iAbsSyncTime, in_iFrameDuration, bDoConsume );

        // Process next item if this one has already synched, or if it is not a segment
        // (branch item time is relative to previous bucket, but subsequent bucket's relative time
        // is not affected by their insertion, therefore the latter must be processed now).
        if ( (*it)->SyncTime() >= 0 && (*it)->IsSegment() )
		{
			// Stop consuming.
			bDoConsume = false;
			// Cumulate sync times (consider this bucket's sync time before consumption).
			iCumulTime += iSyncTime;
		}

		++it;
    }
    Clean();
}

void CAkSegmentChain::ProcessRelativeChainItems(
	AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
	AKASSERT( First() && First()->IsSegment() && First()->SyncTime() == 0 );

	Iterator it = Begin();
	AkInt32 iAbsSyncTime = in_iSyncTime;

	while ( it != End() )
	{
		iAbsSyncTime += (*it)->SyncTime();
		(*it)->ProcessActions( iAbsSyncTime, in_iFrameDuration, false );
		++it;
	}
	// No need to Clean(): first item has not synched yet.
}

// Chain auto-cleansing. Remove obsolete items.
void CAkSegmentChain::Clean()
{
    // Conditions for item removal:
    // + Bucket must be ready.
    // + Its sync must have occurred in the past, that is, next bucket's time (if appl) is also negative.
    // Last bucket can only be removed under certain conditions (m_bIsTopLevel||!bucket->IsSegment())

    IteratorEx it = BeginEx();
    Iterator itNext;
    while ( it != End() )
    {
        if ( (*it)->SyncTime() < 0 )
        {
            itNext = it;
            ++itNext;

            if ( itNext != End() &&
                 (*itNext)->SyncTime() < 0 &&
                 (*it)->CanUnlink() )
            {
				if ( !(*it)->IsSegment() || 
					 static_cast<CAkSegmentBucket*>(*it)->IsVolatile() )
				{
					CAkTimeSequencedItem * pItem = (*it);
	        
					// Dequeue.
					it = Erase( it );

					// "Delete" it (branch items are not ours, they will not be deleted).
					pItem->DestroySegmentChainItem();
				}
				else
					++it;
            }
            else
                return;
			}
        else
			return;
    }

    // Chain is empty. 
    AKASSERT( IsEmpty() );
}

void CAkSegmentChain::FlushAllBuckets()
{
    while ( !IsEmpty() )
    {
		AKASSERT( First()->IsSegment() );
        CAkSegmentBucket * pBucket = static_cast<CAkSegmentBucket*>( First() );
        if ( pBucket->SegmentCtx() )
			pBucket->OnSegmentStoppedNotification( 0 );
        AKVERIFY( RemoveFirst() == AK_Success );
        pBucket->DestroySegmentChainItem();
    }
    LastSegmentBucket( NULL );
	m_pActiveBucket = NULL;
}

// Does not grow (default implementation).
CAkSegmentBucket * CAkSegmentChain::ForceGrow(
    AkInt32 & out_iTimeOffset
    )
{
    // Single sequencable segment cannot grow.
    AKASSERT( !IsEmpty() && LastSegmentBucket() );
    out_iTimeOffset = AK_INFINITE_SYNC_TIME_OFFSET;
    return NULL;
}

// Does nothing (default implementation).
void CAkSegmentChain::UpdateChainLength(
	const CAkSegmentChain::Iterator & in_curItem,	// Current item.
	AkInt32 in_iCurItemTime		// Current item time.
	)
{
}

// Prepares the chain according to the destination transition rule: prepares the first bucket 
// of the chain, adjusts the second bucket's sync time, returns the look-ahead required by the 
// chain (first 2 buckets).
// Returns AK_Success if an entry position could be found in the first segment.
AKRESULT CAkSegmentChain::Prepare(
	const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
	AkInt32 in_iDesiredEntryPosition,		// Desired entry position (applies to SameTime rule only).
	AkInt32 &	out_iRequiredLookAhead		// Returned required look-ahead time.
	)
{

	Iterator it = Begin();
	AKASSERT( it != End() );
	AKASSERT( (*it)->IsSegment() );
	CAkSegmentBucket * pFirstBucket = static_cast<CAkSegmentBucket*>(*it);

	// Get Entry position from first bucket's segment node (if it exists, otherwise it is 0).
	AkInt32 iEntryPosition = 0;
	CAkMusicSegment * pFirstSegmentNode = NULL;
	if ( pFirstBucket->SegmentCtx() )
	{
		pFirstSegmentNode = pFirstBucket->SegmentCtx()->SegmentNode();
		if ( pFirstSegmentNode->GetEntrySyncPos( in_rule, in_iDesiredEntryPosition, iEntryPosition ) != AK_Success )
		{
			return AK_Fail;
		}
	}

    // Play pre-entry?
    bool bPlayPreEntry = in_rule.eEntryType == EntryTypeEntryMarker && in_rule.bPlayPreEntry;


	AKASSERT( iEntryPosition >= 0 );

	// Prepare the first bucket.
	// Look-ahead of first bucket must not include the SyncTime, which is always overriden by the parent context.
	out_iRequiredLookAhead = pFirstBucket->Prepare( iEntryPosition, in_rule.fadeParams, bPlayPreEntry );
	AKASSERT( out_iRequiredLookAhead == -pFirstBucket->EarliestActionTime() );
	
	// Check all following buckets.
	UpdateChainLength( it, iEntryPosition );
	++it;

	if ( it != End() )
	{
		// Special case: Fix second bucket's sync time, and first bucket's stop time.
		AKASSERT( pFirstSegmentNode );
		(*it)->ForceTimeToSync( pFirstSegmentNode->ActiveDuration() - iEntryPosition );

		pFirstBucket->FixStopTime();

		out_iRequiredLookAhead = GetChainRequiredLookAhead();
	}

	return AK_Success;
}

AkInt32 CAkSegmentChain::GetChainRequiredLookAhead()
{
	// Look-ahead of first bucket (ignore sync time):
	Iterator it = Begin();
	AKASSERT( it != End() );
	AkInt32 iRequiredLookAhead = -(*it)->EarliestActionTime();

	// Look-ahead of following buckets:
	// Inspect all buckets of the chain in order to compute the true required look-ahead time.
	++it;
	AkInt32 iCumulSyncTime = 0;
	while ( it != End() )
	{
		// Get next bucket's look-ahead time, relative to its Sync time.
		iCumulSyncTime += (*it)->SyncTime();
		AkInt32 iNextRequiredLookAhead = - ( iCumulSyncTime + (*it)->EarliestActionTime() );
		if ( iNextRequiredLookAhead > iRequiredLookAhead )
			iRequiredLookAhead = iNextRequiredLookAhead;

		++it;
	}

	return iRequiredLookAhead;
}


// Sets "disable playback" flag to all buckets following in_itPreviousBucket.
// REVIEW (LX) If a switch item is inserted there by an inner switch, its switchee's buckets will not
// inherit the InhibatePlayback property.
void CAkSegmentChain::DisablePlaybackAt(
	Iterator &		io_itPreviousBucket
	)
{
	while ( ++io_itPreviousBucket != End() )
	{
		if ( (*io_itPreviousBucket)->IsSegment() )
		{
			CAkSegmentBucket * pBucket = static_cast<CAkSegmentBucket*>(*io_itPreviousBucket);
			pBucket->DisablePlayback();
			
			// Handle Play cmd that already occurred.
			if ( pBucket->HasPlaybackStarted() )
			{
				if ( pBucket == LastSegmentBucket() )
				{
					// REVIEW (LX)
					AkInt32 iDummy;
					ForceGrow( iDummy );
				}

				pBucket->RevertPlayback();
			}
		}
	}
}

void CAkSegmentChain::RevertDisabledPlaybacks()
{
	Iterator it = Begin();
	while ( it != End() )
	{
		if ( (*it)->IsSegment() )
		{
			static_cast<CAkSegmentBucket*>(*it)->EnablePlayback();
		}
		++it;
	}
}

bool CAkSegmentChain::CanRevertDisabledPlaybacks()
{
	Iterator it = Begin();
	while ( it != End() )
	{
		if ( (*it)->IsSegment() )
		{
			if ( static_cast<CAkSegmentBucket*>(*it)->WasPlaybackSkipped() )
				return false;
		}
		++it;
	}
	return true;
}




// ----------------------------------------------------------------------------------
// Null chain.
// ----------------------------------------------------------------------------------
CAkNullChain::CAkNullChain()
{
}

CAkNullChain::~CAkNullChain()
{
}

AKRESULT CAkNullChain::Init(
    CAkSegmentBucket *& out_pFirstBucket // One and only (with null segment).
    )
{
    CAkSegmentChain::Init();

    // Create the one and only null segment.
    out_pFirstBucket = EnqueueSegment( NULL_ITEM_TIME, NULL );
    if ( out_pFirstBucket )
    {
		InitActiveSegment( out_pFirstBucket );
        return AK_Success;
    }
    return AK_Fail;
}


