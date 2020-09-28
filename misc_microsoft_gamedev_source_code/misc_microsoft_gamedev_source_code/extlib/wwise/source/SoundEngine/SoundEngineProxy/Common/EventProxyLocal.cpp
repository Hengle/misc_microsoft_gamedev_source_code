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

#include "EventProxyLocal.h"
#include "AkEvent.h"
#include "AkAudiolib.h"


EventProxyLocal::EventProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Event );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkEvent::Create( in_id ) );
}

EventProxyLocal::~EventProxyLocal()
{
}

void EventProxyLocal::Add( AkUniqueID in_actionID )
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Add( in_actionID );
	}
}

void EventProxyLocal::Remove( AkUniqueID in_actionID )
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Remove( in_actionID );
	}
}

void EventProxyLocal::Clear()
{
	CAkEvent* pIndexable = static_cast<CAkEvent*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Clear();
	}
}
