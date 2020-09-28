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
// AkMusicSegment.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkMusicSegment.h"
#include "AkSegmentCtx.h"
#include "AkMusicRenderer.h"
#include "AkMusicTrack.h"
#include "AkBankFloatConversion.h"
#include "AkPBI.h"
#include "AkMonitor.h"

#include "AkMatrixSequencer.h"
#include "AkSequencableSegmentCtx.h"

#define NUM_MIN_MARKERS     (2)

CAkMusicSegment::CAkMusicSegment( 
    AkUniqueID in_ulID
    )
:CAkMusicNode( in_ulID )
,m_uDuration(0)
#ifndef AK_OPTIMIZED
,m_uStartPos(0)
#endif
{
}

CAkMusicSegment::~CAkMusicSegment()
{
    Term();
}

CAkMusicSegment * CAkMusicSegment::Create(
    AkUniqueID in_ulID 
    )
{
	CAkMusicSegment * pSegment = AkNew( g_DefaultPoolId, CAkMusicSegment( in_ulID ) );
    if( pSegment )
	{
		if( pSegment->Init() != AK_Success )
		{
			pSegment->Release();
			pSegment = NULL;
		}
	}
    return pSegment;
}

AKRESULT CAkMusicSegment::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = SetMusicNodeParams( in_pData, in_ulDataSize, false );

	if( eResult == AK_Success )
	{
		Duration( READBANKDATA( AkReal64, in_pData, in_ulDataSize ) );

		AkUInt32 ulNumMarkers = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		if( ulNumMarkers )
		{
			// Reserve actually commented out since not possible as long as default markers are set upon segment init, uncomment when fixed to optimize mem usage.
			//eResult = m_markers.Reserve( ulNumMarkers );
			//if( eResult != AK_Success )
			//	return eResult;
			//TODO(alessard)
			AkMusicMarkerWwise* pArrayMarkers = (AkMusicMarkerWwise*)AkAlloc( g_DefaultPoolId, ulNumMarkers*sizeof( AkMusicMarkerWwise ) );
			if( !pArrayMarkers )
				return AK_Fail;

			for( AkUInt32 i = 0; i < ulNumMarkers; ++i )
			{
				pArrayMarkers[i].id = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
				pArrayMarkers[i].fPosition = READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			}
			if( eResult == AK_Success )
			{
				eResult = SetMarkers( pArrayMarkers, ulNumMarkers );
			}
			AkFree( g_DefaultPoolId, pArrayMarkers );
		}
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkMusicSegment::Init()
{
    // NOTE Entry and Exit markers have ID 0. See what's best with WAL.
    // Add entry marker (with default value 0).
    AkMusicMarker defaultMarker;
    defaultMarker.id = 0;
    defaultMarker.uPosition = 0;
    if ( !m_markers.AddLast( defaultMarker ) )
        return AK_InsufficientMemory;
    // Add exit marker (with default value 0).
    if ( !m_markers.AddLast( defaultMarker ) )
        return AK_InsufficientMemory;
    return CAkMusicNode::Init();
}

void CAkMusicSegment::Term()
{
    m_markers.Term();
}

AkNodeCategory CAkMusicSegment::NodeCategory()
{
	return AkNodeCategory_MusicSegment;
}


// Context factory. 
// Creates a sequencable segment context, usable by switch containers or as a top-level instance.
CAkMatrixAwareCtx * CAkMusicSegment::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams,
    CAkSegmentBucket *& out_pFirstRelevantBucket
    )
{
    CAkSequencableSegmentCtx * pCtx = AkNew( g_DefaultPoolId, CAkSequencableSegmentCtx( 
        this,
        in_pParentCtx ) );
    if ( pCtx )
    {
		pCtx->AddRef();
        if ( pCtx->Init( in_GameObject, in_rUserparams, out_pFirstRelevantBucket ) == AK_Success )
		{
			pCtx->Release();
		}
		else
        {
			pCtx->OnStopped( 0 );
			pCtx->Release();
            pCtx = NULL;
        }
    }
    return pCtx;
}


