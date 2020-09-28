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
// AkMusicSwitchCtx.cpp
//
// Music switch cntr context.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicSwitchCtx.h"
#include "AkMusicSwitchCntr.h"
#include "AkSegmentCtx.h"
#include "AkMusicSegment.h"
#include "AkSequenceCtx.h"
#include "AkMusicStructs.h"
#include "AkMusicRenderer.h"
#include "AkMusicRanSeqCntr.h"
#include "AkMonitor.h"

#define SWITCH_TRANSITION_QUEUE_POOL_ID	(g_DefaultPoolId)
#define MIN_NUM_PENDING_TRANSITIONS     (LIST_POOL_BLOCK_SIZE/sizeof(CAkPendingSwitchTransition*))
#define MAX_SRC_SEGMENT_LOOK_AHEAD		(64)	// Maximum number of segments of the current chain that 
												// can be looked-ahead in order to schedule a transition.
#define AK_INT_MIN						(-2147483647 - 1)

//-----------------------------------------------------------------------------
// Name: CAkNullCtx
// Desc: Matrix aware context that implement an empty sequence.
//-----------------------------------------------------------------------------
CAkNullCtx::CAkNullCtx(
    CAkMusicCtx *   in_parent
    )
:CAkMatrixAwareCtx( in_parent )
,m_bWasReferenced( false )
{
}

CAkNullCtx::~CAkNullCtx()
{
    CAkSegmentChain::Term();
}

AKRESULT CAkNullCtx::Init(
    CAkRegisteredObj *  in_pGameObj,
    UserParams &    in_rUserparams,
    CAkMatrixSequencer * in_pSequencer,
    CAkSegmentBucket *& out_pFirstBucket    // First bucket (null segment).
    )
{
    if ( CAkMatrixAwareCtx::Init( in_pGameObj, in_rUserparams ) == AK_Success )
    {
        if ( CAkNullChain::Init( out_pFirstBucket ) == AK_Success )
		{
			AddRef();
			m_bWasReferenced = true;
			return AK_Success;
		}
    }
    return AK_Fail;
}

// MusicCtx override. AddRef and Release.
void CAkNullCtx::OnPlayed(
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
	)
{
    if ( !IsPlaying() )
    {
        CAkMusicCtx::OnPlayed( in_uSubFrameOffset );
    }
}

void CAkNullCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
    if ( IsPlaying() )
    {
        CAkMusicCtx::OnStopped( in_uSubFrameOffset );
	}

	ClearChain();

	if ( m_bWasReferenced )
	{
		m_bWasReferenced = false;
        Release();
    }
}

void CAkNullCtx::Process(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
    ProcessChainItems( in_iFrameDuration );
}

void CAkNullCtx::ProcessRelative(
    AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
}

CAkSegmentChain * CAkNullCtx::GetActiveChain()
{
    return this;
}
bool CAkNullCtx::IsNullCtx()
{
	return true;
}

bool CAkNullCtx::CanRestartPlaying()
{
	return true;
}

// Revert playback commands that were disabled. Only possible if it can restart playing.
// Default implementation does nothing.
void CAkNullCtx::RestartPlaying()
{
	AKASSERT( CanRestartPlaying() );
}

// For Music Renderer's music contexts look-up: concrete contexts must return their own node.
CAkMusicNode * CAkNullCtx::Node()
{
    return NULL;
}

CAkMatrixAwareCtx * CAkNullCtx::MatrixAwareCtx()
{
    return this;
}


//-----------------------------------------------------------------------------
// Name: CAkPendingSwitch
// Desc: Pending switch. The switch context keeps a list of all pending switches 
//       (either active or obsolete, but kept alive). They are referenced as 
//       branch items in segment chains, for complex segment paths management.
//-----------------------------------------------------------------------------

CAkPendingSwitchTransition::CAkPendingSwitchTransition(
    AkInt32                 in_iRelativeTime,
    CAkMusicSwitchCtx *     in_pOwner,
    CAkMatrixAwareCtx *     in_pSwitchee,
	bool					in_bIsFromTransSegment
    )
:CAkBranchItem( in_iRelativeTime, in_pOwner, in_pSwitchee )
,m_iSwitchSyncTime( 0 )
,m_bIsFromTransSegment( in_bIsFromTransSegment )
{
    m_cmdPlay.bPending = false;
    m_cmdStop.pTarget = NULL;
    m_cmdStop.bPending = false;
}

CAkPendingSwitchTransition::~CAkPendingSwitchTransition()
{
    AKASSERT( !m_cmdPlay.bPending &&
              !m_cmdStop.bPending );
}

void CAkPendingSwitchTransition::ReleaseContext()
{
	m_cmdPlay.bPending = false;
	m_cmdStop.bPending = false;
	ReleaseSwitchee();
}

// Ask permission to remove from chain. Ask only after it has already synched.
bool CAkPendingSwitchTransition::CanUnlink()
{
    AKASSERT( SyncTime() < 0 );
    return !m_cmdPlay.bPending && !m_cmdStop.bPending;
}

// Transition reversal.
bool CAkPendingSwitchTransition::CanBeReverted()
{
	// Can be reverted if its switchee has not started playing yet, and the Stop command's target
	// can restart playing.
	AKASSERT( Switchee() );
	return !( Switchee()->IsPlaying() ) && m_cmdStop.pTarget->CanRestartPlaying();
}

// Branch item is considered as reverted when it has no more switchee.
void CAkPendingSwitchTransition::SetAsReverted()
{
	// Explicitly stop the destination: reverted transition destination 
	// contexts have not started to play yet, but they might have created
	// and scheduled a whole bunch of children. Propagate STOP command to
	// ensure that the sub hierarchy is properly released.
	if ( Switchee() )
		Switchee()->OnStopped( 0 );

	m_cmdPlay.bPending = false;
	ReleaseSwitchee();
}

void CAkPendingSwitchTransition::RevertTransition(
    CAkPendingSwitchTransition * in_pNewTransition,		// New transition, to compute fade-in time. Can be NULL.
	bool in_bRestartPlayingPreviousCtx
)
{
    // Switchee should already have been cleared.
	// "Set as reverted" now if not already done (should have been done, unless it was a conditional revert,
	// depending upon successfully finding a Sync point in the previous transition segment).
	SetAsReverted();

	if ( in_bRestartPlayingPreviousCtx )
	{   
		// Re-fadein stopping context.

		// Context to ressurect should be there.
		AKASSERT ( m_cmdStop.pTarget );
	

		// ISSUE: Negative fade out offsets: Reference to Stop target is not held by transition.
		// Although we might protect our pointer from destruction, all its children would be 
		// stopped. 
		// Negative fade out offsets have been disabled for Wwise 2007.1.

		// ~negative if already occurred.
		if ( !m_cmdStop.bPending )
		{
			AKASSERT( m_cmdStop.pTarget->IsPlaying() );
			//if ( m_cmdStop.pTarget->IsPlaying() )
			{
				// Not possible that m_cmdStop.transParams.TransitionTime != 0 if !pending && playing.
				AKASSERT( m_cmdStop.transParams.TransitionTime > 0 );

				// Rule for fade-in time : 
				// Take the min value between new transition time and time elapsed since beginning of fade-out.

				AkMusicFade revFadeParams;
				AkInt32 iFadeOutTimeElapsed = -( m_cmdStop.iRelativeTime + m_iSwitchSyncTime );
				AKASSERT( iFadeOutTimeElapsed >= 0 );
				if ( !in_pNewTransition ||
					 iFadeOutTimeElapsed < in_pNewTransition->SwitchSyncTime() )
				{
					revFadeParams.transitionTime = CAkTimeConv::SamplesToMilliseconds( iFadeOutTimeElapsed );
				}
				else
				{
					revFadeParams.transitionTime = CAkTimeConv::SamplesToMilliseconds( in_pNewTransition->SwitchSyncTime() );
				}

				revFadeParams.eFadeCurve = m_cmdStop.transParams.eFadeCurve;
				revFadeParams.iFadeOffset = 0;
				m_cmdStop.pTarget->_Play( revFadeParams );
			}
			/**
			else
			{
				// Current context has already stopped!
				AKASSERT( !"Not implemented" );
			}
			**/
		}
		else
		{
			// Stop command was not yet executed. Disable it.
			m_cmdStop.bPending = false;
		}

		m_cmdStop.pTarget->RestartPlaying();
	}
	else
	{
		m_cmdStop.bPending = false;
	}
}

