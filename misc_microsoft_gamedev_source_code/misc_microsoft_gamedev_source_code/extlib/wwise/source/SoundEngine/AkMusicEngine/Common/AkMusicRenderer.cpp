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
// AkMusicRenderer.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicRenderer.h"
#include "AkSoundBase.h"
#include "AkSource.h"
#include "AkTransitionManager.h"
#include "AkURenderer.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"
#include "AkAudioLib.h"
#include "AkMusicNode.h"
#include "AkSegmentCtx.h"
#include "AkMatrixSequencer.h"
#include "AkMatrixAwareCtx.h"
#include "AkMusicBank.h"
#include "AkMonitorData.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>


//-----------------------------------------------------------------------------
// Default values defines.
//-----------------------------------------------------------------------------
#define DEFAULT_STREAMING_LOOK_AHEAD_RATIO	(1.f);	// Multiplication factor for all streaming look-ahead heuristic values.


// Define interface
namespace AK
{
	namespace MusicEngine
	{
		AKRESULT Init(
			AkMusicSettings *	in_pSettings	///< Initialization settings (can be NULL, to use the default values)
			)
		{
			if( CAkMusicRenderer::Create( in_pSettings ) != NULL )
			{
				SoundEngine::AddBehavioralExtension( CAkMusicRenderer::PerformNextFrameBehavior );
				SoundEngine::AddExternalStateHandler( CAkMusicRenderer::SetState );
				SoundEngine::AddExternalBankHandler( AkMusicBank::LoadBankItem );
#ifndef AK_OPTIMIZED
				SoundEngine::AddExternalProfileHandler( CAkMusicRenderer::HandleProfiling );
#endif
				return AK_Success;
			}
			return AK_Fail;
		}

		/// Get the default values of the initialization settings of the music engine.
		/// \sa
		/// - \ref soundengine_integration_init_advanced
		/// - AK::MusicEngine::Init()
		void GetDefaultInitSettings(
            AkMusicSettings &	out_settings	///< Returned default platform-independent sound engine settings
		    )
		{
			out_settings.fStreamingLookAheadRatio = DEFAULT_STREAMING_LOOK_AHEAD_RATIO;
		}

		void Term()
		{
			CAkMusicRenderer * pMusicRenderer = CAkMusicRenderer::Get( );
			if ( pMusicRenderer )
				pMusicRenderer->Destroy( );
		}

	} // namespace MusicEngine
} // namespace AK

//------------------------------------------------------------------
// Defines.
//------------------------------------------------------------------
#define PENDING_STATE_CHANGES_MIN_ITEMS     (sizeof(CAkMusicRenderer::AkStateChangeRecord)/DEFAULT_POOL_BLOCK_SIZE)
#define PENDING_STATE_CHANGES_MAX_ITEMS     (AK_NO_MAX_LIST_SIZE)

//------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------
CAkMusicRenderer * CAkMusicRenderer::m_pMusicRenderer = NULL;
CAkMusicRenderer::MatrixAwareCtxList CAkMusicRenderer::m_listCtx;
CAkMusicRenderer::PendingStateChanges CAkMusicRenderer::m_queuePendingStateChanges;
// Global music settings.
AkMusicSettings CAkMusicRenderer::m_musicSettings;
AkEvent CAkMusicRenderer::m_hTermEvent;

CAkMusicRenderer * CAkMusicRenderer::Create(
	AkMusicSettings *	in_pSettings
	)
{
    if ( m_pMusicRenderer )
    {
        AKASSERT( !"Should be called only once" );
        return m_pMusicRenderer;
    }

    AKASSERT( g_DefaultPoolId != AK_INVALID_POOL_ID );
    m_pMusicRenderer = AkNew( g_DefaultPoolId, CAkMusicRenderer() );
    if ( m_pMusicRenderer )
    {
        if ( m_pMusicRenderer->Init( in_pSettings ) != AK_Success )
        {
            m_pMusicRenderer->Destroy();
            m_pMusicRenderer = NULL;
        }
    }

    return m_pMusicRenderer;
}

void CAkMusicRenderer::Destroy()
{
	StopAll();
	while ( !m_listCtx.IsEmpty() )
	{
		AKPLATFORM::AkWaitForEvent( m_hTermEvent );
	}
	AkDestroyEvent( m_hTermEvent );
    m_listCtx.Term();
    AKVERIFY( m_queuePendingStateChanges.Term() == AK_Success );
    AkDelete( g_DefaultPoolId, this );
}

