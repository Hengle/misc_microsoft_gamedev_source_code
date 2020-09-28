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
// AkMatrixSequencer.h
//
// Multi-chain, branchable segment sequencer.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MATRIX_SEQUENCER_H_
#define _MATRIX_SEQUENCER_H_

#include <AK/Tools/Common/AkArray.h>
#include "AudiolibDefs.h"
#include "AkMusicStructs.h"
#include "PrivateStructures.h"
#include "AkSegmentChain.h"
#include "AkSegmentCtx.h"
#include "AkStateMgr.h"
#include "AkMatrixAwareCtx.h"

class CAkMusicSwitchCtx;

#define AK_INFINITE_SYNC_TIME_OFFSET    (-1)    // time offset returned by a segment chain when it cannot grow further.

struct AkSegmentPlayCmd
{
    AkInt32         iRelativeTime;
    AkMusicFade     fadeParams;
	AkUInt32		bIsPlaybackDisabled	:1;	// When set, play command will be skipped.
	AkUInt32		bWasPlaybackSkipped:1;	// Set when play command is skipped because of flag bIsPlaybackDisabled.
    
    AkSegmentPlayCmd()
    :iRelativeTime( 0 )
	,bIsPlaybackDisabled( false )
	,bWasPlaybackSkipped( false )
    { }
};

struct AkSegmentStopCmd
{
    AkInt32         iRelativeTime;
    TransParams     transParams;
    
    AkSegmentStopCmd()
    :iRelativeTime( 0 )
    { }
};

// Scheduled stingers record keeping.
struct AkStingerRecord
{
    AkTriggerID                 triggerID;
    AkUniqueID	                segmentID;
    AkInt32                     iDontRepeatTime;    // Takes time before playback into account.
    AkUInt32                    uStopRelativeTime;  // Time between play and stop commands.
    CAkSmartPtr<CAkSegmentCtx>  pStingerSegment;    // REVIEW. Could use a SegmentBucket instead.
    AkUInt32                    bPlaybackStarted :1;// True when playback has started (includes stream preparation).
    AkUInt32                    bCanBeRescheduled:1;// True when they can be rescheduled on path change: 
        // applies to stingers which have look-ahead property and were scheduled in current segment.
};

// Additionnal actions associated to buckets: Used for stingers and states.
enum AkAssociatedActionType
{
    AssocActionTypeStinger,
    AssocActionTypeState
};
struct AkAssociatedAction
{
    AkInt32                 iRelativeTime;
    AkAssociatedActionType  eActionType;
    union
    {
        AkStingerRecord *   pStingerRecord;
        void *              pStateChangeCookie;
    };
};

typedef AkArray<AkAssociatedAction, const AkAssociatedAction&, ArrayPoolDefault,DEFAULT_POOL_BLOCK_SIZE/sizeof(AkAssociatedAction)> AssociatedActionsArray;

class CAkSegmentBucket;

class CAkTimeSequencedItem : public CAkObject
{
public:
    CAkTimeSequencedItem(
        AkInt32 in_iRelativeTime,
        bool    in_bIsSegment
        );
    virtual ~CAkTimeSequencedItem();

    // Getters.
    // Sync time (relative to previous segment bucket). Negative if already synched.
    inline AkInt32 SyncTime() { return m_iRelativeTime; }
    // Time of earliest scheduled action (relative to this item). 
    inline AkInt32 EarliestActionTime() { return m_iEarliestCmdTime; }

    // Used to setup initial first chained bucket's look-ahead.
    void ForceTimeToSync( 
        AkInt32 in_iTime
        ) 
    { 
        m_iRelativeTime = in_iTime; 
    }

    // Process actions. To be called from segment chain once per audio frame.
    // Returns true when it sync happens therein (sync happens when SyncTime becomes negative).
    void ProcessActions(
        AkInt32 in_iSyncTime,				// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
		AkInt32 & io_iFrameDuration,		// Number of samples to process. If the sync point is crossed, it comes out as the remaining of the frame after sync.
		bool	in_bDoConsume				// True when sync time must be consumed. False if processed as a slave.
        );

    // Ask permission to remove from chain.
    virtual bool CanUnlink() = 0;
    // Destroy AFTER having removed from chain.
    virtual void DestroySegmentChainItem() = 0;