// Most of the time the pending switch is the property of the switch context. Therefore this call simply
// marks it as unlinked, so that it can be cleaned up later by the switch context.
// However, if this transition is reverted, it is dequeued from the switch context's transition list, and
// therefore becomes the property of the chain, which will destroy it through a call to this method.
void CAkPendingSwitchTransition::DestroySegmentChainItem()
{
    // Does not destroy. Branch items are not the property of the bucket chain.
    TagAsUnlinked();

	// UNLESS transition was reverted.
	// Transition is reverted if and only if the Switchee was released.
	if ( !Switchee() )
		DestroyPendingSwitch();
}

// Commands scheduling.
void CAkPendingSwitchTransition::AttachPlayCmd(
    const AkMusicFade &     in_fadeParams,
    AkInt32                 in_iRelativeTime
    )
{
	m_cmdPlay.fadeParams = in_fadeParams;

	// Do not bother fading in when Switchee is a null context.
	AKASSERT( Switchee() );
	if ( Switchee()->IsNullCtx() )
	{
		m_cmdPlay.fadeParams.transitionTime = 0;
		AKASSERT( in_iRelativeTime == 0 );	// A null context should not require any look-ahead.
	}

    // Adjust fade offset relatively to sync time if there is a fade (transition time > 0).
	if ( in_fadeParams.transitionTime > 0 )
		m_cmdPlay.fadeParams.iFadeOffset -= in_iRelativeTime;		

    m_cmdPlay.iRelativeTime = in_iRelativeTime;
    m_cmdPlay.bPending      = true;

    AKASSERT( in_iRelativeTime >= -SwitchSyncTime() );
    if ( in_iRelativeTime < EarliestActionTime() )
        EarliestActionTime( in_iRelativeTime );
}

void CAkPendingSwitchTransition::AttachStopCmd(
    CAkMatrixAwareCtx*      in_pTarget,
    AkTimeMs                in_iTransDuration,
    AkCurveInterpolation	in_eFadeCurve,
    AkInt32                 in_iRelativeTime
    )
{
    m_cmdStop.transParams.TransitionTime = in_iTransDuration;
    m_cmdStop.transParams.eFadeCurve = in_eFadeCurve;
    m_cmdStop.iRelativeTime = in_iRelativeTime;
    m_cmdStop.pTarget       = in_pTarget;
    m_cmdStop.bPending      = true;

    AKASSERT( in_iRelativeTime >= -SwitchSyncTime() );
    if ( in_iRelativeTime < EarliestActionTime() )
        EarliestActionTime( in_iRelativeTime );
}

// Process switch transition: Notifies Sync, RelativeProcess()es switchee before sync,
// executes high-level transition commands (play and stop on higher-level contexts).
// Consumes absolute switch time.
void CAkPendingSwitchTransition::ProcessSwitchTransition(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
	// Play command.
    if ( m_cmdPlay.bPending &&
         m_cmdPlay.iRelativeTime + m_iSwitchSyncTime >= 0 &&
         m_cmdPlay.iRelativeTime + m_iSwitchSyncTime < in_iFrameDuration )
    {
		AKASSERT( Switchee() );
        Switchee()->_Play( m_cmdPlay.fadeParams, m_cmdPlay.iRelativeTime + m_iSwitchSyncTime );
        m_cmdPlay.bPending = false;
    }

    // Stop command.
    if ( m_cmdStop.bPending &&
         m_cmdStop.pTarget &&
         m_cmdStop.iRelativeTime + m_iSwitchSyncTime >= 0 &&
         m_cmdStop.iRelativeTime + m_iSwitchSyncTime < in_iFrameDuration )
    {
        m_cmdStop.pTarget->_Stop( m_cmdStop.transParams, m_cmdStop.iRelativeTime + m_iSwitchSyncTime );
        m_cmdStop.bPending = false;
    }


	// Process switchee if it is still playing.
	if ( Switchee() )
	{

		// Relative process it whenever m_iSwitchSyncTime is POSITIVE (that is, sync did not occur yet). Otherwise
		// process it.
		if ( m_iSwitchSyncTime >= 0 )
		{
			// Process sync...
			if ( m_iSwitchSyncTime < in_iFrameDuration )
			{
				// SYNC FRAME! 
				// Relative process until frame, then notify switch owner (switchee will become its new active context),
				// then Process() it for the remaining time.

				Switchee()->ProcessRelative( m_iSwitchSyncTime, m_iSwitchSyncTime );

				m_pSwitchOwner->Sync();

				Switchee()->Process( in_iFrameDuration - m_iSwitchSyncTime );
			}
			else
			{
				// Process non-synched context, and add in exclude list.
				Switchee()->ProcessRelative( m_iSwitchSyncTime, in_iFrameDuration );
			}
		}
		else
		{
			Switchee()->Process( in_iFrameDuration );
		}
	}

    // Consume switch absolute time.
    // In order to avoid int32 wrap across INT_MAX, stop decrementing it when it is not needed:
    // 1) Sync was reached.
    // 2) No more pending play or stop command (ensure that stop fade-out has completed to properly handle fade-out reversal).
	/** Hotfix WG-12009, WG-12010: Cannot use m_cmdStop.pTarget to determine whether the previous context has stopped
		playing. After m_cmdStop.pTarget->_Stop() was called, the context that is pointed to may self-destroy at any moment.
		When that occurs, m_cmdStop.pTarget points to dead memory.
		REVIEW referencing of stop targets in transitions.
		In the meantime, we just need to ensure that we do not wrap (INT_MAX is greater than any human-editable fade out).
	if ( m_iSwitchSyncTime >= 0 
		|| ( m_cmdStop.pTarget && m_cmdStop.pTarget->IsPlaying() ) 
		|| m_cmdPlay.bPending )
		**/
	if ( m_iSwitchSyncTime > AK_INT_MIN+AK_NUM_VOICE_REFILL_FRAMES )
	{
    	m_iSwitchSyncTime -= in_iFrameDuration;
	}
}

void CAkPendingSwitchTransition::SwitchSyncTime(
    AkInt32 in_iSwitchSyncTime
    )
{
    m_iSwitchSyncTime = in_iSwitchSyncTime; 
}

// Can safely destroy pending switch.
bool CAkPendingSwitchTransition::NeedsProcessing()
{
	// Needs processing if it has not already synched, switchee is still
	// playing or it has no pending commands.
	// REVIEW (LX) Destruction mechanism.
    return ( m_iSwitchSyncTime > 0 ||
			 m_cmdStop.bPending ||
			 ( Switchee() &&
			   Switchee()->IsPlaying() ) ||
		     m_cmdPlay.bPending );

	// Note. This can be destroyed if it does not need processing and it is not linked, 
}

// Pending switch destruction.
void CAkPendingSwitchTransition::DestroyPendingSwitch()
{
	ReleaseSwitchee();
    AkDelete( g_DefaultPoolId, this );
}


//-----------------------------------------------------------------------------
// Name: CAkMusicSwitchCtx
// Desc: Music Switch Container Context.
//-----------------------------------------------------------------------------

CAkMusicSwitchCtx::CAkMusicSwitchCtx(
    CAkMusicSwitchCntr *in_pSwitchNode,
    CAkMusicCtx *       in_pParentCtx
    )
:CAkMatrixAwareCtx( in_pParentCtx )
,m_pSwitchCntrNode( in_pSwitchNode )
,m_targetSwitchID( AK_INVALID_UNIQUE_ID )
,m_eGroupType( AkGroupType_Switch )
,m_delayedSwitchID( AK_INVALID_UNIQUE_ID )
,m_bWasReferenced( false )
{
	if( m_pSwitchCntrNode )
		m_pSwitchCntrNode->AddRef();
}

