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
// AkMusicTrack.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBankFloatConversion.h"
#include "AkMusicTrack.h"
#include "AkSound.h"    // for dynamic sound creation.
#include "AudiolibDefs.h"
#include "AkRandom.h"
#include "AkMusicRenderer.h"
#include "AkBus.h"
#include "AkSource.h"

#define PLAYLIST_INIT_SIZE ( DEFAULT_POOL_BLOCK_SIZE/( sizeof(AkTrackSrc) + sizeof(AkUInt32) ) )

CAkMusicTrack::CAkMusicTrack( 
    AkUniqueID in_ulID
    )
	: CAkSoundBase(in_ulID)
	, m_iLookAheadTime( 0 )
	, m_eRSType( AkMusicTrackRanSeqType_Normal )
	, m_uNumSubTrack( 0 )
	, m_SequenceIndex( -1 )
{
	Loop( true, true, 0 );
	m_mmapTrackPlaylist.Init( PLAYLIST_INIT_SIZE, AK_NO_MAX_LIST_SIZE, g_DefaultPoolId );
}

CAkMusicTrack::~CAkMusicTrack()
{
    RemoveAllSources( );

    m_arSrcInfo.Term( );
	AKVERIFY( m_mmapTrackPlaylist.Term() == AK_Success );
}

// Thread safe version of the constructor.
CAkMusicTrack * CAkMusicTrack::Create(
    AkUniqueID in_ulID
    )
{
	CAkMusicTrack * pTrack = AkNew( g_DefaultPoolId, CAkMusicTrack( in_ulID ) );
    if( pTrack )
	{
        if( pTrack->Init() != AK_Success )
		{
			pTrack->Release();
			pTrack = NULL;
		}
	}
    return pTrack;
}

AKRESULT CAkMusicTrack::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize, CAkUsageSlot* in_pUsageSlot, bool in_bIsPartialLoadOnly )
{	
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just read/skip it
	AkUInt32 ulID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	AkUInt32 numSources = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	for( AkUInt32 i = 0; i < numSources; ++i )
	{
		//Read Source info
		if( eResult == AK_Success )
		{
			AkBankSourceData oSourceInfo;
			eResult = CAkBankMgr::LoadSource(in_pData, in_ulDataSize, oSourceInfo);
			if (eResult != AK_Success)
				return eResult;

			if (oSourceInfo.m_pParam == NULL)
			{
				//This is a file source
				eResult = AddSource( oSourceInfo.m_MediaInfo.sourceID, oSourceInfo.m_PluginID, oSourceInfo.m_MediaInfo, oSourceInfo.m_audioFormat );
			}
			else
			{
				//This is a plugin
				eResult = AddPluginSource( oSourceInfo.m_MediaInfo.sourceID, oSourceInfo.m_PluginID, oSourceInfo.m_pParam, oSourceInfo.m_uSize );
			}

			if( eResult != AK_Success )
				return AK_Fail;
		}
	}

	//Read playlist
	AkUInt32 numPlaylistItem = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
	if( numPlaylistItem )
	{
		AkTrackSrcInfo* pPlaylist = ( AkTrackSrcInfo* )AkAlloc( g_DefaultPoolId, numPlaylistItem*sizeof( AkTrackSrcInfo ) );
		if( !pPlaylist )
			return AK_Fail;
		for( AkUInt32 i = 0; i < numPlaylistItem; ++i )
		{
			pPlaylist[i].trackID			= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			pPlaylist[i].sourceID			= READBANKDATA( AkUniqueID, in_pData, in_ulDataSize );
			pPlaylist[i].fPlayAt			= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fBeginTrimOffset	= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fEndTrimOffset		= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
			pPlaylist[i].fSrcDuration		= READBANKDATA( AkReal64, in_pData, in_ulDataSize );
		}
		AkUInt32 numSubTrack = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		eResult = SetPlayList( numPlaylistItem, pPlaylist, numSubTrack );
		AkFree( g_DefaultPoolId, pPlaylist );
	}

	if( eResult != AK_Success )
		return eResult;
	
	//ReadParameterNode
	eResult = SetNodeBaseParams( in_pData, in_ulDataSize, in_bIsPartialLoadOnly );

	if( in_bIsPartialLoadOnly )
	{
		//Partial load has been requested, probable simply replacing the actual source created by the Wwise on load bank.
		return eResult;
	}

	if( eResult == AK_Success )
	{
		m_Loop = READBANKDATA( AkUInt16, in_pData, in_ulDataSize );
		m_LoopMod.m_min = READBANKDATA( AkUInt16, in_pData, in_ulDataSize );
		m_LoopMod.m_max = READBANKDATA( AkUInt16, in_pData, in_ulDataSize );

		SetMusicTrackRanSeqType( ( AkMusicTrackRanSeqType )READBANKDATA( AkUInt32, in_pData, in_ulDataSize ) );
		LookAheadTime( READBANKDATA( AkTimeMs, in_pData, in_ulDataSize ) );
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}
// Return the node category.
AkNodeCategory CAkMusicTrack::NodeCategory()
{
    return AkNodeCategory_MusicTrack;
}

AKRESULT CAkMusicTrack::Play( AkPBIParams& in_rPBIParams )
{
    AKASSERT( !"Cannot play music tracks" );
	return AK_NotImplemented;
}

AKRESULT CAkMusicTrack::ExecuteAction( ActionParams& in_rAction )
{
	AKRESULT eResult = AK_Success;

	if( in_rAction.bIsMasterCall )
	{
		//Only global pauses should Pause a state transition
		bool l_bPause = false;
		if( in_rAction.eType == ActionParamType_Pause )
		{
			l_bPause = true;
		}
		PauseTransitions( l_bPause );
	}

	return eResult;
}

AKRESULT CAkMusicTrack::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	AKRESULT eResult = AK_Success;
	if( in_rAction.pGameObj == NULL )
	{
		//Only global pauses should Pause a state transition
		bool l_bPause = false;
		if( in_rAction.eType == ActionParamType_Pause )
		{
			l_bPause = true;
		}
		PauseTransitions( l_bPause );
	}

	return eResult;
}