AKRESULT CAkMusicRenderer::Init(
	AkMusicSettings *	in_pSettings
	)
{
	// Store user settings.
	if ( in_pSettings )
	{
		m_musicSettings = *in_pSettings;
	}
	else
	{
		// Use defaults.
		AK::MusicEngine::GetDefaultInitSettings( m_musicSettings );
	}

	// TODO: ListBareLight should not return an AKRESULT.
	AKVERIFY( m_listCtx.Init() == AK_Success );
	
	AKRESULT eResult = AkCreateEvent( m_hTermEvent );
	if ( eResult == AK_Success )
		return m_queuePendingStateChanges.Init( PENDING_STATE_CHANGES_MIN_ITEMS, PENDING_STATE_CHANGES_MAX_ITEMS, g_DefaultPoolId );
	return eResult;
}

CAkMusicRenderer::CAkMusicRenderer()
{
    m_pMusicRenderer = NULL;
}

CAkMusicRenderer::~CAkMusicRenderer()
{
    m_pMusicRenderer = NULL;
}

// Similar to URenderer::Play().
// Creates a Music PBI (a PBI that can be a child of a high-level context) and assigns a parent.
// Returns it
// Uses the parent's transition, the parent's game object
AKRESULT CAkMusicRenderer::Play( 
    CAkMusicCtx *		io_pParentCtx,
	CAkSoundBase*		in_pSound,
	CAkSource *			in_pSource,
    CAkRegisteredObj *	in_pGameObj,
    TransParams &		in_transParams,
    UserParams&			in_rUserparams,
    //AkPlaybackState	in_ePlaybackState,
	AkUInt32			in_uSourceOffset,	// Start position of source (in samples, at the native sample rate).
    AkUInt32			in_uFrameOffset,    // Frame offset for look-ahead and LEngine sample accuracy.
    CAkMusicPBI *&		out_pPBI            // TEMP: Created PBI is needed to set the transition from outside.
    )
{
	AKRESULT eResult = AK_Fail;
    // Check parameters.
    AKASSERT( in_pSound != NULL );
    if( in_pSound == NULL )
        return AK_InvalidParameter;

	AkPriority priority = CAkURenderer::_CalcInitialPriority( in_pSound, in_pGameObj );

	if ( in_pSound->IncrementPlayCount( priority, in_pGameObj ) )
	{
		AkPathInfo PathInfo = { NULL, AK_INVALID_UNIQUE_ID };
        // We don't care about the play history. TODO Get rid of it.
        PlayHistory history;
    	history.Init();
		out_pPBI = AkNew( RENDERER_DEFAULT_POOL_ID, 
            CAkMusicPBI( io_pParentCtx,
                         in_pSound,
						 in_pSource,
			             in_pGameObj,
			             in_rUserparams,
			             history,     
			             AK_INVALID_SEQUENCE_ID,
			             priority,
						 in_uSourceOffset
			             ) );

		if( out_pPBI != NULL )
		{
			if( out_pPBI->Init( &PathInfo ) == AK_Success )
			{
                out_pPBI->SetFrameOffset( in_uFrameOffset );
				return Play( out_pPBI, 
                             in_transParams
                             /*, in_ePlaybackState*/ );
			}
			else
			{
				out_pPBI->Term(); //does call DecrementPlayCount()
				AkDelete( RENDERER_DEFAULT_POOL_ID, out_pPBI );
				out_pPBI = NULL;
				return eResult;
			}
		}
	}
	else
	{
		eResult = AK_PartialSuccess;

		PlayHistory playHistory;
		playHistory.Init();

		in_pSound->MonitorNotif( AkMonitorData::NotificationReason_PlayFailedLimit,
			in_pGameObj->ID(),
			in_rUserparams,
			playHistory );
	}

	// Either ran out of memory, or insufficient priority to play
	in_pSound->DecrementPlayCount( in_pGameObj );

	return eResult;
} // Play


AKRESULT CAkMusicRenderer::Play(	
    CAkMusicPBI *   in_pContext, 
    TransParams &   in_transParams
    /*,
	AkPlaybackState	in_ePlaybackState*/
	)
{
    // Check parameters.
    AKASSERT( in_pContext != NULL );
    if( in_pContext == NULL )
        return AK_InvalidParameter;

	// Add PBI context to Upper Renderer's list.
    CAkURenderer::EnqueueContext( in_pContext );
	
    AKRESULT l_eResult = in_pContext->_InitPlay();
	AKASSERT( l_eResult == AK_Success );
    
	bool l_bPaused = false;
    /* TODO
	// Check if the play command is actually a play-pause.
	if( in_ePlaybackState == PB_Paused )
	{
		l_bPaused = true;
	}
    */

	l_eResult = in_pContext->_Play( in_transParams, l_bPaused, true );
	AKASSERT( l_eResult == AK_Success );
	
	return l_eResult;
}