CAkMusicSwitchCtx::~CAkMusicSwitchCtx()
{
    // Destroy all pending transitions.
	if ( m_queueTransitions.IsInitialized() )
	{
		TransitionsQueue::Iterator it = m_queueTransitions.Begin();
		while ( it != m_queueTransitions.End() )
		{
			AKASSERT( !(*it)->Switchee() );
			(*it)->DestroyPendingSwitch();
			++it;
		}
		m_queueTransitions.Term();
	}

	if( m_pSwitchCntrNode )
		m_pSwitchCntrNode->Release();
}

AKRESULT CAkMusicSwitchCtx::Init(
    CAkRegisteredObj *  in_GameObject,
    UserParams &        in_rUserparams,
    AkUInt32	        in_ulGroupID,
    AkGroupType         in_eGroupType,
    CAkSegmentBucket *& out_pFirstBucket
    )
{
	m_eGroupType = in_eGroupType;
    AKRESULT eResult = CAkMatrixAwareCtx::Init( in_GameObject, in_rUserparams );

    // Register to RTPC/State Mgr.
    if ( eResult == AK_Success )
        eResult = SubscribeSwitch( in_ulGroupID, in_eGroupType );

    if ( eResult == AK_Success )
    {
		eResult = m_queueTransitions.Init( MIN_NUM_PENDING_TRANSITIONS, AK_NO_MAX_LIST_SIZE, SWITCH_TRANSITION_QUEUE_POOL_ID );

        if ( eResult == AK_Success )
        {
			// Add ref ourselves. A switch context lives until explicitly stopped (released in OnStopped).
            AddRef();
            m_bWasReferenced = true;

			AkSwitchStateID switchID = GetSwitchToUse( Sequencer()->GameObjectPtr(), 
                    in_ulGroupID,
                    in_eGroupType );

            if ( !m_pParentCtx )
            {
                // Top-level context.

                // Create initial NULL context, and set it as the active child.
                CAkMatrixAwareCtx * pNullCtx = CreateDestinationContext( AK_MUSIC_TRANSITION_RULE_ID_NONE, out_pFirstBucket );
				if ( pNullCtx && 
					 EnqueueFirstContext( pNullCtx ) == AK_Success )
				{
					// Perform switch change.
					// Note. Return switchee's first segment bucket instead of the initial null context,
					// for fade handling by node.
					// Note on errors in ComputeSwitchChange(): in the worse case we play nothing. At least we were
					// able to create the initial Null context.
					CAkSegmentBucket * pFirstBucket = ComputeSwitchChange( switchID );
					// Note. pFirstBucketcan be NULL if ContinueToPlay and first switch is <nothing>.
					if ( pFirstBucket )
						out_pFirstBucket = pFirstBucket;
				}
				else
					eResult = AK_Fail;
            }
            else
            {
                // Child switch context: create our first active child context that corresponds to the switch desired,
                // and return it. Our parent will take care of us.
                // Query our node for the associated child node ID.
                AkUniqueID firstNodeID;
                m_pSwitchCntrNode->GetSwitchNode( switchID, firstNodeID );
                CAkMatrixAwareCtx * pInitialCtx = CreateDestinationContext( firstNodeID, out_pFirstBucket );

				if ( !pInitialCtx ||
					 EnqueueFirstContext( pInitialCtx ) != AK_Success )
				{
					eResult = AK_Fail;
				}
            }

            m_targetSwitchID = switchID;
        }

    }

    return eResult;
}

// Called by parent (switches): completes the initialization.
// Default implementation does nothing.
void CAkMusicSwitchCtx::EndInit()
{
	TransitionsQueue::IteratorEx it = m_queueTransitions.BeginEx();
    while ( it != m_queueTransitions.End() )
    {
		if ( (*it)->Switchee() )
			(*it)->Switchee()->EndInit();
		++it;
	}
}

// Switch Aware interface
// ----------------------
void CAkMusicSwitchCtx::SetSwitch( 
    AkSwitchStateID in_switchID, 
    CAkRegisteredObj * in_pGameObj
    )
{
    // Compute switch change only if notification applies to us.
	if ( in_pGameObj == Sequencer()->GameObjectPtr() || AkGroupType_State == m_eGroupType )  
    {
		if( m_targetSwitchID != in_switchID )
		{
			if ( m_pParentCtx && 
				 static_cast<CAkMusicSwitchCtx*>(m_pParentCtx)->HasOrAscendentHasPendingTransition() )
			{
				m_delayedSwitchID = in_switchID;
			}
			else
			{
				ComputeSwitchChange( in_switchID );
			}
			m_targetSwitchID = in_switchID;
		}
    }
}

// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
void CAkMusicSwitchCtx::PerformDelayedSwitchChange()
{
	if ( m_delayedSwitchID != AK_INVALID_UNIQUE_ID )
	{
		ComputeSwitchChange( m_delayedSwitchID );
	}

	// Propagate to children if this nor any ascendent has a pending transition.
	TryPropagateDelayedSwitchChange();
}


// Helper: Gets child node to switch to (by asking the SwitchCntr node), and
// schedule a musical transition.
CAkSegmentBucket * CAkMusicSwitchCtx::ComputeSwitchChange(
    AkSwitchStateID in_switchID
    )
{
	// Clear delayed switch ID.
	m_delayedSwitchID = AK_INVALID_UNIQUE_ID;

    // Query our node for the associated child node ID.
    AkUniqueID nextNodeID;
    m_pSwitchCntrNode->GetSwitchNode( in_switchID, nextNodeID );

	if ( m_pSwitchCntrNode->ContinuePlayback() &&
		 !IsSwitchTransitionNeeded( nextNodeID ) )
	{
		return NULL;
	}
	
	// Create next node context.
	CAkMatrixAwareCtx * pNewContext;
	CAkSegmentBucket * pFirstBucket;
	pNewContext = CreateDestinationContext( nextNodeID, pFirstBucket );
	// Schedule.
	if ( pNewContext )
	{
		ScheduleSwitchTransition( nextNodeID, pFirstBucket, pNewContext );

		// NOTE: ScheduleSwitchTransition() could decide not to enqueue a new transition. In such a case (io_)pNewContext
		// would be destroyed and set to NULL therein.
		if ( pNewContext )
		{
			pNewContext->EndInit();
			AKASSERT( pFirstBucket );
			return pFirstBucket;
		}			
	}
	return NULL;
}

// Determines whether a new transition must be enqueued (handles "Continue to play" behavior).
bool CAkMusicSwitchCtx::IsSwitchTransitionNeeded(
	AkUniqueID in_nextNodeID
	)
{
	AKASSERT( m_queueTransitions.Last()->Switchee() );
	CAkMusicNode * pCurTargetNode = m_queueTransitions.Last()->Switchee()->Node();
	if ( ( pCurTargetNode && pCurTargetNode->ID() == in_nextNodeID ) ||
		 ( !pCurTargetNode && in_nextNodeID == AK_MUSIC_TRANSITION_RULE_ID_NONE ) )
	{
		return false;
	}
	return true;
}

// Sequencer access.
void CAkMusicSwitchCtx::Process(
	AkInt32 in_iFrameDuration				// Number of samples to process.
	)
{
    AKASSERT( m_itActiveSwitch != m_queueTransitions.End() );

	TransitionsQueue::IteratorEx it = m_queueTransitions.BeginEx();
    while ( it != m_queueTransitions.End() )
    {
        CAkPendingSwitchTransition * pPendingTransition = (*it);

        // Process transition.
		if ( pPendingTransition->NeedsProcessing() )
		{
			pPendingTransition->ProcessSwitchTransition( in_iFrameDuration );
			++it;
		}
		else if ( !pPendingTransition->IsLinked() &&
				  pPendingTransition != (*m_itActiveSwitch) )
		{
	        // Pending transitions clean up.
			// Only if it is unlinked AND it is not the active switch item.
            it = m_queueTransitions.Erase( it );
            pPendingTransition->DestroyPendingSwitch();
        }
        else
            ++it;
    }
}
void CAkMusicSwitchCtx::ProcessRelative(
    AkInt32 in_iSyncTime,					// Sync time to use (controlled from above).
	AkInt32 in_iFrameDuration				// Number of samples to process.
    )
{
    // Propagate processing down.
	// Note. Slave contexts (powered by parent, through ProcessRelative()) cannot have more
	// than one pending transition, because 
	// 1) the parent switch is obviously currently performing a transition;
	// 2) no transition can be enqueued on a child switch before its parent reached steady state
	// (when a set switch notification occurs, the child switch stored the target ID in m_delayedSwitchID,
	// to be scheduled later. See SetSwitch()).
	// Also, it cannot have any playing stinger since it has never been in the Matrix Sequencer's active chain.
    AKASSERT( NumChildren() == 1 &&
              m_queueTransitions.Length() == 1 );

	AKASSERT( (*m_itActiveSwitch) == m_queueTransitions.First() );
	AKASSERT( GetActiveChildContext() );
	GetActiveChildContext()->ProcessRelative( in_iSyncTime, in_iFrameDuration );
}

