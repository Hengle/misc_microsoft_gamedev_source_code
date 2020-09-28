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

#include "stdafx.h"
#include "AkMatrixAwareCtx.h"
#include "AkMusicRenderer.h"
#include "AkMatrixSequencer.h"
#include "AkMusicNode.h"
#include "AkMonitor.h"

CAkMatrixAwareCtx::CAkMatrixAwareCtx(
    CAkMusicCtx *   in_parent // Parent context. NULL if this is a top-level context.
    )
:CAkMusicCtx( in_parent )
,m_pSequencer( NULL )
{
}

CAkMatrixAwareCtx::~CAkMatrixAwareCtx()
{
}

AKRESULT CAkMatrixAwareCtx::Init(
    CAkRegisteredObj *  in_pGameObj,
    UserParams &        in_rUserparams
    )
{
    CAkMusicCtx::Init( in_pGameObj, in_rUserparams );
    if ( m_pParentCtx )
    {
        // Inherit our parent's Sequencer.
        SetSequencer( static_cast<CAkMatrixAwareCtx*>(m_pParentCtx)->Sequencer() );
		return AK_Success;
    }
    else
    {
        // We are a top-level context. Add ourselves to the Music Renderer.
        return CAkMusicRenderer::Get()->AddChild( this, in_rUserparams, in_pGameObj );
    }
}

// Called by parent (switches): completes the initialization.
// Default implementation does nothing.
void CAkMatrixAwareCtx::EndInit()
{
}

// Interface for parent switch context: trigger switch change that was delayed because of parent transition.
void CAkMatrixAwareCtx::PerformDelayedSwitchChange()
{
}

bool CAkMatrixAwareCtx::IsNullCtx()
{
	return false;
}

#ifndef AK_OPTIMIZED
void CAkMatrixAwareCtx::OnPaused()
{
	// NOTE: OnPaused() is called without consideration to pause count.
	if ( IsTopLevel() && !IsPaused() )
	{
		MONITOR_MUSICOBJECTNOTIF( m_pSequencer->PlayingID(), m_pSequencer->GameObjectPtr()->ID(), m_pSequencer->GetUserParams().CustomParam, AkMonitorData::NotificationReason_Paused, Node()->ID(), AK_INVALID_UNIQUE_ID );
	}

	CAkMusicCtx::OnPaused();
}

void CAkMatrixAwareCtx::OnResumed()
{
	// NOTE: OnResumed() is called only when pause count reaches 0.
	if ( IsTopLevel() && IsPaused() )
	{
		MONITOR_MUSICOBJECTNOTIF( m_pSequencer->PlayingID(), m_pSequencer->GameObjectPtr()->ID(), m_pSequencer->GetUserParams().CustomParam, AkMonitorData::NotificationReason_Resumed, Node()->ID(), AK_INVALID_UNIQUE_ID );
	}

	CAkMusicCtx::OnResumed();
}
#endif

// Shared Segment Sequencer.
void CAkMatrixAwareCtx::SetSequencer( 
    CAkMatrixSequencer * in_pSequencer
    ) 
{ 
    AKASSERT( !m_pSequencer ); 
    m_pSequencer = in_pSequencer; 
}




