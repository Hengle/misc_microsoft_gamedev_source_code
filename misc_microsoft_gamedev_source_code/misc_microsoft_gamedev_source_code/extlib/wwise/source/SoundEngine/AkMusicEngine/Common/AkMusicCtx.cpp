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
// AkMusicCtx.h
//
// Base context class for all parent contexts.
// Propagates commands to its children. Implements child removal.
//
// NOTE: Only music contexts are parent contexts, so this class is
// defined here. Move to AkAudioEngine if other standard nodes use them.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkMusicCtx.h"
#include "AkPlayingMgr.h"
#include "AkMusicSegment.h"

#include "AkTransitionManager.h"
#include "AkMusicRenderer.h"

#define AK_MIN_MUSIC_FADE_TRANSITION_TIME	(1)


CAkMusicCtx::CAkMusicCtx( 
    CAkMusicCtx *   in_parent
    )
    :CAkChildCtx( in_parent )
    ,m_uRefCount( 0 ) 
    ,m_uPauseCount( 0 )
    ,m_uPlayStopFade( MUTED_LVL )
    ,m_uPauseResumeFade( UNMUTED_LVL )
    ,m_bIsPlaying( false )
    ,m_bIsPaused( false )
{
}

CAkMusicCtx::~CAkMusicCtx()
{
    // A music context cannot be destroyed until all its children are destroyed and removed.
    AKASSERT( m_listChildren.IsEmpty() || 
              !"A music context cannot be destroyed until all its children have been destroyed");

    // Remove transitions if applicable.
    if( m_PBTrans.pvPSTrans )
	{
        //MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, AkMonitorData::NotificationReason_Fade_Aborted, m_CntrHistArray, id, 0 );
        g_pTransitionManager->RemoveTransitionFromList( m_PBTrans.pvPSTrans );
	}

	if( m_PBTrans.pvPRTrans )
	{
        //MONITOR_OBJECTNOTIF(m_UserParams.PlayingID, m_pGameObj->ID(), m_UserParams.CustomParam, AkMonitorData::NotificationReason_Fade_Aborted, m_CntrHistArray, id, 0 );
        g_pTransitionManager->RemoveTransitionFromList( m_PBTrans.pvPRTrans );
	}

    m_listChildren.Term();
}

// Init: Connects to parent or to Music Renderer.
void CAkMusicCtx::Init(
    CAkRegisteredObj *  in_pGameObj,
    UserParams &    in_rUserparams
    )
{
	// TODO: ListBareLight should not return an AKRESULT.
	AKVERIFY( m_listChildren.Init() == AK_Success );
    if ( m_pParentCtx )
    {
        Connect();
    }
}

// Parent-child management.
void CAkMusicCtx::AddChild(
    CAkChildCtx * in_pChildCtx
    )
{
    AKASSERT( in_pChildCtx );
    AKASSERT( !( m_listChildren.FindEx( in_pChildCtx ) != m_listChildren.End() ) );
    m_listChildren.AddFirst( in_pChildCtx );
    AddRef();
}

void CAkMusicCtx::RemoveChild( 
    CAkChildCtx * in_pChildCtx
    )
{
    AKASSERT( in_pChildCtx );
    AKRESULT eResult = m_listChildren.Remove( in_pChildCtx );
    AKASSERT( eResult == AK_Success );
    if ( eResult == AK_Success )
        Release();
}

void CAkMusicCtx::Release()
{
    AKASSERT( m_uRefCount > 0 );
    --m_uRefCount;
    if ( m_uRefCount == 0 )
    {
        if ( m_pParentCtx )
            m_pParentCtx->RemoveChild( this );
        else
            CAkMusicRenderer::Get()->RemoveChild( (CAkMatrixAwareCtx*)this ); /// TEMP. Templatize child/parent/music contexts. Avoid casts.
        AkDelete( g_DefaultPoolId, this );
    }
}