CAkSegmentCtx * CAkMusicSegment::CreateSegmentCtxAndAddRef( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
    CAkSegmentCtx * pSegmentCtx = AkNew( g_DefaultPoolId, CAkSegmentCtx( 
        this,
        in_pParentCtx ) );
    if ( pSegmentCtx )
    {
		pSegmentCtx->AddRef();
        if ( pSegmentCtx->Init( in_GameObject, in_rUserparams ) != AK_Success )
        {
            pSegmentCtx->OnStopped( 0 );
			pSegmentCtx->Release();
            pSegmentCtx = NULL;
        }
    }
    return pSegmentCtx;
}

AKRESULT CAkMusicSegment::CanAddChild( CAkAudioNode * in_pAudioNode )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		MONITOR_ERRORMSG2( L"Too many children in one single container.", L"" );
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicTrack)
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

AKRESULT CAkMusicSegment::Play( AkPBIParams& in_rPBIParams )
{
    // Create a Context as a top-level (that is, attached to the Music Renderer).

    // OPTIM. Could avoid virtual call.
    CAkSegmentBucket * pBucket;
    CAkMatrixAwareCtx * pCtx = CreateContext( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams, pBucket );
    if ( pCtx )
    {
        AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
        fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
        // Set fade offset to segment context's look-ahead time.
        fadeParams.iFadeOffset      = pBucket->SegmentCtx()->GetStreamingLookAheadTime();
        return pCtx->_Play( fadeParams );
    }
    return AK_Fail;
}

void CAkMusicSegment::Duration(
    AkReal64 in_fDuration               // Duration in milliseconds.
    )
{
    AKASSERT( in_fDuration >= 0 );
    m_uDuration = CAkTimeConv::MillisecondsToSamples( in_fDuration );
}

#ifndef AK_OPTIMIZED
void CAkMusicSegment::StartPos(
		AkReal64 in_fStartPos            // StartPosition in milliseconds.
        )
{
    m_uStartPos = CAkTimeConv::MillisecondsToSamples( in_fStartPos );
}
#endif

AKRESULT CAkMusicSegment::SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                 in_ulNumMarkers
		)
{
	m_markers.RemoveAll();
	for( AkUInt32 i = 0; i < in_ulNumMarkers; ++i )
	{
		AkMusicMarker l_marker;
		l_marker.id = in_pArrayMarkers[i].id;
		l_marker.uPosition = CAkTimeConv::MillisecondsToSamples( in_pArrayMarkers[i].fPosition );
		if( !m_markers.AddLast( l_marker ) )
			return AK_Fail;
	}
	return AK_Success;
}

void CAkMusicSegment::RemoveMarkers()
{
	while( m_markers.Length() > NUM_MIN_MARKERS )
	{
		//we must keep last and first, so we always flush second.
		m_markers.Erase( 1 );
	}
}

// Interface for Contexts
// ----------------------

CAkMusicTrack * CAkMusicSegment::Track(
    AkUInt16 in_uIndex
    )
{
    if ( in_uIndex < m_mapChildId.Length( ) )
        return static_cast<CAkMusicTrack*>( m_mapChildId[in_uIndex].item );
    return NULL;
}

