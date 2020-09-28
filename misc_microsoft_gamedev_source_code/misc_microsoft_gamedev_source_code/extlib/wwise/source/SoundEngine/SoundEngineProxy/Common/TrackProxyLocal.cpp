/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/


#include "stdafx.h"

#include "TrackProxyLocal.h"
#include "AkMusicTrack.h"
#include "AkAudiolib.h"

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


TrackProxyLocal::TrackProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicTrack::Create( in_id ) );
}

TrackProxyLocal::~TrackProxyLocal()
{
}

AKRESULT TrackProxyLocal::AddSource(
		AkUniqueID      in_srcID,
        AkLpCtstr       in_pszFilename,
        AkPluginID      in_pluginID,
        AkAudioFormat & in_audioFormat
        )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
#ifdef WIN32

#pragma message( "Remove this when AL is built in non-unicode" )

#ifdef _UNICODE
		return pIndexable->AddSource( in_srcID, in_pszFilename, in_pluginID, in_audioFormat );
#else
		USES_CONVERSION;
		return pIndexable->AddSource( in_srcID, A2CW( in_szFileName ), in_pluginID, in_audioFormat );
#endif
#endif
	}
	return AK_Fail;
}

AKRESULT TrackProxyLocal::AddPluginSource( 
		AkUniqueID	in_srcID,
		AkPluginID	in_ulID, 
		void*		in_pParam, 
		AkUInt32	in_uSize 
		)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->AddPluginSource( in_srcID, in_ulID, in_pParam, in_uSize );
	}
	return AK_Fail;
}

void TrackProxyLocal::SetPlayList(
		AkUInt32		in_uNumPlaylistItem,
		AkTrackSrcInfo* in_pArrayPlaylistItems,
		AkUInt32		in_uNumSubTrack
		)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetPlayList( in_uNumPlaylistItem, in_pArrayPlaylistItems, in_uNumSubTrack );
	}
}


void TrackProxyLocal::SetSrcParam(	AkUniqueID	in_srcID,
									AkPluginID	in_ID,
									AkPluginParamID in_ParamID,
									void *		in_vpParam,
									AkUInt32	in_ulSize 
									)
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSrcParam( in_srcID, in_ID, in_ParamID, in_vpParam, in_ulSize );
	}
}

void TrackProxyLocal::RemoveSource( AkUniqueID in_srcID )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveSource( in_srcID );
	}
}

void TrackProxyLocal::RemoveAllSources()
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveAllSources();
	}
}

void TrackProxyLocal::Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_sLoopCount, AkInt16 in_sCountModMin, AkInt16 in_sCountModMax )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Loop( in_bIsLoopEnabled, in_bIsLoopInfinite, in_sLoopCount, in_sCountModMin, in_sCountModMax );
	}
}

void TrackProxyLocal::IsStreaming( bool in_bIsStreaming )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		//pIndexable->IsStreaming(in_bIsStreaming);
	}
}

void TrackProxyLocal::IsZeroLatency( bool in_bIsZeroLatency )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IsZeroLatency(in_bIsZeroLatency);
	}
}

void TrackProxyLocal::LookAheadTime( AkTimeMs in_LookAheadTime )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->LookAheadTime( in_LookAheadTime );
	}
}

void TrackProxyLocal::SetMusicTrackRanSeqType( AkMusicTrackRanSeqType in_eRSType )
{
	CAkMusicTrack* pIndexable = static_cast<CAkMusicTrack*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetMusicTrackRanSeqType( in_eRSType );
	}
}