// ITransitionable implementation:
// ----------------------------------------
// Set our own virtual fade ratio.
void CAkMusicCtx::TransUpdateValue(
    TransitionTargetTypes in_eTargetType, 
    TransitionTarget in_unionValue, 
    bool in_bIsTerminated )
{
    bool bIsFadeOut = false;
    switch ( in_eTargetType & TransTarget_TargetMask )
    {
    case TransTarget_Stop:
        bIsFadeOut = true;
    case TransTarget_Play:
        m_uPlayStopFade = static_cast<AkUInt8>(((in_unionValue.fValue - AK_MINIMUM_VOLUME_LEVEL) * 255.0f) / -(AK_MINIMUM_VOLUME_LEVEL));
        SetPBIFade( &m_uPlayStopFade, m_uPlayStopFade );
        if ( in_bIsTerminated )
        {
            m_PBTrans.pvPSTrans = NULL;
            
            // Complete stop.
            // TODO (perhaps) sample accurate transitions.
            if ( bIsFadeOut )
                OnStopped(AK_NO_IN_BUFFER_STOP_REQUESTED);
        }
        break;
    case TransTarget_Pause:
        bIsFadeOut = true;
    case TransTarget_Resume:
        m_uPauseResumeFade = static_cast<AkUInt8>(((in_unionValue.fValue - AK_MINIMUM_VOLUME_LEVEL) * 255.0f) / -(AK_MINIMUM_VOLUME_LEVEL));
        SetPBIFade( &m_uPauseResumeFade, m_uPauseResumeFade );
        if ( in_bIsTerminated )
        {
            m_PBTrans.pvPRTrans = NULL;
            
            // Complete stop if not active.
            if ( bIsFadeOut )
                OnPaused();
        }
        break;
    }
}


// Child context handling implementation.
// ----------------------------------------

// Propage a high-level context _Play() command.
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::OnPlayed(
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
	)
{
    // Set playing flag.
    m_bIsPlaying = true;
}

// Stop context playback, propagated from a high-level context Stop().
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
	ChildrenCtxList::Iterator it = m_listChildren.Begin();
	while ( it != m_listChildren.End() )
	{
		// Cache our pointer on the stack, as it could self-destruct inside OnStopped().
		CAkChildCtx * pChild = (*it);
		++it;
		pChild->OnStopped( in_uSubFrameOffset );
	}

    // Clear playing flag.
    m_bIsPlaying = false;
}

// Pause context playback, propagated from a high-level context _Pause().
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::OnPaused()
{
    // Just propagate the command down to children (will ultimately affect leaf PBIs).
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->OnPaused();
        ++it;
    }

    // Set paused flag.
    m_bIsPaused = true;
}

// Resume context playback, propagated from a high-level context _Resume().
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicCtx::OnResumed()
{
    // Clear paused flag.
    m_bIsPaused = false;

    // Just propagate the command down to children.
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->OnResumed();
        ++it;
    }
}

// Context commands
//

// Start context playback.
// Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Play( 
    TransParams & in_transParams,
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
    )
{
    AkMusicFade fadeParams;
    fadeParams.transitionTime   = in_transParams.TransitionTime;
    fadeParams.eFadeCurve       = in_transParams.eFadeCurve;
    fadeParams.iFadeOffset      = 0;

    return _Play( fadeParams, in_uSubFrameOffset );
}

