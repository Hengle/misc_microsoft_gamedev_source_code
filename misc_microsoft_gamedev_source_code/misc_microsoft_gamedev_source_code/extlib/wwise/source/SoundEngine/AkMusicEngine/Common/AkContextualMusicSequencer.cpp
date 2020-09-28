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
// AkContextualMusicSequencer.cpp
//
// Action sequencer for music contexts.
// Holds a list of pending musical actions, stamped with sample-based
// timing. 
// For example, a sequence context would enqueue actions on children 
// nodes that are scheduled to play or stop in the near future.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkContextualMusicSequencer.h"
#include "AkMusicCtx.h"
#include "AkMusicRenderer.h"

extern AkMemPoolId g_DefaultPoolId;

#define MIN_NUM_ITEMS   (2)
#define MAX_NUM_ITEMS   (256)


CAkContextualMusicSequencer::CAkContextualMusicSequencer( )
{
}

CAkContextualMusicSequencer::~CAkContextualMusicSequencer()
{
}

AKRESULT CAkContextualMusicSequencer::Init()
{
    m_iNow = 0;
    return m_listActions.Init( MIN_NUM_ITEMS, MAX_NUM_ITEMS, g_DefaultPoolId );
}

void CAkContextualMusicSequencer::Term()
{
	AKVERIFY( m_listActions.Term() == AK_Success );
}

// Returns a pointer to the action enqueued if successful, NULL otherwise.
const AkMusicAction * CAkContextualMusicSequencer::ScheduleAction( 
    AkMusicAction & in_action
    )
{
	AKASSERT( m_listActions.IsInitialized() );

    // Insert action chronologically.
    AkInt32 & iTime = in_action.iTime;
    SeqActionsList::IteratorEx it = m_listActions.BeginEx();
    while ( it != m_listActions.End() )
    {
        if ( iTime < (*it).iTime )
        {
            if ( m_listActions.Insert( it, in_action ) != m_listActions.End() )
            {
                // NOTE. This is the only way to get the element we just inserted. Is this a bug with Insert()??
                m_listActions.Actualize( it );
                return &(*it);
            }
            else break;
        }
        ++it;
    }
    
    return m_listActions.AddLast( in_action );
}

// Returns AK_NoMoreData when there is no action to be executed in next frame (out_action is invalid).
// Otherwise, returns AK_DataReady.
AKRESULT CAkContextualMusicSequencer::PopImminentAction(
    AkInt32 in_iFrameDuration,			// Number of samples to process.
	AkMusicAction & out_action
    )
{
	AKASSERT( m_listActions.IsInitialized() );
	AKASSERT( in_iFrameDuration <= AK_NUM_VOICE_REFILL_FRAMES );
    if ( m_listActions.Length() )
    {
        out_action = m_listActions.First();
        AKASSERT( out_action.iTime >= ( m_iNow ) ||
            !"Action should have been executed in the past" );
        if ( out_action.iTime < m_iNow + in_iFrameDuration )
        {
            AKVERIFY( m_listActions.RemoveFirst() == AK_Success );
            return AK_DataReady;
        }
    }
    return AK_NoMoreData;
}

// Removes from sequencer all actions that reference the specified target. 
void CAkContextualMusicSequencer::ClearActionsByTarget( 
    const void * in_pTargetAddress
    )
{
    AKASSERT( m_listActions.IsInitialized() );
	SeqActionsList::IteratorEx it = m_listActions.BeginEx();
    while ( it != m_listActions.End() )
    {
        if ( (*it).pTargetPBI == in_pTargetAddress )
        {
            it = m_listActions.Erase( it );
        }
        else
            ++it;
    }
}

// Remove all actions from sequencer (ref counted targets are released).
void CAkContextualMusicSequencer::Flush()
{
    // Remove all actions.
    if ( m_listActions.IsInitialized() )
	{
		m_listActions.RemoveAll();
	}
}