// Context commands
//

// Override OnPlayed(): Propagate play command to its "first enqueued context" (the slave context,
// controlled through ProcessRelative()). Further OnPlayed commands are issued from CAkPendingTransitions.
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicSwitchCtx::OnPlayed(
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
	)
{
	// If we are already playing, it means that we got restarted from a switch reversal:
	// Nothing to do. We are in steady-state and the playback of children is already managed by transitions queue.
	if ( !IsPlaying() )
	{
		CAkMusicCtx::OnPlayed( in_uSubFrameOffset );

		// Propagate the command to slave child.
		// Note. If this is a top-level context, it will typically have 2 transitions: the first one pointing to the Null Ctx
		// (to be OnPlayed()), the second one being the transition scheduled in Init() towards the target context that
		// corresponds to the switch-child that was required at that time. However it can have only 1 transition if 
		// ComputeSwitchChange() decided that no transition was needed (for example, when switching from <nothing> to <nothing>).
		// Otherwise, there is only one transition. It can also have 3 transitions if it begins with a transition segment.
		AKASSERT( ( m_pParentCtx && m_queueTransitions.Length() == 1 ) || 
				  ( !m_pParentCtx && m_queueTransitions.Length() <= 3 ) );
		AKASSERT( (*m_itActiveSwitch) == m_queueTransitions.First() );
		AKASSERT( GetActiveChildContext() );
		GetActiveChildContext()->OnPlayed( in_uSubFrameOffset );
	}
}

// Override MusicCtx OnStop: Need to flush actions to release children.
void CAkMusicSwitchCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
	// Do not destroy pending transitions: children might reference them. They will be flushed in ~.
	CAkMusicCtx::OnStopped( in_uSubFrameOffset );

	// Clear and release all child processes.
	if ( m_queueTransitions.IsInitialized() )
	{
		TransitionsQueue::Iterator it = m_queueTransitions.Begin();
		while ( it != m_queueTransitions.End() )
		{
			(*it)->ReleaseContext();
			++it;
		}
	}
	UnsubscribeSwitches();
	if ( m_bWasReferenced )
	{
		m_bWasReferenced = false;
		Release();
	}
}


// For Music Renderer's music contexts look-up.
CAkMusicNode * CAkMusicSwitchCtx::Node()
{
    return m_pSwitchCntrNode;
}

CAkSegmentChain * CAkMusicSwitchCtx::GetActiveChain()
{
	CAkMatrixAwareCtx * pActiveCtx = GetActiveChildContext();
	// NOTE (WG-9576) There is always an active child context, unless we are in the process of stopping,
	// with cross-references among switch containers, and someone uses a MatrixIterator (to reschedule a 
	// delayed state change, for example). 
	if ( pActiveCtx )
		return pActiveCtx->GetActiveChain();
	return NULL;
}

bool CAkMusicSwitchCtx::CanRestartPlaying()
{
	AKASSERT( GetActiveChildContext() );
	return GetActiveChildContext()->CanRestartPlaying();
}

// Revert playback commands that were disabled. Only possible if it can restart playing.
// Reverts playbacks of active chain.
void CAkMusicSwitchCtx::RestartPlaying()
{
	AKASSERT( GetActiveChildContext() );
	return GetActiveChildContext()->RestartPlaying();
}

// Callback from pending switch transitions.
void CAkMusicSwitchCtx::Sync()
{
    // Sync: Increment active switch iterator.
	++m_itActiveSwitch;

	TryPropagateDelayedSwitchChange();
}

void CAkMusicSwitchCtx::TryPropagateDelayedSwitchChange()
{
	// If there is no more pending transition in this context and in any ascendent, trigger possibly delayed switch changes on child.
	if ( !HasOrAscendentHasPendingTransition() )
	{
		TransQueueIter itTrans = m_queueTransitions.Begin();
		while ( itTrans != m_queueTransitions.End() )
		{
			CAkMatrixAwareCtx * pChild = (*itTrans)->Switchee();
			if ( pChild && pChild->IsPlaying() )	// Child might just have been stopped, without having been flushed yet.
				pChild->PerformDelayedSwitchChange();
			++itTrans;
		}
	}
}

// Creates next node context.
// Returns the leaf descendant first Bucket. 
// Cannot be NULL (but inner context can be NULL if a child switch is setup to <NOTHING>)
CAkMatrixAwareCtx * CAkMusicSwitchCtx::CreateDestinationContext(
    AkUniqueID in_ID,
    CAkSegmentBucket *& out_pDestBucket
    )
{
	CAkMatrixAwareCtx * pNewContext = NULL;
    out_pDestBucket = NULL;
    if ( in_ID != AK_MUSIC_TRANSITION_RULE_ID_NONE )
    {
		pNewContext = CreateMusicContext( in_ID, out_pDestBucket );
    }
    
	// Create a null context if ID is <nothing>, or if there was an error.
	if ( in_ID == AK_MUSIC_TRANSITION_RULE_ID_NONE ||
		 !pNewContext )
    {
        CAkNullCtx * pNullCtx = AkNew( g_DefaultPoolId, CAkNullCtx( this ) );
        if ( pNullCtx )
        {
			pNullCtx->AddRef();
			if ( pNullCtx->Init( Sequencer()->GameObjectPtr(),
                                  Sequencer()->GetUserParams(),
                                  Sequencer(),
                                  out_pDestBucket ) == AK_Success )
			{
				pNullCtx->Release();
			}
			else
            {
				pNullCtx->OnStopped( 0 );
				pNullCtx->Release();
                pNullCtx = NULL;
            }
			
        }
        pNewContext = pNullCtx;
    }

    return pNewContext;
}

CAkMatrixAwareCtx * CAkMusicSwitchCtx::CreateMusicContext(
	AkUniqueID in_ID,
	CAkSegmentBucket *& out_pDestBucket
	)
{
	CAkMatrixAwareCtx * pNewContext = NULL;
    out_pDestBucket = NULL;

	CAkMusicNode * pNewNode = static_cast<CAkMusicNode*>(g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_ID ));
    if ( pNewNode )
	{
		// TODO Optim. Avoid passing GameObj and Params to child context.
		pNewContext = pNewNode->CreateContext(
			this,
			Sequencer()->GameObjectPtr(),
			Sequencer()->GetUserParams(),
			out_pDestBucket );
		pNewNode->Release();
	}

	return pNewContext;
}