// Start context playback.
// Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Play( 
    AkMusicFade & in_fadeParams,
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
    )
{
    // Play command that is not propagated. 
	// Create/update PS transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new valus).
	if ( m_PBTrans.pvPSTrans )
    {
        // This context has a transition. Revert it.
        TransitionTarget newTarget;
	    newTarget.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	    g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPSTrans,
            TransTarget_Play,
            newTarget,
            in_fadeParams.transitionTime,
            AkValueMeaning_Default );
    }
	else if ( in_fadeParams.transitionTime > 0 )
    {
        // Otherwise create our own if duration is not null.
        TransitionParameters Params;
	    Params.pUser = this;
	    Params.eTargetType = static_cast<TransitionTargetTypes>( TransTarget_Play | AkTypeFloat );
	    Params.uStartValue.fValue = AK_MINIMUM_VOLUME_LEVEL;
	    Params.uTargetValue.fValue = AK_MAXIMUM_VOLUME_LEVEL;
		Params.lDuration = in_fadeParams.transitionTime;
	    Params.eFadeCurve = in_fadeParams.eFadeCurve;
	    Params.bdBs = true;
	    m_PBTrans.pvPSTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPSTransFading = true;

        if( !m_PBTrans.pvPSTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTargetType, Params.uTargetValue, true );
		}
        else if ( in_fadeParams.iFadeOffset != 0 )
        {
            // Use Transition Mgr's transition time-offset service.
            m_PBTrans.pvPSTrans->Offset( in_fadeParams.iFadeOffset / AK_NUM_VOICE_REFILL_FRAMES );
            /** Note. For now we count on trans mgr to update us at every frame, even when we are
            TimeRatio < 0, because new created PBIs will not inherit this property otherwise. But we suffer
            from the cost of interpolation. 
            m_uPlayStopFade = 0;
            SetPBIFade( &m_uPlayStopFade, m_uPlayStopFade );
            */
        }
    }

    /**
    // Notify Music Renderer that we just began playing, so that it can actualize its pending state changes queue.
    CAkMusicRenderer::Get()->NotifyStateChangeStartPlay( this );
    **/
    
    // Now, propagate the command down to children.
    OnPlayed( in_uSubFrameOffset );
    return AK_Success;
}

// Stop context playback.
//Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Stop( 
    TransParams & in_transParams,
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
	// Create/update PS transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new valus).
    if ( m_PBTrans.pvPSTrans )
    {
        // This context has a transition. Revert it.
        TransitionTarget newTarget;
	    newTarget.fValue = AK_MINIMUM_VOLUME_LEVEL;
	    g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPSTrans,
            TransTarget_Stop,
            newTarget,
            in_transParams.TransitionTime,
            AkValueMeaning_Default );
    }
    else if ( in_transParams.TransitionTime > 0 )
    {
        TransitionParameters Params;
	    Params.pUser = this;
	    Params.eTargetType = static_cast<TransitionTargetTypes>( TransTarget_Stop | AkTypeFloat );
	    Params.uStartValue.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	    Params.uTargetValue.fValue = AK_MINIMUM_VOLUME_LEVEL;
	    Params.lDuration = in_transParams.TransitionTime;
	    Params.eFadeCurve = in_transParams.eFadeCurve;
	    Params.bdBs = true;
	    m_PBTrans.pvPSTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPSTransFading = true;

		if( !m_PBTrans.pvPSTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTargetType, Params.uTargetValue, true );
		}
    }
    else
    {
        // Immediate stop
        OnStopped( in_uSubFrameOffset );
    }
    return AK_Success;
}

// Pause context playback.
// Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Pause( 
    TransParams & in_transParams
    )
{
    ++m_uPauseCount;

    // Create/update PR transition if transitionTime > 0, BUT ALSO if there is already a PS transition
	// (because it needs to be updated to avoid having a transition fighting with this new valus).
    if ( m_PBTrans.pvPRTrans )
    {
        // This context has a transition. Revert it.
        TransitionTarget newTarget;
	    newTarget.fValue = AK_MINIMUM_VOLUME_LEVEL;
	    g_pTransitionManager->ChangeParameter(
            m_PBTrans.pvPRTrans,
            TransTarget_Pause,
            newTarget,
            in_transParams.TransitionTime,
            AkValueMeaning_Default );
    }
    else if ( in_transParams.TransitionTime > 0 )
    {
        TransitionParameters Params;
	    Params.pUser = this;
	    Params.eTargetType = static_cast<TransitionTargetTypes>( TransTarget_Pause | AkTypeFloat );
	    Params.uStartValue.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	    Params.uTargetValue.fValue = AK_MINIMUM_VOLUME_LEVEL;
	    Params.lDuration = in_transParams.TransitionTime;
	    Params.eFadeCurve = in_transParams.eFadeCurve;
	    Params.bdBs = true;
	    m_PBTrans.pvPRTrans = g_pTransitionManager->AddTransitionToList(Params);
        m_PBTrans.bIsPRTransFading = true;

		if( !m_PBTrans.pvPRTrans )
		{
			// TODO : Should send a warning to tell the user that the transition is being skipped because 
			// the max num of transition was reached, or that the transition manager refused to do 
			// it for any other reason.

			// Forcing the end of transition right now.
			TransUpdateValue( Params.eTargetType, Params.uTargetValue, true );
		}
    }
    else
    {
        // Immediate pause.
        OnPaused();
    }
    return AK_Success;
}