    // For chain navigation.
    bool IsSegment() { return m_bIsSegment; }
    // Get segment bucket access type conditional logic.
    virtual CAkSegmentBucket * GetNextBucket(
        CAkMatrixAwareCtx * in_pOwner,      // Owner for branch decision.
        CAkSegmentChain *&  io_pChain,      // Chain object passed along
        AkInt32 &           out_iTimeOffset // time offset between this item and return CAkSegmentBucket.
        );

    // List bare public link.
    CAkTimeSequencedItem *  pNextItem;

protected:

    // Commands processing.
    // Standard, from Chain.
    virtual void ExecuteImminentActions(
        AkInt32 in_iSyncTime,					// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
		AkInt32 in_iFrameDuration				// Number of samples to process.
        ) = 0;

    // Consumes time. 
    void ConsumeTime(
        AkUInt32 in_uConsumeSize
        ) 
    { 
        m_iRelativeTime -= in_uConsumeSize; 
    }

    // Update earliest action time on command attach.
    inline void EarliestActionTime(
        AkInt32 in_iEarliestCmdTime
        )
    { 
        m_iEarliestCmdTime = in_iEarliestCmdTime; 
    }

private:
    AkInt32     m_iEarliestCmdTime;
    AkInt32     m_iRelativeTime;
    bool        m_bIsSegment;
    
};

class CAkSegmentBucket : public CAkTimeSequencedItem
{
public:
    CAkSegmentBucket(
        AkInt32 in_iRelativeTime,
        CAkSegmentCtx * in_pSegment
        );
    virtual ~CAkSegmentBucket();

    CAkSegmentCtx * SegmentCtx() { return m_pSegment; }

	// Returns segment's active duration (0 if no segment).
	AkInt32 SegmentDuration();
    
    // Prepares segment context, modifies associated Play action time to look-ahead.
    // The segment will be ready to play at the specified Entry Position (relative to EntryCue). They might however
	// be set to begin earlier, in case in_iFadeOffset is negative, but in this case, the returned preparation
	// time takes this duration into account.
	// Returns the exact time that needs to elapse between segment's _Play() and the Sync point.
	// May modify earliest action time.
    AkInt32 Prepare( 
        AkInt32 in_iEntryPosition,	// Entry Position, relative to Entry Cue. Can be negative.
        const AkMusicFade & in_iFade,	// Fade. Real source offset will take fade offset into account. 
        bool    in_bPlayPreEntry	// True when the pre-entry should be heard.
        );
	AkInt32 Prepare( 
		AkInt32 in_iEntryPosition	// Entry Position, relative to Entry Cue. Can be negative.
		);

    // Used to notify bucket that segment context needs to be stopped/released.
    // Clears context smart pointer, invalidates play/stop commands and recomputes earliest action time.
    void OnSegmentStoppedNotification(
		AkUInt32 in_uSubFrameOffset
		);

	// Find a sync position in bucket given sync rules and time constraint.
	// Returns time of sync relative to the bucket's time.
    // If no possible exit point is found the method returns AK_Fail. AK_Success if it was found.
	// Note: Considers following branch items.
    AKRESULT FindSyncPosition(
		CAkMatrixAwareCtx * in_pOwner,			// Owner for branching decision.
		CAkSegmentChain *   in_pChain,			// Chain object passed along.
        AkInt32         in_iTimeConstraint,     // Time constraint: minimum time before finding a valid sync point (absolute).
		AkInt32         in_iMinSegmentPosition,	// Time constraint: minimum segment position (segment relative position, default 0).
        AkSyncType      in_eSyncType,           // Sync rule.
        bool            in_bDoSkipEntryMarker,  // If true, will not consider Entry marker (returned position will be exclusively greater than 0).
		bool			in_bSucceedOnNothing,	// If true, will return a valid position if segment is <nothing>. Otherwise, will succeed only if SyncType is Immediate.
        AkInt32 &       out_iSyncPosition		// Returned Sync position, relative to bucket's time.
        );

	// Convert bucket-relative positions into segment context-relative positions.
    AkInt32 ConvertToSegmentCtxPosition(
		AkInt32			in_iBucketRelativePos	// Bucket relative position.
		);

	// Convert bucket-relative positions to absolute time (position - NOW).
	// Important: Conversion does not apply if this bucket is scheduled in the future 
	// (has not synched, SyncTime()>0): position returned = position given.
	AkInt32 ConvertToAbsoluteTime(
		AkInt32			in_iBucketRelativePos	// Bucket relative position.
		);