// Schedule transition actions for switch change.
void CAkMusicSwitchCtx::ScheduleSwitchTransition(
	AkUniqueID			in_destinationID,	// Node ID of destination.
    CAkSegmentBucket *& io_pNewFirstBucket,	// First sequenced segment at leaf.
    CAkMatrixAwareCtx *& io_pNewContext 	// New context. Can be destroyed internally if we decide not to schedule it.
    )
{
    AKASSERT( io_pNewFirstBucket &&
              io_pNewContext );
    
	AKASSERT( !m_pParentCtx ||
			  !static_cast<CAkMusicSwitchCtx*>(m_pParentCtx)->HasOrAscendentHasPendingTransition() );

    // Get current segment bucket.
    CAkMatrixSequencer::Iterator it = Sequencer()->GetCurBucket( this );
    if ( it == Sequencer()->End() )
	{
		AKASSERT( !"Trying to schedule a transition while the switch context is in an inconsistent state (probably stopping)" );
		// Destroy destination and bail out.
		io_pNewContext->OnStopped( 0 );
		io_pNewContext = NULL;
		return;
	}


    // Get position of current segment and create transition data structure.
    AkMusicSwitchTransitionData transData;

	transData.iCumulBucketsDuration = 0;


	// Handle current transition(s) reversal.
	// Iterator and iCumulBucketsDuration will reflect the starting bucket data if the upcoming transition
	// needs to be enqueued later than from the current bucket.
	CAkPendingSwitchTransition * pFirstNewTransition = NULL;
	TransitionsArray transitionsToRevert;
    bool bIsFirstRevertConditional = HandleTransitionsReversal( it, transData.iCumulBucketsDuration, transitionsToRevert );

	// Reconsider scheduling new transition after handling reversals.
	if ( transitionsToRevert.IsEmpty() || 
		 IsSwitchTransitionNeeded( in_destinationID ) )
	{    
		// Yes. Proceed.

		//
		// Find transition sync point.
		// 

		AKRESULT eReturnVal;
		AkUInt32 uSrcSegmentLookAhead = 0;	// Used for transition length limit.
		bool bFailedEnqueueingTransition = false;
		do
		{

			// Query transition rule.
			const AkMusicTransitionRule & rule = GetTransitionRule( 
				it, 
				io_pNewContext,
				io_pNewFirstBucket,
				uSrcSegmentLookAhead );

			CAkMatrixAwareCtx * pTransCtx = NULL;
			CAkSegmentBucket * pTransBucket;
			if ( rule.pTransObj )
			{
				// Create transition segment context.				
				pTransCtx = CreateMusicContext( rule.pTransObj->segmentID, pTransBucket );
			}

			if ( pTransCtx )
			{
				// Create a rule for current segment to transition segment.
				AkMusicTransitionRule transRule1;
				transRule1.srcRule = rule.srcRule;
				transRule1.pTransObj = NULL;
				transRule1.destRule.fadeParams = rule.pTransObj->fadeInParams;
				transRule1.destRule.markerID = AK_INVALID_UNIQUE_ID;
				transRule1.destRule.eEntryType = EntryTypeEntryMarker;
				transRule1.destRule.bPlayPreEntry = rule.pTransObj->bPlayPreEntry;

				

				// Compute transition data for first transition, try to find a point to schedule the musical transition.
				eReturnVal = ComputeTransitionData( 
					transRule1, 
					it, 
					pTransCtx, 
					transData,
					false );

				if ( eReturnVal == AK_Success )
				{
					// It worked. Now try to schedule second transition.

					// Create a rule for current segment to transition segment.
					AkMusicTransitionRule transRule2;
					transRule2.srcRule.fadeParams = rule.pTransObj->fadeOutParams;
					transRule2.srcRule.eSyncType = SyncTypeExitMarker;
					transRule2.srcRule.bPlayPostExit = rule.pTransObj->bPlayPostExit;
					transRule2.destRule = rule.destRule;
					// Ignore SameTime transitions with transition segments.
					if ( EntryTypeSameTime == transRule2.destRule.eEntryType )
						transRule2.destRule.eEntryType = EntryTypeEntryMarker;

					// Create a temporary iterator that points to the not yet scheduled transition bucket.
					CAkMatrixSequencer::Iterator itTrans = Sequencer()->GetCurBucket( pTransCtx );

					// Create temporary transition data structure for second transition.
					// Note: transition object is obviously not playing yet.
					AkMusicSwitchTransitionData transData2;
					transData2.bIsFromTransSegment = true;		// Remember that this transition depends upon another.

					// Note: cumulative buckets duration is the one currently used in chain path, plus the relative Exit
					// position of the previous object.
					transData2.iCumulBucketsDuration = (*it)->ConvertToAbsoluteTime( transData.iCumulBucketsDuration + transData.iSegmentTimeToSync );

					// If the source bucket is empty, the second transition MUST succeed.
					// We force it with the bForceSucceed flag. 
					// transData2.iCumulBucketsDuration will be incremented in ComputeTransitionData() so that the
					// transition may occur.
					// Force succeed flag. Set in the special case below.
					bool bForceSucceed = ( (*it)->SegmentCtx() == NULL );

					// Compute transition data for second transition, try to find a point to schedule the musical transition.
					eReturnVal = ComputeTransitionData( 
						transRule2, 
						itTrans, 
						io_pNewContext, 
						transData2,
						bForceSucceed );

					// Push transition segment to occur just before destination if the latter occurs later than
					// after the transition segment active duration.
					if ( bForceSucceed && 
						 transData.iSegmentTimeToSync < ( transData2.iCumulBucketsDuration - transData.iCumulBucketsDuration ) )
					{
						transData.iSegmentTimeToSync = transData2.iCumulBucketsDuration - transData.iCumulBucketsDuration;
					}

					if ( eReturnVal == AK_Success )
					{
						// Both transitions can be executed.
	                    
						//
						// Schedule first.
						//
						pFirstNewTransition = ScheduleTransition( 
							transRule1, 
							it, 
							pTransCtx, 
							transData );
						if ( !pFirstNewTransition )
							bFailedEnqueueingTransition = true;

						//
						// Schedule second.
						//
						if ( !bFailedEnqueueingTransition )
						{
							if ( !ScheduleTransition( 
									transRule2, 
									itTrans, 
									io_pNewContext, 
									transData2 ) )
							{
								bFailedEnqueueingTransition = true;
							}
						}
					}
				}

				if ( eReturnVal != AK_Success )
				{
					// One of the transitions failed. Destroy trans object context.
					pTransCtx->OnStopped( 0 );
				}
			}
			else
			{
				// Compute transition data, try to find a point to schedule the musical transition.
				eReturnVal = ComputeTransitionData( 
					rule, 
					it, 
					io_pNewContext, 
					transData,
					false );

				//
				// Schedule transition.
				//
				if ( eReturnVal == AK_Success )
				{
					// Transition ready to be scheduled. (Do it now because rule is scoped here only).
					pFirstNewTransition = ScheduleTransition( 
						rule, 
						it, 
						io_pNewContext, 
						transData );
					if ( !pFirstNewTransition )
						bFailedEnqueueingTransition = true;
				}
			}
	        
			if ( eReturnVal != AK_Success )
			{
				AKASSERT( !bFailedEnqueueingTransition );

				// If there was a linked transition that was pushed in the reversals list, push it back into 
				// the list of pending transitions, and compute the new iterator with it.
				if ( bIsFirstRevertConditional )
				{
					bIsFirstRevertConditional = false;

					TransitionsArray::Iterator itRevTransToReenqueue = transitionsToRevert.Begin();
					AKASSERT( itRevTransToReenqueue != transitionsToRevert.End() );

					CAkPendingSwitchTransition * pReenqueuedTransition = EnqueueTransition( (*itRevTransToReenqueue) );
					if ( pReenqueuedTransition )
					{
						transitionsToRevert.Erase( itRevTransToReenqueue );

						// Actualize iterator.
						PrepareIteratorToEndOfTransition( 
							pReenqueuedTransition, 
							it, 
							transData.iCumulBucketsDuration );
					}
					else
					{
						// Failed re-enqueueing transition!
						// Other tries will obviously fail: bail out.
						bFailedEnqueueingTransition = true;
						break;						
					}
				}
				else
				{
					// Try with next segment.
					AkInt32 iTimeBeforeNextSync;
					it.NextBucket( this, iTimeBeforeNextSync );

					if ( it != Sequencer()->End() )
					{
						AKASSERT( (*it) && iTimeBeforeNextSync >= 0 );
						transData.iCumulBucketsDuration += iTimeBeforeNextSync;
					}
					else
					{
						// Impossible to compute a transition, probably because there was not enough memory to grow the
						// current chain! This is a fatal error: bail out.
						bFailedEnqueueingTransition = true;
						break;						
					}
				}
				
				AKASSERT( transData.iCumulBucketsDuration >= 0 );
			}
		}
		while ( eReturnVal != AK_Success );

		// Check if an error (out-of-memory) occurred. If it did, destroy destination context.
		if ( bFailedEnqueueingTransition )
		{
			io_pNewContext->OnStopped( 0 );
			io_pNewContext = NULL;
		}	
	}
	else
	{
		// We finally decided not to enqueue a new transition. Destroy the new context.
		io_pNewContext->OnStopped( 0 );
		io_pNewContext = NULL;
	}
    
	// Execute previous pending transitions reversal if applicable.
	AkInt32 iNumTrans = transitionsToRevert.Length();
	while ( --iNumTrans > 0 )
	{
		transitionsToRevert[iNumTrans]->RevertTransition( 
			NULL, 
			false );
	}
	// First transition: restart previous playing context.
	if ( iNumTrans == 0 )
	{
		transitionsToRevert[iNumTrans]->RevertTransition( 
			pFirstNewTransition, 
			true );
    }
	transitionsToRevert.Term();

	AKASSERT( GetActiveChildContext() );
}