// Wwise specific interface.
// -----------------------------------------
AKRESULT CAkMusicTrack::AddPlaylistItem(
		AkTrackSrcInfo &in_srcInfo
		)
{
	AkTrackSrc PlaylistRecord;
	PlaylistRecord.id =					in_srcInfo.sourceID;

	AKASSERT( in_srcInfo.fPlayAt + in_srcInfo.fBeginTrimOffset > - ((AkReal64)(1000.f/(AkReal64)AK_CORE_SAMPLERATE)) );
	PlaylistRecord.uClipStartPosition =	CAkTimeConv::MillisecondsToSamples( in_srcInfo.fPlayAt + in_srcInfo.fBeginTrimOffset );

	AkReal64 fClipDuration = in_srcInfo.fSrcDuration + in_srcInfo.fEndTrimOffset - in_srcInfo.fBeginTrimOffset;
	//AKASSERT( fClipDuration >= 0 );
	// Note: the UI sometimes pushes negative (or ~0) source duration. If it happens, ignore this playlist item.
	if ( fClipDuration <= 0 )
		return AK_Fail;
    PlaylistRecord.uClipDuration =		CAkTimeConv::MillisecondsToSamples( fClipDuration );

	PlaylistRecord.uSrcDuration =		CAkTimeConv::MillisecondsToSamples( in_srcInfo.fSrcDuration );

	AkInt32 iBeginTrimOffset = CAkTimeConv::MillisecondsToSamples( in_srcInfo.fBeginTrimOffset );
	PlaylistRecord.iSourceTrimOffset =	iBeginTrimOffset % (AkInt32)PlaylistRecord.uSrcDuration;
	AKASSERT( abs( (int)PlaylistRecord.iSourceTrimOffset ) < (AkInt32)PlaylistRecord.uSrcDuration );
	if ( PlaylistRecord.iSourceTrimOffset < 0 )
		PlaylistRecord.iSourceTrimOffset += PlaylistRecord.uSrcDuration;
	AKASSERT( PlaylistRecord.iSourceTrimOffset >= 0 );
	
	return m_mmapTrackPlaylist.Insert( in_srcInfo.trackID, PlaylistRecord );
}

AKRESULT CAkMusicTrack::SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		)
{
	AKRESULT eResult = AK_Success;
	m_mmapTrackPlaylist.RemoveAll();

	m_uNumSubTrack = in_uNumSubTrack;

	for( AkUInt32 i = 0; i < in_uNumPlaylistItem && eResult == AK_Success; ++i )
	{
		eResult = AddPlaylistItem( in_pArrayPlaylistItems[i] );
	}
	return eResult;
}

AKRESULT CAkMusicTrack::AddSource( 
	AkUniqueID      in_srcID,
    AkLpCtstr       in_pszFilename,
    AkPluginID      in_pluginID,
    AkAudioFormat & in_audioFormat
    )
{
	CAkSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there, if the source is twice in the same playlist, it is useless to copy it twice.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_pluginID, in_pszFilename, in_audioFormat );
            (*ppSource)->StreamingLookAhead( m_iLookAheadTime );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicTrack::AddSource( 
		AkUniqueID in_srcID, 
		AkUInt32 in_pluginID, 
		AkMediaInformation in_MediaInfo, 
		AkAudioFormat in_audioFormat
		)
{
    CAkSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		//Already there, if the source is twice in the same playlist, it is useless to copy it twice.
		return AK_Success;
	}
	else
	{
		ppSource = m_arSrcInfo.Set( in_srcID );
	}
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_pluginID, in_MediaInfo, in_audioFormat );
            (*ppSource)->StreamingLookAhead( m_iLookAheadTime );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicTrack::AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID	in_ulID, 
		void*		in_pParam, 
		AkUInt32	in_uSize 
		)
{
	CAkSource** ppSource = m_arSrcInfo.Set( in_srcID );
    if ( ppSource )
    {   
		*ppSource = AkNew( g_DefaultPoolId, CAkSource() );
		if(*ppSource)
		{
			(*ppSource)->SetSource( in_ulID, in_pParam, in_uSize );
		}
		else
		{
			m_arSrcInfo.Unset( in_srcID );
		}
    }
	return ( ppSource && *ppSource ) ? AK_Success : AK_Fail;
}

