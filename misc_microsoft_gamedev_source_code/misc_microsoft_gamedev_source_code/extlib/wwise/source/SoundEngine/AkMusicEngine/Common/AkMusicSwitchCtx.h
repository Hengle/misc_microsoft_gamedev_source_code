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
// AkMusicSwitchCtx.h
//
// Music Switch Container Context.
//
//////////////////////////////////////////////////////////////////////
#ifndef _MUSIC_SWITCH_CTX_H_
#define _MUSIC_SWITCH_CTX_H_

#include "AkMatrixAwareCtx.h"
#include "AkRegisteredObj.h"
#include "PrivateStructures.h"
#include "AkContinuationList.h"     // For SmartPtr.
#include "AkMusicTransAware.h"
#include "AkSwitchAware.h"
#include "AkMatrixSequencer.h"
#include "AkList2.h"

class CAkMusicSwitchCntr;




//-----------------------------------------------------------------------------
// Name: CAkNullCtx
// Desc: Matrix aware context that implement an empty sequence.
//-----------------------------------------------------------------------------
class CAkNullCtx : public CAkMatrixAwareCtx
                  ,public CAkNullChain
{
public:
    CAkNullCtx(
        CAkMusicCtx *   in_parent = NULL
        );
    virtual ~CAkNullCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_pGameObj,
        UserParams &    in_rUserparams,
        CAkMatrixSequencer * in_pSequencer,
        CAkSegmentBucket *& out_pFirstBucket    // First bucket (null segment).
        );

	// MusicCtx override. AddRef and Release.
    virtual void OnPlayed(
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
		);
    virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );

    virtual void Process(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    virtual void ProcessRelative(
        AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );

    virtual CAkSegmentChain * GetActiveChain();
	virtual bool IsNullCtx();	// Returns true.

	// Query for transition reversal. Default implementation: true.
	virtual bool CanRestartPlaying();
	// Revert playback commands that were disabled. Only possible if it can restart playing.
	// Default implementation does nothing.
	virtual void RestartPlaying();

    // For Music Renderer's music contexts look-up: concrete contexts must return their own node.
    virtual CAkMusicNode * Node();

    // CAkSegmentChain implementation
    // ----------------------------------------------------------
    virtual CAkMatrixAwareCtx * MatrixAwareCtx();
    // ----------------------------------------------------------

private:
	bool m_bWasReferenced;
};


//-----------------------------------------------------------------------------
// Name: CAkPendingSwitchTransition
// Desc: Pending switch. The switch context keeps a list of all pending switches 
//       (either active or obsolete, but kept alive). They are referenced as 
//       branch items in segment chains, for complex segment paths management.
//-----------------------------------------------------------------------------
struct AkSwitchPlayCmd
{
    AkInt32         iRelativeTime;
    AkMusicFade     fadeParams;
    // Target is always the branch item switchee's.
	AkUInt32		bPending	:1;
};

struct AkSwitchStopCmd
{
    AkInt32         iRelativeTime;
    TransParams     transParams;
    CAkMatrixAwareCtx *   pTarget;
    bool            bPending;
};

class CAkPendingSwitchTransition : public CAkBranchItem
{
public:
    CAkPendingSwitchTransition(
        AkInt32                 in_iRelativeTime,
        CAkMusicSwitchCtx *     in_pOwner,
        CAkMatrixAwareCtx *     in_pSwitchee,
		bool					in_bIsFromTransSegment
        );
    virtual ~CAkPendingSwitchTransition();

	// Release context: Call when owner switch stops.
	void ReleaseContext();

    // Branch item override:
    // Ask permission to remove from chain.
    virtual bool CanUnlink();

	bool NeedsProcessing();