    // Commands scheduling.
    // May modify earliest action time.
    void AttachPlayCmd(
        AkTimeMs                in_iTransDuration,
        AkCurveInterpolation    in_eFadeCurve,
        AkInt32                 in_iFadeOffset,
        AkInt32                 in_iRelativeTime
        );
    void AttachPlayCmd(
        const AkMusicFade &     in_fadeParams,
        AkInt32                 in_iRelativeTime
        );
    void AttachStopCmd(
        AkTimeMs                in_iTransDuration,
        AkCurveInterpolation    in_eFadeCurve,
        AkInt32                 in_iRelativeTime
        );
    // Modify Stop Cmd time: perform a straight stop at either the Exit Cue or end of Post-Exit.
    // Note. Stop command is left untouched if a fade out is already defined.
    void ForcePostExit(
        bool                    in_bPlayPostExit    // Move stop at the end of Post-Exit if True, and Exit Cue if false.
        );
	
	// Recompute stop time based on segment's offset (m_iSegmentStartPos).
	void FixStopTime();

	// Inhibates playback: of bucket's segment. Play command's InhibatePlayback flag is set,
	// which will block its execution. If the play command already occurred, it is stopped
	// (with a fade-out, which length equals the minimum between the time elapsed since the
	// command occurred and the sync time).
	// Warning: must not be called after bucket has synched.
	void DisablePlayback();
	void EnablePlayback();
	bool IsPlaybackDisabled() { return m_cmdPlay.bIsPlaybackDisabled; }
	bool WasPlaybackSkipped() { return m_cmdPlay.bWasPlaybackSkipped; }

	// Query and revert action on playback. These should be called only before Sync (they are usable to revert
	// the pre-entry and/or streaming look-ahead).
	bool HasPlaybackStarted();
	void RevertPlayback();

    AKRESULT AttachAssociatedAction(
        AkAssociatedAction & in_action
        );

    // Removes all associated actions that are scheduled to occur after in_iMinActionTime and pushes them into io_arCancelledActions.
    // Stingers that already started playing are not removed.
    void PopAssociatedActionsToReschedule(
        AssociatedActionsArray & io_arCancelledActions,
        AkInt32 in_iMinActionTime
        );

    // Ask permission to remove from chain.
    virtual bool CanUnlink();
	// Destroy AFTER having removed from chain.
    virtual void DestroySegmentChainItem();

	// Empty segment buckets are always persistent unless explicitly set volatile through this method.
	// Persistent buckets are not removed by segment chains auto-clean.
	void MakeVolatile() { m_bIsVolatile = true; }
	bool IsVolatile() { return m_bIsVolatile; }

protected:

    // Commands processing.
    // Standard, from Chain.
    // Recomputes earliest action time.
    virtual void ExecuteImminentActions(
        AkInt32 in_iSyncTime,					// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );
    
    // Scans actions and recomputes earliest action time.
    // Excludes actions earlier than in_iMinSyncTime.
    void RecomputeEarliestActionTime(
        AkInt32 in_iMinSyncTime 
        );

private:
    // For stingers and state changes.
    AssociatedActionsArray  m_arAssociatedActions;

    AkSegmentPlayCmd        m_cmdPlay;
    AkSegmentStopCmd        m_cmdStop;

    CAkSmartPtr<CAkSegmentCtx> m_pSegment;
	AkInt32					m_iSegmentStartPos;	// Segment's Entry Position; Offset between segment's position and bucket's position. Set in Prepare().
	AkInt32					m_iOriginalStopTime;// Bucket's stop time, originally set by chain scheduler. Used to recompute real stop time when m_iSegmentStartPos > 0.
	bool					m_bIsVolatile;
};

class CAkBranchItem : public CAkTimeSequencedItem
{
public:
    CAkBranchItem(
        AkInt32                 in_iRelativeTime,
        CAkMusicSwitchCtx *     in_pOwner,
        CAkMatrixAwareCtx *     in_pSwitchee
        );
    virtual ~CAkBranchItem();


    CAkMatrixAwareCtx * Switchee() { return m_pSwitchee; }
	CAkMusicSwitchCtx * Owner() { return m_pSwitchOwner; }

	// Get segment bucket access type conditional logic.
    virtual CAkSegmentBucket * GetNextBucket(
        CAkMatrixAwareCtx * in_pOwner,      // Owner for branch decision.
        CAkSegmentChain *&  io_pChain,      // Chain object passed along
        AkInt32 &           out_iTimeOffset // time offset between this item and return CAkSegmentBucket.
        );

