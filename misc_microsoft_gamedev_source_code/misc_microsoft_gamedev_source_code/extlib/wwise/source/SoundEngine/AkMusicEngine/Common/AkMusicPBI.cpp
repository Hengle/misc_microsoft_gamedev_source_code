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
// AkMusicPBI.cpp
//
// Enhances the PBI by keeping a pointer to a parent (music) context. 
// Removes itself from parent in destructor.
// Also, handles transitions of a multi-level context hierarchy:
// checks if it is registered to a transition of one of its ascendant
// before creating one (a extra step before PBI::_Play/_Stop).
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicPBI.h"
#include "AkTransitionManager.h"
#include "AkSegmentCtx.h"
#include "Ak3DParams.h"

CAkMusicPBI::CAkMusicPBI(
    CAkMusicCtx *   in_parent,
    CAkSoundBase*	in_pSound,			// Pointer to the sound.
	CAkSource*		in_pSource,			// Pointer to the source.
	CAkRegisteredObj * in_pGameObj,		// Game object and channel association.
	UserParams&		in_UserParams,		// User Parameters.
	PlayHistory&	in_rPlayHistory,	// History stuff.
	AkUniqueID		in_SeqID,			// Sample accurate seq id.
	AkPriority		in_Priority,
	AkUInt32		in_uSourceOffset
    )
:CAkPBI( in_pSound,
		 in_pSource,
         in_pGameObj,
         in_UserParams,
         in_rPlayHistory,
         in_SeqID,
         in_Priority,
		 false /*Currently no support for feedback in interactive music*/)
,CAkChildCtx( in_parent )
,m_ulSourceOffset( in_uSourceOffset )
,m_ulStopOffset( AK_NO_IN_BUFFER_STOP_REQUESTED )
,m_bWasStoppedByUEngine ( false )
{
#ifndef AK_OPTIMIZED
	m_bNotifyStarvationAtStart = true;
#endif
}

CAkMusicPBI::~CAkMusicPBI()
{
}

void CAkMusicPBI::Term()
{
    AKASSERT( m_pParentCtx );
    if ( !m_bWasStoppedByUEngine )
    {
        // Destruction occurred without the higher-level hierarchy knowing it. 
        // Notify Segment (we know our parent is a Segment).
        static_cast<CAkSegmentCtx*>(m_pParentCtx)->RemoveAllReferences( this );
    }
    m_pParentCtx->RemoveChild( this );

	CAkPBI::Term();
}

AKRESULT CAkMusicPBI::_Stop( AkPBIStopMode in_eStopMode, bool in_bIsFromTransition )
{
	return CAkPBI::_Stop( in_eStopMode, in_bIsFromTransition );
}

AKRESULT CAkMusicPBI::Init( AkPathInfo* in_pPathInfo )
{
    AKASSERT( m_pParentCtx || !"A Music PBI HAS to have a parent" );
    
	Connect();

    return CAkPBI::Init( in_pPathInfo );
}

// Child context implementation.
// ------------------------------------------

// Start context playback, propagated from a high-level context _Play().
// Return - AKRESULT - AK_Success if succeeded

// NOTE: If OnPlay() is called, it means that _Play() handling was propagated from a higher level
// context, thus we were already playing (but probably fading out to stop). 
// ONLY the Music Renderer can actually _Play() a MusicPBI.
void CAkMusicPBI::OnPlayed(
	AkUInt32	in_uSubFrameOffset		// Sample offset inside audio frame to actually start playing.
	)
{
    m_bWasStopped = false;
    m_bWasStoppedByUEngine = false;
}

// Stop context playback, propagated from a high-level context Stop().
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicPBI::OnStopped( 
	AkUInt32 in_uSubFrameOffset		// Sample offset inside audio frame to actually stop playing.
    )
{
	Lock();

    // We set it no matter what it is, faster to set than to check.
    if( m_ulStopOffset == AK_NO_IN_BUFFER_STOP_REQUESTED || m_ulStopOffset > in_uSubFrameOffset )//
		SetStopOffset( in_uSubFrameOffset );

    if ( m_bWasPaused || ( m_PBTrans.pvPRTrans && m_PBTrans.bIsPRTransFading ) )
	{
		// If we are paused or going to be, we stop right away.
		CAkPBI::_Stop();
    }
    else
    {
	    // We need to ensure that the PBI will stop in next CbxNode::GetBuffer(). 
	    // Currently, it is by making the PBI silent, along with PreStopped = true.
	    m_bWasStopped = true;

		if ( in_uSubFrameOffset == AK_NO_IN_BUFFER_STOP_REQUESTED )
		{
			StopWithMinTransTime();
		}
	}
    m_bWasStoppedByUEngine = true;

	Unlock();
}

// Pause context playback, propagated from a high-level context _Pause().
// Return - AKRESULT - AK_Success if succeeded
void CAkMusicPBI::OnPaused()
{
    // Enqueue pause command to lower engine, through upper renderer.
    _Pause( true );
}

// Resume context playback, propagated from a high-level context _Resume().
// Return - AKRESULT - AK_Success if succeeded

void CAkMusicPBI::OnResumed()
{
    // Enqueue pause command to lower engine, through upper renderer.
    _Resume();

    // Note. PBIs mute their m_cPauseResumeFade in _Pause(), but do not unmute it in _Resume().
    // Force it.
    m_cPauseResumeFade = UNMUTED_LVL;
    CalculateMutedEffectiveVolume();
}


// OVERRIDEN METHODS FROM PBIs.
// ---------------------------------------------

// Stop the PBI.
AKRESULT CAkMusicPBI::_Stop( 
    AkUInt32 in_uStopOffset )
{
	//Setting a stop offset is stopping the sound by simulating the end of the source data from the specified offset.
	if( m_ulStopOffset == AK_NO_IN_BUFFER_STOP_REQUESTED || m_ulStopOffset > in_uStopOffset )//
		SetStopOffset( in_uStopOffset );

	// REVIEW m_bWasStopped = true;

    // Set m_bWasStoppedByUEngine. This will ensure that PBI destruction will be considered as coming from above.
    m_bWasStoppedByUEngine = true;

    return AK_Success;
}


// Fade management. Propagate fades down to PBIs muted map
void CAkMusicPBI::SetPBIFade( 
    void * in_pOwner,
    AkUInt8 in_uFadeRatio
    )
{
    AkMutedMapItem mutedMapItem;
    mutedMapItem.m_bIsPersistent = true;
    mutedMapItem.m_bIsGlobal = false;
    mutedMapItem.m_Identifier = in_pOwner;
    
    SetMuteMapEntry( mutedMapItem, in_uFadeRatio );
}

AkUInt32 CAkMusicPBI::GetSourceOffset()
{
	return m_ulSourceOffset; 
}

AkUInt32 CAkMusicPBI::GetAndClearStopOffset()
{
	AkUInt32 ulStopOffset = m_ulStopOffset;
	m_ulStopOffset = AK_NO_IN_BUFFER_STOP_REQUESTED;
	return ulStopOffset;
}

AKRESULT CAkMusicPBI::GetParams( AkSoundParams * io_Parameters )
{
	AKRESULT eResult = CAkPBI::GetParams( io_Parameters );
	io_Parameters->Pitch = 0;

	return eResult;
}

AkPitchValue CAkMusicPBI::GetPitch()
{
	return 0;
}