// Stops all top-level contexts.
void CAkMusicRenderer::StopAll()
{
    // Look among our top-level children.
	// Note. Contexts may dequeue themselves while being stopped
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
	{
		// Cache our pointer on the stack, as it could self-destruct inside OnStopped().
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
		pCtx->OnStopped( 0 );
	}
}


// Game triggered actions (stop/pause/resume).
AKRESULT CAkMusicRenderer::Stop(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams,
	AkPlayingID			in_playingID
    )
{
    AKRESULT eResult = AK_Success;

    // Look among our top-level children.
    // Note. Contexts may dequeue themselves while being stopped.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache our pointer on the stack, as it could self-destruct inside OnStopped().
        CAkMatrixAwareCtx * pCtx = (*it);
		++it;

        if( pCtx->Node() == in_pNode )
		{
			if( !in_pGameObj || pCtx->Sequencer()->GameObjectPtr() == in_pGameObj )
			{
				if( in_playingID == AK_INVALID_PLAYING_ID || pCtx->Sequencer()->PlayingID() == in_playingID )
				{
					if ( pCtx->_Stop( in_transParams ) != AK_Success )
						eResult = AK_Fail;
				}
			}
		}
    }
    return eResult;
}

AKRESULT CAkMusicRenderer::Pause(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams
    )
{
    AKRESULT eResult = AK_Success;

    // Look among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
        if( (*it)->Node() == in_pNode )
		{
			if( !in_pGameObj || (*it)->Sequencer()->GameObjectPtr() == in_pGameObj )
			{
                if ( (*it)->_Pause( in_transParams ) != AK_Success )
                    eResult = AK_Fail;
			}
		}
        ++it;
	} 
    return eResult;
}

AKRESULT CAkMusicRenderer::Resume(	
    CAkMusicNode *      in_pNode,
    CAkRegisteredObj *  in_pGameObj,
    TransParams &       in_transParams,
    bool                in_bMasterResume    // REVIEW
    )
{
    AKRESULT eResult = AK_Success;

    // Look among our top-level children.
    MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
        if( (*it)->Node() == in_pNode )
		{
			if( !in_pGameObj || (*it)->Sequencer()->GameObjectPtr() == in_pGameObj )
			{
                if ( (*it)->_Resume( in_transParams, in_bMasterResume ) != AK_Success )
                    eResult = AK_Fail;
			}
		}
        ++it;
	} 
    return eResult;
}

// Add/Remove Top-Level Music Contexts (happens when created from MusicNode::Play()).
AKRESULT CAkMusicRenderer::AddChild( 
    CAkMatrixAwareCtx * in_pMusicCtx,
    UserParams &        in_rUserparams,
    CAkRegisteredObj *  in_pGameObj        
    )
{
    // Create and enqueue a top-level sequencer.
    CAkMatrixSequencer * pSequencer = AkNew( g_DefaultPoolId, CAkMatrixSequencer( in_pMusicCtx ) );
    if ( pSequencer )
    {
        AKRESULT eResult = pSequencer->Init( in_rUserparams, in_pGameObj );
        if ( eResult == AK_Success )
        {
			m_listCtx.AddFirst( in_pMusicCtx );

            // TODO LX Enforce do not set sequencer elsewhere than here.
            in_pMusicCtx->SetSequencer( pSequencer );

            // We generated a Top-Level context and sequencer:
            // Register/Add ref to the Playing Mgr, so that it keeps the playing ID alive.
            if ( in_rUserparams.PlayingID )
            {
                AKASSERT( g_pPlayingMgr );
                eResult = g_pPlayingMgr->SetPBI( in_rUserparams.PlayingID, in_pMusicCtx );
                if( in_pMusicCtx->Node() )
					in_pMusicCtx->Node()->IncrementActivityCount();
            }
        }
		else
        {
			// Destroy sequencer now if it wasn't assigned to the context yet. Otherwise, let contexts
			// remove themselves normally from the renderer.
            pSequencer->Term();
            AkDelete( g_DefaultPoolId, pSequencer );
        }
		
        return eResult;
    }

    return AK_Fail;
}
void CAkMusicRenderer::RemoveChild( 
    CAkMatrixAwareCtx * in_pMusicCtx
    )
{
    // Note: This call may fail if the context was never added to the renderer's children, because 
	// CAkMusicRenderer::AddChild() failed (because no memory).
    m_listCtx.Remove( in_pMusicCtx );
    
	// Note: The context may not have a sequencer if it was not created because of an out-of-memory condition.
    CAkMatrixSequencer * pSequencer = in_pMusicCtx->Sequencer();
	if( pSequencer )
	{
		// Notify Playing Mgr.
		AKASSERT(g_pPlayingMgr);
		if ( pSequencer->PlayingID( ) )
		{
			g_pPlayingMgr->Remove( pSequencer->PlayingID( ), in_pMusicCtx );
			if( in_pMusicCtx->Node() )
				in_pMusicCtx->Node()->DecrementActivityCount();
		}
	
		pSequencer->Term();
		AkDelete( g_DefaultPoolId, pSequencer );
	}

	AKPLATFORM::AkSignalEvent( m_hTermEvent );
}

