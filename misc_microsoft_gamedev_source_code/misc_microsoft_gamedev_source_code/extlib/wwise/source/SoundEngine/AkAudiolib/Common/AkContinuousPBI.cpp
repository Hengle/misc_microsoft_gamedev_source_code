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
// AkContinuousPBI.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkSound.h"
#include "AkContinuousPBI.h"
#include "AkAudioLibIndex.h"
#include "AkEvent.h"
#include "AkActions.h"
#include "AkContinuationList.h"
#include "AkTransition.h"
#include "AkTransitionManager.h"
#include "AkPathManager.h"
#include "AkPlayingMgr.h"
#include "AkAudioMgr.h"
#include "AkMonitor.h"
#include "AkSwitchCntr.h"

extern CAkAudioMgr*		g_pAudioMgr;

#define MINIMAL_LENGTH_FOR_CROSSFADE 50 //in milliseconds

AkUniqueID CAkContinuousPBI::m_CalSeqID = AK_INVALID_SEQUENCE_ID + 1; 

CAkContinuousPBI::CAkContinuousPBI( CAkSoundBase*	in_pSound,		
								    CAkSource*		in_pSource,
									CAkRegisteredObj * in_pGameObj,
									ContParams&		in_Cparameters,
									UserParams&		in_rUserParams,
									PlayHistory&	in_rPlayHistory,
									bool			in_bIsFirst,
									AkUniqueID		in_SeqID,
                                    CAkPBIAware*    in_pInstigator,
									AkPriority      in_Priority,
									bool			in_bTargetFeedback)
:CAkPBI( in_pSound,		
		 in_pSource,
		 in_pGameObj,
		 in_rUserParams,
		 in_rPlayHistory,
		 in_SeqID,
		 in_Priority,
		 in_bTargetFeedback)
,m_spContList(in_Cparameters.spContList)
,m_eTransitionMode(Transition_Disabled)
,m_TransitionTime(0)
,m_ulNextElementToPlay(AK_INVALID_UNIQUE_ID)
,m_bIsFirstPlay(in_bIsFirst)
,m_bIsContinuousPaused(false)
,m_bIsNextPrepared(false)
,m_pInstigator( in_pInstigator )
{
	AKASSERT( m_pInstigator );
	m_pInstigator->AddRef();

	if( m_SeqID == AK_INVALID_SEQUENCE_ID )
	{
		//generate one
		m_SeqID = GetNewSequenceID();
	}

	m_ulPauseCount = in_Cparameters.ulPauseCount;

	AKASSERT ( g_pTransitionManager );
	AKASSERT ( g_pPathManager );
	// have we got any ?
	if( m_PBTrans.pvPSTrans == NULL )
	{
		// no, then grab this one
		m_PBTrans.pvPSTrans = in_Cparameters.pPlayStopTransition;
		// Setting it to NULL means we took it, important
		in_Cparameters.pPlayStopTransition = NULL;
		m_PBTrans.bIsPSTransFading = in_Cparameters.bIsPlayStopTransitionFading;
	}

	// have we got any ?
	if( m_PBTrans.pvPRTrans == NULL)
	{
		// no, then grab this one
		m_PBTrans.pvPRTrans = in_Cparameters.pPauseResumeTransition;
		// Setting it to NULL means we took it, important
		in_Cparameters.pPauseResumeTransition = NULL;
		m_PBTrans.bIsPRTransFading = in_Cparameters.bIsPauseResumeTransitionFading;
	}

	m_PlayHistoryForNextToPlay.Init();

	PrepareNextPlayHistory( in_rPlayHistory );
}