// Computes and fills a transition data structure given a transition rule and a bucket iterator.
// Prepares destination.
// Finds a Sync point.
// Returns AK_Success if a music transition can be scheduled (found a Sync point), AK_Fail otherwise.
AKRESULT CAkMusicSwitchCtx::ComputeTransitionData(
    const AkMusicTransitionRule & in_rule,      // Transition rule.
    const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
	CAkMatrixAwareCtx * in_pDestinationContext,	// Destination context.
    AkMusicSwitchTransitionData & io_transData, // Transition data.
    bool    in_bDoForceSucceed	                // Force succeed flag: Sync point will be found by incrementing look-ahead value herein.
    )
{
	AKASSERT( in_pDestinationContext );
	CAkSegmentChain * pDestChain = in_pDestinationContext->GetActiveChain();
	if ( ! pDestChain )
	{
		// WG-11783 - Padding for the fact that GetActiveChain() can return NULL in some
		// cases. This kind of thing caused crashes for some customers, but we did not
		// repro these crashes internally so this may not be enough to fix the problems...
		return AK_Fail;
	}

    AKRESULT eReturnVal;

    // Get destination time constraint.
    io_transData.iDestinationLookAhead = 0; // Used for destination time constraint. Takes streaming and pre-entry into account.

    // Prepare target (and get required look-ahead).
    eReturnVal = pDestChain->Prepare( in_rule.destRule, 0, io_transData.iDestinationLookAhead );
	if ( eReturnVal != AK_Success )
	{
		AKASSERT( !in_bDoForceSucceed || !"Prepare() should not fail" );
		return eReturnVal;
	}

    // Get source time-to-sync.
    {
        // Compute source minimal time constraint (depends on fade and current segment position).
        AkInt32 iSrcMinTimeConstraint = 
            CAkTimeConv::MillisecondsToSamples( in_rule.srcRule.fadeParams.transitionTime ) -
            in_rule.srcRule.fadeParams.iFadeOffset;

        // The effective time constraint is the maximum time between the source and the destination constraints.
        // Then it must be adjusted relatively to the inspected segment.
        AkInt32 iSyncTimeConstraint = AkMax( iSrcMinTimeConstraint, io_transData.iDestinationLookAhead );
        AKASSERT( io_transData.iCumulBucketsDuration >= 0 );
        
        // Make constraints relative to the inspected segment.
        iSyncTimeConstraint -= io_transData.iCumulBucketsDuration;
        
		// Handle Force Succeed case, valid only with Exit Cue sync.
        if ( in_bDoForceSucceed &&
             iSyncTimeConstraint > (*in_itSrc)->SegmentDuration() )
        {                
            AKASSERT( in_rule.srcRule.eSyncType == SyncTypeExitMarker ||
                !"in_bDoForceSucceed flag can only be used with ExitCue sync type" );

            // Increment io_transData.iCumulBucketsDuration so that we are guaranteed to succeed finding a sync position.
            io_transData.iCumulBucketsDuration += ( iSyncTimeConstraint - (*in_itSrc)->SegmentDuration() );
            iSyncTimeConstraint = 0;
        }

		// Avoid finding a sync point before pre-entry, unless we are dealing with a "same time" destination rule.
		bool bAvoidSyncOnEntryCue = ( in_rule.destRule.eEntryType != EntryTypeSameTime );

		// Find transition sync position.
		eReturnVal = (*in_itSrc)->FindSyncPosition( this, in_itSrc.pChain, 
			iSyncTimeConstraint,
			0,	// no constraint on segment's position
            (AkSyncType)in_rule.srcRule.eSyncType,
            bAvoidSyncOnEntryCue,  
			true,	// force succeed if source is <nothing>.
            io_transData.iSegmentTimeToSync );

		// Handle SameTime transition rule.
		// (Avoid computing SameTime transitions when going to or coming from <nothing>, it is useless).
		if ( in_rule.destRule.eEntryType == EntryTypeSameTime &&
			 eReturnVal == AK_Success &&
			 (*in_itSrc)->SegmentCtx() &&
			 pDestChain->GetActiveSegment()->SegmentCtx() )
		{
			AKASSERT( !in_bDoForceSucceed || 
					!"in_bDoForceSucceed flag cannot be used with SameTime entry type" );

			AkInt32 & iDestinationLookAhead = io_transData.iDestinationLookAhead;
			AkInt32 & iRelSyncTime = io_transData.iSegmentTimeToSync;

			AkInt32 iSegmentPosition = (*in_itSrc)->ConvertToSegmentCtxPosition( iRelSyncTime );

			// Prepare destination (and get required look-ahead).
			eReturnVal = pDestChain->Prepare( in_rule.destRule, iSegmentPosition, iDestinationLookAhead );
			if ( eReturnVal != AK_Success )
				return eReturnVal;
			
			// Add src iterator offset if it has already synched (iRelSyncTime is relative to bucket time).
			AkInt32 iAbsBucketTimeOffset = (*in_itSrc)->ConvertToAbsoluteTime( io_transData.iCumulBucketsDuration );

			// Iterate until the destination look-ahead is smaller than the absolute sync time.
			iSyncTimeConstraint = iRelSyncTime + iAbsBucketTimeOffset;
			
			// Iterate until a sync position is found. At each pass, increment the position with the difference between
			// old and new look-ahead values, in order to ensure convergence.
			while ( iDestinationLookAhead > iSyncTimeConstraint )
			{
				// Compute new start position.
				iSegmentPosition = (*in_itSrc)->ConvertToSegmentCtxPosition( iRelSyncTime ) + iDestinationLookAhead;

				// Find new sync position.
				eReturnVal = (*in_itSrc)->FindSyncPosition( this, in_itSrc.pChain, 
					iSyncTimeConstraint,
					iSegmentPosition, // has to be at least at destination's position.
					(AkSyncType)in_rule.srcRule.eSyncType,
					false,	// Allow sync on Entry Cue.
					true,	// force succeed if source is <nothing>.
					iRelSyncTime );
				if ( eReturnVal != AK_Success )
					break;
					
				// Prepare destination with computed sync time (converted to segment position),
				// and get new required look-ahead.
				eReturnVal = pDestChain->Prepare( 
					in_rule.destRule, 
					(*in_itSrc)->ConvertToSegmentCtxPosition( iRelSyncTime ), 
					iDestinationLookAhead );
				if ( eReturnVal != AK_Success )
					break;

				// Compute new sync time constraint.
				iSyncTimeConstraint = iRelSyncTime + iAbsBucketTimeOffset;
			}
		}
    }

    return eReturnVal;
}