// Music Audio Loop interface.
void CAkMusicRenderer::PerformNextFrameBehavior()
{
    // Perform top-level segment sequencers.
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
    {
		// Cache our pointer on the stack in case dequeueing occurs from within Execute().
        CAkMatrixAwareCtx * pCtx = (*it);
		++it;
		pCtx->Sequencer()->Execute();
	}
}

//
// States management.
//

// Returns true if state change was handled (delayed) by the Music Renderer. False otherwise.
bool CAkMusicRenderer::SetState( 
    AkStateGroupID     in_stateGroupID, 
    AkStateID          in_stateID
    )
{
    // Query top-level context sequencers that need to handle state change by delaying it.

    AkInt32 iEarliestAbsoluteDelay = 0;
    CAkMatrixAwareCtx * pChosenCtx = NULL;

    // Values for chosen context.
    AkInt32 iChosenRelativeSyncTime;  
    AkUInt32 uChosenSegmentLookAhead;

    if ( GetDelayedStateChangeData(
            in_stateGroupID,
            pChosenCtx,
            iChosenRelativeSyncTime,
            uChosenSegmentLookAhead ) <= 0 )
    {
        // Either a context requires an immediate change, or no one is registered to this state group.
        // Return now as "not handled".        
        return false;
    }


    //
    // Process delayed state change.
    // 
    AKASSERT( pChosenCtx );

    // Reserve a spot for pending state change.
    AkStateChangeRecord * pNewStateChange = m_queuePendingStateChanges.AddFirst();
    if ( !pNewStateChange )
    {
        // No memory. Return without handling state change.
        // Invalidate all pending state changes that relate to this state group.
        PendingStateChangeIter it = m_queuePendingStateChanges.Begin();
    	InvalidateOlderPendingStateChanges( it, in_stateGroupID );
        return false;
    }

    
    // Delegate handling of delayed state change sequencing to the appropriate context sequencer.
    if ( pChosenCtx->Sequencer()->ProcessDelayedStateChange( pNewStateChange, 
                                                             uChosenSegmentLookAhead, 
                                                             iChosenRelativeSyncTime ) == AK_Success )
    {
        // Setup the rest of the state change record.
        pNewStateChange->stateGroupID   = in_stateGroupID;
        pNewStateChange->stateID        = in_stateID;
        pNewStateChange->bWasPosted     = false;
		pNewStateChange->bIsReferenced	= true;

        // Return True ("handled") unless a context required an immediate change.
        return true;
    }
    else
    {
        // Failed handling delayed state change.
        // Remove the record, return False ("not handled").
        AKVERIFY( m_queuePendingStateChanges.RemoveFirst() == AK_Success );
        return false;
    }
}

// Execute a StateChange music action.
void CAkMusicRenderer::PerformDelayedStateChange(
    void *             in_pCookie
    )
{
    // Find pending state change in queue. 
    PendingStateChangeIterEx it;
	// Post state change if required (if was not already posted).
	FindPendingStateChange( in_pCookie, it );
	(*it).bIsReferenced = false;
    if ( !(*it).bWasPosted )
    {
        (*it).bWasPosted = true;
		
        AkStateGroupID stateGroupID = (*it).stateGroupID;
        //
        // Set state on sound engine, with flag "skip call to state handler extension".
        // 
        AKVERIFY( AK::SoundEngine::SetState( 
            stateGroupID, 
            (*it).stateID, 
            false, 
            true ) == AK_Success ); 

		// Invalidate all older pending state changes (for this StateGroup ID).
        InvalidateOlderPendingStateChanges( ++it, stateGroupID );
    }
	// else State change is obsolete.
	
    
    // Clean up queue.
    CleanPendingStateChanges();
}