    // Process switch transition: Notifies Sync, RelativeProcess()es switchee before sync,
	// Process()es it after sync,
    // executes high-level transition commands (play and stop on higher-level contexts).
    // Consumes absolute switch time.
    void ProcessSwitchTransition(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    
    AkInt32 SwitchSyncTime() { return m_iSwitchSyncTime; }
    void SwitchSyncTime(
        AkInt32 in_iSwitchSyncTime
        );

    // Transition reversal.
	bool CanBeReverted();
	void SetAsReverted();
    void RevertTransition(
        CAkPendingSwitchTransition * in_pNewTransition,	// New transition, to compute fade-in time. Can be NULL.
		bool in_bRestartPlayingPreviousCtx
        );
	bool IsFromTransSegment() { return m_bIsFromTransSegment; }

    // Commands scheduling.
    void AttachPlayCmd(
        const AkMusicFade &     in_fadeParams,
        AkInt32                 in_iRelativeTime
        );
    void AttachStopCmd(
        CAkMatrixAwareCtx*      in_pTarget,
        AkTimeMs                in_iTransDuration,
        AkCurveInterpolation	in_eFadeCurve,
        AkInt32                 in_iRelativeTime
        );

	
	// Destruction.
    // Destroy AFTER having removed from chain.
	// IMPORTANT: Most of the time the pending switch is the property of the switch context. 
	// Therefore this method, which is intended to be called from a segment chain, simply marks it as unlinked, 
	// so that it can be cleaned up later by the switch context. However, if this transition is reverted, 
	// it is dequeued from the switch context's transition list, and therefore becomes the property of the chain, 
	// which will destroy it through a call to this method.
    virtual void DestroySegmentChainItem();

    // Can safely destroy pending switch if it does not need processing, and it is not linked in a segment chain.
    // Pending switch destruction.
    void DestroyPendingSwitch();

private:
    AkSwitchPlayCmd             m_cmdPlay;
    AkSwitchStopCmd             m_cmdStop;

    AkInt32                     m_iSwitchSyncTime;

	bool						m_bIsFromTransSegment;

};

//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchCtx
// Desc: Music Switch Container Context.
//-----------------------------------------------------------------------------
class CAkMusicSwitchCtx : public CAkMatrixAwareCtx
                         ,public CAkSwitchAware
{
public:
    CAkMusicSwitchCtx(
        CAkMusicSwitchCntr *in_pSwitchNode,
        CAkMusicCtx *       in_pParentCtx
        );
    virtual ~CAkMusicSwitchCtx();

    AKRESULT Init(
        CAkRegisteredObj *  in_GameObject,
        UserParams &        in_rUserparams,
        AkUInt32	        in_ulGroupID,
        AkGroupType         in_eGroupType,
        CAkSegmentBucket *& out_pFirstBucket
        );

	// MatrixAware::EndInit override. Called by parent (switches): completes the initialization.
	void EndInit();

    // Switch Aware interface
    // ----------------------
    virtual void SetSwitch( 
        AkSwitchStateID in_switchID, 
        CAkRegisteredObj * in_pGameObj = NULL 
        );

    // Music Context interface
    // ----------------------

    // Override OnPlayed(): Matrix Aware contexts propagate IsPlaying property.
	virtual void OnPlayed(
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
		);
    // Override MusicCtx OnStopped: Need to flush actions to release children, and release ourselves.
    virtual void OnStopped( 
		AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
        );


	// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
	virtual void PerformDelayedSwitchChange();


    // Matrix Aware Context interface
    // ----------------------
    
    // Sequencer access.
    virtual void Process(
		AkInt32 in_iFrameDuration				// Number of samples to process.
		);
    virtual void ProcessRelative(
        AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
		AkInt32 in_iFrameDuration				// Number of samples to process.
        );

    // For Music Renderer's music contexts look-up.
    virtual CAkMusicNode * Node();

    virtual CAkSegmentChain * GetActiveChain();

	// Query for transition reversal. Asks active chain if it is not too late to start playing again.
	virtual bool CanRestartPlaying();
	// Revert playback commands that were disabled. Only possible if it can restart playing.
	// Reverts playbacks of active chain.
	virtual void RestartPlaying();


    // Callback from pending switch transitions.
    // The input parameter becomes the active switch. Furthermore, it is enqueued in the list of processable 
    // children for normal frame behavioral processing.
    void Sync();

    // Helpers.
private:

	CAkMatrixAwareCtx * GetActiveChildContext() { return (*m_itActiveSwitch)->Switchee(); }

    // Gets child node to switch to (by asking the SwitchCntr node), and
    // schedule a musical transition.
    CAkSegmentBucket * ComputeSwitchChange(
        AkSwitchStateID in_switchID
        );

    // Schedule all transition actions required for switch change.
    void ScheduleSwitchTransition(
		AkUniqueID			in_destinationID,	// Node ID of destination.
        CAkSegmentBucket *& io_pNewFirstBucket,	// First sequenced segment at leaf.
        CAkMatrixAwareCtx *& io_pNewContext		// New context. Can be destroyed internally if we decide not to schedule it.
        );

	// Definition of structure for transition parameters passing between helpers.
    struct AkMusicSwitchTransitionData
    {
        AkInt32       iSegmentTimeToSync;		// Sync time relative to in_pSourceBucket (Entry Marker). 0 if no segment in in_pPreviousSequencedItem.
        AkInt32       iCumulBucketsDuration;	// Look-ahead time. Time between current segment's Entry and in_pSourceBucket's Sync time.
        AkInt32       iDestinationLookAhead;	// Destination look-ahead.
		bool		  bIsFromTransSegment;	  // True when transition's switchee is the second one of a transition that involves a transition segment.
        AkMusicSwitchTransitionData()
            :bIsFromTransSegment( false )
        {}
    };

    // Computes and fills a transition data structure given a transition rule and a bucket iterator.
    // Prepares destination.
    // Finds a Sync point.
    // Returns AK_Success if a music transition can be scheduled (found a Sync point), AK_Fail otherwise.
    AKRESULT ComputeTransitionData(
        const AkMusicTransitionRule & in_rule,		// Transition rule.
        const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
        CAkMatrixAwareCtx * in_pDestinationContext,	// Destination context.
        AkMusicSwitchTransitionData & io_transData,	// Transition data.        
		bool    in_bDoForceSucceed					// Force succeed flag: Sync point will be found by incrementing look-ahead value herein.
        );
        
    // Schedule a valid music transition: Create a pending switch transition
    // and attach Play and Stop commands.
    CAkPendingSwitchTransition * ScheduleTransition(
        const AkMusicTransitionRule & in_rule,      // Transition rule.
        const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
		CAkMatrixAwareCtx * in_pSwitchee,			// Child context corresponding to the destination transition.
        const AkMusicSwitchTransitionData & in_transData	// Transition data.
        );

    // Creates next node context.
    CAkMatrixAwareCtx * CreateDestinationContext(
		AkUniqueID in_ID,
		CAkSegmentBucket *& out_pDestBucket
		);
	CAkMatrixAwareCtx * CreateMusicContext(
		AkUniqueID in_ID,
		CAkSegmentBucket *& out_pDestBucket
		);

    // Get the appropriate transition rule.
    const AkMusicTransitionRule & GetTransitionRule( 
        const CAkMatrixSequencer::Iterator & in_itSrc,	// Matrix iterator pointing the source bucket.
		CAkMatrixAwareCtx * in_pDestContext,			// Destination matrix aware context.
		CAkSegmentBucket *& io_pDestSeqBucket, 			// Destination bucket. IO: Could be swapped if rule requires a sequence JumpTo.
		AkUInt32 &			io_uSrcSegmentLookAhead 	// Src look-ahead limit. Passed this limit, use panic rule.
        );

    CAkPendingSwitchTransition * CreateSwitch(
        const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
        CAkMatrixAwareCtx * in_pSwitchee,           // Switchee.
        const AkMusicSwitchTransitionData & in_transData	// Transition data.
        );

	// Child contexts / Transitions management.
	AKRESULT EnqueueFirstContext( 
		CAkMatrixAwareCtx * in_pCtx
		);

	// Determines whether a new transition must be enqueued (handles "Continue to play" behavior).
	bool IsSwitchTransitionNeeded(
		AkUniqueID in_nextNodeID
		);

	// Handle current transition(s) reversal.
	// Iterator and iCumulBucketsDuration will reflect the starting bucket data if the upcoming transition
	// needs to be enqueued later than from the current bucket.
	// Returns true if new transition should be scheduled, false otherwise. 
	// Note. The new transition might not be scheduled if, after reverting some transitions, we find out that the
	// new effective destination is the same as what was requested by the new switch, and the container has the
	// "Continue to play" property.
	typedef AkArray<CAkPendingSwitchTransition*, CAkPendingSwitchTransition*, ArrayPoolDefault, LIST_POOL_BLOCK_SIZE/sizeof(CAkPendingSwitchTransition*)> TransitionsArray;
	// Returns True if the iterator returned should be incremented by re-enqueueing the first transition reverted.
	// This occurs when the second transition of a duo (transitions to and from a transition segment) is reverted,
	// and not the first one.
	bool HandleTransitionsReversal( 
		CAkMatrixSequencer::Iterator & io_itSrc,// Matrix iterator pointing the current bucket, updated therein if applicable.
		AkInt32	& io_iCumulBucketsDuration,		// Cumulative buckets duration (0), updated therein if applicable.
		TransitionsArray & out_transitionsToRevert	// Returned array of transitions to revert.
		);
	
	// Returns true if this context or any of its ascendents has a pending transition.
	bool HasOrAscendentHasPendingTransition();

	// Calls CAkMatrixAwareCtx::PerformDelayedSwitchChange() for all its children.
	void TryPropagateDelayedSwitchChange();

    CAkPendingSwitchTransition * EnqueueTransition( 
        CAkPendingSwitchTransition * in_pNewTransition
        );

	// Prepare iterator so that it points to the end of the specified transition (to its switchee).
	void PrepareIteratorToEndOfTransition( 
		CAkPendingSwitchTransition *	in_pTransition,				// Transition.
		CAkMatrixSequencer::Iterator &	out_it,						// Returned iterator.
		AkInt32 &						out_iCumulBucketsDuration	// Returned time before out_it becomes active.
		);

private:

    CAkMusicSwitchCntr *        m_pSwitchCntrNode;
    AkUniqueID                  m_targetSwitchID;
    AkGroupType					m_eGroupType;
	AkUniqueID                  m_delayedSwitchID;	// Switch ID of switch change that was delayed by parent transition.

	// Transitions queue.
	typedef CAkList2<CAkPendingSwitchTransition*, CAkPendingSwitchTransition*, AkAllocAndFree> TransitionsQueue;
    TransitionsQueue			m_queueTransitions;
    
    // Current active switch.
	typedef TransitionsQueue::Iterator TransQueueIter;
    TransQueueIter				m_itActiveSwitch;

	bool						m_bWasReferenced;
};

#endif //_MUSIC_SWITCH_CTX_H_
