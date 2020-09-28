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

//////////////////////////////////////////////////////////////////////
//
// AkDynamicSequence.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "AkAudioMgr.h"
#include "AkPlayingMgr.h"
#include "AkDynamicSequence.h"
#include "AkDialogueEvent.h"
#include "AkAudioLibIndex.h"
#include "AkDynamicSequencePBI.h"

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
extern AkMemPoolId		g_DefaultPoolId;

CAkDynamicSequence::CAkDynamicSequence( AkPlayingID in_playingID, AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType )
	: CAkPBIAware( in_playingID ) // ID of dynamic sequence is its playing ID
	, m_eState( State_Stopped )
	, m_pGameObj( NULL )
	, m_ulPauseCount( 0 )
	, m_bClosed( false )
	, m_uSequenceID( AK_INVALID_SEQUENCE_ID )
	, m_eDynamicSequenceType( in_eDynamicSequenceType )
{
	m_playingItem.audioNodeID = AK_INVALID_UNIQUE_ID;

	m_queuedItem.audioNodeID = AK_INVALID_UNIQUE_ID;

	m_userParams.CustomParam.customParam = 0;
	m_userParams.CustomParam.ui32Reserved = 0;
	m_userParams.PlayingID = in_playingID;
}

CAkDynamicSequence::~CAkDynamicSequence()
{
	m_playList.Term();
	if ( m_pGameObj )
		m_pGameObj->Release();
}

CAkDynamicSequence* CAkDynamicSequence::Create( AkPlayingID in_PlayingID, AK::SoundEngine::DynamicSequence::DynamicSequenceType in_eDynamicSequenceType )
{
	CAkDynamicSequence* pAkDynamicSequence = AkNew( g_DefaultPoolId, CAkDynamicSequence( in_PlayingID, in_eDynamicSequenceType ) );

	if( pAkDynamicSequence && pAkDynamicSequence->Init() != AK_Success )
	{
		pAkDynamicSequence->Release();
		pAkDynamicSequence = NULL;
	}

	return pAkDynamicSequence;
}

AKRESULT CAkDynamicSequence::Init()
{
	AddToIndex();
	return AK_Success;
}

void CAkDynamicSequence::AddToIndex()
{
	AKASSERT( g_pIndex );
	g_pIndex->m_idxDynamicSequences.SetIDToPtr(this);
}

void CAkDynamicSequence::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxDynamicSequences.RemoveID(ID());
}

