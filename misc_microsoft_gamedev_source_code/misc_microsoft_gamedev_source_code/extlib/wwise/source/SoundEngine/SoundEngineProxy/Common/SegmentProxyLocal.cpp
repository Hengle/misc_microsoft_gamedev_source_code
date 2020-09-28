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

#include "SegmentProxyLocal.h"

#include "AkMusicSegment.h"
#include "AkAudiolib.h"
#include "AkRegistryMgr.h"
#include "AkCritical.h"

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif


SegmentProxyLocal::SegmentProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicSegment::Create( in_id ) );
}

SegmentProxyLocal::~SegmentProxyLocal()
{
}

void SegmentProxyLocal::Duration(
        AkReal64 in_fDuration               // Duration in milliseconds.
        )
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Duration( in_fDuration );
	}
}

void SegmentProxyLocal::StartPos(
        AkReal64 in_fStartPos               // PlaybackStartPosition in milliseconds.
        )
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->StartPos( in_fStartPos );
	}
}

void SegmentProxyLocal::SetMarkers(
		AkMusicMarkerWwise*     in_pArrayMarkers, 
		AkUInt32                in_ulNumMarkers
		)
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetMarkers( in_pArrayMarkers, in_ulNumMarkers );
	}
}

void SegmentProxyLocal::RemoveMarkers()
{
	CAkMusicSegment* pIndexable = static_cast<CAkMusicSegment*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveMarkers();
	}
}