AKRESULT CAkMusicSegment::GetExitSyncPos(
    AkInt32         in_iSrcMinTime,         // Minimal time-to-sync constraint related to source, relative to the Entry Marker.
    AkSyncType      in_eSyncType,           // Sync type.
    bool            in_bDoSkipEntryMarker,  // If true, will not consider Entry marker.
    AkInt32 &       out_iExitSyncPos        // Returned Exit Sync position (always >= 0), relative to the Entry Marker.
    )
{
    AKRESULT eResult = AK_Fail;

    // Clamp minimum value to either 0 (entry cue) or absolute beginning of segment (-pre-entry),
    // according to whether this method should return a sync position that is always greater than the entry cue,
    // or if it is allowed to search in the pre-entry.
    if ( in_bDoSkipEntryMarker &&
         in_iSrcMinTime < 0 )
    {
        in_iSrcMinTime = 0;
    }
    else if ( in_iSrcMinTime < -PreEntryDuration() )
    {
        in_iSrcMinTime = -PreEntryDuration();
    }

    // Leave now if uMinSyncPosition is passed the Exit marker.
    if ( in_iSrcMinTime <= ExitMarkerPosition() )
    {
        switch ( in_eSyncType )
        {
        case SyncTypeImmediate:
            out_iExitSyncPos = in_iSrcMinTime;
            eResult = AK_Success;
            break;
        case SyncTypeNextGrid:
            {
                const AkMusicGrid & grid = GetMusicGrid();
                eResult = GetNextMusicGridValue(
                    in_iSrcMinTime,
                    grid.uGridDuration,
                    grid.uGridOffset,
                    out_iExitSyncPos ); // Returns position relative to Entry Marker.
            }
            break;
        case SyncTypeNextBeat:
            eResult = GetNextMusicGridValue(
                in_iSrcMinTime,
                GetMusicGrid().uBeatDuration,
                0,
                out_iExitSyncPos ); // Returns position relative to Entry Marker.
            break;
        case SyncTypeNextBar:
            eResult = GetNextMusicGridValue(
                in_iSrcMinTime,
                GetMusicGrid().uBarDuration,
                0,
                out_iExitSyncPos ); // Returns position relative to Entry Marker.
            break;
        case SyncTypeNextMarker:
            AKASSERT( GetNextMarkerPosition( in_iSrcMinTime, in_bDoSkipEntryMarker ) >= in_iSrcMinTime );
            out_iExitSyncPos = GetNextMarkerPosition( in_iSrcMinTime, in_bDoSkipEntryMarker );
            eResult = AK_Success;   // Must succeed because we already made sure that uMinSyncPosition was not passed Exit Marker.
            break;
        case SyncTypeNextUserMarker:
            eResult = GetNextUserMarkerPosition( in_iSrcMinTime, out_iExitSyncPos );
            break;
        case SyncTypeExitMarker:
            AKASSERT( ExitMarkerPosition() >= in_iSrcMinTime );
            out_iExitSyncPos = ExitMarkerPosition();
            eResult = AK_Success;   // Must succeed because we already made sure that uMinSyncPosition was not passed Exit Marker.
            break;
        case SyncTypeEntryMarker:
            if ( in_iSrcMinTime <= 0 && !in_bDoSkipEntryMarker )
            {
                out_iExitSyncPos = 0;
                eResult = AK_Success;
            }
            break;
        default:
            AKASSERT( !"Invalid source transition type" );
            eResult = AK_Fail;
        }
    }

    return eResult;
}

// Returns the position of the entry Sync point defined by the rule. Positions are relative to Entry Cue.
AKRESULT CAkMusicSegment::GetEntrySyncPos(
    const AkMusicTransDestRule & in_rule,   // Transition destination (arrival) rule.
	AkInt32 in_iDesiredEntryPosition,		// Desired entry position (applies to SameTime rule only).
	AkInt32 & out_iRequiredEntryPosition	// Returned entry position.
    )
{
    // Check rule.
    switch ( in_rule.eEntryType )
    {
    case EntryTypeSameTime:
        // (than the current segment playing, in seconds, from the Entry Marker).
		if ( in_iDesiredEntryPosition >= ExitMarkerPosition() )
			return AK_Fail;

		out_iRequiredEntryPosition = in_iDesiredEntryPosition;
        break;
    case EntryTypeEntryMarker:
        out_iRequiredEntryPosition = 0;
        break;
    case EntryTypeUserMarker:
        out_iRequiredEntryPosition = GetUserMarkerPosition( in_rule.markerID );
        break;
    default:
        AKASSERT( !"Invalid source transition type" );
        return AK_Fail;
    }

    return AK_Success;
}

// Returns the duration of the pre-entry.
AkInt32 CAkMusicSegment::PreEntryDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    return m_markers[0].uPosition;
}

// Returns the duration of the post exit.
AkUInt32 CAkMusicSegment::PostExitDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition <= Duration() );
    return ( Duration() - m_markers.Last().uPosition );
}

AkInt32 CAkMusicSegment::ActiveDuration()
{
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition >= m_markers[0].uPosition );
    return m_markers.Last().uPosition - m_markers[0].uPosition;
}


// Helpers.
// Return marker position, relative to Entry marker.
AkInt32 CAkMusicSegment::ExitMarkerPosition()
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    AKASSERT( m_markers.Last().uPosition >= m_markers[0].uPosition );
    return m_markers.Last().uPosition - m_markers[0].uPosition;
}