// Schedule a valid music transition: Create a pending switch transition
// and attach Play and Stop commands.
CAkPendingSwitchTransition * CAkMusicSwitchCtx::ScheduleTransition(
    const AkMusicTransitionRule & in_rule,      // Transition rule.
    const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
	CAkMatrixAwareCtx * in_pSwitchee,			// Child context corresponding to the destination transition.
    const AkMusicSwitchTransitionData & in_transData	// Transition data.
    )
{
	CAkMatrixAwareCtx * pSrcChildCtx = m_queueTransitions.Last()->Switchee();
    AKASSERT( pSrcChildCtx );

	// Create pending switch transition.
    CAkPendingSwitchTransition * pTransition = CreateSwitch( 
        in_itSrc, 
        in_pSwitchee, 
        in_transData );
	if ( !pTransition )
		return NULL;

    CAkMusicSegment * pInspectedSegmentNode = in_itSrc.SegmentNode();
    {
        // Create the Stop action.
        AkInt32 iStopTime = 0;
        
        // If there is no fade, time to stop depends on whether or not we play
        // the post-exit. Otherwise, it depends on fade duration and offset.
		AkInt32 iFadeOutDuration = in_rule.srcRule.fadeParams.transitionTime;
        if ( iFadeOutDuration > 0 )
        {
            iStopTime = in_rule.srcRule.fadeParams.iFadeOffset - 
                CAkTimeConv::MillisecondsToSamples( iFadeOutDuration );

			// Truncate fade out if it is longer than time to switch (this can only happen when
			// switching from an empty bucket).
			if ( -iStopTime > pTransition->SwitchSyncTime() )
			{
				AKASSERT( !pInspectedSegmentNode );
				iFadeOutDuration += ( pTransition->SwitchSyncTime() + iStopTime );
				iStopTime = -pTransition->SwitchSyncTime();
			}
        }
        else
        {
            // Set stop time to the end of post-exit if the rule requires it.
            if ( in_rule.srcRule.bPlayPostExit &&
                 pInspectedSegmentNode && 
                 in_transData.iSegmentTimeToSync == pInspectedSegmentNode->ActiveDuration() )
            {
                // Exit point is Exit Marker and rule specifies Play Post-Exit.
                iStopTime = pInspectedSegmentNode->PostExitDuration();

				// Inhibate playback of the rest of the chain.
				CAkSegmentChain::Iterator itLastBucket = in_itSrc.ChainIteratorEx();
				in_itSrc.pChain->DisablePlaybackAt( itLastBucket );
            }
        }
        
        pTransition->AttachStopCmd( pSrcChildCtx, 
                                    iFadeOutDuration, 
                                    in_rule.srcRule.fadeParams.eFadeCurve, 
                                    iStopTime );

        // Change bucket behavior regarding Post-Exit.
		(*in_itSrc)->ForcePostExit( in_rule.srcRule.bPlayPostExit );
    }

    // Schedule the Play command.
	pTransition->AttachPlayCmd( in_rule.destRule.fadeParams, -in_transData.iDestinationLookAhead ); 
    
	return pTransition;
}

// Helpers.
// Get the appropriate transition rule.
const AkMusicTransitionRule & CAkMusicSwitchCtx::GetTransitionRule( 
    const CAkMatrixSequencer::Iterator & in_itSrc,	// Matrix iterator pointing the source bucket.
	CAkMatrixAwareCtx * in_pDestContext,			// Destination matrix aware context.
	CAkSegmentBucket *& io_pDestSeqBucket, 			// Destination bucket. IO: Could be swapped if rule requires a sequence JumpTo.
	AkUInt32 &			io_uSrcSegmentLookAhead		// Src look-ahead limit. Passed this limit, use panic rule.
	)
{
	if ( ++io_uSrcSegmentLookAhead > MAX_SRC_SEGMENT_LOOK_AHEAD )
	{
		// Post error message if transition used the panic rule.
		MONITOR_ERROR( AK::Monitor::ErrorCode_CannotScheduleMusicSwitch );
		return CAkMusicTransAware::GetPanicTransitionRule();
	}

	// Compute source data.
	CAkAudioNode * pSrcNode = in_itSrc.SegmentNode();
	CAkAudioNode * pSrcParentNode;
	AkUniqueID srcID;
	if ( pSrcNode )
	{
		pSrcParentNode = pSrcNode->Parent();
		srcID = pSrcNode->ID();
	}
	else
	{
		CAkMatrixAwareCtx * pSrcCtx = in_itSrc.pChain->MatrixAwareCtx();
		pSrcParentNode = pSrcCtx->Node();
		if ( !pSrcParentNode )
		{
			// Matrix Ctx chain is a Null Ctx. Get its parent.
			pSrcParentNode = static_cast<CAkMatrixAwareCtx*>(pSrcCtx->Parent())->Node();
		}
		srcID = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	}

	// Compute destination data.
	CAkAudioNode * pDestParentNode;
	AkUniqueID destID;
	CAkSegmentCtx * pDestCtx = io_pDestSeqBucket->SegmentCtx();
	if ( pDestCtx )
	{
		CAkMusicNode * pDestNode = pDestCtx->SegmentNode();
		pDestParentNode = pDestNode->Parent();
		destID = pDestNode->ID();
	}
	else
	{
		// IMPORTANT Null bucket: since it is a destination, its context is undoubtly a Null Context, and
		// its parent context is certainly a SwitchCtx. Get it through destination's active chain.
		CAkSegmentChain * pChain = in_pDestContext->GetActiveChain();
		if ( ! pChain )
		{
			// WG-11783 - Padding for the fact that GetActiveChain() can return NULL in some
			// cases. This kind of thing caused crashes for some customers, but we did not
			// repro these crashes internally so this may not be enough to fix the problems...
			MONITOR_ERROR( AK::Monitor::ErrorCode_CannotScheduleMusicSwitch );
			return CAkMusicTransAware::GetPanicTransitionRule();
		}

		AKASSERT( pChain->MatrixAwareCtx()->Parent() );
		pDestParentNode = static_cast<CAkMusicSwitchCtx*>(pChain->MatrixAwareCtx()->Parent())->Node();
		
		// ID is <nothing>.
		destID = AK_MUSIC_TRANSITION_RULE_ID_NONE;
	}

	// Get rule.
	bool bIsDestSequenceSpecific;
    const AkMusicTransitionRule & rule = m_pSwitchCntrNode->GetTransitionRule(
		m_pSwitchCntrNode, 
		srcID,
		pSrcParentNode,
		destID,
		pDestParentNode,
		bIsDestSequenceSpecific );

	// Jump to segment if rule found applies explicitly on sequencer container.
    if ( bIsDestSequenceSpecific )
    {
        // The destination node is a sequence container and a rule was specifically defined for it:
        // Reset/Initialize it to the JumpTo index specified in the rule.

		AKASSERT( pDestCtx &&
				  pDestCtx->Parent() &&
				  static_cast<CAkMatrixAwareCtx*>(pDestCtx->Parent())->Node()->NodeCategory() == AkNodeCategory_MusicRanSeqCntr );

        // Leaf sequenced segment will be changed. Swap.
		CAkSequenceCtx * pParentSequenceCtx = static_cast<CAkSequenceCtx*>( pDestCtx->Parent() );

        io_pDestSeqBucket = pParentSequenceCtx->JumpToSegment( rule.destRule.uJumpToID );
    }
    return rule;
}