// Notify Renderer whenever a StateChange music action needs to be rescheduled.
void CAkMusicRenderer::RescheduleDelayedStateChange(
    void *              in_pCookie
    )
{
    // Find pending state change in queue. 
    PendingStateChangeIterEx it;    
    FindPendingStateChange( in_pCookie, it );
	if ( !(*it).bWasPosted )
	{
		AkStateGroupID stateGroupID = (*it).stateGroupID;

		// Values for chosen context.
		AkInt32 iChosenRelativeSyncTime;  
		AkUInt32 uChosenSegmentLookAhead;
		CAkMatrixAwareCtx * pChosenCtx = NULL;

		if ( GetDelayedStateChangeData(
				stateGroupID, 
				pChosenCtx,
				iChosenRelativeSyncTime,
				uChosenSegmentLookAhead ) <= 0 )
		{
			// Either a context requires an immediate change, or no one is registered to this state group.
			CancelDelayedStateChange( stateGroupID, it );
			return;
		}

		AKASSERT( pChosenCtx );

		//
		// Process delayed state change.
		// 
	    
		// Delegate handling of delayed state change sequencing to the appropriate context sequencer.
		if ( pChosenCtx->Sequencer()->ProcessDelayedStateChange( in_pCookie, 
																 uChosenSegmentLookAhead, 
																 iChosenRelativeSyncTime ) != AK_Success )
		{
			// Failed handling delayed state change.
			// Set state on sound engine now, remove the record.
			CancelDelayedStateChange( stateGroupID, it );
		}
	}
	else 
	{
		// State change was obsolete anyway, so mark it as "not referenced" and clean now.
		(*it).bIsReferenced = false;
		CleanPendingStateChanges();
	}
}


// Helpers.

// Set state on sound engine now, with flag "skip call to state handler extension".
// Clean pending delayed state change list.
void CAkMusicRenderer::CancelDelayedStateChange( 
    AkStateGroupID     in_stateGroupID, 
    PendingStateChangeIterEx & in_itPendingStateChg
    )
{
    //
    // Set state on sound engine now, with flag "skip call to state handler extension".
    // 
    AKVERIFY( AK::SoundEngine::SetState( 
        in_stateGroupID, 
        (*in_itPendingStateChg).stateID, 
        false, 
        true ) == AK_Success ); 

    // Invalidate record, clean.
    (*in_itPendingStateChg).bWasPosted = true;
	(*in_itPendingStateChg).bIsReferenced = false;
    InvalidateOlderPendingStateChanges( in_itPendingStateChg, in_stateGroupID );
    CleanPendingStateChanges();
}

// Query top-level context sequencers that need to handle state change by delaying it.
// Returns the minimal absolute delay for state change. Value <= 0 means "immediate".
AkInt32 CAkMusicRenderer::GetDelayedStateChangeData(
    AkStateGroupID          in_stateGroupID, 
    CAkMatrixAwareCtx *&    out_pChosenCtx,
    AkInt32 &               out_iChosenRelativeSyncTime,
    AkUInt32 &              out_uChosenSegmentLookAhead
    )
{
    AkInt32 iEarliestAbsoluteDelay = 0;
    out_pChosenCtx = NULL;

    MatrixAwareCtxList::Iterator itCtx = m_listCtx.Begin();
	while ( itCtx != m_listCtx.End() )
    {
		if ( (*itCtx)->IsPlaying() )
		{
			AkInt32 iRelativeSyncTime;  // state change time relative to segment
			AkUInt32 uSegmentLookAhead;

			AkInt32 iAbsoluteDelay = (*itCtx)->Sequencer()->QueryStateChangeDelay( in_stateGroupID, 
																				   uSegmentLookAhead,
																				   iRelativeSyncTime );
			if ( !out_pChosenCtx || 
				 iAbsoluteDelay < iEarliestAbsoluteDelay )
			{
				// This context requires a state change that should occur the earliest.
				iEarliestAbsoluteDelay = iAbsoluteDelay;

				out_iChosenRelativeSyncTime = iRelativeSyncTime;
				out_uChosenSegmentLookAhead = uSegmentLookAhead;
				out_pChosenCtx = (*itCtx);
			}
		}
        ++itCtx;
    }

    // NOTE. Since delayed processing always occurs one frame later, we substract one frame size out of the returned
    // delay. (Delays smaller than one frame will be considered as immediate).
    iEarliestAbsoluteDelay -= AK_NUM_VOICE_REFILL_FRAMES;
    return iEarliestAbsoluteDelay;
}