AkInt32 CAkMusicSegment::GetNextMarkerPosition(
    AkInt32 in_iPosition,       // Position from which to start search, relative to Entry marker (>=0).
    bool    in_bDoSkipEntryMarker// If true, will not consider Entry marker.
    )
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    // This method should not be called when the context position is behind the Entry marker.
    AKASSERT( !in_bDoSkipEntryMarker || in_iPosition >= 0 || !"Segment position is behind Entry marker; transition should not be queried" );

    // Make position absolute.
    AKASSERT( (AkInt32)m_markers[0].uPosition + in_iPosition >= 0 );
    AkUInt32 uAbsPosition = in_iPosition + m_markers[0].uPosition;

    // Skip Entry marker.
    AKASSERT( m_markers.Length() > 0 );
    MarkersArray::Iterator it = m_markers.Begin();
    if ( in_bDoSkipEntryMarker )
        ++it;
    while ( it != m_markers.End() )
    {
        if ( (*it).uPosition >= uAbsPosition )
            return (*it).uPosition - m_markers[0].uPosition;
        ++it;
    }

    // Could not find a marker before the end.
    AKASSERT( !"Cannot find next marker" );
    return 0;
}

// Note. Might fail if there are no user marker before Exit marker.
// This method MAY be called when the context position is behind the Entry marker.
AKRESULT CAkMusicSegment::GetNextUserMarkerPosition(
    AkInt32 in_iPosition,       // Position from which to start search, relative to Entry marker (>=0).
    AkInt32 & out_iMarkerPosition
    )
{
    // Must have at least 2 markers (Entry and Exit).
    AKASSERT( m_markers.Length() >= 2 ||
              !"Invalid markers array" );
    
    // Make position absolute.
    AkUInt32 uAbsPosition = in_iPosition + m_markers[0].uPosition;

    // Skip Entry marker.
    AkUInt32 uIndex = 1;
    AkUInt32 uLastIndex = m_markers.Length() - 2;

    while ( uIndex <= uLastIndex )
    {
        if ( m_markers[uIndex].uPosition > uAbsPosition )
        {
            out_iMarkerPosition = m_markers[uIndex].uPosition - m_markers[0].uPosition;
            return AK_Success;
        }
        ++uIndex;
    }

    // Could not find user marker before Exit marker.
    return AK_Fail;
}

// Finds a user marker by its ID.
AkUInt32 CAkMusicSegment::GetUserMarkerPosition( 
    AkMusicMarkerID in_markerID 
    )
{
    MarkersArray::Iterator it = m_markers.Begin();
    while ( it != m_markers.End() )
    {
        if ( (*it).id == in_markerID )
            return (*it).uPosition - m_markers[0].uPosition;
        ++it;
    }
    AKASSERT( !"Invalid marker ID" );
    return 0;
}

bool CAkMusicSegment::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
	{
		CAkMusicTrack* pTrack = (CAkMusicTrack*)((*iter).item);
		if( pTrack->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	return CAkParameterNodeBase::GetStateSyncTypes( in_stateGroupID, io_pSyncTypes );
}

// Find next Bar, Beat, Grid absolute position.
// Returns AK_Success if a grid position was found before the Exit Marker.
// NOTE This computation is meant to change with compound time signatures.
AKRESULT CAkMusicSegment::GetNextMusicGridValue(
    AkInt32  in_iPosition,      // Start search position (in samples), relative to Entry marker.
    AkUInt32 in_uGridDuration,  // Grid (beat, bar) duration.
    AkUInt32 in_uOffset,        // Grid offset.
    AkInt32 & out_iExitPosition // Returned position (relative to EntryMarker).
    )
{
    AkInt32 iExitMarker = ExitMarkerPosition();
    out_iExitPosition = in_uOffset;
    while ( out_iExitPosition < in_iPosition &&
            out_iExitPosition <= iExitMarker )
    {
        out_iExitPosition += in_uGridDuration;
    }
    if ( out_iExitPosition <= iExitMarker )
        return AK_Success;

    // Did not find a bar boundary before the Exit marker. 
    return AK_Fail;
}