    // Ask permission to remove from chain.
    virtual bool CanUnlink();

    bool IsLinked() { return m_bIsLinked; }
    void TagAsLinked();
    
protected:

	void TagAsUnlinked();

    // Commands processing.
    // Standard, from Chain.
    virtual void ExecuteImminentActions(
        AkInt32 in_iSyncTime,					// Sync time: usually this bucket's SyncTime() unless it is processed as a slave of a higher level context.
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );

	void ReleaseSwitchee() { m_pSwitchee = NULL; }

    CAkMusicSwitchCtx * m_pSwitchOwner; // Needed for branching decision

private:
	CAkSmartPtr<CAkMatrixAwareCtx> m_pSwitchee;

    // For chain navigation. Branch or not depending on input context's place in hierarchy. 
    bool IsDescendent(
        CAkMusicCtx * in_pChild
        );

    bool m_bIsLinked;
};


class CAkMatrixSequencer : public CAkObject,
                           public IAkTriggerAware
{
public:
    CAkMatrixSequencer(
        CAkMatrixAwareCtx * in_pOwner
        );
    virtual ~CAkMatrixSequencer();

    AKRESULT Init(
        UserParams &    in_rUserparams,
        CAkRegisteredObj *  in_pGameObj
        );
    void Term();

    // Used by Renderer:
    //
    void Execute();


    // Shared object accross a MatrixAware context hierarchy.
    inline CAkRegisteredObj * GameObjectPtr() { return m_pGameObj; }
    inline AkPlayingID		  PlayingID() { return m_UserParams.PlayingID; }
    inline UserParams &		  GetUserParams() { return m_UserParams; }

	AkInt32 GetCurSegmentPosition();

    // Matrix iterator. 
    // -----------------------------------------------------
    struct Iterator
	{
        CAkSegmentChain *   pChain;     // current segment chain.
		CAkSegmentBucket *  pBucket;    // current segment bucket.
        
		inline void NextBucket( 
            CAkMatrixAwareCtx * in_pOwner,      // owner context will determine chain branching conditions.
            AkInt32 &           out_iTimeOffset // time offset between previous and next target bucket (always >= 0).
            )
		{
            AKASSERT( pBucket && pChain );
            
			pBucket = pBucket->GetNextBucket( in_pOwner, pChain, out_iTimeOffset );
		}

		inline CAkSegmentBucket * operator*() const
		{
			AKASSERT( pBucket );
			return pBucket;
		}

        // Get an equivalent segment chain iterator ex.
        CAkSegmentChain::IteratorEx ChainIteratorEx() const
        {
            AKASSERT( pBucket && pChain );
            CAkSegmentChain::IteratorEx it = pChain->BeginEx();
            while ( it != pChain->End() &&
                    (*it) != pBucket )
            {
                ++it;
            }
            return it;
        }

        bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pBucket != in_rOp.pBucket );
		}
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pBucket == in_rOp.pBucket );
		}

        // Get segment node 
        CAkMusicSegment * SegmentNode() const
        {
            return ( pBucket->SegmentCtx() ) ? pBucket->SegmentCtx()->SegmentNode() : NULL;
        }
    };
    // -----------------------------------------------------

    inline Iterator End()
	{
		Iterator returnedIt;
		returnedIt.pBucket = NULL;
		return returnedIt;
	}

    Iterator GetCurBucket(
        CAkMatrixAwareCtx * in_pOwnerCtx    // Owner context determines path to follow.
        );


    // IAkTriggerAware implementation:
    // ----------------------------------------
    virtual void Trigger( 
        AkTriggerID in_triggerID 
        );


    // Delayed states management:
    // ----------------------------------------
    // See if someone in the hierarchy of the current segment (or the next segment if it playing) 
    // is registered to in_stateGroupID, and if it is not immediate.
    // Returns the absolute number of samples in which state change should be effective.
    // Also, returns 
    // the segment look-ahead index which responded to the state group 
    // (0=current segment, 1=next segment, etc.);
    // the sync time relative to this segment.
    AkInt32 QueryStateChangeDelay( 
        AkStateGroupID  in_stateGroupID,   
        AkUInt32 &      out_uSegmentLookAhead,  // returned segment look-ahead index which responded to the state group.
        AkInt32 &       out_iRelativeSyncTime   // returned sync time relative to this segment.
        );
    // Handle delayed state change. Compute and schedule state change in sequencer.
    AKRESULT ProcessDelayedStateChange(
        void *          in_pCookie,             // cookie for Renderer callback.
        AkUInt32        in_uSegmentLookAhead,   // handling segment index (>=0).
        AkInt32         in_iRelativeSyncTime    // sync time relative to this segment.
        );

    // Top-level segment chain listener.
    // ----------------------------------------
    void OnPathChange(
        CAkSegmentBucket * in_pCurBucket,   // Current bucket.
        CAkSegmentBucket * in_pOldBucket,   // Previously expected next bucket.
        AkInt32            in_iTimeOffset   // Time offset between current and new next bucket.
        );

