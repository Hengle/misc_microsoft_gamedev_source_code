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

#include "MusicSwitchProxyLocal.h"
#include "AkMusicSwitchCntr.h"
#include "AkAudiolib.h"

#ifdef WIN32
	#ifdef _DEBUG
		#define new DEBUG_NEW
	#endif
#endif

MusicSwitchProxyLocal::MusicSwitchProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkMusicSwitchCntr::Create( in_id ) );
}

MusicSwitchProxyLocal::~MusicSwitchProxyLocal()
{
}

void MusicSwitchProxyLocal::SetSwitchAssocs(
	AkUInt32 in_uNumAssocs,
	AkMusicSwitchAssoc* in_pAssocs
	)
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );
	if( pMusicSwitch )
	{
		pMusicSwitch->SetSwitchAssocs( in_uNumAssocs, in_pAssocs );
	}
}

void MusicSwitchProxyLocal::SetSwitchGroup( 
    AkUInt32 in_ulGroup, 
    AkGroupType in_eGroupType 
    )
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );
	if( pMusicSwitch )
	{
		pMusicSwitch->SetSwitchGroup( in_ulGroup, in_eGroupType );
	}
}

void MusicSwitchProxyLocal::SetDefaultSwitch( 
	AkUInt32 in_Switch 
	)
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );
	if( pMusicSwitch )
	{
		pMusicSwitch->SetDefaultSwitch( in_Switch );
	}
}

void MusicSwitchProxyLocal::ContinuePlayback( bool in_bContinuePlayback )
{
	CAkMusicSwitchCntr* pMusicSwitch = static_cast<CAkMusicSwitchCntr*>( GetIndexable() );
	if( pMusicSwitch )
	{
		pMusicSwitch->ContinuePlayback( in_bContinuePlayback );
	}
}