AKRESULT CAkContinuousPBI::Init( AkPathInfo* in_pPathInfo )
{
	AKRESULT eResult = CAkPBI::Init( in_pPathInfo );
	if( eResult == AK_Success )
	{
		AKRESULT	Result;//NOT TO BE RETURNED, error handled internally
		if( m_PBTrans.pvPSTrans )
		{
			Result = g_pTransitionManager->AddTransitionUser( m_PBTrans.pvPSTrans, this );
			if( Result == AK_Success )
			{
				if( !g_pTransitionManager->IsTerminated( m_PBTrans.pvPSTrans ) )
				{
					MonitorFade( AkMonitorData::NotificationReason_Fade_Started, UNKNOWN_FADE_TIME );
				}
				else
				{
					g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPSTrans, this );
					m_PBTrans.pvPSTrans = NULL;

					if( m_PBTrans.bIsPSTransFading )
					{
						m_eInitialState = PBI_InitState_Stopped;
						m_PBTrans.bIsPSTransFading = false;
					}
				}
			}
			else 
			{
				if(Result != AK_UserAlreadyInList)
				{
					m_PBTrans.pvPSTrans = NULL;
				}
				if(Result == AK_TransitionNotFound)
				{
					if( m_PBTrans.bIsPSTransFading )
					{
						m_eInitialState = PBI_InitState_Stopped;
						m_PBTrans.bIsPSTransFading = false;
					}
				}
			}
		}
		if( m_PBTrans.pvPRTrans )
		{
			Result = g_pTransitionManager->AddTransitionUser( m_PBTrans.pvPRTrans, this );
			if(Result == AK_Success)
			{
				if( !g_pTransitionManager->IsTerminated( m_PBTrans.pvPRTrans ) )
				{
					MonitorFade( AkMonitorData::NotificationReason_Fade_Started, UNKNOWN_FADE_TIME );
				}
				else
				{
					g_pTransitionManager->RemoveTransitionUser( m_PBTrans.pvPRTrans, this );
					m_PBTrans.pvPRTrans = NULL;

					if( m_PBTrans.bIsPRTransFading )
					{
						if( m_eInitialState == PBI_InitState_Playing )
						{
							m_eInitialState = PBI_InitState_Paused;
						}
						m_PBTrans.bIsPRTransFading = false;
					}
				}
			}
			else 
			{	
				if(Result != AK_UserAlreadyInList)
				{
					m_PBTrans.pvPRTrans = NULL;
				}
				if(Result == AK_TransitionNotFound)
				{
					if( m_PBTrans.bIsPRTransFading )
					{
						if( m_eInitialState == PBI_InitState_Playing )
						{
							m_eInitialState = PBI_InitState_Paused;
						}
						m_PBTrans.bIsPRTransFading = false;
					}
				}
			}
		}
		if( m_eInitialState == PBI_InitState_Playing && m_ulPauseCount != 0 && !m_PBTrans.pvPRTrans )
		{
			m_eInitialState = PBI_InitState_Paused;
		}
	}
	return eResult;
}

CAkContinuousPBI::~CAkContinuousPBI()
{
	AKASSERT( m_pInstigator );
	m_pInstigator->Release();
}

