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
// AkMusicRanSeqCntr.cpp
//
// Music Random/Sequence container definition.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicRanSeqCntr.h"
#include "AkSequenceCtx.h"
#include "AkMatrixSequencer.h"
#include "AkPBI.h"
#include "AkMonitor.h"

extern AkMemPoolId g_DefaultPoolId;


CAkMusicRanSeqCntr::CAkMusicRanSeqCntr( AkUniqueID in_ulID )
:CAkMusicTransAware( in_ulID )
,m_playListRoot( NULL )
{
}
CAkMusicRanSeqCntr::~CAkMusicRanSeqCntr()
{
    Term();
}

// Thread safe version of the constructor.
CAkMusicRanSeqCntr * CAkMusicRanSeqCntr::Create(
    AkUniqueID in_ulID
    )
{
	CAkMusicRanSeqCntr * pSequence = AkNew( g_DefaultPoolId, CAkMusicRanSeqCntr( in_ulID ) );
    if( pSequence )
	{
		if( pSequence->Init() != AK_Success )
		{
			pSequence->Release();
			pSequence = NULL;
		}
	}
    return pSequence;
}

AKRESULT CAkMusicRanSeqCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = SetMusicTransNodeParams( in_pData, in_ulDataSize, false );

	AkUInt32 numPlaylistItems = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( numPlaylistItems )
	{
		AkMusicRanSeqPlaylistItem* pPlayList = (AkMusicRanSeqPlaylistItem*)AkAlloc( g_DefaultPoolId, numPlaylistItems*sizeof(AkMusicRanSeqPlaylistItem) );
		if( !pPlayList ) 
			return AK_Fail;
		for( AkUInt32 i = 0; i < numPlaylistItems; ++i )
		{
			pPlayList[i].m_SegmentID		= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_playlistItemID	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

			pPlayList[i].m_NumChildren		= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_eRSType			= (RSType)READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlayList[i].m_Loop				= READBANKDATA( AkInt16, in_pData, in_ulDataSize );
			pPlayList[i].m_Weight			= READBANKDATA( AkUInt16, in_pData, in_ulDataSize );
			pPlayList[i].m_wAvoidRepeatCount= READBANKDATA( AkUInt16, in_pData, in_ulDataSize );

			pPlayList[i].m_bIsUsingWeight	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0;
			pPlayList[i].m_bIsShuffle		= READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0;
		}
		SetPlayList( pPlayList );
		AkFree( g_DefaultPoolId, pPlayList );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

AKRESULT CAkMusicRanSeqCntr::Init()
{
	AKRESULT eResult = CAkMusicNode::Init();
	return eResult;
}

void CAkMusicRanSeqCntr::Term()
{
	FlushPlaylist();
}

void CAkMusicRanSeqCntr::FlushPlaylist()
{
	for( AkRSList::Iterator iter = m_playListRoot.m_listChildren.Begin(); iter != m_playListRoot.m_listChildren.End(); ++iter )
	{
		if(*iter)
		{
			AkDelete( g_DefaultPoolId, *iter );
		}
	}
	m_playListRoot.m_listChildren.RemoveAll();
	m_playListRoot.Clear();
}

// Return the node category.
AkNodeCategory CAkMusicRanSeqCntr::NodeCategory()
{
    return AkNodeCategory_MusicRanSeqCntr;
}

// Hierarchy enforcement: Music RanSeq Cntr can only have Segments as parents.
AKRESULT CAkMusicRanSeqCntr::CanAddChild(
    CAkAudioNode * in_pAudioNode 
    )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		MONITOR_ERRORMSG2( L"Too many children in one single container.", L"" );
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicSegment)
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

CAkMatrixAwareCtx * CAkMusicRanSeqCntr::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams,
    CAkSegmentBucket *& out_pFirstRelevantBucket
    )
{
    return CreateSequenceCtx( in_pParentCtx, in_GameObject, in_rUserparams, out_pFirstRelevantBucket );
}