AKRESULT CAkMusicTrack::SetSrcParam(		// Set the parameter on an physical model source.
		AkUniqueID      in_srcID,			// Source ID
		AkPluginID		in_ID,				// Plug-in id.  Necessary for validation that the param is set on the current FX.
		AkPluginParamID in_ulParamID,		// Parameter id.
		void *			in_pParam,			// Pointer to a setup param block.
		AkUInt32		in_ulSize			// Size of the parameter block.
		)
{
	CAkSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
		return (*ppSource)->SetSrcParam( in_ID, in_ulParamID, in_pParam, in_ulSize );
	return AK_Fail;
}

void CAkMusicTrack::RemoveSource( 
    AkUniqueID in_srcID
    )
{
	// Remove correcponding PlaylistItems
	TrackPlaylist::IteratorEx it = m_mmapTrackPlaylist.BeginEx();
	while( it != m_mmapTrackPlaylist.End() )
	{
		if( (*it).item.id == in_srcID )
		{
			it = m_mmapTrackPlaylist.Erase( it);
		}
		else
		{
			++it;
		}
	}

	CAkSource** ppSource = m_arSrcInfo.Exists( in_srcID );
	if( ppSource )
	{
		AkDelete( g_DefaultPoolId, *ppSource );
		m_arSrcInfo.Unset( in_srcID );
	}
}

bool CAkMusicTrack::HasBankSource()
{ 
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		if( iter.pItem->item->HasBankSource() )
			return true;
	}
	return false;
}

void CAkMusicTrack::RemoveAllSources()
{
	m_uNumSubTrack = 0;
	m_mmapTrackPlaylist.RemoveAll();
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		AkDelete( g_DefaultPoolId, iter.pItem->item );
	}
    m_arSrcInfo.RemoveAll();
}

void CAkMusicTrack::IsZeroLatency( bool in_bIsZeroLatency )
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->IsZeroLatency( in_bIsZeroLatency );
	}
}

void CAkMusicTrack::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	m_iLookAheadTime = CAkTimeConv::MillisecondsToSamples( in_LookAheadTime * CAkMusicRenderer::StreamingLookAheadRatio() );
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->StreamingLookAhead( m_iLookAheadTime );
	}
}

AkObjectCategory CAkMusicTrack::Category()
{
	return ObjCategory_Track;
}

// Like ParameterNodeBase's, but does not check parent.
bool CAkMusicTrack::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;
	
	if( ParentBus() )
	{
		if( static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		{
			return true;
		}
	}
	return false;
}

// Interface for Contexts
// ----------------------

CAkSource* CAkMusicTrack::GetSourcePtr( AkUniqueID SourceID )
{
	CAkSource ** ppSource = m_arSrcInfo.Exists( SourceID );
	if( ppSource )
		return *ppSource;
	return NULL;
}

AkUInt16 CAkMusicTrack::GetNextRS()
{
	AkUInt16 uIndex = 0;
	switch( m_eRSType )
	{
	case AkMusicTrackRanSeqType_Normal:
		break;
		
	case AkMusicTrackRanSeqType_Random:
		if( m_uNumSubTrack )
			uIndex = (AkUInt16)( AKRANDOM::AkRandom() % m_uNumSubTrack );
		break;

	case AkMusicTrackRanSeqType_Sequence:
		++m_SequenceIndex;
		if( m_SequenceIndex >= m_uNumSubTrack )
		{
			m_SequenceIndex = 0;
		}
		uIndex = m_SequenceIndex;
		break;
	
	default:
		AKASSERT( !"Unknown MusicTrackRanSeqType" );
	}
	return uIndex;
}

AKRESULT CAkMusicTrack::PrepareData()
{
	AKRESULT eResult = AK_Success;
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		eResult = iter.pItem->item->PrepareData();
		if( eResult != AK_Success )
		{
			// undo what has been prepared up to now.
			for( SrcInfoArray::Iterator iterFlush = m_arSrcInfo.Begin(); iterFlush != iter; ++iterFlush )
			{
				iterFlush.pItem->item->UnPrepareData();
			}
			break;
		}
	}
	
	return eResult;
}

void CAkMusicTrack::UnPrepareData()
{
	for( SrcInfoArray::Iterator iter = m_arSrcInfo.Begin(); iter != m_arSrcInfo.End(); ++iter )
	{
		iter.pItem->item->UnPrepareData();
	}
}