void CAkContinuousPBI::Term()
{
	AKASSERT( g_pAudioMgr );

	Lock();

	DecrementPlayCount();

	PrepareNextToPlay( false );

	// g_pAudioMgr may not exist anymore on terminaison of the audiolib
	if( !HasNextToPlay() && g_pAudioMgr)
	{
		// It is possible that an action owns this PBI, should be removed from the list
		g_pAudioMgr->ClearCrossFadeOccurence(this);
	}
	if( HasNextToPlay() && !m_bTerminatedByStop )//nothing to do if no suite or if was killed
	{
		CAkAudioNode* pNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_ulNextElementToPlay );
		if(pNode)
		{
			// create the action we need
			CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, m_spContList );
			if(pAction)
			{
				/*if(m_bIsContinuousPaused)
				{
					++m_ulPauseCount;
					m_bIsContinuousPaused = false;
				}*/
				pAction->SetPauseCount( m_ulPauseCount );
				pAction->SetHistory( m_PlayHistoryForNextToPlay );
				pAction->SetElementID( pNode->ID() );
                pAction->SetInstigator( m_pInstigator );

				AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( m_pGameObj ) );
				if( pPendingAction )
				{

					// copy the transitions we have
					if(
						pAction->SetPlayStopTransition( m_PBTrans.pvPSTrans, m_PBTrans.bIsPSTransFading, pPendingAction ) == AK_Success
						&&
						pAction->SetPauseResumeTransition( m_PBTrans.pvPRTrans, m_PBTrans.bIsPRTransFading, pPendingAction ) == AK_Success
						)
					{
						pAction->SetPathInfo(GetPathInfo());
						if( m_bPlayFailed )
						{
							pAction->SetIsFirstPlay( m_bIsFirstPlay );
							pAction->Delay( AK_NUM_VOICE_REFILL_FRAMES * AK_WAIT_BUFFERS_AFTER_PLAY_FAILED ); //WG-2352: avoid freeze on loop
																		                                      //WG-4724: Delay must be exactly the size of a
							                                                                                  //         buffer to avoid sample accurate glitches
							                                                                                  //         and Buffer inconsistencies
						}

						if(m_eTransitionMode == Transition_Delay)
						{
							pAction->Delay( CAkTimeConv::MillisecondsToSamples( m_TransitionTime ) );
						}
						else if( m_eTransitionMode == Transition_SampleAccurate )
						{
							pAction->SetSAInfo( m_SeqID );
						}

						pPendingAction->pAction = pAction;
						pPendingAction->UserParam = m_UserParams;

						g_pAudioMgr->EnqueueOrExecuteAction( pPendingAction );
						if( m_TransitionTime && m_ulPauseCount && !m_PBTrans.pvPRTrans )
						{
							g_pAudioMgr->PausePending( pPendingAction );
						}
					}
					else
					{
						AkDelete( g_DefaultPoolId, pPendingAction );
					}
				}
				// we are done with these
				// Must not term m_pContinuationList here, releasing pAction will
				pAction->Release();
			}

			if( m_bNeedNotifyEndReached )
			{
				m_bIsNotifyEndReachedContinuous = true;
			}

			pNode->Release();
		}
	}

	m_spContList = NULL;

	if( m_bNeedNotifyEndReached && m_bIsNotifyEndReachedContinuous )
	{
		Monitor(AkMonitorData::NotificationReason_EndReachedAndContinue);
		m_bNeedNotifyEndReached = false;
	}

	Unlock();

	CAkPBI::Term();
}

void CAkContinuousPBI::PrepareNextPlayHistory( PlayHistory& in_rPlayHistory )
{
	// Create the History of the next sound to play
	
	m_PlayHistoryForNextToPlay = in_rPlayHistory;
	AkUInt32& ulrCount = m_PlayHistoryForNextToPlay.HistArray.uiArraySize;

	while( ulrCount )
	{
		if( m_PlayHistoryForNextToPlay.IsContinuous( ulrCount -1 ) )
		{
			break;
		}
		else
		{
			--ulrCount;
		}
	}
}