CAkSequenceCtx * CAkMusicRanSeqCntr::CreateSequenceCtx( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams,
    CAkSegmentBucket *& out_pFirstRelevantBucket
    )
{
    CAkSequenceCtx * pCtx = AkNew( g_DefaultPoolId, CAkSequenceCtx( 
        this,
        in_pParentCtx ) );
    if ( pCtx )
    {
		pCtx->AddRef();
        if ( pCtx->Init( in_GameObject,
                         in_rUserparams,
                         out_pFirstRelevantBucket ) == AK_Success )
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

AKRESULT CAkMusicRanSeqCntr::Play( AkPBIParams& in_rPBIParams )
{
    // Create a top-level sequence context (that is, attached to the Music Renderer).
    CAkSegmentBucket * pFirstRelevantBucket;
    CAkSequenceCtx * pCtx = CreateSequenceCtx( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams, pFirstRelevantBucket );
    if ( pCtx )
    {
        AKASSERT( pFirstRelevantBucket &&
                  pFirstRelevantBucket->SegmentCtx() );

		// Complete initialization of the sequence.
		pCtx->EndInit();

        // Do not set source offset: let it start playback at the position specified by the sequence's 
        // transition rules.
        AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
        fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
        // Set fade offset to segment context's look-ahead time.
        fadeParams.iFadeOffset      = pFirstRelevantBucket->SegmentCtx()->GetStreamingLookAheadTime();
        return pCtx->_Play( fadeParams );
    }
    return AK_Fail;
}

// Interface for Wwise
// ----------------------
AKRESULT CAkMusicRanSeqCntr::AddPlaylistItem(
    AkUniqueID          in_segmentID
    )
{
	return AK_Fail;
    //return ( m_playlist.AddLast( in_segmentID ) ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicRanSeqCntr::RemovePlaylistItem(
    AkUniqueID          in_segmentID
    )
{
	return AK_Fail;
    //return m_playlist.Remove( in_segmentID );
}

AKRESULT CAkMusicRanSeqCntr::SetPlayList(
		AkMusicRanSeqPlaylistItem*	in_pArrayItems
		)
{
	AKASSERT( in_pArrayItems );

	FlushPlaylist();

	AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
	m_playListRoot.AvoidRepeatCount( pItem->m_wAvoidRepeatCount );
	m_playListRoot.SetLoop( pItem->m_Loop );
	m_playListRoot.SetWeight( pItem->m_Weight );

	m_playListRoot.SetType( pItem->m_eRSType );
	m_playListRoot.IsUsingWeight( false );// will be set to true later on if at least one child is not set to default value
	m_playListRoot.RandomMode( pItem->m_bIsShuffle );
	m_playListRoot.PlaylistID( pItem->m_playlistItemID );
	if( (*pItem).m_NumChildren )
	{
		return AddPlaylistChildren( &m_playListRoot, in_pArrayItems, pItem->m_NumChildren );
	}
	return AK_Success;
}

AKRESULT CAkMusicRanSeqCntr::AddPlaylistChildren(	
		CAkRSSub*					in_pParent,	
		AkMusicRanSeqPlaylistItem*&	in_pArrayItems, 
		AkUInt32					in_ulNumItems 
		)
{
	for( AkUInt32 i = 0; i < in_ulNumItems; ++i )
	{
		AkMusicRanSeqPlaylistItem* pItem = in_pArrayItems++;
		if( pItem->m_SegmentID == AK_INVALID_UNIQUE_ID ) //if not segment
		{
			CAkRSSub* pSub = AkNew( g_DefaultPoolId, CAkRSSub( in_pParent ) );
			if( !pSub )
			{
				return AK_Fail;
			}
			if( !in_pParent->m_listChildren.AddLast( pSub ) )
			{
				AkDelete( g_DefaultPoolId, pSub );
				return AK_Fail;
			}

			pSub->AvoidRepeatCount( pItem->m_wAvoidRepeatCount );
			pSub->SetLoop( pItem->m_Loop );
			pSub->SetWeight( pItem->m_Weight );
			pSub->SetType( pItem->m_eRSType );
			pSub->IsUsingWeight( false );// will be set to true if one child is not set to default value
			pSub->RandomMode( pItem->m_bIsShuffle );
			pSub->PlaylistID( pItem->m_playlistItemID );
			if( (*pItem).m_NumChildren )
			{
				if( AddPlaylistChildren( pSub, in_pArrayItems, pItem->m_NumChildren ) != AK_Success )
				{
					return AK_Fail;
				}
			}
		}
		else
		{
			//this is a segment
			CAkRSSegment* pSeg = AkNew( g_DefaultPoolId, CAkRSSegment( in_pParent ) );
			if( !pSeg )
			{
				return AK_Fail;
			}
			if( !in_pParent->m_listChildren.AddLast( pSeg ) )
			{
				AkDelete( g_DefaultPoolId, pSeg );
				return AK_Fail;
			}
			pSeg->SetLoop( pItem->m_Loop );
			pSeg->SetWeight( pItem->m_Weight );
			pSeg->SetSegmentID( pItem->m_SegmentID );
			pSeg->PlaylistID( pItem->m_playlistItemID );

			in_pParent->WasSegmentLeafFound();
		}

		if( pItem->m_Weight != DEFAULT_RANDOM_WEIGHT )
		{
			in_pParent->IsUsingWeight( true );
		}
	}
	return AK_Success;
}

// Interface for Contexts
// ----------------------

// Get first level node by index.
AKRESULT CAkMusicRanSeqCntr::GetNodeAtIndex( 
    AkUInt16        in_index, 
    AkUInt16 &      io_uPlaylistIdx    // TODO Replace with Multiplaylist iterator.
    )
{
    // TMP.
    /*if ( in_index >= m_playlist.Length() )
    {
        AKASSERT( !"Invalid playlist index" );
        return AK_InvalidParameter;
    }*/
    io_uPlaylistIdx = in_index;
    return AK_Success;
}