private:

    // Stingers management.
    // ---------------------------------------

    // Handle trigger event.
    void HandleTrigger( 
        AkTriggerID in_triggerID, 
        bool in_bCurrentSegmentOnly 
        );

    // Trigger registration.
    void RegisterTriggers();
    void UnregisterTriggers();

    // Get first music node from which we can get some stingers.
    // Usually it will be the segment node associated with the segment context of the inspected bucket.
    // However, if it is an empty bucket, we use the node associated with the segment chain.
    CAkMusicNode * GetNodeFromBucket( 
        const Iterator & in_iter
        );
    // Creates a stinger-segment context and addrefs it. Caller needs to release it. 
	// Returns NULL and cleans up if failed.
    CAkSegmentCtx * CreateStingerSegmentAndAddRef(
        AkUniqueID in_segmentID,
        AkInt32 & out_iLookAheadDuration
        );
    // Schedule stinger playback.
    AKRESULT ScheduleStingerForPlayback(
        CAkSegmentBucket *  in_pHostBucket,
        CAkSegmentCtx *     in_pStingerCtx,
        const CAkStinger *  in_pStingerData,
        AkInt32             in_iSyncTime,
        AkInt32             in_iLookAheadDuration,
        bool                in_bScheduledInCurrentSegment
        );    
    // Add to pending stingers record keeping.
    AkStingerRecord * EnqueueStingerRecord( 
        const CAkStinger *  in_pStingerData,
        CAkSegmentCtx *     in_pStingerCtx,
        AkInt32             in_iLookAheadDuration,
        bool                in_bScheduledInCurrentSegment
        );
    // Can play stinger. False if another stinger is there, with positive don't repeat time.
    bool CanPlayStinger( 
        const CAkStinger *  in_pStingerData
        );
    // Consumes don't repeat time of all playing stingers.
    // Cleans up pending stingers.
    void ConsumePlayingStingersTime();
    // Clears pending stingers when top context stopped playing.
    void ClearAllPendingStingers();
    void ClearStingerRecord( 
        const AkStingerRecord * in_pStingerRecord
        );

    // Helpers for state management.
    // -----
    
    enum AkDelayedStateHandlingResult
    {
        AK_DelayedStateSyncFound, 
        AK_DelayedStateCannotSync, 
        AK_DelayedStateImmediate,
        AK_DelayedStateNotHandled
    };

    // If at least one node of the segment's hierarchy is registered to the state group,
    // - returns AK_TriggerImmediate if it requires an immediate state change,
    //  if it also requires a delayed state processing, 
    //  - returns AK_TriggerSyncFound if it was able to schedule it,
    //  - returns AK_TriggerCannotSync if it was not.
    // Otherwise, 
    // - returns AK_TriggerNotHandled.
    // Returned delay is relative to now (segment's position).
    AkDelayedStateHandlingResult GetEarliestStateSyncTime( 
        const CAkMatrixSequencer::Iterator & in_itBucket, 
        AkStateGroupID  in_stateGroupID, 
        AkInt32 &       out_iRelativeSyncTime
        );

private:

    CAkMatrixAwareCtx * m_pOwner;

    // Shared object accross a MatrixAware context hierarchy.
    UserParams			m_UserParams;   // User Parameters.
    CAkRegisteredObj*	m_pGameObj;		// CAkRegisteredObj to use to Desactivate itself once the associated game object were unregistered.

    typedef CAkList2<AkStingerRecord, const AkStingerRecord&, AkAllocAndFree> PendingStingersList;
    PendingStingersList m_listPendingStingers;
};

#endif //_MATRIX_SEQUENCER_H_
