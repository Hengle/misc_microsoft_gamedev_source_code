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

#include "RanSeqContainerProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"


RanSeqContainerProxyConnected::RanSeqContainerProxyConnected( AkUniqueID in_id )
	: m_proxyLocal( in_id )
{
}

RanSeqContainerProxyConnected::~RanSeqContainerProxyConnected()
{
}

void RanSeqContainerProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	ObjectProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IRanSeqContainerProxy::MethodMode:
		{
			RanSeqContainerProxyCommandData::Mode mode;
			in_rSerializer.Get( mode );

			m_proxyLocal.Mode( mode.m_eMode );
			break;
		}

	case IRanSeqContainerProxy::MethodIsGlobal:
		{
			RanSeqContainerProxyCommandData::IsGlobal isGlobal;
			in_rSerializer.Get( isGlobal );

			m_proxyLocal.IsGlobal( isGlobal.m_bIsGlobal );
			break;
		}

	case IRanSeqContainerProxy::MethodAddPlaylistItem:
		{
			RanSeqContainerProxyCommandData::AddPlaylistItem addPlaylistItem;
			in_rSerializer.Get( addPlaylistItem );
			
			m_proxyLocal.AddPlaylistItem( addPlaylistItem.m_elementID, addPlaylistItem.m_weight );

			break;
		}

	case IRanSeqContainerProxy::MethodRemovePlaylistItem:
		{
			RanSeqContainerProxyCommandData::RemovePlaylistItem removePlaylistItem;
			in_rSerializer.Get( removePlaylistItem );
			
			m_proxyLocal.RemovePlaylistItem( removePlaylistItem.m_elementID );

			break;
		}

	case IRanSeqContainerProxy::MethodClearPlaylist:
		{
			RanSeqContainerProxyCommandData::ClearPlaylist clearPlaylist;
			in_rSerializer.Get( clearPlaylist );
			
			m_proxyLocal.ClearPlaylist();

			break;
		}

	case IRanSeqContainerProxy::MethodResetPlayListAtEachPlay:
		{
			RanSeqContainerProxyCommandData::ResetPlayListAtEachPlay resetPlayListAtEachPlay;
			in_rSerializer.Get( resetPlayListAtEachPlay );
			
			m_proxyLocal.ResetPlayListAtEachPlay( resetPlayListAtEachPlay.m_bResetPlayListAtEachPlay );
			break;
		}

	case IRanSeqContainerProxy::MethodRestartBackward:
		{
			RanSeqContainerProxyCommandData::RestartBackward restartBackward;
			in_rSerializer.Get( restartBackward );
			
			m_proxyLocal.RestartBackward( restartBackward.m_bRestartBackward );
			break;
		}

	case IRanSeqContainerProxy::MethodContinuous:
		{
			RanSeqContainerProxyCommandData::Continuous continuous;
			in_rSerializer.Get( continuous );
			
			m_proxyLocal.Continuous( continuous.m_bIsContinuous );
			break;
		}

	case IRanSeqContainerProxy::MethodForceNextToPlay:
		{
			RanSeqContainerProxyCommandData::ForceNextToPlay forceNextToPlay;
			in_rSerializer.Get( forceNextToPlay );
			
			m_proxyLocal.ForceNextToPlay( forceNextToPlay.m_position, forceNextToPlay.m_gameObjPtr, forceNextToPlay.m_playingID );
			break;
		}

	case IRanSeqContainerProxy::MethodNextToPlay:
		{
			RanSeqContainerProxyCommandData::NextToPlay nextToPlay;
			in_rSerializer.Get( nextToPlay );
			
			out_rReturnSerializer.Put( m_proxyLocal.NextToPlay( nextToPlay.m_gameObjPtr ) );

			break;
		}

	case IRanSeqContainerProxy::MethodRandomMode:
		{
			RanSeqContainerProxyCommandData::RandomMode randomMode;
			in_rSerializer.Get( randomMode );
			
			m_proxyLocal.RandomMode( randomMode.m_eRandomMode );
			break;
		}

	case IRanSeqContainerProxy::MethodAvoidRepeatingCount:
		{
			RanSeqContainerProxyCommandData::AvoidRepeatingCount avoidRepeatingCount;
			in_rSerializer.Get( avoidRepeatingCount );
			
			m_proxyLocal.AvoidRepeatingCount( avoidRepeatingCount.m_count );
			break;
		}

	case IRanSeqContainerProxy::MethodSetItemWeight_withID:
		{
			RanSeqContainerProxyCommandData::SetItemWeight_withID setItemWeight_withID;
			in_rSerializer.Get( setItemWeight_withID );
			
			m_proxyLocal.SetItemWeight( setItemWeight_withID.m_itemID, setItemWeight_withID.m_weight );
			break;
		}

	case IRanSeqContainerProxy::MethodSetItemWeight_withPosition:
		{
			RanSeqContainerProxyCommandData::SetItemWeight_withPosition setItemWeight_withPosition;
			in_rSerializer.Get( setItemWeight_withPosition );
			
			m_proxyLocal.SetItemWeight( setItemWeight_withPosition.m_position, setItemWeight_withPosition.m_weight );
			break;
		}

	case IRanSeqContainerProxy::MethodLoop:
		{
			RanSeqContainerProxyCommandData::Loop loop;
			in_rSerializer.Get( loop );

			m_proxyLocal.Loop( loop.m_bIsLoopEnabled, loop.m_bIsLoopInfinite, loop.m_loopCount );
			break;
		}

	case IRanSeqContainerProxy::MethodTransitionMode:
		{
			RanSeqContainerProxyCommandData::TransitionMode transitionMode;
			in_rSerializer.Get( transitionMode );

			m_proxyLocal.TransitionMode( transitionMode.m_eTransitionMode );
			break;
		}

	case IRanSeqContainerProxy::MethodTransitionTime:
		{
			RanSeqContainerProxyCommandData::TransitionTime transitionTime;
			in_rSerializer.Get( transitionTime );

			m_proxyLocal.TransitionTime( transitionTime.m_transitionTime, transitionTime.m_rangeMin, transitionTime.m_rangeMax );
			break;
		}

	default:
		__base::HandleExecute( in_rSerializer, out_rReturnSerializer );
	}
}