void CAkContinuousPBI::PrepareNextToPlay( bool in_bIsPreliminary )
{
	Lock();
	if( !m_bIsNextPrepared && m_spContList )
	{
		while( !m_spContList->m_listItems.IsEmpty() )
		{
			CAkContinueListItem& item = m_spContList->m_listItems.Last();
			if( !( item.m_pMultiPlayNode ) )
			{
				AkUInt16 wPositionSelected = 0;
				CAkAudioNode * pNode = item.m_pContainer->GetNextToPlayContinuous( m_pGameObj, wPositionSelected, item.m_pContainerInfo, item.m_LoopingInfo );
				if(pNode)
				{
					m_PlayHistoryForNextToPlay.HistArray.aCntrHist[m_PlayHistoryForNextToPlay.HistArray.uiArraySize - 1] = wPositionSelected;
					m_ulNextElementToPlay = pNode->ID();
					m_eTransitionMode = item.m_pContainer->TransitionMode();
					if( m_eTransitionMode == Transition_CrossFade 
						|| m_eTransitionMode == Transition_Delay 
						|| m_eTransitionMode == Transition_TriggerRate )
					{
						m_TransitionTime = item.m_pContainer->TransitionTime();
					}
					else
					{
						m_TransitionTime = 0;
					}

					// Exit used if next was found
					m_bIsNextPrepared = true;
					pNode->Release();
					Unlock();
					return;
				}
				else
				{
					m_PlayHistoryForNextToPlay.RemoveLast();

					while(m_PlayHistoryForNextToPlay.HistArray.uiArraySize
						&& !m_PlayHistoryForNextToPlay.IsContinuous( m_PlayHistoryForNextToPlay.HistArray.uiArraySize-1 ) )
					{
						m_PlayHistoryForNextToPlay.RemoveLast();
					}
					m_spContList->m_listItems.RemoveLast();
				}
			}
			else // Encountered a switch block
			{
				if( in_bIsPreliminary )
				{
					CAkContinueListItem* pItemSeeker = &( item.m_pAlternateContList->m_listItems.Last() );
					while( pItemSeeker->m_pMultiPlayNode )
					{
						pItemSeeker = &( pItemSeeker->m_pAlternateContList->m_listItems.Last() );
					}

					if( pItemSeeker->m_pContainer->TransitionMode() != Transition_TriggerRate )
					{
						// we are in a switch and this has been called by a SA or crossfade command
						// we cannot prepare the next right away, so let's consider it is a normal transition(or a delay if still applicable)
						// here, don't set the m_bIsNextPrepared to true, it will be re-called later on
						Unlock();
						return;
					}
				}
				CAkSmartPtr<CAkContinuationList> l_spContList = item.m_pMultiPlayNode->ContGetList( item.m_pAlternateContList );
				m_spContList->m_listItems.RemoveLast();
				if( l_spContList )
				{
					// I am the chosen one, flush the old ContList and use the new one

					// Lets take the new list and lets continue playing!!!
					m_spContList = l_spContList;
				}
				else
				{
					// We are not the continuous one, so terminate normally with no next
					break;
				}
			}
		}

		m_spContList = NULL;
	}

	m_bIsNextPrepared = true;	
	// Exit used if next was not found
	Unlock();
}

void CAkContinuousPBI::SetEstimatedLength( AkTimeMs in_EstimatedLength )
{
	Lock();

	if( m_bWasStopped || m_bWasPreStopped )
	{
		Unlock();
		return;
	}

	PrepareNextToPlay( true );

	if ( m_eTransitionMode == Transition_CrossFade )
	{
		if ( in_EstimatedLength < MINIMAL_LENGTH_FOR_CROSSFADE )
		{
			Unlock();
			return;
		}
	}
	else if ( m_eTransitionMode != Transition_TriggerRate )
	{
		Unlock();
		return;
	}

	if( !HasNextToPlay() )
	{
		Unlock();
		return;
	}

	// create the action we need
	CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, m_spContList );
	if(pAction)
	{
		AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( m_pGameObj ) );
		if( pPendingAction )
		{
			pAction->SetPauseCount( m_ulPauseCount );
			pAction->SetHistory(m_PlayHistoryForNextToPlay);
			pAction->SetElementID(m_ulNextElementToPlay);
            pAction->SetInstigator( m_pInstigator );
			// copy the transitions we have
			// only pause/resume, since play/Stop will be used for the cross fade
			pAction->SetPauseResumeTransition( m_PBTrans.pvPRTrans, m_PBTrans.bIsPRTransFading, pPendingAction );
			pAction->SetPathInfo(GetPathInfo());

			AkUInt32 uDelaySamples;
			if ( m_eTransitionMode == Transition_CrossFade )
			{
				//Max transition time equals half of the length, avoiding having multiple instances all at once
				AkTimeMs TransitionTime = AkMin( m_TransitionTime, in_EstimatedLength / 2 );

				uDelaySamples = CAkTimeConv::MillisecondsToSamples( in_EstimatedLength - TransitionTime );

				pAction->SetFadeBack( this, TransitionTime );
			}
			else // TriggerRate
			{
				// add our own frame offset to the delay
				uDelaySamples = CAkTimeConv::MillisecondsToSamples( m_TransitionTime );
				if( GetFrameOffset() > 0 )
				{
					uDelaySamples += GetFrameOffset();
				}
				if ( uDelaySamples < AK_NUM_VOICE_REFILL_FRAMES )
					uDelaySamples = AK_NUM_VOICE_REFILL_FRAMES;
			}
			pAction->Delay( uDelaySamples );

			pPendingAction->pAction = pAction;
			pPendingAction->UserParam = m_UserParams;

			g_pAudioMgr->EnqueueOrExecuteAction( pPendingAction );
			if( (uDelaySamples >= AK_NUM_VOICE_REFILL_FRAMES) && m_ulPauseCount )
			{
				g_pAudioMgr->PausePending( pPendingAction );
			}
		}
		// we are done with these
		pAction->Release();
		m_bIsNotifyEndReachedContinuous = true;
	}

	m_spContList = NULL;

	m_ulNextElementToPlay = AK_INVALID_UNIQUE_ID;
	Unlock();
}

