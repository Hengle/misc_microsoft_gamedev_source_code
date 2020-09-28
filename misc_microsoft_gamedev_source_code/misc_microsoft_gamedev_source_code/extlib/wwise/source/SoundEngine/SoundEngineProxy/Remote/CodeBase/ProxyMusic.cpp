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

#include "ProxyMusic.h"
#include "ProxyFramework.h"
#include "CommandData.h"
#include "TrackProxyConnected.h"
#include "SegmentProxyConnected.h"
#include "MusicSwitchProxyConnected.h"
#include "MusicRanSeqProxyConnected.h"

extern AkExternalProxyHandlerCallback g_pExternalProxyHandler;

#define PROXY_CAST_ALLOCATION_CONSTRUCTOR( _Creation_Type_, _What_ ) \
	in_pProxyItem = (ProxyFramework::ID2ProxyConnected::Item *)  AkAlloc( in_PoolID, in_lProxyItemOffset + sizeof( _Creation_Type_ ) ); \
	if( in_pProxyItem )\
	{\
		::new( &(in_pProxyItem->Assoc.item) ) _What_;\
	}

#define PROXY_CAST_ALLOCATION( _Creation_Type_ )	PROXY_CAST_ALLOCATION_CONSTRUCTOR( _Creation_Type_, _Creation_Type_( in_Create.m_objectID ) )

static void ProxyHandler( ObjectProxyStoreCommandData::Create& in_Create, ProxyFramework::ID2ProxyConnected::Item *& in_pProxyItem, const long in_lProxyItemOffset, AkMemPoolId in_PoolID )
{
	switch( in_Create.m_eObjectType )
	{
		case ObjectProxyStoreCommandData::TypeMusicTrack:
			PROXY_CAST_ALLOCATION( TrackProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeMusicSegment:
			PROXY_CAST_ALLOCATION( SegmentProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeMusicRanSeq:
			PROXY_CAST_ALLOCATION( MusicRanSeqProxyConnected );
			break;

		case ObjectProxyStoreCommandData::TypeMusicSwitch:
			PROXY_CAST_ALLOCATION( MusicSwitchProxyConnected );
			break;
	}
	return;
}

void AK::ProxyMusic::Init()
{
	g_pExternalProxyHandler = ProxyHandler;
}