AkUInt32 CAkDynamicSequence::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDynamicSequences.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkDynamicSequence::Release() 
{ 
    AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxDynamicSequences.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    {
		RemoveFromIndex();
		AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

CAkPBI* CAkDynamicSequence::CreatePBI( CAkSoundBase* in_pSound, CAkSource* in_pSource, AkPBIParams& in_rPBIParams, AkPriority in_priority ) const
{
	return AkNew( RENDERER_DEFAULT_POOL_ID, CAkDynamicSequencePBI( 
		in_pSound,
		in_pSource,
		in_rPBIParams.pGameObj,
		*in_rPBIParams.pContinuousParams,
		in_rPBIParams.userParams,
		in_rPBIParams.playHistory,
		in_rPBIParams.bIsFirst,
		in_rPBIParams.sequenceID,
		in_rPBIParams.pInstigator,
		in_priority,
		m_eDynamicSequenceType ) );
}

void CAkDynamicSequence::SetGameObject( CAkRegisteredObj* in_pGameObj )
{
	AKASSERT( !m_pGameObj );
	m_pGameObj = in_pGameObj;
	if ( m_pGameObj )
		m_pGameObj->AddRef();
}

AkUniqueID CAkDynamicSequence::_GetNextToPlay( AkTimeMs & out_delay )
{
	m_queuedItem.audioNodeID = AK_INVALID_UNIQUE_ID;

	AkAutoLock<CAkLock> lockPlaylist( m_lockPlaylist );
	
	if ( m_eState != State_Stopped )
	{
		while ( !m_playList.IsEmpty() )
		{
			const AK::SoundEngine::DynamicSequence::PlaylistItem & item = m_playList[0];

			if ( item.audioNodeID == AK_INVALID_UNIQUE_ID )
			{
				CAkCntrHist HistArray;
				MONITOR_OBJECTNOTIF( m_userParams.PlayingID, m_pGameObj->ID(), m_userParams.CustomParam, AkMonitorData::NotificationReason_PlayFailed, HistArray, AK_INVALID_UNIQUE_ID, 0 );
				MONITOR_ERROR( AK::Monitor::ErrorCode_NothingToPlay );
				m_playList.Erase( 0 );
			}
			else
			{
				m_queuedItem = item;
				out_delay = item.msDelay;
				m_playList.Erase( 0 );
				break;
			}
		}

		// Usually, this code would right after the code that has called GetNextToPlay.
		// But to prevent race condition ( m_eState should not be set to State_Waiting outside the playlist lock),
		// We manage the Waiting state here.
		if ( m_queuedItem.audioNodeID == AK_INVALID_UNIQUE_ID )
			m_eState = State_Waiting;
	}

	return m_queuedItem.audioNodeID;
}

AkUniqueID CAkDynamicSequence::GetNextToPlay( AkTimeMs & out_delay )
{
	m_playingItem = m_queuedItem;
	return _GetNextToPlay( out_delay );
}

AKRESULT CAkDynamicSequence::Play()
{
	if ( ( m_eState == State_Stopped || m_eState == State_Waiting ) && m_ulPauseCount == 0 )
	{
		//m_playingItem.audioNodeID = AK_INVALID_UNIQUE_ID;

		m_eState = State_Playing;

		AKRESULT result = AK_Fail;

		// If the play of the next item failed, play the following one.
		// Repeat until there is an element to play or no more element to play in the list

		AkTimeMs delay = 0;

		do
		{
			AkUniqueID nextToPlay = _GetNextToPlay( delay );

			result = nextToPlay == AK_INVALID_UNIQUE_ID ? AK_Success : _PlayNode( nextToPlay, delay );

		} while( result != AK_Success );
	}

	return AK_Success;
}

AKRESULT CAkDynamicSequence::Stop()
{
	// Stop must proceed even if the state is Stopped, just in case the last time it has been stopped it was a break.
	// It will avoid sound being played after a stop all.
	return _Stop( AK_StopImmediate );
}

AKRESULT CAkDynamicSequence::Break()
{
	if ( m_eState != State_Stopped )
		return _Stop( AK_StopAfterCurrentItem );

	return AK_Success;
}

AKRESULT CAkDynamicSequence::_Stop( AkDynamicSequenceStopMode in_eStopMode )
{
	m_eState = State_Stopped;
	m_ulPauseCount = 0;

	g_pAudioMgr->ClearPendingItems( m_userParams.PlayingID );

	if ( m_queuedItem.audioNodeID != AK_INVALID_UNIQUE_ID && ( in_eStopMode == AK_StopImmediate ) )
	{
		CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_queuedItem.audioNodeID );
		if ( pNode )
		{
			pNode->Stop( m_pGameObj, m_userParams.PlayingID );
			pNode->Release();
		}

		m_queuedItem.audioNodeID = AK_INVALID_UNIQUE_ID;
	}

	if ( m_playingItem.audioNodeID != AK_INVALID_UNIQUE_ID )
	{		
		CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_playingItem.audioNodeID );
		if ( pNode )
		{
			if ( in_eStopMode == AK_StopImmediate )
			{
				pNode->Stop( m_pGameObj, m_userParams.PlayingID );
			}
			else
			{
				pNode->PlayToEnd( m_pGameObj, m_playingItem.audioNodeID, m_userParams.PlayingID );
			}
			pNode->Release();	
		}
	}

	return AK_Success;
}

void CAkDynamicSequence::_StopNoPropagation()
{
	m_eState = State_Stopped;
	m_ulPauseCount = 0;	
	m_queuedItem.audioNodeID = AK_INVALID_UNIQUE_ID;
}

AKRESULT CAkDynamicSequence::Pause()
{
	if ( m_eState == State_Stopped )
		return AK_Success;
		
	++m_ulPauseCount;

	if ( m_ulPauseCount == 1 )
	{
		g_pAudioMgr->PausePendingItems( m_userParams.PlayingID );

		if ( m_playingItem.audioNodeID != AK_INVALID_UNIQUE_ID )
		{		
			CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_playingItem.audioNodeID );

			if ( pNode )
			{
				pNode->Pause( m_pGameObj, m_userParams.PlayingID );
				pNode->Release();
			}
		}

		if ( m_queuedItem.audioNodeID != AK_INVALID_UNIQUE_ID )
		{
			CAkAudioNode* pNode  = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_queuedItem.audioNodeID );

			if ( pNode )
			{
				pNode->Pause( m_pGameObj, m_userParams.PlayingID );
				pNode->Release();
			}
		}
	}

	return AK_Success;
}

void CAkDynamicSequence::_PauseNoPropagation()
{
	if ( m_eState != State_Stopped )
		++m_ulPauseCount;
}