CAkPendingSwitchTransition * CAkMusicSwitchCtx::CreateSwitch(
    const CAkMatrixSequencer::Iterator & in_itSrc, // Matrix iterator pointing the source bucket.
    CAkMatrixAwareCtx * in_pSwitchee,           // Switchee.
    const AkMusicSwitchTransitionData & in_transData	// Transition data.
    )
{
	CAkSegmentBucket * pSourceBucket = (*in_itSrc);
    AKASSERT( pSourceBucket &&
              in_pSwitchee );

	// Compute branch item time (time relative to previous item in source chain).
	AkInt32 iBranchItemSyncTime = pSourceBucket->ConvertToAbsoluteTime( in_transData.iSegmentTimeToSync );
	AKASSERT( iBranchItemSyncTime >= 0 );

	// Compute absolute switch time.
	// iCumulatedBucketsDuration: total duration between "now" and the beginning of the source bucket (in_itSrc).
	AkInt32 iAbsoluteSwitchTime = in_transData.iCumulBucketsDuration + iBranchItemSyncTime;
	AKASSERT( iAbsoluteSwitchTime >= m_queueTransitions.Last()->SwitchSyncTime() );

    // Create pending switch item.
    CAkPendingSwitchTransition * pNewSwitch = AkNew( g_DefaultPoolId, CAkPendingSwitchTransition( iBranchItemSyncTime, this, in_pSwitchee, in_transData.bIsFromTransSegment ) );
    if ( pNewSwitch )
    {
        if ( EnqueueTransition( pNewSwitch ) )
        {
			pNewSwitch->SwitchSyncTime( iAbsoluteSwitchTime );
			// Clear time of destination chain's first bucket: chain is processed from above before it reaches its sync point.
			CAkSegmentChain* pActiveChain = in_pSwitchee->GetActiveChain();
			if ( pActiveChain )
			{
				// WG-11783 - Padding for the fact that GetActiveChain() can return NULL in some
				// cases. This kind of thing caused crashes for some customers, but we did not
				// repro these crashes internally so this may not be enough to fix the problems...
				pActiveChain->GetActiveSegment()->ForceTimeToSync( 0 );
			}

			// Insert switch item in sequencer.
			CAkSegmentChain * pCurrentChain = in_itSrc.pChain;
			AKASSERT( pCurrentChain );
            CAkSegmentChain::IteratorEx itChain = in_itSrc.ChainIteratorEx();
            CAkSegmentChain::Iterator itOldNext = pCurrentChain->InsertBranchItem( itChain, pNewSwitch );

			// Notify sequencer that there was a path change.
			//
			CAkSegmentBucket * pOldNextBucket = NULL;
			if ( itOldNext != pCurrentChain->End() )
			{
				if ( (*itOldNext)->IsSegment() )
					pOldNextBucket = static_cast<CAkSegmentBucket*>(*itOldNext);
				else
				{
					AkInt32 iDummy;
					pOldNextBucket = (*itOldNext)->GetNextBucket( static_cast<CAkBranchItem*>(*itOldNext)->Owner(), pCurrentChain, iDummy );
				}
			}
				
			Sequencer()->OnPathChange( pSourceBucket, pOldNextBucket, pNewSwitch->SyncTime() );
			//
        }
        else
        {
            // Failed enqueuing transition. Destroy it.
            pNewSwitch->DestroyPendingSwitch();
            pNewSwitch = NULL;
        }
    }

    return pNewSwitch;

}

AKRESULT CAkMusicSwitchCtx::EnqueueFirstContext( 
	CAkMatrixAwareCtx * in_pCtx
    )
{
	AKASSERT( m_queueTransitions.IsEmpty() );
	AKASSERT( in_pCtx );

	// Create pending switch item.
    CAkPendingSwitchTransition * pNewSwitch = AkNew( g_DefaultPoolId, CAkPendingSwitchTransition( 0, this, in_pCtx, false ) );
    if ( pNewSwitch )
    {
        if ( EnqueueTransition( pNewSwitch ) )
        {
			// Set switch item time.
            pNewSwitch->SwitchSyncTime( AK_INFINITE_SYNC_TIME_OFFSET );

			// Setup active switch iterator.
			m_itActiveSwitch = m_queueTransitions.Begin();
			return AK_Success;
        }
        else
        {
            // Failed enqueuing transition. Destroy it.
			pNewSwitch->DestroyPendingSwitch();
        }
    }
	return AK_Fail;
}

CAkPendingSwitchTransition * CAkMusicSwitchCtx::EnqueueTransition( 
    CAkPendingSwitchTransition * in_pNewTransition
    )
{
    CAkPendingSwitchTransition ** ppEnqueuedTransition = m_queueTransitions.AddLast();
    if ( ppEnqueuedTransition )
    {
        *ppEnqueuedTransition = in_pNewTransition;
        return *ppEnqueuedTransition;
    }
    return NULL;
}

// Returns true if this context or any of its ascendents has a pending transition.
bool CAkMusicSwitchCtx::HasOrAscendentHasPendingTransition()
{
	AKASSERT( m_itActiveSwitch != m_queueTransitions.End() );
	TransQueueIter it = m_itActiveSwitch;
	if ( ++it != m_queueTransitions.End() ) 
	{
		return true;
	}
	else if ( m_pParentCtx )
	{
		// Doesn't have a pending transition, but it has a parent: query it.
		return static_cast<CAkMusicSwitchCtx*>(m_pParentCtx)->HasOrAscendentHasPendingTransition();
	}
	return false;
}


// Returns True if the iterator returned should be incremented by re-enqueueing the first transition reverted.
// This occurs when the second transition of a duo (transitions to and from a transition segment) is reverted,
// and not the first one.
bool CAkMusicSwitchCtx::HandleTransitionsReversal( 
	CAkMatrixSequencer::Iterator & io_itSrc,// Matrix iterator pointing the current bucket, updated therein if applicable.
	AkInt32	& io_iCumulBucketsDuration,		// Cumulative buckets duration (0), updated therein if applicable.
	TransitionsArray & out_transitionsToRevert	// Returned array of transitions to revert.	
	)
{
	AKASSERT( m_itActiveSwitch != m_queueTransitions.End() );

	// Get all transitions that will be reverted: transitions are reverted if and only if they are willing to
	// be reverted, AND no subsequent transitions exist that are not willing to be reverted.
	TransitionsQueue::IteratorEx itTransRevert = m_queueTransitions.FindEx( (*m_itActiveSwitch) );
	AKASSERT( itTransRevert != m_queueTransitions.End() );
	++itTransRevert;
	while ( itTransRevert != m_queueTransitions.End() )
	{
		if ( (*itTransRevert)->CanBeReverted() )
		{
			// This transition can be reverted. 
			// Push it in array of transitions to revert.
			if ( !out_transitionsToRevert.AddLast( *itTransRevert ) )
			{
				// Failed to add this transition to the list of transitions to revert! This transition will not be reverted,
				// therefore we need to flush all transitions before it that were already tagged as "to be reverted".
				out_transitionsToRevert.RemoveAll();
			}
		}
		else
		{
			// Cannot be reverted. Flush any transition that was already pushed.
			out_transitionsToRevert.RemoveAll();
		}
		++itTransRevert;
	}

	// Dequeue all transitions that were pushed (contiguous from the first one that has been pushed).
	bool bIsFirstRevertFromTransSegment = false;
	if ( !out_transitionsToRevert.IsEmpty() )
	{
		itTransRevert = m_queueTransitions.FindEx( out_transitionsToRevert[0] );
		AKASSERT( itTransRevert != m_queueTransitions.End() );

		bIsFirstRevertFromTransSegment = (*itTransRevert)->IsFromTransSegment();

		while ( itTransRevert != m_queueTransitions.End() )
		{
			itTransRevert = m_queueTransitions.Erase( itTransRevert );
		}
	}
	
	// Update the matrix iterator and the cumulative duration of looked-ahead buckets so that the further
	// transitions be enqueued at the end of the transitions queue.
	CAkPendingSwitchTransition * pLastQueuedTransition = m_queueTransitions.Last();
	if ( pLastQueuedTransition != (*m_itActiveSwitch) )
	{
		PrepareIteratorToEndOfTransition( 
			pLastQueuedTransition,
			io_itSrc,
			io_iCumulBucketsDuration );
	}

	return bIsFirstRevertFromTransSegment;
}

// Prepare iterator so that it points to the end of the specified transition (to its switchee).
void CAkMusicSwitchCtx::PrepareIteratorToEndOfTransition( 
	CAkPendingSwitchTransition *	in_pTransition,				// Transition.
	CAkMatrixSequencer::Iterator &	out_it,						// Returned iterator.
	AkInt32 &						out_iCumulBucketsDuration	// Returned time before out_it becomes active.
	)
{
	AKASSERT( in_pTransition->Switchee() );
	out_it.pChain = in_pTransition->Switchee()->GetActiveChain();
	AKASSERT( ! out_it.pChain ||
				( out_it.pChain->First() && out_it.pChain->First()->IsSegment() ) );

	// WG-11783 - Padding for the fact that GetActiveChain() can return NULL in some
	// cases. This kind of thing caused crashes for some customers, but we did not
	// repro these crashes internally so this may not be enough to fix the problems...
	out_it.pBucket = out_it.pChain ? static_cast<CAkSegmentBucket*>( out_it.pChain->First() ) : NULL;
	out_iCumulBucketsDuration = in_pTransition->SwitchSyncTime();
}



