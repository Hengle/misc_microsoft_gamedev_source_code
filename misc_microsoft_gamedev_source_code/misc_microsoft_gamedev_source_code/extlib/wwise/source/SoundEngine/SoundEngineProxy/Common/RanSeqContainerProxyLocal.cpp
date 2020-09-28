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

#include "RanSeqContainerProxyLocal.h"

#include "AkRanSeqCntr.h"
#include "AkAudiolib.h"
#include "AkRegistryMgr.h"
#include "AkCritical.h"


RanSeqContainerProxyLocal::RanSeqContainerProxyLocal( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkRanSeqCntr::Create( in_id ) );
}

RanSeqContainerProxyLocal::~RanSeqContainerProxyLocal()
{
}

void RanSeqContainerProxyLocal::Mode( AkContainerMode in_eMode	)
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Mode( in_eMode );
	}
}

void RanSeqContainerProxyLocal::IsGlobal( bool in_bIsGlobal )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->IsGlobal( in_bIsGlobal );
	}
}

void RanSeqContainerProxyLocal::AddPlaylistItem( AkUniqueID in_elementID, AkUInt8 in_weight )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->AddPlaylistItem( in_elementID, in_weight );
	}
}

void RanSeqContainerProxyLocal::RemovePlaylistItem( AkUniqueID in_elementID )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RemovePlaylistItem( in_elementID );
	}
}

void RanSeqContainerProxyLocal::ClearPlaylist()
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->ClearPlaylist();
	}
}

void RanSeqContainerProxyLocal::ResetPlayListAtEachPlay( bool in_bResetPlayListAtEachPlay )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->ResetPlayListAtEachPlay( in_bResetPlayListAtEachPlay );
	}
}

void RanSeqContainerProxyLocal::RestartBackward( bool in_bRestartBackward )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RestartBackward( in_bRestartBackward );
	}
}

void RanSeqContainerProxyLocal::Continuous( bool in_bIsContinuous )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Continuous( in_bIsContinuous );
	}
}

void RanSeqContainerProxyLocal::ForceNextToPlay( AkInt16 in_position, AkGameObjectID in_gameObjPtr, AkPlayingID in_playingID )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // Need to protect object registry

		CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( in_gameObjPtr );
		if ( pGameObj )
		{
			pIndexable->ForceNextToPlay( in_position, pGameObj, in_playingID );
			pGameObj->Release();
		}
	}
}

AkInt16 RanSeqContainerProxyLocal::NextToPlay( AkGameObjectID in_gameObjPtr )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // Need to protect object registry

		CAkRegisteredObj * pGameObj = g_pRegistryMgr->GetObjAndAddref( in_gameObjPtr );
		if ( pGameObj )
		{
			AkInt16 iNext = pIndexable->NextToPlay( pGameObj );
			pGameObj->Release();
			return iNext;
		}
	}
	return 0;
}

void RanSeqContainerProxyLocal::RandomMode( AkRandomMode in_eRandomMode )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->RandomMode( in_eRandomMode );
	}
}

void RanSeqContainerProxyLocal::AvoidRepeatingCount( AkUInt16 in_count )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->AvoidRepeatingCount( in_count );
	}
}

void RanSeqContainerProxyLocal::SetItemWeight( AkUniqueID in_itemID, AkUInt8 in_weight )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->_SetItemWeight( in_itemID, in_weight );
	}
}

void RanSeqContainerProxyLocal::SetItemWeight( AkUInt16 in_position, AkUInt8 in_weight )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetItemWeight( in_position, in_weight );
	}
}

void RanSeqContainerProxyLocal::Loop( bool in_bIsLoopEnabled, bool in_bIsLoopInfinite, AkInt16 in_loopCount )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->Loop( in_bIsLoopEnabled, in_bIsLoopInfinite, in_loopCount );
	}
}

void RanSeqContainerProxyLocal::TransitionMode( AkTransitionMode in_eTransitionMode )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->TransitionMode( in_eTransitionMode );
	}
}

void RanSeqContainerProxyLocal::TransitionTime( AkTimeMs in_transitionTime, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax )
{
	CAkRanSeqCntr* pIndexable = static_cast<CAkRanSeqCntr*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->TransitionTime( in_transitionTime, in_rangeMin, in_rangeMax );
	}
}