AKRESULT CAkDynamicSequence::Resume()
{
	if ( m_ulPauseCount == 0 )
		return AK_Success;
		
	if ( !--m_ulPauseCount )
	{
		if ( m_eState != State_Playing )
		{
			Play();
		}

		g_pAudioMgr->ResumePausedPendingItems( m_userParams.PlayingID );

		if ( m_playingItem.audioNodeID != AK_INVALID_UNIQUE_ID )
		{		
			CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_playingItem.audioNodeID );

			if ( pNode ) 
			{
				pNode->Resume( m_pGameObj, m_userParams.PlayingID );
				pNode->Release();
			}
		}

		if ( m_queuedItem.audioNodeID != AK_INVALID_UNIQUE_ID )
		{
			CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_queuedItem.audioNodeID );

			if ( pNode )
			{
				pNode->Resume( m_pGameObj, m_userParams.PlayingID );
				pNode->Release();
			}
		}
	}

	return AK_Success;
}

void CAkDynamicSequence::_ResumeNoPropagation()
{
	if ( m_ulPauseCount != 0 )
	{
		if ( !--m_ulPauseCount && m_eState != State_Playing )
			Play();
	}
}

void CAkDynamicSequence::ResumeWaiting()
{
	if ( m_eState == State_Waiting && !m_playList.IsEmpty() )
	{
		Play();
	}
}

AKRESULT CAkDynamicSequence::_PlayNode( AkUniqueID in_nodeID, AkTimeMs in_delay )
{
	AKASSERT(g_pIndex);

	AKRESULT eResult = AK_Success;

	CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( in_nodeID );

	if( pNode )
	{
		// Transition Parameters
		TransParams	Tparameters;
		Tparameters.TransitionTime = 0;
		Tparameters.eFadeCurve = AkCurveInterpolation_Linear;

		// Continuation List
		AkPathInfo	PathInfo = { NULL, AK_INVALID_UNIQUE_ID };

		ContParams continuousParams( &PathInfo );
		continuousParams.spContList.Attach( CAkContinuationList::Create() );
		if ( !continuousParams.spContList )
		{
			pNode->Release();
			return AK_Fail;
		}

		// PBI Params
		AkPBIParams pbiParams;
        
		pbiParams.eType = AkPBIParams::DynamicSequencePBI;
        pbiParams.pInstigator = this;
		pbiParams.userParams = m_userParams;
		pbiParams.ePlaybackState = PB_Playing;
		pbiParams.uFrameOffset = CAkTimeConv::MillisecondsToSamples( in_delay );
        pbiParams.bIsFirst = true;

		pbiParams.pGameObj = m_pGameObj;

		pbiParams.pTransitionParameters = &Tparameters;
        pbiParams.pContinuousParams = &continuousParams;

		// We must generate the sequence ID here, and not let the continuous PBI do it itself.
		// We cannot do it in the constructor as the constructor is in the game thread. (GetNewSequenceID is not thread safe)
		// so we do it here, the first time it is required.
		while( m_uSequenceID == AK_INVALID_SEQUENCE_ID ) // While and not If to pad the int32 overflow.
		{
			m_uSequenceID = CAkContinuousPBI::GetNewSequenceID();
		}
		pbiParams.sequenceID = m_uSequenceID;

		eResult = pNode->Play( pbiParams );

		pNode->Release();
	}
	else
	{
		CAkCntrHist HistArray;
		MONITOR_OBJECTNOTIF( m_userParams.PlayingID, m_pGameObj->ID(), m_userParams.CustomParam, AkMonitorData::NotificationReason_PlayFailed, HistArray, in_nodeID, 0 );
		MONITOR_ERROR( AK::Monitor::ErrorCode_SelectedNodeNotAvailable );
		eResult = AK_IDNotFound;
	}

	return eResult;
}

void CAkDynamicSequence::UnlockPlaylist()
{
	bool bIsWaiting = m_eState == State_Waiting;

	m_lockPlaylist.Unlock();

	if ( bIsWaiting )
	{
		// Queue a message for resuming waiting
		AkQueuedMsg Item;
		Item.type = QueuedMsgType_DynamicSequenceCmd;

		AddRef();
		Item.dynamicsequencecmd.pDynamicSequence = this;

		Item.dynamicsequencecmd.eCommand = AkQueuedMsg_DynamicSequenceCmd::ResumeWaiting;
		g_pAudioMgr->Enqueue(Item, AkQueuedMsg::Sizeof_DynamicSequenceCmd() );
	}
}

void CAkDynamicSequence::AllExec( ActionParamType in_eType, CAkRegisteredObj * in_pGameObj )
{
	// In this function, we simply change the state of the DynamicSequence.
	// We do not have to notify the playing nodes, as they will be notified by the normal process of the incoming event.
	// Making it this way will allow keeping transition time specified in the events/actions as well as maintaining the state of the Dynamic sequence.
	if( m_pGameObj == in_pGameObj || in_pGameObj == NULL )
	{
		switch( in_eType )
		{
		case ActionParamType_Stop:		
			_StopNoPropagation();
			break;

		case ActionParamType_Pause:
			_PauseNoPropagation();
			break;

		case ActionParamType_Resume:	
			_ResumeNoPropagation();
			break;
		}
	}
}