void CAkContinuousPBI::PrepareSampleAccurateTransition()
{
	Lock();
	if( m_bWasStopped )
	{
		Unlock();
		return;
	}

	PrepareNextToPlay( true );

	if( (m_eTransitionMode != Transition_SampleAccurate) || !HasNextToPlay() )
	{
		Unlock();
		return;
	}

	// create the action we need
	CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, m_spContList );
	if(pAction)
	{
		AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( m_pGameObj ) );
		if( pPendingAction )
		{
            pAction->SetPauseCount( m_ulPauseCount );
            pAction->SetHistory( m_PlayHistoryForNextToPlay );
            pAction->SetElementID( m_ulNextElementToPlay );
            pAction->SetInstigator( m_pInstigator );
		
			// copy the transitions we have
			if(
				pAction->SetPlayStopTransition( m_PBTrans.pvPSTrans, m_PBTrans.bIsPSTransFading, pPendingAction ) == AK_Success
				&&
				pAction->SetPauseResumeTransition( m_PBTrans.pvPRTrans, m_PBTrans.bIsPRTransFading, pPendingAction ) == AK_Success
				)
			{
				pAction->SetPathInfo(GetPathInfo());

				if( m_bWasPaused )
				{
					// here set the next to start as paused
					pAction->StartAsPaused();
				}

				pAction->SetSAInfo( m_SeqID );

				pPendingAction->pAction = pAction;
				pPendingAction->UserParam = m_UserParams;

				g_pAudioMgr->EnqueueOrExecuteAction(pPendingAction);
			}
			else
			{
				AkDelete( g_DefaultPoolId, pPendingAction );
			}
		}

		// we are done with these
		pAction->Release();
		m_bIsNotifyEndReachedContinuous = true;
	}

	m_spContList = NULL;

	m_ulNextElementToPlay = AK_INVALID_UNIQUE_ID;

	Unlock();
}

AKRESULT CAkContinuousPBI::_Stop(AkPBIStopMode in_eStopMode /*= AkPBIStopMode_Normal*/, bool in_bIsFromTransition /*= false*/)
{
	Lock();
	m_ulNextElementToPlay = AK_INVALID_UNIQUE_ID;
	m_bIsNextPrepared = true;

	m_spContList = NULL;

	AKRESULT l_eResult = CAkPBI::_Stop( in_eStopMode, in_bIsFromTransition );
	Unlock();
	return l_eResult;
}