void CAkMusicRenderer::FindPendingStateChange( 
    void * in_pCookie,
    PendingStateChangeIterEx & out_iterator
    )
{
    out_iterator = m_queuePendingStateChanges.BeginEx();
    while ( out_iterator != m_queuePendingStateChanges.End() )
    {
        if ( &(*out_iterator) == in_pCookie )
        {
            // Found.
            break;
        }
        ++out_iterator;
    }

    // Must have been found.
    AKASSERT( out_iterator != m_queuePendingStateChanges.End() );
}

void CAkMusicRenderer::CleanPendingStateChanges()
{
    PendingStateChangeIterEx it = m_queuePendingStateChanges.BeginEx();
    while ( it != m_queuePendingStateChanges.End() )
    {
        // Dequeue if required (if ref count is 0).
        if ( !(*it).bIsReferenced )
        {
        	AKASSERT( (*it).bWasPosted );
            it = m_queuePendingStateChanges.Erase( it );
        }
        else
            ++it;
    }
}

void CAkMusicRenderer::InvalidateOlderPendingStateChanges( 
    PendingStateChangeIter & in_iterator,
    AkStateGroupID           in_stateGroupID
    )
{
    while ( in_iterator != m_queuePendingStateChanges.End() )
    {
        // Find next (older) pending state change with that same StateGroup ID.
        if ( (*in_iterator).stateGroupID == in_stateGroupID )
        {
            // Invalidate.
            (*in_iterator).bWasPosted = true;
        }
        ++in_iterator;
    }
}

#ifndef AK_OPTIMIZED
void CAkMusicRenderer::HandleProfiling()
{
	/*
	Get and post :
		SegmentID, 
		PlayingID 
		and position (in double 64 in ms)
	*/

	// We must first count them to make the initial allocation (only if required)
	AkUInt16 uNumPlayingIM = 0;
	MatrixAwareCtxList::Iterator it = m_listCtx.Begin();
	while ( it != m_listCtx.End() )
	{
		CAkMatrixAwareCtx * pCtx = (*it);
		++it;
		if( pCtx->Node()->NodeCategory() == AkNodeCategory_MusicSegment 
			&& pCtx->Sequencer()->GetUserParams().CustomParam.ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
		{
			++uNumPlayingIM;
		}
	}
	if( uNumPlayingIM )
	{
		// We do have something to monitor, so let's gather the info.
		AkInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, segmentPositionData.positions )
						+ uNumPlayingIM * sizeof( AkMonitorData::SegmentPositionData );

		AkProfileDataCreator creator( sizeofItem );
		if ( !creator.m_pData )
			return;

		creator.m_pData->eDataType = AkMonitorData::MonitorDataSegmentPosition;
		creator.m_pData->timeStamp = AkMonitor::GetThreadTime();

		creator.m_pData->segmentPositionData.numPositions = uNumPlayingIM;

		uNumPlayingIM = 0;
		it = m_listCtx.Begin();
		while ( it != m_listCtx.End() )
		{
			CAkMatrixAwareCtx * pCtx = (*it);
			++it;
			CAkMatrixSequencer* pSequencer = pCtx->Sequencer();
			if( pCtx->Node()->NodeCategory() == AkNodeCategory_MusicSegment 
			&& ( pSequencer->GetUserParams().CustomParam.ui32Reserved & AK_EVENTFROMWWISE_RESERVED_BIT )
			&& pCtx->IsPlaying() )
			{	
				AkMonitorData::SegmentPositionData& l_rdata = creator.m_pData->segmentPositionData.positions[ uNumPlayingIM ];
				AkInt32 iCurSegmentPosition = pSequencer->GetCurSegmentPosition();//in samples
				if( iCurSegmentPosition <= 0 )
				{
					l_rdata.f64Position = 0;//negative stands for not started yet, so we pass 0.
				}
				else
				{
					l_rdata.f64Position =	CAkTimeConv::SamplesToSeconds( iCurSegmentPosition )*1000;
				}
				l_rdata.playingID =		pSequencer->PlayingID();
				l_rdata.segmentID =		pCtx->Node()->ID();
				l_rdata.customParam =	pSequencer->GetUserParams().CustomParam;

				++uNumPlayingIM;
			}
		}
	}
}
#endif