// Resume context playback.
// Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Resume( 
    TransParams & in_transParams, 
    bool in_bIsMasterResume
    )
{
    if ( in_bIsMasterResume || m_uPauseCount <= 1 )
    {       
    	m_uPauseCount = 0;
    	
        // Resume command that is not propagated. 
        // Create/update PR transition if transitionTime > 0, BUT ALSO if there is already a PS transition
		// (because it needs to be updated to avoid having a transition fighting with this new valus).
		if ( m_PBTrans.pvPRTrans )
        {
            // This context has a transition. Revert it.
            TransitionTarget newTarget;
	        newTarget.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	        g_pTransitionManager->ChangeParameter(
                m_PBTrans.pvPRTrans,
                TransTarget_Resume,
                newTarget,
                in_transParams.TransitionTime,
                AkValueMeaning_Default );
        }
        else if ( in_transParams.TransitionTime > 0 )
        {
            // Otherwise create our own if duration is not null.
            TransitionParameters Params;
	        Params.pUser = this;
	        Params.eTargetType = static_cast<TransitionTargetTypes>( TransTarget_Resume | AkTypeFloat );
	        Params.uStartValue.fValue = -AK_MINIMUM_VOLUME_LEVEL * m_uPauseResumeFade / 255.0f + AK_MINIMUM_VOLUME_LEVEL;
	        Params.uTargetValue.fValue = AK_MAXIMUM_VOLUME_LEVEL;
	        Params.lDuration = in_transParams.TransitionTime;
	        Params.eFadeCurve = in_transParams.eFadeCurve;
	        Params.bdBs = true;
	        m_PBTrans.pvPRTrans = g_pTransitionManager->AddTransitionToList(Params);
            m_PBTrans.bIsPRTransFading = true;

            if( !m_PBTrans.pvPRTrans )
		    {
			    // TODO : Should send a warning to tell the user that the transition is being skipped because 
			    // the max num of transition was reached, or that the transition manager refused to do 
			    // it for any other reason.
	
			    // Forcing the end of transition right now.
			    TransUpdateValue( Params.eTargetType, Params.uTargetValue, true );
		    }
        }
        else
		{
			// Reset pause resume fade in case we were paused fading. Notify PBIs below.
			m_uPauseResumeFade = UNMUTED_LVL;
			SetPBIFade( &m_uPauseResumeFade, m_uPauseResumeFade );
		}

        // Now, propagate the command down to children.
        OnResumed();
    }
    else
        --m_uPauseCount;

    return AK_Success;
}


// Stop the PBI (the PBI is then destroyed)
//
//Return - AKRESULT - AK_Success if succeeded
AKRESULT CAkMusicCtx::_Stop( AkPBIStopMode in_eStopMode, bool in_bIsFromTransition )
{
    AKASSERT( !"Not implemented" );
    return AK_NotImplemented;
}

#ifndef AK_OPTIMIZED
AKRESULT CAkMusicCtx::_StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		)
{
    AKASSERT( !"Not implemented" );
    return AK_NotImplemented;
}
#endif


// Fade management. Propagate fades down to PBIs muted map
void CAkMusicCtx::SetPBIFade( 
    void * in_pOwner,
    AkUInt8 in_uFadeRatio
    )
{
    ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        (*it)->SetPBIFade( in_pOwner, in_uFadeRatio );
        ++it;
    }
}

#ifdef _DEBUG
AkUInt32 CAkMusicCtx::NumChildren()
{
	AkUInt32 uNumChildren = 0;
	ChildrenCtxList::Iterator it = m_listChildren.Begin( );
    while ( it != m_listChildren.End( ) )
    {
        ++uNumChildren;
        ++it;
    }
	return uNumChildren;
}
#endif