AKRESULT CAkContinuousPBI::PlayToEnd( CAkAudioNode * in_pNode )
{
	// 1 - Tell to not launch next
	// probably includes clearing information for next(maybe deleting it)
	// and setting next to none

	Lock();
	//Don't change future playback if it is targetting a node that should not be modified by this playToEnd command

	CAkAudioNode * pNextToPlay = NULL;
	if ( HasNextToPlay() )
		pNextToPlay = g_pIndex->m_idxAudioNode.GetPtrAndAddRef( m_ulNextElementToPlay );

	if(		!m_bIsNextPrepared 
		||	!HasNextToPlay() 
		||	(static_cast<CAkParameterNode*>( pNextToPlay )->IsOrIsChildOf( in_pNode->ID() ) )
		)
	{
		m_ulNextElementToPlay = AK_INVALID_UNIQUE_ID;
		m_bIsNextPrepared = false;

		if( m_spContList )
		{
			while( !m_spContList->m_listItems.IsEmpty() )
			{
				CAkContinueListItem& item = m_spContList->m_listItems.Last();
				if( !( item.m_pMultiPlayNode ) )
				{
					if( item.m_pContainer->IsOrIsChildOf( in_pNode->ID() ) )
					{
						m_PlayHistoryForNextToPlay.RemoveLast();
						while(m_PlayHistoryForNextToPlay.HistArray.uiArraySize
						&& !m_PlayHistoryForNextToPlay.IsContinuous( m_PlayHistoryForNextToPlay.HistArray.uiArraySize-1 ) )
						{
							m_PlayHistoryForNextToPlay.RemoveLast();
						}
						m_spContList->m_listItems.RemoveLast();
					}
					else
					{
						break;
					}
				}
				else // Encountered a switch block
				{
					m_spContList->m_listItems.RemoveAll();
				}
			}
			if( m_spContList->m_listItems.Length() == 0 )
			{
				m_spContList = NULL;
			}
		}
	}

	if ( pNextToPlay )
		pNextToPlay->Release();

	// 2 - Then call the native Play to end, which will shorten the duration of the sound if possible
	// (will set loop count to last loop if possible)
	
	AKRESULT l_eResult = CAkPBI::PlayToEnd( in_pNode );
	Unlock();
	return l_eResult;
}

void CAkContinuousPBI::SetPauseStateForContinuous(bool in_bIsPaused)
{
	Lock();
	m_bIsContinuousPaused = in_bIsPaused;
	Unlock();
}

#ifndef AK_OPTIMIZED

void CAkContinuousPBI::Monitor(AkMonitorData::NotificationReason in_Reason)
{
	if(in_Reason == AkMonitorData::NotificationReason_Play)
	{
		if(!m_bIsFirstPlay)
		{
			in_Reason = AkMonitorData::NotificationReason_PlayContinue;
		}
	}

	CAkPBI::Monitor(in_Reason);
}

AKRESULT CAkContinuousPBI::_StopAndContinue(
		AkUniqueID			in_ItemToPlay,
		AkUInt16				in_PlaylistPosition,
		CAkContinueListItem* in_pContinueListItem
		)
{
	AKASSERT(g_pIndex);
	AKRESULT eResult = AK_Success;
	Lock();
	if( m_spContList )
	{
		AKASSERT( !m_spContList->m_listItems.IsEmpty() );
		CAkContinueListItem & itemFirst = *(m_spContList->m_listItems.Begin());

		in_pContinueListItem->m_LoopingInfo = itemFirst.m_LoopingInfo;

		if( in_pContinueListItem->m_LoopingInfo.bIsEnabled )
		{
			if( itemFirst.m_pContainerInfo )
			{
				CAkSequenceInfo* pSeqInfo = static_cast<CAkSequenceInfo*>( itemFirst.m_pContainerInfo );
				if( pSeqInfo->m_i16LastPositionChosen == 0 && pSeqInfo->m_bIsForward )
				{
					in_pContinueListItem->m_LoopingInfo.lLoopCount += 1;
				}
			}
		}
		m_spContList = NULL;
	}
	/*else
	{
		in_pContinueListItem->m_LoopingInfo.bIsEnabled = false;
		in_pContinueListItem->m_LoopingInfo.bIsInfinite = false;
		in_pContinueListItem->m_LoopingInfo.lLoopCount = 1;
	}*/
	m_spContList.Attach( CAkContinuationList::Create() );
	if ( m_spContList )
	{
		m_spContList->m_listItems.AddLast(*in_pContinueListItem);
	}
	
	m_PlayHistoryForNextToPlay.Init();
	m_PlayHistoryForNextToPlay.Add( in_PlaylistPosition, true);
	m_ulNextElementToPlay = in_ItemToPlay;

	m_bIsNextPrepared = true;

	//We don't want it to crossfade nor delay, we want it to change right away
	m_eTransitionMode = Transition_Disabled;

	CAkPBI::_Stop( AkPBIStopMode_StopAndContinue, false );
	Unlock();
	
	return eResult;
}

#endif
